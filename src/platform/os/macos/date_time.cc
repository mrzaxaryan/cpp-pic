#include "platform/system/date_time.h"
#include "platform/os/macos/common/syscall.h"
#include "platform/os/macos/common/system.h"

DateTime DateTime::Now()
{
	DateTime dt;
	Timeval tv;

	// Get current time using gettimeofday syscall
	// Note: macOS gettimeofday returns seconds in RAX (retval[0]) on success,
	// not 0. On error, System::Call returns negative (carry flag convention).
	SSIZE result = System::Call(SYS_GETTIMEOFDAY, (USIZE)&tv, (USIZE)0);
	if (result < 0)
	{
		// If syscall fails, return epoch time (1970-01-01 00:00:00)
		dt.Years = 1970;
		dt.Months = 1;
		dt.Days = 1;
		return dt;
	}

	// Convert Unix timestamp (seconds since 1970-01-01) to date/time
	UINT64 totalSeconds = (UINT64)tv.Sec;
	UINT64 nanoseconds = (UINT64)tv.Usec * 1000ULL;

	constexpr UINT64 SECONDS_PER_DAY = 86400;

	UINT64 days = totalSeconds / SECONDS_PER_DAY;
	UINT64 timeOfDay = totalSeconds % SECONDS_PER_DAY;

	DateTime::FromDaysAndTime(dt, days, 1970, timeOfDay, nanoseconds);
	return dt;
}

UINT64 DateTime::GetMonotonicNanoseconds()
{
	// macOS has no clock_gettime BSD syscall — it's userspace-only via commpage.
	// Use gettimeofday instead (not truly monotonic, but functional).
	Timeval tv;
	SSIZE result = System::Call(SYS_GETTIMEOFDAY, (USIZE)&tv, (USIZE)0);
	if (result < 0)
		return 0;
	return ((UINT64)tv.Sec * 1000000000ULL) + ((UINT64)tv.Usec * 1000ULL);
}
