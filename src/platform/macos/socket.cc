#include "socket.h"
#include "syscall.h"
#include "system.h"
#include "memory.h"
#include "ip_address.h"

// BSD socket addresses have a sa_len field that must be set
// macOS AF_INET6 = 30 (different from Linux 10 and Windows 23)
// These are handled via platform-conditional defines in socket.h

// Socket constructor
Socket::Socket(const IPAddress& ipAddress, UINT16 port)
	: ip(ipAddress), port(port), m_socket(NULL)
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
	(VOID)shareType;  // Not used on macOS

	if (!IsValid())
		return FALSE;

	SSIZE sockfd = (SSIZE)m_socket;
	UINT32 addrLen = (socketAddress.sin_family == AF_INET6) ? sizeof(SockAddr6) : sizeof(SockAddr);
	SSIZE result = System::Call(SYS_BIND, sockfd, (USIZE)&socketAddress, addrLen);
	return result == 0;
}

// Open/Connect socket
BOOL Socket::Open()
{
	if (!IsValid())
		return FALSE;

	SSIZE sockfd = (SSIZE)m_socket;

	// Use helper to prepare socket address
	union {
		SockAddr addr4;
		SockAddr6 addr6;
	} addrBuffer;

	UINT32 addrLen = SocketAddressHelper::PrepareAddress(ip, port, &addrBuffer, sizeof(addrBuffer));
	if (addrLen == 0)
		return FALSE;

	SSIZE result = System::Call(SYS_CONNECT, sockfd, (USIZE)&addrBuffer, addrLen);
	return result == 0;
}

// Close socket
BOOL Socket::Close()
{
	if (!IsValid())
		return FALSE;

	SSIZE sockfd = (SSIZE)m_socket;
	System::Call(SYS_CLOSE, sockfd);
	m_socket = (PVOID)INVALID_FD;
	return TRUE;
}

// Read from socket
SSIZE Socket::Read(PVOID buffer, UINT32 bufferLength)
{
	if (!IsValid())
		return -1;

	SSIZE sockfd = (SSIZE)m_socket;
	SSIZE result = System::Call(SYS_RECVFROM, sockfd, (USIZE)buffer, bufferLength, 0, 0, 0);
	return result;
}

// Write to socket
UINT32 Socket::Write(PCVOID buffer, UINT32 bufferLength)
{
	if (!IsValid())
		return 0;

	SSIZE sockfd = (SSIZE)m_socket;
	UINT32 totalSent = 0;

	while (totalSent < bufferLength)
	{
		SSIZE sent = System::Call(SYS_SENDTO, sockfd, (USIZE)((const CHAR*)buffer + totalSent),
		                         bufferLength - totalSent, 0, 0, 0);
		if (sent <= 0)
			break;

		totalSent += (UINT32)sent;
	}

	return totalSent;
}
