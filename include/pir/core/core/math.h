#pragma once

#include "primitives.h"

/**
 * Basic Abstraction Layer - Math Functions
 *
 * Provides common mathematical operations and utilities.
 * Uses a static class with template methods for type safety.
 */
class Math
{
public:
    /**
     * Returns the maximum of two values.
     * @param a First value
     * @param b Second value
     * @return The larger of the two values
     */
    template <typename T, typename U>
    static FORCE_INLINE auto Max(const T& a, const U& b) noexcept
    {
        using CommonType = decltype(true ? a : b);
        const CommonType ca = static_cast<CommonType>(a);
        const CommonType cb = static_cast<CommonType>(b);
        return (ca > cb) ? ca : cb;
    }

    /**
     * Returns the minimum of two values.
     * @param a First value
     * @param b Second value
     * @return The smaller of the two values
     */
    template <typename T, typename U>
    static FORCE_INLINE auto Min(const T& a, const U& b) noexcept
    {
        using CommonType = decltype(true ? a : b);
        const CommonType ca = static_cast<CommonType>(a);
        const CommonType cb = static_cast<CommonType>(b);
        return (ca < cb) ? ca : cb;
    }

    /**
     * Returns the absolute value of a number.
     * @param x Value to get absolute value of
     * @return The absolute value of x
     */
    template <typename T>
    static FORCE_INLINE T Abs(T x) noexcept
    {
        return (x < 0) ? -x : x;
    }

    /**
     * Clamps a value between a minimum and maximum.
     * @param x Value to clamp
     * @param minVal Minimum allowed value
     * @param maxVal Maximum allowed value
     * @return x clamped to [minVal, maxVal]
     */
    template <typename T>
    static FORCE_INLINE T Clamp(T x, T minVal, T maxVal) noexcept
    {
        return Min(Max(x, minVal), maxVal);
    }
};