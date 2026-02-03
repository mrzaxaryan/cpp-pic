/**
 * networkio.h - Network I/O Functions for PIL (via PLATFORM/RUNTIME)
 *
 * Provides network access through the Platform/Runtime Abstraction Layers.
 * Functions use handle-based resource management with fixed-size pools.
 *
 * Position-independent, no .rdata dependencies.
 *
 * Part of RUNTIME (Runtime Abstraction Layer).
 *
 * USAGE:
 *   script::NetworkContext netCtx;
 *   script::State L;
 *   script::OpenNetworkIO(L, &netCtx);  // Registers network functions
 *   L.DoString("var ip = dns_resolve(\"example.com\"); print(ip);");
 *
 * SOCKET FUNCTIONS:
 *   sock_connect(host, port)    - Connect to host:port, returns handle or -1
 *   sock_close(handle)          - Close socket, returns true/false
 *   sock_send(handle, data)     - Send data, returns bytes sent or -1
 *   sock_recv(handle [, size])  - Receive data (max 255 bytes), returns string
 *
 * DNS FUNCTIONS:
 *   dns_resolve(hostname)       - Resolve hostname to IP string
 *   dns_resolve4(hostname)      - Resolve hostname to IPv4 string
 *   dns_resolve6(hostname)      - Resolve hostname to IPv6 string
 *
 * HTTP FUNCTIONS:
 *   http_open(url)              - Create HTTP client for URL, returns handle or -1
 *   http_get(handle)            - Send GET request, returns true/false
 *   http_post(handle, data)     - Send POST request, returns true/false
 *   http_read(handle [, size])  - Read response (max 255 bytes), returns string
 *   http_close(handle)          - Close HTTP client, returns true/false
 *
 * WEBSOCKET FUNCTIONS:
 *   ws_connect(url)             - Connect to WebSocket server (ws:// or wss://), returns handle or -1
 *   ws_close(handle)            - Close WebSocket connection, returns true/false
 *   ws_send(handle, data [, opcode]) - Send data with optional opcode, returns bytes sent or -1
 *   ws_send_text(handle, data)  - Send text data (opcode=1), returns bytes sent or -1
 *   ws_recv(handle [, size])    - Receive data (max 255 bytes), returns string
 *   ws_ping(handle)             - Send ping frame, returns true/false
 *   ws_pong(handle)             - Send pong frame, returns true/false
 */

#pragma once

#include "value.h"  // includes State forward declaration
#include "pir/platform/network/socket.h"
#include "pir/runtime/network/dns.h"
#include "pir/runtime/network/http.h"
#include "pir/runtime/network/websocket.h"
#include "pir/core/types/embedded/embedded_function_pointer.h"

// ============================================================================
// PLACEMENT NEW OPERATOR (no <new> header in no-stdlib environment)
// ============================================================================

// Custom placement new for constructing objects in pre-allocated storage
// This is the standard signature that normally comes from <new>
inline void* operator new(USIZE, void* ptr) noexcept { return ptr; }
inline void operator delete(void*, void*) noexcept { }

namespace script
{

// ============================================================================
// NETWORK I/O CONFIGURATION
// ============================================================================

constexpr USIZE MAX_SOCKET_HANDLES = 8;      // Maximum simultaneously open sockets
constexpr USIZE MAX_HTTP_HANDLES = 4;        // Maximum simultaneously open HTTP clients
constexpr USIZE MAX_WS_HANDLES = 4;          // Maximum simultaneously open WebSocket clients

// ============================================================================
// SOCKET POOL - Manages open socket handles
// ============================================================================

/**
 * SocketPool - Fixed-size pool for managing open socket handles.
 * Socket handles are returned as numbers (0-7) to script code.
 */
class SocketPool
{
private:
    Socket m_sockets[MAX_SOCKET_HANDLES];
    BOOL m_inUse[MAX_SOCKET_HANDLES];

public:
    SocketPool() noexcept
    {
        for (USIZE i = 0; i < MAX_SOCKET_HANDLES; i++)
        {
            m_inUse[i] = FALSE;
        }
    }

    ~SocketPool() noexcept
    {
        CloseAll();
    }

    // Allocate a socket handle, returns -1 if pool exhausted
    NOINLINE INT32 Alloc() noexcept
    {
        for (USIZE i = 0; i < MAX_SOCKET_HANDLES; i++)
        {
            if (!m_inUse[i])
            {
                m_inUse[i] = TRUE;
                return (INT32)i;
            }
        }
        return -1;
    }

    // Get socket by handle
    FORCE_INLINE Socket* Get(INT32 handle) noexcept
    {
        if (handle < 0 || (USIZE)handle >= MAX_SOCKET_HANDLES || !m_inUse[handle])
        {
            return nullptr;
        }
        return &m_sockets[handle];
    }

    // Initialize socket at handle
    NOINLINE BOOL Init(INT32 handle, const IPAddress& ip, UINT16 port) noexcept
    {
        if (handle < 0 || (USIZE)handle >= MAX_SOCKET_HANDLES || !m_inUse[handle])
        {
            return FALSE;
        }
        // Use assignment - Socket has default copy/move since no special members declared
        Socket temp(ip, port);
        m_sockets[handle] = temp;
        return TRUE;
    }

    // Free a socket handle
    NOINLINE void Free(INT32 handle) noexcept
    {
        if (handle >= 0 && (USIZE)handle < MAX_SOCKET_HANDLES && m_inUse[handle])
        {
            m_sockets[handle].Close();
            m_inUse[handle] = FALSE;
        }
    }

    // Close all open sockets
    NOINLINE void CloseAll() noexcept
    {
        for (USIZE i = 0; i < MAX_SOCKET_HANDLES; i++)
        {
            if (m_inUse[i])
            {
                m_sockets[i].Close();
                m_inUse[i] = FALSE;
            }
        }
    }

    // Check if handle is valid
    FORCE_INLINE BOOL IsValid(INT32 handle) const noexcept
    {
        return handle >= 0 && (USIZE)handle < MAX_SOCKET_HANDLES && m_inUse[handle];
    }
};

// ============================================================================
// HTTP CLIENT POOL - Manages open HTTP client handles
// ============================================================================

/**
 * HttpClientPool - Fixed-size pool for managing HTTP client handles.
 * HTTP handles are returned as numbers (0-3) to script code.
 */
class HttpClientPool
{
private:
    HttpClient* m_clients[MAX_HTTP_HANDLES];  // Pointers to clients (stack-allocated externally)
    BOOL m_inUse[MAX_HTTP_HANDLES];

    // Storage for HttpClient objects (inline to avoid dynamic allocation)
    // We use a CHAR array and placement new because HttpClient has non-trivial members
    alignas(HttpClient) CHAR m_storage[MAX_HTTP_HANDLES][sizeof(HttpClient)];

public:
    HttpClientPool() noexcept
    {
        for (USIZE i = 0; i < MAX_HTTP_HANDLES; i++)
        {
            m_inUse[i] = FALSE;
            m_clients[i] = nullptr;
        }
    }

    ~HttpClientPool() noexcept
    {
        CloseAll();
    }

    // Allocate an HTTP client handle, returns -1 if pool exhausted
    NOINLINE INT32 Alloc() noexcept
    {
        for (USIZE i = 0; i < MAX_HTTP_HANDLES; i++)
        {
            if (!m_inUse[i])
            {
                m_inUse[i] = TRUE;
                return (INT32)i;
            }
        }
        return -1;
    }

    // Initialize HTTP client at handle with URL
    NOINLINE BOOL Init(INT32 handle, const CHAR* url) noexcept
    {
        if (handle < 0 || (USIZE)handle >= MAX_HTTP_HANDLES || !m_inUse[handle])
        {
            return FALSE;
        }
        // Destroy existing client if any
        if (m_clients[handle])
        {
            m_clients[handle]->~HttpClient();
        }
        // Placement new to initialize HttpClient
        m_clients[handle] = new (m_storage[handle]) HttpClient(url);
        return TRUE;
    }

    // Get HTTP client by handle
    FORCE_INLINE HttpClient* Get(INT32 handle) noexcept
    {
        if (handle < 0 || (USIZE)handle >= MAX_HTTP_HANDLES || !m_inUse[handle])
        {
            return nullptr;
        }
        return m_clients[handle];
    }

    // Free an HTTP client handle
    NOINLINE void Free(INT32 handle) noexcept
    {
        if (handle >= 0 && (USIZE)handle < MAX_HTTP_HANDLES && m_inUse[handle])
        {
            if (m_clients[handle])
            {
                m_clients[handle]->Close();
                m_clients[handle]->~HttpClient();
                m_clients[handle] = nullptr;
            }
            m_inUse[handle] = FALSE;
        }
    }

    // Close all open HTTP clients
    NOINLINE void CloseAll() noexcept
    {
        for (USIZE i = 0; i < MAX_HTTP_HANDLES; i++)
        {
            if (m_inUse[i])
            {
                if (m_clients[i])
                {
                    m_clients[i]->Close();
                    m_clients[i]->~HttpClient();
                    m_clients[i] = nullptr;
                }
                m_inUse[i] = FALSE;
            }
        }
    }

    // Check if handle is valid
    FORCE_INLINE BOOL IsValid(INT32 handle) const noexcept
    {
        return handle >= 0 && (USIZE)handle < MAX_HTTP_HANDLES && m_inUse[handle];
    }
};

// ============================================================================
// WEBSOCKET CLIENT POOL - Manages open WebSocket client handles
// ============================================================================

/**
 * WebSocketClientPool - Fixed-size pool for managing WebSocket client handles.
 * WebSocket handles are returned as numbers (0-3) to script code.
 */
class WebSocketClientPool
{
private:
    WebSocketClient* m_clients[MAX_WS_HANDLES];
    BOOL m_inUse[MAX_WS_HANDLES];

    // Storage for WebSocketClient objects (inline to avoid dynamic allocation)
    alignas(WebSocketClient) CHAR m_storage[MAX_WS_HANDLES][sizeof(WebSocketClient)];

public:
    WebSocketClientPool() noexcept
    {
        for (USIZE i = 0; i < MAX_WS_HANDLES; i++)
        {
            m_inUse[i] = FALSE;
            m_clients[i] = nullptr;
        }
    }

    ~WebSocketClientPool() noexcept
    {
        CloseAll();
    }

    // Allocate a WebSocket client handle, returns -1 if pool exhausted
    NOINLINE INT32 Alloc() noexcept
    {
        for (USIZE i = 0; i < MAX_WS_HANDLES; i++)
        {
            if (!m_inUse[i])
            {
                m_inUse[i] = TRUE;
                return (INT32)i;
            }
        }
        return -1;
    }

    // Initialize WebSocket client at handle with URL
    NOINLINE BOOL Init(INT32 handle, const CHAR* url) noexcept
    {
        if (handle < 0 || (USIZE)handle >= MAX_WS_HANDLES || !m_inUse[handle])
        {
            return FALSE;
        }
        // Destroy existing client if any
        if (m_clients[handle])
        {
            m_clients[handle]->~WebSocketClient();
        }
        // Placement new to initialize WebSocketClient (use global ::new to bypass class-deleted operator)
        m_clients[handle] = ::new (m_storage[handle]) WebSocketClient(url);
        return TRUE;
    }

    // Get WebSocket client by handle
    FORCE_INLINE WebSocketClient* Get(INT32 handle) noexcept
    {
        if (handle < 0 || (USIZE)handle >= MAX_WS_HANDLES || !m_inUse[handle])
        {
            return nullptr;
        }
        return m_clients[handle];
    }

    // Free a WebSocket client handle
    NOINLINE void Free(INT32 handle) noexcept
    {
        if (handle >= 0 && (USIZE)handle < MAX_WS_HANDLES && m_inUse[handle])
        {
            if (m_clients[handle])
            {
                m_clients[handle]->Close();
                m_clients[handle]->~WebSocketClient();
                m_clients[handle] = nullptr;
            }
            m_inUse[handle] = FALSE;
        }
    }

    // Close all open WebSocket clients
    NOINLINE void CloseAll() noexcept
    {
        for (USIZE i = 0; i < MAX_WS_HANDLES; i++)
        {
            if (m_inUse[i])
            {
                if (m_clients[i])
                {
                    m_clients[i]->Close();
                    m_clients[i]->~WebSocketClient();
                    m_clients[i] = nullptr;
                }
                m_inUse[i] = FALSE;
            }
        }
    }

    // Check if handle is valid
    FORCE_INLINE BOOL IsValid(INT32 handle) const noexcept
    {
        return handle >= 0 && (USIZE)handle < MAX_WS_HANDLES && m_inUse[handle];
    }
};

// ============================================================================
// NETWORK CONTEXT - Holds all network-related pools
// ============================================================================

/**
 * NetworkContext - Container for all network resource pools.
 * Pass to OpenNetworkIO() to register network functions.
 */
struct NetworkContext
{
    SocketPool sockets;
    HttpClientPool httpClients;
    WebSocketClientPool websockets;
};

// ============================================================================
// HELPER TO GET NETWORK CONTEXT FROM STATE
// ============================================================================

FORCE_INLINE NetworkContext* GetNetworkContext(FunctionContext& ctx) noexcept
{
    return ctx.state ? static_cast<NetworkContext*>(ctx.state->GetUserData()) : nullptr;
}

// ============================================================================
// SOCKET FUNCTIONS
// ============================================================================

/**
 * sock_connect(host, port) - Connect to a remote host
 *
 * @param host Hostname or IP address string
 * @param port Port number
 * @return Socket handle (number) or -1 on error
 */
NOINLINE Value NetIO_SockConnect(FunctionContext& ctx) noexcept
{
    if (!ctx.CheckArgs(2) || !ctx.IsString(0) || !ctx.IsNumber(1))
    {
        return Value::Number(-1);
    }

    NetworkContext* netCtx = GetNetworkContext(ctx);
    if (!netCtx)
    {
        return Value::Number(-1);
    }

    const CHAR* host = ctx.ToString(0);
    UINT16 port = (UINT16)ctx.ToNumber(1);

    // Resolve hostname to IP address
    IPAddress ip = DNS::Resolve(host);
    if (!ip.IsValid())
    {
        return Value::Number(-1);
    }

    // Allocate socket handle
    INT32 handle = netCtx->sockets.Alloc();
    if (handle < 0)
    {
        return Value::Number(-1);  // Pool exhausted
    }

    // Initialize socket
    if (!netCtx->sockets.Init(handle, ip, port))
    {
        netCtx->sockets.Free(handle);
        return Value::Number(-1);
    }

    // Open connection
    Socket* sock = netCtx->sockets.Get(handle);
    if (!sock || !sock->Open())
    {
        netCtx->sockets.Free(handle);
        return Value::Number(-1);
    }

    return Value::Number(handle);
}

/**
 * sock_close(handle) - Close a socket
 *
 * @param handle Socket handle from sock_connect
 * @return true on success, false on error
 */
NOINLINE Value NetIO_SockClose(FunctionContext& ctx) noexcept
{
    if (!ctx.CheckArgs(1) || !ctx.IsNumber(0))
    {
        return Value::Bool(FALSE);
    }

    NetworkContext* netCtx = GetNetworkContext(ctx);
    if (!netCtx)
    {
        return Value::Bool(FALSE);
    }

    INT32 handle = (INT32)ctx.ToNumber(0);
    if (!netCtx->sockets.IsValid(handle))
    {
        return Value::Bool(FALSE);
    }

    netCtx->sockets.Free(handle);
    return Value::Bool(TRUE);
}

/**
 * sock_send(handle, data) - Send data through socket
 *
 * @param handle Socket handle
 * @param data String data to send
 * @return Number of bytes sent, or -1 on error
 */
NOINLINE Value NetIO_SockSend(FunctionContext& ctx) noexcept
{
    if (!ctx.CheckArgs(2) || !ctx.IsNumber(0) || !ctx.IsString(1))
    {
        return Value::Number(-1);
    }

    NetworkContext* netCtx = GetNetworkContext(ctx);
    if (!netCtx)
    {
        return Value::Number(-1);
    }

    INT32 handle = (INT32)ctx.ToNumber(0);
    Socket* sock = netCtx->sockets.Get(handle);
    if (!sock || !sock->IsValid())
    {
        return Value::Number(-1);
    }

    const CHAR* data = ctx.ToString(1);
    USIZE dataLen = ctx.ToStringLength(1);

    UINT32 bytesSent = sock->Write(data, (UINT32)dataLen);
    return Value::Number((INT64)bytesSent);
}

/**
 * sock_recv(handle [, size]) - Receive data from socket
 *
 * @param handle Socket handle
 * @param size Optional max bytes to read (default 255)
 * @return String with received data, or empty string on error/no data
 */
NOINLINE Value NetIO_SockRecv(FunctionContext& ctx) noexcept
{
    if (!ctx.CheckArgsMin(1) || !ctx.IsNumber(0))
    {
        return Value::String(""_embed, 0);
    }

    NetworkContext* netCtx = GetNetworkContext(ctx);
    if (!netCtx)
    {
        return Value::String(""_embed, 0);
    }

    INT32 handle = (INT32)ctx.ToNumber(0);
    Socket* sock = netCtx->sockets.Get(handle);
    if (!sock || !sock->IsValid())
    {
        return Value::String(""_embed, 0);
    }

    // Determine read size
    UINT32 readSize = 255;  // Default max (leave room for null terminator)
    if (ctx.GetArgCount() >= 2 && ctx.IsNumber(1))
    {
        INT64 requested = ctx.ToNumber(1);
        if (requested <= 0)
        {
            return Value::String(""_embed, 0);
        }
        if (requested < 255)
        {
            readSize = (UINT32)requested;
        }
    }

    // Read data
    CHAR buffer[256];
    SSIZE bytesRead = sock->Read(buffer, readSize);
    if (bytesRead <= 0)
    {
        return Value::String(""_embed, 0);
    }

    buffer[bytesRead] = '\0';
    return Value::String(buffer, (USIZE)bytesRead);
}

// ============================================================================
// DNS FUNCTIONS
// ============================================================================

/**
 * dns_resolve(hostname) - Resolve hostname to IP address string
 *
 * Tries IPv6 first, then IPv4.
 *
 * @param hostname Hostname to resolve
 * @return IP address string, or empty string on failure
 */
NOINLINE Value NetIO_DnsResolve(FunctionContext& ctx) noexcept
{
    if (!ctx.CheckArgs(1) || !ctx.IsString(0))
    {
        return Value::String(""_embed, 0);
    }

    const CHAR* hostname = ctx.ToString(0);

    IPAddress ip = DNS::Resolve(hostname);
    if (!ip.IsValid())
    {
        return Value::String(""_embed, 0);
    }

    CHAR ipStr[64];
    if (!ip.ToString(ipStr, sizeof(ipStr)))
    {
        return Value::String(""_embed, 0);
    }

    // Calculate string length
    USIZE len = 0;
    while (ipStr[len] != '\0' && len < 63) len++;

    return Value::String(ipStr, len);
}

/**
 * dns_resolve4(hostname) - Resolve hostname to IPv4 address string
 *
 * @param hostname Hostname to resolve
 * @return IPv4 address string, or empty string on failure
 */
NOINLINE Value NetIO_DnsResolve4(FunctionContext& ctx) noexcept
{
    if (!ctx.CheckArgs(1) || !ctx.IsString(0))
    {
        return Value::String(""_embed, 0);
    }

    const CHAR* hostname = ctx.ToString(0);

    // Use CloudflareResolve with A record type for IPv4
    IPAddress ip = DNS::CloudflareResolve(hostname, RequestType::A);
    if (!ip.IsValid())
    {
        return Value::String(""_embed, 0);
    }

    CHAR ipStr[64];
    if (!ip.ToString(ipStr, sizeof(ipStr)))
    {
        return Value::String(""_embed, 0);
    }

    USIZE len = 0;
    while (ipStr[len] != '\0' && len < 63) len++;

    return Value::String(ipStr, len);
}

/**
 * dns_resolve6(hostname) - Resolve hostname to IPv6 address string
 *
 * @param hostname Hostname to resolve
 * @return IPv6 address string, or empty string on failure
 */
NOINLINE Value NetIO_DnsResolve6(FunctionContext& ctx) noexcept
{
    if (!ctx.CheckArgs(1) || !ctx.IsString(0))
    {
        return Value::String(""_embed, 0);
    }

    const CHAR* hostname = ctx.ToString(0);

    // Use CloudflareResolve with AAAA record type for IPv6
    IPAddress ip = DNS::CloudflareResolve(hostname, RequestType::AAAA);
    if (!ip.IsValid())
    {
        return Value::String(""_embed, 0);
    }

    CHAR ipStr[64];
    if (!ip.ToString(ipStr, sizeof(ipStr)))
    {
        return Value::String(""_embed, 0);
    }

    USIZE len = 0;
    while (ipStr[len] != '\0' && len < 63) len++;

    return Value::String(ipStr, len);
}

// ============================================================================
// HTTP FUNCTIONS
// ============================================================================

/**
 * http_open(url) - Create an HTTP client for the given URL
 *
 * @param url Full URL (http:// or https://)
 * @return HTTP client handle (number) or -1 on error
 */
NOINLINE Value NetIO_HttpOpen(FunctionContext& ctx) noexcept
{
    if (!ctx.CheckArgs(1) || !ctx.IsString(0))
    {
        return Value::Number(-1);
    }

    NetworkContext* netCtx = GetNetworkContext(ctx);
    if (!netCtx)
    {
        return Value::Number(-1);
    }

    const CHAR* url = ctx.ToString(0);

    // Allocate handle
    INT32 handle = netCtx->httpClients.Alloc();
    if (handle < 0)
    {
        return Value::Number(-1);  // Pool exhausted
    }

    // Initialize HTTP client with URL
    if (!netCtx->httpClients.Init(handle, url))
    {
        netCtx->httpClients.Free(handle);
        return Value::Number(-1);
    }

    // Open connection
    HttpClient* client = netCtx->httpClients.Get(handle);
    if (!client || !client->Open())
    {
        netCtx->httpClients.Free(handle);
        return Value::Number(-1);
    }

    return Value::Number(handle);
}

/**
 * http_get(handle) - Send HTTP GET request
 *
 * @param handle HTTP client handle from http_open
 * @return true on success, false on error
 */
NOINLINE Value NetIO_HttpGet(FunctionContext& ctx) noexcept
{
    if (!ctx.CheckArgs(1) || !ctx.IsNumber(0))
    {
        return Value::Bool(FALSE);
    }

    NetworkContext* netCtx = GetNetworkContext(ctx);
    if (!netCtx)
    {
        return Value::Bool(FALSE);
    }

    INT32 handle = (INT32)ctx.ToNumber(0);
    HttpClient* client = netCtx->httpClients.Get(handle);
    if (!client)
    {
        return Value::Bool(FALSE);
    }

    return Value::Bool(client->SendGetRequest());
}

/**
 * http_post(handle, data) - Send HTTP POST request
 *
 * @param handle HTTP client handle from http_open
 * @param data String data to send in POST body
 * @return true on success, false on error
 */
NOINLINE Value NetIO_HttpPost(FunctionContext& ctx) noexcept
{
    if (!ctx.CheckArgs(2) || !ctx.IsNumber(0) || !ctx.IsString(1))
    {
        return Value::Bool(FALSE);
    }

    NetworkContext* netCtx = GetNetworkContext(ctx);
    if (!netCtx)
    {
        return Value::Bool(FALSE);
    }

    INT32 handle = (INT32)ctx.ToNumber(0);
    HttpClient* client = netCtx->httpClients.Get(handle);
    if (!client)
    {
        return Value::Bool(FALSE);
    }

    const CHAR* data = ctx.ToString(1);
    USIZE dataLen = ctx.ToStringLength(1);

    return Value::Bool(client->SendPostRequest(data, (UINT32)dataLen));
}

/**
 * http_read(handle [, size]) - Read response data from HTTP client
 *
 * @param handle HTTP client handle
 * @param size Optional max bytes to read (default 255)
 * @return String with response data, or empty string on error/no data
 */
NOINLINE Value NetIO_HttpRead(FunctionContext& ctx) noexcept
{
    if (!ctx.CheckArgsMin(1) || !ctx.IsNumber(0))
    {
        return Value::String(""_embed, 0);
    }

    NetworkContext* netCtx = GetNetworkContext(ctx);
    if (!netCtx)
    {
        return Value::String(""_embed, 0);
    }

    INT32 handle = (INT32)ctx.ToNumber(0);
    HttpClient* client = netCtx->httpClients.Get(handle);
    if (!client)
    {
        return Value::String(""_embed, 0);
    }

    // Determine read size
    UINT32 readSize = 255;  // Default max (leave room for null terminator)
    if (ctx.GetArgCount() >= 2 && ctx.IsNumber(1))
    {
        INT64 requested = ctx.ToNumber(1);
        if (requested <= 0)
        {
            return Value::String(""_embed, 0);
        }
        if (requested < 255)
        {
            readSize = (UINT32)requested;
        }
    }

    // Read data
    CHAR buffer[256];
    SSIZE bytesRead = client->Read(buffer, readSize);
    if (bytesRead <= 0)
    {
        return Value::String(""_embed, 0);
    }

    buffer[bytesRead] = '\0';
    return Value::String(buffer, (USIZE)bytesRead);
}

/**
 * http_close(handle) - Close an HTTP client
 *
 * @param handle HTTP client handle
 * @return true on success, false on error
 */
NOINLINE Value NetIO_HttpClose(FunctionContext& ctx) noexcept
{
    if (!ctx.CheckArgs(1) || !ctx.IsNumber(0))
    {
        return Value::Bool(FALSE);
    }

    NetworkContext* netCtx = GetNetworkContext(ctx);
    if (!netCtx)
    {
        return Value::Bool(FALSE);
    }

    INT32 handle = (INT32)ctx.ToNumber(0);
    if (!netCtx->httpClients.IsValid(handle))
    {
        return Value::Bool(FALSE);
    }

    netCtx->httpClients.Free(handle);
    return Value::Bool(TRUE);
}

// ============================================================================
// WEBSOCKET FUNCTIONS
// ============================================================================

/**
 * ws_connect(url) - Connect to a WebSocket server
 *
 * @param url WebSocket URL (ws:// or wss://)
 * @return WebSocket handle (number) or -1 on error
 */
NOINLINE Value NetIO_WsConnect(FunctionContext& ctx) noexcept
{
    if (!ctx.CheckArgs(1) || !ctx.IsString(0))
    {
        return Value::Number(-1);
    }

    NetworkContext* netCtx = GetNetworkContext(ctx);
    if (!netCtx)
    {
        return Value::Number(-1);
    }

    const CHAR* url = ctx.ToString(0);

    // Allocate handle
    INT32 handle = netCtx->websockets.Alloc();
    if (handle < 0)
    {
        return Value::Number(-1);  // Pool exhausted
    }

    // Initialize WebSocket client with URL
    if (!netCtx->websockets.Init(handle, url))
    {
        netCtx->websockets.Free(handle);
        return Value::Number(-1);
    }

    // Open connection
    WebSocketClient* client = netCtx->websockets.Get(handle);
    if (!client || !client->Open())
    {
        netCtx->websockets.Free(handle);
        return Value::Number(-1);
    }

    return Value::Number(handle);
}

/**
 * ws_close(handle) - Close a WebSocket connection
 *
 * @param handle WebSocket handle from ws_connect
 * @return true on success, false on error
 */
NOINLINE Value NetIO_WsClose(FunctionContext& ctx) noexcept
{
    if (!ctx.CheckArgs(1) || !ctx.IsNumber(0))
    {
        return Value::Bool(FALSE);
    }

    NetworkContext* netCtx = GetNetworkContext(ctx);
    if (!netCtx)
    {
        return Value::Bool(FALSE);
    }

    INT32 handle = (INT32)ctx.ToNumber(0);
    if (!netCtx->websockets.IsValid(handle))
    {
        return Value::Bool(FALSE);
    }

    netCtx->websockets.Free(handle);
    return Value::Bool(TRUE);
}

/**
 * ws_send(handle, data [, opcode]) - Send data through WebSocket
 *
 * @param handle WebSocket handle
 * @param data String data to send
 * @param opcode Optional opcode (default: 2 for binary, 1 for text)
 * @return Number of bytes sent, or -1 on error
 */
NOINLINE Value NetIO_WsSend(FunctionContext& ctx) noexcept
{
    if (!ctx.CheckArgsMin(2) || !ctx.IsNumber(0) || !ctx.IsString(1))
    {
        return Value::Number(-1);
    }

    NetworkContext* netCtx = GetNetworkContext(ctx);
    if (!netCtx)
    {
        return Value::Number(-1);
    }

    INT32 handle = (INT32)ctx.ToNumber(0);
    WebSocketClient* client = netCtx->websockets.Get(handle);
    if (!client)
    {
        return Value::Number(-1);
    }

    const CHAR* data = ctx.ToString(1);
    USIZE dataLen = ctx.ToStringLength(1);

    // Default opcode is BINARY (2)
    INT8 opcode = OPCODE_BINARY;
    if (ctx.GetArgCount() >= 3 && ctx.IsNumber(2))
    {
        opcode = (INT8)ctx.ToNumber(2);
    }

    UINT32 bytesSent = client->Write(data, (UINT32)dataLen, opcode);
    return Value::Number((INT64)bytesSent);
}

/**
 * ws_send_text(handle, data) - Send text data through WebSocket
 *
 * Convenience function that sends with TEXT opcode.
 *
 * @param handle WebSocket handle
 * @param data String data to send
 * @return Number of bytes sent, or -1 on error
 */
NOINLINE Value NetIO_WsSendText(FunctionContext& ctx) noexcept
{
    if (!ctx.CheckArgs(2) || !ctx.IsNumber(0) || !ctx.IsString(1))
    {
        return Value::Number(-1);
    }

    NetworkContext* netCtx = GetNetworkContext(ctx);
    if (!netCtx)
    {
        return Value::Number(-1);
    }

    INT32 handle = (INT32)ctx.ToNumber(0);
    WebSocketClient* client = netCtx->websockets.Get(handle);
    if (!client)
    {
        return Value::Number(-1);
    }

    const CHAR* data = ctx.ToString(1);
    USIZE dataLen = ctx.ToStringLength(1);

    UINT32 bytesSent = client->Write(data, (UINT32)dataLen, OPCODE_TEXT);
    return Value::Number((INT64)bytesSent);
}

/**
 * ws_recv(handle [, size]) - Receive data from WebSocket
 *
 * @param handle WebSocket handle
 * @param size Optional max bytes to read (default 255)
 * @return String with received data, or empty string on error/no data
 */
NOINLINE Value NetIO_WsRecv(FunctionContext& ctx) noexcept
{
    if (!ctx.CheckArgsMin(1) || !ctx.IsNumber(0))
    {
        return Value::String(""_embed, 0);
    }

    NetworkContext* netCtx = GetNetworkContext(ctx);
    if (!netCtx)
    {
        return Value::String(""_embed, 0);
    }

    INT32 handle = (INT32)ctx.ToNumber(0);
    WebSocketClient* client = netCtx->websockets.Get(handle);
    if (!client)
    {
        return Value::String(""_embed, 0);
    }

    // Determine max read size
    USIZE maxSize = 255;  // Default max (leave room for null terminator)
    if (ctx.GetArgCount() >= 2 && ctx.IsNumber(1))
    {
        INT64 requested = ctx.ToNumber(1);
        if (requested <= 0)
        {
            return Value::String(""_embed, 0);
        }
        if (requested < 255)
        {
            maxSize = (USIZE)requested;
        }
    }

    // Read data from WebSocket
    USIZE bytesRead = 0;
    INT8 opcode = 0;
    PVOID data = client->Read(&bytesRead, &opcode);

    if (!data || bytesRead == 0)
    {
        return Value::String(""_embed, 0);
    }

    // Limit to max size
    if (bytesRead > maxSize)
    {
        bytesRead = maxSize;
    }

    // Copy to local buffer and null-terminate
    CHAR buffer[256];
    for (USIZE i = 0; i < bytesRead; i++)
    {
        buffer[i] = ((CHAR*)data)[i];
    }
    buffer[bytesRead] = '\0';

    return Value::String(buffer, bytesRead);
}

/**
 * ws_ping(handle) - Send WebSocket ping frame
 *
 * @param handle WebSocket handle
 * @return true on success, false on error
 */
NOINLINE Value NetIO_WsPing(FunctionContext& ctx) noexcept
{
    if (!ctx.CheckArgs(1) || !ctx.IsNumber(0))
    {
        return Value::Bool(FALSE);
    }

    NetworkContext* netCtx = GetNetworkContext(ctx);
    if (!netCtx)
    {
        return Value::Bool(FALSE);
    }

    INT32 handle = (INT32)ctx.ToNumber(0);
    WebSocketClient* client = netCtx->websockets.Get(handle);
    if (!client)
    {
        return Value::Bool(FALSE);
    }

    // Send empty ping frame - control frames return 0 bytes but still succeed
    client->Write(nullptr, 0, OPCODE_PING);
    return Value::Bool(TRUE);
}

/**
 * ws_pong(handle) - Send WebSocket pong frame
 *
 * @param handle WebSocket handle
 * @return true on success, false on error
 */
NOINLINE Value NetIO_WsPong(FunctionContext& ctx) noexcept
{
    if (!ctx.CheckArgs(1) || !ctx.IsNumber(0))
    {
        return Value::Bool(FALSE);
    }

    NetworkContext* netCtx = GetNetworkContext(ctx);
    if (!netCtx)
    {
        return Value::Bool(FALSE);
    }

    INT32 handle = (INT32)ctx.ToNumber(0);
    WebSocketClient* client = netCtx->websockets.Get(handle);
    if (!client)
    {
        return Value::Bool(FALSE);
    }

    // Send empty pong frame - control frames return 0 bytes but still succeed
    client->Write(nullptr, 0, OPCODE_PONG);
    return Value::Bool(TRUE);
}

// ============================================================================
// OPEN NETWORK I/O LIBRARY
// ============================================================================

/**
 * Register all network I/O functions with a State.
 *
 * @param L The script State to register functions with
 * @param ctx Pointer to NetworkContext instance (must outlive the State)
 *
 * IMPORTANT: The NetworkContext should have the same lifetime as the State.
 *
 * NOTE: This sets State's UserData to the NetworkContext pointer.
 *       If you need both FileIO and NetworkIO, create a combined context
 *       structure and access pools through appropriate casts.
 *
 * Functions registered:
 *   Socket:
 *     1. sock_connect  - Connect to host:port
 *     2. sock_close    - Close socket
 *     3. sock_send     - Send data
 *     4. sock_recv     - Receive data
 *
 *   DNS:
 *     5. dns_resolve   - Resolve hostname (IPv6 preferred)
 *     6. dns_resolve4  - Resolve hostname to IPv4
 *     7. dns_resolve6  - Resolve hostname to IPv6
 *
 *   HTTP:
 *     8. http_open     - Create HTTP client
 *     9. http_get      - Send GET request
 *    10. http_post     - Send POST request
 *    11. http_read     - Read response data
 *    12. http_close    - Close HTTP client
 *
 *   WebSocket:
 *    13. ws_connect    - Connect to WebSocket server
 *    14. ws_close      - Close WebSocket connection
 *    15. ws_send       - Send data with opcode
 *    16. ws_send_text  - Send text data
 *    17. ws_recv       - Receive data
 *    18. ws_ping       - Send ping frame
 *    19. ws_pong       - Send pong frame
 */
NOINLINE void OpenNetworkIO(State& L, NetworkContext* ctx) noexcept
{
    // Store NetworkContext pointer in State's user data
    L.SetUserData(ctx);

    // Socket functions
    L.Register("sock_connect"_embed, EMBED_FUNC(NetIO_SockConnect));
    L.Register("sock_close"_embed, EMBED_FUNC(NetIO_SockClose));
    L.Register("sock_send"_embed, EMBED_FUNC(NetIO_SockSend));
    L.Register("sock_recv"_embed, EMBED_FUNC(NetIO_SockRecv));

    // DNS functions
    L.Register("dns_resolve"_embed, EMBED_FUNC(NetIO_DnsResolve));
    L.Register("dns_resolve4"_embed, EMBED_FUNC(NetIO_DnsResolve4));
    L.Register("dns_resolve6"_embed, EMBED_FUNC(NetIO_DnsResolve6));

    // HTTP functions
    L.Register("http_open"_embed, EMBED_FUNC(NetIO_HttpOpen));
    L.Register("http_get"_embed, EMBED_FUNC(NetIO_HttpGet));
    L.Register("http_post"_embed, EMBED_FUNC(NetIO_HttpPost));
    L.Register("http_read"_embed, EMBED_FUNC(NetIO_HttpRead));
    L.Register("http_close"_embed, EMBED_FUNC(NetIO_HttpClose));

    // WebSocket functions
    L.Register("ws_connect"_embed, EMBED_FUNC(NetIO_WsConnect));
    L.Register("ws_close"_embed, EMBED_FUNC(NetIO_WsClose));
    L.Register("ws_send"_embed, EMBED_FUNC(NetIO_WsSend));
    L.Register("ws_send_text"_embed, EMBED_FUNC(NetIO_WsSendText));
    L.Register("ws_recv"_embed, EMBED_FUNC(NetIO_WsRecv));
    L.Register("ws_ping"_embed, EMBED_FUNC(NetIO_WsPing));
    L.Register("ws_pong"_embed, EMBED_FUNC(NetIO_WsPong));
}

} // namespace script
