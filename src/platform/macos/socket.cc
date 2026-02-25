#include "socket.h"
#include "syscall.h"
#include "system.h"
#include "memory.h"
#include "ip_address.h"

// BSD socket addresses have a sa_len field that must be set
// macOS AF_INET6 = 30 (different from Linux 10 and Windows 23)
// These are handled via platform-conditional defines in socket.h

Socket::Socket(const IPAddress &ipAddress, UINT16 port)
	: ip(ipAddress), port(port), m_socket(nullptr)
{
	SSIZE fd = System::Call(SYS_SOCKET, SocketAddressHelper::GetAddressFamily(ip), SOCK_STREAM, IPPROTO_TCP);
	if (fd < 0)
	{
		m_socket = (PVOID)INVALID_FD;
		return;
	}
	m_socket = (PVOID)fd;
}

Result<void, NetworkError> Socket::Bind(SockAddr &socketAddress, INT32 shareType)
{
	(VOID)shareType; // not used on macOS

	if (!IsValid())
	{
		NetworkError err;
		err.Push(NetworkError::Socket_BindFailed_Bind);
		return Result<void, NetworkError>::Err(err);
	}

	SSIZE  sockfd  = (SSIZE)m_socket;
	UINT32 addrLen = (socketAddress.sin_family == AF_INET6) ? sizeof(SockAddr6) : sizeof(SockAddr);
	SSIZE  result  = System::Call(SYS_BIND, sockfd, (USIZE)&socketAddress, addrLen);
	if (result != 0)
	{
		NetworkError err;
		err.Push((UINT32)(-result));
		err.Push(NetworkError::Socket_BindFailed_Bind);
		return Result<void, NetworkError>::Err(err);
	}

	return Result<void, NetworkError>::Ok();
}

Result<void, NetworkError> Socket::Open()
{
	if (!IsValid())
	{
		NetworkError err;
		err.Push(NetworkError::Socket_OpenFailed_HandleInvalid);
		return Result<void, NetworkError>::Err(err);
	}

	SSIZE sockfd = (SSIZE)m_socket;

	union
	{
		SockAddr  addr4;
		SockAddr6 addr6;
	} addrBuffer;

	UINT32 addrLen = SocketAddressHelper::PrepareAddress(ip, port, &addrBuffer, sizeof(addrBuffer));
	if (addrLen == 0)
	{
		NetworkError err;
		err.Push(NetworkError::Socket_OpenFailed_Connect);
		return Result<void, NetworkError>::Err(err);
	}

	SSIZE result = System::Call(SYS_CONNECT, sockfd, (USIZE)&addrBuffer, addrLen);
	if (result != 0)
	{
		NetworkError err;
		err.Push((UINT32)(-result));
		err.Push(NetworkError::Socket_OpenFailed_Connect);
		return Result<void, NetworkError>::Err(err);
	}

	return Result<void, NetworkError>::Ok();
}

Result<void, NetworkError> Socket::Close()
{
	if (!IsValid())
	{
		NetworkError err;
		err.Push(NetworkError::Socket_CloseFailed_Close);
		return Result<void, NetworkError>::Err(err);
	}

	SSIZE sockfd = (SSIZE)m_socket;
	System::Call(SYS_CLOSE, sockfd);
	m_socket = (PVOID)INVALID_FD;
	return Result<void, NetworkError>::Ok();
}

Result<SSIZE, NetworkError> Socket::Read(PVOID buffer, UINT32 bufferLength)
{
	if (!IsValid())
	{
		NetworkError err;
		err.Push(NetworkError::Socket_ReadFailed_HandleInvalid);
		return Result<SSIZE, NetworkError>::Err(err);
	}

	SSIZE sockfd = (SSIZE)m_socket;
	SSIZE result = System::Call(SYS_RECVFROM, sockfd, (USIZE)buffer, bufferLength, 0, 0, 0);
	if (result < 0)
	{
		NetworkError err;
		err.Push((UINT32)(-result));
		err.Push(NetworkError::Socket_ReadFailed_Recv);
		return Result<SSIZE, NetworkError>::Err(err);
	}

	return Result<SSIZE, NetworkError>::Ok(result);
}

Result<UINT32, NetworkError> Socket::Write(PCVOID buffer, UINT32 bufferLength)
{
	if (!IsValid())
	{
		NetworkError err;
		err.Push(NetworkError::Socket_WriteFailed_HandleInvalid);
		return Result<UINT32, NetworkError>::Err(err);
	}

	SSIZE  sockfd    = (SSIZE)m_socket;
	UINT32 totalSent = 0;

	while (totalSent < bufferLength)
	{
		SSIZE sent = System::Call(SYS_SENDTO, sockfd,
		                          (USIZE)((const CHAR *)buffer + totalSent),
		                          bufferLength - totalSent, 0, 0, 0);
		if (sent <= 0)
		{
			NetworkError err;
			if (sent < 0)
				err.Push((UINT32)(-sent));
			err.Push(NetworkError::Socket_WriteFailed_Send);
			return Result<UINT32, NetworkError>::Err(err);
		}

		totalSent += (UINT32)sent;
	}

	return Result<UINT32, NetworkError>::Ok(totalSent);
}
