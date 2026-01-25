#if defined(PLATFORM_WINDOWS)

#include "console.h"
#include "pal.h"
#include "kernel32.h"
#include "peb.h"

UINT32 Console::Write(const CHAR *text, USIZE length)
{
	PPEB peb = GetCurrentPEB();
	UINT32 numberOfCharsWritten = 0;
	// Call the WriteConsoleA function
	Kernel32::WriteConsoleA(peb->ProcessParameters->StandardOutput, text, length, &numberOfCharsWritten, NULL);
	return numberOfCharsWritten;
}

UINT32 Console::Write(const WCHAR *text, USIZE length)
{
	PPEB peb = GetCurrentPEB();
	UINT32 numberOfCharsWritten = 0;
	// Call the WriteConsoleW function
	Kernel32::WriteConsoleW(peb->ProcessParameters->StandardOutput, text, length, &numberOfCharsWritten, NULL);

	return numberOfCharsWritten;
}
#else

#error Unsupported platform

#endif