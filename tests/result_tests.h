#pragma once

#include "runtime.h"
#include "tests.h"

class ResultTests
{
public:
	static BOOL RunAll()
	{
		BOOL allPassed = true;

		LOG_INFO("Running Result Tests...");


		if (allPassed)
			LOG_INFO("All Result tests passed!");
		else
			LOG_ERROR("Some Result tests failed!");

		return allPassed;
	}

private:
	struct Tracked
	{
		UINT32 value;
		BOOL *destroyed;

		Tracked(UINT32 v, BOOL *flag) : value(v), destroyed(flag) {}
		~Tracked()
		{
			if (destroyed)
				*destroyed = true;
		}

		Tracked(Tracked &&other) noexcept
			: value(other.value), destroyed(other.destroyed)
		{
			other.destroyed = nullptr;
		}

		Tracked &operator=(Tracked &&other) noexcept
		{
			value = other.value;
			destroyed = other.destroyed;
			other.destroyed = nullptr;
			return *this;
		}

		Tracked(const Tracked &) = delete;
		Tracked &operator=(const Tracked &) = delete;
	};

	
};
