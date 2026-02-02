#pragma once

#include "bal.h"
#include "tls_buffer.h"
#include "ecc.h"
#include "tls_hash.h"
#include "chacha20_encoder.h"

#define ECC_COUNT 2
#define RAND_SIZE 32
#define MAX_HASH_LEN 64
#define MAX_PUBKEY_SIZE 2048
#define MAX_KEY_SIZE 32
#define MAX_IV_SIZE 12
#define CIPHER_KEY_SIZE 32
#define CIPHER_HASH_SIZE 32
#define CONTENT_APPLICATION_DATA 0x17

// https://tools.ietf.org/html/rfc4492#section-5.1.1
// https://tools.ietf.org/html/rfc8422#section-5.1.1
// https://tools.ietf.org/html/rfc7919

typedef enum ECC_GROUP
{
    ECC_NONE = 0,           // No ECC Support. It is also used to imply RSA
    ECC_secp256r1 = 0x0017, // Supported Group: secp256r1(0x0017)
    ECC_secp384r1 = 0x0018, // Supported Group: secp384r1 (0x0018)
} ECC_GROUP;


// TLS cipher structure
class TlsCipher
{
private:
    INT32 cipherCount;              // Number of supported ciphers
    INT32 clientSeqNum;             // Client sequence number
    INT32 serverSeqNum;             // Server sequence number
    Ecc *privateEccKeys[ECC_COUNT]; // Private ECC keys
    TlsBuffer publicKey;            // Public key buffer
    TlsBuffer decodeBuffer;         // Buffer for decoded data
    TlsHash handshakeHash;          // Hash for handshake

    union
    {
        struct
        {
            UINT8 mainSecret[MAX_HASH_LEN];      // Main secret
            UINT8 handshakeSecret[MAX_HASH_LEN]; // Handshake secret
            UINT8 pseudoRandomKey[MAX_HASH_LEN]; // Pseudo-random key
        } data13;
        struct
        {
            UINT8 clientRandom[RAND_SIZE]; // Client random value
            UINT8 serverRandom[RAND_SIZE]; // Server random value
            UINT8 masterKey[48];           // Master key
        } data12;
    };
    INT32 cipherIndex;               // Current cipher index
    ChaCha20Encoder chacha20Context; // ChaCha20 encoder context
    BOOL isEncoding;                 // Encoding status

public:
    TlsCipher();
    VOID Reset();
    VOID Destroy();
    PINT8 CreateClientRand();
    BOOL UpdateServerInfo();
    VOID GetHash(const CHAR *out);
    VOID UpdateHash(const CHAR *in, UINT32 len);
    BOOL ComputePublicKey(INT32 eccIndex, TlsBuffer *out);
    BOOL ComputePreKey(ECC_GROUP ecc, const CHAR *serverKey, INT32 serverKeyLen, TlsBuffer *premasterKey);
    BOOL ComputeKey(ECC_GROUP ecc, const CHAR *serverKey, INT32 serverKeyLen, PCHAR finishedHash);
    VOID ComputeVerify(TlsBuffer *out, INT32 verifySize, INT32 localOrRemote);
    VOID Encode(TlsBuffer *sendbuf, const CHAR *packet, INT32 packetSize, BOOL keepOriginal);
    BOOL Decode(TlsBuffer *inout, INT32 version);
    VOID SetEncoding(BOOL encoding);
    VOID ResetSequenceNumber();
    BOOL GetEncoding() { return isEncoding; };
    INT32 GetCipherCount() { return cipherCount; };
    TlsBuffer *GetPubKey() { return &publicKey; };
    VOID SetCipherCount(INT32 count) { cipherCount = count; };
};
