/**
 * file.cc - UEFI File Implementation
 *
 * Implements File class operations using EFI_FILE_PROTOCOL.
 */

#include "platform/fs/file.h"
#include "platform/fs/uefi/uefi_fs_helpers.h"
#include "core/memory/memory.h"

// EFI_FILE_INFO_ID {09576E92-6D3F-11D2-8E39-00A0C969723B}
static NOINLINE EFI_GUID MakeFileInfoGuid()
{
	EFI_GUID g;
	g.Data1 = 0x09576E92;
	g.Data2 = 0x6D3F;
	g.Data3 = 0x11D2;
	g.Data4[0] = 0x8E; g.Data4[1] = 0x39; g.Data4[2] = 0x00; g.Data4[3] = 0xA0;
	g.Data4[4] = 0xC9; g.Data4[5] = 0x69; g.Data4[6] = 0x72; g.Data4[7] = 0x3B;
	return g;
}

// =============================================================================
// Helper: Query file size via EFI_FILE_INFO
// =============================================================================

static NOINLINE USIZE QueryFileSize(EFI_FILE_PROTOCOL &fp)
{
	EFI_GUID FileInfoId = MakeFileInfoGuid();
	USIZE InfoSize = 0;
	fp.GetInfo(&fp, &FileInfoId, &InfoSize, nullptr);
	if (InfoSize == 0)
		return 0;

	EFI_CONTEXT *ctx = GetEfiContext();
	EFI_BOOT_SERVICES *bs = ctx->SystemTable->BootServices;

	USIZE size = 0;
	EFI_FILE_INFO *FileInfo = nullptr;
	if (!EFI_ERROR_CHECK(bs->AllocatePool(EfiLoaderData, InfoSize, (PVOID *)&FileInfo)))
	{
		if (!EFI_ERROR_CHECK(fp.GetInfo(&fp, &FileInfoId, &InfoSize, FileInfo)))
			size = FileInfo->FileSize;
		bs->FreePool(FileInfo);
	}
	return size;
}

// =============================================================================
// Helper: Truncate file to zero length via EFI_FILE_INFO
// =============================================================================

static NOINLINE VOID TruncateFile(EFI_FILE_PROTOCOL &fp)
{
	EFI_GUID FileInfoId = MakeFileInfoGuid();
	USIZE InfoSize = 0;
	fp.GetInfo(&fp, &FileInfoId, &InfoSize, nullptr);
	if (InfoSize == 0)
		return;

	EFI_CONTEXT *ctx = GetEfiContext();
	EFI_BOOT_SERVICES *bs = ctx->SystemTable->BootServices;

	EFI_FILE_INFO *FileInfo = nullptr;
	if (!EFI_ERROR_CHECK(bs->AllocatePool(EfiLoaderData, InfoSize, (PVOID *)&FileInfo)))
	{
		if (!EFI_ERROR_CHECK(fp.GetInfo(&fp, &FileInfoId, &InfoSize, FileInfo)))
		{
			FileInfo->FileSize = 0;
			fp.SetInfo(&fp, &FileInfoId, InfoSize, FileInfo);
		}
		bs->FreePool(FileInfo);
	}
}

// =============================================================================
// File Class Implementation
// =============================================================================

// --- Internal Constructor (trivial — never fails) ---
File::File(PVOID handle, USIZE size) : fileHandle(handle), fileSize(size) {}

// --- Factory & Static Operations ---
Result<File, Error> File::Open(PCWCHAR path, INT32 flags)
{
	EFI_FILE_PROTOCOL *Root = GetRootDirectory();
	if (Root == nullptr)
		return Result<File, Error>::Err(Error::Fs_OpenFailed);

	// Convert flags to EFI modes
	UINT64 mode = 0;
	UINT64 attributes = 0;

	if (flags & File::ModeRead)
		mode |= EFI_FILE_MODE_READ;
	if (flags & File::ModeWrite)
		mode |= EFI_FILE_MODE_WRITE;
	if (flags & File::ModeCreate)
		mode |= EFI_FILE_MODE_CREATE;

	// If no mode specified, default to read
	if (mode == 0)
		mode = EFI_FILE_MODE_READ;

	// Create mode requires write mode
	if (mode & EFI_FILE_MODE_CREATE)
		mode |= EFI_FILE_MODE_READ | EFI_FILE_MODE_WRITE;

	EFI_FILE_PROTOCOL *FileHandle = OpenFileFromRoot(Root, path, mode, attributes);
	Root->Close(Root);

	if (FileHandle == nullptr)
		return Result<File, Error>::Err(Error::Fs_OpenFailed);

	// Handle truncate flag
	if (flags & File::ModeTruncate)
		TruncateFile(*FileHandle);

	// Query file size before constructing the File (keeps the constructor trivial)
	USIZE size = QueryFileSize(*FileHandle);

	return Result<File, Error>::Ok(File((PVOID)FileHandle, size));
}

Result<void, Error> File::Delete(PCWCHAR path)
{
	EFI_FILE_PROTOCOL *Root = GetRootDirectory();
	if (Root == nullptr)
		return Result<void, Error>::Err(Error::Fs_DeleteFailed);

	EFI_FILE_PROTOCOL *FileHandle = OpenFileFromRoot(Root, path, EFI_FILE_MODE_READ | EFI_FILE_MODE_WRITE, 0);
	Root->Close(Root);

	if (FileHandle == nullptr)
		return Result<void, Error>::Err(Error::Fs_DeleteFailed);

	// EFI_FILE_PROTOCOL.Delete closes the handle and deletes the file
	EFI_STATUS Status = FileHandle->Delete(FileHandle);
	if (EFI_ERROR_CHECK(Status))
		return Result<void, Error>::Err(Error::Uefi((UINT32)Status), Error::Fs_DeleteFailed);
	return Result<void, Error>::Ok();
}

Result<void, Error> File::Exists(PCWCHAR path)
{
	EFI_FILE_PROTOCOL *Root = GetRootDirectory();
	if (Root == nullptr)
		return Result<void, Error>::Err(Error::Fs_OpenFailed);

	EFI_FILE_PROTOCOL *FileHandle = OpenFileFromRoot(Root, path, EFI_FILE_MODE_READ, 0);
	Root->Close(Root);

	if (FileHandle == nullptr)
		return Result<void, Error>::Err(Error::Fs_OpenFailed);

	FileHandle->Close(FileHandle);
	return Result<void, Error>::Ok();
}

BOOL File::IsValid() const
{
	return fileHandle != nullptr;
}

VOID File::Close()
{
	if (fileHandle != nullptr)
	{
		EFI_FILE_PROTOCOL *fp = (EFI_FILE_PROTOCOL *)fileHandle;
		fp->Close(fp);
		fileHandle = nullptr;
	}
	fileSize = 0;
}

Result<UINT32, Error> File::Read(Span<UINT8> buffer)
{
	if (fileHandle == nullptr || buffer.Data() == nullptr || buffer.Size() == 0)
		return Result<UINT32, Error>::Err(Error::Fs_ReadFailed);

	EFI_FILE_PROTOCOL *fp = (EFI_FILE_PROTOCOL *)fileHandle;
	USIZE readSize = buffer.Size();

	EFI_STATUS Status = fp->Read(fp, &readSize, buffer.Data());
	if (EFI_ERROR_CHECK(Status))
		return Result<UINT32, Error>::Err(Error::Uefi((UINT32)Status), Error::Fs_ReadFailed);

	return Result<UINT32, Error>::Ok((UINT32)readSize);
}

Result<UINT32, Error> File::Write(Span<const UINT8> buffer)
{
	if (fileHandle == nullptr || buffer.Data() == nullptr || buffer.Size() == 0)
		return Result<UINT32, Error>::Err(Error::Fs_WriteFailed);

	EFI_FILE_PROTOCOL *fp = (EFI_FILE_PROTOCOL *)fileHandle;
	USIZE writeSize = buffer.Size();

	EFI_STATUS Status = fp->Write(fp, &writeSize, (PVOID)buffer.Data());
	if (EFI_ERROR_CHECK(Status))
		return Result<UINT32, Error>::Err(Error::Uefi((UINT32)Status), Error::Fs_WriteFailed);

	// Update file size if we wrote past the end
	UINT64 pos = 0;
	fp->GetPosition(fp, &pos);
	if (pos > fileSize)
		fileSize = pos;

	return Result<UINT32, Error>::Ok((UINT32)writeSize);
}

Result<USIZE, Error> File::GetOffset() const
{
	if (fileHandle == nullptr)
		return Result<USIZE, Error>::Err(Error::Fs_SeekFailed);

	EFI_FILE_PROTOCOL *fp = (EFI_FILE_PROTOCOL *)fileHandle;
	UINT64 position = 0;
	EFI_STATUS status = fp->GetPosition(fp, &position);
	if ((SSIZE)status >= 0)
		return Result<USIZE, Error>::Ok((USIZE)position);
	return Result<USIZE, Error>::Err(Error::Uefi((UINT32)status), Error::Fs_SeekFailed);
}

Result<void, Error> File::SetOffset(USIZE absoluteOffset)
{
	if (fileHandle == nullptr)
		return Result<void, Error>::Err(Error::Fs_SeekFailed);

	EFI_FILE_PROTOCOL *fp = (EFI_FILE_PROTOCOL *)fileHandle;
	EFI_STATUS status = fp->SetPosition(fp, absoluteOffset);
	if ((SSIZE)status >= 0)
		return Result<void, Error>::Ok();
	return Result<void, Error>::Err(Error::Uefi((UINT32)status), Error::Fs_SeekFailed);
}

Result<void, Error> File::MoveOffset(SSIZE relativeAmount, OffsetOrigin origin)
{
	if (fileHandle == nullptr)
		return Result<void, Error>::Err(Error::Fs_SeekFailed);

	EFI_FILE_PROTOCOL *fp = (EFI_FILE_PROTOCOL *)fileHandle;
	UINT64 newPosition = 0;

	switch (origin)
	{
	case OffsetOrigin::Start:
		newPosition = (relativeAmount >= 0) ? (UINT64)relativeAmount : 0;
		break;
	case OffsetOrigin::Current:
	{
		UINT64 currentPos = 0;
		EFI_STATUS getStatus = fp->GetPosition(fp, &currentPos);
		if ((SSIZE)getStatus < 0)
			return Result<void, Error>::Err(Error::Uefi((UINT32)getStatus), Error::Fs_SeekFailed);
		if (relativeAmount >= 0)
			newPosition = currentPos + relativeAmount;
		else
			newPosition = (currentPos > (UINT64)(-relativeAmount)) ? currentPos + relativeAmount : 0;
	}
	break;
	case OffsetOrigin::End:
		if (relativeAmount >= 0)
			newPosition = fileSize + relativeAmount;
		else
			newPosition = (fileSize > (UINT64)(-relativeAmount)) ? fileSize + relativeAmount : 0;
		break;
	}

	EFI_STATUS status = fp->SetPosition(fp, newPosition);
	if ((SSIZE)status >= 0)
		return Result<void, Error>::Ok();
	return Result<void, Error>::Err(Error::Uefi((UINT32)status), Error::Fs_SeekFailed);
}

File::File(File &&other) noexcept
	: fileHandle(other.fileHandle), fileSize(other.fileSize)
{
	other.fileHandle = nullptr;
	other.fileSize = 0;
}

File &File::operator=(File &&other) noexcept
{
	if (this != &other)
	{
		Close();
		fileHandle = other.fileHandle;
		fileSize = other.fileSize;
		other.fileHandle = nullptr;
		other.fileSize = 0;
	}
	return *this;
}
