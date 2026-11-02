// LPROTECT - sealed-blob open/close.
//
// A "sealed blob" binds a chunk of data (e.g. a table of game offsets) to the
// 32-byte seal key K that the activation server delivers only to an
// authenticated session (see activation_seal_key). The blob is produced at BUILD
// time (tools/sealblob) and shipped encrypted inside the app; at RUNTIME the app
// opens it with K obtained from a verified token. Without a valid K (e.g. a
// patched-out license gate that never reached the server) the blob cannot be
// opened, so the data it protects stays unavailable and the app is non-functional.
//
// Cipher (deliberately simple & dependency-light; it only needs to be reversible
// with K — an attacker who already holds K can open it regardless):
//   keystream block i = SHA256( K(32) || iv(16) || LE32(i) )
//   ciphertext        = plaintext XOR keystream
//   tag               = SHA256( K(32) || plaintext )      (integrity / wrong-K)
// Blob layout (little-endian):
//   magic(4)="LPS1" | iv(16) | ct_len(4) | ct(ct_len) | tag(32)
//
// Must match tools/sealblob/sealblob.js byte-for-byte.
#ifndef LPROTECT_LP_SEAL_H
#define LPROTECT_LP_SEAL_H

#include "lp_config.h"
#include <cstdint>
#include <cstddef>

namespace lp {

// Minimum blob size (empty plaintext): magic+iv+len+tag.
static constexpr size_t kSealOverhead = 4 + 16 + 4 + 32;

// Open a sealed blob with key K. On success, writes the plaintext to `out` and
// returns its length; returns 0 on any failure (bad magic, short buffer, or tag
// mismatch = wrong key / tampered blob). `out` is left zeroed on failure.
size_t seal_open(const uint8_t key[32], const uint8_t* blob, size_t blob_len,
                 uint8_t* out, size_t out_cap);

// Seal `data` under key K into `blob` (mainly for tests / a C++ generator). `iv`
// is 16 bytes of caller-supplied randomness. Returns the total blob length, or 0
// if `blob_cap` is too small (need data_len + kSealOverhead).
size_t seal_close(const uint8_t key[32], const uint8_t iv[16],
                  const uint8_t* data, size_t data_len,
                  uint8_t* blob, size_t blob_cap);

} // namespace lp

#endif // LPROTECT_LP_SEAL_H
