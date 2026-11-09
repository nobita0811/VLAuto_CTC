#pragma once

// CDlgBuffBua dialog - Cai dat buff bua (mo tu tab Chien dau, nut "Cai dat bua")
// 2 cot: [checkbox] Ten bua | Cap. Chi liet ke bua (GameDef.h g_BuffBuaTable) nhan vat da hoc (cap >= 1).
// Bua duoc check luu vao PARAM_ATTACK::m_BuaSkills (cau hinh rieng tung nhan vat), VLHook tu cast.

class CDlgBuffBua : public CDialog
{
	DECLARE_DYNAMIC(CDlgBuffBua)

public:
	CDlgBuffBua(CWnd* pParent = NULL);   // standard constructor
	virtual ~CDlgBuffBua();
	void				RefreshFromParams();	// Nap lai danh sach theo nhan vat hien tai

// Dialog Data
	enum { IDD = IDD_BUFFBUA_SETTING_DIALOG };

protected:
	virtual BOOL		OnInitDialog();
	virtual void		OnOK();
	virtual void		OnCancel();
	void				LoadBuaList();
	void				SaveParams();
	afx_msg void		OnLvnItemchanged(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void		OnOnlyPlayerClicked();

	DECLARE_MESSAGE_MAP()

public:
	CListCtrl			m_BuaList;
	BOOL				m_bLoading;
};
