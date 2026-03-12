# =============================================================================
# pir - Position-Independent Runtime Build System
# =============================================================================

cmake_minimum_required(VERSION 3.20)

option(BUILD_DOCS "Build documentation only (no compiler required)" OFF)

# =============================================================================
# Documentation-only build
# =============================================================================
if(BUILD_DOCS)
    project(pir-docs LANGUAGES NONE)
    include(${CMAKE_SOURCE_DIR}/cmake/Logging.cmake)

    find_program(DOXYGEN_EXECUTABLE doxygen REQUIRED)

    add_custom_target(docs ALL
        COMMAND ${DOXYGEN_EXECUTABLE} ${CMAKE_SOURCE_DIR}/Doxyfile
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        COMMENT "Generating documentation with Doxygen"
        VERBATIM
    )

    pir_log_header("Documentation Build")
    pir_log_kv("Doxygen" "${DOXYGEN_EXECUTABLE}")
    pir_log_kv("Output" "docs/html/")
    pir_log_footer()

    return()
endif()

# =============================================================================
# Compilation build
# =============================================================================
include(${CMAKE_SOURCE_DIR}/cmake/Toolchain.cmake)
project(pir LANGUAGES CXX)
include(${CMAKE_SOURCE_DIR}/cmake/Common.cmake)
include(${PIR_ROOT_DIR}/cmake/Target.cmake)
