#include "TlsHKDF.h"
#include "logger.h"
#include "memory.h"
#include "network.h"
#include "TlsHMAC.h"
#include "tlsBuffer.h"

// TlsHKDF class implementation
INT32 TlsHKDF::Label(const CHAR *label, UCHAR labelLen, const UCHAR *data, UCHAR dataLen, PUCHAR hkdflabel, UINT16 length)
{
    auto prefix = "tls13 "_embed;

    LOG_DEBUG("Creating HKDF label with label: %s, label_len: %d, data_len: %d, length: %d", label, labelLen, dataLen, length);
    *(PUINT16)hkdflabel = UINT16SwapByteOrder(length);
    INT32 prefix_len = prefix.Length;
    LOG_DEBUG("HKDF label prefix length: %d", prefix_len);
    Memory::Copy(&hkdflabel[3], (PCCHAR)prefix, prefix_len);

    hkdflabel[2] = (UCHAR)prefix_len + labelLen;
    Memory::Copy(&hkdflabel[3 + prefix_len], (PVOID)label, labelLen);
    hkdflabel[3 + prefix_len + labelLen] = (UCHAR)dataLen;
    if (dataLen)
    {
        LOG_DEBUG("Copying data to HKDF label, data_len: %d", dataLen);
        Memory::Copy(&hkdflabel[4 + prefix_len + labelLen], (PVOID)data, dataLen);
    }

    LOG_DEBUG("HKDF label created with total length: %d bytes", 4 + prefix_len + labelLen + dataLen);
    return 4 + prefix_len + labelLen + dataLen;
}

VOID TlsHKDF::Extract(PUCHAR output, UINT32 outlen, const UCHAR *salt, UINT32 saltLen, const UCHAR *ikm, UCHAR ikmLen)
{
    TlsHMAC hmac(32, salt, saltLen);

    LOG_DEBUG("Extracting HKDF with output length: %d, salt length: %d, ikm length: %d", outlen, saltLen, ikmLen);
    LOG_DEBUG("Salt: %p, IKM: %p", salt, ikm);
    hmac.Update(ikm, ikmLen);
    hmac.Done(output, outlen);
}

VOID TlsHKDF::Expand(PUCHAR output, UINT32 outlen, const UCHAR *secret, UINT32 secretLen, const UCHAR *info, UCHAR infoLen)
{
    UCHAR digestOut[MAX_HASH_LEN];
    UINT32 idx = 0;
    UCHAR i2 = 0;

    LOG_DEBUG("Expanding HKDF with output length: %d, secret length: %d, info length: %d", outlen, secretLen, infoLen);
    UINT32 hashLen = CIPHER_HASH_SIZE;
    while (outlen)
    {
        TlsHMAC hmac(hashLen, secret, secretLen);
        if (i2)
        {
            LOG_DEBUG("Using previous digest for HKDF expansion, i2: %d", i2);
            hmac.Update(digestOut, hashLen);
        }

        if ((info) && (infoLen))
        {
            LOG_DEBUG("Updating HMAC with info, info length: %d", infoLen);
            hmac.Update(info, infoLen);
        }
        i2++;
        hmac.Update(&i2, 1);
        hmac.Done(digestOut, hashLen);

        UINT32 copylen = outlen;
        if (copylen > hashLen)
        {
            LOG_DEBUG("Copying %d bytes from digestOut to output", hashLen);
            copylen = (UINT32)hashLen;
        }

        for (UINT32 i = 0; i < copylen; i++)
        {
            output[idx++] = digestOut[i];
            outlen--;
        }

        if (!outlen)
        {
            LOG_DEBUG("Finished HKDF expansion, no more output needed");
            break;
        }
    }
}

VOID TlsHKDF::ExpandLabel(PUCHAR output, UINT32 outlen, const UCHAR *secret, UINT32 secretLen, const CHAR *label, UCHAR labelLen, const UCHAR *data, UCHAR dataLen)
{
    UCHAR hkdf_label[512];
    INT32 len = TlsHKDF::Label(label, labelLen, data, dataLen, hkdf_label, outlen);
    LOG_DEBUG("Expanding HKDF label with output length: %d, secret length: %d, label length: %d, data length: %d", outlen, secretLen, labelLen, dataLen);
    TlsHKDF::Expand(output, outlen, secret, secretLen, hkdf_label, len);
}
