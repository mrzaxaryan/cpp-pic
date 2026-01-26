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
    UINT32 dummmyVar = 0;
    this->seed = (this->seed * 214013L + 2531011L + (INT32)(SSIZE)(PVOID)&dummmyVar) & 0xffffffff;
    // Generate a new random number using a linear congruential generator formula
    return (INT32)(this->seed % Random::MAX);
}

// Constructor to initialize the random number generator
Random::Random()
{
    // Seed using high-resolution time + this pointer + stack variable
    SSIZE ptr = (SSIZE)this;
    SSIZE stack_var = (SSIZE)&ptr;
    SSIZE seedFromTime = GetSeedFromTime();
    this->seed = (UINT32)((seedFromTime ^ ptr ^ stack_var ^ (SSIZE)&seedFromTime) & 0xFFFFFFFF);
}
