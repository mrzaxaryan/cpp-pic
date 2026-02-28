# Contributing to Position-Independent Runtime

## Quick Start

**Requirements:** Clang/LLVM 21+, CMake 3.20+, Ninja 1.10+, C++23. See [TOOLCHAIN.md](TOOLCHAIN.md) for installation.

```bash
# Build
cmake --preset {platform}-{arch}-{build_type}
cmake --build --preset {platform}-{arch}-{build_type}

# Test (exit code 0 = all pass)
./build/{build_type}/{platform}/{arch}/output.{exe|elf|efi}
```

Presets: `windows|linux|macos|uefi` x `i386|x86_64|armv7a|aarch64` x `debug|release`

## Project Structure

```
include/                    # Headers (.h)
  core/types/embedded/      # EMBEDDED_STRING, EMBEDDED_DOUBLE, EMBEDDED_ARRAY, EMBEDDED_FUNCTION_POINTER
  algorithms/ crypt/ io/ memory/ network/ platform/ system/
  runtime.h                 # Top-level include (CORE + PLATFORM + RUNTIME)
src/                        # Implementations (.cc), mirrors include/ layout
tests/                      # pir_tests.h (master), tests.h (helpers), *_tests.h (suites), start.cc (entry)
cmake/                      # CMake modules, linker scripts, function.order
```

Three-layer architecture (RUNTIME > PLATFORM > CORE) — upper layers depend on lower, never the reverse. See [README.md](README.md) for details.

---

## Rules

### The Golden Rule: No Data Sections

The binary must contain **only** a `.text` section. No `.rdata`, `.rodata`, `.data`, or `.bss`. Verified automatically by `cmake/VerifyPICMode.cmake`.

| Forbidden | Use Instead |
|-----------|-------------|
| `"hello"` string literals | `"hello"_embed` |
| `L"hello"` wide strings | `L"hello"_embed` |
| `3.14` float literals | `3.14_embed` |
| `&MyFunc` function pointers | `EMBED_FUNC(MyFunc)` |
| Global/static variables | Stack-local variables |
| `const` arrays at file scope | `MakeEmbedArray(...)` |
| STL containers/algorithms | Custom PIR implementations |
| Exceptions (`throw`/`try`/`catch`) | `Result<T, Error>` |
| Raw `BOOL`/`NTSTATUS`/`SSIZE` for fallible returns | `Result<T, Error>` or `Result<void, Error>` |
| RTTI (`dynamic_cast`, `typeid`) | Static dispatch |

**Embedded types quick reference:**

```cpp
auto msg  = "Hello, World!"_embed;              // EMBEDDED_STRING (CHAR)
auto wide = L"Hello"_embed;                     // EMBEDDED_STRING (WCHAR)
auto pi   = 3.14159_embed;                      // DOUBLE (IEEE-754 immediate)
auto fn   = EMBED_FUNC(MyFunction);             // Function pointer (PC-relative)

constexpr UINT32 table[] = {0x11, 0x22, 0x33};
auto embedded = MakeEmbedArray(table);           // EMBEDDED_ARRAY (packed words)
UINT32 val = embedded[0];                        // Unpacked at runtime
```

### Code Style

- **Indentation:** Tabs (not spaces)
- **Braces:** Allman style — opening brace on its own line
- **Include guard:** `#pragma once` in every header
- **No namespaces** — use `static` class methods instead
- **No STL, no exceptions, no RTTI**
- **`FORCE_INLINE`** for force-inlined functions, **`NOINLINE`** when inlining must be prevented
- **`constexpr`** for compile-time-evaluable values; **`consteval`** when evaluation *must* happen at compile time
- **Cast to `USIZE`** when passing pointer/handle arguments to `System::Call`
- **Includes:** `runtime.h` = everything; `platform.h` = CORE + PLATFORM; implementation files include own header first

### Naming

| Kind | Convention | Examples |
|------|-----------|----------|
| Primitive typedefs | `UPPER_CASE` | `UINT32`, `INT64`, `WCHAR`, `PVOID`, `BOOL` |
| Pointer typedefs | `P` prefix (`PP` double, `PC` const) | `PCHAR`, `PWCHAR`, `PPVOID`, `PCCHAR` |
| Classes | `PascalCase` or `UPPER_CASE` | `String`, `Allocator`, `NTDLL` |
| Structs (Windows-style) | `_NAME` with typedef | `typedef struct _OBJECT_ATTRIBUTES { ... } OBJECT_ATTRIBUTES;` |
| Template types | `UPPER_CASE` | `EMBEDDED_STRING<...>` |
| Enums | `UPPER_CASE` | `EVENT_TYPE` |
| Class methods (static) | `PascalCase` | `String::Length()`, `NTDLL::ZwCreateFile()` |
| Local variables | `camelCase` | `allPassed`, `fileHandle`, `bufferSize` |
| Macros/constants | `UPPER_CASE` | `FORCE_INLINE`, `NOINLINE`, `ENTRYPOINT` |
| Header files | `snake_case.h` | `embedded_string.h`, `windows_types.h` |
| Source files | `snake_case.cc` | `kernel32.cc`, `entry_point.cc` |
| Platform-specific | `name.platform.cc` | `allocator.windows.cc`, `syscall.linux.h` |
| Test files | `snake_case_tests.h` | `djb2_tests.h`, `socket_tests.h` |

### Parameters & Returns

| Style | When | Example |
|-------|------|---------|
| By value | Small register-sized types | `UINT32 ComputeHash(UINT32 input)` |
| By pointer | Output params, nullable, Windows API compat | `NTSTATUS ZwCreateFile(PPVOID FileHandle, ...)` |
| By reference | Non-null params (compile-time guarantee) | `Socket(const IPAddress &ipAddress, UINT16 port)` |

**`[[nodiscard]]`** — Apply to every fallible function and factory. PIR has no exceptions — a missed return check is a silent bug.

**`Result<T, Error>`** — **All fallible functions must return `Result<T, Error>`** (or `Result<void, Error>` when there is no value). Do not use raw `BOOL`, `NTSTATUS`, or `SSIZE` as return types for success/failure. This ensures a uniform error-handling interface across the codebase:

```cpp
[[nodiscard]] Result<IPAddress, Error> Resolve(PCCHAR host);

auto result = Resolve(hostName);
if (result.IsErr())
    return;
IPAddress &ip = result.Value();  // borrow; Result still owns it

// Use Result<void, Error> when there is no value to return
[[nodiscard]] Result<void, Error> Open();
```

Infallible functions (getters, pure computations, operators) return their value directly — they do not use `Result`.

### Platform-Specific Code

Use preprocessor guards:

```cpp
#if defined(PLATFORM_WINDOWS_X86_64)     // Platform + arch combos
#elif defined(PLATFORM_LINUX_AARCH64)
#endif

#if defined(ARCHITECTURE_X86_64)         // Architecture-only
#elif defined(ARCHITECTURE_AARCH64)
#endif
```

---

## Error Handling

PIR has no exceptions. **Every fallible function must return `Result<T, Error>`** (or `Result<void, Error>` when there is no value to return). Do not use raw `BOOL`, `NTSTATUS`, or `SSIZE` as return types for success/failure.

### The Error Struct

`Error` is a `(Code, Platform)` pair (8 bytes) defined in `include/core/error.h`:

- **Runtime codes** (`PlatformKind::Runtime`): named `ErrorCodes` enumerators — `Socket_WriteFailed_Send`, `Tls_OpenFailed_Handshake`, etc.
- **OS codes**: created via factories — `Error::Windows(ntstatus)`, `Error::Posix(errno)`, `Error::Uefi(efiStatus)`

### Construction Patterns

```cpp
// Single error (most common):
return Result<UINT32, Error>::Err(Error::Socket_WriteFailed_Send);

// OS error + runtime context:
return Result<UINT32, Error>::Err(Error::Posix((UINT32)(-sent)), Error::Socket_WriteFailed_Send);

// Propagation from a failed Result:
auto r = context.Write(buffer, size);
if (!r)
    return Result<UINT32, Error>::Err(r, Error::Tls_WriteFailed_Send);
```

### Formatting

Use `%e` with `result.Error()`:

```cpp
LOG_ERROR("Operation failed (error: %e)", result.Error());
// Runtime: "33"    Windows: "0xC0000034[W]"    Posix: "111[P]"    UEFI: "0x8000...[U]"
```

### Error Rules

- Always `[[nodiscard]]` on functions returning `Result<T, Error>`
- OS errors: use factory methods — `Error::Windows()`, `Error::Posix()`, `Error::Uefi()`
- Runtime errors: pass bare — `Result::Err(Error::Socket_WriteFailed_Send)`
- Each layer adds only its own `ErrorCodes` values
- Discard `[[nodiscard]]` Result with `(void)` only when intentional (destructors, move-assignment)

---

## Memory & Resources

### Heap & Stack

- **Avoid heap** unless no alternative. Prefer stack-local variables and fixed-size buffers
- **Embed by value**, not by pointer: `IPAddress ipAddress;` not `IPAddress *ipAddress;`
- **Watch stack size**: `EMBEDDED_STRING` temporaries materialize words on stack; avoid deep recursion

### Constructor Rules

Constructors must be **trivial and never fail**. All fallible work goes into a `[[nodiscard]]` factory:

```cpp
class MyClient
{
private:
    MyClient() : port(0), isConnected(false) {}  // trivial, cannot fail

public:
    [[nodiscard]] static Result<MyClient, Error> Create(PCCHAR url);
    [[nodiscard]] Result<void, Error> Open();
    [[nodiscard]] Result<void, Error> Close();
};

auto createResult = MyClient::Create((PCCHAR)url);
if (!createResult)
    return false;
MyClient &client = createResult.Value();
```

### RAII Pattern

Every resource-owning class follows this template:

```cpp
class MyResource
{
private:
    PVOID handle;

public:
    MyResource() : handle(nullptr) {}
    ~MyResource() { Close(); }

    // Non-copyable
    MyResource(const MyResource &) = delete;
    MyResource &operator=(const MyResource &) = delete;

    // Movable — transfer ownership, nullify source
    MyResource(MyResource &&other) noexcept : handle(other.handle) { other.handle = nullptr; }
    MyResource &operator=(MyResource &&other) noexcept
    {
        if (this != &other) { Close(); handle = other.handle; other.handle = nullptr; }
        return *this;
    }

    VOID Close()
    {
        if (handle != nullptr) { NTDLL::ZwClose(handle); handle = nullptr; }
    }
};
```

Rules: destructor calls `Close()`, copy deleted, move nullifies source, `Close()` sets handle to `nullptr`. Use `static_cast<T &&>()` instead of `std::move()`.

### Stack-Only Types

Delete heap allocation; keep placement new for `Result`:

```cpp
VOID *operator new(USIZE) = delete;
VOID operator delete(VOID *) = delete;
VOID *operator new(USIZE, PVOID ptr) noexcept { return ptr; }  // Result needs this
```

Examples: `Socket`, `File`, `Random`, `HttpClient`, `WebSocketClient`

### Secure Cleanup

Crypto classes zero all key material on destruction:

```cpp
~ChaCha20Encoder() { Memory::Zero(this, sizeof(ChaCha20Encoder)); initialized = false; }
```

### Conditional Ownership

Use a `BOOL ownsMemory` flag when a class may or may not own its buffer:

```cpp
TlsBuffer() : buffer(nullptr), ownsMemory(true) {}
TlsBuffer(PCHAR buf, INT32 size) : buffer(buf), ownsMemory(false) {}
~TlsBuffer() { if (ownsMemory) Clear(); }
```

---

## Patterns

### Static Class as Module

No namespaces. Classes with only `static` methods serve as modules:

```cpp
class Memory
{
public:
    FORCE_INLINE static PVOID Copy(PVOID dest, PCVOID src, USIZE count);
    FORCE_INLINE static PVOID Zero(PVOID dest, USIZE count);
};
```

Examples: `Memory`, `String`, `Console`, `Djb2`, `Logger`, `Math`, `Base64`

### Compile-Time Embedding

The `_embed` ecosystem converts literals into immediate values, eliminating `.rdata`:

| Type | Literal | Result |
|------|---------|--------|
| `EMBEDDED_STRING` | `"text"_embed` / `L"text"_embed` | Characters packed into machine words |
| `DOUBLE` | `3.14_embed` | IEEE-754 bits as `UINT64` immediate |
| `EMBEDDED_ARRAY` | `MakeEmbedArray(arr)` | Elements packed into machine words |
| `EMBEDDED_FUNCTION_POINTER` | `EMBED_FUNC(Fn)` | PC-relative offset, no relocation |

A **register barrier** (`__asm__ volatile("" : "+r"(word))`) prevents the compiler from coalescing values back into `.rdata`.

### Traits-Based Dispatch

Parameterize algorithm variants via traits structs instead of runtime branching:

```cpp
struct SHA256Traits { using Word = UINT32; static constexpr USIZE DIGEST_SIZE = 32; };
struct SHA384Traits { using Word = UINT64; static constexpr USIZE DIGEST_SIZE = 48; };
template <typename Traits> class SHABase { /* single implementation */ };
```

Examples: `SHA256Traits`/`SHA384Traits`, `UINT_OF_SIZE`, `VOID_TO_TAG`

### Variadic Template Type Erasure

Variadic templates at the API surface, type-erased into a fixed `Argument` array before calling a `NOINLINE` implementation. Prevents code bloat:

```cpp
template <TCHAR TChar, typename... Args>
static VOID Info(const TChar *format, Args... args)
{
    StringFormatter::Argument argArray[] = { StringFormatter::Argument(args)... };
    TimestampedLogOutput(prefix, format, argArray, sizeof...(Args));  // NOINLINE
}
```

Examples: `Logger::Info/Error/Warning/Debug`, `Console::WriteFormatted`, `StringFormatter::Format`

### Concepts and Constraints

C++20 concepts and `requires` clauses enforce type safety. Use Clang builtins, not STL `<type_traits>`:

| Builtin | Purpose |
|---------|---------|
| `__is_same_as(T, U)` | Type equality |
| `__is_trivially_destructible(T)` | Skip destructor when trivial |
| `__builtin_bit_cast(T, v)` | Bit-level reinterpretation (constexpr-safe) |
| `__builtin_bswap16/32/64(v)` | Byte order swapping |

### Guard and Validation

- **Factory-created types** (`Socket`, `TLSClient`): do **not** add `IsValid()` guards — the factory + RAII pattern ensures validity
- **Non-factory types** parsing external input: validate at entry, return `Result::Err` on failure
- Only validate at system boundaries — trust internal code

### Platform Dispatch

Two strategies: **conditional compilation** (`#if defined(PLATFORM_*)`) for small differences within one function, and **separate implementation files** (`src/platform/{windows,linux,macos}/`) when implementations diverge entirely. CMake selects the correct files.

---

## Windows API Wrappers

### Kernel32 / Win32

```cpp
// Header: include/platform/windows/kernel32.h
class Kernel32 { public: [[nodiscard]] static Result<void, Error> MyFunction(UINT32 param1, PVOID param2); };

// Source: src/platform/windows/kernel32.cc
Result<void, Error> Kernel32::MyFunction(UINT32 param1, PVOID param2)
{
    BOOL ok = ((BOOL(STDCALL *)(UINT32, PVOID))
        ResolveKernel32ExportAddress("MyFunction"))(param1, param2);
    if (!ok)
        return Result<void, Error>::Err(Error::Kernel32_MyFunctionFailed);
    return Result<void, Error>::Ok();
}
```

### NTDLL / Zw* Syscalls

Indirect syscalls on x86_64/i386, direct ntdll calls on ARM64:

```cpp
[[nodiscard]] Result<void, Error> NTDLL::ZwMyFunction(PVOID Param1, UINT32 Param2)
{
    SYSCALL_ENTRY entry = ResolveSyscall("ZwMyFunction");
    NTSTATUS status = entry.ssn != SYSCALL_SSN_INVALID
        ? System::Call(entry, (USIZE)Param1, (USIZE)Param2)
        : CALL_FUNCTION("ZwMyFunction", PVOID Param1, UINT32 Param2);
    if (status != STATUS_SUCCESS)
        return Result<void, Error>::Err(Error::Windows((UINT32)status));
    return Result<void, Error>::Ok();
}
```

All arguments to `System::Call` must be cast to `USIZE`.

---

## Writing Tests

Each test suite is a class in `tests/<name>_tests.h`:

```cpp
#pragma once
#include "runtime.h"
#include "tests.h"

class MyFeatureTests
{
public:
    static BOOL RunAll()
    {
        BOOL allPassed = true;
        LOG_INFO("Running MyFeature Tests...");
        RunTest(allPassed, EMBED_FUNC(TestSomething), L"Something works"_embed);
        if (allPassed) LOG_INFO("All MyFeature tests passed!");
        else LOG_ERROR("Some MyFeature tests failed!");
        return allPassed;
    }

private:
    static BOOL TestSomething()
    {
        auto msg = "test input"_embed;
        // ... test logic ...
        return true;  // true = pass, false = fail
    }
};
```

Register: add `#include "my_feature_tests.h"` and `RunTestSuite<MyFeatureTests>(allPassed);` in `tests/pir_tests.h` under the appropriate layer comment (CORE, PLATFORM, or RUNTIME).

---

## Common Pitfalls

1. **Inline asm register clobbers** — On x86_64, declare all volatile registers (RAX, RCX, RDX, R8-R11) as outputs or clobbers
2. **Memory operands with RSP modification** — Never use `"m"` constraints in asm blocks that modify RSP; under `-Oz` the compiler uses RSP-relative addressing
3. **i386 `EMBEDDED_STRING` indexing** — Cast indices to `USIZE` to avoid ambiguous overload between `operator[]` and pointer decay

## Submitting Changes

1. Build cleanly for at least one platform/architecture preset
2. Verify post-build PIC check passes (no data sections)
3. Run the test binary — all tests pass
4. Follow naming conventions and code style above
