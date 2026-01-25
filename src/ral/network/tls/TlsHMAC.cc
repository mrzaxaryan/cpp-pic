#include "TlsHMAC.h"
#include "logger.h"


TlsHMAC::TlsHMAC(INT32 hashSize, const UCHAR *key, UINT32 keySize)
{
    this->hashSize = hashSize;

    LOG_DEBUG("Initializing HMAC with hash size: %d bits", hashSize * 8);
    if (hashSize == 32)
    {
        this->ctx256.Init(key, keySize);
    }
    else
    {
        this->ctx384.Init(key, keySize);
    }
}

VOID TlsHMAC::Update(const UCHAR *message, UINT32 messageLen)
{
    if (this->hashSize == 32)
    {
        LOG_DEBUG("Updating HMAC with SHA256, message length: %d bytes", messageLen);
        this->ctx256.Update((PUCHAR)message, messageLen);
    }

    else
    {
        LOG_DEBUG("Updating HMAC with SHA384, message length: %d bytes", messageLen);
        this->ctx384.Update((PUCHAR)message, messageLen);
    }
}

VOID TlsHMAC::Done(PUCHAR mac, UINT32 macSize)
{
    if (this->hashSize == 32)
    {
        LOG_DEBUG("Finalizing HMAC with SHA256, mac size: %d bytes", macSize);
        this->ctx256.Final(mac, macSize);
    }

    else
    {
        LOG_DEBUG("Finalizing HMAC with SHA384, mac size: %d bytes", macSize);
        this->ctx384.Final(mac, macSize);
    }
}