/**
 * file_system.cc - UEFI FileSystem Implementation
 *
 * Implements file system operations using EFI_SIMPLE_FILE_SYSTEM_PROTOCOL
 * and EFI_FILE_PROTOCOL.
 */

#include "file_system.h"
#include "efi_context.h"
#include "efi_file_protocol.h"
#include "memory.h"
#include "path.h"

// =============================================================================
// Helper: Get Root Directory Handle
// =============================================================================

static EFI_FILE_PROTOCOL *GetRootDirectory()
{
	EFI_CONTEXT *ctx = GetEfiContext();
	if (ctx == NULL || ctx->SystemTable == NULL)
		return NULL;

	EFI_BOOT_SERVICES *bs = ctx->SystemTable->BootServices;

	// EFI_SIMPLE_FILE_SYSTEM_PROTOCOL_GUID {964E5B22-6459-11D2-8E39-00A0C969723B}
	EFI_GUID FsGuid;
	FsGuid.Data1 = 0x964E5B22;
	FsGuid.Data2 = 0x6459;
	FsGuid.Data3 = 0x11D2;
	FsGuid.Data4[0] = 0x8E;
	FsGuid.Data4[1] = 0x39;
	FsGuid.Data4[2] = 0x00;
	FsGuid.Data4[3] = 0xA0;
	FsGuid.Data4[4] = 0xC9;
	FsGuid.Data4[5] = 0x69;
	FsGuid.Data4[6] = 0x72;
	FsGuid.Data4[7] = 0x3B;

	USIZE HandleCount = 0;
	EFI_HANDLE *HandleBuffer = NULL;

	if (EFI_ERROR_CHECK(bs->LocateHandleBuffer(ByProtocol, &FsGuid, NULL, &HandleCount, &HandleBuffer)) || HandleCount == 0)
		return NULL;

	EFI_SIMPLE_FILE_SYSTEM_PROTOCOL *FileSystem = NULL;
	EFI_FILE_PROTOCOL *Root = NULL;

	// Try each handle until we find a working filesystem
	for (USIZE i = 0; i < HandleCount; i++)
	{
		if (EFI_ERROR_CHECK(bs->OpenProtocol(HandleBuffer[i], &FsGuid, (PVOID *)&FileSystem, ctx->ImageHandle, NULL, EFI_OPEN_PROTOCOL_GET_PROTOCOL)))
			continue;

		if (FileSystem != NULL && !EFI_ERROR_CHECK(FileSystem->OpenVolume(FileSystem, &Root)))
		{
			bs->FreePool(HandleBuffer);
			return Root;
		}
	}

	bs->FreePool(HandleBuffer);
	return NULL;
}

// =============================================================================
// Helper: Open File by Path from Root
// =============================================================================

static EFI_FILE_PROTOCOL *OpenFileFromRoot(EFI_FILE_PROTOCOL *Root, PCWCHAR path, UINT64 mode, UINT64 attributes)
{
	if (Root == NULL || path == NULL)
		return NULL;

	// Normalize path separators (convert '/' to '\' for UEFI)
	PWCHAR normalizedPath = Path::NormalizePath(path);
	if (normalizedPath == NULL)
		return NULL;

	EFI_FILE_PROTOCOL *FileHandle = NULL;
	EFI_STATUS Status = Root->Open(Root, &FileHandle, (CHAR16 *)normalizedPath, mode, attributes);

	delete[] normalizedPath;

	if (EFI_ERROR_CHECK(Status))
		return NULL;

	return FileHandle;
}

// =============================================================================
// FileSystem Class Implementation
// =============================================================================

File FileSystem::Open(PCWCHAR path, INT32 flags)
{
	EFI_FILE_PROTOCOL *Root = GetRootDirectory();
	if (Root == NULL)
		return File();

	// Convert flags to EFI modes
	UINT64 mode = 0;
	UINT64 attributes = 0;

	if (flags & FS_READ)
		mode |= EFI_FILE_MODE_READ;
	if (flags & FS_WRITE)
		mode |= EFI_FILE_MODE_WRITE;
	if (flags & FS_CREATE)
		mode |= EFI_FILE_MODE_CREATE;

	// If no mode specified, default to read
	if (mode == 0)
		mode = EFI_FILE_MODE_READ;

	// Create mode requires write mode
	if (mode & EFI_FILE_MODE_CREATE)
		mode |= EFI_FILE_MODE_READ | EFI_FILE_MODE_WRITE;

	EFI_FILE_PROTOCOL *FileHandle = OpenFileFromRoot(Root, path, mode, attributes);
	Root->Close(Root);

	if (FileHandle == NULL)
		return File();

	// Handle truncate flag
	if ((flags & FS_TRUNCATE) && FileHandle != NULL)
	{
		// Set file size to 0 using SetInfo
		// EFI_FILE_INFO_ID {09576E92-6D3F-11D2-8E39-00A0C969723B}
		EFI_GUID FileInfoId;
		FileInfoId.Data1 = 0x09576E92;
		FileInfoId.Data2 = 0x6D3F;
		FileInfoId.Data3 = 0x11D2;
		FileInfoId.Data4[0] = 0x8E;
		FileInfoId.Data4[1] = 0x39;
		FileInfoId.Data4[2] = 0x00;
		FileInfoId.Data4[3] = 0xA0;
		FileInfoId.Data4[4] = 0xC9;
		FileInfoId.Data4[5] = 0x69;
		FileInfoId.Data4[6] = 0x72;
		FileInfoId.Data4[7] = 0x3B;

		// Get current file info size
		USIZE InfoSize = 0;
		FileHandle->GetInfo(FileHandle, &FileInfoId, &InfoSize, NULL);

		if (InfoSize > 0)
		{
			EFI_CONTEXT *ctx = GetEfiContext();
			EFI_BOOT_SERVICES *bs = ctx->SystemTable->BootServices;

			EFI_FILE_INFO *FileInfo = NULL;
			if (!EFI_ERROR_CHECK(bs->AllocatePool(EfiLoaderData, InfoSize, (PVOID *)&FileInfo)))
			{
				if (!EFI_ERROR_CHECK(FileHandle->GetInfo(FileHandle, &FileInfoId, &InfoSize, FileInfo)))
				{
					FileInfo->FileSize = 0;
					FileHandle->SetInfo(FileHandle, &FileInfoId, InfoSize, FileInfo);
				}
				bs->FreePool(FileInfo);
			}
		}
	}

	return File((PVOID)FileHandle);
}

BOOL FileSystem::Delete(PCWCHAR path)
{
	EFI_FILE_PROTOCOL *Root = GetRootDirectory();
	if (Root == NULL)
		return FALSE;

	EFI_FILE_PROTOCOL *FileHandle = OpenFileFromRoot(Root, path, EFI_FILE_MODE_READ | EFI_FILE_MODE_WRITE, 0);
	Root->Close(Root);

	if (FileHandle == NULL)
		return FALSE;

	// EFI_FILE_PROTOCOL.Delete closes the handle and deletes the file
	EFI_STATUS Status = FileHandle->Delete(FileHandle);
	return !EFI_ERROR_CHECK(Status);
}

BOOL FileSystem::Exists(PCWCHAR path)
{
	EFI_FILE_PROTOCOL *Root = GetRootDirectory();
	if (Root == NULL)
		return FALSE;

	EFI_FILE_PROTOCOL *FileHandle = OpenFileFromRoot(Root, path, EFI_FILE_MODE_READ, 0);
	Root->Close(Root);

	if (FileHandle == NULL)
		return FALSE;

	FileHandle->Close(FileHandle);
	return TRUE;
}

BOOL FileSystem::CreateDirectory(PCWCHAR path)
{
	EFI_FILE_PROTOCOL *Root = GetRootDirectory();
	if (Root == NULL)
		return FALSE;

	// Normalize path separators (convert '/' to '\' for UEFI)
	PWCHAR normalizedPath = Path::NormalizePath(path);
	if (normalizedPath == NULL)
	{
		Root->Close(Root);
		return FALSE;
	}

	EFI_FILE_PROTOCOL *DirHandle = NULL;
	EFI_STATUS Status = Root->Open(Root, &DirHandle, (CHAR16 *)normalizedPath,
		EFI_FILE_MODE_READ | EFI_FILE_MODE_WRITE | EFI_FILE_MODE_CREATE,
		EFI_FILE_DIRECTORY);

	delete[] normalizedPath;
	Root->Close(Root);

	if (EFI_ERROR_CHECK(Status) || DirHandle == NULL)
		return FALSE;

	DirHandle->Close(DirHandle);
	return TRUE;
}

BOOL FileSystem::DeleteDirectory(PCWCHAR path)
{
	EFI_FILE_PROTOCOL *Root = GetRootDirectory();
	if (Root == NULL)
		return FALSE;

	EFI_FILE_PROTOCOL *DirHandle = OpenFileFromRoot(Root, path, EFI_FILE_MODE_READ | EFI_FILE_MODE_WRITE, 0);
	Root->Close(Root);

	if (DirHandle == NULL)
		return FALSE;

	// EFI_FILE_PROTOCOL.Delete works for both files and directories
	EFI_STATUS Status = DirHandle->Delete(DirHandle);
	return !EFI_ERROR_CHECK(Status);
}

// =============================================================================
// File Class Implementation
// =============================================================================

File::File(PVOID handle)
	: fileHandle(handle), fileSize(0)
{
	if (handle != NULL)
	{
		// Get file size using GetInfo
		EFI_FILE_PROTOCOL *fp = (EFI_FILE_PROTOCOL *)handle;

		// EFI_FILE_INFO_ID {09576E92-6D3F-11D2-8E39-00A0C969723B}
		EFI_GUID FileInfoId;
		FileInfoId.Data1 = 0x09576E92;
		FileInfoId.Data2 = 0x6D3F;
		FileInfoId.Data3 = 0x11D2;
		FileInfoId.Data4[0] = 0x8E;
		FileInfoId.Data4[1] = 0x39;
		FileInfoId.Data4[2] = 0x00;
		FileInfoId.Data4[3] = 0xA0;
		FileInfoId.Data4[4] = 0xC9;
		FileInfoId.Data4[5] = 0x69;
		FileInfoId.Data4[6] = 0x72;
		FileInfoId.Data4[7] = 0x3B;

		USIZE InfoSize = 0;
		fp->GetInfo(fp, &FileInfoId, &InfoSize, NULL);

		if (InfoSize > 0)
		{
			EFI_CONTEXT *ctx = GetEfiContext();
			EFI_BOOT_SERVICES *bs = ctx->SystemTable->BootServices;

			EFI_FILE_INFO *FileInfo = NULL;
			if (!EFI_ERROR_CHECK(bs->AllocatePool(EfiLoaderData, InfoSize, (PVOID *)&FileInfo)))
			{
				if (!EFI_ERROR_CHECK(fp->GetInfo(fp, &FileInfoId, &InfoSize, FileInfo)))
				{
					fileSize = FileInfo->FileSize;
				}
				bs->FreePool(FileInfo);
			}
		}
	}
}

BOOL File::IsValid() const
{
	return fileHandle != NULL;
}

VOID File::Close()
{
	if (fileHandle != NULL)
	{
		EFI_FILE_PROTOCOL *fp = (EFI_FILE_PROTOCOL *)fileHandle;
		fp->Close(fp);
		fileHandle = NULL;
	}
	fileSize = 0;
}

UINT32 File::Read(PVOID buffer, UINT32 size)
{
	if (fileHandle == NULL || buffer == NULL || size == 0)
		return 0;

	EFI_FILE_PROTOCOL *fp = (EFI_FILE_PROTOCOL *)fileHandle;
	USIZE readSize = size;

	EFI_STATUS Status = fp->Read(fp, &readSize, buffer);
	if (EFI_ERROR_CHECK(Status))
		return 0;

	return (UINT32)readSize;
}

UINT32 File::Write(const VOID *buffer, USIZE size)
{
	if (fileHandle == NULL || buffer == NULL || size == 0)
		return 0;

	EFI_FILE_PROTOCOL *fp = (EFI_FILE_PROTOCOL *)fileHandle;
	USIZE writeSize = size;

	EFI_STATUS Status = fp->Write(fp, &writeSize, (PVOID)buffer);
	if (EFI_ERROR_CHECK(Status))
		return 0;

	// Update file size if we wrote past the end
	UINT64 pos = 0;
	fp->GetPosition(fp, &pos);
	if (pos > fileSize)
		fileSize = pos;

	return (UINT32)writeSize;
}

USIZE File::GetOffset() const
{
	if (fileHandle == NULL)
		return 0;

	EFI_FILE_PROTOCOL *fp = (EFI_FILE_PROTOCOL *)fileHandle;
	UINT64 position = 0;
	fp->GetPosition(fp, &position);
	return (USIZE)position;
}

VOID File::SetOffset(USIZE absoluteOffset)
{
	if (fileHandle == NULL)
		return;

	EFI_FILE_PROTOCOL *fp = (EFI_FILE_PROTOCOL *)fileHandle;
	fp->SetPosition(fp, absoluteOffset);
}

VOID File::MoveOffset(SSIZE relativeAmount, OffsetOrigin origin)
{
	if (fileHandle == NULL)
		return;

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
			fp->GetPosition(fp, &currentPos);
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

	fp->SetPosition(fp, newPosition);
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
// DirectoryIterator Class Implementation
// =============================================================================

DirectoryIterator::DirectoryIterator(PCWCHAR path)
	: handle(NULL), currentEntry{}, first(TRUE)
{
	(VOID)first; // Suppress unused warning - UEFI uses Read to iterate

	EFI_FILE_PROTOCOL *Root = GetRootDirectory();
	if (Root == NULL)
		return;

	EFI_FILE_PROTOCOL *DirHandle = OpenFileFromRoot(Root, path, EFI_FILE_MODE_READ, 0);
	Root->Close(Root);

	if (DirHandle != NULL)
	{
		handle = (PVOID)DirHandle;
	}
}

DirectoryIterator::~DirectoryIterator()
{
	if (handle != NULL)
	{
		EFI_FILE_PROTOCOL *fp = (EFI_FILE_PROTOCOL *)handle;
		fp->Close(fp);
		handle = NULL;
	}
}

BOOL DirectoryIterator::Next()
{
	if (handle == NULL)
		return FALSE;

	EFI_FILE_PROTOCOL *fp = (EFI_FILE_PROTOCOL *)handle;

	// Allocate buffer for EFI_FILE_INFO (needs to include variable-length filename)
	// Use a fixed buffer size that should be large enough for most filenames
	UINT8 Buffer[512];
	USIZE BufferSize = sizeof(Buffer);

	EFI_STATUS Status = fp->Read(fp, &BufferSize, Buffer);

	// End of directory or error
	if (EFI_ERROR_CHECK(Status) || BufferSize == 0)
		return FALSE;

	EFI_FILE_INFO *FileInfo = (EFI_FILE_INFO *)Buffer;

	// Copy filename to currentEntry
	INT32 i = 0;
	while (FileInfo->FileName[i] != 0 && i < 255)
	{
		currentEntry.name[i] = FileInfo->FileName[i];
		i++;
	}
	currentEntry.name[i] = 0;

	// Fill other fields
	currentEntry.size = FileInfo->FileSize;
	currentEntry.isDirectory = (FileInfo->Attribute & EFI_FILE_DIRECTORY) != 0;
	currentEntry.isDrive = FALSE;
	currentEntry.isHidden = (FileInfo->Attribute & EFI_FILE_HIDDEN) != 0;
	currentEntry.isSystem = (FileInfo->Attribute & EFI_FILE_SYSTEM) != 0;
	currentEntry.isReadOnly = (FileInfo->Attribute & EFI_FILE_READ_ONLY) != 0;
	currentEntry.type = 0;
	currentEntry.creationTime = 0;

	return TRUE;
}

BOOL DirectoryIterator::IsValid() const
{
	return handle != NULL;
}
