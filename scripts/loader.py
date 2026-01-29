#!/usr/bin/env python3
"""
Cross-platform shellcode loader for Windows
Supports: i386, x86_64, aarch64

Usage:
    python loader.py --arch i386 shellcode.bin
    python loader.py --arch x86_64 shellcode.bin
    python loader.py --arch aarch64 shellcode.bin
"""

import argparse
import ctypes
import os
import platform
import struct
import sys
from ctypes import wintypes

# Windows API constants
MEM_COMMIT = 0x1000
MEM_RESERVE = 0x2000
MEM_RELEASE = 0x8000
PAGE_EXECUTE_READWRITE = 0x40
PAGE_READWRITE = 0x04
PROCESS_ALL_ACCESS = 0x1F0FFF
INFINITE = 0xFFFFFFFF
CREATE_SUSPENDED = 0x00000004

# Architecture mappings
ARCH_INFO = {
    'i386': {'bits': 32, 'family': 'x86'},
    'x86_64': {'bits': 64, 'family': 'x86'},
    'aarch64': {'bits': 64, 'family': 'arm'},
}

# Host processes for injection by architecture
HOST_PROCESSES = {
    'i386': [
        r'C:\Windows\SysWOW64\cmd.exe',
        r'C:\Windows\SysWOW64\conhost.exe',
    ],
    'x86_64': [
        r'C:\Windows\System32\cmd.exe',
        r'C:\Windows\System32\conhost.exe',
    ],
    'aarch64': [
        r'C:\Windows\System32\cmd.exe',
        r'C:\Windows\System32\conhost.exe',
    ],
}


class STARTUPINFO(ctypes.Structure):
    _fields_ = [
        ("cb", wintypes.DWORD),
        ("lpReserved", wintypes.LPWSTR),
        ("lpDesktop", wintypes.LPWSTR),
        ("lpTitle", wintypes.LPWSTR),
        ("dwX", wintypes.DWORD),
        ("dwY", wintypes.DWORD),
        ("dwXSize", wintypes.DWORD),
        ("dwYSize", wintypes.DWORD),
        ("dwXCountChars", wintypes.DWORD),
        ("dwYCountChars", wintypes.DWORD),
        ("dwFillAttribute", wintypes.DWORD),
        ("dwFlags", wintypes.DWORD),
        ("wShowWindow", wintypes.WORD),
        ("cbReserved2", wintypes.WORD),
        ("lpReserved2", ctypes.POINTER(wintypes.BYTE)),
        ("hStdInput", wintypes.HANDLE),
        ("hStdOutput", wintypes.HANDLE),
        ("hStdError", wintypes.HANDLE),
    ]


class PROCESS_INFORMATION(ctypes.Structure):
    _fields_ = [
        ("hProcess", wintypes.HANDLE),
        ("hThread", wintypes.HANDLE),
        ("dwProcessId", wintypes.DWORD),
        ("dwThreadId", wintypes.DWORD),
    ]


def get_python_bitness():
    """Get the bitness of the current Python interpreter."""
    return struct.calcsize("P") * 8


def get_host_info():
    """Get host CPU family and bitness."""
    machine = platform.machine().lower()
    if machine in ('amd64', 'x86_64'):
        return 'x86', 64
    elif machine in ('arm64', 'aarch64'):
        return 'arm', 64
    elif machine in ('i386', 'i686', 'x86'):
        return 'x86', 32
    return machine, 64


def can_execute(target_arch):
    """Check if we can execute the target architecture on this host."""
    host_family, host_bits = get_host_info()
    target_info = ARCH_INFO.get(target_arch)

    if not target_info:
        return False, f"Unknown architecture: {target_arch}"

    target_family = target_info['family']
    target_bits = target_info['bits']

    # Different CPU families cannot run each other's code
    if host_family != target_family:
        return False, f"Cannot run {target_family.upper()} code on {host_family.upper()} CPU"

    # 32-bit host cannot run 64-bit code
    if host_bits == 32 and target_bits == 64:
        return False, "Cannot run 64-bit code on 32-bit system"

    return True, None


def needs_injection(target_arch):
    """Check if we need process injection (bitness mismatch)."""
    python_bits = get_python_bitness()
    target_bits = ARCH_INFO.get(target_arch, {}).get('bits', 64)
    return python_bits != target_bits


def find_host_process(target_arch):
    """Find a suitable host process for the target architecture."""
    candidates = HOST_PROCESSES.get(target_arch, [])

    for path in candidates:
        if os.path.exists(path):
            return path

    raise FileNotFoundError(f"No suitable host process found for {target_arch}")


def load_shellcode(filepath):
    """Load shellcode from file."""
    if not os.path.exists(filepath):
        raise FileNotFoundError(f"Shellcode file not found: {filepath}")

    with open(filepath, 'rb') as f:
        shellcode = f.read()

    if len(shellcode) == 0:
        raise ValueError("Shellcode file is empty")

    return shellcode


def setup_kernel32():
    """Setup kernel32 function prototypes."""
    kernel32 = ctypes.windll.kernel32

    kernel32.VirtualAlloc.argtypes = [wintypes.LPVOID, ctypes.c_size_t, wintypes.DWORD, wintypes.DWORD]
    kernel32.VirtualAlloc.restype = wintypes.LPVOID

    kernel32.VirtualAllocEx.argtypes = [wintypes.HANDLE, wintypes.LPVOID, ctypes.c_size_t, wintypes.DWORD, wintypes.DWORD]
    kernel32.VirtualAllocEx.restype = wintypes.LPVOID

    kernel32.WriteProcessMemory.argtypes = [wintypes.HANDLE, wintypes.LPVOID, wintypes.LPCVOID, ctypes.c_size_t, ctypes.POINTER(ctypes.c_size_t)]
    kernel32.WriteProcessMemory.restype = wintypes.BOOL

    kernel32.CreateProcessW.argtypes = [
        wintypes.LPCWSTR, wintypes.LPWSTR, wintypes.LPVOID, wintypes.LPVOID,
        wintypes.BOOL, wintypes.DWORD, wintypes.LPVOID, wintypes.LPCWSTR,
        ctypes.POINTER(STARTUPINFO), ctypes.POINTER(PROCESS_INFORMATION)
    ]
    kernel32.CreateProcessW.restype = wintypes.BOOL

    kernel32.CreateRemoteThread.argtypes = [
        wintypes.HANDLE, wintypes.LPVOID, ctypes.c_size_t,
        wintypes.LPVOID, wintypes.LPVOID, wintypes.DWORD, wintypes.LPVOID
    ]
    kernel32.CreateRemoteThread.restype = wintypes.HANDLE

    kernel32.RtlMoveMemory.argtypes = [wintypes.LPVOID, wintypes.LPCVOID, ctypes.c_size_t]
    kernel32.RtlMoveMemory.restype = None

    kernel32.CreateThread.argtypes = [wintypes.LPVOID, ctypes.c_size_t, wintypes.LPVOID, wintypes.LPVOID, wintypes.DWORD, wintypes.LPVOID]
    kernel32.CreateThread.restype = wintypes.HANDLE

    kernel32.WaitForSingleObject.argtypes = [wintypes.HANDLE, wintypes.DWORD]
    kernel32.WaitForSingleObject.restype = wintypes.DWORD

    kernel32.GetExitCodeThread.argtypes = [wintypes.HANDLE, wintypes.LPDWORD]
    kernel32.GetExitCodeThread.restype = wintypes.BOOL

    kernel32.TerminateProcess.argtypes = [wintypes.HANDLE, wintypes.UINT]
    kernel32.TerminateProcess.restype = wintypes.BOOL

    kernel32.CloseHandle.argtypes = [wintypes.HANDLE]
    kernel32.CloseHandle.restype = wintypes.BOOL

    kernel32.GetLastError.argtypes = []
    kernel32.GetLastError.restype = wintypes.DWORD

    return kernel32


def execute_local(kernel32, shellcode):
    """Execute shellcode in current process."""
    shellcode_size = len(shellcode)

    print(f"[*] Allocating {shellcode_size} bytes...")
    ptr = kernel32.VirtualAlloc(None, shellcode_size, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE)

    if not ptr:
        raise OSError(f"VirtualAlloc failed: {kernel32.GetLastError()}")

    print(f"[+] Memory at: 0x{ptr:016x}")
    print("[*] Copying shellcode...")
    kernel32.RtlMoveMemory(ptr, shellcode, shellcode_size)

    print("[*] Executing...")
    thread = kernel32.CreateThread(None, 0, ptr, None, 0, None)

    if not thread:
        raise OSError(f"CreateThread failed: {kernel32.GetLastError()}")

    kernel32.WaitForSingleObject(thread, INFINITE)

    exit_code = wintypes.DWORD()
    kernel32.GetExitCodeThread(thread, ctypes.byref(exit_code))
    kernel32.CloseHandle(thread)

    return exit_code.value


def execute_injected(kernel32, shellcode, target_arch):
    """Execute shellcode via process injection."""
    shellcode_size = len(shellcode)
    host_path = find_host_process(target_arch)

    print(f"[*] Host process: {host_path}")

    si = STARTUPINFO()
    si.cb = ctypes.sizeof(STARTUPINFO)
    pi = PROCESS_INFORMATION()

    print("[*] Creating suspended process...")
    success = kernel32.CreateProcessW(
        host_path, None, None, None, True, CREATE_SUSPENDED,
        None, None, ctypes.byref(si), ctypes.byref(pi)
    )

    if not success:
        raise OSError(f"CreateProcessW failed: {kernel32.GetLastError()}")

    print(f"[+] PID: {pi.dwProcessId}")

    try:
        print(f"[*] Allocating {shellcode_size} bytes...")
        remote_mem = kernel32.VirtualAllocEx(
            pi.hProcess, None, shellcode_size,
            MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE
        )

        if not remote_mem:
            raise OSError(f"VirtualAllocEx failed: {kernel32.GetLastError()}")

        print(f"[+] Remote memory at: 0x{remote_mem:08x}")

        print("[*] Writing shellcode...")
        written = ctypes.c_size_t()
        success = kernel32.WriteProcessMemory(
            pi.hProcess, remote_mem, shellcode, shellcode_size, ctypes.byref(written)
        )

        if not success:
            raise OSError(f"WriteProcessMemory failed: {kernel32.GetLastError()}")

        print(f"[+] Written {written.value} bytes")
        print("[*] Executing...")
        print("=" * 50)

        remote_thread = kernel32.CreateRemoteThread(
            pi.hProcess, None, 0, remote_mem, None, 0, None
        )

        if not remote_thread:
            raise OSError(f"CreateRemoteThread failed: {kernel32.GetLastError()}")

        kernel32.WaitForSingleObject(remote_thread, INFINITE)

        print("=" * 50)

        exit_code = wintypes.DWORD()
        kernel32.GetExitCodeThread(remote_thread, ctypes.byref(exit_code))
        kernel32.CloseHandle(remote_thread)

        return exit_code.value

    finally:
        kernel32.TerminateProcess(pi.hProcess, 0)
        kernel32.CloseHandle(pi.hThread)
        kernel32.CloseHandle(pi.hProcess)


def main():
    parser = argparse.ArgumentParser(description='Shellcode loader for Windows')
    parser.add_argument('--arch', required=True, choices=['i386', 'x86_64', 'aarch64'],
                        help='Target architecture')
    parser.add_argument('shellcode', help='Path to shellcode binary')

    args = parser.parse_args()

    print(f"[*] Target: {args.arch}")
    print(f"[*] File: {args.shellcode}")
    print(f"[*] Host: {platform.system()} {platform.machine()}")
    print()

    # Check if we can run this architecture
    can_run, error = can_execute(args.arch)
    if not can_run:
        print(f"[-] Error: {error}")
        sys.exit(1)

    # Load shellcode
    try:
        shellcode = load_shellcode(args.shellcode)
        print(f"[+] Loaded {len(shellcode)} bytes")
    except (FileNotFoundError, ValueError) as e:
        print(f"[-] Error: {e}")
        sys.exit(1)

    # Must be Windows
    if platform.system().lower() != 'windows':
        print(f"[-] This loader only supports Windows")
        sys.exit(1)

    # Execute
    try:
        kernel32 = setup_kernel32()

        if needs_injection(args.arch):
            print(f"[*] Using process injection")
            exit_code = execute_injected(kernel32, shellcode, args.arch)
        else:
            print(f"[*] Executing locally")
            exit_code = execute_local(kernel32, shellcode)

        print(f"\n[+] Exit code: {exit_code}")
        sys.exit(exit_code)

    except Exception as e:
        print(f"[-] Failed: {e}")
        sys.exit(1)


if __name__ == '__main__':
    main()
