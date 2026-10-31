#include "stdafx.h"
#include "../GameDef.h"
#include "PELib.h"
#include "AntiRE.h"
#include "GameBase.h"
#include "JxCore.h"
#include "VLClasses.h"
#include "VLDebug.h"
#include "VLHook.h"
#include "../CrashHandler.h"

#define WM_HOOKEX			WM_USER + 99
#define WM_HOOK_MAIN		WM_USER + 100
#define WM_HOOK_BASIC		WM_USER + 101
#define WM_HOOK_RECOVER		WM_USER + 102
#define WM_HOOK_ATTACK		WM_USER + 103
#define WM_HOOK_COLLECT		WM_USER + 104
#define WM_HOOK_MOVE		WM_USER + 105
#define WM_HOOK_PARTY		WM_USER + 106
#define WM_HOOK_MAP			WM_USER + 107
#define WM_HOOK_YESOU		WM_USER + 108
#define WM_HOOK_UTIL		WM_USER + 109
#define WM_HOOK_EXDATA		WM_USER + 110
#define WM_HOOK_DEBUG		WM_USER + 111

#define SHIFTED				0x8000
#define SYS_TIMER			40		// ~50ms
#define MAX_PLAY_TIME		36000	// 30*60*20 = 30 minutes
#define MAX_PAUSE_TIME		4		// ~200ms

HINSTANCE			g_hDll;
WNDPROC				OldWndProc = NULL;
LRESULT CALLBACK	NewWndProc(HWND, UINT, WPARAM, LPARAM);
VOID CALLBACK		TimerProc(HWND hWnd, UINT uMsg, UINT_PTR idEvent, DWORD dwTime);

UINT_PTR			nTimerID = 0;
BOOL				bHooked = 0;
BOOL				g_SysPause = 0;
BOOL				bPausing = 0;
UINT				nPausingCount = 0;
int					g_RunningTimer = 0;
int					g_NetworkOffline = 0;

CVLMain theMain;
CVLBasic theBasic;
CVLRecover theRecover;
CVLAttack theAttack;
CVLCollect theCollect;
CVLMove theMove;
CVLParty theParty;
CVLMap theMap;
CVLQuest theQuest;
CVLUtil theUtil;

void GetHardDriveComputerID(char* HardDriveID);
void OnDebugCommand(const DEBUG_PARAM *pParam);

BOOL APIENTRY DllMain(HANDLE hModule, DWORD uReason, LPVOID lpReserved)
{
	if (uReason == DLL_PROCESS_ATTACH)
	{
		g_hDll = (HINSTANCE)hModule;
		DisableThreadLibraryCalls(g_hDll);
    }
    return TRUE;
}

#define pCW ((CWPSTRUCT*)lParam)

LRESULT HookProc(int nCode, WPARAM wParam, LPARAM lParam)
{
	HWND hWndGame = pCW->hwnd;
	HHOOK hHook = (HHOOK)pCW->wParam;
	HWND hWndServer = (HWND)pCW->lParam;

	if (pCW->message != WM_HOOKEX)
		goto END;

	if (hWndServer)
	{
		_UnhookWindowsHookEx(hHook);

		if (bHooked)
			goto END;

		char lib_name[MAX_PATH];
		GetModuleFileName(g_hDll, lib_name, MAX_PATH);

		if (!LoadLibrary(lib_name))
			goto END;

		OldWndProc = (WNDPROC)_SetWindowLong(hWndGame, GWL_WNDPROC, (LONG)NewWndProc);

		if (OldWndProc == NULL)
			FreeLibrary(g_hDll);
		else
		{
			theMain.SetDllFilePath(lib_name);
			theMain.InitParams(hWndGame, hWndServer);
			CrashInstall("Game", TRUE);	// Log crash cua Game (ca loi trong VLHook) vao <Auto>\Crash
			nTimerID = _SetTimer(NULL, NULL, SYS_TIMER, (TIMERPROC)TimerProc);

			bHooked = TRUE;
		}
	}
	else
	{
		_UnhookWindowsHookEx(hHook);

		if (!bHooked)
			goto END;

		if (nTimerID)
			_KillTimer(NULL, nTimerID);

		if (!_SetWindowLong(hWndGame, GWL_WNDPROC, (LONG)OldWndProc))
			goto END;

		DebugPathUnhook();
		DebugAttackUnhook();
		DebugFileUnhook();
		DebugLowCfgUnhook();
		theMain.ClearEverything();
		CrashUninstall();

		FreeLibrary(g_hDll);
		bHooked = FALSE;
	}

END:
	return _CallNextHookEx(hHook, nCode, wParam, lParam);
}

// Xoa CRASH_STAGE khi ra khoi ham (crash sau do la cua game, khong phai cua buoc cu)
struct CCrashStageReset { ~CCrashStageReset() { CRASH_STAGE(NULL); } };

VOID CALLBACK TimerProc(HWND hWnd, UINT uMsg, UINT_PTR idEvent, DWORD dwTime)
{
	if (g_RunningTimer++ < 1) return;
	CCrashStageReset stageReset;
	CRASH_STAGE("TimerProc");

	if (bPausing)
	{
		if (nPausingCount)
			nPausingCount--;
		else
		{
			{ CRASH_STAGE("theMain.SetPause"); theMain.SetPause(FALSE); }
			bPausing = FALSE;
		}
	}

	KSubWorld *pSubWorld = (KSubWorld*)GetPtrAddress(WLD_BASE_ADD);
	if (!pSubWorld) return;

	if (P_PLAYER->m_dwID == 0 || P_PLAYER->m_SyncSignal == 0)
		g_NetworkOffline = -1;
	else
		g_NetworkOffline = (pSubWorld->m_dwCurrentTime - P_PLAYER->m_SyncSignal) / 18;

	// Gioi han Exp: dung Phuc hoi, Nhat do, Di chuyen, To doi, Ban do, Chien dau
	BOOL bExpLimit = theBasic.IsExpLimitRunning();
	if (!bExpLimit)
	{
		{ CRASH_STAGE("theRecover.Breathe"); theRecover.Breathe(); }
		if (!theMain.m_nTongMoney)	// Dang gom tien len bang: khong nhat do
			{ CRASH_STAGE("theCollect.Breathe"); CQuestMoveAllow qa; theCollect.Breathe(); }
	}

	if (g_RunningTimer % 2 == 0) // Chu ky 100ms
		{ CRASH_STAGE("theUtil.Breath"); theUtil.Breath(); }
	if (g_RunningTimer % 4 == 3) // Chu ky 200ms - MapPath + Giam cau hinh (DEBUG), chay ca khi tam dung
	{
		{ CRASH_STAGE("DebugPathBreathe"); CPauseMoveAllow pa; DebugPathBreathe(); }
		{ CRASH_STAGE("DebugLowCfgBreathe"); DebugLowCfgBreathe(); }
		{ CRASH_STAGE("DebugFileBreathe"); DebugFileBreathe(); }
		{ CRASH_STAGE("DebugQuestBreathe"); CQuestMoveAllow qa; CPauseMoveAllow pa; DebugQuestBreathe(); }
		{ CRASH_STAGE("theBasic.TrimRamBreathe"); theBasic.TrimRamBreathe(); }
		{ CRASH_STAGE("theMain.TongMoneyBreathe"); theMain.TongMoneyBreathe(); }
	}

	// Vua tam ngung: dung lenh di dang do (GoTo cua game tu tim duong tiep ke ca khi Auto khong goi nua)
	static BOOL s_bWasPause = FALSE;
	if (g_SysPause != s_bWasPause)
	{
		s_bWasPause = g_SysPause;
		if (g_SysPause && P_PLAYER->m_Doing != do_stand)
		{
			CRASH_STAGE("StopMove");
			CPauseMoveAllow pa;
			int x, y;
			P_PLAYER->GetMapPos(&x, &y);
			g_pCurPlayer->GoTo(x, y);
			P_PLAYER->MoveTo(x, y);
		}
	}

	if (g_SysPause) return;
	if (theMain.m_nTongMoney) return;	// Dang gom tien len bang (phu ve thanh -> gop quy): tam dung Auto

	if (bExpLimit)
	{
		if (g_RunningTimer % 12 == 5)	// Chu ky 600ms
			{ CRASH_STAGE("theBasic.ExpLimitBreathe"); theBasic.ExpLimitBreathe(); }
		if (g_RunningTimer % 20 == 12)	// Chu ky 1s
			{ CRASH_STAGE("theBasic.Breathe"); theBasic.Breathe(); }
		return;
	}

	if (g_RunningTimer % 2 == 1)		// Chu ky 100ms
		{ CRASH_STAGE("theAttack.Breathe"); CQuestMoveAllow qa; theAttack.Breathe(); }
	if (g_RunningTimer % 4 == 1)		// Chu ky 200ms - tu di theo MapPath
		{ CRASH_STAGE("PathTravelBreathe"); PathTravelBreathe(); }
	if (g_RunningTimer % 4 == 2)		// Chu ky 200ms
		{ CRASH_STAGE("theMove.Breathe"); theMove.Breathe(); }
	if (g_RunningTimer % 12 == 5)		// Chu ky 600ms
		{ CRASH_STAGE("theMap.Breathe"); theMap.Breathe(); }
	if (g_RunningTimer % 20 == 12)		// Chu ky 1s
		{ CRASH_STAGE("theBasic.Breathe"); theBasic.Breathe(); }
	if (g_RunningTimer % 30 == 17)		// Chu ky 1.5s
		{ CRASH_STAGE("theParty.Breathe"); theParty.Breathe(); }
	if (g_RunningTimer % 100 == 58)		// Chu ky 5s
		{ CRASH_STAGE("theQuest.CheckStatus"); theQuest.CheckStatus(); }
}

LRESULT CALLBACK NewWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	if (uMsg >= WM_HOOK_MAIN && uMsg <= WM_HOOK_DEBUG)
	{
		CCrashStageReset stageReset;
		CRASH_STAGE(uMsg == WM_HOOK_DEBUG ? "NewWndProc: lenh DEBUG (OnDebugCommand)" : "NewWndProc: nhan tham so tu VLAuto (WM_HOOK_*)");
		DWORD dwProcessID;
		_GetWindowThreadProcessId(theMain.m_hWndAuto, &dwProcessID);
		HANDLE hProcess_Auto = OpenProcess(PROCESS_VM_READ | PROCESS_QUERY_INFORMATION, FALSE, dwProcessID);

		switch (uMsg)
		{
		case WM_HOOK_MAIN:
			if (wParam == cmd_exit)
				theMain.ExitGame();
			else if (wParam == cmd_loadini)
				theMain.LoadOptions();
			else if (wParam == cmd_tongmoney)
				theMain.StoreTongMoney((int)lParam);
			break;
		case WM_HOOK_BASIC:
			ReadProcessMemory(hProcess_Auto, (LPCVOID)wParam, (LPVOID)&theBasic, lParam, NULL);
			break;
		case WM_HOOK_RECOVER:
			ReadProcessMemory(hProcess_Auto, (LPCVOID)wParam, (LPVOID)&theRecover, lParam, NULL);
			break;
		case WM_HOOK_ATTACK:
			ReadProcessMemory(hProcess_Auto, (LPCVOID)wParam, (LPVOID)&theAttack, lParam, NULL);
			theAttack.SetAutoHit();
			break;
		case WM_HOOK_COLLECT:
			ReadProcessMemory(hProcess_Auto, (LPCVOID)wParam, (LPVOID)&theCollect, lParam, NULL);
			theCollect.SetAutoArrange();
			break;
		case WM_HOOK_MOVE:
			ReadProcessMemory(hProcess_Auto, (LPCVOID)wParam, (LPVOID)&theMove, lParam, NULL);
			theMove.GetCurNode();
			break;
		case WM_HOOK_PARTY:
			ReadProcessMemory(hProcess_Auto, (LPCVOID)wParam, (LPVOID)&theParty, lParam, NULL);
			break;
		case WM_HOOK_MAP:
			ReadProcessMemory(hProcess_Auto, (LPCVOID)wParam, (LPVOID)&theMap, lParam, NULL);
			theMap.CheckAutoReturn();
			break;
		case WM_HOOK_YESOU:
			ReadProcessMemory(hProcess_Auto, (LPCVOID)wParam, (LPVOID)&theQuest, lParam, NULL);
			theQuest.CheckAutoYeSou();
			break;
		case WM_HOOK_UTIL:
			ReadProcessMemory(hProcess_Auto, (LPCVOID)wParam, (LPVOID)&theUtil, lParam, NULL);
			break;
		case WM_HOOK_EXDATA:
			theMain.InitGameData();
			theQuest.PrepareLog();
			g_RunningTimer = 1;
			break;
		case WM_HOOK_DEBUG:
			{
				DEBUG_PARAM dbgParam;
				memset(&dbgParam, 0, sizeof(dbgParam));
				ReadProcessMemory(hProcess_Auto, (LPCVOID)wParam, (LPVOID)&dbgParam, min((UINT)lParam, sizeof(dbgParam)), NULL);
				CloseHandle(hProcess_Auto);
				hProcess_Auto = NULL;
				OnDebugCommand(&dbgParam);
			}
			break;
		}

		if (hProcess_Auto) CloseHandle(hProcess_Auto);
		return 0;
	}

	// Other Messages
	RECT rcCenter = { 360, 180, 440, 320 };
	POINT ptPos = { LOWORD(lParam), HIWORD(lParam) };
	switch (uMsg)
	{
	case WM_KEYDOWN:
		if (DebugPathKey(wParam, lParam)) return 0;
		if (_GetKeyState(VK_CONTROL) & SHIFTED)
		{
			BOOL bCaught = TRUE;
			if (wParam == 'A')
				theMain.PostFeedBack(cmd_autohit);
			else if (wParam == 'B')
				theMain.PostFeedBack(cmd_autoarng);
			else if (wParam == 'T')
				theMain.PostFeedBack(cmd_setmap);
			else if (wParam == 'D')
				theMain.PostFeedBack(cmd_setyesou);
			else if (wParam == 'S')
				theMove.SavePoint();
			else if (wParam == 'Z')
				theUtil.OpenCloseStoreBox();
			else if (wParam == 'G')
				theUtil.OpenCloseNpcShop();
			else if (wParam == VK_SPACE)
				theUtil.ReadPosition();
			else if (wParam == VK_DELETE)
				theUtil.DeleteSysMessages();
			else if (wParam == 'X')
				theUtil.CheckLifeTime();
			else if (wParam == 'N')
				theUtil.AutoClickNpc();
			else if (wParam == 'W')
				theQuest.ReportQuest();
			else if (wParam == 'L')
				theMap.SaveNewRepository();
			else if (wParam == 'E')
				theUtil.StartGiveKCL();
			else if (wParam == '9')
				theUtil.StartOpenGift();
			else if (wParam == '0')
				theUtil.StopAllUtils();
			else if (wParam == '1')
				theUtil.TestFunction(1);
			else if (wParam == '2')
				theUtil.TestFunction(2);
			else if (wParam == '3')
				theRecover.OnUseAM();
			else
				bCaught = FALSE;
			if (bCaught) return 0;
		}
		break;
	case WM_ACTIVATE:
		if (LOWORD(wParam) != WA_INACTIVE)
			theMain.PostFeedBack(cmd_activate);
		break;
	case WM_LBUTTONDOWN:
		bPausing = 0;
		theMain.SetPause();
		break;
	case WM_LBUTTONUP:
		bPausing = TRUE;
		nPausingCount = MAX_PAUSE_TIME;
		break;
	case WM_LBUTTONDBLCLK:
		if ((wParam & MK_CONTROL) && _PtInRect(&rcCenter, ptPos))
		{
			SystemMessage(TV(L"<color=yellow>Khởi động lại VLAuto!!!"));
			theMain.RestartHook();

		}
		break;
	case WM_RBUTTONDBLCLK:
	{
		g_SysPause = !g_SysPause;
		if (g_SysPause)
		{
			SystemMessage(TV(L"<color=yellow>Auto: Tạm ngưng!!!<color>"), 1);
		}
		else
			SystemMessage(TV(L"<color=yellow>Auto: <color=green>Mở lại!!!"), 1);
		break;
	}
	}
	return _CallWindowProc(OldWndProc, hWnd, uMsg, wParam, lParam);
}

unsigned __stdcall StartInject(HWND hWndGame, HWND hWndAuto)
{
	if (hWndGame == 0)
	{
		char HardDriveID[33] = "h";
		GetHardDriveComputerID(HardDriveID + 1);
		return HashString2ID(HardDriveID);
	}

	if (!_IsWindow(hWndGame))
		return 0;

	HHOOK hHook = _SetWindowsHookEx(WH_CALLWNDPROC, (HOOKPROC)HookProc, g_hDll, _GetWindowThreadProcessId(hWndGame, NULL));
	if (hHook == NULL)
		return 0;

	_SendMessage(hWndGame, WM_HOOKEX, (WPARAM)hHook, (LPARAM)hWndAuto);

	return 1;
}

unsigned __stdcall StopInject(HWND hWndGame)
{	
	if (!_IsWindow(hWndGame))
		return 0;

	HHOOK hHook = _SetWindowsHookEx(WH_CALLWNDPROC, (HOOKPROC)HookProc, g_hDll, _GetWindowThreadProcessId(hWndGame, NULL));
	if (hHook == NULL)
		return 0;

	_SendMessage(hWndGame, WM_HOOKEX, (WPARAM)hHook, 0);

	return 1;
}
