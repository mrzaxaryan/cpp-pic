#include "TlsBufferReader.h"
#include "memory.h"
#include "logger.h"

TlsBufferReader::TlsBufferReader()
{
    this->buffer = nullptr;
    this->size = 0;
    this->bytesRead = 0;
}

// "Constructor": Initialize the tlsbuf_reader.
TlsBufferReader::TlsBufferReader(PCHAR buffer, INT32 size)
{
    this->buffer = buffer;
    this->size = size;
    this->bytesRead = 0;
}

template <typename T>
T TlsBufferReader::Read()
{
    T value = *(T *)(this->buffer + this->bytesRead);
    this->bytesRead += sizeof(T);
    return value;
}

// Read a short.
template <>
INT16 TlsBufferReader::Read<INT16>()
{
    // PMEMORY pMemory = GetMemory();
    INT16 value;
    Memory::Copy(&value, this->buffer + this->bytesRead, sizeof(INT16));
    this->bytesRead += sizeof(INT16);
    return value;
}

template <>
INT8 TlsBufferReader::Read<INT8>()
{
    INT8 value;
    Memory::Copy(&value, this->buffer + this->bytesRead, sizeof(INT8));
    this->bytesRead += sizeof(INT8);
    return value;
}

VOID TlsBufferReader::Read(PCHAR buf, INT32 size)
{
    Memory::Copy(buf, this->buffer + this->bytesRead, size);
    this->bytesRead += size;
}