// DlgAttack.cpp : implementation file
//

#include "stdafx.h"
#include "../GameDef.h"
#include "DWinProcess.h"
#include "VLAuto.h"
#include "GameCore.h"
#include "GameWindow.h"
#include "DlgAttack.h"

// CDlgAttack dialog

IMPLEMENT_DYNAMIC(CDlgAttack, CDialog)

CDlgAttack::CDlgAttack(CWnd* pParent /*=NULL*/)
	: CDialog(CDlgAttack::IDD, pParent)
{
}

CDlgAttack::~CDlgAttack()
{
}

void CDlgAttack::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	GetDlgItem(IDC_SETTING_SKILL_BTN)->EnableWindow(g_pCurGameWin != NULL);

	// Cua so Cai dat skill dang mo => lam moi theo nhan vat hien tai
	if (m_dlgSkill.GetSafeHwnd() != NULL && !pDX->m_bSaveAndValidate)
	{
		if (g_pCurGameWin == NULL)
			m_dlgSkill.EndDialog(IDCANCEL);
		else
			m_dlgSkill.UpdateData(FALSE);
	}

	if (g_pCurGameWin == NULL)
		return;

	//Checkboxes
	DDX_Check(pDX, IDC_AUTOHIT_CHECK, g_pCurGameWin->m_prAttack.m_bAutoHit);
	DDX_Check(pDX, IDC_HITCLOSE_CHECK1, g_pCurGameWin->m_prAttack.m_bHitClose);
	DDX_Check(pDX, IDC_AVOIDBOSS_CHECK, g_pCurGameWin->m_prAttack.m_bAvoidBoss);
	DDX_Check(pDX, IDC_ONLYBLUEBOSS_CHECK, g_pCurGameWin->m_prAttack.m_bOnlyBlueBoss);
	DDX_Check(pDX, IDC_HITCLOSE_CHECK2, g_pCurGameWin->m_prAttack.m_bDefHitClose);
	DDX_Check(pDX, IDC_ALWAYSPK_CHECK, g_pCurGameWin->m_prAttack.m_bAlwaysF9);
	DDX_Check(pDX, IDC_USEHORSE_CHECK, g_pCurGameWin->m_prAttack.m_bDefHorseMode);

	//Textboxes
	DDX_Text(pDX, IDC_SEARCHRANGE_EDIT, g_pCurGameWin->m_prAttack.m_nSearchRange);
	DDX_Text(pDX, IDC_SKILLRANGE_EDIT1, g_pCurGameWin->m_prAttack.m_nSkillRange);
	DDX_Text(pDX, IDC_DEFENCERANGE_EDIT, g_pCurGameWin->m_prAttack.m_nDefenceRange);
	DDX_Text(pDX, IDC_SKILLRANGE_EDIT2, g_pCurGameWin->m_prAttack.m_nDefSkillRange);

	//ComboBoxes
	DDX_CBIndex(pDX, IDC_USEHORSE_COMBO1, g_pCurGameWin->m_prAttack.m_nHorseMode);
	DDX_CBIndex(pDX, IDC_DEFENCE_COMBO, g_pCurGameWin->m_prAttack.m_nAutoDefence);

	//UI Refresh
	BOOL bChecked;
	bChecked = ((CButton*)GetDlgItem(IDC_AUTOHIT_CHECK))->GetCheck();
	GetDlgItem(IDC_SEARCHRANGE_EDIT)->EnableWindow(!bChecked);
	bChecked = ((CButton*)GetDlgItem(IDC_HITCLOSE_CHECK1))->GetCheck();
	GetDlgItem(IDC_SKILLRANGE_EDIT1)->EnableWindow(!bChecked);
	bChecked = ((CButton*)GetDlgItem(IDC_HITCLOSE_CHECK2))->GetCheck();
	GetDlgItem(IDC_SKILLRANGE_EDIT2)->EnableWindow(!bChecked);

	int nAutoDef = ((CComboBox*)GetDlgItem(IDC_DEFENCE_COMBO))->GetCurSel();
	GetDlgItem(IDC_DEFENCERANGE_EDIT)->EnableWindow(!nAutoDef);
}

BEGIN_MESSAGE_MAP(CDlgAttack, CDialog)
	ON_CONTROL_RANGE(BN_CLICKED, 1000, 2000, &CDlgAttack::OnButtonsClicked)
	ON_CONTROL_RANGE(CBN_SELCHANGE, 1000, 2000, &CDlgAttack::OnCbnSelchanged)
END_MESSAGE_MAP()

BOOL CDlgAttack::OnInitDialog()
{
	CDialog::OnInitDialog();

	CComboBox* pComboCtrl;

	pComboCtrl = (CComboBox*)GetDlgItem(IDC_USEHORSE_COMBO1);
	pComboCtrl->AddString(_T("Ngựa-Auto"));
	pComboCtrl->AddString(_T("Luôn xuống"));
	pComboCtrl->AddString(_T("Luôn lên"));

	pComboCtrl = (CComboBox*)GetDlgItem(IDC_DEFENCE_COMBO);
	pComboCtrl->AddString(_T("Bỏ qua"));
	pComboCtrl->AddString(_T("Đánh trả"));
	pComboCtrl->AddString(_T("Dùng TĐP"));
	pComboCtrl->AddString(_T("Thoát game"));	// Gặp kẻ thù trong tầm: TerminateProcess game ngay

	if (m_ToolTip.Create(this))
	{
		m_ToolTip.AddTool(GetDlgItem(IDC_AUTOHIT_CHECK), _T("Tự đánh quái"));
		m_ToolTip.AddTool(GetDlgItem(IDC_SEARCHRANGE_EDIT), _T("Phạm vi tìm quái"));
		m_ToolTip.AddTool(GetDlgItem(IDC_USEHORSE_COMBO1), _T("Chế độ lên/xuống ngựa khi đánh quái"));
		m_ToolTip.AddTool(GetDlgItem(IDC_HITCLOSE_CHECK1), _T("Áp sát quái trước khi đánh"));
		m_ToolTip.AddTool(GetDlgItem(IDC_SKILLRANGE_EDIT1), _T("Phạm vi áp sát quái"));
		m_ToolTip.AddTool(GetDlgItem(IDC_AVOIDBOSS_CHECK), _T("Tránh tất cả các loại Boss vàng"));
		m_ToolTip.AddTool(GetDlgItem(IDC_ONLYBLUEBOSS_CHECK), _T("Chỉ tìm và đánh boss xanh (tên quái màu xanh), bỏ qua quái thường và boss vàng"));

		m_ToolTip.AddTool(GetDlgItem(IDC_DEFENCE_COMBO), _T("Chế độ Tự vệ"));
		m_ToolTip.AddTool(GetDlgItem(IDC_DEFENCERANGE_EDIT), _T("Phạm vi tìm kẻ thù"));
		m_ToolTip.AddTool(GetDlgItem(IDC_HITCLOSE_CHECK2), _T("Áp sát kẻ thù trước khi đánh"));
		m_ToolTip.AddTool(GetDlgItem(IDC_SKILLRANGE_EDIT2), _T("Phạm vi áp sát kẻ thù"));
		m_ToolTip.AddTool(GetDlgItem(IDC_USEHORSE_CHECK), _T("Tự lên/xuống ngựa khi đánh kẻ thù"));
		m_ToolTip.AddTool(GetDlgItem(IDC_ALWAYSPK_CHECK), _T("Luôn để chế độ Chiến đấu"));

		m_ToolTip.AddTool(GetDlgItem(IDC_SETTING_SKILL_BTN), _T("Thiết lập chiêu thức, chuyển chiêu"));
	}

	return FALSE;
}

BOOL CDlgAttack::PreTranslateMessage(MSG* pMsg)
{
	m_ToolTip.RelayEvent(pMsg);
	return CDialog::PreTranslateMessage(pMsg);
}

void CDlgAttack::OnButtonsClicked(UINT nID)
{
	if (g_pCurGameWin == NULL) return;
	if (nID == IDC_SETTING_SKILL_BTN)
	{
		m_dlgSkill.DoModal();
		return;
	}

	int nValue = 0;
	switch (nID)
	{
	case IDC_HITCLOSE_CHECK1:
		nValue = GetDlgItemInt(IDC_SKILLRANGE_EDIT1);
		if (nValue < 64) SetDlgItemInt(IDC_SKILLRANGE_EDIT1, 64);
	case IDC_HITCLOSE_CHECK2:
		nValue = GetDlgItemInt(IDC_SKILLRANGE_EDIT2);
		if (nValue < 64) SetDlgItemInt(IDC_SKILLRANGE_EDIT2, 64);
	}
	UpdateData();
	g_pCurGameWin->SetGameParams(tab_attack);
}

void CDlgAttack::OnCbnSelchanged(UINT nID)
{
	if (g_pCurGameWin != NULL)
	{
		UpdateData();
		g_pCurGameWin->SetGameParams(tab_attack);
	}
}
