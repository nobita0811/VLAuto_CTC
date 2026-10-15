// UiControls.cpp: implementation file for the User-Control classes.
//

#include "stdafx.h"
#include "UiControls.h"

// ======================================
// CTrayIcon Implementation
// ======================================
CTrayIcon::CTrayIcon()
{
	memset(&m_nid, 0 , sizeof(NOTIFYICONDATA));
	m_nid.cbSize = sizeof(NOTIFYICONDATA);
}

CTrayIcon::~CTrayIcon()
{
	SetIcon(0);
}

BOOL CTrayIcon::Create(CWnd* pWnd, UINT uID, UINT uCbMsg)
{
	m_nid.hWnd = pWnd->GetSafeHwnd();
	m_nid.uCallbackMessage = uCbMsg;
	m_nid.uID = uID;

	return TRUE;
}

BOOL CTrayIcon::SetIcon(HICON hIcon, LPCTSTR szToolTip)
{
	DWORD dwMessage;
	m_nid.uFlags = 0;

	if (hIcon)
	{
		dwMessage = m_nid.hIcon ? NIM_MODIFY : NIM_ADD;
		m_nid.hIcon = hIcon;
		m_nid.uFlags |= NIF_ICON;
	}
	else
	{
		if (m_nid.hIcon == NULL)
			return TRUE;
		dwMessage = NIM_DELETE;
	}

	if (szToolTip)
	{
		m_nid.uFlags |= NIF_TIP;
		int maxChars = sizeof(m_nid.szTip)/sizeof(m_nid.szTip[0]);
		_tcsncpy(m_nid.szTip, szToolTip, maxChars);
	}

	if (m_nid.uCallbackMessage && m_nid.hWnd)
		m_nid.uFlags |= NIF_MESSAGE;

	BOOL bRet = Shell_NotifyIcon(dwMessage, &m_nid);

	if (dwMessage == NIM_DELETE || !bRet)
		m_nid.hIcon = NULL;

	return bRet;
}

BOOL CTrayIcon::ShowBalloonTip(LPCTSTR szInfoMsg, LPCTSTR szTitle, UINT uTimeout, DWORD dwInfoFlags)
{
	m_nid.cbSize = sizeof(NOTIFYICONDATA);
	m_nid.uFlags = NIF_INFO;
	m_nid.uTimeout = uTimeout;
	m_nid.dwInfoFlags = dwInfoFlags;

	int maxChars = sizeof(m_nid.szInfo)/sizeof(m_nid.szInfo[0]);
	_tcsncpy(m_nid.szInfo, szInfoMsg ? szInfoMsg : _T(""), maxChars);

	maxChars = sizeof(m_nid.szInfoTitle)/sizeof(m_nid.szInfoTitle[0]);
	_tcsncpy(m_nid.szInfoTitle, szTitle ? szTitle : _T(""), maxChars);

	return Shell_NotifyIcon(NIM_MODIFY, &m_nid);
}

LRESULT CTrayIcon::OnTrayNotify(WPARAM wParam, LPARAM lParam)
{
	if (m_nid.uID != wParam)
		return 0;

	switch (lParam)
	{
	case WM_LBUTTONUP:
		if (!m_nid.hWnd)
			return 0;

		ShowWindow(m_nid.hWnd, SW_RESTORE);
		SetForegroundWindow(m_nid.hWnd);
		return 1;

	case WM_LBUTTONDBLCLK:
	case WM_RBUTTONUP:
		{
			CMenu menu;
			if (!menu.LoadMenu(m_nid.uID))
				return 0;

			CMenu* pSubMenu = menu.GetSubMenu(0);
			if (!pSubMenu) 
				return 0;

			if (lParam == WM_LBUTTONDBLCLK)
				SendMessage(m_nid.hWnd, WM_COMMAND, pSubMenu->GetMenuItemID(0), 0);
			else //WM_RBUTTONUP
			{
				SetMenuDefaultItem(pSubMenu->m_hMenu, 0, TRUE);
				CPoint mouse;
				GetCursorPos(&mouse);
				SetForegroundWindow(m_nid.hWnd);
				TrackPopupMenu(pSubMenu->m_hMenu, 0, mouse.x, mouse.y, 0, m_nid.hWnd, NULL);
				PostMessage(m_nid.hWnd, WM_NULL, 0, 0);
			}
		}

		return 1;
	}

	return 0;
}

LRESULT CTrayIcon::OnTaskBarCreated(WPARAM wParam, LPARAM lParam)
{
	HICON hIcon = m_nid.hIcon;
	m_nid.hIcon = NULL;

	if (hIcon)
		SetIcon(hIcon, NULL);

	return 0;
}

// ======================================
// CHyperLink Implementation
// ======================================
CHyperLink::CHyperLink()
	: m_strURL(_T(""))
{
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

BOOL CHyperLink::OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message) 
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

// ======================================
// CProgressCtrlEx Implementation
// ======================================
CProgressCtrlEx::CProgressCtrlEx()
{
	m_uPrType = 0;
}

CProgressCtrlEx::~CProgressCtrlEx()
{
}

void CProgressCtrlEx::SetProgressType(UINT uType)
{
	m_uPrType = uType;
}

BEGIN_MESSAGE_MAP(CProgressCtrlEx, CProgressCtrl)
    //{{AFX_MSG_MAP(CProgressCtrlEx)
	//}}AFX_MSG_MAP
	ON_WM_PAINT()
END_MESSAGE_MAP()

void CProgressCtrlEx::OnPaint()
{
	CPaintDC dc(this); // device context for painting
	int nPos = GetPos();
	CRect rectClient, rectFill;
	GetClientRect(&rectClient);

	CBrush brush;
	CPen pen(PS_SOLID, 1, RGB(123, 158, 189));
	brush.CreateSolidBrush(::GetSysColor(COLOR_BTNFACE));
	dc.FillRect(&rectClient, &brush);
	dc.SelectObject(&pen);
	dc.Rectangle(&rectClient);
	pen.DeleteObject();
	brush.DeleteObject();

	if (nPos)
	{
		COLORREF fillColor;
		switch (m_uPrType)
		{
		case 0:
			if (nPos >= 50) fillColor = RGB(0, 248, 0);
			else if (nPos >= 25) fillColor = RGB(248, 248, 0);
			else fillColor = RGB(248, 0, 0);
			break;
		default:
			fillColor = RGB(0, 0, 248);
			break;
		}

		GetClientRect(&rectFill);
		rectFill.left = 1; rectFill.bottom--;
		rectFill.top = 1; rectFill.right = nPos*(rectFill.right-1)/100;
		brush.CreateSolidBrush(fillColor);
		dc.FillRect(&rectFill, &brush);
		brush.DeleteObject();
	}
}

// ======================================
// CButtonEx Implementation
// ======================================
CButtonEx::CButtonEx()
{
	m_pImgList = NULL;
}

CButtonEx::~CButtonEx()
{
	if (m_pImgList != NULL)
		delete m_pImgList;
}

void CButtonEx::SetBmpImageList(UINT uResID, int nBtnWidth, LPCRECT pMargin)
{
	BITMAP bitmap;
	CBitmap cBitmap;
	if (!cBitmap.LoadBitmap(uResID) || !cBitmap.GetBitmap(&bitmap))
		return;

	if (m_pImgList != NULL)
		delete m_pImgList;
	m_pImgList = new CImageList();
	int nImgNum = bitmap.bmWidth / nBtnWidth;
	m_pImgList->Create(nBtnWidth, bitmap.bmHeight, ILC_COLOR24 | ILC_MASK, nImgNum, 0);
	m_pImgList->Add(&cBitmap, RGB(255, 0, 255));

	BUTTON_IMAGELIST btnImg;
	btnImg.himl = m_pImgList->GetSafeHandle();
	btnImg.margin = *pMargin;
	btnImg.uAlign = BUTTON_IMAGELIST_ALIGN_LEFT;

	SetImageList(&btnImg);
}

// ======================================
// CTabCtrlEx Implementation
// ======================================
IMPLEMENT_DYNAMIC(CTabCtrlEx, CTabCtrl)

CTabCtrlEx::CTabCtrlEx()
{
	m_nCurTab = 0;
	m_pImgList = NULL;
}

CTabCtrlEx::~CTabCtrlEx()
{
	if (m_pImgList != NULL)
		delete m_pImgList;
}

BEGIN_MESSAGE_MAP(CTabCtrlEx, CTabCtrl)
	//{{AFX_MSG_MAP(CTabCtrlEx)
	ON_WM_DESTROY()
	ON_WM_LBUTTONDOWN()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

void CTabCtrlEx::OnDestroy()
{
	for (int i = 0; i < GetItemCount(); i++)
	{
		CDialog* pDlg = GetTabDlg(i);
		if (pDlg == NULL) continue;
		pDlg->DestroyWindow();
		delete pDlg;
	}

	CTabCtrl::OnDestroy();
}

void CTabCtrlEx::OnLButtonDown(UINT nFlags, CPoint point) 
{
	CTabCtrl::OnLButtonDown(nFlags, point);

	int nCurTab = GetCurFocus();
	if (nCurTab != m_nCurTab)
	{
		CDialog* pOldTab = GetTabDlg(m_nCurTab);
		if (pOldTab != NULL)
			pOldTab->ShowWindow(SW_HIDE);

		CDialog* pCurTab = GetTabDlg(nCurTab);
		if (pCurTab != NULL)
		{
			pCurTab->ShowWindow(SW_SHOW);
			pCurTab->SetFocus();
			pCurTab->UpdateData(0);
		}

		m_nCurTab = nCurTab;
		ArrageToolTips();
	}
}

void CTabCtrlEx::ArrageDialogs()
{
	CRect tabRect, itemRect;
	int nX, nY, nXc, nYc;

	GetClientRect(&tabRect);
	GetItemRect(0, &itemRect);

	nX = itemRect.left;
	nY = itemRect.bottom + 2;
	nXc = tabRect.right - itemRect.left - 1;
	nYc = tabRect.bottom - nY - 2;

	for (int i = 0; i < GetItemCount(); i++)
	{
		CDialog* pDlg = GetTabDlg(i);
		if (pDlg == NULL) continue;

		if (i > 0)
			pDlg->SetWindowPos(&wndTop, nX, nY, nXc, nYc, SWP_HIDEWINDOW);
		else
			pDlg->SetWindowPos(&wndTop, nX, nY, nXc, nYc, SWP_SHOWWINDOW);
	}

	ArrageToolTips();
}

void CTabCtrlEx::SetBmpImageList(UINT uResID, int nBtnWidth)
{
	BITMAP bitmap;
	CBitmap cBitmap;
	if (!cBitmap.LoadBitmap(uResID) || !cBitmap.GetBitmap(&bitmap))
		return;

	if (m_pImgList != NULL)
		delete m_pImgList;
	m_pImgList = new CImageList();
	int nImgNum = bitmap.bmWidth / nBtnWidth;
	m_pImgList->Create(nBtnWidth, bitmap.bmHeight, ILC_COLOR24 | ILC_MASK, nImgNum, 0);
	m_pImgList->Add(&cBitmap, RGB(255, 0, 255));

	SetImageList(m_pImgList);
}

LONG CTabCtrlEx::InsertTabDlg(UINT nIndex, CDialog* pDialog, LPCTSTR szCaption, int nImage)
{
	UINT nMask = TCIF_PARAM;

	if (szCaption != NULL)
		nMask |= TCIF_TEXT;
	if (nImage >= 0)
		nMask |= TCIF_IMAGE;

	return InsertItem(nMask, nIndex, szCaption, nImage, (LPARAM)pDialog);
}

void CTabCtrlEx::SetTabToolTip(UINT nIndex, LPCTSTR szToolTip)
{
	CToolTipCtrl* pToolTip = GetToolTips();
	if (pToolTip == NULL)
		return;

	ASSERT ((int)nIndex >= 0 && (int)nIndex < GetItemCount());

	UINT nToolId = nIndex + 0x2000;
	pToolTip->DelTool(this, nToolId);
	pToolTip->AddTool(this, szToolTip, CRect(0,0,0,0), nToolId);
}

CDialog* CTabCtrlEx::GetTabDlg(UINT nIndex)
{
	TCITEM tabItem;
	ZeroMemory(&tabItem, sizeof(TCITEM));
	tabItem.mask = TCIF_PARAM;

	if (!GetItem(nIndex, &tabItem))
		return NULL;

	return (CDialog*)tabItem.lParam;
}

void CTabCtrlEx::ArrageToolTips()
{
	CToolTipCtrl* pToolTip = GetToolTips();
	if (pToolTip == NULL)
		return;

	CRect itemRect;
	for (int i = 0; i < GetItemCount(); i++)
	{
		GetItemRect(i, &itemRect);
		UINT nToolId = i + 0x2000;
		pToolTip->SetToolRect(this, nToolId, &itemRect);
	}
}
