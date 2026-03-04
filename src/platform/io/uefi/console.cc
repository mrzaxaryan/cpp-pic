/**
 * console.cc - UEFI Console I/O Implementation
 *
 * Provides Console::Write using EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL.
 * UEFI natively uses CHAR16 (UTF-16), which maps to our WCHAR.
 */

#include "platform/io/console.h"
#include "platform/common/uefi/efi_context.h"

UINT32 Console::Write(Span<const WCHAR> text)
{
	if (text.Data() == nullptr || text.Size() == 0)
		return 0;

	EFI_CONTEXT *ctx = GetEfiContext();
	EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL *conOut = ctx->SystemTable->ConOut;

	if (conOut == nullptr)
		return 0;

	// Output in chunks with null terminator
	constexpr USIZE BUFFER_SIZE = 256;
	WCHAR buffer[BUFFER_SIZE];
	UINT32 totalWritten = 0;
	USIZE offset = 0;

	while (offset < text.Size())
	{
		USIZE remaining = text.Size() - offset;
		USIZE chunk = (remaining < BUFFER_SIZE - 1) ? remaining : BUFFER_SIZE - 1;
		for (USIZE i = 0; i < chunk; i++)
			buffer[i] = text[offset + i];
		buffer[chunk] = L'\0';
		conOut->OutputString(conOut, buffer);
		totalWritten += chunk;
		offset += chunk;
	}

	return totalWritten;
}

UINT32 Console::Write(Span<const CHAR> text)
{
	if (text.Data() == nullptr || text.Size() == 0)
		return 0;

	EFI_CONTEXT *ctx = GetEfiContext();
	EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL *conOut = ctx->SystemTable->ConOut;

	if (conOut == nullptr)
		return 0;

	// Convert to wide and output in chunks
	constexpr USIZE BUFFER_SIZE = 256;
	WCHAR buffer[BUFFER_SIZE];
	UINT32 totalWritten = 0;
	USIZE offset = 0;

	while (offset < text.Size())
	{
		USIZE remaining = text.Size() - offset;
		USIZE chunk = (remaining < BUFFER_SIZE - 1) ? remaining : BUFFER_SIZE - 1;
		for (USIZE i = 0; i < chunk; i++)
			buffer[i] = (WCHAR)(UINT8)text[offset + i];
		buffer[chunk] = L'\0';
		conOut->OutputString(conOut, buffer);
		totalWritten += chunk;
		offset += chunk;
	}

	return totalWritten;
}
