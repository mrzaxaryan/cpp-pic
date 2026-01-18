# Scripts Directory

Automation scripts for building, testing, and deploying CPP-PIC on Windows.

## Scripts

### Deployment Scripts

#### [loader.ps1](loader.ps1)
Loads and executes PIC blob in memory (Windows).

```powershell
# Load base64-encoded PIC blob
.\scripts\loader.ps1 -BlobPath .\build\windows\x86_64\debug\output.b64.txt

# Load raw binary blob
.\scripts\loader.ps1 -BlobPath .\build\windows\x86_64\debug\output.bin -IsRaw
```

**Parameters:**
- `-BlobPath` - Path to PIC blob file (base64 or raw)
- `-IsRaw` - Switch to indicate raw binary (default: base64)

**What it does:**
1. Reads PIC blob file
2. Decodes base64 (if applicable)
3. Allocates executable memory (VirtualAlloc with PAGE_EXECUTE_READWRITE)
4. Copies blob to memory
5. Creates delegate and executes
6. Reports exit code

---

## Usage Examples

### Complete Build & Test Workflow

```powershell
# 1. Build Windows x64 debug
cmake -B build/windows/x86_64/debug/cmake -G Ninja \
  -DCMAKE_TOOLCHAIN_FILE=cmake/toolchain-clang.cmake \
  -DARCHITECTURE=x86_64 -DPLATFORM=windows -DBUILD_TYPE=debug
cmake --build build/windows/x86_64/debug/cmake

# 2. Run tests
.\build\windows\x86_64\debug\output.exe

# 3. Load PIC blob from memory
.\scripts\loader.ps1 -BlobPath .\build\windows\x86_64\debug\output.b64.txt
```

### Testing Multiple Architectures

```powershell
# Build all architectures
foreach ($arch in @('i386', 'x86_64', 'armv7a', 'aarch64')) {
    cmake -B "build/windows/$arch/release/cmake" -G Ninja `
      -DCMAKE_TOOLCHAIN_FILE=cmake/toolchain-clang.cmake `
      -DARCHITECTURE=$arch -DPLATFORM=windows -DBUILD_TYPE=release
    cmake --build "build/windows/$arch/release/cmake"
}

# Run native tests (i386 via WoW64, x86_64 native)
.\build\windows\i386\release\output.exe
.\build\windows\x86_64\release\output.exe
```

---

## Script Dependencies

### Windows Scripts (PowerShell)

**Required:**
- PowerShell 5.1+ (Windows 10+)
- LLVM/Clang 20+ (in PATH)
- CMake 3.20+
- Ninja

**Installation:**

1. **LLVM/Clang 20+**
   - Download from [LLVM Releases](https://github.com/llvm/llvm-project/releases)
   - Install `LLVM-20.1.6-win64.exe`
   - Add to PATH during installation

2. **CMake**
   - Download from [cmake.org](https://cmake.org/download/)
   - Or install via: `winget install Kitware.CMake`

3. **Ninja**
   - Install via: `winget install -e --id Ninja-build.Ninja`

**Verify Installation:**

```powershell
clang --version   # Should show 20.x.x
cmake --version   # Should show 3.20+
ninja --version
```

---

## Adding New Scripts

### Naming Conventions

- **Windows**: `.ps1` extension (PowerShell)
- Use descriptive names (e.g., `analyze-binary.ps1`, `benchmark-runner.ps1`)

### Script Template

**PowerShell** (`.ps1`):
```powershell
#!/usr/bin/env pwsh
# Script description

param(
    [Parameter(Mandatory=$true)]
    [string]$RequiredParam,

    [string]$OptionalParam = "default"
)

# Script logic
Write-Host "Running script..."
```

### Best Practices

1. **Add error handling** - Check for required tools
   ```powershell
   if (!(Get-Command clang -ErrorAction SilentlyContinue)) {
       Write-Error "Clang not found. Install LLVM first."
       exit 1
   }
   ```

2. **Provide usage info** - Show help on `-h` or `--help`
   ```powershell
   if ($h -or $help) {
       Write-Host "Usage: script.ps1 -RequiredParam <value>"
       exit 0
   }
   ```

3. **Use absolute paths** - Avoid issues with working directory
   ```powershell
   $ScriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
   $ProjectRoot = Split-Path -Parent $ScriptDir
   ```

4. **Document parameters** - Clear descriptions in param block
5. **Test on target platforms** - Verify scripts work as expected

---

## Troubleshooting

### Loader Script

**Problem**: "Access denied"
- **Solution**: Run PowerShell as Administrator
- **Reason**: VirtualAlloc with PAGE_EXECUTE_READWRITE requires elevated privileges

**Problem**: "Invalid blob format"
- **Solution**: Verify blob file is valid base64 or raw binary
- **Debug**: Check `output.b64.txt` is generated correctly during build

**Problem**: "Execution policy restriction"
- **Solution**: Allow script execution:
  ```powershell
  Set-ExecutionPolicy -ExecutionPolicy RemoteSigned -Scope CurrentUser
  ```

### Build Scripts

**Problem**: "Clang not found"
- **Solution**: Install LLVM 20 and add to PATH
- **Download**: [LLVM GitHub Releases](https://github.com/llvm/llvm-project/releases)

**Problem**: "CMake not found"
- **Solution**: Install CMake and add to PATH
- **Download**: [cmake.org](https://cmake.org/download/)

**Problem**: "Ninja not found"
- **Solution**: Install Ninja via winget:
  ```powershell
  winget install -e --id Ninja-build.Ninja
  ```

---

## Security Considerations

### PIC Blob Loader

The `loader.ps1` script allocates executable memory and runs arbitrary code. This is intended for:
- **Authorized security research**
- **Testing position-independent code**
- **Educational purposes**

**Never load untrusted PIC blobs!**

The script:
- Allocates memory with `PAGE_EXECUTE_READWRITE`
- Copies arbitrary binary code
- Executes code directly

Only use with PIC blobs you have built yourself from trusted source code.

---

## References

- [Build Guide](../README.md) - Main build instructions
- [Architecture Guide](../docs/architecture.md) - System architecture
- [Platform Guide](../docs/platform_guide.md) - Windows implementation details
- [Tests Guide](../tests/README.md) - Testing documentation
