#pragma once

#include "bal.h"

// Unified TLS buffer for both reading and writing
class TlsBuffer
{
private:
    PCHAR buffer;
    INT32 capacity;
    INT32 size;
    INT32 readPos;
    BOOL ownsMemory;

public:
    // Default constructor - owns memory, write mode
    TlsBuffer() : buffer(nullptr), capacity(0), size(0), readPos(0), ownsMemory(true) {}

    // Constructor for wrapping existing data - read mode (does not own memory)
    TlsBuffer(PCHAR buffer, INT32 size) : buffer(buffer), capacity(size), size(size), readPos(0), ownsMemory(false) {}

    ~TlsBuffer() { if (ownsMemory) Clear(); }

    // Write operations
    INT32 Append(PCVOID data, INT32 size);
    INT32 Append(CHAR data);
    INT32 Append(INT16 data);
    INT32 AppendSize(INT32 size);
    VOID SetSize(INT32 size);
    VOID Clear();
    VOID CheckSize(INT32 appendSize);

    // Read operations
    template <typename T>
    T Read();
    VOID Read(PCHAR buf, INT32 size);

    // Accessors
    INT32 GetSize() { return size; }
    PCHAR GetBuffer() { return buffer; }
    VOID SetBuffer(PCHAR buf) { buffer = buf; if (!ownsMemory) size = 0; }
    INT32 GetReaded() { return readPos; }
    VOID AppendReaded(INT32 sz) { readPos += sz; }
    VOID ResetReadPos() { readPos = 0; }
};
