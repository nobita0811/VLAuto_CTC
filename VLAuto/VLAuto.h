// VLAuto.h : main header file for the PROJECT_NAME application
//

//http://go.microsoft.com/fwlink/?LinkID=208935 = KB2465361

#pragma once

#ifndef __AFXWIN_H__
	#error "include 'stdafx.h' before including this file for PCH"
#endif

#include "Resource.h"		// main symbols
#include "Version.h"

#define DEVIO_CODE_SIZE		16
#define _SZOLDVERSION		"VLAUTO_0500"
#define _SZCURVERSION		"VLAUTO_0600"

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
#define WM_FEEDBACK			WM_USER + 200

enum TAB_INDEXES
{
	tab_basic = 0,
	tab_recover,
	tab_attack,
	tab_collect,
	tab_move,
	tab_party,
	tab_map,
	tab_yesou,
	tab_about,
	tab_num,
};

#define MAX_LIC_COUNT 200
struct GameData
{
	int NpcBaseAdd;
	int PlrBaseAdd;
	int MapAddress;
	int AccountID;
	int KCoreShell;
	int ExeFileSize;
	char RegData[MAX_LIC_COUNT][33];
	int RegCode[MAX_LIC_COUNT];
	int nAddDif;
};

class GameWindow;

// Class CVLAutoApp
class CVLAutoApp : public CWinApp
{
public:
	CVLAutoApp();
	CWnd* GetMainDlg();

private:
	CWnd* m_pMainDlg;

// Overrides
public:
	virtual BOOL InitApplication();
	virtual BOOL InitInstance();
	BOOL FirstInstance();

// Implementation

	DECLARE_MESSAGE_MAP()
};

// Global Functions;
void TCVN3ToUnicode(LPCSTR szTCVN3String, LPWSTR szUnicodeString, int nMaxLen);
void UnicodeToTCVN3(LPCWSTR szUnicodeString, LPSTR szTCVN3String, int nMaxLen);
unsigned int HashString2ID(const char* szFileName);

INT_PTR GetDeviceIoControlAdd();
void GetHardDriveComputerID(char*);
BOOL MyDeviceIoControl(HANDLE hDevice, DWORD dwIoControlCode, LPVOID lpInBuffer, DWORD nInBufferSize, 
	LPVOID lpOutBuffer, DWORD nOutBufferSize, LPDWORD lpBytesReturned, LPOVERLAPPED lpOverlapped);

// Global Variables
extern CVLAutoApp		theApp;
extern GameData			theGameData;
extern CListCtrl*		g_pCharList;
extern GameWindow*		g_pCurGameWin;
extern TCHAR			g_szAppFolder[MAX_PATH];
extern TCHAR			g_szIniFile[MAX_PATH];
extern UINT				g_HardDiskID;
