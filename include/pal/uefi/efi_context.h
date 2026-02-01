/**
 * efi_context.h - EFI Runtime Context
 *
 * Provides storage and access to EFI runtime context.
 * The ImageHandle and SystemTable are stored here after efi_main
 * receives them and can be accessed by all PAL functions.
 */

#pragma once

#include "efi_system_table.h"

// =============================================================================
// EFI Context Structure
// =============================================================================

struct EFI_CONTEXT
{
	EFI_HANDLE ImageHandle;
	EFI_SYSTEM_TABLE *SystemTable;
	BOOL NetworkInitialized;
	BOOL DhcpConfigured;
	BOOL TcpStackReady;
};

// =============================================================================
// Context Register Access (GS on x86_64, TPIDR_EL0 on aarch64)
// =============================================================================

// IA32_GS_BASE MSR number for x86_64
#if defined(ARCHITECTURE_X86_64)
inline constexpr UINT32 IA32_GS_BASE = 0xC0000101;
#endif

/**
 * SetEfiContextRegister - Store context pointer in CPU register
 *
 * Uses architecture-specific registers to store the context pointer:
 * - x86_64: GS base via MSR (IA32_GS_BASE = 0xC0000101)
 * - aarch64: TPIDR_EL0 (thread pointer register)
 *
 * Note: We use WRMSR instead of WRGSBASE because WRGSBASE requires
 * CR4.FSGSBASE to be enabled, which UEFI firmware may not set.
 * WRMSR works in ring 0 (where UEFI apps run) without this requirement.
 *
 * @param ctx Pointer to the EFI context to store
 */
inline void SetEfiContextRegister(EFI_CONTEXT *ctx)
{
#if defined(ARCHITECTURE_X86_64)
	UINT64 value = reinterpret_cast<UINT64>(ctx);
	UINT32 low = static_cast<UINT32>(value);
	UINT32 high = static_cast<UINT32>(value >> 32);
	__asm__ volatile(
		"wrmsr"
		:
		: "c"(IA32_GS_BASE), "a"(low), "d"(high)
		: "memory");
#elif defined(ARCHITECTURE_AARCH64)
	__asm__ volatile("msr tpidr_el0, %0" : : "r"(ctx) : "memory");
#endif
}

/**
 * GetEfiContext - Get the EFI context from CPU register
 *
 * Retrieves the context pointer from architecture-specific register:
 * - x86_64: GS base via MSR (IA32_GS_BASE = 0xC0000101)
 * - aarch64: TPIDR_EL0 (thread pointer register)
 *
 * @return Pointer to the EFI context
 */
inline EFI_CONTEXT *GetEfiContext()
{
#if defined(ARCHITECTURE_X86_64)
	UINT32 low, high;
	__asm__ volatile(
		"rdmsr"
		: "=a"(low), "=d"(high)
		: "c"(IA32_GS_BASE));
	return reinterpret_cast<EFI_CONTEXT *>((static_cast<UINT64>(high) << 32) | low);
#elif defined(ARCHITECTURE_AARCH64)
	EFI_CONTEXT *ctx;
	__asm__ volatile("mrs %0, tpidr_el0" : "=r"(ctx));
	return ctx;
#endif
}
