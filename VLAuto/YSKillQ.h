#pragma once

// CYSKillQ dialog - Cai dat nhiem vu Danh quai Da Tau (trang "Danh quai" trong Tuy chon Da Tau)
// Danh sach map lay tu Maps\MapDaTau.ini (UTF-8): [n] Map=<ID>_<ten>, Ruong=, AttackRange=, Points=x,y;x,y;...
// Cai dat rieng nhan vat luu trong cfg (YSKillQOpt), map chua sua thi dung mac dinh trong ini

struct KQIniMap
{
	int		nSect;			// So thu tu [n]
	int		nMapID;
	CString	szName;
	int		nRange;
	CArray<POINT, POINT> Pts;
};

class CYSKillQ : public CPropertyPage
{
	DECLARE_DYNAMIC(CYSKillQ)

public:
	CYSKillQ();
	virtual ~CYSKillQ();

// Dialog Data
	enum { IDD = IDD_QUEST_ATTACK_DIALOG };

protected:
	virtual void	DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual BOOL	OnInitDialog();
	virtual BOOL	PreTranslateMessage(MSG* pMsg);
	afx_msg void	OnButtonsClicked(UINT nID);
	afx_msg void	OnMapListChanged(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void	OnPointSelChange();
	afx_msg void	OnRangeChange();

	void			LoadIni();
	void			FillMapList();
	void			ShowMap(int nIndex);
	void			ShowSelNode();
	void			InsertPoint(int nIndex, POINT pt);
	POINT			GetPoint(int nIndex);
	void			SavePoints();
	KQIniMap*		CurMap();
	void			OnAdd();
	void			OnDel();
	void			OnMove(int nDir);
	void			OnDelAll();
	BOOL			OnEditNode();
	void			OnSaveFile();
	void			OnLoadFile();
	void			OnSaveDefault();
	void			OnLoadDefault();
	BOOL			WriteIniDefault(KQIniMap* pMap, int nRange, const CArray<POINT, POINT>& Pts);

	DECLARE_MESSAGE_MAP()
public:
	YSKillQOpt*		m_pKillQOpt;
	CListCtrl		m_MapList;
	CListBox		m_PointList;
	CToolTipCtrl	m_ToolTip;
	CArray<POINT, POINT> m_Pts;		// Toa do tuyet doi; ItemData moi dong = chi so trong mang
	KQIniMap		m_IniMaps[MAX_KQ_MAP];
	int				m_nIniMaps;
	int				m_nCurMap;		// Dong dang chon trong danh sach map
	BOOL			m_bFilling;
	CString			m_szIniFile;
};
