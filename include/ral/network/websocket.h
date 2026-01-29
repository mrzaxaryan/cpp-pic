#pragma once
#include "pal.h"
#include "tls.h"

#define OPCODE_CONTINUE 0x0
#define OPCODE_TEXT 0x1
#define OPCODE_BINARY 0x2
#define OPCODE_CLOSE 0x8
#define OPCODE_PING 0x9
#define OPCODE_PONG 0xA

typedef struct WebSocketFrame
{
    INT32 fin;
    INT32 rsv1;
    INT32 rsv2;
    INT32 rsv3;
    INT32 mask;
    INT32 opcode;
    PCHAR data;
    UINT64 length;
} WebSocketFrame, *PWebSocketFrame;

class WebSocketClient
{
private:
    static BOOL FormatterCallback(PVOID context, CHAR ch);
    BOOL isSecure;
    CHAR hostName[1024];
    CHAR path[1024];
    IPAddress ipAddress;
    UINT16 port;

    TLSClient tlsContext;
    Socket socketContext;

    BOOL isConnected;

    BOOL ReceiveRestrict(PVOID buffer, INT32 size);
    BOOL ReceiveFrame(PWebSocketFrame frame);

public:
    VOID *operator new(USIZE) = delete;
    VOID operator delete(VOID *) = delete;

    WebSocketClient(PCCHAR url);
    WebSocketClient(PCCHAR url, PCCHAR ipAddress);
    BOOL Open();
    BOOL Close();
    PVOID Read(PUSIZE dwBufferLength, PINT8 opcode);
    UINT32 Write(PCVOID buffer, UINT32 bufferLength, INT8 opcode = OPCODE_BINARY);
};
