# =============================================================================
# Windows.cmake - Windows Platform Configuration
# =============================================================================

include_guard(GLOBAL)

cpppic_get_target_info()
cpppic_filter_sources(linux uefi)

list(APPEND CPPPIC_INCLUDE_PATHS "${CMAKE_SOURCE_DIR}/include/platform/windows")
list(APPEND CPPPIC_BASE_FLAGS -target ${CPPPIC_TRIPLE})

# Debug-specific
if(CPPPIC_BUILD_TYPE STREQUAL "debug")
    list(APPEND CPPPIC_BASE_FLAGS -gcodeview)
endif()

# ARM stack probing
if(CPPPIC_ARCH MATCHES "^(armv7a|aarch64)$")
    list(APPEND CPPPIC_BASE_FLAGS -mstack-probe-size=0)
endif()

# Linker configuration
cpppic_add_link_flags(
    /Entry:_start
    /SUBSYSTEM:CONSOLE
    /ORDER:@${CMAKE_SOURCE_DIR}/cmake/function.order
    /MERGE:.rdata=.text
    /MAP:${CPPPIC_MAP_FILE}
)

if(CPPPIC_ARCH STREQUAL "i386")
    cpppic_add_link_flags(/BASE:0x400000 /FILEALIGN:0x1000 /SAFESEH:NO)
endif()

if(CPPPIC_BUILD_TYPE STREQUAL "debug")
    cpppic_add_link_flags(/DEBUG)
else()
    cpppic_add_link_flags(--strip-all /OPT:REF /OPT:ICF /RELEASE /LTCG)
endif()

list(APPEND CPPPIC_BASE_LINK_FLAGS -target ${CPPPIC_TRIPLE})
