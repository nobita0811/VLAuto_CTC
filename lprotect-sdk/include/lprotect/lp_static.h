// LPROTECT - static (on-disk) integrity verification.
//
// lp_integrity.h catches RUNTIME patching (a byte changed in memory, e.g. a
// debugger breakpoint or an inline hook). This module catches STATIC patching:
// someone edited your shipped .exe/.dll on disk (a cracked binary, a keygen
// patch, a modified resource).
//
// How it works:
//   1. Your binary embeds a "stamp": a 16-byte magic followed by a 32-byte hash
//      slot (initially zero) - the global g_lp_stamp below.
//   2. After linking, the post-build tool `lprotect_stamp <file>` computes
//      SHA-256 over the whole file (with the hash slot zeroed) and writes the
//      result into the slot.
//   3. At runtime, static_verify() re-opens the running file ON DISK, zeroes the
//      slot in the read buffer, recomputes SHA-256, and compares. Any edit to
//      the shipped file flips the result to Mismatch.
//
// Reading the file from disk (rather than the loaded module image) side-steps
// ASLR relocations entirely - the on-disk bytes are exactly what was stamped.
//
// Limits:
//   * Detects disk tampering, not in-memory patching (use lp_integrity for that).
//   * If you CODE-SIGN the binary, do it AFTER stamping is moot: signing appends
//     a certificate and changes the file, breaking the hash. Either rely on the
//     Authenticode signature instead for signed builds, or stamp a copy you do
//     not sign. See README.
//   * An attacker can also patch out the call to static_verify() - scatter the
//     check, react indirectly, and combine with lp_vm for critical paths.
#ifndef LPROTECT_LP_STATIC_H
#define LPROTECT_LP_STATIC_H

#include "lp_config.h"

namespace lp {

enum class StaticResult {
    Ok = 0,      // on-disk file matches the embedded hash
    NotStamped,  // hash slot is still zero (lprotect_stamp was not run)
    FileError,   // could not read the running file / stamp not found
    Mismatch     // file on disk differs from what was stamped (tampered)
};

const char* static_result_str(StaticResult r);

// Verify the currently running executable's on-disk bytes against the stamp.
StaticResult static_verify();

// SHA-256 helper shared by the client and the stamp tool (identical bytes in).
void sha256(const void* data, size_t len, uint8_t out[32]);

// Fill out[16] with the stamp magic that the tool/client scan for. It is built
// at runtime from an obfuscated constant so the plaintext 16-byte pattern never
// sits in .rdata - the ONLY plaintext copy in a binary is inside g_lp_stamp,
// so the scan has exactly one match.
void stamp_magic(unsigned char out[16]);
constexpr size_t LP_STAMP_MAGIC_LEN = 16;
constexpr size_t LP_STAMP_HASH_LEN  = 32;

} // namespace lp

#endif // LPROTECT_LP_STATIC_H
