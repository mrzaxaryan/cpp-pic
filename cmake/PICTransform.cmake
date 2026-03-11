# =============================================================================
# PICTransform.cmake - Acquire the pic-transform LLVM pass tool
# =============================================================================
# Acquires pic-transform (in order of preference):
#   1. Already on PATH
#   2. Prebuilt binary from GitHub releases
#
# The pass eliminates .rodata/.rdata/.data/.bss sections by transforming global
# constants into stack-local immediate values automatically during compilation.

include_guard(GLOBAL)

set(PIC_TRANSFORM_VERSION "v0.0.1-alpha.1" CACHE STRING "pic-transform release version to download")

# =============================================================================
# Detect host platform for download URL
# =============================================================================
if(CMAKE_HOST_SYSTEM_NAME STREQUAL "Linux")
    if(CMAKE_HOST_SYSTEM_PROCESSOR STREQUAL "aarch64" OR CMAKE_HOST_SYSTEM_PROCESSOR STREQUAL "arm64")
        set(_PT_PLATFORM "linux-aarch64")
    else()
        set(_PT_PLATFORM "linux-x86_64")
    endif()
    set(_PT_EXT ".tar.gz")
    set(_PT_BIN_NAME "pic-transform")
elseif(CMAKE_HOST_SYSTEM_NAME STREQUAL "Darwin")
    if(CMAKE_HOST_SYSTEM_PROCESSOR STREQUAL "arm64" OR CMAKE_HOST_SYSTEM_PROCESSOR STREQUAL "aarch64")
        set(_PT_PLATFORM "macos-arm64")
    else()
        set(_PT_PLATFORM "macos-x86_64")
    endif()
    set(_PT_EXT ".tar.gz")
    set(_PT_BIN_NAME "pic-transform")
elseif(CMAKE_HOST_SYSTEM_NAME STREQUAL "Windows")
    if(CMAKE_HOST_SYSTEM_PROCESSOR STREQUAL "ARM64" OR CMAKE_HOST_SYSTEM_PROCESSOR STREQUAL "aarch64")
        set(_PT_PLATFORM "windows-aarch64")
    else()
        set(_PT_PLATFORM "windows-x86_64")
    endif()
    set(_PT_EXT ".zip")
    set(_PT_BIN_NAME "pic-transform.exe")
else()
    message(FATAL_ERROR "pic-transform: unsupported host platform ${CMAKE_HOST_SYSTEM_NAME}")
endif()

# =============================================================================
# Strategy 1: Already on PATH
# =============================================================================
find_program(PIC_TRANSFORM_EXECUTABLE pic-transform)

if(PIC_TRANSFORM_EXECUTABLE)
    message(STATUS "pic-transform: using system binary (${PIC_TRANSFORM_EXECUTABLE})")
    set(PIC_TRANSFORM_TARGET "")
    return()
endif()

# =============================================================================
# Strategy 2: Download prebuilt from GitHub releases
# =============================================================================
# Use a native filesystem path on WSL (NTFS doesn't support chmod +x properly)
if(NOT CMAKE_HOST_SYSTEM_NAME STREQUAL "Windows"
   AND CMAKE_BINARY_DIR MATCHES "^/mnt/[a-z]/")
    set(_PT_DOWNLOAD_DIR "/tmp/pir-pic-transform-${PIC_TRANSFORM_VERSION}")
else()
    set(_PT_DOWNLOAD_DIR "${CMAKE_BINARY_DIR}/pic-transform-download")
endif()
set(_PT_DOWNLOADED_BIN "${_PT_DOWNLOAD_DIR}/${_PT_BIN_NAME}")

if(NOT EXISTS "${_PT_DOWNLOADED_BIN}")
    set(_PT_URL "https://github.com/mrzaxaryan/pic-transform/releases/download/${PIC_TRANSFORM_VERSION}/pic-transform-${_PT_PLATFORM}${_PT_EXT}")

    message(STATUS "pic-transform: downloading from ${_PT_URL}")
    file(MAKE_DIRECTORY "${_PT_DOWNLOAD_DIR}")

    set(_PT_ARCHIVE "${_PT_DOWNLOAD_DIR}/pic-transform${_PT_EXT}")
    file(DOWNLOAD "${_PT_URL}" "${_PT_ARCHIVE}"
        STATUS _PT_DL_STATUS
        TIMEOUT 60)

    list(GET _PT_DL_STATUS 0 _PT_DL_CODE)
    if(NOT _PT_DL_CODE EQUAL 0)
        list(GET _PT_DL_STATUS 1 _PT_DL_MSG)
        message(FATAL_ERROR "pic-transform: download failed: ${_PT_DL_MSG}\n  URL: ${_PT_URL}")
    endif()

    # Extract
    if(_PT_EXT STREQUAL ".tar.gz")
        execute_process(
            COMMAND ${CMAKE_COMMAND} -E tar xzf "${_PT_ARCHIVE}"
            WORKING_DIRECTORY "${_PT_DOWNLOAD_DIR}"
            RESULT_VARIABLE _PT_EXTRACT_RESULT)
    else()
        execute_process(
            COMMAND ${CMAKE_COMMAND} -E tar xf "${_PT_ARCHIVE}"
            WORKING_DIRECTORY "${_PT_DOWNLOAD_DIR}"
            RESULT_VARIABLE _PT_EXTRACT_RESULT)
    endif()

    if(NOT _PT_EXTRACT_RESULT EQUAL 0)
        message(FATAL_ERROR "pic-transform: extraction failed")
    endif()

    # Make executable on Unix
    if(NOT CMAKE_HOST_SYSTEM_NAME STREQUAL "Windows" AND EXISTS "${_PT_DOWNLOADED_BIN}")
        execute_process(COMMAND chmod +x "${_PT_DOWNLOADED_BIN}")
    endif()
endif()

if(NOT EXISTS "${_PT_DOWNLOADED_BIN}")
    message(FATAL_ERROR "pic-transform: binary not found at ${_PT_DOWNLOADED_BIN} after download")
endif()

set(PIC_TRANSFORM_EXECUTABLE "${_PT_DOWNLOADED_BIN}")
set(PIC_TRANSFORM_TARGET "")
set(PIC_TRANSFORM_PLUGIN "")
message(STATUS "pic-transform: using prebuilt binary (${PIC_TRANSFORM_EXECUTABLE})")
