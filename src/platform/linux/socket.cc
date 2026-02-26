#include "socket.h"
#include "syscall.h"
#include "system.h"
#include "memory.h"
#include "ip_address.h"

// Socket syscall helpers - i386 uses multiplexed socketcall(), others use direct syscalls
static SSIZE linux_socket(INT32 domain, INT32 type, INT32 protocol)
{
#if defined(ARCHITECTURE_I386)
	USIZE args[3] = {(USIZE)domain, (USIZE)type, (USIZE)protocol};
	return System::Call(SYS_SOCKETCALL, SOCKOP_SOCKET, (USIZE)args);
#else
	return System::Call(SYS_SOCKET, domain, type, protocol);
#endif
}

static SSIZE linux_bind(SSIZE sockfd, const SockAddr *addr, UINT32 addrlen)
{
#if defined(ARCHITECTURE_I386)
	USIZE args[3] = {(USIZE)sockfd, (USIZE)addr, addrlen};
	return System::Call(SYS_SOCKETCALL, SOCKOP_BIND, (USIZE)args);
#else
	return System::Call(SYS_BIND, sockfd, (USIZE)addr, addrlen);
#endif
}

static SSIZE linux_connect(SSIZE sockfd, const SockAddr *addr, UINT32 addrlen)
{
#if defined(ARCHITECTURE_I386)
	USIZE args[3] = {(USIZE)sockfd, (USIZE)addr, addrlen};
	return System::Call(SYS_SOCKETCALL, SOCKOP_CONNECT, (USIZE)args);
#else
	return System::Call(SYS_CONNECT, sockfd, (USIZE)addr, addrlen);
#endif
}

static SSIZE linux_send(SSIZE sockfd, const VOID *buf, USIZE len, INT32 flags)
{
#if defined(ARCHITECTURE_I386)
	USIZE args[4] = {(USIZE)sockfd, (USIZE)buf, len, (USIZE)flags};
	return System::Call(SYS_SOCKETCALL, SOCKOP_SEND, (USIZE)args);
#else
	return System::Call(SYS_SENDTO, sockfd, (USIZE)buf, len, flags, 0, 0);
#endif
}

static SSIZE linux_recv(SSIZE sockfd, VOID *buf, USIZE len, INT32 flags)
{
#if defined(ARCHITECTURE_I386)
	USIZE args[4] = {(USIZE)sockfd, (USIZE)buf, len, (USIZE)flags};
	return System::Call(SYS_SOCKETCALL, SOCKOP_RECV, (USIZE)args);
#else
	return System::Call(SYS_RECVFROM, sockfd, (USIZE)buf, len, flags, 0, 0);
#endif
}

Socket::Socket(const IPAddress &ipAddress, UINT16 port)
	: ip(ipAddress), port(port), m_socket(nullptr)
{
	SSIZE fd = linux_socket(SocketAddressHelper::GetAddressFamily(ip), SOCK_STREAM, IPPROTO_TCP);
	if (fd < 0)
	{
		m_socket = (PVOID)INVALID_FD;
		return;
	}
	m_socket = (PVOID)fd;
}

Result<void, Error> Socket::Bind(SockAddr &socketAddress, INT32 shareType)
{
	(VOID)shareType; // not used on Linux

	if (!IsValid())
		return Result<void, Error>::Err(Error::Socket_BindFailed_Bind);

	SSIZE sockfd  = (SSIZE)m_socket;
	UINT32 addrLen = (socketAddress.sin_family == AF_INET6) ? sizeof(SockAddr6) : sizeof(SockAddr);
	SSIZE  result  = linux_bind(sockfd, &socketAddress, addrLen);
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
	if (!IsValid())
		return Result<void, Error>::Err(Error::Socket_OpenFailed_HandleInvalid);

	SSIZE sockfd = (SSIZE)m_socket;

	union
	{
		SockAddr  addr4;
		SockAddr6 addr6;
	} addrBuffer;

	UINT32 addrLen = SocketAddressHelper::PrepareAddress(ip, port, &addrBuffer, sizeof(addrBuffer));
	if (addrLen == 0)
		return Result<void, Error>::Err(Error::Socket_OpenFailed_Connect);

	SSIZE result = linux_connect(sockfd, (SockAddr *)&addrBuffer, addrLen);
	if (result != 0)
	{
		return Result<void, Error>::Err(
			Error::Posix((UINT32)(-result)),
			Error::Socket_OpenFailed_Connect);
	}

	return Result<void, Error>::Ok();
}

Result<void, Error> Socket::Close()
{
	if (!IsValid())
		return Result<void, Error>::Err(Error::Socket_CloseFailed_Close);

	SSIZE sockfd = (SSIZE)m_socket;
	System::Call(SYS_CLOSE, sockfd);
	m_socket = (PVOID)INVALID_FD;
	return Result<void, Error>::Ok();
}

Result<SSIZE, Error> Socket::Read(PVOID buffer, UINT32 bufferLength)
{
	if (!IsValid())
		return Result<SSIZE, Error>::Err(Error::Socket_ReadFailed_HandleInvalid);

	SSIZE sockfd = (SSIZE)m_socket;
	SSIZE result = linux_recv(sockfd, buffer, bufferLength, 0);
	if (result < 0)
	{
		return Result<SSIZE, Error>::Err(
			Error::Posix((UINT32)(-result)),
			Error::Socket_ReadFailed_Recv);
	}

	return Result<SSIZE, Error>::Ok(result);
}

Result<UINT32, Error> Socket::Write(PCVOID buffer, UINT32 bufferLength)
{
	if (!IsValid())
		return Result<UINT32, Error>::Err(Error::Socket_WriteFailed_HandleInvalid);

	SSIZE  sockfd    = (SSIZE)m_socket;
	UINT32 totalSent = 0;

	while (totalSent < bufferLength)
	{
		SSIZE sent = linux_send(sockfd, (const CHAR *)buffer + totalSent,
		                        bufferLength - totalSent, 0);
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
