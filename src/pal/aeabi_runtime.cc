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
}

#endif // ARCHITECTURE_ARMV7A 
