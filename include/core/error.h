/**
 * @file error.h
 * @brief Unified Error Type for Result-Based Error Handling
 *
 * @details Provides a compact error representation used by the `Result<T, Error>`
 * type throughout the codebase. Each Error is a (Code, Platform) pair that
 * identifies either a PIR runtime failure point or a raw OS error code.
 *
 * Design principles:
 * - Zero-cost: Error is 8 bytes, stored directly in Result (no heap allocation)
 * - Single slot: no error chain — each layer picks the most useful code to surface
 * - Platform-aware: factory methods tag errors with their OS origin for formatting
 *
 * @see result.h — Result<T, Error> tagged union that stores Error on failure
 *
 * @ingroup core
 *
 * @defgroup error Error Type
 * @ingroup core
 * @{
 */

#pragma once

#include "primitives.h"

/**
 * @struct Error
 * @brief Unified error code identifying a single failure point
 *
 * @details Stores a (Code, Platform) pair. When Platform is Runtime, Code is
 * an ErrorCodes enumerator identifying the PIR failure site. When Platform is
 * Windows/Posix/Uefi, Code holds the raw OS error value (NTSTATUS, errno, or
 * EFI_STATUS).
 *
 * Result<T, Error> stores a single Error directly — zero-cost, no chain overhead.
 *
 * @par Example Usage:
 * @code
 * // Runtime error:
 * return Result<void, Error>::Err(Error::Socket_CreateFailed_Open);
 *
 * // OS error (Windows NTSTATUS):
 * return Result<void, Error>::Err(Error::Windows(status));
 *
 * // OS error (POSIX errno):
 * return Result<void, Error>::Err(Error::Posix((UINT32)(-result)));
 * @endcode
 */
struct Error
{
	/**
	 * @enum ErrorCodes
	 * @brief PIR runtime failure points — one unique value per failure site
	 *
	 * @details OS error codes (NTSTATUS, errno, EFI_STATUS) are stored directly
	 * in Error.Code when Platform != Runtime; they are not listed here.
	 */
	enum ErrorCodes : UINT32
	{
		None = 0, // no error / empty slot

		// -------------------------
		// Socket errors (1–3, 5–7, 9–11, 13–15, 39)
		// -------------------------
		Socket_CreateFailed_Open = 1,		   // ZwCreateFile / socket() failed
		Socket_BindFailed_EventCreate = 2,	   // ZwCreateEvent failed (Windows only)
		Socket_BindFailed_Bind = 3,			   // AFD_BIND / bind() syscall failed
		Socket_OpenFailed_EventCreate = 5,	   // ZwCreateEvent failed (Windows only)
		Socket_OpenFailed_Connect = 6,		   // AFD_CONNECT / connect() syscall failed
		Socket_CloseFailed_Close = 7,		   // ZwClose / close() failed
		Socket_ReadFailed_EventCreate = 9,	   // ZwCreateEvent failed (Windows only)
		Socket_ReadFailed_Timeout = 10,		   // receive timed out
		Socket_ReadFailed_Recv = 11,		   // AFD_RECV / recv() syscall failed
		Socket_WriteFailed_EventCreate = 13,   // ZwCreateEvent failed (Windows only)
		Socket_WriteFailed_Timeout = 14,	   // send timed out
		Socket_WriteFailed_Send = 15,		   // AFD_SEND / send() syscall failed
		Socket_WaitFailed = 39,				   // ZwWaitForSingleObject failed (Windows only)

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
		// DNS errors (33–38)
		// -------------------------
		Dns_ConnectFailed  = 33, // TLS connection to DNS server failed
		Dns_QueryFailed    = 34, // DNS query generation failed
		Dns_SendFailed     = 35, // failed to send DNS query
		Dns_ResponseFailed = 36, // DNS server returned non-200 or bad content-length
		Dns_ParseFailed    = 37, // failed to parse DNS binary response
		Dns_ResolveFailed  = 38, // all DNS servers/fallbacks exhausted

		// -------------------------
		// HTTP errors (40–48)
		// -------------------------
		Http_OpenFailed             = 40, // TLS connection open failed
		Http_CloseFailed            = 41, // TLS connection close failed
		Http_ReadFailed             = 42, // TLS read failed
		Http_WriteFailed            = 43, // TLS write failed
		Http_SendGetFailed          = 44, // GET request write failed
		Http_SendPostFailed         = 45, // POST request write failed
		Http_ReadHeadersFailed_Read   = 46, // header read failed
		Http_ReadHeadersFailed_Status = 47, // unexpected HTTP status code
		Http_ParseUrlFailed         = 48, // URL format invalid

		// -------------------------
		// FileSystem errors (50–56)
		// -------------------------
		Fs_OpenFailed        = 50, // file open syscall failed
		Fs_DeleteFailed      = 51, // file delete syscall failed
		Fs_ReadFailed        = 52, // file read syscall failed
		Fs_WriteFailed       = 53, // file write syscall failed
		Fs_CreateDirFailed   = 54, // directory create syscall failed
		Fs_DeleteDirFailed   = 55, // directory delete syscall failed
		Fs_PathResolveFailed = 56, // path name resolution failed

		// -------------------------
		// Crypto errors (60–63)
		// -------------------------
		Ecc_InitFailed         = 60, // curve not recognized or random gen failed
		Ecc_ExportKeyFailed    = 61, // null buffer or insufficient size
		Ecc_SharedSecretFailed = 62, // invalid key format or point at infinity
		ChaCha20_DecodeFailed      = 63, // Poly1305 authentication failed
		ChaCha20_GenerateKeyFailed = 64, // invalid nonce size in Poly1305 key generation

		// -------------------------
		// TlsCipher errors (70–73)
		// -------------------------
		TlsCipher_ComputePublicKeyFailed = 70, // ECC key generation failed
		TlsCipher_ComputePreKeyFailed    = 71, // premaster key computation failed
		TlsCipher_ComputeKeyFailed       = 72, // key derivation failed
		TlsCipher_DecodeFailed           = 73, // record decryption failed

		// -------------------------
		// TLS internal errors (74–84)
		// -------------------------
		Tls_SendPacketFailed       = 74, // packet send to socket failed
		Tls_ClientHelloFailed      = 75, // ClientHello send failed
		Tls_ServerHelloFailed      = 76, // ServerHello processing failed
		Tls_ServerHelloDoneFailed  = 77, // ServerHelloDone processing failed
		Tls_ServerFinishedFailed   = 78, // ServerFinished processing failed
		Tls_VerifyFinishedFailed   = 79, // Finished verification failed
		Tls_ClientExchangeFailed   = 80, // ClientKeyExchange send failed
		Tls_ClientFinishedFailed   = 81, // ClientFinished send failed
		Tls_ChangeCipherSpecFailed = 82, // ChangeCipherSpec send failed
		Tls_ProcessReceiveFailed   = 83, // receive processing failed
		Tls_OnPacketFailed         = 84, // packet handling failed
		Tls_ReadFailed_Channel     = 85, // ReadChannel returned 0 bytes

		// -------------------------
		// Process errors (90–94)
		// -------------------------
		Process_ForkFailed      = 90, // fork() syscall failed
		Process_Dup2Failed      = 91, // dup2() syscall failed
		Process_ExecveFailed    = 92, // execve() syscall failed
		Process_SetsidFailed    = 93, // setsid() syscall failed
		Process_BindShellFailed = 94, // shell binding failed

		// -------------------------
		// Misc errors (95–101)
		// -------------------------
		Base64_DecodeFailed           = 95,  // Base64 decoding failed
		String_ParseIntFailed         = 96,  // integer parsing failed
		String_ParseFloatFailed       = 97,  // float parsing failed
		IpAddress_ToStringFailed      = 98,  // buffer too small for IP string
		IpAddress_ParseFailed         = 105, // IP address string parsing failed
		Kernel32_CreateProcessFailed  = 99,  // CreateProcessW failed
		Kernel32_SetHandleInfoFailed  = 100, // SetHandleInformation failed
		Ntdll_RtlPathResolveFailed    = 101, // RtlDosPathNameToNtPathName_U failed

		// -------------------------
		// Factory creation errors (102–104)
		// -------------------------
		Tls_CreateFailed  = 102, // Socket::Create() failed in TlsClient::Create()
		Http_CreateFailed = 103, // URL parse / DNS / TLS create failed in HttpClient::Create()
		Ws_CreateFailed   = 104, // URL parse / DNS / TLS create failed in WebSocketClient::Create()
	};

	/**
	 * @enum PlatformKind
	 * @brief Identifies which OS layer produced the error code
	 *
	 * @details When Platform != Runtime, Code holds the raw OS error value
	 * rather than an ErrorCodes enumerator. The platform tag drives
	 * formatting in %e (hex for Windows/UEFI, decimal for Posix).
	 */
	enum class PlatformKind : UINT8
	{
		Runtime = 0, ///< PIR runtime layer — Code is an ErrorCodes enumerator
		Windows = 1, ///< NTSTATUS — Code holds the raw NTSTATUS value
		Posix   = 2, ///< errno — Code holds errno as a positive UINT32
		Uefi    = 3, ///< EFI_STATUS — Code holds the raw EFI_STATUS value
	};

	ErrorCodes   Code;     ///< Error code value (ErrorCodes enumerator or raw OS code)
	PlatformKind Platform; ///< OS layer that produced this code

	/**
	 * @brief Construct an error with explicit code and platform
	 * @param code Error code value
	 * @param platform Platform origin (defaults to Runtime)
	 */
	constexpr Error(UINT32 code = 0, PlatformKind platform = PlatformKind::Runtime)
		: Code((ErrorCodes)code), Platform(platform)
	{
	}

	/// @name Platform Factory Methods
	/// @{

	/**
	 * @brief Create a Windows NTSTATUS error
	 * @param ntstatus Raw NTSTATUS value
	 * @return Error tagged with PlatformKind::Windows
	 *
	 * @see https://learn.microsoft.com/en-us/openspecs/windows_protocols/ms-erref/596a1078-e883-4972-9bbc-25e5c5b64e7c
	 */
	[[nodiscard]] static constexpr Error Windows(UINT32 ntstatus) { return Error(ntstatus, PlatformKind::Windows); }

	/**
	 * @brief Create a POSIX errno error
	 * @param errnoVal errno value (stored as positive UINT32)
	 * @return Error tagged with PlatformKind::Posix
	 *
	 * @see https://pubs.opengroup.org/onlinepubs/9699919799/functions/errno.html
	 */
	[[nodiscard]] static constexpr Error Posix(UINT32 errnoVal)   { return Error(errnoVal, PlatformKind::Posix); }

	/**
	 * @brief Create a UEFI EFI_STATUS error
	 * @param efiStatus Raw EFI_STATUS value
	 * @return Error tagged with PlatformKind::Uefi
	 *
	 * @see UEFI Specification — Appendix D (Status Codes)
	 *      https://uefi.org/specs/UEFI/2.10/Apx_D_Status_Codes.html
	 */
	[[nodiscard]] static constexpr Error Uefi(UINT32 efiStatus)   { return Error(efiStatus, PlatformKind::Uefi); }

	/// @}
};

/** @} */ // end of error group
