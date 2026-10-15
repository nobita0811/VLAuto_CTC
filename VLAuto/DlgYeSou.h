#pragma once

#include "MyListCtrl.h"
#include "YSBuyItem.h"
#include "YSTuZhi.h"
#include "YSAddPoint.h"
#include "YSPrizeSel.h"
#include "YSTongQ.h"
#include "YSTongPK.h"

// CYeSouOptions
class CYeSouOptions : public CPropertySheet
{
	DECLARE_DYNAMIC(CYeSouOptions)
public:
	CYeSouOptions(LPCTSTR pszCaption, CWnd* pParentWnd = NULL);
	virtual ~CYeSouOptions();
	void			SetOptionAdds();

public:
	HWND			m_hGameWnd;
	CYSBuyItem		m_BuyItemPage;
	CYSTuZhi		m_TuZhiPage;
	CYSAddPoint		m_AddPointPage;
	CYSPrizeSel		m_PrizeSelPage;
	CYSTongQ		m_TongQPage;
	CYSTongPK		m_TongPKPage;

protected:
	DECLARE_MESSAGE_MAP()

};

// CDlgYeSou dialog

class CDlgYeSou : public CDialog
{
	DECLARE_DYNAMIC(CDlgYeSou)

public:
	CDlgYeSou(CWnd* pParent = NULL);   // standard constructor
	virtual ~CDlgYeSou();
	void			UpdateQuestList(BOOL bSave);

// Dialog Data
	enum { IDD = IDD_YESOU_DIALOG };

protected:
	virtual BOOL	OnInitDialog();
	virtual BOOL	PreTranslateMessage(MSG* pMsg);
	virtual void	DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual void	OnOK() {}
	virtual void	OnCancel() {}
	afx_msg void	OnButtonsClicked(UINT nID);
	afx_msg void	OnCbnSelchangeSuspendCombo();
	afx_msg void	OnCustomdrawCharList(NMHDR* pNMHDR, LRESULT* pResult);

	DECLARE_MESSAGE_MAP()

public:
	CToolTipCtrl	m_ToolTip;
	CYeSouOptions*	m_pMoreOptDlg;
	CMyListCtrl		m_QuestList;
};
