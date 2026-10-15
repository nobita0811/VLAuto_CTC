// DlgLogin.cpp : implementation file
//

#include "stdafx.h"
#include "DWinProcess.h"
#include "AntiRE.h"
#include "VLAuto.h"
#include "DlgLogin.h"

#define DEFAULT_PORT 5623
#define SYS_TIMER 1000
#define PRIMARY_SERVER _T("login.vlauto.net")
#define SECONDARY_SERVER _T("loginbk.vlauto.net")
#define MINI_BROWSER_ADDRESS _T("http://vlauto.net/mini/")

unsigned int HashString2ID(const char* szFileName)
{
	unsigned int Id = 0;
	char c = 0;
	for (int i = 0; szFileName[i]; i++)
	{
		c = szFileName[i];
		Id = (Id + (i + 1) * c) % 0x8000000b * 0xffffffef;
	}
	return (Id ^ 0x12345678);
}

// CLoginDialog dialog

IMPLEMENT_DYNAMIC(CLoginDialog, CDialog)

CLoginDialog::CLoginDialog(CWnd* pParent /*=NULL*/)
	: CDialog(CLoginDialog::IDD, pParent)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
	m_pLoginSocket = NULL;
	m_bSigningOut = 0;
	m_nTimeCount = 0;
	m_bUsePrimary = TRUE;
}

CLoginDialog::~CLoginDialog()
{
}

void CLoginDialog::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_MINI_EXPLORER, theIEBrowser);
	DDX_Control(pDX, IDC_LOGIN_BUTTON, m_LoginBtn);
}

BEGIN_MESSAGE_MAP(CLoginDialog, CDialog)
	ON_BN_CLICKED(IDC_LOGIN_BUTTON, &CLoginDialog::OnBnClickedLoginButton)
	ON_WM_TIMER()
END_MESSAGE_MAP()


// CLoginDialog message handlers

BOOL CLoginDialog::OnInitDialog()
{
	CDialog::OnInitDialog();

	SetIcon(m_hIcon, FALSE);

	TCHAR sz_LoginEmail[64];
	BOOL bSaveLogin;
	BOOL bAutoLogin;

	RECT rcMargin = { 10, 0, 0, 0 };
	m_LoginBtn.SetBmpImageList(IDB_LOGIN_BITMAP, 22, &rcMargin);

	GetPrivateProfileString(_T("LoginInfo"), _T("LoginEmail"), _T(""), sz_LoginEmail, 60, g_szIniFile);
	bSaveLogin = (BOOL)GetPrivateProfileInt(_T("LoginInfo"), _T("SaveLogin"), 0, g_szIniFile);
	bAutoLogin = (BOOL)GetPrivateProfileInt(_T("LoginInfo"), _T("AutoLogin"), 0, g_szIniFile);

	((CButton*)GetDlgItem(IDC_SAVELOGIN_CHECK))->SetCheck(bSaveLogin);
	((CButton*)GetDlgItem(IDC_AUTOLOGIN_CHECK))->SetCheck(bAutoLogin);
	SetDlgItemText(IDC_EMAIL_EDIT, sz_LoginEmail);

	if (bAutoLogin && !m_bSigningOut)
		OnBnClickedLoginButton();

	try { theIEBrowser.Navigate(MINI_BROWSER_ADDRESS, 0, 0, 0, 0); }
	catch (...)	{ }

	GetDlgItem(IDC_EMAIL_EDIT)->SetFocus();
	return FALSE;
}

void CLoginDialog::OnOK()
{
	OnBnClickedLoginButton();
	// CDialog::OnOK();
}

void CLoginDialog::OnCancel()
{
	if (m_pLoginSocket)
	{
		delete m_pLoginSocket;
		m_pLoginSocket = NULL;
	}
	CDialog::OnCancel();
	KillTimer(1);
}

void CLoginDialog::OnTimer(UINT_PTR nIDEvent)
{
	CDialog::OnTimer(nIDEvent);
	if (m_nTimeCount) m_nTimeCount--;
	if (m_nTimeCount == 0)
	{
		KillTimer(1);
		ProcessConnect();
	}
	else if (m_nTimeCount <= 7)
	{
		CString szMessage;
		szMessage.Format(_T("Kết nối lại trong %d giây..."), m_nTimeCount);
		SetDlgItemText(IDC_STATUS_STATIC, szMessage);
	}
}

void CLoginDialog::OnBnClickedLoginButton()
{
	CString strEmail;
	GetDlgItemText(IDC_EMAIL_EDIT, strEmail);
	if (!strEmail.IsEmpty())
	{
		m_nTimeCount = 0;
		ProcessConnect();
	}
}

void CLoginDialog::GetEmailAddress(char *szEmailAddress)
{
	if (m_bUsePrimary)
		GetDlgItemTextA(m_hWnd, IDC_EMAIL_EDIT, szEmailAddress, 60);
	else
		strcpy(szEmailAddress, "test@vlauto.net");
}

void CLoginDialog::OnErrorMessage(CString strErrorMsg, BOOL bAutoRetry)
{
	if (bAutoRetry)
	{
		m_nTimeCount = 10;
		SetTimer(1, SYS_TIMER, NULL);
		SetDlgItemText(IDC_STATUS_STATIC, strErrorMsg);
	}
	else
	{
		AfxMessageBox(strErrorMsg);
		SetDlgItemText(IDC_STATUS_STATIC, _T("Xin mời Đăng nhập..."));
	}
	GetDlgItem(IDC_LOGIN_BUTTON)->EnableWindow(TRUE);
	delete m_pLoginSocket;
	m_pLoginSocket = NULL;
}

void CLoginDialog::OnErrorMessage(UINT uMessageID, BOOL bAutoRetry)
{
	CString szMessage;
	if (szMessage.LoadString(uMessageID))
		OnErrorMessage(szMessage, bAutoRetry);
}

void CLoginDialog::ProcessConnect()
{
	CWaitCursor wait;
	GetDlgItem(IDC_LOGIN_BUTTON)->EnableWindow(0);
	SetDlgItemText(IDC_STATUS_STATIC, _T("Đang kết nối đến máy chủ..."));
	BOOL bResult = 0;
	m_pLoginSocket = new CLoginSck();
	if (m_pLoginSocket->Create(this))
	{
		if (m_bUsePrimary)
		{
			bResult = m_pLoginSocket->Connect(PRIMARY_SERVER, DEFAULT_PORT);
			if (!bResult) m_bUsePrimary = FALSE;
		}
		else
		{
			bResult = m_pLoginSocket->Connect(SECONDARY_SERVER, DEFAULT_PORT);
			if (!bResult) m_bUsePrimary = TRUE;
		}
	}
	wait.Restore();

	if (!bResult) OnErrorMessage(IDS_CONNECTION_ERROR, TRUE);
}

void CLoginDialog::PrepareData()
{
	// Encode RegData
	JUNK_CODE_ONE;
	for (int i = 0; i < MAX_LIC_COUNT; i++)
	{
		if (!theGameData.RegData[i][0]) break;
		JUNK_CODE_ONE;
		theGameData.RegCode[i] = HashString2ID(theGameData.RegData[i]);
		JUNK_CODE_ONE;
	}
	memset(theGameData.RegData, 0, sizeof(theGameData.RegData));

	theGameData.nAddDif = 0x200000; // OK for running
	JUNK_CODE_ONE;
	if (!IsPEPacked()) return;

	// Check with ShellWindow
	INT_PTR PtrDevIoCtrlAdd = GetDeviceIoControlAdd();
	JUNK_CODE_ONE;
	BYTE PtrOrgFuncCode[DEVIO_CODE_SIZE];
	memset(PtrOrgFuncCode, 0, DEVIO_CODE_SIZE);
	JUNK_CODE_ONE;

	DWinProcess winProcess(GetShellWindow());
	if (!winProcess.ReadMemory((LPVOID)PtrDevIoCtrlAdd, (LPVOID)&PtrOrgFuncCode, DEVIO_CODE_SIZE))
	{	// Check if IsWin64
		if (IsWow64())
			theGameData.nAddDif = 0x100000;
		return;
	}

	// ReadOK - Check it!
	for (int i = 0; i < DEVIO_CODE_SIZE; i++)
	{
		if (((LPBYTE)PtrOrgFuncCode)[i] == ((LPBYTE)PtrDevIoCtrlAdd)[i]) continue;
		memset(PtrOrgFuncCode, 0, DEVIO_CODE_SIZE);
		return;
	}
	theGameData.nAddDif = 0x100000;
	memset(PtrOrgFuncCode, 0, DEVIO_CODE_SIZE);
}

void CLoginDialog::ProcessLogin(void *pGameData, int nSize)
{
	GetDlgItem(IDC_LOGIN_BUTTON)->EnableWindow(TRUE);
	delete m_pLoginSocket;
	m_pLoginSocket = NULL;

	memset(&theGameData, 0, sizeof(GameData));
	memcpy(&theGameData, pGameData, nSize);
	PrepareData();

	BOOL bSaveLogin = ((CButton*)GetDlgItem(IDC_SAVELOGIN_CHECK))->GetCheck();
	if (bSaveLogin)
	{
		TCHAR szStrValue[64];
		GetDlgItemText(IDC_EMAIL_EDIT, szStrValue, 60);
		WritePrivateProfileString(_T("LoginInfo"), _T("LoginEmail"), szStrValue, g_szIniFile);
		_itot(bSaveLogin, szStrValue, 10);
		WritePrivateProfileString(_T("LoginInfo"), _T("SaveLogin"), szStrValue, g_szIniFile);
		BOOL bAutoLogin = ((CButton*)GetDlgItem(IDC_AUTOLOGIN_CHECK))->GetCheck();
		_itot(bAutoLogin, szStrValue, 10);
		WritePrivateProfileString(_T("LoginInfo"), _T("AutoLogin"), szStrValue, g_szIniFile);
	}
	else
	{
		WritePrivateProfileString(_T("LoginInfo"), _T("LoginEmail"), _T(""), g_szIniFile);
		WritePrivateProfileString(_T("LoginInfo"), _T("SaveLogin"), _T("0"), g_szIniFile);
		WritePrivateProfileString(_T("LoginInfo"), _T("AutoLogin"), _T("0"), g_szIniFile);
	}

	//Return the Main Program
	CDialog::OnOK();
}

void CLoginDialog::ProcessSignOut(BOOL bSigningOut)
{
	m_bSigningOut = bSigningOut;
}

BOOL CLoginDialog::IsWow64()
{
	typedef BOOL (WINAPI *LPFN_ISWOW64PROCESS)(HANDLE, PBOOL);
	LPFN_ISWOW64PROCESS fnIsWow64Process;

	BOOL bIsWow64 = FALSE;

	fnIsWow64Process = (LPFN_ISWOW64PROCESS)GetProcAddress(GetModuleHandle(_T("kernel32")), "IsWow64Process");

	if (fnIsWow64Process != NULL)
		fnIsWow64Process(GetCurrentProcess(),&bIsWow64);

	return bIsWow64;
}

BOOL CLoginDialog::IsPEPacked()
{
#ifndef _DEBUG
	char szFuncName[16] = "SHF5bLvSdfnhg"; //"PEC2_IsPacked"
	int nLen = strlen(szFuncName);
	for (int i = 0; i < nLen; i++)
		szFuncName[i] = szFuncName[i] - 3;

	typedef DWORD (WINAPI *PFNPEC2_IsPacked)();
	PFNPEC2_IsPacked PEC2_IsPacked = (PFNPEC2_IsPacked)GetProcAddress((HMODULE)-1, szFuncName);

	if(PEC2_IsPacked)
		return PEC2_IsPacked();

	return FALSE;
#else
	return TRUE;
#endif
}
