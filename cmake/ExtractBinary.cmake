# =============================================================================
# ExtractBinary.cmake - Extract and analyze binary output (run via cmake -P)
# =============================================================================
# Usage: cmake -DINPUT_FILE=<exe/elf> -DOUTPUT_DIR=<dir> -P ExtractBinary.cmake
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
# Extract raw .text section
# =============================================================================
set(_dump_arg "--dump-section=.text=${_bin}")
execute_process(
    COMMAND llvm-objcopy ${_dump_arg} "${INPUT_FILE}"
    ERROR_VARIABLE _err
    RESULT_VARIABLE _res
)
if(NOT _res EQUAL 0)
    message(FATAL_ERROR "llvm-objcopy failed: ${_err}")
endif()

# =============================================================================
# Generate disassembly
# =============================================================================
execute_process(
    COMMAND llvm-objdump -d -s -h -j .text "${INPUT_FILE}"
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
