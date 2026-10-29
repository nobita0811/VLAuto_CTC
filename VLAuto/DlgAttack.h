#pragma once

#include "DlgSettingSkill.h"

// CDlgAttack dialog

class CDlgAttack : public CDialog
{
	DECLARE_DYNAMIC(CDlgAttack)

public:
	CDlgAttack(CWnd* pParent = NULL);   // standard constructor
	virtual ~CDlgAttack();

// Dialog Data
	enum { IDD = IDD_ATTACK_DIALOG };

protected:
	virtual BOOL		OnInitDialog();
	virtual BOOL		PreTranslateMessage(MSG* pMsg);
	virtual void		DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual void		OnOK() {}
	virtual void		OnCancel() {}
	afx_msg void		OnButtonsClicked(UINT nID);
	afx_msg void		OnCbnSelchanged(UINT nID);

	DECLARE_MESSAGE_MAP()

public:
	CToolTipCtrl		m_ToolTip;
	CDlgSettingSkill	m_dlgSkill;			// Cua so Cai dat skill (modal)
};
