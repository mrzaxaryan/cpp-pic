/**
 * tests.h - Unified Test Suite Header
 *
 * This header exposes all test suite classes for the CPP-PIC runtime.
 * Include this single header to access all test functionality.
 *
 * TEST SUITES:
 *   Djb2Tests              - Hash function tests
 *   MemoryTests            - Memory operations tests
 *   StringTests            - String utility tests
 *   DoubleTests            - Floating-point tests
 *   StringFormatterTests   - Printf-style formatting tests
 *   RandomTests            - Random number generation tests
 *   SocketTests            - Socket and network tests
 *   TlsTests               - TLS 1.3 implementation tests
 *   ArrayStorageTests      - Compile-time array storage tests
 *   ShaTests               - SHA-2 hash function tests (SHA-224/256/384/512 and HMAC)
 *   Base64Tests            - Base64 encoding/decoding tests
 *   EccTests               - Elliptic Curve Cryptography tests (ECDH key exchange)
 *   DnsTests               - DNS resolution tests (DoT, DoH JSON, DoH binary)
 *   WebSocketTests         - WebSocket client implementation tests (ws:// and wss://)
 *
 * USAGE:
 *   #include "tests.h"
 *
 *   // Run all tests
 *   Djb2Tests::RunAll();
 *   MemoryTests::RunAll();
 *   ArrayStorageTests::RunAll();
 *   SocketTests::RunAll();
 *   TlsTests::RunAll();
 *   ShaTests::RunAll();
 *   Base64Tests::RunAll();
 *   EccTests::RunAll();
 *   DnsTests::RunAll();
 *   WebSocketTests::RunAll();
 *   // ... etc
 */

#pragma once

#include "djb2_tests.h"
#include "memory_tests.h"
#include "string_tests.h"
#include "double_tests.h"
#include "string_formatter_tests.h"
#include "random_tests.h"
#include "socket_tests.h"
#include "tls_tests.h"
#include "array_storage_tests.h"
#include "sha_tests.h"
#include "base64_tests.h"
#include "ecc_tests.h"
#include "dns_tests.h"
#include "websocket_tests.h"

static BOOL RunAllTests()
{
	BOOL allPassed = TRUE;

	LOG_INFO("=== CPP-PIC Test Suite ===");

	LOG_INFO("");

	// BAL - Embedded Types and Numeric Primitives
	if (!DoubleTests::RunAll())
		allPassed = FALSE;
	LOG_INFO("");

	if (!StringTests::RunAll())
		allPassed = FALSE;
	LOG_INFO("");

	// BAL - Data Structures, String Utilities, and Algorithms
	if (!ArrayStorageTests::RunAll())
		allPassed = FALSE;
	LOG_INFO("");

	if (!StringFormatterTests::RunAll())
		allPassed = FALSE;
	LOG_INFO("");

	if (!Djb2Tests::RunAll())
		allPassed = FALSE;
	LOG_INFO("");

	if (!Base64Tests::RunAll())
		allPassed = FALSE;
	LOG_INFO("");

	// PAL - Memory and System
	if (!MemoryTests::RunAll())
		allPassed = FALSE;
	LOG_INFO("");

	if (!RandomTests::RunAll())
		allPassed = FALSE;
	LOG_INFO("");

	// RAL - Cryptography
	if (!ShaTests::RunAll())
		allPassed = FALSE;
	LOG_INFO("");

	if (!EccTests::RunAll())
		allPassed = FALSE;
	LOG_INFO("");

	// RAL - Network
	if (!SocketTests::RunAll())
		allPassed = FALSE;
	LOG_INFO("");

	if (!TlsTests::RunAll())
		allPassed = FALSE;
	LOG_INFO("");

	if (!DnsTests::RunAll())
		allPassed = FALSE;
	LOG_INFO("");

	if (!WebSocketTests::RunAll())
		allPassed = FALSE;
	LOG_INFO("");

	// Final summary
	LOG_INFO("=== Test Suite Complete ===");
	if (allPassed)
	{
		LOG_INFO("ALL TESTS PASSED!");
	}
	else
	{
		LOG_ERROR("SOME TESTS FAILED!");
	}

	return allPassed;
}