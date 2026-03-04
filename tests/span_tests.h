#pragma once

#include "runtime/runtime.h"
#include "tests.h"

class SpanTests
{
public:
	static BOOL RunAll()
	{
		BOOL allPassed = true;

		LOG_INFO("Running Span Tests...");

		// Dynamic extent
		RunTest(allPassed, EMBED_FUNC(TestDynamicDefault), "Dynamic default construction"_embed);
		RunTest(allPassed, EMBED_FUNC(TestDynamicPtrSize), "Dynamic pointer+size construction"_embed);
		RunTest(allPassed, EMBED_FUNC(TestDynamicArray), "Dynamic array construction"_embed);
		RunTest(allPassed, EMBED_FUNC(TestDynamicAccessors), "Dynamic accessors"_embed);
		RunTest(allPassed, EMBED_FUNC(TestDynamicSubspan), "Dynamic Subspan"_embed);
		RunTest(allPassed, EMBED_FUNC(TestDynamicFirstLast), "Dynamic First/Last"_embed);
		RunTest(allPassed, EMBED_FUNC(TestDynamicIteration), "Dynamic range-for iteration"_embed);
		RunTest(allPassed, EMBED_FUNC(TestDynamicConstConversion), "Dynamic Span<T> to Span<const T>"_embed);

		// Static extent
		RunTest(allPassed, EMBED_FUNC(TestStaticArray), "Static array construction"_embed);
		RunTest(allPassed, EMBED_FUNC(TestStaticAccessors), "Static accessors"_embed);
		RunTest(allPassed, EMBED_FUNC(TestStaticSubspan), "Static Subspan returns dynamic"_embed);
		RunTest(allPassed, EMBED_FUNC(TestStaticFirstLast), "Static First/Last return dynamic"_embed);
		RunTest(allPassed, EMBED_FUNC(TestStaticIteration), "Static range-for iteration"_embed);
		RunTest(allPassed, EMBED_FUNC(TestStaticConstConversion), "Static Span<T,N> to Span<const T,N>"_embed);

		// Cross-conversion
		RunTest(allPassed, EMBED_FUNC(TestStaticToDynamic), "Span<T,N> to Span<T> conversion"_embed);
		RunTest(allPassed, EMBED_FUNC(TestStaticToDynamicConst), "Span<T,N> to Span<const T> conversion"_embed);
		RunTest(allPassed, EMBED_FUNC(TestStaticPassToFunction), "Pass Span<T,N> to function taking Span<T>"_embed);

		// Size optimization
		RunTest(allPassed, EMBED_FUNC(TestSizeOptimization), "Static extent eliminates size member"_embed);

		// Compile-time slicing — static extent
		RunTest(allPassed, EMBED_FUNC(TestStaticPtrConstruction), "Static explicit pointer construction"_embed);
		RunTest(allPassed, EMBED_FUNC(TestStaticCompileTimeFirst), "Static First<N>() returns Span<T,N>"_embed);
		RunTest(allPassed, EMBED_FUNC(TestStaticCompileTimeLast), "Static Last<N>() returns Span<T,N>"_embed);
		RunTest(allPassed, EMBED_FUNC(TestStaticCompileTimeSubspanOffsetCount), "Static Subspan<O,N>() returns Span<T,N>"_embed);
		RunTest(allPassed, EMBED_FUNC(TestStaticCompileTimeSubspanOffset), "Static Subspan<O>() deduces count from type"_embed);

		// Compile-time slicing — dynamic extent
		RunTest(allPassed, EMBED_FUNC(TestDynamicCompileTimeFirst), "Dynamic First<N>() returns Span<T,N>"_embed);
		RunTest(allPassed, EMBED_FUNC(TestDynamicCompileTimeLast), "Dynamic Last<N>() returns Span<T,N>"_embed);
		RunTest(allPassed, EMBED_FUNC(TestDynamicCompileTimeSubspan), "Dynamic Subspan<O,N>() returns Span<T,N>"_embed);

		// Edge cases
		RunTest(allPassed, EMBED_FUNC(TestEmptyDynamic), "Empty dynamic span"_embed);
		RunTest(allPassed, EMBED_FUNC(TestSingleElement), "Single element static span"_embed);

		if (allPassed)
			LOG_INFO("All Span tests passed!");
		else
			LOG_ERROR("Some Span tests failed!");

		return allPassed;
	}

private:
	// Helper: accepts Span<const UINT8> (dynamic) to verify implicit conversions
	static USIZE SumBytes(Span<const UINT8> data)
	{
		USIZE sum = 0;
		for (USIZE i = 0; i < data.Size(); i++)
			sum += data[i];
		return sum;
	}

	// =====================================================================
	// Dynamic extent
	// =====================================================================

	static BOOL TestDynamicDefault()
	{
		Span<UINT8> s;
		if (s.Data() != nullptr)
		{
			LOG_ERROR("Default Data() != nullptr");
			return false;
		}
		if (s.Size() != 0 || !s.IsEmpty())
		{
			LOG_ERROR("Default Size() != 0 or not empty");
			return false;
		}
		return true;
	}

	static BOOL TestDynamicPtrSize()
	{
		UINT8 buf[4];
		buf[0] = 1; buf[1] = 2; buf[2] = 3; buf[3] = 4;
		Span<UINT8> s(buf, 4);
		if (s.Data() != buf)
		{
			LOG_ERROR("Data() != buf");
			return false;
		}
		if (s.Size() != 4 || s.IsEmpty())
		{
			LOG_ERROR("Size() != 4 or empty");
			return false;
		}
		return true;
	}

	static BOOL TestDynamicArray()
	{
		UINT8 buf[8];
		buf[0] = 42;
		Span<UINT8> s(buf);
		if (s.Data() != buf || s.Size() != 8)
		{
			LOG_ERROR("Array construction: Data or Size mismatch");
			return false;
		}
		return true;
	}

	static BOOL TestDynamicAccessors()
	{
		UINT8 buf[3];
		buf[0] = 10; buf[1] = 20; buf[2] = 30;
		Span<UINT8> s(buf, 3);
		if (s[0] != 10 || s[1] != 20 || s[2] != 30)
		{
			LOG_ERROR("operator[] mismatch");
			return false;
		}
		if (s.SizeBytes() != 3 * sizeof(UINT8))
		{
			LOG_ERROR("SizeBytes() mismatch");
			return false;
		}
		return true;
	}

	static BOOL TestDynamicSubspan()
	{
		UINT8 buf[5];
		buf[0] = 1; buf[1] = 2; buf[2] = 3; buf[3] = 4; buf[4] = 5;
		Span<UINT8> s(buf, 5);

		Span<UINT8> sub = s.Subspan(2);
		if (sub.Size() != 3 || sub[0] != 3)
		{
			LOG_ERROR("Subspan(offset) failed");
			return false;
		}

		Span<UINT8> sub2 = s.Subspan(1, 2);
		if (sub2.Size() != 2 || sub2[0] != 2 || sub2[1] != 3)
		{
			LOG_ERROR("Subspan(offset, count) failed");
			return false;
		}
		return true;
	}

	static BOOL TestDynamicFirstLast()
	{
		UINT8 buf[4];
		buf[0] = 1; buf[1] = 2; buf[2] = 3; buf[3] = 4;
		Span<UINT8> s(buf, 4);

		Span<UINT8> first = s.First(2);
		if (first.Size() != 2 || first[0] != 1 || first[1] != 2)
		{
			LOG_ERROR("First(2) failed");
			return false;
		}

		Span<UINT8> last = s.Last(2);
		if (last.Size() != 2 || last[0] != 3 || last[1] != 4)
		{
			LOG_ERROR("Last(2) failed");
			return false;
		}
		return true;
	}

	static BOOL TestDynamicIteration()
	{
		UINT8 buf[3];
		buf[0] = 10; buf[1] = 20; buf[2] = 30;
		Span<UINT8> s(buf, 3);
		USIZE sum = 0;
		for (UINT8 v : s)
			sum += v;
		if (sum != 60)
		{
			LOG_ERROR("Iteration sum mismatch: expected 60");
			return false;
		}
		return true;
	}

	static BOOL TestDynamicConstConversion()
	{
		UINT8 buf[4];
		buf[0] = 5; buf[1] = 10; buf[2] = 15; buf[3] = 20;
		Span<UINT8> writable(buf, 4);
		Span<const UINT8> readable = writable;
		if (readable.Size() != 4 || readable[0] != 5)
		{
			LOG_ERROR("Dynamic const conversion failed");
			return false;
		}
		return true;
	}

	// =====================================================================
	// Static extent
	// =====================================================================

	static BOOL TestStaticArray()
	{
		UINT8 buf[4];
		buf[0] = 1; buf[1] = 2; buf[2] = 3; buf[3] = 4;
		Span<UINT8, 4> s(buf);
		if (s.Data() != buf || s.Size() != 4 || s.IsEmpty())
		{
			LOG_ERROR("Static array construction failed");
			return false;
		}
		return true;
	}

	static BOOL TestStaticAccessors()
	{
		UINT8 buf[3];
		buf[0] = 10; buf[1] = 20; buf[2] = 30;
		Span<UINT8, 3> s(buf);
		if (s.Size() != 3 || s.SizeBytes() != 3 || s[1] != 20)
		{
			LOG_ERROR("Static accessors mismatch");
			return false;
		}
		return true;
	}

	static BOOL TestStaticSubspan()
	{
		UINT8 buf[5];
		buf[0] = 1; buf[1] = 2; buf[2] = 3; buf[3] = 4; buf[4] = 5;
		Span<UINT8, 5> s(buf);

		Span<UINT8> sub = s.Subspan(2);
		if (sub.Size() != 3 || sub[0] != 3)
		{
			LOG_ERROR("Static Subspan(offset) failed");
			return false;
		}

		Span<UINT8> sub2 = s.Subspan(1, 2);
		if (sub2.Size() != 2 || sub2[0] != 2)
		{
			LOG_ERROR("Static Subspan(offset, count) failed");
			return false;
		}
		return true;
	}

	static BOOL TestStaticFirstLast()
	{
		UINT8 buf[4];
		buf[0] = 1; buf[1] = 2; buf[2] = 3; buf[3] = 4;
		Span<UINT8, 4> s(buf);

		Span<UINT8> first = s.First(2);
		if (first.Size() != 2 || first[0] != 1 || first[1] != 2)
		{
			LOG_ERROR("Static First(2) failed");
			return false;
		}

		Span<UINT8> last = s.Last(2);
		if (last.Size() != 2 || last[0] != 3 || last[1] != 4)
		{
			LOG_ERROR("Static Last(2) failed");
			return false;
		}
		return true;
	}

	static BOOL TestStaticIteration()
	{
		UINT8 buf[3];
		buf[0] = 10; buf[1] = 20; buf[2] = 30;
		Span<UINT8, 3> s(buf);
		USIZE sum = 0;
		for (UINT8 v : s)
			sum += v;
		if (sum != 60)
		{
			LOG_ERROR("Static iteration sum mismatch: expected 60");
			return false;
		}
		return true;
	}

	static BOOL TestStaticConstConversion()
	{
		UINT8 buf[4];
		buf[0] = 5;
		Span<UINT8, 4> writable(buf);
		Span<const UINT8, 4> readable = writable;
		if (readable.Size() != 4 || readable[0] != 5)
		{
			LOG_ERROR("Static const conversion failed");
			return false;
		}
		return true;
	}

	// =====================================================================
	// Cross-conversion
	// =====================================================================

	static BOOL TestStaticToDynamic()
	{
		UINT8 buf[4];
		buf[0] = 1; buf[1] = 2; buf[2] = 3; buf[3] = 4;
		Span<UINT8, 4> staticSpan(buf);
		Span<UINT8> dynamicSpan = staticSpan;
		if (dynamicSpan.Size() != 4 || dynamicSpan[0] != 1)
		{
			LOG_ERROR("Static-to-dynamic conversion failed");
			return false;
		}
		return true;
	}

	static BOOL TestStaticToDynamicConst()
	{
		UINT8 buf[4];
		buf[0] = 1;
		Span<UINT8, 4> staticSpan(buf);
		Span<const UINT8> readOnly = staticSpan;
		if (readOnly.Size() != 4 || readOnly[0] != 1)
		{
			LOG_ERROR("Static-to-dynamic-const conversion failed");
			return false;
		}
		return true;
	}

	static BOOL TestStaticPassToFunction()
	{
		UINT8 buf[3];
		buf[0] = 10; buf[1] = 20; buf[2] = 30;
		Span<UINT8, 3> staticSpan(buf);
		// SumBytes takes Span<const UINT8> (dynamic) -- implicit conversion
		USIZE result = SumBytes(staticSpan);
		if (result != 60)
		{
			LOG_ERROR("Static pass to function: expected 60, got %zu", result);
			return false;
		}
		return true;
	}

	// =====================================================================
	// Size optimization
	// =====================================================================

	static BOOL TestSizeOptimization()
	{
		// Static extent stores only pointer (no m_size member)
		static_assert(sizeof(Span<UINT8, 4>) == sizeof(UINT8 *));
		// Dynamic extent stores pointer + size
		static_assert(sizeof(Span<UINT8>) == sizeof(UINT8 *) + sizeof(USIZE));
		return true;
	}

	// =====================================================================
	// Edge cases
	// =====================================================================

	static BOOL TestEmptyDynamic()
	{
		Span<UINT8> empty;
		if (!empty.IsEmpty() || empty.Size() != 0 || empty.Data() != nullptr)
		{
			LOG_ERROR("Empty dynamic span check failed");
			return false;
		}
		return true;
	}

	static BOOL TestSingleElement()
	{
		UINT8 val[1];
		val[0] = 42;
		Span<UINT8, 1> s(val);
		if (s.Size() != 1 || s[0] != 42 || s.IsEmpty())
		{
			LOG_ERROR("Single element static span check failed");
			return false;
		}
		return true;
	}

	// =====================================================================
	// Compile-time slicing — static extent
	// =====================================================================

	static BOOL TestStaticPtrConstruction()
	{
		UINT8 buf[4];
		buf[0] = 7; buf[1] = 8; buf[2] = 9; buf[3] = 10;
		UINT8 *ptr = buf;
		Span<UINT8, 4> s(ptr);  // explicit pointer constructor
		if (s.Data() != buf || s.Size() != 4 || s[0] != 7)
		{
			LOG_ERROR("Static explicit pointer construction failed");
			return false;
		}
		static_assert(sizeof(s) == sizeof(UINT8 *), "Static span from pointer must store only pointer");
		return true;
	}

	static BOOL TestStaticCompileTimeFirst()
	{
		UINT8 buf[8];
		buf[0] = 1; buf[1] = 2; buf[2] = 3; buf[3] = 4;
		buf[4] = 5; buf[5] = 6; buf[6] = 7; buf[7] = 8;
		Span<UINT8, 8> s(buf);

		Span<UINT8, 3> first = s.First<3>();
		static_assert(sizeof(first) == sizeof(UINT8 *), "First<N>() must return pointer-only span");
		if (first.Size() != 3 || first[0] != 1 || first[2] != 3)
		{
			LOG_ERROR("Static First<3>() failed");
			return false;
		}
		return true;
	}

	static BOOL TestStaticCompileTimeLast()
	{
		UINT8 buf[8];
		buf[0] = 1; buf[1] = 2; buf[2] = 3; buf[3] = 4;
		buf[4] = 5; buf[5] = 6; buf[6] = 7; buf[7] = 8;
		Span<UINT8, 8> s(buf);

		Span<UINT8, 3> last = s.Last<3>();
		static_assert(sizeof(last) == sizeof(UINT8 *), "Last<N>() must return pointer-only span");
		if (last.Size() != 3 || last[0] != 6 || last[2] != 8)
		{
			LOG_ERROR("Static Last<3>() failed");
			return false;
		}
		return true;
	}

	static BOOL TestStaticCompileTimeSubspanOffsetCount()
	{
		UINT8 buf[8];
		buf[0] = 1; buf[1] = 2; buf[2] = 3; buf[3] = 4;
		buf[4] = 5; buf[5] = 6; buf[6] = 7; buf[7] = 8;
		Span<UINT8, 8> s(buf);

		Span<UINT8, 4> mid = s.Subspan<2, 4>();
		static_assert(sizeof(mid) == sizeof(UINT8 *), "Subspan<O,N>() must return pointer-only span");
		if (mid.Size() != 4 || mid[0] != 3 || mid[3] != 6)
		{
			LOG_ERROR("Static Subspan<2,4>() failed");
			return false;
		}
		return true;
	}

	static BOOL TestStaticCompileTimeSubspanOffset()
	{
		UINT8 buf[8];
		buf[0] = 1; buf[1] = 2; buf[2] = 3; buf[3] = 4;
		buf[4] = 5; buf[5] = 6; buf[6] = 7; buf[7] = 8;
		Span<UINT8, 8> s(buf);

		auto tail = s.Subspan<3>();  // Span<UINT8, 8-3> = Span<UINT8, 5>
		static_assert(__is_same_as(decltype(tail), Span<UINT8, 5>), "Subspan<O>() must deduce count from extent");
		static_assert(sizeof(tail) == sizeof(UINT8 *), "Subspan<O>() must return pointer-only span");
		if (tail.Size() != 5 || tail[0] != 4 || tail[4] != 8)
		{
			LOG_ERROR("Static Subspan<3>() failed");
			return false;
		}
		return true;
	}

	// =====================================================================
	// Compile-time slicing — dynamic extent
	// =====================================================================

	static BOOL TestDynamicCompileTimeFirst()
	{
		UINT8 buf[8];
		buf[0] = 10; buf[1] = 20; buf[2] = 30; buf[3] = 40;
		buf[4] = 50; buf[5] = 60; buf[6] = 70; buf[7] = 80;
		Span<UINT8> s(buf, 8);

		Span<UINT8, 4> first = s.First<4>();
		static_assert(sizeof(first) == sizeof(UINT8 *), "Dynamic First<N>() must return pointer-only span");
		if (first.Size() != 4 || first[0] != 10 || first[3] != 40)
		{
			LOG_ERROR("Dynamic First<4>() failed");
			return false;
		}
		return true;
	}

	static BOOL TestDynamicCompileTimeLast()
	{
		UINT8 buf[8];
		buf[0] = 10; buf[1] = 20; buf[2] = 30; buf[3] = 40;
		buf[4] = 50; buf[5] = 60; buf[6] = 70; buf[7] = 80;
		Span<UINT8> s(buf, 8);

		Span<UINT8, 4> last = s.Last<4>();
		static_assert(sizeof(last) == sizeof(UINT8 *), "Dynamic Last<N>() must return pointer-only span");
		if (last.Size() != 4 || last[0] != 50 || last[3] != 80)
		{
			LOG_ERROR("Dynamic Last<4>() failed");
			return false;
		}
		return true;
	}

	static BOOL TestDynamicCompileTimeSubspan()
	{
		UINT8 buf[8];
		buf[0] = 10; buf[1] = 20; buf[2] = 30; buf[3] = 40;
		buf[4] = 50; buf[5] = 60; buf[6] = 70; buf[7] = 80;
		Span<UINT8> s(buf, 8);

		Span<UINT8, 3> mid = s.Subspan<2, 3>();
		static_assert(sizeof(mid) == sizeof(UINT8 *), "Dynamic Subspan<O,N>() must return pointer-only span");
		if (mid.Size() != 3 || mid[0] != 30 || mid[2] != 50)
		{
			LOG_ERROR("Dynamic Subspan<2,3>() failed");
			return false;
		}
		return true;
	}
};
