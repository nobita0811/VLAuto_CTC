// LPROTECT - lightweight obfuscation helpers (header-only).
//
// These make control flow and constants harder to read statically. They are
// deliberately simple and composable; real virtualization lives in lp_vm.h.
//
//  * LP_OPAQUE_TRUE / LP_OPAQUE_FALSE : predicates that are provably constant to
//    a human but opaque to the optimizer (kept via lp::opaque()).
//  * LP_JUNK() : emits dead-but-live-looking arithmetic to pad/space code.
//  * LP_IF_REAL(cond) : branch guarded by an opaque-true, so the disassembler
//    sees an extra conditional that always falls through to your real path.
//  * LP_CONST32(x) : computes a constant at runtime from an opaque identity so
//    it does not appear as an immediate in the instruction stream.
//
// Caveat: modern decompilers (IDA/Ghidra + plugins) can simplify many opaque
// predicates. Treat this as friction, and layer it with lp_vm for anything
// genuinely sensitive.
#ifndef LPROTECT_LP_OBF_H
#define LPROTECT_LP_OBF_H

#include "lp_config.h"

namespace lp {

// (x*x) is never negative; (x*(x+1)) is always even -> always-true/false seeds
// that the optimizer cannot collapse because x comes through opaque().
LP_FORCEINLINE bool opaque_true() {
    volatile int x = static_cast<int>(__LINE__);
    int y = opaque(x);
    return ((y * (y + 1)) & 1) == 0;   // product of consecutive ints is even
}
LP_FORCEINLINE bool opaque_false() {
    volatile int x = static_cast<int>(__LINE__ + 7);
    int y = opaque(x);
    return ((y | 1) & 1) == 0;         // (anything|1) is odd -> false
}

LP_FORCEINLINE uint32_t const32(uint32_t v) {
    uint32_t k = opaque<uint32_t>(0x9E3779B9u);
    return (v ^ k) ^ k;                // identity, but routed through opaque()
}

LP_FORCEINLINE void junk() {
    volatile uint32_t a = opaque<uint32_t>(__LINE__ * 2654435761u);
    volatile uint32_t b = opaque<uint32_t>(a ^ 0xDEADBEEFu);
    a = (a << 7) | (a >> 25);
    b += a * 0x85EBCA6Bu;
    (void)b;
}

} // namespace lp

#define LP_OPAQUE_TRUE  (::lp::opaque_true())
#define LP_OPAQUE_FALSE (::lp::opaque_false())
#define LP_CONST32(x)   (::lp::const32((x)))
#define LP_JUNK()       ::lp::junk()

// Wrap a real condition so there is always a spurious guard in the trace.
#define LP_IF_REAL(cond) if (LP_OPAQUE_TRUE && (cond))

#endif // LPROTECT_LP_OBF_H
