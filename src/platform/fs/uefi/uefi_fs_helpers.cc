#include "platform/fs/uefi/uefi_fs_helpers.h"
#include "platform/fs/path.h"
#include "core/memory/memory.h"

// EFI_SIMPLE_FILE_SYSTEM_PROTOCOL_GUID {964E5B22-6459-11D2-8E39-00A0C969723B}
NOINLINE EFI_GUID MakeFsProtocolGuid()
{
	EFI_GUID g;
	g.Data1 = 0x964E5B22;
	g.Data2 = 0x6459;
	g.Data3 = 0x11D2;
	g.Data4[0] = 0x8E; g.Data4[1] = 0x39; g.Data4[2] = 0x00; g.Data4[3] = 0xA0;
	g.Data4[4] = 0xC9; g.Data4[5] = 0x69; g.Data4[6] = 0x72; g.Data4[7] = 0x3B;
	return g;
}

EFI_FILE_PROTOCOL *GetRootDirectory()
{
	EFI_CONTEXT *ctx = GetEfiContext();
	if (ctx == nullptr || ctx->SystemTable == nullptr)
		return nullptr;

	EFI_BOOT_SERVICES *bs = ctx->SystemTable->BootServices;

	EFI_GUID FsGuid = MakeFsProtocolGuid();

	USIZE HandleCount = 0;
	EFI_HANDLE *HandleBuffer = nullptr;

	if (EFI_ERROR_CHECK(bs->LocateHandleBuffer(ByProtocol, &FsGuid, nullptr, &HandleCount, &HandleBuffer)) || HandleCount == 0)
		return nullptr;

	EFI_SIMPLE_FILE_SYSTEM_PROTOCOL *FileSystem = nullptr;
	EFI_FILE_PROTOCOL *Root = nullptr;

	// Try each handle until we find a working filesystem
	for (USIZE i = 0; i < HandleCount; i++)
	{
		if (EFI_ERROR_CHECK(bs->OpenProtocol(HandleBuffer[i], &FsGuid, (PVOID *)&FileSystem, ctx->ImageHandle, nullptr, EFI_OPEN_PROTOCOL_GET_PROTOCOL)))
			continue;

		if (FileSystem != nullptr && !EFI_ERROR_CHECK(FileSystem->OpenVolume(FileSystem, &Root)))
		{
			bs->FreePool(HandleBuffer);
			return Root;
		}
	}

	bs->FreePool(HandleBuffer);
	return nullptr;
}

EFI_FILE_PROTOCOL *OpenFileFromRoot(EFI_FILE_PROTOCOL *Root, PCWCHAR path, UINT64 mode, UINT64 attributes)
{
	if (Root == nullptr || path == nullptr)
		return nullptr;

	// Normalize path separators (convert '/' to '\' for UEFI)
	WCHAR normalizedBuf[512];
	if (!Path::NormalizePath(path, Span<WCHAR>(normalizedBuf)))
		return nullptr;

	EFI_FILE_PROTOCOL *FileHandle = nullptr;
	EFI_STATUS Status = Root->Open(Root, &FileHandle, (CHAR16 *)normalizedBuf, mode, attributes);

	if (EFI_ERROR_CHECK(Status))
		return nullptr;

	return FileHandle;
}
