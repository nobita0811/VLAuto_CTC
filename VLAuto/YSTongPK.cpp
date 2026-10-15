// YSTongPK.cpp : implementation file
//

#include "stdafx.h"
#include "VLAuto.h"
#include "GameWindow.h"
#include "DlgYeSou.h"
#include "DlgPKQFilter.h"
#include "YSTongPK.h"

// CYSTongPK dialog

IMPLEMENT_DYNAMIC(CYSTongPK, CPropertyPage)

CYSTongPK::CYSTongPK()
	: CPropertyPage(CYSTongPK::IDD)
{
	m_pTongPKOpt = NULL;
	m_nPointX = 0;
	m_nPointY = 0;
}

CYSTongPK::~CYSTongPK()
{
}

void CYSTongPK::DoDataExchange(CDataExchange* pDX)
{
	DDX_Control(pDX, IDC_HQUEST_LIST, m_QuestList);

	CPropertyPage::DoDataExchange(pDX);
	if (!m_pTongPKOpt) return;

	UpdateQuestList(pDX->m_bSaveAndValidate);

	DDX_CBIndex(pDX, IDC_DRIVER_COMBO,  m_pTongPKOpt->m_nDriverOpt);
	DDX_CBIndex(pDX, IDC_QUESTKIND_COMBO,  m_pTongPKOpt->m_nChooseLB);

	DDX_Check(pDX, IDC_MOVE_CHECK, m_pTongPKOpt->m_bMovePt);
	DDX_Check(pDX, IDC_AUTOCHANGE_CHECK, m_pTongPKOpt->m_bAutoChange);
	DDX_Check(pDX, IDC_USEWITHYS_CHECK, m_pTongPKOpt->m_bUseWithYS);
	DDX_Check(pDX, IDC_KILLOTHER_CHECK, m_pTongPKOpt->m_bKillOther);
	DDX_Check(pDX, IDC_USEAUTOLOGIN_CHECK, m_pTongPKOpt->m_bAutoLogin);
	DDX_Check(pDX, IDC_EXITVICTIM_CHECK, m_pTongPKOpt->m_bExitVictim);

	DDX_Text(pDX, IDC_MAXQUEST_EDIT, m_pTongPKOpt->m_nMaxQuestCount);
	DDV_MinMaxInt(pDX, m_pTongPKOpt->m_nMaxQuestCount, 0, 99);
	DDX_Text(pDX, IDC_USEAUTOLOGIN_EDIT, m_pTongPKOpt->m_nAutoLoginIdx);
	if (m_pTongPKOpt->m_bAutoLogin)
		DDV_MinMaxInt(pDX, m_pTongPKOpt->m_nAutoLoginIdx, 1, 999);

	TCHAR szVictimName[40];
	if (!pDX->m_bSaveAndValidate)
	{
		m_nPointX = m_pTongPKOpt->m_nPointX;
		m_nPointY = m_pTongPKOpt->m_nPointY;
		SetDlgItemInt(IDC_POINTX_EDIT, m_nPointX >> 8);
		SetDlgItemInt(IDC_POINTY_EDIT, m_nPointY >> 9);
		MultiByteToWideChar(CP_ACP, 0, m_pTongPKOpt->m_szVictimName, -1, szVictimName, 32);
		SetDlgItemText(IDC_VICTIM_COMBO, szVictimName);
	}
	else
	{
		m_pTongPKOpt->m_nPointX = m_nPointX;
		m_pTongPKOpt->m_nPointY = m_nPointY;
		GetDlgItemText(IDC_VICTIM_COMBO, szVictimName, 32);
		WideCharToMultiByte(CP_ACP, 0, szVictimName, -1, m_pTongPKOpt->m_szVictimName, 32, 0, 0);
	}
}

BEGIN_MESSAGE_MAP(CYSTongPK, CPropertyPage)
	ON_CONTROL_RANGE(BN_CLICKED, 1000, 2000, &CYSTongPK::OnButtonsClicked)
END_MESSAGE_MAP()


// CYSTongPK message handlers

BOOL CYSTongPK::OnInitDialog()
{
	CPropertyPage::OnInitDialog();

	m_QuestList.SetExtendedStyle(LVS_EX_GRIDLINES|LVS_EX_FULLROWSELECT);
	m_QuestList.InsertColumn(0, _T("Tác phường"), LVCFMT_LEFT, 73);
	m_QuestList.InsertColumn(1, _T("Cấp"), LVCFMT_CENTER, 34);
	m_QuestList.InsertColumn(2, _T("Thư"), LVCFMT_CENTER, 34);

	CComboBox *pGetDriver = (CComboBox*)GetDlgItem(IDC_DRIVER_COMBO);
	pGetDriver->AddString(_T("<< Chọn nơi cần nâng PK >>"));
	pGetDriver->AddString(_T("Những nơi đã đi qua 1"));
	pGetDriver->AddString(_T("Những nơi đã đi qua 2"));
	pGetDriver->AddString(_T("Những nơi đã đi qua 3"));

	CComboBox *pGetQuest = (CComboBox*)GetDlgItem(IDC_QUESTKIND_COMBO);
	pGetQuest->AddString(_T("Tự động chọn"));
	pGetQuest->AddString(_T("Lệnh bài cấp 1"));
	pGetQuest->AddString(_T("Lệnh bài cấp 2"));
	pGetQuest->AddString(_T("Lệnh bài cấp 3"));

	TCHAR szPlayerName[40];
	CYeSouOptions *theYSOpt = (CYeSouOptions *)GetParent();
	CComboBox *pVictimList = (CComboBox*)GetDlgItem(IDC_VICTIM_COMBO);

	for (int i = 0; i < g_pCharList->GetItemCount(); i++)
	{
		GameWindow* pGameWin = (GameWindow*)g_pCharList->GetItemData(i);
		if (!pGameWin->m_hWnd) continue;
		if (pGameWin == g_pCurGameWin) continue;

		MultiByteToWideChar(CP_ACP, 0, pGameWin->m_szPlayerName, -1, szPlayerName, 32);
		pVictimList->AddString(szPlayerName);
	}

	UpdateData(0);
	return TRUE;
}

void CYSTongPK::OnButtonsClicked(UINT nID)
{
	if (g_pCurGameWin == NULL) return;

	switch (nID)
	{
	case IDC_READP_BUTTON:
		UdpdatePosition();
		break;
	case IDC_ADD_BUTTON:
		AddHardQuest();
		break;
	case IDC_DEL_BUTTON:
		DelHardQuest();
		break;
	}
}

void CYSTongPK::UdpdatePosition()
{
	m_nPointX = g_pCurGameWin->m_nOffX;
	m_nPointY = g_pCurGameWin->m_nOffY;
	SetDlgItemInt(IDC_POINTX_EDIT, m_nPointX >> 8);
	SetDlgItemInt(IDC_POINTY_EDIT, m_nPointY >> 9);
}

void CYSTongPK::AddHardQuest()
{
	int nItem = m_QuestList.GetItemCount();
	if (nItem >= MAX_HARD_QUEST)
	{
		MessageBox(_T("Số lượng đã đạt tối đa!!!"), _T("VLAuto"), MB_ICONEXCLAMATION);
		return;
	}
	CPKQuestFilter theDlgFilter;
	if (theDlgFilter.DoModal() == IDOK)
	{
		short nQuestKind = (short)theDlgFilter.m_nQuestKind;
		char nWsLevel = (char)theDlgFilter.m_nLevel;
		char nLetters = (char)theDlgFilter.m_nLetters;
		int nQuestCode = ((int)nQuestKind << 16) | ((int)nWsLevel << 8) | ((int)nLetters);
		// Check unique
		for (int i = 0; i < nItem; i++)
		{
			int nCurCode = (int)m_QuestList.GetItemData(i);
			if (nCurCode == nQuestCode)
			{
				MessageBox(_T("Đã có dòng tương tự!!!"), _T("VLAuto"), MB_ICONEXCLAMATION);
				return;
			}
		}
		// Insert Item
		TCHAR szValue[16];
		m_QuestList.InsertItem(nItem, TongPKKind[nQuestKind]);
		m_QuestList.SetItemData(nItem, nQuestCode);
		_stprintf(szValue, _T("%d"), nWsLevel);
		m_QuestList.SetItemText(nItem, 1, szValue);
		_stprintf(szValue, _T("%d"), nLetters);
		m_QuestList.SetItemText(nItem, 2, szValue);
	}
}

void CYSTongPK::DelHardQuest()
{
	int nCount = m_QuestList.GetItemCount();
	for (int i = 0; i < nCount; i++)
	{
		if (m_QuestList.GetItemState(i, LVIS_SELECTED) == LVIS_SELECTED)
		{
			m_QuestList.DeleteItem(i);
			break;
		}
	}
}

void CYSTongPK::UpdateQuestList(BOOL bSave)
{
	int nQuestCode;
	if (bSave)
	{
		memset(m_pTongPKOpt->m_HardQuests, 0, sizeof(m_pTongPKOpt->m_HardQuests));
		int nCount = m_QuestList.GetItemCount();
		for (int i = 0; i < nCount; i++)
		{
			nQuestCode = (int)m_QuestList.GetItemData(i);
			m_pTongPKOpt->m_HardQuests[i].nQuestKind = (short)(nQuestCode >> 16);
			m_pTongPKOpt->m_HardQuests[i].nWsLevel = (char)((nQuestCode >> 8) & 0xFF);
			m_pTongPKOpt->m_HardQuests[i].nLetters = (char)(nQuestCode & 0xFF);
		}
	}
	else
	{
		TCHAR szValue[16];
		m_QuestList.DeleteAllItems();
		int i = 0;
		while (m_pTongPKOpt->m_HardQuests[i].nWsLevel > 0)
		{
			short nQuestKind = m_pTongPKOpt->m_HardQuests[i].nQuestKind;
			char nWsLevel = m_pTongPKOpt->m_HardQuests[i].nWsLevel;
			char nLetters = m_pTongPKOpt->m_HardQuests[i].nLetters;
			nQuestCode = ((int)nQuestKind << 16) | ((int)nWsLevel << 8) | ((int)nLetters);
			m_QuestList.InsertItem(i, TongPKKind[nQuestKind]);
			m_QuestList.SetItemData(i, nQuestCode);
			_stprintf(szValue, _T("%d"), nWsLevel);
			m_QuestList.SetItemText(i, 1, szValue);
			_stprintf(szValue, _T("%d"), nLetters);
			m_QuestList.SetItemText(i, 2, szValue);
			// Now don't forget this.
			i++;
		}
	}
}
