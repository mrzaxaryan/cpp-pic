/**
 * socket.cc - UEFI Socket Stub Implementation
 *
 * UEFI networking requires EFI_TCP4_PROTOCOL or EFI_TCP6_PROTOCOL
 * which are complex to implement. This is a stub for initial bring-up.
 */

#include "socket.h"

Socket::Socket(const IPAddress &ipAddress, UINT16 portNum)
	: ip(ipAddress), port(portNum), m_socket(NULL)
{
	// Suppress unused warnings for stub implementation
	(VOID)ip;
	(VOID)port;
}

BOOL Socket::Open()
{
	// UEFI networking not implemented
	return FALSE;
}

BOOL Socket::Close()
{
	m_socket = NULL;
	return FALSE;
}

BOOL Socket::Bind(SockAddr *SocketAddress, INT32 ShareType)
{
	(VOID)SocketAddress;
	(VOID)ShareType;
	return FALSE;
}

SSIZE Socket::Read(PVOID buffer, UINT32 bufferLength)
{
	(VOID)buffer;
	(VOID)bufferLength;
	return -1;
}

UINT32 Socket::Write(PCVOID buffer, UINT32 bufferLength)
{
	(VOID)buffer;
	(VOID)bufferLength;
	return 0;
}
