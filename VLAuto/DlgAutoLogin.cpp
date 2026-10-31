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
#include <map>
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
#define LOGIN_STATS_INTERVAL	2000	// ms: nhịp cập nhật các cột thống kê
#define AUTO_LOGIN_X			0		// Vị trí cửa sổ Auto Login mỗi lần mở
#define AUTO_LOGIN_Y			150
#define DEFAULT_TONG_KEEP		10		// vạn: mặc định số tiền giữ lại khi gom tiền lên bang
#define WAIT_ROLE_LIST			20000	// ms chờ danh sách nhân vật sau khi đăng nhập
#define EXIT_CHECK_INTERVAL		1000	// ms: nhịp kiểm tra Cài đặt thoát
#define NET_LOST_SEC			20		// giây: nhân vật không được server đồng bộ quá lâu => coi là mất kết nối

#define AUTOLOGIN_DATA_FILE		_T("%s\\AutoLogin.ini")
#define SERVERLIST_FILE			_T("%s\\UiConfig\\serverlist_.txt")
#define MAX_ROLE_COUNT			3		// Số nhân vật tối đa trên một tài khoản
#define DEFAULT_RELOGIN_DELAY	10000	// ms
static void LoginLog(LPCTSTR szFormat, ...);
#define LOGIN_RETRY_DELAY		2000	// ms: đăng nhập thất bại thì chờ rồi thử lại
#define DEFAULT_RETRY_COUNT		3		// Số lần thử lại mặc định
#define DEFAULT_LOGINALL_DELAY	10000	// ms: mặc định của "Tự động login all sau"
#define MIN_LOGINALL_DELAY		2000	// ms: tối thiểu, để VLAuto kịp quét các Game đang chạy (tránh mở thêm Game cho tài khoản đã online)

std::vector<AutoLoginEntry> g_AutoLoginList;

// Cài đặt thoát (tab Thống kê), lưu ở [AutoLogin] trong AutoLogin.ini. nOn / nVal: offset trong AutoLoginExitOpt (-1 = không có)
struct EXIT_OPT_CTRL
{
	UINT	nCheck, nEdit;
	LPCTSTR	szOnKey, szValKey;
	int		nDefOn, nDefVal, nMinVal;
	int		nOn, nVal;
};

#define EXIT_OFF(m) ((int)offsetof(AutoLoginExitOpt, m))
static const EXIT_OPT_CTRL s_ExitOpts[] =
{
	{ IDC_LOGIN_EXIT_LOST_CHECK,	IDC_LOGIN_EXIT_LOST_EDIT,	_T("ExitLost"),		_T("ExitLostMin"),	0, 5,	1,	EXIT_OFF(bLost),	EXIT_OFF(nLostMin) },
	{ IDC_LOGIN_EXIT_NOEXP_CHECK,	IDC_LOGIN_EXIT_NOEXP_EDIT,	_T("ExitNoExp"),	_T("ExitNoExpMin"),	0, 3,	1,	EXIT_OFF(bNoExp),	EXIT_OFF(nNoExpMin) },
	{ IDC_LOGIN_EXIT_HUNG_CHECK,	IDC_LOGIN_EXIT_HUNG_EDIT,	_T("ExitHung"),		_T("ExitHungSec"),	0, 210,	10,	EXIT_OFF(bHung),	EXIT_OFF(nHungSec) },
	{ IDC_LOGIN_EXIT_STAY_CHECK,	IDC_LOGIN_EXIT_STAY_EDIT,	_T("ExitStay"),		_T("ExitStayMin"),	0, 5,	1,	EXIT_OFF(bStay),	EXIT_OFF(nStayMin) },
	{ 0,							IDC_LOGIN_EXIT_DIST_EDIT,	NULL,				_T("ExitStayDist"),	0, 211,	10,	-1,					EXIT_OFF(nStayDist) },
	{ IDC_LOGIN_EXIT_FIGHT_CHECK,	0,							_T("ExitStayFight"),	NULL,			1, 0,	0,	EXIT_OFF(bStayFight), -1 },
};
#undef EXIT_OFF

static int& ExitOptField(AutoLoginExitOpt& opt, int nOffset)
{
	return *(int*)((BYTE*)&opt + nOffset);
}

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
	col_index,		// Ô check (LOGIN ALL + gom tiền lên bang) + số thứ tự
	col_account,
	col_player,
	col_status,
	col_money,
	col_earned,
	col_tong,
	col_perhour,
	col_time,
	col_amulet,
	col_death,
	col_region,
	col_server,
	col_role,
	col_relogin,
};

enum AUTO_LOGIN_TAB
{
	tab_setting,	// Nhóm Tài khoản + Thiết lập
	tab_stats,		// Gom tiền + Cài đặt thoát
	tab_hengio,		// Nhóm hẹn giờ
};

#define GROUP_ROW_BK			RGB(0, 102, 204)	// Màu nền dòng Nhóm trong bảng
#define GROUP_ROW_TEXT			RGB(255, 255, 255)
#define GROUP_SELECTED_BK		RGB(220, 0, 0)		// Màu nền dòng Nhóm đang chọn
#define SELECTED_ROW_BK			RGB(204, 232, 255)	// Màu nền dòng đang chọn
#define SELECTED_ROW_TEXT		RGB(0, 0, 0)
#define HENGIO_OFF_COLOR		RGB(200, 0, 0)		// Nút hẹn giờ: đang tắt
#define HENGIO_ON_COLOR			RGB(0, 150, 0)		// Nút hẹn giờ: đang bật

// "HH:MM" của phút trong ngày (-1 = "--:--")
static CString FormatDayMinute(int nMin)
{
	CString strText;
	if (nMin < 0)
		strText = _T("--:--");
	else
		strText.Format(_T("%02d:%02d"), nMin / 60, nMin % 60);
	return strText;
}

// "HH:MM" -> phút trong ngày (sai / trống = -1)
static int ParseDayMinute(LPCTSTR szText)
{
	int nHour = -1, nMin = -1;
	if (_stscanf(szText, _T("%d:%d"), &nHour, &nMin) != 2 || nHour < 0 || nHour > 23 || nMin < 0 || nMin > 59)
		return -1;
	return nHour * 60 + nMin;
}

static CString GetMapName(int nMapID);
static CString FormatMoney(__int64 nMoney, BOOL bDecimal = FALSE);

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
	: CDialog(CDlgAutoLogin::IDD, pParent), m_PageSetting(IDD_AUTO_LOGIN_SETTING_PAGE), m_PageStats(IDD_AUTO_LOGIN_STATS_PAGE), m_PageHenGio(IDD_AUTO_LOGIN_HENGIO_PAGE), m_nTongKeep(DEFAULT_TONG_KEEP)
{
	m_bHenGio = FALSE;
	m_nHenGioMinute = 0;
	m_chPasswordChar = 0;
	m_bUpdatingList = FALSE;
	m_nNextId = 0;
	m_Job.nId = 0;
	m_Job.hRemote = NULL;
	m_Job.pRemote = NULL;
	m_Job.dwRemotePID = 0;
	m_bRunAll = FALSE;
	m_dwStartAllTick = 0;
	m_dwExitCheckTick = 0;
	for (int i = 0; i < _countof(s_ExitOpts); i++)
	{
		if (s_ExitOpts[i].nOn >= 0)
			ExitOptField(m_ExitOpt, s_ExitOpts[i].nOn) = s_ExitOpts[i].nDefOn;
		if (s_ExitOpts[i].nVal >= 0)
			ExitOptField(m_ExitOpt, s_ExitOpts[i].nVal) = s_ExitOpts[i].nDefVal;
	}
}

CDlgAutoLogin::~CDlgAutoLogin()
{
	if (m_Job.hRemote)
		CloseHandle(m_Job.hRemote);
}

void CDlgAutoLogin::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_ACCOUNT_INFO_ITEM_LIST, m_AccountList);
	DDX_Control(pDX, IDC_AUTO_LOGIN_TAB, m_Tab);
	DDX_Control(pDX, IDC_START_HENGIO_LOGIN_BUTTON, m_HenGioButton);
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
	ON_NOTIFY(TCN_SELCHANGE, IDC_AUTO_LOGIN_TAB, &CDlgAutoLogin::OnTabSelChange)
	ON_EN_KILLFOCUS(IDC_LOGIN_STATS_KEEP_EDIT, &CDlgAutoLogin::OnKeepMoneyChanged)
	ON_BN_CLICKED(IDC_LOGIN_STATS_TONG_BUTTON, &CDlgAutoLogin::OnBnClickedTongMoney)
	ON_CONTROL_RANGE(BN_CLICKED, IDC_LOGIN_EXIT_LOST_CHECK, IDC_LOGIN_EXIT_FIGHT_CHECK, &CDlgAutoLogin::OnExitOptionChanged)
	ON_CONTROL_RANGE(EN_KILLFOCUS, IDC_LOGIN_EXIT_LOST_CHECK, IDC_LOGIN_EXIT_FIGHT_CHECK, &CDlgAutoLogin::OnExitOptionChanged)
	ON_NOTIFY(NM_CUSTOMDRAW, IDC_ACCOUNT_INFO_ITEM_LIST, &CDlgAutoLogin::OnAccountListCustomDraw)
	ON_NOTIFY(NM_RCLICK, IDC_ACCOUNT_INFO_ITEM_LIST, &CDlgAutoLogin::OnAccountListRClick)
	ON_NOTIFY(NM_DBLCLK, IDC_ACCOUNT_INFO_ITEM_LIST, &CDlgAutoLogin::OnAccountListDblClk)
	ON_BN_CLICKED(IDC_ADD_HENGIO_BUTTON, &CDlgAutoLogin::OnBnClickedAddGroup)
	ON_BN_CLICKED(IDC_DELETE_HENGIO_BUTTON, &CDlgAutoLogin::OnBnClickedDeleteGroup)
	ON_BN_CLICKED(IDC_EDIT_HENGIO_BUTTON, &CDlgAutoLogin::OnBnClickedEditGroup)
	ON_BN_CLICKED(IDC_START_HENGIO_LOGIN_BUTTON, &CDlgAutoLogin::OnBnClickedHenGio)
	ON_WM_TIMER()
END_MESSAGE_MAP()


// CDlgAutoLogin message handlers
BOOL CDlgAutoLogin::OnInitDialog()
{
	CDialog::OnInitDialog();

	// Các trang của tab là dialog con, control của trang gắn vào biến sau khi tạo
	m_PageSetting.Create(IDD_AUTO_LOGIN_SETTING_PAGE, this);
	m_PageStats.Create(IDD_AUTO_LOGIN_STATS_PAGE, this);
	m_PageHenGio.Create(IDD_AUTO_LOGIN_HENGIO_PAGE, this);
	m_HenGioButton.ModifyStyle(BS_TYPEMASK, BS_OWNERDRAW);	// Tô màu nền theo trạng thái
	m_RegionCombo.SubclassDlgItem(IDC_SERVER_REGION_COMBO, &m_PageSetting);
	m_ServerCombo.SubclassDlgItem(IDC_SERVER_NAME_COMBO, &m_PageSetting);
	m_RoleCombo.SubclassDlgItem(IDC_ROLE_INDEX_COMBO, &m_PageSetting);
	m_PasswordEdit.SubclassDlgItem(IDC_PASSWORD_EDIT, &m_PageSetting);

	m_strDataFile.Format(AUTOLOGIN_DATA_FILE, g_szAppFolder);
	m_chPasswordChar = m_PasswordEdit.GetPasswordChar();

	m_AccountList.SetExtendedStyle(m_AccountList.GetExtendedStyle() | LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES | LVS_EX_CHECKBOXES);
	m_AccountList.InsertColumn(col_index, _T("#"), LVCFMT_LEFT, 40);
	m_AccountList.InsertColumn(col_account, _T("Tài khoản"), LVCFMT_LEFT, 85);
	m_AccountList.InsertColumn(col_player, _T("Nhân vật"), LVCFMT_LEFT, 95);
	m_AccountList.InsertColumn(col_status, _T("Trạng thái"), LVCFMT_LEFT, 170);
	m_AccountList.InsertColumn(col_money, _T("Tổng tiền"), LVCFMT_RIGHT, 85);
	m_AccountList.InsertColumn(col_earned, _T("Kiếm được"), LVCFMT_RIGHT, 80);
	m_AccountList.InsertColumn(col_tong, _T("Đã gom"), LVCFMT_RIGHT, 75);
	m_AccountList.InsertColumn(col_perhour, _T("TB / giờ"), LVCFMT_RIGHT, 75);
	m_AccountList.InsertColumn(col_time, _T("Thời gian"), LVCFMT_RIGHT, 62);
	m_AccountList.InsertColumn(col_amulet, _T("Thổ Địa Phù"), LVCFMT_RIGHT, 72);
	m_AccountList.InsertColumn(col_death, _T("Chết"), LVCFMT_RIGHT, 40);
	m_AccountList.InsertColumn(col_region, _T("Cụm server"), LVCFMT_LEFT, 80);
	m_AccountList.InsertColumn(col_server, _T("Server"), LVCFMT_LEFT, 90);
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

	// Tab Thống kê
	SetDlgItemText(IDC_LOGIN_STATS_STATIC, _T("Cập nhật mỗi 2 giây. Tổng tiền = tiền trên người + rương (offline: số tiền lưu lần cuối). ")
		_T("Kiếm được (đã cộng tiền đã gom), Đã gom, TB / giờ, Thổ Địa Phù, Chết: tính từ lúc vào game lần này (Thổ Địa Phù chỉ đếm lần Auto dùng).\n")
		_T("Gom tiền: check các tài khoản trong bảng (bấm tiêu đề # để check / bỏ tất cả) rồi bấm nút. Góp cả tiền trên người lẫn trong rương (tiền trên người không đủ thì rút thêm từ rương, không cần mở rương), nhân vật phải online và đã gắn Auto; đang ở ngoài thành thì dùng Thổ Địa Phù về thành rồi mới góp."));
	SetDlgItemInt(IDC_LOGIN_STATS_KEEP_EDIT, m_nTongKeep, FALSE);
	for (int i = 0; i < _countof(s_ExitOpts); i++)
	{
		if (s_ExitOpts[i].nCheck)
			CheckDlgButton(s_ExitOpts[i].nCheck, ExitOptField(m_ExitOpt, s_ExitOpts[i].nOn) ? BST_CHECKED : BST_UNCHECKED);
		if (s_ExitOpts[i].nEdit)
			SetDlgItemInt(s_ExitOpts[i].nEdit, ExitOptField(m_ExitOpt, s_ExitOpts[i].nVal), FALSE);
	}
	OnExitOptionChanged(0);

	m_Tab.InsertItem(tab_setting, _T("Cài đặt"));
	m_Tab.InsertItem(tab_stats, _T("Thống kê"));
	m_Tab.InsertItem(tab_hengio, _T("Hẹn giờ"));
	UpdateHenGioButton();
	LayoutDialog();
	m_Tab.SetCurSel(tab_setting);
	ShowTab(tab_setting);

	UpdateGameStatus();
	UpdateStats();
	SetTimer(TIMER_AUTO_LOGIN, AUTO_LOGIN_INTERVAL, NULL);
	SetTimer(TIMER_LOGIN_STATS, LOGIN_STATS_INTERVAL, NULL);

	if (m_ToolTip.Create(this))
	{
		m_ToolTip.AddTool(GetDlgItem(IDC_ACCOUNT_INFO_ITEM_LIST), _T("Danh sách tài khoản - chọn một dòng để xem/sửa. Chuột phải: login dòng đó. Double click: ẩn / hiện game của dòng đó.\nÔ check ở cột #: chọn tài khoản để gom tiền lên bang và login / out theo giờ của nhóm (bấm tiêu đề # để check/bỏ tất cả)"));
		m_ToolTip.AddTool(GetDlgItem(IDC_ADD_ITEMLIST_BUTTON), _T("Thêm tài khoản đang nhập vào cuối danh sách"));
		m_ToolTip.AddTool(GetDlgItem(IDC_REMOVE_ITEMLIST_BUTTON), _T("Xóa tài khoản đang chọn"));
		m_ToolTip.AddTool(GetDlgItem(IDC_EDIT_ITEMLIST_BUTTON), _T("Ghi thông tin đang nhập vào tài khoản đang chọn"));
		m_ToolTip.AddTool(GetDlgItem(IDC_ITEMLIST_UP_BUTTON), _T("Đưa tài khoản đang chọn lên trên"));
		m_ToolTip.AddTool(GetDlgItem(IDC_ITEMLIST_DOWN_BUTTON), _T("Đưa tài khoản đang chọn xuống dưới"));
		m_ToolTip.AddTool(GetDlgItem(IDC_AUTO_RELOGIN_CHECK), _T("Riêng tài khoản này: sau LOGIN / LOGIN ALL, chưa vào game (mất kết nối, Game tắt...) thì chờ số ms bên cạnh rồi đăng nhập lại"));
		m_ToolTip.AddTool(GetDlgItem(IDC_DELAY_TIME_RELOGIN_EDIT), _T("Thời gian chờ (ms) trước khi đăng nhập lại"));
		m_ToolTip.AddTool(GetDlgItem(IDC_GET_GAMEPATH_BUTTON), _T("Chọn file chạy của Game"));
		m_ToolTip.AddTool(GetDlgItem(IDC_START_LOGIN_BUTTON), _T("Đăng nhập tài khoản đang chọn"));
		m_ToolTip.AddTool(GetDlgItem(IDC_START_LOGINALL_BUTTON), _T("Lần lượt đăng nhập mọi tài khoản trong bảng"));
		m_ToolTip.AddTool(GetDlgItem(IDC_STOP_LOGIN_BUTTON), _T("Dừng đăng nhập và đăng nhập lại (không tắt Game)"));
		m_ToolTip.AddTool(GetDlgItem(IDC_OPENGAME_BUTTON), _T("Mở một Game mới cho tài khoản đang chọn"));
		m_ToolTip.AddTool(GetDlgItem(IDC_TERMINAL_GAME_BUTTON), _T("Thoát Game của tài khoản đang chọn (dừng đăng nhập tài khoản này)"));
		m_ToolTip.AddTool(GetDlgItem(IDC_TERMINAL_GAMEALL_BUTTON), _T("Dừng đăng nhập và thoát tất cả Game đang chạy"));
		m_ToolTip.AddTool(GetDlgItem(IDC_SERVER_REGION_COMBO), _T("Đọc từ UiConfig\\serverlist_.txt"));
		m_ToolTip.AddTool(GetDlgItem(IDC_AUTO_RELOGIN_COUNT_CHECK), _T("Đăng nhập thất bại thì chờ 2s rồi thử lại, tối đa số lần bên cạnh (bỏ check: không thử lại)"));
		m_ToolTip.AddTool(GetDlgItem(IDC_COUNT_RELOGIN_EDIT), _T("Số lần thử lại khi đăng nhập thất bại"));
		m_ToolTip.AddTool(GetDlgItem(IDC_HIDEGAME_LOGIN_CHECK), _T("Auto Login đăng nhập vào game xong thì ẩn cửa sổ Game (bấm \"Mở game\" để hiện lại)"));
		m_ToolTip.AddTool(GetDlgItem(IDC_AUTO_LOGINALL_CHECK), _T("Mở Auto xong chờ số ms bên cạnh rồi tự login các tài khoản bật \"Tự động đăng nhập lại\" (chỉ một lần mỗi lần mở Auto)"));
		m_ToolTip.AddTool(GetDlgItem(IDC_DELAYTIME_LOGINALL_EDIT), _T("Thời gian chờ (ms) sau khi mở Auto, tối thiểu 2000"));
		m_ToolTip.AddTool(GetDlgItem(IDC_WAIT_OPENGAME_LOGIN_EDIT), _T("ms chờ sau khi mở Game rồi mới xem Game đang ở màn hình nào (mặc định 3000, tối thiểu 500)"));
		m_ToolTip.AddTool(GetDlgItem(IDC_WAIT_JOINGAME_LOGIN_EDIT2), _T("ms chờ sau khi bấm Bắt Đầu Trò Chơi (mặc định 2000, tối thiểu 500)"));
		m_ToolTip.AddTool(GetDlgItem(IDC_WAIT_NOTICE_LOGIN_EDIT6), _T("ms chờ sau khi bấm Xác định ở bảng Nhắc nhở (15_10, mặc định 2000, tối thiểu 500)"));
		m_ToolTip.AddTool(GetDlgItem(IDC_WAIT_CHOSESERVER_LOGIN_EDIT3), _T("ms chờ sau khi chọn cụm + server và bấm Đăng nhập (mặc định 4000, tối thiểu 500).\nChờ ngắn quá mà máy chủ chưa kịp chuyển màn hình thì Auto bấm lại; quá 3 lần sẽ báo lỗi"));
		m_ToolTip.AddTool(GetDlgItem(IDC_WAIT_INPUT_ACCPASS_LOGIN_EDIT4), _T("ms chờ sau khi nhập tài khoản + mật khẩu và bấm Đăng nhập (mặc định 6000, tối thiểu 500).\nChờ ngắn quá mà máy chủ chưa kịp trả lời thì Auto đăng nhập lại; quá 3 lần sẽ báo lỗi"));
		m_ToolTip.AddTool(GetDlgItem(IDC_WAIT_CHOSEROLE_LOGIN_EDIT5), _T("ms chờ sau khi chọn nhân vật và vào game (mặc định 8000, tối thiểu 500)"));
		m_ToolTip.AddTool(GetDlgItem(IDC_START_HENGIO_LOGIN_BUTTON), _T("Bật / tắt hẹn giờ theo nhóm: tới giờ login của nhóm thì login các tài khoản được check trong nhóm, tới giờ out thì thoát game các tài khoản đó"));
		m_ToolTip.AddTool(GetDlgItem(IDC_ADD_HENGIO_BUTTON), _T("Thêm nhóm vào vị trí đã nhập (bỏ trống vị trí: cuối bảng). Nhóm quản lý các tài khoản phía dưới tới nhóm kế tiếp"));
		m_ToolTip.AddTool(GetDlgItem(IDC_DELETE_HENGIO_BUTTON), _T("Xóa nhóm đang chọn (không xóa tài khoản trong nhóm)"));
		m_ToolTip.AddTool(GetDlgItem(IDC_EDIT_HENGIO_BUTTON), _T("Ghi tên, vị trí, giờ đang nhập vào nhóm đang chọn"));
		m_ToolTip.AddTool(GetDlgItem(IDC_NUMPOS_HENGIO_EDIT), _T("Số thứ tự của dòng nhóm trong bảng (cột #)"));
		m_ToolTip.AddTool(GetDlgItem(IDC_HOUR_LOGIN_HENGIO_EDIT), _T("Giờ : phút login (bỏ trống: không hẹn login)"));
		m_ToolTip.AddTool(GetDlgItem(IDC_HOUR_OUTGAME_HENGIO_EDIT2), _T("Giờ : phút thoát game (bỏ trống: không hẹn thoát)"));
		m_ToolTip.SetMaxTipWidth(400);
		m_ToolTip.AddTool(GetDlgItem(IDC_AUTO_RELOGIN_LOOP_CHECK), _T("Tài khoản có \"Tự động đăng nhập lại\" mà Game bị văng / tắt / crash (đã từng vào game) thì login lại sau thời gian chờ của nó, không cần LOGIN ALL.\nĐang LOGIN ALL thì login lại cả tài khoản đăng nhập thất bại. STOP / Tắt game: ngừng login lại tới khi tài khoản vào game lần nữa"));
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
	if (!::IsChild(m_PageSetting.m_hWnd, pMsg->hwnd) && !::IsChild(m_PageStats.m_hWnd, pMsg->hwnd) && !::IsChild(m_PageHenGio.m_hWnd, pMsg->hwnd))
		RelayToolTip(pMsg);	// Control trong trang: trang đã chuyển
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
	UpdateStats();
	SetWindowPos(NULL, AUTO_LOGIN_X, AUTO_LOGIN_Y, 0, 0, SWP_NOSIZE | SWP_NOZORDER);	// Luôn mở ở vị trí AUTO_LOGIN_X, AUTO_LOGIN_Y
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
	m_bHenGio = GetPrivateProfileInt(_T("AutoLogin"), _T("HenGio"), 0, m_strDataFile) != 0;
	for (int i = 0; i < _countof(s_ExitOpts); i++)
	{
		const EXIT_OPT_CTRL& ctrl = s_ExitOpts[i];
		if (ctrl.nOn >= 0)
			ExitOptField(m_ExitOpt, ctrl.nOn) = GetPrivateProfileInt(_T("AutoLogin"), ctrl.szOnKey, ctrl.nDefOn, m_strDataFile) != 0;
		if (ctrl.nVal >= 0)
			ExitOptField(m_ExitOpt, ctrl.nVal) = max((int)GetPrivateProfileInt(_T("AutoLogin"), ctrl.szValKey, ctrl.nDefVal, m_strDataFile), ctrl.nMinVal);
	}
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
		if (GetPrivateProfileInt(strSection, _T("Group"), 0, m_strDataFile))
		{
			acc.bGroup = TRUE;
			acc.bChecked = FALSE;
			GetPrivateProfileString(strSection, _T("Name"), _T(""), szValue, 2048, m_strDataFile);
			acc.strGroupName = szValue;
			GetPrivateProfileString(strSection, _T("Login"), _T(""), szValue, 2048, m_strDataFile);
			acc.nLoginMin = ParseDayMinute(szValue);
			GetPrivateProfileString(strSection, _T("Out"), _T(""), szValue, 2048, m_strDataFile);
			acc.nOutMin = ParseDayMinute(szValue);
			acc.nId = ++m_nNextId;
			m_Accounts.push_back(acc);
			continue;
		}
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
	strData.AppendFormat(_T("TongKeep=%d\r\nHenGio=%d\r\n"), m_nTongKeep, m_bHenGio);
	for (int i = 0; i < _countof(s_ExitOpts); i++)
	{
		const EXIT_OPT_CTRL& ctrl = s_ExitOpts[i];
		if (ctrl.nOn >= 0)
			strData.AppendFormat(_T("%s=%d\r\n"), ctrl.szOnKey, ExitOptField(m_ExitOpt, ctrl.nOn));
		if (ctrl.nVal >= 0)
			strData.AppendFormat(_T("%s=%d\r\n"), ctrl.szValKey, ExitOptField(m_ExitOpt, ctrl.nVal));
	}
	for (int i = 0; i < (int)m_Accounts.size(); i++)
	{
		const AutoLoginAccount& acc = m_Accounts[i];
		if (acc.bGroup)
		{
			strData.AppendFormat(_T("\r\n[Account_%d]\r\nGroup=1\r\nName=%s\r\nLogin=%s\r\nOut=%s\r\n"),
				i, (LPCTSTR)acc.strGroupName, acc.nLoginMin < 0 ? _T("") : (LPCTSTR)FormatDayMinute(acc.nLoginMin),
				acc.nOutMin < 0 ? _T("") : (LPCTSTR)FormatDayMinute(acc.nOutMin));
			continue;
		}
		strData.AppendFormat(_T("\r\n[Account_%d]\r\nAccount=%s\r\nPassword=%s\r\nRegion=%s\r\nServer=%s\r\nRole=%d\r\nChecked=%d\r\nAutoRelogin=%d\r\nReloginDelay=%d\r\n"),
			i, (LPCTSTR)acc.strAccount, (LPCTSTR)EncryptPassword(acc.strPassword),
			(LPCTSTR)acc.strRegion, (LPCTSTR)acc.strServer, acc.nRoleIndex, acc.bChecked, acc.bAutoRelogin, acc.nReloginDelay);
		if (!acc.strPlayerCfg.IsEmpty())
			strData.AppendFormat(_T("PlayerName=%s\r\nPlayerCfg=%s\r\n"), (LPCTSTR)acc.strPlayerName, (LPCTSTR)acc.strPlayerCfg);
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
	if (acc.bGroup)
	{
		// Dòng Nhóm: không có ô check, các cột tài khoản / thống kê để trống
		for (int nCol = col_account; nCol <= col_relogin; nCol++)
			m_AccountList.SetItemText(nIndex, nCol, _T(""));
		m_AccountList.SetItemText(nIndex, col_account, _T("NHÓM"));
		m_AccountList.SetItemText(nIndex, col_player, acc.strGroupName);
		m_AccountList.SetItemText(nIndex, col_status, GetStatusText(nIndex));
		m_bUpdatingList = TRUE;
		m_AccountList.SetItemState(nIndex, 0, LVIS_STATEIMAGEMASK);
		m_bUpdatingList = FALSE;
		return;
	}
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
	int nSel = m_AccountList.GetNextItem(-1, LVNI_SELECTED);
	return nSel < (int)m_Accounts.size() ? nSel : -1;	// Dòng TỔNG không phải tài khoản
}

int CDlgAutoLogin::GetSelectedAccount()
{
	int nSel = GetSelectedRow();
	return nSel >= 0 && !m_Accounts[nSel].bGroup ? nSel : -1;
}

BOOL CDlgAutoLogin::IsGroupSelected()
{
	int nSel = GetSelectedRow();
	return nSel >= 0 && m_Accounts[nSel].bGroup;
}

// Tìm tài khoản trùng (cùng tên tài khoản + cùng server), bỏ qua dòng nExcept
int CDlgAutoLogin::FindAccount(const AutoLoginAccount& acc, int nExcept)
{
	for (int i = 0; i < (int)m_Accounts.size(); i++)
	{
		if (i != nExcept && !m_Accounts[i].bGroup && m_Accounts[i].strAccount.CompareNoCase(acc.strAccount) == 0
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
	{
		if (m_Accounts[pNMLV->iItem].bGroup)
			SetGroupFields(pNMLV->iItem);
		else
			SetFields(m_Accounts[pNMLV->iItem]);
	}
	// Bấm ô check ở cột #
	if (!m_bUpdatingList && (pNMLV->uChanged & LVIF_STATE) && ((pNMLV->uNewState ^ pNMLV->uOldState) & LVIS_STATEIMAGEMASK)
		&& pNMLV->iItem >= 0 && pNMLV->iItem < (int)m_Accounts.size() && !m_Accounts[pNMLV->iItem].bGroup)
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
		if (!m_Accounts[i].bGroup && !m_Accounts[i].bChecked)
			bCheckAll = TRUE;
	for (int i = 0; i < (int)m_Accounts.size(); i++)
	{
		if (m_Accounts[i].bGroup)
			continue;
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
	UpdateStats();
}

void CDlgAutoLogin::OnBnClickedRemove()
{
	if (IsGroupSelected())
	{
		MessageBox(_T("Nhóm chỉ xóa / sửa được ở tab Hẹn giờ"), _T("Auto Login"), MB_OK | MB_ICONWARNING);
		return;
	}
	int nSel = GetSelectedAccount();
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
	UpdateStats();
}

void CDlgAutoLogin::OnBnClickedEdit()
{
	if (IsGroupSelected())
	{
		MessageBox(_T("Nhóm chỉ xóa / sửa được ở tab Hẹn giờ"), _T("Auto Login"), MB_OK | MB_ICONWARNING);
		return;
	}
	int nSel = GetSelectedAccount();
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
	if (cur.bBadLogin)
	{
		cur.bBadLogin = FALSE;	// Đã sửa thông tin: cho login lại
		cur.strError.Empty();
	}
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
	if (acc.dwProcessId && (!IsGameProcess(acc.dwProcessId) || IsClosingGame(acc.dwProcessId)))
		acc.dwProcessId = 0;	// Game đang tắt không còn thuộc tài khoản: mở Game mới ngay, không chờ

	for (int i = 0; g_pCharList && i < g_pCharList->GetItemCount(); i++)
	{
		GameWindow* pGameWin = (GameWindow*)g_pCharList->GetItemData(i);
		if (pGameWin == NULL || pGameWin->m_hWnd == NULL)
			continue;
		DWORD dwPID = 0;
		GetWindowThreadProcessId(pGameWin->m_hWnd, &dwPID);
		if (dwPID == 0 || IsClosingGame(dwPID))
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

// Đóng Game: ẩn cửa sổ ngay và không còn tính là Game của tài khoản (được mở Game mới ngay, không chờ Game cũ tắt).
// Game chưa vào thế giới game: TerminateProcess luôn. Đang trong game: gửi WM_CLOSE để thoát êm,
// sau GAME_CLOSE_TIMEOUT mà chưa tắt thì TerminateProcess (CheckClosingGames)
void CDlgAutoLogin::CloseGames(const std::vector<DWORD>& PIDs)
{
	DWORD dwNow = GetTickCount();
	for (int i = 0; i < (int)PIDs.size(); i++)
	{
		if (!IsGameProcess(PIDs[i]))
			continue;
		for (int j = 0; j < (int)m_Accounts.size(); j++)
			if (m_Accounts[j].dwProcessId == PIDs[i])
				m_Accounts[j].dwProcessId = 0;
		BOOL bInGame = FALSE;
		HANDLE hGame = OpenProcess(PROCESS_VM_READ, FALSE, PIDs[i]);
		if (hGame)
		{
			bInGame = ReadGameDword(hGame, LG_SMC_BASE_ADD) != 0;
			CloseHandle(hGame);
		}
		BOOL bPosted = FALSE;
		HWND hWnd = NULL;
		while ((hWnd = ::FindWindowEx(NULL, hWnd, VLTK_WINCLASS, NULL)) != NULL)
		{
			DWORD dwWndPID = 0;
			GetWindowThreadProcessId(hWnd, &dwWndPID);
			if (dwWndPID != PIDs[i])
				continue;
			::ShowWindowAsync(hWnd, SW_HIDE);
			if (bInGame && ::PostMessage(hWnd, WM_CLOSE, 0, 0))
				bPosted = TRUE;
		}
		if (!bPosted)
		{
			TerminateGame(PIDs[i]);	// Chưa vào game / chưa có cửa sổ để gửi WM_CLOSE
			ClosingGame closing = { PIDs[i], dwNow };	// Theo dõi tới khi process kết thúc hẳn
			m_Closing.push_back(closing);
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

BOOL CDlgAutoLogin::IsClosingGame(DWORD dwPID)
{
	for (int i = 0; dwPID && i < (int)m_Closing.size(); i++)
		if (m_Closing[i].dwPID == dwPID)
			return TRUE;
	return FALSE;
}

void CDlgAutoLogin::CheckClosingGames()
{
	DWORD dwNow = GetTickCount();
	for (int i = (int)m_Closing.size() - 1; i >= 0; i--)
	{
		// Giữ trong danh sách tới khi process kết thúc hẳn (để Game đang tắt không bị nhận lại là Game của tài khoản)
		if (!IsGameProcess(m_Closing[i].dwPID))
			m_Closing.erase(m_Closing.begin() + i);
		else if ((int)(dwNow - m_Closing[i].dwDeadline) >= 0)
			TerminateGame(m_Closing[i].dwPID);	// Game không phản hồi WM_CLOSE
	}
}

// Nội dung cột Trạng thái
CString CDlgAutoLogin::GetStatusText(int nIndex)
{
	AutoLoginAccount& acc = m_Accounts[nIndex];
	CString strStatus;
	if (acc.bGroup)	// "giờ login"-"giờ out game"
		return FormatDayMinute(acc.nLoginMin) + _T("-") + FormatDayMinute(acc.nOutMin);
	if (m_Job.nId && m_Job.nId == acc.nId)
		return m_Job.strStatus;
	if (m_dwStartAllTick && acc.bAutoRelogin && !IsAccountInGame(nIndex))
	{
		int nLeft = (int)(m_dwStartAllTick - GetTickCount());
		strStatus.Format(_T("Tự LOGIN ALL sau %ds"), (max(nLeft, 0) + 999) / 1000);
		return strStatus;
	}
	if (acc.bBadLogin && !IsAccountInGame(nIndex))
		return _T("Lỗi: ") + acc.strError + _T(" - sửa thông tin tài khoản để login lại");
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
		// Tọa độ + tên map, vd "192,200 - Lưỡng Thủy Động" (tên nhân vật ở cột Nhân vật)
		if (pGameWin && pGameWin->m_bLoadDone && pGameWin->m_dwLoginTick)
			strStatus.Format(_T("%d,%d - %s"), pGameWin->m_nOffX >> 8, pGameWin->m_nOffY >> 9, (LPCTSTR)GetMapName(pGameWin->m_nMapID));
		else
			strStatus = _T("Trong game");
		return strStatus;
	}

	if (m_bHenGio && acc.bHenGioActive && acc.dwHenGioOffTick)
	{
		int nLeft = acc.nReloginDelay - (int)(GetTickCount() - acc.dwHenGioOffTick);
		strStatus.Format(_T("Hẹn giờ: login lại sau %ds"), (max(nLeft, 0) + 999) / 1000);
		if (!acc.strError.IsEmpty())
			strStatus = _T("Lỗi: ") + acc.strError + _T(" - ") + strStatus;
		return strStatus;
	}
	if ((m_bRunAll || acc.bSeenInGame) && IsDlgButtonChecked(IDC_AUTO_RELOGIN_LOOP_CHECK) == BST_CHECKED && acc.bAutoRelogin && acc.dwOfflineTick)
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
	int nSel = GetSelectedAccount();
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
	int nSel = GetSelectedAccount();
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
		if (!m_Accounts[i].bGroup)
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
	if (acc.bGroup)
		return;
	acc.bHenGioOut = FALSE;	// Được login lại (LOGIN / LOGIN ALL / tới giờ login của nhóm)
	if (FindLoginEntry(acc.nId) >= 0 || m_Job.nId == acc.nId || acc.bBadLogin)
		return;
	AutoLoginEntry entry;
	entry.nId = acc.nId;
	entry.strAccount = acc.strAccount;
	entry.nFails = 0;
	entry.dwNextTick = 0;
	acc.strError.Empty();
	g_AutoLoginList.push_back(entry);
}

// Bước 2: làm trống danh sách cần login rồi thêm mọi tài khoản theo thứ tự trên bảng (ô check không dùng cho login).
// Vòng lặp (!bFirst): chỉ lấy lại tài khoản có "Tự động đăng nhập lại" và đã không ở trong game quá ReloginDelay;
// ngoài LOGIN ALL thì chỉ tài khoản đã từng vào game (Game bị văng / tắt / crash)
void CDlgAutoLogin::BuildLoginList(BOOL bFirst, BOOL bAutoOnly)
{
	g_AutoLoginList.clear();
	DWORD dwNow = GetTickCount();
	for (int i = 0; i < (int)m_Accounts.size(); i++)
	{
		AutoLoginAccount& acc = m_Accounts[i];
		if (acc.bGroup)
			continue;
		BOOL bInGame = IsAccountInGame(i);
		if (bInGame)
			acc.bSeenInGame = TRUE;
		if (bAutoOnly && !acc.bAutoRelogin)
			continue;	// Tự LOGIN ALL lúc mở Auto: chỉ tài khoản bật "Tự động đăng nhập lại"
		if (!bFirst)
		{
			if (!acc.bAutoRelogin || bInGame || (!m_bRunAll && !acc.bSeenInGame) || acc.bHenGioOut)
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

// bWarn = FALSE: tự LOGIN ALL lúc mở Auto ("Tự động login all sau"), chỉ login tài khoản bật "Tự động đăng nhập lại"
void CDlgAutoLogin::StartLoginAll(BOOL bWarn)
{
	int nCount = 0;
	for (int i = 0; i < (int)m_Accounts.size(); i++)
		if (!m_Accounts[i].bGroup && (bWarn || m_Accounts[i].bAutoRelogin))
			nCount++;
	m_dwStartAllTick = 0;	// Tự LOGIN ALL chỉ một lần
	if (nCount == 0)
	{
		if (bWarn)
			MessageBox(_T("Danh sách chưa có tài khoản nào"), _T("Auto Login"), MB_OK | MB_ICONWARNING);
		return;
	}
	m_bRunAll = TRUE;
	m_Job.nId = 0;
	BuildLoginList(TRUE, !bWarn);
	RemoveOnlineEntries();
	UpdateGameStatus();
}

// Bỏ một tài khoản khỏi danh sách cần login (không tắt Game)
void CDlgAutoLogin::StopAccount(int nIndex)
{
	AutoLoginAccount& acc = m_Accounts[nIndex];
	acc.dwOfflineTick = 0;
	acc.bSeenInGame = FALSE;	// Không tự login lại tới khi vào game lần nữa
	acc.bHenGioActive = FALSE;	// Hẹn giờ cũng không login lại tới giờ login kế tiếp
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
	{
		m_Accounts[i].dwOfflineTick = 0;
		m_Accounts[i].bSeenInGame = FALSE;	// Không tự login lại tới khi vào game lần nữa
		m_Accounts[i].bHenGioActive = FALSE;
	}
	UpdateGameStatus();
}

// LOGIN: thêm tài khoản đang chọn vào danh sách cần login
void CDlgAutoLogin::OnBnClickedStartLogin()
{
	int nSel = GetSelectedAccount();
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
	LoginLog(_T("Kết thúc lượt: %s"), szError ? szError : _T("thành công"));
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
	if (!szError)
		acc.bSeenInGame = TRUE;
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
	LoginLog(_T("Màn hình %d lần %d: %s (chờ %d ms, %d ms từ đầu lượt)"), nScreen, m_Job.nTries, szStatus, dwWait, GetTickCount() - m_Job.dwStartTick);
	return m_Job.nTries <= LOGIN_MAX_TRIES;
}

// Nhật ký đăng nhập <Auto>\AutoLogin.log (UTF-8) để dò lỗi kẹt màn hình
static void LoginLog(LPCTSTR szFormat, ...)
{
	CString strLine;
	va_list args;
	va_start(args, szFormat);
	strLine.FormatV(szFormat, args);
	va_end(args);
	SYSTEMTIME st;
	GetLocalTime(&st);
	CString strOut;
	strOut.Format(_T("%02d:%02d:%02d.%03d %s\r\n"), st.wHour, st.wMinute, st.wSecond, st.wMilliseconds, (LPCTSTR)strLine);
	TCHAR szFile[MAX_PATH];
	_stprintf_s(szFile, _T("%s\\AutoLogin.log"), g_szAppFolder);
	HANDLE hFile = CreateFile(szFile, FILE_APPEND_DATA, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hFile == INVALID_HANDLE_VALUE)
		return;
	CT2A szUtf8(strOut, CP_UTF8);
	DWORD dwWritten;
	WriteFile(hFile, (LPCSTR)szUtf8, (DWORD)strlen(szUtf8), &dwWritten, NULL);
	CloseHandle(hFile);
}

static void LogGameScreens(HANDLE hGame, LPCTSTR szWhat)
{
	static const DWORD s_dwPtrs[] = { LG_UIN_BASE_ADD, LG_NOTICE_SELF, LG_SEL_SELF, LG_LOGIN_SELF, LG_ROLE_SELF, LG_SMC_BASE_ADD };
	CString str;
	for (int i = 0; i < _countof(s_dwPtrs); i++)
	{
		DWORD dwWnd = ReadGameDword(hGame, s_dwPtrs[i]);
		str.AppendFormat(_T(" %08X/%08X"), dwWnd, dwWnd && i < 5 ? ReadGameDword(hGame, dwWnd + 4) : 0);
	}
	LoginLog(_T("PID %d %s: init notice sel login role smc =%s"), GetProcessId(hGame), szWhat, (LPCTSTR)str);
}

// Chạy mã thao tác trong Game. Quá REMOTE_TIMEOUT mà thread chưa chạy xong (Game đang bận, vd lần mở đầu còn nạp
// dữ liệu nên thread mới phải chờ) => giữ thread lại, RunLoginStep chờ nó xong mới thao tác tiếp (CheckJobRemote)
BOOL CDlgAutoLogin::ExecJobCode(HANDLE hGame, LPVOID pRemote, const CRemoteCode& code, DWORD* pdwExit)
{
	HANDLE hPending = NULL;
	DWORD dwTick = GetTickCount();
	BOOL bOK = ExecRemoteCode(hGame, pRemote, code, REMOTE_TIMEOUT, pdwExit, &hPending);
	LoginLog(_T("  %s: chạy mã %s sau %d ms, exit %d (GetLastError %d)"), (LPCTSTR)m_Job.strStatus,
		bOK ? _T("xong") : hPending ? _T("CHƯA XONG") : _T("LỖI"), GetTickCount() - dwTick, *pdwExit, GetLastError());
	if (bOK)
	{
		LogGameScreens(hGame, _T("sau thao tác"));
		return TRUE;
	}
	if (hPending)
	{
		if (m_Job.hRemote)
			CloseHandle(m_Job.hRemote);	// Thread cũ ở Game khác (đã bỏ lượt đó)
		m_Job.hRemote = hPending;
		m_Job.pRemote = pRemote;
		m_Job.dwRemotePID = GetProcessId(hGame);
		m_Job.strStatus = _T("Chờ game phản hồi");
	}
	return FALSE;
}

// Thread của ExecJobCode còn chạy => TRUE. Đã xong (hoặc Game đã tắt) => xóa + giải phóng vùng nhớ mã, FALSE
BOOL CDlgAutoLogin::CheckJobRemote()
{
	if (m_Job.hRemote == NULL)
		return FALSE;
	if (WaitForSingleObject(m_Job.hRemote, 0) == WAIT_TIMEOUT)
		return TRUE;
	CloseHandle(m_Job.hRemote);
	m_Job.hRemote = NULL;
	LoginLog(_T("  Mã chờ trong Game PID %d đã chạy xong"), m_Job.dwRemotePID);
	HANDLE hGame = OpenProcess(PROCESS_VM_WRITE | PROCESS_VM_OPERATION, FALSE, m_Job.dwRemotePID);
	if (hGame)
	{
		BYTE buf[CRemoteCode::TOTAL_SIZE] = { 0 };	// Mã có thể chứa mật khẩu
		WriteGameMemory(hGame, m_Job.pRemote, buf, sizeof(buf));
		VirtualFreeEx(hGame, m_Job.pRemote, 0, MEM_RELEASE);
		CloseHandle(hGame);
	}
	m_Job.pRemote = NULL;
	m_Job.dwActionTick = GetTickCount();	// Thời gian chờ sau thao tác tính từ lúc nó thực sự chạy
	return FALSE;
}

// Đăng nhập lần lượt danh sách cần login; hết danh sách mà đang LOGIN ALL có check vòng lặp thì lấy lại danh sách (bước 2)
void CDlgAutoLogin::RunLogin()
{
	CheckJobRemote();
	if (m_dwStartAllTick && (int)(GetTickCount() - m_dwStartAllTick) >= 0)
		StartLoginAll(FALSE);

	if (m_Job.nId == 0)
	{
		while (!g_AutoLoginList.empty() && FindAccountById(g_AutoLoginList.front().nId) < 0)
			g_AutoLoginList.erase(g_AutoLoginList.begin());
		if (g_AutoLoginList.empty())
		{
			if (IsDlgButtonChecked(IDC_AUTO_RELOGIN_LOOP_CHECK) != BST_CHECKED)
			{
				m_bRunAll = FALSE;	// Đã login xong danh sách
				return;
			}
			BuildLoginList(FALSE);	// Lặp lại: login lại tài khoản bị văng game (có hay không LOGIN ALL)
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
		m_Job.strGameInfo.Empty();
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
	BOOL bWaiting = dwNow - m_Job.dwActionTick < m_Job.dwWait;	// Đang chờ sau thao tác: chỉ đọc bảng thông báo

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
		if (bWaiting)
			return;
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
		if (bWaiting)
			return;
		CString strError;
		strError.Format(_T("Không truy cập được Game (GetLastError = %d)"), GetLastError());
		EndJob(nIndex, strError, TRUE, FALSE);
		return;
	}
	if (bWaiting)
		HandleConnectInfo(nIndex, hGame);
	else
		RunLoginStep(nIndex, hGame);
	CloseHandle(hGame);
}

// Bảng thông báo lúc kết nối / đăng nhập (KUiConnectInfo) đang hiện => TRUE, nhận nội dung (bỏ ký tự điều khiển)
// và bError = thông báo lỗi đang chờ bấm Xác định (sai mật khẩu, tài khoản đang online, server bảo trì...)
static BOOL ReadConnectInfo(HANDLE hGame, CString& strMsg, BOOL& bError)
{
	strMsg.Empty();
	bError = FALSE;
	DWORD dwWnd = GetVisibleGameWnd(hGame, LG_CI_SELF);
	if (dwWnd == 0)
		return FALSE;
	char szMsg[LG_CI_MESSAGE_SIZE + 1] = { 0 };
	ReadProcessMemory(hGame, (LPCVOID)(UINT_PTR)(dwWnd + LG_CI_MESSAGE), szMsg, LG_CI_MESSAGE_SIZE, NULL);
	int nLen = (int)ReadGameDword(hGame, dwWnd + LG_CI_MSGLEN);
	if (nLen >= 0 && nLen < LG_CI_MESSAGE_SIZE)
		szMsg[nLen] = 0;
	for (int i = 0; szMsg[i]; i++)
		if ((BYTE)szMsg[i] < 0x20)
			szMsg[i] = ' ';
	WCHAR szText[LG_CI_MESSAGE_SIZE + 1];
	TCVN3ToUnicode(szMsg, szText, LG_CI_MESSAGE_SIZE);
	strMsg = szText;
	strMsg.Trim();
	while (strMsg.Replace(_T("  "), _T(" ")))
		;
	int nDesire = (int)ReadGameDword(hGame, dwWnd + LG_CI_DESIRE);
	bError = nDesire == 0 || nDesire >= LG_CI_NS_FIRST;
	return TRUE;
}

// Bảng thông báo của Game, kiểm tra ở cả 2 giai đoạn:
//	1. Tài khoản vào server (sau khi bấm Đăng nhập: kết nối, sai mật khẩu, đang được sử dụng, bị khóa...)
//	2. Nhân vật vào thế giới game (sau khi chọn nhân vật: kết nối game server, server đầy / bảo trì...)
// Đã xử lý (lỗi => kết thúc lượt; đang kết nối => chờ) thì TRUE
BOOL CDlgAutoLogin::HandleConnectInfo(int nIndex, HANDLE hGame)
{
	CString strInfo;
	BOOL bInfoError = FALSE;
	if (!ReadConnectInfo(hGame, strInfo, bInfoError))
		return FALSE;
	BOOL bRoleStage = m_Job.nScreen == scr_role || (int)ReadGameDword(hGame, LG_LOGIC + LG_LOGIC_STATUS) >= LG_LOGIC_STATUS_ROLES;
	if (!strInfo.IsEmpty())
		strInfo = (bRoleStage ? _T("Nhân vật vào game: ") : _T("Tài khoản vào server: ")) + strInfo;
	if (strInfo != m_Job.strGameInfo)
	{
		m_Job.strGameInfo = strInfo;
		LoginLog(_T("  Thông báo của Game%s: %s"), bInfoError ? _T(" (lỗi)") : _T(""), (LPCTSTR)strInfo);
	}
	if (bInfoError && strInfo.Find(_T("Mật khẩu không đúng")) >= 0)
	{
		// Mã 5 "Tài khoản hoặc Mật khẩu không đúng!": tắt Game, không login tài khoản này tới khi sửa thông tin
		m_Accounts[nIndex].bBadLogin = TRUE;
		EndJob(nIndex, strInfo, FALSE, TRUE);
		return TRUE;
	}
	if (bInfoError)
	{
		// Không bấm Xác định: đóng Game, lần sau mở lại từ đầu
		EndJob(nIndex, strInfo.IsEmpty() ? CString(_T("Game báo lỗi khi đăng nhập")) : strInfo, TRUE, TRUE);
		return TRUE;
	}
	if (strInfo.IsEmpty())
		return FALSE;
	m_Job.strStatus = strInfo;	// Đang kết nối / đang lấy dữ liệu nhân vật / đang vào game...
	return TRUE;
}

// Một bước đăng nhập theo màn hình Game đang hiện
void CDlgAutoLogin::RunLoginStep(int nIndex, HANDLE hGame)
{
	if (m_Job.hRemote && m_Job.dwRemotePID == GetProcessId(hGame))
	{
		m_Job.strStatus = _T("Chờ game phản hồi");	// Thao tác trước chưa chạy xong: không bấm thêm, không tính lần thử
		return;
	}
	CString strError;
	if (ReadGameDword(hGame, LG_SMC_BASE_ADD))
	{
		EndJob(nIndex, NULL, FALSE, FALSE);	// Đã vào thế giới game
		return;
	}

	if (HandleConnectInfo(nIndex, hGame))
		return;

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
			ExecJobCode(hGame, pRemote, code, &dwExit);
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
			ExecJobCode(hGame, pRemote, code, &dwExit);
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
		ExecJobCode(hGame, pRemote, code, &dwExit);
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
	ExecJobCode(hGame, pRemote, code, &dwExit);
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
	BOOL bOK = ExecJobCode(hGame, pRemote, code, &dwExit) ? dwExit != 0
		: (m_Job.hRemote && m_Job.dwRemotePID == GetProcessId(hGame));	// Game đang bận: chờ thread chạy xong
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
	ExecJobCode(hGame, pRemote, code, &dwExit);
	return TRUE;
}

void CDlgAutoLogin::OnTimer(UINT_PTR nIDEvent)
{
	if (nIDEvent == TIMER_AUTO_LOGIN)
	{
		CheckClosingGames();
		CheckExitGames();
		CheckHenGio();
		RunLogin();
		UpdateGameStatus();
		UpdatePlayerNames();
		return;
	}
	if (nIDEvent == TIMER_LOGIN_STATS)
	{
		if (IsWindowVisible())
			UpdateStats();
		return;
	}
	CDialog::OnTimer(nIDEvent);
}

// Cửa sổ Game của tiến trình dwPID (NULL = chưa có)
static HWND FindGameWnd(DWORD dwPID)
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

// Cài đặt thoát (tab Thống kê): Game của tài khoản bị treo / mất kết nối / không tăng kinh nghiệm / quanh quẩn 1 điểm
// quá thời gian đặt thì thoát game (check Lặp lại và tài khoản bật Login lại thì Auto Login tự đăng nhập lại)
void CDlgAutoLogin::CheckExitGames()
{
	DWORD dwNow = GetTickCount();
	if (dwNow - m_dwExitCheckTick < EXIT_CHECK_INTERVAL)
		return;
	m_dwExitCheckTick = dwNow;
	const AutoLoginExitOpt& opt = m_ExitOpt;
	if (!opt.bLost && !opt.bNoExp && !opt.bHung && !opt.bStay)
		return;

	for (int i = 0; i < (int)m_Accounts.size(); i++)
	{
		AutoLoginAccount& acc = m_Accounts[i];
		if (acc.bGroup)
			continue;
		GameWindow* pGameWin = NULL;
		DWORD dwPID = GetAccountGame(i, &pGameWin);
		BOOL bSkip = dwPID == 0 || (m_Job.nId && m_Job.nId == acc.nId);	// Đang đăng nhập: Auto Login tự xử lý
		for (int j = 0; dwPID && j < (int)m_Closing.size(); j++)
			bSkip |= m_Closing[j].dwPID == dwPID;
		if (bSkip || dwPID != acc.dwExitPID)
			acc.ResetExitTrack(dwPID);
		if (bSkip)
			continue;
		HANDLE hGame = OpenProcess(PROCESS_VM_READ, FALSE, dwPID);
		if (hGame == NULL)
			continue;

		// Treo: cửa sổ Game không xử lý message
		HWND hWnd = pGameWin ? pGameWin->m_hWnd : FindGameWnd(dwPID);
		if (hWnd && IsHungAppWindow(hWnd))
		{
			if (!acc.dwHungTick)
				acc.dwHungTick = dwNow;
		}
		else
			acc.dwHungTick = 0;

		// Kết nối: đang trong thế giới game và server còn đồng bộ nhân vật
		BOOL bConnected = FALSE;
		if (ReadGameDword(hGame, LG_SMC_BASE_ADD) && pGameWin && pGameWin->m_bLoadDone && pGameWin->m_dwLoginTick)
		{
			DWORD dwNpc = ReadGameDword(hGame, NPC_BASE_ADD) + pGameWin->m_nPlayerIndex * NPC_DATA_SIZE;
			DWORD dwID = ReadGameDword(hGame, dwNpc + offsetof(KNpc, m_dwID));
			int nSync = (int)ReadGameDword(hGame, dwNpc + offsetof(KNpc, m_SyncSignal));
			DWORD dwWorld = ReadGameDword(hGame, WLD_BASE_ADD);
			int nTime = dwWorld ? (int)ReadGameDword(hGame, dwWorld + WLD_TIME_OFFSET) : 0;
			bConnected = dwID && nSync && dwWorld && (nTime - nSync) / 18 < NET_LOST_SEC;
		}
		if (bConnected)
		{
			acc.bWasInGame = TRUE;
			acc.dwLostTick = 0;
		}
		else if (acc.bWasInGame && !acc.dwLostTick)
			acc.dwLostTick = dwNow;

		// Kinh nghiệm / vị trí: chỉ đếm khi đang kết nối
		if (bConnected)
		{
			__int64 nExp = 0;
			DWORD dwPlayer = ReadGameDword(hGame, PLR_BASE_ADD);
			if (dwPlayer && ReadProcessMemory(hGame, (LPCVOID)(UINT_PTR)(dwPlayer + PLAYER_OFFSET + PLR_EXP_OFFSET), &nExp, sizeof(nExp), NULL)
				&& (!acc.dwExpTick || nExp != acc.nExpLast))
			{
				acc.nExpLast = nExp;
				acc.dwExpTick = dwNow;
			}

			if (!opt.bStayFight && pGameWin->m_FightMode)
				acc.dwStayTick = 0;
			else
			{
				__int64 dx = pGameWin->m_nOffX - acc.nStayX, dy = pGameWin->m_nOffY - acc.nStayY;
				if (!acc.dwStayTick || pGameWin->m_nMapID != acc.nStayMap || dx * dx + dy * dy > (__int64)opt.nStayDist * opt.nStayDist)
				{
					acc.nStayMap = pGameWin->m_nMapID;
					acc.nStayX = pGameWin->m_nOffX;
					acc.nStayY = pGameWin->m_nOffY;
					acc.dwStayTick = dwNow;
				}
			}
		}
		else
			acc.dwExpTick = acc.dwStayTick = 0;
		CloseHandle(hGame);

		CString strReason;
		if (opt.bHung && acc.dwHungTick && dwNow - acc.dwHungTick >= (DWORD)opt.nHungSec * 1000)
			strReason.Format(_T("game treo %d giây"), opt.nHungSec);
		else if (opt.bLost && acc.dwLostTick && dwNow - acc.dwLostTick >= (DWORD)opt.nLostMin * 60000)
			strReason.Format(_T("mất kết nối %d phút"), opt.nLostMin);
		else if (opt.bNoExp && acc.dwExpTick && dwNow - acc.dwExpTick >= (DWORD)opt.nNoExpMin * 60000)
			strReason.Format(_T("không tăng kinh nghiệm %d phút"), opt.nNoExpMin);
		else if (opt.bStay && acc.dwStayTick && dwNow - acc.dwStayTick >= (DWORD)opt.nStayMin * 60000)
			strReason.Format(_T("quanh quẩn 1 điểm %d phút"), opt.nStayMin);
		if (strReason.IsEmpty())
			continue;

		acc.strError = _T("Đã thoát game: ") + strReason;
		LoginLog(_T("%s: %s"), (LPCTSTR)acc.strAccount, (LPCTSTR)acc.strError);
		acc.ResetExitTrack(dwPID);
		CloseGames(std::vector<DWORD>(1, dwPID));
	}
}

// Ghi nhớ nhân vật đang vào game của mỗi tài khoản (file cfg giữ số tiền để cột Tổng tiền hiện khi offline)
void CDlgAutoLogin::UpdatePlayerNames()
{
	BOOL bChanged = FALSE;
	for (int i = 0; i < (int)m_Accounts.size(); i++)
	{
		GameWindow* pGameWin = NULL;
		if (m_Accounts[i].bGroup || !GetAccountGame(i, &pGameWin) || pGameWin == NULL || !pGameWin->m_bLoadDone)
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

/////////////////////////////////////////////////////////////////////////////
// Tab Thống kê + các cột thống kê trong bảng tài khoản

// Tên bản đồ theo MapNames.ini (giống tab Cơ bản). Nhớ lại để cột Trạng thái không đọc file mỗi nhịp
static CString GetMapName(int nMapID)
{
	static std::map<int, CString> s_MapNames;
	std::map<int, CString>::const_iterator it = s_MapNames.find(nMapID);
	if (it != s_MapNames.end())
		return it->second;
	TCHAR szMapID[12], szInfo[64], szMapNameFile[MAX_PATH];
	_stprintf(szMapID, _T("%d"), nMapID);
	_stprintf(szMapNameFile, _T("%s\\Maps\\MapNames.ini"), g_szAppFolder);
	GetPrivateProfileString(_T("Map_Name"), szMapID, _T("<Unknown_Map>"), szInfo, 64, szMapNameFile);
	char szTCNV3Str[64];
	WideCharToMultiByte(CP_ACP, 0, szInfo, -1, szTCNV3Str, 64, 0, 0);
	TCVN3ToUnicode(szTCNV3Str, szInfo, 63);
	return s_MapNames[nMapID] = szInfo;
}

// Đơn vị vạn (1 vạn = 10.000 lượng): 123456789 -> "12.345 vạn"
// bDecimal: thêm 1 số lẻ (làm tròn), vd 5000 -> "0,5 vạn"
static CString FormatMoney(__int64 nMoney, BOOL bDecimal)
{
	__int64 nAbs = nMoney < 0 ? -nMoney : nMoney;
	__int64 nTenth = bDecimal ? (nAbs + 500) / 1000 : nAbs / 10000 * 10;	// Đơn vị 0,1 vạn
	CString strDigits;
	strDigits.Format(_T("%I64d"), nTenth / 10);
	CString strText;
	for (int i = 0; i < strDigits.GetLength(); i++)
	{
		if (i > 0 && (strDigits.GetLength() - i) % 3 == 0)
			strText += _T('.');
		strText += strDigits[i];
	}
	if (bDecimal)
		strText.AppendFormat(_T(",%d"), (int)(nTenth % 10));
	strText += _T(" vạn");
	return nMoney < 0 && nTenth ? _T("-") + strText : strText;
}

// Cửa sổ giữ kích thước theo dialog trong .rc, đặt ở vị trí AUTO_LOGIN_X, AUTO_LOGIN_Y.
// Trang của tab đặt ngay dưới thanh tab, rộng bằng tab
void CDlgAutoLogin::LayoutDialog()
{
	SetWindowPos(NULL, AUTO_LOGIN_X, AUTO_LOGIN_Y, 0, 0, SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
	CRect rcTab;
	m_Tab.GetWindowRect(&rcTab);
	ScreenToClient(&rcTab);
	CRect rcGap(0, 0, 0, 2);
	MapDialogRect(&rcGap);
	CDlgAutoLoginPage* Pages[] = { &m_PageSetting, &m_PageStats, &m_PageHenGio };
	for (int i = 0; i < _countof(Pages); i++)
	{
		CRect rcPage;
		Pages[i]->GetWindowRect(&rcPage);
		int dxPage = rcTab.Width() - rcPage.Width();
		Pages[i]->SetWindowPos(NULL, rcTab.left, rcTab.bottom + rcGap.bottom, rcTab.Width(), rcPage.Height(), SWP_NOZORDER | SWP_NOACTIVATE);
		if (Pages[i] != &m_PageStats)
			continue;
		// Tab Thống kê: các dòng chữ + nhóm Cài đặt thoát giãn theo bề ngang
		static const UINT s_WideCtrls[] = { IDC_LOGIN_STATS_TONG_STATIC, IDC_LOGIN_STATS_STATIC, IDC_LOGIN_EXIT_GROUP, IDC_LOGIN_EXIT_NOTE_STATIC };
		for (int j = 0; j < _countof(s_WideCtrls); j++)
		{
			CWnd* pCtrl = m_PageStats.GetDlgItem(s_WideCtrls[j]);
			CRect rc;
			pCtrl->GetWindowRect(&rc);
			m_PageStats.ScreenToClient(&rc);
			rc.right += dxPage;
			pCtrl->MoveWindow(&rc, FALSE);
		}
	}
	Invalidate();
}

void CDlgAutoLogin::ShowTab(int nTab)
{
	m_PageSetting.ShowWindow(nTab == tab_setting ? SW_SHOW : SW_HIDE);
	m_PageStats.ShowWindow(nTab == tab_stats ? SW_SHOW : SW_HIDE);
	m_PageHenGio.ShowWindow(nTab == tab_hengio ? SW_SHOW : SW_HIDE);
}

// Dialog (chính hoặc trang của tab) chứa control nID; không thấy thì trả về dialog chính
CWnd* CDlgAutoLogin::GetCtrlParent(int nID) const
{
	const CWnd* Parents[] = { this, &m_PageSetting, &m_PageStats, &m_PageHenGio };
	for (int i = 0; i < _countof(Parents); i++)
		if (Parents[i]->m_hWnd && ::GetDlgItem(Parents[i]->m_hWnd, nID))
			return const_cast<CWnd*>(Parents[i]);
	return const_cast<CDlgAutoLogin*>(this);
}

CWnd* CDlgAutoLogin::GetDlgItem(int nID) const
{
	return GetCtrlParent(nID)->CWnd::GetDlgItem(nID);
}

UINT CDlgAutoLogin::IsDlgButtonChecked(int nID) const
{
	return GetCtrlParent(nID)->CWnd::IsDlgButtonChecked(nID);
}

void CDlgAutoLogin::CheckDlgButton(int nID, UINT nCheck)
{
	GetCtrlParent(nID)->CWnd::CheckDlgButton(nID, nCheck);
}

void CDlgAutoLogin::SetDlgItemText(int nID, LPCTSTR szText)
{
	GetCtrlParent(nID)->CWnd::SetDlgItemText(nID, szText);
}

int CDlgAutoLogin::GetDlgItemText(int nID, CString& strText) const
{
	return GetCtrlParent(nID)->CWnd::GetDlgItemText(nID, strText);
}

void CDlgAutoLogin::SetDlgItemInt(int nID, UINT nValue, BOOL bSigned)
{
	GetCtrlParent(nID)->CWnd::SetDlgItemInt(nID, nValue, bSigned);
}

UINT CDlgAutoLogin::GetDlgItemInt(int nID, BOOL* pTrans, BOOL bSigned) const
{
	return GetCtrlParent(nID)->CWnd::GetDlgItemInt(nID, pTrans, bSigned);
}

/////////////////////////////////////////////////////////////////////////////
// CDlgAutoLoginPage

// Thông báo của control (lParam = HWND control) chuyển lên dialog chính, message map của CDlgAutoLogin xử lý
BOOL CDlgAutoLoginPage::OnCommand(WPARAM wParam, LPARAM lParam)
{
	if (lParam == 0)
		return CDialog::OnCommand(wParam, lParam);
	GetParent()->SendMessage(WM_COMMAND, wParam, lParam);
	return TRUE;
}

BOOL CDlgAutoLoginPage::OnNotify(WPARAM wParam, LPARAM lParam, LRESULT* pResult)
{
	*pResult = GetParent()->SendMessage(WM_NOTIFY, wParam, lParam);
	return TRUE;
}

// Tooltip của dialog chính cho control trong trang (IsDialogMessage của trang nuốt message chuột trước khi tới dialog chính)
BOOL CDlgAutoLoginPage::PreTranslateMessage(MSG* pMsg)
{
	((CDlgAutoLogin*)GetParent())->RelayToolTip(pMsg);
	return CDialog::PreTranslateMessage(pMsg);
}

void CDlgAutoLogin::OnTabSelChange(NMHDR* pNMHDR, LRESULT* pResult)
{
	ShowTab(m_Tab.GetCurSel());
	*pResult = 0;
}

void CDlgAutoLogin::SetCell(int nRow, int nCol, LPCTSTR szText)
{
	if (m_AccountList.GetItemText(nRow, nCol) != szText)
		m_AccountList.SetItemText(nRow, nCol, szText);
}

// Các cột thống kê của mỗi tài khoản + dòng TỔNG cuối bảng (cột Trạng thái của tài khoản do UpdateGameStatus cập nhật)
void CDlgAutoLogin::UpdateStats()
{
	int nCount = (int)m_Accounts.size();
	int nRows = nCount ? nCount + 1 : 0;
	m_bUpdatingList = TRUE;
	while (m_AccountList.GetItemCount() < nRows)
		m_AccountList.InsertItem(m_AccountList.GetItemCount(), _T(""));
	while (m_AccountList.GetItemCount() > nRows)
		m_AccountList.DeleteItem(m_AccountList.GetItemCount() - 1);
	m_bUpdatingList = FALSE;

	DWORD dwNow = GetTickCount();
	__int64 nTotalMoney = 0, nTotalEarned = 0, nTotalPerHour = 0;
	__int64 nTotalTong = 0;
	int nOnline = 0, nAccounts = 0, nTotalAmulet = 0, nTotalDeath = 0;
	CString strText;
	for (int i = 0; i < nCount; i++)
	{
		const AutoLoginAccount& acc = m_Accounts[i];
		if (acc.bGroup)
			continue;	// Dòng Nhóm do UpdateRow ghi
		nAccounts++;
		GameWindow* pGameWin = NULL;
		GetAccountGame(i, &pGameWin);
		BOOL bOnline = pGameWin && pGameWin->m_bLoadDone && pGameWin->m_dwLoginTick;

		CString strName = acc.strPlayerName;
		if (bOnline)
		{
			TCHAR szName[32];
			TCVN3ToUnicode(pGameWin->m_szPlayerName, szName, 31);
			strName = szName;
		}
		SetCell(i, col_player, strName);

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
				strPerHour = FormatMoney(nPerHour, TRUE);
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
		SetCell(i, col_money, strMoney);
		SetCell(i, col_earned, strEarned);
		SetCell(i, col_tong, strTong);
		SetCell(i, col_perhour, strPerHour);
		SetCell(i, col_time, strTime);
		SetCell(i, col_amulet, strAmulet);
		SetCell(i, col_death, strDeath);
	}

	if (nRows == 0)
		return;
	int nTotal = nCount;
	if (m_AccountList.GetItemState(nTotal, LVIS_STATEIMAGEMASK) != 0)
	{
		m_bUpdatingList = TRUE;
		m_AccountList.SetItemState(nTotal, 0, LVIS_STATEIMAGEMASK);	// Dòng TỔNG không có ô check
		m_bUpdatingList = FALSE;
	}
	SetCell(nTotal, col_account, _T("TỔNG"));
	strText.Format(_T("%d/%d online"), nOnline, nAccounts);
	SetCell(nTotal, col_status, strText);
	SetCell(nTotal, col_money, FormatMoney(nTotalMoney));
	SetCell(nTotal, col_earned, FormatMoney(nTotalEarned));
	SetCell(nTotal, col_tong, FormatMoney(nTotalTong));
	SetCell(nTotal, col_perhour, FormatMoney(nTotalPerHour, TRUE));
	strText.Format(_T("%d"), nTotalAmulet);
	SetCell(nTotal, col_amulet, strText);
	strText.Format(_T("%d"), nTotalDeath);
	SetCell(nTotal, col_death, strText);
}

// Đổi ô trong nhóm Cài đặt thoát: lấy giá trị (số không hợp lệ thì trả lại giá trị cũ), lưu file
void CDlgAutoLogin::OnExitOptionChanged(UINT nID)
{
	AutoLoginExitOpt& opt = m_ExitOpt;
	AutoLoginExitOpt old = opt;
	for (int i = 0; i < _countof(s_ExitOpts); i++)
	{
		const EXIT_OPT_CTRL& ctrl = s_ExitOpts[i];
		if (ctrl.nCheck)
			ExitOptField(opt, ctrl.nOn) = IsDlgButtonChecked(ctrl.nCheck) == BST_CHECKED;
		if (ctrl.nEdit)
		{
			BOOL bOK = FALSE;
			int nValue = (int)GetDlgItemInt(ctrl.nEdit, &bOK, FALSE);
			if (bOK && nValue >= ctrl.nMinVal)
				ExitOptField(opt, ctrl.nVal) = nValue;
			else if (GetFocus() != GetDlgItem(ctrl.nEdit))
				SetDlgItemInt(ctrl.nEdit, ExitOptField(opt, ctrl.nVal), FALSE);
		}
	}
	GetDlgItem(IDC_LOGIN_EXIT_LOST_EDIT)->EnableWindow(opt.bLost);
	GetDlgItem(IDC_LOGIN_EXIT_NOEXP_EDIT)->EnableWindow(opt.bNoExp);
	GetDlgItem(IDC_LOGIN_EXIT_HUNG_EDIT)->EnableWindow(opt.bHung);
	GetDlgItem(IDC_LOGIN_EXIT_STAY_EDIT)->EnableWindow(opt.bStay);
	GetDlgItem(IDC_LOGIN_EXIT_DIST_STATIC)->EnableWindow(opt.bStay);
	GetDlgItem(IDC_LOGIN_EXIT_DIST_EDIT)->EnableWindow(opt.bStay);
	GetDlgItem(IDC_LOGIN_EXIT_FIGHT_CHECK)->EnableWindow(opt.bStay);
	if (nID && memcmp(&old, &opt, sizeof(opt)) != 0)
		SaveAccounts();
}

void CDlgAutoLogin::OnKeepMoneyChanged()
{
	BOOL bOK = FALSE;
	int nKeep = (int)GetDlgItemInt(IDC_LOGIN_STATS_KEEP_EDIT, &bOK, FALSE);
	if (!bOK)
	{
		SetDlgItemInt(IDC_LOGIN_STATS_KEEP_EDIT, m_nTongKeep, FALSE);
		return;
	}
	if (nKeep != m_nTongKeep)
	{
		m_nTongKeep = nKeep;
		SaveAccounts();
	}
}

// Góp tiền (trên người + rương) của các tài khoản được check vào quỹ bang, mỗi nhân vật giữ lại m_nTongKeep vạn
void CDlgAutoLogin::OnBnClickedTongMoney()
{
	OnKeepMoneyChanged();
	int nChecked = 0;
	for (int i = 0; i < (int)m_Accounts.size(); i++)
		nChecked += m_Accounts[i].bChecked ? 1 : 0;
	if (nChecked == 0)
	{
		SetDlgItemText(IDC_LOGIN_STATS_TONG_STATIC, _T("Chưa check tài khoản nào để gom tiền"));
		return;
	}
	__int64 nKeep = (__int64)m_nTongKeep * 10000;
	CString strText;
	strText.Format(_T("Góp tiền (trên người + rương) của %d tài khoản đã check vào quỹ bang?\nMỗi nhân vật giữ lại %s (tính cả tiền trong rương).\nNhân vật ở ngoài thành sẽ dùng Thổ Địa Phù về thành trước; tiền trên người không đủ thì rút thêm từ rương."),
		nChecked, (LPCTSTR)FormatMoney(nKeep));
	if (MessageBox(strText, _T("Gom tiền lên bang"), MB_YESNO | MB_ICONQUESTION) != IDYES)
		return;

	__int64 nSent = 0;
	int nSentCount = 0;
	CString strSkip;
	for (int i = 0; i < (int)m_Accounts.size(); i++)
	{
		const AutoLoginAccount& acc = m_Accounts[i];
		if (!acc.bChecked)
			continue;
		GameWindow* pGameWin = NULL;
		GetAccountGame(i, &pGameWin);
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
	strText.Format(_T("[%s] Đã gửi lệnh góp %s (%d nhân vật)"), (LPCTSTR)CTime::GetCurrentTime().Format(_T("%H:%M:%S")),
		(LPCTSTR)FormatMoney(nSent), nSentCount);
	if (!strSkip.IsEmpty())
		strText += _T(". Bỏ qua: ") + strSkip;
	SetDlgItemText(IDC_LOGIN_STATS_TONG_STATIC, strText);
	UpdateStats();
}

/////////////////////////////////////////////////////////////////////////////
// Hẹn giờ theo nhóm (tab Hẹn giờ). Nhóm là một dòng trong bảng, quản lý các tài khoản phía dưới tới Nhóm kế tiếp:
// tới giờ login thì login các tài khoản được check trong nhóm, tới giờ out thì thoát game các tài khoản đó

// Chuột phải vào dòng tài khoản: login tài khoản đó
void CDlgAutoLogin::OnAccountListRClick(NMHDR* pNMHDR, LRESULT* pResult)
{
	LPNMITEMACTIVATE pItem = reinterpret_cast<LPNMITEMACTIVATE>(pNMHDR);
	*pResult = 0;
	int nIndex = pItem->iItem;
	if (nIndex < 0 || nIndex >= (int)m_Accounts.size() || m_Accounts[nIndex].bGroup)
		return;
	SelectRow(nIndex);
	AddLoginEntry(nIndex);
	RemoveOnlineEntries();
	UpdateGameStatus();
}

// Double click vào dòng tài khoản: ẩn / hiện cửa sổ Game của tài khoản đó
// (ô "Ẩn game" ở tab Cơ bản đọc lại trạng thái cửa sổ nên tự đồng bộ)
void CDlgAutoLogin::OnAccountListDblClk(NMHDR* pNMHDR, LRESULT* pResult)
{
	LPNMITEMACTIVATE pItem = reinterpret_cast<LPNMITEMACTIVATE>(pNMHDR);
	*pResult = 0;
	int nIndex = pItem->iItem;
	if (nIndex < 0 || nIndex >= (int)m_Accounts.size() || m_Accounts[nIndex].bGroup)
		return;
	DWORD dwPID = GetAccountGame(nIndex);
	HWND hGameWnd = dwPID ? FindGameWindowOfProcess(dwPID) : NULL;
	if (hGameWnd == NULL)
		return;
	if (::IsWindowVisible(hGameWnd))
	{
		ShowGameWindows(dwPID, SW_HIDE);
		return;
	}
	ShowGameWindows(dwPID, SW_SHOW);
	if (::IsIconic(hGameWnd))
		::ShowWindow(hGameWnd, SW_RESTORE);
}

// Dòng Nhóm tô màu nền cả hàng
void CDlgAutoLogin::OnAccountListCustomDraw(NMHDR* pNMHDR, LRESULT* pResult)
{
	LPNMLVCUSTOMDRAW pCD = reinterpret_cast<LPNMLVCUSTOMDRAW>(pNMHDR);
	*pResult = CDRF_DODEFAULT;
	if (pCD->nmcd.dwDrawStage == CDDS_PREPAINT)
		*pResult = CDRF_NOTIFYITEMDRAW;
	else if (pCD->nmcd.dwDrawStage == CDDS_ITEMPREPAINT)
	{
		int nRow = (int)pCD->nmcd.dwItemSpec;
		BOOL bGroup = nRow >= 0 && nRow < (int)m_Accounts.size() && m_Accounts[nRow].bGroup;
		if (m_AccountList.GetItemState(nRow, LVIS_SELECTED) & LVIS_SELECTED)
		{
			// Dòng đang chọn: tài khoản xanh nhạt, Nhóm đỏ (thay cho màu chọn mặc định xanh đậm)
			pCD->nmcd.uItemState &= ~CDIS_SELECTED;
			pCD->clrTextBk = bGroup ? GROUP_SELECTED_BK : SELECTED_ROW_BK;
			pCD->clrText = bGroup ? GROUP_ROW_TEXT : SELECTED_ROW_TEXT;
		}
		else if (bGroup)
		{
			pCD->clrTextBk = GROUP_ROW_BK;
			pCD->clrText = GROUP_ROW_TEXT;
		}
	}
}

// Ô giờ + ô phút => phút trong ngày. Cả 2 ô trống = -1 (không hẹn), sai => FALSE
static BOOL GetDayMinuteEdits(CWnd* pHour, CWnd* pMin, int& nDayMin)
{
	CString strHour, strMin;
	pHour->GetWindowText(strHour);
	pMin->GetWindowText(strMin);
	strHour.Trim();
	strMin.Trim();
	nDayMin = -1;
	if (strHour.IsEmpty() && strMin.IsEmpty())
		return TRUE;
	if (strHour.IsEmpty() || strHour.SpanIncluding(_T("0123456789")) != strHour || strMin.SpanIncluding(_T("0123456789")) != strMin)
		return FALSE;
	int nHour = _ttoi(strHour), nMin = _ttoi(strMin);	// Phút trống = 0
	if (nHour > 23 || nMin > 59)
		return FALSE;
	nDayMin = nHour * 60 + nMin;
	return TRUE;
}

// Lấy thông tin đang nhập ở tab Hẹn giờ. nPos: vị trí (số thứ tự trên bảng, 1..), trống = -1. Sai thì báo và trả về FALSE
BOOL CDlgAutoLogin::GetGroupFields(AutoLoginAccount& group, int& nPos)
{
	group.bGroup = TRUE;
	group.bChecked = FALSE;
	GetDlgItemText(IDC_NAMEGROUP_HENGIO_EDIT, group.strGroupName);
	group.strGroupName.Trim();
	CString strPos;
	GetDlgItemText(IDC_NUMPOS_HENGIO_EDIT, strPos);
	strPos.Trim();
	nPos = strPos.IsEmpty() ? -1 : _ttoi(strPos);

	UINT nFocusID = 0;
	LPCTSTR szError = NULL;
	if (group.strGroupName.IsEmpty())
		nFocusID = IDC_NAMEGROUP_HENGIO_EDIT, szError = _T("Chưa nhập tên nhóm");
	else if (!strPos.IsEmpty() && (strPos.SpanIncluding(_T("0123456789")) != strPos || nPos < 1))
		nFocusID = IDC_NUMPOS_HENGIO_EDIT, szError = _T("Vị trí là số thứ tự trên bảng (1, 2, ...), bỏ trống = cuối bảng");
	else if (!GetDayMinuteEdits(GetDlgItem(IDC_HOUR_LOGIN_HENGIO_EDIT), GetDlgItem(IDC_MIN_LOGIN_HENGIO_EDIT2), group.nLoginMin))
		nFocusID = IDC_HOUR_LOGIN_HENGIO_EDIT, szError = _T("Thời gian login: giờ 0..23, phút 0..59 (bỏ trống cả 2 ô = không hẹn login)");
	else if (!GetDayMinuteEdits(GetDlgItem(IDC_HOUR_OUTGAME_HENGIO_EDIT2), GetDlgItem(IDC_MIN_OUTGAME_HENGIO_EDIT3), group.nOutMin))
		nFocusID = IDC_HOUR_OUTGAME_HENGIO_EDIT2, szError = _T("Thời gian out: giờ 0..23, phút 0..59 (bỏ trống cả 2 ô = không hẹn thoát)");
	if (szError)
	{
		MessageBox(szError, _T("Hẹn giờ"), MB_OK | MB_ICONWARNING);
		GetDlgItem(nFocusID)->SetFocus();
		return FALSE;
	}
	return TRUE;
}

void CDlgAutoLogin::SetGroupFields(int nIndex)
{
	const AutoLoginAccount& group = m_Accounts[nIndex];
	SetDlgItemText(IDC_NAMEGROUP_HENGIO_EDIT, group.strGroupName);
	SetDlgItemInt(IDC_NUMPOS_HENGIO_EDIT, nIndex + 1, FALSE);
	CString strHour, strMin;
	if (group.nLoginMin >= 0)
		strHour.Format(_T("%d"), group.nLoginMin / 60), strMin.Format(_T("%02d"), group.nLoginMin % 60);
	SetDlgItemText(IDC_HOUR_LOGIN_HENGIO_EDIT, strHour);
	SetDlgItemText(IDC_MIN_LOGIN_HENGIO_EDIT2, strMin);
	strHour.Empty();
	strMin.Empty();
	if (group.nOutMin >= 0)
		strHour.Format(_T("%d"), group.nOutMin / 60), strMin.Format(_T("%02d"), group.nOutMin % 60);
	SetDlgItemText(IDC_HOUR_OUTGAME_HENGIO_EDIT2, strHour);
	SetDlgItemText(IDC_MIN_OUTGAME_HENGIO_EDIT3, strMin);
}

// Chuyển dòng nIndex tới vị trí nPos (0..), các dòng giữa dịch lên / xuống
void CDlgAutoLogin::PlaceGroup(int nIndex, int nPos)
{
	nPos = max(0, min(nPos, (int)m_Accounts.size() - 1));
	if (nPos == nIndex)
	{
		UpdateRow(nIndex);
		return;
	}
	AutoLoginAccount row = m_Accounts[nIndex];
	m_Accounts.erase(m_Accounts.begin() + nIndex);
	m_Accounts.insert(m_Accounts.begin() + nPos, row);
	for (int i = min(nIndex, nPos); i <= max(nIndex, nPos); i++)
		UpdateRow(i);
	UpdateStats();	// Các cột thống kê của tài khoản đã dịch dòng
	SelectRow(nPos);
}

void CDlgAutoLogin::OnBnClickedAddGroup()
{
	AutoLoginAccount group;
	int nPos = -1;
	if (!GetGroupFields(group, nPos))
		return;
	int nCount = (int)m_Accounts.size();
	nPos = nPos < 0 ? nCount : min(nPos - 1, nCount);
	group.nId = ++m_nNextId;
	m_Accounts.insert(m_Accounts.begin() + nPos, group);
	InsertRow(nPos);
	for (int i = nPos + 1; i < (int)m_Accounts.size(); i++)
		UpdateRow(i);	// Số thứ tự các dòng phía dưới
	SelectRow(nPos);
	SaveAccounts();
	UpdateStats();
}

void CDlgAutoLogin::OnBnClickedDeleteGroup()
{
	int nSel = GetSelectedRow();
	if (nSel < 0 || !m_Accounts[nSel].bGroup)
	{
		MessageBox(_T("Chọn nhóm cần xóa trong danh sách (dòng NHÓM)"), _T("Hẹn giờ"), MB_OK | MB_ICONWARNING);
		return;
	}
	CString strMsg;
	strMsg.Format(_T("Xóa nhóm \"%s\"?\nCác tài khoản trong nhóm vẫn giữ lại (thuộc nhóm phía trên)."), (LPCTSTR)m_Accounts[nSel].strGroupName);
	if (MessageBox(strMsg, _T("Hẹn giờ"), MB_YESNO | MB_ICONQUESTION) != IDYES)
		return;
	m_Accounts.erase(m_Accounts.begin() + nSel);
	m_AccountList.DeleteItem(nSel);
	for (int i = nSel; i < (int)m_Accounts.size(); i++)
		UpdateRow(i);
	SelectRow(min(nSel, (int)m_Accounts.size() - 1));
	SaveAccounts();
	UpdateStats();
}

void CDlgAutoLogin::OnBnClickedEditGroup()
{
	int nSel = GetSelectedRow();
	if (nSel < 0 || !m_Accounts[nSel].bGroup)
	{
		MessageBox(_T("Chọn nhóm cần sửa trong danh sách (dòng NHÓM)"), _T("Hẹn giờ"), MB_OK | MB_ICONWARNING);
		return;
	}
	AutoLoginAccount group;
	int nPos = -1;
	if (!GetGroupFields(group, nPos))
		return;
	AutoLoginAccount& cur = m_Accounts[nSel];
	cur.strGroupName = group.strGroupName;
	cur.nLoginMin = group.nLoginMin;
	cur.nOutMin = group.nOutMin;
	PlaceGroup(nSel, nPos < 0 ? nSel : nPos - 1);
	SaveAccounts();
}

void CDlgAutoLogin::UpdateHenGioButton()
{
	m_HenGioButton.SetWindowText(m_bHenGio ? _T("STOP Hẹn giờ") : _T("START Hẹn giờ"));
	m_HenGioButton.SetFaceColor(m_bHenGio ? HENGIO_ON_COLOR : HENGIO_OFF_COLOR);
}

void CDlgAutoLogin::OnBnClickedHenGio()
{
	if (!m_bHenGio)
	{
		BOOL bHasTime = FALSE;
		for (int i = 0; i < (int)m_Accounts.size(); i++)
			if (m_Accounts[i].bGroup && (m_Accounts[i].nLoginMin >= 0 || m_Accounts[i].nOutMin >= 0))
				bHasTime = TRUE;
		if (!bHasTime)
		{
			MessageBox(_T("Chưa có nhóm nào được hẹn giờ login / out (thêm nhóm ở tab Hẹn giờ)"), _T("Hẹn giờ"), MB_OK | MB_ICONWARNING);
			return;
		}
	}
	m_bHenGio = !m_bHenGio;
	m_nHenGioMinute = 0;	// Bật: xét luôn phút hiện tại
	for (int i = 0; i < (int)m_Accounts.size(); i++)
		m_Accounts[i].bHenGioActive = FALSE;
	UpdateHenGioButton();
	SaveAccounts();
	LoginLog(m_bHenGio ? _T("Bật hẹn giờ theo nhóm") : _T("Tắt hẹn giờ theo nhóm"));
}

// Giờ nDayMin nằm trong khoảng hẹn của nhóm: từ giờ login tới trước giờ out (qua nửa đêm được). Không hẹn out = luôn trong khoảng
static BOOL IsInHenGioWindow(const AutoLoginAccount& group, int nDayMin)
{
	if (group.nLoginMin < 0)
		return FALSE;
	if (group.nOutMin < 0)
		return TRUE;
	if (group.nLoginMin <= group.nOutMin)
		return nDayMin >= group.nLoginMin && nDayMin < group.nOutMin;
	return nDayMin >= group.nLoginMin || nDayMin < group.nOutMin;
}

// Mỗi phút một lần: nhóm tới giờ out thì thoát game các tài khoản được check trong nhóm, tới giờ login thì login các tài khoản đó.
// Timer bị trễ (vd đang mở hộp thoại) thì xét bù các phút vừa qua, tối đa HENGIO_CATCHUP_MIN phút.
// Bật hẹn giờ giữa khoảng login - out của nhóm thì login luôn. Trong khoảng đó Game bị tắt / văng thì login lại sau ReloginDelay của tài khoản
#define HENGIO_CATCHUP_MIN		5
void CDlgAutoLogin::CheckHenGio()
{
	if (!m_bHenGio)
		return;
	CTime tNow = CTime::GetCurrentTime();
	int nDayNow = tNow.GetHour() * 60 + tNow.GetMinute();
	__int64 nNow = tNow.GetTime() / 60;
	std::vector<DWORD> PIDs;
	BOOL bLogin = FALSE;
	if (nNow != m_nHenGioMinute)
	{
		BOOL bStart = m_nHenGioMinute == 0;
		__int64 nFrom = nNow - m_nHenGioMinute <= HENGIO_CATCHUP_MIN ? m_nHenGioMinute + 1 : nNow;
		m_nHenGioMinute = nNow;
		for (__int64 nMinute = nFrom; nMinute <= nNow; nMinute++)
		{
			CTime t((time_t)(nMinute * 60));
			int nDayMin = t.GetHour() * 60 + t.GetMinute();
			int nGroup = -1, nOut = 0, nIn = 0;
			for (int i = 0; i <= (int)m_Accounts.size(); i++)
			{
				// Hết nhóm (gặp nhóm mới / cuối bảng): ghi log của nhóm vừa xét
				if (i == (int)m_Accounts.size() || m_Accounts[i].bGroup)
				{
					if (nGroup >= 0 && nOut)
						LoginLog(_T("Hẹn giờ nhóm %s: %s out game %d tài khoản"), (LPCTSTR)m_Accounts[nGroup].strGroupName, (LPCTSTR)t.Format(_T("%H:%M")), nOut);
					if (nGroup >= 0 && nIn)
						LoginLog(_T("Hẹn giờ nhóm %s: %s login %d tài khoản"), (LPCTSTR)m_Accounts[nGroup].strGroupName, (LPCTSTR)t.Format(_T("%H:%M")), nIn);
					if (i == (int)m_Accounts.size())
						break;
					nGroup = i;
					nOut = nIn = 0;
					continue;
				}
				AutoLoginAccount& acc = m_Accounts[i];
				if (nGroup < 0 || !acc.bChecked)
					continue;	// Tài khoản không thuộc nhóm nào / không check
				const AutoLoginAccount& group = m_Accounts[nGroup];
				if (group.nOutMin == nDayMin)
				{
					DWORD dwPID = GetAccountGame(i);
					StopAccount(i);
					acc.bHenGioOut = TRUE;	// Không tự login lại (Lặp lại / LOGIN ALL) tới giờ login kế tiếp
					AddUniquePID(PIDs, dwPID);
					nOut++;
				}
				else if (group.nLoginMin == nDayMin || (bStart && group.nOutMin >= 0 && IsInHenGioWindow(group, nDayMin)))
				{
					AddLoginEntry(i);
					acc.bHenGioActive = TRUE;
					acc.dwHenGioOffTick = 0;
					bLogin = TRUE;
					nIn++;
				}
			}
		}
	}

	// Trong khoảng login - out: tài khoản đã login theo giờ mà không ở trong game (Game tắt / văng / bị Cài đặt thoát) thì login lại
	DWORD dwTick = GetTickCount();
	int nGroup = -1;
	for (int i = 0; i < (int)m_Accounts.size(); i++)
	{
		AutoLoginAccount& acc = m_Accounts[i];
		if (acc.bGroup)
		{
			nGroup = i;
			continue;
		}
		if (acc.bHenGioActive && (nGroup < 0 || !acc.bChecked || !IsInHenGioWindow(m_Accounts[nGroup], nDayNow)))
			acc.bHenGioActive = FALSE;	// Bỏ check / đổi nhóm / sửa giờ / hết khoảng hẹn
		if (!acc.bHenGioActive || acc.bBadLogin || FindLoginEntry(acc.nId) >= 0 || m_Job.nId == acc.nId || IsAccountInGame(i))
		{
			acc.dwHenGioOffTick = 0;
			continue;
		}
		if (acc.dwHenGioOffTick == 0)
		{
			acc.dwHenGioOffTick = dwTick ? dwTick : 1;
			continue;
		}
		if ((int)(dwTick - acc.dwHenGioOffTick) < acc.nReloginDelay)
			continue;
		acc.dwHenGioOffTick = 0;
		LoginLog(_T("%s: Hẹn giờ nhóm %s - không ở trong game, login lại"), (LPCTSTR)acc.strAccount, (LPCTSTR)m_Accounts[nGroup].strGroupName);
		AddLoginEntry(i);
		bLogin = TRUE;
	}

	if (!PIDs.empty())
		CloseGames(PIDs);
	if (bLogin)
		RemoveOnlineEntries();
}

/////////////////////////////////////////////////////////////////////////////
// CColorButton

void CColorButton::DrawItem(LPDRAWITEMSTRUCT lpDIS)
{
	CDC* pDC = CDC::FromHandle(lpDIS->hDC);
	CRect rc = lpDIS->rcItem;
	BOOL bPushed = (lpDIS->itemState & ODS_SELECTED) != 0;
	pDC->FillSolidRect(&rc, m_clrFace);
	pDC->DrawEdge(&rc, bPushed ? EDGE_SUNKEN : EDGE_RAISED, BF_RECT);
	CString strText;
	GetWindowText(strText);
	if (bPushed)
		rc.OffsetRect(1, 1);
	CFont* pOldFont = pDC->SelectObject(GetFont());
	pDC->SetBkMode(TRANSPARENT);
	pDC->SetTextColor((lpDIS->itemState & ODS_DISABLED) ? GetSysColor(COLOR_GRAYTEXT) : RGB(255, 255, 255));
	pDC->DrawText(strText, &rc, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
	pDC->SelectObject(pOldFont);
	if (lpDIS->itemState & ODS_FOCUS)
	{
		CRect rcFocus = lpDIS->rcItem;
		rcFocus.DeflateRect(4, 4);
		pDC->DrawFocusRect(&rcFocus);
	}
}
