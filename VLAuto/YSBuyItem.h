#pragma once
#include "MyListCtrl.h"

// CYSBuyItem : Property page dialog
class CYSBuyItem : public CPropertyPage
{
	DECLARE_DYNAMIC(CYSBuyItem)

public:
	CYSBuyItem();
	virtual ~CYSBuyItem();

// Dialog Data
	enum { IDD = IDD_BUYITEM_DIALOG };

protected:
	virtual void		DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual BOOL		OnInitDialog();
	virtual void		OnOK();

	DECLARE_MESSAGE_MAP()
public:
	YSBuyItemOpt*		m_pBuyItemOpt;
	CListCtrl			m_CityList;
	CMyListCtrl			m_ItemPriceList;
};
