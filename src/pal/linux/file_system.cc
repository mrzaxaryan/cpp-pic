#include "file_system.h"
#include "syscall.h"
#include "primitives.h"
#include "string.h"

// Linux syscall numbers for this file
#if defined(ARCHITECTURE_X86_64)
constexpr USIZE SYS_OPEN = 2;
constexpr USIZE SYS_CLOSE = 3;
constexpr USIZE SYS_READ = 0;
constexpr USIZE SYS_WRITE = 1;
constexpr USIZE SYS_LSEEK = 8;
#elif defined(ARCHITECTURE_I386)
constexpr USIZE SYS_OPEN = 5;
constexpr USIZE SYS_CLOSE = 6;
constexpr USIZE SYS_READ = 3;
constexpr USIZE SYS_WRITE = 4;
constexpr USIZE SYS_LSEEK = 19;
#elif defined(ARCHITECTURE_AARCH64)
constexpr USIZE SYS_OPENAT = 56;
constexpr USIZE SYS_CLOSE = 57;
constexpr USIZE SYS_READ = 63;
constexpr USIZE SYS_WRITE = 64;
constexpr USIZE SYS_LSEEK = 62;
#elif defined(ARCHITECTURE_ARMV7A)
constexpr USIZE SYS_OPEN = 5;
constexpr USIZE SYS_CLOSE = 6;
constexpr USIZE SYS_READ = 3;
constexpr USIZE SYS_WRITE = 4;
constexpr USIZE SYS_LSEEK = 19;
#endif

// Linux open flags
constexpr INT32 O_RDONLY = 0x0000;
constexpr INT32 O_WRONLY = 0x0001;
constexpr INT32 O_RDWR = 0x0002;
constexpr INT32 O_CREAT = 0x0040;
constexpr INT32 O_TRUNC = 0x0200;
constexpr INT32 O_APPEND = 0x0400;

// Linux file modes
constexpr INT32 S_IRUSR = 0x0100;  // User read
constexpr INT32 S_IWUSR = 0x0080;  // User write
constexpr INT32 S_IRGRP = 0x0020;  // Group read
constexpr INT32 S_IWGRP = 0x0010;  // Group write
constexpr INT32 S_IROTH = 0x0004;  // Others read

// Invalid file descriptor
constexpr SSIZE INVALID_FD = -1;

// Helper: Convert wide char path to UTF-8
static BOOL WCharToUtf8(PCWCHAR wstr, CHAR* utf8, USIZE maxLen)
{
    USIZE pos = 0;
    USIZE i = 0;

    while (wstr[i] != L'\0' && pos < maxLen - 4)
    {
        UINT32 codepoint = wstr[i++];

        // Handle UTF-16 surrogate pairs
        if (codepoint >= 0xD800 && codepoint <= 0xDBFF && wstr[i] != L'\0')
        {
            UINT32 low = wstr[i];
            if (low >= 0xDC00 && low <= 0xDFFF)
            {
                codepoint = 0x10000 + ((codepoint & 0x3FF) << 10) + (low & 0x3FF);
                i++;
            }
        }

        // Convert to UTF-8
        if (codepoint < 0x80)
        {
            utf8[pos++] = (CHAR)codepoint;
        }
        else if (codepoint < 0x800)
        {
            utf8[pos++] = (CHAR)(0xC0 | (codepoint >> 6));
            utf8[pos++] = (CHAR)(0x80 | (codepoint & 0x3F));
        }
        else if (codepoint < 0x10000)
        {
            utf8[pos++] = (CHAR)(0xE0 | (codepoint >> 12));
            utf8[pos++] = (CHAR)(0x80 | ((codepoint >> 6) & 0x3F));
            utf8[pos++] = (CHAR)(0x80 | (codepoint & 0x3F));
        }
        else if (codepoint < 0x110000)
        {
            utf8[pos++] = (CHAR)(0xF0 | (codepoint >> 18));
            utf8[pos++] = (CHAR)(0x80 | ((codepoint >> 12) & 0x3F));
            utf8[pos++] = (CHAR)(0x80 | ((codepoint >> 6) & 0x3F));
            utf8[pos++] = (CHAR)(0x80 | (codepoint & 0x3F));
        }
    }

    utf8[pos] = '\0';
    return TRUE;
}

// --- File Implementation ---

File::File(PVOID handle) : fileHandle(handle), fileSize(0)
{
    // TODO: Get file size using fstat if needed
}

File::File(File&& other) noexcept : fileHandle(nullptr), fileSize(0)
{
    fileHandle = other.fileHandle;
    fileSize = other.fileSize;
    other.fileHandle = (PVOID)INVALID_FD;
    other.fileSize = 0;
}

File& File::operator=(File&& other) noexcept
{
    if (this != &other)
    {
        Close();
        fileHandle = other.fileHandle;
        fileSize = other.fileSize;
        other.fileHandle = (PVOID)INVALID_FD;
        other.fileSize = 0;
    }
    return *this;
}

BOOL File::IsValid() const
{
    SSIZE fd = (SSIZE)fileHandle;
    return fd >= 0;
}

VOID File::Close()
{
    if (IsValid())
    {
        Syscall::syscall1(SYS_CLOSE, (USIZE)fileHandle);
        fileHandle = (PVOID)INVALID_FD;
        fileSize = 0;
    }
}

UINT32 File::Read(PVOID buffer, UINT32 size)
{
    if (!IsValid())
        return 0;

    SSIZE result = Syscall::syscall3(SYS_READ, (USIZE)fileHandle, (USIZE)buffer, size);
    return (result >= 0) ? (UINT32)result : 0;
}

UINT32 File::Write(const VOID* buffer, USIZE size)
{
    if (!IsValid())
        return 0;

    SSIZE result = Syscall::syscall3(SYS_WRITE, (USIZE)fileHandle, (USIZE)buffer, size);
    return (result >= 0) ? (UINT32)result : 0;
}

USIZE File::GetOffset() const
{
    if (!IsValid())
        return 0;

    // lseek with offset 0 and SEEK_CUR returns current position
    SSIZE result = Syscall::syscall3(SYS_LSEEK, (USIZE)fileHandle, 0, (USIZE)OffsetOrigin::Current);
    return (result >= 0) ? (USIZE)result : 0;
}

VOID File::SetOffset(USIZE absoluteOffset)
{
    if (!IsValid())
        return;

    Syscall::syscall3(SYS_LSEEK, (USIZE)fileHandle, absoluteOffset, (USIZE)OffsetOrigin::Start);
}

VOID File::MoveOffset(SSIZE relativeAmount, OffsetOrigin origin)
{
    if (!IsValid())
        return;

    Syscall::syscall3(SYS_LSEEK, (USIZE)fileHandle, (USIZE)relativeAmount, (USIZE)origin);
}

// --- FileSystem Implementation ---

File FileSystem::Open(PCWCHAR path, INT32 flags)
{
    // Convert wide char path to UTF-8
    CHAR utf8Path[1024];
    if (!WCharToUtf8(path, utf8Path, sizeof(utf8Path)))
        return File();

    // Map FileSystem flags to Linux open flags
    INT32 openFlags = 0;
    INT32 mode = S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH;

    // Access mode
    if ((flags & FS_READ) && (flags & FS_WRITE))
        openFlags |= O_RDWR;
    else if (flags & FS_WRITE)
        openFlags |= O_WRONLY;
    else
        openFlags |= O_RDONLY;

    // Creation/truncation flags
    if (flags & FS_CREATE)
        openFlags |= O_CREAT;
    if (flags & FS_TRUNCATE)
        openFlags |= O_TRUNC;
    if (flags & FS_APPEND)
        openFlags |= O_APPEND;

    // Open the file
    SSIZE fd;
#if defined(ARCHITECTURE_X86_64) || defined(ARCHITECTURE_I386) || defined(ARCHITECTURE_ARMV7A)
    fd = Syscall::syscall3(SYS_OPEN, (USIZE)utf8Path, openFlags, mode);
#else
    // ARM64 uses openat instead of open
    constexpr SSIZE AT_FDCWD = -100;
    fd = Syscall::syscall4(SYS_OPENAT, AT_FDCWD, (USIZE)utf8Path, openFlags, mode);
#endif

    if (fd < 0)
        return File();

    return File((PVOID)fd);
}

BOOL FileSystem::Delete(PCWCHAR path)
{
    // TODO: Implement using unlink syscall
    (VOID)path;
    return FALSE;
}

BOOL FileSystem::Exists(PCWCHAR path)
{
    // TODO: Implement using stat syscall
    (VOID)path;
    return FALSE;
}

BOOL FileSystem::CreateDirectroy(PCWCHAR path)
{
    // TODO: Implement using mkdir syscall
    (VOID)path;
    return FALSE;
}

BOOL FileSystem::DeleteDirectory(PCWCHAR path)
{
    // TODO: Implement using rmdir syscall
    (VOID)path;
    return FALSE;
}

// --- DirectoryIterator Implementation ---

DirectoryIterator::DirectoryIterator(PCWCHAR path)
    : handle((PVOID)INVALID_FD), nread(0), bpos(0)
{
    // TODO: Implement directory iteration using getdents syscall
    (VOID)path;
    (VOID)buffer;  // Suppress unused warning
    (VOID)nread;   // Suppress unused warning
    (VOID)bpos;    // Suppress unused warning
    first = TRUE;
}

DirectoryIterator::~DirectoryIterator()
{
    if (IsValid())
    {
        Syscall::syscall1(SYS_CLOSE, (USIZE)handle);
        handle = (PVOID)INVALID_FD;
    }
}

BOOL DirectoryIterator::Next()
{
    // TODO: Implement directory iteration
    return FALSE;
}

BOOL DirectoryIterator::IsValid() const
{
    return (SSIZE)handle >= 0;
}
