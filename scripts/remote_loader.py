#!/usr/bin/env python3
"""
Remote PIC Shellcode Loader

Downloads the correct .bin for the current platform/architecture
from GitHub Releases and executes it in memory.

Usage:
    python remote_loader.py
    python remote_loader.py --tag v0.0.1-alpha.1
"""

import argparse
import ctypes
import json
import mmap
import os
import platform
import sys
import urllib.error
import urllib.request

REPO = "mrzaxaryan/Position-Independent-Runtime"

# --- Host detection ---

_HOST_FAMILIES = [
    (('amd64', 'x86_64', 'i86pc'), 'x86',   64),
    (('arm64', 'aarch64'),          'arm',   64),
    (('i386', 'i686', 'x86'),       'x86',   32),
    (('armv7l', 'armv7a'),          'arm',   32),
    (('riscv64',),                  'riscv', 64),
    (('riscv32',),                  'riscv', 32),
    (('mips64',),                   'mips',  64),
]

# Map (os_name, family, bits) -> (platform_name, arch_name)
_HOST_TO_ARTIFACT = {
    ('linux',   'x86',   64): ('linux',   'x86_64'),
    ('linux',   'x86',   32): ('linux',   'i386'),
    ('linux',   'arm',   64): ('linux',   'aarch64'),
    ('linux',   'arm',   32): ('linux',   'armv7a'),
    ('linux',   'riscv', 64): ('linux',   'riscv64'),
    ('linux',   'riscv', 32): ('linux',   'riscv32'),
    ('windows', 'x86',   64): ('windows', 'x86_64'),
    ('windows', 'x86',   32): ('windows', 'i386'),
    ('windows', 'arm',   64): ('windows', 'aarch64'),
    ('windows', 'arm',   32): ('windows', 'armv7a'),
    ('darwin',  'x86',   64): ('macos',   'x86_64'),
    ('darwin',  'arm',   64): ('macos',   'aarch64'),
    ('freebsd', 'x86',   64): ('freebsd', 'x86_64'),
    ('freebsd', 'x86',   32): ('freebsd', 'i386'),
    ('freebsd', 'arm',   64): ('freebsd', 'aarch64'),
    ('freebsd', 'riscv', 64): ('freebsd', 'riscv64'),
    ('sunos',   'x86',   64): ('solaris', 'x86_64'),
    ('sunos',   'x86',   32): ('solaris', 'i386'),
    ('sunos',   'arm',   64): ('solaris', 'aarch64'),
    ('linux',   'mips',  64): ('linux',   'mips64'),
}


def get_host():
    os_name = platform.system().lower()
    machine = platform.machine().lower()
    for aliases, family, bits in _HOST_FAMILIES:
        if machine in aliases:
            return os_name, family, bits
    return os_name, machine, 64


# --- Execution ---

def _flush_icache(addr, size):
    machine = platform.machine().lower()
    if machine not in ('arm64', 'aarch64'):
        return
    libc = ctypes.CDLL(None)
    if platform.system().lower() == 'darwin':
        libc.sys_icache_invalidate.argtypes = [ctypes.c_void_p, ctypes.c_size_t]
        libc.sys_icache_invalidate.restype = None
        libc.sys_icache_invalidate(ctypes.c_void_p(addr), ctypes.c_size_t(size))


def run_mmap(shellcode):
    mem = mmap.mmap(-1, len(shellcode), prot=mmap.PROT_READ | mmap.PROT_WRITE)
    mem.write(shellcode)
    addr = ctypes.addressof(ctypes.c_char.from_buffer(mem))
    libc = ctypes.CDLL(None)
    page_size = os.sysconf('SC_PAGE_SIZE')
    aligned = addr & ~(page_size - 1)
    size = len(shellcode) + (addr - aligned)
    if libc.mprotect(ctypes.c_void_p(aligned), ctypes.c_size_t(size), mmap.PROT_READ | mmap.PROT_EXEC) != 0:
        raise OSError("mprotect failed")
    _flush_icache(addr, len(shellcode))

    print(f"[+] Entry: 0x{addr:x}")
    print("[*] Executing...")
    sys.stdout.flush()

    return ctypes.CFUNCTYPE(ctypes.c_int)(addr)()


def run_injected(shellcode, target_arch):
    """Run shellcode via process injection (Windows only)."""
    from ctypes import wintypes

    MEM_COMMIT_RESERVE = 0x3000
    PAGE_EXECUTE_READWRITE = 0x40
    CREATE_SUSPENDED = 0x00000004
    INFINITE = 0xFFFFFFFF

    HOST_PROCESS = {
        'i386':    r'C:\Windows\SysWOW64\cmd.exe',
        'x86_64':  r'C:\Windows\System32\cmd.exe',
        'armv7a':  r'C:\Windows\SysArm32\cmd.exe',
        'aarch64': r'C:\Windows\System32\cmd.exe',
    }

    host_exe = HOST_PROCESS.get(target_arch)
    if not host_exe or not os.path.exists(host_exe):
        raise OSError(f"No suitable host process for {target_arch}")

    print(f"[+] Host process: {host_exe}")

    k32 = ctypes.windll.kernel32

    k32.VirtualAllocEx.argtypes = [wintypes.HANDLE, wintypes.LPVOID, ctypes.c_size_t, wintypes.DWORD, wintypes.DWORD]
    k32.VirtualAllocEx.restype = wintypes.LPVOID
    k32.WriteProcessMemory.argtypes = [wintypes.HANDLE, wintypes.LPVOID, wintypes.LPCVOID, ctypes.c_size_t, ctypes.POINTER(ctypes.c_size_t)]
    k32.WriteProcessMemory.restype = wintypes.BOOL
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

    if not k32.CreateProcessW(host_exe, None, None, None, False, CREATE_SUSPENDED, None, None, ctypes.byref(si), ctypes.byref(pi)):
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


# --- Download ---

def http_get(url):
    req = urllib.request.Request(url, headers={"User-Agent": "PIR-RemoteLoader/1.0"})
    with urllib.request.urlopen(req) as resp:
        return resp.read()


def find_latest_tag():
    url = f"https://api.github.com/repos/{REPO}/releases?per_page=1"
    req = urllib.request.Request(url, headers={
        "User-Agent": "PIR-RemoteLoader/1.0",
        "Accept": "application/vnd.github+json",
    })
    with urllib.request.urlopen(req) as resp:
        releases = json.loads(resp.read())
    if not releases:
        sys.exit("[-] No releases found")
    return releases[0]["tag_name"]


def download_release_asset(platform_name, arch, tag):
    if not tag:
        tag = find_latest_tag()
        print(f"[+] Latest release: {tag}")

    asset = f"{platform_name}-{arch}.bin"
    url = f"https://github.com/{REPO}/releases/download/{tag}/{asset}"

    print(f"[*] Downloading: {asset}")
    print(f"[*] URL: {url}")
    try:
        return http_get(url)
    except urllib.error.HTTPError as e:
        if e.code == 404:
            sys.exit(
                f"[-] Asset '{asset}' not found in release {tag}.\n"
                f"    URL: {url}"
            )
        raise


# --- Entry point ---

def main():
    parser = argparse.ArgumentParser(description='Remote PIC Shellcode Loader')
    parser.add_argument('--tag', default=None,
                        help='Release tag (default: latest)')
    args = parser.parse_args()

    host_os, host_family, host_bits = get_host()

    print(f"[*] Host: {host_os}/{host_family}/{host_bits}bit")

    host_key = (host_os, host_family, host_bits)
    if host_key not in _HOST_TO_ARTIFACT:
        sys.exit(f"[-] Unsupported host: {host_os}/{host_family}/{host_bits}bit")

    platform_name, arch = _HOST_TO_ARTIFACT[host_key]

    print(f"[*] Platform: {platform_name}/{arch}")
    print(f"[*] Release: {args.tag or 'latest'}")

    shellcode = download_release_asset(platform_name, arch, args.tag)
    print(f"[+] Loaded: {len(shellcode)} bytes")

    if host_os == 'windows':
        code = run_injected(shellcode, arch)
    else:
        code = run_mmap(shellcode)

    print(f"[+] Exit: {code}")
    os._exit(code)


if __name__ == '__main__':
    main()
