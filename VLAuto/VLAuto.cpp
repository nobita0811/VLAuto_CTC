// VLAuto.cpp : Defines the class behaviors for the application.
//

#include "stdafx.h"
#include "../GameDef.h"
#include "../VLHook/VLHook.h"
#include "VLAuto.h"
#include "GameWindow.h"
#include "DlgLogin.h"
#include "VLAutoDlg.h"
#include "../CrashHandler.h"

// CVLAutoApp

BEGIN_MESSAGE_MAP(CVLAutoApp, CWinApp)
	ON_COMMAND(ID_HELP, &CWinApp::OnHelp)
END_MESSAGE_MAP()

// CVLAutoApp construction

CVLAutoApp::CVLAutoApp()
{
	EnableHtmlHelp();
	g_HardDiskID = 0;// StartInject(0, 0);
}

CWnd* CVLAutoApp::GetMainDlg()
{
	return m_pMainDlg;
}

// Global Variables
CVLAutoApp		theApp;
GameData		theGameData;
CListCtrl*		g_pCharList = NULL;
GameWindow*		g_pCurGameWin = NULL;
TCHAR			g_szAppFolder[MAX_PATH];
TCHAR			g_szIniFile[MAX_PATH];
UINT			g_HardDiskID;

BOOL CVLAutoApp::InitApplication()
{
	CWinApp::InitApplication();
	WNDCLASS wc;
	memset(&wc, 0, sizeof(WNDCLASS));
	GetClassInfo(AfxGetInstanceHandle(), _T("#32770"), &wc);
	wc.lpszClassName = _TVLAWinClass;
	return RegisterClass(&wc);
}

BOOL CVLAutoApp::InitInstance()
{
	CrashInstall("VLAuto", TRUE);	// Log crash vao <Auto>\Crash

	if (!FirstInstance())
		return FALSE;

	INITCOMMONCONTROLSEX InitCtrls;
	InitCtrls.dwSize = sizeof(InitCtrls);
	InitCtrls.dwICC = ICC_WIN95_CLASSES;
	InitCommonControlsEx(&InitCtrls);

	CWinApp::InitInstance();
	AfxEnableControlContainer();

	if (!AfxSocketInit())
	{
		AfxMessageBox(IDP_SOCKETS_INIT_FAILED);
		return FALSE;
	}

	GetModuleFileName(NULL, g_szAppFolder, MAX_PATH);
	TCHAR *pStr = _tcsrchr(g_szAppFolder, _T('\\'));
	pStr[0] = 0;
	TCHAR szUiCfgFolder[MAX_PATH];
	TCHAR szLogFolder[MAX_PATH];
	_stprintf(g_szIniFile, _TVLAIniFile, g_szAppFolder);
	_stprintf(szUiCfgFolder, _T("%s\\UiConfig"), g_szAppFolder);
	_stprintf(szLogFolder, _T("%s\\Logs"), g_szAppFolder);
	CreateDirectory(szUiCfgFolder, NULL);
	_stprintf(szUiCfgFolder, _T("%s\\UserConfig"), g_szAppFolder);	// Config rieng tung nhan vat
	CreateDirectory(szUiCfgFolder, NULL);
	CreateDirectory(szLogFolder, NULL);

	/*INT_PTR nExitCode = IDOK;
	BOOL bSigningOut = 0;

	while (nExitCode == IDOK)
	{
		CLoginDialog *pLoginDlg = new CLoginDialog();
		pLoginDlg->ProcessSignOut(bSigningOut);
		nExitCode = pLoginDlg->DoModal();
		delete pLoginDlg;
		if (nExitCode == IDOK)
		{
			CVLAutoDlg *pMainDlg = new CVLAutoDlg();
			m_pMainDlg = pMainDlg;
			nExitCode = pMainDlg->DoModal();
			delete pMainDlg;
			bSigningOut = TRUE;
		}
	}*/

	CVLAutoDlg *pMainDlg = new CVLAutoDlg();
	m_pMainDlg = pMainDlg;
	pMainDlg->DoModal();
	delete pMainDlg;

	return FALSE;
}

BOOL CVLAutoApp::FirstInstance()
{
	CWnd *PrevCWnd = CWnd::FindWindow(_TVLAWinClass, NULL);
	if (PrevCWnd == NULL) PrevCWnd = CWnd::FindWindow(NULL, _TLgnDlgTitle);
	if (PrevCWnd != NULL)
	{
		PrevCWnd->BringWindowToTop();
		if (PrevCWnd->IsIconic() || !PrevCWnd->IsWindowVisible())
		{
			PrevCWnd->ShowWindow(SW_RESTORE);
			PrevCWnd->SetForegroundWindow();
			PrevCWnd->PostMessage(WM_FEEDBACK, cmd_showme, 0);
		}
		return FALSE;
	}
	return TRUE;
}
