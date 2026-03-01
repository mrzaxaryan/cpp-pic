#include "core/types/ip_address.h"
#include "core/memory/memory.h"
#include "core/string/string.h"

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
						UINT32 value = String::ParseHex(Span<const CHAR>(hexBuffer, hexIndex));
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
					UINT32 value = String::ParseHex(Span<const CHAR>(hexBuffer, hexIndex));
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
			UINT32 value = String::ParseHex(Span<const CHAR>(hexBuffer, hexIndex));
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

				auto octetResult = String::ParseInt64(currentOctet);
				if (!octetResult)
				{
					return Result<IPAddress, Error>::Err(Error::IpAddress_ParseFailed);
				}
				auto& octet = octetResult.Value();
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
			USIZE len = String::WriteDecimal(Span<CHAR>(temp), octets[i]);
			Memory::Copy(&buffer[offset], temp, len);
			offset += (UINT32)len;
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
			USIZE hexLen = String::WriteHex(Span<CHAR>(hexStr), group);
			Memory::Copy(&buffer[offset], hexStr, hexLen);
			offset += (UINT32)hexLen;
		}
		buffer[offset] = '\0';
		return Result<void, Error>::Ok();
	}

	return Result<void, Error>::Err(Error::IpAddress_ToStringFailed);
}
