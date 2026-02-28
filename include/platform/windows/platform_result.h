#pragma once

#include "result.h"
#include "error.h"

namespace result
{

/// Windows NTSTATUS: success when status >= 0 (NT_SUCCESS semantics).
template <typename T>
[[nodiscard]] FORCE_INLINE Result<T, Error> FromNTSTATUS(INT32 status) noexcept
{
	if (status >= 0)
	{
		if constexpr (__is_same_as(T, void))
			return Result<T, Error>::Ok();
		else
			return Result<T, Error>::Ok((T)status);
	}
	return Result<T, Error>::Err(Error::Windows((UINT32)status));
}

} // namespace result
