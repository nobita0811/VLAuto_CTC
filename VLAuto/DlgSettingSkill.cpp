// DlgSettingSkill.cpp : implementation file
//

#include "stdafx.h"
#include "../GameDef.h"
#include "DWinProcess.h"
#include "VLAuto.h"
#include "GameCore.h"
#include "GameWindow.h"
#include "DlgSettingSkill.h"

#define MIN_TIMEI 200
#define MIN_TIMED 2000
#define UNSET_SKILL _T("[ Không thiết lập ]")

// CDlgSettingSkill dialog

IMPLEMENT_DYNAMIC(CDlgSettingSkill, CDialog)

CDlgSettingSkill::CDlgSettingSkill(CWnd* pParent /*=NULL*/)
	: CDialog(CDlgSettingSkill::IDD, pParent)
{
}

CDlgSettingSkill::~CDlgSettingSkill()
{
}

void CDlgSettingSkill::DoDataExchange(CDataExchange* pDX)
{
	DDX_Control(pDX, IDC_SKILL_LIST, m_SkillList);

	CDialog::DoDataExchange(pDX);
	if (g_pCurGameWin == NULL)
	{
		m_SkillList.SetReadOnly();
		return;
	}

	//Textboxes
	DDX_Text(pDX, IDC_SKILL1_EDIT, g_pCurGameWin->m_prAttack.m_AlterSkill[0].nAlterValue);
	DDX_Text(pDX, IDC_SKILL2_EDIT, g_pCurGameWin->m_prAttack.m_AlterSkill[1].nAlterValue);

	//ComboBoxes
	DDX_CBIndex(pDX, IDC_SKILL1_COMBO, g_pCurGameWin->m_prAttack.m_AlterSkill[0].nUseSkill);
	DDX_CBIndex(pDX, IDC_SKILL2_COMBO, g_pCurGameWin->m_prAttack.m_AlterSkill[1].nUseSkill);
	DDX_CBIndex(pDX, IDC_SKILL1A_COMBO, g_pCurGameWin->m_prAttack.m_AlterSkill[0].nAlterType);
	DDX_CBIndex(pDX, IDC_SKILL2A_COMBO, g_pCurGameWin->m_prAttack.m_AlterSkill[1].nAlterType);

	//UI Refresh
	int nSkillIdx1 = ((CComboBox*)GetDlgItem(IDC_SKILL1_COMBO))->GetCurSel();
	int nSkillIdx2 = ((CComboBox*)GetDlgItem(IDC_SKILL2_COMBO))->GetCurSel();
	BOOL bEnable = (!nSkillIdx1 && !nSkillIdx2);

	GetDlgItem(IDC_SKILL1_EDIT)->EnableWindow(bEnable);
	GetDlgItem(IDC_SKILL1A_COMBO)->EnableWindow(bEnable);
	GetDlgItem(IDC_SKILL2_EDIT)->EnableWindow(bEnable);
	GetDlgItem(IDC_SKILL2A_COMBO)->EnableWindow(bEnable);

	m_SkillList.SetReadOnly(0);
	UpdateSkillList(pDX->m_bSaveAndValidate);
}

BEGIN_MESSAGE_MAP(CDlgSettingSkill, CDialog)
	ON_CONTROL_RANGE(CBN_SELCHANGE, 1000, 2000, &CDlgSettingSkill::OnCbnSelchanged)
	ON_CONTROL_RANGE(EN_KILLFOCUS, 1000, 2000, &CDlgSettingSkill::OnEnKillfocus)
	ON_MESSAGE(WM_ONSETTEXT, &CDlgSettingSkill::OnSetTextSkillList)
END_MESSAGE_MAP()

BOOL CDlgSettingSkill::OnInitDialog()
{
	m_SkillList.SubclassDlgItem(IDC_SKILL_LIST, this);

	m_SkillList.InsertColumn(0, _T("Thiết lập"), LVCFMT_LEFT, 75);
	m_SkillList.InsertColumn(1, _T("Tên chiêu thức"), LVCFMT_LEFT, 185);
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

	// Combo da co du lieu => moi nap gia tri
	CDialog::OnInitDialog();

	if (m_ToolTip.Create(this))
	{
		m_ToolTip.AddTool(GetDlgItem(IDC_SKILL1_COMBO), _T("Phím tắt chiêu thức"));
		m_ToolTip.AddTool(GetDlgItem(IDC_SKILL2_COMBO), _T("Phím tắt chiêu thức"));
		m_ToolTip.AddTool(GetDlgItem(IDC_SKILL1_EDIT), _T("Mức thời gian, sinh lực, nội lực (dưới)"));
		m_ToolTip.AddTool(GetDlgItem(IDC_SKILL2_EDIT), _T("Mức thời gian, sinh lực, nội lực (trên)"));
		m_ToolTip.AddTool(GetDlgItem(IDC_SKILL1A_COMBO), _T("Chế độ chuyển chiêu / xuất chiêu"));
		m_ToolTip.AddTool(GetDlgItem(IDC_SKILL2A_COMBO), _T("Chế độ chuyển chiêu / xuất chiêu"));
	}

	return TRUE;
}

BOOL CDlgSettingSkill::PreTranslateMessage(MSG* pMsg)
{
	m_ToolTip.RelayEvent(pMsg);
	return CDialog::PreTranslateMessage(pMsg);
}

void CDlgSettingSkill::SaveParams()
{
	if (g_pCurGameWin != NULL)
	{
		UpdateData();
		g_pCurGameWin->SetGameParams(tab_attack);
	}
}

void CDlgSettingSkill::OnOK()
{
	SaveParams();
	CDialog::OnOK();
}

void CDlgSettingSkill::OnCancel()
{
	// Thay doi da ap dung ngay khi chon => dong cua so cung luu lai
	SaveParams();
	CDialog::OnCancel();
}

void CDlgSettingSkill::OnCbnSelchanged(UINT nID)
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

	SaveParams();
}

void CDlgSettingSkill::OnEnKillfocus(UINT nID)
{
	if (!IsWindowVisible()) return;	// dang dong cua so (EndDialog) => da luu o OnOK/OnCancel
	if (nID == IDC_SKILL1_EDIT || nID == IDC_SKILL2_EDIT)
		SaveParams();
}

LRESULT CDlgSettingSkill::OnSetTextSkillList(WPARAM wParam, LPARAM lParam)
{
	SaveParams();
	return 0;
}

void CDlgSettingSkill::UpdateSkillList(BOOL bSave)
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
