#if defined(PLATFORM_UEFI)

#include "console.h"
#include "uefi/efi_system_table.h"
#include "string.h"
#include "memory.h"

// ANSI to UEFI color mapping
// ANSI colors: 30-37 (foreground), 40-47 (background)
// UEFI colors: 0x00-0x0F (foreground), 0x00-0x70 (background shifted)
// Using switch to avoid static arrays in .rdata section (PIC requirement)
static UINTN AnsiToUefiForeground(INT32 ansiColor)
{
    // ANSI: 30=black, 31=red, 32=green, 33=yellow, 34=blue, 35=magenta, 36=cyan, 37=white
    // UEFI: 0=black, 1=blue, 2=green, 3=cyan, 4=red, 5=magenta, 6=brown, 7=lightgray
    switch (ansiColor)
    {
    case 30:
        return EFI_BLACK;
    case 31:
        return EFI_RED;
    case 32:
        return EFI_GREEN;
    case 33:
        return EFI_YELLOW;
    case 34:
        return EFI_BLUE;
    case 35:
        return EFI_MAGENTA;
    case 36:
        return EFI_CYAN;
    case 37:
        return EFI_LIGHTGRAY;
    // Bright colors (90-97)
    case 90:
        return EFI_DARKGRAY;
    case 91:
        return EFI_LIGHTRED;
    case 92:
        return EFI_LIGHTGREEN;
    case 93:
        return EFI_YELLOW;
    case 94:
        return EFI_LIGHTBLUE;
    case 95:
        return EFI_LIGHTMAGENTA;
    case 96:
        return EFI_LIGHTCYAN;
    case 97:
        return EFI_WHITE;
    default:
        return EFI_LIGHTGRAY;
    }
}

// Parse ANSI escape sequence and apply UEFI attributes
// Returns number of characters consumed from the escape sequence
template <typename TChar>
static USIZE ProcessAnsiEscape(const TChar *text, USIZE remaining)
{
    if (remaining < 2 || text[0] != 0x1B) // ESC character
        return 0;

    if (text[1] != '[') // CSI sequence
        return 0;

    // Parse the escape sequence: ESC [ <params> m
    USIZE pos = 2;
    INT32 params[8] = {0};
    INT32 paramCount = 0;
    INT32 currentParam = 0;
    BOOL hasParam = FALSE;

    while (pos < remaining && paramCount < 8)
    {
        TChar ch = text[pos];
        if (ch >= '0' && ch <= '9')
        {
            currentParam = currentParam * 10 + (ch - '0');
            hasParam = TRUE;
            pos++;
        }
        else if (ch == ';')
        {
            params[paramCount++] = currentParam;
            currentParam = 0;
            hasParam = FALSE;
            pos++;
        }
        else if (ch == 'm')
        {
            // End of SGR sequence
            if (hasParam || paramCount == 0)
                params[paramCount++] = currentParam;
            pos++;
            break;
        }
        else
        {
            // Unknown character, skip the sequence
            pos++;
            break;
        }
    }

    // Apply the attributes
    if (gST && gST->ConOut)
    {
        UINTN attr = EFI_LIGHTGRAY | EFI_BACKGROUND_BLACK; // Default

        for (INT32 i = 0; i < paramCount; i++)
        {
            INT32 p = params[i];
            if (p == 0)
            {
                // Reset
                attr = EFI_LIGHTGRAY | EFI_BACKGROUND_BLACK;
            }
            else if (p >= 30 && p <= 37)
            {
                // Foreground color
                attr = (attr & 0xF0) | AnsiToUefiForeground(p);
            }
            else if (p >= 90 && p <= 97)
            {
                // Bright foreground color
                attr = (attr & 0xF0) | AnsiToUefiForeground(p);
            }
            else if (p >= 40 && p <= 47)
            {
                // Background color (shift by 4 bits)
                attr = (attr & 0x0F) | (AnsiToUefiForeground(p - 10) << 4);
            }
            else if (p == 1)
            {
                // Bold/bright - use bright version of current color
                UINTN fg = attr & 0x0F;
                if (fg < 8)
                    attr = (attr & 0xF0) | (fg | 0x08);
            }
        }

        gST->ConOut->SetAttribute(gST->ConOut, attr);
    }

    return pos;
}

// Output a buffer of text, handling ANSI escapes
template <typename TChar>
static UINT32 OutputWithAnsiHandling(const TChar *text, USIZE count)
{
    if (!text || !count || !gST || !gST->ConOut)
        return 0;

    const USIZE CHUNK_SIZE = 256;
    WCHAR buffer[CHUNK_SIZE + 1];
    UINT32 totalWritten = 0;
    USIZE bufferPos = 0;

    for (USIZE i = 0; i < count; i++)
    {
        // Check for ESC character (start of ANSI sequence)
        if (text[i] == 0x1B)
        {
            // Flush any pending text first
            if (bufferPos > 0)
            {
                buffer[bufferPos] = 0;
                gST->ConOut->OutputString(gST->ConOut, buffer);
                totalWritten += (UINT32)bufferPos;
                bufferPos = 0;
            }

            // Process the ANSI escape sequence
            USIZE consumed = ProcessAnsiEscape(text + i, count - i);
            if (consumed > 0)
            {
                i += consumed - 1; // -1 because loop will increment
                continue;
            }
        }

        // Regular character - add to buffer
        buffer[bufferPos++] = (WCHAR)text[i];

        // Flush buffer if full
        if (bufferPos >= CHUNK_SIZE)
        {
            buffer[bufferPos] = 0;
            gST->ConOut->OutputString(gST->ConOut, buffer);
            totalWritten += (UINT32)bufferPos;
            bufferPos = 0;
        }
    }

    // Flush remaining text
    if (bufferPos > 0)
    {
        buffer[bufferPos] = 0;
        gST->ConOut->OutputString(gST->ConOut, buffer);
        totalWritten += (UINT32)bufferPos;
    }

    return totalWritten;
}

UINT32 Console::Write(const CHAR *output, USIZE outputLength)
{
    return OutputWithAnsiHandling(output, outputLength);
}

UINT32 Console::Write(const WCHAR *text, USIZE wcharCount)
{
    return OutputWithAnsiHandling(text, wcharCount);
}

#endif
