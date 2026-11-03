// License.h : thong tin goi license cho ban User + cac co gate tinh nang.
//
// Du lieu goi (plan / quota / expiry) nam trong token DA KY (Ed25519) cua server,
// doc qua lp::activation_get_info() - khong the gia mao/patch o client. Anh xa goi:
//   BASIC    : chi train (an tab Nhiem vu)
//   PRO      : BASIC + Nhiem vu
//   BASIC++  : BASIC + Login khong gioi han (LG_FAKE_VOLSN)
//   PRO++    : PRO   + Login khong gioi han
// Quy tac: plan chua "PRO" -> co Nhiem vu; plan co hau to "++" -> Login khong gioi han.
// quota = so account chay cung luc toi da cua Auto (0 = khong gioi han).
//
// Ban Tester offline (!LICENSE_REQUIRED) khong co license -> mo moi tinh nang.
#pragma once
#include "Version.h"

#if LICENSE_REQUIRED

// Doc + luu thong tin license tu token da ky (goi 1 lan sau activation_session_begin Ok).
// app_id phai trung voi app_id dung khi kich hoat (cache token nam theo app_id).
void LicenseCapture(const char* app_id);

bool               LicenseHasQuest();        // plan chua "PRO" -> co tab/chuc nang Nhiem vu
bool               LicenseUnlimitedLogin();  // plan co "++"   -> bat LG_FAKE_VOLSN
int                LicenseQuota();           // so account chay cung luc toi da (0 = khong gioi han)
CString            LicensePlanName();        // ten goi de hien thi ("BASIC", "PRO++", ...)
unsigned long long LicenseExpiry();          // unix seconds het han (0 = vinh vien)

#else   // Tester offline: khong rang buoc license -> mo het

inline void               LicenseCapture(const char*) {}
inline bool               LicenseHasQuest()       { return true; }
inline bool               LicenseUnlimitedLogin() { return true; }
inline int                LicenseQuota()          { return 0; }     // 0 = khong gioi han
inline CString            LicensePlanName()       { return _T("Tester Offline"); }
inline unsigned long long LicenseExpiry()         { return 0; }

#endif
