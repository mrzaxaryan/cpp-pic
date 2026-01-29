#include "websocket.h"
#include "memory.h"
#include "network.h"
#include "string.h"
#include "base64.h"
#include "random.h"
#include "logger.h"
#include "dns.h"
#include "http.h"
#include "embedded_string.h"

// HTTP header terminator: "\r\n\r\n"
static constexpr UINT32 HTTP_HEADER_END = 0x0A0D0A0D;
// HTTP 101 status: " 101" at offset 8 in "HTTP/1.1 101"
static constexpr UINT32 HTTP_101_STATUS = 0x31303120;

BOOL WebSocketClient::FormatterCallback(PVOID context, CHAR ch)
{
    WebSocketClient *client = (WebSocketClient *)context;
    if (client->isSecure)
        return client->tlsContext.Write(&ch, 1);
    return client->socketContext.Write(&ch, 1);
}

BOOL WebSocketClient::Open()
{
    LOG_DEBUG("Opening WebSocket client to %s:%u%s (secure: %s)", hostName, port, path, isSecure ? "true"_embed : "false"_embed);

    if (isSecure)
    {
        if (!tlsContext.Open())
        {
            LOG_DEBUG("Failed to open TLS transport");
            return FALSE;
        }
    }
    else
    {
        if (!socketContext.Open())
        {
            LOG_DEBUG("Failed to open socket transport");
            return FALSE;
        }
    }

    // Generate WebSocket key (RFC 6455: base64 of 16 random bytes)
    UINT8 key[16];
    Random random;
    random.GetArray(16, key);

    PCHAR secureKey = new CHAR[Base64::GetEncodeOutSize(16)];
    Base64::Encode((PCCHAR)key, 16, secureKey);

    // Send WebSocket upgrade request
    auto callback = EMBED_FUNC(FormatterCallback);
    StringFormatter::Format<CHAR>(callback, this,
        "GET %s HTTP/1.1\r\nHost: %s\r\nUpgrade: websocket\r\nConnection: Upgrade\r\nSec-WebSocket-Key: %s\r\nSec-WebSocket-Version: 13\r\n"_embed,
        path, hostName, secureKey);
    delete[] secureKey;

    if (isSecure)
        StringFormatter::Format<CHAR>(callback, this, "Origin: https://%s\r\n\r\n"_embed, hostName);
    else
        StringFormatter::Format<CHAR>(callback, this, "Origin: http://%s\r\n\r\n"_embed, hostName);

    // Read handshake response
    constexpr UINT16 RESPONSE_BUFFER_SIZE = 4096;
    PCHAR response = new CHAR[RESPONSE_BUFFER_SIZE];
    UINT32 totalRead = 0;

    for (;;)
    {
        if (totalRead >= RESPONSE_BUFFER_SIZE)
        {
            delete[] response;
            Close();
            return FALSE;
        }

        UINT32 bytesRead = isSecure
            ? tlsContext.Read(response + totalRead, 1)
            : socketContext.Read(response + totalRead, 1);

        if (bytesRead == 0)
        {
            delete[] response;
            Close();
            return FALSE;
        }

        totalRead += bytesRead;

        // Check for "\r\n\r\n" header terminator
        if (totalRead >= 4 && *(PUINT32)(response + totalRead - 4) == HTTP_HEADER_END)
            break;
    }

    // Verify "HTTP/1.1 101" response
    BOOL success = (totalRead >= 12) && (*(PUINT32)(response + 8) == HTTP_101_STATUS);
    delete[] response;

    if (!success)
    {
        Close();
        return FALSE;
    }

    isConnected = TRUE;
    return TRUE;
}

BOOL WebSocketClient::Close()
{
    isConnected = FALSE;
    if (isSecure)
        tlsContext.Close();
    else
        socketContext.Close();
    LOG_DEBUG("WebSocket closed: %s:%u%s", hostName, port, path);
    return TRUE;
}

UINT32 WebSocketClient::Write(PCVOID buffer, UINT32 bufferLength, INT8 opcode)
{
    // Calculate header length: 2 base + 4 mask + extended length (0, 2, or 8 bytes)
    UINT32 headerLength;
    if (bufferLength <= 125)
        headerLength = 6;
    else if (bufferLength <= 0xFFFF)
        headerLength = 8;
    else
        headerLength = 14;

    // Generate masking key
    UINT8 maskKey[4];
    Random random;
    random.GetArray(4, maskKey);

    // Build header
    PUINT8 header = new UINT8[headerLength];
    header[0] = 0x80 | opcode;  // FIN + opcode

    if (bufferLength <= 125)
    {
        header[1] = 0x80 | (UINT8)bufferLength;
        Memory::Copy(header + 2, maskKey, 4);
    }
    else if (bufferLength <= 0xFFFF)
    {
        header[1] = 0x80 | 126;
        UINT16 len16 = UINT16SwapByteOrder((UINT16)bufferLength);
        Memory::Copy(header + 2, &len16, 2);
        Memory::Copy(header + 4, maskKey, 4);
    }
    else
    {
        header[1] = 0x80 | 127;
        UINT64 len64 = UINT64SwapByteOrder((UINT64)bufferLength);
        Memory::Copy(header + 2, &len64, 8);
        Memory::Copy(header + 10, maskKey, 4);
    }

    // Build frame: header + masked payload
    UINT32 frameLength = headerLength + bufferLength;
    PCHAR frame = new CHAR[frameLength];
    Memory::Copy(frame, header, headerLength);
    delete[] header;

    if (buffer != NULL)
        Memory::Copy(frame + headerLength, buffer, bufferLength);

    // Apply mask to payload
    for (UINT32 i = 0; i < bufferLength; i++)
        frame[headerLength + i] ^= maskKey[i % 4];

    INT32 result = isSecure
        ? tlsContext.Write(frame, frameLength)
        : socketContext.Write(frame, frameLength);

    delete[] frame;
    return (result > 0) ? bufferLength : 0;
}

BOOL WebSocketClient::ReceiveExact(PVOID buffer, INT32 size)
{
    INT32 totalRead = 0;
    while (totalRead < size)
    {
        INT32 bytesRead = isSecure
            ? tlsContext.Read((PCHAR)buffer + totalRead, size - totalRead)
            : socketContext.Read((PCHAR)buffer + totalRead, size - totalRead);

        if (bytesRead <= 0)
            return FALSE;
        totalRead += bytesRead;
    }
    return TRUE;
}

BOOL WebSocketClient::ReceiveFrame(WebSocketFrame *frame)
{
    UINT8 header[2];
    if (!ReceiveExact(header, 2))
        return FALSE;

    // Parse header bytes
    frame->fin = (header[0] >> 7) & 1;
    frame->rsv1 = (header[0] >> 6) & 1;
    frame->rsv2 = (header[0] >> 5) & 1;
    frame->rsv3 = (header[0] >> 4) & 1;
    frame->opcode = header[0] & 0x0F;
    frame->mask = (header[1] >> 7) & 1;

    // Parse payload length
    UINT8 lengthBits = header[1] & 0x7F;
    if (lengthBits == 126)
    {
        UINT16 len16;
        if (!ReceiveExact(&len16, 2))
            return FALSE;
        frame->length = UINT16SwapByteOrder(len16);
    }
    else if (lengthBits == 127)
    {
        UINT64 len64;
        if (!ReceiveExact(&len64, 8))
            return FALSE;
        frame->length = UINT64SwapByteOrder(len64);
    }
    else
    {
        frame->length = lengthBits;
    }

    // Read mask key if present
    UINT32 maskKey = 0;
    if (frame->mask)
    {
        if (!ReceiveExact(&maskKey, 4))
            return FALSE;
    }

    // Read payload
    frame->data = NULL;
    if (frame->length > 0)
    {
        frame->data = new CHAR[(UINT32)frame->length];
        if (!ReceiveExact(frame->data, (INT32)frame->length))
        {
            delete[] frame->data;
            frame->data = NULL;
            return FALSE;
        }

        // Unmask payload if masked
        if (frame->mask)
        {
            PUINT8 mask = (PUINT8)&maskKey;
            for (UINT32 i = 0; i < frame->length; i++)
                frame->data[i] ^= mask[i % 4];
        }
    }

    return TRUE;
}

PVOID WebSocketClient::Read(PUSIZE outLength, PINT8 outOpcode)
{
    *outLength = 0;
    *outOpcode = 0;
    PVOID buffer = NULL;

    for (;;)
    {
        WebSocketFrame frame;
        Memory::Zero(&frame, sizeof(frame));

        if (!ReceiveFrame(&frame))
            break;

        switch (frame.opcode)
        {
        case WebSocketOpcode::TEXT:
        case WebSocketOpcode::BINARY:
        case WebSocketOpcode::CONTINUE:
        {
            // Continuation without initial frame is invalid
            if (frame.opcode == WebSocketOpcode::CONTINUE && buffer == NULL)
            {
                delete[] frame.data;
                return NULL;
            }

            if (frame.length == 0)
            {
                delete[] frame.data;
                return buffer;
            }

            // Append frame data to buffer
            if (buffer)
            {
                PCHAR newBuffer = new CHAR[*outLength + (UINT32)frame.length];
                Memory::Copy(newBuffer, buffer, *outLength);
                Memory::Copy(newBuffer + *outLength, frame.data, (UINT32)frame.length);
                delete[] (PCHAR)buffer;
                buffer = newBuffer;
                *outLength += (UINT32)frame.length;
            }
            else
            {
                buffer = new CHAR[(UINT32)frame.length];
                Memory::Copy(buffer, frame.data, (UINT32)frame.length);
                *outLength = (UINT32)frame.length;
            }
            delete[] frame.data;

            // Return if this is the final frame
            if (frame.fin)
            {
                *outOpcode = frame.opcode;
                return buffer;
            }
            break;
        }

        case WebSocketOpcode::CLOSE:
            delete[] frame.data;
            return NULL;

        case WebSocketOpcode::PING:
            Write(frame.data, frame.length, WebSocketOpcode::PONG);
            delete[] frame.data;
            break;

        case WebSocketOpcode::PONG:
            delete[] frame.data;
            break;

        default:
            delete[] frame.data;
            return buffer;
        }
    }

    return buffer;
}

WebSocketClient::WebSocketClient(PCCHAR url, PCCHAR ipAddressStr)
    : isConnected(FALSE), isSecure(FALSE), port(0)
{
    ipAddress = ConvertIP(ipAddressStr);
    HttpClient::ParseUrl(url, hostName, path, &port, &isSecure);

    if (isSecure)
        tlsContext = TLSClient(hostName, ipAddress, port);
    else
        socketContext = Socket(ipAddress, port);
}

WebSocketClient::WebSocketClient(PCCHAR url)
    : isConnected(FALSE), isSecure(FALSE), port(0)
{
    HttpClient::ParseUrl(url, hostName, path, &port, &isSecure);

    ipAddress = DNS::ResolveOverHttp(hostName);
    if (ipAddress == INVALID_IPV4)
    {
        LOG_ERROR("Failed to resolve hostname %s", hostName);
        return;
    }

    if (isSecure)
        tlsContext = TLSClient(hostName, ipAddress, port);
    else
        socketContext = Socket(ipAddress, port);
}
