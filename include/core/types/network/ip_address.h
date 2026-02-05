/**
 * @file ip_address.h
 * @brief IP Address Type Supporting IPv4 and IPv6
 *
 * @details Provides a unified IP address type that supports both IPv4 and IPv6
 * addresses. Designed for network programming in position-independent code
 * without CRT dependencies.
 *
 * Features:
 * - Unified IPv4/IPv6 representation
 * - String parsing and formatting
 * - Factory methods for construction
 * - Comparison operators
 *
 * @ingroup core
 *
 * @defgroup ip_address IP Address
 * @ingroup core
 * @{
 */

#pragma once

#include "primitives.h"

/**
 * @enum IPVersion
 * @brief IP address version enumeration
 */
enum class IPVersion : UINT8
{
    IPv4 = 4,      ///< IPv4 address (32-bit)
    IPv6 = 6,      ///< IPv6 address (128-bit)
    Invalid = 0    ///< Invalid/uninitialized address
};

/**
 * @class IPAddress
 * @brief Unified IP address class supporting both IPv4 and IPv6
 *
 * @details Uses a union to efficiently store either an IPv4 (4 bytes) or IPv6
 * (16 bytes) address. Factory methods provide type-safe construction.
 *
 * @par Example Usage:
 * @code
 * // Create IPv4 address from string
 * IPAddress addr = IPAddress::FromString("192.168.1.1");
 * if (addr.IsValid() && addr.IsIPv4()) {
 *     UINT32 ip = addr.ToIPv4();  // Network byte order
 * }
 *
 * // Create IPv4 from raw value
 * IPAddress local = IPAddress::FromIPv4(0x7F000001);  // 127.0.0.1
 *
 * // Convert to string
 * CHAR buffer[64];
 * addr.ToString(buffer, sizeof(buffer));
 * @endcode
 */
class IPAddress
{
private:
    IPVersion version;  ///< Address version (IPv4, IPv6, or Invalid)
    union
    {
        UINT32 ipv4;        ///< IPv4 address (4 bytes, network byte order)
        UINT8 ipv6[16];     ///< IPv6 address (16 bytes)
    } address;

private:
    /**
     * @brief Private constructor for IPv4 address
     * @param ipv4Address IPv4 address in network byte order
     */
    IPAddress(UINT32 ipv4Address);

    /**
     * @brief Private constructor for IPv6 address
     * @param ipv6Address 16-byte IPv6 address array
     */
    IPAddress(const UINT8 ipv6Address[16]);

public:
    /// @name Constructors
    /// @{

    /**
     * @brief Default constructor - creates an invalid IP address
     */
    IPAddress();

    /**
     * @brief Copy constructor
     * @param other IPAddress to copy
     */
    IPAddress(const IPAddress& other);

    /// @}
    /// @name Factory Methods
    /// @{

    /**
     * @brief Create IPv4 address from raw 32-bit value
     * @param ipv4Address IPv4 address in network byte order
     * @return IPAddress instance
     */
    static IPAddress FromIPv4(UINT32 ipv4Address);

    /**
     * @brief Create IPv6 address from 16-byte array
     * @param ipv6Address 16-byte IPv6 address
     * @return IPAddress instance
     */
    static IPAddress FromIPv6(const UINT8 ipv6Address[16]);

    /**
     * @brief Parse IP address from string
     * @param ipString String representation (e.g., "192.168.1.1" or "::1")
     * @return IPAddress instance (Invalid if parsing fails)
     */
    static IPAddress FromString(PCCHAR ipString);

    /**
     * @brief Create an invalid IP address
     * @return IPAddress with Invalid version
     */
    static IPAddress Invalid();

    /// @}
    /// @name Validation
    /// @{

    /**
     * @brief Check if address is valid
     * @return TRUE if IPv4 or IPv6, FALSE if Invalid
     */
    BOOL IsValid() const;

    /**
     * @brief Check if address is IPv4
     * @return TRUE if IPv4 address
     */
    BOOL IsIPv4() const;

    /**
     * @brief Check if address is IPv6
     * @return TRUE if IPv6 address
     */
    BOOL IsIPv6() const;

    /**
     * @brief Get address version
     * @return IPVersion enumeration value
     */
    IPVersion GetVersion() const;

    /// @}
    /// @name Conversion Methods
    /// @{

    /**
     * @brief Get IPv4 address value
     * @return IPv4 address in network byte order (undefined if not IPv4)
     */
    UINT32 ToIPv4() const;

    /**
     * @brief Get IPv6 address array
     * @return Pointer to 16-byte IPv6 address (undefined if not IPv6)
     */
    const UINT8* ToIPv6() const;

    /**
     * @brief Convert to string representation
     * @param buffer Output buffer
     * @param bufferSize Size of output buffer
     * @return TRUE on success, FALSE if buffer too small
     */
    BOOL ToString(PCHAR buffer, UINT32 bufferSize) const;

    /// @}
    /// @name Operators
    /// @{

    /**
     * @brief Equality comparison
     * @param other IPAddress to compare
     * @return TRUE if addresses are equal
     */
    BOOL operator==(const IPAddress& other) const;

    /**
     * @brief Inequality comparison
     * @param other IPAddress to compare
     * @return TRUE if addresses are not equal
     */
    BOOL operator!=(const IPAddress& other) const;

    /**
     * @brief Assignment operator
     * @param other IPAddress to assign
     * @return Reference to this
     */
    IPAddress& operator=(const IPAddress& other);

    /// @}
};

/** @} */ // end of ip_address group
