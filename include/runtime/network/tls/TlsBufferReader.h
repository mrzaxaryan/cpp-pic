#include "primitives.h"
#pragma once

// Definitions for buffer reader
class TlsBufferReader
{
private:
    PCHAR buffer;
    INT32 size;
    INT32 bytesRead;

public:
    TlsBufferReader();
    TlsBufferReader(PCHAR buffer, INT32 size);
    template <typename T>
    T Read();
    VOID Read(PCHAR buf, INT32 size);
    INT32 GetSize() { return size; }
    VOID SetSize(INT32 size) { this->size = size; }
    PCHAR GetBuffer() { return buffer; }
    VOID SetBuffer(PCHAR buffer) { this->buffer = buffer; }
    INT32 GetReaded() { return bytesRead; }
    VOID AppendReaded(INT32 size) { bytesRead += size; }
};