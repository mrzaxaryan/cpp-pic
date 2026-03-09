# Contributing to Position-Independent Runtime

Thank you for your interest in contributing to PIR! This guide covers everything you need to build, develop, and submit changes.

## Table of Contents

- [Quick Start](#quick-start)
- [Toolchain Installation](#toolchain-installation)
- [IDE Configuration](#ide-configuration)
- [Project Structure](#project-structure)
- [The Golden Rule: No Data Sections](#the-golden-rule-no-data-sections)
- [Code Style](#code-style)
- [Documentation](#documentation)
- [Naming Conventions](#naming-conventions)
- [Parameters & Returns](#parameters--returns)
- [Error Handling](#error-handling)
- [Memory & Resources](#memory--resources)
- [Patterns](#patterns)
- [Windows API Wrappers](#windows-api-wrappers)
- [Writing Tests](#writing-tests)
- [Common Pitfalls](#common-pitfalls)
- [Submitting Changes](#submitting-changes)

---

## Quick Start

**Requirements:** To build this project, ensure the following tools are installed:

- **Clang/LLVM** 22+
- **CMake** 3.20+
- **Ninja** 1.10+
- **C++23** compatible compiler

*See **Toolchain Installation** below for setup instructions.*

```bash
# Build
cmake --preset {platform}-{arch}-{build_type}
cmake --build --preset {platform}-{arch}-{build_type}

# Test (exit code 0 = all pass)
./build/{build_type}/{platform}/{arch}/output.{exe|elf|efi}
```

**Presets:** Build presets follow this format:

* Platforms
    - `windows`
    - `linux`
    - `macos`
    - `ios`
    - `freebsd`
    - `uefi`

* Architectures
    - `i386`
    - `x86_64`
    - `armv7a`
    - `aarch64`

* Configurations
    - `debug`
    - `release`
---

## Toolchain Installation

### Linux (Ubuntu/Debian) / WSL

```bash
# Install dependencies
sudo apt update && sudo apt install -y xz-utils cmake ninja-build

# Download and install LLVM 22
wget -q --show-progress 'https://github.com/llvm/llvm-project/releases/download/llvmorg-22.1.0/LLVM-22.1.0-Linux-X64.tar.xz' -O /tmp/LLVM-22.1.0-Linux-X64.tar.xz
sudo mkdir -p /opt/llvm-22 && sudo tar xf /tmp/LLVM-22.1.0-Linux-X64.tar.xz -C /opt/llvm-22 --strip-components=1

# Add to PATH
echo 'export PATH=/opt/llvm-22/bin:$PATH' >> ~/.bashrc
source ~/.bashrc
```

Verify:

```bash
clang --version && clang++ --version && lld --version && cmake --version && ninja --version
```

### macOS

**Prerequisites:** [Homebrew](https://brew.sh/).

```bash
# Install all dependencies
brew install llvm cmake ninja
```

Add LLVM to your PATH:

```bash
# For Apple Silicon (M1/M2/M3/M4)
echo 'export PATH="/opt/homebrew/opt/llvm/bin:$PATH"' >> ~/.zshrc
source ~/.zshrc

# For Intel Macs
echo 'export PATH="/usr/local/opt/llvm/bin:$PATH"' >> ~/.zshrc
source ~/.zshrc
```

Verify:

```bash
clang --version && clang++ --version && ld64.lld --version && cmake --version && ninja --version
```

---

## IDE Configuration

### Visual Studio Code

This project is designed and optimized for [Visual Studio Code](https://code.visualstudio.com/). When you open the project in VSCode, you will be automatically prompted to install the recommended extensions.

### WSL Integration

If you are developing on **Windows using WSL**, navigate to the project directory inside WSL and run:

```bash
code .
```

**Prerequisites:**
- WSL properly configured on your Windows system
- QEMU, UEFI firmware, and disk image tools:
  ```bash
  sudo apt-get update && sudo apt-get install -y qemu-user-static qemu-system-x86 qemu-system-arm ovmf qemu-efi-aarch64 dosfstools mtools
  ```

For more information, see the [VSCode WSL documentation](https://code.visualstudio.com/docs/remote/wsl) and [.vscode/README.md](../.vscode/README.md).

---

## Project Structure

```
src/                        # Source layers
  core/                     # CORE layer (.h + .cc co-located)
    core.h                  # Aggregate header for CORE layer
    compiler/               # Compiler abstractions (FORCE_INLINE, NOINLINE, compiler runtime)
    memory/                 # Memory operations (Copy, Set, Compare, Zero)
    math/                   # Math utilities, bit operations, byte order
    binary/                 # Binary reader/writer
    types/                  # Primitives, Span, Result, Error, Double, IP address
      embedded/             # EMBEDDED_STRING, EMBEDDED_DOUBLE, EMBEDDED_ARRAY, EMBEDDED_FUNCTION_POINTER
    string/                 # String utilities and formatting
    algorithms/             # DJB2 hashing, Base64
    encoding/               # UTF-16
  platform/                 # PLATFORM layer
    platform.h              # Aggregate header for PLATFORM layer
    kernel/                 # Per-OS kernel interfaces
      windows/              # PEB, PE, NTDLL, System, Kernel32, types
      linux/                # Syscall numbers, System::Call, result conversion
      macos/                # Syscall numbers, System::Call, result conversion
      ios/                  # iOS wrappers (reuses macOS XNU syscall interface)
      solaris/              # Syscall numbers, System::Call, result conversion
      freebsd/              # Syscall numbers, System::Call, result conversion
      uefi/                 # EFI types, protocols, boot/runtime services
    memory/                 # Allocator (heap management)
      windows/              # NtAllocateVirtualMemory/NtFreeVirtualMemory
      posix/                # mmap/munmap
      uefi/                 # AllocatePool/FreePool
    console/                # Console, Logger
      windows/              # ZwWriteFile console output
      posix/                # write() syscall to STDOUT
      uefi/                 # SimpleTextOut protocol
    fs/                     # FileSystem, Path, Directory, DirectoryIterator
      windows/              # NtCreateFile, NtReadFile, NtWriteFile
      posix/                # open/read/write/close syscalls, posix_path
      uefi/                 # EFI_FILE_PROTOCOL, uefi_fs_helpers
    socket/                 # Socket
      windows/              # AFD (Auxiliary Function Driver)
      posix/                # Direct socket syscalls
      uefi/                 # EFI_TCP4/TCP6_PROTOCOL
    system/                 # DateTime, Environment, Process, Random
      windows/              # Windows-specific system operations
      posix/                # Shared POSIX date_time, process
      linux/                # Linux-specific environment, platform, process
      macos/                # macOS-specific environment, platform, process
      ios/                  # iOS-specific platform (ExitProcess, dyld_stub_binder)
      solaris/              # Solaris-specific environment, platform, process
      freebsd/              # FreeBSD-specific environment, platform, process
      uefi/                 # UEFI-specific system operations
  runtime/                  # RUNTIME layer (.h + .cc co-located)
    runtime.h               # Aggregate header (CORE + PLATFORM + RUNTIME)
    entry_point.cc          # Program entry point (no CRT)
    crypto/                 # SHA2, ECC, ChaCha20
    network/                # DNS, HTTP, WebSocket
      tls/                  # TLS 1.3 implementation
tests/                      # pir_tests.h (master), tests.h (helpers), *_tests.h (suites), start.cc (entry)
cmake/                      # CMake modules, linker scripts, function.order
```

Three-layer architecture (**RUNTIME → PLATFORM → CORE**).  
Upper layers depend on lower layers, never the reverse.

See [documentation](../README.md) for an overview.

---

## The Golden Rule: No Data Sections

The binary must contain **only** a `.text` section.  
No `.rdata`, `.rodata`, `.data`, or `.bss`.

This is automatically verified by `cmake/VerifyPICMode.cmake`.

| Forbidden | Use Instead |
|-----------|-------------|
| `"hello"` string literals | `"hello"_embed` |
| `L"hello"` wide strings | `L"hello"_embed` |
| `3.14` float literals | `3.14_embed` |
| `&MyFunc` function pointers | `EMBED_FUNC(MyFunc)` |
| Global/static variables | Stack-local variables |
| `const`/`constexpr` arrays | `MakeEmbedArray<T>(vals...)` |
| STL containers/algorithms | Custom PIR implementations |
| Exceptions (`throw`/`try`/`catch`) | `Result<T, Error>` |
| Raw `BOOL`/`NTSTATUS`/`SSIZE` for fallible returns | `Result<T, Error>` or `Result<void, Error>` |
| RTTI (`dynamic_cast`, `typeid`) | Static dispatch |
| `(T*, USIZE)` buffer parameter pairs | `Span<T>` / `Span<const T>` |

---

## Code Style

- **Indentation:** Tabs (not spaces)
- **Braces:** Allman style - opening brace on its own line
- **Include guard:** `#pragma once` in every header
- **No STL, no exceptions, no RTTI**

### Function Attributes
- Use `FORCE_INLINE` for functions that must always be inlined  
- Use `NOINLINE` when inlining must be prevented  

### Compile-Time Evaluation
- Use `constexpr` / `consteval` wherever possible  
- Mark every function and variable `constexpr` if it can be evaluated at compile time  
- Use `consteval` when evaluation **must** occur at compile time  

### System Calls
- Cast to `USIZE` when passing pointer/handle arguments to `System::Call`

### Includes
- `runtime.h` → includes everything  
- `platform.h` → includes **CORE + PLATFORM**  
- Implementation files must include their **own header first**

### Include Paths
- Always use full paths relative to `src/` (e.g., `"core/types/primitives.h"`, not `"primitives.h"`). This eliminates ambiguity and does not rely on CMake include-path ordering. 
- *Exception*: test files in `tests/` may use bare filenames for other test headers in the same directory.

---

## Documentation

All public APIs and protocol implementations **must** include Doxygen documentation.

### RFC References

- Every header starts with `@file`, `@brief`, `@details`, and relevant `@see` RFC links
- Functions include `@brief`, `@details` (algorithm steps), `@param`, `@return`, and `@see` with specific RFC section links
- RFC `@see` format: human-readable citation on the first line, full URL (`https://datatracker.ietf.org/doc/html/rfcXXXX#section-Y.Z`) indented on the next
- Inline references use `///` comments for enum values, struct members, and implementation comments

**Required for:** all cryptographic algorithms, network protocols, encoding/decoding schemes, wire-format structs and enums, and implementation comments referencing protocol steps.

**Optional for:** internal utility functions with no protocol specification, and test files.

### Windows NT API References

Platform-specific OS API wrappers must include `@see` links to the official Microsoft Learn documentation instead of RFC links.

- **WDK-documented functions** - link to the WDK DDI reference
- **Partially documented functions** - link to the closest available page (Win32 DevNotes or Nt prefix equivalent)
- **Undocumented functions** - add `@note This function is undocumented by Microsoft.` and link to the closest documented Win32 equivalent
- **Requirements** - every function must include a `@par Requirements` block listing minimum supported Windows client and server versions

**Required for:**  
- All NTDLL `Zw*/Nt*` and `Rtl*` wrappers  
- All Kernel32/Win32 wrappers  
- Windows-specific structs, enums, and constants

**Common URL patterns:**

| Source | URL Pattern |
|--------|-------------|
| WDK DDI (wdm.h) | `https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/wdm/nf-wdm-{function}` |
| WDK DDI (ntifs.h) | `https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ntifs/nf-ntifs-{function}` |
| WDK DDI (ntddk.h) | `https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ntddk/nf-ntddk-{function}` |
| Win32 API | `https://learn.microsoft.com/en-us/windows/win32/api/{header}/nf-{header}-{function}` |
| Win32 DevNotes | `https://learn.microsoft.com/en-us/windows/win32/devnotes/{function}` |
| Kernel concepts | `https://learn.microsoft.com/en-us/windows-hardware/drivers/kernel/{topic}` |

---

## Naming Conventions

| Kind | Convention | Examples |
|------|-----------|----------|
| Primitive typedefs | `UPPER_CASE` | `UINT32`, `INT64`, `WCHAR`, `PVOID`, `BOOL`, `DOUBLE` |
| Pointer typedefs | `P` prefix (`PP` double, `PC` const) | `PCHAR`, `PWCHAR`, `PPVOID`, `PCCHAR` |
| Classes | `PascalCase`; all-caps for acronym names | `Socket`, `HttpClient`, `TlsBuffer`, `NTDLL`, `ECC`, `DNS` |
| Structs (Windows-style) | `_NAME` with typedef | `typedef struct _OBJECT_ATTRIBUTES { ... } OBJECT_ATTRIBUTES;` |
| Template utility types | `UPPER_CASE` | `EMBEDDED_STRING<...>`, `EMBEDDED_ARRAY<...>`, `VOID_TO_TAG<T>` |
| Template classes | `PascalCase` | `Result<T, E>`, `Span<T>`, `SHABase<Traits>` |
| Enum names | `PascalCase` | `ErrorCodes`, `PlatformKind`, `IPVersion`, `CmpOp` |
| Enum values (scoped `enum class`) | `PascalCase` | `PlatformKind::Runtime`, `IPVersion::IPv4` |
| Enum values (unscoped) | `PascalCase_Underscore` | `Socket_CreateFailed_Open`, `Tls_OpenFailed_Socket` |
| Class methods | `PascalCase` | `StringUtils::Length()`, `NTDLL::ZwCreateFile()` |
| Member variables (private) | `camelCase` | `address`, `offset`, `maxSize`, `bits` |
| Member variables (in `Result`/`Span`) | `m_` prefix + `camelCase` | `m_data`, `m_size`, `m_value`, `m_isOk` |
| Struct fields (public) | `PascalCase` | `Error::Code`, `Error::Platform` |
| Local variables | `camelCase` | `allPassed`, `fileHandle`, `bufferSize` |
| Function parameters | `camelCase` | `ipAddress`, `maxSize`, `errnoVal` |
| `#define` macros | `UPPER_CASE` | `FORCE_INLINE`, `NOINLINE`, `ENTRYPOINT`, `EMBED_FUNC` |
| `constexpr` free constants | `UPPER_CASE` | `DYNAMIC_EXTENT` |
| `static constexpr` class members | `PascalCase` | `Count`, `SizeBytes`, `WordCount`, `Seed` |
| Concepts | `UPPER_CASE` | `TCHAR` |
| Template type parameters | `T` or `T` prefix + `PascalCase` | `T`, `E`, `TChar`, `TValue` |
| Template value parameters | `PascalCase` | `N`, `Extent`, `Bytes` |
| Header files | `snake_case.h` | `embedded_string.h`, `windows_types.h` |
| Source files | `snake_case.cc` | `kernel32.cc`, `entry_point.cc` |
| Platform-specific | `name.platform.cc` | `allocator.windows.cc`, `syscall.linux.h` |
| Platform+arch-specific | `name.platform.arch.cc` | `syscall.windows.x86_64.cc`, `entry_point.linux.aarch64.cc` |
| Test files | `snake_case_tests.h` | `djb2_tests.h`, `socket_tests.h` |

---

## Parameters & Returns

| Style | When | Example |
|-------|------|---------|
| By value | Small register-sized types | `UINT32 ComputeHash(UINT32 input)` |
| By pointer | Output params, nullable, Windows API compat | `Result<NTSTATUS, Error> ZwCreateFile(PPVOID FileHandle, ...)` |
| By reference | Non-null params (compile-time guarantee) | `Socket(const IPAddress &ipAddress, UINT16 port)` |
| `Span<T>` | Contiguous buffer params (replaces `T*, USIZE` pairs) | `void Process(Span<const UINT8> data)` |

### Key Rules

- **Prefer references over pointers** for non-null arguments. Reserve pointers for output parameters, nullable arguments, and Windows API compatibility.
- **Use `Span<T>` instead of raw pointer buffer parameters.** Exempt: core primitives that `Span` itself is built on (`Memory::Copy`, `Memory::Set`, `Memory::Compare`) and null-terminated string functions (`StringUtils::Length`, etc.).
- **Prefer `Span<T, N>` (static extent) when the size is a compile-time constant.** Use a template parameter `N` when the exact size varies per call site but is still known at compile time. Reserve `Span<T>` (dynamic extent) for genuinely runtime-only sizes.
- **`Span<T, N>` implicitly converts to `Span<T>`** (static-to-dynamic), so callers with static-extent spans can pass them to functions accepting dynamic extent.
- **All fallible functions must return `Result<T, Error>`** (or `Result<void, Error>` when there is no value). The `Result` class itself is `[[nodiscard]]`.
- **Never use `Result<bool, Error>`** - use `Result<void, Error>` instead. `Result` is already bool-testable via `operator BOOL`.
- Infallible functions (getters, pure computations, operators) return their value directly.

---

## Error Handling

PIR has no exceptions. Every fallible function returns `Result<T, Error>` or `Result<void, Error>`.

### The Error Struct

`Error` stores a top-level `(Code, Platform)` pair plus a fixed-capacity chain of inner (cause) errors, defined in `src/core/types/error.h`:

- **Runtime codes** (`PlatformKind::Runtime`): named `ErrorCodes` enumerators - `Socket_WriteFailed_Send`, `Tls_OpenFailed_Handshake`, etc.
- **OS codes**: created via factories - `Error::Windows(ntstatus)`, `Error::Posix(errno)`, `Error::Uefi(efiStatus)`
- **Chain**: `Code`/`Platform` is the outermost error; `InnerCodes[0..Depth-1]` holds cause errors from most-recent-inner to root-cause (max `MaxInnerDepth=4` inner entries, total `MaxDepth=5`)

### Construction Rules

`Error` preserves the full error propagation path. The two-arg `Err` overloads chain both errors using `Error::Wrap()`:

- **Single runtime error (no prior result):** pass a bare `ErrorCodes` enumerator to `Result::Err` - only when there is no underlying `Result` to forward
- **Single OS error:** use `Error::Windows()`, `Error::Posix()`, or `Error::Uefi()` factory methods
- **Two-arg shorthand (preferred):** `Result::Err(r, Error::Tls_WriteFailed_Send)` - always use this form when an underlying `Result` failed. Both the inner result's full chain and the outer site code are preserved.
- **Manual wrapping:** `Error::Wrap(innerError, outerCode, outerPlatform)` creates a new Error with `outerCode` on top and `innerError`'s chain beneath

**Important:** When a lower-level call returns a failed `Result` and you return `Err` with your own error code, **always** pass the failed result as the first argument:

```cpp
// WRONG - loses the underlying error chain
auto tlsResult = TlsClient::Create(host, ip, port, isSecure);
if (!tlsResult)
    return Result<void, Error>::Err(Error::Ws_CreateFailed);

// CORRECT - chains the underlying error under Ws_CreateFailed
auto tlsResult = TlsClient::Create(host, ip, port, isSecure);
if (!tlsResult)
    return Result<void, Error>::Err(tlsResult, Error::Ws_CreateFailed);
// Result: Code=Ws_CreateFailed, Inner[0]=Tls_*, Inner[1]=Socket_*, Inner[2]=NTSTATUS
```

### Platform Conversion Factories

Each platform provides `result::From*` template functions in `src/platform/<platform>/platform_result.h`.

These functions convert a raw OS status into `Ok`/`Err` in a single call. Examples:

  ```
  result::FromNTSTATUS<T>(status)
  result::FromLinux<T>(result)
  result::FromMacOS<T>(result)
  result::FromFreeBSD<T>(result)
  result::FromEfiStatus<T>(status)
  ```

### Formatting

Use `%e` with `result.Error()` in log macros.

**Output format:**

- **Runtime codes:** decimal  
- **Windows:** `0xNNNNNNNN[W]`  
- **Posix:** `N[P]`  
- **UEFI:** `0xNNNN...[U]`  

When an error has a chain, entries are separated by ` <- ` from outermost to root cause. 

Example:
`22 <- 15 <- 0xC0000034[W]`

### Exceptions to Result

- **Low-level primitives** (`System::Call`, `Memory::Copy`, etc.) return raw OS types or operate infallibly
- **Best-effort output** (`Console::Write`, logging callbacks) - failures are non-actionable
- **Infallible functions** (getters, pure computations, operators) return their value directly

### Rules Summary

- `[[nodiscard]]` may also be used on factory methods and non-Result functions where discarding the return value is always a bug
- OS errors: use factory methods
- Runtime errors: pass bare enumerators
- Each layer adds only its own `ErrorCodes` values
- Discard `[[nodiscard]]` Result with `(void)` only when intentional (destructors, move-assignment)

---

## Memory & Resources

### Heap & Stack

- **Avoid heap** unless no alternative. Prefer stack-local variables and fixed-size buffers.
- **`new`/`new[]`/`delete`/`delete[]` are safe** - globally overloaded to route through the custom `Allocator` (see `src/platform/allocator.cc`).
- **Embed by value**, not by pointer: `IPAddress ipAddress;` not `IPAddress *ipAddress;`
- **Watch stack size**: `EMBEDDED_STRING` temporaries materialize words on stack; avoid deep recursion.

### Constructor Rules

- Constructors must be **trivial and never fail**. All fallible work goes into a `[[nodiscard]] static` factory method returning `Result<T, Error>`.
- The factory creates the object, then performs fallible initialization (connecting, opening, etc.) via separate methods if needed.

### RAII Pattern

Every resource-owning class follows these rules:

- Destructor calls `Close()`  
- Copy constructor and copy assignment are **deleted**  
- Move constructor/assignment **nullifies the source**  
- `Close()` sets the handle to `nullptr`  

**Note:** Use `static_cast<T &&>()` instead of `std::move()`.

### Stack-Only Types

Delete heap allocation operators (`operator new`/`operator delete = delete`).

---

## Patterns

### Compile-Time Embedding

| Type | Literal | Result |
|------|---------|--------|
| `EMBEDDED_STRING` | `"text"_embed` / `L"text"_embed` | Characters packed into machine words |
| `DOUBLE` | `3.14_embed` | IEEE-754 bits as `UINT64` immediate |
| `EMBEDDED_ARRAY` | `MakeEmbedArray<T>(vals...)` | Elements packed into machine words |
| `EMBEDDED_FUNCTION_POINTER` | `EMBED_FUNC(Fn)` | PC-relative offset, no relocation |

**Variadic `MakeEmbedArray<T>(vals...)`** - Pass values directly as arguments instead of through a named `constexpr` array. Named `constexpr` arrays leak to `.rdata` at `-O0` on some compilers (e.g., Clang cross-compiling from Linux). The variadic overload constructs the array inside a `consteval` function where it cannot leak. For compound literal callers, `MakeEmbedArray((const T[]){...})` also works.

A **register barrier** (`__asm__ volatile("" : "+r"(word))`) prevents the compiler from coalescing values back into `.rdata`.

**LOG macros auto-embed** - `LOG_INFO`, `LOG_ERROR`, `LOG_WARNING`, and `LOG_DEBUG` automatically apply `_embed` to their format string.
Write `LOG_INFO("msg")` not `LOG_INFO("msg"_embed)`.

### Traits-Based Dispatch

Parameterize algorithm variants via traits structs instead of runtime branching. The traits struct defines associated types and constants; a single template class implements the algorithm.

Examples: `SHA256Traits`/`SHA384Traits`, `UINT_OF_SIZE`, `VOID_TO_TAG`.

### Variadic Template Type Erasure

Variadic templates at the API surface are type-erased into a fixed `Argument` array before calling a `NOINLINE` implementation. This prevents code bloat from template instantiation.

Examples: `Logger::Info/Error/Warning/Debug`, `Console::WriteFormatted`, `StringFormatter::Format`.

### Concepts and Constraints

C++20 concepts and `requires` clauses enforce type safety. Use Clang builtins, not STL `<type_traits>`:

| Builtin | Purpose |
|---------|---------|
| `__is_same_as(T, U)` | Type equality |
| `__is_trivially_destructible(T)` | Skip destructor when trivial |
| `__builtin_bit_cast(T, v)` | Bit-level reinterpretation (constexpr-safe) |
| `__builtin_bswap16/32/64(v)` | Byte order swapping |

### Guard and Validation

- **Factory-created types**: the factory + RAII pattern ensures validity.
- **Non-factory types** parsing external input: validate at entry, return `Result::Err` on failure.
- Only validate at system boundaries - trust internal code.

### Platform Dispatch

Two strategies:

  * **conditional compilation** (`#if defined(PLATFORM_*)`) for small differences within one function

  * **separate implementation files** (`src/platform/{windows,linux,macos}/`) when implementations diverge entirely. CMake selects the correct files.

---

## Windows API Wrappers

### Kernel32 / Win32

- Declare in `src/platform/windows/kernel32.h`, implement in `src/platform/windows/kernel32.cc`
- Resolve function addresses dynamically via `ResolveKernel32ExportAddress` with the function name
- Cast the resolved address to the correct function pointer type and call it
- Return `Result<T, Error>` with an appropriate `ErrorCodes` enumerator on failure

### NTDLL / Zw* Syscalls

- Indirect syscalls on x86_64/i386 (via `System::Call` with resolved SSN), direct ntdll calls on ARM64 (via `CALL_FUNCTION`)
- Use `result::FromNTSTATUS<T>(status)` to convert raw `NTSTATUS` into `Result`
- All arguments to `System::Call` must be cast to `USIZE`
- Include `"platform_result.h"` for the conversion factories

---

## Writing Tests

- Each test suite is a class in `tests/<name>_tests.h`
- The class has a `static BOOL RunAll()` method that calls `RunTest(allPassed, EMBED_FUNC(TestName), L"description"_embed)` for each test
- Individual test methods are `static BOOL` functions returning `true` for pass, `false` for fail
- Register by adding `#include "my_feature_tests.h"` and `RunTestSuite<MyFeatureTests>(allPassed);` in `tests/pir_tests.h` under the appropriate layer comment (CORE, PLATFORM, or RUNTIME)
- See existing test files for reference

---

## Common Pitfalls

1. **Inline asm register clobbers** - On x86_64, declare all volatile registers (RAX, RCX, RDX, R8-R11) as outputs or clobbers
2. **Memory operands with RSP modification** - Never use `"m"` constraints in asm blocks that modify RSP; under `-Oz` the compiler uses RSP-relative addressing
3. **i386 `EMBEDDED_STRING` indexing** - Cast indices to `USIZE` to avoid ambiguous overload between `operator[]` and pointer decay

---

## Submitting Changes

### Before You Submit

1. Build cleanly for at least one platform/architecture preset
2. Verify the post-build PIC check passes (no data sections)
3. Run the test binary - all tests pass
4. Follow naming conventions and code style above

### Pull Request Requirements

- Use the [pull request template](pull_request_template.md)
- **Report the binary size diff** - build the same preset before and after your change, then include the `.text` section size (exe and bin) in the PR description. Size regressions require justification; prefer `-Oz` builds for the comparison:

   ```bash
   llvm-size build/release/<platform>/<arch>/output.exe
   ```

   Example PR note: `windows-x86_64-release: exe 42 312 -> 42 480 (+168 B), bin 39 888 -> 40 056 (+168 B)`

### Community

- Please read our [Code of Conduct](CODE_OF_CONDUCT.md) before participating
- Report security vulnerabilities privately - see [Security Policy](SECURITY.md)
