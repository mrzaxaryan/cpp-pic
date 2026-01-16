# CPP-PIC Project Structure

This document provides an overview of the reorganized project structure.

## Directory Layout

```
cpp-pic-private/
├── include/                         # Public API headers
│   └── runtime/
│       ├── platform/               # Platform abstraction layer
│       │   ├── primitives/        # Core types (EMBEDDED_STRING, UINT64, etc.)
│       │   ├── windows/           # Windows-specific headers
│       │   ├── linux/             # Linux-specific headers
│       │   ├── uefi/              # UEFI-specific headers
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
│       │   ├── linux/             # Linux platform code
│       │   │   ├── platform.linux.cc
│       │   │   ├── platform.linux.i386.cc
│       │   │   ├── platform.linux.x86_64.cc
│       │   │   ├── platform.linux.armv7a.cc
│       │   │   ├── platform.linux.aarch64.cc
│       │   │   ├── allocator.linux.cc
│       │   │   └── syscall.cc
│       │   ├── uefi/              # UEFI platform code
│       │   │   ├── platform.uefi.cc
│       │   │   └── allocator.uefi.cc
│       │   ├── allocator.cc       # Generic allocator
│       │   └── platform.cc        # Generic platform
│       ├── console/               # Console implementations
│       │   ├── console.cc         # Generic console
│       │   ├── windows/
│       │   │   └── console.windows.cc
│       │   ├── linux/
│       │   │   └── console.linux.cc
│       │   └── uefi/
│       │       └── console.uefi.cc
│       └── start.cc               # Entry point
│
├── build/                          # Build artifacts (generated)
│   ├── windows/
│   │   └── <arch>/
│   │       ├── debug/
│   │       │   ├── cmake/         # CMake build files
│   │       │   ├── output.exe     # Executable
│   │       │   ├── output.bin     # PIC blob
│   │       │   ├── output.b64.txt # Base64 PIC blob
│   │       │   ├── output.txt     # Disassembly
│   │       │   └── output.map.txt # Linker map
│   │       └── release/
│   ├── linux/
│   │   └── <arch>/
│   │       ├── debug/
│   │       └── release/
│   └── uefi/
│       └── <arch>/
│           ├── debug/
│           └── release/
│
├── cmake/                          # Build system modules
│   ├── toolchain-clang.cmake      # Clang/LLVM toolchain
│   ├── base64_encode.cmake        # Base64 encoding script
│   └── verify_no_rdata.cmake      # .rdata validation script
│
├── scripts/                        # 🆕 Automation scripts
│   ├── README.md                  # Scripts documentation
│   ├── install.sh                 # Linux/macOS dependency installer
│   ├── run-uefi-qemu.sh           # UEFI testing (Linux/macOS)
│   ├── run-uefi-qemu.ps1          # UEFI testing (Windows)
│   └── loader.ps1                 # PIC blob loader (Windows)
│
├── docs/                           # 🆕 Documentation
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
│   ├── tasks.json                 # Build tasks (20+ configs)
│   ├── c_cpp_properties.json      # IntelliSense config
│   └── settings.json              # Workspace settings
│
├── CMakeLists.txt                  # Root build configuration
├── linker.script                   # Linux linker script
├── orderfile.txt                   # Windows function ordering
├── LICENSE                         # Proprietary license
├── README.md                       # Main documentation
└── STRUCTURE.md                    # This file

```

## What Changed (Step 2 Reorganization)

### 🆕 New Directories

1. **scripts/** - Centralized automation scripts
   - Moved: `install.sh`, `run-uefi-qemu.sh`, `run-uefi-qemu.ps1`, `loader.ps1`
   - Added: `scripts/README.md` for documentation

2. **docs/** - Architecture and platform documentation
   - Added: `architecture.md` - Comprehensive architecture guide
   - Added: `platform_guide.md` - Platform-specific implementation details

3. **tests/** - Test organization
   - Added: `tests/README.md` - Test documentation
   - Added: `tests/unit/` - Placeholder for future unit test sources

### 📝 Updated Files

1. **README.md**
   - Updated all script paths to use `scripts/` directory
   - Added "Documentation" section with links to new docs
   - Updated project structure diagram

2. **.vscode/launch.json**
   - Updated all `loader.ps1` references to `scripts/loader.ps1`
   - Updated all `run-uefi-qemu.ps1` references to `scripts/run-uefi-qemu.ps1`

3. **New Documentation**
   - Created comprehensive architecture documentation
   - Created platform implementation guide
   - Created scripts usage guide
   - Created tests documentation

## Benefits of New Structure

### 📂 Better Organization

- **Cleaner Root** - Automation scripts moved to dedicated directory
- **Discoverable Docs** - Architecture and platform guides easy to find
- **Logical Grouping** - Related files organized together

### 📚 Improved Documentation

- **Architecture Guide** - Deep dive into system design and components
- **Platform Guide** - Detailed platform-specific implementation details
- **Scripts Guide** - Clear usage examples and troubleshooting

### 🔧 Easier Maintenance

- **Centralized Scripts** - All automation in one place
- **Versioned Docs** - Documentation alongside code
- **Test Structure** - Ready for test expansion

### 👥 Better Developer Experience

- **Quick Onboarding** - New developers can find documentation easily
- **Clear Patterns** - Consistent organization across project
- **Future-Proof** - Structure supports growth

## File Counts

| Category | Count | Location |
|----------|-------|----------|
| **Header files** | 26 | `include/runtime/` |
| **Test headers** | 8 | `tests/` |
| **Source files** | 21 | `src/runtime/` |
| **CMake scripts** | 3 | `cmake/` |
| **Automation scripts** | 4 | `scripts/` |
| **Documentation** | 5 | `docs/`, `scripts/`, `tests/` |
| **VSCode configs** | 4 | `.vscode/` |

## Quick Navigation

### For Developers

- Start here: [README.md](README.md)
- Understand architecture: [docs/architecture.md](docs/architecture.md)
- Platform details: [docs/platform_guide.md](docs/platform_guide.md)
- Run scripts: [scripts/README.md](scripts/README.md)

### For Contributors

- Project structure: This file
- Build system: [CMakeLists.txt](CMakeLists.txt)
- Test suite: [tests/README.md](tests/README.md)
- VSCode setup: [.vscode/README.md](.vscode/README.md)

## Future Enhancements

Potential future directory additions:

```
cpp-pic-private/
├── examples/              # Usage examples
│   ├── basic/
│   ├── advanced/
│   └── README.md
├── benchmarks/            # Performance benchmarks
│   └── README.md
├── libs/                  # Modular libraries (if needed)
│   ├── core/
│   ├── platform/
│   └── primitives/
└── tools/                 # Development tools
    ├── analyzers/
    └── README.md
```

## References

- [Main README](README.md) - Getting started and build instructions
- [Architecture Documentation](docs/architecture.md) - System design and components
- [Platform Guide](docs/platform_guide.md) - Platform-specific implementations
- [Scripts Guide](scripts/README.md) - Automation script reference
- [Tests Guide](tests/README.md) - Testing documentation
