# =============================================================================
# Logging.cmake - Unified Build System Logging
# =============================================================================
# Provides consistent, prefixed logging across the entire build system.
# All messages use the [pir] prefix (or [pir:<component>] for subsystems).

include_guard(GLOBAL)

# ─── Status messages ────────────────────────────────────────────────────────

function(pir_log msg)
    message(STATUS "[pir] ${msg}")
endfunction()

function(pir_log_at component msg)
    message(STATUS "[pir:${component}] ${msg}")
endfunction()

# ─── Build summary helpers ──────────────────────────────────────────────────

function(pir_log_header title)
    message(STATUS "")
    message(STATUS "[pir] ──────────────────────────────────────────")
    message(STATUS "[pir]  ${title}")
    message(STATUS "[pir] ──────────────────────────────────────────")
endfunction()

function(pir_log_kv key value)
    if("${value}" STREQUAL "")
        set(value "(none)")
    endif()
    set(_col 18)
    string(LENGTH "${key}" _klen)
    math(EXPR _pad "${_col} - ${_klen}")
    if(_pad LESS 2)
        set(_pad 2)
    endif()
    string(REPEAT "." ${_pad} _dots)
    message(STATUS "[pir]   ${key} ${_dots} ${value}")
endfunction()

function(pir_log_footer)
    message(STATUS "[pir] ──────────────────────────────────────────")
    message(STATUS "")
endfunction()
