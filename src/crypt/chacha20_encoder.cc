#include "chacha20_encoder.h"
#include "logger.h"
#include "memory.h"

// ChaCha20Encoder Class Implementation

// Destructor
ChaCha20Encoder::~ChaCha20Encoder()
{
    // LOG_DEBUG("Freeing encoder: %p", cipher->encoder);
    Memory::Zero(this, sizeof(ChaCha20Encoder));
    this->initialized = FALSE;
}

// Constructor
ChaCha20Encoder::ChaCha20Encoder()
{
    this->ivLength = TLS_CHACHA20_IV_LENGTH;
    this->initialized = TRUE;
    Memory::Zero(this->localNonce, TLS_CHACHA20_IV_LENGTH);
    Memory::Zero(this->remoteNonce, TLS_CHACHA20_IV_LENGTH);
}

// Initialize the ChaCha20 encoder with keys and IVs
BOOL ChaCha20Encoder::Initialize(PUCHAR localKey, PUCHAR remoteKey, PUCHAR localIv, PUCHAR remoteIv, INT32 keyLength)
{
    UINT32 counter = 1;
    this->ivLength = TLS_CHACHA20_IV_LENGTH;
    LOG_DEBUG("Initializing Chacha20 encoder with key length: %d bits", keyLength * 8);
    this->localCipher.KeySetup(localKey, keyLength * 8);
    this->remoteCipher.KeySetup(remoteKey, keyLength * 8);
    LOG_DEBUG("Chacha20 encoder initialized with local key: %p, remote key: %p", localKey, remoteKey);
    this->localCipher.IVSetup96BitNonce(localIv, (PUCHAR)&counter);
    Memory::Copy(this->localNonce, localIv, this->ivLength);
    this->remoteCipher.IVSetup96BitNonce(remoteIv, (PUCHAR)&counter);
    Memory::Copy(this->remoteNonce, remoteIv, this->ivLength);

    return TRUE;
}

// Encode data using ChaCha20 and Poly1305
VOID ChaCha20Encoder::Encode(TlsBuffer &out, const CHAR *packet, INT32 packetSize, const UCHAR *aad, INT32 aadSize)
{
    const UCHAR *sequence = aad + 5;

    aadSize = 5;
    INT32 counter = 1;
    LOG_DEBUG("Encoding packet with Chacha20 encoder, packet size: %d bytes", packetSize);
    out.AppendSize(packetSize + POLY1305_TAGLEN);
    UCHAR poly1305_key[POLY1305_KEYLEN];
    this->localCipher.IvUpdate(this->localNonce, sequence, (UINT8 *)&counter);
    LOG_DEBUG("Chacha20 encoder updated IV with sequence: %p, counter: %d", sequence, counter);
    this->localCipher.Poly1305Key(poly1305_key);
    LOG_DEBUG("Chacha20 encoder computed Poly1305 key: %p", poly1305_key);
    this->localCipher.Poly1305Aead((UINT8 *)packet, packetSize, (UINT8 *)aad, aadSize, poly1305_key, (UINT8 *)out.GetBuffer() + out.GetSize() - POLY1305_TAGLEN - packetSize);
}

// Decode data using ChaCha20 and Poly1305
BOOL ChaCha20Encoder::Decode(TlsBuffer &in, TlsBuffer &out, const UCHAR *aad, INT32 aadSize)
{
    out.CheckSize(in.GetSize());

    const UCHAR *sequence = aad + 5;

    aadSize = 5;
    LOG_DEBUG("Decoding packet with Chacha20 encoder, input size: %d bytes", in.GetSize());
    UINT32 counter = 1;

    // Update IV with sequence number and counter
    this->remoteCipher.IvUpdate(this->remoteNonce, (UINT8 *)sequence, (PUCHAR)&counter);
    LOG_DEBUG("Chacha20 encoder updated IV with sequence: %p, counter: %d", sequence, counter);

    // Generate Poly1305 key from current cipher state
    UCHAR poly1305_key[POLY1305_KEYLEN];
    this->remoteCipher.Poly1305Key(poly1305_key);
    LOG_DEBUG("Chacha20 encoder computed Poly1305 key: %p", poly1305_key);

    // Decode and verify (poly_key is already computed, don't regenerate inside Poly1305Decode)
    INT32 size = this->remoteCipher.Poly1305Decode((UINT8 *)in.GetBuffer(), in.GetSize(), (UINT8 *)aad, aadSize, poly1305_key, (UINT8 *)out.GetBuffer());
    LOG_DEBUG("Chacha20 decode returned size: %d", size);
    if (size <= 0)
    {
        LOG_ERROR("Chacha20 Decode failed, size: %d", size);
        return FALSE;
    }
    LOG_DEBUG("Chacha20 Decode succeeded, output size: %d bytes", size);
    out.SetSize(size);
    return TRUE;
}

// Compute size for encoding or decoding
INT32 ChaCha20Encoder::ComputeSize(INT32 size, INT32 encodeOrDecode)
{
    return encodeOrDecode == 1 ? size - POLY1305_TAGLEN : size + POLY1305_TAGLEN;
}