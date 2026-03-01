/**
 * runtime.h - Runtime Abstraction Layer
 *
 * Application-level features.
 * Depends on CORE and PLATFORM.
 */

#pragma once

#include "platform.h"

// =============================================================================
// Cryptography
// =============================================================================
#include "crypto/sha2.h"
#include "crypto/ecc.h"
#include "crypto/chacha20.h"
#include "crypto/chacha20_encoder.h"

// =============================================================================
// Networking
// =============================================================================
#include "network/dns.h"
#include "network/http.h"
#include "network/websocket.h"

// TLS
#include "network/tls/tls.h"
#include "network/tls/tls_buffer.h"
#include "network/tls/tls_cipher.h"
#include "network/tls/tls_hash.h"
#include "network/tls/tls_hkdf.h"
