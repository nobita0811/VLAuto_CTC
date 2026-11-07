#pragma once

// CDlgMovePoints dialog - mo tu tab Di chuyen
// Danh sach 0: toa do Hanh trinh luyen cong (m_Nodes)
// Danh sach 1: toa do di den noi luyen cong (m_GoNodes) - sau khi vao cong map train, MapPath toi diem dau roi di theo danh sach

#define MOVEPOINTS_LISTS	2

class CDlgMovePoints : public CDialog
{
	DECLARE_DYNAMIC(CDlgMovePoints)

public:
	CDlgMovePoints(CWnd* pParent = NULL);   // standard constructor
	virtual ~CDlgMovePoints();
	void			UpdatePointsList(int nList, BOOL bSave);
	void			UpdateMapName();
	void			OnBnClickedAddButton(int nList);
	void			OnBnClickedDelButton(int nList);
	void			OnBnClickedUpButton(int nList);
	void			OnBnClickedDownButton(int nList);
	void			OnBnClickedDelAllButton(int nList);
	void			OnBnClickedSaveButton(int nList);
	void			OnBnClickedLoadButton(int nList);
	BOOL			OnBnClickedEditButton(int nList);
	void			OnReadPosNode(int nList, int nMapID, POINT ptAbs);	// Chen toa do VLHook tra ve vao danh sach (goi ca tu cua so chinh khi popup dang mo)
	void			ShowSelNode(int nList);
	void			InsertPoint(int nList, int nIndex, POINT pt);
	POINT			GetPoint(int nList, int nIndex);
	static CString	GetMapName(int nMapID);
	static int		GetTrainMapID();

// Dialog Data
	enum { IDD = IDD_MOVEPOINTS_DIALOG };

protected:
	virtual BOOL	OnInitDialog();
	virtual BOOL	PreTranslateMessage(MSG* pMsg);
	virtual void	DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual void	OnOK() {}
	afx_msg void	OnButtonsClicked(UINT nID);
	afx_msg void	OnListSelChange(UINT nID);
	afx_msg BOOL	OnCopyData(CWnd* pWnd, COPYDATASTRUCT* pCopyDataStruct);	// Nhan toa do doc tu VLHook
	afx_msg void	OnDestroy();
	void			SendReadPosCmd(int nCmd, int nKind);		// Gui lenh doc toa do xuong VLHook
	POINT*			GetNodes(int nList);
	int&			GetNodesMapID(int nList);

	DECLARE_MESSAGE_MAP()
public:
	CListBox		m_PoinList[MOVEPOINTS_LISTS];
	CArray<POINT, POINT> m_Pts[MOVEPOINTS_LISTS];	// Toa do tuyet doi; ItemData cua moi dong = chi so trong mang
	CToolTipCtrl	m_ToolTip;
};

// Con tro toi cua so Danh sach toa do dang mo (NULL neu dang dong).
// Cua so chinh dung de cap nhat live khi doc toa do bang phim nong trong game.
extern CDlgMovePoints* g_pMovePointsDlg;
