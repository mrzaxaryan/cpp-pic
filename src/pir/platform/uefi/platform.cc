/**
 * platform.cc - UEFI Platform Abstraction Layer Core
 *
 * Provides ExitProcess implementation for UEFI.
 */

#include "platform.h"
#include "efi_context.h"

// QEMU debug exit port (configured with -device isa-debug-exit,iobase=0xf4,iosize=0x04)
// QEMU will exit with code: (value << 1) | 1
// So to get exit code 0, we write 0 and QEMU exits with 1
// To get exit code 1, we write 1 and QEMU exits with 3
#define QEMU_DEBUG_EXIT_PORT 0xf4

/**
 * QemuDebugExit - Signal exit code to QEMU
 *
 * Writes to the QEMU isa-debug-exit port to pass exit code to host.
 * QEMU transforms exit code: host_exit = (guest_value << 1) | 1
 *
 * @param code - Exit code to pass to QEMU
 */
static VOID QemuDebugExit(UINT32 code)
{
#if defined(__x86_64__)
	// x86_64: Use outb instruction to write to I/O port
	__asm__ volatile("outb %0, %1" : : "a"((UINT8)code), "Nd"((UINT16)QEMU_DEBUG_EXIT_PORT));
#elif defined(__i386__)
	// i386: Use outb instruction (same as x86_64)
	__asm__ volatile("outb %0, %1" : : "a"((UINT8)code), "Nd"((UINT16)QEMU_DEBUG_EXIT_PORT));
#elif defined(__aarch64__)
	// aarch64: Use semihosting to exit
	// SYS_EXIT (0x18) with ADP_Stopped_ApplicationExit (0x20026)
	// Register X0 = 0x18 (SYS_EXIT), X1 = pointer to parameter block
	UINT64 params[2] = {0x20026, code}; // ADP_Stopped_ApplicationExit, exit code
	__asm__ volatile("mov x0, #0x18\n"  // SYS_EXIT
	                 "mov x1, %0\n"     // parameter block
	                 "hlt #0xf000"      // semihosting call
	                 :
	                 : "r"(params)
	                 : "x0", "x1");
#endif
}

/**
 * ExitProcess - Shutdown the system
 *
 * Signals exit code to QEMU via debug port, then uses EFI Runtime
 * Services ResetSystem() to power off the machine.
 *
 * @param code - Exit code (0 = success, non-zero = error)
 */
NO_RETURN VOID ExitProcess(USIZE code)
{
	EFI_CONTEXT *ctx = GetEfiContext();
	EFI_RUNTIME_SERVICES *rs = ctx->SystemTable->RuntimeServices;

	// Signal exit code to QEMU before shutdown
	QemuDebugExit((UINT32)code);

	// Shutdown the system
	rs->ResetSystem(EfiResetShutdown, (EFI_STATUS)code, 0, NULL);

	// Should never reach here
	__builtin_unreachable();
}
