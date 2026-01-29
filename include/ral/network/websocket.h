#pragma once
#include "primitives.h"
#include "socket.h"
#include "tls.h"

namespace WebSocketOpcode
{
    constexpr INT8 CONTINUE = 0x0;
    constexpr INT8 TEXT = 0x1;
    constexpr INT8 BINARY = 0x2;
    constexpr INT8 CLOSE = 0x8;
    constexpr INT8 PING = 0x9;
    constexpr INT8 PONG = 0xA;
}

struct WebSocketFrame
{
    UINT8 fin;
    UINT8 rsv1;
    UINT8 rsv2;
    UINT8 rsv3;
    UINT8 mask;
    UINT8 opcode;
    PCHAR data;
    UINT64 length;
};

class WebSocketClient
{
private:
    static BOOL FormatterCallback(PVOID context, CHAR ch);

    BOOL isConnected;
    BOOL isSecure;
    CHAR hostName[256];
    CHAR path[512];
    IPv4 ipAddress;
    UINT16 port;

    TLSClient tlsContext;
    Socket socketContext;

    BOOL ReceiveExact(PVOID buffer, INT32 size);
    BOOL ReceiveFrame(WebSocketFrame *frame);

public:
    VOID *operator new(USIZE) = delete;
    VOID operator delete(VOID *) = delete;

    WebSocketClient(PCCHAR url);
    WebSocketClient(PCCHAR url, PCCHAR ipAddress);
    BOOL Open();
    BOOL Close();
    PVOID Read(PUSIZE outLength, PINT8 outOpcode);
    UINT32 Write(PCVOID buffer, UINT32 bufferLength, INT8 opcode = WebSocketOpcode::BINARY);
};
