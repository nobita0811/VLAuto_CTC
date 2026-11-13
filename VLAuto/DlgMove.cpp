// DlgMove.cpp : implementation file
//

#include "stdafx.h"
#include "../GameDef.h"
#include "DWinProcess.h"
#include "VLAuto.h"
#include "GameCore.h"
#include "GameWindow.h"
#include "UiControls.h"
#include "DlgMap.h"
#include "DlgMove.h"
#include "DlgMovePoints.h"

// CDlgMove dialog

IMPLEMENT_DYNAMIC(CDlgMove, CDialog)

CDlgMove::CDlgMove(CWnd* pParent /*=NULL*/)
	: CDialog(CDlgMove::IDD, pParent)
{
	m_nEditList = -1;
}

// ID control theo tung danh sach: 0 = luyen cong, 1 = di den noi luyen cong
static const UINT s_uListID[MOVEPOINTS_LISTS]	= { IDC_MOVEPT_LIST0, IDC_MOVEPT_LIST1 };
static const UINT s_uOpenID[MOVEPOINTS_LISTS]	= { IDC_MOVEPT_OPEN0, IDC_MOVEPT_OPEN1 };
static const UINT s_uSaveID[MOVEPOINTS_LISTS]	= { IDC_MOVEPT_SAVE0, IDC_MOVEPT_SAVE1 };
static const UINT s_uReadID[MOVEPOINTS_LISTS]	= { IDC_MOVEPT_READ0, IDC_MOVEPT_READ1 };
static const UINT s_uStopID[MOVEPOINTS_LISTS]	= { IDC_MOVEPT_STOP0, IDC_MOVEPT_STOP1 };

POINT* CDlgMove::GetNodes(int nList)
{
	return nList ? g_pCurGameWin->m_prMove.m_GoNodes : g_pCurGameWin->m_prMove.m_Nodes;
}

int& CDlgMove::GetNodesMapID(int nList)
{
	return nList ? g_pCurGameWin->m_prMove.m_nGoNodesMapID : g_pCurGameWin->m_prMove.m_nNodesMapID;
}

CDlgMove::~CDlgMove()
{
}

void CDlgMove::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	for (int i = 0; i < MOVEPOINTS_LISTS; i++)
		DDX_Control(pDX, s_uListID[i], m_PointList[i]);

	if (g_pCurGameWin == NULL) return;

	//Checkboxes
	DDX_Check(pDX, IDC_FOLLOW_CHECK, g_pCurGameWin->m_prMove.m_bFollow);
	DDX_Check(pDX, IDC_RIDEHORSEFOLLOW_CHECK, g_pCurGameWin->m_prMove.m_bRideHorseFollow);
	DDX_Check(pDX, IDC_AROUNDPOINT_CHECK, g_pCurGameWin->m_prMove.m_bAroundPoint);
	DDX_Check(pDX, IDC_ROUTEPOINT_CHECK, g_pCurGameWin->m_prMove.m_bRouteMove);
	DDX_Check(pDX, IDC_RIDEHORSEMOVE_CHECK, g_pCurGameWin->m_prMove.m_bRideHorseMove);
	DDX_Check(pDX, IDC_ATTACKWHENMOVE_CHECK, g_pCurGameWin->m_prMove.m_bAttackWhenMove);
	DDX_Check(pDX, IDC_NOATTACKFOLLOW_CHECK, g_pCurGameWin->m_prMove.m_bNoAttackFollow);
	DDX_Check(pDX, IDC_MOVEBYMAP_CHECK, g_pCurGameWin->m_prMove.m_bMoveByMap);
	DDX_Check(pDX, IDC_LURE_CREEP_CHECK, g_pCurGameWin->m_prMove.m_bLureCreep);

	//Editboxes
	DDX_Text(pDX, IDC_FOLLOWDISTANCE_EDIT, g_pCurGameWin->m_prMove.m_nFollowDistance);
	DDX_Text(pDX, IDC_POINTRANGE_EDIT, g_pCurGameWin->m_prMove.m_nPointRange);
	DDX_Text(pDX, IDC_LURE_CREEP_RANGE_EDIT, g_pCurGameWin->m_prMove.m_nLureRange);

	//UI Refresh
	BOOL bChecked, bChecked1;
	bChecked = ((CButton*)GetDlgItem(IDC_FOLLOW_CHECK))->GetCheck();
	GetDlgItem(IDC_NEARBYPLAYER_COMBO)->EnableWindow(!bChecked);
	GetDlgItem(IDC_FOLLOWDISTANCE_EDIT)->EnableWindow(!bChecked);
	bChecked = ((CButton*)GetDlgItem(IDC_AROUNDPOINT_CHECK))->GetCheck();
	if (bChecked)
	{
		SetDlgItemInt(IDC_POINTX_EDIT, g_pCurGameWin->m_prMove.m_nPointX >> 8);
		SetDlgItemInt(IDC_POINTY_EDIT, g_pCurGameWin->m_prMove.m_nPointY >> 9);
	}
	bChecked1 = ((CButton*)GetDlgItem(IDC_ROUTEPOINT_CHECK))->GetCheck();
	GetDlgItem(IDC_POINTRANGE_EDIT)->EnableWindow(!bChecked1 && !bChecked);
	GetDlgItem(IDC_LURE_CREEP_RANGE_EDIT)->EnableWindow(((CButton*)GetDlgItem(IDC_LURE_CREEP_CHECK))->GetCheck());

	UpdateNearByPlayers(pDX->m_bSaveAndValidate);
	if (!pDX->m_bSaveAndValidate) UpdatePointsList();

	if (!pDX->m_bSaveAndValidate)
	{
		// Map train = map cua danh sach toa do (da bo "Noi luyen cong" o Tab Ban do)
		int nTrainMapID = CDlgMovePoints::GetTrainMapID();
		g_pCurGameWin->m_prMap.m_nTrainMapID = nTrainMapID;
		SetDlgItemText(IDC_TRAINMAP_EDIT, nTrainMapID ? CDlgMovePoints::GetMapName(nTrainMapID) : CString(_T("Bản đồ bất kỳ")));
	}
}

BEGIN_MESSAGE_MAP(CDlgMove, CDialog)
	ON_CONTROL_RANGE(BN_CLICKED, 1000, 2000, &CDlgMove::OnButtonsClicked)
	ON_CONTROL_RANGE(LBN_SELCHANGE, IDC_MOVEPT_LIST0, IDC_MOVEPT_LIST1, &CDlgMove::OnListSelChange)
	ON_EN_KILLFOCUS(IDC_LURE_CREEP_RANGE_EDIT, &CDlgMove::OnEnKillfocusLureRange)
END_MESSAGE_MAP()

// CDlgMove message handlers

BOOL CDlgMove::OnInitDialog()
{
	CDialog::OnInitDialog();
	m_NameEdit.AttachTo(this, IDC_NEARBYPLAYER_COMBO);

	if (m_ToolTip.Create(this))
	{
		m_ToolTip.AddTool(GetDlgItem(IDC_FOLLOW_CHECK), _T("Theo sau nhân vật khác"));
		m_ToolTip.AddTool(GetDlgItem(IDC_NEARBYPLAYER_COMBO), _T("Tên nhân vật cần theo"));
		m_ToolTip.AddTool(GetDlgItem(IDC_FOLLOWDISTANCE_EDIT), _T("Phạm vi theo sau"));
		m_ToolTip.AddTool(GetDlgItem(IDC_NOATTACKFOLLOW_CHECK), _T("Không tự đánh nếu mất người dẫn đường"));
		m_ToolTip.AddTool(GetDlgItem(IDC_MOVEBYMAP_CHECK), _T("Di chuyển tự do toàn bản đồ theo các điểm MapPath (Maps\\Path)"));
		m_ToolTip.AddTool(GetDlgItem(IDC_RIDEHORSEFOLLOW_CHECK), _T("Tự lên ngựa khi theo sau"));
		m_ToolTip.AddTool(GetDlgItem(IDC_AROUNDPOINT_CHECK), _T("Đánh quanh 1 điểm cố định"));
		m_ToolTip.AddTool(GetDlgItem(IDC_POINTX_EDIT), _T("Tọa độ đánh quanh điểm"));
		m_ToolTip.AddTool(GetDlgItem(IDC_POINTY_EDIT), _T("Tọa độ đánh quanh điểm"));
		m_ToolTip.AddTool(GetDlgItem(IDC_POINTRANGE_EDIT), _T("Phạm vi đánh Quanh điểm và theo Tọa độ"));
		m_ToolTip.AddTool(GetDlgItem(IDC_ROUTEPOINT_CHECK), _T("Hành trình luyện công - Đánh quái theo tọa độ"));
		m_ToolTip.AddTool(GetDlgItem(IDC_LURE_CREEP_CHECK), _T("Quây quái: tới tọa độ train, nếu có hơn 1 con quái thì chạy qua từng con cho nó đi theo, về lại tọa độ đó rồi mới đánh"));
		m_ToolTip.AddTool(GetDlgItem(IDC_LURE_CREEP_RANGE_EDIT), _T("Phạm vi quây quái quanh tọa độ train (100 - 2000)"));
		for (int i = 0; i < MOVEPOINTS_LISTS; i++)
		{
			m_ToolTip.AddTool(GetDlgItem(s_uOpenID[i]), _T("Mở file danh sách tọa độ (.mvp)"));
			m_ToolTip.AddTool(GetDlgItem(s_uSaveID[i]), _T("Ghi danh sách ra file (.mvp)"));
			m_ToolTip.AddTool(GetDlgItem(s_uReadID[i]), _T("Bắt đầu đọc tọa độ: xóa danh sách cũ, vào game bấm phím cách (SPACE) để đọc từng tọa độ"));
			m_ToolTip.AddTool(GetDlgItem(s_uStopID[i]), _T("Kết thúc đọc tọa độ, khôi phục tên bang trên đầu nhân vật (phím tắt Ctrl + End)"));
		}
		m_ToolTip.AddTool(GetDlgItem(IDC_MOVEPT_MAP), _T("Bản đồ của danh sách - chỉ đi theo tọa độ khi ở đúng bản đồ này"));
		m_ToolTip.AddTool(GetDlgItem(IDC_MOVEPT_X), _T("Tọa độ X tuyệt đối của dòng đang chọn"));
		m_ToolTip.AddTool(GetDlgItem(IDC_MOVEPT_Y), _T("Tọa độ Y tuyệt đối của dòng đang chọn"));
		m_ToolTip.AddTool(GetDlgItem(IDC_MOVEPT_EDIT), _T("Sửa dòng đang chọn theo X, Y đã nhập (dùng chung cho cả 2 danh sách)"));
	}

	// Nap icon cho 4 nut moi danh sach (Mo file / Ghi file / Doc TD / Ket thuc)
	static const UINT s_uBmpID[4] = { IDB_MVOPEN, IDB_MVSAVE, IDB_MVREAD, IDB_MVSTOP };
	for (int b = 0; b < 4; b++)
		m_bmpIcon[b].LoadBitmap(s_uBmpID[b]);
	for (int i = 0; i < MOVEPOINTS_LISTS; i++)
	{
		((CButton*)GetDlgItem(s_uOpenID[i]))->SetBitmap((HBITMAP)m_bmpIcon[0]);
		((CButton*)GetDlgItem(s_uSaveID[i]))->SetBitmap((HBITMAP)m_bmpIcon[1]);
		((CButton*)GetDlgItem(s_uReadID[i]))->SetBitmap((HBITMAP)m_bmpIcon[2]);
		((CButton*)GetDlgItem(s_uStopID[i]))->SetBitmap((HBITMAP)m_bmpIcon[3]);
	}

	return FALSE;
}

BOOL CDlgMove::PreTranslateMessage(MSG* pMsg)
{
	m_ToolTip.RelayEvent(pMsg);
	return CDialog::PreTranslateMessage(pMsg);
}

// O pham vi quay quai: roi o la ap dung ngay (cac o so khac chi luu khi bam checkbox)
void CDlgMove::OnEnKillfocusLureRange()
{
	if (g_pCurGameWin == NULL) return;
	UpdateData();
	g_pCurGameWin->SetGameParams(tab_move);
}

void CDlgMove::OnButtonsClicked(UINT nID)
{
	if (g_pCurGameWin == NULL) return;

	// Nut cua 2 danh sach toa do (moi list 1 bo: Mo file / Ghi file / Doc TD / Ket thuc)
	for (int i = 0; i < MOVEPOINTS_LISTS; i++)
	{
		if (nID == s_uOpenID[i])	{ LoadList(i); return; }
		if (nID == s_uSaveID[i])	{ SaveList(i); return; }
		if (nID == s_uReadID[i])	{ SendReadPosCmd(cmd_startreadpos, i ? rpk_gonode : rpk_node); return; }
		if (nID == s_uStopID[i])	{ SendReadPosCmd(cmd_stopreadpos, 0); return; }
	}
	if (nID == IDC_MOVEPT_EDIT)		{ EditSelNode(); return; }	// o Sua dung chung cho ca 2 danh sach

	if (nID == IDC_AROUNDPOINT_CHECK)
	{
		if (((CButton*)GetDlgItem(nID))->GetCheck())
		{
			((CButton*)GetDlgItem(IDC_ROUTEPOINT_CHECK))->SetCheck(0);	// Loai tru nhau: chi 1 trong 2
			g_pCurGameWin->m_prMove.m_nPointX = g_pCurGameWin->m_nOffX;
			g_pCurGameWin->m_prMove.m_nPointY = g_pCurGameWin->m_nOffY;
			g_pCurGameWin->m_prMove.m_nPointMapID = g_pCurGameWin->m_nMapID;	// Lay map vua nhan lam map train
		}
	}
	else if (nID == IDC_ROUTEPOINT_CHECK)
	{
		if (((CButton*)GetDlgItem(nID))->GetCheck())
			((CButton*)GetDlgItem(IDC_AROUNDPOINT_CHECK))->SetCheck(0);	// Loai tru nhau: chi 1 trong 2
	}
	UpdateData();
	g_pCurGameWin->SetGameParams(tab_move);
}

// Chon dong o danh sach nao thi o Sua lien ket voi danh sach do (bo chon danh sach con lai)
void CDlgMove::OnListSelChange(UINT nID)
{
	int nList = (nID == IDC_MOVEPT_LIST1) ? 1 : 0;
	m_nEditList = nList;
	m_PointList[1 - nList].SetCurSel(-1);
	ShowSelNode();
}

// Dung lai 1 listbox tu mang node (chi so dong == chi so node); giu dong dang chon neu con hop le
void CDlgMove::RefreshList(int nList)
{
	if (g_pCurGameWin == NULL) return;
	CListBox& lst = m_PointList[nList];
	int nSel = lst.GetCurSel();
	lst.ResetContent();
	POINT* pNodes = GetNodes(nList);
	for (int i = 0; i < MAX_NODES && pNodes[i].x && pNodes[i].y; i++)
	{
		CString szLine;	// [x, y] tren ban do game - X, Y tuyet doi
		szLine.Format(_T("[%d, %d] - %d, %d"), pNodes[i].x >> 8, pNodes[i].y >> 9, pNodes[i].x, pNodes[i].y);
		lst.AddString(szLine);
	}
	if (nSel >= lst.GetCount()) nSel = lst.GetCount() - 1;
	lst.SetCurSel(nSel);
}

// Dung lai ca 2 danh sach + o ban do (goi khi mo tab, hoac khi VLHook tra toa do ve)
void CDlgMove::UpdatePointsList()
{
	if (g_pCurGameWin == NULL) return;
	for (int i = 0; i < MOVEPOINTS_LISTS; i++)
		RefreshList(i);
	UpdateMapLabel();
	ShowSelNode();
}

// Dua toa do tuyet doi cua dong dang chon (o danh sach m_nEditList) len 2 o Sua X, Y
void CDlgMove::ShowSelNode()
{
	if (g_pCurGameWin == NULL || m_nEditList < 0)
	{
		SetDlgItemText(IDC_MOVEPT_X, _T(""));
		SetDlgItemText(IDC_MOVEPT_Y, _T(""));
		return;
	}
	CListBox& lst = m_PointList[m_nEditList];
	int nIndex = lst.GetCurSel();
	POINT* pNodes = GetNodes(m_nEditList);
	if (nIndex < 0 || nIndex >= MAX_NODES || !pNodes[nIndex].x || !pNodes[nIndex].y)
	{
		SetDlgItemText(IDC_MOVEPT_X, _T(""));
		SetDlgItemText(IDC_MOVEPT_Y, _T(""));
		return;
	}
	SetDlgItemInt(IDC_MOVEPT_X, pNodes[nIndex].x, FALSE);
	SetDlgItemInt(IDC_MOVEPT_Y, pNodes[nIndex].y, FALSE);
}

// O ban do chung: 2 danh sach thuong cung map; khac nhau thi hien ca 2
void CDlgMove::UpdateMapLabel()
{
	CString szMap[MOVEPOINTS_LISTS];
	for (int i = 0; i < MOVEPOINTS_LISTS; i++)
	{
		int nMapID = GetNodesMapID(i);
		if (nMapID)
			szMap[i] = CDlgMovePoints::GetMapName(nMapID);
		else if (m_PointList[i].GetCount())
			szMap[i] = _T("Chưa rõ - bấm Đọc TĐ để gán");
	}
	if (szMap[0].IsEmpty() || szMap[1].IsEmpty() || szMap[0] == szMap[1])
		SetDlgItemText(IDC_MOVEPT_MAP, szMap[0].IsEmpty() ? szMap[1] : szMap[0]);
	else
		SetDlgItemText(IDC_MOVEPT_MAP, szMap[0] + _T(" / ") + szMap[1]);
}

// Gui lenh doc toa do xuong VLHook. Reply ve cua so chinh (da AllowMessageFromGame) ->
// CVLAutoDlg::OnCopyData ghi vao node + goi UpdatePointsList(). Param tinh vi PostMessage bat dong bo.
void CDlgMove::SendReadPosCmd(int nCmd, int nKind)
{
	if (g_pCurGameWin == NULL || !g_pCurGameWin->m_hWnd) return;
	static READPOS_PARAM param;
	memset(&param, 0, sizeof(param));
	param.cbSize	= sizeof(param);
	CWnd* pMain		= AfxGetMainWnd();
	param.hWndReply	= pMain ? pMain->GetSafeHwnd() : GetSafeHwnd();
	param.nCmd		= nCmd;
	param.nKind		= nKind;
	param.nCurMapID	= g_pCurGameWin->m_nMapID;
	::PostMessage(g_pCurGameWin->m_hWnd, WM_HOOK_READPOS, (WPARAM)&param, sizeof(param));
}

// Sua dong dang chon (danh sach m_nEditList) theo X, Y nhap tay (toa do tuyet doi)
BOOL CDlgMove::EditSelNode()
{
	if (g_pCurGameWin == NULL) return FALSE;
	int nList = m_nEditList;
	int nIndex = (nList >= 0) ? m_PointList[nList].GetCurSel() : -1;
	if (nList < 0 || nIndex < 0)
	{
		MessageBox(_T("Chọn 1 dòng trong danh sách để sửa!"), _T("VLAuto"), MB_ICONEXCLAMATION);
		return FALSE;
	}
	BOOL bOkX, bOkY;
	POINT pt;
	pt.x = (int)GetDlgItemInt(IDC_MOVEPT_X, &bOkX, FALSE);
	pt.y = (int)GetDlgItemInt(IDC_MOVEPT_Y, &bOkY, FALSE);
	if (!bOkX || !bOkY || pt.x <= 0 || pt.y <= 0)	// Hook coi x hoac y = 0 la het danh sach
	{
		MessageBox(_T("Tọa độ X, Y không hợp lệ!"), _T("VLAuto"), MB_ICONEXCLAMATION);
		return FALSE;
	}
	if (nIndex >= MAX_NODES) return FALSE;
	GetNodes(nList)[nIndex] = pt;
	RefreshList(nList);
	m_PointList[nList].SetCurSel(nIndex);
	UpdateMapLabel();
	g_pCurGameWin->SetGameParams(tab_move);		// day xuong hook (luu cau hinh)
	return TRUE;
}

void CDlgMove::SaveList(int nList)
{
	CFileDialog fDlg(FALSE, _T(".mvp"), 0,
		OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT, _T("Move Points File (*.mvp)|*.mvp|"), this);
	if (fDlg.DoModal() == IDCANCEL)
		return;
	CFile mvFile;
	if (!mvFile.Open(fDlg.GetPathName(), CFile::modeCreate | CFile::modeWrite))
	{
		MessageBox(_T("Lỗi: không thể ghi tập tin!"), 0, MB_ICONEXCLAMATION);
		return;
	}
	char szVersion[12] = "";
	strcpy(szVersion, _SZCURVERSION);
	mvFile.Write(szVersion, sizeof(szVersion));
	mvFile.Write(GetNodes(nList), sizeof(POINT) * MAX_NODES);
	mvFile.Write(&GetNodesMapID(nList), sizeof(int));	// 10-10: file cu khong co
	mvFile.Close();
}

void CDlgMove::LoadList(int nList)
{
	CFileDialog fDlg(TRUE, _T(".mvp"), 0,
		OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT, _T("Move Points File (*.mvp)|*.mvp|"), this);
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
			(LPCTSTR)CDlgMovePoints::GetMapName(g_pCurGameWin->m_nMapID));
		if (MessageBox(szMessage, _T("VLAuto"), MB_YESNO | MB_ICONQUESTION) == IDYES)
			nMapID = g_pCurGameWin->m_nMapID;
	}
	GetNodesMapID(nList) = nMapID;
	RefreshList(nList);
	UpdateMapLabel();
	g_pCurGameWin->SetGameParams(tab_move);
}

void CDlgMove::UpdateNearByPlayers(BOOL bSave)
{
	TCHAR szName[32]=_T("");
	if (bSave)
	{
		GetDlgItemText(IDC_NEARBYPLAYER_COMBO, szName, 32);
		UnicodeToTCVN3(szName, g_pCurGameWin->m_prMove.m_szLeader, 31);
	}
	else
	{
		if (((CButton*)GetDlgItem(IDC_FOLLOW_CHECK))->GetCheck())
		{
			TCVN3ToUnicode(g_pCurGameWin->m_prMove.m_szLeader, szName, 31);
			SetDlgItemText(IDC_NEARBYPLAYER_COMBO, szName);
			return;
		}
	}
	if (((CButton*)GetDlgItem(IDC_FOLLOW_CHECK))->GetCheck())
		return;

	LPBYTE lpBaseAdd, lpCurAdd;
	DWinProcess winProcess(g_pCurGameWin->m_hWnd);
	if (!winProcess.ReadMemory((LPVOID)NPC_BASE_ADD, (LPVOID)&lpBaseAdd, 4))
		return;

	CComboBox *pNearByPlayer = (CComboBox*)GetDlgItem(IDC_NEARBYPLAYER_COMBO);
	pNearByPlayer->ResetContent();

	KNpc Npc;
	for (int i = 2; i < MAX_NPC; i++)
	{
		lpCurAdd = lpBaseAdd + i*NPC_DATA_SIZE;
		winProcess.ReadMemory(lpCurAdd, (LPVOID)&Npc, sizeof(Npc));
		if (!Npc.m_Index || Npc.m_NpcKind != 1) continue;
		Npc.Name[31] = 0;
		TCVN3ToUnicode(Npc.Name, szName, 31);
		pNearByPlayer->AddString(szName);
	}
}
