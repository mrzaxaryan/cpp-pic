/**
 * socket.cc - UEFI Socket Implementation using TCP4/TCP6 Protocol
 *
 * Implements TCP socket functionality using EFI_TCP4_PROTOCOL (IPv4)
 * and EFI_TCP6_PROTOCOL (IPv6). Uses synchronous event-based I/O.
 */

#include "socket.h"
#include "memory.h"
#include "efi_context.h"
#include "efi_tcp4_protocol.h"
#include "efi_tcp6_protocol.h"
#include "efi_service_binding.h"
#include "efi_simple_network_protocol.h"

// Track if network has been initialized
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
	BOOL IsIPv6;

	// Protocol pointers (only one is used based on IsIPv6)
	union
	{
		EFI_TCP4_PROTOCOL *Tcp4;
		EFI_TCP6_PROTOCOL *Tcp6;
	};
};

// =============================================================================
// Helper Functions
// =============================================================================

// Empty event notification function (required for CreateEvent)
static VOID EFIAPI EmptyNotify(EFI_EVENT Event, PVOID Context)
{
	(VOID) Event;
	(VOID) Context;
}

// Initialize network interface using SNP
static BOOL InitializeNetworkInterface(EFI_BOOT_SERVICES *bs, EFI_HANDLE ImageHandle)
{
	if (g_NetworkInitialized)
		return TRUE;

	EFI_GUID SnpGuid = EFI_SIMPLE_NETWORK_PROTOCOL_GUID;

	// Find all SNP handles
	USIZE HandleCount = 0;
	EFI_HANDLE *HandleBuffer = NULL;

	EFI_STATUS Status = bs->LocateHandleBuffer(ByProtocol, &SnpGuid, NULL, &HandleCount, &HandleBuffer);
	if (EFI_ERROR_CHECK(Status) || HandleCount == 0)
		return FALSE;

	// Try to initialize each network interface
	for (USIZE i = 0; i < HandleCount; i++)
	{
		EFI_SIMPLE_NETWORK_PROTOCOL *Snp = NULL;
		Status = bs->OpenProtocol(
			HandleBuffer[i],
			&SnpGuid,
			(PVOID *)&Snp,
			ImageHandle,
			NULL,
			EFI_OPEN_PROTOCOL_GET_PROTOCOL);

		if (EFI_ERROR_CHECK(Status) || Snp == NULL)
			continue;

		// Check current state and initialize if needed
		if (Snp->Mode != NULL)
		{
			if (Snp->Mode->State == EfiSimpleNetworkStopped)
			{
				Status = Snp->Start(Snp);
				if (EFI_ERROR_CHECK(Status))
					continue;
			}

			if (Snp->Mode->State == EfiSimpleNetworkStarted)
			{
				Status = Snp->Initialize(Snp, 0, 0);
				// Continue even if init fails
			}

			g_NetworkInitialized = TRUE;
		}
	}

	bs->FreePool(HandleBuffer);
	return g_NetworkInitialized;
}

// Wait for a completion token with timeout using polling
static EFI_STATUS WaitForCompletionToken(EFI_BOOT_SERVICES *bs, EFI_STATUS *TokenStatus, UINT64 TimeoutMs)
{
	UINT64 PollIntervalMs = 1; // Poll every 50ms
	UINT64 ElapsedMs = 0;

	while (ElapsedMs < TimeoutMs)
	{
		if (*TokenStatus != EFI_NOT_READY)
			return EFI_SUCCESS;

		bs->Stall(PollIntervalMs * 1000); // Stall takes microseconds
		ElapsedMs += PollIntervalMs;
	}

	return EFI_TIMEOUT;
}


// =============================================================================
// Socket Constructor - IPv4 and IPv6 Support
// =============================================================================

Socket::Socket(const IPAddress &ipAddress, UINT16 portNum)
	: ip(ipAddress), port(portNum), m_socket(NULL)
{
	EFI_CONTEXT *ctx = GetEfiContext();
	if (ctx == NULL || ctx->SystemTable == NULL)
		return;

	EFI_BOOT_SERVICES *bs = ctx->SystemTable->BootServices;

	// Allocate socket context
	UefiSocketContext *sockCtx = NULL;
	EFI_STATUS Status = bs->AllocatePool(EfiLoaderData, sizeof(UefiSocketContext), (PVOID *)&sockCtx);
	if (EFI_ERROR_CHECK(Status) || sockCtx == NULL)
		return;

	Memory::Zero(sockCtx, sizeof(UefiSocketContext));
	sockCtx->IsIPv6 = ip.IsIPv6();

	// Select appropriate service binding GUID based on IP version
	EFI_GUID ServiceBindingGuid;
	EFI_GUID ProtocolGuid;

	if (sockCtx->IsIPv6)
	{
		ServiceBindingGuid = (EFI_GUID)EFI_TCP6_SERVICE_BINDING_PROTOCOL_GUID;
		ProtocolGuid = (EFI_GUID)EFI_TCP6_PROTOCOL_GUID;
	}
	else
	{
		ServiceBindingGuid = (EFI_GUID)EFI_TCP4_SERVICE_BINDING_PROTOCOL_GUID;
		ProtocolGuid = (EFI_GUID)EFI_TCP4_PROTOCOL_GUID;
	}

	// Find TCP service binding protocol
	USIZE HandleCount = 0;
	EFI_HANDLE *HandleBuffer = NULL;

	Status = bs->LocateHandleBuffer(ByProtocol, &ServiceBindingGuid, NULL, &HandleCount, &HandleBuffer);
	if (EFI_ERROR_CHECK(Status) || HandleCount == 0)
	{
		bs->FreePool(sockCtx);
		return;
	}

	// Use first available handle
	sockCtx->ServiceHandle = HandleBuffer[0];

	// Open service binding protocol
	Status = bs->OpenProtocol(
		sockCtx->ServiceHandle,
		&ServiceBindingGuid,
		(PVOID *)&sockCtx->ServiceBinding,
		ctx->ImageHandle,
		NULL,
		EFI_OPEN_PROTOCOL_GET_PROTOCOL);

	bs->FreePool(HandleBuffer);

	if (EFI_ERROR_CHECK(Status))
	{
		bs->FreePool(sockCtx);
		return;
	}

	// Create TCP child handle
	sockCtx->TcpHandle = NULL;
	Status = sockCtx->ServiceBinding->CreateChild(sockCtx->ServiceBinding, &sockCtx->TcpHandle);
	if (EFI_ERROR_CHECK(Status))
	{
		bs->FreePool(sockCtx);
		return;
	}

	// Open TCP protocol on child handle
	PVOID TcpInterface = NULL;
	Status = bs->OpenProtocol(
		sockCtx->TcpHandle,
		&ProtocolGuid,
		&TcpInterface,
		ctx->ImageHandle,
		NULL,
		EFI_OPEN_PROTOCOL_GET_PROTOCOL);

	if (EFI_ERROR_CHECK(Status))
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
// Open (Connect) - IPv4 and IPv6
// =============================================================================

BOOL Socket::Open()
{
	if (!IsValid())
		return FALSE;

	UefiSocketContext *sockCtx = (UefiSocketContext *)m_socket;
	EFI_CONTEXT *ctx = GetEfiContext();
	EFI_BOOT_SERVICES *bs = ctx->SystemTable->BootServices;
	EFI_STATUS Status;

	// Initialize network interface if not already done
	InitializeNetworkInterface(bs, ctx->ImageHandle);

	if (sockCtx->IsIPv6)
	{
		// Configure TCP6 connection
		EFI_TCP6_CONFIG_DATA ConfigData;
		Memory::Zero(&ConfigData, sizeof(ConfigData));

		ConfigData.TrafficClass = 0;
		ConfigData.HopLimit = 64;
		ConfigData.AccessPoint.StationPort = 0;
		ConfigData.AccessPoint.ActiveFlag = TRUE;

		// Set remote IPv6 address
		const UINT8 *ipv6Addr = ip.ToIPv6();
		if (ipv6Addr != NULL)
			Memory::Copy(ConfigData.AccessPoint.RemoteAddress.Addr, ipv6Addr, 16);
		ConfigData.AccessPoint.RemotePort = port;
		ConfigData.ControlOption = NULL;

		Status = sockCtx->Tcp6->Configure(sockCtx->Tcp6, &ConfigData);
		if (EFI_ERROR_CHECK(Status))
			return FALSE;

		sockCtx->IsConfigured = TRUE;

		// Create event for connection completion
		EFI_EVENT ConnectEvent;
		Status = bs->CreateEvent(EVT_NOTIFY_SIGNAL, TPL_CALLBACK, EmptyNotify, NULL, &ConnectEvent);
		if (EFI_ERROR_CHECK(Status))
			return FALSE;

		// Setup connection token
		EFI_TCP6_CONNECTION_TOKEN ConnectToken;
		Memory::Zero(&ConnectToken, sizeof(ConnectToken));
		ConnectToken.CompletionToken.Event = ConnectEvent;
		ConnectToken.CompletionToken.Status = EFI_NOT_READY;

		Status = sockCtx->Tcp6->Connect(sockCtx->Tcp6, &ConnectToken);
		if (EFI_ERROR_CHECK(Status) && Status != EFI_NOT_READY)
		{
			bs->CloseEvent(ConnectEvent);
			return FALSE;
		}

		Status = WaitForCompletionToken(bs, &ConnectToken.CompletionToken.Status, 30000);
		bs->CloseEvent(ConnectEvent);

		if (EFI_ERROR_CHECK(Status))
			return FALSE;
		if (EFI_ERROR_CHECK(ConnectToken.CompletionToken.Status))
			return FALSE;
	}
	else
	{
		// Configure TCP4 connection
		EFI_TCP4_CONFIG_DATA ConfigData;
		Memory::Zero(&ConfigData, sizeof(ConfigData));

		ConfigData.TypeOfService = 0;
		ConfigData.TimeToLive = 64;
		ConfigData.AccessPoint.StationPort = 0;
		ConfigData.AccessPoint.ActiveFlag = TRUE;
		ConfigData.ControlOption = NULL;

		// Set remote IPv4 address
		UINT32 ipv4Addr = ip.ToIPv4();
		ConfigData.AccessPoint.RemoteAddress.Addr[0] = (UINT8)(ipv4Addr & 0xFF);
		ConfigData.AccessPoint.RemoteAddress.Addr[1] = (UINT8)((ipv4Addr >> 8) & 0xFF);
		ConfigData.AccessPoint.RemoteAddress.Addr[2] = (UINT8)((ipv4Addr >> 16) & 0xFF);
		ConfigData.AccessPoint.RemoteAddress.Addr[3] = (UINT8)((ipv4Addr >> 24) & 0xFF);
		ConfigData.AccessPoint.RemotePort = port;

		// Use DHCP (UseDefaultAddress = TRUE)
		ConfigData.AccessPoint.UseDefaultAddress = TRUE;
		Status = sockCtx->Tcp4->Configure(sockCtx->Tcp4, &ConfigData);
		if (EFI_ERROR_CHECK(Status))
			return FALSE;

		sockCtx->IsConfigured = TRUE;

		// Create event for connection completion
		EFI_EVENT ConnectEvent;
		Status = bs->CreateEvent(EVT_NOTIFY_SIGNAL, TPL_CALLBACK, EmptyNotify, NULL, &ConnectEvent);
		if (EFI_ERROR_CHECK(Status))
			return FALSE;

		// Setup connection token
		EFI_TCP4_CONNECTION_TOKEN ConnectToken;
		Memory::Zero(&ConnectToken, sizeof(ConnectToken));
		ConnectToken.CompletionToken.Event = ConnectEvent;
		ConnectToken.CompletionToken.Status = EFI_NOT_READY;

		Status = sockCtx->Tcp4->Connect(sockCtx->Tcp4, &ConnectToken);
		if (EFI_ERROR_CHECK(Status) && Status != EFI_NOT_READY)
		{
			bs->CloseEvent(ConnectEvent);
			return FALSE;
		}

		Status = WaitForCompletionToken(bs, &ConnectToken.CompletionToken.Status, 30000);
		bs->CloseEvent(ConnectEvent);

		if (EFI_ERROR_CHECK(Status))
			return FALSE;
		if (EFI_ERROR_CHECK(ConnectToken.CompletionToken.Status))
			return FALSE;
	}

	return TRUE;
}

// =============================================================================
// Close - IPv4 and IPv6
// =============================================================================

BOOL Socket::Close()
{
	if (!IsValid())
		return FALSE;

	UefiSocketContext *sockCtx = (UefiSocketContext *)m_socket;
	EFI_CONTEXT *ctx = GetEfiContext();
	EFI_BOOT_SERVICES *bs = ctx->SystemTable->BootServices;

	if (sockCtx->IsConfigured)
	{
		EFI_EVENT CloseEvent;
		EFI_STATUS Status = bs->CreateEvent(EVT_NOTIFY_SIGNAL, TPL_CALLBACK, EmptyNotify, NULL, &CloseEvent);
		if (!EFI_ERROR_CHECK(Status))
		{
			if (sockCtx->IsIPv6)
			{
				EFI_TCP6_CLOSE_TOKEN CloseToken;
				Memory::Zero(&CloseToken, sizeof(CloseToken));
				CloseToken.CompletionToken.Event = CloseEvent;
				CloseToken.CompletionToken.Status = EFI_NOT_READY;
				CloseToken.AbortOnClose = FALSE;

				Status = sockCtx->Tcp6->Close(sockCtx->Tcp6, &CloseToken);
				if (!EFI_ERROR_CHECK(Status) || Status == EFI_NOT_READY)
					WaitForCompletionToken(bs, &CloseToken.CompletionToken.Status, 500);

				sockCtx->Tcp6->Configure(sockCtx->Tcp6, NULL);
			}
			else
			{
				EFI_TCP4_CLOSE_TOKEN CloseToken;
				Memory::Zero(&CloseToken, sizeof(CloseToken));
				CloseToken.CompletionToken.Event = CloseEvent;
				CloseToken.CompletionToken.Status = EFI_NOT_READY;
				CloseToken.AbortOnClose = FALSE;

				Status = sockCtx->Tcp4->Close(sockCtx->Tcp4, &CloseToken);
				if (!EFI_ERROR_CHECK(Status) || Status == EFI_NOT_READY)
					WaitForCompletionToken(bs, &CloseToken.CompletionToken.Status, 500);

				sockCtx->Tcp4->Configure(sockCtx->Tcp4, NULL);
			}

			bs->CloseEvent(CloseEvent);
		}
	}

	// Close TCP protocol
	EFI_GUID ProtocolGuid;
	if (sockCtx->IsIPv6)
		ProtocolGuid = (EFI_GUID)EFI_TCP6_PROTOCOL_GUID;
	else
		ProtocolGuid = (EFI_GUID)EFI_TCP4_PROTOCOL_GUID;

	bs->CloseProtocol(sockCtx->TcpHandle, &ProtocolGuid, ctx->ImageHandle, NULL);

	// Destroy child handle
	sockCtx->ServiceBinding->DestroyChild(sockCtx->ServiceBinding, sockCtx->TcpHandle);

	// Close service binding protocol
	EFI_GUID ServiceBindingGuid;
	if (sockCtx->IsIPv6)
		ServiceBindingGuid = (EFI_GUID)EFI_TCP6_SERVICE_BINDING_PROTOCOL_GUID;
	else
		ServiceBindingGuid = (EFI_GUID)EFI_TCP4_SERVICE_BINDING_PROTOCOL_GUID;

	bs->CloseProtocol(sockCtx->ServiceHandle, &ServiceBindingGuid, ctx->ImageHandle, NULL);

	bs->FreePool(sockCtx);
	m_socket = NULL;

	return TRUE;
}

// =============================================================================
// Bind (not used for active connections)
// =============================================================================

BOOL Socket::Bind(SockAddr *SocketAddress, INT32 ShareType)
{
	(VOID) SocketAddress;
	(VOID) ShareType;
	return FALSE;
}

// =============================================================================
// Read - IPv4 and IPv6
// =============================================================================

SSIZE Socket::Read(PVOID buffer, UINT32 bufferLength)
{
	if (!IsValid() || buffer == NULL || bufferLength == 0)
		return -1;

	UefiSocketContext *sockCtx = (UefiSocketContext *)m_socket;
	EFI_CONTEXT *ctx = GetEfiContext();
	EFI_BOOT_SERVICES *bs = ctx->SystemTable->BootServices;
	EFI_STATUS Status;
	SSIZE bytesRead = -1;

	EFI_EVENT RxEvent;
	Status = bs->CreateEvent(EVT_NOTIFY_SIGNAL, TPL_CALLBACK, EmptyNotify, NULL, &RxEvent);
	if (EFI_ERROR_CHECK(Status))
		return -1;

	if (sockCtx->IsIPv6)
	{
		EFI_TCP6_RECEIVE_DATA *RxData = NULL;
		Status = bs->AllocatePool(EfiLoaderData, sizeof(EFI_TCP6_RECEIVE_DATA), (PVOID *)&RxData);
		if (EFI_ERROR_CHECK(Status))
		{
			bs->CloseEvent(RxEvent);
			return -1;
		}

		Memory::Zero(RxData, sizeof(EFI_TCP6_RECEIVE_DATA));
		RxData->DataLength = bufferLength;
		RxData->FragmentCount = 1;
		RxData->FragmentTable[0].FragmentLength = bufferLength;
		RxData->FragmentTable[0].FragmentBuffer = buffer;

		EFI_TCP6_IO_TOKEN RxToken;
		Memory::Zero(&RxToken, sizeof(RxToken));
		RxToken.CompletionToken.Event = RxEvent;
		RxToken.CompletionToken.Status = EFI_NOT_READY;
		RxToken.Packet.RxData = RxData;

		Status = sockCtx->Tcp6->Receive(sockCtx->Tcp6, &RxToken);
		if (!EFI_ERROR_CHECK(Status) || Status == EFI_NOT_READY)
		{
			Status = WaitForCompletionToken(bs, &RxToken.CompletionToken.Status, 60000);
			if (!EFI_ERROR_CHECK(Status) && !EFI_ERROR_CHECK(RxToken.CompletionToken.Status))
				bytesRead = (SSIZE)RxData->DataLength;
		}

		bs->FreePool(RxData);
	}
	else
	{
		EFI_TCP4_RECEIVE_DATA *RxData = NULL;
		Status = bs->AllocatePool(EfiLoaderData, sizeof(EFI_TCP4_RECEIVE_DATA), (PVOID *)&RxData);
		if (EFI_ERROR_CHECK(Status))
		{
			bs->CloseEvent(RxEvent);
			return -1;
		}

		Memory::Zero(RxData, sizeof(EFI_TCP4_RECEIVE_DATA));
		RxData->DataLength = bufferLength;
		RxData->FragmentCount = 1;
		RxData->FragmentTable[0].FragmentLength = bufferLength;
		RxData->FragmentTable[0].FragmentBuffer = buffer;

		EFI_TCP4_IO_TOKEN RxToken;
		Memory::Zero(&RxToken, sizeof(RxToken));
		RxToken.CompletionToken.Event = RxEvent;
		RxToken.CompletionToken.Status = EFI_NOT_READY;
		RxToken.Packet.RxData = RxData;

		Status = sockCtx->Tcp4->Receive(sockCtx->Tcp4, &RxToken);
		if (!EFI_ERROR_CHECK(Status) || Status == EFI_NOT_READY)
		{
			Status = WaitForCompletionToken(bs, &RxToken.CompletionToken.Status, 60000);
			if (!EFI_ERROR_CHECK(Status) && !EFI_ERROR_CHECK(RxToken.CompletionToken.Status))
				bytesRead = (SSIZE)RxData->DataLength;
		}

		bs->FreePool(RxData);
	}

	bs->CloseEvent(RxEvent);
	return bytesRead;
}

// =============================================================================
// Write - IPv4 and IPv6
// =============================================================================

UINT32 Socket::Write(PCVOID buffer, UINT32 bufferLength)
{
	if (!IsValid() || buffer == NULL || bufferLength == 0)
		return 0;

	UefiSocketContext *sockCtx = (UefiSocketContext *)m_socket;
	EFI_CONTEXT *ctx = GetEfiContext();
	EFI_BOOT_SERVICES *bs = ctx->SystemTable->BootServices;
	EFI_STATUS Status;
	UINT32 bytesSent = 0;

	EFI_EVENT TxEvent;
	Status = bs->CreateEvent(EVT_NOTIFY_SIGNAL, TPL_CALLBACK, EmptyNotify, NULL, &TxEvent);
	if (EFI_ERROR_CHECK(Status))
		return 0;

	if (sockCtx->IsIPv6)
	{
		EFI_TCP6_TRANSMIT_DATA *TxData = NULL;
		Status = bs->AllocatePool(EfiLoaderData, sizeof(EFI_TCP6_TRANSMIT_DATA), (PVOID *)&TxData);
		if (EFI_ERROR_CHECK(Status))
		{
			bs->CloseEvent(TxEvent);
			return 0;
		}

		Memory::Zero(TxData, sizeof(EFI_TCP6_TRANSMIT_DATA));
		TxData->Push = TRUE;
		TxData->Urgent = FALSE;
		TxData->DataLength = bufferLength;
		TxData->FragmentCount = 1;
		TxData->FragmentTable[0].FragmentLength = bufferLength;
		TxData->FragmentTable[0].FragmentBuffer = (PVOID)buffer;

		EFI_TCP6_IO_TOKEN TxToken;
		Memory::Zero(&TxToken, sizeof(TxToken));
		TxToken.CompletionToken.Event = TxEvent;
		TxToken.CompletionToken.Status = EFI_NOT_READY;
		TxToken.Packet.TxData = TxData;

		Status = sockCtx->Tcp6->Transmit(sockCtx->Tcp6, &TxToken);
		if (!EFI_ERROR_CHECK(Status) || Status == EFI_NOT_READY)
		{
			Status = WaitForCompletionToken(bs, &TxToken.CompletionToken.Status, 30000);
			if (!EFI_ERROR_CHECK(Status) && !EFI_ERROR_CHECK(TxToken.CompletionToken.Status))
				bytesSent = bufferLength;
		}

		bs->FreePool(TxData);
	}
	else
	{
		EFI_TCP4_TRANSMIT_DATA *TxData = NULL;
		Status = bs->AllocatePool(EfiLoaderData, sizeof(EFI_TCP4_TRANSMIT_DATA), (PVOID *)&TxData);
		if (EFI_ERROR_CHECK(Status))
		{
			bs->CloseEvent(TxEvent);
			return 0;
		}

		Memory::Zero(TxData, sizeof(EFI_TCP4_TRANSMIT_DATA));
		TxData->Push = TRUE;
		TxData->Urgent = FALSE;
		TxData->DataLength = bufferLength;
		TxData->FragmentCount = 1;
		TxData->FragmentTable[0].FragmentLength = bufferLength;
		TxData->FragmentTable[0].FragmentBuffer = (PVOID)buffer;

		EFI_TCP4_IO_TOKEN TxToken;
		Memory::Zero(&TxToken, sizeof(TxToken));
		TxToken.CompletionToken.Event = TxEvent;
		TxToken.CompletionToken.Status = EFI_NOT_READY;
		TxToken.Packet.TxData = TxData;

		Status = sockCtx->Tcp4->Transmit(sockCtx->Tcp4, &TxToken);
		if (!EFI_ERROR_CHECK(Status) || Status == EFI_NOT_READY)
		{
			Status = WaitForCompletionToken(bs, &TxToken.CompletionToken.Status, 30000);
			if (!EFI_ERROR_CHECK(Status) && !EFI_ERROR_CHECK(TxToken.CompletionToken.Status))
				bytesSent = bufferLength;
		}

		bs->FreePool(TxData);
	}

	bs->CloseEvent(TxEvent);
	return bytesSent;
}
