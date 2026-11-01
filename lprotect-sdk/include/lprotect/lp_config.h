// LPROTECT - global configuration & small shared utilities.
#ifndef LPROTECT_LP_CONFIG_H
#define LPROTECT_LP_CONFIG_H

#include <cstdint>
#include <cstddef>

// ---------------------------------------------------------------------------
// Compiler intrinsics / attributes
// ---------------------------------------------------------------------------
#if defined(_MSC_VER)
#  define LP_FORCEINLINE __forceinline
#  define LP_NOINLINE    __declspec(noinline)
#else
#  define LP_FORCEINLINE inline __attribute__((always_inline))
#  define LP_NOINLINE    __attribute__((noinline))
#endif

// Prevents the optimizer from treating a value as a known constant.
// Useful to stop constant-folding of decrypted data / opaque predicates.
namespace lp {
template <typename T>
LP_FORCEINLINE T opaque(T v) {
#if defined(_MSC_VER)
    volatile T sink = v;
    return sink;
#else
    asm volatile("" : "+r"(v) : :);
    return v;
#endif
}
} // namespace lp

// ---------------------------------------------------------------------------
// Per-build seed. Change this (or generate it per build) so that polymorphic
// elements (string keys, vm layout) differ between releases.
// ---------------------------------------------------------------------------
#ifndef LP_BUILD_SEED
#  define LP_BUILD_SEED 0xA5C3F17Bu
#endif

// A tiny constexpr PRNG so compile-time keys vary by location.
namespace lp {
constexpr uint32_t ct_mix(uint32_t x) {
    x ^= x >> 16; x *= 0x7feb352du;
    x ^= x >> 15; x *= 0x846ca68bu;
    x ^= x >> 16;
    return x;
}
// Derive a per-site key from __LINE__ and __COUNTER__ plus the build seed.
constexpr uint32_t ct_key(uint32_t line, uint32_t counter) {
    return ct_mix(line * 2654435761u + counter * 40503u + LP_BUILD_SEED);
}
} // namespace lp

#endif // LPROTECT_LP_CONFIG_H
