#include "allocator.h"
#include "ntdll.h"
#include "peb.h"

PVOID Allocator::AllocateMemory(USIZE len)
{
    return NTDLL::RtlAllocateHeap(GetCurrentPEB()->ProcessHeap, 0, len);
}

VOID Allocator::ReleaseMemory(PVOID ptr, USIZE)
{
    NTDLL::RtlFreeHeap(GetCurrentPEB()->ProcessHeap, 0, ptr);
}