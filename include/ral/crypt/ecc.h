#pragma once

#include "bal.h"

/* Convenience typedef */

/* Maximum number of 64-bit words for curve parameters (secp384r1 has the longest) */
#define MAX_NUM_ECC_DIGITS (384 / 64)

typedef struct
{
    UINT64 low;   // Lower 64 bits
    UINT64 high;  // Upper 64 bits
} UINT128_;

/* Elliptic Curve Point structure */
typedef struct
{
    UINT64 x[MAX_NUM_ECC_DIGITS];
    UINT64 y[MAX_NUM_ECC_DIGITS];
} EccPoint;

class Ecc
{
private:
    UINT32 eccBytes;                        // Number of bytes for ECC
    UINT32 numEccDigits;                     // Number of ECC digits
    UINT64 curveP[MAX_NUM_ECC_DIGITS];       // Curve parameter p
    UINT64 curveB[MAX_NUM_ECC_DIGITS];       // Curve parameter b
    EccPoint curveG;                         // Curve generator point G
    UINT64 curveN[MAX_NUM_ECC_DIGITS];       // Curve order n
    UINT64 privateKey[MAX_NUM_ECC_DIGITS];   // ECC private key
    EccPoint publicKey;                      // ECC public key

    // Low-level big integer and ECC operations
    VOID VliClear(UINT64 *pVli);
    INT32 VliIsZero(UINT64 *pVli);
    UINT64 VliTestBit(UINT64 *pVli, UINT32 p_bit);
    UINT32 VliNumDigits(UINT64 *pVli);
    UINT32 VliNumBits(UINT64 *pVli);
    VOID VliSet(UINT64 *pDest, UINT64 *pSrc);
    INT32 VliCmp(UINT64 *pLeft, UINT64 *pRight);
    UINT64 VliLShift(UINT64 *pResult, UINT64 *pIn, UINT32 shift);
    VOID VliRShift1(UINT64 *pVli);
    UINT64 VliAdd(UINT64 *pResult, UINT64 *pLeft, UINT64 *pRight);
    UINT64 VliSub(UINT64 *pResult, UINT64 *pLeft, UINT64 *pRight);
    UINT128_ Mul64_64(UINT64 left, UINT64 right);
    UINT128_ Add128_128(UINT128_ a, UINT128_ b);
    VOID VliMult(UINT64 *pResult, UINT64 *pLeft, UINT64 *pRight);
    VOID VliSquare(UINT64 *pResult, UINT64 *pLeft);
    VOID VliModAdd(UINT64 *pResult, UINT64 *pLeft, UINT64 *pRight, UINT64 *pMod);
    VOID VliModSub(UINT64 *pResult, UINT64 *pLeft, UINT64 *pRight, UINT64 *pMod);
    VOID VliMmodFast128(UINT64 *pResult, UINT64 *pProduct);
    VOID VliMmodFast192(UINT64 *pResult, UINT64 *pProduct);
    VOID VliMmodFast256(UINT64 *pResult, UINT64 *pProduct);
    VOID OmegaMult384(UINT64 *pResult, UINT64 *pProduct);
    VOID VliMmodFast384(UINT64 *pResult, UINT64 *pProduct);
    VOID VliModMultFast(UINT64 *pResult, UINT64 *pLeft, UINT64 *pRight);
    VOID VliModSquareFast(UINT64 *pResult, UINT64 *pLeft);
    VOID VliModInv(UINT64 *pResult, UINT64 *pInput, UINT64 *pMod);
    INT32 IsZero(EccPoint *point);
    VOID DoubleJacobian(UINT64 *X1, UINT64 *Y1, UINT64 *Z1);
    VOID ApplyZ(UINT64 *X1, UINT64 *Y1, UINT64 *Z);
    VOID XYcZInitialDouble(UINT64 *X1, UINT64 *Y1, UINT64 *X2, UINT64 *Y2, UINT64 *initialZ);
    VOID XYcZAdd(UINT64 *X1, UINT64 *Y1, UINT64 *X2, UINT64 *Y2);
    VOID XYcZAddC(UINT64 *X1, UINT64 *Y1, UINT64 *X2, UINT64 *Y2);
    VOID Mult(EccPoint *pResult, EccPoint *pPoint, UINT64 *pScalar, UINT64 *pInitialZ);
    VOID Bytes2Native(UINT64 *pNative, const UINT8 *pBytes);
    VOID Native2Bytes(UINT8 *pBytes, const UINT64 *pNative);
    VOID ModSqrt(UINT64 *pA);
    VOID PointDecompress(EccPoint *pPoint, const UINT8 *pCompressed);

public:
    Ecc(); // Constructor
    INT32 Initialize(INT32 bytes); // Initialize ECC with specified byte size
    INT32 ExportPublicKey(UINT8 *publicKey, UINT32 publicKeySize); // Export ECC public key
    INT32 ComputeSharedSecret(const UINT8 *publicKey, UINT32 publicKeySize, UINT8 *secret); // Compute shared secret
};