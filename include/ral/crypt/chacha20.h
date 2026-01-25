#pragma once

#include "primitives.h"
// ChaCha20 implementation by D. J. Bernstein
// Public domain.

// Constants for ChaCha20
#define CHACHA_BLOCKLEN 64
#define TLS_CHACHA20_IV_LENGTH 12

// Constants for Poly1305
#define POLY1305_KEYLEN 32
#define POLY1305_TAGLEN 16
#define POLY1305_BLOCK_SIZE 16

// Class to handle Poly1305 message authentication code
class Poly1305
{
private:
    // r - the "r" portion of the key, h - current hash value, pad - the pad portion of the key
    // leftover - number of bytes in buffer, buffer - data block being processed, final - indicates final block
    UINT32 m_r[5];
    UINT32 m_h[5];
    UINT32 m_pad[4];
    USIZE m_leftover;
    UCHAR m_buffer[POLY1305_BLOCK_SIZE];
    UCHAR m_final;

    VOID ProcessBlocks(const UCHAR *data, USIZE bytes); // Process data blocks for MAC computation

public:
    // Constructor and destructor
    Poly1305(const UCHAR key[32]);
    ~Poly1305();
    // Update the MAC with data and finalize to produce the MAC tag
    VOID Update(const UCHAR *data, USIZE bytes);
    VOID Finish(UCHAR mac[16]);

    // Helper functions for byte and word conversions 
    static UINT32 U8TO32(const UCHAR *p);
    static VOID U32TO8(PUCHAR p, UINT32 v);
    // Generate Poly1305 key from ChaCha20 key and nonce
    static INT32 GenerateKey(PUCHAR key256, PUCHAR nonce, UINT32 noncelen, PUCHAR poly_key, UINT32 counter);
};

// Class to handle ChaCha20
class ChaChaPoly1305
{
private:
    UINT32 input[16];          // ChaCha20 state
    UINT8 ks[CHACHA_BLOCKLEN]; // Key stream buffer
    UINT8 unused;              // Number of unused bytes in key stream buffer

public:
    // Destructor           
    ChaChaPoly1305();
    // ChaCha20 functions
    VOID KeySetup(const UINT8 *k, UINT32 kbits);
    VOID Key(UINT8 *k);
    VOID Nonce(UINT8 *nonce);
    VOID IvSetup(const UINT8 *iv, const UINT8 *counter);
    VOID IVSetup96BitNonce(const UINT8 *iv, const UINT8 *counter);
    VOID IvUpdate(const UINT8 *iv, const UINT8 *aad, const UINT8 *counter);
    VOID EncryptBytes(const UINT8 *m, UINT8 *c, UINT32 bytes);
    VOID Block(PUCHAR c, UINT32 len);
    // Poly1305 functions
    VOID Poly1305Key(PUCHAR poly1305_key);
    INT32 Poly1305Aead(PUCHAR pt, UINT32 len, PUCHAR aad, UINT32 aad_len, PUCHAR poly_key, PUCHAR out);
    INT32 Poly1305Decode(PUCHAR pt, UINT32 len, PUCHAR aad, UINT32 aad_len, PUCHAR poly_key, PUCHAR out);
};
