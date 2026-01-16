#
# launch_qemu.cmake - Launch builds in QEMU
#
# This module is included by CMakeLists.txt when LAUNCH_QEMU=ON
# It creates custom targets:
#   - 'launch'       : Run QEMU normally
#   - 'launch-debug' : Run QEMU with GDB server on port 1234 (paused, waiting for debugger)
#
# Supported platforms: uefi, linux
#

if(NOT (PLATFORM_LC STREQUAL "uefi" OR PLATFORM_LC STREQUAL "linux"))
    message(FATAL_ERROR "LAUNCH_QEMU is only supported for UEFI and Linux platforms")
endif()

# Validate architecture
set(VALID_QEMU_ARCHS "i386" "x86_64" "aarch64" "armv7a")
if(NOT ARCHITECTURE_LC IN_LIST VALID_QEMU_ARCHS)
    message(FATAL_ERROR "LAUNCH_QEMU: Architecture '${ARCHITECTURE_LC}' not supported. Use: ${VALID_QEMU_ARCHS}")
endif()

# Determine QEMU executable and GDB architecture based on architecture
if(ARCHITECTURE_LC STREQUAL "x86_64")
    set(QEMU_SYSTEM "qemu-system-x86_64")
    set(QEMU_USER "qemu-x86_64")
    set(QEMU_MACHINE_ARGS "-machine;q35;-m;512M")
    set(GDB_ARCH "i386:x86-64")
elseif(ARCHITECTURE_LC STREQUAL "i386")
    set(QEMU_SYSTEM "qemu-system-i386")
    set(QEMU_USER "qemu-i386")
    set(QEMU_MACHINE_ARGS "-machine;q35;-m;512M")
    set(GDB_ARCH "i386")
elseif(ARCHITECTURE_LC STREQUAL "aarch64")
    set(QEMU_SYSTEM "qemu-system-aarch64")
    set(QEMU_USER "qemu-aarch64")
    set(QEMU_MACHINE_ARGS "-machine;virt;-cpu;cortex-a57;-m;512M")
    set(GDB_ARCH "aarch64")
elseif(ARCHITECTURE_LC STREQUAL "armv7a")
    set(QEMU_SYSTEM "qemu-system-arm")
    set(QEMU_USER "qemu-arm")
    set(QEMU_MACHINE_ARGS "-machine;virt;-cpu;cortex-a15;-m;512M")
    set(GDB_ARCH "arm")
endif()

# Find QEMU executables
find_program(QEMU_SYSTEM_EXECUTABLE ${QEMU_SYSTEM}
    PATHS
        "C:/Program Files/qemu"
        "C:/qemu"
        "$ENV{ProgramFiles}/qemu"
        "/usr/bin"
        "/usr/local/bin"
    DOC "QEMU system emulator for ${ARCHITECTURE_LC}"
)

find_program(QEMU_USER_EXECUTABLE ${QEMU_USER}
    PATHS
        "C:/Program Files/qemu"
        "C:/qemu"
        "$ENV{ProgramFiles}/qemu"
        "/usr/bin"
        "/usr/local/bin"
    DOC "QEMU user-mode emulator for ${ARCHITECTURE_LC}"
)

if(NOT QEMU_SYSTEM_EXECUTABLE AND NOT QEMU_USER_EXECUTABLE)
    message(WARNING "QEMU not found. The 'launch' target will not work.")
    message(STATUS "Install QEMU:")
    if(WIN32)
        message(STATUS "  choco install qemu")
        message(STATUS "  Or download from: https://qemu.weilnetz.de/")
    else()
        message(STATUS "  apt install qemu-system-x86 qemu-system-arm qemu-user")
    endif()
endif()

#
# Platform-specific configuration
#
if(PLATFORM_LC STREQUAL "uefi")
    # UEFI platform - use system emulation with OVMF firmware

    # Only x86_64, i386, aarch64 supported for UEFI
    if(ARCHITECTURE_LC STREQUAL "armv7a")
        message(FATAL_ERROR "LAUNCH_QEMU: armv7a not supported for UEFI platform")
    endif()

    # Determine boot filename
    if(ARCHITECTURE_LC STREQUAL "x86_64")
        set(QEMU_BOOT_FILE "BOOTX64.EFI")
    elseif(ARCHITECTURE_LC STREQUAL "i386")
        set(QEMU_BOOT_FILE "BOOTIA32.EFI")
    elseif(ARCHITECTURE_LC STREQUAL "aarch64")
        set(QEMU_BOOT_FILE "BOOTAA64.EFI")
    endif()

    # Create ESP directory structure for QEMU FAT boot
    set(QEMU_ESP_DIR "${OUTPUT_DIR}/esp")
    set(QEMU_EFI_BOOT_DIR "${QEMU_ESP_DIR}/EFI/BOOT")

    # Find OVMF firmware
    # Note: WIN32 is not set because CMAKE_SYSTEM_NAME is Generic (cross-compiling)
    # Use CMAKE_HOST_SYSTEM_NAME to detect the host OS
    set(OVMF_SEARCH_PATHS "")
    if(CMAKE_HOST_SYSTEM_NAME STREQUAL "Windows")
        # Use forward slashes for CMake compatibility
        file(TO_CMAKE_PATH "$ENV{ProgramFiles}/qemu/share" QEMU_SHARE_PATH)
        list(APPEND OVMF_SEARCH_PATHS
            "${QEMU_SHARE_PATH}"
            "C:/Program Files/qemu/share"
            "C:/Program Files/qemu"
            "C:/qemu/share"
            "C:/qemu"
        )
    else()
        list(APPEND OVMF_SEARCH_PATHS
            "/usr/share/OVMF"
            "/usr/share/qemu"
            "/usr/share/edk2/ovmf"
            "/usr/share/edk2-ovmf"
        )
    endif()

    # Architecture-specific firmware names
    if(ARCHITECTURE_LC STREQUAL "x86_64")
        set(OVMF_NAMES "edk2-x86_64-code.fd" "OVMF_CODE.fd" "OVMF.fd")
    elseif(ARCHITECTURE_LC STREQUAL "i386")
        set(OVMF_NAMES "edk2-i386-code.fd" "OVMF_CODE_IA32.fd" "OVMF32.fd")
    elseif(ARCHITECTURE_LC STREQUAL "aarch64")
        set(OVMF_NAMES "edk2-aarch64-code.fd" "QEMU_EFI.fd" "AAVMF_CODE.fd")
    endif()

    # Clear cached value to force re-search (architecture may have changed)
    unset(OVMF_FIRMWARE CACHE)

    find_file(OVMF_FIRMWARE
        NAMES ${OVMF_NAMES}
        PATHS ${OVMF_SEARCH_PATHS}
        NO_DEFAULT_PATH
        NO_CMAKE_FIND_ROOT_PATH
        DOC "OVMF/EDK2 UEFI firmware for ${ARCHITECTURE_LC}"
    )

    if(OVMF_FIRMWARE)
        message(STATUS "OVMF firmware: ${OVMF_FIRMWARE}")
        set(QEMU_FIRMWARE_ARGS "-drive;if=pflash,format=raw,readonly=on,file=${OVMF_FIRMWARE}")
    else()
        message(WARNING "OVMF firmware not found for ${ARCHITECTURE_LC}. QEMU will use legacy BIOS (may not work for UEFI apps).")
        set(QEMU_FIRMWARE_ARGS "")
    endif()

    # Post-build: prepare ESP directory
    add_custom_command(TARGET ${TARGET_TRIPLE} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory "${QEMU_EFI_BOOT_DIR}"
        COMMAND ${CMAKE_COMMAND} -E copy "${OUTPUT_EXE}" "${QEMU_EFI_BOOT_DIR}/${QEMU_BOOT_FILE}"
        COMMENT "Preparing UEFI ESP for QEMU: ${QEMU_EFI_BOOT_DIR}/${QEMU_BOOT_FILE}"
    )

    # Create targets if QEMU is found
    if(QEMU_SYSTEM_EXECUTABLE)
        set(QEMU_EXECUTABLE ${QEMU_SYSTEM_EXECUTABLE})

        # Base QEMU arguments
        set(QEMU_BASE_ARGS
            ${QEMU_MACHINE_ARGS}
            ${QEMU_FIRMWARE_ARGS}
            "-drive;file=fat:rw:${QEMU_ESP_DIR},format=raw"
            "-nographic"
            "-serial;stdio"
            "-monitor;none"
        )

        # 'launch' target
        add_custom_target(launch
            COMMAND ${QEMU_EXECUTABLE} ${QEMU_BASE_ARGS}
            DEPENDS ${TARGET_TRIPLE}
            WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
            COMMENT "Launching QEMU with UEFI application..."
            USES_TERMINAL
        )

        # 'launch-debug' target
        set(QEMU_DEBUG_ARGS ${QEMU_BASE_ARGS} "-s" "-S")
        add_custom_target(launch-debug
            COMMAND ${QEMU_EXECUTABLE} ${QEMU_DEBUG_ARGS}
            DEPENDS ${TARGET_TRIPLE}
            WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
            COMMENT "Launching QEMU with GDB server on port 1234 (waiting for debugger)..."
            USES_TERMINAL
        )

        message(STATUS "")
        message(STATUS "=== QEMU UEFI Launch Configuration ===")
        message(STATUS "QEMU:           ${QEMU_EXECUTABLE}")
        message(STATUS "Boot file:      \\EFI\\BOOT\\${QEMU_BOOT_FILE}")
        message(STATUS "ESP directory:  ${QEMU_ESP_DIR}")
        message(STATUS "GDB arch:       ${GDB_ARCH}")
        message(STATUS "======================================")
        message(STATUS "")
    endif()

elseif(PLATFORM_LC STREQUAL "linux")
    # Linux platform - use qemu-user for userspace emulation
    # This allows running Linux ELF binaries directly without a full Linux kernel

    if(QEMU_USER_EXECUTABLE)
        set(QEMU_EXECUTABLE ${QEMU_USER_EXECUTABLE})

        # 'launch' target - run with qemu-user
        add_custom_target(launch
            COMMAND ${QEMU_EXECUTABLE} "${OUTPUT_EXE}"
            DEPENDS ${TARGET_TRIPLE}
            WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
            COMMENT "Launching Linux ELF with QEMU user-mode emulation..."
            USES_TERMINAL
        )

        # 'launch-debug' target - run with qemu-user and GDB server
        # -g 1234 starts GDB server on port 1234
        add_custom_target(launch-debug
            COMMAND ${QEMU_EXECUTABLE} -g 1234 "${OUTPUT_EXE}"
            DEPENDS ${TARGET_TRIPLE}
            WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
            COMMENT "Launching Linux ELF with QEMU GDB server on port 1234..."
            USES_TERMINAL
        )

        message(STATUS "")
        message(STATUS "=== QEMU Linux Launch Configuration ===")
        message(STATUS "QEMU user:      ${QEMU_EXECUTABLE}")
        message(STATUS "Binary:         ${OUTPUT_EXE}")
        message(STATUS "GDB arch:       ${GDB_ARCH}")
        message(STATUS "=======================================")
        message(STATUS "")
    else()
        message(WARNING "QEMU user-mode (${QEMU_USER}) not found. The 'launch' target will not work.")
        message(STATUS "Note: QEMU user-mode is typically only available on Linux hosts.")
        message(STATUS "On Windows, consider using WSL or a Linux VM.")
    endif()
endif()

# Print common debug instructions
if(QEMU_EXECUTABLE)
    message(STATUS "Targets:")
    message(STATUS "  launch       - Run QEMU normally")
    message(STATUS "  launch-debug - Run QEMU with GDB server on :1234")
    message(STATUS "")
    message(STATUS "To debug:")
    message(STATUS "  1. cmake --build <build-dir> --target launch-debug")
    message(STATUS "  2. In another terminal: gdb -ex 'target remote :1234' ${OUTPUT_EXE}")
    message(STATUS "")
endif()
