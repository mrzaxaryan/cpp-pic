/**
 * file_system.cc - UEFI FileSystem Stub Implementation
 *
 * UEFI file system support requires EFI_FILE_PROTOCOL which is complex.
 * This is a stub implementation for initial bring-up.
 */

#include "file_system.h"

// =============================================================================
// FileSystem Class - Stub Implementation
// =============================================================================

File FileSystem::Open(PCWCHAR path, INT32 flags)
{
	(VOID)path;
	(VOID)flags;
	return File(); // Returns invalid file
}

BOOL FileSystem::Delete(PCWCHAR path)
{
	(VOID)path;
	return FALSE;
}

BOOL FileSystem::Exists(PCWCHAR path)
{
	(VOID)path;
	return FALSE;
}

BOOL FileSystem::CreateDirectroy(PCWCHAR path)
{
	(VOID)path;
	return FALSE;
}

BOOL FileSystem::DeleteDirectory(PCWCHAR path)
{
	(VOID)path;
	return FALSE;
}

// =============================================================================
// File Class - Stub Implementation
// =============================================================================

File::File(PVOID handle)
	: fileHandle(handle), fileSize(0)
{
}

BOOL File::IsValid() const
{
	return FALSE;
}

VOID File::Close()
{
	fileHandle = NULL;
	fileSize = 0;
}

UINT32 File::Read(PVOID buffer, UINT32 size)
{
	(VOID)buffer;
	(VOID)size;
	return 0;
}

UINT32 File::Write(const VOID *buffer, USIZE size)
{
	(VOID)buffer;
	(VOID)size;
	return 0;
}

USIZE File::GetOffset() const
{
	return 0;
}

VOID File::SetOffset(USIZE absoluteOffset)
{
	(VOID)absoluteOffset;
}

VOID File::MoveOffset(SSIZE relativeAmount, OffsetOrigin origin)
{
	(VOID)relativeAmount;
	(VOID)origin;
}

File::File(File &&other) noexcept
	: fileHandle(other.fileHandle), fileSize(other.fileSize)
{
	other.fileHandle = NULL;
	other.fileSize = 0;
}

File &File::operator=(File &&other) noexcept
{
	if (this != &other)
	{
		Close();
		fileHandle = other.fileHandle;
		fileSize = other.fileSize;
		other.fileHandle = NULL;
		other.fileSize = 0;
	}
	return *this;
}

// =============================================================================
// DirectoryIterator Class - Stub Implementation
// =============================================================================

DirectoryIterator::DirectoryIterator(PCWCHAR path)
	: handle(NULL), currentEntry{}, first(TRUE)
{
	(VOID)path;
	(VOID)first; // Suppress unused warning for UEFI stub
}

DirectoryIterator::~DirectoryIterator()
{
	handle = NULL;
}

BOOL DirectoryIterator::Next()
{
	return FALSE;
}

BOOL DirectoryIterator::IsValid() const
{
	return FALSE;
}
