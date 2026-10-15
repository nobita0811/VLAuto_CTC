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

// CDlgMovePoints dialog

IMPLEMENT_DYNAMIC(CDlgMovePoints, CDialog)

CDlgMovePoints::CDlgMovePoints(CWnd* pParent /*=NULL*/)
	: CDialog(CDlgMovePoints::IDD, pParent)
{
}

CDlgMovePoints::~CDlgMovePoints()
{
}

void CDlgMovePoints::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_MOVEPOINT_LIST, m_PoinList);

	if (g_pCurGameWin == NULL) return;

	// Luon cho sua danh sach (ke ca khi dang bat Theo toa do) - moi thay doi day xuong hook ngay

	UpdatePointsList(pDX->m_bSaveAndValidate);
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
		m_ToolTip.AddTool(GetDlgItem(IDC_MOVEPOINT_LIST), _T("Danh sách tọa độ Hành trình"));
		m_ToolTip.AddTool(GetDlgItem(IDC_ADD_BUTTON), _T("Thêm tọa độ hiện tại của nhân vật"));
		m_ToolTip.AddTool(GetDlgItem(IDC_MAP_MOVELIST2_EDIT), _T("Bản đồ của danh sách - chỉ đi theo tọa độ khi ở đúng bản đồ này"));
		m_ToolTip.AddTool(GetDlgItem(IDC_DEL_BUTTON), _T("Xóa tọa độ đang chọn"));
		m_ToolTip.AddTool(GetDlgItem(IDC_SAVE_BUTTON), _T("Ghi danh sách ra file .mvp"));
		m_ToolTip.AddTool(GetDlgItem(IDC_LOAD_BUTTON), _T("Đọc danh sách từ file .mvp"));
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
	switch (nID)
	{
	case IDC_ADD_BUTTON:
		OnBnClickedAddButton();
		break;
	case IDC_DEL_BUTTON:
		OnBnClickedDelButton();
		break;
	case IDC_UP_BUTTON:
		OnBnClickedUpButton();
		break;
	case IDC_DOWN_BUTTON:
		OnBnClickedDownButton();
		break;
	case IDC_DELALL_BUTTON:
		OnBnClickedDelAllButton();
		break;
	case IDC_SAVE_BUTTON:
		OnBnClickedSaveButton();
		return;
	case IDC_LOAD_BUTTON:
		OnBnClickedLoadButton();
		return;
	default:
		return;
	}
	UpdateData();
	g_pCurGameWin->SetGameParams(tab_move);
}

void CDlgMovePoints::UpdatePointsList(BOOL bSave)
{
	CString szLine;
	DWORD dwItemData;
	int nIndex, nX, nY;
	if (bSave)
	{
		memset(g_pCurGameWin->m_prMove.m_Nodes, 0, sizeof(g_pCurGameWin->m_prMove.m_Nodes));
		int nCount = m_PoinList.GetCount();
		for (int i = 0; i < nCount; i++)
		{
			dwItemData = (DWORD)m_PoinList.GetItemData(i);
			nX = LOWORD(dwItemData) << 5;
			nY = HIWORD(dwItemData) << 5;
			g_pCurGameWin->m_prMove.m_Nodes[i].x = nX;
			g_pCurGameWin->m_prMove.m_Nodes[i].y = nY;
		}
	}
	else
	{
		int nSel = m_PoinList.GetCurSel();
		m_PoinList.ResetContent();
		for (int i = 0; i < MAX_NODES; i++)
		{
			nX = g_pCurGameWin->m_prMove.m_Nodes[i].x;
			nY = g_pCurGameWin->m_prMove.m_Nodes[i].y;
			if (nX && nY)
			{
				szLine.Format(_T("(%d, %d)"), nX>>8, nY>>9);
				nIndex = m_PoinList.AddString(szLine);
				dwItemData = MAKELONG(nX>>5, nY>>5);
				m_PoinList.SetItemData(nIndex, dwItemData);
			}
		}
		m_PoinList.SetCurSel(nSel);
	}
}

void CDlgMovePoints::OnBnClickedAddButton()
{
	CString szLine;
	DWORD dwItemData;
	int nIndex, nX, nY;
	if (g_pCurGameWin == NULL) return;
	int nCount = m_PoinList.GetCount();
	int& nNodesMapID = g_pCurGameWin->m_prMove.m_nNodesMapID;
	int nCurMapID = g_pCurGameWin->m_nMapID;
	if (nCount && nNodesMapID && nNodesMapID != nCurMapID)
	{
		CString szMessage;
		szMessage.Format(_T("Danh sách đang thuộc bản đồ %s,\nnhân vật đang ở %s.\n\nXóa danh sách cũ để lập danh sách mới cho bản đồ này?"),
			(LPCTSTR)GetMapName(nNodesMapID), (LPCTSTR)GetMapName(nCurMapID));
		if (MessageBox(szMessage, _T("VLAuto"), MB_YESNO|MB_ICONQUESTION) != IDYES)
			return;
		m_PoinList.ResetContent();
		nCount = 0;
	}
	nNodesMapID = nCurMapID; // Danh sach chua co ban do -> gan ban do hien tai
	if (nCount < MAX_NODES)
	{
		nX = g_pCurGameWin->m_nOffX;
		nY = g_pCurGameWin->m_nOffY;

		szLine.Format(_T("(%d, %d)"), nX>>8, nY>>9);
		nIndex = m_PoinList.AddString(szLine);
		dwItemData = MAKELONG(nX>>5, nY>>5);
		m_PoinList.SetItemData(nIndex, dwItemData);
		m_PoinList.SetCurSel(nIndex);
	}
}

void CDlgMovePoints::OnBnClickedDelButton()
{
	int nIndex = m_PoinList.GetCurSel();
	if (nIndex < 0) return;
	m_PoinList.DeleteString(nIndex);
	if (nIndex >= m_PoinList.GetCount()) nIndex--;
	m_PoinList.SetCurSel(nIndex);
}

void CDlgMovePoints::OnBnClickedUpButton()
{
	int nIndex = m_PoinList.GetCurSel();
	DWORD dwTemp; CString szTemp;
	if (nIndex>0)
	{
		m_PoinList.GetText(nIndex, szTemp);
		dwTemp = (DWORD)m_PoinList.GetItemData(nIndex);
		m_PoinList.DeleteString(nIndex);
		m_PoinList.InsertString(nIndex-1, szTemp);
		m_PoinList.SetItemData(nIndex-1, dwTemp);
		m_PoinList.SetCurSel(nIndex-1);
	}
}

void CDlgMovePoints::OnBnClickedDownButton()
{
	int nIndex = m_PoinList.GetCurSel();
	int nCount = m_PoinList.GetCount();
	DWORD dwTemp; CString szTemp;
	if (nIndex<nCount-1 && nIndex>-1)
	{
		m_PoinList.GetText(nIndex, szTemp);
		dwTemp = (DWORD)m_PoinList.GetItemData(nIndex);
		m_PoinList.DeleteString(nIndex);
		m_PoinList.InsertString(nIndex+1, szTemp);
		m_PoinList.SetItemData(nIndex+1, dwTemp);
		m_PoinList.SetCurSel(nIndex+1);
	}
}

void CDlgMovePoints::OnBnClickedDelAllButton()
{
	if (m_PoinList.GetCount() &&
		MessageBox(_T("Xóa hết danh sách tọa độ?"), _T("VLAuto"), MB_YESNO|MB_ICONQUESTION) != IDYES)
		return;
	m_PoinList.ResetContent();
	g_pCurGameWin->m_prMove.m_nNodesMapID = 0;
}

void CDlgMovePoints::OnBnClickedSaveButton()
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
	mvFile.Write(g_pCurGameWin->m_prMove.m_Nodes, sizeof(g_pCurGameWin->m_prMove.m_Nodes));
	mvFile.Write(&g_pCurGameWin->m_prMove.m_nNodesMapID, sizeof(int)); // 10-10: file cu khong co
	mvFile.Close();
}

void CDlgMovePoints::OnBnClickedLoadButton()
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
	memset(&g_pCurGameWin->m_prMove.m_Nodes, 0, sizeof(g_pCurGameWin->m_prMove.m_Nodes));
	mvFile.Read(&g_pCurGameWin->m_prMove.m_Nodes, sizeof(g_pCurGameWin->m_prMove.m_Nodes));
	int nMapID = 0;
	if (mvFile.Read(&nMapID, sizeof(int)) < sizeof(int)) nMapID = 0;
	mvFile.Close();
	if (!nMapID && g_pCurGameWin->m_prMove.m_Nodes[0].x)
	{	// File cu chua co ban do
		CString szMessage;
		szMessage.Format(_T("File chưa có thông tin bản đồ.\nGán bản đồ hiện tại (%s) cho danh sách này?"),
			(LPCTSTR)GetMapName(g_pCurGameWin->m_nMapID));
		if (MessageBox(szMessage, _T("VLAuto"), MB_YESNO|MB_ICONQUESTION) == IDYES)
			nMapID = g_pCurGameWin->m_nMapID;
	}
	g_pCurGameWin->m_prMove.m_nNodesMapID = nMapID;
	// Update Data
	UpdateData(0);
	g_pCurGameWin->SetGameParams(tab_move);
}

void CDlgMovePoints::UpdateMapName()
{
	int nMapID = g_pCurGameWin->m_prMove.m_nNodesMapID;
	if (nMapID)
		SetDlgItemText(IDC_MAP_MOVELIST2_EDIT, GetMapName(nMapID));
	else if (m_PoinList.GetCount())
		SetDlgItemText(IDC_MAP_MOVELIST2_EDIT, _T("Chưa rõ - bấm Đọc TĐ để gán"));
	else
		SetDlgItemText(IDC_MAP_MOVELIST2_EDIT, _T(""));
}
