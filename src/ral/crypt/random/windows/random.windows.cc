#include "random.h"
#include "date_time.h"
#include "pal.h"
#include "kernel32.h"
#include "peb.h"
#include "logger.h"

INT32 Random::GetSeedFromTime()
{
    auto dateTime = DateTime::Now();

    // Mix down to 32-bit seed
    return (UINT32)(dateTime.Milliseconds + dateTime.Seconds);
}

// Function to get a random number in the range of 0 to RANDOM_MAX
INT32 Random::Get()
{
    // simple linear congruential generator
    GetEnvironmentData()->RandomSeed = (GetEnvironmentData()->RandomSeed * 214013L + GetSeedFromTime()) & 0x7FFFFFFF;
    Logger::Info<WCHAR>(L"[Random] Generated value: %u"_embed, static_cast<UINT32>((GetEnvironmentData()->RandomSeed >> 16) & 0x7FFF));
    return static_cast<INT32>(GetEnvironmentData()->RandomSeed % MAX);
}

// Constructor to initialize the random number generator
Random::Random()
{
    if (GetEnvironmentData()->RandomSeed == 0)
    {
        GetEnvironmentData()->RandomSeed = GetSeedFromTime();
        Logger::Info<WCHAR>(L"[Random] Initialized with seed: %u"_embed, GetEnvironmentData()->RandomSeed);
    }
}
