#pragma once

#include "AutoParams.h"

class DWinProcess;

class GameWindow
{
public:
	HWND			m_hWnd;
	BOOL			m_bAttached;
	int				m_nAttachTimeCount;
	DWORD			m_dwServerIP;
	int				m_nPlayerIndex;
	int				m_nPlayerLevel;
	char			m_szPlayerName[32];
	char			m_szAccountID[32];
	__int64			m_nExp;
	__int64			m_nNextExp;
	int				m_nCurLife;
	int				m_nCurLifeMax;
	int				m_nCurMana;
	int				m_nCurManaMax;
	int				m_nMapID;
	int				m_nOffX;
	int				m_nOffY;
	int				m_nFlagX;
	int				m_nFlagY;
	BOOL			m_FightMode;
	char			m_szEnemyName[32];
	int				m_nEnemyLevel;
	int				m_nEnemySeries;
	int				m_nEnemyLife;
	int				m_nEnemyLifeMax;
	int				m_nTimerCount;
	BOOL			m_bLoadDone;
	BOOL			m_bRegistered;
	BOOL			m_bVersionChecked;
	BOOL			m_bInfoChanged;
	BOOL			m_bAutoStartDone;	// Da tu check Auto trong lan vao game nay
	DWORD			m_dwAutoStartTick;	// Moc bat dau dem nguoc tu check Auto

	// Thong ke (Auto Login > Xem Thong Ke)
	__int64			m_nMoney;			// Tien tren nguoi + ruong
	__int64			m_nStartMoney;		// m_nMoney luc bat dau dem (STAT_START_DELAY sau khi vao game)
	__int64			m_nSavedMoney;		// Tien da ghi trong file cfg
	DWORD			m_dwLoginTick;		// Moc vao game lan nay (0 = chua vao game)
	DWORD			m_dwMoneySaveTick;	// Lan ghi tien vao cfg gan nhat
	BOOL			m_bStatReady;		// Da lay m_nStartMoney
	BOOL			m_bDead;
	int				m_nAMUsed;			// So lan dung Tho Dia Phu (VLHook bao cmd_useam)
	int				m_nDeathCount;
	__int64			m_nEquipMoney;		// Tien tren nguoi (khong tinh ruong)
	__int64			m_nTongMoney;		// Da gom len quy bang trong lan vao game nay (cong vao Kiem duoc)
	__int64			m_nTongPending;		// Vua gui lenh gom, cho tien tren nguoi giam moi cong vao m_nTongMoney
	__int64			m_nTongBase;		// m_nMoney luc gui lenh gom
	DWORD			m_dwTongTick;		// Moc gui lenh gom

public:
	PARAM_BASIC		m_prBasic;
	PARAM_RECOVER	m_prRecover;
	PARAM_ATTACK	m_prAttack;
	PARAM_COLLECT	m_prCollect;
	PARAM_MOVE		m_prMove;
	PARAM_PARTY		m_prParty;
	PARAM_MAP		m_prMap;
	PARAM_YESOU		m_prYeSou;

public:
	/*void*/		GameWindow(HWND hWnd);
	void			InitValue();
	void			Refresh();
	void			LoadProfile();
	BOOL			GetProfileFileName(LPTSTR szFileName);
	BOOL			LoadConfigFile(LPCTSTR szFileName);
	void			SaveProfile();
	BOOL			SaveConfigFile(LPCTSTR szFileName);
	BOOL			IsRegistered();
	void			SetGameParams(int nTabIdx = -1);
	void			UpdateStats(DWinProcess& winProcess, int nDoing);
	void			SaveMoney();
	int				StoreTongMoney(__int64 nKeep);
	static BOOL		ReadSavedMoney(LPCTSTR szFileName, __int64* pMoney);
};

class GameWindowsSet
{
public:
	static GameWindow*	FindGameWindow(HWND hGameWnd);
	static void			RefreshWindows(BOOL bRefreshAll = FALSE);
};
