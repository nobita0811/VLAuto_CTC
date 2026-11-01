// LPROTECT - Ed25519 signatures (self-contained, no external deps).
//
// Implementation is a faithful port of TweetNaCl (D. J. Bernstein et al.,
// public domain), reduced to the Ed25519 sign/verify path plus its SHA-512.
// It lets you ship an ASYMMETRIC licensing scheme: the client embeds only the
// 32-byte PUBLIC key and can verify but never forge; your keygen holds the
// 64-byte secret key and signs. Extracting the client no longer yields a
// key-minting capability (unlike the symmetric HMAC scheme).
//
//   sign:   ed25519_sign(sig, msg, n, sk)        // sk = 64 bytes (seed||pub)
//   verify: ok = ed25519_verify(sig, msg, n, pk) // pk = 32 bytes
//   keys:   ed25519_keypair(pk, sk)              // uses OS RNG (needs bcrypt)
#ifndef LPROTECT_LP_ED25519_H
#define LPROTECT_LP_ED25519_H

#include "lp_config.h"

namespace lp {

constexpr size_t ED25519_PUBLIC_SIZE  = 32;
constexpr size_t ED25519_SECRET_SIZE  = 64;  // seed(32) || public(32)
constexpr size_t ED25519_SIGN_SIZE    = 64;

// Produce a detached 64-byte signature over msg[0..n).
void ed25519_sign(uint8_t sig[64], const void* msg, size_t n, const uint8_t sk[64]);

// Verify a detached signature. Returns true iff valid.
bool ed25519_verify(const uint8_t sig[64], const void* msg, size_t n, const uint8_t pk[32]);

// Generate a fresh keypair using the OS CSPRNG (BCryptGenRandom).
// Links bcrypt.lib only in the final binary that actually calls this.
// Returns true on success. Keep `sk` secret and OFF the client.
bool ed25519_keypair(uint8_t pk[32], uint8_t sk[64]);

} // namespace lp

#endif // LPROTECT_LP_ED25519_H
