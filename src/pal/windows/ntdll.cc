#include "ntdll.h"

#define ResolveNtdllExportAddress(functionName) ResolveExportAddressFromPebModule(Djb2::HashCompileTime(L"ntdll.dll"), Djb2::HashCompileTime(functionName))

// =============================================================================
// Memory Management
// =============================================================================

PVOID NTDLL::RtlAllocateHeap(PVOID HeapHandle, INT32 Flags, USIZE Size)
{
	return ((PVOID(STDCALL *)(PVOID, INT32, USIZE))ResolveNtdllExportAddress("RtlAllocateHeap"))(HeapHandle, Flags, Size);
}

BOOL NTDLL::RtlFreeHeap(PVOID HeapHandle, INT32 Flags, PVOID Pointer)
{
	return ((BOOL(STDCALL *)(PVOID, INT32, PVOID))ResolveNtdllExportAddress("RtlFreeHeap"))(HeapHandle, Flags, Pointer);
}

// =============================================================================
// Process Management
// =============================================================================

NTSTATUS NTDLL::ZwTerminateProcess(PVOID ProcessHandle, NTSTATUS ExitStatus)
{
	return ((NTSTATUS(STDCALL *)(PVOID, NTSTATUS))ResolveNtdllExportAddress("ZwTerminateProcess"))(ProcessHandle, ExitStatus);
}

// =============================================================================
// File/Device I/O
// =============================================================================

NTSTATUS NTDLL::NtCreateFile(PVOID *FileHandle, UINT32 DesiredAccess, POBJECT_ATTRIBUTES ObjectAttributes, PIO_STATUS_BLOCK IoStatusBlock, PLARGE_INTEGER AllocationSize, UINT32 FileAttributes, UINT32 ShareAccess, UINT32 CreateDisposition, UINT32 CreateOptions, PVOID EaBuffer, UINT32 EaLength)
{
	return ((NTSTATUS(STDCALL *)(PVOID *, UINT32, POBJECT_ATTRIBUTES, PIO_STATUS_BLOCK, PLARGE_INTEGER, UINT32, UINT32, UINT32, UINT32, PVOID, UINT32))ResolveNtdllExportAddress("NtCreateFile"))(FileHandle, DesiredAccess, ObjectAttributes, IoStatusBlock, AllocationSize, FileAttributes, ShareAccess, CreateDisposition, CreateOptions, EaBuffer, EaLength);
}

NTSTATUS NTDLL::NtDeviceIoControlFile(PVOID FileHandle, PVOID Event, PVOID ApcRoutine, PVOID ApcContext, PIO_STATUS_BLOCK IoStatusBlock, UINT32 IoControlCode, PVOID InputBuffer, UINT32 InputBufferLength, PVOID OutputBuffer, UINT32 OutputBufferLength)
{
	return ((NTSTATUS(STDCALL *)(PVOID, PVOID, PVOID, PVOID, PIO_STATUS_BLOCK, UINT32, PVOID, UINT32, PVOID, UINT32))ResolveNtdllExportAddress("NtDeviceIoControlFile"))(FileHandle, Event, ApcRoutine, ApcContext, IoStatusBlock, IoControlCode, InputBuffer, InputBufferLength, OutputBuffer, OutputBufferLength);
}

NTSTATUS NTDLL::NtClose(PVOID Handle)
{
	return ((NTSTATUS(STDCALL *)(PVOID))ResolveNtdllExportAddress("NtClose"))(Handle);
}

// =============================================================================
// Synchronization
// =============================================================================

NTSTATUS NTDLL::NtCreateEvent(PVOID *EventHandle, UINT32 DesiredAccess, POBJECT_ATTRIBUTES ObjectAttributes, EVENT_TYPE EventType, BOOL InitialState)
{
	return ((NTSTATUS(STDCALL *)(PVOID *, UINT32, POBJECT_ATTRIBUTES, EVENT_TYPE, BOOL))ResolveNtdllExportAddress("NtCreateEvent"))(EventHandle, DesiredAccess, ObjectAttributes, EventType, InitialState);
}

NTSTATUS NTDLL::NtWaitForSingleObject(PVOID Handle, BOOL Alertable, PLARGE_INTEGER Timeout)
{
	return ((NTSTATUS(STDCALL *)(PVOID, BOOL, PLARGE_INTEGER))ResolveNtdllExportAddress("NtWaitForSingleObject"))(Handle, Alertable, Timeout);
}
