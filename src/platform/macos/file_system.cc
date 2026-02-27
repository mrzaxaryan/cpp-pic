#include "file_system.h"
#include "memory.h"
#include "syscall.h"
#include "system.h"
#include "string.h"
#include "utf16.h"

// --- File Implementation ---

File::File(PVOID handle) : fileHandle(handle), fileSize(0)
{
}

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

Result<UINT32, Error> File::Read(PVOID buffer, UINT32 size)
{
	if (!IsValid())
		return Result<UINT32, Error>::Err(Error::Fs_ReadFailed);

	SSIZE result = System::Call(SYS_READ, (USIZE)fileHandle, (USIZE)buffer, size);
	if (result >= 0)
		return Result<UINT32, Error>::Ok((UINT32)result);
	return Result<UINT32, Error>::Err(Error::Posix((UINT32)(-result)), Error::Fs_ReadFailed);
}

Result<UINT32, Error> File::Write(const VOID *buffer, USIZE size)
{
	if (!IsValid())
		return Result<UINT32, Error>::Err(Error::Fs_WriteFailed);

	SSIZE result = System::Call(SYS_WRITE, (USIZE)fileHandle, (USIZE)buffer, size);
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

	SSIZE fd = System::Call(SYS_OPEN, (USIZE)utf8Path, openFlags, mode);

	if (fd < 0)
		return File();

	return File((PVOID)fd);
}

Result<void, Error> FileSystem::Delete(PCWCHAR path)
{
	CHAR utf8Path[1024];
	USIZE pathLen = String::Length(path);
	USIZE utf8Len = UTF16::ToUTF8(path, pathLen, utf8Path, sizeof(utf8Path) - 1);
	utf8Path[utf8Len] = '\0';

	SSIZE result = System::Call(SYS_UNLINK, (USIZE)utf8Path);
	if (result == 0)
		return Result<void, Error>::Ok();
	return Result<void, Error>::Err(Error::Posix((UINT32)(-result)), Error::Fs_DeleteFailed);
}

BOOL FileSystem::Exists(PCWCHAR path)
{
	CHAR utf8Path[1024];
	USIZE pathLen = String::Length(path);
	USIZE utf8Len = UTF16::ToUTF8(path, pathLen, utf8Path, sizeof(utf8Path) - 1);
	utf8Path[utf8Len] = '\0';

	UINT8 statbuf[144];
	return System::Call(SYS_STAT64, (USIZE)utf8Path, (USIZE)statbuf) == 0;
}

Result<void, Error> FileSystem::CreateDirectory(PCWCHAR path)
{
	CHAR utf8Path[1024];
	USIZE pathLen = String::Length(path);
	USIZE utf8Len = UTF16::ToUTF8(path, pathLen, utf8Path, sizeof(utf8Path) - 1);
	utf8Path[utf8Len] = '\0';

	// Mode 0755 (rwxr-xr-x)
	INT32 mode = S_IRUSR | S_IWUSR | S_IXUSR | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH;
	SSIZE result = System::Call(SYS_MKDIR, (USIZE)utf8Path, mode);
	if (result == 0 || result == -17) // -EEXIST: directory already exists
		return Result<void, Error>::Ok();
	return Result<void, Error>::Err(Error::Posix((UINT32)(-result)), Error::Fs_CreateDirFailed);
}

Result<void, Error> FileSystem::DeleteDirectory(PCWCHAR path)
{
	CHAR utf8Path[1024];
	USIZE pathLen = String::Length(path);
	USIZE utf8Len = UTF16::ToUTF8(path, pathLen, utf8Path, sizeof(utf8Path) - 1);
	utf8Path[utf8Len] = '\0';

	SSIZE result = System::Call(SYS_RMDIR, (USIZE)utf8Path);
	if (result == 0)
		return Result<void, Error>::Ok();
	return Result<void, Error>::Err(Error::Posix((UINT32)(-result)), Error::Fs_DeleteDirFailed);
}

// --- DirectoryIterator Implementation ---

DirectoryIterator::DirectoryIterator()
	: handle((PVOID)INVALID_FD), first(false), nread(0), bpos(0)
{}

Result<void, Error> DirectoryIterator::Initialization(PCWCHAR path)
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

	SSIZE fd = System::Call(SYS_OPEN, (USIZE)utf8Path, O_RDONLY | O_DIRECTORY);

	if (fd >= 0)
	{
		handle = (PVOID)fd;
		first = true;
	}
	return Result<void, Error>::Ok();
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
		return false;

	if (first || bpos >= nread)
	{
		first = false;
		// macOS getdirentries64: fd, buf, bufsize, basep
		USIZE basep = 0;
		nread = (INT32)System::Call(SYS_GETDIRENTRIES64, (USIZE)handle, (USIZE)buffer, sizeof(buffer), (USIZE)&basep);

		if (nread <= 0)
			return false;
		bpos = 0;
	}

	bsd_dirent64 *d = (bsd_dirent64 *)(buffer + bpos);

	String::Utf8ToWide(d->d_name, currentEntry.name, 256);

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

	return true;
}

BOOL DirectoryIterator::IsValid() const
{
	return (SSIZE)handle >= 0;
}
