#include "platform/system/date_time.h"
#include "platform/os/linux/common/syscall.h"
#include "platform/os/linux/common/system.h"

DateTime DateTime::Now()
{
	DateTime dt;
	Timespec ts;

	// Get current time using clock_gettime syscall
	SSIZE result = System::Call(SYS_CLOCK_GETTIME, CLOCK_REALTIME, (USIZE)&ts);
	if (result != 0)
	{
		// If syscall fails, return epoch time (1970-01-01 00:00:00)
		dt.Years = 1970;
		dt.Months = 1;
		dt.Days = 1;
		return dt;
	}

	// Convert Unix timestamp (seconds since 1970-01-01) to date/time
	UINT64 totalSeconds = (UINT64)ts.Sec;
	UINT64 nanoseconds = (UINT64)ts.Nsec;

	constexpr UINT64 SECONDS_PER_DAY = 86400;

	UINT64 days = totalSeconds / SECONDS_PER_DAY;
	UINT64 timeOfDay = totalSeconds % SECONDS_PER_DAY;

	DateTime::FromDaysAndTime(dt, days, 1970, timeOfDay, nanoseconds);
	return dt;
}

UINT64 DateTime::GetMonotonicNanoseconds()
{
	Timespec ts;

	// Get monotonic time (not affected by system clock changes)
	SSIZE result = System::Call(SYS_CLOCK_GETTIME, CLOCK_MONOTONIC, (USIZE)&ts);
	if (result != 0)
	{
		// Fallback: return 0 if syscall fails
		return 0;
	}

	// Convert to nanoseconds
	UINT64 nanoseconds = ((UINT64)ts.Sec * 1000000000ULL) + (UINT64)ts.Nsec;
	return nanoseconds;
}
