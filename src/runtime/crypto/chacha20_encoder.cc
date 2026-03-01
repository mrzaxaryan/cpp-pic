#include "chacha20_encoder.h"
#include "logger.h"
#include "memory.h"

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
Result<void, Error> ChaCha20Encoder::Initialize(Span<const UINT8> localKey, Span<const UINT8> remoteKey, const UCHAR (&localIv)[TLS_CHACHA20_IV_LENGTH], const UCHAR (&remoteIv)[TLS_CHACHA20_IV_LENGTH])
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
    INT32 packetSize = (INT32)packet.Size();
    const UCHAR *sequence = aad.Data() + 5;

    INT32 aadSize = 5;
    INT32 counter = 1;
    LOG_DEBUG("Encoding packet with Chacha20 encoder, packet size: %d bytes", packetSize);
    out.AppendSize(packetSize + POLY1305_TAGLEN);
    UCHAR poly1305_key[POLY1305_KEYLEN];
    this->localCipher.IvUpdate(this->localNonce, Span<const UINT8>(sequence, 8), (UINT8 *)&counter);
    LOG_DEBUG("Chacha20 encoder updated IV with sequence: %p, counter: %d", sequence, counter);
    this->localCipher.Poly1305Key(poly1305_key);
    LOG_DEBUG("Chacha20 encoder computed Poly1305 key: %p", poly1305_key);
    (void)this->localCipher.Poly1305Aead(
        Span<UCHAR>((UINT8 *)packet.Data(), packetSize),
        Span<const UCHAR>((UINT8 *)aad.Data(), aadSize),
        poly1305_key,
        Span<UCHAR>((UINT8 *)out.GetBuffer() + out.GetSize() - POLY1305_TAGLEN - packetSize, packetSize + POLY1305_TAGLEN));
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
    this->remoteCipher.IvUpdate(this->remoteNonce, Span<const UINT8>(sequence, 8), (PUCHAR)&counter);
    LOG_DEBUG("Chacha20 encoder updated IV with sequence: %p, counter: %d", sequence, counter);

    // Generate Poly1305 key from current cipher state
    UCHAR poly1305_key[POLY1305_KEYLEN];
    this->remoteCipher.Poly1305Key(poly1305_key);
    LOG_DEBUG("Chacha20 encoder computed Poly1305 key: %p", poly1305_key);

    // Decode and verify (poly_key is already computed, don't regenerate inside Poly1305Decode)
    auto decodeResult = this->remoteCipher.Poly1305Decode(
        Span<UCHAR>((UINT8 *)in.GetBuffer(), in.GetSize()),
        Span<const UCHAR>((UINT8 *)aad.Data(), aadSize),
        poly1305_key,
        Span<UCHAR>((UINT8 *)out.GetBuffer(), in.GetSize()));
    LOG_DEBUG("Chacha20 decode returned");
    if (!decodeResult)
    {
        LOG_ERROR("Chacha20 Decode failed");
        return Result<void, Error>::Err(Error::ChaCha20_DecodeFailed);
    }
    INT32 size = decodeResult.Value();
    LOG_DEBUG("Chacha20 Decode succeeded, output size: %d bytes", size);
    out.SetSize(size);
    return Result<void, Error>::Ok();
}

// Compute size for encoding or decoding
INT32 ChaCha20Encoder::ComputeSize(INT32 size, INT32 encodeOrDecode)
{
    return encodeOrDecode == 1 ? size - POLY1305_TAGLEN : size + POLY1305_TAGLEN;
}