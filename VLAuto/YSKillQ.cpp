// YSKillQ.cpp : implementation file
//

#include "stdafx.h"
#include "VLAuto.h"
#include "../GameDef.h"
#include "GameCore.h"
#include "GameWindow.h"
#include "DlgYeSou.h"
#include "YSKillQ.h"

// CYSKillQ dialog

IMPLEMENT_DYNAMIC(CYSKillQ, CPropertyPage)

CYSKillQ::CYSKillQ()
	: CPropertyPage(CYSKillQ::IDD)
{
	m_pKillQOpt = NULL;
	m_nIniMaps = 0;
	m_nCurMap = -1;
	m_bFilling = FALSE;
}

CYSKillQ::~CYSKillQ()
{
}

void CYSKillQ::DoDataExchange(CDataExchange* pDX)
{
	DDX_Control(pDX, IDC_QUEST_MAP_LIST, m_MapList);
	DDX_Control(pDX, IDC_QUEST_MOVEPOINT_LIST, m_PointList);

	CPropertyPage::DoDataExchange(pDX);
	if (!m_pKillQOpt) return;

	DDX_Check(pDX, IDC_QUEST_LONG_CHECK, m_pKillQOpt->m_bLongCheck);
	DDX_Check(pDX, IDC_QUEST_CANCEL_LONG_CHECK, m_pKillQOpt->m_bCancelLong);
	DDX_Check(pDX, IDC_QUEST_CANCEL_NOMAP_CHECK, m_pKillQOpt->m_bCancelNoMap);
	DDX_Check(pDX, IDC_QUEST_ATTACK_USE_ITEM_CHECK, m_pKillQOpt->m_bCancelUseItem);
	DDX_Text(pDX, IDC_QUEST_CHECK_TIME_EDIT, m_pKillQOpt->m_nCheckTime);
	DDV_MinMaxInt(pDX, m_pKillQOpt->m_nCheckTime, 1, 600);
	DDX_Text(pDX, IDC_QUEST_CANCEL_TIME_EDIT, m_pKillQOpt->m_nCancelTime);
	DDV_MinMaxInt(pDX, m_pKillQOpt->m_nCancelTime, 1, 600);

	GetDlgItem(IDC_QUEST_CHECK_TIME_EDIT)->EnableWindow(m_pKillQOpt->m_bLongCheck);
	GetDlgItem(IDC_QUEST_CANCEL_TIME_EDIT)->EnableWindow(m_pKillQOpt->m_bCancelLong);
}

BEGIN_MESSAGE_MAP(CYSKillQ, CPropertyPage)
	ON_CONTROL_RANGE(BN_CLICKED, 1000, 2000, &CYSKillQ::OnButtonsClicked)
	ON_NOTIFY(LVN_ITEMCHANGED, IDC_QUEST_MAP_LIST, &CYSKillQ::OnMapListChanged)
	ON_LBN_SELCHANGE(IDC_QUEST_MOVEPOINT_LIST, &CYSKillQ::OnPointSelChange)
	ON_EN_CHANGE(IDC_QUEST_RANGESEARCH_EDIT, &CYSKillQ::OnRangeChange)
END_MESSAGE_MAP()

// CYSKillQ message handlers

BOOL CYSKillQ::OnInitDialog()
{
	CPropertyPage::OnInitDialog();

	m_MapList.SetExtendedStyle(LVS_EX_GRIDLINES|LVS_EX_FULLROWSELECT|LVS_EX_CHECKBOXES);
	m_MapList.InsertColumn(0, _T("STT"), LVCFMT_LEFT, 42);
	m_MapList.InsertColumn(1, _T("Bản đồ"), LVCFMT_LEFT, 140);

	if (m_ToolTip.Create(this))
	{
		m_ToolTip.AddTool(GetDlgItem(IDC_QUEST_MAP_LIST), _T("Các map nhiệm vụ đánh quái trong Maps\\MapDaTau.ini - bỏ chọn = hủy nhiệm vụ ở map đó"));
		m_ToolTip.AddTool(GetDlgItem(IDC_QUEST_MOVEPOINT_LIST), _T("Các tọa độ đi tìm quái nhiệm vụ trên map đang chọn (đi lần lượt, hết thì quay lại điểm đầu)"));
		m_ToolTip.AddTool(GetDlgItem(IDC_QUEST_ADD_BUTTON), _T("Thêm tọa độ hiện tại của nhân vật"));
		m_ToolTip.AddTool(GetDlgItem(IDC_QUEST_DEL_BUTTON), _T("Xóa tọa độ đang chọn"));
		m_ToolTip.AddTool(GetDlgItem(IDC_QUEST_UP_BUTTON), _T("Chuyển tọa độ đang chọn lên trên"));
		m_ToolTip.AddTool(GetDlgItem(IDC_QUEST_DOWN_BUTTON), _T("Chuyển tọa độ đang chọn xuống dưới"));
		m_ToolTip.AddTool(GetDlgItem(IDC_QUEST_DELALL_BUTTON), _T("Xóa hết danh sách"));
		m_ToolTip.AddTool(GetDlgItem(IDC_QUEST_SAVE_BUTTON), _T("Ghi danh sách ra file .mvp"));
		m_ToolTip.AddTool(GetDlgItem(IDC_QUEST_LOAD_BUTTON), _T("Đọc danh sách từ file .mvp"));
		m_ToolTip.AddTool(GetDlgItem(IDC_QUEST_NODEX_EDIT), _T("Tọa độ X tuyệt đối của dòng đang chọn"));
		m_ToolTip.AddTool(GetDlgItem(IDC_QUEST_NODEY_EDIT), _T("Tọa độ Y tuyệt đối của dòng đang chọn"));
		m_ToolTip.AddTool(GetDlgItem(IDC_QUEST_EDITNODE_BUTTON), _T("Sửa dòng đang chọn theo X, Y đã nhập"));
		m_ToolTip.AddTool(GetDlgItem(IDC_QUEST_RANGESEARCH_EDIT), _T("Tầm quét quái nhiệm vụ quanh mỗi tọa độ"));
		m_ToolTip.AddTool(GetDlgItem(IDC_QUEST_SAVE_DEFAULT_BUTTON), _T("Ghi danh sách + tầm quét làm mặc định của map này vào MapDaTau.ini"));
		m_ToolTip.AddTool(GetDlgItem(IDC_QUEST_LOAD_DEFAULT_BUTTON), _T("Xóa danh sách, nạp lại tọa độ + tầm quét mặc định trong MapDaTau.ini"));
		m_ToolTip.AddTool(GetDlgItem(IDC_QUEST_LONG_CHECK), _T("Cứ sau N phút kiểm tra lại tình hình nhiệm vụ (sửa lỗi đọc sai tiến độ, về thành quá nhiều lần)"));
		m_ToolTip.AddTool(GetDlgItem(IDC_QUEST_CANCEL_LONG_CHECK), _T("Sau N phút mà chưa giết được 50% số quái thì về Dã Tẩu hủy nhiệm vụ"));
		m_ToolTip.AddTool(GetDlgItem(IDC_QUEST_CANCEL_NOMAP_CHECK), _T("Map nhiệm vụ không có trong MapDaTau.ini thì hủy và ghi vào nhật ký nhiệm vụ"));
	}

	LoadIni();
	FillMapList();
	UpdateData(0);
	return TRUE;
}

BOOL CYSKillQ::PreTranslateMessage(MSG* pMsg)
{
	m_ToolTip.RelayEvent(pMsg);
	return CPropertyPage::PreTranslateMessage(pMsg);
}

static int ParseInt(LPCTSTR& p)
{
	while (*p == ' ' || *p == '\t') p++;
	int n = _ttoi(p);
	while (*p == '-' || (*p >= '0' && *p <= '9')) p++;
	return n;
}

// Doc MapDaTau.ini (UTF-8, co the co BOM) - khong dung GetPrivateProfileString vi ten map la UTF-8
void CYSKillQ::LoadIni()
{
	for (int i = 0; i < MAX_KQ_MAP; i++)
		m_IniMaps[i].Pts.RemoveAll();
	m_nIniMaps = 0;
	m_szIniFile.Format(_T("%s\\Maps\\MapDaTau.ini"), g_szAppFolder);

	CFile iniFile;
	if (!iniFile.Open(m_szIniFile, CFile::modeRead | CFile::shareDenyNone))
		return;
	UINT uLen = (UINT)iniFile.GetLength();
	CStringA szRaw;
	char* pBuf = szRaw.GetBuffer(uLen + 1);
	uLen = iniFile.Read(pBuf, uLen);
	pBuf[uLen] = 0;
	szRaw.ReleaseBuffer(uLen);
	iniFile.Close();
	if (szRaw.GetLength() >= 3 && (BYTE)szRaw[0] == 0xEF && (BYTE)szRaw[1] == 0xBB && (BYTE)szRaw[2] == 0xBF)
		szRaw = szRaw.Mid(3);

	int nLen = MultiByteToWideChar(CP_UTF8, 0, szRaw, -1, NULL, 0);
	CString szText;
	MultiByteToWideChar(CP_UTF8, 0, szRaw, -1, szText.GetBuffer(nLen), nLen);
	szText.ReleaseBuffer();

	KQIniMap* pMap = NULL;
	int nPos = 0;
	CString szLine = szText.Tokenize(_T("\n"), nPos);
	while (nPos >= 0)
	{
		szLine.Trim();
		if (szLine.GetLength() > 2 && szLine[0] == '[')
		{
			pMap = NULL;
			if (m_nIniMaps < MAX_KQ_MAP)
			{
				pMap = &m_IniMaps[m_nIniMaps];
				pMap->nSect = _ttoi((LPCTSTR)szLine + 1);
				pMap->nMapID = 0;
				pMap->nRange = 500;
				pMap->szName.Empty();
				pMap->Pts.RemoveAll();
			}
		}
		else if (pMap)
		{
			int nEq = szLine.Find('=');
			CString szKey = nEq > 0 ? szLine.Left(nEq).Trim() : _T("");
			CString szVal = nEq > 0 ? szLine.Mid(nEq + 1).Trim() : _T("");
			if (!szKey.CompareNoCase(_T("Map")))
			{
				pMap->nMapID = _ttoi(szVal);
				int nUnder = szVal.Find('_');
				pMap->szName = nUnder >= 0 ? szVal.Mid(nUnder + 1) : szVal;
				if (pMap->nMapID > 0) m_nIniMaps++;	// Section hop le khi co Map=
			}
			else if (!szKey.CompareNoCase(_T("AttackRange")))
				pMap->nRange = _ttoi(szVal);
			else if (!szKey.CompareNoCase(_T("Points")))
			{
				LPCTSTR p = szVal;
				while (*p && pMap->Pts.GetCount() < MAX_KQ_POINT)
				{
					POINT pt;
					pt.x = ParseInt(p);
					while (*p == ' ' || *p == ',') p++;
					pt.y = ParseInt(p);
					if (pt.x > 0 && pt.y > 0) pMap->Pts.Add(pt);
					while (*p && *p != ';') p++;
					if (*p == ';') p++;
				}
			}
		}
		szLine = szText.Tokenize(_T("\n"), nPos);
	}
}

void CYSKillQ::FillMapList()
{
	m_bFilling = TRUE;
	m_MapList.DeleteAllItems();
	for (int i = 0; i < m_nIniMaps; i++)
	{
		KQIniMap& im = m_IniMaps[i];
		CString szText;
		szText.Format(_T("%d"), im.nSect);
		int nItem = m_MapList.InsertItem(i, szText);
		szText.Format(_T("%s (%d)"), (LPCTSTR)im.szName, im.nMapID);
		m_MapList.SetItemText(nItem, 1, szText);
		m_MapList.SetItemData(nItem, i);
		KQMapOpt* pOpt = m_pKillQOpt ? m_pKillQOpt->FindMap(im.nMapID, FALSE) : NULL;
		m_MapList.SetCheck(nItem, !(pOpt && pOpt->bDisable));
	}
	m_bFilling = FALSE;

	// Chon san map nhan vat dang dung (neu co), khong thi dong dau
	int nSel = 0;
	if (g_pCurGameWin)
		for (int i = 0; i < m_nIniMaps; i++)
			if (m_IniMaps[i].nMapID == g_pCurGameWin->m_nMapID) nSel = i;
	if (m_nIniMaps)
	{
		m_MapList.SetItemState(nSel, LVIS_SELECTED|LVIS_FOCUSED, LVIS_SELECTED|LVIS_FOCUSED);
		m_MapList.EnsureVisible(nSel, FALSE);
	}
	ShowMap(m_nIniMaps ? nSel : -1);
}

KQIniMap* CYSKillQ::CurMap()
{
	if (m_nCurMap < 0 || m_nCurMap >= m_nIniMaps) return NULL;
	return &m_IniMaps[m_nCurMap];
}

// Hien danh sach toa do + tam quet cua map: cfg da sua thi lay cfg, khong thi mac dinh ini
void CYSKillQ::ShowMap(int nIndex)
{
	m_nCurMap = nIndex;
	KQIniMap* pMap = CurMap();
	m_bFilling = TRUE;
	m_PointList.ResetContent();
	m_Pts.RemoveAll();
	BOOL bEnable = pMap != NULL && m_pKillQOpt != NULL;
	if (bEnable)
	{
		KQMapOpt* pOpt = m_pKillQOpt->FindMap(pMap->nMapID, FALSE);
		if (pOpt && (pOpt->nFlags & KQF_POINTS))
		{
			for (int i = 0; i < pOpt->nPoints && i < MAX_KQ_POINT; i++)
				InsertPoint(-1, pOpt->Pts[i]);
		}
		else
		{
			for (int i = 0; i < pMap->Pts.GetCount(); i++)
				InsertPoint(-1, pMap->Pts[i]);
		}
		int nRange = (pOpt && (pOpt->nFlags & KQF_RANGE)) ? pOpt->nRange : pMap->nRange;
		SetDlgItemInt(IDC_QUEST_RANGESEARCH_EDIT, nRange);
		m_PointList.SetCurSel(m_PointList.GetCount() ? 0 : -1);
	}
	else
		SetDlgItemText(IDC_QUEST_RANGESEARCH_EDIT, _T(""));
	m_bFilling = FALSE;

	UINT uCtrls[] = { IDC_QUEST_MOVEPOINT_LIST, IDC_QUEST_ADD_BUTTON, IDC_QUEST_DEL_BUTTON, IDC_QUEST_UP_BUTTON,
		IDC_QUEST_DOWN_BUTTON, IDC_QUEST_DELALL_BUTTON, IDC_QUEST_SAVE_BUTTON, IDC_QUEST_LOAD_BUTTON,
		IDC_QUEST_NODEX_EDIT, IDC_QUEST_NODEY_EDIT, IDC_QUEST_EDITNODE_BUTTON, IDC_QUEST_RANGESEARCH_EDIT,
		IDC_QUEST_SAVE_DEFAULT_BUTTON, IDC_QUEST_LOAD_DEFAULT_BUTTON };
	for (int i = 0; i < _countof(uCtrls); i++)
		GetDlgItem(uCtrls[i])->EnableWindow(bEnable);
	ShowSelNode();
}

void CYSKillQ::OnMapListChanged(NMHDR* pNMHDR, LRESULT* pResult)
{
	LPNMLISTVIEW pNMLV = reinterpret_cast<LPNMLISTVIEW>(pNMHDR);
	*pResult = 0;
	if (m_bFilling || !m_pKillQOpt || pNMLV->iItem < 0 || pNMLV->iItem >= m_nIniMaps) return;

	// Doi checkbox => bat/tat map
	if ((pNMLV->uChanged & LVIF_STATE) && ((pNMLV->uNewState ^ pNMLV->uOldState) & LVIS_STATEIMAGEMASK)
		&& (pNMLV->uOldState & LVIS_STATEIMAGEMASK))
	{
		BOOL bCheck = ((pNMLV->uNewState & LVIS_STATEIMAGEMASK) >> 12) == 2;
		KQMapOpt* pOpt = m_pKillQOpt->FindMap(m_IniMaps[pNMLV->iItem].nMapID, !bCheck);
		if (pOpt) pOpt->bDisable = !bCheck;
		else if (!bCheck)
		{
			MessageBox(_T("Đã đủ số map cài đặt riêng tối đa!"), _T("VLAuto"), MB_ICONEXCLAMATION);
			m_bFilling = TRUE;
			m_MapList.SetCheck(pNMLV->iItem, TRUE);
			m_bFilling = FALSE;
		}
	}
	// Doi dong chon => hien toa do cua map do
	if ((pNMLV->uChanged & LVIF_STATE) && (pNMLV->uNewState & LVIS_SELECTED) && !(pNMLV->uOldState & LVIS_SELECTED))
		ShowMap((int)m_MapList.GetItemData(pNMLV->iItem));
}

void CYSKillQ::OnPointSelChange()
{
	ShowSelNode();
}

void CYSKillQ::OnRangeChange()
{
	if (m_bFilling || !m_pKillQOpt) return;
	KQIniMap* pMap = CurMap();
	if (!pMap) return;
	BOOL bOk;
	int nRange = (int)GetDlgItemInt(IDC_QUEST_RANGESEARCH_EDIT, &bOk, FALSE);
	if (!bOk || nRange <= 0) return;
	KQMapOpt* pOpt = m_pKillQOpt->FindMap(pMap->nMapID, TRUE);
	if (!pOpt) return;
	pOpt->nRange = nRange;
	pOpt->nFlags |= KQF_RANGE;
}

void CYSKillQ::OnButtonsClicked(UINT nID)
{
	if (!m_pKillQOpt) return;
	switch (nID)
	{
	case IDC_QUEST_ADD_BUTTON:		OnAdd(); break;
	case IDC_QUEST_DEL_BUTTON:		OnDel(); break;
	case IDC_QUEST_UP_BUTTON:		OnMove(-1); break;
	case IDC_QUEST_DOWN_BUTTON:		OnMove(1); break;
	case IDC_QUEST_DELALL_BUTTON:	OnDelAll(); break;
	case IDC_QUEST_EDITNODE_BUTTON:	if (!OnEditNode()) return; break;
	case IDC_QUEST_SAVE_BUTTON:		OnSaveFile(); return;
	case IDC_QUEST_LOAD_BUTTON:		OnLoadFile(); break;
	case IDC_QUEST_SAVE_DEFAULT_BUTTON:	OnSaveDefault(); return;
	case IDC_QUEST_LOAD_DEFAULT_BUTTON:	OnLoadDefault(); return;
	case IDC_QUEST_LONG_CHECK:
	case IDC_QUEST_CANCEL_LONG_CHECK:
	case IDC_QUEST_CANCEL_NOMAP_CHECK:
		UpdateData();
		return;
	default:
		return;
	}
	SavePoints();
	ShowSelNode();
}

// Moi dong giu chi so toa do tuyet doi trong m_Pts (ItemData), hien thi "[x, y] - X, Y" giong Danh sach toa do luyen cong
void CYSKillQ::InsertPoint(int nIndex, POINT pt)
{
	CString szLine;
	szLine.Format(_T("[%d, %d] - %d, %d"), pt.x>>8, pt.y>>9, pt.x, pt.y);
	nIndex = m_PointList.InsertString(nIndex, szLine);
	m_PointList.SetItemData(nIndex, (DWORD_PTR)m_Pts.Add(pt));
	m_PointList.SetCurSel(nIndex);
}

POINT CYSKillQ::GetPoint(int nIndex)
{
	return m_Pts[(INT_PTR)m_PointList.GetItemData(nIndex)];
}

void CYSKillQ::ShowSelNode()
{
	int nIndex = m_PointList.GetCurSel();
	if (nIndex < 0)
	{
		SetDlgItemText(IDC_QUEST_NODEX_EDIT, _T(""));
		SetDlgItemText(IDC_QUEST_NODEY_EDIT, _T(""));
		return;
	}
	POINT pt = GetPoint(nIndex);
	SetDlgItemInt(IDC_QUEST_NODEX_EDIT, pt.x, FALSE);
	SetDlgItemInt(IDC_QUEST_NODEY_EDIT, pt.y, FALSE);
}

// Danh sach tren man hinh => cfg nhan vat (map nay thanh "da sua")
void CYSKillQ::SavePoints()
{
	KQIniMap* pMap = CurMap();
	if (!pMap || !m_pKillQOpt) return;
	KQMapOpt* pOpt = m_pKillQOpt->FindMap(pMap->nMapID, TRUE);
	if (!pOpt)
	{
		MessageBox(_T("Đã đủ số map cài đặt riêng tối đa!"), _T("VLAuto"), MB_ICONEXCLAMATION);
		return;
	}
	int nCount = m_PointList.GetCount();
	pOpt->nPoints = 0;
	memset(pOpt->Pts, 0, sizeof(pOpt->Pts));
	for (int i = 0; i < nCount && i < MAX_KQ_POINT; i++)
		pOpt->Pts[pOpt->nPoints++] = GetPoint(i);
	pOpt->nFlags |= KQF_POINTS;
}

void CYSKillQ::OnAdd()
{
	KQIniMap* pMap = CurMap();
	if (!pMap || !g_pCurGameWin) return;
	if (g_pCurGameWin->m_nMapID != pMap->nMapID)
	{
		CString szMessage;
		szMessage.Format(_T("Nhân vật không ở map %s (%d).\nVẫn thêm tọa độ hiện tại vào danh sách của map này?"),
			(LPCTSTR)pMap->szName, pMap->nMapID);
		if (MessageBox(szMessage, _T("VLAuto"), MB_YESNO|MB_ICONQUESTION) != IDYES)
			return;
	}
	if (m_PointList.GetCount() >= MAX_KQ_POINT)
	{
		MessageBox(_T("Danh sách đã đủ số tọa độ tối đa!"), _T("VLAuto"), MB_ICONEXCLAMATION);
		return;
	}
	POINT pt = { g_pCurGameWin->m_nOffX, g_pCurGameWin->m_nOffY };
	InsertPoint(-1, pt);
}

void CYSKillQ::OnDel()
{
	int nIndex = m_PointList.GetCurSel();
	if (nIndex < 0) return;
	m_PointList.DeleteString(nIndex);
	if (nIndex >= m_PointList.GetCount()) nIndex--;
	m_PointList.SetCurSel(nIndex);
}

void CYSKillQ::OnMove(int nDir)
{
	int nIndex = m_PointList.GetCurSel();
	int nNew = nIndex + nDir;
	if (nIndex < 0 || nNew < 0 || nNew >= m_PointList.GetCount()) return;
	CString szTemp;
	m_PointList.GetText(nIndex, szTemp);
	DWORD_PTR dwTemp = m_PointList.GetItemData(nIndex);
	m_PointList.DeleteString(nIndex);
	m_PointList.InsertString(nNew, szTemp);
	m_PointList.SetItemData(nNew, dwTemp);
	m_PointList.SetCurSel(nNew);
}

void CYSKillQ::OnDelAll()
{
	if (m_PointList.GetCount() &&
		MessageBox(_T("Xóa hết danh sách tọa độ làm nhiệm vụ của map này?"), _T("VLAuto"), MB_YESNO|MB_ICONQUESTION) != IDYES)
		return;
	m_PointList.ResetContent();
}

BOOL CYSKillQ::OnEditNode()
{
	int nIndex = m_PointList.GetCurSel();
	if (nIndex < 0)
	{
		MessageBox(_T("Chọn 1 dòng trong danh sách để sửa!"), _T("VLAuto"), MB_ICONEXCLAMATION);
		return FALSE;
	}
	BOOL bOkX, bOkY;
	POINT pt;
	pt.x = (int)GetDlgItemInt(IDC_QUEST_NODEX_EDIT, &bOkX, FALSE);
	pt.y = (int)GetDlgItemInt(IDC_QUEST_NODEY_EDIT, &bOkY, FALSE);
	if (!bOkX || !bOkY || pt.x <= 0 || pt.y <= 0)
	{
		MessageBox(_T("Tọa độ X, Y không hợp lệ!"), _T("VLAuto"), MB_ICONEXCLAMATION);
		return FALSE;
	}
	m_PointList.DeleteString(nIndex);
	InsertPoint(nIndex, pt);
	return TRUE;
}

// Cung dinh dang file .mvp cua Danh sach toa do luyen cong (doc qua lai duoc)
void CYSKillQ::OnSaveFile()
{
	KQIniMap* pMap = CurMap();
	if (!pMap) return;
	CFileDialog fDlg(FALSE, _T(".mvp"), 0,
		OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT, _T("Move Points File (*.mvp)|*.mvp|"), this);
	if (fDlg.DoModal() == IDCANCEL)
		return;
	CFile mvFile;
	if (!mvFile.Open(fDlg.GetPathName(), CFile::modeCreate|CFile::modeWrite))
	{
		MessageBox(_T("Lỗi: không thể ghi tập tin!"), 0, MB_ICONEXCLAMATION);
		return;
	}
	char szVersion[12] = "";
	strcpy(szVersion, _SZCURVERSION);
	POINT Nodes[MAX_NODES];
	memset(Nodes, 0, sizeof(Nodes));
	for (int i = 0; i < m_PointList.GetCount() && i < MAX_NODES; i++)
		Nodes[i] = GetPoint(i);
	mvFile.Write(szVersion, sizeof(szVersion));
	mvFile.Write(Nodes, sizeof(Nodes));
	mvFile.Write(&pMap->nMapID, sizeof(int));
	mvFile.Close();
}

void CYSKillQ::OnLoadFile()
{
	KQIniMap* pMap = CurMap();
	if (!pMap) return;
	CFileDialog fDlg(TRUE, _T(".mvp"), 0,
		OFN_HIDEREADONLY, _T("Move Points File (*.mvp)|*.mvp|"), this);
	if (fDlg.DoModal() == IDCANCEL)
		return;
	CFile mvFile;
	if (!mvFile.Open(fDlg.GetPathName(), CFile::modeRead | CFile::shareDenyWrite))
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
	POINT Nodes[MAX_NODES];
	memset(Nodes, 0, sizeof(Nodes));
	mvFile.Read(Nodes, sizeof(Nodes));
	int nMapID = 0;
	if (mvFile.Read(&nMapID, sizeof(int)) < sizeof(int)) nMapID = 0;
	mvFile.Close();
	if (nMapID && nMapID != pMap->nMapID)
	{
		CString szMessage;
		szMessage.Format(_T("File là tọa độ của map %d, không phải map %s (%d).\nVẫn nạp vào danh sách?"),
			nMapID, (LPCTSTR)pMap->szName, pMap->nMapID);
		if (MessageBox(szMessage, _T("VLAuto"), MB_YESNO|MB_ICONQUESTION) != IDYES)
			return;
	}
	m_PointList.ResetContent();
	m_Pts.RemoveAll();
	for (int i = 0; i < MAX_NODES && m_PointList.GetCount() < MAX_KQ_POINT; i++)
		if (Nodes[i].x && Nodes[i].y)
			InsertPoint(-1, Nodes[i]);
}

// Ghi danh sach + tam quet hien tai lam mac dinh cua map vao MapDaTau.ini
void CYSKillQ::OnSaveDefault()
{
	KQIniMap* pMap = CurMap();
	if (!pMap || !m_pKillQOpt) return;
	BOOL bOk;
	int nRange = (int)GetDlgItemInt(IDC_QUEST_RANGESEARCH_EDIT, &bOk, FALSE);
	if (!bOk || nRange <= 0) nRange = pMap->nRange;
	CArray<POINT, POINT> Pts;
	for (int i = 0; i < m_PointList.GetCount(); i++)
		Pts.Add(GetPoint(i));

	CString szMessage;
	szMessage.Format(_T("Ghi %d tọa độ, tầm quét %d làm mặc định của map %s (%d) vào MapDaTau.ini?"),
		(int)Pts.GetCount(), nRange, (LPCTSTR)pMap->szName, pMap->nMapID);
	if (MessageBox(szMessage, _T("VLAuto"), MB_YESNO|MB_ICONQUESTION) != IDYES)
		return;
	if (!WriteIniDefault(pMap, nRange, Pts))
	{
		MessageBox(_T("Lỗi: không thể ghi MapDaTau.ini!"), 0, MB_ICONEXCLAMATION);
		return;
	}
	pMap->nRange = nRange;
	pMap->Pts.Copy(Pts);
	// Gio da trung mac dinh => bo cai dat rieng (giu lai trang thai chon/bo chon map)
	KQMapOpt* pOpt = m_pKillQOpt->FindMap(pMap->nMapID, FALSE);
	if (pOpt) pOpt->nFlags &= ~(KQF_POINTS|KQF_RANGE);
}

void CYSKillQ::OnLoadDefault()
{
	KQIniMap* pMap = CurMap();
	if (!pMap || !m_pKillQOpt) return;
	if (MessageBox(_T("Xóa danh sách và nạp lại tọa độ mặc định của map này trong MapDaTau.ini?"),
		_T("VLAuto"), MB_YESNO|MB_ICONQUESTION) != IDYES)
		return;
	KQMapOpt* pOpt = m_pKillQOpt->FindMap(pMap->nMapID, FALSE);
	if (pOpt) pOpt->nFlags &= ~(KQF_POINTS|KQF_RANGE);
	int nCur = m_nCurMap;
	LoadIni();	// File co the vua bi sua tay
	ShowMap(nCur < m_nIniMaps ? nCur : -1);
}

// Sua truc tiep tung dong byte (giu nguyen ten map UTF-8 va cac key khac): thay/them AttackRange= va Points= trong [nSect]
BOOL CYSKillQ::WriteIniDefault(KQIniMap* pMap, int nRange, const CArray<POINT, POINT>& Pts)
{
	CFile iniFile;
	if (!iniFile.Open(m_szIniFile, CFile::modeRead | CFile::shareDenyNone))
		return FALSE;
	UINT uLen = (UINT)iniFile.GetLength();
	CStringA szRaw;
	char* pBuf = szRaw.GetBuffer(uLen + 1);
	uLen = iniFile.Read(pBuf, uLen);
	pBuf[uLen] = 0;
	szRaw.ReleaseBuffer(uLen);
	iniFile.Close();

	CStringA szBOM;
	if (szRaw.GetLength() >= 3 && (BYTE)szRaw[0] == 0xEF && (BYTE)szRaw[1] == 0xBB && (BYTE)szRaw[2] == 0xBF)
	{
		szBOM = szRaw.Left(3);
		szRaw = szRaw.Mid(3);
	}
	LPCSTR szEOL = szRaw.Find("\r\n") >= 0 ? "\r\n" : "\n";

	CArray<CStringA, CStringA&> Lines;
	int nStart = 0;
	while (nStart <= szRaw.GetLength())
	{
		int nEnd = szRaw.Find('\n', nStart);
		if (nEnd < 0) nEnd = szRaw.GetLength();
		CStringA szLine = szRaw.Mid(nStart, nEnd - nStart);
		szLine.TrimRight("\r");
		if (nEnd < szRaw.GetLength() || !szLine.IsEmpty())
			Lines.Add(szLine);
		nStart = nEnd + 1;
	}

	CStringA szRange, szPoints, szSect;
	szRange.Format("AttackRange=%d", nRange);
	szPoints = "Points=";
	for (int i = 0; i < Pts.GetCount(); i++)
	{
		CStringA szPt;
		szPt.Format(i ? ";%d,%d" : "%d,%d", Pts[i].x, Pts[i].y);
		szPoints += szPt;
	}
	szSect.Format("[%d]", pMap->nSect);

	int nSect = -1;
	for (int i = 0; i < Lines.GetCount() && nSect < 0; i++)
	{
		CStringA szLine = Lines[i];
		if (szLine.Trim() == szSect) nSect = i;
	}
	if (nSect < 0) return FALSE;

	int nRangeLine = -1, nPointsLine = -1, nLast = nSect;
	for (int i = nSect + 1; i < Lines.GetCount(); i++)
	{
		CStringA szLine = Lines[i];
		szLine.Trim();
		if (!szLine.IsEmpty() && szLine[0] == '[') break;
		if (!szLine.IsEmpty()) nLast = i;
		CStringA szKey = szLine.SpanExcluding("=").Trim();
		if (!szKey.CompareNoCase("AttackRange")) nRangeLine = i;
		else if (!szKey.CompareNoCase("Points")) nPointsLine = i;
	}
	if (nRangeLine >= 0) Lines[nRangeLine] = szRange;
	else Lines.InsertAt(++nLast, szRange);
	if (nPointsLine >= 0) Lines[nPointsLine] = szPoints;
	else Lines.InsertAt(++nLast, szPoints);

	CStringA szOut = szBOM;
	for (int i = 0; i < Lines.GetCount(); i++)
	{
		szOut += Lines[i];
		szOut += szEOL;
	}
	if (!iniFile.Open(m_szIniFile, CFile::modeCreate | CFile::modeWrite))
		return FALSE;
	iniFile.Write((LPCSTR)szOut, szOut.GetLength());
	iniFile.Close();
	return TRUE;
}
