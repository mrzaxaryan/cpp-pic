#if defined(PLATFORM_WINDOWS)

#include "primitives.h"
#include "windows_types.h"
#include "socket.h"
#include "logger.h"
#include "ntdll.h"
#include "memory.h"
#include "network.h"

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
#define IPPROTO_TCP 6 /* tcp */
#define ERROR_SUCCESS 0L

#define SOCKET_ERROR (-1)


// Bind data structure
typedef struct _AFD_BindData
{
    UINT32 ShareType;
    SockAddr Address;
} AFD_BindData;

// Connect data structure
typedef struct _AFD_ConnctInfo
{
    SSIZE UseSAN;
    SSIZE Root;
    SSIZE Unknown;
    SockAddr Address;
} AFD_ConnctInfo;

// Data buffer structure
typedef struct _AFD_DataBufferStruct
{
    UINT32 dwDataLength;
    PUINT8 pData;
} AFD_DataBufferStruct;

// Send/Receive information structure
typedef struct _AFD_SendRecvInfo
{
    PVOID BufferArray;
    UINT32 BufferCount;
    UINT32 AfdFlags;
    UINT32 TdiFlags;
} AFD_SendRecvInfo;

// Buffer structure
typedef struct _AFD_Wsbuf
{
    UINT32 len;
    PVOID buf;
} AFD_Wsbuf;

// Structure for socket parameters
typedef struct _SocketParams
{
    INT32 field_0;         // 0-3
    UINT16 field_4;        // 4-5
    UINT16 field_6;        // 6-7
    CHAR AfdOperation[16]; // 8-23
    UINT32 flag;           // 24-27
    INT32 Group;           // 28-31
    INT32 AddressFamily;   // 32-35
    INT32 SocketType;      // 36-39
    INT32 Protocol;        // 40-43
    UINT32 dwStringLength; // 44-47
    WCHAR szString[8];     // 48-63
} SocketParams;

// Function to bind a socket to a specific address and port
BOOL Socket::Bind(SockAddr *SocketAddress, INT32 ShareType)
{
    LOG_DEBUG("Bind(pNTSocket: 0x%p, SocketAddress: 0x%p, ShareType: %d)\n", m_socket, SocketAddress, ShareType);

    // Validate input parameters
    // ASSERT_NOT_NULL(pNTSocketContext, -1);
    // ASSERT_NOT_NULL(SocketAddress, -1);
    // Getting the necessary components from the system
    // PMEMORY_PAL pMemory = GetMemoryPal();
    // PNTDLL pNTDLL = &GetApiPal()->NTDLL;
    // Status variable to hold the result of operations
    // STATUS status = 0;

    // Checking if the socket not initialized
    if (m_socket == 0)
    {
        LOG_ERROR("Socket not initialized\n");
        return FALSE;
    }
    // Variable to hold NT status
    NTSTATUS Status;
    // Handle for the socket event
    PVOID SockEvent = NULL;
    // Create an event for the socket binding operation
    Status = NTDLL::NtCreateEvent(&SockEvent,
                                  EVENT_ALL_ACCESS,
                                  NULL,
                                  SynchronizationEvent,
                                  FALSE);

    // // Validate the status of creating the event
    // if (STATUS_FAILED(status))
    // {
    //     LOG_ERROR("Failed to resolve NtCreateEvent function.");
    //     GetProcessPal()->FastFail(0);
    //     NT_SUCCESS(Status);
    // }
    if (Status != STATUS_SUCCESS)
    {
        LOG_ERROR("Failed to create event for socket binding: 0x%08X\n", Status);
        return NT_SUCCESS(Status);
    }
    LOG_DEBUG("Event successfully created for socket binding\n");
    // Prepare the bind configuration structure
    AFD_BindData BindConfig;
    IO_STATUS_BLOCK IOSB;
    // Zero out the bind configuration structure
    Memory::Zero(&BindConfig, sizeof(BindConfig));
    BindConfig.ShareType = ShareType;
    BindConfig.Address = *SocketAddress;
    UINT8 OutputBlock[40];
    // Zero out the output block
    Memory::Zero(&OutputBlock, sizeof(OutputBlock));
    // Attempt to bind the socket using NtDeviceIoControlFile
    Status = NTDLL::NtDeviceIoControlFile(m_socket,
                                          SockEvent,
                                          NULL,
                                          NULL,
                                          &IOSB,
                                          IOCTL_AFD_BIND,
                                          &BindConfig,
                                          sizeof(BindConfig),
                                          &OutputBlock,
                                          sizeof(OutputBlock));
    // // Validate the status of the bind operation
    // if (STATUS_FAILED(status))
    // {
    //     LOG_ERROR("Failed to resolve NtDeviceIoControlFile function.");
    //     GetProcessPal()->FastFail(0);
    //     NT_SUCCESS(Status);
    // }
    // Check if the operation is pending
    if (Status == STATUS_PENDING)
    {
        NTDLL::NtWaitForSingleObject(SockEvent, 0, NULL);
        // if (STATUS_FAILED(status))
        // {
        //     LOG_ERROR("Failed to resolve NtWaitForSingleObject function.");
        //     GetProcessPal()->FastFail(0);
        //     NT_SUCCESS(Status);
        // }
        Status = IOSB.Status;
    }

    NTDLL::NtClose(SockEvent);
    return NT_SUCCESS(Status);
}

// Function to connect the NT socket to a specified IP and port
BOOL Socket::Open()
{
    LOG_DEBUG("Connect(pNTSocket: 0x%p, ip: %d, port: %d)\n", this, ip, port);
    // Validate input parameter
    // ASSERT_NOT_NULL(pNTSocketContext, FALSE);
    // STATUS status = 0;   // Variable to hold the status of the operation
    NTSTATUS Status = 0; // Variable to hold the NT status of the operation
    // Getting the necessary components from the system
    // PMEMORY_PAL pMemory = GetMemoryPal();
    // PNETWORK_PAL pNetworkPal = GetNetworkPal();
    // PNTDLL pNTDLL = &GetApiPal()->NTDLL;

    // // Check if the socket is initialized
    // if (m_socket == 0)
    // {
    //     LOG_ERROR("Socket not initialized\n");
    //     return FALSE;
    // }
    // Prepare the socket address structure
    SockAddr SocketAddress;
    Memory::Zero(&SocketAddress, sizeof(SocketAddress));
    SocketAddress.sin_family = AF_INET;
    SocketAddress.sin_port = UINT16SwapByteOrder(port);
    SocketAddress.sin_addr = ip;

    SockAddr SockAddr;
    Memory::Zero(&SockAddr, sizeof(SockAddr));
    SockAddr.sin_family = AF_INET;
    // Bind the socket to the specified address and port
    if (Bind(&SockAddr, AFD_SHARE_REUSE) == FALSE)
    {
        LOG_ERROR("Failed to bind socket\n");
        return FALSE;
    }
    LOG_DEBUG("Socket bound successfully\n");
    // Handle for the socket event
    PVOID SockEvent = NULL;
    // Attempt to create an event for the socket connection operation
    Status = NTDLL::NtCreateEvent(&SockEvent,
                                  EVENT_ALL_ACCESS,
                                  NULL,
                                  SynchronizationEvent,
                                  FALSE);
    // Validate the status of creating the event
    // if (STATUS_FAILED(status))
    // {
    //     LOG_ERROR("Failed to resolve NtCreateEvent function.");
    //     GetProcessPal()->FastFail(0);
    //     return FALSE;
    // }
    if (Status != STATUS_SUCCESS)
    {
        LOG_ERROR("Failed to create event\n");
        return FALSE;
    }
    LOG_DEBUG("Event successfully created for socket connection\n");
    // Prepare the connection information structure
    AFD_ConnctInfo ConnectInfo;
    IO_STATUS_BLOCK IOSB;
    ConnectInfo.UseSAN = 0;
    ConnectInfo.Root = 0;
    ConnectInfo.Unknown = 0;
    ConnectInfo.Address = SocketAddress;
    // Attempt to connect the socket using NtDeviceIoControlFile
    Status = NTDLL::NtDeviceIoControlFile((PVOID)m_socket,
                                          SockEvent,
                                          NULL,
                                          NULL,
                                          &IOSB,
                                          IOCTL_AFD_CONNECT,
                                          &ConnectInfo,
                                          sizeof(ConnectInfo),
                                          NULL,
                                          0);
    // // Validate the status of the connect operation
    // if (STATUS_FAILED(status))
    // {
    //     LOG_ERROR("Failed to resolve NtDeviceIoControlFile function.");
    //     GetProcessPal()->FastFail(0);
    //     return FALSE;
    // }
    // Check if the operation is pending
    if (Status == STATUS_PENDING)
    {
        // Wait for the connection to complete
        NTDLL::NtWaitForSingleObject(SockEvent, 0, NULL);
        // if (STATUS_FAILED(status))
        // {
        //     LOG_ERROR("Failed to resolve NtWaitForSingleObject function.");
        //     GetProcessPal()->FastFail(0);
        //     return FALSE;
        // }

        Status = IOSB.Status;
    }
    // Socket_Close the socket event handle
    NTDLL::NtClose(SockEvent);
    // Check the final status of the connection
    return NT_SUCCESS(Status);
}

// Function to disconnect the NT socket
BOOL Socket::Close()
{
    LOG_DEBUG("Disconnect(pNTSocket: 0x%p)\n", this);
    // Validate input parameter
    // ASSERT_NOT_NULL(pNTSocketContext, -1);
    // STATUS status = 0; // Variable to hold the status of the operation
    // // Getting the necessary components from the system
    // PNTDLL pNTDLL = &GetApiPal()->NTDLL;
    // Check if the socket is initialized
    // Variable to hold the NT status
    NTSTATUS NTstatus = 0;
    // Attempt to close the socket using NtClose
    NTstatus = NTDLL::NtClose((PVOID)m_socket);
    // Validate the status of closing the socket
    // if (STATUS_FAILED(status))
    // {
    //     LOG_ERROR("Failed to resolve NtClose function.");
    //     return FALSE;
    // }
    // Reset the socket handle to NULL
    m_socket = NULL;
    return NT_SUCCESS(NTstatus);
}

// Function to read data from the NT socket
SSIZE Socket::Read(PVOID buffer, UINT32 bufferSize)
{

    LOG_DEBUG("Read(pNTSocket: 0x%p, buffer: 0x%p, bufferSize: %d)\n", this, buffer, bufferSize);

    // // Validate input parameters
    // ASSERT_NOT_NULL(pNTSocketContext, 0);
    // ASSERT_NOT_NULL(buffer, 0);
    // Variable to hold the status of the operation
    // STATUS status = 0;
    // Getting the necessary components from the system
    // PNTDLL pNTDLL = &GetApiPal()->NTDLL;
    SSIZE lpNumberOfBytesRead = 0; // Variable to hold the number of bytes read
    // Check if the socket is initialized
    if (m_socket == 0)
    {
        LOG_ERROR("Socket not initialized\n");
        return lpNumberOfBytesRead;
    }
    // Handle for the socket event
    PVOID SockEvent = NULL;
    NTSTATUS Status; // Variable to hold the NT status
    // Attempt to create an event for the socket read operation
    Status = NTDLL::NtCreateEvent(&SockEvent,
                                  EVENT_ALL_ACCESS,
                                  NULL,
                                  SynchronizationEvent,
                                  FALSE);
    // // Validate the status of creating the event
    // if (STATUS_FAILED(status))
    // {
    //     LOG_ERROR("Failed to resolve NtCreateEvent function.");
    //     GetProcessPal()->FastFail(0);
    //     return -1;
    // }
    // Check if the event creation was successful
    if (Status != STATUS_SUCCESS)
    {
        LOG_ERROR("Failed to create event\n");
        return lpNumberOfBytesRead; // changed from original ntsocket
    }
    LOG_DEBUG("Event successfully created for socket read\n");
    // Prepare the send/receive information structure
    AFD_SendRecvInfo SendRecvInfo;
    SendRecvInfo.BufferCount = 1;
    SendRecvInfo.AfdFlags = 0;
    SendRecvInfo.TdiFlags = 0x20;
    SendRecvInfo.BufferArray = NULL;

    AFD_Wsbuf SendRecvBuffer;
    SendRecvBuffer.len = bufferSize;
    SendRecvBuffer.buf = buffer;
    SendRecvInfo.BufferArray = &SendRecvBuffer;
    IO_STATUS_BLOCK IOSB;
    // Attempt to read data from the socket using NtDeviceIoControlFile
    Status = NTDLL::NtDeviceIoControlFile((PVOID)m_socket,
                                          SockEvent,
                                          NULL,
                                          NULL,
                                          &IOSB,
                                          IOCTL_AFD_RECV,
                                          &SendRecvInfo,
                                          sizeof(SendRecvInfo),
                                          NULL,
                                          0);
    // // Validate the status of the read operation
    // if (STATUS_FAILED(status))
    // {
    //     LOG_ERROR("Failed to resolve NtDeviceIoControlFile function.");
    //     GetProcessPal()->FastFail(0);
    //     return -1;
    // }
    // Check if the operation is pending
    if (Status == STATUS_PENDING)
    {
        // set timeout to 5 minutes
        LARGE_INTEGER Timeout;
        Timeout.QuadPart = 5 * 60 * 1000 * -10000LL;

        // Wait for the socket read operation to complete
        NTSTATUS waitStatus = NTDLL::NtWaitForSingleObject(SockEvent, 0, &Timeout);
        // if (STATUS_FAILED(status))
        // {
        //     LOG_ERROR("Failed to resolve NtWaitForSingleObject function.");
        //     GetProcessPal()->FastFail(0);
        //     return -1;
        // }
        // Check for timeout
        if (waitStatus == 0x00000102)
        {
            NTDLL::NtClose(SockEvent);
            return -1;
        }
        Status = IOSB.Status;
        lpNumberOfBytesRead = IOSB.Information;
    }
    // Check if the read operation was successful
    if (Status == STATUS_SUCCESS)
    {
        lpNumberOfBytesRead = IOSB.Information;
    }
    // Socket_Close the socket event handle
    NTDLL::NtClose(SockEvent);
    // Leave the critical section
    // // Validate the status of leaving the critical section
    // if (STATUS_FAILED(status))
    // Return the number of bytes read
    return lpNumberOfBytesRead;
}

// Function to write data to the NT socket
UINT32 Socket::Write(PCVOID buffer, UINT32 bufferLength)
{

    LOG_DEBUG("Write(pNTSocket: 0x%p, pData: 0x%p, length: %d)\n", this, buffer, bufferLength);

    // // Validate input parameters
    // ASSERT_NOT_NULL(pNTSocketContext, FALSE);
    // ASSERT_NOT_NULL(pData, FALSE);
    // ASSERT_NOT_NULL(bytesWritten, FALSE);
    // Variable to hold the status of the operation
    // STATUS status = 0;
    // // Getting the necessary components from the system
    // PMEMORY_PAL pMemory = GetMemoryPal();
    // PNTDLL pNTDLL = &GetApiPal()->NTDLL;
    UINT32 lpNumberOfBytesAlreadySend = 0; // Variable to hold the number of bytes already sent

    // Check if the socket is initialized
    if (m_socket == 0)
    {
        LOG_ERROR("Socket not initialized\n");
        return FALSE;
    }
    // Handle for the socket event
    PVOID SockEvent = NULL;
    NTSTATUS Status; // Variable to hold the NT status
    // Attempt to create an event for the socket write operation
    Status = NTDLL::NtCreateEvent(&SockEvent,
                                  EVENT_ALL_ACCESS,
                                  NULL,
                                  SynchronizationEvent,
                                  FALSE);
    // // Validate the status of creating the event
    // if (STATUS_FAILED(status))
    // {
    //     LOG_ERROR("Failed to resolve NtCreateEvent function.");
    //     GetProcessPal()->FastFail(0);
    //     return FALSE;
    // }
    // Check if the event creation was successful
    if (Status != STATUS_SUCCESS)
    {
        LOG_ERROR("Failed to create event for socket write: 0x%08X\n", Status);
        return FALSE;
    }
    LOG_DEBUG("Event successfully created for socket write\n");
    // Prepare the send/receive information structure
    AFD_SendRecvInfo SendRecvInfo;
    Memory::Zero(&SendRecvInfo, sizeof(SendRecvInfo));
    SendRecvInfo.BufferCount = 1;
    SendRecvInfo.AfdFlags = 0;
    SendRecvInfo.TdiFlags = 0;

    IO_STATUS_BLOCK IOSB;

    AFD_Wsbuf SendRecvBuffer;
    // Loop to send data until all bytes are sent
    do
    {
        SendRecvBuffer.buf = (PVOID)((PCHAR)buffer + lpNumberOfBytesAlreadySend);
        SendRecvBuffer.len = bufferLength - lpNumberOfBytesAlreadySend;
        SendRecvInfo.BufferArray = &SendRecvBuffer;
        // Attempt to write data to the socket using NtDeviceIoControlFile
        Status = NTDLL::NtDeviceIoControlFile(m_socket,
                                              SockEvent,
                                              NULL,
                                              NULL,
                                              &IOSB,
                                              IOCTL_AFD_SEND,
                                              &SendRecvInfo,
                                              sizeof(SendRecvInfo),
                                              NULL,
                                              0);
        // // Validate the status of the write operation
        // if (STATUS_FAILED(status))
        // {
        //     LOG_ERROR("Failed to resolve NtDeviceIoControlFile function.");
        //     NtClose(NULL, SockEvent);
        //     GetProcessPal()->FastFail(0);
        //     return FALSE;
        // }
        // Check if the operation is pending
        if (Status == STATUS_PENDING)
        {
            LARGE_INTEGER Timeout;
            Timeout.QuadPart = 1 * 60 * 1000 * -10000LL; // one minute
                                                         // Wait for the socket write operation to complete
            NTDLL::NtWaitForSingleObject(SockEvent, 0, &Timeout);
            // // Validate the status of the wait operation
            // if (STATUS_FAILED(status))
            // {
            //     LOG_ERROR("Failed to resolve NtWaitForSingleObject function.");
            //     GetProcessPal()->FastFail(0);
            //     return FALSE;
            // }
        }
        // Update the status and number of bytes already sent
        Status = IOSB.Status;
        lpNumberOfBytesAlreadySend = IOSB.Information;
        // Check if the write operation was successful
        if (Status != STATUS_SUCCESS)
        {
            NTDLL::NtClose(SockEvent);
            LOG_ERROR("Failed to write to socket: 0x%08X\n", Status);
            return FALSE;
        }
    } while (lpNumberOfBytesAlreadySend < bufferLength);

    // Socket_Close the socket event handle
    NTDLL::NtClose(SockEvent);
    // Set the number of bytes written
    // *bytesWritten = lpNumberOfBytesAlreadySend;
    LOG_DEBUG("Successfully wrote %d bytes to socket\n", lpNumberOfBytesAlreadySend);
    return lpNumberOfBytesAlreadySend;
}

// Function to create a new NT socket
Socket::Socket(UINT32 ip, UINT16 port) : ip(ip), port(port)
{
    LOG_DEBUG("Create(pNTSocket: 0x%p)\n", this);
    // Validate input parameter
    // ASSERT_NOT_NULL(pNTSocketContext, FALSE);

    // STATUS status = 0;   // Variable to hold the status of the operation
    NTSTATUS Status = 0; // Variable to hold the NT status of the operation

    // Define socket parameters
    INT32 AddressFamily = AF_INET;
    INT32 SocketType = SOCK_STREAM;
    INT32 Protocol = IPPROTO_TCP;

    SocketParams EaBuffer;
    Memory::Zero(&(EaBuffer), sizeof(EaBuffer));
    // 4-5: field_4
    EaBuffer.field_4 = 0x0F1E; // Assuming little endian

    // 6-7: field_6
    EaBuffer.field_6 = 0x001E; // 30 in decimal

    // 8-23: AfdOperation ("AfdOpenPacketXX\0")
    auto afdOpSource = "AfdOpenPacketXX"_embed;

    // Ensure null-termination and safe copy
    Memory::Copy(EaBuffer.AfdOperation, afdOpSource, 16);
    EaBuffer.AfdOperation[15] = '\0'; // Explicitly null-terminate

    // 32-35: AddressFamily
    EaBuffer.AddressFamily = AddressFamily;

    // 36-39: SocketType
    EaBuffer.SocketType = SocketType;

    // 40-43: Protocol
    EaBuffer.Protocol = Protocol;

    UNICODE_STRING AfdName;
    auto afdNameSource = L"\\Device\\Afd\\Endpoint"_embed;
    AfdName.Buffer = (PWCHAR)(PCWCHAR)afdNameSource;
    AfdName.Length = afdNameSource.Length * sizeof(WCHAR);
    AfdName.MaximumLength = afdNameSource.Length + sizeof(WCHAR);
    OBJECT_ATTRIBUTES Object;
    IO_STATUS_BLOCK IOSB;
    // Initialize the object attributes for the socket
    InitializeObjectAttributes(&Object,
                               &AfdName,
                               OBJ_CASE_INSENSITIVE | OBJ_INHERIT,
                               0,
                               0);
    // Create the socket using NtCreateFile
    Status = NTDLL::NtCreateFile(&m_socket,
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

    // // Validate the status of the socket creation
    // if (STATUS_FAILED(status))
    // {
    //     LOG_ERROR("Failed to resolve NtCreateFile function.");
    //     GetProcessPal()->FastFail(0);
    //     return FALSE;
    // }

    if (Status != STATUS_SUCCESS)
    {
        LOG_ERROR("Failed to create socket. Status: 0x%08X\n", Status);
    }
    LOG_DEBUG("Socket created successfully: 0x%p\n", m_socket);
}

#endif // PLATFORM_WINDOWS