#include "dns.h"
#include "http.h"
#include "logger.h"
#include "memory.h"
#include "string.h"
#include "tls.h"
#include "embedded_string.h"

// https://tools.ietf.org/html/rfc1035#section-4.1.1
// Note: z often is shown as of size 3, but it is actually separated into 3 fields of 1 bit each - ad, cd
typedef struct DNS_REQUEST_HEADER
{
    UINT16 id;        // identification number
    UCHAR rd : 1;     // recursion desired
    UCHAR tc : 1;     // truncated message
    UCHAR aa : 1;     // authoritive answer
    UCHAR opcode : 4; // purpose of message
    UCHAR qr : 1;     // query/response flag

    UCHAR rcode : 4; // response code
    UCHAR cd : 1;    // checking disabled
    UCHAR ad : 1;    // authenticated data
    UCHAR z : 1;     // reserved
    UCHAR ra : 1;    // recursion available

    UINT16 qCount;    // number of question entries
    UINT16 ansCount;  // number of answer entries
    UINT16 authCount; // number of authority entries
    UINT16 addCount;  // number of resource entries
} DNS_REQUEST_HEADER, *PDNS_REQUEST_HEADER;

typedef struct DNS_REQUEST_QUESTION
{
    UINT16 qtype;  // type of the query
    UINT16 qclass; // class of the query
} DNS_REQUEST_QUESTION, *PDNS_REQUEST_QUESTION;

static_assert(sizeof(DNS_REQUEST_HEADER) == 12, "DNS header must be 12 bytes (no padding)");
static_assert(sizeof(DNS_REQUEST_QUESTION) == 4, "DNS question must be 4 bytes (no padding)");

[[nodiscard]] FORCE_INLINE static BOOL IsLocalhost(PCCHAR host, IPAddress &result, RequestType type)
{
    auto localhost = "localhost"_embed;
    if (String::Compare(host, (PCCHAR)localhost))
    {
        result = IPAddress::LocalHost(type == AAAA);
        return true;
    }
    return false;
}

FORCE_INLINE static UINT16 ReadU16BE(const UINT8 *p, INT32 index)
{
    return (UINT16)((p[index] << 8) | p[index + 1]);
}

// Skip over a DNS name in wire format. maxLen is bytes remaining from ptr.
// Returns bytes consumed, or -1 on error.
[[nodiscard]] static INT32 SkipName(const UINT8 *ptr, INT32 maxLen)
{
    INT32 offset = 0;
    while (offset < maxLen)
    {
        UINT8 label = ptr[offset];

        if (label == 0)
            return offset + 1;

        if (label >= 0xC0)
        {
            if (offset + 2 > maxLen)
                return -1;
            return offset + 2;
        }

        if (label > 63)
        {
            LOG_WARNING("SkipName: invalid label length: %d", label);
            return -1;
        }

        offset += label + 1;
    }
    return -1;
}

// Parse DNS answer section, extract A/AAAA address.
// bufferLen is the total bytes available from ptr.
[[nodiscard]] static Result<void, Error> ParseAnswer(const UINT8 *ptr, INT32 cnt, INT32 bufferLen, IPAddress &ipAddress)
{
    // type(2) + class(2) + ttl(4) + rdlength(2)
    constexpr INT32 FIXED_FIELDS_SIZE = 10;

    INT32 len = 0;
    while (cnt > 0)
    {
        INT32 remaining = bufferLen - len;
        if (remaining <= 0)
            break;

        const UINT8 *p = ptr + len;

        INT32 nameLen = SkipName(p, remaining);
        if (nameLen <= 0)
        {
            LOG_WARNING("ParseAnswer: failed to skip answer name");
            break;
        }

        if (remaining - nameLen < FIXED_FIELDS_SIZE)
        {
            LOG_WARNING("ParseAnswer: truncated fixed fields");
            break;
        }

        const UINT8 *fixedFields = p + nameLen;
        UINT16 type = ReadU16BE(fixedFields, 0);
        UINT16 rdlength = ReadU16BE(fixedFields, 8);

        INT32 recordSize = nameLen + FIXED_FIELDS_SIZE + rdlength;
        if (remaining < recordSize)
        {
            LOG_WARNING("ParseAnswer: truncated rdata");
            break;
        }

        const UINT8 *rdata = fixedFields + FIXED_FIELDS_SIZE;

        if (type == A && rdlength == 4)
        {
            UINT32 ipv4;
            Memory::Copy(&ipv4, rdata, 4);
            ipAddress = IPAddress::FromIPv4(ipv4);
            return Result<void, Error>::Ok();
        }
        else if (type == AAAA && rdlength == 16)
        {
            ipAddress = IPAddress::FromIPv6(rdata);
            return Result<void, Error>::Ok();
        }

        len += recordSize;
        cnt--;
    }

    return Result<void, Error>::Err(Error::Dns_ParseFailed);
}

[[nodiscard]] static INT32 ParseQuery(const UINT8 *ptr, INT32 cnt, INT32 bufferLen)
{
    INT32 offset = 0;
    while (cnt > 0)
    {
        INT32 remaining = bufferLen - offset;
        if (remaining <= 0)
        {
            LOG_WARNING("ParseQuery: buffer exhausted");
            return -1;
        }

        INT32 nameLen = SkipName(ptr + offset, remaining);
        if (nameLen <= 0)
        {
            LOG_WARNING("ParseQuery: invalid name length");
            return -1;
        }

        INT32 entrySize = nameLen + (INT32)sizeof(DNS_REQUEST_QUESTION);
        if (remaining < entrySize)
        {
            LOG_WARNING("ParseQuery: truncated question entry");
            return -1;
        }

        offset += entrySize;
        cnt--;
    }
    return offset;
}

[[nodiscard]] static Result<void, Error> ParseDnsResponse(const UINT8 *buffer, INT32 len, IPAddress &ipAddress)
{
    if (!buffer || len < (INT32)sizeof(DNS_REQUEST_HEADER))
    {
        LOG_WARNING("ParseDnsResponse: invalid parameters");
        return Result<void, Error>::Err(Error::Dns_ParseFailed);
    }

    UINT16 flags = ReadU16BE(buffer, 2);
    if (!(flags & 0x8000))
    {
        LOG_WARNING("ParseDnsResponse: not a response");
        return Result<void, Error>::Err(Error::Dns_ParseFailed);
    }

    UINT8 rcode = flags & 0x000F;
    if (rcode != 0)
    {
        LOG_WARNING("ParseDnsResponse: server returned error (rcode=%d)", rcode);
        return Result<void, Error>::Err(Error::Dns_ParseFailed);
    }

    UINT16 qCount = ReadU16BE(buffer, 4);
    UINT16 ansCount = ReadU16BE(buffer, 6);

    if (ansCount == 0 || ansCount > 20)
    {
        LOG_WARNING("ParseDnsResponse: invalid answer count: %d", ansCount);
        return Result<void, Error>::Err(Error::Dns_ParseFailed);
    }

    if (qCount > 10)
    {
        LOG_WARNING("ParseDnsResponse: suspicious question count: %d", qCount);
        return Result<void, Error>::Err(Error::Dns_ParseFailed);
    }

    INT32 recordOffset = (INT32)sizeof(DNS_REQUEST_HEADER);

    if (qCount > 0)
    {
        INT32 size = ParseQuery(buffer + recordOffset, qCount, len - recordOffset);
        if (size <= 0)
        {
            LOG_WARNING("ParseDnsResponse: invalid query size: %d", size);
            return Result<void, Error>::Err(Error::Dns_ParseFailed);
        }
        recordOffset += size;
    }

    if (recordOffset >= len)
    {
        LOG_WARNING("ParseDnsResponse: no space for answer section");
        return Result<void, Error>::Err(Error::Dns_ParseFailed);
    }

    return ParseAnswer(buffer + recordOffset, ansCount, len - recordOffset, ipAddress);
}

// Convert hostname to DNS wire format (length-prefixed labels).
// Returns bytes written (including null terminator), or -1 on overflow.
[[nodiscard]] static INT32 FormatDnsName(UINT8 *dns, INT32 dnsSize, PCCHAR host)
{
    if (!dns || !host || dnsSize <= 0)
        return -1;

    UINT32 hostLen = (UINT32)String::Length(host);
    // Worst case: hostLen bytes + 1 extra label-length byte + null terminator
    if ((INT32)(hostLen + 2) > dnsSize)
        return -1;

    INT32 written = 0;
    UINT32 i, t = 0;
    for (i = 0; i < hostLen; i++)
    {
        if (host[i] == '.')
        {
            UINT32 labelLen = i - t;
            if (labelLen == 0)
                return -1; // empty label (leading dot or consecutive dots)
            if (labelLen > 63)
                return -1; // RFC 1035: label max 63 octets
            if (written + 1 + (INT32)labelLen >= dnsSize)
                return -1;
            dns[written++] = (UINT8)labelLen;
            for (; t < i; t++)
                dns[written++] = host[t];
            t++;
        }
    }
    if (hostLen > 0 && host[hostLen - 1] != '.')
    {
        UINT32 labelLen = i - t;
        if (labelLen == 0)
            return -1; // trailing empty segment (shouldn't happen, but guard)
        if (labelLen > 63)
            return -1; // RFC 1035: label max 63 octets
        if (written + 1 + (INT32)labelLen >= dnsSize)
            return -1;
        dns[written++] = (UINT8)labelLen;
        for (; t < i; t++)
            dns[written++] = host[t];
    }
    dns[written++] = '\0';
    return written;
}

// Generate a DNS-over-HTTPS query packet (no TCP length prefix).
// Returns query size in bytes, or 0 on error (buffer too small).
[[nodiscard]] static UINT32 GenerateQuery(PCCHAR host, RequestType dnstype, PCHAR buffer, UINT32 bufferSize)
{
    if (bufferSize < sizeof(DNS_REQUEST_HEADER) + sizeof(DNS_REQUEST_QUESTION) + 2)
        return 0;

    PDNS_REQUEST_HEADER pHeader = (PDNS_REQUEST_HEADER)buffer;

    pHeader->id = (UINT16)0x24a1;
    pHeader->qr = 0;
    pHeader->opcode = 0;
    pHeader->aa = 0;
    pHeader->tc = 0;
    pHeader->rd = 1;
    pHeader->ra = 0;
    pHeader->z = 0;
    pHeader->ad = 0;
    pHeader->cd = 0;
    pHeader->rcode = 0;
    pHeader->qCount = UINT16SwapByteOrder(1);
    pHeader->ansCount = 0;
    pHeader->authCount = 0;
    pHeader->addCount = 0;

    UINT8 *qname = (UINT8 *)buffer + sizeof(DNS_REQUEST_HEADER);
    INT32 nameSpaceLeft = (INT32)(bufferSize - sizeof(DNS_REQUEST_HEADER) - sizeof(DNS_REQUEST_QUESTION));
    INT32 nameLen = FormatDnsName(qname, nameSpaceLeft, host);
    if (nameLen <= 0)
    {
        LOG_WARNING("GenerateQuery: hostname too long for buffer");
        return 0;
    }

    PDNS_REQUEST_QUESTION pQuestion = (PDNS_REQUEST_QUESTION)(qname + nameLen);
    pQuestion->qclass = UINT16SwapByteOrder(1);
    pQuestion->qtype = UINT16SwapByteOrder(dnstype);

    return (UINT32)(sizeof(DNS_REQUEST_HEADER) + nameLen + sizeof(DNS_REQUEST_QUESTION));
}

Result<IPAddress, Error> DNS::ResolveOverHttp(PCCHAR host, const IPAddress &DNSServerIp, PCCHAR DNSServerName, RequestType dnstype)
{
    IPAddress localhostResult;
    if (IsLocalhost(host, localhostResult, dnstype))
        return Result<IPAddress, Error>::Ok(localhostResult);

    auto tlsResult = TLSClient::Create(DNSServerName, DNSServerIp, 443);
    if (!tlsResult)
    {
        LOG_WARNING("Failed to create TLS client for DNS server");
        return Result<IPAddress, Error>::Err(Error::Dns_ConnectFailed);
    }
    TLSClient &tlsClient = tlsResult.Value();

    if (!tlsClient.Open())
    {
        LOG_WARNING("Failed to connect to DNS server");
        return Result<IPAddress, Error>::Err(Error::Dns_ConnectFailed);
    }

    UINT8 queryBuffer[256];
    UINT32 querySize = GenerateQuery(host, dnstype, (PCHAR)queryBuffer, sizeof(queryBuffer));
    if (querySize == 0)
    {
        LOG_WARNING("Failed to generate DNS query");
        return Result<IPAddress, Error>::Err(Error::Dns_QueryFailed);
    }

    auto writeStr = [&tlsClient](PCCHAR s) -> Result<void, Error>
    {
        UINT32 len = String::Length(s);
        auto r = tlsClient.Write(Span<const CHAR>(s, len));
        if (!r || r.Value() != len)
            return Result<void, Error>::Err(Error::Dns_SendFailed);
        return Result<void, Error>::Ok();
    };

    CHAR sizeBuf[8];
    String::UIntToStr(querySize, sizeBuf, sizeof(sizeBuf));

    if (!writeStr("POST /dns-query HTTP/1.1\r\nHost: "_embed) ||
        !writeStr(DNSServerName) ||
        !writeStr("\r\nContent-Type: application/dns-message\r\nAccept: application/dns-message\r\nContent-Length: "_embed) ||
        !writeStr(sizeBuf) ||
        !writeStr("\r\n\r\n"_embed))
    {
        LOG_WARNING("Failed to send DNS query");
        return Result<IPAddress, Error>::Err(Error::Dns_SendFailed);
    }

    auto writeBody = tlsClient.Write(Span<const CHAR>((PCHAR)queryBuffer, querySize));
    if (!writeBody || writeBody.Value() != querySize)
    {
        LOG_WARNING("Failed to send DNS query");
        return Result<IPAddress, Error>::Err(Error::Dns_SendFailed);
    }

    auto headerResult = HttpClient::ReadResponseHeaders(tlsClient, 200);
    if (!headerResult)
    {
        LOG_WARNING("DNS server returned non-200 response");
        return Result<IPAddress, Error>::Err(headerResult, Error::Dns_ResponseFailed);
    }
    INT64 contentLength = headerResult.Value();

    if (contentLength <= 0 || contentLength > 512)
    {
        LOG_WARNING("Invalid or missing Content-Length header");
        return Result<IPAddress, Error>::Err(Error::Dns_ResponseFailed);
    }

    CHAR binaryResponse[512];
    UINT32 totalRead = 0;
    while (totalRead < (UINT32)contentLength)
    {
        auto readResult = tlsClient.Read(Span<CHAR>(binaryResponse + totalRead, (UINT32)contentLength - totalRead));
        if (!readResult || readResult.Value() <= 0)
        {
            LOG_WARNING("Failed to read DNS binary response");
            return Result<IPAddress, Error>::Err(Error::Dns_ResponseFailed);
        }
        totalRead += (UINT32)readResult.Value();
    }

    IPAddress ipAddress;
    if (!ParseDnsResponse((const UINT8 *)binaryResponse, (INT32)contentLength, ipAddress))
    {
        LOG_WARNING("Failed to parse DNS response");
        return Result<IPAddress, Error>::Err(Error::Dns_ParseFailed);
    }

    return Result<IPAddress, Error>::Ok(ipAddress);
}

Result<IPAddress, Error> DNS::CloudflareResolve(PCCHAR host, RequestType dnstype)
{
    auto serverName = "one.one.one.one"_embed;
    IPAddress ips[] = {IPAddress::FromIPv4(0x01010101), IPAddress::FromIPv4(0x01000001)};
    return ResolveWithFallback(host, ips, (PCCHAR)serverName, dnstype);
}

Result<IPAddress, Error> DNS::GoogleResolve(PCCHAR host, RequestType dnstype)
{
    auto serverName = "dns.google"_embed;
    IPAddress ips[] = {IPAddress::FromIPv4(0x08080808), IPAddress::FromIPv4(0x08080404)};
    return ResolveWithFallback(host, ips, (PCCHAR)serverName, dnstype);
}

Result<IPAddress, Error> DNS::Resolve(PCCHAR host, RequestType dnstype)
{
    LOG_DEBUG("Resolve(host: %s) called", host);

    auto result = CloudflareResolve(host, dnstype);
    if (!result)
        result = GoogleResolve(host, dnstype);

    if (!result && dnstype == AAAA)
    {
        LOG_DEBUG("IPv6 resolution failed, falling back to IPv4 (A) for %s", host);
        result = CloudflareResolve(host, A);
        if (!result)
            result = GoogleResolve(host, A);
    }

    return result;
}
