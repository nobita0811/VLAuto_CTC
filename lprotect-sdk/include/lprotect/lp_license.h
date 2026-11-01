// LPROTECT - licensing / activation (hardware-locked serials + trial).
//
// Scheme (ASYMMETRIC, Ed25519):
//   license_key = BASE32( payload || Ed25519_sig(sk, payload) )
//   payload     = { version, hwid32, expiry_unix }   (16 bytes)
//   signature   = 64 bytes
//
// The client embeds ONLY the 32-byte PUBLIC key (kPublicKey in lp_license.cpp)
// and verifies the signature. Your keygen holds the 64-byte SECRET key and
// signs - it is never shipped. Extracting the client yields no ability to mint
// keys, unlike a symmetric/HMAC scheme.
//
// Setup: run `lprotect_keygen --genkey` once, paste the printed public key into
// kPublicKey[] in lp_license.cpp, and keep the secret key safe (it is what your
// keygen signs with). See README.
#ifndef LPROTECT_LP_LICENSE_H
#define LPROTECT_LP_LICENSE_H

#include "lp_config.h"

namespace lp {

enum class LicResult {
    Ok = 0,
    BadFormat,       // not decodable / wrong length
    BadSignature,    // HMAC tag mismatch (forged/corrupt)
    WrongMachine,    // HWID does not match this machine
    Expired,         // past expiry
    VersionMismatch  // payload version not understood
};

// Which inputs feed the machine fingerprint. More sources = more unique per
// machine (a VM/disk clone is more likely to differ), but also more likely to
// change when the user swaps hardware. Tune to your tolerance.
enum HwidSource : uint32_t {
    HWID_VOLUME      = 1u << 0, // system volume serial (copied by a disk clone)
    HWID_MACHINEGUID = 1u << 1, // registry MachineGuid (copied by a disk clone)
    HWID_CPU         = 1u << 2, // CPUID vendor/features (near-zero uniqueness)
    HWID_SMBIOS_UUID = 1u << 3, // SMBIOS system UUID (hypervisors reissue on clone)
    HWID_MAC         = 1u << 4, // primary NIC MAC (full-clone usually reassigns)

    // Default now includes SMBIOS UUID + MAC so typical VM clones get a DIFFERENT
    // id (needed for device-count anti-sharing to work). NOTE: this changes the
    // value vs. the older build - re-issue keys/tokens after upgrading.
    HWID_DEFAULT = HWID_VOLUME | HWID_MACHINEGUID | HWID_CPU | HWID_SMBIOS_UUID | HWID_MAC,

    // The old, clone-stable set (survives a raw disk/VM clone).
    HWID_STABLE  = HWID_VOLUME | HWID_MACHINEGUID | HWID_CPU
};

// 32-bit machine fingerprint. Not a secret; only has to be consistent on the
// same machine for the chosen `sources`.
uint32_t hardware_id(uint32_t sources = HWID_DEFAULT);

// Validate a user-entered license key against THIS machine.
// `out_expiry_unix` (optional) receives the encoded expiry on Ok.
LicResult license_check(const char* key, uint64_t* out_expiry_unix = nullptr);

// ---- Trial support ----
// First call records "now" in an obfuscated store (registry under HKCU by
// default). Subsequent calls report remaining seconds. Returns false if the
// trial has elapsed or the store looks tampered.
bool trial_begin_or_check(const char* app_id, uint64_t trial_seconds,
                          uint64_t* out_remaining = nullptr);

// ---- For building your OWN keygen tool. Declared here so the tool reuses the
// exact codec and signs with YOUR secret key. ----
namespace keygen {
    // Encode + sign a key for a target machine. `sk` is your 64-byte Ed25519
    // secret key. buf must be >= 160 bytes. Returns length written (excluding
    // null) or 0 on error. `hwid` is the TARGET machine's id, expiry 0 = never.
    size_t make_key(char* buf, size_t buf_len,
                    uint32_t hwid, uint64_t expiry_unix,
                    const uint8_t sk[64], uint8_t version = 1);
}

} // namespace lp

#endif // LPROTECT_LP_LICENSE_H
