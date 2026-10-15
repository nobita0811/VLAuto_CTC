#pragma once

// CYSAddPoint dialog

class CYSAddPoint : public CPropertyPage
{
	DECLARE_DYNAMIC(CYSAddPoint)

public:
	CYSAddPoint();
	virtual ~CYSAddPoint();

// Dialog Data
	enum { IDD = IDD_ADDPOINT_DIALOG };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual BOOL OnInitDialog();
	virtual void OnOK();
	void UpdateItemSelCombo();
	afx_msg void OnBnClickedAddButton();
	afx_msg void OnBnClickedDelButton();

	DECLARE_MESSAGE_MAP()
public:
	YSAddPointOpt *m_pAddPointOpt;
	CListCtrl m_UseItemList;
};
