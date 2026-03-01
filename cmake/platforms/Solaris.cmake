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
    "${CMAKE_SOURCE_DIR}/src/platform/os/solaris"
    "${CMAKE_SOURCE_DIR}/src/platform/os/solaris/common")

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

# Clang's Solaris driver doesn't support -fuse-ld=lld or --ld-path, so invoke
# LLD directly for linking. The input objects carry the correct Solaris ELF
# attributes from compilation, and LLD infers the output format from them.
find_program(PIR_LLD ld.lld REQUIRED)
set(PIR_DIRECT_LINKER TRUE)
set(CMAKE_CXX_LINK_EXECUTABLE "\"${PIR_LLD}\" <LINK_FLAGS> <OBJECTS> -o <TARGET>")

# Reset linker flags: compiler-driver flags (-nostdlib, -fno-jump-tables, -target)
# are not valid when invoking LLD directly. The -fno-jump-tables attribute is
# already embedded in the LTO bitcode objects from compilation, so LLD's LTO
# pipeline honors it without any extra flag.
set(PIR_BASE_LINK_FLAGS "")

# Linker flags (no -Wl, prefix needed for direct LLD invocation)
list(APPEND PIR_BASE_LINK_FLAGS
    -e entry_point
    --no-dynamic-linker
    --no-pie
    --symbol-ordering-file=${CMAKE_SOURCE_DIR}/cmake/data/function.order.solaris
    --build-id=none
    -Map=${PIR_MAP_FILE}
)

if(PIR_BUILD_TYPE STREQUAL "release")
    list(APPEND PIR_BASE_LINK_FLAGS --strip-all --gc-sections)
endif()
