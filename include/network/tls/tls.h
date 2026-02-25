#pragma once

#include "platform.h"
#include "tls_buffer.h"
#include "tls_cipher.h"

// TLS state
typedef struct _tlsstate
{
    INT32 contentType;   // TLS content type
    INT32 handshakeType; // TLS handshake type
} tlsstate;

class TLSClient final
{
private:
    PCCHAR host;
    IPAddress ip;
    Socket context;
    TlsCipher crypto;
    BOOL secure;             // Whether to use TLS handshake or plain TCP
    INT32 stateIndex;        // Current state index
    TlsBuffer sendBuffer;    // Send buffer
    TlsBuffer recvBuffer;    // Receive buffer
    TlsBuffer channelBuffer; // Channel buffer for received data
    INT32 channelBytesRead;  // Number of bytes read from channel buffer
    INT32 ReadChannel(PCHAR out, INT32 size);
    [[nodiscard]] BOOL ProcessReceive();
    [[nodiscard]] BOOL OnPacket(INT32 packetType, INT32 version, TlsBuffer &TlsReader);
    [[nodiscard]] BOOL OnServerFinished();
    [[nodiscard]] BOOL VerifyFinished(TlsBuffer &TlsReader);
    [[nodiscard]] BOOL OnServerHelloDone();
    [[nodiscard]] BOOL OnServerHello(TlsBuffer &TlsReader);
    [[nodiscard]] BOOL SendChangeCipherSpec();
    [[nodiscard]] BOOL SendClientExchange();
    [[nodiscard]] BOOL SendClientFinished();
    [[nodiscard]] BOOL SendClientHello(const CHAR *host);
    [[nodiscard]] BOOL SendPacket(INT32 packetType, INT32 ver, TlsBuffer &TlsBuffer);

public:
    VOID *operator new(USIZE) = delete;
    VOID operator delete(VOID *) = delete;
    TLSClient() : host(nullptr), ip(), secure(true), stateIndex(0), channelBytesRead(0) {}
    TLSClient(PCCHAR host, const IPAddress &ipAddress, UINT16 port, BOOL secure = true);
    ~TLSClient()
    {
        if (IsValid())
            (void)Close();
    }

    TLSClient(const TLSClient &) = delete;
    TLSClient &operator=(const TLSClient &) = delete;

    TLSClient(TLSClient &&other)
        : host(other.host), ip(other.ip), context(static_cast<Socket &&>(other.context)), crypto(static_cast<TlsCipher &&>(other.crypto)), secure(other.secure), stateIndex(other.stateIndex), sendBuffer(static_cast<TlsBuffer &&>(other.sendBuffer)), recvBuffer(static_cast<TlsBuffer &&>(other.recvBuffer)), channelBuffer(static_cast<TlsBuffer &&>(other.channelBuffer)), channelBytesRead(other.channelBytesRead)
    {
        other.host = nullptr;
        other.secure = true;
        other.stateIndex = 0;
        other.channelBytesRead = 0;
    }

    TLSClient &operator=(TLSClient &&other)
    {
        if (this != &other)
        {
            (void)Close();
            host = other.host;
            ip = other.ip;
            context = static_cast<Socket &&>(other.context);
            crypto = static_cast<TlsCipher &&>(other.crypto);
            secure = other.secure;
            stateIndex = other.stateIndex;
            sendBuffer = static_cast<TlsBuffer &&>(other.sendBuffer);
            recvBuffer = static_cast<TlsBuffer &&>(other.recvBuffer);
            channelBuffer = static_cast<TlsBuffer &&>(other.channelBuffer);
            channelBytesRead = other.channelBytesRead;
            other.host = nullptr;
            other.secure = true;
            other.stateIndex = 0;
            other.channelBytesRead = 0;
        }
        return *this;
    }

    BOOL IsValid() const { return context.IsValid(); }
    BOOL IsSecure() const { return secure; }
    [[nodiscard]] Result<void, NetworkError> Open();
    [[nodiscard]] Result<void, NetworkError> Close();
    [[nodiscard]] Result<SSIZE, NetworkError> Read(PVOID buffer, UINT32 bufferLength);
    [[nodiscard]] Result<UINT32, NetworkError> Write(PCVOID buffer, UINT32 bufferLength);
};
