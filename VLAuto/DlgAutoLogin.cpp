// DlgAutoLogin.cpp : implementation file
//

#include "stdafx.h"
#include "../GameDef.h"
#include "VLAuto.h"
#include "GameCore.h"
#include "GameWindow.h"
#include "DlgAutoLogin.h"
#include "GameLogin.h"
#include <shlwapi.h>
#include <psapi.h>
#include <wincrypt.h>
#include <algorithm>
#pragma comment(lib, "crypt32.lib")
#pragma comment(lib, "psapi.lib")

#define TIMER_AUTO_LOGIN		1
#define AUTO_LOGIN_INTERVAL		500		// ms: nhịp kiểm tra Game / đăng nhập
#define GAME_CLOSE_TIMEOUT		5000	// ms: gửi WM_CLOSE quá thời gian này mà Game chưa tắt thì TerminateProcess
#define REMOTE_TIMEOUT			3000	// ms: chờ thread chạy hàm giao diện trong Game
#define LOGIN_TIMEOUT			180000	// ms: một lượt đăng nhập (từ mở Game tới vào thế giới game)
#define LOGIN_MAX_TRIES			3		// Thao tác trên cùng một màn hình quá số lần này => kẹt
#define WAIT_LAUNCH				3000	// ms chờ sau mỗi thao tác: mặc định của các ô "Chờ mở game"... trong nhóm Thiết lập
#define WAIT_INIT				2000
#define WAIT_SERVER				4000
#define WAIT_LOGIN				6000
#define WAIT_ENTER_GAME			8000
#define MIN_WAIT_ACTION			500		// ms: tối thiểu của các ô thời gian chờ
#define TIMER_LOGIN_STATS		2
#define LOGIN_STATS_INTERVAL	2000	// ms: nhịp cập nhật bảng Thống kê
#define DEFAULT_TONG_KEEP		10		// vạn: mặc định số tiền giữ lại khi gom tiền lên bang
#define WAIT_ROLE_LIST			20000	// ms chờ danh sách nhân vật sau khi đăng nhập

#define AUTOLOGIN_DATA_FILE		_T("%s\\AutoLogin.ini")
#define SERVERLIST_FILE			_T("%s\\UiConfig\\serverlist_.txt")
#define MAX_ROLE_COUNT			3		// Số nhân vật tối đa trên một tài khoản
#define DEFAULT_RELOGIN_DELAY	10000	// ms
#define LOGIN_RETRY_DELAY		2000	// ms: đăng nhập thất bại thì chờ rồi thử lại
#define DEFAULT_RETRY_COUNT		3		// Số lần thử lại mặc định
#define DEFAULT_LOGINALL_DELAY	10000	// ms: mặc định của "Tự động login all sau"
#define MIN_LOGINALL_DELAY		2000	// ms: tối thiểu, để VLAuto kịp quét các Game đang chạy (tránh mở thêm Game cho tài khoản đã online)

std::vector<AutoLoginEntry> g_AutoLoginList;

// Ô thời gian chờ sau mỗi thao tác đăng nhập (nhóm Thiết lập), lưu ở [AutoLogin] trong AutoLogin.ini
struct WAIT_TIME_EDIT
{
	UINT	nID;
	LPCTSTR	szKey;
	DWORD	dwDefault;
};

static const WAIT_TIME_EDIT s_WaitEdits[] =
{
	{ IDC_WAIT_OPENGAME_LOGIN_EDIT,			_T("WaitOpenGame"),		WAIT_LAUNCH },		// scr_launch
	{ IDC_WAIT_JOINGAME_LOGIN_EDIT2,		_T("WaitStartGame"),	WAIT_INIT },		// scr_init
	{ IDC_WAIT_NOTICE_LOGIN_EDIT6,			_T("WaitNotice"),		WAIT_INIT },		// scr_notice (15_10: bảng Nhắc nhở)
	{ IDC_WAIT_CHOSESERVER_LOGIN_EDIT3,		_T("WaitSelectServer"),	WAIT_SERVER },		// scr_server
	{ IDC_WAIT_INPUT_ACCPASS_LOGIN_EDIT4,	_T("WaitLogin"),		WAIT_LOGIN },		// scr_login
	{ IDC_WAIT_CHOSEROLE_LOGIN_EDIT5,		_T("WaitEnterGame"),	WAIT_ENTER_GAME },	// scr_role
};

enum ACCOUNT_LIST_COLUMN
{
	col_index,
	col_account,
	col_status,
	col_region,
	col_server,
	col_role,
	col_relogin,
};

// Mật khẩu lưu dạng "P1:" + hex, mã hóa bằng khóa cố định => chép AutoLogin.ini sang máy khác vẫn đọc được.
// (Bản cũ dùng DPAPI: chỉ giải mã được trên đúng máy + tài khoản Windows đã lưu; vẫn đọc được để chuyển dần sang P1)
#define PASSWORD_PREFIX		_T("P1:")

static BYTE PasswordKeyByte(int i)
{
	static const char s_szKey[] = "VLAuto#AutoLogin@10_10";
	return (BYTE)(s_szKey[i % (sizeof(s_szKey) - 1)] ^ (i * 0x9D + 0x5B));
}

static CString EncryptPassword(const CString& strPassword)
{
	CString strHex;
	if (strPassword.IsEmpty())
		return strHex;
	strHex = PASSWORD_PREFIX;
	const BYTE* p = (const BYTE*)(LPCTSTR)strPassword;
	int nLen = strPassword.GetLength() * sizeof(TCHAR);
	for (int i = 0; i < nLen; i++)
		strHex.AppendFormat(_T("%02X"), p[i] ^ PasswordKeyByte(i));
	return strHex;
}

static CString DecryptPassword(CString strHex)
{
	CString strPassword;
	BOOL bPortable = strHex.Left(3) == PASSWORD_PREFIX;
	if (bPortable)
		strHex = strHex.Mid(3);
	int nLen = strHex.GetLength() / 2;
	if (nLen == 0)
		return strPassword;
	std::vector<BYTE> data(nLen);
	for (int i = 0; i < nLen; i++)
		data[i] = (BYTE)_tcstoul(strHex.Mid(i * 2, 2), NULL, 16);
	if (bPortable)
	{
		for (int i = 0; i < nLen; i++)
			data[i] ^= PasswordKeyByte(i);
		strPassword = CString((LPCTSTR)&data[0], nLen / sizeof(TCHAR));
		SecureZeroMemory(&data[0], nLen);
		return strPassword;
	}
	DATA_BLOB in = { (DWORD)nLen, &data[0] };
	DATA_BLOB out = { 0, NULL };
	if (CryptUnprotectData(&in, NULL, NULL, NULL, NULL, CRYPTPROTECT_UI_FORBIDDEN, &out))
	{
		strPassword = CString((LPCTSTR)out.pbData, out.cbData / sizeof(TCHAR));
		SecureZeroMemory(out.pbData, out.cbData);
		LocalFree(out.pbData);
	}
	return strPassword;
}

// Đọc UiConfig\serverlist_.txt (UTF-8, \settings\serverlist.ini trong pak của Game - Debug > Đăng nhập > Đọc danh sách server),
// tách thành "section|key" (chữ thường) -> value
static BOOL ReadServerListFile(LPCTSTR szFile, CMapStringToString& mapValues)
{
	HANDLE hFile = CreateFile(szFile, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);
	if (hFile == INVALID_HANDLE_VALUE)
		return FALSE;
	DWORD dwSize = GetFileSize(hFile, NULL), dwRead = 0;
	if (dwSize == INVALID_FILE_SIZE || dwSize > 4 * 1024 * 1024)
		dwSize = 0;
	CStringA strDataA;
	LPSTR pData = strDataA.GetBuffer(dwSize + 1);
	BOOL bRead = ReadFile(hFile, pData, dwSize, &dwRead, NULL);
	CloseHandle(hFile);
	if (!bRead)
		dwRead = 0;
	strDataA.ReleaseBuffer(dwRead);
	if (strDataA.Left(3) == "\xEF\xBB\xBF")
		strDataA.Delete(0, 3);

	CStringW strData;
	int nLen = MultiByteToWideChar(CP_UTF8, 0, strDataA, strDataA.GetLength(), NULL, 0);
	MultiByteToWideChar(CP_UTF8, 0, strDataA, strDataA.GetLength(), strData.GetBuffer(nLen + 1), nLen);
	strData.ReleaseBuffer(nLen);

	CString strSection;
	int nPos = 0;
	while (nPos < strData.GetLength())
	{
		int nEnd = strData.Find(L'\n', nPos);
		if (nEnd < 0)
			nEnd = strData.GetLength();
		CString strLine = strData.Mid(nPos, nEnd - nPos);
		nPos = nEnd + 1;
		strLine.Trim();
		if (strLine.IsEmpty() || strLine[0] == _T(';'))
			continue;
		if (strLine[0] == _T('['))
		{
			int nClose = strLine.Find(_T(']'));
			strSection = strLine.Mid(1, nClose > 0 ? nClose - 1 : strLine.GetLength() - 1);
			strSection.Trim();
			strSection.MakeLower();
			continue;
		}
		int nEqual = strLine.Find(_T('='));
		if (nEqual <= 0)
			continue;
		CString strKey = strLine.Left(nEqual), strValue = strLine.Mid(nEqual + 1);
		strKey.Trim();
		strKey.MakeLower();
		strValue.Trim();
		mapValues[strSection + _T("|") + strKey] = strValue;
	}
	return dwRead > 0;
}

static CString GetServerListValue(CMapStringToString& mapValues, LPCTSTR szSection, LPCTSTR szKey)
{
	CString strKey, strValue;
	strKey.Format(_T("%s|%s"), szSection, szKey);
	strKey.MakeLower();
	mapValues.Lookup(strKey, strValue);
	return strValue;
}

// Chọn dòng có chữ szText; chưa có (vd chưa đọc được serverlist) thì thêm vào
static void SelectComboText(CComboBox& combo, LPCTSTR szText)
{
	int nIndex = CB_ERR;
	if (szText[0])
	{
		nIndex = combo.FindStringExact(-1, szText);
		if (nIndex == CB_ERR)
			nIndex = combo.AddString(szText);
	}
	combo.SetCurSel(nIndex);
}

static CString GetComboText(CComboBox& combo)
{
	CString strText;
	int nIndex = combo.GetCurSel();
	if (nIndex != CB_ERR)
		combo.GetLBText(nIndex, strText);
	return strText;
}

// CDlgAutoLogin dialog

IMPLEMENT_DYNAMIC(CDlgAutoLogin, CDialog)

CDlgAutoLogin::CDlgAutoLogin(CWnd* pParent /*=NULL*/)
	: CDialog(CDlgAutoLogin::IDD, pParent), m_DlgStats(this), m_nTongKeep(DEFAULT_TONG_KEEP)
{
	m_chPasswordChar = 0;
	m_bUpdatingList = FALSE;
	m_nNextId = 0;
	m_Job.nId = 0;
	m_bRunAll = FALSE;
	m_nRound = 0;
	m_dwStartAllTick = 0;
}

CDlgAutoLogin::~CDlgAutoLogin()
{
}

void CDlgAutoLogin::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_ACCOUNT_INFO_ITEM_LIST, m_AccountList);
	DDX_Control(pDX, IDC_SERVER_REGION_COMBO, m_RegionCombo);
	DDX_Control(pDX, IDC_SERVER_NAME_COMBO, m_ServerCombo);
	DDX_Control(pDX, IDC_ROLE_INDEX_COMBO, m_RoleCombo);
	DDX_Control(pDX, IDC_PASSWORD_EDIT, m_PasswordEdit);
}

BEGIN_MESSAGE_MAP(CDlgAutoLogin, CDialog)
	ON_NOTIFY(LVN_ITEMCHANGED, IDC_ACCOUNT_INFO_ITEM_LIST, &CDlgAutoLogin::OnAccountListItemChanged)
	ON_NOTIFY(LVN_COLUMNCLICK, IDC_ACCOUNT_INFO_ITEM_LIST, &CDlgAutoLogin::OnAccountListColumnClick)
	ON_CBN_SELCHANGE(IDC_SERVER_REGION_COMBO, &CDlgAutoLogin::OnRegionComboSelChange)
	ON_BN_CLICKED(IDC_ADD_ITEMLIST_BUTTON, &CDlgAutoLogin::OnBnClickedAdd)
	ON_BN_CLICKED(IDC_REMOVE_ITEMLIST_BUTTON, &CDlgAutoLogin::OnBnClickedRemove)
	ON_BN_CLICKED(IDC_EDIT_ITEMLIST_BUTTON, &CDlgAutoLogin::OnBnClickedEdit)
	ON_BN_CLICKED(IDC_ITEMLIST_UP_BUTTON, &CDlgAutoLogin::OnBnClickedUp)
	ON_BN_CLICKED(IDC_ITEMLIST_DOWN_BUTTON, &CDlgAutoLogin::OnBnClickedDown)
	ON_BN_CLICKED(IDC_SHOW_PASSWORD_CHECK, &CDlgAutoLogin::OnBnClickedShowPassword)
	ON_BN_CLICKED(IDC_GET_GAMEPATH_BUTTON, &CDlgAutoLogin::OnBnClickedGetGamePath)
	ON_EN_KILLFOCUS(IDC_GAMEPATH_EDIT, &CDlgAutoLogin::OnGamePathKillFocus)
	ON_BN_CLICKED(IDC_OPENGAME_BUTTON, &CDlgAutoLogin::OnBnClickedOpenGame)
	ON_BN_CLICKED(IDC_TERMINAL_GAME_BUTTON, &CDlgAutoLogin::OnBnClickedTerminalGame)
	ON_BN_CLICKED(IDC_TERMINAL_GAMEALL_BUTTON, &CDlgAutoLogin::OnBnClickedTerminalGameAll)
	ON_BN_CLICKED(IDC_START_LOGIN_BUTTON, &CDlgAutoLogin::OnBnClickedStartLogin)
	ON_BN_CLICKED(IDC_START_LOGINALL_BUTTON, &CDlgAutoLogin::OnBnClickedStartLoginAll)
	ON_BN_CLICKED(IDC_STOP_LOGIN_BUTTON, &CDlgAutoLogin::OnBnClickedStopLogin)
	ON_BN_CLICKED(IDC_AUTO_RELOGIN_COUNT_CHECK, &CDlgAutoLogin::OnLoginOptionChanged)
	ON_EN_KILLFOCUS(IDC_COUNT_RELOGIN_EDIT, &CDlgAutoLogin::OnLoginOptionChanged)
	ON_BN_CLICKED(IDC_AUTO_RELOGIN_LOOP_CHECK, &CDlgAutoLogin::OnLoginOptionChanged)
	ON_BN_CLICKED(IDC_AUTO_LOGINALL_CHECK, &CDlgAutoLogin::OnLoginOptionChanged)
	ON_EN_KILLFOCUS(IDC_DELAYTIME_LOGINALL_EDIT, &CDlgAutoLogin::OnLoginOptionChanged)
	ON_BN_CLICKED(IDC_HIDEGAME_LOGIN_CHECK, &CDlgAutoLogin::OnLoginOptionChanged)
	ON_EN_KILLFOCUS(IDC_WAIT_OPENGAME_LOGIN_EDIT, &CDlgAutoLogin::OnLoginOptionChanged)
	ON_EN_KILLFOCUS(IDC_WAIT_JOINGAME_LOGIN_EDIT2, &CDlgAutoLogin::OnLoginOptionChanged)
	ON_EN_KILLFOCUS(IDC_WAIT_NOTICE_LOGIN_EDIT6, &CDlgAutoLogin::OnLoginOptionChanged)
	ON_EN_KILLFOCUS(IDC_WAIT_CHOSESERVER_LOGIN_EDIT3, &CDlgAutoLogin::OnLoginOptionChanged)
	ON_EN_KILLFOCUS(IDC_WAIT_INPUT_ACCPASS_LOGIN_EDIT4, &CDlgAutoLogin::OnLoginOptionChanged)
	ON_EN_KILLFOCUS(IDC_WAIT_CHOSEROLE_LOGIN_EDIT5, &CDlgAutoLogin::OnLoginOptionChanged)
	ON_BN_CLICKED(IDC_REPORT_LOGIN_BUTTON, &CDlgAutoLogin::OnBnClickedReport)
	ON_WM_TIMER()
END_MESSAGE_MAP()


// CDlgAutoLogin message handlers
BOOL CDlgAutoLogin::OnInitDialog()
{
	CDialog::OnInitDialog();

	m_strDataFile.Format(AUTOLOGIN_DATA_FILE, g_szAppFolder);
	m_chPasswordChar = m_PasswordEdit.GetPasswordChar();

	m_AccountList.SetExtendedStyle(m_AccountList.GetExtendedStyle() | LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES | LVS_EX_CHECKBOXES);
	m_AccountList.InsertColumn(col_index, _T("#"), LVCFMT_LEFT, 42);
	m_AccountList.InsertColumn(col_account, _T("Tài khoản"), LVCFMT_LEFT, 95);
	m_AccountList.InsertColumn(col_status, _T("Trạng thái"), LVCFMT_LEFT, 210);
	m_AccountList.InsertColumn(col_region, _T("Cụm server"), LVCFMT_LEFT, 80);
	m_AccountList.InsertColumn(col_server, _T("Server"), LVCFMT_LEFT, 95);
	m_AccountList.InsertColumn(col_role, _T("NV"), LVCFMT_LEFT, 30);
	m_AccountList.InsertColumn(col_relogin, _T("Login lại"), LVCFMT_LEFT, 58);

	CString strText;
	for (int i = 0; i < MAX_ROLE_COUNT; i++)
	{
		strText.Format(_T("%d"), i + 1);
		m_RoleCombo.AddString(strText);
	}
	m_RoleCombo.SetCurSel(0);
	SetDlgItemInt(IDC_DELAY_TIME_RELOGIN_EDIT, DEFAULT_RELOGIN_DELAY, FALSE);

	LoadAccounts();
	LoadServerList();
	for (int i = 0; i < (int)m_Accounts.size(); i++)
		InsertRow(i);
	if (!m_Accounts.empty())
		SelectRow(0);

	UpdateGameStatus();
	SetTimer(TIMER_AUTO_LOGIN, AUTO_LOGIN_INTERVAL, NULL);

	if (m_ToolTip.Create(this))
	{
		m_ToolTip.AddTool(GetDlgItem(IDC_ACCOUNT_INFO_ITEM_LIST), _T("Danh sách tài khoản - chọn một dòng để xem/sửa, check ô ở cột # để chạy LOGIN ALL (bấm tiêu đề # để check/bỏ tất cả)"));
		m_ToolTip.AddTool(GetDlgItem(IDC_ADD_ITEMLIST_BUTTON), _T("Thêm tài khoản đang nhập vào cuối danh sách"));
		m_ToolTip.AddTool(GetDlgItem(IDC_REMOVE_ITEMLIST_BUTTON), _T("Xóa tài khoản đang chọn"));
		m_ToolTip.AddTool(GetDlgItem(IDC_EDIT_ITEMLIST_BUTTON), _T("Ghi thông tin đang nhập vào tài khoản đang chọn"));
		m_ToolTip.AddTool(GetDlgItem(IDC_ITEMLIST_UP_BUTTON), _T("Đưa tài khoản đang chọn lên trên"));
		m_ToolTip.AddTool(GetDlgItem(IDC_ITEMLIST_DOWN_BUTTON), _T("Đưa tài khoản đang chọn xuống dưới"));
		m_ToolTip.AddTool(GetDlgItem(IDC_AUTO_RELOGIN_CHECK), _T("Riêng tài khoản này: sau LOGIN / LOGIN ALL, chưa vào game (mất kết nối, Game tắt...) thì chờ số ms bên cạnh rồi đăng nhập lại"));
		m_ToolTip.AddTool(GetDlgItem(IDC_DELAY_TIME_RELOGIN_EDIT), _T("Thời gian chờ (ms) trước khi đăng nhập lại"));
		m_ToolTip.AddTool(GetDlgItem(IDC_GET_GAMEPATH_BUTTON), _T("Chọn file chạy của Game"));
		m_ToolTip.AddTool(GetDlgItem(IDC_START_LOGIN_BUTTON), _T("Đăng nhập tài khoản đang chọn"));
		m_ToolTip.AddTool(GetDlgItem(IDC_START_LOGINALL_BUTTON), _T("Lần lượt đăng nhập các tài khoản được check"));
		m_ToolTip.AddTool(GetDlgItem(IDC_STOP_LOGIN_BUTTON), _T("Dừng đăng nhập và đăng nhập lại (không tắt Game)"));
		m_ToolTip.AddTool(GetDlgItem(IDC_OPENGAME_BUTTON), _T("Mở một Game mới cho tài khoản đang chọn"));
		m_ToolTip.AddTool(GetDlgItem(IDC_TERMINAL_GAME_BUTTON), _T("Thoát Game của tài khoản đang chọn (dừng đăng nhập tài khoản này)"));
		m_ToolTip.AddTool(GetDlgItem(IDC_TERMINAL_GAMEALL_BUTTON), _T("Dừng đăng nhập và thoát tất cả Game đang chạy"));
		m_ToolTip.AddTool(GetDlgItem(IDC_SERVER_REGION_COMBO), _T("Đọc từ UiConfig\\serverlist_.txt"));
		m_ToolTip.AddTool(GetDlgItem(IDC_AUTO_RELOGIN_COUNT_CHECK), _T("Đăng nhập thất bại thì chờ 2s rồi thử lại, tối đa số lần bên cạnh (bỏ check: không thử lại)"));
		m_ToolTip.AddTool(GetDlgItem(IDC_COUNT_RELOGIN_EDIT), _T("Số lần thử lại khi đăng nhập thất bại"));
		m_ToolTip.AddTool(GetDlgItem(IDC_HIDEGAME_LOGIN_CHECK), _T("Auto Login đăng nhập vào game xong thì ẩn cửa sổ Game (bấm \"Mở game\" để hiện lại)"));
		m_ToolTip.AddTool(GetDlgItem(IDC_AUTO_LOGINALL_CHECK), _T("Mở Auto xong chờ số ms bên cạnh rồi tự LOGIN ALL (chỉ một lần mỗi lần mở Auto)"));
		m_ToolTip.AddTool(GetDlgItem(IDC_DELAYTIME_LOGINALL_EDIT), _T("Thời gian chờ (ms) sau khi mở Auto, tối thiểu 2000"));
		m_ToolTip.AddTool(GetDlgItem(IDC_WAIT_OPENGAME_LOGIN_EDIT), _T("ms chờ sau khi mở Game rồi mới xem Game đang ở màn hình nào (mặc định 3000, tối thiểu 500)"));
		m_ToolTip.AddTool(GetDlgItem(IDC_WAIT_JOINGAME_LOGIN_EDIT2), _T("ms chờ sau khi bấm Bắt Đầu Trò Chơi (mặc định 2000, tối thiểu 500)"));
		m_ToolTip.AddTool(GetDlgItem(IDC_WAIT_NOTICE_LOGIN_EDIT6), _T("ms chờ sau khi bấm Xác định ở bảng Nhắc nhở (15_10, mặc định 2000, tối thiểu 500)"));
		m_ToolTip.AddTool(GetDlgItem(IDC_WAIT_CHOSESERVER_LOGIN_EDIT3), _T("ms chờ sau khi chọn cụm + server và bấm Đăng nhập (mặc định 4000, tối thiểu 500).\nChờ ngắn quá mà máy chủ chưa kịp chuyển màn hình thì Auto bấm lại; quá 3 lần sẽ báo lỗi"));
		m_ToolTip.AddTool(GetDlgItem(IDC_WAIT_INPUT_ACCPASS_LOGIN_EDIT4), _T("ms chờ sau khi nhập tài khoản + mật khẩu và bấm Đăng nhập (mặc định 6000, tối thiểu 500).\nChờ ngắn quá mà máy chủ chưa kịp trả lời thì Auto đăng nhập lại; quá 3 lần sẽ báo lỗi"));
		m_ToolTip.AddTool(GetDlgItem(IDC_WAIT_CHOSEROLE_LOGIN_EDIT5), _T("ms chờ sau khi chọn nhân vật và vào game (mặc định 8000, tối thiểu 500)"));
		m_ToolTip.AddTool(GetDlgItem(IDC_REPORT_LOGIN_BUTTON), _T("Bảng thống kê tiền, số lần dùng Thổ Địa Phù, số lần chết của các nhân vật trong danh sách"));
		m_ToolTip.SetMaxTipWidth(400);
		m_ToolTip.AddTool(GetDlgItem(IDC_AUTO_RELOGIN_LOOP_CHECK), _T("LOGIN ALL xong danh sách thì lấy lại các tài khoản được check để login tiếp (tài khoản có \"Tự động đăng nhập lại\" bị văng game sẽ được login lại sau thời gian chờ của nó)"));
	}

	// Mở Auto: chờ "Tự động login all sau" rồi tự LOGIN ALL (một lần)
	if (IsDlgButtonChecked(IDC_AUTO_LOGINALL_CHECK) == BST_CHECKED)
	{
		m_dwStartAllTick = GetTickCount() + max(GetDlgItemInt(IDC_DELAYTIME_LOGINALL_EDIT, NULL, FALSE), (UINT)MIN_LOGINALL_DELAY);
		if (m_dwStartAllTick == 0)
			m_dwStartAllTick = 1;
	}

	return TRUE;
}

BOOL CDlgAutoLogin::PreTranslateMessage(MSG* pMsg)
{
	if (m_ToolTip.m_hWnd)
		m_ToolTip.RelayEvent(pMsg);
	return CDialog::PreTranslateMessage(pMsg);
}

void CDlgAutoLogin::OnCancel()
{
	ShowWindow(SW_HIDE);	// Giữ lại dữ liệu đã nhập, bấm Login để mở lại; đăng nhập vẫn chạy tiếp
}

void CDlgAutoLogin::ShowAutoLogin()
{
	LoadServerList();	// serverlist_.txt có thể vừa được cập nhật
	UpdateGameStatus();
	ShowWindow(SW_SHOW);
	SetForegroundWindow();
}

// Đọc AutoLogin.ini: Game Path + danh sách tài khoản
void CDlgAutoLogin::LoadAccounts()
{
	TCHAR szValue[2048];
	GetPrivateProfileString(_T("AutoLogin"), _T("GamePath"), _T(""), szValue, 2048, m_strDataFile);
	m_strGamePath = szValue;
	SetDlgItemText(IDC_GAMEPATH_EDIT, m_strGamePath);
	CheckDlgButton(IDC_AUTO_RELOGIN_COUNT_CHECK, GetPrivateProfileInt(_T("AutoLogin"), _T("RetryEnable"), 1, m_strDataFile) ? BST_CHECKED : BST_UNCHECKED);
	SetDlgItemInt(IDC_COUNT_RELOGIN_EDIT, GetPrivateProfileInt(_T("AutoLogin"), _T("RetryCount"), DEFAULT_RETRY_COUNT, m_strDataFile), FALSE);
	CheckDlgButton(IDC_AUTO_RELOGIN_LOOP_CHECK, GetPrivateProfileInt(_T("AutoLogin"), _T("Loop"), 0, m_strDataFile) ? BST_CHECKED : BST_UNCHECKED);
	CheckDlgButton(IDC_AUTO_LOGINALL_CHECK, GetPrivateProfileInt(_T("AutoLogin"), _T("AutoLoginAll"), 0, m_strDataFile) ? BST_CHECKED : BST_UNCHECKED);
	SetDlgItemInt(IDC_DELAYTIME_LOGINALL_EDIT, GetPrivateProfileInt(_T("AutoLogin"), _T("AutoLoginAllDelay"), DEFAULT_LOGINALL_DELAY, m_strDataFile), FALSE);
	CheckDlgButton(IDC_HIDEGAME_LOGIN_CHECK, GetPrivateProfileInt(_T("AutoLogin"), _T("HideGame"), 0, m_strDataFile) ? BST_CHECKED : BST_UNCHECKED);
	m_nTongKeep = (int)GetPrivateProfileInt(_T("AutoLogin"), _T("TongKeep"), DEFAULT_TONG_KEEP, m_strDataFile);
	for (int i = 0; i < _countof(s_WaitEdits); i++)
	{
		UINT nWait = GetPrivateProfileInt(_T("AutoLogin"), s_WaitEdits[i].szKey, s_WaitEdits[i].dwDefault, m_strDataFile);
		SetDlgItemInt(s_WaitEdits[i].nID, max(nWait, (UINT)MIN_WAIT_ACTION), FALSE);
	}

	m_Accounts.clear();
	BOOL bOldPassword = FALSE;
	int nCount = GetPrivateProfileInt(_T("AutoLogin"), _T("Count"), 0, m_strDataFile);
	for (int i = 0; i < nCount; i++)
	{
		CString strSection;
		strSection.Format(_T("Account_%d"), i);
		AutoLoginAccount acc;
		GetPrivateProfileString(strSection, _T("Account"), _T(""), szValue, 2048, m_strDataFile);
		acc.strAccount = szValue;
		if (acc.strAccount.IsEmpty())
			continue;
		GetPrivateProfileString(strSection, _T("Password"), _T(""), szValue, 2048, m_strDataFile);
		acc.strPassword = DecryptPassword(szValue);
		if (!acc.strPassword.IsEmpty() && _tcsncmp(szValue, PASSWORD_PREFIX, 3) != 0)
			bOldPassword = TRUE;
		GetPrivateProfileString(strSection, _T("Region"), _T(""), szValue, 2048, m_strDataFile);
		acc.strRegion = szValue;
		GetPrivateProfileString(strSection, _T("Server"), _T(""), szValue, 2048, m_strDataFile);
		acc.strServer = szValue;
		acc.nRoleIndex = GetPrivateProfileInt(strSection, _T("Role"), 0, m_strDataFile);
		if (acc.nRoleIndex < 0 || acc.nRoleIndex >= MAX_ROLE_COUNT)
			acc.nRoleIndex = 0;
		acc.bChecked = GetPrivateProfileInt(strSection, _T("Checked"), 1, m_strDataFile) != 0;
		acc.bAutoRelogin = GetPrivateProfileInt(strSection, _T("AutoRelogin"), 0, m_strDataFile) != 0;
		acc.nReloginDelay = (int)GetPrivateProfileInt(strSection, _T("ReloginDelay"), DEFAULT_RELOGIN_DELAY, m_strDataFile);
		GetPrivateProfileString(strSection, _T("PlayerName"), _T(""), szValue, 2048, m_strDataFile);
		acc.strPlayerName = szValue;
		GetPrivateProfileString(strSection, _T("PlayerCfg"), _T(""), szValue, 2048, m_strDataFile);
		acc.strPlayerCfg = szValue;
		acc.bTongGather = GetPrivateProfileInt(strSection, _T("TongGather"), 0, m_strDataFile) != 0;
		acc.nId = ++m_nNextId;
		m_Accounts.push_back(acc);
	}
	if (bOldPassword)
		SaveAccounts();	// Chuyển mật khẩu DPAPI cũ sang dạng P1 (chép sang máy khác vẫn đọc được)
}

// Ghi lại toàn bộ AutoLogin.ini (UTF-16 để giữ tên server tiếng Việt)
void CDlgAutoLogin::SaveAccounts()
{
	CString strGamePath;
	GetDlgItemText(IDC_GAMEPATH_EDIT, strGamePath);
	strGamePath.Trim();

	CString strData;
	strData.Format(_T("[AutoLogin]\r\nGamePath=%s\r\nRetryEnable=%d\r\nRetryCount=%d\r\nLoop=%d\r\nAutoLoginAll=%d\r\nAutoLoginAllDelay=%d\r\nHideGame=%d\r\nCount=%d\r\n"), (LPCTSTR)strGamePath,
		IsDlgButtonChecked(IDC_AUTO_RELOGIN_COUNT_CHECK) == BST_CHECKED, (int)GetDlgItemInt(IDC_COUNT_RELOGIN_EDIT, NULL, FALSE),
		IsDlgButtonChecked(IDC_AUTO_RELOGIN_LOOP_CHECK) == BST_CHECKED,
		IsDlgButtonChecked(IDC_AUTO_LOGINALL_CHECK) == BST_CHECKED, (int)GetDlgItemInt(IDC_DELAYTIME_LOGINALL_EDIT, NULL, FALSE),
		IsDlgButtonChecked(IDC_HIDEGAME_LOGIN_CHECK) == BST_CHECKED,
		(int)m_Accounts.size());
	for (int i = 0; i < _countof(s_WaitEdits); i++)
		strData.AppendFormat(_T("%s=%d\r\n"), s_WaitEdits[i].szKey, (int)GetWaitTime(i + 1));
	strData.AppendFormat(_T("TongKeep=%d\r\n"), m_nTongKeep);
	for (int i = 0; i < (int)m_Accounts.size(); i++)
	{
		const AutoLoginAccount& acc = m_Accounts[i];
		strData.AppendFormat(_T("\r\n[Account_%d]\r\nAccount=%s\r\nPassword=%s\r\nRegion=%s\r\nServer=%s\r\nRole=%d\r\nChecked=%d\r\nAutoRelogin=%d\r\nReloginDelay=%d\r\n"),
			i, (LPCTSTR)acc.strAccount, (LPCTSTR)EncryptPassword(acc.strPassword),
			(LPCTSTR)acc.strRegion, (LPCTSTR)acc.strServer, acc.nRoleIndex, acc.bChecked, acc.bAutoRelogin, acc.nReloginDelay);
		if (!acc.strPlayerCfg.IsEmpty())
			strData.AppendFormat(_T("PlayerName=%s\r\nPlayerCfg=%s\r\n"), (LPCTSTR)acc.strPlayerName, (LPCTSTR)acc.strPlayerCfg);
		if (acc.bTongGather)
			strData.Append(_T("TongGather=1\r\n"));
	}

	HANDLE hFile = CreateFile(m_strDataFile, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hFile == INVALID_HANDLE_VALUE)
		return;
	DWORD dwWritten = 0;
	WCHAR wBOM = 0xFEFF;
	WriteFile(hFile, &wBOM, sizeof(wBOM), &dwWritten, NULL);
	WriteFile(hFile, (LPCTSTR)strData, strData.GetLength() * sizeof(TCHAR), &dwWritten, NULL);
	CloseHandle(hFile);
}

// Đọc danh sách cụm/server từ UiConfig\serverlist_.txt (cập nhật tay mỗi khi Game đổi danh sách)
void CDlgAutoLogin::LoadServerList()
{
	CString strRegion = GetComboText(m_RegionCombo), strServer = GetComboText(m_ServerCombo);
	m_Regions.clear();
	CString strFile;
	strFile.Format(SERVERLIST_FILE, g_szAppFolder);
	CMapStringToString mapValues;
	if (ReadServerListFile(strFile, mapValues))
	{
		int nRegionCount = _ttoi(GetServerListValue(mapValues, _T("List"), _T("RegionCount")));
		for (int i = 0; i < nRegionCount; i++)
		{
			CString strKey, strSection;
			strKey.Format(_T("Region_%d"), i);
			AutoLoginRegion region;
			region.strTitle = GetServerListValue(mapValues, _T("List"), strKey);
			if (region.strTitle.IsEmpty())
				continue;
			strSection.Format(_T("Region_%d"), i);
			int nServerCount = _ttoi(GetServerListValue(mapValues, strSection, _T("Count")));
			for (int j = 0; j < nServerCount; j++)
			{
				strKey.Format(_T("%d_Title"), j);
				CString strTitle = GetServerListValue(mapValues, strSection, strKey);
				if (strTitle.IsEmpty())
					continue;
				strKey.Format(_T("%d_Address"), j);
				region.Servers.push_back(strTitle);
				region.Addresses.push_back(GetServerListValue(mapValues, strSection, strKey));
			}
			m_Regions.push_back(region);
		}
	}

	m_RegionCombo.ResetContent();
	for (int i = 0; i < (int)m_Regions.size(); i++)
		m_RegionCombo.AddString(m_Regions[i].strTitle);
	SelectComboText(m_RegionCombo, strRegion);
	FillServerCombo();
	SelectComboText(m_ServerCombo, strServer);
}

// Đổ server của cụm đang chọn vào combo Server
void CDlgAutoLogin::FillServerCombo()
{
	CString strRegion = GetComboText(m_RegionCombo);
	m_ServerCombo.ResetContent();
	for (int i = 0; i < (int)m_Regions.size(); i++)
	{
		if (m_Regions[i].strTitle != strRegion)
			continue;
		for (int j = 0; j < (int)m_Regions[i].Servers.size(); j++)
			m_ServerCombo.AddString(m_Regions[i].Servers[j]);
		break;
	}
	if (m_ServerCombo.GetCount() > 0)
		m_ServerCombo.SetCurSel(0);
}

void CDlgAutoLogin::InsertRow(int nIndex)
{
	m_bUpdatingList = TRUE;
	m_AccountList.InsertItem(nIndex, _T(""));
	m_bUpdatingList = FALSE;
	UpdateRow(nIndex);
}

void CDlgAutoLogin::UpdateRow(int nIndex)
{
	const AutoLoginAccount& acc = m_Accounts[nIndex];
	CString strText;
	strText.Format(_T("%d"), nIndex + 1);
	m_AccountList.SetItemText(nIndex, col_index, strText);
	m_AccountList.SetItemText(nIndex, col_account, acc.strAccount);
	m_AccountList.SetItemText(nIndex, col_region, acc.strRegion);
	m_AccountList.SetItemText(nIndex, col_server, acc.strServer);
	strText.Format(_T("%d"), acc.nRoleIndex + 1);
	m_AccountList.SetItemText(nIndex, col_role, strText);
	if (acc.bAutoRelogin)
		strText.Format(_T("%d ms"), acc.nReloginDelay);
	else
		strText.Empty();
	m_AccountList.SetItemText(nIndex, col_relogin, strText);
	m_AccountList.SetItemText(nIndex, col_status, GetStatusText(nIndex));
	m_bUpdatingList = TRUE;
	m_AccountList.SetCheck(nIndex, acc.bChecked);
	m_bUpdatingList = FALSE;
}

void CDlgAutoLogin::SelectRow(int nIndex)
{
	if (nIndex < 0 || nIndex >= m_AccountList.GetItemCount())
		return;
	m_AccountList.SetItemState(nIndex, LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED);
	m_AccountList.EnsureVisible(nIndex, FALSE);
}

int CDlgAutoLogin::GetSelectedRow()
{
	return m_AccountList.GetNextItem(-1, LVNI_SELECTED);
}

// Tìm tài khoản trùng (cùng tên tài khoản + cùng server), bỏ qua dòng nExcept
int CDlgAutoLogin::FindAccount(const AutoLoginAccount& acc, int nExcept)
{
	for (int i = 0; i < (int)m_Accounts.size(); i++)
	{
		if (i != nExcept && m_Accounts[i].strAccount.CompareNoCase(acc.strAccount) == 0
			&& m_Accounts[i].strRegion == acc.strRegion && m_Accounts[i].strServer == acc.strServer)
			return i;
	}
	return -1;
}

int CDlgAutoLogin::FindAccountById(int nId)
{
	for (int i = 0; nId && i < (int)m_Accounts.size(); i++)
		if (m_Accounts[i].nId == nId)
			return i;
	return -1;
}

// Lấy thông tin đang nhập ở nhóm Tài khoản, thiếu thì báo và trả về FALSE
BOOL CDlgAutoLogin::GetFields(AutoLoginAccount& acc)
{
	GetDlgItemText(IDC_ACCOUNT_EDIT, acc.strAccount);
	acc.strAccount.Trim();
	m_PasswordEdit.GetWindowText(acc.strPassword);
	acc.strRegion = GetComboText(m_RegionCombo);
	acc.strServer = GetComboText(m_ServerCombo);
	acc.nRoleIndex = max(m_RoleCombo.GetCurSel(), 0);
	acc.bAutoRelogin = IsDlgButtonChecked(IDC_AUTO_RELOGIN_CHECK) == BST_CHECKED;
	acc.nReloginDelay = (int)GetDlgItemInt(IDC_DELAY_TIME_RELOGIN_EDIT, NULL, FALSE);

	UINT nFocusID = 0;
	LPCTSTR szError = NULL;
	if (acc.strAccount.IsEmpty())
		nFocusID = IDC_ACCOUNT_EDIT, szError = _T("Chưa nhập tài khoản");
	else if (acc.strPassword.IsEmpty())
		nFocusID = IDC_PASSWORD_EDIT, szError = _T("Chưa nhập mật khẩu");
	else if (acc.strRegion.IsEmpty())
		nFocusID = IDC_SERVER_REGION_COMBO, szError = _T("Chưa chọn cụm server (kiểm tra lại UiConfig\\serverlist_.txt)");
	else if (acc.strServer.IsEmpty())
		nFocusID = IDC_SERVER_NAME_COMBO, szError = _T("Chưa chọn server");
	if (szError)
	{
		MessageBox(szError, _T("Auto Login"), MB_OK | MB_ICONWARNING);
		GetDlgItem(nFocusID)->SetFocus();
		return FALSE;
	}
	return TRUE;
}

void CDlgAutoLogin::SetFields(const AutoLoginAccount& acc)
{
	SetDlgItemText(IDC_ACCOUNT_EDIT, acc.strAccount);
	m_PasswordEdit.SetWindowText(acc.strPassword);
	SelectComboText(m_RegionCombo, acc.strRegion);
	FillServerCombo();
	SelectComboText(m_ServerCombo, acc.strServer);
	m_RoleCombo.SetCurSel(acc.nRoleIndex);
	CheckDlgButton(IDC_AUTO_RELOGIN_CHECK, acc.bAutoRelogin ? BST_CHECKED : BST_UNCHECKED);
	SetDlgItemInt(IDC_DELAY_TIME_RELOGIN_EDIT, acc.nReloginDelay, FALSE);
}

void CDlgAutoLogin::MoveRow(int nFrom, int nTo)
{
	if (nFrom < 0 || nTo < 0 || nTo >= (int)m_Accounts.size())
		return;
	std::swap(m_Accounts[nFrom], m_Accounts[nTo]);
	UpdateRow(nFrom);
	UpdateRow(nTo);
	SelectRow(nTo);
	SaveAccounts();
}

void CDlgAutoLogin::OnAccountListItemChanged(NMHDR* pNMHDR, LRESULT* pResult)
{
	LPNMLISTVIEW pNMLV = reinterpret_cast<LPNMLISTVIEW>(pNMHDR);
	if ((pNMLV->uChanged & LVIF_STATE) && (pNMLV->uNewState & LVIS_SELECTED) && !(pNMLV->uOldState & LVIS_SELECTED)
		&& pNMLV->iItem >= 0 && pNMLV->iItem < (int)m_Accounts.size())
		SetFields(m_Accounts[pNMLV->iItem]);
	// Bấm ô check ở cột #
	if (!m_bUpdatingList && (pNMLV->uChanged & LVIF_STATE) && ((pNMLV->uNewState ^ pNMLV->uOldState) & LVIS_STATEIMAGEMASK)
		&& pNMLV->iItem >= 0 && pNMLV->iItem < (int)m_Accounts.size())
	{
		BOOL bChecked = (pNMLV->uNewState & LVIS_STATEIMAGEMASK) == INDEXTOSTATEIMAGEMASK(2);
		if (m_Accounts[pNMLV->iItem].bChecked != bChecked)
		{
			m_Accounts[pNMLV->iItem].bChecked = bChecked;
			SaveAccounts();
		}
	}
	*pResult = 0;
}

// Bấm tiêu đề cột #: còn tài khoản chưa check thì check tất cả, ngược lại bỏ check tất cả
void CDlgAutoLogin::OnAccountListColumnClick(NMHDR* pNMHDR, LRESULT* pResult)
{
	LPNMLISTVIEW pNMLV = reinterpret_cast<LPNMLISTVIEW>(pNMHDR);
	*pResult = 0;
	if (pNMLV->iSubItem != col_index || m_Accounts.empty())
		return;
	BOOL bCheckAll = FALSE;
	for (int i = 0; i < (int)m_Accounts.size(); i++)
		if (!m_Accounts[i].bChecked)
			bCheckAll = TRUE;
	for (int i = 0; i < (int)m_Accounts.size(); i++)
	{
		m_Accounts[i].bChecked = bCheckAll;
		UpdateRow(i);
	}
	SaveAccounts();
}

void CDlgAutoLogin::OnRegionComboSelChange()
{
	FillServerCombo();
}

void CDlgAutoLogin::OnBnClickedAdd()
{
	AutoLoginAccount acc;
	if (!GetFields(acc))
		return;
	int nSame = FindAccount(acc, -1);
	if (nSame >= 0)
	{
		MessageBox(_T("Tài khoản này đã có trong danh sách"), _T("Auto Login"), MB_OK | MB_ICONWARNING);
		SelectRow(nSame);
		return;
	}
	acc.nId = ++m_nNextId;
	m_Accounts.push_back(acc);
	InsertRow((int)m_Accounts.size() - 1);
	SelectRow((int)m_Accounts.size() - 1);
	SaveAccounts();
}

void CDlgAutoLogin::OnBnClickedRemove()
{
	int nSel = GetSelectedRow();
	if (nSel < 0)
		return;
	CString strMsg;
	strMsg.Format(_T("Xóa tài khoản \"%s\" khỏi danh sách?"), (LPCTSTR)m_Accounts[nSel].strAccount);
	if (MessageBox(strMsg, _T("Auto Login"), MB_YESNO | MB_ICONQUESTION) != IDYES)
		return;
	StopAccount(nSel);
	m_Accounts.erase(m_Accounts.begin() + nSel);
	m_AccountList.DeleteItem(nSel);
	for (int i = nSel; i < (int)m_Accounts.size(); i++)
		UpdateRow(i);
	SelectRow(min(nSel, (int)m_Accounts.size() - 1));
	SaveAccounts();
}

void CDlgAutoLogin::OnBnClickedEdit()
{
	int nSel = GetSelectedRow();
	if (nSel < 0)
	{
		MessageBox(_T("Chọn tài khoản cần sửa trong danh sách"), _T("Auto Login"), MB_OK | MB_ICONWARNING);
		return;
	}
	AutoLoginAccount acc;
	if (!GetFields(acc))
		return;
	if (FindAccount(acc, nSel) >= 0)
	{
		MessageBox(_T("Tài khoản này đã có trong danh sách"), _T("Auto Login"), MB_OK | MB_ICONWARNING);
		return;
	}
	// Chỉ ghi phần cài đặt, giữ trạng thái đang chạy
	AutoLoginAccount& cur = m_Accounts[nSel];
	cur.strAccount = acc.strAccount;
	cur.strPassword = acc.strPassword;
	cur.strRegion = acc.strRegion;
	cur.strServer = acc.strServer;
	cur.nRoleIndex = acc.nRoleIndex;
	cur.bAutoRelogin = acc.bAutoRelogin;
	cur.nReloginDelay = acc.nReloginDelay;
	UpdateRow(nSel);
	SaveAccounts();
}

void CDlgAutoLogin::OnBnClickedUp()
{
	int nSel = GetSelectedRow();
	MoveRow(nSel, nSel - 1);
	m_AccountList.SetFocus();
}

void CDlgAutoLogin::OnBnClickedDown()
{
	int nSel = GetSelectedRow();
	if (nSel >= 0)
		MoveRow(nSel, nSel + 1);
	m_AccountList.SetFocus();
}

void CDlgAutoLogin::OnBnClickedShowPassword()
{
	BOOL bShow = IsDlgButtonChecked(IDC_SHOW_PASSWORD_CHECK) == BST_CHECKED;
	m_PasswordEdit.SetPasswordChar(bShow ? 0 : m_chPasswordChar);
	m_PasswordEdit.Invalidate();
}

void CDlgAutoLogin::OnBnClickedGetGamePath()
{
	CString strPath;
	GetDlgItemText(IDC_GAMEPATH_EDIT, strPath);
	CFileDialog dlg(TRUE, _T("exe"), strPath, OFN_FILEMUSTEXIST | OFN_HIDEREADONLY, _T("Game (*.exe)|*.exe||"), this);
	if (dlg.DoModal() != IDOK)
		return;
	SetDlgItemText(IDC_GAMEPATH_EDIT, dlg.GetPathName());
	OnGamePathKillFocus();
}

void CDlgAutoLogin::OnGamePathKillFocus()
{
	CString strPath;
	GetDlgItemText(IDC_GAMEPATH_EDIT, strPath);
	strPath.Trim();
	if (strPath == m_strGamePath)
		return;
	m_strGamePath = strPath;
	SaveAccounts();
}

/////////////////////////////////////////////////////////////////////////////
// Game

// Cửa sổ chính của Game thuộc process dwPID (NULL nếu Game chưa tạo xong cửa sổ)
static HWND FindGameWindowOfProcess(DWORD dwPID)
{
	HWND hWnd = NULL;
	while ((hWnd = ::FindWindowEx(NULL, hWnd, VLTK_WINCLASS, NULL)) != NULL)
	{
		DWORD dwWndPID = 0;
		GetWindowThreadProcessId(hWnd, &dwWndPID);
		if (dwWndPID == dwPID)
			return hWnd;
	}
	return NULL;
}

// Ẩn / hiện mọi cửa sổ Game của process dwPID
static void ShowGameWindows(DWORD dwPID, int nCmdShow)
{
	HWND hWnd = NULL;
	while ((hWnd = ::FindWindowEx(NULL, hWnd, VLTK_WINCLASS, NULL)) != NULL)
	{
		DWORD dwWndPID = 0;
		GetWindowThreadProcessId(hWnd, &dwWndPID);
		if (dwWndPID == dwPID)
			::ShowWindow(hWnd, nCmdShow);
	}
}

static void AddUniquePID(std::vector<DWORD>& PIDs, DWORD dwPID)
{
	if (dwPID && std::find(PIDs.begin(), PIDs.end(), dwPID) == PIDs.end())
		PIDs.push_back(dwPID);
}

static void TerminateGame(DWORD dwPID)
{
	HANDLE hProcess = OpenProcess(PROCESS_TERMINATE, FALSE, dwPID);
	if (hProcess)
	{
		TerminateProcess(hProcess, 0);
		CloseHandle(hProcess);
	}
}

// Process còn chạy và là Game (cùng tên file với Game Path, hoặc game_y.exe / gamecl.exe)
BOOL CDlgAutoLogin::IsGameProcess(DWORD dwPID)
{
	if (dwPID == 0)
		return FALSE;
	HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | SYNCHRONIZE, FALSE, dwPID);
	if (hProcess == NULL)
		return FALSE;
	BOOL bGame = FALSE;
	TCHAR szImage[MAX_PATH];
	if (WaitForSingleObject(hProcess, 0) == WAIT_TIMEOUT && GetProcessImageFileName(hProcess, szImage, MAX_PATH))
	{
		LPCTSTR szName = PathFindFileName(szImage);
		bGame = (!m_strGamePath.IsEmpty() && _tcsicmp(szName, PathFindFileName(m_strGamePath)) == 0)
			|| _tcsicmp(szName, _TGameY_Exe1) == 0 || _tcsicmp(szName, _TGameY_Exe2) == 0;
	}
	CloseHandle(hProcess);
	return bGame;
}

// Game đang chạy của tài khoản nIndex: Game do Auto Login mở (PID đã lưu),
// hoặc Game đã đăng nhập đúng tài khoản này (kể cả Game mở tay)
DWORD CDlgAutoLogin::GetAccountGame(int nIndex, GameWindow** ppGameWin)
{
	AutoLoginAccount& acc = m_Accounts[nIndex];
	if (ppGameWin)
		*ppGameWin = NULL;
	if (acc.dwProcessId && !IsGameProcess(acc.dwProcessId))
		acc.dwProcessId = 0;

	for (int i = 0; g_pCharList && i < g_pCharList->GetItemCount(); i++)
	{
		GameWindow* pGameWin = (GameWindow*)g_pCharList->GetItemData(i);
		if (pGameWin == NULL || pGameWin->m_hWnd == NULL)
			continue;
		DWORD dwPID = 0;
		GetWindowThreadProcessId(pGameWin->m_hWnd, &dwPID);
		if (dwPID == 0)
			continue;
		if (acc.dwProcessId == 0)
		{
			if (!pGameWin->m_szAccountID[0] || acc.strAccount.CompareNoCase(CString(pGameWin->m_szAccountID)) != 0)
				continue;
			// Game này đã thuộc tài khoản khác trong danh sách (vd cùng tài khoản, khác server)
			BOOL bOwned = FALSE;
			for (int j = 0; j < (int)m_Accounts.size(); j++)
				if (j != nIndex && m_Accounts[j].dwProcessId == dwPID)
					bOwned = TRUE;
			if (bOwned)
				continue;
			acc.dwProcessId = dwPID;
		}
		if (dwPID == acc.dwProcessId)
		{
			if (ppGameWin)
				*ppGameWin = pGameWin;
			break;
		}
	}
	return acc.dwProcessId;
}

// Game của tài khoản đã vào thế giới game (KUiSysMsgCentre != 0)
BOOL CDlgAutoLogin::IsAccountInGame(int nIndex)
{
	DWORD dwPID = GetAccountGame(nIndex);
	if (dwPID == 0)
		return FALSE;
	HANDLE hGame = OpenProcess(PROCESS_VM_READ, FALSE, dwPID);
	if (hGame == NULL)
		return FALSE;
	BOOL bInGame = ReadGameDword(hGame, LG_SMC_BASE_ADD) != 0;
	CloseHandle(hGame);
	return bInGame;
}

// Mở Game mới, gắn với tài khoản nIndex (nIndex < 0: không gắn)
BOOL CDlgAutoLogin::LaunchGame(int nIndex, CString& strError)
{
	OnGamePathKillFocus();	// Lấy Game Path vừa gõ tay
	if (m_strGamePath.IsEmpty() || !PathFileExists(m_strGamePath))
	{
		strError = _T("Chưa chọn Game Path hoặc file Game không tồn tại");
		return FALSE;
	}
	CString strDir = m_strGamePath;
	PathRemoveFileSpec(strDir.GetBuffer());
	strDir.ReleaseBuffer();
	CString strCmdLine;
	strCmdLine.Format(_T("\"%s\""), (LPCTSTR)m_strGamePath);
	STARTUPINFO si = { sizeof(si) };
	PROCESS_INFORMATION pi = { 0 };
	BOOL bCreated = CreateProcess(m_strGamePath, strCmdLine.GetBuffer(), NULL, NULL, FALSE, 0, NULL, strDir, &si, &pi);
	DWORD dwError = GetLastError();
	strCmdLine.ReleaseBuffer();
	if (!bCreated)
	{
		strError.Format(_T("Không mở được Game (GetLastError = %d)"), dwError);
		return FALSE;
	}
	CloseHandle(pi.hThread);
	CloseHandle(pi.hProcess);
	if (nIndex >= 0)
		m_Accounts[nIndex].dwProcessId = pi.dwProcessId;
	return TRUE;
}

// Đóng Game: gửi WM_CLOSE, sau GAME_CLOSE_TIMEOUT mà Game chưa tắt thì TerminateProcess (CheckClosingGames)
void CDlgAutoLogin::CloseGames(const std::vector<DWORD>& PIDs)
{
	DWORD dwNow = GetTickCount();
	for (int i = 0; i < (int)PIDs.size(); i++)
	{
		if (!IsGameProcess(PIDs[i]))
			continue;
		BOOL bPosted = FALSE;
		HWND hWnd = NULL;
		while ((hWnd = ::FindWindowEx(NULL, hWnd, VLTK_WINCLASS, NULL)) != NULL)
		{
			DWORD dwWndPID = 0;
			GetWindowThreadProcessId(hWnd, &dwWndPID);
			if (dwWndPID == PIDs[i] && ::PostMessage(hWnd, WM_CLOSE, 0, 0))
				bPosted = TRUE;
		}
		if (!bPosted)
		{
			TerminateGame(PIDs[i]);	// Chưa có cửa sổ để gửi WM_CLOSE
			continue;
		}
		BOOL bFound = FALSE;
		for (int j = 0; j < (int)m_Closing.size(); j++)
			if (m_Closing[j].dwPID == PIDs[i])
				bFound = TRUE;
		if (!bFound)
		{
			ClosingGame closing = { PIDs[i], dwNow + GAME_CLOSE_TIMEOUT };
			m_Closing.push_back(closing);
		}
	}
	UpdateGameStatus();
}

void CDlgAutoLogin::CheckClosingGames()
{
	DWORD dwNow = GetTickCount();
	for (int i = (int)m_Closing.size() - 1; i >= 0; i--)
	{
		if (IsGameProcess(m_Closing[i].dwPID) && (int)(dwNow - m_Closing[i].dwDeadline) < 0)
			continue;
		if (IsGameProcess(m_Closing[i].dwPID))
			TerminateGame(m_Closing[i].dwPID);	// Game không phản hồi WM_CLOSE
		m_Closing.erase(m_Closing.begin() + i);
	}
}

// Nội dung cột Trạng thái
CString CDlgAutoLogin::GetStatusText(int nIndex)
{
	AutoLoginAccount& acc = m_Accounts[nIndex];
	CString strStatus;
	if (m_Job.nId && m_Job.nId == acc.nId)
		return m_Job.strStatus;
	if (m_dwStartAllTick && acc.bChecked && !IsAccountInGame(nIndex))
	{
		int nLeft = (int)(m_dwStartAllTick - GetTickCount());
		strStatus.Format(_T("Tự LOGIN ALL sau %ds"), (max(nLeft, 0) + 999) / 1000);
		return strStatus;
	}
	int nEntry = FindLoginEntry(acc.nId);
	if (nEntry >= 0)
	{
		const AutoLoginEntry& entry = g_AutoLoginList[nEntry];
		int nLeft = entry.dwNextTick ? (int)(entry.dwNextTick - GetTickCount()) : 0;
		if (entry.nFails == 0 || nLeft <= 0)
			return _T("Chờ đăng nhập");
		strStatus.Format(_T("Lỗi: %s - thử lại %d/%d sau %ds"), (LPCTSTR)acc.strError, entry.nFails, GetRetryCount(), (nLeft + 999) / 1000);
		return strStatus;
	}

	GameWindow* pGameWin = NULL;
	DWORD dwPID = GetAccountGame(nIndex, &pGameWin);
	for (int i = 0; dwPID && i < (int)m_Closing.size(); i++)
		if (m_Closing[i].dwPID == dwPID)
			return _T("Đang thoát game");
	if (dwPID && IsAccountInGame(nIndex))
	{
		if (pGameWin && pGameWin->m_szPlayerName[0])
		{
			TCHAR szName[32];
			TCVN3ToUnicode(pGameWin->m_szPlayerName, szName, 31);
			strStatus.Format(_T("Trong game: %s"), szName);
		}
		else
			strStatus = _T("Trong game");
		return strStatus;
	}

	if (m_bRunAll && IsDlgButtonChecked(IDC_AUTO_RELOGIN_LOOP_CHECK) == BST_CHECKED && acc.bChecked && acc.bAutoRelogin && acc.dwOfflineTick)
	{
		int nLeft = acc.nReloginDelay - (int)(GetTickCount() - acc.dwOfflineTick);
		strStatus.Format(_T("Login lại sau %ds"), (max(nLeft, 0) + 999) / 1000);
		if (!acc.strError.IsEmpty())
			strStatus = _T("Lỗi: ") + acc.strError + _T(" - ") + strStatus;
		return strStatus;
	}
	if (!acc.strError.IsEmpty())
		return _T("Lỗi: ") + acc.strError;
	if (dwPID)
		return _T("Đã mở game");
	return strStatus;
}

// Cập nhật cột Trạng thái của mọi tài khoản
void CDlgAutoLogin::UpdateGameStatus()
{
	for (int i = 0; i < (int)m_Accounts.size(); i++)
	{
		CString strStatus = GetStatusText(i);
		if (m_AccountList.GetItemText(i, col_status) != strStatus)
			m_AccountList.SetItemText(i, col_status, strStatus);
	}
}

// Mở Game mới cho tài khoản đang chọn; tài khoản đã có Game thì đưa Game đó lên trước
void CDlgAutoLogin::OnBnClickedOpenGame()
{
	int nSel = GetSelectedRow();
	if (nSel >= 0 && GetAccountGame(nSel))
	{
		HWND hGameWnd = FindGameWindowOfProcess(m_Accounts[nSel].dwProcessId);
		if (hGameWnd)
		{
			if (!::IsWindowVisible(hGameWnd))
				ShowGameWindows(m_Accounts[nSel].dwProcessId, SW_SHOW);
			if (::IsIconic(hGameWnd))
				::ShowWindow(hGameWnd, SW_RESTORE);
			::SetForegroundWindow(hGameWnd);
		}
		return;
	}
	CString strError;
	if (!LaunchGame(nSel, strError))
	{
		MessageBox(strError + _T("\n") + m_strGamePath, _T("Auto Login"), MB_OK | MB_ICONWARNING);
		return;
	}
	UpdateGameStatus();
}

// Thoát Game của tài khoản đang chọn (và thôi đăng nhập / đăng nhập lại tài khoản này)
void CDlgAutoLogin::OnBnClickedTerminalGame()
{
	int nSel = GetSelectedRow();
	if (nSel < 0)
		return;
	StopAccount(nSel);
	DWORD dwPID = GetAccountGame(nSel);
	if (dwPID)
		CloseGames(std::vector<DWORD>(1, dwPID));
	UpdateGameStatus();
}

// Dừng đăng nhập + thoát mọi Game đang chạy: Game có cửa sổ + Game Auto Login vừa mở chưa kịp tạo cửa sổ
void CDlgAutoLogin::OnBnClickedTerminalGameAll()
{
	StopAllLogin();
	std::vector<DWORD> PIDs;
	HWND hWnd = NULL;
	while ((hWnd = ::FindWindowEx(NULL, hWnd, VLTK_WINCLASS, NULL)) != NULL)
	{
		DWORD dwPID = 0;
		GetWindowThreadProcessId(hWnd, &dwPID);
		if (IsGameProcess(dwPID))
			AddUniquePID(PIDs, dwPID);
	}
	for (int i = 0; i < (int)m_Accounts.size(); i++)
		AddUniquePID(PIDs, GetAccountGame(i));
	CloseGames(PIDs);
}

/////////////////////////////////////////////////////////////////////////////
// Đăng nhập: lần lượt từng tài khoản trong g_AutoLoginList (m_Job = tài khoản đang login).
// Mỗi nhịp timer xem Game đang ở màn hình nào rồi làm bước tương ứng:
// mở Game -> Bắt Đầu Trò Chơi -> Nhắc nhở: Xác định -> chọn cụm server + server -> tài khoản + mật khẩu -> chọn nhân vật + vào game

enum LOGIN_SCREEN
{
	scr_none,
	scr_launch,
	scr_init,
	scr_notice,
	scr_server,
	scr_login,
	scr_role,
};

// ms chờ sau thao tác trên màn hình nScreen (scr_launch..scr_role), đọc từ ô tương ứng trong nhóm Thiết lập
DWORD CDlgAutoLogin::GetWaitTime(int nScreen)
{
	int i = nScreen - scr_launch;
	if (i < 0 || i >= _countof(s_WaitEdits))
		return WAIT_LAUNCH;
	BOOL bOK = FALSE;
	UINT nWait = GetDlgItemInt(s_WaitEdits[i].nID, &bOK, FALSE);
	if (!bOK)
		nWait = s_WaitEdits[i].dwDefault;
	return max(nWait, (UINT)MIN_WAIT_ACTION);
}

int CDlgAutoLogin::FindLoginEntry(int nId)
{
	for (int i = 0; nId && i < (int)g_AutoLoginList.size(); i++)
		if (g_AutoLoginList[i].nId == nId)
			return i;
	return -1;
}

// Thêm tài khoản nIndex vào cuối danh sách cần login (đã có thì thôi)
void CDlgAutoLogin::AddLoginEntry(int nIndex)
{
	AutoLoginAccount& acc = m_Accounts[nIndex];
	if (FindLoginEntry(acc.nId) >= 0 || m_Job.nId == acc.nId)
		return;
	AutoLoginEntry entry;
	entry.nId = acc.nId;
	entry.strAccount = acc.strAccount;
	entry.nFails = 0;
	entry.dwNextTick = 0;
	acc.strError.Empty();
	g_AutoLoginList.push_back(entry);
}

// Bước 2: làm trống danh sách cần login rồi thêm các tài khoản được check theo thứ tự trên bảng.
// Từ vòng thứ 2 (vòng lặp): chỉ lấy lại tài khoản có "Tự động đăng nhập lại" và đã không ở trong game quá ReloginDelay
void CDlgAutoLogin::BuildLoginList()
{
	g_AutoLoginList.clear();
	m_nRound++;
	DWORD dwNow = GetTickCount();
	for (int i = 0; i < (int)m_Accounts.size(); i++)
	{
		AutoLoginAccount& acc = m_Accounts[i];
		if (!acc.bChecked)
			continue;
		if (m_nRound > 1)
		{
			if (!acc.bAutoRelogin || IsAccountInGame(i))
			{
				acc.dwOfflineTick = 0;
				continue;
			}
			if (acc.dwOfflineTick == 0)
				acc.dwOfflineTick = dwNow ? dwNow : 1;
			if ((int)(dwNow - acc.dwOfflineTick) < acc.nReloginDelay)
				continue;
		}
		acc.dwOfflineTick = 0;
		AddLoginEntry(i);
	}
}

// Bước 3: loại khỏi danh sách cần login các tài khoản đã ở trong game
void CDlgAutoLogin::RemoveOnlineEntries()
{
	for (int i = (int)g_AutoLoginList.size() - 1; i >= 0; i--)
	{
		int nIndex = FindAccountById(g_AutoLoginList[i].nId);
		if (nIndex < 0 || (m_Job.nId != g_AutoLoginList[i].nId && IsAccountInGame(nIndex)))
			g_AutoLoginList.erase(g_AutoLoginList.begin() + i);
	}
}

// Số lần thử lại khi đăng nhập thất bại (bỏ check = 0)
int CDlgAutoLogin::GetRetryCount()
{
	if (IsDlgButtonChecked(IDC_AUTO_RELOGIN_COUNT_CHECK) != BST_CHECKED)
		return 0;
	return (int)GetDlgItemInt(IDC_COUNT_RELOGIN_EDIT, NULL, FALSE);
}

void CDlgAutoLogin::StartLoginAll(BOOL bWarn)
{
	int nCount = 0;
	for (int i = 0; i < (int)m_Accounts.size(); i++)
		if (m_Accounts[i].bChecked)
			nCount++;
	if (nCount == 0)
	{
		if (bWarn)
			MessageBox(_T("Chưa check tài khoản nào (ô ở cột #)"), _T("Auto Login"), MB_OK | MB_ICONWARNING);
		return;
	}
	m_dwStartAllTick = 0;
	m_bRunAll = TRUE;
	m_nRound = 0;
	m_Job.nId = 0;
	BuildLoginList();
	RemoveOnlineEntries();
	UpdateGameStatus();
}

// Bỏ một tài khoản khỏi danh sách cần login (không tắt Game)
void CDlgAutoLogin::StopAccount(int nIndex)
{
	AutoLoginAccount& acc = m_Accounts[nIndex];
	acc.dwOfflineTick = 0;
	int nEntry = FindLoginEntry(acc.nId);
	if (nEntry >= 0)
		g_AutoLoginList.erase(g_AutoLoginList.begin() + nEntry);
	if (m_Job.nId == acc.nId)
		m_Job.nId = 0;
}

void CDlgAutoLogin::StopAllLogin()
{
	g_AutoLoginList.clear();
	m_Job.nId = 0;
	m_bRunAll = FALSE;
	m_dwStartAllTick = 0;
	for (int i = 0; i < (int)m_Accounts.size(); i++)
		m_Accounts[i].dwOfflineTick = 0;
	UpdateGameStatus();
}

// LOGIN: thêm tài khoản đang chọn vào danh sách cần login
void CDlgAutoLogin::OnBnClickedStartLogin()
{
	int nSel = GetSelectedRow();
	if (nSel < 0)
	{
		MessageBox(_T("Chọn tài khoản cần đăng nhập trong danh sách"), _T("Auto Login"), MB_OK | MB_ICONWARNING);
		return;
	}
	AddLoginEntry(nSel);
	RemoveOnlineEntries();
	UpdateGameStatus();
}

void CDlgAutoLogin::OnBnClickedStartLoginAll()
{
	StartLoginAll(TRUE);
}

void CDlgAutoLogin::OnBnClickedStopLogin()
{
	StopAllLogin();
}

void CDlgAutoLogin::OnLoginOptionChanged()
{
	if (IsDlgButtonChecked(IDC_AUTO_LOGINALL_CHECK) != BST_CHECKED)
		m_dwStartAllTick = 0;	// Bỏ check khi đang đếm ngược => hủy lần tự LOGIN ALL
	for (int i = 0; i < _countof(s_WaitEdits); i++)
	{
		DWORD dwWait = GetWaitTime(i + 1);
		if (GetDlgItemInt(s_WaitEdits[i].nID, NULL, FALSE) != dwWait)
			SetDlgItemInt(s_WaitEdits[i].nID, dwWait, FALSE);
	}
	SaveAccounts();
}

// Kết thúc lượt đăng nhập của tài khoản nIndex (đầu danh sách cần login). szError = NULL: thành công => loại khỏi danh sách.
// Thất bại: bRetry và còn lượt thử thì giữ ở đầu danh sách, chờ LOGIN_RETRY_DELAY rồi thử lại; hết lượt thì loại khỏi danh sách.
// bCloseGame: đóng Game để lần sau mở lại từ đầu
void CDlgAutoLogin::EndJob(int nIndex, LPCTSTR szError, BOOL bRetry, BOOL bCloseGame)
{
	int nId = m_Job.nId;
	m_Job.nId = 0;
	int nEntry = FindLoginEntry(nId);
	if (nIndex < 0)
	{
		if (nEntry >= 0)
			g_AutoLoginList.erase(g_AutoLoginList.begin() + nEntry);
		return;
	}
	AutoLoginAccount& acc = m_Accounts[nIndex];
	acc.strError = szError ? szError : _T("");
	acc.dwOfflineTick = 0;
	if (szError && bCloseGame && acc.dwProcessId)
		CloseGames(std::vector<DWORD>(1, acc.dwProcessId));
	if (!szError && acc.dwProcessId && IsDlgButtonChecked(IDC_HIDEGAME_LOGIN_CHECK) == BST_CHECKED)
		ShowGameWindows(acc.dwProcessId, SW_HIDE);
	if (nEntry < 0)
		return;
	AutoLoginEntry& entry = g_AutoLoginList[nEntry];
	if (szError && bRetry && entry.nFails < GetRetryCount())
	{
		entry.nFails++;
		entry.dwNextTick = GetTickCount() + LOGIN_RETRY_DELAY;
		if (entry.dwNextTick == 0)
			entry.dwNextTick = 1;
		return;
	}
	g_AutoLoginList.erase(g_AutoLoginList.begin() + nEntry);
	if (szError)
		acc.dwOfflineTick = GetTickCount() | 1;	// Vòng lặp: chờ ReloginDelay rồi mới thử lại tài khoản này
}

// Ghi nhận một thao tác trên màn hình nScreen: quá LOGIN_MAX_TRIES lần liên tiếp trên cùng màn hình => FALSE (kẹt)
BOOL CDlgAutoLogin::LoginAction(int nScreen, LPCTSTR szStatus, DWORD dwWait)
{
	if (m_Job.nScreen == nScreen)
		m_Job.nTries++;
	else
	{
		m_Job.nScreen = nScreen;
		m_Job.nTries = 1;
	}
	m_Job.strStatus = szStatus;
	m_Job.dwActionTick = GetTickCount();
	m_Job.dwWait = dwWait;
	return m_Job.nTries <= LOGIN_MAX_TRIES;
}

// Đăng nhập lần lượt danh sách cần login; hết danh sách mà đang LOGIN ALL có check vòng lặp thì lấy lại danh sách (bước 2)
void CDlgAutoLogin::RunLogin()
{
	if (m_dwStartAllTick && (int)(GetTickCount() - m_dwStartAllTick) >= 0)
		StartLoginAll(FALSE);

	if (m_Job.nId == 0)
	{
		while (!g_AutoLoginList.empty() && FindAccountById(g_AutoLoginList.front().nId) < 0)
			g_AutoLoginList.erase(g_AutoLoginList.begin());
		if (g_AutoLoginList.empty())
		{
			if (!m_bRunAll)
				return;
			if (IsDlgButtonChecked(IDC_AUTO_RELOGIN_LOOP_CHECK) != BST_CHECKED)
			{
				m_bRunAll = FALSE;	// Đã login xong danh sách
				return;
			}
			BuildLoginList();
			RemoveOnlineEntries();
			if (g_AutoLoginList.empty())
				return;
		}
		AutoLoginEntry& entry = g_AutoLoginList.front();
		if (entry.dwNextTick && (int)(GetTickCount() - entry.dwNextTick) < 0)
			return;	// Chờ thử lại
		entry.dwNextTick = 0;
		m_Job.nId = entry.nId;
		m_Job.dwStartTick = m_Job.dwActionTick = GetTickCount();
		m_Job.dwWait = 0;
		m_Job.bLaunched = FALSE;
		m_Job.nScreen = scr_none;
		m_Job.nTries = 0;
		m_Job.strStatus = _T("Kiểm tra đăng nhập");
	}

	int nIndex = FindAccountById(m_Job.nId);
	if (nIndex < 0)
	{
		EndJob(-1, NULL, FALSE, FALSE);
		return;
	}
	DWORD dwNow = GetTickCount();
	if (dwNow - m_Job.dwStartTick > LOGIN_TIMEOUT)
	{
		EndJob(nIndex, _T("Quá thời gian đăng nhập"), TRUE, TRUE);
		return;
	}
	if (dwNow - m_Job.dwActionTick < m_Job.dwWait)
		return;

	DWORD dwPID = GetAccountGame(nIndex);
	for (int i = 0; dwPID && i < (int)m_Closing.size(); i++)
	{
		if (m_Closing[i].dwPID == dwPID)
		{
			m_Job.strStatus = _T("Chờ game cũ tắt");
			return;
		}
	}
	if (dwPID == 0)
	{
		if (m_Job.bLaunched)
		{
			EndJob(nIndex, _T("Game đã tắt"), TRUE, FALSE);
			return;
		}
		CString strError;
		if (!LaunchGame(nIndex, strError))
		{
			EndJob(nIndex, strError, FALSE, FALSE);
			return;
		}
		m_Job.bLaunched = TRUE;
		LoginAction(scr_launch, _T("Đang mở game"), GetWaitTime(scr_launch));
		return;
	}

	HANDLE hGame = OpenProcess(PROCESS_VM_READ | PROCESS_VM_WRITE | PROCESS_VM_OPERATION | PROCESS_CREATE_THREAD | PROCESS_QUERY_INFORMATION, FALSE, dwPID);
	if (hGame == NULL)
	{
		CString strError;
		strError.Format(_T("Không truy cập được Game (GetLastError = %d)"), GetLastError());
		EndJob(nIndex, strError, TRUE, FALSE);
		return;
	}
	RunLoginStep(nIndex, hGame);
	CloseHandle(hGame);
}

// Một bước đăng nhập theo màn hình Game đang hiện
void CDlgAutoLogin::RunLoginStep(int nIndex, HANDLE hGame)
{
	CString strError;
	if (ReadGameDword(hGame, LG_SMC_BASE_ADD))
	{
		EndJob(nIndex, NULL, FALSE, FALSE);	// Đã vào thế giới game
		return;
	}

	DWORD dwWnd;
	if ((dwWnd = GetVisibleGameWnd(hGame, LG_ROLE_SELF)) != 0)
	{
		BOOL bWait = FALSE, bRetry = FALSE;
		if (!DoSelectRole(nIndex, hGame, dwWnd, strError, bWait, bRetry))
			EndJob(nIndex, strError, bRetry, bRetry);
		else if (!bWait && !LoginAction(scr_role, _T("Đang vào game"), GetWaitTime(scr_role)))
			EndJob(nIndex, _T("Không vào được game"), TRUE, TRUE);
		return;
	}
	if (GetVisibleGameWnd(hGame, LG_LOGIN_SELF))
	{
		if (!LoginAction(scr_login, _T("Đang đăng nhập"), GetWaitTime(scr_login)))
			EndJob(nIndex, _T("Đăng nhập thất bại (sai mật khẩu / tài khoản đang online?)"), TRUE, TRUE);
		else if (!DoAccountLogin(nIndex, hGame))
			EndJob(nIndex, _T("Không nhập được tài khoản"), TRUE, TRUE);
		return;
	}
	if ((dwWnd = GetVisibleGameWnd(hGame, LG_SEL_SELF)) != 0)
	{
		if (!LoginAction(scr_server, _T("Đang chọn server"), GetWaitTime(scr_server)))
			EndJob(nIndex, _T("Không kết nối được server"), TRUE, TRUE);
		else if (!DoSelectServer(nIndex, hGame, dwWnd, strError))
			EndJob(nIndex, strError, FALSE, FALSE);
		return;
	}
	if (GetVisibleGameWnd(hGame, LG_NOTICE_SELF))
	{
		// 15_10: Bắt Đầu Trò Chơi mở bảng "Nhắc nhở" trước, bấm Xác định mới sang chọn máy chủ
		if (!LoginAction(scr_notice, _T("Bấm Xác định (Nhắc nhở)"), GetWaitTime(scr_notice)))
		{
			EndJob(nIndex, _T("Không đóng được bảng Nhắc nhở"), TRUE, TRUE);
			return;
		}
		LPVOID pRemote = VirtualAllocEx(hGame, NULL, CRemoteCode::TOTAL_SIZE, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
		if (pRemote)
		{
			CRemoteCode code((DWORD)(UINT_PTR)pRemote);
			code.Begin(LG_NOTICE_SELF);
			code.PushImm(0);					// nParam
			code.PushThis(LG_NOTICE_OK);		// uParam = nút Xác định
			code.PushImm(LG_WND_BUTTON_CLICK);	// msg
			code.EcxThis(0);
			code.Call(LG_NOTICE_WNDPROC);
			code.End();
			DWORD dwExit = 0;
			ExecRemoteCode(hGame, pRemote, code, REMOTE_TIMEOUT, &dwExit);
		}
		return;
	}
	if (GetVisibleGameWnd(hGame, LG_UIN_BASE_ADD))
	{
		if (!LoginAction(scr_init, _T("Bấm Bắt Đầu Trò Chơi"), GetWaitTime(scr_init)))
		{
			EndJob(nIndex, _T("Không bấm được Bắt Đầu Trò Chơi"), TRUE, TRUE);
			return;
		}
		LPVOID pRemote = VirtualAllocEx(hGame, NULL, CRemoteCode::TOTAL_SIZE, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
		if (pRemote)
		{
			CRemoteCode code((DWORD)(UINT_PTR)pRemote);
			code.Begin(LG_UIN_BASE_ADD);
			code.PushThis(s_dwInitButtons[0]);	// Bắt Đầu Trò Chơi
			code.EcxThis(0);
			code.Call(LG_IOC_FUNC_ADD);
			code.End();
			DWORD dwExit = 0;
			ExecRemoteCode(hGame, pRemote, code, REMOTE_TIMEOUT, &dwExit);
		}
		return;
	}
	// Game đang tải / đang kết nối: chờ (LOGIN_TIMEOUT chặn nếu kẹt)
	if (m_Job.nScreen == scr_launch)
		m_Job.strStatus = _T("Đang chờ game");
}

// Tên dòng thứ i của danh sách KLoginServer (0x24 byte/dòng: Title[32] TCVN3 + IP[4])
static int FindGameServerTitle(HANDLE hGame, DWORD dwList, int nCount, CString strTitle)
{
	strTitle.Trim();
	for (int i = 0; dwList && i < nCount && i < 64; i++)
	{
		BYTE row[LG_SERVER_ROW] = { 0 };
		if (!ReadProcessMemory(hGame, (LPCVOID)(UINT_PTR)(dwList + i * LG_SERVER_ROW), row, sizeof(row), NULL))
			break;
		row[31] = 0;
		WCHAR szTitle[64];
		TCVN3ToUnicode((LPCSTR)row, szTitle, 63);
		CString strGameTitle(szTitle);
		strGameTitle.Trim();
		if (strGameTitle.CompareNoCase(strTitle) == 0)
			return i;
	}
	return -1;
}

// Màn hình chọn máy chủ: chọn cụm server (nếu khác) -> chọn server -> Đăng nhập (KUiSelServer::OnLogin)
BOOL CDlgAutoLogin::DoSelectServer(int nIndex, HANDLE hGame, DWORD dwSel, CString& strError)
{
	AutoLoginAccount& acc = m_Accounts[nIndex];
	int nRegions = GetRegionCount(hGame, dwSel);
	int nRegion = FindGameServerTitle(hGame, ReadGameDword(hGame, dwSel + LG_SEL_REGIONS), nRegions, acc.strRegion);
	if (nRegion < 0)
	{
		strError.Format(_T("Game không có cụm server \"%s\""), (LPCTSTR)acc.strRegion);
		return FALSE;
	}

	DWORD dwExit = 0;
	LPVOID pRemote;
	if ((int)ReadGameDword(hGame, dwSel + LG_SEL_REGION) != nRegion)
	{
		pRemote = VirtualAllocEx(hGame, NULL, CRemoteCode::TOTAL_SIZE, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
		if (!pRemote)
			return TRUE;	// Thử lại nhịp sau
		// Giống chọn 1 dòng ở Left/RightList (xem CDlgDebug::RunLoginCmd dbg_lg_region)
		CRemoteCode code((DWORD)(UINT_PTR)pRemote);
		code.Begin(LG_SEL_SELF);
		code.EcxThis(nRegion < LG_LIST_LEFT_NUM ? LG_SEL_RIGHTLIST : LG_SEL_LEFTLIST);
		code.Call(LG_LIST_CLEARSEL);
		code.PushImm(nRegion < LG_LIST_LEFT_NUM ? nRegion : nRegion - LG_LIST_LEFT_NUM);
		code.EcxThis(nRegion < LG_LIST_LEFT_NUM ? LG_SEL_LEFTLIST : LG_SEL_RIGHTLIST);
		code.Call(LG_LIST_SETCURSEL);	// Tự nạp lại IpList (LG_SEL_AUTOUPDATE)
		if (ReadGameDword(hGame, dwSel + LG_SEL_AUTOUPDATE) != 1)
		{
			code.EcxThis(0);
			code.Call(LG_SEL_UPDATE);
		}
		code.PushImm(0);
		code.EcxThis(LG_SEL_LEFTLIST);
		code.Call(LG_LIST_FOCUS);
		code.PushImm(0);
		code.EcxThis(LG_SEL_RIGHTLIST);
		code.Call(LG_LIST_FOCUS);
		code.PushImm(1);
		code.EcxThis(LG_SEL_IPLIST);
		code.Call(LG_LIST_FOCUS);
		code.End();
		ExecRemoteCode(hGame, pRemote, code, REMOTE_TIMEOUT, &dwExit);
		// Chọn server + Xác định ở nhịp sau (WAIT_SERVER): OnLogin dừng thread ping (TerminateThread),
		// gọi ngay khi thread ping vừa tạo còn đang khởi tạo sẽ làm treo Game
		m_Job.strStatus = _T("Đã chọn cụm server");
		return TRUE;
	}

	int nServers = (int)ReadGameDword(hGame, dwSel + LG_SEL_IPLIST + LG_LIST_COUNT);
	int nServer = FindGameServerTitle(hGame, ReadGameDword(hGame, dwSel + LG_SEL_SERVERS), nServers, acc.strServer);
	if (nServer < 0)
	{
		strError.Format(_T("Cụm \"%s\" không có server \"%s\""), (LPCTSTR)acc.strRegion, (LPCTSTR)acc.strServer);
		return FALSE;
	}

	pRemote = VirtualAllocEx(hGame, NULL, CRemoteCode::TOTAL_SIZE, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
	if (!pRemote)
		return TRUE;
	// Chọn server ở IpList rồi bấm Xác định (nút "Login" +0x1594 => KUiSelServer::OnLogin): kết nối + mở cửa sổ nhập tài khoản
	CRemoteCode code((DWORD)(UINT_PTR)pRemote);
	code.Begin(LG_SEL_SELF);
	code.PushImm(nServer);
	code.EcxThis(LG_SEL_IPLIST);
	code.Call(LG_LIST_SETCURSEL);
	code.EcxThis(0);
	code.Call(LG_SEL_LOGIN_FUNC);
	code.End();
	m_Job.strStatus = _T("Chọn server + Xác định");
	ExecRemoteCode(hGame, pRemote, code, REMOTE_TIMEOUT, &dwExit);
	return TRUE;
}

// Cửa sổ đăng nhập: điền tài khoản + mật khẩu, tick đồng ý điều khoản, Đăng nhập (KUiLogin::OnLogin)
BOOL CDlgAutoLogin::DoAccountLogin(int nIndex, HANDLE hGame)
{
	AutoLoginAccount& acc = m_Accounts[nIndex];
	LPVOID pRemote = VirtualAllocEx(hGame, NULL, CRemoteCode::TOTAL_SIZE, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
	if (!pRemote)
		return FALSE;
	char szAccount[64] = "", szPassword[64] = "";
	UnicodeToTCVN3(acc.strAccount, szAccount, 31);
	UnicodeToTCVN3(acc.strPassword, szPassword, 31);
	CRemoteCode code((DWORD)(UINT_PTR)pRemote);
	code.Begin(LG_LOGIN_SELF);
	code.PushImm(code.AddString(szAccount));
	code.EcxThis(LG_LOGIN_ACCOUNT);
	code.Call(LG_EDIT_SETTEXT);
	code.PushImm(code.AddString(szPassword));
	code.EcxThis(LG_LOGIN_PASSWORD);
	code.Call(LG_EDIT_SETTEXT);
	if (LG_LOGIN_AGREE)	// 15_10: cửa sổ đăng nhập không còn ô đồng ý
	{
		code.PushImm(1);
		code.EcxThis(LG_LOGIN_AGREE);
		code.Call(LG_BTN_CHECK);
	}
	code.EcxThis(0);
	code.Call(LG_LOGIN_FUNC);
	code.End();
	SecureZeroMemory(szPassword, sizeof(szPassword));
	DWORD dwExit = 0;
	BOOL bOK = ExecRemoteCode(hGame, pRemote, code, REMOTE_TIMEOUT, &dwExit) && dwExit;
	return bOK;
}

// Màn hình chọn nhân vật: chọn nhân vật thứ nRoleIndex rồi vào game (KUiSelPlayer::OnSelectPlayer + OnEnterGame).
// Danh sách nhân vật chưa tải xong => bWait = TRUE. Lỗi => FALSE + strError (bRetry: có thể thử lại).
BOOL CDlgAutoLogin::DoSelectRole(int nIndex, HANDLE hGame, DWORD dwRoleWnd, CString& strError, BOOL& bWait, BOOL& bRetry)
{
	AutoLoginAccount& acc = m_Accounts[nIndex];
	short sh[8] = { 0 };
	ReadProcessMemory(hGame, (LPCVOID)(UINT_PTR)(dwRoleWnd + LG_ROLE_NUM), sh, sizeof(sh), NULL);
	int nNumPlayer = sh[0];
	int nPage = sh[(LG_ROLE_PAGE - LG_ROLE_NUM) / 2];
	BOOL bLoaded = ReadGameDword(hGame, LG_LOGIC + LG_LOGIC_STATUS) == LG_LOGIC_STATUS_ROLES;
	if (bLoaded && nNumPlayer <= 0)
	{
		strError = _T("Tài khoản chưa có nhân vật");
		return FALSE;
	}
	if (!bLoaded)
	{
		bWait = TRUE;
		if (GetTickCount() - m_Job.dwActionTick > WAIT_ROLE_LIST)
		{
			strError = _T("Không tải được danh sách nhân vật");
			bRetry = TRUE;
			return FALSE;
		}
		m_Job.strStatus = _T("Đang tải nhân vật");
		return TRUE;
	}
	if (acc.nRoleIndex >= nNumPlayer)
	{
		strError.Format(_T("Tài khoản chỉ có %d nhân vật"), nNumPlayer);
		return FALSE;
	}
	int nSlot = acc.nRoleIndex - nPage;
	if (nSlot < 0 || nSlot >= LG_ROLE_PER_PAGE)
	{
		strError.Format(_T("Nhân vật %d không ở trang đang hiện"), acc.nRoleIndex + 1);
		return FALSE;
	}

	LPVOID pRemote = VirtualAllocEx(hGame, NULL, CRemoteCode::TOTAL_SIZE, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
	if (!pRemote)
	{
		bWait = TRUE;
		return TRUE;
	}
	CRemoteCode code((DWORD)(UINT_PTR)pRemote);
	code.Begin(LG_ROLE_SELF);
	code.PushImm(0);		// bPlaySound
	code.PushImm(nSlot);	// nSel = ô trên trang
	code.EcxThis(0);
	code.Call(LG_ROLE_SELECT);
	code.PushImm(1);		// 15_10: tick đồng ý điều khoản (cbUserAgreement) trước khi vào game
	code.EcxThis(LG_ROLE_AGREE);
	code.Call(LG_BTN_CHECK);
	code.EcxThis(0);
	code.Call(LG_ROLE_ENTER);
	code.End();
	DWORD dwExit = 0;
	ExecRemoteCode(hGame, pRemote, code, REMOTE_TIMEOUT, &dwExit);
	return TRUE;
}

void CDlgAutoLogin::OnTimer(UINT_PTR nIDEvent)
{
	if (nIDEvent == TIMER_AUTO_LOGIN)
	{
		CheckClosingGames();
		RunLogin();
		UpdateGameStatus();
		UpdatePlayerNames();
		return;
	}
	CDialog::OnTimer(nIDEvent);
}

// Ghi nhớ nhân vật đang vào game của mỗi tài khoản (file cfg giữ số tiền để bảng Thống kê hiện khi offline)
void CDlgAutoLogin::UpdatePlayerNames()
{
	BOOL bChanged = FALSE;
	for (int i = 0; i < (int)m_Accounts.size(); i++)
	{
		GameWindow* pGameWin = NULL;
		if (!GetAccountGame(i, &pGameWin) || pGameWin == NULL || !pGameWin->m_bLoadDone)
			continue;
		TCHAR szFile[MAX_PATH], szName[32];
		if (!pGameWin->GetProfileFileName(szFile))
			continue;
		TCVN3ToUnicode(pGameWin->m_szPlayerName, szName, 31);
		AutoLoginAccount& acc = m_Accounts[i];
		LPCTSTR szCfg = PathFindFileName(szFile);
		if (acc.strPlayerCfg != szCfg || acc.strPlayerName != szName)
		{
			acc.strPlayerCfg = szCfg;
			acc.strPlayerName = szName;
			bChanged = TRUE;
		}
	}
	if (bChanged)
		SaveAccounts();
}

void CDlgAutoLogin::OnBnClickedReport()
{
	if (m_DlgStats.m_hWnd == NULL && !m_DlgStats.Create(CDlgLoginStats::IDD, this))
		return;
	m_DlgStats.ShowStats();
}

/////////////////////////////////////////////////////////////////////////////
// CDlgLoginStats - bảng Thống kê

enum STATS_COLUMN
{
	sc_index,
	sc_account,
	sc_player,
	sc_status,
	sc_money,
	sc_earned,
	sc_tong,
	sc_perhour,
	sc_time,
	sc_amulet,
	sc_death,
};

// 12345678 -> "12.345.678"
static CString FormatMoney(__int64 nMoney)
{
	CString strDigits;
	strDigits.Format(_T("%I64d"), nMoney < 0 ? -nMoney : nMoney);
	CString strText;
	for (int i = 0; i < strDigits.GetLength(); i++)
	{
		if (i > 0 && (strDigits.GetLength() - i) % 3 == 0)
			strText += _T('.');
		strText += strDigits[i];
	}
	return nMoney < 0 ? _T("-") + strText : strText;
}

// Các control dời theo kích thước cửa sổ (m_rcInit cùng thứ tự)
static const UINT s_StatsCtrls[] =
{
	IDC_LOGIN_STATS_LIST,			// Giãn cả 2 chiều
	IDC_LOGIN_STATS_KEEP_STATIC,	// Dời xuống
	IDC_LOGIN_STATS_KEEP_EDIT,
	IDC_LOGIN_STATS_UNIT_STATIC,
	IDC_LOGIN_STATS_TONG_BUTTON,
	IDC_LOGIN_STATS_TONG_STATIC,	// Dời xuống + giãn ngang
	IDC_LOGIN_STATS_STATIC,
};

BEGIN_MESSAGE_MAP(CDlgLoginStats, CDialog)
	ON_WM_TIMER()
	ON_WM_SIZE()
	ON_WM_GETMINMAXINFO()
	ON_NOTIFY(LVN_ITEMCHANGED, IDC_LOGIN_STATS_LIST, &CDlgLoginStats::OnStatsItemChanged)
	ON_EN_KILLFOCUS(IDC_LOGIN_STATS_KEEP_EDIT, &CDlgLoginStats::OnKeepMoneyChanged)
	ON_BN_CLICKED(IDC_LOGIN_STATS_TONG_BUTTON, &CDlgLoginStats::OnBnClickedTongMoney)
END_MESSAGE_MAP()

void CDlgLoginStats::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_LOGIN_STATS_LIST, m_StatsList);
}

BOOL CDlgLoginStats::OnInitDialog()
{
	CDialog::OnInitDialog();

	m_StatsList.SetExtendedStyle(m_StatsList.GetExtendedStyle() | LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES | LVS_EX_CHECKBOXES);
	m_StatsList.InsertColumn(sc_index, _T("#"), LVCFMT_LEFT, 42);
	m_StatsList.InsertColumn(sc_account, _T("Tài khoản"), LVCFMT_LEFT, 95);
	m_StatsList.InsertColumn(sc_player, _T("Nhân vật"), LVCFMT_LEFT, 110);
	m_StatsList.InsertColumn(sc_status, _T("Trạng thái"), LVCFMT_LEFT, 80);
	m_StatsList.InsertColumn(sc_money, _T("Tổng tiền"), LVCFMT_RIGHT, 100);
	m_StatsList.InsertColumn(sc_earned, _T("Kiếm được"), LVCFMT_RIGHT, 90);
	m_StatsList.InsertColumn(sc_tong, _T("Đã gom"), LVCFMT_RIGHT, 90);
	m_StatsList.InsertColumn(sc_perhour, _T("TB / giờ"), LVCFMT_RIGHT, 90);
	m_StatsList.InsertColumn(sc_time, _T("Thời gian"), LVCFMT_RIGHT, 65);
	m_StatsList.InsertColumn(sc_amulet, _T("Thổ Địa Phù"), LVCFMT_RIGHT, 75);
	m_StatsList.InsertColumn(sc_death, _T("Chết"), LVCFMT_RIGHT, 45);

	SetDlgItemText(IDC_LOGIN_STATS_STATIC, _T("Cập nhật mỗi 2 giây. Tổng tiền = tiền trên người + rương (offline: số tiền lưu lần cuối). ")
		_T("Kiếm được (đã cộng tiền đã gom), Đã gom, TB / giờ, Thổ Địa Phù, Chết: tính từ lúc vào game lần này (Thổ Địa Phù chỉ đếm lần Auto dùng).\n")
		_T("Gom tiền: check các nhân vật (check dòng TỔNG = chọn tất cả) rồi bấm nút. Chỉ góp tiền trên người, nhân vật phải online và đã gắn Auto; đang ở ngoài thành thì dùng Thổ Địa Phù về thành rồi mới góp."));
	SetDlgItemInt(IDC_LOGIN_STATS_KEEP_EDIT, m_pAutoLogin->m_nTongKeep, FALSE);

	CRect rcClient;
	GetClientRect(&rcClient);
	for (int i = 0; i < _countof(s_StatsCtrls); i++)
	{
		GetDlgItem(s_StatsCtrls[i])->GetWindowRect(&m_rcInit[i]);
		ScreenToClient(&m_rcInit[i]);
	}
	m_cxInit = rcClient.Width();
	m_cyInit = rcClient.Height();
	return TRUE;
}

void CDlgLoginStats::OnCancel()
{
	KillTimer(TIMER_LOGIN_STATS);
	ShowWindow(SW_HIDE);
}

void CDlgLoginStats::ShowStats()
{
	UpdateStats();
	SetTimer(TIMER_LOGIN_STATS, LOGIN_STATS_INTERVAL, NULL);
	ShowWindow(SW_SHOW);
	SetForegroundWindow();
}

void CDlgLoginStats::OnTimer(UINT_PTR nIDEvent)
{
	if (nIDEvent == TIMER_LOGIN_STATS)
	{
		UpdateStats();
		return;
	}
	CDialog::OnTimer(nIDEvent);
}

void CDlgLoginStats::OnSize(UINT nType, int cx, int cy)
{
	CDialog::OnSize(nType, cx, cy);
	if (m_cyInit == 0 || nType == SIZE_MINIMIZED)
		return;
	int dx = cx - m_cxInit, dy = cy - m_cyInit;
	for (int i = 0; i < _countof(s_StatsCtrls); i++)
	{
		CWnd* pCtrl = GetDlgItem(s_StatsCtrls[i]);
		if (pCtrl == NULL)
			continue;
		CRect rc = m_rcInit[i];
		if (i == 0)
		{
			rc.right = max(rc.right + dx, rc.left);
			rc.bottom = max(rc.bottom + dy, rc.top);
		}
		else
		{
			rc.OffsetRect(0, dy);
			if (s_StatsCtrls[i] == IDC_LOGIN_STATS_TONG_STATIC || s_StatsCtrls[i] == IDC_LOGIN_STATS_STATIC)
				rc.right = max(rc.right + dx, rc.left);
		}
		pCtrl->MoveWindow(&rc);
		pCtrl->Invalidate();
	}
}

void CDlgLoginStats::OnGetMinMaxInfo(MINMAXINFO* lpMMI)
{
	lpMMI->ptMinTrackSize.x = 500;
	lpMMI->ptMinTrackSize.y = 220;
	CDialog::OnGetMinMaxInfo(lpMMI);
}

void CDlgLoginStats::SetCell(int nRow, int nCol, LPCTSTR szText)
{
	if (m_StatsList.GetItemText(nRow, nCol) != szText)
		m_StatsList.SetItemText(nRow, nCol, szText);
}

void CDlgLoginStats::SetRowCheck(int nRow, BOOL bCheck)
{
	if (m_StatsList.GetCheck(nRow) != bCheck)
		m_StatsList.SetCheck(nRow, bCheck);
}

// Mỗi tài khoản trong bảng Auto Login một dòng + dòng Tổng cuối bảng
void CDlgLoginStats::UpdateStats()
{
	m_bUpdating = TRUE;
	std::vector<AutoLoginAccount>& Accounts = m_pAutoLogin->m_Accounts;
	int nRows = Accounts.empty() ? 0 : (int)Accounts.size() + 1;
	while (m_StatsList.GetItemCount() < nRows)
		m_StatsList.InsertItem(m_StatsList.GetItemCount(), _T(""));
	while (m_StatsList.GetItemCount() > nRows)
		m_StatsList.DeleteItem(m_StatsList.GetItemCount() - 1);

	DWORD dwNow = GetTickCount();
	__int64 nTotalMoney = 0, nTotalEarned = 0, nTotalPerHour = 0;
	__int64 nTotalTong = 0;
	int nOnline = 0, nTotalAmulet = 0, nTotalDeath = 0;
	BOOL bAllChecked = TRUE;
	CString strText;
	for (int i = 0; i < (int)Accounts.size(); i++)
	{
		const AutoLoginAccount& acc = Accounts[i];
		GameWindow* pGameWin = NULL;
		m_pAutoLogin->GetAccountGame(i, &pGameWin);
		BOOL bOnline = pGameWin && pGameWin->m_bLoadDone && pGameWin->m_dwLoginTick;

		strText.Format(_T("%d"), i + 1);
		SetCell(i, sc_index, strText);
		SetRowCheck(i, acc.bTongGather);
		bAllChecked &= acc.bTongGather;
		SetCell(i, sc_account, acc.strAccount);
		CString strName = acc.strPlayerName;
		if (bOnline)
		{
			TCHAR szName[32];
			TCVN3ToUnicode(pGameWin->m_szPlayerName, szName, 31);
			strName = szName;
		}
		SetCell(i, sc_player, strName);
		SetCell(i, sc_status, bOnline ? _T("Online") : _T("Offline"));

		CString strMoney, strEarned, strTong, strPerHour, strTime, strAmulet, strDeath;
		if (bOnline)
		{
			nOnline++;
			DWORD dwTime = dwNow - pGameWin->m_dwLoginTick;
			strTime.Format(_T("%d:%02d:%02d"), dwTime / 3600000, dwTime / 60000 % 60, dwTime / 1000 % 60);
			if (pGameWin->m_bStatReady)
			{
				__int64 nEarned = pGameWin->m_nMoney - pGameWin->m_nStartMoney + pGameWin->m_nTongMoney;
				__int64 nPerHour = dwTime ? nEarned * 3600000 / dwTime : 0;
				strMoney = FormatMoney(pGameWin->m_nMoney);
				strEarned = FormatMoney(nEarned);
				strTong = FormatMoney(pGameWin->m_nTongMoney);
				if (pGameWin->m_nTongPending)
					strTong += _T(" (đang gom)");
				strPerHour = FormatMoney(nPerHour);
				strAmulet.Format(_T("%d"), pGameWin->m_nAMUsed);
				strDeath.Format(_T("%d"), pGameWin->m_nDeathCount);
				nTotalMoney += pGameWin->m_nMoney;
				nTotalEarned += nEarned;
				nTotalTong += pGameWin->m_nTongMoney;
				nTotalPerHour += nPerHour;
				nTotalAmulet += pGameWin->m_nAMUsed;
				nTotalDeath += pGameWin->m_nDeathCount;
			}
			else
				strMoney = _T("...");
		}
		else if (!acc.strPlayerCfg.IsEmpty())
		{
			CString strFile;
			strFile.Format(_T("%s\\UserConfig\\%s"), g_szAppFolder, (LPCTSTR)acc.strPlayerCfg);
			__int64 nMoney = 0;
			if (GameWindow::ReadSavedMoney(strFile, &nMoney))
			{
				strMoney = FormatMoney(nMoney);
				nTotalMoney += nMoney;
			}
		}
		SetCell(i, sc_money, strMoney);
		SetCell(i, sc_earned, strEarned);
		SetCell(i, sc_tong, strTong);
		SetCell(i, sc_perhour, strPerHour);
		SetCell(i, sc_time, strTime);
		SetCell(i, sc_amulet, strAmulet);
		SetCell(i, sc_death, strDeath);
	}

	if (nRows == 0)
	{
		m_bUpdating = FALSE;
		return;
	}
	int nTotal = nRows - 1;
	SetRowCheck(nTotal, bAllChecked);
	SetCell(nTotal, sc_index, _T(""));
	SetCell(nTotal, sc_account, _T("TỔNG"));
	SetCell(nTotal, sc_player, _T(""));
	strText.Format(_T("%d/%d online"), nOnline, (int)Accounts.size());
	SetCell(nTotal, sc_status, strText);
	SetCell(nTotal, sc_money, FormatMoney(nTotalMoney));
	SetCell(nTotal, sc_earned, FormatMoney(nTotalEarned));
	SetCell(nTotal, sc_tong, FormatMoney(nTotalTong));
	SetCell(nTotal, sc_perhour, FormatMoney(nTotalPerHour));
	SetCell(nTotal, sc_time, _T(""));
	strText.Format(_T("%d"), nTotalAmulet);
	SetCell(nTotal, sc_amulet, strText);
	strText.Format(_T("%d"), nTotalDeath);
	SetCell(nTotal, sc_death, strText);
	m_bUpdating = FALSE;
}

// Check / bỏ check 1 dòng: chọn nhân vật để gom tiền. Dòng TỔNG = chọn / bỏ tất cả
void CDlgLoginStats::OnStatsItemChanged(NMHDR* pNMHDR, LRESULT* pResult)
{
	*pResult = 0;
	LPNMLISTVIEW pNMLV = reinterpret_cast<LPNMLISTVIEW>(pNMHDR);
	if (m_bUpdating || pNMLV->iItem < 0 || !(pNMLV->uChanged & LVIF_STATE)
		|| !((pNMLV->uNewState ^ pNMLV->uOldState) & LVIS_STATEIMAGEMASK) || !(pNMLV->uOldState & LVIS_STATEIMAGEMASK))
		return;

	std::vector<AutoLoginAccount>& Accounts = m_pAutoLogin->m_Accounts;
	BOOL bCheck = m_StatsList.GetCheck(pNMLV->iItem);
	if (pNMLV->iItem == (int)Accounts.size())
	{
		for (int i = 0; i < (int)Accounts.size(); i++)
			Accounts[i].bTongGather = bCheck;
	}
	else if (pNMLV->iItem < (int)Accounts.size())
		Accounts[pNMLV->iItem].bTongGather = bCheck;
	else
		return;
	m_pAutoLogin->SaveAccounts();
	UpdateStats();
}

void CDlgLoginStats::OnKeepMoneyChanged()
{
	BOOL bOK = FALSE;
	int nKeep = (int)GetDlgItemInt(IDC_LOGIN_STATS_KEEP_EDIT, &bOK, FALSE);
	if (!bOK)
	{
		SetDlgItemInt(IDC_LOGIN_STATS_KEEP_EDIT, m_pAutoLogin->m_nTongKeep, FALSE);
		return;
	}
	if (nKeep != m_pAutoLogin->m_nTongKeep)
	{
		m_pAutoLogin->m_nTongKeep = nKeep;
		m_pAutoLogin->SaveAccounts();
	}
}

// Góp tiền trên người của các nhân vật được check vào quỹ bang, mỗi nhân vật giữ lại m_nTongKeep vạn (tính cả rương)
void CDlgLoginStats::OnBnClickedTongMoney()
{
	OnKeepMoneyChanged();
	std::vector<AutoLoginAccount>& Accounts = m_pAutoLogin->m_Accounts;
	int nChecked = 0;
	for (int i = 0; i < (int)Accounts.size(); i++)
		nChecked += Accounts[i].bTongGather ? 1 : 0;
	if (nChecked == 0)
	{
		SetDlgItemText(IDC_LOGIN_STATS_TONG_STATIC, _T("Chưa check nhân vật nào để gom tiền"));
		return;
	}
	__int64 nKeep = (__int64)m_pAutoLogin->m_nTongKeep * 10000;
	CString strText;
	strText.Format(_T("Góp tiền trên người của %d nhân vật đã check vào quỹ bang?\nMỗi nhân vật giữ lại %s lượng (tính cả tiền trong rương).\nNhân vật ở ngoài thành sẽ dùng Thổ Địa Phù về thành trước."),
		nChecked, (LPCTSTR)FormatMoney(nKeep));
	if (MessageBox(strText, _T("Gom tiền lên bang"), MB_YESNO | MB_ICONQUESTION) != IDYES)
		return;

	__int64 nSent = 0;
	int nSentCount = 0;
	CString strSkip;
	for (int i = 0; i < (int)Accounts.size(); i++)
	{
		const AutoLoginAccount& acc = Accounts[i];
		if (!acc.bTongGather)
			continue;
		GameWindow* pGameWin = NULL;
		m_pAutoLogin->GetAccountGame(i, &pGameWin);
		LPCTSTR szReason = NULL;
		int nMoney = 0;
		if (!pGameWin || !pGameWin->m_bLoadDone || !pGameWin->m_dwLoginTick)
			szReason = _T("offline");
		else if (!pGameWin->m_bAttached)
			szReason = _T("chưa gắn Auto");
		else if (!pGameWin->m_bStatReady)
			szReason = _T("vừa vào game");
		else if (pGameWin->m_nTongPending)
			szReason = _T("đang gom");
		else if ((nMoney = pGameWin->StoreTongMoney(nKeep)) == 0)
			szReason = _T("không đủ tiền");
		else if (nMoney < 0)
			szReason = _T("lỗi gửi lệnh");
		if (szReason)
		{
			strSkip.AppendFormat(_T("%s%s (%s)"), strSkip.IsEmpty() ? _T("") : _T(", "), (LPCTSTR)acc.strAccount, szReason);
			continue;
		}
		nSent += nMoney;
		nSentCount++;
	}
	strText.Format(_T("[%s] Đã gửi lệnh góp %s lượng (%d nhân vật)"), (LPCTSTR)CTime::GetCurrentTime().Format(_T("%H:%M:%S")),
		(LPCTSTR)FormatMoney(nSent), nSentCount);
	if (!strSkip.IsEmpty())
		strText += _T(". Bỏ qua: ") + strSkip;
	SetDlgItemText(IDC_LOGIN_STATS_TONG_STATIC, strText);
	UpdateStats();
}
