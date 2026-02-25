#include "primitives.h"
#include "windows_types.h"
#include "socket.h"
#include "logger.h"
#include "ntdll.h"
#include "memory.h"

#define IOCTL_AFD_BIND ((0x00000012) << 12 | (0 << 2) | 3)
#define IOCTL_AFD_CONNECT ((0x00000012) << 12 | (1 << 2) | 3)
#define IOCTL_AFD_SEND ((0x00000012) << 12 | (7 << 2) | 3)
#define IOCTL_AFD_RECV ((0x00000012) << 12 | (5 << 2) | 3)
#define IOCTL_AFD_DISCONNECT ((0x00000012) << 12 | (10 << 2) | 3)

#define AFD_SHARE_REUSE 0x1L
#define AFD_DISCONNECT_SEND 0x01L
#define AFD_DISCONNECT_RECV 0x02L
#define AFD_DISCONNECT_ABORT 0x04L
#define AFD_DISCONNECT_DATAGRAM 0x08L

#define OBJ_INHERIT 0x00000002L

#define STATUS_PENDING ((UINT32)0x00000103L)
#define CRYPT_STRING_BASE64 0x00000001
#define IPPROTO_TCP 6
#define ERROR_SUCCESS 0L
#define SOCKET_ERROR (-1)

typedef struct AfdBindData
{
    UINT32 ShareType;
    SockAddr Address;
} AfdBindData;

typedef struct AfdBindData6
{
    UINT32 ShareType;
    SockAddr6 Address;
} AfdBindData6;

typedef struct AfdConnectInfo
{
    SSIZE UseSAN;
    SSIZE Root;
    SSIZE Unknown;
    SockAddr Address;
} AfdConnectInfo;

typedef struct AfdConnectInfo6
{
    SSIZE UseSAN;
    SSIZE Root;
    SSIZE Unknown;
    SockAddr6 Address;
} AfdConnectInfo6;

typedef struct AfdDataBuffer
{
    UINT32 DataLength;
    PUINT8 Data;
} AfdDataBuffer;

typedef struct AfdSendRecvInfo
{
    PVOID BufferArray;
    UINT32 BufferCount;
    UINT32 AfdFlags;
    UINT32 TdiFlags;
} AfdSendRecvInfo;

typedef struct AfdWsaBuf
{
    UINT32 Length;
    PVOID Buffer;
} AfdWsaBuf;

typedef struct AfdSocketParams
{
    INT32 Reserved;
    UINT16 EaNameLength;
    UINT16 EaValueLength;
    CHAR AfdOperation[16];
    UINT32 Flags;
    INT32 GroupId;
    INT32 AddressFamily;
    INT32 SocketType;
    INT32 Protocol;
    UINT32 ProviderInfoLength;
    WCHAR ProviderInfo[8];
} AfdSocketParams;

BOOL Socket::Bind(SockAddr &SocketAddress, INT32 ShareType)
{
    LOG_DEBUG("Bind(pNTSocket: 0x%p, SocketAddress: 0x%p, ShareType: %d)\n", m_socket, &SocketAddress, ShareType);

    if (!IsValid())
    {
        LOG_ERROR("Socket not initialized\n");
        return FALSE;
    }

    NTSTATUS Status;
    PVOID SockEvent = NULL;
    Status = NTDLL::ZwCreateEvent(&SockEvent,
                                  EVENT_ALL_ACCESS,
                                  NULL,
                                  SynchronizationEvent,
                                  FALSE);

    if (!NT_SUCCESS(Status))
    {
        LOG_ERROR("Failed to create event for socket binding: 0x%08X\n", Status);
        return FALSE;
    }
    LOG_DEBUG("Event successfully created for socket binding\n");

    IO_STATUS_BLOCK IOSB;
    UINT8 OutputBlock[40];
    Memory::Zero(&OutputBlock, sizeof(OutputBlock));

    if (SocketAddress.sin_family == AF_INET6)
    {
        AfdBindData6 BindConfig;
        Memory::Zero(&BindConfig, sizeof(BindConfig));
        BindConfig.ShareType = ShareType;
        BindConfig.Address = (SockAddr6 &)SocketAddress;

        Status = NTDLL::ZwDeviceIoControlFile(m_socket,
                                              SockEvent,
                                              NULL,
                                              NULL,
                                              &IOSB,
                                              IOCTL_AFD_BIND,
                                              &BindConfig,
                                              sizeof(BindConfig),
                                              &OutputBlock,
                                              sizeof(OutputBlock));
    }
    else
    {
        AfdBindData BindConfig;
        Memory::Zero(&BindConfig, sizeof(BindConfig));
        BindConfig.ShareType = ShareType;
        BindConfig.Address = SocketAddress;

        Status = NTDLL::ZwDeviceIoControlFile(m_socket,
                                              SockEvent,
                                              NULL,
                                              NULL,
                                              &IOSB,
                                              IOCTL_AFD_BIND,
                                              &BindConfig,
                                              sizeof(BindConfig),
                                              &OutputBlock,
                                              sizeof(OutputBlock));
    }

    if (Status == STATUS_PENDING)
    {
        NTDLL::ZwWaitForSingleObject(SockEvent, 0, NULL);
        Status = IOSB.Status;
    }

    NTDLL::ZwClose(SockEvent);
    if (!NT_SUCCESS(Status))
    {
        LOG_ERROR("AFD_BIND failed with NTSTATUS: 0x%08X\n", Status);
    }
    return NT_SUCCESS(Status);
}

BOOL Socket::Open()
{
    LOG_DEBUG("Connect(pNTSocket: 0x%p, port: %d)\n", this, port);

    NTSTATUS Status = 0;

    // Prepare bind address using helper
    union
    {
        SockAddr addr4;
        SockAddr6 addr6;
    } bindBuffer;

    SocketAddressHelper::PrepareBindAddress(ip.IsIPv6(), 0, &bindBuffer, sizeof(bindBuffer));
    if (Bind((SockAddr &)bindBuffer, AFD_SHARE_REUSE) == FALSE)
    {
        LOG_ERROR("Failed to bind socket\n");
        return FALSE;
    }
    LOG_DEBUG("Socket bound successfully\n");

    PVOID SockEvent = NULL;
    Status = NTDLL::ZwCreateEvent(&SockEvent,
                                  EVENT_ALL_ACCESS,
                                  NULL,
                                  SynchronizationEvent,
                                  FALSE);

    if (!NT_SUCCESS(Status))
    {
        LOG_ERROR("Failed to create event\n");
        return FALSE;
    }
    LOG_DEBUG("Event successfully created for socket connection\n");

    IO_STATUS_BLOCK IOSB;

    // Prepare connect address using helper
    union
    {
        SockAddr addr4;
        SockAddr6 addr6;
    } addrBuffer;

    SocketAddressHelper::PrepareAddress(ip, port, &addrBuffer, sizeof(addrBuffer));

    if (ip.IsIPv6())
    {
        AfdConnectInfo6 ConnectInfo;
        ConnectInfo.UseSAN = 0;
        ConnectInfo.Root = 0;
        ConnectInfo.Unknown = 0;
        ConnectInfo.Address = addrBuffer.addr6;

        Status = NTDLL::ZwDeviceIoControlFile((PVOID)m_socket,
                                              SockEvent,
                                              NULL,
                                              NULL,
                                              &IOSB,
                                              IOCTL_AFD_CONNECT,
                                              &ConnectInfo,
                                              sizeof(ConnectInfo),
                                              NULL,
                                              0);
    }
    else
    {
        AfdConnectInfo ConnectInfo;
        ConnectInfo.UseSAN = 0;
        ConnectInfo.Root = 0;
        ConnectInfo.Unknown = 0;
        ConnectInfo.Address = addrBuffer.addr4;

        Status = NTDLL::ZwDeviceIoControlFile((PVOID)m_socket,
                                              SockEvent,
                                              NULL,
                                              NULL,
                                              &IOSB,
                                              IOCTL_AFD_CONNECT,
                                              &ConnectInfo,
                                              sizeof(ConnectInfo),
                                              NULL,
                                              0);
    }

    if (Status == STATUS_PENDING)
    {
        NTDLL::ZwWaitForSingleObject(SockEvent, 0, NULL);
        Status = IOSB.Status;
    }

    NTDLL::ZwClose(SockEvent);
    return NT_SUCCESS(Status);
}

BOOL Socket::Close()
{
    LOG_DEBUG("Disconnect(pNTSocket: 0x%p)\n", this);

    NTSTATUS NTstatus = -1;
    NTstatus = NTDLL::ZwClose((PVOID)m_socket);
    m_socket = NULL;
    return NT_SUCCESS(NTstatus);
}

SSIZE Socket::Read(PVOID buffer, UINT32 bufferSize)
{
    LOG_DEBUG("Read(pNTSocket: 0x%p, buffer: 0x%p, bufferSize: %d)\n", this, buffer, bufferSize);

    SSIZE lpNumberOfBytesRead = 0;

    if (!IsValid())
    {
        LOG_ERROR("Socket not initialized\n");
        return lpNumberOfBytesRead;
    }

    PVOID SockEvent = NULL;
    NTSTATUS Status;
    Status = NTDLL::ZwCreateEvent(&SockEvent,
                                  EVENT_ALL_ACCESS,
                                  NULL,
                                  SynchronizationEvent,
                                  FALSE);

    if (!NT_SUCCESS(Status))
    {
        LOG_ERROR("Failed to create event\n");
        return lpNumberOfBytesRead;
    }
    LOG_DEBUG("Event successfully created for socket read\n");

    AfdSendRecvInfo SendRecvInfo;
    SendRecvInfo.BufferCount = 1;
    SendRecvInfo.AfdFlags = 0;
    SendRecvInfo.TdiFlags = 0x20;
    SendRecvInfo.BufferArray = NULL;

    AfdWsaBuf SendRecvBuffer;
    SendRecvBuffer.Length = bufferSize;
    SendRecvBuffer.Buffer = buffer;
    SendRecvInfo.BufferArray = &SendRecvBuffer;

    IO_STATUS_BLOCK IOSB;
    Status = NTDLL::ZwDeviceIoControlFile((PVOID)m_socket,
                                          SockEvent,
                                          NULL,
                                          NULL,
                                          &IOSB,
                                          IOCTL_AFD_RECV,
                                          &SendRecvInfo,
                                          sizeof(SendRecvInfo),
                                          NULL,
                                          0);

    if (Status == STATUS_PENDING)
    {
        LARGE_INTEGER Timeout;
        Timeout.QuadPart = 5 * 60 * 1000 * -10000LL;

        NTSTATUS waitStatus = NTDLL::ZwWaitForSingleObject(SockEvent, 0, &Timeout);

        if (waitStatus == 0x00000102)
        {
            NTDLL::ZwClose(SockEvent);
            return -1;
        }
        Status = IOSB.Status;
        lpNumberOfBytesRead = IOSB.Information;
    }

    if (NT_SUCCESS(Status))
    {
        lpNumberOfBytesRead = IOSB.Information;
    }

    NTDLL::ZwClose(SockEvent);
    return lpNumberOfBytesRead;
}

UINT32 Socket::Write(PCVOID buffer, UINT32 bufferLength)
{
    LOG_DEBUG("Write(pNTSocket: 0x%p, pData: 0x%p, length: %d)\n", this, buffer, bufferLength);

    UINT32 lpNumberOfBytesAlreadySend = 0;

    if (!IsValid())
    {
        LOG_ERROR("Socket not initialized\n");
        return 0;
    }

    PVOID SockEvent = NULL;
    NTSTATUS Status;
    Status = NTDLL::ZwCreateEvent(&SockEvent,
                                  EVENT_ALL_ACCESS,
                                  NULL,
                                  SynchronizationEvent,
                                  FALSE);

    if (!NT_SUCCESS(Status))
    {
        LOG_ERROR("Failed to create event for socket write: 0x%08X\n", Status);
        return 0;
    }
    LOG_DEBUG("Event successfully created for socket write\n");

    AfdSendRecvInfo SendRecvInfo;
    Memory::Zero(&SendRecvInfo, sizeof(SendRecvInfo));
    SendRecvInfo.BufferCount = 1;
    SendRecvInfo.AfdFlags = 0;
    SendRecvInfo.TdiFlags = 0;

    IO_STATUS_BLOCK IOSB;
    AfdWsaBuf SendRecvBuffer;

    do
    {
        SendRecvBuffer.Buffer = (PVOID)((PCHAR)buffer + lpNumberOfBytesAlreadySend);
        SendRecvBuffer.Length = bufferLength - lpNumberOfBytesAlreadySend;
        SendRecvInfo.BufferArray = &SendRecvBuffer;

        Status = NTDLL::ZwDeviceIoControlFile(m_socket,
                                              SockEvent,
                                              NULL,
                                              NULL,
                                              &IOSB,
                                              IOCTL_AFD_SEND,
                                              &SendRecvInfo,
                                              sizeof(SendRecvInfo),
                                              NULL,
                                              0);

        if (Status == STATUS_PENDING)
        {
            LARGE_INTEGER Timeout;
            Timeout.QuadPart = 1 * 60 * 1000 * -10000LL;
            NTDLL::ZwWaitForSingleObject(SockEvent, 0, &Timeout);
        }

        Status = IOSB.Status;

        if (!NT_SUCCESS(Status))
        {
            NTDLL::ZwClose(SockEvent);
            LOG_ERROR("Failed to write to socket: 0x%08X\n", Status);
            return 0;
        }

        lpNumberOfBytesAlreadySend += (UINT32)IOSB.Information;
    } while (lpNumberOfBytesAlreadySend < bufferLength);

    NTDLL::ZwClose(SockEvent);
    LOG_DEBUG("Successfully wrote %d bytes to socket\n", lpNumberOfBytesAlreadySend);
    return lpNumberOfBytesAlreadySend;
}

Socket::Socket(const IPAddress &ipAddress, UINT16 port) : ip(ipAddress), port(port)
{
    LOG_DEBUG("Create(pNTSocket: 0x%p)\n", this);

    NTSTATUS Status = 0;

    INT32 AddressFamily = SocketAddressHelper::GetAddressFamily(ip);
    INT32 SocketType = SOCK_STREAM;
    INT32 Protocol = IPPROTO_TCP;

    AfdSocketParams EaBuffer;
    Memory::Zero(&(EaBuffer), sizeof(EaBuffer));
    EaBuffer.EaNameLength = 0x0F1E;
    EaBuffer.EaValueLength = 0x001E;

    auto afdOpSource = "AfdOpenPacketXX"_embed;
    Memory::Copy(EaBuffer.AfdOperation, afdOpSource, 16);
    EaBuffer.AfdOperation[15] = '\0';

    EaBuffer.AddressFamily = AddressFamily;
    EaBuffer.SocketType = SocketType;
    EaBuffer.Protocol = Protocol;

    UNICODE_STRING AfdName;
    auto afdNameSource = L"\\Device\\Afd\\Endpoint"_embed;
    AfdName.Buffer = (PWCHAR)(PCWCHAR)afdNameSource;
    AfdName.Length = afdNameSource.Length() * sizeof(WCHAR);
    AfdName.MaximumLength = afdNameSource.Length() * sizeof(WCHAR);

    OBJECT_ATTRIBUTES Object;
    IO_STATUS_BLOCK IOSB;
    InitializeObjectAttributes(&Object,
                               &AfdName,
                               OBJ_CASE_INSENSITIVE | OBJ_INHERIT,
                               0,
                               0);

    Status = NTDLL::ZwCreateFile(&m_socket,
                                 GENERIC_READ | GENERIC_WRITE | SYNCHRONIZE,
                                 &Object,
                                 &IOSB,
                                 NULL,
                                 0,
                                 FILE_SHARE_READ | FILE_SHARE_WRITE,
                                 FILE_OPEN_IF,
                                 0,
                                 &(EaBuffer),
                                 sizeof(EaBuffer));

    if (!NT_SUCCESS(Status))
    {
        LOG_ERROR("Failed to create socket. Status: 0x%08X\n", Status);
    }
    LOG_DEBUG("Socket created successfully: 0x%p\n", m_socket);
}
