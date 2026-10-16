#pragma once

#include <vector>

// Một tài khoản trong danh sách Auto Login
struct AutoLoginAccount
{
	CString			strAccount;
	CString			strPassword;
	CString			strRegion;		// Tên cụm server (Region_x trong UiConfig\serverlist_.txt)
	CString			strServer;		// Tên server (x_Title trong [Region_x])
	int				nRoleIndex;		// Nhân vật thứ mấy (0..2)
	BOOL			bChecked;		// Được chọn để chạy LOGIN ALL
	BOOL			bAutoRelogin;	// Chưa đăng nhập (mất kết nối, Game tắt...) thì đăng nhập lại
	int				nReloginDelay;	// ms chờ trước khi đăng nhập lại
	CString			strPlayerName;	// Nhân vật vào game gần nhất (để bảng Thống kê hiện tiền khi offline)
	CString			strPlayerCfg;	// Tên file cfg của nhân vật đó trong UserConfig
	BOOL			bTongGather;	// Được chọn trong bảng Thống kê để gom tiền lên bang

	// Không lưu file
	int				nId;			// Định danh cố định trong lúc chạy (dòng có thể bị đổi chỗ / xóa)
	DWORD			dwProcessId;	// Game đang chạy của tài khoản
	DWORD			dwOfflineTick;	// Mốc bắt đầu không ở trong game, để đếm ReloginDelay ở vòng lặp (0 = chưa đếm)
	CString			strError;		// Lỗi của lần đăng nhập gần nhất

	AutoLoginAccount() : nRoleIndex(0), bChecked(TRUE), bAutoRelogin(FALSE), nReloginDelay(10000), bTongGather(FALSE),
		nId(0), dwProcessId(0), dwOfflineTick(0) {}
};

// Một tài khoản trong danh sách cần login
struct AutoLoginEntry
{
	int				nId;			// AutoLoginAccount::nId
	CString			strAccount;
	int				nFails;			// Số lần đăng nhập thất bại
	DWORD			dwNextTick;		// Chưa tới mốc này thì chưa thử lại (0 = login ngay)
};

// Danh sách cần login: Auto Login đăng nhập lần lượt từ đầu danh sách,
// thành công thì loại khỏi danh sách, thất bại thì thử lại sau LOGIN_RETRY_DELAY
extern std::vector<AutoLoginEntry> g_AutoLoginList;

// Một cụm server đọc từ UiConfig\serverlist_.txt
struct AutoLoginRegion
{
	CString					strTitle;
	std::vector<CString>	Servers;
	std::vector<CString>	Addresses;
};

// Tài khoản đang được đăng nhập (mỗi lúc chỉ một tài khoản)
struct AutoLoginJob
{
	int				nId;			// 0 = không có
	DWORD			dwStartTick;
	DWORD			dwActionTick;	// Lần thao tác gần nhất
	DWORD			dwWait;			// Chờ bao lâu sau thao tác đó
	BOOL			bLaunched;		// Đã mở Game cho lượt này
	int				nScreen;		// Màn hình của thao tác gần nhất
	int				nTries;			// Số lần thao tác liên tiếp trên cùng màn hình
	CString			strStatus;
};

// Game đang chờ đóng: đã gửi WM_CLOSE, quá hạn thì TerminateProcess
struct ClosingGame
{
	DWORD			dwPID;
	DWORD			dwDeadline;
};

class CDlgAutoLogin;

// CDlgLoginStats - Thống kê các nhân vật trong bảng Auto Login (nút "Xem Thống Kê", cửa sổ không modal)
class CDlgLoginStats : public CDialog
{
public:
	CDlgLoginStats(CDlgAutoLogin* pAutoLogin) : CDialog(IDD_LOGIN_STATS_DIALOG), m_pAutoLogin(pAutoLogin), m_bUpdating(FALSE), m_cyInit(0) {}

	enum { IDD = IDD_LOGIN_STATS_DIALOG };

	void			ShowStats();

protected:
	CDlgAutoLogin*	m_pAutoLogin;
	CListCtrl		m_StatsList;
	BOOL			m_bUpdating;	// Đang đổi ô check bằng code, bỏ qua LVN_ITEMCHANGED
	int				m_cxInit, m_cyInit;	// Kích thước client lúc tạo, để OnSize dời các control
	CRect			m_rcInit[8];

	virtual BOOL	OnInitDialog();
	virtual void	DoDataExchange(CDataExchange* pDX);
	virtual void	OnOK() {}
	virtual void	OnCancel();

	void			UpdateStats();
	void			SetCell(int nRow, int nCol, LPCTSTR szText);

	void			SetRowCheck(int nRow, BOOL bCheck);

	afx_msg void	OnTimer(UINT_PTR nIDEvent);
	afx_msg void	OnSize(UINT nType, int cx, int cy);
	afx_msg void	OnGetMinMaxInfo(MINMAXINFO* lpMMI);
	afx_msg void	OnStatsItemChanged(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void	OnKeepMoneyChanged();
	afx_msg void	OnBnClickedTongMoney();
	DECLARE_MESSAGE_MAP()
};

// CDlgAutoLogin dialog - Tự động đăng nhập (cửa sổ không modal)

class CDlgAutoLogin : public CDialog
{
	DECLARE_DYNAMIC(CDlgAutoLogin)
	friend class CDlgLoginStats;

public:
	CDlgAutoLogin(CWnd* pParent = NULL);   // standard constructor
	virtual ~CDlgAutoLogin();

// Dialog Data
	enum { IDD = IDD_AUTO_LOGIN_DIALOG };

protected:
	CListCtrl		m_AccountList;
	CComboBox		m_RegionCombo;
	CComboBox		m_ServerCombo;
	CComboBox		m_RoleCombo;
	CEdit			m_PasswordEdit;
	CToolTipCtrl	m_ToolTip;
	TCHAR			m_chPasswordChar;
	BOOL			m_bUpdatingList;	// Đang đổi ô check bằng code, bỏ qua LVN_ITEMCHANGED
	CString			m_strDataFile;		// AutoLogin.ini
	CString			m_strGamePath;		// Đường dẫn file exe của Game
	int				m_nNextId;

	std::vector<AutoLoginAccount>	m_Accounts;
	std::vector<AutoLoginRegion>	m_Regions;
	AutoLoginJob					m_Job;
	BOOL							m_bRunAll;		// Đang chạy LOGIN ALL (vòng lặp lấy lại danh sách tài khoản được check)
	int								m_nRound;		// Vòng thứ mấy của LOGIN ALL (từ 1)
	DWORD							m_dwStartAllTick;	// Mở Auto: chờ tới mốc này mới tự LOGIN ALL (0 = không)
	std::vector<ClosingGame>		m_Closing;
	CDlgLoginStats					m_DlgStats;
	int								m_nTongKeep;	// Vạn: số tiền mỗi nhân vật giữ lại khi gom tiền lên bang (bảng Thống kê)

	virtual BOOL	OnInitDialog();
	virtual void	DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual BOOL	PreTranslateMessage(MSG* pMsg);
	virtual void	OnOK() {}
	virtual void	OnCancel();

	void			LoadAccounts();
	void			SaveAccounts();
	void			LoadServerList();
	void			FillServerCombo();
	void			InsertRow(int nIndex);
	void			UpdateRow(int nIndex);
	void			SelectRow(int nIndex);
	int				GetSelectedRow();
	int				FindAccount(const AutoLoginAccount& acc, int nExcept);
	int				FindAccountById(int nId);
	BOOL			GetFields(AutoLoginAccount& acc);
	void			SetFields(const AutoLoginAccount& acc);
	void			MoveRow(int nFrom, int nTo);

	// Game
	BOOL			IsGameProcess(DWORD dwPID);
	DWORD			GetAccountGame(int nIndex, GameWindow** ppGameWin = NULL);
	BOOL			IsAccountInGame(int nIndex);
	BOOL			LaunchGame(int nIndex, CString& strError);
	void			CloseGames(const std::vector<DWORD>& PIDs);
	void			CheckClosingGames();
	CString			GetStatusText(int nIndex);
	void			UpdateGameStatus();
	void			UpdatePlayerNames();
	DWORD			GetWaitTime(int nScreen);

	// Đăng nhập
	void			StartLoginAll(BOOL bWarn);
	void			StopAccount(int nIndex);
	void			StopAllLogin();
	int				FindLoginEntry(int nId);
	void			AddLoginEntry(int nIndex);
	void			BuildLoginList();
	void			RemoveOnlineEntries();
	int				GetRetryCount();
	void			RunLogin();
	void			RunLoginStep(int nIndex, HANDLE hGame);
	void			EndJob(int nIndex, LPCTSTR szError, BOOL bRetry, BOOL bCloseGame);
	BOOL			LoginAction(int nScreen, LPCTSTR szStatus, DWORD dwWait);
	BOOL			DoSelectServer(int nIndex, HANDLE hGame, DWORD dwSel, CString& strError);
	BOOL			DoAccountLogin(int nIndex, HANDLE hGame);
	BOOL			DoSelectRole(int nIndex, HANDLE hGame, DWORD dwRoleWnd, CString& strError, BOOL& bWait, BOOL& bRetry);

	afx_msg void	OnAccountListItemChanged(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void	OnAccountListColumnClick(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void	OnRegionComboSelChange();
	afx_msg void	OnBnClickedAdd();
	afx_msg void	OnBnClickedRemove();
	afx_msg void	OnBnClickedEdit();
	afx_msg void	OnBnClickedUp();
	afx_msg void	OnBnClickedDown();
	afx_msg void	OnBnClickedShowPassword();
	afx_msg void	OnBnClickedGetGamePath();
	afx_msg void	OnGamePathKillFocus();
	afx_msg void	OnBnClickedOpenGame();
	afx_msg void	OnBnClickedTerminalGame();
	afx_msg void	OnBnClickedTerminalGameAll();
	afx_msg void	OnBnClickedStartLogin();
	afx_msg void	OnBnClickedStartLoginAll();
	afx_msg void	OnBnClickedStopLogin();
	afx_msg void	OnLoginOptionChanged();
	afx_msg void	OnBnClickedReport();
	afx_msg void	OnTimer(UINT_PTR nIDEvent);
	DECLARE_MESSAGE_MAP()

public:
	void			ShowAutoLogin();
	int				GetAccountCount() { return (int)m_Accounts.size(); }
	const AutoLoginAccount&	GetAccount(int nIndex) { return m_Accounts[nIndex]; }
};
