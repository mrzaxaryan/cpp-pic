#include "file_system.h"
#include "memory.h"
#include "syscall.h"
#include "system.h"
#include "string.h"
#include "utf16.h"

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
        System::Call(SYS_CLOSE, (USIZE)fileHandle);
        fileHandle = (PVOID)INVALID_FD;
        fileSize = 0;
    }
}

UINT32 File::Read(PVOID buffer, UINT32 size)
{
    if (!IsValid())
        return 0;

    SSIZE result = System::Call(SYS_READ, (USIZE)fileHandle, (USIZE)buffer, size);
    return (result >= 0) ? (UINT32)result : 0;
}

UINT32 File::Write(const VOID* buffer, USIZE size)
{
    if (!IsValid())
        return 0;

    SSIZE result = System::Call(SYS_WRITE, (USIZE)fileHandle, (USIZE)buffer, size);
    return (result >= 0) ? (UINT32)result : 0;
}

USIZE File::GetOffset() const
{
    if (!IsValid())
        return 0;

    SSIZE result = System::Call(SYS_LSEEK, (USIZE)fileHandle, 0, SEEK_CUR);
    return (result >= 0) ? (USIZE)result : 0;
}

VOID File::SetOffset(USIZE absoluteOffset)
{
    if (!IsValid())
        return;

    System::Call(SYS_LSEEK, (USIZE)fileHandle, absoluteOffset, SEEK_SET);
}

VOID File::MoveOffset(SSIZE relativeAmount, OffsetOrigin origin)
{
    if (!IsValid())
        return;

    INT32 whence = SEEK_CUR;
    if (origin == OffsetOrigin::Start)
        whence = SEEK_SET;
    else if (origin == OffsetOrigin::End)
        whence = SEEK_END;

    System::Call(SYS_LSEEK, (USIZE)fileHandle, (USIZE)relativeAmount, whence);
}

// --- FileSystem Implementation ---

File FileSystem::Open(PCWCHAR path, INT32 flags)
{
    CHAR utf8Path[1024];
    USIZE pathLen = String::Length(path);
    USIZE utf8Len = UTF16::ToUTF8(path, pathLen, utf8Path, sizeof(utf8Path) - 1);
    utf8Path[utf8Len] = '\0';

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

    SSIZE fd;
#if defined(ARCHITECTURE_X86_64) || defined(ARCHITECTURE_I386) || defined(ARCHITECTURE_ARMV7A)
    fd = System::Call(SYS_OPEN, (USIZE)utf8Path, openFlags, mode);
#else
    fd = System::Call(SYS_OPENAT, AT_FDCWD, (USIZE)utf8Path, openFlags, mode);
#endif

    if (fd < 0)
        return File();

    return File((PVOID)fd);
}

BOOL FileSystem::Delete(PCWCHAR path)
{
    CHAR utf8Path[1024];
    USIZE pathLen = String::Length(path);
    USIZE utf8Len = UTF16::ToUTF8(path, pathLen, utf8Path, sizeof(utf8Path) - 1);
    utf8Path[utf8Len] = '\0';

#if defined(ARCHITECTURE_AARCH64)
    return System::Call(SYS_UNLINKAT, AT_FDCWD, (USIZE)utf8Path, 0) == 0;
#else
    return System::Call(SYS_UNLINK, (USIZE)utf8Path) == 0;
#endif
}

BOOL FileSystem::Exists(PCWCHAR path)
{
    CHAR utf8Path[1024];
    USIZE pathLen = String::Length(path);
    USIZE utf8Len = UTF16::ToUTF8(path, pathLen, utf8Path, sizeof(utf8Path) - 1);
    utf8Path[utf8Len] = '\0';

    UINT8 statbuf[144];

#if defined(ARCHITECTURE_AARCH64)
    return System::Call(SYS_FSTATAT, AT_FDCWD, (USIZE)utf8Path, (USIZE)statbuf, 0) == 0;
#else
    return System::Call(SYS_STAT, (USIZE)utf8Path, (USIZE)statbuf) == 0;
#endif
}

BOOL FileSystem::CreateDirectory(PCWCHAR path)
{
    CHAR utf8Path[1024];
    USIZE pathLen = String::Length(path);
    USIZE utf8Len = UTF16::ToUTF8(path, pathLen, utf8Path, sizeof(utf8Path) - 1);
    utf8Path[utf8Len] = '\0';

    // Mode 0755 (rwxr-xr-x)
    INT32 mode = S_IRUSR | S_IWUSR | S_IXUSR | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH;

#if defined(ARCHITECTURE_AARCH64)
    SSIZE result = System::Call(SYS_MKDIRAT, AT_FDCWD, (USIZE)utf8Path, mode);
#else
    SSIZE result = System::Call(SYS_MKDIR, (USIZE)utf8Path, mode);
#endif
    return result == 0 || result == -17; // -EEXIST: directory already exists
}

BOOL FileSystem::DeleteDirectory(PCWCHAR path)
{
    CHAR utf8Path[1024];
    USIZE pathLen = String::Length(path);
    USIZE utf8Len = UTF16::ToUTF8(path, pathLen, utf8Path, sizeof(utf8Path) - 1);
    utf8Path[utf8Len] = '\0';

#if defined(ARCHITECTURE_AARCH64)
    return System::Call(SYS_UNLINKAT, AT_FDCWD, (USIZE)utf8Path, AT_REMOVEDIR) == 0;
#else
    return System::Call(SYS_RMDIR, (USIZE)utf8Path) == 0;
#endif
}

// --- DirectoryIterator Implementation ---

DirectoryIterator::DirectoryIterator(PCWCHAR path)
    : handle((PVOID)INVALID_FD), first(FALSE), nread(0), bpos(0)
{
    CHAR utf8Path[1024];

    if (path && path[0] != L'\0')
    {
        USIZE pathLen = String::Length(path);
        USIZE utf8Len = UTF16::ToUTF8(path, pathLen, utf8Path, sizeof(utf8Path) - 1);
        utf8Path[utf8Len] = '\0';
    }
    else
    {
        utf8Path[0] = '.';
        utf8Path[1] = '\0';
    }

    SSIZE fd;
#if defined(ARCHITECTURE_X86_64) || defined(ARCHITECTURE_I386) || defined(ARCHITECTURE_ARMV7A)
    fd = System::Call(SYS_OPEN, (USIZE)utf8Path, O_RDONLY | O_DIRECTORY);
#else
    fd = System::Call(SYS_OPENAT, AT_FDCWD, (USIZE)utf8Path, O_RDONLY | O_DIRECTORY);
#endif

    if (fd >= 0)
    {
        handle = (PVOID)fd;
        first = TRUE;
    }
}

DirectoryIterator::DirectoryIterator(DirectoryIterator &&other) noexcept
    : handle(other.handle), currentEntry(other.currentEntry), first(other.first), nread(other.nread), bpos(other.bpos)
{
    Memory::Copy(buffer, other.buffer, sizeof(buffer));
    other.handle = (PVOID)INVALID_FD;
}

DirectoryIterator &DirectoryIterator::operator=(DirectoryIterator &&other) noexcept
{
    if (this != &other)
    {
        if (IsValid())
            System::Call(SYS_CLOSE, (USIZE)handle);
        handle = other.handle;
        currentEntry = other.currentEntry;
        first = other.first;
        nread = other.nread;
        bpos = other.bpos;
        Memory::Copy(buffer, other.buffer, sizeof(buffer));
        other.handle = (PVOID)INVALID_FD;
    }
    return *this;
}

DirectoryIterator::~DirectoryIterator()
{
    if (IsValid())
    {
        System::Call(SYS_CLOSE, (USIZE)handle);
        handle = (PVOID)INVALID_FD;
    }
}

BOOL DirectoryIterator::Next()
{
    if (!IsValid())
        return FALSE;

    if (first || bpos >= nread)
    {
        first = FALSE;
        nread = (INT32)System::Call(SYS_GETDENTS64, (USIZE)handle, (USIZE)buffer, sizeof(buffer));

        if (nread <= 0)
            return FALSE;
        bpos = 0;
    }

    linux_dirent64* d = (linux_dirent64*)(buffer + bpos);

    String::Utf8ToWide(d->d_name, currentEntry.name, 256);

    currentEntry.isDirectory = (d->d_type == DT_DIR);
    currentEntry.isDrive = FALSE;
    currentEntry.type = (UINT32)d->d_type;
    currentEntry.isHidden = (d->d_name[0] == '.');
    currentEntry.isSystem = FALSE;
    currentEntry.isReadOnly = FALSE;
    currentEntry.size = 0;
    currentEntry.creationTime = 0;

    bpos += d->d_reclen;

    return TRUE;
}

BOOL DirectoryIterator::IsValid() const
{
    return (SSIZE)handle >= 0;
}
