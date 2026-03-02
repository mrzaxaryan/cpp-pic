# =============================================================================
# Solaris.cmake - Solaris/illumos Platform Configuration
# =============================================================================

include_guard(GLOBAL)

# Validate: Solaris supports i386, x86_64, and aarch64
if(NOT PIR_ARCH MATCHES "^(i386|x86_64|aarch64)$")
    message(FATAL_ERROR "Solaris only supports i386, x86_64, and aarch64 (got: ${PIR_ARCH})")
endif()

pir_get_target_info()
pir_filter_sources(windows linux macos uefi)

list(APPEND PIR_INCLUDE_PATHS
    "${CMAKE_SOURCE_DIR}/src/platform/common/solaris")

# Architecture-specific compiler flags
if(PIR_ARCH STREQUAL "x86_64")
    # Disable the red zone for x86_64 (same rationale as Linux/macOS/UEFI)
    list(APPEND PIR_BASE_FLAGS -mno-red-zone)
    # Clang's Solaris target defaults to -fPIC and medium code model, producing
    # R_X86_64_32 relocations that fail at link time. Force small code model
    # (matching Linux behavior) for our freestanding binary.
    list(APPEND PIR_BASE_FLAGS -fno-pic -mcmodel=small)
elseif(PIR_ARCH STREQUAL "i386")
    # No red zone on i386. Force non-PIC for freestanding binary.
    list(APPEND PIR_BASE_FLAGS -fno-pic)
elseif(PIR_ARCH STREQUAL "aarch64")
    # No red zone concept on AArch64 (no leaf function optimization zone).
    # No code model override needed for AArch64.
endif()

# Linker configuration (ELF via LLD)
# Use clang++ as the linker driver so it passes the correct
# -m elf_{i386,x86_64}_sol2 emulation to LLD, producing proper Solaris ELFs.
# Direct ld.lld invocation misses the emulation flag and produces Linux-format
# ELFs that the Solaris kernel refuses to execute (ENOEXEC).
#
# Clang's Solaris driver does not accept the '-fuse-ld=lld' shorthand, so we
# resolve the full path to ld.lld and pass it via -fuse-ld=/path/to/ld.lld.
find_program(PIR_LLD_PATH ld.lld REQUIRED)

pir_add_link_flags(
    -e,entry_point
    --no-dynamic-linker
    --no-pie
    --symbol-ordering-file=${CMAKE_SOURCE_DIR}/cmake/data/function.order.solaris
    --build-id=none
    -Map,${PIR_MAP_FILE}
)

if(PIR_BUILD_TYPE STREQUAL "release")
    pir_add_link_flags(--strip-all --gc-sections)
endif()

list(APPEND PIR_BASE_LINK_FLAGS "-fuse-ld=${PIR_LLD_PATH}")
