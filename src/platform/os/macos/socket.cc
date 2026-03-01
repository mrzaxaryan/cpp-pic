#include "socket.h"
#include "syscall.h"
#include "system.h"
#include "memory.h"
#include "ip_address.h"

// BSD socket addresses have a sa_len field that must be set
// macOS AF_INET6 = 30 (different from Linux 10 and Windows 23)
// These are handled via platform-conditional defines in socket.h

Result<Socket, Error> Socket::Create(const IPAddress &ipAddress, UINT16 port)
{
	Socket sock(ipAddress, port);
	SSIZE fd = System::Call(SYS_SOCKET, SocketAddressHelper::GetAddressFamily(sock.ip), SOCK_STREAM, IPPROTO_TCP);
	if (fd < 0)
		return Result<Socket, Error>::Err(Error::Posix((UINT32)(-fd)), Error::Socket_CreateFailed_Open);
	sock.m_socket = (PVOID)fd;
	return Result<Socket, Error>::Ok(static_cast<Socket &&>(sock));
}

Result<void, Error> Socket::Bind(SockAddr &socketAddress, INT32 shareType)
{
	(VOID)shareType; // not used on macOS

	SSIZE  sockfd  = (SSIZE)m_socket;
	UINT32 addrLen = (socketAddress.sin_family == AF_INET6) ? sizeof(SockAddr6) : sizeof(SockAddr);
	SSIZE  result  = System::Call(SYS_BIND, sockfd, (USIZE)&socketAddress, addrLen);
	if (result != 0)
	{
		return Result<void, Error>::Err(
			Error::Posix((UINT32)(-result)),
			Error::Socket_BindFailed_Bind);
	}

	return Result<void, Error>::Ok();
}

Result<void, Error> Socket::Open()
{
	SSIZE sockfd = (SSIZE)m_socket;

	union
	{
		SockAddr  addr4;
		SockAddr6 addr6;
	} addrBuffer;

	UINT32 addrLen = SocketAddressHelper::PrepareAddress(ip, port, Span<UINT8>((UINT8 *)&addrBuffer, sizeof(addrBuffer)));
	if (addrLen == 0)
		return Result<void, Error>::Err(Error::Socket_OpenFailed_Connect);

	// Set socket to non-blocking for connect with timeout
	SSIZE flags = System::Call(SYS_FCNTL, sockfd, (USIZE)F_GETFL);
	if (flags < 0)
		return Result<void, Error>::Err(Error::Posix((UINT32)(-flags)), Error::Socket_OpenFailed_Connect);

	SSIZE setResult = System::Call(SYS_FCNTL, sockfd, (USIZE)F_SETFL, (USIZE)(flags | O_NONBLOCK));
	if (setResult < 0)
		return Result<void, Error>::Err(Error::Posix((UINT32)(-setResult)), Error::Socket_OpenFailed_Connect);

	SSIZE result = System::Call(SYS_CONNECT, sockfd, (USIZE)&addrBuffer, addrLen);
	if (result != 0 && (-(INT32)result) != EINPROGRESS)
	{
		(void)System::Call(SYS_FCNTL, sockfd, (USIZE)F_SETFL, (USIZE)flags);
		return Result<void, Error>::Err(
			Error::Posix((UINT32)(-result)),
			Error::Socket_OpenFailed_Connect);
	}

	if (result != 0)
	{
		// Connect in progress — wait with 5-second timeout
		struct pollfd pfd;
		pfd.fd = (INT32)sockfd;
		pfd.events = POLLOUT;
		pfd.revents = 0;

		// poll timeout is in milliseconds
		SSIZE pollResult = System::Call(SYS_POLL, (USIZE)&pfd, 1, 5000);
		if (pollResult <= 0)
		{
			(void)System::Call(SYS_FCNTL, sockfd, (USIZE)F_SETFL, (USIZE)flags);
			return Result<void, Error>::Err(Error::Socket_OpenFailed_Connect);
		}

		// Check for connection error
		INT32 sockError = 0;
		UINT32 optLen = sizeof(sockError);
		(void)System::Call(SYS_GETSOCKOPT, sockfd, (USIZE)SOL_SOCKET, (USIZE)SO_ERROR, (USIZE)&sockError, (USIZE)&optLen);
		if (sockError != 0)
		{
			(void)System::Call(SYS_FCNTL, sockfd, (USIZE)F_SETFL, (USIZE)flags);
			return Result<void, Error>::Err(
				Error::Posix((UINT32)sockError),
				Error::Socket_OpenFailed_Connect);
		}
	}

	// Restore blocking mode
	(void)System::Call(SYS_FCNTL, sockfd, (USIZE)F_SETFL, (USIZE)flags);
	return Result<void, Error>::Ok();
}

Result<void, Error> Socket::Close()
{
	SSIZE sockfd = (SSIZE)m_socket;
	System::Call(SYS_CLOSE, sockfd);
	m_socket = nullptr;
	return Result<void, Error>::Ok();
}

Result<SSIZE, Error> Socket::Read(Span<CHAR> buffer)
{
	PVOID bufferPtr = (PVOID)buffer.Data();
	UINT32 bufferLength = (UINT32)buffer.Size();

	SSIZE sockfd = (SSIZE)m_socket;
	SSIZE result = System::Call(SYS_RECVFROM, sockfd, (USIZE)bufferPtr, bufferLength, 0, 0, 0);
	if (result < 0)
	{
		return Result<SSIZE, Error>::Err(
			Error::Posix((UINT32)(-result)),
			Error::Socket_ReadFailed_Recv);
	}

	return Result<SSIZE, Error>::Ok(result);
}

Result<UINT32, Error> Socket::Write(Span<const CHAR> buffer)
{
	PCVOID bufferPtr = (PCVOID)buffer.Data();
	UINT32 bufferLength = (UINT32)buffer.Size();

	SSIZE  sockfd    = (SSIZE)m_socket;
	UINT32 totalSent = 0;

	while (totalSent < bufferLength)
	{
		SSIZE sent = System::Call(SYS_SENDTO, sockfd,
		                          (USIZE)((const CHAR *)bufferPtr + totalSent),
		                          bufferLength - totalSent, 0, 0, 0);
		if (sent <= 0)
		{
			if (sent < 0)
				return Result<UINT32, Error>::Err(
					Error::Posix((UINT32)(-sent)),
					Error::Socket_WriteFailed_Send);
			return Result<UINT32, Error>::Err(
				Error::Socket_WriteFailed_Send);
		}

		totalSent += (UINT32)sent;
	}

	return Result<UINT32, Error>::Ok(totalSent);
}
