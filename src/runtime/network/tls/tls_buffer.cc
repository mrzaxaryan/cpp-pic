#include "tls_buffer.h"
#include "memory.h"
#include "logger.h"

/// @brief Append data to the TLS buffer
/// @param data Pointer to the data to append
/// @param size Size of the data to append
/// @return The offset at which the data was appended

INT32 TlsBuffer::Append(Span<const CHAR> data)
{
    INT32 dataSize = (INT32)data.Size();
    CheckSize(dataSize);
    Memory::Copy(buffer + this->size, (PVOID)data.Data(), dataSize);
    this->size += dataSize;
    return this->size - dataSize;
}

/// @brief Append a single character to the TLS buffer
/// @param data Character to append
/// @return The offset at which the character was appended


/// @brief Append a 32-bit integer to the TLS buffer
/// @param size The 32-bit integer to append
/// @return The offset at which the 32-bit integer was appended

INT32 TlsBuffer::AppendSize(INT32 size)
{
    CheckSize(size);
    this->size += size;
    return this->size - size;
}

/// @brief Set the size of the TLS buffer
/// @param size The new size of the buffer
/// @return void

VOID TlsBuffer::SetSize(INT32 size)
{
    this->size = 0;
    CheckSize(size);
    this->size = size;
}

/// @brief Clean up the TLS buffer by freeing memory if owned and resetting size and capacity
/// @return void

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

/// @brief Ensure there is enough capacity in the TLS buffer to append additional data
/// @param appendSize The size of the data to be appended
/// @return void

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

    PCHAR newBuffer = (PCHAR) new CHAR[newLen];
    if (!newBuffer)
    {
        // Allocation failed: restore invariant so the buffer stays valid (empty)
        this->size = 0;
        this->capacity = 0;
        if (oldBuffer && this->ownsMemory)
            delete[] oldBuffer;
        this->buffer = nullptr;
        this->ownsMemory = true;
        return;
    }
    if (this->size > 0)
    {
        LOG_DEBUG("Resizing buffer from %d to %d bytes\n", this->capacity, newLen);
        Memory::Copy(newBuffer, oldBuffer, this->size);
    }
    if (oldBuffer && this->ownsMemory)
    {
        delete[] oldBuffer;
        oldBuffer = nullptr;
    }
    buffer = newBuffer;
    this->capacity = newLen;
    this->ownsMemory = true;
}

/// @brief Read a value of type T from the TLS buffer
/// @tparam T The type of the value to read
/// @return The value read from the buffer

template <typename T>
T TlsBuffer::Read()
{
    T value = *(T *)(this->buffer + this->readPos);
    this->readPos += sizeof(T);
    return value;
}

/// @brief Read 2-byte value from the TLS buffer
/// @return The 2-byte value read from the buffer

template <>
INT16 TlsBuffer::Read<INT16>()
{
    INT16 value;
    Memory::Copy(&value, this->buffer + this->readPos, sizeof(INT16));
    this->readPos += sizeof(INT16);
    return value;
}

/// @brief Read a single byte from the TLS buffer
/// @return The byte read from the buffer

template <>
INT8 TlsBuffer::Read<INT8>()
{
    INT8 value;
    Memory::Copy(&value, this->buffer + this->readPos, sizeof(INT8));
    this->readPos += sizeof(INT8);
    return value;
}

/// @brief Read a block of data from the TLS buffer
/// @param buf The buffer to store the read data
/// @param size The number of bytes to read
/// @return void

VOID TlsBuffer::Read(Span<CHAR> buf)
{
    Memory::Copy(buf.Data(), this->buffer + this->readPos, buf.Size());
    this->readPos += (INT32)buf.Size();
}

/// @brief Read a 24-bit big-endian unsigned integer from the TLS buffer
/// @return The 24-bit value read from the buffer

UINT32 TlsBuffer::ReadU24BE()
{
    UINT8 b0 = (UINT8)this->buffer[this->readPos];
    UINT8 b1 = (UINT8)this->buffer[this->readPos + 1];
    UINT8 b2 = (UINT8)this->buffer[this->readPos + 2];
    this->readPos += 3;
    return ((UINT32)b0 << 16) | ((UINT32)b1 << 8) | (UINT32)b2;
}
