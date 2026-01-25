#include "primitives.h"
#pragma once

// HKDF (HMAC-based Key Derivation Function) for TLS 1.3
class TlsHKDF
{
private:
    static INT32 Label(const CHAR *label, UCHAR label_len, const UCHAR *data, UCHAR data_len, PUCHAR hkdflabel, UINT16 length);

public:
    static VOID Extract(PUCHAR output, UINT32 outlen, const UCHAR *salt, UINT32 salt_len, const UCHAR *ikm, UCHAR ikm_len);
    static VOID Expand(PUCHAR output, UINT32 outlen, const UCHAR *secret, UINT32 secret_len, const UCHAR *info, UCHAR info_len);
    static VOID ExpandLabel(PUCHAR output, UINT32 outlen, const UCHAR *secret, UINT32 secret_len, const CHAR *label, UCHAR label_len, const UCHAR *data, UCHAR data_len);
};