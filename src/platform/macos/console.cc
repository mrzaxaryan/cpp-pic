#include "console.h"
#include "syscall.h"
#include "system.h"
#include "encoding/utf16.h"

// Write ANSI/ASCII string to console (straightforward)
UINT32 Console::Write(const CHAR *text, USIZE length)
{
	SSIZE result = System::Call(SYS_WRITE, STDOUT_FILENO, (USIZE)text, length);
	return (result >= 0) ? (UINT32)result : 0;
}

// Write wide string to console (WCHAR is 4-byte UTF-32 on macOS)
UINT32 Console::Write(const WCHAR *text, USIZE length)
{
	constexpr USIZE BUFFER_SIZE = 256;
	CHAR utf8[BUFFER_SIZE];
	UINT32 totalWritten = 0;
	USIZE bufIndex = 0;

	for (USIZE i = 0; i < length; i++)
	{
		CHAR bytes[4];
		USIZE n = UTF16::CodepointToUTF8Bytes((UINT32)text[i], bytes);

		if (bufIndex + n > BUFFER_SIZE)
		{
			totalWritten += Write(utf8, bufIndex);
			bufIndex = 0;
		}

		for (USIZE j = 0; j < n; j++)
			utf8[bufIndex++] = bytes[j];
	}

	if (bufIndex > 0)
		totalWritten += Write(utf8, bufIndex);

	return totalWritten;
}
