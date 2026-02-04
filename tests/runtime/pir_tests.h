/**
 * pir_tests.h - Unified PIR Test Suite Header
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
#include "filesystem_tests.h"

static BOOL RunPIRTests()
{
	BOOL allPassed = TRUE;

	LOG_INFO("=== CPP-PIC Test Suite ===");
	LOG_INFO("");

	// CORE - Embedded Types and Numeric Primitives
	RunTestSuite<DoubleTests>(allPassed);
	RunTestSuite<StringTests>(allPassed);

	// CORE - Data Structures, String Utilities, and Algorithms
	RunTestSuite<ArrayStorageTests>(allPassed);
	RunTestSuite<StringFormatterTests>(allPassed);
	RunTestSuite<Djb2Tests>(allPassed);
	RunTestSuite<Base64Tests>(allPassed);

	// PLATFORM - Memory and System
	RunTestSuite<MemoryTests>(allPassed);
	RunTestSuite<RandomTests>(allPassed);

	// RAL - Cryptography
	RunTestSuite<ShaTests>(allPassed);
	RunTestSuite<EccTests>(allPassed);

	// RAL - Network
	RunTestSuite<SocketTests>(allPassed);
	RunTestSuite<TlsTests>(allPassed);
	RunTestSuite<DnsTests>(allPassed);
	RunTestSuite<WebSocketTests>(allPassed);
	RunTestSuite<FileSystemTests>(allPassed);

	// Final summary
	LOG_INFO("=== Test Suite Complete ===");
	if (allPassed)
		LOG_INFO("ALL TESTS PASSED!");
	else
		LOG_ERROR("SOME TESTS FAILED!");

	return allPassed;
}