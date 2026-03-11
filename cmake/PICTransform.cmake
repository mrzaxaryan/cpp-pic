# =============================================================================
# PICTransform.cmake - Build and use the pic-transform LLVM pass
# =============================================================================
# Acquires pic-transform (in order of preference):
#   1. Already on PATH
#   2. Prebuilt binary from GitHub releases (no LLVM dev needed)
#   3. Build from submodule (requires LLVM dev headers)
#
# The pass eliminates .rodata/.rdata/.data/.bss sections by transforming global
# constants into stack-local immediate values automatically during compilation.

include_guard(GLOBAL)

set(PIC_TRANSFORM_VERSION "v0.1.1" CACHE STRING "pic-transform release version to download")
set(PIC_TRANSFORM_DIR "${PIR_ROOT_DIR}/pic-transform")

# =============================================================================
# Detect host platform for download URL
# =============================================================================
if(CMAKE_HOST_SYSTEM_NAME STREQUAL "Linux")
    set(_PT_PLATFORM "linux-x86_64")
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
    set(_PT_PLATFORM "windows-x86_64")
    set(_PT_EXT ".zip")
    set(_PT_BIN_NAME "pic-transform.exe")
else()
    set(_PT_PLATFORM "")
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
set(_PT_DOWNLOAD_DIR "${CMAKE_BINARY_DIR}/pic-transform-download")
set(_PT_DOWNLOADED_BIN "${_PT_DOWNLOAD_DIR}/${_PT_BIN_NAME}")

if(_PT_PLATFORM AND NOT EXISTS "${_PT_DOWNLOADED_BIN}")
    set(_PT_URL "https://github.com/mrzaxaryan/pic-transform/releases/download/${PIC_TRANSFORM_VERSION}/pic-transform-${_PT_PLATFORM}${_PT_EXT}")

    message(STATUS "pic-transform: downloading prebuilt binary from ${_PT_URL}")
    file(MAKE_DIRECTORY "${_PT_DOWNLOAD_DIR}")

    set(_PT_ARCHIVE "${_PT_DOWNLOAD_DIR}/pic-transform${_PT_EXT}")
    file(DOWNLOAD "${_PT_URL}" "${_PT_ARCHIVE}"
        STATUS _PT_DL_STATUS
        TIMEOUT 60)

    list(GET _PT_DL_STATUS 0 _PT_DL_CODE)
    if(_PT_DL_CODE EQUAL 0)
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

        # Make executable on Unix
        if(NOT CMAKE_HOST_SYSTEM_NAME STREQUAL "Windows" AND EXISTS "${_PT_DOWNLOADED_BIN}")
            execute_process(COMMAND chmod +x "${_PT_DOWNLOADED_BIN}")
        endif()
    else()
        list(GET _PT_DL_STATUS 1 _PT_DL_MSG)
        message(STATUS "pic-transform: download failed (${_PT_DL_MSG}), will try building from source")
    endif()
endif()

if(EXISTS "${_PT_DOWNLOADED_BIN}")
    set(PIC_TRANSFORM_EXECUTABLE "${_PT_DOWNLOADED_BIN}")
    set(PIC_TRANSFORM_TARGET "")
    message(STATUS "pic-transform: using prebuilt binary (${PIC_TRANSFORM_EXECUTABLE})")
    return()
endif()

# =============================================================================
# Strategy 3: Build from submodule (requires LLVM dev)
# =============================================================================
if(NOT EXISTS "${PIC_TRANSFORM_DIR}/CMakeLists.txt")
    message(FATAL_ERROR
        "pic-transform: no binary found and submodule not checked out.\n"
        "Either:\n"
        "  1. Tag a release (git tag v0.1.0 && git push origin v0.1.0) and rebuild\n"
        "  2. Run: git submodule update --init pic-transform\n"
        "  3. Install llvm-dev and rebuild")
endif()

include(ExternalProject)

# Find LLVM for the host build
find_program(_HOST_LLVM_CONFIG llvm-config)
if(_HOST_LLVM_CONFIG)
    execute_process(
        COMMAND ${_HOST_LLVM_CONFIG} --cmakedir
        OUTPUT_VARIABLE _HOST_LLVM_DIR
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )
endif()

if(NOT _HOST_LLVM_DIR)
    foreach(_ver 22 21 20)
        foreach(_prefix
            "/usr/lib/llvm-${_ver}/lib/cmake/llvm"
            "/opt/homebrew/opt/llvm/lib/cmake/llvm"
            "/usr/local/opt/llvm/lib/cmake/llvm"
        )
            if(EXISTS "${_prefix}/LLVMConfig.cmake")
                set(_HOST_LLVM_DIR "${_prefix}")
                break()
            endif()
        endforeach()
        if(_HOST_LLVM_DIR)
            break()
        endif()
    endforeach()
endif()

if(NOT _HOST_LLVM_DIR)
    message(FATAL_ERROR
        "pic-transform: Cannot find LLVM development files.\n"
        "Install llvm-dev (e.g. apt install llvm-22-dev) or set PIC_TRANSFORM_LLVM_DIR.")
endif()

set(PIC_TRANSFORM_LLVM_DIR "${_HOST_LLVM_DIR}" CACHE PATH "LLVM cmake dir for pic-transform build")

set(_PIC_TRANSFORM_BUILD_DIR "${CMAKE_BINARY_DIR}/pic-transform-build")
set(_PIC_TRANSFORM_BIN "${_PIC_TRANSFORM_BUILD_DIR}/pic-transform")

# Find host compiler (avoid LLVM-bundled clang++ that lacks libstdc++)
set(PIC_TRANSFORM_HOST_CXX "" CACHE FILEPATH "Host C++ compiler for pic-transform (auto-detected if empty)")
set(PIC_TRANSFORM_HOST_CC "" CACHE FILEPATH "Host C compiler for pic-transform (auto-detected if empty)")

set(_pt_cmake_extra_args
    -DLLVM_DIR=${PIC_TRANSFORM_LLVM_DIR}
    -DCMAKE_BUILD_TYPE=Release
)

if(PIC_TRANSFORM_HOST_CXX)
    list(APPEND _pt_cmake_extra_args -DCMAKE_CXX_COMPILER=${PIC_TRANSFORM_HOST_CXX})
else()
    foreach(_candidate /usr/bin/g++ /usr/local/bin/g++ /usr/bin/c++)
        if(EXISTS "${_candidate}")
            list(APPEND _pt_cmake_extra_args -DCMAKE_CXX_COMPILER=${_candidate})
            break()
        endif()
    endforeach()
endif()

if(PIC_TRANSFORM_HOST_CC)
    list(APPEND _pt_cmake_extra_args -DCMAKE_C_COMPILER=${PIC_TRANSFORM_HOST_CC})
else()
    foreach(_candidate /usr/bin/gcc /usr/local/bin/gcc /usr/bin/cc)
        if(EXISTS "${_candidate}")
            list(APPEND _pt_cmake_extra_args -DCMAKE_C_COMPILER=${_candidate})
            break()
        endif()
    endforeach()
endif()

ExternalProject_Add(pic-transform-build
    SOURCE_DIR "${PIC_TRANSFORM_DIR}"
    BINARY_DIR "${_PIC_TRANSFORM_BUILD_DIR}"
    CMAKE_ARGS ${_pt_cmake_extra_args}
    BUILD_COMMAND ${CMAKE_COMMAND} --build <BINARY_DIR>
    INSTALL_COMMAND ""
    BUILD_BYPRODUCTS "${_PIC_TRANSFORM_BIN}"
)

set(PIC_TRANSFORM_EXECUTABLE "${_PIC_TRANSFORM_BIN}")
set(PIC_TRANSFORM_TARGET pic-transform-build)
message(STATUS "pic-transform: building from submodule (LLVM_DIR=${PIC_TRANSFORM_LLVM_DIR})")

# Check for plugin .so (for -fpass-plugin= usage)
find_file(PIC_TRANSFORM_PLUGIN
    NAMES PICTransform.so PICTransform.dylib
    PATHS "${_PIC_TRANSFORM_BUILD_DIR}" "${PIC_TRANSFORM_DIR}/build"
    NO_DEFAULT_PATH
)
