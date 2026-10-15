#pragma once

#include "../GameDef.h"

#define WM_HOOK_DEBUG		WM_USER + 111

// VLAuto chạy Admin, Game chạy quyền thường: cho phép Game gửi uMsg lên cửa sổ hWnd (UIPI)
void AllowMessageFromGame(HWND hWnd, UINT uMsg);

// CDlgDebug dialog - Kiểm tra từng hàm game trong VLHook (cửa sổ không modal)

class CDlgDebug : public CDialog
{
	DECLARE_DYNAMIC(CDlgDebug)

public:
	CDlgDebug(CWnd* pParent = NULL);   // standard constructor
	virtual ~CDlgDebug();

// Dialog Data
	enum { IDD = IDD_DEBUG_DIALOG };

	struct DEBUG_BUTTON
	{
		int		nTab;
		int		nCmd;			// DEBUG_CMD
		LPCTSTR	szLabel;
		LPCTSTR	szTip;
		BOOL	bConfirm;		// Hỏi lại trước khi chạy
	};

protected:
	virtual BOOL	OnInitDialog();
	virtual BOOL	PreTranslateMessage(MSG* pMsg);
	virtual void	DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual void	OnOK() {}
	virtual void	OnCancel();

	afx_msg void	OnTcnSelchangeMainTab(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void	OnCmdButtonClicked(UINT nID);
	afx_msg void	OnBnClickedGetPos();
	afx_msg void	OnBnClickedGetFlag();
	afx_msg void	OnBnClickedClear();
	afx_msg void	OnBnClickedCheckHook();
	afx_msg BOOL	OnCopyData(CWnd* pWnd, COPYDATASTRUCT* pCopyDataStruct);
	afx_msg void	OnTimer(UINT_PTR nIDEvent);
	afx_msg void	OnDestroy();

	DECLARE_MESSAGE_MAP()

public:
	void			ShowDebug();

private:
	CTabCtrl		m_MainTab;
	CRichEditCtrl	m_Output;
	CToolTipCtrl	m_ToolTip;
	CFont			m_OutFont;
	enum { MAX_BUTTONS = 256 };
	HWND			m_hButtons[MAX_BUTTONS];
	int				m_nButtons;

	enum { MAX_PENDING = 16 };
	DEBUG_PARAM		m_Params[MAX_PENDING];	// Hook đọc bằng ReadProcessMemory sau khi PostMessage
	int				m_nSeq;
	int				m_nWaitSeq;				// Lệnh đang chờ phản hồi (0 = không)
	DWORD			m_dwWaitTick;
	int				m_nFillPosSeq;			// Phản hồi của lệnh này sẽ điền vào X/Y

	CRect			GetDlgRect(int x, int y, int cx, int cy);
	void			CreateButtons();
	void			ShowTabButtons();
	BOOL			SendDebugCmd(int nCmd, BOOL bFillPos = FALSE);
	void			RunLoginCmd(int nCmd);	// Tab Đăng nhập khi Game chưa có VLHook: tự đọc bộ nhớ + CreateRemoteThread
	void			LoginStatus(HANDLE hGame);
	void			ServerListFile(HANDLE hGame, CString strPath);	// Đọc + giải mã UserDataCl\serverlist.ini, xuất serverlist_.txt
	void			UpdateTarget();
	void			CheckHook();
	LPCTSTR			GetCmdLabel(int nCmd);
	void			AppendText(LPCTSTR szText, COLORREF crColor, BOOL bBold = FALSE);
	void			AppendLog(LPCTSTR szText, COLORREF crColor, BOOL bBold = FALSE);
};
