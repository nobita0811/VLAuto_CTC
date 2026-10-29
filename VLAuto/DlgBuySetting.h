#pragma once

// CDlgBuySetting dialog

class CDlgBuySetting : public CDialog
{
	DECLARE_DYNAMIC(CDlgBuySetting)

public:
	CDlgBuySetting(CWnd* pParent = NULL);   // standard constructor
	virtual ~CDlgBuySetting();
	void			InitComboBox(CComboBox *pComboBox);

// Dialog Data
	enum { IDD = IDD_BUY_SETTING_DIALOG };

protected:
	virtual BOOL	OnInitDialog();
	virtual BOOL	PreTranslateMessage(MSG* pMsg);
	virtual void	DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual void	OnOK();
	virtual void	OnCancel();

	afx_msg void	OnButtonsClicked(UINT nID);
	afx_msg void	OnCbnSelchangeBuyitemCombo();

	DECLARE_MESSAGE_MAP()

public:
	CToolTipCtrl	m_ToolTip;
};
