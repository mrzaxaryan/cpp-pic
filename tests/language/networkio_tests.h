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
        RunScriptTestNetworkIO(allPassed, L"tests/language/scripts/networkio/dns_resolve.pil"_embed,         L"DNS resolve"_embed);
        RunScriptTestNetworkIO(allPassed, L"tests/language/scripts/networkio/dns_resolve4.pil"_embed,        L"DNS resolve IPv4"_embed);
        RunScriptTestNetworkIO(allPassed, L"tests/language/scripts/networkio/dns_resolve6.pil"_embed,        L"DNS resolve IPv6"_embed);
        RunScriptTestNetworkIO(allPassed, L"tests/language/scripts/networkio/dns_resolve_invalid.pil"_embed, L"DNS resolve invalid"_embed);

        // Socket tests
        RunScriptTestNetworkIO(allPassed, L"tests/language/scripts/networkio/socket_connect.pil"_embed,      L"Socket connect"_embed);
        RunScriptTestNetworkIO(allPassed, L"tests/language/scripts/networkio/socket_send_recv.pil"_embed,    L"Socket send/recv"_embed);
        RunScriptTestNetworkIO(allPassed, L"tests/language/scripts/networkio/socket_close.pil"_embed,        L"Socket close"_embed);
        RunScriptTestNetworkIO(allPassed, L"tests/language/scripts/networkio/socket_invalid_close.pil"_embed,L"Socket invalid close"_embed);
        RunScriptTestNetworkIO(allPassed, L"tests/language/scripts/networkio/socket_invalid_send.pil"_embed, L"Socket invalid send"_embed);
        RunScriptTestNetworkIO(allPassed, L"tests/language/scripts/networkio/socket_invalid_recv.pil"_embed, L"Socket invalid recv"_embed);
        RunScriptTestNetworkIO(allPassed, L"tests/language/scripts/networkio/socket_invalid_port.pil"_embed, L"Socket invalid port"_embed);

        // HTTP tests
        RunScriptTestNetworkIO(allPassed, L"tests/language/scripts/networkio/http_open.pil"_embed,           L"HTTP open"_embed);
        RunScriptTestNetworkIO(allPassed, L"tests/language/scripts/networkio/http_get.pil"_embed,            L"HTTP GET"_embed);
        RunScriptTestNetworkIO(allPassed, L"tests/language/scripts/networkio/http_close.pil"_embed,          L"HTTP close"_embed);
        RunScriptTestNetworkIO(allPassed, L"tests/language/scripts/networkio/http_invalid_close.pil"_embed,  L"HTTP invalid close"_embed);
        RunScriptTestNetworkIO(allPassed, L"tests/language/scripts/networkio/http_invalid_get.pil"_embed,    L"HTTP invalid GET"_embed);
        RunScriptTestNetworkIO(allPassed, L"tests/language/scripts/networkio/http_invalid_post.pil"_embed,   L"HTTP invalid POST"_embed);
        RunScriptTestNetworkIO(allPassed, L"tests/language/scripts/networkio/http_invalid_read.pil"_embed,   L"HTTP invalid read"_embed);

        // Multiple handles tests
        RunScriptTestNetworkIO(allPassed, L"tests/language/scripts/networkio/multiple_sockets.pil"_embed,    L"Multiple sockets"_embed);
        RunScriptTestNetworkIO(allPassed, L"tests/language/scripts/networkio/multiple_http.pil"_embed,       L"Multiple HTTP"_embed);

        // WebSocket tests
        RunScriptTestNetworkIO(allPassed, L"tests/language/scripts/networkio/ws_connect.pil"_embed,          L"WebSocket connect"_embed);
        RunScriptTestNetworkIO(allPassed, L"tests/language/scripts/networkio/ws_send_recv.pil"_embed,        L"WebSocket send/recv"_embed);
        RunScriptTestNetworkIO(allPassed, L"tests/language/scripts/networkio/ws_close.pil"_embed,            L"WebSocket close"_embed);
        RunScriptTestNetworkIO(allPassed, L"tests/language/scripts/networkio/ws_invalid_close.pil"_embed,    L"WebSocket invalid close"_embed);
        RunScriptTestNetworkIO(allPassed, L"tests/language/scripts/networkio/ws_invalid_send.pil"_embed,     L"WebSocket invalid send"_embed);
        RunScriptTestNetworkIO(allPassed, L"tests/language/scripts/networkio/ws_invalid_recv.pil"_embed,     L"WebSocket invalid recv"_embed);
        RunScriptTestNetworkIO(allPassed, L"tests/language/scripts/networkio/multiple_ws.pil"_embed,         L"Multiple WebSockets"_embed);

        if (allPassed)
            LOG_INFO("All Network I/O Tests passed!");
        else
            LOG_ERROR("Some Network I/O Tests failed!");

        return allPassed;
    }
};
