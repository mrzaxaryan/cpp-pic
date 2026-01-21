/**
 * console.h - Position-Independent Console I/O Interface
 *
 * Provides printf-style console output without depending on:
 *   - C Runtime Library (CRT)
 *   - Standard I/O functions (printf, puts, etc.)
 *   - Dynamic memory allocation (malloc/free)
 *   - .rdata section for format strings
 *
 * The Console class abstracts platform differences between Windows and Linux,
 * providing a unified interface for writing to the console using direct syscalls.
 */

#pragma once

#include "string_formatter.h"  // Printf-style formatting engine
#include "string.h"            // String utilities (length, copy, etc.)

/**
 * Console - Static class providing console I/O operations
 */
class Console
{
private:
	/**
	 * FormatterCallback - Internal callback for character-by-character output
	 *
	 * Used by StringFormatter to emit formatted characters one at a time.
	 * This callback is invoked for each character in the formatted output.
	 */
	template <TCHAR TChar>
	static BOOL FormatterCallback(PVOID context, TChar ch);

public:
	/**
	 * Write - Output narrow (ANSI) string to console
	 */
	static UINT32 Write(const CHAR *text, USIZE length);

	/**
	 * Write - Output wide (Unicode) string to console
	 */
	static UINT32 Write(const WCHAR *text, USIZE length);

	/**
	 * Write - Output null-terminated string to console (template version)
	 *
	 * Convenience wrapper that automatically calculates string length.
	 * Ideal for use with embedded strings.
	 */
	template <TCHAR TChar>
	static UINT32 Write(const TChar *text);

	/**
	 * WriteFormatted - Printf-style formatted output
	 *
	 * Supports standard printf format specifiers:
	 *   %d    - Signed decimal integer
	 *   %u    - Unsigned decimal integer
	 *   %ld   - Long signed decimal integer
	 *   %X    - Uppercase hexadecimal
	 *   %x    - Lowercase hexadecimal
	 *   %f    - Floating-point (default precision)
	 *   %.Nf  - Floating-point with N decimal places
	 *   %c    - Single character
	 *   %s    - Narrow string (CHAR*)
	 *   %ls   - Wide string (WCHAR*)
	 *   %p    - Pointer (hexadecimal)
	 */
	template <TCHAR TChar>
	static UINT32 WriteFormatted(const TChar *format, ...);

	/**
	 * WriteFormattedV - Printf-style formatted output (va_list version)
	 *
	 * Same as WriteFormatted but accepts a va_list instead of variadic args.
	 * Useful when you're already in a variadic function and want to forward args.
	 */
	template <TCHAR TChar>
	static UINT32 WriteFormattedV(const TChar *format, VA_LIST args);
};

/**
 * Write<TChar> - Null-terminated string output (inline implementation)
 */
template <TCHAR TChar>
UINT32 Console::Write(const TChar *text)
{
	// Calculate string length and forward to length-based overload
	// String::Length() walks the string until it finds the null terminator
	return Write(text, String::Length(text));
}

/**
 * FormatterCallback<TChar> - Character emission callback (inline implementation)
 *
 * StringFormatter calls this function for each formatted character.
 * We simply write each character directly to the console.
 */
template <TCHAR TChar>
BOOL Console::FormatterCallback(PVOID context, TChar ch)
{
	(VOID)context;

	// Write single character to console
	// Return value: TRUE = success, FALSE = error
	return Write(&ch, 1);
}

/**
 * WriteFormattedV<TChar> - Variadic formatting implementation
 *
 * This is the core formatting function. WriteFormatted() is just a thin
 * wrapper that sets up the va_list and calls this function.
 */
template <TCHAR TChar>
UINT32 Console::WriteFormattedV(const TChar *format, VA_LIST args)
{
	// Perform position-independent relocation of the callback function pointer
	// Without this, the callback address would be incorrect in PIC environments
	auto fixed = (BOOL (*)(PVOID, TChar))PerformRelocation((PVOID)FormatterCallback<TChar>);

	// Delegate to StringFormatter which handles all format specifier parsing
	return StringFormatter::FormatV(fixed, NULL, format, args);
}

/**
 * WriteFormatted<TChar> - Variadic formatting wrapper
 */
template <TCHAR TChar>
UINT32 Console::WriteFormatted(const TChar *format, ...)
{
	VA_LIST args;
	VA_START(args, format);
	INT32 written = WriteFormattedV(format, args);
	VA_END(args);
	return written;
}
