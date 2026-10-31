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
	BOOL			bChecked;		// Ô check: gom tiền lên bang + login / out theo giờ của nhóm (không dùng cho LOGIN ALL)
	BOOL			bAutoRelogin;	// Chưa đăng nhập (mất kết nối, Game tắt...) thì đăng nhập lại
	int				nReloginDelay;	// ms chờ trước khi đăng nhập lại
	CString			strPlayerName;	// Nhân vật vào game gần nhất (để cột Tổng tiền hiện khi offline)
	CString			strPlayerCfg;	// Tên file cfg của nhân vật đó trong UserConfig

	// Dòng Nhóm (tab Hẹn giờ): quản lý các tài khoản phía dưới tới Nhóm kế tiếp. Không phải tài khoản (không login, không ô check)
	BOOL			bGroup;
	CString			strGroupName;
	int				nLoginMin;		// Giờ login, phút trong ngày (-1 = không hẹn)
	int				nOutMin;		// Giờ out game, phút trong ngày (-1 = không hẹn)

	// Không lưu file
	BOOL			bHenGioOut;		// Đã bị thoát theo giờ out của nhóm: không tự login lại tới khi được login lần nữa
	BOOL			bHenGioActive;	// Đã login theo giờ của nhóm: Game tắt trước giờ out thì login lại
	DWORD			dwHenGioOffTick;	// Mốc bắt đầu không ở trong game khi bHenGioActive (0 = đang trong game / đang login)
	int				nId;			// Định danh cố định trong lúc chạy (dòng có thể bị đổi chỗ / xóa)
	DWORD			dwProcessId;	// Game đang chạy của tài khoản
	DWORD			dwOfflineTick;	// Mốc bắt đầu không ở trong game, để đếm ReloginDelay ở vòng lặp (0 = chưa đếm)
	BOOL			bBadLogin;		// Game báo sai tài khoản / mật khẩu: không login tài khoản này tới khi sửa thông tin
	BOOL			bSeenInGame;	// Đã thấy vào thế giới game (Game bị văng / tắt / crash thì được login lại khi check Lặp lại)
	CString			strError;		// Lỗi của lần đăng nhập gần nhất

	// Cài đặt thoát (tab Thống kê): theo dõi Game dwExitPID, đổi Game thì đếm lại
	DWORD			dwExitPID;
	BOOL			bWasInGame;		// Game này đã từng vào thế giới game (để nhận ra mất kết nối)
	DWORD			dwLostTick;		// Mốc bắt đầu mất kết nối (0 = đang kết nối)
	DWORD			dwHungTick;		// Mốc bắt đầu treo (0 = không treo)
	__int64			nExpLast;
	DWORD			dwExpTick;		// Mốc kinh nghiệm thay đổi lần cuối (0 = chưa đếm)
	int				nStayMap, nStayX, nStayY;	// Điểm đang quanh quẩn
	DWORD			dwStayTick;		// Mốc bắt đầu quanh quẩn điểm đó (0 = chưa đếm)

	AutoLoginAccount() : nRoleIndex(0), bChecked(TRUE), bAutoRelogin(FALSE), nReloginDelay(10000),
		bGroup(FALSE), nLoginMin(-1), nOutMin(-1), bHenGioOut(FALSE), bHenGioActive(FALSE), dwHenGioOffTick(0), nId(0), dwProcessId(0), dwOfflineTick(0), bBadLogin(FALSE), bSeenInGame(FALSE) { ResetExitTrack(0); }
	void ResetExitTrack(DWORD dwPID)
	{
		dwExitPID = dwPID; bWasInGame = FALSE; dwLostTick = dwHungTick = dwExpTick = dwStayTick = 0;
		nExpLast = 0; nStayMap = nStayX = nStayY = 0;
	}
};

// Cài đặt thoát game (tab Thống kê), áp dụng cho Game của mọi tài khoản trong bảng Auto Login
struct AutoLoginExitOpt
{
	BOOL			bLost;			int nLostMin;		// Mất kết nối quá nLostMin phút
	BOOL			bNoExp;			int nNoExpMin;		// Kinh nghiệm không tăng quá nNoExpMin phút
	BOOL			bHung;			int nHungSec;		// Game treo (không xử lý message) quá nHungSec giây
	BOOL			bStay;			int nStayMin;		// Quanh quẩn trong bán kính nStayDist quá nStayMin phút
	int				nStayDist;
	BOOL			bStayFight;		// Áp dụng cả khi ở nơi chiến đấu (FALSE: chỉ khi ở trạng thái phi chiến đấu)
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
	CString			strGameInfo;	// Thông báo đăng nhập gần nhất của Game (đã ghi log)
	HANDLE			hRemote;		// Thread thao tác trong Game chưa chạy xong (Game đang bận, vd lần mở đầu), NULL = không có
	LPVOID			pRemote;		// Vùng nhớ mã của thread đó trong Game
	DWORD			dwRemotePID;
};

// Game đang chờ đóng: đã gửi WM_CLOSE, quá hạn thì TerminateProcess
struct ClosingGame
{
	DWORD			dwPID;
	DWORD			dwDeadline;
};

class CDlgAutoLogin;
class CRemoteCode;

// Nút tô màu nền (owner draw): nút START / STOP Hẹn giờ
class CColorButton : public CButton
{
public:
	CColorButton() : m_clrFace(RGB(200, 0, 0)) {}
	void			SetFaceColor(COLORREF clrFace) { m_clrFace = clrFace; if (m_hWnd) Invalidate(); }

protected:
	COLORREF		m_clrFace;
	virtual void	DrawItem(LPDRAWITEMSTRUCT lpDIS);
};

// Trang của tab trong Auto Login (dialog con IDD_AUTO_LOGIN_SETTING_PAGE / IDD_AUTO_LOGIN_STATS_PAGE).
// Thông báo của control chuyển hết lên CDlgAutoLogin xử lý
class CDlgAutoLoginPage : public CDialog
{
public:
	CDlgAutoLoginPage(UINT nIDTemplate) : CDialog(nIDTemplate) {}

protected:
	virtual void	OnOK() {}
	virtual void	OnCancel() {}
	virtual BOOL	OnCommand(WPARAM wParam, LPARAM lParam);
	virtual BOOL	OnNotify(WPARAM wParam, LPARAM lParam, LRESULT* pResult);
	virtual BOOL	PreTranslateMessage(MSG* pMsg);
};

// CDlgAutoLogin dialog - Tự động đăng nhập (cửa sổ không modal)

class CDlgAutoLogin : public CDialog
{
	DECLARE_DYNAMIC(CDlgAutoLogin)

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
	DWORD							m_dwStartAllTick;	// Mở Auto: chờ tới mốc này mới tự LOGIN ALL (0 = không)
	std::vector<ClosingGame>		m_Closing;
	CTabCtrl						m_Tab;			// Cài đặt / Thống kê
	CDlgAutoLoginPage				m_PageSetting;	// Tab Cài đặt: nhóm Tài khoản + Thiết lập
	CDlgAutoLoginPage				m_PageStats;	// Tab Thống kê: gom tiền + Cài đặt thoát
	CDlgAutoLoginPage				m_PageHenGio;	// Tab Hẹn giờ: thêm / sửa / xóa Nhóm
	CColorButton					m_HenGioButton;
	BOOL							m_bHenGio;		// Đang bật hẹn giờ theo nhóm
	__int64							m_nHenGioMinute;	// Phút (tính từ 1970) đã xử lý hẹn giờ gần nhất
	int								m_nTongKeep;	// Vạn: số tiền mỗi nhân vật giữ lại khi gom tiền lên bang (tab Thống kê)
	AutoLoginExitOpt				m_ExitOpt;		// Cài đặt thoát (tab Thống kê)
	DWORD							m_dwExitCheckTick;

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
	int				GetSelectedRow();		// Dòng đang chọn (tài khoản hoặc Nhóm), -1 = không có / dòng TỔNG
	int				GetSelectedAccount();	// Tài khoản đang chọn, -1 = không có / đang chọn Nhóm
	BOOL			IsGroupSelected();
	int				FindAccount(const AutoLoginAccount& acc, int nExcept);
	int				FindAccountById(int nId);
	BOOL			GetFields(AutoLoginAccount& acc);
	void			SetFields(const AutoLoginAccount& acc);
	void			MoveRow(int nFrom, int nTo);
	void			LayoutDialog();
	void			ShowTab(int nTab);

	// Control nằm ở dialog chính hoặc ở trang của tab: các hàm cùng tên của CWnd tìm cả trong các trang
	CWnd*			GetCtrlParent(int nID) const;
	CWnd*			GetDlgItem(int nID) const;
	UINT			IsDlgButtonChecked(int nID) const;
	void			CheckDlgButton(int nID, UINT nCheck);
	void			SetDlgItemText(int nID, LPCTSTR szText);
	int				GetDlgItemText(int nID, CString& strText) const;
	void			SetDlgItemInt(int nID, UINT nValue, BOOL bSigned = TRUE);
	UINT			GetDlgItemInt(int nID, BOOL* pTrans = NULL, BOOL bSigned = TRUE) const;

public:
	void			RelayToolTip(MSG* pMsg) { if (m_ToolTip.m_hWnd) m_ToolTip.RelayEvent(pMsg); }

protected:
	void			UpdateStats();
	void			SetCell(int nRow, int nCol, LPCTSTR szText);

	// Hẹn giờ theo nhóm
	BOOL			GetGroupFields(AutoLoginAccount& group, int& nPos);
	void			SetGroupFields(int nIndex);
	void			PlaceGroup(int nIndex, int nPos);
	void			UpdateHenGioButton();
	void			CheckHenGio();

	// Game
	BOOL			IsGameProcess(DWORD dwPID);
	DWORD			GetAccountGame(int nIndex, GameWindow** ppGameWin = NULL);
	BOOL			IsAccountInGame(int nIndex);
	BOOL			LaunchGame(int nIndex, CString& strError);
	void			CloseGames(const std::vector<DWORD>& PIDs);
	void			CheckClosingGames();
	BOOL			IsClosingGame(DWORD dwPID);
	CString			GetStatusText(int nIndex);
	void			UpdateGameStatus();
	void			UpdatePlayerNames();
	DWORD			GetWaitTime(int nScreen);
	void			CheckExitGames();

	// Đăng nhập
	void			StartLoginAll(BOOL bWarn);
	void			StopAccount(int nIndex);
	void			StopAllLogin();
	int				FindLoginEntry(int nId);
	void			AddLoginEntry(int nIndex);
	void			BuildLoginList(BOOL bFirst, BOOL bAutoOnly = FALSE);
	void			RemoveOnlineEntries();
	int				GetRetryCount();
	void			RunLogin();
	void			RunLoginStep(int nIndex, HANDLE hGame);
	BOOL			HandleConnectInfo(int nIndex, HANDLE hGame);
	BOOL			ExecJobCode(HANDLE hGame, LPVOID pRemote, const CRemoteCode& code, DWORD* pdwExit);
	BOOL			CheckJobRemote();
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
	afx_msg void	OnTabSelChange(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void	OnKeepMoneyChanged();
	afx_msg void	OnBnClickedTongMoney();
	afx_msg void	OnExitOptionChanged(UINT nID);
	afx_msg void	OnAccountListCustomDraw(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void	OnAccountListRClick(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void	OnAccountListDblClk(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void	OnBnClickedAddGroup();
	afx_msg void	OnBnClickedDeleteGroup();
	afx_msg void	OnBnClickedEditGroup();
	afx_msg void	OnBnClickedHenGio();
	afx_msg void	OnTimer(UINT_PTR nIDEvent);
	DECLARE_MESSAGE_MAP()

public:
	void			ShowAutoLogin();
	int				GetAccountCount() { return (int)m_Accounts.size(); }
	const AutoLoginAccount&	GetAccount(int nIndex) { return m_Accounts[nIndex]; }
};
