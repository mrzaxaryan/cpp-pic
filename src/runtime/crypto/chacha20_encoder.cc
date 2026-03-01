#include "runtime/crypto/chacha20_encoder.h"
#include "platform/io/logger.h"
#include "core/memory/memory.h"

// ChaCha20Encoder Class Implementation

// Destructor
ChaCha20Encoder::~ChaCha20Encoder()
{
	Memory::Zero(this->localNonce, TLS_CHACHA20_IV_LENGTH);
	Memory::Zero(this->remoteNonce, TLS_CHACHA20_IV_LENGTH);
	this->initialized = false;
}

// Constructor
ChaCha20Encoder::ChaCha20Encoder()
{
	this->ivLength = TLS_CHACHA20_IV_LENGTH;
	this->initialized = false;
	Memory::Zero(this->localNonce, TLS_CHACHA20_IV_LENGTH);
	Memory::Zero(this->remoteNonce, TLS_CHACHA20_IV_LENGTH);
}

// Initialize the ChaCha20 encoder with keys and IVs
Result<void, Error> ChaCha20Encoder::Initialize(Span<const UINT8, POLY1305_KEYLEN> localKey, Span<const UINT8, POLY1305_KEYLEN> remoteKey, const UCHAR (&localIv)[TLS_CHACHA20_IV_LENGTH], const UCHAR (&remoteIv)[TLS_CHACHA20_IV_LENGTH])
{
	UINT32 counter = 1;
	this->ivLength = TLS_CHACHA20_IV_LENGTH;
	LOG_DEBUG("Initializing Chacha20 encoder with key length: %d bits", (INT32)localKey.Size() * 8);
	this->localCipher.KeySetup(localKey);
	this->remoteCipher.KeySetup(remoteKey);
	LOG_DEBUG("Chacha20 encoder initialized with local key: %p, remote key: %p", localKey.Data(), remoteKey.Data());
	this->localCipher.IVSetup96BitNonce(localIv, (PUCHAR)&counter);
	Memory::Copy(this->localNonce, localIv, sizeof(localIv));
	this->remoteCipher.IVSetup96BitNonce(remoteIv, (PUCHAR)&counter);
	Memory::Copy(this->remoteNonce, remoteIv, sizeof(remoteIv));

	this->initialized = true;
	return Result<void, Error>::Ok();
}

// Encode data using ChaCha20 and Poly1305
VOID ChaCha20Encoder::Encode(TlsBuffer &out, Span<const CHAR> packet, Span<const UCHAR> aad)
{
	const UCHAR *sequence = aad.Data() + 5;

	INT32 aadSize = 5;
	INT32 counter = 1;
	LOG_DEBUG("Encoding packet with Chacha20 encoder, packet size: %d bytes", (INT32)packet.Size());
	out.AppendSize((INT32)packet.Size() + POLY1305_TAGLEN);
	UCHAR polyKey[POLY1305_KEYLEN];
	this->localCipher.IVUpdate(this->localNonce, Span<const UINT8, 8>(sequence), (UINT8 *)&counter);
	LOG_DEBUG("Chacha20 encoder updated IV with sequence: %p, counter: %d", sequence, counter);
	this->localCipher.Poly1305Key(polyKey);
	LOG_DEBUG("Chacha20 encoder computed Poly1305 key: %p", polyKey);
	this->localCipher.Poly1305Aead(
		Span<UCHAR>((UINT8 *)packet.Data(), packet.Size()),
		Span<const UCHAR>((UINT8 *)aad.Data(), aadSize),
		polyKey,
		Span<UCHAR>((UINT8 *)out.GetBuffer() + out.GetSize() - POLY1305_TAGLEN - (INT32)packet.Size(), (INT32)packet.Size() + POLY1305_TAGLEN));
	Memory::Zero(polyKey, sizeof(polyKey));
}

// Decode data using ChaCha20 and Poly1305
Result<void, Error> ChaCha20Encoder::Decode(TlsBuffer &in, TlsBuffer &out, Span<const UCHAR> aad)
{
	out.CheckSize(in.GetSize());

	const UCHAR *sequence = aad.Data() + 5;

	INT32 aadSize = 5;
	LOG_DEBUG("Decoding packet with Chacha20 encoder, input size: %d bytes", in.GetSize());
	UINT32 counter = 1;

	// Update IV with sequence number and counter
	this->remoteCipher.IVUpdate(this->remoteNonce, Span<const UINT8, 8>(sequence), (PUCHAR)&counter);
	LOG_DEBUG("Chacha20 encoder updated IV with sequence: %p, counter: %d", sequence, counter);

	// Generate Poly1305 key from current cipher state
	UCHAR polyKey[POLY1305_KEYLEN];
	this->remoteCipher.Poly1305Key(polyKey);
	LOG_DEBUG("Chacha20 encoder computed Poly1305 key: %p", polyKey);

	// Decode and verify (polyKey is already computed, don't regenerate inside Poly1305Decode)
	auto decodeResult = this->remoteCipher.Poly1305Decode(
		Span<UCHAR>((UINT8 *)in.GetBuffer(), in.GetSize()),
		Span<const UCHAR>((UINT8 *)aad.Data(), aadSize),
		polyKey,
		Span<UCHAR>((UINT8 *)out.GetBuffer(), in.GetSize()));
	Memory::Zero(polyKey, sizeof(polyKey));
	LOG_DEBUG("Chacha20 decode returned");
	if (!decodeResult)
	{
		LOG_ERROR("Chacha20 Decode failed");
		return Result<void, Error>::Err(Error::ChaCha20_DecodeFailed);
	}
	auto& size = decodeResult.Value();
	LOG_DEBUG("Chacha20 Decode succeeded, output size: %d bytes", size);
	out.SetSize(size);
	return Result<void, Error>::Ok();
}

// Compute size for encoding or decoding
INT32 ChaCha20Encoder::ComputeSize(INT32 size, INT32 encodeOrDecode)
{
	return encodeOrDecode == 1 ? size - POLY1305_TAGLEN : size + POLY1305_TAGLEN;
}