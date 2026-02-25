#pragma once

#include "primitives.h"

// Unified error code — identifies a single failure point.
// Result<T, Error> stores up to MaxChainDepth of these in a chain (innermost first).
struct Error
{
	// PIR runtime failure points — one unique value per failure site.
	// OS error codes (NTSTATUS, errno, EFI_STATUS) are stored directly in
	// Error.Code when Platform != Runtime; they are not listed here.
	enum ErrorCodes : UINT32
	{
		None = 0, // no error / empty slot

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
	};

	// Which OS layer this code came from.
	// When Platform != Runtime, Code holds the raw OS error value.
	enum class PlatformKind : UINT8
	{
		Runtime = 0, // PIR runtime layer — Code is an ErrorCodes enumerator
		Windows = 1, // NTSTATUS  — Code holds the raw NTSTATUS value
		Posix   = 2, // errno     — Code holds errno as a positive UINT32
		Uefi    = 3, // EFI_STATUS — Code holds the raw EFI_STATUS value
	};

	// Result uses this to enable chain storage (up to MaxChainDepth codes).
	static constexpr UINT32 MaxChainDepth = 8;

	// Backward-compat alias: Error::ErrorCode(...) still compiles.
	using ErrorCode = Error;

	// Fields — Error IS a single error code.
	ErrorCodes   Code;
	PlatformKind Platform;

	Error(UINT32 code = 0, PlatformKind platform = PlatformKind::Runtime)
		: Code((ErrorCodes)code), Platform(platform)
	{
	}
};
