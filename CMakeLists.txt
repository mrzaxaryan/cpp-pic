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
find_program(DOXYGEN_EXECUTABLE doxygen)

if(DOXYGEN_EXECUTABLE)
    add_custom_target(docs
        COMMAND ${DOXYGEN_EXECUTABLE} ${CMAKE_SOURCE_DIR}/Doxyfile
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        COMMENT "Generating documentation with Doxygen"
        VERBATIM
    )
    set(_doxygen_status "Available")
else()
    set(_doxygen_status "Not found")
endif()

include(${PIR_ROOT_DIR}/cmake/Target.cmake)
