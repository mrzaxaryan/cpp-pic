/**
 * @file chacha20.h
 * @brief ChaCha20-Poly1305 AEAD Cipher Implementation
 *
 * @details Position-independent implementation of the ChaCha20 stream cipher
 * and Poly1305 message authentication code, as specified in RFC 7539.
 *
 * ChaCha20-Poly1305 is an Authenticated Encryption with Associated Data (AEAD)
 * algorithm that combines:
 * - ChaCha20: A high-speed stream cipher by D. J. Bernstein
 * - Poly1305: A fast one-time authenticator
 *
 * Key features:
 * - 256-bit key, 96-bit nonce (TLS 1.3 compatible)
 * - Constant-time operations to prevent timing attacks
 * - No lookup tables (cache-timing resistant)
 * - PIC-safe: No .rdata dependencies
 *
 * @note Original ChaCha20 implementation by D. J. Bernstein, public domain.
 *
 * @ingroup crypt
 *
 * @defgroup chacha20 ChaCha20-Poly1305
 * @ingroup crypt
 * @{
 */

#pragma once

#include "core.h"

/** @brief ChaCha20 block size in bytes (512 bits) */
#define CHACHA_BLOCKLEN 64

/** @brief TLS 1.3 ChaCha20 IV/nonce length in bytes (96 bits) */
#define TLS_CHACHA20_IV_LENGTH 12

/** @brief Poly1305 key length in bytes (256 bits) */
#define POLY1305_KEYLEN 32

/** @brief Poly1305 authentication tag length in bytes (128 bits) */
#define POLY1305_TAGLEN 16

/** @brief Poly1305 block size in bytes (128 bits) */
#define POLY1305_BLOCK_SIZE 16

/**
 * @class Poly1305
 * @brief Poly1305 Message Authentication Code
 *
 * @details Implements the Poly1305 one-time authenticator as described in RFC 7539.
 * Poly1305 computes a 128-bit authentication tag using a 256-bit one-time key.
 *
 * The key consists of two parts:
 * - r (128 bits): "clamped" secret multiplier
 * - s (128 bits): secret addend
 *
 * @warning The key MUST be used only once. Reusing a key with different messages
 * completely breaks security.
 *
 * @par Example Usage:
 * @code
 * UCHAR key[32];  // One-time key (r || s)
 * UCHAR tag[16];
 *
 * Poly1305 mac(key);
 * mac.Update(data, dataLen);
 * mac.Finish(tag);
 * @endcode
 */
class Poly1305
{
private:
    UINT32 m_r[5];                           /**< @brief Clamped r value (5 x 26-bit limbs) */
    UINT32 m_h[5];                           /**< @brief Accumulator h (5 x 26-bit limbs) */
    UINT32 m_pad[4];                         /**< @brief Pad value s (128 bits) */
    USIZE m_leftover;                        /**< @brief Bytes remaining in buffer */
    UCHAR m_buffer[POLY1305_BLOCK_SIZE];     /**< @brief Partial block buffer */
    UCHAR m_final;                           /**< @brief Final block flag */

    /**
     * @brief Processes complete 16-byte blocks
     * @param data Block data (size must be multiple of 16)
     */
    VOID ProcessBlocks(Span<const UCHAR> data);

public:
    /**
     * @brief Constructs Poly1305 context with one-time key
     * @param key 32-byte one-time key (r || s)
     *
     * @details The first 16 bytes are clamped to produce r.
     * The last 16 bytes are used as the pad s.
     */
    Poly1305(const UCHAR (&key)[32]);

    /**
     * @brief Destructor - securely clears sensitive state
     */
    ~Poly1305();

    // Non-copyable: one-time key must not be duplicated
    Poly1305(const Poly1305 &) = delete;
    Poly1305 &operator=(const Poly1305 &) = delete;

    /**
     * @brief Updates MAC computation with additional data
     * @param data Span of input data bytes
     */
    VOID Update(Span<const UCHAR> data);

    /**
     * @brief Finalizes MAC computation and outputs tag
     * @param mac Output buffer for 16-byte authentication tag
     */
    VOID Finish(Span<UCHAR, POLY1305_TAGLEN> mac);

    /**
     * @brief Converts 4 bytes to 32-bit word (little-endian)
     * @param p Pointer to 4 bytes
     * @return 32-bit word
     */
    static UINT32 U8TO32(const UCHAR *p);

    /**
     * @brief Converts 32-bit word to 4 bytes (little-endian)
     * @param p Output buffer for 4 bytes
     * @param v 32-bit word to convert
     */
    static VOID U32TO8(PUCHAR p, UINT32 v);

    /**
     * @brief Generates Poly1305 key from ChaCha20 keystream
     * @param key256 256-bit ChaCha20 key
     * @param nonce Span of nonce bytes (8 or 12 bytes)
     * @param poly_key Output buffer for 32-byte Poly1305 key
     * @param counter ChaCha20 block counter (usually 0)
     * @return Ok on success, Err on invalid nonce size
     *
     * @details Derives the Poly1305 key by encrypting zeros with ChaCha20
     * using block counter 0, as specified in RFC 7539.
     */
    [[nodiscard]] static Result<void, Error> GenerateKey(Span<const UCHAR, POLY1305_KEYLEN> key256, Span<const UCHAR> nonce, Span<UCHAR, POLY1305_KEYLEN> poly_key, UINT32 counter);
};

/**
 * @class ChaChaPoly1305
 * @brief ChaCha20-Poly1305 Authenticated Encryption
 *
 * @details Implements the ChaCha20-Poly1305 AEAD cipher as specified in RFC 7539.
 * Combines ChaCha20 stream cipher with Poly1305 MAC for authenticated encryption.
 *
 * The ChaCha20 state is a 4x4 matrix of 32-bit words:
 * @verbatim
 * [ constant  constant  constant  constant  ]   (16 bytes)
 * [ key       key       key       key       ]   (32 bytes)
 * [ counter   nonce     nonce     nonce     ]   (16 bytes)
 * @endverbatim
 *
 * @par TLS 1.3 Usage:
 * @code
 * ChaChaPoly1305 cipher;
 * cipher.KeySetup(key, 256);
 * cipher.IVSetup96BitNonce(nonce, counter);
 *
 * // Generate Poly1305 key from first block
 * UCHAR polyKey[32];
 * cipher.Poly1305Key(polyKey);
 *
 * // Encrypt and authenticate
 * cipher.Poly1305Aead(plaintext, ptLen, aad, aadLen, polyKey, ciphertext);
 * @endcode
 */
class ChaChaPoly1305
{
private:
    UINT32 input[16];              /**< @brief ChaCha20 state matrix (16 x 32-bit words) */
    UINT8 ks[CHACHA_BLOCKLEN];     /**< @brief Keystream buffer for partial block handling */
    UINT8 unused;                  /**< @brief Unused bytes remaining in keystream buffer */

public:
    /**
     * @brief Default constructor - initializes state
     */
    ChaChaPoly1305();

    /**
     * @brief Destructor - securely clears key material
     */
    ~ChaChaPoly1305();

    // Non-copyable: prevents accidental key material duplication
    ChaChaPoly1305(const ChaChaPoly1305 &) = delete;
    ChaChaPoly1305 &operator=(const ChaChaPoly1305 &) = delete;

    // Movable: transfers ownership and zeroes source
    ChaChaPoly1305(ChaChaPoly1305 &&other) noexcept;
    ChaChaPoly1305 &operator=(ChaChaPoly1305 &&other) noexcept;

    /**
     * @brief Sets up the encryption key
     * @param k Pointer to key bytes
     * @param kbits Key size in bits (128 or 256)
     *
     * @details Initializes the ChaCha20 state with the provided key.
     * For 128-bit keys, the key is duplicated to fill 256 bits.
     */
    VOID KeySetup(Span<const UINT8> key);

    /**
     * @brief Extracts current key from state
     * @param k Output array for 32-byte key
     */
    VOID Key(UINT8 (&k)[32]);

    /**
     * @brief Extracts current nonce from state
     * @param nonce Output array for 12-byte nonce
     */
    VOID Nonce(UINT8 (&nonce)[TLS_CHACHA20_IV_LENGTH]);

    /**
     * @brief Sets up IV/nonce (64-bit nonce variant)
     * @param iv 8-byte nonce
     * @param counter 8-byte counter (little-endian)
     *
     * @details Original ChaCha20 IV setup with 64-bit nonce and 64-bit counter.
     */
    VOID IvSetup(const UINT8 *iv, const UINT8 *counter);

    /**
     * @brief Sets up IV/nonce for TLS 1.3 (96-bit nonce)
     * @param iv 12-byte nonce
     * @param counter 4-byte counter (little-endian)
     *
     * @details RFC 7539 compliant nonce setup with 96-bit nonce and 32-bit counter.
     */
    VOID IVSetup96BitNonce(const UINT8 *iv, const UINT8 *counter);

    /**
     * @brief Updates IV with XOR of nonce and sequence number
     * @param iv Original IV (12 bytes)
     * @param aad Additional data to XOR (sequence number bytes)
     * @param counter Block counter (nullable)
     *
     * @details Used in TLS 1.3 to derive per-record nonce.
     */
    VOID IvUpdate(Span<const UINT8, TLS_CHACHA20_IV_LENGTH> iv, Span<const UINT8> aad, const UINT8 *counter);

    /**
     * @brief Encrypts/decrypts data using ChaCha20
     * @param m Input data (plaintext or ciphertext)
     * @param c Output data (ciphertext or plaintext)
     * @param bytes Number of bytes to process
     *
     * @details XORs input with keystream. Same function for encryption and decryption.
     */
    VOID EncryptBytes(Span<const UINT8> m_span, Span<UINT8> c_span);

    /**
     * @brief Generates raw keystream block
     * @param output Output buffer for keystream (up to 64 bytes)
     */
    VOID Block(Span<UCHAR> output);

    /**
     * @brief Generates Poly1305 key from ChaCha20 block 0
     * @param poly1305_key Output buffer for 32-byte Poly1305 key
     *
     * @details Per RFC 7539, the Poly1305 key is derived by encrypting
     * a zero block with ChaCha20 using counter = 0.
     */
    VOID Poly1305Key(Span<UCHAR, POLY1305_KEYLEN> poly1305_key);

    /**
     * @brief Performs AEAD encryption with Poly1305 authentication
     * @param pt Plaintext to encrypt
     * @param len Plaintext length in bytes
     * @param aad Additional authenticated data
     * @param aad_len AAD length in bytes
     * @param poly_key 32-byte Poly1305 key
     * @param out Output: ciphertext followed by 16-byte tag
     * @return Ok on success
     *
     * @details Encrypts plaintext and computes authentication tag over AAD and ciphertext.
     */
    [[nodiscard]] Result<void, Error> Poly1305Aead(Span<UCHAR> pt, Span<const UCHAR> aad, const UCHAR (&poly_key)[POLY1305_KEYLEN], Span<UCHAR> out);

    /**
     * @brief Performs AEAD decryption with Poly1305 verification
     * @param pt Ciphertext with appended tag
     * @param len Ciphertext length (including 16-byte tag)
     * @param aad Additional authenticated data
     * @param aad_len AAD length in bytes
     * @param poly_key 32-byte Poly1305 key
     * @param out Output buffer for plaintext
     * @return 0 on success, non-zero if authentication fails
     *
     * @details Verifies authentication tag and decrypts if valid.
     * @warning Returns error if tag verification fails - do not use output in this case.
     */
    [[nodiscard]] Result<INT32, Error> Poly1305Decode(Span<UCHAR> pt, Span<const UCHAR> aad, const UCHAR (&poly_key)[POLY1305_KEYLEN], Span<UCHAR> out);
};

/** @} */ // end of chacha20 group
