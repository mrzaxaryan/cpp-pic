/**
 * sha2.h - FIPS 180-2 SHA-256/384 and HMAC Implementation
 *
 * PIC-safe cryptographic hash functions for position-independent code.
 * Uses EMBEDDED_ARRAY to avoid .rdata section dependencies.
 *
 * USAGE:
 *   // One-shot hash
 *   UINT8 digest[SHA256_DIGEST_SIZE];
 *   SHA256::Hash(message, len, digest);
 *
 *   // Incremental hash
 *   SHA256 ctx;
 *   ctx.Update(part1, len1);
 *   ctx.Update(part2, len2);
 *   ctx.Final(digest);
 *
 *   // HMAC
 *   UINT8 mac[SHA256_DIGEST_SIZE];
 *   HMAC_SHA256::Compute(key, keyLen, message, msgLen, mac, sizeof(mac));
 */

#pragma once

#include "core.h"

#define SHA256_DIGEST_SIZE (256 / 8)
#define SHA384_DIGEST_SIZE (384 / 8)
#define SHA256_BLOCK_SIZE  (512 / 8)
#define SHA384_BLOCK_SIZE  (1024 / 8)

struct SHA256Traits;
struct SHA384Traits;
template<typename Traits> class SHABase;
template<typename SHAType, typename Traits> class HMACBase;

struct SHA256Traits
{
    using Word = UINT32;
    static constexpr USIZE BLOCK_SIZE    = SHA256_BLOCK_SIZE;
    static constexpr USIZE DIGEST_SIZE   = SHA256_DIGEST_SIZE;
    static constexpr USIZE ROUND_COUNT   = 64;
    static constexpr USIZE OUTPUT_WORDS  = 8;
    static constexpr USIZE BLOCK_SHIFT   = 6;
    static constexpr USIZE WORD_SHIFT    = 2;
    static constexpr USIZE PADDING_OFFSET = 9;

    static FORCE_INLINE VOID FillH0(Word* out);
    static FORCE_INLINE VOID FillK(Word* out);
    static FORCE_INLINE VOID Pack(const UINT8* str, Word* x);
    static FORCE_INLINE VOID Unpack(Word x, UINT8* str);
    static FORCE_INLINE Word F1(Word x) { return ROTR(x, 2) ^ ROTR(x, 13) ^ ROTR(x, 22); }
    static FORCE_INLINE Word F2(Word x) { return ROTR(x, 6) ^ ROTR(x, 11) ^ ROTR(x, 25); }
    static FORCE_INLINE Word F3(Word x) { return ROTR(x, 7) ^ ROTR(x, 18) ^ (x >> 3); }
    static FORCE_INLINE Word F4(Word x) { return ROTR(x, 17) ^ ROTR(x, 19) ^ (x >> 10); }

private:
    static FORCE_INLINE Word ROTR(Word x, UINT32 n) { return (x >> n) | (x << (32 - n)); }
};

struct SHA384Traits
{
    using Word = UINT64;
    static constexpr USIZE BLOCK_SIZE    = SHA384_BLOCK_SIZE;
    static constexpr USIZE DIGEST_SIZE   = SHA384_DIGEST_SIZE;
    static constexpr USIZE ROUND_COUNT   = 80;
    static constexpr USIZE OUTPUT_WORDS  = 6;
    static constexpr USIZE BLOCK_SHIFT   = 7;
    static constexpr USIZE WORD_SHIFT    = 3;
    static constexpr USIZE PADDING_OFFSET = 17;

    static FORCE_INLINE VOID FillH0(Word* out);
    static FORCE_INLINE VOID FillK(Word* out);
    static FORCE_INLINE VOID Pack(const UINT8* str, Word* x);
    static FORCE_INLINE VOID Unpack(Word x, UINT8* str);
    static FORCE_INLINE Word F1(Word x) { return ROTR(x, 28) ^ ROTR(x, 34) ^ ROTR(x, 39); }
    static FORCE_INLINE Word F2(Word x) { return ROTR(x, 14) ^ ROTR(x, 18) ^ ROTR(x, 41); }
    static FORCE_INLINE Word F3(Word x) { return ROTR(x, 1) ^ ROTR(x, 8) ^ (x >> 7); }
    static FORCE_INLINE Word F4(Word x) { return ROTR(x, 19) ^ ROTR(x, 61) ^ (x >> 6); }

private:
    static FORCE_INLINE Word ROTR(Word x, UINT32 n) { return (x >> n) | (x << (64 - n)); }
};

template<typename Traits>
class SHABase
{
public:
    using Word = typename Traits::Word;

private:
    UINT64 tot_len;
    UINT64 len;
    UINT8 block[2 * Traits::BLOCK_SIZE];
    Word h[8];

public:
    SHABase();
    VOID Update(const UINT8 *message, UINT64 len);
    VOID Final(UINT8 *digest);
    static VOID Hash(const UINT8 *message, UINT64 len, UINT8 *digest);
    static VOID Transform(SHABase *ctx, const UINT8 *message, UINT64 block_nb);
};

using SHA256 = SHABase<SHA256Traits>;
using SHA384 = SHABase<SHA384Traits>;

template<typename SHAType, typename Traits>
class HMACBase
{
private:
    SHAType ctx_inside;
    SHAType ctx_outside;
    SHAType ctx_inside_reinit;
    SHAType ctx_outside_reinit;
    UCHAR block_ipad[Traits::BLOCK_SIZE];
    UCHAR block_opad[Traits::BLOCK_SIZE];

public:
    VOID Init(const UCHAR *key, UINT32 key_size);
    VOID Reinit();
    VOID Update(const UCHAR *message, UINT32 messageLen);
    VOID Final(PUCHAR mac, UINT32 macSize);
    static VOID Compute(const UCHAR *key, UINT32 keySize, const UCHAR *message, UINT32 messageLen, PUCHAR mac, UINT32 macSize);
};

using HMAC_SHA256 = HMACBase<SHA256, SHA256Traits>;
using HMAC_SHA384 = HMACBase<SHA384, SHA384Traits>;
