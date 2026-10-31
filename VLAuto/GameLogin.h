#pragma once

// Dùng chung cho Tab Đăng nhập (DlgDebug) và Auto Login (DlgAutoLogin)

// Tab Đăng nhập: ở màn hình bắt đầu Game chưa nạp VLHook (chỉ gắn khi đã vào game)
// => VLAuto tự đọc bộ nhớ Game và tạo thread trong Game (CreateRemoteThread) để gọi hàm giao diện.
// Địa chỉ 15_10 (xem Offsets_15_10.md mục Đăng nhập)
#define LG_UIN_BASE_ADD		0x007C47D8	// KUiInit::m_pSelf
#define LG_IOC_FUNC_ADD		0x00490EA0	// KUiInit::OnClickButton(KWndButton*) thiscall
#define LG_SMC_BASE_ADD		0x007CA1FC	// KUiSysMsgCentre (!= 0 khi đang trong game)
#define LG_NOTICE_SELF		0x007BE5E8	// 15_10: bảng "Nhắc nhở" KUiHealthNotice::m_pSelf ("UiNewLogin\健康游戏公告.ini"), mở bởi Bắt Đầu Trò Chơi (0x483760)
#define LG_NOTICE_OK		0x037C		// KWndButton "Close" = nút Xác định
#define LG_NOTICE_WNDPROC	0x00483B10	// KUiHealthNotice::WndProc(msg, uParam, nParam) thiscall: 0x565 + nút Xác định => KUiSelServer::OpenWindow 0x486BE0, đóng bảng 0x483860(1)
#define LG_WND_BUTTON_CLICK	0x0565		// Thông báo nút được bấm (uParam = KWndButton*)
#define LG_SEL_SELF			0x007BE604	// KUiSelServer::m_pSelf ("UiNewLogin\选服务器.ini", OpenWindow 0x486BE0)
#define LG_SEL_LOGIN_FUNC	0x00487D00	// KUiSelServer::OnLogin() thiscall: máy chủ đang chọn -> mở cửa sổ đăng nhập
#define LG_SEL_REGIONS		0x06FC		// KLoginServer* danh sách KHU VỰC (0x5AC720, Settings\ServerList.ini: RegionCount, Region_%d), 0x24 byte/dòng
#define LG_SERVER_ROW		0x24		// KLoginServer: char Title[32] (TCVN3) + BYTE IP[4] ở +0x20 ("%d_Title" / "%d_Address", 0x5AC910)
#define LG_SEL_REGION		0x2080		// Chỉ số khu vực đang chọn
#define LG_SEL_LEFTLIST		0x0704		// KWndList: 17 khu vực đầu
#define LG_SEL_RIGHTLIST	0x0AB0		// KWndList: khu vực thứ 18 trở đi (0x486E73: count > 17)
#define LG_SEL_IPLIST		0x0E5C		// KWndList: MÁY CHỦ của khu vực đang chọn
#define LG_SEL_SERVERS		0x0700		// KLoginServer* máy chủ của khu vực đang chọn (GetServerList 0x5AC910(region, &count, &cursel)); số dòng = IpList count
#define LG_SEL_UPDATE		0x00487160	// KUiSelServer: khu vực đang chọn ở Left/RightList -> LG_SEL_REGION, nạp lại IpList + bắt đầu ping (0x488560)
#define LG_SEL_AUTOUPDATE	0x226C		// = 1 (OpenWindow 0x486CAE): Left/RightList SetCurSel báo 0x691 => WndProc tự gọi LG_SEL_UPDATE
										// KHÔNG gọi LG_SEL_UPDATE lần 2 ngay sau đó: 0x488560 TerminateThread thread ping vừa tạo
										// (+0x2244) khi nó đang khởi tạo (giữ loader lock) => mọi thread mới trong Game bị treo
#define LG_LIST_LEFT_NUM	17			// LeftList chứa tối đa 17 khu vực (10_10: 14)
#define LG_LIST_COUNT		0x029C		// KWndList::số dòng
#define LG_LIST_CURSEL		0x02B0		// KWndList::dòng đang chọn
#define LG_LIST_SETCURSEL	0x0044C4F0	// KWndList::SetCurSel(int) thiscall, báo 0x691 lên cửa sổ cha
#define LG_LIST_CLEARSEL	0x00480DD0	// KWndList: dòng đang chọn = -1 (không báo)
#define LG_LIST_FOCUS		0x00480C90	// KWndList: màu list đang được chọn (int) thiscall (+0x2C4 = +0x3A8 / +0x3A4)
#define LG_LOGIN_SELF		0x007BE5EC	// KUiLogin::m_pSelf ("UiNewLogin\登陆.ini", OpenWindow 0x483E70)
#define LG_LOGIN_ACCOUNT	0x0380		// KWndEdit m_Account
#define LG_LOGIN_PASSWORD	0x067C		// KWndEdit m_PassWord
#define LG_LOGIN_AGREE		0			// 15_10: cửa sổ đăng nhập KHÔNG có ô đồng ý (đã chuyển sang chọn nhân vật)
#define LG_LOGIN_FUNC		0x00484C20	// KUiLogin::OnLogin() thiscall
#define LG_EDIT_SETTEXT		0x004488F0	// KWndEdit::SetText(const char*) thiscall
#define LG_BTN_CHECK		0x00442DD0	// KWndButton::CheckButton(int) thiscall (cờ 0x200 ở +0x34C)
#define LG_ROLE_SELF		0x007C48AC	// KUiSelPlayer::m_pSelf ("UiNewLogin\选游戏存档人物.ini")
#define LG_ROLE_SELECT		0x0049FEE0	// KUiSelPlayer::OnSelectPlayer(int nSel, bool bPlaySound) thiscall
#define LG_ROLE_ENTER		0x004A0370	// KUiSelPlayer::OnEnterGame() thiscall: cần tick LG_ROLE_AGREE, SelectRole(m_nSelPlayer + trang) 0x5AAE40
#define LG_ROLE_AGREE		0x8464		// 15_10: ô tick "cbUserAgreement" (chưa tick thì OnEnterGame chỉ hiện thông báo)
#define LG_ROLE_NUM			0x7E1C		// short m_nNumPlayer (= số nhân vật)
#define LG_ROLE_SEL			0x7E1E		// short m_nSelPlayer (ô đang chọn trên trang, -1 = chưa chọn)
#define LG_ROLE_PAGE		0x7E28		// short: số nhân vật đã lật qua bằng Next/Pre
#define LG_ROLE_PER_PAGE	3			// Hiện 3 nhân vật mỗi trang ("Player3Pos_*")
#define LG_LOGIC			0x00848A68	// g_LoginLogic (KLogin)
#define LG_LOGIC_STATUS		0x0008		// m_Status
#define LG_LOGIC_STATUS_ROLES	6		// m_Status khi đã nhận danh sách nhân vật (SelectRole cần = 6; 10_10: 7)
#define LG_LOGIC_ROLENUM	0x0012		// short m_nNumRole
#define LG_LOGIC_ROLES		0x0014		// KRoleChiefInfo[]: Name[32], BYTE Gender +0x20, BYTE Attribute +0x21, int Level +0x24
#define LG_ROLE_INFO_SIZE	0x28		// 15_10 (10_10: 0x24, short Level +0x22)
#define LG_ROLE_INFO_LEVEL	0x24
#define LG_WND_S_VISIBLE	0x80000000
#define LG_CI_SELF			0x007BE5E4	// KUiConnectInfo::m_pSelf ("登陆过程提示.ini", OpenWindow 0x481D50): bảng thông báo lúc kết nối / đăng nhập
#define LG_CI_DESIRE		0x2814		// int m_nDesireLoginStatus: >= 1000 (CI_NS_*) hoặc 0 = thông báo lỗi, chờ bấm Xác định; còn lại = đang chờ trạng thái đăng nhập đó
#define LG_CI_MESSAGE		0x2818		// char m_szMessage[0x180] (TCVN3): sprintf(chuỗi InfoString[eIndex] của UiCommon, m_szParam) trong SetInfoMsg 0x4828E0
#define LG_CI_MESSAGE_SIZE	0x180
#define LG_CI_MSGLEN		0x2998		// int m_nMsgLen
#define LG_CI_NS_FIRST		1000		// CI_NS_INIT_WND

static const DWORD s_dwInitButtons[] = { 0x039C, 0x0E64, 0x23F4, 0x192C };
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

// Ghi vào bộ nhớ Game. Game 15_10: WriteProcessMemory từ Auto (32-bit) báo lỗi 6 (ERROR_INVALID_HANDLE) dù handle
// đủ quyền (VirtualAllocEx / ReadProcessMemory vẫn được) => ghi thẳng bằng NtWriteVirtualMemory
inline BOOL WriteGameMemory(HANDLE hGame, LPVOID pAddr, LPCVOID pData, SIZE_T nSize)
{
	if (WriteProcessMemory(hGame, pAddr, pData, nSize, NULL))
		return TRUE;
	typedef LONG (NTAPI *PFN_NtWriteVirtualMemory)(HANDLE, PVOID, PVOID, SIZE_T, SIZE_T*);
	static PFN_NtWriteVirtualMemory pfnWrite = (PFN_NtWriteVirtualMemory)GetProcAddress(GetModuleHandle(_T("ntdll.dll")), "NtWriteVirtualMemory");
	SIZE_T nWritten = 0;
	return pfnWrite && pfnWrite(hGame, pAddr, (PVOID)pData, nSize, &nWritten) >= 0 && nWritten == nSize;
}

// Ghi mã (CRemoteCode tạo với base = pRemote) vào Game, chạy bằng CreateRemoteThread, chờ xong
// thì xóa vùng nhớ (có thể chứa mật khẩu) rồi giải phóng. Thread chưa xong sau dwTimeout thì
// không giải phóng (Game còn đang chạy đoạn mã) và trả về FALSE; phPending != NULL thì nhận handle
// thread đó (người gọi chờ xong rồi xóa + giải phóng pRemote), ngược lại handle bị đóng.
inline BOOL ExecRemoteCode(HANDLE hGame, LPVOID pRemote, const CRemoteCode& code, DWORD dwTimeout, DWORD* pdwExit, HANDLE* phPending = NULL)
{
	if (phPending)
		*phPending = NULL;
	*pdwExit = 0;
	BYTE buf[CRemoteCode::TOTAL_SIZE];
	code.Build(buf);
	HANDLE hThread = NULL;
	if (WriteGameMemory(hGame, pRemote, buf, sizeof(buf)))
		hThread = CreateRemoteThread(hGame, NULL, 0, (LPTHREAD_START_ROUTINE)pRemote, NULL, 0, NULL);
	SecureZeroMemory(buf, sizeof(buf));
	if (!hThread)
	{
		VirtualFreeEx(hGame, pRemote, 0, MEM_RELEASE);
		return FALSE;
	}
	DWORD dwWait = WaitForSingleObject(hThread, dwTimeout);
	GetExitCodeThread(hThread, pdwExit);
	if (dwWait != WAIT_OBJECT_0 && phPending)
	{
		*phPending = hThread;
		return FALSE;
	}
	CloseHandle(hThread);
	if (dwWait != WAIT_OBJECT_0)
		return FALSE;
	WriteGameMemory(hGame, pRemote, buf, sizeof(buf));	// buf đã xóa về 0
	VirtualFreeEx(hGame, pRemote, 0, MEM_RELEASE);
	return TRUE;
}
