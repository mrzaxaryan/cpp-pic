#pragma once

#include "primitives.h"

// Class to handle random number generation
class Random
{
private:
    // Internal state for the random number generator
    UINT32 seed;
    INT32 GetSeedFromTime();

public:
    VOID *operator new(USIZE) = delete;    // Disable dynamic allocation
    VOID operator delete(VOID *) = delete; // Disable dynamic deallocation
    // The maximum value for the random number generator
    static constexpr INT32 MAX = 32767;
    // Constructor
    Random();

    // Generate a random string of specified length
    template <typename TChar>
    UINT32 GetString(TChar *pString, UINT32 length);
    // Generate a random character
    template <typename TChar>
    TChar GetChar();

    // Core random function
    INT32 Get();
    // Random byte array
    INT32 GetArray(USIZE size, PUINT8 buffer);
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
UINT32 Random::GetString(TChar *pString, UINT32 length)
{
    UINT32 i = 0; // Loop counter for the string length
    for (i = 0; i < length; i++)
    {
        pString[i] = Random::GetChar<TChar>(); // Get a random wide character
    }
    pString[length] = (TChar)'\0'; // Null-terminate the wide string
    return length;                 // Return the length of the wide string
}