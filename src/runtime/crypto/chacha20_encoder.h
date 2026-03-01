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
    BOOL initialized;                          /**< @brief true if encoder is initialized */

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

    ChaCha20Encoder(const ChaCha20Encoder &) = delete;
    ChaCha20Encoder &operator=(const ChaCha20Encoder &) = delete;

    // Stack-only
    VOID *operator new(USIZE) = delete;
    VOID operator delete(VOID *) = delete;

    ChaCha20Encoder(ChaCha20Encoder &&other) noexcept
        : remoteCipher(static_cast<ChaChaPoly1305 &&>(other.remoteCipher)), localCipher(static_cast<ChaChaPoly1305 &&>(other.localCipher)), ivLength(other.ivLength), initialized(other.initialized)
    {
        Memory::Copy(remoteNonce, other.remoteNonce, TLS_CHACHA20_IV_LENGTH);
        Memory::Copy(localNonce, other.localNonce, TLS_CHACHA20_IV_LENGTH);
        other.ivLength = 0;
        other.initialized = false;
        Memory::Zero(other.remoteNonce, TLS_CHACHA20_IV_LENGTH);
        Memory::Zero(other.localNonce, TLS_CHACHA20_IV_LENGTH);
    }

    ChaCha20Encoder &operator=(ChaCha20Encoder &&other) noexcept
    {
        if (this != &other)
        {
            remoteCipher = static_cast<ChaChaPoly1305 &&>(other.remoteCipher);
            localCipher = static_cast<ChaChaPoly1305 &&>(other.localCipher);
            ivLength = other.ivLength;
            initialized = other.initialized;
            Memory::Copy(remoteNonce, other.remoteNonce, TLS_CHACHA20_IV_LENGTH);
            Memory::Copy(localNonce, other.localNonce, TLS_CHACHA20_IV_LENGTH);
            other.ivLength = 0;
            other.initialized = false;
            Memory::Zero(other.remoteNonce, TLS_CHACHA20_IV_LENGTH);
            Memory::Zero(other.localNonce, TLS_CHACHA20_IV_LENGTH);
        }
        return *this;
    }

    /**
     * @brief Initializes encoder with TLS-derived keys and IVs
     * @param localKey Key for encrypting outgoing data (client_write_key)
     * @param remoteKey Key for decrypting incoming data (server_write_key)
     * @param localIv IV for outgoing data (client_write_iv)
     * @param remoteIv IV for incoming data (server_write_iv)
     * @return Result<void, Error>::Ok() on success, Result<void, Error>::Err() on failure
     *
     * @details Keys and IVs are derived from the TLS 1.3 key schedule.
     * For client: local = client_write, remote = server_write
     * For server: local = server_write, remote = client_write
     */
    [[nodiscard]] Result<void, Error> Initialize(Span<const UINT8> localKey, Span<const UINT8> remoteKey, const UCHAR (&localIv)[TLS_CHACHA20_IV_LENGTH], const UCHAR (&remoteIv)[TLS_CHACHA20_IV_LENGTH]);

    /**
     * @brief Encrypts and authenticates a TLS record
     * @param out Output buffer for encrypted record (ciphertext + tag)
     * @param packet Span of plaintext data to encrypt
     * @param aad Span of additional authenticated data (TLS record header)
     *
     * @details Encrypts packet with ChaCha20, computes Poly1305 tag over AAD
     * and ciphertext, and appends the 16-byte tag to output.
     * Automatically increments the local sequence number.
     */
    VOID Encode(TlsBuffer &out, Span<const CHAR> packet, Span<const UCHAR> aad);

    /**
     * @brief Decrypts and verifies a TLS record
     * @param in Input buffer containing ciphertext + tag
     * @param out Output buffer for decrypted plaintext
     * @param aad Span of additional authenticated data (TLS record header)
     * @return Result<void, Error>::Ok() if decryption and authentication succeed, Error::ChaCha20_DecodeFailed otherwise
     *
     * @details Verifies Poly1305 tag over AAD and ciphertext, then decrypts
     * if authentication succeeds. Automatically increments the remote sequence number.
     *
     * @warning Returns Err if authentication fails - output buffer contents
     * are undefined and MUST NOT be used.
     */
    [[nodiscard]] Result<void, Error> Decode(TlsBuffer &in, TlsBuffer &out, Span<const UCHAR> aad);

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
     * @brief Checks if encoder is initialized and valid
     * @return true if initialized, false otherwise
     */
    BOOL IsValid() const { return initialized; }
};

/** @} */ // end of chacha20_encoder group