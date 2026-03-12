#pragma once

#include "runtime/runtime.h"
#include "tests.h"

class VectorTests
{
public:
	static BOOL RunAll()
	{
		BOOL allPassed = true;

		LOG_INFO("Running Vector Tests...");

		// Construction and initialization
		RunTest(allPassed, &TestDefaultConstruction, "Default construction");
		RunTest(allPassed, &TestInit, "Init allocates backing array");

		// Add elements
		RunTest(allPassed, &TestAddSingle, "Add single element");
		RunTest(allPassed, &TestAddMultiple, "Add multiple elements");
		RunTest(allPassed, &TestAddGrow, "Add beyond initial capacity triggers growth");

		// Move semantics
		RunTest(allPassed, &TestMoveConstruct, "Move construction transfers ownership");
		RunTest(allPassed, &TestMoveAssign, "Move assignment transfers ownership");
		RunTest(allPassed, &TestMoveAssignSelf, "Move self-assignment is safe");

		// Release
		RunTest(allPassed, &TestRelease, "Release returns pointer and resets");

		// Edge cases
		RunTest(allPassed, &TestAddAfterGrowPreservesData, "Growth preserves existing data");
		RunTest(allPassed, &TestMultipleGrowths, "Multiple capacity doublings");
		RunTest(allPassed, &TestMoveAssignNonEmpty, "Move assign into non-empty vector");

		if (allPassed)
			LOG_INFO("All Vector tests passed!");
		else
			LOG_ERROR("Some Vector tests failed!");

		return allPassed;
	}

private:
	// =====================================================================
	// Construction and initialization
	// =====================================================================

	static BOOL TestDefaultConstruction()
	{
		Vector<INT32> v;
		if (v.Data != nullptr)
		{
			LOG_ERROR("Default Data != nullptr");
			return false;
		}
		if (v.Capacity != 0 || v.Count != 0)
		{
			LOG_ERROR("Default Capacity/Count != 0");
			return false;
		}
		return true;
	}

	static BOOL TestInit()
	{
		Vector<INT32> v;
		if (!v.Init())
		{
			LOG_ERROR("Init() returned false");
			return false;
		}
		if (v.Data == nullptr)
		{
			LOG_ERROR("Data == nullptr after Init");
			return false;
		}
		if (v.Capacity != VectorInitialCapacity)
		{
			LOG_ERROR("Capacity != VectorInitialCapacity after Init");
			return false;
		}
		if (v.Count != 0)
		{
			LOG_ERROR("Count != 0 after Init");
			return false;
		}
		return true;
	}

	// =====================================================================
	// Add elements
	// =====================================================================

	static BOOL TestAddSingle()
	{
		Vector<INT32> v;
		if (!v.Init()) return false;

		if (!v.Add(42))
		{
			LOG_ERROR("Add(42) returned false");
			return false;
		}
		if (v.Count != 1)
		{
			LOG_ERROR("Count != 1 after Add");
			return false;
		}
		if (v.Data[0] != 42)
		{
			LOG_ERROR("Data[0] != 42");
			return false;
		}
		return true;
	}

	static BOOL TestAddMultiple()
	{
		Vector<INT32> v;
		if (!v.Init()) return false;

		for (INT32 i = 0; i < 5; i++)
		{
			if (!v.Add(i * 10))
			{
				LOG_ERROR("Add failed at index %d", i);
				return false;
			}
		}
		if (v.Count != 5)
		{
			LOG_ERROR("Count != 5");
			return false;
		}
		for (INT32 i = 0; i < 5; i++)
		{
			if (v.Data[i] != i * 10)
			{
				LOG_ERROR("Data[%d] != %d", i, i * 10);
				return false;
			}
		}
		return true;
	}

	static BOOL TestAddGrow()
	{
		Vector<INT32> v;
		if (!v.Init()) return false;

		INT32 initialCap = v.Capacity;

		// Fill to capacity (Add triggers growth when Count + 1 >= Capacity)
		for (INT32 i = 0; i < initialCap; i++)
		{
			if (!v.Add(i))
			{
				LOG_ERROR("Add failed at index %d", i);
				return false;
			}
		}

		if (v.Capacity <= initialCap)
		{
			LOG_ERROR("Capacity did not grow: still %d", v.Capacity);
			return false;
		}
		if (v.Capacity != initialCap * 2)
		{
			LOG_ERROR("Capacity != %d (expected double)", initialCap * 2);
			return false;
		}
		if (v.Count != initialCap)
		{
			LOG_ERROR("Count != %d after filling", initialCap);
			return false;
		}
		return true;
	}

	// =====================================================================
	// Move semantics
	// =====================================================================

	static BOOL TestMoveConstruct()
	{
		Vector<INT32> v;
		if (!v.Init()) return false;
		if (!v.Add(1)) return false;
		if (!v.Add(2)) return false;

		INT32 *origData = v.Data;
		INT32 origCount = v.Count;
		INT32 origCap = v.Capacity;

		Vector<INT32> v2((Vector<INT32> &&)v);

		// Source should be empty
		if (v.Data != nullptr || v.Capacity != 0 || v.Count != 0)
		{
			LOG_ERROR("Source not zeroed after move construct");
			return false;
		}
		// Destination should have the data
		if (v2.Data != origData || v2.Count != origCount || v2.Capacity != origCap)
		{
			LOG_ERROR("Destination does not match original after move construct");
			return false;
		}
		if (v2.Data[0] != 1 || v2.Data[1] != 2)
		{
			LOG_ERROR("Data values incorrect after move construct");
			return false;
		}
		return true;
	}

	static BOOL TestMoveAssign()
	{
		Vector<INT32> v;
		if (!v.Init()) return false;
		if (!v.Add(10)) return false;
		if (!v.Add(20)) return false;

		INT32 *origData = v.Data;

		Vector<INT32> v2;
		v2 = (Vector<INT32> &&)v;

		if (v.Data != nullptr || v.Capacity != 0 || v.Count != 0)
		{
			LOG_ERROR("Source not zeroed after move assign");
			return false;
		}
		if (v2.Data != origData || v2.Count != 2)
		{
			LOG_ERROR("Destination incorrect after move assign");
			return false;
		}
		if (v2.Data[0] != 10 || v2.Data[1] != 20)
		{
			LOG_ERROR("Data values incorrect after move assign");
			return false;
		}
		return true;
	}

	static BOOL TestMoveAssignSelf()
	{
		Vector<INT32> v;
		if (!v.Init()) return false;
		if (!v.Add(99)) return false;

		INT32 *origData = v.Data;
		v = (Vector<INT32> &&)v;

		if (v.Data != origData || v.Count != 1 || v.Data[0] != 99)
		{
			LOG_ERROR("Self move-assign corrupted data");
			return false;
		}
		return true;
	}

	// =====================================================================
	// Release
	// =====================================================================

	static BOOL TestRelease()
	{
		Vector<INT32> v;
		if (!v.Init()) return false;
		if (!v.Add(7)) return false;
		if (!v.Add(8)) return false;

		INT32 *released = v.Release();

		if (released == nullptr)
		{
			LOG_ERROR("Release() returned nullptr");
			return false;
		}
		if (released[0] != 7 || released[1] != 8)
		{
			LOG_ERROR("Released data values incorrect");
			delete[] released;
			return false;
		}
		if (v.Data != nullptr || v.Capacity != 0 || v.Count != 0)
		{
			LOG_ERROR("Vector not reset after Release()");
			delete[] released;
			return false;
		}

		delete[] released;
		return true;
	}

	// =====================================================================
	// Edge cases
	// =====================================================================

	static BOOL TestAddAfterGrowPreservesData()
	{
		Vector<INT32> v;
		if (!v.Init()) return false;

		// Fill past initial capacity
		for (INT32 i = 0; i < VectorInitialCapacity + 5; i++)
		{
			if (!v.Add(i * 3))
			{
				LOG_ERROR("Add failed at index %d", i);
				return false;
			}
		}

		// Verify all data is intact
		for (INT32 i = 0; i < VectorInitialCapacity + 5; i++)
		{
			if (v.Data[i] != i * 3)
			{
				LOG_ERROR("Data[%d] = %d, expected %d", i, v.Data[i], i * 3);
				return false;
			}
		}
		return true;
	}

	static BOOL TestMultipleGrowths()
	{
		Vector<INT32> v;
		if (!v.Init()) return false;

		// Add enough elements to trigger multiple doublings
		// Initial = 10, then 20, 40, 80 => need > 40 elements for 3 growths
		INT32 target = VectorInitialCapacity * 4 + 1;
		for (INT32 i = 0; i < target; i++)
		{
			if (!v.Add(i))
			{
				LOG_ERROR("Add failed at index %d", i);
				return false;
			}
		}

		if (v.Count != target)
		{
			LOG_ERROR("Count = %d, expected %d", v.Count, target);
			return false;
		}

		// Spot check some values
		if (v.Data[0] != 0 || v.Data[target - 1] != target - 1)
		{
			LOG_ERROR("Data mismatch after multiple growths");
			return false;
		}
		return true;
	}

	static BOOL TestMoveAssignNonEmpty()
	{
		Vector<INT32> v1;
		if (!v1.Init()) return false;
		if (!v1.Add(100)) return false;

		Vector<INT32> v2;
		if (!v2.Init()) return false;
		if (!v2.Add(200)) return false;
		if (!v2.Add(300)) return false;

		// Move v1 into v2 — v2's old data should be freed
		v2 = (Vector<INT32> &&)v1;

		if (v2.Count != 1 || v2.Data[0] != 100)
		{
			LOG_ERROR("v2 should have v1's data after move");
			return false;
		}
		if (v1.Data != nullptr || v1.Count != 0)
		{
			LOG_ERROR("v1 not zeroed after move");
			return false;
		}
		return true;
	}
};
