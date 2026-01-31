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
foreach(lang C CXX)
    set(CMAKE_${lang}_COMPILER_FORCED TRUE)
    set(CMAKE_${lang}_COMPILER_WORKS TRUE)
    set(CMAKE_${lang}_STANDARD_LIBRARIES "")
endforeach()

# Use GNU-style LLD linker
set(CMAKE_LINKER lld)
set(CMAKE_CXX_LINK_EXECUTABLE "<CMAKE_CXX_COMPILER> -fuse-ld=lld <LINK_FLAGS> <OBJECTS> -o <TARGET>")
set(CMAKE_C_LINK_EXECUTABLE "<CMAKE_C_COMPILER> -fuse-ld=lld <LINK_FLAGS> <OBJECTS> -o <TARGET>")

project(cpp-pic LANGUAGES CXX)

# =============================================================================
# Configuration Options
# =============================================================================
set(ARCHITECTURE "x86_64" CACHE STRING "Target architecture: i386, x86_64, armv7a, aarch64")
set(PLATFORM "windows" CACHE STRING "Target platform: windows, linux")
set(BUILD_TYPE "release" CACHE STRING "Build type: debug, release")
set(OPTIMIZATION_LEVEL "" CACHE STRING "Optimization level: O0, O1, O2, O3, Os, Oz, Og (default: Og for debug, O3 for release)")

# Normalize inputs to lowercase
string(TOLOWER "${ARCHITECTURE}" ARCHITECTURE_LC)
string(TOLOWER "${PLATFORM}" PLATFORM_LC)
string(TOLOWER "${BUILD_TYPE}" BUILD_TYPE_LC)

# Validate inputs
set(VALID_ARCHITECTURES "i386" "x86_64" "armv7a" "aarch64")
set(VALID_PLATFORMS "windows" "linux")
set(VALID_BUILD_TYPES "debug" "release")

if(NOT ARCHITECTURE_LC IN_LIST VALID_ARCHITECTURES)
    message(FATAL_ERROR "Invalid ARCHITECTURE: ${ARCHITECTURE}. Must be one of: ${VALID_ARCHITECTURES}")
endif()
if(NOT PLATFORM_LC IN_LIST VALID_PLATFORMS)
    message(FATAL_ERROR "Invalid PLATFORM: ${PLATFORM}. Must be one of: ${VALID_PLATFORMS}")
endif()
if(NOT BUILD_TYPE_LC IN_LIST VALID_BUILD_TYPES)
    message(FATAL_ERROR "Invalid BUILD_TYPE: ${BUILD_TYPE}. Must be one of: ${VALID_BUILD_TYPES}")
endif()

# =============================================================================
# Output Directory Configuration
# =============================================================================
# Hierarchical structure: build/buildtype/platform/arch/
# This structure groups by build type first, making it easier to manage
# different platforms and architectures for the same build configuration
set(BUILD_ROOT "${CMAKE_SOURCE_DIR}/build/${BUILD_TYPE_LC}/${PLATFORM_LC}/${ARCHITECTURE_LC}")
set(OUTPUT_DIR "${BUILD_ROOT}")

# =============================================================================
# Architecture-Specific Target Triple
# =============================================================================
if(PLATFORM_LC STREQUAL "windows")
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
elseif(PLATFORM_LC STREQUAL "linux")
    if(ARCHITECTURE_LC STREQUAL "i386")
        set(TARGET_TRIPLE "i386-unknown-linux-gnu")
        set(ARCH_DEFINES ARCHITECTURE_I386 PLATFORM_LINUX PLATFORM_LINUX_I386)
    elseif(ARCHITECTURE_LC STREQUAL "x86_64")
        set(TARGET_TRIPLE "x86_64-unknown-linux-gnu")
        set(ARCH_DEFINES ARCHITECTURE_X86_64 PLATFORM_LINUX PLATFORM_LINUX_X86_64)
    elseif(ARCHITECTURE_LC STREQUAL "armv7a")
        set(TARGET_TRIPLE "armv7a-unknown-linux-gnueabihf")
        set(ARCH_DEFINES ARCHITECTURE_ARMV7A PLATFORM_LINUX PLATFORM_LINUX_ARMV7A)
    elseif(ARCHITECTURE_LC STREQUAL "aarch64")
        set(TARGET_TRIPLE "aarch64-unknown-linux-gnu")
        set(ARCH_DEFINES ARCHITECTURE_AARCH64 PLATFORM_LINUX PLATFORM_LINUX_AARCH64)
    endif()
endif()

# =============================================================================
# Optimization Flags
# =============================================================================
# Determine optimization level (user-specified or default)
if(OPTIMIZATION_LEVEL STREQUAL "")
    # No user-specified optimization level, use defaults
    if(BUILD_TYPE_LC STREQUAL "debug")
        set(OPT_LEVEL "Og")
    else()
        set(OPT_LEVEL "O3")
    endif()
else()
    # Use user-specified optimization level
    set(OPT_LEVEL "${OPTIMIZATION_LEVEL}")
endif()

if(BUILD_TYPE_LC STREQUAL "debug")
    # Debug optimization flags:
    #   -Og              - Optimize for debugging experience (minimal optimizations)
    #   -ferror-limit=200 - Show up to 200 errors before stopping
    set(OPTIMIZATION_FLAGS "-${OPT_LEVEL}" "-ferror-limit=200")
    list(APPEND ARCH_DEFINES DEBUG)
else()
    # Release optimization flags:
    #   -O3 - Maximum optimization (aggressive inlining, vectorization, loop opts)
    #   -O2 - Moderate optimization (good balance)
    #   -Os - Optimize for size
    #   -Oz - Optimize aggressively for size
    set(OPTIMIZATION_FLAGS "-${OPT_LEVEL}")
endif()


# =============================================================================
# Source Files
# =============================================================================
set(INCLUDE_PATHS
    ${CMAKE_SOURCE_DIR}/
    ${CMAKE_SOURCE_DIR}/include/
    # BAL - Base Abstraction Layer (platform-independent)
    ${CMAKE_SOURCE_DIR}/include/bal/
    ${CMAKE_SOURCE_DIR}/include/bal/core/
    ${CMAKE_SOURCE_DIR}/include/bal/algorithms/
    ${CMAKE_SOURCE_DIR}/include/bal/types/
    ${CMAKE_SOURCE_DIR}/include/bal/types/numeric/
    ${CMAKE_SOURCE_DIR}/include/bal/types/embedded/
    ${CMAKE_SOURCE_DIR}/include/bal/types/network/
    ${CMAKE_SOURCE_DIR}/include/bal/string/
    # PAL - Platform Abstraction Layer (OS/hardware)
    ${CMAKE_SOURCE_DIR}/include/pal/
    ${CMAKE_SOURCE_DIR}/include/pal/memory/
    ${CMAKE_SOURCE_DIR}/include/pal/io/
    ${CMAKE_SOURCE_DIR}/include/pal/network/
    ${CMAKE_SOURCE_DIR}/include/pal/system/
    # RAL - Runtime Abstraction Layer (application features)
    ${CMAKE_SOURCE_DIR}/include/ral/
    ${CMAKE_SOURCE_DIR}/include/ral/crypt/
    ${CMAKE_SOURCE_DIR}/include/ral/network/
    ${CMAKE_SOURCE_DIR}/include/ral/network/tls/
    ${CMAKE_SOURCE_DIR}/tests/
)

# Add platform-specific include paths
if(PLATFORM_LC STREQUAL "windows")
    list(APPEND INCLUDE_PATHS ${CMAKE_SOURCE_DIR}/include/pal/windows/)
elseif(PLATFORM_LC STREQUAL "linux")
    list(APPEND INCLUDE_PATHS ${CMAKE_SOURCE_DIR}/include/pal/linux/)
endif()

# Collect all source files from src/ directory
file(GLOB_RECURSE SOURCES ${CMAKE_SOURCE_DIR}/src/*.cc)

# Collect header files for dependency tracking
# CMake doesn't auto-track headers with Generic system, so add them explicitly
file(GLOB_RECURSE HEADERS
    ${CMAKE_SOURCE_DIR}/include/*.h
    ${CMAKE_SOURCE_DIR}/tests/*.h
)

# Exclude platform-specific files for non-target platforms
if(PLATFORM_LC STREQUAL "windows")
    list(FILTER SOURCES EXCLUDE REGEX ".*/linux/.*")
elseif(PLATFORM_LC STREQUAL "linux")
    list(FILTER SOURCES EXCLUDE REGEX ".*/windows/.*")
endif()

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
        -g3                                 # Maximum debug information
        -fno-omit-frame-pointer             # Keep frame pointer for stack traces
    )
    # Platform-specific debug info format
    if(PLATFORM_LC STREQUAL "windows")
        list(APPEND BASE_COMPILER_FLAGS
            -gcodeview                      # CodeView debug info (WinDbg/VS)
        )
    endif()
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

# =============================================================================
# Platform-Specific Linker Flags
# =============================================================================
if(PLATFORM_LC STREQUAL "windows")
    # Windows linker flags (passed to LLD via -Wl):
    #   /Entry:_start        - Use _start as entry point (bypass CRT)
    #   /SUBSYSTEM:CONSOLE   - Console application
    #   /ORDER:@orderfile    - Control function placement in .text
    #   /MERGE:.rdata=.text  - CRITICAL: Merge read-only data into code for PIC
    set(LINKER_BASE "-Wl,/Entry:_start,/SUBSYSTEM:CONSOLE,/ORDER:@${CMAKE_SOURCE_DIR}/orderfile.txt,/MERGE:.rdata=.text")

    # =============================================================================
    # Architecture-Specific Linker Flags (Windows)
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
    # Build Type Specific Linker Flags (Windows)
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

    set(OUTPUT_EXTENSION ".exe")

elseif(PLATFORM_LC STREQUAL "linux")
    # Linux linker flags (ELF format):
    #   -e _start               - Use _start as entry point
    #   -T linker_script.txt    - Use linker script to place _start first
    #   --build-id=none         - Don't add build ID section
    set(LINKER_BASE "-Wl,-e,_start,-T,${CMAKE_SOURCE_DIR}/linker_script.txt,--build-id=none")

    # =============================================================================
    # Build Type Specific Linker Flags (Linux)
    # =============================================================================
    if(BUILD_TYPE_LC STREQUAL "debug")
        # Debug linker flags:
        #   -Map - Generate linker map file
        set(LINKER_FLAGS "${LINKER_BASE},-Map,${OUTPUT_DIR}/output.map.txt")
    else()
        # Release linker flags:
        #   --strip-all - Remove all symbols and debug info
        #   --gc-sections - Remove unused sections
        #   -Map - Generate linker map file
        set(LINKER_FLAGS "${LINKER_BASE},--strip-all,--gc-sections,-Map,${OUTPUT_DIR}/output.map.txt")
    endif()

    set(OUTPUT_EXTENSION ".elf")

endif()

# =============================================================================
# Create Output Directory
# =============================================================================
file(MAKE_DIRECTORY "${OUTPUT_DIR}")
foreach(config "" _DEBUG _RELEASE)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY${config} "${OUTPUT_DIR}")
    set(CMAKE_PDB_OUTPUT_DIRECTORY${config} "${OUTPUT_DIR}")
endforeach()

# =============================================================================
# Create Executable Target
# =============================================================================
add_executable(${TARGET_TRIPLE} ${SOURCES} ${HEADERS})

# Set target properties - use simple "output" name (directory provides context)
set_target_properties(${TARGET_TRIPLE} PROPERTIES
    OUTPUT_NAME "output"
    SUFFIX "${OUTPUT_EXTENSION}"
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
set(OUTPUT_EXE "${OUTPUT_DIR}/output${OUTPUT_EXTENSION}")
set(OUTPUT_BIN "${OUTPUT_DIR}/output.bin")
set(OUTPUT_TXT "${OUTPUT_DIR}/output.txt")
set(OUTPUT_STR "${OUTPUT_DIR}/output.strings.txt")
set(OUTPUT_B64 "${OUTPUT_DIR}/output.b64.txt")
set(OUTPUT_MAP "${OUTPUT_DIR}/output.map.txt")

# Generate inline verification script for .rdata/.rodata/.data/.bss checking
set(VERIFY_RDATA_SCRIPT "${CMAKE_BINARY_DIR}/verify_no_rdata.cmake")
file(WRITE "${VERIFY_RDATA_SCRIPT}" [[
# Verify no .rdata/.rodata/.data/.bss sections exist in the binary
if(NOT DEFINED MAP_FILE)
    message(FATAL_ERROR "MAP_FILE must be defined")
endif()

# Map file may not exist for debug builds
if(NOT EXISTS "${MAP_FILE}")
    message(STATUS "Map file not found (expected for debug): ${MAP_FILE}")
    return()
endif()

file(READ "${MAP_FILE}" MAP_CONTENT)

# Check for .rdata/.rodata/.data/.bss sections (both input and output)
# Windows PE format uses .rdata/.data/.bss
# Linux ELF uses .rodata/.data/.bss
string(REGEX MATCH "[ \t]+[0-9a-fA-F]+:[0-9a-fA-F]+[ \t]+[0-9a-fA-F]+H[ \t]+\\.rdata[ \t]" RDATA_IN "${MAP_CONTENT}")
string(REGEX MATCH "[ \t]+[0-9a-fA-F]+[ \t]+[0-9a-fA-F]+[ \t]+[0-9a-fA-F]+[ \t]+[0-9]+[ \t]+\\.rdata[ \t]" RDATA_OUT "${MAP_CONTENT}")
string(REGEX MATCH "[ \t]+[0-9a-fA-F]+[ \t]+[0-9a-fA-F]+[ \t]+[0-9a-fA-F]+[ \t]+[0-9]+[ \t]+\\.rodata[^.]" RODATA_OUT "${MAP_CONTENT}")
string(REGEX MATCH "[ \t]+[0-9a-fA-F]+:[0-9a-fA-F]+[ \t]+[0-9a-fA-F]+H[ \t]+\\.data[ \t]" DATA_IN "${MAP_CONTENT}")
string(REGEX MATCH "[ \t]+[0-9a-fA-F]+[ \t]+[0-9a-fA-F]+[ \t]+[0-9a-fA-F]+[ \t]+[0-9]+[ \t]+\\.data[ \t]" DATA_OUT "${MAP_CONTENT}")
string(REGEX MATCH "[ \t]+[0-9a-fA-F]+:[0-9a-fA-F]+[ \t]+[0-9a-fA-F]+H[ \t]+\\.bss[ \t]" BSS_IN "${MAP_CONTENT}")
string(REGEX MATCH "[ \t]+[0-9a-fA-F]+[ \t]+[0-9a-fA-F]+[ \t]+[0-9a-fA-F]+[ \t]+[0-9]+[ \t]+\\.bss[ \t]" BSS_OUT "${MAP_CONTENT}")

if(RDATA_IN OR RDATA_OUT OR RODATA_OUT OR DATA_IN OR DATA_OUT OR BSS_IN OR BSS_OUT)
    set(ERROR_MSG "CRITICAL: Data section detected that breaks position-independence!\n")
    if(RDATA_IN OR RDATA_OUT)
        set(ERROR_MSG "${ERROR_MSG}  - .rdata section found (Windows PE read-only data)\n")
    endif()
    if(RODATA_OUT)
        set(ERROR_MSG "${ERROR_MSG}  - .rodata section found (Linux ELF read-only data)\n")
    endif()
    if(DATA_IN OR DATA_OUT)
        set(ERROR_MSG "${ERROR_MSG}  - .data section found (initialized writable data)\n")
    endif()
    if(BSS_IN OR BSS_OUT)
        set(ERROR_MSG "${ERROR_MSG}  - .bss section found (uninitialized writable data)\n")
    endif()
    message(FATAL_ERROR
        "${ERROR_MSG}"
        "This breaks position-independence. Check for:\n"
        "  - Static/global variables (use stack allocation or EMBEDDED_* types)\n"
        "  - Uninitialized static/global variables (causes .bss section)\n"
        "  - Floating-point constants not using EMBEDDED_DOUBLE\n"
        "  - String literals not using EMBEDDED_STRING\n"
        "  - Array literals not using EMBEDDED_ARRAY\n"
        "Map file: ${MAP_FILE}"
    )
endif()

message(STATUS "Verification PASSED: No .rdata/.rodata/.data/.bss sections found")
]])

# Generate inline Base64 encoding script
set(BASE64_ENCODE_SCRIPT "${CMAKE_BINARY_DIR}/base64_encode.cmake")
file(WRITE "${BASE64_ENCODE_SCRIPT}" [[
# Base64 encode script - cross-platform
if(NOT DEFINED PIC_FILE OR NOT DEFINED BASE64_FILE)
    message(FATAL_ERROR "PIC_FILE and BASE64_FILE required")
endif()
if(NOT EXISTS "${PIC_FILE}")
    message(FATAL_ERROR "Input file not found: ${PIC_FILE}")
endif()

if(WIN32)
    execute_process(COMMAND certutil -encodehex -f "${PIC_FILE}" "${BASE64_FILE}" 0x40000001 RESULT_VARIABLE RES)
else()
    # Try GNU base64 first, fallback to BSD
    execute_process(COMMAND base64 -w 0 "${PIC_FILE}" OUTPUT_FILE "${BASE64_FILE}" ERROR_QUIET RESULT_VARIABLE RES)
    if(NOT RES EQUAL 0)
        execute_process(COMMAND base64 "${PIC_FILE}" OUTPUT_VARIABLE B64 RESULT_VARIABLE RES)
        string(REPLACE "\n" "" B64 "${B64}")
        file(WRITE "${BASE64_FILE}" "${B64}")
    endif()
    file(APPEND "${BASE64_FILE}" "\n")
endif()

if(NOT RES EQUAL 0)
    message(WARNING "Base64 encoding may have failed: ${RES}")
endif()
]])

add_custom_command(TARGET ${TARGET_TRIPLE} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E echo "Build SUCCEEDED: ${OUTPUT_EXE}"

    # Generate object dump (disassembly + section headers + .text content)
    COMMAND llvm-objdump -d -s -h -j .text ${OUTPUT_EXE} > ${OUTPUT_TXT}

    # Extract .text section as PIC blob
    COMMAND llvm-objcopy "--dump-section=.text=${OUTPUT_BIN}" ${OUTPUT_EXE}

    # Generate strings file (verify no embedded string literals)
    COMMAND llvm-strings ${OUTPUT_EXE} > ${OUTPUT_STR}

    # Base64 encode the PIC blob
    COMMAND ${CMAKE_COMMAND} -DPIC_FILE=${OUTPUT_BIN} -DBASE64_FILE=${OUTPUT_B64} -P "${BASE64_ENCODE_SCRIPT}"

    # Verify no .rdata section exists (critical for PIC)
    COMMAND ${CMAKE_COMMAND} -DMAP_FILE=${OUTPUT_MAP} -P "${VERIFY_RDATA_SCRIPT}"

    COMMENT "Generating PIC blob and analysis files..."
    VERBATIM
)

# =============================================================================
# Configuration Summary
# =============================================================================
message(STATUS "")
message(STATUS "=== Build Configuration ===")
message(STATUS "BUILD_TYPE:       ${BUILD_TYPE_LC}")
message(STATUS "PLATFORM:         ${PLATFORM_LC}")
message(STATUS "ARCHITECTURE:     ${ARCHITECTURE_LC}")
message(STATUS "OPTIMIZATION:     -${OPT_LEVEL}")
message(STATUS "TARGET_TRIPLE:    ${TARGET_TRIPLE}")
message(STATUS "OUTPUT_DIR:       ${OUTPUT_DIR}")
message(STATUS "OUTPUT:           ${OUTPUT_EXE}")
message(STATUS "===========================")
message(STATUS "")
