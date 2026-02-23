#include "ntdll.h"
#include "platform.h"
#include "peb.h"
#include "platform/windows/system.h"

#define ResolveNtdllExportAddress(functionName) ResolveExportAddressFromPebModule(Djb2::HashCompileTime(L"ntdll.dll"), Djb2::HashCompileTime(functionName))
#define CALL_FUNCTION(functionName, ...) -1
NTSTATUS NTDLL::ZwCreateEvent(PPVOID EventHandle, UINT32 DesiredAccess, POBJECT_ATTRIBUTES ObjectAttributes, EVENT_TYPE EventType, INT8 InitialState)
{
    SYSCALL_ENTRY entry = ResolveSyscall("ZwCreateEvent");
    return entry.ssn != SYSCALL_SSN_INVALID
               ? System::Call(entry, (USIZE)EventHandle, DesiredAccess, (USIZE)ObjectAttributes, (USIZE)EventType, (USIZE)InitialState)
               : CALL_FUNCTION("ZwCreateEvent", PPVOID EventHandle, UINT32 DesiredAccess, POBJECT_ATTRIBUTES ObjectAttributes, EVENT_TYPE EventType, INT8 InitialState);
}
NTSTATUS NTDLL::ZwDeviceIoControlFile(PVOID FileHandle, PVOID Event, PIO_APC_ROUTINE ApcRoutine, PVOID ApcContext, PIO_STATUS_BLOCK IoStatusBlock, UINT32 IoControlCode, PVOID InputBuffer, UINT32 InputBufferLength, PVOID OutputBuffer, UINT32 OutputBufferLength)
{
    SYSCALL_ENTRY entry = ResolveSyscall("ZwDeviceIoControlFile");
    return entry.ssn != SYSCALL_SSN_INVALID
               ? System::Call(entry, (USIZE)FileHandle, (USIZE)Event, (USIZE)ApcRoutine, (USIZE)ApcContext, (USIZE)IoStatusBlock, IoControlCode, (USIZE)InputBuffer, InputBufferLength, (USIZE)OutputBuffer, OutputBufferLength)
               : CALL_FUNCTION("ZwDeviceIoControlFile", PVOID FileHandle, PVOID Event, PIO_APC_ROUTINE ApcRoutine, PVOID ApcContext, PIO_STATUS_BLOCK IoStatusBlock, UINT32 IoControlCode, PVOID InputBuffer, UINT32 InputBufferLength, PVOID OutputBuffer, UINT32 OutputBufferLength);
}

NTSTATUS NTDLL::ZwWaitForSingleObject(PVOID Object, INT8 Alertable, PLARGE_INTEGER Timeout)
{
    SYSCALL_ENTRY entry = ResolveSyscall("ZwWaitForSingleObject");
    return entry.ssn != SYSCALL_SSN_INVALID
               ? System::Call(entry, (USIZE)Object, (USIZE)Alertable, (USIZE)Timeout)
               : CALL_FUNCTION("ZwWaitForSingleObject", PVOID Object, INT8 Alertable, PLARGE_INTEGER Timeout);
}

NTSTATUS NTDLL::ZwClose(PVOID Handle)
{
    SYSCALL_ENTRY entry = ResolveSyscall("ZwClose");
    return entry.ssn != SYSCALL_SSN_INVALID
               ? System::Call(entry, (USIZE)Handle)
               : CALL_FUNCTION("ZwClose", PVOID Handle);
}

NTSTATUS NTDLL::ZwCreateFile(PPVOID FileHandle, UINT32 DesiredAccess, PVOID ObjectAttributes, PIO_STATUS_BLOCK IoStatusBlock, PLARGE_INTEGER AllocationSize, UINT32 FileAttributes, UINT32 ShareAccess, UINT32 CreateDisposition, UINT32 CreateOptions, PVOID EaBuffer, UINT32 EaLength)
{
    SYSCALL_ENTRY entry = ResolveSyscall("ZwCreateFile");

    return entry.ssn != SYSCALL_SSN_INVALID
               ? System::Call(entry, (USIZE)FileHandle, DesiredAccess, (USIZE)ObjectAttributes, (USIZE)IoStatusBlock, (USIZE)AllocationSize, FileAttributes, ShareAccess, CreateDisposition, CreateOptions, (USIZE)EaBuffer, EaLength)
               : CALL_FUNCTION("ZwCreateFile", PPVOID FileHandle, UINT32 DesiredAccess, PVOID ObjectAttributes, PIO_STATUS_BLOCK IoStatusBlock, PLARGE_INTEGER AllocationSize, UINT32 FileAttributes, UINT32 ShareAccess, UINT32 CreateDisposition, UINT32 CreateOptions, PVOID EaBuffer, UINT32 EaLength);
}

NTSTATUS NTDLL::ZwAllocateVirtualMemory(PVOID ProcessHandle, PPVOID BaseAddress, USIZE ZeroBits, PUSIZE RegionSize, UINT32 AllocationType, UINT32 Protect)
{
    SYSCALL_ENTRY entry = ResolveSyscall("ZwAllocateVirtualMemory");
    return entry.ssn != SYSCALL_SSN_INVALID
               ? System::Call(entry, (USIZE)ProcessHandle, (USIZE)BaseAddress, ZeroBits, (USIZE)RegionSize, (USIZE)AllocationType, (USIZE)Protect)
               : CALL_FUNCTION("ZwAllocateVirtualMemory", PVOID ProcessHandle, PPVOID BaseAddress, USIZE ZeroBits, PUSIZE RegionSize, UINT32 AllocationType, UINT32 Protect);
}

NTSTATUS NTDLL::ZwFreeVirtualMemory(PVOID ProcessHandle, PPVOID BaseAddress, PUSIZE RegionSize, UINT32 FreeType)
{
    SYSCALL_ENTRY entry = ResolveSyscall("ZwFreeVirtualMemory");
    return entry.ssn != SYSCALL_SSN_INVALID
               ? System::Call(entry, (USIZE)ProcessHandle, (USIZE)BaseAddress, (USIZE)RegionSize, (USIZE)FreeType)
               : CALL_FUNCTION("ZwFreeVirtualMemory", PVOID ProcessHandle, PPVOID BaseAddress, PUSIZE RegionSize, UINT32 FreeType);
}

NTSTATUS NTDLL::ZwTerminateProcess(PVOID ProcessHandle, NTSTATUS ExitStatus)
{
    SYSCALL_ENTRY entry = ResolveSyscall("ZwTerminateProcess");

    return entry.ssn != SYSCALL_SSN_INVALID
               ? System::Call(entry, (USIZE)ProcessHandle, (USIZE)ExitStatus)
               : CALL_FUNCTION("ZwTerminateProcess", PVOID ProcessHandle, NTSTATUS ExitStatus);
}

NTSTATUS NTDLL::ZwQueryInformationFile(PVOID FileHandle, PIO_STATUS_BLOCK IoStatusBlock, PVOID FileInformation, UINT32 Length, UINT32 FileInformationClass)
{
    SYSCALL_ENTRY entry = ResolveSyscall("ZwQueryInformationFile");
    return entry.ssn != SYSCALL_SSN_INVALID
               ? System::Call(entry, (USIZE)FileHandle, (USIZE)IoStatusBlock, (USIZE)FileInformation, Length, FileInformationClass)
               : CALL_FUNCTION("ZwQueryInformationFile", PVOID FileHandle, PIO_STATUS_BLOCK IoStatusBlock, PVOID FileInformation, UINT32 Length, UINT32 FileInformationClass);
}
NTSTATUS NTDLL::ZwReadFile(PVOID FileHandle, PVOID Event, PIO_APC_ROUTINE ApcRoutine, PVOID ApcContext, PIO_STATUS_BLOCK IoStatusBlock, PVOID Buffer, UINT32 Length, PLARGE_INTEGER ByteOffset, PUINT32 Key)
{
    SYSCALL_ENTRY entry = ResolveSyscall("ZwReadFile");
    return entry.ssn != SYSCALL_SSN_INVALID
               ? System::Call(entry, (USIZE)FileHandle, (USIZE)Event, (USIZE)ApcRoutine, (USIZE)ApcContext, (USIZE)IoStatusBlock, (USIZE)Buffer, Length, (USIZE)ByteOffset, (USIZE)Key)
               : CALL_FUNCTION("ZwReadFile", PVOID FileHandle, PVOID Event, PIO_APC_ROUTINE ApcRoutine, PVOID ApcContext, PIO_STATUS_BLOCK IoStatusBlock, PVOID Buffer, UINT32 Length, PLARGE_INTEGER ByteOffset, PUINT32 Key);
}
NTSTATUS NTDLL::ZwWriteFile(PVOID FileHandle, PVOID Event, PIO_APC_ROUTINE ApcRoutine, PVOID ApcContext, PIO_STATUS_BLOCK IoStatusBlock, PVOID Buffer, UINT32 Length, PLARGE_INTEGER ByteOffset, PUINT32 Key)
{
    SYSCALL_ENTRY entry = ResolveSyscall("ZwWriteFile");
    return entry.ssn != SYSCALL_SSN_INVALID
               ? System::Call(entry, (USIZE)FileHandle, (USIZE)Event, (USIZE)ApcRoutine, (USIZE)ApcContext, (USIZE)IoStatusBlock, (USIZE)Buffer, Length, (USIZE)ByteOffset, (USIZE)Key)
               : CALL_FUNCTION("ZwWriteFile", PVOID FileHandle, PVOID Event, PIO_APC_ROUTINE ApcRoutine, PVOID ApcContext, PIO_STATUS_BLOCK IoStatusBlock, PVOID Buffer, UINT32 Length, PLARGE_INTEGER ByteOffset, PUINT32 Key);
}
NTSTATUS NTDLL::ZwSetInformationFile(PVOID FileHandle, PIO_STATUS_BLOCK IoStatusBlock, PVOID FileInformation, UINT32 Length, UINT32 FileInformationClass)
{
    SYSCALL_ENTRY entry = ResolveSyscall("ZwSetInformationFile");
    return entry.ssn != SYSCALL_SSN_INVALID
               ? System::Call(entry, (USIZE)FileHandle, (USIZE)IoStatusBlock, (USIZE)FileInformation, Length, FileInformationClass)
               : CALL_FUNCTION("ZwSetInformationFile", PVOID FileHandle, PIO_STATUS_BLOCK IoStatusBlock, PVOID FileInformation, UINT32 Length, UINT32 FileInformationClass);
}
BOOL NTDLL::ZwDeleteFile(POBJECT_ATTRIBUTES FileName)
{
    SYSCALL_ENTRY entry = ResolveSyscall("ZwDeleteFile");
    return entry.ssn != SYSCALL_SSN_INVALID
               ? System::Call(entry, (USIZE)FileName)
               : CALL_FUNCTION("ZwDeleteFile", POBJECT_ATTRIBUTES FileName);
}
NTSTATUS NTDLL::ZwQueryAttributesFile(POBJECT_ATTRIBUTES ObjectAttributes, PFILE_BASIC_INFORMATION FileInformation)
{
    SYSCALL_ENTRY entry = ResolveSyscall("ZwQueryAttributesFile");
    return entry.ssn != SYSCALL_SSN_INVALID
               ? System::Call(entry, (USIZE)ObjectAttributes, (USIZE)FileInformation)
               : CALL_FUNCTION("ZwQueryAttributesFile", POBJECT_ATTRIBUTES ObjectAttributes, PFILE_BASIC_INFORMATION FileInformation);
}
NTSTATUS NTDLL::ZwOpenFile(PPVOID FileHandle, UINT32 DesiredAccess, POBJECT_ATTRIBUTES ObjectAttributes, PIO_STATUS_BLOCK IoStatusBlock, UINT32 ShareAccess, UINT32 OpenOptions)
{
    SYSCALL_ENTRY entry = ResolveSyscall("ZwOpenFile");
    return entry.ssn != SYSCALL_SSN_INVALID
               ? System::Call(entry, (USIZE)FileHandle, (USIZE)DesiredAccess, (USIZE)ObjectAttributes, (USIZE)IoStatusBlock, (USIZE)ShareAccess, (USIZE)OpenOptions)
               : CALL_FUNCTION("ZwOpenFile", PPVOID FileHandle, UINT32 DesiredAccess, POBJECT_ATTRIBUTES ObjectAttributes, PIO_STATUS_BLOCK IoStatusBlock, UINT32 ShareAccess, UINT32 OpenOptions);
}
NTSTATUS NTDLL::RtlDosPathNameToNtPathName_U(const WCHAR *DosName, UNICODE_STRING *NtName, WCHAR **FilePart, PRTL_RELATIVE_NAME_U RelativeName)
{
    return ((NTSTATUS(STDCALL *)(const WCHAR *DosName, UNICODE_STRING *NtName, WCHAR **FilePart, PRTL_RELATIVE_NAME_U RelativeName))ResolveNtdllExportAddress("RtlDosPathNameToNtPathName_U"))(DosName, NtName, FilePart, RelativeName);
}
NTSTATUS NTDLL::RtlFreeUnicodeString(PUNICODE_STRING UnicodeString)
{
    return ((NTSTATUS(STDCALL *)(PUNICODE_STRING UnicodeString))ResolveNtdllExportAddress("RtlFreeUnicodeString"))(UnicodeString);
}
NTSTATUS NTDLL::ZwQueryVolumeInformationFile(PVOID FileHandle, PIO_STATUS_BLOCK IoStatusBlock, PVOID FsInformation, UINT32 Length, UINT32 FsInformationClass)
{
    SYSCALL_ENTRY entry = ResolveSyscall("ZwQueryVolumeInformationFile");
    return entry.ssn != SYSCALL_SSN_INVALID
               ? System::Call(entry, (USIZE)FileHandle, (USIZE)IoStatusBlock, (USIZE)FsInformation, Length, FsInformationClass)
               : CALL_FUNCTION("ZwQueryVolumeInformationFile", PVOID FileHandle, PIO_STATUS_BLOCK IoStatusBlock, PVOID FsInformation, UINT32 Length, UINT32 FsInformationClass);
}
NTSTATUS NTDLL::ZwQueryInformationProcess(PVOID ProcessHandle, UINT32 ProcessInformationClass, PVOID ProcessInformation, UINT32 ProcessInformationLength, PUINT32 ReturnLength)
{
    SYSCALL_ENTRY entry = ResolveSyscall("ZwQueryInformationProcess");
    return entry.ssn != SYSCALL_SSN_INVALID
               ? System::Call(entry, (USIZE)ProcessHandle, (USIZE)ProcessInformationClass, (USIZE)ProcessInformation, ProcessInformationLength, (USIZE)ReturnLength)
               : CALL_FUNCTION("ZwQueryInformationProcess", PVOID ProcessHandle, UINT32 ProcessInformationClass, PVOID ProcessInformation, UINT32 ProcessInformationLength, PUINT32 ReturnLength);
}
NTSTATUS NTDLL::ZwCreateNamedPipeFile(PPVOID FileHandle, UINT32 DesiredAccess, POBJECT_ATTRIBUTES ObjectAttributes, PIO_STATUS_BLOCK IoStatusBlock, UINT32 ShareAccess, UINT32 CreateDisposition, UINT32 CreateOptions, UINT32 NamedPipeType, UINT32 ReadMode, UINT32 CompletionMode, UINT32 MaximumInstances, UINT32 InboundQuota, UINT32 OutboundQuota, PLARGE_INTEGER DefaultTimeout)
{
    SYSCALL_ENTRY entry = ResolveSyscall("ZwCreateNamedPipeFile");
    return entry.ssn != SYSCALL_SSN_INVALID
               ? System::Call(entry, (USIZE)FileHandle, DesiredAccess, (USIZE)ObjectAttributes, (USIZE)IoStatusBlock, ShareAccess, CreateDisposition, CreateOptions, NamedPipeType, ReadMode, CompletionMode, MaximumInstances, InboundQuota, OutboundQuota, (USIZE)DefaultTimeout)
               : CALL_FUNCTION("ZwCreateNamedPipeFile", PPVOID FileHandle, UINT32 DesiredAccess, POBJECT_ATTRIBUTES ObjectAttributes, PIO_STATUS_BLOCK IoStatusBlock, UINT32 ShareAccess, UINT32 CreateDisposition, UINT32 CreateOptions, UINT32 NamedPipeType, UINT32 ReadMode, UINT32 CompletionMode, UINT32 MaximumInstances, UINT32 InboundQuota, UINT32 OutboundQuota, PLARGE_INTEGER DefaultTimeout);
}
NTSTATUS NTDLL::ZwSetInformationObject(PVOID Handle, UINT32 ObjectInformationClass, PVOID ObjectInformation, UINT32 ObjectInformationLength)
{
    SYSCALL_ENTRY entry = ResolveSyscall("ZwSetInformationObject");
    return entry.ssn != SYSCALL_SSN_INVALID
               ? System::Call(entry, (USIZE)Handle, (USIZE)ObjectInformationClass, (USIZE)ObjectInformation, (USIZE)ObjectInformationLength)
               : CALL_FUNCTION("ZwSetInformationObject", PVOID Handle, UINT32 ObjectInformationClass, PVOID ObjectInformation, UINT32 ObjectInformationLength);
}
NTSTATUS NTDLL::ZwCreateUserProcess(PPVOID ProcessHandle, PPVOID ThreadHandle, UINT32 ProcessDesiredAccess, UINT32 ThreadDesiredAccess, POBJECT_ATTRIBUTES ProcessObjectAttributes, POBJECT_ATTRIBUTES ThreadObjectAttributes, UINT32 ProcessFlags, UINT32 ThreadFlags, PVOID ProcessParameters, PVOID CreateInfo, PVOID AttributeList)
{
    SYSCALL_ENTRY entry = ResolveSyscall("ZwCreateUserProcess");
    return entry.ssn != SYSCALL_SSN_INVALID
               ? System::Call(entry, (USIZE)ProcessHandle, (USIZE)ThreadHandle, (USIZE)ProcessDesiredAccess, (USIZE)ThreadDesiredAccess, (USIZE)ProcessObjectAttributes, (USIZE)ThreadObjectAttributes, (USIZE)ProcessFlags, (USIZE)ThreadFlags, (USIZE)ProcessParameters, (USIZE)CreateInfo, (USIZE)AttributeList)
               : CALL_FUNCTION("ZwCreateUserProcess", PPVOID ProcessHandle, PPVOID ThreadHandle, UINT32 ProcessDesiredAccess, UINT32 ThreadDesiredAccess, POBJECT_ATTRIBUTES ProcessObjectAttributes, POBJECT_ATTRIBUTES ThreadObjectAttributes, UINT32 ProcessFlags, UINT32 ThreadFlags, PVOID ProcessParameters, PVOID CreateInfo, PVOID AttributeList);
}
NTSTATUS NTDLL::RtlCreateProcessParametersEx(PVOID *ProcessParameters, PUNICODE_STRING ImagePathName, PUNICODE_STRING DllPath, PUNICODE_STRING CurrentDirectory, PUNICODE_STRING CommandLine, PVOID Environment, PUNICODE_STRING WindowTitle, PUNICODE_STRING DesktopInfo, PUNICODE_STRING ShellInfo, PUNICODE_STRING RuntimeData, UINT32 Flags)
{
    return ((NTSTATUS(STDCALL *)(PVOID * ProcessParameters, PUNICODE_STRING ImagePathName, PUNICODE_STRING DllPath, PUNICODE_STRING CurrentDirectory, PUNICODE_STRING CommandLine, PVOID Environment, PUNICODE_STRING WindowTitle, PUNICODE_STRING DesktopInfo, PUNICODE_STRING ShellInfo, PUNICODE_STRING RuntimeData, UINT32 Flags)) ResolveNtdllExportAddress("RtlCreateProcessParametersEx"))(ProcessParameters, ImagePathName, DllPath, CurrentDirectory, CommandLine, Environment, WindowTitle, DesktopInfo, ShellInfo, RuntimeData, Flags);
}
NTSTATUS NTDLL::RtlDestroyProcessParameters(PVOID ProcessParameters)
{
    return ((NTSTATUS(STDCALL *)(PVOID ProcessParameters))ResolveNtdllExportAddress("RtlDestroyProcessParameters"))(ProcessParameters);
}
NTSTATUS NTDLL::ZwQueryDirectoryFile(PVOID FileHandle, PVOID Event, PIO_APC_ROUTINE ApcRoutine, PVOID ApcContext, PIO_STATUS_BLOCK IoStatusBlock, PVOID FileInformation, UINT32 Length, UINT32 FileInformationClass, BOOL ReturnSingleEntry, PUNICODE_STRING FileName, BOOL RestartScan)
{
    SYSCALL_ENTRY entry = ResolveSyscall("ZwQueryDirectoryFile");
    return entry.ssn != SYSCALL_SSN_INVALID
               ? System::Call(entry, (USIZE)FileHandle, (USIZE)Event, (USIZE)ApcRoutine, (USIZE)ApcContext, (USIZE)IoStatusBlock, (USIZE)FileInformation, Length, FileInformationClass, (USIZE)ReturnSingleEntry, (USIZE)FileName, (USIZE)RestartScan)
               : CALL_FUNCTION("ZwQueryDirectoryFile", PVOID FileHandle, PVOID Event, PIO_APC_ROUTINE ApcRoutine, PVOID ApcContext, PIO_STATUS_BLOCK IoStatusBlock, PVOID FileInformation, UINT32 Length, UINT32 FileInformationClass, BOOL ReturnSingleEntry, PUNICODE_STRING FileName, BOOL RestartScan);
}
