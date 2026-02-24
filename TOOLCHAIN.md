# Toolchain Installation Guide

This guide provides step-by-step instructions for installing the required build toolchain (LLVM/Clang, CMake, Ninja) for Position-Independent Runtime (PIR) on Windows, Linux, and macOS.

## Windows Installation

### Prerequisites

Ensure [winget](https://github.com/microsoft/winget-cli) is installed (pre-installed on Windows 11 and recent Windows 10).

### Installation

Copy and paste the following command into PowerShell or Command Prompt:

```powershell
# Install all dependencies
winget install --id Kitware.CMake && winget install --id Ninja-build.Ninja && winget install --id LLVM.LLVM

# Optional: Install Doxygen for documentation generation
winget install -e --id DimitriVanHeesch.Doxygen
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

## Linux Installation

### Ubuntu/Debian

Copy and paste the following command to install all dependencies:

```bash
# Install all dependencies (LLVM 21)
LLVM_VER=21 && sudo apt-get update && sudo apt-get install -y wget lsb-release ca-certificates gnupg cmake ninja-build && sudo mkdir -p /etc/apt/keyrings && wget -qO- https://apt.llvm.org/llvm-snapshot.gpg.key | gpg --dearmor | sudo tee /etc/apt/keyrings/apt.llvm.org.gpg >/dev/null && echo "deb [signed-by=/etc/apt/keyrings/apt.llvm.org.gpg] http://apt.llvm.org/$(lsb_release -cs)/ llvm-toolchain-$(lsb_release -cs)-${LLVM_VER} main" | sudo tee /etc/apt/sources.list.d/llvm.list && sudo apt-get update && sudo apt-get install -y clang-${LLVM_VER} clang++-${LLVM_VER} lld-${LLVM_VER} llvm-${LLVM_VER} lldb-${LLVM_VER} && sudo update-alternatives --install /usr/bin/clang clang /usr/bin/clang-${LLVM_VER} 100 && sudo update-alternatives --install /usr/bin/clang++ clang++ /usr/bin/clang++-${LLVM_VER} 100 && sudo update-alternatives --install /usr/bin/lld lld /usr/bin/lld-${LLVM_VER} 100 && sudo update-alternatives --install /usr/bin/llvm-objdump llvm-objdump /usr/bin/llvm-objdump-${LLVM_VER} 100 && sudo update-alternatives --install /usr/bin/llvm-objcopy llvm-objcopy /usr/bin/llvm-objcopy-${LLVM_VER} 100 && sudo update-alternatives --install /usr/bin/llvm-strings llvm-strings /usr/bin/llvm-strings-${LLVM_VER} 100 && sudo update-alternatives --install /usr/bin/lldb-dap lldb-dap /usr/bin/lldb-dap-${LLVM_VER} 100 && sudo update-alternatives --set clang "/usr/bin/clang-${LLVM_VER}" && sudo update-alternatives --set clang++ "/usr/bin/clang++-${LLVM_VER}" && sudo update-alternatives --set lld "/usr/bin/lld-${LLVM_VER}" && sudo update-alternatives --set llvm-objdump "/usr/bin/llvm-objdump-${LLVM_VER}" && sudo update-alternatives --set llvm-objcopy "/usr/bin/llvm-objcopy-${LLVM_VER}" && sudo update-alternatives --set llvm-strings "/usr/bin/llvm-strings-${LLVM_VER}" && sudo update-alternatives --set lldb-dap "/usr/bin/lldb-dap-${LLVM_VER}"
```

**Note:** To install a different LLVM version, change `LLVM_VER=21` to your desired version (e.g., `LLVM_VER=22`).

### Verify Installation

```bash
clang --version
clang++ --version
lld --version
cmake --version
ninja --version
```

## macOS Installation

### Prerequisites

Ensure [Homebrew](https://brew.sh/) is installed.

### Installation

Copy and paste the following command into Terminal:

```bash
# Install all dependencies
brew install llvm cmake ninja
```

Homebrew installs LLVM to a non-default path. Add it to your PATH:

```bash
# For Apple Silicon (M1/M2/M3/M4)
echo 'export PATH="/opt/homebrew/opt/llvm/bin:$PATH"' >> ~/.zshrc
source ~/.zshrc

# For Intel Macs
echo 'export PATH="/usr/local/opt/llvm/bin:$PATH"' >> ~/.zshrc
source ~/.zshrc
```

### Verify Installation

```bash
clang --version
clang++ --version
ld64.lld --version
cmake --version
ninja --version
```

## IDE Configuration

### Visual Studio Code

This project is designed and optimized for [Visual Studio Code](https://code.visualstudio.com/).

When you open this project in VSCode, you will be automatically prompted to install the recommended extensions.

### WSL Integration

If you're developing on Windows with WSL (Windows Subsystem for Linux), you can open this project in WSL using VSCode's remote development features. This is recommended for C++ development as it provides a Linux build environment.

#### Opening in WSL

**From within WSL:**

Navigate to the project directory and run:

```bash
code .
```

**Prerequisites:**
- Ensure WSL is properly configured on your Windows system
- Install QEMU, UEFI firmware, and disk image tools:
  ```bash
  sudo apt-get update && sudo apt-get install -y qemu-user-static qemu-system-x86 qemu-system-arm ovmf qemu-efi-aarch64 dosfstools mtools
  ```
  This installs:
  - QEMU for cross-architecture execution (ARM on x86_64) and UEFI testing in virtual machines (x86_64 and aarch64)
  - `dosfstools` and `mtools` for creating FAT disk images required by UEFI filesystem tests

For more information, see the [VSCode WSL documentation](https://code.visualstudio.com/docs/remote/wsl).

Refer to [.vscode/README.md](.vscode/README.md) for detailed VSCode configuration and usage instructions.

## Version Requirements

- **CMake**: 3.20 or higher
- **Ninja**: 1.10 or higher
- **LLVM/Clang**: 21 or higher

