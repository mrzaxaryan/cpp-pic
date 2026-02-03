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

HttpClient::HttpClient(PCCHAR url, PCCHAR ipAddress)
{
    // Attempt to parse the URL to extract the host name, path, port, and security setting
    {
        if (!ParseUrl(url, hostName, path, &port, &isSecure))
        {
            return;
        }
        this->ipAddress = IPAddress::FromString(ipAddress);
        if (isSecure)
        {
            tlsContext = TLSClient(hostName, this->ipAddress, port);
        }
        else
        {
            socketContext = Socket(this->ipAddress, port);
        }
    }
}

HttpClient::HttpClient(PCCHAR url)
{
    // Attempt to parse the URL to extract the host name, path, port, and security setting
    if (!ParseUrl(url, hostName, path, &port, &isSecure))
    {
        // return FALSE;
    }

    // Buffer to hold the resolved IP address
    // Attempt to resolve the host name to an IP address (tries IPv6 first, falls back to IPv4)
    ipAddress = DNS::Resolve(hostName);

    if (!ipAddress.IsValid())
    {
        LOG_ERROR("Failed to resolve hostname %s", hostName);
        return;
    }
    if (isSecure)
    {
        tlsContext = TLSClient(hostName, ipAddress, port);
    }
    else
    {
        socketContext = Socket(ipAddress, port);
    }
}

HttpClient::~HttpClient()
{
    Close();
}

BOOL HttpClient::Open()
{
    if (isSecure)
    {
        return tlsContext.Open();
    }
    else
    {
        return socketContext.Open();
    }
}

BOOL HttpClient::Close()
{
    if (isSecure)
    {
        return tlsContext.Close();
    }
    else
    {
        return socketContext.Close();
    }
}

SSIZE HttpClient::Read(PVOID buffer, UINT32 bufferLength)
{
    if (isSecure)
    {
        return tlsContext.Read(buffer, bufferLength);
    }
    else
    {
        return socketContext.Read(buffer, bufferLength);
    }
}

UINT32 HttpClient::Write(PCVOID buffer, UINT32 bufferLength)
{
    if (isSecure)
    {
        return tlsContext.Write(buffer, bufferLength);
    }
    else
    {
        return socketContext.Write(buffer, bufferLength);
    }
}

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

BOOL HttpClient::ParseUrl(PCCHAR url, PCHAR host, PCHAR path, PUINT16 port, PBOOL secure)
{
    CHAR portBuffer[6];

    host[0] = '\0';
    path[0] = '\0';
    *port = 0;
    *secure = FALSE;

    UINT8 schemeLength = 0;
    if (String::StartsWith<CHAR>(url, "ws://"_embed))
    {
        *secure = FALSE;
        schemeLength = 5; // ws://
    }
    else if (String::StartsWith<CHAR>(url, "wss://"_embed))
    {
        *secure = TRUE;
        schemeLength = 6; // wss://
    }
    else if (String::StartsWith<CHAR>(url, "http://"_embed))
    {
        *secure = FALSE;
        schemeLength = 7; // http://
    }
    else if (String::StartsWith<CHAR>(url, "https://"_embed))
    {
        *secure = TRUE;
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
        *port = *secure ? 443 : 80;

        USIZE hostLen = (USIZE)(pathStart - pHostStart);
        if (hostLen == 0)
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
            Memory::Copy(path, pathStart, pLen);
            path[pLen] = '\0';
        }
    }
    else
    {
        USIZE hostLen = (USIZE)(portStart - pHostStart);
        if (hostLen == 0)
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

        INT64 pnum = ParseINT64(portBuffer);
        if (pnum == 0 || pnum > 65535)
            return FALSE;
        *port = (UINT16)pnum;

        if (*pathStart == '\0')
        {
            path[0] = '/';
            path[1] = '\0';
        }
        else
        {
            USIZE pLen = (USIZE)String::Length(pathStart);
            Memory::Copy(path, pathStart, pLen);
            path[pLen] = '\0';
        }
    }

    return TRUE;
}
