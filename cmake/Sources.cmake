# =============================================================================
# Sources.cmake - Source Collection and Platform Filtering
# =============================================================================

include_guard(GLOBAL)

# =============================================================================
# Source Collection
# =============================================================================
# Auto-discover include paths (all subdirs containing headers)
file(GLOB_RECURSE _all_headers CONFIGURE_DEPENDS
    "${PIR_ROOT_DIR}/src/core/*.h"
    "${PIR_ROOT_DIR}/src/platform/*.h"
    "${PIR_ROOT_DIR}/src/runtime/*.h"
)
set(PIR_INCLUDE_PATHS "${PIR_ROOT_DIR}/src")
foreach(_hdr ${_all_headers})
    get_filename_component(_dir "${_hdr}" DIRECTORY)
    list(APPEND PIR_INCLUDE_PATHS "${_dir}")
endforeach()

file(GLOB PIR_SOURCES_ROOT CONFIGURE_DEPENDS
    "${PIR_ROOT_DIR}/src/*.cc"
)
file(GLOB_RECURSE PIR_SOURCES CONFIGURE_DEPENDS
    "${PIR_ROOT_DIR}/src/core/*.cc"
    "${PIR_ROOT_DIR}/src/platform/*.cc"
    "${PIR_ROOT_DIR}/src/runtime/*.cc"
)
list(APPEND PIR_SOURCES ${PIR_SOURCES_ROOT})
file(GLOB_RECURSE PIR_HEADERS CONFIGURE_DEPENDS
    "${PIR_ROOT_DIR}/src/core/*.h"
    "${PIR_ROOT_DIR}/src/platform/*.h"
    "${PIR_ROOT_DIR}/src/runtime/*.h"
)

# Append APP_DIR sources if provided (supports multiple directories)
foreach(_app_dir IN LISTS PIR_APP_DIR)
    list(APPEND PIR_INCLUDE_PATHS "${_app_dir}")
    file(GLOB_RECURSE _app_sources CONFIGURE_DEPENDS "${_app_dir}/*.cc")
    file(GLOB_RECURSE _app_headers CONFIGURE_DEPENDS "${_app_dir}/*.h")
    list(APPEND PIR_SOURCES ${_app_sources})
    list(APPEND PIR_HEADERS ${_app_headers})
endforeach()

list(REMOVE_DUPLICATES PIR_INCLUDE_PATHS)

# =============================================================================
# Helper: Filter Platform Sources
# =============================================================================
# Excludes sources and include paths for the listed platforms.
# Usage: pir_filter_sources(linux macos uefi)  -- excludes listed platforms
macro(pir_filter_sources)
    foreach(_exclude ${ARGN})
        list(FILTER PIR_SOURCES EXCLUDE REGEX ".*/${_exclude}/.*")
        list(FILTER PIR_INCLUDE_PATHS EXCLUDE REGEX ".*/platform/(common|memory|io|fs|network|system)/${_exclude}(/.*)?$")
    endforeach()
endmacro()
