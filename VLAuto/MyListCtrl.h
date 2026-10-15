#pragma once

#define WM_ONSETTEXT WM_USER + 1000

// CMyEdit
class CMyEdit : public CEdit
{
	DECLARE_DYNAMIC(CMyEdit)

public:
	CMyEdit();
	virtual ~CMyEdit();

protected:
	DECLARE_MESSAGE_MAP()
public:
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnKillFocus(CWnd* pNewWnd);
};


// CMyCombo
class CMyCombo : public CComboBox
{
	DECLARE_DYNAMIC(CMyCombo)

public:
	CMyCombo();
	virtual ~CMyCombo();

protected:
	DECLARE_MESSAGE_MAP()
public:
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnKillFocus(CWnd* pNewWnd);
};

enum LISTCTRL_COLUMN_TYPE
{
	column_edit,
	column_combo,
};

// CMyListCtrl
class CMyListCtrl : public CListCtrl
{
	DECLARE_DYNAMIC(CMyListCtrl)

public:
	CMyListCtrl();
	virtual ~CMyListCtrl();

public:
	void SetColumnType(int nType);
	void InsertComboText(LPCTSTR szAddText, int nItemData = 0);
	void ResetComboText();
	void OnEdit();
	void OnCombo();
	void SetReadOnly(BOOL bReadonly = TRUE);
	BOOL IsReadOnly() { return m_bReadonly; }
	LRESULT OnSetText(WPARAM wParam, LPARAM lParam);
protected:
	//{{AFX_MSG(CMyListCtrl)
	afx_msg void OnLvnColumnclick(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnNMClick(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
	afx_msg void OnVScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
	afx_msg BOOL OnMouseWheel(UINT nFlags, short zDelta, CPoint pt);
	afx_msg void OnNcDestroy();
	//}}AFX_MSG

protected:
	int m_nCol;
	int m_nRow;
	int m_nType; // 0=Edit, 1=Combo
	BOOL m_bReadonly;
	CMyEdit *m_pEdit;
	CMyCombo *m_pCombo;

	DECLARE_MESSAGE_MAP()
	virtual BOOL OnNotify(WPARAM wParam, LPARAM lParam, LRESULT* pResult);
};
