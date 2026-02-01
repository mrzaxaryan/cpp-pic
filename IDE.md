# IDE Configuration

## Visual Studio Code

This project is designed and optimized for [Visual Studio Code](https://code.visualstudio.com/).

When you open this project in VSCode, you will be automatically prompted to install the recommended extensions.

## WSL Integration

If you're developing on Windows with WSL (Windows Subsystem for Linux), you can open this project in WSL using VSCode's remote development features. This is recommended for C++ development as it provides a Linux build environment.

### Opening in WSL

**From within WSL:**

Navigate to the project directory and run:

```bash
code .
```

**Prerequisites:**
- Ensure WSL is properly configured on your Windows system
- Install QEMU and UEFI firmware:
  ```bash
  sudo apt-get update && sudo apt-get install -y qemu-user-static qemu-system-x86 qemu-system-arm ovmf qemu-efi-aarch64
  ```
  This installs QEMU for cross-architecture execution (ARM on x86_64) and UEFI testing in virtual machines (x86_64 and aarch64).

For more information, see the [VSCode WSL documentation](https://code.visualstudio.com/docs/remote/wsl).

## Next Steps

Refer to [.vscode/README.md](.vscode/README.md) for detailed VSCode configuration and usage instructions.
