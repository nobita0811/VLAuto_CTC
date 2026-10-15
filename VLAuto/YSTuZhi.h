#pragma once

// CYSTuZhi dialog

class CYSTuZhi : public CPropertyPage
{
	DECLARE_DYNAMIC(CYSTuZhi)

public:
	CYSTuZhi();
	virtual ~CYSTuZhi();

// Dialog Data
	enum { IDD = IDD_TUZHI_DIALOG };

protected:
	virtual void	DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual BOOL	OnInitDialog();
	virtual void	OnOK();

	DECLARE_MESSAGE_MAP()
public:
	YSTuZhiOpt*		m_pTuZhiOpt;
	CListCtrl		m_TuZhiList;
	CListCtrl		m_MiZhiList;
};
