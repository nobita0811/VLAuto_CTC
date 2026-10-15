#pragma once

// COptionDlg dialog

class COptionDlg : public CDialog
{
	DECLARE_DYNAMIC(COptionDlg)
public:
	COptionDlg(CWnd* pParent = NULL);   // standard constructor
	virtual ~COptionDlg();

// Dialog Data
	enum { IDD = IDD_OPTION_DIALOG };

protected:
	virtual void	DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual BOOL	OnInitDialog();
	virtual void	OnOK();
	DECLARE_MESSAGE_MAP()

public:
	BOOL			m_bSilentMode;
	BOOL			m_bAutoChat;
	BOOL			m_bPrintScreen;
	BOOL			m_bAttackBossFirst;
	CString			m_szControlPass;
};
