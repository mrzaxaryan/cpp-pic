#pragma once

#include "primitives.h"
#include "uint64.h"

// SHA-2 Constants
#define SHA224_DIGEST_SIZE (224 / 8)
#define SHA256_DIGEST_SIZE (256 / 8)
#define SHA384_DIGEST_SIZE (384 / 8)
#define SHA512_DIGEST_SIZE (512 / 8)

#define SHA256_BLOCK_SIZE (512 / 8)
#define SHA512_BLOCK_SIZE (1024 / 8)
#define SHA384_BLOCK_SIZE SHA512_BLOCK_SIZE
#define SHA224_BLOCK_SIZE SHA256_BLOCK_SIZE

// SHA-224 Class
class SHA224
{
private:
    UINT64 tot_len;                     // Total length of the message
    UINT64 len;                         // Current length of the message     
    UINT8 block[2 * SHA224_BLOCK_SIZE]; // Message block buffer for processing
    UINT32 h[8];                        // Hash state          

public:
    SHA224();                                                           // Constructor
    VOID Update(const UINT8 *message, UINT64 len);                      // Update hash with message      
    VOID Final(UINT8 *digest);                                          // Finalize and produce the digest        
    static VOID Hash(const UINT8 *message, UINT64 len, UINT8 *digest);  // Static method to compute hash 
};

// SHA-256 Class
class SHA256
{
private:
    UINT64 tot_len;                     // Total length of the message
    UINT64 len;                         // Current length of the message     
    UINT8 block[2 * SHA256_BLOCK_SIZE]; // Message block buffer for processing
    UINT32 h[8];                        // Hash state

public:
    SHA256();                                                                  // Constructor                                         
    VOID Update(const UINT8 *message, UINT64 len);                             // Update hash with message
    VOID Final(UINT8 *digest);                                                 // Finalize and produce the digest
    static VOID Hash(const UINT8 *message, UINT64 len, UINT8 *digest);         // Static method to compute hash
    static VOID Transform(SHA256 *ctx, const UINT8 *message, UINT64 block_nb); // Transform function
};

// SHA-384 Class
class SHA384
{
private:
    UINT64 tot_len;                     // Total length of the message
    UINT64 len;                         // Current length of the message     
    UINT8 block[2 * SHA384_BLOCK_SIZE]; // Message block buffer for processing
    UINT64 h[8];                        // Hash state

public:
    SHA384();                                                          // Constructor
    VOID Update(const UINT8 *message, UINT64 len);                     // Update hash with message
    VOID Final(UINT8 *digest);                                         // Finalize and produce the digest
    static VOID Hash(const UINT8 *message, UINT64 len, UINT8 *digest); // Static method to compute hash
};

// SHA-512 Class
class SHA512
{
private:
    UINT64 tot_len;                     // Total length of the message
    UINT64 len;                         // Current length of the message     
    UINT8 block[2 * SHA512_BLOCK_SIZE]; // Message block buffer for processing
    UINT64 h[8];                        // Hash state                              

public:
    SHA512();                                                          // Constructor
    VOID Update(const UINT8 *message, UINT64 len);                     // Update hash with message
    VOID Final(UINT8 *digest);                                         // Finalize and produce the digest
    static VOID Hash(const UINT8 *message, UINT64 len, UINT8 *digest); // Static method to compute hash
    static VOID Transform(SHA512 *ctx, const UINT8 *message, UINT64 block_nb); // Transform function
};

// HMAC-SHA224 Class
class HMAC_SHA224
{
private:
    SHA224 ctx_inside;                   // Inside SHA-224 context
    SHA224 ctx_outside;                  // Outside SHA-224 context
    SHA224 ctx_inside_reinit;            // Reinitialization context for inside
    SHA224 ctx_outside_reinit;           // Reinitialization context for outside
    UCHAR block_ipad[SHA224_BLOCK_SIZE]; // Inner padding block
    UCHAR block_opad[SHA224_BLOCK_SIZE]; // Outer padding block

public:
    HMAC_SHA224(const UCHAR *key, UINT32 key_size); // Constructor
    VOID Reinit();                                  // Reinitialize HMAC contexts
    VOID Update(const UCHAR *message, UINT32 message_len); // Update HMAC with message
    VOID Final(PUCHAR mac, UINT32 mac_size);               // Finalize and produce the MAC
    static VOID Compute(const UCHAR *key, UINT32 key_size, const UCHAR *message, UINT32 message_len, PUCHAR mac, UINT32 mac_size); // Method to compute HMAC
};

// HMAC-SHA256 Class
class HMAC_SHA256
{
private:
    SHA256 ctx_inside;                    // Inside SHA-256 context
    SHA256 ctx_outside;                   // Outside SHA-256 context
    SHA256 ctx_inside_reinit;             // Reinitialization context for inside
    SHA256 ctx_outside_reinit;            // Reinitialization context for outside
    UCHAR block_ipad[SHA256_BLOCK_SIZE];  // Inner padding block
    UCHAR block_opad[SHA256_BLOCK_SIZE];  // Outer padding block

public:
    VOID Init(const UCHAR *key, UINT32 key_size); // Initialize HMAC with key
    VOID Reinit(); // Reinitialize HMAC contexts
    VOID Update(const UCHAR *message, UINT32 messageLen); // Update HMAC with message
    VOID Final(PUCHAR mac, UINT32 macSize); // Finalize and produce the MAC
    static VOID Compute(const UCHAR *key, UINT32 keySize, const UCHAR *message, UINT32 messageLen, PUCHAR mac, UINT32 macSize); // Method to compute HMAC
};

// HMAC-SHA384 Class
class HMAC_SHA384
{
private:
    SHA384 ctx_inside;                   // Inside SHA-384 context
    SHA384 ctx_outside;                  // Outside SHA-384 context
    SHA384 ctx_inside_reinit;            // Reinitialization context for inside
    SHA384 ctx_outside_reinit;           // Reinitialization context for outside
    UCHAR block_ipad[SHA384_BLOCK_SIZE]; // Inner padding block
    UCHAR block_opad[SHA384_BLOCK_SIZE]; // Outer padding block
public:
    VOID Init(const UCHAR *key, UINT32 key_size);   // Initialize HMAC with key
    VOID Reinit(); // Reinitialize HMAC contexts
    VOID Update(const UCHAR *message, UINT32 message_len); // Update HMAC with message
    VOID Final(PUCHAR mac, UINT32 mac_size); // Finalize and produce the MAC
    static VOID Compute(const UCHAR *key, UINT32 key_size, const UCHAR *message, UINT32 message_len, PUCHAR mac, UINT32 mac_size); // Method to compute HMAC
};

// HMAC-SHA512 Class
class HMAC_SHA512
{
private:
    SHA512 ctx_inside;                   // Inside SHA-512 context
    SHA512 ctx_outside;                  // Outside SHA-512 context
    SHA512 ctx_inside_reinit;            // Reinitialization context for inside 
    SHA512 ctx_outside_reinit;           // Reinitialization context for outside
    UCHAR block_ipad[SHA512_BLOCK_SIZE]; // Inner padding block
    UCHAR block_opad[SHA512_BLOCK_SIZE]; // Outer padding block

public:
    HMAC_SHA512(const UCHAR *key, UINT32 key_size); // Constructor
    VOID Reinit(); // Reinitialize HMAC contexts
    VOID Update(const UCHAR *message, UINT32 message_len); // Update HMAC with message
    VOID Final(PUCHAR mac, UINT32 mac_size); // Finalize and produce the MAC
    static VOID Compute(const UCHAR *key, UINT32 key_size, const UCHAR *message, UINT32 message_len, PUCHAR mac, UINT32 mac_size); // Method to compute HMAC
};
