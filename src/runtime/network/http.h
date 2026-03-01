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
    TlsClient tlsContext;

    // Private constructor — only used by Create()
    HttpClient(const CHAR (&host)[254], const CHAR (&parsedPath)[2048], const IPAddress &ip, UINT16 portNum, TlsClient &&tls)
        : ipAddress(ip), port(portNum), tlsContext(static_cast<TlsClient &&>(tls))
    {
        Memory::Copy(hostName, host, sizeof(hostName));
        Memory::Copy(path, parsedPath, sizeof(path));
    }

public:
    VOID *operator new(USIZE) = delete;
    VOID operator delete(VOID *) = delete;
    // Placement new required by Result<HttpClient, Error>
    VOID *operator new(USIZE, PVOID ptr) noexcept { return ptr; }

    ~HttpClient()
    {
        if (IsValid())
            (void)Close();
    }

    HttpClient(const HttpClient &) = delete;
    HttpClient &operator=(const HttpClient &) = delete;

    HttpClient(HttpClient &&other) noexcept
        : ipAddress(other.ipAddress), port(other.port),
          tlsContext(static_cast<TlsClient &&>(other.tlsContext))
    {
        Memory::Copy(hostName, other.hostName, sizeof(hostName));
        Memory::Copy(path, other.path, sizeof(path));
        other.port = 0;
    }

    HttpClient &operator=(HttpClient &&other) noexcept
    {
        if (this != &other)
        {
            if (IsValid())
                (void)Close();
            Memory::Copy(hostName, other.hostName, sizeof(hostName));
            Memory::Copy(path, other.path, sizeof(path));
            ipAddress = other.ipAddress;
            port = other.port;
            tlsContext = static_cast<TlsClient &&>(other.tlsContext);
            other.port = 0;
        }
        return *this;
    }

    // Factory — caller MUST check the result (enforced by [[nodiscard]])
    [[nodiscard]] static Result<HttpClient, Error> Create(PCCHAR url);
    [[nodiscard]] static Result<HttpClient, Error> Create(PCCHAR url, PCCHAR ipAddress);

    BOOL IsValid() const { return tlsContext.IsValid(); }
    BOOL IsSecure() const { return tlsContext.IsSecure(); }
    // Operations with HttpClient
    [[nodiscard]] Result<void, Error> Open();
    [[nodiscard]] Result<void, Error> Close();
    [[nodiscard]] Result<SSIZE, Error> Read(Span<CHAR> buffer);
    [[nodiscard]] Result<UINT32, Error> Write(Span<const CHAR> buffer);

    [[nodiscard]] Result<void, Error> SendGetRequest();
    [[nodiscard]] Result<void, Error> SendPostRequest(Span<const CHAR> data);
    // Static method to parse a URL into its components (host, path, port, secure) and validate the format
    [[nodiscard]] static Result<void, Error> ParseUrl(PCCHAR url, CHAR (&host)[254], CHAR (&path)[2048], UINT16 &port, BOOL &secure);
    // Read HTTP response headers using a rolling window (no buffer needed).
    // Returns Ok(contentLength) if headers were read and status matches expectedStatus.
    // contentLength is the Content-Length value or -1 if not present.
    [[nodiscard]] static Result<INT64, Error> ReadResponseHeaders(TlsClient &client, UINT16 expectedStatus);
};
