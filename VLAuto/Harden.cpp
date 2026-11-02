// Harden.cpp : xem Harden.h. Khung phan ung tre - doc lap lprotect nen co the dung
// lai cho VLHook sau nay (VLHook khong link lprotect).
#include "stdafx.h"
#include "Harden.h"

#if LP_HARDEN
#include <windows.h>
#include <stdio.h>

#if !LP_HARDEN_SAFE
namespace {
    // Co tamper luu dang gia tri "la" (khong phai 0/1) de tranh so sanh lo lieu.
    volatile LONG g_trip = 0;
    const  LONG   kTripMark = 0x5EA17C3D;   // "da tamper"
}
#endif

namespace hard {

void Flag(const char* where)
{
#if LP_HARDEN_SAFE
    // Che do noi bo: chi ghi log, khong lam hong gi.
    char buf[160];
    _snprintf_s(buf, sizeof buf, _TRUNCATE, "[LP_HARDEN_SAFE] tamper: %s\n",
                where ? where : "?");
    OutputDebugStringA(buf);
#else
    (void)where;
    InterlockedExchange(&g_trip, kTripMark);   // dat co; degrade xay ra sau
#endif
}

bool Tripped()
{
#if LP_HARDEN_SAFE
    return false;                              // SAFE: khong bao gio degrade
#else
    return InterlockedCompareExchange(&g_trip, 0, 0) == kTripMark;
#endif
}

unsigned int PoisonMask()
{
#if LP_HARDEN_SAFE
    return 0u;
#else
    return Tripped() ? 0xC7A93E51u : 0u;
#endif
}

} // namespace hard
#endif // LP_HARDEN
