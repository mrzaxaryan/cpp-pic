# =============================================================================
# UEFI.cmake - UEFI Platform Configuration
# =============================================================================

include_guard(GLOBAL)

# UEFI architecture validation
if(NOT CPPPIC_ARCH MATCHES "^(x86_64|aarch64)$")
    message(FATAL_ERROR "UEFI requires x86_64 or aarch64 (got: ${CPPPIC_ARCH})")
endif()

cpppic_get_target_info()
cpppic_filter_sources(windows linux)

list(APPEND CPPPIC_INCLUDE_PATHS "${CMAKE_SOURCE_DIR}/include/platform/uefi")
list(APPEND CPPPIC_BASE_FLAGS -target ${CPPPIC_TRIPLE})

# Architecture-specific flags
if(CPPPIC_ARCH STREQUAL "x86_64")
    list(APPEND CPPPIC_BASE_FLAGS -mno-red-zone)
elseif(CPPPIC_ARCH STREQUAL "aarch64")
    list(APPEND CPPPIC_BASE_FLAGS -mstack-probe-size=0)
endif()

# Linker configuration (PE/COFF for UEFI)
cpppic_add_link_flags(
    /Entry:_start
    /SUBSYSTEM:EFI_APPLICATION
    /NODEFAULTLIB
    /ORDER:@${CMAKE_SOURCE_DIR}/cmake/function.order
    /MAP:${CPPPIC_MAP_FILE}
)

if(CPPPIC_BUILD_TYPE STREQUAL "debug")
    cpppic_add_link_flags(/DEBUG)
else()
    cpppic_add_link_flags(/OPT:REF /OPT:ICF)
endif()

list(APPEND CPPPIC_BASE_LINK_FLAGS -target ${CPPPIC_TRIPLE})

# =============================================================================
# UEFI Boot Structure
# =============================================================================
function(cpppic_add_uefi_boot target_name)
    set(_boot_dir "${CPPPIC_OUTPUT_DIR}/EFI/BOOT")
    if(CPPPIC_ARCH STREQUAL "x86_64")
        set(_boot_name "BOOTX64.EFI")
    else()
        set(_boot_name "BOOTAA64.EFI")
    endif()

    add_custom_command(TARGET ${target_name} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory "${_boot_dir}"
        COMMAND ${CMAKE_COMMAND} -E copy "${CPPPIC_OUTPUT_DIR}/output${CPPPIC_EXT}" "${_boot_dir}/${_boot_name}"
        COMMENT "Creating UEFI boot image..."
        VERBATIM
    )
endfunction()
