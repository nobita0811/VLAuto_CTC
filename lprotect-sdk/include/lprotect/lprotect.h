// LPROTECT - lightweight native protection library for your own Windows software.
//
// Umbrella header. Include this to pull in every module, or include individual
// headers under <lprotect/...> if you only need part of it.
//
// Scope: defensive self-protection of software YOU own/ship. All techniques here
// are standard, publicly documented, and meant to raise the cost of reverse
// engineering your own binaries - not to evade security tooling maliciously.
//
// Build: C++17, MSVC (x86/x64). Some modules are header-only; others have a .cpp
// in src/ compiled into the lprotect static library.

#ifndef LPROTECT_LPROTECT_H
#define LPROTECT_LPROTECT_H

#define LPROTECT_VERSION_MAJOR 0
#define LPROTECT_VERSION_MINOR 1
#define LPROTECT_VERSION_PATCH 0

#if !defined(_WIN32)
#  error "LPROTECT currently targets Windows (PE) only."
#endif

#include "lprotect/lp_config.h"
#include "lprotect/lp_string.h"
#include "lprotect/lp_obf.h"
#include "lprotect/lp_antidebug.h"
#include "lprotect/lp_integrity.h"
#include "lprotect/lp_static.h"
#include "lprotect/lp_antidump.h"
#include "lprotect/lp_ed25519.h"
#include "lprotect/lp_license.h"
#include "lprotect/lp_activation.h"
#include "lprotect/lp_vm.h"

#endif // LPROTECT_LPROTECT_H
