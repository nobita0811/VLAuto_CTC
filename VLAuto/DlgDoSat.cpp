// DlgDoSat.cpp : Cai dat Do Sat / Cuu sat (mo tu tab Chien dau)
//

#include "stdafx.h"
#include "../GameDef.h"
#include "DWinProcess.h"
#include "VLAuto.h"
#include "GameCore.h"
#include "GameWindow.h"
#include "DlgMovePoints.h"
#include "DlgDoSat.h"

#define ENEMY_FILE_FILTER	_T("Enemy List File (*.enm)|*.enm|Text File (*.txt)|*.txt|")

// CDlgDoSat dialog

IMPLEMENT_DYNAMIC(CDlgDoSat, CDialog)

CDlgDoSat::CDlgDoSat(CWnd* pParent /*=NULL*/)
	: CDialog(CDlgDoSat::IDD, pParent)
{
}

CDlgDoSat::~CDlgDoSat()
{
}

void CDlgDoSat::DoDataExchange(CDataExchange* pDX)
{
	DDX_Control(pDX, IDC_ENEMY_LIST, m_EnemyList);
	CDialog::DoDataExchange(pDX);
	if (g_pCurGameWin == NULL)
		return;
	PARAM_ATTACK &pa = g_pCurGameWin->m_prAttack;
	if (pa.m_nPKMode < PKMODE_OFF || pa.m_nPKMode > PKMODE_REVENGE)
		pa.m_nPKMode = PKMODE_OFF;
	DDX_CBIndex(pDX, IDC_DOSAT_COMBO, pa.m_nPKMode);
	if (pa.m_nPKMode < PKMODE_OFF)
		pa.m_nPKMode = PKMODE_OFF;
	UpdateEnemyList(pDX->m_bSaveAndValidate);
	if (!pDX->m_bSaveAndValidate)
		UpdateMapName();
}

BEGIN_MESSAGE_MAP(CDlgDoSat, CDialog)
	ON_BN_CLICKED(IDC_GET_MAPID_BUTTON, &CDlgDoSat::OnBnClickedGetMapID)
	ON_CBN_SELCHANGE(IDC_DOSAT_COMBO, &CDlgDoSat::OnCbnSelchangeMode)
	ON_CBN_DROPDOWN(IDC_DOSAT_COMBO2, &CDlgDoSat::OnCbnDropdownNearBy)
	ON_BN_CLICKED(IDC_ADD_ENEMY_BUTTON1, &CDlgDoSat::OnBnClickedAddNearBy)
	ON_BN_CLICKED(IDC_ADD_ENEMY_BUTTON, &CDlgDoSat::OnBnClickedAddInput)
	ON_BN_CLICKED(IDC_DOSAT_LIST_REMOVE_BUTTON, &CDlgDoSat::OnBnClickedRemove)
	ON_BN_CLICKED(IDC_DOSAT_LIST_CLEAR_BUTTON2, &CDlgDoSat::OnBnClickedClear)
	ON_BN_CLICKED(IDC_DOSAT_LIST_SAVE_BUTTON, &CDlgDoSat::OnBnClickedSave)
	ON_BN_CLICKED(IDC_DOSAT_LIST_LOAD_BUTTON, &CDlgDoSat::OnBnClickedLoad)
END_MESSAGE_MAP()

BOOL CDlgDoSat::OnInitDialog()
{
	CComboBox *pMode = (CComboBox*)GetDlgItem(IDC_DOSAT_COMBO);
	pMode->AddString(_T("Không dùng"));					// PKMODE_OFF
	pMode->AddString(_T("Đồ sát tất cả"));				// PKMODE_ALL
	pMode->AddString(_T("Đồ sát theo danh sách"));		// PKMODE_LIST
	pMode->AddString(_T("Cừu sát theo danh sách"));		// PKMODE_REVENGE
	((CEdit*)GetDlgItem(IDC_INPUT_ENEMY_EDIT))->SetLimitText(31);

	CDialog::OnInitDialog();	// UpdateData(FALSE)
	UpdateNearByPlayers();
	return TRUE;
}

// Tab Chien dau doi nhan vat khi cua so dang mo
void CDlgDoSat::RefreshFromParams()
{
	if (GetSafeHwnd() == NULL) return;
	if (g_pCurGameWin == NULL) { EndDialog(IDCANCEL); return; }
	UpdateData(FALSE);
	UpdateNearByPlayers();
}

// Enter trong o nhap ten = bam "them"
void CDlgDoSat::OnOK()
{
	if (GetFocus() == GetDlgItem(IDC_INPUT_ENEMY_EDIT))
		OnBnClickedAddInput();
}

void CDlgDoSat::OnCancel()
{
	if (g_pCurGameWin != NULL)
	{
		ApplyParams();
		g_pCurGameWin->SaveProfile();
	}
	CDialog::OnCancel();
}

void CDlgDoSat::ApplyParams()
{
	if (g_pCurGameWin == NULL) return;
	UpdateData();
	g_pCurGameWin->SetGameParams(tab_attack);
}

void CDlgDoSat::UpdateMapName()
{
	int nMapID = g_pCurGameWin ? g_pCurGameWin->m_prAttack.m_nPKMapID : 0;
	SetDlgItemText(IDC_DOSAT_MAPNAME_EDIT, nMapID ? (LPCTSTR)CDlgMovePoints::GetMapName(nMapID) : _T("(chưa chọn)"));
}

// Lay map hien tai cua nhan vat lam map bat Do sat / Cuu sat
void CDlgDoSat::OnBnClickedGetMapID()
{
	if (g_pCurGameWin == NULL) return;
	int nMapID = g_pCurGameWin->m_nMapID;
	if (nMapID <= 0)
	{
		MessageBox(_T("Chưa lấy được bản đồ hiện tại của nhân vật!"), 0, MB_ICONEXCLAMATION);
		return;
	}
	UpdateData();
	g_pCurGameWin->m_prAttack.m_nPKMapID = nMapID;
	UpdateMapName();
	g_pCurGameWin->SetGameParams(tab_attack);
}

void CDlgDoSat::OnCbnSelchangeMode()
{
	ApplyParams();
}

void CDlgDoSat::OnCbnDropdownNearBy()
{
	UpdateNearByPlayers();
}

// Nguoi choi xung quanh (giong tab To doi)
void CDlgDoSat::UpdateNearByPlayers()
{
	CComboBox *pNearBy = (CComboBox*)GetDlgItem(IDC_DOSAT_COMBO2);
	CString strSel;
	pNearBy->GetWindowText(strSel);
	pNearBy->ResetContent();
	if (g_pCurGameWin == NULL) return;

	LPBYTE lpBaseAdd, lpCurAdd;
	DWinProcess winProcess(g_pCurGameWin->m_hWnd);
	if (!winProcess.ReadMemory((LPVOID)NPC_BASE_ADD, (LPVOID)&lpBaseAdd, 4))
		return;

	TCHAR szName[32];
	KNpc Npc;
	for (int i = 1; i < MAX_NPC; i++)
	{
		lpCurAdd = lpBaseAdd + i*NPC_DATA_SIZE;
		if (!winProcess.ReadMemory(lpCurAdd, (LPVOID)&Npc, sizeof(Npc))) continue;
		if (!Npc.m_Index || Npc.m_NpcKind != 1) continue;
		Npc.Name[31] = 0;
		if (!Npc.Name[0] || !strcmp(Npc.Name, g_pCurGameWin->m_szPlayerName)) continue;	// Chinh minh
		TCVN3ToUnicode(Npc.Name, szName, 31);
		if (pNearBy->FindStringExact(-1, szName) == CB_ERR)
			pNearBy->AddString(szName);
	}
	int nSel = strSel.IsEmpty() ? CB_ERR : pNearBy->FindStringExact(-1, strSel);
	pNearBy->SetCurSel(nSel != CB_ERR ? nSel : (pNearBy->GetCount() ? 0 : -1));
}

void CDlgDoSat::UpdateEnemyList(BOOL bSave)
{
	PARAM_ATTACK &pa = g_pCurGameWin->m_prAttack;
	TCHAR szName[64];
	if (bSave)
	{
		memset(pa.m_szEnemies, 0, sizeof(pa.m_szEnemies));
		int nCount = min(m_EnemyList.GetCount(), MAX_ENEMY);
		for (int i = 0, j = 0; i < nCount; i++)
		{
			if (m_EnemyList.GetTextLen(i) >= 32) continue;
			m_EnemyList.GetText(i, szName);
			UnicodeToTCVN3(szName, pa.m_szEnemies[j], 31);
			if (pa.m_szEnemies[j][0]) j++;
		}
	}
	else
	{
		m_EnemyList.ResetContent();
		for (int i = 0; i < MAX_ENEMY && pa.m_szEnemies[i][0]; i++)
		{
			pa.m_szEnemies[i][31] = 0;
			TCVN3ToUnicode(pa.m_szEnemies[i], szName, 31);
			m_EnemyList.AddString(szName);
		}
	}
}

// TRUE = da them (hoac da co san)
BOOL CDlgDoSat::AddEnemy(LPCTSTR szName)
{
	CString strName(szName);
	strName.Trim();
	if (strName.IsEmpty()) return FALSE;
	if (strName.GetLength() > 31)
	{
		MessageBox(_T("Tên quá dài (tối đa 31 ký tự)!"), 0, MB_ICONEXCLAMATION);
		return FALSE;
	}
	if (m_EnemyList.FindStringExact(-1, strName) != LB_ERR)
		return TRUE;
	if (m_EnemyList.GetCount() >= MAX_ENEMY)
	{
		CString strMsg;
		strMsg.Format(_T("Danh sách kẻ thù đã đủ %d người!"), MAX_ENEMY);
		MessageBox(strMsg, 0, MB_ICONEXCLAMATION);
		return FALSE;
	}
	m_EnemyList.AddString(strName);
	ApplyParams();
	return TRUE;
}

void CDlgDoSat::OnBnClickedAddNearBy()
{
	CString strName;
	GetDlgItemText(IDC_DOSAT_COMBO2, strName);
	AddEnemy(strName);
}

void CDlgDoSat::OnBnClickedAddInput()
{
	CString strName;
	GetDlgItemText(IDC_INPUT_ENEMY_EDIT, strName);
	if (AddEnemy(strName))
		SetDlgItemText(IDC_INPUT_ENEMY_EDIT, _T(""));
	GetDlgItem(IDC_INPUT_ENEMY_EDIT)->SetFocus();
}

void CDlgDoSat::OnBnClickedRemove()
{
	int nSel = m_EnemyList.GetCurSel();
	if (nSel == LB_ERR)
	{
		MessageBox(_T("Chọn một kẻ thù trong danh sách để xóa!"), 0, MB_ICONINFORMATION);
		return;
	}
	m_EnemyList.DeleteString(nSel);
	if (m_EnemyList.GetCount())
		m_EnemyList.SetCurSel(min(nSel, m_EnemyList.GetCount() - 1));
	ApplyParams();
}

void CDlgDoSat::OnBnClickedClear()
{
	if (!m_EnemyList.GetCount()) return;
	if (MessageBox(_T("Xóa hết danh sách kẻ thù?"), 0, MB_YESNO | MB_ICONQUESTION) != IDYES)
		return;
	m_EnemyList.ResetContent();
	ApplyParams();
}

// File danh sach: van ban Unicode (UTF-16 co BOM), moi dong 1 ten - sua tay bang Notepad duoc
void CDlgDoSat::OnBnClickedSave()
{
	CFileDialog fDlg(FALSE, _T(".enm"), 0, OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT, ENEMY_FILE_FILTER, this);
	if (fDlg.DoModal() == IDCANCEL)
		return;
	CFile file;
	if (!file.Open(fDlg.GetPathName(), CFile::modeCreate | CFile::modeWrite))
	{
		MessageBox(_T("Lỗi: không thể ghi tập tin!"), 0, MB_ICONEXCLAMATION);
		return;
	}
	WORD wBOM = 0xFEFF;
	file.Write(&wBOM, sizeof(wBOM));
	CString strName;
	for (int i = 0; i < m_EnemyList.GetCount(); i++)
	{
		m_EnemyList.GetText(i, strName);
		strName += _T("\r\n");
		file.Write((LPCTSTR)strName, strName.GetLength() * sizeof(TCHAR));
	}
	file.Close();
}

// Doc file UTF-16 (co BOM) / UTF-8 / ANSI, moi dong 1 ten
void CDlgDoSat::OnBnClickedLoad()
{
	CFileDialog fDlg(TRUE, _T(".enm"), 0, OFN_HIDEREADONLY | OFN_FILEMUSTEXIST, ENEMY_FILE_FILTER, this);
	if (fDlg.DoModal() == IDCANCEL)
		return;
	CFile file;
	if (!file.Open(fDlg.GetPathName(), CFile::modeRead | CFile::shareDenyWrite))
	{
		MessageBox(_T("Lỗi: không thể đọc tập tin!"), 0, MB_ICONEXCLAMATION);
		return;
	}
	UINT nLen = (UINT)min(file.GetLength(), (ULONGLONG)(64 * 1024));
	CByteArray buf;
	buf.SetSize(nLen + 2);
	nLen = file.Read(buf.GetData(), nLen);
	file.Close();
	buf[nLen] = 0;
	buf[nLen + 1] = 0;

	CString strText;
	LPBYTE p = buf.GetData();
	if (nLen >= 2 && p[0] == 0xFF && p[1] == 0xFE)
		strText = (LPCWSTR)(p + 2);
	else
	{
		LPCSTR s = (LPCSTR)p;
		if (nLen >= 3 && p[0] == 0xEF && p[1] == 0xBB && p[2] == 0xBF) s += 3;
		UINT cp = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, s, -1, NULL, 0) > 0 ? CP_UTF8 : CP_ACP;
		int nW = MultiByteToWideChar(cp, 0, s, -1, NULL, 0);
		if (nW > 0)
		{
			MultiByteToWideChar(cp, 0, s, -1, strText.GetBuffer(nW), nW);
			strText.ReleaseBuffer();
		}
	}

	m_EnemyList.ResetContent();
	int nPos = 0;
	while (nPos >= 0)
	{
		CString strLine = strText.Tokenize(_T("\r\n"), nPos);
		if (nPos < 0) break;
		strLine.Trim();
		if (!strLine.IsEmpty() && strLine.GetLength() <= 31 && m_EnemyList.GetCount() < MAX_ENEMY
			&& m_EnemyList.FindStringExact(-1, strLine) == LB_ERR)
			m_EnemyList.AddString(strLine);
	}
	ApplyParams();
}
