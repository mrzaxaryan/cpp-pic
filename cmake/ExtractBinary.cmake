# =============================================================================
# ExtractBinary.cmake - Extract and analyze binary output (run via cmake -P)
# =============================================================================
# Usage: cmake -DINPUT_FILE=<exe/elf/macho> -DOUTPUT_DIR=<dir> -P ExtractBinary.cmake
# =============================================================================

cmake_minimum_required(VERSION 3.20)

if(NOT DEFINED INPUT_FILE OR NOT DEFINED OUTPUT_DIR)
    message(FATAL_ERROR "INPUT_FILE and OUTPUT_DIR must be defined")
endif()

get_filename_component(_name "${INPUT_FILE}" NAME_WE)
set(_bin "${OUTPUT_DIR}/${_name}.bin")
set(_txt "${OUTPUT_DIR}/${_name}.txt")
set(_strings "${OUTPUT_DIR}/${_name}.strings.txt")

# =============================================================================
# Detect binary format (Mach-O vs PE/ELF)
# =============================================================================
# Mach-O binaries use __TEXT,__text; PE/ELF use .text
set(_section_name ".text")
set(_is_macho FALSE)

# Check if this is a Mach-O binary by looking at the file extension or trying both
get_filename_component(_ext "${INPUT_FILE}" LAST_EXT)
if("${_ext}" STREQUAL "" OR "${INPUT_FILE}" MATCHES "macos")
    # Try Mach-O section name first
    execute_process(
        COMMAND llvm-objcopy "--dump-section=__TEXT,__text=${_bin}" "${INPUT_FILE}"
        ERROR_VARIABLE _err
        RESULT_VARIABLE _res
    )
    if(_res EQUAL 0)
        set(_is_macho TRUE)
        set(_section_name "__TEXT,__text")
    endif()
endif()

# =============================================================================
# Extract raw code section
# =============================================================================
if(NOT _is_macho)
    set(_dump_arg "--dump-section=${_section_name}=${_bin}")
    execute_process(
        COMMAND llvm-objcopy ${_dump_arg} "${INPUT_FILE}"
        ERROR_VARIABLE _err
        RESULT_VARIABLE _res
    )
    if(NOT _res EQUAL 0)
        message(FATAL_ERROR "llvm-objcopy failed: ${_err}")
    endif()
endif()

# =============================================================================
# Generate disassembly
# =============================================================================
if(_is_macho)
    set(_objdump_section "__TEXT,__text")
else()
    set(_objdump_section ".text")
endif()

execute_process(
    COMMAND llvm-objdump -d -s -h -j ${_objdump_section} "${INPUT_FILE}"
    OUTPUT_FILE "${_txt}"
    ERROR_VARIABLE _err
    RESULT_VARIABLE _res
)
if(NOT _res EQUAL 0)
    message(FATAL_ERROR "llvm-objdump failed: ${_err}")
endif()

# =============================================================================
# Extract strings
# =============================================================================
execute_process(
    COMMAND llvm-strings "${INPUT_FILE}"
    OUTPUT_FILE "${_strings}"
    ERROR_VARIABLE _err
    RESULT_VARIABLE _res
)
if(NOT _res EQUAL 0)
    message(FATAL_ERROR "llvm-strings failed: ${_err}")
endif()
