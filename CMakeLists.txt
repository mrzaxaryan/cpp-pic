cmake_minimum_required(VERSION 3.20)

# This project requires the toolchain file to be used:
#   cmake -B build -G Ninja -DCMAKE_TOOLCHAIN_FILE=cmake/toolchain-clang.cmake
#
# The toolchain file sets CMAKE_SYSTEM_NAME to Generic to avoid platform-specific
# CMake behavior that would otherwise add unwanted flags.

if(NOT CMAKE_TOOLCHAIN_FILE)
    message(FATAL_ERROR
        "This project requires a toolchain file. Please configure with:\n"
        "  cmake -B build -G Ninja -DCMAKE_TOOLCHAIN_FILE=cmake/toolchain-clang.cmake\n"
    )
endif()

project(cpp-pic LANGUAGES CXX)

# =============================================================================
# Configuration Options
# =============================================================================
set(ARCHITECTURE "x86_64" CACHE STRING "Target architecture: i386, x86_64, armv7a, aarch64")
set(PLATFORM "windows" CACHE STRING "Target platform: windows, linux")
set(BUILD_TYPE "release" CACHE STRING "Build type: debug, release")

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
# Hierarchical structure: build/<platform>/<arch>/<buildtype>/
set(OUTPUT_DIR "${CMAKE_SOURCE_DIR}/build/${PLATFORM_LC}/${ARCHITECTURE_LC}/${BUILD_TYPE_LC}")

# =============================================================================
# Platform and Architecture-Specific Configuration
# =============================================================================
if(PLATFORM_LC STREQUAL "windows")
    # -------------------------------------------------------------------------
    # Windows Platform Configuration
    # -------------------------------------------------------------------------
    set(OUTPUT_SUFFIX ".exe")

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

    # Windows-specific include paths
    set(PLATFORM_INCLUDE_PATHS
        ${CMAKE_SOURCE_DIR}/include/runtime/platform/windows/
    )

elseif(PLATFORM_LC STREQUAL "linux")
    # -------------------------------------------------------------------------
    # Linux Platform Configuration
    # -------------------------------------------------------------------------
    set(OUTPUT_SUFFIX ".elf")

    if(ARCHITECTURE_LC STREQUAL "i386")
        set(TARGET_TRIPLE "i386-unknown-linux-gnu")
        set(ARCH_DEFINES ARCHITECTURE_I386 PLATFORM_LINUX PLATFORM_LINUX_I386)
    elseif(ARCHITECTURE_LC STREQUAL "x86_64")
        set(TARGET_TRIPLE "x86_64-unknown-linux-gnu")
        set(ARCH_DEFINES ARCHITECTURE_X86_64 PLATFORM_LINUX PLATFORM_LINUX_X86_64)
    elseif(ARCHITECTURE_LC STREQUAL "armv7a")
        set(TARGET_TRIPLE "armv7a-unknown-linux-gnueabi")
        set(ARCH_DEFINES ARCHITECTURE_ARMV7A PLATFORM_LINUX PLATFORM_LINUX_ARMV7A)
    elseif(ARCHITECTURE_LC STREQUAL "aarch64")
        set(TARGET_TRIPLE "aarch64-unknown-linux-gnu")
        set(ARCH_DEFINES ARCHITECTURE_AARCH64 PLATFORM_LINUX PLATFORM_LINUX_AARCH64)
    endif()

    # Linux-specific include paths
    set(PLATFORM_INCLUDE_PATHS
        ${CMAKE_SOURCE_DIR}/include/runtime/platform/linux/
    )
endif()

# =============================================================================
# Common Include Paths
# =============================================================================
set(INCLUDE_PATHS
    ${CMAKE_SOURCE_DIR}/
    ${CMAKE_SOURCE_DIR}/include/
    ${CMAKE_SOURCE_DIR}/include/runtime/
    ${CMAKE_SOURCE_DIR}/include/runtime/platform/
    ${CMAKE_SOURCE_DIR}/include/runtime/primitives/
    ${CMAKE_SOURCE_DIR}/tests/
    ${PLATFORM_INCLUDE_PATHS}
)

# =============================================================================
# Source Files (Platform-Specific)
# =============================================================================
# Core source files
file(GLOB CORE_SOURCES
    ${CMAKE_SOURCE_DIR}/src/*.cc
    ${CMAKE_SOURCE_DIR}/src/runtime/console/*.cc
    ${CMAKE_SOURCE_DIR}/src/runtime/platform/*.cc
)

# Platform-specific source files
if(PLATFORM_LC STREQUAL "windows")
    file(GLOB PLATFORM_SOURCES
        ${CMAKE_SOURCE_DIR}/src/runtime/console/windows/*.cc
        ${CMAKE_SOURCE_DIR}/src/runtime/platform/windows/*.cc
    )
elseif(PLATFORM_LC STREQUAL "linux")
    file(GLOB PLATFORM_SOURCES
        ${CMAKE_SOURCE_DIR}/src/runtime/console/linux/*.cc
        ${CMAKE_SOURCE_DIR}/src/runtime/platform/linux/*.cc
    )
endif()

set(SOURCES ${CORE_SOURCES} ${PLATFORM_SOURCES})

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
# Base Compiler Flags (All Platforms, All Architectures)
# =============================================================================
# These flags are essential for position-independent code without .rdata/.rodata dependencies
set(BASE_COMPILER_FLAGS
    -std=c++23                              # C++23 standard for consteval, concepts, fold expressions
    -fno-jump-tables                        # CRITICAL: Prevent switch jump tables in .rdata/.rodata
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
    -fshort-wchar                           # 2-byte wchar_t (matches Windows ABI)
)

# =============================================================================
# Architecture-Specific Compiler Flags
# =============================================================================
if(ARCHITECTURE_LC STREQUAL "i386" OR ARCHITECTURE_LC STREQUAL "x86_64")
    # Windows x86 specific - disable SSE and use software float
    if(PLATFORM_LC STREQUAL "windows")
        list(APPEND BASE_COMPILER_FLAGS
            -mno-sse                        # Disable SSE (avoid xmm register usage)
            -mno-stack-arg-probe            # Disable __chkstk calls (Windows x86 only)
            -msoft-float                    # Software floating-point ABI
        )
    endif()
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
    if(PLATFORM_LC STREQUAL "windows")
        list(APPEND BASE_COMPILER_FLAGS
            -gcodeview                      # CodeView debug info (WinDbg/VS)
        )
    endif()
    list(APPEND BASE_COMPILER_FLAGS
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
# Linker Flags (Platform-Specific)
# =============================================================================
set(BASE_LINKER_FLAGS
    -fuse-ld=lld                            # LLVM LLD linker
    -nostdlib                               # No standard libraries (no CRT startup)
)

if(PLATFORM_LC STREQUAL "windows")
    # -------------------------------------------------------------------------
    # Windows Linker Flags (LLD-link, PE format)
    # -------------------------------------------------------------------------
    # Windows linker flags (passed to LLD via -Wl):
    #   /Entry:_start        - Use _start as entry point (bypass CRT)
    #   /SUBSYSTEM:CONSOLE   - Console application
    #   /ORDER:@orderfile    - Control function placement in .text
    #   /MERGE:.rdata=.text  - Merge read-only data into code for PIC
    set(LINKER_BASE "-Wl,/Entry:_start,/SUBSYSTEM:CONSOLE,/ORDER:@${CMAKE_SOURCE_DIR}/orderfile.txt,/MERGE:.rdata=.text")

    # Architecture-specific Windows linker flags
    if(ARCHITECTURE_LC STREQUAL "i386")
        # i386-specific linker flags:
        #   /BASE:0x400000    - Preferred load address (standard for 32-bit Windows)
        #   /FILEALIGN:0x1000 - File section alignment (4KB pages)
        #   /SAFESEH:NO       - Disable SafeSEH
        set(LINKER_EXTRA ",/BASE:0x400000,/FILEALIGN:0x1000,/SAFESEH:NO")
    else()
        set(LINKER_EXTRA "")
    endif()

    # Build type specific Windows linker flags
    if(BUILD_TYPE_LC STREQUAL "debug")
        set(LINKER_FLAGS "${LINKER_BASE}${LINKER_EXTRA},/DEBUG,/MERGE:.rdata=.text,/MAP:${OUTPUT_DIR}/output.map.txt")
    else()
        set(LINKER_FLAGS "${LINKER_BASE}${LINKER_EXTRA},--strip-all,/OPT:REF,/OPT:ICF,/RELEASE,/LTCG,/MAP:${OUTPUT_DIR}/output.map.txt")
    endif()

elseif(PLATFORM_LC STREQUAL "linux")
    # -------------------------------------------------------------------------
    # Linux Linker Flags (LLD, ELF format)
    # -------------------------------------------------------------------------
    # Linux linker flags (passed to LLD via -Wl):
    #   -T linker.script  - Use custom linker script (merges .rodata into .text)
    #   -e _start         - Use _start as entry point
    set(LINKER_BASE "-Wl,-T,${CMAKE_SOURCE_DIR}/linker.script,-e,_start")

    # Build type specific Linux linker flags
    if(BUILD_TYPE_LC STREQUAL "debug")
        set(LINKER_FLAGS "${LINKER_BASE}")
    else()
        # Release: strip symbols and enable garbage collection
        set(LINKER_FLAGS "${LINKER_BASE},--strip-all,--gc-sections")
    endif()
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

# Set target properties
set_target_properties(${TARGET_TRIPLE} PROPERTIES
    OUTPUT_NAME "output"
    SUFFIX "${OUTPUT_SUFFIX}"
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
set(OUTPUT_FILE "${OUTPUT_DIR}/output${OUTPUT_SUFFIX}")
set(OUTPUT_BIN "${OUTPUT_DIR}/output.bin")
set(OUTPUT_TXT "${OUTPUT_DIR}/output.txt")
set(OUTPUT_STR "${OUTPUT_DIR}/output.strings.txt")
set(OUTPUT_B64 "${OUTPUT_DIR}/output.b64.txt")
set(OUTPUT_MAP "${OUTPUT_DIR}/output.map.txt")

add_custom_command(TARGET ${TARGET_TRIPLE} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E echo "Build SUCCEEDED: ${OUTPUT_FILE}"

    # Generate object dump (disassembly + section headers + .text content)
    COMMAND llvm-objdump -d -s -h -j .text ${OUTPUT_FILE} > ${OUTPUT_TXT}

    # Extract .text section as PIC blob
    COMMAND llvm-objcopy "--dump-section=.text=${OUTPUT_BIN}" ${OUTPUT_FILE}

    # Generate strings file (verify no embedded string literals)
    COMMAND llvm-strings ${OUTPUT_FILE} > ${OUTPUT_STR}

    # Base64 encode the PIC blob
    COMMAND ${CMAKE_COMMAND} -DPIC_FILE=${OUTPUT_BIN} -DBASE64_FILE=${OUTPUT_B64} -P "${CMAKE_SOURCE_DIR}/cmake/base64_encode.cmake"

    COMMENT "Generating PIC blob and analysis files..."
)

# =============================================================================
# Configuration Summary
# =============================================================================
message(STATUS "")
message(STATUS "=== Build Configuration ===")
message(STATUS "ARCHITECTURE:     ${ARCHITECTURE_LC}")
message(STATUS "PLATFORM:         ${PLATFORM_LC}")
message(STATUS "BUILD_TYPE:       ${BUILD_TYPE_LC}")
message(STATUS "TARGET_TRIPLE:    ${TARGET_TRIPLE}")
message(STATUS "OUTPUT_DIR:       ${OUTPUT_DIR}")
message(STATUS "OUTPUT:           output${OUTPUT_SUFFIX}")
message(STATUS "===========================")
message(STATUS "")
