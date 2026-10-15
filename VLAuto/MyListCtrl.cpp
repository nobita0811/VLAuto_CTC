// MyListCtrl.cpp : implementation file
//

#include "stdafx.h"
#include "VLAuto.h"
#include "MyListCtrl.h"

// CMyEdit

IMPLEMENT_DYNAMIC(CMyEdit, CEdit)

CMyEdit::CMyEdit()
{
}

CMyEdit::~CMyEdit()
{
}

BEGIN_MESSAGE_MAP(CMyEdit, CEdit)
	ON_WM_CREATE()
	ON_WM_KILLFOCUS()
END_MESSAGE_MAP()

// CMyEdit message handlers
int CMyEdit::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CEdit::OnCreate(lpCreateStruct) == -1)
		return -1;
    CFont* Font = GetParent()->GetFont();
    SetFont (Font);
	return 0;
}

void CMyEdit::OnKillFocus(CWnd* pNewWnd)
{
	CEdit::OnKillFocus(pNewWnd);
	GetParent()->SendMessage(WM_ONSETTEXT);
	ShowWindow(SW_HIDE);
}

// CMyCombo

IMPLEMENT_DYNAMIC(CMyCombo, CComboBox)

CMyCombo::CMyCombo()
{
}

CMyCombo::~CMyCombo()
{
}

BEGIN_MESSAGE_MAP(CMyCombo, CComboBox)
	ON_WM_CREATE()
	ON_WM_KILLFOCUS()
END_MESSAGE_MAP()

// CMyCombo message handlers
int CMyCombo::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CComboBox::OnCreate(lpCreateStruct) == -1)
		return -1;
    CFont* Font = GetParent()->GetFont();
    SetFont (Font);
	return 0;
}

void CMyCombo::OnKillFocus(CWnd* pNewWnd)
{
	CComboBox::OnKillFocus(pNewWnd);
	GetParent()->SendMessage(WM_ONSETTEXT);
	ShowWindow(SW_HIDE);
}

// CMyListCtrl

IMPLEMENT_DYNAMIC(CMyListCtrl, CListCtrl)

CMyListCtrl::CMyListCtrl()
{
	m_pEdit = NULL;
	m_pCombo = NULL;
	m_nRow = 0;
	m_nCol = -1;
	m_nType = 0;
	m_bReadonly = 0;
}

CMyListCtrl::~CMyListCtrl()
{
}

BEGIN_MESSAGE_MAP(CMyListCtrl, CListCtrl)
	//{{AFX_MSG_MAP(CMyListCtrl)
	ON_MESSAGE(WM_ONSETTEXT, &CMyListCtrl::OnSetText)
	ON_NOTIFY_REFLECT(LVN_COLUMNCLICK, &CMyListCtrl::OnLvnColumnclick)
	ON_NOTIFY_REFLECT(NM_CLICK, &CMyListCtrl::OnNMClick)
	ON_WM_HSCROLL()
	ON_WM_VSCROLL()
	ON_WM_MOUSEWHEEL()
	ON_WM_NCDESTROY()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

// CMyListCtrl message handlers

void CMyListCtrl::OnLvnColumnclick(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMLISTVIEW pNMLV = reinterpret_cast<LPNMLISTVIEW>(pNMHDR);
	
	if (m_pEdit && m_pEdit->IsWindowVisible())
		m_pEdit->ShowWindow(SW_HIDE);

	if (m_pCombo && m_pCombo->IsWindowVisible())
		m_pCombo->ShowWindow(SW_HIDE);

	*pResult = 0;
}

void CMyListCtrl::OnNMClick(NMHDR *pNMHDR, LRESULT *pResult)
{
	if (m_bReadonly) return;

	LPNMLISTVIEW pNMLV = reinterpret_cast<LPNMLISTVIEW>(pNMHDR);
	if(pNMLV->iItem == -1 || pNMLV->iSubItem == -1)
	{
		*pResult =0;
		return;
	}
	m_nRow = pNMLV->iItem;
    m_nCol = pNMLV->iSubItem;
	if (m_nCol > 0)
	{
		switch(m_nType)
		{
		case column_edit:
			OnEdit();
			break;
		case column_combo:
			OnCombo();
			break;
		}
	}
	*pResult = 0;
}

void CMyListCtrl::SetColumnType(int nType)
{
	m_nType = nType;
}

void CMyListCtrl::InsertComboText(LPCTSTR szAddText, int nItemData)
{
	if (!m_pCombo) //If combo box does not exist create one
	{
		m_pCombo = new CMyCombo();
		DWORD dwStyle = WS_CHILD|WS_VISIBLE|WS_CLIPSIBLINGS|WS_BORDER|CBS_DROPDOWNLIST|WS_VSCROLL|WS_HSCROLL; 
		CRect rect;
		GetSubItemRect(m_nRow, m_nCol, LVIR_LABEL, rect);
		rect.bottom += 100;
		m_pCombo->Create(dwStyle, rect, this, 1002);
		m_pCombo->ShowWindow(SW_HIDE);
	}
	int nIdx = m_pCombo->AddString(szAddText);
	if (nItemData)
		m_pCombo->SetItemData(nIdx, nItemData);
}

void CMyListCtrl::ResetComboText()
{
	if (m_pCombo)
		m_pCombo->ResetContent();
}

void CMyListCtrl::OnEdit()
{

	if (!m_pEdit) //If Edit box does not exist create one
	{
		LV_COLUMN lvCol;
		lvCol.mask = LVCF_FMT;
		GetColumn(m_nCol, &lvCol);
		DWORD dwStyle;
		if ((lvCol.fmt & LVCFMT_JUSTIFYMASK) == LVCFMT_LEFT)
			dwStyle = ES_LEFT;
		else if ((lvCol.fmt & LVCFMT_JUSTIFYMASK) == LVCFMT_RIGHT)
			dwStyle = ES_RIGHT;
		else dwStyle = ES_CENTER;

		m_pEdit = new CMyEdit();
		dwStyle |= WS_CHILD|WS_VISIBLE|WS_CLIPSIBLINGS|ES_NUMBER|ES_AUTOHSCROLL; 
		CRect rect;
		GetSubItemRect(m_nRow, m_nCol, LVIR_LABEL, rect);
		rect.left += 3; rect.right -= 3;
		rect.top += 2; rect.bottom -= 2;
		m_pEdit->Create(dwStyle, rect, this, 1001);
		CString szCurText = GetItemText(m_nRow, m_nCol);
		m_pEdit->SetWindowText(szCurText);
		m_pEdit->ShowWindow(SW_SHOW);
		m_pEdit->SetSel(0, -1);
		m_pEdit->SetFocus();
		if(m_pCombo) m_pCombo->ShowWindow(SW_HIDE);
	}
	else
	{
		CRect rect;
		GetSubItemRect(m_nRow, m_nCol, LVIR_LABEL, rect);
		rect.left += 3; rect.right -= 3;
		rect.top += 2; rect.bottom -= 2;
		m_pEdit->SetWindowPos(NULL, rect.left, rect.top, rect.Width(), rect.Height(), SWP_SHOWWINDOW);
		CString szCurText = GetItemText(m_nRow, m_nCol);
		m_pEdit->SetWindowText(szCurText);
		m_pEdit->SetSel(0, -1);
		m_pEdit->SetFocus();
		if(m_pCombo) m_pCombo->ShowWindow(SW_HIDE);
	}
}

void CMyListCtrl::OnCombo()
{
	if(!m_pCombo) //If combo box does not exist create one
	{
		m_pCombo = new CMyCombo();
		DWORD dwStyle = WS_CHILD|WS_VISIBLE|WS_CLIPSIBLINGS|WS_BORDER|CBS_DROPDOWNLIST|WS_VSCROLL|WS_HSCROLL; 
		CRect rect;
		GetSubItemRect(m_nRow, m_nCol, LVIR_LABEL, rect);
		rect.bottom += 100;
		m_pCombo->Create(dwStyle, rect, this, 1002);
		m_pCombo->SetCurSel(0);
		m_pCombo->ShowWindow(SW_SHOW);
		m_pCombo->SetFocus();
		if(m_pEdit) m_pEdit->ShowWindow(SW_HIDE);
	}
	else
	{
		CRect rect;
		GetSubItemRect(m_nRow,m_nCol,LVIR_LABEL,rect);
		rect.bottom += 100;
		m_pCombo->SetWindowPos(NULL, rect.left, rect.top, rect.Width(), rect.Height(), SWP_SHOWWINDOW);
		CString szCurText = GetItemText(m_nRow, m_nCol);
		m_pCombo->SetCurSel(m_pCombo->FindString(0, szCurText));
		m_pCombo->SetFocus();
		if(m_pEdit) m_pEdit->ShowWindow(SW_HIDE);
	}
}

void CMyListCtrl::SetReadOnly(BOOL bReadonly)
{
	m_bReadonly = bReadonly;
}

LRESULT CMyListCtrl::OnSetText(WPARAM wParam, LPARAM lParam)
{
	CString szNewText;

	switch (m_nType)
	{
	case column_edit:
		if (m_pEdit)
		{
			m_pEdit->GetWindowText(szNewText);
			szNewText.Trim();
			SetItemText(m_nRow, m_nCol, szNewText);
		}
		break;
	case column_combo:
		if (m_pCombo)
		{
			int nCurIdx = m_pCombo->GetCurSel();
			if (nCurIdx < 0) break;
			int nItemData = (int)m_pCombo->GetItemData(nCurIdx);
			m_pCombo->GetLBText(nCurIdx, szNewText);
			szNewText.Trim();
			SetItemText(m_nRow, m_nCol, szNewText);
			if (!nItemData)
				SetItemData(m_nRow, nCurIdx);
			else
				SetItemData(m_nRow, nItemData);
		}
		break;
	}

	GetParent()->SendMessage(WM_ONSETTEXT);

	return 0;
}

BOOL CMyListCtrl::OnNotify(WPARAM wParam, LPARAM lParam, LRESULT* pResult)
{
	switch (((NMHDR*)lParam)->code)
	{
	case HDN_BEGINTRACKW:
	case HDN_BEGINTRACKA:
		if (m_pEdit && m_pEdit->IsWindowVisible())
			m_pEdit->ShowWindow(SW_HIDE);

		if (m_pCombo && m_pCombo->IsWindowVisible())
			m_pCombo->ShowWindow(SW_HIDE);
	}

	return CListCtrl::OnNotify(wParam, lParam, pResult);
}

void CMyListCtrl::OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar)
{
	if (m_pEdit && m_pEdit->IsWindowVisible())
		m_pEdit->ShowWindow(SW_HIDE);

	if (m_pCombo && m_pCombo->IsWindowVisible())
		m_pCombo->ShowWindow(SW_HIDE);

	CListCtrl::OnHScroll(nSBCode, nPos, pScrollBar);
}

void CMyListCtrl::OnVScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar)
{
	if (m_pEdit && m_pEdit->IsWindowVisible())
		m_pEdit->ShowWindow(SW_HIDE);

	if (m_pCombo && m_pCombo->IsWindowVisible())
		m_pCombo->ShowWindow(SW_HIDE);

	CListCtrl::OnVScroll(nSBCode, nPos, pScrollBar);
}

BOOL CMyListCtrl::OnMouseWheel(UINT nFlags, short zDelta, CPoint pt)
{
	if (m_pEdit && m_pEdit->IsWindowVisible())
		m_pEdit->ShowWindow(SW_HIDE);

	if (m_pCombo && m_pCombo->IsWindowVisible())
		m_pCombo->ShowWindow(SW_HIDE);

	return CListCtrl::OnMouseWheel(nFlags, zDelta, pt);
}

void CMyListCtrl::OnNcDestroy()
{
	CListCtrl::OnNcDestroy();
	if (m_pEdit)
	{
		delete m_pEdit;
		m_pEdit = NULL;
	}
	if (m_pCombo)
	{
		delete m_pCombo;
		m_pCombo = NULL;
	}
}
