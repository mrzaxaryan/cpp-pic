# Position-Independent Runtime (PIR)

### A Modern C++23 Approach to Zero-Dependency, Position-Independent Code Generation

<sub>

![](https://img.shields.io/badge/%E2%80%8B-orange?style=flat-square) Compile only (no runner) &nbsp;&nbsp; ![](https://img.shields.io/badge/%E2%80%8B-yellow?style=flat-square) Tests pass (as executable) &nbsp;&nbsp; ![](https://img.shields.io/badge/%E2%80%8B-brightgreen?style=flat-square) Tests pass (as shellcode)

</sub>

<table>
<tr>
<th width="30">#</th>
<th width="120">Platform</th>
<th width="220">i386</th>
<th width="220">x86_64</th>
<th width="220">armv7a</th>
<th width="220">aarch64</th>
<th width="220">riscv32</th>
<th width="220">riscv64</th>
<th width="220">mips64</th>
</tr>
<tr>
<td align="center">1</td>
<td>Windows</td>
<td align="center"><a href="https://github.com/mrzaxaryan/Position-Independent-Runtime/actions/workflows/build-windows-i386.yml?query=branch%3Amain"><img src="https://img.shields.io/github/actions/workflow/status/mrzaxaryan/Position-Independent-Runtime/build-windows-i386.yml?branch=main&label=%E2%80%8B&labelColor=brightgreen&style=for-the-badge" alt="Build" height="28"></a></td>
<td align="center"><a href="https://github.com/mrzaxaryan/Position-Independent-Runtime/actions/workflows/build-windows-x86_64.yml?query=branch%3Amain"><img src="https://img.shields.io/github/actions/workflow/status/mrzaxaryan/Position-Independent-Runtime/build-windows-x86_64.yml?branch=main&label=%E2%80%8B&labelColor=brightgreen&style=for-the-badge" alt="Build" height="28"></a></td>
<td align="center"><a href="https://github.com/mrzaxaryan/Position-Independent-Runtime/actions/workflows/build-windows-armv7a.yml?query=branch%3Amain"><img src="https://img.shields.io/github/actions/workflow/status/mrzaxaryan/Position-Independent-Runtime/build-windows-armv7a.yml?branch=main&label=%E2%80%8B&labelColor=orange&style=for-the-badge" alt="Build" height="28"></a></td>
<td align="center"><a href="https://github.com/mrzaxaryan/Position-Independent-Runtime/actions/workflows/build-windows-aarch64.yml?query=branch%3Amain"><img src="https://img.shields.io/github/actions/workflow/status/mrzaxaryan/Position-Independent-Runtime/build-windows-aarch64.yml?branch=main&label=%E2%80%8B&labelColor=brightgreen&style=for-the-badge" alt="Build" height="28"></a></td>
<td align="center">-</td>
<td align="center">-</td>
<td align="center">-</td>
</tr>
<tr>
<td align="center">2</td>
<td>macOS</td>
<td align="center">-</td>
<td align="center"><a href="https://github.com/mrzaxaryan/Position-Independent-Runtime/actions/workflows/build-macos-x86_64.yml?query=branch%3Amain"><img src="https://img.shields.io/github/actions/workflow/status/mrzaxaryan/Position-Independent-Runtime/build-macos-x86_64.yml?branch=main&label=%E2%80%8B&labelColor=brightgreen&style=for-the-badge" alt="Build" height="28"></a></td>
<td align="center">-</td>
<td align="center"><a href="https://github.com/mrzaxaryan/Position-Independent-Runtime/actions/workflows/build-macos-aarch64.yml?query=branch%3Amain"><img src="https://img.shields.io/github/actions/workflow/status/mrzaxaryan/Position-Independent-Runtime/build-macos-aarch64.yml?branch=main&label=%E2%80%8B&labelColor=brightgreen&style=for-the-badge" alt="Build" height="28"></a></td>
<td align="center">-</td>
<td align="center">-</td>
<td align="center">-</td>
</tr>
<tr>
<td align="center">3</td>
<td>Linux</td>
<td align="center"><a href="https://github.com/mrzaxaryan/Position-Independent-Runtime/actions/workflows/build-linux-i386.yml?query=branch%3Amain"><img src="https://img.shields.io/github/actions/workflow/status/mrzaxaryan/Position-Independent-Runtime/build-linux-i386.yml?branch=main&label=%E2%80%8B&labelColor=brightgreen&style=for-the-badge" alt="Build" height="28"></a></td>
<td align="center"><a href="https://github.com/mrzaxaryan/Position-Independent-Runtime/actions/workflows/build-linux-x86_64.yml?query=branch%3Amain"><img src="https://img.shields.io/github/actions/workflow/status/mrzaxaryan/Position-Independent-Runtime/build-linux-x86_64.yml?branch=main&label=%E2%80%8B&labelColor=brightgreen&style=for-the-badge" alt="Build" height="28"></a></td>
<td align="center"><a href="https://github.com/mrzaxaryan/Position-Independent-Runtime/actions/workflows/build-linux-armv7a.yml?query=branch%3Amain"><img src="https://img.shields.io/github/actions/workflow/status/mrzaxaryan/Position-Independent-Runtime/build-linux-armv7a.yml?branch=main&label=%E2%80%8B&labelColor=brightgreen&style=for-the-badge" alt="Build" height="28"></a></td>
<td align="center"><a href="https://github.com/mrzaxaryan/Position-Independent-Runtime/actions/workflows/build-linux-aarch64.yml?query=branch%3Amain"><img src="https://img.shields.io/github/actions/workflow/status/mrzaxaryan/Position-Independent-Runtime/build-linux-aarch64.yml?branch=main&label=%E2%80%8B&labelColor=brightgreen&style=for-the-badge" alt="Build" height="28"></a></td>
<td align="center"><a href="https://github.com/mrzaxaryan/Position-Independent-Runtime/actions/workflows/build-linux-riscv32.yml?query=branch%3Amain"><img src="https://img.shields.io/github/actions/workflow/status/mrzaxaryan/Position-Independent-Runtime/build-linux-riscv32.yml?branch=main&label=%E2%80%8B&labelColor=yellow&style=for-the-badge" alt="Build" height="28"></a></td>
<td align="center"><a href="https://github.com/mrzaxaryan/Position-Independent-Runtime/actions/workflows/build-linux-riscv64.yml?query=branch%3Amain"><img src="https://img.shields.io/github/actions/workflow/status/mrzaxaryan/Position-Independent-Runtime/build-linux-riscv64.yml?branch=main&label=%E2%80%8B&labelColor=brightgreen&style=for-the-badge" alt="Build" height="28"></a></td>
<td align="center"><a href="https://github.com/mrzaxaryan/Position-Independent-Runtime/actions/workflows/build-linux-mips64.yml?query=branch%3Amain"><img src="https://img.shields.io/github/actions/workflow/status/mrzaxaryan/Position-Independent-Runtime/build-linux-mips64.yml?branch=main&label=%E2%80%8B&labelColor=orange&style=for-the-badge" alt="Build" height="28"></a></td>
</tr>
<tr>
<td align="center">4</td>
<td>Solaris</td>
<td align="center"><a href="https://github.com/mrzaxaryan/Position-Independent-Runtime/actions/workflows/build-solaris-i386.yml?query=branch%3Amain"><img src="https://img.shields.io/github/actions/workflow/status/mrzaxaryan/Position-Independent-Runtime/build-solaris-i386.yml?branch=main&label=%E2%80%8B&labelColor=yellow&style=for-the-badge" alt="Build" height="28"></a></td>
<td align="center"><a href="https://github.com/mrzaxaryan/Position-Independent-Runtime/actions/workflows/build-solaris-x86_64.yml?query=branch%3Amain"><img src="https://img.shields.io/github/actions/workflow/status/mrzaxaryan/Position-Independent-Runtime/build-solaris-x86_64.yml?branch=main&label=%E2%80%8B&labelColor=brightgreen&style=for-the-badge" alt="Build" height="28"></a></td>
<td align="center">-</td>
<td align="center"><a href="https://github.com/mrzaxaryan/Position-Independent-Runtime/actions/workflows/build-solaris-aarch64.yml?query=branch%3Amain"><img src="https://img.shields.io/github/actions/workflow/status/mrzaxaryan/Position-Independent-Runtime/build-solaris-aarch64.yml?branch=main&label=%E2%80%8B&labelColor=orange&style=for-the-badge" alt="Build" height="28"></a></td>
<td align="center">-</td>
<td align="center">-</td>
<td align="center">-</td>
</tr>
<tr>
<td align="center">5</td>
<td>FreeBSD</td>
<td align="center"><a href="https://github.com/mrzaxaryan/Position-Independent-Runtime/actions/workflows/build-freebsd-i386.yml?query=branch%3Amain"><img src="https://img.shields.io/github/actions/workflow/status/mrzaxaryan/Position-Independent-Runtime/build-freebsd-i386.yml?branch=main&label=%E2%80%8B&labelColor=yellow&style=for-the-badge" alt="Build" height="28"></a></td>
<td align="center"><a href="https://github.com/mrzaxaryan/Position-Independent-Runtime/actions/workflows/build-freebsd-x86_64.yml?query=branch%3Amain"><img src="https://img.shields.io/github/actions/workflow/status/mrzaxaryan/Position-Independent-Runtime/build-freebsd-x86_64.yml?branch=main&label=%E2%80%8B&labelColor=brightgreen&style=for-the-badge" alt="Build" height="28"></a></td>
<td align="center">-</td>
<td align="center"><a href="https://github.com/mrzaxaryan/Position-Independent-Runtime/actions/workflows/build-freebsd-aarch64.yml?query=branch%3Amain"><img src="https://img.shields.io/github/actions/workflow/status/mrzaxaryan/Position-Independent-Runtime/build-freebsd-aarch64.yml?branch=main&label=%E2%80%8B&labelColor=brightgreen&style=for-the-badge" alt="Build" height="28"></a></td>
<td align="center">-</td>
<td align="center"><a href="https://github.com/mrzaxaryan/Position-Independent-Runtime/actions/workflows/build-freebsd-riscv64.yml?query=branch%3Amain"><img src="https://img.shields.io/github/actions/workflow/status/mrzaxaryan/Position-Independent-Runtime/build-freebsd-riscv64.yml?branch=main&label=%E2%80%8B&labelColor=yellow&style=for-the-badge" alt="Build" height="28"></a></td>
<td align="center">-</td>
</tr>
<tr>
<td align="center">6</td>
<td>UEFI</td>
<td align="center">-</td>
<td align="center"><a href="https://github.com/mrzaxaryan/Position-Independent-Runtime/actions/workflows/build-uefi-x86_64.yml?query=branch%3Amain"><img src="https://img.shields.io/github/actions/workflow/status/mrzaxaryan/Position-Independent-Runtime/build-uefi-x86_64.yml?branch=main&label=%E2%80%8B&labelColor=yellow&style=for-the-badge" alt="Build" height="28"></a></td>
<td align="center">-</td>
<td align="center"><a href="https://github.com/mrzaxaryan/Position-Independent-Runtime/actions/workflows/build-uefi-aarch64.yml?query=branch%3Amain"><img src="https://img.shields.io/github/actions/workflow/status/mrzaxaryan/Position-Independent-Runtime/build-uefi-aarch64.yml?branch=main&label=%E2%80%8B&labelColor=yellow&style=for-the-badge" alt="Build" height="28"></a></td>
<td align="center">-</td>
<td align="center">-</td>
<td align="center">-</td>
</tr>
<tr>
<td align="center">7</td>
<td>Android</td>
<td align="center">-</td>
<td align="center"><a href="https://github.com/mrzaxaryan/Position-Independent-Runtime/actions/workflows/build-android-x86_64.yml?query=branch%3Amain"><img src="https://img.shields.io/github/actions/workflow/status/mrzaxaryan/Position-Independent-Runtime/build-android-x86_64.yml?branch=main&label=%E2%80%8B&labelColor=brightgreen&style=for-the-badge" alt="Build" height="28"></a></td>
<td align="center"><a href="https://github.com/mrzaxaryan/Position-Independent-Runtime/actions/workflows/build-android-armv7a.yml?query=branch%3Amain"><img src="https://img.shields.io/github/actions/workflow/status/mrzaxaryan/Position-Independent-Runtime/build-android-armv7a.yml?branch=main&label=%E2%80%8B&labelColor=brightgreen&style=for-the-badge" alt="Build" height="28"></a></td>
<td align="center"><a href="https://github.com/mrzaxaryan/Position-Independent-Runtime/actions/workflows/build-android-aarch64.yml?query=branch%3Amain"><img src="https://img.shields.io/github/actions/workflow/status/mrzaxaryan/Position-Independent-Runtime/build-android-aarch64.yml?branch=main&label=%E2%80%8B&labelColor=brightgreen&style=for-the-badge" alt="Build" height="28"></a></td>
<td align="center">-</td>
<td align="center">-</td>
<td align="center">-</td>
</tr>
<tr>
<td align="center">8</td>
<td>iOS</td>
<td align="center">-</td>
<td align="center">-</td>
<td align="center">-</td>
<td align="center"><a href="https://github.com/mrzaxaryan/Position-Independent-Runtime/actions/workflows/build-ios-aarch64.yml?query=branch%3Amain"><img src="https://img.shields.io/github/actions/workflow/status/mrzaxaryan/Position-Independent-Runtime/build-ios-aarch64.yml?branch=main&label=%E2%80%8B&labelColor=brightgreen&style=for-the-badge" alt="Build" height="28"></a></td>
<td align="center">-</td>
<td align="center">-</td>
<td align="center">-</td>
</tr>
</table>

<details>
<summary>Shellcode sizes (release builds)</summary>

<table>
<tr>
<th width="30">#</th>
<th width="120">Platform</th>
<th width="220">i386</th>
<th width="220">x86_64</th>
<th width="220">armv7a</th>
<th width="220">aarch64</th>
<th width="220">riscv32</th>
<th width="220">riscv64</th>
<th width="220">mips64</th>
</tr>
<tr>
<td align="center">1</td>
<td>Windows</td>
<td align="center"><img src="https://img.shields.io/badge/dynamic/json?url=https%3A%2F%2Fraw.githubusercontent.com%2Fmrzaxaryan%2FPosition-Independent-Runtime%2Fbuild-sizes%2Fsizes.json&query=%24.windows_i386_bin&label=&style=for-the-badge&color=blue" alt="Bin" height="28"></td>
<td align="center"><img src="https://img.shields.io/badge/dynamic/json?url=https%3A%2F%2Fraw.githubusercontent.com%2Fmrzaxaryan%2FPosition-Independent-Runtime%2Fbuild-sizes%2Fsizes.json&query=%24.windows_x86_64_bin&label=&style=for-the-badge&color=blue" alt="Bin" height="28"></td>
<td align="center"><img src="https://img.shields.io/badge/dynamic/json?url=https%3A%2F%2Fraw.githubusercontent.com%2Fmrzaxaryan%2FPosition-Independent-Runtime%2Fbuild-sizes%2Fsizes.json&query=%24.windows_armv7a_bin&label=&style=for-the-badge&color=blue" alt="Bin" height="28"></td>
<td align="center"><img src="https://img.shields.io/badge/dynamic/json?url=https%3A%2F%2Fraw.githubusercontent.com%2Fmrzaxaryan%2FPosition-Independent-Runtime%2Fbuild-sizes%2Fsizes.json&query=%24.windows_aarch64_bin&label=&style=for-the-badge&color=blue" alt="Bin" height="28"></td>
<td align="center">-</td>
<td align="center">-</td>
<td align="center">-</td>
</tr>
<tr>
<td align="center">2</td>
<td>macOS</td>
<td align="center">-</td>
<td align="center"><img src="https://img.shields.io/badge/dynamic/json?url=https%3A%2F%2Fraw.githubusercontent.com%2Fmrzaxaryan%2FPosition-Independent-Runtime%2Fbuild-sizes%2Fsizes.json&query=%24.macos_x86_64_bin&label=&style=for-the-badge&color=blue" alt="Bin" height="28"></td>
<td align="center">-</td>
<td align="center"><img src="https://img.shields.io/badge/dynamic/json?url=https%3A%2F%2Fraw.githubusercontent.com%2Fmrzaxaryan%2FPosition-Independent-Runtime%2Fbuild-sizes%2Fsizes.json&query=%24.macos_aarch64_bin&label=&style=for-the-badge&color=blue" alt="Bin" height="28"></td>
<td align="center">-</td>
<td align="center">-</td>
<td align="center">-</td>
</tr>
<tr>
<td align="center">3</td>
<td>Linux</td>
<td align="center"><img src="https://img.shields.io/badge/dynamic/json?url=https%3A%2F%2Fraw.githubusercontent.com%2Fmrzaxaryan%2FPosition-Independent-Runtime%2Fbuild-sizes%2Fsizes.json&query=%24.linux_i386_bin&label=&style=for-the-badge&color=blue" alt="Bin" height="28"></td>
<td align="center"><img src="https://img.shields.io/badge/dynamic/json?url=https%3A%2F%2Fraw.githubusercontent.com%2Fmrzaxaryan%2FPosition-Independent-Runtime%2Fbuild-sizes%2Fsizes.json&query=%24.linux_x86_64_bin&label=&style=for-the-badge&color=blue" alt="Bin" height="28"></td>
<td align="center"><img src="https://img.shields.io/badge/dynamic/json?url=https%3A%2F%2Fraw.githubusercontent.com%2Fmrzaxaryan%2FPosition-Independent-Runtime%2Fbuild-sizes%2Fsizes.json&query=%24.linux_armv7a_bin&label=&style=for-the-badge&color=blue" alt="Bin" height="28"></td>
<td align="center"><img src="https://img.shields.io/badge/dynamic/json?url=https%3A%2F%2Fraw.githubusercontent.com%2Fmrzaxaryan%2FPosition-Independent-Runtime%2Fbuild-sizes%2Fsizes.json&query=%24.linux_aarch64_bin&label=&style=for-the-badge&color=blue" alt="Bin" height="28"></td>
<td align="center"><img src="https://img.shields.io/badge/dynamic/json?url=https%3A%2F%2Fraw.githubusercontent.com%2Fmrzaxaryan%2FPosition-Independent-Runtime%2Fbuild-sizes%2Fsizes.json&query=%24.linux_riscv32_bin&label=&style=for-the-badge&color=blue" alt="Bin" height="28"></td>
<td align="center"><img src="https://img.shields.io/badge/dynamic/json?url=https%3A%2F%2Fraw.githubusercontent.com%2Fmrzaxaryan%2FPosition-Independent-Runtime%2Fbuild-sizes%2Fsizes.json&query=%24.linux_riscv64_bin&label=&style=for-the-badge&color=blue" alt="Bin" height="28"></td>
<td align="center"><img src="https://img.shields.io/badge/dynamic/json?url=https%3A%2F%2Fraw.githubusercontent.com%2Fmrzaxaryan%2FPosition-Independent-Runtime%2Fbuild-sizes%2Fsizes.json&query=%24.linux_mips64_bin&label=&style=for-the-badge&color=blue" alt="Bin" height="28"></td>
</tr>
<tr>
<td align="center">4</td>
<td>Solaris</td>
<td align="center"><img src="https://img.shields.io/badge/dynamic/json?url=https%3A%2F%2Fraw.githubusercontent.com%2Fmrzaxaryan%2FPosition-Independent-Runtime%2Fbuild-sizes%2Fsizes.json&query=%24.solaris_i386_bin&label=&style=for-the-badge&color=blue" alt="Bin" height="28"></td>
<td align="center"><img src="https://img.shields.io/badge/dynamic/json?url=https%3A%2F%2Fraw.githubusercontent.com%2Fmrzaxaryan%2FPosition-Independent-Runtime%2Fbuild-sizes%2Fsizes.json&query=%24.solaris_x86_64_bin&label=&style=for-the-badge&color=blue" alt="Bin" height="28"></td>
<td align="center">-</td>
<td align="center"><img src="https://img.shields.io/badge/dynamic/json?url=https%3A%2F%2Fraw.githubusercontent.com%2Fmrzaxaryan%2FPosition-Independent-Runtime%2Fbuild-sizes%2Fsizes.json&query=%24.solaris_aarch64_bin&label=&style=for-the-badge&color=blue" alt="Bin" height="28"></td>
<td align="center">-</td>
<td align="center">-</td>
<td align="center">-</td>
</tr>
<tr>
<td align="center">5</td>
<td>FreeBSD</td>
<td align="center"><img src="https://img.shields.io/badge/dynamic/json?url=https%3A%2F%2Fraw.githubusercontent.com%2Fmrzaxaryan%2FPosition-Independent-Runtime%2Fbuild-sizes%2Fsizes.json&query=%24.freebsd_i386_bin&label=&style=for-the-badge&color=blue" alt="Bin" height="28"></td>
<td align="center"><img src="https://img.shields.io/badge/dynamic/json?url=https%3A%2F%2Fraw.githubusercontent.com%2Fmrzaxaryan%2FPosition-Independent-Runtime%2Fbuild-sizes%2Fsizes.json&query=%24.freebsd_x86_64_bin&label=&style=for-the-badge&color=blue" alt="Bin" height="28"></td>
<td align="center">-</td>
<td align="center"><img src="https://img.shields.io/badge/dynamic/json?url=https%3A%2F%2Fraw.githubusercontent.com%2Fmrzaxaryan%2FPosition-Independent-Runtime%2Fbuild-sizes%2Fsizes.json&query=%24.freebsd_aarch64_bin&label=&style=for-the-badge&color=blue" alt="Bin" height="28"></td>
<td align="center">-</td>
<td align="center"><img src="https://img.shields.io/badge/dynamic/json?url=https%3A%2F%2Fraw.githubusercontent.com%2Fmrzaxaryan%2FPosition-Independent-Runtime%2Fbuild-sizes%2Fsizes.json&query=%24.freebsd_riscv64_bin&label=&style=for-the-badge&color=blue" alt="Bin" height="28"></td>
<td align="center">-</td>
</tr>
<tr>
<td align="center">6</td>
<td>UEFI</td>
<td align="center">-</td>
<td align="center"><img src="https://img.shields.io/badge/dynamic/json?url=https%3A%2F%2Fraw.githubusercontent.com%2Fmrzaxaryan%2FPosition-Independent-Runtime%2Fbuild-sizes%2Fsizes.json&query=%24.uefi_x86_64_bin&label=&style=for-the-badge&color=blue" alt="Bin" height="28"></td>
<td align="center">-</td>
<td align="center"><img src="https://img.shields.io/badge/dynamic/json?url=https%3A%2F%2Fraw.githubusercontent.com%2Fmrzaxaryan%2FPosition-Independent-Runtime%2Fbuild-sizes%2Fsizes.json&query=%24.uefi_aarch64_bin&label=&style=for-the-badge&color=blue" alt="Bin" height="28"></td>
<td align="center">-</td>
<td align="center">-</td>
<td align="center">-</td>
</tr>
<tr>
<td align="center">7</td>
<td>Android</td>
<td align="center">-</td>
<td align="center"><img src="https://img.shields.io/badge/dynamic/json?url=https%3A%2F%2Fraw.githubusercontent.com%2Fmrzaxaryan%2FPosition-Independent-Runtime%2Fbuild-sizes%2Fsizes.json&query=%24.android_x86_64_bin&label=&style=for-the-badge&color=blue" alt="Bin" height="28"></td>
<td align="center"><img src="https://img.shields.io/badge/dynamic/json?url=https%3A%2F%2Fraw.githubusercontent.com%2Fmrzaxaryan%2FPosition-Independent-Runtime%2Fbuild-sizes%2Fsizes.json&query=%24.android_armv7a_bin&label=&style=for-the-badge&color=blue" alt="Bin" height="28"></td>
<td align="center"><img src="https://img.shields.io/badge/dynamic/json?url=https%3A%2F%2Fraw.githubusercontent.com%2Fmrzaxaryan%2FPosition-Independent-Runtime%2Fbuild-sizes%2Fsizes.json&query=%24.android_aarch64_bin&label=&style=for-the-badge&color=blue" alt="Bin" height="28"></td>
<td align="center">-</td>
<td align="center">-</td>
<td align="center">-</td>
</tr>
<tr>
<td align="center">8</td>
<td>iOS</td>
<td align="center">-</td>
<td align="center">-</td>
<td align="center">-</td>
<td align="center"><img src="https://img.shields.io/badge/dynamic/json?url=https%3A%2F%2Fraw.githubusercontent.com%2Fmrzaxaryan%2FPosition-Independent-Runtime%2Fbuild-sizes%2Fsizes.json&query=%24.ios_aarch64_bin&label=&style=for-the-badge&color=blue" alt="Bin" height="28"></td>
<td align="center">-</td>
<td align="center">-</td>
<td align="center">-</td>
</tr>
</table>

</details>

---

## Table of Contents

- [Introduction](#introduction)
- [Motivation](#motivation)
- [Architecture](#architecture)
- [Common Problems and Solutions](#common-problems-and-solutions)
- [Build System](#build-system)
- [Windows Implementation](#windows-implementation)
- [Use Cases](#use-cases)
- [Roadmap](#roadmap)
- [Contributing](#contributing)
- [License](#license)

---

## Introduction

Shellcode is a small, self-contained sequence of machine instructions that can be injected into memory and executed from an arbitrary location. It must operate without relying on external components such as DLLs, runtime initialization routines, or fixed stack layouts. Because of these strict constraints, shellcode is traditionally written in assembly language, which provides precise control over instructions, registers, and memory access.

While assembly ensures fully dependency-free and position-independent execution, it quickly becomes impractical as complexity grows due to its low-level nature and limited expressiveness. High-level languages like C offer improved readability, maintainability, and development speed, but standard C and C++ compilation models introduce significant challenges for shellcode development. Modern compilers typically generate binaries that depend on runtime libraries, import tables, relocation information, and read-only data sections. These dependencies violate the core requirement of shellcode -- execution independent of any fixed memory layout or external support -- and these problems do not admit simple or universally effective solutions.

As a result, code produced by conventional toolchains cannot be used as standalone shellcode without substantial modification or manual restructuring.

---

## Motivation

A long time ago, in a corner of the darknet, two users debated which programming language was better. One argued that assembly provides almost complete control over execution, while the other claimed that C, as a higher-level language, is a better choice for implementing complex systems, since writing something like a TLS client in assembly is impractical. That debate ended with the assembly coder being kicked out of that forum.

With this work, we would like to add our two cents to that debate by arguing that it is possible to leverage modern C++23 without compromising the strict execution guarantees required for shellcode.

---

## Architecture

PIR is built on a clean three-layer abstraction that separates concerns and enables multi-platform support:

```
+-------------------------------------------------------------+
|  RUNTIME (Runtime Abstraction Layer)                         |
|  High-level features: Cryptography, Networking, TLS 1.3     |
+-------------------------------------------------------------+
|  PLATFORM (Platform Abstraction Layer)                       |
|  OS-specific: Windows PEB/NTAPI, Linux/macOS syscalls        |
+-------------------------------------------------------------+
|  CORE (Core Abstraction Layer)                               |
|  Platform-independent: Types, Memory, Strings, Algorithms    |
+-------------------------------------------------------------+
```

**CORE** provides platform-independent primitives:
- Embedded types (`EMBEDDED_STRING`, `EMBEDDED_DOUBLE`, `EMBEDDED_ARRAY`)
- Numeric types (`UINT64`, `INT64`, `DOUBLE`) with guaranteed no `.rdata` generation
- Memory operations, string utilities, and formatting
- Algorithms (DJB2 hashing, Base64, random number generation)

**PLATFORM** handles OS and hardware specifics:
- Windows: PEB walking, PE parsing, NTAPI-based operations
- Linux: Direct syscall interface without libc
- macOS/iOS: Direct BSD syscall interface without libc (shared XNU kernel)
- FreeBSD: Direct BSD syscall interface without libc
- Android: Direct Linux kernel syscall interface without libc/Bionic
- Solaris: Direct syscall interface
- UEFI: Boot/runtime services
- Console I/O, file system, networking, memory allocation

**RUNTIME** provides high-level application features:
- Cryptography: SHA-256/384/512, HMAC, ChaCha20-Poly1305, ECC
- Networking: DNS resolution, HTTP client, WebSocket, TLS 1.3

Upper layers depend on lower layers, never the reverse. See [CONTRIBUTING.md](.github/CONTRIBUTING.md) for the full project structure and source tree layout.

---

## Common Problems and Solutions

When writing shellcode in C/C++, developers face several fundamental challenges. This section examines each problem, outlines traditional approaches, explains their limitations, and demonstrates how PIR provides a robust solution.

### Problem 1: String Literals in .rdata and Relocation Dependencies

C-generated shellcode relies on loader-handled relocations that are not applied in a loaderless execution environment, preventing reliable execution from arbitrary memory.

<details>
<summary>Traditional approaches and why they fail</summary>

**Option 1: Custom shellcode loader.** Requires saving the `.reloc` section in shellcode and implementing relocation logic -- not straightforward.

**Option 2: Stack-based strings.** Represent strings as character arrays on the stack:

```cpp
char path[] = {'e', 'x', 'a', 'm', 'p', 'l', 'e', '.', 'e', 'x', 'e', '\0'};
```

**Option 3: Manual runtime relocation.** Merge `.rdata` into `.text` via `/MERGE:.rdata=.text`, then fix up absolute addresses at runtime using pattern scanning:

```cpp
PCHAR currentAddress = GetInstructionAddress();
PCHAR startAddress = ReversePatternSearch(currentAddress, (PCHAR)&functionPrologue, sizeof(functionPrologue));
CHAR *relocatedString = string + (SSIZE)startAddress;
```

**Why they fail:** Modern compilers are sophisticated enough to recognize stack string patterns; with optimizations enabled, the compiler may consolidate individual character assignments, place the string data in `.rdata`, and replace the code with a `memcpy` call. This defeats the technique and reintroduces the `.rdata` dependency. Manual relocation approaches are fragile, increase binary size, and rely on compiler-specific behavior.
</details>

**PIR Solution: Compile-Time Embedding**

By eliminating all `.rdata` dependencies through compile-time embedding, PIR produces code that requires no relocations. String characters are packed into 64-bit words at compile time and written as immediate values in the instruction stream:

```cpp
auto msg = "Hello, World!"_embed; // Embedded in code, not .rdata
```

Assembly output:
```asm
movabsq $0x57202C6F6C6C6548, (%rsp)   ; "Hello, W" packed into a single immediate
movabsq $0x00000021646C726F, 8(%rsp)   ; "orld!\0"
```

The same technique applies to constant arrays via `MakeEmbedArray<T>(vals...)`:

```cpp
auto embedded = MakeEmbedArray<UINT32>(0x12345678, 0xABCDEF00);
UINT32 value = embedded[0]; // Unpacked at runtime
```

### Problem 2: Floating-Point Constants

Floating-point constants are emitted into `.rdata` sections, making them inaccessible in loaderless environments.

<details>
<summary>Traditional approaches and why they fail</summary>

Represent values using IEEE-754 hex and cast at runtime:

```cpp
UINT64 f = 0x3426328629;
double d = *((double*)&f);
```

This avoids embedding float literals but increases code size and complexity.
</details>

**PIR Solution: Floating-Point Constant Embedding**

Values are converted at compile time into IEEE-754 bit patterns and injected as immediate operands:

```cpp
auto pi = 3.14159_embed; // IEEE-754 as immediate value
```

```asm
movabsq $0x400921f9f01b866e, %rax ; Pi as 64-bit immediate
```

### Problem 3: Function Pointers

Function pointer addresses are resolved by the loader. Without the loader, indirect calls reference invalid addresses.

**PIR Solution:** The `EMBED_FUNC` macro uses inline assembly to compute pure PC-relative offsets, eliminating relocation dependencies entirely. See [embedded_function_pointer.h](src/core/types/embedded/embedded_function_pointer.h).

### Problem 4: 64-bit Arithmetic on 32-bit Systems

64-bit arithmetic on 32-bit systems causes the compiler to emit helper routines that may not be present.

**PIR Solution:** Custom `UINT64` and `INT64` classes store values as two 32-bit words. All operations (multiplication via 16-bit partial products, division via bit-by-bit long division, shifts with carry handling) are decomposed into 32-bit arithmetic with manual carry propagation.

### Problem 5: CRT and Runtime Dependencies

Standard programs depend on the CRT for initialization, memory management, and helper functions.

**PIR Solution:** Complete independence from the CRT by providing custom implementations for memory management, string manipulation, formatted output, and runtime initialization. A custom entry point eliminates loader-managed startup. On Windows, the PEB is traversed to locate modules and PE export tables are parsed using hash-based lookup -- no import tables, no `GetProcAddress`.

### Problem 6: Type Conversions

Integer-to-float conversions can cause the compiler to emit hidden constants or helper routines.

**PIR Solution:** All conversions use explicit bitwise and integer operations, preventing hidden constant generation:

```cpp
INT64 d_to_i64(const DOUBLE& d)
{
    UINT64 bits = d.bits;
    int exponent = ((bits >> 52) & 0x7FF) - 1023;
    UINT64 mantissa = (bits & 0xFFFFFFFFFFFFF) | 0x10000000000000;
    // ... bit shifting logic ...
}
```

---

## Build System

### Quick Start

**Requirements:** Clang/LLVM 22+, CMake 3.20+, Ninja 1.10+, C++23.

```bash
# Configure
cmake --preset {platform}-{arch}-{build_type}

# Build
cmake --build --preset {platform}-{arch}-{build_type}

# Test (exit code 0 = all pass)
./build/{build_type}/{platform}/{arch}/output.{exe|elf|efi}
```

Presets: `windows|linux|macos|ios|android|freebsd|solaris|uefi` x `i386|x86_64|armv7a|aarch64|riscv32|riscv64|mips64` x `debug|release`

See [CONTRIBUTING.md](.github/CONTRIBUTING.md) for toolchain installation instructions per platform.

### Critical Compiler Flags

Achieving true position-independence requires specific compiler flags:

```bash
-fno-jump-tables      # Prevents switch statement jump tables in .rdata
-fno-exceptions       # No exception handling tables
-fno-rtti             # No runtime type information
-nostdlib             # No standard C/C++ libraries
-fno-builtin          # Disable compiler built-in functions
-ffunction-sections   # Each function in own section (dead code elimination)
-fdata-sections       # Each data item in own section (garbage collection)
```

The `-fno-jump-tables` flag is particularly critical -- without it, `switch` statements generate jump tables stored in `.rdata`, breaking position-independence.

### Post-Build Verification

The build system automatically verifies that the final binary contains no data sections (`.rdata`, `.rodata`, `.data`, `.bss`, `.got`, `.plt`). This check runs after every build via `cmake/VerifyPICMode.cmake`.

---

## Windows Implementation

### Low-Level Native Interfaces

PIR builds directly on the NT Native API. On x86_64 and i386, Zw* wrappers use indirect syscalls -- resolving System Service Numbers (SSNs) at runtime and executing through `syscall`/`sysenter` gadgets found in ntdll. On ARM64, where the kernel validates that the `svc` instruction originates from within ntdll, wrappers call the ntdll export directly.

By eliminating static import tables and bypassing `GetProcAddress`, PIR removes all dependencies on the OS runtime initialization and dynamic linking. Function addresses are resolved internally using hash-based lookups of exported symbols.

### Capabilities

- **File system:** File creation, reading, writing, deletion; directory operations; path management -- all via NTAPI
- **Console output:** Printf-style output implemented natively within the runtime
- **Cryptography:** SHA-256/512, HMAC, ChaCha20, ECC, Base64
- **Networking:** DNS resolution, HTTP client, WebSocket, TLS 1.3 with certificate verification

---

## Use Cases

PIR is designed for execution environments where traditional runtime assumptions do not apply:

- **Authorized penetration testing** with written scope and client approval
- **Security research** with proper disclosure practices
- Shellcode and loaderless code execution
- Embedded and low-level system programming
- Cross-architecture C++ development
- Environments without standard C runtime support

> **Disclaimer:** Any unauthorized or malicious use of this software is strictly prohibited and falls outside the scope of the project's design goals.

---

## Roadmap

This project is a work in progress. Contributions are welcome -- see [CONTRIBUTING.md](.github/CONTRIBUTING.md).

### Planned Platforms
- NetBSD, OpenBSD, HaikuOS, QNX

### Planned Architectures
- PowerPC64 (ppc64/ppc64le), LoongArch64, s390x

### Other
- Additional Windows direct syscall implementations
- Compile-time polymorphism

---

## Contributing

We welcome contributions of all kinds. Please read:

- [Contributing Guide](.github/CONTRIBUTING.md) -- build instructions, code style, project structure
- [Code of Conduct](.github/CODE_OF_CONDUCT.md) -- community standards
- [Security Policy](.github/SECURITY.md) -- reporting vulnerabilities

---

## License

This project is licensed under the **GNU Affero General Public License v3.0** (AGPL-3.0). See [LICENSE](LICENSE) for full terms.

You are free to use, modify, and distribute this software, provided that any derivative work -- including network services -- is also made available under the AGPL-3.0. For proprietary/closed-source licensing, contact [mrzaxaryan](https://github.com/mrzaxaryan).
