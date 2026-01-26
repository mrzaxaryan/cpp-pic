#include "random.h"
#include "date_time.h"
#include "pal.h"

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
    // Check if the seed is zero, if so, set it to a default value
    this->seed = (this->seed * 214013L + GetSeedFromTime()) & 0x7fffffff;
    // Generate a new random number using a linear congruential generator formula
    return (INT32)(this->seed % Random::MAX);
}

// Constructor to initialize the random number generator
Random::Random()
{
    this->seed = (UINT32)GetSeedFromTime();
}
