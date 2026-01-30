#include "socket.h"
#include "syscall.h"
#include "primitives.h"
#include "memory.h"
#include "ip_address.h"

// macOS syscall numbers for socket operations
constexpr USIZE SYS_SOCKET = 97;
constexpr USIZE SYS_CONNECT = 98;
constexpr USIZE SYS_SENDTO = 133;
constexpr USIZE SYS_RECVFROM = 29;
constexpr USIZE SYS_BIND = 104;
constexpr USIZE SYS_CLOSE = 6;

// Protocol numbers
#define IPPROTO_TCP 6

// Socket invalid value
constexpr SSIZE INVALID_SOCKET = -1;

// macOS uses direct syscalls (no socketcall multiplexer)
static SSIZE macos_socket(INT32 domain, INT32 type, INT32 protocol)
{
    return Syscall::syscall3(SYS_SOCKET, domain, type, protocol);
}

static SSIZE macos_bind(SSIZE sockfd, const SockAddr* addr, UINT32 addrlen)
{
    return Syscall::syscall3(SYS_BIND, sockfd, (USIZE)addr, addrlen);
}

static SSIZE macos_connect(SSIZE sockfd, const SockAddr* addr, UINT32 addrlen)
{
    return Syscall::syscall3(SYS_CONNECT, sockfd, (USIZE)addr, addrlen);
}

static SSIZE macos_send(SSIZE sockfd, const VOID* buf, USIZE len, INT32 flags)
{
    return Syscall::syscall6(SYS_SENDTO, sockfd, (USIZE)buf, len, flags, 0, 0);
}

static SSIZE macos_recv(SSIZE sockfd, VOID* buf, USIZE len, INT32 flags)
{
    return Syscall::syscall6(SYS_RECVFROM, sockfd, (USIZE)buf, len, flags, 0, 0);
}

// Socket constructor
Socket::Socket(const IPAddress& ipAddress, UINT16 port)
    : ip(ipAddress), port(port), m_socket(NULL)
{
    INT32 addressFamily = ip.IsIPv6() ? AF_INET6 : AF_INET;
    INT32 socketType = SOCK_STREAM;
    INT32 protocol = IPPROTO_TCP;

    SSIZE fd = macos_socket(addressFamily, socketType, protocol);
    if (fd < 0)
    {
        m_socket = (PVOID)INVALID_SOCKET;
        return;
    }

    m_socket = (PVOID)fd;
}

// Bind socket to address
BOOL Socket::Bind(SockAddr* socketAddress, INT32 shareType)
{
    (VOID)shareType;  // Not used on macOS

    if (!IsValid())
        return FALSE;

    SSIZE sockfd = (SSIZE)m_socket;

    if (socketAddress->sin_family == AF_INET6)
    {
        SockAddr6* addr6 = (SockAddr6*)socketAddress;
        SSIZE result = macos_bind(sockfd, (SockAddr*)addr6, sizeof(SockAddr6));
        return result == 0;
    }
    else
    {
        SSIZE result = macos_bind(sockfd, socketAddress, sizeof(SockAddr));
        return result == 0;
    }
}

// Open/Connect socket
BOOL Socket::Open()
{
    if (!IsValid())
        return FALSE;

    SSIZE sockfd = (SSIZE)m_socket;

    // Prepare socket address
    if (ip.IsIPv6())
    {
        SockAddr6 addr;
        Memory::Zero(&addr, sizeof(addr));
        addr.sin6_family = AF_INET6;
        addr.sin6_port = UINT16SwapByteOrder(port);

        const UINT8* ipv6Addr = ip.ToIPv6();
        if (ipv6Addr != NULL)
        {
            Memory::Copy(addr.sin6_addr, ipv6Addr, 16);
        }

        addr.sin6_flowinfo = 0;
        addr.sin6_scope_id = 0;

        SSIZE result = macos_connect(sockfd, (SockAddr*)&addr, sizeof(addr));
        return result == 0;
    }
    else
    {
        SockAddr addr;
        Memory::Zero(&addr, sizeof(addr));
        addr.sin_family = AF_INET;
        addr.sin_port = UINT16SwapByteOrder(port);
        addr.sin_addr = ip.ToIPv4();

        SSIZE result = macos_connect(sockfd, &addr, sizeof(addr));
        return result == 0;
    }
}

// Close socket
BOOL Socket::Close()
{
    if (!IsValid())
        return FALSE;

    SSIZE sockfd = (SSIZE)m_socket;
    Syscall::syscall1(SYS_CLOSE, sockfd);
    m_socket = (PVOID)INVALID_SOCKET;
    return TRUE;
}

// Read from socket
SSIZE Socket::Read(PVOID buffer, UINT32 bufferLength)
{
    if (!IsValid())
        return -1;

    SSIZE sockfd = (SSIZE)m_socket;
    SSIZE result = macos_recv(sockfd, buffer, bufferLength, 0);
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
        SSIZE sent = macos_send(sockfd, (const CHAR*)buffer + totalSent,
                                bufferLength - totalSent, 0);
        if (sent <= 0)
            break;

        totalSent += (UINT32)sent;
    }

    return totalSent;
}
