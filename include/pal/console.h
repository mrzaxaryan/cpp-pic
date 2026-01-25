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
 *
 * DESIGN PHILOSOPHY:
 *   ✓ Zero dependencies - calls kernel directly
 *   ✓ Position independent - works at any memory address
 *   ✓ Stack-based - no heap allocations
 *   ✓ Type-safe - C++ templates for compile-time dispatch
 *
 * PLATFORM IMPLEMENTATION:
 *   Windows: Uses NtDll syscalls to WriteConsoleW/WriteFile
 *   Linux:   Uses write() syscall directly (syscall number 1 on x64)
 *
 * USAGE EXAMPLE:
 *   Console::Write<WCHAR>(L"Hello"_embed);
 *   Console::WriteFormatted<WCHAR>(L"Value: %d\n"_embed, 42);
 */

#pragma once

#include "string_formatter.h"  // Printf-style formatting engine
#include "string.h"            // String utilities (length, copy, etc.)
#include "pal.h"          // PerformRelocation

/**
 * Console - Static class providing console I/O operations
 *
 * All methods are static - no instance needed. This is by design:
 *   1. No global state to initialize
 *   2. No vtable in .rdata
 *   3. Direct function calls (no virtual dispatch)
 *   4. Simplifies position-independent code
 */
class Console
{
private:
	/**
	 * FormatterCallback - Internal callback for character-by-character output
	 *
	 * Used by StringFormatter to emit formatted characters one at a time.
	 * This callback is invoked for each character in the formatted output.
	 *
	 * @param context - User context (unused, reserved for future use)
	 * @param ch      - Character to write to console
	 * @return TRUE on success, FALSE on error
	 *
	 * TEMPLATE PARAMETER:
	 *   TChar - Character type (CHAR or WCHAR) determined at compile-time
	 *
	 * POSITION-INDEPENDENT NOTE:
	 *   The function pointer passed to StringFormatter must be relocated
	 *   using PerformRelocation() to work in position-independent code.
	 */
	template <TCHAR TChar>
	static BOOL FormatterCallback(PVOID context, TChar ch);

public:
	/**
	 * Write - Output narrow (ANSI) string to console
	 *
	 * Platform Behavior:
	 *   Windows: Converts to UTF-16 and calls WriteConsoleW
	 *   Linux:   Writes directly via write(STDOUT_FILENO, text, length)
	 *
	 * @param text   - Pointer to narrow character string
	 * @param length - Number of characters to write
	 * @return Number of characters written, 0 on error
	 *
	 * SYSCALL IMPLEMENTATION:
	 *   Windows: NtDll!NtWriteFile or Kernel32!WriteConsoleA
	 *   Linux:   syscall(__NR_write, 1, text, length)
	 */
	static UINT32 Write(const CHAR *text, USIZE length);

	/**
	 * Write - Output wide (Unicode) string to console
	 *
	 * Platform Behavior:
	 *   Windows: Calls WriteConsoleW directly (native Unicode support)
	 *   Linux:   Converts UTF-16 → UTF-8, then write() syscall
	 *
	 * @param text   - Pointer to wide character string
	 * @param length - Number of characters to write
	 * @return Number of characters written, 0 on error
	 *
	 * UNICODE HANDLING:
	 *   - Windows: Full Unicode support including emoji, CJK, etc.
	 *   - Linux:   Converts to UTF-8 (handles surrogate pairs correctly)
	 *
	 * IMPORTANT:
	 *   Length is in characters, not bytes!
	 *   For WCHAR, each character is 2 bytes (UTF-16 code unit)
	 */
	static UINT32 Write(const WCHAR *text, USIZE length);

	/**
	 * Write - Output null-terminated string to console (template version)
	 *
	 * Convenience wrapper that automatically calculates string length.
	 * Ideal for use with embedded strings.
	 *
	 * @param text - Null-terminated string (CHAR* or WCHAR*)
	 * @return Number of characters written
	 *
	 * TEMPLATE PARAMETER:
	 *   TChar - Automatically deduced from argument (CHAR or WCHAR)
	 *
	 * USAGE:
	 *   Console::Write<WCHAR>(L"Hello"_embed);    // Wide string
	 *   Console::Write<CHAR>("Hello"_embed);      // Narrow string
	 *
	 * PERFORMANCE NOTE:
	 *   Calls String::Length() to find null terminator - O(n) operation.
	 *   If you already know the length, use the length-based overload instead.
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
	 *
	 * @param format - Format string with embedded specifiers
	 * @param ...    - Variable arguments matching format specifiers
	 * @return Number of characters written
	 *
	 * TEMPLATE PARAMETER:
	 *   TChar - Character type for format string (CHAR or WCHAR)
	 *
	 * USAGE:
	 *   Console::WriteFormatted<WCHAR>(
	 *       L"Value: %d, Pi: %.5f\n"_embed,
	 *       42,
	 *       3.14159_embed
	 *   );
	 *
	 * POSITION-INDEPENDENT IMPLEMENTATION:
	 *   - Format string embedded in .text (no .rdata)
	 *   - Floating-point constants as immediates
	 *   - Stack-based buffer for formatting
	 *   - No heap allocations
	 */
	template <TCHAR TChar>
	static UINT32 WriteFormatted(const TChar *format, ...);

	/**
	 * WriteFormattedV - Printf-style formatted output (va_list version)
	 *
	 * Same as WriteFormatted but accepts a va_list instead of variadic args.
	 * Useful when you're already in a variadic function and want to forward args.
	 *
	 * @param format - Format string with embedded specifiers
	 * @param args   - Variable arguments list (already initialized)
	 * @return Number of characters written
	 *
	 * IMPLEMENTATION DETAIL:
	 *   This function performs position-independent callback relocation:
	 *   1. Gets address of FormatterCallback<TChar>
	 *   2. Calls PerformRelocation() to adjust for current load address
	 *   3. Passes relocated callback to StringFormatter::FormatV()
	 *
	 * WHY RELOCATION IS NEEDED:
	 *   In position-independent code, function addresses are relative.
	 *   StringFormatter needs an absolute address to call our callback.
	 *   PerformRelocation() converts relative → absolute at runtime.
	 */
	template <TCHAR TChar>
	static UINT32 WriteFormattedV(const TChar *format, VA_LIST args);
};

// ============================================================================
// TEMPLATE IMPLEMENTATIONS
// ============================================================================

/**
 * Write<TChar> - Null-terminated string output (inline implementation)
 *
 * This template is instantiated at compile-time for each character type.
 * The compiler generates two versions:
 *   1. Console::Write<CHAR>(const CHAR* text)
 *   2. Console::Write<WCHAR>(const WCHAR* text)
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
 *
 * DESIGN RATIONALE:
 *   Why not buffer the output?
 *   - Buffering requires allocating memory (heap or large stack)
 *   - Character-by-character is simpler and more position-independent
 *   - Kernel handles buffering internally anyway
 *   - For small outputs, overhead is negligible
 */
template <TCHAR TChar>
BOOL Console::FormatterCallback(PVOID context, TChar ch)
{
	(VOID)context;  // Unused - reserved for future use (e.g., buffer pointer)

	// Write single character to console
	// Return value: TRUE = success, FALSE = error
	return Write(&ch, 1);
}

/**
 * WriteFormattedV<TChar> - Variadic formatting implementation
 *
 * This is the core formatting function. WriteFormatted() is just a thin
 * wrapper that sets up the va_list and calls this function.
 *
 * POSITION-INDEPENDENT CALLBACK RELOCATION:
 *
 * The challenge: FormatterCallback is a template function, so its address
 * depends on the template parameter TChar. We need to pass this address
 * to StringFormatter, but in position-independent code, we need to relocate it.
 *
 * The solution:
 *   1. Get compile-time address: FormatterCallback<TChar>
 *   2. Cast to void pointer: (PVOID)FormatterCallback<TChar>
 *   3. Relocate at runtime: PerformRelocation((PVOID)FormatterCallback<TChar>)
 *   4. Cast to function pointer: (BOOL (*)(PVOID, TChar))
 *   5. Pass to formatter: StringFormatter::FormatV(callback, ...)
 *
 * This ensures the callback works correctly regardless of where the code is loaded.
 */
template <TCHAR TChar>
UINT32 Console::WriteFormattedV(const TChar *format, VA_LIST args)
{
	// Perform position-independent relocation of the callback function pointer
	// Without this, the callback address would be incorrect in PIC environments
	auto fixed = (BOOL (*)(PVOID, TChar))PerformRelocation((PVOID)FormatterCallback<TChar>);

	// Delegate to StringFormatter which handles all format specifier parsing
	// Parameters:
	//   fixed  - Relocated callback function
	//   NULL   - Context (unused, could be used for buffering)
	//   format - Format string (embedded, not in .rdata)
	//   args   - Variable arguments list
	return StringFormatter::FormatV(fixed, NULL, format, args);
}

/**
 * WriteFormatted<TChar> - Variadic formatting wrapper
 *
 * This function uses the C-style variadic argument mechanism:
 *   1. VA_LIST declares the va_list variable
 *   2. VA_START initializes it, pointing to the first unnamed argument
 *   3. WriteFormattedV processes the arguments
 *   4. VA_END cleans up (usually a no-op on modern platforms)
 *
 * CALLING CONVENTION NOTE:
 *   On x64, the first few arguments are passed in registers (RCX, RDX, R8, R9),
 *   with overflow on the stack. va_list abstracts this complexity.
 */
template <TCHAR TChar>
UINT32 Console::WriteFormatted(const TChar *format, ...)
{
	VA_LIST args;                                  // Declare va_list variable
	VA_START(args, format);                        // Initialize va_list (points after 'format')
	INT32 written = WriteFormattedV(format, args); // Call the va_list version
	VA_END(args);                                  // Clean up va_list (required by C standard)
	return written;                                // Return number of characters written
}
