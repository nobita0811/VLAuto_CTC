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
#include "License.h"
#include <time.h>
#include <winhttp.h>
#pragma comment(lib, "winhttp.lib")

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
	// chức năng giành cho Admin / Tester: không hiện cho người dùng bình thường
#if ADMIN_MODE
	ON_BN_CLICKED(IDC_DEBUG_BUTTON, &CDlgAbout::OnBnClickedDebugButton)
#endif
	ON_BN_CLICKED(IDC_MOVE_MAP37_BUTTON, &CDlgAbout::OnBnClickedMoveMap37Button)
	ON_BN_CLICKED(IDC_TLS_TEST_BUTTON, &CDlgAbout::OnBnClickedTlsTestButton)
#if TEST_OFFLINE_BUILD
	ON_BN_CLICKED(IDC_SHOWTEXT_DEBUG_INGAME_CHECK, &CDlgAbout::OnBnClickedShowtextDebugIngameCheck)
#endif
END_MESSAGE_MAP()

// CDlgAbout message handlers

BOOL CDlgAbout::OnInitDialog()
{
	CDialog::OnInitDialog();

	CString strInfo;
	strInfo += _T("VLCTCPro - Phần mềm tự chơi Võ lâm bản CTC\n");
	strInfo += _T("Tác giả\t:\tLONG\n");
	SetDlgItemText(IDC_ABOUT_STATIC, strInfo);
	m_WebLink.SetURL(_T("http://autovolam.net"));

	// Thong tin License (lay tu token da ky):
	//   Dong 1: Goi + so account (quota; 0 = khong gioi han)
	//   Dong 2: Het han DD-MM-YYYY [con N ngay]
	{
		CString strLic;
		int nQuota = LicenseQuota();
		if (nQuota > 0)
			strLic.Format(_T("Gói: %s - %d account"), (LPCTSTR)LicensePlanName(), nQuota);
		else
			strLic.Format(_T("Gói: %s - không giới hạn account"), (LPCTSTR)LicensePlanName());
		unsigned long long exp = LicenseExpiry();
		if (exp == 0)
			strLic += _T("\nHết hạn: Vĩnh viễn");
		else
		{
			time_t tExp = (time_t)exp;
			struct tm tmExp = {};
			localtime_s(&tmExp, &tExp);
			long long days = ((long long)exp - (long long)time(NULL)) / 86400;
			if (days < 0) days = 0;
			CString strExp;
			strExp.Format(_T("\nHết hạn: %02d-%02d-%04d [còn %lld ngày]"),
				tmExp.tm_mday, tmExp.tm_mon + 1, tmExp.tm_year + 1900, days);
			strLic += strExp;
		}
		SetDlgItemText(IDC_LICENSE_INFO_STATIC, strLic);
	}

	memset(m_HardDriveID, 0, sizeof(m_HardDriveID));
	TCHAR HardDriveIDW[32] = _T("");
	GetHardDriveComputerID(m_HardDriveID);
	MultiByteToWideChar(CP_ACP, 0, m_HardDriveID, -1, HardDriveIDW, 32);
	SetDlgItemText(IDC_HDDCODE_EDIT, HardDriveIDW);

	m_UtilMenu.LoadMenu(IDR_UTIL_MENU);
	// tắt hiện nút Debug cho người dùng bình thường, chỉ hiện cho Admin / Tester
#if !ADMIN_MODE
	if (GetDlgItem(IDC_DEBUG_BUTTON)) GetDlgItem(IDC_DEBUG_BUTTON)->ShowWindow(SW_HIDE);	// Ban nguoi dung: khong co cua so DEBUG
	if (GetDlgItem(IDC_MOVE_MAP37_BUTTON)) GetDlgItem(IDC_MOVE_MAP37_BUTTON)->ShowWindow(SW_HIDE);	// Ẩn nút di chuyển Map 37 cho người dùng bình thường
#endif
	// Nut "Kiem tra TLS 1.2": chi hien tren ban Tester Offline, an o moi ban khac.
#if !TEST_OFFLINE_BUILD
	if (GetDlgItem(IDC_TLS_TEST_BUTTON)) GetDlgItem(IDC_TLS_TEST_BUTTON)->ShowWindow(SW_HIDE);
#endif
	// Checkbox "Hien text debug ingame": chi ban Tester Offline; an o moi ban khac.
#if TEST_OFFLINE_BUILD
	if (GetDlgItem(IDC_SHOWTEXT_DEBUG_INGAME_CHECK))
		((CButton*)GetDlgItem(IDC_SHOWTEXT_DEBUG_INGAME_CHECK))->SetCheck(
			GetPrivateProfileInt(_T("Options"), _T("ShowDbgTextIngame"), 0, g_szIniFile) ? BST_CHECKED : BST_UNCHECKED);
#else
	if (GetDlgItem(IDC_SHOWTEXT_DEBUG_INGAME_CHECK)) GetDlgItem(IDC_SHOWTEXT_DEBUG_INGAME_CHECK)->ShowWindow(SW_HIDE);
#endif

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

#if ADMIN_MODE
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
#endif

// Nhan vat dang chon: chay bo (MapPath) ve Bien Kinh roi mo ruong de luu ruong tai Bien Kinh (VLHook: CVLMain::StartWalkCityBox).
// Bam lai khi dang chay = huy.
void CDlgAbout::OnBnClickedMoveMap37Button()
{
	if (g_pCurGameWin == NULL)
	{
		MessageBox(_T("Chưa chọn nhân vật!"), 0, MB_ICONEXCLAMATION);
		return;
	}
	char *szPlayerName = g_pCurGameWin->m_szPlayerName;
	if (!szPlayerName[0] || strstr(szPlayerName, VLTK_ERROR) || !g_pCurGameWin->m_hWnd || !g_pCurGameWin->m_bAttached)
	{
		MessageBox(_T("Nhân vật chưa chạy Auto, không thể thực hiện chức năng này!"), 0, MB_ICONEXCLAMATION);
		return;
	}
	::PostMessage(g_pCurGameWin->m_hWnd, WM_HOOK_MAIN, cmd_walkcitybox, (LPARAM)city_bienkinh);
}

// Nut chan doan (chi ban Tester Offline): ep CHI TLS 1.2 va thu bat tay toi may chu
// license. Dung de kiem tra nhanh tren may ao Win7 xem co noi duoc TLS 1.2 khong.
void CDlgAbout::OnBnClickedTlsTestButton()
{
	const wchar_t* kHost = L"lprotect-activation.longit0811.workers.dev";
	const wchar_t* kPath = L"/activate";

#ifndef WINHTTP_OPTION_SECURE_PROTOCOLS
#define WINHTTP_OPTION_SECURE_PROTOCOLS 84
#endif
#ifndef WINHTTP_FLAG_SECURE_PROTOCOL_TLS1_2
#define WINHTTP_FLAG_SECURE_PROTOCOL_TLS1_2 0x00000800
#endif

	HINTERNET hS = WinHttpOpen(L"VLCTCPro-TLSTest/1.0", WINHTTP_ACCESS_TYPE_AUTOMATIC_PROXY,
		WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
	if (!hS)
	{
		MessageBox(_T("Không khởi tạo được WinHTTP."), _T("Kiểm tra TLS 1.2"), MB_ICONERROR);
		return;
	}
	WinHttpSetTimeouts(hS, 5000, 5000, 8000, 8000);
	DWORD secprot = WINHTTP_FLAG_SECURE_PROTOCOL_TLS1_2;	// CHI TLS 1.2 (dung muc may chu yeu cau)
	WinHttpSetOption(hS, WINHTTP_OPTION_SECURE_PROTOCOLS, &secprot, sizeof secprot);

	DWORD err = 0, status = 0;
	bool ok = false;

	HINTERNET hC = WinHttpConnect(hS, kHost, 443, 0);
	if (hC)
	{
		HINTERNET hR = WinHttpOpenRequest(hC, L"GET", kPath, NULL,
			WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, WINHTTP_FLAG_SECURE);
		if (hR)
		{
			if (WinHttpSendRequest(hR, WINHTTP_NO_ADDITIONAL_HEADERS, 0,
					WINHTTP_NO_REQUEST_DATA, 0, 0, 0)
				&& WinHttpReceiveResponse(hR, NULL))
			{
				DWORD scl = sizeof status;
				WinHttpQueryHeaders(hR, WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
					WINHTTP_HEADER_NAME_BY_INDEX, &status, &scl, WINHTTP_NO_HEADER_INDEX);
				ok = true;	// bat tay TLS 1.2 THANH CONG (co phan hoi HTTP, bat ke ma trang thai)
			}
			else err = GetLastError();
			WinHttpCloseHandle(hR);
		}
		else err = GetLastError();
		WinHttpCloseHandle(hC);
	}
	else err = GetLastError();
	WinHttpCloseHandle(hS);

	CString strMsg;
	if (ok)
	{
		strMsg.Format(_T("TLS 1.2: THÀNH CÔNG\n\n")
			_T("Đã bắt tay TLS 1.2 với máy chủ (HTTP %lu).\n")
			_T("Máy này hỗ trợ TLS 1.2 - bản User sẽ đăng nhập license được."), status);
		MessageBox(strMsg, _T("Kiểm tra TLS 1.2"), MB_ICONINFORMATION);
		return;
	}

	switch (err)
	{
	case ERROR_WINHTTP_SECURE_FAILURE:		// 12175
		strMsg = _T("TLS 1.2: THẤT BẠI\n\n")
			_T("Máy này KHÔNG bắt tay được TLS 1.2 với máy chủ.\n")
			_T("Win7 cần: SP1 + bản vá KB3140245 (bật TLS 1.2 cho WinHTTP), rồi khởi động lại.");
		break;
	case ERROR_WINHTTP_CANNOT_CONNECT:		// 12029
		strMsg = _T("Không kết nối được tới máy chủ (CANNOT_CONNECT).\nKiểm tra mạng / tường lửa.");
		break;
	case ERROR_WINHTTP_NAME_NOT_RESOLVED:	// 12007
		strMsg = _T("Không phân giải được tên miền máy chủ (DNS).\nKiểm tra mạng / DNS của máy ảo.");
		break;
	case ERROR_WINHTTP_TIMEOUT:				// 12002
		strMsg = _T("Hết thời gian chờ máy chủ (TIMEOUT).\nKiểm tra mạng.");
		break;
	default:
		strMsg.Format(_T("Kết nối thất bại. Mã lỗi WinHTTP: %lu.\n")
			_T("(12175 = lỗi TLS/SSL -> thiếu TLS 1.2)"), err);
		break;
	}
	MessageBox(strMsg, _T("Kiểm tra TLS 1.2"), MB_ICONWARNING);
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

#if TEST_OFFLINE_BUILD
void CDlgAbout::OnBnClickedShowtextDebugIngameCheck()
{
	BOOL bOn = ((CButton*)GetDlgItem(IDC_SHOWTEXT_DEBUG_INGAME_CHECK))->GetCheck() == BST_CHECKED;
	WritePrivateProfileString(_T("Options"), _T("ShowDbgTextIngame"), bOn ? _T("1") : _T("0"), g_szIniFile);
	// Gui lenh cho moi cua so game doc lai ini (VLHook: CVLMain::LoadOptions -> g_bDbgText)
	for (int i = 0; i < g_pCharList->GetItemCount(); i++)
	{
		GameWindow* pGameWin = (GameWindow*)g_pCharList->GetItemData(i);
		if (pGameWin->m_hWnd && pGameWin->m_bAttached)
			::PostMessage(pGameWin->m_hWnd, WM_HOOK_MAIN, cmd_loadini, 0);
	}
}
#endif

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
