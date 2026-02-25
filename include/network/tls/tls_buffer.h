#pragma once

#include "core.h"

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

    TlsBuffer(const TlsBuffer &) = delete;
    TlsBuffer &operator=(const TlsBuffer &) = delete;

    TlsBuffer(TlsBuffer &&other)
        : buffer(other.buffer), capacity(other.capacity), size(other.size), readPos(other.readPos), ownsMemory(other.ownsMemory)
    {
        other.buffer = nullptr;
        other.capacity = 0;
        other.size = 0;
        other.readPos = 0;
        other.ownsMemory = false;
    }
    TlsBuffer &operator=(TlsBuffer &&other)
    {
        if (this != &other)
        {
            if (ownsMemory) Clear();
            buffer = other.buffer;
            capacity = other.capacity;
            size = other.size;
            readPos = other.readPos;
            ownsMemory = other.ownsMemory;
            other.buffer = nullptr;
            other.capacity = 0;
            other.size = 0;
            other.readPos = 0;
            other.ownsMemory = false;
        }
        return *this;
    }

    // Write operations
    INT32 Append(PCVOID data, INT32 size);
    INT32 Append(CHAR data);
    INT32 Append(INT16 data);
    INT32 AppendSize(INT32 size);
    // Setting operation
    VOID SetSize(INT32 size);
    // Clean up for buffers
    VOID Clear();
    // Ensure there is enough capacity to append data
    VOID CheckSize(INT32 appendSize);

    // Read operations
    template <typename T>
    T Read();
    VOID Read(PCHAR buf, INT32 size);
    UINT32 ReadU24BE();

    // Accessors
    INT32 GetSize() const { return size; }
    PCHAR GetBuffer() const { return buffer; }
    VOID SetBuffer(PCHAR buf) { buffer = buf; if (!ownsMemory) size = 0; }
    INT32 GetReaded() const { return readPos; }
    VOID AppendReaded(INT32 sz) { readPos += sz; }
    VOID ResetReadPos() { readPos = 0; }
};
