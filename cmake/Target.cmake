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
set(_platform_map_android Android)
set(_platform_map_ios iOS)
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

# ── pic-transform integration ────────────────────────────────────────────────
# Use -fpass-plugin= (if plugin available) or compile-to-bitcode + standalone
# tool + bitcode-to-object pipeline.
if(PIC_TRANSFORM_PLUGIN)
    target_compile_options(${PIR_TRIPLE} PRIVATE
        "SHELL:-fpass-plugin=${PIC_TRANSFORM_PLUGIN}")
    pir_log_at("pic-transform" "Using plugin mode")
elseif(PIC_TRANSFORM_EXECUTABLE)
    # Standalone mode: emit LLVM bitcode, transform, then compile to object.
    # Override the compile rule with a 3-step pipeline.
    # Use cmd /C on Windows for && chaining; Unix shells handle it natively.
    if(CMAKE_HOST_WIN32)
        set(CMAKE_CXX_COMPILE_OBJECT
            "cmd /C \"<CMAKE_CXX_COMPILER> <DEFINES> <INCLUDES> <FLAGS> -emit-llvm -c <SOURCE> -o <OBJECT>.bc && ${PIC_TRANSFORM_EXECUTABLE} <OBJECT>.bc -o <OBJECT>.transformed.bc && <CMAKE_CXX_COMPILER> -Wno-unused-command-line-argument <FLAGS> -c <OBJECT>.transformed.bc -o <OBJECT>\""
        )
    else()
        set(CMAKE_CXX_COMPILE_OBJECT
            "<CMAKE_CXX_COMPILER> <DEFINES> <INCLUDES> <FLAGS> -emit-llvm -c <SOURCE> -o <OBJECT>.bc && ${PIC_TRANSFORM_EXECUTABLE} <OBJECT>.bc -o <OBJECT>.transformed.bc && <CMAKE_CXX_COMPILER> -Wno-unused-command-line-argument <FLAGS> -c <OBJECT>.transformed.bc -o <OBJECT>"
        )
    endif()
    pir_log_at("pic-transform" "Using bitcode pipeline")
endif()

# Ensure pic-transform is built before the main target
if(PIC_TRANSFORM_TARGET)
    add_dependencies(${PIR_TRIPLE} ${PIC_TRANSFORM_TARGET})
endif()

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
pir_log_header("Build Configuration")
pir_log_kv("Platform"      "${PIR_PLATFORM}")
pir_log_kv("Architecture"  "${PIR_ARCH}")
pir_log_kv("Build type"    "${PIR_BUILD_TYPE}")
pir_log_kv("Optimization"  "-${PIR_OPT_LEVEL}")
pir_log_kv("Triple"        "${PIR_TRIPLE}")
pir_log_kv("Output"        "output${PIR_EXT}")
pir_log_kv("App dir"       "${APP_DIR}")
pir_log_kv("Logging"       "${ENABLE_LOGGING}")
if(PIC_TRANSFORM_PLUGIN)
    pir_log_kv("pic-transform" "plugin")
elseif(PIC_TRANSFORM_EXECUTABLE)
    pir_log_kv("pic-transform" "standalone")
endif()
pir_log_footer()
