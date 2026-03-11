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
 * StringFormatter::Argument arrays, then forward everything to a single
 * NOINLINE helper (WriteLogLine).
 *
 * DESIGN NOTE (Windows aarch64 -Oz):
 *   Two separate code-gen hazards require care here:
 *
 *   1. Stack-slot colouring: LLVM can merge PIC-transformed string allocas
 *      whose lifetimes appear non-overlapping.  Combined with instruction
 *      reordering this makes the format-string pointer read stale
 *      colour-prefix data.
 *
 *   2. Tail-call optimisation: a NOINLINE function whose last statement is
 *      Console::Write(Span) may be tail-called.  The epilogue tears down the
 *      frame *before* Console::Write dereferences the Span pointer, leaving
 *      the PIC data on a freed stack slot.
 *
 *   Both are avoided by building the ANSI prefix and reset from register
 *   immediates (individual char stores, not string literals) inside a single
 *   NOINLINE function that also formats the message.  The only PIC string
 *   that remains is the caller's format string, which lives in the caller's
 *   frame and is always valid for the duration of the NOINLINE call.
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
	 * WriteLogLine - single NOINLINE helper for all log levels.
	 *
	 * @param colorDigit  ANSI colour digit: '2' green, '1' red, '3' yellow
	 * @param l1,l2,l3    Three-letter level tag, e.g. 'I','N','F'
	 * @param format      Caller's PIC format string (lives in caller's frame)
	 * @param args        Type-erased argument span
	 *
	 * Colour prefix and reset are assembled from immediate char stores
	 * (no string literals ⇒ no PIC alloca ⇒ no stack-slot / tail-call hazard).
	 * The timestamp format string "[%s] " is the only PIC literal inside this
	 * frame, and it is consumed before the caller's format pointer is used,
	 * so their allocas cannot collide.
	 */
	static NOINLINE VOID WriteLogLine(CHAR colorDigit, CHAR l1, CHAR l2, CHAR l3,
	                                  const CHAR *format, Span<const StringFormatter::Argument> args)
	{
		// ── colour prefix  "\033[0;3Xm[LVL] " ──────────────────────────
		CHAR prefix[13];
		prefix[0]  = '\033';
		prefix[1]  = '[';
		prefix[2]  = '0';
		prefix[3]  = ';';
		prefix[4]  = '3';
		prefix[5]  = colorDigit;
		prefix[6]  = 'm';
		prefix[7]  = '[';
		prefix[8]  = l1;
		prefix[9]  = l2;
		prefix[10] = l3;
		prefix[11] = ']';
		prefix[12] = ' ';
		Console::Write(Span<const CHAR>(prefix, 13));

		// ── timestamp [HH:MM:SS] ────────────────────────────────────────
		DateTime now = DateTime::Now();
		TimeOnlyString<CHAR> timeStr = now.ToTimeOnlyString<CHAR>();
		StringFormatter::Format<CHAR>(&ConsoleCallbackA, nullptr, "[%s] ", (const CHAR *)timeStr);

		// ── caller's message ────────────────────────────────────────────
		StringFormatter::FormatWithArgs<CHAR>(&ConsoleCallbackA, nullptr, format, args);

		// ── reset "\033[0m\n" ───────────────────────────────────────────
		// Written char-by-char so the last Console::Write cannot be
		// tail-called with a dangling stack pointer.
		ConsoleCallbackA(nullptr, '\033');
		ConsoleCallbackA(nullptr, '[');
		ConsoleCallbackA(nullptr, '0');
		ConsoleCallbackA(nullptr, 'm');
		ConsoleCallbackA(nullptr, '\n');
	}

public:
	template <TCHAR TChar, typename... Args>
	static VOID Info(const TChar *format, Args... args)
	{
		if constexpr (sizeof...(Args) == 0)
			WriteLogLine('2', 'I', 'N', 'F', format, Span<const StringFormatter::Argument>());
		else
		{
			StringFormatter::Argument argArray[] = {StringFormatter::Argument(args)...};
			WriteLogLine('2', 'I', 'N', 'F', format, Span<const StringFormatter::Argument>(argArray));
		}
	}

	template <TCHAR TChar, typename... Args>
	static VOID Error(const TChar *format, Args... args)
	{
		if constexpr (sizeof...(Args) == 0)
			WriteLogLine('1', 'E', 'R', 'R', format, Span<const StringFormatter::Argument>());
		else
		{
			StringFormatter::Argument argArray[] = {StringFormatter::Argument(args)...};
			WriteLogLine('1', 'E', 'R', 'R', format, Span<const StringFormatter::Argument>(argArray));
		}
	}

	template <TCHAR TChar, typename... Args>
	static VOID Warning(const TChar *format, Args... args)
	{
		if constexpr (sizeof...(Args) == 0)
			WriteLogLine('3', 'W', 'R', 'N', format, Span<const StringFormatter::Argument>());
		else
		{
			StringFormatter::Argument argArray[] = {StringFormatter::Argument(args)...};
			WriteLogLine('3', 'W', 'R', 'N', format, Span<const StringFormatter::Argument>(argArray));
		}
	}

	template <TCHAR TChar, typename... Args>
	static VOID Debug(const TChar *format, Args... args)
	{
		if constexpr (sizeof...(Args) == 0)
			WriteLogLine('3', 'D', 'B', 'G', format, Span<const StringFormatter::Argument>());
		else
		{
			StringFormatter::Argument argArray[] = {StringFormatter::Argument(args)...};
			WriteLogLine('3', 'D', 'B', 'G', format, Span<const StringFormatter::Argument>(argArray));
		}
	}
};
