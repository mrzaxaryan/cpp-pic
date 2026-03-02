# =============================================================================
# UEFI.cmake - UEFI Platform Configuration
# =============================================================================

include_guard(GLOBAL)

# UEFI architecture validation
if(NOT PIR_ARCH MATCHES "^(x86_64|aarch64)$")
    message(FATAL_ERROR "UEFI requires x86_64 or aarch64 (got: ${PIR_ARCH})")
endif()

pir_get_target_info()
pir_filter_sources(windows linux macos posix solaris)

list(APPEND PIR_INCLUDE_PATHS
    "${CMAKE_SOURCE_DIR}/src/platform/common/uefi")

# Architecture-specific compiler flags
if(PIR_ARCH STREQUAL "x86_64")
    list(APPEND PIR_BASE_FLAGS -mno-red-zone)
elseif(PIR_ARCH STREQUAL "aarch64")
    list(APPEND PIR_BASE_FLAGS -mstack-probe-size=0)
endif()

# Linker configuration (PE/COFF)
pir_add_link_flags(
    /Entry:entry_point
    /SUBSYSTEM:EFI_APPLICATION
    /NODEFAULTLIB
    /ORDER:@${CMAKE_SOURCE_DIR}/cmake/data/function.order.uefi
    /MAP:${PIR_MAP_FILE}
)

if(PIR_BUILD_TYPE STREQUAL "debug")
    pir_add_link_flags(/DEBUG)
else()
    pir_add_link_flags(--strip-all /OPT:REF /OPT:ICF /RELEASE)
endif()

list(APPEND PIR_BASE_LINK_FLAGS -fuse-ld=lld)

# =============================================================================
# UEFI Boot Structure
# =============================================================================
function(pir_add_uefi_boot target_name)
    set(_boot_dir "${PIR_OUTPUT_DIR}/EFI/BOOT")
    if(PIR_ARCH STREQUAL "x86_64")
        set(_boot_name "BOOTX64.EFI")
    else()
        set(_boot_name "BOOTAA64.EFI")
    endif()

    add_custom_command(TARGET ${target_name} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory "${_boot_dir}"
        COMMAND ${CMAKE_COMMAND} -E copy "${PIR_OUTPUT_DIR}/output${PIR_EXT}" "${_boot_dir}/${_boot_name}"
        COMMENT "Creating UEFI boot image..."
        VERBATIM
    )
endfunction()
