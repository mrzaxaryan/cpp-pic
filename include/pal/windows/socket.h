#pragma once

#include "primitives.h"
#include "network.h"

/* Socket address families */
#define AF_INET      2

/* Socket types */
#define SOCK_STREAM  1
#define SOCK_DGRAM   2

/* Shutdown modes */
#define SHUT_RD      0
#define SHUT_WR      1
#define SHUT_RDWR    2

struct SockAddr
{
	INT16 sin_family;
	UINT16 sin_port;
	UINT32 sin_addr;
	CHAR sin_zero[8];
};

class Socket
{
private:
	UINT32 ip;
	UINT16 port;
	PVOID m_socket;
	BOOL Bind(SockAddr *SocketAddress, INT32 ShareType);

public:
	VOID *operator new(USIZE) = delete;
	VOID operator delete(VOID *) = delete;
	Socket() = default;
	Socket(UINT32 ip, UINT16 port);
	BOOL IsValid() const { return m_socket != NULL && m_socket != (PVOID)(SSIZE)(-1); }
	BOOL Open();
	BOOL Close();
	SSIZE Read(PVOID buffer, UINT32 bufferLength);
	UINT32 Write(PCVOID buffer, UINT32 bufferLength);
};