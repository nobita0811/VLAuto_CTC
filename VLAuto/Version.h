#pragma once

#define LOGIN_PARAM 1
#define CLIENT_VERSION		"0.1"
#define HASH_CODE			0x4683

#define VLVERSION	"1.02"

#define QUANGCAOAUTO	1

#define DebugSuaDo 1
#define DebugMoveMode 0
#define DebugCuuSat 0
#define DebugDanhQuai 0
#define DebugUserCheckAccount 1
// Ban build (dung chung VLAuto + VLHook qua GameDef.h). Sua 2 dong duoi roi build lai,
// hoac chay BuildRelease.ps1 (truyen /DADMIN_MODE=.. /DADMIN_TESTER=.. qua bien CL, khong sua file nay)
//   ADMIN_MODE 0                  : ban nguoi dung - can license, bo cua so DEBUG / lenh debug / phim test
//   ADMIN_MODE 1, ADMIN_TESTER 1  : ban tester online - co DEBUG, can license
//   ADMIN_MODE 1, ADMIN_TESTER 0  : ban tester offline - co DEBUG, khong can license
#ifndef ADMIN_MODE
#define ADMIN_MODE		1
#endif
#ifndef ADMIN_TESTER
#define ADMIN_TESTER	0
#endif
#define LICENSE_REQUIRED	(!ADMIN_MODE || ADMIN_TESTER)
// Ban Tester Offline (ADMIN_MODE 1, ADMIN_TESTER 0): co nut chan doan tai cho (vd: kiem tra TLS 1.2).
#define TEST_OFFLINE_BUILD	(ADMIN_MODE && !ADMIN_TESTER)

// ---- Khung anti-tamper (Phase 0) ----
// LP_HARDEN: cong tac tong bat/tat MOI lop anti-tamper (integrity P2, anti-debug
//   P3, anti-dump P4). Chi bat khi LICENSE_REQUIRED => ban User + Tester online
//   co; ban Tester offline (LICENSE_REQUIRED=0) KHONG dinh bat ky anti-tamper nao.
// LP_HARDEN_SAFE: che do thu nghiem noi bo - phat hien thi CHI GHI LOG
//   (OutputDebugString), KHONG giet/degrade. Mac dinh theo ADMIN_TESTER nen:
//     User          (0,0): HARDEN=1, SAFE=0 -> phan ung that (degrade tre).
//     Tester online (1,1): HARDEN=1, SAFE=1 -> chi log, tester khong bi giet.
//     TesterOffline (1,0): HARDEN=0        -> sach hoan toan.
// Ghi de duoc qua bien CL (/DLP_HARDEN=.. /DLP_HARDEN_SAFE=..) khi can.
#ifndef LP_HARDEN
  #if LICENSE_REQUIRED
    #define LP_HARDEN		1
  #else
    #define LP_HARDEN		0
  #endif
#endif
#ifndef LP_HARDEN_SAFE
  #define LP_HARDEN_SAFE	ADMIN_TESTER
#endif

#if !ADMIN_MODE
#define VLADlgTitle "VLCTCPro " VLVERSION
#elif ADMIN_TESTER
#define VLADlgTitle "VLCTCPro " VLVERSION " [Tester]"
#else
#define VLADlgTitle "VLCTCPro " VLVERSION " [Tester Offline]"
#endif
#define VLAWinClass "VLCTCAutoClass"
#define LgnDlgTitle "VLCTCAuto - Login"
#define VLADefIcon "res\\VLAuto.ico"

// =============
#define _TVLAIniFile _T("%s\\VLCTCPro.ini")
#define _TVLAWinClass _T("VLAutoClass")
#define _TLgnDlgTitle _T("VLAuto - Login")
#define _TTrayToolTip _T("VLAuto (Cong Thanh Chien)")
#define _TVLGWinClass _T("AutoLoginClass")
#define _TGameY_Exe _T("game_y.exe")
#define _TGameY_Exe1 _T("game_y.exe")
#define _TGameY_Exe2 _T("gamethienha_y.exe")
