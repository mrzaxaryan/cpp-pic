#pragma once

#include "runtime/runtime.h"
#include "tests.h"

class BinaryIOTests
{
public:
	static BOOL RunAll()
	{
		BOOL allPassed = true;
		LOG_INFO("Running BinaryIO Tests...");

		RunTest(allPassed, EMBED_FUNC(TestReaderReadU8), "BinaryReader Read<UINT8>"_embed);
		RunTest(allPassed, EMBED_FUNC(TestReaderReadU16BE), "BinaryReader ReadU16BE"_embed);
		RunTest(allPassed, EMBED_FUNC(TestReaderReadU24BE), "BinaryReader ReadU24BE"_embed);
		RunTest(allPassed, EMBED_FUNC(TestReaderReadU32BE), "BinaryReader ReadU32BE"_embed);
		RunTest(allPassed, EMBED_FUNC(TestReaderSkipAndRemaining), "BinaryReader Skip and Remaining"_embed);
		RunTest(allPassed, EMBED_FUNC(TestReaderSetOffset), "BinaryReader SetOffset"_embed);
		RunTest(allPassed, EMBED_FUNC(TestReaderBoundsCheck), "BinaryReader bounds checking"_embed);
		RunTest(allPassed, EMBED_FUNC(TestWriterWriteU8), "BinaryWriter WriteU8"_embed);
		RunTest(allPassed, EMBED_FUNC(TestWriterWriteU16BE), "BinaryWriter WriteU16BE"_embed);
		RunTest(allPassed, EMBED_FUNC(TestWriterWriteU32BE), "BinaryWriter WriteU32BE"_embed);
		RunTest(allPassed, EMBED_FUNC(TestWriterBoundsCheck), "BinaryWriter bounds checking"_embed);
		RunTest(allPassed, EMBED_FUNC(TestRoundTrip), "BinaryReader/Writer round-trip"_embed);

		if (allPassed)
			LOG_INFO("All BinaryIO tests passed!");
		else
			LOG_ERROR("Some BinaryIO tests failed!");

		return allPassed;
	}

private:
	static BOOL TestReaderReadU8()
	{
		UINT8 data[3];
		data[0] = 0x42;
		data[1] = 0xFF;
		data[2] = 0x00;
		BinaryReader reader{Span<const UINT8>(data)};

		if (reader.Read<UINT8>() != 0x42)
			return false;
		if (reader.Read<UINT8>() != 0xFF)
			return false;
		if (reader.Read<UINT8>() != 0x00)
			return false;
		if (reader.GetOffset() != 3)
			return false;

		return true;
	}

	static BOOL TestReaderReadU16BE()
	{
		// Big-endian: 0x1234 stored as [0x12, 0x34]
		UINT8 data[4];
		data[0] = 0x12;
		data[1] = 0x34;
		data[2] = 0xAB;
		data[3] = 0xCD;
		BinaryReader reader{Span<const UINT8>(data)};

		UINT16 val1 = reader.ReadU16BE();
		if (val1 != 0x1234)
			return false;

		UINT16 val2 = reader.ReadU16BE();
		if (val2 != 0xABCD)
			return false;

		if (reader.GetOffset() != 4)
			return false;

		return true;
	}

	static BOOL TestReaderReadU24BE()
	{
		// Big-endian 24-bit: 0x123456 stored as [0x12, 0x34, 0x56]
		UINT8 data[3];
		data[0] = 0x12;
		data[1] = 0x34;
		data[2] = 0x56;
		BinaryReader reader{Span<const UINT8>(data)};

		UINT32 val = reader.ReadU24BE();
		if (val != 0x123456)
			return false;

		if (reader.GetOffset() != 3)
			return false;

		return true;
	}

	static BOOL TestReaderReadU32BE()
	{
		// Big-endian: 0x12345678 stored as [0x12, 0x34, 0x56, 0x78]
		UINT8 data[4];
		data[0] = 0x12;
		data[1] = 0x34;
		data[2] = 0x56;
		data[3] = 0x78;
		BinaryReader reader{Span<const UINT8>(data)};

		UINT32 val = reader.ReadU32BE();
		if (val != 0x12345678)
			return false;

		if (reader.GetOffset() != 4)
			return false;

		return true;
	}

	static BOOL TestReaderSkipAndRemaining()
	{
		UINT8 data[10];
		Memory::Zero(data, sizeof(data));
		BinaryReader reader{Span<const UINT8>(data)};

		if (reader.Remaining() != 10)
			return false;

		if (!reader.Skip(3))
			return false;
		if (reader.Remaining() != 7)
			return false;
		if (reader.GetOffset() != 3)
			return false;

		if (!reader.Skip(7))
			return false;
		if (reader.Remaining() != 0)
			return false;

		// Should fail: can't skip past end
		if (reader.Skip(1))
			return false;

		return true;
	}

	static BOOL TestReaderSetOffset()
	{
		UINT8 data[4];
		data[0] = 0xAA;
		data[1] = 0xBB;
		data[2] = 0xCC;
		data[3] = 0xDD;
		BinaryReader reader{Span<const UINT8>(data)};

		reader.Skip(2);
		if (reader.Read<UINT8>() != 0xCC)
			return false;

		// Jump back to offset 0
		if (!reader.SetOffset(0))
			return false;
		if (reader.Read<UINT8>() != 0xAA)
			return false;

		// Jump to offset 3
		if (!reader.SetOffset(3))
			return false;
		if (reader.Read<UINT8>() != 0xDD)
			return false;

		// Should fail: past end
		if (reader.SetOffset(5))
			return false;

		return true;
	}

	static BOOL TestReaderBoundsCheck()
	{
		UINT8 data[2];
		data[0] = 0x12;
		data[1] = 0x34;
		BinaryReader reader{Span<const UINT8>(data)};

		// ReadU32BE should fail (only 2 bytes)
		UINT32 val32 = reader.ReadU32BE();
		if (val32 != 0)
			return false;
		// Offset should not advance on failed read
		if (reader.GetOffset() != 0)
			return false;

		// ReadU16BE should succeed
		UINT16 val16 = reader.ReadU16BE();
		if (val16 != 0x1234)
			return false;

		// Now at end, another read should fail
		if (reader.Read<UINT8>() != 0)
			return false;

		return true;
	}

	static BOOL TestWriterWriteU8()
	{
		UINT8 buf[4];
		Memory::Zero(buf, sizeof(buf));
		BinaryWriter writer{Span<UINT8>(buf)};

		writer.WriteU8(0xAA);
		writer.WriteU8(0xBB);

		if (buf[0] != 0xAA || buf[1] != 0xBB)
			return false;
		if (writer.GetOffset() != 2)
			return false;

		return true;
	}

	static BOOL TestWriterWriteU16BE()
	{
		UINT8 buf[4];
		Memory::Zero(buf, sizeof(buf));
		BinaryWriter writer{Span<UINT8>(buf)};

		writer.WriteU16BE(0x1234);

		// Should be stored as [0x12, 0x34]
		if (buf[0] != 0x12 || buf[1] != 0x34)
			return false;
		if (writer.GetOffset() != 2)
			return false;

		return true;
	}

	static BOOL TestWriterWriteU32BE()
	{
		UINT8 buf[4];
		Memory::Zero(buf, sizeof(buf));
		BinaryWriter writer{Span<UINT8>(buf)};

		writer.WriteU32BE(0x12345678);

		if (buf[0] != 0x12 || buf[1] != 0x34 || buf[2] != 0x56 || buf[3] != 0x78)
			return false;
		if (writer.GetOffset() != 4)
			return false;

		return true;
	}

	static BOOL TestWriterBoundsCheck()
	{
		UINT8 buf[2];
		Memory::Zero(buf, sizeof(buf));
		BinaryWriter writer{Span<UINT8>(buf)};

		// WriteU32BE should fail (only 2 bytes capacity)
		if (writer.WriteU32BE(0x12345678) != nullptr)
			return false;
		if (writer.GetOffset() != 0)
			return false;

		// WriteU16BE should succeed
		if (writer.WriteU16BE(0xABCD) == nullptr)
			return false;

		// Now full, another write should fail
		if (writer.WriteU8(0xFF) != nullptr)
			return false;

		return true;
	}

	static BOOL TestRoundTrip()
	{
		UINT8 buf[16];
		Memory::Zero(buf, sizeof(buf));

		// Write big-endian values
		BinaryWriter writer{Span<UINT8>(buf)};
		writer.WriteU8(0x42);
		writer.WriteU16BE(0x1234);
		writer.WriteU24BE(0xABCDEF);
		writer.WriteU32BE(0xDEADBEEF);

		// Read them back
		BinaryReader reader{Span<const UINT8>(buf, writer.GetOffset())};

		if (reader.Read<UINT8>() != 0x42)
			return false;
		if (reader.ReadU16BE() != 0x1234)
			return false;
		if (reader.ReadU24BE() != 0xABCDEF)
			return false;
		if (reader.ReadU32BE() != 0xDEADBEEF)
			return false;

		if (reader.Remaining() != 0)
			return false;

		return true;
	}
};
