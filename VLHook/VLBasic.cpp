#include "stdafx.h"
#include "../GameDef.h"
#include "GameBase.h"
#include "JxCore.h"
#include "S3Client.h"
#include "VLClasses.h"

//================================Class CVLBasic================================
CVLBasic::CVLBasic(void)
{
	m_bExitWhenAM = 0;
	m_bAutoStart = 0;
	m_nAutoStartTime = 3000;
	m_bExpLimit = 0;
	m_nExpMax = 20;
	m_nExpMin = 1;
	m_nExpLevel = 0;
	m_bTrimRam = 0;
	m_nTrimRamTime = 60;
	m_nGiamCauHinh = 0;
	m_dwLastTrimRam = 0;
	m_nExpStep = 0;
	memset(&Last_Save_Repository, 0, sizeof(Last_Save_Repository));
	m_nDistTimeCount = 0;
	m_nIdleTimeCount = 0;
	strcpy(m_szCurName, "");
}

void CVLBasic::Breathe()
{
	static int nCheckName = 0;
	KUiSysMsgCentre *pSysMsg = (KUiSysMsgCentre*)(GetPtrAddress(SMC_BASE_ADD));
	if (!pSysMsg)	// not in Game
	{
		if (nCheckName == 1)
			nCheckName = 2;
		if (theMain.m_bQuitingGame)
		{
			KUiInit *pUiUnit = (KUiInit*)(GetPtrAddress(UIN_BASE_ADD));
			if (pUiUnit && pUiUnit->IsVisible())
				pUiUnit->QuitGame();
		}
		return;
	}

	if (g_NetworkOffline == -1)	// disconnected
	{
		nCheckName = 2;
		m_nDistTimeCount++;
		return;
	}
	m_nDistTimeCount = 0;

	// Update TongList
	if (!g_GlobalTongInfo)
		theMain.UpdateTongList();

	if (nCheckName == 0)
	{
		nCheckName = 1;
		strcpy(m_szCurName, P_PLAYER->Name);
	}
	else if (nCheckName == 2)
	{
		nCheckName = 0;
		if (strcmp(m_szCurName, P_PLAYER->Name))
		{
			SystemMessage(TV(L"Nhân vật đã thay đổi, khởi động lại..."));
			theMain.RestartHook();
		}
		return;
	}

	if (g_NetworkOffline > 180)	// no response over 3 minutes
	{
		SystemMessage(TV(L"Ngắt mạng quá 3 phút, kết nối lại..."));
		ExecuteScript("SayPhrase(1)");
		return;
	}

	static int nReviveTime = 2;
	if (P_PLAYER->m_Doing == do_revive || P_PLAYER->m_Doing == do_death) // online, but role died
	{
		if (nReviveTime && P_PLAYER->m_FightMode) { nReviveTime--; return; } // wait for AM
		if (g_UiInformation->IsVisible())
		{
			if (strstr(g_UiInformation->m_szInfoText, TV(L"Bạn đã bị trọng thương")))
				g_UiInformation->Hide(0);
			return;
		}
		if (P_PLAYER->m_bRideHorse)
		{
			ExecuteScript(SCK_SHORTCUT_HORSE); // Xuong ngua
			return;
		}
		ExecuteScript(SCK_SHORTCUT_SIT);
	}
	nReviveTime = 2;

	if (ExpLimitCheck())	// Gioi han Exp: dung cho quai danh, khong tinh dung yen
	{
		m_nIdleTimeCount = 0;
		return;
	}
	if (theUtil.m_bDoingUtil) return;
	if (theQuest.m_bGoTrainGetPK) return;

	if (g_pCurPlayer->m_nEntrustMode)	// online entrustment
	{
		m_nIdleTimeCount = 0;
		return;
	}
	if (P_PLAYER->m_Doing != do_run && P_PLAYER->m_Doing != do_walk && P_PLAYER->m_Doing != do_attack) // online, but no move
		m_nIdleTimeCount++;
	else
		m_nIdleTimeCount = 0;
	if (m_nIdleTimeCount > 180)
	{
		if (g_UiInformation->IsVisible())
		{
			if (strstr(g_UiInformation->m_szInfoText, TV(L"Bạn đã bị trọng thương")))
				g_UiInformation->Hide(0);
			else
				g_UiInformation->Hide(1);
		}
		if (g_UiInformation2->IsVisible())
			g_UiInformation2->Hide();
		KUiMsgSel *pMsgSel = (KUiMsgSel*)GetPtrAddress(MSG_BASE_ADD);
		if (pMsgSel && pMsgSel->IsVisible())
			pMsgSel->OnClickMsg(pMsgSel->m_nNumMessage);
		KUiMsgSel2 *pMsgSel2 = (KUiMsgSel2*)GetPtrAddress(MSG2_BASE_ADD);
		if (pMsgSel2 && pMsgSel2->IsVisible())
			pMsgSel2->OnClickMsg(pMsgSel2->m_nNumMessage);

		m_nIdleTimeCount = 0;
		const char *szMsg = TV(L"Ngừng hoạt động quá 3 phút, khởi động lại...");
		theQuest.SaveYSLog(szMsg);
		SystemMessage(szMsg);
		CloseAllWindows(0);
		theMain.RestartHook();
	}
}

// Giam RAM: day trang nho cua game ra standby/pagefile (Task Manager giam manh, game nap lai dan trang dang dung)
void CVLBasic::TrimRamBreathe()
{
	if (!m_bTrimRam || m_nTrimRamTime <= 0)
	{
		m_dwLastTrimRam = 0;
		return;
	}
	DWORD dwNow = GetTickCount();
	if (m_dwLastTrimRam && dwNow - m_dwLastTrimRam < (DWORD)m_nTrimRamTime * 1000)
		return;
	SetProcessWorkingSetSize(GetCurrentProcess(), (SIZE_T)-1, (SIZE_T)-1);
	m_dwLastTrimRam = dwNow ? dwNow : 1;
}

// ===== Giam cau hinh (tab Co ban: IDC_GIAMCAUHINH_COMBO) =====
// Sua ma game 15-10 (KeoXe F10):
//   muc 0: ma goc, Sleep goc (1 ms)
//   muc 1: KScenePlaceC::Paint chi ve region dau trong 9 region quanh nhan vat, Sleep 40 ms
//   muc 2: bo han phan ve canh (den man hinh, giao dien van ve), Sleep 60 ms
// Chi ghi khi byte hien tai dung la ma mong doi (khong de len cho da bi sua khac).
static int	s_nGchApplied = 0;		// Muc dang ap dung
#define GCH_ORG_SLEEP	1			// Sleep goc cua game 15-10 (push 1)

static void GchPatch(DWORD dwAddr, const BYTE* pData, SIZE_T nSize)
{
	DWORD dwOld;
	if (!VirtualProtect((LPVOID)dwAddr, nSize, PAGE_EXECUTE_READWRITE, &dwOld)) return;
	memcpy((void*)dwAddr, pData, nSize);
	VirtualProtect((LPVOID)dwAddr, nSize, dwOld, &dwOld);
	FlushInstructionCache(GetCurrentProcess(), (LPCVOID)dwAddr, nSize);
}

static void GchSwap2(DWORD dwAddr, BYTE f0, BYTE f1, BYTE t0, BYTE t1)
{
	const BYTE* p = (const BYTE*)dwAddr;
	if (p[0] == f0 && p[1] == f1)
	{
		const BYTE b[2] = { t0, t1 };
		GchPatch(dwAddr, b, 2);
	}
}

static void GchSetSleep(DWORD dwAddr, BYTE nMs)
{
	const BYTE* p = (const BYTE*)dwAddr;
	if (p[0] != 0x6A) return;	// push imm8
	if (p[1] != nMs) GchPatch(dwAddr + 1, &nMs, 1);
}

static void GchApply(int nMode)
{
	BYTE nSleep;
	switch (nMode)
	{
	case 1:
		GchSwap2(CPU_GIAM_MUC_2, 0x90, 0xE9, 0x0F, 0x84);	// tra lai je
		GchSwap2(CPU_GIAM_MUC_1, 0x75, 0xED, 0x90, 0x90);	// jne -> nop nop
		nSleep = 40;
		break;
	case 2:
		GchSwap2(CPU_GIAM_MUC_1, 0x90, 0x90, 0x75, 0xED);	// tra lai jne
		GchSwap2(CPU_GIAM_MUC_2, 0x0F, 0x84, 0x90, 0xE9);	// je -> nop; jmp
		nSleep = 60;
		break;
	default:
		GchSwap2(CPU_GIAM_MUC_1, 0x90, 0x90, 0x75, 0xED);
		GchSwap2(CPU_GIAM_MUC_2, 0x90, 0xE9, 0x0F, 0x84);
		nSleep = GCH_ORG_SLEEP;
		nMode = 0;
		break;
	}
	GchSetSleep(CPU_SLEEP_1, nSleep);
	GchSetSleep(CPU_SLEEP_2, nSleep);
	GchSetSleep(CPU_SLEEP_3, nSleep);
	s_nGchApplied = nMode;
}

void CVLBasic::GiamCauHinhBreathe()
{
	int nMode = (m_nGiamCauHinh >= 1 && m_nGiamCauHinh <= 2) ? m_nGiamCauHinh : 0;
	if (nMode != s_nGchApplied) GchApply(nMode);
}

void CVLBasic::GiamCauHinhRestore()
{
	if (s_nGchApplied) GchApply(0);
}
