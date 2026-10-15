// DlgParty.cpp : implementation file
//

#include "stdafx.h"
#include "DWinProcess.h"
#include "VLAuto.h"
#include "GameCore.h"
#include "GameWindow.h"
#include "DlgParty.h"

// CDlgParty dialog
IMPLEMENT_DYNAMIC(CDlgParty, CDialog)

CDlgParty::CDlgParty(CWnd* pParent /*=NULL*/)
	: CDialog(CDlgParty::IDD, pParent)
{
}

CDlgParty::~CDlgParty()
{
}

void CDlgParty::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_MEMBER_LIST, m_MemberList);

	if (g_pCurGameWin == NULL) return;

	UpdateMembersList(pDX->m_bSaveAndValidate);

	//Checkboxes
	DDX_Check(pDX, IDC_AUTOPT_CHECK, g_pCurGameWin->m_prParty.m_bAutoPT);
	DDX_Check(pDX, IDC_AUTOLEAVE_CHECK, g_pCurGameWin->m_prParty.m_bAutoLeave);
	DDX_Check(pDX, IDC_AUTOKICK_CHECK, g_pCurGameWin->m_prParty.m_bAutoKick);

	//Comboboxes
	DDX_CBIndex(pDX, IDC_PTALL_COMBO, g_pCurGameWin->m_prParty.m_nPTAll);
	DDX_CBIndex(pDX, IDC_ACCEPTALL_COMBO, g_pCurGameWin->m_prParty.m_nAcceptAll);

	//Textboxes
	DDX_Text(pDX, IDC_LEAVEMEMNUM_EDIT, g_pCurGameWin->m_prParty.m_nAutoLeaveMemNum);
	DDX_Text(pDX, IDC_PTLEAVETIME_EDIT, g_pCurGameWin->m_prParty.m_nAutoLeaveTime);
	DDX_Text(pDX, IDC_PTKICKTIME_EDIT, g_pCurGameWin->m_prParty.m_nAutoKickTime);

	//UI Refresh
	BOOL bChecked = ((CButton*)GetDlgItem(IDC_AUTOPT_CHECK))->GetCheck();
	GetDlgItem(IDC_NEARBYPLAYER_COMBO)->EnableWindow(!bChecked);
	GetDlgItem(IDC_ADD_BUTTON)->EnableWindow(!bChecked);
	GetDlgItem(IDC_DEL_BUTTON)->EnableWindow(!bChecked);
	GetDlgItem(IDC_UP_BUTTON)->EnableWindow(!bChecked);
	GetDlgItem(IDC_DOWN_BUTTON)->EnableWindow(!bChecked);
	GetDlgItem(IDC_DELALL_BUTTON)->EnableWindow(!bChecked);
	GetDlgItem(IDC_LOAD_BUTTON)->EnableWindow(!bChecked);
	bChecked = ((CButton*)GetDlgItem(IDC_AUTOLEAVE_CHECK))->GetCheck();
	GetDlgItem(IDC_LEAVEMEMNUM_EDIT)->EnableWindow(!bChecked);
	GetDlgItem(IDC_PTLEAVETIME_EDIT)->EnableWindow(!bChecked);
	bChecked = ((CButton*)GetDlgItem(IDC_AUTOKICK_CHECK))->GetCheck();
	GetDlgItem(IDC_PTKICKTIME_EDIT)->EnableWindow(!bChecked);

	UpdateNearByPlayers();
}

BEGIN_MESSAGE_MAP(CDlgParty, CDialog)
	ON_CONTROL_RANGE(BN_CLICKED, 1000, 2000, &CDlgParty::OnButtonsClicked)
	ON_CBN_SELCHANGE(IDC_PTALL_COMBO, &CDlgParty::OnCbnSelchangePtallCombo)
	ON_CBN_SELCHANGE(IDC_ACCEPTALL_COMBO, &CDlgParty::OnCbnSelchangeAcceptallCombo)
END_MESSAGE_MAP()

// CDlgParty message handlers

BOOL CDlgParty::OnInitDialog()
{
	CDialog::OnInitDialog();

	CComboBox *pCombo = (CComboBox *)GetDlgItem(IDC_PTALL_COMBO);
	pCombo->AddString(_T("Không mời tất cả"));
	pCombo->AddString(_T("Luôn mời tất cả"));
	pCombo->AddString(_T("Chỉ mời khi làm DT"));

	pCombo = (CComboBox *)GetDlgItem(IDC_ACCEPTALL_COMBO);
	pCombo->AddString(_T("Kô nhận t.cả"));
	pCombo->AddString(_T("Luôn nhận t.cả"));
	pCombo->AddString(_T("Chỉ nhận khi DT"));

	if (m_ToolTip.Create(this))
	{
		m_ToolTip.AddTool(GetDlgItem(IDC_AUTOPT_CHECK), _T("Bật/Tắt chế độ tự lập Tổ đội (PT)"));
		m_ToolTip.AddTool(GetDlgItem(IDC_NEARBYPLAYER_COMBO), _T("Chọn nhân vật vào danh sách PT"));
		m_ToolTip.AddTool(GetDlgItem(IDC_MEMBER_LIST), _T("Danh sách PT theo thứ tự - chủ PT ở trên"));
		m_ToolTip.AddTool(GetDlgItem(IDC_PTALL_COMBO), _T("Mời PT tất cả mọi người"));
		m_ToolTip.AddTool(GetDlgItem(IDC_ACCEPTALL_COMBO), _T("Chấp nhận mọi lời mời PT"));
		m_ToolTip.AddTool(GetDlgItem(IDC_AUTOLEAVE_CHECK), _T("Tự động rời PT khi số thành viên quá ít"));
		m_ToolTip.AddTool(GetDlgItem(IDC_LEAVEMEMNUM_EDIT), _T("Số lượng thành viên kiểm tra tự rời PT"));
		m_ToolTip.AddTool(GetDlgItem(IDC_PTLEAVETIME_EDIT), _T("Thời gian kiểm tra tự rời PT"));
		m_ToolTip.AddTool(GetDlgItem(IDC_AUTOKICK_CHECK), _T("(Chủ PT) Tự kick thành viên vắng mặt quá lâu"));
		m_ToolTip.AddTool(GetDlgItem(IDC_PTKICKTIME_EDIT), _T("Thời gian kiểm tra tự kick thành viên"));
	}

	return FALSE;
}

BOOL CDlgParty::PreTranslateMessage(MSG* pMsg)
{
	m_ToolTip.RelayEvent(pMsg);
	return CDialog::PreTranslateMessage(pMsg);
}

void CDlgParty::OnButtonsClicked(UINT nID)
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
		break;
	case IDC_LOAD_BUTTON:
		OnBnClickedLoadButton();
		break;
	}
	UpdateData();
	g_pCurGameWin->SetGameParams(tab_party);
}

void CDlgParty::UpdateNearByPlayers()
{
	TCHAR szName[32]=_T("");
	if (((CButton*)GetDlgItem(IDC_AUTOPT_CHECK))->GetCheck())
		return;

	LPBYTE lpBaseAdd, lpCurAdd;
	DWinProcess winProcess(g_pCurGameWin->m_hWnd);
	if (!winProcess.ReadMemory((LPVOID)NPC_BASE_ADD, (LPVOID)&lpBaseAdd, 4))
		return;

	CComboBox *pNearByPlayer = (CComboBox*)GetDlgItem(IDC_NEARBYPLAYER_COMBO);
	pNearByPlayer->ResetContent();

	KNpc Npc;
	for (int i = 1;  i < MAX_NPC; i++)
	{
		lpCurAdd = lpBaseAdd + i*NPC_DATA_SIZE;
		winProcess.ReadMemory(lpCurAdd, (LPVOID)&Npc, sizeof(Npc));
		if (!Npc.m_Index || Npc.m_NpcKind != 1) continue;
		MultiByteToWideChar(CP_ACP, 0, Npc.Name, -1, szName, 32);
		pNearByPlayer->AddString(szName);
	}
}

void CDlgParty::UpdateMembersList(BOOL bSave)
{
	TCHAR szName[32];
	if (bSave)
	{
		memset(g_pCurGameWin->m_prParty.m_szMemberName, 0, sizeof(g_pCurGameWin->m_prParty.m_szMemberName));
		int nCount = m_MemberList.GetCount();
		for (int i = 0; i < nCount; i++)
		{
			m_MemberList.GetText(i, szName);
			WideCharToMultiByte(CP_ACP, 0, szName, -1, g_pCurGameWin->m_prParty.m_szMemberName[i], 32, 0, 0);
		}
	}
	else
	{
		m_MemberList.ResetContent();
		for (int i=0; g_pCurGameWin->m_prParty.m_szMemberName[i][0]; i++)
		{
			MultiByteToWideChar(CP_ACP, 0, g_pCurGameWin->m_prParty.m_szMemberName[i], -1, szName, 32);
			m_MemberList.AddString(szName);
		}
	}
}

void CDlgParty::OnBnClickedAddButton()
{
	TCHAR szName[32], szCmpName[32];
	int nCount, nIndex = LB_ERR;

	GetDlgItemText(IDC_NEARBYPLAYER_COMBO, szName, 32);
	if (!szName[0]) return;
	nCount = m_MemberList.GetCount();
	if (nCount >= MAX_MEMBERS) return;

	//Case-Sensitive Search
	for (int i = 0; i <  nCount; i++)
	{
		m_MemberList.GetText(i, szCmpName);
		if (!_tcscmp(szName, szCmpName))
		{
			m_MemberList.SetCurSel(i);
			nIndex = i;
			break;
		}
	}

	if (nIndex == LB_ERR)
	{
		nIndex = m_MemberList.AddString(szName);
		m_MemberList.SetCurSel(nIndex);
	}
}

void CDlgParty::OnBnClickedDelButton()
{
	m_MemberList.DeleteString(m_MemberList.GetCurSel());
}

void CDlgParty::OnBnClickedUpButton()
{
	int nIndex = m_MemberList.GetCurSel();
	CString szTemp;
	if (nIndex>0)
	{
		m_MemberList.GetText(nIndex, szTemp);
		m_MemberList.DeleteString(nIndex);
		m_MemberList.InsertString(nIndex-1, szTemp);
		m_MemberList.SetCurSel(nIndex-1);
	}
}

void CDlgParty::OnBnClickedDownButton()
{
	int nIndex = m_MemberList.GetCurSel();
	int nCount = m_MemberList.GetCount();
	CString szTemp;
	if (nIndex<nCount-1 && nIndex>-1)
	{
		m_MemberList.GetText(nIndex, szTemp);
		m_MemberList.DeleteString(nIndex);
		m_MemberList.InsertString(nIndex+1, szTemp);
		m_MemberList.SetCurSel(nIndex+1);
	}
}

void CDlgParty::OnBnClickedDelAllButton()
{
	m_MemberList.ResetContent();
}

void CDlgParty::OnBnClickedSaveButton()
{
	CFileDialog fDlg(FALSE, _T(".ptm"), 0,
		OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT, _T("Party Member File (*.ptm)|*.ptm|"), this);
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
	mvFile.Write(g_pCurGameWin->m_prParty.m_szMemberName, sizeof(g_pCurGameWin->m_prParty.m_szMemberName));
	mvFile.Close();
}

void CDlgParty::OnBnClickedLoadButton()
{
	CFileDialog fDlg(TRUE, _T(".ptm"), 0,
		OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT, _T("Party Member File (*.ptm)|*.ptm|"), this);
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
	memset(&g_pCurGameWin->m_prParty.m_szMemberName, 0, sizeof(g_pCurGameWin->m_prParty.m_szMemberName));
	mvFile.Read(&g_pCurGameWin->m_prParty.m_szMemberName, sizeof(g_pCurGameWin->m_prParty.m_szMemberName));
	mvFile.Close();
	// Update Data
	UpdateData(0);
	g_pCurGameWin->SetGameParams(tab_party);
}

void CDlgParty::OnCbnSelchangePtallCombo()
{
	// Update Data
	UpdateData();
	g_pCurGameWin->SetGameParams(tab_party);
}

void CDlgParty::OnCbnSelchangeAcceptallCombo()
{
	// Update Data
	UpdateData();
	g_pCurGameWin->SetGameParams(tab_party);
}
