#pragma once

#include "core.h"

// Unified network error — all layers push codes onto a call-stack array.
// Each layer appends its code after any codes pushed by lower layers.
// Unique enum values across all layers identify which layer set each code.
struct NetworkError
{
	enum Code : UINT32
	{
		// -------------------------
		// Socket errors (1–15)
		// -------------------------
		Socket_CreateFailed_Open         = 1,  // ZwCreateFile / socket() failed
		Socket_BindFailed_EventCreate    = 2,  // ZwCreateEvent failed (Windows only)
		Socket_BindFailed_Bind           = 3,  // AFD_BIND / bind() syscall failed
		Socket_OpenFailed_HandleInvalid  = 4,  // socket was never created successfully
		Socket_OpenFailed_EventCreate    = 5,  // ZwCreateEvent failed (Windows only)
		Socket_OpenFailed_Connect        = 6,  // AFD_CONNECT / connect() syscall failed
		Socket_CloseFailed_Close         = 7,  // ZwClose / close() failed
		Socket_ReadFailed_HandleInvalid  = 8,  // socket handle invalid
		Socket_ReadFailed_EventCreate    = 9,  // ZwCreateEvent failed (Windows only)
		Socket_ReadFailed_Timeout        = 10, // receive timed out
		Socket_ReadFailed_Recv           = 11, // AFD_RECV / recv() syscall failed
		Socket_WriteFailed_HandleInvalid = 12, // socket handle invalid
		Socket_WriteFailed_EventCreate   = 13, // ZwCreateEvent failed (Windows only)
		Socket_WriteFailed_Timeout       = 14, // send timed out
		Socket_WriteFailed_Send          = 15, // AFD_SEND / send() syscall failed

		// -------------------------
		// TLS errors (16–22)
		// -------------------------
		Tls_OpenFailed_Socket    = 16, // underlying socket Open() failed
		Tls_OpenFailed_Handshake = 17, // TLS handshake failed
		Tls_CloseFailed_Socket   = 18, // underlying socket Close() failed
		Tls_ReadFailed_NotReady  = 19, // connection not established
		Tls_ReadFailed_Receive   = 20, // ProcessReceive() failed
		Tls_WriteFailed_NotReady = 21, // connection not established
		Tls_WriteFailed_Send     = 22, // SendPacket() failed

		// -------------------------
		// WebSocket errors (23–32)
		// -------------------------
		Ws_TransportFailed  = 23, // TLS/socket transport open failed
		Ws_DnsFailed        = 24, // DNS resolution failed
		Ws_HandshakeFailed  = 25, // HTTP 101 upgrade handshake failed
		Ws_WriteFailed      = 26, // frame write to transport failed
		Ws_NotConnected     = 27, // operation attempted on closed connection
		Ws_AllocFailed      = 28, // memory allocation failed
		Ws_ReceiveFailed    = 29, // frame receive failed
		Ws_ConnectionClosed = 30, // server sent CLOSE frame
		Ws_InvalidFrame     = 31, // received frame with invalid RSV bits or opcode
		Ws_FrameTooLarge    = 32, // received frame exceeds size limit
	};

	UINT32 ErrorCode[16];

	NetworkError() { Memory::Zero(ErrorCode, sizeof(ErrorCode)); }

	VOID Push(UINT32 code)
	{
		for (UINT32 i = 0; i < 16; i++)
		{
			if (ErrorCode[i] == 0)
			{
				ErrorCode[i] = code;
				return;
			}
		}
	}
};

/* Socket address families */
#define AF_INET 2
#if defined(PLATFORM_WINDOWS) || defined(PLATFORM_UEFI)
#define AF_INET6 23
#elif defined(PLATFORM_MACOS)
#define AF_INET6 30
#else
#define AF_INET6 10
#endif

/* Socket types */
#define SOCK_STREAM 1
#define SOCK_DGRAM 2

/* Shutdown modes */
#define SHUT_RD 0
#define SHUT_WR 1
#define SHUT_RDWR 2

struct SockAddr
{
	INT16 sin_family;
	UINT16 sin_port;
	UINT32 sin_addr;
	CHAR sin_zero[8];
};

struct SockAddr6
{
	UINT16 sin6_family;
	UINT16 sin6_port;
	UINT32 sin6_flowinfo;
	UINT8 sin6_addr[16];
	UINT32 sin6_scope_id;
};

// Helper class for preparing socket addresses from IPAddress
class SocketAddressHelper
{
public:
	// Prepare a socket address for connect/bind operations
	// Returns the size of the prepared address structure
	static UINT32 PrepareAddress(const IPAddress &ip, UINT16 port, PVOID addrBuffer, UINT32 bufferSize)
	{
		if (ip.IsIPv6())
		{
			if (bufferSize < sizeof(SockAddr6))
				return 0;

			SockAddr6 *addr6 = (SockAddr6 *)addrBuffer;
			Memory::Zero(addr6, sizeof(SockAddr6));
			addr6->sin6_family = AF_INET6;
			addr6->sin6_port = UINT16SwapByteOrder(port);
			addr6->sin6_flowinfo = 0;
			addr6->sin6_scope_id = 0;

			const UINT8 *ipv6Addr = ip.ToIPv6();
			if (ipv6Addr != nullptr)
			{
				Memory::Copy(addr6->sin6_addr, ipv6Addr, 16);
			}

			return sizeof(SockAddr6);
		}
		else
		{
			if (bufferSize < sizeof(SockAddr))
				return 0;

			SockAddr *addr = (SockAddr *)addrBuffer;
			Memory::Zero(addr, sizeof(SockAddr));
			addr->sin_family = AF_INET;
			addr->sin_port = UINT16SwapByteOrder(port);
			addr->sin_addr = ip.ToIPv4();

			return sizeof(SockAddr);
		}
	}

	// Prepare a bind address (zeroed IP, just family and port)
	static UINT32 PrepareBindAddress(BOOL isIPv6, UINT16 port, PVOID addrBuffer, UINT32 bufferSize)
	{
		if (isIPv6)
		{
			if (bufferSize < sizeof(SockAddr6))
				return 0;

			SockAddr6 *addr6 = (SockAddr6 *)addrBuffer;
			Memory::Zero(addr6, sizeof(SockAddr6));
			addr6->sin6_family = AF_INET6;
			addr6->sin6_port = UINT16SwapByteOrder(port);

			return sizeof(SockAddr6);
		}
		else
		{
			if (bufferSize < sizeof(SockAddr))
				return 0;

			SockAddr *addr = (SockAddr *)addrBuffer;
			Memory::Zero(addr, sizeof(SockAddr));
			addr->sin_family = AF_INET;
			addr->sin_port = UINT16SwapByteOrder(port);

			return sizeof(SockAddr);
		}
	}

	// Get the address family for an IP address
	static INT32 GetAddressFamily(const IPAddress &ip)
	{
		return ip.IsIPv6() ? AF_INET6 : AF_INET;
	}
};

class Socket
{
private:
	IPAddress ip;
	UINT16 port;
	PVOID m_socket;
	[[nodiscard]] Result<void, NetworkError> Bind(SockAddr &SocketAddress, INT32 ShareType);

public:
	VOID *operator new(USIZE) = delete;
	VOID operator delete(VOID *) = delete;
	Socket() : ip(), port(0), m_socket(nullptr) {}
	Socket(const IPAddress &ipAddress, UINT16 port);
	~Socket()
	{
		if (IsValid())
			(void)Close();
	}

	Socket(const Socket &) = delete;
	Socket &operator=(const Socket &) = delete;

	Socket(Socket &&other) : ip(other.ip), port(other.port), m_socket(other.m_socket)
	{
		other.m_socket = nullptr;
	}
	Socket &operator=(Socket &&other)
	{
		if (this != &other)
		{
			if (IsValid())
				(void)Close();
			ip = other.ip;
			port = other.port;
			m_socket = other.m_socket;
			other.m_socket = nullptr;
		}
		return *this;
	}

	BOOL IsValid() const { return m_socket != nullptr && m_socket != (PVOID)(SSIZE)(-1); }
	SSIZE GetFd() const { return (SSIZE)m_socket; }
	[[nodiscard]] Result<void, NetworkError> Open();
	[[nodiscard]] Result<void, NetworkError> Close();
	[[nodiscard]] Result<SSIZE, NetworkError> Read(PVOID buffer, UINT32 bufferLength);
	[[nodiscard]] Result<UINT32, NetworkError> Write(PCVOID buffer, UINT32 bufferLength);
};