#include "kernel32.h"

#define ResolveKernel32ExportAddress(functionName) ResolveExportAddressFromPebModule(Djb2::HashCompileTime(L"kernel32.dll"), Djb2::HashCompileTime(functionName))

BOOL Kernel32::WriteConsoleA(PVOID hConsoleOutput, const void *lpBuffer, INT32 nNumberOfCharsToWrite, PUINT32 lpNumberOfCharsWritten, LPOVERLAPPED lpOverlapped)
{
    return ((BOOL(STDCALL *)(PVOID hConsoleOutput, const void *lpBuffer, INT32 nNumberOfCharsToWrite, PUINT32 lpNumberOfCharsWritten, LPOVERLAPPED lpOverlapped))ResolveKernel32ExportAddress("WriteConsoleA"))(hConsoleOutput, lpBuffer, nNumberOfCharsToWrite, lpNumberOfCharsWritten, lpOverlapped);
}

BOOL Kernel32::WriteConsoleW(PVOID hConsoleOutput, const void *lpBuffer, INT32 nNumberOfCharsToWrite, PUINT32 lpNumberOfCharsWritten, LPOVERLAPPED lpOverlapped)
{
    return ((BOOL(STDCALL *)(PVOID hConsoleOutput, const void *lpBuffer, INT32 nNumberOfCharsToWrite, PUINT32 lpNumberOfCharsWritten, LPOVERLAPPED lpOverlapped))ResolveKernel32ExportAddress("WriteConsoleW"))(hConsoleOutput, lpBuffer, nNumberOfCharsToWrite, lpNumberOfCharsWritten, lpOverlapped);
}
