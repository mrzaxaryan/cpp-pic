#if defined(PLATFORM_WINDOWS)

#include "socket.h"
#include "logger.h"
#include "string.h"
#include "network.h"

// =============================================================================
// Socket Class Implementation - AFD-based Windows Networking
// =============================================================================

// -----------------------------------------------------------------------------
// Constructor: Initialize socket with target IP and port
// -----------------------------------------------------------------------------
Socket::Socket(UINT32 ip, UINT16 port) : m_socket(NULL), m_ip(ip), m_port(port)
{
	LOG_DEBUG("Socket::Socket(ip: 0x%08X, port: %d)", ip, port);

	// Prepare socket creation parameters (Extended Attributes)
	SocketParams EaBuffer;
	Memory::Zero(&EaBuffer, sizeof(EaBuffer));

	// Magic values required by AFD
	EaBuffer.field_4 = 0x0F1E;
	EaBuffer.field_6 = 0x001E; // 30 decimal

	// AFD operation identifier
	auto afdOperation = "AfdOpenPacketXX"_embed;
	Memory::Copy(EaBuffer.AfdOperation, (PCVOID)(PCCHAR)afdOperation, 16);
	EaBuffer.AfdOperation[15] = '\0';

	// Socket parameters
	EaBuffer.AddressFamily = AF_INET;
	EaBuffer.SocketType = SOCK_STREAM;
	EaBuffer.Protocol = IPPROTO_TCP;

	// Prepare AFD device path
	UNICODE_STRING AfdName;
	auto afdNameSource = L"\\Device\\Afd\\Endpoint"_embed;
	AfdName.Buffer = (PWCHAR)(PCWCHAR)afdNameSource;
	AfdName.Length = afdNameSource.Length * sizeof(WCHAR);
	AfdName.MaximumLength = (afdNameSource.Length + 1) * sizeof(WCHAR);

	// Initialize object attributes
	OBJECT_ATTRIBUTES ObjectAttr;
	InitializeObjectAttributes(&ObjectAttr, &AfdName, OBJ_CASE_INSENSITIVE | OBJ_INHERIT, NULL, NULL);

	// Create the AFD socket handle
	IO_STATUS_BLOCK IOSB;
	NTSTATUS Status = NTDLL::NtCreateFile(&m_socket,
	                                      GENERIC_READ | GENERIC_WRITE | SYNCHRONIZE,
	                                      &ObjectAttr,
	                                      &IOSB,
	                                      NULL,
	                                      0,
	                                      FILE_SHARE_READ | FILE_SHARE_WRITE,
	                                      FILE_OPEN_IF,
	                                      0,
	                                      &EaBuffer,
	                                      sizeof(EaBuffer));

	if (!NT_SUCCESS(Status))
	{
		LOG_ERROR("Failed to create AFD socket. Status: 0x%08X", Status);
		m_socket = NULL;
		return;
	}

	LOG_DEBUG("AFD socket created successfully: 0x%p", m_socket);
}

// -----------------------------------------------------------------------------
// Bind: Bind socket to a local address
// -----------------------------------------------------------------------------
BOOL Socket::Bind(PSockAddr SocketAddress, UINT32 ShareType)
{
	LOG_DEBUG("Socket::Bind(handle: 0x%p, ShareType: %d)", m_socket, ShareType);

	if (!IsValid())
	{
		LOG_ERROR("Socket not initialized");
		return FALSE;
	}

	// Create event for synchronous operation
	PVOID SockEvent = NULL;
	NTSTATUS Status = NTDLL::NtCreateEvent(&SockEvent, EVENT_ALL_ACCESS, NULL, SynchronizationEvent, FALSE);

	if (!NT_SUCCESS(Status))
	{
		LOG_ERROR("Failed to create bind event. Status: 0x%08X", Status);
		return FALSE;
	}

	// Prepare bind configuration
	AFD_BindData BindConfig;
	Memory::Zero(&BindConfig, sizeof(BindConfig));
	BindConfig.ShareType = ShareType;
	BindConfig.Address = *SocketAddress;

	// Output buffer for bind operation
	UINT8 OutputBlock[40];
	Memory::Zero(&OutputBlock, sizeof(OutputBlock));

	// Perform bind operation
	IO_STATUS_BLOCK IOSB;
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

	// Wait for completion if pending
	if (Status == STATUS_PENDING)
	{
		NTDLL::NtWaitForSingleObject(SockEvent, FALSE, NULL);
		Status = IOSB.DUMMYUNIONNAME.Status;
	}

	NTDLL::NtClose(SockEvent);

	if (!NT_SUCCESS(Status))
	{
		LOG_ERROR("Bind failed. Status: 0x%08X", Status);
		return FALSE;
	}

	LOG_DEBUG("Socket bound successfully");
	return TRUE;
}

// -----------------------------------------------------------------------------
// Open: Connect to remote server
// -----------------------------------------------------------------------------
BOOL Socket::Open()
{
	LOG_DEBUG("Socket::Open(ip: 0x%08X, port: %d)", m_ip, m_port);

	if (!IsValid())
	{
		LOG_ERROR("Socket not initialized");
		return FALSE;
	}

	// Prepare local bind address (INADDR_ANY)
	SockAddr LocalAddr;
	Memory::Zero(&LocalAddr, sizeof(LocalAddr));
	LocalAddr.sin_family = AF_INET;

	// Bind to local address first
	if (!Bind(&LocalAddr, AFD_SHARE_REUSE))
	{
		LOG_ERROR("Failed to bind socket locally");
		return FALSE;
	}

	// Prepare remote server address
	SockAddr ServerAddr;
	Memory::Zero(&ServerAddr, sizeof(ServerAddr));
	ServerAddr.sin_family = AF_INET;
	ServerAddr.sin_port = UINT16SwapByteOrder(m_port);
	ServerAddr.sin_addr = m_ip;

	// Create event for connect operation
	PVOID SockEvent = NULL;
	NTSTATUS Status = NTDLL::NtCreateEvent(&SockEvent, EVENT_ALL_ACCESS, NULL, SynchronizationEvent, FALSE);

	if (!NT_SUCCESS(Status))
	{
		LOG_ERROR("Failed to create connect event. Status: 0x%08X", Status);
		return FALSE;
	}

	// Prepare connect information
	AFD_ConnectInfo ConnectInfo;
	Memory::Zero(&ConnectInfo, sizeof(ConnectInfo));
	ConnectInfo.UseSAN = 0;
	ConnectInfo.Root = 0;
	ConnectInfo.Unknown = 0;
	ConnectInfo.Address = ServerAddr;

	// Perform connect operation
	IO_STATUS_BLOCK IOSB;
	Status = NTDLL::NtDeviceIoControlFile(m_socket,
	                                      SockEvent,
	                                      NULL,
	                                      NULL,
	                                      &IOSB,
	                                      IOCTL_AFD_CONNECT,
	                                      &ConnectInfo,
	                                      sizeof(ConnectInfo),
	                                      NULL,
	                                      0);

	// Wait for completion if pending
	if (Status == STATUS_PENDING)
	{
		NTDLL::NtWaitForSingleObject(SockEvent, FALSE, NULL);
		Status = IOSB.DUMMYUNIONNAME.Status;
	}

	NTDLL::NtClose(SockEvent);

	if (!NT_SUCCESS(Status))
	{
		LOG_ERROR("Connect failed. Status: 0x%08X", Status);
		return FALSE;
	}

	LOG_INFO("Connected successfully to server");
	return TRUE;
}

// -----------------------------------------------------------------------------
// Close: Disconnect and close socket
// -----------------------------------------------------------------------------
BOOL Socket::Close()
{
	LOG_DEBUG("Socket::Close(handle: 0x%p)", m_socket);

	if (!IsValid())
	{
		LOG_WARNING("Attempted to close invalid socket");
		return FALSE;
	}

	NTSTATUS Status = NTDLL::NtClose(m_socket);
	m_socket = NULL;

	if (!NT_SUCCESS(Status))
	{
		LOG_ERROR("Failed to close socket. Status: 0x%08X", Status);
		return FALSE;
	}

	LOG_DEBUG("Socket closed successfully");
	return TRUE;
}

// -----------------------------------------------------------------------------
// Read: Receive data from socket
// -----------------------------------------------------------------------------
SSIZE Socket::Read(PVOID buffer, UINT32 bufferSize)
{
	LOG_DEBUG("Socket::Read(handle: 0x%p, bufferSize: %d)", m_socket, bufferSize);

	if (!IsValid())
	{
		LOG_ERROR("Socket not initialized");
		return 0;
	}

	// Create event for receive operation
	PVOID SockEvent = NULL;
	NTSTATUS Status = NTDLL::NtCreateEvent(&SockEvent, EVENT_ALL_ACCESS, NULL, SynchronizationEvent, FALSE);

	if (!NT_SUCCESS(Status))
	{
		LOG_ERROR("Failed to create read event. Status: 0x%08X", Status);
		return 0;
	}

	// Prepare receive buffer
	AFD_Wsbuf RecvBuffer;
	RecvBuffer.len = bufferSize;
	RecvBuffer.buf = buffer;

	// Prepare send/receive information
	AFD_SendRecvInfo RecvInfo;
	Memory::Zero(&RecvInfo, sizeof(RecvInfo));
	RecvInfo.BufferArray = &RecvBuffer;
	RecvInfo.BufferCount = 1;
	RecvInfo.AfdFlags = 0;
	RecvInfo.TdiFlags = 0x20; // TDI_RECEIVE_NORMAL

	// Perform receive operation
	IO_STATUS_BLOCK IOSB;
	Status = NTDLL::NtDeviceIoControlFile(m_socket,
	                                      SockEvent,
	                                      NULL,
	                                      NULL,
	                                      &IOSB,
	                                      IOCTL_AFD_RECV,
	                                      &RecvInfo,
	                                      sizeof(RecvInfo),
	                                      NULL,
	                                      0);

	// Wait for completion with 5 minute timeout
	if (Status == STATUS_PENDING)
	{
		LARGE_INTEGER Timeout;
		Timeout.QuadPart = -3000000000LL; // 5 minutes in 100ns units (negative = relative)

		Status = NTDLL::NtWaitForSingleObject(SockEvent, FALSE, &Timeout);

		// Check for timeout (STATUS_TIMEOUT = 0x00000102)
		if (Status == 0x00000102)
		{
			LOG_ERROR("Read operation timed out");
			NTDLL::NtClose(SockEvent);
			return -1;
		}

		Status = IOSB.DUMMYUNIONNAME.Status;
	}

	NTDLL::NtClose(SockEvent);

	if (!NT_SUCCESS(Status))
	{
		LOG_ERROR("Read failed. Status: 0x%08X", Status);
		return 0;
	}

	SSIZE bytesRead = (SSIZE)IOSB.Information;
	LOG_DEBUG("Read %d bytes from socket", bytesRead);
	return bytesRead;
}

// -----------------------------------------------------------------------------
// Write: Send data to socket
// -----------------------------------------------------------------------------
UINT32 Socket::Write(PCVOID buffer, UINT32 bufferLength)
{
	LOG_DEBUG("Socket::Write(handle: 0x%p, length: %d)", m_socket, bufferLength);

	if (!IsValid())
	{
		LOG_ERROR("Socket not initialized");
		return 0;
	}

	// Create event for send operation
	PVOID SockEvent = NULL;
	NTSTATUS Status = NTDLL::NtCreateEvent(&SockEvent, EVENT_ALL_ACCESS, NULL, SynchronizationEvent, FALSE);

	if (!NT_SUCCESS(Status))
	{
		LOG_ERROR("Failed to create write event. Status: 0x%08X", Status);
		return 0;
	}

	UINT32 totalBytesSent = 0;
	IO_STATUS_BLOCK IOSB;

	// Send data in chunks until all bytes are transmitted
	while (totalBytesSent < bufferLength)
	{
		// Prepare send buffer
		AFD_Wsbuf SendBuffer;
		SendBuffer.buf = (PVOID)((PUINT8)buffer + totalBytesSent);
		SendBuffer.len = bufferLength - totalBytesSent;

		// Prepare send information
		AFD_SendRecvInfo SendInfo;
		Memory::Zero(&SendInfo, sizeof(SendInfo));
		SendInfo.BufferArray = &SendBuffer;
		SendInfo.BufferCount = 1;
		SendInfo.AfdFlags = 0;
		SendInfo.TdiFlags = 0;

		// Perform send operation
		Status = NTDLL::NtDeviceIoControlFile(m_socket,
		                                      SockEvent,
		                                      NULL,
		                                      NULL,
		                                      &IOSB,
		                                      IOCTL_AFD_SEND,
		                                      &SendInfo,
		                                      sizeof(SendInfo),
		                                      NULL,
		                                      0);

		// Wait for completion with 1 minute timeout
		if (Status == STATUS_PENDING)
		{
			LARGE_INTEGER Timeout;
			Timeout.QuadPart = -600000000LL; // 1 minute in 100ns units

			NTDLL::NtWaitForSingleObject(SockEvent, FALSE, &Timeout);
		}

		Status = IOSB.DUMMYUNIONNAME.Status;

		if (!NT_SUCCESS(Status))
		{
			LOG_ERROR("Write failed. Status: 0x%08X", Status);
			NTDLL::NtClose(SockEvent);
			return totalBytesSent;
		}

		totalBytesSent += (UINT32)IOSB.Information;
	}

	NTDLL::NtClose(SockEvent);
	LOG_DEBUG("Successfully wrote %d bytes to socket", totalBytesSent);
	return totalBytesSent;
}

#endif // PLATFORM_WINDOWS
