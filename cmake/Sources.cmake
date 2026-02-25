# =============================================================================
# Sources.cmake - Source Collection and Platform Filtering
# =============================================================================

include_guard(GLOBAL)

# =============================================================================
# Source Collection
# =============================================================================
# Auto-discover include paths (all subdirs under include/)
file(GLOB_RECURSE _all_headers CONFIGURE_DEPENDS "${CMAKE_SOURCE_DIR}/include/*.h")
set(PIR_INCLUDE_PATHS "${CMAKE_SOURCE_DIR}" "${CMAKE_SOURCE_DIR}/include" "${CMAKE_SOURCE_DIR}/tests")
foreach(_hdr ${_all_headers})
    get_filename_component(_dir "${_hdr}" DIRECTORY)
    list(APPEND PIR_INCLUDE_PATHS "${_dir}")
endforeach()
list(REMOVE_DUPLICATES PIR_INCLUDE_PATHS)

file(GLOB_RECURSE PIR_SOURCES CONFIGURE_DEPENDS "${CMAKE_SOURCE_DIR}/src/*.cc" "${CMAKE_SOURCE_DIR}/tests/*.cc")
file(GLOB_RECURSE PIR_HEADERS CONFIGURE_DEPENDS "${CMAKE_SOURCE_DIR}/include/*.h" "${CMAKE_SOURCE_DIR}/tests/*.h")

# =============================================================================
# Helper: Filter Platform Sources
# =============================================================================
# Excludes sources and include paths for the listed platforms.
# Usage: pir_filter_sources(linux macos uefi)  -- excludes listed platforms
macro(pir_filter_sources)
    foreach(_exclude ${ARGN})
        list(FILTER PIR_SOURCES EXCLUDE REGEX ".*/${_exclude}/.*")
        list(FILTER PIR_INCLUDE_PATHS EXCLUDE REGEX ".*/platform/${_exclude}$")
    endforeach()
endmacro()
