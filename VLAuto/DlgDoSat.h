#pragma once

// CDlgDoSat dialog (mo tu tab Chien dau: nut "Cai dat Do Sat , Cuu sat")
// Luu vao PARAM_ATTACK (m_nPKMapID, m_nPKMode, m_szEnemies) - cau hinh rieng tung nhan vat

class CDlgDoSat : public CDialog
{
	DECLARE_DYNAMIC(CDlgDoSat)

public:
	CDlgDoSat(CWnd* pParent = NULL);   // standard constructor
	virtual ~CDlgDoSat();
	void				RefreshFromParams();	// Nap lai giao dien theo nhan vat hien tai

// Dialog Data
	enum { IDD = IDD_DOSAT_SETTING_DIALOG };

protected:
	virtual BOOL		OnInitDialog();
	virtual void		DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual void		OnOK();
	virtual void		OnCancel();
	void				UpdateMapName();
	void				UpdateNearByPlayers();
	void				UpdateEnemyList(BOOL bSave);
	void				ApplyParams();
	BOOL				AddEnemy(LPCTSTR szName);
	afx_msg void		OnBnClickedGetMapID();
	afx_msg void		OnCbnSelchangeMode();
	afx_msg void		OnCbnDropdownNearBy();
	afx_msg void		OnBnClickedAddNearBy();
	afx_msg void		OnBnClickedAddInput();
	afx_msg void		OnBnClickedRemove();
	afx_msg void		OnBnClickedClear();
	afx_msg void		OnBnClickedSave();
	afx_msg void		OnBnClickedLoad();

	DECLARE_MESSAGE_MAP()

public:
	CListBox			m_EnemyList;
};
