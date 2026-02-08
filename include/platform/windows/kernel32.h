#pragma once

#include "primitives.h"
#include "windows_types.h"
#include "djb2.h"

#define HANDLE_FLAG_INHERIT 0x00000001
#define SW_HIDE 0
#define STARTF_USESHOWWINDOW 0x00000001
#define STARTF_USESTDHANDLES 0x00000100

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

typedef struct _PROCESS_INFORMATION
{
    PVOID hProcess;
    PVOID hThread;
    UINT32 dwProcessId;
    UINT32 dwThreadId;
} PROCESS_INFORMATION, *PPROCESS_INFORMATION, *LPPROCESS_INFORMATION;

#define CP_UTF8 65001
#define CREATE_NO_WINDOW 0x08000000

class Kernel32
{
private:
public:
    // MultiByteToWideChar
    static int MultiByteToWideChar(UINT32 CodePage, UINT32 dwFlags, const CHAR *lpMultiByteStr, INT32 cbMultiByte, PWCHAR lpWideCharStr, INT32 cchWideChar);
    // CreateProcessW
    static BOOL CreateProcessW(PWCHAR lpApplicationName, PWCHAR lpCommandLine, PVOID lpProcessAttributes, PVOID lpThreadAttributes, BOOL bInheritHandles, UINT32 dwCreationFlags, PVOID lpEnvironment, PWCHAR lpCurrentDirectory, LPSTARTUPINFOW lpStartupInfo, LPPROCESS_INFORMATION lpProcessInformation);
    // SetHandleInformation
    static BOOL SetHandleInformation(PVOID hObject, UINT32 dwMask, UINT32 dwFlags);
};
