cmake_minimum_required(VERSION 3.20)

# =============================================================================
# Toolchain Configuration (Clang for bare-metal/PIC compilation)
# =============================================================================
# CMAKE_SYSTEM_NAME is set to Generic to avoid Windows/MSVC-specific
# CMake behavior that would otherwise add unwanted flags like -fuse-ld=lld-link and
# -Xclang --dependent-lib=msvcrtd.

# Use Generic system to prevent Windows-specific linker configuration
set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR x86_64)

# Use environment variables if set, otherwise default to clang/clang++
if(DEFINED ENV{CC})
    set(CMAKE_C_COMPILER $ENV{CC})
else()
    set(CMAKE_C_COMPILER clang)
endif()

if(DEFINED ENV{CXX})
    set(CMAKE_CXX_COMPILER $ENV{CXX})
else()
    set(CMAKE_CXX_COMPILER clang++)
endif()

# Force compilers to be found without testing
set(CMAKE_C_COMPILER_FORCED TRUE)
set(CMAKE_CXX_COMPILER_FORCED TRUE)
set(CMAKE_C_COMPILER_WORKS TRUE)
set(CMAKE_CXX_COMPILER_WORKS TRUE)

# Disable standard library linking
set(CMAKE_CXX_STANDARD_LIBRARIES "")
set(CMAKE_C_STANDARD_LIBRARIES "")

# Use GNU-style LLD linker
set(CMAKE_LINKER lld)
set(CMAKE_CXX_LINK_EXECUTABLE "<CMAKE_CXX_COMPILER> -fuse-ld=lld <LINK_FLAGS> <OBJECTS> -o <TARGET>")
set(CMAKE_C_LINK_EXECUTABLE "<CMAKE_C_COMPILER> -fuse-ld=lld <LINK_FLAGS> <OBJECTS> -o <TARGET>")

project(cpp-pic LANGUAGES CXX)

# =============================================================================
# Configuration Options
# =============================================================================
set(ARCHITECTURE "x86_64" CACHE STRING "Target architecture: i386, x86_64, armv7a, aarch64")
set(PLATFORM "windows" CACHE STRING "Target platform (windows only, parameter kept for compatibility)")
set(BUILD_TYPE "release" CACHE STRING "Build type: debug, release")

# Normalize inputs to lowercase (except BUILD_TYPE keeps original case for output dir)
string(TOLOWER "${ARCHITECTURE}" ARCHITECTURE_LC)
string(TOLOWER "${BUILD_TYPE}" BUILD_TYPE_LC)
set(BUILD_TYPE_DIR "${BUILD_TYPE}")  # Preserve case for output directory

# Validate inputs
set(VALID_ARCHITECTURES "i386" "x86_64" "armv7a" "aarch64")
set(VALID_BUILD_TYPES "debug" "release")

if(NOT ARCHITECTURE_LC IN_LIST VALID_ARCHITECTURES)
    message(FATAL_ERROR "Invalid ARCHITECTURE: ${ARCHITECTURE}. Must be one of: ${VALID_ARCHITECTURES}")
endif()
if(NOT BUILD_TYPE_LC IN_LIST VALID_BUILD_TYPES)
    message(FATAL_ERROR "Invalid BUILD_TYPE: ${BUILD_TYPE}. Must be one of: ${VALID_BUILD_TYPES}")
endif()

# =============================================================================
# Output Directory Configuration
# =============================================================================
# Hierarchical structure: build/windows/arch/buildtype/
set(BUILD_ROOT "${CMAKE_SOURCE_DIR}/build/windows/${ARCHITECTURE_LC}/${BUILD_TYPE_LC}")
set(OUTPUT_DIR "${BUILD_ROOT}")

# =============================================================================
# Architecture-Specific Target Triple
# =============================================================================
if(ARCHITECTURE_LC STREQUAL "i386")
    set(TARGET_TRIPLE "i386-pc-windows-gnu")
    set(ARCH_DEFINES ARCHITECTURE_I386 PLATFORM_WINDOWS PLATFORM_WINDOWS_I386)
elseif(ARCHITECTURE_LC STREQUAL "x86_64")
    set(TARGET_TRIPLE "x86_64-pc-windows-gnu")
    set(ARCH_DEFINES ARCHITECTURE_X86_64 PLATFORM_WINDOWS PLATFORM_WINDOWS_X86_64)
elseif(ARCHITECTURE_LC STREQUAL "armv7a")
    set(TARGET_TRIPLE "armv7a-pc-windows-gnu")
    set(ARCH_DEFINES ARCHITECTURE_ARMV7A PLATFORM_WINDOWS PLATFORM_WINDOWS_ARMV7A)
elseif(ARCHITECTURE_LC STREQUAL "aarch64")
    set(TARGET_TRIPLE "aarch64-pc-windows-gnu")
    set(ARCH_DEFINES ARCHITECTURE_AARCH64 PLATFORM_WINDOWS PLATFORM_WINDOWS_AARCH64)
endif()

# =============================================================================
# Optimization Flags
# =============================================================================
if(BUILD_TYPE_LC STREQUAL "debug")
    # Debug optimization flags:
    #   -Og              - Optimize for debugging experience (minimal optimizations)
    #   -ferror-limit=200 - Show up to 200 errors before stopping
    set(OPTIMIZATION_FLAGS "-Og" "-ferror-limit=200")
    list(APPEND ARCH_DEFINES DEBUG)
else()
    # Release optimization flag:
    #   -O3 - Maximum optimization (aggressive inlining, vectorization, loop opts)
    set(OPTIMIZATION_FLAGS "-O3")
endif()


# =============================================================================
# Source Files
# =============================================================================
set(INCLUDE_PATHS
    ${CMAKE_SOURCE_DIR}/
    ${CMAKE_SOURCE_DIR}/include/
    # BAL - Base Abstraction Layer (platform-independent)
    ${CMAKE_SOURCE_DIR}/include/bal/
    ${CMAKE_SOURCE_DIR}/include/bal/primitives/
    # PAL - Platform Abstraction Layer (OS/hardware)
    ${CMAKE_SOURCE_DIR}/include/pal/
    ${CMAKE_SOURCE_DIR}/include/pal/windows/
    # RAL - Runtime Abstraction Layer (application features)
    ${CMAKE_SOURCE_DIR}/include/ral/
    ${CMAKE_SOURCE_DIR}/include/ral/crypt/
    ${CMAKE_SOURCE_DIR}/include/ral/network/
    ${CMAKE_SOURCE_DIR}/include/ral/network/tls/
    ${CMAKE_SOURCE_DIR}/tests/
)

# Collect all source files from src/ directory
file(GLOB_RECURSE SOURCES ${CMAKE_SOURCE_DIR}/src/*.cc)

# Exclude platform-specific files for non-target platforms
list(FILTER SOURCES EXCLUDE REGEX ".*/linux/.*")
list(FILTER SOURCES EXCLUDE REGEX ".*/uefi/.*")

# =============================================================================
# Base Compiler Flags (All Architectures)
# =============================================================================
# These flags are essential for position-independent code without .rdata dependencies
set(BASE_COMPILER_FLAGS
    -std=c++23                              # C++23 standard for consteval, concepts, fold expressions
    -fno-jump-tables                        # CRITICAL: Prevent switch jump tables in .rdata
    -Werror                                 # Treat all warnings as errors
    -Wall                                   # Enable all common warnings
    -Wextra                                 # Enable additional warnings
    -Wno-gnu-string-literal-operator-template # Suppress warning for _embed literals
    -Qn                                     # Suppress compiler identification
    -nostdlib                               # No standard C/C++ libraries (no CRT)
    -fno-ident                              # No .comment section with compiler version
    -fno-exceptions                         # Disable C++ exceptions (no .pdata/.xdata)
    -fno-rtti                               # Disable RTTI (no typeinfo in .rdata)
    -fno-stack-check                        # Disable stack limit checking
    -ffunction-sections                     # Each function in own section (dead code elimination)
    -fdata-sections                         # Each data item in own section (garbage collection)
    -fno-builtin                            # Disable compiler built-ins
    -fshort-wchar                           # 2-byte wchar_t (Windows ABI)
)

# =============================================================================
# Architecture-Specific Compiler Flags
# =============================================================================
if(ARCHITECTURE_LC STREQUAL "i386" OR ARCHITECTURE_LC STREQUAL "x86_64")
    # x86-specific flags
    list(APPEND BASE_COMPILER_FLAGS
        -mno-stack-arg-probe                # Disable __chkstk calls (x86 only)
        -msoft-float                        # Software floating-point ABI (x86 only)
    )
endif()

if(ARCHITECTURE_LC STREQUAL "armv7a" OR ARCHITECTURE_LC STREQUAL "aarch64")
    # ARM-specific flags
    list(APPEND BASE_COMPILER_FLAGS
        -mno-implicit-float                 # No implicit FP instructions
    )
endif()

# =============================================================================
# Build Type Specific Compiler Flags
# =============================================================================
if(BUILD_TYPE_LC STREQUAL "debug")
    # Debug-specific compiler flags
    list(APPEND BASE_COMPILER_FLAGS
        -gcodeview                          # CodeView debug info (WinDbg/VS)
        -g3                                 # Maximum debug information
        -fno-omit-frame-pointer             # Keep frame pointer for stack traces
    )
else()
    # Release-specific compiler flags (maximum optimization)
    list(APPEND BASE_COMPILER_FLAGS
        -fno-omit-frame-pointer             # Keep frame pointer (profiling/crash analysis)
        -fno-asynchronous-unwind-tables     # No .eh_frame section
        -fno-unwind-tables                  # No unwind tables
        -flto=full                          # Full link-time optimization
        -finline-functions                  # Aggressive inlining
        -funroll-loops                      # Unroll loops
        -fwhole-program-vtables             # LTO devirtualization
    )
endif()

# =============================================================================
# Base Linker Flags
# =============================================================================
set(BASE_LINKER_FLAGS
    -fuse-ld=lld                            # LLVM LLD linker
    -nostdlib                               # No standard libraries (no CRT startup)
)

# Windows linker flags (passed to LLD via -Wl):
#   /Entry:_start        - Use _start as entry point (bypass CRT)
#   /SUBSYSTEM:CONSOLE   - Console application
#   /ORDER:@orderfile    - Control function placement in .text
#   /MERGE:.rdata=.text  - CRITICAL: Merge read-only data into code for PIC
set(LINKER_BASE "-Wl,/Entry:_start,/SUBSYSTEM:CONSOLE,/ORDER:@${CMAKE_SOURCE_DIR}/orderfile.txt,/MERGE:.rdata=.text")

# =============================================================================
# Architecture-Specific Linker Flags
# =============================================================================
if(ARCHITECTURE_LC STREQUAL "i386")
    # i386-specific linker flags:
    #   /BASE:0x400000    - Preferred load address (standard for 32-bit Windows)
    #   /FILEALIGN:0x1000 - File section alignment (4KB pages)
    #   /SAFESEH:NO       - Disable SafeSEH
    set(LINKER_EXTRA ",/BASE:0x400000,/FILEALIGN:0x1000,/SAFESEH:NO")
else()
    set(LINKER_EXTRA "")
endif()

# =============================================================================
# Build Type Specific Linker Flags
# =============================================================================
if(BUILD_TYPE_LC STREQUAL "debug")
    # Debug linker flags:
    #   /DEBUG - Include debug information in PE
    #   /MAP   - Generate linker map file
    set(LINKER_FLAGS "${LINKER_BASE}${LINKER_EXTRA},/DEBUG,/MAP:${OUTPUT_DIR}/output.map.txt")
else()
    # Release linker flags:
    #   --strip-all - Remove all symbols and debug info
    #   /OPT:REF    - Remove unreferenced functions/data
    #   /OPT:ICF    - Fold identical code sequences
    #   /RELEASE    - Set release flag in PE header
    #   /LTCG       - Link-time code generation
    #   /MAP        - Generate linker map file
    set(LINKER_FLAGS "${LINKER_BASE}${LINKER_EXTRA},--strip-all,/OPT:REF,/OPT:ICF,/RELEASE,/LTCG,/MAP:${OUTPUT_DIR}/output.map.txt")
endif()

# =============================================================================
# Create Output Directory
# =============================================================================
file(MAKE_DIRECTORY "${OUTPUT_DIR}")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${OUTPUT_DIR}")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG "${OUTPUT_DIR}")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE "${OUTPUT_DIR}")
set(CMAKE_PDB_OUTPUT_DIRECTORY "${OUTPUT_DIR}")
set(CMAKE_PDB_OUTPUT_DIRECTORY_DEBUG "${OUTPUT_DIR}")
set(CMAKE_PDB_OUTPUT_DIRECTORY_RELEASE "${OUTPUT_DIR}")

# =============================================================================
# Create Executable Target
# =============================================================================
add_executable(${TARGET_TRIPLE} ${SOURCES})

# Set target properties - use simple "output" name (directory provides context)
set_target_properties(${TARGET_TRIPLE} PROPERTIES
    OUTPUT_NAME "output"
    SUFFIX ".exe"
)

# Include directories
target_include_directories(${TARGET_TRIPLE} PRIVATE ${INCLUDE_PATHS})

# Compile definitions
target_compile_definitions(${TARGET_TRIPLE} PRIVATE ${ARCH_DEFINES})

# Compile options
target_compile_options(${TARGET_TRIPLE} PRIVATE
    ${BASE_COMPILER_FLAGS}
    ${OPTIMIZATION_FLAGS}
    -target ${TARGET_TRIPLE}
)

# Link options
target_link_options(${TARGET_TRIPLE} PRIVATE
    -target ${TARGET_TRIPLE}
    ${BASE_LINKER_FLAGS}
    "SHELL:${LINKER_FLAGS}"
)

# =============================================================================
# Post-Build Commands (PIC Blob Generation & Analysis)
# =============================================================================
set(OUTPUT_EXE "${OUTPUT_DIR}/output.exe")
set(OUTPUT_BIN "${OUTPUT_DIR}/output.bin")
set(OUTPUT_TXT "${OUTPUT_DIR}/output.txt")
set(OUTPUT_STR "${OUTPUT_DIR}/output.strings.txt")
set(OUTPUT_B64 "${OUTPUT_DIR}/output.b64.txt")
set(OUTPUT_MAP "${OUTPUT_DIR}/output.map.txt")

# Generate inline verification script for .rdata checking
set(VERIFY_RDATA_SCRIPT "${CMAKE_BINARY_DIR}/verify_no_rdata.cmake")
file(WRITE "${VERIFY_RDATA_SCRIPT}" "
# Verify no .rdata section exists in the binary
# This verification ensures position-independence by confirming that no read-only
# data section (.rdata) was generated. The presence of .rdata would break PIC
# because it contains absolute addresses that cannot be relocated.

if(NOT DEFINED MAP_FILE)
    message(FATAL_ERROR \"MAP_FILE must be defined\")
endif()

# Map file may not exist for debug builds
if(NOT EXISTS \"\${MAP_FILE}\")
    message(STATUS \"Map file not found (expected for debug builds): \${MAP_FILE}\")
    message(STATUS \"Skipping .rdata verification\")
    return()
endif()

# Read the map file
file(READ \"\${MAP_FILE}\" MAP_CONTENT)

# Check for .rdata section (Windows PE format)
# We check for ANY .rdata section - both input and output sections
# Even if .rdata is merged into .text, its presence indicates problematic code generation
string(REGEX MATCH \"[ \\t]+[0-9a-fA-F]+:[0-9a-fA-F]+[ \\t]+[0-9a-fA-F]+H[ \\t]+\\\\.rdata[ \\t]\" RDATA_INPUT \"\${MAP_CONTENT}\")

# Also check for output sections (format without segment:offset)
string(REGEX MATCH \"[ \\t]+[0-9a-fA-F]+[ \\t]+[0-9a-fA-F]+[ \\t]+[0-9a-fA-F]+[ \\t]+[0-9]+[ \\t]+\\\\.rdata[ \\t]\" RDATA_OUTPUT \"\${MAP_CONTENT}\")

if(RDATA_INPUT)
    message(FATAL_ERROR
        \"CRITICAL: .rdata input section detected in map file!\\n\"
        \"Even though merged into .text, this indicates problematic code generation.\\n\"
        \"The presence of ANY .rdata breaks position-independence. Check for:\\n\"
        \"  - Static const variables with runtime initialization\\n\"
        \"  - Jump tables from switch statements (use -fno-jump-tables)\\n\"
        \"  - Floating-point constants not using EMBEDDED_DOUBLE\\n\"
        \"  - String literals not using EMBEDDED_STRING\\n\"
        \"  - RTTI or exception tables (use -fno-rtti -fno-exceptions)\\n\"
        \"Map file: \${MAP_FILE}\\n\"
        \"Matched line: \${RDATA_INPUT}\"
    )
endif()

if(RDATA_OUTPUT)
    message(FATAL_ERROR
        \"CRITICAL: .rdata OUTPUT section detected in binary!\\n\"
        \"This breaks position-independence. Check for:\\n\"
        \"  - Static const variables with runtime initialization\\n\"
        \"  - Jump tables from switch statements (use -fno-jump-tables)\\n\"
        \"  - RTTI or exception tables (use -fno-rtti -fno-exceptions)\\n\"
        \"Map file: \${MAP_FILE}\"
    )
endif()

# For ELF, check for .rodata as a top-level output section
string(REGEX MATCH \"[ \\t]+[0-9a-fA-F]+[ \\t]+[0-9a-fA-F]+[ \\t]+[0-9a-fA-F]+[ \\t]+[0-9]+[ \\t]+\\\\.rodata[^.]\" RODATA_OUTPUT \"\${MAP_CONTENT}\")

if(RODATA_OUTPUT)
    message(FATAL_ERROR
        \"CRITICAL: .rodata OUTPUT section detected in binary!\\n\"
        \"This breaks position-independence. Check for:\\n\"
        \"  - String literals (use EMBEDDED_STRING)\\n\"
        \"  - Static const variables\\n\"
        \"  - Jump tables from switch statements (use -fno-jump-tables)\\n\"
        \"Map file: \${MAP_FILE}\"
    )
endif()

message(STATUS \"Verification PASSED: No .rdata/.rodata output sections found\")
")

add_custom_command(TARGET ${TARGET_TRIPLE} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E echo "Build SUCCEEDED: ${OUTPUT_EXE}"

    # Generate object dump (disassembly + section headers + .text content)
    COMMAND llvm-objdump -d -s -h -j .text ${OUTPUT_EXE} > ${OUTPUT_TXT}

    # Extract .text section as PIC blob
    COMMAND llvm-objcopy "--dump-section=.text=${OUTPUT_BIN}" ${OUTPUT_EXE}

    # Generate strings file (verify no embedded string literals)
    COMMAND llvm-strings ${OUTPUT_EXE} > ${OUTPUT_STR}

    # Base64 encode the PIC blob
    COMMAND ${CMAKE_COMMAND} -DPIC_FILE=${OUTPUT_BIN} -DBASE64_FILE=${OUTPUT_B64} -P "${CMAKE_SOURCE_DIR}/cmake/base64_encode.cmake"

    # Verify no .rdata section exists (critical for PIC)
    COMMAND ${CMAKE_COMMAND} -DMAP_FILE=${OUTPUT_MAP} -P "${VERIFY_RDATA_SCRIPT}"

    COMMENT "Generating PIC blob and analysis files..."
)

# =============================================================================
# Configuration Summary
# =============================================================================
message(STATUS "")
message(STATUS "=== Build Configuration ===")
message(STATUS "ARCHITECTURE:     ${ARCHITECTURE_LC}")
message(STATUS "PLATFORM:         ${PLATFORM}")
message(STATUS "BUILD_TYPE:       ${BUILD_TYPE_LC}")
message(STATUS "TARGET_TRIPLE:    ${TARGET_TRIPLE}")
message(STATUS "OUTPUT_DIR:       ${OUTPUT_DIR}")
message(STATUS "OUTPUT:           output.exe")
message(STATUS "===========================")
message(STATUS "")
