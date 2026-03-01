#include "core/string/string.h"
#include "core/types/double.h"

// ============================================================================
// NUMBER CONVERSION IMPLEMENTATIONS (non-constexpr — use DOUBLE operations)
// ============================================================================

USIZE String::FloatToStr(DOUBLE value, Span<CHAR> buffer, UINT8 precision) noexcept
{
	if (buffer.Size() < 2)
		return 0;
	if (precision > 15)
		precision = 15;

	USIZE pos = 0;
	DOUBLE zero = DOUBLE(INT32(0));

	// Handle negative
	if (value < zero)
	{
		if (pos < buffer.Size() - 1)
			buffer[pos++] = '-';
		value = -value;
	}

	// Rounding: add 0.5 / 10^precision
	if (precision > 0)
	{
		DOUBLE scale = DOUBLE(INT32(1));
		for (UINT8 p = 0; p < precision; p++)
			scale = scale * DOUBLE(INT32(10));
		value = value + DOUBLE(INT32(5)) / (scale * DOUBLE(INT32(10)));
	}
	else
	{
		value = value + DOUBLE(INT32(5)) / DOUBLE(INT32(10));
	}

	// Integer part
	UINT64 intPart = (UINT64)(INT64)value;
	DOUBLE fracPart = value - DOUBLE((INT64)intPart);

	CHAR intBuf[24];
	USIZE intLen = UIntToStr(intPart, Span<CHAR>(intBuf));
	for (USIZE i = 0; i < intLen && pos < buffer.Size() - 1; i++)
		buffer[pos++] = intBuf[i];

	// Fractional part
	if (precision > 0 && pos < buffer.Size() - 1)
	{
		buffer[pos++] = '.';

		for (UINT8 p = 0; p < precision && pos < buffer.Size() - 1; p++)
		{
			fracPart = fracPart * DOUBLE(INT32(10));
			INT32 digit = (INT32)fracPart;
			if (digit < 0)
				digit = 0;
			if (digit > 9)
				digit = 9;
			buffer[pos++] = '0' + digit;
			fracPart = fracPart - DOUBLE(digit);
		}

		// Trim trailing zeros
		while (pos > 2 && buffer[pos - 1] == '0' && buffer[pos - 2] != '.')
			pos--;
	}

	buffer[pos] = '\0';
	return pos;
}

Result<DOUBLE, Error> String::StrToFloat(Span<const CHAR> str) noexcept
{
	if (str.Size() == 0)
	{
		return Result<DOUBLE, Error>::Err(Error::String_ParseFloatFailed);
	}

	CHAR buffer[64];
	USIZE copyLen = str.Size() < 63 ? str.Size() : 63;
	for (USIZE i = 0; i < copyLen; i++)
	{
		buffer[i] = str[i];
	}
	buffer[copyLen] = '\0';

	DOUBLE result = DOUBLE::Parse(buffer);
	return Result<DOUBLE, Error>::Ok(result);
}

// ============================================================================
// UTF CONVERSION IMPLEMENTATIONS
// ============================================================================

// Converts a UTF-8 string to wide string (UTF-16)
// Returns the number of wide characters written (excluding null terminator)
USIZE String::Utf8ToWide(PCCHAR utf8, Span<WCHAR> wide)
{
	if (!utf8 || wide.Size() < 3)
	{
		if (wide.Size() > 0)
			wide[0] = L'\0';
		return 0;
	}

	USIZE wideLen = 0;

	while (*utf8 && wideLen + 2 < wide.Size())
	{
		UINT32 ch;
		UINT8 byte = (UINT8)*utf8++;

		if (byte < 0x80)
		{
			ch = byte;
		}
		else if ((byte & 0xE0) == 0xC0)
		{
			ch = (byte & 0x1F) << 6;
			if (*utf8)
				ch |= (*utf8++ & 0x3F);
		}
		else if ((byte & 0xF0) == 0xE0)
		{
			ch = (byte & 0x0F) << 12;
			if (*utf8)
				ch |= (*utf8++ & 0x3F) << 6;
			if (*utf8)
				ch |= (*utf8++ & 0x3F);
		}
		else if ((byte & 0xF8) == 0xF0)
		{
			ch = (byte & 0x07) << 18;
			if (*utf8)
				ch |= (*utf8++ & 0x3F) << 12;
			if (*utf8)
				ch |= (*utf8++ & 0x3F) << 6;
			if (*utf8)
				ch |= (*utf8++ & 0x3F);

			// Encode as surrogate pair for characters > 0xFFFF
			if (ch >= 0x10000)
			{
				ch -= 0x10000;
				wide[wideLen++] = (WCHAR)(0xD800 + (ch >> 10));
				wide[wideLen++] = (WCHAR)(0xDC00 + (ch & 0x3FF));
				continue;
			}
		}
		else
		{
			continue; // Invalid UTF-8 byte, skip
		}

		wide[wideLen++] = (WCHAR)ch;
	}

	wide[wideLen] = L'\0';
	return wideLen;
}
