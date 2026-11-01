// LPROTECT - anti-debugging / anti-analysis checks.
//
// Multiple independent detections. None is unbeatable alone; the point is that
// an attacker must find and neutralize ALL of them, scattered through your code.
//
// Usage:
//   if (lp::debugger_present(lp::AD_ALL)) { ... react ... }
//   lp::ad_install_guard([]{ /* called on detection */ });  // optional watchdog
//
// Reacting: prefer *delayed, indirect* responses (corrupt a result, exit later,
// fail a license check) over an immediate exit at the detection site, which is
// trivial to NOP out. See react_* helpers.
#ifndef LPROTECT_LP_ANTIDEBUG_H
#define LPROTECT_LP_ANTIDEBUG_H

#include "lp_config.h"

namespace lp {

enum AdFlags : uint32_t {
    AD_API            = 1u << 0,  // IsDebuggerPresent / CheckRemoteDebuggerPresent
    AD_PEB            = 1u << 1,  // PEB.BeingDebugged + NtGlobalFlag
    AD_NTQUERY        = 1u << 2,  // NtQueryInformationProcess (port/flags/object)
    AD_HWBP           = 1u << 3,  // hardware breakpoints in debug registers
    AD_TIMING         = 1u << 4,  // rdtsc timing anomaly
    AD_ALL            = 0xFFFFFFFFu
};

// Returns a bitmask of the AdFlags that fired (0 == nothing detected).
uint32_t debugger_present(uint32_t which = AD_ALL);

// Convenience: true if anything in `which` tripped.
inline bool is_debugged(uint32_t which = AD_ALL) { return debugger_present(which) != 0; }

// Install a watchdog thread that re-checks every `period_ms` and calls `on_detect`.
// Call once. The callback runs on the watchdog thread.
typedef void (*ad_callback)(uint32_t fired_flags);
void ad_install_guard(ad_callback on_detect, unsigned period_ms = 1500, uint32_t which = AD_ALL);

// Suggested reaction helpers (use indirectly / after a delay):
//  - react_corrupt: returns a mask to XOR into your own computed values so the
//    program misbehaves subtly instead of obviously bailing out.
uint32_t react_corrupt();

} // namespace lp

#endif // LPROTECT_LP_ANTIDEBUG_H
