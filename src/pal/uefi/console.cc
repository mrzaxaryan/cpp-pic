/**
 * console.cc - UEFI Console I/O Implementation
 *
 * Provides Console::Write using EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL.
 * UEFI natively uses CHAR16 (UTF-16), which maps to our WCHAR.
 */

#include "console.h"
#include "efi_context.h"
#include "memory.h"

/**
 * Console::Write (WCHAR) - Output wide string to UEFI console
 *
 * UEFI natively uses CHAR16 (identical to our WCHAR with -fshort-wchar),
 * so this is the primary output path.
 *
 * @param text   - Pointer to wide character string
 * @param length - Number of characters to write
 * @return Number of characters written
 */
UINT32 Console::Write(const WCHAR *text, USIZE length)
{
	if (text == NULL || length == 0)
		return 0;

	EFI_CONTEXT *ctx = GetEfiContext();
	EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL *conOut = ctx->SystemTable->ConOut;

	if (conOut == NULL)
		return 0;

	// UEFI OutputString expects null-terminated string
	// We chunk the output to avoid large stack allocations
	constexpr USIZE BUFFER_SIZE = 256;
	WCHAR buffer[BUFFER_SIZE];

	USIZE remaining = length;
	UINT32 totalWritten = 0;
	const WCHAR *ptr = text;

	while (remaining > 0)
	{
		USIZE chunk = (remaining > BUFFER_SIZE - 1) ? BUFFER_SIZE - 1 : remaining;
		Memory::Copy(buffer, ptr, chunk * sizeof(WCHAR));
		buffer[chunk] = L'\0';
		conOut->OutputString(conOut, buffer);
		totalWritten += chunk;
		remaining -= chunk;
		ptr += chunk;
	}

	return totalWritten;
}

/**
 * Console::Write (CHAR) - Output narrow string to UEFI console
 *
 * Converts ASCII/Latin-1 to CHAR16 (UTF-16) for UEFI output.
 *
 * @param text   - Pointer to narrow character string
 * @param length - Number of characters to write
 * @return Number of characters written
 */
UINT32 Console::Write(const CHAR *text, USIZE length)
{
	if (text == NULL || length == 0)
		return 0;

	EFI_CONTEXT *ctx = GetEfiContext();
	EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL *conOut = ctx->SystemTable->ConOut;

	if (conOut == NULL)
		return 0;

	// Convert to wide string in chunks
	constexpr USIZE BUFFER_SIZE = 256;
	WCHAR buffer[BUFFER_SIZE];

	USIZE remaining = length;
	UINT32 totalWritten = 0;
	const CHAR *ptr = text;

	while (remaining > 0)
	{
		USIZE chunk = (remaining > BUFFER_SIZE - 1) ? BUFFER_SIZE - 1 : remaining;

		// Convert narrow to wide
		for (USIZE i = 0; i < chunk; i++)
		{
			buffer[i] = (WCHAR)(UINT8)ptr[i];
		}
		buffer[chunk] = L'\0';

		// Output
		conOut->OutputString(conOut, buffer);

		totalWritten += chunk;
		remaining -= chunk;
		ptr += chunk;
	}

	return totalWritten;
}
