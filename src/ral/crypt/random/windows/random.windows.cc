#include "random.h"
#include "date_time.h"
#include "pal.h"
#include "kernel32.h"

INT32 Random::GetSeedFromTime()
{
    auto dateTime = DateTime::Now();

    // Make a 64-bit value combining all time components
    UINT64 t = 0;
    t |= (UINT64)dateTime.Milliseconds << 32; // cast first!
    t |= (UINT64)dateTime.Microseconds << 12; // shift enough to avoid overlap
    t |= (UINT64)dateTime.Second;             // seconds in lowest bits

    // Mix down to 32-bit seed
    return (UINT32)(t ^ (t >> 32));
}

// Function to get a random number in the range of 0 to RANDOM_MAX
INT32 Random::Get()
{
    // simple linear congruential generator
    seed = (seed * 214013L + 2531011L) & 0x7FFFFFFF;
    return static_cast<INT32>(seed % MAX);
}

// Constructor to initialize the random number generator
Random::Random()
{
    seed = static_cast<UINT32>(Kernel32::GetTickCount64() & 0xFFFFFFFF);
}
