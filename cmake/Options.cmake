# =============================================================================
# Options.cmake - Build Option Validation and Derived Settings
# =============================================================================

include_guard(GLOBAL)

# =============================================================================
# Dependency Scanning
# =============================================================================
# Required because CMAKE_CXX_COMPILER_FORCED skips compiler detection,
# which includes dependency scanning setup. This ensures header changes
# trigger rebuilds.
set(CMAKE_DEPENDS_USE_COMPILER TRUE)

# =============================================================================
# Build Options
# =============================================================================
set(ARCHITECTURE "x86_64" CACHE STRING "Target: i386, x86_64, armv7a, aarch64")
set(PLATFORM "windows" CACHE STRING "Target: windows, linux, uefi")
set(BUILD_TYPE "release" CACHE STRING "Build type: debug, release")
set(OPTIMIZATION_LEVEL "" CACHE STRING "Override optimization level (e.g., O2, Os)")
option(ENABLE_LOGGING "Enable logging macros" ON)

# Normalize inputs
string(TOLOWER "${ARCHITECTURE}" PIR_ARCH)
string(TOLOWER "${PLATFORM}" PIR_PLATFORM)
string(TOLOWER "${BUILD_TYPE}" PIR_BUILD_TYPE)

# Validate inputs
set(_valid_archs i386 x86_64 armv7a aarch64)
set(_valid_platforms windows linux macos uefi)

if(NOT PIR_ARCH IN_LIST _valid_archs)
    message(FATAL_ERROR "Invalid ARCHITECTURE '${ARCHITECTURE}'. Valid: ${_valid_archs}")
endif()
if(NOT PIR_PLATFORM IN_LIST _valid_platforms)
    message(FATAL_ERROR "Invalid PLATFORM '${PLATFORM}'. Valid: ${_valid_platforms}")
endif()
if(NOT PIR_BUILD_TYPE MATCHES "^(debug|release)$")
    message(FATAL_ERROR "Invalid BUILD_TYPE '${BUILD_TYPE}'. Valid: debug, release")
endif()

# Derived settings
string(TOUPPER "${PIR_ARCH}" PIR_ARCH_UPPER)
string(TOUPPER "${PIR_PLATFORM}" PIR_PLATFORM_UPPER)
set(PIR_OUTPUT_DIR "${CMAKE_SOURCE_DIR}/build/${PIR_BUILD_TYPE}/${PIR_PLATFORM}/${PIR_ARCH}")
set(PIR_MAP_FILE "${PIR_OUTPUT_DIR}/output.map.txt")
set(PIR_IS_DEBUG $<STREQUAL:${PIR_BUILD_TYPE},debug>)

if(OPTIMIZATION_LEVEL)
    set(PIR_OPT_LEVEL "${OPTIMIZATION_LEVEL}")
elseif(PIR_BUILD_TYPE STREQUAL "debug")
    set(PIR_OPT_LEVEL "Og")
else()
    set(PIR_OPT_LEVEL "Oz")
endif()

# =============================================================================
# Preprocessor Defines
# =============================================================================
set(PIR_DEFINES
    ARCHITECTURE_${PIR_ARCH_UPPER}
    PLATFORM_${PIR_PLATFORM_UPPER}
    PLATFORM_${PIR_PLATFORM_UPPER}_${PIR_ARCH_UPPER}
)
if(PIR_BUILD_TYPE STREQUAL "debug")
    list(APPEND PIR_DEFINES DEBUG)
endif()
if(ENABLE_LOGGING)
    list(APPEND PIR_DEFINES ENABLE_LOGGING)
endif()
