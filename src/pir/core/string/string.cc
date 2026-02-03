#include "string.h"

// Converts a wide string (UTF-16) to UTF-8
// Returns the number of bytes written (excluding null terminator)
USIZE String::WideToUtf8(PCWCHAR wide, PCHAR utf8, USIZE utf8BufferSize)
{
    if (!wide || !utf8 || utf8BufferSize == 0)
        return 0;

    USIZE utf8Len = 0;

    while (*wide && utf8Len < utf8BufferSize - 4)
    {
        UINT32 ch = *wide++;

        // Handle surrogate pairs for characters > 0xFFFF
        if (ch >= 0xD800 && ch <= 0xDBFF && *wide >= 0xDC00 && *wide <= 0xDFFF)
        {
            ch = 0x10000 + ((ch - 0xD800) << 10) + (*wide++ - 0xDC00);
        }

        if (ch < 0x80)
        {
            utf8[utf8Len++] = (CHAR)ch;
        }
        else if (ch < 0x800)
        {
            utf8[utf8Len++] = (CHAR)(0xC0 | (ch >> 6));
            utf8[utf8Len++] = (CHAR)(0x80 | (ch & 0x3F));
        }
        else if (ch < 0x10000)
        {
            utf8[utf8Len++] = (CHAR)(0xE0 | (ch >> 12));
            utf8[utf8Len++] = (CHAR)(0x80 | ((ch >> 6) & 0x3F));
            utf8[utf8Len++] = (CHAR)(0x80 | (ch & 0x3F));
        }
        else
        {
            utf8[utf8Len++] = (CHAR)(0xF0 | (ch >> 18));
            utf8[utf8Len++] = (CHAR)(0x80 | ((ch >> 12) & 0x3F));
            utf8[utf8Len++] = (CHAR)(0x80 | ((ch >> 6) & 0x3F));
            utf8[utf8Len++] = (CHAR)(0x80 | (ch & 0x3F));
        }
    }

    utf8[utf8Len] = '\0';
    return utf8Len;
}

// Converts a UTF-8 string to wide string (UTF-16)
// Returns the number of wide characters written (excluding null terminator)
USIZE String::Utf8ToWide(PCCHAR utf8, PWCHAR wide, USIZE wideBufferSize)
{
    if (!utf8 || !wide || wideBufferSize == 0)
        return 0;

    USIZE wideLen = 0;

    while (*utf8 && wideLen < wideBufferSize - 2)
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

