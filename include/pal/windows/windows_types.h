#pragma once

#include "primitives.h"

// NTSTATUS type definition
typedef INT32 NTSTATUS;

// Structure for async I/O operations
typedef struct _OVERLAPPED
{
	USIZE Internal;
	USIZE InternalHigh;
	union
	{
		struct
		{
			UINT32 Offset;	   // low-order 32-bits of the file pointer
			UINT32 OffsetHigh; // high-order 32-bits of the file pointer
		} DUMMYSTRUCTNAME;
		PVOID Pointer;
	} DUMMYUNIONNAME;

	PVOID hEvent;
} OVERLAPPED, *LPOVERLAPPED;

// Unicode string structure
typedef struct _UNICODE_STRING
{
    UINT16 Length;
    UINT16 MaximumLength;
    PWCHAR Buffer;
} UNICODE_STRING, *PUNICODE_STRING;

