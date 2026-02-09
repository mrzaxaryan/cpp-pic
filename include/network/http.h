#pragma once

#include "platform.h"
#include "tls.h"

// HttpClient class for making HTTP requests, supporting both secure (HTTPS) and non-secure (HTTP) connections
class HttpClient
{
private:
    BOOL isSecure;
    CHAR hostName[1024];
    CHAR path[1024];
    IPAddress ipAddress;
    UINT16 port;
    TLSClient tlsContext;
    Socket socketContext;

public:
    // Constructors for HttpClient class, allowing initialization with a URL and optional IP address
    HttpClient(PCCHAR url, PCCHAR ipAddress);
    HttpClient(PCCHAR url);
    // Operations with HttpClient
    BOOL Open();
    BOOL Close();
    SSIZE Read(PVOID buffer, UINT32 bufferLength);
    UINT32 Write(PCVOID buffer, UINT32 bufferLength);

    BOOL SendGetRequest();
    BOOL SendPostRequest(PCVOID data, UINT32 dataLength);
    // Destructor to clean up resources
    ~HttpClient();
    // Static method to parse a URL into its components (host, path, port, secure) and validate the format
    static BOOL ParseUrl(PCCHAR url, PCHAR host, PCHAR path, PUINT16 port, PBOOL secure);
};
