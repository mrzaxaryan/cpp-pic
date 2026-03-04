/**
 * @file embedded_function_pointer.h
 * @brief Universal Position-Independent Function Pointers
 *
 * @details Provides PC/RIP-relative function pointers that work at any memory address.
 * Uses inline assembly to compute pure relative offsets with no absolute addresses.
 *
 * Design:
 * - Unified implementation for all architectures (i386, x86_64, armv7a, aarch64)
 * - Pure PC/RIP-relative addressing computed at runtime
 * - No link-time absolute addresses
 * - Works identically in PIC blob and normal EXE modes
 * - Architecture detection via CMake-defined macros (PLATFORM_*)
 *
 * Advantages:
 * - Compile-time offset calculation by assembler
 * - No runtime architecture detection overhead
 *
 * Disassembly Examples:
 * - i386:   call/pop %eax + leal offset(%eax), %eax
 * - x86_64: leaq offset(%rip), %rax
 * - armv7a: adr r0, offset
 * - aarch64: adr x0, offset
 *
 * @ingroup core
 *
 * @defgroup embedded_func Embedded Function Pointer
 * @ingroup core
 * @{
 */

#pragma once

#include "core/types/primitives.h"

/**
 * @class EMBEDDED_FUNCTION_POINTER
 * @brief Template class for position-independent function pointer embedding
 *
 * @tparam FuncPtr Function pointer type (e.g., BOOL (*)(PVOID, CHAR))
 * @tparam Func The actual function to embed
 *
 * @details Computes PC-relative function addresses at runtime using inline assembly.
 * This eliminates the need for relocation tables and allows function pointers to
 * work correctly in position-independent shellcode.
 *
 * @par Example Usage:
 * @code
 * // Define a callback function
 * BOOL MyCallback(PVOID ctx, CHAR ch) {
 *     return true;
 * }
 *
 * // Get position-independent function pointer
 * auto callback = EMBEDDED_FUNCTION_POINTER<BOOL (*)(PVOID, CHAR), MyCallback>::Get();
 *
 * // Or use the helper macro
 * auto callback = EMBED_FUNC(MyCallback);
 * @endcode
 */
template <typename FuncPtr, FuncPtr Func>
class EMBEDDED_FUNCTION_POINTER
{
public:
	/**
	 * @brief Returns position-independent function pointer
	 * @return Function pointer computed using PC-relative addressing
	 *
	 * @details Uses inline assembly to compute PC/RIP-relative addresses.
	 * Architecture is selected at compile-time via CMake-defined macros.
	 *
	 * @par Architecture-Specific Implementation:
	 *
	 * **i386 (PLATFORM_WINDOWS_I386):**
	 * @code{.asm}
	 * call 1f          ; Push return address onto stack
	 * 1:
	 * pop %eax         ; Get current EIP in eax
	 * leal offset(%eax), %eax  ; Compute: target - label_1b + eip
	 * @endcode
	 *
	 * **x86_64:**
	 * @code{.asm}
	 * leaq offset(%rip), %rax  ; Direct RIP-relative addressing
	 * @endcode
	 *
	 * **armv7a (PLATFORM_WINDOWS_ARMV7A):**
	 * @code{.asm}
	 * ldr r0, =target   ; Load via PC-relative literal pool
	 * @endcode
	 *
	 * **aarch64:**
	 * @code{.asm}
	 * adr x0, target    ; PC-relative address (±1MB range)
	 * @endcode
	 *
	 * @note NOINLINE ensures stable addresses for PC-relative calculations.
	 * The offset is calculated by the assembler as (Func - label), which is
	 * a pure compile-time constant. No absolute addresses at runtime!
	 */
	NOINLINE static FuncPtr Get() noexcept
	{
		FuncPtr result;

#if defined(PLATFORM_WINDOWS_I386)
		// i386: Use call/pop to get EIP, then compute PC-relative offset
		__asm__ volatile(
			"call 1f\n" // Push return address onto stack
			"1:\n"
			"popl %%eax\n"                // Get current EIP in eax
			"leal %c1-1b(%%eax), %%eax\n" // Compute: target - label_1b + eip
			"movl %%eax, %0\n"            // Store result
			: "=m"(result)                // Output: result variable
			: "i"(Func)                   // Input: target (compile-time constant)
			: "eax"                       // Clobber: eax register
		);
#elif defined(PLATFORM_WINDOWS_ARMV7A)
		// armv7a: Use LDR pseudo-instruction for PC-relative addressing
		__asm__ volatile(
			"ldr %0, =%1\n" // Load address via PC-relative literal pool
			: "=r"(result)  // Output: result variable
			: "X"(Func)     // Input: target address
		);
#else
		// Fallback for other architectures: direct assignment
		// This works on most modern architectures with PIC support
		result = Func;
#endif

		return result;
	}
};

/**
 * @def EMBED_FUNC
 * @brief Helper macro for cleaner embedded function pointer syntax
 * @param func_name Name of the function to embed
 * @return Position-independent function pointer
 *
 * @par Example:
 * @code
 * // Instead of:
 * auto cb = EMBEDDED_FUNCTION_POINTER<decltype(&MyFunc), &MyFunc>::Get();
 *
 * // Use:
 * auto cb = EMBED_FUNC(MyFunc);
 * @endcode
 */
#define EMBED_FUNC(func_name) \
	EMBEDDED_FUNCTION_POINTER<decltype(&func_name), &func_name>::Get()

/** @} */ // end of embedded_func group
