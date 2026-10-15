#pragma once

// CDlgMap dialog

class CDlgMap : public CDialog
{
	DECLARE_DYNAMIC(CDlgMap)

public:
	CDlgMap(CWnd* pParent = NULL);   // standard constructor
	virtual ~CDlgMap();
	void			InitComboBox(CComboBox *pComboBox);
	void			UpdateTrainMap(BOOL bSave);

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
	afx_msg void	OnCbnSelchangeTrainmapCombo();
	afx_msg void	OnCbnSelchangeSaveitemCombo();

	DECLARE_MESSAGE_MAP()

public:
	CToolTipCtrl	m_ToolTip;
};
