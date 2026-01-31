// ARM EABI Runtime Support for Division Operations
// Required when building with -nostdlib on ARM platforms
// These functions are called by the compiler for integer division/modulo operations

#include "primitives.h"

#if defined(ARCHITECTURE_ARMV7A)

// Disable LTO and inlining for EABI functions to preserve their exact signatures
#define AEABI_FUNC __attribute__((noinline, used, optnone))

extern "C"
{
    // Helper function for unsigned division - returns quotient
    static UINT32 udiv_internal(UINT32 numerator, UINT32 denominator, UINT32 *remainder)
    {
        if (denominator == 0)
        {
            if (remainder)
                *remainder = numerator;
            return 0;
        }

        // Fast path for powers of 2
        if ((denominator & (denominator - 1)) == 0)
        {
            UINT32 shift = 0;
            UINT32 temp = denominator;
            while ((temp & 1) == 0)
            {
                temp >>= 1;
                shift++;
            }
            if (remainder)
                *remainder = numerator & (denominator - 1);
            return numerator >> shift;
        }

        // Software division algorithm
        UINT32 quotient = 0;
        UINT32 rem = 0;

        for (INT32 i = 31; i >= 0; i--)
        {
            rem = (rem << 1) | ((numerator >> i) & 1);
            if (rem >= denominator)
            {
                rem -= denominator;
                quotient |= (1U << i);
            }
        }

        if (remainder)
            *remainder = rem;
        return quotient;
    }

    // ARM EABI: unsigned division - returns quotient in r0
    AEABI_FUNC UINT32 __aeabi_uidiv(UINT32 numerator, UINT32 denominator)
    {
        return udiv_internal(numerator, denominator, NULL);
    }

    // ARM EABI: unsigned division with modulo - returns quotient in r0, remainder in r1
    // The struct layout matches ARM register passing: first field in r0, second in r1
    AEABI_FUNC unsigned long long __aeabi_uidivmod(UINT32 numerator, UINT32 denominator)
    {
        UINT32 remainder = 0;
        UINT32 quotient = udiv_internal(numerator, denominator, &remainder);

        // Pack into 64-bit value: quotient (low 32 bits) and remainder (high 32 bits)
        // This matches ARM EABI convention: r0=quotient, r1=remainder
        return ((unsigned long long)remainder << 32) | quotient;
    }

    // ARM EABI: signed division with modulo - returns quotient in r0, remainder in r1
    AEABI_FUNC long long __aeabi_idivmod(INT32 numerator, INT32 denominator)
    {
        if (denominator == 0)
        {
            return ((long long)numerator << 32) | 0;
        }

        // Handle signs
        INT32 sign_num = numerator < 0 ? -1 : 1;
        INT32 sign_den = denominator < 0 ? -1 : 1;
        INT32 sign_quot = sign_num * sign_den;

        // Work with absolute values
        UINT32 abs_num = (UINT32)(numerator < 0 ? -numerator : numerator);
        UINT32 abs_den = (UINT32)(denominator < 0 ? -denominator : denominator);

        // Perform unsigned division
        UINT32 remainder = 0;
        UINT32 quotient = udiv_internal(abs_num, abs_den, &remainder);

        // Apply signs to results
        INT32 signed_quot = sign_quot < 0 ? -(INT32)quotient : (INT32)quotient;
        INT32 signed_rem = sign_num < 0 ? -(INT32)remainder : (INT32)remainder;

        // Pack into 64-bit value: quotient (low 32 bits) and remainder (high 32 bits)
        return ((long long)signed_rem << 32) | (UINT32)signed_quot;
    }

    // ARM EABI: signed division - returns quotient only
    AEABI_FUNC INT32 __aeabi_idiv(INT32 numerator, INT32 denominator)
    {
        if (denominator == 0)
            return 0;

        // Handle signs
        INT32 sign_num = numerator < 0 ? -1 : 1;
        INT32 sign_den = denominator < 0 ? -1 : 1;
        INT32 sign_quot = sign_num * sign_den;

        // Work with absolute values
        UINT32 abs_num = (UINT32)(numerator < 0 ? -numerator : numerator);
        UINT32 abs_den = (UINT32)(denominator < 0 ? -denominator : denominator);

        // Perform unsigned division
        UINT32 quotient = udiv_internal(abs_num, abs_den, NULL);

        // Apply sign to result
        return sign_quot < 0 ? -(INT32)quotient : (INT32)quotient;
    }

    // Helper for 64-bit unsigned division
    static void udiv64_internal(unsigned long long numerator, unsigned long long denominator,
                                 unsigned long long *quotient, unsigned long long *remainder)
    {
        if (denominator == 0)
        {
            *quotient = 0;
            *remainder = numerator;
            return;
        }

        // Software 64-bit division algorithm
        unsigned long long q = 0;
        unsigned long long r = 0;

        for (int i = 63; i >= 0; i--)
        {
            // Shift remainder left by 1
            r <<= 1;

            // Set the lowest bit of remainder to bit i of numerator
            if ((numerator >> i) & 1ULL)
                r |= 1ULL;

            if (r >= denominator)
            {
                r -= denominator;
                q |= (1ULL << i);
            }
        }

        *quotient = q;
        *remainder = r;
    }

    // ARM EABI: 64-bit unsigned division with modulo
    // Returns {quotient, remainder} as a struct in r0:r1 and r2:r3
    struct uldivmod_return { unsigned long long quot; unsigned long long rem; };

    AEABI_FUNC struct uldivmod_return __aeabi_uldivmod(unsigned long long numerator, unsigned long long denominator)
    {
        struct uldivmod_return result;
        udiv64_internal(numerator, denominator, &result.quot, &result.rem);
        return result;
    }

    // ARM EABI: 64-bit logical shift right
    AEABI_FUNC unsigned long long __aeabi_llsr(unsigned long long value, int shift)
    {
        if (shift < 0 || shift >= 64)
            return 0;
        return value >> shift;
    }

    // ARM EABI: 64-bit logical shift left
    AEABI_FUNC unsigned long long __aeabi_llsl(unsigned long long value, int shift)
    {
        if (shift < 0 || shift >= 64)
            return 0;
        return value << shift;
    }
}

#endif // ARCHITECTURE_ARMV7A

#if defined(ARCHITECTURE_I386)

// x86 compiler runtime support for 64-bit division on 32-bit platform
// Required when building with -nostdlib on i386
#define X86_RUNTIME_FUNC __attribute__((noinline, used, optnone))

extern "C"
{
    // Helper for 64-bit unsigned division
    static void udiv64_internal(unsigned long long numerator, unsigned long long denominator,
                                 unsigned long long *quotient, unsigned long long *remainder)
    {
        if (denominator == 0)
        {
            *quotient = 0;
            *remainder = numerator;
            return;
        }

        // Software 64-bit division algorithm
        unsigned long long q = 0;
        unsigned long long r = 0;

        for (int i = 63; i >= 0; i--)
        {
            // Shift remainder left by 1
            r <<= 1;

            // Set the lowest bit of remainder to bit i of numerator
            if ((numerator >> i) & 1ULL)
                r |= 1ULL;

            if (r >= denominator)
            {
                r -= denominator;
                q |= (1ULL << i);
            }
        }

        *quotient = q;
        *remainder = r;
    }

    // x86: 64-bit unsigned division - returns quotient
    X86_RUNTIME_FUNC unsigned long long __udivdi3(unsigned long long numerator, unsigned long long denominator)
    {
        unsigned long long quotient, remainder;
        udiv64_internal(numerator, denominator, &quotient, &remainder);
        return quotient;
    }

    // x86: 64-bit unsigned modulo - returns remainder
    X86_RUNTIME_FUNC unsigned long long __umoddi3(unsigned long long numerator, unsigned long long denominator)
    {
        unsigned long long quotient, remainder;
        udiv64_internal(numerator, denominator, &quotient, &remainder);
        return remainder;
    }

    // x86: 64-bit signed division - returns quotient
    X86_RUNTIME_FUNC long long __divdi3(long long numerator, long long denominator)
    {
        if (denominator == 0)
            return 0;

        // Handle signs
        int sign_num = numerator < 0 ? -1 : 1;
        int sign_den = denominator < 0 ? -1 : 1;
        int sign_quot = sign_num * sign_den;

        // Work with absolute values
        unsigned long long abs_num = (unsigned long long)(numerator < 0 ? -numerator : numerator);
        unsigned long long abs_den = (unsigned long long)(denominator < 0 ? -denominator : denominator);

        // Perform unsigned division
        unsigned long long quotient, remainder;
        udiv64_internal(abs_num, abs_den, &quotient, &remainder);

        // Apply sign to quotient
        return sign_quot < 0 ? -(long long)quotient : (long long)quotient;
    }

    // x86: 64-bit signed modulo - returns remainder
    X86_RUNTIME_FUNC long long __moddi3(long long numerator, long long denominator)
    {
        if (denominator == 0)
            return numerator;

        // Handle signs
        int sign_num = numerator < 0 ? -1 : 1;

        // Work with absolute values
        unsigned long long abs_num = (unsigned long long)(numerator < 0 ? -numerator : numerator);
        unsigned long long abs_den = (unsigned long long)(denominator < 0 ? -denominator : denominator);

        // Perform unsigned division
        unsigned long long quotient, remainder;
        udiv64_internal(abs_num, abs_den, &quotient, &remainder);

        // Apply sign to remainder (remainder takes sign of numerator)
        return sign_num < 0 ? -(long long)remainder : (long long)remainder;
    }
}

#endif // ARCHITECTURE_I386

#if defined(ARCHITECTURE_ARMV7A) && defined(PLATFORM_WINDOWS)

// Windows ARM runtime support
// Required when building with -nostdlib on Windows ARM platforms
#define WIN_ARM_RUNTIME_FUNC __attribute__((noinline, used, optnone))

extern "C"
{
    // __chkstk: Stack probing function required by Windows ARM ABI
    // Called before large stack allocations to ensure stack pages are committed
    // Input: r4 = size to probe (in bytes)
    // Must preserve all registers except r4, r5, r12
    WIN_ARM_RUNTIME_FUNC void __chkstk(void)
    {
        // Stub implementation - in a real embedded environment without
        // virtual memory, stack probing is not needed. We just return.
        // The compiler will inline this to essentially a no-op.
        return;
    }

    // Helper for 64-bit unsigned division (Windows ARM specific)
    static unsigned long long rt_udiv64_internal(unsigned long long numerator, unsigned long long denominator)
    {
        if (denominator == 0)
            return 0;

        // Software 64-bit division algorithm
        unsigned long long q = 0;
        unsigned long long r = 0;

        for (int i = 63; i >= 0; i--)
        {
            r <<= 1;
            if ((numerator >> i) & 1ULL)
                r |= 1ULL;
            if (r >= denominator)
            {
                r -= denominator;
                q |= (1ULL << i);
            }
        }

        return q;
    }

    // Windows ARM: 64-bit unsigned division - returns quotient
    WIN_ARM_RUNTIME_FUNC unsigned long long __rt_udiv64(unsigned long long numerator, unsigned long long denominator)
    {
        return rt_udiv64_internal(numerator, denominator);
    }
}

#endif // ARCHITECTURE_ARMV7A && PLATFORM_WINDOWS
