#include "tls_hash.h"
#include "logger.h"
#include "sha2.h"

/// @brief Reset the hash cache by clearing the underlying buffer
/// @return void

VOID TlsHash::Reset()
{
    this->cache.Clear();
}

/// @brief Append data to the hash cache by adding it to the underlying buffer
/// @param buffer The data to append
/// @param size The size of the data to append
/// @return void

VOID TlsHash::Append(const CHAR *buffer, INT32 size)
{
    this->cache.Append(buffer, size);
}

/// @brief Get the hash value from the cache by computing the appropriate hash (SHA-256 or SHA-384) based on the specified hash size
/// @param out The buffer to store the computed hash
/// @param hashSize The size of the hash to compute
/// @return void

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
