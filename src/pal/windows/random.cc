#include "random.h"
#include "date_time.h"
#include "pal.h"
#include "kernel32.h"
#include "peb.h"
#include "logger.h"

static inline unsigned long long GetHardwareTimestamp()
{
#if defined(__x86_64__) || defined(_M_X64) || defined(__i386__) || defined(_M_IX86)
    // x86/x64: Read the Time Stamp Counter
    unsigned int lo, hi;
    __asm__ __volatile__("rdtsc" : "=a"(lo), "=d"(hi));
    return ((unsigned long long)hi << 32) | lo;

#elif defined(__aarch64__) || defined(_M_ARM64)
    // ARM64: Read the Virtual Counter Register (CNTVCT_EL0)
    unsigned long long virtual_timer_value;
    __asm__ __volatile__("mrs %0, cntvct_el0" : "=r"(virtual_timer_value));
    return virtual_timer_value;

#else
    // Fallback for other architectures (less precise)
    return 0;
#endif
}

UINT64 Random::GetSeedFromTime()
{

    // Mix down to 32-bit seed
    return (UINT64)(GetHardwareTimestamp());
}

// Function to get a random number in the range of 0 to RANDOM_MAX
INT32 Random::Get()
{
    // simple linear congruential generator
    seed = (seed * GetSeedFromTime() + (UINT64)214013) & 0x7FFFFFFF;
    Logger::Debug<WCHAR>(L"[Random] Generated value: %u"_embed, static_cast<UINT32>((seed >> 16) & 0x7FFF));
    return static_cast<INT32>(seed % MAX);
}

// Constructor to initialize the random number generator
Random::Random()
{
    seed = GetSeedFromTime();
    Logger::Debug<WCHAR>(L"[Random] Initialized with seed: %llu"_embed, seed);
}
