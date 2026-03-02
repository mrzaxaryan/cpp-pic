# =============================================================================
# pir - Position-Independent Runtime Build System
# =============================================================================

cmake_minimum_required(VERSION 3.20)
include(${CMAKE_SOURCE_DIR}/cmake/Toolchain.cmake)
project(pir LANGUAGES CXX)
include(${CMAKE_SOURCE_DIR}/cmake/Common.cmake)

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
        ${PIR_ROOT_DIR}/src/core
        ${PIR_ROOT_DIR}/src/platform
        ${PIR_ROOT_DIR}/src/runtime
        COMMENT "Generating documentation with Doxygen"
    )
    set(_doxygen_status "Available")
else()
    set(_doxygen_status "Not found")
endif()

include(${PIR_ROOT_DIR}/cmake/Target.cmake)
