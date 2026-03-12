# =============================================================================
# Linux.cmake - Linux Platform Configuration
# =============================================================================

include_guard(GLOBAL)

pir_get_target_info()
pir_filter_sources(windows macos uefi solaris freebsd android ios)

list(APPEND PIR_INCLUDE_PATHS
    "${PIR_ROOT_DIR}/src/platform/kernel/linux")

# Linker configuration (ELF)
pir_add_link_flags(
    -e,entry_point
    --no-dynamic-linker
    --symbol-ordering-file=${PIR_ROOT_DIR}/cmake/data/function.order.linux
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
    pir_log_debug_at("linux" "riscv64: custom linker script (rodata->text merge)")
endif()

# MIPS64: merge .rodata (LTO constant pools) into .text, same rationale.
if(PIR_ARCH STREQUAL "mips64")
    pir_add_link_flags(-T,${PIR_ROOT_DIR}/cmake/data/linker.mips64.ld)
    pir_log_debug_at("linux" "mips64: custom linker script (rodata->text merge)")
endif()

list(APPEND PIR_BASE_LINK_FLAGS -fuse-ld=lld)
