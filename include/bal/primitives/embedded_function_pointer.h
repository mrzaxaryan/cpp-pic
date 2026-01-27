#pragma once

#include "primitives.h"

/**
 * EMBEDDED_FUNCTION_POINTER - Universal position-independent function pointers
 *
 * Provides PC/RIP-relative function pointers that work at any memory address.
 * Uses inline assembly to compute pure relative offsets with no absolute addresses.
 *
 * DESIGN:
 *   - Unified implementation for all architectures (i386, x86_64, armv7a, aarch64)
 *   - Pure PC/RIP-relative addressing computed at runtime
 *   - No link-time absolute addresses
 *   - Works identically in PIC blob and normal EXE modes
 *   - Architecture detection via CMake-defined macros (ARCHITECTURE_*)
 *
 * ADVANTAGES:
 *   - Compile-time offset calculation by assembler
 *   - No runtime architecture detection overhead
 *
 * DISASSEMBLY EXAMPLES:
 *   i386:   call/pop %eax + leal offset(%eax), %eax
 *   x86_64: leaq offset(%rip), %rax
 *   armv7a: adr r0, offset
 *   aarch64: adr x0, offset
 *
 * USAGE:
 *   auto callback = EMBEDDED_FUNCTION_POINTER<BOOL (*)(PVOID, CHAR), MyCallback>::Get();
 *   // or use the helper macro:
 *   auto callback = EMBED_FUNC(MyCallback);
 */

template<typename FuncPtr, FuncPtr Func>
class EMBEDDED_FUNCTION_POINTER
{
public:
    /**
     * Get() - Returns position-independent function pointer
     *
     * Uses inline assembly to compute PC/RIP-relative addresses.
     * Architecture is selected at compile-time via CMake-defined macros.
     *
     * i386 (ARCHITECTURE_I386):
     *   - call 1f / pop %eax        - Get current EIP
     *   - leal offset(%eax), %eax   - Add PC-relative offset
     *   Example: leal 0x2757(%eax), %eax
     *
     * x86_64 (ARCHITECTURE_X86_64):
     *   - leaq offset(%rip), %rax   - Direct RIP-relative addressing
     *   Example: leaq 0x1234(%rip), %rax
     *
     * armv7a (ARCHITECTURE_ARMV7A):
     *   - adr r0, target            - PC-relative address (±1KB range)
     *   Example: adr r0, #0x1234
     *
     * aarch64 (ARCHITECTURE_AARCH64):
     *   - adr x0, target            - PC-relative address (±1MB range)
     *   Example: adr x0, #0x1234
     *
     * The offset is calculated by the assembler as (Func - label), which is
     * a pure compile-time constant. No absolute addresses at runtime!
     *
     * NOINLINE ensures stable addresses for PC-relative calculations
     */
    NOINLINE static FuncPtr Get() noexcept
    {
        FuncPtr result;

#if defined(ARCHITECTURE_I386)
        // i386: Use call/pop to get EIP, then compute PC-relative offset
        __asm__ volatile (
            "call 1f\n"                         // Push return address onto stack
            "1:\n"
            "popl %%eax\n"                      // Get current EIP in eax
            "leal %c1-1b(%%eax), %%eax\n"       // Compute: target - label_1b + eip
            "movl %%eax, %0\n"                  // Store result
            : "=m"(result)                      // Output: result variable
            : "i"(Func)                         // Input: target (compile-time constant)
            : "eax"                             // Clobber: eax register
        );
#elif defined(ARCHITECTURE_X86_64)
        // x86_64: Use native RIP-relative addressing
        __asm__ volatile (
            "leaq %c1(%%rip), %%rax\n"          // Load PC-relative address
            "movq %%rax, %0\n"                  // Store result
            : "=m"(result)                      // Output: result variable
            : "i"(Func)                         // Input: target (compile-time constant)
            : "rax"                             // Clobber: rax register
        );
#elif defined(ARCHITECTURE_AARCH64)
        // aarch64: Use ADR instruction for PC-relative addressing
        __asm__ volatile (
            "adr %0, %1\n"                      // Load PC-relative address
            : "=r"(result)                      // Output: result variable
            : "i"(Func)                         // Input: target (compile-time constant)
        );
#elif defined(ARCHITECTURE_ARMV7A)
        // armv7a: Use ADR instruction for PC-relative addressing
        __asm__ volatile (
            "adr %0, %1\n"                      // Load PC-relative address
            : "=r"(result)                      // Output: result variable
            : "i"(Func)                         // Input: target (compile-time constant)
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
 * Helper macro for cleaner syntax
 *
 * Usage:
 *   auto callback = EMBED_FUNC(MyCallback);
 */
#define EMBED_FUNC(func_name) \
    EMBEDDED_FUNCTION_POINTER<decltype(&func_name), &func_name>::Get()
