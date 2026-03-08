#pragma once

#include "runtime/runtime.h"
#include "tests.h"

class JpegTests
{
public:
	static BOOL RunAll()
	{
		BOOL allPassed = true;

		LOG_INFO("Running JPEG Encoder Tests...");

		// Validation tests
		RunTest(allPassed, EMBED_FUNC(TestInvalidComponents_Returns_Error), "JPEG reject invalid component count"_embed);
		RunTest(allPassed, EMBED_FUNC(TestZeroWidth_Returns_Error), "JPEG reject zero width"_embed);
		RunTest(allPassed, EMBED_FUNC(TestZeroHeight_Returns_Error), "JPEG reject zero height"_embed);
		RunTest(allPassed, EMBED_FUNC(TestNegativeWidth_Returns_Error), "JPEG reject negative width"_embed);
		RunTest(allPassed, EMBED_FUNC(TestNegativeHeight_Returns_Error), "JPEG reject negative height"_embed);
		RunTest(allPassed, EMBED_FUNC(TestOversizedWidth_Returns_Error), "JPEG reject oversized width"_embed);
		RunTest(allPassed, EMBED_FUNC(TestOversizedHeight_Returns_Error), "JPEG reject oversized height"_embed);

		// Encoding tests
		RunTest(allPassed, EMBED_FUNC(TestEncode1x1_RGB), "JPEG encode 1x1 RGB"_embed);
		RunTest(allPassed, EMBED_FUNC(TestEncode1x1_RGBA), "JPEG encode 1x1 RGBA"_embed);
		RunTest(allPassed, EMBED_FUNC(TestEncode8x8_RGB), "JPEG encode 8x8 RGB"_embed);
		RunTest(allPassed, EMBED_FUNC(TestEncodeNonMultipleOf8), "JPEG encode non-multiple-of-8 dimensions"_embed);
		RunTest(allPassed, EMBED_FUNC(TestEncodeQualityBounds), "JPEG encode with clamped quality"_embed);
		RunTest(allPassed, EMBED_FUNC(TestEncodeAllBlack), "JPEG encode all-black image"_embed);
		RunTest(allPassed, EMBED_FUNC(TestEncodeAllWhite), "JPEG encode all-white image"_embed);

		if (allPassed)
			LOG_INFO("All JPEG tests passed!");
		else
			LOG_ERROR("Some JPEG tests failed!");

		return allPassed;
	}

private:
	// Captured output buffer for test verification
	struct CaptureBuffer
	{
		UINT8 data[16384];
		USIZE size;
	};

	static VOID CaptureCallback(PVOID context, PVOID data, INT32 size)
	{
		auto *buf = (CaptureBuffer *)context;
		if (buf->size + (USIZE)size <= sizeof(buf->data))
		{
			Memory::Copy(buf->data + buf->size, data, (USIZE)size);
			buf->size += (USIZE)size;
		}
	}

	// Verify SOI (0xFFD8) at start and EOI (0xFFD9) at end
	static BOOL VerifyJpegMarkers(const CaptureBuffer &buf)
	{
		if (buf.size < 4)
		{
			LOG_ERROR("JPEG output too small: %u bytes", (UINT32)buf.size);
			return false;
		}
		if (buf.data[0] != 0xFF || buf.data[1] != 0xD8)
		{
			LOG_ERROR("Missing SOI marker: got 0x%02X%02X", buf.data[0], buf.data[1]);
			return false;
		}
		if (buf.data[buf.size - 2] != 0xFF || buf.data[buf.size - 1] != 0xD9)
		{
			LOG_ERROR("Missing EOI marker: got 0x%02X%02X", buf.data[buf.size - 2], buf.data[buf.size - 1]);
			return false;
		}
		return true;
	}

	// --- Validation tests ---

	static BOOL TestInvalidComponents_Returns_Error()
	{
		UINT8 pixel[3];
		pixel[0] = 128; // B
		pixel[1] = 128; // G
		pixel[2] = 128; // R
		CaptureBuffer buf;
		buf.size = 0;

		auto r2 = JpegEncoder::Encode(EMBED_FUNC(CaptureCallback), &buf, 50, 1, 1, 2, Span<const UINT8>(pixel, 3));
		if (r2)
		{
			LOG_ERROR("numComponents=2 should fail");
			return false;
		}

		auto r5 = JpegEncoder::Encode(EMBED_FUNC(CaptureCallback), &buf, 50, 1, 1, 5, Span<const UINT8>(pixel, 3));
		if (r5)
		{
			LOG_ERROR("numComponents=5 should fail");
			return false;
		}

		return true;
	}

	static BOOL TestZeroWidth_Returns_Error()
	{
		UINT8 pixel[3];
		pixel[0] = 0; // B
		pixel[1] = 0; // G
		pixel[2] = 0; // R
		CaptureBuffer buf;
		buf.size = 0;
		auto r = JpegEncoder::Encode(EMBED_FUNC(CaptureCallback), &buf, 50, 0, 1, 3, Span<const UINT8>(pixel, 3));
		return !r;
	}

	static BOOL TestZeroHeight_Returns_Error()
	{
		UINT8 pixel[3];
		pixel[0] = 0; // B
		pixel[1] = 0; // G
		pixel[2] = 0; // R
		CaptureBuffer buf;
		buf.size = 0;
		auto r = JpegEncoder::Encode(EMBED_FUNC(CaptureCallback), &buf, 50, 1, 0, 3, Span<const UINT8>(pixel, 3));
		return !r;
	}

	static BOOL TestNegativeWidth_Returns_Error()
	{
		UINT8 pixel[3];
		pixel[0] = 0; // B
		pixel[1] = 0; // G
		pixel[2] = 0; // R
		CaptureBuffer buf;
		buf.size = 0;
		auto r = JpegEncoder::Encode(EMBED_FUNC(CaptureCallback), &buf, 50, -1, 1, 3, Span<const UINT8>(pixel, 3));
		return !r;
	}

	static BOOL TestNegativeHeight_Returns_Error()
	{
		UINT8 pixel[3];
		pixel[0] = 0; // B
		pixel[1] = 0; // G
		pixel[2] = 0; // R
		CaptureBuffer buf;
		buf.size = 0;
		auto r = JpegEncoder::Encode(EMBED_FUNC(CaptureCallback), &buf, 50, 1, -1, 3, Span<const UINT8>(pixel, 3));
		return !r;
	}

	static BOOL TestOversizedWidth_Returns_Error()
	{
		UINT8 pixel[3];
		pixel[0] = 0; // B
		pixel[1] = 0; // G
		pixel[2] = 0; // R
		CaptureBuffer buf;
		buf.size = 0;
		auto r = JpegEncoder::Encode(EMBED_FUNC(CaptureCallback), &buf, 50, 0x10000, 1, 3, Span<const UINT8>(pixel, 3));
		return !r;
	}

	static BOOL TestOversizedHeight_Returns_Error()
	{
		UINT8 pixel[3];
		pixel[0] = 0; // B
		pixel[1] = 0; // G
		pixel[2] = 0; // R
		CaptureBuffer buf;
		buf.size = 0;
		auto r = JpegEncoder::Encode(EMBED_FUNC(CaptureCallback), &buf, 50, 1, 0x10000, 3, Span<const UINT8>(pixel, 3));
		return !r;
	}

	// --- Encoding tests ---

	static BOOL TestEncode1x1_RGB()
	{
		UINT8 pixel[3];
		pixel[0] = 255; // B
		pixel[1] = 0;	// G
		pixel[2] = 0;	// R
		CaptureBuffer buf;
		buf.size = 0;

		auto r = JpegEncoder::Encode(EMBED_FUNC(CaptureCallback), &buf, 75, 1, 1, 3, Span<const UINT8>(pixel, 3));
		if (!r)
		{
			LOG_ERROR("Encode 1x1 RGB failed: %e", r.Error());
			return false;
		}
		return VerifyJpegMarkers(buf);
	}

	static BOOL TestEncode1x1_RGBA()
	{
		UINT8 pixel[4];
		pixel[0] = 0;	// B
		pixel[1] = 255; // G
		pixel[2] = 0;	// R
		pixel[3] = 255; // A
		CaptureBuffer buf;
		buf.size = 0;

		auto r = JpegEncoder::Encode(EMBED_FUNC(CaptureCallback), &buf, 75, 1, 1, 4, Span<const UINT8>(pixel, 4));
		if (!r)
		{
			LOG_ERROR("Encode 1x1 RGBA failed: %e", r.Error());
			return false;
		}
		return VerifyJpegMarkers(buf);
	}

	static BOOL TestEncode8x8_RGB()
	{
		// 8x8 gradient image
		UINT8 pixels[8 * 8 * 3];
		for (INT32 y = 0; y < 8; ++y)
		{
			for (INT32 x = 0; x < 8; ++x)
			{
				INT32 i = (y * 8 + x) * 3;
				pixels[i + 0] = (UINT8)(x * 32); // B
				pixels[i + 1] = (UINT8)(y * 32); // G
				pixels[i + 2] = (UINT8)(128);	 // R
			}
		}

		CaptureBuffer buf;
		buf.size = 0;

		auto r = JpegEncoder::Encode(EMBED_FUNC(CaptureCallback), &buf, 90, 8, 8, 3, Span<const UINT8>(pixels, sizeof(pixels)));
		if (!r)
		{
			LOG_ERROR("Encode 8x8 RGB failed: %e", r.Error());
			return false;
		}
		return VerifyJpegMarkers(buf);
	}

	static BOOL TestEncodeNonMultipleOf8()
	{
		// 3x5 image — tests edge-block clamping
		constexpr INT32 w = 3;
		constexpr INT32 h = 5;
		UINT8 pixels[w * h * 3];
		for (INT32 i = 0; i < w * h * 3; ++i)
			pixels[i] = (UINT8)(i & 0xFF);

		CaptureBuffer buf;
		buf.size = 0;

		auto r = JpegEncoder::Encode(EMBED_FUNC(CaptureCallback), &buf, 50, w, h, 3, Span<const UINT8>(pixels, sizeof(pixels)));
		if (!r)
		{
			LOG_ERROR("Encode 3x5 failed: %e", r.Error());
			return false;
		}
		return VerifyJpegMarkers(buf);
	}

	static BOOL TestEncodeQualityBounds()
	{
		UINT8 pixel[3];
		pixel[0] = 100; // B
		pixel[1] = 150; // G
		pixel[2] = 200; // R
		CaptureBuffer buf;

		// Quality 0 (clamped to 1)
		buf.size = 0;
		auto r1 = JpegEncoder::Encode(EMBED_FUNC(CaptureCallback), &buf, 0, 1, 1, 3, Span<const UINT8>(pixel, 3));
		if (!r1)
		{
			LOG_ERROR("Quality 0 failed: %e", r1.Error());
			return false;
		}
		if (!VerifyJpegMarkers(buf))
			return false;

		// Quality 200 (clamped to 100)
		buf.size = 0;
		auto r2 = JpegEncoder::Encode(EMBED_FUNC(CaptureCallback), &buf, 200, 1, 1, 3, Span<const UINT8>(pixel, 3));
		if (!r2)
		{
			LOG_ERROR("Quality 200 failed: %e", r2.Error());
			return false;
		}
		if (!VerifyJpegMarkers(buf))
			return false;

		return true;
	}

	static BOOL TestEncodeAllBlack()
	{
		UINT8 pixels[8 * 8 * 3];
		Memory::Zero(pixels, sizeof(pixels));

		CaptureBuffer buf;
		buf.size = 0;

		auto r = JpegEncoder::Encode(EMBED_FUNC(CaptureCallback), &buf, 75, 8, 8, 3, Span<const UINT8>(pixels, sizeof(pixels)));
		if (!r)
		{
			LOG_ERROR("Encode all-black failed: %e", r.Error());
			return false;
		}
		return VerifyJpegMarkers(buf);
	}

	static BOOL TestEncodeAllWhite()
	{
		UINT8 pixels[8 * 8 * 3];
		Memory::Set(pixels, 0xFF, sizeof(pixels));

		CaptureBuffer buf;
		buf.size = 0;

		auto r = JpegEncoder::Encode(EMBED_FUNC(CaptureCallback), &buf, 75, 8, 8, 3, Span<const UINT8>(pixels, sizeof(pixels)));
		if (!r)
		{
			LOG_ERROR("Encode all-white failed: %e", r.Error());
			return false;
		}
		return VerifyJpegMarkers(buf);
	}
};
