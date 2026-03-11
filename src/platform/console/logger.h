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
// Convenience macros for logging
#define LOG_INFO(format, ...) Logger::Info<CHAR>(format, ##__VA_ARGS__)
#define LOG_ERROR(format, ...) Logger::Error<CHAR>(format, ##__VA_ARGS__)
#define LOG_WARNING(format, ...) Logger::Warning<CHAR>(format, ##__VA_ARGS__)
#if defined(ENABLE_DEBUG_LOGGING)
#define LOG_DEBUG(format, ...) Logger::Debug<CHAR>(format, ##__VA_ARGS__)
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
 * StringFormatter::Argument arrays, then format the message in the caller's
 * inline scope.
 *
 * DESIGN NOTE: All PIC string literals (colour prefix, format string, reset
 * sequence) and any pointers captured inside the Argument array MUST be
 * consumed within the SAME inline scope that materialised them.  Passing
 * a PIC-stack pointer as const CHAR* to a NOINLINE function can fail under
 * -Oz (machine outliner / aggressive stack-slot reuse) because the compiler
 * loses the alias chain between the stack data and the escaped pointer.
 * Only the timestamp is computed inside the NOINLINE helper (WriteTimestamp),
 * since DateTime::Now / ToTimeOnlyString use no caller-provided PIC data.
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
	 * WriteTimestamp - NOINLINE helper that emits [HH:MM:SS].
	 *
	 * Kept NOINLINE so that DateTime::Now / ToTimeOnlyString are not
	 * duplicated into every template instantiation.  This function uses
	 * NO caller-provided PIC pointers, so it is safe across the NOINLINE
	 * boundary.
	 */
	static NOINLINE VOID WriteTimestamp()
	{
		DateTime now = DateTime::Now();
		TimeOnlyString<CHAR> timeStr = now.ToTimeOnlyString<CHAR>();

		StringFormatter::Format<CHAR>(&ConsoleCallbackA, nullptr, "[%s] ", (const CHAR *)timeStr);
	}

public:
	template <TCHAR TChar, typename... Args>
	static VOID Info(const TChar *format, Args... args)
	{
		Console::Write(Span<const CHAR>("\033[0;32m[INF] "));
		WriteTimestamp();
		if constexpr (sizeof...(Args) == 0)
			StringFormatter::FormatWithArgs<CHAR>(&ConsoleCallbackA, nullptr, format, Span<const StringFormatter::Argument>());
		else
		{
			StringFormatter::Argument argArray[] = {StringFormatter::Argument(args)...};
			StringFormatter::FormatWithArgs<CHAR>(&ConsoleCallbackA, nullptr, format, Span<const StringFormatter::Argument>(argArray));
		}
		Console::Write(Span<const CHAR>("\033[0m\n"));
	}

	template <TCHAR TChar, typename... Args>
	static VOID Error(const TChar *format, Args... args)
	{
		Console::Write(Span<const CHAR>("\033[0;31m[ERR] "));
		WriteTimestamp();
		if constexpr (sizeof...(Args) == 0)
			StringFormatter::FormatWithArgs<CHAR>(&ConsoleCallbackA, nullptr, format, Span<const StringFormatter::Argument>());
		else
		{
			StringFormatter::Argument argArray[] = {StringFormatter::Argument(args)...};
			StringFormatter::FormatWithArgs<CHAR>(&ConsoleCallbackA, nullptr, format, Span<const StringFormatter::Argument>(argArray));
		}
		Console::Write(Span<const CHAR>("\033[0m\n"));
	}

	template <TCHAR TChar, typename... Args>
	static VOID Warning(const TChar *format, Args... args)
	{
		Console::Write(Span<const CHAR>("\033[0;33m[WRN] "));
		WriteTimestamp();
		if constexpr (sizeof...(Args) == 0)
			StringFormatter::FormatWithArgs<CHAR>(&ConsoleCallbackA, nullptr, format, Span<const StringFormatter::Argument>());
		else
		{
			StringFormatter::Argument argArray[] = {StringFormatter::Argument(args)...};
			StringFormatter::FormatWithArgs<CHAR>(&ConsoleCallbackA, nullptr, format, Span<const StringFormatter::Argument>(argArray));
		}
		Console::Write(Span<const CHAR>("\033[0m\n"));
	}

	template <TCHAR TChar, typename... Args>
	static VOID Debug(const TChar *format, Args... args)
	{
		Console::Write(Span<const CHAR>("\033[0;33m[DBG] "));
		WriteTimestamp();
		if constexpr (sizeof...(Args) == 0)
			StringFormatter::FormatWithArgs<CHAR>(&ConsoleCallbackA, nullptr, format, Span<const StringFormatter::Argument>());
		else
		{
			StringFormatter::Argument argArray[] = {StringFormatter::Argument(args)...};
			StringFormatter::FormatWithArgs<CHAR>(&ConsoleCallbackA, nullptr, format, Span<const StringFormatter::Argument>(argArray));
		}
		Console::Write(Span<const CHAR>("\033[0m\n"));
	}
};
