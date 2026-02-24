#pragma once

#include "platform.h"
#include "tls.h"

// HttpClient class for making HTTP requests, supporting both secure (HTTPS) and non-secure (HTTP) connections
class HttpClient
{
private:
    CHAR hostName[254];  // RFC 1035: max 253 chars + null
    CHAR path[2048];     // De facto max URL path length
    IPAddress ipAddress;
    UINT16 port;
    TLSClient tlsContext;

public:
    VOID *operator new(USIZE) = delete;
    VOID operator delete(VOID *) = delete;
    // Constructors for HttpClient class, allowing initialization with a URL and optional IP address
    HttpClient(PCCHAR url, PCCHAR ipAddress);
    HttpClient(PCCHAR url);
    ~HttpClient() { if (IsValid()) Close(); }

    HttpClient(const HttpClient &) = delete;
    HttpClient &operator=(const HttpClient &) = delete;
    HttpClient(HttpClient &&) = default;
    HttpClient &operator=(HttpClient &&) = default;

    BOOL IsValid() const { return tlsContext.IsValid(); }
    BOOL IsSecure() const { return tlsContext.IsSecure(); }
    // Operations with HttpClient
    BOOL Open();
    BOOL Close();
    SSIZE Read(PVOID buffer, UINT32 bufferLength);
    UINT32 Write(PCVOID buffer, UINT32 bufferLength);

    BOOL SendGetRequest();
    BOOL SendPostRequest(PCVOID data, UINT32 dataLength);
    // Static method to parse a URL into its components (host, path, port, secure) and validate the format
    static BOOL ParseUrl(PCCHAR url, PCHAR host, PCHAR path, UINT16 &port, BOOL &secure);
};
