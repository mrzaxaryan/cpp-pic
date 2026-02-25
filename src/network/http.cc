#include "http.h"
#include "dns.h"
#include "logger.h"

// Helper to append an embedded string to a buffer
static USIZE AppendStr(CHAR* buf, USIZE pos, USIZE maxPos, const CHAR* str) noexcept
{
    for (USIZE i = 0; str[i] != '\0' && pos < maxPos; i++)
    {
        buf[pos++] = str[i];
    }
    return pos;
}

/// @brief Parameterized constructor for HttpClient class
/// @param url The URL of the server to connect to
/// @param ipAddress The IP address of the server to connect to

HttpClient::HttpClient(PCCHAR url, PCCHAR ipAddress)
{
    BOOL isSecure = FALSE;
    if (!ParseUrl(url, hostName, path, port, isSecure))
    {
        return;
    }
    this->ipAddress = IPAddress::FromString(ipAddress);
    tlsContext = TLSClient(hostName, this->ipAddress, port, isSecure);
}

/// @brief Parameterized constructor for HttpClient class
/// @param url URL of the server to connect to (IP address will be resolved from the hostname)

HttpClient::HttpClient(PCCHAR url)
{
    BOOL isSecure = FALSE;
    if (!ParseUrl(url, hostName, path, port, isSecure))
    {
        return;
    }

    ipAddress = DNS::Resolve(hostName);

    if (!ipAddress.IsValid())
    {
        LOG_ERROR("Failed to resolve hostname %s", hostName);
        return;
    }
    tlsContext = TLSClient(hostName, ipAddress, port, isSecure);
}

/// @brief Open a connection to the server
/// @return Indicates whether the connection was opened successfully (TRUE) or if there was an error (FALSE)

BOOL HttpClient::Open()
{
    return tlsContext.Open();
}

/// @brief Closes the connection to the server and cleans up resources
/// @return Indicates whether the connection was closed successfully (TRUE) or if there was an error (FALSE)

BOOL HttpClient::Close()
{
    return tlsContext.Close();
}

/// @brief Read data from the server into the provided buffer, handling decryption if the connection is secure
/// @param buffer The buffer to store the read data
/// @param bufferLength The maximum number of bytes to read into the buffer
/// @return The number of bytes read from the server

SSIZE HttpClient::Read(PVOID buffer, UINT32 bufferLength)
{
    return tlsContext.Read(buffer, bufferLength);
}

/// @brief Write data to the server
/// @param buffer Pointer to the data to be sent to the server
/// @param bufferLength The length of the data to be sent in bytes
/// @return The number of bytes written to the server

UINT32 HttpClient::Write(PCVOID buffer, UINT32 bufferLength)
{
    return tlsContext.Write(buffer, bufferLength);
}

/// @brief Send an HTTP GET request to the server 
/// @return Indicates whether the GET request was sent successfully (TRUE) or if there was an error (FALSE)

BOOL HttpClient::SendGetRequest()
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

    UINT32 written = Write(request, (UINT32)pos);
    return written == pos;
}

/// @brief Send an HTTP POST request to the server
/// @param data The data to be sent in the body of the POST request
/// @param dataLength Length of the data to be sent in bytes
/// @return Indicates whether the POST request was sent successfully (TRUE) or if there was an error (FALSE)

BOOL HttpClient::SendPostRequest(PCVOID data, UINT32 dataLength)
{
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
    UINT32 written = Write(request, (UINT32)pos);
    if (written != pos)
    {
        return FALSE;
    }

    // Send body
    if (dataLength > 0)
    {
        written = Write(data, dataLength);
        return written == dataLength;
    }

    return TRUE;
}

/// @brief Parse a URL into its components (host, path, port, secure) and validate the format
/// @param url The URL to be parsed
/// @param host Reference to array to store the parsed host (RFC 1035: max 253 chars + null)
/// @param path Reference to array to store the parsed path (max 2048 chars)
/// @param port Reference to store the parsed port
/// @param secure Reference to store whether the connection is secure (TRUE) or not (FALSE)
/// @return Indicates whether the URL was parsed successfully (TRUE) or if there was an error (FALSE)

BOOL HttpClient::ParseUrl(PCCHAR url, CHAR (&host)[254], CHAR (&path)[2048], UINT16 &port, BOOL &secure)
{
    CHAR portBuffer[6];

    host[0] = '\0';
    path[0] = '\0';
    port = 0;
    secure = FALSE;

    UINT8 schemeLength = 0;
    if (String::StartsWith<CHAR>(url, "ws://"_embed))
    {
        secure = FALSE;
        schemeLength = 5; // ws://
    }
    else if (String::StartsWith<CHAR>(url, "wss://"_embed))
    {
        secure = TRUE;
        schemeLength = 6; // wss://
    }
    else if (String::StartsWith<CHAR>(url, "http://"_embed))
    {
        secure = FALSE;
        schemeLength = 7; // http://
    }
    else if (String::StartsWith<CHAR>(url, "https://"_embed))
    {
        secure = TRUE;
        schemeLength = 8; // https://
    }
    else
    {
        return FALSE;
    }

    PCCHAR pHostStart = url + schemeLength;

    PCCHAR pathStart = String::AddressOf('/', pHostStart);
    if (pathStart == NULL)
        pathStart = pHostStart + String::Length(pHostStart);

    PCCHAR portStart = String::AddressOf(':', pHostStart);
    if (portStart != NULL && portStart >= pathStart)
        portStart = NULL;

    if (portStart == NULL)
    {
        port = secure ? 443 : 80;

        USIZE hostLen = (USIZE)(pathStart - pHostStart);
        if (hostLen == 0 || hostLen > 253)
            return FALSE;

        Memory::Copy(host, pHostStart, hostLen);
        host[hostLen] = '\0';

        if (*pathStart == '\0')
        {
            path[0] = '/';
            path[1] = '\0';
        }
        else
        {
            USIZE pLen = (USIZE)String::Length(pathStart);
            if (pLen > 2047)
                return FALSE;
            Memory::Copy(path, pathStart, pLen);
            path[pLen] = '\0';
        }
    }
    else
    {
        USIZE hostLen = (USIZE)(portStart - pHostStart);
        if (hostLen == 0 || hostLen > 253)
            return FALSE;

        Memory::Copy(host, pHostStart, hostLen);
        host[hostLen] = '\0';

        USIZE portLen = (USIZE)(pathStart - (portStart + 1));
        if (portLen == 0 || portLen > 5)
            return FALSE;

        Memory::Copy(portBuffer, portStart + 1, portLen);
        portBuffer[portLen] = '\0';

        for (USIZE i = 0; i < portLen; i++)
            if (portBuffer[i] < '0' || portBuffer[i] > '9')
                return FALSE;

        INT64 pnum = String::ParseInt64(portBuffer);
        if (pnum == 0 || pnum > 65535)
            return FALSE;
        port = (UINT16)pnum;

        if (*pathStart == '\0')
        {
            path[0] = '/';
            path[1] = '\0';
        }
        else
        {
            USIZE pLen = (USIZE)String::Length(pathStart);
            if (pLen > 2047)
                return FALSE;
            Memory::Copy(path, pathStart, pLen);
            path[pLen] = '\0';
        }
    }

    return TRUE;
}
