#pragma once

#include "primitives.h"
#include "windows_types.h"
#include "djb2.h"

#define EVENT_ALL_ACCESS ((0x000F0000L) | (0x00100000L) | 0x3)
#define NT_SUCCESS(Status) ((NTSTATUS)(Status) >= 0)
#define OBJ_CASE_INSENSITIVE 0x00000040L

// Structure for event type
typedef enum _EVENT_TYPE
{
    NotificationEvent,
    SynchronizationEvent
} EVENT_TYPE,
    *PEVENT_TYPE;

// Structure for file information classes - basic, standard, position and disposition information
typedef struct _FILE_BASIC_INFORMATION
{
    LARGE_INTEGER CreationTime;
    LARGE_INTEGER LastAccessTime;
    LARGE_INTEGER LastWriteTime;
    LARGE_INTEGER ChangeTime;
    UINT32 FileAttributes;
} FILE_BASIC_INFORMATION, *PFILE_BASIC_INFORMATION;

typedef struct _FILE_STANDARD_INFORMATION
{
    LARGE_INTEGER AllocationSize; // Number of bytes allocated for the file
    LARGE_INTEGER EndOfFile;      // Actual file size in bytes
    UINT32 NumberOfLinks;         // Number of hard links
    INT8 DeletePending;           // TRUE if file is marked for deletion
    INT8 Directory;               // TRUE if it is a directory
} FILE_STANDARD_INFORMATION, *PFILE_STANDARD_INFORMATION;

typedef struct _FILE_POSITION_INFORMATION
{
    LARGE_INTEGER CurrentByteOffset;
} FILE_POSITION_INFORMATION, *PFILE_POSITION_INFORMATION;

typedef struct _FILE_DISPOSITION_INFORMATION
{
    BOOL DeleteFile;
} FILE_DISPOSITION_INFORMATION, *PFILE_DISPOSITION_INFORMATION;

typedef VOID(STDCALL *PIO_APC_ROUTINE)(
    PVOID ApcContext,
    PIO_STATUS_BLOCK IoStatusBlock,
    UINT32 Reserved);

typedef struct _RTLP_CURDIR_REF *PRTLP_CURDIR_REF;

// Relative name structure
typedef struct _RTL_RELATIVE_NAME_U
{
    UNICODE_STRING RelativeName;
    PVOID ContainingDirectory;
    PRTLP_CURDIR_REF CurDirRef;
} RTL_RELATIVE_NAME_U, *PRTL_RELATIVE_NAME_U;

// File information class
typedef struct _FILE_FS_DEVICE_INFORMATION
{
    UINT32 DeviceType;      // Type of the device (e.g., FILE_DEVICE_DISK)
    UINT32 Characteristics; // Bitmask of device characteristics (see FILE_DEVICE_* flags)
} FILE_FS_DEVICE_INFORMATION, *PFILE_FS_DEVICE_INFORMATION;

// Structure to query detailed information about files in a directory
typedef struct _FILE_BOTH_DIR_INFORMATION
{
    UINT32 NextEntryOffset;
    UINT32 FileIndex;
    LARGE_INTEGER CreationTime;
    LARGE_INTEGER LastAccessTime;
    LARGE_INTEGER LastWriteTime;
    LARGE_INTEGER ChangeTime;
    LARGE_INTEGER EndOfFile;
    LARGE_INTEGER AllocationSize;
    UINT32 FileAttributes;
    UINT32 FileNameLength;
    UINT32 EaSize;
    INT8 ShortNameLength;
    WCHAR ShortName[12];
    WCHAR FileName[1];
} FILE_BOTH_DIR_INFORMATION, *PFILE_BOTH_DIR_INFORMATION;

// Information class
typedef enum _FILE_INFORMATION_CLASS_DIR
{
    FileBothDirectoryInformation = 3,
    FileFsDeviceInformation = 4,
    FileStandardInformation = 5,
    FileDispositionInformation = 13,
    FilePositionInformation = 14,
} FILE_INFORMATION_CLASS_DIR;

class NTDLL
{
private:
public:
    // This functiuon converts a DOS path name to an NT path name.
    // Minimum supported client
    static NTSTATUS RtlDosPathNameToNtPathName_U(const WCHAR *DosName, UNICODE_STRING *NtName, WCHAR **FilePart, PRTL_RELATIVE_NAME_U RelativeName);
    // This function frees a Unicode string that was allocated.
    // Minimum supported client Windows 2000 Professional [desktop apps only].
    static NTSTATUS RtlFreeUnicodeString(PUNICODE_STRING UnicodeString);
    // This function creates or opens an event object.
    // Minimum supported client	Windows XP.
    static NTSTATUS ZwCreateEvent(PPVOID EventHandle, UINT32 DesiredAccess, POBJECT_ATTRIBUTES ObjectAttributes, EVENT_TYPE EventType, INT8 InitialState);
    // This function builds descriptors for the supplied buffer(s) and passes the untyped data to the device driver associated with the file handle
    // Minimum supported client	Windows 2000 Professional [desktop apps only]
    static NTSTATUS ZwDeviceIoControlFile(PVOID FileHandle, PVOID Event, PIO_APC_ROUTINE ApcRoutine, PVOID ApcContext, PIO_STATUS_BLOCK IoStatusBlock, UINT32 IoControlCode, PVOID InputBuffer, UINT32 InputBufferLength, PVOID OutputBuffer, UINT32 OutputBufferLength);
    // This function waits until the specified object attains a state of signaled.
    // Minimum supported client	Windows 2000 Professional [desktop apps only]
    static NTSTATUS ZwWaitForSingleObject(PVOID Object, INT8 Alertable, PLARGE_INTEGER Timeout);
    // This function closes a handle to an object.
    // Minimum supported client	Windows 2000 Professional [desktop apps only]
    static NTSTATUS ZwClose(PVOID Handle);
    // This function creates a new file or directory, or opens an existing file, device, directory, or volume.
    // Minimum supported client	Windows XP [desktop apps | UWP apps]
    static NTSTATUS ZwCreateFile(PPVOID FileHandle, UINT32 DesiredAccess, PVOID ObjectAttributes, PIO_STATUS_BLOCK IoStatusBlock, PLARGE_INTEGER AllocationSize, UINT32 FileAttributes, UINT32 ShareAccess, UINT32 CreateDisposition, UINT32 CreateOptions, PVOID EaBuffer, UINT32 EaLength);
    // This function allocates virtual memory in the address space of a process.
    // Minimum supported client	Windows XP.
    static NTSTATUS ZwAllocateVirtualMemory(PVOID ProcessHandle, PPVOID BaseAddress, USIZE ZeroBits, PUSIZE RegionSize, UINT32 AllocationType, UINT32 Protect);
    // This function releases virtual memory in the address space of a process.
    // Minimum supported client	Windows XP.
    static NTSTATUS ZwFreeVirtualMemory(PVOID ProcessHandle, PPVOID BaseAddress, PUSIZE RegionSize, UINT32 FreeType);
    // This function terminates the specified process and all of its threads.
    // Target platform is universal.
    static NTSTATUS ZwTerminateProcess(PVOID ProcessHandle, NTSTATUS ExitStatus);
    // This function retrieves file information for the specified file.
    // Minimum supported client	Windows 2000.
    static NTSTATUS ZwQueryInformationFile(PVOID FileHandle, PIO_STATUS_BLOCK IoStatusBlock, PVOID FileInformation, UINT32 Length, UINT32 FileInformationClass);
    // This function reads data from an open file.
    // Minimum supported client	Windows 2000 Professional [desktop apps | UWP apps].
    static NTSTATUS ZwReadFile(PVOID FileHandle, PVOID Event, PIO_APC_ROUTINE ApcRoutine, PVOID ApcContext, PIO_STATUS_BLOCK IoStatusBlock, PVOID Buffer, UINT32 Length, PLARGE_INTEGER ByteOffset, PUINT32 Key);
    // This function writes data to an open file.
    // Minimum supported client	Windows 2000.
    static NTSTATUS ZwWriteFile(PVOID FileHandle, PVOID Event, PIO_APC_ROUTINE ApcRoutine, PVOID ApcContext, PIO_STATUS_BLOCK IoStatusBlock, PVOID Buffer, UINT32 Length, PLARGE_INTEGER ByteOffset, PUINT32 Key);
    // This function sets various types of information for a file object.
    // Minimum supported client	Windows 2000.
    static NTSTATUS ZwSetInformationFile(PVOID FileHandle, PIO_STATUS_BLOCK IoStatusBlock, PVOID FileInformation, UINT32 Length, UINT32 FileInformationClass);
    // This function deletes a file.
    // Minimum supported client	Windows XP.
    static BOOL ZwDeleteFile(POBJECT_ATTRIBUTES FileName);
    // This function retrieves basic information about a file.
    // Minimum supported client
    static NTSTATUS ZwQueryAttributesFile(POBJECT_ATTRIBUTES ObjectAttributes, PFILE_BASIC_INFORMATION FileInformation);
    // This function opens an existing file, device, directory, or volume, and returns a handle for the file object.
    // Target Platform	Windows.
    static NTSTATUS ZwOpenFile(PPVOID FileHandle, UINT32 DesiredAccess, POBJECT_ATTRIBUTES ObjectAttributes, PIO_STATUS_BLOCK IoStatusBlock, UINT32 ShareAccess, UINT32 OpenOptions);
    // This function retrieves volume information for the specified file system.
    // Minimum supported client	Windows XP.
    static NTSTATUS ZwQueryVolumeInformationFile(PVOID FileHandle, PIO_STATUS_BLOCK IoStatusBlock, PVOID FsInformation, UINT32 Length, UINT32 FsInformationClass);
    // This function retrieves information about a process.
    // Target Platform	Windows
    static NTSTATUS ZwQueryInformationProcess(PVOID ProcessHandle, UINT32 ProcessInformationClass, PVOID ProcessInformation, UINT32 ProcessInformationLength, PUINT32 ReturnLength);
    // This function returns a handle to the current process.
    // All target platforms.
    static inline PVOID NtCurrentProcess() { return (PVOID)(USIZE)-1L; }
    static inline PVOID NtCurrentThread() { return (PVOID)(USIZE)-2L; }
    // This function creates a named pipe.
    // Minimum supported client Windows 2000.
    static NTSTATUS ZwCreateNamedPipeFile(PPVOID FileHandle, UINT32 DesiredAccess, POBJECT_ATTRIBUTES ObjectAttributes, PIO_STATUS_BLOCK IoStatusBlock, UINT32 ShareAccess, UINT32 CreateDisposition, UINT32 CreateOptions, UINT32 NamedPipeType, UINT32 ReadMode, UINT32 CompletionMode, UINT32 MaximumInstances, UINT32 InboundQuota, UINT32 OutboundQuota, PLARGE_INTEGER DefaultTimeout);
    // This function sets information for an object.
    // Minimum supported client Windows 2000.
    static NTSTATUS ZwSetInformationObject(PVOID Handle, UINT32 ObjectInformationClass, PVOID ObjectInformation, UINT32 ObjectInformationLength);
    // This function creates a user process.
    // Minimum supported client Windows Vista.
    static NTSTATUS ZwCreateUserProcess(PPVOID ProcessHandle, PPVOID ThreadHandle, UINT32 ProcessDesiredAccess, UINT32 ThreadDesiredAccess, POBJECT_ATTRIBUTES ProcessObjectAttributes, POBJECT_ATTRIBUTES ThreadObjectAttributes, UINT32 ProcessFlags, UINT32 ThreadFlags, PVOID ProcessParameters, PVOID CreateInfo, PVOID AttributeList);
    // This function creates process parameters.
    // Minimum supported client Windows 2000.
    static NTSTATUS RtlCreateProcessParametersEx(PVOID *ProcessParameters, PUNICODE_STRING ImagePathName, PUNICODE_STRING DllPath, PUNICODE_STRING CurrentDirectory, PUNICODE_STRING CommandLine, PVOID Environment, PUNICODE_STRING WindowTitle, PUNICODE_STRING DesktopInfo, PUNICODE_STRING ShellInfo, PUNICODE_STRING RuntimeData, UINT32 Flags);
    // This function destroys process parameters.
    // Minimum supported client Windows 2000.
    static NTSTATUS RtlDestroyProcessParameters(PVOID ProcessParameters);
    // This function queries directory information for a file.
    // Minimum supported client Windows 2000.
    static NTSTATUS ZwQueryDirectoryFile(PVOID FileHandle, PVOID Event, PIO_APC_ROUTINE ApcRoutine, PVOID ApcContext, PIO_STATUS_BLOCK IoStatusBlock, PVOID FileInformation, UINT32 Length, UINT32 FileInformationClass, BOOL ReturnSingleEntry, PUNICODE_STRING FileName, BOOL RestartScan);
};