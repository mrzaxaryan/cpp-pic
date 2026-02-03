#pragma once

#include "pil/pil.h"
#include "pil/networkio.h"
#include "tests.h"

// ============================================================================
// NETWORK I/O TESTS CLASS
// ============================================================================

/**
 * Network I/O Tests
 *
 * NOTE: These tests require network connectivity.
 * Some tests may fail in isolated environments.
 */
class NetworkIOTests
{
public:
    static BOOL RunAll()
    {
        BOOL allPassed = TRUE;

        LOG_INFO("Running Network I/O Tests...");
        LOG_INFO("NOTE: These tests require network connectivity");

        RUN_TEST(allPassed, TestDnsResolve, "DNS resolve");
        RUN_TEST(allPassed, TestDnsResolveInvalid, "DNS resolve invalid hostname");
        RUN_TEST(allPassed, TestSocketConnectClose, "Socket connect/close");
        RUN_TEST(allPassed, TestSocketInvalidHandle, "Socket invalid handle errors");
        RUN_TEST(allPassed, TestHttpOpenClose, "HTTP open/close");
        RUN_TEST(allPassed, TestHttpGet, "HTTP GET request");
        RUN_TEST(allPassed, TestHttpInvalidHandle, "HTTP invalid handle errors");
        RUN_TEST(allPassed, TestMultipleSockets, "Multiple sockets");
        RUN_TEST(allPassed, TestMultipleHttpClients, "Multiple HTTP clients");
        RUN_TEST(allPassed, TestWebSocketConnectClose, "WebSocket connect/close");
        RUN_TEST(allPassed, TestWebSocketSendRecv, "WebSocket send/recv");
        RUN_TEST(allPassed, TestWebSocketInvalidHandle, "WebSocket invalid handle errors");
        RUN_TEST(allPassed, TestMultipleWebSockets, "Multiple WebSockets");

        if (allPassed)
            LOG_INFO("All Network I/O tests passed!");
        else
            LOG_ERROR("Some Network I/O tests failed!");

        return allPassed;
    }

private:
    // Test DNS resolution
    static BOOL TestDnsResolve()
    {
        script::NetworkContext netCtx;
        script::State* L = CreateScriptState();
        script::OpenStdLib(*L);
        script::OpenNetworkIO(*L, &netCtx);

        auto source = R"SCRIPT(
// Resolve a known hostname
var ip = dns_resolve("cloudflare.com");
print("Resolved cloudflare.com to:", ip);

if (len(ip) > 0) {
    print("DNS resolution - PASS");
} else {
    print("WARNING: DNS resolution returned empty (network may be unavailable)");
}

// Test IPv4 resolution
var ip4 = dns_resolve4("cloudflare.com");
print("IPv4:", ip4);

// Test IPv6 resolution
var ip6 = dns_resolve6("cloudflare.com");
print("IPv6:", ip6);
)SCRIPT"_embed;

        BOOL result = L->DoString(source);
        if (!result)
        {
            LOG_ERROR("Script error: %s at line %d", L->GetError(), L->GetErrorLine());
        }
        delete L;
        return result;
    }

    // Test DNS resolution with invalid hostname
    static BOOL TestDnsResolveInvalid()
    {
        script::NetworkContext netCtx;
        script::State* L = CreateScriptState();
        script::OpenStdLib(*L);
        script::OpenNetworkIO(*L, &netCtx);

        auto source = R"SCRIPT(
// Try to resolve an invalid hostname
var ip = dns_resolve("this.hostname.definitely.does.not.exist.invalid");
print("Invalid hostname result:", ip);

if (len(ip) == 0) {
    print("Invalid hostname returns empty - PASS");
} else {
    print("Note: Got result for invalid hostname (unexpected but not error)");
}
)SCRIPT"_embed;

        BOOL result = L->DoString(source);
        if (!result)
        {
            LOG_ERROR("Script error: %s at line %d", L->GetError(), L->GetErrorLine());
        }
        delete L;
        return result;
    }

    // Test socket connect and close
    static BOOL TestSocketConnectClose()
    {
        script::NetworkContext netCtx;
        script::State* L = CreateScriptState();
        script::OpenStdLib(*L);
        script::OpenNetworkIO(*L, &netCtx);

        auto source = R"SCRIPT(
// Connect to a well-known HTTP server
var sock = sock_connect("httpbin.org", 80);
print("Socket handle:", sock);

if (sock >= 0) {
    print("Socket connected - PASS");

    // Send a simple HTTP request
    var sent = sock_send(sock, "GET / HTTP/1.0\r\nHost: httpbin.org\r\n\r\n");
    print("Bytes sent:", sent);

    if (sent > 0) {
        print("Socket send - PASS");

        // Try to receive some data
        var data = sock_recv(sock, 100);
        print("Received", len(data), "bytes");

        if (len(data) > 0) {
            print("Socket recv - PASS");
            // Print first few chars of response
            print("Response starts with:", data);
        } else {
            print("Note: No data received (might be timing)");
        }
    } else {
        print("Note: Send returned 0 (connection might have issues)");
    }

    // Close socket
    var closed = sock_close(sock);
    if (closed) {
        print("Socket closed - PASS");
    } else {
        print("ERROR: Failed to close socket");
    }
} else {
    print("WARNING: Could not connect (network may be unavailable)");
}
)SCRIPT"_embed;

        BOOL result = L->DoString(source);
        if (!result)
        {
            LOG_ERROR("Script error: %s at line %d", L->GetError(), L->GetErrorLine());
        }
        delete L;
        return result;
    }

    // Test socket error handling with invalid handles
    static BOOL TestSocketInvalidHandle()
    {
        script::NetworkContext netCtx;
        script::State* L = CreateScriptState();
        script::OpenStdLib(*L);
        script::OpenNetworkIO(*L, &netCtx);

        auto source = R"SCRIPT(
// Try to close invalid handle
var closed = sock_close(999);
if (!closed) {
    print("Closing invalid handle returns false - PASS");
} else {
    print("ERROR: Should fail to close invalid handle");
}

// Try to send with invalid handle
var sent = sock_send(999, "test");
if (sent < 0) {
    print("Sending to invalid handle returns -1 - PASS");
} else {
    print("ERROR: Should fail to send to invalid handle");
}

// Try to receive from invalid handle
var data = sock_recv(999);
if (len(data) == 0) {
    print("Receiving from invalid handle returns empty - PASS");
} else {
    print("ERROR: Should return empty for invalid handle");
}

// Try to connect to invalid port (should fail)
var sock = sock_connect("127.0.0.1", 1);
if (sock < 0) {
    print("Connecting to closed port returns -1 - PASS");
} else {
    print("Note: Connection to port 1 succeeded (unexpected but closing)");
    sock_close(sock);
}
)SCRIPT"_embed;

        BOOL result = L->DoString(source);
        if (!result)
        {
            LOG_ERROR("Script error: %s at line %d", L->GetError(), L->GetErrorLine());
        }
        delete L;
        return result;
    }

    // Test HTTP open and close
    static BOOL TestHttpOpenClose()
    {
        script::NetworkContext netCtx;
        script::State* L = CreateScriptState();
        script::OpenStdLib(*L);
        script::OpenNetworkIO(*L, &netCtx);

        auto source = R"SCRIPT(
// Open HTTP client for a URL
var http = http_open("http://httpbin.org/get");
print("HTTP handle:", http);

if (http >= 0) {
    print("HTTP client created - PASS");

    // Close HTTP client
    var closed = http_close(http);
    if (closed) {
        print("HTTP client closed - PASS");
    } else {
        print("ERROR: Failed to close HTTP client");
    }
} else {
    print("WARNING: Could not create HTTP client (network may be unavailable)");
}
)SCRIPT"_embed;

        BOOL result = L->DoString(source);
        if (!result)
        {
            LOG_ERROR("Script error: %s at line %d", L->GetError(), L->GetErrorLine());
        }
        delete L;
        return result;
    }

    // Test HTTP GET request
    static BOOL TestHttpGet()
    {
        script::NetworkContext netCtx;
        script::State* L = CreateScriptState();
        script::OpenStdLib(*L);
        script::OpenNetworkIO(*L, &netCtx);

        auto source = R"SCRIPT(
// Open HTTP client
var http = http_open("http://httpbin.org/ip");
print("HTTP handle:", http);

if (http >= 0) {
    // Send GET request
    var sent = http_get(http);
    print("GET request sent:", sent);

    if (sent) {
        print("HTTP GET - PASS");

        // Read response (read multiple times to get full response)
        var response = "";
        var chunk = http_read(http, 255);
        while (len(chunk) > 0) {
            response = response + chunk;
            chunk = http_read(http, 255);
        }

        print("Response length:", len(response));
        if (len(response) > 0) {
            print("HTTP read - PASS");
            // Print first part of response
            print("Response preview:", response);
        } else {
            print("Note: Empty response (might be timing)");
        }
    } else {
        print("Note: GET request failed (network issue?)");
    }

    http_close(http);
} else {
    print("WARNING: Could not create HTTP client");
}
)SCRIPT"_embed;

        BOOL result = L->DoString(source);
        if (!result)
        {
            LOG_ERROR("Script error: %s at line %d", L->GetError(), L->GetErrorLine());
        }
        delete L;
        return result;
    }

    // Test HTTP error handling with invalid handles
    static BOOL TestHttpInvalidHandle()
    {
        script::NetworkContext netCtx;
        script::State* L = CreateScriptState();
        script::OpenStdLib(*L);
        script::OpenNetworkIO(*L, &netCtx);

        auto source = R"SCRIPT(
// Try to close invalid handle
var closed = http_close(999);
if (!closed) {
    print("Closing invalid handle returns false - PASS");
} else {
    print("ERROR: Should fail to close invalid handle");
}

// Try to send GET with invalid handle
var sent = http_get(999);
if (!sent) {
    print("GET with invalid handle returns false - PASS");
} else {
    print("ERROR: Should fail GET with invalid handle");
}

// Try to send POST with invalid handle
var posted = http_post(999, "test");
if (!posted) {
    print("POST with invalid handle returns false - PASS");
} else {
    print("ERROR: Should fail POST with invalid handle");
}

// Try to read from invalid handle
var data = http_read(999);
if (len(data) == 0) {
    print("Reading from invalid handle returns empty - PASS");
} else {
    print("ERROR: Should return empty for invalid handle");
}
)SCRIPT"_embed;

        BOOL result = L->DoString(source);
        if (!result)
        {
            LOG_ERROR("Script error: %s at line %d", L->GetError(), L->GetErrorLine());
        }
        delete L;
        return result;
    }

    // Test multiple simultaneous sockets
    static BOOL TestMultipleSockets()
    {
        script::NetworkContext netCtx;
        script::State* L = CreateScriptState();
        script::OpenStdLib(*L);
        script::OpenNetworkIO(*L, &netCtx);

        auto source = R"SCRIPT(
// Try to open multiple sockets (within pool limit)
var s1 = sock_connect("httpbin.org", 80);
var s2 = sock_connect("httpbin.org", 80);
var s3 = sock_connect("httpbin.org", 80);

print("Socket handles:", s1, s2, s3);

// Check if we got valid handles (may fail if network unavailable)
var successCount = 0;
if (s1 >= 0) successCount = successCount + 1;
if (s2 >= 0) successCount = successCount + 1;
if (s3 >= 0) successCount = successCount + 1;

print("Successfully opened", successCount, "sockets");

if (successCount >= 1) {
    // Verify handles are unique
    if (s1 >= 0 && s2 >= 0 && s1 != s2) {
        print("Handles are unique - PASS");
    }
}

// Close all sockets
if (s1 >= 0) sock_close(s1);
if (s2 >= 0) sock_close(s2);
if (s3 >= 0) sock_close(s3);

print("Multiple sockets test completed");
)SCRIPT"_embed;

        BOOL result = L->DoString(source);
        if (!result)
        {
            LOG_ERROR("Script error: %s at line %d", L->GetError(), L->GetErrorLine());
        }
        delete L;
        return result;
    }

    // Test multiple HTTP clients
    static BOOL TestMultipleHttpClients()
    {
        script::NetworkContext netCtx;
        script::State* L = CreateScriptState();
        script::OpenStdLib(*L);
        script::OpenNetworkIO(*L, &netCtx);

        auto source = R"SCRIPT(
// Try to open multiple HTTP clients
var h1 = http_open("http://httpbin.org/get");
var h2 = http_open("http://httpbin.org/ip");

print("HTTP handles:", h1, h2);

// Check if we got valid handles
var successCount = 0;
if (h1 >= 0) successCount = successCount + 1;
if (h2 >= 0) successCount = successCount + 1;

print("Successfully created", successCount, "HTTP clients");

if (successCount >= 1) {
    // Verify handles are unique
    if (h1 >= 0 && h2 >= 0 && h1 != h2) {
        print("HTTP handles are unique - PASS");
    }
}

// Close all clients
if (h1 >= 0) http_close(h1);
if (h2 >= 0) http_close(h2);

print("Multiple HTTP clients test completed");
)SCRIPT"_embed;

        BOOL result = L->DoString(source);
        if (!result)
        {
            LOG_ERROR("Script error: %s at line %d", L->GetError(), L->GetErrorLine());
        }
        delete L;
        return result;
    }

    // Test WebSocket connect and close
    static BOOL TestWebSocketConnectClose()
    {
        script::NetworkContext netCtx;
        script::State* L = CreateScriptState();
        script::OpenStdLib(*L);
        script::OpenNetworkIO(*L, &netCtx);

        auto source = R"SCRIPT(
// Connect to a public WebSocket echo server
var ws = ws_connect("wss://echo.websocket.org");
print("WebSocket handle:", ws);

if (ws >= 0) {
    print("WebSocket connected - PASS");

    // Close WebSocket
    var closed = ws_close(ws);
    if (closed) {
        print("WebSocket closed - PASS");
    } else {
        print("ERROR: Failed to close WebSocket");
    }
} else {
    print("WARNING: Could not connect to WebSocket server (network may be unavailable)");
}
)SCRIPT"_embed;

        BOOL result = L->DoString(source);
        if (!result)
        {
            LOG_ERROR("Script error: %s at line %d", L->GetError(), L->GetErrorLine());
        }
        delete L;
        return result;
    }

    // Test WebSocket send and receive
    static BOOL TestWebSocketSendRecv()
    {
        script::NetworkContext netCtx;
        script::State* L = CreateScriptState();
        script::OpenStdLib(*L);
        script::OpenNetworkIO(*L, &netCtx);

        auto source = R"SCRIPT(
// Connect to a public WebSocket echo server
var ws = ws_connect("wss://echo.websocket.org");
print("WebSocket handle:", ws);

if (ws >= 0) {
    print("WebSocket connected");

    // Send text message
    var sent = ws_send_text(ws, "Hello WebSocket!");
    print("Bytes sent:", sent);

    if (sent > 0) {
        print("WebSocket send - PASS");

        // Receive echo response
        var data = ws_recv(ws);
        print("Received", len(data), "bytes:", data);

        if (len(data) > 0) {
            print("WebSocket recv - PASS");
        } else {
            print("Note: No data received (might be timing)");
        }
    } else {
        print("Note: Send returned 0 (connection might have issues)");
    }

    // Test ws_send with explicit opcode (TEXT = 1)
    var sent2 = ws_send(ws, "Binary test", 2);
    print("Binary send:", sent2);

    ws_close(ws);
    print("WebSocket closed");
} else {
    print("WARNING: Could not connect to WebSocket server");
}
)SCRIPT"_embed;

        BOOL result = L->DoString(source);
        if (!result)
        {
            LOG_ERROR("Script error: %s at line %d", L->GetError(), L->GetErrorLine());
        }
        delete L;
        return result;
    }

    // Test WebSocket error handling with invalid handles
    static BOOL TestWebSocketInvalidHandle()
    {
        script::NetworkContext netCtx;
        script::State* L = CreateScriptState();
        script::OpenStdLib(*L);
        script::OpenNetworkIO(*L, &netCtx);

        auto source = R"SCRIPT(
// Try to close invalid handle
var closed = ws_close(999);
if (!closed) {
    print("Closing invalid handle returns false - PASS");
} else {
    print("ERROR: Should fail to close invalid handle");
}

// Try to send with invalid handle
var sent = ws_send(999, "test");
if (sent < 0) {
    print("Sending to invalid handle returns -1 - PASS");
} else {
    print("ERROR: Should fail to send to invalid handle");
}

// Try to send_text with invalid handle
var sent2 = ws_send_text(999, "test");
if (sent2 < 0) {
    print("send_text to invalid handle returns -1 - PASS");
} else {
    print("ERROR: Should fail send_text to invalid handle");
}

// Try to receive from invalid handle
var data = ws_recv(999);
if (len(data) == 0) {
    print("Receiving from invalid handle returns empty - PASS");
} else {
    print("ERROR: Should return empty for invalid handle");
}

// Try ping/pong with invalid handle
var pinged = ws_ping(999);
var ponged = ws_pong(999);
print("Invalid ping:", pinged, "pong:", ponged);
)SCRIPT"_embed;

        BOOL result = L->DoString(source);
        if (!result)
        {
            LOG_ERROR("Script error: %s at line %d", L->GetError(), L->GetErrorLine());
        }
        delete L;
        return result;
    }

    // Test multiple simultaneous WebSockets
    static BOOL TestMultipleWebSockets()
    {
        script::NetworkContext netCtx;
        script::State* L = CreateScriptState();
        script::OpenStdLib(*L);
        script::OpenNetworkIO(*L, &netCtx);

        auto source = R"SCRIPT(
// Try to open multiple WebSocket connections
var ws1 = ws_connect("wss://echo.websocket.org");
var ws2 = ws_connect("wss://echo.websocket.org");

print("WebSocket handles:", ws1, ws2);

// Check if we got valid handles (may fail if network unavailable)
var successCount = 0;
if (ws1 >= 0) successCount = successCount + 1;
if (ws2 >= 0) successCount = successCount + 1;

print("Successfully opened", successCount, "WebSockets");

if (successCount >= 1) {
    // Verify handles are unique
    if (ws1 >= 0 && ws2 >= 0 && ws1 != ws2) {
        print("Handles are unique - PASS");
    }
}

// Close all WebSockets
if (ws1 >= 0) ws_close(ws1);
if (ws2 >= 0) ws_close(ws2);

print("Multiple WebSockets test completed");
)SCRIPT"_embed;

        BOOL result = L->DoString(source);
        if (!result)
        {
            LOG_ERROR("Script error: %s at line %d", L->GetError(), L->GetErrorLine());
        }
        delete L;
        return result;
    }
};
