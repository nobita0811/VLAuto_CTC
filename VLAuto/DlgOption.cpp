// DlgOption.cpp : implementation file
//

#include "stdafx.h"
#include "VLAuto.h"
#include "DlgOption.h"

// COptionDlg dialog

IMPLEMENT_DYNAMIC(COptionDlg, CDialog)

COptionDlg::COptionDlg(CWnd* pParent /*=NULL*/)
	: CDialog(COptionDlg::IDD, pParent)
{
	m_bSilentMode = TRUE;
	m_bPrintScreen = FALSE;
	m_bAttackBossFirst = TRUE;
	m_szControlPass = "";
}

COptionDlg::~COptionDlg()
{
}

void COptionDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Check(pDX, IDC_SILENTMODE_CHECK, m_bSilentMode);
	DDX_Check(pDX, IDC_PRINTSCREEN_CHECK, m_bPrintScreen);
	DDX_Check(pDX, IDC_ATTACKBOSSFIRST_CHECK, m_bAttackBossFirst);
	DDX_Text(pDX, IDC_CTRLPASS_EDIT, m_szControlPass);
}

BEGIN_MESSAGE_MAP(COptionDlg, CDialog)
END_MESSAGE_MAP()

// COptionDlg message handlers

BOOL COptionDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	m_bSilentMode = GetPrivateProfileInt(_T("Options"), _T("SilentMode"), 1, g_szIniFile);
	m_bPrintScreen = GetPrivateProfileInt(_T("Options"), _T("PrintScreen"), 0, g_szIniFile);
	m_bAttackBossFirst = GetPrivateProfileInt(_T("Options"), _T("AttackBossFirst"), 1, g_szIniFile);

	TCHAR szCtrlPass[40];
	GetPrivateProfileString(_T("Options"), _T("ControlPass"), _T(""), szCtrlPass, 32, g_szIniFile);
	m_szControlPass = szCtrlPass;

	UpdateData(0);

	return TRUE;  // return TRUE unless you set the focus to a control
}

void COptionDlg::OnOK()
{
	UpdateData();
	TCHAR szValue[16];
	_itot(m_bSilentMode, szValue, 10);
	WritePrivateProfileString(_T("Options"), _T("SilentMode"), szValue, g_szIniFile);
	_itot(m_bPrintScreen, szValue, 10);
	WritePrivateProfileString(_T("Options"), _T("PrintScreen"), szValue, g_szIniFile);
	_itot(m_bAttackBossFirst, szValue, 10);
	WritePrivateProfileString(_T("Options"), _T("AttackBossFirst"), szValue, g_szIniFile);
	WritePrivateProfileString(_T("Options"), _T("ControlPass"), m_szControlPass, g_szIniFile);
	CDialog::OnOK();
}
