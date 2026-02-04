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
# Verify entry_point is first function in .text section
# =============================================================================
set(entry_point_first FALSE)

# Windows/UEFI (PE format): Check .text$entry_point is at offset 0
# Format: "0001:00000000 <size>H .text$entry_point"
if(_content MATCHES "0001:00000000[ \t]+[0-9a-fA-F]+H[ \t]+\\.text\\$entry_point")
    set(entry_point_first TRUE)
endif()

# Linux (ELF format): Check entry_point is first symbol in .text section
# Format: "VMA LMA Size Align Out In Symbol" with entry_point immediately after .text section
if(_content MATCHES "\\.text\n[^\n]+\\(\\.text\\.entry_point\\)")
    set(entry_point_first TRUE)
endif()

if(NOT entry_point_first)
    message(FATAL_ERROR
        "CRITICAL: entry_point is not the first function in .text section!\n\n"
        "The entry point must be at the beginning of the code section.\n"
        "Check that cmake/function.order contains 'entry_point' and is being used.\n\n"
        "Map file: ${MAP_FILE}"
    )
endif()

# =============================================================================
# Verify no forbidden data sections
# =============================================================================
set(_sections rdata rodata data bss)
set(_found)

foreach(_sec ${_sections})
    # Windows/UEFI (PE format): "0001:offset sizeH .section"
    if(_content MATCHES "[0-9a-fA-F]+:[0-9a-fA-F]+[ \t]+[0-9a-fA-F]+H[ \t]+\\.${_sec}[ \t\n]")
        list(APPEND _found ".${_sec}")
    # Linux (ELF format): "VMA LMA Size Align .section"
    elseif(_content MATCHES "[ \t]+[0-9a-fA-F]+[ \t]+[0-9a-fA-F]+[ \t]+[0-9a-fA-F]+[ \t]+[0-9]+[ \t]+\\.${_sec}[ \t\n]")
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

message(STATUS "Position-Independent verification tests passed.")
