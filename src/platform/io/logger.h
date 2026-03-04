/**
 * @file logger.h
 * @brief Structured logging with ANSI color support
 *
 * @details Provides log level filtering and colored console output without CRT
 * dependencies. All logging is performed via direct console syscalls with ANSI
 * escape sequences. Log levels include Info (green), Error (red), Warning
 * (yellow), and Debug (yellow). Type-erased arguments eliminate per-argument-type
 * template instantiations, and logging is zero-overhead when disabled at compile
 * time via the ENABLE_LOGGING and ENABLE_DEBUG_LOGGING preprocessor flags.
 */

#pragma once

#include "platform/platform.h"
#if defined(ENABLE_LOGGING)
// Convenience macros that automatically embed strings
#define LOG_INFO(format, ...) Logger::Info<CHAR>(format##_embed, ##__VA_ARGS__)
#define LOG_ERROR(format, ...) Logger::Error<CHAR>(format##_embed, ##__VA_ARGS__)
#define LOG_WARNING(format, ...) Logger::Warning<CHAR>(format##_embed, ##__VA_ARGS__)
#if defined(ENABLE_DEBUG_LOGGING)
#define LOG_DEBUG(format, ...) Logger::Debug<CHAR>(format##_embed, ##__VA_ARGS__)
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
	static BOOL ConsoleCallbackA(PVOID context, CHAR ch)
	{
		(VOID) context;
		return Console::Write(Span<const CHAR>(&ch, 1));
	}

	/**
	 * TimestampedLogOutput - Single non-templated helper for all log levels.
	 * Arguments are pre-erased into a StringFormatter::Argument array by
	 * the public Info/Error/Warning/Debug methods, so this function is
	 * instantiated only once regardless of how many argument-type
	 * combinations appear across the codebase.
	 *
	 * @param colorPrefix - ANSI-colored prefix (e.g., "\033[0;32m[INF] ")
	 * @param format      - Format string with embedded specifiers
	 * @param args        - Pre-erased argument array (nullptr when argCount == 0)
	 * @param argCount    - Number of arguments
	 */
	static NOINLINE VOID TimestampedLogOutput(const CHAR *colorPrefix, const CHAR *format, Span<const StringFormatter::Argument> args)
	{
		DateTime now = DateTime::Now();
		TimeOnlyString<CHAR> timeStr = now.ToTimeOnlyString<CHAR>();

		auto consoleA = EMBED_FUNC(ConsoleCallbackA);

		StringFormatter::Format<CHAR>(consoleA, nullptr, "%s[%s] "_embed, colorPrefix, (const CHAR *)timeStr);
		StringFormatter::FormatWithArgs<CHAR>(consoleA, nullptr, format, args);
		StringFormatter::Format<CHAR>(consoleA, nullptr, "\033[0m\n"_embed);
	}

public:
	/**
	 * Info - Informational messages (green)
	 *
	 * Use for: Normal operation events, status updates, confirmations
	 * Color: Green (ANSI: \033[0;32m)
	 *
	 * @tparam TChar Character type (CHAR or WCHAR)
	 * @tparam Args Variadic format arguments (deduced automatically)
	 */
	template <TCHAR TChar, typename... Args>
	static VOID Info(const TChar *format, Args... args)
	{
		if constexpr (sizeof...(Args) == 0)
			TimestampedLogOutput("\033[0;32m[INF] "_embed, format, Span<const StringFormatter::Argument>());
		else
		{
			StringFormatter::Argument argArray[] = {StringFormatter::Argument(args)...};
			TimestampedLogOutput("\033[0;32m[INF] "_embed, format, Span<const StringFormatter::Argument>(argArray));
		}
	}

	/**
	 * Error - Error messages (red)
	 *
	 * Use for: Failures, exceptions, critical issues
	 * Color: Red (ANSI: \033[0;31m)
	 *
	 * @tparam TChar Character type (CHAR or WCHAR)
	 * @tparam Args Variadic format arguments (deduced automatically)
	 */
	template <TCHAR TChar, typename... Args>
	static VOID Error(const TChar *format, Args... args)
	{
		if constexpr (sizeof...(Args) == 0)
			TimestampedLogOutput("\033[0;31m[ERR] "_embed, format, Span<const StringFormatter::Argument>());
		else
		{
			StringFormatter::Argument argArray[] = {StringFormatter::Argument(args)...};
			TimestampedLogOutput("\033[0;31m[ERR] "_embed, format, Span<const StringFormatter::Argument>(argArray));
		}
	}

	/**
	 * Warning - Warning messages (yellow)
	 *
	 * Use for: Non-critical issues, deprecation notices, potential problems
	 * Color: Yellow (ANSI: \033[0;33m)
	 *
	 * @tparam TChar Character type (CHAR or WCHAR)
	 * @tparam Args Variadic format arguments (deduced automatically)
	 */
	template <TCHAR TChar, typename... Args>
	static VOID Warning(const TChar *format, Args... args)
	{
		if constexpr (sizeof...(Args) == 0)
			TimestampedLogOutput("\033[0;33m[WRN] "_embed, format, Span<const StringFormatter::Argument>());
		else
		{
			StringFormatter::Argument argArray[] = {StringFormatter::Argument(args)...};
			TimestampedLogOutput("\033[0;33m[WRN] "_embed, format, Span<const StringFormatter::Argument>(argArray));
		}
	}

	/**
	 * Debug - Debug messages (yellow)
	 *
	 * Use for: Detailed diagnostic information, variable dumps, trace logs
	 * Color: Yellow (ANSI: \033[0;33m)
	 *
	 * @tparam TChar Character type (CHAR or WCHAR)
	 * @tparam Args Variadic format arguments (deduced automatically)
	 */
	template <TCHAR TChar, typename... Args>
	static VOID Debug(const TChar *format, Args... args)
	{
		if constexpr (sizeof...(Args) == 0)
			TimestampedLogOutput("\033[0;33m[DBG] "_embed, format, Span<const StringFormatter::Argument>());
		else
		{
			StringFormatter::Argument argArray[] = {StringFormatter::Argument(args)...};
			TimestampedLogOutput("\033[0;33m[DBG] "_embed, format, Span<const StringFormatter::Argument>(argArray));
		}
	}
};
