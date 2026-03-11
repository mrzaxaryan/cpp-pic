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
 * StringFormatter::Argument arrays, then forward to a single non-templated
 * TimestampedLogOutput. This eliminates per-argument-type template
 * instantiations that previously bloated the binary.
 *
 * DESIGN NOTE: The colour prefix is written by each public method BEFORE
 * calling TimestampedLogOutput, not inside the NOINLINE helper. This is
 * required because the string literal holding the prefix must be consumed
 * (written via Console::Write) within the SAME inline scope. Passing the
 * prefix as a const CHAR* to a NOINLINE function can fail on FreeBSD
 * x86_64 at -O1+ with LTO due to alias chain breakage between the stack
 * data and the escaped pointer.
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
	 *
	 * The colour prefix has already been written by the caller; this function
	 * emits the [HH:MM:SS] timestamp, the formatted message, and the ANSI
	 * reset sequence.
	 *
	 * @param format - Format string with embedded specifiers
	 * @param args   - Pre-erased argument array
	 */
	static NOINLINE VOID TimestampedLogOutput(const CHAR *format, Span<const StringFormatter::Argument> args)
	{
		DateTime now = DateTime::Now();
		TimeOnlyString<CHAR> timeStr = now.ToTimeOnlyString<CHAR>();

		auto consoleA = EMBED_FUNC(ConsoleCallbackA);
		StringFormatter::Format<CHAR>(consoleA, nullptr, "[%s] ", (const CHAR *)timeStr);
		StringFormatter::FormatWithArgs<CHAR>(consoleA, nullptr, format, args);
		Console::Write<CHAR>("\033[0m\n");
	}

public:
	template <TCHAR TChar, typename... Args>
	static VOID Info(const TChar *format, Args... args)
	{
		Console::Write(Span<const CHAR>("\033[0;32m[INF] "));
		if constexpr (sizeof...(Args) == 0)
			TimestampedLogOutput(format, Span<const StringFormatter::Argument>());
		else
		{
			StringFormatter::Argument argArray[] = {StringFormatter::Argument(args)...};
			TimestampedLogOutput(format, Span<const StringFormatter::Argument>(argArray));
		}
	}

	template <TCHAR TChar, typename... Args>
	static VOID Error(const TChar *format, Args... args)
	{
		Console::Write(Span<const CHAR>("\033[0;31m[ERR] "));
		if constexpr (sizeof...(Args) == 0)
			TimestampedLogOutput(format, Span<const StringFormatter::Argument>());
		else
		{
			StringFormatter::Argument argArray[] = {StringFormatter::Argument(args)...};
			TimestampedLogOutput(format, Span<const StringFormatter::Argument>(argArray));
		}
	}

	template <TCHAR TChar, typename... Args>
	static VOID Warning(const TChar *format, Args... args)
	{
		Console::Write(Span<const CHAR>("\033[0;33m[WRN] "));
		if constexpr (sizeof...(Args) == 0)
			TimestampedLogOutput(format, Span<const StringFormatter::Argument>());
		else
		{
			StringFormatter::Argument argArray[] = {StringFormatter::Argument(args)...};
			TimestampedLogOutput(format, Span<const StringFormatter::Argument>(argArray));
		}
	}

	template <TCHAR TChar, typename... Args>
	static VOID Debug(const TChar *format, Args... args)
	{
		Console::Write(Span<const CHAR>("\033[0;33m[DBG] "));
		if constexpr (sizeof...(Args) == 0)
			TimestampedLogOutput(format, Span<const StringFormatter::Argument>());
		else
		{
			StringFormatter::Argument argArray[] = {StringFormatter::Argument(args)...};
			TimestampedLogOutput(format, Span<const StringFormatter::Argument>(argArray));
		}
	}
};
