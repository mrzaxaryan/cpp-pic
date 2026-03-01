#pragma once

#include "core/types/primitives.h"
#include "core/types/span.h"
#include "core/types/error.h"
#include "core/types/result.h"
#include "platform/io/file_system/offset_origin.h"

// Class to represent a file
class File
{
private:
	PVOID fileHandle; // File handle
	USIZE fileSize;   // File size

	// Private constructor for FileSystem's use (trivial — never fails)
	File(PVOID handle, USIZE size);

public:
	// Default constructor and destructor
	File() : fileHandle(InvalidFileHandle()), fileSize(0) {}

	// Platform-neutral invalid handle sentinel.
	// Windows: nullptr (INVALID_HANDLE_VALUE is -1, but nullptr is the "never opened" state).
	// POSIX/UEFI: (PVOID)(SSIZE)-1, because fd 0 is a valid descriptor (stdin).
	// Note: FORCE_INLINE function instead of constexpr because integer-to-pointer
	// casts are not allowed in constant expressions.
	static FORCE_INLINE PVOID InvalidFileHandle()
	{
#if defined(PLATFORM_LINUX) || defined(PLATFORM_MACOS)
		return (PVOID)(SSIZE)-1;
#else
		return nullptr;
#endif
	}
	~File() { Close(); }

	// Disable copying to prevent double-close bugs
	File(const File &) = delete;
	File &operator=(const File &) = delete;

	// Enable moving (transferring ownership)
	File(File &&other) noexcept;
	File &operator=(File &&other) noexcept;

	// Stack-only
	VOID *operator new(USIZE) = delete;
	VOID operator delete(VOID *) = delete;
	VOID *operator new(USIZE, PVOID ptr) noexcept { return ptr; } // Result needs this

	// Check if the file handle is valid
	BOOL IsValid() const;
	// Close the file handle
	VOID Close();

	// Read and write methods
	[[nodiscard]] Result<UINT32, Error> Read(Span<UINT8> buffer);
	[[nodiscard]] Result<UINT32, Error> Write(Span<const UINT8> buffer);

	// Get the size of the file
	constexpr USIZE GetSize() const { return fileSize; }
	// Get, set and move the file offset
	USIZE GetOffset() const;
	VOID SetOffset(USIZE absoluteOffset);
	VOID MoveOffset(SSIZE relativeAmount, OffsetOrigin origin = OffsetOrigin::Current);

	// Friend class for FileSystem to access private constructor
	friend class FileSystem;
};
