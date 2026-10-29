// DlgCollect.cpp : implementation file
//

#include "stdafx.h"
#include "../GameDef.h"
#include "VLAuto.h"
#include "GameWindow.h"
#include "DlgCollect.h"
#include "DlgCollectSetting.h"

// CDlgCollect dialog

IMPLEMENT_DYNAMIC(CDlgCollect, CDialog)

CDlgCollect::CDlgCollect(CWnd* pParent /*=NULL*/)
	: CDialog(CDlgCollect::IDD, pParent)
{
}

CDlgCollect::~CDlgCollect()
{
}

void CDlgCollect::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);

	if (g_pCurGameWin == NULL) return;

	//Checkboxes
	DDX_Check(pDX, IDC_AUTOCOLLECT_CHECK, g_pCurGameWin->m_prCollect.m_bAutoCollect);
	DDX_Check(pDX, IDC_PICKUPFORCE_CHECK, g_pCurGameWin->m_prCollect.m_bForcePickup);
	DDX_Check(pDX, IDC_MONEY_CHECK, g_pCurGameWin->m_prCollect.m_bCollectMoney);
	DDX_Check(pDX, IDC_PICKUPINCITY_CHECK, g_pCurGameWin->m_prCollect.m_bPickUpInCity);
	DDX_Check(pDX, IDC_SAVEJEWELLERY_CHECK, g_pCurGameWin->m_prCollect.m_bSaveJewellery);
	DDX_Check(pDX, IDC_AUTOARRANGE_CHECK, g_pCurGameWin->m_prCollect.m_bAutoArrange);
	DDX_Check(pDX, IDC_DECOMPOSE_CHECK, g_pCurGameWin->m_prCollect.m_bDecompose);

	//Comboboxes
	DDX_CBIndex(pDX, IDC_ITEM_COMBO, g_pCurGameWin->m_prCollect.m_nCollectEquip);
	DDX_CBIndex(pDX, IDC_VIOLETITEM_COMBO, g_pCurGameWin->m_prCollect.m_nVioletLine);

	//Editboxes
	DDX_Text(pDX, IDC_COLLECTRANGE_EDIT, g_pCurGameWin->m_prCollect.m_nCollectRange);
	DDX_Text(pDX, IDC_RINGLINE_EDIT, g_pCurGameWin->m_prCollect.m_nRingLine);
	DDX_Text(pDX, IDC_LACELINE_EDIT, g_pCurGameWin->m_prCollect.m_nLaceLine);

	//UI Refresh
	BOOL bChecked;
	bChecked = ((CButton*)GetDlgItem(IDC_AUTOCOLLECT_CHECK))->GetCheck();
	GetDlgItem(IDC_COLLECTRANGE_EDIT)->EnableWindow(!bChecked);
	GetDlgItem(IDC_ITEM_COMBO)->EnableWindow(bChecked);

	bChecked = ((CButton*)GetDlgItem(IDC_SAVEJEWELLERY_CHECK))->GetCheck();
	GetDlgItem(IDC_RINGLINE_EDIT)->EnableWindow(!bChecked);
	GetDlgItem(IDC_LACELINE_EDIT)->EnableWindow(!bChecked);
}

BEGIN_MESSAGE_MAP(CDlgCollect, CDialog)
	ON_CONTROL_RANGE(BN_CLICKED, 1000, 2000, &CDlgCollect::OnButtonsClicked)
	ON_CBN_SELCHANGE(IDC_ITEM_COMBO, &CDlgCollect::OnCbnSelchangeItemCombo)
	ON_CBN_SELCHANGE(IDC_VIOLETITEM_COMBO, &CDlgCollect::OnCbnSelchangeVioletitemCombo)
END_MESSAGE_MAP()


// CDlgCollect message handlers

BOOL CDlgCollect::OnInitDialog()
{
	CDialog::OnInitDialog();

	CComboBox *pItemCollectCombo = (CComboBox*)GetDlgItem(IDC_ITEM_COMBO);
	pItemCollectCombo->AddString(_T("Không nhặt V.khí & T.bị"));
	pItemCollectCombo->AddString(_T("Trang sức & đồ 1-4 ô (màu)"));
	pItemCollectCombo->AddString(_T("V.khí & T.bị theo thuộc tính"));
	pItemCollectCombo->AddString(_T("V.khí & Trang bị màu"));
	pItemCollectCombo->AddString(_T("V.khí & Trang bị (tất cả)"));

	CComboBox *pVioletItemCombo = (CComboBox*)GetDlgItem(IDC_VIOLETITEM_COMBO);
	pVioletItemCombo->AddString(_T("Giữ hết đồ tím"));
	pVioletItemCombo->AddString(_T("Đồ tím từ 2d"));
	pVioletItemCombo->AddString(_T("Đồ tím từ 3d"));
	pVioletItemCombo->AddString(_T("Đồ tím từ 4d"));
	pVioletItemCombo->AddString(_T("Đồ tím từ 5d"));
	pVioletItemCombo->AddString(_T("Đồ tím từ 6d"));
	pVioletItemCombo->AddString(_T("Ko nhặt đồ tím"));

	if (m_ToolTip.Create(this))
	{
		m_ToolTip.AddTool(GetDlgItem(IDC_AUTOCOLLECT_CHECK), _T("Bật/Tắt chế độ nhặt đồ"));
		m_ToolTip.AddTool(GetDlgItem(IDC_COLLECTRANGE_EDIT), _T("Phạm vi nhặt đồ"));
		m_ToolTip.AddTool(GetDlgItem(IDC_PICKUPFORCE_CHECK), _T("Đứng lại để nhặt bằng được vật phẩm mới di chuyển"));
		m_ToolTip.AddTool(GetDlgItem(IDC_MONEY_CHECK), _T("Nhặt tiền và các vật phẩm (không phải TB/VK)"));
		m_ToolTip.AddTool(GetDlgItem(IDC_ITEM_COMBO), _T("Nhặt trang bị và vũ khí"));
		m_ToolTip.AddTool(GetDlgItem(IDC_COLLECTSETTING_BUTTON), _T("Lọc thuộc tính, giá và cấp trang bị"));
		m_ToolTip.AddTool(GetDlgItem(IDC_PICKUPINCITY_CHECK), _T("Nhặt đồ trong thành"));
		m_ToolTip.AddTool(GetDlgItem(IDC_AUTOARRANGE_CHECK), _T("Sắp xếp đồ trong Hành trang"));
		m_ToolTip.AddTool(GetDlgItem(IDC_VIOLETITEM_COMBO), _T("Bán shop - giữ lại đồ tím theo số dòng"));
		m_ToolTip.AddTool(GetDlgItem(IDC_SAVEJEWELLERY_CHECK), _T("Giữ lại Nhẫn, Dây chuyền, Ngọc bội theo số dòng"));
		m_ToolTip.AddTool(GetDlgItem(IDC_RINGLINE_EDIT), _T("Số dòng Nhẫn muốn giữ lại"));
		m_ToolTip.AddTool(GetDlgItem(IDC_LACELINE_EDIT), _T("Số dòng DC/NB muốn giữ lại"));
		m_ToolTip.AddTool(GetDlgItem(IDC_DECOMPOSE_CHECK), _T("Rã trang bị không đạt điều kiện lọc (kể cả đồ khóa) thay vì vứt"));
	}

	return FALSE;
}

BOOL CDlgCollect::PreTranslateMessage(MSG* pMsg)
{
	m_ToolTip.RelayEvent(pMsg);
	return CDialog::PreTranslateMessage(pMsg);
}

void CDlgCollect::OnButtonsClicked(UINT nID)
{
	if (g_pCurGameWin == NULL) return;

	CButton *pButton = (CButton*)GetDlgItem(nID);
	switch (nID)
	{
	case IDC_COLLECTSETTING_BUTTON:
		{
			UpdateData();
			CDlgCollectSetting dlgSetting(this);
			dlgSetting.DoModal();
		}
		break;
	case IDC_DECOMPOSE_CHECK:
		if (pButton->GetCheck())
		{
			CString szMessage = _T("Nhân vật sẽ RÃ Vũ khí và Trang bị không đạt điều kiện lọc\n");
			szMessage += _T("(kể cả đồ khóa) thành Mảnh Trang Bị, không thể lấy lại.\n\n");
			szMessage += _T("Bạn chắc chắn muốn bật?");
			int nRet = MessageBox(szMessage, _T("VLAuto - Xác nhận"), MB_YESNO|MB_ICONQUESTION);
			if (nRet != IDYES) pButton->SetCheck(0);
		}
		break;
	}
	UpdateData();
	g_pCurGameWin->SetGameParams(tab_collect);
}

void CDlgCollect::OnCbnSelchangeItemCombo()
{
	if (g_pCurGameWin == NULL) return;
	CComboBox* pItemCollectCombo = (CComboBox*)GetDlgItem(IDC_ITEM_COMBO);
	int nSel = pItemCollectCombo->GetCurSel();
	if (nSel == 2)
	{
		int nAttribCount = 0;
		for (int i = 0; i < NUM_ATTRIB; i++)
			if (g_pCurGameWin->m_prCollect.m_arrMagic[i].nAttribType > 0) nAttribCount++;
		if (nAttribCount < 1)
		{
			CString szMessage = _T("Bạn chưa thiết lập điều kiện cho phần lượm Trang bị/ Vũ khí.\n");
			szMessage += _T("Nhân vật sẽ vứt bỏ những trang bị không thỏa mãn điều kiện.\n");
			szMessage += _T("Hãy đọc kỹ Hướng dẫn sử dụng cho mục này.");
			MessageBox(szMessage, _T("VLAuto - Chú ý"), MB_ICONEXCLAMATION);
			pItemCollectCombo->SetCurSel(3);
		}
	}
	UpdateData();
	if (nSel > 0 && nSel != 2)
	{
		g_pCurGameWin->m_prCollect.m_bCollectPrice = FALSE;
		g_pCurGameWin->m_prCollect.m_bCollectLevel = FALSE;
	}
	g_pCurGameWin->SetGameParams(tab_collect);
}

void CDlgCollect::OnCbnSelchangeVioletitemCombo()
{
	UpdateData();
	g_pCurGameWin->SetGameParams(tab_collect);
}
