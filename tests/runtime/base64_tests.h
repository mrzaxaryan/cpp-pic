#pragma once

#include "runtime.h"
#include "tests.h"

class Base64Tests
{
public:
	static BOOL RunAll()
	{
		BOOL allPassed = TRUE;

		LOG_INFO("Running Base64 Tests...");

		// Encoding Tests
		RUN_TEST(allPassed, TestEncode_Empty, "Base64 encode empty string");
		RUN_TEST(allPassed, TestEncode_SingleChar, "Base64 encode single character");
		RUN_TEST(allPassed, TestEncode_TwoChars, "Base64 encode two characters");
		RUN_TEST(allPassed, TestEncode_ThreeChars, "Base64 encode three characters");
		RUN_TEST(allPassed, TestEncode_StandardText, "Base64 encode standard text");
		RUN_TEST(allPassed, TestEncode_BinaryData, "Base64 encode binary data");
		RUN_TEST(allPassed, TestEncode_AllPaddingCases, "Base64 encode all padding cases");

		// Decoding Tests
		RUN_TEST(allPassed, TestDecode_Empty, "Base64 decode empty string");
		RUN_TEST(allPassed, TestDecode_SingleChar, "Base64 decode single character");
		RUN_TEST(allPassed, TestDecode_TwoChars, "Base64 decode two characters");
		RUN_TEST(allPassed, TestDecode_ThreeChars, "Base64 decode three characters");
		RUN_TEST(allPassed, TestDecode_StandardText, "Base64 decode standard text");
		RUN_TEST(allPassed, TestDecode_BinaryData, "Base64 decode binary data");

		// Round-trip Tests
		RUN_TEST(allPassed, TestRoundTrip_Various, "Base64 round-trip test");

		// Size Calculation Tests
		RUN_TEST(allPassed, TestEncodeOutSize, "Base64 encode output size calculation");
		RUN_TEST(allPassed, TestDecodeOutSize, "Base64 decode output size calculation");

		if (allPassed)
			LOG_INFO("All Base64 tests passed!");
		else
			LOG_ERROR("Some Base64 tests failed!");

		return allPassed;
	}

private:

	// Test: Encode empty string
	// Expected: ""
	static BOOL TestEncode_Empty()
	{
		CHAR output[10];
		auto input = ""_embed;
		Base64::Encode(static_cast<PCCHAR>(input), 0, output);
		return String::Compare<CHAR>(output, static_cast<PCCHAR>(""_embed));
	}

	// Test: Encode single character "f"
	// Expected: "Zg=="
	static BOOL TestEncode_SingleChar()
	{
		CHAR output[10];
		auto input = "f"_embed;
		Base64::Encode(static_cast<PCCHAR>(input), 1, output);
		return String::Compare<CHAR>(output, static_cast<PCCHAR>("Zg=="_embed));
	}

	// Test: Encode two characters "fo"
	// Expected: "Zm8="
	static BOOL TestEncode_TwoChars()
	{
		CHAR output[10];
		auto input = "fo"_embed;
		Base64::Encode(static_cast<PCCHAR>(input), 2, output);
		return String::Compare<CHAR>(output, static_cast<PCCHAR>("Zm8="_embed));
	}

	// Test: Encode three characters "foo"
	// Expected: "Zm9v"
	static BOOL TestEncode_ThreeChars()
	{
		CHAR output[10];
		auto input = "foo"_embed;
		Base64::Encode(static_cast<PCCHAR>(input), 3, output);
		return String::Compare<CHAR>(output, static_cast<PCCHAR>("Zm9v"_embed));
	}

	// Test: Encode standard text "Hello, World!"
	// Expected: "SGVsbG8sIFdvcmxkIQ=="
	static BOOL TestEncode_StandardText()
	{
		CHAR output[30];
		auto input = "Hello, World!"_embed;
		Base64::Encode(static_cast<PCCHAR>(input), 13, output);
		return String::Compare<CHAR>(output, static_cast<PCCHAR>("SGVsbG8sIFdvcmxkIQ=="_embed));
	}

	// Test: Encode binary data
	// Input: {0x00, 0x01, 0x02, 0x03, 0x04, 0x05}
	// Expected: "AAECAwQF"
	static BOOL TestEncode_BinaryData()
	{
		CHAR output[20];
		auto input = MakeEmbedArray((const UINT8[]){0x00, 0x01, 0x02, 0x03, 0x04, 0x05});
		Base64::Encode(reinterpret_cast<const char*>(static_cast<const VOID*>(input)), 6, output);
		return String::Compare<CHAR>(output, static_cast<PCCHAR>("AAECAwQF"_embed));
	}

	// Test: Encode strings of various lengths to test all padding cases
	// "f" -> "Zg==" (2 padding)
	// "fo" -> "Zm8=" (1 padding)
	// "foo" -> "Zm9v" (0 padding)
	// "foob" -> "Zm9vYg==" (2 padding)
	// "fooba" -> "Zm9vYmE=" (1 padding)
	// "foobar" -> "Zm9vYmFy" (0 padding)
	static BOOL TestEncode_AllPaddingCases()
	{
		CHAR output[20];

		auto input1 = "f"_embed;
		Base64::Encode(static_cast<PCCHAR>(input1), 1, output);
		if (!String::Compare<CHAR>(output, static_cast<PCCHAR>("Zg=="_embed)))
			return FALSE;

		auto input2 = "fo"_embed;
		Base64::Encode(static_cast<PCCHAR>(input2), 2, output);
		if (!String::Compare<CHAR>(output, static_cast<PCCHAR>("Zm8="_embed)))
			return FALSE;

		auto input3 = "foo"_embed;
		Base64::Encode(static_cast<PCCHAR>(input3), 3, output);
		if (!String::Compare<CHAR>(output, static_cast<PCCHAR>("Zm9v"_embed)))
			return FALSE;

		auto input4 = "foob"_embed;
		Base64::Encode(static_cast<PCCHAR>(input4), 4, output);
		if (!String::Compare<CHAR>(output, static_cast<PCCHAR>("Zm9vYg=="_embed)))
			return FALSE;

		auto input5 = "fooba"_embed;
		Base64::Encode(static_cast<PCCHAR>(input5), 5, output);
		if (!String::Compare<CHAR>(output, static_cast<PCCHAR>("Zm9vYmE="_embed)))
			return FALSE;

		auto input6 = "foobar"_embed;
		Base64::Encode(static_cast<PCCHAR>(input6), 6, output);
		if (!String::Compare<CHAR>(output, static_cast<PCCHAR>("Zm9vYmFy"_embed)))
			return FALSE;

		return TRUE;
	}

	// Test: Decode empty string
	// Expected: ""
	static BOOL TestDecode_Empty()
	{
		CHAR output[10];
		auto input = ""_embed;
		Base64::Decode(static_cast<PCCHAR>(input), 0, output);
		return TRUE;  // Empty decode should succeed
	}

	// Test: Decode "Zg==" to "f"
	static BOOL TestDecode_SingleChar()
	{
		CHAR output[10];
		auto input = "Zg=="_embed;
		Base64::Decode(static_cast<PCCHAR>(input), 4, output);
		return Memory::Compare(output, static_cast<PCCHAR>("f"_embed), 1) == 0;
	}

	// Test: Decode "Zm8=" to "fo"
	static BOOL TestDecode_TwoChars()
	{
		CHAR output[10];
		auto input = "Zm8="_embed;
		Base64::Decode(static_cast<PCCHAR>(input), 4, output);
		return Memory::Compare(output, static_cast<PCCHAR>("fo"_embed), 2) == 0;
	}

	// Test: Decode "Zm9v" to "foo"
	static BOOL TestDecode_ThreeChars()
	{
		CHAR output[10];
		auto input = "Zm9v"_embed;
		Base64::Decode(static_cast<PCCHAR>(input), 4, output);
		return Memory::Compare(output, static_cast<PCCHAR>("foo"_embed), 3) == 0;
	}

	// Test: Decode "SGVsbG8sIFdvcmxkIQ==" to "Hello, World!"
	static BOOL TestDecode_StandardText()
	{
		CHAR output[30];
		auto input = "SGVsbG8sIFdvcmxkIQ=="_embed;
		Base64::Decode(static_cast<PCCHAR>(input), 20, output);
		return Memory::Compare(output, static_cast<PCCHAR>("Hello, World!"_embed), 13) == 0;
	}

	// Test: Decode "AAECAwQF" to binary data {0x00, 0x01, 0x02, 0x03, 0x04, 0x05}
	static BOOL TestDecode_BinaryData()
	{
		CHAR output[20];
		auto input = "AAECAwQF"_embed;
		Base64::Decode(static_cast<PCCHAR>(input), 8, output);

		auto expected = MakeEmbedArray((const UINT8[]){0x00, 0x01, 0x02, 0x03, 0x04, 0x05});

		return Memory::Compare(output, static_cast<const VOID*>(expected), 6) == 0;
	}

	// Test: Round-trip encoding and decoding
	static BOOL TestRoundTrip_Various()
	{
		CHAR encoded[100];
		CHAR decoded[100];

		// Test various strings
		auto test1 = "The quick brown fox jumps over the lazy dog"_embed;
		UINT32 len1 = 44;
		Base64::Encode(static_cast<PCCHAR>(test1), len1, encoded);
		Base64::Decode(encoded, Base64::GetEncodeOutSize(len1) - 1, decoded);
		if (Memory::Compare(decoded, static_cast<PCCHAR>(test1), len1) != 0)
			return FALSE;

		auto test2 = "1234567890"_embed;
		UINT32 len2 = 10;
		Base64::Encode(static_cast<PCCHAR>(test2), len2, encoded);
		Base64::Decode(encoded, Base64::GetEncodeOutSize(len2) - 1, decoded);
		if (Memory::Compare(decoded, static_cast<PCCHAR>(test2), len2) != 0)
			return FALSE;

		auto test3 = "!@#$%^&*()_+-=[]{}|;:,.<>?"_embed;
		UINT32 len3 = 26;
		Base64::Encode(static_cast<PCCHAR>(test3), len3, encoded);
		Base64::Decode(encoded, Base64::GetEncodeOutSize(len3) - 1, decoded);
		if (Memory::Compare(decoded, static_cast<PCCHAR>(test3), len3) != 0)
			return FALSE;

		return TRUE;
	}

	// Test: GetEncodeOutSize returns correct sizes
	static BOOL TestEncodeOutSize()
	{
		// Empty: 0 -> 1 (null terminator)
		if (Base64::GetEncodeOutSize(0) != 1)
			return FALSE;

		// 1 byte: 1 -> 5 (4 chars + null)
		if (Base64::GetEncodeOutSize(1) != 5)
			return FALSE;

		// 2 bytes: 2 -> 5 (4 chars + null)
		if (Base64::GetEncodeOutSize(2) != 5)
			return FALSE;

		// 3 bytes: 3 -> 5 (4 chars + null)
		if (Base64::GetEncodeOutSize(3) != 5)
			return FALSE;

		// 4 bytes: 4 -> 9 (8 chars + null)
		if (Base64::GetEncodeOutSize(4) != 9)
			return FALSE;

		// 6 bytes: 6 -> 9 (8 chars + null)
		if (Base64::GetEncodeOutSize(6) != 9)
			return FALSE;

		return TRUE;
	}

	// Test: GetDecodeOutSize returns correct sizes
	static BOOL TestDecodeOutSize()
	{
		// 0 chars: 0 -> 0
		if (Base64::GetDecodeOutSize(0) != 0)
			return FALSE;

		// 4 chars: 4 -> 3
		if (Base64::GetDecodeOutSize(4) != 3)
			return FALSE;

		// 8 chars: 8 -> 6
		if (Base64::GetDecodeOutSize(8) != 6)
			return FALSE;

		// 12 chars: 12 -> 9
		if (Base64::GetDecodeOutSize(12) != 9)
			return FALSE;

		return TRUE;
	}
};
