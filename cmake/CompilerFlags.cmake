# =============================================================================
# CompilerFlags.cmake - Compiler and Linker Flag Assembly
# =============================================================================

include_guard(GLOBAL)

# =============================================================================
# Base Compiler Flags
# =============================================================================
set(PIR_BASE_FLAGS
    -std=c++23
    -Werror -Wall -Wextra
    -Wno-gnu-string-literal-operator-template
    -nostdlib
    -fno-ident
    -fno-exceptions
    -fno-rtti
    -fno-builtin
    -fno-stack-check
    -fno-jump-tables
    -ffunction-sections
    -fdata-sections
)

# Architecture-specific
if(PIR_ARCH MATCHES "^(i386|x86_64)$")
    list(APPEND PIR_BASE_FLAGS -mno-stack-arg-probe -mno-implicit-float)
else()
    list(APPEND PIR_BASE_FLAGS -mno-implicit-float)
endif()

# Build-type-specific
if(PIR_BUILD_TYPE STREQUAL "debug")
    list(APPEND PIR_BASE_FLAGS
        -fno-omit-frame-pointer
        -g3 -ferror-limit=200 -${PIR_OPT_LEVEL}
    )
else()
    list(APPEND PIR_BASE_FLAGS
        -fomit-frame-pointer
        -fno-asynchronous-unwind-tables
        -fno-unwind-tables
        -flto=full
        -fvisibility=hidden
        -fno-threadsafe-statics
        -fmerge-all-constants
        -fno-math-errno
        -${PIR_OPT_LEVEL}
    )
endif()

# =============================================================================
# Base Linker Flags
# =============================================================================
set(PIR_BASE_LINK_FLAGS -nostdlib)

# =============================================================================
# Helper: Append Linker Flags
# =============================================================================
macro(pir_add_link_flags)
    foreach(_flag ${ARGN})
        list(APPEND PIR_BASE_LINK_FLAGS "SHELL:-Wl,${_flag}")
    endforeach()
endmacro()
