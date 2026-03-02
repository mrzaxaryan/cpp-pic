#include "platform/network/socket.h"
#include "platform/common/linux/syscall.h"
#include "platform/common/linux/system.h"
#include "core/memory/memory.h"
#include "core/types/ip_address.h"
#include "platform/io/logger.h"

// Socket syscall helpers - i386 uses multiplexed socketcall(), others use direct syscalls
static SSIZE LinuxSocket(INT32 domain, INT32 type, INT32 protocol)
{
#if defined(ARCHITECTURE_I386)
	USIZE args[3] = {(USIZE)domain, (USIZE)type, (USIZE)protocol};
	return System::Call(SYS_SOCKETCALL, SOCKOP_SOCKET, (USIZE)args);
#else
	return System::Call(SYS_SOCKET, domain, type, protocol);
#endif
}

static SSIZE LinuxBind(SSIZE sockfd, const SockAddr &addr, UINT32 addrlen)
{
#if defined(ARCHITECTURE_I386)
	USIZE args[3] = {(USIZE)sockfd, (USIZE)&addr, addrlen};
	return System::Call(SYS_SOCKETCALL, SOCKOP_BIND, (USIZE)args);
#else
	return System::Call(SYS_BIND, sockfd, (USIZE)&addr, addrlen);
#endif
}

static SSIZE LinuxConnect(SSIZE sockfd, const SockAddr &addr, UINT32 addrlen)
{
#if defined(ARCHITECTURE_I386)
	USIZE args[3] = {(USIZE)sockfd, (USIZE)&addr, addrlen};
	return System::Call(SYS_SOCKETCALL, SOCKOP_CONNECT, (USIZE)args);
#else
	return System::Call(SYS_CONNECT, sockfd, (USIZE)&addr, addrlen);
#endif
}

static SSIZE LinuxSend(SSIZE sockfd, const VOID *buf, USIZE len, INT32 flags)
{
#if defined(ARCHITECTURE_I386)
	USIZE args[4] = {(USIZE)sockfd, (USIZE)buf, len, (USIZE)flags};
	return System::Call(SYS_SOCKETCALL, SOCKOP_SEND, (USIZE)args);
#else
	return System::Call(SYS_SENDTO, sockfd, (USIZE)buf, len, flags, 0, 0);
#endif
}

static SSIZE LinuxRecv(SSIZE sockfd, VOID *buf, USIZE len, INT32 flags)
{
#if defined(ARCHITECTURE_I386)
	USIZE args[4] = {(USIZE)sockfd, (USIZE)buf, len, (USIZE)flags};
	return System::Call(SYS_SOCKETCALL, SOCKOP_RECV, (USIZE)args);
#else
	return System::Call(SYS_RECVFROM, sockfd, (USIZE)buf, len, flags, 0, 0);
#endif
}

static SSIZE LinuxGetsockopt(SSIZE sockfd, INT32 level, INT32 optname, PVOID optval, UINT32 *optlen)
{
#if defined(ARCHITECTURE_I386)
	USIZE args[5] = {(USIZE)sockfd, (USIZE)level, (USIZE)optname, (USIZE)optval, (USIZE)optlen};
	return System::Call(SYS_SOCKETCALL, SOCKOP_GETSOCKOPT, (USIZE)args);
#else
	return System::Call(SYS_GETSOCKOPT, sockfd, (USIZE)level, (USIZE)optname, (USIZE)optval, (USIZE)optlen);
#endif
}

static SSIZE LinuxFcntl(SSIZE fd, INT32 cmd, SSIZE arg = 0)
{
#if defined(ARCHITECTURE_I386) || defined(ARCHITECTURE_ARMV7A)
	return System::Call(SYS_FCNTL64, fd, (USIZE)cmd, (USIZE)arg);
#else
	return System::Call(SYS_FCNTL, fd, (USIZE)cmd, (USIZE)arg);
#endif
}

static SSIZE LinuxPpoll(Pollfd &fds, USIZE nfds, const Timespec &timeout)
{
	return System::Call(SYS_PPOLL, (USIZE)&fds, nfds, (USIZE)&timeout, 0, 0);
}

Result<Socket, Error> Socket::Create(const IPAddress &ipAddress, UINT16 port)
{
	Socket sock(ipAddress, port);
	SSIZE fd = LinuxSocket(SocketAddressHelper::GetAddressFamily(sock.ip), SOCK_STREAM, IPPROTO_TCP);
	if (fd < 0)
		return Result<Socket, Error>::Err(Error::Posix((UINT32)(-fd)), Error::Socket_CreateFailed_Open);
	sock.handle = (PVOID)fd;
	return Result<Socket, Error>::Ok(static_cast<Socket &&>(sock));
}

Result<void, Error> Socket::Bind(const SockAddr &socketAddress, INT32 shareType)
{
	(VOID)shareType; // not used on Linux

	SSIZE sockfd  = (SSIZE)handle;
	UINT32 addrLen = (socketAddress.SinFamily == AF_INET6) ? sizeof(SockAddr6) : sizeof(SockAddr);
	SSIZE  result  = LinuxBind(sockfd, socketAddress, addrLen);
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
	SSIZE sockfd = (SSIZE)handle;

	union
	{
		SockAddr  addr4;
		SockAddr6 addr6;
	} addrBuffer;

	UINT32 addrLen = SocketAddressHelper::PrepareAddress(ip, port, Span<UINT8>((UINT8 *)&addrBuffer, sizeof(addrBuffer)));
	if (addrLen == 0)
		return Result<void, Error>::Err(Error::Socket_OpenFailed_Connect);

	// Set socket to non-blocking for connect with timeout
	SSIZE flags = LinuxFcntl(sockfd, F_GETFL);
	if (flags < 0)
		return Result<void, Error>::Err(Error::Posix((UINT32)(-flags)), Error::Socket_OpenFailed_Connect);

	SSIZE setResult = LinuxFcntl(sockfd, F_SETFL, flags | O_NONBLOCK);
	if (setResult < 0)
		return Result<void, Error>::Err(Error::Posix((UINT32)(-setResult)), Error::Socket_OpenFailed_Connect);

	SSIZE result = LinuxConnect(sockfd, *(const SockAddr *)&addrBuffer, addrLen);
	if (result != 0 && (-result) != EINPROGRESS)
	{
		// Restore blocking mode before returning error
		(void)LinuxFcntl(sockfd, F_SETFL, flags);
		return Result<void, Error>::Err(
			Error::Posix((UINT32)(-result)),
			Error::Socket_OpenFailed_Connect);
	}

	if (result != 0)
	{
		// Connect in progress — wait with 5-second timeout
		Pollfd pfd;
		pfd.Fd = (INT32)sockfd;
		pfd.Events = POLLOUT;
		pfd.Revents = 0;

		Timespec timeout;
		timeout.Sec = 5;
		timeout.Nsec = 0;

		SSIZE pollResult = LinuxPpoll(pfd, 1, timeout);
		if (pollResult <= 0)
		{
			(void)LinuxFcntl(sockfd, F_SETFL, flags);
			return Result<void, Error>::Err(Error::Socket_OpenFailed_Connect);
		}

		// Check for connection error
		INT32 sockError = 0;
		UINT32 optLen = sizeof(sockError);
		(void)LinuxGetsockopt(sockfd, SOL_SOCKET, SO_ERROR, &sockError, &optLen);
		if (sockError != 0)
		{
			(void)LinuxFcntl(sockfd, F_SETFL, flags);
			return Result<void, Error>::Err(
				Error::Posix((UINT32)sockError),
				Error::Socket_OpenFailed_Connect);
		}
	}

	// Restore blocking mode
	(void)LinuxFcntl(sockfd, F_SETFL, flags);
	return Result<void, Error>::Ok();
}

Result<void, Error> Socket::Close()
{
	SSIZE sockfd = (SSIZE)handle;
	System::Call(SYS_CLOSE, sockfd);
	handle = nullptr;
	return Result<void, Error>::Ok();
}

Result<SSIZE, Error> Socket::Read(Span<CHAR> buffer)
{
	SSIZE sockfd = (SSIZE)handle;
	SSIZE result = LinuxRecv(sockfd, (PVOID)buffer.Data(), (UINT32)buffer.Size(), 0);
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
	SSIZE  sockfd    = (SSIZE)handle;
	UINT32 totalSent = 0;

	while (totalSent < (UINT32)buffer.Size())
	{
		SSIZE sent = LinuxSend(sockfd, (const CHAR *)buffer.Data() + totalSent,
		                        (UINT32)buffer.Size() - totalSent, 0);
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
