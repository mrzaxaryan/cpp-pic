# =============================================================================
# PICTransform.cmake - Acquire the pic-transform LLVM pass tool
# =============================================================================
# Acquires pic-transform (in order of preference):
#   1. Pre-installed on PATH
#   2. Built from in-tree source (tools/pic-transform)
#
# The pass eliminates .rodata/.rdata/.data/.bss sections by transforming global
# constants into stack-local immediate values automatically during compilation.
#
# Outputs (at most one of these will be set):
#   PIC_TRANSFORM_PLUGIN      - Path to loadable pass plugin (.so/.dylib/.dll)
#   PIC_TRANSFORM_EXECUTABLE  - Path to standalone pic-transform binary
#   PIC_TRANSFORM_TARGET      - CMake target to add as build dependency

include_guard(GLOBAL)

# Platform-specific artifact names
if(CMAKE_HOST_SYSTEM_NAME STREQUAL "Windows")
    set(_pt_bin_name    "pic-transform.exe")
    set(_pt_plugin_name "PICTransform.dll")
elseif(CMAKE_HOST_SYSTEM_NAME STREQUAL "Darwin")
    set(_pt_bin_name    "pic-transform")
    set(_pt_plugin_name "PICTransform.dylib")
else()
    set(_pt_bin_name    "pic-transform")
    set(_pt_plugin_name "PICTransform.so")
endif()

# -----------------------------------------------------------------------------
# Helper: set output variables and return
# -----------------------------------------------------------------------------
macro(_pt_found mode path)
    if("${mode}" STREQUAL "plugin")
        set(PIC_TRANSFORM_PLUGIN     "${path}")
        set(PIC_TRANSFORM_EXECUTABLE "")
    else()
        set(PIC_TRANSFORM_PLUGIN     "")
        set(PIC_TRANSFORM_EXECUTABLE "${path}")
    endif()
    set(PIC_TRANSFORM_TARGET "")
    pir_log_at("pic-transform" "${ARGN} (${path})")
    return()
endmacro()

# Helper: find a built artifact in both single-config and multi-config layouts
macro(_pt_find_artifact var name build_dir)
    set(${var} "")
    if(EXISTS "${build_dir}/${name}")
        set(${var} "${build_dir}/${name}")
    elseif(EXISTS "${build_dir}/Release/${name}")
        set(${var} "${build_dir}/Release/${name}")
    endif()
endmacro()

# =============================================================================
# Strategy 1: Pre-installed on PATH
# =============================================================================
find_program(_pt_system_bin pic-transform)

if(_pt_system_bin)
    _pt_found(standalone "${_pt_system_bin}" "Using system binary")
endif()

# =============================================================================
# Strategy 2: Build from in-tree source
# =============================================================================
# Builds tools/pic-transform if LLVM dev files are available.
# Override LLVM location: -DPIC_TRANSFORM_LLVM_DIR=/path/to/lib/cmake/llvm

set(_pt_source_dir "${PIR_ROOT_DIR}/tools/pic-transform")

if(NOT EXISTS "${_pt_source_dir}/CMakeLists.txt")
    message(WARNING "[pir:pic-transform] Source not found at ${_pt_source_dir} — building without pic-transform")
    return()
endif()

# ── Locate LLVM cmake config from the compiler installation ─────────────
get_filename_component(_pt_compiler_real "${CMAKE_CXX_COMPILER}" REALPATH)
get_filename_component(_pt_compiler_dir  "${_pt_compiler_real}" DIRECTORY)
get_filename_component(_pt_llvm_root     "${_pt_compiler_dir}/.." ABSOLUTE)

if(NOT DEFINED PIC_TRANSFORM_LLVM_DIR)
    foreach(_dir "${_pt_llvm_root}/lib/cmake/llvm"
                 "${_pt_llvm_root}/lib64/cmake/llvm")
        if(EXISTS "${_dir}/LLVMConfig.cmake")
            set(PIC_TRANSFORM_LLVM_DIR "${_dir}")
            break()
        endif()
    endforeach()
endif()

if(NOT DEFINED PIC_TRANSFORM_LLVM_DIR OR NOT EXISTS "${PIC_TRANSFORM_LLVM_DIR}/LLVMConfig.cmake")
    message(WARNING "[pir:pic-transform] LLVM dev files not found (need LLVMConfig.cmake) — building without pic-transform")
    return()
endif()

# ── Configure ───────────────────────────────────────────────────────────
set(_pt_build_dir "${CMAKE_BINARY_DIR}/pic-transform-build")

if(NOT EXISTS "${_pt_build_dir}/CMakeCache.txt")
    pir_log_at("pic-transform" "Configuring (LLVM_DIR=${PIC_TRANSFORM_LLVM_DIR})")
    execute_process(
        COMMAND ${CMAKE_COMMAND}
            -S "${_pt_source_dir}"
            -B "${_pt_build_dir}"
            -DLLVM_DIR=${PIC_TRANSFORM_LLVM_DIR}
            -DCMAKE_BUILD_TYPE=Release
            -DCMAKE_C_COMPILER=${_pt_compiler_dir}/clang
            -DCMAKE_CXX_COMPILER=${_pt_compiler_dir}/clang++
            -DSTATIC_LINK=ON
        RESULT_VARIABLE _pt_cfg_rc
        OUTPUT_VARIABLE _pt_cfg_out
        ERROR_VARIABLE  _pt_cfg_err
    )
    if(NOT _pt_cfg_rc EQUAL 0)
        message(WARNING "[pir:pic-transform] Configure failed — building without pic-transform:\n${_pt_cfg_err}")
        return()
    endif()
endif()

# ── Build ───────────────────────────────────────────────────────────────
pir_log_at("pic-transform" "Building from source...")
execute_process(
    COMMAND ${CMAKE_COMMAND} --build "${_pt_build_dir}" --config Release
    RESULT_VARIABLE _pt_build_rc
    OUTPUT_VARIABLE _pt_build_out
    ERROR_VARIABLE  _pt_build_err
)

if(NOT _pt_build_rc EQUAL 0)
    message(WARNING "[pir:pic-transform] Build failed — building without pic-transform:\n${_pt_build_err}")
    return()
endif()

# ── Locate artifact (prefer plugin over standalone) ─────────────────────
_pt_find_artifact(_pt_plugin "${_pt_plugin_name}" "${_pt_build_dir}")
if(_pt_plugin)
    _pt_found(plugin "${_pt_plugin}" "Built plugin from source")
endif()

_pt_find_artifact(_pt_bin "${_pt_bin_name}" "${_pt_build_dir}")
if(_pt_bin)
    _pt_found(standalone "${_pt_bin}" "Built standalone from source")
endif()

message(WARNING "[pir:pic-transform] Build succeeded but no artifact found in ${_pt_build_dir} — building without pic-transform")
