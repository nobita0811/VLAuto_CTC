// DlgBasic.cpp : implementation file
//

#include "stdafx.h"
#include "../GameDef.h"
#include "VLAuto.h"
#include "GameCore.h"
#include "GameWindow.h"
#include "DlgBasic.h"

TCHAR* szSeriesName[] = { _T("Kim"), _T("Mộc"),  _T("Thủy"),  _T("Hỏa"), _T("Thổ"), _T("") };

// CDlgBasic dialog

IMPLEMENT_DYNAMIC(CDlgBasic, CDialog)

CDlgBasic::CDlgBasic(CWnd* pParent /*=NULL*/)
	: CDialog(CDlgBasic::IDD, pParent)
{
}

CDlgBasic::~CDlgBasic()
{
}

void CDlgBasic::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_LIFE_PROGRESS, m_LifeIndicator);
	DDX_Control(pDX, IDC_MANA_PROGRESS, m_ManaIndicator);

	if (g_pCurGameWin == NULL) return;

	DDX_Check(pDX, IDC_AMEXIT_CHECK, g_pCurGameWin->m_prBasic.m_bExitWhenAM);
	DDX_Check(pDX, IDC_AUTO_START_CHECK, g_pCurGameWin->m_prBasic.m_bAutoStart);
	DDX_Text(pDX, IDC_TIME_AUTO_START_EDIT, g_pCurGameWin->m_prBasic.m_nAutoStartTime);
	DDV_MinMaxInt(pDX, g_pCurGameWin->m_prBasic.m_nAutoStartTime, 0, 3600000);
	DDX_Check(pDX, IDC_EXP_LIMIT_CHECK, g_pCurGameWin->m_prBasic.m_bExpLimit);
	DDX_Text(pDX, IDC_EXP_MAX_EDIT, g_pCurGameWin->m_prBasic.m_nExpMax);
	DDV_MinMaxInt(pDX, g_pCurGameWin->m_prBasic.m_nExpMax, 1, 100);
	DDX_Text(pDX, IDC_EXP_MIN_EDIT, g_pCurGameWin->m_prBasic.m_nExpMin);
	DDV_MinMaxInt(pDX, g_pCurGameWin->m_prBasic.m_nExpMin, 0, 99);
	DDX_Text(pDX, IDC_EXP_LEVEL_EDIT, g_pCurGameWin->m_prBasic.m_nExpLevel);
	DDV_MinMaxInt(pDX, g_pCurGameWin->m_prBasic.m_nExpLevel, 0, 200);
	DDX_Check(pDX, IDC_TRIM_RAM_CHECK, g_pCurGameWin->m_prBasic.m_bTrimRam);
	DDX_Text(pDX, IDC_TRIM_RAM_EDIT, g_pCurGameWin->m_prBasic.m_nTrimRamTime);
	DDV_MinMaxInt(pDX, g_pCurGameWin->m_prBasic.m_nTrimRamTime, 10, 86400);

	//UI Refresh
	BOOL bChecked = ((CButton*)GetDlgItem(IDC_AUTO_START_CHECK))->GetCheck();
	GetDlgItem(IDC_TIME_AUTO_START_EDIT)->EnableWindow(!bChecked);
	bChecked = ((CButton*)GetDlgItem(IDC_EXP_LIMIT_CHECK))->GetCheck();
	GetDlgItem(IDC_EXP_MAX_EDIT)->EnableWindow(!bChecked);
	GetDlgItem(IDC_EXP_MIN_EDIT)->EnableWindow(!bChecked);
	GetDlgItem(IDC_EXP_LEVEL_EDIT)->EnableWindow(!bChecked);
	bChecked = ((CButton*)GetDlgItem(IDC_TRIM_RAM_CHECK))->GetCheck();
	GetDlgItem(IDC_TRIM_RAM_EDIT)->EnableWindow(!bChecked);
}

BEGIN_MESSAGE_MAP(CDlgBasic, CDialog)
	ON_BN_CLICKED(IDC_HIDEWIN_CHECK, &CDlgBasic::OnBnClickedHidewinCheck)
	ON_BN_CLICKED(IDC_AMEXIT_CHECK, &CDlgBasic::OnBnClickedAmexitCheck)
	ON_BN_CLICKED(IDC_AUTO_START_CHECK, &CDlgBasic::OnBnClickedAutoStartCheck)
	ON_BN_CLICKED(IDC_EXP_LIMIT_CHECK, &CDlgBasic::OnBnClickedExpLimitCheck)
	ON_BN_CLICKED(IDC_TRIM_RAM_CHECK, &CDlgBasic::OnBnClickedTrimRamCheck)
END_MESSAGE_MAP()

BOOL CDlgBasic::OnInitDialog()
{
	CDialog::OnInitDialog();
	m_LifeIndicator.SetProgressType(0);
	m_ManaIndicator.SetProgressType(1);

	if (m_ToolTip.Create(this))
	{
		m_ToolTip.AddTool(GetDlgItem(IDC_ENAME_EDIT), _T("Tên và Hệ đối thủ hoặc người dẫn"));
		m_ToolTip.AddTool(GetDlgItem(IDC_ESERIES_EDIT), _T("Tên và Hệ đối thủ hoặc người dẫn"));
		m_ToolTip.AddTool(GetDlgItem(IDC_ELIFE_EDIT), _T("Sinh lực và Đẳng cấp đối thủ hoặc người dẫn"));
		m_ToolTip.AddTool(GetDlgItem(IDC_ELEVEL_EDIT), _T("Sinh lực và Đẳng cấp đối thủ hoặc người dẫn"));
		m_ToolTip.AddTool(GetDlgItem(IDC_HIDEWIN_CHECK), _T("Hiện/Ẩn cửa sổ Game"));
		m_ToolTip.AddTool(GetDlgItem(IDC_AMEXIT_CHECK), _T("Thoát Game sau khi Thổ Địa Phù"));
		m_ToolTip.AddTool(GetDlgItem(IDC_AUTO_START_CHECK), _T("Tự check chạy Auto khi nhân vật vào game"));
		m_ToolTip.AddTool(GetDlgItem(IDC_TIME_AUTO_START_EDIT), _T("Số mili giây chờ trước khi tự check Auto (1000 = 1 giây)"));
		m_ToolTip.AddTool(GetDlgItem(IDC_EXP_LIMIT_CHECK), _T("Đạt Level giới hạn và kinh nghiệm đạt % này thì về thành, sang Dương Châu đứng cho quái đánh chết đến khi xuống % bên phải"));
		m_ToolTip.AddTool(GetDlgItem(IDC_EXP_LEVEL_EDIT), _T("Đẳng cấp nhân vật phải đạt mức này mới chạy Giới hạn Exp (0 = không giới hạn cấp)"));
		m_ToolTip.AddTool(GetDlgItem(IDC_EXP_MAX_EDIT), _T("% kinh nghiệm bắt đầu giảm (mặc định 20)"));
		m_ToolTip.AddTool(GetDlgItem(IDC_EXP_MIN_EDIT), _T("% kinh nghiệm dừng giảm, chạy lại Auto (mặc định 1)"));
		m_ToolTip.AddTool(GetDlgItem(IDC_TRIM_RAM_CHECK), _T("Giảm RAM game định kỳ (đẩy bộ nhớ ra standby/pagefile), chạy cả khi chưa check Auto"));
		m_ToolTip.AddTool(GetDlgItem(IDC_TRIM_RAM_EDIT), _T("Số giây giữa 2 lần giảm RAM (10 - 86400, mặc định 60)"));
	}

	return FALSE;
}

BOOL CDlgBasic::PreTranslateMessage(MSG* pMsg)
{
	m_ToolTip.RelayEvent(pMsg);
	return CDialog::PreTranslateMessage(pMsg);
}

void CDlgBasic::RefreshUI()
{
	static int nLastMapID = 0;
	TCHAR szOldInfo[40];
	TCHAR szInfo[64];

	if (g_pCurGameWin == NULL) return;

	if (g_pCurGameWin->m_nCurLifeMax == 0)
		g_pCurGameWin->m_nCurLifeMax = 100;
	m_LifeIndicator.SetPos(g_pCurGameWin->m_nCurLife*100/g_pCurGameWin->m_nCurLifeMax);
	m_LifeIndicator.Invalidate();
	SetDlgItemInt(IDC_LIFE_STATIC, g_pCurGameWin->m_nCurLife);

	if (g_pCurGameWin->m_nCurManaMax == 0)
		g_pCurGameWin->m_nCurManaMax = 100;
	m_ManaIndicator.SetPos(g_pCurGameWin->m_nCurMana*100/g_pCurGameWin->m_nCurManaMax);
	m_ManaIndicator.Invalidate();
	SetDlgItemInt(IDC_MANA_STATIC, g_pCurGameWin->m_nCurMana);

	if (g_pCurGameWin->m_nMapID != nLastMapID)
	{
		//Read MapList.ini
		nLastMapID = g_pCurGameWin->m_nMapID;
		TCHAR szMapID[12];
		_stprintf(szMapID, _T("%d"), nLastMapID);
		TCHAR szMapNameFile[MAX_PATH];
		_stprintf(szMapNameFile, _T("%s\\Maps\\MapNames.ini"), g_szAppFolder);
		GetPrivateProfileString(_T("Map_Name"), szMapID, _T("<Unknown_Map>"), szInfo, 64, szMapNameFile);
		char szTCNV3Str[64];
		WideCharToMultiByte(CP_ACP, 0, szInfo, -1, szTCNV3Str, 64, 0, 0);
		TCVN3ToUnicode(szTCNV3Str, szInfo, 63);
		SetDlgItemText(IDC_MAP_EDIT, szInfo);
	}

	_stprintf(szInfo,	_T("%d,%d"), g_pCurGameWin->m_nOffX >> 8, g_pCurGameWin->m_nOffY >> 9);
	SetDlgItemText(IDC_POS_EDIT, szInfo);
	TCVN3ToUnicode(g_pCurGameWin->m_szEnemyName, szInfo, 40);
	GetDlgItemText(IDC_ENAME_EDIT, szOldInfo, 40);
	if (_tcscmp(szOldInfo, szInfo))
	{
		SetDlgItemText(IDC_ENAME_EDIT, szInfo);

		if (g_pCurGameWin->m_nEnemySeries >= 0)
			SetDlgItemText(IDC_ESERIES_EDIT, szSeriesName[g_pCurGameWin->m_nEnemySeries]);
		else
			SetDlgItemText(IDC_ESERIES_EDIT, _T(""));
	}

	_stprintf(szInfo,	_T("%d / %d"), g_pCurGameWin->m_nEnemyLife, g_pCurGameWin->m_nEnemyLifeMax);
	SetDlgItemText(IDC_ELIFE_EDIT, szInfo);
	SetDlgItemInt(IDC_ELEVEL_EDIT, g_pCurGameWin->m_nEnemyLevel);
	((CButton*)GetDlgItem(IDC_HIDEWIN_CHECK))->SetCheck(!(::IsWindowVisible(g_pCurGameWin->m_hWnd)));
}

void CDlgBasic::OnBnClickedHidewinCheck()
{
	if (g_pCurGameWin == NULL) return;
	if (((CButton*)GetDlgItem(IDC_HIDEWIN_CHECK))->GetCheck())
		::ShowWindow(g_pCurGameWin->m_hWnd, SW_HIDE);
	else
		::ShowWindow(g_pCurGameWin->m_hWnd, SW_SHOW);
}

void CDlgBasic::OnBnClickedAmexitCheck()
{
	if (g_pCurGameWin == NULL) return;
	UpdateData();
	g_pCurGameWin->SetGameParams(tab_basic);
}

void CDlgBasic::OnBnClickedAutoStartCheck()
{
	if (g_pCurGameWin == NULL) return;
	if (!UpdateData())
	{
		// Thoi gian khong hop le: bo check, de nguoi dung sua lai
		((CButton*)GetDlgItem(IDC_AUTO_START_CHECK))->SetCheck(BST_UNCHECKED);
		g_pCurGameWin->m_prBasic.m_bAutoStart = FALSE;
		GetDlgItem(IDC_TIME_AUTO_START_EDIT)->EnableWindow(TRUE);
		return;
	}
	g_pCurGameWin->SetGameParams(tab_basic);
	if (g_pCurGameWin->m_bLoadDone)
		g_pCurGameWin->SaveProfile();

	// Vua bat: dem nguoc lai tu dau theo thoi gian moi
	if (g_pCurGameWin->m_prBasic.m_bAutoStart)
	{
		g_pCurGameWin->m_bAutoStartDone = FALSE;
		g_pCurGameWin->m_dwAutoStartTick = 0;
	}
}

void CDlgBasic::OnBnClickedExpLimitCheck()
{
	if (g_pCurGameWin == NULL) return;
	CButton* pCheck = (CButton*)GetDlgItem(IDC_EXP_LIMIT_CHECK);
	BOOL bValid = UpdateData();
	if (bValid && pCheck->GetCheck() && g_pCurGameWin->m_prBasic.m_nExpMax <= g_pCurGameWin->m_prBasic.m_nExpMin)
	{
		AfxMessageBox(_T("% bắt đầu phải lớn hơn % dừng"));
		bValid = FALSE;
	}
	if (!bValid)
	{
		// Gia tri khong hop le: bo check, de nguoi dung sua lai
		pCheck->SetCheck(BST_UNCHECKED);
		g_pCurGameWin->m_prBasic.m_bExpLimit = FALSE;
		GetDlgItem(IDC_EXP_MAX_EDIT)->EnableWindow(TRUE);
		GetDlgItem(IDC_EXP_MIN_EDIT)->EnableWindow(TRUE);
		GetDlgItem(IDC_EXP_LEVEL_EDIT)->EnableWindow(TRUE);
	}
	g_pCurGameWin->SetGameParams(tab_basic);
	if (g_pCurGameWin->m_bLoadDone)
		g_pCurGameWin->SaveProfile();
}

void CDlgBasic::OnBnClickedTrimRamCheck()
{
	if (g_pCurGameWin == NULL) return;
	if (!UpdateData())
	{
		// Thoi gian khong hop le: bo check, de nguoi dung sua lai
		((CButton*)GetDlgItem(IDC_TRIM_RAM_CHECK))->SetCheck(BST_UNCHECKED);
		g_pCurGameWin->m_prBasic.m_bTrimRam = FALSE;
		GetDlgItem(IDC_TRIM_RAM_EDIT)->EnableWindow(TRUE);
	}
	g_pCurGameWin->SetGameParams(tab_basic);
	if (g_pCurGameWin->m_bLoadDone)
		g_pCurGameWin->SaveProfile();
}
