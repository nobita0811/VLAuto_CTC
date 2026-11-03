// License.cpp : doc goi license tu token da ky va cung cap cac co gate tinh nang.
#include "stdafx.h"
#include "License.h"

#if LICENSE_REQUIRED
#include "lprotect/lprotect.h"
#include <string.h>
#include <ctype.h>

namespace {

lp::LicenseInfo g_info = {};
bool            g_have = false;   // da doc duoc token hop le chua

// Tim chuoi con, khong phan biet hoa thuong (plan it ky tu nen quet thang).
bool ContainsCI(const char* s, const char* sub)
{
    if (!s || !sub || !*sub) return false;
    size_t n = strlen(sub);
    for (const char* p = s; *p; ++p)
    {
        size_t i = 0;
        for (; i < n && p[i]; ++i)
            if (toupper((unsigned char)p[i]) != toupper((unsigned char)sub[i]))
                break;
        if (i == n) return true;
    }
    return false;
}

} // namespace

void LicenseCapture(const char* app_id)
{
    lp::LicenseInfo info;
    if (lp::activation_get_info(info, app_id) && info.valid)
    {
        g_info = info;
        g_have = true;
    }
}

bool LicenseHasQuest()
{
    if (!g_have || !g_info.valid) return false;   // khong co license hop le -> khong mo
    return ContainsCI(g_info.plan, "PRO");
}

bool LicenseUnlimitedLogin()
{
    if (!g_have || !g_info.valid) return false;
    return strstr(g_info.plan, "++") != NULL;     // hau to ++ (++ chi co o dang ascii nay)
}

int LicenseQuota()
{
    if (!g_have) return 0;                         // 0 = khong gioi han
    return (int)g_info.quota;
}

CString LicensePlanName()
{
    if (!g_have || !g_info.plan[0]) return _T("?");
    return CString(g_info.plan);                   // char[16] -> CString
}

unsigned long long LicenseExpiry()
{
    return g_have ? g_info.expiry : 0ULL;
}

#endif // LICENSE_REQUIRED
