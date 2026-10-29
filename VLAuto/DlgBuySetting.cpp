// DlgBuySetting.cpp : implementation file
//

#include "stdafx.h"
#include "VLAuto.h"
#include "GameWindow.h"
#include "DlgBuySetting.h"

// CDlgBuySetting dialog

IMPLEMENT_DYNAMIC(CDlgBuySetting, CDialog)

CDlgBuySetting::CDlgBuySetting(CWnd* pParent /*=NULL*/)
	: CDialog(CDlgBuySetting::IDD, pParent)
{
}

CDlgBuySetting::~CDlgBuySetting()
{
}

void CDlgBuySetting::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);

	if (g_pCurGameWin == NULL) return;

	//Checkboxes
	DDX_Check(pDX, IDC_BUYITEM_CHECK1, g_pCurGameWin->m_prMap.m_TradeItem[0].bBuyCheck);
	DDX_Check(pDX, IDC_BUYITEM_CHECK2, g_pCurGameWin->m_prMap.m_TradeItem[1].bBuyCheck);
	DDX_Check(pDX, IDC_BUYITEM_CHECK3, g_pCurGameWin->m_prMap.m_TradeItem[2].bBuyCheck);
	DDX_Check(pDX, IDC_BUYAM_CHECK, g_pCurGameWin->m_prMap.m_bBuyTDP);

	//Textboxes
	DDX_Text(pDX, IDC_BUYITEM_EDIT1, g_pCurGameWin->m_prMap.m_TradeItem[0].nBuyAmout);
	DDX_Text(pDX, IDC_BUYITEM_EDIT2, g_pCurGameWin->m_prMap.m_TradeItem[1].nBuyAmout);
	DDX_Text(pDX, IDC_BUYITEM_EDIT3, g_pCurGameWin->m_prMap.m_TradeItem[2].nBuyAmout);
	DDX_Text(pDX, IDC_BUYAM_EDIT, g_pCurGameWin->m_prMap.m_nAMNumbers);

	//ComboBoxes
	DDX_CBIndex(pDX, IDC_BUYITEM_COMBO1, g_pCurGameWin->m_prMap.m_TradeItem[0].nItemPos);
	DDX_CBIndex(pDX, IDC_BUYITEM_COMBO2, g_pCurGameWin->m_prMap.m_TradeItem[1].nItemPos);
	DDX_CBIndex(pDX, IDC_BUYITEM_COMBO3, g_pCurGameWin->m_prMap.m_TradeItem[2].nItemPos);

	//UI Refresh
	BOOL bChecked;
	bChecked = ((CButton*)GetDlgItem(IDC_BUYITEM_CHECK1))->GetCheck();
	GetDlgItem(IDC_BUYITEM_COMBO1)->EnableWindow(!bChecked);
	GetDlgItem(IDC_BUYITEM_EDIT1)->EnableWindow(!bChecked);
	bChecked = ((CButton*)GetDlgItem(IDC_BUYITEM_CHECK2))->GetCheck();
	GetDlgItem(IDC_BUYITEM_COMBO2)->EnableWindow(!bChecked);
	GetDlgItem(IDC_BUYITEM_EDIT2)->EnableWindow(!bChecked);
	bChecked = ((CButton*)GetDlgItem(IDC_BUYITEM_CHECK3))->GetCheck();
	GetDlgItem(IDC_BUYITEM_COMBO3)->EnableWindow(!bChecked);
	GetDlgItem(IDC_BUYITEM_EDIT3)->EnableWindow(!bChecked);
	bChecked = ((CButton*)GetDlgItem(IDC_BUYAM_CHECK))->GetCheck();
	GetDlgItem(IDC_BUYAM_EDIT)->EnableWindow(!bChecked);
}

BEGIN_MESSAGE_MAP(CDlgBuySetting, CDialog)
	ON_CONTROL_RANGE(BN_CLICKED, 1000, 2000, &CDlgBuySetting::OnButtonsClicked)
	ON_CBN_SELCHANGE(IDC_BUYITEM_COMBO1, &CDlgBuySetting::OnCbnSelchangeBuyitemCombo)
	ON_CBN_SELCHANGE(IDC_BUYITEM_COMBO2, &CDlgBuySetting::OnCbnSelchangeBuyitemCombo)
	ON_CBN_SELCHANGE(IDC_BUYITEM_COMBO3, &CDlgBuySetting::OnCbnSelchangeBuyitemCombo)
END_MESSAGE_MAP()

// CDlgBuySetting message handlers

BOOL CDlgBuySetting::OnInitDialog()
{
	InitComboBox((CComboBox*)GetDlgItem(IDC_BUYITEM_COMBO1));
	InitComboBox((CComboBox*)GetDlgItem(IDC_BUYITEM_COMBO2));
	InitComboBox((CComboBox*)GetDlgItem(IDC_BUYITEM_COMBO3));

	// Combo da co du lieu truoc khi CDialog::OnInitDialog goi UpdateData(FALSE)
	CDialog::OnInitDialog();

	if (m_ToolTip.Create(this))
	{
		m_ToolTip.AddTool(GetDlgItem(IDC_BUYITEM_COMBO1), _T("Loại thuốc cần mua"));
		m_ToolTip.AddTool(GetDlgItem(IDC_BUYITEM_EDIT1), _T("Số lượng cần mua"));
		m_ToolTip.AddTool(GetDlgItem(IDC_BUYITEM_COMBO2), _T("Loại thuốc cần mua"));
		m_ToolTip.AddTool(GetDlgItem(IDC_BUYITEM_EDIT2), _T("Số lượng cần mua"));
		m_ToolTip.AddTool(GetDlgItem(IDC_BUYITEM_COMBO3), _T("Loại thuốc cần mua"));
		m_ToolTip.AddTool(GetDlgItem(IDC_BUYITEM_EDIT3), _T("Số lượng cần mua"));
		m_ToolTip.AddTool(GetDlgItem(IDC_BUYAM_CHECK), _T("Mua Thổ Địa Phù (ở Tạp hóa)"));
		m_ToolTip.AddTool(GetDlgItem(IDC_BUYAM_EDIT), _T("Số lượng TĐP cần mua"));
	}

	return TRUE;
}

BOOL CDlgBuySetting::PreTranslateMessage(MSG* pMsg)
{
	m_ToolTip.RelayEvent(pMsg);
	return CDialog::PreTranslateMessage(pMsg);
}

void CDlgBuySetting::OnOK()
{
	if (g_pCurGameWin != NULL)
	{
		UpdateData();
		g_pCurGameWin->SetGameParams(tab_map);
	}
	EndDialog(IDOK);
}

void CDlgBuySetting::OnCancel()
{
	// Thay doi duoc ap dung ngay, dong cua so cung luu nhu nut Dong
	OnOK();
}

void CDlgBuySetting::OnButtonsClicked(UINT nID)
{
	if (g_pCurGameWin == NULL) return;
	UpdateData();
	g_pCurGameWin->SetGameParams(tab_map);
}

void CDlgBuySetting::OnCbnSelchangeBuyitemCombo()
{
	if (g_pCurGameWin == NULL) return;
	UpdateData();
	g_pCurGameWin->SetGameParams(tab_map);
}

void CDlgBuySetting::InitComboBox(CComboBox *pComboBox)
{
	/*
	  [0] uId = 0  Kim Sáng Dược (tiểu)
	  [1] uId = 1  Kim Sáng Dược (trung)
	  [2] uId = 2  Kim Sáng Dược (đại)
	  [3] uId = 3  Hồi Thiên đan
	  [4] uId = 4  Ngưng Thần đan (tiểu)
	  [5] uId = 5  Ngưng Thần đan (trung)
	  [6] uId = 6  Ngưng Thần đan (đại)
	  [7] uId = 7  Đại Bổ tán
	  [8] uId = 8  Thừa Tiên Mật (tiểu)
	  [9] uId = 9  Thừa Tiên Mật (trung)
	  [10] uId = 10  Tam Hoa Tán
	  [11] uId = 11  Hoắc Hương Chính khí hoàn
	  [12] uId = 12  Tam Thuận tán
	  [13] uId = 13  Ngân Kiều Khứ độc đan
	  [14] uId = 14  Cửu Chuyển Hoàn Hồn đan
	  [15] uId = 15  Ô đầu Hoàn thần đan
	  [16] uId = 16  Ngũ Hoa Ngọc Lộ Hoàn

	*/
	pComboBox->ResetContent();
	pComboBox->AddString(_T("Kim Sáng Dược (tiểu)"));
	pComboBox->AddString(_T("Kim Sáng Dược (trung)"));
	pComboBox->AddString(_T("Kim Sáng Dược (đại)"));
	pComboBox->AddString(_T("Hồi Thiên đan"));
	pComboBox->AddString(_T("Ngưng Thần đan (tiểu)"));
	pComboBox->AddString(_T("Ngưng Thần đan (trung)"));
	pComboBox->AddString(_T("Ngưng Thần đan (đại)"));
	pComboBox->AddString(_T("Đại Bổ tán"));
	pComboBox->AddString(_T("Thừa Tiên Mật (tiểu)"));
	pComboBox->AddString(_T("Thừa Tiên Mật (trung)"));
	pComboBox->AddString(_T("Tam Hoa Tán"));
	pComboBox->AddString(_T("Hoắc Hương Chính Khí Hoàn"));
	pComboBox->AddString(_T("Tam Thuận Tán"));
	pComboBox->AddString(_T("Ngân Kiều Khứ Độc Đan"));
	pComboBox->AddString(_T("Cửu Chuyển Hoàn Hồn đan"));
	pComboBox->AddString(_T("Ô Đầu Hoàn thần đan"));
	pComboBox->AddString(_T("Ngũ Hoa Ngọc Lộ Hoàn"));
	pComboBox->AddString(_T("Long Đàm Hóa Độc đan"));
}
