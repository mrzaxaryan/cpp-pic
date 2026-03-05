# =============================================================================
# FreeBSD.cmake - FreeBSD Platform Configuration
# =============================================================================

include_guard(GLOBAL)

# Validate: FreeBSD supports i386, x86_64, aarch64, and riscv64
if(NOT PIR_ARCH MATCHES "^(i386|x86_64|aarch64|riscv64)$")
    message(FATAL_ERROR "FreeBSD only supports i386, x86_64, aarch64, and riscv64 (got: ${PIR_ARCH})")
endif()

pir_get_target_info()
pir_filter_sources(windows linux macos uefi solaris)

list(APPEND PIR_INCLUDE_PATHS
    "${PIR_ROOT_DIR}/src/platform/common/freebsd")

# Architecture-specific compiler flags
if(PIR_ARCH STREQUAL "x86_64")
    # Disable the red zone for x86_64 (same rationale as Linux/macOS/Solaris)
    list(APPEND PIR_BASE_FLAGS -mno-red-zone)
endif()

# Prevent GOT indirection. Clang's FreeBSD target defaults to PIC, producing
# GOT-relative relocations for data that require a loader-initialized GOT.
# Instead of disabling PIC entirely (which causes LTO code generation issues at
# -O1+), use -fdirect-access-external-data to force direct PC-relative access
# for all data symbols while keeping PIC enabled for code. This matches the
# approach used on macOS. Combined with -fvisibility=hidden (which prevents
# GOT/PLT entries for all internal symbols), the binary has zero GOT references.
list(APPEND PIR_BASE_FLAGS -fdirect-access-external-data)

# Force hidden visibility in all build types (release already gets this from
# CompilerFlags.cmake; debug builds need it explicitly). Hidden visibility
# prevents the linker from generating GOT/PLT entries for interposable symbols,
# which is essential since the PIC binary has no dynamic linker to populate them.
list(APPEND PIR_BASE_FLAGS -fvisibility=hidden)

# Disable stack protector — Clang enables -fstack-protector by default for
# FreeBSD targets (>= 13.0). The canary read (%fs:0x28 on x86_64) requires
# CRT-initialized TLS, which is unavailable in this freestanding binary.
list(APPEND PIR_BASE_FLAGS -fno-stack-protector)

# Linker configuration (ELF via LLD through clang driver)
pir_add_link_flags(
    -e,entry_point
    --no-dynamic-linker
    --no-pie
    --symbol-ordering-file=${PIR_ROOT_DIR}/cmake/data/function.order.freebsd
    --build-id=none
    -Map,${PIR_MAP_FILE}
)

if(PIR_BUILD_TYPE STREQUAL "release")
    pir_add_link_flags(--strip-all --gc-sections)
endif()

# RISC-V 64: merge .rodata (LTO constant pools) into .text so the
# PIC binary contains the constant-pool data that auipc+ld references.
if(PIR_ARCH STREQUAL "riscv64")
    pir_add_link_flags(-T,${PIR_ROOT_DIR}/cmake/data/linker.riscv64.ld)
endif()

list(APPEND PIR_BASE_LINK_FLAGS -fuse-ld=lld)

# ELFOSABI patch — LLD produces ELFOSABI_NONE; FreeBSD requires
# ELFOSABI_FREEBSD (9). PostBuild.cmake patches this after linking.
set(PIR_ELF_OSABI 9)
