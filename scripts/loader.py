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

# --- Architecture definitions ---

ARCH = {
    'i386':    {'bits': 32, 'family': 'x86', 'qemu': ['qemu-i386-static', 'qemu-i386']},
    'x86_64':  {'bits': 64, 'family': 'x86', 'qemu': ['qemu-x86_64-static', 'qemu-x86_64']},
    'armv7a':  {'bits': 32, 'family': 'arm', 'qemu': ['qemu-arm-static', 'qemu-arm']},
    'aarch64': {'bits': 64, 'family': 'arm', 'qemu': ['qemu-aarch64-static', 'qemu-aarch64']},
}

# --- Win32 constants ---

MEM_COMMIT_RESERVE                 = 0x3000
PAGE_EXECUTE_READWRITE             = 0x40
CREATE_SUSPENDED                   = 0x00000004
EXTENDED_STARTUPINFO_PRESENT       = 0x00080000
INFINITE                           = 0xFFFFFFFF
PROC_THREAD_ATTRIBUTE_MACHINE_TYPE = 0x00020019

MACHINE_TYPE = {
    'i386':    0x014c,  # IMAGE_FILE_MACHINE_I386
    'x86_64':  0x8664,  # IMAGE_FILE_MACHINE_AMD64
    'aarch64': 0xAA64,  # IMAGE_FILE_MACHINE_ARM64
}

HOST_PROCESS = {
    'i386':    r'C:\Windows\SysWOW64\cmd.exe',
    'x86_64':  r'C:\Windows\System32\cmd.exe',
    'aarch64': r'C:\Windows\System32\cmd.exe',
}

# --- Host detection ---

_HOST_FAMILIES = [
    (('amd64', 'x86_64'),     'x86', 64),
    (('arm64', 'aarch64'),    'arm', 64),
    (('i386', 'i686', 'x86'), 'x86', 32),
    (('armv7l', 'armv7a'),    'arm', 32),
]


def get_host():
    os_name = platform.system().lower()
    machine = platform.machine().lower()
    for aliases, family, bits in _HOST_FAMILIES:
        if machine in aliases:
            return os_name, family, bits
    return os_name, machine, 64


# --- Linux runners ---

def run_qemu(elf_path, qemu_cmds):
    qemu_cmd = next((c for c in qemu_cmds if shutil.which(c)), None)
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


# --- Windows helpers ---

def setup_kernel32():
    from ctypes import wintypes
    k32 = ctypes.windll.kernel32

    # Memory
    k32.VirtualAllocEx.argtypes = [wintypes.HANDLE, wintypes.LPVOID, ctypes.c_size_t, wintypes.DWORD, wintypes.DWORD]
    k32.VirtualAllocEx.restype = wintypes.LPVOID

    k32.WriteProcessMemory.argtypes = [wintypes.HANDLE, wintypes.LPVOID, wintypes.LPCVOID, ctypes.c_size_t, ctypes.POINTER(ctypes.c_size_t)]
    k32.WriteProcessMemory.restype = wintypes.BOOL

    # Threads
    k32.CreateRemoteThread.argtypes = [wintypes.HANDLE, wintypes.LPVOID, ctypes.c_size_t, wintypes.LPVOID, wintypes.LPVOID, wintypes.DWORD, wintypes.LPVOID]
    k32.CreateRemoteThread.restype = wintypes.HANDLE

    k32.WaitForSingleObject.argtypes = [wintypes.HANDLE, wintypes.DWORD]
    k32.WaitForSingleObject.restype = wintypes.DWORD

    k32.GetExitCodeThread.argtypes = [wintypes.HANDLE, wintypes.LPDWORD]
    k32.GetExitCodeThread.restype = wintypes.BOOL

    # Handles / processes
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

    # Proc thread attributes (cross-family process creation)
    k32.InitializeProcThreadAttributeList.argtypes = [
        ctypes.c_void_p, wintypes.DWORD, wintypes.DWORD, ctypes.POINTER(ctypes.c_size_t)
    ]
    k32.InitializeProcThreadAttributeList.restype = wintypes.BOOL

    k32.UpdateProcThreadAttribute.argtypes = [
        ctypes.c_void_p, wintypes.DWORD, ctypes.c_size_t,
        ctypes.c_void_p, ctypes.c_size_t, ctypes.c_void_p, ctypes.c_void_p
    ]
    k32.UpdateProcThreadAttribute.restype = wintypes.BOOL

    k32.DeleteProcThreadAttributeList.argtypes = [ctypes.c_void_p]
    k32.DeleteProcThreadAttributeList.restype = None

    return k32


# --- Windows runner ---

def run_injected(shellcode, target_arch, cross_family=False):
    """Run shellcode via process injection (for architecture mismatch on Windows)."""
    from ctypes import wintypes

    host_exe = HOST_PROCESS.get(target_arch)
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

    class STARTUPINFOEXW(ctypes.Structure):
        _fields_ = [
            ("StartupInfo", STARTUPINFOW),
            ("lpAttributeList", ctypes.c_void_p),
        ]

    class PROCESS_INFORMATION(ctypes.Structure):
        _fields_ = [
            ("hProcess", wintypes.HANDLE), ("hThread", wintypes.HANDLE),
            ("dwProcessId", wintypes.DWORD), ("dwThreadId", wintypes.DWORD),
        ]

    pi = PROCESS_INFORMATION()
    creation_flags = CREATE_SUSPENDED
    attr_list_buf = None

    if cross_family and target_arch in MACHINE_TYPE:
        machine = ctypes.c_ushort(MACHINE_TYPE[target_arch])

        # Query required buffer size
        size = ctypes.c_size_t(0)
        k32.InitializeProcThreadAttributeList(None, 1, 0, ctypes.byref(size))

        # Allocate and initialize attribute list
        attr_list_buf = (ctypes.c_byte * size.value)()
        if not k32.InitializeProcThreadAttributeList(attr_list_buf, 1, 0, ctypes.byref(size)):
            raise OSError(f"InitializeProcThreadAttributeList failed: {k32.GetLastError()}")

        if not k32.UpdateProcThreadAttribute(
            attr_list_buf, 0, PROC_THREAD_ATTRIBUTE_MACHINE_TYPE,
            ctypes.byref(machine), ctypes.sizeof(machine), None, None
        ):
            k32.DeleteProcThreadAttributeList(attr_list_buf)
            raise OSError(f"UpdateProcThreadAttribute failed: {k32.GetLastError()}")

        siex = STARTUPINFOEXW()
        siex.StartupInfo.cb = ctypes.sizeof(STARTUPINFOEXW)
        siex.lpAttributeList = ctypes.addressof(attr_list_buf)
        creation_flags |= EXTENDED_STARTUPINFO_PRESENT

        print(f"[*] Machine type override: 0x{MACHINE_TYPE[target_arch]:04x}")

        if not k32.CreateProcessW(
            host_exe, None, None, None, False, creation_flags,
            None, None, ctypes.byref(siex), ctypes.byref(pi)
        ):
            k32.DeleteProcThreadAttributeList(attr_list_buf)
            raise OSError(f"CreateProcessW failed: {k32.GetLastError()}")
    else:
        si = STARTUPINFOW()
        si.cb = ctypes.sizeof(STARTUPINFOW)

        if not k32.CreateProcessW(host_exe, None, None, None, False, creation_flags, None, None, ctypes.byref(si), ctypes.byref(pi)):
            raise OSError(f"CreateProcessW failed: {k32.GetLastError()}")

    print(f"[+] Created process PID: {pi.dwProcessId}")

    try:
        remote_mem = k32.VirtualAllocEx(pi.hProcess, None, len(shellcode), MEM_COMMIT_RESERVE, PAGE_EXECUTE_READWRITE)
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

        k32.WaitForSingleObject(remote_thread, INFINITE)

        code = wintypes.DWORD()
        k32.GetExitCodeThread(remote_thread, ctypes.byref(code))
        k32.CloseHandle(remote_thread)
        return code.value

    finally:
        k32.TerminateProcess(pi.hProcess, 0)
        k32.CloseHandle(pi.hThread)
        k32.CloseHandle(pi.hProcess)
        if attr_list_buf is not None:
            k32.DeleteProcThreadAttributeList(attr_list_buf)


# --- Entry point ---

def main():
    parser = argparse.ArgumentParser(description='PIC Shellcode Loader')
    parser.add_argument('--arch', required=True, choices=list(ARCH.keys()))
    parser.add_argument('shellcode')
    args = parser.parse_args()

    host_os, host_family, host_bits = get_host()
    target = ARCH[args.arch]
    python_bits = struct.calcsize("P") * 8

    print(f"[*] Host: {host_os}/{host_family}/{host_bits}bit")
    print(f"[*] Python: {python_bits}bit")
    print(f"[*] Target: {args.arch}")

    with open(args.shellcode, 'rb') as f:
        shellcode = f.read()
    print(f"[+] Loaded: {len(shellcode)} bytes")

    if host_os == 'windows':
        cross_family = host_family != target['family']
        code = run_injected(shellcode, args.arch, cross_family=cross_family)
    elif target['family'] != host_family or target['bits'] != host_bits:
        elf_path = args.shellcode.rsplit('.', 1)[0] + '.elf'
        if not os.path.exists(elf_path):
            sys.exit(f"[-] ELF file not found: {elf_path}")
        code = run_qemu(elf_path, target['qemu'])
    else:
        code = run_mmap(shellcode)

    print(f"[+] Exit: {code}")
    os._exit(code)


if __name__ == '__main__':
    main()
