#include "dns.h"
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

// Struct to represent a DNS request question
typedef struct DNS_REQUEST_QUESTION
{
    UINT16 qtype;  // type of the query
    UINT16 qclass; // class of the query
} DNS_REQUEST_QUESTION, *PDNS_REQUEST_QUESTION;

// Helper: Check if host is localhost and return loopback address
static inline BOOL IsLocalhost(PCCHAR host, IPAddress &result, RequestType type)
{
    auto localhost = "localhost"_embed;
    if (String::Compare(host, (PCCHAR)localhost))
    {
        result = IPAddress::LocalHost(type == AAAA);
        return TRUE;
    }
    return FALSE;
}

// Helper: Read HTTP response headers until \r\n\r\n
static BOOL ReadHttpHeaders(TLSClient &client, PCHAR buffer, UINT16 bufferSize, UINT32 &bytesRead)
{
    UINT32 totalBytesRead = 0;
    for (;;)
    {
        if (totalBytesRead >= bufferSize)
        {
            LOG_WARNING("DNS response too large.");
            return FALSE;
        }
        UINT32 read = client.Read(buffer + totalBytesRead, 1);
        if (read == 0)
        {
            LOG_WARNING("Failed to read DNS response.");
            return FALSE;
        }
        totalBytesRead += read;
        // Check for \r\n\r\n (168626701 as UINT32)
        if (totalBytesRead >= 4 && *(PUINT32)(buffer + totalBytesRead - 4) == 168626701)
            break;
    }
    bytesRead = totalBytesRead;
    return TRUE;
}

// Helper: Validate HTTP 200 response
static inline BOOL ValidateHttp200(PCHAR response, UINT32 bytesRead)
{
    // Check for " 200" at offset 9 (540028978 as UINT32), needs bytes 9-12 = 13 minimum
    return bytesRead >= 13 && *(PUINT32)(response + 9) == 540028978;
}

// Helper: Parse Content-Length header value
static INT64 ParseContentLength(PCHAR response, UINT32 responseLen)
{
    auto contentLengthHeader = "Content-Length: "_embed;
    USIZE headerSize = String::Length((PCCHAR)contentLengthHeader);
    for (USIZE offset = 0; offset + headerSize <= responseLen; offset++)
    {
        if (Memory::Compare(response + offset, contentLengthHeader, headerSize) == 0)
            return String::ParseInt64(response + offset + headerSize);
    }
    return -1; // Header not found
}

static inline UINT16 ReadU16BE(PCVOID buffer, USIZE index)
{
    const UINT8 *p = (const UINT8 *)buffer;
    return (UINT16)((p[index] << 8) | p[index + 1]);
}

// Function to skip the DNS name
static INT32 DNS_skipName(PUINT8 ptr)
{
    LOG_DEBUG("DNS_skipName (ptr: 0x%p) called", ptr);

    PUINT8 p = ptr; // Pointer to the start of the name
    // Loop through the name until we reach the null terminator
    while (p)
    {
        UINT8 label = *p;

        if (label == 0)
        { // If length is 0, we reached the end of the name
            LOG_DEBUG("DNS_skipName reached end of name");
            return (INT32)(p - ptr + 1);
        }

        if (label >= 0xC0)
        { // Compression pointer: 2 bytes, terminates the name
            LOG_DEBUG("DNS_skipName encountered compression pointer");
            return (INT32)(p - ptr + 2);
        }

        if (label > 63)
        { // Label length must be 0-63
            LOG_WARNING("DNS_skipName failed, invalid label length: %d", label);
            return -1;
        }

        p += label + 1; // Move to the next label
    }
    LOG_WARNING("DNS_skipName failed, invalid pointer");
    return -1; // Invalid name
}

// This function parses the DNS answer section and extracts the IP address if the type is A (IPv4) or AAAA (IPv6).
// Returns 1 on success (A/AAAA found), 0 if no address record found. Sets parsedLen to bytes consumed.
static BOOL DNS_parseAnswer(PUINT8 ptr, INT32 cnt, IPAddress &ipAddress, INT32 &parsedLen)
{
    LOG_DEBUG("DNS_parseAnswer(ptr: 0x%p, cnt: %d) called", ptr, cnt);

    INT32 len = 0; // Length of the answer section
    BOOL found = FALSE;
    // Loop through the answers to parse them
    while (cnt > 0)
    {
        PUINT8 p = ptr + len; // Pointer to the current position in the answer section

        // Skip the name field (variable length: inline labels or 2-byte compression pointer)
        INT32 nameLen = DNS_skipName(p);
        if (nameLen <= 0)
        {
            LOG_WARNING("DNS_parseAnswer: failed to skip answer name");
            break;
        }

        PUINT8 fixedFields = p + nameLen; // Point past the name to the fixed fields
        // Fixed fields: type(2) + class(2) + ttl(4) + rdlength(2) = 10 bytes
        UINT16 type = ReadU16BE(fixedFields, 0);
        [[maybe_unused]] UINT16 _class = ReadU16BE(fixedFields, 2);
        [[maybe_unused]] UINT32 ttl = (((UINT32)ReadU16BE(fixedFields, 4)) << 16) | ReadU16BE(fixedFields, 6);
        UINT16 rdlength = ReadU16BE(fixedFields, 8);

        PUINT8 rdata = fixedFields + 10; // RDATA starts after the 10-byte fixed fields

        // Check if the type is A (IPv4 address)
        if (type == A && rdlength == 4)
        {
            LOG_DEBUG("Processing A record with TTL: %d", ttl);
            ipAddress = IPAddress::FromIPv4(*(PUINT32)rdata);
            found = TRUE;
            len += nameLen + 10 + rdlength;
            break;
        }
        // Check if the type is AAAA (IPv6 address)
        else if (type == AAAA && rdlength == 16)
        {
            LOG_DEBUG("Processing AAAA record with TTL: %d", ttl);
            ipAddress = IPAddress::FromIPv6(rdata);
            found = TRUE;
            len += nameLen + 10 + rdlength;
            break;
        }

        len += nameLen + 10 + rdlength; // name + fixed fields + rdata
        cnt--;
    }

    parsedLen = len;
    return found;
}

// This function parses the DNS query section of the DNS request.
static INT32 DNS_parseQuery(PUINT8 ptr, INT32 cnt)
{
    LOG_DEBUG("DNS_parseQuery(ptr: 0x%p, cnt: %d) called", ptr, cnt);

    PUINT8 p = ptr;
    while (cnt > 0)
    {
        // https://tools.ietf.org/html/rfc1035#section-4.1.2
        // QNAME + QTYPE + QCLASS
        // skip the name for now. we know the owner of the request
        INT32 nameLen = DNS_skipName(p);
        // Check if the name length is valid
        if (nameLen <= 0)
        {
            LOG_WARNING("DNS_parseQuery failed, invalid name length");
            return -1;
        }
        p += nameLen + sizeof(DNS_REQUEST_QUESTION); // Move the pointer to the next question
        cnt--;                                       // Decrement the count of questions left to parse
    }
    LOG_DEBUG("DNS_parseQuery completed successfully, processed %d queries", cnt);
    return (p - ptr); // Return the total length of the parsed queries
}

// This function resolves a DNS request for a given host and type, and stores the resolved IP address in the provided buffer.

static BOOL DNS_parse(PUINT8 buffer, UINT16 len, IPAddress &ipAddress)
{
    LOG_DEBUG("DNS_parse(buffer: 0x%p, len: %d) called", buffer, len);

    if (!buffer || len < sizeof(DNS_REQUEST_HEADER))
    {
        LOG_WARNING("Invalid parameters for DNS_parse");
        return FALSE;
    }

    UINT16 flags = ReadU16BE(buffer, 2);
    if (!(flags & 0x8000))
    {
        LOG_WARNING("DNS_parse failed, not a response");
        return FALSE;
    }

    UINT16 qCount = ReadU16BE(buffer, 4);
    UINT16 ansCount = ReadU16BE(buffer, 6);

    if (ansCount == 0 || ansCount > 20)
    {
        LOG_WARNING("DNS_parse failed, invalid answer count: %d", ansCount);
        return FALSE;
    }

    INT32 recordOffset = sizeof(DNS_REQUEST_HEADER);

    if (qCount > 0)
    {
        INT32 size = DNS_parseQuery(buffer + recordOffset, qCount);
        if (size <= 0)
        {
            LOG_WARNING("DNS_parse failed, invalid query size: %d", size);
            return FALSE;
        }
        recordOffset += size;
    }

    INT32 parsedLen = 0;
    return DNS_parseAnswer(buffer + recordOffset, ansCount, ipAddress, parsedLen);
}

// This function converts a hostname to DNS format, which is a sequence of labels separated by dots, with each label prefixed by its length.
static VOID toDnsFormat(PUINT8 dns, PCCHAR host)
{
    LOG_DEBUG("toDnsFormat(dns: 0x%p, host: %s) called", dns, host);
    if (!dns || !host)
    {
        LOG_WARNING("Invalid parameters for toDnsFormat");
        return;
    }

    UINT32 i, t = 0;
    USIZE hostLen = String::Length(host);
    for (i = 0; i < (UINT32)hostLen; i++)
    {
        if (host[i] == '.')
        {
            *dns++ = i - t;
            for (; t < i; t++)
                *dns++ = host[t];
            t++;
        }
    }
    // Write the final label (if host doesn't end with '.')
    if (hostLen > 0 && host[hostLen - 1] != '.')
    {
        *dns++ = i - t;
        for (; t < i; t++)
            *dns++ = host[t];
    }
    *dns = '\0';
}
// This function generates a DNS query for a given hostname and request type, and fills the provided buffer with the query data.
static USIZE DNS_GenerateQuery(PCCHAR host, RequestType dnstype, PCHAR buffer, BOOL useLengthPrefix = TRUE)
{
    LOG_DEBUG("DNS_GenerateQuery(host: %s, dnstype: %d, buffer: 0x%p) called", host, dnstype, buffer);

    // 1. Determine the starting point.
    // If we use a length prefix (TCP), the header starts at index 2.
    // For DoH, the header starts at index 0.
    UINT32 offset = useLengthPrefix ? 2 : 0;
    PDNS_REQUEST_HEADER pDnsRequestHeader = (PDNS_REQUEST_HEADER)(buffer + offset);

    // Set Header fields
    pDnsRequestHeader->id = (UINT16)0x24a1;
    pDnsRequestHeader->qr = 0;
    pDnsRequestHeader->opcode = 0;
    pDnsRequestHeader->aa = 0;
    pDnsRequestHeader->tc = 0;
    pDnsRequestHeader->rd = 1;
    pDnsRequestHeader->ra = 0;
    pDnsRequestHeader->z = 0;
    pDnsRequestHeader->ad = 0;
    pDnsRequestHeader->cd = 0;
    pDnsRequestHeader->rcode = 0;

    pDnsRequestHeader->qCount = UINT16SwapByteOrder(1);
    pDnsRequestHeader->ansCount = 0;
    pDnsRequestHeader->authCount = 0;
    pDnsRequestHeader->addCount = 0;

    // 2. Point qname immediately after the 12-byte header
    PCHAR qname = (PCHAR)pDnsRequestHeader + sizeof(DNS_REQUEST_HEADER);
    toDnsFormat((PUINT8)qname, host);

    // 3. Navigate to the end of the encoded name labels
    PCHAR pCurrent = qname;
    while (*pCurrent != 0)
    {
        pCurrent += (*pCurrent + 1);
    }
    pCurrent++; // Move past the final 0x00 null terminator

    // 4. Set the Question Footer (Type and Class)
    PDNS_REQUEST_QUESTION pDNSRequestQuestion = (PDNS_REQUEST_QUESTION)pCurrent;
    pDNSRequestQuestion->qclass = UINT16SwapByteOrder(1);
    pDNSRequestQuestion->qtype = UINT16SwapByteOrder(dnstype);

    // 5. Calculate the size of the DNS packet itself
    USIZE dnsPacketSize = (PCHAR)pDNSRequestQuestion + sizeof(DNS_REQUEST_QUESTION) - (PCHAR)pDnsRequestHeader;

    if (useLengthPrefix)
    {
        // For TCP, the first 2 bytes contain the length of the DNS message (Big Endian)
        UINT16 dnsPacketSizeBE = UINT16SwapByteOrder((UINT16)dnsPacketSize);
        Memory::Copy(buffer, &dnsPacketSizeBE, 2);

        // Total size returned is the packet + the 2-byte prefix
        return dnsPacketSize + 2;
    }

    // For DoH, return just the packet size
    return dnsPacketSize;
}

BOOL DNS::FormatterCallback(PVOID context, CHAR ch)
{
    TLSClient *tlsClient = (TLSClient *)context;
    return tlsClient->Write(&ch, 1);
}

IPAddress DNS::ResolveOverHttp(PCCHAR host, const IPAddress &DNSServerIp, PCCHAR DNSServerName, RequestType dnstype)
{
    IPAddress result;
    if (IsLocalhost(host, result, dnstype))
        return result;

    TLSClient tlsClient(DNSServerName, DNSServerIp, 443);
    if (!tlsClient.Open())
    {
        LOG_WARNING("Failed to connect to DNS server");
        return IPAddress::Invalid();
    }

    auto format = "POST /dns-query HTTP/1.1\r\n"_embed
                  "Host: %s\r\n"_embed
                  "Content-Type: application/dns-message\r\n"_embed
                  "Accept: application/dns-message\r\n"_embed
                  "Content-Length: %d\r\n"_embed
                  "\r\n"_embed;

    UINT8 queryBuffer[0xff];
    UINT16 querySize = DNS_GenerateQuery(host, dnstype, (PCHAR)queryBuffer, false);
    auto fixed = EMBED_FUNC(FormatterCallback);
    StringFormatter::Format<CHAR>(fixed, &tlsClient, (PCCHAR)format, DNSServerName, querySize);

    tlsClient.Write(queryBuffer, querySize);

    CHAR httpHeaders[512];
    UINT32 totalBytesRead = 0;

    if (!ReadHttpHeaders(tlsClient, httpHeaders, sizeof(httpHeaders), totalBytesRead))
        return IPAddress::Invalid();

    LOG_DEBUG("DNS response received.\n\n%s", httpHeaders);
    if (!ValidateHttp200(httpHeaders, totalBytesRead))
    {
        LOG_WARNING("Invalid handshake response.");
        return IPAddress::Invalid();
    }

    INT64 contentLength = ParseContentLength(httpHeaders, totalBytesRead);
    LOG_DEBUG("Content length: %d", contentLength);
    if (contentLength <= 0 || contentLength > 0xff)
    {
        LOG_WARNING("Invalid or missing Content-Length header.");
        return IPAddress::Invalid();
    }

    CHAR binaryResponse[0xff];
    tlsClient.Read(binaryResponse, (UINT32)contentLength);

    IPAddress ipAddress;
    if (!DNS_parse((PUINT8)binaryResponse, (UINT16)contentLength, ipAddress))
    {
        LOG_WARNING("Failed to parse DNS response");
        return IPAddress::Invalid();
    }

    return ipAddress;
}

IPAddress DNS::CloudflareResolve(PCCHAR host, RequestType dnstype)
{
    auto serverName = "one.one.one.one"_embed;
    IPAddress ip = ResolveOverHttp(host, IPAddress::FromIPv4(0x01010101), (PCCHAR)serverName, dnstype);
    if (!ip.IsValid())
        ip = ResolveOverHttp(host, IPAddress::FromIPv4(0x01000001), (PCCHAR)serverName, dnstype);
    return ip;
}

IPAddress DNS::GoogleResolve(PCCHAR host, RequestType dnstype)
{
    auto serverName = "dns.google"_embed;
    IPAddress ip = ResolveOverHttp(host, IPAddress::FromIPv4(0x08080808), (PCCHAR)serverName, dnstype);
    if (!ip.IsValid())
        ip = ResolveOverHttp(host, IPAddress::FromIPv4(0x04040808), (PCCHAR)serverName, dnstype);
    return ip;
}

IPAddress DNS::Resolve(PCCHAR host, RequestType dnstype)
{
    LOG_DEBUG("DNS_resolve(host: %s) called - trying IPv6 first", host);

    // Try IPv6 (AAAA) first via Cloudflare, then Google
    IPAddress ip = CloudflareResolve(host, dnstype);
    if (ip.IsValid())
        return ip;

    ip = GoogleResolve(host, dnstype);
    if (ip.IsValid())
        return ip;

    if (dnstype == AAAA)
    { // Fall back to IPv4 (A)
        LOG_DEBUG("IPv6 resolution failed, falling back to IPv4 (A) for %s", host);
        ip = CloudflareResolve(host, A);
        if (ip.IsValid())
            return ip;

        return GoogleResolve(host, A);
    }
    else
    {
        return ip; // Return the result (which may be invalid if both attempts failed)
    }
}