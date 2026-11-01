// LPROTECT - online activation client (Cloudflare Workers + D1 backend).
//
// Adds a server round-trip on top of the offline Ed25519 licensing: the server
// (which holds the activation PRIVATE key) signs a short-lived, HWID-bound token
// that this client verifies OFFLINE with the embedded activation PUBLIC key
// (kActivationPublicKey in lp_activation.cpp). The token is cached, so the app
// keeps working offline until the token expires, then re-checks.
//
// What online activation adds over offline-only licensing:
//   * anti-sharing  - the server counts activated devices per key and caps them
//   * revocation    - the server can refuse to re-issue; short TTL makes it bite
//   * expiry control- server-side, independent of the perpetual offline key
//
// Security model (see server/ and docs/USAGE.md for the matching Worker):
//   * PRIVATE key lives only in the Worker secret, never in the client.
//   * Token is Ed25519-signed, so even a MITM cannot forge a valid response.
//   * A client nonce is echoed inside the signed token -> replies cannot be
//     replayed for another request/device.
//   * The server timestamp is remembered as a high-water mark -> an old captured
//     token cannot be replayed after a clock rollback.
//
// Transport: WinHTTP (HTTPS). Links winhttp.lib and bcrypt.lib in the final exe.
#ifndef LPROTECT_LP_ACTIVATION_H
#define LPROTECT_LP_ACTIVATION_H

#include "lp_config.h"

namespace lp {

enum class ActResult {
    Ok = 0,        // activated online just now, token valid
    OkOffline,     // a still-valid cached token was used (no network needed)
    Expired,       // token/license expired and could not be renewed
    Revoked,       // server revoked this key
    DeviceLimit,   // too many devices activated for this key
    NotFound,      // server does not know this key
    NetworkError,  // could not reach the server and no valid cache
    BadToken,      // server reply failed signature/sanity checks
    NotConfigured, // no endpoint set (see activation_set_endpoint / LP_ACTIVATION_URL)
    AuthRequired,  // license is account-bound; call activation_login first
    BadCredentials,// login failed (wrong email/password)
    SessionInUse   // single-session: another live instance already holds the lease
};

const char* act_result_str(ActResult r);

// Set the activation endpoint, e.g. L"https://lic.example.workers.dev/activate".
// Alternatively define LP_ACTIVATION_URL (narrow string) at build time.
void activation_set_endpoint(const wchar_t* url);

// Verify this machine's activation for `license_key`. Uses the cache first
// (OkOffline) and only calls the server when needed. `app_id` names the local
// cache slot (HKCU\Software\<app_id>\act).
ActResult activation_check(const char* license_key, const char* app_id = "LProtectApp");

// Drop the cached token (e.g. on "deactivate this device").
void activation_clear_cache(const char* app_id = "LProtectApp");

// ---- License tier / options (carried in the signed token) ----
// The server stamps these into the token, so the client can trust and display
// them. `quota` is an app-defined number (e.g. worker processes, jobs, seats);
// its meaning is entirely up to your app.
struct LicenseInfo {
    char     plan[16];   // tier name, null-terminated: "Basic", "Pro", ...
    uint32_t quota;      // app-defined numeric option (0 if unused)
    uint64_t expiry;     // subscription end, unix seconds (0 = perpetual)
    uint64_t token_exp;  // token freshness deadline (offline grace)
    uint32_t hwid;       // machine this token was issued to
    bool     expired;    // true if the subscription/token end has passed
    bool     valid;      // true if a verified token for THIS machine was found
};

// Read + verify the cached activation token and fill `out`. Returns true when a
// valid token for this machine exists (available after Ok or OkOffline). Does
// not touch the network.
bool activation_get_info(LicenseInfo& out, const char* app_id = "LProtectApp");

// Format "<plan>-<quota>-<YYYYDDMM>" into `out`, e.g. "Basic-8-20273101".
// Returns the number of characters written (excluding the NUL), 0 on error.
// The date part is YEAR,DAY,MONTH (matching the requested example); a perpetual
// license (expiry 0) prints "00000000".
int license_info_format(const LicenseInfo& in, char* out, size_t cap);

// ---- Account login (only needed for account-bound licenses) ----
// Log in with email + password. On success a session token is cached locally
// (HKCU\Software\<app_id>\act\ses) and automatically sent by activation_check.
// The password is sent over HTTPS to /login and never stored. Returns Ok,
// BadCredentials, NetworkError or NotConfigured.
ActResult activation_login(const char* email, const char* password,
                           const char* app_id = "LProtectApp");

// Forget the cached session (sign out). Does not touch the activation token.
void activation_logout(const char* app_id = "LProtectApp");

// Human-readable machine fingerprint for THIS computer: "<PC name>-<hash>",
// where <PC name> is plain text and <hash> is 16 hex chars of
// SHA-256(CPU brand | system-volume serial | first MAC). Shown to the user and
// sent to the server (field "mid") so one account can be bound to one machine.
// Cached after the first call. Returns the length written (excluding the NUL),
// 0 on error. Pass a buffer of >= 96 bytes.
int activation_machine_id(char* out, size_t cap);

// ---- Periodic re-check (background heartbeat) ----
// Called after each background re-check with its result (runs ON the background
// thread, so keep it short and do not call activation_stop_guard() from here).
typedef void (*ActCheckCallback)(ActResult result, void* user);

// Start a background thread that calls activation_check(license_key, app_id)
// over and over, sleeping a RANDOM interval in [min_seconds, max_seconds]
// before each call. The interval is re-rolled every round (and the FIRST wait
// is random too), so many copies of the app do not hit the server in lockstep
// -> the load stays flat instead of spiking. `cb` (optional) receives each
// result. Only one guard runs at a time; a second call replaces the first.
// Returns false if the thread could not be created.
//
// Defaults are 15-30 min: a good balance of revocation latency vs. server load.
// You still want a one-shot activation_check() at startup; this keeps it fresh.
// Call activation_stop_guard() before the app exits.
bool activation_start_guard(const char* license_key,
                            const char* app_id = "LProtectApp",
                            ActCheckCallback cb = nullptr, void* user = nullptr,
                            unsigned min_seconds = 15 * 60,
                            unsigned max_seconds = 30 * 60);

// Stop the background re-check thread and wait for it to exit. Safe to call when
// no guard is running. Do NOT call from inside the ActCheckCallback.
void activation_stop_guard();

// ---------------------------------------------------------------------------
// Single-session enforcement ("only one live client at a time").
//
// Scope: one running instance per ACCOUNT when the key is account-bound (bind it
// with `UPDATE licenses SET account_email=...` and call activation_login first),
// otherwise one per key. Policy is OLDEST-WINS: while one instance is live, a
// second one is refused (SessionInUse); the holder keeps the lease by
// heartbeating, and frees it on clean exit so a restart is instant.
//
// Typical startup:
//     if (lp::activation_login(email, pass, APP) != lp::ActResult::Ok) { ... }
//     lp::ActResult r = lp::activation_session_begin(key, APP);
//     if (r == lp::ActResult::SessionInUse) { /* "dang chay o may khac" */ exit; }
//     if (r != lp::ActResult::Ok)           { /* handle revoked/expired/... */ }
//     lp::activation_start_session_guard(key, APP, on_lost, nullptr);
//     ... on shutdown:
//     lp::activation_session_end(key, APP);     // release the lease
// ---------------------------------------------------------------------------

// Acquire the single-session lease. ALWAYS contacts the server (no offline
// grace: the whole point is to learn, now, whether another instance is live).
// Stores a lease session id locally (HKCU\Software\<app_id>\act\sid) and caches
// the signed token like activation_check. Returns:
//   Ok            - this instance now holds the lease (safe to run)
//   SessionInUse  - another live instance already holds it -> do not run
//   Revoked / Expired / NotFound / AuthRequired / NetworkError / NotConfigured
ActResult activation_session_begin(const char* license_key,
                                   const char* app_id = "LProtectApp");

// Start a short-interval heartbeat that renews the lease. If the lease is lost
// (SessionInUse) or the license becomes invalid (Revoked/Expired), `cb` is
// called with that result and the heartbeat STOPS itself — react by shutting the
// app down (delayed/indirect, as elsewhere in LPROTECT). Transient network
// errors do not stop it. One session guard runs at a time; a second call
// replaces the first. `interval_seconds` should be well under the server's
// 150s lease (default 60s). Returns false if the thread could not start.
bool activation_start_session_guard(const char* license_key,
                                    const char* app_id = "LProtectApp",
                                    ActCheckCallback cb = nullptr, void* user = nullptr,
                                    unsigned interval_seconds = 60);

// Stop the session heartbeat (does not release the lease). Safe if none runs.
void activation_stop_session_guard();

// Release the single-session lease on the server (call on clean shutdown) so the
// next login succeeds immediately instead of waiting out the lease. Also stops
// the session guard and clears the local sid. Best-effort (ignores network
// errors): a crash simply lets the lease expire server-side.
void activation_session_end(const char* license_key,
                            const char* app_id = "LProtectApp");

} // namespace lp

#endif // LPROTECT_LP_ACTIVATION_H
