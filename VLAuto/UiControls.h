// UiControls.h: header file for the User-Control classes.
//

#pragma once

#define WM_NOTIFYICON WM_APP + 100

// ======================================
// CTrayIcon Declaration
// ======================================
class CTrayIcon
{
public:
	CTrayIcon();
	virtual ~CTrayIcon();

	BOOL				Create(CWnd* pWnd, UINT uID = 0, UINT uCbMsg = WM_NOTIFYICON);
	BOOL				SetIcon(HICON hIcon, LPCTSTR szToolTip = NULL);
	BOOL				ShowBalloonTip(LPCTSTR szInfoMsg, LPCTSTR szTitle, UINT uTimeout, DWORD dwInfoFlags = NIIF_INFO);
	virtual LRESULT		OnTrayNotify(WPARAM wParam, LPARAM lParam);
	virtual LRESULT		OnTaskBarCreated(WPARAM wParam, LPARAM lParam);

private:
	NOTIFYICONDATA		m_nid;
};

// ======================================
// CHyperLink Declaration
// ======================================
class CHyperLink : public CStatic
{
public:
	CHyperLink();
	virtual ~CHyperLink();

	void				SetURL(CString strURL);

protected:
    //{{AFX_MSG(CHyperLink)
    afx_msg HBRUSH		CtlColor(CDC* pDC, UINT nCtlColor);
	afx_msg BOOL		OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message);
	afx_msg void		OnClicked();
	//}}AFX_MSG
    DECLARE_MESSAGE_MAP()

private:
	CString				m_strURL;
};

// ======================================
// CProgressCtrlEx Declaration
// ======================================
class CProgressCtrlEx : public CProgressCtrl
{
public:
	CProgressCtrlEx();
	virtual ~CProgressCtrlEx();

	void				SetProgressType(UINT uType);

protected:
    //{{AFX_MSG(CProgressCtrlEx)
	afx_msg void		OnPaint();
	//}}AFX_MSG
    DECLARE_MESSAGE_MAP()

private:
	UINT m_uPrType;
};

// ======================================
// CButtonEx Declaration
// ======================================
class CButtonEx : public CButton
{
public:
	CButtonEx();
	virtual ~CButtonEx();

	void				SetBmpImageList(UINT uResID, int nBtnWidth, LPCRECT pMargin);

private:
	CImageList*			m_pImgList;
};

// ======================================
// CTabCtrlEx Declaration
// ======================================
class CTabCtrlEx : public CTabCtrl
{
	DECLARE_DYNAMIC(CTabCtrlEx)

public:
	CTabCtrlEx();
	virtual ~CTabCtrlEx();

	void				SetBmpImageList(UINT uResID, int nBtnWidth);
	LONG				InsertTabDlg(UINT nIndex, CDialog* pDialog, LPCTSTR szCaption = NULL, int nImage = -1);
	void				SetTabToolTip(UINT nIndex, LPCTSTR szToolTip);
	int					GetCurTabIdx() { return m_nCurTab; }
	CDialog*			GetCurTabDlg() { return GetTabDlg(m_nCurTab); }
	CDialog*			GetTabDlg(UINT nIndex);
	void				ArrageDialogs();

protected:
	//{{AFX_MSG(CMyTabCtrl)
	afx_msg void		OnDestroy();
	afx_msg void		OnLButtonDown(UINT nFlags, CPoint point);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

private:
	int					m_nCurTab;
	CImageList*			m_pImgList;
	void				ArrageToolTips();
};
