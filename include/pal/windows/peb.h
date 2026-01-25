#if defined(PLATFORM_WINDOWS)

#pragma once

#include "windows_types.h"

#define CONTAINING_RECORD(address, type, field) ((type *)((PCHAR)(address) - (USIZE)(&((type *)0)->field)))

// Macro to get the containing record from a field pointer

// List entry structure for linked lists
typedef struct _LIST_ENTRY
{
    struct _LIST_ENTRY *Flink;
    struct _LIST_ENTRY *Blink;
} LIST_ENTRY, *PLIST_ENTRY;

// Loader module structure
typedef struct _LDR_DATA_TABLE_ENTRY
{
    LIST_ENTRY InLoadOrderModuleList;
    LIST_ENTRY InMemoryOrderModuleList;
    LIST_ENTRY InInitializationOrderModuleList;
    PVOID DllBase;
    PVOID EntryPoint;
    UINT32 SizeOfImage;
    UNICODE_STRING FullDllName;
    UNICODE_STRING BaseDllName;
    UINT32 Flags;
    INT16 LoadCount;
    INT16 TlsIndex;
    LIST_ENTRY HashTableEntry;
    UINT32 TimeDateStamp;
} LDR_DATA_TABLE_ENTRY, *PLDR_DATA_TABLE_ENTRY;

// Process Environment Block Loader Data
typedef struct _PEB_LDR_DATA
{
    UINT32 Length;
    UINT32 Initialized;
    PVOID SsHandle;
    LIST_ENTRY InLoadOrderModuleList;
    LIST_ENTRY InMemoryOrderModuleList;
    LIST_ENTRY InInitializationOrderModuleList;
} PEB_LDR_DATA, *PPEB_LDR_DATA;

// Process parameters structure
typedef struct _RTL_USER_PROCESS_PARAMETERS
{
    UINT32 MaximumLength;
    UINT32 Length;
    UINT32 Flags;
    UINT32 DebugFlags;
    PVOID ConsoleHandle;
    UINT32 ConsoleFlags;
    PVOID StandardInput;
    PVOID StandardOutput;
    PVOID StandardError;
} RTL_USER_PROCESS_PARAMETERS, *PRTL_USER_PROCESS_PARAMETERS;

// Process Environment Block
typedef struct _PEB
{
    UINT8 InheritedAddressSpace;
    UINT8 ReadImageFileExecOptions;
    UINT8 BeingDebugged;
    UINT8 Spare;
    PVOID Mutant;
    PVOID ImageBase;
    PPEB_LDR_DATA LoaderData;
    PRTL_USER_PROCESS_PARAMETERS ProcessParameters;
    PVOID SubSystemData;
    PVOID ProcessHeap;
} PEB, *PPEB;

// Function to get the current process's PEB pointer
PPEB GetCurrentPEB(VOID);
// Function to resolve module handle by its name
PVOID GetModuleHandleFromPEB(USIZE moduleNameHash);

#else
#error Unsupported platform
#endif