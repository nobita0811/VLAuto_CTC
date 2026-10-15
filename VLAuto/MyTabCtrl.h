// MyTabCtrl.h: interface for the CMyTabCtrl class.
//

#pragma once

// CMyTabCtrl

enum TAB_INDEXES
{
	tab_basic = 0,
	tab_recover,
	tab_attack,
	tab_collect,
	tab_move,
	tab_party,
	tab_map,
	tab_yesou,
	tab_about,
	tab_num,
};

class CMyTabCtrl : public CTabCtrl
{
	DECLARE_DYNAMIC(CMyTabCtrl)

	void SetRectangle();

public:
	CMyTabCtrl();
	virtual ~CMyTabCtrl();
	void Init();
	void DestroyItems();
	void UpdateAllTabs(int nActiveChar);
	void SetGameParams(int nClickedChar);
	void InitAllTabs(int nIndex);
	void LoadProfile(int nIndex, const char* szAccountID);
	BOOL LoadConfigFile(int nIndex, LPCTSTR zsFileName, const char* szAccountID);
	void SaveProfile(int nIndex, const char* szAccountID);
	BOOL SaveConfigFile(int nIndex, LPCTSTR szFileName, const char* szAccountID);

public:
	CDialog *m_tabPages[tab_num];
	int m_tabCurrent;

protected:
	//{{AFX_MSG(CMyTabCtrl)
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};
