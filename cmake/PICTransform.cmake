# =============================================================================
# PICTransform.cmake - Build and use the pic-transform LLVM pass
# =============================================================================
# Builds pic-transform as a host tool (separate from the freestanding PIR build)
# and integrates it into the compile pipeline via -fpass-plugin= or as a
# standalone bitcode transformer.
#
# The pass eliminates .rodata/.rdata/.data/.bss sections by transforming global
# constants into stack-local immediate values automatically during compilation.

include_guard(GLOBAL)

set(PIC_TRANSFORM_DIR "${PIR_ROOT_DIR}/pic-transform")

# Require the submodule
if(NOT EXISTS "${PIC_TRANSFORM_DIR}/CMakeLists.txt")
    message(FATAL_ERROR
        "pic-transform submodule not found.\n"
        "Run: git submodule update --init pic-transform")
endif()

# =============================================================================
# Find or build pic-transform
# =============================================================================
# First check if pic-transform is already available on PATH
find_program(PIC_TRANSFORM_EXECUTABLE pic-transform)

if(NOT PIC_TRANSFORM_EXECUTABLE)
    # Build it as an external project (host tool, not cross-compiled)
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
        # Try common locations
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

    # Allow override via cache variable
    set(PIC_TRANSFORM_LLVM_DIR "${_HOST_LLVM_DIR}" CACHE PATH "LLVM cmake dir for pic-transform build")

    set(_PIC_TRANSFORM_BUILD_DIR "${CMAKE_BINARY_DIR}/pic-transform-build")
    set(_PIC_TRANSFORM_BIN "${_PIC_TRANSFORM_BUILD_DIR}/pic-transform")

    # Determine host compiler. Prefer system compilers (g++/cc) over LLVM-bundled
    # clang++ which may lack libstdc++ and fail to link host tools.
    # Search /usr/bin first to avoid picking up cross-compilation toolchains.
    find_program(_HOST_CXX NAMES g++ c++ clang++
        PATHS /usr/bin /usr/local/bin
        NO_DEFAULT_PATH)
    if(NOT _HOST_CXX)
        find_program(_HOST_CXX NAMES g++ c++ clang++)
    endif()
    find_program(_HOST_CC NAMES gcc cc clang
        PATHS /usr/bin /usr/local/bin
        NO_DEFAULT_PATH)
    if(NOT _HOST_CC)
        find_program(_HOST_CC NAMES gcc cc clang)
    endif()

    # Allow user override
    set(PIC_TRANSFORM_HOST_CXX "${_HOST_CXX}" CACHE FILEPATH "Host C++ compiler for pic-transform")
    set(PIC_TRANSFORM_HOST_CC "${_HOST_CC}" CACHE FILEPATH "Host C compiler for pic-transform")

    ExternalProject_Add(pic-transform-build
        SOURCE_DIR "${PIC_TRANSFORM_DIR}"
        BINARY_DIR "${_PIC_TRANSFORM_BUILD_DIR}"
        CMAKE_ARGS
            -DLLVM_DIR=${PIC_TRANSFORM_LLVM_DIR}
            -DCMAKE_BUILD_TYPE=Release
            -DCMAKE_CXX_COMPILER=${PIC_TRANSFORM_HOST_CXX}
            -DCMAKE_C_COMPILER=${PIC_TRANSFORM_HOST_CC}
        BUILD_COMMAND ${CMAKE_COMMAND} --build <BINARY_DIR>
        INSTALL_COMMAND ""
        BUILD_BYPRODUCTS "${_PIC_TRANSFORM_BIN}"
    )

    set(PIC_TRANSFORM_EXECUTABLE "${_PIC_TRANSFORM_BIN}")
    set(PIC_TRANSFORM_TARGET pic-transform-build)
    message(STATUS "pic-transform: building from submodule (LLVM_DIR=${PIC_TRANSFORM_LLVM_DIR})")
else()
    message(STATUS "pic-transform: using system binary (${PIC_TRANSFORM_EXECUTABLE})")
    set(PIC_TRANSFORM_TARGET "")
endif()

# Also check for the plugin .so (for -fpass-plugin= usage)
find_file(PIC_TRANSFORM_PLUGIN
    NAMES PICTransform.so PICTransform.dylib
    PATHS "${_PIC_TRANSFORM_BUILD_DIR}" "${PIC_TRANSFORM_DIR}/build"
    NO_DEFAULT_PATH
)

# =============================================================================
# Function: Transform a bitcode file with pic-transform
# =============================================================================
# pir_pic_transform(<input.bc> <output.bc>)
function(pir_pic_transform INPUT OUTPUT)
    add_custom_command(
        OUTPUT "${OUTPUT}"
        COMMAND "${PIC_TRANSFORM_EXECUTABLE}" "${INPUT}" -o "${OUTPUT}"
        DEPENDS "${INPUT}" ${PIC_TRANSFORM_TARGET}
        COMMENT "pic-transform: ${INPUT}"
        VERBATIM
    )
endfunction()
