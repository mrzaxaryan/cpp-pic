#pragma once

#include "result.h"
#include "error.h"

namespace result
{

/// UEFI EFI_STATUS: success when (SSIZE)status >= 0.
template <typename T>
[[nodiscard]] FORCE_INLINE Result<T, Error> FromEfiStatus(USIZE status) noexcept
{
	if ((SSIZE)status >= 0)
	{
		if constexpr (__is_same_as(T, void))
			return Result<T, Error>::Ok();
		else
			return Result<T, Error>::Ok((T)status);
	}
	return Result<T, Error>::Err(Error::Uefi((UINT32)status));
}

} // namespace result
