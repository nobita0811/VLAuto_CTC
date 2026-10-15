#pragma once

// CYSTongQ dialog

class CYSTongQ : public CPropertyPage
{
	DECLARE_DYNAMIC(CYSTongQ)

public:
	CYSTongQ();
	virtual ~CYSTongQ();

// Dialog Data
	enum { IDD = IDD_TONGQ_DIALOG };

protected:
	virtual void	DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual BOOL	OnInitDialog();

	DECLARE_MESSAGE_MAP()

public:
	YSTongQOpt*		m_pTongQOpt;
	CListCtrl		m_CollQList;
	CListCtrl		m_KillQList;
};
