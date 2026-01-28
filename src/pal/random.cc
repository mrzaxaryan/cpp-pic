#include "random.h"
#include "date_time.h"
#include "pal.h"

// Function to get a random number in the range of 0 to RANDOM_MAX
INT32 Random::GetArray(USIZE size, PUINT8 buffer)
{
    // Fill the buffer with random bytes
    for (USIZE i = 0; i < size; ++i)
        buffer[i] = (UINT8)(Random::Get() % 0xFF); // Get random byte
    return 1;                                      // Indicate success
}