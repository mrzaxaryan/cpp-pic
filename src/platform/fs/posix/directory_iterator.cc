#include "platform/fs/directory_iterator.h"
#include "platform/fs/posix/posix_path.h"
#include "core/memory/memory.h"
#if defined(PLATFORM_LINUX)
#include "platform/common/linux/syscall.h"
#include "platform/common/linux/system.h"
#elif defined(PLATFORM_MACOS)
#include "platform/common/macos/syscall.h"
#include "platform/common/macos/system.h"
#elif defined(PLATFORM_SOLARIS)
#include "platform/common/solaris/syscall.h"
#include "platform/common/solaris/system.h"
#elif defined(PLATFORM_FREEBSD)
#include "platform/common/freebsd/syscall.h"
#include "platform/common/freebsd/system.h"
#endif
#include "core/string/string.h"

// =============================================================================
// DirectoryIterator Implementation
// =============================================================================

DirectoryIterator::DirectoryIterator()
	: handle((PVOID)INVALID_FD), currentEntry{}, isFirst(false), bytesRead(0), bufferPosition(0)
{
	Memory::Zero(buffer, sizeof(buffer));
}

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
#if (defined(PLATFORM_LINUX) || defined(PLATFORM_FREEBSD)) && (defined(ARCHITECTURE_AARCH64) || defined(ARCHITECTURE_RISCV64) || defined(ARCHITECTURE_RISCV32))
	// RISC-V: omit O_DIRECTORY — QEMU user-mode does not translate the
	// asm-generic O_DIRECTORY (0x4000) to the host value, so the flag is
	// mis-interpreted as O_DIRECT on x86_64 hosts.  Safety is preserved
	// because getdents64 returns ENOTDIR on non-directory fds.
	INT32 openFlags = O_RDONLY;
#if defined(ARCHITECTURE_AARCH64)
	openFlags |= O_DIRECTORY;
#endif
	fd = System::Call(SYS_OPENAT, AT_FDCWD, (USIZE)utf8Path, openFlags, 0);
#elif defined(PLATFORM_SOLARIS)
	fd = System::Call(SYS_OPENAT, AT_FDCWD, (USIZE)utf8Path, (USIZE)(O_RDONLY | O_DIRECTORY), (USIZE)0);
#else
	fd = System::Call(SYS_OPEN, (USIZE)utf8Path, O_RDONLY | O_DIRECTORY);
#endif

	if (fd < 0)
	{
		return Result<DirectoryIterator, Error>::Err(Error::Posix((UINT32)(-fd)), Error::Fs_OpenFailed);
	}

	iter.handle = (PVOID)fd;
	iter.isFirst = true;
	return Result<DirectoryIterator, Error>::Ok(static_cast<DirectoryIterator &&>(iter));
}

DirectoryIterator::DirectoryIterator(DirectoryIterator &&other) noexcept
	: handle(other.handle), currentEntry(other.currentEntry), isFirst(other.isFirst), bytesRead(other.bytesRead), bufferPosition(other.bufferPosition)
{
	Memory::Copy(buffer, other.buffer, sizeof(buffer));
	other.handle = (PVOID)INVALID_FD;
}

DirectoryIterator &DirectoryIterator::operator=(DirectoryIterator &&other) noexcept
{
	if (this != &other)
	{
		if (IsValid())
			Close();
		handle = other.handle;
		currentEntry = other.currentEntry;
		isFirst = other.isFirst;
		bytesRead = other.bytesRead;
		bufferPosition = other.bufferPosition;
		Memory::Copy(buffer, other.buffer, sizeof(buffer));
		other.handle = (PVOID)INVALID_FD;
	}
	return *this;
}

VOID DirectoryIterator::Close()
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

	if (isFirst || bufferPosition >= bytesRead)
	{
		isFirst = false;
#if defined(PLATFORM_SOLARIS) && (defined(ARCHITECTURE_X86_64) || defined(ARCHITECTURE_AARCH64))
		// LP64 Solaris: getdents64 triggers SIGSYS for 64-bit processes.
		// Use getdents (81) which natively returns 64-bit dirent on LP64.
		bytesRead = (INT32)System::Call(SYS_GETDENTS, (USIZE)handle, (USIZE)buffer, sizeof(buffer));
#elif defined(PLATFORM_LINUX) || defined(PLATFORM_SOLARIS)
		bytesRead = (INT32)System::Call(SYS_GETDENTS64, (USIZE)handle, (USIZE)buffer, sizeof(buffer));
#elif defined(PLATFORM_MACOS)
		USIZE basep = 0;
		bytesRead = (INT32)System::Call(SYS_GETDIRENTRIES64, (USIZE)handle, (USIZE)buffer, sizeof(buffer), (USIZE)&basep);
#elif defined(PLATFORM_FREEBSD)
		USIZE basep = 0;
		bytesRead = (INT32)System::Call(SYS_GETDIRENTRIES, (USIZE)handle, (USIZE)buffer, sizeof(buffer), (USIZE)&basep);
#endif

		if (bytesRead < 0)
			return Result<void, Error>::Err(Error::Posix((UINT32)(-bytesRead)), Error::Fs_ReadFailed);
		if (bytesRead == 0)
			return Result<void, Error>::Err(Error::Fs_ReadFailed);
		bufferPosition = 0;
	}

#if defined(PLATFORM_LINUX)
	LinuxDirent64 *d = (LinuxDirent64 *)(buffer + bufferPosition);
#elif defined(PLATFORM_SOLARIS)
	SolarisDirent64 *d = (SolarisDirent64 *)(buffer + bufferPosition);
#elif defined(PLATFORM_MACOS)
	BsdDirent64 *d = (BsdDirent64 *)(buffer + bufferPosition);
#elif defined(PLATFORM_FREEBSD)
	FreeBsdDirent *d = (FreeBsdDirent *)(buffer + bufferPosition);
#endif

	StringUtils::Utf8ToWide(Span<const CHAR>(d->Name, StringUtils::Length(d->Name)), Span<WCHAR>(currentEntry.Name, 256));

#if defined(PLATFORM_SOLARIS)
	// Solaris dirent has no Type field; determine IsDirectory via fstatat.
	{
		UINT8 statbuf[144];
		SSIZE statResult = System::Call(SYS_FSTATAT, (USIZE)handle, (USIZE)d->Name, (USIZE)statbuf, 0);
		if (statResult == 0)
		{
			// st_mode offset: ILP32 = 20 (dev4+pad12+ino4), LP64 = 16 (dev8+ino8)
#if defined(ARCHITECTURE_I386)
			constexpr USIZE MODE_OFFSET = 20;
#else
			constexpr USIZE MODE_OFFSET = 16;
#endif
			UINT32 mode = *(UINT32 *)(statbuf + MODE_OFFSET);
			currentEntry.IsDirectory = ((mode & 0xF000) == 0x4000); // S_IFDIR
		}
		else
		{
			currentEntry.IsDirectory = false;
		}
	}
	currentEntry.Type = currentEntry.IsDirectory ? 4 : 0; // DT_DIR=4 or DT_UNKNOWN=0
#else // Linux, macOS, FreeBSD — dirent has Type field
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

	bufferPosition += d->Reclen;

	return Result<void, Error>::Ok();
}

BOOL DirectoryIterator::IsValid() const
{
	return (SSIZE)handle >= 0;
}
