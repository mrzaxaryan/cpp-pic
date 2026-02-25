# =============================================================================
# Linux.cmake - Linux Platform Configuration
# =============================================================================

include_guard(GLOBAL)

pir_get_target_info()
pir_filter_sources(windows macos uefi)

list(APPEND PIR_INCLUDE_PATHS "${CMAKE_SOURCE_DIR}/include/platform/linux")

# Linker configuration (ELF)
pir_add_link_flags(
    -e,entry_point
    --no-dynamic-linker
    --symbol-ordering-file=${CMAKE_SOURCE_DIR}/cmake/data/function.order.linux
    --build-id=none
    -Map,${PIR_MAP_FILE}
)

if(PIR_BUILD_TYPE STREQUAL "release")
    pir_add_link_flags(--strip-all --gc-sections)
endif()

list(APPEND PIR_BASE_LINK_FLAGS -fuse-ld=lld)
