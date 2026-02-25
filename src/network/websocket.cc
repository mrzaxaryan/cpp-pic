#include "websocket.h"
#include "memory.h"
#include "string.h"
#include "random.h"
#include "logger.h"
#include "dns.h"
#include "http.h"
#include "embedded_string.h"

BOOL WebSocketClient::Open()
{
    BOOL isSecure = tlsContext.IsSecure();
    LOG_DEBUG("Opening WebSocket client to %s:%u%s (secure: %s)", hostName, port, path, isSecure ? "true"_embed : "false"_embed);

    BOOL result = tlsContext.Open();

    if (!result && ipAddress.IsIPv6())
    {
        LOG_DEBUG("Failed to open network transport for WebSocket client using IPv6 address, attempting IPv4 fallback");

        // IPv6 failed, fall back to IPv4
        IPAddress ipv4Address = DNS::Resolve(hostName, A);
        if (!ipv4Address.IsValid())
        {
            LOG_ERROR("Failed to resolve IPv4 address for %s, cannot connect to WebSocket server", hostName);
            return FALSE;
        }

        ipAddress = ipv4Address;

        tlsContext.Close();
        tlsContext = TLSClient(hostName, ipAddress, port, isSecure);
        result = tlsContext.Open();
    }

    if (!result)
    {
        LOG_DEBUG("Failed to open network transport for WebSocket client");
        return FALSE;
    }

    // Generate random 16-byte WebSocket key (RFC 6455: 16 random bytes, base64-encoded)
    UINT32 key[4];
    Random random;
    for (INT32 i = 0; i < 4; i++)
        key[i] = (UINT32)random.Get();

    CHAR secureKey[25]; // Base64 of 16 bytes = 24 chars + null
    Base64::Encode((PCHAR)key, 16, secureKey);

    // Write the HTTP upgrade request directly to the TLS transport
    auto writeStr = [&](PCCHAR s) -> BOOL
    {
        UINT32 len = String::Length(s);
        return tlsContext.Write(s, len) == len;
    };

    if (!writeStr("GET "_embed) ||
        !writeStr(path) ||
        !writeStr(" HTTP/1.1\r\nHost: "_embed) ||
        !writeStr(hostName) ||
        !writeStr("\r\nUpgrade: WebSocket\r\nConnection: Upgrade\r\nSec-WebSocket-Key: "_embed) ||
        !writeStr(secureKey) ||
        !writeStr("\r\nSec-WebSocket-Version: 13\r\nOrigin: "_embed) ||
        !writeStr(isSecure ? "https://"_embed : "http://"_embed) ||
        !writeStr(hostName) ||
        !writeStr("\r\n\r\n"_embed))
    {
        Close();
        return FALSE;
    }

    INT64 contentLength = -1;
    if (!HttpClient::ReadResponseHeaders(tlsContext, 101, contentLength))
    {
        Close();
        return FALSE;
    }

    isConnected = TRUE;
    return TRUE;
}

BOOL WebSocketClient::Close()
{
    if (isConnected)
    {
        // Send a WebSocket CLOSE frame (status code 1000 = normal closure, big-endian)
        UINT8 closePayload[2];
        closePayload[0] = 0x03;
        closePayload[1] = 0xE8;
        Write(closePayload, sizeof(closePayload), OPCODE_CLOSE);
    }

    isConnected = FALSE;
    tlsContext.Close();
    LOG_DEBUG("WebSocket client to %s:%u%s closed", hostName, port, path);
    return TRUE;
}

UINT32 WebSocketClient::Write(PCVOID buffer, UINT32 bufferLength, WebSocketOpcode opcode)
{
    if (!isConnected && opcode != OPCODE_CLOSE)
        return 0;

    // Frame layout: [opcode+FIN][length+MASK][ext length?][mask key][payload]
    // Header sizes: <=125 -> 6, 126..65535 -> 8, >65535 -> 14
    UINT32 headerLength;
    if (bufferLength <= 125)
        headerLength = 6;
    else if (bufferLength <= 0xFFFF)
        headerLength = 8;
    else
        headerLength = 14;

    UINT32 frameLength = headerLength + bufferLength;

    // Stack buffer for small/medium frames, heap for larger
    UINT8 stackFrame[1024];
    PUINT8 frame = (frameLength <= sizeof(stackFrame)) ? stackFrame : new UINT8[frameLength];
    if (!frame)
        return 0;

    // FIN bit + opcode
    frame[0] = (UINT8)(opcode | 0x80);

    // Generate masking key from a single random value
    Random random;
    UINT32 maskKeyVal = (UINT32)random.Get();
    PUINT8 maskKey = (PUINT8)&maskKeyVal;

    // Encode payload length + mask bit
    if (bufferLength <= 125)
    {
        frame[1] = (UINT8)(bufferLength | 0x80);
        Memory::Copy(frame + 2, maskKey, 4);
    }
    else if (bufferLength <= 0xFFFF)
    {
        frame[1] = (126 | 0x80);
        UINT16 len16 = UINT16SwapByteOrder((UINT16)bufferLength);
        Memory::Copy(frame + 2, &len16, 2);
        Memory::Copy(frame + 4, maskKey, 4);
    }
    else
    {
        frame[1] = (127 | 0x80);
        UINT64 len64 = UINT64SwapByteOrder((UINT64)bufferLength);
        Memory::Copy(frame + 2, &len64, 8);
        Memory::Copy(frame + 10, maskKey, 4);
    }

    // Copy and mask payload in a single pass
    PUINT8 dst = frame + headerLength;
    PUINT8 src = (PUINT8)buffer;
    for (UINT32 i = 0; i < bufferLength; i++)
        dst[i] = src[i] ^ maskKey[i & 3];

    UINT32 result = tlsContext.Write(frame, frameLength);
    if (frame != stackFrame)
        delete[] frame;

    return (result == frameLength) ? bufferLength : 0;
}

// Read exactly `size` bytes from the TLS transport
BOOL WebSocketClient::ReceiveRestrict(PVOID buffer, UINT32 size)
{
    UINT32 totalBytesRead = 0;
    while (totalBytesRead < size)
    {
        SSIZE bytesRead = tlsContext.Read((PCHAR)buffer + totalBytesRead, size - totalBytesRead);
        if (bytesRead <= 0)
            return FALSE;
        totalBytesRead += (UINT32)bytesRead;
    }
    return TRUE;
}

VOID WebSocketClient::MaskFrame(WebSocketFrame &frame, UINT32 maskKey)
{
    PUINT8 mask = (PUINT8)&maskKey;
    PUINT8 d = (PUINT8)frame.data;
    UINT32 len = (UINT32)frame.length;

    // Process 4 bytes at a time (unrolled, no modulo in main loop)
    UINT32 i = 0;
    for (; i + 4 <= len; i += 4)
    {
        d[i]     ^= mask[0];
        d[i + 1] ^= mask[1];
        d[i + 2] ^= mask[2];
        d[i + 3] ^= mask[3];
    }

    // Remaining 0-3 bytes
    for (; i < len; i++)
        d[i] ^= mask[i & 3];
}

BOOL WebSocketClient::ReceiveFrame(WebSocketFrame &frame)
{
    UINT8 header[2] = {0};
    if (!ReceiveRestrict(&header, 2))
        return FALSE;

    UINT8 b1 = header[0];
    UINT8 b2 = header[1];

    frame.fin = (b1 >> 7) & 1;
    frame.rsv1 = (b1 >> 6) & 1;
    frame.rsv2 = (b1 >> 5) & 1;
    frame.rsv3 = (b1 >> 4) & 1;
    frame.opcode = (WebSocketOpcode)(b1 & 0x0F);
    frame.mask = (b2 >> 7) & 1;

    // RFC 6455 Section 5.2: RSV1-3 must be 0 unless extensions are negotiated
    if (frame.rsv1 || frame.rsv2 || frame.rsv3)
        return FALSE;

    UINT8 lengthBits = b2 & 0x7F;

    if (lengthBits == 126)
    {
        UINT16 len16 = 0;
        if (!ReceiveRestrict(&len16, 2))
            return FALSE;
        frame.length = UINT16SwapByteOrder(len16);
    }
    else if (lengthBits == 127)
    {
        UINT64 len64 = 0;
        if (!ReceiveRestrict(&len64, 8))
            return FALSE;
        frame.length = UINT64SwapByteOrder(len64);
    }
    else
    {
        frame.length = lengthBits;
    }

    UINT32 frameMask = 0;
    if (frame.mask)
    {
        if (!ReceiveRestrict(&frameMask, 4))
            return FALSE;
    }

    // Reject frames that would require an absurd allocation (>64 MB)
    if (frame.length > 0x4000000)
        return FALSE;

    frame.data = NULL;
    if (frame.length > 0)
    {
        frame.data = new CHAR[(UINT32)frame.length];
        if (!frame.data)
            return FALSE;

        if (!ReceiveRestrict(frame.data, (UINT32)frame.length))
        {
            delete[] frame.data;
            frame.data = NULL;
            return FALSE;
        }
    }

    if (frame.mask && frame.data)
        MaskFrame(frame, frameMask);

    return TRUE;
}

PVOID WebSocketClient::Read(USIZE &dwBufferLength, WebSocketOpcode &opcode)
{
    WebSocketFrame frame;
    PVOID pvBuffer = NULL;
    dwBufferLength = 0;
    WebSocketOpcode messageOpcode = OPCODE_BINARY;
    BOOL messageComplete = FALSE;

    while (isConnected)
    {
        Memory::Zero(&frame, sizeof(frame));
        if (!ReceiveFrame(frame))
            break;

        if (frame.opcode == OPCODE_TEXT || frame.opcode == OPCODE_BINARY || frame.opcode == OPCODE_CONTINUE)
        {
            if (frame.opcode == OPCODE_CONTINUE && pvBuffer == NULL)
            {
                delete[] frame.data;
                break;
            }

            // Capture opcode from the initial (non-continuation) frame
            if (frame.opcode != OPCODE_CONTINUE)
                messageOpcode = frame.opcode;

            if (frame.length > 0)
            {
                if (pvBuffer)
                {
                    PCHAR tempBuffer = new CHAR[dwBufferLength + (UINT32)frame.length];
                    if (!tempBuffer)
                    {
                        delete[] frame.data;
                        break;
                    }
                    Memory::Copy(tempBuffer, (PCHAR)pvBuffer, dwBufferLength);
                    Memory::Copy(tempBuffer + dwBufferLength, frame.data, (UINT32)frame.length);
                    dwBufferLength += (UINT32)frame.length;
                    delete[] (PCHAR)pvBuffer;
                    pvBuffer = tempBuffer;
                    delete[] frame.data;
                }
                else
                {
                    pvBuffer = frame.data;
                    dwBufferLength = (UINT32)frame.length;
                }
            }

            if (frame.fin)
            {
                opcode = messageOpcode;
                messageComplete = TRUE;
                break;
            }
        }
        else if (frame.opcode == OPCODE_CLOSE)
        {
            // Send close response per RFC 6455 Section 5.5.1
            Write(frame.data, (frame.length >= 2) ? 2 : 0, OPCODE_CLOSE);
            delete[] frame.data;
            isConnected = FALSE;
            break;
        }
        else if (frame.opcode == OPCODE_PING)
        {
            Write(frame.data, (UINT32)frame.length, OPCODE_PONG);
            delete[] frame.data;
        }
        else if (frame.opcode == OPCODE_PONG)
        {
            delete[] frame.data;
        }
        else
        {
            delete[] frame.data;
            break;
        }
    }

    if (!messageComplete)
    {
        delete[] (PCHAR)pvBuffer;
        pvBuffer = NULL;
        dwBufferLength = 0;
    }

    return pvBuffer;
}

WebSocketClient::WebSocketClient(PCCHAR url)
{
    Memory::Zero(hostName, sizeof(hostName));
    Memory::Zero(path, sizeof(path));
    port = 0;
    isConnected = FALSE;

    BOOL isSecure = FALSE;
    if (!HttpClient::ParseUrl(url, hostName, path, port, isSecure))
        return;

    ipAddress = DNS::Resolve(hostName);
    if (!ipAddress.IsValid())
    {
        LOG_ERROR("Failed to resolve hostname %s", hostName);
        return;
    }

    tlsContext = TLSClient(hostName, ipAddress, port, isSecure);
}
