#ifndef VLTK_FREE_MODE
#define	EXITGAME_BUTTON		0x0880
#define COMPOUND_MENU		0x4E2C
#define	COMPOUND_BUTTON		0x0684
#define	COMPOUND_WIND		0x0170

class KUiMsgSel : public KWndImage
{
public:
	BYTE			m_MsgData1[0x024C];
	KOneMsgInfo**	m_pMessages;		// 0x0254
	int				m_nNumMessage;		// 0x0258
	int				m_nCapability;
	BYTE			m_MsgData2[0x04B0];
	int				m_nInfoLen;
	int				m_nInfoColor;
	int				m_nInfoLines;
	char			m_InfoText[512];	// 0x071C
public:
	void OnClickMsg(int nMsg);
};

class KUiMsgSel2 : public KWndImage
{
public:
	BYTE			m_MsgData1[0x055C];
	KOneMsgInfo**	m_pMessages;		// 0x0564
	int				m_nNumMessage;		// 0x0568
	int				m_nCapability;
	BYTE			m_MsgData2[0x07E0];
	char			m_InfoText[512];	// 0x0D50
public:
	void OnClickMsg(int nMsg);
};

class KUiShop : public KWndImage
{
public:
	BYTE			ShopData1[0x0158];
	KItemObject		*m_pObjectList;		// 0x0160
	BYTE			ShopData2[0x001C];
	int				m_nObjCount;		// 0x0180
	int				m_nPageCount;
	int				m_nCurrentPage;
};

class KUiPlayerBar : public KWndImage
{
public:
	BYTE			BarData1[0x0668];
	DWORD			m_dwGShopHandle;	// 0x0670
	BYTE			BarData2[0x06B0];
	int				m_nHK3Gengre;		// 0x0D24
public:
	void OnClickGShop(int nButtAdd);
};

class KUiInformation : public KWndImage
{
public:
	BYTE			InfoData[0x02A8];
	char			m_szInfoText[128];		// 0x02B0
public:
	void Hide(int nBtnIndex);
};

class KUiInformation2 : public KWndImage
{
public:
	BYTE			InfoData[0x02A8];
	char			m_szInfoText[128];		// 0x02B0
public:
	void Hide();
};

class KUiPlayerStall : public KWndImage
{
public:
	BYTE			StallData[0x03BC];
	int				m_nObjCount;			// 0x03C4
	KShopObject		*m_pObjectList;
};

struct PrizeItem
{
	BYTE			PrzData1[0x0498];
	char			m_szIcon[0x0080];
	BYTE			PrzData2[0x0024];
	char			m_szText[0x0080];
};

class KUiPrizeSel : public KWndImage
{
public:
	BYTE			PrzData1[0x00A8];
	PrizeItem		PrzItem[3];
public:
	void OnClickBtn(int nBtn);
};

class KWndMessageListBox
{
public:
	BYTE m_MsgListBoxData[0x007C];
	KOneMsgInfo**	m_pMessages;
	int				m_nNumMessage;
	int				m_nCapability;
};

#endif
