#pragma once

// Dùng chung cho Tab Đăng nhập (DlgDebug) và Auto Login (DlgAutoLogin)

// Tab Đăng nhập: ở màn hình bắt đầu Game chưa nạp VLHook (chỉ gắn khi đã vào game)
// => VLAuto tự đọc bộ nhớ Game và tạo thread trong Game (CreateRemoteThread) để gọi hàm giao diện.
// Địa chỉ 10_10 (xem Offsets_10_10.md mục Đăng nhập)
#define LG_UIN_BASE_ADD		0x00840460	// KUiInit::m_pSelf
#define LG_IOC_FUNC_ADD		0x0048FFB0	// KUiInit::OnClickButton(KWndButton*) thiscall
#define LG_SMC_BASE_ADD		0x00847EF4	// KUiSysMsgCentre (!= 0 khi đang trong game)
#define LG_SEL_SELF			0x00839A18	// KUiSelServer::m_pSelf ("UiNewLogin\选服务器.ini", OpenWindow 0x48BA00)
#define LG_SEL_LOGIN_FUNC	0x0048ACD0	// KUiSelServer::OnLogin() thiscall: máy chủ đang chọn -> mở cửa sổ đăng nhập
#define LG_SEL_REGIONS		0x0904		// KLoginServer* danh sách KHU VỰC (GetServerRegionList 0x597E00), 0x24 byte/dòng
#define LG_SEL_REGION		0x2EB8		// Chỉ số khu vực đang chọn
#define LG_SEL_LEFTLIST		0x090C		// KWndList: 14 khu vực đầu
#define LG_SEL_RIGHTLIST	0x0EBC		// KWndList: khu vực thứ 15 trở đi (0x489F20: count > 14)
#define LG_SEL_IPLIST		0x146C		// KWndList: MÁY CHỦ của khu vực đang chọn
#define LG_SEL_SERVERS		0x0908		// KLoginServer* máy chủ của khu vực đang chọn (GetServerList 0x597F10)
#define LG_SEL_UPDATE		0x0048AF80	// KUiSelServer: khu vực đang chọn ở Left/RightList -> LG_SEL_REGION, nạp lại IpList
#define LG_LIST_LEFT_NUM	14			// LeftList chứa tối đa 14 khu vực
#define LG_LIST_COUNT		0x049C		// KWndList::số dòng
#define LG_LIST_CURSEL		0x04B0		// KWndList::dòng đang chọn
#define LG_LIST_SETCURSEL	0x0045BD20	// KWndList::SetCurSel(int) thiscall, báo 0x691 lên cửa sổ cha
#define LG_LIST_CLEARSEL	0x00483C30	// KWndList: dòng đang chọn = -1 (không báo)
#define LG_LIST_FOCUS		0x00483B10	// KWndList: màu list đang được chọn (int) thiscall (+0x4C4 = +0x5A8 / +0x5A4)
#define LG_LOGIN_SELF		0x008399FC	// KUiLogin::m_pSelf ("UiNewLogin\登陆.ini", OpenWindow 0x487CF0)
#define LG_LOGIN_ACCOUNT	0x0588		// KWndEdit m_Account
#define LG_LOGIN_PASSWORD	0x0A94		// KWndEdit m_PassWord
#define LG_LOGIN_AGREE		0x80C0		// Ô tick đồng ý điều khoản (chưa tick thì OnLogin chỉ hiện thông báo)
#define LG_LOGIN_FUNC		0x004871D0	// KUiLogin::OnLogin() thiscall
#define LG_EDIT_SETTEXT		0x00458A30	// KWndEdit::SetText(const char*) thiscall
#define LG_BTN_CHECK		0x00453AD0	// KWndButton::CheckButton(int) thiscall
#define LG_ROLE_SELF		0x0084053C	// KUiSelPlayer::m_pSelf ("UiNewLogin\选游戏存档人物.ini", OpenWindow 0x49D160)
#define LG_ROLE_SELECT		0x0049C940	// KUiSelPlayer::OnSelectPlayer(int nSel, bool bPlaySound) thiscall
#define LG_ROLE_ENTER		0x0049C210	// KUiSelPlayer::OnEnterGame() thiscall: SelectRole(m_nSelPlayer + trang) 0x595E50
#define LG_ROLE_NUM			0xB964		// short m_nNumPlayer (= số nhân vật)
#define LG_ROLE_SEL			0xB966		// short m_nSelPlayer (ô đang chọn trên trang, -1 = chưa chọn)
#define LG_ROLE_PAGE		0xB970		// short: số nhân vật đã lật qua bằng Next/Pre
#define LG_ROLE_PER_PAGE	3			// Hiện 3 nhân vật mỗi trang (m_player 0x5764, 0x770 byte/ô)
#define LG_LOGIC			0x009BCBB8	// g_LoginLogic (KLogin)
#define LG_LOGIC_STATUS		0x0008		// m_Status (SelectRole cần = 7)
#define LG_LOGIC_ROLENUM	0x0012		// short m_nNumRole
#define LG_LOGIC_ROLES		0x0014		// KRoleChiefInfo[] (Name[32], Gender, Attribute, short Level) 0x24 byte
#define LG_WND_S_VISIBLE	0x80000000

static const DWORD s_dwInitButtons[] = { 0x05A4, 0x1484, 0x3244, 0x2364 };
#define INIT_BUTTON_NUM		(sizeof(s_dwInitButtons) / sizeof(s_dwInitButtons[0]))

inline DWORD ReadGameDword(HANDLE hGame, DWORD dwAddr)
{
	DWORD dwValue = 0;
	SIZE_T nRead = 0;
	if (!dwAddr || !ReadProcessMemory(hGame, (LPCVOID)(UINT_PTR)dwAddr, &dwValue, sizeof(dwValue), &nRead))
		return 0;
	return dwValue;
}

// Mã máy chạy trong thread của Game (ThreadProc stdcall, 1 tham số):
//	push esi
//	mov esi, [m_pSelf]; xor eax, eax; test esi, esi; jz end	(cửa sổ chưa mở => exit code 0)
//	... các lệnh gọi hàm thiscall với ecx = esi + offset ...
//	mov eax, 1
// end: pop esi; ret 4
class CRemoteCode
{
public:
	CRemoteCode(DWORD dwBase) : m_dwBase(dwBase), m_nCode(0), m_nData(0), m_nJzEnd(0) {}

	void Begin(DWORD dwSelfPtr)
	{
		B(0x56);										// push esi
		B(0x8B); B(0x35); D(dwSelfPtr);					// mov esi, [dwSelfPtr]
		B(0x33); B(0xC0);								// xor eax, eax
		B(0x85); B(0xF6);								// test esi, esi
		B(0x0F); B(0x84); m_nJzEnd = m_nCode; D(0);		// jz end
	}
	void PushImm(DWORD dwValue)		{ B(0x68); D(dwValue); }
	void PushThis(DWORD dwOffset)	{ B(0x8D); B(0x86); D(dwOffset); B(0x50); }	// lea eax, [esi + dwOffset]; push eax
	void EcxThis(DWORD dwOffset)	{ B(0x8D); B(0x8E); D(dwOffset); }		// lea ecx, [esi + dwOffset]
	void Call(DWORD dwFunc)			{ B(0xB8); D(dwFunc); B(0xFF); B(0xD0); }	// mov eax, dwFunc; call eax
	void End()
	{
		B(0xB8); D(1);									// mov eax, 1
		*(DWORD*)(m_Code + m_nJzEnd) = m_nCode - (m_nJzEnd + 4);
		B(0x5E);										// pop esi
		B(0xC2); B(0x04); B(0x00);						// ret 4
	}
	// Chuỗi đặt sau phần mã, trả về địa chỉ trong Game
	DWORD AddString(const char *szText)
	{
		int nLen = (int)strlen(szText) + 1;
		if (m_nData + nLen > (int)sizeof(m_Data)) return 0;
		memcpy(m_Data + m_nData, szText, nLen);
		DWORD dwAddr = m_dwBase + CODE_SIZE + m_nData;
		m_nData += nLen;
		return dwAddr;
	}
	void Build(BYTE *pOut) const
	{
		memset(pOut, 0xCC, TOTAL_SIZE);
		memcpy(pOut, m_Code, m_nCode);
		memcpy(pOut + CODE_SIZE, m_Data, m_nData);
	}

	enum { CODE_SIZE = 256, TOTAL_SIZE = 512 };

private:
	void B(BYTE b)		{ if (m_nCode < CODE_SIZE) m_Code[m_nCode++] = b; }
	void D(DWORD d)		{ for (int i = 0; i < 4; i++) B((BYTE)(d >> (i * 8))); }

	DWORD	m_dwBase;
	BYTE	m_Code[CODE_SIZE];
	int		m_nCode;
	char	m_Data[TOTAL_SIZE - CODE_SIZE];
	int		m_nData;
	int		m_nJzEnd;
};

inline int GetRegionCount(HANDLE hGame, DWORD dwSel)
{
	return (int)ReadGameDword(hGame, dwSel + LG_SEL_LEFTLIST + LG_LIST_COUNT) + (int)ReadGameDword(hGame, dwSel + LG_SEL_RIGHTLIST + LG_LIST_COUNT);
}

// Cửa sổ giao diện *dwSelfPtr đang mở và đang hiện => địa chỉ cửa sổ, ngược lại 0
inline DWORD GetVisibleGameWnd(HANDLE hGame, DWORD dwSelfPtr)
{
	DWORD dwWnd = ReadGameDword(hGame, dwSelfPtr);
	if (dwWnd && (ReadGameDword(hGame, dwWnd + 4) & LG_WND_S_VISIBLE))
		return dwWnd;
	return 0;
}

// Ghi mã (CRemoteCode tạo với base = pRemote) vào Game, chạy bằng CreateRemoteThread, chờ xong
// thì xóa vùng nhớ (có thể chứa mật khẩu) rồi giải phóng. Thread chưa xong sau dwTimeout thì
// không giải phóng (Game còn đang chạy đoạn mã) và trả về FALSE.
inline BOOL ExecRemoteCode(HANDLE hGame, LPVOID pRemote, const CRemoteCode& code, DWORD dwTimeout, DWORD* pdwExit)
{
	*pdwExit = 0;
	BYTE buf[CRemoteCode::TOTAL_SIZE];
	code.Build(buf);
	HANDLE hThread = NULL;
	if (WriteProcessMemory(hGame, pRemote, buf, sizeof(buf), NULL))
		hThread = CreateRemoteThread(hGame, NULL, 0, (LPTHREAD_START_ROUTINE)pRemote, NULL, 0, NULL);
	SecureZeroMemory(buf, sizeof(buf));
	if (!hThread)
	{
		VirtualFreeEx(hGame, pRemote, 0, MEM_RELEASE);
		return FALSE;
	}
	DWORD dwWait = WaitForSingleObject(hThread, dwTimeout);
	GetExitCodeThread(hThread, pdwExit);
	CloseHandle(hThread);
	if (dwWait != WAIT_OBJECT_0)
		return FALSE;
	WriteProcessMemory(hGame, pRemote, buf, sizeof(buf), NULL);	// buf đã xóa về 0
	VirtualFreeEx(hGame, pRemote, 0, MEM_RELEASE);
	return TRUE;
}
