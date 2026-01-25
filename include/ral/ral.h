/**
 * ral.h - Runtime Abstraction Layer
 *
 * Application-level features.
 * Depends on BAL and PAL.
 */

#pragma once

#include "pal.h"

// =============================================================================
// Logging
// =============================================================================
#include "logger.h"

// =============================================================================
// Cryptography
// =============================================================================
#include "sha2.h"
#include "ecc.h"
#include "chacha20.h"
#include "chacha20_encoder.h"
#include "base64.h"
#include "random.h"

// =============================================================================
// Networking
// =============================================================================
#include "network.h"
#include "dns.h"
#include "http.h"
#include "websocket.h"

// TLS
#include "tls.h"
#include "tls_buffer.h"
#include "tls_buffer_reader.h"
#include "tls_cipher.h"
#include "tls_hash.h"
#include "tls_hkdf.h"
#include "tls_hmac.h"
