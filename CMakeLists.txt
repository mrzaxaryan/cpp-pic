# =============================================================================
# pir - Position-Independent Runtime Build System
# =============================================================================

cmake_minimum_required(VERSION 3.20)

# =============================================================================
# Toolchain Setup (must precede project())
# =============================================================================
set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR x86_64)

# Compiler detection
if(DEFINED ENV{CC})
    set(CMAKE_C_COMPILER "$ENV{CC}" CACHE STRING "" FORCE)
else()
    set(CMAKE_C_COMPILER clang CACHE STRING "" FORCE)
endif()
if(DEFINED ENV{CXX})
    set(CMAKE_CXX_COMPILER "$ENV{CXX}" CACHE STRING "" FORCE)
else()
    set(CMAKE_CXX_COMPILER clang++ CACHE STRING "" FORCE)
endif()

# Skip compiler tests (freestanding environment)
foreach(_lang C CXX)
    set(CMAKE_${_lang}_COMPILER_FORCED TRUE)
    set(CMAKE_${_lang}_COMPILER_WORKS TRUE)
    set(CMAKE_${_lang}_STANDARD_LIBRARIES "")
endforeach()

set(CMAKE_CXX_LINK_EXECUTABLE "<CMAKE_CXX_COMPILER> <LINK_FLAGS> <OBJECTS> -o <TARGET>")

# =============================================================================
# Project
# =============================================================================
project(pir LANGUAGES CXX)

include(${CMAKE_SOURCE_DIR}/cmake/Common.cmake)

# Platform modules (windows->Windows, linux->Linux, uefi->UEFI)
set(_platform_map_windows Windows)
set(_platform_map_linux Linux)
set(_platform_map_macos macOS)
set(_platform_map_uefi UEFI)
include(${CMAKE_SOURCE_DIR}/cmake/platforms/${_platform_map_${PIR_PLATFORM}}.cmake)

# Universal target flags (every platform uses -target for both compile and link)
list(APPEND PIR_BASE_FLAGS -target ${PIR_TRIPLE})
list(APPEND PIR_BASE_LINK_FLAGS -target ${PIR_TRIPLE})

# =============================================================================
# Output Configuration
# =============================================================================
file(MAKE_DIRECTORY "${PIR_OUTPUT_DIR}")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${PIR_OUTPUT_DIR}")
set(CMAKE_PDB_OUTPUT_DIRECTORY "${PIR_OUTPUT_DIR}")

# =============================================================================
# Target
# =============================================================================
add_executable(${PIR_TRIPLE} ${PIR_SOURCES} ${PIR_HEADERS})

set_target_properties(${PIR_TRIPLE} PROPERTIES
    OUTPUT_NAME "output"
    SUFFIX "${PIR_EXT}"
)

target_include_directories(${PIR_TRIPLE} PRIVATE ${PIR_INCLUDE_PATHS})
target_compile_definitions(${PIR_TRIPLE} PRIVATE ${PIR_DEFINES})
target_compile_options(${PIR_TRIPLE} PRIVATE ${PIR_BASE_FLAGS})
target_link_options(${PIR_TRIPLE} PRIVATE ${PIR_BASE_LINK_FLAGS})

# =============================================================================
# Post-Build
# =============================================================================
pir_add_postbuild(${PIR_TRIPLE})

if(PIR_PLATFORM STREQUAL "uefi")
    pir_add_uefi_boot(${PIR_TRIPLE})
endif()

# =============================================================================
# Documentation (Doxygen)
# =============================================================================
find_package(Doxygen OPTIONAL_COMPONENTS dot)

if(DOXYGEN_FOUND)
    set(DOXYGEN_PROJECT_NAME "Position-Independent Runtime")
    set(DOXYGEN_PROJECT_BRIEF "Position-Independent Runtime for Windows, Linux, and UEFI")
    set(DOXYGEN_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/docs)
    set(DOXYGEN_GENERATE_HTML YES)
    set(DOXYGEN_EXTRACT_ALL YES)
    set(DOXYGEN_EXTRACT_STATIC YES)
    set(DOXYGEN_SOURCE_BROWSER YES)

    doxygen_add_docs(docs
        ${CMAKE_SOURCE_DIR}/include
        ${CMAKE_SOURCE_DIR}/src
        COMMENT "Generating documentation with Doxygen"
    )
    set(_doxygen_status "Available")
else()
    set(_doxygen_status "Not found")
endif()

# =============================================================================
# Build Summary
# =============================================================================
message(STATUS "")
message(STATUS "┌─────────────────────────────────────┐")
message(STATUS "│      Build Configuration            │")
message(STATUS "├─────────────────────────────────────┤")
message(STATUS "│ Platform:     ${PIR_PLATFORM}")
message(STATUS "│ Architecture: ${PIR_ARCH}")
message(STATUS "│ Build Type:   ${PIR_BUILD_TYPE}")
message(STATUS "│ Optimization: -${PIR_OPT_LEVEL}")
message(STATUS "│ Triple:       ${PIR_TRIPLE}")
message(STATUS "├─────────────────────────────────────┤")
message(STATUS "│ Output:       output${PIR_EXT}")
message(STATUS "│ Doxygen:      ${_doxygen_status}")
message(STATUS "└─────────────────────────────────────┘")
message(STATUS "")
