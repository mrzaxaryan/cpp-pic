#pragma once

#include "primitives.h"
#include "windows_types.h"
#include "djb2.h"
#include "error.h"
#include "result.h"

#define HANDLE_FLAG_INHERIT 0x00000001
#define STARTF_USESTDHANDLES 0x00000100


// Structures for process creation and management in Windows API
typedef struct _STARTUPINFOW
{
    UINT32 cb;
    PWCHAR lpReserved;
    PWCHAR lpDesktop;
    PWCHAR lpTitle;
    UINT32 dwX;
    UINT32 dwY;
    UINT32 dwXSize;
    UINT32 dwYSize;
    UINT32 dwXCountChars;
    UINT32 dwYCountChars;
    UINT32 dwFillAttribute;
    UINT32 dwFlags;
    UINT16 wShowWindow;
    UINT16 cbReserved2;
    PCHAR lpReserved2;
    PVOID hStdInput;
    PVOID hStdOutput;
    PVOID hStdError;
} STARTUPINFOW, *LPSTARTUPINFOW;

// Structure for process information
typedef struct _PROCESS_INFORMATION
{
    PVOID hProcess;
    PVOID hThread;
    UINT32 dwProcessId;
    UINT32 dwThreadId;
} PROCESS_INFORMATION, *PPROCESS_INFORMATION, *LPPROCESS_INFORMATION;


// Kernel32 class for wrapping Windows API functions 
class Kernel32
{
private:
public:
    // Creates a new process and its primary thread.
    // Returns Ok() on success, Err(Error{Kernel32_CreateProcessFailed}) on failure.
    // Minimum supported client Windows Xp [desktop apps | UWP apps]
    [[nodiscard]] static Result<void, Error> CreateProcessW(PWCHAR lpApplicationName, PWCHAR lpCommandLine, PVOID lpProcessAttributes, PVOID lpThreadAttributes, BOOL bInheritHandles, UINT32 dwCreationFlags, PVOID lpEnvironment, PWCHAR lpCurrentDirectory, LPSTARTUPINFOW lpStartupInfo, LPPROCESS_INFORMATION lpProcessInformation);
    // Sets certain properties of an object handle.
    // Returns Ok() on success, Err(Error{Kernel32_SetHandleInfoFailed}) on failure.
    // Minimum supported client Windows 2000 Professional [desktop apps only]
    [[nodiscard]] static Result<void, Error> SetHandleInformation(PVOID hObject, UINT32 dwMask, UINT32 dwFlags);
};
