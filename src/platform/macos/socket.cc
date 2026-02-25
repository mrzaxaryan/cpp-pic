#include "socket.h"
#include "syscall.h"
#include "system.h"
#include "memory.h"
#include "ip_address.h"

// BSD socket addresses have a sa_len field that must be set
// macOS AF_INET6 = 30 (different from Linux 10 and Windows 23)
// These are handled via platform-conditional defines in socket.h

// Socket constructor
Socket::Socket(const IPAddress &ipAddress, UINT16 port)
	: ip(ipAddress), port(port), m_socket(nullptr)
{
	INT32 addressFamily = SocketAddressHelper::GetAddressFamily(ip);
	INT32 socketType = SOCK_STREAM;
	INT32 protocol = IPPROTO_TCP;

	SSIZE fd = System::Call(SYS_SOCKET, addressFamily, socketType, protocol);
	if (fd < 0)
	{
		m_socket = (PVOID)INVALID_FD;
		return;
	}

	m_socket = (PVOID)fd;
}

// Bind socket to address
BOOL Socket::Bind(SockAddr &socketAddress, INT32 shareType)
{
	(VOID) shareType; // Not used on macOS

	if (!IsValid())
		return false;

	SSIZE sockfd = (SSIZE)m_socket;
	UINT32 addrLen = (socketAddress.sin_family == AF_INET6) ? sizeof(SockAddr6) : sizeof(SockAddr);
	SSIZE result = System::Call(SYS_BIND, sockfd, (USIZE)&socketAddress, addrLen);
	return result == 0;
}

// Open/Connect socket
Result<void, SocketError> Socket::Open()
{
	if (!IsValid())
		return Result<void, SocketError>::Err(SOCKET_ERROR_CONNECT_FAILED);

	SSIZE sockfd = (SSIZE)m_socket;

	// Use helper to prepare socket address
	union
	{
		SockAddr addr4;
		SockAddr6 addr6;
	} addrBuffer;

	UINT32 addrLen = SocketAddressHelper::PrepareAddress(ip, port, &addrBuffer, sizeof(addrBuffer));
	if (addrLen == 0)
		return Result<void, SocketError>::Err(SOCKET_ERROR_CONNECT_FAILED);

	SSIZE result = System::Call(SYS_CONNECT, sockfd, (USIZE)&addrBuffer, addrLen);
	if (result != 0)
		return Result<void, SocketError>::Err(SOCKET_ERROR_CONNECT_FAILED);
	return Result<void, SocketError>::Ok();
}

// Close socket
Result<void, SocketError> Socket::Close()
{
	if (!IsValid())
		return Result<void, SocketError>::Err(SOCKET_ERROR_CLOSE_FAILED);

	SSIZE sockfd = (SSIZE)m_socket;
	System::Call(SYS_CLOSE, sockfd);
	m_socket = (PVOID)INVALID_FD;
	return Result<void, SocketError>::Ok();
}

// Read from socket
SSIZE Socket::Read(PVOID buffer, UINT32 bufferLength)
{
	if (!IsValid())
		return -1;

	SSIZE sockfd = (SSIZE)m_socket;
	return System::Call(SYS_RECVFROM, sockfd, (USIZE)buffer, bufferLength, 0, 0, 0);
}

// Write to socket
Result<UINT32, SocketError> Socket::Write(PCVOID buffer, UINT32 bufferLength)
{
	if (!IsValid())
		return Result<UINT32, SocketError>::Err(SOCKET_ERROR_SEND_FAILED);

	SSIZE sockfd = (SSIZE)m_socket;
	UINT32 totalSent = 0;

	while (totalSent < bufferLength)
	{
		SSIZE sent = System::Call(SYS_SENDTO, sockfd, (USIZE)((const CHAR *)buffer + totalSent),
								  bufferLength - totalSent, 0, 0, 0);
		if (sent <= 0)
			return Result<UINT32, SocketError>::Err(SOCKET_ERROR_SEND_FAILED);

		totalSent += (UINT32)sent;
	}

	return Result<UINT32, SocketError>::Ok(totalSent);
}
