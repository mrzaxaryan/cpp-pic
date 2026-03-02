#include "platform/memory/allocator.h"
#include "platform/common/windows/ntdll.h"
#include "platform/common/windows/windows_types.h"

PVOID Allocator::AllocateMemory(USIZE len)
{
	PVOID base = nullptr;
	USIZE size = len;
	auto result = NTDLL::ZwAllocateVirtualMemory(NTDLL::NtCurrentProcess(), &base, 0, &size, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
	return result ? base : nullptr;
}

VOID Allocator::ReleaseMemory(PVOID ptr, USIZE)
{
	USIZE size = 0;
	(void)NTDLL::ZwFreeVirtualMemory(NTDLL::NtCurrentProcess(), &ptr, &size, MEM_RELEASE);
}