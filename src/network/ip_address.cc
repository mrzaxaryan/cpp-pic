#include "ip_address.h"
#include "memory.h"
#include "string.h"

// Default constructor - creates an invalid IP address
IPAddress::IPAddress() : version(IPVersion::Invalid)
{
    Memory::Zero(&address, sizeof(address));
}

// Constructor from IPv4 address
IPAddress::IPAddress(UINT32 ipv4Address) : version(IPVersion::IPv4)
{
    address.ipv4 = ipv4Address;
}

// Constructor from IPv6 address
IPAddress::IPAddress(const UINT8 ipv6Address[16]) : version(IPVersion::IPv6)
{
    Memory::Copy(address.ipv6, ipv6Address, 16);
}

// Copy constructor
IPAddress::IPAddress(const IPAddress &other) : version(other.version)
{
    if (version == IPVersion::IPv4)
    {
        address.ipv4 = other.address.ipv4;
    }
    else if (version == IPVersion::IPv6)
    {
        Memory::Copy(address.ipv6, other.address.ipv6, 16);
    }
}

// Static factory method for IPv4
IPAddress IPAddress::FromIPv4(UINT32 ipv4Address)
{
    return IPAddress(ipv4Address);
}

// Static factory method for IPv6
IPAddress IPAddress::FromIPv6(const UINT8 ipv6Address[16])
{
    return IPAddress(ipv6Address);
}

IPAddress IPAddress::LocalHost(BOOL ipv6)
{
    if (ipv6)
    {
        // IPv6 loopback is ::1 (15 bytes of 0, 1 byte of 1)
        UINT8 loopbackV6[16]{};
        loopbackV6[15] = 1;
        return IPAddress::FromIPv6(loopbackV6);
    }
    else
    {
        // IPv4 loopback is 127.0.0.1
        // Using your specific hex format: 0x0100007F
        return IPAddress::FromIPv4(0x0100007F);
    }
}

// Static factory method for invalid IP
IPAddress IPAddress::Invalid()
{
    return IPAddress();
}

// Convert string to IPAddress (supports both IPv4 and IPv6)
IPAddress IPAddress::FromString(PCCHAR ipString)
{
    if (ipString == nullptr)
    {
        return Invalid();
    }

    // Check if it's IPv6 (contains ':')
    PCCHAR pChar = ipString;
    BOOL hasColon = false;
    while (*pChar != '\0')
    {
        if (*pChar == ':')
        {
            hasColon = true;
            break;
        }
        pChar++;
    }

    if (hasColon)
    {
        // Parse IPv6 address
        UINT8 ipv6[16];
        Memory::Zero(ipv6, 16);

        // IPv6 parsing logic
        UINT32 groupIndex = 0;
        UINT32 doubleColonPos = 0xFFFFFFFF;
        BOOL foundDoubleColon = false;
        PCCHAR pCurrent = ipString;
        CHAR hexBuffer[5];
        UINT32 hexIndex = 0;

        while (*pCurrent != '\0' && groupIndex < 8)
        {
            if (*pCurrent == ':')
            {
                if (*(pCurrent + 1) == ':' && !foundDoubleColon)
                {
                    // Flush accumulated hex digits before :: as a separate group
                    if (hexIndex > 0 && groupIndex < 8)
                    {
                        hexBuffer[hexIndex] = '\0';
                        UINT32 value = String::ParseHex(hexBuffer);
                        ipv6[groupIndex * 2] = (UINT8)(value >> 8);
                        ipv6[groupIndex * 2 + 1] = (UINT8)(value & 0xFF);
                        groupIndex++;
                        hexIndex = 0;
                    }
                    // Handle double colon
                    foundDoubleColon = true;
                    doubleColonPos = groupIndex;
                    pCurrent += 2;
                    if (*pCurrent == '\0')
                        break;
                    continue;
                }
                else if (hexIndex > 0)
                {
                    // Process accumulated hex digits
                    hexBuffer[hexIndex] = '\0';
                    UINT32 value = String::ParseHex(hexBuffer);
                    ipv6[groupIndex * 2] = (UINT8)(value >> 8);
                    ipv6[groupIndex * 2 + 1] = (UINT8)(value & 0xFF);
                    groupIndex++;
                    hexIndex = 0;
                    pCurrent++;
                }
                else
                {
                    pCurrent++;
                }
            }
            else if ((*pCurrent >= '0' && *pCurrent <= '9') ||
                     (*pCurrent >= 'a' && *pCurrent <= 'f') ||
                     (*pCurrent >= 'A' && *pCurrent <= 'F'))
            {
                if (hexIndex < 4)
                {
                    hexBuffer[hexIndex++] = *pCurrent;
                }
                pCurrent++;
            }
            else
            {
                // Invalid character
                return Invalid();
            }
        }

        // Process final group if any
        if (hexIndex > 0 && groupIndex < 8)
        {
            hexBuffer[hexIndex] = '\0';
            UINT32 value = String::ParseHex(hexBuffer);
            ipv6[groupIndex * 2] = (UINT8)(value >> 8);
            ipv6[groupIndex * 2 + 1] = (UINT8)(value & 0xFF);
            groupIndex++;
        }

        // Handle double colon expansion
        if (foundDoubleColon && groupIndex < 8)
        {
            UINT32 tailGroups = groupIndex - doubleColonPos;

            // Move tail groups to the end
            if (tailGroups > 0)
            {
                for (UINT32 i = 0; i < tailGroups; i++)
                {
                    UINT32 srcIdx = (groupIndex - tailGroups + i) * 2;
                    UINT32 dstIdx = (8 - tailGroups + i) * 2;
                    ipv6[dstIdx] = ipv6[srcIdx];
                    ipv6[dstIdx + 1] = ipv6[srcIdx + 1];
                    ipv6[srcIdx] = 0;
                    ipv6[srcIdx + 1] = 0;
                }
            }
        }

        return IPAddress(ipv6);
    }
    else
    {
        // Parse IPv4 address (reuse old ConvertIP logic)
        CHAR szCurrOctet[8];
        UINT32 dwCurrOctetIndex = 0;
        UINT32 dwCompletedOctetCount = 0;
        UINT32 dwEndOfOctet = 0;
        UINT32 dwEndOfString = 0;
        INT64 dwOctet = 0;
        UINT8 bOctets[4];
        UINT32 dwAddr = 0;

        Memory::Zero(szCurrOctet, sizeof(szCurrOctet));
        dwCurrOctetIndex = 0;
        PCCHAR pCurrByte = ipString;

        for (;;)
        {
            dwEndOfOctet = 0;
            if (*pCurrByte == '\0')
            {
                dwEndOfOctet = 1;
                dwEndOfString = 1;
            }
            else if (*pCurrByte == '.')
            {
                dwEndOfOctet = 1;
            }
            else
            {
                if (*pCurrByte >= '0' && *pCurrByte <= '9')
                {
                    if (dwCurrOctetIndex > 2)
                    {
                        return Invalid();
                    }
                    szCurrOctet[dwCurrOctetIndex] = *pCurrByte;
                    dwCurrOctetIndex++;
                }
                else
                {
                    return Invalid();
                }
            }

            if (dwEndOfOctet != 0)
            {
                if (dwCurrOctetIndex == 0)
                {
                    return Invalid();
                }

                dwOctet = String::ParseInt64(szCurrOctet);
                if (dwOctet > 255)
                {
                    return Invalid();
                }

                if (dwCompletedOctetCount >= 4)
                {
                    return Invalid();
                }

                bOctets[dwCompletedOctetCount] = (UINT8)dwOctet;
                dwCompletedOctetCount++;

                if (dwEndOfString != 0)
                {
                    break;
                }

                Memory::Zero(szCurrOctet, sizeof(szCurrOctet));
                dwCurrOctetIndex = 0;
            }

            pCurrByte++;
        }

        if (dwCompletedOctetCount != 4)
        {
            return Invalid();
        }

        Memory::Copy((PVOID)&dwAddr, bOctets, 4);
        return IPAddress(dwAddr);
    }
}

// Check if IP address is valid
BOOL IPAddress::IsValid() const
{
    return version != IPVersion::Invalid;
}

// Check if IP address is IPv4
BOOL IPAddress::IsIPv4() const
{
    return version == IPVersion::IPv4;
}

// Check if IP address is IPv6
BOOL IPAddress::IsIPv6() const
{
    return version == IPVersion::IPv6;
}

// Get IP version
IPVersion IPAddress::GetVersion() const
{
    return version;
}

// Convert to IPv4 (returns 0xFFFFFFFF if not IPv4)
UINT32 IPAddress::ToIPv4() const
{
    if (version == IPVersion::IPv4)
    {
        return address.ipv4;
    }
    return 0xFFFFFFFF; // INVALID_IPV4
}

// Get IPv6 address (returns nullptr if not IPv6)
const UINT8 *IPAddress::ToIPv6() const
{
    if (version == IPVersion::IPv6)
    {
        return address.ipv6;
    }
    return nullptr;
}

// Convert IP address to string
Result<void, Error> IPAddress::ToString(PCHAR buffer, UINT32 bufferSize) const
{
	if (buffer == nullptr || bufferSize == 0)
	{
		return Result<void, Error>::Err(Error::IpAddress_ToStringFailed);
	}

	if (version == IPVersion::IPv4)
	{
		// Convert IPv4 to string
		if (bufferSize < 16) // Minimum size for "255.255.255.255\0"
		{
			return Result<void, Error>::Err(Error::IpAddress_ToStringFailed);
		}

		UINT8 octets[4];
		Memory::Copy(octets, &address.ipv4, 4);

		UINT32 offset = 0;
		for (UINT32 i = 0; i < 4; i++)
		{
			if (i > 0)
			{
				buffer[offset++] = '.';
			}
			CHAR temp[4];
			String::WriteDecimal(temp, octets[i]);
			UINT32 len = String::Length(temp);
			Memory::Copy(buffer + offset, temp, len);
			offset += len;
		}
		buffer[offset] = '\0';
		return Result<void, Error>::Ok();
	}
	else if (version == IPVersion::IPv6)
	{
		// Convert IPv6 to string (simplified format)
		if (bufferSize < 40) // Minimum size for full IPv6 address
		{
			return Result<void, Error>::Err(Error::IpAddress_ToStringFailed);
		}

		UINT32 offset = 0;
		for (UINT32 i = 0; i < 8; i++)
		{
			if (i > 0)
			{
				buffer[offset++] = ':';
			}
			UINT16 group = ((UINT16)address.ipv6[i * 2] << 8) | address.ipv6[i * 2 + 1];

			// Convert to hex
			CHAR hexStr[5];
			String::WriteHex(hexStr, group);
			UINT32 hexLen = String::Length(hexStr);
			Memory::Copy(buffer + offset, hexStr, hexLen);
			offset += hexLen;
		}
		buffer[offset] = '\0';
		return Result<void, Error>::Ok();
	}

	return Result<void, Error>::Err(Error::IpAddress_ToStringFailed);
}

// Equality operator
BOOL IPAddress::operator==(const IPAddress &other) const
{
    if (version != other.version)
    {
        return false;
    }

    if (version == IPVersion::IPv4)
    {
        return address.ipv4 == other.address.ipv4;
    }
    else if (version == IPVersion::IPv6)
    {
        return Memory::Compare(address.ipv6, other.address.ipv6, 16) == 0;
    }

    return version == IPVersion::Invalid;
}

// Inequality operator
BOOL IPAddress::operator!=(const IPAddress &other) const
{
    return !(*this == other);
}

// Assignment operator
IPAddress &IPAddress::operator=(const IPAddress &other)
{
    if (this != &other)
    {
        version = other.version;
        if (version == IPVersion::IPv4)
        {
            address.ipv4 = other.address.ipv4;
        }
        else if (version == IPVersion::IPv6)
        {
            Memory::Copy(address.ipv6, other.address.ipv6, 16);
        }
    }
    return *this;
}
