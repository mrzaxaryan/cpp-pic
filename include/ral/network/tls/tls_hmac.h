#pragma once

#include "bal.h"
#include "sha2.h"

#define MAX_HASH_LEN 64
#define CIPHER_HASH_SIZE 32

// HMAC structure
class TlsHMAC
{
private:
    INT32 hashSize;
    HMAC_SHA256 ctx256;
    HMAC_SHA384 ctx384;

public:
    TlsHMAC(INT32 hashSize, const UCHAR *key, UINT32 keySize);
    VOID Update(const UCHAR *message, UINT32 message_len);
    VOID Done(PUCHAR mac, UINT32 macSize);
};