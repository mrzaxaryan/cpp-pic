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

VOID TlsHash::Append(Span<const CHAR> buffer)
{
    this->cache.Append(buffer);
}

/// @brief Get the hash value from the cache by computing SHA-256 (out.Size()==32) or SHA-384 (out.Size()==48)
/// @param out Output span; size determines which hash algorithm is used
/// @return void

VOID TlsHash::GetHash(Span<CHAR> out)
{
    if (out.Size() == SHA256_DIGEST_SIZE)
    {
        LOG_DEBUG("Computing SHA256 hash with size: %d bytes", (INT32)out.Size());
        SHA256 ctx;
        if (this->cache.GetSize() > 0)
        {
            LOG_DEBUG("SHA256 hash cache size: %d bytes", this->cache.GetSize());
            ctx.Update(Span<const UINT8>((UINT8 *)this->cache.GetBuffer(), this->cache.GetSize()));
        }
        ctx.Final(Span<UINT8, SHA256_DIGEST_SIZE>((UINT8 *)out.Data()));
    }
    else
    {
        LOG_DEBUG("Computing SHA384 hash with size: %d bytes", (INT32)out.Size());
        SHA384 ctx;
        if (this->cache.GetSize() > 0)
            ctx.Update(Span<const UINT8>((UINT8 *)this->cache.GetBuffer(), this->cache.GetSize()));
        ctx.Final(Span<UINT8, SHA384_DIGEST_SIZE>((UINT8 *)out.Data()));
    }
}
