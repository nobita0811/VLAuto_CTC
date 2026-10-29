#pragma once

// CDlgCollect dialog

class CDlgCollect : public CDialog
{
	DECLARE_DYNAMIC(CDlgCollect)

public:
	CDlgCollect(CWnd* pParent = NULL);   // standard constructor
	virtual ~CDlgCollect();

// Dialog Data
	enum { IDD = IDD_COLLECT_DIALOG };

protected:
	virtual BOOL	OnInitDialog();
	virtual BOOL	PreTranslateMessage(MSG* pMsg);
	virtual void	DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual void	OnOK() {}
	virtual void	OnCancel() {}

	afx_msg void	OnButtonsClicked(UINT nID);
	afx_msg void	OnCbnSelchangeItemCombo();
	afx_msg void	OnCbnSelchangeVioletitemCombo();

	DECLARE_MESSAGE_MAP()
public:
	CToolTipCtrl	m_ToolTip;
};
