# =============================================================================
# Linux.cmake - Linux Platform Configuration
# =============================================================================

include_guard(GLOBAL)

cpppic_get_target_info()
cpppic_filter_sources(windows uefi)

list(APPEND CPPPIC_INCLUDE_PATHS "${CMAKE_SOURCE_DIR}/include/pal/linux")
list(APPEND CPPPIC_BASE_FLAGS -target ${CPPPIC_TRIPLE})

# Linker configuration
cpppic_add_link_flags(
    -e,_start
    --no-dynamic-linker
    --symbol-ordering-file=${CMAKE_SOURCE_DIR}/orderfile.txt
    --build-id=none
    -Map,${CPPPIC_MAP_FILE}
)

if(CPPPIC_BUILD_TYPE STREQUAL "release")
    cpppic_add_link_flags(--strip-all --gc-sections)
endif()

list(APPEND CPPPIC_BASE_LINK_FLAGS -target ${CPPPIC_TRIPLE})
