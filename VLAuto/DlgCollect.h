#pragma once

// CDlgCollect dialog

class CDlgCollect : public CDialog
{
	DECLARE_DYNAMIC(CDlgCollect)

public:
	CDlgCollect(CWnd* pParent = NULL);   // standard constructor
	virtual ~CDlgCollect();
	void			UpdateMagicOptions(BOOL bSave);
	void			OnBnClickedAddButton();
	void			OnBnClickedDelButton();
	void			OnBnClickedSaveButton();
	void			OnBnClickedLoadButton();

// Dialog Data
	enum { IDD = IDD_COLLECT_DIALOG };

protected:
	virtual BOOL	OnInitDialog();
	virtual BOOL	PreTranslateMessage(MSG* pMsg);
	virtual void	DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual void	OnOK() {}
	virtual void	OnCancel() {}

	afx_msg void	OnButtonsClicked(UINT nID);
	afx_msg void	OnCbnSelchangeItemattribCombo();
	afx_msg void	OnCbnSelchangeItemCombo();
	afx_msg void	OnCbnSelchangeVioletitemCombo();
	afx_msg void	OnLbnSelchangeItemattribList();

	DECLARE_MESSAGE_MAP()
public:
	CComboBox		m_ItemAttribCombo;
	CListBox		m_ItemAttribList;
	CToolTipCtrl	m_ToolTip;
};
