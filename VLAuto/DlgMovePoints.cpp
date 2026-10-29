// DlgMovePoints.cpp : implementation file
//

#include "stdafx.h"
#include "../GameDef.h"
#include "VLAuto.h"
#include "GameCore.h"
#include "GameWindow.h"
#include "DlgMovePoints.h"

// Ten ban do theo MapNames.ini (giong tab Co ban)
static CString GetMapName(int nMapID)
{
	TCHAR szMapID[12], szInfo[64];
	TCHAR szMapNameFile[MAX_PATH];
	_stprintf(szMapID, _T("%d"), nMapID);
	_stprintf(szMapNameFile, _T("%s\\Maps\\MapNames.ini"), g_szAppFolder);
	GetPrivateProfileString(_T("Map_Name"), szMapID, _T("<Unknown_Map>"), szInfo, 64, szMapNameFile);
	char szTCNV3Str[64];
	WideCharToMultiByte(CP_ACP, 0, szInfo, -1, szTCNV3Str, 64, 0, 0);
	TCVN3ToUnicode(szTCNV3Str, szInfo, 63);
	CString szName;
	szName.Format(_T("%s (%d)"), szInfo, nMapID);
	return szName;
}

// ID cac control theo tung danh sach: 0 = Hanh trinh luyen cong, 1 = Di den noi luyen cong
static const UINT s_uListID[MOVEPOINTS_LISTS]		= { IDC_MOVEPOINT_LIST, IDC_MOVEPOINT_LIST2 };
static const UINT s_uAddID[MOVEPOINTS_LISTS]		= { IDC_ADD_BUTTON, IDC_ADD_BUTTON2 };
static const UINT s_uDelID[MOVEPOINTS_LISTS]		= { IDC_DEL_BUTTON, IDC_DEL_BUTTON2 };
static const UINT s_uUpID[MOVEPOINTS_LISTS]			= { IDC_UP_BUTTON, IDC_UP_BUTTON2 };
static const UINT s_uDownID[MOVEPOINTS_LISTS]		= { IDC_DOWN_BUTTON, IDC_DOWN_BUTTON2 };
static const UINT s_uDelAllID[MOVEPOINTS_LISTS]		= { IDC_DELALL_BUTTON, IDC_DELALL_BUTTON2 };
static const UINT s_uSaveID[MOVEPOINTS_LISTS]		= { IDC_SAVE_BUTTON, IDC_SAVE_BUTTON2 };
static const UINT s_uLoadID[MOVEPOINTS_LISTS]		= { IDC_LOAD_BUTTON, IDC_LOAD_BUTTON2 };
static const TCHAR* s_szListName[MOVEPOINTS_LISTS]	= { _T("Danh sách tọa độ luyện công"), _T("Danh sách tọa độ đi đến nơi luyện công") };

// CDlgMovePoints dialog

IMPLEMENT_DYNAMIC(CDlgMovePoints, CDialog)

CDlgMovePoints::CDlgMovePoints(CWnd* pParent /*=NULL*/)
	: CDialog(CDlgMovePoints::IDD, pParent)
{
}

CDlgMovePoints::~CDlgMovePoints()
{
}

POINT* CDlgMovePoints::GetNodes(int nList)
{
	return nList ? g_pCurGameWin->m_prMove.m_GoNodes : g_pCurGameWin->m_prMove.m_Nodes;
}

int& CDlgMovePoints::GetNodesMapID(int nList)
{
	return nList ? g_pCurGameWin->m_prMove.m_nGoNodesMapID : g_pCurGameWin->m_prMove.m_nNodesMapID;
}

void CDlgMovePoints::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	for (int i = 0; i < MOVEPOINTS_LISTS; i++)
		DDX_Control(pDX, s_uListID[i], m_PoinList[i]);

	if (g_pCurGameWin == NULL) return;

	// Luon cho sua danh sach (ke ca khi dang bat Theo toa do) - moi thay doi day xuong hook ngay

	for (int i = 0; i < MOVEPOINTS_LISTS; i++)
		UpdatePointsList(i, pDX->m_bSaveAndValidate);
	if (!pDX->m_bSaveAndValidate) UpdateMapName();
}

BEGIN_MESSAGE_MAP(CDlgMovePoints, CDialog)
	ON_CONTROL_RANGE(BN_CLICKED, 1000, 2000, &CDlgMovePoints::OnButtonsClicked)
END_MESSAGE_MAP()

// CDlgMovePoints message handlers

BOOL CDlgMovePoints::OnInitDialog()
{
	CDialog::OnInitDialog();

	if (m_ToolTip.Create(this))
	{
		m_ToolTip.AddTool(GetDlgItem(IDC_MOVEPOINT_LIST), _T("Danh sách tọa độ Hành trình luyện công"));
		m_ToolTip.AddTool(GetDlgItem(IDC_MOVEPOINT_LIST2), _T("Sau khi vào cổng map luyện công: đi theo MapPath tới tọa độ đầu tiên, rồi đi lần lượt theo danh sách tới nơi luyện công"));
		m_ToolTip.AddTool(GetDlgItem(IDC_MAP_MOVELIST2_EDIT), _T("Bản đồ của danh sách - chỉ đi theo tọa độ khi ở đúng bản đồ này"));
		for (int i = 0; i < MOVEPOINTS_LISTS; i++)
		{
			m_ToolTip.AddTool(GetDlgItem(s_uAddID[i]), _T("Thêm tọa độ hiện tại của nhân vật"));
			m_ToolTip.AddTool(GetDlgItem(s_uDelID[i]), _T("Xóa tọa độ đang chọn"));
			m_ToolTip.AddTool(GetDlgItem(s_uUpID[i]), _T("Chuyển tọa độ đang chọn lên trên"));
			m_ToolTip.AddTool(GetDlgItem(s_uDownID[i]), _T("Chuyển tọa độ đang chọn xuống dưới"));
			m_ToolTip.AddTool(GetDlgItem(s_uDelAllID[i]), _T("Xóa hết danh sách"));
			m_ToolTip.AddTool(GetDlgItem(s_uSaveID[i]), _T("Ghi danh sách ra file .mvp"));
			m_ToolTip.AddTool(GetDlgItem(s_uLoadID[i]), _T("Đọc danh sách từ file .mvp"));
		}
	}

	return TRUE;
}

BOOL CDlgMovePoints::PreTranslateMessage(MSG* pMsg)
{
	m_ToolTip.RelayEvent(pMsg);
	return CDialog::PreTranslateMessage(pMsg);
}

void CDlgMovePoints::OnButtonsClicked(UINT nID)
{
	if (g_pCurGameWin == NULL) return;
	for (int i = 0; i < MOVEPOINTS_LISTS; i++)
	{
		if (nID == s_uAddID[i])			OnBnClickedAddButton(i);
		else if (nID == s_uDelID[i])	OnBnClickedDelButton(i);
		else if (nID == s_uUpID[i])		OnBnClickedUpButton(i);
		else if (nID == s_uDownID[i])	OnBnClickedDownButton(i);
		else if (nID == s_uDelAllID[i])	OnBnClickedDelAllButton(i);
		else if (nID == s_uSaveID[i])	{ OnBnClickedSaveButton(i); return; }
		else if (nID == s_uLoadID[i])	{ OnBnClickedLoadButton(i); return; }
		else continue;
		UpdateData();
		UpdateMapName();
		g_pCurGameWin->SetGameParams(tab_move);
		return;
	}
}

void CDlgMovePoints::UpdatePointsList(int nList, BOOL bSave)
{
	CString szLine;
	DWORD dwItemData;
	int nIndex, nX, nY;
	CListBox& lstPoints = m_PoinList[nList];
	POINT* pNodes = GetNodes(nList);
	if (bSave)
	{
		memset(pNodes, 0, sizeof(POINT) * MAX_NODES);
		int nCount = lstPoints.GetCount();
		for (int i = 0; i < nCount && i < MAX_NODES; i++)
		{
			dwItemData = (DWORD)lstPoints.GetItemData(i);
			nX = LOWORD(dwItemData) << 5;
			nY = HIWORD(dwItemData) << 5;
			pNodes[i].x = nX;
			pNodes[i].y = nY;
		}
	}
	else
	{
		int nSel = lstPoints.GetCurSel();
		lstPoints.ResetContent();
		for (int i = 0; i < MAX_NODES; i++)
		{
			nX = pNodes[i].x;
			nY = pNodes[i].y;
			if (nX && nY)
			{
				szLine.Format(_T("(%d, %d)"), nX>>8, nY>>9);
				nIndex = lstPoints.AddString(szLine);
				dwItemData = MAKELONG(nX>>5, nY>>5);
				lstPoints.SetItemData(nIndex, dwItemData);
			}
		}
		lstPoints.SetCurSel(nSel);
	}
}

void CDlgMovePoints::OnBnClickedAddButton(int nList)
{
	CString szLine;
	DWORD dwItemData;
	int nIndex, nX, nY;
	if (g_pCurGameWin == NULL) return;
	CListBox& lstPoints = m_PoinList[nList];
	int nCount = lstPoints.GetCount();
	int& nNodesMapID = GetNodesMapID(nList);
	int nCurMapID = g_pCurGameWin->m_nMapID;
	if (nCount && nNodesMapID && nNodesMapID != nCurMapID)
	{
		CString szMessage;
		szMessage.Format(_T("%s đang thuộc bản đồ %s,\nnhân vật đang ở %s.\n\nXóa danh sách cũ để lập danh sách mới cho bản đồ này?"),
			s_szListName[nList], (LPCTSTR)GetMapName(nNodesMapID), (LPCTSTR)GetMapName(nCurMapID));
		if (MessageBox(szMessage, _T("VLAuto"), MB_YESNO|MB_ICONQUESTION) != IDYES)
			return;
		lstPoints.ResetContent();
		nCount = 0;
	}
	nNodesMapID = nCurMapID; // Danh sach chua co ban do -> gan ban do hien tai
	if (nCount < MAX_NODES)
	{
		nX = g_pCurGameWin->m_nOffX;
		nY = g_pCurGameWin->m_nOffY;

		szLine.Format(_T("(%d, %d)"), nX>>8, nY>>9);
		nIndex = lstPoints.AddString(szLine);
		dwItemData = MAKELONG(nX>>5, nY>>5);
		lstPoints.SetItemData(nIndex, dwItemData);
		lstPoints.SetCurSel(nIndex);
	}
}

void CDlgMovePoints::OnBnClickedDelButton(int nList)
{
	CListBox& lstPoints = m_PoinList[nList];
	int nIndex = lstPoints.GetCurSel();
	if (nIndex < 0) return;
	lstPoints.DeleteString(nIndex);
	if (nIndex >= lstPoints.GetCount()) nIndex--;
	lstPoints.SetCurSel(nIndex);
}

void CDlgMovePoints::OnBnClickedUpButton(int nList)
{
	CListBox& lstPoints = m_PoinList[nList];
	int nIndex = lstPoints.GetCurSel();
	DWORD dwTemp; CString szTemp;
	if (nIndex>0)
	{
		lstPoints.GetText(nIndex, szTemp);
		dwTemp = (DWORD)lstPoints.GetItemData(nIndex);
		lstPoints.DeleteString(nIndex);
		lstPoints.InsertString(nIndex-1, szTemp);
		lstPoints.SetItemData(nIndex-1, dwTemp);
		lstPoints.SetCurSel(nIndex-1);
	}
}

void CDlgMovePoints::OnBnClickedDownButton(int nList)
{
	CListBox& lstPoints = m_PoinList[nList];
	int nIndex = lstPoints.GetCurSel();
	int nCount = lstPoints.GetCount();
	DWORD dwTemp; CString szTemp;
	if (nIndex<nCount-1 && nIndex>-1)
	{
		lstPoints.GetText(nIndex, szTemp);
		dwTemp = (DWORD)lstPoints.GetItemData(nIndex);
		lstPoints.DeleteString(nIndex);
		lstPoints.InsertString(nIndex+1, szTemp);
		lstPoints.SetItemData(nIndex+1, dwTemp);
		lstPoints.SetCurSel(nIndex+1);
	}
}

void CDlgMovePoints::OnBnClickedDelAllButton(int nList)
{
	CListBox& lstPoints = m_PoinList[nList];
	if (lstPoints.GetCount())
	{
		CString szMessage;
		szMessage.Format(_T("Xóa hết %s?"), s_szListName[nList]);
		if (MessageBox(szMessage, _T("VLAuto"), MB_YESNO|MB_ICONQUESTION) != IDYES)
			return;
	}
	lstPoints.ResetContent();
	GetNodesMapID(nList) = 0;
}

void CDlgMovePoints::OnBnClickedSaveButton(int nList)
{
	CFileDialog fDlg(FALSE, _T(".mvp"), 0,
		OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT, _T("Move Points File (*.mvp)|*.mvp|"), this);
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
	mvFile.Write(GetNodes(nList), sizeof(POINT) * MAX_NODES);
	mvFile.Write(&GetNodesMapID(nList), sizeof(int)); // 10-10: file cu khong co
	mvFile.Close();
}

void CDlgMovePoints::OnBnClickedLoadButton(int nList)
{
	CFileDialog fDlg(TRUE, _T(".mvp"), 0,
		OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT, _T("Move Points File (*.mvp)|*.mvp|"), this);
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
	POINT* pNodes = GetNodes(nList);
	memset(pNodes, 0, sizeof(POINT) * MAX_NODES);
	mvFile.Read(pNodes, sizeof(POINT) * MAX_NODES);
	int nMapID = 0;
	if (mvFile.Read(&nMapID, sizeof(int)) < sizeof(int)) nMapID = 0;
	mvFile.Close();
	if (!nMapID && pNodes[0].x)
	{	// File cu chua co ban do
		CString szMessage;
		szMessage.Format(_T("File chưa có thông tin bản đồ.\nGán bản đồ hiện tại (%s) cho danh sách này?"),
			(LPCTSTR)GetMapName(g_pCurGameWin->m_nMapID));
		if (MessageBox(szMessage, _T("VLAuto"), MB_YESNO|MB_ICONQUESTION) == IDYES)
			nMapID = g_pCurGameWin->m_nMapID;
	}
	GetNodesMapID(nList) = nMapID;
	// Update Data
	UpdateData(0);
	g_pCurGameWin->SetGameParams(tab_move);
}

// Ban do cua 2 danh sach (thuong cung la map luyen cong); khac nhau thi hien ca 2
void CDlgMovePoints::UpdateMapName()
{
	CString szMap[MOVEPOINTS_LISTS];
	for (int i = 0; i < MOVEPOINTS_LISTS; i++)
	{
		int nMapID = GetNodesMapID(i);
		if (nMapID)
			szMap[i] = GetMapName(nMapID);
		else if (m_PoinList[i].GetCount())
			szMap[i] = _T("Chưa rõ - bấm Đọc TĐ để gán");
	}
	if (szMap[0].IsEmpty() || szMap[1].IsEmpty() || szMap[0] == szMap[1])
		SetDlgItemText(IDC_MAP_MOVELIST2_EDIT, szMap[0].IsEmpty() ? szMap[1] : szMap[0]);
	else
		SetDlgItemText(IDC_MAP_MOVELIST2_EDIT, szMap[0] + _T(" / ") + szMap[1]);
}
