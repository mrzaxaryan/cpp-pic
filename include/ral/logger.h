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

#include "console.h"
#include "DateTime.h"

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
	None = 0,    // No logging
	Default = 1, // Info, Error, Warning
	Debug = 2    // All messages
};

// Global log level - modify this to control logging at compile-time
inline constexpr LogLevels LogLevel = LogLevels::Default;

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
	 * LogWithPrefix - Internal helper to eliminate VA_LIST duplication
	 *
	 * Centralizes the common pattern:
	 *   1. VA_START to initialize variadic args
	 *   2. Write colored prefix
	 *   3. Format and write message
	 *   4. Write color reset and newline
	 *   5. VA_END cleanup
	 *
	 * @param prefix - ANSI-colored prefix string (e.g., "[INFO]")
	 * @param format - Format string with embedded specifiers
	 * @param args   - Variadic argument list (already initialized)
	 *
	 * TEMPLATE PARAMETERS:
	 *   TChar - Character type for format string (CHAR or WCHAR)
	 */
	template <TCHAR TChar>
	FORCE_INLINE static VOID LogWithPrefixV(const WCHAR *prefix, const TChar *format, VA_LIST args)
	{
		// Get current time
		DateTime now = DateTime::Now();
		TimeOnlyString<WCHAR> timeStr = now.ToTimeOnlyString<WCHAR>();

		Console::Write<WCHAR>(prefix);                 // Colored prefix (e.g., "[INFO]")
		Console::Write<WCHAR>(L"["_embed);             // Start time bracket
		Console::Write<WCHAR>((const WCHAR *)timeStr); // Current time
		Console::Write<WCHAR>(L"] "_embed);            // End time bracket + space
		Console::WriteFormattedV<TChar>(format, args); // User message
		Console::Write<WCHAR>(L"\033[0m\n"_embed);     // Reset color + newline
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
		LogWithPrefixV<TChar>(L"\033[0;32m[INF] "_embed, format, args);
		VA_END(args);
	}
	else
	{
		(VOID)format; // Suppress unused parameter warning
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
		LogWithPrefixV<TChar>(L"\033[0;31m[ERR] "_embed, format, args);
		VA_END(args);
	}
	else
	{
		(VOID)format; // Suppress unused parameter warning
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
		LogWithPrefixV<TChar>(L"\033[0;33m[WRN] "_embed, format, args);
		VA_END(args);
	}
	else
	{
		(VOID)format; // Suppress unused parameter warning
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
		LogWithPrefixV<TChar>(L"\033[0;33m[DBG] "_embed, format, args);
		VA_END(args);
	}
	else
	{
		(VOID)format; // Suppress unused parameter warning
	}
}