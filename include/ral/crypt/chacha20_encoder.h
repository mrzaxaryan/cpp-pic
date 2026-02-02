#pragma once

#include "bal.h"
#include "chacha20.h"
#include "tls_buffer.h"

typedef struct ChaCha20Encoder ChaCha20Encoder;

// ChaCha20 encoder
struct ChaCha20Encoder
{
private:
    ChaChaPoly1305 remoteCipher;               // Cipher for remote (decrypt)
    ChaChaPoly1305 localCipher;                // Cipher for local (encrypt)
    INT32 ivLength;                            // IV length in bytes
    UCHAR remoteNonce[TLS_CHACHA20_IV_LENGTH]; // Nonce for remote
    UCHAR localNonce[TLS_CHACHA20_IV_LENGTH];  // Nonce for local
    BOOL initialized;                          // Initialization status

public:
    // Constructor and destructor
    ChaCha20Encoder();                          
    ~ChaCha20Encoder();
    // Initialize encoder with keys and IVs
    BOOL Initialize(PUCHAR localKey, PUCHAR remoteKey, PUCHAR localIv, PUCHAR remoteIv, INT32 keyLength);
    // Encode and decode data
    VOID Encode(TlsBuffer *out, const CHAR *packet, INT32 packetSize, const UCHAR *aad, INT32 aadSize);
    BOOL Decode(TlsBufferReader *in, TlsBuffer *out, const UCHAR *aad, INT32 aadSize);
    // Compute size for encoding or decoding
    static INT32 ComputeSize(INT32 size, INT32 encodeOrDecode);
    // Function to get IV length
    INT32 GetIvLength() { return ivLength; }
    // Function to check if initialized
    BOOL IsInitialized() { return initialized; }
};