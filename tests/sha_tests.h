#pragma once

#include "ral.h"
#include "sha2.h"

class ShaTests
{
public:
	static BOOL RunAll()
	{
		BOOL allPassed = TRUE;

		Logger::Info<WCHAR>(L"Running SHA Tests..."_embed);

		// SHA-224 Tests
		if (!TestSHA224_Empty())
		{
			allPassed = FALSE;
			Logger::Error<WCHAR>(L"  FAILED: SHA-224 empty string"_embed);
		}
		else
		{
			Logger::Info<WCHAR>(L"  PASSED: SHA-224 empty string"_embed);
		}

		if (!TestSHA224_ABC())
		{
			allPassed = FALSE;
			Logger::Error<WCHAR>(L"  FAILED: SHA-224 'abc'"_embed);
		}
		else
		{
			Logger::Info<WCHAR>(L"  PASSED: SHA-224 'abc'"_embed);
		}

		if (!TestSHA224_Long())
		{
			allPassed = FALSE;
			Logger::Error<WCHAR>(L"  FAILED: SHA-224 long message"_embed);
		}
		else
		{
			Logger::Info<WCHAR>(L"  PASSED: SHA-224 long message"_embed);
		}

		if (!TestSHA224_Incremental())
		{
			allPassed = FALSE;
			Logger::Error<WCHAR>(L"  FAILED: SHA-224 incremental update"_embed);
		}
		else
		{
			Logger::Info<WCHAR>(L"  PASSED: SHA-224 incremental update"_embed);
		}

		// SHA-256 Tests
		if (!TestSHA256_Empty())
		{
			allPassed = FALSE;
			Logger::Error<WCHAR>(L"  FAILED: SHA-256 empty string"_embed);
		}
		else
		{
			Logger::Info<WCHAR>(L"  PASSED: SHA-256 empty string"_embed);
		}

		if (!TestSHA256_ABC())
		{
			allPassed = FALSE;
			Logger::Error<WCHAR>(L"  FAILED: SHA-256 'abc'"_embed);
		}
		else
		{
			Logger::Info<WCHAR>(L"  PASSED: SHA-256 'abc'"_embed);
		}

		if (!TestSHA256_Long())
		{
			allPassed = FALSE;
			Logger::Error<WCHAR>(L"  FAILED: SHA-256 long message"_embed);
		}
		else
		{
			Logger::Info<WCHAR>(L"  PASSED: SHA-256 long message"_embed);
		}

		if (!TestSHA256_Incremental())
		{
			allPassed = FALSE;
			Logger::Error<WCHAR>(L"  FAILED: SHA-256 incremental update"_embed);
		}
		else
		{
			Logger::Info<WCHAR>(L"  PASSED: SHA-256 incremental update"_embed);
		}

		// SHA-384 Tests
		if (!TestSHA384_Empty())
		{
			allPassed = FALSE;
			Logger::Error<WCHAR>(L"  FAILED: SHA-384 empty string"_embed);
		}
		else
		{
			Logger::Info<WCHAR>(L"  PASSED: SHA-384 empty string"_embed);
		}

		if (!TestSHA384_ABC())
		{
			allPassed = FALSE;
			Logger::Error<WCHAR>(L"  FAILED: SHA-384 'abc'"_embed);
		}
		else
		{
			Logger::Info<WCHAR>(L"  PASSED: SHA-384 'abc'"_embed);
		}

		if (!TestSHA384_Long())
		{
			allPassed = FALSE;
			Logger::Error<WCHAR>(L"  FAILED: SHA-384 long message"_embed);
		}
		else
		{
			Logger::Info<WCHAR>(L"  PASSED: SHA-384 long message"_embed);
		}

		if (!TestSHA384_Incremental())
		{
			allPassed = FALSE;
			Logger::Error<WCHAR>(L"  FAILED: SHA-384 incremental update"_embed);
		}
		else
		{
			Logger::Info<WCHAR>(L"  PASSED: SHA-384 incremental update"_embed);
		}

		// SHA-512 Tests
		if (!TestSHA512_Empty())
		{
			allPassed = FALSE;
			Logger::Error<WCHAR>(L"  FAILED: SHA-512 empty string"_embed);
		}
		else
		{
			Logger::Info<WCHAR>(L"  PASSED: SHA-512 empty string"_embed);
		}

		if (!TestSHA512_ABC())
		{
			allPassed = FALSE;
			Logger::Error<WCHAR>(L"  FAILED: SHA-512 'abc'"_embed);
		}
		else
		{
			Logger::Info<WCHAR>(L"  PASSED: SHA-512 'abc'"_embed);
		}

		if (!TestSHA512_Long())
		{
			allPassed = FALSE;
			Logger::Error<WCHAR>(L"  FAILED: SHA-512 long message"_embed);
		}
		else
		{
			Logger::Info<WCHAR>(L"  PASSED: SHA-512 long message"_embed);
		}

		if (!TestSHA512_Incremental())
		{
			allPassed = FALSE;
			Logger::Error<WCHAR>(L"  FAILED: SHA-512 incremental update"_embed);
		}
		else
		{
			Logger::Info<WCHAR>(L"  PASSED: SHA-512 incremental update"_embed);
		}

		// HMAC Tests
		if (!TestHMAC_SHA224())
		{
			allPassed = FALSE;
			Logger::Error<WCHAR>(L"  FAILED: HMAC-SHA224"_embed);
		}
		else
		{
			Logger::Info<WCHAR>(L"  PASSED: HMAC-SHA224"_embed);
		}

		if (!TestHMAC_SHA256())
		{
			allPassed = FALSE;
			Logger::Error<WCHAR>(L"  FAILED: HMAC-SHA256"_embed);
		}
		else
		{
			Logger::Info<WCHAR>(L"  PASSED: HMAC-SHA256"_embed);
		}

		if (!TestHMAC_SHA384())
		{
			allPassed = FALSE;
			Logger::Error<WCHAR>(L"  FAILED: HMAC-SHA384"_embed);
		}
		else
		{
			Logger::Info<WCHAR>(L"  PASSED: HMAC-SHA384"_embed);
		}

		if (!TestHMAC_SHA512())
		{
			allPassed = FALSE;
			Logger::Error<WCHAR>(L"  FAILED: HMAC-SHA512"_embed);
		}
		else
		{
			Logger::Info<WCHAR>(L"  PASSED: HMAC-SHA512"_embed);
		}

		if (allPassed)
		{
			Logger::Info<WCHAR>(L"All SHA tests passed!"_embed);
		}
		else
		{
			Logger::Error<WCHAR>(L"Some SHA tests failed!"_embed);
		}

		return allPassed;
	}

private:
	// Helper function to compare byte arrays
	static BOOL CompareDigest(const UINT8* digest, const UINT8* expected, UINT32 length)
	{
		for (UINT32 i = 0; i < length; i++)
		{
			if (digest[i] != expected[i])
				return FALSE;
		}
		return TRUE;
	}

	// SHA-224 Test: Empty string
	// Expected: d14a028c2a3a2bc9476102bb288234c415a2b01f828ea62ac5b3e42f
	static BOOL TestSHA224_Empty()
	{
		UINT8 digest[SHA224_DIGEST_SIZE];
		static constexpr UINT8 expected_data[SHA224_DIGEST_SIZE] = {
			0xd1, 0x4a, 0x02, 0x8c, 0x2a, 0x3a, 0x2b, 0xc9,
			0x47, 0x61, 0x02, 0xbb, 0x28, 0x82, 0x34, 0xc4,
			0x15, 0xa2, 0xb0, 0x1f, 0x82, 0x8e, 0xa6, 0x2a,
			0xc5, 0xb3, 0xe4, 0x2f
		};
		auto expected = MakeEmbedArray(expected_data);

		auto message = ""_embed;
		SHA224::Hash(reinterpret_cast<const UINT8*>(static_cast<const char*>(message)), 0, digest);
		return CompareDigest(digest, static_cast<const UINT8*>(static_cast<const VOID*>(expected)), SHA224_DIGEST_SIZE);
	}

	// SHA-224 Test: "abc"
	// Expected: 23097d223405d8228642a477bda255b32aadbce4bda0b3f7e36c9da7
	static BOOL TestSHA224_ABC()
	{
		UINT8 digest[SHA224_DIGEST_SIZE];
		static constexpr UINT8 expected_data[SHA224_DIGEST_SIZE] = {
			0x23, 0x09, 0x7d, 0x22, 0x34, 0x05, 0xd8, 0x22,
			0x86, 0x42, 0xa4, 0x77, 0xbd, 0xa2, 0x55, 0xb3,
			0x2a, 0xad, 0xbc, 0xe4, 0xbd, 0xa0, 0xb3, 0xf7,
			0xe3, 0x6c, 0x9d, 0xa7
		};
		auto expected = MakeEmbedArray(expected_data);

		auto message = "abc"_embed;
		SHA224::Hash(reinterpret_cast<const UINT8*>(static_cast<const char*>(message)), 3, digest);
		return CompareDigest(digest, static_cast<const UINT8*>(static_cast<const VOID*>(expected)), SHA224_DIGEST_SIZE);
	}

	// SHA-224 Test: "abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq"
	// Expected: 75388b16512776cc5dba5da1fd890150b0c6455cb4f58b1952522525
	static BOOL TestSHA224_Long()
	{
		UINT8 digest[SHA224_DIGEST_SIZE];
		static constexpr UINT8 expected_data[SHA224_DIGEST_SIZE] = {
			0x75, 0x38, 0x8b, 0x16, 0x51, 0x27, 0x76, 0xcc,
			0x5d, 0xba, 0x5d, 0xa1, 0xfd, 0x89, 0x01, 0x50,
			0xb0, 0xc6, 0x45, 0x5c, 0xb4, 0xf5, 0x8b, 0x19,
			0x52, 0x52, 0x25, 0x25
		};
		auto expected = MakeEmbedArray(expected_data);

		auto message = "abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq"_embed;
		SHA224::Hash(reinterpret_cast<const UINT8*>(static_cast<const char*>(message)), 56, digest);
		return CompareDigest(digest, static_cast<const UINT8*>(static_cast<const VOID*>(expected)), SHA224_DIGEST_SIZE);
	}

	// SHA-224 Test: Incremental update (split "abc" into "ab" + "c")
	static BOOL TestSHA224_Incremental()
	{
		UINT8 digest[SHA224_DIGEST_SIZE];
		static constexpr UINT8 expected_data[SHA224_DIGEST_SIZE] = {
			0x23, 0x09, 0x7d, 0x22, 0x34, 0x05, 0xd8, 0x22,
			0x86, 0x42, 0xa4, 0x77, 0xbd, 0xa2, 0x55, 0xb3,
			0x2a, 0xad, 0xbc, 0xe4, 0xbd, 0xa0, 0xb3, 0xf7,
			0xe3, 0x6c, 0x9d, 0xa7
		};
		auto expected = MakeEmbedArray(expected_data);

		SHA224 ctx;
		auto msg1 = "ab"_embed;
		auto msg2 = "c"_embed;
		ctx.Update(reinterpret_cast<const UINT8*>(static_cast<const char*>(msg1)), 2);
		ctx.Update(reinterpret_cast<const UINT8*>(static_cast<const char*>(msg2)), 1);
		ctx.Final(digest);

		return CompareDigest(digest, static_cast<const UINT8*>(static_cast<const VOID*>(expected)), SHA224_DIGEST_SIZE);
	}

	// SHA-256 Test: Empty string
	// Expected: e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855
	static BOOL TestSHA256_Empty()
	{
		UINT8 digest[SHA256_DIGEST_SIZE];
		static constexpr UINT8 expected_data[SHA256_DIGEST_SIZE] = {
			0xe3, 0xb0, 0xc4, 0x42, 0x98, 0xfc, 0x1c, 0x14,
			0x9a, 0xfb, 0xf4, 0xc8, 0x99, 0x6f, 0xb9, 0x24,
			0x27, 0xae, 0x41, 0xe4, 0x64, 0x9b, 0x93, 0x4c,
			0xa4, 0x95, 0x99, 0x1b, 0x78, 0x52, 0xb8, 0x55
		};
		auto expected = MakeEmbedArray(expected_data);

		auto message = ""_embed;
		SHA256::Hash(reinterpret_cast<const UINT8*>(static_cast<const char*>(message)), 0, digest);
		return CompareDigest(digest, static_cast<const UINT8*>(static_cast<const VOID*>(expected)), SHA256_DIGEST_SIZE);
	}

	// SHA-256 Test: "abc"
	// Expected: ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad
	static BOOL TestSHA256_ABC()
	{
		UINT8 digest[SHA256_DIGEST_SIZE];
		static constexpr UINT8 expected_data[SHA256_DIGEST_SIZE] = {
			0xba, 0x78, 0x16, 0xbf, 0x8f, 0x01, 0xcf, 0xea,
			0x41, 0x41, 0x40, 0xde, 0x5d, 0xae, 0x22, 0x23,
			0xb0, 0x03, 0x61, 0xa3, 0x96, 0x17, 0x7a, 0x9c,
			0xb4, 0x10, 0xff, 0x61, 0xf2, 0x00, 0x15, 0xad
		};
		auto expected = MakeEmbedArray(expected_data);

		auto message = "abc"_embed;
		SHA256::Hash(reinterpret_cast<const UINT8*>(static_cast<const char*>(message)), 3, digest);
		return CompareDigest(digest, static_cast<const UINT8*>(static_cast<const VOID*>(expected)), SHA256_DIGEST_SIZE);
	}

	// SHA-256 Test: "abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq"
	// Expected: 248d6a61d20638b8e5c026930c3e6039a33ce45964ff2167f6ecedd419db06c1
	static BOOL TestSHA256_Long()
	{
		UINT8 digest[SHA256_DIGEST_SIZE];
		static constexpr UINT8 expected_data[SHA256_DIGEST_SIZE] = {
			0x24, 0x8d, 0x6a, 0x61, 0xd2, 0x06, 0x38, 0xb8,
			0xe5, 0xc0, 0x26, 0x93, 0x0c, 0x3e, 0x60, 0x39,
			0xa3, 0x3c, 0xe4, 0x59, 0x64, 0xff, 0x21, 0x67,
			0xf6, 0xec, 0xed, 0xd4, 0x19, 0xdb, 0x06, 0xc1
		};
		auto expected = MakeEmbedArray(expected_data);

		auto message = "abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq"_embed;
		SHA256::Hash(reinterpret_cast<const UINT8*>(static_cast<const char*>(message)), 56, digest);
		return CompareDigest(digest, static_cast<const UINT8*>(static_cast<const VOID*>(expected)), SHA256_DIGEST_SIZE);
	}

	// SHA-256 Test: Incremental update (split "abc" into "ab" + "c")
	static BOOL TestSHA256_Incremental()
	{
		UINT8 digest[SHA256_DIGEST_SIZE];
		static constexpr UINT8 expected_data[SHA256_DIGEST_SIZE] = {
			0xba, 0x78, 0x16, 0xbf, 0x8f, 0x01, 0xcf, 0xea,
			0x41, 0x41, 0x40, 0xde, 0x5d, 0xae, 0x22, 0x23,
			0xb0, 0x03, 0x61, 0xa3, 0x96, 0x17, 0x7a, 0x9c,
			0xb4, 0x10, 0xff, 0x61, 0xf2, 0x00, 0x15, 0xad
		};
		auto expected = MakeEmbedArray(expected_data);

		SHA256 ctx;
		auto msg1 = "ab"_embed;
		auto msg2 = "c"_embed;
		ctx.Update(reinterpret_cast<const UINT8*>(static_cast<const char*>(msg1)), 2);
		ctx.Update(reinterpret_cast<const UINT8*>(static_cast<const char*>(msg2)), 1);
		ctx.Final(digest);

		return CompareDigest(digest, static_cast<const UINT8*>(static_cast<const VOID*>(expected)), SHA256_DIGEST_SIZE);
	}

	// SHA-384 Test: Empty string
	// Expected: 38b060a751ac96384cd9327eb1b1e36a21fdb71114be07434c0cc7bf63f6e1da274edebfe76f65fbd51ad2f14898b95b
	static BOOL TestSHA384_Empty()
	{
		UINT8 digest[SHA384_DIGEST_SIZE];
		static constexpr UINT8 expected_data[SHA384_DIGEST_SIZE] = {
			0x38, 0xb0, 0x60, 0xa7, 0x51, 0xac, 0x96, 0x38,
			0x4c, 0xd9, 0x32, 0x7e, 0xb1, 0xb1, 0xe3, 0x6a,
			0x21, 0xfd, 0xb7, 0x11, 0x14, 0xbe, 0x07, 0x43,
			0x4c, 0x0c, 0xc7, 0xbf, 0x63, 0xf6, 0xe1, 0xda,
			0x27, 0x4e, 0xde, 0xbf, 0xe7, 0x6f, 0x65, 0xfb,
			0xd5, 0x1a, 0xd2, 0xf1, 0x48, 0x98, 0xb9, 0x5b
		};
		auto expected = MakeEmbedArray(expected_data);

		auto message = ""_embed;
		SHA384::Hash(reinterpret_cast<const UINT8*>(static_cast<const char*>(message)), 0, digest);
		return CompareDigest(digest, static_cast<const UINT8*>(static_cast<const VOID*>(expected)), SHA384_DIGEST_SIZE);
	}

	// SHA-384 Test: "abc"
	// Expected: cb00753f45a35e8bb5a03d699ac65007272c32ab0eded1631a8b605a43ff5bed8086072ba1e7cc2358baeca134c825a7
	static BOOL TestSHA384_ABC()
	{
		UINT8 digest[SHA384_DIGEST_SIZE];
		static constexpr UINT8 expected_data[SHA384_DIGEST_SIZE] = {
			0xcb, 0x00, 0x75, 0x3f, 0x45, 0xa3, 0x5e, 0x8b,
			0xb5, 0xa0, 0x3d, 0x69, 0x9a, 0xc6, 0x50, 0x07,
			0x27, 0x2c, 0x32, 0xab, 0x0e, 0xde, 0xd1, 0x63,
			0x1a, 0x8b, 0x60, 0x5a, 0x43, 0xff, 0x5b, 0xed,
			0x80, 0x86, 0x07, 0x2b, 0xa1, 0xe7, 0xcc, 0x23,
			0x58, 0xba, 0xec, 0xa1, 0x34, 0xc8, 0x25, 0xa7
		};
		auto expected = MakeEmbedArray(expected_data);

		auto message = "abc"_embed;
		SHA384::Hash(reinterpret_cast<const UINT8*>(static_cast<const char*>(message)), 3, digest);
		return CompareDigest(digest, static_cast<const UINT8*>(static_cast<const VOID*>(expected)), SHA384_DIGEST_SIZE);
	}

	// SHA-384 Test: "abcdefghbcdefghicdefghijdefghijkefghijklfghijklmghijklmnhijklmnoijklmnopjklmnopqklmnopqrlmnopqrsmnopqrstnopqrstu"
	// Expected: 09330c33f71147e83d192fc782cd1b4753111b173b3b05d22fa08086e3b0f712fcc7c71a557e2db966c3e9fa91746039
	static BOOL TestSHA384_Long()
	{
		UINT8 digest[SHA384_DIGEST_SIZE];
		static constexpr UINT8 expected_data[SHA384_DIGEST_SIZE] = {
			0x09, 0x33, 0x0c, 0x33, 0xf7, 0x11, 0x47, 0xe8,
			0x3d, 0x19, 0x2f, 0xc7, 0x82, 0xcd, 0x1b, 0x47,
			0x53, 0x11, 0x1b, 0x17, 0x3b, 0x3b, 0x05, 0xd2,
			0x2f, 0xa0, 0x80, 0x86, 0xe3, 0xb0, 0xf7, 0x12,
			0xfc, 0xc7, 0xc7, 0x1a, 0x55, 0x7e, 0x2d, 0xb9,
			0x66, 0xc3, 0xe9, 0xfa, 0x91, 0x74, 0x60, 0x39
		};
		auto expected = MakeEmbedArray(expected_data);

		auto message = "abcdefghbcdefghicdefghijdefghijkefghijklfghijklmghijklmnhijklmnoijklmnopjklmnopqklmnopqrlmnopqrsmnopqrstnopqrstu"_embed;
		SHA384::Hash(reinterpret_cast<const UINT8*>(static_cast<const char*>(message)), 112, digest);
		return CompareDigest(digest, static_cast<const UINT8*>(static_cast<const VOID*>(expected)), SHA384_DIGEST_SIZE);
	}

	// SHA-384 Test: Incremental update (split "abc" into "ab" + "c")
	static BOOL TestSHA384_Incremental()
	{
		UINT8 digest[SHA384_DIGEST_SIZE];
		static constexpr UINT8 expected_data[SHA384_DIGEST_SIZE] = {
			0xcb, 0x00, 0x75, 0x3f, 0x45, 0xa3, 0x5e, 0x8b,
			0xb5, 0xa0, 0x3d, 0x69, 0x9a, 0xc6, 0x50, 0x07,
			0x27, 0x2c, 0x32, 0xab, 0x0e, 0xde, 0xd1, 0x63,
			0x1a, 0x8b, 0x60, 0x5a, 0x43, 0xff, 0x5b, 0xed,
			0x80, 0x86, 0x07, 0x2b, 0xa1, 0xe7, 0xcc, 0x23,
			0x58, 0xba, 0xec, 0xa1, 0x34, 0xc8, 0x25, 0xa7
		};
		auto expected = MakeEmbedArray(expected_data);

		SHA384 ctx;
		auto msg1 = "ab"_embed;
		auto msg2 = "c"_embed;
		ctx.Update(reinterpret_cast<const UINT8*>(static_cast<const char*>(msg1)), 2);
		ctx.Update(reinterpret_cast<const UINT8*>(static_cast<const char*>(msg2)), 1);
		ctx.Final(digest);

		return CompareDigest(digest, static_cast<const UINT8*>(static_cast<const VOID*>(expected)), SHA384_DIGEST_SIZE);
	}

	// SHA-512 Test: Empty string
	// Expected: cf83e1357eefb8bdf1542850d66d8007d620e4050b5715dc83f4a921d36ce9ce47d0d13c5d85f2b0ff8318d2877eec2f63b931bd47417a81a538327af927da3e
	static BOOL TestSHA512_Empty()
	{
		UINT8 digest[SHA512_DIGEST_SIZE];
		static constexpr UINT8 expected_data[SHA512_DIGEST_SIZE] = {
			0xcf, 0x83, 0xe1, 0x35, 0x7e, 0xef, 0xb8, 0xbd,
			0xf1, 0x54, 0x28, 0x50, 0xd6, 0x6d, 0x80, 0x07,
			0xd6, 0x20, 0xe4, 0x05, 0x0b, 0x57, 0x15, 0xdc,
			0x83, 0xf4, 0xa9, 0x21, 0xd3, 0x6c, 0xe9, 0xce,
			0x47, 0xd0, 0xd1, 0x3c, 0x5d, 0x85, 0xf2, 0xb0,
			0xff, 0x83, 0x18, 0xd2, 0x87, 0x7e, 0xec, 0x2f,
			0x63, 0xb9, 0x31, 0xbd, 0x47, 0x41, 0x7a, 0x81,
			0xa5, 0x38, 0x32, 0x7a, 0xf9, 0x27, 0xda, 0x3e
		};
		auto expected = MakeEmbedArray(expected_data);

		auto message = ""_embed;
		SHA512::Hash(reinterpret_cast<const UINT8*>(static_cast<const char*>(message)), 0, digest);
		return CompareDigest(digest, static_cast<const UINT8*>(static_cast<const VOID*>(expected)), SHA512_DIGEST_SIZE);
	}

	// SHA-512 Test: "abc"
	// Expected: ddaf35a193617abacc417349ae20413112e6fa4e89a97ea20a9eeee64b55d39a2192992a274fc1a836ba3c23a3feebbd454d4423643ce80e2a9ac94fa54ca49f
	static BOOL TestSHA512_ABC()
	{
		UINT8 digest[SHA512_DIGEST_SIZE];
		static constexpr UINT8 expected_data[SHA512_DIGEST_SIZE] = {
			0xdd, 0xaf, 0x35, 0xa1, 0x93, 0x61, 0x7a, 0xba,
			0xcc, 0x41, 0x73, 0x49, 0xae, 0x20, 0x41, 0x31,
			0x12, 0xe6, 0xfa, 0x4e, 0x89, 0xa9, 0x7e, 0xa2,
			0x0a, 0x9e, 0xee, 0xe6, 0x4b, 0x55, 0xd3, 0x9a,
			0x21, 0x92, 0x99, 0x2a, 0x27, 0x4f, 0xc1, 0xa8,
			0x36, 0xba, 0x3c, 0x23, 0xa3, 0xfe, 0xeb, 0xbd,
			0x45, 0x4d, 0x44, 0x23, 0x64, 0x3c, 0xe8, 0x0e,
			0x2a, 0x9a, 0xc9, 0x4f, 0xa5, 0x4c, 0xa4, 0x9f
		};
		auto expected = MakeEmbedArray(expected_data);

		auto message = "abc"_embed;
		SHA512::Hash(reinterpret_cast<const UINT8*>(static_cast<const char*>(message)), 3, digest);
		return CompareDigest(digest, static_cast<const UINT8*>(static_cast<const VOID*>(expected)), SHA512_DIGEST_SIZE);
	}

	// SHA-512 Test: "abcdefghbcdefghicdefghijdefghijkefghijklfghijklmghijklmnhijklmnoijklmnopjklmnopqklmnopqrlmnopqrsmnopqrstnopqrstu"
	// Expected: 8e959b75dae313da8cf4f72814fc143f8f7779c6eb9f7fa17299aeadb6889018501d289e4900f7e4331b99dec4b5433ac7d329eeb6dd26545e96e55b874be909
	static BOOL TestSHA512_Long()
	{
		UINT8 digest[SHA512_DIGEST_SIZE];
		static constexpr UINT8 expected_data[SHA512_DIGEST_SIZE] = {
			0x8e, 0x95, 0x9b, 0x75, 0xda, 0xe3, 0x13, 0xda,
			0x8c, 0xf4, 0xf7, 0x28, 0x14, 0xfc, 0x14, 0x3f,
			0x8f, 0x77, 0x79, 0xc6, 0xeb, 0x9f, 0x7f, 0xa1,
			0x72, 0x99, 0xae, 0xad, 0xb6, 0x88, 0x90, 0x18,
			0x50, 0x1d, 0x28, 0x9e, 0x49, 0x00, 0xf7, 0xe4,
			0x33, 0x1b, 0x99, 0xde, 0xc4, 0xb5, 0x43, 0x3a,
			0xc7, 0xd3, 0x29, 0xee, 0xb6, 0xdd, 0x26, 0x54,
			0x5e, 0x96, 0xe5, 0x5b, 0x87, 0x4b, 0xe9, 0x09
		};
		auto expected = MakeEmbedArray(expected_data);

		auto message = "abcdefghbcdefghicdefghijdefghijkefghijklfghijklmghijklmnhijklmnoijklmnopjklmnopqklmnopqrlmnopqrsmnopqrstnopqrstu"_embed;
		SHA512::Hash(reinterpret_cast<const UINT8*>(static_cast<const char*>(message)), 112, digest);
		return CompareDigest(digest, static_cast<const UINT8*>(static_cast<const VOID*>(expected)), SHA512_DIGEST_SIZE);
	}

	// SHA-512 Test: Incremental update (split "abc" into "ab" + "c")
	static BOOL TestSHA512_Incremental()
	{
		UINT8 digest[SHA512_DIGEST_SIZE];
		static constexpr UINT8 expected_data[SHA512_DIGEST_SIZE] = {
			0xdd, 0xaf, 0x35, 0xa1, 0x93, 0x61, 0x7a, 0xba,
			0xcc, 0x41, 0x73, 0x49, 0xae, 0x20, 0x41, 0x31,
			0x12, 0xe6, 0xfa, 0x4e, 0x89, 0xa9, 0x7e, 0xa2,
			0x0a, 0x9e, 0xee, 0xe6, 0x4b, 0x55, 0xd3, 0x9a,
			0x21, 0x92, 0x99, 0x2a, 0x27, 0x4f, 0xc1, 0xa8,
			0x36, 0xba, 0x3c, 0x23, 0xa3, 0xfe, 0xeb, 0xbd,
			0x45, 0x4d, 0x44, 0x23, 0x64, 0x3c, 0xe8, 0x0e,
			0x2a, 0x9a, 0xc9, 0x4f, 0xa5, 0x4c, 0xa4, 0x9f
		};
		auto expected = MakeEmbedArray(expected_data);

		SHA512 ctx;
		auto msg1 = "ab"_embed;
		auto msg2 = "c"_embed;
		ctx.Update(reinterpret_cast<const UINT8*>(static_cast<const char*>(msg1)), 2);
		ctx.Update(reinterpret_cast<const UINT8*>(static_cast<const char*>(msg2)), 1);
		ctx.Final(digest);

		return CompareDigest(digest, static_cast<const UINT8*>(static_cast<const VOID*>(expected)), SHA512_DIGEST_SIZE);
	}

	// HMAC-SHA224 Test (RFC 4231 Test Case 1)
	// Key: "Jefe" (0x4a656665)
	// Data: "what do ya want for nothing?"
	// Expected: a30e01098bc6dbbf45690f3a7e9e6d0f8bbea2a39e6148008fd05e44
	static BOOL TestHMAC_SHA224()
	{
		UINT8 mac[SHA224_DIGEST_SIZE];
		static constexpr UINT8 expected_data[SHA224_DIGEST_SIZE] = {
			0xa3, 0x0e, 0x01, 0x09, 0x8b, 0xc6, 0xdb, 0xbf,
			0x45, 0x69, 0x0f, 0x3a, 0x7e, 0x9e, 0x6d, 0x0f,
			0x8b, 0xbe, 0xa2, 0xa3, 0x9e, 0x61, 0x48, 0x00,
			0x8f, 0xd0, 0x5e, 0x44
		};
		auto expected = MakeEmbedArray(expected_data);

		auto key = "Jefe"_embed;
		auto message = "what do ya want for nothing?"_embed;

		HMAC_SHA224::Compute(reinterpret_cast<const UCHAR*>(static_cast<const char*>(key)), 4, reinterpret_cast<const UCHAR*>(static_cast<const char*>(message)), 28, mac, SHA224_DIGEST_SIZE);

		return CompareDigest(mac, static_cast<const UINT8*>(static_cast<const VOID*>(expected)), SHA224_DIGEST_SIZE);
	}

	// HMAC-SHA256 Test (RFC 4231 Test Case 1)
	// Key: "Jefe" (0x4a656665)
	// Data: "what do ya want for nothing?"
	// Expected: 5bdcc146bf60754e6a042426089575c75a003f089d2739839dec58b964ec3843
	static BOOL TestHMAC_SHA256()
	{
		UINT8 mac[SHA256_DIGEST_SIZE];
		static constexpr UINT8 expected_data[SHA256_DIGEST_SIZE] = {
			0x5b, 0xdc, 0xc1, 0x46, 0xbf, 0x60, 0x75, 0x4e,
			0x6a, 0x04, 0x24, 0x26, 0x08, 0x95, 0x75, 0xc7,
			0x5a, 0x00, 0x3f, 0x08, 0x9d, 0x27, 0x39, 0x83,
			0x9d, 0xec, 0x58, 0xb9, 0x64, 0xec, 0x38, 0x43
		};
		auto expected = MakeEmbedArray(expected_data);

		auto key = "Jefe"_embed;
		auto message = "what do ya want for nothing?"_embed;

		HMAC_SHA256::Compute(reinterpret_cast<const UCHAR*>(static_cast<const char*>(key)), 4, reinterpret_cast<const UCHAR*>(static_cast<const char*>(message)), 28, mac, SHA256_DIGEST_SIZE);

		return CompareDigest(mac, static_cast<const UINT8*>(static_cast<const VOID*>(expected)), SHA256_DIGEST_SIZE);
	}

	// HMAC-SHA384 Test (RFC 4231 Test Case 1)
	// Key: "Jefe" (0x4a656665)
	// Data: "what do ya want for nothing?"
	// Expected: af45d2e376484031617f78d2b58a6b1b9c7ef464f5a01b47e42ec3736322445e8e2240ca5e69e2c78b3239ecfab21649
	static BOOL TestHMAC_SHA384()
	{
		UINT8 mac[SHA384_DIGEST_SIZE];
		static constexpr UINT8 expected_data[SHA384_DIGEST_SIZE] = {
			0xaf, 0x45, 0xd2, 0xe3, 0x76, 0x48, 0x40, 0x31,
			0x61, 0x7f, 0x78, 0xd2, 0xb5, 0x8a, 0x6b, 0x1b,
			0x9c, 0x7e, 0xf4, 0x64, 0xf5, 0xa0, 0x1b, 0x47,
			0xe4, 0x2e, 0xc3, 0x73, 0x63, 0x22, 0x44, 0x5e,
			0x8e, 0x22, 0x40, 0xca, 0x5e, 0x69, 0xe2, 0xc7,
			0x8b, 0x32, 0x39, 0xec, 0xfa, 0xb2, 0x16, 0x49
		};
		auto expected = MakeEmbedArray(expected_data);

		auto key = "Jefe"_embed;
		auto message = "what do ya want for nothing?"_embed;

		HMAC_SHA384::Compute(reinterpret_cast<const UCHAR*>(static_cast<const char*>(key)), 4, reinterpret_cast<const UCHAR*>(static_cast<const char*>(message)), 28, mac, SHA384_DIGEST_SIZE);

		return CompareDigest(mac, static_cast<const UINT8*>(static_cast<const VOID*>(expected)), SHA384_DIGEST_SIZE);
	}

	// HMAC-SHA512 Test (RFC 4231 Test Case 1)
	// Key: "Jefe" (0x4a656665)
	// Data: "what do ya want for nothing?"
	// Expected: 164b7a7bfcf819e2e395fbe73b56e0a387bd64222e831fd610270cd7ea2505549758bf75c05a994a6d034f65f8f0e6fdcaeab1a34d4a6b4b636e070a38bce737
	static BOOL TestHMAC_SHA512()
	{
		UINT8 mac[SHA512_DIGEST_SIZE];
		static constexpr UINT8 expected_data[SHA512_DIGEST_SIZE] = {
			0x16, 0x4b, 0x7a, 0x7b, 0xfc, 0xf8, 0x19, 0xe2,
			0xe3, 0x95, 0xfb, 0xe7, 0x3b, 0x56, 0xe0, 0xa3,
			0x87, 0xbd, 0x64, 0x22, 0x2e, 0x83, 0x1f, 0xd6,
			0x10, 0x27, 0x0c, 0xd7, 0xea, 0x25, 0x05, 0x54,
			0x97, 0x58, 0xbf, 0x75, 0xc0, 0x5a, 0x99, 0x4a,
			0x6d, 0x03, 0x4f, 0x65, 0xf8, 0xf0, 0xe6, 0xfd,
			0xca, 0xea, 0xb1, 0xa3, 0x4d, 0x4a, 0x6b, 0x4b,
			0x63, 0x6e, 0x07, 0x0a, 0x38, 0xbc, 0xe7, 0x37
		};
		auto expected = MakeEmbedArray(expected_data);

		auto key = "Jefe"_embed;
		auto message = "what do ya want for nothing?"_embed;

		HMAC_SHA512 hmac(reinterpret_cast<const UCHAR*>(static_cast<const char*>(key)), 4);
		hmac.Update(reinterpret_cast<const UCHAR*>(static_cast<const char*>(message)), 28);
		hmac.Final(mac, SHA512_DIGEST_SIZE);

		return CompareDigest(mac, static_cast<const UINT8*>(static_cast<const VOID*>(expected)), SHA512_DIGEST_SIZE);
	}
};
