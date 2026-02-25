# =============================================================================
# Triples.cmake - Target Triple and Extension Mapping
# =============================================================================

include_guard(GLOBAL)

# =============================================================================
# Target Triples
# =============================================================================
set(_triple_windows_i386    "i386-pc-windows-gnu")
set(_triple_windows_x86_64  "x86_64-pc-windows-gnu")
set(_triple_windows_armv7a  "armv7a-pc-windows-gnu")
set(_triple_windows_aarch64 "aarch64-pc-windows-gnu")
set(_triple_linux_i386      "i386-unknown-linux-gnu")
set(_triple_linux_x86_64    "x86_64-unknown-linux-gnu")
set(_triple_linux_armv7a    "armv7a-unknown-linux-gnueabihf")
set(_triple_linux_aarch64   "aarch64-unknown-linux-gnu")
set(_triple_macos_x86_64    "x86_64-apple-macos11")
set(_triple_macos_aarch64   "arm64-apple-macos11")
set(_triple_uefi_x86_64     "x86_64-pc-windows-gnu")
set(_triple_uefi_aarch64    "aarch64-pc-windows-gnu")

# Platform extensions
set(_ext_windows ".exe")
set(_ext_linux ".elf")
set(_ext_macos "")
set(_ext_uefi ".efi")

# =============================================================================
# Helper: Get Triple and Extension
# =============================================================================
macro(pir_get_target_info)
    set(PIR_TRIPLE "${_triple_${PIR_PLATFORM}_${PIR_ARCH}}")
    set(PIR_EXT "${_ext_${PIR_PLATFORM}}")
    if(NOT PIR_TRIPLE)
        message(FATAL_ERROR "${PIR_PLATFORM}/${PIR_ARCH} is not a valid combination")
    endif()
endmacro()
