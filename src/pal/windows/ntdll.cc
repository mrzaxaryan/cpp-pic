#include "ntdll.h"
#include "pal.h"
#include "peb.h"

#define ResolveNtdllExportAddress(functionName) ResolveExportAddressFromPebModule(Djb2::HashCompileTime(L"ntdll.dll"), Djb2::HashCompileTime(functionName))

NTSTATUS NTDLL::NtCreateEvent(PPVOID EventHandle, UINT32 DesiredAccess, POBJECT_ATTRIBUTES ObjectAttributes, EVENT_TYPE EventType, INT8 InitialState)
{
    return ((NTSTATUS(STDCALL *)(PPVOID EventHandle, UINT32 DesiredAccess, POBJECT_ATTRIBUTES ObjectAttributes, EVENT_TYPE EventType, INT8 InitialState))ResolveNtdllExportAddress("NtCreateEvent"))(EventHandle, DesiredAccess, ObjectAttributes, EventType, InitialState);
}
NTSTATUS NTDLL::NtDeviceIoControlFile(PVOID FileHandle, PVOID Event, PIO_APC_ROUTINE ApcRoutine, PVOID ApcContext, PIO_STATUS_BLOCK IoStatusBlock, UINT32 IoControlCode, PVOID InputBuffer, UINT32 InputBufferLength, PVOID OutputBuffer, UINT32 OutputBufferLength)
{
    return ((NTSTATUS(STDCALL *)(PVOID FileHandle, PVOID Event, PIO_APC_ROUTINE ApcRoutine, PVOID ApcContext, PIO_STATUS_BLOCK IoStatusBlock, UINT32 IoControlCode, PVOID InputBuffer, UINT32 InputBufferLength, PVOID OutputBuffer, UINT32 OutputBufferLength))ResolveNtdllExportAddress("NtDeviceIoControlFile"))(FileHandle, Event, ApcRoutine, ApcContext, IoStatusBlock, IoControlCode, InputBuffer, InputBufferLength, OutputBuffer, OutputBufferLength);
}

NTSTATUS NTDLL::NtWaitForSingleObject(PVOID Object, INT8 Alertable, PLARGE_INTEGER Timeout)
{
    return ((NTSTATUS(STDCALL *)(PVOID Object, INT8 Alertable, PLARGE_INTEGER Timeout))ResolveNtdllExportAddress("NtWaitForSingleObject"))(Object, Alertable, Timeout);
}

INT64 NTDLL::NtClose(PVOID Handle)
{
    return ((INT64(STDCALL *)(PVOID Handle))ResolveNtdllExportAddress("NtClose"))(Handle);
}

NTSTATUS NTDLL::NtCreateFile(PPVOID FileHandle, UINT32 DesiredAccess, PVOID ObjectAttributes, PIO_STATUS_BLOCK IoStatusBlock, PLARGE_INTEGER AllocationSize, UINT32 FileAttributes, UINT32 ShareAccess, UINT32 CreateDisposition, UINT32 CreateOptions, PVOID EaBuffer, UINT32 EaLength)
{
    return ((NTSTATUS(STDCALL *)(PPVOID FileHandle, UINT32 DesiredAccess, PVOID ObjectAttributes, PIO_STATUS_BLOCK IoStatusBlock, PLARGE_INTEGER AllocationSize, UINT32 FileAttributes, UINT32 ShareAccess, UINT32 CreateDisposition, UINT32 CreateOptions, PVOID EaBuffer, UINT32 EaLength))ResolveNtdllExportAddress("NtCreateFile"))(FileHandle, DesiredAccess, ObjectAttributes, IoStatusBlock, AllocationSize, FileAttributes, ShareAccess, CreateDisposition, CreateOptions, EaBuffer, EaLength);
}

PVOID NTDLL::RtlAllocateHeap(PVOID HeapHandle, INT32 Flags, USIZE Size)
{
    return ((PVOID(STDCALL *)(PVOID HeapHandle, INT32 Flags, USIZE Size))ResolveNtdllExportAddress("RtlAllocateHeap"))(HeapHandle, Flags, Size);
}

BOOL NTDLL::RtlFreeHeap(PVOID HeapHandle, INT32 Flags, PVOID Pointer)
{
    return ((BOOL(STDCALL *)(PVOID HeapHandle, INT32 Flags, PVOID Pointer))ResolveNtdllExportAddress("RtlFreeHeap"))(HeapHandle, Flags, Pointer);
}

NTSTATUS NTDLL::ZwTerminateProcess(PVOID ProcessHandle, NTSTATUS ExitStatus)
{
    return ((NTSTATUS(STDCALL *)(PVOID ProcessHandle, NTSTATUS ExitStatus))ResolveNtdllExportAddress("ZwTerminateProcess"))(ProcessHandle, ExitStatus);
}
NTSTATUS NTDLL::NtQueryInformationFile(PVOID FileHandle, PIO_STATUS_BLOCK IoStatusBlock, PVOID FileInformation, UINT32 Length, UINT32 FileInformationClass)
{
    return ((NTSTATUS(STDCALL *)(PVOID FileHandle, PIO_STATUS_BLOCK IoStatusBlock, PVOID FileInformation, UINT32 Length, UINT32 FileInformationClass))ResolveNtdllExportAddress("NtQueryInformationFile"))(FileHandle, IoStatusBlock, FileInformation, Length, FileInformationClass);
}
NTSTATUS NTDLL::NtReadFile(PVOID FileHandle, PVOID Event, PIO_APC_ROUTINE ApcRoutine, PVOID ApcContext, PIO_STATUS_BLOCK IoStatusBlock, PVOID Buffer, UINT32 Length, PLARGE_INTEGER ByteOffset, PUINT32 Key)
{
    return ((NTSTATUS(STDCALL *)(PVOID FileHandle, PVOID Event, PIO_APC_ROUTINE ApcRoutine, PVOID ApcContext, PIO_STATUS_BLOCK IoStatusBlock, PVOID Buffer, UINT32 Length, PLARGE_INTEGER ByteOffset, PUINT32 Key))ResolveNtdllExportAddress("NtReadFile"))(FileHandle, Event, ApcRoutine, ApcContext, IoStatusBlock, Buffer, Length, ByteOffset, Key);
}   
NTSTATUS NTDLL::NtWriteFile(PVOID FileHandle, PVOID Event, PIO_APC_ROUTINE ApcRoutine, PVOID ApcContext, PIO_STATUS_BLOCK IoStatusBlock, PVOID Buffer, UINT32 Length, PLARGE_INTEGER ByteOffset, PUINT32 Key)
{
    return ((NTSTATUS(STDCALL *)(PVOID FileHandle, PVOID Event, PIO_APC_ROUTINE ApcRoutine, PVOID ApcContext, PIO_STATUS_BLOCK IoStatusBlock, PVOID Buffer, UINT32 Length, PLARGE_INTEGER ByteOffset, PUINT32 Key))ResolveNtdllExportAddress("NtWriteFile"))(FileHandle, Event, ApcRoutine, ApcContext, IoStatusBlock, Buffer, Length, ByteOffset, Key);
}
NTSTATUS NTDLL::NtSetInformationFile(PVOID FileHandle, PIO_STATUS_BLOCK IoStatusBlock, PVOID FileInformation, UINT32 Length, UINT32 FileInformationClass)
{
    return ((NTSTATUS(STDCALL *)(PVOID FileHandle, PIO_STATUS_BLOCK IoStatusBlock, PVOID FileInformation, UINT32 Length, UINT32 FileInformationClass))ResolveNtdllExportAddress("NtSetInformationFile"))(FileHandle, IoStatusBlock, FileInformation, Length, FileInformationClass);
}
BOOL NTDLL::ZwDeleteFile(POBJECT_ATTRIBUTES FileName)
{
    return ((BOOL(STDCALL *)(POBJECT_ATTRIBUTES FileName))ResolveNtdllExportAddress("NtDeleteFile"))(FileName);
}
NTSTATUS NTDLL::NtQueryAttributesFile(POBJECT_ATTRIBUTES ObjectAttributes, PFILE_BASIC_INFORMATION FileInformation)
{
    return ((NTSTATUS(STDCALL *)(POBJECT_ATTRIBUTES ObjectAttributes, PFILE_BASIC_INFORMATION FileInformation))ResolveNtdllExportAddress("NtQueryAttributesFile"))(ObjectAttributes, FileInformation);
}
NTSTATUS NTDLL::NtOpenFile(PPVOID FileHandle, UINT32 DesiredAccess, POBJECT_ATTRIBUTES ObjectAttributes, PIO_STATUS_BLOCK IoStatusBlock, UINT32 ShareAccess, UINT32 OpenOptions)
{
    return ((NTSTATUS(STDCALL *)(PPVOID FileHandle, UINT32 DesiredAccess, POBJECT_ATTRIBUTES ObjectAttributes, PIO_STATUS_BLOCK IoStatusBlock, UINT32 ShareAccess, UINT32 OpenOptions))ResolveNtdllExportAddress("NtOpenFile"))(FileHandle, DesiredAccess, ObjectAttributes, IoStatusBlock, ShareAccess, OpenOptions);
}
NTSTATUS NTDLL::RtlDosPathNameToNtPathName_U(const WCHAR* DosName, UNICODE_STRING* NtName, WCHAR** FilePart, PRTL_RELATIVE_NAME_U RelativeName)
{
    return ((NTSTATUS(STDCALL *)(const WCHAR* DosName, UNICODE_STRING* NtName, WCHAR** FilePart, PRTL_RELATIVE_NAME_U RelativeName))ResolveNtdllExportAddress("RtlDosPathNameToNtPathName_U"))(DosName, NtName, FilePart, RelativeName);
}
NTSTATUS NTDLL::RtlFreeUnicodeString(PUNICODE_STRING UnicodeString)
{
    return ((NTSTATUS(STDCALL *)(PUNICODE_STRING UnicodeString))ResolveNtdllExportAddress("RtlFreeUnicodeString"))(UnicodeString);
}
NTSTATUS NTDLL::NtQueryVolumeInformationFile(PVOID FileHandle, PIO_STATUS_BLOCK IoStatusBlock, PVOID FsInformation, UINT32 Length, UINT32 FsInformationClass)
{
    return ((NTSTATUS(STDCALL *)(PVOID FileHandle, PIO_STATUS_BLOCK IoStatusBlock, PVOID FsInformation, UINT32 Length, UINT32 FsInformationClass))ResolveNtdllExportAddress("NtQueryVolumeInformationFile"))(FileHandle, IoStatusBlock, FsInformation, Length, FsInformationClass);
}
NTSTATUS NTDLL::NtQueryInformationProcess(PVOID ProcessHandle, UINT32 ProcessInformationClass, PVOID ProcessInformation, UINT32 ProcessInformationLength, PUINT32 ReturnLength)
{
    return ((NTSTATUS(STDCALL *)(PVOID ProcessHandle, UINT32 ProcessInformationClass, PVOID ProcessInformation, UINT32 ProcessInformationLength, PUINT32 ReturnLength))ResolveNtdllExportAddress("NtQueryInformationProcess"))(ProcessHandle, ProcessInformationClass, ProcessInformation, ProcessInformationLength, ReturnLength);
}
