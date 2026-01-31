#include "socket.h"
#include "system.h"
#include "memory.h"
#include "ip_address.h"

// Linux syscall numbers for socket operations
#if defined(ARCHITECTURE_X86_64)
constexpr USIZE SYS_SOCKET = 41;
constexpr USIZE SYS_CONNECT = 42;
constexpr USIZE SYS_SENDTO = 44;
constexpr USIZE SYS_RECVFROM = 45;
constexpr USIZE SYS_BIND = 49;
constexpr USIZE SYS_CLOSE = 3;
#elif defined(ARCHITECTURE_I386)
// i386 uses socketcall multiplexer
constexpr USIZE SYS_SOCKETCALL = 102;
constexpr USIZE SYS_CLOSE = 6;
// socketcall opcodes
constexpr USIZE SYS_SOCKET_SC = 1;
constexpr USIZE SYS_BIND_SC = 2;
constexpr USIZE SYS_CONNECT_SC = 3;
constexpr USIZE SYS_SEND_SC = 9;
constexpr USIZE SYS_RECV_SC = 10;
#elif defined(ARCHITECTURE_AARCH64)
constexpr USIZE SYS_SOCKET = 198;
constexpr USIZE SYS_CONNECT = 203;
constexpr USIZE SYS_SENDTO = 206;
constexpr USIZE SYS_RECVFROM = 207;
constexpr USIZE SYS_BIND = 200;
constexpr USIZE SYS_CLOSE = 57;
#elif defined(ARCHITECTURE_ARMV7A)
// ARMv7 uses socketcall multiplexer
constexpr USIZE SYS_SOCKETCALL = 102;
constexpr USIZE SYS_CLOSE = 6;
// socketcall opcodes
constexpr USIZE SYS_SOCKET_SC = 1;
constexpr USIZE SYS_BIND_SC = 2;
constexpr USIZE SYS_CONNECT_SC = 3;
constexpr USIZE SYS_SEND_SC = 9;
constexpr USIZE SYS_RECV_SC = 10;
#endif

// Protocol numbers
#define IPPROTO_TCP 6

// Socket invalid value
constexpr SSIZE INVALID_SOCKET = -1;

// Helper functions for socketcall architectures (i386, ARMv7)
#if defined(ARCHITECTURE_I386) || defined(ARCHITECTURE_ARMV7A)

static SSIZE linux_socket(INT32 domain, INT32 type, INT32 protocol)
{
    USIZE args[3] = {(USIZE)domain, (USIZE)type, (USIZE)protocol};
    return System::Call(SYS_SOCKETCALL, SYS_SOCKET_SC, (USIZE)args);
}

static SSIZE linux_bind(SSIZE sockfd, const SockAddr* addr, UINT32 addrlen)
{
    USIZE args[3] = {(USIZE)sockfd, (USIZE)addr, addrlen};
    return System::Call(SYS_SOCKETCALL, SYS_BIND_SC, (USIZE)args);
}

static SSIZE linux_connect(SSIZE sockfd, const SockAddr* addr, UINT32 addrlen)
{
    USIZE args[3] = {(USIZE)sockfd, (USIZE)addr, addrlen};
    return System::Call(SYS_SOCKETCALL, SYS_CONNECT_SC, (USIZE)args);
}

static SSIZE linux_send(SSIZE sockfd, const VOID* buf, USIZE len, INT32 flags)
{
    USIZE args[4] = {(USIZE)sockfd, (USIZE)buf, len, (USIZE)flags};
    return System::Call(SYS_SOCKETCALL, SYS_SEND_SC, (USIZE)args);
}

static SSIZE linux_recv(SSIZE sockfd, VOID* buf, USIZE len, INT32 flags)
{
    USIZE args[4] = {(USIZE)sockfd, (USIZE)buf, len, (USIZE)flags};
    return System::Call(SYS_SOCKETCALL, SYS_RECV_SC, (USIZE)args);
}

#else

// Direct syscall versions for x86_64 and AArch64
static SSIZE linux_socket(INT32 domain, INT32 type, INT32 protocol)
{
    return System::Call(SYS_SOCKET, domain, type, protocol);
}

static SSIZE linux_bind(SSIZE sockfd, const SockAddr* addr, UINT32 addrlen)
{
    return System::Call(SYS_BIND, sockfd, (USIZE)addr, addrlen);
}

static SSIZE linux_connect(SSIZE sockfd, const SockAddr* addr, UINT32 addrlen)
{
    return System::Call(SYS_CONNECT, sockfd, (USIZE)addr, addrlen);
}

static SSIZE linux_send(SSIZE sockfd, const VOID* buf, USIZE len, INT32 flags)
{
    return System::Call(SYS_SENDTO, sockfd, (USIZE)buf, len, flags, 0, 0);
}

static SSIZE linux_recv(SSIZE sockfd, VOID* buf, USIZE len, INT32 flags)
{
    return System::Call(SYS_RECVFROM, sockfd, (USIZE)buf, len, flags, 0, 0);
}

#endif

// Socket constructor
Socket::Socket(const IPAddress& ipAddress, UINT16 port)
    : ip(ipAddress), port(port), m_socket(NULL)
{
    INT32 addressFamily = ip.IsIPv6() ? AF_INET6 : AF_INET;
    INT32 socketType = SOCK_STREAM;
    INT32 protocol = IPPROTO_TCP;

    SSIZE fd = linux_socket(addressFamily, socketType, protocol);
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
    (VOID)shareType;  // Not used on Linux

    if (!IsValid())
        return FALSE;

    SSIZE sockfd = (SSIZE)m_socket;

    if (socketAddress->sin_family == AF_INET6)
    {
        SockAddr6* addr6 = (SockAddr6*)socketAddress;
        SSIZE result = linux_bind(sockfd, (SockAddr*)addr6, sizeof(SockAddr6));
        return result == 0;
    }
    else
    {
        SSIZE result = linux_bind(sockfd, socketAddress, sizeof(SockAddr));
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

        SSIZE result = linux_connect(sockfd, (SockAddr*)&addr, sizeof(addr));
        return result == 0;
    }
    else
    {
        SockAddr addr;
        Memory::Zero(&addr, sizeof(addr));
        addr.sin_family = AF_INET;
        addr.sin_port = UINT16SwapByteOrder(port);
        addr.sin_addr = ip.ToIPv4();

        SSIZE result = linux_connect(sockfd, &addr, sizeof(addr));
        return result == 0;
    }
}

// Close socket
BOOL Socket::Close()
{
    if (!IsValid())
        return FALSE;

    SSIZE sockfd = (SSIZE)m_socket;
    System::Call(SYS_CLOSE, sockfd);
    m_socket = (PVOID)INVALID_SOCKET;
    return TRUE;
}

// Read from socket
SSIZE Socket::Read(PVOID buffer, UINT32 bufferLength)
{
    if (!IsValid())
        return -1;

    SSIZE sockfd = (SSIZE)m_socket;
    SSIZE result = linux_recv(sockfd, buffer, bufferLength, 0);
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
        SSIZE sent = linux_send(sockfd, (const CHAR*)buffer + totalSent,
                                bufferLength - totalSent, 0);
        if (sent <= 0)
            break;

        totalSent += (UINT32)sent;
    }

    return totalSent;
}
