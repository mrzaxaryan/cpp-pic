#pragma once

#include "result.h"
#include "error.h"

namespace result
{

/// macOS syscall: success when result >= 0, failure stores -result as errno.
template <typename T>
[[nodiscard]] FORCE_INLINE Result<T, Error> FromMacOS(SSIZE result) noexcept
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
