# Contributing to Position-Independent Runtime

This guide explains the conventions, patterns, and rules you must follow when writing code for PIR. Read this before submitting any changes.

## Prerequisites

- **Compiler:** Clang/LLVM 21+ (enforced at compile-time; MSVC and GCC are not supported)
- **Build tools:** CMake 3.20+, Ninja 1.10+
- **Language:** C++23 (`-std=c++23`)

See [TOOLCHAIN.md](TOOLCHAIN.md) for installation instructions.

## Building

Configure and build using CMake presets:

```bash
# Windows x86_64 debug
cmake --preset windows-x86_64-debug
cmake --build --preset windows-x86_64-debug

# Linux aarch64 release
cmake --preset linux-aarch64-release
cmake --build --preset linux-aarch64-release
```

Available presets follow the pattern `{platform}-{arch}-{build_type}`:
- **Platforms:** `windows`, `linux`, `macos`, `uefi`
- **Architectures:** `i386`, `x86_64`, `armv7a`, `aarch64`
- **Build types:** `debug`, `release`

Output binaries go to `build/{build_type}/{platform}/{arch}/output.{exe|elf|efi}` (macOS has no extension).

## Running Tests

Tests are built into the output binary. Simply run it:

```bash
# Windows
./build/debug/windows/x86_64/output.exe

# Linux
./build/debug/linux/x86_64/output.elf

# macOS
./build/debug/macos/x86_64/output
```

Exit code `0` means all tests passed.

## Project Structure

```
include/                    # Headers (.h)
  core/                     # Platform-independent primitives
    types/                  # Embedded types, numeric types
    types/embedded/         # EMBEDDED_STRING, EMBEDDED_DOUBLE, EMBEDDED_ARRAY, EMBEDDED_FUNCTION_POINTER
  algorithms/               # DJB2, Base64
  crypt/                    # SHA-2, ChaCha20, ECC, HMAC
  io/                       # Console, FileSystem, Logger, Path
  memory/                   # Allocator
  network/                  # Socket, DNS, HTTP, TLS, WebSocket
  platform/                 # OS-specific abstractions
    windows/                # PEB, NTDLL, Kernel32, system calls
    linux/                  # Linux syscall wrappers
    macos/                  # macOS BSD syscall wrappers
    uefi/                   # EFI boot/runtime services
  system/                   # DateTime, Random, Process, Environment
  runtime.h                 # Top-level include (CORE + PLATFORM + RUNTIME)
src/                        # Implementations (.cc)
  (mirrors include/ layout)
tests/                      # Test suite
  pir_tests.h               # Master test runner
  tests.h                   # RunTest/RunTestSuite helpers
  *_tests.h                 # Individual test suites
  start.cc                  # Test entry point
cmake/                      # CMake modules, linker scripts, function.order
```

### Three-Layer Architecture

All code belongs to one of three layers. Respect the dependency direction -- upper layers depend on lower layers, never the reverse.

```
RUNTIME (Runtime Abstraction Layer)
  Cryptography, Networking, TLS 1.3
  Includes: runtime.h
      |
  depends on
      v
PLATFORM (Platform Abstraction Layer)
  Windows PEB/NTAPI, Linux/macOS syscalls, UEFI services
  Includes: platform.h
      |
  depends on
      v
CORE (Core Abstraction Layer)
  Types, Memory, Strings, Algorithms, Embedded types
  Includes: core/ headers
```

---

## Code Requirements Checklist

Every new file, function, or edit must satisfy the rules below. If a build or post-build check fails, one of these rules was violated.

### Position-Independence Rules

The binary must contain **only** a `.text` section. No `.rdata`, `.rodata`, `.data`, or `.bss`. The build system verifies this automatically via `cmake/VerifyPICMode.cmake`.

| Forbidden | Why | Use Instead |
|-----------|-----|-------------|
| String literals (`"hello"`) | Placed in `.rdata` | `"hello"_embed` |
| Wide string literals (`L"hello"`) | Placed in `.rdata` | `L"hello"_embed` |
| Floating-point literals (`3.14`) | Constant in `.rdata` | `3.14_embed` |
| Global/static variables | `.data` or `.bss` | Local variables on the stack |
| Function pointers (`&MyFunc`) | Relocation entry | `EMBED_FUNC(MyFunc)` |
| `const` arrays at file scope | `.rodata` | `MakeEmbedArray(...)` or stack-local arrays |
| STL containers/algorithms | Links CRT | Custom implementations in PIR |
| Exceptions (`throw`/`try`/`catch`) | `.pdata`/`.xdata` tables | Return error codes or `Result<T, E>` |
| RTTI (`dynamic_cast`, `typeid`) | `.rdata` typeinfo | Static dispatch |

**Embedded types quick reference:**

```cpp
auto msg  = "Hello, World!"_embed;              // String (CHAR)
auto wide = L"Hello"_embed;                     // String (WCHAR)
auto pi   = 3.14159_embed;                      // Double (IEEE-754 immediate)
auto fn   = EMBED_FUNC(MyFunction);             // Function pointer (PC-relative)

constexpr UINT32 table[] = {0x11, 0x22, 0x33};
auto embedded = MakeEmbedArray(table);           // Array (packed into machine words)
UINT32 val = embedded[0];                        // Unpacked at runtime
```

### Code Style Rules

- **Indentation:** Tabs (not spaces)
- **Braces:** Allman style -- opening brace on its own line for classes, functions, and control flow
- **Include guard:** `#pragma once` in every header
- **No namespaces:** Use `static` class methods instead of free functions in namespaces
- **No STL:** Everything is implemented from scratch
- **No exceptions:** Use `Result<T, E>` for fallible operations
- **Prefer `static` methods** on classes over free functions
- **Use `FORCE_INLINE`** for force-inlined functions
- **Use `NOINLINE`** when inlining must be prevented (e.g., for function pointer embedding)
- **Use `constexpr`** for compile-time-evaluable variables and functions. Use `consteval` when evaluation *must* happen at compile time (embedded type constructors, hash computations)
- **Cast to `USIZE`** when passing pointer/handle arguments to `System::Call`

### Naming Rules

**Types:**

| Kind | Convention | Examples |
|------|-----------|----------|
| Primitive typedefs | `UPPER_CASE` | `UINT32`, `INT64`, `WCHAR`, `PVOID`, `NTSTATUS`, `BOOL` |
| Pointer typedefs | `P` prefix (`PP` for double pointer) | `PCHAR`, `PWCHAR`, `PPVOID`, `PCCHAR` (const) |
| Classes | `PascalCase` or `UPPER_CASE` | `String`, `Allocator`, `NTDLL`, `Kernel32` |
| Structs (Windows-style) | `_NAME` with typedef | `typedef struct _OBJECT_ATTRIBUTES { ... } OBJECT_ATTRIBUTES;` |
| Template types | `UPPER_CASE` | `EMBEDDED_STRING<CHAR, 'H', 'e', 'l', 'l', 'o'>` |
| Enums | `UPPER_CASE` | `EVENT_TYPE` |

**Functions and variables:**

| Kind | Convention | Examples |
|------|-----------|----------|
| Class methods (static) | `PascalCase` | `String::Length()`, `NTDLL::ZwCreateFile()`, `Djb2::Hash()` |
| Local variables | `camelCase` | `allPassed`, `fileHandle`, `bufferSize` |
| Macro constants | `UPPER_CASE` | `HANDLE_FLAG_INHERIT`, `STARTF_USESTDHANDLES` |
| Attributes/macros | `UPPER_CASE` | `FORCE_INLINE`, `NOINLINE`, `STDCALL`, `ENTRYPOINT` |

**Files:**

| Kind | Convention | Examples |
|------|-----------|----------|
| Headers | `snake_case.h` | `embedded_string.h`, `windows_types.h` |
| Source files | `snake_case.cc` | `kernel32.cc`, `entry_point.cc` |
| Platform-specific | `name.platform.cc` | `allocator.windows.cc`, `syscall.linux.h` |
| Test files | `snake_case_tests.h` | `djb2_tests.h`, `socket_tests.h` |

### Include Rules

```cpp
#include "runtime.h"     // or "platform.h" or specific core headers
#include "tests.h"       // (test files only)
```

- `runtime.h` includes everything (CORE + PLATFORM + RUNTIME)
- `platform.h` includes CORE + PLATFORM
- Implementation files: include your own header first, then `platform.h` or `runtime.h` as needed

### Parameter & Return Rules

**`[[nodiscard]]`** -- Apply to every function whose return value must not be ignored. Since PIR has no exceptions, return codes are the only failure signal -- a missed check is a silent bug:

```cpp
// GOOD -- compiler warns if caller ignores the result
[[nodiscard]] BOOL Open();
[[nodiscard]] NTSTATUS ZwCreateFile(PPVOID FileHandle, ...);
[[nodiscard]] static IPAddress FromIPv4(UINT32 address);

// NOT needed -- void, pure setters, or side-effect-only functions
VOID Close();
VOID SetPort(UINT16 port);
```

Apply `[[nodiscard]]` to all functions returning: `BOOL` success/failure, `NTSTATUS`, `SSIZE` (negative = error), `Result<T, E>`, and factory methods returning objects that must be used.

**Pass by value** -- small types that fit in registers:

```cpp
UINT32 ComputeHash(UINT32 input);
BOOL IsValid(PVOID handle);
IPAddress FromIPv4(UINT32 address);       // Return small structs by value
```

**Pass by pointer** -- output parameters, nullable parameters, Windows API compatibility:

```cpp
NTSTATUS ZwCreateFile(PPVOID FileHandle, ...);           // Output parameter
NTSTATUS ZwCreateEvent(PPVOID EventHandle, UINT32 Access,
                       POBJECT_ATTRIBUTES ObjectAttributes,  // Nullable (may be nullptr)
                       EVENT_TYPE EventType, INT8 InitialState);
BOOL Read(PVOID buffer, UINT32 bufferSize);              // Buffer pointer
```

**Pass by reference** -- whenever a parameter must not be null. Only use pointers when `nullptr` is a valid, meaningful value:

```cpp
// GOOD -- reference guarantees non-null
Socket(const IPAddress &ipAddress, UINT16 port);
BOOL operator==(const IPAddress &other) const;
BOOL Send(const TlsBuffer &buffer);

// BAD -- pointer allows null when null is never valid
Socket(const IPAddress *ipAddress, UINT16 port);
BOOL Send(const TlsBuffer *buffer);
```

**`Result<T, E>`** -- Use for functions that can fail and need to return a value or an error:

```cpp
[[nodiscard]] Result<IPAddress, UINT32> Resolve(PCCHAR host)
{
    if (failed)
        return Result<IPAddress, UINT32>::Err(errorCode);
    return Result<IPAddress, UINT32>::Ok(address);
}

auto result = Resolve(hostName);
if (result.IsErr())
    return;                     // error path
IPAddress &ip = result.Value(); // borrow; Result still owns it

// Use Result<void, E> when there is no value to return
[[nodiscard]] Result<void, UINT32> Open();
```

Key `Result` behaviors:
- `Value()` returns a reference -- the `Result` owns the object and destroys it at scope exit
- Non-trivially destructible types: destructor called automatically
- Trivially destructible types: destructor is a no-op (zero codegen)
- `operator BOOL()` is implicit, so `if (result)` and `if (!result)` work naturally

### Memory & Resource Rules

PIR executes in constrained environments (injected shellcode, early boot, loaderless contexts). All code must follow strict memory discipline.

**Heap:**
- Avoid heap allocation unless there is no alternative
- Prefer stack-local variables and fixed-size buffers
- Embed objects as class members by value, not as pointers to heap-allocated objects
- When heap is unavoidable: allocate late, release early, never leak across boundaries

```cpp
class HttpClient
{
private:
    IPAddress ipAddress;    // Embedded by value, not IPAddress*
    TLSClient tlsContext;   // Embedded by value, not TLSClient*
    Socket socketContext;   // Embedded by value, not Socket*
    CHAR hostName[254];     // Fixed-size buffer, sized to actual need
};
```

**Stack:**
- Avoid large local arrays
- Be mindful of `EMBEDDED_STRING` temporaries (each materializes packed words on the stack)
- Avoid deep recursion -- prefer iterative algorithms
- Watch aggregate sizes -- large fixed-size members consume stack when instantiated as locals
- Test under `-Oz` (release builds) to catch stack issues early

**RAII pattern** -- Acquire resources in constructors, release in destructors:

```cpp
class MyResource
{
private:
    PVOID handle;

public:
    MyResource() : handle(nullptr) {}
    ~MyResource() { Close(); }

    // Non-copyable -- prevent double-close
    MyResource(const MyResource &) = delete;
    MyResource &operator=(const MyResource &) = delete;

    // Movable -- transfer ownership
    MyResource(MyResource &&other) noexcept
        : handle(other.handle)
    {
        other.handle = nullptr;
    }

    MyResource &operator=(MyResource &&other) noexcept
    {
        if (this != &other)
        {
            Close();
            handle = other.handle;
            other.handle = nullptr;
        }
        return *this;
    }

    BOOL IsValid() const { return handle != nullptr && handle != (PVOID)(SSIZE)(-1); }

    VOID Close()
    {
        if (IsValid())
        {
            NTDLL::ZwClose(handle);
            handle = nullptr;
        }
    }
};
```

RAII rules:
1. Destructor calls `Close()` -- resources released when the object leaves scope
2. Delete copy operations -- prevents double-close
3. Implement move semantics -- nullify source after move
4. `IsValid()` checks both `nullptr` and `-1` (Windows `INVALID_HANDLE_VALUE`)
5. Delete `new`/`delete` for stack-only types:

```cpp
class Socket
{
public:
    VOID *operator new(USIZE) = delete;
    VOID operator delete(VOID *) = delete;
};
```

**Secure cleanup** -- Cryptographic classes must zero memory on destruction:

```cpp
ChaCha20Encoder::~ChaCha20Encoder()
{
    Memory::Zero(this, sizeof(ChaCha20Encoder));
    this->initialized = false;
}
```

**Conditional ownership** -- Use an ownership flag when a class may or may not own its buffer:

```cpp
class TlsBuffer
{
private:
    PCHAR buffer;
    BOOL ownsMemory;

public:
    TlsBuffer() : buffer(nullptr), ownsMemory(true) {}
    TlsBuffer(PCHAR buf, INT32 size) : buffer(buf), ownsMemory(false) {}

    ~TlsBuffer() { if (ownsMemory) Clear(); }
};
```
### Platform-Specific Code Rules

Use preprocessor guards for platform/architecture-specific code:

```cpp
#if defined(PLATFORM_WINDOWS_X86_64)
    // Windows x86_64 only
#elif defined(PLATFORM_WINDOWS_I386)
    // Windows i386 only
#elif defined(PLATFORM_WINDOWS_AARCH64)
    // Windows ARM64 only
#elif defined(PLATFORM_LINUX_X86_64)
    // Linux x86_64 only
#elif defined(PLATFORM_MACOS_X86_64)
    // macOS x86_64 only
#elif defined(PLATFORM_MACOS_AARCH64)
    // macOS aarch64 only
#endif
```

Architecture-only guards:

```cpp
#if defined(ARCHITECTURE_X86_64)
#elif defined(ARCHITECTURE_I386)
#elif defined(ARCHITECTURE_AARCH64)
#elif defined(ARCHITECTURE_ARMV7A)
#endif
```

---

## Error Handling

PIR has no exceptions. Every fallible function returns either a `BOOL`, an `NTSTATUS`, an `SSIZE`, or a `Result<T, Error>`. A missed check is always a silent bug — use `[[nodiscard]]` to make the compiler enforce it.

### The `Error` Struct

`Error` is a plain struct defined in `include/core/error.h`. It represents a **single error code** — a `(Code, Platform)` pair identifying one failure point:

```cpp
struct Error
{
    ErrorCodes   Code;     // runtime enumerator OR raw OS error code cast to UINT32
    PlatformKind Platform; // which OS layer (explicit, not derived)

    Error(UINT32 code = 0, PlatformKind platform = PlatformKind::Runtime);
};
```

`sizeof(Error)` = **8 bytes**. Platform-specific errors are created via factory methods: `Error::Windows(ntstatus)`, `Error::Posix(errno)`, `Error::Uefi(efiStatus)`.

The `ErrorCodes` enum assigns a unique value to every failure point in the PIR runtime layer:

```
Range   Layer      ErrorCodes examples
1–15    Socket     Socket_WriteFailed_Send, Socket_ReadFailed_Timeout
16–22   TLS        Tls_OpenFailed_Handshake, Tls_WriteFailed_Send
23–32   WebSocket  Ws_WriteFailed, Ws_HandshakeFailed
33–38   DNS        Dns_ConnectFailed, Dns_ResolveFailed
```

`PlatformKind` values:
- `Runtime` (0) — PIR runtime layer; Code holds a named `ErrorCodes` enumerator
- `Windows` (1) — Code holds a raw `NTSTATUS` value; high bit set = failure
- `Posix`   (2) — Code holds `errno` as a positive `UINT32` (negated from the syscall return)
- `Uefi`    (3) — Code holds a raw `EFI_STATUS` value; high bit set = error

### Zero-Cost Error Storage in Result

`Result<T, Error>` stores a **single Error** directly — no chain, no overhead beyond `sizeof(Error)` (8 bytes). The compile-time type system (`[[nodiscard]]`, `operator BOOL`) enforces that callers check results, while the runtime cost is zero.

Non-Error types (e.g., `Result<T, UINT32>`) also store a single value directly.

### Construction Patterns

`Result::Err` has three overloads:

```cpp
// Single error (most common):
return Result<UINT32, Error>::Err(Error::Socket_WriteFailed_HandleInvalid);

// 2-arg compatibility (stores only the last code):
return Result<UINT32, Error>::Err(
    Error::Posix((UINT32)(-sent)),
    Error::Socket_WriteFailed_Send);

// Propagation compatibility (stores only the appended code):
return Result<UINT32, Error>::Err(writeResult, Error::Tls_WriteFailed_Send);
```

The 2-arg and propagation overloads exist for source compatibility. They store only the **outermost** (last) error code. OS error details should be logged at the point of failure.

**Windows NTDLL failure** — ntdll.cc packages the `NTSTATUS`:
```cpp
return Result<NTSTATUS, Error>::Err(
    Error::Windows((UINT32)status));
```

**Callers propagate** — pass the failed Result and a new code:
```cpp
auto evtResult = NTDLL::ZwCreateEvent(&handle, ...);
if (!evtResult)
    return Result<SSIZE, Error>::Err(evtResult, Error::Socket_ReadFailed_EventCreate);
```

**Pre-syscall guard failure** — pass a plain runtime code:
```cpp
return Result<UINT32, Error>::Err(Error::Socket_WriteFailed_HandleInvalid);
```

**Higher-layer propagation**:
```cpp
auto r = context.Write(buffer, size);
if (!r)
    return Result<UINT32, Error>::Err(r, Error::Tls_WriteFailed_Send);
```

### Querying the Error

The `Error()` method returns a reference to the stored Error:

```cpp
const Error &err = result.Error();
// err.Code     — ErrorCodes enumerator or raw OS code
// err.Platform — PlatformKind (Runtime, Windows, Posix, Uefi)
```

### Formatting the Error

Use the `%e` format specifier with `result.Error()` to print the error in a single `LOG_ERROR` call:

```cpp
LOG_ERROR("Operation failed (error: %e)", result.Error());
// Output examples:
//   "33"              — runtime code (decimal)
//   "0xC0000034[W]"   — Windows NTSTATUS (hex)
//   "111[P]"          — Posix errno (decimal)
```

Format rules:
- Runtime codes: decimal, no tag
- Windows/UEFI codes: hex with `0x` prefix, tagged `[W]` or `[U]`
- Posix codes: decimal, tagged `[P]`

### Rules Summary

- Always `[[nodiscard]]` on functions returning `Result<T, Error>`, `BOOL`, `NTSTATUS`, or `SSIZE`.
- Pass error codes directly to `Result::Err(...)`.
- When the error originates from an OS call, use the factory methods — `Error::Windows(ntstatus)`, `Error::Posix(errno)`, `Error::Uefi(efiStatus)`.
- Runtime-layer codes (`Socket_*`, `Tls_*`, `Ws_*`, `Dns_*`) use the default `PlatformKind::Runtime` — pass them bare: `Result::Err(Error::Socket_WriteFailed_Send)`.
- For guard failures (handle invalid, nullptr check) where no syscall was attempted, pass only a runtime code.
- Each layer adds only its own `ErrorCodes` values; never add another layer's codes.
- Use `Err(osCode, runtimeCode)` for fresh errors, `Err(failedResult, runtimeCode)` for propagation.
- Discard a `[[nodiscard]]` Result with `(void)` only when the error is intentionally ignored (e.g., in destructors / move-assignment).

---

## Adding a Windows API Wrapper

A core project goal is to provide comprehensive wrappers for all `ntdll.dll` exports and their underlying system calls. On x86_64 and i386 the wrappers use indirect syscalls (SSN + gadget); on ARM64 they call the resolved ntdll export directly. Contributions that add missing Zw*/Nt* wrappers are always welcome.

Windows APIs are resolved dynamically at runtime via hash-based PEB walking. There are two patterns:

### Kernel32 / Win32 APIs

**Header** (`include/platform/windows/kernel32.h`):
```cpp
class Kernel32
{
public:
    static BOOL MyFunction(UINT32 param1, PVOID param2);
};
```

**Source** (`src/platform/windows/kernel32.cc`):
```cpp
#define ResolveKernel32ExportAddress(functionName) \
    ResolveExportAddressFromPebModule(Djb2::HashCompileTime(L"kernel32.dll"), Djb2::HashCompileTime(functionName))

BOOL Kernel32::MyFunction(UINT32 param1, PVOID param2)
{
    return ((BOOL(STDCALL *)(UINT32, PVOID))
        ResolveKernel32ExportAddress("MyFunction"))(param1, param2);
}
```

### NTDLL / Zw* Syscall Wrappers

Zw* functions use indirect syscalls (x86_64/i386) or direct ntdll calls (ARM64):

```cpp
NTSTATUS NTDLL::ZwMyFunction(PVOID Param1, UINT32 Param2)
{
    SYSCALL_ENTRY entry = ResolveSyscall("ZwMyFunction");
    return entry.ssn != SYSCALL_SSN_INVALID
        ? System::Call(entry, (USIZE)Param1, (USIZE)Param2)
        : CALL_FUNCTION("ZwMyFunction", PVOID Param1, UINT32 Param2);
}
```

**Important:** All arguments passed to `System::Call` must be cast to `USIZE`.

## Writing Tests

Each test suite is a class with a `RunAll()` static method in `tests/<name>_tests.h`:

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
        RunTest(allPassed, EMBED_FUNC(TestEdgeCase), L"Edge case handled"_embed);

        if (allPassed)
            LOG_INFO("All MyFeature tests passed!");
        else
            LOG_ERROR("Some MyFeature tests failed!");

        return allPassed;
    }

private:
    static BOOL TestSomething()
    {
        auto msg = "test input"_embed;
        // ... test logic ...
        return true; // true = pass, false = fail
    }

    static BOOL TestEdgeCase()
    {
        return true;
    }
};
```

Then register it:

1. Add `#include "my_feature_tests.h"` in `tests/pir_tests.h`
2. Add `RunTestSuite<MyFeatureTests>(allPassed);` in the `RunPIRTests()` function under the appropriate layer comment (CORE, PLATFORM, or RAL)

## Common Pitfalls

1. **Inline asm register clobbers** -- On x86_64, all volatile registers (RAX, RCX, RDX, R8-R11) must be declared as outputs or clobbers in inline assembly
2. **Memory operands with RSP modification** -- Never use `"m"` (memory) constraints in asm blocks that modify RSP. Under `-Oz`, the compiler uses RSP-relative addressing which breaks after `sub rsp`
3. **i386 `EMBEDDED_STRING` indexing** -- Cast indices to `USIZE` when indexing `EMBEDDED_STRING` on 32-bit targets to avoid ambiguous overload between the custom `operator[]` and built-in pointer decay

## Submitting Changes

1. Ensure your code builds cleanly for at least one platform/architecture preset
2. Verify the post-build PIC verification passes (no data sections detected)
3. Run the test binary and confirm all tests pass
4. Follow the naming conventions and code style described above
