/**
 * @file ecc.h
 * @brief Elliptic Curve Cryptography (ECC) Implementation
 *
 * @details Position-independent implementation of Elliptic Curve Diffie-Hellman (ECDH)
 * key exchange for TLS 1.3. Supports NIST P-256, P-384 curves (secp256r1, secp384r1).
 *
 * Key features:
 * - Pure integer arithmetic (no floating point)
 * - PIC-safe: All curve parameters embedded in code
 * - Constant-time operations where security-critical
 * - Support for compressed and uncompressed point formats
 *
 * Supported curves:
 * - secp256r1 (P-256): 256-bit prime field, 32-byte keys
 * - secp384r1 (P-384): 384-bit prime field, 48-byte keys
 *
 * @par TLS 1.3 Key Exchange:
 * @code
 * Ecc ecdh;
 * ecdh.Initialize(32);  // P-256
 *
 * // Export public key for key_share extension
 * UINT8 pubKey[65];  // Uncompressed: 0x04 || x || y
 * ecdh.ExportPublicKey(pubKey, sizeof(pubKey));
 *
 * // Compute shared secret from peer's public key
 * UINT8 secret[32];
 * ecdh.ComputeSharedSecret(peerPubKey, peerPubKeyLen, secret);
 * @endcode
 *
 * @note Uses big integer arithmetic implemented in VLI (Variable Length Integer) operations.
 *
 * @ingroup crypt
 *
 * @defgroup ecc Elliptic Curve Cryptography
 * @ingroup crypt
 * @{
 */

#pragma once

#include "core.h"

/** @brief Maximum 64-bit words needed for largest supported curve (P-384 = 6 words) */
#define MAX_NUM_ECC_DIGITS (384 / 64)

/**
 * @struct UINT128_
 * @brief 128-bit unsigned integer for intermediate multiplication results
 *
 * @details Used internally for 64x64->128 bit multiplication results
 * before reduction. Stored as low/high 64-bit halves.
 */
typedef struct
{
    UINT64 low;  /**< @brief Lower 64 bits */
    UINT64 high; /**< @brief Upper 64 bits */
} UINT128_;

/**
 * @struct EccPoint
 * @brief Elliptic curve point in affine coordinates
 *
 * @details Represents a point (x, y) on the elliptic curve.
 * Coordinates are stored as arrays of 64-bit words in little-endian order.
 * The point at infinity is represented by x = y = 0.
 */
typedef struct
{
    UINT64 x[MAX_NUM_ECC_DIGITS]; /**< @brief X coordinate */
    UINT64 y[MAX_NUM_ECC_DIGITS]; /**< @brief Y coordinate */
} EccPoint;

/**
 * @class Ecc
 * @brief Elliptic Curve Diffie-Hellman (ECDH) Key Exchange
 *
 * @details Implements ECDH key exchange for TLS 1.3 using NIST prime curves.
 * Generates ephemeral key pairs and computes shared secrets for key derivation.
 *
 * The implementation uses:
 * - Jacobian coordinates for point multiplication (faster than affine)
 * - Montgomery ladder for constant-time scalar multiplication
 * - Curve-specific fast reduction for modular arithmetic
 *
 * @par Supported Curves:
 * - secp256r1 (P-256): Initialize with bytes=32
 * - secp384r1 (P-384): Initialize with bytes=48
 *
 * @par Security Considerations:
 * - Private keys are generated randomly and stored internally
 * - Point validation is performed on imported public keys
 * - Shared secret computation is constant-time
 */
class Ecc
{
private:
    UINT32 eccBytes;                       /**< @brief Key size in bytes (32 or 48) */
    UINT32 numEccDigits;                   /**< @brief Number of 64-bit words per coordinate */
    UINT64 curveP[MAX_NUM_ECC_DIGITS];     /**< @brief Prime field modulus p */
    UINT64 curveB[MAX_NUM_ECC_DIGITS];     /**< @brief Curve coefficient b (y^2 = x^3 - 3x + b) */
    EccPoint curveG;                       /**< @brief Base point (generator) G */
    UINT64 curveN[MAX_NUM_ECC_DIGITS];     /**< @brief Order of base point n */
    UINT64 privateKey[MAX_NUM_ECC_DIGITS]; /**< @brief Private key d (random scalar) */
    EccPoint publicKey;                    /**< @brief Public key Q = d * G */

    // =========================================================================
    // Variable Length Integer (VLI) Operations
    // =========================================================================

    /** @brief Sets VLI to zero */
    VOID VliClear(UINT64 *pVli);

    /** @brief Tests if VLI is zero */
    INT32 VliIsZero(UINT64 *pVli);

    /** @brief Tests specific bit of VLI */
    UINT64 VliTestBit(UINT64 *pVli, UINT32 p_bit);

    /** @brief Returns number of significant digits */
    UINT32 VliNumDigits(UINT64 *pVli);

    /** @brief Returns number of significant bits */
    UINT32 VliNumBits(UINT64 *pVli);

    /** @brief Copies VLI: pDest = pSrc */
    VOID VliSet(UINT64 *pDest, UINT64 *pSrc);

    /** @brief Compares two VLIs: returns -1, 0, or 1 */
    INT32 VliCmp(UINT64 *pLeft, UINT64 *pRight);

    /** @brief Left shift: pResult = pIn << shift */
    UINT64 VliLShift(UINT64 *pResult, UINT64 *pIn, UINT32 shift);

    /** @brief Right shift by 1: pVli >>= 1 */
    VOID VliRShift1(UINT64 *pVli);

    /** @brief Addition: pResult = pLeft + pRight, returns carry */
    UINT64 VliAdd(UINT64 *pResult, UINT64 *pLeft, UINT64 *pRight);

    /** @brief Subtraction: pResult = pLeft - pRight, returns borrow */
    UINT64 VliSub(UINT64 *pResult, UINT64 *pLeft, UINT64 *pRight);

    /** @brief 64x64 -> 128 bit multiplication */
    UINT128_ Mul64_64(UINT64 left, UINT64 right);

    /** @brief 128-bit addition */
    UINT128_ Add128_128(UINT128_ a, UINT128_ b);

    /** @brief Full multiplication: pResult = pLeft * pRight */
    VOID VliMult(UINT64 *pResult, UINT64 *pLeft, UINT64 *pRight);

    /** @brief Squaring: pResult = pLeft^2 */
    VOID VliSquare(UINT64 *pResult, UINT64 *pLeft);

    // =========================================================================
    // Modular Arithmetic
    // =========================================================================

    /** @brief Modular addition: pResult = (pLeft + pRight) mod pMod */
    VOID VliModAdd(UINT64 *pResult, UINT64 *pLeft, UINT64 *pRight, UINT64 *pMod);

    /** @brief Modular subtraction: pResult = (pLeft - pRight) mod pMod */
    VOID VliModSub(UINT64 *pResult, UINT64 *pLeft, UINT64 *pRight, UINT64 *pMod);

    /** @brief Fast reduction mod p for 256-bit curves (P-256) */
    VOID VliMmodFast256(UINT64 *pResult, UINT64 *pProduct);

    /** @brief Helper for P-384 reduction */
    VOID OmegaMult384(UINT64 *pResult, UINT64 *pProduct);

    /** @brief Fast reduction mod p for 384-bit curves (P-384) */
    VOID VliMmodFast384(UINT64 *pResult, UINT64 *pProduct);

    /** @brief Dispatches to curve-specific fast reduction */
    VOID MmodFast(UINT64 *pResult, UINT64 *pProduct);

    /** @brief Fast modular multiplication using curve-specific reduction */
    VOID VliModMultFast(UINT64 *pResult, UINT64 *pLeft, UINT64 *pRight);

    /** @brief Fast modular squaring using curve-specific reduction */
    VOID VliModSquareFast(UINT64 *pResult, UINT64 *pLeft);

    /** @brief Modular inverse: pResult = pInput^(-1) mod pMod */
    VOID VliModInv(UINT64 *pResult, UINT64 *pInput, UINT64 *pMod);

    // =========================================================================
    // Elliptic Curve Point Operations
    // =========================================================================

    /** @brief Tests if point is at infinity */
    INT32 IsZero(EccPoint &point);

    /** @brief Point doubling in Jacobian coordinates */
    VOID DoubleJacobian(UINT64 *X1, UINT64 *Y1, UINT64 *Z1);

    /** @brief Converts from Jacobian to affine coordinates */
    VOID ApplyZ(UINT64 *X1, UINT64 *Y1, UINT64 *Z);

    /** @brief Initial doubling for co-Z arithmetic */
    VOID XYcZInitialDouble(UINT64 *X1, UINT64 *Y1, UINT64 *X2, UINT64 *Y2, UINT64 *initialZ);

    /** @brief Co-Z point addition */
    VOID XYcZAdd(UINT64 *X1, UINT64 *Y1, UINT64 *X2, UINT64 *Y2);

    /** @brief Co-Z conjugate addition */
    VOID XYcZAddC(UINT64 *X1, UINT64 *Y1, UINT64 *X2, UINT64 *Y2);

    /** @brief Scalar multiplication: result = pScalar * point */
    VOID Mult(EccPoint &result, EccPoint &point, UINT64 *pScalar, UINT64 *pInitialZ);

    // =========================================================================
    // Serialization
    // =========================================================================

    /** @brief Converts big-endian bytes to native VLI format */
    VOID Bytes2Native(UINT64 *pNative, const UINT8 *pBytes);

    /** @brief Converts native VLI format to big-endian bytes */
    VOID Native2Bytes(UINT8 *pBytes, const UINT64 *pNative);

    /** @brief Computes modular square root for point decompression */
    VOID ModSqrt(UINT64 *pA);

    /** @brief Decompresses point from compressed format (02/03 || x) */
    VOID PointDecompress(EccPoint &point, const UINT8 *pCompressed);

public:
    /**
     * @brief Default constructor
     * @details Initializes internal state. Must call Initialize() before use.
     */
    Ecc();

    /**
     * @brief Destructor - securely clears private key material
     */
    ~Ecc();

    // Non-copyable -- prevent duplication of private key material
    Ecc(const Ecc &) = delete;
    Ecc &operator=(const Ecc &) = delete;

    // Non-movable -- Ecc objects are heap-allocated and managed through pointers
    Ecc(Ecc &&) = delete;
    Ecc &operator=(Ecc &&) = delete;

    /**
     * @brief Checks if the ECC instance is initialized with a valid curve
     * @return true if initialized, false otherwise
     */
    BOOL IsValid() const { return eccBytes != 0; }

    /**
     * @brief Initializes ECC with specified curve
     * @param bytes Key size in bytes: 32 for P-256, 48 for P-384
     * @return Ok on success, Err(Ecc_InitFailed) on error
     *
     * @details Loads curve parameters and generates ephemeral key pair.
     * The private key is generated randomly; public key is computed as Q = d * G.
     */
    [[nodiscard]] Result<void, Error> Initialize(INT32 bytes);

    /**
     * @brief Exports public key in uncompressed format
     * @param publicKey Output buffer for public key
     * @param publicKeySize Size of output buffer (must be >= 2*eccBytes + 1)
     * @return Ok(bytesWritten) on success, Err(Ecc_ExportKeyFailed) on error
     *
     * @details Exports public key as: 0x04 || x || y (uncompressed point format)
     * For P-256: 65 bytes (1 + 32 + 32)
     * For P-384: 97 bytes (1 + 48 + 48)
     */
    [[nodiscard]] Result<UINT32, Error> ExportPublicKey(UINT8 *publicKey, UINT32 publicKeySize);

    /**
     * @brief Computes ECDH shared secret
     * @param publicKey Peer's public key (uncompressed or compressed)
     * @param publicKeySize Size of peer's public key
     * @param secret Output buffer for shared secret (x-coordinate)
     * @return Ok(bytesWritten) on success, Err(Ecc_SharedSecretFailed) on error
     *
     * @details Computes shared secret as x-coordinate of d * Q where:
     * - d is this instance's private key
     * - Q is the peer's public key
     *
     * @warning The raw shared secret should be passed through a KDF before use.
     */
    [[nodiscard]] Result<UINT32, Error> ComputeSharedSecret(const UINT8 *publicKey, UINT32 publicKeySize, UINT8 *secret);
};

/** @} */ // end of ecc group