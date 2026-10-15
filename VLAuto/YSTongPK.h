#pragma once

// CYSTongPK dialog

class CYSTongPK : public CPropertyPage
{
	DECLARE_DYNAMIC(CYSTongPK)

public:
	CYSTongPK();
	virtual ~CYSTongPK();
	void			UdpdatePosition();
	void			UpdateQuestList(BOOL bSave);

// Dialog Data
	enum { IDD = IDD_TONGPK_DIALOG };

protected:
	virtual void	DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual BOOL	OnInitDialog();
	afx_msg void	OnButtonsClicked(UINT nID);
	void			AddHardQuest();
	void			DelHardQuest();

	DECLARE_MESSAGE_MAP()
public:
	CListCtrl		m_QuestList;
	YSTongPKOpt*	m_pTongPKOpt;
	int				m_nPointX;
	int				m_nPointY;
};
