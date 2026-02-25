#include "console.h"
#include "platform.h"
#include "ntdll.h"
#include "peb.h"
#include "encoding/utf16.h"

UINT32 Console::Write(const CHAR *text, USIZE length)
{
	PPEB peb = GetCurrentPEB();
	IO_STATUS_BLOCK ioStatusBlock;
	Memory::Zero(&ioStatusBlock, sizeof(IO_STATUS_BLOCK));
	(void)NTDLL::ZwWriteFile(peb->ProcessParameters->StandardOutput, nullptr, nullptr, nullptr, &ioStatusBlock, (PVOID)text, length, nullptr, nullptr);
	return (UINT32)ioStatusBlock.Information;
}

UINT32 Console::Write(const WCHAR *text, USIZE length)
{
	constexpr USIZE BUFFER_SIZE = 256;
	CHAR utf8[BUFFER_SIZE];
	UINT32 totalWritten = 0;

	USIZE inputIndex = 0;
	USIZE bufIndex = 0;

	while (inputIndex < length)
	{
		CHAR bytes[4];
		USIZE n = UTF16::CodepointToUTF8(text, length, inputIndex, bytes);

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
