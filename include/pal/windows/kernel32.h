#if defined(PLATFORM_WINDOWS)

#pragma once

#include "windows_types.h"
#include "pal.h"
#include "djb2.h"

// Kernel32 API Wrappers
class Kernel32
{
private:
	/* data */
public:
	static BOOL WriteConsoleA(PVOID hConsoleOutput, const void *lpBuffer, INT32 nNumberOfCharsToWrite, PUINT32 lpNumberOfCharsWritten, LPOVERLAPPED lpOverlapped);
	static BOOL WriteConsoleW(PVOID hConsoleOutput, const void *lpBuffer, INT32 nNumberOfCharsToWrite, PUINT32 lpNumberOfCharsWritten, LPOVERLAPPED lpOverlapped);
};

#endif // PLATFORM_WINDOWS