# CPP-PIC: Position-Independent C++23 Runtime for Windows

**A Revolutionary Approach to Zero-Dependency C++ Code Generation**

[![License](https://img.shields.io/badge/license-Proprietary-blue.svg)](LICENSE)
[![C++23](https://img.shields.io/badge/C%2B%2B-23-blue.svg)](https://en.cppreference.com/w/cpp/23)
[![Platform](https://img.shields.io/badge/platform-Windows-blue.svg)](README.md)
[![Architecture](https://img.shields.io/badge/arch-i386%20%7C%20x86__64%20%7C%20armv7a%20%7C%20aarch64-orange.svg)](README.md)

---

## Table of Contents

- [Overview](#overview)
- [Key Features](#key-features)
- [Platform Support](#platform-support)
- [Quick Start](#quick-start)
- [How It Works](#how-it-works)
- [Building](#building)
- [Windows Implementation](#windows-implementation)
- [Cryptography](#cryptography)
- [Networking](#networking)
- [Logging](#logging)
- [Testing](#testing)
- [Use Cases](#use-cases)
- [Documentation](#documentation)

---

## Overview

CPP-PIC is a groundbreaking C++23 runtime library designed specifically for Windows that eliminates all `.rdata` section dependencies. By embedding constants directly in executable code, we've created a truly position-independent runtime suitable for shellcode, code injection, and embedded systems.

This project represents years of research into compiler behavior, binary structure, and low-level Windows programming. Whether you're conducting security research, developing kernel components, or exploring the boundaries of modern C++, CPP-PIC provides unprecedented control over your binary's memory layout.

## Key Features

- **Zero .rdata Dependencies**: All string literals and floating-point constants embedded as immediate values in code
- **Position-Independent**: No relocations required, suitable for shellcode and injection payloads
- **No CRT Required**: Complete standalone runtime with no standard library dependencies
- **Modern C++23**: Leverages compile-time features including concepts, consteval, and fold expressions
- **Windows-Native**: Direct syscalls, PEB walking, PE parsing - no imports
- **Multi-Architecture**: Supports i386, x86_64, armv7a, and aarch64
- **Full Optimization Support**: Works with all LLVM optimization levels (-O0 through -Oz)
- **Cryptography**: SHA-256/512, ChaCha20, ECC (Elliptic Curve), Base64, cryptographic RNG
- **Networking**: DNS resolution, HTTP client, WebSocket support, TLS 1.3 with certificate verification
- **File System**: Complete file and directory operations via NTAPI

## Platform Support

| Architecture | Target Triple | Status |
|-------------|---------------|--------|
| **i386** | `i386-pc-windows-gnu` | ✅ Full support |
| **x86_64** | `x86_64-pc-windows-gnu` | ✅ Full support |
| **armv7a** | `armv7a-pc-windows-gnu` | ✅ Full support |
| **aarch64** | `aarch64-pc-windows-gnu` | ✅ Full support |

All architectures support both debug and release builds with comprehensive testing via GitHub Actions CI/CD.

## Quick Start

### Prerequisites

**Required Tools:**
- **LLVM/Clang 20+** - Download from [LLVM GitHub Releases](https://github.com/llvm/llvm-project/releases)
  - Version 20.1.6 or later recommended
  - Must include LLD linker
- **CMake 3.20+** - Download from [cmake.org](https://cmake.org/download/)
- **Ninja** - Install via: `winget install -e --id Ninja-build.Ninja`

**Installation:**

1. Download and install LLVM 20+ for Windows (LLVM-20.1.6-win64.exe)
2. Add LLVM to your PATH during installation
3. Install CMake and Ninja
4. Verify installation:
```powershell
clang --version   # Should show version 20.x.x
cmake --version   # Should show version 3.20+
ninja --version
```

### Build

```bash
# Configure
cmake -B build -G Ninja

# Build
cmake --build build

# Run
.\build\windows\x86_64\release\output.exe
```

### Build Options

| Option | Values | Default | Description |
|--------|--------|---------|-------------|
| `ARCHITECTURE` | `i386`, `x86_64`, `armv7a`, `aarch64` | `x86_64` | Target CPU architecture |
| `PLATFORM` | `windows` | `windows` | Target platform |
| `BUILD_TYPE` | `debug`, `release` | `release` | Build configuration |

### Build Examples

**Windows x64 Release (default):**
```bash
cmake -B build -G Ninja
cmake --build build
```

**Windows i386 Debug:**
```bash
cmake -B build/windows/i386/debug/cmake -G Ninja \
    -DARCHITECTURE=i386 \
    -DBUILD_TYPE=debug
cmake --build build/windows/i386/debug/cmake
```

**Windows ARM64 Release:**
```bash
cmake -B build/windows/aarch64/release/cmake -G Ninja \
    -DARCHITECTURE=aarch64 \
    -DBUILD_TYPE=release
cmake --build build/windows/aarch64/release/cmake
```

## How It Works

CPP-PIC leverages cutting-edge C++23 features to achieve complete position independence through three key innovations:

### 1. Compile-Time String Decomposition

Using user-defined literal operators and variadic templates, strings are decomposed into individual characters at compile-time:

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

**Usage:**
```cpp
auto msg = "Hello, World!"_embed;   // Embedded in code, not .rdata
```

**Assembly Output:**
```asm
movw $0x48, (%rdi)      ; 'H'
movw $0x65, 2(%rdi)     ; 'e'
movw $0x6C, 4(%rdi)     ; 'l'
movw $0x6C, 6(%rdi)     ; 'l'
movw $0x6F, 8(%rdi)     ; 'o'
```

### 2. IEEE-754 Bit Pattern Embedding

Floating-point values are converted to their IEEE-754 bit representation at compile-time:

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

**Usage:**
```cpp
auto pi = 3.14159_embed;  // IEEE-754 as immediate value
```

**Assembly Output:**
```asm
movabsq $0x400921f9f01b866e, %rax  ; Pi as 64-bit immediate
```

### 3. Pure Integer-Based Type Conversions

All type conversions use bitwise operations, eliminating compiler-generated conversion constants:

```cpp
// Extracts integer value from IEEE-754 without FPU instructions
INT64 operator(INT64)(const DOUBLE& d) {
    UINT64 bits = d.bits;
    int exponent = ((bits >> 52) & 0x7FF) - 1023;
    UINT64 mantissa = (bits & 0xFFFFFFFFFFFFF) | 0x10000000000000;
    // ... bit shifting magic ...
}
```

## Building

### VSCode Integration (Recommended)

The project includes comprehensive VSCode integration:

**Quick Actions:**
- `Ctrl+Shift+B` - Build all configurations
- `F5` - Run/Debug with selected configuration

**Available Build Tasks:**
- `i386-pc-windows-gnu-debug/release`
- `x86_64-pc-windows-gnu-debug/release`
- `armv7a-pc-windows-gnu-debug/release`
- `aarch64-pc-windows-gnu-debug/release`
- `Build All` - Builds all release configurations in parallel

### Build Outputs

After building, artifacts are placed in `build/windows/<arch>/<type>/`:

| File | Description |
|------|-------------|
| `output.exe` | Main executable |
| `output.bin` | Extracted `.text` section (PIC blob) |
| `output.b64.txt` | Base64-encoded PIC blob |
| `output.txt` | Disassembly and section dump |
| `output.strings.txt` | Extracted strings (should be minimal) |
| `output.map.txt` | Linker map file |

### Optimization Levels

**Default Settings:**
- **DEBUG builds** use `-Og` (optimized for debugging experience)
- **RELEASE builds** use `-O3` (maximum performance optimization)

**Features:**
- Debug builds include CodeView symbols for WinDbg/Visual Studio debugging
- Release builds include LTO (Link-Time Optimization), aggressive inlining, and loop unrolling
- No string literals or floating-point constants in `.rdata` at any optimization level

## Windows Implementation

### Direct Syscalls

CPP-PIC bypasses the standard Windows API and uses direct syscalls:

**ntdll.dll:**
- `NtAllocateVirtualMemory` - Allocate memory pages
- `NtFreeVirtualMemory` - Free memory pages
- `NtTerminateProcess` - Exit process

**kernel32.dll:**
- `GetStdHandle` - Get console handles
- `WriteConsoleW` - Write wide strings to console

### PEB Walking

The Process Environment Block (PEB) is accessed to locate loaded modules:

```cpp
// Locate PEB (x64)
PEB* peb = GetPEB();

// Walk loaded modules
PEB_LDR_DATA* ldr = peb->Ldr;
LIST_ENTRY* moduleList = &ldr->InMemoryOrderModuleList;

// Enumerate modules: ntdll.dll, kernel32.dll, etc.
```

### PE Parsing

Export tables are parsed to resolve API functions by hash:

```cpp
// Find export by DJB2 hash
FARPROC GetExportByHash(HMODULE module, UINT32 hash);

// Example: Resolve WriteConsoleW
auto writeConsole = GetExportByHash(kernel32, 0x7B8F69D2);
```

**Benefits:**
- No import table
- No GetProcAddress calls
- Hash-based lookups prevent string analysis
- Position-independent

### File System Operations

Complete file and directory operations via NTAPI:

```cpp
// File operations
File file;
file.Open(path, FileMode::Read);
file.Read(buffer, size, &bytesRead);
file.Write(data, size, &bytesWritten);
file.Close();

// Directory operations
Directory dir;
dir.Create(path);
dir.Exists(path);
dir.Delete(path);
dir.Enumerate(path, callback);
```

**Supported Operations:**
- File create, read, write, delete
- Directory create, enumerate, delete
- File attributes and metadata
- Path manipulation

### Console Output

Printf-style formatting without CRT:

```cpp
Console::WriteFormatted<WCHAR>(L"Integer: %d\n"_embed, 42);
Console::WriteFormatted<WCHAR>(L"Float: %.5f\n"_embed, 3.14159_embed);
Console::WriteFormatted<WCHAR>(L"Hex: 0x%X\n"_embed, 255);
```

**Format Specifiers:**
- `%d` - Signed decimal integer
- `%u` - Unsigned decimal integer
- `%X` - Uppercase hexadecimal
- `%f`, `%.Nf` - Floating-point with precision
- `%s`, `%ls` - Narrow/wide strings
- `%p` - Pointer

### Linker Configuration

Critical flags for position-independence:

```cmake
# Merge .rdata into .text (CRITICAL for PIC)
/MERGE:.rdata=.text

# Custom entry point (no CRT)
/Entry:_start

# Function ordering
/ORDER:@orderfile.txt

# Release optimizations
/OPT:REF        # Remove unreferenced code
/OPT:ICF        # Fold identical code
/LTCG           # Link-time code generation
```

## Cryptography

CPP-PIC includes a complete cryptographic suite implemented without any external dependencies:

### Hashing

```cpp
// SHA-256
UINT8 hash[32];
Sha256::Hash(data, dataLen, hash);

// SHA-512
UINT8 hash[64];
Sha512::Hash(data, dataLen, hash);

// HMAC variants
Sha256::Hmac(key, keyLen, data, dataLen, mac);
```

### Symmetric Encryption

```cpp
// ChaCha20 stream cipher
ChaCha20 cipher(key, nonce);
cipher.Encrypt(plaintext, ciphertext, length);
```

### Elliptic Curve Cryptography

```cpp
// ECC key generation and operations
Ecc::GenerateKeyPair(privateKey, publicKey);
Ecc::Sign(privateKey, message, signature);
Ecc::Verify(publicKey, message, signature);
```

### Encoding

```cpp
// Base64 encoding/decoding
Base64::Encode(input, inputLen, output, &outputLen);
Base64::Decode(input, inputLen, output, &outputLen);
```

## Networking

Full networking stack built on Windows sockets:

### DNS Resolution

```cpp
// Resolve domain to IP address
Dns dns;
auto ip = dns.Resolve("example.com"_embed);
```

### HTTP Client

```cpp
// HTTP requests
Http http;
http.Connect(host, port);
http.Get("/api/endpoint"_embed, response);
```

### WebSocket

```cpp
// WebSocket connections
WebSocket ws;
ws.Connect(host, port, path);
ws.Send(message, length);
ws.Receive(buffer, &received);
```

### TLS 1.3

```cpp
// Secure connections with certificate verification
Tls tls;
tls.Connect(host, port);
tls.Send(data, length);
tls.Receive(buffer, &received);
```

**TLS Features:**
- TLS 1.3 only (RFC 8446)
- Certificate chain verification
- ChaCha20-Poly1305 cipher suite
- ECDHE key exchange (secp256r1, secp384r1)

## Logging

The Logger class provides structured logging with multiple output targets:

```cpp
// Initialize logger with console and file output
Logger logger;
logger.SetLevel(LogLevel::Debug);
logger.SetFileOutput("app.log"_embed);

// Log messages at different levels
logger.Debug("Debug message: %d"_embed, value);
logger.Info("Information message"_embed);
logger.Warning("Warning: %s"_embed, message);
logger.Error("Error occurred: 0x%X"_embed, errorCode);
```

**Features:**
- **Log Levels**: Debug, Info, Warning, Error
- **ANSI Colors**: Colored output for different log levels (console)
- **Timestamps**: Automatic timestamp prefixing
- **File Output**: Write logs to file with automatic flushing
- **Format Specifiers**: Full printf-style formatting support

## Compiler & Linker Flags

### PIC-Critical Flags

| Flag | Purpose |
|------|---------|
| `-fno-jump-tables` | Prevents switch statement jump tables in .rdata |
| `-fno-rtti` | Disables runtime type information |
| `-fno-exceptions` | Disables C++ exceptions |
| `/MERGE:.rdata=.text` | Merges read-only data into code section |

### Base Compiler Flags

| Flag | Purpose |
|------|---------|
| `-std=c++23` | Enable C++23 standard features |
| `-nostdlib` | No standard C/C++ libraries |
| `-fno-builtin` | Disable compiler built-ins |
| `-fshort-wchar` | Use 2-byte wchar_t (Windows ABI) |
| `-msoft-float` | Software floating-point (x86 only) |

## Project Structure

The codebase follows a **three-layer architecture** for clean separation of concerns:

```
cpp-pic/
├── include/
│   ├── bal/                       # Base Abstraction Layer (platform-independent)
│   │   ├── primitives/            # EMBEDDED_STRING, UINT64, INT64, DOUBLE
│   │   ├── djb2.h                 # Hash function for symbol lookup
│   │   ├── memory.h               # Copy, Zero, Compare operations
│   │   ├── string.h               # String manipulation utilities
│   │   ├── string_formatter.h     # Printf-style formatting
│   │   └── bal.h                  # Master header
│   ├── pal/                       # Platform Abstraction Layer (OS/hardware)
│   │   ├── windows/               # Windows-specific types and APIs
│   │   ├── allocator.h            # Memory allocation interface
│   │   ├── console.h              # Console I/O interface
│   │   ├── date_time.h            # Date/time operations
│   │   ├── file_system.h          # File and directory operations
│   │   ├── socket.h               # Network socket interface
│   │   └── pal.h                  # Master header
│   └── ral/                       # Runtime Abstraction Layer (application features)
│       ├── logger.h               # Logging with file output, ANSI colors
│       ├── crypt/                 # Cryptography (SHA2, ChaCha20, ECC, Base64)
│       ├── network/               # Networking (DNS, HTTP, WebSocket, TLS)
│       └── ral.h                  # Master header
├── src/
│   ├── start.cc                   # Entry point (_start)
│   ├── bal/                       # BAL implementations
│   │   └── string.cc
│   └── pal/windows/               # Windows PAL implementations
│       ├── platform.windows.cc    # PEB walking, API resolution
│       ├── allocator.windows.cc   # NtAllocateVirtualMemory
│       ├── console.windows.cc     # WriteConsoleW
│       ├── socket.windows.cc      # Windows sockets
│       ├── file_system.cc         # File/directory operations
│       ├── date_time.windows.cc   # Windows time functions
│       ├── peb.cc                 # Process Environment Block
│       ├── pe.cc                  # PE parsing for exports
│       ├── ntdll.cc               # ntdll.dll API resolution
│       ├── kernel32.cc            # kernel32.dll API resolution
│       └── random/windows/        # Cryptographic RNG
├── tests/                         # Test suite (13+ categories)
├── docs/                          # Architecture and platform guides
├── scripts/                       # Automation scripts (PowerShell loader)
├── build/windows/                 # Build artifacts (generated)
├── .vscode/                       # VSCode integration
├── .github/workflows/             # CI/CD pipeline
├── CMakeLists.txt
└── README.md
```

### Three-Layer Architecture

| Layer | Purpose | Examples |
|-------|---------|----------|
| **BAL** (Base Abstraction) | Platform-independent primitives and utilities | EMBEDDED_STRING, UINT64, INT64, DOUBLE, DJB2, Memory, StringFormatter |
| **PAL** (Platform Abstraction) | OS/hardware abstraction interfaces | Allocator, Console, Socket, FileSystem, DateTime |
| **RAL** (Runtime Abstraction) | High-level application features | Logger, Cryptography (SHA, ChaCha20, ECC), Networking (DNS, HTTP, TLS) |

## Use Cases

| Domain | Examples |
|--------|----------|
| **Security Research** | Shellcode, code injection, exploit development |
| **Embedded Systems** | No CRT, minimal dependencies, bare metal |
| **Kernel Development** | Windows kernel modules, drivers |
| **Binary Analysis** | Understanding compiler behavior, reverse engineering |
| **Education** | Compiler construction, OS development |

### Why CPP-PIC?

- **Security**: Position-independent payloads without relocation handling
- **Embedded**: Modern C++23 in resource-constrained environments
- **Kernel**: C++23 features without runtime initialization
- **Research**: Explore compiler behavior and binary structure

## Testing

### Running Tests

Tests run automatically when executing the built binary:

```powershell
# Run x64 release build
.\build\windows\x86_64\release\output.exe

# Run i386 debug build
.\build\windows\i386\debug\output.exe
```

**Expected Output:**
```
CPP-PIC Runtime Starting...
Running DJB2 Tests... PASSED
Running Memory Tests... PASSED
Running String Tests... PASSED
Running UINT64 Tests... PASSED
Running INT64 Tests... PASSED
Running Double Tests... PASSED
Running ArrayStorage Tests... PASSED
Running StringFormatter Tests... PASSED
Running Random Tests... PASSED
Running SHA Tests... PASSED
Running ECC Tests... PASSED
Running Socket Tests... PASSED
Running TLS Tests... PASSED
Running DNS Tests... PASSED
All tests passed!
```

### Test Coverage

The test suite validates all major components:

| Test Suite | Purpose |
|------------|---------|
| **Djb2Tests** | Hash function consistency |
| **MemoryTests** | Copy, Zero, Compare, Fill operations |
| **StringTests** | String manipulation functions |
| **Uint64Tests** | 64-bit unsigned arithmetic, shifts, comparisons |
| **Int64Tests** | 64-bit signed arithmetic with sign handling |
| **DoubleTests** | IEEE-754 floating-point operations and conversions |
| **ArrayStorageTests** | Compile-time template array storage |
| **StringFormatterTests** | Printf-style format specifiers (%d, %f, %s, %x, etc.) |
| **RandomTests** | Cryptographic RNG (Windows entropy source) |
| **ShaTests** | SHA-256/512 hashing with HMAC variants |
| **EccTests** | Elliptic curve cryptography operations |
| **SocketTests** | TCP socket connectivity |
| **TlsTests** | TLS 1.3 handshake, encryption, certificate verification |
| **DnsTests** | Domain name resolution |

### CI/CD Testing

GitHub Actions automatically tests all configurations:
- Builds: i386, x86_64, aarch64 (Windows)
- Tests: i386 (WoW64), x86_64 (native), aarch64 (self-hosted ARM64 runner)
- Validation: No .rdata dependencies, correct exit codes

## Binary Analysis

### Verifying Position Independence

```powershell
# Check .rdata section (should be minimal)
llvm-objdump -h build\windows\x86_64\release\output.exe | findstr .rdata

# Extract strings (should not contain your embedded strings)
llvm-strings build\windows\x86_64\release\output.exe

# View disassembly showing immediate values
llvm-objdump -d build\windows\x86_64\release\output.exe | findstr "mov.*\$0x"
```

**Expected Results:**
- `.rdata`: Only ~32 bytes (compiler constants)
- Strings: None of your embedded strings visible
- Disassembly: Characters as immediate operands (`movw $0x57, (%rax)`)

## Documentation

For more detailed information:

- **[Architecture Guide](docs/architecture.md)** - System design and components
- **[Platform Guide](docs/platform_guide.md)** - Windows implementation details
- **[Project Structure](STRUCTURE.md)** - Project organization
- **[Scripts Documentation](scripts/README.md)** - Automation scripts

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

---

**CPP-PIC** - Pushing the boundaries of position-independent C++23 on Windows.
