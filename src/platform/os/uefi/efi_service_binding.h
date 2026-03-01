/**
 * efi_service_binding.h - EFI Service Binding Protocol
 *
 * Defines the generic service binding protocol used to create
 * and destroy child handles for network protocols.
 */

#pragma once

#include "efi_types.h"

// =============================================================================
// Service Binding Protocol
// =============================================================================

typedef struct EFI_SERVICE_BINDING_PROTOCOL EFI_SERVICE_BINDING_PROTOCOL;

typedef EFI_STATUS(EFIAPI *EFI_SERVICE_BINDING_CREATE_CHILD)(
	EFI_SERVICE_BINDING_PROTOCOL *This,
	EFI_HANDLE *ChildHandle);

typedef EFI_STATUS(EFIAPI *EFI_SERVICE_BINDING_DESTROY_CHILD)(
	EFI_SERVICE_BINDING_PROTOCOL *This,
	EFI_HANDLE ChildHandle);

struct EFI_SERVICE_BINDING_PROTOCOL
{
	EFI_SERVICE_BINDING_CREATE_CHILD CreateChild;
	EFI_SERVICE_BINDING_DESTROY_CHILD DestroyChild;
};
