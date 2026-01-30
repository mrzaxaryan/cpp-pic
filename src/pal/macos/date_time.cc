#include "date_time.h"
#include "syscall.h"
#include "primitives.h"

// macOS syscall number for gettimeofday
constexpr USIZE SYS_GETTIMEOFDAY = 116;

// macOS timeval structure
struct timeval
{
    SSIZE tv_sec;   // seconds
    SSIZE tv_usec;  // microseconds
};

// Helper: check if a year is a leap year
static BOOL isLeapYear(UINT64 year)
{
    return (year % 4 == 0) && ((year % 100 != 0) || (year % 400 == 0));
}

DateTime DateTime::Now()
{
    DateTime dt;
    timeval tv;

    // Get current time using gettimeofday syscall
    // Second argument (timezone) is obsolete and should be NULL
    SSIZE result = Syscall::syscall2(SYS_GETTIMEOFDAY, (USIZE)&tv, 0);
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
    UINT64 microseconds = (UINT64)tv.tv_usec;

    // Constants
    const UINT64 SECONDS_PER_DAY = 86400;
    const UINT64 SECONDS_PER_HOUR = 3600;
    const UINT64 SECONDS_PER_MINUTE = 60;

    // Calculate days since Unix epoch (1970-01-01)
    UINT64 days = totalSeconds / SECONDS_PER_DAY;
    UINT64 timeOfDay = totalSeconds % SECONDS_PER_DAY;

    // Time of day
    dt.Hours = (UINT32)(timeOfDay / SECONDS_PER_HOUR);
    dt.Minutes = (UINT32)((timeOfDay % SECONDS_PER_HOUR) / SECONDS_PER_MINUTE);
    dt.Seconds = (UINT32)(timeOfDay % SECONDS_PER_MINUTE);

    // Sub-second precision
    dt.Milliseconds = microseconds / 1000;
    dt.Microseconds = microseconds % 1000;
    dt.Nanoseconds = 0;  // gettimeofday doesn't provide nanosecond precision

    // Convert days since 1970-01-01 to (Year, Month, Day)
    UINT64 year = 1970;

    // Fast-forward through years
    while (TRUE)
    {
        UINT32 daysInYear = isLeapYear(year) ? 366 : 365;
        if (days >= daysInYear)
        {
            days -= daysInYear;
            year++;
        }
        else
        {
            break;
        }
    }

    // Days in each month (non-leap year)
    UINT32 daysInMonth[12] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

    // Adjust February for leap year
    if (isLeapYear(year))
        daysInMonth[1] = 29;

    // Find the month
    UINT32 month = 0;
    while (month < 12 && days >= daysInMonth[month])
    {
        days -= daysInMonth[month];
        month++;
    }

    dt.Years = year;
    dt.Monthes = month + 1;  // Months are 1-indexed
    dt.Days = (UINT32)days + 1;  // Days are 1-indexed

    return dt;
}
