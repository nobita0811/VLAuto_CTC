#pragma once


// CFindItemDlg dialog

class CFindItemDlg : public CDialog
{
	DECLARE_DYNAMIC(CFindItemDlg)

public:
	CFindItemDlg(CWnd* pParent = NULL);   // standard constructor
	virtual ~CFindItemDlg();

// Dialog Data
	enum { IDD = IDD_FINDITEM_DIALOG };

protected:
	virtual BOOL		OnInitDialog();
	virtual BOOL		PreTranslateMessage(MSG* pMsg);
	virtual void		DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual void		OnOK() {}
	virtual void		OnCancel();
	afx_msg void		OnBnClickedFinditemButton();
	afx_msg void		OnBnClickedStopButton();

	DECLARE_MESSAGE_MAP()

private:
	PARAM_UTIL			m_UtilParam;
	CToolTipCtrl		m_ToolTip;
	CComboBox			m_ItemNameCombo;
	CComboBox			m_EquipTypeCombo;
	CComboBox			m_EquipSexCombo;
	CComboBox			m_ItemSeriesCombo;
	CComboBox			m_ItemSpTypeCombo;
	CComboBox			m_ItemAttribCombo;
	CComboBox			m_AttribLineCombo;

	void				SetGameParams();
	void				InitComboBoxes();
};
