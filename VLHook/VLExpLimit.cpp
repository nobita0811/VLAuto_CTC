#include "stdafx.h"
#include "../GameDef.h"
#include "GameBase.h"
#include "JxCore.h"
#include "S3Client.h"
#include "VLClasses.h"
#include "VLDebug.h"

// Gioi han Exp (tab Co ban): cap >= m_nExpLevel va % exp >= m_nExpMax -> ve thanh, luu ruong Duong Chau, dung cho quai danh chet
// den khi % exp <= m_nExpMin, roi luu lai ruong cu (Last_Save_Repository) va chay lai cac chuc nang khac.
// Trong luc chay: TimerProc bo qua Phuc hoi, Nhat do, Di chuyen, To doi, Ban do, Chien dau (IsExpLimitRunning).

#define DC_REPO_X		52013	// Ruong Duong Chau
#define DC_REPO_Y		101660
#define DC_DIE_X		44345	// Cho dung cho quai danh
#define DC_DIE_Y		109892
#define MIN_MONEY		1000	// Tien xa phu

static int s_nLastMapID = 0;
static BOOL s_bDirect = FALSE;		// Het duong trong thanh, chay thang toi dich
static DWORD s_dwActionTick = 0;	// Lan dung TDP / bao loi cuoi
static char s_szLoadedName[32] = "";
static int s_nMoneyTry = 0;			// So lan gui lenh cat / rut tien o buoc hien tai
static BOOL s_bNoDeposit = FALSE;	// Cat tien khong duoc: khong quay lai cat nua
static BOOL s_bWithdrawn = FALSE;	// Buoc restore: da rut tien xa phu o Duong Chau

static WCHAR s_wszCfgFile[MAX_PATH] = L"";	// UserConfig\<ten nhan vat>.ini

static const char* s_szStep[] = { "", "Ve thanh", "Luu ruong cu", "Toi Duong Chau", "Luu ruong Duong Chau",
	"Cho quai danh", "Luu lai ruong cu" };

int CVLBasic::GetExpPercent()
{
	__int64 nNext = g_pCurPlayer->m_nNextExp;
	if (nNext <= 0) return 0;
	return (int)(g_pCurPlayer->m_nCurExp * 100 / nNext);
}

void CVLBasic::SetExpStep(int nStep)
{
	m_nExpStep = nStep;
	s_bDirect = FALSE;
	PathWalkReset();
	s_dwActionTick = 0;
	s_nMoneyTry = 0;
	s_bWithdrawn = FALSE;
	if (nStep == exp_none || nStep == exp_return) s_bNoDeposit = FALSE;
	theMap.m_nCurPoint = -1;
	theMap.m_bDoShoping = 0;
	theMap.m_nTimeWait = 0;

	// Luu trang thai vao config rieng cua nhan vat: khoi dong lai Hook giua chung van chay tiep
	if (s_wszCfgFile[0])
	{
		WCHAR szVal[64];
		swprintf_s(szVal, L"%d", nStep);
		WritePrivateProfileStringW(L"ExpLimit", L"Step", szVal, s_wszCfgFile);
		swprintf_s(szVal, L"%d,%d,%d", Last_Save_Repository.nMapID, Last_Save_Repository.x, Last_Save_Repository.y);
		WritePrivateProfileStringW(L"ExpLimit", L"Repository", szVal, s_wszCfgFile);
	}

	if (nStep > 0)
		SystemMessage(0, "<color=yellow>Gioi han Exp (%d%%): <color=green>%s", GetExpPercent(), s_szStep[nStep]);
}

void CVLBasic::LoadExpState()
{
	if (!strcmp(s_szLoadedName, P_PLAYER->Name)) return;
	strcpy(s_szLoadedName, P_PLAYER->Name);

	// UserConfig\<ten nhan vat>.ini - ten file giong UserConfig\<ten>.cfg cua VLAuto (TCVN3 doc theo cp1252)
	WCHAR szFolder[MAX_PATH], szName[33] = L"";
	MultiByteToWideChar(CP_ACP, 0, theMain.m_szCurFolder, -1, szFolder, MAX_PATH);
	MultiByteToWideChar(1252, 0, P_PLAYER->Name, -1, szName, 32);
	szName[32] = 0;
	for (int i = 0; szName[i]; i++)
		if (szName[i] < 0x20 || wcschr(L"\\/:*?\"<>|", szName[i]))
			szName[i] = L'_';
	swprintf_s(s_wszCfgFile, L"%s\\UserConfig", szFolder);
	CreateDirectoryW(s_wszCfgFile, NULL);
	swprintf_s(s_wszCfgFile, L"%s\\UserConfig\\%s.ini", szFolder, szName);

	WCHAR szVal[64];
	m_nExpStep = GetPrivateProfileIntW(L"ExpLimit", L"Step", 0, s_wszCfgFile);
	GetPrivateProfileStringW(L"ExpLimit", L"Repository", L"0,0,0", szVal, 64, s_wszCfgFile);
	memset(&Last_Save_Repository, 0, sizeof(Last_Save_Repository));
	swscanf(szVal, L"%d,%d,%d", &Last_Save_Repository.nMapID, &Last_Save_Repository.x, &Last_Save_Repository.y);

	// Ban cu luu trong VLAutoPr.ini [ExpLimit] <ten>_Step / <ten>_Repository: chuyen sang file nhan vat roi xoa
	char szKey[64], szOld[64];
	sprintf(szKey, "%s_Repository", P_PLAYER->Name);
	if (GetPrivateProfileString("ExpLimit", szKey, "", szOld, 64, theMain.m_szIniFile))
	{
		sprintf(szKey, "%s_Step", P_PLAYER->Name);
		m_nExpStep = GetPrivateProfileInt("ExpLimit", szKey, 0, theMain.m_szIniFile);
		WritePrivateProfileString("ExpLimit", szKey, NULL, theMain.m_szIniFile);
		sprintf(szKey, "%s_Repository", P_PLAYER->Name);
		WritePrivateProfileString("ExpLimit", szKey, NULL, theMain.m_szIniFile);
		memset(&Last_Save_Repository, 0, sizeof(Last_Save_Repository));
		sscanf(szOld, "%d,%d,%d", &Last_Save_Repository.nMapID, &Last_Save_Repository.x, &Last_Save_Repository.y);
		MultiByteToWideChar(CP_ACP, 0, szOld, -1, szVal, 64);
		WritePrivateProfileStringW(L"ExpLimit", L"Repository", szVal, s_wszCfgFile);
		swprintf_s(szVal, L"%d", m_nExpStep);
		WritePrivateProfileStringW(L"ExpLimit", L"Step", szVal, s_wszCfgFile);
	}
	if (m_nExpStep < 0 || m_nExpStep >= exp_num) m_nExpStep = 0;
	s_nLastMapID = 0;
	if (m_nExpStep)
		SystemMessage(0, "<color=yellow>Gioi han Exp: chay tiep buoc %s", s_szStep[m_nExpStep]);
}

// Goi tu Breathe (1s). TRUE = dang chay Gioi han Exp
BOOL CVLBasic::ExpLimitCheck()
{
	LoadExpState();

	if (!m_bExpLimit)
	{
		// Vua khoi dong lai Hook: cho VLAuto gui tham so roi moi coi la bo check
		if (m_nExpStep && g_RunningTimer > 200)
		{
			SetExpStep(exp_none);
			theMap.m_nLastMapID = 0;
			SystemMessage("<color=yellow>Gioi han Exp: <color>Tat");
		}
		return FALSE;
	}

	if (m_nExpStep) return TRUE;

	if (m_nExpMax <= m_nExpMin) return FALSE;
	if (theUtil.m_bDoingUtil) return FALSE;
	if (P_PLAYER->m_Level < m_nExpLevel) return FALSE;
	if (GetExpPercent() < m_nExpMax) return FALSE;

	// Bat dau: dung danh quai, xoa muc tieu
	theAttack.SetPeopleIdx(0);
	P_PLAYER->m_nObjectIdx = 0;
	PathTravelStop("Gioi han Exp");
	memset(&Last_Save_Repository, 0, sizeof(Last_Save_Repository));
	SetExpStep(exp_return);
	return TRUE;
}

// Chay toi (x, y): theo MapPath cua map (Maps\Path), het duong / khong co MapPath thi chay thang doan con lai. FALSE = da toi
static BOOL RunTo(int x, int y)
{
	int nPlayerX, nPlayerY;
	P_PLAYER->GetMapPos(&nPlayerX, &nPlayerY);
	int dZ = __distance(nPlayerX, nPlayerY, x, y);
	if (dZ <= 64) return FALSE;

	if (!s_bDirect)
	{
		int nWalk = PathWalkTo(x, y);
		if (nWalk > 0) return TRUE;
		s_bDirect = TRUE;
		theMap.m_nLagCount = 0;
		theMap.m_nLastdZ = MAX_INTNUM;
	}
	if (theMap.IsLagMoving(nPlayerX, nPlayerY, dZ))
	{
		s_bDirect = FALSE;
		PathWalkReset();
		return TRUE;
	}
	if (dZ > 500)
	{
		x = nPlayerX + (x - nPlayerX) * 500 / dZ;
		y = nPlayerY + (y - nPlayerY) * 500 / dZ;
	}
	g_pCurPlayer->MoveTo(x, y);
	return TRUE;
}

// Chay toi (x, y) va mo ruong o do (= luu ruong). TRUE = da mo xong (hoac khong co ruong)
static BOOL SaveRepositoryAt(int x, int y)
{
	if (RunTo(x, y)) return FALSE;
	int nRet = theMap.OpenStoreBox();
	if (nRet == 0) return FALSE;
	if (nRet < 0)
		SystemMessage(0, "<color=yellow>Gioi han Exp: khong thay ruong o %d,%d", x, y);
	return TRUE;
}

static BOOL IsStoreBoxOpen()
{
	KWndImage* pKUiStoreBox = (KWndImage*)(*(LPDWORD)SBO_BASE_ADD);
	return pKUiStoreBox && pKUiStoreBox->IsVisible();
}

// Ruong dang mo: rut cho du MIN_MONEY trong tui
static void WithdrawFare()
{
	int nWithdraw = MIN_MONEY - EQUIP_MONEY;
	if (nWithdraw > ROOM_MONEY) nWithdraw = ROOM_MONEY;
	if (nWithdraw > 0)
		g_pCoreShell->OperationRequest(GOI_MONEY_INOUT_STORE_BOX, 0, nWithdraw);
}

// Doi xa phu sang map nMapID. TRUE = dang di
static BOOL GoToMap(int nMapID)
{
	if (*g_pMapID == nMapID) return FALSE;
	if (theMap.m_nPoints <= 0)
	{
		if (!s_dwActionTick || GetTickCount() - s_dwActionTick > 10000)
		{
			SystemMessage(0, "<color=yellow>Gioi han Exp: thieu muc [NPC] trong Maps\\Path (map %d), khong tim duoc xa phu", *g_pMapID);
			s_dwActionTick = GetTickCount();
		}
		return TRUE;
	}
	theMap.ChangeMap(nMapID);
	return TRUE;
}

// Chu ky 600ms, thay cho cac chuc nang bi dung
void CVLBasic::ExpLimitBreathe()
{
	if (!m_nExpStep) return;
	if (g_NetworkOffline) return;
	if (P_PLAYER->m_Doing == do_death || P_PLAYER->m_Doing == do_revive)
	{	// Breathe lo hoi sinh; hoi sinh xong tinh lai duong tu dau
		s_bDirect = FALSE;
		PathWalkReset();
		return;
	}

	if (*g_pMapID != s_nLastMapID)
	{	// Doi map: nap toa do NPC trong thanh, tim xa phu gan nhat
		s_nLastMapID = *g_pMapID;
		theMap.m_nPoints = 0;
		theMap.LoadMap(*g_pMapID);
		theMap.m_nCurPoint = -1;
		theMap.m_bDoShoping = 0;
		theMap.m_nTimeWait = 2;
		theMap.NearestDriver();
		s_bDirect = FALSE;
		PathWalkReset();
	}

	switch (m_nExpStep)
	{
	case exp_return:		// Dang tren map train -> dung TDP
		if (!P_PLAYER->m_FightMode)
		{
			SetExpStep(exp_saveold);
			break;
		}
		if (s_dwActionTick && GetTickCount() - s_dwActionTick < 6000) break;
		s_dwActionTick = GetTickCount();
		if (!g_pCurPlayer->UseItem(AMULET_PAPER) && !g_pCurPlayer->UseItem(AMULETX_PAPER))
		{
			KUiPlayerBar* pUiPlayerBar = (KUiPlayerBar*)GetPtrAddress(UPB_BASE_ADD);
			if (pUiPlayerBar && pUiPlayerBar->m_nHK3Gengre)
			{
				ExecuteScript("ShortcutUseItem(2)");
				theMain.PostFeedBack(cmd_useam);
			}
			else
				SystemMessage("<color=yellow>Gioi han Exp: khong co Tho Dia Phu!!!");
		}
		else
			theMain.PostFeedBack(cmd_useam);
		break;

	case exp_saveold:		// Trong thanh: luu ruong o day, ghi lai vi tri, rut tien xa phu
		{
			if (theMain.OpenLockRoom(theMap.m_nRepositoryPass)) break;
			POINT ptRepo = theMap.NpcPoints[npc_Repository];
			int nPlayerX, nPlayerY;
			P_PLAYER->GetMapPos(&nPlayerX, &nPlayerY);
			if (!ptRepo.x || !ptRepo.y || theMap.m_nPoints <= 0)
			{	// Khong co toa do ruong: tim ruong quanh day
				ptRepo.x = nPlayerX;
				ptRepo.y = nPlayerY;
			}
			if (!SaveRepositoryAt(ptRepo.x, ptRepo.y)) break;
			P_PLAYER->GetMapPos(&nPlayerX, &nPlayerY);
			Last_Save_Repository.nMapID = *g_pMapID;
			Last_Save_Repository.x = nPlayerX;
			Last_Save_Repository.y = nPlayerY;
			WithdrawFare();
			CloseAllWindows(0);
			theMap.NearestDriver();
			SetExpStep(exp_goto);
		}
		break;

	case exp_goto:			// Xa phu sang Duong Chau
		if (GoToMap(city_duongchau)) break;
		SetExpStep(exp_savenew);
		break;

	case exp_savenew:		// Luu ruong Duong Chau: chet thi hoi sinh gan cho dung
		if (GoToMap(city_duongchau)) break;
		if (!SaveRepositoryAt(DC_REPO_X, DC_REPO_Y)) break;
		// Cat het tien vao ruong truoc khi ra cho chet (chet roi tien)
		if (EQUIP_MONEY > 0)
		{
			if (!IsStoreBoxOpen() || s_nMoneyTry >= 10)
			{
				SystemMessage(0, "<color=red>Gioi han Exp: khong cat duoc %d luong vao ruong, chet se rot tien!", EQUIP_MONEY);
				s_bNoDeposit = TRUE;
			}
			else
			{
				s_nMoneyTry++;
				g_pCoreShell->OperationRequest(GOI_MONEY_INOUT_STORE_BOX, TRUE, EQUIP_MONEY);
				break;	// Cho server cap nhat, lan sau kiem tra lai
			}
		}
		CloseAllWindows(0);
		SetExpStep(exp_waitdie);
		break;

	case exp_waitdie:		// Dung cho quai danh chet, hoi sinh lai quay ra
		if (GetExpPercent() <= m_nExpMin)
		{
			theMap.NearestDriver();
			SetExpStep(exp_restore);
			break;
		}
		if (!P_PLAYER->m_FightMode && GoToMap(city_duongchau)) break;
		if (!P_PLAYER->m_FightMode && EQUIP_MONEY > 0 && !s_bNoDeposit)
		{	// Con tien trong tui (vd khoi dong lai giua chung): quay lai cat
			SetExpStep(exp_savenew);
			break;
		}
		if (PEOPLE_IDX) theAttack.SetPeopleIdx(0);
		RunTo(DC_DIE_X, DC_DIE_Y);
		break;

	case exp_restore:		// Ve lai ruong cu, luu ruong, chay lai cac chuc nang
		if (Last_Save_Repository.nMapID)
		{
			if (P_PLAYER->m_FightMode)
			{	// Con o ngoai cho dung: chay ve ruong Duong Chau truoc
				RunTo(DC_REPO_X, DC_REPO_Y);
				break;
			}
			if (*g_pMapID == city_duongchau && Last_Save_Repository.nMapID != city_duongchau && !s_bWithdrawn)
			{	// Rut tien xa phu o ruong Duong Chau
				if (!SaveRepositoryAt(DC_REPO_X, DC_REPO_Y)) break;
				if (EQUIP_MONEY < MIN_MONEY && ROOM_MONEY > 0 && IsStoreBoxOpen() && s_nMoneyTry < 10)
				{
					s_nMoneyTry++;
					WithdrawFare();
					break;	// Cho server cap nhat
				}
				CloseAllWindows(0);
				theMap.NearestDriver();
				theMap.m_nCurPoint = -1;
				theMap.m_bDoShoping = 0;
				s_bWithdrawn = TRUE;
				break;
			}
			if (GoToMap(Last_Save_Repository.nMapID)) break;
			if (!SaveRepositoryAt(Last_Save_Repository.x, Last_Save_Repository.y)) break;
		}
		CloseAllWindows(0);
		SystemMessage(0, "<color=yellow>Gioi han Exp: xong (%d%%), chay lai Auto", GetExpPercent());
		SetExpStep(exp_none);
		theMap.m_nLastMapID = 0;	// CVLMap::Breathe nap lai map
		break;
	}
}
