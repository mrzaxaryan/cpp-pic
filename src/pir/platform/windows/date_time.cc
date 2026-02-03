#include "date_time.h"
#include "primitives.h"
#include "string.h"
#include "console.h"

#define MM_SHARED_USER_DATA_VA 0x7FFE0000 // address of the shared user data in the process address space

// System time structure
typedef struct _KSYSTEM_TIME
{
    UINT32 LowPart;
    INT32 High1Time;
    INT32 High2Time;
} KSYSTEM_TIME;

// KUSER_SHARED_DATA
typedef struct _USER_SHARED_DATA
{
    UINT32 TickCountLowDeprecated;       // 0x0
    UINT32 TickCountMultiplier;          // 0x4
    volatile KSYSTEM_TIME InterruptTime; // 0x8
    volatile KSYSTEM_TIME SystemTime;    // 0x14
    volatile KSYSTEM_TIME TimeZoneBias;  // 0x20
} USER_SHARED_DATA, *PUSER_SHARED_DATA;

// Macro to get the USER_SHARED_DATA structure
#define GetUserSharedData() ((PUSER_SHARED_DATA)MM_SHARED_USER_DATA_VA)

// Read KSYSTEM_TIME as UINT64
UINT64 readKSystemTimeU64(volatile const KSYSTEM_TIME *t)
{
    KSYSTEM_TIME v;
    do
    {
        v.High1Time = t->High1Time;
        v.LowPart = t->LowPart;
    } while (v.High1Time != t->High2Time);

    return (((UINT64)(UINT32)v.High1Time) << 32) | ((UINT64)v.LowPart);
}

// Read KSYSTEM_TIME as INT64
INT64 readKSystemTimeS64(volatile const KSYSTEM_TIME *t)
{
    KSYSTEM_TIME v;
    do
    {
        v.High1Time = t->High1Time;
        v.LowPart = t->LowPart;
    } while (v.High1Time != t->High2Time);

    return (INT64)(((UINT64)(UINT32)v.High1Time << 32) | (UINT64)v.LowPart);
}

// Get the current local date and time
DateTime DateTime::Now()
{
    DateTime dt;

    volatile USER_SHARED_DATA *usd = GetUserSharedData();

    // 1) UTC time (100ns since 1601-01-01)
    UINT64 utc100ns = readKSystemTimeU64(&usd->SystemTime);

    // 2) TimeZoneBias (signed 100ns). local = utc - bias
    INT64 bias100ns = readKSystemTimeS64(&usd->TimeZoneBias);
    INT64 utc100ns_s = (INT64)utc100ns;
    INT64 local100ns_s = utc100ns_s - bias100ns;
    UINT64 local100ns = (UINT64)local100ns_s;

    // 3) Split into date/time
    const UINT64 TICKS_PER_SEC = 10000000ULL;
    const UINT64 TICKS_PER_DAY = 86400ULL * TICKS_PER_SEC;

    UINT64 days = local100ns / TICKS_PER_DAY;
    UINT64 dayTicks = local100ns % TICKS_PER_DAY;

    // ----- inline: days since 1601-01-01 -> (Y,M,D) -----
    UINT64 year = 1601;

    while (1)
    {
        UINT32 diy = DateTime::IsLeapYear(year) ? 366u : 365u;
        if (days >= diy)
        {
            days -= diy;
            year++;
        }
        else
            break;
    }

    UINT32 mdays_norm[12];
    mdays_norm[0] = 31;
    mdays_norm[1] = 28;
    mdays_norm[2] = 31;
    mdays_norm[3] = 30;
    mdays_norm[4] = 31;
    mdays_norm[5] = 30;
    mdays_norm[6] = 31;
    mdays_norm[7] = 31;
    mdays_norm[8] = 30;
    mdays_norm[9] = 31;
    mdays_norm[10] = 30;
    mdays_norm[11] = 31;

    UINT32 mdays[12];
    for (INT32 i = 0; i < 12; ++i)
        mdays[i] = mdays_norm[i];

    // Adjust for leap year
    if (DateTime::IsLeapYear(year))
        mdays[1] = 29;

    UINT32 month = 0;
    while (month < 12 && days >= mdays[month])
    {
        days -= mdays[month];
        month++;
    }

    dt.Years = year;
    dt.Monthes = month + 1;     // 1..12
    dt.Days = (UINT32)days + 1; // 1..31

    // time-of-day
    UINT64 total_secs = dayTicks / TICKS_PER_SEC;
    dt.Hours = (UINT32)(total_secs / UINT64(3600u));
    dt.Minutes = (UINT32)((total_secs / UINT64(60u)) % UINT64(60u));
    dt.Seconds = (UINT32)(total_secs % UINT64(60u));

    // sub-second from remaining 100ns ticks
    UINT64 sub100ns = dayTicks % TICKS_PER_SEC;                 // 0..9,999,999 (100ns units)
    dt.Milliseconds = sub100ns / UINT64(10000u);                // 1 ms = 10,000 * 100ns
    dt.Microseconds = (sub100ns / UINT64(10u)) % UINT64(1000u); // 1 us = 10 * 100ns
    dt.Nanoseconds = (sub100ns % UINT64(10u)) * UINT64(100u);   // remainder * 100ns -> ns
    return dt;
}

UINT64 DateTime::GetMonotonicNanoseconds()
{
    volatile USER_SHARED_DATA *usd = GetUserSharedData();

    // Read InterruptTime (monotonic, unaffected by system clock changes)
    UINT64 interruptTime100ns = readKSystemTimeU64(&usd->InterruptTime);

    // Convert from 100ns units to nanoseconds
    UINT64 nanoseconds = interruptTime100ns * UINT64(100u);
    return nanoseconds;
}