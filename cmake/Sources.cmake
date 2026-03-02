# =============================================================================
# Sources.cmake - Source Collection and Platform Filtering
# =============================================================================

include_guard(GLOBAL)

# =============================================================================
# Source Collection
# =============================================================================
# Auto-discover include paths (all subdirs containing headers)
file(GLOB_RECURSE _all_headers CONFIGURE_DEPENDS
    "${CMAKE_SOURCE_DIR}/src/core/*.h"
    "${CMAKE_SOURCE_DIR}/src/platform/*.h"
    "${CMAKE_SOURCE_DIR}/src/runtime/*.h"
)
set(PIR_INCLUDE_PATHS "${CMAKE_SOURCE_DIR}/src")
foreach(_hdr ${_all_headers})
    get_filename_component(_dir "${_hdr}" DIRECTORY)
    list(APPEND PIR_INCLUDE_PATHS "${_dir}")
endforeach()

file(GLOB_RECURSE PIR_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_SOURCE_DIR}/src/core/*.cc"
    "${CMAKE_SOURCE_DIR}/src/platform/*.cc"
    "${CMAKE_SOURCE_DIR}/src/runtime/*.cc"
)
file(GLOB_RECURSE PIR_HEADERS CONFIGURE_DEPENDS
    "${CMAKE_SOURCE_DIR}/src/core/*.h"
    "${CMAKE_SOURCE_DIR}/src/platform/*.h"
    "${CMAKE_SOURCE_DIR}/src/runtime/*.h"
)

# Append START_DIR sources if provided
if(PIR_APP_DIR)
    list(APPEND PIR_INCLUDE_PATHS "${PIR_APP_DIR}")
    file(GLOB_RECURSE _start_sources CONFIGURE_DEPENDS "${PIR_APP_DIR}/*.cc")
    file(GLOB_RECURSE _start_headers CONFIGURE_DEPENDS "${PIR_APP_DIR}/*.h")
    list(APPEND PIR_SOURCES ${_start_sources})
    list(APPEND PIR_HEADERS ${_start_headers})
endif()

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
