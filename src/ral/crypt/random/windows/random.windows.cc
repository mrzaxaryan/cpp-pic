#include "random.h"
#include "DateTime.h"
#include "pal.h"

// Function to get a random number in the range of 0 to RANDOM_MAX
INT32 Random::Get()
{
    // Check if the seed is zero, if so, set it to a default value
    this->seed = (this->seed * 214013L + 2531011L) & 0x7fffffff;
    // Generate a new random number using a linear congruential generator formula
    return (INT32)(this->seed % Random::MAX);
}

// Constructor to initialize the random number generator
Random::Random()
{
    // Initialize the random seed using the current time
    auto dateTime = DateTime::Now();
    // Combine seconds, microseconds, and milliseconds to create a seed
    this->seed = dateTime.Second << 8 | dateTime.Microseconds << 16 | dateTime.Milliseconds << 32;
}
