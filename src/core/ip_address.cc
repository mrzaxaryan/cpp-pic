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
IPAddress::IPAddress(const UINT8 (&ipv6Address)[16]) : version(IPVersion::IPv6)
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
IPAddress IPAddress::FromIPv6(const UINT8 (&ipv6Address)[16])
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
Result<IPAddress, Error> IPAddress::FromString(PCCHAR ipString)
{
	if (ipString == nullptr)
	{
		return Result<IPAddress, Error>::Err(Error::IpAddress_ParseFailed);
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
				return Result<IPAddress, Error>::Err(Error::IpAddress_ParseFailed);
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

		return Result<IPAddress, Error>::Ok(IPAddress(ipv6));
	}
	else
	{
		// Parse IPv4 address
		CHAR currentOctet[8];
		UINT32 currentOctetIndex = 0;
		UINT32 completedOctetCount = 0;
		UINT32 endOfOctet = 0;
		UINT32 endOfString = 0;
		INT64 octet = 0;
		UINT8 octets[4];
		UINT32 addr = 0;

		Memory::Zero(currentOctet, sizeof(currentOctet));
		currentOctetIndex = 0;
		PCCHAR currentByte = ipString;

		for (;;)
		{
			endOfOctet = 0;
			if (*currentByte == '\0')
			{
				endOfOctet = 1;
				endOfString = 1;
			}
			else if (*currentByte == '.')
			{
				endOfOctet = 1;
			}
			else
			{
				if (*currentByte >= '0' && *currentByte <= '9')
				{
					if (currentOctetIndex > 2)
					{
						return Result<IPAddress, Error>::Err(Error::IpAddress_ParseFailed);
					}
					currentOctet[currentOctetIndex] = *currentByte;
					currentOctetIndex++;
				}
				else
				{
					return Result<IPAddress, Error>::Err(Error::IpAddress_ParseFailed);
				}
			}

			if (endOfOctet != 0)
			{
				if (currentOctetIndex == 0)
				{
					return Result<IPAddress, Error>::Err(Error::IpAddress_ParseFailed);
				}

				octet = String::ParseInt64(currentOctet);
				if (octet > 255)
				{
					return Result<IPAddress, Error>::Err(Error::IpAddress_ParseFailed);
				}

				if (completedOctetCount >= 4)
				{
					return Result<IPAddress, Error>::Err(Error::IpAddress_ParseFailed);
				}

				octets[completedOctetCount] = (UINT8)octet;
				completedOctetCount++;

				if (endOfString != 0)
				{
					break;
				}

				Memory::Zero(currentOctet, sizeof(currentOctet));
				currentOctetIndex = 0;
			}

			currentByte++;
		}

		if (completedOctetCount != 4)
		{
			return Result<IPAddress, Error>::Err(Error::IpAddress_ParseFailed);
		}

		Memory::Copy((PVOID)&addr, octets, 4);
		return Result<IPAddress, Error>::Ok(IPAddress(addr));
	}
}

// Convert IP address to string
Result<void, Error> IPAddress::ToString(Span<CHAR> buffer) const
{
	if (buffer.Size() == 0)
	{
		return Result<void, Error>::Err(Error::IpAddress_ToStringFailed);
	}

	if (version == IPVersion::IPv4)
	{
		// Convert IPv4 to string
		if (buffer.Size() < 16) // Minimum size for "255.255.255.255\0"
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
			Memory::Copy(&buffer[offset], temp, len);
			offset += len;
		}
		buffer[offset] = '\0';
		return Result<void, Error>::Ok();
	}
	else if (version == IPVersion::IPv6)
	{
		// Convert IPv6 to string (simplified format)
		if (buffer.Size() < 40) // Minimum size for full IPv6 address
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
			Memory::Copy(&buffer[offset], hexStr, hexLen);
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
