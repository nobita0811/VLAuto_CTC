#pragma once

////////////////////////////////////////////
// Common Classes & Structs
////////////////////////////////////////////
struct KUiPlayerItem
{
	char			Name[64];
	unsigned int	uId;
	int				nIndex;
	int				nData;
};

struct KOneMsgInfo
{
	int			 nLines;
	int			 nCharWidth;
	unsigned int uColor[3];		// 15-10: van dung (nLen 0x18, Msg 0x1C - KWndMessageListBox::AddOneMessage 0x44E6C0)
	int			 nMaxSize;
	int			 nLen;
	char		 Msg[1];
};

struct KShopObject
{
	unsigned int	uGenre;
	unsigned int	uId;
	int				DataX;
	int				DataY;
	int				DataW;
	int				DataH;
	int				eContainer;
	int				UnusedData[3];
};

struct KUiSkillData
{
	unsigned int	uGenre;
	unsigned int	uId;
	int				DataX;
	int				DataY;
};

class KWndImage
{
public:
	DWORD			m_dwWinHandle;
	unsigned int	m_Style;
public:
	BOOL IsVisible();
};

class KUiInit : public KWndImage
{
public:
	void ClickButton(DWORD dwButton);	// OnClickButton(this + dwButton)
	void EnterGame();
	void QuitGame();
};

class KUiMiniMap : public KWndImage
{
public:
	void SetMapFlag(int nDx, int nDy);
};

class KUiYeSouBox : public KWndImage
{
public:
	void OnConfirm(int btnYes = 1);
};

struct KSystemMessage
{
	char			szMessage[64];
	BYTE			Data01[0x03C8];	// 15-10: sizeof(KSystemMessage) = 0x408 (10-10: 0x108)
	KUiPlayerItem	mInviter;		// 15-10: msg+0x408 (TEAM_INVITE_RESPONSE 0x4D942C)
};

class KUiSysMsgCentre
{
public:
	BYTE			Data01[0x0294];	// 15-10: m_MsgHeap = this+0x294 (DeleteMsgInHeap 0x4DA540; 10-10: 0x4C8)
	struct SYS_MSG_HEAP
	{
		KSystemMessage**	pMsgList;
		int					nListSpace;
		int					nNumValid;
	} m_MsgHeap[8];

public:
	void DeleteMsgInHeap(int nHeapIndex, int nMsgIndex);
};

class KUiTongManagement
{
public:
	BYTE m_TongData[0x01AC];
	int m_nTongListPage;

public:
	int PrepareTongList(int nSortType);
	TongList* GetTongList();
};

////////////////////////////////////////////
// Specified Classes & Structs
////////////////////////////////////////////
#define	GSHOP_BUTTON		0x1378	// 15-10: nut Ky Tran Cac trong KUiPlayerBar (WndProc 0x46FCA0; 10-10: 0xCC4)
#define	ENTERGAME_BUTTON	0x039C	// 15-10: KUiInit "EnterGame" ("Bat Dau Tro Choi"; 10-10: 0x5A4)
#define	EXITGAME_BUTTON		0x192C	// 15-10: KUiInit "ExitGame" (10-10: 0x2364)
#define	CHATROOM_LIST		0		// CHUA TIM (0 = tat dieu khien qua tin nhan)

// 15-10: m_pSelf 0x7CBD2C, sizeof 0x1758. "Select" 0x37C (list ben trong 0x610),
// m_InfoText = KWndText512 "InfoText" 0x11E0, bo dem chu o +0x36C. KWndMessageListBox: pMessages +0x298, nNum +0x29C
class KUiMsgSel : public KWndImage
{
public:
	BYTE			m_MsgData1[0x08A0];
	KOneMsgInfo**	m_pMessages;		// 0x08A8 = 0x610 + 0x298
	int				m_nNumMessage;		// 0x08AC
	int				m_nCapability;
	BYTE			m_MsgData2[0x0C98];
	char			m_InfoText[512];	// 0x154C = 0x11E0 + 0x36C
public:
	void OnClickMsg(int nMsg);
};

// 15-10: m_pSelf 0x7CB470, sizeof 0x4588. "Select" 0xC4C (list 0xEE0), "Text" 0x20EC, "Image" 0x1DA0,
// "MessageList" 0x1AB0 = noi hien loi thoai NPC (list kieu rich: pMessages +0x294, nNum +0x29C).
// Show(KUiQuestionAndAnswer*) 0x5217B0: strcpy(this+0x2578, loi thoai) roi moi AddMessage vao MessageList
// => doc loi thoai o m_szQuestion (char[0x2000], ctor 0x5212A0 xoa 0x2000 byte, Clear 0x5219C0 gan [0] = 0)
class KUiMsgSel2 : public KWndImage
{
public:
	BYTE			m_MsgData1[0x1170];
	KOneMsgInfo**	m_pMessages;		// 0x1178 = 0xEE0 + 0x298
	int				m_nNumMessage;		// 0x117C
	int				m_nCapability;
	BYTE			m_MsgData2[0x0BC0];
	LPBYTE*			m_pInfoMsgs;		// 0x1D44 = 0x1AB0 + 0x294
	int				m_nInfoData;
	int				m_nNumInfoMsg;		// 0x1D4C
	BYTE			m_MsgData3[0x0828];
	char			m_szQuestion[0x2000];	// 0x2578: loi thoai NPC (TCVN3, con ma mau 0x02 r g b / 0x03)
public:
	// Loi thoai NPC dang hien, con ma mau (khong dung strlen/strstr truc tiep - xem GameTextLen)
	const char* GetQuestion() { return m_szQuestion; }
	// Loi thoai NPC da bo ma mau, dung duoc voi strstr/%s (bo dem tinh, goi lai se ghi de)
	const char* GetQuestionText();
	void OnClickMsg(int nMsg);
};

class KUiShop : public KWndImage
{
public:
	BYTE			ShopData1[0x0374];
	KItemObject		*m_pObjectList;		// 0x037C (15-10: UpdateData 0x4CBCC0; 10-10: 0x584)
	BYTE			ShopData2[0x001C];
	int				m_nObjCount;		// 0x039C
	int				m_nPageCount;
	int				m_nCurrentPage;
};

class KUiPlayerBar : public KWndImage
{
public:
	BYTE			BarData1[0x2B6C];
	int				m_nHK3Gengre;		// 0x2B74 = m_ImmediaItem[2].m_Object.uGenre (15-10: 0x22E4 + 2*0x2F0 + 0x2B0)
public:
	void OnClickGShop();
};

class KUiInformation : public KWndImage
{
public:
	BYTE			InfoData[0x06E0];
	char			m_szInfoText[128];		// 0x06E8 = "Info" 0x37C + 0x36C (15-10)
public:
	void Hide(int nBtnIndex);
};

class KUiInformation2 : public KWndImage
{
public:
	BYTE			InfoData[0x06E0];
	char			m_szInfoText[128];		// 0x06E8 = "Info" 0x37C + 0x36C (15-10)
public:
	void Hide();
};

// 15-10: m_pSelf 0x7CAD54 (scheme "bai tan vat pham", new 0x103C): "CloseBtn" 0x37C, "ItemBox" 0x8E0, "Owner" 0xBCC.
// Cua so khong giu danh sach hang: UpdateData 0x5187B0 goi GetGameData(GDI_STALL_ITEMS) vao bo dem tam roi do vao ItemBox.
struct KStallObject;
class KUiPlayerStall : public KWndImage
{
public:
	// Lay hang tren sap (toi da nMax mon, so mon thuc > nMax => 0). Tra ve so mon.
	int GetObjects(KStallObject* pBuf, int nMax);
};

// 15-10: moi phan thuong la 1 nut "PriseBtn1..3" (0x8E8 / 0x189C / 0x2850, KWndButton 0xFB4 byte).
// Du lieu "chu/duong dan anh" tach o 0x53EDD0: anh -> nut+0x2B4 (SetImage 0x4494E0), chu -> nut+0x358 (SetLabel vt+0x38 = 0x443580)
struct PrizeItem
{
	BYTE			PrzData1[0x02B4];
	char			m_szIcon[0x0080];		// +0x2B4
	BYTE			PrzData2[0x0024];
	char			m_szText[0x0200];		// +0x358 (do dai o +0x558)
	BYTE			PrzData3[0x0A5C];
};

// 15-10: m_pSelf 0x7CBD1C (scheme "phan thuong", new 0x3804), "PriseText" 0x37C
class KUiPrizeSel : public KWndImage
{
public:
	BYTE			PrzData1[0x08E0];
	PrizeItem		PrzItem[3];				// 0x08E8
public:
	void OnClickBtn(int nBtn);
};

class KWndMessageListBox
{
public:
	BYTE m_MsgListBoxData[0x0204];
	KOneMsgInfo**	m_pMessages;		//0x1E38
	int				m_nNumMessage;
	int				m_nCapability;
};

////////////////////////////////////////////
// Global Functions & Variables
////////////////////////////////////////////
void SendPrivateMessage(const char *szName, const char *szMsg);
void SystemMessage(const char *szMsg, int nChanel = 0);
void SystemMessage(int nChanel, const char *szMsgFormat, ...);

// Chu cua game (TCVN3) co ma mau 0x02 r g b (3 byte mau, CO THE = 0, vd vang FF C7 00) va 0x03 (het mau)
// => strlen/strstr dung som. Hai ham duoi day bo qua ca 3 byte mau.
int GameTextLen(const char *szText, int nMax);
int GameTextPlain(const char *szText, int nMax, char *szOut, int nOutSize);	// Bo ma mau, tra ve do dai

extern KUiInformation*	g_UiInformation;
extern KUiInformation2* g_UiInformation2;

extern void(*ProcessPeople)(KUiPlayerItem*, int);
extern void(*OnSendSomeoneMessage)(const char*, const char*, int);
extern void(*SystemMessageArrival)(const char*, unsigned short, int);
extern void(*OpenStoreWindow)();
extern void(*OpenShopWindow)();
extern void(*CloseAllWindows)(int);

// Kiem tra layout 15_10 luc bien dich
#define CHECK_UI(c, m, o) static_assert(offsetof(c, m) == (o), #c "::" #m " sai offset")
CHECK_UI(KOneMsgInfo, Msg, 0x001C);
CHECK_UI(KUiMsgSel, m_pMessages, 0x08A8);		CHECK_UI(KUiMsgSel, m_nNumMessage, 0x08AC);		CHECK_UI(KUiMsgSel, m_InfoText, 0x154C);
CHECK_UI(KUiMsgSel2, m_pMessages, 0x1178);		CHECK_UI(KUiMsgSel2, m_nNumMessage, 0x117C);	CHECK_UI(KUiMsgSel2, m_pInfoMsgs, 0x1D44);
CHECK_UI(KUiMsgSel2, m_nNumInfoMsg, 0x1D4C);
CHECK_UI(KUiMsgSel2, m_szQuestion, 0x2578);
static_assert(sizeof(KUiMsgSel2) <= 0x4588, "KUiMsgSel2 vuot qua kich thuoc that (new 0x4588)");
CHECK_UI(KUiShop, m_pObjectList, 0x037C);		CHECK_UI(KUiShop, m_nObjCount, 0x039C);			CHECK_UI(KUiShop, m_nCurrentPage, 0x03A4);
CHECK_UI(KUiInformation, m_szInfoText, 0x06E8);	CHECK_UI(KUiInformation2, m_szInfoText, 0x06E8);
CHECK_UI(KUiSysMsgCentre, m_MsgHeap, 0x0294);	CHECK_UI(KSystemMessage, mInviter, 0x0408);
static_assert(sizeof(KItemObject) == 0x1C, "KItemObject sai kich thuoc");
static_assert(sizeof(PrizeItem) == 0xFB4, "PrizeItem sai kich thuoc");
CHECK_UI(KUiPrizeSel, PrzItem, 0x08E8);			CHECK_UI(PrizeItem, m_szIcon, 0x02B4);			CHECK_UI(PrizeItem, m_szText, 0x0358);
#undef CHECK_UI
static_assert(offsetof(KUiPlayerBar, m_nHK3Gengre) == 0x2B74, "KUiPlayerBar::m_nHK3Gengre sai offset");
