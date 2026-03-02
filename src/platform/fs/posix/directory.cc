#include "platform/fs/directory.h"
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

// =============================================================================
// Directory Implementation
// =============================================================================

Result<void, Error> Directory::Create(PCWCHAR path)
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

Result<void, Error> Directory::Delete(PCWCHAR path)
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
