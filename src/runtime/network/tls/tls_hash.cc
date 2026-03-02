#include "runtime/network/tls/tls_hash.h"
#include "platform/io/logger.h"

/// @brief Reset the transcript hash by destroying and reconstructing the SHA-256 context

VOID TlsHash::Reset()
{
	ctx.~SHA256();
	new (&ctx) SHA256();
}

/// @brief Update the running transcript hash with new handshake data
/// @param buffer The data to append

VOID TlsHash::Append(Span<const CHAR> buffer)
{
	ctx.Update(Span<const UINT8>((const UINT8 *)buffer.Data(), buffer.Size()));
}

/// @brief Snapshot the running SHA-256 context and finalize the copy to produce the transcript hash
/// @param out Output span for the hash digest (must be SHA256_DIGEST_SIZE bytes)

VOID TlsHash::GetHash(Span<CHAR> out)
{
	LOG_DEBUG("Computing incremental SHA256 transcript hash");
	SHA256 snapshot;
	snapshot.CopyStateFrom(ctx);
	snapshot.Final(Span<UINT8, SHA256_DIGEST_SIZE>((UINT8 *)out.Data()));
}
