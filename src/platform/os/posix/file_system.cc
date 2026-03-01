#include "file_system.h"
#include "memory.h"
#include "syscall.h"
#include "system.h"
#include "string.h"
#include "utf16.h"
#include "path.h"

// =============================================================================
// Helper: Normalize a wide path to a null-terminated UTF-8 string.
// Centralises the normalise + convert sequence that every FileSystem
// method needs, avoiding 2 KB of stack per inlined copy.
// =============================================================================

static NOINLINE USIZE NormalizePathToUtf8(PCWCHAR path, Span<CHAR> utf8Out)
{
	WCHAR normalizedPath[1024];
	USIZE pathLen = Path::NormalizePath(path, Span<WCHAR>(normalizedPath));
	USIZE utf8Len = UTF16::ToUTF8(Span<const WCHAR>(normalizedPath, pathLen),
								   Span<CHAR>(utf8Out.Data(), utf8Out.Size() - 1));
	utf8Out.Data()[utf8Len] = '\0';
	return utf8Len;
}

// =============================================================================
// File Implementation
// =============================================================================

// --- Internal Constructor (trivial — never fails) ---
File::File(PVOID handle, USIZE size) : fileHandle(handle), fileSize(size) {}

File::File(File &&other) noexcept : fileHandle(nullptr), fileSize(0)
{
	fileHandle = other.fileHandle;
	fileSize = other.fileSize;
	other.fileHandle = (PVOID)INVALID_FD;
	other.fileSize = 0;
}

File &File::operator=(File &&other) noexcept
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

Result<UINT32, Error> File::Read(Span<UINT8> buffer)
{
	if (!IsValid())
		return Result<UINT32, Error>::Err(Error::Fs_ReadFailed);

	SSIZE result = System::Call(SYS_READ, (USIZE)fileHandle, (USIZE)buffer.Data(), buffer.Size());
	if (result >= 0)
		return Result<UINT32, Error>::Ok((UINT32)result);
	return Result<UINT32, Error>::Err(Error::Posix((UINT32)(-result)), Error::Fs_ReadFailed);
}

Result<UINT32, Error> File::Write(Span<const UINT8> buffer)
{
	if (!IsValid())
		return Result<UINT32, Error>::Err(Error::Fs_WriteFailed);

	SSIZE result = System::Call(SYS_WRITE, (USIZE)fileHandle, (USIZE)buffer.Data(), buffer.Size());
	if (result >= 0)
		return Result<UINT32, Error>::Ok((UINT32)result);
	return Result<UINT32, Error>::Err(Error::Posix((UINT32)(-result)), Error::Fs_WriteFailed);
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

// =============================================================================
// FileSystem Implementation
// =============================================================================

Result<File, Error> FileSystem::Open(PCWCHAR path, INT32 flags)
{
	CHAR utf8Path[1024];
	NormalizePathToUtf8(path, Span<CHAR>(utf8Path));

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
#if defined(PLATFORM_LINUX) && defined(ARCHITECTURE_AARCH64)
	fd = System::Call(SYS_OPENAT, AT_FDCWD, (USIZE)utf8Path, openFlags, mode);
#else
	fd = System::Call(SYS_OPEN, (USIZE)utf8Path, openFlags, mode);
#endif

	if (fd < 0)
		return Result<File, Error>::Err(Error::Posix((UINT32)(-fd)), Error::Fs_OpenFailed);

	return Result<File, Error>::Ok(File((PVOID)fd, 0));
}

Result<void, Error> FileSystem::Delete(PCWCHAR path)
{
	CHAR utf8Path[1024];
	NormalizePathToUtf8(path, Span<CHAR>(utf8Path));

#if defined(PLATFORM_LINUX) && defined(ARCHITECTURE_AARCH64)
	SSIZE result = System::Call(SYS_UNLINKAT, AT_FDCWD, (USIZE)utf8Path, 0);
#else
	SSIZE result = System::Call(SYS_UNLINK, (USIZE)utf8Path);
#endif
	if (result == 0)
		return Result<void, Error>::Ok();
	return Result<void, Error>::Err(Error::Posix((UINT32)(-result)), Error::Fs_DeleteFailed);
}

Result<void, Error> FileSystem::Exists(PCWCHAR path)
{
	CHAR utf8Path[1024];
	NormalizePathToUtf8(path, Span<CHAR>(utf8Path));

	UINT8 statbuf[144];

#if defined(PLATFORM_LINUX) && defined(ARCHITECTURE_AARCH64)
	SSIZE result = System::Call(SYS_FSTATAT, AT_FDCWD, (USIZE)utf8Path, (USIZE)statbuf, 0);
#elif defined(PLATFORM_MACOS)
	SSIZE result = System::Call(SYS_STAT64, (USIZE)utf8Path, (USIZE)statbuf);
#else
	SSIZE result = System::Call(SYS_STAT, (USIZE)utf8Path, (USIZE)statbuf);
#endif
	if (result == 0)
		return Result<void, Error>::Ok();
	return Result<void, Error>::Err(Error::Posix((UINT32)(-result)), Error::Fs_OpenFailed);
}

Result<void, Error> FileSystem::CreateDirectory(PCWCHAR path)
{
	CHAR utf8Path[1024];
	NormalizePathToUtf8(path, Span<CHAR>(utf8Path));

	// Mode 0755 (rwxr-xr-x)
	INT32 mode = S_IRUSR | S_IWUSR | S_IXUSR | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH;

#if defined(PLATFORM_LINUX) && defined(ARCHITECTURE_AARCH64)
	SSIZE result = System::Call(SYS_MKDIRAT, AT_FDCWD, (USIZE)utf8Path, mode);
#else
	SSIZE result = System::Call(SYS_MKDIR, (USIZE)utf8Path, mode);
#endif
	if (result == 0 || result == -17) // -EEXIST: directory already exists
		return Result<void, Error>::Ok();
	return Result<void, Error>::Err(Error::Posix((UINT32)(-result)), Error::Fs_CreateDirFailed);
}

Result<void, Error> FileSystem::DeleteDirectory(PCWCHAR path)
{
	CHAR utf8Path[1024];
	NormalizePathToUtf8(path, Span<CHAR>(utf8Path));

#if defined(PLATFORM_LINUX) && defined(ARCHITECTURE_AARCH64)
	SSIZE result = System::Call(SYS_UNLINKAT, AT_FDCWD, (USIZE)utf8Path, AT_REMOVEDIR);
#else
	SSIZE result = System::Call(SYS_RMDIR, (USIZE)utf8Path);
#endif
	if (result == 0)
		return Result<void, Error>::Ok();
	return Result<void, Error>::Err(Error::Posix((UINT32)(-result)), Error::Fs_DeleteDirFailed);
}

// =============================================================================
// DirectoryIterator Implementation
// =============================================================================

DirectoryIterator::DirectoryIterator()
	: handle((PVOID)INVALID_FD), first(false), nread(0), bpos(0)
{}

Result<DirectoryIterator, Error> DirectoryIterator::Create(PCWCHAR path)
{
	DirectoryIterator iter;
	CHAR utf8Path[1024];

	if (path && path[0] != L'\0')
	{
		NormalizePathToUtf8(path, Span<CHAR>(utf8Path));
	}
	else
	{
		utf8Path[0] = '.';
		utf8Path[1] = '\0';
	}

	SSIZE fd;
#if defined(PLATFORM_LINUX) && defined(ARCHITECTURE_AARCH64)
	fd = System::Call(SYS_OPENAT, AT_FDCWD, (USIZE)utf8Path, O_RDONLY | O_DIRECTORY);
#else
	fd = System::Call(SYS_OPEN, (USIZE)utf8Path, O_RDONLY | O_DIRECTORY);
#endif

	if (fd >= 0)
	{
		iter.handle = (PVOID)fd;
		iter.first = true;
	}
	return Result<DirectoryIterator, Error>::Ok(static_cast<DirectoryIterator &&>(iter));
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

Result<void, Error> DirectoryIterator::Next()
{
	if (!IsValid())
		return Result<void, Error>::Err(Error::Fs_ReadFailed);

	if (first || bpos >= nread)
	{
		first = false;
#if defined(PLATFORM_LINUX)
		nread = (INT32)System::Call(SYS_GETDENTS64, (USIZE)handle, (USIZE)buffer, sizeof(buffer));
#elif defined(PLATFORM_MACOS)
		USIZE basep = 0;
		nread = (INT32)System::Call(SYS_GETDIRENTRIES64, (USIZE)handle, (USIZE)buffer, sizeof(buffer), (USIZE)&basep);
#endif

		if (nread < 0)
			return Result<void, Error>::Err(Error::Posix((UINT32)(-nread)), Error::Fs_ReadFailed);
		if (nread == 0)
			return Result<void, Error>::Err(Error::Fs_ReadFailed);
		bpos = 0;
	}

#if defined(PLATFORM_LINUX)
	linux_dirent64 *d = (linux_dirent64 *)(buffer + bpos);
#elif defined(PLATFORM_MACOS)
	bsd_dirent64 *d = (bsd_dirent64 *)(buffer + bpos);
#endif

	String::Utf8ToWide(d->d_name, Span<WCHAR>(currentEntry.name, 256));

	currentEntry.isDirectory = (d->d_type == DT_DIR);
	currentEntry.isDrive = false;
	currentEntry.type = (UINT32)d->d_type;
	currentEntry.isHidden = (d->d_name[0] == '.');
	currentEntry.isSystem = false;
	currentEntry.isReadOnly = false;
	currentEntry.size = 0;
	currentEntry.creationTime = 0;
	currentEntry.lastModifiedTime = 0;

	bpos += d->d_reclen;

	return Result<void, Error>::Ok();
}

BOOL DirectoryIterator::IsValid() const
{
	return (SSIZE)handle >= 0;
}
