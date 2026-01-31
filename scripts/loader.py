#!/usr/bin/env python3
"""
PIC Shellcode Loader for Windows and Linux

Loads position-independent code into executable memory and runs it.
The shellcode must be fully self-contained with no external dependencies.

Usage:
    python loader.py -windows --arch x86_64 shellcode.bin
    python loader.py -linux --arch x86_64 shellcode.bin
    python loader.py -linux --arch i386 shellcode.bin
"""

import argparse
import ctypes
import mmap
import os
import platform
import struct
import subprocess
import sys
import tempfile

ARCH_INFO = {
    'i386': {'bits': 32, 'family': 'x86', 'entry': 0x70},
    'x86_64': {'bits': 64, 'family': 'x86', 'entry': 0x1e0},
    'aarch64': {'bits': 64, 'family': 'arm', 'entry': 0x78},
}


def get_host_info():
    machine = platform.machine().lower()
    if machine in ('amd64', 'x86_64'):
        return 'x86', 64
    elif machine in ('arm64', 'aarch64'):
        return 'arm', 64
    elif machine in ('i386', 'i686', 'x86'):
        return 'x86', 32
    return machine, 64


def load_shellcode(filepath):
    with open(filepath, 'rb') as f:
        return f.read()


# =============================================================================
# LINUX
# =============================================================================

def build_elf32(shellcode, entry_offset=0):
    """Build minimal ELF32 executable with shellcode."""
    load_addr = 0x10000
    header_size = 52 + 32  # ELF header + 1 program header

    # ELF32 Header
    elf_header = struct.pack(
        '<4sBBBBB7sHHIIIIIHHHHHH',
        b'\x7fELF',     # magic
        1,              # 32-bit
        1,              # little endian
        1,              # ELF version
        0,              # OS/ABI (SYSV)
        0,              # ABI version
        b'\x00' * 7,    # padding
        2,              # ET_EXEC
        3,              # EM_386
        1,              # ELF version
        load_addr + header_size + entry_offset,  # entry point
        52,             # program header offset
        0,              # section header offset
        0,              # flags
        52,             # ELF header size
        32,             # program header size
        1,              # program header count
        0,              # section header size
        0,              # section header count
        0,              # section name string table index
    )

    # Program Header (PT_LOAD)
    file_size = header_size + len(shellcode)
    program_header = struct.pack(
        '<IIIIIIII',
        1,              # PT_LOAD
        0,              # offset
        load_addr,      # vaddr
        load_addr,      # paddr
        file_size,      # file size
        file_size,      # memory size
        7,              # flags: PF_R | PF_W | PF_X
        0x1000,         # alignment
    )

    return elf_header + program_header + shellcode


def build_elf64(shellcode, entry_offset=0):
    """Build minimal ELF64 executable with shellcode."""
    load_addr = 0x10000
    header_size = 64 + 56  # ELF header + 1 program header

    # ELF64 Header
    elf_header = struct.pack(
        '<4sBBBBB7sHHIQQQIHHHHHH',
        b'\x7fELF',     # magic
        2,              # 64-bit
        1,              # little endian
        1,              # ELF version
        0,              # OS/ABI
        0,              # ABI version
        b'\x00' * 7,    # padding
        2,              # ET_EXEC
        62,             # EM_X86_64
        1,              # ELF version
        load_addr + header_size + entry_offset,  # entry point
        64,             # program header offset
        0,              # section header offset
        0,              # flags
        64,             # ELF header size
        56,             # program header size
        1,              # program header count
        0,              # section header size
        0,              # section header count
        0,              # section name string table index
    )

    # Program Header (PT_LOAD)
    file_size = header_size + len(shellcode)
    program_header = struct.pack(
        '<IIQQQQQQ',
        1,              # PT_LOAD
        7,              # flags: PF_R | PF_W | PF_X
        0,              # offset
        load_addr,      # vaddr
        load_addr,      # paddr
        file_size,      # file size
        file_size,      # memory size
        0x1000,         # alignment
    )

    return elf_header + program_header + shellcode


def run_linux_native(shellcode, entry_offset=0x70):
    """Run shellcode natively using mmap."""
    size = len(shellcode)

    mem = mmap.mmap(-1, size, prot=mmap.PROT_READ | mmap.PROT_WRITE | mmap.PROT_EXEC)
    mem.write(shellcode)

    addr = ctypes.addressof(ctypes.c_char.from_buffer(mem))
    entry = addr + entry_offset
    print(f"[+] Loaded at: 0x{addr:x}")
    print(f"[+] Entry at: 0x{entry:x}")

    func = ctypes.CFUNCTYPE(ctypes.c_int)(entry)

    print("[*] Executing...")
    sys.stdout.flush()

    return func()


def run_linux_elf(shellcode, bits, entry_offset=0):
    """Run shellcode by wrapping in minimal ELF and executing."""
    if bits == 32:
        elf = build_elf32(shellcode, entry_offset)
    else:
        elf = build_elf64(shellcode, entry_offset)

    with tempfile.NamedTemporaryFile(delete=False, suffix='.elf') as f:
        f.write(elf)
        elf_path = f.name

    try:
        os.chmod(elf_path, 0o755)
        print(f"[+] Created ELF{bits}: {elf_path}")
        print("[*] Executing...")
        sys.stdout.flush()

        result = subprocess.run([elf_path])
        return result.returncode
    finally:
        os.unlink(elf_path)


def run_linux(shellcode, target_arch):
    """Execute shellcode on Linux."""
    host_family, host_bits = get_host_info()
    target_bits = ARCH_INFO[target_arch]['bits']
    entry_offset = ARCH_INFO[target_arch]['entry']

    if ARCH_INFO[target_arch]['family'] != host_family:
        print(f"[-] Cannot run {target_arch} on {host_family} CPU")
        return 1

    # Same bitness: can run directly via mmap
    if host_bits == target_bits:
        return run_linux_native(shellcode, entry_offset)

    # Cross-bitness: wrap in ELF and execute (kernel handles it)
    if host_bits == 64 and target_bits == 32:
        return run_linux_elf(shellcode, 32, entry_offset)

    print(f"[-] Cannot run 64-bit code on 32-bit host")
    return 1


# =============================================================================
# WINDOWS
# =============================================================================

def run_windows(shellcode, target_arch):
    """Execute shellcode on Windows."""
    from ctypes import wintypes

    kernel32 = ctypes.windll.kernel32

    MEM_COMMIT = 0x1000
    MEM_RESERVE = 0x2000
    PAGE_EXECUTE_READWRITE = 0x40
    INFINITE = 0xFFFFFFFF

    host_bits = struct.calcsize("P") * 8
    target_bits = ARCH_INFO[target_arch]['bits']

    if host_bits != target_bits:
        print(f"[-] Cross-bitness not supported on Windows")
        return 1

    size = len(shellcode)
    ptr = kernel32.VirtualAlloc(None, size, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE)
    if not ptr:
        print(f"[-] VirtualAlloc failed")
        return 1

    print(f"[+] Loaded at: 0x{ptr:x}")
    ctypes.memmove(ptr, shellcode, size)

    print("[*] Executing...")
    sys.stdout.flush()

    thread = kernel32.CreateThread(None, 0, ptr, None, 0, None)
    if not thread:
        print(f"[-] CreateThread failed")
        return 1

    kernel32.WaitForSingleObject(thread, INFINITE)

    exit_code = wintypes.DWORD()
    kernel32.GetExitCodeThread(thread, ctypes.byref(exit_code))
    kernel32.CloseHandle(thread)

    return exit_code.value


# =============================================================================
# MAIN
# =============================================================================

def main():
    parser = argparse.ArgumentParser(description='PIC Shellcode Loader')

    os_group = parser.add_mutually_exclusive_group(required=True)
    os_group.add_argument('-windows', action='store_true')
    os_group.add_argument('-linux', action='store_true')

    parser.add_argument('--arch', required=True, choices=['i386', 'x86_64', 'aarch64'])
    parser.add_argument('shellcode', help='Path to shellcode binary')

    args = parser.parse_args()

    target_os = 'windows' if args.windows else 'linux'
    host_os = platform.system().lower()

    print(f"[*] Target: {target_os}/{args.arch}")
    print(f"[*] Host: {platform.system()}/{platform.machine()}")
    print(f"[*] File: {args.shellcode}")

    if target_os != host_os:
        print(f"[-] Cannot run {target_os} shellcode on {host_os}")
        sys.exit(1)

    shellcode = load_shellcode(args.shellcode)
    print(f"[+] Size: {len(shellcode)} bytes")

    if target_os == 'linux':
        exit_code = run_linux(shellcode, args.arch)
    else:
        exit_code = run_windows(shellcode, args.arch)

    print(f"[+] Exit code: {exit_code}")
    sys.exit(exit_code)


if __name__ == '__main__':
    main()
