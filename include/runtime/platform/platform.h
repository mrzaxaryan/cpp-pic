/**
 * platform.h - Cross-Platform Definitions and Utilities
 *
 * This header provides platform-agnostic definitions used throughout the
 * CPP-PIC runtime. It abstracts the differences between Windows and Linux
 * while providing common utilities for position-independent code.
 *
 * PLATFORM SUPPORT:
 *   Windows: PEB-based module resolution, PE parsing, ntdll/kernel32 syscalls
 *   Linux:   Direct kernel syscalls via architecture-specific implementations
 *
 * ARCHITECTURE SUPPORT:
 *   - x86_64: 64-bit x86 (AMD64)
 *   - i386:   32-bit x86 (requires relocation support on Windows)
 *   - aarch64: 64-bit ARM (ARMv8-A)
 *   - armv7a:  32-bit ARM (ARMv7-A EABI)
 */

#pragma once

#include "primitives.h"
#include "uint64.h"
#include "int64.h"
#include "double.h"
#include "embedded_double.h"
#include "embedded_string.h"

/* ============================================================================
 * Platform-Specific Declarations
 * ============================================================================
 * These functions and macros are implemented differently on each platform.
 */

#if defined(PLATFORM_WINDOWS)
/* ----------------------------------------------------------------------------
 * Windows Platform
 * ----------------------------------------------------------------------------
 * Windows uses the Process Environment Block (PEB) to locate loaded modules
 * and resolve exports by hash. This allows position-independent code to
 * call Windows APIs without import tables.
 */

/**
 * GetInstructionAddress - Get current instruction pointer
 *
 * Used for position-independent code to determine the current execution
 * address at runtime.
 *
 * @return Current instruction pointer value
 */
PVOID GetInstructionAddress(VOID);

/**
 * ReversePatternSearch - Search backward for a byte pattern
 *
 * Used to locate function prologues when determining base address
 * for position-independent code relocation.
 *
 * @param ip      - Starting address to search from
 * @param pattern - Pattern to search for
 * @param len     - Length of pattern in bytes
 * @return Pointer to pattern match, or NULL if not found
 */
PCHAR ReversePatternSearch(PCHAR ip, const CHAR *pattern, UINT32 len);

/**
 * ResolveExportAddressFromPebModule - Resolve a function by hash
 *
 * Walks the PEB module list, finds a module by name hash, then resolves
 * an export by function name hash. This avoids string literals in .rdata.
 *
 * @param moduleNameHash   - DJB2 hash of module name (e.g., L"ntdll.dll")
 * @param functionNameHash - DJB2 hash of function name (e.g., "ZwTerminateProcess")
 * @return Function address, or NULL if not found
 */
PVOID ResolveExportAddressFromPebModule(USIZE moduleNameHash, USIZE functionNameHash);

/* Environment data access macros - uses PEB.SubSystemData for storage */
#define GetEnvironmentBaseAddress() (USIZE)(GetCurrentPEB()->SubSystemData)
#define SetEnvironmentBaseAddress(v) (GetCurrentPEB()->SubSystemData = (PVOID)(v))

/**
 * ENVIRONMENT_DATA - Runtime environment for PIC relocation
 *
 * On Windows i386, position-independent code may need to relocate function
 * pointers at runtime. This structure stores the information needed.
 *
 * BaseAddress:    The actual load address of _start
 * ShouldRelocate: TRUE if running as PIC blob (not normal EXE)
 */
typedef struct _ENVIRONMENT_DATA
{
    PVOID BaseAddress;      /* Actual address where code is loaded */
    BOOL ShouldRelocate;    /* TRUE if relocation is needed */
} ENVIRONMENT_DATA, *PENVIRONMENT_DATA;

#if defined(PLATFORM_WINDOWS_I386)
/**
 * Windows i386 Relocation Support
 *
 * On 32-bit Windows, when running as a PIC blob injected at an arbitrary
 * address, function pointers need to be relocated from their link-time
 * addresses to their runtime addresses.
 *
 * IMAGE_LINK_BASE: The address the code was linked at (0x401000)
 * GetEnvironmentData(): Access the environment data structure
 * Initialize(): Set up relocation info at startup
 * PerformRelocation(): Adjust a pointer from link-time to runtime address
 */
#define IMAGE_LINK_BASE ((USIZE)0x401000)
#define GetEnvironmentData() ((PENVIRONMENT_DATA)(GetCurrentPEB()->SubSystemData))

NOINLINE VOID Initialize(PENVIRONMENT_DATA envData);
PVOID PerformRelocation(PVOID p);

#else
/* Non-i386 Windows: No relocation needed (64-bit code is RIP-relative) */
#define PerformRelocation(p) (p)
#define Initialize(envData) ((VOID)envData)
#endif /* PLATFORM_WINDOWS_I386 */

#elif defined(PLATFORM_LINUX)
/* ----------------------------------------------------------------------------
 * Linux Platform
 * ----------------------------------------------------------------------------
 * Linux uses direct syscalls without library dependencies. The syscall
 * interface is provided by architecture-specific implementations.
 */

/**
 * ENVIRONMENT_DATA - Runtime environment (Linux version)
 *
 * Linux version of the environment structure. Currently not used for
 * relocation (Linux PIC uses different mechanisms), but kept for
 * API compatibility with Windows code.
 */
typedef struct _ENVIRONMENT_DATA
{
    PVOID BaseAddress;      /* Not currently used on Linux */
    BOOL ShouldRelocate;    /* Always FALSE on Linux */
} ENVIRONMENT_DATA, *PENVIRONMENT_DATA;

/* Linux: No relocation needed (uses standard ELF PIC mechanisms) */
#define PerformRelocation(p) (p)
#define Initialize(envData) ((VOID)envData)

/**
 * __syscall - Low-level syscall invocation
 *
 * This function is implemented in platform.linux.<arch>.cc for each
 * supported architecture. It provides the raw syscall mechanism.
 *
 * @param nr   - Syscall number
 * @param a1-6 - Syscall arguments (number varies by syscall)
 * @return Syscall return value (negative = -errno on error)
 *
 * Architecture implementations:
 *   x86_64:  syscall instruction
 *   i386:    int 0x80
 *   aarch64: svc #0
 *   armv7a:  svc 0 (EABI)
 */
SSIZE __syscall(SSIZE nr, USIZE a1, USIZE a2, USIZE a3, USIZE a4, USIZE a5, USIZE a6);

#endif /* PLATFORM_LINUX */

/* ============================================================================
 * Cross-Platform Declarations
 * ============================================================================
 * These declarations are available on all platforms.
 */

/**
 * ENTRYPOINT - Entry point function attribute
 */
#if defined(PLATFORM_LINUX) && defined(ARCHITECTURE_X86_64)
#define ENTRYPOINT extern "C" __attribute__((noreturn, force_align_arg_pointer))
#else
#define ENTRYPOINT extern "C" __attribute__((noreturn))
#endif


/**
 * ExitProcess - Terminate the current process
 *
 * Cross-platform process termination function. Never returns.
 *
 * @param code - Exit status code (0 = success, non-zero = error)
 *
 * Implementation:
 *   Windows: NtTerminateProcess via ntdll
 *   Linux:   exit syscall (60 on x86_64, 1 on i386, etc.)
 */
NO_RETURN VOID ExitProcess(USIZE code);

/* ============================================================================
 * Compile-Time Utilities
 * ============================================================================
 * These templates and helpers support position-independent code by
 * providing stack-based storage for data that would normally go in .rodata.
 */

/**
 * UINT_OF_SIZE - Map byte size to unsigned integer type
 *
 * Compile-time mapping from size in bytes to the corresponding unsigned
 * integer type. Used by STACK_ARRAY_STORAGE for type-safe byte manipulation.
 *
 * UINT_OF_SIZE<1>::type = UINT8
 * UINT_OF_SIZE<2>::type = UINT16
 * UINT_OF_SIZE<4>::type = UINT32
 * UINT_OF_SIZE<8>::type = UINT64
 */
template <USIZE Bytes>
struct UINT_OF_SIZE;

template <>
struct UINT_OF_SIZE<1>
{
    using type = UINT8;
};

template <>
struct UINT_OF_SIZE<2>
{
    using type = UINT16;
};

template <>
struct UINT_OF_SIZE<4>
{
    using type = UINT32;
};

template <>
struct UINT_OF_SIZE<8>
{
    using type = UINT64;
};

/**
 * STACK_ARRAY_STORAGE - Compile-time array storage in code section
 *
 * This class provides a way to embed array data (strings, byte arrays)
 * directly in the code section rather than .rodata. It stores the data
 * as an array of USIZE words, with byte-level access methods.
 *
 * TEMPLATE PARAMETERS:
 *   TChar - Element type (CHAR, WCHAR, UINT8, etc.)
 *   N     - Number of elements (including null terminator for strings)
 *
 * USAGE:
 *   constexpr auto msg = STACK_ARRAY_STORAGE<CHAR, 6>("Hello");
 *   for (USIZE i = 0; i < msg.Count; i++)
 *       putchar(msg[i]);
 */
template <typename TChar, USIZE N>
class STACK_ARRAY_STORAGE
{
public:
    static constexpr USIZE Count = N;                          /* Element count */
    static constexpr USIZE SizeBytes = N * sizeof(TChar);      /* Total bytes */

private:
    static constexpr USIZE WordBytes = sizeof(USIZE);          /* Bytes per word */
    static constexpr USIZE WordCount = (SizeBytes + WordBytes - 1) / WordBytes;

    alignas(USIZE) USIZE words[WordCount]{};                   /* Packed storage */

    /**
     * SetByte - Set a single byte in the packed storage (consteval)
     *
     * Used during compile-time initialization to pack bytes into words.
     */
    consteval VOID SetByte(USIZE byteIndex, UINT8 v)
    {
        const USIZE wi = byteIndex / WordBytes;                /* Word index */
        const USIZE sh = (byteIndex % WordBytes) * 8u;         /* Bit shift */

        const USIZE mask = (USIZE)0xFFu << sh;
        words[wi] = (words[wi] & ~mask) | ((USIZE)v << sh);
    }

    /**
     * GetByte - Get a single byte from the packed storage (constexpr)
     *
     * Used at runtime to extract bytes from words.
     */
    consteval UINT8 GetByte(USIZE byteIndex) const
    {
        const USIZE wi = byteIndex / WordBytes;
        const USIZE sh = (byteIndex % WordBytes) * 8u;
        return (UINT8)((words[wi] >> sh) & (USIZE)0xFFu);
    }

public:
    /**
     * Constructor - Pack array into word storage at compile-time
     *
     * Iterates through source array, converting each element to bytes
     * and packing them into the words array.
     */
    consteval STACK_ARRAY_STORAGE(const TChar (&src)[N]) : words{}
    {
        using U = typename UINT_OF_SIZE<sizeof(TChar)>::type;

        for (USIZE i = 0; i < N; ++i)
        {
            const U v = (U)src[i];

            /* Pack each byte of the element */
            for (USIZE b = 0; b < sizeof(TChar); ++b)
            {
                const UINT8 data = (UINT8)((v >> (b * 8u)) & (U)0xFFu);
                SetByte(i * sizeof(TChar) + b, data);
            }
        }
    }

    /**
     * operator[] - Access element by index at runtime
     *
     * Reconstructs the element from packed bytes.
     */
    constexpr TChar operator[](USIZE index) const
    {
        using U = typename UINT_OF_SIZE<sizeof(TChar)>::type;

        U v = 0;
        const USIZE base = index * sizeof(TChar);

        for (USIZE b = 0; b < sizeof(TChar); ++b)
            v |= (U)GetByte(base + b) << (b * 8u);

        return (TChar)v;
    }

    /**
     * operator const VOID* - Get raw pointer to storage
     */
    constexpr operator const VOID *() const
    {
        return (const VOID *)words;
    }

    /**
     * Words - Get pointer to underlying word array
     */
    constexpr const USIZE *Words() const { return words; }

    static constexpr USIZE WordsCount = WordCount;
};

/**
 * MakeArrayStorage - Helper to create STACK_ARRAY_STORAGE
 *
 * Deduces template parameters from the source array.
 *
 * Usage:
 *   constexpr auto storage = MakeArrayStorage("Hello");
 */
template <typename TChar, USIZE N>
consteval auto MakeArrayStorage(const TChar (&arr)[N])
{
    return STACK_ARRAY_STORAGE<TChar, N>(arr);
}
