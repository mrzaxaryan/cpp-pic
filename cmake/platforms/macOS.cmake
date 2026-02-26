# =============================================================================
# macOS.cmake - macOS Platform Configuration
# =============================================================================

include_guard(GLOBAL)

# Validate: macOS only supports x86_64 and aarch64
if(NOT PIR_ARCH MATCHES "^(x86_64|aarch64)$")
    message(FATAL_ERROR "macOS only supports x86_64 and aarch64 (got: ${PIR_ARCH})")
endif()

pir_get_target_info()
pir_filter_sources(windows linux uefi)

list(APPEND PIR_INCLUDE_PATHS "${CMAKE_SOURCE_DIR}/include/platform/macos")

# macOS-specific compiler flags
list(APPEND PIR_BASE_FLAGS -fno-stack-protector)

# Prevent GOT indirection. macOS enforces PIC, so by default the compiler may
# emit GOT-relative relocations instead of direct PC-relative accesses:
#   x86_64:  mov sym@GOTPCREL(%rip), %rax  →  lea sym(%rip), %rax
#   aarch64: adrp+ldr via GOT page          →  adrp+add direct
# The linker materializes these into a __DATA_CONST,__got section — a synthetic
# section that CANNOT be merged into __TEXT,__text via -rename_section. Since
# the PIC loader only maps __TEXT,__text, any GOT reference hits unmapped memory
# and crashes (SIGSEGV). This flag forces direct PC-relative access for all
# data symbols on both architectures.
list(APPEND PIR_BASE_FLAGS -fdirect-access-external-data)

# Force hidden visibility in all build types. ld64.lld does not implement the
# -static flag, so without hidden visibility the linker treats weak symbols
# (e.g. template explicit instantiations like SHABase<SHA256Traits>) as
# interposable and generates __TEXT,__stubs + __DATA,__la_symbol_ptr sections.
# The stubs load pointers from __DATA, which is not mapped by the PIC loader,
# causing a crash on any call through a stub. Hidden visibility tells both the
# compiler and linker that symbols cannot be interposed, eliminating stubs.
# (Release builds already get this from CompilerFlags.cmake; debug builds need
# it explicitly here.)
list(APPEND PIR_BASE_FLAGS -fvisibility=hidden)

if(PIR_ARCH STREQUAL "x86_64")
    # Disable the red zone for x86_64. The System V ABI allows leaf functions to
    # use 128 bytes below RSP without adjusting RSP (the "red zone"). PIR is
    # designed as position-independent shellcode that executes syscalls via inline
    # asm. When injected into arbitrary contexts the red zone may not be available
    # (signal handlers, exception contexts, foreign stacks). At -O1+ with LTO the
    # optimizer may inline System::Call into what becomes a leaf function, placing
    # syscall buffers in the red zone. This flag is already used for UEFI x86_64
    # (where interrupts clobber the red zone) and is standard practice for
    # freestanding / position-independent code that makes direct syscalls.
    list(APPEND PIR_BASE_FLAGS -mno-red-zone)

    # Disable the LLVM machine outliner for x86_64. At -Os/-Oz the machine
    # outliner extracts common instruction sequences into shared functions called
    # via CALL rel32. While these calls are RIP-relative, the outliner runs during
    # LTO code generation and can interact with constant pool placement in ways
    # that break position-independence: outlined sequences containing RIP-relative
    # loads from constant pools get relocated, changing the offset to the pool
    # entry that was resolved for the original call site. On aarch64 this is not
    # an issue because literal pools are placed inline within __TEXT,__text.
    # The flag must be on both the compile and link command lines so that it
    # reaches the LTO backend (clang forwards -mllvm to the LTO code generator).
    if(PIR_BUILD_TYPE STREQUAL "release")
        list(APPEND PIR_BASE_FLAGS "SHELL:-mllvm -enable-machine-outliner=never")
        list(APPEND PIR_BASE_LINK_FLAGS "SHELL:-mllvm -enable-machine-outliner=never")
    endif()
elseif(PIR_ARCH STREQUAL "aarch64")
    list(APPEND PIR_BASE_FLAGS -mstack-probe-size=0)
endif()

# Linker configuration (ld64.lld / Mach-O)
pir_add_link_flags(
    -e,_entry_point
    -static
    -platform_version,macos,11.0.0,11.0.0
    -order_file,${CMAKE_SOURCE_DIR}/cmake/data/function.order.macos
    -map,${PIR_MAP_FILE}
)

# Merge constant pool sections into __TEXT,__text.
# At -Os/-Oz the compiler may place constants (float literals, merged constants,
# SIMD shuffle masks) into __TEXT,__const or __TEXT,__literal* sections. These
# are NOT extracted into output.bin (only __TEXT,__text is), so any PC-relative
# reference from code to these sections crashes the PIC loader. Renaming them
# into __TEXT,__text ensures they are included in the flat binary.
# Note: The register barriers in embedded_array.h prevent SOME constant pool
# emissions, but cannot cover every path the optimiser may take at -Os/-Oz
# with LTO. These linker flags are the robust, final safety net.
pir_add_link_flags(
    -rename_section,__TEXT,__const,__TEXT,__text
    -rename_section,__TEXT,__literal4,__TEXT,__text
    -rename_section,__TEXT,__literal8,__TEXT,__text
    -rename_section,__TEXT,__literal16,__TEXT,__text
    -rename_section,__TEXT,__literal32,__TEXT,__text
    -rename_section,__TEXT,__cstring,__TEXT,__text
)

if(PIR_BUILD_TYPE STREQUAL "release")
    pir_add_link_flags(-dead_strip)
endif()

# macOS uses ld64.lld via target triple (no explicit -fuse-ld=lld)
