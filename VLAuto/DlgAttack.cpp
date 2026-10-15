// DlgAttack.cpp : implementation file
//

#include "stdafx.h"
#include "../GameDef.h"
#include "DWinProcess.h"
#include "VLAuto.h"
#include "GameCore.h"
#include "GameWindow.h"
#include "DlgAttack.h"

#define MIN_TIMEI 200
#define MIN_TIMED 2000
#define UNSET_SKILL _T("[ Không thiết lập ]")

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
	DDX_Control(pDX, IDC_SKILL_LIST, m_SkillList);

	CDialog::DoDataExchange(pDX);
	if (g_pCurGameWin == NULL)
	{
		m_SkillList.SetReadOnly();
		return;
	}

	//Checkboxes
	DDX_Check(pDX, IDC_AUTOHIT_CHECK, g_pCurGameWin->m_prAttack.m_bAutoHit);
	DDX_Check(pDX, IDC_HITCLOSE_CHECK1, g_pCurGameWin->m_prAttack.m_bHitClose);
	DDX_Check(pDX, IDC_AVOIDBOSS_CHECK, g_pCurGameWin->m_prAttack.m_bAvoidBoss);
	DDX_Check(pDX, IDC_HITCLOSE_CHECK2, g_pCurGameWin->m_prAttack.m_bDefHitClose);
	DDX_Check(pDX, IDC_ALWAYSPK_CHECK, g_pCurGameWin->m_prAttack.m_bAlwaysF9);
	DDX_Check(pDX, IDC_USEHORSE_CHECK, g_pCurGameWin->m_prAttack.m_bDefHorseMode);

	//Textboxes
	DDX_Text(pDX, IDC_SEARCHRANGE_EDIT, g_pCurGameWin->m_prAttack.m_nSearchRange);
	DDX_Text(pDX, IDC_SKILLRANGE_EDIT1, g_pCurGameWin->m_prAttack.m_nSkillRange);
	DDX_Text(pDX, IDC_DEFENCERANGE_EDIT, g_pCurGameWin->m_prAttack.m_nDefenceRange);
	DDX_Text(pDX, IDC_SKILLRANGE_EDIT2, g_pCurGameWin->m_prAttack.m_nDefSkillRange);
	DDX_Text(pDX, IDC_SKILL1_EDIT, g_pCurGameWin->m_prAttack.m_AlterSkill[0].nAlterValue);
	DDX_Text(pDX, IDC_SKILL2_EDIT, g_pCurGameWin->m_prAttack.m_AlterSkill[1].nAlterValue);

	//ComboBoxes
	DDX_CBIndex(pDX, IDC_USEHORSE_COMBO1, g_pCurGameWin->m_prAttack.m_nHorseMode);
	DDX_CBIndex(pDX, IDC_DEFENCE_COMBO, g_pCurGameWin->m_prAttack.m_nAutoDefence);
	DDX_CBIndex(pDX, IDC_SKILL1_COMBO, g_pCurGameWin->m_prAttack.m_AlterSkill[0].nUseSkill);
	DDX_CBIndex(pDX, IDC_SKILL2_COMBO, g_pCurGameWin->m_prAttack.m_AlterSkill[1].nUseSkill);
	DDX_CBIndex(pDX, IDC_SKILL1A_COMBO, g_pCurGameWin->m_prAttack.m_AlterSkill[0].nAlterType);
	DDX_CBIndex(pDX, IDC_SKILL2A_COMBO, g_pCurGameWin->m_prAttack.m_AlterSkill[1].nAlterType);

	//UI Refresh
	BOOL bChecked;
	bChecked = ((CButton*)GetDlgItem(IDC_AUTOHIT_CHECK))->GetCheck();
	GetDlgItem(IDC_SEARCHRANGE_EDIT)->EnableWindow(!bChecked);
	bChecked = ((CButton*)GetDlgItem(IDC_HITCLOSE_CHECK1))->GetCheck();
	GetDlgItem(IDC_SKILLRANGE_EDIT1)->EnableWindow(!bChecked);
	bChecked = ((CButton*)GetDlgItem(IDC_HITCLOSE_CHECK2))->GetCheck();
	GetDlgItem(IDC_SKILLRANGE_EDIT2)->EnableWindow(!bChecked);

	int nSkillIdx1 = ((CComboBox*)GetDlgItem(IDC_SKILL1_COMBO))->GetCurSel();
	int nSkillIdx2 = ((CComboBox*)GetDlgItem(IDC_SKILL2_COMBO))->GetCurSel();
	BOOL bEnable = (!nSkillIdx1 && !nSkillIdx2);

	GetDlgItem(IDC_SKILL1_EDIT)->EnableWindow(bEnable);
	GetDlgItem(IDC_SKILL1A_COMBO)->EnableWindow(bEnable);
	GetDlgItem(IDC_SKILL2_EDIT)->EnableWindow(bEnable);
	GetDlgItem(IDC_SKILL2A_COMBO)->EnableWindow(bEnable);

	int nAutoDef = ((CComboBox*)GetDlgItem(IDC_DEFENCE_COMBO))->GetCurSel();
	GetDlgItem(IDC_DEFENCERANGE_EDIT)->EnableWindow(!nAutoDef);

	m_SkillList.SetReadOnly(0);
	UpdateSkillList(pDX->m_bSaveAndValidate);
}

BEGIN_MESSAGE_MAP(CDlgAttack, CDialog)
	ON_CONTROL_RANGE(BN_CLICKED, 1000, 2000, &CDlgAttack::OnButtonsClicked)
	ON_CONTROL_RANGE(CBN_SELCHANGE, 1000, 2000, &CDlgAttack::OnCbnSelchanged)
	ON_MESSAGE(WM_ONSETTEXT, &CDlgAttack::OnSetTextSkillList)
END_MESSAGE_MAP()

BOOL CDlgAttack::OnInitDialog()
{
	CDialog::OnInitDialog();

	m_SkillList.InsertColumn(0, _T("Thiết lập"), LVCFMT_LEFT, 70);
	m_SkillList.InsertColumn(1, _T("Tên chiêu thức"), LVCFMT_LEFT, 125);
	m_SkillList.SetExtendedStyle(LVS_EX_GRIDLINES|LVS_EX_FULLROWSELECT);
	CImageList imgList;
	imgList.Create(1, 16, ILC_COLOR4, 1, 1);
	m_SkillList.SetImageList(&imgList, LVSIL_SMALL);
	m_SkillList.SetColumnType(column_combo);
	m_SkillList.InsertComboText(UNSET_SKILL);
	m_SkillList.InsertItem(0, _T("Hỗ trợ #1"));
	m_SkillList.InsertItem(1, _T("Hỗ trợ #2"));
	m_SkillList.InsertItem(2, _T("Hỗ trợ #3"));
	m_SkillList.InsertItem(3, _T("Tay trái"));
	m_SkillList.InsertItem(4, _T("Tay phải"));
	m_SkillList.InsertItem(5, _T("Đánh Boss"));
	m_SkillList.InsertItem(6, _T("Tự vệ"));

	CComboBox* pComboCtrl;

	pComboCtrl = (CComboBox*)GetDlgItem(IDC_USEHORSE_COMBO1);
	pComboCtrl->AddString(_T("Ngựa-Auto"));
	pComboCtrl->AddString(_T("Luôn xuống"));
	pComboCtrl->AddString(_T("Luôn lên"));

	pComboCtrl = (CComboBox*)GetDlgItem(IDC_DEFENCE_COMBO);
	pComboCtrl->AddString(_T("Bỏ qua"));
	pComboCtrl->AddString(_T("Đánh trả"));
	pComboCtrl->AddString(_T("Dùng TĐP"));

	pComboCtrl = (CComboBox*)GetDlgItem(IDC_SKILL1_COMBO);
	pComboCtrl->AddString(_T("Bỏ qua"));
	pComboCtrl->AddString(_T("Phím Q/F2"));
	pComboCtrl->AddString(_T("Phím W/F3"));
	pComboCtrl->AddString(_T("Phím E/F4"));
	pComboCtrl->AddString(_T("Phím A/F5"));
	pComboCtrl->AddString(_T("Phím S/F6"));

	pComboCtrl = (CComboBox*)GetDlgItem(IDC_SKILL2_COMBO);
	pComboCtrl->AddString(_T("Bỏ qua"));
	pComboCtrl->AddString(_T("Phím Q/F2"));
	pComboCtrl->AddString(_T("Phím W/F3"));
	pComboCtrl->AddString(_T("Phím E/F4"));
	pComboCtrl->AddString(_T("Phím A/F5"));
	pComboCtrl->AddString(_T("Phím S/F6"));

	pComboCtrl = (CComboBox*)GetDlgItem(IDC_SKILL1A_COMBO);
	pComboCtrl->AddString(_T("Xuất chiêu"));
	pComboCtrl->AddString(_T("Chuyển t.gian"));
	pComboCtrl->AddString(_T("Chuyển NL <"));
	pComboCtrl->AddString(_T("Chuyển SL <"));

	pComboCtrl = (CComboBox*)GetDlgItem(IDC_SKILL2A_COMBO);
	pComboCtrl->AddString(_T("Xuất chiêu"));
	pComboCtrl->AddString(_T("Chuyển t.gian"));
	pComboCtrl->AddString(_T("Chuyển NL >"));
	pComboCtrl->AddString(_T("Chuyển SL >"));

	if (m_ToolTip.Create(this))
	{
		m_ToolTip.AddTool(GetDlgItem(IDC_AUTOHIT_CHECK), _T("Tự đánh quái"));
		m_ToolTip.AddTool(GetDlgItem(IDC_SEARCHRANGE_EDIT), _T("Phạm vi tìm quái"));
		m_ToolTip.AddTool(GetDlgItem(IDC_USEHORSE_COMBO1), _T("Chế độ lên/xuống ngựa khi đánh quái"));
		m_ToolTip.AddTool(GetDlgItem(IDC_HITCLOSE_CHECK1), _T("Áp sát quái trước khi đánh"));
		m_ToolTip.AddTool(GetDlgItem(IDC_SKILLRANGE_EDIT1), _T("Phạm vi áp sát quái"));
		m_ToolTip.AddTool(GetDlgItem(IDC_AVOIDBOSS_CHECK), _T("Tránh tất cả các loại Boss vàng"));

		m_ToolTip.AddTool(GetDlgItem(IDC_DEFENCE_COMBO), _T("Chế độ Tự vệ"));
		m_ToolTip.AddTool(GetDlgItem(IDC_DEFENCERANGE_EDIT), _T("Phạm vi tìm kẻ thù"));
		m_ToolTip.AddTool(GetDlgItem(IDC_HITCLOSE_CHECK2), _T("Áp sát kẻ thù trước khi đánh"));
		m_ToolTip.AddTool(GetDlgItem(IDC_SKILLRANGE_EDIT2), _T("Phạm vi áp sát kẻ thù"));
		m_ToolTip.AddTool(GetDlgItem(IDC_USEHORSE_CHECK), _T("Tự lên/xuống ngựa khi đánh kẻ thù"));
		m_ToolTip.AddTool(GetDlgItem(IDC_ALWAYSPK_CHECK), _T("Luôn để chế độ Chiến đấu"));

		m_ToolTip.AddTool(GetDlgItem(IDC_SKILL1_COMBO), _T("Phím tắt chiêu thức"));
		m_ToolTip.AddTool(GetDlgItem(IDC_SKILL2_COMBO), _T("Phím tắt chiêu thức"));
		m_ToolTip.AddTool(GetDlgItem(IDC_SKILL1_EDIT), _T("Mức thời gian, sinh lực, nội lực (dưới)"));
		m_ToolTip.AddTool(GetDlgItem(IDC_SKILL2_EDIT), _T("Mức thời gian, sinh lực, nội lực (trên)"));
		m_ToolTip.AddTool(GetDlgItem(IDC_SKILL1A_COMBO), _T("Chế độ chuyển chiêu / xuất chiêu"));
		m_ToolTip.AddTool(GetDlgItem(IDC_SKILL2A_COMBO), _T("Chế độ chuyển chiêu / xuất chiêu"));
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
	int nIndex = ((CComboBox*)GetDlgItem(nID))->GetCurSel();
	if (nID == IDC_SKILL1_COMBO)
	{
		int nAltType = ((CComboBox*)GetDlgItem(IDC_SKILL1A_COMBO))->GetCurSel();
		int nValue = GetDlgItemInt(IDC_SKILL1_EDIT);
		if (nAltType == 0 && nValue < MIN_TIMED)
			SetDlgItemInt(IDC_SKILL1_EDIT, MIN_TIMED);
		if (nAltType == 1 && nValue < MIN_TIMEI)
			SetDlgItemInt(IDC_SKILL1_EDIT, MIN_TIMEI);
	}
	else if (nID == IDC_SKILL2_COMBO)
	{
		int nAltType = ((CComboBox*)GetDlgItem(IDC_SKILL2A_COMBO))->GetCurSel();
		int nValue = GetDlgItemInt(IDC_SKILL2_EDIT);
		if (nAltType == 0 && nValue < MIN_TIMED)
			SetDlgItemInt(IDC_SKILL2_EDIT, MIN_TIMED);
		if (nAltType == 1 && nValue < MIN_TIMEI)
			SetDlgItemInt(IDC_SKILL2_EDIT, MIN_TIMEI);
	}
	else if (nID == IDC_SKILL1A_COMBO || nID == IDC_SKILL2A_COMBO)
	{
		((CComboBox*)GetDlgItem(IDC_SKILL1A_COMBO))->SetCurSel(nIndex);
		((CComboBox*)GetDlgItem(IDC_SKILL2A_COMBO))->SetCurSel(nIndex);
	}

	if (g_pCurGameWin != NULL)
	{
		UpdateData();
		g_pCurGameWin->SetGameParams(tab_attack);
	}
}

LRESULT CDlgAttack::OnSetTextSkillList(WPARAM wParam, LPARAM lParam)
{
	if (g_pCurGameWin != NULL)
	{
		UpdateData();
		g_pCurGameWin->SetGameParams(tab_attack);
	}
	return 0;
}

void CDlgAttack::UpdateSkillList(BOOL bSave)
{
	if (bSave)
	{
		for (int i=0; i < skill_total; i++)
		{
			int nSkillID = (int)m_SkillList.GetItemData(i);
			if (nSkillID < 0) nSkillID = 0;
			g_pCurGameWin->m_prAttack.m_SettingSkills[i] = nSkillID;
		}
	}
	else
	{
		TCHAR szSkillName[40] = _T("");
		int nPlayerIndex; KNpc curPlayer;

		LPBYTE lpBaseAdd, lpCurAdd;
		DWinProcess winProcess(g_pCurGameWin->m_hWnd);

		winProcess.ReadMemory((LPVOID)PLR_BASE_ADD, &lpBaseAdd, sizeof(LPBYTE)); lpBaseAdd += PLAYER_OFFSET;
		lpCurAdd = lpBaseAdd + PLR_IDX_OFFSET;
		winProcess.ReadMemory(lpCurAdd, &nPlayerIndex, sizeof(int));

		if (!winProcess.ReadMemory((LPVOID)NPC_BASE_ADD, (LPVOID)&lpBaseAdd, 4))
			return;

		lpCurAdd = lpBaseAdd + nPlayerIndex * NPC_DATA_SIZE;
		winProcess.ReadMemory(lpCurAdd, (LPVOID)&curPlayer, sizeof(curPlayer));
		
		m_SkillList.ResetComboText();
		m_SkillList.InsertComboText(UNSET_SKILL);
		for (int j=0; j < skill_total; j++)
		{
			m_SkillList.SetItemText(j, 1, UNSET_SKILL);
			m_SkillList.SetItemData(j, 0);
		}

		KSkill Skill;
		for (int i=0; i < MAX_NPCSKILL; i++)
		{
			int nSkillId = curPlayer.m_Skills[i].SkillId;
			if (nSkillId < 1 || nSkillId >= MAX_SKILL) continue;	// m_Skills co the co o trong o giua
			if (nSkillId == 53 || (nSkillId >= 229 && nSkillId <= 232)) continue;	// Danh thuong theo vu khi
			lpCurAdd = (LPBYTE)SKL_BASE_ADD + (nSkillId - 1) * SKL_DATA_SIZE;

			if (!winProcess.ReadMemory(lpCurAdd, (LPVOID)&Skill, sizeof(Skill))) continue;
			if (Skill.m_dwSkillID != (DWORD)nSkillId) continue;

			// m_bIsPhysics = 1 voi moi chieu ngoai cong (Thieu Lam, Thien Vuong, Duong Mon...) => khong dung de loc.
			// 10-10: chieu moi (ID 554+) co MaxLevel 10 => chi bo MaxLevel <= 1 (danh thuong, khinh cong, phao hoa)
			if (Skill.m_nSkillType == SKILL_TYPE_PASSIVE) continue;
			if (Skill.m_nMaxLevel <= 1) continue;
			TCVN3ToUnicode(Skill.m_szSkillName, szSkillName, 40);

			m_SkillList.InsertComboText(szSkillName, curPlayer.m_Skills[i].SkillId);

			for (int j=0; j < skill_total; j++)
			{
				if (g_pCurGameWin->m_prAttack.m_SettingSkills[j] == curPlayer.m_Skills[i].SkillId)
				{
					m_SkillList.SetItemText(j, 1, szSkillName);
					m_SkillList.SetItemData(j, curPlayer.m_Skills[i].SkillId);
					break;
				}
			}
		}
	}
}
