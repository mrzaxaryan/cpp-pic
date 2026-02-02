# =============================================================================
# VerifyPICMode.cmake - Verify PIC requirements (run via cmake -P)
# =============================================================================
# Usage: cmake -DMAP_FILE=<path> -P VerifyPICMode.cmake
# =============================================================================

cmake_minimum_required(VERSION 3.20)

if(NOT DEFINED MAP_FILE)
    message(FATAL_ERROR "MAP_FILE is required")
endif()

if(NOT EXISTS "${MAP_FILE}")
    message(STATUS "Map file not found (expected for debug builds): ${MAP_FILE}")
    return()
endif()

file(READ "${MAP_FILE}" _content)

# =============================================================================
# Verify _start is first function in .text section
# =============================================================================
set(_start_first FALSE)

# Windows/UEFI (PE format): Check .text$_start is at offset 0
# Format: "0001:00000000 <size>H .text$_start"
if(_content MATCHES "0001:00000000[ \t]+[0-9a-fA-F]+H[ \t]+\\.text\\$_start")
    set(_start_first TRUE)
endif()

# Linux (ELF format): Check _start is first symbol in .text section
# Format: "VMA LMA Size Align Out In Symbol" with _start immediately after .text section
if(_content MATCHES "\\.text\n[^\n]+\\(\\.text\\._start\\)")
    set(_start_first TRUE)
endif()

if(NOT _start_first)
    message(FATAL_ERROR
        "CRITICAL: _start is not the first function in .text section!\n\n"
        "The entry point must be at the beginning of the code section.\n"
        "Check that orderfile.txt contains '_start' and is being used.\n\n"
        "Map file: ${MAP_FILE}"
    )
endif()

# =============================================================================
# Verify no forbidden data sections
# =============================================================================
set(_sections rdata rodata data bss)
set(_found)

foreach(_sec ${_sections})
    # Match both input sections (addr:offset sizeH) and output sections (vma size)
    if(_content MATCHES "[ \t]+[0-9a-fA-F]+[:\t ]+[0-9a-fA-F]+[H\t ]+[0-9a-fA-F]*[ \t]+\\.${_sec}[ \t\n]")
        list(APPEND _found ".${_sec}")
    endif()
endforeach()

if(_found)
    list(JOIN _found ", " _list)
    message(FATAL_ERROR
        "CRITICAL: Data sections break position-independence!\n"
        "Found: ${_list}\n\n"
        "Common causes:\n"
        "  - Static/global variables (use stack or EMBEDDED_* types)\n"
        "  - String literals (use EMBEDDED_STRING)\n"
        "  - Floating-point constants (use EMBEDDED_DOUBLE)\n"
        "  - Array literals (use EMBEDDED_ARRAY)\n\n"
        "Map file: ${MAP_FILE}"
    )
endif()

message(STATUS "PIC verification passed")
