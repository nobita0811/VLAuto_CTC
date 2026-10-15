// VLAutoDlg.h : header file
//

#pragma once

#include "UiControls.h"
#include "DlgAutoLogin.h"

// CVLAutoDlg dialog
class CVLAutoDlg : public CDialog
{
// Construction
public:
	CVLAutoDlg(CWnd* pParent = NULL);	// standard constructor
	virtual ~CVLAutoDlg();
	enum { IDD = IDD_VLAUTO_DIALOG };

	CTabCtrlEx			m_MainTab;

// Implementation
protected:
	HICON				m_hIcon;
	CTrayIcon			m_TrayIcon;
	CToolTipCtrl		m_ToolTip;
	CListCtrl			m_CharList;
	CDlgAutoLogin		m_DlgAutoLogin;
	//CHyperLink			m_WebLink;

	virtual void		DoDataExchange(CDataExchange* pDX);
	virtual BOOL		OnInitDialog();
	virtual BOOL		PreTranslateMessage(MSG* pMsg);
	virtual void		OnOK() {}
	virtual void		OnCancel();
	virtual BOOL		OnCommand(WPARAM wParam, LPARAM lParam);
	void				SendAttackCommand(HWND hWnd);

	// Generated message map functions
	afx_msg void		OnPaint();
	afx_msg HCURSOR		OnQueryDragIcon();
	afx_msg LRESULT		OnTrayNotify(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT		OnTaskBarCreated(WPARAM wParam, LPARAM lParam);
	afx_msg void		OnActivate(UINT nState, CWnd* pWndOther, BOOL bMinimized);
	afx_msg void		OnDestroy();
	afx_msg void		OnTimer(UINT nIDEvent);
	afx_msg void		OnLvnItemchangedCharList(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void		OnNMDblclkCharList(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void		OnCustomdrawCharList(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg LRESULT		OnReceiveFeedBack(WPARAM wParam, LPARAM lParam);
	afx_msg void		OnBnClickedAutoLoginButton();

	DECLARE_MESSAGE_MAP()

private:
	void				InitTabDialogs();
	void				OnExitDlg(INT_PTR nExitCode);
	void				OnShutDown(BOOL bRestart = FALSE);
};
