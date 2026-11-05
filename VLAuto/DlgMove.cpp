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
}

CDlgMove::~CDlgMove()
{
}

void CDlgMove::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);

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
	ON_EN_KILLFOCUS(IDC_LURE_CREEP_RANGE_EDIT, &CDlgMove::OnEnKillfocusLureRange)
END_MESSAGE_MAP()

// CDlgMove message handlers

BOOL CDlgMove::OnInitDialog()
{
	CDialog::OnInitDialog();

	if (m_ToolTip.Create(this))
	{
		m_ToolTip.AddTool(GetDlgItem(IDC_FOLLOW_CHECK), _T("Theo sau nhân vật khác"));
		m_ToolTip.AddTool(GetDlgItem(IDC_NEARBYPLAYER_COMBO), _T("Tên nhân vật cần theo"));
		m_ToolTip.AddTool(GetDlgItem(IDC_FOLLOWDISTANCE_EDIT), _T("Phạm vi theo sau"));
		m_ToolTip.AddTool(GetDlgItem(IDC_NOATTACKFOLLOW_CHECK), _T("Không tự đánh nếu mất người dẫn đường"));
		m_ToolTip.AddTool(GetDlgItem(IDC_MOVEBYMAP_CHECK), _T("Di chuyển tự do toàn bản đồ (đã soạn file MAP)"));
		m_ToolTip.AddTool(GetDlgItem(IDC_RIDEHORSEFOLLOW_CHECK), _T("Tự lên ngựa khi theo sau"));
		m_ToolTip.AddTool(GetDlgItem(IDC_AROUNDPOINT_CHECK), _T("Đánh quanh 1 điểm cố định"));
		m_ToolTip.AddTool(GetDlgItem(IDC_POINTX_EDIT), _T("Tọa độ đánh quanh điểm"));
		m_ToolTip.AddTool(GetDlgItem(IDC_POINTY_EDIT), _T("Tọa độ đánh quanh điểm"));
		m_ToolTip.AddTool(GetDlgItem(IDC_POINTRANGE_EDIT), _T("Phạm vi đánh Quanh điểm và theo Tọa độ"));
		m_ToolTip.AddTool(GetDlgItem(IDC_ROUTEPOINT_CHECK), _T("Hành trình luyện công - Đánh quái theo tọa độ"));
		m_ToolTip.AddTool(GetDlgItem(IDC_LURE_CREEP_CHECK), _T("Quây quái: tới tọa độ train, nếu có hơn 1 con quái thì chạy qua từng con cho nó đi theo, về lại tọa độ đó rồi mới đánh"));
		m_ToolTip.AddTool(GetDlgItem(IDC_LURE_CREEP_RANGE_EDIT), _T("Phạm vi quây quái quanh tọa độ train (100 - 2000)"));
		m_ToolTip.AddTool(GetDlgItem(IDC_MOVE_LIST_BUTTON), _T("Sửa danh sách tọa độ Hành trình (số TĐ luyện công / số TĐ đi đến nơi luyện công)"));
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
	if (nID == IDC_MOVE_LIST_BUTTON)
	{
		CDlgMovePoints dlgPoints(this);
		dlgPoints.DoModal(); // Sua truc tiep m_prMove.m_Nodes + SetGameParams
		UpdateData(0);
		return;
	}
	if (nID == IDC_AROUNDPOINT_CHECK)
	{
		if (((CButton*)GetDlgItem(nID))->GetCheck())
		{
			g_pCurGameWin->m_prMove.m_nPointX = g_pCurGameWin->m_nOffX;
			g_pCurGameWin->m_prMove.m_nPointY = g_pCurGameWin->m_nOffY;
		}
	}
	UpdateData();
	g_pCurGameWin->SetGameParams(tab_move);
}

void CDlgMove::UpdateNearByPlayers(BOOL bSave)
{
	TCHAR szName[32]=_T("");
	if (bSave)
	{
		GetDlgItemText(IDC_NEARBYPLAYER_COMBO, szName, 32);
		WideCharToMultiByte(CP_ACP, 0, szName, -1, g_pCurGameWin->m_prMove.m_szLeader, 32, 0, 0);
	}
	else
	{
		if (((CButton*)GetDlgItem(IDC_FOLLOW_CHECK))->GetCheck())
		{
			MultiByteToWideChar(CP_ACP, 0, g_pCurGameWin->m_prMove.m_szLeader, -1, szName, 32);
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
		MultiByteToWideChar(CP_ACP, 0, Npc.Name, -1, szName, 32);
		pNearByPlayer->AddString(szName);
	}
}

void CDlgMove::UpdatePointsList()
{
	int nCount = 0, nGoCount = 0;
	for (int i = 0; i < MAX_NODES; i++)
	{
		if (g_pCurGameWin->m_prMove.m_Nodes[i].x && g_pCurGameWin->m_prMove.m_Nodes[i].y)
			nCount++;
		if (g_pCurGameWin->m_prMove.m_GoNodes[i].x && g_pCurGameWin->m_prMove.m_GoNodes[i].y)
			nGoCount++;
	}
	CString szText;
	szText.Format(_T("Danh sách tọa độ (%d/%d)"), nCount, nGoCount);
	SetDlgItemText(IDC_MOVE_LIST_BUTTON, szText);
}
