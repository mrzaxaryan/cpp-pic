/**
 * @file image_processor.h
 * @brief Image Processing Utilities
 *
 * @details Provides contour detection (Suzuki-Abe border following algorithm),
 * binary image differencing, and noise removal for position-independent
 * execution.
 *
 * @see S. Suzuki and K. Abe, "Topological Structural Analysis of Digitized
 *      Binary Images by Border Following," Computer Vision, Graphics, and
 *      Image Processing, vol. 30, no. 1, pp. 32-46, 1985.
 *
 * @ingroup runtime
 *
 * @defgroup image Image Processing
 * @ingroup runtime
 * @{
 */

#pragma once

#include "platform/platform.h"

/// @brief RGB pixel with 8-bit color channels
struct RGB
{
	UINT8 Red;
	UINT8 Green;
	UINT8 Blue;
};

using PRGB = RGB *;
using PCRGB = const RGB *;

/// @brief 2D point with row/column indices
struct Point
{
	INT32 Row;
	INT32 Col;
};

using PPoint = Point *;
using PPPoint = Point **;

/// @brief Contour: array of points and its size
struct Contour
{
	PPoint Points;
	INT32 Count;
};

using PContour = Contour *;
using PPContour = Contour **;

/// @brief Border descriptor used during contour tracing
struct Border
{
	INT32 SeqNum;
	INT32 BorderType;
};

/// @brief Hierarchy node for contour parent-child relationships
struct ContourNode
{
	Border NodeBorder;
	INT32 Parent;
	INT32 FirstChild;
	INT32 NextSibling;
};

using PContourNode = ContourNode *;
using PPContourNode = ContourNode **;

/// @brief Result of contour detection containing contours and hierarchy
struct ContourResult
{
	PContour Contours;
	INT32 ContourCount;
	PContourNode Hierarchy;
	INT32 HierarchyCount;

	/// @brief Free all allocated memory owned by this result
	VOID Free()
	{
		if (Contours)
		{
			for (INT32 i = 0; i < ContourCount; i++)
				delete[] Contours[i].Points;
			delete[] Contours;
			Contours = nullptr;
		}
		if (Hierarchy)
		{
			delete[] Hierarchy;
			Hierarchy = nullptr;
		}
	}
};

/**
 * @class ImageProcessor
 * @brief Image processing utilities: contour detection, differencing, noise removal
 *
 * @details All methods are static and stateless. The class is stack-only
 * (no heap allocation of the class itself).
 */
class ImageProcessor
{
public:
	VOID *operator new(USIZE) = delete;
	VOID *operator new[](USIZE) = delete;
	VOID operator delete(VOID *) = delete;
	VOID operator delete[](VOID *) = delete;

	/**
	 * @brief Find contours in a binary image using the Suzuki-Abe algorithm
	 *
	 * @details Implements topological structural analysis of a digitized binary
	 * image by border following. The image is modified in-place (border pixels
	 * are relabeled with sequence numbers).
	 *
	 * @param image Binary image buffer (signed, modified in-place)
	 * @param numRows Number of rows in the image
	 * @param numCols Number of columns in the image
	 * @return Ok(ContourResult) on success, or error on allocation failure
	 *
	 * @see Suzuki & Abe, CVGIP 30(1), 1985
	 */
	[[nodiscard]] static Result<ContourResult, Error> FindContours(
		Span<CHAR> image,
		INT32 numRows,
		INT32 numCols);

	/**
	 * @brief Calculate per-pixel binary difference between two RGB images
	 *
	 * @param image1 First image (width * height RGB pixels)
	 * @param image2 Second image (same dimensions)
	 * @param width Image width in pixels
	 * @param height Image height in pixels
	 * @param biDiff Output: 1 where pixels differ, 0 where identical
	 */
	static VOID CalculateBiDifference(
		Span<const RGB> image1,
		Span<const RGB> image2,
		UINT32 width,
		UINT32 height,
		Span<UINT8> biDiff);

	/**
	 * @brief Remove noise from a binary difference image using block averaging
	 *
	 * @details Divides the image into blocks of size (max(w,h)/32) and sets
	 * each block to 1 if any pixel in it is nonzero, 0 otherwise.
	 *
	 * @param biDiff Binary difference image (modified in-place)
	 * @param width Image width in pixels
	 * @param height Image height in pixels
	 */
	static VOID RemoveNoise(
		Span<UINT8> biDiff,
		UINT32 width,
		UINT32 height);
};

/** @} */ // end of image group
