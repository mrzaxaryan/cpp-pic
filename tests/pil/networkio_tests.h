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

        BOOL result = RunScriptFile(L, L"tests/pil/scripts/networkio/dns_resolve.pil");
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

        BOOL result = RunScriptFile(L, L"tests/pil/scripts/networkio/dns_resolve4.pil");
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

        BOOL result = RunScriptFile(L, L"tests/pil/scripts/networkio/dns_resolve6.pil");
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

        BOOL result = RunScriptFile(L, L"tests/pil/scripts/networkio/dns_resolve_invalid.pil");
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

        BOOL result = RunScriptFile(L, L"tests/pil/scripts/networkio/socket_connect.pil");
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

        BOOL result = RunScriptFile(L, L"tests/pil/scripts/networkio/socket_send_recv.pil");
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

        BOOL result = RunScriptFile(L, L"tests/pil/scripts/networkio/socket_close.pil");
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

        BOOL result = RunScriptFile(L, L"tests/pil/scripts/networkio/socket_invalid_close.pil");
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

        BOOL result = RunScriptFile(L, L"tests/pil/scripts/networkio/socket_invalid_send.pil");
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

        BOOL result = RunScriptFile(L, L"tests/pil/scripts/networkio/socket_invalid_recv.pil");
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

        BOOL result = RunScriptFile(L, L"tests/pil/scripts/networkio/socket_invalid_port.pil");
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

        BOOL result = RunScriptFile(L, L"tests/pil/scripts/networkio/http_open.pil");
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

        BOOL result = RunScriptFile(L, L"tests/pil/scripts/networkio/http_get.pil");
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

        BOOL result = RunScriptFile(L, L"tests/pil/scripts/networkio/http_close.pil");
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

        BOOL result = RunScriptFile(L, L"tests/pil/scripts/networkio/http_invalid_close.pil");
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

        BOOL result = RunScriptFile(L, L"tests/pil/scripts/networkio/http_invalid_get.pil");
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

        BOOL result = RunScriptFile(L, L"tests/pil/scripts/networkio/http_invalid_post.pil");
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

        BOOL result = RunScriptFile(L, L"tests/pil/scripts/networkio/http_invalid_read.pil");
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

        BOOL result = RunScriptFile(L, L"tests/pil/scripts/networkio/multiple_sockets.pil");
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

        BOOL result = RunScriptFile(L, L"tests/pil/scripts/networkio/multiple_http.pil");
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

        BOOL result = RunScriptFile(L, L"tests/pil/scripts/networkio/ws_connect.pil");
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

        BOOL result = RunScriptFile(L, L"tests/pil/scripts/networkio/ws_send_recv.pil");
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

        BOOL result = RunScriptFile(L, L"tests/pil/scripts/networkio/ws_close.pil");
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

        BOOL result = RunScriptFile(L, L"tests/pil/scripts/networkio/ws_invalid_close.pil");
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

        BOOL result = RunScriptFile(L, L"tests/pil/scripts/networkio/ws_invalid_send.pil");
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

        BOOL result = RunScriptFile(L, L"tests/pil/scripts/networkio/ws_invalid_recv.pil");
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

        BOOL result = RunScriptFile(L, L"tests/pil/scripts/networkio/multiple_ws.pil");
        if (!result)
            LOG_ERROR("Script error: %s", L->GetError());
        delete L;
        return result;
    }
};
