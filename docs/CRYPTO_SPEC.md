# TinyBFT Cryptography Specification

Complete authentication and cryptographic specification for porting.

## 1. Overview

TinyBFT uses a hybrid authentication scheme:

| Mode | Used For | Algorithm |
|------|----------|-----------|
| HMAC (default) | Inter-replica consensus | HMAC-SHA256 with session keys |
| RSA-PSS | Client requests, key exchange | RSA-1024 with PSS padding |

**Crypto library:** mbedTLS (required dependency)

---

## 2. Hash Functions

### 2.1 SHA-256 Digest

Used for message digests, state checksums, and request identification.

**Algorithm:** SHA-256
**Output size:** 32 bytes

```c
// Compute SHA-256 digest
mbedtls_sha256_context ctx;
mbedtls_sha256_init(&ctx);
mbedtls_sha256_starts(&ctx, false);  // false = SHA-256 (not SHA-224)
mbedtls_sha256_update(&ctx, data, len);
mbedtls_sha256_finish(&ctx, digest_buffer);
```

**Digest comparison (constant-time):**
```c
bool digest_equal(const unsigned char* a, const unsigned char* b) {
    uint32_t diff = 0;
    for (int i = 0; i < 32; i++) {
        diff |= a[i] ^ b[i];
    }
    return (diff == 0);
}
```

### 2.2 State Block Digest

For checkpointing state blocks:
```python
digest = SHA-256(block_index || last_modified_seqno || state_data)
```

Where:
- `block_index`: 4-byte integer (block identifier)
- `last_modified_seqno`: 8-byte Seqno (last modification)
- `state_data`: variable-length block data

---

## 3. HMAC-SHA256

### 3.1 Constants

```c
const size_t Key_size = 32;    // Session key size (bytes)
const size_t HMAC_size = 32;   // MAC output size (bytes)
```

### 3.2 Session Keys

Each principal maintains two session keys per peer:
```c
unsigned char kin[32];   // Incoming session key (for verifying)
unsigned char kout[32];  // Outgoing session key (for generating)
```

**Key derivation:** Keys are randomly generated and distributed via `New_key` messages.

### 3.3 HMAC Computation

```c
// Initialize HMAC context
mbedtls_md_context_t ctx;
mbedtls_md_init(&ctx);
const mbedtls_md_info_t* info = mbedtls_md_info_from_type(MBEDTLS_MD_SHA256);
mbedtls_md_setup(&ctx, info, 1);  // 1 = HMAC mode

// Generate MAC
mbedtls_md_hmac_starts(&ctx, session_key, Key_size);
mbedtls_md_hmac_update(&ctx, message_data, message_len);
mbedtls_md_hmac_finish(&ctx, mac_buffer);

// Reset for reuse
mbedtls_md_hmac_reset(&ctx);
```

### 3.4 Authenticator Layout

For a message sent from replica `sender`:

```
Offset calculation for verifier:
  if (my_id < sender_id):
      offset = my_id * HMAC_size
  else:
      offset = (my_id - 1) * HMAC_size

Total authenticator: (num_replicas - 1) * HMAC_size bytes
```

Each MAC covers the entire message header (not including the authenticator itself).

### 3.5 Constant-Time MAC Comparison

```c
bool mac_equal(const unsigned char* a, const unsigned char* b, size_t len) {
    uint32_t diff = 0;
    for (size_t i = 0; i < len; i++) {
        diff |= a[i] ^ b[i];
    }
    return (((diff ^ 0U) - 1) >> 31) & 1;
}
```

---

## 4. RSA Digital Signatures

### 4.1 Key Specifications

| Parameter | Value |
|-----------|-------|
| Algorithm | RSA |
| Key size | 1024 bits |
| Signature padding | PKCS#1 v2.1 (PSS) |
| Signature hash | SHA-256 |
| Encryption padding | PKCS#1 v2.1 (OAEP) |
| Encryption hash | SHA-256 |

### 4.2 Key Generation

```bash
# Generate 1024-bit RSA private key
openssl genrsa -out replica.pem 1024

# Extract public key
openssl rsa -in replica.pem -pubout -out replica.pub
```

### 4.3 Key Loading (mbedTLS)

```c
// Private key
mbedtls_pk_context pk_ctx;
mbedtls_pk_init(&pk_ctx);
mbedtls_pk_parse_keyfile(&pk_ctx, "key.pem", NULL,
                          mbedtls_ctr_drbg_random, &drbg_ctx);

// Set RSA padding to PKCS#1 v2.1
mbedtls_rsa_set_padding(mbedtls_pk_rsa(pk_ctx),
                        MBEDTLS_RSA_PKCS_V21,
                        MBEDTLS_MD_SHA256);
```

### 4.4 Signature Generation

```c
// Sign message digest
unsigned char hash[32];
mbedtls_sha256(message, msg_len, hash, 0);

unsigned char signature[128];  // Exact for 1024-bit key
size_t sig_len;
mbedtls_rsa_rsassa_pss_sign(mbedtls_pk_rsa(pk_ctx),
                            mbedtls_ctr_drbg_random, &drbg_ctx,
                            MBEDTLS_MD_SHA256, 32, hash,
                            signature);
sig_len = 128;  // 1024-bit RSA signature length
```

### 4.5 Signature Verification

```c
// Verify signature
unsigned char hash[32];
mbedtls_sha256(message, msg_len, hash, 0);

int ret = mbedtls_rsa_rsassa_pss_verify(mbedtls_pk_rsa(pub_key_ctx),
                                        MBEDTLS_MD_SHA256,
                                        32, hash,
                                        signature);
// ret == 0 means valid
```

### 4.6 Signature Format in Messages

```
4 bytes:  signature_length (uint32, big-endian)
N bytes:  RSA-PSS signature (N = key_size / 8 = 128 for 1024-bit)
```

Total: 132 bytes for 1024-bit RSA

---

## 5. RSA-OAEP Encryption (Key Exchange)

Used in `New_key` messages to distribute session keys securely.

### 5.1 Encryption

```c
// Encrypt 32-byte session key with peer's public key
unsigned char encrypted[256];
size_t enc_len;
mbedtls_rsa_rsaes_oaep_encrypt(mbedtls_pk_rsa(peer_pub_key),
                               mbedtls_ctr_drbg_random, &drbg_ctx,
                               NULL, 0,  // No label
                               MBEDTLS_MD_SHA256,
                               32, session_key,  // Input: 32 bytes
                               encrypted);
enc_len = 128;  // 1024-bit RSA output
```

### 5.2 Decryption

```c
// Decrypt session key with own private key
unsigned char decrypted[32];
size_t dec_len;
mbedtls_rsa_rsaes_oaep_decrypt(mbedtls_pk_rsa(my_priv_key),
                               mbedtls_ctr_drbg_random, &drbg_ctx,
                               NULL, 0,
                               MBEDTLS_MD_SHA256,
                               &dec_len,
                               encrypted, 128,  // Input
                               decrypted, 32);  // Output buffer
```

---

## 6. Random Number Generation

### 6.1 DRBG Initialization

```c
mbedtls_entropy_context entropy;
mbedtls_ctr_drbg_context drbg_ctx;

mbedtls_entropy_init(&entropy);
mbedtls_ctr_drbg_init(&drbg_ctx);
mbedtls_ctr_drbg_seed(&drbg_ctx,
                      mbedtls_entropy_func, &entropy,
                      (const unsigned char*)"tinybft", 7);
```

### 6.2 Key Generation

```c
// Generate random 32-byte session key
unsigned char key[32];
mbedtls_ctr_drbg_random(&drbg_ctx, key, 32);
```

---

## 7. Key Rotation Protocol

### 7.1 Rotation Trigger

- Timer-based: every `auth_timeout` milliseconds (config value)
- Default: 1,800,000 ms (30 minutes)

### 7.2 New_key Message Construction

```python
for each peer (except self):
    session_key = random_bytes(32)
    encrypted_key = RSA_OAEP_encrypt(peer.public_key, session_key)
    append (peer_id, encrypted_key) to message

sign message with own private key
broadcast to all replicas
```

### 7.3 Key Update on Receipt

```python
verify signature with sender's public key
for each key entry:
    decrypted_key = RSA_OAEP_decrypt(my.private_key, encrypted_key)
    if sender_id < my_id:
        my.kin[sender_id] = decrypted_key  # Incoming from sender
    else:
        my.kin[sender_id] = decrypted_key

# Re-authenticate pending messages
for each pending certificate:
    re_authenticate()
```

### 7.4 Stale Key Handling

After key rotation:
- All existing authenticators become stale
- `Prepared_cert::mark_stale()` called
- New messages must use fresh keys
- Old messages with stale authenticators are rejected

---

## 8. ESP32 Crypto Considerations

### 8.1 Hardware Acceleration

ESP32-C3 has hardware-accelerated SHA and AES. mbedTLS in ESP-IDF uses these automatically.

### 8.2 Memory Usage

| Operation | Stack Usage | Notes |
|-----------|-------------|-------|
| SHA-256 | ~200 bytes | Streaming, low memory |
| HMAC-SHA256 | ~400 bytes | Includes SHA-256 context |
| RSA-1024 sign | ~2KB | Key + working memory |
| RSA-1024 verify | ~2KB | Key + working memory |
| RSA-OAEP encrypt | ~2KB | Key + working memory |

### 8.3 Performance (ESP32-C3 @ 160MHz)

| Operation | Approximate Time |
|-----------|------------------|
| SHA-256 (1KB) | ~0.1ms |
| HMAC-SHA256 (1KB) | ~0.15ms |
| RSA-1024 sign | ~5ms |
| RSA-1024 verify | ~1ms |
| RSA-OAEP encrypt | ~1ms |
| Random key gen (32B) | ~0.01ms |

### 8.4 Key Storage on ESP32

Keys stored in SPIFFS filesystem:
```
/spiffs/priv/r0.pem     # Private key
/spiffs/priv/r0.pub     # Public key
/spiffs/config           # Peer MAC addresses
```

---

## 9. Security Checklist

- [ ] SHA-256 produces correct 32-byte digests
- [ ] HMAC-SHA256 produces correct 32-byte MACs
- [ ] HMAC comparison uses constant-time algorithm
- [ ] RSA-1024 PSS signature generation works
- [ ] RSA-1024 PSS signature verification works
- [ ] RSA-OAEP encryption/decryption works for 32-byte keys
- [ ] DRBG seeded with sufficient entropy
- [ ] Session keys are 32 bytes, randomly generated
- [ ] Key rotation triggers at configured interval
- [ ] Stale authenticators are rejected after rotation
- [ ] No timing side-channels in crypto operations

---

## Source Files

- `src/Digest.cc` - SHA-256 digest computation
- `src/hmac.cc` - HMAC-SHA256 implementation
- `src/rsa_private_key.cc` - RSA private key loading and signing
- `src/rsa_public_key.cc` - RSA public key loading and verification
- `src/Principal.cc` - Per-principal key management
- `src/New_key.cc` - Key rotation message
- `include/Principal.h` - Session key storage
- `include/rsa_private_key.h` - Private key interface
- `include/rsa_public_key.h` - Public key interface
