#pragma once

// CDlgParty dialog

class CDlgParty : public CDialog
{
	DECLARE_DYNAMIC(CDlgParty)

public:
	CDlgParty(CWnd* pParent = NULL);   // standard constructor
	virtual ~CDlgParty();
	void				UpdateNearByPlayers();
	void				UpdateMembersList(BOOL bSave);
	void				OnBnClickedAddButton();
	void				OnBnClickedDelButton();
	void				OnBnClickedUpButton();
	void				OnBnClickedDownButton();
	void				OnBnClickedDelAllButton();
	void				OnBnClickedSaveButton();
	void				OnBnClickedLoadButton();

// Dialog Data
	enum { IDD = IDD_PARTY_DIALOG };

protected:
	virtual BOOL		OnInitDialog();
	virtual BOOL		PreTranslateMessage(MSG* pMsg);
	virtual void		DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual void		OnOK() {}
	virtual void		OnCancel() {}
	afx_msg void		OnButtonsClicked(UINT nID);
	afx_msg void		OnCbnSelchangePtallCombo();
	afx_msg void		OnCbnSelchangeAcceptallCombo();

	DECLARE_MESSAGE_MAP()
public:
	CListBox			m_MemberList;
	CToolTipCtrl		m_ToolTip;
};
