#pragma once

#include "uefi_types.h"
#include "efi_boot_services.h"
#include "efi_runtime_services.h"
#include "efi_simple_text_output.h"

// EFI System Table signature
#define EFI_SYSTEM_TABLE_SIGNATURE 0x5453595320494249ULL // "IBI SYST"

// Simple Text Input Protocol (stub - not fully implemented)
typedef struct _EFI_SIMPLE_TEXT_INPUT_PROTOCOL
{
    PVOID Reset;
    PVOID ReadKeyStroke;
    EFI_EVENT WaitForKey;
} EFI_SIMPLE_TEXT_INPUT_PROTOCOL;

// Configuration Table
typedef struct _EFI_CONFIGURATION_TABLE
{
    EFI_GUID VendorGuid;
    PVOID VendorTable;
} EFI_CONFIGURATION_TABLE;

// EFI System Table
struct _EFI_SYSTEM_TABLE
{
    EFI_TABLE_HEADER Hdr;
    WCHAR *FirmwareVendor;
    UINT32 FirmwareRevision;
    EFI_HANDLE ConsoleInHandle;
    EFI_SIMPLE_TEXT_INPUT_PROTOCOL *ConIn;
    EFI_HANDLE ConsoleOutHandle;
    EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL *ConOut;
    EFI_HANDLE StandardErrorHandle;
    EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL *StdErr;
    EFI_RUNTIME_SERVICES *RuntimeServices;
    EFI_BOOT_SERVICES *BootServices;
    UINTN NumberOfTableEntries;
    EFI_CONFIGURATION_TABLE *ConfigurationTable;
};

// Global system table pointer (set during initialization)
extern EFI_SYSTEM_TABLE *gST;
extern EFI_BOOT_SERVICES *gBS;
extern EFI_HANDLE gImageHandle;
