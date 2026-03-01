#include "chacha20.h"
#include "memory.h"
#include "platform.h"
#include "logger.h"
#include "bitops.h"

// ChaCha20 implementation by D. J. Bernstein
// Public domain.

// Constants
#define U8C(v) (v##U)

#define U8V(v) ((UINT8)(v) & U8C(0xFF))
#define U32V(v) ((UINT32)(v) & (0xFFFFFFFF))

#define _private_tls_U8TO32_LITTLE(p) \
    (((UINT32)((p)[0])) |             \
     ((UINT32)((p)[1]) << 8) |        \
     ((UINT32)((p)[2]) << 16) |       \
     ((UINT32)((p)[3]) << 24))

#define _private_tls_U32TO8_LITTLE(p, v) \
    do                                   \
    {                                    \
        (p)[0] = U8V((v));               \
        (p)[1] = U8V((v) >> 8);          \
        (p)[2] = U8V((v) >> 16);         \
        (p)[3] = U8V((v) >> 24);         \
    } while (0)

#define ROTATE(v, c) (BitOps::ROTL32(v, c))
#define XOR(v, w) ((v) ^ (w))
#define PLUS(v, w) (U32V((v) + (w)))
#define PLUSONE(v) (PLUS((v), 1))

#define QUARTERROUND(a, b, c, d) \
    a = PLUS(a, b);              \
    d = ROTATE(XOR(d, a), 16);   \
    c = PLUS(c, d);              \
    b = ROTATE(XOR(b, c), 12);   \
    a = PLUS(a, b);              \
    d = ROTATE(XOR(d, a), 8);    \
    c = PLUS(c, d);              \
    b = ROTATE(XOR(b, c), 7);

//========== Poly1305 Class Implementation ========= //

/* interpret four 8 bit unsigned integers as a 32 bit unsigned integer in little endian */
Poly1305::Poly1305(const UCHAR (&key)[32])
{
    /* r &= 0xffffffc0ffffffc0ffffffc0fffffff */
    m_r[0] = (U8TO32(&key[0])) & 0x3ffffff;
    m_r[1] = (U8TO32(&key[3]) >> 2) & 0x3ffff03;
    m_r[2] = (U8TO32(&key[6]) >> 4) & 0x3ffc0ff;
    m_r[3] = (U8TO32(&key[9]) >> 6) & 0x3f03fff;
    m_r[4] = (U8TO32(&key[12]) >> 8) & 0x00fffff;

    /* h = 0 */
    Memory::Zero(m_h, sizeof(m_h));

    /* save pad for later */
    m_pad[0] = U8TO32(&key[16]);
    m_pad[1] = U8TO32(&key[20]);
    m_pad[2] = U8TO32(&key[24]);
    m_pad[3] = U8TO32(&key[28]);

    m_leftover = 0;
    m_final = 0;
}

Poly1305::~Poly1305()
{
    /* zero out sensitive state */
    Memory::Zero(m_h, sizeof(m_h));
    Memory::Zero(m_r, sizeof(m_r));
    Memory::Zero(m_pad, sizeof(m_pad));
}

UINT32 Poly1305::U8TO32(const UCHAR *p)
{
    return (((UINT32)(p[0] & 0xff)) |
            ((UINT32)(p[1] & 0xff) << 8) |
            ((UINT32)(p[2] & 0xff) << 16) |
            ((UINT32)(p[3] & 0xff) << 24));
}

/* store a 32 bit unsigned integer as four 8 bit unsigned integers in little endian */
VOID Poly1305::U32TO8(PUCHAR p, UINT32 v)
{
    p[0] = (v) & 0xff;
    p[1] = (v >> 8) & 0xff;
    p[2] = (v >> 16) & 0xff;
    p[3] = (v >> 24) & 0xff;
}

VOID Poly1305::ProcessBlocks(Span<const UCHAR> data)
{
    const UINT32 hibit = (m_final) ? 0 : (1UL << 24); /* 1 << 128 */
    UINT32 r0, r1, r2, r3, r4;
    UINT32 s1, s2, s3, s4;
    UINT32 h0, h1, h2, h3, h4;
    UINT64 d0, d1, d2, d3, d4;
    UINT32 c;

    const UCHAR *p = data.Data();
    USIZE bytes = data.Size();

    r0 = m_r[0];
    r1 = m_r[1];
    r2 = m_r[2];
    r3 = m_r[3];
    r4 = m_r[4];

    s1 = r1 * 5;
    s2 = r2 * 5;
    s3 = r3 * 5;
    s4 = r4 * 5;

    h0 = m_h[0];
    h1 = m_h[1];
    h2 = m_h[2];
    h3 = m_h[3];
    h4 = m_h[4];

    while (bytes >= POLY1305_BLOCK_SIZE)
    {
        /* h += m[i] */
        h0 += (U8TO32(p + 0)) & 0x3ffffff;
        h1 += (U8TO32(p + 3) >> 2) & 0x3ffffff;
        h2 += (U8TO32(p + 6) >> 4) & 0x3ffffff;
        h3 += (U8TO32(p + 9) >> 6) & 0x3ffffff;
        h4 += (U8TO32(p + 12) >> 8) | hibit;

        /* h *= r */
        d0 = ((UINT64)h0 * r0) + ((UINT64)h1 * s4) + ((UINT64)h2 * s3) + ((UINT64)h3 * s2) + ((UINT64)h4 * s1);
        d1 = ((UINT64)h0 * r1) + ((UINT64)h1 * r0) + ((UINT64)h2 * s4) + ((UINT64)h3 * s3) + ((UINT64)h4 * s2);
        d2 = ((UINT64)h0 * r2) + ((UINT64)h1 * r1) + ((UINT64)h2 * r0) + ((UINT64)h3 * s4) + ((UINT64)h4 * s3);
        d3 = ((UINT64)h0 * r3) + ((UINT64)h1 * r2) + ((UINT64)h2 * r1) + ((UINT64)h3 * r0) + ((UINT64)h4 * s4);
        d4 = ((UINT64)h0 * r4) + ((UINT64)h1 * r3) + ((UINT64)h2 * r2) + ((UINT64)h3 * r1) + ((UINT64)h4 * r0);

        /* (partial) h %= p */
        c = (UINT32)(d0 >> 26);
        h0 = (UINT32)d0 & 0x3ffffff;
        d1 += c;
        c = (UINT32)(d1 >> 26);
        h1 = (UINT32)d1 & 0x3ffffff;
        d2 += c;
        c = (UINT32)(d2 >> 26);
        h2 = (UINT32)d2 & 0x3ffffff;
        d3 += c;
        c = (UINT32)(d3 >> 26);
        h3 = (UINT32)d3 & 0x3ffffff;
        d4 += c;
        c = (UINT32)(d4 >> 26);
        h4 = (UINT32)d4 & 0x3ffffff;
        h0 += c * 5;
        c = (h0 >> 26);
        h0 = h0 & 0x3ffffff;
        h1 += c;

        p += POLY1305_BLOCK_SIZE;
        bytes -= POLY1305_BLOCK_SIZE;
    }

    m_h[0] = h0;
    m_h[1] = h1;
    m_h[2] = h2;
    m_h[3] = h3;
    m_h[4] = h4;
}

Result<void, Error> Poly1305::GenerateKey(Span<const UCHAR, POLY1305_KEYLEN> key256, Span<const UCHAR> nonce, Span<UCHAR, POLY1305_KEYLEN> poly_key, UINT32 counter)
{
    ChaChaPoly1305 ctx;
    UINT64 ctr;
    ctx.KeySetup(key256);

    if (nonce.Size() == 8)
    {
        ctr = counter;
        ctx.IvSetup(nonce.Data(), (PUCHAR)&ctr);
    }
    else if (nonce.Size() == 12)
    {
        ctx.IVSetup96BitNonce(nonce.Data(), (PUCHAR)&counter);
    }
    else
    {
        return Result<void, Error>::Err(Error::ChaCha20_GenerateKeyFailed);
    }

    ctx.Block(poly_key);
    return Result<void, Error>::Ok();
}

VOID Poly1305::Update(Span<const UCHAR> data)
{
    const UCHAR *p = data.Data();
    USIZE bytes = data.Size();
    USIZE i;
    /* handle leftover */
    if (m_leftover)
    {
        USIZE want = (POLY1305_BLOCK_SIZE - m_leftover);
        if (want > bytes)
            want = bytes;
        for (i = 0; i < want; i++)
            m_buffer[m_leftover + i] = p[i];
        bytes -= want;
        p += want;
        m_leftover += want;
        if (m_leftover < POLY1305_BLOCK_SIZE)
            return;
        ProcessBlocks(Span<const UCHAR>(m_buffer, POLY1305_BLOCK_SIZE));
        m_leftover = 0;
    }

    /* process full blocks */
    if (bytes >= POLY1305_BLOCK_SIZE)
    {
        USIZE want = (bytes & ~(POLY1305_BLOCK_SIZE - 1));
        ProcessBlocks(Span<const UCHAR>(p, want));
        p += want;
        bytes -= want;
    }

    /* store leftover */
    if (bytes)
    {
        for (i = 0; i < bytes; i++)
            m_buffer[m_leftover + i] = p[i];
        m_leftover += bytes;
    }
}

VOID Poly1305::Finish(Span<UCHAR, POLY1305_TAGLEN> mac)
{
    UINT32 h0, h1, h2, h3, h4, c;
    UINT32 g0, g1, g2, g3, g4;
    UINT64 f;
    UINT32 mask;

    /* process the remaining block */
    if (m_leftover)
    {
        USIZE i = m_leftover;
        m_buffer[i++] = 1;
        Memory::Zero(&m_buffer[i], POLY1305_BLOCK_SIZE - i);
        m_final = 1;
        ProcessBlocks(Span<const UCHAR>(m_buffer, POLY1305_BLOCK_SIZE));
    }

    /* fully carry h */
    h0 = m_h[0];
    h1 = m_h[1];
    h2 = m_h[2];
    h3 = m_h[3];
    h4 = m_h[4];

    c = h1 >> 26;
    h1 = h1 & 0x3ffffff;
    h2 += c;
    c = h2 >> 26;
    h2 = h2 & 0x3ffffff;
    h3 += c;
    c = h3 >> 26;
    h3 = h3 & 0x3ffffff;
    h4 += c;
    c = h4 >> 26;
    h4 = h4 & 0x3ffffff;
    h0 += c * 5;
    c = h0 >> 26;
    h0 = h0 & 0x3ffffff;
    h1 += c;

    /* compute h + -p */
    g0 = h0 + 5;
    c = g0 >> 26;
    g0 &= 0x3ffffff;
    g1 = h1 + c;
    c = g1 >> 26;
    g1 &= 0x3ffffff;
    g2 = h2 + c;
    c = g2 >> 26;
    g2 &= 0x3ffffff;
    g3 = h3 + c;
    c = g3 >> 26;
    g3 &= 0x3ffffff;
    g4 = h4 + c - (1UL << 26);

    /* select h if h < p, or h + -p if h >= p */
    mask = (g4 >> ((sizeof(UINT32) * 8) - 1)) - 1;
    g0 &= mask;
    g1 &= mask;
    g2 &= mask;
    g3 &= mask;
    g4 &= mask;
    mask = ~mask;
    h0 = (h0 & mask) | g0;
    h1 = (h1 & mask) | g1;
    h2 = (h2 & mask) | g2;
    h3 = (h3 & mask) | g3;
    h4 = (h4 & mask) | g4;

    /* h = h % (2^128) */
    h0 = ((h0) | (h1 << 26)) & 0xffffffff;
    h1 = ((h1 >> 6) | (h2 << 20)) & 0xffffffff;
    h2 = ((h2 >> 12) | (h3 << 14)) & 0xffffffff;
    h3 = ((h3 >> 18) | (h4 << 8)) & 0xffffffff;

    /* mac = (h + pad) % (2^128) */
    f = (UINT64)h0 + m_pad[0];
    h0 = (UINT32)f;
    f = (UINT64)h1 + m_pad[1] + (f >> 32);
    h1 = (UINT32)f;
    f = (UINT64)h2 + m_pad[2] + (f >> 32);
    h2 = (UINT32)f;
    f = (UINT64)h3 + m_pad[3] + (f >> 32);
    h3 = (UINT32)f;

    U32TO8(mac.Data() + 0, h0);
    U32TO8(mac.Data() + 4, h1);
    U32TO8(mac.Data() + 8, h2);
    U32TO8(mac.Data() + 12, h3);

    /* zero out the state */
    Memory::Zero(m_h, sizeof(m_h));
    Memory::Zero(m_r, sizeof(m_r));
    Memory::Zero(m_pad, sizeof(m_pad));
}

//========== ChaCha20 from D. J. Bernstein ========= //
// Source available at https://cr.yp.to/chacha.html  //

VOID ChaChaPoly1305::KeySetup(Span<const UINT8> key)
{
    const UINT8 *k = key.Data();
    UINT32 kbits = (UINT32)key.Size() * 8;

    this->input[4] = _private_tls_U8TO32_LITTLE(k + 0);
    this->input[5] = _private_tls_U8TO32_LITTLE(k + 4);
    this->input[6] = _private_tls_U8TO32_LITTLE(k + 8);
    this->input[7] = _private_tls_U8TO32_LITTLE(k + 12);
    // Declare _embed strings separately to avoid type deduction issues with ternary
    auto constants32 = "expand 32-byte k"_embed;
    auto constants16 = "expand 16-byte k"_embed;
    const CHAR *constants = kbits == 256 ? (const CHAR *)constants32 : (const CHAR *)constants16;
    if (kbits == 256)
    { /* recommended */
        k += 16;
    }
    this->input[8] = _private_tls_U8TO32_LITTLE(k + 0);
    this->input[9] = _private_tls_U8TO32_LITTLE(k + 4);
    this->input[10] = _private_tls_U8TO32_LITTLE(k + 8);
    this->input[11] = _private_tls_U8TO32_LITTLE(k + 12);
    this->input[0] = _private_tls_U8TO32_LITTLE(constants + 0);
    this->input[1] = _private_tls_U8TO32_LITTLE(constants + 4);
    this->input[2] = _private_tls_U8TO32_LITTLE(constants + 8);
    this->input[3] = _private_tls_U8TO32_LITTLE(constants + 12);
}

VOID ChaChaPoly1305::Key(UINT8 (&k)[32])
{
    _private_tls_U32TO8_LITTLE(k, this->input[4]);
    _private_tls_U32TO8_LITTLE(k + 4, this->input[5]);
    _private_tls_U32TO8_LITTLE(k + 8, this->input[6]);
    _private_tls_U32TO8_LITTLE(k + 12, this->input[7]);

    _private_tls_U32TO8_LITTLE(k + 16, this->input[8]);
    _private_tls_U32TO8_LITTLE(k + 20, this->input[9]);
    _private_tls_U32TO8_LITTLE(k + 24, this->input[10]);
    _private_tls_U32TO8_LITTLE(k + 28, this->input[11]);
}

VOID ChaChaPoly1305::Nonce(UINT8 (&nonce)[TLS_CHACHA20_IV_LENGTH])
{
    _private_tls_U32TO8_LITTLE(nonce + 0, this->input[13]);
    _private_tls_U32TO8_LITTLE(nonce + 4, this->input[14]);
    _private_tls_U32TO8_LITTLE(nonce + 8, this->input[15]);
}

VOID ChaChaPoly1305::IvSetup(const UINT8 *iv, const UINT8 *counter)
{
    this->input[12] = counter == nullptr ? 0 : _private_tls_U8TO32_LITTLE(counter + 0);
    this->input[13] = counter == nullptr ? 0 : _private_tls_U8TO32_LITTLE(counter + 4);
    if (iv)
    {
        this->input[14] = _private_tls_U8TO32_LITTLE(iv + 0);
        this->input[15] = _private_tls_U8TO32_LITTLE(iv + 4);
    }
}

VOID ChaChaPoly1305::IVSetup96BitNonce(const UINT8 *iv, const UINT8 *counter)
{
    this->input[12] = counter == nullptr ? 0 : _private_tls_U8TO32_LITTLE(counter + 0);
    if (iv)
    {
        this->input[13] = _private_tls_U8TO32_LITTLE(iv + 0);
        this->input[14] = _private_tls_U8TO32_LITTLE(iv + 4);
        this->input[15] = _private_tls_U8TO32_LITTLE(iv + 8);
    }
}

VOID ChaChaPoly1305::IvUpdate(Span<const UINT8, TLS_CHACHA20_IV_LENGTH> iv, Span<const UINT8> aad, const UINT8 *counter)
{
    this->input[12] = counter == nullptr ? 0 : _private_tls_U8TO32_LITTLE(counter + 0);
    this->input[13] = _private_tls_U8TO32_LITTLE(iv.Data() + 0);
    this->input[14] = _private_tls_U8TO32_LITTLE(iv.Data() + 4) ^ _private_tls_U8TO32_LITTLE(aad.Data());
    this->input[15] = _private_tls_U8TO32_LITTLE(iv.Data() + 8) ^ _private_tls_U8TO32_LITTLE(aad.Data() + 4);
}

VOID ChaChaPoly1305::EncryptBytes(Span<const UINT8> m_span, Span<UINT8> c_span)
{
    const UINT8 *m = m_span.Data();
    UINT8 *c = c_span.Data();
    UINT32 bytes = (UINT32)m_span.Size();

    UINT32 x0, x1, x2, x3, x4, x5, x6, x7;
    UINT32 x8, x9, x10, x11, x12, x13, x14, x15;
    UINT32 j0, j1, j2, j3, j4, j5, j6, j7;
    UINT32 j8, j9, j10, j11, j12, j13, j14, j15;
    UINT8 *ctarget = nullptr;
    UINT8 tmp[64];
    UINT32 i;

    if (!bytes)
        return;

    j0 = this->input[0];
    j1 = this->input[1];
    j2 = this->input[2];
    j3 = this->input[3];
    j4 = this->input[4];
    j5 = this->input[5];
    j6 = this->input[6];
    j7 = this->input[7];
    j8 = this->input[8];
    j9 = this->input[9];
    j10 = this->input[10];
    j11 = this->input[11];
    j12 = this->input[12];
    j13 = this->input[13];
    j14 = this->input[14];
    j15 = this->input[15];

    for (;;)
    {
        if (bytes < 64)
        {
            for (i = 0; i < bytes; ++i)
                tmp[i] = m[i];
            m = tmp;
            ctarget = c;
            c = tmp;
        }
        x0 = j0;
        x1 = j1;
        x2 = j2;
        x3 = j3;
        x4 = j4;
        x5 = j5;
        x6 = j6;
        x7 = j7;
        x8 = j8;
        x9 = j9;
        x10 = j10;
        x11 = j11;
        x12 = j12;
        x13 = j13;
        x14 = j14;
        x15 = j15;
        for (i = 20; i > 0; i -= 2)
        {
            QUARTERROUND(x0, x4, x8, x12)
            QUARTERROUND(x1, x5, x9, x13)
            QUARTERROUND(x2, x6, x10, x14)
            QUARTERROUND(x3, x7, x11, x15)
            QUARTERROUND(x0, x5, x10, x15)
            QUARTERROUND(x1, x6, x11, x12)
            QUARTERROUND(x2, x7, x8, x13)
            QUARTERROUND(x3, x4, x9, x14)
        }
        x0 = PLUS(x0, j0);
        x1 = PLUS(x1, j1);
        x2 = PLUS(x2, j2);
        x3 = PLUS(x3, j3);
        x4 = PLUS(x4, j4);
        x5 = PLUS(x5, j5);
        x6 = PLUS(x6, j6);
        x7 = PLUS(x7, j7);
        x8 = PLUS(x8, j8);
        x9 = PLUS(x9, j9);
        x10 = PLUS(x10, j10);
        x11 = PLUS(x11, j11);
        x12 = PLUS(x12, j12);
        x13 = PLUS(x13, j13);
        x14 = PLUS(x14, j14);
        x15 = PLUS(x15, j15);

        if (bytes < 64)
        {
            _private_tls_U32TO8_LITTLE(this->ks + 0, x0);
            _private_tls_U32TO8_LITTLE(this->ks + 4, x1);
            _private_tls_U32TO8_LITTLE(this->ks + 8, x2);
            _private_tls_U32TO8_LITTLE(this->ks + 12, x3);
            _private_tls_U32TO8_LITTLE(this->ks + 16, x4);
            _private_tls_U32TO8_LITTLE(this->ks + 20, x5);
            _private_tls_U32TO8_LITTLE(this->ks + 24, x6);
            _private_tls_U32TO8_LITTLE(this->ks + 28, x7);
            _private_tls_U32TO8_LITTLE(this->ks + 32, x8);
            _private_tls_U32TO8_LITTLE(this->ks + 36, x9);
            _private_tls_U32TO8_LITTLE(this->ks + 40, x10);
            _private_tls_U32TO8_LITTLE(this->ks + 44, x11);
            _private_tls_U32TO8_LITTLE(this->ks + 48, x12);
            _private_tls_U32TO8_LITTLE(this->ks + 52, x13);
            _private_tls_U32TO8_LITTLE(this->ks + 56, x14);
            _private_tls_U32TO8_LITTLE(this->ks + 60, x15);
        }

        x0 = XOR(x0, _private_tls_U8TO32_LITTLE(m + 0));
        x1 = XOR(x1, _private_tls_U8TO32_LITTLE(m + 4));
        x2 = XOR(x2, _private_tls_U8TO32_LITTLE(m + 8));
        x3 = XOR(x3, _private_tls_U8TO32_LITTLE(m + 12));
        x4 = XOR(x4, _private_tls_U8TO32_LITTLE(m + 16));
        x5 = XOR(x5, _private_tls_U8TO32_LITTLE(m + 20));
        x6 = XOR(x6, _private_tls_U8TO32_LITTLE(m + 24));
        x7 = XOR(x7, _private_tls_U8TO32_LITTLE(m + 28));
        x8 = XOR(x8, _private_tls_U8TO32_LITTLE(m + 32));
        x9 = XOR(x9, _private_tls_U8TO32_LITTLE(m + 36));
        x10 = XOR(x10, _private_tls_U8TO32_LITTLE(m + 40));
        x11 = XOR(x11, _private_tls_U8TO32_LITTLE(m + 44));
        x12 = XOR(x12, _private_tls_U8TO32_LITTLE(m + 48));
        x13 = XOR(x13, _private_tls_U8TO32_LITTLE(m + 52));
        x14 = XOR(x14, _private_tls_U8TO32_LITTLE(m + 56));
        x15 = XOR(x15, _private_tls_U8TO32_LITTLE(m + 60));

        j12 = PLUSONE(j12);
        if (!j12)
        {
            j13 = PLUSONE(j13);
            /*
             * Stopping at 2^70 bytes per nonce is the user's
             * responsibility.
             */
        }

        _private_tls_U32TO8_LITTLE(c + 0, x0);
        _private_tls_U32TO8_LITTLE(c + 4, x1);
        _private_tls_U32TO8_LITTLE(c + 8, x2);
        _private_tls_U32TO8_LITTLE(c + 12, x3);
        _private_tls_U32TO8_LITTLE(c + 16, x4);
        _private_tls_U32TO8_LITTLE(c + 20, x5);
        _private_tls_U32TO8_LITTLE(c + 24, x6);
        _private_tls_U32TO8_LITTLE(c + 28, x7);
        _private_tls_U32TO8_LITTLE(c + 32, x8);
        _private_tls_U32TO8_LITTLE(c + 36, x9);
        _private_tls_U32TO8_LITTLE(c + 40, x10);
        _private_tls_U32TO8_LITTLE(c + 44, x11);
        _private_tls_U32TO8_LITTLE(c + 48, x12);
        _private_tls_U32TO8_LITTLE(c + 52, x13);
        _private_tls_U32TO8_LITTLE(c + 56, x14);
        _private_tls_U32TO8_LITTLE(c + 60, x15);

        if (bytes <= 64)
        {
            if (bytes < 64)
            {
                for (i = 0; i < bytes; ++i)
                    ctarget[i] = c[i];
            }
            this->input[12] = j12;
            this->input[13] = j13;
            this->unused = 64 - bytes;
            return;
        }
        bytes -= 64;
        c += 64;
        m += 64;
    }
}

VOID ChaChaPoly1305::Block(Span<UCHAR> output)
{
    UINT32 i;
    UINT32 len = (UINT32)output.Size();
    PUCHAR c = output.Data();

    UINT32 state[16];
    for (i = 0; i < 16; i++)
        state[i] = this->input[i];
    for (i = 20; i > 0; i -= 2)
    {
        QUARTERROUND(state[0], state[4], state[8], state[12])
        QUARTERROUND(state[1], state[5], state[9], state[13])
        QUARTERROUND(state[2], state[6], state[10], state[14])
        QUARTERROUND(state[3], state[7], state[11], state[15])
        QUARTERROUND(state[0], state[5], state[10], state[15])
        QUARTERROUND(state[1], state[6], state[11], state[12])
        QUARTERROUND(state[2], state[7], state[8], state[13])
        QUARTERROUND(state[3], state[4], state[9], state[14])
    }

    for (i = 0; i < 16; i++)
        state[i] = PLUS(state[i], this->input[i]);

    for (i = 0; i < len; i += 4)
    {
        _private_tls_U32TO8_LITTLE(c + i, state[i / 4]);
    }
    this->input[12] = PLUSONE(this->input[12]);
}

Result<void, Error> ChaChaPoly1305::Poly1305Aead(Span<UCHAR> pt, Span<const UCHAR> aad, const UCHAR (&poly_key)[POLY1305_KEYLEN], Span<UCHAR> out)
{
    UCHAR zeropad[15];
    Memory::Zero(zeropad, sizeof(zeropad));

    UINT32 len = (UINT32)pt.Size();
    UINT32 counter = 1;
    this->IVSetup96BitNonce(nullptr, (PUCHAR)&counter);
    this->EncryptBytes(Span<const UINT8>(pt.Data(), len), Span<UINT8>(out.Data(), len));

    Poly1305 poly(poly_key);
    poly.Update(aad);
    INT32 rem = (INT32)aad.Size() % 16;
    if (rem)
        poly.Update(Span<const UCHAR>(zeropad, 16 - rem));
    poly.Update(Span<const UCHAR>(out.Data(), len));
    rem = (INT32)len % 16;
    if (rem)
        poly.Update(Span<const UCHAR>(zeropad, 16 - rem));
    UCHAR trail[16];
    Poly1305::U32TO8(trail, (UINT32)aad.Size());
    Memory::Zero(trail + 4, 4);
    Poly1305::U32TO8(trail + 8, len);
    Memory::Zero(trail + 12, 4);

    poly.Update(Span<const UCHAR>(trail));
    poly.Finish(out.Last<POLY1305_TAGLEN>());

    return Result<void, Error>::Ok();
}

Result<INT32, Error> ChaChaPoly1305::Poly1305Decode(Span<UCHAR> pt, Span<const UCHAR> aad, const UCHAR (&poly_key)[POLY1305_KEYLEN], Span<UCHAR> out)
{
    if (pt.Size() < POLY1305_TAGLEN)
        return Result<INT32, Error>::Err(Error::ChaCha20_DecodeFailed);

    UINT32 len = (UINT32)pt.Size() - POLY1305_TAGLEN;

    // Authenticate BEFORE decrypting (AEAD requirement)
    // poly_key is already computed by the caller; use it directly
    Poly1305 poly(poly_key);
    poly.Update(aad);
    UCHAR zeropad[15];
    Memory::Zero(zeropad, sizeof(zeropad));
    INT32 rem = (INT32)aad.Size() % 16;
    if (rem)
        poly.Update(Span<const UCHAR>(zeropad, 16 - rem));
    poly.Update(Span<const UCHAR>(pt.Data(), len));
    rem = (INT32)len % 16;
    if (rem)
        poly.Update(Span<const UCHAR>(zeropad, 16 - rem));
    UCHAR trail[16];
    Poly1305::U32TO8(&trail[0], (UINT32)aad.Size());
    Memory::Zero(trail + 4, 4);
    Poly1305::U32TO8(&trail[8], len);
    Memory::Zero(trail + 12, 4);

    UCHAR mac_tag[POLY1305_TAGLEN];
    poly.Update(Span<const UCHAR>(trail));
    poly.Finish(mac_tag);

    // Constant-time comparison to prevent timing oracle
    UINT8 diff = 0;
    for (UINT32 i = 0; i < POLY1305_TAGLEN; i++)
        diff |= mac_tag[i] ^ pt[len + i];

    if (diff != 0)
    {
        LOG_ERROR("ChaChaPoly1305::Poly1305Decode: Authentication tag mismatch");
        return Result<INT32, Error>::Err(Error::ChaCha20_DecodeFailed);
    }

    // Only decrypt after authentication succeeds
    this->EncryptBytes(Span<const UINT8>(pt.Data(), len), Span<UINT8>(out.Data(), len));

    return Result<INT32, Error>::Ok((INT32)len);
}

VOID ChaChaPoly1305::Poly1305Key(Span<UCHAR, POLY1305_KEYLEN> poly1305_key)
{
    UCHAR key[32];
    UCHAR nonce[12];
    this->Key(key);
    this->Nonce(nonce);
    (void)Poly1305::GenerateKey(key, nonce, poly1305_key, 0);
}

ChaChaPoly1305::ChaChaPoly1305()
{
    Memory::Zero(&this->input, sizeof(this->input));
    Memory::Zero(&this->ks, sizeof(this->ks));
    this->unused = 0;
}

ChaChaPoly1305::~ChaChaPoly1305()
{
    Memory::Zero(&this->input, sizeof(this->input));
    Memory::Zero(&this->ks, sizeof(this->ks));
    this->unused = 0;
}

ChaChaPoly1305::ChaChaPoly1305(ChaChaPoly1305 &&other) noexcept
{
    Memory::Copy(&this->input, &other.input, sizeof(this->input));
    Memory::Copy(&this->ks, &other.ks, sizeof(this->ks));
    this->unused = other.unused;
    Memory::Zero(&other.input, sizeof(other.input));
    Memory::Zero(&other.ks, sizeof(other.ks));
    other.unused = 0;
}

ChaChaPoly1305 &ChaChaPoly1305::operator=(ChaChaPoly1305 &&other) noexcept
{
    if (this != &other)
    {
        Memory::Copy(&this->input, &other.input, sizeof(this->input));
        Memory::Copy(&this->ks, &other.ks, sizeof(this->ks));
        this->unused = other.unused;
        Memory::Zero(&other.input, sizeof(other.input));
        Memory::Zero(&other.ks, sizeof(other.ks));
        other.unused = 0;
    }
    return *this;
}
