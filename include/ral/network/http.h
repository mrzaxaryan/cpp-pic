#pragma once

#include "primitives.h"
#include "string.h"
#include "memory.h"
#include "socket.h"
#include "tls.h"

class HttpClient
{
private:
    BOOL isSecure;
    CHAR hostName[1024];
    CHAR path[1024];
    IPv4 ipAddress;
    UINT16 port;
    TLSClient tlsContext;
    Socket socketContext;

public:
    HttpClient(PCCHAR url, PCCHAR ipAddress);
    HttpClient(PCCHAR url);

    BOOL Open();
    BOOL Close();
    SSIZE Read(PVOID buffer, UINT32 bufferLength);
    UINT32 Write(PCVOID buffer, UINT32 bufferLength);

    BOOL SendGetRequest();
    BOOL SendPostRequest(PCVOID data, UINT32 dataLength);

    ~HttpClient();

    static BOOL ParseUrl(PCCHAR url, PCHAR host, PCHAR path, PUINT16 port, PBOOL secure);
};

inline HttpClient::HttpClient(PCCHAR url, PCCHAR ipAddress)
{
    // Attempt to parse the URL to extract the host name, path, port, and security setting
    {
        if (!ParseUrl(url, hostName, path, &port, &isSecure))
        {
            return;
        }
        if (isSecure)
        {
            tlsContext = TLSClient(hostName, ConvertIP(ipAddress), port);
        }
        else
        {
            socketContext = Socket(ConvertIP(ipAddress), port);
        }
    }
}
inline HttpClient::HttpClient(PCCHAR url)
{
    // Attempt to parse the URL to extract the host name, path, port, and security setting
    if (!ParseUrl(url, hostName, path, &port, &isSecure))
    {
        // return FALSE;
    }

    // Buffer to hold the resolved IP address
    // Attempt to resolve the host name to an IP address
    ipAddress = DNS::ResolveOverHttp(hostName);

    if (ipAddress == INVALID_IPV4)
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

inline HttpClient::~HttpClient()
{
}

inline BOOL HttpClient::ParseUrl(PCCHAR url, PCHAR host, PCHAR path, PUINT16 port, PBOOL secure)
{
    CHAR portBuffer[6];

    host[0] = '\0';
    path[0] = '\0';
    *port = 0;
    *secure = FALSE;

    UINT8 schemeLength = 0;
    if (url[0] == 'w' && url[1] == 's' && url[2] == ':' && url[3] == '/' && url[4] == '/')
    {
        *secure = FALSE;
        schemeLength = 5; // ws://
    }
    else if (url[0] == 'w' && url[1] == 's' && url[2] == 's' && url[3] == ':' && url[4] == '/' && url[5] == '/')
    {
        *secure = TRUE;
        schemeLength = 6; // wss://
    }
    else if (url[0] == 'h' && url[1] == 't' && url[2] == 't' && url[3] == 'p' && url[4] == ':' && url[5] == '/' && url[6] == '/')
    {
        *secure = FALSE;
        schemeLength = 7; // http://
    }
    else if (url[0] == 'h' && url[1] == 't' && url[2] == 't' && url[3] == 'p' && url[4] == 's' && url[5] == ':' && url[6] == '/' && url[7] == '/')
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

        UINT32 pnum = (UINT32)String::ParseString<INT32>(portBuffer);
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
