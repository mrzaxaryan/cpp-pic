/**
 * @file chacha20_encoder.h
 * @brief TLS 1.3 ChaCha20-Poly1305 Record Layer Encoder
 *
 * @details Provides bidirectional encryption/decryption for TLS 1.3 record layer
 * using ChaCha20-Poly1305 AEAD cipher. Manages separate cipher states and nonces
 * for local (outgoing) and remote (incoming) traffic.
 *
 * Key features:
 * - Separate cipher contexts for send and receive directions
 * - Per-record nonce derivation per RFC 8446 (TLS 1.3)
 * - Automatic nonce increment after each record
 * - AEAD with additional authenticated data (AAD) support
 *
 * @par TLS 1.3 Nonce Construction:
 * The per-record nonce is constructed by XORing the IV with the 64-bit
 * record sequence number, left-padded with zeros to 12 bytes.
 *
 * @see chacha20.h for underlying cipher implementation
 *
 * @ingroup crypt
 *
 * @defgroup chacha20_encoder ChaCha20 TLS Encoder
 * @ingroup crypt
 * @{
 */

#pragma once

#include "core.h"
#include "chacha20.h"
#include "tls_buffer.h"

typedef struct ChaCha20Encoder ChaCha20Encoder;

/**
 * @struct ChaCha20Encoder
 * @brief Bidirectional TLS 1.3 record encryption/decryption
 *
 * @details Manages ChaCha20-Poly1305 cipher states for both directions of a
 * TLS connection. Each direction has its own key and IV, derived from the
 * TLS key schedule.
 *
 * @par Usage:
 * @code
 * ChaCha20Encoder encoder;
 * encoder.Initialize(clientKey, serverKey, clientIv, serverIv, 32);
 *
 * // Encrypt outgoing record
 * TlsBuffer encrypted;
 * encoder.Encode(&encrypted, plaintext, ptLen, aad, aadLen);
 *
 * // Decrypt incoming record
 * TlsBuffer decrypted;
 * if (encoder.Decode(&incoming, &decrypted, aad, aadLen)) {
 *     // Decryption and authentication successful
 * }
 * @endcode
 */
struct ChaCha20Encoder
{
private:
    ChaChaPoly1305 remoteCipher;               /**< @brief Cipher for decrypting remote data */
    ChaChaPoly1305 localCipher;                /**< @brief Cipher for encrypting local data */
    INT32 ivLength;                            /**< @brief IV length in bytes (12 for TLS 1.3) */
    UCHAR remoteNonce[TLS_CHACHA20_IV_LENGTH]; /**< @brief Base IV for remote (server) direction */
    UCHAR localNonce[TLS_CHACHA20_IV_LENGTH];  /**< @brief Base IV for local (client) direction */
    BOOL initialized;                          /**< @brief TRUE if encoder is initialized */

public:
    /**
     * @brief Default constructor
     * @details Initializes encoder in uninitialized state.
     */
    ChaCha20Encoder();

    /**
     * @brief Destructor - securely clears key material
     */
    ~ChaCha20Encoder();

    /**
     * @brief Initializes encoder with TLS-derived keys and IVs
     * @param localKey Key for encrypting outgoing data (client_write_key)
     * @param remoteKey Key for decrypting incoming data (server_write_key)
     * @param localIv IV for outgoing data (client_write_iv)
     * @param remoteIv IV for incoming data (server_write_iv)
     * @param keyLength Key length in bytes (must be 32 for ChaCha20)
     * @return TRUE on success, FALSE on failure
     *
     * @details Keys and IVs are derived from the TLS 1.3 key schedule.
     * For client: local = client_write, remote = server_write
     * For server: local = server_write, remote = client_write
     */
    BOOL Initialize(PUCHAR localKey, PUCHAR remoteKey, PUCHAR localIv, PUCHAR remoteIv, INT32 keyLength);

    /**
     * @brief Encrypts and authenticates a TLS record
     * @param out Output buffer for encrypted record (ciphertext + tag)
     * @param packet Plaintext data to encrypt
     * @param packetSize Length of plaintext in bytes
     * @param aad Additional authenticated data (TLS record header)
     * @param aadSize Length of AAD in bytes
     *
     * @details Encrypts packet with ChaCha20, computes Poly1305 tag over AAD
     * and ciphertext, and appends the 16-byte tag to output.
     * Automatically increments the local sequence number.
     */
    VOID Encode(TlsBuffer &out, const CHAR *packet, INT32 packetSize, const UCHAR *aad, INT32 aadSize);

    /**
     * @brief Decrypts and verifies a TLS record
     * @param in Input buffer containing ciphertext + tag
     * @param out Output buffer for decrypted plaintext
     * @param aad Additional authenticated data (TLS record header)
     * @param aadSize Length of AAD in bytes
     * @return TRUE if decryption and authentication succeed, FALSE otherwise
     *
     * @details Verifies Poly1305 tag over AAD and ciphertext, then decrypts
     * if authentication succeeds. Automatically increments the remote sequence number.
     *
     * @warning Returns FALSE if authentication fails - output buffer contents
     * are undefined and MUST NOT be used.
     */
    BOOL Decode(TlsBuffer &in, TlsBuffer &out, const UCHAR *aad, INT32 aadSize);

    /**
     * @brief Computes output size for encoding or decoding
     * @param size Input size in bytes
     * @param encodeOrDecode 0 for encoding (adds tag), 1 for decoding (removes tag)
     * @return Output size in bytes
     *
     * @details For encoding: output = input + 16 (Poly1305 tag)
     * For decoding: output = input - 16 (remove tag)
     */
    static INT32 ComputeSize(INT32 size, INT32 encodeOrDecode);

    /**
     * @brief Returns the IV length
     * @return IV length in bytes (12 for TLS 1.3)
     */
    INT32 GetIvLength() { return ivLength; }

    /**
     * @brief Checks if encoder is initialized
     * @return TRUE if initialized, FALSE otherwise
     */
    BOOL IsInitialized() { return initialized; }
};

/** @} */ // end of chacha20_encoder group