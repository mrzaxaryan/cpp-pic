/**
 * @file kernel32.h
 * @brief Kernel32.dll Win32 API Wrappers
 *
 * @details Provides position-independent wrappers around the Windows Win32 API
 * functions exported by kernel32.dll. These wrappers sit above the NT Native
 * API layer and provide higher-level process and handle management operations.
 *
 * All function addresses are resolved dynamically at runtime via
 * ResolveKernel32ExportAddress() using DJB2 hash-based PEB module lookup,
 * eliminating static import table entries.
 *
 * @note All wrappers return Result<void, Error> for uniform error handling.
 *
 * @see Windows API Index
 *      https://learn.microsoft.com/en-us/windows/win32/apiindex/windows-api-list
 */

#pragma once

#include "core/types/primitives.h"
#include "platform/common/windows/windows_types.h"
#include "core/algorithms/djb2.h"
#include "core/types/error.h"
#include "core/types/result.h"

#define HANDLE_FLAG_INHERIT 0x00000001
#define STARTF_USESTDHANDLES 0x00000100

/**
 * @brief Specifies the window station, desktop, standard handles, and appearance
 * of the main window for a new process at creation time.
 *
 * @details Passed to CreateProcessW to configure the initial environment of the
 * new process. The dwFlags member controls which fields are used. When
 * STARTF_USESTDHANDLES is set, hStdInput, hStdOutput, and hStdError specify
 * the standard handles for the new process.
 *
 * @see STARTUPINFOW structure
 *      https://learn.microsoft.com/en-us/windows/win32/api/processthreadsapi/ns-processthreadsapi-startupinfow
 */
typedef struct _STARTUPINFOW
{
	UINT32 cb;              ///< Size of this structure in bytes
	PWCHAR lpReserved;      ///< Reserved; must be NULL
	PWCHAR lpDesktop;       ///< Name of the desktop, or NULL for the default desktop
	PWCHAR lpTitle;         ///< Title displayed in the title bar for console processes
	UINT32 dwX;             ///< X offset of the upper-left corner of a window (in pixels)
	UINT32 dwY;             ///< Y offset of the upper-left corner of a window (in pixels)
	UINT32 dwXSize;         ///< Width of the window (in pixels)
	UINT32 dwYSize;         ///< Height of the window (in pixels)
	UINT32 dwXCountChars;   ///< Screen buffer width (in character columns) for console processes
	UINT32 dwYCountChars;   ///< Screen buffer height (in character rows) for console processes
	UINT32 dwFillAttribute; ///< Initial text and background colors for a console window
	UINT32 dwFlags;         ///< Bitmask controlling which STARTUPINFOW members are used (STARTF_*)
	UINT16 wShowWindow;     ///< Window show state (SW_*) if STARTF_USESHOWWINDOW is set
	UINT16 cbReserved2;     ///< Reserved; must be zero
	PCHAR lpReserved2;      ///< Reserved; must be NULL
	PVOID hStdInput;        ///< Standard input handle if STARTF_USESTDHANDLES is set
	PVOID hStdOutput;       ///< Standard output handle if STARTF_USESTDHANDLES is set
	PVOID hStdError;        ///< Standard error handle if STARTF_USESTDHANDLES is set
} STARTUPINFOW, *LPSTARTUPINFOW;

/**
 * @brief Contains identification information for a newly created process and its
 * primary thread.
 *
 * @details Populated by CreateProcessW upon successful process creation. Contains
 * handles to the new process and thread, as well as their identifiers. Both handles
 * must be closed with CloseHandle (or ZwClose) when no longer needed.
 *
 * @see PROCESS_INFORMATION structure
 *      https://learn.microsoft.com/en-us/windows/win32/api/processthreadsapi/ns-processthreadsapi-process_information
 */
typedef struct _PROCESS_INFORMATION
{
	PVOID hProcess;      ///< Handle to the newly created process
	PVOID hThread;       ///< Handle to the primary thread of the new process
	UINT32 dwProcessId;  ///< System-wide unique identifier for the new process
	UINT32 dwThreadId;   ///< System-wide unique identifier for the primary thread
} PROCESS_INFORMATION, *PPROCESS_INFORMATION, *LPPROCESS_INFORMATION;

/**
 * @brief Wrappers for Win32 API functions exported by kernel32.dll.
 *
 * @details Provides position-independent access to kernel32.dll exports for
 * process creation and handle management. All function addresses are resolved
 * dynamically via ResolveKernel32ExportAddress() at call time.
 *
 * @see Windows API Index
 *      https://learn.microsoft.com/en-us/windows/win32/apiindex/windows-api-list
 */
class Kernel32
{
public:
	/**
	 * @brief Creates a new process and its primary thread.
	 *
	 * @details Creates a new process that runs the specified executable. The new
	 * process runs in the security context of the calling process. The function
	 * returns handles to the new process and its primary thread in the
	 * PROCESS_INFORMATION structure.
	 *
	 * @param lpApplicationName Path to the executable module, or NULL to use lpCommandLine.
	 * @param lpCommandLine Command line to execute (may be modified by the function).
	 * @param lpProcessAttributes Security attributes for the process handle, or NULL for defaults.
	 * @param lpThreadAttributes Security attributes for the thread handle, or NULL for defaults.
	 * @param bInheritHandles TRUE if inheritable handles in the calling process are inherited.
	 * @param dwCreationFlags Process creation flags (e.g., CREATE_NO_WINDOW, CREATE_SUSPENDED).
	 * @param lpEnvironment Pointer to the environment block, or NULL to inherit the parent's.
	 * @param lpCurrentDirectory Current directory for the new process, or NULL for the caller's.
	 * @param lpStartupInfo Pointer to STARTUPINFOW specifying window and handle configuration.
	 * @param lpProcessInformation Receives identification information about the new process.
	 *
	 * @return Result<void, Error> Ok() on success, Err(Kernel32_CreateProcessFailed) on failure.
	 *
	 * @par Requirements
	 * Minimum supported client: Windows XP [desktop apps | UWP apps]
	 * Minimum supported server: Windows Server 2003
	 *
	 * @see Microsoft Learn -- CreateProcessW function
	 *      https://learn.microsoft.com/en-us/windows/win32/api/processthreadsapi/nf-processthreadsapi-createprocessw
	 */
	[[nodiscard]] static Result<void, Error> CreateProcessW(PWCHAR lpApplicationName, PWCHAR lpCommandLine, PVOID lpProcessAttributes, PVOID lpThreadAttributes, BOOL bInheritHandles, UINT32 dwCreationFlags, PVOID lpEnvironment, PWCHAR lpCurrentDirectory, LPSTARTUPINFOW lpStartupInfo, LPPROCESS_INFORMATION lpProcessInformation);

	/**
	 * @brief Sets certain properties of an object handle.
	 *
	 * @details Modifies the properties of an object handle. Used to set or clear
	 * the HANDLE_FLAG_INHERIT flag, which controls whether the handle is inherited
	 * by child processes created with CreateProcessW when bInheritHandles is TRUE.
	 *
	 * @param hObject Handle whose properties are to be set.
	 * @param dwMask Bitmask specifying which flags to change (e.g., HANDLE_FLAG_INHERIT).
	 * @param dwFlags Bitmask specifying the new values for the flags identified by dwMask.
	 *
	 * @return Result<void, Error> Ok() on success, Err(Kernel32_SetHandleInfoFailed) on failure.
	 *
	 * @par Requirements
	 * Minimum supported client: Windows 2000 Professional [desktop apps only]
	 * Minimum supported server: Windows 2000 Server
	 *
	 * @see Microsoft Learn -- SetHandleInformation function
	 *      https://learn.microsoft.com/en-us/windows/win32/api/handleapi/nf-handleapi-sethandleinformation
	 */
	[[nodiscard]] static Result<void, Error> SetHandleInformation(PVOID hObject, UINT32 dwMask, UINT32 dwFlags);
};
