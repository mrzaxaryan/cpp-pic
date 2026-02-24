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
- **Platforms:** `windows`, `linux`, `uefi`
- **Architectures:** `i386`, `x86_64`, `armv7a`, `aarch64`
- **Build types:** `debug`, `release`

Output binaries go to `build/{build_type}/{platform}/{arch}/output.{exe|elf|efi}`.

## Running Tests

Tests are built into the output binary. Simply run it:

```bash
# Windows
./build/debug/windows/x86_64/output.exe

# Linux
./build/debug/linux/x86_64/output.elf
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
  Windows PEB/NTAPI, Linux syscalls, UEFI services
  Includes: platform.h
      |
  depends on
      v
CORE (Core Abstraction Layer)
  Types, Memory, Strings, Algorithms, Embedded types
  Includes: core/ headers
```

## The Golden Rule: No Data Sections

**The binary must contain only a `.text` section.** No `.rdata`, `.rodata`, `.data`, or `.bss` sections are allowed. The build system verifies this automatically via `cmake/VerifyPICMode.cmake` and will fail if any data section is detected.

This means you **cannot** use:

| Forbidden | Why | Use Instead |
|-----------|-----|-------------|
| String literals (`"hello"`) | Placed in `.rdata` | `"hello"_embed` |
| Wide string literals (`L"hello"`) | Placed in `.rdata` | `L"hello"_embed` |
| Floating-point literals (`3.14`) | Constant in `.rdata` | `3.14_embed` |
| `switch` with jump tables | Jump table in `.rdata` | `if/else` chains, or compiler uses `-fno-jump-tables` |
| Global/static variables | `.data` or `.bss` | Local variables on the stack |
| Function pointers (`&MyFunc`) | Relocation entry | `EMBED_FUNC(MyFunc)` |
| `const` arrays at file scope | `.rodata` | `MakeEmbedArray(...)` or stack-local arrays |
| STL containers/algorithms | Links CRT | Custom implementations |
| Exceptions (`throw`/`try`/`catch`) | `.pdata`/`.xdata` tables | Return error codes |
| RTTI (`dynamic_cast`, `typeid`) | `.rdata` typeinfo | Static dispatch |

### Embedded Types Quick Reference

```cpp
// Strings -- characters packed into 64-bit immediates in the instruction stream
auto msg = "Hello, World!"_embed;
auto wide = L"Hello"_embed;

// Floating-point -- IEEE-754 bits as a 64-bit immediate
auto pi = 3.14159_embed;

// Function pointers -- PC-relative offset, no relocation
auto fn = EMBED_FUNC(MyFunction);

// Arrays -- elements packed into machine words at compile time
constexpr UINT32 table[] = {0x11, 0x22, 0x33};
auto embedded = MakeEmbedArray(table);
UINT32 val = embedded[0]; // Unpacked at runtime
```

## Naming Conventions

### Types

| Kind | Convention | Examples |
|------|-----------|----------|
| Primitive typedefs | `UPPER_CASE` | `UINT32`, `INT64`, `WCHAR`, `PVOID`, `NTSTATUS`, `BOOL` |
| Pointer typedefs | `P` prefix (or `PP` for double pointer) | `PCHAR`, `PWCHAR`, `PPVOID`, `PCCHAR` (const) |
| Classes | `PascalCase` or `UPPER_CASE` | `String`, `Allocator`, `NTDLL`, `Kernel32` |
| Structs (Windows-style) | `_NAME` with typedef | `typedef struct _OBJECT_ATTRIBUTES { ... } OBJECT_ATTRIBUTES;` |
| Template types | `UPPER_CASE` with template params | `EMBEDDED_STRING<CHAR, 'H', 'e', 'l', 'l', 'o'>` |
| Enums | `UPPER_CASE` | `EVENT_TYPE` |

### Functions and Variables

| Kind | Convention | Examples |
|------|-----------|----------|
| Class methods (static) | `PascalCase` | `String::Length()`, `NTDLL::ZwCreateFile()`, `Djb2::Hash()` |
| Local variables | `camelCase` | `allPassed`, `fileHandle`, `bufferSize` |
| Macro constants | `UPPER_CASE` | `HANDLE_FLAG_INHERIT`, `STARTF_USESTDHANDLES` |
| Attributes/macros | `UPPER_CASE` | `FORCE_INLINE`, `NOINLINE`, `STDCALL`, `ENTRYPOINT` |

### Files

| Kind | Convention | Examples |
|------|-----------|----------|
| Headers | `snake_case.h` | `embedded_string.h`, `windows_types.h` |
| Source files | `snake_case.cc` | `kernel32.cc`, `entry_point.cc` |
| Platform-specific | `name.platform.cc` | `allocator.windows.cc`, `syscall.linux.h` |
| Test files | `snake_case_tests.h` | `djb2_tests.h`, `socket_tests.h` |

## Platform-Specific Code

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
        BOOL allPassed = TRUE;

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
        // Use _embed for all string/float literals
        auto msg = "test input"_embed;
        // ... test logic ...
        return TRUE; // TRUE = pass, FALSE = fail
    }

    static BOOL TestEdgeCase()
    {
        return TRUE;
    }
};
```

Then register it:

1. Add `#include "my_feature_tests.h"` in `tests/pir_tests.h`
2. Add `RunTestSuite<MyFeatureTests>(allPassed);` in the `RunPIRTests()` function under the appropriate layer comment (CORE, PLATFORM, or RAL)

## Include Order

```cpp
#include "runtime.h"     // or "platform.h" or specific core headers
#include "tests.h"       // (test files only)
```

- `runtime.h` includes everything (CORE + PLATFORM + RUNTIME)
- `platform.h` includes CORE + PLATFORM
- For implementation files, include your own header first, then `platform.h` or `runtime.h` as needed

Use `#pragma once` as the include guard in all headers.

## Code Style

- **Indentation:** Tabs
- **Braces:** Allman style (opening brace on its own line) for classes, functions, and control flow
- **No namespaces:** Use static class methods instead of free functions in namespaces
- **No STL:** Everything is implemented from scratch
- **No exceptions:** Return error codes (`NTSTATUS`, `BOOL`, etc.)
- **Prefer `static` methods** on classes over free functions
- **Use `FORCE_INLINE`** for force inline functions
- **Use `NOINLINE`** when you need to prevent inlining (e.g., for function pointer embedding)
- **Cast to `USIZE`** when passing pointer arguments to syscall wrappers

## Reference Style

PIR has no heap-allocated smart pointers and no STL. Follow these rules for passing data between functions.

### Pass by Value

Use for small types that fit in registers. This is the default for primitives and small structs:

```cpp
UINT32 ComputeHash(UINT32 input);
BOOL IsValid(PVOID handle);
IPAddress FromIPv4(UINT32 address);       // Return small structs by value
IPAddress addr = DNS::Resolve(hostName);  // Factory methods return by value
```

### Pass by Pointer

Use for output parameters, nullable parameters, and Windows API compatibility:

```cpp
// Output parameter -- caller provides storage, callee fills it
NTSTATUS ZwCreateFile(PPVOID FileHandle, ...);

// Nullable parameter -- NULL means "not provided"
NTSTATUS ZwCreateEvent(PPVOID EventHandle, UINT32 Access,
                       POBJECT_ATTRIBUTES ObjectAttributes,  // may be NULL
                       EVENT_TYPE EventType, INT8 InitialState);

// Buffer pointers -- raw memory the callee reads/writes
BOOL Read(PVOID buffer, UINT32 bufferSize);
```

### Pass by const Reference

Use for larger objects that the callee only needs to read:

```cpp
Socket(const IPAddress &ipAddress, UINT16 port);
BOOL operator==(const IPAddress &other) const;
```

### Never Use

| Pattern | Why |
|---------|-----|
| `std::unique_ptr` / `std::shared_ptr` | No STL |
| `new` / `delete` for resource classes | Many classes delete these operators to enforce stack allocation |
| Raw `&function` pointers | Generates relocations; use `EMBED_FUNC(Function)` |
| Heap-allocated wrappers | Prefer embedding objects as class members by value |

### Embedding Objects as Members

Prefer embedding resources directly as class members rather than holding pointers to heap-allocated objects:

```cpp
class HttpClient
{
private:
    IPAddress ipAddress;    // Embedded by value, not IPAddress*
    TLSClient tlsContext;   // Embedded by value, not TLSClient*
    Socket socketContext;   // Embedded by value, not Socket*
    CHAR hostName[1024];    // Fixed-size buffer on the stack
};
```

This avoids heap allocation, simplifies lifetime management, and ensures automatic cleanup when the parent goes out of scope.

## RAII Pattern

PIR uses RAII (Resource Acquisition Is Initialization) for resource management, but adapted for a freestanding environment with **no exceptions** and **no STL**. The core idea remains: acquire resources in constructors, release them in destructors.

### Basic RAII Class Template

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
            Close();                    // Release current resource
            handle = other.handle;      // Take ownership
            other.handle = nullptr;     // Nullify source
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

### Key Rules

1. **Destructor calls `Close()`** -- Resources are released automatically when the object leaves scope
2. **Delete copy operations** -- Prevents two objects from owning the same handle
3. **Implement move semantics** -- Allows transferring ownership (nullify the source after move)
4. **`IsValid()` checks both `nullptr` and `-1`** -- Windows uses `(PVOID)-1` (`INVALID_HANDLE_VALUE`) as an error sentinel
5. **No exceptions** -- Every function returns an error code (`BOOL`, `NTSTATUS`). The caller must check
6. **Delete `new`/`delete` for stack-only types** -- Forces callers to use stack allocation

### Deleting Heap Allocation

Network and I/O classes enforce stack-only usage:

```cpp
class Socket
{
public:
    VOID *operator new(USIZE) = delete;
    VOID operator delete(VOID *) = delete;
    // ... rest of class
};
```

This prevents `new Socket(...)` and forces all instances onto the stack, where RAII cleanup is guaranteed.

### Secure Cleanup for Sensitive Data

Cryptographic classes zero their memory on destruction to prevent key material from lingering:

```cpp
ChaCha20Encoder::~ChaCha20Encoder()
{
    Memory::Zero(this, sizeof(ChaCha20Encoder));
    this->initialized = FALSE;
}
```

### Conditional Ownership

When a class may or may not own its buffer, use an ownership flag:

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

### Temporary Scoped Resources

For resources that don't warrant a full RAII wrapper (e.g., a one-off event handle), use explicit create-use-close with validation on every path:

```cpp
PVOID event = NULL;
NTSTATUS status = NTDLL::ZwCreateEvent(&event, EVENT_ALL_ACCESS, NULL, SynchronizationEvent, FALSE);
if (!NT_SUCCESS(status))
    return FALSE;

// ... use event ...

NTDLL::ZwClose(event);
return TRUE;
```

Since there are no exceptions, the only way to skip the `ZwClose` is an early `return` -- make sure every return path cleans up.

### Manual Allocation with `Allocator`

For dynamic memory, use `Allocator::AllocateMemory` / `Allocator::ReleaseMemory`. You must track the size yourself:

```cpp
USIZE size = 4096;
PVOID buffer = Allocator::AllocateMemory(size);
if (buffer == nullptr)
    return FALSE;

// ... use buffer ...

Allocator::ReleaseMemory(buffer, size);
```

There are no smart pointers. You are responsible for matching every allocation with a release.

## Common Pitfalls

1. **Forgetting `_embed`** -- Writing `"hello"` instead of `"hello"_embed` will put the string in `.rdata` and break the build
2. **Global/static variables** -- Any variable with static storage duration generates `.data` or `.bss`. Use stack locals instead
3. **Implicit float constants** -- Even `0.0` generates `.rdata`. Always use `0.0_embed`
4. **ARM64 syscalls** -- `System::Call` does not work on ARM64 Windows. ARM64 Zw* functions must resolve via `ResolveNtdllExportAddress` and call the ntdll function directly
5. **Inline asm register clobbers** -- On x86_64, all volatile registers (RAX, RCX, RDX, R8-R11) must be declared as outputs or clobbers in inline assembly
6. **Memory operands with RSP modification** -- Never use `"m"` (memory) constraints in asm blocks that modify RSP. Under `-Oz`, the compiler uses RSP-relative addressing which breaks after `sub rsp`

## Submitting Changes

1. Ensure your code builds cleanly for at least one platform/architecture preset
2. Verify the post-build PIC verification passes (no data sections detected)
3. Run the test binary and confirm all tests pass
4. Follow the naming conventions and code style described above
