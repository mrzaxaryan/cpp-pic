#include "kernel32.h"
#include "pal.h"
#include "peb.h"

#define ResolveKernel32ExportAddress(functionName) ResolveExportAddressFromPebModule(Djb2::HashCompileTime(L"kernel32.dll"), Djb2::HashCompileTime(functionName))

BOOL Kernel32::WriteConsoleA(PVOID hConsoleOutput, const void *lpBuffer, INT32 nNumberOfCharsToWrite, PUINT32 lpNumberOfCharsWritten, LPOVERLAPPED lpOverlapped)
{
    return ((BOOL(STDCALL *)(PVOID hConsoleOutput, const void *lpBuffer, INT32 nNumberOfCharsToWrite, PUINT32 lpNumberOfCharsWritten, LPOVERLAPPED lpOverlapped))ResolveKernel32ExportAddress("WriteConsoleA"))(hConsoleOutput, lpBuffer, nNumberOfCharsToWrite, lpNumberOfCharsWritten, lpOverlapped);
}

BOOL Kernel32::CreateProcessA(PCHAR lpApplicationName, PCHAR lpCommandLine, LPSECURITY_ATTRIBUTES lpProcessAttributes, LPSECURITY_ATTRIBUTES lpThreadAttributes, BOOL bInheritHandles, UINT32 dwCreationFlags, PVOID lpEnvironment, PCHAR lpCurrentDirectory, LPSTARTUPINFOA lpStartupInfo, LPPROCESS_INFORMATION lpProcessInformation)
{
    return ((BOOL(STDCALL *)(PCHAR lpApplicationName, PCHAR lpCommandLine, LPSECURITY_ATTRIBUTES lpProcessAttributes, LPSECURITY_ATTRIBUTES lpThreadAttributes, BOOL bInheritHandles, UINT32 dwCreationFlags, PVOID lpEnvironment, PCHAR lpCurrentDirectory, LPSTARTUPINFOA lpStartupInfo, LPPROCESS_INFORMATION lpProcessInformation))ResolveKernel32ExportAddress("CreateProcessA"))(lpApplicationName, lpCommandLine, lpProcessAttributes, lpThreadAttributes, bInheritHandles, dwCreationFlags, lpEnvironment, lpCurrentDirectory, lpStartupInfo, lpProcessInformation);
}

BOOL Kernel32::WriteConsoleW(PVOID hConsoleOutput, const void *lpBuffer, INT32 nNumberOfCharsToWrite, PUINT32 lpNumberOfCharsWritten, LPOVERLAPPED lpOverlapped)
{
    return ((BOOL(STDCALL *)(PVOID hConsoleOutput, const void *lpBuffer, INT32 nNumberOfCharsToWrite, PUINT32 lpNumberOfCharsWritten, LPOVERLAPPED lpOverlapped))ResolveKernel32ExportAddress("WriteConsoleW"))(hConsoleOutput, lpBuffer, nNumberOfCharsToWrite, lpNumberOfCharsWritten, lpOverlapped);
}

BOOL Kernel32::CreatePipe(PPVOID hReadPipe, PPVOID hWritePipe, LPSECURITY_ATTRIBUTES lpPipeAttributes, UINT32 nSize)
{
    return ((BOOL(STDCALL *)(PPVOID hReadPipe, PPVOID hWritePipe, LPSECURITY_ATTRIBUTES lpPipeAttributes, UINT32 nSize))ResolveKernel32ExportAddress("CreatePipe"))(hReadPipe, hWritePipe, lpPipeAttributes, nSize);
}

BOOL Kernel32::SetHandleInformation(PVOID hObject, UINT32 dwMask, UINT32 dwFlags)
{
    return ((BOOL(STDCALL *)(PVOID hObject, UINT32 dwMask, UINT32 dwFlags))ResolveKernel32ExportAddress("SetHandleInformation"))(hObject, dwMask, dwFlags);
}

BOOL Kernel32::WriteFile(PVOID hFile, const void *lpBuffer, INT32 nNumberOfBytesToWrite, PUINT32 lpNumberOfBytesWritten, LPOVERLAPPED lpOverlapped)
{
    return ((BOOL(STDCALL *)(PVOID hFile, const void *lpBuffer, INT32 nNumberOfBytesToWrite, PUINT32 lpNumberOfBytesWritten, LPOVERLAPPED lpOverlapped))ResolveKernel32ExportAddress("WriteFile"))(hFile, lpBuffer, nNumberOfBytesToWrite, lpNumberOfBytesWritten, lpOverlapped);
}

BOOL Kernel32::ReadFile(PVOID hFile, PVOID lpBuffer, INT32 nNumberOfBytesToRead, PUINT32 lpNumberOfBytesRead, LPOVERLAPPED lpOverlapped)
{
    return ((BOOL(STDCALL *)(PVOID hFile, PVOID lpBuffer, INT32 nNumberOfBytesToRead, PUINT32 lpNumberOfBytesRead, LPOVERLAPPED lpOverlapped))ResolveKernel32ExportAddress("ReadFile"))(hFile, lpBuffer, nNumberOfBytesToRead, lpNumberOfBytesRead, lpOverlapped);
}

PVOID Kernel32::FindFirstFileW(PCWCHAR lpFileName, LPWIN32_FIND_DATAW lpFindFileData)
{
    return ((PVOID(STDCALL *)(PCWCHAR lpFileName, LPWIN32_FIND_DATAW lpFindFileData))ResolveKernel32ExportAddress("FindFirstFileW"))(lpFileName, lpFindFileData);
}
BOOL Kernel32::FindNextFileW(PVOID hFindFile, LPWIN32_FIND_DATAW lpFindFileData)
{
    return ((BOOL(STDCALL *)(PVOID hFindFile, LPWIN32_FIND_DATAW lpFindFileData))ResolveKernel32ExportAddress("FindNextFileW"))(hFindFile, lpFindFileData);
}

BOOL Kernel32::FindClose(PVOID hFindFile)
{
    return ((BOOL(STDCALL *)(PVOID hFindFile))ResolveKernel32ExportAddress("FindClose"))(hFindFile);
}
