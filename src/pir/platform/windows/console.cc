#include "console.h"
#include "platform.h"
#include "ntdll.h"
#include "peb.h"

UINT32 Console::Write(const CHAR *text, USIZE length)
{
	PPEB peb = GetCurrentPEB();
	IO_STATUS_BLOCK ioStatusBlock;
	Memory::Zero(&ioStatusBlock, sizeof(IO_STATUS_BLOCK));
	NTDLL::NtWriteFile(peb->ProcessParameters->StandardOutput, NULL, NULL, NULL, &ioStatusBlock, (PVOID)text, length, NULL, NULL);
	return (UINT32)ioStatusBlock.Information;
}

// Write wide string to console (convert UTF-16 to UTF-8 first)
// This ensures ANSI escape codes work properly in terminals and piped output
UINT32 Console::Write(const WCHAR *text, USIZE length)
{
	PPEB peb = GetCurrentPEB();

	// UTF-16 to UTF-8 conversion buffer (stack-allocated)
	// Each UTF-16 character can become up to 4 UTF-8 bytes
	constexpr USIZE BUFFER_SIZE = 1024;
	CHAR utf8Buffer[BUFFER_SIZE];
	UINT32 totalWritten = 0;

	for (USIZE i = 0; i < length; )
	{
		USIZE utf8Pos = 0;

		// Convert as many characters as fit in buffer
		while (i < length && utf8Pos < BUFFER_SIZE - 4)
		{
			UINT32 codepoint = text[i++];

			// Handle UTF-16 surrogate pairs
			if (codepoint >= 0xD800 && codepoint <= 0xDBFF && i < length)
			{
				UINT32 low = text[i];
				if (low >= 0xDC00 && low <= 0xDFFF)
				{
					codepoint = 0x10000 + ((codepoint & 0x3FF) << 10) + (low & 0x3FF);
					i++;
				}
			}

			// Convert codepoint to UTF-8
			if (codepoint < 0x80)
			{
				// 1-byte sequence (ASCII)
				utf8Buffer[utf8Pos++] = (CHAR)codepoint;
			}
			else if (codepoint < 0x800)
			{
				// 2-byte sequence
				utf8Buffer[utf8Pos++] = (CHAR)(0xC0 | (codepoint >> 6));
				utf8Buffer[utf8Pos++] = (CHAR)(0x80 | (codepoint & 0x3F));
			}
			else if (codepoint < 0x10000)
			{
				// 3-byte sequence
				utf8Buffer[utf8Pos++] = (CHAR)(0xE0 | (codepoint >> 12));
				utf8Buffer[utf8Pos++] = (CHAR)(0x80 | ((codepoint >> 6) & 0x3F));
				utf8Buffer[utf8Pos++] = (CHAR)(0x80 | (codepoint & 0x3F));
			}
			else if (codepoint < 0x110000)
			{
				// 4-byte sequence
				utf8Buffer[utf8Pos++] = (CHAR)(0xF0 | (codepoint >> 18));
				utf8Buffer[utf8Pos++] = (CHAR)(0x80 | ((codepoint >> 12) & 0x3F));
				utf8Buffer[utf8Pos++] = (CHAR)(0x80 | ((codepoint >> 6) & 0x3F));
				utf8Buffer[utf8Pos++] = (CHAR)(0x80 | (codepoint & 0x3F));
			}
		}

		// Write the converted UTF-8 buffer
		if (utf8Pos > 0)
		{
			IO_STATUS_BLOCK ioStatusBlock;
			Memory::Zero(&ioStatusBlock, sizeof(IO_STATUS_BLOCK));
			NTDLL::NtWriteFile(peb->ProcessParameters->StandardOutput, NULL, NULL, NULL, &ioStatusBlock, (PVOID)utf8Buffer, utf8Pos, NULL, NULL);
			if (ioStatusBlock.Information > 0)
				totalWritten += (UINT32)ioStatusBlock.Information;
		}
	}

	return totalWritten;
}