#pragma once

#include "primitives.h"

// Offset origin for file seeking
enum class OffsetOrigin : INT32
{
    Start = 0,   // Beginning of the file
    Current = 1, // Current file pointer position
    End = 2      // End of the file
};

// Directory entry structure
struct DirectoryEntry
{
    WCHAR name[256];  // File or directory name
    USIZE size;       // Size in bytes
    UINT32 type;      // Store DriveType (2=Removable, 3=Fixed, etc.)
    BOOL isDirectory; // Set if the entry is a directory
    BOOL isDrive;     // Set if the entry represents a root (e.g., C:\)
    BOOL isHidden;    // Flag for hidden files
    BOOL isSystem;    // Flag for system files
    BOOL isReadOnly;  // Flag for read-only files

    UINT64 creationTime; // Filetime format
};

// Class to represent a file
class File
{
private:
    PVOID fileHandle; // File handle
    USIZE fileSize;   // File size

    // Private constructor for FileSystem's use
    File(PVOID handle);

public:
    // Default constructor and destructor
    File() : fileHandle(nullptr), fileSize(0) {}
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

    // Check if the file handle is valid
    BOOL IsValid() const;
    // Close the file handle
    VOID Close();

    // Read and write methods
    UINT32 Read(PVOID buffer, UINT32 size);
    UINT32 Write(const VOID *buffer, USIZE size);

    // Get the size of the file
    USIZE GetSize() const { return fileSize; }
    // Get, set and move the file offset
    USIZE GetOffset() const;
    VOID SetOffset(USIZE absoluteOffset);
    VOID MoveOffset(SSIZE relativeAmount, OffsetOrigin origin = OffsetOrigin::Current);

    // Friend class for FileSystem to access private constructor
    friend class FileSystem;
};

// Class to iterate over directory entries
class DirectoryIterator
{
private:
    PVOID handle;                // Handle to the directory or drive bitmask
    DirectoryEntry currentEntry; // Current directory entry
    BOOL first;                  // Flag for first call to Next()
#ifdef PLATFORM_WINDOWS
    BOOL isBitMaskMode; // Flag for bitmask mode on Windows
#endif

#if defined(PLATFORM_LINUX) || defined(PLATFORM_MACOS)
    // Linux/macOS-specific (directory entry buffering for getdents64/getdirentries64)
    CHAR buffer[1024];
    INT32 nread;
    INT32 bpos;
#endif

public:
    // Constructor and destructor
    DirectoryIterator(PCWCHAR path);
    ~DirectoryIterator();

    // Move to next entry. Returns FALSE when no more files.
    BOOL Next();
    // Get the current directory entry
    const DirectoryEntry &Get() const { return currentEntry; }
    // Check if the iterator is valid
    BOOL IsValid() const;

    // Non-copyable
    DirectoryIterator(const DirectoryIterator &) = delete;
    DirectoryIterator &operator=(const DirectoryIterator &) = delete;

    // Movable (transfer ownership of directory handle)
    DirectoryIterator(DirectoryIterator &&other) noexcept;
    DirectoryIterator &operator=(DirectoryIterator &&other) noexcept;

    // Stack-only
    VOID *operator new(USIZE) = delete;
    VOID operator delete(VOID *) = delete;
};

// Class to handle file system operations
class FileSystem
{
public:
    static constexpr INT32 FS_READ = 0x0001;
    static constexpr INT32 FS_WRITE = 0x0002;
    static constexpr INT32 FS_APPEND = 0x0004;
    static constexpr INT32 FS_CREATE = 0x0008;
    static constexpr INT32 FS_TRUNCATE = 0x0010;
    static constexpr INT32 FS_BINARY = 0x0020;

    // File operations
    static File Open(PCWCHAR path, INT32 flags = 0);
    static BOOL Delete(PCWCHAR path);
    static BOOL Exists(PCWCHAR path);

    // New Directory Methods
    static BOOL CreateDirectory(PCWCHAR path);
    static BOOL DeleteDirectory(PCWCHAR path);
};
