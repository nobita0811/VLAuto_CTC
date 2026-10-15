#pragma once

// CMyTabCtrl

#define NUM_OF_TABS 8

class CMyTabCtrl : public CTabCtrl
{
	DECLARE_DYNAMIC(CMyTabCtrl)

	void SetRectangle();

public:
	CMyTabCtrl();
	virtual ~CMyTabCtrl();
	void Init();
	void UpdateAllTabs(int nActiveChar);
	void SetGameParams(int nClickedChar);
	void InitAllTabs(int nIndex);
	void LoadProfile(int nIndex, LPCTSTR szAccountID);
	void SaveProfile(int nIndex, LPCTSTR szAccountID);

public:
	CDialog *m_tabPages[NUM_OF_TABS];
	int m_tabCurrent;

protected:
	//{{AFX_MSG(CMyTabCtrl)
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

// CHyperLink

class CHyperLink : public CStatic
{
	DECLARE_DYNAMIC(CHyperLink)

public:
	CHyperLink();
	virtual ~CHyperLink();
	void SetURL(CString strURL);
private:
	CString  m_strURL;

protected:
    //{{AFX_MSG(CHyperLink)
    afx_msg HBRUSH CtlColor(CDC* pDC, UINT nCtlColor);
	afx_msg BOOL OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message);
	//}}AFX_MSG
	afx_msg void OnClicked();
    DECLARE_MESSAGE_MAP()
};

// CMyProgressCtrl

class CMyProgressCtrl : public CProgressCtrl
{
	DECLARE_DYNAMIC(CMyProgressCtrl)

	int m_nIndType;
public:
	CMyProgressCtrl();
	virtual ~CMyProgressCtrl();
	void SetType(int nType);

protected:
    //{{AFX_MSG(CMyProgressCtrl)
	afx_msg void OnPaint();
	//}}AFX_MSG
    DECLARE_MESSAGE_MAP()
};
