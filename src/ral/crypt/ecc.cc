
#include "ecc.h"
#include "random.h"
#include "date_time.h"
#include "pal.h"
#include "memory.h"

/* Curve selection options. */
#define secp128r1 16
#define secp192r1 24
#define secp256r1 32
#define secp384r1 48
#define MAX_TRIES 16

#define EVEN(vli) (!(vli[0] & 1))

constexpr UINT64 Curve_P_16[] = {0xFFFFFFFFFFFFFFFF, 0xFFFFFFFDFFFFFFFF};
constexpr UINT64 Curve_B_16[] = {0xD824993C2CEE5ED3, 0xE87579C11079F43D};
constexpr EccPoint Curve_G_16 = {{0x0C28607CA52C5B86, 0x161FF7528B899B2D}, {0xC02DA292DDED7A83, 0xCF5AC8395BAFEB13}};
constexpr UINT64 Curve_N_16[] = {0x75A30D1B9038A115, 0xFFFFFFFE00000000};

constexpr UINT64 Curve_P_24[] = {0xFFFFFFFFFFFFFFFFull, 0xFFFFFFFFFFFFFFFEull, 0xFFFFFFFFFFFFFFFFull};
constexpr UINT64 Curve_B_24[] = {0xFEB8DEECC146B9B1ull, 0x0FA7E9AB72243049ull, 0x64210519E59C80E7ull};
constexpr EccPoint Curve_G_24 = {{0xF4FF0AFD82FF1012ull, 0x7CBF20EB43A18800ull, 0x188DA80EB03090F6ull}, {0x73F977A11E794811ull, 0x631011ED6B24CDD5ull, 0x07192B95FFC8DA78ull}};
constexpr UINT64 Curve_N_24[] = {0x146BC9B1B4D22831ull, 0xFFFFFFFF99DEF836ull, 0xFFFFFFFFFFFFFFFFull};

constexpr UINT64 Curve_P_32[] = {0xFFFFFFFFFFFFFFFF, 0x00000000FFFFFFFF, 0x0000000000000000, 0xFFFFFFFF00000001};
constexpr UINT64 Curve_B_32[] = {0x3BCE3C3E27D2604B, 0x651D06B0CC53B0F6, 0xB3EBBD55769886BC, 0x5AC635D8AA3A93E7};
constexpr EccPoint Curve_G_32 = {{0xF4A13945D898C296, 0x77037D812DEB33A0, 0xF8BCE6E563A440F2, 0x6B17D1F2E12C4247}, {0xCBB6406837BF51F5, 0x2BCE33576B315ECE, 0x8EE7EB4A7C0F9E16, 0x4FE342E2FE1A7F9B}};
constexpr UINT64 Curve_N_32[] = {0xF3B9CAC2FC632551, 0xBCE6FAADA7179E84, 0xFFFFFFFFFFFFFFFF, 0xFFFFFFFF00000000};

constexpr UINT64 Curve_P_48[] = {0x00000000FFFFFFFF, 0xFFFFFFFF00000000, 0xFFFFFFFFFFFFFFFE, 0xFFFFFFFFFFFFFFFF, 0xFFFFFFFFFFFFFFFF, 0xFFFFFFFFFFFFFFFF};
constexpr UINT64 Curve_B_48[] = {0x2A85C8EDD3EC2AEF, 0xC656398D8A2ED19D, 0x0314088F5013875A, 0x181D9C6EFE814112, 0x988E056BE3F82D19, 0xB3312FA7E23EE7E4};
constexpr EccPoint Curve_G_48 = {{0x3A545E3872760AB7, 0x5502F25DBF55296C, 0x59F741E082542A38, 0x6E1D3B628BA79B98, 0x8EB1C71EF320AD74, 0xAA87CA22BE8B0537},
                                 {0x7A431D7C90EA0E5F, 0x0A60B1CE1D7E819D, 0xE9DA3113B5F0B8C0, 0xF8F41DBD289A147C, 0x5D9E98BF9292DC29, 0x3617DE4A96262C6F}};
constexpr UINT64 Curve_N_48[] = {0xECEC196ACCC52973, 0x581A0DB248B0A77A, 0xC7634D81F4372DDF, 0xFFFFFFFFFFFFFFFF, 0xFFFFFFFFFFFFFFFF, 0xFFFFFFFFFFFFFFFF};

VOID Ecc::VliClear(UINT64 *pVli)
{
    UINT32 i;
    for (i = 0; i < this->numEccDigits; ++i)
    {
        pVli[i] = 0;
    }
}

/* Returns 1 if vli == 0, 0 otherwise. */
INT32 Ecc::VliIsZero(UINT64 *pVli)
{
    UINT32 i;
    for (i = 0; i < this->numEccDigits; ++i)
    {
        if (pVli[i])
        {
            return 0;
        }
    }
    return 1;
}

/* Returns nonzero if bit bit of vli is set. */
UINT64 Ecc::VliTestBit(UINT64 *pVli, UINT32 bit)
{
    return (pVli[bit >> 6] & ((UINT64)1 << (bit & 63)));
}

/* Counts the number of 64-bit "digits" in vli. */
UINT32 Ecc::VliNumDigits(UINT64 *pVli)
{
    INT32 i;
    /* Search from the end until we find a non-zero digit.
       We do it in reverse because we expect that most digits will be nonzero. */
    for (i = this->numEccDigits - 1; i >= 0 && pVli[i] == 0; --i)
    {
    }

    return (i + 1);
}

/* Counts the number of bits required for vli. */
UINT32 Ecc::VliNumBits(UINT64 *pVli)
{
    UINT32 i;
    UINT64 l_digit;

    UINT32 l_numDigits = this->VliNumDigits(pVli);
    if (l_numDigits == 0)
    {
        return 0;
    }

    l_digit = pVli[l_numDigits - 1];
    for (i = 0; l_digit; ++i)
    {
        l_digit >>= 1;
    }

    return ((l_numDigits - 1) * 64 + i);
}

/* Sets dest = src. */
VOID Ecc::VliSet(UINT64 *pDest, UINT64 *pSrc)
{
    UINT32 i;
    for (i = 0; i < this->numEccDigits; ++i)
    {
        pDest[i] = pSrc[i];
    }
}

/* Returns sign of left - right. */
INT32 Ecc::VliCmp(UINT64 *pLeft, UINT64 *pRight)
{
    INT32 i;
    for (i = this->numEccDigits - 1; i >= 0; --i)
    {
        if (pLeft[i] > pRight[i])
        {
            return 1;
        }
        else if (pLeft[i] < pRight[i])
        {
            return -1;
        }
    }
    return 0;
}

/* Computes result = in << c, returning carry. Can modify in place (if result == in). 0 < shift < 64. */
UINT64 Ecc::VliLShift(UINT64 *pResult, UINT64 *pIn, UINT32 shift)
{
    UINT64 l_carry = 0;
    UINT32 i;
    for (i = 0; i < this->numEccDigits; ++i)
    {
        UINT64 l_temp = pIn[i];
        pResult[i] = (l_temp << shift) | l_carry;
        l_carry = l_temp >> (64 - shift);
    }

    return l_carry;
}

/* Computes vli = vli >> 1. */
VOID Ecc::VliRShift1(UINT64 *pVli)
{
    UINT64 *l_end = pVli;
    UINT64 l_carry = 0;

    pVli += this->numEccDigits;
    while (pVli-- > l_end)
    {
        UINT64 l_temp = *pVli;
        *pVli = (l_temp >> 1) | l_carry;
        l_carry = l_temp << 63;
    }
}

/* Computes result = left + right, returning carry. Can modify in place. */
UINT64 Ecc::VliAdd(UINT64 *pResult, UINT64 *pLeft, UINT64 *pRight)
{
    UINT64 l_carry = 0;
    UINT32 i;
    for (i = 0; i < this->numEccDigits; ++i)
    {
        UINT64 l_sum = pLeft[i] + pRight[i] + l_carry;
        if (l_sum != pLeft[i])
        {
            l_carry = (l_sum < pLeft[i]);
        }
        pResult[i] = l_sum;
    }
    return l_carry;
}

/* Computes result = left - right, returning borrow. Can modify in place. */
UINT64 Ecc::VliSub(UINT64 *pResult, UINT64 *pLeft, UINT64 *pRight)
{
    UINT64 l_borrow = 0;
    UINT32 i;
    for (i = 0; i < this->numEccDigits; ++i)
    {
        UINT64 l_diff = pLeft[i] - pRight[i] - l_borrow;
        if (l_diff != pLeft[i])
        {
            l_borrow = (l_diff > pLeft[i]);
        }
        pResult[i] = l_diff;
    }
    return l_borrow;
}

UINT128_ Ecc::Mul64_64(UINT64 left, UINT64 right)
{
    UINT128_ result;

    UINT64 a0 = left & 0xffffffffull;
    UINT64 a1 = left >> 32;
    UINT64 b0 = right & 0xffffffffull;
    UINT64 b1 = right >> 32;

    UINT64 m0 = a0 * b0;
    UINT64 m1 = a0 * b1;
    UINT64 m2 = a1 * b0;
    UINT64 m3 = a1 * b1;

    m2 += (m0 >> 32);
    m2 += m1;
    if (m2 < m1)
    { // overflow
        m3 += UINT64(0x100000000ull);
    }

    result.low = (m0 & 0xffffffffull) | (m2 << 32);
    result.high = m3 + (m2 >> 32);

    return result;
}

UINT128_ Ecc::Add128_128(UINT128_ a, UINT128_ b)
{
    UINT128_ result;
    result.low = a.low + b.low;
    result.high = a.high + b.high + (result.low < a.low);

    return result;
}

VOID Ecc::VliMult(UINT64 *pResult, UINT64 *pLeft, UINT64 *pRight)
{
    UINT128_ r01 = {0, 0};
    UINT64 r2 = 0;

    UINT32 i, k;

    /* Compute each digit of result in sequence, maintaining the carries. */
    for (k = 0; k < this->numEccDigits * 2 - 1; ++k)
    {
        UINT32 l_min = (k < this->numEccDigits ? 0 : (k + 1) - this->numEccDigits);
        for (i = l_min; i <= k && i < this->numEccDigits; ++i)
        {
            UINT128_ l_product = this->Mul64_64(pLeft[i], pRight[k - i]);
            r01 = this->Add128_128(r01, l_product);
            r2 += (r01.high < l_product.high);
        }
        pResult[k] = r01.low;
        r01.low = r01.high;
        r01.high = r2;
        r2 = 0;
    }

    pResult[this->numEccDigits * 2 - 1] = r01.low;
}

VOID Ecc::VliSquare(UINT64 *pResult, UINT64 *pLeft)
{
    UINT128_ r01 = {0, 0};
    UINT64 r2 = 0;

    UINT32 i, k;
    for (k = 0; k < this->numEccDigits * 2 - 1; ++k)
    {
        UINT32 l_min = (k < this->numEccDigits ? 0 : (k + 1) - this->numEccDigits);
        for (i = l_min; i <= k && i <= k - i; ++i)
        {
            UINT128_ l_product = this->Mul64_64(pLeft[i], pLeft[k - i]);
            if (i < k - i)
            {
                r2 += l_product.high >> 63;
                l_product.high = (l_product.high << 1) | (l_product.low >> 63);
                l_product.low <<= 1;
            }
            r01 = this->Add128_128(r01, l_product);
            r2 += (r01.high < l_product.high);
        }
        pResult[k] = r01.low;
        r01.low = r01.high;
        r01.high = r2;
        r2 = 0;
    }

    pResult[this->numEccDigits * 2 - 1] = r01.low;
}

/* Computes result = (left + right) % mod.
   Assumes that left < mod and right < mod, result != mod. */
VOID Ecc::VliModAdd(UINT64 *pResult, UINT64 *pLeft, UINT64 *pRight, UINT64 *pMod)
{
    UINT64 l_carry = this->VliAdd(pResult, pLeft, pRight);
    if (l_carry || this->VliCmp(pResult, pMod) >= 0)
    { /* result > mod (result = mod + remainder), so subtract mod to get remainder. */
        this->VliSub(pResult, pResult, pMod);
    }
}

/* Computes result = (left - right) % mod.
   Assumes that left < mod and right < mod, result != mod. */
VOID Ecc::VliModSub(UINT64 *pResult, UINT64 *pLeft, UINT64 *pRight, UINT64 *pMod)
{
    UINT64 l_borrow = this->VliSub(pResult, pLeft, pRight);
    if (l_borrow)
    { /* In this case, result == -diff == (max int) - diff.
         Since -x % d == d - x, we can get the correct result from result + mod (with overflow). */
        this->VliAdd(pResult, pResult, pMod);
    }
}

/* Computes result = product % curve_p.
   See algorithm 5 and 6 from http://www.isys.uni-klu.ac.at/PDF/2001-0126-MT.pdf */
VOID Ecc::VliMmodFast128(UINT64 *pResult, UINT64 *pProduct)
{
    UINT64 l_tmp[MAX_NUM_ECC_DIGITS];
    INT64 l_carry;

    this->VliSet(pResult, pProduct);

    l_tmp[0] = pProduct[2];
    l_tmp[1] = (pProduct[3] & 0x1FFFFFFFFull) | (pProduct[2] << 33);
    l_carry = this->VliAdd(pResult, pResult, l_tmp);

    l_tmp[0] = (pProduct[2] >> 31) | (pProduct[3] << 33);
    l_tmp[1] = (pProduct[3] >> 31) | ((pProduct[2] & 0xFFFFFFFF80000000ull) << 2);
    l_carry += this->VliAdd(pResult, pResult, l_tmp);
    l_tmp[0] = (pProduct[2] >> 62) | (pProduct[3] << 2);
    l_tmp[1] = (pProduct[3] >> 62) | ((pProduct[2] & 0xC000000000000000ull) >> 29) | (pProduct[3] << 35);
    l_carry += this->VliAdd(pResult, pResult, l_tmp);

    l_tmp[0] = (pProduct[3] >> 29);
    l_tmp[1] = ((pProduct[3] & 0xFFFFFFFFE0000000ull) << 4);
    l_carry += this->VliAdd(pResult, pResult, l_tmp);
    l_tmp[0] = (pProduct[3] >> 60);
    l_tmp[1] = (pProduct[3] & 0xFFFFFFFE00000000ull);
    l_carry += this->VliAdd(pResult, pResult, l_tmp);

    l_tmp[0] = 0;
    l_tmp[1] = ((pProduct[3] & 0xF000000000000000ull) >> 27);
    l_carry += this->VliAdd(pResult, pResult, l_tmp);
    while (l_carry || this->VliCmp(this->curveP, pResult) != 1)
    {
        l_carry -= this->VliSub(pResult, pResult, this->curveP);
    }
}

/* Computes p_result = p_product % curveP.
   See algorithm 5 and 6 from http://www.isys.uni-klu.ac.at/PDF/2001-0126-MT.pdf */
VOID Ecc::VliMmodFast192(UINT64 *pResult, UINT64 *pProduct)
{
    UINT64 l_tmp[MAX_NUM_ECC_DIGITS];
    INT64 l_carry;

    this->VliSet(pResult, pProduct);

    this->VliSet(l_tmp, &pProduct[3]);
    l_carry = this->VliAdd(pResult, pResult, l_tmp);

    l_tmp[0] = 0;
    l_tmp[1] = pProduct[3];
    l_tmp[2] = pProduct[4];
    l_carry += this->VliAdd(pResult, pResult, l_tmp);

    l_tmp[0] = l_tmp[1] = pProduct[5];
    l_tmp[2] = 0;
    l_carry += this->VliAdd(pResult, pResult, l_tmp);
    while (l_carry || this->VliCmp(this->curveP, pResult) != 1)
    {
        l_carry -= this->VliSub(pResult, pResult, this->curveP);
    }
}

/* Computes result = product % curveP
   from http://www.nsa.gov/ia/_files/nist-routines.pdf */
VOID Ecc::VliMmodFast256(UINT64 *pResult, UINT64 *pProduct)
{
    UINT64 l_tmp[MAX_NUM_ECC_DIGITS];
    INT64 l_carry;

    /* t */
    this->VliSet(pResult, pProduct);

    /* s1 */
    l_tmp[0] = 0;
    l_tmp[1] = pProduct[5] & 0xffffffff00000000ull;
    l_tmp[2] = pProduct[6];
    l_tmp[3] = pProduct[7];
    l_carry = this->VliLShift(l_tmp, l_tmp, 1);
    l_carry += this->VliAdd(pResult, pResult, l_tmp);

    /* s2 */
    l_tmp[1] = pProduct[6] << 32;
    l_tmp[2] = (pProduct[6] >> 32) | (pProduct[7] << 32);
    l_tmp[3] = pProduct[7] >> 32;
    l_carry += this->VliLShift(l_tmp, l_tmp, 1);
    l_carry += this->VliAdd(pResult, pResult, l_tmp);

    /* s3 */
    l_tmp[0] = pProduct[4];
    l_tmp[1] = pProduct[5] & 0xffffffff;
    l_tmp[2] = 0;
    l_tmp[3] = pProduct[7];
    l_carry += this->VliAdd(pResult, pResult, l_tmp);
    /* s4 */
    l_tmp[0] = (pProduct[4] >> 32) | (pProduct[5] << 32);
    l_tmp[1] = (pProduct[5] >> 32) | (pProduct[6] & 0xffffffff00000000ull);
    l_tmp[2] = pProduct[7];
    l_tmp[3] = (pProduct[6] >> 32) | (pProduct[4] << 32);
    l_carry += this->VliAdd(pResult, pResult, l_tmp);

    /* d1 */
    l_tmp[0] = (pProduct[5] >> 32) | (pProduct[6] << 32);
    l_tmp[1] = (pProduct[6] >> 32);
    l_tmp[2] = 0;
    l_tmp[3] = (pProduct[4] & 0xffffffff) | (pProduct[5] << 32);
    l_carry -= this->VliSub(pResult, pResult, l_tmp);

    /* d2 */
    l_tmp[0] = pProduct[6];
    l_tmp[1] = pProduct[7];
    l_tmp[2] = 0;
    l_tmp[3] = (pProduct[4] >> 32) | (pProduct[5] & 0xffffffff00000000ull);
    l_carry -= this->VliSub(pResult, pResult, l_tmp);
    /* d3 */
    l_tmp[0] = (pProduct[6] >> 32) | (pProduct[7] << 32);
    l_tmp[1] = (pProduct[7] >> 32) | (pProduct[4] << 32);
    l_tmp[2] = (pProduct[4] >> 32) | (pProduct[5] << 32);
    l_tmp[3] = (pProduct[6] << 32);
    l_carry -= this->VliSub(pResult, pResult, l_tmp);
    /* d4 */
    l_tmp[0] = pProduct[7];
    l_tmp[1] = pProduct[4] & 0xffffffff00000000ull;
    l_tmp[2] = pProduct[5];
    l_tmp[3] = pProduct[6] & 0xffffffff00000000ull;
    l_carry -= this->VliSub(pResult, pResult, l_tmp);
    if (l_carry < 0)
    {
        do
        {
            l_carry += this->VliAdd(pResult, pResult, this->curveP);
        } while (l_carry < 0);
    }
    else
    {
        while (l_carry || this->VliCmp(this->curveP, pResult) != 1)
        {
            l_carry -= this->VliSub(pResult, pResult, this->curveP);
        }
    }
}

VOID Ecc::OmegaMult384(UINT64 *pResult, UINT64 *pRight)
{
    UINT64 l_tmp[MAX_NUM_ECC_DIGITS];
    UINT64 l_carry, l_diff;

    /* Multiply by (2^128 + 2^96 - 2^32 + 1). */
    this->VliSet(pResult, pRight); /* 1 */
    l_carry = this->VliLShift(l_tmp, pRight, 32);
    pResult[1 + this->numEccDigits] = l_carry + this->VliAdd(pResult + 1, pResult + 1, l_tmp); /* 2^96 + 1 */
    pResult[2 + this->numEccDigits] = this->VliAdd(pResult + 2, pResult + 2, pRight);          /* 2^128 + 2^96 + 1 */
    l_carry += this->VliSub(pResult, pResult, l_tmp);                                          /* 2^128 + 2^96 - 2^32 + 1 */
    l_diff = pResult[this->numEccDigits] - l_carry;
    if (l_diff > pResult[this->numEccDigits])
    { /* Propagate borrow if necessary. */
        UINT32 i;
        for (i = 1 + this->numEccDigits;; ++i)
        {
            --pResult[i];
            if (pResult[i] != (UINT64)-1)
            {
                break;
            }
        }
    }
    pResult[this->numEccDigits] = l_diff;
}

/* Computes p_result = p_product % curveP
    see PDF "Comparing Elliptic Curve Cryptography and RSA on 8-bit CPUs"
    section "Curve-Specific Optimizations" */
VOID Ecc::VliMmodFast384(UINT64 *pResult, UINT64 *pProduct)
{
    UINT64 l_tmp[2 * MAX_NUM_ECC_DIGITS];

    while (!this->VliIsZero(pProduct + this->numEccDigits)) /* While c1 != 0 */
    {
        UINT64 l_carry = 0;
        UINT32 i;

        this->VliClear(l_tmp);
        this->VliClear(l_tmp + this->numEccDigits);
        this->OmegaMult384(l_tmp, pProduct + this->numEccDigits); /* tmp = w * c1 */
        this->VliClear(pProduct + this->numEccDigits);            /* p = c0 */

        /* (c1, c0) = c0 + w * c1 */
        for (i = 0; i < this->numEccDigits + 3; ++i)
        {
            UINT64 l_sum = pProduct[i] + l_tmp[i] + l_carry;
            if (l_sum != pProduct[i])
            {
                l_carry = (l_sum < pProduct[i]);
            }
            pProduct[i] = l_sum;
        }
    }

    while (this->VliCmp(pProduct, this->curveP) > 0)
    {
        this->VliSub(pProduct, pProduct, this->curveP);
    }
    this->VliSet(pResult, pProduct);
}

/* Computes p_result = (p_left * p_right) % curve_p. */
VOID Ecc::VliModMultFast(UINT64 *pResult, UINT64 *pLeft, UINT64 *pRight)
{
    UINT64 l_product[2 * MAX_NUM_ECC_DIGITS];
    this->VliMult(l_product, pLeft, pRight);
    if (this->eccBytes == secp128r1)
        this->VliMmodFast128(pResult, l_product);
    else if (this->eccBytes == secp192r1)
        this->VliMmodFast192(pResult, l_product);
    else if (this->eccBytes == secp256r1)
        this->VliMmodFast256(pResult, l_product);
    else if (this->eccBytes == secp384r1)
        this->VliMmodFast384(pResult, l_product);
}

/* Computes p_result = p_left^2 % curveP. */
VOID Ecc::VliModSquareFast(UINT64 *pResult, UINT64 *pLeft)
{
    UINT64 l_product[2 * MAX_NUM_ECC_DIGITS];
    this->VliSquare(l_product, pLeft);
    if (this->eccBytes == secp128r1)
        this->VliMmodFast128(pResult, l_product);
    else if (this->eccBytes == secp192r1)
        this->VliMmodFast192(pResult, l_product);
    else if (this->eccBytes == secp256r1)
        this->VliMmodFast256(pResult, l_product);
    else if (this->eccBytes == secp384r1)
        this->VliMmodFast384(pResult, l_product);
}

/* Computes p_result = (1 / p_input) % p_mod. All VL== are the same size.
   See "From Euclid's GCD to Montgomery Multiplication to the Great Divide"
   https://labs.oracle.com/techrep/2001/smli_tr-2001-95.pdf */
VOID Ecc::VliModInv(UINT64 *pResult, UINT64 *pInput, UINT64 *pMod)
{
    UINT64 a[MAX_NUM_ECC_DIGITS], b[MAX_NUM_ECC_DIGITS], u[MAX_NUM_ECC_DIGITS], v[MAX_NUM_ECC_DIGITS];
    UINT64 l_carry;
    INT32 l_cmpResult;

    if (this->VliIsZero(pInput))
    {
        this->VliClear(pResult);
        return;
    }

    this->VliSet(a, pInput);
    this->VliSet(b, pMod);
    this->VliClear(u);
    u[0] = 1;
    this->VliClear(v);

    while ((l_cmpResult = this->VliCmp(a, b)) != 0)
    {
        l_carry = 0;
        if (EVEN(a))
        {
            this->VliRShift1(a);
            if (!EVEN(u))
            {
                l_carry = this->VliAdd(u, u, pMod);
            }
            this->VliRShift1(u);
            if (l_carry)
            {
                u[this->numEccDigits - 1] |= 0x8000000000000000ull;
            }
        }
        else if (EVEN(b))
        {
            this->VliRShift1(b);
            if (!EVEN(v))
            {
                l_carry = this->VliAdd(v, v, pMod);
            }
            this->VliRShift1(v);
            if (l_carry)
            {
                v[this->numEccDigits - 1] |= 0x8000000000000000ull;
            }
        }
        else if (l_cmpResult > 0)
        {
            this->VliSub(a, a, b);
            this->VliRShift1(a);
            if (this->VliCmp(u, v) < 0)
            {
                this->VliAdd(u, u, pMod);
            }
            this->VliSub(u, u, v);
            if (!EVEN(u))
            {
                l_carry = this->VliAdd(u, u, pMod);
            }
            this->VliRShift1(u);
            if (l_carry)
            {
                u[this->numEccDigits - 1] |= 0x8000000000000000ull;
            }
        }
        else
        {
            this->VliSub(b, b, a);
            this->VliRShift1(b);
            if (this->VliCmp(v, u) < 0)
            {
                this->VliAdd(v, v, pMod);
            }
            this->VliSub(v, v, u);
            if (!EVEN(v))
            {
                l_carry = this->VliAdd(v, v, pMod);
            }
            this->VliRShift1(v);
            if (l_carry)
            {
                v[this->numEccDigits - 1] |= 0x8000000000000000ull;
            }
        }
    }

    this->VliSet(pResult, u);
}

/* ------ Point operations ------ */

/* Returns 1 if p_point is the point at infinity, 0 otherwise. */
INT32 Ecc::IsZero(EccPoint *pPoint)
{
    return (this->VliIsZero(pPoint->x) && this->VliIsZero(pPoint->y));
}

/* Point multiplication algorithm using Montgomery's ladder with co-Z coordinates.
From http://eprint.iacr.org/2011/338.pdf
*/

/* Double in place */
VOID Ecc::DoubleJacobian(UINT64 *X1, UINT64 *Y1, UINT64 *Z1)
{
    /* t1 = X, t2 = Y, t3 = Z */
    UINT64 t4[MAX_NUM_ECC_DIGITS];
    UINT64 t5[MAX_NUM_ECC_DIGITS];

    if (this->VliIsZero(Z1))
    {
        return;
    }

    this->VliModSquareFast(t4, Y1);   /* t4 = y1^2 */
    this->VliModMultFast(t5, X1, t4); /* t5 = x1*y1^2 = A */
    this->VliModSquareFast(t4, t4);   /* t4 = y1^4 */
    this->VliModMultFast(Y1, Y1, Z1); /* t2 = y1*z1 = z3 */
    this->VliModSquareFast(Z1, Z1);   /* t3 = z1^2 */

    this->VliModAdd(X1, X1, Z1, this->curveP); /* t1 = x1 + z1^2 */
    this->VliModAdd(Z1, Z1, Z1, this->curveP); /* t3 = 2*z1^2 */
    this->VliModSub(Z1, X1, Z1, this->curveP); /* t3 = x1 - z1^2 */
    this->VliModMultFast(X1, X1, Z1);          /* t1 = x1^2 - z1^4 */

    this->VliModAdd(Z1, X1, X1, this->curveP); /* t3 = 2*(x1^2 - z1^4) */
    this->VliModAdd(X1, X1, Z1, this->curveP); /* t1 = 3*(x1^2 - z1^4) */
    if (this->VliTestBit(X1, 0))
    {
        UINT64 l_carry = this->VliAdd(X1, X1, this->curveP);
        this->VliRShift1(X1);
        X1[this->numEccDigits - 1] |= l_carry << 63;
    }
    else
    {
        this->VliRShift1(X1);
    }
    /* t1 = 3/2*(x1^2 - z1^4) = B */

    this->VliModSquareFast(Z1, X1);            /* t3 = B^2 */
    this->VliModSub(Z1, Z1, t5, this->curveP); /* t3 = B^2 - A */
    this->VliModSub(Z1, Z1, t5, this->curveP); /* t3 = B^2 - 2A = x3 */
    this->VliModSub(t5, t5, Z1, this->curveP); /* t5 = A - x3 */
    this->VliModMultFast(X1, X1, t5);          /* t1 = B * (A - x3) */
    this->VliModSub(t4, X1, t4, this->curveP); /* t4 = B * (A - x3) - y1^4 = y3 */
    this->VliSet(X1, Z1);
    this->VliSet(Z1, Y1);
    this->VliSet(Y1, t4);
}

/* Modify (x1, y1) => (x1 * z^2, y1 * z^3) */
VOID Ecc::ApplyZ(UINT64 *X1, UINT64 *Y1, UINT64 *Z)
{
    UINT64 t1[MAX_NUM_ECC_DIGITS];

    this->VliModSquareFast(t1, Z);    /* z^2 */
    this->VliModMultFast(X1, X1, t1); /* x1 * z^2 */
    this->VliModMultFast(t1, t1, Z);  /* z^3 */
    this->VliModMultFast(Y1, Y1, t1); /* y1 * z^3 */
}

/* P = (x1, y1) => 2P, (x2, y2) => P' */
VOID Ecc::XYcZInitialDouble(UINT64 *X1, UINT64 *Y1, UINT64 *X2, UINT64 *Y2, UINT64 *p_initialZ)
{
    UINT64 z[MAX_NUM_ECC_DIGITS];

    this->VliSet(X2, X1);
    this->VliSet(Y2, Y1);

    this->VliClear(z);
    z[0] = 1;
    if (p_initialZ)
    {
        this->VliSet(z, p_initialZ);
    }

    this->ApplyZ(X1, Y1, z);

    this->DoubleJacobian(X1, Y1, z);

    this->ApplyZ(X2, Y2, z);
}

/* Input P = (x1, y1, Z), Q = (x2, y2, Z)
   Output P' = (x1', y1', Z3), P + Q = (x3, y3, Z3)
   or P => P', Q => P + Q
*/
VOID Ecc::XYcZAdd(UINT64 *X1, UINT64 *Y1, UINT64 *X2, UINT64 *Y2)
{
    /* t1 = X1, t2 = Y1, t3 = X2, t4 = Y2 */
    UINT64 t5[MAX_NUM_ECC_DIGITS];

    this->VliModSub(t5, X2, X1, this->curveP); /* t5 = x2 - x1 */
    this->VliModSquareFast(t5, t5);            /* t5 = (x2 - x1)^2 = A */
    this->VliModMultFast(X1, X1, t5);          /* t1 = x1*A = B */
    this->VliModMultFast(X2, X2, t5);          /* t3 = x2*A = C */
    this->VliModSub(Y2, Y2, Y1, this->curveP); /* t4 = y2 - y1 */
    this->VliModSquareFast(t5, Y2);            /* t5 = (y2 - y1)^2 = D */

    this->VliModSub(t5, t5, X1, this->curveP); /* t5 = D - B */
    this->VliModSub(t5, t5, X2, this->curveP); /* t5 = D - B - C = x3 */
    this->VliModSub(X2, X2, X1, this->curveP); /* t3 = C - B */
    this->VliModMultFast(Y1, Y1, X2);          /* t2 = y1*(C - B) */
    this->VliModSub(X2, X1, t5, this->curveP); /* t3 = B - x3 */
    this->VliModMultFast(Y2, Y2, X2);          /* t4 = (y2 - y1)*(B - x3) */
    this->VliModSub(Y2, Y2, Y1, this->curveP); /* t4 = y3 */
    this->VliSet(X2, t5);
}

/* Input P = (x1, y1, Z), Q = (x2, y2, Z)
   Output P + Q = (x3, y3, Z3), P - Q = (x3', y3', Z3)
   or P => P - Q, Q => P + Q
*/
VOID Ecc::XYcZAddC(UINT64 *X1, UINT64 *Y1, UINT64 *X2, UINT64 *Y2)
{
    /* t1 = X1, t2 = Y1, t3 = X2, t4 = Y2 */
    UINT64 t5[MAX_NUM_ECC_DIGITS];
    UINT64 t6[MAX_NUM_ECC_DIGITS];
    UINT64 t7[MAX_NUM_ECC_DIGITS];

    this->VliModSub(t5, X2, X1, this->curveP); /* t5 = x2 - x1 */
    this->VliModSquareFast(t5, t5);            /* t5 = (x2 - x1)^2 = A */
    this->VliModMultFast(X1, X1, t5);          /* t1 = x1*A = B */
    this->VliModMultFast(X2, X2, t5);          /* t3 = x2*A = C */
    this->VliModAdd(t5, Y2, Y1, this->curveP); /* t4 = y2 + y1 */
    this->VliModSub(Y2, Y2, Y1, this->curveP); /* t4 = y2 - y1 */

    this->VliModSub(t6, X2, X1, this->curveP); /* t6 = C - B */
    this->VliModMultFast(Y1, Y1, t6);          /* t2 = y1 * (C - B) */
    this->VliModAdd(t6, X1, X2, this->curveP); /* t6 = B + C */
    this->VliModSquareFast(X2, Y2);            /* t3 = (y2 - y1)^2 */
    this->VliModSub(X2, X2, t6, this->curveP); /* t3 = x3 */

    this->VliModSub(t7, X1, X2, this->curveP); /* t7 = B - x3 */
    this->VliModMultFast(Y2, Y2, t7);          /* t4 = (y2 - y1)*(B - x3) */
    this->VliModSub(Y2, Y2, Y1, this->curveP); /* t4 = y3 */

    this->VliModSquareFast(t7, t5);            /* t7 = (y2 + y1)^2 = F */
    this->VliModSub(t7, t7, t6, this->curveP); /* t7 = x3' */
    this->VliModSub(t6, t7, X1, this->curveP); /* t6 = x3' - B */
    this->VliModMultFast(t6, t6, t5);          /* t6 = (y2 + y1)*(x3' - B) */
    this->VliModSub(Y1, t6, Y1, this->curveP); /* t2 = y3' */

    this->VliSet(X1, t7);
}

VOID Ecc::Mult(EccPoint *pResult, EccPoint *pPoint, UINT64 *pScalar, UINT64 *pInitialZ)
{
    /* R0 and R1 */
    UINT64 Rx[2][MAX_NUM_ECC_DIGITS];
    UINT64 Ry[2][MAX_NUM_ECC_DIGITS];
    UINT64 z[MAX_NUM_ECC_DIGITS];

    INT32 i, nb;

    this->VliSet(Rx[1], pPoint->x);
    this->VliSet(Ry[1], pPoint->y);

    this->XYcZInitialDouble(Rx[1], Ry[1], Rx[0], Ry[0], pInitialZ);

    for (i = this->VliNumBits(pScalar) - 2; i > 0; --i)
    {
        nb = !(this->VliTestBit(pScalar, i));
        this->XYcZAddC(Rx[1 - nb], Ry[1 - nb], Rx[nb], Ry[nb]);
        this->XYcZAdd(Rx[nb], Ry[nb], Rx[1 - nb], Ry[1 - nb]);
    }

    nb = !(this->VliTestBit(pScalar, 0));
    this->XYcZAddC(Rx[1 - nb], Ry[1 - nb], Rx[nb], Ry[nb]);
    /* Find final 1/Z value. */
    this->VliModSub(z, Rx[1], Rx[0], this->curveP); /* X1 - X0 */
    this->VliModMultFast(z, z, Ry[1 - nb]);         /* Yb * (X1 - X0) */
    this->VliModMultFast(z, z, pPoint->x);          /* xP * Yb * (X1 - X0) */
    this->VliModInv(z, z, this->curveP);            /* 1 / (xP * Yb * (X1 - X0)) */
    this->VliModMultFast(z, z, pPoint->y);          /* yP / (xP * Yb * (X1 - X0)) */
    this->VliModMultFast(z, z, Rx[1 - nb]);         /* Xb * yP / (xP * Yb * (X1 - X0)) */
    /* End 1/Z calculation */

    this->XYcZAdd(Rx[nb], Ry[nb], Rx[1 - nb], Ry[1 - nb]);

    this->ApplyZ(Rx[0], Ry[0], z);

    this->VliSet(pResult->x, Rx[0]);
    this->VliSet(pResult->y, Ry[0]);
}

VOID Ecc::Bytes2Native(UINT64 *pNative, const UINT8 *pBytes)
{
    UINT32 i;
    for (i = 0; i < this->numEccDigits; ++i)
    {
        const UINT8 *p_digit = pBytes + 8 * (this->numEccDigits - 1 - i);
        pNative[i] = ((UINT64)p_digit[0] << 56) | ((UINT64)p_digit[1] << 48) | ((UINT64)p_digit[2] << 40) | ((UINT64)p_digit[3] << 32) |
                     ((UINT64)p_digit[4] << 24) | ((UINT64)p_digit[5] << 16) | ((UINT64)p_digit[6] << 8) | (UINT64)p_digit[7];
    }
}

VOID Ecc::Native2Bytes(UINT8 *pBytes, const UINT64 *pNative)
{
    UINT32 i;
    for (i = 0; i < this->numEccDigits; ++i)
    {
        UINT8 *p_digit = pBytes + 8 * (this->numEccDigits - 1 - i);
        p_digit[0] = (UINT8)(pNative[i] >> 56);
        p_digit[1] = (UINT8)(pNative[i] >> 48);
        p_digit[2] = (UINT8)(pNative[i] >> 40);
        p_digit[3] = (UINT8)(pNative[i] >> 32);
        p_digit[4] = (UINT8)(pNative[i] >> 24);
        p_digit[5] = (UINT8)(pNative[i] >> 16);
        p_digit[6] = (UINT8)(pNative[i] >> 8);
        p_digit[7] = (UINT8)pNative[i];
    }
}

/* Compute a = sqrt(a) (mod curveP). */
VOID Ecc::ModSqrt(UINT64 *pA)
{
    UINT32 i;
    UINT64 p1[MAX_NUM_ECC_DIGITS] = {1};
    UINT64 l_result[MAX_NUM_ECC_DIGITS] = {1};

    /* Since curveP == 3 (mod 4) for all supported curves, we can
       compute sqrt(a) = a^((curveP + 1) / 4) (mod curveP). */
    this->VliAdd(p1, this->curveP, p1); /* p1 = curveP + 1 */
    for (i = this->VliNumBits(p1) - 1; i > 1; --i)
    {
        this->VliModSquareFast(l_result, l_result);
        if (this->VliTestBit(p1, i))
        {
            this->VliModMultFast(l_result, l_result, pA);
        }
    }
    this->VliSet(pA, l_result);
}

VOID Ecc::PointDecompress(EccPoint *pPoint, const UINT8 *pCompressed)
{
    UINT64 _3[MAX_NUM_ECC_DIGITS] = {3}; /* -a = 3 */
    this->Bytes2Native(pPoint->x, pCompressed + 1);

    this->VliModSquareFast(pPoint->y, pPoint->x);                      /* y = x^2 */
    this->VliModSub(pPoint->y, pPoint->y, _3, this->curveP);           /* y = x^2 - 3 */
    this->VliModMultFast(pPoint->y, pPoint->y, pPoint->x);             /* y = x^3 - 3x */
    this->VliModAdd(pPoint->y, pPoint->y, this->curveB, this->curveP); /* y = x^3 - 3x + b */

    this->ModSqrt(pPoint->y);

    if ((pPoint->y[0] & 0x01) != (pCompressed[0] & 0x01))
    {
        this->VliSub(pPoint->y, this->curveP, pPoint->y);
    }
}

Ecc::Ecc()
{
    this->eccBytes = 0;
    this->numEccDigits = 0;
    Memory::Set(this->privateKey, 0, sizeof(this->privateKey));
    Memory::Set(this->publicKey.x, 0, sizeof(this->publicKey.x));
    Memory::Set(this->publicKey.y, 0, sizeof(this->publicKey.y));
    Memory::Set(this->curveP, 0, sizeof(this->curveP));
    Memory::Set(this->curveB, 0, sizeof(this->curveB));
    Memory::Set(this->curveG.x, 0, sizeof(this->curveG.x));
    Memory::Set(this->curveG.y, 0, sizeof(this->curveG.y));
    Memory::Set(this->curveN, 0, sizeof(this->curveN));
}

INT32 Ecc::ComputeSharedSecret(const UINT8 *publicKey, UINT32 publicKeySize, UINT8 *secret)
{
    if (publicKeySize != this->eccBytes * 2 + 1 || publicKey[0] != 0x04)
        return -1;

    Random random;
    EccPoint l_public;
    UINT64 l_random[MAX_NUM_ECC_DIGITS];

    if (!random.GetArray((USIZE)(this->numEccDigits * sizeof(UINT64)), (UINT8 *)l_random))
    {
        return 0;
    }
    this->Bytes2Native(l_public.x, publicKey + 1);
    this->Bytes2Native(l_public.y, publicKey + 1 + this->eccBytes);

    EccPoint l_product;
    this->Mult(&l_product, &l_public, this->privateKey, l_random);
    this->Native2Bytes(secret, l_product.x);
    return this->IsZero(&l_product) ? -1 : 0;
}

INT32 Ecc::Initialize(INT32 bytes)
{
    this->eccBytes = bytes;
    this->numEccDigits = bytes >> 3;
    if (bytes == secp128r1)
    {
        Memory::Copy(this->curveP, MakeEmbedArray(Curve_P_16), sizeof(Curve_P_16));
        Memory::Copy(this->curveB, MakeEmbedArray(Curve_B_16), sizeof(Curve_B_16));
        Memory::Copy(this->curveG.x, MakeEmbedArray(Curve_G_16.x), sizeof(Curve_G_16.x));
        Memory::Copy(this->curveG.y, MakeEmbedArray(Curve_G_16.y), sizeof(Curve_G_16.y));
        Memory::Copy(this->curveN, MakeEmbedArray(Curve_N_16), sizeof(Curve_N_16));
    }
    else if (bytes == secp192r1)
    {
        Memory::Copy(this->curveP, MakeEmbedArray(Curve_P_24), sizeof(Curve_P_24));
        Memory::Copy(this->curveB, MakeEmbedArray(Curve_B_24), sizeof(Curve_B_24));
        Memory::Copy(this->curveG.x, MakeEmbedArray(Curve_G_24.x), sizeof(Curve_G_24.x));
        Memory::Copy(this->curveG.y, MakeEmbedArray(Curve_G_24.y), sizeof(Curve_G_24.y));
        Memory::Copy(this->curveN, MakeEmbedArray(Curve_N_24), sizeof(Curve_N_24));
    }
    else if (bytes == secp256r1)
    {
        Memory::Copy(this->curveP, MakeEmbedArray(Curve_P_32), sizeof(Curve_P_32));
        Memory::Copy(this->curveB, MakeEmbedArray(Curve_B_32), sizeof(Curve_B_32));
        Memory::Copy(this->curveG.x, MakeEmbedArray(Curve_G_32.x), sizeof(Curve_G_32.x));
        Memory::Copy(this->curveG.y, MakeEmbedArray(Curve_G_32.y), sizeof(Curve_G_32.y));
        Memory::Copy(this->curveN, MakeEmbedArray(Curve_N_32), sizeof(Curve_N_32));
    }
    else if (bytes == secp384r1)
    {
        Memory::Copy(this->curveP, MakeEmbedArray(Curve_P_48), sizeof(Curve_P_48));
        Memory::Copy(this->curveB, MakeEmbedArray(Curve_B_48), sizeof(Curve_B_48));
        Memory::Copy(this->curveG.x, MakeEmbedArray(Curve_G_48.x), sizeof(Curve_G_48.x));
        Memory::Copy(this->curveG.y, MakeEmbedArray(Curve_G_48.y), sizeof(Curve_G_48.y));
        Memory::Copy(this->curveN, MakeEmbedArray(Curve_N_48), sizeof(Curve_N_48));
    }
    else
        return -1;

    UINT32 l_tries = 0;

    do
    {
        Random random;
        if (!random.GetArray((USIZE)(this->numEccDigits * sizeof(UINT64)), (UINT8 *)this->privateKey) || (l_tries++ >= MAX_TRIES))
            return -1;
        if (this->VliIsZero(this->privateKey))
            continue;

        /* Make sure the private key is in the range [1, n-1].
           For the supported curves, n is always large enough that we only need to subtract once at most. */
        if (this->VliCmp(this->curveN, this->privateKey) != 1)
            this->VliSub(this->privateKey, this->privateKey, this->curveN);

        this->Mult(&this->publicKey, &this->curveG, this->privateKey, NULL);
    } while (this->IsZero(&this->publicKey));
    return 0;
};

INT32 Ecc::ExportPublicKey(UINT8 *publicKey, UINT32 publicKeySize)
{
    if (publicKey == 0 || publicKeySize < this->eccBytes * 2 + 1)
        return 0;
    publicKey[0] = 0x04;
    this->Native2Bytes(publicKey + 1, this->publicKey.x);
    this->Native2Bytes(publicKey + 1 + this->eccBytes, this->publicKey.y);
    return this->eccBytes * 2 + 1;
}