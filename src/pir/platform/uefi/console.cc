/**
 * console.cc - UEFI Console I/O Implementation
 *
 * Provides Console::Write using EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL.
 * UEFI natively uses CHAR16 (UTF-16), which maps to our WCHAR.
 */

#include "console.h"
#include "efi_context.h"

UINT32 Console::Write(const WCHAR *text, USIZE length)
{
	if (text == NULL || length == 0)
		return 0;

	EFI_CONTEXT *ctx = GetEfiContext();
	EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL *conOut = ctx->SystemTable->ConOut;

	if (conOut == NULL)
		return 0;

	// Output in chunks with null terminator
	constexpr USIZE BUFFER_SIZE = 256;
	WCHAR buffer[BUFFER_SIZE];
	UINT32 totalWritten = 0;

	while (length > 0)
	{
		USIZE chunk = (length < BUFFER_SIZE - 1) ? length : BUFFER_SIZE - 1;
		for (USIZE i = 0; i < chunk; i++)
			buffer[i] = text[i];
		buffer[chunk] = L'\0';
		conOut->OutputString(conOut, buffer);
		totalWritten += chunk;
		text += chunk;
		length -= chunk;
	}

	return totalWritten;
}

UINT32 Console::Write(const CHAR *text, USIZE length)
{
	if (text == NULL || length == 0)
		return 0;

	EFI_CONTEXT *ctx = GetEfiContext();
	EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL *conOut = ctx->SystemTable->ConOut;

	if (conOut == NULL)
		return 0;

	// Convert to wide and output in chunks
	constexpr USIZE BUFFER_SIZE = 256;
	WCHAR buffer[BUFFER_SIZE];
	UINT32 totalWritten = 0;

	while (length > 0)
	{
		USIZE chunk = (length < BUFFER_SIZE - 1) ? length : BUFFER_SIZE - 1;
		for (USIZE i = 0; i < chunk; i++)
			buffer[i] = (WCHAR)(UINT8)text[i];
		buffer[chunk] = L'\0';
		conOut->OutputString(conOut, buffer);
		totalWritten += chunk;
		text += chunk;
		length -= chunk;
	}

	return totalWritten;
}
