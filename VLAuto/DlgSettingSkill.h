#pragma once

#include "MyListCtrl.h"

// CDlgSettingSkill dialog (mo tu tab Chien dau)

class CDlgSettingSkill : public CDialog
{
	DECLARE_DYNAMIC(CDlgSettingSkill)

public:
	CDlgSettingSkill(CWnd* pParent = NULL);   // standard constructor
	virtual ~CDlgSettingSkill();
	void				UpdateSkillList(BOOL bSave);

// Dialog Data
	enum { IDD = IDD_SETTING_SKILL_DIALOG };

protected:
	virtual BOOL		OnInitDialog();
	virtual BOOL		PreTranslateMessage(MSG* pMsg);
	virtual void		DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual void		OnOK();
	virtual void		OnCancel();
	void				SaveParams();
	afx_msg void		OnCbnSelchanged(UINT nID);
	afx_msg void		OnEnKillfocus(UINT nID);
	afx_msg LRESULT		OnSetTextSkillList(WPARAM wParam, LPARAM lParam);

	DECLARE_MESSAGE_MAP()

public:
	CToolTipCtrl		m_ToolTip;
	CMyListCtrl			m_SkillList;
};
