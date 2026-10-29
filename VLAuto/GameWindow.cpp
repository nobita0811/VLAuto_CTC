// GameWindow.cpp : implementation file
//

#include "stdafx.h"
#include "../GameDef.h"
#include "AntiRE.h"
#include "DWinProcess.h"
#include "GameCore.h"
#include "VLAuto.h"
#include "VLAutoDlg.h"
#include "DlgMap.h"
#include "GameWindow.h"

// Khoi tien o cuoi file cfg nhan vat
#define CFG_MONEY_TAG		0x594E4F4D	// 'MONY'
#define STAT_START_DELAY	5000		// ms sau khi vao game moi lay tien ban dau (cho dong bo vat pham)
#define TONG_MONEY_TIMEOUT	60000		// ms cho tien giam sau lenh gom tien len bang (phu ve thanh + rut tien ruong + gop)
#define MONEY_SAVE_INTERVAL	30000		// ms: tien thay doi thi ghi vao cfg toi da moi 30s

#pragma pack(push, 4)
struct CFG_MONEY_BLOCK
{
	DWORD	dwTag;
	__int64	nMoney;
};
#pragma pack(pop)

struct WinMessage
{
	UINT uMsg;
	WPARAM wParam;
	LPARAM lParam;
};

const WinMessage AutoParamMsg[] =
{
	{ WM_HOOK_BASIC, NULL, sizeof(PARAM_BASIC) },
	{ WM_HOOK_RECOVER, NULL, sizeof(PARAM_RECOVER) },
	{ WM_HOOK_ATTACK, NULL, sizeof(PARAM_ATTACK) },
	{ WM_HOOK_COLLECT, NULL, sizeof(PARAM_COLLECT) },
	{ WM_HOOK_MOVE, NULL, sizeof(PARAM_MOVE) },
	{ WM_HOOK_PARTY, NULL, sizeof(PARAM_PARTY) },
	{ WM_HOOK_MAP, NULL, sizeof(PARAM_MAP) },
	{ WM_HOOK_YESOU, NULL, sizeof(PARAM_YESOU) },
};

// GameWindow Class
GameWindow::GameWindow(HWND hWnd)
{
	InitValue();
	m_hWnd = hWnd;
}

void GameWindow::InitValue()
{
	memset(this, 0, sizeof(GameWindow));

	strcpy(m_szPlayerName, VLTK_UNNAMED);
	m_nPlayerLevel = -1;
	m_nNextExp = 100;
	m_nCurLifeMax = 100;
	m_nCurManaMax = 100;

	m_prBasic.InitValue();
	m_prRecover.InitValue();
	m_prAttack.InitValue();
	m_prCollect.InitValue();
	m_prMove.InitValue();
	m_prParty.InitValue();
	m_prMap.InitValue();
	m_prYeSou.InitValue();
}

void GameWindow::Refresh()
{
	if (!IsWindow(m_hWnd))
	{
		m_hWnd = NULL;
		return;
	}

	m_bInfoChanged = FALSE;

	m_nTimerCount++;

	KNpc Npc;
	LPBYTE lpBaseAdd, lpCurAdd;
	DWinProcess winProcess(m_hWnd);

	if (!m_bVersionChecked)
	{
		// Check Game Version - BEGIN
		/*TCHAR szGameY[MAX_PATH];
		if (!winProcess.GetModuleFileNameEx(NULL, szGameY, MAX_PATH)) return;
		HANDLE hFile = CreateFile(szGameY, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
		if (hFile == INVALID_HANDLE_VALUE) return;
		DWORD dwFileSize = GetFileSize(hFile, NULL);
		CloseHandle(hFile);

		if (dwFileSize != GAMEY_SIZE)
		{
			if (strcmp(m_szPlayerName, VLTK_WRONGVER))
			{
				strcpy(m_szPlayerName, VLTK_WRONGVER);
				m_bInfoChanged = TRUE;
			}
			return;
		}*/

		m_bVersionChecked = TRUE;
		// Check Game Version - END
	}

	winProcess.ReadMemory((LPVOID)PLR_BASE_ADD, &lpBaseAdd, sizeof(LPBYTE)); lpBaseAdd += PLAYER_OFFSET;
	lpCurAdd = lpBaseAdd + PLR_IDX_OFFSET;
	winProcess.ReadMemory(lpCurAdd, &m_nPlayerIndex, sizeof(int));

	if (!winProcess.ReadMemory((LPBYTE)NPC_BASE_ADD, (LPVOID)&lpBaseAdd, 4)) return;
	lpCurAdd = lpBaseAdd + m_nPlayerIndex * NPC_DATA_SIZE;
	if (!winProcess.ReadMemory(lpCurAdd, (LPVOID)&Npc, sizeof(Npc))) return;

	if (Npc.m_Index == 0) // Disconnected
	{
		if (m_nPlayerLevel < 0)
		{
			m_nPlayerLevel = 0;
			m_bInfoChanged = TRUE;
		}
		else if (strcmp(m_szPlayerName, VLTK_UNNAMED)) // Connected => Disconnected
		{
			// Config luu theo ten nhan vat => SaveProfile truoc khi xoa ten
			if (m_bLoadDone)
				SaveProfile();

			m_nExp = 0;
			strcpy(m_szPlayerName, VLTK_UNNAMED);
			m_bInfoChanged = TRUE;

			if (m_bLoadDone)
			{
				m_bLoadDone = 0;
				m_bRegistered = 0;
				m_bAutoStartDone = 0;
				m_dwAutoStartTick = 0;
			}
			m_dwLoginTick = 0;
			m_bStatReady = FALSE;
			m_bDead = FALSE;
		}
		return;
	}

	if (!strcmp(m_szPlayerName, VLTK_UNNAMED)) // Disconnected => Connected
	{
		// Check DevIoCode
		BOOL bIsCodeOK = TRUE;
		/*BYTE brDevIoCode[DEVIO_CODE_SIZE];
		INT_PTR PtrDevIoCtrlAdd = GetDeviceIoControlAdd();
		if (!winProcess.ReadMemory((LPBYTE)PtrDevIoCtrlAdd, (LPVOID)brDevIoCode, DEVIO_CODE_SIZE)) return;
		for (int i = 0; i < DEVIO_CODE_SIZE; i++)
		{
			if (((LPBYTE)PtrDevIoCtrlAdd)[i] != brDevIoCode[i])
			{
				bIsCodeOK = FALSE;
				break;
			}
		}*/

		if (!winProcess.ReadMemory((LPCVOID)ACNT_ID_ADD, (LPVOID)m_szAccountID, sizeof(m_szAccountID))) return;
		// 10-10: KLogin::m_Choices.Account luu dang dao bit (~c), 32 byte
		for (int i = 0; i < 32 && i < (int)sizeof(m_szAccountID); i++)
			m_szAccountID[i] = ~m_szAccountID[i];
		m_szAccountID[sizeof(m_szAccountID) - 1] = 0;
		if (!m_szAccountID[0]) return;

		/*if (!winProcess.ReadMemory((LPCVOID)SERVERIP_ADD, (LPVOID)&m_dwServerIP, sizeof(m_dwServerIP))) return;
		if (!m_dwServerIP) return;*/

		m_nPlayerLevel = Npc.m_Level;
		strcpy(m_szPlayerName, Npc.Name);
		m_bInfoChanged = TRUE;

		// Thong ke lan dang nhap nay
		m_nMoney = m_nStartMoney = m_nSavedMoney = 0;
		m_dwLoginTick = GetTickCount() | 1;
		m_dwMoneySaveTick = 0;
		m_bStatReady = FALSE;
		m_bDead = FALSE;
		m_nAMUsed = 0;
		m_nDeathCount = 0;
		m_nEquipMoney = m_nTongMoney = m_nTongPending = m_nTongBase = 0;
		m_dwTongTick = 0;

		LoadProfile();
		m_bLoadDone = TRUE;
		m_bRegistered = bIsCodeOK && IsRegistered();

		// Check for same server
		/*for (int i = 0; i < g_pCharList->GetItemCount(); i++)
		{
			GameWindow* pGameWin = (GameWindow*)g_pCharList->GetItemData(i);
			if (!pGameWin->m_hWnd || pGameWin->m_hWnd == m_hWnd) continue;
			if (pGameWin->m_dwServerIP != m_dwServerIP) continue;

			if (!strcmp(pGameWin->m_szPlayerName, Npc.Name))
			{
				m_nPlayerLevel = 100;
				m_bRegistered = 0;
				break;
			}
		}*/

		if (!m_bRegistered && m_bAttached)
		{
			for (int i = 0; i < g_pCharList->GetItemCount(); i++)
			{
				GameWindow* pGameWin = (GameWindow*)g_pCharList->GetItemData(i);
				if (pGameWin->m_hWnd == m_hWnd)
				{
					g_pCharList->SetItemState(i, 0x1000, LVIS_STATEIMAGEMASK);
					break;
				}
			}
		}
	}

	m_nCurLife = Npc.m_CurLife;
	m_nCurLifeMax = Npc.GetLifeMax();
	m_nCurMana = Npc.m_CurMana;
	m_nCurManaMax = Npc.GetManaMax();
	winProcess.ReadMemory((LPBYTE)MAP_ID_ADD, (LPVOID)&m_nMapID, sizeof(m_nMapID));
	winProcess.ReadMemory((LPBYTE)FLAG_X_OFFST, (LPVOID)&m_nFlagX, 8);
	m_nFlagX <<= 4;	m_nFlagY <<= 5;
	Npc.GetMapPos(&m_nOffX, &m_nOffY);
	m_FightMode = Npc.m_FightMode;
	UpdateStats(winProcess, Npc.m_Doing);
	memset(m_szEnemyName, 0, 32);
	m_nEnemySeries = -1;
	m_nEnemyLife = 0;
	m_nEnemyLifeMax = 100;
	m_nEnemyLevel = 0;

	if (Npc.m_nPeopleIdx > 1)
	{
		lpCurAdd = lpBaseAdd + Npc.m_nPeopleIdx*NPC_DATA_SIZE;
		winProcess.ReadMemory(lpCurAdd, (LPVOID)&Npc, sizeof(Npc));
		strncpy(m_szEnemyName, Npc.Name, 31);
		m_nEnemySeries = Npc.m_Series;
		m_nEnemyLife = Npc.m_CurLife;
		m_nEnemyLifeMax = Npc.GetLifeMax();
		m_nEnemyLevel = Npc.m_Level;
	}

	__int64 nExp;
	winProcess.ReadMemory((LPBYTE)PLR_BASE_ADD, (LPVOID)&lpBaseAdd, 4);
	lpBaseAdd += PLAYER_OFFSET;
	lpCurAdd = lpBaseAdd + PLR_EXP_OFFSET;
	winProcess.ReadMemory(lpCurAdd, (LPVOID)&nExp, 8);
	lpCurAdd = lpBaseAdd + PLR_EXP_OFFSET + 8;
	winProcess.ReadMemory(lpCurAdd, (LPVOID)&m_nNextExp, 8);

	if (m_nNextExp == 0)
	{
		m_nExp = 0;
		return;
	}

	if (nExp < m_nExp || ((nExp - m_nExp) * 10000 / m_nNextExp > 0))
	{
		m_nExp = nExp;
		m_bInfoChanged = TRUE;
	}
}

// UserConfig\<ten nhan vat>.cfg - giu nguyen byte TCVN3 (doc theo cp1252, khong phu thuoc ACP may),
// ten file khong co ky tu Unicode tieng Viet => doc duoc tren Win7. Ky tu cam trong ten file -> '_'
BOOL GameWindow::GetProfileFileName(LPTSTR szFileName)
{
	if (!m_szPlayerName[0] || !strcmp(m_szPlayerName, VLTK_UNNAMED)) return FALSE;

	WCHAR szName[33] = L"";
	MultiByteToWideChar(1252, 0, m_szPlayerName, -1, szName, 32);
	szName[32] = 0;
	for (int i = 0; szName[i]; i++)
		if (szName[i] < 0x20 || wcschr(L"\\/:*?\"<>|", szName[i]))
			szName[i] = L'_';

	_stprintf(szFileName, _T("%s\\UserConfig\\%s.cfg"), g_szAppFolder, szName);
	return TRUE;
}

void GameWindow::LoadProfile()
{
	TCHAR szFileName[MAX_PATH];
	if (!GetProfileFileName(szFileName)) return;

	if (LoadConfigFile(szFileName)) return;

	// Chua co file trong UserConfig: doc file cu trong UiConfig (theo nhan vat, roi theo tai khoan), luu sang UserConfig
	TCHAR szOldFile[MAX_PATH];
	_stprintf(szOldFile, _T("%s\\UiConfig\\%s"), g_szAppFolder, _tcsrchr(szFileName, _T('\\')) + 1);
	BOOL bLoaded = LoadConfigFile(szOldFile);
	if (!bLoaded)
	{
		TCHAR szAccountID[32] = _T("");
		MultiByteToWideChar(CP_ACP, 0, m_szAccountID, -1, szAccountID, 32);
		if (!szAccountID[0]) return;
		_stprintf(szOldFile, _T("%s\\UiConfig\\%s.cfg"), g_szAppFolder, szAccountID);
		bLoaded = LoadConfigFile(szOldFile);
	}
	if (bLoaded)
		SaveConfigFile(szFileName);
}

BOOL GameWindow::LoadConfigFile(LPCTSTR szFileName)
{
	CFile cfgFile;
	if (!cfgFile.Open(szFileName, CFile::modeRead | CFile::shareDenyWrite))
		return FALSE;

	//Identification
	char szVersion[12] = "";
	cfgFile.Read(szVersion, sizeof(szVersion));
	if (strcmp(szVersion, _SZCURVERSION))
	{
		cfgFile.Close();
		return FALSE;
	}

	char szAccountID[32] = "";
	cfgFile.Read(szAccountID, sizeof(szAccountID));

	if (!m_prBasic.ReadFromFile(cfgFile))
		return FALSE;
	if (!m_prRecover.ReadFromFile(cfgFile))
		return FALSE;
	if (!m_prAttack.ReadFromFile(cfgFile))
		return FALSE;
	if (!m_prCollect.ReadFromFile(cfgFile))
		return FALSE;
	if (!m_prMove.ReadFromFile(cfgFile))
		return FALSE;
	if (!m_prParty.ReadFromFile(cfgFile))
		return FALSE;
	if (!m_prMap.ReadFromFile(cfgFile))
		return FALSE;
	if (!m_prYeSou.ReadFromFile(cfgFile))
		return FALSE;

	// Khoi tien (them sau, file cu khong co)
	CFG_MONEY_BLOCK money = { 0 };
	if (cfgFile.Read(&money, sizeof(money)) == sizeof(money) && money.dwTag == CFG_MONEY_TAG)
		m_nSavedMoney = money.nMoney;

	// File cua nhan vat khac => bo skill da chon (file cu luu ten tai khoan van giu)
	if (strcmp(m_szPlayerName, szAccountID) && strcmp(m_szAccountID, szAccountID))
		memset(&m_prAttack.m_SettingSkills, 0, sizeof(m_prAttack.m_SettingSkills));

	// Chi cap nhat giao dien khi day la nhan vat dang chon: cac tab doc/ghi g_pCurGameWin, co the NULL
	// (nhan vat vao game khi chua chon ai - crash doc NULL+0x844 m_prMap.m_nTrainMapID) hoac la nhan vat khac
	if (this == g_pCurGameWin)
	{
		CVLAutoDlg *pMainDlg = (CVLAutoDlg*)theApp.GetMainDlg();
		((CDlgMap*)pMainDlg->m_MainTab.GetTabDlg(tab_map))->UpdateTrainMap(0);
		pMainDlg->m_MainTab.GetTabDlg(tab_basic)->UpdateData(0);
	}

	cfgFile.Close();

	return TRUE;
}

void GameWindow::SaveProfile()
{
	TCHAR szFileName[MAX_PATH];
	if (!GetProfileFileName(szFileName)) return;

	SaveConfigFile(szFileName);
}

BOOL GameWindow::SaveConfigFile(LPCTSTR szFileName)
{
	CFile cfgFile;
	if (!cfgFile.Open(szFileName, CFile::modeCreate | CFile::modeWrite))
		return 0;

	//Identification
	char szVersion[12] = "";
	strcpy(szVersion, _SZCURVERSION);
	cfgFile.Write(szVersion, sizeof(szVersion));

	cfgFile.Write(m_szPlayerName, sizeof(m_szPlayerName));	// ID chu file = ten nhan vat

	if (!m_prBasic.WriteToFile(cfgFile))
		return FALSE;
	if (!m_prRecover.WriteToFile(cfgFile))
		return FALSE;
	if (!m_prAttack.WriteToFile(cfgFile))
		return FALSE;
	if (!m_prCollect.WriteToFile(cfgFile))
		return FALSE;
	if (!m_prMove.WriteToFile(cfgFile))
		return FALSE;
	if (!m_prParty.WriteToFile(cfgFile))
		return FALSE;
	if (!m_prMap.WriteToFile(cfgFile))
		return FALSE;
	if (!m_prYeSou.WriteToFile(cfgFile))
		return FALSE;

	CFG_MONEY_BLOCK money = { CFG_MONEY_TAG, m_bStatReady ? m_nMoney : m_nSavedMoney };
	cfgFile.Write(&money, sizeof(money));
	m_nSavedMoney = money.nMoney;

	cfgFile.Close();

	return TRUE;
}

// Thong ke: tien (tren nguoi + ruong), so lan chet. Goi moi lan Refresh khi dang trong game
void GameWindow::UpdateStats(DWinProcess& winProcess, int nDoing)
{
	DWORD dwNow = GetTickCount();
	if (!m_dwLoginTick)
		m_dwLoginTick = dwNow | 1;

	LPBYTE lpPlayer = NULL;
	int Rooms[2 * INVENTORY_SIZE / sizeof(int)];	// room_equipment, room_repository
	if (winProcess.ReadMemory((LPCVOID)PLR_BASE_ADD, &lpPlayer, sizeof(lpPlayer)) && lpPlayer
		&& winProcess.ReadMemory(lpPlayer + PLAYER_OFFSET + PLR_ROOM_OFFSET, Rooms, sizeof(Rooms)))
	{
		m_nEquipMoney = Rooms[1];
		m_nMoney = (__int64)Rooms[1] + Rooms[INVENTORY_SIZE / sizeof(int) + 1];
	}

	// Lenh gom tien: tien da giam => may chu da nhan, cong vao m_nTongMoney. Qua TONG_MONEY_TIMEOUT ma chua giam => that bai
	if (m_nTongPending)
	{
		if (m_nMoney <= m_nTongBase - m_nTongPending)
		{
			m_nTongMoney += m_nTongPending;
			m_nTongPending = 0;
		}
		else if (dwNow - m_dwTongTick >= TONG_MONEY_TIMEOUT)
			m_nTongPending = 0;
	}

	// Vua vao game tien chua dong bo xong => cho STAT_START_DELAY moi lay moc
	if (!m_bStatReady)
	{
		if (dwNow - m_dwLoginTick < STAT_START_DELAY)
			return;
		m_bStatReady = TRUE;
		m_nStartMoney = m_nMoney;
	}

	BOOL bDead = nDoing == do_death || nDoing == do_revive;
	if (bDead && !m_bDead)
		m_nDeathCount++;
	m_bDead = bDead;

	if (m_nMoney != m_nSavedMoney && (!m_dwMoneySaveTick || dwNow - m_dwMoneySaveTick >= MONEY_SAVE_INTERVAL))
	{
		m_dwMoneySaveTick = dwNow | 1;
		SaveMoney();
	}
}

// Gom tien len quy bang (tien tren nguoi + ruong), giu lai nKeep. Phan tien tren nguoi khong du: VLHook rut tu ruong truoc khi gop.
// Tra ve so tien da gui lenh gom, 0 = khong gom (chua vao game / dang cho lenh truoc / khong du tien), -1 = loi gui lenh
int GameWindow::StoreTongMoney(__int64 nKeep)
{
	if (!m_hWnd || !m_bAttached || !m_bStatReady || m_nTongPending)
		return 0;
	__int64 nMoney = min(m_nMoney - nKeep, (__int64)INT_MAX);
	if (nMoney <= 0)
		return 0;
	if (!::PostMessage(m_hWnd, WM_HOOK_MAIN, cmd_tongmoney, (LPARAM)nMoney))
		return -1;
	m_nTongPending = nMoney;
	m_nTongBase = m_nMoney;
	m_dwTongTick = GetTickCount();
	return (int)nMoney;
}

// Ghi tien vao cuoi file cfg cua nhan vat (khong ghi lai cac tab)
void GameWindow::SaveMoney()
{
	TCHAR szFileName[MAX_PATH];
	if (!m_bStatReady || !GetProfileFileName(szFileName)) return;

	CFile cfgFile;
	if (!cfgFile.Open(szFileName, CFile::modeReadWrite | CFile::shareDenyWrite))
	{
		SaveProfile();	// Chua co file cfg
		return;
	}
	CFG_MONEY_BLOCK money = { 0 };
	ULONGLONG nLength = cfgFile.GetLength();
	if (nLength >= sizeof(money))
	{
		cfgFile.Seek(nLength - sizeof(money), CFile::begin);
		if (cfgFile.Read(&money, sizeof(money)) != sizeof(money) || money.dwTag != CFG_MONEY_TAG)
			cfgFile.Seek(0, CFile::end);	// File cu chua co khoi tien => them vao cuoi
		else
			cfgFile.Seek(nLength - sizeof(money), CFile::begin);
	}
	money.dwTag = CFG_MONEY_TAG;
	money.nMoney = m_nMoney;
	cfgFile.Write(&money, sizeof(money));
	cfgFile.Close();
	m_nSavedMoney = m_nMoney;
}

// Doc tien da luu trong file cfg (nhan vat dang offline)
BOOL GameWindow::ReadSavedMoney(LPCTSTR szFileName, __int64* pMoney)
{
	CFile cfgFile;
	if (!cfgFile.Open(szFileName, CFile::modeRead | CFile::shareDenyNone))
		return FALSE;
	CFG_MONEY_BLOCK money = { 0 };
	ULONGLONG nLength = cfgFile.GetLength();
	BOOL bOK = FALSE;
	if (nLength >= sizeof(money))
	{
		cfgFile.Seek(nLength - sizeof(money), CFile::begin);
		bOK = cfgFile.Read(&money, sizeof(money)) == sizeof(money) && money.dwTag == CFG_MONEY_TAG;
	}
	cfgFile.Close();
	if (bOK)
		*pMoney = money.nMoney;
	return bOK;
}

BOOL GameWindow::IsRegistered()
{
	int nLevel = m_nPlayerLevel;
	__asm { add nLevel, 30 }
	if (nLevel < 50) return TRUE;

	char szCharName[33] = "c";
	strcat(szCharName, m_szPlayerName);
	unsigned int uCharacterID = HashString2ID(szCharName);
	memset(szCharName, 0, sizeof(szCharName));

	JUNK_CODE_ONE;
	return TRUE;

	for (int i = 0; i < MAX_LIC_COUNT; i++)
	{
		if (theGameData.RegCode[i] == g_HardDiskID)
			return TRUE;
		if (theGameData.RegCode[i] == uCharacterID)
			return TRUE;
	}

	return FALSE;
}

void GameWindow::SetGameParams(int nTabIdx)
{
	LPCVOID wpOffsets[] = { &m_prBasic, &m_prRecover, &m_prAttack, &m_prCollect,
		&m_prMove, &m_prParty, &m_prMap, &m_prYeSou };

	if (nTabIdx < 0)
	{
		for (int i = tab_basic; i <= tab_yesou; i++)
			::PostMessage(m_hWnd, AutoParamMsg[i].uMsg, (WPARAM)wpOffsets[i], AutoParamMsg[i].lParam);
	}
	else if (nTabIdx <= tab_yesou)
		::PostMessage(m_hWnd, AutoParamMsg[nTabIdx].uMsg, (WPARAM)wpOffsets[nTabIdx], AutoParamMsg[nTabIdx].lParam);
}

// GameWindowsSet Class
GameWindow* GameWindowsSet::FindGameWindow(HWND hGameWnd)
{
	for (int i = 0; i < g_pCharList->GetItemCount(); i++)
	{
		GameWindow* pGameWin = (GameWindow*)g_pCharList->GetItemData(i);
		if (pGameWin->m_hWnd == hGameWnd)
			return pGameWin;
	}

	return NULL;
}

void GameWindowsSet::RefreshWindows(BOOL bRefreshAll)
{
	if (bRefreshAll)
	{
		HWND hWnd = NULL;

		while (TRUE)
		{
			hWnd = FindWindowEx(NULL, hWnd, VLTK_WINCLASS, NULL);
			if (hWnd == NULL)
				break;

			TCHAR szGameY[MAX_PATH];
			DWinProcess winProcess(hWnd);
			winProcess.GetModuleBaseName(NULL, szGameY, MAX_PATH);

			_tcslwr(szGameY);
			//if (!_tcsstr(szGameY, _TGameY_Exe)) continue;
			if (!_tcsstr(szGameY, _TGameY_Exe1)
				&& !_tcsstr(szGameY, _TGameY_Exe2)				
				)
				continue;
			if (FindGameWindow(hWnd) == NULL)
			{
				GameWindow* pGameWin = new GameWindow(hWnd);
				g_pCharList->InsertItem(0, _T(""));
				g_pCharList->SetItemData(0, (DWORD_PTR)pGameWin);
				g_pCharList->SetItemState(0, LVIS_SELECTED, LVIS_SELECTED);
			}
		}

		return;
	}

	for (int i = 0; i < g_pCharList->GetItemCount(); i++)
	{
		GameWindow* pGameWin = (GameWindow*)g_pCharList->GetItemData(i);
		pGameWin->Refresh();
	}
}
