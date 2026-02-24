# =============================================================================
# macOS.cmake - macOS Platform Configuration
# =============================================================================

include_guard(GLOBAL)

# Validate: macOS only supports x86_64 and aarch64
if(NOT CPPPIC_ARCH MATCHES "^(x86_64|aarch64)$")
    message(FATAL_ERROR "macOS only supports x86_64 and aarch64 (got: ${CPPPIC_ARCH})")
endif()

cpppic_get_target_info()
cpppic_filter_sources(windows linux uefi)

list(APPEND CPPPIC_INCLUDE_PATHS "${CMAKE_SOURCE_DIR}/include/platform/macos")
list(APPEND CPPPIC_BASE_FLAGS -target ${CPPPIC_TRIPLE})

# macOS-specific compiler flags
list(APPEND CPPPIC_BASE_FLAGS -fno-stack-protector)

if(CPPPIC_ARCH STREQUAL "aarch64")
    list(APPEND CPPPIC_BASE_FLAGS -mstack-probe-size=0)
endif()

# Linker configuration (ld64.lld / Mach-O)
cpppic_add_link_flags(
    -e,_entry_point
    -static
    -platform_version,macos,11.0.0,11.0.0
    -order_file,${CMAKE_SOURCE_DIR}/cmake/function.order.macos
    -map,${CPPPIC_MAP_FILE}
)

if(CPPPIC_BUILD_TYPE STREQUAL "release")
    cpppic_add_link_flags(
        -dead_strip
        --icf=all
    )
endif()

list(APPEND CPPPIC_BASE_LINK_FLAGS -target ${CPPPIC_TRIPLE})
