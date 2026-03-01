#include "http.h"
#include "dns.h"
#include "logger.h"
#include "embedded_string.h"

// Helper to append an embedded string to a buffer
static USIZE AppendStr(CHAR *buf, USIZE pos, USIZE maxPos, const CHAR *str) noexcept
{
    for (USIZE i = 0; str[i] != '\0' && pos < maxPos; i++)
    {
        buf[pos++] = str[i];
    }
    return pos;
}

/// @brief Factory method for HttpClient — creates from URL with explicit IP address
/// @param url The URL of the server to connect to
/// @param ipAddress The IP address of the server to connect to
/// @return Ok(HttpClient) on success, or Err with Http_CreateFailed on failure

Result<HttpClient, Error> HttpClient::Create(PCCHAR url, PCCHAR ipAddress)
{
    CHAR host[254];
    CHAR parsedPath[2048];
    UINT16 port;
    BOOL isSecure = false;
    auto parseResult = ParseUrl(url, host, parsedPath, port, isSecure);
    if (!parseResult)
        return Result<HttpClient, Error>::Err(Error::Http_CreateFailed);

    auto ipResult = IPAddress::FromString(ipAddress);
    if (!ipResult)
        return Result<HttpClient, Error>::Err(Error::Http_CreateFailed);
    auto tlsResult = TlsClient::Create(host, ipResult.Value(), port, isSecure);
    if (!tlsResult)
        return Result<HttpClient, Error>::Err(Error::Http_CreateFailed);

    HttpClient client(host, parsedPath, ipResult.Value(), port, static_cast<TlsClient &&>(tlsResult.Value()));
    return Result<HttpClient, Error>::Ok(static_cast<HttpClient &&>(client));
}

/// @brief Factory method for HttpClient — creates from URL with DNS resolution
/// @param url URL of the server to connect to (IP address will be resolved from the hostname)
/// @return Ok(HttpClient) on success, or Err with Http_CreateFailed on failure

Result<HttpClient, Error> HttpClient::Create(PCCHAR url)
{
    CHAR host[254];
    CHAR parsedPath[2048];
    UINT16 port;
    BOOL isSecure = false;
    auto parseResult = ParseUrl(url, host, parsedPath, port, isSecure);
    if (!parseResult)
        return Result<HttpClient, Error>::Err(Error::Http_CreateFailed);

    auto dnsResult = DNS::Resolve(host);
    if (!dnsResult)
    {
        LOG_ERROR("Failed to resolve hostname %s", host);
        return Result<HttpClient, Error>::Err(Error::Http_CreateFailed);
    }
    IPAddress ip = dnsResult.Value();

    auto tlsResult = TlsClient::Create(host, ip, port, isSecure);

    // IPv6 socket creation can fail on platforms without IPv6 support (e.g. UEFI)
    if (!tlsResult && ip.IsIPv6())
    {
        auto dnsResultV4 = DNS::Resolve(host, A);
        if (dnsResultV4)
        {
            ip = dnsResultV4.Value();
            tlsResult = TlsClient::Create(host, ip, port, isSecure);
        }
    }

    if (!tlsResult)
        return Result<HttpClient, Error>::Err(Error::Http_CreateFailed);

    HttpClient client(host, parsedPath, ip, port, static_cast<TlsClient &&>(tlsResult.Value()));
    return Result<HttpClient, Error>::Ok(static_cast<HttpClient &&>(client));
}

/// @brief Open a connection to the server
/// @return Ok on success, or Err with Http_OpenFailed on failure

Result<void, Error> HttpClient::Open()
{
    auto r = tlsContext.Open();
    if (!r)
        return Result<void, Error>::Err(r, Error::Http_OpenFailed);
    return Result<void, Error>::Ok();
}

/// @brief Closes the connection to the server and cleans up resources
/// @return Ok on success, or Err with Http_CloseFailed on failure

Result<void, Error> HttpClient::Close()
{
    auto r = tlsContext.Close();
    if (!r)
        return Result<void, Error>::Err(r, Error::Http_CloseFailed);
    return Result<void, Error>::Ok();
}

/// @brief Read data from the server into the provided buffer, handling decryption if the connection is secure
/// @param buffer The buffer to store the read data
/// @param bufferLength The maximum number of bytes to read into the buffer
/// @return Ok(bytesRead) on success, or Err with Http_ReadFailed on failure

Result<SSIZE, Error> HttpClient::Read(Span<CHAR> buffer)
{
    auto r = tlsContext.Read(buffer);
    if (!r)
        return Result<SSIZE, Error>::Err(r, Error::Http_ReadFailed);
    return Result<SSIZE, Error>::Ok(r.Value());
}

/// @brief Write data to the server
/// @param buffer Pointer to the data to be sent to the server
/// @param bufferLength The length of the data to be sent in bytes
/// @return Ok(bytesWritten) on success, or Err with Http_WriteFailed on failure

Result<UINT32, Error> HttpClient::Write(Span<const CHAR> buffer)
{
    auto r = tlsContext.Write(buffer);
    if (!r)
        return Result<UINT32, Error>::Err(r, Error::Http_WriteFailed);
    return Result<UINT32, Error>::Ok(r.Value());
}

/// @brief Send an HTTP GET request to the server
/// @return Ok on success, or Err with Http_SendGetFailed on failure

Result<void, Error> HttpClient::SendGetRequest()
{
    // Build GET request: "GET <path> HTTP/1.1\r\nHost: <host>\r\nConnection: close\r\n\r\n"
    CHAR request[2048];
    USIZE pos = 0;

    pos = AppendStr(request, pos, 2000, "GET "_embed);
    pos = AppendStr(request, pos, 2000, path);
    pos = AppendStr(request, pos, 2000, " HTTP/1.1\r\nHost: "_embed);
    pos = AppendStr(request, pos, 2000, hostName);
    pos = AppendStr(request, pos, 2000, "\r\nConnection: close\r\n\r\n"_embed);

    request[pos] = '\0';

    auto r = Write(Span<const CHAR>(request, (UINT32)pos));
    if (!r || r.Value() != pos)
        return Result<void, Error>::Err(r, Error::Http_SendGetFailed);
    return Result<void, Error>::Ok();
}

/// @brief Send an HTTP POST request to the server
/// @param data The data to be sent in the body of the POST request
/// @param dataLength Length of the data to be sent in bytes
/// @return Ok on success, or Err with Http_SendPostFailed on failure

Result<void, Error> HttpClient::SendPostRequest(Span<const CHAR> data)
{
    UINT32 dataLength = (UINT32)data.Size();
    // Build POST request with Content-Length
    CHAR request[2048];
    USIZE pos = 0;

    pos = AppendStr(request, pos, 1900, "POST "_embed);
    pos = AppendStr(request, pos, 1900, path);
    pos = AppendStr(request, pos, 1900, " HTTP/1.1\r\nHost: "_embed);
    pos = AppendStr(request, pos, 1900, hostName);
    pos = AppendStr(request, pos, 1900, "\r\nContent-Length: "_embed);

    // Convert dataLength to string
    CHAR lenStr[16];
    USIZE lenPos = 0;
    UINT32 tempLen = dataLength;
    if (tempLen == 0)
    {
        lenStr[lenPos++] = '0';
    }
    else
    {
        CHAR digits[16];
        USIZE digitCount = 0;
        while (tempLen > 0)
        {
            digits[digitCount++] = '0' + (tempLen % 10);
            tempLen /= 10;
        }
        // Reverse digits
        while (digitCount > 0)
        {
            lenStr[lenPos++] = digits[--digitCount];
        }
    }
    lenStr[lenPos] = '\0';

    pos = AppendStr(request, pos, 1900, lenStr);
    pos = AppendStr(request, pos, 1900, "\r\nConnection: close\r\n\r\n"_embed);

    request[pos] = '\0';

    // Send headers
    auto r = Write(Span<const CHAR>(request, (UINT32)pos));
    if (!r || r.Value() != pos)
    {
        return Result<void, Error>::Err(r, Error::Http_SendPostFailed);
    }

    // Send body
    if (dataLength > 0)
    {
        auto bodyResult = Write(data);
        if (!bodyResult || bodyResult.Value() != dataLength)
            return Result<void, Error>::Err(bodyResult, Error::Http_SendPostFailed);
    }

    return Result<void, Error>::Ok();
}

/// @brief Parse a URL into its components (host, path, port, secure) and validate the format
/// @param url The URL to be parsed
/// @param host Reference to array to store the parsed host (RFC 1035: max 253 chars + null)
/// @param path Reference to array to store the parsed path (max 2048 chars)
/// @param port Reference to store the parsed port
/// @param secure Reference to store whether the connection is secure (true) or not (false)
/// @return Ok on success, or Err with Http_ParseUrlFailed on failure

Result<void, Error> HttpClient::ParseUrl(PCCHAR url, CHAR (&host)[254], CHAR (&path)[2048], UINT16 &port, BOOL &secure)
{
    CHAR portBuffer[6];

    host[0] = '\0';
    path[0] = '\0';
    port = 0;
    secure = false;

    UINT8 schemeLength = 0;
    if (String::StartsWith<CHAR>(url, "ws://"_embed))
    {
        secure = false;
        schemeLength = 5; // ws://
    }
    else if (String::StartsWith<CHAR>(url, "wss://"_embed))
    {
        secure = true;
        schemeLength = 6; // wss://
    }
    else if (String::StartsWith<CHAR>(url, "http://"_embed))
    {
        secure = false;
        schemeLength = 7; // http://
    }
    else if (String::StartsWith<CHAR>(url, "https://"_embed))
    {
        secure = true;
        schemeLength = 8; // https://
    }
    else
    {
        return Result<void, Error>::Err(Error::Http_ParseUrlFailed);
    }

    PCCHAR pHostStart = url + schemeLength;

    PCCHAR pathStart = String::AddressOf('/', pHostStart);
    if (pathStart == nullptr)
        pathStart = pHostStart + String::Length(pHostStart);

    PCCHAR portStart = String::AddressOf(':', pHostStart);
    if (portStart != nullptr && portStart >= pathStart)
        portStart = nullptr;

    if (portStart == nullptr)
    {
        port = secure ? 443 : 80;

        USIZE hostLen = (USIZE)(pathStart - pHostStart);
        if (hostLen == 0 || hostLen > 253)
            return Result<void, Error>::Err(Error::Http_ParseUrlFailed);

        Memory::Copy(host, pHostStart, hostLen);
        host[hostLen] = '\0';
    }
    else
    {
        USIZE hostLen = (USIZE)(portStart - pHostStart);
        if (hostLen == 0 || hostLen > 253)
            return Result<void, Error>::Err(Error::Http_ParseUrlFailed);

        Memory::Copy(host, pHostStart, hostLen);
        host[hostLen] = '\0';

        USIZE portLen = (USIZE)(pathStart - (portStart + 1));
        if (portLen == 0 || portLen > 5)
            return Result<void, Error>::Err(Error::Http_ParseUrlFailed);

        Memory::Copy(portBuffer, portStart + 1, portLen);
        portBuffer[portLen] = '\0';

        for (USIZE i = 0; i < portLen; i++)
            if (portBuffer[i] < '0' || portBuffer[i] > '9')
                return Result<void, Error>::Err(Error::Http_ParseUrlFailed);

        INT64 pnum = String::ParseInt64(portBuffer);
        if (pnum == 0 || pnum > 65535)
            return Result<void, Error>::Err(Error::Http_ParseUrlFailed);
        port = (UINT16)pnum;
    }

    // Extract path (common to both branches)
    if (*pathStart == '\0')
    {
        path[0] = '/';
        path[1] = '\0';
    }
    else
    {
        USIZE pLen = (USIZE)String::Length(pathStart);
        if (pLen > 2047)
            return Result<void, Error>::Err(Error::Http_ParseUrlFailed);
        Memory::Copy(path, pathStart, pLen);
        path[pLen] = '\0';
    }

    return Result<void, Error>::Ok();
}

Result<INT64, Error> HttpClient::ReadResponseHeaders(TlsClient &client, UINT16 expectedStatus)
{
    // Compute expected "XYZ " pattern for the rolling window (big-endian byte order)
    UINT32 expectedTail =
        ((UINT32)('0' + expectedStatus / 100) << 24) |
        ((UINT32)('0' + (expectedStatus / 10) % 10) << 16) |
        ((UINT32)('0' + expectedStatus % 10) << 8) |
        0x20;

    UINT32 tail = 0;
    UINT32 bytesConsumed = 0;
    BOOL statusValid = false;
    INT64 contentLength = -1;

    // Content-Length state machine
    auto clHeader = "Content-Length: "_embed;
    UINT32 matchIndex = 0;
    BOOL parsingValue = false;
    BOOL atLineStart = true;

    for (;;)
    {
        CHAR c;
        auto readResult = client.Read(Span<CHAR>(&c, 1));
        if (!readResult || readResult.Value() <= 0)
            return Result<INT64, Error>::Err(readResult, Error::Http_ReadHeadersFailed_Read);

        tail = (tail << 8) | (UINT8)c;
        bytesConsumed++;

        if (bytesConsumed > 16384)
            return Result<INT64, Error>::Err(Error::Http_ReadHeadersFailed_Read);

        // After 13 bytes, tail holds bytes 9-12: check status code
        if (bytesConsumed == 13)
            statusValid = (tail == expectedTail);

        // Content-Length extraction state machine
        if (parsingValue)
        {
            if (c >= '0' && c <= '9')
            {
                if (contentLength > (INT64)0xFFFFFFFFFFFFF / 10)
                    parsingValue = false;
                else
                    contentLength = contentLength * 10 + (c - '0');
            }
            else
                parsingValue = false;
        }
        else if (atLineStart)
        {
            matchIndex = 0;
            if (c == ((PCCHAR)clHeader)[0])
                matchIndex = 1;
            atLineStart = false;
        }
        else if (matchIndex > 0 && matchIndex < 16)
        {
            if (c == ((PCCHAR)clHeader)[matchIndex])
            {
                matchIndex++;
                if (matchIndex == 16)
                {
                    parsingValue = true;
                    contentLength = 0;
                }
            }
            else
            {
                matchIndex = 0;
            }
        }

        if (c == '\n')
            atLineStart = true;

        // Check for \r\n\r\n end-of-headers (0x0D0A0D0A)
        if (tail == 0x0D0A0D0A)
            break;
    }

    if (!statusValid)
        return Result<INT64, Error>::Err(Error::Http_ReadHeadersFailed_Status);

    return Result<INT64, Error>::Ok(contentLength);
}
