#pragma once

#include "bal.h"
#include "tls_buffer.h"

// Hash structure
class TlsHash
{
private:
    TlsBuffer cache;
public:
    VOID Reset();
    VOID Append(const CHAR *buffer, INT32 size);
    VOID GetHash(PCHAR out, INT32 hashSize);
};