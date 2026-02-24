#pragma once
#include "platform.h"
#include "tls.h"

enum WebSocketOpcode : INT8
{
    OPCODE_CONTINUE = 0x0,
    OPCODE_TEXT     = 0x1,
    OPCODE_BINARY   = 0x2,
    OPCODE_CLOSE    = 0x8,
    OPCODE_PING     = 0x9,
    OPCODE_PONG     = 0xA
};

struct WebSocketFrame
{
    PCHAR data;
    UINT64 length;
    INT8 opcode;
    UINT8 fin;
    UINT8 mask;
    UINT8 rsv1;
    UINT8 rsv2;
    UINT8 rsv3;
};

class WebSocketClient
{
private:
    static BOOL FormatterCallback(PVOID context, CHAR ch);
    CHAR hostName[254];  // RFC 1035: max 253 chars + null
    CHAR path[2048];     // De facto max URL path length
    IPAddress ipAddress;
    UINT16 port;
    TLSClient tlsContext;
    BOOL isConnected;

    BOOL ReceiveRestrict(PVOID buffer, UINT32 size);
    BOOL ReceiveFrame(WebSocketFrame &frame);

public:
    VOID *operator new(USIZE) = delete;
    VOID operator delete(VOID *) = delete;
    WebSocketClient(PCCHAR url);
    ~WebSocketClient() { if (IsValid()) Close(); }

    WebSocketClient(const WebSocketClient &) = delete;
    WebSocketClient &operator=(const WebSocketClient &) = delete;
    WebSocketClient(WebSocketClient &&) = default;
    WebSocketClient &operator=(WebSocketClient &&) = default;

    BOOL IsValid() const { return tlsContext.IsValid(); }
    BOOL IsSecure() const { return tlsContext.IsSecure(); }
    BOOL IsConnected() const { return isConnected; }
    BOOL Open();
    BOOL Close();
    PVOID Read(USIZE &dwBufferLength, INT8 &opcode);
    UINT32 Write(PCVOID buffer, UINT32 bufferLength, INT8 opcode = OPCODE_BINARY);
};
