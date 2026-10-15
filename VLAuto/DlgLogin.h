#pragma once

#include "LoginSck.h"
#include "MiniBrowser.h"
#include "UiControls.h"

// CLoginDialog dialog

class CLoginDialog : public CDialog
{
	DECLARE_DYNAMIC(CLoginDialog)

public:
	CLoginDialog(CWnd* pParent = NULL);   // standard constructor
	virtual ~CLoginDialog();

// Dialog Data
	enum { IDD = IDD_LOGIN_DIALOG };

protected:
	HICON m_hIcon;
	BOOL m_bSigningOut;
	BOOL m_bUsePrimary;
	int m_nTimeCount;
	CButtonEx m_LoginBtn;

	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual BOOL OnInitDialog();
	virtual void OnOK();
	virtual void OnCancel();
	DECLARE_MESSAGE_MAP()

public:
	CLoginSck *m_pLoginSocket;
	CMiniBrowser theIEBrowser;

	afx_msg void OnTimer(UINT_PTR nIDEvent);
	afx_msg void OnBnClickedLoginButton();
	void GetEmailAddress(char *szEmailAddress);
	void OnErrorMessage(CString strErrorMsg, BOOL bAutoRetry = 0);
	void OnErrorMessage(UINT uMessageID, BOOL bAutoRetry = 0);
	void ProcessConnect();
	void PrepareData();
	void ProcessLogin(void *pGameData, int nSize);
	void ProcessSignOut(BOOL bSigningOut);

private:
	BOOL IsWow64();
	BOOL IsPEPacked();
};
