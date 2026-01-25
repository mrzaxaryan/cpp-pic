#if defined(PLATFORM_WINDOWS)
#pragma once

#include "windows_types.h"
#include "afd.h"
#include "pal.h"
#include "djb2.h"

// NTDLL API Wrappers
class NTDLL
{
private:
public:
	// Memory Management
	static PVOID RtlAllocateHeap(PVOID HeapHandle, INT32 Flags, USIZE Size);
	static BOOL RtlFreeHeap(PVOID HeapHandle, INT32 Flags, PVOID Pointer);

	// Process Management
	static NTSTATUS ZwTerminateProcess(PVOID ProcessHandle, NTSTATUS ExitStatus);
	static PVOID NtCurrentProcess() { return (PVOID)(USIZE)-1L; }
	static PVOID NtCurrentThread() { return (PVOID)(USIZE)-2L; }

	// File/Device I/O
	static NTSTATUS NtCreateFile(PVOID *FileHandle, UINT32 DesiredAccess, POBJECT_ATTRIBUTES ObjectAttributes, PIO_STATUS_BLOCK IoStatusBlock, PLARGE_INTEGER AllocationSize, UINT32 FileAttributes, UINT32 ShareAccess, UINT32 CreateDisposition, UINT32 CreateOptions, PVOID EaBuffer, UINT32 EaLength);
	static NTSTATUS NtDeviceIoControlFile(PVOID FileHandle, PVOID Event, PVOID ApcRoutine, PVOID ApcContext, PIO_STATUS_BLOCK IoStatusBlock, UINT32 IoControlCode, PVOID InputBuffer, UINT32 InputBufferLength, PVOID OutputBuffer, UINT32 OutputBufferLength);
	static NTSTATUS NtClose(PVOID Handle);

	// Synchronization
	static NTSTATUS NtCreateEvent(PVOID *EventHandle, UINT32 DesiredAccess, POBJECT_ATTRIBUTES ObjectAttributes, EVENT_TYPE EventType, BOOL InitialState);
	static NTSTATUS NtWaitForSingleObject(PVOID Handle, BOOL Alertable, PLARGE_INTEGER Timeout);
};

#endif // PLATFORM_WINDOWS