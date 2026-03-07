# =============================================================================
# Solaris.cmake - Solaris/illumos Platform Configuration
# =============================================================================

include_guard(GLOBAL)

# Validate: Solaris supports i386, x86_64, and aarch64
if(NOT PIR_ARCH MATCHES "^(i386|x86_64|aarch64)$")
    message(FATAL_ERROR "Solaris only supports i386, x86_64, and aarch64 (got: ${PIR_ARCH})")
endif()

pir_get_target_info()
pir_filter_sources(windows linux macos uefi freebsd android)

list(APPEND PIR_INCLUDE_PATHS
    "${PIR_ROOT_DIR}/src/platform/common/solaris")

# Architecture-specific compiler flags
if(PIR_ARCH STREQUAL "x86_64")
    # Disable the red zone for x86_64 (same rationale as Linux/macOS/UEFI)
    list(APPEND PIR_BASE_FLAGS -mno-red-zone)
    # Clang's Solaris target defaults to -fPIC and medium code model. Instead of
    # disabling PIC entirely (which causes LTO code generation issues at -O1+),
    # use -fdirect-access-external-data to force direct PC-relative access for
    # all data symbols while keeping PIC enabled. Force small code model so
    # 32-bit displacements are used (matching Linux behavior).
    list(APPEND PIR_BASE_FLAGS -fdirect-access-external-data -mcmodel=small)
    # Force hidden visibility to prevent GOT/PLT entries for interposable symbols.
    list(APPEND PIR_BASE_FLAGS -fvisibility=hidden)
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
# aarch64elf) because LLD does not support Solaris-specific _sol2 emulation
# variants. These produce ELFOSABI_NONE, which Solaris kernels reject with
# "Exec format error". A post-build step patches EI_OSABI to
# ELFOSABI_SOLARIS (6) — see PIR_ELF_OSABI below.
# Locate ld.lld next to the C++ compiler (same LLVM installation).
# Resolve symlinks (e.g. /usr/bin/clang++ -> /opt/llvm/bin/clang++) to find
# the real LLVM bin directory where ld.lld lives.
get_filename_component(_llvm_bin_dir "${CMAKE_CXX_COMPILER}" REALPATH)
get_filename_component(_llvm_bin_dir "${_llvm_bin_dir}" DIRECTORY)
find_program(PIR_LLD_PATH ld.lld HINTS "${_llvm_bin_dir}" REQUIRED)
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
#
# A linker script with an explicit PHDRS command
# (cmake/data/linker.solaris.ld) controls which program headers LLD
# emits. Only the segments listed in PHDRS are created, so PT_PHDR
# and PT_GNU_STACK are never generated:
#   - PT_PHDR: Solaris kernel rejects static binaries with PT_PHDR but
#     no PT_INTERP (if (uphdr && !intphdr) goto bad; → ENOEXEC).
#   - PT_GNU_STACK: OS-specific type 0x6474e551 that Solaris rejects
#     when EI_OSABI is ELFOSABI_SOLARIS.
set(CMAKE_LINKER "${PIR_LLD_PATH}")
set(CMAKE_CXX_LINK_EXECUTABLE
    "<CMAKE_LINKER> -m ${_solaris_emulation} -T ${PIR_ROOT_DIR}/cmake/data/linker.solaris.ld <LINK_FLAGS> <OBJECTS> -o <TARGET>")

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
    --symbol-ordering-file=${PIR_ROOT_DIR}/cmake/data/function.order.solaris
    --build-id=none
    -Map=${PIR_MAP_FILE}
)

# x86 requires --pie: with -flto=full the LTO code generator runs at link time
# and selects its relocation model based on the output type. A non-PIE
# executable causes the x86 backend to emit absolute addressing (movl
# $0xNNNNNN, %reg) instead of RIP-relative (leaq offset(%rip), %reg) for
# references to data embedded in .text. These absolute addresses are resolved at
# the link-time VMA and break when the PIC binary is loaded at a different
# address. --pie produces a PIE (ET_DYN) executable, which forces the LTO
# backend to generate position-independent code. AArch64 is unaffected because
# that ISA always uses PC-relative addressing.
if(PIR_ARCH STREQUAL "x86_64")
    list(APPEND PIR_BASE_LINK_FLAGS --pie)
else()
    list(APPEND PIR_BASE_LINK_FLAGS --no-pie)
endif()

if(PIR_BUILD_TYPE STREQUAL "release")
    list(APPEND PIR_BASE_LINK_FLAGS --strip-all --gc-sections)
endif()

# ELFOSABI patch — LLD produces ELFOSABI_NONE; Solaris requires
# ELFOSABI_SOLARIS (6). PostBuild.cmake patches this after linking.
set(PIR_ELF_OSABI 6)
