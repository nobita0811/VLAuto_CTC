#include "stdafx.h"
#include "../GameDef.h"
#include "GameBase.h"
#include "JxCore.h"
#include "S3Client.h"
#include "VLClasses.h"
#include "VLDebug.h"
#include <psapi.h>

// Tab Giam cau hinh cua cua so DEBUG: giam RAM, FPS ve, tat hinh, an nguoi choi / thoi tiet / bong dong / am thanh.
// Thu truoc o day, chay on thi moi dua vao tab Co ban.
//  - GameLoop 0x5A9160 (15-10): chi ve khi so khung*1000 <= tick*[LCF_PAINT_FPS_ADD] => giam FPS khong anh huong logic.
//    (10-10 luu ms/khung = 1000/PaintFps, 15-10 luu thang PaintFps)
//  - [LCF_INACTIVE_ADD] != 0 va qua 10s => GameLoop chi RepresentBegin/End (khong ve). Engine ghi lai 0 khi cua so
//    duoc kich hoat => DebugLowCfgBreathe ghi lai lien tuc khi dang tat hinh.
//  - Giam RAM = SetProcessWorkingSetSize(-1, -1): day trang nho ra standby/pagefile, game van giu bo nho da cap phat.

#define LCF_LOW_FPS			5		// FPS khi bam "Ap dung tat ca" ma Idx = 0
#define LCF_LOW_TRIM_SEC	60		// Tu giam RAM khi bam "Ap dung tat ca"

#define PAINT_FPS		(*(int*)LCF_PAINT_FPS_ADD)
#define INACTIVE_TICK	(*(DWORD*)LCF_INACTIVE_ADD)
#define VISIBLE_NUM		(*(int*)LCF_VISIBLE_ADD)
#define SND_VOLUME		(*(int*)(LCF_KOPTION_ADD + 0))
#define MUSIC_VOLUME	(*(int*)(LCF_KOPTION_ADD + 4))
#define WEATHER_ON		(*(BYTE*)(SCP_BASE_ADD + SCP_WEATHER_OFF))
#define DYNALIGHT_ON	(*(BYTE*)(SCP_BASE_ADD + SCP_DYNALIGHT_OFF))

static int		s_nOrgPaintFps = 0;			// 0 = chua doi
static int		s_nOrgVisible = -1;			// -1 = chua doi
static int		s_nOrgWeather = -1;
static int		s_nOrgDynaLight = -1;
static BOOL		s_bSoundChanged = FALSE;
static BOOL		s_bScreenOff = FALSE;
static int		s_nAutoTrimSec = 0;
static DWORD	s_dwNextTrim = 0;
static int		s_nTrimCount = 0;
static SIZE_T	s_nLastBefore = 0, s_nLastAfter = 0;

typedef BOOL (WINAPI *PFN_GETPMI)(HANDLE, PPROCESS_MEMORY_COUNTERS, DWORD);

static BOOL GetMemInfo(PROCESS_MEMORY_COUNTERS *pmc)
{
	static PFN_GETPMI pfn = NULL;
	if (!pfn)
	{
		HMODULE hPsapi = LoadLibraryA("psapi.dll");
		if (hPsapi) pfn = (PFN_GETPMI)GetProcAddress(hPsapi, "GetProcessMemoryInfo");
		if (!pfn) return FALSE;
	}
	memset(pmc, 0, sizeof(*pmc));
	pmc->cb = sizeof(*pmc);
	return pfn(GetCurrentProcess(), pmc, sizeof(*pmc));
}

static SIZE_T GetWorkingSet()
{
	PROCESS_MEMORY_COUNTERS pmc;
	return GetMemInfo(&pmc) ? pmc.WorkingSetSize : 0;
}

static void TrimRam(BOOL bOut)
{
	s_nLastBefore = GetWorkingSet();
	BOOL bOK = SetProcessWorkingSetSize(GetCurrentProcess(), (SIZE_T)-1, (SIZE_T)-1);
	s_nLastAfter = GetWorkingSet();
	s_nTrimCount++;
	if (!bOut) return;
	if (!bOK)
	{
		DbgOut("LOI: SetProcessWorkingSetSize that bai (GetLastError = %d)\n", GetLastError());
		return;
	}
	DbgOut("Giam RAM: Working set %u MB -> %u MB\n", (UINT)(s_nLastBefore >> 20), (UINT)(s_nLastAfter >> 20));
	g_DbgReply.nResult[0] = (int)(s_nLastBefore >> 20);
	g_DbgReply.nResult[1] = (int)(s_nLastAfter >> 20);
	g_DbgReply.bOK = TRUE;
}

static BOOL CheckCore()
{
	if (g_pCoreShell) return TRUE;
	DbgOut("LOI: g_pCoreShell = NULL (chua vao game)\n");
	return FALSE;
}

static void SetFps(int nFps)
{
	if (nFps <= 0)
	{
		if (s_nOrgPaintFps) PAINT_FPS = s_nOrgPaintFps;
		s_nOrgPaintFps = 0;
	}
	else
	{
		if (!s_nOrgPaintFps) s_nOrgPaintFps = PAINT_FPS > 0 ? PAINT_FPS : LCF_PAINT_FPS_DEF;
		PAINT_FPS = nFps > 1000 ? 1000 : nFps;
	}
}

static void SetScreenOff(BOOL bOff)
{
	s_bScreenOff = bOff;
	if (bOff)
	{
		if (INACTIVE_TICK == 0) INACTIVE_TICK = 1;
	}
	else if (INACTIVE_TICK == 1)
		INACTIVE_TICK = 0;
}

static void SetVisible(int nNum)
{
	if (s_nOrgVisible < 0) s_nOrgVisible = VISIBLE_NUM;
	g_pCoreShell->OperationRequest(GOI_OPTION_SETTING, OPTION_VISIBLE_NUM, nNum);
}

static void SetWeather(BOOL bOn)
{
	if (s_nOrgWeather < 0) s_nOrgWeather = WEATHER_ON;
	g_pCoreShell->OperationRequest(GOI_OPTION_SETTING, OPTION_WEATHER, bOn);
}

static void SetDynaLight(BOOL bOn)
{
	if (s_nOrgDynaLight < 0) s_nOrgDynaLight = DYNALIGHT_ON;
	g_pCoreShell->OperationRequest(GOI_OPTION_SETTING, OPTION_DYNALIGHT, bOn);
}

static void SetSound(int nVolume)
{
	s_bSoundChanged = TRUE;
	g_pCoreShell->OperationRequest(GOI_OPTION_SETTING, OPTION_MUSIC_VALUE, nVolume);
	g_pCoreShell->OperationRequest(GOI_OPTION_SETTING, OPTION_SOUND_VALUE, nVolume);
}

static void SetAutoTrim(int nSec)
{
	s_nAutoTrimSec = nSec > 0 ? nSec : 0;
	s_dwNextTrim = GetTickCount() + s_nAutoTrimSec * 1000;
}

static void LowCfgStatus()
{
	int nFps = PAINT_FPS;
	int nMs = nFps > 0 ? 1000 / nFps : 0;
	DbgOut("FPS ve: %d FPS (~%d ms/khung)%s\n", nFps, nMs,
		s_nOrgPaintFps ? "" : "  - goc (config.ini [Client] PaintFps)");
	if (s_nOrgPaintFps)
		DbgOut("        goc %d FPS\n", s_nOrgPaintFps);
	DWORD dwInactive = INACTIVE_TICK;
	DbgOut("Tat hinh: %s   [0x%08X] = %u%s\n", s_bScreenOff ? "BAT" : "tat", LCF_INACTIVE_ADD, dwInactive,
		dwInactive == 0 ? " (dang ve)" : (GetTickCount() - dwInactive >= 10000 ? " (khong ve)" : " (con < 10s moi ngung ve)"));
	DbgOut("Nguoi choi hien thi (VisibleNum): %d%s\n", VISIBLE_NUM, VISIBLE_NUM <= 0 ? " - AN nguoi choi khac" : "");
	DbgOut("Thoi tiet: %d   Bong dong: %d\n", WEATHER_ON, DYNALIGHT_ON);
	DbgOut("KOption: SndVolume %d  MusicVolume %d (gia tri trong game, da doi thang)%s\n", SND_VOLUME, MUSIC_VOLUME,
		s_bSoundChanged ? "  - Auto da doi am luong" : "");

	PROCESS_MEMORY_COUNTERS pmc;
	if (GetMemInfo(&pmc))
		DbgOut("RAM: Working set %u MB (dinh %u MB)  Commit (Pagefile) %u MB\n", (UINT)(pmc.WorkingSetSize >> 20),
			(UINT)(pmc.PeakWorkingSetSize >> 20), (UINT)(pmc.PagefileUsage >> 20));
	else
		DbgOut("RAM: khong doc duoc GetProcessMemoryInfo\n");
	if (s_nAutoTrimSec)
		DbgOut("Tu giam RAM: moi %ds, lan sau sau %ds\n", s_nAutoTrimSec, (int)(s_dwNextTrim - GetTickCount()) / 1000);
	else
		DbgOut("Tu giam RAM: tat\n");
	if (s_nTrimCount)
		DbgOut("Da giam %d lan, lan cuoi %u MB -> %u MB\n", s_nTrimCount, (UINT)(s_nLastBefore >> 20), (UINT)(s_nLastAfter >> 20));
	g_DbgReply.nResult[0] = nMs;
	g_DbgReply.nResult[1] = (int)(GetWorkingSet() >> 20);
	g_DbgReply.bOK = TRUE;
}

static void LowCfgRestore(BOOL bCallGame)
{
	SetFps(0);
	SetScreenOff(FALSE);
	s_nAutoTrimSec = 0;
	if (s_nOrgVisible >= 0)
	{
		VISIBLE_NUM = s_nOrgVisible;	// OPTION_VISIBLE_NUM chi ghi bien nay
		s_nOrgVisible = -1;
	}
	if (!bCallGame || !g_pCoreShell) return;
	if (s_nOrgWeather >= 0)
		g_pCoreShell->OperationRequest(GOI_OPTION_SETTING, OPTION_WEATHER, s_nOrgWeather);
	if (s_nOrgDynaLight >= 0)
		g_pCoreShell->OperationRequest(GOI_OPTION_SETTING, OPTION_DYNALIGHT, s_nOrgDynaLight);
	if (s_bSoundChanged)
	{
		g_pCoreShell->OperationRequest(GOI_OPTION_SETTING, OPTION_MUSIC_VALUE, 100);
		g_pCoreShell->OperationRequest(GOI_OPTION_SETTING, OPTION_SOUND_VALUE, 100);
	}
	s_nOrgWeather = s_nOrgDynaLight = -1;
	s_bSoundChanged = FALSE;
}

// 200ms (TimerProc, ca khi tam dung)
void DebugLowCfgBreathe()
{
	if (s_bScreenOff && INACTIVE_TICK == 0)
		INACTIVE_TICK = 1;	// Engine (KWin32App::MsgProc) ghi 0 khi cua so game duoc kich hoat
	if (s_nAutoTrimSec && (int)(GetTickCount() - s_dwNextTrim) >= 0)
	{
		TrimRam(FALSE);
		s_dwNextTrim = GetTickCount() + s_nAutoTrimSec * 1000;
	}
}

// Truoc khi FreeLibrary: tra lai FPS, hinh, nguoi choi (chi ghi bien, khong goi ham game)
void DebugLowCfgUnhook()
{
	LowCfgRestore(FALSE);
}

BOOL DebugLowCfgCmd(const DEBUG_PARAM *pParam)
{
	const int *P = pParam->nParam;
	switch (pParam->nCmd)
	{
	case dbg_lc_status:
		LowCfgStatus();
		break;
	case dbg_lc_trim:
		TrimRam(TRUE);
		break;
	case dbg_lc_autotrim:
		SetAutoTrim(P[2]);
		if (s_nAutoTrimSec)
			DbgOut("Tu giam RAM moi %d giay (TimerProc 200ms, ca khi tam dung)\n", s_nAutoTrimSec);
		else
			DbgOut("Tat tu giam RAM\n");
		g_DbgReply.bOK = TRUE;
		break;
	case dbg_lc_fps:
		SetFps(P[2]);
		DbgOut("FPS ve = %s: [0x%08X] = %d FPS\n", P[2] > 0 ? "Idx" : "goc", LCF_PAINT_FPS_ADD, PAINT_FPS);
		g_DbgReply.nResult[0] = PAINT_FPS > 0 ? 1000 / PAINT_FPS : 0;	// giu don vi ms/khung cho VLAuto
		g_DbgReply.bOK = TRUE;
		break;
	case dbg_lc_screen:
		SetScreenOff(P[2] != 0);
		DbgOut(s_bScreenOff ? "Tat hinh: BAT - game ngung ve ngay (logic van chay). Idx = 0 de ve lai\n"
			: "Tat hinh: tat - game ve lai binh thuong\n");
		g_DbgReply.bOK = TRUE;
		break;
	case dbg_lc_players:
		if (!CheckCore()) break;
		SetVisible(P[2]);
		DbgOut("OperationRequest(GOI_OPTION_SETTING, OPTION_VISIBLE_NUM, %d) => VisibleNum = %d%s\n", P[2], VISIBLE_NUM,
			VISIBLE_NUM <= 0 ? " (an nguoi choi khac)" : "");
		g_DbgReply.bOK = TRUE;
		break;
	case dbg_lc_weather:
		if (!CheckCore()) break;
		SetWeather(P[2] != 0);
		DbgOut("OperationRequest(GOI_OPTION_SETTING, OPTION_WEATHER, %d) => m_bEnableWeather = %d\n", P[2] != 0, WEATHER_ON);
		g_DbgReply.bOK = TRUE;
		break;
	case dbg_lc_dynalight:
		if (!CheckCore()) break;
		SetDynaLight(P[2] != 0);
		DbgOut("OperationRequest(GOI_OPTION_SETTING, OPTION_DYNALIGHT, %d) => bong dong = %d\n", P[2] != 0, DYNALIGHT_ON);
		g_DbgReply.bOK = TRUE;
		break;
	case dbg_lc_sound:
		if (!CheckCore()) break;
		SetSound(P[2] < 0 ? 0 : (P[2] > 100 ? 100 : P[2]));
		DbgOut("Am luong nhac + tieng = %d => KOption Snd %d  Music %d\n", P[2], SND_VOLUME, MUSIC_VOLUME);
		g_DbgReply.bOK = TRUE;
		break;
	case dbg_lc_lowall:
		if (!CheckCore()) break;
		SetFps(P[2] > 0 ? P[2] : LCF_LOW_FPS);
		SetVisible(0);
		SetWeather(FALSE);
		SetDynaLight(FALSE);
		SetSound(0);
		SetAutoTrim(LCF_LOW_TRIM_SEC);
		DbgOut("Ap dung: FPS %d, an nguoi choi, tat thoi tiet + bong dong + am thanh, tu giam RAM moi %ds\n",
			P[2] > 0 ? P[2] : LCF_LOW_FPS, LCF_LOW_TRIM_SEC);
		TrimRam(TRUE);
		break;
	case dbg_lc_restore:
		if (!CheckCore()) break;
		LowCfgRestore(TRUE);
		DbgOut("Da tra lai FPS, hinh, nguoi choi, thoi tiet, bong dong; am luong = 100 neu Auto da doi; tat tu giam RAM\n");
		LowCfgStatus();
		break;
	default:
		return FALSE;
	}
	return TRUE;
}
