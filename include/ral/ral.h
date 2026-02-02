/**
 * ral.h - Runtime Abstraction Layer
 *
 * Application-level features.
 * Depends on BAL and PAL.
 */

#pragma once

#include "pal.h"

// =============================================================================
// Cryptography
// =============================================================================
#include "crypt/sha2.h"
#include "crypt/ecc.h"
#include "crypt/chacha20.h"
#include "crypt/chacha20_encoder.h"

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
