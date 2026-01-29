#include "random.h"
#include "date_time.h"
#include "pal.h"
#include "kernel32.h"
#include "peb.h"
#include "logger.h"

INT32 Random::GetSeedFromTime()
{
    // Get current date and time
    auto dateTime = DateTime::Now();

    // Mix down to 32-bit seed
    return (UINT32)(dateTime.Nanoseconds * dateTime.Seconds);
}

// Function to get a random number in the range of 0 to RANDOM_MAX
INT32 Random::Get()
{
    // simple linear congruential generator
    seed = (seed * 214013L + GetSeedFromTime()) & 0x7FFFFFFF;
    Logger::Debug<WCHAR>(L"[Random] Generated value: %u"_embed, static_cast<UINT32>((seed >> 16) & 0x7FFF));
    return static_cast<INT32>(seed % MAX);
}

// Constructor to initialize the random number generator
Random::Random()
{
    seed = GetSeedFromTime();
    Logger::Debug<WCHAR>(L"[Random] Initialized with seed: %u"_embed, seed);
}
