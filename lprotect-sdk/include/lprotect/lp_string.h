// LPROTECT - compile-time string encryption.
//
// Plaintext literals are a goldmine for reverse engineers ("strings.exe").
// LP_STR("...") stores the bytes XOR-encrypted in the binary and decrypts them
// into a short-lived, auto-wiping buffer at the point of use.
//
//   puts(LP_STR("secret message").c_str());
//   auto s = LP_STR("path\\to\\thing");   // s.c_str() valid while s is alive
//
// Notes / limits:
//  * This is obfuscation, not cryptography. A determined analyst with a debugger
//    can still read the decrypted buffer at runtime. The goal is to defeat static
//    string scanning and raise effort, nothing more.
//  * Keys are per-call-site (see ct_key) so identical literals encrypt differently.
//  * Requires C++17.
#ifndef LPROTECT_LP_STRING_H
#define LPROTECT_LP_STRING_H

#include "lp_config.h"
#include <utility>
#include <cstring>

namespace lp {

// Keystream byte for position i given a 32-bit site key.
constexpr uint8_t ks_byte(uint32_t key, size_t i) {
    uint32_t m = ct_mix(key + static_cast<uint32_t>(i) * 2246822519u);
    return static_cast<uint8_t>((m >> ((i & 3) * 8)) & 0xFF);
}

// RAII decrypted buffer: plaintext lives here only while the object is alive,
// then is overwritten. c_str() gives a null-terminated view.
template <size_t N>
class secure_string {
public:
    secure_string() { buf_[0] = 0; }
    ~secure_string() {
        // volatile loop so the wipe is not optimized away.
        volatile char* p = buf_;
        for (size_t i = 0; i < N; ++i) p[i] = 0;
    }
    secure_string(const secure_string&) = delete;
    secure_string& operator=(const secure_string&) = delete;
    // Movable so decrypt() can return by value (NRVO is not guaranteed).
    secure_string(secure_string&& o) noexcept {
        for (size_t i = 0; i < N; ++i) buf_[i] = o.buf_[i];
    }

    const char* c_str() const { return buf_; }
    char*       data()        { return buf_; }
    size_t      size()  const { return N ? N - 1 : 0; }

private:
    char buf_[N];
};

// Encrypted-at-compile-time string holder.
template <size_t N, uint32_t KEY>
class crypt_string {
public:
    template <size_t... I>
    constexpr crypt_string(const char (&s)[N], std::index_sequence<I...>)
        : data_{ static_cast<char>(s[I] ^ ks_byte(KEY, I))... } {}

    // Decrypt into a fresh auto-wiping buffer.
    LP_NOINLINE secure_string<N> decrypt() const {
        secure_string<N> out;
        char* o = out.data();
        for (size_t i = 0; i < N; ++i) {
            // read through volatile so the whole thing isn't constant-folded.
            volatile char enc = data_[i];
            o[i] = static_cast<char>(enc ^ ks_byte(KEY, i));
        }
        return out;
    }

private:
    char data_[N];
};

template <uint32_t KEY, size_t N>
constexpr crypt_string<N, KEY> make_crypt_string(const char (&s)[N]) {
    return crypt_string<N, KEY>(s, std::make_index_sequence<N>{});
}

} // namespace lp

// LP_STR expands to a temporary secure_string. Bind it (auto x = LP_STR(..))
// if you need the pointer to outlive the full expression.
//
// The per-site key is derived from __COUNTER__ only. We deliberately do NOT use
// __LINE__: under MSVC /ZI (Edit-and-Continue) __LINE__ is a runtime value, not
// a constant, which would break this compile-time constant expression.
// __COUNTER__ is captured once via the _IMPL indirection so it is stable.
#define LP_STR_IMPL(str, ctr)                                                  \
    ([]() {                                                                     \
        constexpr auto _lp_cs =                                                 \
            ::lp::make_crypt_string<::lp::ct_key((ctr), (ctr))>(str);           \
        return _lp_cs.decrypt();                                                \
    }())
#define LP_STR(str) LP_STR_IMPL(str, __COUNTER__)

#endif // LPROTECT_LP_STRING_H
