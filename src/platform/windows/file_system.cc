#include "file_system.h"
#include "logger.h"
#include "primitives.h"
#include "string.h"
#include "windows_types.h"
#include "ntdll.h"

// --- Internal Constructor ---
// Translates a Windows HANDLE into our File object
File::File(PVOID handle) : fileHandle(handle), fileSize(0)
{
    if (IsValid())
    {
        FILE_STANDARD_INFORMATION fileStandardInfo;
        IO_STATUS_BLOCK ioStatusBlock;
        Memory::Zero(&fileStandardInfo, sizeof(FILE_STANDARD_INFORMATION));
        Memory::Zero(&ioStatusBlock, sizeof(IO_STATUS_BLOCK));

        NTSTATUS status = NTDLL::ZwQueryInformationFile(fileHandle, &ioStatusBlock, &fileStandardInfo, sizeof(fileStandardInfo), FileStandardInformation);

        if (NT_SUCCESS(status))
        {
            fileSize = fileStandardInfo.EndOfFile.QuadPart;
        }
    }
}

// --- Move Semantics ---
File::File(File &&other) noexcept : fileHandle(nullptr), fileSize(0)
{
    fileHandle = other.fileHandle;
    fileSize = other.fileSize;
    other.fileHandle = nullptr;
    other.fileSize = 0;
}

// Operator move assignment
File &File::operator=(File &&other) noexcept
{
    if (this != &other)
    {
        Close(); // Close existing handle before taking new one
        fileHandle = other.fileHandle;
        fileSize = other.fileSize;
        other.fileHandle = nullptr;
        other.fileSize = 0;
    }
    return *this;
}

// --- Logic ---
BOOL File::IsValid() const
{
    // Windows returns INVALID_HANDLE_VALUE (-1) on many errors,
    // but some APIs return nullptr. We check for both.
    return fileHandle != nullptr && fileHandle != INVALID_HANDLE_VALUE;
}

// Close the file handle
void File::Close()
{
    if (IsValid())
    {
        (void)NTDLL::ZwClose((PVOID)fileHandle);
        fileHandle = nullptr;
        fileSize = 0;
    }
}

// Read data from the file into the buffer
Result<UINT32, Error> File::Read(PVOID buffer, UINT32 size)
{
    if (!IsValid())
        return Result<UINT32, Error>::Err(Error::Fs_ReadFailed);

    IO_STATUS_BLOCK ioStatusBlock;
    Memory::Zero(&ioStatusBlock, sizeof(IO_STATUS_BLOCK));

    NTSTATUS status = NTDLL::ZwReadFile((PVOID)fileHandle, nullptr, nullptr, nullptr, &ioStatusBlock, buffer, (UINT32)size, nullptr, nullptr);

    if (NT_SUCCESS(status))
    {
        return Result<UINT32, Error>::Ok((UINT32)ioStatusBlock.Information);
    }
    return Result<UINT32, Error>::Err(Error::Windows((UINT32)status), Error::Fs_ReadFailed);
}

// Write data from the buffer to the file
Result<UINT32, Error> File::Write(PCVOID buffer, USIZE size)
{
    if (!IsValid())
        return Result<UINT32, Error>::Err(Error::Fs_WriteFailed);

    IO_STATUS_BLOCK ioStatusBlock;
    Memory::Zero(&ioStatusBlock, sizeof(IO_STATUS_BLOCK));

    NTSTATUS status = NTDLL::ZwWriteFile((PVOID)fileHandle, nullptr, nullptr, nullptr, &ioStatusBlock, (PVOID)buffer, (UINT32)size, nullptr, nullptr);

    if (NT_SUCCESS(status))
    {
        return Result<UINT32, Error>::Ok((UINT32)ioStatusBlock.Information);
    }
    return Result<UINT32, Error>::Err(Error::Windows((UINT32)status), Error::Fs_WriteFailed);
}

// Get the current file offset
USIZE File::GetOffset() const
{
    if (!IsValid())
        return 0;

    FILE_POSITION_INFORMATION posFile;
    IO_STATUS_BLOCK ioStatusBlock;
    Memory::Zero(&posFile, sizeof(posFile));
    Memory::Zero(&ioStatusBlock, sizeof(ioStatusBlock));

    NTSTATUS status = NTDLL::ZwQueryInformationFile((PVOID)fileHandle, &ioStatusBlock, &posFile, sizeof(posFile), FilePositionInformation);

    if (NT_SUCCESS(status))
    {
        return (USIZE)posFile.CurrentByteOffset.QuadPart;
    }
    return 0;
}

// Set the file offset to an absolute position
void File::SetOffset(USIZE absoluteOffset)
{
    if (!IsValid())
        return;

    FILE_POSITION_INFORMATION posInfo;
    IO_STATUS_BLOCK ioStatusBlock;
    Memory::Zero(&posInfo, sizeof(FILE_POSITION_INFORMATION));
    Memory::Zero(&ioStatusBlock, sizeof(IO_STATUS_BLOCK));
    posInfo.CurrentByteOffset.QuadPart = (INT64)absoluteOffset;
    // Set the file pointer to the specified absolute offset using ZwSetInformationFile
    (void)NTDLL::ZwSetInformationFile((PVOID)fileHandle, &ioStatusBlock, &posInfo, sizeof(posInfo), FilePositionInformation);
}

// Move the file offset by a relative amount based on the specified origin
void File::MoveOffset(SSIZE relativeAmount, OffsetOrigin origin)
{
    if (!IsValid())
        return;

    IO_STATUS_BLOCK ioStatusBlock;
    FILE_POSITION_INFORMATION posInfo;
    FILE_STANDARD_INFORMATION fileStandardInfo;
    Memory::Zero(&ioStatusBlock, sizeof(IO_STATUS_BLOCK));
    Memory::Zero(&posInfo, sizeof(FILE_POSITION_INFORMATION));
    Memory::Zero(&fileStandardInfo, sizeof(FILE_STANDARD_INFORMATION));
    INT64 distance = 0;

    NTSTATUS status = NTDLL::ZwQueryInformationFile((PVOID)fileHandle, &ioStatusBlock, &posInfo, sizeof(posInfo), FilePositionInformation);
    if (!NT_SUCCESS(status))
        return;

    switch (origin)
    {
    case OffsetOrigin::Start:
        distance = relativeAmount;
        break;
    case OffsetOrigin::Current:
        distance = posInfo.CurrentByteOffset.QuadPart + relativeAmount;
        break;
    case OffsetOrigin::End:
        status = NTDLL::ZwQueryInformationFile(fileHandle, &ioStatusBlock, &fileStandardInfo, sizeof(fileStandardInfo), FileStandardInformation);
        if (!NT_SUCCESS(status))
            return;
        distance = fileStandardInfo.EndOfFile.QuadPart + relativeAmount;
        break;
    default:
        LOG_ERROR("Invalid OffsetOrigin specified in MoveOffset");
        return;
    }
    posInfo.CurrentByteOffset.QuadPart = distance;
    (void)NTDLL::ZwSetInformationFile((PVOID)fileHandle, &ioStatusBlock, &posInfo, sizeof(posInfo), FilePositionInformation);
}

// --- FileSystem Implementation ---
File FileSystem::Open(PCWCHAR path, INT32 flags)
{
    UINT32 dwDesiredAccess = 0;
    UINT32 dwShareMode = FILE_SHARE_READ;
    UINT32 dwCreationDisposition = FILE_OPEN;
    UINT32 ntFlags = 0;
    UINT32 fileAttributes = FILE_ATTRIBUTE_NORMAL;

    // 1. Map Access Flags
    if (flags & FS_READ)
        dwDesiredAccess |= GENERIC_READ;
    if (flags & FS_WRITE)
        dwDesiredAccess |= GENERIC_WRITE;
    if (flags & FS_APPEND)
        dwDesiredAccess |= FILE_APPEND_DATA;

    // 2. Map Creation/Truncation Flags
    if (flags & FS_CREATE)
    {
        if (flags & FS_TRUNCATE)
            dwCreationDisposition = FILE_OVERWRITE_IF;
        else
            dwCreationDisposition = FILE_OPEN_IF;
    }
    else if (flags & FS_TRUNCATE)
    {
        dwCreationDisposition = FILE_OVERWRITE;
    }

    // Synchronous I/O — PIR never uses overlapped file handles
    ntFlags |= FILE_SYNCHRONOUS_IO_NONALERT | FILE_NON_DIRECTORY_FILE;

    // Always allow waiting and querying attributes
    dwDesiredAccess |= SYNCHRONIZE | FILE_READ_ATTRIBUTES;

    // Convert DOS path to NT path
    UNICODE_STRING ntPathU;
    if (!NTDLL::RtlDosPathNameToNtPathName_U(path, &ntPathU, nullptr, nullptr))
        return File();

    OBJECT_ATTRIBUTES objAttr;
    InitializeObjectAttributes(&objAttr, &ntPathU, 0, nullptr, nullptr);

    IO_STATUS_BLOCK ioStatusBlock;
    PVOID hFile = nullptr;

    auto createResult = NTDLL::ZwCreateFile(
        &hFile,
        dwDesiredAccess,
        &objAttr,
        &ioStatusBlock,
        nullptr,
        fileAttributes,
        dwShareMode,
        dwCreationDisposition,
        ntFlags,
        nullptr,
        0);

    NTDLL::RtlFreeUnicodeString(&ntPathU);

    if (!createResult || hFile == INVALID_HANDLE_VALUE)
        return File();

    return File((PVOID)hFile);
}

// Delete a file at the specified path
Result<void, Error> FileSystem::Delete(PCWCHAR path)
{
    UNICODE_STRING ntName;
    OBJECT_ATTRIBUTES attr;
    PVOID hFile = nullptr;
    IO_STATUS_BLOCK io;

    if (!NTDLL::RtlDosPathNameToNtPathName_U(path, &ntName, nullptr, nullptr))
        return Result<void, Error>::Err(Error::Fs_PathResolveFailed, Error::Fs_DeleteFailed);

    InitializeObjectAttributes(&attr, &ntName, OBJ_CASE_INSENSITIVE, nullptr, nullptr);

    auto createResult = NTDLL::ZwCreateFile(&hFile, SYNCHRONIZE | DELETE, &attr, &io, nullptr, 0,
                                            FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                                            FILE_OPEN, FILE_DELETE_ON_CLOSE | FILE_NON_DIRECTORY_FILE, nullptr, 0);

    if (!createResult)
    {
        NTDLL::RtlFreeUnicodeString(&ntName);
        return Result<void, Error>::Err(createResult, Error::Fs_DeleteFailed);
    }

    (void)NTDLL::ZwClose(hFile);
    NTDLL::RtlFreeUnicodeString(&ntName);
    return Result<void, Error>::Ok();
}

// Check if a file exists at the specified path
BOOL FileSystem::Exists(PCWCHAR path)
{
    OBJECT_ATTRIBUTES objAttr;
    UNICODE_STRING uniName;
    FILE_BASIC_INFORMATION fileBasicInfo;

    if (!NTDLL::RtlDosPathNameToNtPathName_U(path, &uniName, nullptr, nullptr))
        return false;

    InitializeObjectAttributes(&objAttr, &uniName, 0, nullptr, nullptr);
    NTSTATUS status = NTDLL::ZwQueryAttributesFile(&objAttr, &fileBasicInfo);

    NTDLL::RtlFreeUnicodeString(&uniName);

    if (!NT_SUCCESS(status))
        return false;

    UINT32 attr = fileBasicInfo.FileAttributes;
    // If attributes are not 0xFFFFFFFF, the file exists
    return (attr != 0xFFFFFFFF);
}

// --- FileSystem Directory Management ---
Result<void, Error> FileSystem::CreateDirectory(PCWCHAR path)
{
    // Returns non-zero on success
    PVOID hDir;
    UNICODE_STRING uniName;
    OBJECT_ATTRIBUTES objAttr;
    IO_STATUS_BLOCK ioStatusBlock;

    if (!NTDLL::RtlDosPathNameToNtPathName_U(path, &uniName, nullptr, nullptr))
        return Result<void, Error>::Err(Error::Fs_PathResolveFailed, Error::Fs_CreateDirFailed);

    InitializeObjectAttributes(&objAttr, &uniName, 0, nullptr, nullptr);

    auto createResult = NTDLL::ZwCreateFile(
        &hDir,
        FILE_LIST_DIRECTORY | SYNCHRONIZE,
        &objAttr,
        &ioStatusBlock,
        nullptr,
        FILE_ATTRIBUTE_DIRECTORY,
        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
        FILE_OPEN_IF,
        FILE_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT,
        nullptr,
        0);

    NTDLL::RtlFreeUnicodeString(&uniName);

    if (createResult)
    {
        (void)NTDLL::ZwClose(hDir);
        return Result<void, Error>::Ok();
    }
    LOG_ERROR("CreateDirectory failed: errors=%e path=%ls", createResult.Error(), path);
    return Result<void, Error>::Err(createResult, Error::Fs_CreateDirFailed);
}

// Delete a directory at the specified path
Result<void, Error> FileSystem::DeleteDirectory(PCWCHAR path)
{
    PVOID hDir;
    FILE_DISPOSITION_INFORMATION disp;
    UNICODE_STRING uniName;
    OBJECT_ATTRIBUTES objAttr;
    IO_STATUS_BLOCK ioStatusBlock;
    Memory::Zero(&disp, sizeof(FILE_DISPOSITION_INFORMATION));
    disp.DeleteFile = true;

    if (!NTDLL::RtlDosPathNameToNtPathName_U(path, &uniName, nullptr, nullptr))
        return Result<void, Error>::Err(Error::Fs_PathResolveFailed, Error::Fs_DeleteDirFailed);

    InitializeObjectAttributes(&objAttr, &uniName, 0, nullptr, nullptr);

    NTSTATUS status = NTDLL::ZwOpenFile(&hDir, DELETE | SYNCHRONIZE, &objAttr, &ioStatusBlock, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, FILE_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT);
    if (!NT_SUCCESS(status))
    {
        NTDLL::RtlFreeUnicodeString(&uniName);
        return Result<void, Error>::Err(Error::Windows((UINT32)status), Error::Fs_DeleteDirFailed);
    }

    status = NTDLL::ZwSetInformationFile(
        hDir,
        &ioStatusBlock,
        &disp,
        sizeof(disp),
        FileDispositionInformation);

    (void)NTDLL::ZwClose(hDir);
    NTDLL::RtlFreeUnicodeString(&uniName);

    if (!NT_SUCCESS(status))
        return Result<void, Error>::Err(Error::Windows((UINT32)status), Error::Fs_DeleteDirFailed);

    return Result<void, Error>::Ok();
}
// --- DirectoryIterator Implementation ---

// Helper to fill the entry from FILE_BOTH_DIR_INFORMATION
static void FillEntry(DirectoryEntry &entry, const FILE_BOTH_DIR_INFORMATION &data)
{
    // 1. Copy Name (FileNameLength is in bytes, divide by sizeof(WCHAR))
    UINT32 nameLen = data.FileNameLength / sizeof(WCHAR);
    if (nameLen > 255)
        nameLen = 255;
    for (UINT32 j = 0; j < nameLen; j++)
    {
        entry.name[j] = data.FileName[j];
    }
    entry.name[nameLen] = L'\0';

    // 2. Size
    entry.size = data.EndOfFile.QuadPart;

    // 3. Attributes
    UINT32 attr = data.FileAttributes;
    entry.isDirectory = (attr & FILE_ATTRIBUTE_DIRECTORY);
    entry.isHidden = (attr & FILE_ATTRIBUTE_HIDDEN);
    entry.isSystem = (attr & FILE_ATTRIBUTE_SYSTEM);
    entry.isReadOnly = (attr & FILE_ATTRIBUTE_READONLY);

    // 4. Timestamps
    entry.creationTime = data.CreationTime.QuadPart;

    // 5. lastModifiedTime
    entry.lastModifiedTime = data.LastWriteTime.QuadPart;

    // 6. IsDrive
    entry.isDrive = (entry.name[1] == ':' && entry.name[2] == L'\0');

    entry.type = 3; // Default to Fixed
}

// DirectoryIterator Constructor
DirectoryIterator::DirectoryIterator(PCWCHAR path) : handle((PVOID)-1), first(true), isBitMaskMode(false)
{
    NTSTATUS status;
    // CASE: List Drives (Path is empty or nullptr)
    if (!path || path[0] == L'\0')
    {
        PROCESS_DEVICEMAP_INFORMATION ProcessDeviceMapInfo;
        status = NTDLL::ZwQueryInformationProcess(
            NTDLL::NtCurrentProcess(),
            ProcessDeviceMap,
            &ProcessDeviceMapInfo.Query,
            sizeof(ProcessDeviceMapInfo.Query),
            nullptr);

        if (!NT_SUCCESS(status))
        {
            return;
        }
        if (ProcessDeviceMapInfo.Query.DriveMap != 0)
        {
            // Store the mask in the pointer itself
            handle = (PVOID)(USIZE)ProcessDeviceMapInfo.Query.DriveMap;
            first = true; // Flag to indicate we are in "Drive Mode"
            isBitMaskMode = true;
        }
        return;
    }

    // Convert path to NT path and open directory handle
    UNICODE_STRING uniPath;
    if (!NTDLL::RtlDosPathNameToNtPathName_U(path, &uniPath, nullptr, nullptr))
        return;

    OBJECT_ATTRIBUTES objAttr;
    InitializeObjectAttributes(&objAttr, &uniPath, OBJ_CASE_INSENSITIVE, nullptr, nullptr);

    IO_STATUS_BLOCK ioStatusBlock;
    status = NTDLL::ZwOpenFile(
        &handle,
        FILE_LIST_DIRECTORY | SYNCHRONIZE,
        &objAttr,
        &ioStatusBlock,
        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
        FILE_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT);

    NTDLL::RtlFreeUnicodeString(&uniPath);

    if (!NT_SUCCESS(status))
    {
        handle = (PVOID)-1;
        return;
    }

    // Query the first entry
    alignas(alignof(FILE_BOTH_DIR_INFORMATION)) UINT8 buffer[sizeof(FILE_BOTH_DIR_INFORMATION) + 260 * sizeof(WCHAR)];
    Memory::Zero(buffer, sizeof(buffer));

    status = NTDLL::ZwQueryDirectoryFile(
        handle,
        nullptr,
        nullptr,
        nullptr,
        &ioStatusBlock,
        buffer,
        sizeof(buffer),
        FileBothDirectoryInformation,
        true,
        nullptr,
        true);

    if (NT_SUCCESS(status))
    {
        const FILE_BOTH_DIR_INFORMATION &info = *(const FILE_BOTH_DIR_INFORMATION *)buffer;
        FillEntry(currentEntry, info);
    }
    else
    {
        (void)NTDLL::ZwClose(handle);
        handle = (PVOID)-1;
    }
}

// Move to next entry. Returns false when no more files.
BOOL DirectoryIterator::Next()
{
    if (!IsValid())
        return false;

    IO_STATUS_BLOCK ioStatusBlock;

    // --- MODE 1: Drive Bitmask Mode (first is true and handle is small) ---
    // We treat handles < 0x1000000 as bitmasks (drives)
    if (isBitMaskMode)
    {
        USIZE mask = (USIZE)handle;

        if (mask == 0)
            return false;

        // Query the process device map to get drive types
        PROCESS_DEVICEMAP_INFORMATION devMapInfo;
        Memory::Zero(&devMapInfo, sizeof(devMapInfo));
        NTSTATUS devMapStatus = NTDLL::ZwQueryInformationProcess(
            NTDLL::NtCurrentProcess(),
            ProcessDeviceMap,
            &devMapInfo.Query,
            sizeof(devMapInfo.Query),
            nullptr);

        // Find the next set bit
        for (INT32 i = 0; i < 26; i++)
        {
            if (mask & (1 << i))
            {
                // Found a drive! Format it as "X:\"
                currentEntry.name[0] = (WCHAR)(L'A' + i);
                currentEntry.name[1] = L':';
                currentEntry.name[2] = L'\\';
                currentEntry.name[3] = L'\0';

                currentEntry.isDirectory = true;
                currentEntry.isDrive = true;

                // DriveType[] uses Win32 drive type constants directly
                if (NT_SUCCESS(devMapStatus))
                    currentEntry.type = (UINT32)devMapInfo.Query.DriveType[i];
                else
                    currentEntry.type = DRIVE_UNKNOWN;

                // Update mask for next time (remove the bit we just processed)
                mask &= ~(1 << i);
                handle = (PVOID)mask;
                first = false;

                return true;
            }
        }

        return false;
    }

    // --- NORMAL MODE ---
    if (first)
    {
        first = false;
        return true;
    }

    alignas(alignof(FILE_BOTH_DIR_INFORMATION)) UINT8 buffer[sizeof(FILE_BOTH_DIR_INFORMATION) + 260 * sizeof(WCHAR)];
    Memory::Zero(buffer, sizeof(buffer));

    NTSTATUS status = NTDLL::ZwQueryDirectoryFile(
        handle,
        nullptr,
        nullptr,
        nullptr,
        &ioStatusBlock,
        buffer,
        sizeof(buffer),
        FileBothDirectoryInformation,
        true,
        nullptr,
        false);

    if (NT_SUCCESS(status))
    {
        const FILE_BOTH_DIR_INFORMATION &dirInfo = *(const FILE_BOTH_DIR_INFORMATION *)buffer;
        FillEntry(currentEntry, dirInfo);
        return true;
    }

    return false;
}

// Move constructor
DirectoryIterator::DirectoryIterator(DirectoryIterator &&other) noexcept
    : handle(other.handle), currentEntry(other.currentEntry), first(other.first), isBitMaskMode(other.isBitMaskMode)
{
    other.handle = (PVOID)-1;
}

DirectoryIterator &DirectoryIterator::operator=(DirectoryIterator &&other) noexcept
{
    if (this != &other)
    {
        if (IsValid() && !isBitMaskMode)
            (void)NTDLL::ZwClose(handle);
        handle = other.handle;
        currentEntry = other.currentEntry;
        first = other.first;
        isBitMaskMode = other.isBitMaskMode;
        other.handle = (PVOID)-1;
    }
    return *this;
}

DirectoryIterator::~DirectoryIterator()
{
    // If handle is a bitmask (less than a valid memory address), don't close it
    if (IsValid())
    {
        if (!isBitMaskMode)
        {
            (void)NTDLL::ZwClose(handle);
        }
        // If it's a bitmask, we do nothing. No memory was allocated!
        handle = (PVOID)-1;
    }
}

// Check if the iterator is valid
BOOL DirectoryIterator::IsValid() const
{
    // Windows returns (HANDLE)-1 (0xFFFFFFFF) on failure for FindFirstFile
    return handle != nullptr && handle != (PVOID)(SSIZE)-1;
}