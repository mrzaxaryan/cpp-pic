#pragma once

#include "uefi_types.h"

// Reset types for ResetSystem
typedef enum
{
    EfiResetCold,
    EfiResetWarm,
    EfiResetShutdown,
    EfiResetPlatformSpecific
} EFI_RESET_TYPE;

// Runtime Services function pointer types
typedef VOID(EFIAPI *EFI_RESET_SYSTEM)(
    EFI_RESET_TYPE ResetType,
    EFI_STATUS ResetStatus,
    UINTN DataSize,
    PVOID ResetData);

// EFI Runtime Services Table
struct _EFI_RUNTIME_SERVICES
{
    EFI_TABLE_HEADER Hdr;

    // Time Services
    PVOID GetTime;
    PVOID SetTime;
    PVOID GetWakeupTime;
    PVOID SetWakeupTime;

    // Virtual Memory Services
    PVOID SetVirtualAddressMap;
    PVOID ConvertPointer;

    // Variable Services
    PVOID GetVariable;
    PVOID GetNextVariableName;
    PVOID SetVariable;

    // Miscellaneous Services
    PVOID GetNextHighMonotonicCount;
    EFI_RESET_SYSTEM ResetSystem;

    // UEFI 2.0 Capsule Services
    PVOID UpdateCapsule;
    PVOID QueryCapsuleCapabilities;

    // Miscellaneous UEFI 2.0 Services
    PVOID QueryVariableInfo;
};
