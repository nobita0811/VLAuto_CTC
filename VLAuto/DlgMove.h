#pragma once

// CDlgMove dialog

class CDlgMove : public CDialog
{
	DECLARE_DYNAMIC(CDlgMove)

public:
	CDlgMove(CWnd* pParent = NULL);   // standard constructor
	virtual ~CDlgMove();
	void			UpdateNearByPlayers(BOOL bSave);
	void			UpdatePointsList();

// Dialog Data
	enum { IDD = IDD_MOVE_DIALOG };

protected:
	virtual BOOL	OnInitDialog();
	virtual BOOL	PreTranslateMessage(MSG* pMsg);
	virtual void	DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual void	OnOK() {}
	virtual void	OnCancel() {}
	afx_msg void	OnButtonsClicked(UINT nID);

	DECLARE_MESSAGE_MAP()
public:
	CToolTipCtrl	m_ToolTip;
	afx_msg void OnBnClickedMoveListButton();
};
