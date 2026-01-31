#pragma once

#include "primitives.h"
#include "windows_types.h"
#include "djb2.h"

#define HANDLE_FLAG_INHERIT 0x00000001
#define SW_HIDE 0
#define STARTF_USESHOWWINDOW 0x00000001
#define STARTF_USESTDHANDLES 0x00000100

typedef struct _OVERLAPPED
{
	USIZE Internal;
	USIZE InternalHigh;
	union
	{
		struct
		{
			UINT32 Offset;	   // low-order 32-bits of the file pointer
			UINT32 OffsetHigh; // high-order 32-bits of the file pointer
		} DUMMYSTRUCTNAME;
		PVOID Pointer;
	} DUMMYUNIONNAME;

	PVOID hEvent;
} OVERLAPPED, *LPOVERLAPPED;

typedef struct _SECURITY_ATTRIBUTES
{
	UINT32 nLength;
	PVOID lpSecurityDescriptor;
	BOOL bInheritHandle;
} SECURITY_ATTRIBUTES, *PSECURITY_ATTRIBUTES, *LPSECURITY_ATTRIBUTES;

// Startup information structure
typedef struct _STARTUPINFOA
{
	UINT32 cb; // size of structure in bytes
	PCHAR lpReserved;
	PCHAR lpDesktop; // name of the desktop
	PCHAR lpTitle;
	UINT32 dwX;
	UINT32 dwY;
	UINT32 dwXSize;			// with of the window
	UINT32 dwYSize;			// height of the window
	UINT32 dwXCountChars;	// width of the window in character cells
	UINT32 dwYCountChars;	// height of the window in character cells
	UINT32 dwFillAttribute; // text and background colors
	UINT32 dwFlags;			// controls the window behavior
	UINT16 wShowWindow;		// how the window is to be shown
	UINT16 cbReserved2;
	PUINT8 lpReserved2;
	PVOID hStdInput;
	PVOID hStdOutput;
	PVOID hStdError;
} STARTUPINFOA, *LPSTARTUPINFOA;

typedef struct _PROCESS_INFORMATION
{
	PVOID hProcess;
	PVOID hThread;
	UINT32 dwProcessId;
	UINT32 dwThreadId;
} PROCESS_INFORMATION, *PPROCESS_INFORMATION, *LPPROCESS_INFORMATION;

typedef struct _FILETIME
{
	INT32 dwLowDateTime;
	INT32 dwHighDateTime;
} FILETIME, *PFILETIME, *LPFILETIME;

typedef struct _WIN32_FIND_DATAA
{
	INT32 dwFileAttributes;
	FILETIME ftCreationTime;
	FILETIME ftLastAccessTime;
	FILETIME ftLastWriteTime;
	INT32 nFileSizeHigh;
	INT32 nFileSizeLow;
	INT32 dwReserved0;
	INT32 dwReserved1;
	CHAR cFileName[260];
	CHAR cAlternateFileName[14];
} WIN32_FIND_DATAA, *PWIN32_FIND_DATAA, *LPWIN32_FIND_DATAA;

typedef struct _WIN32_FIND_DATAW
{
	UINT32 dwFileAttributes;
	FILETIME ftCreationTime;
	FILETIME ftLastAccessTime;
	FILETIME ftLastWriteTime;
	UINT32 nFileSizeHigh;
	UINT32 nFileSizeLow;
	UINT32 dwReserved0;
	UINT32 dwReserved1;
	WCHAR cFileName[260];
	WCHAR cAlternateFileName[14];
} WIN32_FIND_DATAW, *PWIN32_FIND_DATAW, *LPWIN32_FIND_DATAW;

class Kernel32
{
private:
	/* data */
public:
	// This function writes a character string to a console screen buffer beginning at the current cursor location.
	// Minimum supported client	Windows 2000 Professional [desktop apps only]
	static BOOL WriteConsoleA(PVOID hConsoleOutput, PCVOID lpBuffer, INT32 nNumberOfCharsToWrite, PUINT32 lpNumberOfCharsWritten, LPOVERLAPPED lpOverlapped);
	// This function writes a character string to a console screen buffer beginning at the current cursor location.
	// Minimum supported client	Windows 2000 Professional [desktop apps only]
	static BOOL WriteConsoleW(PVOID hConsoleOutput, PCVOID lpBuffer, INT32 nNumberOfCharsToWrite, PUINT32 lpNumberOfCharsWritten, LPOVERLAPPED lpOverlapped);
	// This function creates an anonymous pipe, and returns handles to the read and write ends of the pipe.
	// Minimum supported client	Windows 2000 Professional [desktop apps | UWP apps]
	static BOOL CreatePipe(PPVOID hReadPipe, PPVOID hWritePipe, LPSECURITY_ATTRIBUTES lpPipeAttributes, UINT32 nSize);
	// This function sets certain properties of an object handle.
	// Minimum supported client	Windows 2000 Professional [desktop apps only]
	static BOOL SetHandleInformation(PVOID hObject, UINT32 dwMask, UINT32 dwFlags);
	// This function creates a new process and its primary thread. The new process runs in the security context of the calling process.
	// Minimum supported client	Windows XP [desktop apps | UWP apps]
	static BOOL CreateProcessA(PCHAR lpApplicationName, PCHAR lpCommandLine, LPSECURITY_ATTRIBUTES lpProcessAttributes, LPSECURITY_ATTRIBUTES lpThreadAttributes, BOOL bInheritHandles, UINT32 dwCreationFlags, PVOID lpEnvironment, PCHAR lpCurrentDirectory, LPSTARTUPINFOA lpStartupInfo, LPPROCESS_INFORMATION lpProcessInformation);
	// This function writes data to the specified file or input/output (I/O) device.
	// Minimum supported client	Windows XP [desktop apps | UWP apps]
	static BOOL WriteFile(PVOID hFile, PCVOID lpBuffer, INT32 nNumberOfBytesToWrite, PUINT32 lpNumberOfBytesWritten, LPOVERLAPPED lpOverlapped);
	// This function reads data from the specified file or input/output (I/O) device.
	// Minimum supported client	Windows XP [desktop apps | UWP apps]
	static BOOL ReadFile(PVOID hFile, PVOID lpBuffer, INT32 nNumberOfBytesToRead, PUINT32 lpNumberOfBytesRead, LPOVERLAPPED lpOverlapped);
	// This function searches a directory for a file or subdirectory with a name that matches a specific name.
	// Minimum supported client	Windows XP [desktop apps | UWP apps]
	static PVOID FindFirstFileW(PCWCHAR lpFileName, LPWIN32_FIND_DATAW lpFindFileData);
	// This function continues a file search from a previous call to the FindFirstFile or FindFirstFileEx function.
	// Minimum supported client	Windows XP [desktop apps | UWP apps]
	static BOOL FindNextFileW(PVOID hFindFile, LPWIN32_FIND_DATAW lpFindFileData);
	// This function closes a file search handle opened by the FindFirstFile, FindFirstFileEx, or FindFirstFileTransacted function.
	// Minimum supported client	Windows XP [desktop apps | UWP apps]
	static BOOL FindClose(PVOID hFindFile);
};