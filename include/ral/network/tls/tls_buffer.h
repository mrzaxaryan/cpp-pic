#pragma once

#include "bal.h"

class TlsBuffer
{
private:
    PCHAR buffer;
    INT32 capacity;
    INT32 size;

public:
    TlsBuffer() : buffer(nullptr), capacity(0), size(0) {};
    ~TlsBuffer() { Clear(); }
    INT32 Append(const void *data, INT32 size);
    INT32 Append(CHAR data);
    INT32 Append(INT16 data);
    INT32 AppendSize(INT32 size);
    VOID SetSize(INT32 size);
    INT32 GetSize();
    PCHAR GetBuffer();
    VOID Clear();
    VOID CheckSize(INT32 appendSize);
};