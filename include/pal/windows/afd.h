#if defined(PLATFORM_WINDOWS)
#pragma once

#include "primitives.h"
#include "windows_types.h"

// =============================================================================
// AFD (Ancillary Function Driver) Constants
// =============================================================================

// AFD I/O Control Codes
#define IOCTL_AFD_BIND           0x00012003      // ((0x12 << 12) | (0 << 2) | 3)
#define IOCTL_AFD_CONNECT        0x00012007      // ((0x12 << 12) | (1 << 2) | 3)
#define IOCTL_AFD_SEND           0x0001201F      // ((0x12 << 12) | (7 << 2) | 3)
#define IOCTL_AFD_RECV           0x00012017      // ((0x12 << 12) | (5 << 2) | 3)
#define IOCTL_AFD_DISCONNECT     0x0001202B      // ((0x12 << 12) | (10 << 2) | 3)

// AFD Share Flags
#define AFD_SHARE_REUSE          0x00000001L

// AFD Disconnect Flags
#define AFD_DISCONNECT_SEND      0x00000001L
#define AFD_DISCONNECT_RECV      0x00000002L
#define AFD_DISCONNECT_ABORT     0x00000004L
#define AFD_DISCONNECT_DATAGRAM  0x00000008L

// Object Attribute Flags
#define OBJ_CASE_INSENSITIVE     0x00000040L
#define OBJ_INHERIT              0x00000002L

// NTSTATUS Values
#define STATUS_SUCCESS           ((NTSTATUS)0x00000000L)
#define STATUS_PENDING           ((NTSTATUS)0x00000103L)
#define NT_SUCCESS(Status)       (((NTSTATUS)(Status)) >= 0)

// Socket Constants
#define AF_INET                  2
#define SOCK_STREAM              1
#define IPPROTO_TCP              6

// Generic Access Rights
#define GENERIC_READ             0x80000000L
#define GENERIC_WRITE            0x40000000L
#define SYNCHRONIZE              0x00100000L
#define EVENT_ALL_ACCESS         0x001F0003L

// File Share and Creation Flags
#define FILE_SHARE_READ          0x00000001
#define FILE_SHARE_WRITE         0x00000002
#define FILE_OPEN_IF             0x00000003

// Event Types
typedef enum _EVENT_TYPE
{
	NotificationEvent,
	SynchronizationEvent
} EVENT_TYPE;

// =============================================================================
// AFD Data Structures
// =============================================================================

// Socket Address Structure
typedef struct _SockAddr
{
	UINT16 sin_family;
	UINT16 sin_port;
	UINT32 sin_addr;
	UINT8 sin_zero[8];
} SockAddr, *PSockAddr;

// AFD Bind Data Structure
typedef struct _AFD_BindData
{
	UINT32 ShareType;
	SockAddr Address;
} AFD_BindData, *PAFD_BindData;

// AFD Connect Information Structure
typedef struct _AFD_ConnectInfo
{
	SSIZE UseSAN;
	SSIZE Root;
	SSIZE Unknown;
	SockAddr Address;
} AFD_ConnectInfo, *PAFD_ConnectInfo;

// AFD Buffer Structure (Windows Socket Buffer)
typedef struct _AFD_Wsbuf
{
	UINT32 len;
	PVOID buf;
} AFD_Wsbuf, *PAFD_Wsbuf;

// AFD Send/Receive Information Structure
typedef struct _AFD_SendRecvInfo
{
	PAFD_Wsbuf BufferArray;
	UINT32 BufferCount;
	UINT32 AfdFlags;
	UINT32 TdiFlags;
} AFD_SendRecvInfo, *PAFD_SendRecvInfo;

// Socket Creation Parameters (Extended Attributes for NtCreateFile)
typedef struct _SocketParams
{
	INT32 field_0;           // Reserved
	UINT16 field_4;          // Magic value 0x0F1E
	UINT16 field_6;          // Size value 0x001E (30)
	CHAR AfdOperation[16];   // "AfdOpenPacketXX\0"
	UINT32 flag;             // Reserved flags
	INT32 Group;             // Socket group
	INT32 AddressFamily;     // AF_INET, etc.
	INT32 SocketType;        // SOCK_STREAM, etc.
	INT32 Protocol;          // IPPROTO_TCP, etc.
	UINT32 dwStringLength;   // String length
	WCHAR szString[8];       // Additional data
} SocketParams, *PSocketParams;

// I/O Status Block
typedef struct _IO_STATUS_BLOCK
{
	union
	{
		NTSTATUS Status;
		PVOID Pointer;
	} DUMMYUNIONNAME;
	USIZE Information;
} IO_STATUS_BLOCK, *PIO_STATUS_BLOCK;

// Object Attributes
typedef struct _OBJECT_ATTRIBUTES
{
	UINT32 Length;
	PVOID RootDirectory;
	PUNICODE_STRING ObjectName;
	UINT32 Attributes;
	PVOID SecurityDescriptor;
	PVOID SecurityQualityOfService;
} OBJECT_ATTRIBUTES, *POBJECT_ATTRIBUTES;

// Large Integer (64-bit value)
typedef union _LARGE_INTEGER
{
	struct
	{
		UINT32 LowPart;
		INT32 HighPart;
	} DUMMYSTRUCTNAME;
#if defined(ARCHITECTURE_X86_64) || defined(ARCHITECTURE_AARCH64)
	signed long long QuadPart;
#else
	signed long long QuadPart;
#endif
} LARGE_INTEGER, *PLARGE_INTEGER;

// =============================================================================
// Helper Macros
// =============================================================================

#define InitializeObjectAttributes(p, n, a, r, s) \
	do                                            \
	{                                             \
		(p)->Length = sizeof(OBJECT_ATTRIBUTES);  \
		(p)->RootDirectory = r;                   \
		(p)->Attributes = a;                      \
		(p)->ObjectName = n;                      \
		(p)->SecurityDescriptor = s;              \
		(p)->SecurityQualityOfService = NULL;     \
	} while (0)

#endif // PLATFORM_WINDOWS
