#pragma once

#include "runtime/runtime.h"
#include "tests.h"

class EccTests
{
public:
	static BOOL RunAll()
	{
		BOOL allPassed = true;

		LOG_INFO("Running ECC Tests...");

		RunTest(allPassed, EMBED_FUNC(TestEccInitialization), "ECC initialization"_embed);
		RunTest(allPassed, EMBED_FUNC(TestEccSecp256r1), "ECC secp256r1"_embed);
		RunTest(allPassed, EMBED_FUNC(TestEccSecp384r1), "ECC secp384r1"_embed);
		RunTest(allPassed, EMBED_FUNC(TestPublicKeyExport), "Public key export"_embed);
		RunTest(allPassed, EMBED_FUNC(TestPublicKeyFormat), "Public key format"_embed);
		RunTest(allPassed, EMBED_FUNC(TestSharedSecretComputation), "Shared secret computation (ECDH)"_embed);
		RunTest(allPassed, EMBED_FUNC(TestInvalidCurveSize), "Invalid curve size handling"_embed);
		RunTest(allPassed, EMBED_FUNC(TestExportBufferSizeValidation), "Export buffer size validation"_embed);
		RunTest(allPassed, EMBED_FUNC(TestInvalidPublicKey), "Invalid public key handling"_embed);
		RunTest(allPassed, EMBED_FUNC(TestMultipleKeyGeneration), "Multiple key generation uniqueness"_embed);

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
		ECC ecc;
		// Test successful initialization with secp256r1 (32 bytes)
		auto result = ecc.Initialize(32);
		if (!result)
		{
			LOG_ERROR("ECC Initialize(32) failed (error: %e)", result.Error());
			return false;
		}
		return true;
	}

	// Test 2: secp256r1 curve (32 bytes)
	static BOOL TestEccSecp256r1()
	{
		ECC ecc;
		auto initResult = ecc.Initialize(32);
		if (!initResult)
		{
			LOG_ERROR("secp256r1 Initialize failed (error: %e)", initResult.Error());
			return false;
		}

		// Verify we can export a public key
		UINT8 publicKey[32 * 2 + 1];
		auto result = ecc.ExportPublicKey(Span<UINT8>(publicKey));
		if (!result)
		{
			LOG_ERROR("secp256r1 ExportPublicKey failed (error: %e)", result.Error());
			return false;
		}

		if (publicKey[0] != 0x04)
		{
			LOG_ERROR("secp256r1 public key format byte: expected 0x04, got 0x%02X", (UINT32)publicKey[0]);
			return false;
		}
		return true;
	}

	// Test 5: secp384r1 curve (48 bytes)
	static BOOL TestEccSecp384r1()
	{
		ECC ecc;
		auto initResult = ecc.Initialize(48);
		if (!initResult)
		{
			LOG_ERROR("secp384r1 Initialize failed (error: %e)", initResult.Error());
			return false;
		}

		// Verify we can export a public key
		UINT8 publicKey[48 * 2 + 1];
		auto result = ecc.ExportPublicKey(Span<UINT8>(publicKey));
		if (!result)
		{
			LOG_ERROR("secp384r1 ExportPublicKey failed (error: %e)", result.Error());
			return false;
		}

		if (publicKey[0] != 0x04)
		{
			LOG_ERROR("secp384r1 public key format byte: expected 0x04, got 0x%02X", (UINT32)publicKey[0]);
			return false;
		}
		return true;
	}

	// Test 6: Public key export functionality
	static BOOL TestPublicKeyExport()
	{
		ECC ecc;
		(void)ecc.Initialize(32); // secp256r1

		UINT8 publicKey[32 * 2 + 1];
		auto result = ecc.ExportPublicKey(Span<UINT8>(publicKey));

		// Export should succeed
		if (!result)
		{
			LOG_ERROR("Public key export failed (error: %e)", result.Error());
			return false;
		}

		// Public key should not be all zeros
		if (IsAllZeros(Span<const UINT8>(publicKey)))
		{
			LOG_ERROR("Public key is all zeros");
			return false;
		}

		return true;
	}

	// Test 7: Public key format validation
	static BOOL TestPublicKeyFormat()
	{
		ECC ecc;
		(void)ecc.Initialize(32); // secp256r1

		UINT8 publicKey[32 * 2 + 1];
		(void)ecc.ExportPublicKey(Span<UINT8>(publicKey));

		// First byte should be 0x04 (uncompressed point format)
		if (publicKey[0] != 0x04)
		{
			LOG_ERROR("Public key format byte: expected 0x04, got 0x%02X", (UINT32)publicKey[0]);
			return false;
		}

		// X and Y coordinates should not both be all zeros
		BOOL xAllZeros = IsAllZeros(Span<const UINT8>(publicKey + 1, 32));
		BOOL yAllZeros = IsAllZeros(Span<const UINT8>(publicKey + 1 + 32, 32));

		if (xAllZeros && yAllZeros)
		{
			LOG_ERROR("Both X and Y coordinates are all zeros");
			return false;
		}
		return true;
	}

	// Test 8: Shared secret computation (ECDH key exchange)
	static BOOL TestSharedSecretComputation()
	{
		// Create two ECC instances (Alice and Bob)
		ECC alice, bob;

		(void)alice.Initialize(32); // secp256r1
		(void)bob.Initialize(32);

		// Export public keys
		UINT8 alicePublicKey[32 * 2 + 1];
		UINT8 bobPublicKey[32 * 2 + 1];

		(void)alice.ExportPublicKey(Span<UINT8>(alicePublicKey));
		(void)bob.ExportPublicKey(Span<UINT8>(bobPublicKey));

		// Compute shared secrets
		UINT8 aliceSecret[32];
		UINT8 bobSecret[32];

		auto aliceResult = alice.ComputeSharedSecret(Span<const UINT8>(bobPublicKey), Span<UINT8>(aliceSecret));
		if (!aliceResult)
		{
			LOG_ERROR("Alice ECDH shared secret computation failed (error: %e)", aliceResult.Error());
			return false;
		}

		auto bobResult = bob.ComputeSharedSecret(Span<const UINT8>(alicePublicKey), Span<UINT8>(bobSecret));
		if (!bobResult)
		{
			LOG_ERROR("Bob ECDH shared secret computation failed (error: %e)", bobResult.Error());
			return false;
		}

		// Shared secrets should match
		if (!CompareBytes(Span<const UINT8>(aliceSecret), Span<const UINT8>(bobSecret)))
		{
			LOG_ERROR("ECDH shared secrets do not match");
			return false;
		}
		return true;
	}

	// Test 9: Invalid curve size handling
	static BOOL TestInvalidCurveSize()
	{
		ECC ecc;

		// Try to initialize with invalid size (should fail)
		auto result = ecc.Initialize(64); // Invalid size

		// Should fail
		if (!result.IsErr())
		{
			LOG_ERROR("Initialize(64) should have failed but succeeded");
			return false;
		}
		return true;
	}

	// Test 10: Export buffer size validation
	static BOOL TestExportBufferSizeValidation()
	{
		ECC ecc;
		(void)ecc.Initialize(32);

		UINT8 tooSmallBuffer[32]; // Too small for secp256r1 (needs 65 bytes)
		auto result = ecc.ExportPublicKey(Span<UINT8>(tooSmallBuffer));

		// Should fail due to insufficient buffer size
		if (!result.IsErr())
		{
			LOG_ERROR("ExportPublicKey with small buffer should have failed but succeeded");
			return false;
		}
		return true;
	}

	// Test 11: Invalid public key handling
	static BOOL TestInvalidPublicKey()
	{
		ECC ecc;
		(void)ecc.Initialize(32);

		// Create an invalid public key (wrong format byte)
		UINT8 invalidPublicKey[32 * 2 + 1];
		Memory::Zero(invalidPublicKey, sizeof(invalidPublicKey));
		invalidPublicKey[0] = 0x03; // Invalid format (should be 0x04)

		UINT8 secret[32];
		auto result = ecc.ComputeSharedSecret(Span<const UINT8>(invalidPublicKey), Span<UINT8>(secret));

		// Should fail
		if (!result.IsErr())
		{
			LOG_ERROR("ComputeSharedSecret with invalid key should have failed but succeeded");
			return false;
		}
		return true;
	}

	// Test 12: Sequential key generation produces different keys
	static BOOL TestMultipleKeyGeneration()
	{
		ECC ecc1;
		(void)ecc1.Initialize(32);

		UINT8 pubKey1[32 * 2 + 1];
		(void)ecc1.ExportPublicKey(Span<UINT8>(pubKey1));

		// Generate second key - should be different because Initialize()
		// uses random bytes which advances the RNG state
		ECC ecc2;
		(void)ecc2.Initialize(32);

		UINT8 pubKey2[32 * 2 + 1];
		(void)ecc2.ExportPublicKey(Span<UINT8>(pubKey2));

		// Keys should be different (each Initialize() call uses RNG)
		BOOL key1DiffersFrom2 = !CompareBytes(Span<const UINT8>(pubKey1), Span<const UINT8>(pubKey2));

		// Verify keys are valid (not all zeros)
		BOOL key1Valid = pubKey1[0] == 0x04 && !IsAllZeros(Span<const UINT8>(pubKey1 + 1, 64));
		BOOL key2Valid = pubKey2[0] == 0x04 && !IsAllZeros(Span<const UINT8>(pubKey2 + 1, 64));

		LOG_INFO("Key 1 valid: %d, Key 2 valid: %d, Keys differ: %d", key1Valid, key2Valid, key1DiffersFrom2);

		if (!key1DiffersFrom2 || !key1Valid || !key2Valid)
		{
			LOG_ERROR("Key generation uniqueness failed: key1Valid=%d, key2Valid=%d, differ=%d", key1Valid, key2Valid, key1DiffersFrom2);
			return false;
		}
		return true;
	}
};
