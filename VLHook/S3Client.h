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
	unsigned int uColor[3];		// 10-10: van dung (nLen 0x18, Msg 0x1C - KWndMessageListBox::GetOneMessage 0x45E110)
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
	BYTE			Data01[0x00C8];	// 10-10: sizeof(KSystemMessage) = 0x108
	KUiPlayerItem	mInviter;		// 10-10: msg+0x108 (KUiSysMsgCentre 0x4C5822)
};

class KUiSysMsgCentre
{
public:
	BYTE			Data01[0x04C8];	// 10-10: m_MsgHeap = this+0x4C8 (DeleteMsgInHeap 0x4C5390)
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
#define	GSHOP_BUTTON		0x0CC4	// 10-10: nut Ky Tran Cac trong KUiPlayerBar
#define	ENTERGAME_BUTTON	0x05A4	// 10-10: KUiInit::m_EnterGame ("Bat Dau Tro Choi")
#define	EXITGAME_BUTTON		0x2364	// 10-10: KUiInit::m_ExitGame
#define	CHATROOM_LIST		0		// 10-10 CHUA TIM (0 = tat dieu khien qua tin nhan)

// 10-10: m_pSelf 0x8C13D8, sizeof 0x2380. "Select" 0x584 (list ben trong 0xA18),
// m_InfoText = KWndText512 "InfoText" 0x1BFC, bo dem chu o +0x578
class KUiMsgSel : public KWndImage
{
public:
	BYTE			m_MsgData1[0x0EA8];
	KOneMsgInfo**	m_pMessages;		// 0x0EB0 = 0xA18 + 0x498
	int				m_nNumMessage;		// 0x0EB4 (GetMsgCount 0x51EBC0)
	int				m_nCapability;
	BYTE			m_MsgData2[0x12B8];
	char			m_InfoText[512];	// 0x2174 = 0x1BFC + 0x578
public:
	void OnClickMsg(int nMsg);
};

// 10-10: m_pSelf 0x8C1368, sizeof 0x4DCC. "Select" 0x1268 (list 0x16FC), "Text" 0x3324, "Image" 0x2DD0,
// "MessageList" 0x28E0 = loi thoai NPC, list kieu rich: pMessages +0x494, nNum +0x49C, chu o msg+0x10D8
class KUiMsgSel2 : public KWndImage
{
public:
	BYTE			m_MsgData1[0x1B8C];
	KOneMsgInfo**	m_pMessages;		// 0x1B94 = 0x16FC + 0x498
	int				m_nNumMessage;		// 0x1B98
	int				m_nCapability;
	BYTE			m_MsgData2[0x11D4];
	LPBYTE*			m_pInfoMsgs;		// 0x2D74 = 0x28E0 + 0x494
	int				m_nInfoData;
	int				m_nNumInfoMsg;		// 0x2D7C
public:
	// Dong thu i cua loi thoai NPC (NULL neu khong co)
	char* GetInfoText(int i) { return (i >= 0 && i < m_nNumInfoMsg && m_pInfoMsgs && m_pInfoMsgs[i]) ? (char*)(m_pInfoMsgs[i] + 0x10D8) : NULL; }
	void OnClickMsg(int nMsg);
};

class KUiShop : public KWndImage
{
public:
	BYTE			ShopData1[0x057C];
	KItemObject		*m_pObjectList;		// 0x0584 (10-10: UpdateData 0x4BEDA0)
	BYTE			ShopData2[0x001C];
	int				m_nObjCount;		// 0x05A4
	int				m_nPageCount;
	int				m_nCurrentPage;
};

class KUiPlayerBar : public KWndImage
{
public:
	BYTE			BarData1[0x3258];
	int				m_nHK3Gengre;		// 0x3260 = m_ImmediaItem[2].m_Object.uGenre (10-10: 0x23D0 + 2*0x4F0 + 0x4B0)
public:
	void OnClickGShop();
};

class KUiInformation : public KWndImage
{
public:
	BYTE			InfoData[0x0AF4];
	char			m_szInfoText[128];		// 0x0AFC = "Info" 0x584 + 0x578 (10-10)
public:
	void Hide(int nBtnIndex);
};

class KUiInformation2 : public KWndImage
{
public:
	BYTE			InfoData[0x0AF4];
	char			m_szInfoText[128];		// 0x0AFC = "Info" 0x584 + 0x578 (10-10)
public:
	void Hide();
};

class KUiPlayerStall : public KWndImage
{
public:
	BYTE			StallData[0x055C];
	int				m_nObjCount;			// 0x0564
	KShopObject		*m_pObjectList;
};

struct PrizeItem
{
	BYTE			PrzData1[0x06C4];
	char			m_szIcon[0x0080];
	BYTE			PrzData2[0x0020];
	char			m_szText[0x0080];
};

class KUiPrizeSel : public KWndImage
{
public:
	BYTE			PrzData1[0x0018];
	PrizeItem		PrzItem[3];
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

extern KUiInformation*	g_UiInformation;
extern KUiInformation2* g_UiInformation2;

extern void(*ProcessPeople)(KUiPlayerItem*, int);
extern void(*OnSendSomeoneMessage)(const char*, const char*, int);
extern void(*SystemMessageArrival)(const char*, unsigned short, int);
extern void(*OpenStoreWindow)();
extern void(*OpenShopWindow)();
extern void(*CloseAllWindows)(int);

// Kiem tra layout 10_10 luc bien dich
#define CHECK_UI(c, m, o) static_assert(offsetof(c, m) == (o), #c "::" #m " sai offset")
CHECK_UI(KOneMsgInfo, Msg, 0x001C);
CHECK_UI(KUiMsgSel, m_pMessages, 0x0EB0);		CHECK_UI(KUiMsgSel, m_nNumMessage, 0x0EB4);		CHECK_UI(KUiMsgSel, m_InfoText, 0x2174);
CHECK_UI(KUiMsgSel2, m_pMessages, 0x1B94);		CHECK_UI(KUiMsgSel2, m_nNumMessage, 0x1B98);	CHECK_UI(KUiMsgSel2, m_pInfoMsgs, 0x2D74);
CHECK_UI(KUiMsgSel2, m_nNumInfoMsg, 0x2D7C);
CHECK_UI(KUiShop, m_pObjectList, 0x0584);		CHECK_UI(KUiShop, m_nObjCount, 0x05A4);			CHECK_UI(KUiShop, m_nCurrentPage, 0x05AC);
CHECK_UI(KUiInformation, m_szInfoText, 0x0AFC);	CHECK_UI(KUiInformation2, m_szInfoText, 0x0AFC);
CHECK_UI(KUiSysMsgCentre, m_MsgHeap, 0x04C8);	CHECK_UI(KSystemMessage, mInviter, 0x0108);
static_assert(sizeof(KItemObject) == 0x1C, "KItemObject sai kich thuoc");
#undef CHECK_UI
static_assert(offsetof(KUiPlayerBar, m_nHK3Gengre) == 0x3260, "KUiPlayerBar::m_nHK3Gengre sai offset");
