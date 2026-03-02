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
    "${PIR_ROOT_DIR}/src/platform/common/solaris")

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

# Linker configuration (ELF via LLD — direct invocation)
#
# Clang's Solaris toolchain driver unconditionally injects the native-linker
# flag "-C" (demangle) when linking through the driver. ld.lld does not
# recognise this flag:
#   ld.lld: error: unknown argument '-C'
#
# To work around this, invoke ld.lld directly instead of going through the
# clang driver. Standard ELF emulations are used (elf_i386, elf_x86_64,
# aarch64elf); these produce ELFOSABI_NONE which Solaris/illumos kernels
# accept alongside ELFOSABI_SOLARIS.
find_program(PIR_LLD_PATH ld.lld REQUIRED)
set(PIR_DIRECT_LINKER TRUE)

# Select the LLD emulation for Solaris ELFs
if(PIR_ARCH STREQUAL "x86_64")
    set(_solaris_emulation "elf_x86_64")
elseif(PIR_ARCH STREQUAL "i386")
    set(_solaris_emulation "elf_i386")
elseif(PIR_ARCH STREQUAL "aarch64")
    set(_solaris_emulation "aarch64elf")
endif()

# Override the link command to call ld.lld directly, bypassing the clang
# driver (and its unwanted -C injection).
# Use <CMAKE_LINKER> so CMake auto-quotes paths containing spaces
# (e.g. "C:/Program Files/LLVM/bin/ld.lld.exe").
set(CMAKE_LINKER "${PIR_LLD_PATH}")
set(CMAKE_CXX_LINK_EXECUTABLE
    "<CMAKE_LINKER> -m ${_solaris_emulation} <LINK_FLAGS> <OBJECTS> -o <TARGET>")

# Clear clang-driver-level link flags set by CompilerFlags.cmake
# (-nostdlib, -fno-jump-tables are driver flags; ld.lld doesn't need them —
#  -nostdlib is implicit when calling ld.lld directly, and -fno-jump-tables
#  is already encoded as a function attribute in LTO bitcode).
set(PIR_BASE_LINK_FLAGS "")

# Linker flags passed directly to ld.lld (no -Wl, prefix needed).
# Use --long-form=value syntax so each flag is a single token, avoiding
# CMake list-splitting surprises.
list(APPEND PIR_BASE_LINK_FLAGS
    --entry=entry_point
    --no-dynamic-linker
    --no-pie
    --symbol-ordering-file=${PIR_ROOT_DIR}/cmake/data/function.order.solaris
    --build-id=none
    -Map=${PIR_MAP_FILE}
)

if(PIR_BUILD_TYPE STREQUAL "release")
    list(APPEND PIR_BASE_LINK_FLAGS --strip-all --gc-sections)
endif()
