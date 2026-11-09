// DlgBuffBua.cpp : implementation file
//

#include "stdafx.h"
#include "../GameDef.h"
#include "DWinProcess.h"
#include "VLAuto.h"
#include "GameCore.h"
#include "GameWindow.h"
#include "DlgBuffBua.h"

// CDlgBuffBua dialog

IMPLEMENT_DYNAMIC(CDlgBuffBua, CDialog)

CDlgBuffBua::CDlgBuffBua(CWnd* pParent /*=NULL*/)
	: CDialog(CDlgBuffBua::IDD, pParent)
{
	m_bLoading = FALSE;
}

CDlgBuffBua::~CDlgBuffBua()
{
}

BEGIN_MESSAGE_MAP(CDlgBuffBua, CDialog)
	ON_NOTIFY(LVN_ITEMCHANGED, IDC_SKILL_BUFFBUA_LIST, &CDlgBuffBua::OnLvnItemchanged)
	ON_BN_CLICKED(IDC_BUFFBUA_ONLYPLAYER_CHECK, &CDlgBuffBua::OnOnlyPlayerClicked)
	ON_EN_KILLFOCUS(IDC_RANGE_BUFFBUA_EDIT, &CDlgBuffBua::OnOnlyPlayerClicked)
END_MESSAGE_MAP()

BOOL CDlgBuffBua::OnInitDialog()
{
	CDialog::OnInitDialog();

	m_BuaList.SubclassDlgItem(IDC_SKILL_BUFFBUA_LIST, this);
	m_BuaList.ModifyStyle(LVS_TYPEMASK | LVS_SORTASCENDING | LVS_SORTDESCENDING,
		LVS_REPORT | LVS_SINGLESEL | LVS_SHOWSELALWAYS | LVS_NOSORTHEADER);
	m_BuaList.SetExtendedStyle(LVS_EX_CHECKBOXES | LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);

	CRect rc;
	m_BuaList.GetClientRect(&rc);
	int nLevelWidth = 50;
	int nNameWidth = rc.Width() - nLevelWidth - GetSystemMetrics(SM_CXVSCROLL);
	if (nNameWidth < 80) nNameWidth = 80;
	m_BuaList.InsertColumn(0, _T("Tên bùa"), LVCFMT_LEFT, nNameWidth);
	m_BuaList.InsertColumn(1, _T("Cấp"), LVCFMT_CENTER, nLevelWidth);

	LoadBuaList();
	return TRUE;
}

void CDlgBuffBua::RefreshFromParams()
{
	if (GetSafeHwnd() == NULL) return;
	LoadBuaList();
}

// Doc skill nhan vat tu bo nho game (giong CDlgSettingSkill::UpdateSkillList), chi giu bua cap >= 1
void CDlgBuffBua::LoadBuaList()
{
	m_bLoading = TRUE;
	m_BuaList.DeleteAllItems();
	CheckDlgButton(IDC_BUFFBUA_ONLYPLAYER_CHECK,
		(g_pCurGameWin && g_pCurGameWin->m_prAttack.m_bBuaOnlyPlayer) ? BST_CHECKED : BST_UNCHECKED);
	if (g_pCurGameWin)
	{	// Chua dat (0) => hien tam tim quai dang dung
		int nRange = g_pCurGameWin->m_prAttack.m_nBuaRange;
		if (nRange < 75) nRange = g_pCurGameWin->m_prAttack.m_nSearchRange;
		SetDlgItemInt(IDC_RANGE_BUFFBUA_EDIT, nRange, FALSE);
	}
	else
		SetDlgItemText(IDC_RANGE_BUFFBUA_EDIT, _T(""));

	if (g_pCurGameWin == NULL)
	{
		m_bLoading = FALSE;
		return;
	}

	DWinProcess winProcess(g_pCurGameWin->m_hWnd);
	LPBYTE lpBaseAdd = NULL, lpCurAdd;
	int nPlayerIndex = 0;

	winProcess.ReadMemory((LPVOID)PLR_BASE_ADD, &lpBaseAdd, sizeof(LPBYTE)); lpBaseAdd += PLAYER_OFFSET;
	lpCurAdd = lpBaseAdd + PLR_IDX_OFFSET;
	winProcess.ReadMemory(lpCurAdd, &nPlayerIndex, sizeof(int));

	NpcSkill Skills[MAX_NPCSKILL];
	memset(Skills, 0, sizeof(Skills));
	if (!winProcess.ReadMemory((LPVOID)NPC_BASE_ADD, (LPVOID)&lpBaseAdd, 4) || nPlayerIndex <= 0
		|| !winProcess.ReadMemory(lpBaseAdd + nPlayerIndex * NPC_DATA_SIZE + offsetof(KNpc, m_Skills), Skills, sizeof(Skills)))
	{
		m_bLoading = FALSE;
		return;
	}

	PARAM_ATTACK &pa = g_pCurGameWin->m_prAttack;
	TCHAR szName[40];
	TCHAR szLevel[16];
	int nRow = 0;
	// Theo thu tu bang bua (thu tu cast), khong theo thu tu o skill
	for (int t = 0; t < BUFFBUA_TABLE_LEN; t++)
	{
		int nSkillId = g_BuffBuaTable[t].nSkillId;
		int nLevel = 0;
		for (int i = 0; i < MAX_NPCSKILL; i++)
		{
			if (Skills[i].SkillId == nSkillId)
			{
				nLevel = Skills[i].SkillLevel;
				break;
			}
		}
		if (nLevel < 1) continue;

		KSkill Skill;
		_stprintf(szName, _T("Skill %d"), nSkillId);
		lpCurAdd = (LPBYTE)SKL_BASE_ADD + (nSkillId - 1) * SKL_DATA_SIZE;
		if (winProcess.ReadMemory(lpCurAdd, (LPVOID)&Skill, sizeof(Skill)) && Skill.m_dwSkillID == (DWORD)nSkillId)
		{
			Skill.m_szSkillName[sizeof(Skill.m_szSkillName) - 1] = 0;
			TCVN3ToUnicode(Skill.m_szSkillName, szName, 40);
		}
		_stprintf(szLevel, _T("%d"), nLevel);

		int nItem = m_BuaList.InsertItem(nRow++, szName);
		m_BuaList.SetItemText(nItem, 1, szLevel);
		m_BuaList.SetItemData(nItem, (DWORD_PTR)nSkillId);

		BOOL bChecked = FALSE;
		for (int k = 0; k < MAX_BUFFBUA; k++)
			if (pa.m_BuaSkills[k] == nSkillId) { bChecked = TRUE; break; }
		m_BuaList.SetCheck(nItem, bChecked);
	}

	m_bLoading = FALSE;
}

void CDlgBuffBua::SaveParams()
{
	if (g_pCurGameWin == NULL || m_BuaList.GetSafeHwnd() == NULL) return;

	PARAM_ATTACK &pa = g_pCurGameWin->m_prAttack;
	int nCount = 0;
	int nNew[MAX_BUFFBUA] = { 0 };
	for (int i = 0; i < m_BuaList.GetItemCount() && nCount < MAX_BUFFBUA; i++)
	{
		if (m_BuaList.GetCheck(i))
			nNew[nCount++] = (int)m_BuaList.GetItemData(i);
	}
	BOOL bOnlyPlayer = (IsDlgButtonChecked(IDC_BUFFBUA_ONLYPLAYER_CHECK) == BST_CHECKED);
	BOOL bOK = FALSE;
	int nRange = (int)GetDlgItemInt(IDC_RANGE_BUFFBUA_EDIT, &bOK, FALSE);
	if (!bOK) nRange = pa.m_nBuaRange;	// O trong / sai => giu gia tri cu
	else if (nRange < 75) nRange = 75;
	else if (nRange > 5000) nRange = 5000;
	if (!memcmp(nNew, pa.m_BuaSkills, sizeof(nNew)) && bOnlyPlayer == pa.m_bBuaOnlyPlayer
		&& nRange == pa.m_nBuaRange) return;

	memcpy(pa.m_BuaSkills, nNew, sizeof(nNew));
	pa.m_bBuaOnlyPlayer = bOnlyPlayer;
	pa.m_nBuaRange = nRange;
	g_pCurGameWin->SetGameParams(tab_attack);
}

void CDlgBuffBua::OnLvnItemchanged(NMHDR* pNMHDR, LRESULT* pResult)
{
	LPNMLISTVIEW pNMLV = reinterpret_cast<LPNMLISTVIEW>(pNMHDR);
	*pResult = 0;
	if (m_bLoading) return;
	// Chi khi doi trang thai checkbox (state image index nam o LVIS_STATEIMAGEMASK)
	if (!(pNMLV->uChanged & LVIF_STATE)) return;
	if ((pNMLV->uOldState & LVIS_STATEIMAGEMASK) == (pNMLV->uNewState & LVIS_STATEIMAGEMASK)) return;
	SaveParams();
}

void CDlgBuffBua::OnOnlyPlayerClicked()
{
	if (!m_bLoading) SaveParams();
}

void CDlgBuffBua::OnOK()
{
	SaveParams();
	CDialog::OnOK();
}

void CDlgBuffBua::OnCancel()
{
	// Thay doi da ap dung ngay khi check => dong cua so cung luu lai
	SaveParams();
	CDialog::OnCancel();
}
