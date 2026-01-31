/**
 * memory.cc - UEFI Memory Allocation Implementation
 *
 * Provides Allocator::AllocateMemory and ReleaseMemory using
 * EFI Boot Services AllocatePool/FreePool.
 */

#include "allocator.h"
#include "efi_context.h"

/**
 * Allocator::AllocateMemory - Allocate memory from UEFI pool
 *
 * Uses EFI_BOOT_SERVICES->AllocatePool with EfiLoaderData type.
 *
 * @param size - Number of bytes to allocate
 * @return Pointer to allocated memory, or NULL on failure
 */
PVOID Allocator::AllocateMemory(USIZE size)
{
	if (size == 0)
		return NULL;

	EFI_CONTEXT *ctx = GetEfiContext();
	EFI_BOOT_SERVICES *bs = ctx->SystemTable->BootServices;

	PVOID buffer = NULL;
	EFI_STATUS status = bs->AllocatePool(EfiLoaderData, size, &buffer);

	if (status != EFI_SUCCESS)
		return NULL;

	return buffer;
}

/**
 * Allocator::ReleaseMemory - Free memory back to UEFI pool
 *
 * Uses EFI_BOOT_SERVICES->FreePool. Note that UEFI FreePool
 * does not require the size parameter (unlike mmap).
 *
 * @param ptr  - Pointer to memory to free
 * @param size - Size hint (ignored by UEFI)
 */
VOID Allocator::ReleaseMemory(PVOID ptr, USIZE size)
{
	(VOID)size; // UEFI FreePool doesn't need size

	if (ptr == NULL)
		return;

	EFI_CONTEXT *ctx = GetEfiContext();
	EFI_BOOT_SERVICES *bs = ctx->SystemTable->BootServices;

	bs->FreePool(ptr);
}
