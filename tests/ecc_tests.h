#pragma once

#include "runtime.h"
#include "tests.h"

class EccTests
{
public:
	static BOOL RunAll()
	{
		BOOL allPassed = true;

		LOG_INFO("Running ECC Tests...");

		RunTest(allPassed, EMBED_FUNC(TestEccInitialization), L"ECC initialization"_embed);
		RunTest(allPassed, EMBED_FUNC(TestEccSecp256r1), L"ECC secp256r1"_embed);
		RunTest(allPassed, EMBED_FUNC(TestEccSecp384r1), L"ECC secp384r1"_embed);
		RunTest(allPassed, EMBED_FUNC(TestPublicKeyExport), L"Public key export"_embed);
		RunTest(allPassed, EMBED_FUNC(TestPublicKeyFormat), L"Public key format"_embed);
		RunTest(allPassed, EMBED_FUNC(TestSharedSecretComputation), L"Shared secret computation (ECDH)"_embed);
		RunTest(allPassed, EMBED_FUNC(TestInvalidCurveSize), L"Invalid curve size handling"_embed);
		RunTest(allPassed, EMBED_FUNC(TestExportBufferSizeValidation), L"Export buffer size validation"_embed);
		RunTest(allPassed, EMBED_FUNC(TestInvalidPublicKey), L"Invalid public key handling"_embed);
		RunTest(allPassed, EMBED_FUNC(TestMultipleKeyGeneration), L"Multiple key generation uniqueness"_embed);

		if (allPassed)
			LOG_INFO("All ECC tests passed!");
		else
			LOG_ERROR("Some ECC tests failed!");

		return allPassed;
	}

private:
	// Test 1: Basic ECC initialization
	static BOOL TestEccInitialization()
	{
		Ecc ecc;
		// Test successful initialization with secp256r1 (32 bytes)
		auto result = ecc.Initialize(32);
		return result.IsOk();
	}

	// Test 2: secp256r1 curve (32 bytes)
	static BOOL TestEccSecp256r1()
	{
		Ecc ecc;
		if (!ecc.Initialize(32))
			return false;

		// Verify we can export a public key
		UINT8 publicKey[32 * 2 + 1];
		auto result = ecc.ExportPublicKey(publicKey, sizeof(publicKey));
		if (!result)
			return false;

		return publicKey[0] == 0x04;
	}

	// Test 5: secp384r1 curve (48 bytes)
	static BOOL TestEccSecp384r1()
	{
		Ecc ecc;
		if (!ecc.Initialize(48))
			return false;

		// Verify we can export a public key
		UINT8 publicKey[48 * 2 + 1];
		auto result = ecc.ExportPublicKey(publicKey, sizeof(publicKey));
		if (!result)
			return false;

		return publicKey[0] == 0x04;
	}

	// Test 6: Public key export functionality
	static BOOL TestPublicKeyExport()
	{
		Ecc ecc;
		(void)ecc.Initialize(32); // secp256r1

		UINT8 publicKey[32 * 2 + 1];
		auto result = ecc.ExportPublicKey(publicKey, sizeof(publicKey));

		// Export should succeed
		if (!result)
			return false;

		// Public key should not be all zeros
		if (IsAllZeros(publicKey, sizeof(publicKey)))
			return false;

		return true;
	}

	// Test 7: Public key format validation
	static BOOL TestPublicKeyFormat()
	{
		Ecc ecc;
		(void)ecc.Initialize(32); // secp256r1

		UINT8 publicKey[32 * 2 + 1];
		(void)ecc.ExportPublicKey(publicKey, sizeof(publicKey));

		// First byte should be 0x04 (uncompressed point format)
		if (publicKey[0] != 0x04)
			return false;

		// X and Y coordinates should not both be all zeros
		BOOL xAllZeros = IsAllZeros(publicKey + 1, 32);
		BOOL yAllZeros = IsAllZeros(publicKey + 1 + 32, 32);

		return !(xAllZeros && yAllZeros);
	}

	// Test 8: Shared secret computation (ECDH key exchange)
	static BOOL TestSharedSecretComputation()
	{
		// Create two ECC instances (Alice and Bob)
		Ecc alice, bob;

		(void)alice.Initialize(32); // secp256r1
		(void)bob.Initialize(32);

		// Export public keys
		UINT8 alicePublicKey[32 * 2 + 1];
		UINT8 bobPublicKey[32 * 2 + 1];

		(void)alice.ExportPublicKey(alicePublicKey, sizeof(alicePublicKey));
		(void)bob.ExportPublicKey(bobPublicKey, sizeof(bobPublicKey));

		// Compute shared secrets
		UINT8 aliceSecret[32];
		UINT8 bobSecret[32];

		auto aliceResult = alice.ComputeSharedSecret(bobPublicKey, sizeof(bobPublicKey), aliceSecret);
		auto bobResult = bob.ComputeSharedSecret(alicePublicKey, sizeof(alicePublicKey), bobSecret);

		// Both should succeed
		if (!aliceResult || !bobResult)
			return false;

		// Shared secrets should match
		return CompareBytes(aliceSecret, bobSecret, 32);
	}

	// Test 9: Invalid curve size handling
	static BOOL TestInvalidCurveSize()
	{
		Ecc ecc;

		// Try to initialize with invalid size (should fail)
		auto result = ecc.Initialize(64); // Invalid size

		// Should fail
		return result.IsErr();
	}

	// Test 10: Export buffer size validation
	static BOOL TestExportBufferSizeValidation()
	{
		Ecc ecc;
		(void)ecc.Initialize(32);

		UINT8 tooSmallBuffer[32]; // Too small for secp256r1 (needs 65 bytes)
		auto result = ecc.ExportPublicKey(tooSmallBuffer, sizeof(tooSmallBuffer));

		// Should fail due to insufficient buffer size
		return result.IsErr();
	}

	// Test 11: Invalid public key handling
	static BOOL TestInvalidPublicKey()
	{
		Ecc ecc;
		(void)ecc.Initialize(32);

		// Create an invalid public key (wrong format byte)
		UINT8 invalidPublicKey[32 * 2 + 1];
		Memory::Zero(invalidPublicKey, sizeof(invalidPublicKey));
		invalidPublicKey[0] = 0x03; // Invalid format (should be 0x04)

		UINT8 secret[32];
		auto result = ecc.ComputeSharedSecret(invalidPublicKey, sizeof(invalidPublicKey), secret);

		// Should fail
		return result.IsErr();
	}

	// Test 12: Sequential key generation produces different keys
	static BOOL TestMultipleKeyGeneration()
	{
		Ecc ecc1;
		(void)ecc1.Initialize(32);

		UINT8 pubKey1[32 * 2 + 1];
		(void)ecc1.ExportPublicKey(pubKey1, sizeof(pubKey1));

		// Generate second key - should be different because Initialize()
		// uses random bytes which advances the RNG state
		Ecc ecc2;
		(void)ecc2.Initialize(32);

		UINT8 pubKey2[32 * 2 + 1];
		(void)ecc2.ExportPublicKey(pubKey2, sizeof(pubKey2));

		// Keys should be different (each Initialize() call uses RNG)
		BOOL key1DiffersFrom2 = !CompareBytes(pubKey1, pubKey2, sizeof(pubKey1));

		// Verify keys are valid (not all zeros)
		BOOL key1Valid = pubKey1[0] == 0x04 && !IsAllZeros(pubKey1 + 1, 64);
		BOOL key2Valid = pubKey2[0] == 0x04 && !IsAllZeros(pubKey2 + 1, 64);

		LOG_INFO("Key 1 valid: %d, Key 2 valid: %d, Keys differ: %d", key1Valid, key2Valid, key1DiffersFrom2);

		return key1DiffersFrom2 && key1Valid && key2Valid;
	}
};
