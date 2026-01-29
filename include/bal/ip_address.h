#pragma once

#include "primitives.h"

// IP address version enum
enum class IPVersion : UINT8
{
    IPv4 = 4,
    IPv6 = 6,
    Invalid = 0
};

// IPAddress class supporting both IPv4 and IPv6
class IPAddress
{
private:
    IPVersion version;
    union
    {
        UINT32 ipv4;        // IPv4 address (4 bytes)
        UINT8 ipv6[16];     // IPv6 address (16 bytes)
    } address;

private:
    // Private constructors - use static factory methods instead
    IPAddress(UINT32 ipv4Address);
    IPAddress(const UINT8 ipv6Address[16]);

public:
    // Default constructor - creates an invalid IP address
    IPAddress();

    // Copy constructor
    IPAddress(const IPAddress& other);

    // Static factory methods
    static IPAddress FromIPv4(UINT32 ipv4Address);
    static IPAddress FromIPv6(const UINT8 ipv6Address[16]);
    static IPAddress FromString(PCCHAR ipString);
    static IPAddress Invalid();

    // Validation
    BOOL IsValid() const;
    BOOL IsIPv4() const;
    BOOL IsIPv6() const;
    IPVersion GetVersion() const;

    // Conversion methods
    UINT32 ToIPv4() const;
    const UINT8* ToIPv6() const;
    BOOL ToString(PCHAR buffer, UINT32 bufferSize) const;

    // Comparison operators
    BOOL operator==(const IPAddress& other) const;
    BOOL operator!=(const IPAddress& other) const;

    // Assignment operator
    IPAddress& operator=(const IPAddress& other);
};
