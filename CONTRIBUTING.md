# Contributing to Position-Independent Runtime

## Quick Start

**Requirements:** Clang/LLVM 21+, CMake 3.20+, Ninja 1.10+, C++23. See [Toolchain Installation](#toolchain-installation) below.

```bash
# Build
cmake --preset {platform}-{arch}-{build_type}
cmake --build --preset {platform}-{arch}-{build_type}

# Test (exit code 0 = all pass)
./build/{build_type}/{platform}/{arch}/output.{exe|elf|efi}
```

Presets: `windows|linux|macos|uefi` x `i386|x86_64|armv7a|aarch64` x `debug|release`

## Toolchain Installation

### Windows

**Prerequisites:** [winget](https://github.com/microsoft/winget-cli) (pre-installed on Windows 11 and recent Windows 10).

```powershell
# Install all dependencies
winget install --id Kitware.CMake && winget install --id Ninja-build.Ninja && winget install --id LLVM.LLVM

# Optional: Install Doxygen for documentation generation
winget install -e --id DimitriVanHeesch.Doxygen
```

**Important:** After installation, restart your terminal or log out and log back in for the PATH changes to take effect.

Verify:

```powershell
clang --version && clang++ --version && lld-link --version && cmake --version && ninja --version
```

### Linux (Ubuntu/Debian)

```bash
# Install all dependencies (LLVM 21)
LLVM_VER=21 && sudo apt-get update && sudo apt-get install -y wget lsb-release ca-certificates gnupg cmake ninja-build && sudo mkdir -p /etc/apt/keyrings && wget -qO- https://apt.llvm.org/llvm-snapshot.gpg.key | gpg --dearmor | sudo tee /etc/apt/keyrings/apt.llvm.org.gpg >/dev/null && echo "deb [signed-by=/etc/apt/keyrings/apt.llvm.org.gpg] http://apt.llvm.org/$(lsb_release -cs)/ llvm-toolchain-$(lsb_release -cs)-${LLVM_VER} main" | sudo tee /etc/apt/sources.list.d/llvm.list && sudo apt-get update && sudo apt-get install -y clang-${LLVM_VER} clang++-${LLVM_VER} lld-${LLVM_VER} llvm-${LLVM_VER} lldb-${LLVM_VER} && sudo update-alternatives --install /usr/bin/clang clang /usr/bin/clang-${LLVM_VER} 100 && sudo update-alternatives --install /usr/bin/clang++ clang++ /usr/bin/clang++-${LLVM_VER} 100 && sudo update-alternatives --install /usr/bin/lld lld /usr/bin/lld-${LLVM_VER} 100 && sudo update-alternatives --install /usr/bin/llvm-objdump llvm-objdump /usr/bin/llvm-objdump-${LLVM_VER} 100 && sudo update-alternatives --install /usr/bin/llvm-objcopy llvm-objcopy /usr/bin/llvm-objcopy-${LLVM_VER} 100 && sudo update-alternatives --install /usr/bin/llvm-strings llvm-strings /usr/bin/llvm-strings-${LLVM_VER} 100 && sudo update-alternatives --install /usr/bin/lldb-dap lldb-dap /usr/bin/lldb-dap-${LLVM_VER} 100 && sudo update-alternatives --set clang "/usr/bin/clang-${LLVM_VER}" && sudo update-alternatives --set clang++ "/usr/bin/clang++-${LLVM_VER}" && sudo update-alternatives --set lld "/usr/bin/lld-${LLVM_VER}" && sudo update-alternatives --set llvm-objdump "/usr/bin/llvm-objdump-${LLVM_VER}" && sudo update-alternatives --set llvm-objcopy "/usr/bin/llvm-objcopy-${LLVM_VER}" && sudo update-alternatives --set llvm-strings "/usr/bin/llvm-strings-${LLVM_VER}" && sudo update-alternatives --set lldb-dap "/usr/bin/lldb-dap-${LLVM_VER}"
```

**Note:** To install a different LLVM version, change `LLVM_VER=21` to your desired version (e.g., `LLVM_VER=22`).

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

### IDE Configuration

#### Visual Studio Code

This project is designed and optimized for [Visual Studio Code](https://code.visualstudio.com/). When you open this project in VSCode, you will be automatically prompted to install the recommended extensions.

#### WSL Integration

If you're developing on Windows with WSL, you can open this project in WSL using VSCode's remote development features.

**From within WSL**, navigate to the project directory and run:

```bash
code .
```

**Prerequisites:**
- Ensure WSL is properly configured on your Windows system
- Install QEMU, UEFI firmware, and disk image tools:
  ```bash
  sudo apt-get update && sudo apt-get install -y qemu-user-static qemu-system-x86 qemu-system-arm ovmf qemu-efi-aarch64 dosfstools mtools
  ```

For more information, see the [VSCode WSL documentation](https://code.visualstudio.com/docs/remote/wsl) and [.vscode/README.md](.vscode/README.md).

## Project Structure

```
src/                        # Source layers
  core/                     # CORE layer (.h + .cc co-located)
    core.h                  # Aggregate header for CORE layer
    compiler/               # Compiler abstractions (FORCE_INLINE, NOINLINE, compiler runtime)
    memory/                 # Memory operations (Copy, Set, Compare, Zero)
    math/                   # Math utilities, bit operations, byte order
    io/                     # Binary reader/writer
    types/                  # Primitives, Span, Result, Error, Double, IP address
      embedded/             # EMBEDDED_STRING, EMBEDDED_DOUBLE, EMBEDDED_ARRAY, EMBEDDED_FUNCTION_POINTER
    string/                 # String utilities and formatting
    algorithms/             # DJB2 hashing, Base64
    encoding/               # UTF-16
  platform/                 # PLATFORM layer
    platform.h              # Aggregate header for PLATFORM layer
    common/                 # Per-OS shared infrastructure
      windows/              # PEB, PE, NTDLL, System, Kernel32, types
      linux/                # Syscall numbers, System::Call, result conversion
      macos/                # Syscall numbers, System::Call, result conversion
      solaris/              # Syscall numbers, System::Call, result conversion
      uefi/                 # EFI types, protocols, boot/runtime services
    memory/                 # Allocator (heap management)
      windows/              # NtAllocateVirtualMemory/NtFreeVirtualMemory
      posix/                # mmap/munmap
      uefi/                 # AllocatePool/FreePool
    io/                     # Console, Logger
      windows/              # ZwWriteFile console output
      posix/                # write() syscall to STDOUT
      uefi/                 # SimpleTextOut protocol
    fs/                     # FileSystem, Path, Directory, DirectoryIterator
      windows/              # NtCreateFile, NtReadFile, NtWriteFile
      posix/                # open/read/write/close syscalls, posix_path
      uefi/                 # EFI_FILE_PROTOCOL, uefi_fs_helpers
    network/                # Socket
      windows/              # AFD (Auxiliary Function Driver)
      linux/                # Direct socket syscalls
      macos/                # Direct socket syscalls
      solaris/              # Direct socket syscalls
      uefi/                 # EFI_TCP4/TCP6_PROTOCOL
    system/                 # DateTime, Environment, Process, Random
      windows/              # Windows-specific system operations
      posix/                # Shared POSIX date_time, process
      linux/                # Linux-specific environment, platform, process
      macos/                # macOS-specific environment, platform, process
      solaris/              # Solaris-specific environment, platform, process
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
| `(T*, USIZE)` buffer parameter pairs | `Span<T>` / `Span<const T>` |

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
- **No STL, no exceptions, no RTTI**
- **`FORCE_INLINE`** for force-inlined functions, **`NOINLINE`** when inlining must be prevented
- **`constexpr`/`consteval` everywhere possible:** mark every function and variable `constexpr` if it *can* be evaluated at compile time; use `consteval` when it *must* be compile-time only
- **Cast to `USIZE`** when passing pointer/handle arguments to `System::Call`
- **Includes:** `runtime.h` = everything; `platform.h` = CORE + PLATFORM; implementation files include own header first
- **Include paths:** Always use full paths relative to `src/` — never bare filenames. This eliminates ambiguity, makes dependencies self-documenting, and does not rely on CMake include-path ordering:
  ```cpp
  // Good — unambiguous, self-documenting:
  #include "core/types/primitives.h"
  #include "core/string/string.h"
  #include "platform/io/console.h"

  // Bad — relies on search-path ordering, ambiguous origin:
  #include "primitives.h"
  #include "string.h"
  #include "console.h"
  ```
  Exception: test files in `tests/` may use bare filenames for other test headers in the same directory (e.g., `#include "tests.h"`).

### Documentation & RFC References

All public APIs and protocol implementations **must** include Doxygen documentation with RFC references where applicable.

**File-level documentation** — every header starts with `@file`, `@brief`, `@details`, and relevant `@see` RFC links:

```cpp
/**
 * @file base64.h
 * @brief Base64 Encoding and Decoding
 *
 * @details Platform-independent Base64 encoding and decoding implementing
 * the standard alphabet defined in RFC 4648 Section 4.
 *
 * @see RFC 4648 — The Base16, Base32, and Base64 Data Encodings
 *      https://datatracker.ietf.org/doc/html/rfc4648
 * @see RFC 4648 Section 4 — Base 64 Encoding (alphabet and padding definition)
 *      https://datatracker.ietf.org/doc/html/rfc4648#section-4
 */
```

**Function/method documentation** — `@brief`, `@details` explaining the algorithm step-by-step, `@param`, `@return`, and `@see` with specific RFC section links:

```cpp
/**
 * @brief Encodes binary data to Base64 format
 *
 * @details Implements the encoding procedure from RFC 4648 Section 4:
 * 1. Input bytes are consumed in 3-byte (24-bit) groups
 * 2. Each group is split into four 6-bit values ...
 *
 * @param input Input binary data
 * @param output Output buffer
 *
 * @see RFC 4648 Section 4 — Base 64 Encoding
 *      https://datatracker.ietf.org/doc/html/rfc4648#section-4
 */
```

**RFC reference format** — always use the `@see` tag with both the human-readable citation and the full URL on the next line:

```
@see RFC XXXX Section Y.Z — Short description
     https://datatracker.ietf.org/doc/html/rfcXXXX#section-Y.Z
```

**Inline RFC references** — use `///` comments for enum values, struct members, and implementation comments:

```cpp
Text = 0x1, ///< Text data frame — payload is UTF-8 (RFC 6455 Section 5.6)
```

**When RFC documentation is required:**
- All cryptographic algorithms (SHA, HMAC, ChaCha20, ECC, etc.)
- All network protocols (TLS, HTTP, WebSocket, DNS, etc.)
- All encoding/decoding schemes (Base64, ASN.1/DER, etc.)
- All wire-format structs and enums
- Implementation comments referencing specific protocol steps or state machines

**When RFC documentation is optional:**
- Internal utility functions with no protocol specification
- Test files

### Windows NT API Documentation References

Platform-specific OS API wrappers (NTDLL, Kernel32) **must** include Doxygen documentation with `@see` links to the official Microsoft Learn documentation instead of RFC links. Use the same `@see` tag pattern as RFC references: human-readable name on the first line, full URL indented on the next.

- **WDK-documented functions** — most Zw* syscall wrappers are in the WDK DDI reference
- **Partially documented functions** — some are only in Win32 DevNotes or under the Nt prefix; link to the closest available page
- **Undocumented functions** (e.g., ZwCreateUserProcess, ZwSetInformationObject) — add `@note This function is undocumented by Microsoft.` and link to the closest documented Win32 equivalent
- **Requirements** — every function must include a `@par Requirements` block listing the minimum supported Windows client and server versions (e.g., `Minimum supported client: Windows XP`, `Minimum supported server: Windows Server 2003`). Place it after `@return` and before `@see`/`@note`

**When Microsoft Learn documentation is required:**
- All NTDLL Zw*/Nt* syscall wrappers
- All NTDLL Rtl* runtime library wrappers
- All Kernel32/Win32 API wrappers
- Windows-specific structs, enums, and constants that correspond to documented types

**Common URL patterns:**

| Source | URL Pattern |
|--------|-------------|
| WDK DDI (wdm.h) | `https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/wdm/nf-wdm-{function}` |
| WDK DDI (ntifs.h) | `https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ntifs/nf-ntifs-{function}` |
| WDK DDI (ntddk.h) | `https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ntddk/nf-ntddk-{function}` |
| Win32 API | `https://learn.microsoft.com/en-us/windows/win32/api/{header}/nf-{header}-{function}` |
| Win32 DevNotes | `https://learn.microsoft.com/en-us/windows/win32/devnotes/{function}` |
| Kernel concepts | `https://learn.microsoft.com/en-us/windows-hardware/drivers/kernel/{topic}` |

### Naming

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

### Parameters & Returns

| Style | When | Example |
|-------|------|---------|
| By value | Small register-sized types | `UINT32 ComputeHash(UINT32 input)` |
| By pointer | Output params, nullable, Windows API compat | `Result<NTSTATUS, Error> ZwCreateFile(PPVOID FileHandle, ...)` |
| By reference | Non-null params (compile-time guarantee) | `Socket(const IPAddress &ipAddress, UINT16 port)` |
| `Span<T>` | Contiguous buffer params (replaces `T*, USIZE` pairs) | `void Process(Span<const UINT8> data)` |

**Prefer references over pointers** — Use reference parameters (`T&` / `const T&`) by default for non-null arguments. References provide a compile-time non-null guarantee, which means the callee never needs a null check — any null guard inside the function is dead code and a sign the parameter should be a reference. Reserve pointers only for output parameters, nullable arguments, and Windows API compatibility.

**`Span<T>`** — **Use `Span<T>` instead of raw pointer buffer parameters** for functions that operate on contiguous buffers. This applies to both `(T*, USIZE)` pairs and bare `T*` pointers without any size — bare pointers are worse since no bounds checking is possible at all. This eliminates size-mismatch bugs at zero runtime cost:

```cpp
// Bad — caller can pass wrong size:
void Write(const UINT8 *data, USIZE size);

// Good — size is bundled with pointer:
void Write(Span<const UINT8> data);

UINT8 buffer[64];
Write(Span(buffer));              // size deduced from array
Write(Span(ptr, len));            // explicit pointer + size
Write(writable.Subspan(4, 16));   // slicing
```

Use `Span<T>` for writable buffers and `Span<const T>` for read-only views. `Span<T>` implicitly converts to `Span<const T>`. The following are exempt from this rule:
- Core primitives that Span itself is built on (e.g., `Memory::Copy`, `Memory::Set`, `Memory::Compare`) — they take raw `(PVOID, PCVOID, USIZE)` by design.
- Null-terminated string functions (e.g., `StringUtils::Length`, `StringUtils::Compare`, `StringUtils::Equals`) — they compute length from the null terminator, so they are string parameters with implicit length, not buffer parameters. These typically have `Span` overloads as well for callers that already know the length.

**Do not cache `Span::Size()` into a read-only local variable** — `Size()` is `constexpr FORCE_INLINE` and compiles to a direct member access, so there is no cost to calling it repeatedly. Caching it into a local just for loop bounds or repeated comparisons adds a redundant variable with no benefit. Mutable counters initialized from `Size()` and derived computations from multiple `Size()` calls are fine:

```cpp
// Bad — unnecessary read-only local for loop bound:
INT32 maxLen = (INT32)data.Size();
while (offset < maxLen) { ... }

// Good — call Size() directly:
while (offset < (INT32)data.Size()) { ... }

// Good — mutable counter that gets modified:
USIZE len = str.Size();
while (len > 0 && IsSpace(str[len - 1])) { len--; }

// Good — derived computation from multiple Size() calls:
USIZE offset = str.Size() - suffix.Size();
```

**`[[nodiscard]] Result<T, Error>`** — **All fallible functions must return `Result<T, Error>`** (or `Result<void, Error>` when there is no value). Do not use raw `BOOL`, `NTSTATUS`, or `SSIZE` as return types for success/failure. The `Result` class itself is declared `class [[nodiscard]] Result`, so all Result-returning functions automatically warn on discard. Adding `[[nodiscard]]` on the function declaration as well is encouraged for explicitness but not strictly required. This ensures a uniform error-handling interface across the codebase:

```cpp
[[nodiscard]] Result<IPAddress, Error> Resolve(PCCHAR host);

auto result = Resolve(hostName);
if (result.IsErr())
    return;
IPAddress &ip = result.Value();  // borrow; Result still owns it

// Use Result<void, Error> when there is no value to return
[[nodiscard]] Result<void, Error> Open();
```

**Do not copy `Result::Value()`** into local variables — `Value()` returns a reference to the value owned by `Result`, so copying it creates an unnecessary duplicate. Pass `Value()` directly to functions, or bind to `auto&` when you need to call multiple methods on it:

```cpp
// Good — pass Value() directly to functions:
auto createResult = Socket::Create(result.Value(), 443);

// Good — single method call chaining is fine:
result.Value().Method();

// Good — local reference when calling multiple methods (avoids repeating .Value()):
auto& x = result.Value();
x.Method1();
x.Method2();

// Bad — unnecessary copy just to pass as argument:
IPAddress ip = result.Value();
auto createResult = Socket::Create(ip, 443);
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

**Exceptions — these do NOT use `Result`:**
- **Low-level primitives** (`System::Call`, `Memory::Copy`, etc.) — these return raw OS types (`NTSTATUS`, `SSIZE`) or operate infallibly. Higher-level wrappers (e.g., `NTDLL::Zw*`, `result::FromNTSTATUS`) are responsible for converting raw returns into `Result`.
- **Best-effort output** (`Console::Write`, logging callbacks) — failures are non-actionable; forcing `Result` through the entire logging chain would add code bloat for zero benefit.
- **Infallible functions** (getters, pure computations, operators) — return their value directly.

### The Error Struct

`Error` is a `(Code, Platform)` pair (8 bytes) defined in `src/core/error.h`:

- **Runtime codes** (`PlatformKind::Runtime`): named `ErrorCodes` enumerators — `Socket_WriteFailed_Send`, `Tls_OpenFailed_Handshake`, etc.
- **OS codes**: created via factories — `Error::Windows(ntstatus)`, `Error::Posix(errno)`, `Error::Uefi(efiStatus)`

### Construction Patterns

`Error` stores a single `(Code, Platform)` slot — there is no call-chain. Each layer picks the most useful code to surface.

```cpp
// Single runtime error:
return Result<UINT32, Error>::Err(Error::Socket_WriteFailed_Send);

// Single OS error (low-level wrappers — OS code is the most useful context):
return Result<UINT32, Error>::Err(Error::Posix((UINT32)(-sent)));

// Propagate lower-level error unchanged:
auto r = context.Write(buffer, size);
if (!r)
    return Result<UINT32, Error>::Err(r.Error());

// Replace with own layer's error code (when the caller's context is more useful):
auto r = context.Write(buffer, size);
if (!r)
    return Result<UINT32, Error>::Err(Error::Tls_WriteFailed_Send);

// Replace error via 2-arg shorthand (stores only the new code, discards the original):
auto r = context.Write(buffer, size);
if (!r)
    return Result<UINT32, Error>::Err(r, Error::Tls_WriteFailed_Send);
```

### Platform Conversion Factories

Each platform provides `result::From*` template functions in `src/platform/<platform>/platform_result.h`. These convert a raw OS status into `Ok`/`Err` in one call. Use them in low-level wrappers that only need the OS error code:

```cpp
// Windows (include "platform_result.h"):
return result::FromNTSTATUS<NTSTATUS>(status);    // Ok when status >= 0
return result::FromNTSTATUS<void>(status);         // void: discards value on success

// Linux (include "platform_result.h"):
return result::FromLinux<UINT32>(result);           // Ok when result >= 0
return result::FromLinux<void>(result);             // void: discards value on success

// macOS (include "platform_result.h"):
return result::FromMacOS<UINT32>(result);           // Ok when result >= 0

// UEFI (include "platform_result.h"):
return result::FromEfiStatus<void>(status);         // Ok when (SSIZE)status >= 0
```

For void Results, the raw value is discarded on success. For non-void Results, the raw value is stored as the Ok value.

### Formatting

Use `%e` with `result.Error()`:

```cpp
LOG_ERROR("Operation failed (error: %e)", result.Error());
// Runtime: "33"    Windows: "0xC0000034[W]"    Posix: "111[P]"    UEFI: "0x8000...[U]"
```

### Error Rules

- `[[nodiscard]]` — see [Parameters & Returns](#parameters--returns) for the full policy. Additionally, `[[nodiscard]]` **may** be used on factory methods and non-Result functions where discarding the return value is always a bug.
- **Never use `Result<bool, Error>`** — use `Result<void, Error>` instead. `Result` itself is already bool-testable via `operator BOOL`, so wrapping a `bool` value creates confusing double-boolean checks (`!r || !r.Value()`). With `Result<void, Error>`, truthy means success and falsy means failure — clean and unambiguous.
- OS errors: use factory methods — `Error::Windows()`, `Error::Posix()`, `Error::Uefi()`
- Runtime errors: pass bare — `Result::Err(Error::Socket_WriteFailed_Send)`
- Each layer adds only its own `ErrorCodes` values
- Discard `[[nodiscard]]` Result with `(void)` only when intentional (destructors, move-assignment)

---

## Memory & Resources

### Heap & Stack

- **Avoid heap** unless no alternative. Prefer stack-local variables and fixed-size buffers
- **`new`/`new[]`/`delete`/`delete[]` are safe** — they are globally overloaded to route through the custom `Allocator` (see `src/platform/allocator.cc`), so all heap allocations use the PIR memory backend
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
    return Result<void, Error>::Err(createResult.Error());
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
        if (handle != nullptr) { (void)NTDLL::ZwClose(handle); handle = nullptr; }
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
TlsBuffer(Span<CHAR> data) : buffer(data.Data()), ownsMemory(false) {}
~TlsBuffer() { if (ownsMemory) Clear(); }
```

---

## Patterns

### Compile-Time Embedding

The `_embed` ecosystem converts literals into immediate values, eliminating `.rdata`:

| Type | Literal | Result |
|------|---------|--------|
| `EMBEDDED_STRING` | `"text"_embed` / `L"text"_embed` | Characters packed into machine words |
| `DOUBLE` | `3.14_embed` | IEEE-754 bits as `UINT64` immediate |
| `EMBEDDED_ARRAY` | `MakeEmbedArray(arr)` | Elements packed into machine words |
| `EMBEDDED_FUNCTION_POINTER` | `EMBED_FUNC(Fn)` | PC-relative offset, no relocation |

A **register barrier** (`__asm__ volatile("" : "+r"(word))`) prevents the compiler from coalescing values back into `.rdata`.

**LOG macros auto-embed** — `LOG_INFO`, `LOG_ERROR`, `LOG_WARNING`, and `LOG_DEBUG` automatically apply `_embed` to their format string. Write `LOG_INFO("msg")` not `LOG_INFO("msg"_embed)`.

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

- **Factory-created types** (`Socket`, `TlsClient`): do **not** add `IsValid()` guards — the factory + RAII pattern ensures validity
- **Non-factory types** parsing external input: validate at entry, return `Result::Err` on failure
- Only validate at system boundaries — trust internal code

### Platform Dispatch

Two strategies: **conditional compilation** (`#if defined(PLATFORM_*)`) for small differences within one function, and **separate implementation files** (`src/platform/{windows,linux,macos}/`) when implementations diverge entirely. CMake selects the correct files.

---

## Windows API Wrappers

### Kernel32 / Win32

```cpp
// Header: src/platform/windows/kernel32.h
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

Indirect syscalls on x86_64/i386, direct ntdll calls on ARM64. Use `result::FromNTSTATUS` to convert the raw status:

```cpp
#include "platform_result.h"

[[nodiscard]] Result<NTSTATUS, Error> NTDLL::ZwMyFunction(PVOID Param1, UINT32 Param2)
{
    SYSCALL_ENTRY entry = ResolveSyscall("ZwMyFunction");
    NTSTATUS status = entry.ssn != SYSCALL_SSN_INVALID
        ? System::Call(entry, (USIZE)Param1, (USIZE)Param2)
        : CALL_FUNCTION("ZwMyFunction", PVOID Param1, UINT32 Param2);
    return result::FromNTSTATUS<NTSTATUS>(status);
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
5. **Report the binary size diff** — build the same preset before and after your change, then include the `.text` section size (exe and bin) in the PR description. Size regressions require justification; prefer `-Oz` builds for the comparison. Use `llvm-size` or `llvm-objdump -h` to measure:

   ```bash
   llvm-size build/release/<platform>/<arch>/output.exe
   ```

   Example PR note: `windows-x86_64-release: exe 42 312 → 42 480 (+168 B), bin 39 888 → 40 056 (+168 B)`
