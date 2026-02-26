#pragma once

#include "platform.h"
#include "tls.h"

// HttpClient class for making HTTP requests, supporting both secure (HTTPS) and non-secure (HTTP) connections
class HttpClient
{
private:
    CHAR hostName[254]; // RFC 1035: max 253 chars + null
    CHAR path[2048];    // De facto max URL path length
    IPAddress ipAddress;
    UINT16 port;
    TLSClient tlsContext;

public:
    VOID *operator new(USIZE) = delete;
    VOID operator delete(VOID *) = delete;
    // Constructors for HttpClient class, allowing initialization with a URL and optional IP address
    HttpClient(PCCHAR url, PCCHAR ipAddress);
    HttpClient(PCCHAR url);
    ~HttpClient()
    {
        if (IsValid())
            (void)Close();
    }

    HttpClient(const HttpClient &) = delete;
    HttpClient &operator=(const HttpClient &) = delete;
    HttpClient(HttpClient &&) = delete;
    HttpClient &operator=(HttpClient &&) = delete;

    BOOL IsValid() const { return tlsContext.IsValid(); }
    BOOL IsSecure() const { return tlsContext.IsSecure(); }
    // Operations with HttpClient
    [[nodiscard]] Result<void, Error> Open();
    [[nodiscard]] Result<void, Error> Close();
    [[nodiscard]] Result<SSIZE, Error> Read(PVOID buffer, UINT32 bufferLength);
    [[nodiscard]] Result<UINT32, Error> Write(PCVOID buffer, UINT32 bufferLength);

    [[nodiscard]] Result<void, Error> SendGetRequest();
    [[nodiscard]] Result<void, Error> SendPostRequest(PCVOID data, UINT32 dataLength);
    // Static method to parse a URL into its components (host, path, port, secure) and validate the format
    [[nodiscard]] static Result<void, Error> ParseUrl(PCCHAR url, CHAR (&host)[254], CHAR (&path)[2048], UINT16 &port, BOOL &secure);
    // Read HTTP response headers using a rolling window (no buffer needed).
    // Returns Ok(contentLength) if headers were read and status matches expectedStatus.
    // contentLength is the Content-Length value or -1 if not present.
    [[nodiscard]] static Result<INT64, Error> ReadResponseHeaders(TLSClient &client, UINT16 expectedStatus);
};
