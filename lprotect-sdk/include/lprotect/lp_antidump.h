// LPROTECT - anti-dump (scrub PE headers in memory after startup).
//
// A process dumper (Scylla, PE-sieve, Task Manager "create dump file", etc.)
// reconstructs a runnable .exe from a running process by reading its PE headers
// out of memory. If you erase/corrupt those headers after the loader is done
// with them, a naive dump no longer rebuilds into a valid image.
//
//   lp::antidump_scrub();                    // safe default (see below)
//   lp::antidump_scrub(lp::ADUMP_AGGRESSIVE);// also wipes MZ/PE signatures
//
// WHAT IS SAFE TO ERASE
//   Once a module is loaded and mapped "as image", the running code does not
//   need the DOS stub, the Rich header, or the section header table anymore, so
//   wiping those (ADUMP_DEFAULT) is low-risk and already defeats most dumpers.
//
// WHAT IS RISKY (ADUMP_AGGRESSIVE)
//   Zeroing the DOS "MZ" magic or the "PE\0\0" signature makes the OS unable to
//   parse this module's headers afterwards. On x64 that breaks exception/stack
//   unwinding for frames not already cached (RtlLookupFunctionEntry fails), so a
//   C++ exception propagating THROUGH this module after the scrub can crash the
//   process. Use aggressive mode only if you call it late and no exception will
//   unwind through your module afterwards. GetModuleFileName and lp_static still
//   work (they use the PEB path / the on-disk file, not these headers).
//
// Call it LATE (end of startup), and remember it only protects THIS run in
// memory - pair it with lp_static for on-disk integrity.
#ifndef LPROTECT_LP_ANTIDUMP_H
#define LPROTECT_LP_ANTIDUMP_H

#include "lp_config.h"

namespace lp {

enum AntiDumpFlags : uint32_t {
    ADUMP_RICH       = 1u << 0, // DOS stub + Rich header (between DOS hdr and PE)
    ADUMP_SECTIONS   = 1u << 1, // the section header table
    ADUMP_SIZEOFIMG  = 1u << 2, // corrupt OptionalHeader.SizeOfImage
    ADUMP_PE_SIG     = 1u << 3, // zero "PE\0\0"  (risky - see header)
    ADUMP_DOS_MAGIC  = 1u << 4, // zero "MZ"      (risky - see header)

    ADUMP_DEFAULT    = ADUMP_RICH | ADUMP_SECTIONS,
    ADUMP_AGGRESSIVE = ADUMP_RICH | ADUMP_SECTIONS | ADUMP_SIZEOFIMG
                     | ADUMP_PE_SIG | ADUMP_DOS_MAGIC
};

// Scrub this module's in-memory PE headers. `module` defaults to the running
// .exe; pass a HMODULE to scrub a specific DLL. Returns the bitmask actually
// scrubbed (0 on failure / headers already gone).
uint32_t antidump_scrub(uint32_t flags = ADUMP_DEFAULT, void* module = nullptr);

// Report which header parts are STILL intact (ADUMP_DOS_MAGIC / ADUMP_PE_SIG /
// ADUMP_SECTIONS bits set = present). Useful to demonstrate the scrub.
uint32_t antidump_probe(void* module = nullptr);

} // namespace lp

#endif // LPROTECT_LP_ANTIDUMP_H
