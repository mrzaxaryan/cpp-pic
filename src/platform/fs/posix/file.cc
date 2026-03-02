#include "platform/fs/file.h"
#include "platform/fs/posix/posix_path.h"
#include "core/memory/memory.h"
#include "core/string/string.h"
#if defined(PLATFORM_LINUX)
#include "platform/common/linux/syscall.h"
#include "platform/common/linux/system.h"
#elif defined(PLATFORM_MACOS)
#include "platform/common/macos/syscall.h"
#include "platform/common/macos/system.h"
#elif defined(PLATFORM_SOLARIS)
#include "platform/common/solaris/syscall.h"
#include "platform/common/solaris/system.h"
#endif

// --- Internal Constructor (trivial — never fails) ---
File::File(PVOID handle, USIZE size) : fileHandle(handle), fileSize(size) {}

// --- Factory & Static Operations ---
Result<File, Error> File::Open(PCWCHAR path, INT32 flags)
{
	CHAR utf8Path[1024];
	NormalizePathToUtf8(path, Span<CHAR>(utf8Path));

	INT32 openFlags = 0;
	INT32 mode = S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH;

	// Access mode
	if ((flags & File::ModeRead) && (flags & File::ModeWrite))
		openFlags |= O_RDWR;
	else if (flags & File::ModeWrite)
		openFlags |= O_WRONLY;
	else
		openFlags |= O_RDONLY;

	// Creation/truncation flags
	if (flags & File::ModeCreate)
		openFlags |= O_CREAT;
	if (flags & File::ModeTruncate)
		openFlags |= O_TRUNC;
	if (flags & File::ModeAppend)
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

Result<void, Error> File::Delete(PCWCHAR path)
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

Result<void, Error> File::Exists(PCWCHAR path)
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

Result<USIZE, Error> File::GetOffset() const
{
	if (!IsValid())
		return Result<USIZE, Error>::Err(Error::Fs_SeekFailed);

	SSIZE result = System::Call(SYS_LSEEK, (USIZE)fileHandle, 0, SEEK_CUR);
	if (result >= 0)
		return Result<USIZE, Error>::Ok((USIZE)result);
	return Result<USIZE, Error>::Err(Error::Posix((UINT32)(-result)), Error::Fs_SeekFailed);
}

Result<void, Error> File::SetOffset(USIZE absoluteOffset)
{
	if (!IsValid())
		return Result<void, Error>::Err(Error::Fs_SeekFailed);

	SSIZE result = System::Call(SYS_LSEEK, (USIZE)fileHandle, absoluteOffset, SEEK_SET);
	if (result >= 0)
		return Result<void, Error>::Ok();
	return Result<void, Error>::Err(Error::Posix((UINT32)(-result)), Error::Fs_SeekFailed);
}

Result<void, Error> File::MoveOffset(SSIZE relativeAmount, OffsetOrigin origin)
{
	if (!IsValid())
		return Result<void, Error>::Err(Error::Fs_SeekFailed);

	INT32 whence = SEEK_CUR;
	if (origin == OffsetOrigin::Start)
		whence = SEEK_SET;
	else if (origin == OffsetOrigin::End)
		whence = SEEK_END;

	SSIZE result = System::Call(SYS_LSEEK, (USIZE)fileHandle, (USIZE)relativeAmount, whence);
	if (result >= 0)
		return Result<void, Error>::Ok();
	return Result<void, Error>::Err(Error::Posix((UINT32)(-result)), Error::Fs_SeekFailed);
}
