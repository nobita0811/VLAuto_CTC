// LPROTECT - runtime self-integrity checking.
//
// Detects if a region of your own code was modified in memory after startup -
// e.g. a 0xCC software breakpoint or an inline hook patched over a function.
//
// Model: at init you register one or more code regions. A baseline hash is taken
// then. Later, verify() rehashes and compares. A mismatch means the bytes you
// shipped are not the bytes currently executing.
//
//   lp::integrity_add(reinterpret_cast<const void*>(&MyHotFunction), 256);
//   lp::integrity_snapshot();              // take baselines (once, early)
//   ...
//   if (!lp::integrity_verify()) { react(); }
//
// This catches *runtime* modification. Detecting *on-disk* patching of your
// shipped file needs a baseline embedded at build time by a post-build tool;
// see README ("static integrity") for how to add that layer.
#ifndef LPROTECT_LP_INTEGRITY_H
#define LPROTECT_LP_INTEGRITY_H

#include "lp_config.h"

namespace lp {

// Register a region to watch. Returns an id (>=0) or -1 on failure/overflow.
int  integrity_add(const void* addr, size_t len);

// Compute and store baseline hashes for all registered regions. Call once,
// as early as possible, before anything could have patched memory.
void integrity_snapshot();

// Rehash all regions; returns true if every region still matches its baseline.
bool integrity_verify();

// Per-region check. Returns true if region `id` matches its baseline.
bool integrity_verify_one(int id);

// Standalone CRC32 (IEEE) over a buffer - handy for your own checks.
uint32_t crc32(const void* data, size_t len, uint32_t seed = 0);

} // namespace lp

#endif // LPROTECT_LP_INTEGRITY_H
