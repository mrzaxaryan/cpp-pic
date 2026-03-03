#include "tls_buffer.h"
#include "memory.h"
#include "logger.h"

/// @brief Append data to the TLS buffer
/// @param data Pointer to the data to append
/// @param size Size of the data to append
/// @return The offset at which the data was appended

INT32 TlsBuffer::Append(Span<const CHAR> data)
{
    auto r = CheckSize((INT32)data.Size());
    if (!r)
        return -1;
    Memory::Copy(buffer + size, data.Data(), data.Size());
    size += (INT32)data.Size();
    return size - (INT32)data.Size();
}

/// @brief Append a single character to the TLS buffer
/// @param data Character to append
/// @return The offset at which the character was appended


/// @brief Append a 32-bit integer to the TLS buffer
/// @param size The 32-bit integer to append
/// @return The offset at which the 32-bit integer was appended

INT32 TlsBuffer::AppendSize(INT32 count)
{
    auto r = CheckSize(count);
    if (!r)
        return -1;
    size += count;
    return size - count;
}

/// @brief Set the size of the TLS buffer
/// @param size The new size of the buffer
/// @return void

Result<void, Error> TlsBuffer::SetSize(INT32 newSize)
{
    size = 0;
    auto r = CheckSize(newSize);
    if (!r)
        return Result<void, Error>::Err(r.Error());
    size = newSize;
    return Result<void, Error>::Ok();
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

Result<void, Error> TlsBuffer::CheckSize(INT32 appendSize)
{
    if (size + appendSize <= capacity)
    {
        LOG_DEBUG("Buffer size is sufficient: %d + %d <= %d", size, appendSize, capacity);
        return Result<void, Error>::Ok();
    }

    PCHAR oldBuffer = buffer;
    INT32 newLen = (size + appendSize) * 4;
    if (newLen < 256)
    {
        newLen = 256;
    }

    PCHAR newBuffer = (PCHAR) new CHAR[newLen];
    if (!newBuffer)
    {
        return Result<void, Error>::Err(Error::TlsBuffer_AllocationFailed);
    }
    if (size > 0)
    {
        LOG_DEBUG("Resizing buffer from %d to %d bytes", capacity, newLen);
        Memory::Copy(newBuffer, oldBuffer, size);
    }
    if (oldBuffer && ownsMemory)
    {
        delete[] oldBuffer;
        oldBuffer = nullptr;
    }
    buffer = newBuffer;
    capacity = newLen;
    ownsMemory = true;
    return Result<void, Error>::Ok();
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
