// UpdateGameDataDlg.h : cua so chinh
#pragma once

#include "GameDiff.h"

#define WM_APP_PROGRESS		(WM_APP + 1)	// wParam = i, lParam = n
#define WM_APP_DONE			(WM_APP + 2)	// wParam = TRUE: thanh cong

class CUpdateGameDataDlg : public CDialogEx
{
public:
	CUpdateGameDataDlg(CWnd* pParent = NULL);

	enum { IDD = IDD_UPDATEGAMEDATA_DIALOG };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);
	virtual BOOL OnInitDialog();
	virtual void OnOK();
	virtual void OnCancel();

	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnGetMinMaxInfo(MINMAXINFO* lpMMI);
	afx_msg void OnBnClickedBtnRef();
	afx_msg void OnBnClickedBtnTgt();
	afx_msg void OnBnClickedBtnAddFile();
	afx_msg void OnBnClickedBtnDelFile();
	afx_msg void OnBnClickedBtnOut();
	afx_msg void OnBnClickedBtnRun();
	afx_msg void OnBnClickedBtnWrite();
	afx_msg void OnBnClickedBtnReport();
	afx_msg void OnBnClickedChkShowSame();
	afx_msg void OnNMCustomDrawListResult(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnNMDblclkListResult(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg LRESULT OnAppProgress(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnAppDone(WPARAM wParam, LPARAM lParam);
	DECLARE_MESSAGE_MAP()

private:
	static UINT ThreadProc(LPVOID pParam);
	void BrowseExe(UINT nID);
	void LoadSettings();
	void SaveSettings();
	void GetFileList(std::vector<CString> &files);
	void FillResult();
	void EnableControls(BOOL bEnable);
	CString ExeDir();
	CString IniPath() { return ExeDir() + _T("\\UpdateGameData.ini"); }

	CListBox		m_lstFiles;
	CListCtrl		m_lstResult;
	CProgressCtrl	m_Progress;

	CGameDiff		m_Diff;
	BOOL			m_bAnalyzed;
	CWinThread		*m_pThread;
	volatile LONG	m_lCancel;
	CString			m_strRef, m_strTgt, m_strThreadErr;
	std::vector<CString> m_ThreadFiles;

	CSize			m_szMin;		// Kich thuoc toi thieu cua cua so
	int				m_nListMarginR, m_nListMarginB, m_nStatusMarginR;
};
