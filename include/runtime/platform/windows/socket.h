#if defined(PLATFORM_WINDOWS)
#pragma once

#include "primitives.h"
#include "afd.h"
#include "ntdll.h"
#include "memory.h"

// =============================================================================
// Socket Class - Windows AFD-based Network Socket Implementation
// =============================================================================
// This class provides a lightweight, CRT-free socket implementation using
// Windows Ancillary Function Driver (AFD) directly through NtDeviceIoControlFile.
// It supports TCP connections without relying on winsock2 or standard libraries.
// =============================================================================

class Socket
{
private:
	PVOID m_socket;  // Handle to the AFD socket
	UINT32 m_ip;     // IP address in network byte order
	UINT16 m_port;   // Port number in host byte order

	// Internal helper: Bind socket to a local address
	BOOL Bind(PSockAddr SocketAddress, UINT32 ShareType);

	// Internal helper: Create event object for async operations
	NTSTATUS CreateSocketEvent(PVOID *pSockEvent);

	// Internal helper: Wait for async operation with timeout
	NTSTATUS WaitForOperation(PVOID SockEvent, PIO_STATUS_BLOCK pIOSB, PLARGE_INTEGER pTimeout);

public:
	// Default constructor: Creates an uninitialized socket
	Socket() : m_socket(NULL), m_ip(0), m_port(0) {}

	// Constructor: Creates a new AFD socket handle
	// Parameters:
	//   ip - Target IP address (network byte order)
	//   port - Target port number (host byte order)
	Socket(UINT32 ip, UINT16 port);

	// Destructor: Cleanup is handled by Close()
	~Socket() = default;

	// Connect to the remote server (combines socket creation, bind, and connect)
	// Returns: TRUE on success, FALSE on failure
	BOOL Open();

	// Close and cleanup the socket connection
	// Returns: TRUE on success, FALSE on failure
	BOOL Close();

	// Read data from the socket
	// Parameters:
	//   buffer - Destination buffer for received data
	//   bufferSize - Size of the buffer in bytes
	// Returns: Number of bytes read, or -1 on error/timeout
	SSIZE Read(PVOID buffer, UINT32 bufferSize);

	// Write data to the socket
	// Parameters:
	//   buffer - Source buffer containing data to send
	//   bufferLength - Number of bytes to send
	// Returns: Number of bytes sent, or 0 on failure
	UINT32 Write(PCVOID buffer, UINT32 bufferLength);

	// Check if socket is initialized
	FORCE_INLINE BOOL IsValid() const { return m_socket != NULL; }

	// Get the socket handle
	FORCE_INLINE PVOID GetHandle() const { return m_socket; }
};

#endif // PLATFORM_WINDOWS
