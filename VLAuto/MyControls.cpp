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
#include "DlgAbout.h"
#include "MyControls.h"

// CMyTabCtrl

extern TCHAR szCurFolder[MAX_PATH];

IMPLEMENT_DYNAMIC(CMyTabCtrl, CTabCtrl)

CMyTabCtrl::CMyTabCtrl()
{
	::ZeroMemory(&m_tabPages, sizeof(m_tabPages));

	m_tabPages[0] = new CDlgBasic;
	m_tabPages[1] = new CDlgRecover;
	m_tabPages[2] = new CDlgAttack;
	m_tabPages[3] = new CDlgCollect;
	m_tabPages[4] = new CDlgMove;
	m_tabPages[5] = new CDlgParty;
	m_tabPages[6] = new CDlgMap;
	m_tabPages[7] = new CDlgAbout;
}

CMyTabCtrl::~CMyTabCtrl()
{
	for(int nCount=0; nCount < NUM_OF_TABS; nCount++)
	{
		delete m_tabPages[nCount];
	}
}

void CMyTabCtrl::Init()
{
	m_tabCurrent=0;

	InsertItem(0, _T("Cơ bản"));
	m_tabPages[0]->Create(IDD_BASIC_DIALOG, this);
	InsertItem(1, _T("Phục hồi"));
	m_tabPages[1]->Create(IDD_RECOVER_DIALOG, this);
	InsertItem(2, _T("Chiến đấu"));
	m_tabPages[2]->Create(IDD_ATTACK_DIALOG, this);
	InsertItem(3, _T("Nhặt đồ"));
	m_tabPages[3]->Create(IDD_COLLECT_DIALOG, this);
	InsertItem(4, _T("Di chuyển"));
	m_tabPages[4]->Create(IDD_MOVE_DIALOG, this);
	InsertItem(5, _T("Tổ đội"));
	m_tabPages[5]->Create(IDD_PARTY_DIALOG, this);
	InsertItem(6, _T("Bản đồ"));
	m_tabPages[6]->Create(IDD_MAP_DIALOG, this);
	InsertItem(7, _T("Trợ giúp"));
	m_tabPages[7]->Create(IDD_ABOUT_DIALOG, this);

	SetRectangle();
}

void CMyTabCtrl::UpdateAllTabs(int nActiveChar)
{
	((CDlgBasic*)m_tabPages[0])->m_nActiveChar = nActiveChar;
	((CDlgRecover*)m_tabPages[1])->m_nActiveChar = nActiveChar;
	((CDlgAttack*)m_tabPages[2])->m_nActiveChar = nActiveChar;
	((CDlgCollect*)m_tabPages[3])->m_nActiveChar = nActiveChar;
	((CDlgMove*)m_tabPages[4])->m_nActiveChar = nActiveChar;
	((CDlgParty*)m_tabPages[5])->m_nActiveChar = nActiveChar;
	((CDlgMap*)m_tabPages[6])->m_nActiveChar = nActiveChar;
	((CDlgAbout*)m_tabPages[7])->m_nActiveChar = nActiveChar;
	m_tabPages[m_tabCurrent]->UpdateData(0);
}

void CMyTabCtrl::SetGameParams(int nClickedChar)
{
	((CDlgBasic*)m_tabPages[0])->SetGameParams(nClickedChar);
	((CDlgRecover*)m_tabPages[1])->SetGameParams(nClickedChar);
	((CDlgAttack*)m_tabPages[2])->SetGameParams(nClickedChar);
	((CDlgCollect*)m_tabPages[3])->SetGameParams(nClickedChar);
	((CDlgMove*)m_tabPages[4])->SetGameParams(nClickedChar);
	((CDlgParty*)m_tabPages[5])->SetGameParams(nClickedChar);
	((CDlgMap*)m_tabPages[6])->SetGameParams(nClickedChar);
}

void CMyTabCtrl::InitAllTabs(int nIndex)
{
	((CDlgBasic*)m_tabPages[0])->Init(nIndex);
	((CDlgRecover*)m_tabPages[1])->Init(nIndex);
	((CDlgAttack*)m_tabPages[2])->Init(nIndex);
	((CDlgCollect*)m_tabPages[3])->Init(nIndex);
	((CDlgMove*)m_tabPages[4])->Init(nIndex);
	((CDlgParty*)m_tabPages[5])->Init(nIndex);
	((CDlgMap*)m_tabPages[6])->Init(nIndex);
	((CDlgAbout*)m_tabPages[7])->Init(nIndex);
}

void CMyTabCtrl::LoadProfile(int nIndex, LPCTSTR szAccountID)
{
	TCHAR szFileName[MAX_PATH];
	_stprintf(szFileName, _T("%s\\UserConfig\\%s.cfg"), szCurFolder, szAccountID);
	CFile CfgFile;
	if (!CfgFile.Open(szFileName, CFile::modeRead))
		return;
	//Identification
	char szVersion[12] = "";
	CfgFile.Read(szVersion, sizeof(szVersion));
	if (strcmp(szVersion, _SZCURVERSION))
	{
		CfgFile.Close();
		return;
	}
	UINT cbSize, UIntSize;
	UIntSize = sizeof(cbSize);
	//CDlgBasic
	CDlgBasic::PARAM_BASIC paramBasic;
	CfgFile.Read(&cbSize, UIntSize);
	if ((cbSize > UIntSize) && CfgFile.Read((LPBYTE)&paramBasic+UIntSize, cbSize-UIntSize))
		memcpy(&((CDlgBasic*)m_tabPages[0])->m_Basic[nIndex], &paramBasic, cbSize);
	//CDlgRecover
	CDlgRecover::PARAM_RECOVER paramRecover;
	CfgFile.Read(&cbSize, UIntSize);
	if ((cbSize > UIntSize) && CfgFile.Read((LPBYTE)&paramRecover+UIntSize, cbSize-UIntSize))
		memcpy(&((CDlgRecover*)m_tabPages[1])->m_Recover[nIndex], &paramRecover, cbSize);
	//CDlgAttack
	CDlgAttack::PARAM_ATTACK paramAttack;
	CfgFile.Read(&cbSize, UIntSize);
	if ((cbSize > UIntSize) && CfgFile.Read((LPBYTE)&paramAttack+UIntSize, cbSize-UIntSize))
		memcpy(&((CDlgAttack*)m_tabPages[2])->m_Attack[nIndex], &paramAttack, cbSize);
	//CDlgCollect
	CDlgCollect::PARAM_COLLECT paramCollect;
	CfgFile.Read(&cbSize, UIntSize);
	if ((cbSize > UIntSize) && CfgFile.Read((LPBYTE)&paramCollect+UIntSize, cbSize-UIntSize))
		memcpy(&((CDlgCollect*)m_tabPages[3])->m_Collect[nIndex], &paramCollect, cbSize);
	//CDlgMove
	CDlgMove::PARAM_MOVE paramMove;
	CfgFile.Read(&cbSize, UIntSize);
	if ((cbSize > UIntSize) && CfgFile.Read((LPBYTE)&paramMove+UIntSize, cbSize-UIntSize))
		memcpy(&((CDlgMove*)m_tabPages[4])->m_Move[nIndex], &paramMove, cbSize);
	//CDlgParty
	CDlgParty::PARAM_PARTY paramParty;
	CfgFile.Read(&cbSize, UIntSize);
	if ((cbSize > UIntSize) && CfgFile.Read((LPBYTE)&paramParty+UIntSize, cbSize-UIntSize))
		memcpy(&((CDlgParty*)m_tabPages[5])->m_Party[nIndex], &paramParty, cbSize);
	//CDlgMap
	CDlgMap::PARAM_MAP paramMap;
	CfgFile.Read(&cbSize, UIntSize);
	if ((cbSize > UIntSize) && CfgFile.Read((LPBYTE)&paramMap+UIntSize, cbSize-UIntSize))
		memcpy(&((CDlgMap*)m_tabPages[6])->m_Map[nIndex], &paramMap, cbSize);

	m_tabPages[m_tabCurrent]->UpdateData(0);
	CfgFile.Close();
}

void CMyTabCtrl::SaveProfile(int nIndex, LPCTSTR szAccountID)
{
	TCHAR szFileName[MAX_PATH];
	if (!szAccountID[0]) return;
	_stprintf(szFileName, _T("%s\\UserConfig\\%s.cfg"), szCurFolder, szAccountID);
	CFile CfgFile;
	if (!CfgFile.Open(szFileName, CFile::modeCreate|CFile::modeWrite))
		return;
	//Identification
	char szVersion[12] = "";
	strcpy(szVersion, _SZCURVERSION);
	CfgFile.Write(szVersion, sizeof(szVersion));
	//CDlgBasic
	CDlgBasic::PARAM_BASIC paramBasic;
	paramBasic = ((CDlgBasic*)m_tabPages[0])->m_Basic[nIndex];
	paramBasic.m_cbSize = sizeof(paramBasic);
	CfgFile.Write(&paramBasic, paramBasic.m_cbSize);
	//CDlgRecover
	CDlgRecover::PARAM_RECOVER paramRecover;
	paramRecover = ((CDlgRecover*)m_tabPages[1])->m_Recover[nIndex];
	paramRecover.m_cbSize = sizeof(paramRecover);
	CfgFile.Write(&paramRecover, paramRecover.m_cbSize);
	//CDlgAttack
	CDlgAttack::PARAM_ATTACK paramAttack;
	paramAttack = ((CDlgAttack*)m_tabPages[2])->m_Attack[nIndex];
	paramAttack.m_cbSize = sizeof(paramAttack);
	CfgFile.Write(&paramAttack, paramAttack.m_cbSize);
	//CDlgCollect
	CDlgCollect::PARAM_COLLECT paramCollect;
	paramCollect = ((CDlgCollect*)m_tabPages[3])->m_Collect[nIndex];
	paramCollect.m_cbSize = sizeof(paramCollect);
	CfgFile.Write(&paramCollect, paramCollect.m_cbSize);
	//CDlgMove
	CDlgMove::PARAM_MOVE paramMove;
	paramMove = ((CDlgMove*)m_tabPages[4])->m_Move[nIndex];
	paramMove.m_cbSize = sizeof(paramMove);
	CfgFile.Write(&paramMove, paramMove.m_cbSize);
	//CDlgParty
	CDlgParty::PARAM_PARTY paramParty;
	paramParty = ((CDlgParty*)m_tabPages[5])->m_Party[nIndex];
	paramParty.m_cbSize = sizeof(paramParty);
	CfgFile.Write(&paramParty, paramParty.m_cbSize);
	//CDlgMap
	CDlgMap::PARAM_MAP paramMap;
	paramMap = ((CDlgMap*)m_tabPages[6])->m_Map[nIndex];
	paramMap.m_cbSize = sizeof(paramMap);
	CfgFile.Write(&paramMap, paramMap.m_cbSize);

	CfgFile.Close();
}

void CMyTabCtrl::SetRectangle()
{
	CRect tabRect, itemRect;
	int nX, nY, nXc, nYc;

	GetClientRect(&tabRect);
	GetItemRect(0, &itemRect);

	nX=itemRect.left;
	nY=itemRect.bottom+1;
	nXc=tabRect.right-itemRect.left-1;
	nYc=tabRect.bottom-nY-1;

	m_tabPages[0]->SetWindowPos(&wndTop, nX, nY, nXc, nYc, SWP_SHOWWINDOW);
	for(int nCount=1; nCount < NUM_OF_TABS; nCount++){
		m_tabPages[nCount]->SetWindowPos(&wndTop, nX, nY, nXc, nYc, SWP_HIDEWINDOW);
	}
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


// CHyperLink

IMPLEMENT_DYNAMIC(CHyperLink, CStatic)

CHyperLink::CHyperLink()
{
	m_strURL = _T("");
}

CHyperLink::~CHyperLink()
{
}

BEGIN_MESSAGE_MAP(CHyperLink, CStatic)
    //{{AFX_MSG_MAP(CHyperLink)
    ON_WM_CTLCOLOR_REFLECT()
	ON_WM_SETCURSOR()
	ON_CONTROL_REFLECT(STN_CLICKED, OnClicked)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

HBRUSH CHyperLink::CtlColor(CDC* pDC, UINT nCtlColor) 
{
    ASSERT(nCtlColor == CTLCOLOR_STATIC);

	pDC->SetTextColor(RGB(0,0,238));
    pDC->SetBkMode(TRANSPARENT);
    return (HBRUSH)GetStockObject(NULL_BRUSH);
}

BOOL CHyperLink::OnSetCursor(CWnd* /*pWnd*/, UINT /*nHitTest*/, UINT /*message*/) 
{
	HCURSOR hLinkCursor = ::LoadCursor(NULL, IDC_HAND); 
    if (hLinkCursor)
    {
        ::SetCursor(hLinkCursor);
        return TRUE;
    }
    return FALSE;
}

void CHyperLink::SetURL(CString strURL)
{
	m_strURL = strURL;
}

void CHyperLink::OnClicked()
{
	ShellExecute(NULL, _T("Open"), m_strURL, NULL, NULL, SW_SHOW);
}


// CMyProgressCtrl

IMPLEMENT_DYNAMIC(CMyProgressCtrl, CProgressCtrl)

CMyProgressCtrl::CMyProgressCtrl()
{
	m_nIndType = 0;
}

CMyProgressCtrl::~CMyProgressCtrl()
{
}

BEGIN_MESSAGE_MAP(CMyProgressCtrl, CProgressCtrl)
    //{{AFX_MSG_MAP(CMyProgressCtrl)
	//}}AFX_MSG_MAP
	ON_WM_PAINT()
END_MESSAGE_MAP()

void CMyProgressCtrl::OnPaint()
{
	CPaintDC dc(this); // device context for painting
	int nPos = GetPos();
	CRect rectClient, rectFill;
	GetClientRect(&rectClient);
	CBrush brush; CPen pen(PS_SOLID, 1, RGB(123, 158, 189));
	brush.CreateSolidBrush(::GetSysColor(COLOR_BTNFACE));
	dc.FillRect(&rectClient, &brush);
	dc.SelectObject(&pen);
	dc.Rectangle(&rectClient);
	VERIFY(pen.DeleteObject());
	VERIFY(brush.DeleteObject());

	if (nPos)
	{
		COLORREF fillColor;
		switch (m_nIndType)
		{
		case 0:
			if (nPos>=50) fillColor = RGB(0, 248, 0);
			else if (nPos>=25) fillColor = RGB(248, 248, 0);
			else fillColor = RGB(248, 0, 0);
			break;
		case 1:
			fillColor = RGB(0, 0, 248);
			break;
		}
		GetClientRect(&rectFill);
		rectFill.left = 1; rectFill.bottom--;
		rectFill.top = 1; rectFill.right = nPos*(rectFill.right-1)/100;
		brush.CreateSolidBrush(fillColor);
		dc.FillRect(&rectFill, &brush);
		VERIFY(brush.DeleteObject());
	}
}

void CMyProgressCtrl::SetType(int nType)
{
	m_nIndType = nType;
}
