/**
 * @file image_processor.cc
 * @brief Image Processing Utilities — Implementation
 *
 * @details Implements the Suzuki-Abe border following algorithm for contour
 * detection in binary images, binary image differencing, and block-based
 * noise removal.
 *
 * @see S. Suzuki and K. Abe, "Topological Structural Analysis of Digitized
 *      Binary Images by Border Following," CVGIP 30(1):32-46, 1985.
 */

#include "runtime/image/image_processor.h"
#include "runtime/vector.h"

// ============================================================
//  Constants
// ============================================================

static constexpr INT32 HoleBorder = 1;
static constexpr INT32 OuterBorder = 2;

static VOID ResetNode(ContourNode *n)
{
	n->Parent = -1;
	n->FirstChild = -1;
	n->NextSibling = -1;
}

// ============================================================
//  Geometry helpers
// ============================================================

[[nodiscard]] static BOOL SamePoint(Point a, Point b)
{
	return a.Row == b.Row && a.Col == b.Col;
}

/// @brief Mark a neighbor as examined based on its position relative to the center point
/// @param mark Point being examined
/// @param center Center point of reference
/// @param checked Array of booleans indicating which directions have been checked
/// @return void
static VOID MarkExamined(Point mark, Point center, BOOL checked[4])
{
	//    3
	//  2 x 0
	//    1
	INT32 loc = -1;
	if (mark.Col > center.Col)
		loc = 0;
	else if (mark.Col < center.Col)
		loc = 2;
	else if (mark.Row > center.Row)
		loc = 1;
	else if (mark.Row < center.Row)
		loc = 3;

	if (loc == -1)
		return;

	checked[loc] = true;
}

/// @brief Step clockwise around a pivot point
/// @param current Current point to be updated
/// @param pivot Pivot point around which to step
/// @return void
static VOID StepCW(Point *current, Point pivot)
{
	if (current->Col > pivot.Col)
	{
		current->Col = pivot.Col;
		current->Row = pivot.Row + 1;
	}
	else if (current->Col < pivot.Col)
	{
		current->Col = pivot.Col;
		current->Row = pivot.Row - 1;
	}
	else if (current->Row > pivot.Row)
	{
		current->Col = pivot.Col - 1;
		current->Row = pivot.Row;
	}
	else if (current->Row < pivot.Row)
	{
		current->Col = pivot.Col + 1;
		current->Row = pivot.Row;
	}
}

/// @brief Step counter-clockwise around a pivot point
/// @param current Current point to be updated
/// @param pivot Pivot point around which to step
/// @return void
static VOID StepCCW(Point *current, Point pivot)
{
	if (current->Col > pivot.Col)
	{
		current->Col = pivot.Col;
		current->Row = pivot.Row - 1;
	}
	else if (current->Col < pivot.Col)
	{
		current->Col = pivot.Col;
		current->Row = pivot.Row + 1;
	}
	else if (current->Row > pivot.Row)
	{
		current->Col = pivot.Col + 1;
		current->Row = pivot.Row;
	}
	else if (current->Row < pivot.Row)
	{
		current->Col = pivot.Col - 1;
		current->Row = pivot.Row;
	}
}

// ============================================================
//  Border following (Suzuki-Abe algorithm, step 3)
// ============================================================

/**
 * @brief Follow a single border starting from (row, col) with neighbor p2
 *
 * @details Implements step 3 of the Suzuki-Abe algorithm: traces the border
 * of a connected component, recording each boundary pixel.
 *
 * @param image Binary image span (modified in-place with border labels)
 * @param numRows Number of image rows
 * @param numCols Number of image columns
 * @param row Starting row of the border
 * @param col Starting column of the border
 * @param p2 Initial neighbor pixel
 * @param nbd Current border sequential number and type
 * @param contourVector Output vector of contour point arrays
 * @param contourCounter Output vector of contour point counts
 * @return true on success, false on allocation failure
 */
[[nodiscard]] static BOOL FollowBorder(
	Span<INT8> image,
	INT32 numRows,
	INT32 numCols,
	INT32 row,
	INT32 col,
	Point p2,
	Border nbd,
	Vector<PPoint> *contourVector,
	Vector<INT32> *contourCounter)
{
	Point current;
	current.Row = p2.Row;
	current.Col = p2.Col;

	Point start;
	start.Row = row;
	start.Col = col;

	// (3.1) Starting from p2, look CW around start for a nonzero pixel.
	// If none found, assign -NBD to the pixel and return.
	do
	{
		StepCW(&current, start);
		if (SamePoint(current, p2))
		{
			image[start.Row * numCols + start.Col] = (INT8)(-nbd.SeqNum);
			PPoint temp = new Point[1];
			if (temp == nullptr)
				return false;
			temp[0] = start;
			if (!contourVector->Add(temp))
			{
				delete[] temp;
				return false;
			}
			return contourCounter->Add(1);
		}
	} while ((current.Col >= numCols || current.Row >= numRows ||
			  current.Col < 0 || current.Row < 0) ||
			 image[current.Row * numCols + current.Col] == 0);

	Vector<Point> pointStorage;
	if (!pointStorage.Init())
		return false;

	Point p1 = current;

	// (3.2) (i2,j2) <- (i1,j1) and (i3,j3) <- (i,j)
	Point p3 = start;
	Point p4;
	p2 = p1;
	BOOL checked[4];

	while (true)
	{
		// (3.3) Starting from the next element of p2 in CCW order,
		// examine CCW the neighborhood of p3 to find a nonzero pixel p4.
		current = p2;

		for (INT32 i = 0; i < 4; i++)
			checked[i] = false;

		do
		{
			MarkExamined(current, p3, checked);
			StepCCW(&current, p3);
		} while ((current.Col >= numCols || current.Row >= numRows ||
				  current.Col < 0 || current.Row < 0) ||
				 image[current.Row * numCols + current.Col] == 0);

		p4 = current;

		// (3.4) Update pixel value at p3
		if ((p3.Col + 1 >= numCols || image[p3.Row * numCols + p3.Col + 1] == 0) && checked[0])
		{
			image[p3.Row * numCols + p3.Col] = (INT8)(-nbd.SeqNum);
		}
		else if (p3.Col + 1 < numCols && image[p3.Row * numCols + p3.Col] == 1)
		{
			image[p3.Row * numCols + p3.Col] = (INT8)nbd.SeqNum;
		}

		if (!pointStorage.Add(p3))
			return false;

		// (3.5) If p4 == start and p3 == p1, we have completed the border.
		if (SamePoint(start, p4) && SamePoint(p1, p3))
		{
			// Transfer ownership of point data to contour vector
			PPoint contourPoints = new Point[(USIZE)pointStorage.Count];
			if (contourPoints == nullptr)
				return false;
			Memory::Copy(contourPoints, pointStorage.Data, sizeof(Point) * (USIZE)pointStorage.Count);
			if (!contourVector->Add(contourPoints))
			{
				delete[] contourPoints;
				return false;
			}
			return contourCounter->Add(pointStorage.Count);
		}

		p2 = p3;
		p3 = p4;
	}
}

// ============================================================
//  Absolute value helper (no CRT dependency)
// ============================================================

[[nodiscard]] static constexpr INT32 AbsInt(INT32 x)
{
	return x < 0 ? -x : x;
}

// ============================================================
//  Public API
// ============================================================

[[nodiscard]] Result<ContourResult, Error> ImageProcessor::FindContours(
	Span<INT8> image,
	INT32 numRows,
	INT32 numCols)
{
	Border nbd;
	Border lnbd;

	lnbd.BorderType = HoleBorder;
	nbd.BorderType = HoleBorder;
	nbd.SeqNum = 1;

	Vector<ContourNode> hierarchyVector;
	if (!hierarchyVector.Init())
		return Result<ContourResult, Error>::Err(Error::Image_AllocationFailed);

	ContourNode tempNode;
	ResetNode(&tempNode);
	tempNode.NodeBorder = nbd;
	if (!hierarchyVector.Add(tempNode))
		return Result<ContourResult, Error>::Err(Error::Image_AllocationFailed);

	// Add padding so contour and hierarchy have the same offset
	Vector<PPoint> contourVector;
	if (!contourVector.Init())
		return Result<ContourResult, Error>::Err(Error::Image_AllocationFailed);

	PPoint padding = new Point[1];
	if (padding == nullptr)
		return Result<ContourResult, Error>::Err(Error::Image_AllocationFailed);
	padding[0] = {0, -1};
	if (!contourVector.Add(padding))
	{
		delete[] padding;
		return Result<ContourResult, Error>::Err(Error::Image_AllocationFailed);
	}

	Vector<INT32> contourCounter;
	if (!contourCounter.Init())
	{
		for (INT32 i = 0; i < contourVector.Count; i++)
			delete[] contourVector.Data[i];
		return Result<ContourResult, Error>::Err(Error::Image_AllocationFailed);
	}
	if (!contourCounter.Add(1))
	{
		for (INT32 i = 0; i < contourVector.Count; i++)
			delete[] contourVector.Data[i];
		return Result<ContourResult, Error>::Err(Error::Image_AllocationFailed);
	}

	Point p2;
	BOOL borderStartFound;

	for (INT32 r = 0; r < numRows; r++)
	{
		lnbd.SeqNum = 1;
		lnbd.BorderType = HoleBorder;
		for (INT32 c = 0; c < numCols; c++)
		{
			borderStartFound = false;

			// Phase 1: Find border
			// If f(i,j)==1 and f(i,j-1)==0, outer border starting point
			if ((image[r * numCols + c] == 1 && c - 1 < 0) ||
				(image[r * numCols + c] == 1 && image[r * numCols + c - 1] == 0))
			{
				nbd.BorderType = OuterBorder;
				nbd.SeqNum += 1;
				p2.Row = r;
				p2.Col = c - 1;
				borderStartFound = true;
			}
			// If f(i,j)>=1 and f(i,j+1)==0, hole border starting point
			else if (c + 1 < numCols &&
					 (image[r * numCols + c] >= 1 && image[r * numCols + c + 1] == 0))
			{
				nbd.BorderType = HoleBorder;
				nbd.SeqNum += 1;
				if (image[r * numCols + c] > 1)
				{
					lnbd.SeqNum = image[r * numCols + c];
					lnbd.BorderType = hierarchyVector.Data[lnbd.SeqNum - 1].NodeBorder.BorderType;
				}
				p2.Row = r;
				p2.Col = c + 1;
				borderStartFound = true;
			}

			if (borderStartFound)
			{
				// Phase 2: Store parent
				ResetNode(&tempNode);
				if (nbd.BorderType == lnbd.BorderType)
				{
					tempNode.Parent = hierarchyVector.Data[lnbd.SeqNum - 1].Parent;
					tempNode.NextSibling = hierarchyVector.Data[tempNode.Parent - 1].FirstChild;
					hierarchyVector.Data[tempNode.Parent - 1].FirstChild = nbd.SeqNum;
					tempNode.NodeBorder = nbd;
					if (!hierarchyVector.Add(tempNode))
					{
						for (INT32 i = 0; i < contourVector.Count; i++)
							delete[] contourVector.Data[i];
						return Result<ContourResult, Error>::Err(Error::Image_AllocationFailed);
					}
				}
				else
				{
					if (hierarchyVector.Data[lnbd.SeqNum - 1].FirstChild != -1)
					{
						tempNode.NextSibling = hierarchyVector.Data[lnbd.SeqNum - 1].FirstChild;
					}
					tempNode.Parent = lnbd.SeqNum;
					hierarchyVector.Data[lnbd.SeqNum - 1].FirstChild = nbd.SeqNum;
					tempNode.NodeBorder = nbd;
					if (!hierarchyVector.Add(tempNode))
					{
						for (INT32 i = 0; i < contourVector.Count; i++)
							delete[] contourVector.Data[i];
						return Result<ContourResult, Error>::Err(Error::Image_AllocationFailed);
					}
				}

				// Phase 3: Follow border
				if (!FollowBorder(image, numRows, numCols, r, c, p2, nbd,
								  &contourVector, &contourCounter))
				{
					for (INT32 i = 0; i < contourVector.Count; i++)
						delete[] contourVector.Data[i];
					return Result<ContourResult, Error>::Err(Error::Image_AllocationFailed);
				}
			}

			// Phase 4: Continue to next border
			if (AbsInt(image[r * numCols + c]) > 1)
			{
				lnbd.SeqNum = AbsInt(image[r * numCols + c]);
				lnbd.BorderType = hierarchyVector.Data[lnbd.SeqNum - 1].NodeBorder.BorderType;
			}
		}
	}

	// Build output arrays
	INT32 totalContours = contourVector.Count;

	PContour outContours = new Contour[(USIZE)totalContours];
	if (outContours == nullptr)
	{
		for (INT32 i = 0; i < contourVector.Count; i++)
			delete[] contourVector.Data[i];
		return Result<ContourResult, Error>::Err(Error::Image_AllocationFailed);
	}

	for (INT32 i = 0; i < totalContours; i++)
	{
		outContours[i].Points = contourVector.Data[i];
		outContours[i].Count = contourCounter.Data[i];
	}

	// Transfer hierarchy ownership
	PContourNode outHierarchy = new ContourNode[(USIZE)hierarchyVector.Count];
	if (outHierarchy == nullptr)
	{
		for (INT32 i = 0; i < totalContours; i++)
			delete[] outContours[i].Points;
		delete[] outContours;
		return Result<ContourResult, Error>::Err(Error::Image_AllocationFailed);
	}
	Memory::Copy(outHierarchy, hierarchyVector.Data, sizeof(ContourNode) * (USIZE)hierarchyVector.Count);

	ContourResult result;
	result.Contours = outContours;
	result.ContourCount = totalContours;
	result.Hierarchy = outHierarchy;
	result.HierarchyCount = hierarchyVector.Count;

	delete[] contourVector.Data;
	contourVector.Release();

	return Result<ContourResult, Error>::Ok(result);
}

/// @brief Calculate per-pixel binary difference between two RGB images
/// @param image1 First image (width * height RGB pixels)
/// @param image2 Second image (same dimensions)
/// @param width Image width in pixels
/// @param height Image height in pixels
/// @param biDiff Output binary difference image (width * height)
/// @return void
VOID ImageProcessor::CalculateBiDifference(
	Span<const RGB> image1,
	Span<const RGB> image2,
	UINT32 width,
	UINT32 height,
	Span<UINT8> biDiff)
{
	for (UINT32 i = 0; i < height; ++i)
	{
		for (UINT32 j = 0; j < width; ++j)
		{
			UINT32 idx = i * width + j;
			if (image1[idx].Red != image2[idx].Red ||
				image1[idx].Green != image2[idx].Green ||
				image1[idx].Blue != image2[idx].Blue)
			{
				biDiff[idx] = 1;
			}
			else
			{
				biDiff[idx] = 0;
			}
		}
	}
}

/// @brief Remove noise from a binary difference image using block averaging
/// @param biDiff Binary difference image (modified in-place)
/// @param width Width of the image in pixels
/// @param height Height of the image in pixels
/// @return void
VOID ImageProcessor::RemoveNoise(
	Span<UINT8> biDiff,
	UINT32 width,
	UINT32 height)
{
	UINT32 kernelSize = height < width ? width : height;
	kernelSize = kernelSize / 32;
	if (kernelSize == 0)
		kernelSize = 1;

	for (UINT32 i = 0; i < height; i += kernelSize)
	{
		for (UINT32 j = 0; j < width; j += kernelSize)
		{
			BOOL hasChanged = false;

			for (UINT32 k = 0; k < kernelSize && i + k < height; ++k)
			{
				for (UINT32 l = 0; l < kernelSize && j + l < width; ++l)
				{
					if (biDiff[(i + k) * width + (j + l)] != 0)
					{
						hasChanged = true;
						k = l = kernelSize;
					}
				}
			}

			for (UINT32 k = 0; k < kernelSize && i + k < height; ++k)
			{
				for (UINT32 l = 0; l < kernelSize && j + l < width; ++l)
				{
					biDiff[(i + k) * width + (j + l)] = hasChanged ? 1 : 0;
				}
			}
		}
	}
}
