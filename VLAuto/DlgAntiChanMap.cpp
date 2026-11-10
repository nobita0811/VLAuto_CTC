// DlgAntiChanMap.cpp : Cai dat chong chan map (mo tu tab Co ban)
//

#include "stdafx.h"
#include "../GameDef.h"
#include "VLAuto.h"
#include "GameCore.h"
#include "GameWindow.h"
#include "DlgAntiChanMap.h"

// CDlgAntiChanMap dialog

IMPLEMENT_DYNAMIC(CDlgAntiChanMap, CDialog)

CDlgAntiChanMap::CDlgAntiChanMap(CWnd* pParent /*=NULL*/)
	: CDialog(CDlgAntiChanMap::IDD, pParent)
{
}

CDlgAntiChanMap::~CDlgAntiChanMap()
{
}

void CDlgAntiChanMap::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	if (g_pCurGameWin == NULL) return;

	PARAM_BASIC &pb = g_pCurGameWin->m_prBasic;
	DDX_Check(pDX, IDC_CHANMAP_CHECK, pb.m_bFixChanMap);
	DDX_Check(pDX, IDC_CHANMAP_JUMPNOW_CHECK, pb.m_bFixChanMap2);
	DDX_Check(pDX, IDC_CHANMAP_STUCK_CHECK, pb.m_bFixChanMap1);
	DDX_Text(pDX, IDC_CHANMAP_JUMPDIST_EDIT, pb.m_nChanJumpDist);
	DDV_MinMaxInt(pDX, pb.m_nChanJumpDist, 50, 2000);
	DDX_Text(pDX, IDC_CHANMAP_JUMPCOUNT_EDIT, pb.m_nChanJumpCount);
	DDV_MinMaxInt(pDX, pb.m_nChanJumpCount, 1, 50);
	DDX_Text(pDX, IDC_CHANMAP_CHECKTIME_EDIT, pb.m_nChanCheckTime);
	DDV_MinMaxInt(pDX, pb.m_nChanCheckTime, 0, 60000);
	DDX_Text(pDX, IDC_CHANMAP_CHECKDIST_EDIT, pb.m_nChanCheckDist);
	DDV_MinMaxInt(pDX, pb.m_nChanCheckDist, 10, 2000);

	if (!pDX->m_bSaveAndValidate)
		UpdateChanMapInfo();
}

BEGIN_MESSAGE_MAP(CDlgAntiChanMap, CDialog)
	ON_BN_CLICKED(IDC_CHANMAP_CHECK, &CDlgAntiChanMap::OnBnClickedChanMapCheck)
	ON_BN_CLICKED(IDC_CHANMAP_JUMPNOW_CHECK, &CDlgAntiChanMap::OnBnClickedChanMapOption)
	ON_BN_CLICKED(IDC_CHANMAP_STUCK_CHECK, &CDlgAntiChanMap::OnBnClickedChanMapOption)
	ON_BN_CLICKED(IDC_CHANMAP_GATE_BTN, &CDlgAntiChanMap::OnBnClickedChanMapGate)
	ON_BN_CLICKED(IDC_CHANMAP_DIR_BTN, &CDlgAntiChanMap::OnBnClickedChanMapDir)
	ON_EN_KILLFOCUS(IDC_CHANMAP_JUMPDIST_EDIT, &CDlgAntiChanMap::OnEnKillfocusChanMapEdit)
	ON_EN_KILLFOCUS(IDC_CHANMAP_JUMPCOUNT_EDIT, &CDlgAntiChanMap::OnEnKillfocusChanMapEdit)
	ON_EN_KILLFOCUS(IDC_CHANMAP_CHECKTIME_EDIT, &CDlgAntiChanMap::OnEnKillfocusChanMapEdit)
	ON_EN_KILLFOCUS(IDC_CHANMAP_CHECKDIST_EDIT, &CDlgAntiChanMap::OnEnKillfocusChanMapEdit)
END_MESSAGE_MAP()

BOOL CDlgAntiChanMap::OnInitDialog()
{
	CDialog::OnInitDialog();	// UpdateData(FALSE)

	if (m_ToolTip.Create(this))
	{
		m_ToolTip.AddTool(GetDlgItem(IDC_CHANMAP_CHECK), _T("Tự khinh công qua điểm bị chặn map (áp dụng bảng cố định theo ID map + toạ độ tuỳ chọn)"));
		m_ToolTip.AddTool(GetDlgItem(IDC_CHANMAP_JUMPNOW_CHECK), _T("Nhảy ngay khi vừa chuyển map (bảng cố định)"));
		m_ToolTip.AddTool(GetDlgItem(IDC_CHANMAP_STUCK_CHECK), _T("Nhảy khi đứng kẹt gần cổng một lúc (bảng cố định)"));
		m_ToolTip.AddTool(GetDlgItem(IDC_CHANMAP_GATE_BTN), _T("Đứng tại cổng bị chặn rồi bấm: lưu Map + toạ độ cổng hiện tại"));
		m_ToolTip.AddTool(GetDlgItem(IDC_CHANMAP_DIR_BTN), _T("Đứng ở phía muốn nhảy tới (bên kia cổng) rồi bấm: lưu hướng nhảy"));
		m_ToolTip.AddTool(GetDlgItem(IDC_CHANMAP_JUMPDIST_EDIT), _T("Độ dài mỗi lần khinh công (toạ độ tuỳ chọn, mặc định 200)"));
		m_ToolTip.AddTool(GetDlgItem(IDC_CHANMAP_JUMPCOUNT_EDIT), _T("Số lần nhảy tối đa mỗi lần vào vùng cổng (mặc định 3)"));
		m_ToolTip.AddTool(GetDlgItem(IDC_CHANMAP_CHECKTIME_EDIT), _T("Thời gian (ms) đứng gần cổng trước khi nhảy (mặc định 1000)"));
		m_ToolTip.AddTool(GetDlgItem(IDC_CHANMAP_CHECKDIST_EDIT), _T("Bán kính coi là 'gần cổng' để kích hoạt nhảy (mặc định 150)"));
	}

	return TRUE;
}

BOOL CDlgAntiChanMap::PreTranslateMessage(MSG* pMsg)
{
	m_ToolTip.RelayEvent(pMsg);
	return CDialog::PreTranslateMessage(pMsg);
}

// Bam OK: xac nhan (validate) roi dong. Moi thay doi da duoc luu truc tiep nen chi can dong.
void CDlgAntiChanMap::OnOK()
{
	if (g_pCurGameWin != NULL)
	{
		if (!UpdateData()) return;		// gia tri khong hop le: giu cua so mo
		g_pCurGameWin->SetGameParams(tab_basic);
		if (g_pCurGameWin->m_bLoadDone)
			g_pCurGameWin->SaveProfile();
	}
	CDialog::OnOK();
}

void CDlgAntiChanMap::OnCancel()
{
	CDialog::OnCancel();
}

void CDlgAntiChanMap::OnBnClickedChanMapCheck()
{
	if (g_pCurGameWin == NULL) return;
	if (!UpdateData())
	{
		// Gia tri edit khong hop le: bo check, de nguoi dung sua lai
		((CButton*)GetDlgItem(IDC_CHANMAP_CHECK))->SetCheck(BST_UNCHECKED);
		g_pCurGameWin->m_prBasic.m_bFixChanMap = FALSE;
	}
	g_pCurGameWin->SetGameParams(tab_basic);
	if (g_pCurGameWin->m_bLoadDone)
		g_pCurGameWin->SaveProfile();
}

void CDlgAntiChanMap::OnBnClickedChanMapOption()
{
	if (g_pCurGameWin == NULL) return;
	UpdateData();
	g_pCurGameWin->SetGameParams(tab_basic);
	if (g_pCurGameWin->m_bLoadDone)
		g_pCurGameWin->SaveProfile();
}

void CDlgAntiChanMap::OnEnKillfocusChanMapEdit()
{
	if (g_pCurGameWin == NULL) return;
	if (!UpdateData()) return;
	g_pCurGameWin->SetGameParams(tab_basic);
	if (g_pCurGameWin->m_bLoadDone)
		g_pCurGameWin->SaveProfile();
}

void CDlgAntiChanMap::OnBnClickedChanMapGate()
{
	if (g_pCurGameWin == NULL) return;
	UpdateData();
	g_pCurGameWin->m_prBasic.m_nChanMapId = g_pCurGameWin->m_nMapID;
	g_pCurGameWin->m_prBasic.m_nChanMapX  = g_pCurGameWin->m_nOffX;
	g_pCurGameWin->m_prBasic.m_nChanMapY  = g_pCurGameWin->m_nOffY;
	g_pCurGameWin->SetGameParams(tab_basic);
	if (g_pCurGameWin->m_bLoadDone)
		g_pCurGameWin->SaveProfile();
	UpdateChanMapInfo();
}

void CDlgAntiChanMap::OnBnClickedChanMapDir()
{
	if (g_pCurGameWin == NULL) return;
	UpdateData();
	g_pCurGameWin->m_prBasic.m_nChanDirX = g_pCurGameWin->m_nOffX;
	g_pCurGameWin->m_prBasic.m_nChanDirY = g_pCurGameWin->m_nOffY;
	g_pCurGameWin->SetGameParams(tab_basic);
	if (g_pCurGameWin->m_bLoadDone)
		g_pCurGameWin->SaveProfile();
	UpdateChanMapInfo();
}

void CDlgAntiChanMap::UpdateChanMapInfo()
{
	if (g_pCurGameWin == NULL) return;
	TCHAR szInfo[128];
	if (g_pCurGameWin->m_prBasic.m_nChanMapId == 0 &&
		g_pCurGameWin->m_prBasic.m_nChanMapX == 0 &&
		g_pCurGameWin->m_prBasic.m_nChanMapY == 0)
	{
		_stprintf(szInfo, _T("(chưa đặt)"));
	}
	else
	{
		_stprintf(szInfo, _T("Map %d  Cổng %d,%d  Hướng %d,%d"),
			g_pCurGameWin->m_prBasic.m_nChanMapId,
			g_pCurGameWin->m_prBasic.m_nChanMapX >> 8, g_pCurGameWin->m_prBasic.m_nChanMapY >> 9,
			g_pCurGameWin->m_prBasic.m_nChanDirX >> 8, g_pCurGameWin->m_prBasic.m_nChanDirY >> 9);
	}
	SetDlgItemText(IDC_CHANMAP_INFO_STATIC, szInfo);
}
