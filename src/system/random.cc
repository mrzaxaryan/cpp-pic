#include "random.h"
#include "date_time.h"
#include "platform.h"

static inline UINT64 GetHardwareTimestamp()
{
#if defined(ARCHITECTURE_X86_64) || defined(ARCHITECTURE_I386)
    // x86/x64: Read the Time Stamp Counter
    UINT32 lo, hi;
    __asm__ __volatile__("rdtsc" : "=a"(lo), "=d"(hi));
    return ((UINT64)hi << 32) | lo;

#elif defined(ARCHITECTURE_AARCH64)
    // ARM64: Standard 64-bit system counter access
    UINT64 virtual_timer_value;
    __asm__ __volatile__("mrs %0, cntvct_el0" : "=r"(virtual_timer_value));
    return virtual_timer_value;

#elif defined(ARCHITECTURE_ARMV7A)
    // ARMv7-A (32-bit): Use syscall-based monotonic timestamp
    // This uses clock_gettime(CLOCK_MONOTONIC) via PLATFORM
    return DateTime::GetMonotonicNanoseconds();

#else
#error "GetHardwareTimestamp not implemented for this architecture"
#endif
}

// Function to get a random number in the range of 0 to RANDOM_MAX
INT32 Random::Get()
{
    // Lazy seed on first call
    if (seed == 0)
        seed = GetHardwareTimestamp();

    // xorshift64 (Marsaglia)
    seed ^= seed << 13;
    seed ^= seed >> 7;
    seed ^= seed << 17;
    return static_cast<INT32>(seed & 0x7FFFFFFF);
}

// Function to fill a buffer with random bytes
INT32 Random::GetArray(Span<UINT8> buffer)
{
    USIZE size = buffer.Size();
    // Fill the buffer with random bytes
    for (USIZE i = 0; i < size; ++i)
        buffer[i] = (UINT8)(Random::Get() & 0xFF); // Get random byte
    return 1;                                      // Indicate success
}