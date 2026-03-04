#pragma once

#include "runtime/runtime.h"
#include "tests.h"

class Base64Tests
{
public:
	static BOOL RunAll()
	{
		BOOL allPassed = true;

		LOG_INFO("Running Base64 Tests...");

		// Encoding Tests
		RunTest(allPassed, EMBED_FUNC(TestEncode_Empty), "Base64 encode empty string"_embed);
		RunTest(allPassed, EMBED_FUNC(TestEncode_SingleChar), "Base64 encode single character"_embed);
		RunTest(allPassed, EMBED_FUNC(TestEncode_TwoChars), "Base64 encode two characters"_embed);
		RunTest(allPassed, EMBED_FUNC(TestEncode_ThreeChars), "Base64 encode three characters"_embed);
		RunTest(allPassed, EMBED_FUNC(TestEncode_StandardText), "Base64 encode standard text"_embed);
		RunTest(allPassed, EMBED_FUNC(TestEncode_BinaryData), "Base64 encode binary data"_embed);
		RunTest(allPassed, EMBED_FUNC(TestEncode_AllPaddingCases), "Base64 encode all padding cases"_embed);

		// Decoding Tests
		RunTest(allPassed, EMBED_FUNC(TestDecode_Empty), "Base64 decode empty string"_embed);
		RunTest(allPassed, EMBED_FUNC(TestDecode_SingleChar), "Base64 decode single character"_embed);
		RunTest(allPassed, EMBED_FUNC(TestDecode_TwoChars), "Base64 decode two characters"_embed);
		RunTest(allPassed, EMBED_FUNC(TestDecode_ThreeChars), "Base64 decode three characters"_embed);
		RunTest(allPassed, EMBED_FUNC(TestDecode_StandardText), "Base64 decode standard text"_embed);
		RunTest(allPassed, EMBED_FUNC(TestDecode_BinaryData), "Base64 decode binary data"_embed);

		// Round-trip Tests
		RunTest(allPassed, EMBED_FUNC(TestRoundTrip_Various), "Base64 round-trip test"_embed);

		// Size Calculation Tests
		RunTest(allPassed, EMBED_FUNC(TestEncodeOutSize), "Base64 encode output size calculation"_embed);
		RunTest(allPassed, EMBED_FUNC(TestDecodeOutSize), "Base64 decode output size calculation"_embed);

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
		Base64::Encode(Span<const CHAR>(static_cast<PCCHAR>(input), 0), Span<CHAR>(output));
		if (!StringUtils::Compare<CHAR>(output, static_cast<PCCHAR>(""_embed)))
		{
			LOG_ERROR("Encode empty: got '%s'", output);
			return false;
		}
		return true;
	}

	// Test: Encode single character "f"
	// Expected: "Zg=="
	static BOOL TestEncode_SingleChar()
	{
		CHAR output[10];
		auto input = "f"_embed;
		Base64::Encode(Span<const CHAR>(static_cast<PCCHAR>(input), 1), Span<CHAR>(output));
		if (!StringUtils::Compare<CHAR>(output, static_cast<PCCHAR>("Zg=="_embed)))
		{
			LOG_ERROR("Encode 'f': got '%s'", output);
			return false;
		}
		return true;
	}

	// Test: Encode two characters "fo"
	// Expected: "Zm8="
	static BOOL TestEncode_TwoChars()
	{
		CHAR output[10];
		auto input = "fo"_embed;
		Base64::Encode(Span<const CHAR>(static_cast<PCCHAR>(input), 2), Span<CHAR>(output));
		if (!StringUtils::Compare<CHAR>(output, static_cast<PCCHAR>("Zm8="_embed)))
		{
			LOG_ERROR("Encode 'fo': got '%s'", output);
			return false;
		}
		return true;
	}

	// Test: Encode three characters "foo"
	// Expected: "Zm9v"
	static BOOL TestEncode_ThreeChars()
	{
		CHAR output[10];
		auto input = "foo"_embed;
		Base64::Encode(Span<const CHAR>(static_cast<PCCHAR>(input), 3), Span<CHAR>(output));
		if (!StringUtils::Compare<CHAR>(output, static_cast<PCCHAR>("Zm9v"_embed)))
		{
			LOG_ERROR("Encode 'foo': got '%s'", output);
			return false;
		}
		return true;
	}

	// Test: Encode standard text "Hello, World!"
	// Expected: "SGVsbG8sIFdvcmxkIQ=="
	static BOOL TestEncode_StandardText()
	{
		CHAR output[30];
		auto input = "Hello, World!"_embed;
		Base64::Encode(Span<const CHAR>(static_cast<PCCHAR>(input), 13), Span<CHAR>(output));
		if (!StringUtils::Compare<CHAR>(output, static_cast<PCCHAR>("SGVsbG8sIFdvcmxkIQ=="_embed)))
		{
			LOG_ERROR("Encode 'Hello, World!': got '%s'", output);
			return false;
		}
		return true;
	}

	// Test: Encode binary data
	// Input: {0x00, 0x01, 0x02, 0x03, 0x04, 0x05}
	// Expected: "AAECAwQF"
	static BOOL TestEncode_BinaryData()
	{
		CHAR output[20];
		auto input = MakeEmbedArray((const UINT8[]){0x00, 0x01, 0x02, 0x03, 0x04, 0x05});
		Base64::Encode(Span<const CHAR>(reinterpret_cast<const char *>(static_cast<const VOID *>(input)), 6), Span<CHAR>(output));
		if (!StringUtils::Compare<CHAR>(output, static_cast<PCCHAR>("AAECAwQF"_embed)))
		{
			LOG_ERROR("Encode binary: got '%s'", output);
			return false;
		}
		return true;
	}

	// Test: Encode strings of various lengths to test all padding cases
	static BOOL TestEncode_AllPaddingCases()
	{
		CHAR output[20];

		auto input1 = "f"_embed;
		Base64::Encode(Span<const CHAR>(static_cast<PCCHAR>(input1), 1), Span<CHAR>(output));
		if (!StringUtils::Compare<CHAR>(output, static_cast<PCCHAR>("Zg=="_embed)))
		{
			LOG_ERROR("Padding 'f': got '%s'", output);
			return false;
		}

		auto input2 = "fo"_embed;
		Base64::Encode(Span<const CHAR>(static_cast<PCCHAR>(input2), 2), Span<CHAR>(output));
		if (!StringUtils::Compare<CHAR>(output, static_cast<PCCHAR>("Zm8="_embed)))
		{
			LOG_ERROR("Padding 'fo': got '%s'", output);
			return false;
		}

		auto input3 = "foo"_embed;
		Base64::Encode(Span<const CHAR>(static_cast<PCCHAR>(input3), 3), Span<CHAR>(output));
		if (!StringUtils::Compare<CHAR>(output, static_cast<PCCHAR>("Zm9v"_embed)))
		{
			LOG_ERROR("Padding 'foo': got '%s'", output);
			return false;
		}

		auto input4 = "foob"_embed;
		Base64::Encode(Span<const CHAR>(static_cast<PCCHAR>(input4), 4), Span<CHAR>(output));
		if (!StringUtils::Compare<CHAR>(output, static_cast<PCCHAR>("Zm9vYg=="_embed)))
		{
			LOG_ERROR("Padding 'foob': got '%s'", output);
			return false;
		}

		auto input5 = "fooba"_embed;
		Base64::Encode(Span<const CHAR>(static_cast<PCCHAR>(input5), 5), Span<CHAR>(output));
		if (!StringUtils::Compare<CHAR>(output, static_cast<PCCHAR>("Zm9vYmE="_embed)))
		{
			LOG_ERROR("Padding 'fooba': got '%s'", output);
			return false;
		}

		auto input6 = "foobar"_embed;
		Base64::Encode(Span<const CHAR>(static_cast<PCCHAR>(input6), 6), Span<CHAR>(output));
		if (!StringUtils::Compare<CHAR>(output, static_cast<PCCHAR>("Zm9vYmFy"_embed)))
		{
			LOG_ERROR("Padding 'foobar': got '%s'", output);
			return false;
		}

		return true;
	}

	// Test: Decode empty string
	// Expected: ""
	static BOOL TestDecode_Empty()
	{
		CHAR output[10];
		auto input = ""_embed;
		auto r = Base64::Decode(Span<const CHAR>(static_cast<PCCHAR>(input), 0), Span<CHAR>(output));
		if (!r)
		{
			LOG_ERROR("Decode empty failed (error: %e)", r.Error());
			return false;
		}
		return true;
	}

	// Test: Decode "Zg==" to "f"
	static BOOL TestDecode_SingleChar()
	{
		CHAR output[10];
		auto input = "Zg=="_embed;
		auto r = Base64::Decode(Span<const CHAR>(static_cast<PCCHAR>(input), 4), Span<CHAR>(output));
		if (!r)
		{
			LOG_ERROR("Decode 'Zg==' failed (error: %e)", r.Error());
			return false;
		}
		if (Memory::Compare(output, static_cast<PCCHAR>("f"_embed), 1) != 0)
		{
			LOG_ERROR("Decode 'Zg==' content mismatch");
			return false;
		}
		return true;
	}

	// Test: Decode "Zm8=" to "fo"
	static BOOL TestDecode_TwoChars()
	{
		CHAR output[10];
		auto input = "Zm8="_embed;
		auto r = Base64::Decode(Span<const CHAR>(static_cast<PCCHAR>(input), 4), Span<CHAR>(output));
		if (!r)
		{
			LOG_ERROR("Decode 'Zm8=' failed (error: %e)", r.Error());
			return false;
		}
		if (Memory::Compare(output, static_cast<PCCHAR>("fo"_embed), 2) != 0)
		{
			LOG_ERROR("Decode 'Zm8=' content mismatch");
			return false;
		}
		return true;
	}

	// Test: Decode "Zm9v" to "foo"
	static BOOL TestDecode_ThreeChars()
	{
		CHAR output[10];
		auto input = "Zm9v"_embed;
		auto r = Base64::Decode(Span<const CHAR>(static_cast<PCCHAR>(input), 4), Span<CHAR>(output));
		if (!r)
		{
			LOG_ERROR("Decode 'Zm9v' failed (error: %e)", r.Error());
			return false;
		}
		if (Memory::Compare(output, static_cast<PCCHAR>("foo"_embed), 3) != 0)
		{
			LOG_ERROR("Decode 'Zm9v' content mismatch");
			return false;
		}
		return true;
	}

	// Test: Decode "SGVsbG8sIFdvcmxkIQ==" to "Hello, World!"
	static BOOL TestDecode_StandardText()
	{
		CHAR output[30];
		auto input = "SGVsbG8sIFdvcmxkIQ=="_embed;
		auto r = Base64::Decode(Span<const CHAR>(static_cast<PCCHAR>(input), 20), Span<CHAR>(output));
		if (!r)
		{
			LOG_ERROR("Decode standard text failed (error: %e)", r.Error());
			return false;
		}
		if (Memory::Compare(output, static_cast<PCCHAR>("Hello, World!"_embed), 13) != 0)
		{
			LOG_ERROR("Decode standard text content mismatch");
			return false;
		}
		return true;
	}

	// Test: Decode "AAECAwQF" to binary data {0x00, 0x01, 0x02, 0x03, 0x04, 0x05}
	static BOOL TestDecode_BinaryData()
	{
		CHAR output[20];
		auto input = "AAECAwQF"_embed;
		auto r = Base64::Decode(Span<const CHAR>(static_cast<PCCHAR>(input), 8), Span<CHAR>(output));
		if (!r)
		{
			LOG_ERROR("Decode binary data failed (error: %e)", r.Error());
			return false;
		}

		auto expected = MakeEmbedArray((const UINT8[]){0x00, 0x01, 0x02, 0x03, 0x04, 0x05});

		if (Memory::Compare(output, static_cast<const VOID *>(expected), 6) != 0)
		{
			LOG_ERROR("Decode binary data content mismatch");
			return false;
		}
		return true;
	}

	// Test: Round-trip encoding and decoding
	static BOOL TestRoundTrip_Various()
	{
		CHAR encoded[100];
		CHAR decoded[100];

		// Test various strings
		auto test1 = "The quick brown fox jumps over the lazy dog"_embed;
		UINT32 len1 = 44;
		Base64::Encode(Span<const CHAR>(static_cast<PCCHAR>(test1), len1), Span<CHAR>(encoded));
		auto r1 = Base64::Decode(Span<const CHAR>(encoded, Base64::GetEncodeOutSize(len1) - 1), Span<CHAR>(decoded));
		if (!r1)
		{
			LOG_ERROR("Round-trip decode failed for test1 (error: %e)", r1.Error());
			return false;
		}
		if (Memory::Compare(decoded, static_cast<PCCHAR>(test1), len1) != 0)
		{
			LOG_ERROR("Round-trip content mismatch for test1");
			return false;
		}

		auto test2 = "1234567890"_embed;
		UINT32 len2 = 10;
		Base64::Encode(Span<const CHAR>(static_cast<PCCHAR>(test2), len2), Span<CHAR>(encoded));
		auto r2 = Base64::Decode(Span<const CHAR>(encoded, Base64::GetEncodeOutSize(len2) - 1), Span<CHAR>(decoded));
		if (!r2)
		{
			LOG_ERROR("Round-trip decode failed for test2 (error: %e)", r2.Error());
			return false;
		}
		if (Memory::Compare(decoded, static_cast<PCCHAR>(test2), len2) != 0)
		{
			LOG_ERROR("Round-trip content mismatch for test2");
			return false;
		}

		auto test3 = "!@#$%^&*()_+-=[]{}|;:,.<>?"_embed;
		UINT32 len3 = 26;
		Base64::Encode(Span<const CHAR>(static_cast<PCCHAR>(test3), len3), Span<CHAR>(encoded));
		auto r3 = Base64::Decode(Span<const CHAR>(encoded, Base64::GetEncodeOutSize(len3) - 1), Span<CHAR>(decoded));
		if (!r3)
		{
			LOG_ERROR("Round-trip decode failed for test3 (error: %e)", r3.Error());
			return false;
		}
		if (Memory::Compare(decoded, static_cast<PCCHAR>(test3), len3) != 0)
		{
			LOG_ERROR("Round-trip content mismatch for test3");
			return false;
		}

		return true;
	}

	// Test: GetEncodeOutSize returns correct sizes
	static BOOL TestEncodeOutSize()
	{
		// Empty: 0 -> 1 (null terminator)
		if (Base64::GetEncodeOutSize(0) != 1)
		{
			LOG_ERROR("GetEncodeOutSize(0): expected 1, got %u", Base64::GetEncodeOutSize(0));
			return false;
		}

		// 1 byte: 1 -> 5 (4 chars + null)
		if (Base64::GetEncodeOutSize(1) != 5)
		{
			LOG_ERROR("GetEncodeOutSize(1): expected 5, got %u", Base64::GetEncodeOutSize(1));
			return false;
		}

		// 2 bytes: 2 -> 5 (4 chars + null)
		if (Base64::GetEncodeOutSize(2) != 5)
		{
			LOG_ERROR("GetEncodeOutSize(2): expected 5, got %u", Base64::GetEncodeOutSize(2));
			return false;
		}

		// 3 bytes: 3 -> 5 (4 chars + null)
		if (Base64::GetEncodeOutSize(3) != 5)
		{
			LOG_ERROR("GetEncodeOutSize(3): expected 5, got %u", Base64::GetEncodeOutSize(3));
			return false;
		}

		// 4 bytes: 4 -> 9 (8 chars + null)
		if (Base64::GetEncodeOutSize(4) != 9)
		{
			LOG_ERROR("GetEncodeOutSize(4): expected 9, got %u", Base64::GetEncodeOutSize(4));
			return false;
		}

		// 6 bytes: 6 -> 9 (8 chars + null)
		if (Base64::GetEncodeOutSize(6) != 9)
		{
			LOG_ERROR("GetEncodeOutSize(6): expected 9, got %u", Base64::GetEncodeOutSize(6));
			return false;
		}

		return true;
	}

	// Test: GetDecodeOutSize returns correct sizes
	static BOOL TestDecodeOutSize()
	{
		// 0 chars: 0 -> 0
		if (Base64::GetDecodeOutSize(0) != 0)
		{
			LOG_ERROR("GetDecodeOutSize(0): expected 0, got %u", Base64::GetDecodeOutSize(0));
			return false;
		}

		// 4 chars: 4 -> 3
		if (Base64::GetDecodeOutSize(4) != 3)
		{
			LOG_ERROR("GetDecodeOutSize(4): expected 3, got %u", Base64::GetDecodeOutSize(4));
			return false;
		}

		// 8 chars: 8 -> 6
		if (Base64::GetDecodeOutSize(8) != 6)
		{
			LOG_ERROR("GetDecodeOutSize(8): expected 6, got %u", Base64::GetDecodeOutSize(8));
			return false;
		}

		// 12 chars: 12 -> 9
		if (Base64::GetDecodeOutSize(12) != 9)
		{
			LOG_ERROR("GetDecodeOutSize(12): expected 9, got %u", Base64::GetDecodeOutSize(12));
			return false;
		}

		return true;
	}
};
