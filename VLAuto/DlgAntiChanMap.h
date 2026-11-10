#pragma once

// CDlgAntiChanMap dialog : Cai dat chong chan map (mo tu tab Co ban bang nut)

class CDlgAntiChanMap : public CDialog
{
	DECLARE_DYNAMIC(CDlgAntiChanMap)

public:
	CDlgAntiChanMap(CWnd* pParent = NULL);   // standard constructor
	virtual ~CDlgAntiChanMap();

	enum { IDD = IDD_ANTI_CHANMAP_SETTING_DIALOG };

protected:
	virtual BOOL		OnInitDialog();
	virtual BOOL		PreTranslateMessage(MSG* pMsg);
	virtual void		DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual void		OnOK();
	virtual void		OnCancel();

	afx_msg void		OnBnClickedChanMapCheck();
	afx_msg void		OnBnClickedChanMapOption();
	afx_msg void		OnBnClickedChanMapGate();
	afx_msg void		OnBnClickedChanMapDir();
	afx_msg void		OnEnKillfocusChanMapEdit();
	void				UpdateChanMapInfo();

	DECLARE_MESSAGE_MAP()

public:
	CToolTipCtrl		m_ToolTip;
};
