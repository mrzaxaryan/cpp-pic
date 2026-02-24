#pragma once
#include "platform.h"
#include "tls.h"

#define OPCODE_CONTINUE 0x0
#define OPCODE_TEXT 0x1
#define OPCODE_BINARY 0x2
#define OPCODE_CLOSE 0x8
#define OPCODE_PING 0x9
#define OPCODE_PONG 0xA

// Structure that represents a Websocket frame - final, reserved bits, mask, opcode, payload data, and payload length
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

// WebSocketClient class for managing WebSocket connections, sending and receiving data, and handling WebSocket frames
class WebSocketClient
{
private:
    static BOOL FormatterCallback(PVOID context, CHAR ch);
    BOOL isSecure;          // The WebSocket connection is secure (wss) or not (ws)
    CHAR hostName[1024];    // Host name extracted from the URL
    CHAR path[1024];        // Path extracted from the URL
    IPAddress ipAddress;    // IP address of the WebSocket server
    UINT16 port;            // Port number of the WebSocket server

    TLSClient tlsContext;   // TLS client context for secure WebSocket connections
    Socket socketContext;   // Socket context for non-secure WebSocket connections

    BOOL isConnected;       // Indicates whether the WebSocket client is currently connected to the server

    BOOL ReceiveRestrict(PVOID buffer, INT32 size);
    BOOL ReceiveFrame(WebSocketFrame &frame);

public:
    // Disable dynamic memory allocation for WebSocketClient instances
    VOID *operator new(USIZE) = delete;
    VOID operator delete(VOID *) = delete;
    // Constructors for WebSocketClient class, allowing initialization with a URL and optional IP address
    WebSocketClient(PCCHAR url);
    WebSocketClient(PCCHAR url, PCCHAR ipAddress);
    // Open, Close, Read, and Write operations for WebSocketClient
    BOOL Open();
    BOOL Close();
    PVOID Read(USIZE &dwBufferLength, INT8 &opcode);
    UINT32 Write(PCVOID buffer, UINT32 bufferLength, INT8 opcode = OPCODE_BINARY);
};
