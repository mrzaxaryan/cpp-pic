# =============================================================================
# Windows.cmake - Windows Platform Configuration
# =============================================================================

include_guard(GLOBAL)

pir_get_target_info()
pir_filter_sources(linux macos uefi)

list(APPEND PIR_INCLUDE_PATHS "${CMAKE_SOURCE_DIR}/include/platform/windows")

# Debug-specific
if(PIR_BUILD_TYPE STREQUAL "debug")
    list(APPEND PIR_BASE_FLAGS -gcodeview)
endif()

# Architecture-specific compiler flags
if(PIR_ARCH STREQUAL "x86_64")
    list(APPEND PIR_BASE_FLAGS -mno-red-zone)
elseif(PIR_ARCH MATCHES "^(armv7a|aarch64)$")
    list(APPEND PIR_BASE_FLAGS -mstack-probe-size=0)
endif()

# Linker configuration (PE/COFF)
pir_add_link_flags(
    /Entry:entry_point
    /SUBSYSTEM:CONSOLE
    /ORDER:@${CMAKE_SOURCE_DIR}/cmake/data/function.order.windows
    /MERGE:.rdata=.text
    /MAP:${PIR_MAP_FILE}
    /FILEALIGN:0x200
)

if(PIR_ARCH STREQUAL "i386")
    pir_add_link_flags(/BASE:0x400000 /SAFESEH:NO)
endif()

if(PIR_BUILD_TYPE STREQUAL "debug")
    pir_add_link_flags(/DEBUG)
else()
    pir_add_link_flags(--strip-all /OPT:REF /OPT:ICF /RELEASE)
endif()

list(APPEND PIR_BASE_LINK_FLAGS -fuse-ld=lld)
