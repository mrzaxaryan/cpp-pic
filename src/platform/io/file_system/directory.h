/**
 * @file directory.h
 * @brief Directory operations
 *
 * @details Provides static methods for creating and deleting directories
 * across platforms. All operations return Result<void, Error> for uniform
 * error handling via the PIR Result type.
 */
#pragma once

#include "core/types/primitives.h"
#include "core/types/error.h"
#include "core/types/result.h"
class Directory
{
public:
	/**
	 * @brief Creates a directory at the given path.
	 * @param path Null-terminated wide string directory path.
	 * @return Void on success, or an Error on failure.
	 */
	[[nodiscard]] static Result<void, Error> Create(PCWCHAR path);

	/**
	 * @brief Deletes a directory at the given path.
	 * @param path Null-terminated wide string directory path.
	 * @return Void on success, or an Error on failure.
	 */
	[[nodiscard]] static Result<void, Error> Delete(PCWCHAR path);
};
