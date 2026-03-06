# =============================================================================
# OpenBSD.cmake - OpenBSD Platform Configuration
# =============================================================================

include_guard(GLOBAL)

# Validate: OpenBSD supports i386, x86_64, aarch64, and riscv64
if(NOT PIR_ARCH MATCHES "^(i386|x86_64|aarch64|riscv64)$")
    message(FATAL_ERROR "OpenBSD only supports i386, x86_64, aarch64, and riscv64 (got: ${PIR_ARCH})")
endif()

pir_get_target_info()
pir_filter_sources(windows linux macos uefi solaris freebsd)

list(APPEND PIR_INCLUDE_PATHS
    "${PIR_ROOT_DIR}/src/platform/common/openbsd")

# Architecture-specific compiler flags
if(PIR_ARCH STREQUAL "x86_64")
    # Disable the red zone for x86_64 (same rationale as Linux/macOS/Solaris)
    list(APPEND PIR_BASE_FLAGS -mno-red-zone)
endif()

# Prevent GOT indirection. Clang's OpenBSD target defaults to PIC, producing
# GOT-relative relocations for data that require a loader-initialized GOT.
# Instead of disabling PIC entirely (which causes LTO code generation issues at
# -O1+), use -fdirect-access-external-data to force direct PC-relative access
# for all data symbols while keeping PIC enabled for code. Combined with
# -fvisibility=hidden (which prevents GOT/PLT entries for all internal
# symbols), the binary has zero GOT references.
list(APPEND PIR_BASE_FLAGS -fdirect-access-external-data)

# Force hidden visibility in all build types (release already gets this from
# CompilerFlags.cmake; debug builds need it explicitly). Hidden visibility
# prevents the linker from generating GOT/PLT entries for interposable symbols,
# which is essential since the PIC binary has no dynamic linker to populate them.
list(APPEND PIR_BASE_FLAGS -fvisibility=hidden)

# Disable stack protector — Clang enables -fstack-protector by default for
# OpenBSD targets. The canary read requires CRT-initialized TLS, which is
# unavailable in this freestanding binary.
list(APPEND PIR_BASE_FLAGS -fno-stack-protector)

# Linker configuration (ELF via LLD through clang driver)
# OpenBSD enforces EXEC_ELF_ONLY_PIE on amd64/aarch64/riscv64 — the kernel
# only accepts ET_DYN (PIE) executables. Unlike Linux, OpenBSD does NOT
# support static PIE: ET_DYN without PT_INTERP returns ENOEXEC. We must set
# --dynamic-linker so LLD emits PT_INTERP. At runtime ld.so finds zero
# relocations and zero dynamic symbols, so it simply jumps to entry_point.
# The PT_INTERP segment is only in the ELF — the extracted PIC .bin is unaffected.
pir_add_link_flags(
    -e,entry_point
    --dynamic-linker=/usr/libexec/ld.so
    --symbol-ordering-file=${PIR_ROOT_DIR}/cmake/data/function.order.openbsd
    --build-id=none
    -Map,${PIR_MAP_FILE}
)

# --pie produces a PIE (ET_DYN) executable. OpenBSD requires PIE on all
# architectures — the kernel rejects non-PIE ET_EXEC binaries with ENOEXEC.
# On x86 this also ensures -flto=full uses RIP-relative addressing rather
# than absolute. For riscv64, the custom linker script (which merges .rodata
# into .text) omits the fixed base address to remain compatible with PIE.
pir_add_link_flags(--pie)

if(PIR_BUILD_TYPE STREQUAL "release")
    pir_add_link_flags(--strip-all --gc-sections)
endif()

# RISC-V 64: merge .rodata (LTO constant pools) into .text so the
# PIC binary contains the constant-pool data that auipc+ld references.
# The linker script omits a fixed base address to stay compatible with
# --pie (ET_DYN); the kernel assigns the load address at runtime.
# Disable linker relaxation: the relaxation pass can convert auipc+ld
# sequences into GP-relative loads (e.g. c.ld via gp). PIR has no CRT
# to initialise the gp register, so GP-relative accesses fault at runtime.
if(PIR_ARCH STREQUAL "riscv64")
    list(APPEND PIR_BASE_FLAGS -mno-relax)
    pir_add_link_flags(-T,${PIR_ROOT_DIR}/cmake/data/linker.openbsd.riscv64.ld --no-relax)
endif()

list(APPEND PIR_BASE_LINK_FLAGS -fuse-ld=lld)

# ELFOSABI patch — LLD produces ELFOSABI_NONE; OpenBSD requires
# ELFOSABI_OPENBSD (12). PostBuild.cmake patches this after linking.
set(PIR_ELF_OSABI 12)
