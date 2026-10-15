#pragma once
#include "afxwin.h"


// CUtilityDlg dialog

class CUtilityDlg : public CDialog
{
	DECLARE_DYNAMIC(CUtilityDlg)

public:
	CUtilityDlg(CWnd* pParent = NULL);   // standard constructor
	virtual ~CUtilityDlg();

// Dialog Data
	enum { IDD = IDD_UTILITY_DIALOG };

protected:
	virtual BOOL		OnInitDialog();
	virtual BOOL		PreTranslateMessage(MSG* pMsg);
	virtual void		DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual void		OnOK() {}
	virtual void		OnCancel();
	afx_msg void		OnBnClickedMoveitemButton();
	afx_msg void		OnBnClickedMakehtButton();
	afx_msg void		OnBnClickedUpgrhtButton();
	afx_msg void		OnBnClickedGivekclButton();
	afx_msg void		OnBnClickedStopButton();

	DECLARE_MESSAGE_MAP()

private:
	PARAM_UTIL			m_UtilParam;
	CToolTipCtrl		m_ToolTip;
	CComboBox			m_ItemKindCombo;
	CComboBox			m_SourceRoomCombo;
	CComboBox			m_DestRoomCombo;
	CComboBox			m_HTLevelCombo;

	void				SetGameParams();
	void				InitComboBoxes();
};
