#include "random.h"
#include "date_time.h"
#include <stdint.h>

// Helper to mix entropy bits (prevents similar seeds from producing similar sequences)
static uint32_t MixBits(uint32_t x)
{
    x = ((x >> 16) ^ x) * 0x45d9f3b;
    x = ((x >> 16) ^ x) * 0x45d9f3b;
    x = (x >> 16) ^ x;
    return x;
}

INT32 Random::GetSeedFromTime()
{
    auto dateTime = DateTime::Now();

    // Combine ms, us, and seconds into a 64-bit block
    UINT64 t = 0;
    t |= (UINT64)dateTime.Milliseconds << 32;
    t |= (UINT64)dateTime.Microseconds << 12;
    t |= (UINT64)dateTime.Second;

    // Fold 64-bit time into 32-bit using XOR
    return (UINT32)(t ^ (t >> 32));
}

Random::Random()
{
    // Collect various sources of "noise"
    UINT32 timeSeed = (UINT32)GetSeedFromTime();
    USIZE ptrSeed = (USIZE)this;

    // Mix them together
    this->seed = MixBits(timeSeed ^ (UINT32)ptrSeed);

    // Xorshift seed must never be 0
    if (this->seed == 0)
    {
        this->seed = 0xACE1u;
    }
}

INT32 Random::Get()
{
    // Xorshift32 algorithm: Fast and passes most statistical tests
    UINT32 x = this->seed;
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    this->seed = x;

    // Use 64-bit math to map the result to [0, Random::MAX)
    // This significantly reduces "Modulo Bias"
    return (INT32)(((UINT64)x * (UINT64)Random::MAX) >> 32);
}