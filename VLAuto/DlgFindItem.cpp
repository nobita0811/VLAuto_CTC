// FindItemDlg.cpp : implementation file
//

#include "stdafx.h"
#include "../GameDef.h"
#include "DWinProcess.h"
#include "VLAuto.h"
#include "GameCore.h"
#include "GameWindow.h"
#include "DlgFindItem.h"

// CFindItemDlg dialog

IMPLEMENT_DYNAMIC(CFindItemDlg, CDialog)

CFindItemDlg::CFindItemDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CFindItemDlg::IDD, pParent)
{
	memset(&m_UtilParam, 0, sizeof(m_UtilParam));
}

CFindItemDlg::~CFindItemDlg()
{
}

void CFindItemDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_ITEMNAME_COMBO, m_ItemNameCombo);
	DDX_Control(pDX, IDC_SERIES_COMBO, m_ItemSeriesCombo);
	DDX_Control(pDX, IDC_ATTRIBUTE_COMBO, m_ItemAttribCombo);
	DDX_Control(pDX, IDC_ATTLINE_COMBO, m_AttribLineCombo);
	DDX_Control(pDX, IDC_SPTYPE_COMBO, m_ItemSpTypeCombo);
	DDX_Control(pDX, IDC_EQUIPTYPE_COMBO, m_EquipTypeCombo);
	DDX_Control(pDX, IDC_EQUIPSEX_COMBO, m_EquipSexCombo);
}

BEGIN_MESSAGE_MAP(CFindItemDlg, CDialog)
	ON_BN_CLICKED(IDC_FINDITEM_BUTTON, &CFindItemDlg::OnBnClickedFinditemButton)
	ON_BN_CLICKED(IDC_STOP_BUTTON, &CFindItemDlg::OnBnClickedStopButton)
END_MESSAGE_MAP()


// CFindItemDlg message handlers

BOOL CFindItemDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	if (g_pCurGameWin != NULL)
	{
		TCHAR szCharName[32];
		TCVN3ToUnicode(g_pCurGameWin->m_szPlayerName, szCharName, 32);
		SetDlgItemText(IDC_NAME_EDIT, szCharName);
	}

	InitComboBoxes();

	SetDlgItemText(IDC_LEVEL_EDIT,_T("0"));
	SetDlgItemText(IDC_ATTMIN_EDIT,_T("0"));
	SetDlgItemText(IDC_ATTMAX_EDIT,_T("0"));
	SetDlgItemText(IDC_SELLPRICE_EDIT,_T("0"));
	((CButton*)GetDlgItem(IDC_R0_CHECK))->SetCheck(1);
	((CButton*)GetDlgItem(IDC_R1_CHECK))->SetCheck(1);
	((CButton*)GetDlgItem(IDC_R2_CHECK))->SetCheck(1);
	((CButton*)GetDlgItem(IDC_R3_CHECK))->SetCheck(1);
	((CButton*)GetDlgItem(IDC_R4_CHECK))->SetCheck(1);
	((CButton*)GetDlgItem(IDC_PLRSHOP_CHECK))->SetCheck(1);

	if (m_ToolTip.Create(this))
	{
		m_ToolTip.AddTool(GetDlgItem(IDC_ITEMNAME_COMBO), _T("Tên vật phẩm cần tìm (Unicode)"));
		m_ToolTip.AddTool(GetDlgItem(IDC_SERIES_COMBO), _T("Hệ (Kim - Mộc - Thủy - Hỏa - Thổ)"));
		m_ToolTip.AddTool(GetDlgItem(IDC_LEVEL_EDIT), _T("Đẳng cấp vật phẩm"));
		m_ToolTip.AddTool(GetDlgItem(IDC_ATTRIBUTE_COMBO), _T("Thuộc tính cần tìm"));
		m_ToolTip.AddTool(GetDlgItem(IDC_SPTYPE_COMBO), _T("Loại Trang bị"));
		m_ToolTip.AddTool(GetDlgItem(IDC_ATTLINE_COMBO), _T("Dòng thuộc tính"));
		m_ToolTip.AddTool(GetDlgItem(IDC_ATTMIN_EDIT), _T("Giá trị thuộc tính MIN"));
		m_ToolTip.AddTool(GetDlgItem(IDC_ATTMAX_EDIT), _T("Giá trị thuộc tính MAX"));
		m_ToolTip.AddTool(GetDlgItem(IDC_R0_CHECK), _T("Tìm trong Hành trang"));
		m_ToolTip.AddTool(GetDlgItem(IDC_R1_CHECK), _T("Tìm trong Rương đồ 1"));
		m_ToolTip.AddTool(GetDlgItem(IDC_R2_CHECK), _T("Tìm trong Rương đồ 2"));
		m_ToolTip.AddTool(GetDlgItem(IDC_R3_CHECK), _T("Tìm trong Rương đồ 3"));
		m_ToolTip.AddTool(GetDlgItem(IDC_R4_CHECK), _T("Tìm trong Rương Bang hội"));
		m_ToolTip.AddTool(GetDlgItem(IDC_PLRSHOP_CHECK), _T("Tìm từ Quầy hàng của người chơi khác"));
		m_ToolTip.AddTool(GetDlgItem(IDC_SELLPRICE_EDIT), _T("Giá bán chấp nhận được (lượng)"));
		m_ToolTip.AddTool(GetDlgItem(IDC_BUYIT_CHECK), _T("Mua luôn nếu tìm thấy"));
		m_ToolTip.AddTool(GetDlgItem(IDC_FINDITEM_BUTTON), _T("Bắt đầu tìm kiếm"));
		m_ToolTip.AddTool(GetDlgItem(IDCANCEL), _T("Đóng cửa sổ và ngưng mọi hoạt động"));
		m_ToolTip.AddTool(GetDlgItem(IDC_STOP_BUTTON), _T("Tạm ngưng hoạt động"));
	}

	return TRUE;
}

void CFindItemDlg::InitComboBoxes()
{
	m_EquipTypeCombo.AddString(_T("< Loại Trang phục >"));
	m_EquipTypeCombo.AddString(_T("Áo"));
	m_EquipTypeCombo.AddString(_T("Nhẫn"));
	m_EquipTypeCombo.AddString(_T("Dây chuyền"));
	m_EquipTypeCombo.AddString(_T("Giày"));
	m_EquipTypeCombo.AddString(_T("Thắt lưng"));
	m_EquipTypeCombo.AddString(_T("Nón (mũ)"));
	m_EquipTypeCombo.AddString(_T("Bao tay"));
	m_EquipTypeCombo.AddString(_T("Ngọc bội"));
	m_EquipTypeCombo.SetCurSel(0);
	
	m_EquipSexCombo.AddString(_T("< Giới tính >"));
	m_EquipSexCombo.AddString(_T("Nam"));
	m_EquipSexCombo.AddString(_T("Nữ"));
	m_EquipSexCombo.SetCurSel(0);

	m_ItemSeriesCombo.AddString(_T("< Hệ >"));
	m_ItemSeriesCombo.AddString(_T("Hệ Kim"));
	m_ItemSeriesCombo.AddString(_T("Hệ Mộc"));
	m_ItemSeriesCombo.AddString(_T("Hệ Thủy"));
	m_ItemSeriesCombo.AddString(_T("Hệ Hỏa"));
	m_ItemSeriesCombo.AddString(_T("Hệ Thổ"));
	m_ItemSeriesCombo.SetCurSel(0);

	m_ItemSpTypeCombo.AddString(_T("< Loại TB >"));
	m_ItemSpTypeCombo.AddString(_T("T.bị thường"));
	m_ItemSpTypeCombo.AddString(_T("Trang bị HK"));
	m_ItemSpTypeCombo.AddString(_T("Trang bị tím"));
	m_ItemSpTypeCombo.SetCurSel(0);

	int nIndex = m_ItemAttribCombo.AddString(_T("< Thuộc tính >"));
	m_ItemAttribCombo.SetItemData(nIndex, 0);
	for (int i = 0; i < ALL_ATTRIB; i++)
	{
		nIndex = m_ItemAttribCombo.AddString(AllAttribs[i].szAttDesc);
		m_ItemAttribCombo.SetItemData(nIndex, AllAttribs[i].nAttVal);
	}
	m_ItemAttribCombo.SetCurSel(0);

	m_AttribLineCombo.AddString(_T("< Dòng >"));
	m_AttribLineCombo.AddString(_T("Hiện 1"));
	m_AttribLineCombo.AddString(_T("Ẩn 1"));
	m_AttribLineCombo.AddString(_T("Hiện 2"));
	m_AttribLineCombo.AddString(_T("Ẩn 2"));
	m_AttribLineCombo.AddString(_T("Hiện 3"));
	m_AttribLineCombo.AddString(_T("Ẩn 3"));
	m_AttribLineCombo.SetCurSel(0);

	m_ItemNameCombo.AddString(_T(" < Tên vật phẩm (Unicode) >"));

	if (g_pCurGameWin == NULL)
		return;

	LPBYTE lpBaseAdd, lpCurAdd;
	DWinProcess winProcess(g_pCurGameWin->m_hWnd);
	if (!winProcess.ReadMemory((LPVOID)ITM_BASE_ADD, (LPVOID)&lpBaseAdd, 4))
		return;

	TCHAR szItemName[40];
	KItem Item;
	for (int i = 1; i < MAX_ITEM; i++)
	{
		lpCurAdd = lpBaseAdd + i*ITM_DATA_SIZE;
		winProcess.ReadMemory(lpCurAdd, (LPVOID)&Item, sizeof(Item));
		if (!Item.nObjIdx) continue;
		TCVN3ToUnicode(Item.szItemName, szItemName, 36);
		if (m_ItemNameCombo.FindString(0, szItemName) == CB_ERR)
			m_ItemNameCombo.AddString(szItemName);
	}
	m_ItemNameCombo.SetCurSel(0);
}

BOOL CFindItemDlg::PreTranslateMessage(MSG* pMsg)
{
	m_ToolTip.RelayEvent(pMsg);
	return CDialog::PreTranslateMessage(pMsg);
}

void CFindItemDlg::OnCancel()
{
	memset(&m_UtilParam, 0, sizeof(PARAM_UTIL));
	SetGameParams();
	CDialog::OnCancel();
}

void CFindItemDlg::SetGameParams()
{
	if (!g_pCurGameWin || !g_pCurGameWin->m_hWnd) return;
	::PostMessage(g_pCurGameWin->m_hWnd, WM_HOOK_UTIL, (WPARAM)&m_UtilParam, sizeof(PARAM_UTIL));
}

void CFindItemDlg::OnBnClickedFinditemButton()
{
	memset(&m_UtilParam, 0, sizeof(m_UtilParam));
	if (m_ItemNameCombo.GetCurSel())
	{
		CString szItemName;
		m_ItemNameCombo.GetWindowText(szItemName);
		UnicodeToTCVN3(szItemName, m_UtilParam.m_szItemName, 80);
	}
	m_UtilParam.m_nEquipDetail = m_EquipTypeCombo.GetCurSel() + 1;
	m_UtilParam.m_nEquipSex = m_EquipSexCombo.GetCurSel() - 1;
	m_UtilParam.m_nItemSeries = m_ItemSeriesCombo.GetCurSel() - 1;
	m_UtilParam.m_nItemLevel = GetDlgItemInt(IDC_LEVEL_EDIT);
	m_UtilParam.m_nItemSpType = m_ItemSpTypeCombo.GetCurSel() - 1;
	m_UtilParam.m_nItemAttrib = (int)m_ItemAttribCombo.GetItemData(m_ItemAttribCombo.GetCurSel());
	m_UtilParam.m_nAttLine = m_AttribLineCombo.GetCurSel();
	m_UtilParam.m_nAttMin = GetDlgItemInt(IDC_ATTMIN_EDIT);
	m_UtilParam.m_nAttMax = GetDlgItemInt(IDC_ATTMAX_EDIT);
	m_UtilParam.m_nItemPrice = GetDlgItemInt(IDC_SELLPRICE_EDIT);
	m_UtilParam.m_bFromR0 = ((CButton*)GetDlgItem(IDC_R0_CHECK))->GetCheck();
	m_UtilParam.m_bFromR1 = ((CButton*)GetDlgItem(IDC_R1_CHECK))->GetCheck();
	m_UtilParam.m_bFromR2 = ((CButton*)GetDlgItem(IDC_R2_CHECK))->GetCheck();
	m_UtilParam.m_bFromR3 = ((CButton*)GetDlgItem(IDC_R3_CHECK))->GetCheck();
	m_UtilParam.m_bFromR4 = ((CButton*)GetDlgItem(IDC_R4_CHECK))->GetCheck();
	m_UtilParam.m_bFromPlrShop = ((CButton*)GetDlgItem(IDC_PLRSHOP_CHECK))->GetCheck();
	m_UtilParam.m_bBuyIt = ((CButton*)GetDlgItem(IDC_BUYIT_CHECK))->GetCheck();

	if (!m_UtilParam.m_szItemName[0] &&
		m_UtilParam.m_nEquipDetail < 2 &&
		m_UtilParam.m_nItemSeries < 0 && 
		m_UtilParam.m_nItemLevel < 1 &&
		m_UtilParam.m_nItemAttrib < 1)
	{
		MessageBox(_T("Bạn chưa chọn điều kiện tìm kiếm nào cả!"), 0, MB_ICONEXCLAMATION);
		memset(m_UtilParam.m_szItemName, 0, sizeof(m_UtilParam.m_szItemName));
		return;
	}
	m_UtilParam.m_bDoingUtil = TRUE;
	m_UtilParam.m_nUtilTask = util_buyitem;
	SetGameParams();
}

void CFindItemDlg::OnBnClickedStopButton()
{
	memset(&m_UtilParam, 0, sizeof(m_UtilParam));
	SetGameParams();
}
