# Toolchain Installation Guide

This guide provides step-by-step instructions for installing the required build toolchain (LLVM/Clang, CMake, Ninja) for NOSTDLIB-RUNTIME on Linux and Windows.

## Linux Installation

### Ubuntu/Debian

Copy and paste the following command to install all dependencies:

```bash
# Install all dependencies (LLVM 20)
LLVM_VER=20 && sudo apt-get update && sudo apt-get install -y wget lsb-release ca-certificates gnupg cmake ninja-build && sudo mkdir -p /etc/apt/keyrings && wget -qO- https://apt.llvm.org/llvm-snapshot.gpg.key | gpg --dearmor | sudo tee /etc/apt/keyrings/apt.llvm.org.gpg >/dev/null && echo "deb [signed-by=/etc/apt/keyrings/apt.llvm.org.gpg] http://apt.llvm.org/$(lsb_release -cs)/ llvm-toolchain-$(lsb_release -cs)-${LLVM_VER} main" | sudo tee /etc/apt/sources.list.d/llvm.list && sudo apt-get update && sudo apt-get install -y clang-${LLVM_VER} clang++-${LLVM_VER} lld-${LLVM_VER} llvm-${LLVM_VER} lldb-${LLVM_VER} && sudo update-alternatives --install /usr/bin/clang clang /usr/bin/clang-${LLVM_VER} 100 && sudo update-alternatives --install /usr/bin/clang++ clang++ /usr/bin/clang++-${LLVM_VER} 100 && sudo update-alternatives --install /usr/bin/lld lld /usr/bin/lld-${LLVM_VER} 100
```

**Note:** To install a different LLVM version, change `LLVM_VER=20` to your desired version (e.g., `LLVM_VER=21`).

### Verify Installation

```bash
clang --version
clang++ --version
lld --version
cmake --version
ninja --version
```

## Windows Installation

### Prerequisites

Ensure [winget](https://github.com/microsoft/winget-cli) is installed (pre-installed on Windows 11 and recent Windows 10).

### Installation

Copy and paste the following command into PowerShell or Command Prompt:

```powershell
# Install all dependencies
winget install --id Kitware.CMake && winget install --id Ninja-build.Ninja && winget install --id LLVM.LLVM
```

**Important:** After installation, restart your terminal or log out and log back in for the PATH changes to take effect.

### Verify Installation

After restarting your terminal:

```powershell
clang --version
clang++ --version
lld-link --version
cmake --version
ninja --version
```

## Version Requirements

- **CMake**: 3.20 or higher
- **Ninja**: 1.10 or higher
- **LLVM/Clang**: 20 or higher
## Next Steps

After installing the toolchain, refer to the main [README.md](README.md) for build instructions and usage examples.
