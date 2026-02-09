#include "websocket.h"
#include "memory.h"
#include "string.h"
#include "random.h"
#include "logger.h"
#include "dns.h"
#include "http.h"
#include "embedded_string.h"

// This function creates a WebSocket frame with the specified parameters.
static BOOL web_socket_create_frame(PWebSocketFrame frame, INT32 fin, INT32 rsv1, INT32 rsv2, INT32 rsv3, INT32 opcode, INT32 has_mask, PVOID data, INT32 len)
{

    // Initialize the frame structure
    frame->fin = fin;
    frame->rsv1 = rsv1;
    frame->rsv2 = rsv2;
    frame->rsv3 = rsv3;
    frame->mask = has_mask;
    frame->opcode = opcode;
    frame->data = (PCHAR)data;
    frame->length = len;

    return TRUE; // Frame created successfully
}

// Callback function for formatting WebSocket handshake request headers, used with StringFormatter
BOOL WebSocketClient::FormatterCallback(PVOID context, CHAR ch)
{
    WebSocketClient *wsClient = (WebSocketClient *)context;
    if (wsClient->isSecure)
    {
        return wsClient->tlsContext.Write(&ch, 1);
    }
    else
    {
        return wsClient->socketContext.Write(&ch, 1);
    }
    return TRUE;
}

// This function connects a WebSocket client context to a specified IP address, host, path, port, and security setting.
BOOL WebSocketClient::Open()
{
    PCHAR headers = NULL;
    LOG_DEBUG("Opening WebSocket client to %s:%u%s (secure: %s)", hostName, port, path, isSecure ? "true"_embed : "false"_embed);

    if (isSecure)
    {
        if (!tlsContext.Open())
        {
            LOG_DEBUG("Failed to open network transport for WebSocket client");
            return FALSE;
        }
    }
    else
    {
        if (!socketContext.Open())
        {
            LOG_DEBUG("Failed to open network transport for WebSocket client");
            return FALSE;
        }
    }

    CHAR key[16];
    CHAR alphanum[63];
    for (INT32 i = 0; i < 10; i++)
    {
        alphanum[i] = '0' + i;
    }

    for (INT32 i = 0; i < 26; i++)
    {
        alphanum[i + 10] = 'A' + i;
    }

    for (INT32 i = 0; i < 26; i++)
    {
        alphanum[i + 36] = 'a' + i;
    }

    // set \0
    alphanum[62] = '\0';

    Random random;
    // Generate a random 16-byte key
    for (INT32 i = 0; i < 16; i++)
    {
        key[i] = alphanum[random.Get() % 61];
    }

    // Allocate memory for the encoded string
    PCHAR secureKey = new CHAR[Base64::GetEncodeOutSize(16)];
    Base64::Encode(key, 16, secureKey);

    // Prepare the WebSocket handshake request
    auto format = "GET %s HTTP/1.1\r\n"_embed
                  "Host: %s\r\n"_embed
                  "Upgrade: WebSocket\r\n"_embed
                  "Connection: Upgrade\r\n"_embed
                  "Sec-WebSocket-Key: %s\r\n"_embed
                  "Sec-WebSocket-Version: 13\r\n"_embed;

    auto fixed = EMBED_FUNC(FormatterCallback);
    StringFormatter::Format<CHAR>(fixed, this, format, path, hostName, secureKey);
    delete[] secureKey;

    if (isSecure)
        StringFormatter::Format<CHAR>(fixed, this, "Origin: https://%s\r\n"_embed, hostName);
    else
        StringFormatter::Format<CHAR>(fixed, this, "Origin: http://%s\r\n"_embed, hostName);

    // End headers
    if (headers != NULL)
        StringFormatter::Format<CHAR>(fixed, this, "%s\r\n\r\n"_embed, headers);
    else
        StringFormatter::Format<CHAR>(fixed, this, "\r\n"_embed);

    UINT16 handshakeResponseBufferSize = 4096;                       // Buffer size for the handshake response
    PCHAR handshakeResponse = new CHAR[handshakeResponseBufferSize]; // Allocate memory for the handshake response
    UINT32 totalBytesRead = 0;                                       // Total bytes read from the handshake response

    for (;;)
    {
        // Check if the total bytes read exceeds the buffer size
        if (totalBytesRead >= handshakeResponseBufferSize)
        {
            delete[] handshakeResponse;
            Close();
            return FALSE;
        }

        UINT32 bytesRead; // Variable to hold the number of bytes read

        // Read 1 byte at a time from the WebSocket client context depending on whether it is secure or not
        if (isSecure)
        {
            bytesRead = tlsContext.Read(handshakeResponse + totalBytesRead, 1);
        }
        else
        {
            bytesRead = socketContext.Read(handshakeResponse + totalBytesRead, 1);
        }
        // Check if no bytes were read, indicating an error
        if (bytesRead == 0)
        {
            delete[] handshakeResponse;
            Close();
            return FALSE;
        }

        totalBytesRead += bytesRead; // Update the total bytes read

        // Check if the handshake response ends with the HTTP response end sequence
        if (totalBytesRead >= 4
            //  { '\r', '\n', '\r', '\n } in UINT32 == 168626701
            && *(PUINT32)(handshakeResponse + totalBytesRead - 4) ==
                   168626701)
        {
            break;
        }
    }
    handshakeResponse[totalBytesRead] = '\0'; // Null-terminate the handshake response string

    if (totalBytesRead < 12
        // { ' ', '1','0','1' } in UINT32 = 540094513
        || *(PUINT32)(handshakeResponse + 9) != 540094513)
    {
        delete[] handshakeResponse;
        Close();
        return FALSE;
    }

    delete[] handshakeResponse;
    isConnected = TRUE;
    return TRUE;
}

// This function disconnects a WebSocket client context
BOOL WebSocketClient::Close()
{
    isConnected = FALSE;
    if (isSecure)
    {
        tlsContext.Close();
    }
    else
    {
        socketContext.Close();
    }
    LOG_DEBUG("WebSocket client to %s:%u%s closed", hostName, port, path);
    return TRUE;
}

// This function writes data to a WebSocket client context, handling masking and framing according to the WebSocket protocol.
UINT32 WebSocketClient::Write(PCVOID buffer, UINT32 bufferLength, INT8 opcode)
{
    // Variable to hold the length of the WebSocket frame header
    // 6 for the first 2 bytes, 2 for the extended payload length if needed, and 4 for the masking key if masking is used
    // if bufferLength is greater than 125, we need to add 2 bytes for the extended payload length
    // if bufferLength is greater than 0xffff, we need to add 6 bytes for the extended payload length
    UINT32 header_length = 6 + (bufferLength > 125 ? 2 : 0) + (bufferLength > 0xffff ? 6 : 0);
    Random random;

    UINT8 masking_key[4];
    // Fill the masking key with random bytes
    for (USIZE i = 0; i < sizeof(masking_key); ++i)
    {
        masking_key[i] = random.Get() & 0xff;
    }
    // Allocate memory for the WebSocket frame header
    PUINT8 header = new UINT8[header_length];
    header[0] = (opcode | 0x80);

    // Handle the masking and payload length according to the WebSocket protocol
    if (bufferLength <= 125)
    {

        header[1] = (UINT8)(bufferLength | 0x80);
        Memory::Copy(header + 2, masking_key, 4);
    }
    else if (bufferLength > 125 && bufferLength <= 0xffff)
    { // 125 && 65535

        header[1] = (126 | 0x80);

        UINT16 len16 = UINT16SwapByteOrder((UINT16)bufferLength);
        Memory::Copy(header + 2, &len16, 2);
        Memory::Copy(header + 4, masking_key, 4);
    }
    else if (bufferLength > 0xffff)
    { // 65535 && 18446744073709551615

        // Use pointer arithmetic to extract the high and low 32-bit parts
        UINT64 len64 = UINT64SwapByteOrder(bufferLength);
        // Set the header fields
        header[1] = (127 | 0x80);
        Memory::Copy(header + 2, &len64, 8);
        Memory::Copy(header + 10, masking_key, 4); // Copy masking key
    }

    // Prepare the WebSocket frame length
    INT32 frame_length = header_length + bufferLength;
    // Allocate memory for the WebSocket frame
    PCHAR framebuf = new CHAR[frame_length];
    // Initialize the frame buffer with zeros and copy the header into it
    Memory::Zero(framebuf, frame_length);
    Memory::Copy(framebuf, header, header_length);
    // If the buffer is not NULL, copy the data into the frame buffer after the header
    if (buffer != NULL)
        Memory::Copy(framebuf + header_length, buffer, bufferLength);

    UINT32 i; // Iteration variable for the loop
    // Loop through the frame buffer and apply the masking key to the data
    for (i = 0; i < bufferLength; i++)
    {
        framebuf[header_length + i] ^= masking_key[i % 4] & 0xff;
    }

    INT32 result; // Variable to hold the result of the write operation

    if (isSecure)
    {
        result = tlsContext.Write(framebuf, frame_length);
    }
    else
    {
        result = socketContext.Write(framebuf, frame_length);
    }

    // Free the allocated memory for the header and frame buffer
    delete[] header;
    delete[] framebuf;

    // Return the payload length (bufferLength) if write succeeded, 0 on failure
    if (result > 0)
    {
        return bufferLength;
    }
    else
    {
        return 0;
    }
}

// This function receives data from a WebSocket client context
BOOL WebSocketClient::ReceiveRestrict(PVOID buffer, INT32 size)
{
    INT32 totalBytesRead = 0; // Total bytes read from the WebSocket client context
    INT32 bytesRead = 0;      // Variable to hold the number of bytes read in each iteration

    // Loop until the total bytes read is less than the requested size
    while (totalBytesRead < size)
    {
        if (isSecure)
        {
            bytesRead = tlsContext.Read((PCHAR)buffer + totalBytesRead, size - totalBytesRead);
        }
        else
        {
            bytesRead = socketContext.Read((PCHAR)buffer + totalBytesRead, size - totalBytesRead);
        }

        // Checking if read operation was successful
        if (bytesRead > 0)
        {
            totalBytesRead += bytesRead; // Update the total bytes read
        }
        else
        {
            return FALSE;
        }
    }

    return TRUE; // Indicating successful read operation
}

// This function masks a WebSocket frame using the provided mask key and data.
static PVOID web_socket_mask_frame(UINT32 mask_key, PVOID data, INT32 len)
{
    INT32 i = 0;                   // Iteration variable for the loop
    PUINT8 _m = (PUINT8)&mask_key; // Cast the mask key to a byte array
    PUINT8 _d = (PUINT8)data;      // Cast the data pointer to a byte array
    // Loop through the data and apply the mask to each byte
    for (; i < len; i++)
    {
        _d[i] ^= _m[i % 4];
    }
    return _d;
}

// This function receives a WebSocket frame from the client context and fills the provided frame structure with the received data.
BOOL WebSocketClient::ReceiveFrame(PWebSocketFrame frame)
{
    UINT8 b1, b2, fin, rsv1, rsv2, rsv3, opcode, has_mask; // these are the bits of the frame header
    UINT64 frame_length = 0;
    UINT16 length_data_16 = 0;
    UINT64 length_data_64 = 0;
    UINT32 frame_mask = 0;
    UINT8 length_bits = 0;
    UINT8 frame_header[2] = {0};
    PCHAR payload = NULL;

    // Attempt to receive the first 2 bytes of the WebSocket frame header
    if (!ReceiveRestrict(&frame_header, 2))
    {
        return FALSE;
    }

    // Initialize the frame header bits from the received data
    b1 = frame_header[0];
    b2 = frame_header[1];
    length_bits = b2 & 0x7f;
    fin = b1 >> 7 & 1;
    rsv1 = b1 >> 6 & 1;
    rsv2 = b1 >> 5 & 1;
    rsv3 = b1 >> 4 & 1;
    opcode = b1 & 0xf;
    has_mask = b2 >> 7 & 1;

    // Checking if the length bits indicate an extended payload length
    if (length_bits == 0x7e)
    {
        // Attempt to receive 2 bytes for the extended payload length
        if (!ReceiveRestrict(&length_data_16, 2))
        {
            return FALSE;
        }
        // Swap the byte order of the received length data
        frame_length = UINT16SwapByteOrder(length_data_16);
    }
    else if (length_bits == 0x7f)
    {
        // Attempt to receive 8 bytes for the extended payload length
        if (!ReceiveRestrict(&length_data_64, 8))
        {
            return FALSE;
        }
        // Swap the byte order of the received length data
        frame_length = UINT64SwapByteOrder(length_data_64);
    }
    else
    {
        frame_length = length_bits; // If the length bits are not extended, use the length bits directly
    }

    // Check if it has a mask
    if (has_mask)
    {
        // Attempt to receive 4 bytes for the masking key
        if (!ReceiveRestrict(&frame_mask, 4))
        {
            return FALSE;
        }
    }

    // Check if the frame length is valid
    if (frame_length > 0)
    {
        payload = new CHAR[(INT32)frame_length]; // Allocate memory for the payload
        // Attempt to receive the payload data
        if (!ReceiveRestrict(payload, (INT32)frame_length))
        {
            delete[] payload;
            return FALSE;
        }
    }
    /*else {
        return FALSE;

    }*/

    if (has_mask)
    {
        web_socket_mask_frame(frame_mask, payload, (UINT32)frame_length);
    }

    return web_socket_create_frame(frame, fin, rsv1, rsv2, rsv3, opcode, has_mask, payload, (UINT32)frame_length);
}
PVOID WebSocketClient::Read(PUSIZE dwBufferLength, PINT8 opcode)
{
    // Initialize the WebSocket frame structure and buffer for received data
    WebSocketFrame webSocketFrame;
    Memory::Zero(&webSocketFrame, sizeof(webSocketFrame));
    PWebSocketFrame pWebSocketFrame = &webSocketFrame;
    PVOID pvBuffer = NULL;
    *dwBufferLength = 0;

    while (1)
    {
        Memory::Zero(pWebSocketFrame, sizeof(webSocketFrame)); // Reset the WebSocket frame structure
        // Attempt to receive a WebSocket frame from the client context
        if (!ReceiveFrame(pWebSocketFrame))
        {
            goto end; // Error handling: if receiving the frame fails, exit the loop
        }
        // Checking the opcode of the received WebSocket frame
        if (pWebSocketFrame->opcode == OPCODE_TEXT || pWebSocketFrame->opcode == OPCODE_BINARY || pWebSocketFrame->opcode == OPCODE_CONTINUE)
        {
            if (pWebSocketFrame->opcode == OPCODE_CONTINUE && pvBuffer == NULL)
            {
                goto end;
            }
            if (pWebSocketFrame->length == 0)
            {
                goto end;
            }
            if (pvBuffer)
            {
                // Reallocate the buffer to hold the new data
                PCHAR tempBuffer = new CHAR[*dwBufferLength + (UINT32)pWebSocketFrame->length];
                Memory::Copy(tempBuffer, (PCHAR)pvBuffer, *dwBufferLength);
                delete[] (PCHAR)pvBuffer;
                pvBuffer = tempBuffer;
                // Otherwise, copy the new data into the existing buffer
                Memory::Copy((PCHAR)pvBuffer + *dwBufferLength, pWebSocketFrame->data, (UINT32)pWebSocketFrame->length);
                *dwBufferLength += (UINT32)pWebSocketFrame->length;
                delete[] pWebSocketFrame->data;
            }
            else
            {
                // Otherwise, allocate a new buffer to hold the received data
                pvBuffer = new CHAR[(UINT32)pWebSocketFrame->length];
                // Check if the allocation was successful
                if (pvBuffer == NULL)
                {
                    goto end;
                }
                // Copy the received data into the newly allocated buffer
                Memory::Copy(pvBuffer, pWebSocketFrame->data, (UINT32)pWebSocketFrame->length);
                *dwBufferLength = (UINT32)pWebSocketFrame->length;
                delete[] pWebSocketFrame->data;
            }
            // Check if the opcode is a final frame (fin bit is set)
            if (pWebSocketFrame->fin)
            {
                *opcode = pWebSocketFrame->opcode; // Set the opcode to the received frame's opcode
                goto end;                          // Exit the loop if the frame is final
            }
        }
        else if (pWebSocketFrame->opcode == OPCODE_CLOSE)
        {
            pvBuffer = NULL;
            *dwBufferLength = 0;
            CHAR reason[126];
            Memory::Zero(reason, sizeof(reason));
            Memory::Copy(reason, pWebSocketFrame->data + 2, (UINT32)((USIZE)pWebSocketFrame->length - 2));
            delete[] pWebSocketFrame->data;
            goto end;
        }
        // If opcode is OPCODE_PING, send a pong response
        else if (pWebSocketFrame->opcode == OPCODE_PING)
        {
            Write(pWebSocketFrame->data, pWebSocketFrame->length, OPCODE_PONG);
            delete[] pWebSocketFrame->data;
        }
        // If opcode is OPCODE_PONG,
        else if (pWebSocketFrame->opcode == OPCODE_PONG)
        {
        }
        else
        {

            goto end;
        }
    }

end:
    return pvBuffer;
}
WebSocketClient::WebSocketClient(PCCHAR url, PCCHAR ipAddress)
{
    this->ipAddress = IPAddress::FromString(ipAddress);
    isConnected = FALSE;

    // Attempt to parse the URL to extract the host name, path, port, and security setting
    if (!HttpClient::ParseUrl(url, hostName, path, &port, &isSecure))
    {
        // return FALSE;
    }
    if (isSecure)
    {
        tlsContext = TLSClient(hostName, this->ipAddress, port);
    }
    else
    {
        socketContext = Socket(this->ipAddress, port);
    }
}
WebSocketClient::WebSocketClient(PCCHAR url)
{
    isConnected = FALSE;

    // Attempt to parse the URL to extract the host name, path, port, and security setting
    if (!HttpClient::ParseUrl(url, hostName, path, &port, &isSecure))
    {
        // return FALSE;
    }

    // Buffer to hold the resolved IP address
    // Attempt to resolve the host name to an IP address (tries IPv6 first, falls back to IPv4)
    ipAddress = DNS::Resolve(hostName);
    if (!ipAddress.IsValid())
    {
        LOG_ERROR("Failed to resolve hostname %s", hostName);
        return;
    }

    if (isSecure)
    {
        tlsContext = TLSClient(hostName, ipAddress, port);
    }
    else
    {
        socketContext = Socket(ipAddress, port);
    }
}
