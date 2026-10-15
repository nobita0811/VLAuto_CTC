#pragma once

// CDlgMovePoints dialog - Danh sach toa do Hanh trinh (mo tu tab Di chuyen)

class CDlgMovePoints : public CDialog
{
	DECLARE_DYNAMIC(CDlgMovePoints)

public:
	CDlgMovePoints(CWnd* pParent = NULL);   // standard constructor
	virtual ~CDlgMovePoints();
	void			UpdatePointsList(BOOL bSave);
	void			UpdateMapName();
	void			OnBnClickedAddButton();
	void			OnBnClickedDelButton();
	void			OnBnClickedUpButton();
	void			OnBnClickedDownButton();
	void			OnBnClickedDelAllButton();
	void			OnBnClickedSaveButton();
	void			OnBnClickedLoadButton();

// Dialog Data
	enum { IDD = IDD_MOVEPOINTS_DIALOG };

protected:
	virtual BOOL	OnInitDialog();
	virtual BOOL	PreTranslateMessage(MSG* pMsg);
	virtual void	DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual void	OnOK() {}
	afx_msg void	OnButtonsClicked(UINT nID);

	DECLARE_MESSAGE_MAP()
public:
	CListBox		m_PoinList;
	CToolTipCtrl	m_ToolTip;
};
