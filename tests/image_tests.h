#pragma once

#include "runtime/runtime.h"
#include "tests.h"

class ImageTests
{
public:
	static BOOL RunAll()
	{
		BOOL allPassed = true;

		LOG_INFO("Running Image Processing Tests...");

		// BiDifference tests
		RunTest(allPassed, EMBED_FUNC(TestBiDiff_IdenticalImages), "BiDiff identical images all zero"_embed);
		RunTest(allPassed, EMBED_FUNC(TestBiDiff_CompletelyDifferent), "BiDiff completely different images all one"_embed);
		RunTest(allPassed, EMBED_FUNC(TestBiDiff_PartialDifference), "BiDiff partial difference"_embed);
		RunTest(allPassed, EMBED_FUNC(TestBiDiff_SingleChannelDifference), "BiDiff single channel difference detected"_embed);

		// RemoveNoise tests
		RunTest(allPassed, EMBED_FUNC(TestRemoveNoise_AllZero), "RemoveNoise all-zero stays zero"_embed);
		RunTest(allPassed, EMBED_FUNC(TestRemoveNoise_AllOne), "RemoveNoise all-one stays one"_embed);
		RunTest(allPassed, EMBED_FUNC(TestRemoveNoise_SinglePixel), "RemoveNoise single pixel fills block"_embed);

		// FindContours tests
		RunTest(allPassed, EMBED_FUNC(TestFindContours_EmptyImage), "FindContours empty image"_embed);
		RunTest(allPassed, EMBED_FUNC(TestFindContours_SinglePixel), "FindContours single pixel"_embed);
		RunTest(allPassed, EMBED_FUNC(TestFindContours_Rectangle), "FindContours rectangle"_embed);
		RunTest(allPassed, EMBED_FUNC(TestFindContours_TwoObjects), "FindContours two separate objects"_embed);

		if (allPassed)
			LOG_INFO("All Image Processing tests passed!");
		else
			LOG_ERROR("Some Image Processing tests failed!");

		return allPassed;
	}

private:
	// ---- BiDifference tests ----

	static BOOL TestBiDiff_IdenticalImages()
	{
		RGB img[4];
		img[0] = {10, 20, 30};
		img[1] = {40, 50, 60};
		img[2] = {70, 80, 90};
		img[3] = {100, 110, 120};

		UINT8 diff[4];
		ImageProcessor::CalculateBiDifference(img, img, 2, 2, diff);

		for (INT32 i = 0; i < 4; i++)
		{
			if (diff[i] != 0)
			{
				LOG_ERROR("Expected diff[%d] = 0, got %d", i, diff[i]);
				return false;
			}
		}
		return true;
	}

	static BOOL TestBiDiff_CompletelyDifferent()
	{
		RGB img1[4];
		img1[0] = {0, 0, 0};
		img1[1] = {0, 0, 0};
		img1[2] = {0, 0, 0};
		img1[3] = {0, 0, 0};

		RGB img2[4];
		img2[0] = {255, 255, 255};
		img2[1] = {255, 255, 255};
		img2[2] = {255, 255, 255};
		img2[3] = {255, 255, 255};

		UINT8 diff[4];
		ImageProcessor::CalculateBiDifference(img1, img2, 2, 2, diff);

		for (INT32 i = 0; i < 4; i++)
		{
			if (diff[i] != 1)
			{
				LOG_ERROR("Expected diff[%d] = 1, got %d", i, diff[i]);
				return false;
			}
		}
		return true;
	}

	static BOOL TestBiDiff_PartialDifference()
	{
		RGB img1[4];
		img1[0] = {10, 20, 30};
		img1[1] = {40, 50, 60};
		img1[2] = {70, 80, 90};
		img1[3] = {100, 110, 120};

		RGB img2[4];
		img2[0] = {10, 20, 30}; // same
		img2[1] = {40, 50, 61}; // different (Blue)
		img2[2] = {70, 80, 90}; // same
		img2[3] = {99, 110, 120}; // different (Red)

		UINT8 diff[4];
		ImageProcessor::CalculateBiDifference(img1, img2, 2, 2, diff);

		if (diff[0] != 0 || diff[1] != 1 || diff[2] != 0 || diff[3] != 1)
		{
			LOG_ERROR("Partial diff mismatch: %d %d %d %d", diff[0], diff[1], diff[2], diff[3]);
			return false;
		}
		return true;
	}

	static BOOL TestBiDiff_SingleChannelDifference()
	{
		RGB img1[1];
		img1[0] = {100, 200, 50};

		RGB img2[1];
		img2[0] = {100, 201, 50}; // only Green differs

		UINT8 diff[1];
		ImageProcessor::CalculateBiDifference(img1, img2, 1, 1, diff);

		if (diff[0] != 1)
		{
			LOG_ERROR("Single channel diff not detected");
			return false;
		}
		return true;
	}

	// ---- RemoveNoise tests ----

	static BOOL TestRemoveNoise_AllZero()
	{
		// 32x32 so kernelSize = 1
		constexpr UINT32 sz = 32;
		UINT8 data[sz * sz];
		Memory::Zero(data, sizeof(data));

		ImageProcessor::RemoveNoise(data, sz, sz);

		for (UINT32 i = 0; i < sz * sz; i++)
		{
			if (data[i] != 0)
			{
				LOG_ERROR("Expected 0 at index %u, got %d", i, data[i]);
				return false;
			}
		}
		return true;
	}

	static BOOL TestRemoveNoise_AllOne()
	{
		constexpr UINT32 sz = 32;
		UINT8 data[sz * sz];
		Memory::Set(data, 1, sizeof(data));

		ImageProcessor::RemoveNoise(data, sz, sz);

		for (UINT32 i = 0; i < sz * sz; i++)
		{
			if (data[i] != 1)
			{
				LOG_ERROR("Expected 1 at index %u, got %d", i, data[i]);
				return false;
			}
		}
		return true;
	}

	static BOOL TestRemoveNoise_SinglePixel()
	{
		// 32x32 image, kernelSize = 1, so each pixel is its own block
		// Use 64x64 so kernelSize = 2
		constexpr UINT32 sz = 64;
		UINT8 data[sz * sz];
		Memory::Zero(data, sizeof(data));

		// Set a single pixel in a 2x2 block
		data[0] = 1;

		ImageProcessor::RemoveNoise(data, sz, sz);

		// The entire first block (2x2) should be filled
		if (data[0] != 1 || data[1] != 1 || data[sz] != 1 || data[sz + 1] != 1)
		{
			LOG_ERROR("Block not filled: %d %d %d %d", data[0], data[1], data[sz], data[sz + 1]);
			return false;
		}

		// A pixel outside the first block should remain 0
		if (data[2] != 0)
		{
			LOG_ERROR("Pixel outside block should be 0, got %d", data[2]);
			return false;
		}
		return true;
	}

	// ---- FindContours tests ----

	static BOOL TestFindContours_EmptyImage()
	{
		// 4x4 all-zero image — no contours expected (only padding entry)
		INT8 image[16];
		Memory::Zero(image, sizeof(image));

		auto r = ImageProcessor::FindContours(image, 4, 4);
		if (!r)
		{
			LOG_ERROR("FindContours failed: %e", r.Error());
			return false;
		}

		auto result = r.Value();

		// Only padding entry expected
		if (result.ContourCount != 1)
		{
			LOG_ERROR("Expected 1 (padding), got %d contours", result.ContourCount);
			result.Free();
			return false;
		}

		result.Free();
		return true;
	}

	static BOOL TestFindContours_SinglePixel()
	{
		// 3x3 image with a single pixel set in the center
		// The border needs room, so place '1' at (1,1)
		INT8 image[9];
		Memory::Zero(image, sizeof(image));
		image[1 * 3 + 1] = 1; // center pixel

		auto r = ImageProcessor::FindContours(image, 3, 3);
		if (!r)
		{
			LOG_ERROR("FindContours failed: %e", r.Error());
			return false;
		}

		auto result = r.Value();

		// Should find padding + 1 contour = 2
		if (result.ContourCount < 2)
		{
			LOG_ERROR("Expected >= 2, got %d contours", result.ContourCount);
			result.Free();
			return false;
		}

		// The real contour (index 1) should have at least 1 point
		if (result.Contours[1].Count < 1)
		{
			LOG_ERROR("Contour 1 has %d points, expected >= 1", result.Contours[1].Count);
			result.Free();
			return false;
		}

		result.Free();
		return true;
	}

	static BOOL TestFindContours_Rectangle()
	{
		// 6x6 image with a 4x2 filled rectangle in the middle
		//   0 0 0 0 0 0
		//   0 1 1 1 1 0
		//   0 1 1 1 1 0
		//   0 0 0 0 0 0
		//   0 0 0 0 0 0
		//   0 0 0 0 0 0
		INT8 image[36];
		Memory::Zero(image, sizeof(image));
		for (INT32 r = 1; r <= 2; r++)
			for (INT32 c = 1; c <= 4; c++)
				image[r * 6 + c] = 1;

		auto res = ImageProcessor::FindContours(image, 6, 6);
		if (!res)
		{
			LOG_ERROR("FindContours failed: %e", res.Error());
			return false;
		}

		auto result = res.Value();

		// padding + 1 outer contour = 2
		if (result.ContourCount < 2)
		{
			LOG_ERROR("Expected >= 2 contours, got %d", result.ContourCount);
			result.Free();
			return false;
		}

		// The rectangle contour should trace the border (8 boundary pixels)
		if (result.Contours[1].Count < 4)
		{
			LOG_ERROR("Rectangle contour has %d points, expected >= 4", result.Contours[1].Count);
			result.Free();
			return false;
		}

		// Hierarchy should have at least 2 nodes (root + rectangle)
		if (result.HierarchyCount < 2)
		{
			LOG_ERROR("Expected >= 2 hierarchy nodes, got %d", result.HierarchyCount);
			result.Free();
			return false;
		}

		result.Free();
		return true;
	}

	static BOOL TestFindContours_TwoObjects()
	{
		// 8x4 image with two separate 1-pixel objects
		//   0 0 0 0 0 0 0 0
		//   0 1 0 0 0 1 0 0
		//   0 0 0 0 0 0 0 0
		//   0 0 0 0 0 0 0 0
		INT8 image[32];
		Memory::Zero(image, sizeof(image));
		image[1 * 8 + 1] = 1; // object 1
		image[1 * 8 + 5] = 1; // object 2

		auto r = ImageProcessor::FindContours(image, 4, 8);
		if (!r)
		{
			LOG_ERROR("FindContours failed: %e", r.Error());
			return false;
		}

		auto result = r.Value();

		// padding + 2 contours = 3
		if (result.ContourCount < 3)
		{
			LOG_ERROR("Expected >= 3 contours for two objects, got %d", result.ContourCount);
			result.Free();
			return false;
		}

		result.Free();
		return true;
	}
};
