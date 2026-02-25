#pragma once

#include "primitives.h"
#include "memory.h"

// Unified error — all network/platform layers push codes onto a call-stack array.
// Each layer appends its code after any codes pushed by lower layers.
// Unique enum values across all layers identify which layer set each code.
struct Error
{
	enum Code : UINT32
	{
		// -------------------------
		// Socket errors (1–15)
		// -------------------------
		Socket_CreateFailed_Open = 1,		   // ZwCreateFile / socket() failed
		Socket_BindFailed_EventCreate = 2,	   // ZwCreateEvent failed (Windows only)
		Socket_BindFailed_Bind = 3,			   // AFD_BIND / bind() syscall failed
		Socket_OpenFailed_HandleInvalid = 4,   // socket was never created successfully
		Socket_OpenFailed_EventCreate = 5,	   // ZwCreateEvent failed (Windows only)
		Socket_OpenFailed_Connect = 6,		   // AFD_CONNECT / connect() syscall failed
		Socket_CloseFailed_Close = 7,		   // ZwClose / close() failed
		Socket_ReadFailed_HandleInvalid = 8,   // socket handle invalid
		Socket_ReadFailed_EventCreate = 9,	   // ZwCreateEvent failed (Windows only)
		Socket_ReadFailed_Timeout = 10,		   // receive timed out
		Socket_ReadFailed_Recv = 11,		   // AFD_RECV / recv() syscall failed
		Socket_WriteFailed_HandleInvalid = 12, // socket handle invalid
		Socket_WriteFailed_EventCreate = 13,   // ZwCreateEvent failed (Windows only)
		Socket_WriteFailed_Timeout = 14,	   // send timed out
		Socket_WriteFailed_Send = 15,		   // AFD_SEND / send() syscall failed

		// -------------------------
		// TLS errors (16–22)
		// -------------------------
		Tls_OpenFailed_Socket = 16,	   // underlying socket Open() failed
		Tls_OpenFailed_Handshake = 17, // TLS handshake failed
		Tls_CloseFailed_Socket = 18,   // underlying socket Close() failed
		Tls_ReadFailed_NotReady = 19,  // connection not established
		Tls_ReadFailed_Receive = 20,   // ProcessReceive() failed
		Tls_WriteFailed_NotReady = 21, // connection not established
		Tls_WriteFailed_Send = 22,	   // SendPacket() failed

		// -------------------------
		// WebSocket errors (23–32)
		// -------------------------
		Ws_TransportFailed = 23,  // TLS/socket transport open failed
		Ws_DnsFailed = 24,		  // DNS resolution failed
		Ws_HandshakeFailed = 25,  // HTTP 101 upgrade handshake failed
		Ws_WriteFailed = 26,	  // frame write to transport failed
		Ws_NotConnected = 27,	  // operation attempted on closed connection
		Ws_AllocFailed = 28,	  // memory allocation failed
		Ws_ReceiveFailed = 29,	  // frame receive failed
		Ws_ConnectionClosed = 30, // server sent CLOSE frame
		Ws_InvalidFrame = 31,	  // received frame with invalid RSV bits or opcode
		Ws_FrameTooLarge = 32,	  // received frame exceeds size limit

		// -------------------------
		// Windows NTDLL operations (33–47)
		// Pushed by the NTDLL layer automatically when a syscall fails.
		// The Error arrives pre-packaged from NTDLL::Zw*; callers only push their own layer code on top.
		// -------------------------
		Ntdll_ZwCreateFile = 33,            // ZwCreateFile (socket object or file creation)
		Ntdll_ZwCreateEvent = 34,           // ZwCreateEvent (async I/O event)
		Ntdll_ZwDeviceIoControlFile = 35,   // ZwDeviceIoControlFile (AFD bind/connect/send/recv)
		Ntdll_ZwWaitForSingleObject = 36,   // ZwWaitForSingleObject (async wait → timeout)
		Ntdll_ZwClose = 37,                 // ZwClose (socket or event handle close)

		// -------------------------
		// POSIX syscalls – Linux / macOS (48–63)
		// Pushed by the socket layer immediately before the Socket_* code
		// to identify which syscall was invoked at the point of failure.
		// -------------------------
		Syscall_Socket = 48,   // socket() – socket file descriptor creation
		Syscall_Bind = 49,     // bind()
		Syscall_Connect = 50,  // connect()
		Syscall_Send = 51,     // send() / sendto()
		Syscall_Recv = 52,     // recv() / recvfrom()
		Syscall_Close = 53,    // close()

		// -------------------------
		// UEFI EFI Boot Services / TCP protocol (64–79)
		// Pushed by the socket layer immediately before the Socket_* code
		// to identify which EFI service or TCP operation failed.
		// -------------------------
		Efi_CreateEvent = 64,  // EFI_BOOT_SERVICES::CreateEvent
		Efi_Transmit = 65,     // EFI_TCP4/6_PROTOCOL::Transmit
		Efi_Receive = 66,      // EFI_TCP4/6_PROTOCOL::Receive
		Efi_Configure = 67,    // EFI_TCP4/6_PROTOCOL::Configure
		Efi_Connect = 68,      // EFI_TCP4/6_PROTOCOL::Connect
	};

	UINT32 PlatformCode;	// raw OS error code (NTSTATUS, errno, EFI_STATUS); 0 = none
	UINT32 RuntimeCode[16]; // Error::Code call stack, innermost first; 0 = empty slot

	Error() { Memory::Zero(this, sizeof(Error)); }

	// Set the raw platform error code (NTSTATUS, Linux errno, EFI_STATUS, etc.).
	// Call this before Push() so the platform code sits beneath the runtime stack.
	VOID SetPlatformCode(UINT32 code) { PlatformCode = code; }

	// Push a runtime error code onto the call stack.
	VOID Push(UINT32 code)
	{
		for (UINT32 i = 0; i < 16; i++)
		{
			if (RuntimeCode[i] == 0)
			{
				RuntimeCode[i] = code;
				return;
			}
		}
	}
};
