/**
 * socket.cc - UEFI Socket Implementation using TCP4/TCP6 Protocol
 */

#include "socket.h"
#include "memory.h"
#include "efi_context.h"
#include "efi_tcp4_protocol.h"
#include "efi_tcp6_protocol.h"
#include "efi_service_binding.h"
#include "efi_simple_network_protocol.h"

static BOOL g_NetworkInitialized = FALSE;

// =============================================================================
// Internal Socket Context
// =============================================================================

struct UefiSocketContext
{
	EFI_HANDLE TcpHandle;
	EFI_SERVICE_BINDING_PROTOCOL *ServiceBinding;
	EFI_HANDLE ServiceHandle;
	BOOL IsConfigured;
	BOOL IsConnected;
	BOOL IsIPv6;
	union
	{
		EFI_TCP4_PROTOCOL *Tcp4;
		EFI_TCP6_PROTOCOL *Tcp6;
	};
};

// =============================================================================
// Helper Functions
// =============================================================================

static VOID EFIAPI EmptyNotify(EFI_EVENT Event, PVOID Context)
{
	(VOID)Event;
	(VOID)Context;
}

static BOOL InitializeNetworkInterface(EFI_BOOT_SERVICES *bs, EFI_HANDLE ImageHandle)
{
	if (g_NetworkInitialized)
		return TRUE;

	EFI_GUID SnpGuid = EFI_SIMPLE_NETWORK_PROTOCOL_GUID;
	USIZE HandleCount = 0;
	EFI_HANDLE *HandleBuffer = NULL;

	if (EFI_ERROR_CHECK(bs->LocateHandleBuffer(ByProtocol, &SnpGuid, NULL, &HandleCount, &HandleBuffer)) || HandleCount == 0)
		return FALSE;

	for (USIZE i = 0; i < HandleCount; i++)
	{
		EFI_SIMPLE_NETWORK_PROTOCOL *Snp = NULL;
		if (EFI_ERROR_CHECK(bs->OpenProtocol(HandleBuffer[i], &SnpGuid, (PVOID *)&Snp, ImageHandle, NULL, EFI_OPEN_PROTOCOL_GET_PROTOCOL)) || Snp == NULL)
			continue;

		if (Snp->Mode != NULL)
		{
			if (Snp->Mode->State == EfiSimpleNetworkStopped)
				Snp->Start(Snp);
			if (Snp->Mode->State == EfiSimpleNetworkStarted)
				Snp->Initialize(Snp, 0, 0);
			g_NetworkInitialized = TRUE;
			break;
		}
	}

	bs->FreePool(HandleBuffer);
	return g_NetworkInitialized;
}

// Wait for async operation using WaitForEvent + Poll
template <typename TCP_PROTOCOL>
static EFI_STATUS WaitForCompletion(EFI_BOOT_SERVICES *bs, TCP_PROTOCOL *Tcp, EFI_EVENT Event, volatile EFI_STATUS *TokenStatus, UINT64 TimeoutMs)
{
	// Check immediately
	if (*TokenStatus != EFI_NOT_READY)
		return EFI_SUCCESS;

	// Convert ms to 100ns units for timer
	UINT64 timeout100ns = TimeoutMs * 10000;
	EFI_EVENT timerEvent;

	if (EFI_ERROR_CHECK(bs->CreateEvent(EVT_TIMER, 0, NULL, NULL, &timerEvent)))
	{
		// Fallback to polling if timer fails
		for (UINT64 i = 0; i < TimeoutMs && *TokenStatus == EFI_NOT_READY; i++)
		{
			Tcp->Poll(Tcp);
			bs->Stall(1000);
		}
		return (*TokenStatus != EFI_NOT_READY) ? EFI_SUCCESS : EFI_TIMEOUT;
	}

	bs->SetTimer(timerEvent, TimerRelative, timeout100ns);

	EFI_EVENT waitEvents[2] = {Event, timerEvent};
	USIZE index;

	while (*TokenStatus == EFI_NOT_READY)
	{
		Tcp->Poll(Tcp);

		EFI_STATUS waitStatus = bs->WaitForEvent(2, waitEvents, &index);
		if (EFI_ERROR_CHECK(waitStatus) || index == 1)
			break; // Timeout or error
	}

	bs->SetTimer(timerEvent, TimerCancel, 0);
	bs->CloseEvent(timerEvent);

	return (*TokenStatus != EFI_NOT_READY) ? EFI_SUCCESS : EFI_TIMEOUT;
}

// =============================================================================
// Socket Constructor
// =============================================================================

Socket::Socket(const IPAddress &ipAddress, UINT16 portNum)
	: ip(ipAddress), port(portNum), m_socket(NULL)
{
	EFI_CONTEXT *ctx = GetEfiContext();
	if (ctx == NULL || ctx->SystemTable == NULL)
		return;

	EFI_BOOT_SERVICES *bs = ctx->SystemTable->BootServices;
	UefiSocketContext *sockCtx = NULL;

	if (EFI_ERROR_CHECK(bs->AllocatePool(EfiLoaderData, sizeof(UefiSocketContext), (PVOID *)&sockCtx)) || sockCtx == NULL)
		return;

	Memory::Zero(sockCtx, sizeof(UefiSocketContext));
	sockCtx->IsIPv6 = ip.IsIPv6();

	EFI_GUID ServiceBindingGuid = sockCtx->IsIPv6
		? (EFI_GUID)EFI_TCP6_SERVICE_BINDING_PROTOCOL_GUID
		: (EFI_GUID)EFI_TCP4_SERVICE_BINDING_PROTOCOL_GUID;
	EFI_GUID ProtocolGuid = sockCtx->IsIPv6
		? (EFI_GUID)EFI_TCP6_PROTOCOL_GUID
		: (EFI_GUID)EFI_TCP4_PROTOCOL_GUID;

	USIZE HandleCount = 0;
	EFI_HANDLE *HandleBuffer = NULL;
	if (EFI_ERROR_CHECK(bs->LocateHandleBuffer(ByProtocol, &ServiceBindingGuid, NULL, &HandleCount, &HandleBuffer)) || HandleCount == 0)
	{
		bs->FreePool(sockCtx);
		return;
	}

	sockCtx->ServiceHandle = HandleBuffer[0];
	EFI_STATUS Status = bs->OpenProtocol(sockCtx->ServiceHandle, &ServiceBindingGuid, (PVOID *)&sockCtx->ServiceBinding, ctx->ImageHandle, NULL, EFI_OPEN_PROTOCOL_GET_PROTOCOL);
	bs->FreePool(HandleBuffer);

	if (EFI_ERROR_CHECK(Status))
	{
		bs->FreePool(sockCtx);
		return;
	}

	sockCtx->TcpHandle = NULL;
	if (EFI_ERROR_CHECK(sockCtx->ServiceBinding->CreateChild(sockCtx->ServiceBinding, &sockCtx->TcpHandle)))
	{
		bs->CloseProtocol(sockCtx->ServiceHandle, &ServiceBindingGuid, ctx->ImageHandle, NULL);
		bs->FreePool(sockCtx);
		return;
	}

	PVOID TcpInterface = NULL;
	if (EFI_ERROR_CHECK(bs->OpenProtocol(sockCtx->TcpHandle, &ProtocolGuid, &TcpInterface, ctx->ImageHandle, NULL, EFI_OPEN_PROTOCOL_GET_PROTOCOL)))
	{
		sockCtx->ServiceBinding->DestroyChild(sockCtx->ServiceBinding, sockCtx->TcpHandle);
		bs->FreePool(sockCtx);
		return;
	}

	if (sockCtx->IsIPv6)
		sockCtx->Tcp6 = (EFI_TCP6_PROTOCOL *)TcpInterface;
	else
		sockCtx->Tcp4 = (EFI_TCP4_PROTOCOL *)TcpInterface;

	m_socket = sockCtx;
}

// =============================================================================
// Open (Connect)
// =============================================================================

BOOL Socket::Open()
{
	if (!IsValid())
		return FALSE;

	UefiSocketContext *sockCtx = (UefiSocketContext *)m_socket;
	if (sockCtx->IsConnected)
		return TRUE; // Already connected

	EFI_CONTEXT *ctx = GetEfiContext();
	EFI_BOOT_SERVICES *bs = ctx->SystemTable->BootServices;

	InitializeNetworkInterface(bs, ctx->ImageHandle);

	EFI_EVENT ConnectEvent;
	if (EFI_ERROR_CHECK(bs->CreateEvent(EVT_NOTIFY_SIGNAL, TPL_CALLBACK, EmptyNotify, NULL, &ConnectEvent)))
		return FALSE;

	EFI_STATUS Status;
	BOOL success = FALSE;

	if (sockCtx->IsIPv6)
	{
		EFI_TCP6_CONFIG_DATA ConfigData;
		Memory::Zero(&ConfigData, sizeof(ConfigData));
		ConfigData.HopLimit = 64;
		ConfigData.AccessPoint.ActiveFlag = TRUE;
		ConfigData.AccessPoint.RemotePort = port;
		const UINT8 *ipv6Addr = ip.ToIPv6();
		if (ipv6Addr)
			Memory::Copy(ConfigData.AccessPoint.RemoteAddress.Addr, ipv6Addr, 16);

		if (EFI_ERROR_CHECK(sockCtx->Tcp6->Configure(sockCtx->Tcp6, &ConfigData)))
		{
			bs->CloseEvent(ConnectEvent);
			return FALSE;
		}
		sockCtx->IsConfigured = TRUE;

		EFI_TCP6_CONNECTION_TOKEN ConnectToken;
		Memory::Zero(&ConnectToken, sizeof(ConnectToken));
		ConnectToken.CompletionToken.Event = ConnectEvent;
		ConnectToken.CompletionToken.Status = EFI_NOT_READY;

		Status = sockCtx->Tcp6->Connect(sockCtx->Tcp6, &ConnectToken);
		if (!EFI_ERROR_CHECK(Status) || Status == EFI_NOT_READY)
		{
			Status = WaitForCompletion(bs, sockCtx->Tcp6, ConnectEvent, &ConnectToken.CompletionToken.Status, 30000);
			success = !EFI_ERROR_CHECK(Status) && !EFI_ERROR_CHECK(ConnectToken.CompletionToken.Status);
		}

		if (!success)
		{
			sockCtx->Tcp6->Configure(sockCtx->Tcp6, NULL);
			sockCtx->IsConfigured = FALSE;
		}
	}
	else
	{
		EFI_TCP4_CONFIG_DATA ConfigData;
		Memory::Zero(&ConfigData, sizeof(ConfigData));
		ConfigData.TimeToLive = 64;
		ConfigData.AccessPoint.ActiveFlag = TRUE;
		ConfigData.AccessPoint.UseDefaultAddress = TRUE;
		ConfigData.AccessPoint.RemotePort = port;

		UINT32 ipv4Addr = ip.ToIPv4();
		ConfigData.AccessPoint.RemoteAddress.Addr[0] = (UINT8)(ipv4Addr & 0xFF);
		ConfigData.AccessPoint.RemoteAddress.Addr[1] = (UINT8)((ipv4Addr >> 8) & 0xFF);
		ConfigData.AccessPoint.RemoteAddress.Addr[2] = (UINT8)((ipv4Addr >> 16) & 0xFF);
		ConfigData.AccessPoint.RemoteAddress.Addr[3] = (UINT8)((ipv4Addr >> 24) & 0xFF);

		if (EFI_ERROR_CHECK(sockCtx->Tcp4->Configure(sockCtx->Tcp4, &ConfigData)))
		{
			bs->CloseEvent(ConnectEvent);
			return FALSE;
		}
		sockCtx->IsConfigured = TRUE;

		EFI_TCP4_CONNECTION_TOKEN ConnectToken;
		Memory::Zero(&ConnectToken, sizeof(ConnectToken));
		ConnectToken.CompletionToken.Event = ConnectEvent;
		ConnectToken.CompletionToken.Status = EFI_NOT_READY;

		Status = sockCtx->Tcp4->Connect(sockCtx->Tcp4, &ConnectToken);
		if (!EFI_ERROR_CHECK(Status) || Status == EFI_NOT_READY)
		{
			Status = WaitForCompletion(bs, sockCtx->Tcp4, ConnectEvent, &ConnectToken.CompletionToken.Status, 30000);
			success = !EFI_ERROR_CHECK(Status) && !EFI_ERROR_CHECK(ConnectToken.CompletionToken.Status);
		}

		if (!success)
		{
			sockCtx->Tcp4->Configure(sockCtx->Tcp4, NULL);
			sockCtx->IsConfigured = FALSE;
		}
	}

	bs->CloseEvent(ConnectEvent);
	sockCtx->IsConnected = success;
	return success;
}

// =============================================================================
// Close
// =============================================================================

BOOL Socket::Close()
{
	if (!IsValid())
		return FALSE;

	UefiSocketContext *sockCtx = (UefiSocketContext *)m_socket;
	EFI_CONTEXT *ctx = GetEfiContext();
	EFI_BOOT_SERVICES *bs = ctx->SystemTable->BootServices;

	if (sockCtx->IsIPv6)
	{
		// Cancel any pending I/O
		sockCtx->Tcp6->Cancel(sockCtx->Tcp6, NULL);

		if (sockCtx->IsConnected)
		{
			EFI_EVENT CloseEvent;
			if (!EFI_ERROR_CHECK(bs->CreateEvent(EVT_NOTIFY_SIGNAL, TPL_CALLBACK, EmptyNotify, NULL, &CloseEvent)))
			{
				EFI_TCP6_CLOSE_TOKEN CloseToken;
				Memory::Zero(&CloseToken, sizeof(CloseToken));
				CloseToken.CompletionToken.Event = CloseEvent;
				CloseToken.CompletionToken.Status = EFI_NOT_READY;

				EFI_STATUS Status = sockCtx->Tcp6->Close(sockCtx->Tcp6, &CloseToken);
				if (!EFI_ERROR_CHECK(Status) || Status == EFI_NOT_READY)
					WaitForCompletion(bs, sockCtx->Tcp6, CloseEvent, &CloseToken.CompletionToken.Status, 100);

				bs->CloseEvent(CloseEvent);
			}
		}

		if (sockCtx->IsConfigured)
			sockCtx->Tcp6->Configure(sockCtx->Tcp6, NULL);
	}
	else
	{
		// Cancel any pending I/O
		sockCtx->Tcp4->Cancel(sockCtx->Tcp4, NULL);

		if (sockCtx->IsConnected)
		{
			EFI_EVENT CloseEvent;
			if (!EFI_ERROR_CHECK(bs->CreateEvent(EVT_NOTIFY_SIGNAL, TPL_CALLBACK, EmptyNotify, NULL, &CloseEvent)))
			{
				EFI_TCP4_CLOSE_TOKEN CloseToken;
				Memory::Zero(&CloseToken, sizeof(CloseToken));
				CloseToken.CompletionToken.Event = CloseEvent;
				CloseToken.CompletionToken.Status = EFI_NOT_READY;

				EFI_STATUS Status = sockCtx->Tcp4->Close(sockCtx->Tcp4, &CloseToken);
				if (!EFI_ERROR_CHECK(Status) || Status == EFI_NOT_READY)
					WaitForCompletion(bs, sockCtx->Tcp4, CloseEvent, &CloseToken.CompletionToken.Status, 100);

				bs->CloseEvent(CloseEvent);
			}
		}

		if (sockCtx->IsConfigured)
			sockCtx->Tcp4->Configure(sockCtx->Tcp4, NULL);
	}

	EFI_GUID ProtocolGuid = sockCtx->IsIPv6 ? (EFI_GUID)EFI_TCP6_PROTOCOL_GUID : (EFI_GUID)EFI_TCP4_PROTOCOL_GUID;
	bs->CloseProtocol(sockCtx->TcpHandle, &ProtocolGuid, ctx->ImageHandle, NULL);
	sockCtx->ServiceBinding->DestroyChild(sockCtx->ServiceBinding, sockCtx->TcpHandle);

	EFI_GUID ServiceBindingGuid = sockCtx->IsIPv6 ? (EFI_GUID)EFI_TCP6_SERVICE_BINDING_PROTOCOL_GUID : (EFI_GUID)EFI_TCP4_SERVICE_BINDING_PROTOCOL_GUID;
	bs->CloseProtocol(sockCtx->ServiceHandle, &ServiceBindingGuid, ctx->ImageHandle, NULL);

	bs->FreePool(sockCtx);
	m_socket = NULL;
	return TRUE;
}

// =============================================================================
// Bind (not used)
// =============================================================================

BOOL Socket::Bind(SockAddr *SocketAddress, INT32 ShareType)
{
	(VOID)SocketAddress;
	(VOID)ShareType;
	return FALSE;
}

// =============================================================================
// Read
// =============================================================================

SSIZE Socket::Read(PVOID buffer, UINT32 bufferLength)
{
	if (!IsValid() || buffer == NULL || bufferLength == 0)
		return -1;

	UefiSocketContext *sockCtx = (UefiSocketContext *)m_socket;
	if (!sockCtx->IsConnected)
		return -1;

	EFI_CONTEXT *ctx = GetEfiContext();
	EFI_BOOT_SERVICES *bs = ctx->SystemTable->BootServices;

	EFI_EVENT RxEvent;
	if (EFI_ERROR_CHECK(bs->CreateEvent(EVT_NOTIFY_SIGNAL, TPL_CALLBACK, EmptyNotify, NULL, &RxEvent)))
		return -1;

	SSIZE bytesRead = -1;

	if (sockCtx->IsIPv6)
	{
		EFI_TCP6_RECEIVE_DATA RxData;
		Memory::Zero(&RxData, sizeof(RxData));
		RxData.DataLength = bufferLength;
		RxData.FragmentCount = 1;
		RxData.FragmentTable[0].FragmentLength = bufferLength;
		RxData.FragmentTable[0].FragmentBuffer = buffer;

		EFI_TCP6_IO_TOKEN RxToken;
		Memory::Zero(&RxToken, sizeof(RxToken));
		RxToken.CompletionToken.Event = RxEvent;
		RxToken.CompletionToken.Status = EFI_NOT_READY;
		RxToken.Packet.RxData = &RxData;

		EFI_STATUS Status = sockCtx->Tcp6->Receive(sockCtx->Tcp6, &RxToken);
		if (!EFI_ERROR_CHECK(Status) || Status == EFI_NOT_READY)
		{
			if (!EFI_ERROR_CHECK(WaitForCompletion(bs, sockCtx->Tcp6, RxEvent, &RxToken.CompletionToken.Status, 60000)) && !EFI_ERROR_CHECK(RxToken.CompletionToken.Status))
				bytesRead = (SSIZE)RxData.DataLength;
		}
	}
	else
	{
		EFI_TCP4_RECEIVE_DATA RxData;
		Memory::Zero(&RxData, sizeof(RxData));
		RxData.DataLength = bufferLength;
		RxData.FragmentCount = 1;
		RxData.FragmentTable[0].FragmentLength = bufferLength;
		RxData.FragmentTable[0].FragmentBuffer = buffer;

		EFI_TCP4_IO_TOKEN RxToken;
		Memory::Zero(&RxToken, sizeof(RxToken));
		RxToken.CompletionToken.Event = RxEvent;
		RxToken.CompletionToken.Status = EFI_NOT_READY;
		RxToken.Packet.RxData = &RxData;

		EFI_STATUS Status = sockCtx->Tcp4->Receive(sockCtx->Tcp4, &RxToken);
		if (!EFI_ERROR_CHECK(Status) || Status == EFI_NOT_READY)
		{
			if (!EFI_ERROR_CHECK(WaitForCompletion(bs, sockCtx->Tcp4, RxEvent, &RxToken.CompletionToken.Status, 60000)) && !EFI_ERROR_CHECK(RxToken.CompletionToken.Status))
				bytesRead = (SSIZE)RxData.DataLength;
		}
	}

	bs->CloseEvent(RxEvent);
	return bytesRead;
}

// =============================================================================
// Write
// =============================================================================

UINT32 Socket::Write(PCVOID buffer, UINT32 bufferLength)
{
	if (!IsValid() || buffer == NULL || bufferLength == 0)
		return 0;

	UefiSocketContext *sockCtx = (UefiSocketContext *)m_socket;
	if (!sockCtx->IsConnected)
		return 0;

	EFI_CONTEXT *ctx = GetEfiContext();
	EFI_BOOT_SERVICES *bs = ctx->SystemTable->BootServices;

	EFI_EVENT TxEvent;
	if (EFI_ERROR_CHECK(bs->CreateEvent(EVT_NOTIFY_SIGNAL, TPL_CALLBACK, EmptyNotify, NULL, &TxEvent)))
		return 0;

	UINT32 bytesSent = 0;

	if (sockCtx->IsIPv6)
	{
		EFI_TCP6_TRANSMIT_DATA TxData;
		Memory::Zero(&TxData, sizeof(TxData));
		TxData.Push = TRUE;
		TxData.DataLength = bufferLength;
		TxData.FragmentCount = 1;
		TxData.FragmentTable[0].FragmentLength = bufferLength;
		TxData.FragmentTable[0].FragmentBuffer = (PVOID)buffer;

		EFI_TCP6_IO_TOKEN TxToken;
		Memory::Zero(&TxToken, sizeof(TxToken));
		TxToken.CompletionToken.Event = TxEvent;
		TxToken.CompletionToken.Status = EFI_NOT_READY;
		TxToken.Packet.TxData = &TxData;

		EFI_STATUS Status = sockCtx->Tcp6->Transmit(sockCtx->Tcp6, &TxToken);
		if (!EFI_ERROR_CHECK(Status) || Status == EFI_NOT_READY)
		{
			if (!EFI_ERROR_CHECK(WaitForToken(bs, &TxToken.CompletionToken.Status, 30000)) && !EFI_ERROR_CHECK(TxToken.CompletionToken.Status))
				bytesSent = bufferLength;
		}
	}
	else
	{
		EFI_TCP4_TRANSMIT_DATA TxData;
		Memory::Zero(&TxData, sizeof(TxData));
		TxData.Push = TRUE;
		TxData.DataLength = bufferLength;
		TxData.FragmentCount = 1;
		TxData.FragmentTable[0].FragmentLength = bufferLength;
		TxData.FragmentTable[0].FragmentBuffer = (PVOID)buffer;

		EFI_TCP4_IO_TOKEN TxToken;
		Memory::Zero(&TxToken, sizeof(TxToken));
		TxToken.CompletionToken.Event = TxEvent;
		TxToken.CompletionToken.Status = EFI_NOT_READY;
		TxToken.Packet.TxData = &TxData;

		EFI_STATUS Status = sockCtx->Tcp4->Transmit(sockCtx->Tcp4, &TxToken);
		if (!EFI_ERROR_CHECK(Status) || Status == EFI_NOT_READY)
		{
			if (!EFI_ERROR_CHECK(WaitForToken(bs, &TxToken.CompletionToken.Status, 30000)) && !EFI_ERROR_CHECK(TxToken.CompletionToken.Status))
				bytesSent = bufferLength;
		}
	}

	bs->CloseEvent(TxEvent);
	return bytesSent;
}
