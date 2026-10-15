// Recover.cpp : implementation file
//

#include "stdafx.h"
#include "VLAuto.h"
#include "GameWindow.h"
#include "DlgRecover.h"

// CDlgRecover dialog

IMPLEMENT_DYNAMIC(CDlgRecover, CDialog)

CDlgRecover::CDlgRecover(CWnd* pParent /*=NULL*/)
	: CDialog(CDlgRecover::IDD, pParent)
{
}

CDlgRecover::~CDlgRecover()
{
}

void CDlgRecover::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);

	if (g_pCurGameWin == NULL) return;

	//Checkboxes
	DDX_Check(pDX, IDC_AUTOHP_CHECK, g_pCurGameWin->m_prRecover.m_bAutoHP);
	DDX_Check(pDX, IDC_AUTOMP_CHECK, g_pCurGameWin->m_prRecover.m_bAutoMP);
	DDX_Check(pDX, IDC_AMBYHP_CHECK, g_pCurGameWin->m_prRecover.m_bAMByHP);
	DDX_Check(pDX, IDC_AMBYMP_CHECK, g_pCurGameWin->m_prRecover.m_bAMByMP);
	DDX_Check(pDX, IDC_AMBYFULLROOM_CHECK, g_pCurGameWin->m_prRecover.m_bAMByFullRoom);
	DDX_Check(pDX, IDC_AMBYMON_CHECK, g_pCurGameWin->m_prRecover.m_bAMByMoney);
	DDX_Check(pDX, IDC_AMBYDUR_CHECK, g_pCurGameWin->m_prRecover.m_bAMByDur);
	DDX_Check(pDX, IDC_AMBYBLOOD_CHECK, g_pCurGameWin->m_prRecover.m_bAMByBlood);
	DDX_Check(pDX, IDC_AMBYMANA_CHECK, g_pCurGameWin->m_prRecover.m_bAMByMana);
	DDX_Check(pDX, IDC_USEPOISON_CHECK, g_pCurGameWin->m_prRecover.m_bUsePoison);
	DDX_Check(pDX, IDC_USEDBLEXP_CHECK, g_pCurGameWin->m_prRecover.m_bUseDoubleExp);
	DDX_Check(pDX, IDC_USEMASTER_CHECK, g_pCurGameWin->m_prRecover.m_bUseMaster);
	DDX_Check(pDX, IDC_NGAMI_CHECK, g_pCurGameWin->m_prRecover.m_bIsNgaMiBuff);
	DDX_Check(pDX, IDC_PTBUFF_CHECK, g_pCurGameWin->m_prRecover.m_bPartyBuff);
	DDX_Check(pDX, IDC_AUTOREPAIR_CHECK, g_pCurGameWin->m_prRecover.m_bAutoRepair);

	//Editboxes
	DDX_Text(pDX, IDC_AUTOHP_EDIT, g_pCurGameWin->m_prRecover.m_nAutoHPAmount);
	DDX_Text(pDX, IDC_HPTIME_EDIT, g_pCurGameWin->m_prRecover.m_nAutoHPTimer);
	DDX_Text(pDX, IDC_AUTOMP_EDIT, g_pCurGameWin->m_prRecover.m_nAutoMPAmount);
	DDX_Text(pDX, IDC_MPTIME_EDIT, g_pCurGameWin->m_prRecover.m_nAutoMPTimer);
	DDX_Text(pDX, IDC_AMBYHP_EDIT, g_pCurGameWin->m_prRecover.m_nAMByHPAmount);
	DDX_Text(pDX, IDC_AMBYMP_EDIT, g_pCurGameWin->m_prRecover.m_nAMByMPAmount);
	DDX_Text(pDX, IDC_AMBYMON_EDIT, g_pCurGameWin->m_prRecover.m_nAMByMoneyAmount);
	DDX_Text(pDX, IDC_AMBYDUR_EDIT, g_pCurGameWin->m_prRecover.m_nAMByDurAmount);
	DDX_Text(pDX, IDC_NGAMI_EDIT, g_pCurGameWin->m_prRecover.m_nNgaMiBuffPercent);

	//Comboboxes
	DDX_CBIndex(pDX, IDC_FULLROOM_COMBO, g_pCurGameWin->m_prRecover.m_nAMByFullRoomVal);

	//UI Refresh
	BOOL bChecked;
	bChecked = ((CButton*)GetDlgItem(IDC_AUTOHP_CHECK))->GetCheck();
	GetDlgItem(IDC_AUTOHP_EDIT)->EnableWindow(!bChecked);
	GetDlgItem(IDC_HPTIME_EDIT)->EnableWindow(!bChecked);
	bChecked = ((CButton*)GetDlgItem(IDC_AUTOMP_CHECK))->GetCheck();
	GetDlgItem(IDC_AUTOMP_EDIT)->EnableWindow(!bChecked);
	GetDlgItem(IDC_MPTIME_EDIT)->EnableWindow(!bChecked);
	bChecked = ((CButton*)GetDlgItem(IDC_AMBYHP_CHECK))->GetCheck();
	GetDlgItem(IDC_AMBYHP_EDIT)->EnableWindow(!bChecked);
	bChecked = ((CButton*)GetDlgItem(IDC_AMBYMP_CHECK))->GetCheck();
	GetDlgItem(IDC_AMBYMP_EDIT)->EnableWindow(!bChecked);
	bChecked = ((CButton*)GetDlgItem(IDC_AMBYFULLROOM_CHECK))->GetCheck();
	GetDlgItem(IDC_FULLROOM_COMBO)->EnableWindow(!bChecked);
	bChecked = ((CButton*)GetDlgItem(IDC_AMBYMON_CHECK))->GetCheck();
	GetDlgItem(IDC_AMBYMON_EDIT)->EnableWindow(!bChecked);
	bChecked = ((CButton*)GetDlgItem(IDC_AMBYDUR_CHECK))->GetCheck();
	GetDlgItem(IDC_AMBYDUR_EDIT)->EnableWindow(!bChecked);
	bChecked = ((CButton*)GetDlgItem(IDC_NGAMI_CHECK))->GetCheck();
	GetDlgItem(IDC_NGAMI_EDIT)->EnableWindow(!bChecked);
	GetDlgItem(IDC_PTBUFF_CHECK)->EnableWindow(!bChecked);
}

BEGIN_MESSAGE_MAP(CDlgRecover, CDialog)
	ON_CONTROL_RANGE(BN_CLICKED, 1000, 2000, &CDlgRecover::OnButtonsClicked)
END_MESSAGE_MAP()


// CDlgRecover message handlers

BOOL CDlgRecover::OnInitDialog()
{
	CDialog::OnInitDialog();
	CComboBox* pItemCollectCombo = ((CComboBox*)GetDlgItem(IDC_FULLROOM_COMBO));

	pItemCollectCombo->AddString(_T("Hết chỗ đồ 2 ô"));
	pItemCollectCombo->AddString(_T("Hết chỗ đồ 3 ô"));
	pItemCollectCombo->AddString(_T("Hết chỗ đồ 4 ô"));
	pItemCollectCombo->AddString(_T("Hết chỗ đồ 6 ô"));

	if (m_ToolTip.Create(this))
	{
		m_ToolTip.AddTool(GetDlgItem(IDC_AUTOHP_CHECK), _T("Tự động bơm Sinh lực (Máu)"));
		m_ToolTip.AddTool(GetDlgItem(IDC_AUTOHP_EDIT), _T("Mức Sinh lực cần bơm"));
		m_ToolTip.AddTool(GetDlgItem(IDC_HPTIME_EDIT), _T("Số giây giãn cách giữa 2 lần bơm"));
		m_ToolTip.AddTool(GetDlgItem(IDC_AUTOMP_CHECK), _T("Tự động bơm Nội lực (Mana)"));
		m_ToolTip.AddTool(GetDlgItem(IDC_AUTOMP_EDIT), _T("Mức Nội lực cần bơm"));
		m_ToolTip.AddTool(GetDlgItem(IDC_MPTIME_EDIT), _T("Số giây giãn cách giữa 2 lần bơm"));
		m_ToolTip.AddTool(GetDlgItem(IDC_AMBYHP_CHECK), _T("Hồi thành khi Sinh lực thấp"));
		m_ToolTip.AddTool(GetDlgItem(IDC_AMBYHP_EDIT), _T("Mức Sinh lực hồi thành"));
		m_ToolTip.AddTool(GetDlgItem(IDC_AMBYMP_CHECK), _T("Hồi thành khi Nội lực thấp"));
		m_ToolTip.AddTool(GetDlgItem(IDC_AMBYMP_EDIT), _T("Mức Nội lực hồi thành"));
		m_ToolTip.AddTool(GetDlgItem(IDC_AMBYBLOOD_CHECK), _T("Hồi thành khi hết bình Máu đem theo"));
		m_ToolTip.AddTool(GetDlgItem(IDC_AMBYMANA_CHECK), _T("Hồi thành khi hết bình Mana đem theo"));
		m_ToolTip.AddTool(GetDlgItem(IDC_AMBYFULLROOM_CHECK), _T("Hồi thành khi hành trang quá đầy"));
		m_ToolTip.AddTool(GetDlgItem(IDC_FULLROOM_COMBO), _T("Kích thước vật phẩm hết chỗ chứa"));
		m_ToolTip.AddTool(GetDlgItem(IDC_AMBYMON_CHECK), _T("Hồi thành khi tiền quá nhiều"));
		m_ToolTip.AddTool(GetDlgItem(IDC_AMBYMON_EDIT), _T("Số tiền ngưỡng hồi thành (vạn lượng)"));
		m_ToolTip.AddTool(GetDlgItem(IDC_AMBYDUR_CHECK), _T("Hồi thành khi hư (hỏng) đồ"));
		m_ToolTip.AddTool(GetDlgItem(IDC_AMBYDUR_EDIT), _T("Độ bền trang bị hồi thành"));
		m_ToolTip.AddTool(GetDlgItem(IDC_USEDBLEXP_CHECK), _T("Tự dùng TTL (và các vật phẩm x2 KN)"));
		m_ToolTip.AddTool(GetDlgItem(IDC_TTL_BUTTON), _T("Định nghĩa Danh sách vật phẩm x2 KN"));
		m_ToolTip.AddTool(GetDlgItem(IDC_NGAMI_CHECK), _T("Nga Mi buff Từ Hàng Phổ Độ"));
		m_ToolTip.AddTool(GetDlgItem(IDC_NGAMI_EDIT), _T("Buff khi máu còn xx %"));
		m_ToolTip.AddTool(GetDlgItem(IDC_AUTOREPAIR_CHECK), _T("Tự sửa đồ trên bãi luyện công (khi độ bền còn 80%)"));
	}

	return FALSE;
}

BOOL CDlgRecover::PreTranslateMessage(MSG* pMsg)
{
	m_ToolTip.RelayEvent(pMsg);
	return CDialog::PreTranslateMessage(pMsg);
}

void CDlgRecover::OnButtonsClicked(UINT nID)
{
	if (nID == IDC_TTL_BUTTON)
	{
		TCHAR szFileName[MAX_PATH];
		_stprintf(szFileName, _T("%s\\Maps\\X2Items.txt"), g_szAppFolder);
		::ShellExecute(NULL, _T("Open"), szFileName, 0, 0, SW_SHOW);
		return;
	}

	if (g_pCurGameWin == NULL) return;
	int nValue = 0;
	switch (nID)
	{
	case IDC_AUTOHP_CHECK:
		nValue = GetDlgItemInt(IDC_HPTIME_EDIT);
		if (nValue < 1) SetDlgItemInt(IDC_HPTIME_EDIT, 1);
		break;
	case IDC_AUTOMP_CHECK:
		nValue = GetDlgItemInt(IDC_MPTIME_EDIT);
		if (nValue < 1) SetDlgItemInt(IDC_MPTIME_EDIT, 1);
		break;
	case IDC_NGAMI_CHECK:
		nValue = GetDlgItemInt(IDC_NGAMI_EDIT);
		if (nValue < 50) SetDlgItemInt(IDC_NGAMI_EDIT, 50);
		if (nValue > 90) SetDlgItemInt(IDC_NGAMI_EDIT, 90);
		break;
	}
	UpdateData();
	g_pCurGameWin->SetGameParams(tab_recover);
}
