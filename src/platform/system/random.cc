#include "platform/system/random.h"
#include "platform/system/date_time.h"
#include "platform/platform.h"

static inline UINT64 GetHardwareTimestamp()
{
#if defined(ARCHITECTURE_X86_64) || defined(ARCHITECTURE_I386)
	// x86/x64: Read the Time Stamp Counter
	UINT32 lo, hi;
	__asm__ __volatile__("rdtsc" : "=a"(lo), "=d"(hi));
	return ((UINT64)hi << 32) | lo;

#elif defined(ARCHITECTURE_AARCH64)
	// ARM64: Standard 64-bit system counter access
	UINT64 virtualTimerValue;
	__asm__ __volatile__("mrs %0, cntvct_el0" : "=r"(virtualTimerValue));
	return virtualTimerValue;

#elif defined(ARCHITECTURE_ARMV7A)
	// ARMv7-A (32-bit): Use syscall-based monotonic timestamp
	// This uses clock_gettime(CLOCK_MONOTONIC) via PLATFORM
	return DateTime::GetMonotonicNanoseconds();

#else
#error "GetHardwareTimestamp not implemented for this architecture"
#endif
}

VOID Random::EnsureSeeded()
{
	if (!prng.IsSeeded())
		prng.Seed(GetHardwareTimestamp());
}
