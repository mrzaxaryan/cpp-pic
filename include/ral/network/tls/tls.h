#pragma once

#include "pal.h"
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
    INT32 stateIndex;        // Current state index
    TlsBuffer sendBuffer;    // Send buffer
    TlsBuffer recvBuffer;    // Receive buffer
    TlsBuffer channelBuffer; // Channel buffer for received data
    INT32 channelBytesRead;  // Number of bytes read from channel buffer
    INT32 ReadChannel(PCHAR out, INT32 size);
    BOOL ProcessReceive();
    BOOL OnPacket(INT32 packetType, INT32 version, TlsBuffer *TlsReader);
    BOOL OnServerFinished();
    BOOL VerifyFinished(TlsBuffer *TlsReader);
    BOOL OnServerHelloDone();
    BOOL OnServerHello(TlsBuffer *TlsReader);
    BOOL SendChangeCipherSpec();
    BOOL SendClientExchange();
    BOOL SendClientFinished();
    BOOL SendClientHello(const CHAR *host);
    BOOL SendPacket(INT32 packetType, INT32 ver, TlsBuffer *TlsBuffer);

public:
    VOID *operator new(USIZE) = delete;
    VOID operator delete(VOID *) = delete;
    TLSClient() : host(nullptr), ip(), stateIndex(0), channelBytesRead(0) {}
    TLSClient(PCCHAR host, const IPAddress &ipAddress, UINT16 port);
    BOOL Open();
    BOOL Close();
    SSIZE Read(PVOID buffer, UINT32 bufferLength);
    UINT32 Write(PCVOID buffer, UINT32 bufferLength);
};