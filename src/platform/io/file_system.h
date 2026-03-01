#pragma once

#include "primitives.h"
#include "span.h"
#include "error.h"
#include "result.h"

// Offset origin for file seeking
enum class OffsetOrigin : INT32
{
    Start = 0,   // Beginning of the file
    Current = 1, // Current file pointer position
    End = 2      // End of the file
};

#pragma pack(push, 1) // Ensure no padding in structures
// Directory entry structure
struct DirectoryEntry
{
    WCHAR name[256];         // File or directory name
    UINT64 creationTime;     // Filetime format
    UINT64 lastModifiedTime; // Filetime format
    UINT64 size;             // Size in bytes
    UINT32 type;             // Store DriveType (2=Removable, 3=Fixed, etc.)
    BOOL isDirectory;        // Set if the entry is a directory
    BOOL isDrive;            // Set if the entry represents a root (e.g., C:\)
    BOOL isHidden;           // Flag for hidden files
    BOOL isSystem;           // Flag for system files
    BOOL isReadOnly;         // Flag for read-only files
};

#pragma pack(pop)

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
    BOOL isBitMaskMode = false; // Flag for bitmask mode on Windows
#endif

#if defined(PLATFORM_LINUX) || defined(PLATFORM_MACOS)
    // Linux/macOS-specific (directory entry buffering for getdents64/getdirentries64)
    CHAR buffer[1024];
    INT32 nread;
    INT32 bpos;
#endif

    // Private constructor for factory use only
    DirectoryIterator();

public:
    ~DirectoryIterator();

    // Non-copyable
    DirectoryIterator(const DirectoryIterator &) = delete;
    DirectoryIterator &operator=(const DirectoryIterator &) = delete;

    // Movable (transfer ownership of directory handle)
    DirectoryIterator(DirectoryIterator &&other) noexcept;
    DirectoryIterator &operator=(DirectoryIterator &&other) noexcept;

    // Stack-only (placement new allowed for Result<>)
    VOID *operator new(USIZE) = delete;
    VOID operator delete(VOID *) = delete;
    VOID *operator new(USIZE, PVOID ptr) noexcept { return ptr; }

    // Factory — creates and initializes an iterator for the given path
    [[nodiscard]] static Result<DirectoryIterator, Error> Create(PCWCHAR path);

    // Move to next entry. Ok = has entry, Err = done or syscall failed.
    [[nodiscard]] Result<void, Error> Next();
    // Get the current directory entry
    const DirectoryEntry &Get() const { return currentEntry; }
    // Check if the iterator is valid
    BOOL IsValid() const;
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
    [[nodiscard]] static Result<File, Error> Open(PCWCHAR path, INT32 flags = 0);
    [[nodiscard]] static Result<void, Error> Delete(PCWCHAR path);
    [[nodiscard]] static Result<void, Error> Exists(PCWCHAR path);

    // New Directory Methods
    [[nodiscard]] static Result<void, Error> CreateDirectory(PCWCHAR path);
    [[nodiscard]] static Result<void, Error> DeleteDirectory(PCWCHAR path);
};
