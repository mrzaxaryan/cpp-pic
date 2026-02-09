#pragma once

#include "core.h"
#include "tls_buffer.h"

// Hash structure
class TlsHash
{
private:
    TlsBuffer cache;
public:
    // Reset the hash cache
    VOID Reset();
    // Append data to the hash cache
    VOID Append(const CHAR *buffer, INT32 size);
    // Get the hash value from the cache
    VOID GetHash(PCHAR out, INT32 hashSize);
};