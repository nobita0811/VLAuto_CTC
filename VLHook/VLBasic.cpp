#include "stdafx.h"
#include "../GameDef.h"
#include "GameBase.h"
#include "JxCore.h"
#include "S3Client.h"
#include "VLClasses.h"
#include <math.h>

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
	m_bFixChanMap = 0;
	m_bFixChanMap1 = 1;
	m_bFixChanMap2 = 1;
	m_nChanMapId = 0;
	m_nChanMapX = 0;
	m_nChanMapY = 0;
	m_nChanDirX = 0;
	m_nChanDirY = 0;
	m_nChanJumpDist = 200;
	m_nChanJumpCount = 3;
	m_nChanCheckTime = 1000;
	m_nChanCheckDist = 150;
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

//================================Chong chan map================================
// Bang co dinh theo ID map: moi dong {mapId, congX, congY, nhayToiX, nhayToiY, diffX, diffY}.
// diffX/diffY cong vao vi tri hien tai de ra diem khinh cong (giu nguyen tu ban auto cu).
struct ChanMapData { int m_nMapId; int m_nPosX; int m_nPosY; int m_nJumpToX; int m_nJumpToY; int m_nDiffX; int m_nDiffY; };
static ChanMapData g_ChanMap[] = {
	{198, 48704,  94560,  48713,  94282,  200, -200},	// Thanh Khe
	{203, 49632,  95744,  49520,  95980, -200,  200},	// Vo Danh
	{199, 52608,  92992,  52420,  93136, -200,  200},	// Vu Lang
	{204, 53728, 108512,  53502, 108650, -200,  200},	// Phi Thien
	{200, 51584, 101472,  51765, 101624,  200,  200},	// Co Duong
	{201, 51168, 102304,  51403, 102214,  200, -200},	// Bang Ha
	{205, 54336, 109056,  54158, 108852, -200, -200},	// Duong Trung
};

#define CHANMAP_BREATHE_MS	400		// AntiBlockBreathe goi moi 400ms (VLHook.cpp)
#define CHANMAP_JUMP_SKILLID	210		// Skill khinh cong (giong LAG_JUMP_SKILLID o VLMove.cpp)
static int ChanMsToTicks(int nMs) { int t = nMs / CHANMAP_BREATHE_MS; return t < 1 ? 1 : t; }

// Khinh cong (skill 210) toi (jx, jy) de thoat diem bi chan
static void DoAntiBlockJump(int jx, int jy)
{
	if (P_PLAYER->m_bRideHorse) ExecuteScript(SCK_SHORTCUT_HORSE);	// Xuong ngua moi khinh cong duoc
	P_PLAYER->SendCommand(do_skill, CHANMAP_JUMP_SKILLID, jx, jy);
	theAttack.SendVLCmdSkill(CHANMAP_JUMP_SKILLID, jx, jy);
	SystemMessage(1, "<color=red>Chong chan map: khinh cong toi %d , %d<color>", jx, jy);
}

void CVLBasic::AntiBlockBreathe()
{
	if (!m_bFixChanMap) return;
	KUiSysMsgCentre *pSysMsg = (KUiSysMsgCentre*)(GetPtrAddress(SMC_BASE_ADD));
	if (!pSysMsg) return;				// chua vao game
	if (g_NetworkOffline == -1) return;	// mat ket noi
	if (P_PLAYER->m_Doing == do_death || P_PLAYER->m_Doing == do_revive) return;

	int nPlayerX = 0, nPlayerY = 0;
	P_PLAYER->GetMapPos(&nPlayerX, &nPlayerY);

	// (1) Bang co dinh theo ID map
	static int s_nCountFixed = 0;
	static int s_nLastMap = 0;
	for (int i = 0; i < (int)_countof(g_ChanMap); i++)
	{
		if (g_ChanMap[i].m_nMapId != *g_pMapID) continue;
		int jx = nPlayerX + g_ChanMap[i].m_nDiffX;
		int jy = nPlayerY + g_ChanMap[i].m_nDiffY;
		if (s_nLastMap != *g_pMapID)
		{
			if (m_bFixChanMap2) DoAntiBlockJump(jx, jy);	// nhay ngay khi doi map
			s_nCountFixed = 0;
		}
		else if (m_bFixChanMap1)	// dem tick khi ket gan cong roi nhay
		{
			int d = __distance(g_ChanMap[i].m_nPosX, g_ChanMap[i].m_nPosY, nPlayerX, nPlayerY);
			if (d < 150 && ++s_nCountFixed > 2) { DoAntiBlockJump(jx, jy); s_nCountFixed = 0; }
		}
		break;
	}
	s_nLastMap = *g_pMapID;

	// (2) Toa do tuy chon
	static int s_nInZone = 0;
	static int s_nJumpDone = 0;
	if (m_nChanMapId && *g_pMapID == m_nChanMapId && m_nChanMapX && m_nChanMapY)
	{
		int nCheckDist = m_nChanCheckDist > 0 ? m_nChanCheckDist : 150;
		int nCheckTick = ChanMsToTicks(m_nChanCheckTime > 0 ? m_nChanCheckTime : 1000);
		int nMaxJumps  = m_nChanJumpCount > 0 ? m_nChanJumpCount : 1;
		int nJumpDist  = m_nChanJumpDist > 0 ? m_nChanJumpDist : 200;
		int d = __distance(m_nChanMapX, m_nChanMapY, nPlayerX, nPlayerY);
		if (d < nCheckDist)
		{
			s_nInZone++;
			if (s_nJumpDone < nMaxJumps && s_nInZone >= nCheckTick)
			{
				double dx = (double)(m_nChanDirX - m_nChanMapX);
				double dy = (double)(m_nChanDirY - m_nChanMapY);
				double len = sqrt(dx * dx + dy * dy);
				if (len >= 1.0)
				{
					int jx = nPlayerX + (int)(dx / len * nJumpDist);
					int jy = nPlayerY + (int)(dy / len * nJumpDist);
					DoAntiBlockJump(jx, jy);
					s_nJumpDone++;
					s_nInZone = 0;
				}
			}
		}
		else { s_nInZone = 0; s_nJumpDone = 0; }
	}
}
