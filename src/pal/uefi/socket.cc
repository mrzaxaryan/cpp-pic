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
#include "efi_ip4_config2_protocol.h"
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
	(VOID)Event;
	(VOID)Context;
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
	if (EFI_ERROR(Status) || HandleCount == 0)
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

		if (EFI_ERROR(Status) || Snp == NULL)
			continue;

		// Check current state and initialize if needed
		if (Snp->Mode != NULL)
		{
			if (Snp->Mode->State == EfiSimpleNetworkStopped)
			{
				Status = Snp->Start(Snp);
				if (EFI_ERROR(Status))
					continue;
			}

			if (Snp->Mode->State == EfiSimpleNetworkStarted)
			{
				Status = Snp->Initialize(Snp, 0, 0);
				if (EFI_ERROR(Status))
				{
					// Try to continue anyway
				}
			}

			g_NetworkInitialized = TRUE;
		}
	}

	bs->FreePool(HandleBuffer);
	return g_NetworkInitialized;
}

// Configure IP4 layer with static QEMU settings
static BOOL ConfigureStaticIP4(EFI_BOOT_SERVICES *bs, EFI_HANDLE ImageHandle)
{
	EFI_GUID Ip4Config2Guid = EFI_IP4_CONFIG2_PROTOCOL_GUID;

	// Find IP4 Config2 protocol
	USIZE HandleCount = 0;
	EFI_HANDLE *HandleBuffer = NULL;

	EFI_STATUS Status = bs->LocateHandleBuffer(ByProtocol, &Ip4Config2Guid, NULL, &HandleCount, &HandleBuffer);
	if (EFI_ERROR(Status) || HandleCount == 0)
		return FALSE;

	// Open the protocol on the first handle
	EFI_IP4_CONFIG2_PROTOCOL *Ip4Config2 = NULL;
	Status = bs->OpenProtocol(
		HandleBuffer[0],
		&Ip4Config2Guid,
		(PVOID *)&Ip4Config2,
		ImageHandle,
		NULL,
		EFI_OPEN_PROTOCOL_GET_PROTOCOL);

	bs->FreePool(HandleBuffer);

	if (EFI_ERROR(Status) || Ip4Config2 == NULL)
		return FALSE;

	// Set policy to static
	EFI_IP4_CONFIG2_POLICY Policy = Ip4Config2PolicyStatic;
	Status = Ip4Config2->SetData(Ip4Config2, Ip4Config2DataTypePolicy, sizeof(Policy), &Policy);
	if (EFI_ERROR(Status))
		return FALSE;

	// Set static IP address (10.0.2.15/255.255.255.0) for QEMU
	EFI_IP4_CONFIG2_MANUAL_ADDRESS ManualAddr;
	ManualAddr.Address.Addr[0] = 10;
	ManualAddr.Address.Addr[1] = 0;
	ManualAddr.Address.Addr[2] = 2;
	ManualAddr.Address.Addr[3] = 15;
	ManualAddr.SubnetMask.Addr[0] = 255;
	ManualAddr.SubnetMask.Addr[1] = 255;
	ManualAddr.SubnetMask.Addr[2] = 255;
	ManualAddr.SubnetMask.Addr[3] = 0;

	Status = Ip4Config2->SetData(Ip4Config2, Ip4Config2DataTypeManualAddress, sizeof(ManualAddr), &ManualAddr);
	if (EFI_ERROR(Status))
		return FALSE;

	// Set gateway (10.0.2.2) for QEMU
	EFI_IPv4_ADDRESS Gateway;
	Gateway.Addr[0] = 10;
	Gateway.Addr[1] = 0;
	Gateway.Addr[2] = 2;
	Gateway.Addr[3] = 2;

	Status = Ip4Config2->SetData(Ip4Config2, Ip4Config2DataTypeGateway, sizeof(Gateway), &Gateway);
	// Gateway might fail if not supported, continue anyway

	return TRUE;
}

// Wait for an event with timeout (in 100ns units)
static EFI_STATUS WaitForEventWithTimeout(EFI_BOOT_SERVICES *bs, EFI_EVENT Event, UINT64 Timeout100ns)
{
	// Create a timer event
	EFI_EVENT TimerEvent;
	EFI_STATUS Status = bs->CreateEvent(EVT_TIMER, 0, NULL, NULL, &TimerEvent);
	if (EFI_ERROR(Status))
		return Status;

	// Set timer
	Status = bs->SetTimer(TimerEvent, TimerRelative, Timeout100ns);
	if (EFI_ERROR(Status))
	{
		bs->CloseEvent(TimerEvent);
		return Status;
	}

	// Wait for either event
	EFI_EVENT Events[2] = {Event, TimerEvent};
	USIZE Index;
	Status = bs->WaitForEvent(2, Events, &Index);

	bs->CloseEvent(TimerEvent);

	if (EFI_ERROR(Status))
		return Status;

	// Index 1 means timer fired (timeout)
	if (Index == 1)
		return EFI_TIMEOUT;

	return EFI_SUCCESS;
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
	if (EFI_ERROR(Status) || sockCtx == NULL)
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
	if (EFI_ERROR(Status) || HandleCount == 0)
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

	if (EFI_ERROR(Status))
	{
		bs->FreePool(sockCtx);
		return;
	}

	// Create TCP child handle
	sockCtx->TcpHandle = NULL;
	Status = sockCtx->ServiceBinding->CreateChild(sockCtx->ServiceBinding, &sockCtx->TcpHandle);
	if (EFI_ERROR(Status))
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

	if (EFI_ERROR(Status))
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
		ConfigData.AccessPoint.StationPort = 0; // Ephemeral port
		ConfigData.AccessPoint.ActiveFlag = TRUE;

		// Set remote IPv6 address
		const UINT8 *ipv6Addr = ip.ToIPv6();
		if (ipv6Addr != NULL)
		{
			Memory::Copy(ConfigData.AccessPoint.RemoteAddress.Addr, ipv6Addr, 16);
		}
		ConfigData.AccessPoint.RemotePort = port;

		ConfigData.ControlOption = NULL;

		// Configure the TCP6 protocol
		Status = sockCtx->Tcp6->Configure(sockCtx->Tcp6, &ConfigData);
		if (EFI_ERROR(Status))
			return FALSE;

		sockCtx->IsConfigured = TRUE;

		// Create event for connection completion
		EFI_EVENT ConnectEvent;
		Status = bs->CreateEvent(EVT_NOTIFY_SIGNAL, TPL_CALLBACK, EmptyNotify, NULL, &ConnectEvent);
		if (EFI_ERROR(Status))
			return FALSE;

		// Setup connection token
		EFI_TCP6_CONNECTION_TOKEN ConnectToken;
		Memory::Zero(&ConnectToken, sizeof(ConnectToken));
		ConnectToken.CompletionToken.Event = ConnectEvent;
		ConnectToken.CompletionToken.Status = EFI_NOT_READY;

		// Initiate connection
		Status = sockCtx->Tcp6->Connect(sockCtx->Tcp6, &ConnectToken);
		if (EFI_ERROR(Status) && Status != EFI_NOT_READY)
		{
			bs->CloseEvent(ConnectEvent);
			return FALSE;
		}

		// Wait for connection (30 second timeout)
		Status = WaitForEventWithTimeout(bs, ConnectEvent, 30ULL * 10000000ULL);
		bs->CloseEvent(ConnectEvent);

		if (EFI_ERROR(Status))
			return FALSE;

		if (EFI_ERROR(ConnectToken.CompletionToken.Status))
			return FALSE;
	}
	else
	{
		// Configure TCP4 connection
		EFI_TCP4_CONFIG_DATA ConfigData;
		Memory::Zero(&ConfigData, sizeof(ConfigData));

		ConfigData.TypeOfService = 0;
		ConfigData.TimeToLive = 64;
		ConfigData.AccessPoint.StationPort = 0; // Ephemeral port
		ConfigData.AccessPoint.ActiveFlag = TRUE;
		ConfigData.ControlOption = NULL;

		// Set remote IPv4 address
		UINT32 ipv4Addr = ip.ToIPv4();
		ConfigData.AccessPoint.RemoteAddress.Addr[0] = (UINT8)(ipv4Addr & 0xFF);
		ConfigData.AccessPoint.RemoteAddress.Addr[1] = (UINT8)((ipv4Addr >> 8) & 0xFF);
		ConfigData.AccessPoint.RemoteAddress.Addr[2] = (UINT8)((ipv4Addr >> 16) & 0xFF);
		ConfigData.AccessPoint.RemoteAddress.Addr[3] = (UINT8)((ipv4Addr >> 24) & 0xFF);
		ConfigData.AccessPoint.RemotePort = port;

		// Try UseDefaultAddress first (works on real hardware with DHCP)
		ConfigData.AccessPoint.UseDefaultAddress = TRUE;
		Status = sockCtx->Tcp4->Configure(sockCtx->Tcp4, &ConfigData);

		if (EFI_ERROR(Status))
		{
			// Configure IP4 layer with static QEMU settings
			ConfigureStaticIP4(bs, ctx->ImageHandle);

			// Retry with default address (now uses our static config)
			Status = sockCtx->Tcp4->Configure(sockCtx->Tcp4, &ConfigData);
		}

		if (EFI_ERROR(Status))
		{
			// Last resort: set address directly in TCP4 config
			// QEMU uses: Guest=10.0.2.15, Gateway=10.0.2.2
			ConfigData.AccessPoint.UseDefaultAddress = FALSE;
			ConfigData.AccessPoint.StationAddress.Addr[0] = 10;
			ConfigData.AccessPoint.StationAddress.Addr[1] = 0;
			ConfigData.AccessPoint.StationAddress.Addr[2] = 2;
			ConfigData.AccessPoint.StationAddress.Addr[3] = 15;
			ConfigData.AccessPoint.SubnetMask.Addr[0] = 255;
			ConfigData.AccessPoint.SubnetMask.Addr[1] = 255;
			ConfigData.AccessPoint.SubnetMask.Addr[2] = 255;
			ConfigData.AccessPoint.SubnetMask.Addr[3] = 0;

			Status = sockCtx->Tcp4->Configure(sockCtx->Tcp4, &ConfigData);
			if (EFI_ERROR(Status))
				return FALSE;

			// Add default route via QEMU gateway for external network access
			EFI_IPv4_ADDRESS DefaultSubnet = {{0, 0, 0, 0}};
			EFI_IPv4_ADDRESS DefaultMask = {{0, 0, 0, 0}};
			EFI_IPv4_ADDRESS Gateway = {{10, 0, 2, 2}};
			sockCtx->Tcp4->Routes(sockCtx->Tcp4, FALSE, &DefaultSubnet, &DefaultMask, &Gateway);
		}

		sockCtx->IsConfigured = TRUE;

		// Create event for connection completion
		EFI_EVENT ConnectEvent;
		Status = bs->CreateEvent(EVT_NOTIFY_SIGNAL, TPL_CALLBACK, EmptyNotify, NULL, &ConnectEvent);
		if (EFI_ERROR(Status))
			return FALSE;

		// Setup connection token
		EFI_TCP4_CONNECTION_TOKEN ConnectToken;
		Memory::Zero(&ConnectToken, sizeof(ConnectToken));
		ConnectToken.CompletionToken.Event = ConnectEvent;
		ConnectToken.CompletionToken.Status = EFI_NOT_READY;

		// Initiate connection
		Status = sockCtx->Tcp4->Connect(sockCtx->Tcp4, &ConnectToken);
		if (EFI_ERROR(Status) && Status != EFI_NOT_READY)
		{
			bs->CloseEvent(ConnectEvent);
			return FALSE;
		}

		// Wait for connection (30 second timeout)
		Status = WaitForEventWithTimeout(bs, ConnectEvent, 30ULL * 10000000ULL);
		bs->CloseEvent(ConnectEvent);

		if (EFI_ERROR(Status))
			return FALSE;

		if (EFI_ERROR(ConnectToken.CompletionToken.Status))
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
		// Create close event
		EFI_EVENT CloseEvent;
		EFI_STATUS Status = bs->CreateEvent(EVT_NOTIFY_SIGNAL, TPL_CALLBACK, EmptyNotify, NULL, &CloseEvent);
		if (!EFI_ERROR(Status))
		{
			if (sockCtx->IsIPv6)
			{
				EFI_TCP6_CLOSE_TOKEN CloseToken;
				Memory::Zero(&CloseToken, sizeof(CloseToken));
				CloseToken.CompletionToken.Event = CloseEvent;
				CloseToken.AbortOnClose = FALSE;

				Status = sockCtx->Tcp6->Close(sockCtx->Tcp6, &CloseToken);
				if (!EFI_ERROR(Status) || Status == EFI_NOT_READY)
				{
					WaitForEventWithTimeout(bs, CloseEvent, 5ULL * 10000000ULL);
				}

				// Reset configuration
				sockCtx->Tcp6->Configure(sockCtx->Tcp6, NULL);
			}
			else
			{
				EFI_TCP4_CLOSE_TOKEN CloseToken;
				Memory::Zero(&CloseToken, sizeof(CloseToken));
				CloseToken.CompletionToken.Event = CloseEvent;
				CloseToken.AbortOnClose = FALSE;

				Status = sockCtx->Tcp4->Close(sockCtx->Tcp4, &CloseToken);
				if (!EFI_ERROR(Status) || Status == EFI_NOT_READY)
				{
					WaitForEventWithTimeout(bs, CloseEvent, 5ULL * 10000000ULL);
				}

				// Reset configuration
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

	// Free context
	bs->FreePool(sockCtx);
	m_socket = NULL;

	return TRUE;
}

// =============================================================================
// Bind (not used for active connections)
// =============================================================================

BOOL Socket::Bind(SockAddr *SocketAddress, INT32 ShareType)
{
	(VOID)SocketAddress;
	(VOID)ShareType;
	// Bind is handled during Configure for UEFI TCP
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

	// Create receive event
	EFI_EVENT RxEvent;
	Status = bs->CreateEvent(EVT_NOTIFY_SIGNAL, TPL_CALLBACK, EmptyNotify, NULL, &RxEvent);
	if (EFI_ERROR(Status))
		return -1;

	if (sockCtx->IsIPv6)
	{
		// Allocate receive data structure
		EFI_TCP6_RECEIVE_DATA *RxData = NULL;
		Status = bs->AllocatePool(EfiLoaderData, sizeof(EFI_TCP6_RECEIVE_DATA), (PVOID *)&RxData);
		if (EFI_ERROR(Status))
		{
			bs->CloseEvent(RxEvent);
			return -1;
		}

		Memory::Zero(RxData, sizeof(EFI_TCP6_RECEIVE_DATA));
		RxData->DataLength = bufferLength;
		RxData->FragmentCount = 1;
		RxData->FragmentTable[0].FragmentLength = bufferLength;
		RxData->FragmentTable[0].FragmentBuffer = buffer;

		// Setup I/O token
		EFI_TCP6_IO_TOKEN RxToken;
		Memory::Zero(&RxToken, sizeof(RxToken));
		RxToken.CompletionToken.Event = RxEvent;
		RxToken.CompletionToken.Status = EFI_NOT_READY;
		RxToken.Packet.RxData = RxData;

		// Initiate receive
		Status = sockCtx->Tcp6->Receive(sockCtx->Tcp6, &RxToken);
		if (!EFI_ERROR(Status) || Status == EFI_NOT_READY)
		{
			// Wait for receive (60 second timeout)
			Status = WaitForEventWithTimeout(bs, RxEvent, 60ULL * 10000000ULL);
			if (!EFI_ERROR(Status) && !EFI_ERROR(RxToken.CompletionToken.Status))
			{
				bytesRead = (SSIZE)RxData->DataLength;
			}
		}

		bs->FreePool(RxData);
	}
	else
	{
		// Allocate receive data structure
		EFI_TCP4_RECEIVE_DATA *RxData = NULL;
		Status = bs->AllocatePool(EfiLoaderData, sizeof(EFI_TCP4_RECEIVE_DATA), (PVOID *)&RxData);
		if (EFI_ERROR(Status))
		{
			bs->CloseEvent(RxEvent);
			return -1;
		}

		Memory::Zero(RxData, sizeof(EFI_TCP4_RECEIVE_DATA));
		RxData->DataLength = bufferLength;
		RxData->FragmentCount = 1;
		RxData->FragmentTable[0].FragmentLength = bufferLength;
		RxData->FragmentTable[0].FragmentBuffer = buffer;

		// Setup I/O token
		EFI_TCP4_IO_TOKEN RxToken;
		Memory::Zero(&RxToken, sizeof(RxToken));
		RxToken.CompletionToken.Event = RxEvent;
		RxToken.CompletionToken.Status = EFI_NOT_READY;
		RxToken.Packet.RxData = RxData;

		// Initiate receive
		Status = sockCtx->Tcp4->Receive(sockCtx->Tcp4, &RxToken);
		if (!EFI_ERROR(Status) || Status == EFI_NOT_READY)
		{
			// Wait for receive (60 second timeout)
			Status = WaitForEventWithTimeout(bs, RxEvent, 60ULL * 10000000ULL);
			if (!EFI_ERROR(Status) && !EFI_ERROR(RxToken.CompletionToken.Status))
			{
				bytesRead = (SSIZE)RxData->DataLength;
			}
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

	// Create transmit event
	EFI_EVENT TxEvent;
	Status = bs->CreateEvent(EVT_NOTIFY_SIGNAL, TPL_CALLBACK, EmptyNotify, NULL, &TxEvent);
	if (EFI_ERROR(Status))
		return 0;

	if (sockCtx->IsIPv6)
	{
		// Allocate transmit data structure
		EFI_TCP6_TRANSMIT_DATA *TxData = NULL;
		Status = bs->AllocatePool(EfiLoaderData, sizeof(EFI_TCP6_TRANSMIT_DATA), (PVOID *)&TxData);
		if (EFI_ERROR(Status))
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

		// Setup I/O token
		EFI_TCP6_IO_TOKEN TxToken;
		Memory::Zero(&TxToken, sizeof(TxToken));
		TxToken.CompletionToken.Event = TxEvent;
		TxToken.CompletionToken.Status = EFI_NOT_READY;
		TxToken.Packet.TxData = TxData;

		// Initiate transmit
		Status = sockCtx->Tcp6->Transmit(sockCtx->Tcp6, &TxToken);
		if (!EFI_ERROR(Status) || Status == EFI_NOT_READY)
		{
			// Wait for transmit (30 second timeout)
			Status = WaitForEventWithTimeout(bs, TxEvent, 30ULL * 10000000ULL);
			if (!EFI_ERROR(Status) && !EFI_ERROR(TxToken.CompletionToken.Status))
			{
				bytesSent = bufferLength;
			}
		}

		bs->FreePool(TxData);
	}
	else
	{
		// Allocate transmit data structure
		EFI_TCP4_TRANSMIT_DATA *TxData = NULL;
		Status = bs->AllocatePool(EfiLoaderData, sizeof(EFI_TCP4_TRANSMIT_DATA), (PVOID *)&TxData);
		if (EFI_ERROR(Status))
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

		// Setup I/O token
		EFI_TCP4_IO_TOKEN TxToken;
		Memory::Zero(&TxToken, sizeof(TxToken));
		TxToken.CompletionToken.Event = TxEvent;
		TxToken.CompletionToken.Status = EFI_NOT_READY;
		TxToken.Packet.TxData = TxData;

		// Initiate transmit
		Status = sockCtx->Tcp4->Transmit(sockCtx->Tcp4, &TxToken);
		if (!EFI_ERROR(Status) || Status == EFI_NOT_READY)
		{
			// Wait for transmit (30 second timeout)
			Status = WaitForEventWithTimeout(bs, TxEvent, 30ULL * 10000000ULL);
			if (!EFI_ERROR(Status) && !EFI_ERROR(TxToken.CompletionToken.Status))
			{
				bytesSent = bufferLength;
			}
		}

		bs->FreePool(TxData);
	}

	bs->CloseEvent(TxEvent);
	return bytesSent;
}
