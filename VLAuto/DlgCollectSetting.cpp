// DlgCollectSetting.cpp : implementation file
//

#include "stdafx.h"
#include "../GameDef.h"
#include "VLAuto.h"
#include "GameWindow.h"
#include "DlgCollectSetting.h"

#ifndef WM_HOOK_DEBUG
#define WM_HOOK_DEBUG		WM_USER + 111
#endif

// CDlgCollectSetting dialog

IMPLEMENT_DYNAMIC(CDlgCollectSetting, CDialog)

CDlgCollectSetting::CDlgCollectSetting(CWnd* pParent /*=NULL*/)
	: CDialog(CDlgCollectSetting::IDD, pParent)
{
	static const struct { LPCTSTR szFile; int nCmd; UINT nList, nCombo, nAdd, nDel; } s_Init[nl_count] =
	{
		{ _T("BlackItems.txt"), dbg_cl_blacklist, IDC_BLACKITEM_LIST, IDC_BLACKITEM_COMBO, IDC_BLACKADD_BUTTON, IDC_BLACKDEL_BUTTON },
		{ _T("RuongItem.txt"), dbg_cl_nosavelist, IDC_RUONGITEM_LIST, IDC_RUONGITEM_COMBO, IDC_RUONGADD_BUTTON, IDC_RUONGDEL_BUTTON },
	};
	for (int k = 0; k < nl_count; k++)
	{
		ITEM_NAME_LIST& nl = m_NameList[k];
		nl.szFile = s_Init[k].szFile;
		nl.nReloadCmd = s_Init[k].nCmd;
		nl.nListID = s_Init[k].nList;
		nl.nComboID = s_Init[k].nCombo;
		nl.nAddID = s_Init[k].nAdd;
		nl.nDelID = s_Init[k].nDel;
		memset(&nl.ReloadParam, 0, sizeof(nl.ReloadParam));
	}
	memset(&m_InvParam, 0, sizeof(m_InvParam));
}

CDlgCollectSetting::~CDlgCollectSetting()
{
}

void CDlgCollectSetting::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_ITEMATTRIB_COMBO, m_ItemAttribCombo);
	DDX_Control(pDX, IDC_ITEMATTRIB_LIST, m_ItemAttribList);
	for (int k = 0; k < nl_count; k++)
	{
		DDX_Control(pDX, m_NameList[k].nListID, m_NameList[k].List);
		DDX_Control(pDX, m_NameList[k].nComboID, m_NameList[k].Combo);
	}

	if (g_pCurGameWin == NULL) return;

	//Checkboxes
	DDX_Check(pDX, IDC_PRICE_CHECK, g_pCurGameWin->m_prCollect.m_bCollectPrice);
	DDX_Check(pDX, IDC_LEVEL_CHECK, g_pCurGameWin->m_prCollect.m_bCollectLevel);

	//Editboxes
	DDX_Text(pDX, IDC_PRICE_EDIT, g_pCurGameWin->m_prCollect.m_nCollectPrice);
	DDX_Text(pDX, IDC_LEVEL_EDIT, g_pCurGameWin->m_prCollect.m_nCollectLevel);

	//UI Refresh
	BOOL bChecked;
	bChecked = ((CButton*)GetDlgItem(IDC_PRICE_CHECK))->GetCheck();
	GetDlgItem(IDC_PRICE_EDIT)->EnableWindow(!bChecked);
	bChecked = ((CButton*)GetDlgItem(IDC_LEVEL_CHECK))->GetCheck();
	GetDlgItem(IDC_LEVEL_EDIT)->EnableWindow(!bChecked);

	UpdateMagicOptions(pDX->m_bSaveAndValidate);
}

BEGIN_MESSAGE_MAP(CDlgCollectSetting, CDialog)
	ON_CONTROL_RANGE(BN_CLICKED, 1000, 2000, &CDlgCollectSetting::OnButtonsClicked)
	ON_CBN_SELCHANGE(IDC_ITEMATTRIB_COMBO, &CDlgCollectSetting::OnCbnSelchangeItemattribCombo)
	ON_LBN_SELCHANGE(IDC_ITEMATTRIB_LIST, &CDlgCollectSetting::OnLbnSelchangeItemattribList)
	ON_CBN_DROPDOWN(IDC_BLACKITEM_COMBO, &CDlgCollectSetting::OnCbnDropdownNameCombo)
	ON_CBN_DROPDOWN(IDC_RUONGITEM_COMBO, &CDlgCollectSetting::OnCbnDropdownNameCombo)
	ON_WM_COPYDATA()
END_MESSAGE_MAP()


// CDlgCollectSetting message handlers

BOOL CDlgCollectSetting::OnInitDialog()
{
	CDialog::OnInitDialog();

	for (int i = 0; i < ALL_ATTRIB; i++)
	{
		// Hut SL/NL, +ky nang, khong the pha huy, bo qua ne tranh: mac dinh luon giu (VLCollect),
		// nhung neu them vao danh sach thi dung nguong trong danh sach
		int nIndex = m_ItemAttribCombo.AddString(AllAttribs[i].szAttDesc);
		m_ItemAttribCombo.SetItemData(nIndex, AllAttribs[i].nAttVal);
	}
	// Combo thuoc tinh da co du lieu, nap lai danh sach loc
	if (g_pCurGameWin != NULL) UpdateMagicOptions(FALSE);

	// Danh sach khong nhat (Maps\BlackItems.txt), khong cat vao ruong (Maps\RuongItem.txt)
	for (int k = 0; k < nl_count; k++)
	{
		ITEM_NAME_LIST& nl = m_NameList[k];
		nl.List.SetExtendedStyle(LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);
		CRect rcList;
		nl.List.GetClientRect(&rcList);
		nl.List.InsertColumn(0, _T("Tên vật phẩm"), LVCFMT_LEFT, rcList.Width() - GetSystemMetrics(SM_CXVSCROLL));
		nl.Combo.LimitText(LIST_ITEM_LEN);
		LoadNameList(nl);
	}
	RequestInventoryNames();

	if (m_ToolTip.Create(this))
	{
		m_ToolTip.AddTool(GetDlgItem(IDC_ITEMATTRIB_COMBO), _T("Thiết lập thuộc tính lọc đồ"));
		m_ToolTip.AddTool(GetDlgItem(IDC_ITEMATTRIB_LIST), _T("Danh sách thuộc tính lọc đồ"));
		m_ToolTip.AddTool(GetDlgItem(IDC_ADD_BUTTON), _T("Thêm vào danh sách"));
		m_ToolTip.AddTool(GetDlgItem(IDC_DEL_BUTTON), _T("Xóa khỏi danh sách"));
		m_ToolTip.AddTool(GetDlgItem(IDC_PRICE_CHECK), _T("Giữ lại đồ giá cao (lượng)"));
		m_ToolTip.AddTool(GetDlgItem(IDC_PRICE_EDIT), _T("Giá giữ lại trên mỗi ô hành trang"));
		m_ToolTip.AddTool(GetDlgItem(IDC_LEVEL_CHECK), _T("Giữ lại đồ cấp cao"));
		m_ToolTip.AddTool(GetDlgItem(IDC_BLACKITEM_LIST), _T("Vật phẩm có tên chứa các chuỗi này sẽ không nhặt (dùng chung mọi nhân vật)"));
		m_ToolTip.AddTool(GetDlgItem(IDC_BLACKITEM_COMBO), _T("Chọn vật phẩm trong hành trang hoặc gõ tên (tối đa 31 ký tự)"));
		m_ToolTip.AddTool(GetDlgItem(IDC_BLACKADD_BUTTON), _T("Thêm vào danh sách không nhặt và lưu file"));
		m_ToolTip.AddTool(GetDlgItem(IDC_BLACKDEL_BUTTON), _T("Xóa dòng đang chọn và lưu file"));
		m_ToolTip.AddTool(GetDlgItem(IDC_RUONGITEM_LIST), _T("Vật phẩm có tên chứa các chuỗi này sẽ không cất vào rương (dùng chung mọi nhân vật)"));
		m_ToolTip.AddTool(GetDlgItem(IDC_RUONGITEM_COMBO), _T("Chọn vật phẩm trong hành trang hoặc gõ tên (tối đa 31 ký tự)"));
		m_ToolTip.AddTool(GetDlgItem(IDC_RUONGADD_BUTTON), _T("Thêm vào danh sách không cất rương và lưu file"));
		m_ToolTip.AddTool(GetDlgItem(IDC_RUONGDEL_BUTTON), _T("Xóa dòng đang chọn và lưu file"));
	}

	return TRUE;
}

BOOL CDlgCollectSetting::PreTranslateMessage(MSG* pMsg)
{
	m_ToolTip.RelayEvent(pMsg);
	return CDialog::PreTranslateMessage(pMsg);
}

void CDlgCollectSetting::OnOK()
{
	if (g_pCurGameWin != NULL)
	{
		UpdateData();
		g_pCurGameWin->SetGameParams(tab_collect);
	}
	EndDialog(IDOK);
}

void CDlgCollectSetting::OnCancel()
{
	// Thay doi duoc ap dung ngay, dong cua so cung luu nhu nut Dong
	OnOK();
}

void CDlgCollectSetting::OnButtonsClicked(UINT nID)
{
	// Danh sach khong nhat dung chung, khong can chon nhan vat
	for (int k = 0; k < nl_count; k++)
	{
		if (nID == m_NameList[k].nAddID) { OnBnClickedNameAdd(m_NameList[k]); return; }
		if (nID == m_NameList[k].nDelID) { OnBnClickedNameDel(m_NameList[k]); return; }
	}

	if (g_pCurGameWin == NULL) return;

	CButton *pButton = (CButton*)GetDlgItem(nID);
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
		if (pButton->GetCheck() && g_pCurGameWin->m_prCollect.m_nCollectEquip != 2)
		{
			CString szMessage = _T("Nhân vật sẽ vứt bỏ Vũ khí và Trang bị\n");
			szMessage += _T("nếu giá bán Shop không lớn hơn mức thiết lập.\n\n");
			szMessage += _T("Bạn đã đọc kỹ Hướng dẫn cho mục này chưa?");
			int nRet = MessageBox(szMessage, _T("VLAuto - Xác nhận"), MB_YESNO|MB_ICONQUESTION);
			if (nRet != IDYES) pButton->SetCheck(0);
		}
		break;
	case IDC_LEVEL_CHECK:
		if (pButton->GetCheck() && g_pCurGameWin->m_prCollect.m_nCollectEquip != 2)
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

void CDlgCollectSetting::UpdateMagicOptions(BOOL bSave)
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

void CDlgCollectSetting::OnCbnSelchangeItemattribCombo()
{
	SetDlgItemInt(IDC_ITEMATTRIB_EDIT, 0);
	GetDlgItem(IDC_ITEMATTRIB_EDIT)->SetFocus();
}

void CDlgCollectSetting::OnLbnSelchangeItemattribList()
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

void CDlgCollectSetting::OnBnClickedAddButton()
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

void CDlgCollectSetting::OnBnClickedDelButton()
{
	m_ItemAttribList.DeleteString(m_ItemAttribList.GetCurSel());
}

void CDlgCollectSetting::OnBnClickedSaveButton()
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

void CDlgCollectSetting::OnBnClickedLoadButton()
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

//=========================== BlackItems.txt / RuongItem.txt ===========================
// File luu ma TCVN3, dong bat dau '#' la chu thich; VLHook so khop chuoi con, dung doc o dong trong

static void GetNameListFile(const ITEM_NAME_LIST& nl, CString& szFileName)
{
	szFileName.Format(_T("%s\\Maps\\%s"), g_szAppFolder, nl.szFile);
}

void CDlgCollectSetting::LoadNameList(ITEM_NAME_LIST& nl)
{
	nl.List.DeleteAllItems();
	nl.arrHeader.RemoveAll();

	CString szFileName;
	GetNameListFile(nl, szFileName);
	CFile file;
	if (!file.Open(szFileName, CFile::modeRead | CFile::shareDenyNone))
		return;
	UINT nSize = (UINT)file.GetLength();
	CStringA strData;
	if (nSize > 0)
	{
		file.Read(strData.GetBuffer(nSize), nSize);
		strData.ReleaseBuffer(nSize);
	}
	file.Close();

	int nPos = 0, nCount = 0;
	while (nPos < strData.GetLength())
	{
		int nEnd = strData.Find('\n', nPos);
		if (nEnd < 0) nEnd = strData.GetLength();
		CStringA strLine = strData.Mid(nPos, nEnd - nPos);
		nPos = nEnd + 1;
		strLine.TrimRight("\r");

		TCHAR szLine[256];
		TCVN3ToUnicode(strLine, szLine, 255);
		if (strLine.GetLength() > 0 && strLine[0] == '#')
		{
			// Chi giu chu thich dau file (truoc dong ten dau tien)
			if (nCount == 0) nl.arrHeader.Add(szLine);
			continue;
		}
		if (strLine.IsEmpty()) continue;
		nl.List.InsertItem(nCount++, szLine);
	}
}

BOOL CDlgCollectSetting::SaveNameList(ITEM_NAME_LIST& nl)
{
	CString szFileName;
	GetNameListFile(nl, szFileName);
	CFile file;
	if (!file.Open(szFileName, CFile::modeCreate | CFile::modeWrite))
	{
		CString szMessage;
		szMessage.Format(_T("Lỗi: không thể ghi tập tin Maps\\%s!"), nl.szFile);
		MessageBox(szMessage, 0, MB_ICONEXCLAMATION);
		return FALSE;
	}

	CStringA strData;
	char szLine[512];
	for (int i = 0; i < nl.arrHeader.GetCount(); i++)
	{
		UnicodeToTCVN3(nl.arrHeader[i], szLine, sizeof(szLine) - 1);
		strData += szLine;
		strData += "\r\n";
	}
	int nCount = nl.List.GetItemCount();
	for (int i = 0; i < nCount; i++)
	{
		UnicodeToTCVN3(nl.List.GetItemText(i, 0), szLine, LIST_ITEM_LEN);
		strData += szLine;
		strData += "\r\n";
	}
	file.Write((LPCSTR)strData, strData.GetLength());
	file.Close();

	ReloadNameListInGame(nl);
	return TRUE;
}

void CDlgCollectSetting::ReloadNameListInGame(ITEM_NAME_LIST& nl)
{
	// VLHook: dbg_cl_blacklist / dbg_cl_nosavelist nap lai file; khong can tra loi (hWndReply = NULL)
	memset(&nl.ReloadParam, 0, sizeof(nl.ReloadParam));
	nl.ReloadParam.cbSize = sizeof(DEBUG_PARAM);
	nl.ReloadParam.nCmd = nl.nReloadCmd;

	int nCharCount = g_pCharList ? g_pCharList->GetItemCount() : 0;
	for (int i = 0; i < nCharCount; i++)
	{
		GameWindow* pGameWin = (GameWindow*)g_pCharList->GetItemData(i);
		if (!pGameWin || !pGameWin->m_hWnd || !pGameWin->m_bAttached || !::IsWindow(pGameWin->m_hWnd)) continue;
		::PostMessage(pGameWin->m_hWnd, WM_HOOK_DEBUG, (WPARAM)&nl.ReloadParam, sizeof(DEBUG_PARAM));
	}
}

void CDlgCollectSetting::RequestInventoryNames()
{
	if (g_pCurGameWin == NULL || !g_pCurGameWin->m_hWnd || !g_pCurGameWin->m_bAttached || !::IsWindow(g_pCurGameWin->m_hWnd))
		return;
	memset(&m_InvParam, 0, sizeof(m_InvParam));
	m_InvParam.cbSize = sizeof(DEBUG_PARAM);
	m_InvParam.hWndReply = GetSafeHwnd();
	m_InvParam.nCmd = dbg_cl_invnames;
	::PostMessage(g_pCurGameWin->m_hWnd, WM_HOOK_DEBUG, (WPARAM)&m_InvParam, sizeof(DEBUG_PARAM));
}

void CDlgCollectSetting::OnCbnDropdownNameCombo()
{
	RequestInventoryNames();
}

BOOL CDlgCollectSetting::OnCopyData(CWnd* pWnd, COPYDATASTRUCT* pCopyDataStruct)
{
	if (pCopyDataStruct->dwData != DEBUG_COPYDATA_ID)
		return CDialog::OnCopyData(pWnd, pCopyDataStruct);

	static DEBUG_REPLY reply;
	DWORD cbHeader = offsetof(DEBUG_REPLY, szText);
	if (pCopyDataStruct->cbData < cbHeader || pCopyDataStruct->cbData > sizeof(DEBUG_REPLY) || !pCopyDataStruct->lpData)
		return TRUE;
	memset(&reply, 0, sizeof(reply));
	memcpy(&reply, pCopyDataStruct->lpData, pCopyDataStruct->cbData);
	reply.szText[DEBUG_TEXT_SIZE - 1] = 0;
	if (reply.nCmd != dbg_cl_invnames || !reply.bOK)
		return TRUE;

	// Nap lai ten vat pham trong hanh trang vao ca 2 combo, giu chu dang go
	CString szCurText[nl_count];
	for (int k = 0; k < nl_count; k++)
	{
		m_NameList[k].Combo.GetWindowText(szCurText[k]);
		m_NameList[k].Combo.ResetContent();
	}
	char *pLine = reply.szText;
	while (*pLine)
	{
		char *pEnd = strchr(pLine, '\n');
		if (pEnd) *pEnd = 0;
		if (*pLine)
		{
			TCHAR szName[64];
			TCVN3ToUnicode(pLine, szName, LIST_ITEM_LEN);
			for (int k = 0; k < nl_count; k++)
			{
				if (m_NameList[k].Combo.FindStringExact(-1, szName) < 0)
					m_NameList[k].Combo.AddString(szName);
			}
		}
		if (!pEnd) break;
		pLine = pEnd + 1;
	}
	for (int k = 0; k < nl_count; k++)
		m_NameList[k].Combo.SetWindowText(szCurText[k]);
	return TRUE;
}

void CDlgCollectSetting::OnBnClickedNameAdd(ITEM_NAME_LIST& nl)
{
	CString szName;
	nl.Combo.GetWindowText(szName);
	szName.Trim();
	if (szName.IsEmpty())
	{
		MessageBox(_T("Chọn vật phẩm trong danh sách hoặc gõ tên cần thêm."), _T("VLAuto"), MB_ICONINFORMATION);
		nl.Combo.SetFocus();
		return;
	}
	if (szName[0] == _T('#'))
	{
		MessageBox(_T("Tên không được bắt đầu bằng dấu #."), _T("VLAuto"), MB_ICONEXCLAMATION);
		return;
	}
	szName = szName.Left(LIST_ITEM_LEN);

	int nCount = nl.List.GetItemCount();
	for (int i = 0; i < nCount; i++)
	{
		if (nl.List.GetItemText(i, 0).CompareNoCase(szName) == 0)
		{
			nl.List.SetItemState(i, LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED);
			nl.List.EnsureVisible(i, FALSE);
			return;
		}
	}
	if (nCount >= MAX_LIST_ITEMS_UI)
	{
		CString szMessage;
		szMessage.Format(_T("Danh sách đã đủ %d vật phẩm, hãy xóa bớt trước khi thêm."), MAX_LIST_ITEMS_UI);
		MessageBox(szMessage, _T("VLAuto"), MB_ICONEXCLAMATION);
		return;
	}

	int nIndex = nl.List.InsertItem(nCount, szName);
	if (!SaveNameList(nl))
	{
		nl.List.DeleteItem(nIndex);
		return;
	}
	nl.List.SetItemState(nIndex, LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED);
	nl.List.EnsureVisible(nIndex, FALSE);
	nl.Combo.SetWindowText(_T(""));
}

void CDlgCollectSetting::OnBnClickedNameDel(ITEM_NAME_LIST& nl)
{
	int nIndex = nl.List.GetNextItem(-1, LVNI_SELECTED);
	if (nIndex < 0)
	{
		MessageBox(_T("Chọn 1 vật phẩm trong danh sách để xóa."), _T("VLAuto"), MB_ICONINFORMATION);
		return;
	}
	nl.List.DeleteItem(nIndex);
	SaveNameList(nl);
	int nCount = nl.List.GetItemCount();
	if (nCount > 0)
	{
		if (nIndex >= nCount) nIndex = nCount - 1;
		nl.List.SetItemState(nIndex, LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED);
	}
}
