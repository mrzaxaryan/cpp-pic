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

if(PIR_BUILD_TYPE STREQUAL "release")
    pir_add_link_flags(-dead_strip)
endif()

# macOS uses ld64.lld via target triple (no explicit -fuse-ld=lld)
