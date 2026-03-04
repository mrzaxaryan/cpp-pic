#include "platform/common/windows/kernel32.h"
#include "platform/platform.h"
#include "platform/common/windows/peb.h"

#define ResolveKernel32ExportAddress(functionName) ResolveExportAddressFromPebModule(Djb2::HashCompileTime(L"kernel32.dll"), Djb2::HashCompileTime(functionName))

// CreateProcessW
Result<void, Error> Kernel32::CreateProcessW(PWCHAR lpApplicationName, PWCHAR lpCommandLine, PVOID lpProcessAttributes, PVOID lpThreadAttributes, BOOL bInheritHandles, UINT32 dwCreationFlags, PVOID lpEnvironment, PWCHAR lpCurrentDirectory, LPSTARTUPINFOW lpStartupInfo, LPPROCESS_INFORMATION lpProcessInformation)
{
	BOOL result = ((BOOL(STDCALL *)(PWCHAR lpApplicationName, PWCHAR lpCommandLine, PVOID lpProcessAttributes, PVOID lpThreadAttributes, BOOL bInheritHandles, UINT32 dwCreationFlags, PVOID lpEnvironment, PWCHAR lpCurrentDirectory, LPSTARTUPINFOW lpStartupInfo, LPPROCESS_INFORMATION lpProcessInformation))ResolveKernel32ExportAddress("CreateProcessW"))(lpApplicationName, lpCommandLine, lpProcessAttributes, lpThreadAttributes, bInheritHandles, dwCreationFlags, lpEnvironment, lpCurrentDirectory, lpStartupInfo, lpProcessInformation);
	if (!result)
	{
		return Result<void, Error>::Err(Error(Error::Kernel32_CreateProcessFailed));
	}
	return Result<void, Error>::Ok();
}

// SetHandleInformation
Result<void, Error> Kernel32::SetHandleInformation(PVOID hObject, UINT32 dwMask, UINT32 dwFlags)
{
	BOOL result = ((BOOL(STDCALL *)(PVOID hObject, UINT32 dwMask, UINT32 dwFlags))ResolveKernel32ExportAddress("SetHandleInformation"))(hObject, dwMask, dwFlags);
	if (!result)
	{
		return Result<void, Error>::Err(Error(Error::Kernel32_SetHandleInfoFailed));
	}
	return Result<void, Error>::Ok();
}