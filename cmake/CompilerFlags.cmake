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
list(APPEND PIR_BASE_FLAGS -mno-stack-arg-probe -mno-implicit-float)

# RISC-V: disable linker relaxation to avoid GP-relative addressing.
# PIR has no CRT to initialise the gp register, so GP-relative accesses
# emitted by the relaxation pass would fault at runtime.
if(PIR_ARCH MATCHES "^riscv(32|64)$")
    list(APPEND PIR_BASE_FLAGS -mno-relax)
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
# -fno-jump-tables is repeated here (also in compile flags) to ensure the LTO
# code generator respects it. With -flto=full the actual machine code generation
# happens at link time; passing the flag here guarantees it reaches the backend.
set(PIR_BASE_LINK_FLAGS -nostdlib -fno-jump-tables)

# =============================================================================
# Helper: Append Linker Flags
# =============================================================================
macro(pir_add_link_flags)
    foreach(_flag ${ARGN})
        list(APPEND PIR_BASE_LINK_FLAGS "SHELL:-Wl,${_flag}")
    endforeach()
endmacro()
