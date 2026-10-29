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

// Dialog Data
	enum { IDD = IDD_MOVEPOINTS_DIALOG };

protected:
	virtual BOOL	OnInitDialog();
	virtual BOOL	PreTranslateMessage(MSG* pMsg);
	virtual void	DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual void	OnOK() {}
	afx_msg void	OnButtonsClicked(UINT nID);
	POINT*			GetNodes(int nList);
	int&			GetNodesMapID(int nList);

	DECLARE_MESSAGE_MAP()
public:
	CListBox		m_PoinList[MOVEPOINTS_LISTS];
	CToolTipCtrl	m_ToolTip;
};
