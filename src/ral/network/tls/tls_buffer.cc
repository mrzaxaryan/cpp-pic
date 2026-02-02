#include "tls_buffer.h"
#include "memory.h"
#include "logger.h"

// Write operations
INT32 TlsBuffer::Append(PCVOID data, INT32 size)
{
    CheckSize(size);
    Memory::Copy(buffer + this->size, (PVOID)data, size);
    this->size += size;
    return this->size - size;
}

INT32 TlsBuffer::Append(CHAR data)
{
    CheckSize(1);
    buffer[this->size] = data;
    this->size++;
    return this->size - 1;
}

INT32 TlsBuffer::Append(INT16 data)
{
    CheckSize(2);
    *(INT16 *)(buffer + this->size) = data;
    this->size += 2;
    return this->size - 2;
}

INT32 TlsBuffer::AppendSize(INT32 size)
{
    CheckSize(size);
    this->size += size;
    return this->size - size;
}

VOID TlsBuffer::SetSize(INT32 size)
{
    this->size = 0;
    CheckSize(size);
    this->size = size;
}

VOID TlsBuffer::Clear()
{
    if (this->buffer && this->ownsMemory)
    {
        delete[] this->buffer;
        this->buffer = nullptr;
    }
    this->size = 0;
    this->capacity = 0;
    this->readPos = 0;
}

VOID TlsBuffer::CheckSize(INT32 appendSize)
{
    if (this->size + appendSize <= this->capacity)
    {
        LOG_DEBUG("Buffer size is sufficient: %d + %d <= %d\n", this->size, appendSize, this->capacity);
        return;
    }

    PCHAR oldBuffer = buffer;
    INT32 newLen = (this->size + appendSize) * 4;
    if (newLen < 256)
    {
        newLen = 256;
    }

    buffer = (PCHAR) new CHAR[newLen];
    if (this->size > 0)
    {
        LOG_DEBUG("Resizing buffer from %d to %d bytes\n", this->capacity, newLen);
        Memory::Copy(buffer, oldBuffer, this->size);
    }
    if (oldBuffer && this->ownsMemory)
    {
        delete[] oldBuffer;
        oldBuffer = nullptr;
    }
    this->capacity = newLen;
    this->ownsMemory = true;
}

// Read operations
template <typename T>
T TlsBuffer::Read()
{
    T value = *(T *)(this->buffer + this->readPos);
    this->readPos += sizeof(T);
    return value;
}

template <>
INT16 TlsBuffer::Read<INT16>()
{
    INT16 value;
    Memory::Copy(&value, this->buffer + this->readPos, sizeof(INT16));
    this->readPos += sizeof(INT16);
    return value;
}

template <>
INT8 TlsBuffer::Read<INT8>()
{
    INT8 value;
    Memory::Copy(&value, this->buffer + this->readPos, sizeof(INT8));
    this->readPos += sizeof(INT8);
    return value;
}

VOID TlsBuffer::Read(PCHAR buf, INT32 size)
{
    Memory::Copy(buf, this->buffer + this->readPos, size);
    this->readPos += size;
}
