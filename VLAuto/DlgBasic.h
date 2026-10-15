#pragma once

#include "UiControls.h"

// CDlgBasic dialog

class CDlgBasic : public CDialog
{
	DECLARE_DYNAMIC(CDlgBasic)

public:
	CDlgBasic(CWnd* pParent = NULL);   // standard constructor
	virtual ~CDlgBasic();
	void				RefreshUI();

	enum { IDD = IDD_BASIC_DIALOG };

protected:
	virtual BOOL		OnInitDialog();
	virtual BOOL		PreTranslateMessage(MSG* pMsg);
	virtual void		DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual void		OnOK() {}
	virtual void		OnCancel()	{}

	afx_msg void		OnBnClickedHidewinCheck();
	afx_msg void		OnBnClickedAmexitCheck();
	afx_msg void		OnBnClickedAutoStartCheck();
	afx_msg void		OnBnClickedExpLimitCheck();
	afx_msg void		OnBnClickedTrimRamCheck();

	DECLARE_MESSAGE_MAP()

public:
	CProgressCtrlEx		m_LifeIndicator;
	CProgressCtrlEx		m_ManaIndicator;
	CToolTipCtrl		m_ToolTip;
};
