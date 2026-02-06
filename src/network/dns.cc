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

#pragma pack(push, 1)
// Struct to represent a DNS answer
typedef struct
{
    UINT16 nameOffset;
    UINT16 type;
    UINT16 _class; // bummer! 'class' is the keyword in c++
    UINT32 ttl;
    UINT16 len;
} Answer;
#pragma pack(pop)

// Struct to represent a query
typedef struct
{
    PCHAR name;
    PDNS_REQUEST_QUESTION ques; // question
} Query;

#define isdigit(c) ((c) >= '0' && (c) <= '9')

// Helper: Check if host is localhost and return loopback address
static inline BOOL IsLocalhost(PCCHAR host, IPAddress *pResult, RequestType type)
{
    auto localhost = "localhost"_embed;
    if (String::Compare(host, (PCCHAR)localhost))
    {
        *pResult = IPAddress::LocalHost(type == AAAA);
        return TRUE;
    }
    return FALSE;
}

// Helper: Read HTTP response headers until \r\n\r\n
static BOOL ReadHttpHeaders(TLSClient *client, PCHAR buffer, UINT16 bufferSize, PUINT32 pBytesRead)
{
    UINT32 totalBytesRead = 0;
    for (;;)
    {
        if (totalBytesRead >= bufferSize)
        {
            LOG_WARNING("DNS response too large.");
            return FALSE;
        }
        UINT32 bytesRead = client->Read(buffer + totalBytesRead, 1);
        if (bytesRead == 0)
        {
            LOG_WARNING("Failed to read DNS response.");
            return FALSE;
        }
        totalBytesRead += bytesRead;
        // Check for \r\n\r\n (168626701 as UINT32)
        if (totalBytesRead >= 4 && *(PUINT32)(buffer + totalBytesRead - 4) == 168626701)
            break;
    }
    *pBytesRead = totalBytesRead;
    return TRUE;
}

// Helper: Validate HTTP 200 response
static inline BOOL ValidateHttp200(PCHAR response, UINT32 bytesRead)
{
    // Check for " 200" at offset 9 (540028978 as UINT32)
    return bytesRead >= 12 && *(PUINT32)(response + 9) == 540028978;
}

// Helper: Parse Content-Length header value
static INT64 ParseContentLength(PCHAR response)
{
    auto contentLengthHeader = "Content-Length: "_embed;
    USIZE headerSize = String::Length((PCCHAR)contentLengthHeader);
    USIZE offset = 0;
    while (Memory::Compare(response + offset, contentLengthHeader, headerSize) != 0)
        offset++;
    return String::ParseInt64(response + offset + headerSize);
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
        INT32 dotLen = *p; // Length of the next label
        if (dotLen < 0)
        { // Invalid length
            LOG_WARNING("DNS_skipName failed, invalid length");
            return -1;
        }

        if (dotLen == 0)
        { // If length is 0, we reached the end of the name
            LOG_DEBUG("DNS_skipName reached end of name");
            return (INT32)(p - ptr + 1);
        }
        p += dotLen + 1; // Move to the next label
    }
    LOG_WARNING("DNS_skipName failed, invalid pointer");
    return -1; // Invalid name
}

// Function to read the DNS name from the buffer
static PUINT8 DNS_readName(PUINT8 data, PUINT8 src, PINT32 size)
{
    // Check for NULL pointers
    LOG_DEBUG("DNS_readName(data: 0x%p, src: 0x%p, size: 0x%p) called", data, src, size);

    PUINT8 name;         // Pointer to hold the name
    UINT32 idx, offset;  // Index for the name
    BOOL bMoved = FALSE; // Flag to indicate if we have moved to another section of the answer

    *size = 1;               // Null terminator
    name = new UINT8[256];   // Allocate memory for the name
    Memory::Zero(name, 256); // Initialize the buffer

    idx = 0; // Index for the name
    // Loop through the data until we reach the null terminator
    while (*data != 0)
    {
        if (*data >= 192) // If the first byte is greater than 192, it indicates a pointer to another secion
        {
            offset = (*data) * 256 + *(data + 1) - 49152; // 49152 = 11000000 00000000
            data = src + offset - 1;                      // Jump to the new offset
            bMoved = TRUE;                                // We have jumped to another section of the answer
        }
        else
            name[idx++] = *data; // Copy the label to the name

        data++;        // Move to the next byte
        if (!bMoved)   // If we have not jumped to another section, we need to increment the size
            (*size)++; // Increment the size for each label
    }

    name[idx] = '\0'; // Null-terminate the name
    if (bMoved)       // if we have jumped to another section, we need to adjust the size
        (*size)++;

    INT32 i; // Index for the name
    // Loop to move the name to the left
    USIZE len = String::Length((const PCHAR)name);
    for (i = 0; i < (INT32)len; i++)
    {
        idx = name[i];
        // Move the name to the left
        for (INT32 j = 0; j < (INT32)idx; j++)
        {
            name[i] = name[i + 1];
            i++;
        }
        name[i] = '.'; // Add a dot after each label
    }
    name[i - 1] = '\0'; // Remove the last dot
    return name;        // Return the name
}

// This function parses the DNS answer section and extracts the IP address if the type is A (IPv4) or AAAA (IPv6).
static INT32 DNS_parseAnswer(PUINT8 ptr, PUINT8 buffer, INT32 cnt, IPAddress *pIpAddress)
{
    LOG_DEBUG("DNS_parseAnswer(ptr: 0x%p, buffer: 0x%p, cnt: %d, pIpAddress: 0x%p) called", ptr, buffer, cnt, pIpAddress);
    // Check for NULL pointers

    // UINT16 count = 0;
    INT32 len = 0; // Length of the answer section
    INT32 tmp = 0; // Temporary variable for reading names
    // Loop through the answers to parse them
    while (cnt > 0)
    {
        Answer a;             // Structure to hold the answer
        PUINT8 p = ptr + len; // Pointer to the current position in the answer section

        // https://tools.ietf.org/html/rfc1035#section-4.1.3
        a.nameOffset = ReadU16BE(p, 0);                              // Read the name offset
        a.type = ReadU16BE(p, 2);                                    // Read the type of the answer
        a._class = ReadU16BE(p, 4);                                  // Read the class of the answer
        a.ttl = (((UINT32)ReadU16BE(p, 6)) << 16) | ReadU16BE(p, 8); // Read the time-to-live (TTL) value
        a.len = ReadU16BE(p, 10);                                    // Read the length of the data in the answer

        INT32 nameOffset = a.nameOffset & 0x3fff;                    // Mask the name offset to get the actual offset
        PUINT8 tt = DNS_readName(buffer + nameOffset, buffer, &tmp); // Read the name from the buffer
        LOG_DEBUG("%s", tt);
        delete[] tt; // Free the memory allocated for the name

        // Check if the type is A (IPv4 address)
        if (a.type == A)
        {
            LOG_DEBUG("Processing A record with TTL: %d", a.ttl);
            PUINT32 ipv4Data = (PUINT32)(p + sizeof(Answer)); // Pointer to the IPv4 address
            *pIpAddress = IPAddress::FromIPv4(*ipv4Data);     // Create IPAddress from IPv4
            return 0;
        }
        // Check if the type is AAAA (IPv6 address)
        else if (a.type == AAAA)
        {
            LOG_DEBUG("Processing AAAA record with TTL: %d", a.ttl);
            PUINT8 ipv6Data = p + sizeof(Answer);        // Pointer to the IPv6 address (16 bytes)
            *pIpAddress = IPAddress::FromIPv6(ipv6Data); // Create IPAddress from IPv6
            return 0;
        }
        else
        {
            // const char* rr = "";
            //// in case of MX record, p + sizeof(Answer)+1 is MX priority, but I don't need it now, skip
            // if (a.type == MX)
            //     tt = DNS_readName(p + sizeof(Answer) + 2, buffer, &tmp);
            // else
            //     tt = DNS_readName(p + sizeof(Answer), buffer, &tmp);

            // if (a.type == CNAME)
            //     rr = "CNAME";
            // else if (a.type == NS)
            //     rr = "NS";
            // else if (a.type == TXT)
            //     rr = "TXT";
            // else if (a.type == MX)
            //     rr = "MX";
            // else if (a.type == PTR)
            //     rr = "PTR";
            // else
            //     rr = "???";

            // debug("\t%d\t%s\t%s\n", a.ttl, rr, tt);
            // free(tt);
        }

        len += sizeof(a) + a.len; // Update the length to point to the next answer
        cnt--;                    // Decrement the count of answers left to parse
    }

    return len; // Return the total length of the parsed answers
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

static BOOL DNS_parse(PUINT8 buffer, UINT16 len, IPAddress *pIpAddress)
{
    LOG_DEBUG("DNS_parse(buffer: 0x%p, len: %d, pIPAddress: 0x%p) called", buffer, len, pIpAddress);
    // Validate the input parameters
    if (!buffer || !len || len < sizeof(DNS_REQUEST_HEADER))
    {
        LOG_WARNING("Invalid parameters for DNS_parse");
        return FALSE;
    }

    DNS_REQUEST_HEADER dnsRequestHeader;                       // Structure to hold the DNS request header
    PDNS_REQUEST_HEADER pDNSRequestHeader = &dnsRequestHeader; // Pointer to the DNS request header

    pDNSRequestHeader->id = ReadU16BE(buffer, 0);       // Read the ID from the buffer
    UINT16 flags = ReadU16BE(buffer, 2);                // Read the flags from the buffer
    pDNSRequestHeader->qCount = ReadU16BE(buffer, 4);   // Read the question count from the buffer
    pDNSRequestHeader->ansCount = ReadU16BE(buffer, 6); // Read the answer count from the buffer

    if (!(flags & 0x8000))
    {
        LOG_WARNING("DNS_parse failed, flags indicate this is not a response");
        return FALSE; // this is not an answer
    }
    // should I check flags here before parsing data?
    if (pDNSRequestHeader->ansCount <= 0 || pDNSRequestHeader->ansCount > 20)
    {
        pDNSRequestHeader->ansCount = 0;
        LOG_WARNING("DNS_parse failed, invalid answer count: %d", pDNSRequestHeader->ansCount);
        return FALSE;
    }

    INT32 recordOffset = sizeof(DNS_REQUEST_HEADER); // Offset to the start of the records in the DNS request
    // If the question count is zero, we cannot parse the queries
    if (pDNSRequestHeader->qCount > 0)
    {
        // parse Queries. basically skip them
        INT32 size = DNS_parseQuery(buffer + recordOffset, pDNSRequestHeader->qCount);
        if (size <= 0)
        {
            LOG_WARNING("DNS_parse failed, invalid query size: %d", size);
            return FALSE;
        }
        recordOffset += size;
    }

    // Start parsing the answers
    if (pDNSRequestHeader->ansCount > 0)
        recordOffset += DNS_parseAnswer(buffer + recordOffset, buffer, pDNSRequestHeader->ansCount, pIpAddress);

    return TRUE;
    // the same with
    // Auth
    // Additional
}

// This function converts a hostname to DNS format, which is a sequence of labels separated by dots, with each label prefixed by its length.
static VOID toDnsFormat(PUINT8 dns, PCCHAR host)
{
    LOG_DEBUG("toDnsFormat(dns: 0x%p, host: %s) called", dns, host);
    if (!dns || !host) // Check for NULL pointers
    {
        LOG_WARNING("Invalid parameters for toDnsFormat");
        return;
    }

    // Validate the input parameters
    if (!host || !dns)
    {
        LOG_WARNING("toDnsFormat failed, host or dns is NULL");
        return;
    }

    UINT32 i, t = 0;                      // Temporary index for the current label
    USIZE hostLen = String::Length(host); // Length of the hostname
    for (i = 0; i < (UINT32)hostLen; i++)
    {
        if (host[i] == '.') // This indicates the end of a label
        {
            *dns++ = i - t; // Store the length of the label
            for (; t < i; t++)
                *dns++ = host[t]; // Copy the label to the DNS format
            t++;
        }
    }
    // Check if the last character is not a dot that indicates the end of the hostname
    if (host[String::Length(host) - 1] != '.')
    {
        *dns++ = i - t;
        for (; t < i; t++)
            *dns++ = host[t];
    }
    *dns++ = '\0'; // Null-terminate the DNS formatted string
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
    if (IsLocalhost(host, &result, dnstype))
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

    UINT8 buffer[0xff];
    UINT16 bufferSize = DNS_GenerateQuery(host, dnstype, (PCHAR)buffer, false);
    auto fixed = EMBED_FUNC(FormatterCallback);
    StringFormatter::Format<CHAR>(fixed, &tlsClient, (PCCHAR)format, DNSServerName, bufferSize);

    tlsClient.Write(buffer, bufferSize);

    UINT16 responseBufferSize = 4096;
    PCHAR dnsResponse = new CHAR[responseBufferSize];
    PCHAR dnsResponseStart = dnsResponse;
    UINT32 totalBytesRead = 0;

    if (!ReadHttpHeaders(&tlsClient, dnsResponse, responseBufferSize, &totalBytesRead))
    {
        delete[] dnsResponseStart;
        return IPAddress::Invalid();
    }

    LOG_DEBUG("DNS response received.\n\n%s", dnsResponse);
    if (!ValidateHttp200(dnsResponse, totalBytesRead))
    {
        delete[] dnsResponseStart;
        LOG_WARNING("Invalid handshake response.");
        return IPAddress::Invalid();
    }
    INT64 contentLength = ParseContentLength(dnsResponse);
    LOG_DEBUG("Content length: %d", contentLength);

    delete[] dnsResponseStart;

    CHAR binaryResponse[0xff];
    tlsClient.Read(binaryResponse, (UINT32)contentLength); // Move the pointer to the start of the binary response

    IPAddress ipAddress;

    if (!DNS_parse((PUINT8)binaryResponse, (UINT16)contentLength, &ipAddress))
    {
        tlsClient.Close();
        LOG_WARNING("Failed to parse DNS response");
        return IPAddress::Invalid();
    }

    tlsClient.Close();
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

IPAddress DNS::Resolve(PCCHAR host)
{
    LOG_DEBUG("DNS_resolve(host: %s) called - trying IPv6 first", host);

    // Try IPv6 (AAAA) first via Cloudflare, then Google
    IPAddress ip = CloudflareResolve(host, AAAA);
    if (ip.IsValid())
        return ip;

    ip = GoogleResolve(host, AAAA);
    if (ip.IsValid())
        return ip;

    // Fall back to IPv4 (A)
    LOG_DEBUG("IPv6 resolution failed, falling back to IPv4 (A) for %s", host);
    ip = CloudflareResolve(host, A);
    if (ip.IsValid())
        return ip;

    return GoogleResolve(host, A);
}