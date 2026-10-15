#include "stdafx.h"
#include "../GameDef.h"
#include "AntiRE.h"
#include "GameBase.h"
#include "JxCore.h"
#include "S3Client.h"
#include "VLClasses.h"

const int MapIDArray[] =
{ 0,1,1,1,1,1,1,1,1,11,11,11,11,11,11,0,0,0,0,11,11,11,11,11,11,11,11,11,11,0,0,0,0,0,0,0,0,37,37,37,37,80,80,80,0,37,37,37,37,37,
37,37,37,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,1,1,1,1,1,1,1,78,78,80,78,78,78,0,0,0,0,0,0,80,80,80,80,80,0,0,0,0,1,
37,80,0,80,0,0,0,0,0,0,0,80,0,80,80,78,78,78,78,78,78,37,80,80,80,78,78,78,78,78,78,37,37,0,0,37,37,0,0,0,37,37,37,37,37,80,80,80,80,80,
80,80,80,162,162,0,0,0,162,0,0,0,162,162,162,162,162,162,162,162,162,162,162,162,162,0,176,0,176,162,162,162,162,176,0,0,0,0,0,0,176,0,0,162,162,162,162,0,162,78,
80,1,11,162,37,80,1,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,176,176,176,176,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,80,78,1,37,0,0,0,0,0,0,0,0,0,0,0,0,0,78,0,0,0,11,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0 };

const unsigned AllowServers[] =
{
	 0x4CE15A67,0x6515C867 ,0xB0E25A67 ,0xA1E05A67
};

//Global Variables
GameData theGameData;
int g_nAccrueSeries[series_num];
int	g_nConquerSeries[series_num];

//Global Functions
void GetHardDriveComputerID(char* HardDriveID);

void g_InitSeries()
{
	g_nAccrueSeries[series_metal] = series_water;
	g_nConquerSeries[series_metal] = series_wood;
	g_nAccrueSeries[series_wood] = series_fire;
	g_nConquerSeries[series_wood] = series_earth;
	g_nAccrueSeries[series_water] = series_wood;
	g_nConquerSeries[series_water] = series_fire;
	g_nAccrueSeries[series_fire] = series_earth;
	g_nConquerSeries[series_fire] = series_metal;
	g_nAccrueSeries[series_earth] = series_metal;
	g_nConquerSeries[series_earth] = series_water;
}

//================================Class CVLMain================================
CVLMain::CVLMain(void)
{
	m_bPause = FALSE;
	m_hWndGame = m_hWndAuto = NULL;
	m_bQuitingGame = 0;
	m_uHardDiskID = 0;
}

BOOL CVLMain::IsPEPacked()
{
	char szFuncName[16] = "SHF5bLvSdfnhg"; //"PEC2_IsPacked"
	int nLen = strlen(szFuncName);
	for (int i = 0; i < nLen; i++)
		szFuncName[i] = szFuncName[i] - 3;

	typedef DWORD(WINAPI * PFNPEC2_IsPacked)();
	PFNPEC2_IsPacked PEC2_IsPacked = (PFNPEC2_IsPacked)GetProcAddress((HMODULE)-1, szFuncName);

	if (PEC2_IsPacked)
		return PEC2_IsPacked();

	return FALSE;
}

BOOL CVLMain::PostFeedBack(WPARAM wParam, LPARAM lParam)
{
	if (lParam == -1)
		lParam = (LPARAM)m_hWndGame;
	return _PostMessage(m_hWndAuto, WM_FEEDBACK, wParam, lParam);
}

void CVLMain::SetDllFilePath(char* szDllFile)
{
	strcpy(m_szCurFolder, szDllFile);
	char* pStr = strrchr(m_szCurFolder, '\\');
	pStr[0] = 0;
	sprintf(m_szIniFile, "%s\\VLAutoPr.ini", m_szCurFolder);

	char HardDriveID[33] = "h";
	GetHardDriveComputerID(HardDriveID + 1);	// Get HDD Code
	m_uHardDiskID = HashString2ID(HardDriveID);
	memset(HardDriveID, 0, sizeof(HardDriveID));
}

int CVLMain::CheckRegistration()
{
	JUNK_CODE_ONE;
	if (!P_PLAYER->m_dwID)
		return 1984;

	JUNK_CODE_ONE;
	char szCharName[33] = "c";
	strcat(szCharName, P_PLAYER->Name);		// Get Char Name
	unsigned int uCHR_ID = HashString2ID(szCharName);

	JUNK_CODE_ONE;
	BOOL m_bNameValid = (HashString2ID(szCharName + 1) == g_pCurPlayer->m_dwHashID);

	JUNK_CODE_ONE;
	for (int i = 0; i < MAX_LIC_COUNT; i++)
	{
		if (theGameData.RegCode[i] == m_uHardDiskID)
			return 1984;
		JUNK_CODE_ONE;
		if (m_bNameValid && theGameData.RegCode[i] == uCHR_ID)
			return 1984;
	}

	int nLevel = P_PLAYER->m_Level;
	JUNK_CODE_ONE;
	__asm { add nLevel, 30 }
	JUNK_CODE_ONE;

	if (nLevel < 50)
		return 1984;

	return 1977; // Not Registered
}

void CVLMain::InitParams(HWND hWndGame, HWND hWndServer)
{
	//Window Handles
	m_hWndGame = hWndGame;
	m_hWndAuto = hWndServer;
	g_InitSeries();

	//Load Options
	LoadOptions();
	theRecover.LoadX2List();
	theCollect.LoadBlackList();
	theCollect.LoadRedList();
	theParty.LoadNoPTList();
	theMap.LoadSellList();

	//Inject Codes
	//theCollect.InjectCode();
}

void CVLMain::InitGameData()
{
	g_pCurPlayer = (KPlayer*)(GetPtrAddress(PLR_BASE_ADD) + PLAYER_OFFSET);
	DWORD dwNpcBaseAdd = GetPtrAddress(NPC_BASE_ADD);
	for (int i = 0; i < MAX_NPC; i++)
		g_pNpcArray[i] = (KNpc*)(dwNpcBaseAdd + i * NPC_DATA_SIZE);

	dwObjBaseAdd = GetPtrAddress(OBJ_BASE_ADD);
	for (int i = 0; i < MAX_OBJ; i++)
		g_pObjArray[i] = (KObj*)(dwObjBaseAdd + i * OBJ_DATA_SIZE);

	dwItmBaseAdd = GetPtrAddress(ITM_BASE_ADD);
	for (int i = 0; i < MAX_ITM; i++)
		g_pItemArray[i] = (KItem*)(dwItmBaseAdd + i * ITM_DATA_SIZE);

	g_pTeam = (KTeam*)(TEAM_ADDRESS);
	g_pBuySell = (KBuySell*)(BSL_BASE_ADD);
	g_pCoreShell = (KCoreShell*)GetPtrAddress(CORE_BASE_ADD);
	g_UiInformation = (KUiInformation*)(IF1_BASE_ADD);
	g_UiInformation2 = (KUiInformation2*)(IF2_BASE_ADD);

	KUiTongManagement* pTongMg = (KUiTongManagement*)TMC_BASE_ADD;
	if (pTongMg)	// 10-10: TMC chua tim
	{
		pTongMg->m_nTongListPage = 0;
		pTongMg->PrepareTongList(2);
	}

	// Map Addresses
	g_pMapID = (int*)(MAP_ID_ADD);
	g_szMapName = (char*)(MAP_NAME_ADD);

	/**
	// Server IP
	DWORD dwJxBase = (DWORD)::GetModuleHandle("JX.Dll");
	unsigned* pServerIp = (unsigned*)(dwJxBase + 0x00057720);

	BOOL bFound = FALSE;
	for (auto i = 0; i < COUNT_OF(AllowServers); i++)
	{
		if (*pServerIp == AllowServers[i])
		{
			bFound = TRUE;
			break;
		}
	}

	if (!bFound)
		return;
	**/
	__asm
	{
		mov ExecuteScript, EXS_FUNC_ADD;
		mov SendClientCmdSkill, SKL_FUNC_ADD;
		mov ProcessPeople, PCP_FUNC_ADD;
		mov OnSendSomeoneMessage, OSM_FUNC_ADD;
		mov SystemMessageArrival, SMA_FUNC_ADD;
		mov OpenStoreWindow, SBO_FUNC_ADD;
		mov OpenShopWindow, SHO_FUNC_ADD;
		mov CloseAllWindows, CLA_FUNC_ADD;
	}
}

void CVLMain::LoadOptions()
{
	m_bSilentMode = GetPrivateProfileInt("Options", "SilentMode", 1, m_szIniFile);
	m_bAutoChat = GetPrivateProfileInt("Options", "AutoChat", 1, m_szIniFile);
	m_bPrintScreen = GetPrivateProfileInt("Options", "PrintScreen", 0, m_szIniFile);
	m_bAttackBossFirst = GetPrivateProfileInt("Options", "AttackBossFirst", 1, m_szIniFile);
	GetPrivateProfileString("Options", "ControlPass", "", m_szControlPass, 32, m_szIniFile);
}

void CVLMain::ExitGame(int nLicCheck)
{
	char szExitScript[8] = "";
	szExitScript[3] = 't';
	szExitScript[4] = '(';
	szExitScript[5] = ')';
	if ((nLicCheck ^ 1977) == 121) return;
	szExitScript[0] = 'E';
	szExitScript[1] = 'x';
	szExitScript[2] = 'i';
	ExecuteScript(szExitScript);
	memset(szExitScript, 0, sizeof(szExitScript));
}

void CVLMain::QuitGame()
{
	m_bQuitingGame = TRUE;
}

void CVLMain::RestartHook()
{
	PostFeedBack(cmd_restart);
}

void CVLMain::SetPause(BOOL bPause)
{
	m_bPause = bPause;
	theBasic.m_nIdleTimeCount = 0;

	if (!bPause && !PEOPLE_IDX)
		theAttack.SetPeopleIdx(0);
}

void CVLMain::GetContainerPos(int nRoom, int* nContainer, int* nPosition)
{
	*nContainer = 0;
	*nPosition = 0;
	switch (nRoom)
	{
	case room_equipment:
		*nContainer = UOC_ITEM_TAKE_WITH;
		*nPosition = pos_equiproom;
		break;
	case room_repository:
		*nContainer = UOC_STORE_BOX;
		*nPosition = pos_repositoryroom;
		break;
	case room_repo02:
		*nContainer = UOC_STORE_BOX2;
		*nPosition = pos_repo02;
		break;
	case room_repo03:
		*nContainer = UOC_STORE_BOX3;
		*nPosition = pos_repo03;
		break;
	case room_guid:
		*nContainer = UOC_GUID_BOX;
		*nPosition = pos_guid;
		break;
	case room_trade:
		*nContainer = UOC_TRADE_BOX;
		*nPosition = pos_traderoom;
		break;
	case room_num:
		*nContainer = UOC_GAMESPACE;
		*nPosition = 0;
		break;
	}
}

BOOL CVLMain::OpenLockRoom(int nRoomPass)
{
	static int nTimerCount = 0;
	if (!nRoomPass) return 0;
	if (!g_pCoreShell->ProcessGameSecurity(1, 0, 0)) return 0;
	if (g_pCoreShell->ProcessGameSecurity(0, 0, 0)) // Room being locked
	{
		if (nTimerCount < g_RunningTimer)
		{
			g_pCoreShell->ProcessGameSecurity(6, nRoomPass, 0); // Unlock the room
			nTimerCount = g_RunningTimer + 100;					// Wait for next 5s
		}
		return TRUE;
	}
	return 0;
}
void CVLMain::CloseLockRoom()
{
	if (!g_pCoreShell->ProcessGameSecurity(1, 0, 0)) return;
	g_pCoreShell->ProcessGameSecurity(5, 0, 0);
}

void CVLMain::GetMapName(int nMapID, char* szMapName)
{
	char szMapNameFile[MAX_PATH];
	sprintf(szMapNameFile, "%s\\Maps\\MapNames.ini", m_szCurFolder);
	char szMapID[12];
	sprintf(szMapID, "%d", nMapID);
	GetPrivateProfileString("Map_Name", szMapID, "<Unknown_Map>", szMapName, 64, szMapNameFile);
}

int CVLMain::GetSubCity(int nCityID)
{
	if (nCityID >= city_banghoimin && nCityID <= city_banghoimax)
	{
		return city_daily;
	}
	switch (nCityID)
	{
	case city_phuongtuong:
		return city_vinhlac;
	case city_thanhdo:
		return city_giangtan;
	case city_bienkinh:
		return city_chutien;
	case city_tuongduong:
		return city_balang;
	case city_duongchau:
		return city_daohuong;
	case city_daily:
		return city_thachco;
	case city_laman:
		return city_longtuyen;
	}
	return nCityID;
}

int CVLMain::GetOwnerCity(int nTownID)
{
	if (nTownID >= city_banghoimin && nTownID <= city_banghoimax)
	{
		return city_phuongtuong;
	}
	switch (nTownID)
	{
	case city_longmon:
	case city_vinhlac:
		return city_phuongtuong;
	case city_giangtan:
		return city_thanhdo;
	case city_chutien:
		return city_bienkinh;
	case city_balang:
		return city_tuongduong;
	case city_daohuong:
		return city_duongchau;
	case city_thachco:
		return city_daily;
	case city_longtuyen:
		return city_laman;
	}
	return nTownID;
}

void CVLMain::GetCityName(int nCityID, char* szCityName)
{
	switch (nCityID)
	{
	case city_phuongtuong:
		strcpy(szCityName, "Ph­îng T­êng");
		break;
	case city_thanhdo:
		strcpy(szCityName, "Thµnh §«");
		break;
	case city_bienkinh:
		strcpy(szCityName, "BiÖn Kinh");
		break;
	case city_tuongduong:
		strcpy(szCityName, "T­¬ng D­¬ng");
		break;
	case city_duongchau:
		strcpy(szCityName, "D­¬ng Ch©u");
		break;
	case city_daily:
		strcpy(szCityName, "§¹i Lý");
		break;
	case city_laman:
		strcpy(szCityName, "L©m An");
		break;
	default:
		GetMapName(nCityID, szCityName);
	}
}

int CVLMain::GetReturnCity(int nTrainMapID)
{
	if (nTrainMapID == 875)
		return city_laman;

	//if (nTrainMapID == 998 || nTrainMapID == 999)
	//	return city_daily;

	if (nTrainMapID == 990)
		return city_tuongduong;

	if (nTrainMapID == 996 || nTrainMapID == 997)
		return city_duongchau;

	//if (nTrainMapID == 997)
	//	return city_bienkinh;

	//if (nTrainMapID == 998)
	//	return city_thanhdo;

	if (nTrainMapID >= 500 || nTrainMapID < 0)
		return 0;

	return MapIDArray[nTrainMapID];
}

int CVLMain::GetLevelExp(int nLevel)
{
	if (nLevel < 1 || nLevel > 200) return 0;
	int* pLevelExp = (int*)LEVEL_EXP_TB;
	return pLevelExp[nLevel - 1];
}

int CVLMain::GetExpUpLevel(int nLevel1, int nLevel2)
{
	if (nLevel1 < 1 || nLevel1 > 200) return -1;
	if (nLevel2 < 1 || nLevel2 > 200) return -1;

	int* pLevelExp = (int*)LEVEL_EXP_TB;
	int nResult = 0;
	int nLevel = nLevel1;
	while (nLevel < nLevel2)
	{
		nResult += pLevelExp[nLevel - 1];
		nLevel++;
	}
	return nResult;
}

int CVLMain::GetConquerVal(int nSeries1, int nSeries2)
{
	if (g_nConquerSeries[nSeries1] == nSeries2)
		return 1;
	if (g_nConquerSeries[nSeries2] == nSeries1)
		return 0;
	return 0;
}

void CVLMain::UpdateTongList()
{
	KUiTongManagement* pTongMg = (KUiTongManagement*)TMC_BASE_ADD;

	TongList* pTongLst = pTongMg ? pTongMg->GetTongList() : NULL;	// 10-10: TMC chua tim
	if (pTongLst)
	{
		int nTongCount = pTongLst->nTongCount;
		if (nTongCount > MAX_TOP_TONG) nTongCount = MAX_TOP_TONG;

		if (g_GlobalTongInfo) delete[] g_GlobalTongInfo;
		g_GlobalTongInfo = new GlobalTongInfo[MAX_TOP_TONG + 1];
		memset(g_GlobalTongInfo, 0, (MAX_TOP_TONG + 1) * sizeof(GlobalTongInfo));

		for (int i = 0; i < nTongCount; i++)
		{
			strcpy(g_GlobalTongInfo[i].szTongName, pTongLst->cTongInfo[i].szTongName);
			g_GlobalTongInfo[i].dwTongID = HashString2ID(g_GlobalTongInfo[i].szTongName);
			g_GlobalTongInfo[i].bIsRestricted = TRUE;
		}
	}
	else if (g_GlobalTongInfo)
	{
		delete[] g_GlobalTongInfo;
		g_GlobalTongInfo = NULL;
	}
}

void CVLMain::ClearEverything()
{
	if (g_GlobalTongInfo)
	{
		delete g_GlobalTongInfo;
		g_GlobalTongInfo = NULL;
	}

	//UnInject Codes
	//theCollect.InjectCode(0);

	theAttack.m_bAutoHit = 0;
	theAttack.SetAutoHit();
}

int CVLMain::GetMenuCount()
{
	KUiMsgSel* pMsgSel = (KUiMsgSel*)GetPtrAddress(MSG_BASE_ADD);
	KUiMsgSel2* pMsgSel2 = (KUiMsgSel2*)GetPtrAddress(MSG2_BASE_ADD);

	if (pMsgSel2 && pMsgSel2->IsVisible())
		return pMsgSel2->m_nNumMessage;

	if (pMsgSel && pMsgSel->IsVisible())
		return pMsgSel->m_nNumMessage;

	return 0;
}

BOOL CVLMain::ClickMenuIdx(int nIdx)
{
	KUiMsgSel* pMsgSel = (KUiMsgSel*)GetPtrAddress(MSG_BASE_ADD);
	KUiMsgSel2* pMsgSel2 = (KUiMsgSel2*)GetPtrAddress(MSG2_BASE_ADD);

	if (pMsgSel2 && pMsgSel2->IsVisible())
	{
		pMsgSel2->OnClickMsg(nIdx);
		return TRUE;
	}
	if (pMsgSel && pMsgSel->IsVisible())
	{
		pMsgSel->OnClickMsg(nIdx);
		return TRUE;
	}
	return 0;
}

BOOL CVLMain::ClickMenuStr(const char* szLine, BOOL bClick)
{
	KUiMsgSel* pMsgSel = (KUiMsgSel*)GetPtrAddress(MSG_BASE_ADD);
	KUiMsgSel2* pMsgSel2 = (KUiMsgSel2*)GetPtrAddress(MSG2_BASE_ADD);

	if (pMsgSel2 && pMsgSel2->IsVisible())
	{
		for (int i = 0; i < pMsgSel2->m_nNumMessage; i++)
		{
			char* szMsg = (char*)pMsgSel2->m_pMessages[i]->Msg;
			if (strstr(szMsg, szLine))
			{
				if (bClick)
					pMsgSel2->OnClickMsg(i);
				return TRUE;
			}
		}
	}

	if (pMsgSel && pMsgSel->IsVisible())
	{
		for (int i = 0; i < pMsgSel->m_nNumMessage; i++)
		{
			char* szMsg = (char*)pMsgSel->m_pMessages[i]->Msg;
			if (strstr(szMsg, szLine))
			{
				if (bClick)
					pMsgSel->OnClickMsg(i);
				return TRUE;
			}
		}
	}

	return 0;
}
