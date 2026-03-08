/**
 * @file runtime.h
 * @brief Runtime Abstraction Layer
 *
 * @details Aggregate header for the RUNTIME layer. Includes CORE + PLATFORM
 * (via platform.h) plus all runtime-level features: cryptography, networking,
 * and TLS 1.3.
 *
 * @defgroup runtime Runtime Abstraction Layer
 * @{
 */

#pragma once

#include "platform/platform.h"

/// @name Cryptography
/// @{
#include "runtime/crypto/sha2.h"
#include "runtime/crypto/ecc.h"
#include "runtime/crypto/chacha20.h"
#include "runtime/crypto/chacha20_encoder.h"
/// @}

/// @name Networking
/// @{
#include "runtime/network/dns/dns.h"
#include "runtime/network/http/http.h"
#include "runtime/network/websocket/websocket.h"
/// @}

/// @name Image
/// @{
#include "runtime/image/jpeg.h"
/// @}

/// @name Display
/// @{
#include "runtime/display/screen.h"
/// @}

/// @name TLS 1.3
/// @{
#include "runtime/network/tls/tls.h"
#include "runtime/network/tls/tls_buffer.h"
#include "runtime/network/tls/tls_cipher.h"
#include "runtime/network/tls/tls_hash.h"
#include "runtime/network/tls/tls_hkdf.h"
/// @}

/** @} */ // end of runtime group
