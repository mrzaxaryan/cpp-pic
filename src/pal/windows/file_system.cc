#include "file_system.h"
#include "logger.h"
#include "primitives.h"
#include "kernel32.h"
#include "string.h"
#include "windows_types.h"
#include "ntdll.h"

// --- Internal Constructor ---
// Translates a Windows HANDLE into our File object
File::File(void *handle) : fileHandle(handle), fileSize(0)
{
    if (IsValid())
    {
        FILE_STANDARD_INFORMATION fileStandardInfo{};
        IO_STATUS_BLOCK ioStatusBlock;

        if (NTDLL::NtQueryInformationFile(fileHandle, &ioStatusBlock, &fileStandardInfo, sizeof(fileStandardInfo), FileStandardInformation) == 0)
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
    // but some APIs return NULL. We check for both.
    return fileHandle != nullptr && fileHandle != INVALID_HANDLE_VALUE;
}

// Close the file handle
void File::Close()
{
    if (IsValid())
    {
        NTDLL::NtClose((PVOID)fileHandle);
        fileHandle = nullptr;
        fileSize = 0;
    }
}

// Read data from the file into the buffer
UINT32 File::Read(void *buffer, UINT32 size)
{
    if (!IsValid())
        return 0;

    // Properly initialize IO_STATUS_BLOCK with all fields
    IO_STATUS_BLOCK ioStatusBlock;
    if (NTDLL::NtReadFile((PVOID)fileHandle, NULL, NULL, NULL, &ioStatusBlock, buffer, (UINT32)size, NULL, NULL) == 0)
    {
        return (UINT32)ioStatusBlock.Information;
    }
    return 0;
}

// Write data from the buffer to the file
UINT32 File::Write(const void *buffer, USIZE size)
{
    if (!IsValid())
        return 0;

    IO_STATUS_BLOCK ioStatusBlock;
    if (NTDLL::NtWriteFile((PVOID)fileHandle, NULL, NULL, NULL, &ioStatusBlock, (PVOID)buffer, (UINT32)size, NULL, NULL) == 0)
    {
        return (UINT32)ioStatusBlock.Information;
    }
    return 0;
}

// Get the current file offset
USIZE File::GetOffset() const
{
    if (!IsValid())
        return 0;

    FILE_POSITION_INFORMATION posFIle;
    IO_STATUS_BLOCK ioStatusBlock;
    // In Win32, moving 0 bytes from the current position returns the current offset
    if (NTDLL::NtQueryInformationFile((PVOID)fileHandle, &ioStatusBlock, &posFIle, sizeof(posFIle), FilePositionInformation) == 0)
    {
        return (USIZE)posFIle.CurrentByteOffset.QuadPart;
    }
    return 0;
}

// Set the file offset to an absolute position
void File::SetOffset(USIZE absoluteOffset)
{
    if (!IsValid())
        return;

    FILE_POSITION_INFORMATION posInfo{};
    posInfo.CurrentByteOffset.QuadPart = INT64((signed long long)absoluteOffset);
    IO_STATUS_BLOCK ioStatusBlock{};
    // Set the file pointer to the specified absolute offset using NtSetInformationFile
    NTDLL::NtSetInformationFile((PVOID)fileHandle, &ioStatusBlock, &posInfo, sizeof(posInfo), FilePositionInformation);
}

// Move the file offset by a relative amount based on the specified origin
void File::MoveOffset(SSIZE relativeAmount, OffsetOrigin origin)
{
    if (!IsValid())
        return;

    IO_STATUS_BLOCK ioStatusBlock{};
    FILE_POSITION_INFORMATION posInfo{};
    FILE_STANDARD_INFORMATION fileStandardInfo{}; // Properly initialize EndOfFile
    INT64 distance = 0;

    if (NTDLL::NtQueryInformationFile((PVOID)fileHandle, &ioStatusBlock, &posInfo, sizeof(posInfo), FilePositionInformation) != 0)
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
        if (NTDLL::NtQueryInformationFile(fileHandle, &ioStatusBlock, &fileStandardInfo, sizeof(fileStandardInfo), FileStandardInformation) != 0)
            return;
        distance = fileStandardInfo.EndOfFile.QuadPart + relativeAmount;
        break;
    default:
        LOG_ERROR("Invalid OffsetOrigin specified in MoveOffset");
        break;
    }
    posInfo.CurrentByteOffset.QuadPart = distance;
    NTDLL::NtSetInformationFile((PVOID)fileHandle, &ioStatusBlock, &posInfo, sizeof(posInfo), FilePositionInformation);
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
        LOG_ERROR("FS_TRUNCATE flag set.");
        dwCreationDisposition = FILE_OVERWRITE;
    }

    // Map NT flags
    if (!(flags & FILE_FLAG_OVERLAPPED))
        ntFlags |= FILE_SYNCHRONOUS_IO_NONALERT;
    if (flags & FILE_FLAG_WRITE_THROUGH)
        ntFlags |= FILE_WRITE_THROUGH;
    ntFlags |= FILE_NON_DIRECTORY_FILE;

    // Always allow waiting and querying attributes
    dwDesiredAccess |= SYNCHRONIZE | FILE_READ_ATTRIBUTES;

    // Convert DOS path to NT path
    UNICODE_STRING ntPathU;
    if (!NTDLL::RtlDosPathNameToNtPathName_U(path, &ntPathU, NULL, NULL))
        return File();

    OBJECT_ATTRIBUTES objAttr;
    InitializeObjectAttributes(&objAttr, &ntPathU, 0, NULL, NULL);

    IO_STATUS_BLOCK ioStatusBlock;
    PVOID hFile = nullptr;
    NTSTATUS status = NTDLL::NtCreateFile(
        &hFile,
        dwDesiredAccess,
        &objAttr,
        &ioStatusBlock,
        NULL,
        fileAttributes,
        dwShareMode,
        dwCreationDisposition,
        ntFlags,
        NULL,
        0);

    NTDLL::RtlFreeUnicodeString(&ntPathU);

    if (!NT_SUCCESS(status) || hFile == INVALID_HANDLE_VALUE)
        return File();

    return File((void *)hFile);
}

// Delete a file at the specified path
BOOL FileSystem::Delete(PCWCHAR path)
{
    UNICODE_STRING ntName;
    OBJECT_ATTRIBUTES attr;
    NTSTATUS status;
    PVOID hFile = NULL;
    IO_STATUS_BLOCK io;

    if (!NTDLL::RtlDosPathNameToNtPathName_U(path, &ntName, NULL, NULL))
        return FALSE;

    InitializeObjectAttributes(&attr, &ntName, OBJ_CASE_INSENSITIVE, NULL, NULL);

    status = NTDLL::NtCreateFile(&hFile, SYNCHRONIZE | DELETE, &attr, &io, NULL, 0,
                                 FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                                 FILE_OPEN, FILE_DELETE_ON_CLOSE | FILE_NON_DIRECTORY_FILE, NULL, 0);

    if (status == 0)
        status = NTDLL::NtClose(hFile);

    NTDLL::RtlFreeUnicodeString(&ntName);
    return status == 0;
}

// Check if a file exists at the specified path
BOOL FileSystem::Exists(PCWCHAR path)
{
    OBJECT_ATTRIBUTES objAttr;
    UNICODE_STRING uniName;
    FILE_BASIC_INFORMATION fileBasicInfo;

    if (!NTDLL::RtlDosPathNameToNtPathName_U(path, &uniName, NULL, NULL))
        return FALSE;

    InitializeObjectAttributes(&objAttr, &uniName, 0, NULL, NULL);
    NTSTATUS status = NTDLL::NtQueryAttributesFile(&objAttr, &fileBasicInfo);

    NTDLL::RtlFreeUnicodeString(&uniName);

    if (status != 0)
    {
        return FALSE;
    }
    UINT32 attr = fileBasicInfo.FileAttributes;
    // If attributes are not 0xFFFFFFFF, the file exists
    return (attr != 0xFFFFFFFF);
}

// --- FileSystem Directory Management ---
BOOL FileSystem::CreateDirectroy(PCWCHAR path)
{
    // Returns non-zero on success
    PVOID hDir;
    UNICODE_STRING uniName;
    OBJECT_ATTRIBUTES objAttr;
    IO_STATUS_BLOCK ioStatusBlock;

    if (!NTDLL::RtlDosPathNameToNtPathName_U(path, &uniName, NULL, NULL))
        return FALSE;

    InitializeObjectAttributes(&objAttr, &uniName, 0, NULL, NULL);

    NTDLL::RtlFreeUnicodeString(&uniName);

    return NTDLL::NtCreateFile(
               &hDir,
               FILE_LIST_DIRECTORY | SYNCHRONIZE,
               &objAttr,
               &ioStatusBlock,
               NULL,
               FILE_ATTRIBUTE_DIRECTORY,
               FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
               FILE_CREATE,
               FILE_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT,
               NULL,
               0) == 0;
}

// Delete a directory at the specified path
BOOL FileSystem::DeleteDirectory(PCWCHAR path)
{
    PVOID hDir;
    FILE_DISPOSITION_INFORMATION disp = {TRUE};
    UNICODE_STRING uniName;
    OBJECT_ATTRIBUTES objAttr;
    IO_STATUS_BLOCK ioStatusBlock;

    if (!NTDLL::RtlDosPathNameToNtPathName_U(path, &uniName, NULL, NULL))
    {
        return FALSE;
    }
    InitializeObjectAttributes(&objAttr, &uniName, 0, NULL, NULL);

    NTSTATUS status = NTDLL::NtOpenFile(&hDir, DELETE | SYNCHRONIZE, &objAttr, &ioStatusBlock, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, FILE_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT);
    if (status != 0)
        return FALSE;

    status = NTDLL::NtSetInformationFile(
        hDir,
        &ioStatusBlock,
        &disp,
        sizeof(disp),
        FileDispositionInformation);

    NTDLL::NtClose(hDir);
    NTDLL::RtlFreeUnicodeString(&uniName);

    return status == 0;
}
// --- DirectoryIterator Implementation ---

// Helper to fill the entry from WIN32_FIND_DATAA
static void FillEntry(DirectoryEntry &entry, const WIN32_FIND_DATAW &data)
{
    // 1. Copy Name
    for (INT32 j = 0; j < 260; j++)
    {
        entry.name[j] = data.cFileName[j];
        if (data.cFileName[j] == L'\0')
            break;
    }

    // 2. Size
    entry.size = INT64((INT32)data.nFileSizeHigh, data.nFileSizeLow);

    // 3. Attributes
    UINT32 attr = data.dwFileAttributes;
    entry.isDirectory = (attr & 0x10); // FILE_ATTRIBUTE_DIRECTORY
    entry.isHidden = (attr & 0x02);    // FILE_ATTRIBUTE_HIDDEN
    entry.isSystem = (attr & 0x04);    // FILE_ATTRIBUTE_SYSTEM
    entry.isReadOnly = (attr & 0x01);  // FILE_ATTRIBUTE_READONLY

    // 4. Timestamps (Convert 2xUINT32 to UINT64)
    entry.creationTime = UINT64(data.ftCreationTime.dwHighDateTime,
                                data.ftCreationTime.dwLowDateTime);

    // 5. IsDrive
    // Usually false in an iterator unless you are at the "This PC" level.
    // We can check if the name looks like "C:"
    entry.isDrive = (entry.name[1] == ':' && entry.name[2] == L'\0');

    entry.type = 3; // Default to Fixed (or 0 for 'Unknown/Generic')
}

// DirectoryIterator Constructor
DirectoryIterator::DirectoryIterator(PCWCHAR path) : handle((PVOID)-1), first(TRUE), isBitMaskMode(FALSE)
{
    // CASE: List Drives (Path is empty or NULL)
    if (!path || path[0] == L'\0')
    {
        NTSTATUS status;
        PROCESS_DEVICEMAP_INFORMATION ProcessDeviceMapInfo;
        status = NTDLL::NtQueryInformationProcess(
            NTDLL::NtCurrentProcess(),
            ProcessDeviceMap,
            &ProcessDeviceMapInfo.Query,
            sizeof(ProcessDeviceMapInfo.Query),
            NULL);

        if (!NT_SUCCESS(status))
        {
            return;
        }
        if (ProcessDeviceMapInfo.Query.DriveMap != 0)
        {
            // Store the mask in the pointer itself
            handle = (PVOID)(USIZE)ProcessDeviceMapInfo.Query.DriveMap;
            first = TRUE; // Flag to indicate we are in "Drive Mode"
            isBitMaskMode = TRUE;
        }
        return;
    }

    WCHAR searchPath[260];

    // 1. Prepare search string (Path + "\*")
    // Note: You'll need a simple string copy/cat utility
    INT32 i = 0;
    while (path[i] != L'\0' && i < 255)
    {
        searchPath[i] = path[i];
        i++;
    }
    searchPath[i++] = L'\\';
    searchPath[i++] = L'*';
    searchPath[i] = L'\0';

    WIN32_FIND_DATAW findData;
    handle = Kernel32::FindFirstFileW(searchPath, &findData);

    if (IsValid())
    {
        // FindFirstFile actually finds the first entry (usually "." or "..")
        // We store it so the first call to Next() returns it.
        // Or we can fill currentEntry now.
        String::Copy(currentEntry.name, findData.cFileName);
        currentEntry.size = INT64((INT32)findData.nFileSizeHigh, findData.nFileSizeLow);
        currentEntry.isDirectory = (findData.dwFileAttributes & 0x10); // FILE_ATTRIBUTE_DIRECTORY
    }
}

// Move to next entry. Returns FALSE when no more files.
BOOL DirectoryIterator::Next()
{
    if (!IsValid())
        return FALSE;

    FILE_FS_DEVICE_INFORMATION info;
    IO_STATUS_BLOCK ioStatusBlock;

    // --- MODE 1: Drive Bitmask Mode (first is TRUE and handle is small) ---
    // We treat handles < 0x1000000 as bitmasks (drives)
    if (isBitMaskMode && first)
    {
        USIZE mask = (USIZE)handle;

        if (mask == 0)
            return FALSE;

        // Find the next set bit
        for (int i = 0; i < 26; i++)
        {
            if (mask & (1 << i))
            {
                // Found a drive! Format it as "X:\"
                currentEntry.name[0] = (CHAR)('A' + i);
                currentEntry.name[1] = ':';
                currentEntry.name[2] = '\\';
                currentEntry.name[3] = '\0';

                currentEntry.isDirectory = TRUE;
                currentEntry.isDrive = TRUE;

                NTDLL::NtQueryVolumeInformationFile(
                    NTDLL::NtCurrentProcess(),
                    &ioStatusBlock,
                    &info,
                    sizeof(info),
                    FileFsDeviceInformation);
                NTDLL::NtClose(handle);

                switch (info.DeviceType)
                {
                case FILE_DEVICE_CD_ROM_FILE_SYSTEM:
                    currentEntry.type = DRIVE_CDROM;
                    break;
                case FILE_DEVICE_VIRTUAL_DISK:
                    currentEntry.type = DRIVE_RAMDISK;
                    break;
                case FILE_DEVICE_NETWORK_FILE_SYSTEM:
                    currentEntry.type = DRIVE_REMOTE;
                    break;
                case FILE_DEVICE_DISK_FILE_SYSTEM:
                    if (info.Characteristics & FILE_REMOTE_DEVICE)
                        currentEntry.type = DRIVE_REMOTE;
                    else if (info.Characteristics & FILE_REMOVABLE_MEDIA)
                        currentEntry.type = DRIVE_REMOVABLE;
                    else
                        currentEntry.type = DRIVE_FIXED;
                    break;
                default:
                    currentEntry.type = DRIVE_UNKNOWN;
                    break;
                }
            }

            // Update mask for next time (remove the bit we just processed)
            mask &= ~(1 << i);
            handle = (PVOID)mask;
            first = FALSE;

            return TRUE;
        }

        return FALSE;
    }

    // --- NORMAL MODE ---
    if (first)
    {
        first = FALSE;
        return TRUE;
    }

    WIN32_FIND_DATAW findData;
    if (Kernel32::FindNextFileW(handle, &findData))
    {
        FillEntry(currentEntry, findData);
        return TRUE;
    }

    return FALSE;
}

// Destructor
DirectoryIterator::~DirectoryIterator()
{
    // If handle is a bitmask (less than a valid memory address), don't close it
    if (IsValid())
    {
        if (!isBitMaskMode)
        {
            Kernel32::FindClose(handle);
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