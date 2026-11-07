#pragma once

// CDlgMap dialog

class CDlgMap : public CDialog
{
	DECLARE_DYNAMIC(CDlgMap)

public:
	CDlgMap(CWnd* pParent = NULL);   // standard constructor
	virtual ~CDlgMap();
	void			UpdateTrainMap(BOOL bSave);
	void			UpdateCustomBox();

// Dialog Data
	enum { IDD = IDD_MAP_DIALOG };

protected:
	virtual BOOL	OnInitDialog();
	virtual BOOL	PreTranslateMessage(MSG* pMsg);
	virtual void	DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual void	OnOK() {}
	virtual void	OnCancel() {}

	afx_msg void	OnButtonsClicked(UINT nID);
	afx_msg void	OnCbnSelchangeDriverCombo();
	afx_msg void	OnCbnSelchangeSaveitemCombo();
	afx_msg BOOL	OnCopyData(CWnd* pWnd, COPYDATASTRUCT* pCopyDataStruct);	// Nhan vi tri ruong tu VLHook

	DECLARE_MESSAGE_MAP()

public:
	CToolTipCtrl	m_ToolTip;
};
