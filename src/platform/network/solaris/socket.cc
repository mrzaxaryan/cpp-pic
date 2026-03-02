#include "platform/network/socket.h"
#include "platform/common/solaris/syscall.h"
#include "platform/common/solaris/system.h"
#include "core/memory/memory.h"
#include "core/types/ip_address.h"

// Solaris uses direct socket syscalls (SYS_so_socket etc.), no multiplexing.
// pollsys is used instead of poll/ppoll for connection timeout.

static SSIZE SolarisPollsys(Pollfd *fds, USIZE nfds, const Timespec *timeout)
{
	return System::Call(SYS_POLLSYS, (USIZE)fds, nfds, (USIZE)timeout, 0);
}

Result<Socket, Error> Socket::Create(const IPAddress &ipAddress, UINT16 port)
{
	Socket sock(ipAddress, port);
	SSIZE fd = System::Call(SYS_SO_SOCKET, SocketAddressHelper::GetAddressFamily(sock.ip), SOCK_STREAM, IPPROTO_TCP);
	if (fd < 0)
		return Result<Socket, Error>::Err(Error::Posix((UINT32)(-fd)), Error::Socket_CreateFailed_Open);
	sock.handle = (PVOID)fd;
	return Result<Socket, Error>::Ok(static_cast<Socket &&>(sock));
}

Result<void, Error> Socket::Bind(const SockAddr &socketAddress, INT32 shareType)
{
	(VOID)shareType; // not used on Solaris

	SSIZE  sockfd  = (SSIZE)handle;
	UINT32 addrLen = (socketAddress.SinFamily == AF_INET6) ? sizeof(SockAddr6) : sizeof(SockAddr);
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
		// Connect in progress — wait with 5-second timeout via pollsys
		Pollfd pfd;
		pfd.Fd = (INT32)sockfd;
		pfd.Events = POLLOUT;
		pfd.Revents = 0;

		Timespec timeout;
		timeout.Sec = 5;
		timeout.Nsec = 0;

		SSIZE pollResult = SolarisPollsys(&pfd, 1, &timeout);
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
	SSIZE sockfd = (SSIZE)handle;
	System::Call(SYS_CLOSE, sockfd);
	handle = nullptr;
	return Result<void, Error>::Ok();
}

Result<SSIZE, Error> Socket::Read(Span<CHAR> buffer)
{
	SSIZE sockfd = (SSIZE)handle;
	SSIZE result = System::Call(SYS_RECVFROM, sockfd, (USIZE)buffer.Data(), (UINT32)buffer.Size(), 0, 0, 0);
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
		SSIZE sent = System::Call(SYS_SENDTO, sockfd,
		                          (USIZE)((const CHAR *)buffer.Data() + totalSent),
		                          (UINT32)buffer.Size() - totalSent, 0, 0, 0);
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
