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

void CuuSatLogParams();	// VLAttack.cpp - debug log Cuu sat / Do sat (DebugCuuSat)

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
#define WM_HOOK_READPOS		WM_USER + 112

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
		theBasic.GiamCauHinhRestore();
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
	EnterMapUpdate();

	if (P_PLAYER->m_dwID == 0 || P_PLAYER->m_SyncSignal == 0)
		g_NetworkOffline = -1;
	else
		g_NetworkOffline = (pSubWorld->m_dwCurrentTime - P_PLAYER->m_SyncSignal) / 18;

	// Gioi han Exp: dung Phuc hoi, Nhat do, Di chuyen, To doi, Ban do, Chien dau
	BOOL bExpLimit = theBasic.IsExpLimitRunning();
	if (!bExpLimit)
	{
		{ CRASH_STAGE("theRecover.Breathe"); theRecover.Breathe(); }
		if (!theMain.m_nTongMoney && !theMain.m_nWcbStep)	// Dang gom tien len bang / chay bo ve thanh luu ruong: khong nhat do (nhat do keo nhan vat lai, dung im)
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
		{ CRASH_STAGE("theBasic.GiamCauHinhBreathe"); theBasic.GiamCauHinhBreathe(); }
		{ CRASH_STAGE("theMain.TongMoneyBreathe"); theMain.TongMoneyBreathe(); }
		{ CRASH_STAGE("theMain.WalkCityBoxBreathe"); CPauseMoveAllow pa; theMain.WalkCityBoxBreathe(); }
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
	if (theMain.m_nWcbStep) return;		// Dang chay bo ve thanh luu ruong (WalkCityBoxBreathe): tam dung Auto

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

// Map train chi xac dinh khi tick 1 trong 2 option (loai tru nhau):
//  - "Di chuyen theo toa do" (m_bRouteMove): map = map cua danh sach toa do train
//  - "Danh quanh diem" (m_bAroundPoint): map = map vua nhan khi ghi diem
// Khong tick ca hai => khong co map train => khong tim duong / di chuyen lien map.
static void SyncTrainMap()
{
	int nMap = 0;
	if (theMove.m_bRouteMove && theMove.m_nNodesMapID && theMove.m_Nodes[0].x && theMove.m_Nodes[0].y)
		nMap = theMove.m_nNodesMapID;
	else if (theMove.m_bAroundPoint && theMove.m_nPointMapID && theMove.m_nPointX && theMove.m_nPointY)
		nMap = theMove.m_nPointMapID;
	theMap.m_nTrainMapID = nMap;
}

//================= READPOS: doc toa do TRONG game, tra ve UI qua WM_COPYDATA =================
// Phim nong trong game (Ctrl+Insert / Ctrl+Del / Space) chi hoat dong khi dang o che do doc.
// hWndReply + nKind duoc GIU qua moi lan bam phim cho den khi UI gui cmd_stopreadpos
// (hoac dong cua so Danh sach toa do). Thong bao hien tren dau nhan vat (m_szTopText),
// khoi phuc ten bang goc khi ket thuc.
static struct READPOS_STATE
{
	BOOL	bActive;			// Dang trong che do doc toa do
	int		nKind;				// List dang doc (rpk_node / rpk_gonode)
	HWND	hWndReply;			// Cua so UI nhan WM_COPYDATA (GIU qua moi lan bam phim)
	BOOL	bNameSaved;			// Da luu chu tren dau goc chua
	char	szTopText[64];		// Chu tren dau goc (khoi phuc khi stop)
} g_ReadPos;

// Doc vi tri HIEN TAI cua nhan vat -> toa do map (hien thi) + tuyet doi (luu config)
static void GetCurReadPos(int *pnMapID, int *pnMapX, int *pnMapY, int *pnAbsX, int *pnAbsY)
{
	int nAbsX = 0, nAbsY = 0;
	P_PLAYER->GetMapPos(&nAbsX, &nAbsY);
	*pnMapID = *g_pMapID;
	*pnAbsX = nAbsX;
	*pnAbsY = nAbsY;
	*pnMapX = nAbsX >> 8;
	*pnMapY = nAbsY >> 9;
}

// Tam thay chu tren dau nhan vat bang thong bao (doc toa do / Auto tam ngung...).
// Dung chung 1 slot backup: nhan vat chi co 1 dong chu tren dau, luu lai ten goc de khoi phuc.
static BOOL g_bTopSaved = FALSE;
static char g_szTopSaved[64];

static void SetOverheadText(const char *szText)
{
	if (!g_bTopSaved)
	{
		memcpy(g_szTopSaved, P_PLAYER->m_szTopText, sizeof(g_szTopSaved));
		g_bTopSaved = TRUE;
	}
	lstrcpynA(P_PLAYER->m_szTopText, szText, sizeof(P_PLAYER->m_szTopText));
}

static void RestoreOverheadText()
{
	if (g_bTopSaved)
	{
		memcpy(P_PLAYER->m_szTopText, g_szTopSaved, sizeof(g_szTopSaved));
		g_bTopSaved = FALSE;
	}
}

static void SendReadPosReply(HWND hWndReply, int nKind, int nMapID, int nMapX, int nMapY, int nAbsX, int nAbsY)
{
	if (!hWndReply || !_IsWindow(hWndReply)) return;
	READPOS_REPLY reply;
	memset(&reply, 0, sizeof(reply));
	reply.cbSize = sizeof(reply);
	reply.nKind = nKind;
	reply.nMapID = nMapID;
	reply.nMapX = nMapX;	reply.nMapY = nMapY;
	reply.nAbsX = nAbsX;	reply.nAbsY = nAbsY;
	COPYDATASTRUCT cds;
	cds.dwData = READPOS_COPYDATA_ID;
	cds.cbData = sizeof(reply);
	cds.lpData = &reply;
	_SendMessage(hWndReply, WM_COPYDATA, (WPARAM)theMain.m_hWndGame, (LPARAM)&cds);
}

// Toa do vua doc da co trong list chua (cung ban do) -> tranh luu trung.
// theMove duoc UI dong bo qua WM_HOOK_MOVE moi lan SetGameParams(tab_move).
// Khac ban do thi UI se xoa list cu roi tao moi, nen khong tinh la trung.
static BOOL IsDupReadPos(int nKind, int nMapID, int nAbsX, int nAbsY)
{
	const POINT *pNodes  = (nKind == rpk_gonode) ? theMove.m_GoNodes : theMove.m_Nodes;
	int          nListID = (nKind == rpk_gonode) ? theMove.m_nGoNodesMapID : theMove.m_nNodesMapID;
	if (nListID && nMapID && nListID != nMapID) return FALSE;	// khac ban do: list cu se bi xoa
	for (int i = 0; i < MAX_NODES; i++)
	{
		if (!pNodes[i].x && !pNodes[i].y) break;	// het node (slot trong)
		if (pNodes[i].x == nAbsX && pNodes[i].y == nAbsY) return TRUE;
	}
	return FALSE;
}

// Doc vi tri hien tai vao list dang doc + hien thong bao tren dau nhan vat
static void DoReadPosInto(int nKind)
{
	int nMapID, nMapX, nMapY, nAbsX, nAbsY;
	GetCurReadPos(&nMapID, &nMapX, &nMapY, &nAbsX, &nAbsY);

	if (IsDupReadPos(nKind, nMapID, nAbsX, nAbsY))
	{
		char szTop[128], szSys[160];
		sprintf(szTop, TV(L"Tọa độ (%d,%d) đã có, không lưu trùng. Bấm SPACE đọc tiếp"), nMapX, nMapY);
		sprintf(szSys, TV(L"<color=red>Tọa độ (%d,%d) đã có, không lưu trùng.<color> Bấm SPACE đọc tiếp"), nMapX, nMapY);
		SetOverheadText(szTop);
		SystemMessage(szSys);
		return;		// khong gui reply -> UI khong luu
	}

	SendReadPosReply(g_ReadPos.hWndReply, nKind, nMapID, nMapX, nMapY, nAbsX, nAbsY);

	char szMsg[128];
	if (nKind == rpk_gonode)
		sprintf(szMsg, TV(L"Tọa độ nơi LC :%d,%d , bấm SPACE đọc tiếp"), nMapX, nMapY);
	else
		sprintf(szMsg, TV(L"Tọa độ LC :%d,%d , bấm SPACE đọc tiếp"), nMapX, nMapY);
	SetOverheadText(szMsg);
	SystemMessage(szMsg);
}

// Bat dau (hoac chuyen) che do doc toa do cho list nKind, doc ngay vi tri hien tai
static void StartReadPos(HWND hWndReply, int nKind)
{
	g_ReadPos.bActive = TRUE;
	g_ReadPos.hWndReply = (hWndReply && _IsWindow(hWndReply)) ? hWndReply : theMain.m_hWndAuto;
	g_ReadPos.nKind = nKind;
	DoReadPosInto(nKind);
}

static void StopReadPos()
{
	if (!g_ReadPos.bActive) return;
	g_ReadPos.bActive = FALSE;
	g_ReadPos.hWndReply = NULL;
	RestoreOverheadText();
	SystemMessage(TV(L"<color=yellow>Kết thúc đọc tọa độ."));
}

// Lenh doc tiep (bare SPACE) vao list dang active
static void ReadPosNext()
{
	if (!g_ReadPos.bActive) return;
	DoReadPosInto(g_ReadPos.nKind);
}

// Phim nong Ctrl+Insert (list 0) / Ctrl+Del (list 1): bat/chuyen list roi doc ngay
static void HotkeyReadList(int nKind)
{
	StartReadPos(g_ReadPos.hWndReply, nKind);
}

// Lenh mot lan: doc vi tri luu ruong, tra ve UI (rpk_box), khong vao che do doc
static void DoSaveBox(HWND hWndReply)
{
	int nMapID, nMapX, nMapY, nAbsX, nAbsY;
	GetCurReadPos(&nMapID, &nMapX, &nMapY, &nAbsX, &nAbsY);
	SendReadPosReply(hWndReply, rpk_box, nMapID, nMapX, nMapY, nAbsX, nAbsY);
	char szMsg[128];
	sprintf(szMsg, TV(L"Đã lưu vị trí rương: <color=green>(%d,%d)<color> map %d"), nMapX, nMapY, nMapID);
	SystemMessage(szMsg);
}

LRESULT CALLBACK NewWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	if (uMsg >= WM_HOOK_MAIN && uMsg <= WM_HOOK_READPOS)
	{
		CCrashStageReset stageReset;
		CRASH_STAGE(uMsg == WM_HOOK_DEBUG ? "NewWndProc: lenh DEBUG (OnDebugCommand)" :
		            (uMsg == WM_HOOK_READPOS ? "NewWndProc: lenh READPOS (doc toa do)" : "NewWndProc: nhan tham so tu VLAuto (WM_HOOK_*)"));
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
			else if (wParam == cmd_walkcitybox)
				theMain.StartWalkCityBox((int)lParam);
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
			CuuSatLogParams();	// VLAttack.cpp - chi ghi khi DebugCuuSat 1
			break;
		case WM_HOOK_COLLECT:
			ReadProcessMemory(hProcess_Auto, (LPCVOID)wParam, (LPVOID)&theCollect, lParam, NULL);
			theCollect.SetAutoArrange();
			break;
		case WM_HOOK_MOVE:
			ReadProcessMemory(hProcess_Auto, (LPCVOID)wParam, (LPVOID)&theMove, lParam, NULL);
			theMove.GetCurNode();
			SyncTrainMap();
			break;
		case WM_HOOK_PARTY:
			ReadProcessMemory(hProcess_Auto, (LPCVOID)wParam, (LPVOID)&theParty, lParam, NULL);
			break;
		case WM_HOOK_MAP:
			ReadProcessMemory(hProcess_Auto, (LPCVOID)wParam, (LPVOID)&theMap, lParam, NULL);
			SyncTrainMap();
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
#if !ADMIN_MODE
			{
				// Ban User: keo bang 5 dia chi loi (da giai seal) tu bo nho VLAuto.
				// wParam = (&theGameData cua VLAuto) ^ CLIENT_SERIAL.
				ReadProcessMemory(hProcess_Auto, (LPCVOID)(wParam ^ CLIENT_SERIAL),
				                  (LPVOID)&theGameData, lParam, NULL);
				// Thu tu nap + fail-safe: chi khoi dong auto khi 5 dia chi (DA UNMASK)
				// nam trong vung module game. Chua kich hoat => theGameData ~ 0 =>
				// im lang khong InitGameData, khong dat g_RunningTimer (khong crash, khong bao loi).
				const DWORD mask = (DWORD)theGameData.ExeFileSize ^ CLIENT_SERIAL;
				const DWORD lo = 0x00400000, hi = 0x02000000;
				DWORD c[5] = { (DWORD)theGameData.PlrBaseAdd ^ mask, (DWORD)theGameData.NpcBaseAdd ^ mask,
				               (DWORD)theGameData.KCoreShell ^ mask, (DWORD)theGameData.MapAddress ^ mask,
				               (DWORD)theGameData.AccountID ^ mask };
				bool ok = true;
				for (int i = 0; i < 5; i++) if (c[i] < lo || c[i] >= hi) ok = false;
				if (!ok) break;   // bang chua san sang -> degrade im lang (handle dong o cuoi)
			}
#endif
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
		case WM_HOOK_READPOS:
			{
				READPOS_PARAM rpParam;
				memset(&rpParam, 0, sizeof(rpParam));
				ReadProcessMemory(hProcess_Auto, (LPCVOID)wParam, (LPVOID)&rpParam, min((UINT)lParam, sizeof(rpParam)), NULL);
				CloseHandle(hProcess_Auto);
				hProcess_Auto = NULL;
				if (rpParam.cbSize == sizeof(READPOS_PARAM))
				{
					if (rpParam.nCmd == cmd_startreadpos)
						StartReadPos(rpParam.hWndReply, rpParam.nKind);
					else if (rpParam.nCmd == cmd_stopreadpos)
						StopReadPos();
					else if (rpParam.nCmd == cmd_savebox)
						DoSaveBox(rpParam.hWndReply);
				}
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
#if ADMIN_MODE
		if (DebugPathKey(wParam, lParam)) return 0;	// Phim sua MapPath (cua so DEBUG)
#endif
		// Dang doc toa do: Space don = doc tiep vao list dang active (chi khi khong giu Ctrl)
		if (g_ReadPos.bActive && wParam == VK_SPACE && !(_GetKeyState(VK_CONTROL) & SHIFTED))
		{
			ReadPosNext();
			return 0;
		}
		if (_GetKeyState(VK_CONTROL) & SHIFTED)
		{
			BOOL bCaught = TRUE;
			if (wParam == 'A')
				theMain.PostFeedBack(cmd_autohit);
			else if (wParam == 'B')
				theMain.PostFeedBack(cmd_autoarng);
			else if (wParam == 'Y')
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
			else if (wParam == VK_INSERT)
				HotkeyReadList(rpk_node);			// Ctrl+Insert: doc toa do vao list 0 (luyen cong)
			else if (wParam == VK_DELETE)
			{
				if (_GetKeyState(VK_SHIFT) & SHIFTED)
					theUtil.DeleteSysMessages();	// Ctrl+Shift+Del: xoa thong bao he thong
				else
					HotkeyReadList(rpk_gonode);		// Ctrl+Del: bat/doc toa do vao list 1 (noi luyen cong)
			}
			else if (wParam == VK_END)
				StopReadPos();						// Ctrl+End: ket thuc che do doc toa do (khoi phuc ten bang)
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
#if ADMIN_MODE
			else if (wParam == '1')
				theUtil.TestFunction(1);
			else if (wParam == '2')
				theUtil.TestFunction(2);
#endif
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
			SetOverheadText(TV(L"Auto đang tạm ngưng, click chuột phải 2 lần để tiếp tục"));
		}
		else
		{
			SystemMessage(TV(L"<color=yellow>Auto: <color=green>Mở lại!!!"), 1);
			RestoreOverheadText();
		}
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
