#include "tls_buffer.h"
#include "memory.h"
#include "logger.h"

/// @brief Append data to the TLS buffer
/// @param data Pointer to the data to append
/// @param size Size of the data to append
/// @return The offset at which the data was appended

INT32 TlsBuffer::Append(PCVOID data, INT32 size)
{
    CheckSize(size);
    Memory::Copy(buffer + this->size, (PVOID)data, size);
    this->size += size;
    return this->size - size;
}

/// @brief Append a single character to the TLS buffer
/// @param data Character to append
/// @return The offset at which the character was appended

INT32 TlsBuffer::Append(CHAR data)
{
    CheckSize(1);
    buffer[this->size] = data;
    this->size++;
    return this->size - 1;
}

/// @brief Append a 16-bit integer to the TLS buffer
/// @param data The 16-bit integer to append
/// @return The offset at which the 16-bit integer was appended

INT32 TlsBuffer::Append(INT16 data)
{
    CheckSize(2);
    *(INT16 *)(buffer + this->size) = data;
    this->size += 2;
    return this->size - 2;
}

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

VOID TlsBuffer::Read(PCHAR buf, INT32 size)
{
    Memory::Copy(buf, this->buffer + this->readPos, size);
    this->readPos += size;
}
