#include "random.h"
#include "date_time.h"
#include "pal.h"
#include "kernel32.h"
#include "peb.h"
#include "logger.h"

static inline UINT64 GetHardwareTimestamp()
{
#if defined(PLATFORM_WINDOWS_X86_64) || defined(PLATFORM_WINDOWS_I386)
    // x86/x64: Read the Time Stamp Counter
    unsigned int lo, hi;
    __asm__ __volatile__("rdtsc" : "=a"(lo), "=d"(hi));
    return ((UINT64)hi << 32) | lo;

#elif defined(PLATFORM_WINDOWS_ARMV7A) || defined(PLATFORM_WINDOWS_AARCH64)
    // ARM64: Read the Virtual Counter Register (CNTVCT_EL0)
    UINT64 virtual_timer_value;
    __asm__ __volatile__("mrs %0, cntvct_el0" : "=r"(virtual_timer_value));
    return virtual_timer_value;

#else
#error "GetHardwareTimestamp not implemented for this architecture"
#endif
}

// Function to get a random number in the range of 0 to RANDOM_MAX
// Not the best one but it works
INT32 Random::Get()
{
    // simple linear congruential generator
    seed = (seed * GetHardwareTimestamp() + (UINT64)214013) & 0x7FFFFFFF;
    Logger::Debug<WCHAR>(L"[Random] Generated value: %u"_embed, static_cast<UINT32>((seed >> 16) & 0x7FFF));
    return static_cast<INT32>(seed % MAX);
}

// Constructor to initialize the random number generator
Random::Random()
{
    seed = GetHardwareTimestamp();
    Logger::Debug<WCHAR>(L"[Random] Initialized with seed: %llu"_embed, seed);
}
