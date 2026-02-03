#pragma once

#include "test_framework.h"

// ============================================================================
// NETWORK I/O TESTS
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

        // DNS tests
        RUN_SCRIPT_TEST_NETWORKIO(allPassed, L"tests/pil/scripts/networkio/dns_resolve.pil"_embed,         "DNS resolve");
        RUN_SCRIPT_TEST_NETWORKIO(allPassed, L"tests/pil/scripts/networkio/dns_resolve4.pil"_embed,        "DNS resolve IPv4");
        RUN_SCRIPT_TEST_NETWORKIO(allPassed, L"tests/pil/scripts/networkio/dns_resolve6.pil"_embed,        "DNS resolve IPv6");
        RUN_SCRIPT_TEST_NETWORKIO(allPassed, L"tests/pil/scripts/networkio/dns_resolve_invalid.pil"_embed, "DNS resolve invalid");

        // Socket tests
        RUN_SCRIPT_TEST_NETWORKIO(allPassed, L"tests/pil/scripts/networkio/socket_connect.pil"_embed,      "Socket connect");
        RUN_SCRIPT_TEST_NETWORKIO(allPassed, L"tests/pil/scripts/networkio/socket_send_recv.pil"_embed,    "Socket send/recv");
        RUN_SCRIPT_TEST_NETWORKIO(allPassed, L"tests/pil/scripts/networkio/socket_close.pil"_embed,        "Socket close");
        RUN_SCRIPT_TEST_NETWORKIO(allPassed, L"tests/pil/scripts/networkio/socket_invalid_close.pil"_embed,"Socket invalid close");
        RUN_SCRIPT_TEST_NETWORKIO(allPassed, L"tests/pil/scripts/networkio/socket_invalid_send.pil"_embed, "Socket invalid send");
        RUN_SCRIPT_TEST_NETWORKIO(allPassed, L"tests/pil/scripts/networkio/socket_invalid_recv.pil"_embed, "Socket invalid recv");
        RUN_SCRIPT_TEST_NETWORKIO(allPassed, L"tests/pil/scripts/networkio/socket_invalid_port.pil"_embed, "Socket invalid port");

        // HTTP tests
        RUN_SCRIPT_TEST_NETWORKIO(allPassed, L"tests/pil/scripts/networkio/http_open.pil"_embed,           "HTTP open");
        RUN_SCRIPT_TEST_NETWORKIO(allPassed, L"tests/pil/scripts/networkio/http_get.pil"_embed,            "HTTP GET");
        RUN_SCRIPT_TEST_NETWORKIO(allPassed, L"tests/pil/scripts/networkio/http_close.pil"_embed,          "HTTP close");
        RUN_SCRIPT_TEST_NETWORKIO(allPassed, L"tests/pil/scripts/networkio/http_invalid_close.pil"_embed,  "HTTP invalid close");
        RUN_SCRIPT_TEST_NETWORKIO(allPassed, L"tests/pil/scripts/networkio/http_invalid_get.pil"_embed,    "HTTP invalid GET");
        RUN_SCRIPT_TEST_NETWORKIO(allPassed, L"tests/pil/scripts/networkio/http_invalid_post.pil"_embed,   "HTTP invalid POST");
        RUN_SCRIPT_TEST_NETWORKIO(allPassed, L"tests/pil/scripts/networkio/http_invalid_read.pil"_embed,   "HTTP invalid read");

        // Multiple handles tests
        RUN_SCRIPT_TEST_NETWORKIO(allPassed, L"tests/pil/scripts/networkio/multiple_sockets.pil"_embed,    "Multiple sockets");
        RUN_SCRIPT_TEST_NETWORKIO(allPassed, L"tests/pil/scripts/networkio/multiple_http.pil"_embed,       "Multiple HTTP");

        // WebSocket tests
        RUN_SCRIPT_TEST_NETWORKIO(allPassed, L"tests/pil/scripts/networkio/ws_connect.pil"_embed,          "WebSocket connect");
        RUN_SCRIPT_TEST_NETWORKIO(allPassed, L"tests/pil/scripts/networkio/ws_send_recv.pil"_embed,        "WebSocket send/recv");
        RUN_SCRIPT_TEST_NETWORKIO(allPassed, L"tests/pil/scripts/networkio/ws_close.pil"_embed,            "WebSocket close");
        RUN_SCRIPT_TEST_NETWORKIO(allPassed, L"tests/pil/scripts/networkio/ws_invalid_close.pil"_embed,    "WebSocket invalid close");
        RUN_SCRIPT_TEST_NETWORKIO(allPassed, L"tests/pil/scripts/networkio/ws_invalid_send.pil"_embed,     "WebSocket invalid send");
        RUN_SCRIPT_TEST_NETWORKIO(allPassed, L"tests/pil/scripts/networkio/ws_invalid_recv.pil"_embed,     "WebSocket invalid recv");
        RUN_SCRIPT_TEST_NETWORKIO(allPassed, L"tests/pil/scripts/networkio/multiple_ws.pil"_embed,         "Multiple WebSockets");

        if (allPassed)
            LOG_INFO("All Network I/O Tests passed!");
        else
            LOG_ERROR("Some Network I/O Tests failed!");

        return allPassed;
    }
};
