/**
 * directory.cc - UEFI Directory Implementation
 *
 * Implements Directory static methods using EFI_FILE_PROTOCOL.
 */

#include "platform/fs/directory.h"
#include "platform/fs/uefi/uefi_fs_helpers.h"
#include "platform/fs/path.h"
#include "core/memory/memory.h"

// =============================================================================
// Directory Implementation
// =============================================================================

Result<void, Error> Directory::Create(PCWCHAR path)
{
	EFI_FILE_PROTOCOL *Root = GetRootDirectory();
	if (Root == nullptr)
		return Result<void, Error>::Err(Error::Fs_CreateDirFailed);

	// Normalize path separators (convert '/' to '\' for UEFI)
	WCHAR normalizedBuf[512];
	if (!Path::NormalizePath(path, Span<WCHAR>(normalizedBuf)))
	{
		Root->Close(Root);
		return Result<void, Error>::Err(Error::Fs_PathResolveFailed, Error::Fs_CreateDirFailed);
	}

	EFI_FILE_PROTOCOL *DirHandle = nullptr;
	EFI_STATUS Status = Root->Open(Root, &DirHandle, (CHAR16 *)normalizedBuf,
								   EFI_FILE_MODE_READ | EFI_FILE_MODE_WRITE | EFI_FILE_MODE_CREATE,
								   EFI_FILE_DIRECTORY);
	Root->Close(Root);

	if (EFI_ERROR_CHECK(Status) || DirHandle == nullptr)
		return Result<void, Error>::Err(Error::Uefi((UINT32)Status), Error::Fs_CreateDirFailed);

	DirHandle->Close(DirHandle);
	return Result<void, Error>::Ok();
}

Result<void, Error> Directory::Delete(PCWCHAR path)
{
	EFI_FILE_PROTOCOL *Root = GetRootDirectory();
	if (Root == nullptr)
		return Result<void, Error>::Err(Error::Fs_DeleteDirFailed);

	EFI_FILE_PROTOCOL *DirHandle = OpenFileFromRoot(Root, path, EFI_FILE_MODE_READ | EFI_FILE_MODE_WRITE, 0);
	Root->Close(Root);

	if (DirHandle == nullptr)
		return Result<void, Error>::Err(Error::Fs_DeleteDirFailed);

	// EFI_FILE_PROTOCOL.Delete works for both files and directories
	EFI_STATUS Status = DirHandle->Delete(DirHandle);
	if (EFI_ERROR_CHECK(Status))
		return Result<void, Error>::Err(Error::Uefi((UINT32)Status), Error::Fs_DeleteDirFailed);
	return Result<void, Error>::Ok();
}
