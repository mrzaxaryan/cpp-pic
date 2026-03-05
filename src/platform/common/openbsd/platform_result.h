/**
 * @file platform_result.h
 * @brief OpenBSD syscall result to Result<T, Error> conversion.
 *
 * @details Provides the FromOpenBSD() helper that converts raw OpenBSD BSD
 * syscall return values into the PIR Result<T, Error> type. After the
 * carry-flag negation in the System::Call wrappers, negative return values
 * indicate errors; this function maps them to Result::Err with a POSIX
 * error code.
 */
#pragma once

#include "core/types/result.h"
#include "core/types/error.h"

namespace result
{

/// OpenBSD syscall: success when result >= 0, failure stores -result as errno.
template <typename T>
[[nodiscard]] FORCE_INLINE Result<T, Error> FromOpenBSD(SSIZE result) noexcept
{
	if (result >= 0)
	{
		if constexpr (__is_same_as(T, void))
			return Result<T, Error>::Ok();
		else
			return Result<T, Error>::Ok((T)result);
	}
	return Result<T, Error>::Err(Error::Posix((UINT32)(-result)));
}

} // namespace result
