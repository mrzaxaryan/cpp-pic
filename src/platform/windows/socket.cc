#include "primitives.h"
#include "windows_types.h"
#include "socket.h"
#include "logger.h"
#include "ntdll.h"
#include "memory.h"

#define IOCTL_AFD_BIND       ((0x00000012) << 12 | (0  << 2) | 3)
#define IOCTL_AFD_CONNECT    ((0x00000012) << 12 | (1  << 2) | 3)
#define IOCTL_AFD_SEND       ((0x00000012) << 12 | (7  << 2) | 3)
#define IOCTL_AFD_RECV       ((0x00000012) << 12 | (5  << 2) | 3)
#define IOCTL_AFD_DISCONNECT ((0x00000012) << 12 | (10 << 2) | 3)

#define AFD_SHARE_REUSE         0x1L
#define AFD_DISCONNECT_SEND     0x01L
#define AFD_DISCONNECT_RECV     0x02L
#define AFD_DISCONNECT_ABORT    0x04L
#define AFD_DISCONNECT_DATAGRAM 0x08L

#define OBJ_INHERIT    0x00000002L
#define STATUS_PENDING ((UINT32)0x00000103L)
#define STATUS_TIMEOUT ((UINT32)0x00000102L)
#define IPPROTO_TCP    6

typedef struct AfdBindData
{
	UINT32  ShareType;
	SockAddr Address;
} AfdBindData;

typedef struct AfdBindData6
{
	UINT32   ShareType;
	SockAddr6 Address;
} AfdBindData6;

typedef struct AfdConnectInfo
{
	SSIZE    UseSAN;
	SSIZE    Root;
	SSIZE    Unknown;
	SockAddr Address;
} AfdConnectInfo;

typedef struct AfdConnectInfo6
{
	SSIZE     UseSAN;
	SSIZE     Root;
	SSIZE     Unknown;
	SockAddr6 Address;
} AfdConnectInfo6;

typedef struct AfdWsaBuf
{
	UINT32 Length;
	PVOID  Buffer;
} AfdWsaBuf;

typedef struct AfdSendRecvInfo
{
	PVOID  BufferArray;
	UINT32 BufferCount;
	UINT32 AfdFlags;
	UINT32 TdiFlags;
} AfdSendRecvInfo;

typedef struct AfdSocketParams
{
	INT32  Reserved;
	UINT16 EaNameLength;
	UINT16 EaValueLength;
	CHAR   AfdOperation[16];
	UINT32 Flags;
	INT32  GroupId;
	INT32  AddressFamily;
	INT32  SocketType;
	INT32  Protocol;
	UINT32 ProviderInfoLength;
	WCHAR  ProviderInfo[8];
} AfdSocketParams;

// Wait for a pending AFD IOCTL to complete via the signaled event.
// On completion, reads IOSB.Status back into Status.
// Returns STATUS_TIMEOUT if timed out (Status is NOT updated in that case).
static NTSTATUS AfdWait(PVOID SockEvent, IO_STATUS_BLOCK &IOSB, NTSTATUS &Status, LARGE_INTEGER *Timeout)
{
	NTSTATUS waitStatus = NTDLL::ZwWaitForSingleObject(SockEvent, 0, Timeout);
	if (waitStatus != (NTSTATUS)STATUS_TIMEOUT)
		Status = IOSB.Status;
	return waitStatus;
}

Result<void, NetworkError> Socket::Bind(SockAddr &SocketAddress, INT32 ShareType)
{
	LOG_DEBUG("Bind(handle: 0x%p, family: %d, ShareType: %d)\n", m_socket, SocketAddress.sin_family, ShareType);

	PVOID SockEvent = nullptr;
	NTSTATUS Status = NTDLL::ZwCreateEvent(&SockEvent,
	                                       EVENT_ALL_ACCESS,
	                                       nullptr,
	                                       SynchronizationEvent,
	                                       false);
	if (!NT_SUCCESS(Status))
	{
		NetworkError err;
		err.Push((UINT32)Status);
		err.Push(NetworkError::Socket_BindFailed_EventCreate);
		return Result<void, NetworkError>::Err(err);
	}

	IO_STATUS_BLOCK IOSB;
	Memory::Zero(&IOSB, sizeof(IOSB));

	UINT8 OutputBlock[40];
	Memory::Zero(&OutputBlock, sizeof(OutputBlock));

	if (SocketAddress.sin_family == AF_INET6)
	{
		AfdBindData6 BindConfig;
		Memory::Zero(&BindConfig, sizeof(BindConfig));
		BindConfig.ShareType = ShareType;
		BindConfig.Address   = (SockAddr6 &)SocketAddress;

		Status = NTDLL::ZwDeviceIoControlFile(m_socket, SockEvent, nullptr, nullptr, &IOSB,
		                                      IOCTL_AFD_BIND,
		                                      &BindConfig, sizeof(BindConfig),
		                                      &OutputBlock, sizeof(OutputBlock));
	}
	else
	{
		AfdBindData BindConfig;
		Memory::Zero(&BindConfig, sizeof(BindConfig));
		BindConfig.ShareType = ShareType;
		BindConfig.Address   = SocketAddress;

		Status = NTDLL::ZwDeviceIoControlFile(m_socket, SockEvent, nullptr, nullptr, &IOSB,
		                                      IOCTL_AFD_BIND,
		                                      &BindConfig, sizeof(BindConfig),
		                                      &OutputBlock, sizeof(OutputBlock));
	}

	if (Status == (NTSTATUS)STATUS_PENDING)
		(void)AfdWait(SockEvent, IOSB, Status, nullptr);

	(void)NTDLL::ZwClose(SockEvent);

	if (!NT_SUCCESS(Status))
	{
		NetworkError err;
		err.Push((UINT32)Status);
		err.Push(NetworkError::Socket_BindFailed_Bind);
		return Result<void, NetworkError>::Err(err);
	}

	return Result<void, NetworkError>::Ok();
}

Result<void, NetworkError> Socket::Open()
{
	LOG_DEBUG("Open(handle: 0x%p, port: %d)\n", this, port);

	if (!IsValid())
	{
		NetworkError err;
		err.Push(NetworkError::Socket_OpenFailed_HandleInvalid);
		return Result<void, NetworkError>::Err(err);
	}

	// AFD requires an explicit bind to a wildcard local address before connect
	union
	{
		SockAddr  addr4;
		SockAddr6 addr6;
	} bindBuffer;

	SocketAddressHelper::PrepareBindAddress(ip.IsIPv6(), 0, &bindBuffer, sizeof(bindBuffer));

	auto bindResult = Bind((SockAddr &)bindBuffer, AFD_SHARE_REUSE);
	if (!bindResult)
		return bindResult; // propagates BindFailed_* with its NTSTATUS

	PVOID SockEvent = nullptr;
	NTSTATUS Status = NTDLL::ZwCreateEvent(&SockEvent,
	                                       EVENT_ALL_ACCESS,
	                                       nullptr,
	                                       SynchronizationEvent,
	                                       false);
	if (!NT_SUCCESS(Status))
	{
		NetworkError err;
		err.Push((UINT32)Status);
		err.Push(NetworkError::Socket_OpenFailed_EventCreate);
		return Result<void, NetworkError>::Err(err);
	}

	IO_STATUS_BLOCK IOSB;
	Memory::Zero(&IOSB, sizeof(IOSB));

	union
	{
		SockAddr  addr4;
		SockAddr6 addr6;
	} addrBuffer;

	SocketAddressHelper::PrepareAddress(ip, port, &addrBuffer, sizeof(addrBuffer));

	if (ip.IsIPv6())
	{
		AfdConnectInfo6 ConnectInfo;
		Memory::Zero(&ConnectInfo, sizeof(ConnectInfo));
		ConnectInfo.Address = addrBuffer.addr6;

		Status = NTDLL::ZwDeviceIoControlFile(m_socket, SockEvent, nullptr, nullptr, &IOSB,
		                                      IOCTL_AFD_CONNECT,
		                                      &ConnectInfo, sizeof(ConnectInfo),
		                                      nullptr, 0);
	}
	else
	{
		AfdConnectInfo ConnectInfo;
		Memory::Zero(&ConnectInfo, sizeof(ConnectInfo));
		ConnectInfo.Address = addrBuffer.addr4;

		Status = NTDLL::ZwDeviceIoControlFile(m_socket, SockEvent, nullptr, nullptr, &IOSB,
		                                      IOCTL_AFD_CONNECT,
		                                      &ConnectInfo, sizeof(ConnectInfo),
		                                      nullptr, 0);
	}

	if (Status == (NTSTATUS)STATUS_PENDING)
		(void)AfdWait(SockEvent, IOSB, Status, nullptr);

	(void)NTDLL::ZwClose(SockEvent);

	if (!NT_SUCCESS(Status))
	{
		NetworkError err;
		err.Push((UINT32)Status);
		err.Push(NetworkError::Socket_OpenFailed_Connect);
		return Result<void, NetworkError>::Err(err);
	}

	LOG_DEBUG("Open: connected successfully\n");
	return Result<void, NetworkError>::Ok();
}

Result<void, NetworkError> Socket::Close()
{
	LOG_DEBUG("Close(handle: 0x%p)\n", this);

	NTSTATUS Status = NTDLL::ZwClose(m_socket);
	m_socket = nullptr;

	if (!NT_SUCCESS(Status))
	{
		NetworkError err;
		err.Push((UINT32)Status);
		err.Push(NetworkError::Socket_CloseFailed_Close);
		return Result<void, NetworkError>::Err(err);
	}

	return Result<void, NetworkError>::Ok();
}

Result<SSIZE, NetworkError> Socket::Read(PVOID buffer, UINT32 bufferSize)
{
	LOG_DEBUG("Read(handle: 0x%p, bufferSize: %d)\n", this, bufferSize);

	if (!IsValid())
	{
		NetworkError err;
		err.Push(NetworkError::Socket_ReadFailed_HandleInvalid);
		return Result<SSIZE, NetworkError>::Err(err);
	}

	PVOID SockEvent = nullptr;
	NTSTATUS Status = NTDLL::ZwCreateEvent(&SockEvent,
	                                       EVENT_ALL_ACCESS,
	                                       nullptr,
	                                       SynchronizationEvent,
	                                       false);
	if (!NT_SUCCESS(Status))
	{
		NetworkError err;
		err.Push((UINT32)Status);
		err.Push(NetworkError::Socket_ReadFailed_EventCreate);
		return Result<SSIZE, NetworkError>::Err(err);
	}

	AfdWsaBuf RecvBuffer;
	RecvBuffer.Length = bufferSize;
	RecvBuffer.Buffer = buffer;

	AfdSendRecvInfo RecvInfo;
	Memory::Zero(&RecvInfo, sizeof(RecvInfo));
	RecvInfo.BufferArray = &RecvBuffer;
	RecvInfo.BufferCount = 1;
	RecvInfo.TdiFlags    = 0x20;

	IO_STATUS_BLOCK IOSB;
	Memory::Zero(&IOSB, sizeof(IOSB));

	Status = NTDLL::ZwDeviceIoControlFile(m_socket, SockEvent, nullptr, nullptr, &IOSB,
	                                      IOCTL_AFD_RECV,
	                                      &RecvInfo, sizeof(RecvInfo),
	                                      nullptr, 0);

	if (Status == (NTSTATUS)STATUS_PENDING)
	{
		// 5-minute receive timeout (100-ns units, negative = relative to now)
		LARGE_INTEGER Timeout;
		Timeout.QuadPart = 5 * 60 * 1000 * -10000LL;

		if (AfdWait(SockEvent, IOSB, Status, &Timeout) == (NTSTATUS)STATUS_TIMEOUT)
		{
			(void)NTDLL::ZwClose(SockEvent);
			NetworkError err;
			err.Push((UINT32)STATUS_TIMEOUT);
			err.Push(NetworkError::Socket_ReadFailed_Timeout);
			return Result<SSIZE, NetworkError>::Err(err);
		}
	}

	(void)NTDLL::ZwClose(SockEvent);

	if (!NT_SUCCESS(Status))
	{
		NetworkError err;
		err.Push((UINT32)Status);
		err.Push(NetworkError::Socket_ReadFailed_Recv);
		return Result<SSIZE, NetworkError>::Err(err);
	}

	return Result<SSIZE, NetworkError>::Ok((SSIZE)IOSB.Information);
}

Result<UINT32, NetworkError> Socket::Write(PCVOID buffer, UINT32 bufferLength)
{
	LOG_DEBUG("Write(handle: 0x%p, length: %d)\n", this, bufferLength);

	if (!IsValid())
	{
		NetworkError err;
		err.Push(NetworkError::Socket_WriteFailed_HandleInvalid);
		return Result<UINT32, NetworkError>::Err(err);
	}

	PVOID SockEvent = nullptr;
	NTSTATUS Status = NTDLL::ZwCreateEvent(&SockEvent,
	                                       EVENT_ALL_ACCESS,
	                                       nullptr,
	                                       SynchronizationEvent,
	                                       false);
	if (!NT_SUCCESS(Status))
	{
		NetworkError err;
		err.Push((UINT32)Status);
		err.Push(NetworkError::Socket_WriteFailed_EventCreate);
		return Result<UINT32, NetworkError>::Err(err);
	}

	AfdSendRecvInfo SendInfo;
	Memory::Zero(&SendInfo, sizeof(SendInfo));
	SendInfo.BufferCount = 1;

	AfdWsaBuf      SendBuffer;
	IO_STATUS_BLOCK IOSB;
	UINT32          totalSent = 0;

	do
	{
		Memory::Zero(&IOSB, sizeof(IOSB));
		SendBuffer.Buffer    = (PVOID)((PCHAR)buffer + totalSent);
		SendBuffer.Length    = bufferLength - totalSent;
		SendInfo.BufferArray = &SendBuffer;

		Status = NTDLL::ZwDeviceIoControlFile(m_socket, SockEvent, nullptr, nullptr, &IOSB,
		                                      IOCTL_AFD_SEND,
		                                      &SendInfo, sizeof(SendInfo),
		                                      nullptr, 0);

		if (Status == (NTSTATUS)STATUS_PENDING)
		{
			// 1-minute send timeout
			LARGE_INTEGER Timeout;
			Timeout.QuadPart = 1 * 60 * 1000 * -10000LL;

			if (AfdWait(SockEvent, IOSB, Status, &Timeout) == (NTSTATUS)STATUS_TIMEOUT)
			{
				(void)NTDLL::ZwClose(SockEvent);
				NetworkError err;
				err.Push((UINT32)STATUS_TIMEOUT);
				err.Push(NetworkError::Socket_WriteFailed_Timeout);
				return Result<UINT32, NetworkError>::Err(err);
			}
		}

		if (!NT_SUCCESS(Status))
		{
			(void)NTDLL::ZwClose(SockEvent);
			NetworkError err;
			err.Push((UINT32)Status);
			err.Push(NetworkError::Socket_WriteFailed_Send);
			return Result<UINT32, NetworkError>::Err(err);
		}

		totalSent += (UINT32)IOSB.Information;
	} while (totalSent < bufferLength);

	(void)NTDLL::ZwClose(SockEvent);
	LOG_DEBUG("Write: sent %d bytes\n", totalSent);
	return Result<UINT32, NetworkError>::Ok(totalSent);
}

Socket::Socket(const IPAddress &ipAddress, UINT16 port) : ip(ipAddress), port(port), m_socket(nullptr)
{
	LOG_DEBUG("Create(this: 0x%p)\n", this);

	AfdSocketParams EaBuffer;
	Memory::Zero(&EaBuffer, sizeof(EaBuffer));
	EaBuffer.EaNameLength  = 0x0F1E;
	EaBuffer.EaValueLength = 0x001E;

	auto afdOpSource = "AfdOpenPacketXX"_embed;
	Memory::Copy(EaBuffer.AfdOperation, afdOpSource, 16);
	EaBuffer.AfdOperation[15] = '\0';

	EaBuffer.AddressFamily = SocketAddressHelper::GetAddressFamily(ip);
	EaBuffer.SocketType    = SOCK_STREAM;
	EaBuffer.Protocol      = IPPROTO_TCP;

	UNICODE_STRING AfdName;
	auto afdNameSource  = L"\\Device\\Afd\\Endpoint"_embed;
	AfdName.Buffer      = (PWCHAR)(PCWCHAR)afdNameSource;
	AfdName.Length      = afdNameSource.Length() * sizeof(WCHAR);
	AfdName.MaximumLength = afdNameSource.Length() * sizeof(WCHAR);

	OBJECT_ATTRIBUTES   Object;
	IO_STATUS_BLOCK     IOSB;
	Memory::Zero(&IOSB, sizeof(IOSB));
	InitializeObjectAttributes(&Object, &AfdName, OBJ_CASE_INSENSITIVE | OBJ_INHERIT, 0, 0);

	NTSTATUS Status = NTDLL::ZwCreateFile(&m_socket,
	                                      GENERIC_READ | GENERIC_WRITE | SYNCHRONIZE,
	                                      &Object,
	                                      &IOSB,
	                                      nullptr,
	                                      0,
	                                      FILE_SHARE_READ | FILE_SHARE_WRITE,
	                                      FILE_OPEN_IF,
	                                      0,
	                                      &EaBuffer,
	                                      sizeof(EaBuffer));
	if (!NT_SUCCESS(Status))
	{
		// Caller will detect failure via IsValid() → Open() returns OpenFailed_HandleInvalid
		LOG_DEBUG("Create: ZwCreateFile failed: 0x%08X\n", Status);
		m_socket = nullptr;
	}
	else
	{
		LOG_DEBUG("Create: handle: 0x%p\n", m_socket);
	}
}
