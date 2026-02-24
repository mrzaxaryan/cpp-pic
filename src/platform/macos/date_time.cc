#include "date_time.h"
#include "syscall.h"
#include "system.h"

DateTime DateTime::Now()
{
	DateTime dt;
	timeval tv;

	// Get current time using gettimeofday syscall
	SSIZE result = System::Call(SYS_GETTIMEOFDAY, (USIZE)&tv, (USIZE)0);
	if (result != 0)
	{
		// If syscall fails, return epoch time (1970-01-01 00:00:00)
		dt.Years = 1970;
		dt.Monthes = 1;
		dt.Days = 1;
		return dt;
	}

	// Convert Unix timestamp (seconds since 1970-01-01) to date/time
	UINT64 totalSeconds = (UINT64)tv.tv_sec;
	UINT64 nanoseconds = (UINT64)tv.tv_usec * 1000ULL;

	const UINT64 SECONDS_PER_DAY = 86400;

	UINT64 days = totalSeconds / SECONDS_PER_DAY;
	UINT64 timeOfDay = totalSeconds % SECONDS_PER_DAY;

	DateTime::FromDaysAndTime(dt, days, 1970, timeOfDay, nanoseconds);
	return dt;
}

UINT64 DateTime::GetMonotonicNanoseconds()
{
	timespec ts;

	// Use clock_gettime with CLOCK_MONOTONIC (macOS 10.12+)
	SSIZE result = System::Call(SYS_CLOCK_GETTIME, CLOCK_MONOTONIC, (USIZE)&ts);
	if (result != 0)
	{
		// Fallback to gettimeofday if clock_gettime not available
		timeval tv;
		result = System::Call(SYS_GETTIMEOFDAY, (USIZE)&tv, (USIZE)0);
		if (result != 0)
			return 0;
		return ((UINT64)tv.tv_sec * 1000000000ULL) + ((UINT64)tv.tv_usec * 1000ULL);
	}

	UINT64 nanoseconds = ((UINT64)ts.tv_sec * 1000000000ULL) + (UINT64)ts.tv_nsec;
	return nanoseconds;
}
