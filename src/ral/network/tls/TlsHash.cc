#include "TlsHash.h"
#include "logger.h"
#include "sha2.h"


VOID TlsHash::Reset()
{
    this->cache.Clear();
}

VOID TlsHash::Append(const CHAR *buffer, INT32 size)
{
    this->cache.Append(buffer, size);
}

VOID TlsHash::GetHash(PCHAR out, INT32 hashSize)
{
    if (hashSize == 32)
    {
        LOG_DEBUG("Computing SHA256 hash with size: %d bytes", hashSize);
        SHA256 ctx;
        if (this->cache.GetSize() > 0)
        {
            LOG_DEBUG("SHA256 hash cache size: %d bytes", this->cache.GetSize());
            ctx.Update((UINT8 *)this->cache.GetBuffer(), this->cache.GetSize());
        }
        ctx.Final((UINT8 *)out);
    }
    else
    {
        LOG_DEBUG("Computing SHA384 hash with size: %d bytes", hashSize);
        SHA384 ctx;
        if (this->cache.GetSize() > 0)
            ctx.Update((UINT8 *)this->cache.GetBuffer(), this->cache.GetSize());
        ctx.Final((UINT8 *)out);
    }
}
