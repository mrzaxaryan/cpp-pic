#pragma once

#include "ral.h"


class EccTests
{
public:
	static BOOL RunAll()
	{
		BOOL allPassed = TRUE;

		Logger::Info<WCHAR>(L"Running ECC Tests..."_embed);

		// Test 1: Basic initialization
		if (!TestEccInitialization())
		{
			allPassed = FALSE;
			Logger::Error<WCHAR>(L"  FAILED: ECC initialization"_embed);
		}
		else
		{
			Logger::Info<WCHAR>(L"  PASSED: ECC initialization"_embed);
		}

		// Test 2: secp128r1 curve
		if (!TestEccSecp128r1())
		{
			allPassed = FALSE;
			Logger::Error<WCHAR>(L"  FAILED: ECC secp128r1"_embed);
		}
		else
		{
			Logger::Info<WCHAR>(L"  PASSED: ECC secp128r1"_embed);
		}

		// Test 3: secp192r1 curve
		if (!TestEccSecp192r1())
		{
			allPassed = FALSE;
			Logger::Error<WCHAR>(L"  FAILED: ECC secp192r1"_embed);
		}
		else
		{
			Logger::Info<WCHAR>(L"  PASSED: ECC secp192r1"_embed);
		}

		// Test 4: secp256r1 curve
		if (!TestEccSecp256r1())
		{
			allPassed = FALSE;
			Logger::Error<WCHAR>(L"  FAILED: ECC secp256r1"_embed);
		}
		else
		{
			Logger::Info<WCHAR>(L"  PASSED: ECC secp256r1"_embed);
		}

		// Test 5: secp384r1 curve
		if (!TestEccSecp384r1())
		{
			allPassed = FALSE;
			Logger::Error<WCHAR>(L"  FAILED: ECC secp384r1"_embed);
		}
		else
		{
			Logger::Info<WCHAR>(L"  PASSED: ECC secp384r1"_embed);
		}

		// Test 6: Public key export
		if (!TestPublicKeyExport())
		{
			allPassed = FALSE;
			Logger::Error<WCHAR>(L"  FAILED: Public key export"_embed);
		}
		else
		{
			Logger::Info<WCHAR>(L"  PASSED: Public key export"_embed);
		}

		// Test 7: Public key format
		if (!TestPublicKeyFormat())
		{
			allPassed = FALSE;
			Logger::Error<WCHAR>(L"  FAILED: Public key format"_embed);
		}
		else
		{
			Logger::Info<WCHAR>(L"  PASSED: Public key format"_embed);
		}

		// Test 8: Shared secret computation (ECDH)
		if (!TestSharedSecretComputation())
		{
			allPassed = FALSE;
			Logger::Error<WCHAR>(L"  FAILED: Shared secret computation (ECDH)"_embed);
		}
		else
		{
			Logger::Info<WCHAR>(L"  PASSED: Shared secret computation (ECDH)"_embed);
		}

		// Test 9: Invalid curve size handling
		if (!TestInvalidCurveSize())
		{
			allPassed = FALSE;
			Logger::Error<WCHAR>(L"  FAILED: Invalid curve size handling"_embed);
		}
		else
		{
			Logger::Info<WCHAR>(L"  PASSED: Invalid curve size handling"_embed);
		}

		// Test 10: Export buffer size validation
		if (!TestExportBufferSizeValidation())
		{
			allPassed = FALSE;
			Logger::Error<WCHAR>(L"  FAILED: Export buffer size validation"_embed);
		}
		else
		{
			Logger::Info<WCHAR>(L"  PASSED: Export buffer size validation"_embed);
		}

		// Test 11: Invalid public key handling
		if (!TestInvalidPublicKey())
		{
			allPassed = FALSE;
			Logger::Error<WCHAR>(L"  FAILED: Invalid public key handling"_embed);
		}
		else
		{
			Logger::Info<WCHAR>(L"  PASSED: Invalid public key handling"_embed);
		}

		// Test 12: Multiple key generation uniqueness
		if (!TestMultipleKeyGeneration())
		{
			allPassed = FALSE;
			Logger::Error<WCHAR>(L"  FAILED: Multiple key generation uniqueness"_embed);
		}
		else
		{
			Logger::Info<WCHAR>(L"  PASSED: Multiple key generation uniqueness"_embed);
		}

		if (allPassed)
		{
			Logger::Info<WCHAR>(L"All ECC tests passed!"_embed);
		}
		else
		{
			Logger::Error<WCHAR>(L"Some ECC tests failed!"_embed);
		}

		return allPassed;
	}

private:
	// Helper function to compare byte arrays
	static BOOL CompareBytes(const UINT8 *a, const UINT8 *b, UINT32 length)
	{
		for (UINT32 i = 0; i < length; i++)
		{
			if (a[i] != b[i])
				return FALSE;
		}
		return TRUE;
	}

	// Helper function to check if all bytes are zero
	static BOOL IsAllZeros(const UINT8 *data, UINT32 length)
	{
		for (UINT32 i = 0; i < length; i++)
		{
			if (data[i] != 0)
				return FALSE;
		}
		return TRUE;
	}

	// Test 1: Basic ECC initialization
	static BOOL TestEccInitialization()
	{
		Ecc ecc;
		// Test successful initialization with secp256r1 (32 bytes)
		INT32 result = ecc.Initialize(32);
		return result == 0;
	}

	// Test 2: secp128r1 curve (16 bytes)
	static BOOL TestEccSecp128r1()
	{
		Ecc ecc;
		INT32 result = ecc.Initialize(16);
		if (result != 0)
			return FALSE;

		// Verify we can export a public key
		UINT8 publicKey[16 * 2 + 1]; // 16 bytes * 2 (x,y) + 1 (format byte)
		result = ecc.ExportPublicKey(publicKey, sizeof(publicKey));

		// Should successfully export
		if (result == 0)
			return FALSE;

		// Public key should start with 0x04 (uncompressed format)
		return publicKey[0] == 0x04;
	}

	// Test 3: secp192r1 curve (24 bytes)
	static BOOL TestEccSecp192r1()
	{
		Ecc ecc;
		INT32 result = ecc.Initialize(24);
		if (result != 0)
			return FALSE;

		// Verify we can export a public key
		UINT8 publicKey[24 * 2 + 1];
		result = ecc.ExportPublicKey(publicKey, sizeof(publicKey));

		if (result == 0)
			return FALSE;

		return publicKey[0] == 0x04;
	}

	// Test 4: secp256r1 curve (32 bytes)
	static BOOL TestEccSecp256r1()
	{
		Ecc ecc;
		INT32 result = ecc.Initialize(32);
		if (result != 0)
			return FALSE;

		// Verify we can export a public key
		UINT8 publicKey[32 * 2 + 1];
		result = ecc.ExportPublicKey(publicKey, sizeof(publicKey));

		if (result == 0)
			return FALSE;

		return publicKey[0] == 0x04;
	}

	// Test 5: secp384r1 curve (48 bytes)
	static BOOL TestEccSecp384r1()
	{
		Ecc ecc;
		INT32 result = ecc.Initialize(48);
		if (result != 0)
			return FALSE;

		// Verify we can export a public key
		UINT8 publicKey[48 * 2 + 1];
		result = ecc.ExportPublicKey(publicKey, sizeof(publicKey));

		if (result == 0)
			return FALSE;

		return publicKey[0] == 0x04;
	}

	// Test 6: Public key export functionality
	static BOOL TestPublicKeyExport()
	{
		Ecc ecc;
		ecc.Initialize(32); // secp256r1

		UINT8 publicKey[32 * 2 + 1];
		INT32 result = ecc.ExportPublicKey(publicKey, sizeof(publicKey));

		// Export should succeed (returns non-zero on success)
		if (result == 0)
			return FALSE;

		// Public key should not be all zeros
		if (IsAllZeros(publicKey, sizeof(publicKey)))
			return FALSE;

		return TRUE;
	}

	// Test 7: Public key format validation
	static BOOL TestPublicKeyFormat()
	{
		Ecc ecc;
		ecc.Initialize(32); // secp256r1

		UINT8 publicKey[32 * 2 + 1];
		ecc.ExportPublicKey(publicKey, sizeof(publicKey));

		// First byte should be 0x04 (uncompressed point format)
		if (publicKey[0] != 0x04)
			return FALSE;

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

		alice.Initialize(32); // secp256r1
		bob.Initialize(32);

		// Export public keys
		UINT8 alicePublicKey[32 * 2 + 1];
		UINT8 bobPublicKey[32 * 2 + 1];

		alice.ExportPublicKey(alicePublicKey, sizeof(alicePublicKey));
		bob.ExportPublicKey(bobPublicKey, sizeof(bobPublicKey));

		// Compute shared secrets
		UINT8 aliceSecret[32];
		UINT8 bobSecret[32];

		INT32 aliceResult = alice.ComputeSharedSecret(bobPublicKey, sizeof(bobPublicKey), aliceSecret);
		INT32 bobResult = bob.ComputeSharedSecret(alicePublicKey, sizeof(alicePublicKey), bobSecret);

		// Both should succeed
		if (aliceResult != 0 || bobResult != 0)
			return FALSE;

		// Shared secrets should match
		return CompareBytes(aliceSecret, bobSecret, 32);
	}

	// Test 9: Invalid curve size handling
	static BOOL TestInvalidCurveSize()
	{
		Ecc ecc;

		// Try to initialize with invalid size (should fail)
		INT32 result = ecc.Initialize(64); // Invalid size

		// Should return -1 (failure)
		return result == -1;
	}

	// Test 10: Export buffer size validation
	static BOOL TestExportBufferSizeValidation()
	{
		Ecc ecc;
		ecc.Initialize(32);

		UINT8 tooSmallBuffer[32]; // Too small for secp256r1 (needs 65 bytes)
		INT32 result = ecc.ExportPublicKey(tooSmallBuffer, sizeof(tooSmallBuffer));

		// Should fail due to insufficient buffer size
		return result == 0;
	}

	// Test 11: Invalid public key handling
	static BOOL TestInvalidPublicKey()
	{
		Ecc ecc;
		ecc.Initialize(32);

		// Create an invalid public key (wrong format byte)
		UINT8 invalidPublicKey[32 * 2 + 1];
		Memory::Zero(invalidPublicKey, sizeof(invalidPublicKey));
		invalidPublicKey[0] = 0x03; // Invalid format (should be 0x04)

		UINT8 secret[32];
		INT32 result = ecc.ComputeSharedSecret(invalidPublicKey, sizeof(invalidPublicKey), secret);

		// Should fail
		return result == -1;
	}

	// Test 12: Sequential key generation produces different keys
	static BOOL TestMultipleKeyGeneration()
	{
		Ecc ecc1;
		ecc1.Initialize(32);

		UINT8 pubKey1[32 * 2 + 1];
		ecc1.ExportPublicKey(pubKey1, sizeof(pubKey1));

		// Generate second key - should be different because Initialize()
		// uses random bytes which advances the RNG state
		Ecc ecc2;
		ecc2.Initialize(32);

		UINT8 pubKey2[32 * 2 + 1];
		ecc2.ExportPublicKey(pubKey2, sizeof(pubKey2));

		// Keys should be different (each Initialize() call uses RNG)
		BOOL key1DiffersFrom2 = !CompareBytes(pubKey1, pubKey2, sizeof(pubKey1));

		// Verify keys are valid (not all zeros)
		BOOL key1Valid = pubKey1[0] == 0x04 && !IsAllZeros(pubKey1 + 1, 64);
		BOOL key2Valid = pubKey2[0] == 0x04 && !IsAllZeros(pubKey2 + 1, 64);

		LOG_INFO("Key 1 valid: %d, Key 2 valid: %d, Keys differ: %d", key1Valid, key2Valid, key1DiffersFrom2);

		return key1DiffersFrom2 && key1Valid && key2Valid;
	}
};
