# =============================================================================
# PostBuild.cmake - Post-Build Artifact Generation Pipeline
# =============================================================================

include_guard(GLOBAL)

# =============================================================================
# Post-Build Commands
# =============================================================================
function(pir_add_postbuild target_name)
    set(_out "${PIR_OUTPUT_DIR}/output")

    # Patch ELF EI_OSABI when required (e.g. Solaris needs ELFOSABI_SOLARIS)
    set(_osabi_cmd)
    if(DEFINED PIR_ELF_OSABI)
        set(_osabi_cmd
            COMMAND ${CMAKE_COMMAND}
                -DELF_FILE="${_out}${PIR_EXT}"
                -DOSABI_VALUE=${PIR_ELF_OSABI}
                -P "${PIR_ROOT_DIR}/cmake/scripts/PatchElfOSABI.cmake"
        )
    endif()

    add_custom_command(TARGET ${target_name} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory "${PIR_OUTPUT_DIR}"
        COMMAND ${CMAKE_COMMAND} -E echo "Build complete: ${_out}${PIR_EXT}"
        ${_osabi_cmd}
        COMMAND ${CMAKE_COMMAND}
            -DINPUT_FILE="${_out}${PIR_EXT}"
            -DOUTPUT_DIR="${PIR_OUTPUT_DIR}"
            -P "${PIR_ROOT_DIR}/cmake/scripts/ExtractBinary.cmake"
        COMMAND ${CMAKE_COMMAND}
            -DPIC_FILE="${_out}.bin"
            -DBASE64_FILE="${_out}.b64.txt"
            -P "${PIR_ROOT_DIR}/cmake/scripts/Base64Encode.cmake"
        COMMAND ${CMAKE_COMMAND}
            -DMAP_FILE="${PIR_MAP_FILE}"
            -P "${PIR_ROOT_DIR}/cmake/scripts/VerifyPICMode.cmake"
        COMMENT "Generating PIC artifacts..."
    )
endfunction()
