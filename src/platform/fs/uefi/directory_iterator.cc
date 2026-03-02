/**
 * directory_iterator.cc - UEFI DirectoryIterator Implementation
 *
 * Implements directory iteration using EFI_FILE_PROTOCOL.
 */

#include "platform/fs/directory_iterator.h"
#include "platform/fs/uefi/uefi_fs_helpers.h"
#include "core/memory/memory.h"

// =============================================================================
// DirectoryIterator Class Implementation
// =============================================================================

DirectoryIterator::DirectoryIterator()
	: handle(nullptr), currentEntry{}, isFirst(true)
{}

Result<DirectoryIterator, Error> DirectoryIterator::Create(PCWCHAR path)
{
	DirectoryIterator iter;
	(VOID) iter.isFirst; // Suppress unused warning - UEFI uses Read to iterate

	EFI_FILE_PROTOCOL *Root = GetRootDirectory();
	if (Root == nullptr)
		return Result<DirectoryIterator, Error>::Err(Error::Fs_OpenFailed);

	// Empty path means root directory - use the volume root handle directly
	// rather than calling Open() with L"" which some firmware doesn't support
	if (path == nullptr || path[0] == 0)
	{
		iter.handle = (PVOID)Root;
		return Result<DirectoryIterator, Error>::Ok(static_cast<DirectoryIterator &&>(iter));
	}

	EFI_FILE_PROTOCOL *DirHandle = OpenFileFromRoot(Root, path, EFI_FILE_MODE_READ, 0);
	Root->Close(Root);

	if (DirHandle != nullptr)
	{
		iter.handle = (PVOID)DirHandle;
		return Result<DirectoryIterator, Error>::Ok(static_cast<DirectoryIterator &&>(iter));
	}
	return Result<DirectoryIterator, Error>::Err(Error::Fs_OpenFailed);
}

DirectoryIterator::DirectoryIterator(DirectoryIterator &&other) noexcept
	: handle(other.handle), currentEntry(other.currentEntry), isFirst(other.isFirst)
{
	other.handle = nullptr;
}

DirectoryIterator &DirectoryIterator::operator=(DirectoryIterator &&other) noexcept
{
	if (this != &other)
	{
		if (handle != nullptr)
		{
			EFI_FILE_PROTOCOL *fp = (EFI_FILE_PROTOCOL *)handle;
			fp->Close(fp);
		}
		handle = other.handle;
		currentEntry = other.currentEntry;
		isFirst = other.isFirst;
		other.handle = nullptr;
	}
	return *this;
}

DirectoryIterator::~DirectoryIterator()
{
	if (handle != nullptr)
	{
		EFI_FILE_PROTOCOL *fp = (EFI_FILE_PROTOCOL *)handle;
		fp->Close(fp);
		handle = nullptr;
	}
}

Result<void, Error> DirectoryIterator::Next()
{
	if (handle == nullptr)
		return Result<void, Error>::Err(Error::Fs_ReadFailed);

	EFI_FILE_PROTOCOL *fp = (EFI_FILE_PROTOCOL *)handle;

	// Allocate buffer for EFI_FILE_INFO (needs to include variable-length filename)
	// Use a fixed buffer size that should be large enough for most filenames
	UINT8 Buffer[512];
	USIZE BufferSize = sizeof(Buffer);

	EFI_STATUS Status = fp->Read(fp, &BufferSize, Buffer);

	if (EFI_ERROR_CHECK(Status))
		return Result<void, Error>::Err(Error::Uefi((UINT32)Status), Error::Fs_ReadFailed);

	// End of directory
	if (BufferSize == 0)
		return Result<void, Error>::Err(Error::Fs_ReadFailed);

	EFI_FILE_INFO *FileInfo = (EFI_FILE_INFO *)Buffer;

	// Copy filename to currentEntry
	INT32 i = 0;
	while (FileInfo->FileName[i] != 0 && i < 255)
	{
		currentEntry.Name[i] = FileInfo->FileName[i];
		i++;
	}
	currentEntry.Name[i] = 0;

	// Fill other fields
	currentEntry.Size = FileInfo->FileSize;
	currentEntry.IsDirectory = (FileInfo->Attribute & EFI_FILE_DIRECTORY) != 0;
	currentEntry.IsDrive = false;
	currentEntry.IsHidden = (FileInfo->Attribute & EFI_FILE_HIDDEN) != 0;
	currentEntry.IsSystem = (FileInfo->Attribute & EFI_FILE_SYSTEM) != 0;
	currentEntry.IsReadOnly = (FileInfo->Attribute & EFI_FILE_READ_ONLY) != 0;
	currentEntry.Type = 0;
	currentEntry.CreationTime = 0;
	currentEntry.LastModifiedTime = 0;

	return Result<void, Error>::Ok();
}

BOOL DirectoryIterator::IsValid() const
{
	return handle != nullptr;
}
