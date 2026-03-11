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
    -nostdlib
    -fno-ident
    -fno-exceptions
    -fno-rtti
    -fno-builtin
    -fno-stack-check
    -fno-jump-tables
    -ffunction-sections
)

# Architecture-specific
# -mno-stack-arg-probe: x86/ARM specific, silently ignored on most targets
# -mno-implicit-float: not supported on MIPS targets (Clang errors with -Werror)
list(APPEND PIR_BASE_FLAGS -mno-stack-arg-probe)
if(NOT PIR_ARCH STREQUAL "mips64")
    list(APPEND PIR_BASE_FLAGS -mno-implicit-float)
endif()

# MIPS64: disable PIC/GOT-based addressing (MIPS defaults to -mabicalls which
# generates GOT-relative code via $gp). -mno-abicalls + -fno-pic produces
# direct addressing without GOT/PLT sections.
if(PIR_ARCH STREQUAL "mips64")
    list(APPEND PIR_BASE_FLAGS -mno-abicalls -fno-pic)
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
        -fno-math-errno
        -${PIR_OPT_LEVEL}
    )
endif()

# =============================================================================
# Base Linker Flags
# =============================================================================
# -fno-jump-tables is repeated here (also in compile flags) to ensure the LTO
# code generator respects it. With -flto=full the actual machine code generation
# happens at link time; passing the flag here guarantees it reaches the backend.
set(PIR_BASE_LINK_FLAGS -nostdlib -fno-jump-tables)

# MIPS64: repeat -mno-abicalls -fno-pic for the linker driver so the LTO
# backend (which generates machine code at link time) does not emit GOT entries.
# -no-pie overrides the Clang driver's default -pie for Linux targets; without
# it the LTO backend attempts PIC code generation which fatally conflicts with
# -mno-abicalls ("position-independent code requires '-mabicalls'").
if(PIR_ARCH STREQUAL "mips64")
    list(APPEND PIR_BASE_LINK_FLAGS -mno-abicalls -fno-pic -no-pie)
endif()

# =============================================================================
# Helper: Append Linker Flags
# =============================================================================
macro(pir_add_link_flags)
    foreach(_flag ${ARGN})
        list(APPEND PIR_BASE_LINK_FLAGS "SHELL:-Wl,${_flag}")
    endforeach()
endmacro()
