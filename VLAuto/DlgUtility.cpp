// DlgUtility.cpp : implementation file
//

#include "stdafx.h"
#include "../GameDef.h"
#include "VLAuto.h"
#include "GameWindow.h"
#include "DlgUtility.h"

// CUtilityDlg dialog

IMPLEMENT_DYNAMIC(CUtilityDlg, CDialog)

CUtilityDlg::CUtilityDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CUtilityDlg::IDD, pParent)
{
	memset(&m_UtilParam, 0, sizeof(m_UtilParam));
}

CUtilityDlg::~CUtilityDlg()
{
}

void CUtilityDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_ITEMOBJ_COMBO, m_ItemKindCombo);
	DDX_Control(pDX, IDC_MOVEFROM_COMBO, m_SourceRoomCombo);
	DDX_Control(pDX, IDC_MOVETO_COMBO, m_DestRoomCombo);
	DDX_Control(pDX, IDC_HTLEVEL_COMBO, m_HTLevelCombo);
}

BEGIN_MESSAGE_MAP(CUtilityDlg, CDialog)
	ON_BN_CLICKED(IDC_MOVEITEM_BUTTON, &CUtilityDlg::OnBnClickedMoveitemButton)
	ON_BN_CLICKED(IDC_MAKEHT_BUTTON, &CUtilityDlg::OnBnClickedMakehtButton)
	ON_BN_CLICKED(IDC_UPGRHT_BUTTON, &CUtilityDlg::OnBnClickedUpgrhtButton)
	ON_BN_CLICKED(IDC_GIVEKCL_BUTTON, &CUtilityDlg::OnBnClickedGivekclButton)
	ON_BN_CLICKED(IDC_STOP_BUTTON, &CUtilityDlg::OnBnClickedStopButton)
END_MESSAGE_MAP()


// CUtilityDlg message handlers

BOOL CUtilityDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	if (g_pCurGameWin != NULL)
	{
		TCHAR szCharName[32];
		TCVN3ToUnicode(g_pCurGameWin->m_szPlayerName, szCharName, 32);
		SetDlgItemText(IDC_NAME_EDIT, szCharName);
	}

	InitComboBoxes();

	if (m_ToolTip.Create(this))
	{
		m_ToolTip.AddTool(GetDlgItem(IDC_ITEMOBJ_COMBO), _T("Loại vật phẩm cần chuyển"));
		m_ToolTip.AddTool(GetDlgItem(IDC_MOVEFROM_COMBO), _T("Địa điểm ban đầu của vật phẩm"));
		m_ToolTip.AddTool(GetDlgItem(IDC_MOVETO_COMBO), _T("Địa điểm muốn chuyển vật phẩm đến"));
		m_ToolTip.AddTool(GetDlgItem(IDC_MOVEITEM_BUTTON), _T("Bắt đầu chuyển vật phẩm"));

		m_ToolTip.AddTool(GetDlgItem(IDC_MAKEHT_BUTTON), _T("Tinh luyện Huyền Tinh từ trang sức"));
		m_ToolTip.AddTool(GetDlgItem(IDC_UPGRHT_BUTTON), _T("Nâng cấp Huyền Tinh lên cấp cao"));
		m_ToolTip.AddTool(GetDlgItem(IDC_HTLEVEL_COMBO), _T("Cấp độ Huyền Tinh cần nâng"));

		m_ToolTip.AddTool(GetDlgItem(IDC_GIVEKCL_BUTTON), _T("Giao nộp Khiêu chiến lệnh cho Công thành quan"));
		m_ToolTip.AddTool(GetDlgItem(IDCANCEL), _T("Đóng cửa sổ và ngưng mọi hoạt động Tiện ích"));
		m_ToolTip.AddTool(GetDlgItem(IDC_STOP_BUTTON), _T("Tạm ngưng hoạt động Tiện ích"));
	}

	return TRUE;
}

void CUtilityDlg::InitComboBoxes()
{
	m_ItemKindCombo.AddString(_T("Vật phẩm mẫu"));
	m_ItemKindCombo.AddString(_T("Trang sức"));
	m_ItemKindCombo.AddString(_T("Nhẫn"));
	m_ItemKindCombo.AddString(_T("Dây chuyền"));
	m_ItemKindCombo.AddString(_T("Ngọc bội"));
	m_ItemKindCombo.AddString(_T("Kim Nguyên Bảo"));
	m_ItemKindCombo.AddString(_T("Thủy Tinh"));
	m_ItemKindCombo.AddString(_T("Tinh Hồng Bảo Thạch"));
	m_ItemKindCombo.AddString(_T("Tiên Thảo Lộ"));
	m_ItemKindCombo.AddString(_T("Phúc Duyên Lộ"));
	m_ItemKindCombo.AddString(_T("Khiêu chiến lệnh"));
	m_ItemKindCombo.AddString(_T("Tất cả vật phẩm"));
	m_ItemKindCombo.SetCurSel(0);

	int nIndex;
	nIndex = m_SourceRoomCombo.AddString(_T("Hành trang"));
	m_SourceRoomCombo.SetItemData(nIndex, room_equipment);
	nIndex = m_SourceRoomCombo.AddString(_T("Rương đồ 1"));
	m_SourceRoomCombo.SetItemData(nIndex, room_repository);
	nIndex = m_SourceRoomCombo.AddString(_T("Rương đồ 2"));
	m_SourceRoomCombo.SetItemData(nIndex, room_repo02);
	nIndex = m_SourceRoomCombo.AddString(_T("Rương đồ 3"));
	m_SourceRoomCombo.SetItemData(nIndex, room_repo03);
	nIndex = m_SourceRoomCombo.AddString(_T("Rương Bang hội"));
	m_SourceRoomCombo.SetItemData(nIndex, room_guid);
	nIndex = m_SourceRoomCombo.AddString(_T("Giao dịch"));
	m_SourceRoomCombo.SetItemData(nIndex, room_trade);
	m_SourceRoomCombo.SetCurSel(0);

	nIndex = m_DestRoomCombo.AddString(_T("Hành trang"));
	m_DestRoomCombo.SetItemData(nIndex, room_equipment);
	nIndex = m_DestRoomCombo.AddString(_T("Rương đồ 1"));
	m_DestRoomCombo.SetItemData(nIndex, room_repository);
	nIndex = m_DestRoomCombo.AddString(_T("Rương đồ 2"));
	m_DestRoomCombo.SetItemData(nIndex, room_repo02);
	nIndex = m_DestRoomCombo.AddString(_T("Rương đồ 3"));
	m_DestRoomCombo.SetItemData(nIndex, room_repo03);
	nIndex = m_DestRoomCombo.AddString(_T("Rương Bang hội"));
	m_DestRoomCombo.SetItemData(nIndex, room_guid);
	nIndex = m_DestRoomCombo.AddString(_T("Giao dịch"));
	m_DestRoomCombo.SetItemData(nIndex, room_trade);
	nIndex = m_DestRoomCombo.AddString(_T("Quăng ra đất"));
	m_DestRoomCombo.SetItemData(nIndex, room_num);
	m_DestRoomCombo.SetCurSel(1);

	m_HTLevelCombo.AddString(_T("Thăng cấp 1->2"));
	m_HTLevelCombo.AddString(_T("Thăng cấp 2->3"));
	m_HTLevelCombo.AddString(_T("Thăng cấp 3->4"));
	m_HTLevelCombo.AddString(_T("Thăng cấp 4->5"));
	m_HTLevelCombo.AddString(_T("Thăng cấp 5->6"));
	m_HTLevelCombo.AddString(_T("Thăng cấp 6->7"));
	m_HTLevelCombo.AddString(_T("Thăng cấp 7->8"));
	m_HTLevelCombo.SetCurSel(0);
}

BOOL CUtilityDlg::PreTranslateMessage(MSG* pMsg)
{
	m_ToolTip.RelayEvent(pMsg);
	return CDialog::PreTranslateMessage(pMsg);
}

void CUtilityDlg::OnCancel()
{
	memset(&m_UtilParam, 0, sizeof(PARAM_UTIL));
	SetGameParams();
	CDialog::OnCancel();
}

void CUtilityDlg::SetGameParams()
{
	if (!g_pCurGameWin || !g_pCurGameWin->m_hWnd) return;
	::PostMessage(g_pCurGameWin->m_hWnd, WM_HOOK_UTIL, (WPARAM)&m_UtilParam, sizeof(PARAM_UTIL));
}

void CUtilityDlg::OnBnClickedMoveitemButton()
{
	memset(&m_UtilParam, 0, sizeof(m_UtilParam));
	m_UtilParam.m_nItemObjIdx = m_ItemKindCombo.GetCurSel();
	m_UtilParam.m_nFromRoom = (int)m_SourceRoomCombo.GetItemData(m_SourceRoomCombo.GetCurSel());
	m_UtilParam.m_nToRoom = (int)m_DestRoomCombo.GetItemData(m_DestRoomCombo.GetCurSel());
	if (m_UtilParam.m_nItemObjIdx < 0 ||
		m_UtilParam.m_nFromRoom < 0 ||
		m_UtilParam.m_nToRoom < 0)
		return;
	if (m_UtilParam.m_nFromRoom == m_UtilParam.m_nToRoom)
	{
		MessageBox(_T("Hãy chọn nơi chuyển đi và chuyển đến khác nhau!"), 0, MB_ICONEXCLAMATION);
		return;
	}
	if (m_UtilParam.m_nToRoom == room_num)
	{
		if (m_UtilParam.m_nItemObjIdx >= 5 && m_UtilParam.m_nItemObjIdx <= 9)
		{
			CString szItemName;
			m_ItemKindCombo.GetLBText(m_UtilParam.m_nItemObjIdx, szItemName);
			szItemName += _T(" không thể ném ra được!");
			MessageBox(szItemName, 0, MB_ICONEXCLAMATION);
			return;
		}
	}
	m_UtilParam.m_bDoingUtil = TRUE;
	m_UtilParam.m_nUtilTask = util_moveitem;
	SetGameParams();
}

void CUtilityDlg::OnBnClickedMakehtButton()
{
	memset(&m_UtilParam, 0, sizeof(m_UtilParam));
	m_UtilParam.m_bDoingUtil = TRUE;
	m_UtilParam.m_nUtilTask = util_makeht;
	SetGameParams();
}

void CUtilityDlg::OnBnClickedUpgrhtButton()
{
	memset(&m_UtilParam, 0, sizeof(m_UtilParam));
	m_UtilParam.m_nHTLevel = m_HTLevelCombo.GetCurSel() + 1;
	if (m_UtilParam.m_nHTLevel < 1) return;
	m_UtilParam.m_bDoingUtil = TRUE;
	m_UtilParam.m_nUtilTask = util_upgradeht;
	SetGameParams();
}

void CUtilityDlg::OnBnClickedGivekclButton()
{
	memset(&m_UtilParam, 0, sizeof(m_UtilParam));
	m_UtilParam.m_bDoingUtil = TRUE;
	m_UtilParam.m_nUtilTask = util_givekcl;
	SetGameParams();
}

void CUtilityDlg::OnBnClickedStopButton()
{
	memset(&m_UtilParam, 0, sizeof(m_UtilParam));
	SetGameParams();
}
