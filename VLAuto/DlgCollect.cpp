// DlgCollect.cpp : implementation file
//

#include "stdafx.h"
#include "../GameDef.h"
#include "VLAuto.h"
#include "GameWindow.h"
#include "DlgCollect.h"

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
	DDX_Control(pDX, IDC_ITEMATTRIB_COMBO, m_ItemAttribCombo);
	DDX_Control(pDX, IDC_ITEMATTRIB_LIST, m_ItemAttribList);

	if (g_pCurGameWin == NULL) return;

	//Checkboxes
	DDX_Check(pDX, IDC_AUTOCOLLECT_CHECK, g_pCurGameWin->m_prCollect.m_bAutoCollect);
	DDX_Check(pDX, IDC_PICKUPFORCE_CHECK, g_pCurGameWin->m_prCollect.m_bForcePickup);
	DDX_Check(pDX, IDC_MONEY_CHECK, g_pCurGameWin->m_prCollect.m_bCollectMoney);
	DDX_Check(pDX, IDC_PICKUPINCITY_CHECK, g_pCurGameWin->m_prCollect.m_bPickUpInCity);
	DDX_Check(pDX, IDC_SAVEJEWELLERY_CHECK, g_pCurGameWin->m_prCollect.m_bSaveJewellery);
	DDX_Check(pDX, IDC_AUTOARRANGE_CHECK, g_pCurGameWin->m_prCollect.m_bAutoArrange);
	DDX_Check(pDX, IDC_PRICE_CHECK, g_pCurGameWin->m_prCollect.m_bCollectPrice);
	DDX_Check(pDX, IDC_LEVEL_CHECK, g_pCurGameWin->m_prCollect.m_bCollectLevel);
	DDX_Check(pDX, IDC_DECOMPOSE_CHECK, g_pCurGameWin->m_prCollect.m_bDecompose);

	//Comboboxes
	DDX_CBIndex(pDX, IDC_ITEM_COMBO, g_pCurGameWin->m_prCollect.m_nCollectEquip);
	DDX_CBIndex(pDX, IDC_VIOLETITEM_COMBO, g_pCurGameWin->m_prCollect.m_nVioletLine);

	//Editboxes
	DDX_Text(pDX, IDC_COLLECTRANGE_EDIT, g_pCurGameWin->m_prCollect.m_nCollectRange);
	DDX_Text(pDX, IDC_PRICE_EDIT, g_pCurGameWin->m_prCollect.m_nCollectPrice);
	DDX_Text(pDX, IDC_LEVEL_EDIT, g_pCurGameWin->m_prCollect.m_nCollectLevel);
	DDX_Text(pDX, IDC_RINGLINE_EDIT, g_pCurGameWin->m_prCollect.m_nRingLine);
	DDX_Text(pDX, IDC_LACELINE_EDIT, g_pCurGameWin->m_prCollect.m_nLaceLine);

	//UI Refresh
	BOOL bChecked;
	bChecked = ((CButton*)GetDlgItem(IDC_AUTOCOLLECT_CHECK))->GetCheck();
	GetDlgItem(IDC_COLLECTRANGE_EDIT)->EnableWindow(!bChecked);
	GetDlgItem(IDC_ITEM_COMBO)->EnableWindow(bChecked);
	GetDlgItem(IDC_ITEMATTRIB_COMBO)->EnableWindow(bChecked);
	GetDlgItem(IDC_ITEMATTRIB_EDIT)->EnableWindow(bChecked);
	GetDlgItem(IDC_ITEMATTRIB_LIST)->EnableWindow(bChecked);
	GetDlgItem(IDC_ADD_BUTTON)->EnableWindow(bChecked);
	GetDlgItem(IDC_DEL_BUTTON)->EnableWindow(bChecked);
	GetDlgItem(IDC_LOAD_BUTTON)->EnableWindow(bChecked);

	bChecked = ((CButton*)GetDlgItem(IDC_PRICE_CHECK))->GetCheck();
	GetDlgItem(IDC_PRICE_EDIT)->EnableWindow(!bChecked);
	bChecked = ((CButton*)GetDlgItem(IDC_LEVEL_CHECK))->GetCheck();
	GetDlgItem(IDC_LEVEL_EDIT)->EnableWindow(!bChecked);
	bChecked = ((CButton*)GetDlgItem(IDC_SAVEJEWELLERY_CHECK))->GetCheck();
	GetDlgItem(IDC_RINGLINE_EDIT)->EnableWindow(!bChecked);
	GetDlgItem(IDC_LACELINE_EDIT)->EnableWindow(!bChecked);

	UpdateMagicOptions(pDX->m_bSaveAndValidate);
}

BEGIN_MESSAGE_MAP(CDlgCollect, CDialog)
	ON_CONTROL_RANGE(BN_CLICKED, 1000, 2000, &CDlgCollect::OnButtonsClicked)
	ON_CBN_SELCHANGE(IDC_ITEMATTRIB_COMBO, &CDlgCollect::OnCbnSelchangeItemattribCombo)
	ON_CBN_SELCHANGE(IDC_ITEM_COMBO, &CDlgCollect::OnCbnSelchangeItemCombo)
	ON_CBN_SELCHANGE(IDC_VIOLETITEM_COMBO, &CDlgCollect::OnCbnSelchangeVioletitemCombo)
	ON_LBN_SELCHANGE(IDC_ITEMATTRIB_LIST, &CDlgCollect::OnLbnSelchangeItemattribList)
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

	for (int i = 0; i < ALL_ATTRIB; i++)
	{
		// Hut SL/NL, +ky nang, khong the pha huy, bo qua ne tranh: mac dinh luon giu (VLCollect),
		// nhung neu them vao danh sach thi dung nguong trong danh sach
		int nIndex = m_ItemAttribCombo.AddString(AllAttribs[i].szAttDesc);
		m_ItemAttribCombo.SetItemData(nIndex, AllAttribs[i].nAttVal);
	}

	if (m_ToolTip.Create(this))
	{
		m_ToolTip.AddTool(GetDlgItem(IDC_AUTOCOLLECT_CHECK), _T("Bật/Tắt chế độ nhặt đồ"));
		m_ToolTip.AddTool(GetDlgItem(IDC_COLLECTRANGE_EDIT), _T("Phạm vi nhặt đồ"));
		m_ToolTip.AddTool(GetDlgItem(IDC_PICKUPFORCE_CHECK), _T("Đứng lại để nhặt bằng được vật phẩm mới di chuyển"));
		m_ToolTip.AddTool(GetDlgItem(IDC_MONEY_CHECK), _T("Nhặt tiền và các vật phẩm (không phải TB/VK)"));
		m_ToolTip.AddTool(GetDlgItem(IDC_ITEM_COMBO), _T("Nhặt trang bị và vũ khí"));
		m_ToolTip.AddTool(GetDlgItem(IDC_ITEMATTRIB_COMBO), _T("Thiết lập thuộc tính lọc đồ"));
		m_ToolTip.AddTool(GetDlgItem(IDC_ITEMATTRIB_LIST), _T("Danh sách thuộc tính lọc đồ"));
		m_ToolTip.AddTool(GetDlgItem(IDC_ADD_BUTTON), _T("Thêm vào danh sách"));
		m_ToolTip.AddTool(GetDlgItem(IDC_DEL_BUTTON), _T("Xóa khỏi danh sách"));
		m_ToolTip.AddTool(GetDlgItem(IDC_PICKUPINCITY_CHECK), _T("Nhặt đồ trong thành"));
		m_ToolTip.AddTool(GetDlgItem(IDC_PRICE_CHECK), _T("Giữ lại đồ giá cao (lượng)"));
		m_ToolTip.AddTool(GetDlgItem(IDC_PRICE_EDIT), _T("Giá giữ lại trên mỗi ô hành trang"));
		m_ToolTip.AddTool(GetDlgItem(IDC_LEVEL_CHECK), _T("Giữ lại đồ cấp cao"));
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
	CComboBox* pItemCollectCombo = (CComboBox*)GetDlgItem(IDC_ITEM_COMBO);
	switch (nID)
	{
	case IDC_ADD_BUTTON:
		OnBnClickedAddButton();
		break;
	case IDC_DEL_BUTTON:
		OnBnClickedDelButton();
		break;
	case IDC_SAVE_BUTTON:
		OnBnClickedSaveButton();
		break;
	case IDC_LOAD_BUTTON:
		OnBnClickedLoadButton();
		break;
	case IDC_PRICE_CHECK:
		if (pButton->GetCheck() && pItemCollectCombo->GetCurSel() != 2)
		{
			CString szMessage = _T("Nhân vật sẽ vứt bỏ Vũ khí và Trang bị\n");
			szMessage += _T("nếu giá bán Shop không lớn hơn mức thiết lập.\n\n");
			szMessage += _T("Bạn đã đọc kỹ Hướng dẫn cho mục này chưa?");
			int nRet = MessageBox(szMessage, _T("VLAuto - Xác nhận"), MB_YESNO|MB_ICONQUESTION);
			if (nRet != IDYES) pButton->SetCheck(0);
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
	case IDC_LEVEL_CHECK:
		if (pButton->GetCheck() && pItemCollectCombo->GetCurSel() != 2)
		{
			CString szMessage = _T("Nhân vật sẽ vứt bỏ Vũ khí và Trang bị\n");
			szMessage += _T("nếu cấp độ không lớn hơn mức thiết lập.\n\n");
			szMessage += _T("Bạn đã đọc kỹ Hướng dẫn cho mục này chưa?");
			int nRet = MessageBox(szMessage, _T("VLAuto - Xác nhận"), MB_YESNO|MB_ICONQUESTION);
			if (nRet != IDYES) pButton->SetCheck(0);
		}
		break;
	}
	UpdateData();
	g_pCurGameWin->SetGameParams(tab_collect);
}

void CDlgCollect::UpdateMagicOptions(BOOL bSave)
{
	CString szDescript, szValue;
	int i, j;
	if (bSave)
	{
		memset(g_pCurGameWin->m_prCollect.m_arrMagic, 0, sizeof(g_pCurGameWin->m_prCollect.m_arrMagic));
		int nListCount = m_ItemAttribList.GetCount();
		for (i=0; i<nListCount; i++)
		{
			int nAttType = LOWORD(m_ItemAttribList.GetItemData(i));
			int nAttValue = HIWORD(m_ItemAttribList.GetItemData(i));
			g_pCurGameWin->m_prCollect.m_arrMagic[i].nAttribType = nAttType;
			g_pCurGameWin->m_prCollect.m_arrMagic[i].nAttribValue = nAttValue;
		}
	
	}
	else
	{
		m_ItemAttribList.ResetContent();
		for (i = 0; i < NUM_ATTRIB; i++)
		{
			int nAttType = g_pCurGameWin->m_prCollect.m_arrMagic[i].nAttribType;
			int nAttValue = g_pCurGameWin->m_prCollect.m_arrMagic[i].nAttribValue;
			int nListCount = m_ItemAttribCombo.GetCount();
			if (nAttType>0) for (j=0; j<nListCount; j++)
			{
				if (m_ItemAttribCombo.GetItemData(j) == nAttType)
				{
					szValue.Format(_T(" > %d"), nAttValue);
					m_ItemAttribCombo.GetLBText(j, szDescript);
					szDescript += szValue;
					int nIndex = m_ItemAttribList.AddString(szDescript);
					m_ItemAttribList.SetItemData(nIndex, MAKELONG(nAttType, nAttValue));
				}
			}
		}
	}
}

void CDlgCollect::OnCbnSelchangeItemattribCombo()
{
	SetDlgItemInt(IDC_ITEMATTRIB_EDIT, 0);
	GetDlgItem(IDC_ITEMATTRIB_EDIT)->SetFocus();
}

void CDlgCollect::OnCbnSelchangeItemCombo()
{
	if (g_pCurGameWin == NULL) return;
	CComboBox* pItemCollectCombo = (CComboBox*)GetDlgItem(IDC_ITEM_COMBO);
	if (pItemCollectCombo->GetCurSel() == 2)
	{
		if (m_ItemAttribList.GetCount() < 1)
		{
			CString szMessage = _T("Bạn chưa thiết lập điều kiện cho phần lượm Trang bị/ Vũ khí.\n");
			szMessage += _T("Nhân vật sẽ vứt bỏ những trang bị không thỏa mãn điều kiện.\n");
			szMessage += _T("Hãy đọc kỹ Hướng dẫn sử dụng cho mục này.");
			MessageBox(szMessage, _T("VLAuto - Chú ý"), MB_ICONEXCLAMATION);
			pItemCollectCombo->SetCurSel(3);
		}
	}
	else if (pItemCollectCombo->GetCurSel() > 0)
	{
		((CButton*)GetDlgItem(IDC_PRICE_CHECK))->SetCheck(0);
		((CButton*)GetDlgItem(IDC_LEVEL_CHECK))->SetCheck(0);
	}
	UpdateData();
	g_pCurGameWin->SetGameParams(tab_collect);
}

void CDlgCollect::OnCbnSelchangeVioletitemCombo()
{
	UpdateData();
	g_pCurGameWin->SetGameParams(tab_collect);
}

void CDlgCollect::OnLbnSelchangeItemattribList()
{
	int nIndex = m_ItemAttribList.GetCurSel();
	if (nIndex < 0) return;
	int nAttType = LOWORD(m_ItemAttribList.GetItemData(nIndex));
	int nAttValue = HIWORD(m_ItemAttribList.GetItemData(nIndex));

	int nListCount = m_ItemAttribCombo.GetCount();
	for (int i = 0; i < nListCount; i++)
	{
		if (m_ItemAttribCombo.GetItemData(i) == nAttType)
		{
			m_ItemAttribCombo.SetCurSel(i);
			SetDlgItemInt(IDC_ITEMATTRIB_EDIT, nAttValue);
			return;
		}
	}
}

void CDlgCollect::OnBnClickedAddButton()
{
	CString szDescript, szValue;
	int nIndex = m_ItemAttribCombo.GetCurSel();
	if (nIndex < 0) return;
	int nAttType = (int)m_ItemAttribCombo.GetItemData(nIndex);
	int nAttValue = GetDlgItemInt(IDC_ITEMATTRIB_EDIT);
	szValue.Format(_T(" > %d"), nAttValue);
	m_ItemAttribCombo.GetLBText(nIndex, szDescript);
	szDescript += szValue;

	int nListCount = m_ItemAttribList.GetCount();
	for (int i = 0; i < nListCount; i++)
	{
		if (LOWORD(m_ItemAttribList.GetItemData(i)) == nAttType)
		{
			m_ItemAttribList.DeleteString(i);
			m_ItemAttribList.InsertString(i, szDescript);
			m_ItemAttribList.SetItemData(i, MAKELONG(nAttType, nAttValue));
			m_ItemAttribList.SetCurSel(i);
			return;
		}
	}
	if (nListCount >= NUM_ATTRIB) return;
	nIndex = m_ItemAttribList.AddString(szDescript);
	m_ItemAttribList.SetItemData(nIndex, MAKELONG(nAttType, nAttValue));
	m_ItemAttribList.SetCurSel(nIndex);
}

void CDlgCollect::OnBnClickedDelButton()
{
	m_ItemAttribList.DeleteString(m_ItemAttribList.GetCurSel());
}

void CDlgCollect::OnBnClickedSaveButton()
{
	CFileDialog fDlg(FALSE, _T(".ita"), 0,
		OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT, _T("Item Attribute File (*.ita)|*.ita|"), this);
	if (fDlg.DoModal() == IDCANCEL)
		return;
	CString szFileName = fDlg.GetPathName();
	CFile mvFile;
	if (!mvFile.Open(szFileName, CFile::modeCreate|CFile::modeWrite))
	{
		MessageBox(_T("Lỗi: không thể ghi tập tin!"), 0, MB_ICONEXCLAMATION);
		return;
	}
	char szVersion[12] = "";
	strcpy(szVersion, _SZCURVERSION);
	mvFile.Write(szVersion, sizeof(szVersion));
	mvFile.Write(g_pCurGameWin->m_prCollect.m_arrMagic, sizeof(g_pCurGameWin->m_prCollect.m_arrMagic));
	mvFile.Close();
}

void CDlgCollect::OnBnClickedLoadButton()
{
	CFileDialog fDlg(TRUE, _T(".ita"), 0,
		OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT, _T("Item Attribute File (*.ita)|*.ita|"), this);
	if (fDlg.DoModal() == IDCANCEL)
		return;
	CString szFileName = fDlg.GetPathName();
	CFile mvFile;
	if (!mvFile.Open(szFileName, CFile::modeRead | CFile::shareDenyWrite))
	{
		MessageBox(_T("Lỗi: không thể đọc tập tin!"), 0, MB_ICONEXCLAMATION);
		return;
	}
	char szVersion[12] = "";
	mvFile.Read(szVersion, sizeof(szVersion));
	if (strcmp(szVersion, _SZCURVERSION))
	{
		MessageBox(_T("Lỗi: phiên bản không tương thích!"), 0, MB_ICONEXCLAMATION);
		mvFile.Close();
		return;
	}
	memset(&g_pCurGameWin->m_prCollect.m_arrMagic, 0, sizeof(g_pCurGameWin->m_prCollect.m_arrMagic));
	mvFile.Read(&g_pCurGameWin->m_prCollect.m_arrMagic, sizeof(g_pCurGameWin->m_prCollect.m_arrMagic));
	mvFile.Close();
	// Update Data
	UpdateData(0);
	g_pCurGameWin->SetGameParams(tab_collect);
}
