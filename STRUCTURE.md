# CPP-PIC Project Structure

This document provides an overview of the project organization.

## Directory Layout

```
cpp-pic/
├── include/                         # Public API headers
│   └── runtime/
│       ├── platform/               # Platform abstraction layer
│       │   ├── primitives/        # Core types (EMBEDDED_STRING, UINT64, etc.)
│       │   ├── windows/           # Windows-specific headers
│       │   ├── allocator.h        # Memory allocation interface
│       │   └── platform.h         # Platform initialization
│       ├── console.h              # Console I/O abstraction
│       ├── logger.h               # Logging utilities
│       ├── memory.h               # Memory operations
│       ├── string.h               # String utilities
│       ├── string_formatter.h     # Printf-style formatting
│       ├── djb2.h                 # Hash function
│       └── runtime.h              # Master runtime header
│
├── src/                            # Implementation files
│   └── runtime/
│       ├── platform/              # Platform implementations
│       │   ├── windows/           # Windows platform code
│       │   │   ├── platform.windows.cc
│       │   │   ├── allocator.windows.cc
│       │   │   ├── kernel32.cc
│       │   │   ├── ntdll.cc
│       │   │   ├── peb.cc
│       │   │   └── pe.cc
│       │   ├── allocator.cc       # Generic allocator
│       │   └── platform.cc        # Generic platform
│       ├── console/               # Console implementations
│       │   ├── console.cc         # Generic console
│       │   └── windows/
│       │       └── console.windows.cc
│       └── start.cc               # Entry point
│
├── build/                          # Build artifacts (generated)
│   └── windows/
│       └── <arch>/
│           ├── debug/
│           │   ├── cmake/         # CMake build files
│           │   ├── output.exe     # Executable
│           │   ├── output.bin     # PIC blob
│           │   ├── output.b64.txt # Base64 PIC blob
│           │   ├── output.txt     # Disassembly
│           │   └── output.map.txt # Linker map
│           └── release/
│
├── cmake/                          # Build system modules
│   ├── base64_encode.cmake        # Base64 encoding script
│   └── verify_no_rdata.cmake      # .rdata validation script
│
├── scripts/                        # Automation scripts
│   ├── README.md                  # Scripts documentation
│   └── loader.ps1                 # PIC blob loader (Windows)
│
├── docs/                           # Documentation
│   ├── architecture.md            # Architecture overview
│   └── platform_guide.md          # Platform implementation guide
│
├── tests/                          # Test suite headers
│   ├── tests.h                    # Master test header
│   ├── uint64_tests.h             # Unsigned 64-bit integer tests
│   ├── int64_tests.h              # Signed 64-bit integer tests
│   ├── double_tests.h             # Floating-point tests
│   ├── string_tests.h             # String utility tests
│   ├── string_formatter_tests.h   # Printf-style formatting tests
│   ├── djb2_tests.h               # Hash function tests
│   ├── memory_tests.h             # Memory operations tests
│   └── README.md                  # Test documentation
│
├── .vscode/                        # VSCode integration
│   ├── launch.json                # Debug/run configurations
│   ├── tasks.json                 # Build tasks
│   ├── c_cpp_properties.json      # IntelliSense config
│   └── settings.json              # Workspace settings
│
├── .github/                        # GitHub Actions CI/CD
│   └── workflows/
│       └── build.yml              # Build and test workflow
│
├── CMakeLists.txt                  # Root build configuration
├── orderfile.txt                   # Windows function ordering
├── LICENSE                         # Proprietary license
├── README.md                       # Main documentation
└── STRUCTURE.md                    # This file

```

## Key Components

### Headers (`include/runtime/`)

**Platform Abstraction:**
- `platform/platform.h` - Platform initialization
- `platform/allocator.h` - Memory allocation interface
- `platform/windows/` - Windows-specific types and APIs

**Primitives (`platform/primitives/`):**
- `embedded_string.h` - Compile-time string embedding
- `embedded_double.h` - IEEE-754 double embedding
- `uint64.h` / `int64.h` - Software 64-bit integers
- `double.h` - IEEE-754 operations

**Utilities:**
- `console.h` - Console I/O abstraction
- `logger.h` - Logging framework
- `memory.h` - Memory operations (Copy, Zero, Compare)
- `string.h` - String manipulation
- `string_formatter.h` - Printf-style formatting
- `djb2.h` - DJB2 hash function

### Source Files (`src/runtime/`)

**Entry Point:**
- `start.cc` - Main entry point (`_start`)

**Windows Platform (`platform/windows/`):**
- `platform.windows.cc` - Platform initialization
- `allocator.windows.cc` - Memory allocation (NtAllocateVirtualMemory)
- `peb.cc` - Process Environment Block walking
- `pe.cc` - PE file parsing
- `ntdll.cc` - ntdll.dll API resolution
- `kernel32.cc` - kernel32.dll API resolution

**Console (`console/windows/`):**
- `console.windows.cc` - Windows console implementation (WriteConsoleW)

### Build System (`cmake/`)

- `base64_encode.cmake` - Base64 encoding for PIC blobs
- `verify_no_rdata.cmake` - Post-build .rdata section validation

### Scripts (`scripts/`)

- `loader.ps1` - Load and execute PIC blobs in memory (Windows)

### Tests (`tests/`)

All tests are header-based and run from `src/start.cc`:

- `uint64_tests.h` - UINT64 arithmetic tests
- `int64_tests.h` - INT64 arithmetic tests
- `double_tests.h` - Floating-point tests
- `string_tests.h` - String operations
- `string_formatter_tests.h` - Printf formatting
- `djb2_tests.h` - Hash function tests
- `memory_tests.h` - Memory operations

### VSCode Integration (`.vscode/`)

- `tasks.json` - Build tasks for all Windows architectures
- `launch.json` - Debug configurations
- `c_cpp_properties.json` - IntelliSense settings
- `settings.json` - Workspace configuration

## Build Output Structure

```
build/
└── windows/
    ├── i386/
    │   ├── debug/
    │   │   ├── output.exe           # Main executable
    │   │   ├── output.bin           # Extracted .text section
    │   │   ├── output.txt           # Disassembly
    │   │   ├── output.strings.txt   # Extracted strings
    │   │   ├── output.b64.txt       # Base64-encoded blob
    │   │   ├── output.map.txt       # Linker map
    │   │   └── cmake/               # CMake build files
    │   └── release/
    ├── x86_64/
    ├── armv7a/
    └── aarch64/
```

## File Counts

| Category | Count | Location |
|----------|-------|----------|
| **Header files** | 20 | `include/runtime/` |
| **Test headers** | 8 | `tests/` |
| **Source files** | 10 | `src/runtime/` (Windows only) |
| **CMake scripts** | 3 | `cmake/` |
| **Automation scripts** | 1 | `scripts/` |
| **Documentation** | 4 | `docs/`, `scripts/`, `tests/` |
| **VSCode configs** | 4 | `.vscode/` |

## Quick Navigation

### For Developers

- Start here: [README.md](README.md)
- Understand architecture: [docs/architecture.md](docs/architecture.md)
- Windows implementation: [docs/platform_guide.md](docs/platform_guide.md)
- Run scripts: [scripts/README.md](scripts/README.md)

### For Contributors

- Project structure: This file
- Build system: [CMakeLists.txt](CMakeLists.txt)
- Test suite: [tests/README.md](tests/README.md)

## Development Workflow

```
┌─────────────────────────────────────────────────────────────────┐
│                      CPP-PIC Development                        │
└─────────────────────────────────────────────────────────────────┘

VSCode Integration:
┌──────────────┐  Press F5   ┌──────────────┐  Auto-run  ┌─────────────┐
│   Edit Code  │ ────────────▶│  Build Task  │ ──────────▶│  Run/Debug  │
│   (C++23)    │              │  (CMake +    │            │  (Native)   │
│              │              │   Ninja)     │            │             │
└──────────────┘              └──────────────┘            └─────────────┘
                                     │
                                     ▼
                              ┌─────────────┐
                              │  Windows:   │
                              │  • i386     │
                              │  • x86_64   │
                              │  • armv7a   │
                              │  • aarch64  │
                              └─────────────┘
```

## References

- [Main README](README.md) - Getting started and build instructions
- [Architecture Documentation](docs/architecture.md) - System design and components
- [Platform Guide](docs/platform_guide.md) - Windows implementation details
- [Scripts Guide](scripts/README.md) - Automation script reference
- [Tests Guide](tests/README.md) - Testing documentation
