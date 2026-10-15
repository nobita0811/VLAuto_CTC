#pragma once

// Bat crash: ghi <thu muc module>\Crash\<Tag>_<ngay_gio>_<pid>.log (+ .dmp khi crash that).
// Dung chung cho VLAuto (VL20Pro.exe) va VLHook (VL20Ddll.dll trong Game) - moi module mot ban rieng.
//  - Unhandled exception: log (ma loi, module+offset, thanh ghi, call stack, cac module) + minidump.
//  - First-chance (tuy chon): ngoai le AV / chia 0 / lenh sai... trong module nay hoac khi module nay
//    dang tren stack - bat ca loi bi __try / he thong nuot (Win7 x64 nuot loi trong WndProc, TimerProc).
//  - Loi CRT (invalid parameter, pure call, abort) duoc doi thanh exception de ghi log.

void	CrashInstall(const char *szTag, BOOL bLogFirstChance);
void	CrashUninstall();						// Truoc khi FreeLibrary (VLHook)

// Dang lam gi (chuoi tinh) - ghi vao log khi crash
extern const char* volatile g_pszCrashStage;
#define CRASH_STAGE(s)	(g_pszCrashStage = (s))
