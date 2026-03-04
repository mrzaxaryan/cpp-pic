# =============================================================================
# Target.cmake - Platform Selection, Target Definition, and Post-Build
# =============================================================================

include_guard(GLOBAL)

# =============================================================================
# Platform Modules
# =============================================================================
set(_platform_map_windows Windows)
set(_platform_map_linux Linux)
set(_platform_map_macos macOS)
set(_platform_map_uefi UEFI)
set(_platform_map_solaris Solaris)
set(_platform_map_freebsd FreeBSD)
include(${PIR_ROOT_DIR}/cmake/platforms/${_platform_map_${PIR_PLATFORM}}.cmake)

# Universal target flags (every platform uses -target for both compile and link)
list(APPEND PIR_BASE_FLAGS -target ${PIR_TRIPLE})
if(NOT PIR_DIRECT_LINKER)
    if(DEFINED PIR_LINK_TRIPLE)
        list(APPEND PIR_BASE_LINK_FLAGS -target ${PIR_LINK_TRIPLE})
    else()
        list(APPEND PIR_BASE_LINK_FLAGS -target ${PIR_TRIPLE})
    endif()
endif()

# =============================================================================
# Output Configuration
# =============================================================================
file(MAKE_DIRECTORY "${PIR_OUTPUT_DIR}")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${PIR_OUTPUT_DIR}")
set(CMAKE_PDB_OUTPUT_DIRECTORY "${PIR_OUTPUT_DIR}")

# =============================================================================
# Target
# =============================================================================
add_executable(${PIR_TRIPLE} ${PIR_SOURCES} ${PIR_HEADERS})

set_target_properties(${PIR_TRIPLE} PROPERTIES
    OUTPUT_NAME "output"
    SUFFIX "${PIR_EXT}"
)

target_include_directories(${PIR_TRIPLE} PRIVATE ${PIR_INCLUDE_PATHS})
target_compile_definitions(${PIR_TRIPLE} PRIVATE ${PIR_DEFINES})
target_compile_options(${PIR_TRIPLE} PRIVATE ${PIR_BASE_FLAGS})
target_link_options(${PIR_TRIPLE} PRIVATE ${PIR_BASE_LINK_FLAGS})

# =============================================================================
# Post-Build
# =============================================================================
pir_add_postbuild(${PIR_TRIPLE})

if(PIR_PLATFORM STREQUAL "uefi")
    pir_add_uefi_boot(${PIR_TRIPLE})
endif()

# =============================================================================
# Build Summary
# =============================================================================
message(STATUS "")
message(STATUS "┌─────────────────────────────────────┐")
message(STATUS "│      Build Configuration            │")
message(STATUS "├─────────────────────────────────────┤")
message(STATUS "│ Platform:     ${PIR_PLATFORM}")
message(STATUS "│ Architecture: ${PIR_ARCH}")
message(STATUS "│ Build Type:   ${PIR_BUILD_TYPE}")
message(STATUS "│ Optimization: -${PIR_OPT_LEVEL}")
message(STATUS "│ Triple:       ${PIR_TRIPLE}")
message(STATUS "├─────────────────────────────────────┤")
message(STATUS "│ Output:       output${PIR_EXT}")
message(STATUS "│ App Dir:      ${APP_DIR}")
message(STATUS "└─────────────────────────────────────┘")
message(STATUS "")
