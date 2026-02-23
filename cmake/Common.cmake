# =============================================================================
# Common.cmake - Shared Build Configuration
# =============================================================================

include_guard(GLOBAL)

# =============================================================================
# Dependency Scanning
# =============================================================================
# Required because CMAKE_CXX_COMPILER_FORCED skips compiler detection,
# which includes dependency scanning setup. This ensures header changes
# trigger rebuilds.
set(CMAKE_DEPENDS_USE_COMPILER TRUE)

# =============================================================================
# Build Options
# =============================================================================
set(ARCHITECTURE "x86_64" CACHE STRING "Target: i386, x86_64, armv7a, aarch64")
set(PLATFORM "windows" CACHE STRING "Target: windows, linux, uefi")
set(BUILD_TYPE "release" CACHE STRING "Build type: debug, release")
set(OPTIMIZATION_LEVEL "" CACHE STRING "Override optimization level (e.g., O2, Os)")
option(ENABLE_LOGGING "Enable logging macros" ON)

# Normalize inputs
string(TOLOWER "${ARCHITECTURE}" CPPPIC_ARCH)
string(TOLOWER "${PLATFORM}" CPPPIC_PLATFORM)
string(TOLOWER "${BUILD_TYPE}" CPPPIC_BUILD_TYPE)

# Validate inputs
set(_valid_archs i386 x86_64 armv7a aarch64)
set(_valid_platforms windows linux uefi)

if(NOT CPPPIC_ARCH IN_LIST _valid_archs)
    message(FATAL_ERROR "Invalid ARCHITECTURE '${ARCHITECTURE}'. Valid: ${_valid_archs}")
endif()
if(NOT CPPPIC_PLATFORM IN_LIST _valid_platforms)
    message(FATAL_ERROR "Invalid PLATFORM '${PLATFORM}'. Valid: ${_valid_platforms}")
endif()
if(NOT CPPPIC_BUILD_TYPE MATCHES "^(debug|release)$")
    message(FATAL_ERROR "Invalid BUILD_TYPE '${BUILD_TYPE}'. Valid: debug, release")
endif()

# Derived settings
string(TOUPPER "${CPPPIC_ARCH}" _ARCH)
string(TOUPPER "${CPPPIC_PLATFORM}" _PLAT)
set(CPPPIC_OUTPUT_DIR "${CMAKE_SOURCE_DIR}/build/${CPPPIC_BUILD_TYPE}/${CPPPIC_PLATFORM}/${CPPPIC_ARCH}")
set(CPPPIC_MAP_FILE "${CPPPIC_OUTPUT_DIR}/output.map.txt")
set(CPPPIC_IS_DEBUG $<STREQUAL:${CPPPIC_BUILD_TYPE},debug>)

if(OPTIMIZATION_LEVEL)
    set(CPPPIC_OPT_LEVEL "${OPTIMIZATION_LEVEL}")
elseif(CPPPIC_BUILD_TYPE STREQUAL "debug")
    set(CPPPIC_OPT_LEVEL "Og")
else()
    set(CPPPIC_OPT_LEVEL "Oz")
endif()

# =============================================================================
# Target Triples (consolidated)
# =============================================================================
set(_triple_windows_i386    "i386-pc-windows-gnu")
set(_triple_windows_x86_64  "x86_64-pc-windows-gnu")
set(_triple_windows_armv7a  "armv7a-pc-windows-gnu")
set(_triple_windows_aarch64 "aarch64-pc-windows-gnu")
set(_triple_linux_i386      "i386-unknown-linux-gnu")
set(_triple_linux_x86_64    "x86_64-unknown-linux-gnu")
set(_triple_linux_armv7a    "armv7a-unknown-linux-gnueabihf")
set(_triple_linux_aarch64   "aarch64-unknown-linux-gnu")
set(_triple_uefi_x86_64     "x86_64-pc-windows-gnu")
set(_triple_uefi_aarch64    "aarch64-pc-windows-gnu")

# Platform extensions
set(_ext_windows ".exe")
set(_ext_linux ".elf")
set(_ext_uefi ".efi")

# =============================================================================
# Helper: Get Triple and Extension
# =============================================================================
macro(cpppic_get_target_info)
    set(CPPPIC_TRIPLE "${_triple_${CPPPIC_PLATFORM}_${CPPPIC_ARCH}}")
    set(CPPPIC_EXT "${_ext_${CPPPIC_PLATFORM}}")
    if(NOT CPPPIC_TRIPLE)
        message(FATAL_ERROR "${CPPPIC_PLATFORM}/${CPPPIC_ARCH} is not a valid combination")
    endif()
endmacro()

# =============================================================================
# Preprocessor Defines
# =============================================================================
set(CPPPIC_DEFINES
    ARCHITECTURE_${_ARCH}
    PLATFORM_${_PLAT}
    PLATFORM_${_PLAT}_${_ARCH}
)
if(CPPPIC_BUILD_TYPE STREQUAL "debug")
    list(APPEND CPPPIC_DEFINES DEBUG)
endif()
if(ENABLE_LOGGING)
    list(APPEND CPPPIC_DEFINES ENABLE_LOGGING)
endif()

# =============================================================================
# Base Compiler Flags
# =============================================================================
set(CPPPIC_BASE_FLAGS
    -std=c++23
    -Werror -Wall -Wextra
    -Wno-gnu-string-literal-operator-template
    -Qn
    -nostdlib
    -fno-omit-frame-pointer
    -fno-ident
    -fno-exceptions
    -fno-rtti
    -fno-builtin
    -fno-stack-check
    -fno-jump-tables
    -ffunction-sections
    -fdata-sections
    -fshort-wchar
)

# Architecture-specific
if(CPPPIC_ARCH MATCHES "^(i386|x86_64)$")
    list(APPEND CPPPIC_BASE_FLAGS -mno-stack-arg-probe -msoft-float)
else()
    list(APPEND CPPPIC_BASE_FLAGS -mno-implicit-float)
endif()

# Build-type-specific
if(CPPPIC_BUILD_TYPE STREQUAL "debug")
    list(APPEND CPPPIC_BASE_FLAGS
        -g3 -ferror-limit=200 -${CPPPIC_OPT_LEVEL}
    )
else()
    list(APPEND CPPPIC_BASE_FLAGS
        -fno-asynchronous-unwind-tables
        -fno-unwind-tables
        -flto=full
        -finline-functions
        -funroll-loops
        -fwhole-program-vtables
        -${CPPPIC_OPT_LEVEL}
    )
endif()

# =============================================================================
# Base Linker Flags
# =============================================================================
set(CPPPIC_BASE_LINK_FLAGS -fuse-ld=lld -nostdlib)

# =============================================================================
# Helper: Append Linker Flags
# =============================================================================
macro(cpppic_add_link_flags)
    foreach(_flag ${ARGN})
        list(APPEND CPPPIC_BASE_LINK_FLAGS "SHELL:-Wl,${_flag}")
    endforeach()
endmacro()

# =============================================================================
# Source Collection
# =============================================================================
# Auto-discover include paths (all subdirs under include/)
file(GLOB_RECURSE _all_headers CONFIGURE_DEPENDS "${CMAKE_SOURCE_DIR}/include/*.h")
set(CPPPIC_INCLUDE_PATHS "${CMAKE_SOURCE_DIR}" "${CMAKE_SOURCE_DIR}/include" "${CMAKE_SOURCE_DIR}/tests")
foreach(_hdr ${_all_headers})
    get_filename_component(_dir "${_hdr}" DIRECTORY)
    list(APPEND CPPPIC_INCLUDE_PATHS "${_dir}")
endforeach()
list(REMOVE_DUPLICATES CPPPIC_INCLUDE_PATHS)

file(GLOB_RECURSE CPPPIC_SOURCES CONFIGURE_DEPENDS "${CMAKE_SOURCE_DIR}/src/*.cc" "${CMAKE_SOURCE_DIR}/tests/*.cc")
file(GLOB_RECURSE CPPPIC_HEADERS CONFIGURE_DEPENDS "${CMAKE_SOURCE_DIR}/include/*.h" "${CMAKE_SOURCE_DIR}/tests/*.h")

# =============================================================================
# Helper: Filter Platform Sources
# =============================================================================
macro(cpppic_filter_sources)
    foreach(_exclude ${ARGN})
        list(FILTER CPPPIC_SOURCES EXCLUDE REGEX ".*/${_exclude}/.*")
    endforeach()
endmacro()

# =============================================================================
# Post-Build Commands
# =============================================================================
function(cpppic_add_postbuild target_name)
    set(_out "${CPPPIC_OUTPUT_DIR}/output")

    add_custom_command(TARGET ${target_name} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory "${CPPPIC_OUTPUT_DIR}"
        COMMAND ${CMAKE_COMMAND} -E echo "Build complete: ${_out}${CPPPIC_EXT}"
        COMMAND ${CMAKE_COMMAND}
            -DINPUT_FILE="${_out}${CPPPIC_EXT}"
            -DOUTPUT_DIR="${CPPPIC_OUTPUT_DIR}"
            -P "${CMAKE_SOURCE_DIR}/cmake/ExtractBinary.cmake"
        COMMAND ${CMAKE_COMMAND}
            -DPIC_FILE="${_out}.bin"
            -DBASE64_FILE="${_out}.b64.txt"
            -P "${CMAKE_SOURCE_DIR}/cmake/Base64Encode.cmake"
        COMMAND ${CMAKE_COMMAND}
            -DMAP_FILE="${CPPPIC_MAP_FILE}"
            -P "${CMAKE_SOURCE_DIR}/cmake/VerifyPICMode.cmake"
        COMMENT "Generating PIC artifacts..."
    )
endfunction()
