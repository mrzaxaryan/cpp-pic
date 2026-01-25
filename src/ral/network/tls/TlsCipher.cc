#include "TlsCipher.h"
#include "logger.h"
#include "memory.h"
#include "random.h"
#include "TlsHMAC.h"
#include "TlsHKDF.h"
#include "network.h"

TlsCipher::TlsCipher()
{
    this->SetCipherCount(1);
    Memory::Zero(this->privateEccKeys, sizeof(this->privateEccKeys));
    Memory::Zero(&this->publicKey, sizeof(TlsBuffer));
    Memory::Zero(&this->decodeBuffer, sizeof(TlsBuffer));
    this->handshakeHash.Reset();
    this->Reset();
}

VOID TlsCipher::Reset()
{
    // PMEMORY pMemory = GetMemory();

    //	CLock lock(lockdata);
    this->publicKey.Clear();
    this->decodeBuffer.Clear();
    LOG_DEBUG("Resetting tls_cipher structure for cipher: %p", this);
    Memory::Zero(&this->data12, max(sizeof(this->data12), sizeof(this->data13)));
    this->clientSeqNum = 0;
    this->serverSeqNum = 0;
    this->handshakeHash.Reset();

    // cipher->hash
    this->handshakeHash.Reset();
    this->cipherIndex = -1;
    this->isEncoding = FALSE;
    // cipher->chacha20_ctx.Clear();
    for (INT32 i = 0; i < ECC_COUNT; i++)
        if (this->privateEccKeys[i])
        {
            LOG_DEBUG("Freeing ECC key: %p", this->privateEccKeys[i]);
            delete this->privateEccKeys[i];
            this->privateEccKeys[i] = nullptr;
        }
    LOG_DEBUG("Resetting private ECC keys to zero");
    Memory::Zero(this->privateEccKeys, sizeof(this->privateEccKeys));
}

VOID TlsCipher::Destroy()
{
    this->Reset();
    this->publicKey.Clear();
    this->decodeBuffer.Clear();
}

PINT8 TlsCipher::CreateClientRand()
{
    Random random;

    // random.SetSeed(0);

    LOG_DEBUG("Creating client random data for cipher: %p", this);
    for (UINT64 i = 0; i < (UINT64)sizeof(this->data12.clientRandom); i++)
    {

        this->data12.clientRandom[i] = random.Get() & 0xff;
    }
    LOG_DEBUG("Client random data created: %p", this->data12.clientRandom);
    return (PINT8)this->data12.clientRandom;
}

BOOL TlsCipher::UpdateServerInfo()
{
    this->cipherIndex = 0;

    return TRUE;
}

VOID TlsCipher::GetHash(const CHAR *out)
{
    //	CLock lock(lockdata);
    this->handshakeHash.GetHash((PCHAR)out, CIPHER_HASH_SIZE);
}
VOID TlsCipher::UpdateHash(const CHAR *in, UINT32 len)
{
    //	CLock lock(lockdata);
    this->handshakeHash.Append(in, len);
}

BOOL TlsCipher::ComputePublicKey(INT32 eccIndex, TlsBuffer *out)
{
    //	CLock lock(lockdata);

    if (this->privateEccKeys[eccIndex] == 0)
    {
        LOG_DEBUG("Allocating memory for private ECC key at index %d", eccIndex);
        this->privateEccKeys[eccIndex] = new Ecc();
        INT32 ecc_size_list[2];
        ecc_size_list[0] = 32;
        ecc_size_list[1] = 48;

        if (this->privateEccKeys[eccIndex]->Initialize(ecc_size_list[eccIndex]) != 0)
        {
            LOG_DEBUG("Failed to initialize ECC key at index %d", eccIndex);
            return FALSE;
        }
    }

    out->CheckSize(MAX_PUBKEY_SIZE);

    out->SetSize(out->GetSize() + this->privateEccKeys[eccIndex]->ExportPublicKey((UINT8 *)out->GetBuffer() + out->GetSize(), MAX_PUBKEY_SIZE));

    return TRUE;
}

BOOL TlsCipher::ComputePreKey(ECC_GROUP ecc, const CHAR *serverKey, INT32 serverKeyLen, TlsBuffer *premasterKey)
{
    INT32 eccIndex;
    INT32 eccSize;

    // Replace loop with two if statements
    if (ecc == ECC_secp256r1)
    {
        eccSize = 32;
        eccIndex = 0;
    }
    else if (ecc == ECC_secp384r1)
    {
        eccSize = 48;
        eccIndex = 1;
    }
    else
    {
        return FALSE;
    }
    if (!(this->ComputePublicKey(eccIndex, &this->publicKey)))
    {
        LOG_DEBUG("Failed to compute public key for ECC group %d", ecc);
        return FALSE;
    }

    premasterKey->SetSize(eccSize);

    if (this->privateEccKeys[eccIndex]->ComputeSharedSecret((UINT8 *)serverKey, serverKeyLen, (UINT8 *)premasterKey->GetBuffer()) != 0)
    {
        LOG_DEBUG("Failed to compute shared secret for ECC group %d", ecc);
        return FALSE;
    }

    return TRUE;
}

BOOL TlsCipher::ComputeKey(ECC_GROUP ecc, const CHAR *serverKey, INT32 serverKeyLen, PCHAR finishedHash)
{

    if (this->cipherIndex == -1)
    {
        LOG_DEBUG("Cipher index is -1, cannot compute TLS key");
        return FALSE;
    }
    LOG_DEBUG("Computing TLS key for cipher: %p, ECC group: %d", this, ecc);
    //	CLock lock(lockdata);

    INT32 keyLen = CIPHER_KEY_SIZE;
    INT32 hashLen = CIPHER_HASH_SIZE;

    UINT8 hash[MAX_HASH_LEN];
    UINT8 earlysecret[MAX_HASH_LEN], salt[MAX_HASH_LEN];
    UINT8 localKeyBuffer[MAX_KEY_SIZE], remoteKeyBuffer[MAX_KEY_SIZE];
    UINT8 localIvBuffer[MAX_IV_SIZE], remoteIvBuffer[MAX_IV_SIZE];
    // Declare _embed strings separately to avoid type deduction issues with ternary
    auto server_key_app = "s ap traffic"_embed;
    auto server_key_hs = "s hs traffic"_embed;
    auto client_key_app = "c ap traffic"_embed;
    auto client_key_hs = "c hs traffic"_embed;
    const CHAR *server_key = ecc == ECC_NONE ? (const CHAR *)server_key_app : (const CHAR *)server_key_hs;
    const CHAR *client_key = ecc == ECC_NONE ? (const CHAR *)client_key_app : (const CHAR *)client_key_hs;
    TlsHash hash2;
    hash2.GetHash((PCHAR)hash, hashLen);
    Memory::Zero(earlysecret, sizeof(earlysecret));

    if (ecc == ECC_NONE)
    {
        LOG_DEBUG("Using ECC_NONE for TLS key computation");

        TlsHKDF::ExpandLabel(salt, hashLen, (UINT8 *)this->data13.pseudoRandomKey, hashLen, "derived"_embed, 7, hash, hashLen);
        TlsHKDF::Extract(this->data13.pseudoRandomKey, hashLen, salt, hashLen, earlysecret, hashLen);

        if (finishedHash)
        {
            LOG_DEBUG("Using finished hash for TLS key computation with size: %d bytes", String::Length(finishedHash));
            Memory::Copy(hash, (VOID *)finishedHash, hashLen);
        }
    }
    else
    {
        TlsBuffer premaster_key;
        Memory::Zero(&premaster_key, sizeof(TlsBuffer));
        if (!this->ComputePreKey(ecc, serverKey, serverKeyLen, &premaster_key))
        {
            LOG_DEBUG("Failed to compute pre-master key for ECC group %d", ecc);
            return FALSE;
        }
        LOG_DEBUG("Computed pre-master key for ECC group %d, size: %d bytes", ecc, premaster_key.GetSize());

        UCHAR dummyLabel[1] = {0};
        UINT32 saltLen = 1;

        TlsHKDF::Extract(this->data13.pseudoRandomKey, hashLen, dummyLabel, saltLen, earlysecret, hashLen);
        TlsHKDF::ExpandLabel(salt, hashLen, this->data13.pseudoRandomKey, hashLen, "derived"_embed, 7, hash, hashLen);
        TlsHKDF::Extract(this->data13.pseudoRandomKey, hashLen, salt, hashLen, (UINT8 *)premaster_key.GetBuffer(), premaster_key.GetSize());

        this->GetHash((PCHAR)hash);

        premaster_key.Clear();
    }

    TlsHKDF::ExpandLabel(this->data13.handshakeSecret, hashLen, this->data13.pseudoRandomKey, hashLen, client_key, 12, hash, hashLen);

    TlsHKDF::ExpandLabel(localKeyBuffer, keyLen, this->data13.handshakeSecret, hashLen, "key"_embed, 3, NULL, 0);
    TlsHKDF::ExpandLabel(localIvBuffer, this->chacha20Context.GetIvLength(), this->data13.handshakeSecret, hashLen, "iv"_embed, 2, NULL, 0);

    TlsHKDF::ExpandLabel(this->data13.mainSecret, hashLen, this->data13.pseudoRandomKey, hashLen, server_key, 12, hash, hashLen);

    TlsHKDF::ExpandLabel(remoteKeyBuffer, keyLen, this->data13.mainSecret, hashLen, "key"_embed, 3, NULL, 0);
    TlsHKDF::ExpandLabel(remoteIvBuffer, this->chacha20Context.GetIvLength(), this->data13.mainSecret, hashLen, "iv"_embed, 2, NULL, 0);

    if (!this->chacha20Context.Initialize(localKeyBuffer, remoteKeyBuffer, localIvBuffer, remoteIvBuffer, keyLen))
    {
        LOG_DEBUG("Failed to initialize encoder with local key: %p, remote key: %p", localKeyBuffer, remoteKeyBuffer);
        return FALSE;
    }

    LOG_DEBUG("Encoder initialized with local key: %p, remote key: %p", localKeyBuffer, remoteKeyBuffer);
    return TRUE;
}

VOID TlsCipher::ComputeVerify(TlsBuffer *out, INT32 verifySize, INT32 localOrRemote)
{
    if (this->cipherIndex == -1)
    {
        LOG_DEBUG("tls_cipher_compute_verify: cipher_index is -1, cannot compute verify data");
        return;
    }
    //	CLock lock(lockdata);
    CHAR hash[MAX_HASH_LEN];
    INT32 hashLen = CIPHER_HASH_SIZE;
    LOG_DEBUG("tls_cipher_compute_verify: Getting handshake hash, hash_len=%d", hashLen);
    this->GetHash((PCHAR)hash);

    UINT8 finished_key[MAX_HASH_LEN];
    if (localOrRemote)
    {
        LOG_DEBUG("tls_cipher_compute_verify: Using server finished key");
        TlsHKDF::ExpandLabel(finished_key, hashLen, this->data13.mainSecret, hashLen, (CHAR[]){'f', 'i', 'n', 'i', 's', 'h', 'e', 'd', '\0'}, 8, NULL, 0);
    }
    else
    {
        LOG_DEBUG("tls_cipher_compute_verify: Using client finished key");
        TlsHKDF::ExpandLabel(finished_key, hashLen, this->data13.handshakeSecret, hashLen, (CHAR[]){'f', 'i', 'n', 'i', 's', 'h', 'e', 'd', '\0'}, 8, NULL, 0);
    }
    out->SetSize(verifySize);
    LOG_DEBUG("tls_cipher_compute_verify: Calculating HMAC for verify, verify_size=%d", verifySize);
    TlsHMAC hmac(CIPHER_HASH_SIZE, finished_key, hashLen);
    hmac.Update((UINT8 *)hash, hashLen);

    hmac.Done((UINT8 *)out->GetBuffer(), out->GetSize());
    LOG_DEBUG("tls_cipher_compute_verify: Finished verify computation");
}

VOID TlsCipher::Encode(TlsBuffer *sendbuf, const CHAR *packet, INT32 packetSize, BOOL keepOriginal)
{
    //	CLock lock(lockdata);
    if (!this->isEncoding || !this->chacha20Context.IsInitialized() || keepOriginal)
    {
        LOG_DEBUG("Encoding not enabled or encoder is NULL, appending packet directly to sendbuf");
        sendbuf->Append(packet, packetSize);
        return;
    }
    LOG_DEBUG("Encoding packet with size: %d bytes", packetSize);

    UCHAR aad[13];

    aad[0] = CONTENT_APPLICATION_DATA;
    aad[1] = sendbuf->GetBuffer()[1];
    aad[2] = sendbuf->GetBuffer()[2];
    *((UINT16 *)(aad + 3)) = UINT16SwapByteOrder(ChaCha20Encoder::ComputeSize(packetSize, 0)); //-header_size
    UINT64 clientSeq = UINT64SwapByteOrder(this->clientSeqNum++);
    Memory::Copy(aad + 5, &clientSeq, sizeof(UINT64));

    this->chacha20Context.Encode(sendbuf, packet, packetSize, aad, sizeof(aad));
}

BOOL TlsCipher::Decode(TlsBufferReader *inout, INT32 version)
{
    if (!this->isEncoding || !this->chacha20Context.IsInitialized())
    {
        LOG_DEBUG("Encoding not enabled or encoder is NULL, cannot Decode packet");
        return TRUE;
    }
    UCHAR aad[13];

    aad[0] = CONTENT_APPLICATION_DATA;
    aad[1] = UINT16SwapByteOrder(version) >> 8;
    aad[2] = UINT16SwapByteOrder(version) & 0xff;
    *((UINT16 *)(aad + 3)) = UINT16SwapByteOrder(inout->GetSize()); //-header_size
    UINT64 serverSeq = UINT64SwapByteOrder(this->serverSeqNum++);
    Memory::Copy(aad + 5, &serverSeq, sizeof(UINT64));

    BOOL ret = this->chacha20Context.Decode(inout, &this->decodeBuffer, aad, sizeof(aad));
    if (!ret)
    {
        LOG_ERROR("Decoding failed, returning error");
        return ret;
    }
    inout->SetBuffer(this->decodeBuffer.GetBuffer());
    inout->SetSize(this->decodeBuffer.GetSize());

    return TRUE;
}

VOID TlsCipher::SetEncoding(BOOL encoding)
{
    this->isEncoding = encoding;
}

VOID TlsCipher::ResetSequenceNumber()
{
    this->clientSeqNum = 0;
    this->serverSeqNum = 0;
}