#!/usr/bin/env python3
"""
PIC Shellcode Loader

Cross-platform loader for position-independent code.
Auto-detects platform. Uses QEMU for cross-arch on Linux.

Usage:
    python loader.py --arch x86_64 output.bin
    python loader.py --arch i386 output.bin
    python loader.py --arch aarch64 output.bin
    python loader.py --arch armv7a output.bin
"""

import argparse
import ctypes
import mmap
import os
import platform
import shutil
import struct
import subprocess
import sys

ARCH = {
    'i386':    {'bits': 32, 'family': 'x86', 'qemu': ['qemu-i386-static', 'qemu-i386']},
    'x86_64':  {'bits': 64, 'family': 'x86', 'qemu': ['qemu-x86_64-static', 'qemu-x86_64']},
    'armv7a':  {'bits': 32, 'family': 'arm', 'qemu': ['qemu-arm-static', 'qemu-arm']},
    'aarch64': {'bits': 64, 'family': 'arm', 'qemu': ['qemu-aarch64-static', 'qemu-aarch64']},
}


def get_host():
    os_name = platform.system().lower()
    machine = platform.machine().lower()

    if machine in ('amd64', 'x86_64'):
        return os_name, 'x86', 64
    elif machine in ('arm64', 'aarch64'):
        return os_name, 'arm', 64
    elif machine in ('i386', 'i686', 'x86'):
        return os_name, 'x86', 32
    elif machine in ('armv7l', 'armv7a'):
        return os_name, 'arm', 32
    return os_name, machine, 64


def needs_qemu(host_family, host_bits, target_arch):
    target = ARCH[target_arch]
    return target['family'] != host_family or target['bits'] != host_bits


def run_qemu(elf_path, qemu_cmds):
    qemu_cmd = None
    for cmd in qemu_cmds:
        if shutil.which(cmd):
            qemu_cmd = cmd
            break

    if not qemu_cmd:
        sys.exit("[-] QEMU not found. Install qemu-user-static.")

    print(f"[+] Using: {qemu_cmd}")
    print("[*] Executing...")
    sys.stdout.flush()

    return subprocess.run([qemu_cmd, elf_path]).returncode


def run_mmap(shellcode):
    mem = mmap.mmap(-1, len(shellcode), prot=mmap.PROT_READ | mmap.PROT_WRITE | mmap.PROT_EXEC)
    mem.write(shellcode)

    entry = ctypes.addressof(ctypes.c_char.from_buffer(mem))

    print(f"[+] Entry: 0x{entry:x}")
    print("[*] Executing...")
    sys.stdout.flush()

    return ctypes.CFUNCTYPE(ctypes.c_int)(entry)()


def setup_kernel32():
    from ctypes import wintypes
    k32 = ctypes.windll.kernel32

    k32.VirtualAlloc.argtypes = [wintypes.LPVOID, ctypes.c_size_t, wintypes.DWORD, wintypes.DWORD]
    k32.VirtualAlloc.restype = wintypes.LPVOID

    k32.VirtualAllocEx.argtypes = [wintypes.HANDLE, wintypes.LPVOID, ctypes.c_size_t, wintypes.DWORD, wintypes.DWORD]
    k32.VirtualAllocEx.restype = wintypes.LPVOID

    k32.VirtualProtect.argtypes = [wintypes.LPVOID, ctypes.c_size_t, wintypes.DWORD, ctypes.POINTER(wintypes.DWORD)]
    k32.VirtualProtect.restype = wintypes.BOOL

    k32.WriteProcessMemory.argtypes = [wintypes.HANDLE, wintypes.LPVOID, wintypes.LPCVOID, ctypes.c_size_t, ctypes.POINTER(ctypes.c_size_t)]
    k32.WriteProcessMemory.restype = wintypes.BOOL

    k32.CreateThread.argtypes = [wintypes.LPVOID, ctypes.c_size_t, wintypes.LPVOID, wintypes.LPVOID, wintypes.DWORD, wintypes.LPVOID]
    k32.CreateThread.restype = wintypes.HANDLE

    k32.CreateRemoteThread.argtypes = [wintypes.HANDLE, wintypes.LPVOID, ctypes.c_size_t, wintypes.LPVOID, wintypes.LPVOID, wintypes.DWORD, wintypes.LPVOID]
    k32.CreateRemoteThread.restype = wintypes.HANDLE

    k32.WaitForSingleObject.argtypes = [wintypes.HANDLE, wintypes.DWORD]
    k32.WaitForSingleObject.restype = wintypes.DWORD

    k32.GetExitCodeThread.argtypes = [wintypes.HANDLE, wintypes.LPDWORD]
    k32.GetExitCodeThread.restype = wintypes.BOOL

    k32.CloseHandle.argtypes = [wintypes.HANDLE]
    k32.CloseHandle.restype = wintypes.BOOL

    k32.TerminateProcess.argtypes = [wintypes.HANDLE, wintypes.UINT]
    k32.TerminateProcess.restype = wintypes.BOOL

    k32.GetLastError.argtypes = []
    k32.GetLastError.restype = wintypes.DWORD

    k32.CreateProcessW.argtypes = [
        wintypes.LPCWSTR, wintypes.LPWSTR, wintypes.LPVOID, wintypes.LPVOID,
        wintypes.BOOL, wintypes.DWORD, wintypes.LPVOID, wintypes.LPCWSTR,
        wintypes.LPVOID, wintypes.LPVOID
    ]
    k32.CreateProcessW.restype = wintypes.BOOL

    return k32


def run_virtualalloc(shellcode):
    from ctypes import wintypes
    k32 = setup_kernel32()

    ptr = k32.VirtualAlloc(None, len(shellcode), 0x3000, 0x04)  # PAGE_READWRITE
    if not ptr:
        raise OSError(f"VirtualAlloc failed: {k32.GetLastError()}")

    print(f"[+] Allocated: 0x{ptr:x}")

    ctypes.memmove(ptr, shellcode, len(shellcode))

    old_protect = wintypes.DWORD()
    if not k32.VirtualProtect(ptr, len(shellcode), 0x40, ctypes.byref(old_protect)):  # PAGE_EXECUTE_READWRITE
        raise OSError(f"VirtualProtect failed: {k32.GetLastError()}")

    print(f"[+] Entry: 0x{ptr:x}")
    print("[*] Executing...")
    sys.stdout.flush()

    thread = k32.CreateThread(None, 0, ptr, None, 0, None)
    if not thread:
        raise OSError(f"CreateThread failed: {k32.GetLastError()}")

    k32.WaitForSingleObject(thread, 0xFFFFFFFF)
    code = wintypes.DWORD()
    k32.GetExitCodeThread(thread, ctypes.byref(code))
    k32.CloseHandle(thread)
    return code.value


def run_injected(shellcode, target_arch):
    """Run shellcode via process injection (for bitness mismatch on Windows)."""
    from ctypes import wintypes

    host_procs = {
        'i386': r'C:\Windows\SysWOW64\cmd.exe',
        'x86_64': r'C:\Windows\System32\cmd.exe',
        'aarch64': r'C:\Windows\System32\cmd.exe',
    }
    host_exe = host_procs.get(target_arch)
    if not host_exe or not os.path.exists(host_exe):
        raise OSError(f"No suitable host process for {target_arch}")

    print(f"[+] Host process: {host_exe}")

    k32 = setup_kernel32()

    class STARTUPINFOW(ctypes.Structure):
        _fields_ = [
            ("cb", wintypes.DWORD), ("lpReserved", wintypes.LPWSTR),
            ("lpDesktop", wintypes.LPWSTR), ("lpTitle", wintypes.LPWSTR),
            ("dwX", wintypes.DWORD), ("dwY", wintypes.DWORD),
            ("dwXSize", wintypes.DWORD), ("dwYSize", wintypes.DWORD),
            ("dwXCountChars", wintypes.DWORD), ("dwYCountChars", wintypes.DWORD),
            ("dwFillAttribute", wintypes.DWORD), ("dwFlags", wintypes.DWORD),
            ("wShowWindow", wintypes.WORD), ("cbReserved2", wintypes.WORD),
            ("lpReserved2", ctypes.POINTER(wintypes.BYTE)),
            ("hStdInput", wintypes.HANDLE), ("hStdOutput", wintypes.HANDLE),
            ("hStdError", wintypes.HANDLE),
        ]

    class PROCESS_INFORMATION(ctypes.Structure):
        _fields_ = [
            ("hProcess", wintypes.HANDLE), ("hThread", wintypes.HANDLE),
            ("dwProcessId", wintypes.DWORD), ("dwThreadId", wintypes.DWORD),
        ]

    si = STARTUPINFOW()
    si.cb = ctypes.sizeof(STARTUPINFOW)
    pi = PROCESS_INFORMATION()

    CREATE_SUSPENDED = 0x00000004
    if not k32.CreateProcessW(host_exe, None, None, None, False, CREATE_SUSPENDED, None, None, ctypes.byref(si), ctypes.byref(pi)):
        raise OSError(f"CreateProcessW failed: {k32.GetLastError()}")

    print(f"[+] Created process PID: {pi.dwProcessId}")

    try:
        remote_mem = k32.VirtualAllocEx(pi.hProcess, None, len(shellcode), 0x3000, 0x40)  # RWX
        if not remote_mem:
            raise OSError(f"VirtualAllocEx failed: {k32.GetLastError()}")

        print(f"[+] Remote memory: 0x{remote_mem:x}")

        written = ctypes.c_size_t()
        if not k32.WriteProcessMemory(pi.hProcess, remote_mem, shellcode, len(shellcode), ctypes.byref(written)):
            raise OSError(f"WriteProcessMemory failed: {k32.GetLastError()}")

        print(f"[+] Written: {written.value} bytes")
        print(f"[+] Entry: 0x{remote_mem:x}")
        print("[*] Executing...")
        sys.stdout.flush()

        remote_thread = k32.CreateRemoteThread(pi.hProcess, None, 0, remote_mem, None, 0, None)
        if not remote_thread:
            raise OSError(f"CreateRemoteThread failed: {k32.GetLastError()}")

        k32.WaitForSingleObject(remote_thread, 0xFFFFFFFF)

        code = wintypes.DWORD()
        k32.GetExitCodeThread(remote_thread, ctypes.byref(code))
        k32.CloseHandle(remote_thread)
        return code.value

    finally:
        k32.TerminateProcess(pi.hProcess, 0)
        k32.CloseHandle(pi.hThread)
        k32.CloseHandle(pi.hProcess)


def get_python_bits():
    return struct.calcsize("P") * 8


def main():
    parser = argparse.ArgumentParser(description='PIC Shellcode Loader')
    parser.add_argument('--arch', required=True, choices=list(ARCH.keys()))
    parser.add_argument('shellcode')
    args = parser.parse_args()

    host_os, host_family, host_bits = get_host()
    target = ARCH[args.arch]
    python_bits = get_python_bits()

    print(f"[*] Host: {host_os}/{host_family}/{host_bits}bit")
    print(f"[*] Python: {python_bits}bit")
    print(f"[*] Target: {args.arch}")

    with open(args.shellcode, 'rb') as f:
        shellcode = f.read()
    print(f"[+] Loaded: {len(shellcode)} bytes")

    if host_os == 'windows':
        if python_bits != target['bits']:
            print(f"[*] Bitness mismatch ({python_bits}bit Python, {target['bits']}bit target) - using injection")
            code = run_injected(shellcode, args.arch)
        else:
            code = run_virtualalloc(shellcode)
    elif needs_qemu(host_family, host_bits, args.arch):
        # QEMU needs the ELF file, not raw binary
        elf_path = args.shellcode.rsplit('.', 1)[0] + '.elf'
        if not os.path.exists(elf_path):
            sys.exit(f"[-] ELF file not found: {elf_path}")
        code = run_qemu(elf_path, target['qemu'])
    else:
        code = run_mmap(shellcode)

    print(f"[+] Exit: {code}")
    sys.exit(code)


if __name__ == '__main__':
    main()
