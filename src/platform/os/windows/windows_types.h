#pragma once

#include "primitives.h"

#define FILE_OPEN 0x00000001
#define FILE_NON_DIRECTORY_FILE 0x00000040
#define SYNCHRONIZE (0x00100000L)
#define DELETE (0x00010000L)
#define FILE_READ_ATTRIBUTES (0x0080)
#define FILE_SYNCHRONOUS_IO_NONALERT 0x00000020
#define FILE_ATTRIBUTE_NORMAL 0x00000080
#define GENERIC_READ (0x80000000L)
#define FILE_SHARE_READ 0x00000001
#define INVALID_HANDLE_VALUE ((PVOID)(SSIZE)(-1))
#define GENERIC_WRITE (0x40000000L)
#define FILE_APPEND_DATA 4
#define FILE_DELETE_ON_CLOSE 0x00001000

#define FILE_SHARE_WRITE 0x00000002
#define FILE_SHARE_DELETE 0x00000004

#define FILE_DEVICE_CD_ROM_FILE_SYSTEM 0x00000003
#define FILE_DEVICE_VIRTUAL_DISK 0x00000024
#define FILE_DEVICE_NETWORK_FILE_SYSTEM 0x00000014
#define FILE_DEVICE_DISK_FILE_SYSTEM 0x00000008

#define FILE_REMOTE_DEVICE 0x00000010
#define FILE_REMOVABLE_MEDIA 0x00000002

#define DRIVE_UNKNOWN 0
#define DRIVE_REMOVABLE 2
#define DRIVE_FIXED 3
#define DRIVE_REMOTE 4
#define DRIVE_CDROM 5
#define DRIVE_RAMDISK 6

#define FILE_CREATE 0x00000002
#define FILE_LIST_DIRECTORY 0x00000001
#define FILE_DIRECTORY_FILE 0x00000001
#define FILE_ATTRIBUTE_READONLY  0x00000001
#define FILE_ATTRIBUTE_HIDDEN    0x00000002
#define FILE_ATTRIBUTE_SYSTEM    0x00000004
#define FILE_ATTRIBUTE_DIRECTORY 0x00000010
#define FILE_FLAG_OVERLAPPED 0x40000000

#define FILE_FLAG_WRITE_THROUGH 0x80000000
#define FILE_WRITE_THROUGH 0x80000000

#define FILE_OVERWRITE_IF 0x00000005
#define FILE_OPEN_IF 0x00000003
#define FILE_OVERWRITE 0x00000004

#define MEM_COMMIT 0x00001000
#define MEM_RESERVE 0x00002000
#define MEM_RELEASE 0x00008000

#define PAGE_READWRITE 0x04

#define ProcessDeviceMap 23

#define InitializeObjectAttributes(p, n, a, r, s) \
    do                                            \
    {                                             \
        (p)->Length = sizeof(OBJECT_ATTRIBUTES);  \
        (p)->RootDirectory = r;                   \
        (p)->Attributes = a;                      \
        (p)->ObjectName = n;                      \
        (p)->SecurityDescriptor = s;              \
        (p)->SecurityQualityOfService = nullptr;  \
    } while (0)

// Unicode string structure
typedef struct _UNICODE_STRING
{
    UINT16 Length;
    UINT16 MaximumLength;
    PWCHAR Buffer;
} UNICODE_STRING, *PUNICODE_STRING;

typedef INT32 NTSTATUS;

typedef struct _PROCESS_DEVICEMAP_INFORMATION
{
    union
    {
        struct
        {
            PVOID DirectoryHandle; // A handle to a directory object that can be set as the new device map for the process. This handle must have DIRECTORY_TRAVERSE access.
        } Set;
        struct
        {
            UINT32 DriveMap;     // A bitmask that indicates which drive letters are currently in use in the process's device map.
            UCHAR DriveType[32]; // A value that indicates the type of each drive (e.g., local disk, network drive, etc.). // DRIVE_* WinBase.h
        } Query;
    };
} PROCESS_DEVICEMAP_INFORMATION, *PPROCESS_DEVICEMAP_INFORMATION;

typedef struct _IO_STATUS_BLOCK
{
    union
    {
        NTSTATUS Status;
        PVOID Pointer;
    };
    USIZE Information;
} IO_STATUS_BLOCK, *PIO_STATUS_BLOCK;

typedef struct _OBJECT_ATTRIBUTES
{
    UINT32 Length;
    PVOID RootDirectory;
    PUNICODE_STRING ObjectName;
    UINT32 Attributes;
    PVOID SecurityDescriptor;
    PVOID SecurityQualityOfService;
} OBJECT_ATTRIBUTES, *POBJECT_ATTRIBUTES;

typedef union _LARGE_INTEGER
{
    struct
    {
        UINT32 LowPart;
        INT32 HighPart;
    };
    struct
    {
        UINT32 LowPart;
        INT32 HighPart;
    } u;
    INT64 QuadPart;
} LARGE_INTEGER, *PLARGE_INTEGER;
