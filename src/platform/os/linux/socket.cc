#include "socket.h"
#include "syscall.h"
#include "system.h"
#include "memory.h"
#include "ip_address.h"
#include "logger.h"

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

static SSIZE linux_getsockopt(SSIZE sockfd, INT32 level, INT32 optname, PVOID optval, UINT32 *optlen)
{
#if defined(ARCHITECTURE_I386)
	USIZE args[5] = {(USIZE)sockfd, (USIZE)level, (USIZE)optname, (USIZE)optval, (USIZE)optlen};
	return System::Call(SYS_SOCKETCALL, SOCKOP_GETSOCKOPT, (USIZE)args);
#else
	return System::Call(SYS_GETSOCKOPT, sockfd, (USIZE)level, (USIZE)optname, (USIZE)optval, (USIZE)optlen);
#endif
}

static SSIZE linux_fcntl(SSIZE fd, INT32 cmd, SSIZE arg = 0)
{
#if defined(ARCHITECTURE_I386) || defined(ARCHITECTURE_ARMV7A)
	return System::Call(SYS_FCNTL64, fd, (USIZE)cmd, (USIZE)arg);
#else
	return System::Call(SYS_FCNTL, fd, (USIZE)cmd, (USIZE)arg);
#endif
}

static SSIZE linux_ppoll(struct pollfd *fds, USIZE nfds, const struct timespec *timeout)
{
	return System::Call(SYS_PPOLL, (USIZE)fds, nfds, (USIZE)timeout, 0, 0);
}

Result<Socket, Error> Socket::Create(const IPAddress &ipAddress, UINT16 port)
{
	Socket sock(ipAddress, port);
	SSIZE fd = linux_socket(SocketAddressHelper::GetAddressFamily(sock.ip), SOCK_STREAM, IPPROTO_TCP);
	if (fd < 0)
		return Result<Socket, Error>::Err(Error::Posix((UINT32)(-fd)), Error::Socket_CreateFailed_Open);
	sock.m_socket = (PVOID)fd;
	return Result<Socket, Error>::Ok(static_cast<Socket &&>(sock));
}

Result<void, Error> Socket::Bind(SockAddr &socketAddress, INT32 shareType)
{
	(VOID)shareType; // not used on Linux

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
	SSIZE flags = linux_fcntl(sockfd, F_GETFL);
	if (flags < 0)
		return Result<void, Error>::Err(Error::Posix((UINT32)(-flags)), Error::Socket_OpenFailed_Connect);

	SSIZE setResult = linux_fcntl(sockfd, F_SETFL, flags | O_NONBLOCK);
	if (setResult < 0)
		return Result<void, Error>::Err(Error::Posix((UINT32)(-setResult)), Error::Socket_OpenFailed_Connect);

	SSIZE result = linux_connect(sockfd, (SockAddr *)&addrBuffer, addrLen);
	if (result != 0 && (-result) != EINPROGRESS)
	{
		// Restore blocking mode before returning error
		(void)linux_fcntl(sockfd, F_SETFL, flags);
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

		struct timespec timeout;
		timeout.tv_sec = 5;
		timeout.tv_nsec = 0;

		SSIZE pollResult = linux_ppoll(&pfd, 1, &timeout);
		if (pollResult <= 0)
		{
			(void)linux_fcntl(sockfd, F_SETFL, flags);
			return Result<void, Error>::Err(Error::Socket_OpenFailed_Connect);
		}

		// Check for connection error
		INT32 sockError = 0;
		UINT32 optLen = sizeof(sockError);
		(void)linux_getsockopt(sockfd, SOL_SOCKET, SO_ERROR, &sockError, &optLen);
		if (sockError != 0)
		{
			(void)linux_fcntl(sockfd, F_SETFL, flags);
			return Result<void, Error>::Err(
				Error::Posix((UINT32)sockError),
				Error::Socket_OpenFailed_Connect);
		}
	}

	// Restore blocking mode
	(void)linux_fcntl(sockfd, F_SETFL, flags);
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
	SSIZE result = linux_recv(sockfd, bufferPtr, bufferLength, 0);
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
		SSIZE sent = linux_send(sockfd, (const CHAR *)bufferPtr + totalSent,
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
