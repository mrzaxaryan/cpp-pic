# CPP-PIC: Position-Independent C++23 Runtime

**A Revolutionary Approach to Zero-Dependency C++ Code Generation**

[![License](https://img.shields.io/badge/license-Proprietary-blue.svg)](LICENSE)
[![C++23](https://img.shields.io/badge/C%2B%2B-23-blue.svg)](https://en.cppreference.com/w/cpp/23)
[![Platform](https://img.shields.io/badge/platform-Windows%20%7C%20Linux%20%7C%20UEFI-lightgrey.svg)](README.md)
[![Architecture](https://img.shields.io/badge/arch-i386%20%7C%20x86__64%20%7C%20armv7a%20%7C%20aarch64-orange.svg)](README.md)

---

## Table of Contents

- [Introduction](#introduction)
- [The Problem We Solve](#the-problem-we-solve)
- [How It Works](#how-it-works)
- [Features](#features)
- [Use Cases](#use-cases)
- [Toolchain Dependencies](#toolchain-dependencies)
- [Getting Started](#getting-started)
- [Technical Deep Dive](#technical-deep-dive)
- [Performance & Optimization](#performance--optimization)
- [Building](#building)
- [UEFI Platform Support](#uefi-platform-support)
- [Examples](#examples)
- [Contributing](#contributing)
- [License](#license)

---

## Introduction

CPP-PIC is a groundbreaking C++23 runtime library that challenges fundamental assumptions about how C++ programs interact with memory. By eliminating all `.rdata` section dependencies and embedding constants directly in executable code, we've created a truly position-independent runtime suitable for the most demanding environments.

This project represents years of research into compiler behavior, binary structure, and low-level systems programming. Whether you're building embedded systems, conducting security research, developing kernel components, or exploring the boundaries of what's possible with modern C++, CPP-PIC provides unprecedented control over your binary's memory layout.

## The Problem We Solve

Traditional C++ programs rely heavily on the `.rdata` (read-only data) section to store:
- String literals
- Floating-point constants
- Virtual function tables
- RTTI information
- Compiler-generated constants

This creates several challenges:

1. **Position Dependencies**: Code that references `.rdata` requires relocations, making true position-independence nearly impossible
2. **Memory Constraints**: Embedded systems and kernels often can't allocate separate data sections
3. **Security Concerns**: String tables are easily extracted from binaries, revealing sensitive information
4. **Deployment Complexity**: Injecting code into running processes requires handling relocations
5. **Bootloader Limitations**: Pre-MMU environments can't handle segmented memory layouts

### Traditional Approach vs. CPP-PIC

**Traditional C++ Compilation:**
```cpp
const char* msg = "Hello, World!";  // Stored in .rdata
double pi = 3.14159;                // Stored in .rdata
```

Assembly output:
```asm
movq .L.str, %rdi        ; Load pointer from .rdata
movsd .LCPI0_0, %xmm0    ; Load constant from .rdata
```

**CPP-PIC Approach:**
```cpp
auto msg = "Hello, World!"_embed;   // Embedded in code
auto pi = 3.14159_embed;            // Embedded in code
```

Assembly output:
```asm
movw $0x48, (%rdi)           ; Write 'H' directly
movw $0x65, 2(%rdi)          ; Write 'e' directly
movabsq $0x400921f9f01b866e, %rax  ; Pi as immediate value
```

## How It Works

CPP-PIC leverages cutting-edge C++23 features to achieve complete position independence through three key innovations:

### 1. **Compile-Time String Decomposition**

Using user-defined literal operators and variadic templates, strings are decomposed into individual characters at compile-time, then reconstructed at runtime on the stack:

```cpp
template <typename TChar, TChar... Chars>
class EMBEDDED_STRING {
    TChar data[sizeof...(Chars) + 1];

    NOINLINE DISABLE_OPTIMIZATION EMBEDDED_STRING() {
        USIZE i = 0;
        ((data[i++] = Chars), ...);  // Fold expression
        data[i] = 0;
    }
};
```

### 2. **IEEE-754 Bit Pattern Embedding**

Floating-point values are converted to their IEEE-754 bit representation at compile-time and embedded as 64-bit integer immediates:

```cpp
struct EMBEDDED_DOUBLE {
    consteval explicit EMBEDDED_DOUBLE(double v) {
        bits = __builtin_bit_cast(unsigned long long, v);
    }

    operator double() const {
        return __builtin_bit_cast(double, bits);
    }
};
```

### 3. **Pure Integer-Based Type Conversions**

All type conversions (double-to-int, int-to-double) are implemented using bitwise operations, eliminating compiler-generated conversion constants:

```cpp
// Extracts integer value from IEEE-754 without FPU instructions
INT64 operator(INT64)(const DOUBLE& d) {
    UINT64 bits = d.bits;
    int exponent = ((bits >> 52) & 0x7FF) - 1023;
    UINT64 mantissa = (bits & 0xFFFFFFFFFFFFF) | 0x10000000000000;
    // ... bit shifting magic ...
}

## Features

### Core Capabilities

- **Zero .rdata Dependencies**: All string literals and floating-point constants embedded as immediate values in code
- **Position-Independent**: No relocations required, suitable for shellcode and injection payloads
- **No CRT Required**: Complete standalone runtime with no dependencies on standard libraries
- **Modern C++23**: Leverages compile-time features including concepts, consteval, and fold expressions
- **Full Optimization Support**: Works with all LLVM optimization levels (-O0 through -Oz)

### Embedded Types

#### EMBEDDED_STRING
- Runtime stack construction of string literals
- Characters materialized one-by-one from immediate values
- Supports both narrow (char) and wide (wchar_t) strings
- Usage: `auto str = "Hello"_embed;`

#### EMBEDDED_DOUBLE
- IEEE-754 double-precision floating-point without .rdata
- Pure integer-based bit manipulation for conversions
- Compile-time literal embedding
- Usage: `auto pi = 3.14159_embed;`

#### Integer Types (INT64, UINT64)
- Position-independent 64-bit integer operations
- Software implementations of all arithmetic operations
- Split high/low 32-bit representation for portability

### Platform Support

| Platform | i386 | x86_64 | armv7a | aarch64 | Status |
|----------|------|--------|--------|---------|--------|
| **Windows** | ✅ | ✅ | ✅ | ✅ | Full support |
| **Linux** | ✅ | ✅ | ✅ | ✅ | Full support |
| **UEFI** | ✅ | ✅ | ❌ | ✅ | Boot services only |

#### Windows
- Direct syscall interface (no kernel32.dll imports)
- PEB walking for runtime API resolution
- Console output via WriteConsoleW
- Process termination via NtTerminateProcess
- Memory allocation via NtAllocateVirtualMemory

#### Linux
- Direct syscall interface (no libc)
- Architecture-specific syscall implementations (i386, x86_64, armv7a, aarch64)
- Console output via write() syscall
- Process termination via exit() syscall
- Memory allocation via mmap/munmap

#### UEFI
- UEFI Boot Services interface
- Simple Text Output Protocol for console
- UEFI System Table access
- Memory allocation via UEFI AllocatePool
- Compatible with UEFI firmware environments

## Toolchain Dependencies

This project requires a specific toolchain to build position-independent code without `.rdata` section dependencies. Below is a comprehensive list of all required tools and their versions.

### Required Tools

| Tool | Version | Purpose |
|------|---------|---------|
| **CMake** | 3.20+ | Build system configuration |
| **Ninja** | Latest | Fast build executor |
| **Clang** | 20.x+ | C/C++ compiler with C++23 support |
| **Clang++** | 20.x+ | C++ compiler frontend |
| **LLD** | 20.x+ | LLVM linker (supports PE/COFF and ELF) |
| **QEMU** | Latest | Cross-architecture testing and emulation |

### Optional Tools (for post-build analysis)

| Tool | Version | Purpose |
|------|---------|---------|
| **llvm-objdump** | 20.x+ | Binary disassembly and section inspection |
| **llvm-objcopy** | 20.x+ | Extract `.text` section as PIC blob |
| **llvm-strings** | 20.x+ | Verify no string literals in `.rdata` |

### Platform-Specific Dependencies

#### Linux

| Tool | When Required | Purpose |
|------|---------------|---------|
| **gcc-multilib** | i386 builds | 32-bit library support |
| **g++-multilib** | i386 builds | 32-bit C++ support |
| **libc6:i386** | i386 runtime | 32-bit runtime libraries |
| **qemu-user-static** | Cross-arch testing | ARM64/ARM32 binary emulation on x86_64 |
| **qemu-system-x86_64** | UEFI testing | x86_64 UEFI firmware testing |
| **qemu-system-aarch64** | UEFI testing | ARM64 UEFI firmware testing |
| **binfmt-support** | Cross-arch testing | Binary format registration for QEMU |

#### Windows

| Tool | When Required | Purpose |
|------|---------------|---------|
| **certutil** | Post-build | Base64 encoding (built-in) |
| **QEMU** | Cross-arch/UEFI testing | ARM64 emulation and UEFI testing |

### Installation

#### Linux (Ubuntu/Debian)

Use the provided installation script:

```bash
# Install LLVM 20 toolchain
./scripts/install.sh

# Or specify a different version
LLVM_VER=20 ./scripts/install.sh
```

The script:
1. Adds the official LLVM APT repository (`apt.llvm.org`)
2. Installs `clang-20`, `clang++-20`, `lld-20`, and LLVM tools
3. Sets up `update-alternatives` for seamless version switching

**Manual installation:**

```bash
# Add LLVM repository
wget -qO- https://apt.llvm.org/llvm-snapshot.gpg.key | sudo apt-key add -
sudo add-apt-repository "deb http://apt.llvm.org/$(lsb_release -cs)/ llvm-toolchain-$(lsb_release -cs)-20 main"

# Install toolchain
sudo apt update
sudo apt install -y clang-20 clang++-20 lld-20 llvm-20

# Install CMake and Ninja
sudo apt install -y cmake ninja-build

# For i386 builds
sudo apt install -y gcc-multilib g++-multilib

# For cross-architecture testing and UEFI emulation
sudo apt install -y qemu-user-static qemu-system-x86 qemu-system-arm binfmt-support
```

#### Windows

1. Download the LLVM installer from [LLVM GitHub Releases](https://github.com/llvm/llvm-project/releases):
   - `LLVM-20.1.0-win64.exe` (or latest 20.x release)

2. Run the installer and add LLVM to your PATH

3. Install CMake from [https://cmake.org/download/](https://cmake.org/download/) or via Chocolatey:
   ```powershell
   choco install cmake
   ```

4. Install Ninja:
     ```powershell
     winget install -e --id Ninja-build.Ninja
     ```

5. (Optional) Install QEMU for cross-architecture testing and UEFI emulation:
   ```powershell
   choco install qemu
   ```

   Or download from [https://qemu.weilnetz.de/](https://qemu.weilnetz.de/)

### Version Requirements

| Requirement | Minimum Version | Recommended |
|-------------|-----------------|-------------|
| LLVM/Clang | 20.0 | 20.1.0+ |
| CMake | 3.20 | 3.28+ |
| Ninja | Any | Latest |
| QEMU | 6.0+ | Latest |

> **Note:** Clang 20+ is required for full C++23 support including `consteval`, concepts, and fold expressions used throughout this codebase. Earlier versions may compile but are not tested.

### Verifying Installation

```bash
# Check Clang version
clang --version
# Expected: clang version 20.x.x ...

# Check LLD version
ld.lld --version
# Expected: LLD 20.x.x ...

# Check CMake version
cmake --version
# Expected: cmake version 3.20.x or higher

# Check Ninja version
ninja --version

# Check QEMU version (optional but recommended for testing)
qemu-system-x86_64 --version
# Expected: QEMU emulator version 6.x.x or higher
```

## Build Requirements

- **Compiler**: LLVM/Clang 20+ with C++23 support
- **Linker**: LLD 20+ (LLVM Linker)
- **Build System**: CMake 3.20+ with Ninja
- **Target Architectures**: i386, x86_64, armv7a, aarch64
- **Platforms**: Windows, Linux, UEFI

### Continuous Integration

The project includes a comprehensive CI/CD pipeline via GitHub Actions (`.github/workflows/build.yml`):

- **Build Matrix**: Automatically builds all platform/architecture combinations
  - Linux: i386, x86_64, aarch64
  - Windows: i386, x86_64, aarch64
  - UEFI: i386, x86_64, aarch64
- **Automated Testing**:
  - Native execution for i386/x86_64 on matching runners
  - QEMU emulation for aarch64-linux binaries
  - Test results validated with exit code checks
- **Artifact Management**: All binaries, PIC blobs, and analysis files uploaded as artifacts
- **Quality Checks**: Post-build verification ensures no `.rdata` dependencies

## Building

CPP-PIC uses CMake with a custom toolchain file to ensure proper cross-compilation without unwanted MSVC-specific behavior.

### Quick Start

```bash
# Configure (required: specify toolchain file)
cmake -B build -G Ninja -DCMAKE_TOOLCHAIN_FILE=cmake/toolchain-clang.cmake

# Build
cmake --build build
```

### Build Options

| Option | Values | Default | Description |
|--------|--------|---------|-------------|
| `ARCHITECTURE` | `i386`, `x86_64`, `armv7a`, `aarch64` | `x86_64` | Target CPU architecture |
| `PLATFORM` | `windows`, `linux`, `uefi` | `windows` | Target operating system |
| `BUILD_TYPE` | `debug`, `release` | `release` | Build configuration |

### Examples

**Windows x64 Release (default):**
```bash
cmake -B build -G Ninja -DCMAKE_TOOLCHAIN_FILE=cmake/toolchain-clang.cmake
cmake --build build
```

**Windows i386 Debug:**
```bash
cmake -B build/i386-debug -G Ninja \
    -DCMAKE_TOOLCHAIN_FILE=cmake/toolchain-clang.cmake \
    -DARCHITECTURE=i386 \
    -DBUILD_TYPE=debug
cmake --build build/i386-debug
```

**Linux x64 Release:**
```bash
cmake -B build/linux-x64 -G Ninja \
    -DCMAKE_TOOLCHAIN_FILE=cmake/toolchain-clang.cmake \
    -DPLATFORM=linux
cmake --build build/linux-x64
```

**Linux ARM64 Debug:**
```bash
cmake -B build/linux-arm64 -G Ninja \
    -DCMAKE_TOOLCHAIN_FILE=cmake/toolchain-clang.cmake \
    -DPLATFORM=linux \
    -DARCHITECTURE=aarch64 \
    -DBUILD_TYPE=debug
cmake --build build/linux-arm64
```

**UEFI x64 Release:**
```bash
cmake -B build/uefi-x64 -G Ninja \
    -DCMAKE_TOOLCHAIN_FILE=cmake/toolchain-clang.cmake \
    -DPLATFORM=uefi \
    -DARCHITECTURE=x86_64
cmake --build build/uefi-x64
```

### Build Outputs

After building, artifacts are placed in `bin/<BUILD_TYPE>/<TARGET_TRIPLE>/`:

| File | Description |
|------|-------------|
| `<triple>.exe` / `<triple>.elf` | Main executable |
| `<triple>.bin` | Extracted `.text` section (PIC blob) |
| `<triple>.b64.txt` | Base64-encoded PIC blob |
| `<triple>.txt` | Disassembly and section dump |
| `<triple>.strings.txt` | Extracted strings (should be minimal) |
| `<triple>.map.txt` | Linker map file |

### Target Triples

| Architecture | Windows | Linux | UEFI |
|--------------|---------|-------|------|
| i386 | `i386-pc-windows-gnu` | `i386-unknown-linux-gnu` | `i386-unknown-windows`* |
| x86_64 | `x86_64-pc-windows-gnu` | `x86_64-unknown-linux-gnu` | `x86_64-unknown-uefi` |
| armv7a | `armv7a-pc-windows-gnu` | `armv7a-unknown-linux-gnueabi` | N/A |
| aarch64 | `aarch64-pc-windows-gnu` | `aarch64-unknown-linux-gnu` | `aarch64-unknown-windows`* |

\* Only `x86_64-unknown-uefi` is officially supported in LLVM/Clang. For i386 and aarch64, we use `-unknown-windows` as a compatible fallback since UEFI uses PE/COFF format.

## Optimization Levels

All LLVM optimization levels are supported and tested:

**Default Settings:**
- **DEBUG builds** use `-Og` (optimized for debugging experience)
- **RELEASE builds** use `-O3` (maximum performance optimization)

- DEBUG builds (`-Og`) include additional debug information and CodeView symbols for better debugging
- RELEASE builds include LTO (Link-Time Optimization), function inlining, and loop unrolling
- No string literals or floating-point constants are stored in `.rdata` for any optimization level
- To customize optimization, modify `OPTIMIZATION_FLAGS` in `CMakeLists.txt`

## Technical Details

### String Embedding

Strings are embedded using the `_embed` suffix operator:

```cpp
Logger::Info<WCHAR>(L"Application starting..."_embed);
Console::WriteFormatted<WCHAR>(L"Value: %d\n"_embed, 42);
```

**Implementation**: The `EMBEDDED_STRING` class uses `NOINLINE` and `DISABLE_OPTIMIZATION` attributes to force runtime construction. Characters are written one-by-one using fold expressions, preventing the compiler from merging them into .rdata.

**Assembly Example** (x64):
```asm
movw $0x43, (%rsi,%rax,2)    ; Write 'C'
movw $0x50, (%rsi,%rax,2)    ; Write 'P'
movw $0x50, (%rsi,%rax,2)    ; Write 'P'
```

### Floating-Point Embedding

Floating-point literals are embedded using the `_embed` suffix operator:

```cpp
DOUBLE pi = 3.14159_embed;
Console::WriteFormatted<WCHAR>(L"Pi: %.5f\n"_embed, pi);
```

**Implementation**: The `EMBEDDED_DOUBLE` struct constructs IEEE-754 bit patterns at compile-time, which are then embedded as 64-bit immediate values in code.

**Assembly Example** (x64):
```asm
movabsq $0x400921f9f01b866e, %rdx    ; 3.14159 as 64-bit immediate
```

### Double-to-Integer Conversion

The `DOUBLE` class provides pure integer-based conversions:

```cpp
DOUBLE value = 42.7_embed;
INT64 int_val = (INT64)value;    // Pure bit manipulation, no FPU
UINT32 uint_val = (UINT32)value; // No SSE cvttsd2si instructions
```

**Implementation**: Extracts IEEE-754 components (sign, exponent, mantissa) using bitwise operations, then reconstructs the integer value through bit shifting. Completely eliminates FPU/SSE conversion instructions that would require .rdata constants.

## Compiler & Linker Flags Reference

This section provides a comprehensive reference for all compiler and linker flags used in the CPP-PIC build system. Understanding these flags is essential for maintaining position-independence and eliminating `.rdata` dependencies.

### PIC-Critical Flags

These flags are **essential** for achieving position-independent code without `.rdata` section dependencies:

| Flag | Purpose | Why It's Critical for PIC |
|------|---------|---------------------------|
| `-fno-jump-tables` | Prevents switch statements from generating jump tables | Jump tables are stored in `.rdata`, creating relocations |
| `-fno-rtti` | Disables C++ runtime type information | RTTI generates `typeinfo` structures in `.rdata` |
| `-fno-exceptions` | Disables C++ exception handling | Exception tables (`.pdata`, `.xdata`) create data sections |
| `-fno-builtin` | Disables compiler built-in function replacements | Built-ins may reference `.rdata` constants |
| `/MERGE:.rdata=.text` | (Windows) Merges read-only data into code section | Ensures any residual `.rdata` becomes part of `.text` |

### Base Compiler Flags

Required for all builds regardless of platform or architecture:

| Flag | Purpose |
|------|---------|
| `-std=c++23` | Enable C++23 standard (required for `consteval`, concepts, fold expressions) |
| `-Werror` | Treat all warnings as errors for strict code quality |
| `-Wall -Wextra` | Enable comprehensive compiler warnings |
| `-Wno-gnu-string-literal-operator-template` | Suppress warning for GNU extension used in `_embed` literals |
| `-Qn` | Suppress compiler identification in output (reduces binary fingerprint) |
| `-nostdlib` | Do not link standard C/C++ libraries (no CRT dependency) |
| `-fno-ident` | Do not emit `.comment` section with compiler version strings |
| `-mno-stack-arg-probe` | Disable Windows stack probing (`__chkstk` calls) |
| `-fno-stack-check` | Disable stack limit checking code generation |
| `-fshort-wchar` | Use 2-byte `wchar_t` (matches Windows ABI for wide strings) |
| `-msoft-float` | Use software floating-point ABI (no FPU register dependencies) |

### Size Optimization Flags

Enable linker dead-code elimination:

| Flag | Purpose |
|------|---------|
| `-ffunction-sections` | Place each function in its own section |
| `-fdata-sections` | Place each data item in its own section |
| `--gc-sections` | (Linker) Garbage collect unused sections |

### Debug Build Flags

Additional flags for debug builds:

| Flag | Platform | Purpose |
|------|----------|---------|
| `-Og` | All | Optimize for debugging experience |
| `-ferror-limit=200` | All | Show up to 200 errors before stopping |
| `-g3` | All | Maximum debug information level |
| `-gcodeview` | Windows | Generate CodeView debug format (for WinDbg/VS) |
| `-fno-omit-frame-pointer` | All | Keep frame pointer for accurate stack traces |
| `/DEBUG` | Windows | Include debug information in PE file |

### Release Build Flags

Additional flags for release builds:

| Flag | Platform | Purpose |
|------|----------|---------|
| `-O3` | All | Maximum optimization level |
| `-flto` / `-flto=full` | All | Link-time optimization (cross-module inlining) |
| `-finline-functions` | Windows | Aggressively inline functions |
| `-funroll-loops` | Windows | Unroll loops to reduce branch overhead |
| `-fwhole-program-vtables` | Windows | LTO optimization for devirtualization |
| `-fno-asynchronous-unwind-tables` | All | No `.eh_frame` section (reduces size) |
| `-fno-unwind-tables` | All | No unwind tables (exceptions disabled) |
| `--strip-all` | All | Remove all symbol and debug information |
| `/OPT:REF` | Windows | Remove unreferenced functions/data |
| `/OPT:ICF` | Windows | Fold identical code sequences |
| `/RELEASE` | Windows | Set release flag in PE header |
| `/LTCG` | Windows | Link-time code generation |

### Base Linker Flags

| Flag | Purpose |
|------|---------|
| `-fuse-ld=lld` | Use LLVM's LLD linker (fast, cross-platform, required for COFF/ELF) |
| `-nostdlib` | Do not link standard libraries (no CRT startup code) |

### Windows-Specific Linker Flags

| Flag | Purpose |
|------|---------|
| `/Entry:_start` | Use `_start` as entry point (bypass CRT initialization) |
| `/SUBSYSTEM:CONSOLE` | Console application subsystem |
| `/ORDER:@orderfile.txt` | Control function placement order in `.text` section |
| `/MERGE:.rdata=.text` | Merge read-only data into code section (critical for PIC) |
| `/BASE:0x400000` | (i386) Preferred load address for 32-bit Windows |
| `/FILEALIGN:0x1000` | (i386) File section alignment (4KB pages) |
| `/MAP:file.map.txt` | Generate linker map file for analysis |

### Linux-Specific Linker Flags

| Flag | Purpose |
|------|---------|
| `-T linker.script` | Use custom linker script for section layout control |
| `-e _start` | Specify `_start` as the entry point symbol |
| `-Map=file.map.txt` | Generate linker map file for analysis |

### Preprocessor Definitions

Architecture and platform macros defined during compilation:

| Define | When Set |
|--------|----------|
| `ARCHITECTURE_I386` | Building for i386 |
| `ARCHITECTURE_X86_64` | Building for x86_64 |
| `ARCHITECTURE_ARMV7A` | Building for ARMv7-A |
| `ARCHITECTURE_AARCH64` | Building for AArch64 |
| `PLATFORM_WINDOWS` | Building for Windows |
| `PLATFORM_LINUX` | Building for Linux |
| `PLATFORM_WINDOWS_*` | Combined platform+arch (e.g., `PLATFORM_WINDOWS_X86_64`) |
| `PLATFORM_LINUX_*` | Combined platform+arch (e.g., `PLATFORM_LINUX_AARCH64`) |
| `DEBUG` | Debug build only |

## Use Cases

| Domain | Examples |
|--------|----------|
| **Embedded & IoT** | Microcontroller firmware, RTOS, bare-metal programming |
| **Kernel & Drivers** | Windows/Linux kernel modules, hypervisors, HAL |
| **Bootloaders** | UEFI applications, BIOS, firmware, secure boot |
| **Security Research** | Penetration testing, CTF tools, vulnerability research |
| **Education** | Compiler construction, OS development, binary analysis |

### Why CPP-PIC?

- **Embedded**: No separate data sections required—modern C++ in resource-constrained environments
- **Kernel**: C++23 features without paged memory or runtime initialization
- **Bootloaders**: Works in pre-OS environments with no memory manager or heap
- **Security**: Position-independent payloads without relocation handling

## Getting Started

### Quick Start: Your First CPP-PIC Program

Here's a simple example demonstrating the key features:

```cpp
#include "platform.h"
#include "console.h"

ENTRYPOINT INT32 _start(VOID)
{
    // Initialize the runtime environment
    ENVIRONMENT_DATA envData;
    Initialize(&envData);

    // Embedded strings - no .rdata needed!
    Console::Write<WCHAR>(L"Welcome to CPP-PIC!\n"_embed);

    // Formatted output with embedded constants
    Console::WriteFormatted<WCHAR>(
        L"Pi is approximately %.5f\n"_embed,
        3.14159_embed  // Embedded double literal
    );

    // Integer formatting
    Console::WriteFormatted<WCHAR>(
        L"The answer is %d\n"_embed,
        42
    );

    // Hex output
    Console::WriteFormatted<WCHAR>(
        L"Hex value: 0x%X\n"_embed,
        0xDEADBEEF
    );

    ExitProcess(0);
}
```

### What Makes This Special?

Looking at this code, it might seem ordinary. But here's what's happening under the hood:

1. **No CRT initialization** - `_start` is called directly by the OS
2. **No .rdata section** - All strings and constants are embedded in the `.text` section
3. **No relocations** - The entire program is position-independent
4. **No imports** - Direct syscalls to kernel (Windows) or kernel (Linux)

### Building Your First Project

**Using CMake (Recommended):**
```bash
# Windows x64 Release build
cmake -B build/windows/x86_64/release/cmake -G Ninja \
  -DCMAKE_TOOLCHAIN_FILE=cmake/toolchain-clang.cmake \
  -DARCHITECTURE=x86_64 -DPLATFORM=windows -DBUILD_TYPE=release
cmake --build build/windows/x86_64/release/cmake

# Linux x64 Debug build
cmake -B build/linux/x86_64/debug/cmake -G Ninja \
  -DCMAKE_TOOLCHAIN_FILE=cmake/toolchain-clang.cmake \
  -DARCHITECTURE=x86_64 -DPLATFORM=linux -DBUILD_TYPE=debug
cmake --build build/linux/x86_64/debug/cmake

# UEFI x64 Release build
cmake -B build/uefi/x86_64/release/cmake -G Ninja \
  -DCMAKE_TOOLCHAIN_FILE=cmake/toolchain-clang.cmake \
  -DARCHITECTURE=x86_64 -DPLATFORM=uefi -DBUILD_TYPE=release
cmake --build build/uefi/x86_64/release/cmake
```

**Using VSCode (Easiest):**
1. Open project in VSCode
2. Press `Ctrl+Shift+B` to build all configurations
3. Press `F5` and select a debug configuration to run
4. See [.vscode/README.md](.vscode/README.md) for all available configurations

The VSCode integration provides:
- **20+ pre-configured build tasks** for all platforms/architectures
- **Debug and Release configurations** for Windows, Linux, and UEFI
- **Automated UEFI testing** in QEMU with one-click launch
- **IntelliSense support** for C++23 features

### Verifying Position Independence

After building, verify that strings aren't in `.rdata`:

```batch
# Check .rdata section size (should be minimal)
llvm-objdump -h bin\RELEASE\x86_64-pc-windows-gnu\x86_64-pc-windows-gnu.exe | findstr .rdata

# Extract strings (should find none of your literal strings)
llvm-strings bin\RELEASE\x86_64-pc-windows-gnu\x86_64-pc-windows-gnu.exe

# Disassemble and see immediate values
llvm-objdump -d bin\RELEASE\x86_64-pc-windows-gnu\x86_64-pc-windows-gnu.exe | findstr "mov.*\$0x"
```

Expected output:
- `.rdata`: Only 32 bytes (x64) or 16 bytes (i386) containing compiler constants
- Strings: None of your embedded strings appear in string table
- Disassembly: Characters embedded as immediate operands like `movw $0x57, (%rax)`

---

## Technical Deep Dive

### Architecture Overview

```
┌─────────────────────────────────────────────────────────────┐
│                     CPP-PIC Runtime                         │
├─────────────────────────────────────────────────────────────┤
│                                                              │
│  ┌────────────────────┐         ┌─────────────────────┐    │
│  │  Embedded Types    │         │  Console I/O        │    │
│  ├────────────────────┤         ├─────────────────────┤    │
│  │ • EMBEDDED_STRING  │────────▶│ • Write()           │    │
│  │ • EMBEDDED_DOUBLE  │         │ • WriteFormatted()  │    │
│  │ • INT64/UINT64     │         │ • StringFormatter   │    │
│  │ • DOUBLE           │         └─────────────────────┘    │
│  └────────────────────┘                                     │
│           │                                                  │
│           ▼                                                  │
│  ┌────────────────────────────────────────────────────────┐ │
│  │              Platform Abstraction                      │ │
│  ├────────────────────────────────────────────────────────┤ │
│  │  Windows          │  Linux            │  UEFI          │ │
│  │  • PEB Walking    │  • syscalls       │  • Boot Svcs   │ │
│  │  • NtDll syscalls │  • write()/exit() │  • System Tbl  │ │
│  │  • WriteConsoleW  │  • mmap/munmap    │  • Text Proto  │ │
│  └────────────────────────────────────────────────────────┘ │
│                                                              │
└─────────────────────────────────────────────────────────────┘
```

### String Embedding: A Deep Dive

**The Challenge:**

When you write `const char* str = "Hello";`, the compiler:
1. Stores "Hello\0" in the `.rdata` section
2. Generates a pointer to that location
3. Uses the pointer in your code

This creates a relocation that must be fixed up when the binary loads.

**The CPP-PIC Solution:**

```cpp
// Step 1: User writes this
auto str = "Hello"_embed;

// Step 2: Compiler expands to character pack
EMBEDDED_STRING<char, 'H', 'e', 'l', 'l', 'o'> str;

// Step 3: Constructor runs at runtime
NOINLINE DISABLE_OPTIMIZATION EMBEDDED_STRING() {
    USIZE i = 0;
    ((data[i++] = 'H'),
     (data[i++] = 'e'),
     (data[i++] = 'l'),
     (data[i++] = 'l'),
     (data[i++] = 'o'));
    data[i] = 0;
}

// Step 4: Assembly output (each character as immediate)
movw $0x48, (%rax)      ; 'H'
movw $0x65, 2(%rax)     ; 'e'
movw $0x6C, 4(%rax)     ; 'l'
movw $0x6C, 6(%rax)     ; 'l'
movw $0x6F, 8(%rax)     ; 'o'
movw $0x00, 10(%rax)    ; '\0'
```

**Key Insights:**

- `NOINLINE` prevents the function from being inlined
- `DISABLE_OPTIMIZATION` prevents constant folding
- Fold expressions `((data[i++] = Cs), ...)` unpack the character pack
- Each character becomes an immediate operand in machine code
- No `.rdata` reference, no relocation needed

### Floating-Point Embedding: IEEE-754 Bit Magic

**The Challenge:**

Traditional floating-point literals:
```cpp
double pi = 3.14159;  // Stored in .rdata as 0x400921f9f01b866e
```

Generates assembly:
```asm
movsd .LCPI0_0(%rip), %xmm0  ; Load from .rdata
```

**The CPP-PIC Solution:**

```cpp
// Step 1: User writes this
auto pi = 3.14159_embed;

// Step 2: Compile-time conversion to bits
consteval EMBEDDED_DOUBLE(double v) {
    bits = __builtin_bit_cast(unsigned long long, v);
    // bits = 0x400921f9f01b866e
}

// Step 3: Runtime reconstruction
operator double() const {
    return __builtin_bit_cast(double, bits);
}

// Step 4: Assembly output (immediate value)
movabsq $0x400921f9f01b866e, %rax  ; Pi as immediate
```

**IEEE-754 Double Precision Format:**
```
Sign (1 bit) | Exponent (11 bits) | Mantissa (52 bits)
     0       |   10000000000      | 1001001000011111001111100001101101110

For 3.14159:
  Sign: 0 (positive)
  Exponent: 1024 (biased by 1023 = 1)
  Mantissa: Represents 1.5707963...
```

### Integer Arithmetic Without Hardware Support

CPP-PIC implements 64-bit integer operations in pure software, ensuring portability and position-independence:

```cpp
// 64-bit multiplication using 32-bit operations
UINT64 operator*(const UINT64& a, const UINT64& b) {
    // Split into high/low 32-bit words
    UINT32 a_lo = a.low, a_hi = a.high;
    UINT32 b_lo = b.low, b_hi = b.high;

    // Perform partial products
    UINT64 lo_lo = (UINT64)a_lo * b_lo;
    UINT64 lo_hi = (UINT64)a_lo * b_hi;
    UINT64 hi_lo = (UINT64)a_hi * b_lo;

    // Combine with carry handling
    UINT64 result;
    result.low = lo_lo & 0xFFFFFFFF;
    UINT64 carry = (lo_lo >> 32) + (lo_hi & 0xFFFFFFFF) + (hi_lo & 0xFFFFFFFF);
    result.high = carry.low;

    return result;
}
```

This approach:
- Works on any architecture
- Generates predictable code
- Avoids compiler intrinsics that might use `.rdata`

---

## Binary Analysis

### Verifying .rdata Contents

```batch
llvm-objdump -h output.exe | grep .rdata
llvm-objdump -s --section=.rdata output.exe
```

Expected output (x64):
```
  1 .rdata        00000020 0000000140007000 DATA

Contents of section .rdata:
 140007000 ffffffff ffffffff 00000000 00000000
 140007010 ffffffff ffffffff 00000000 00000000
```

### Verifying String Embedding

```batch
llvm-objdump -d output.exe | grep "mov.*\$0x"
```

Expected output (character-by-character writes):
```asm
movw $0x43, (%rsi,%rax,2)
movw $0x50, (%rsi,%rax,2)
movw $0x50, (%rsi,%rax,2)
```

### Verifying Floating-Point Embedding

```batch
llvm-objdump -d output.exe | grep movabsq
```

Expected output (64-bit immediate values):
```asm
movabsq $0x400921f9f01b866e, %rdx
```

---

## UEFI Platform Support

CPP-PIC includes comprehensive UEFI (Unified Extensible Firmware Interface) support, enabling the runtime to operate in pre-boot environments without an operating system.

### UEFI Features

**Entry Point**: UEFI applications use `EfiMain` as the entry point, which receives:
- `EFI_HANDLE ImageHandle`: Handle to the loaded image
- `EFI_SYSTEM_TABLE* SystemTable`: Pointer to UEFI System Table

**System Services**:
- **Boot Services**: Memory allocation, protocol handling, event management
- **Runtime Services**: Variable storage, time services (available after OS handoff)
- **Console I/O**: Simple Text Output Protocol for console output

**Memory Management**:
- Uses UEFI `AllocatePool`/`FreePool` for dynamic memory
- No reliance on traditional OS memory managers (mmap, VirtualAlloc)
- Compatible with pre-MMU boot environments

### Building for UEFI

```bash
# UEFI x64 application
cmake -B build/uefi-x64 -G Ninja \
    -DCMAKE_TOOLCHAIN_FILE=cmake/toolchain-clang.cmake \
    -DPLATFORM=uefi \
    -DARCHITECTURE=x86_64
cmake --build build/uefi-x64

# Output: x86_64-unknown-windows.efi
```

### UEFI Output Format

UEFI binaries use the PE32+ format with:
- Subsystem: `EFI_APPLICATION` (10)
- Entry point: `EfiMain`
- Extension: `.efi`
- Compatible with UEFI firmware (version 2.x+)

### Testing UEFI Applications

#### Quick Start: Automated Test Scripts

The easiest way to test UEFI builds is using the provided automated scripts:

**Linux/macOS:**
```bash
# Build and run x86_64 UEFI in QEMU (default)
./scripts/run-uefi-qemu.sh

# Build and run ARM64 UEFI
./scripts/run-uefi-qemu.sh aarch64

# Run debug build
./scripts/run-uefi-qemu.sh x86_64 debug
```

**Windows (PowerShell):**
```powershell
# Build and run x86_64 UEFI in QEMU (default)
.\scripts\run-uefi-qemu.ps1

# Build and run ARM64 UEFI
.\scripts\run-uefi-qemu.ps1 aarch64

# Run debug build
.\scripts\run-uefi-qemu.ps1 x86_64 debug
```

These scripts automatically:
- Verify the UEFI binary exists
- Create a FAT filesystem image
- Install the UEFI application to the correct ESP (EFI System Partition) path
- Detect and configure OVMF/AAVMF firmware
- Launch QEMU with appropriate settings for the target architecture

**Prerequisites:**
- QEMU installed (see installation section above)
- OVMF firmware:
  - **Ubuntu/Debian:** `sudo apt install ovmf qemu-efi-aarch64`
  - **Fedora:** `sudo dnf install edk2-ovmf edk2-aarch64`
  - **Arch:** `sudo pacman -S edk2-ovmf`
  - **Windows:** OVMF firmware included with QEMU installation

#### Manual Testing: QEMU with OVMF (UEFI firmware)

For manual testing or custom configurations:

```bash
# Create a FAT filesystem image
dd if=/dev/zero of=uefi.img bs=1M count=64
mkfs.vfat uefi.img
mkdir -p mnt
mount -o loop uefi.img mnt
mkdir -p mnt/EFI/BOOT
cp build/uefi/x86_64/release/output.efi mnt/EFI/BOOT/BOOTX64.EFI
umount mnt

# Run with QEMU
qemu-system-x86_64 \
    -drive if=pflash,format=raw,readonly=on,file=/usr/share/OVMF/OVMF_CODE.fd \
    -drive format=raw,file=uefi.img \
    -machine q35 -m 512M -nographic
```

**Option 2: Physical Hardware**
1. Copy `.efi` file to a FAT32 USB drive under `\EFI\BOOT\BOOTX64.EFI`
2. Boot from USB in UEFI mode
3. UEFI firmware will automatically execute the application

**Option 3: VirtualBox/VMware**
- Configure VM to use UEFI firmware
- Attach the `.efi` file as a virtual disk or ISO
- Boot from the UEFI shell

### UEFI Architecture Support

| Architecture | Support | Triple | Notes |
|--------------|---------|--------|-------|
| i386 | ✅ | `i386-unknown-windows` | UEFI 1.x/2.x (rare) |
| x86_64 | ✅ | `x86_64-unknown-windows` | Most common UEFI platform |
| armv7a | ❌ | N/A | Not supported by UEFI spec |
| aarch64 | ✅ | `aarch64-unknown-windows` | ARM64 servers, some laptops |

### UEFI Limitations

- **No Exception Handling**: UEFI firmware doesn't support C++ exceptions (already disabled in CPP-PIC)
- **No RTTI**: Runtime Type Information unavailable (already disabled)
- **Limited Standard Library**: No C/C++ standard library (CPP-PIC provides custom runtime)
- **Boot Services Only**: Runtime services are limited after `ExitBootServices()` is called
- **Memory Constraints**: Available memory depends on firmware implementation

### Use Cases for UEFI

- **Bootloaders**: Custom OS loaders and boot managers
- **Firmware Tools**: Hardware diagnostics, firmware updates, configuration utilities
- **Pre-Boot Security**: Secure boot tools, TPM management, disk encryption setup
- **Recovery Environments**: Backup/restore tools, partition managers
- **Embedded Systems**: Headless server management, BMC interfaces

---

## Project Structure

```
cpp-pic-private/
├── cmake/
│   ├── toolchain-clang.cmake        # Clang cross-compilation toolchain
│   └── base64_encode.cmake          # Base64 encoding for PIC blobs
├── include/
│   ├── runtime/
│   │   ├── platform/
│   │   │   ├── primitives/
│   │   │   │   ├── primitives.h     # Combined primitives header
│   │   │   │   ├── embedded_string.h # String embedding
│   │   │   │   ├── embedded_double.h # FP embedding
│   │   │   │   ├── double.h         # IEEE-754 operations
│   │   │   │   ├── int64.h          # Signed 64-bit integers
│   │   │   │   └── uint64.h         # Unsigned 64-bit integers
│   │   │   ├── windows/
│   │   │   │   ├── kernel32.h       # Kernel32 API resolution
│   │   │   │   ├── ntdll.h          # NtDll syscall interface
│   │   │   │   ├── pe.h             # PE file parsing
│   │   │   │   ├── peb.h            # PEB walking
│   │   │   │   └── windows_types.h  # Windows type definitions
│   │   │   ├── linux/
│   │   │   │   └── syscall.h        # Linux syscall interface
│   │   │   ├── uefi/
│   │   │   │   ├── boot_services.h  # UEFI Boot Services
│   │   │   │   ├── system_table.h   # UEFI System Table
│   │   │   │   └── uefi_types.h     # UEFI type definitions
│   │   │   ├── platform.h           # Platform abstraction
│   │   │   └── allocator.h          # Memory allocator
│   │   ├── console.h                # Console I/O
│   │   ├── logger.h                 # Logging utilities
│   │   ├── memory.h                 # Memory operations
│   │   ├── string.h                 # String utilities
│   │   ├── string_formatter.h       # Printf-style formatting
│   │   └── djb2.h                   # DJB2 hash function
│   └── tests/
│       ├── uint64_tests.h           # UINT64 arithmetic tests
│       ├── int64_tests.h            # INT64 arithmetic tests
│       ├── double_tests.h           # Floating-point tests
│       ├── string_tests.h           # String operation tests
│       ├── string_formatter_tests.h # Printf formatting tests
│       ├── djb2_tests.h             # Hash function tests
│       └── memory_tests.h           # Memory operation tests
├── src/
│   ├── start.cc                     # Entry point
│   └── runtime/
│       ├── platform/
│       │   ├── platform.cc          # Platform initialization
│       │   ├── allocator.cc         # Allocator implementation
│       │   ├── windows/
│       │   │   ├── platform.windows.cc
│       │   │   ├── kernel32.cc
│       │   │   ├── ntdll.cc
│       │   │   ├── pe.cc
│       │   │   ├── peb.cc
│       │   │   └── allocator.windows.cc
│       │   ├── linux/
│       │   │   ├── platform.linux.cc
│       │   │   ├── platform.linux.*.cc  # Arch-specific (i386, x86_64, armv7a, aarch64)
│       │   │   ├── syscall.cc
│       │   │   └── allocator.linux.cc
│       │   └── uefi/
│       │       ├── platform.uefi.cc
│       │       ├── boot_services.cc
│       │       └── allocator.uefi.cc
│       └── console/
│           ├── console.cc           # Console abstraction
│           ├── windows/
│           │   └── console.windows.cc
│           ├── linux/
│           │   └── console.linux.cc
│           └── uefi/
│               └── console.uefi.cc
├── bin/                             # Build output directory
│   ├── debug/
│   │   └── <target-triple>/
│   └── release/
│       └── <target-triple>/
├── CMakeLists.txt                   # CMake build configuration
├── orderfile.txt                    # Linker function ordering
├── linker.script                    # Linux linker script
├── scripts/                         # Automation scripts
│   ├── install.sh                   # Linux/macOS dependency installer
│   ├── run-uefi-qemu.sh             # UEFI test launcher (Linux/macOS)
│   ├── run-uefi-qemu.ps1            # UEFI test launcher (Windows)
│   ├── loader.ps1                   # PIC blob loader (Windows)
│   └── README.md                    # Scripts documentation
├── docs/                            # Documentation
│   ├── architecture.md              # Architecture overview
│   └── platform_guide.md            # Platform-specific details
├── tests/                           # Test files
│   ├── unit/                        # Unit tests (future)
│   └── README.md                    # Test documentation
├── .vscode/                         # VSCode integration
│   ├── launch.json                  # Debug/run configurations
│   ├── tasks.json                   # Build tasks
│   ├── c_cpp_properties.json        # IntelliSense config
│   └── README.md                    # VSCode documentation
└── README.md
```

### Development Workflow

```
┌─────────────────────────────────────────────────────────────────┐
│                      CPP-PIC Development                        │
└─────────────────────────────────────────────────────────────────┘

VSCode Integration:
┌──────────────┐  Press F5   ┌──────────────┐  Auto-run  ┌─────────────┐
│   Edit Code  │ ────────────▶│  Build Task  │ ──────────▶│  Run/Debug  │
│   (C++23)    │              │  (CMake +    │            │  (Native/   │
│              │              │   Ninja)     │            │   QEMU)     │
└──────────────┘              └──────────────┘            └─────────────┘
                                     │
                                     ▼
                              ┌─────────────┐
                              │  Platform:  │
                              │  • Windows  │
                              │  • Linux    │
                              │  • UEFI     │
                              └─────────────┘

UEFI Testing Flow:
┌────────────────────────────────────────────────────────────────────────┐
│  F5 → Build UEFI → scripts/run-uefi-qemu.ps1 → QEMU + OVMF → Tests Run│
└────────────────────────────────────────────────────────────────────────┘
                                  ▼
                         ┌─────────────────┐
                         │  FAT Image      │
                         │  ├─ EFI/        │
                         │  │  └─ BOOT/    │
                         │  │     └─ *.EFI │
                         │  └─ (automated) │
                         └─────────────────┘
```

## Console Output Support

### Formatted Output

Supports printf-style formatting:

```cpp
Console::WriteFormatted<WCHAR>(L"Integer: %d\n"_embed, 42);
Console::WriteFormatted<WCHAR>(L"Float: %.5f\n"_embed, 3.14159_embed);
Console::WriteFormatted<WCHAR>(L"Hex: 0x%X\n"_embed, 255);
Console::WriteFormatted<WCHAR>(L"String: %ls\n"_embed, L"Hello");
```

### Format Specifiers

- `%d` - Signed decimal integer
- `%u` - Unsigned decimal integer
- `%ld` - Signed long integer
- `%X` - Uppercase hexadecimal
- `%x` - Lowercase hexadecimal
- `%f` - Floating-point (default precision)
- `%.Nf` - Floating-point (N decimal places)
- `%c` - Character
- `%s` - Narrow string (char*)
- `%ls` - Wide string (wchar_t*)
- `%p` - Pointer

## Testing

### Native Testing

Run the built executable to verify functionality:

```bash
# Linux x86_64 (native)
./build/linux/x86_64/release/output.elf

# Windows x86_64 (native)
.\build\windows\x86_64\release\output.exe
```

### Cross-Architecture Testing with QEMU

For testing binaries built for different architectures than your host system, use QEMU:

**Linux ARM64 on x86_64 host:**
```bash
# Build for aarch64-linux
cmake -B build/linux/aarch64/release/cmake -G Ninja \
    -DCMAKE_TOOLCHAIN_FILE=cmake/toolchain-clang.cmake \
    -DARCHITECTURE=aarch64 \
    -DPLATFORM=linux \
    -DBUILD_TYPE=release
cmake --build build/linux/aarch64/release/cmake

# Run with QEMU user-mode emulation
qemu-aarch64-static ./build/linux/aarch64/release/output.elf
```

**Linux ARM32 on x86_64 host:**
```bash
# Build for armv7a-linux
cmake -B build/linux/armv7a/release/cmake -G Ninja \
    -DCMAKE_TOOLCHAIN_FILE=cmake/toolchain-clang.cmake \
    -DARCHITECTURE=armv7a \
    -DPLATFORM=linux \
    -DBUILD_TYPE=release
cmake --build build/linux/armv7a/release/cmake

# Run with QEMU user-mode emulation
qemu-arm-static ./build/linux/armv7a/release/output.elf
```

**Note:** QEMU user-mode emulation (`qemu-aarch64-static`, `qemu-arm-static`) transparently executes foreign-architecture binaries on Linux hosts when `binfmt-support` is properly configured. The CI pipeline uses this for automated testing.

Expected output:
```
CPP-PIC Runtime Starting...
Hello, World! from C++23 CPP-PIC Runtime!
Hello, World! from C++23 CPP-PIC Runtime! (char)
The answer is: 42
Double: 3.14159
Hex: 0xFF
Character: A
Pointer: 0x12345678
Unsigned: 3000000000
Long: 1234567890
Null wide string: (null)
Null string: (null)
```

### Unit Test Suite

CPP-PIC includes a comprehensive unit test suite that validates core runtime functionality. Tests are located in `include/tests/` and run as part of the main executable.

#### Test Categories

| Test Suite | File | Description |
|------------|------|-------------|
| **UINT64 Tests** | `uint64_tests.h` | Unsigned 64-bit integer operations |
| **INT64 Tests** | `int64_tests.h` | Signed 64-bit integer operations |
| **Double Tests** | `double_tests.h` | IEEE-754 floating-point operations |
| **String Tests** | `string_tests.h` | String manipulation functions |
| **StringFormatter Tests** | `string_formatter_tests.h` | Printf-style formatting |
| **DJB2 Tests** | `djb2_tests.h` | Hash function consistency |
| **Memory Tests** | `memory_tests.h` | Memory operations (Copy, Zero, Compare) |

#### UINT64 Test Coverage

Tests 64-bit unsigned integer software implementation:
- **Construction**: Default, single-arg, two-arg, and native `unsigned long long` constructors
- **Arithmetic**: Addition (with carry), subtraction (with borrow), multiplication, division, modulo
- **Bitwise**: AND, OR, XOR, NOT operations
- **Shifts**: Left/right shifts including cross-word shifts and edge cases (shift by 0, 32, 64+)
- **Comparisons**: `==`, `!=`, `<`, `<=`, `>`, `>=` operators
- **Increment/Decrement**: Pre/post increment and decrement with carry/borrow
- **Overflow**: Wraparound behavior verification

#### INT64 Test Coverage

Tests 64-bit signed integer software implementation:
- All UINT64 tests plus signed-specific behavior
- Negative number handling
- Sign extension in shifts
- Signed comparison operators

#### StringFormatter Test Coverage

Tests printf-style formatting without `.rdata` dependencies:
- **Integer formats**: `%d` (signed), `%u` (unsigned), `%ld` (long)
- **Hex formats**: `%x` (lowercase), `%X` (uppercase), `%#x` (with prefix)
- **String formats**: `%s` (narrow), `%ls` (wide)
- **Character format**: `%c`
- **Float format**: `%f`, `%.Nf` (precision specifiers)
- **Width/padding**: Right-align, left-align (`%-`), zero-padding (`%0`)
- **Percent literal**: `%%`

#### Running Tests

Tests run automatically when the executable starts. Output shows pass/fail for each test category:

```
Running UINT64 Tests...
  PASSED: Construction
  PASSED: Addition
  PASSED: Subtraction
  PASSED: Multiplication
  PASSED: Division
  PASSED: Modulo
  PASSED: Bitwise operations
  PASSED: Shift operations
  PASSED: Comparisons
  PASSED: Increment/Decrement
  PASSED: Overflow behavior
All UINT64 tests passed!
```

#### Adding New Tests

To add tests, create a new test class following the existing pattern:

```cpp
#pragma once
#include "primitives.h"
#include "logger.h"

class MyFeatureTests {
public:
    static BOOL RunAll() {
        BOOL allPassed = TRUE;
        Logger::Info<WCHAR>(L"Running MyFeature Tests..."_embed);

        if (!TestSomething()) {
            allPassed = FALSE;
            Logger::Error<WCHAR>(L"  FAILED: Something"_embed);
        } else {
            Logger::Info<WCHAR>(L"  PASSED: Something"_embed);
        }

        return allPassed;
    }

private:
    static BOOL TestSomething() {
        // Test implementation using embedded strings
        return TRUE;
    }
};
```

Then include and call from `start.cc`:
```cpp
#include "tests/my_feature_tests.h"
// ...
MyFeatureTests::RunAll();
```

## Quick Reference

### VSCode Integration (Recommended)

The easiest way to build and test CPP-PIC is using the integrated VSCode configurations:

**Quick Actions:**
- `Ctrl+Shift+B` - Build all configurations
- `F5` - Run/Debug with selected configuration
- `Ctrl+Shift+D` - Open Run and Debug panel

**Available Configurations:**
- **Windows**: i386, x86_64, armv7a, aarch64 (debug/release)
- **Linux (WSL)**: i386, x86_64, armv7a, aarch64 (debug)
- **UEFI (QEMU)**: i386, x86_64, aarch64 (debug/release)

**UEFI Testing in One Click:**
1. Select UEFI configuration from debug dropdown
2. Press `F5`
3. Watch tests run in QEMU

See [.vscode/README.md](.vscode/README.md) for detailed documentation.

### Common Commands

**Build Commands:**
```bash
# Linux/Windows - using CMake directly
cmake -B build/<platform>/<arch>/<type>/cmake -G Ninja \
  -DCMAKE_TOOLCHAIN_FILE=cmake/toolchain-clang.cmake \
  -DARCHITECTURE=<arch> \
  -DPLATFORM=<platform> \
  -DBUILD_TYPE=<type>
cmake --build build/<platform>/<arch>/<type>/cmake

# Examples
cmake -B build/linux/x86_64/release/cmake -G Ninja \
  -DCMAKE_TOOLCHAIN_FILE=cmake/toolchain-clang.cmake \
  -DARCHITECTURE=x86_64 -DPLATFORM=linux -DBUILD_TYPE=release
cmake --build build/linux/x86_64/release/cmake
```

**Run Commands:**
```bash
# Native Linux
./build/linux/x86_64/release/output.elf

# Native Windows
.\build\windows\x86_64\release\output.exe

# Cross-architecture with QEMU (Linux ARM64 on x86_64)
qemu-aarch64-static ./build/linux/aarch64/release/output.elf

# UEFI in QEMU (automated)
./scripts/run-uefi-qemu.sh x86_64          # Linux/macOS
.\scripts\run-uefi-qemu.ps1 x86_64         # Windows
```

**Analysis Commands:**
```bash
# Disassemble and inspect sections
llvm-objdump -d -s -h build/<platform>/<arch>/<type>/output.<ext>

# Check for strings in .rdata (should be minimal)
llvm-strings build/<platform>/<arch>/<type>/output.<ext>

# Extract .text section as PIC blob
llvm-objcopy --dump-section=.text=output.bin build/<platform>/<arch>/<type>/output.<ext>

# View linker map
cat build/<platform>/<arch>/<type>/output.map.txt
```

### Platform/Architecture Matrix

| Command | Platform | Architecture | Output |
|---------|----------|--------------|--------|
| `-DPLATFORM=windows -DARCHITECTURE=x86_64` | Windows | x86-64 | `output.exe` (PE64) |
| `-DPLATFORM=windows -DARCHITECTURE=i386` | Windows | x86 (32-bit) | `output.exe` (PE32) |
| `-DPLATFORM=windows -DARCHITECTURE=aarch64` | Windows | ARM64 | `output.exe` (PE64) |
| `-DPLATFORM=linux -DARCHITECTURE=x86_64` | Linux | x86-64 | `output.elf` (ELF64) |
| `-DPLATFORM=linux -DARCHITECTURE=i386` | Linux | x86 (32-bit) | `output.elf` (ELF32) |
| `-DPLATFORM=linux -DARCHITECTURE=aarch64` | Linux | ARM64 | `output.elf` (ELF64) |
| `-DPLATFORM=linux -DARCHITECTURE=armv7a` | Linux | ARMv7-A | `output.elf` (ELF32) |
| `-DPLATFORM=uefi -DARCHITECTURE=x86_64` | UEFI | x86-64 | `output.efi` (PE64) |
| `-DPLATFORM=uefi -DARCHITECTURE=i386` | UEFI | x86 (32-bit) | `output.efi` (PE32) |
| `-DPLATFORM=uefi -DARCHITECTURE=aarch64` | UEFI | ARM64 | `output.efi` (PE64) |

### Build Output Structure

```
build/
├── linux/
│   ├── x86_64/
│   │   ├── release/
│   │   │   ├── output.elf           # Main executable
│   │   │   ├── output.bin           # Extracted .text section (PIC blob)
│   │   │   ├── output.txt           # Disassembly and section dump
│   │   │   ├── output.strings.txt   # Extracted strings
│   │   │   ├── output.b64.txt       # Base64-encoded PIC blob
│   │   │   ├── output.map.txt       # Linker map
│   │   │   └── cmake/               # CMake build files
│   │   └── debug/
│   │       └── (same structure)
│   ├── i386/
│   ├── aarch64/
│   └── armv7a/
├── windows/
│   ├── x86_64/
│   ├── i386/
│   └── aarch64/
└── uefi/
    ├── x86_64/
    ├── i386/
    └── aarch64/
```

### Essential Tools

| Tool | Purpose | Installation |
|------|---------|--------------|
| **clang-20** | C++23 compiler | `sudo apt install clang-20` / `choco install llvm` |
| **lld-20** | LLVM linker | Included with LLVM |
| **cmake** | Build system | `sudo apt install cmake` / `choco install cmake` |
| **ninja** | Build executor | `sudo apt install ninja-build` / `choco install ninja` |
| **qemu-user-static** | Cross-arch testing | `sudo apt install qemu-user-static` |
| **qemu-system-x86** | UEFI testing (x86) | `sudo apt install qemu-system-x86` / `choco install qemu` |
| **ovmf** | UEFI firmware | `sudo apt install ovmf` |

### Environment Variables

```bash
# Override LLVM version for install script
LLVM_VER=20 ./scripts/install.sh

# CMake build configuration
export CMAKE_BUILD_PARALLEL_LEVEL=8  # Parallel build jobs
```

### Troubleshooting

**Build fails with "toolchain file required":**
```bash
# Always use -DCMAKE_TOOLCHAIN_FILE
cmake -B build -G Ninja -DCMAKE_TOOLCHAIN_FILE=cmake/toolchain-clang.cmake
```

**QEMU "Exec format error" on Linux:**
```bash
# Install binfmt-support for transparent execution
sudo apt install qemu-user-static binfmt-support
```

**OVMF firmware not found:**
```bash
# Install UEFI firmware
sudo apt install ovmf qemu-efi-aarch64  # Ubuntu/Debian
sudo dnf install edk2-ovmf edk2-aarch64 # Fedora
```

**Tests fail with exit code != 0:**
- Check test output for specific failure
- Run in debug mode: `-DBUILD_TYPE=debug`
- Enable verbose output: `cmake --build build --verbose`

## Known Limitations

1. **String Size**: Very long strings (>1KB) may increase code size significantly due to character-by-character materialization
2. **Compile Time**: String embedding increases compilation time for large string literals
3. **SSE Usage**: While SSE instructions are used for arithmetic, they only operate on stack values (no .rdata loads)

## Performance Considerations

- String construction is performed at runtime (not compile-time), adding minimal overhead
- Each unique string literal generates a separate constructor function
- Floating-point operations use SSE when available, falling back to software implementations
- Integer operations are software-based for portability

## Documentation

For more detailed information, see:

- **[Architecture Guide](docs/architecture.md)** - Comprehensive architecture overview, component details, and design decisions
- **[Platform Guide](docs/platform_guide.md)** - Platform-specific implementation details, syscall interfaces, and porting guide
- **[Scripts Documentation](scripts/README.md)** - Automation script usage and reference
- **[Tests Documentation](tests/README.md)** - Testing structure and guidelines

## Contributing

This is a private research project. Code is provided for educational and authorized security research purposes only.

## License

Proprietary - All rights reserved

## Security Notice

This runtime is designed for position-independent code (PIC) environments including shellcode and injection payloads. It should only be used for:

- Authorized security testing and penetration testing
- Academic research and education
- Legitimate software development requiring PIC constraints
- Defensive security research and analysis

Unauthorized use for malicious purposes is strictly prohibited.
