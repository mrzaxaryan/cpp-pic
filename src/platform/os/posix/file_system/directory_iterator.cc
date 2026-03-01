#include "platform/io/file_system/directory_iterator.h"
#include "platform/os/posix/file_system/posix_path.h"
#include "core/memory/memory.h"
#if defined(PLATFORM_LINUX)
#include "platform/os/linux/common/syscall.h"
#include "platform/os/linux/common/system.h"
#elif defined(PLATFORM_MACOS)
#include "platform/os/macos/common/syscall.h"
#include "platform/os/macos/common/system.h"
#elif defined(PLATFORM_SOLARIS)
#include "platform/os/solaris/common/syscall.h"
#include "platform/os/solaris/common/system.h"
#endif
#include "core/string/string.h"

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

	if (fd < 0)
	{
		return Result<DirectoryIterator, Error>::Err(Error::Fs_OpenFailed);
	}

	iter.handle = (PVOID)fd;
	iter.first = true;
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
#if defined(PLATFORM_LINUX) || defined(PLATFORM_SOLARIS)
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
	LinuxDirent64 *d = (LinuxDirent64 *)(buffer + bpos);
#elif defined(PLATFORM_SOLARIS)
	SolarisDirent64 *d = (SolarisDirent64 *)(buffer + bpos);
#elif defined(PLATFORM_MACOS)
	BsdDirent64 *d = (BsdDirent64 *)(buffer + bpos);
#endif

	StringUtils::Utf8ToWide(Span<const CHAR>(d->Name, StringUtils::Length(d->Name)), Span<WCHAR>(currentEntry.Name, 256));

#if defined(PLATFORM_SOLARIS)
	currentEntry.IsDirectory = false;       // Solaris dirent64 has no Type; cannot determine without stat
	currentEntry.Type = 0;                  // DT_UNKNOWN
#else
	currentEntry.IsDirectory = (d->Type == DT_DIR);
	currentEntry.Type = (UINT32)d->Type;
#endif
	currentEntry.IsDrive = false;
	currentEntry.IsHidden = (d->Name[0] == '.');
	currentEntry.IsSystem = false;
	currentEntry.IsReadOnly = false;
	currentEntry.Size = 0;
	currentEntry.CreationTime = 0;
	currentEntry.LastModifiedTime = 0;

	bpos += d->Reclen;

	return Result<void, Error>::Ok();
}

BOOL DirectoryIterator::IsValid() const
{
	return (SSIZE)handle >= 0;
}
