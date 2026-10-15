// MyControls.cpp : implementation file
//

#include "stdafx.h"
#include "VLAuto.h"
#include "DlgBasic.h"
#include "DlgRecover.h"
#include "DlgAttack.h"
#include "DlgCollect.h"
#include "DlgMove.h"
#include "DlgParty.h"
#include "DlgMap.h"
#include "DlgYeSou.h"
#include "DlgAbout.h"
#include "MyTabCtrl.h"

// CMyTabCtrl

IMPLEMENT_DYNAMIC(CMyTabCtrl, CTabCtrl)

CMyTabCtrl::CMyTabCtrl()
{
	::ZeroMemory(&m_tabPages, sizeof(m_tabPages));

	m_tabPages[tab_basic] = new CDlgBasic;
	m_tabPages[tab_recover] = new CDlgRecover;
	m_tabPages[tab_attack] = new CDlgAttack;
	m_tabPages[tab_collect] = new CDlgCollect;
	m_tabPages[tab_move] = new CDlgMove;
	m_tabPages[tab_party] = new CDlgParty;
	m_tabPages[tab_map] = new CDlgMap;
	m_tabPages[tab_yesou] = new CDlgYeSou;
	m_tabPages[tab_about] = new CDlgAbout;
}

CMyTabCtrl::~CMyTabCtrl()
{
	for(int i=0; i < tab_num; i++)
		delete m_tabPages[i];
}

void CMyTabCtrl::Init()
{
	CImageList *pilTab = new CImageList();
	pilTab->Create(16, 16, ILC_COLOR24 | ILC_MASK, tab_num, tab_num);
	pilTab->SetBkColor(CLR_NONE);
	CBitmap bm;
	bm.LoadBitmap(IDB_TAB_BITMAP);
	pilTab->Add(&bm, RGB(127, 127, 127));
	bm.DeleteObject();
	SetImageList(pilTab);

	InsertItem(tab_basic, _T("Cơ bản"), 0);
	m_tabPages[tab_basic]->Create(IDD_BASIC_DIALOG, this);
	InsertItem(tab_recover, _T("Phục hồi"), 1);
	m_tabPages[tab_recover]->Create(IDD_RECOVER_DIALOG, this);
	InsertItem(tab_attack, _T("Chiến đấu"), 2);
	m_tabPages[tab_attack]->Create(IDD_ATTACK_DIALOG, this);
	InsertItem(tab_collect, _T("Nhặt đồ"), 3);
	m_tabPages[tab_collect]->Create(IDD_COLLECT_DIALOG, this);
	InsertItem(tab_move, _T("Di chuyển"), 4);
	m_tabPages[tab_move]->Create(IDD_MOVE_DIALOG, this);
	InsertItem(tab_party, _T("Tổ đội"), 5);
	m_tabPages[tab_party]->Create(IDD_PARTY_DIALOG, this);
	InsertItem(tab_map, _T("Bản đồ"), 6);
	m_tabPages[tab_map]->Create(IDD_MAP_DIALOG, this);
	InsertItem(tab_yesou, _T("Nhiệm vụ"), 7);
	m_tabPages[tab_yesou]->Create(IDD_YESOU_DIALOG, this);
	InsertItem(tab_about, _T("Trợ giúp"), 8);
	m_tabPages[tab_about]->Create(IDD_ABOUT_DIALOG, this);

	m_tabCurrent = tab_basic;
	SetRectangle();
}

void CMyTabCtrl::DestroyItems()
{
	CImageList *pilTab = GetImageList();
	if (pilTab)
	{
		pilTab->DeleteImageList();
		delete pilTab;
	}
	DeleteAllItems();
}

void CMyTabCtrl::UpdateAllTabs(int nActiveChar)
{
	((CDlgBasic*)m_tabPages[tab_basic])->m_nActiveChar = nActiveChar;
	((CDlgRecover*)m_tabPages[tab_recover])->m_nActiveChar = nActiveChar;
	((CDlgAttack*)m_tabPages[tab_attack])->m_nActiveChar = nActiveChar;
	((CDlgCollect*)m_tabPages[tab_collect])->m_nActiveChar = nActiveChar;
	((CDlgMove*)m_tabPages[tab_move])->m_nActiveChar = nActiveChar;
	((CDlgParty*)m_tabPages[tab_party])->m_nActiveChar = nActiveChar;
	((CDlgMap*)m_tabPages[tab_map])->m_nActiveChar = nActiveChar;
	((CDlgYeSou*)m_tabPages[tab_yesou])->m_nActiveChar = nActiveChar;
	((CDlgAbout*)m_tabPages[tab_about])->m_nActiveChar = nActiveChar;
	m_tabPages[m_tabCurrent]->UpdateData(0);
}

void CMyTabCtrl::SetGameParams(int nClickedChar)
{
	((CDlgBasic*)m_tabPages[tab_basic])->SetGameParams(nClickedChar);
	((CDlgRecover*)m_tabPages[tab_recover])->SetGameParams(nClickedChar);
	((CDlgAttack*)m_tabPages[tab_attack])->SetGameParams(nClickedChar);
	((CDlgCollect*)m_tabPages[tab_collect])->SetGameParams(nClickedChar);
	((CDlgMove*)m_tabPages[tab_move])->SetGameParams(nClickedChar);
	((CDlgParty*)m_tabPages[tab_party])->SetGameParams(nClickedChar);
	((CDlgMap*)m_tabPages[tab_map])->SetGameParams(nClickedChar);
	((CDlgYeSou*)m_tabPages[tab_yesou])->SetGameParams(nClickedChar);
}

void CMyTabCtrl::InitAllTabs(int nIndex)
{
	((CDlgBasic*)m_tabPages[tab_basic])->Init(nIndex);
	((CDlgRecover*)m_tabPages[tab_recover])->Init(nIndex);
	((CDlgAttack*)m_tabPages[tab_attack])->Init(nIndex);
	((CDlgCollect*)m_tabPages[tab_collect])->Init(nIndex);
	((CDlgMove*)m_tabPages[tab_move])->Init(nIndex);
	((CDlgParty*)m_tabPages[tab_party])->Init(nIndex);
	((CDlgMap*)m_tabPages[tab_map])->Init(nIndex);
	((CDlgYeSou*)m_tabPages[tab_yesou])->Init(nIndex);
	((CDlgAbout*)m_tabPages[tab_about])->Init(nIndex);
}

void CMyTabCtrl::LoadProfile(int nIndex, const char* szAccountID)
{
	TCHAR szAccID[32] = _T("");
	MultiByteToWideChar(CP_ACP, 0, szAccountID, -1, szAccID, 32);

	TCHAR szFileName[MAX_PATH];
	_stprintf(szFileName, _T("%s\\UserConfig\\%s.cfg"), g_szAppFolder, szAccID);
	LoadConfigFile(nIndex, szFileName, szAccountID);
}

BOOL CMyTabCtrl::LoadConfigFile(int nIndex, LPCTSTR szFileName, const char* szAccountID)
{
	CFile cfgFile;
	if (!cfgFile.Open(szFileName, CFile::modeRead))
		return 0;

	//Identification
	char szVersion[12] = "";
	char szAccID[32] = "";
	cfgFile.Read(szVersion, sizeof(szVersion));
	if (strcmp(szVersion, _SZCURVERSION) &&
		strcmp(szVersion, _SZOLDVERSION))
	{
		cfgFile.Close();
		return 0;
	}

	cfgFile.Read(szAccID, sizeof(szAccID));
	UINT cbSize, UIntSize;
	UIntSize = sizeof(cbSize);

	//CDlgBasic
	CDlgBasic::PARAM_BASIC paramBasic;
	cfgFile.Read(&cbSize, UIntSize);
	if ((cbSize > UIntSize) && cfgFile.Read((LPBYTE)&paramBasic+UIntSize, cbSize-UIntSize))
		memcpy(&((CDlgBasic*)m_tabPages[tab_basic])->m_Basic[nIndex], &paramBasic, cbSize);

	//CDlgRecover
	CDlgRecover::PARAM_RECOVER paramRecover;
	cfgFile.Read(&cbSize, UIntSize);
	if ((cbSize > UIntSize) && cfgFile.Read((LPBYTE)&paramRecover+UIntSize, cbSize-UIntSize))
		memcpy(&((CDlgRecover*)m_tabPages[tab_recover])->m_Recover[nIndex], &paramRecover, cbSize);

	//CDlgAttack
	CDlgAttack::PARAM_ATTACK paramAttack;
	cfgFile.Read(&cbSize, UIntSize);
	if ((cbSize > UIntSize) && cfgFile.Read((LPBYTE)&paramAttack+UIntSize, cbSize-UIntSize))
	{
		if (strcmp(szVersion, _SZOLDVERSION))
			memcpy(&((CDlgAttack*)m_tabPages[tab_attack])->m_Attack[nIndex], &paramAttack, cbSize);
	}

	//CDlgCollect
	CDlgCollect::PARAM_COLLECT paramCollect;
	cfgFile.Read(&cbSize, UIntSize);
	if ((cbSize > UIntSize) && cfgFile.Read((LPBYTE)&paramCollect+UIntSize, cbSize-UIntSize))
		memcpy(&((CDlgCollect*)m_tabPages[tab_collect])->m_Collect[nIndex], &paramCollect, cbSize);

	//CDlgMove
	CDlgMove::PARAM_MOVE paramMove;
	cfgFile.Read(&cbSize, UIntSize);
	if ((cbSize > UIntSize) && cfgFile.Read((LPBYTE)&paramMove+UIntSize, cbSize-UIntSize))
		memcpy(&((CDlgMove*)m_tabPages[tab_move])->m_Move[nIndex], &paramMove, cbSize);

	//CDlgParty
	CDlgParty::PARAM_PARTY paramParty;
	cfgFile.Read(&cbSize, UIntSize);
	if ((cbSize > UIntSize) && cfgFile.Read((LPBYTE)&paramParty+UIntSize, cbSize-UIntSize))
		memcpy(&((CDlgParty*)m_tabPages[tab_party])->m_Party[nIndex], &paramParty, cbSize);

	//CDlgMap
	CDlgMap::PARAM_MAP paramMap;
	cfgFile.Read(&cbSize, UIntSize);
	if ((cbSize > UIntSize) && cfgFile.Read((LPBYTE)&paramMap+UIntSize, cbSize-UIntSize))
		memcpy(&((CDlgMap*)m_tabPages[tab_map])->m_Map[nIndex], &paramMap, cbSize);

	//CDlgYeSou
	CDlgYeSou::PARAM_YESOU paramYeSou;
	cfgFile.Read(&cbSize, UIntSize);
	if ((cbSize > UIntSize) && cfgFile.Read((LPBYTE)&paramYeSou+UIntSize, cbSize-UIntSize))
		memcpy(&((CDlgYeSou*)m_tabPages[tab_yesou])->m_YeSou[nIndex], &paramYeSou, cbSize);

	if (strcmp(szAccountID, szAccID))
		memset(((CDlgAttack*)m_tabPages[tab_attack])->m_Attack[nIndex].m_SettingSkills, 0, sizeof(((CDlgAttack*)m_tabPages[tab_attack])->m_Attack[nIndex].m_SettingSkills));
	((CDlgMap *)m_tabPages[tab_map])->UpdateTrainMap(0);

	m_tabPages[m_tabCurrent]->UpdateData(0);
	cfgFile.Close();

	return TRUE;
}

void CMyTabCtrl::SaveProfile(int nIndex, const char* szAccountID)
{
	if (strlen(szAccountID) < 4) return;
	TCHAR szAccID[32] = _T("");
	MultiByteToWideChar(CP_ACP, 0, szAccountID, -1, szAccID, 32);

	TCHAR szFileName[MAX_PATH];
	_stprintf(szFileName, _T("%s\\UserConfig\\%s.cfg"), g_szAppFolder, szAccID);
	SaveConfigFile(nIndex, szFileName, szAccountID);
}

BOOL CMyTabCtrl::SaveConfigFile(int nIndex, LPCTSTR szFileName, const char* szAccountID)
{
	CFile cfgFile;
	if (!cfgFile.Open(szFileName, CFile::modeCreate|CFile::modeWrite))
		return 0;

	//Identification
	char szVersion[12] = "";
	strcpy(szVersion, _SZCURVERSION);
	char szAccID[32] = "";
	strcpy(szAccID, szAccountID);
	cfgFile.Write(szVersion, sizeof(szVersion));
	cfgFile.Write(szAccID, sizeof(szAccID));

	//CDlgBasic
	CDlgBasic::PARAM_BASIC paramBasic;
	paramBasic = ((CDlgBasic*)m_tabPages[tab_basic])->m_Basic[nIndex];
	paramBasic.m_cbSize = sizeof(paramBasic);
	cfgFile.Write(&paramBasic, paramBasic.m_cbSize);

	//CDlgRecover
	CDlgRecover::PARAM_RECOVER paramRecover;
	paramRecover = ((CDlgRecover*)m_tabPages[tab_recover])->m_Recover[nIndex];
	paramRecover.m_cbSize = sizeof(paramRecover);
	cfgFile.Write(&paramRecover, paramRecover.m_cbSize);

	//CDlgAttack
	CDlgAttack::PARAM_ATTACK paramAttack;
	paramAttack = ((CDlgAttack*)m_tabPages[tab_attack])->m_Attack[nIndex];
	paramAttack.m_cbSize = sizeof(paramAttack);
	cfgFile.Write(&paramAttack, paramAttack.m_cbSize);

	//CDlgCollect
	CDlgCollect::PARAM_COLLECT paramCollect;
	paramCollect = ((CDlgCollect*)m_tabPages[tab_collect])->m_Collect[nIndex];
	paramCollect.m_cbSize = sizeof(paramCollect);
	cfgFile.Write(&paramCollect, paramCollect.m_cbSize);

	//CDlgMove
	CDlgMove::PARAM_MOVE paramMove;
	paramMove = ((CDlgMove*)m_tabPages[tab_move])->m_Move[nIndex];
	paramMove.m_cbSize = sizeof(paramMove);
	cfgFile.Write(&paramMove, paramMove.m_cbSize);

	//CDlgParty
	CDlgParty::PARAM_PARTY paramParty;
	paramParty = ((CDlgParty*)m_tabPages[tab_party])->m_Party[nIndex];
	paramParty.m_cbSize = sizeof(paramParty);
	cfgFile.Write(&paramParty, paramParty.m_cbSize);

	//CDlgMap
	CDlgMap::PARAM_MAP paramMap;
	paramMap = ((CDlgMap*)m_tabPages[tab_map])->m_Map[nIndex];
	paramMap.m_cbSize = sizeof(paramMap);
	cfgFile.Write(&paramMap, paramMap.m_cbSize);

	//CDlgYeSou
	CDlgYeSou::PARAM_YESOU paramYeSou;
	paramYeSou = ((CDlgYeSou*)m_tabPages[tab_yesou])->m_YeSou[nIndex];
	paramYeSou.m_cbSize = sizeof(paramYeSou);
	cfgFile.Write(&paramYeSou, paramYeSou.m_cbSize);

	cfgFile.Close();
	
	return TRUE;
}

void CMyTabCtrl::SetRectangle()
{
	CRect tabRect, itemRect;
	int nX, nY, nXc, nYc;

	GetClientRect(&tabRect);
	GetItemRect(0, &itemRect);

	nX = itemRect.left;
	nY = itemRect.bottom + 1;
	nXc = tabRect.right - itemRect.left - 1;
	nYc = tabRect.bottom - nY - 1;

	m_tabPages[tab_basic]->SetWindowPos(&wndTop, nX, nY, nXc, nYc, SWP_SHOWWINDOW);
	for(int i=1; i < tab_num; i++)
		m_tabPages[i]->SetWindowPos(&wndTop, nX, nY, nXc, nYc, SWP_HIDEWINDOW);
}

BEGIN_MESSAGE_MAP(CMyTabCtrl, CTabCtrl)
	//{{AFX_MSG_MAP(CMyTabCtrl)
	ON_WM_LBUTTONDOWN()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

void CMyTabCtrl::OnLButtonDown(UINT nFlags, CPoint point) 
{
	CTabCtrl::OnLButtonDown(nFlags, point);

	if(m_tabCurrent != GetCurFocus()){
		m_tabPages[m_tabCurrent]->ShowWindow(SW_HIDE);
		m_tabCurrent=GetCurFocus();
		m_tabPages[m_tabCurrent]->ShowWindow(SW_SHOW);
		m_tabPages[m_tabCurrent]->SetFocus();
		m_tabPages[m_tabCurrent]->UpdateData(0);
	}
}
