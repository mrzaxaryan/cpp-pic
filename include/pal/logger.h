/**
 * Logger.h - Structured Logging with ANSI Color Support
 *
 * Provides log level filtering and colored console output without CRT dependencies.
 * All logging is performed via direct console syscalls with ANSI escape sequences.
 *
 * DESIGN PATTERN:
 *   - Template-based for compile-time log level optimization
 *   - ANSI colors: Green (Info), Red (Error), Yellow (Warning/Debug)
 *   - Zero-overhead when LogLevel = None (code eliminated at compile-time)
 *
 * USAGE:
 *   LOG_INFO("Server started on port %d", 8080);
 *   LOG_ERROR("Failed to allocate %d bytes", size);
 *   LOG_DEBUG("Variable value: %d", x);
 */

#pragma once

#include "pal.h"  // Includes bal.h (with string_formatter.h), console.h, date_time.h, file_system.h

// Convenience macros that automatically embed wide strings
#define LOG_INFO(format, ...) Logger::Info<WCHAR>(L##format##_embed, ##__VA_ARGS__)
#define LOG_ERROR(format, ...) Logger::Error<WCHAR>(L##format##_embed, ##__VA_ARGS__)
#define LOG_DEBUG(format, ...) Logger::Debug<WCHAR>(L##format##_embed, ##__VA_ARGS__)
#define LOG_WARNING(format, ...) Logger::Warning<WCHAR>(L##format##_embed, ##__VA_ARGS__)

/**
 * LogLevels - Compile-time log filtering levels
 *
 * None:    Disable all logging (code eliminated by optimizer)
 * Default: Info, Error, Warning (production)
 * Debug:   All messages including Debug (development)
 */
enum class LogLevels : UINT8
{
	None = 0,	 // No logging
	Default = 1, // Info, Error, Warning
	Debug = 2	 // All messages
};

enum class LogOutputs : UINT8
{
	Console = 0x1, // Output to console
	File = 0x2,	   // Output to file (not implemented)
	Both = 0x3	   // Output to both console and file
};

// Global log level - modify this to control logging at compile-time
inline constexpr LogLevels LogLevel = LogLevels::Default;
inline constexpr LogOutputs LogOutput = LogOutputs::Both;
/**
 * Logger - Static logging utility class
 *
 * All methods are static and use variadic templates for type-safe formatting.
 * Log level checks are performed at compile-time using `if constexpr`.
 */
class Logger
{
private:
	/**
	 * ConsoleCallback - Callback for console output (with ANSI colors)
	 */
	template <TCHAR TChar>
	static BOOL ConsoleCallback(PVOID context, TChar ch)
	{
		(VOID) context;
		return Console::Write(&ch, 1);
	}

	/**
	 * FileCallback - Callback for file output (plain text, no colors)
	 */
	template <TCHAR TChar>
	static BOOL FileCallback(PVOID context, TChar ch)
	{
		File *logFile = static_cast<File *>(context);
		if (logFile && logFile->IsValid())
		{
			logFile->Write(&ch, sizeof(TChar));
		}
		return TRUE;
	}

	/**
	 * LogWithPrefix - Internal helper to eliminate VA_LIST duplication
	 *
	 * Writes colored output to console and plain text to file.
	 *
	 * @param colorPrefix - ANSI-colored prefix for console (e.g., "\033[0;32m[INF] ")
	 * @param plainPrefix - Plain prefix for file (e.g., "[INF] ")
	 * @param format      - Format string with embedded specifiers
	 * @param args        - Variadic argument list (already initialized)
	 *
	 * TEMPLATE PARAMETERS:
	 *   TChar - Character type for format string (CHAR or WCHAR)
	 */
	template <TCHAR TChar>
	FORCE_INLINE static VOID LogWithPrefixV(const WCHAR *colorPrefix, const WCHAR *plainPrefix, const TChar *format, VA_LIST args)
	{
		// Get current time
		DateTime now = DateTime::Now();
		TimeOnlyString<WCHAR> timeStr = now.ToTimeOnlyString<WCHAR>();

		// Console output (with colors)
		if constexpr ((static_cast<UINT8>(LogOutput) & static_cast<UINT8>(LogOutputs::Console)) != 0)
		{
			auto consoleW = EMBED_FUNC(ConsoleCallback<WCHAR>);
			auto consoleT = EMBED_FUNC(ConsoleCallback<TChar>);

			StringFormatter::Format<WCHAR>(consoleW, NULL, L"%ls[%ls] "_embed, colorPrefix, (const WCHAR *)timeStr);
			StringFormatter::FormatV<TChar>(consoleT, NULL, format, args);
			StringFormatter::Format<WCHAR>(consoleW, NULL, L"\033[0m\n"_embed);
		}

		// File output (plain text, no colors)
		if constexpr ((static_cast<UINT8>(LogOutput) & static_cast<UINT8>(LogOutputs::File)) != 0)
		{
			File logFile = FileSystem::Open(L"output.log.txt"_embed, FileSystem::FS_WRITE | FileSystem::FS_CREATE | FileSystem::FS_APPEND);
			if (logFile.IsValid())
			{
				logFile.MoveOffset(0, OffsetOrigin::End);

				auto fileW = EMBED_FUNC(FileCallback<WCHAR>);
				auto fileT = EMBED_FUNC(FileCallback<TChar>);

				StringFormatter::Format<WCHAR>(fileW, &logFile, L"%ls[%ls] "_embed, plainPrefix, (const WCHAR *)timeStr);
				StringFormatter::FormatV<TChar>(fileT, &logFile, format, args);
				StringFormatter::Format<WCHAR>(fileW, &logFile, L"\n"_embed);
			}
		}
	}

public:
	/**
	 * Info - Informational messages (green)
	 *
	 * Use for: Normal operation events, status updates, confirmations
	 * Enabled when: LogLevel >= Default
	 * Color: Green (ANSI: \033[0;32m)
	 */
	template <TCHAR TChar>
	static VOID Info(const TChar *format, ...);

	/**
	 * Error - Error messages (red)
	 *
	 * Use for: Failures, exceptions, critical issues
	 * Enabled when: LogLevel >= Default
	 * Color: Red (ANSI: \033[0;31m)
	 */
	template <TCHAR TChar>
	static VOID Error(const TChar *format, ...);

	/**
	 * Warning - Warning messages (yellow)
	 *
	 * Use for: Non-critical issues, deprecation notices, potential problems
	 * Enabled when: LogLevel >= Default
	 * Color: Yellow (ANSI: \033[0;33m)
	 */
	template <TCHAR TChar>
	static VOID Warning(const TChar *format, ...);

	/**
	 * Debug - Debug messages (yellow)
	 *
	 * Use for: Detailed diagnostic information, variable dumps, trace logs
	 * Enabled when: LogLevel >= Debug
	 * Color: Yellow (ANSI: \033[0;33m)
	 */
	template <TCHAR TChar>
	static VOID Debug(const TChar *format, ...);
};

// ============================================================================
// TEMPLATE IMPLEMENTATIONS
// ============================================================================

/**
 * Info - Informational logging (implementation)
 *
 * Compile-time optimization:
 *   - If LogLevel == None, entire function body is eliminated
 *   - No runtime overhead when logging is disabled
 */
template <TCHAR TChar>
VOID Logger::Info(const TChar *format, ...)
{
	if constexpr (LogLevel != LogLevels::None)
	{
		VA_LIST args;
		VA_START(args, format);
		LogWithPrefixV<TChar>(L"\033[0;32m[INF] "_embed, L"[INF] "_embed, format, args);
		VA_END(args);
	}
	else
	{
		(VOID) format; // Suppress unused parameter warning
	}
}

/**
 * Error - Error logging (implementation)
 *
 * Enabled for Default and Debug log levels.
 * Uses red color to highlight critical issues.
 */
template <TCHAR TChar>
VOID Logger::Error(const TChar *format, ...)
{
	if constexpr (LogLevel != LogLevels::None)
	{
		VA_LIST args;
		VA_START(args, format);
		LogWithPrefixV<TChar>(L"\033[0;31m[ERR] "_embed, L"[ERR] "_embed, format, args);
		VA_END(args);
	}
	else
	{
		(VOID) format; // Suppress unused parameter warning
	}
}

/**
 * Warning - Warning logging (implementation)
 *
 * Enabled for Default and Debug log levels.
 * Uses yellow color for non-critical warnings.
 */
template <TCHAR TChar>
VOID Logger::Warning(const TChar *format, ...)
{
	if constexpr (LogLevel != LogLevels::None)
	{
		VA_LIST args;
		VA_START(args, format);
		LogWithPrefixV<TChar>(L"\033[0;33m[WRN] "_embed, L"[WRN] "_embed, format, args);
		VA_END(args);
	}
	else
	{
		(VOID) format; // Suppress unused parameter warning
	}
}

/**
 * Debug - Debug logging (implementation)
 *
 * Only enabled when LogLevel == Debug.
 * Compile-time check eliminates debug code in production builds.
 */
template <TCHAR TChar>
VOID Logger::Debug(const TChar *format, ...)
{
	if constexpr (LogLevel == LogLevels::Debug)
	{
		VA_LIST args;
		VA_START(args, format);
		LogWithPrefixV<TChar>(L"\033[0;33m[DBG] "_embed, L"[DBG] "_embed, format, args);
		VA_END(args);
	}
	else
	{
		(VOID) format; // Suppress unused parameter warning
	}
}