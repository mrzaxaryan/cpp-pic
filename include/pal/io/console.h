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

#include "bal.h"  // Includes EMBEDDED_FUNCTION_POINTER, string.h, string_formatter.h

// EMBEDDED_FUNCTION_POINTER is now available for position-independent function pointers

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
	 *   The function pointer is wrapped with EMBEDDED_FUNCTION_POINTER
	 *   to work correctly in position-independent code.
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
	 * WriteFormatted - Printf-style formatted output using C++11 variadic templates
	 *
	 * Supports standard printf format specifiers:
	 *   %d    - Signed decimal integer
	 *   %u    - Unsigned decimal integer
	 *   %ld   - Long signed decimal integer
	 *   %X    - Uppercase hexadecimal
	 *   %x    - Lowercase hexadecimal
	 *   %f    - Floating-point (default precision) - accepts DOUBLE directly!
	 *   %.Nf  - Floating-point with N decimal places
	 *   %c    - Single character
	 *   %s    - Narrow string (CHAR*)
	 *   %ls   - Wide string (WCHAR*)
	 *   %p    - Pointer (hexadecimal)
	 *
	 * @param format - Format string with embedded specifiers
	 * @param args   - Variable arguments matching format specifiers (supports custom types!)
	 * @return Number of characters written
	 *
	 * TEMPLATE PARAMETERS:
	 *   TChar - Character type for format string (CHAR or WCHAR)
	 *   Args  - Variadic template arguments (deduced automatically)
	 *
	 * USAGE:
	 *   Console::WriteFormatted<WCHAR>(
	 *       L"Value: %d, Pi: %.5f\n"_embed,
	 *       42,
	 *       3.14159_embed  // Pass DOUBLE directly - no casting needed!
	 *   );
	 *
	 * POSITION-INDEPENDENT IMPLEMENTATION:
	 *   - Format string embedded in .text (no .rdata)
	 *   - Floating-point constants as immediates
	 *   - Stack-based buffer for formatting
	 *   - No heap allocations
	 *   - Type-safe variadic templates (no VA_LIST)
	 */
	template <TCHAR TChar, typename... Args>
	static UINT32 WriteFormatted(const TChar *format, Args&&... args);
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
 * WriteFormatted<TChar, Args...> - Variadic template formatting implementation
 *
 * This function uses C++11 variadic templates for type-safe formatting.
 * No more VA_LIST or VA_ARG - everything is type-safe at compile time!
 *
 * POSITION-INDEPENDENT CALLBACK USING EMBEDDED_FUNCTION_POINTER:
 *
 * The challenge: FormatterCallback is a template function, so its address
 * depends on the template parameter TChar. We need to pass this address
 * to StringFormatter in a position-independent way.
 *
 * The solution using EMBEDDED_FUNCTION_POINTER:
 *   1. Template instantiation: FormatterCallback<TChar>
 *   2. PC-relative wrapper: EMBEDDED_FUNCTION_POINTER<BOOL (*)(PVOID, TChar), FormatterCallback<TChar>>
 *   3. Get position-independent pointer: ::Get()
 *   4. Pass to formatter: StringFormatter::Format(callback, ...)
 *
 * This works correctly in both PIC blob and normal EXE modes without runtime checks.
 *
 * BENEFITS OVER VA_LIST:
 *   - Type-safe: compiler knows exact types at compile time
 *   - Supports custom types like DOUBLE without casting
 *   - No POD requirement
 *   - Better optimization opportunities
 */
template <TCHAR TChar, typename... Args>
UINT32 Console::WriteFormatted(const TChar *format, Args&&... args)
{
	// Get position-independent function pointer using EMBED_FUNC
	// This works correctly regardless of where the code is loaded (PIC or normal EXE)
	auto fixed = EMBED_FUNC(FormatterCallback<TChar>);

	// Delegate to StringFormatter which handles all format specifier parsing
	// Parameters:
	//   fixed  - Position-independent callback function
	//   NULL   - Context (unused, could be used for buffering)
	//   format - Format string (embedded, not in .rdata)
	//   args   - Variadic template arguments (perfectly forwarded)
	return StringFormatter::Format(fixed, NULL, format, static_cast<Args&&>(args)...);
}
