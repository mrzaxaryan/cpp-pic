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
        RUN_TEST(allPassed, TestDnsResolve4, "DNS resolve IPv4");
        RUN_TEST(allPassed, TestDnsResolve6, "DNS resolve IPv6");
        RUN_TEST(allPassed, TestDnsResolveInvalid, "DNS resolve invalid");
        RUN_TEST(allPassed, TestSocketConnect, "Socket connect");
        RUN_TEST(allPassed, TestSocketSendRecv, "Socket send/recv");
        RUN_TEST(allPassed, TestSocketClose, "Socket close");
        RUN_TEST(allPassed, TestSocketInvalidClose, "Socket invalid close");
        RUN_TEST(allPassed, TestSocketInvalidSend, "Socket invalid send");
        RUN_TEST(allPassed, TestSocketInvalidRecv, "Socket invalid recv");
        RUN_TEST(allPassed, TestSocketInvalidPort, "Socket invalid port");
        RUN_TEST(allPassed, TestHttpOpen, "HTTP open");
        RUN_TEST(allPassed, TestHttpGet, "HTTP GET");
        RUN_TEST(allPassed, TestHttpClose, "HTTP close");
        RUN_TEST(allPassed, TestHttpInvalidClose, "HTTP invalid close");
        RUN_TEST(allPassed, TestHttpInvalidGet, "HTTP invalid GET");
        RUN_TEST(allPassed, TestHttpInvalidPost, "HTTP invalid POST");
        RUN_TEST(allPassed, TestHttpInvalidRead, "HTTP invalid read");
        RUN_TEST(allPassed, TestMultipleSockets, "Multiple sockets");
        RUN_TEST(allPassed, TestMultipleHttp, "Multiple HTTP");
        RUN_TEST(allPassed, TestWsConnect, "WebSocket connect");
        RUN_TEST(allPassed, TestWsSendRecv, "WebSocket send/recv");
        RUN_TEST(allPassed, TestWsClose, "WebSocket close");
        RUN_TEST(allPassed, TestWsInvalidClose, "WebSocket invalid close");
        RUN_TEST(allPassed, TestWsInvalidSend, "WebSocket invalid send");
        RUN_TEST(allPassed, TestWsInvalidRecv, "WebSocket invalid recv");
        RUN_TEST(allPassed, TestMultipleWs, "Multiple WebSockets");

        if (allPassed)
            LOG_INFO("All Network I/O tests passed!");
        else
            LOG_ERROR("Some Network I/O tests failed!");

        return allPassed;
    }

private:
    // ========================================================================
    // DNS TESTS
    // ========================================================================

    static BOOL TestDnsResolve()
    {
        script::NetworkContext netCtx;
        script::State* L = CreateScriptState();
        script::OpenStdLib(*L);
        script::OpenNetworkIO(*L, &netCtx);

        BOOL result = L->DoString("var ip = dns_resolve(\"cloudflare.com\");"_embed);
        result = result && L->DoString("print(\"Resolved:\", ip);"_embed);
        result = result && L->DoString("if (len(ip) > 0) { print(\"PASS\"); }"_embed);

        if (!result)
            LOG_ERROR("Script error: %s", L->GetError());
        delete L;
        return result;
    }

    static BOOL TestDnsResolve4()
    {
        script::NetworkContext netCtx;
        script::State* L = CreateScriptState();
        script::OpenStdLib(*L);
        script::OpenNetworkIO(*L, &netCtx);

        BOOL result = L->DoString("var ip4 = dns_resolve4(\"cloudflare.com\");"_embed);
        result = result && L->DoString("print(\"IPv4:\", ip4);"_embed);

        if (!result)
            LOG_ERROR("Script error: %s", L->GetError());
        delete L;
        return result;
    }

    static BOOL TestDnsResolve6()
    {
        script::NetworkContext netCtx;
        script::State* L = CreateScriptState();
        script::OpenStdLib(*L);
        script::OpenNetworkIO(*L, &netCtx);

        BOOL result = L->DoString("var ip6 = dns_resolve6(\"cloudflare.com\");"_embed);
        result = result && L->DoString("print(\"IPv6:\", ip6);"_embed);

        if (!result)
            LOG_ERROR("Script error: %s", L->GetError());
        delete L;
        return result;
    }

    static BOOL TestDnsResolveInvalid()
    {
        script::NetworkContext netCtx;
        script::State* L = CreateScriptState();
        script::OpenStdLib(*L);
        script::OpenNetworkIO(*L, &netCtx);

        BOOL result = L->DoString("var ip = dns_resolve(\"invalid.invalid\");"_embed);
        result = result && L->DoString("print(\"Invalid result:\", ip);"_embed);
        result = result && L->DoString("if (len(ip) == 0) { print(\"PASS\"); }"_embed);

        if (!result)
            LOG_ERROR("Script error: %s", L->GetError());
        delete L;
        return result;
    }

    // ========================================================================
    // SOCKET TESTS
    // ========================================================================

    static BOOL TestSocketConnect()
    {
        script::NetworkContext netCtx;
        script::State* L = CreateScriptState();
        script::OpenStdLib(*L);
        script::OpenNetworkIO(*L, &netCtx);

        BOOL result = L->DoString("var sock = sock_connect(\"httpbin.org\", 80);"_embed);
        result = result && L->DoString("print(\"Socket handle:\", sock);"_embed);
        result = result && L->DoString("if (sock >= 0) { print(\"PASS\"); }"_embed);
        result = result && L->DoString("if (sock >= 0) { sock_close(sock); }"_embed);

        if (!result)
            LOG_ERROR("Script error: %s", L->GetError());
        delete L;
        return result;
    }

    static BOOL TestSocketSendRecv()
    {
        script::NetworkContext netCtx;
        script::State* L = CreateScriptState();
        script::OpenStdLib(*L);
        script::OpenNetworkIO(*L, &netCtx);

        BOOL result = L->DoString("var sock = sock_connect(\"httpbin.org\", 80);"_embed);
        result = result && L->DoString("var req = \"GET / HTTP/1.0\\r\\nHost: httpbin.org\\r\\n\\r\\n\";"_embed);
        result = result && L->DoString("var sent = sock_send(sock, req);"_embed);
        result = result && L->DoString("print(\"Sent:\", sent);"_embed);
        result = result && L->DoString("var data = sock_recv(sock, 100);"_embed);
        result = result && L->DoString("print(\"Received:\", len(data), \"bytes\");"_embed);
        result = result && L->DoString("if (len(data) > 0) { print(\"PASS\"); }"_embed);
        result = result && L->DoString("sock_close(sock);"_embed);

        if (!result)
            LOG_ERROR("Script error: %s", L->GetError());
        delete L;
        return result;
    }

    static BOOL TestSocketClose()
    {
        script::NetworkContext netCtx;
        script::State* L = CreateScriptState();
        script::OpenStdLib(*L);
        script::OpenNetworkIO(*L, &netCtx);

        BOOL result = L->DoString("var sock = sock_connect(\"httpbin.org\", 80);"_embed);
        result = result && L->DoString("var closed = sock_close(sock);"_embed);
        result = result && L->DoString("if (closed) { print(\"PASS\"); }"_embed);

        if (!result)
            LOG_ERROR("Script error: %s", L->GetError());
        delete L;
        return result;
    }

    static BOOL TestSocketInvalidClose()
    {
        script::NetworkContext netCtx;
        script::State* L = CreateScriptState();
        script::OpenStdLib(*L);
        script::OpenNetworkIO(*L, &netCtx);

        BOOL result = L->DoString("var closed = sock_close(999);"_embed);
        result = result && L->DoString("if (!closed) { print(\"PASS\"); }"_embed);

        if (!result)
            LOG_ERROR("Script error: %s", L->GetError());
        delete L;
        return result;
    }

    static BOOL TestSocketInvalidSend()
    {
        script::NetworkContext netCtx;
        script::State* L = CreateScriptState();
        script::OpenStdLib(*L);
        script::OpenNetworkIO(*L, &netCtx);

        BOOL result = L->DoString("var sent = sock_send(999, \"test\");"_embed);
        result = result && L->DoString("if (sent < 0) { print(\"PASS\"); }"_embed);

        if (!result)
            LOG_ERROR("Script error: %s", L->GetError());
        delete L;
        return result;
    }

    static BOOL TestSocketInvalidRecv()
    {
        script::NetworkContext netCtx;
        script::State* L = CreateScriptState();
        script::OpenStdLib(*L);
        script::OpenNetworkIO(*L, &netCtx);

        BOOL result = L->DoString("var data = sock_recv(999);"_embed);
        result = result && L->DoString("if (len(data) == 0) { print(\"PASS\"); }"_embed);

        if (!result)
            LOG_ERROR("Script error: %s", L->GetError());
        delete L;
        return result;
    }

    static BOOL TestSocketInvalidPort()
    {
        script::NetworkContext netCtx;
        script::State* L = CreateScriptState();
        script::OpenStdLib(*L);
        script::OpenNetworkIO(*L, &netCtx);

        BOOL result = L->DoString("var sock = sock_connect(\"127.0.0.1\", 1);"_embed);
        result = result && L->DoString("if (sock < 0) { print(\"PASS\"); }"_embed);
        result = result && L->DoString("if (sock >= 0) { sock_close(sock); }"_embed);

        if (!result)
            LOG_ERROR("Script error: %s", L->GetError());
        delete L;
        return result;
    }

    // ========================================================================
    // HTTP TESTS
    // ========================================================================

    static BOOL TestHttpOpen()
    {
        script::NetworkContext netCtx;
        script::State* L = CreateScriptState();
        script::OpenStdLib(*L);
        script::OpenNetworkIO(*L, &netCtx);

        BOOL result = L->DoString("var http = http_open(\"http://httpbin.org/get\");"_embed);
        result = result && L->DoString("print(\"HTTP handle:\", http);"_embed);
        result = result && L->DoString("if (http >= 0) { print(\"PASS\"); }"_embed);
        result = result && L->DoString("if (http >= 0) { http_close(http); }"_embed);

        if (!result)
            LOG_ERROR("Script error: %s", L->GetError());
        delete L;
        return result;
    }

    static BOOL TestHttpGet()
    {
        script::NetworkContext netCtx;
        script::State* L = CreateScriptState();
        script::OpenStdLib(*L);
        script::OpenNetworkIO(*L, &netCtx);

        BOOL result = L->DoString("var http = http_open(\"http://httpbin.org/ip\");"_embed);
        result = result && L->DoString("var sent = http_get(http);"_embed);
        result = result && L->DoString("print(\"GET sent:\", sent);"_embed);
        result = result && L->DoString("var resp = http_read(http, 255);"_embed);
        result = result && L->DoString("print(\"Response:\", len(resp), \"bytes\");"_embed);
        result = result && L->DoString("if (len(resp) > 0) { print(\"PASS\"); }"_embed);
        result = result && L->DoString("http_close(http);"_embed);

        if (!result)
            LOG_ERROR("Script error: %s", L->GetError());
        delete L;
        return result;
    }

    static BOOL TestHttpClose()
    {
        script::NetworkContext netCtx;
        script::State* L = CreateScriptState();
        script::OpenStdLib(*L);
        script::OpenNetworkIO(*L, &netCtx);

        BOOL result = L->DoString("var http = http_open(\"http://httpbin.org/get\");"_embed);
        result = result && L->DoString("var closed = http_close(http);"_embed);
        result = result && L->DoString("if (closed) { print(\"PASS\"); }"_embed);

        if (!result)
            LOG_ERROR("Script error: %s", L->GetError());
        delete L;
        return result;
    }

    static BOOL TestHttpInvalidClose()
    {
        script::NetworkContext netCtx;
        script::State* L = CreateScriptState();
        script::OpenStdLib(*L);
        script::OpenNetworkIO(*L, &netCtx);

        BOOL result = L->DoString("var closed = http_close(999);"_embed);
        result = result && L->DoString("if (!closed) { print(\"PASS\"); }"_embed);

        if (!result)
            LOG_ERROR("Script error: %s", L->GetError());
        delete L;
        return result;
    }

    static BOOL TestHttpInvalidGet()
    {
        script::NetworkContext netCtx;
        script::State* L = CreateScriptState();
        script::OpenStdLib(*L);
        script::OpenNetworkIO(*L, &netCtx);

        BOOL result = L->DoString("var sent = http_get(999);"_embed);
        result = result && L->DoString("if (!sent) { print(\"PASS\"); }"_embed);

        if (!result)
            LOG_ERROR("Script error: %s", L->GetError());
        delete L;
        return result;
    }

    static BOOL TestHttpInvalidPost()
    {
        script::NetworkContext netCtx;
        script::State* L = CreateScriptState();
        script::OpenStdLib(*L);
        script::OpenNetworkIO(*L, &netCtx);

        BOOL result = L->DoString("var posted = http_post(999, \"test\");"_embed);
        result = result && L->DoString("if (!posted) { print(\"PASS\"); }"_embed);

        if (!result)
            LOG_ERROR("Script error: %s", L->GetError());
        delete L;
        return result;
    }

    static BOOL TestHttpInvalidRead()
    {
        script::NetworkContext netCtx;
        script::State* L = CreateScriptState();
        script::OpenStdLib(*L);
        script::OpenNetworkIO(*L, &netCtx);

        BOOL result = L->DoString("var data = http_read(999);"_embed);
        result = result && L->DoString("if (len(data) == 0) { print(\"PASS\"); }"_embed);

        if (!result)
            LOG_ERROR("Script error: %s", L->GetError());
        delete L;
        return result;
    }

    // ========================================================================
    // MULTIPLE HANDLES TESTS
    // ========================================================================

    static BOOL TestMultipleSockets()
    {
        script::NetworkContext netCtx;
        script::State* L = CreateScriptState();
        script::OpenStdLib(*L);
        script::OpenNetworkIO(*L, &netCtx);

        BOOL result = L->DoString("var s1 = sock_connect(\"httpbin.org\", 80);"_embed);
        result = result && L->DoString("var s2 = sock_connect(\"httpbin.org\", 80);"_embed);
        result = result && L->DoString("print(\"Handles:\", s1, s2);"_embed);
        result = result && L->DoString("if (s1 >= 0 && s2 >= 0 && s1 != s2) { print(\"PASS\"); }"_embed);
        result = result && L->DoString("if (s1 >= 0) { sock_close(s1); }"_embed);
        result = result && L->DoString("if (s2 >= 0) { sock_close(s2); }"_embed);

        if (!result)
            LOG_ERROR("Script error: %s", L->GetError());
        delete L;
        return result;
    }

    static BOOL TestMultipleHttp()
    {
        script::NetworkContext netCtx;
        script::State* L = CreateScriptState();
        script::OpenStdLib(*L);
        script::OpenNetworkIO(*L, &netCtx);

        BOOL result = L->DoString("var h1 = http_open(\"http://httpbin.org/get\");"_embed);
        result = result && L->DoString("var h2 = http_open(\"http://httpbin.org/ip\");"_embed);
        result = result && L->DoString("print(\"Handles:\", h1, h2);"_embed);
        result = result && L->DoString("if (h1 >= 0 && h2 >= 0 && h1 != h2) { print(\"PASS\"); }"_embed);
        result = result && L->DoString("if (h1 >= 0) { http_close(h1); }"_embed);
        result = result && L->DoString("if (h2 >= 0) { http_close(h2); }"_embed);

        if (!result)
            LOG_ERROR("Script error: %s", L->GetError());
        delete L;
        return result;
    }

    // ========================================================================
    // WEBSOCKET TESTS
    // ========================================================================

    static BOOL TestWsConnect()
    {
        script::NetworkContext netCtx;
        script::State* L = CreateScriptState();
        script::OpenStdLib(*L);
        script::OpenNetworkIO(*L, &netCtx);

        BOOL result = L->DoString("var ws = ws_connect(\"wss://echo.websocket.org\");"_embed);
        result = result && L->DoString("print(\"WebSocket handle:\", ws);"_embed);
        result = result && L->DoString("if (ws >= 0) { print(\"PASS\"); }"_embed);
        result = result && L->DoString("if (ws >= 0) { ws_close(ws); }"_embed);

        if (!result)
            LOG_ERROR("Script error: %s", L->GetError());
        delete L;
        return result;
    }

    static BOOL TestWsSendRecv()
    {
        script::NetworkContext netCtx;
        script::State* L = CreateScriptState();
        script::OpenStdLib(*L);
        script::OpenNetworkIO(*L, &netCtx);

        BOOL result = L->DoString("var ws = ws_connect(\"wss://echo.websocket.org\");"_embed);
        result = result && L->DoString("var sent = ws_send_text(ws, \"Hello\");"_embed);
        result = result && L->DoString("print(\"Sent:\", sent);"_embed);
        result = result && L->DoString("var data = ws_recv(ws);"_embed);
        result = result && L->DoString("print(\"Received:\", len(data), \"bytes\");"_embed);
        result = result && L->DoString("if (len(data) > 0) { print(\"PASS\"); }"_embed);
        result = result && L->DoString("ws_close(ws);"_embed);

        if (!result)
            LOG_ERROR("Script error: %s", L->GetError());
        delete L;
        return result;
    }

    static BOOL TestWsClose()
    {
        script::NetworkContext netCtx;
        script::State* L = CreateScriptState();
        script::OpenStdLib(*L);
        script::OpenNetworkIO(*L, &netCtx);

        BOOL result = L->DoString("var ws = ws_connect(\"wss://echo.websocket.org\");"_embed);
        result = result && L->DoString("var closed = ws_close(ws);"_embed);
        result = result && L->DoString("if (closed) { print(\"PASS\"); }"_embed);

        if (!result)
            LOG_ERROR("Script error: %s", L->GetError());
        delete L;
        return result;
    }

    static BOOL TestWsInvalidClose()
    {
        script::NetworkContext netCtx;
        script::State* L = CreateScriptState();
        script::OpenStdLib(*L);
        script::OpenNetworkIO(*L, &netCtx);

        BOOL result = L->DoString("var closed = ws_close(999);"_embed);
        result = result && L->DoString("if (!closed) { print(\"PASS\"); }"_embed);

        if (!result)
            LOG_ERROR("Script error: %s", L->GetError());
        delete L;
        return result;
    }

    static BOOL TestWsInvalidSend()
    {
        script::NetworkContext netCtx;
        script::State* L = CreateScriptState();
        script::OpenStdLib(*L);
        script::OpenNetworkIO(*L, &netCtx);

        BOOL result = L->DoString("var sent = ws_send(999, \"test\");"_embed);
        result = result && L->DoString("if (sent < 0) { print(\"PASS\"); }"_embed);

        if (!result)
            LOG_ERROR("Script error: %s", L->GetError());
        delete L;
        return result;
    }

    static BOOL TestWsInvalidRecv()
    {
        script::NetworkContext netCtx;
        script::State* L = CreateScriptState();
        script::OpenStdLib(*L);
        script::OpenNetworkIO(*L, &netCtx);

        BOOL result = L->DoString("var data = ws_recv(999);"_embed);
        result = result && L->DoString("if (len(data) == 0) { print(\"PASS\"); }"_embed);

        if (!result)
            LOG_ERROR("Script error: %s", L->GetError());
        delete L;
        return result;
    }

    static BOOL TestMultipleWs()
    {
        script::NetworkContext netCtx;
        script::State* L = CreateScriptState();
        script::OpenStdLib(*L);
        script::OpenNetworkIO(*L, &netCtx);

        BOOL result = L->DoString("var ws1 = ws_connect(\"wss://echo.websocket.org\");"_embed);
        result = result && L->DoString("var ws2 = ws_connect(\"wss://echo.websocket.org\");"_embed);
        result = result && L->DoString("print(\"Handles:\", ws1, ws2);"_embed);
        result = result && L->DoString("if (ws1 >= 0 && ws2 >= 0 && ws1 != ws2) { print(\"PASS\"); }"_embed);
        result = result && L->DoString("if (ws1 >= 0) { ws_close(ws1); }"_embed);
        result = result && L->DoString("if (ws2 >= 0) { ws_close(ws2); }"_embed);

        if (!result)
            LOG_ERROR("Script error: %s", L->GetError());
        delete L;
        return result;
    }
};
