#pragma once
#include "MyListCtrl.h"

// CYSPrizeSel : Property page dialog

class CYSPrizeSel : public CPropertyPage
{
	DECLARE_DYNAMIC(CYSPrizeSel)

public:
	CYSPrizeSel();
	virtual ~CYSPrizeSel();

// Dialog Data
	enum { IDD = IDD_PRIZESEL_DIALOG };

protected:
	virtual void		DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual BOOL		OnInitDialog();
	virtual void		OnOK();
	afx_msg void		OnBnClickedResetButton();
	afx_msg void		OnBnClickedLoadButton();
	afx_msg void		OnBnClickedSaveButton();

	DECLARE_MESSAGE_MAP()
public:
	YSPrizeSelOpt*		m_pPrizeSelOpt;
	CMyListCtrl			m_PrizeList;
};
