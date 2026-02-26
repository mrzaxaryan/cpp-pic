#include "string.h"
#include "double.h"

// ============================================================================
// NUMBER CONVERSION IMPLEMENTATIONS
// ============================================================================

USIZE String::IntToStr(INT64 value, CHAR *buffer, USIZE bufSize) noexcept
{
	if (!buffer || bufSize < 2)
		return 0;

	CHAR temp[24];
	USIZE pos = 0;
	BOOL negative = false;

	if (value < 0)
	{
		negative = true;
	}

	// Use unsigned to handle INT64_MIN correctly (negating INT64_MIN is UB)
	UINT64 uval = negative ? (UINT64)0 - (UINT64)value : (UINT64)value;

	if (uval == 0)
	{
		temp[pos++] = '0';
	}
	else
	{
		while (uval > 0 && pos < 22)
		{
			temp[pos++] = '0' + (CHAR)(uval % 10);
			uval = uval / 10;
		}
	}

	if (negative && pos < 22)
	{
		temp[pos++] = '-';
	}

	USIZE copyLen = pos < bufSize - 1 ? pos : bufSize - 1;
	for (USIZE i = 0; i < copyLen; i++)
	{
		buffer[i] = temp[pos - 1 - i];
	}
	buffer[copyLen] = '\0';
	return copyLen;
}

USIZE String::UIntToStr(UINT64 value, CHAR *buffer, USIZE bufSize) noexcept
{
	if (!buffer || bufSize < 2)
		return 0;

	CHAR temp[24];
	USIZE pos = 0;

	if (value == 0)
	{
		temp[pos++] = '0';
	}
	else
	{
		while (value > 0 && pos < 22)
		{
			temp[pos++] = '0' + (CHAR)(value % 10);
			value = value / 10;
		}
	}

	USIZE copyLen = pos < bufSize - 1 ? pos : bufSize - 1;
	for (USIZE i = 0; i < copyLen; i++)
	{
		buffer[i] = temp[pos - 1 - i];
	}
	buffer[copyLen] = '\0';
	return copyLen;
}

USIZE String::FloatToStr(DOUBLE value, CHAR *buffer, USIZE bufSize, UINT8 precision) noexcept
{
	if (!buffer || bufSize < 2)
		return 0;
	if (precision > 15)
		precision = 15;

	USIZE pos = 0;
	DOUBLE zero = DOUBLE(INT32(0));

	// Handle negative
	if (value < zero)
	{
		if (pos < bufSize - 1)
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
	USIZE intLen = UIntToStr(intPart, intBuf, sizeof(intBuf));
	for (USIZE i = 0; i < intLen && pos < bufSize - 1; i++)
		buffer[pos++] = intBuf[i];

	// Fractional part
	if (precision > 0 && pos < bufSize - 1)
	{
		buffer[pos++] = '.';

		for (UINT8 p = 0; p < precision && pos < bufSize - 1; p++)
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

Result<INT64, Error> String::ParseInt64(const CHAR *str, USIZE len) noexcept
{
	if (!str || len == 0)
	{
		return Result<INT64, Error>::Err(Error::String_ParseIntFailed);
	}

	USIZE i = 0;
	BOOL negative = false;

	while (i < len && (str[i] == ' ' || str[i] == '\t'))
	{
		i++;
	}

	if (i < len && str[i] == '-')
	{
		negative = true;
		i++;
	}
	else if (i < len && str[i] == '+')
	{
		i++;
	}

	UINT64 value = 0;
	BOOL hasDigits = false;
	constexpr UINT64 maxPositive = 0x7FFFFFFFFFFFFFFFULL;
	constexpr UINT64 maxNegative = 0x8000000000000000ULL;
	UINT64 limit = negative ? maxNegative : maxPositive;

	while (i < len && str[i] >= '0' && str[i] <= '9')
	{
		UINT64 digit = (UINT64)(str[i] - '0');
		if (value > (limit - digit) / 10)
			return Result<INT64, Error>::Err(Error::String_ParseIntFailed);
		value = value * 10 + digit;
		hasDigits = true;
		i++;
	}

	if (!hasDigits)
	{
		return Result<INT64, Error>::Err(Error::String_ParseIntFailed);
	}

	INT64 result = negative ? -(INT64)value : (INT64)value;
	return Result<INT64, Error>::Ok(result);
}

INT64 String::ParseInt64(PCCHAR str) noexcept
{
	if (!str)
		return 0;
	auto r = ParseInt64(str, Length(str));
	return r.IsOk() ? r.Value() : 0;
}

Result<DOUBLE, Error> String::StrToFloat(const CHAR *str, USIZE len) noexcept
{
	if (!str || len == 0)
	{
		return Result<DOUBLE, Error>::Err(Error::String_ParseFloatFailed);
	}

	CHAR buffer[64];
	USIZE copyLen = len < 63 ? len : 63;
	for (USIZE i = 0; i < copyLen; i++)
	{
		buffer[i] = str[i];
	}
	buffer[copyLen] = '\0';

	DOUBLE result = DOUBLE::Parse(buffer);
	return Result<DOUBLE, Error>::Ok(result);
}

UINT32 String::ParseHex(PCCHAR str) noexcept
{
	UINT32 result = 0;
	while (*str != '\0')
	{
		CHAR c = *str;
		UINT32 digit = 0;

		if (c >= '0' && c <= '9')
		{
			digit = c - '0';
		}
		else if (c >= 'a' && c <= 'f')
		{
			digit = 10 + (c - 'a');
		}
		else if (c >= 'A' && c <= 'F')
		{
			digit = 10 + (c - 'A');
		}
		else
		{
			break;
		}

		result = (result << 4) | digit;
		str++;
	}
	return result;
}

PCHAR String::WriteDecimal(PCHAR buffer, UINT32 num) noexcept
{
	USIZE len = UIntToStr((UINT64)num, buffer, 12);
	return buffer + len;
}

PCHAR String::WriteHex(PCHAR buffer, UINT32 num, BOOL uppercase) noexcept
{
	if (num == 0)
	{
		buffer[0] = '0';
		buffer[1] = '\0';
		return buffer + 1;
	}

	CHAR temp[9];
	INT32 i = 0;
	CHAR baseChar = uppercase ? 'A' : 'a';

	while (num > 0)
	{
		UINT32 digit = num & 0xF;
		temp[i++] = (digit < 10) ? ('0' + digit) : (baseChar + digit - 10);
		num >>= 4;
	}

	INT32 j = 0;
	while (i > 0)
	{
		buffer[j++] = temp[--i];
	}
	buffer[j] = '\0';
	return buffer + j;
}

// ============================================================================
// UTF CONVERSION IMPLEMENTATIONS
// ============================================================================

// Converts a UTF-8 string to wide string (UTF-16)
// Returns the number of wide characters written (excluding null terminator)
USIZE String::Utf8ToWide(PCCHAR utf8, PWCHAR wide, USIZE wideBufferSize)
{
	if (!utf8 || !wide || wideBufferSize < 3)
	{
		if (wide && wideBufferSize > 0)
			wide[0] = L'\0';
		return 0;
	}

	USIZE wideLen = 0;

	while (*utf8 && wideLen + 2 < wideBufferSize)
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
