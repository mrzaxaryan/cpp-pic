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

#include "pal.h"  // Includes bal.h (with string_formatter.h), console.h, date_time.h

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
	 * ConsoleCallback - Callback for console output (with ANSI colors)
	 */
	template <TCHAR TChar>
	static BOOL ConsoleCallback(PVOID context, TChar ch)
	{
		(VOID) context;
		return Console::Write(&ch, 1);
	}

	/**
	 * TimestampedLogOutput - Internal helper using variadic templates
	 *
	 * Writes colored output to console.
	 *
	 * @param colorPrefix - ANSI-colored prefix for console (e.g., "\033[0;32m[INF] ")
	 * @param format      - Format string with embedded specifiers
	 * @param args        - Variadic template arguments
	 *
	 * TEMPLATE PARAMETERS:
	 *   TChar - Character type for format string (CHAR or WCHAR)
	 *   Args  - Variadic template arguments (deduced automatically)
	 */
	template <TCHAR TChar, typename... Args>
	FORCE_INLINE static VOID TimestampedLogOutput(const WCHAR *colorPrefix, const TChar *format, Args&&... args)
	{
		// Get current time
		DateTime now = DateTime::Now();
		TimeOnlyString<WCHAR> timeStr = now.ToTimeOnlyString<WCHAR>();

		auto consoleW = EMBED_FUNC(ConsoleCallback<WCHAR>);
		auto consoleT = EMBED_FUNC(ConsoleCallback<TChar>);

		StringFormatter::Format<WCHAR>(consoleW, NULL, L"%ls[%ls] "_embed, colorPrefix, (const WCHAR *)timeStr);
		StringFormatter::Format<TChar>(consoleT, NULL, format, static_cast<Args&&>(args)...);
		StringFormatter::Format<WCHAR>(consoleW, NULL, L"\033[0m\n"_embed);
	}

public:
	/**
	 * Info - Informational messages (green)
	 *
	 * Use for: Normal operation events, status updates, confirmations
	 * Enabled when: LogLevel >= Default
	 * Color: Green (ANSI: \033[0;32m)
	 *
	 * USAGE: Now supports custom types like DOUBLE directly!
	 *   LOG_INFO("Temperature: %.2f degrees", 98.6_embed);
	 */
	template <TCHAR TChar, typename... Args>
	static VOID Info(const TChar *format, Args&&... args);

	/**
	 * Error - Error messages (red)
	 *
	 * Use for: Failures, exceptions, critical issues
	 * Enabled when: LogLevel >= Default
	 * Color: Red (ANSI: \033[0;31m)
	 *
	 * USAGE: Now supports custom types like DOUBLE directly!
	 *   LOG_ERROR("Failed with code %d, value %.3f", errorCode, 1.234_embed);
	 */
	template <TCHAR TChar, typename... Args>
	static VOID Error(const TChar *format, Args&&... args);

	/**
	 * Warning - Warning messages (yellow)
	 *
	 * Use for: Non-critical issues, deprecation notices, potential problems
	 * Enabled when: LogLevel >= Default
	 * Color: Yellow (ANSI: \033[0;33m)
	 *
	 * USAGE: Now supports custom types like DOUBLE directly!
	 *   LOG_WARNING("CPU usage at %.1f%%", 85.5_embed);
	 */
	template <TCHAR TChar, typename... Args>
	static VOID Warning(const TChar *format, Args&&... args);

	/**
	 * Debug - Debug messages (yellow)
	 *
	 * Use for: Detailed diagnostic information, variable dumps, trace logs
	 * Enabled when: LogLevel >= Debug
	 * Color: Yellow (ANSI: \033[0;33m)
	 *
	 * USAGE: Now supports custom types like DOUBLE directly!
	 *   LOG_DEBUG("Calculated value: %.6f", 3.141592_embed);
	 */
	template <TCHAR TChar, typename... Args>
	static VOID Debug(const TChar *format, Args&&... args);

	/**
	 * Clear - Clear console output
	 *
	 * Clears the console screen.
	 */
	static VOID Clear();
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
 *   - Type-safe variadic templates (no VA_LIST)
 */
template <TCHAR TChar, typename... Args>
VOID Logger::Info(const TChar *format, Args&&... args)
{
	if constexpr (LogLevel != LogLevels::None)
	{
		TimestampedLogOutput<TChar>(L"\033[0;32m[INF] "_embed, format, static_cast<Args&&>(args)...);
	}
	else
	{
		(VOID) format; // Suppress unused parameter warning
		((VOID) args, ...); // Suppress unused parameter warnings for all args
	}
}

/**
 * Error - Error logging (implementation)
 *
 * Enabled for Default and Debug log levels.
 * Uses red color to highlight critical issues.
 * Type-safe variadic templates (no VA_LIST).
 */
template <TCHAR TChar, typename... Args>
VOID Logger::Error(const TChar *format, Args&&... args)
{
	if constexpr (LogLevel != LogLevels::None)
	{
		TimestampedLogOutput<TChar>(L"\033[0;31m[ERR] "_embed, format, static_cast<Args&&>(args)...);
	}
	else
	{
		(VOID) format; // Suppress unused parameter warning
		((VOID) args, ...); // Suppress unused parameter warnings for all args
	}
}

/**
 * Warning - Warning logging (implementation)
 *
 * Enabled for Default and Debug log levels.
 * Uses yellow color for non-critical warnings.
 * Type-safe variadic templates (no VA_LIST).
 */
template <TCHAR TChar, typename... Args>
VOID Logger::Warning(const TChar *format, Args&&... args)
{
	if constexpr (LogLevel != LogLevels::None)
	{
		TimestampedLogOutput<TChar>(L"\033[0;33m[WRN] "_embed, format, static_cast<Args&&>(args)...);
	}
	else
	{
		(VOID) format; // Suppress unused parameter warning
		((VOID) args, ...); // Suppress unused parameter warnings for all args
	}
}

/**
 * Debug - Debug logging (implementation)
 *
 * Only enabled when LogLevel == Debug.
 * Compile-time check eliminates debug code in production builds.
 * Type-safe variadic templates (no VA_LIST).
 */
template <TCHAR TChar, typename... Args>
VOID Logger::Debug(const TChar *format, Args&&... args)
{
	if constexpr (LogLevel == LogLevels::Debug)
	{
		TimestampedLogOutput<TChar>(L"\033[0;33m[DBG] "_embed, format, static_cast<Args&&>(args)...);
	}
	else
	{
		(VOID) format; // Suppress unused parameter warning
		((VOID) args, ...); // Suppress unused parameter warnings for all args
	}
}
