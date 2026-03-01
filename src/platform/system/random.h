#pragma once

#include "platform.h"

// Class to handle random number generation
class Random
{
private:
    // Internal state for the random number generator
    UINT64 seed;

public:
    VOID *operator new(USIZE) = delete;    // Disable dynamic allocation
    VOID operator delete(VOID *) = delete; // Disable dynamic deallocation

    // The maximum value for the random number generator
    static constexpr INT32 MAX = 0x7FFFFFFF;
    // Constructor — trivial; Get() auto-seeds on first call
    Random() : seed(0) {}

    // Generate a random string: fills str.Size()-1 positions with random chars,
    // writes null terminator at str.Size()-1, returns str.Size()-1 (number of random chars).
    template <typename TChar>
    UINT32 GetString(Span<TChar> str);
    // Generate a random character
    template <typename TChar>
    TChar GetChar();

    // Core random function
    INT32 Get();
    // Random byte array
    INT32 GetArray(Span<UINT8> buffer);
};

template <typename TChar>
TChar Random::GetChar()
{
    // Get a random value and map to 0-25 range for a-z
    // Avoid modulo by using multiplication and shift (fast range reduction)
    INT32 val = Random::Get();
    // Map [0, 32767] to [0, 25] using: (val * 26) / 32768
    // This is approximately val * 26 / 32768 = val * 26 >> 15
    INT32 charOffset = ((val & 0x7FFF) * 26) >> 15; // Result is 0-25
    // Clamp to ensure we're in range (safety check)
    if (charOffset > 25)
        charOffset = 25;
    TChar c = (TChar)('a' + charOffset);
    return c;
}

template <typename TChar>
UINT32 Random::GetString(Span<TChar> str)
{
    if (str.Size() == 0)
        return 0;
    UINT32 length = (UINT32)str.Size() - 1;
    for (UINT32 i = 0; i < length; i++)
    {
        str[i] = Random::GetChar<TChar>();
    }
    str[length] = (TChar)'\0';
    return length;
}
