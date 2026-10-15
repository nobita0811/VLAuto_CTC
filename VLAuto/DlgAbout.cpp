// CDlgAbout.cpp : implementation file
//

#include "stdafx.h"
#include "../GameDef.h"
#include "VLAuto.h"
#include "GameCore.h"
#include "GameWindow.h"
#include "DlgUtility.h"
#include "DlgFindItem.h"
#include "DlgOption.h"
#include "VLAutoDlg.h"
#include "DlgAbout.h"

// CDlgAbout dialog

IMPLEMENT_DYNAMIC(CDlgAbout, CDialog)

CDlgAbout::CDlgAbout(CWnd* pParent /*=NULL*/)
	: CDialog(CDlgAbout::IDD, pParent)
{
}

CDlgAbout::~CDlgAbout()
{
}

void CDlgAbout::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	// IDD_ABOUT_DIALOG hien khong co IDC_WEBSITE_STATIC: DDX_Control se nem CNotSupportedException
	if (GetDlgItem(IDC_WEBSITE_STATIC))
		DDX_Control(pDX, IDC_WEBSITE_STATIC, m_WebLink);

	if (g_pCurGameWin == NULL)
	{
		SetDlgItemText(IDC_CHARNAME_EDIT, _T(""));
		GetDlgItem(IDC_CCOPY_BUTTON)->EnableWindow(0);
		GetDlgItem(IDC_SAVE_BUTTON)->EnableWindow(0);
		GetDlgItem(IDC_LOAD_BUTTON)->EnableWindow(0);
		GetDlgItem(IDC_UTILITY_BUTTON)->EnableWindow(0);
		return;
	}

	char *szPlayerName = g_pCurGameWin->m_szPlayerName;
	BOOL bIsCharOnline = (szPlayerName[0] && !strstr(szPlayerName, VLTK_ERROR));
	if (bIsCharOnline)
	{
		TCHAR szCharName[32]=_T("");
		MultiByteToWideChar(CP_ACP, 0, szPlayerName, -1, szCharName, 32);
		SetDlgItemText(IDC_CHARNAME_EDIT, szCharName);
	}
	else
		SetDlgItemText(IDC_CHARNAME_EDIT, _T(""));

	GetDlgItem(IDC_CCOPY_BUTTON)->EnableWindow(bIsCharOnline);
	GetDlgItem(IDC_SAVE_BUTTON)->EnableWindow(bIsCharOnline);
	GetDlgItem(IDC_LOAD_BUTTON)->EnableWindow(bIsCharOnline);
	GetDlgItem(IDC_UTILITY_BUTTON)->EnableWindow(bIsCharOnline);
}

BEGIN_MESSAGE_MAP(CDlgAbout, CDialog)
	ON_BN_CLICKED(IDC_UTILITY_BUTTON, &CDlgAbout::OnBnClickedUtilityButton)
	ON_BN_CLICKED(IDC_OPTION_BUTTON, &CDlgAbout::OnBnClickedOptionButton)
	ON_BN_CLICKED(IDC_SAVE_BUTTON, &CDlgAbout::OnBnClickedSaveButton)
	ON_BN_CLICKED(IDC_LOAD_BUTTON, &CDlgAbout::OnBnClickedLoadButton)
	ON_BN_CLICKED(IDC_HCOPY_BUTTON, &CDlgAbout::OnBnClickedHcopyButton)
	ON_BN_CLICKED(IDC_CCOPY_BUTTON, &CDlgAbout::OnBnClickedCcopyButton)
	ON_BN_CLICKED(IDC_DEBUG_BUTTON, &CDlgAbout::OnBnClickedDebugButton)
END_MESSAGE_MAP()

// CDlgAbout message handlers

BOOL CDlgAbout::OnInitDialog()
{
	CDialog::OnInitDialog();

	CString strInfo;
	strInfo += _T("VLAuto - Phần mềm tự chơi VLTK\n");
	strInfo += _T("Tác giả\t:\tNguyễn Lưu Ngọc\n");
	SetDlgItemText(IDC_ABOUT_STATIC, strInfo);
	m_WebLink.SetURL(_T("http://vlauto.net"));

	memset(m_HardDriveID, 0, sizeof(m_HardDriveID));
	TCHAR HardDriveIDW[32] = _T("");
	GetHardDriveComputerID(m_HardDriveID);
	MultiByteToWideChar(CP_ACP, 0, m_HardDriveID, -1, HardDriveIDW, 32);
	SetDlgItemText(IDC_HDDCODE_EDIT, HardDriveIDW);

	m_UtilMenu.LoadMenu(IDR_UTIL_MENU);

	if (m_ToolTip.Create(this))
	{
		if (GetDlgItem(IDC_WEBSITE_STATIC))
			m_ToolTip.AddTool(GetDlgItem(IDC_WEBSITE_STATIC), _T("Trang chủ VLAuto"));
		m_ToolTip.AddTool(GetDlgItem(IDC_HDDCODE_EDIT), _T("Mã số ổ cứng"));
		m_ToolTip.AddTool(GetDlgItem(IDC_CHARNAME_EDIT), _T("Tên nhân vật"));
		m_ToolTip.AddTool(GetDlgItem(IDC_OPTION_BUTTON), _T("Tùy chọn mở rộng"));
	}

	return FALSE;
}

BOOL CDlgAbout::PreTranslateMessage(MSG* pMsg)
{
	m_ToolTip.RelayEvent(pMsg);
	return CDialog::PreTranslateMessage(pMsg);
}

BOOL CDlgAbout::OnCommand(WPARAM wParam, LPARAM lParam)
{
	CUtilityDlg dlgUtility;
	CFindItemDlg dlgFindItem;

	switch (wParam)
	{
	case ID_UTIL_GIVEKCL:
	case ID_UTIL_MOVEITEM:
	case ID_UTIL_HTINH:
		dlgUtility.DoModal();
		break;
	case ID_UTIL_FINDITEM:
		dlgFindItem.DoModal();
		break;
	}

	return CDialog::OnCommand(wParam, lParam);
}

void CDlgAbout::OnBnClickedUtilityButton()
{
	char *szPlayerName = g_pCurGameWin->m_szPlayerName;
	if (!szPlayerName[0] || strstr(szPlayerName, VLTK_ERROR) || !g_pCurGameWin->m_bAttached)
	{
		MessageBox(_T("Nhân vật chưa chạy Auto, không thể thực hiện chức năng này!"), 0, MB_ICONEXCLAMATION);
		return;
	}

	POINT pt;
	GetCursorPos(&pt);
	m_UtilMenu.GetSubMenu(0)->TrackPopupMenu(TPM_RIGHTALIGN|TPM_BOTTOMALIGN, pt.x, pt.y, this);
}

void CDlgAbout::OnBnClickedOptionButton()
{
	COptionDlg dlgOption;
	if (dlgOption.DoModal() != IDOK)
		return;

	for (int i = 0; i < g_pCharList->GetItemCount(); i++)
	{
		GameWindow* pGameWin = (GameWindow*)g_pCharList->GetItemData(i);
		if (!pGameWin->m_hWnd || !pGameWin->m_bAttached) continue;
		::PostMessage(pGameWin->m_hWnd, WM_HOOK_MAIN, cmd_loadini, 0);
	}
}

void CDlgAbout::OnBnClickedSaveButton()
{
	char *szPlayerName = g_pCurGameWin->m_szPlayerName;
	if (!szPlayerName[0] || strstr(szPlayerName, VLTK_ERROR))
	{
		MessageBox(_T("Nhân vật chưa đăng nhập, không thể thực hiện chức năng này!"), 0, MB_ICONEXCLAMATION);
		return;
	}
	CFileDialog fDlg(FALSE, _T(".cfg"), 0,
		OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT, _T("Configration File (*.cfg)|*.cfg|"), this);
	if (fDlg.DoModal() == IDCANCEL)
		return;
	CString szFileName = fDlg.GetPathName();
	CVLAutoDlg *pMainDlg = (CVLAutoDlg*)theApp.GetMainDlg();
	if (!g_pCurGameWin->SaveConfigFile(szFileName))
		MessageBox(_T("Lỗi: Không thể ghi file CFG! Xin hãy kiểm tra lại!"), 0, MB_ICONEXCLAMATION);
}

void CDlgAbout::OnBnClickedLoadButton()
{
	char *szPlayerName = g_pCurGameWin->m_szPlayerName;
	if (!szPlayerName[0] || strstr(szPlayerName, VLTK_ERROR))
	{
		MessageBox(_T("Nhân vật chưa đăng nhập, không thể thực hiện chức năng này!"), 0, MB_ICONEXCLAMATION);
		return;
	}
	CFileDialog fDlg(TRUE, _T(".cfg"), 0,
		OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT, _T("Item Attribute File (*.cfg)|*.cfg|"), this);
	if (fDlg.DoModal() == IDCANCEL)
		return;

	CString szFileName = fDlg.GetPathName();
	CVLAutoDlg *pMainDlg = (CVLAutoDlg*)theApp.GetMainDlg();
	if (!g_pCurGameWin->LoadConfigFile(szFileName))
		MessageBox(_T("Lỗi: Không thể đọc file CFG hoặc file CFG thuộc phiên bản cũ! Xin hãy kiểm tra lại!"), 0, MB_ICONEXCLAMATION);
}

void CDlgAbout::OnBnClickedHcopyButton()
{
	if (CopyToCliboard(m_HardDriveID))
	{
		MessageBox(_T("Mã ổ cứng đã được COPY vào Clipboard!"), 0, MB_ICONINFORMATION);
		SaveRegData();
	}
}

void CDlgAbout::OnBnClickedCcopyButton()
{
	char *szPlayerName = g_pCurGameWin->m_szPlayerName;
	if (!szPlayerName[0] || strstr(szPlayerName, VLTK_ERROR))
	{
		MessageBox(_T("Nhân vật chưa đăng nhập, không thể thực hiện chức năng này!"), 0, MB_ICONEXCLAMATION);
		return;
	}
	if (CopyToCliboard(g_pCurGameWin->m_szPlayerName))
	{
		MessageBox(_T("Tên nhân vật đã được COPY vào Clipboard!"), 0, MB_ICONINFORMATION);
		SaveRegData();
	}
}

void CDlgAbout::OnBnClickedDebugButton()
{
	if (!m_DlgDebug.GetSafeHwnd())
	{
		AfxInitRichEdit2();		// IDD_DEBUG_DIALOG co RichEdit20W
		if (!m_DlgDebug.Create(CDlgDebug::IDD, theApp.GetMainDlg()))
		{
			MessageBox(_T("Không tạo được cửa sổ DEBUG!"), 0, MB_ICONEXCLAMATION);
			return;
		}
	}
	m_DlgDebug.ShowDebug();
}

BOOL CDlgAbout::CopyToCliboard(char* szData)
{
	if (!OpenClipboard())
		return 0;
	EmptyClipboard();
	int nLen = (int)strlen(szData);
	HGLOBAL hCopy = GlobalAlloc(GMEM_MOVEABLE, nLen + 1);
	if (hCopy == NULL)
	{
		CloseClipboard();
		return 0;
	}
	char *szCopy = (char*)GlobalLock(hCopy); 
	strcpy(szCopy, szData);
	GlobalUnlock(hCopy);
	SetClipboardData(CF_TEXT, hCopy);
	CloseClipboard();
	return TRUE;
}

void CDlgAbout::SaveRegData()
{
	TCHAR szRegFile[MAX_PATH];
	_stprintf(szRegFile, _T("%s\\RegData.txt"), g_szAppFolder);

	FILE *pFile = _tfopen(szRegFile, _T("w"));
	if (pFile == NULL)
		return;

	fprintf(pFile, "M· HDD:\n%s\n\nNh©n vËt:\n", m_HardDriveID);
	for (int i = 0; i < g_pCharList->GetItemCount(); i++)
	{
		GameWindow* pGameWin = (GameWindow*)g_pCharList->GetItemData(i);
		if (!pGameWin->m_hWnd) continue;
		fprintf(pFile, "%s\n", pGameWin->m_szPlayerName);
	}

	fclose(pFile);
}
