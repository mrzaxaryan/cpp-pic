# =============================================================================
# cpp-pic - Position-Independent Code Build System
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

set(CMAKE_LINKER lld)
set(CMAKE_CXX_LINK_EXECUTABLE "<CMAKE_CXX_COMPILER> -fuse-ld=lld <LINK_FLAGS> <OBJECTS> -o <TARGET>")

# =============================================================================
# Project
# =============================================================================
project(cpp-pic LANGUAGES CXX)

include(${CMAKE_SOURCE_DIR}/cmake/Common.cmake)

# Platform modules (windows->Windows, linux->Linux, uefi->UEFI)
set(_platform_map_windows Windows)
set(_platform_map_linux Linux)
set(_platform_map_macos macOS)
set(_platform_map_uefi UEFI)
include(${CMAKE_SOURCE_DIR}/cmake/${_platform_map_${CPPPIC_PLATFORM}}.cmake)

# =============================================================================
# Output Configuration
# =============================================================================
file(MAKE_DIRECTORY "${CPPPIC_OUTPUT_DIR}")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CPPPIC_OUTPUT_DIR}")
set(CMAKE_PDB_OUTPUT_DIRECTORY "${CPPPIC_OUTPUT_DIR}")

# =============================================================================
# Target
# =============================================================================
add_executable(${CPPPIC_TRIPLE} ${CPPPIC_SOURCES} ${CPPPIC_HEADERS})

set_target_properties(${CPPPIC_TRIPLE} PROPERTIES
    OUTPUT_NAME "output"
    SUFFIX "${CPPPIC_EXT}"
)

target_include_directories(${CPPPIC_TRIPLE} PRIVATE ${CPPPIC_INCLUDE_PATHS})
target_compile_definitions(${CPPPIC_TRIPLE} PRIVATE ${CPPPIC_DEFINES})
target_compile_options(${CPPPIC_TRIPLE} PRIVATE ${CPPPIC_BASE_FLAGS})
target_link_options(${CPPPIC_TRIPLE} PRIVATE ${CPPPIC_BASE_LINK_FLAGS})

# =============================================================================
# Post-Build
# =============================================================================
cpppic_add_postbuild(${CPPPIC_TRIPLE})

if(CPPPIC_PLATFORM STREQUAL "uefi")
    cpppic_add_uefi_boot(${CPPPIC_TRIPLE})
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
message(STATUS "│ Platform:     ${CPPPIC_PLATFORM}")
message(STATUS "│ Architecture: ${CPPPIC_ARCH}")
message(STATUS "│ Build Type:   ${CPPPIC_BUILD_TYPE}")
message(STATUS "│ Optimization: -${CPPPIC_OPT_LEVEL}")
message(STATUS "│ Triple:       ${CPPPIC_TRIPLE}")
message(STATUS "├─────────────────────────────────────┤")
message(STATUS "│ Output:       output${CPPPIC_EXT}")
message(STATUS "│ Doxygen:      ${_doxygen_status}")
message(STATUS "└─────────────────────────────────────┘")
message(STATUS "")
