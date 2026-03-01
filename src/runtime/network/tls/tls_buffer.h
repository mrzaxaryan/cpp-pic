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
    TlsBuffer(Span<CHAR> data) : buffer(data.Data()), capacity((INT32)data.Size()), size((INT32)data.Size()), readPos(0), ownsMemory(false) {}

    ~TlsBuffer()
    {
        if (ownsMemory)
            Clear();
    }

    TlsBuffer(const TlsBuffer &) = delete;
    TlsBuffer &operator=(const TlsBuffer &) = delete;

    TlsBuffer(TlsBuffer &&other) noexcept
        : buffer(other.buffer), capacity(other.capacity), size(other.size), readPos(other.readPos), ownsMemory(other.ownsMemory)
    {
        other.buffer = nullptr;
        other.capacity = 0;
        other.size = 0;
        other.readPos = 0;
        other.ownsMemory = false;
    }
    TlsBuffer &operator=(TlsBuffer &&other) noexcept
    {
        if (this != &other)
        {
            if (ownsMemory)
                Clear();
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
    INT32 Append(Span<const CHAR> data);

    template <typename T>
    INT32 Append(T data)
    {
        CheckSize(sizeof(T));
        *(T *)(buffer + this->size) = data;
        this->size += sizeof(T);
        return this->size - sizeof(T);
    }

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
    VOID Read(Span<CHAR> buf);
    UINT32 ReadU24BE();

    // Accessors
    INT32 GetSize() const { return size; }
    PCHAR GetBuffer() const { return buffer; }
    VOID SetBuffer(PCHAR buf)
    {
        buffer = buf;
        if (!ownsMemory)
            size = 0;
    }
    INT32 GetReadPosition() const { return readPos; }
    VOID AdvanceReadPosition(INT32 sz) { readPos += sz; }
    VOID ResetReadPos() { readPos = 0; }
};
