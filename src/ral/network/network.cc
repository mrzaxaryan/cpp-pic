#include "primitives.h"
#include "network.h"
#include "memory.h"
#include "string.h"

// Convert an IP address from string format to a 32-bit unsigned integer.
UINT32 ConvertIP(PCCHAR pIP)
{
    CHAR szCurrOctet[8];              // Buffer to hold the current octet string
    UINT32 dwCurrOctetIndex = 0;      // Index for the current octet in the string
    UINT32 dwCompletedOctetCount = 0; // Count of completed octets
    UINT32 dwEndOfOctet = 0;          // Flag to indicate the end of the current octet
    UINT32 dwEndOfString = 0;         // Flag to indicate the end of the IP string
    UINT32 dwOctet = 0;               // Variable to hold the current octet value as an integer
    UINT8 bOctets[4];                 // Array to hold the 4 octets of the IP address as bytes
    UINT32 dwAddr = 0;                // Variable to hold the final IP address as a 32-bit unsigned integer

    Memory::Zero(szCurrOctet, sizeof(szCurrOctet)); // Initialize the current octet string to zero
    dwCurrOctetIndex = 0;                           // Reset the current octet index to zero
    PCCHAR pCurrByte = pIP;                         // Set the current byte pointer to the start of the IP string
    for (;;)
    {
        dwEndOfOctet = 0;       // Reset the end of octet flag
        if (*pCurrByte == '\0') // Check if the current byte is the null terminator
        {
            // end of string
            dwEndOfOctet = 1;  // Set the end of octet flag
            dwEndOfString = 1; // Set the end of string flag
        }
        else if (*pCurrByte == '.') // Check if the current byte is .
        {
            // end of octet
            dwEndOfOctet = 1; // Set the end of octet flag
        }
        else
        { // Ensure this character is a number
            if (*pCurrByte >= '0' && *pCurrByte <= '9')
            {
                if (dwCurrOctetIndex > 2) // Check if the current octet index is greater than 2
                {
                    // invalid ip
                    return INVALID_IPV4; // Return INADDR_NONE to indicate an invalid IP address
                }

                // store current character
                szCurrOctet[dwCurrOctetIndex] = *pCurrByte;
                dwCurrOctetIndex++;
            }
            else
            {
                // invalid ip
                return INVALID_IPV4;
            }
        }

        // Check if the current octet is complete
        if (dwEndOfOctet != 0)
        {
            if (dwCurrOctetIndex == 0)
            {
                // invalid ip
                return INVALID_IPV4;
            }

            // Convert octet string to integer
            dwOctet = String::ParseString<INT32>(szCurrOctet);
            if (dwOctet > 255)
            {
                // invalid ip
                return INVALID_IPV4;
            }

            // Already read 4 octets
            if (dwCompletedOctetCount >= 4)
            {
                // invalid ip
                return INVALID_IPV4;
            }

            // Store current octet
            bOctets[dwCompletedOctetCount] = (UINT8)dwOctet;

            // Current octet complete
            dwCompletedOctetCount++;

            if (dwEndOfString != 0)
            {
                // end of string
                break;
            }

            // Reset szCurrOctet string
            Memory::Zero(szCurrOctet, sizeof(szCurrOctet));
            dwCurrOctetIndex = 0;
        }

        // Move to the next character
        pCurrByte++;
    }

    // Ensure 4 octets were found
    if (dwCompletedOctetCount != 4)
    {
        // invalid string
        return INVALID_IPV4;
    }

    // Store octets in dword value
    Memory::Copy((PVOID)&dwAddr, bOctets, 4);

    return dwAddr;
}