/**
 * Logger.h - Structured Logging with ANSI Color Support
 *
 * Provides log level filtering and colored console output without CRT dependencies.
 * All logging is performed via direct console syscalls with ANSI escape sequences.
 *
 * DESIGN PATTERN:
 *   - Type-erased arguments eliminate per-argument-type template instantiations
 *   - ANSI colors: Green (Info), Red (Error), Yellow (Warning/Debug)
 *   - Zero-overhead when LogLevel = None (code eliminated at compile-time)
 *
 * USAGE:
 *   LOG_INFO("Server started on port %d", 8080);
 *   LOG_ERROR("Failed to allocate %d bytes", size);
 *   LOG_DEBUG("Variable value: %d", x);
 */

#pragma once

#include "platform.h" // Includes core.h (with string_formatter.h), console.h, date_time.h
#if defined(ENABLE_LOGGING)
// Convenience macros that automatically embed wide strings
#define LOG_INFO(format, ...) Logger::Info<WCHAR>(L##format##_embed, ##__VA_ARGS__)
#define LOG_ERROR(format, ...) Logger::Error<WCHAR>(L##format##_embed, ##__VA_ARGS__)
#define LOG_WARNING(format, ...) Logger::Warning<WCHAR>(L##format##_embed, ##__VA_ARGS__)
#if defined(ENABLE_DEBUG_LOGGING)
#define LOG_DEBUG(format, ...) Logger::Debug<WCHAR>(L##format##_embed, ##__VA_ARGS__)
#else
#define LOG_DEBUG(format, ...)
#endif // ENABLE_DEBUG_LOGGING
#else
// Define empty macros when logging is disabled
#define LOG_INFO(format, ...)
#define LOG_ERROR(format, ...)
#define LOG_DEBUG(format, ...)
#define LOG_WARNING(format, ...)
#endif // ENABLE_LOGGING

/**
 * Logger - Static logging utility class
 *
 * Public methods are variadic templates that type-erase arguments into
 * StringFormatter::Argument arrays, then forward to a single non-templated
 * TimestampedLogOutput. This eliminates per-argument-type template
 * instantiations that previously bloated the binary.
 */
class Logger
{
private:
	/**
	 * ConsoleCallback - Callback for console output (with ANSI colors)
	 */
	static BOOL ConsoleCallbackW(PVOID context, WCHAR ch)
	{
		(VOID) context;
		return Console::Write(&ch, 1);
	}

	/**
	 * TimestampedLogOutput - Single non-templated helper for all log levels.
	 * Arguments are pre-erased into a StringFormatter::Argument array by
	 * the public Info/Error/Warning/Debug methods, so this function is
	 * instantiated only once regardless of how many argument-type
	 * combinations appear across the codebase.
	 *
	 * @param colorPrefix - ANSI-colored prefix (e.g., "\033[0;32m[INF] ")
	 * @param format      - Wide format string with embedded specifiers
	 * @param args        - Pre-erased argument array (nullptr when argCount == 0)
	 * @param argCount    - Number of arguments
	 */
	static NOINLINE VOID TimestampedLogOutput(const WCHAR *colorPrefix, const WCHAR *format, const StringFormatter::Argument *args, INT32 argCount)
	{
		DateTime now = DateTime::Now();
		TimeOnlyString<WCHAR> timeStr = now.ToTimeOnlyString<WCHAR>();

		auto consoleW = EMBED_FUNC(ConsoleCallbackW);

		StringFormatter::Format<WCHAR>(consoleW, nullptr, L"%ls[%ls] "_embed, colorPrefix, (const WCHAR *)timeStr);
		StringFormatter::FormatWithArgs<WCHAR>(consoleW, nullptr, format, args, argCount);
		StringFormatter::Format<WCHAR>(consoleW, nullptr, L"\033[0m\n"_embed);
	}

public:
	/**
	 * Info - Informational messages (green)
	 *
	 * Use for: Normal operation events, status updates, confirmations
	 * Color: Green (ANSI: \033[0;32m)
	 */
	template <TCHAR TChar, typename... Args>
	static VOID Info(const TChar *format, Args... args)
	{
		if constexpr (sizeof...(Args) == 0)
			TimestampedLogOutput(L"\033[0;32m[INF] "_embed, format, nullptr, 0);
		else
		{
			StringFormatter::Argument argArray[] = {StringFormatter::Argument(args)...};
			TimestampedLogOutput(L"\033[0;32m[INF] "_embed, format, argArray, sizeof...(Args));
		}
	}

	/**
	 * Error - Error messages (red)
	 *
	 * Use for: Failures, exceptions, critical issues
	 * Color: Red (ANSI: \033[0;31m)
	 */
	template <TCHAR TChar, typename... Args>
	static VOID Error(const TChar *format, Args... args)
	{
		if constexpr (sizeof...(Args) == 0)
			TimestampedLogOutput(L"\033[0;31m[ERR] "_embed, format, nullptr, 0);
		else
		{
			StringFormatter::Argument argArray[] = {StringFormatter::Argument(args)...};
			TimestampedLogOutput(L"\033[0;31m[ERR] "_embed, format, argArray, sizeof...(Args));
		}
	}

	/**
	 * Warning - Warning messages (yellow)
	 *
	 * Use for: Non-critical issues, deprecation notices, potential problems
	 * Color: Yellow (ANSI: \033[0;33m)
	 */
	template <TCHAR TChar, typename... Args>
	static VOID Warning(const TChar *format, Args... args)
	{
		if constexpr (sizeof...(Args) == 0)
			TimestampedLogOutput(L"\033[0;33m[WRN] "_embed, format, nullptr, 0);
		else
		{
			StringFormatter::Argument argArray[] = {StringFormatter::Argument(args)...};
			TimestampedLogOutput(L"\033[0;33m[WRN] "_embed, format, argArray, sizeof...(Args));
		}
	}

	/**
	 * Debug - Debug messages (yellow)
	 *
	 * Use for: Detailed diagnostic information, variable dumps, trace logs
	 * Color: Yellow (ANSI: \033[0;33m)
	 */
	template <TCHAR TChar, typename... Args>
	static VOID Debug(const TChar *format, Args... args)
	{
		if constexpr (sizeof...(Args) == 0)
			TimestampedLogOutput(L"\033[0;33m[DBG] "_embed, format, nullptr, 0);
		else
		{
			StringFormatter::Argument argArray[] = {StringFormatter::Argument(args)...};
			TimestampedLogOutput(L"\033[0;33m[DBG] "_embed, format, argArray, sizeof...(Args));
		}
	}
};
