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
    // Note: Cannot use static variables in PIC mode

    // Get entropy from stack addresses and uninitialized stack content
    // Uninitialized volatiles contain remnants from previous function calls
    volatile unsigned int stack_var1;
    volatile unsigned int stack_var2;
    volatile unsigned int stack_var3;
    volatile unsigned int stack_var4;

    // Read both uninitialized values (stack remnants) and addresses
    unsigned long long val1 = stack_var1;
    unsigned long long val2 = stack_var2;
    unsigned long long val3 = stack_var3;
    unsigned long long val4 = stack_var4;

    unsigned long long sp1 = (unsigned long long)(unsigned int)&stack_var1;
    unsigned long long sp2 = (unsigned long long)(unsigned int)&stack_var2;

    // Mix all entropy sources: stack addresses + stack content
    unsigned long long result = sp1 ^ sp2 ^ val1 ^ val2 ^ val3 ^ val4;

    // Apply MurmurHash3 finalizer for avalanche mixing
    result ^= result >> 33;
    result *= 0xff51afd7ed558ccdULL;
    result ^= result >> 33;
    result *= 0xc4ceb9fe1a85ec53ULL;
    result ^= result >> 33;

    return result;

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
