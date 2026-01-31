#include "random.h"

static inline UINT64 GetHardwareTimestamp()
{
#if defined(ARCHITECTURE_X86_64) || defined(ARCHITECTURE_I386)
    // x86/x64: Read the Time Stamp Counter
    unsigned int lo, hi;
    __asm__ __volatile__("rdtsc" : "=a"(lo), "=d"(hi));
    return ((UINT64)hi << 32) | lo;

#elif defined(ARCHITECTURE_AARCH64)
    // ARM64: Standard 64-bit system counter access
    UINT64 virtual_timer_value;
    __asm__ __volatile__("mrs %0, cntvct_el0" : "=r"(virtual_timer_value));
    return virtual_timer_value;

#elif defined(ARCHITECTURE_ARMV7A)
    // ARMv7-A (32-bit): User-space timestamp using software counters
    // Hardware counters require kernel/QEMU config to enable user access
    static UINT64 counter = 0;

    // Get entropy from stack address (safe and portable)
    volatile unsigned int stack_var = 0;
    UINT64 sp_entropy = (UINT64)(USIZE)&stack_var;

    // Simple mixing using counter and stack position
    // This provides sufficient entropy for random number generation
    counter++;
    return (counter * 6364136223846793005ULL) ^ (sp_entropy * 1103515245ULL);

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
    return static_cast<INT32>(seed % MAX);
}

// Constructor to initialize the random number generator
Random::Random()
{
    seed = GetHardwareTimestamp();
}
