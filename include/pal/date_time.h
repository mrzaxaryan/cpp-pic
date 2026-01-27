#pragma once

#include "bal.h" // primitives, TCHAR

// Fixed-size string template
template <TCHAR TChar, USIZE N>
struct FixedString
{
private:
    TChar data[N]{}; // Fixed-size character array

public:
    VOID *operator new(USIZE) = delete;    // Disable dynamic allocation
    VOID operator delete(VOID *) = delete; // Disable dynamic deallocation

    // Operators for conversion and indexing
    explicit constexpr operator const TChar *() const { return data; }
    explicit constexpr operator TChar *() { return data; }
    constexpr TChar &operator[](USIZE i) { return data[i]; }
};

template <TCHAR TChar>
using TimeOnlyString = FixedString<TChar, 9>; // "HH:MM:SS\0"

template <TCHAR TChar>
using DateOnlyString = FixedString<TChar, 11>; // "YYYY-MM-DD\0"

// Full date-time
template <TCHAR TChar>
using DateTimeString = FixedString<TChar, 20>; // "YYYY-MM-DD HH:MM:SS\0"

// Class to represent date and time
class DateTime
{
private:
    // Helper functions to convert numbers to strings
    template <TCHAR TChar>
    static constexpr VOID Put2(TChar *dst, UINT32 v)
    {
        dst[0] = (TChar)('0' + ((v / 10u) % 10u));
        dst[1] = (TChar)('0' + (v % 10u));
    }
    // Helper function to convert 4-digit year to string
    template <TCHAR TChar>
    static constexpr VOID Put4(TChar *dst, UINT32 v)
    {
        dst[0] = (TChar)('0' + ((v / 1000u) % 10u));
        dst[1] = (TChar)('0' + ((v / 100u) % 10u));
        dst[2] = (TChar)('0' + ((v / 10u) % 10u));
        dst[3] = (TChar)('0' + (v % 10u));
    }

public:
    // Date and time components
    UINT64 Years = 0;
    UINT32 Monthes = 0;
    UINT32 Days = 0;
    UINT32 Hours = 0;
    UINT32 Minutes = 0;
    UINT32 Seconds = 0;
    UINT64 Milliseconds = 0;
    UINT64 Microseconds = 0;
    UINT64 Nanoseconds = 0;
    // Disable dynamic allocation and deallocation
    VOID *operator new(USIZE) = delete;
    VOID operator delete(VOID *) = delete;

    // time only: HH:MM:SS
    template <TCHAR TChar>
    constexpr TimeOnlyString<TChar> ToTimeOnlyString() const
    {
        TimeOnlyString<TChar> out{};

        Put2<TChar>(&out[0], (UINT32)Hours);
        out[2] = (TChar)':';
        Put2<TChar>(&out[3], (UINT32)Minutes);
        out[5] = (TChar)':';
        Put2<TChar>(&out[6], (UINT32)Seconds);

        out[8] = (TChar)0;
        return out;
    }

    // date only: YYYY-MM-DD
    template <TCHAR TChar>
    constexpr DateOnlyString<TChar> ToDateOnlyString() const
    {
        DateOnlyString<TChar> out{};

        Put4<TChar>(&out[0], (UINT32)Years);
        out[4] = (TChar)'-';
        Put2<TChar>(&out[5], (UINT32)Monthes);
        out[7] = (TChar)'-';
        Put2<TChar>(&out[8], (UINT32)Days);

        out[10] = (TChar)0;
        return out;
    }

    // full: YYYY-MM-DD HH:MM:SS
    template <TCHAR TChar>
    constexpr DateTimeString<TChar> ToDateTimeString() const
    {
        DateTimeString<TChar> out{};

        // Date
        Put4<TChar>(&out[0], (UINT32)Years);
        out[4] = (TChar)'-';
        Put2<TChar>(&out[5], (UINT32)Monthes);
        out[7] = (TChar)'-';
        Put2<TChar>(&out[8], (UINT32)Days);
        out[10] = (TChar)' ';

        // Time
        Put2<TChar>(&out[11], (UINT32)Hours);
        out[13] = (TChar)':';
        Put2<TChar>(&out[14], (UINT32)Minutes);
        out[16] = (TChar)':';
        Put2<TChar>(&out[17], (UINT32)Seconds);

        out[19] = (TChar)0;
        return out;
    }

    // Optional: keep your old names as aliases (so existing code still compiles)
    template <TCHAR TChar>
    constexpr TimeOnlyString<TChar> ToTimeString() const
    {
        return ToTimeOnlyString<TChar>();
    }

    template <TCHAR TChar>
    constexpr DateOnlyString<TChar> ToDateString() const
    {
        return ToDateOnlyString<TChar>();
    }

    // Structured time
    static DateTime Now();
};
