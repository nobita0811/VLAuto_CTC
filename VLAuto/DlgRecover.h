#pragma once

class CDlgRecover : public CDialog
{
	DECLARE_DYNAMIC(CDlgRecover)

public:
	CDlgRecover(CWnd* pParent = NULL);
	virtual ~CDlgRecover();

	enum { IDD = IDD_RECOVER_DIALOG };

protected:
	virtual BOOL	OnInitDialog();
	virtual BOOL	PreTranslateMessage(MSG* pMsg);
	virtual void	DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual void	OnOK() {}
	virtual void	OnCancel() {}
	afx_msg void	OnButtonsClicked(UINT nID);

	DECLARE_MESSAGE_MAP()

public:
	CToolTipCtrl	m_ToolTip;
};
