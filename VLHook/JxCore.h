#pragma once

#include "KNode.h"
#include <stddef.h>

////////////////////////////////////////////
// Common Definitions
////////////////////////////////////////////
#define GetPtrAddress(p) (*(PDWORD_PTR)p)
#define P_PLAYER g_pNpcArray[g_pCurPlayer->m_nIndex]
#define PEOPLE_IDX P_PLAYER->m_nPeopleIdx
#define HAVE_HORSE g_pCurPlayer->m_ItemList.m_EquipItem[itempart_horse] > 0
#define ItmAttr(j) pItem->m_aryAllAttrib[j].nAttribType
#define ComAttr pItem->m_CommonAttrib
#define PLAYER_ROOM g_pCurPlayer->m_ItemList.m_Room
#define EQUIP_ROOM PLAYER_ROOM[room_equipment]
#define EQUIP_MONEY PLAYER_ROOM[room_equipment].m_nMoney
#define ROOM_MONEY PLAYER_ROOM[room_repository].m_nMoney
#define PLAYER_ITEM(i) g_pCurPlayer->m_ItemList.m_Items[i]
#define IAM_IN_A_PARTY g_pCurPlayer->m_cTeam.m_nFlag
#define IAM_THE_CAPTAIN g_pCurPlayer->m_cTeam.m_nFigure == 0
#define PLAY_HAND_IDX g_pCurPlayer->m_ItemList.m_nForeHand

////////////////////////////////////////////
// Common Classes & Structs
////////////////////////////////////////////

struct KSubWorld
{
	int			m_ReservedData[12];
	int			m_nWorldRegionWidth;
	int			m_nWorldRegionHeight;
	int			m_nTotalRegion;
	int			m_nCellWidth;
	int			m_nCellHeight;
	int			m_nRegionBeginX;
	int			m_nRegionBeginY;
	int			m_nWeather;
	DWORD		m_dwCurrentTime;				// 0x0050
};

class KSkill
{
public:
	int			m_nIndex;
	int			m_nSkillType;
	int			m_nMaxLevel;
	char		m_szSkillName[32];
	DWORD		m_dwSkillID;
	BYTE		SkillData1[0x0020];
	int			m_nSkillRange;			// 0x50 = cot AttackRadius
	BYTE		SkillData2[0x0064];		// 10-10 (cu 0x60)
	int			m_nHorseLimited;		// 0xB8 = cot HorseLimit (KSkill load 0x6F7DB0)
};

struct ObjSkill
{
	int			m_nKind;
	int			m_nCamp;
	int			m_nRange;
	int			m_nCastTime;
	int			m_nID;
	int			m_nLevel;
	int			m_nTarX;
	int			m_nTarY;
};

class KObj
{
public:
	int			m_nID;
	int			m_nDataID;
	int			m_Kind;
	int			m_Index;
	WORD		m_RegX;
	WORD		m_RegY;
	int			m_SubWorldID;
	int			m_RegionIdx;
	int			m_MapX;
	int			m_MapY;
	int			m_OffX;
	int			m_OffY;
	int			m_nDir;
	int			m_nState;
	int			m_nLifeTime;
	int			m_nBornTime;
	int			m_nWaitTime;
	ObjSkill	m_cSkill;			// 0x0040
	char		m_szName[32];		// 0x0060
	DWORD		m_dwScriptID;
	int			m_nColorID;
	int			m_nItemDataID;		// 0x0088
	int			m_nItemWidth;
	int			m_nItemHeight;
	int			m_nMoneyNum;
public:
	void GetMapPos(int *nX, int *nY);
};

struct KItemObject
{
	unsigned	uGenre;
	unsigned	uId;
	int			DataX;
	int			DataY;
	int			DataW;
	int			DataH;
	int			eContainer;
};

struct KCommonAttrib
{
	int			nItemGenre;
	int			nSpecialType;
	int			nDetailType;
	int			nParticularType;
	int			nObjIdx;
	BOOL		bStack;
	int			nWidth;
	int			nHeight;
	int			nPrice;
	int			nLevel;
	int			nSeries;
	char		szItemName[80];
	char		szImageName[80];
	char		szIntro[256];
	char		szScript[84];
	int			nNpcImage;
};

struct KIndexNode
{
	LPDWORD			__vfptr;
	KIndexNode*		m_pNext;
	KIndexNode*		m_pPrev;
	DWORD			NodeData[3];
};

struct KNpcGold
{
	int		m_nNpcIdx;
	int		m_nIsGold;
	int		m_nIsGolding;
	int		m_nGoldType;
};

struct KState
{
	int		nMagicAttrib;
	int		nValue[2];
	int		nTime;
};

struct KMagicAttrib
{
	int		nAttribType;
	int		nValue[3];

	KMagicAttrib()
	{
		nValue[0] = nValue[1] = nValue[2] = nAttribType = 0;
	}
};

class KStateNode : public KNode
{
public:
	int				m_nData;
	int				m_SkillID;
	int				m_Level;
	int				m_LeftTime;
	KMagicAttrib	m_State[10];
	int				m_StateGraphics;
};

struct NpcCommand
{
	NPCCMD		CmdKind;
	int			Param_X;
	int			Param_Y;
	int			Param_Z;
};

struct NpcSkill
{
	int		SkillId;
	int		SkillLevel;
	int		Reserverds[10];
};

struct PlayerItem
{
	int		nIdx;
	int		nPlace;
	int		nX;
	int		nY;
	int		nZ;
};

class KInventory
{
public:
	int*	m_pArray;
	int		m_nMoney;
	int		m_nWidth;
	int		m_nHeight;
	int		m_nMaxWidth;
	int		m_nMaxHeight;
	int		m_nReserved;
public:
	BOOL	FindRoom(int nWidth, int nHeight, POINT* pPos, BOOL bReversed = 0, int itemIdx = -1);
	BOOL	CanPlaceItem(int nX, int nY, int nWidth, int nHeight, int itemIdx = -1);
};

class KPlayerTeam
{
public:
	int		m_nFlag;
	int		m_nFigure;
};

class KPlayerTong
{
public:
	char		m_szMasterName[32];
	int			m_nUnusedData[3];
	int			m_nFlag;
	int			m_nFigure;
	int			m_nCamp;
	DWORD		m_dwTongNameID;			//0x5B28
	char		m_szTongName[32];
	BYTE		m_TongData[0x0068];
	int			m_nContributePoint;		//0x5BB4
};

class KCoreShell
{
public:
	int		GetGameData(unsigned int nRequest, int uParam, int nParam);
	int		OperationRequest(unsigned int nRequest, int uParam, int nParam);
	int		GetNPCRelation(int nIndex);
	int		TongOperation(unsigned int nRequest, int uParam, int nParam);
	int		TeamOperation(unsigned int nRequest, int uParam, int nParam);
	int		ProcessGameSecurity(unsigned int nRequest, int uParam, int nParam);
};

////////////////////////////////////////////
// Specified Classes & Structs
////////////////////////////////////////////
struct KStallObject
{
	unsigned int	uId;
	int				Data01[15];
	int				DataX;
	int				DataY;
	int				DataW;
	int				DataH;
	int				nPrice;						//0x0040
	int				Data02[2];
};

class KTeam
{
public:
	int		m_nMember[MAX_TEAM_MEMBER];
	int		m_nMemNum;
	int		m_nMemNumEx;
	int		m_nMemLevel[MAX_TEAM_MEMBER];
	char	m_szMemName[MAX_TEAM_MEMBER][32];
};

// ==========================================================================
// Layout ban client 10_10 (gamecl_dump_SCY_10_10.exe)
// Do bang cach doi chieu Src_JX voi disassembly. Xem Offsets_10_10.md.
// Offset da xac minh co static_assert ben duoi; "(?)" = chua xac minh.
// ==========================================================================
class KNpc
{
public:
	DWORD				m_dwVNData;				// 0x0000 (VN) con tro buffer 0x401 byte
	DWORD				m_dwUnknown04;			// 0x0004
	DWORD				m_dwID;					// 0x0008
	int					m_Index;				// 0x000C
	KIndexNode			m_Node;					// 0x0010
	int					m_Level;				// 0x0028
	DWORD				m_Kind;					// 0x002C
	int					m_Series;				// 0x0030
	int					m_Height;				// 0x0034
	BYTE				m_btRankId;				// 0x0038
	int					m_nStature;				// 0x003C
	int					m_nVNFlags[3];			// 0x0040 (VN) bit co tu SyncNpc
	KNpcGold			m_cGold;				// 0x004C
	BYTE				NpcData1[0x0028];
	KState				m_PowerState;			// 0x0084
	KState				m_PoisonState;			// 0x0094
	KState				m_FreezeState;
	KState				m_BurnState;
	KState				m_ConfuseState;
	KState				m_StunState;
	KState				m_DrunkState;
	int					m_Camp;					// 0x00F4
	int					m_CurrentCamp;			// 0x00F8
	NPCCMD				m_Doing;				// 0x00FC
	BYTE				NpcData2[0x0010];
	KStateNode*			m_pHeadNode;			// 0x0110 KList{head,tail,count}
	KStateNode*			m_pTailNode;			// 0x0114
	int					m_nStateCount;			// 0x0118
	int					m_ActiveSkillID;		// 0x011C
	BYTE				NpcData3[0x003C];		// 0x0124 = KSkillList (phan tu 0 khong dung)
	NpcSkill			m_Skills[MAX_NPCSKILL];	// 0x015C = KSkillList.m_Skills[1]
	int					m_CurrentLife;			// 0x105C
	int					m_LifeRep[4];
	int					m_CurrentMana;			// 0x1070
	int					m_ManaRep;
	int					m_CurrentStamina;		// 0x1078
	int					m_CurrentStaminaMax;	// 0x107C
	BYTE				NpcData4[0x00D0];
	int					m_CurrentWalkSpeed;		// 0x1150
	int					m_CurrentRunSpeed;		// 0x1154
	BYTE				NpcData5[0x0018];
	int					m_CurrentAttackRadius;	// 0x1170
	int					m_CurrentActiveRadius;	// 0x1174
	BYTE				NpcData6[0x0218];
	int					m_Dir;					// 0x1390
	int					m_RedLum;
	int					m_GreenLum;
	int					m_BlueLum;
	int					m_MapX, m_MapY, m_MapZ;	// 0x13A0
	int					m_OffX, m_OffY;			// 0x13AC
	BYTE				NpcData7[0x0054];
	char				Name0;					// 0x1408
	char				Name[39];				// 0x1409
	int					m_nSex;					// 0x1430
	int					m_NpcSettingIdx;		// 0x1434
	int					m_CorpseSettingIdx;		// 0x1438
	BYTE				NpcData8[0x005C];
	int					m_nPeopleIdx;			// 0x1498
	int					m_nLastDam[3];
	int					m_nObjectIdx;			// 0x14A8
	BYTE				NpcData9[0x020C];
	int					m_SyncSignal;			// 0x16B8
	int					m_nUnknown16BC;
	int					m_sClientNpcID[2];		// 0x16C0
	WORD				m_RegX;					// 0x16C8 = LOWORD(m_dwRegionID)
	WORD				m_RegY;					// 0x16CA = HIWORD(m_dwRegionID)
	int					m_sSyncPos[6];			// 0x16CC
	int					m_nPKFlag;				// 0x16E4
	BYTE				NpcDataA[0x0128];
	DWORD				m_dwTongNameID;			// 0x1810 (?) chua xac minh
	BOOL				m_bHaveStall;			// 0x1814 (?) chua xac minh
	BYTE				NpcDataB[0x0108];
	BOOL				m_FightMode;			// 0x1920
	BYTE				NpcDataC[0x003C];
	NpcCommand			m_Command;				// 0x1960
	BYTE				NpcDataD[0x0050];
	BOOL				m_bRideHorse;			// 0x19C0
	BYTE				NpcDataE[0x11150];
	int					m_CurrentLifeMax;		// 0x12B14 game dung max(LifeMax, LifeMaxEx)
	int					m_CurrentLifeMaxEx;		// 0x12B18
	int					m_CurrentManaMax;		// 0x12B1C game dung max(ManaMax, ManaMaxEx)
	int					m_CurrentManaMaxEx;		// 0x12B20
	BYTE				NpcDataF[0x00B0];

public:
	int GetLifeMax() { return m_CurrentLifeMax > m_CurrentLifeMaxEx ? m_CurrentLifeMax : m_CurrentLifeMaxEx; }
	int GetManaMax() { return m_CurrentManaMax > m_CurrentManaMaxEx ? m_CurrentManaMax : m_CurrentManaMaxEx; }
	BOOL IsExist();
	BOOL IsNotMob();
	void GetMapPos(int *nX, int *nY);
	void MoveTo(int nX, int nY, int nAction = 0);
	void SendCommand(NPCCMD cmd, int x = 0, int y = 0, int z = 0);
};

class KItemList
{
public:
	BYTE		m_ItemData0[0x001C];
	int			m_nForeHand;				// 0x001C m_Hand (vat dang cam tren chuot)
	int			m_nBackHand;				// 0x0020 (?)
	int			m_ItemData1;
	int			m_EquipItem[itempart_num];	// 0x0028 (10-10: int[], truoc la {m_nIndex,m_nData})
	PlayerItem	m_Items[MAX_PLAYER_ITEM];	// 0x006C
	BYTE		m_ItemData2[0x2634];
	KInventory	m_Room[room_num];			// 0x4C5C
};

class KPlayerTask
{
public:
	BYTE	m_TaskData[0x06D8];
public:
	int GetTaskVal(int nTaskNum);
};

class KPlayer
{
public:
	LPDWORD			__vfptr;
	BYTE			PlayerData0[0x0024];
	BYTE			m_nEntrustMode;			// 0x0028 game doc nhu BYTE
	BYTE			m_btEntrustPad[3];
	int				m_nLeftSkillID;			// 0x002C
	int				m_nLeftSkillLevel;
	int				m_nRightSkillID;
	int				m_nRightSkillLevel;
	BYTE			PlayerData1[0x17CC];
	int				m_RunStatus;			// 0x1808
	DWORD			m_dwNextLevelLeadExp;
	int				m_nSendMoveFrames;		// 0x1810
	BYTE			PlayerData2[0x003C];
	DWORD			m_dwHashID;				// 0x1850 = m_dwID
	int				m_nIndex;				// 0x1854
	int				m_nNetConnectIdx;
	KItemList		m_ItemList;				// 0x185C
	BYTE			PlayerData3[0x0138];
	int				m_nCurShop;				// 0x6794 = m_BuyInfo.m_nBuyIdx
	BYTE			PlayerData4[0x0A88];
	__int64			m_nCurExp;				// 0x7220
	__int64			m_nNextExp;				// 0x7228
	BYTE			PlayerData5[0x0028];
	KPlayerTeam		m_cTeam;				// 0x7258
	BYTE			PlayerData6[0x05A0];
	int				m_nCurFaction;			// 0x7800 m_cFaction.m_nCurFaction
	BYTE			PlayerData7[0x029C];
	int				m_nPKValue;				// 0x7AA0 = m_cPK(0x7A48) + 0x58
	BYTE			PlayerData8[0x0258];
	KPlayerTong		m_cTong;				// 0x7CFC
	BYTE			PlayerData9[0x23DC];
	KPlayerTask		m_cTask;				// 0xA1A0

public:
	void DoSit();
	void MoveTo(KNpc* pNpc);
	void MoveTo(int nX, int nY);
	void GoTo(int nX, int nY);
	int GetTaskVal(int nTaskNum);
	void PickUpObj(int nobjIndex);
	int CheckRoom(int nRoom = room_equipment);
	int GetItemsCount(const char *szItemName, int nPlace = pos_equiproom);
	int GetItemsCount(int nGenre, int nDetailType, int nParticular = -1, int nSeries = -1, int nPlace = pos_equiproom);
	BOOL UseItem(const char *szItemName);
	BOOL EatMedicine(int nDetailType);
	void BuyItem(int nItemIdx);
	void SellItem(int nItemIdx);
	void RepairItem(int nItemIdx);
	BOOL DecomposeItem(int nItemIdx);	// Ra trang bi trong hanh trang thanh "Manh Trang Bi"
	void SwitchItem(int nContainer, int nX, int nY);
	void DialogNpc(int nIndex);
	BOOL EnterTongMap(int nStep = 0, DWORD dwTongID = 0); // nStep = 0,1,2
	void ApplyAddBaseAttribute(int nAttribute, int nNo);
};

class KItem
{
public:
	LPDWORD				__vfptr;
	KCommonAttrib		m_CommonAttrib;
	KMagicAttrib		m_aryAllAttrib[19];
	BYTE				ItemData1[0x54];		// 10-10 (cu 0x34); m_aryAllAttrib = 0x228 da khop (0x2F8 = [13])
	int					m_MagicLevel[6];		// 0x3AC - KItem::GetTotalMagicLevel 0x634650
	int					m_nUnUsed[6];
	BYTE				ItemData1b[0x104];		// 10-10
	DWORD				m_dwUniID;				// 0x4E0 - goi 0x7A trong 0x5BB7A0
	int					m_nCurDur;				// 0x4E4 - KItem::GetRepairPrice 0x63A500
	int					m_nMaxDur;				// 0x4E8
	BYTE				ItemData2[0x28];
	BYTE				m_nItemPlus;			// 0x514 = "+N" cuong hoa (GetDesc 0x639563 "%s+%d%s"); ItemExParam[0]
	BYTE				m_btExParam1;			// 0x515 = ItemExParam[1]
	WORD				m_wExParam2;			// 0x516
	int					m_nLeftUsageTime;		// 0x518 (lua ITEM_GetLeftUsageTime)
	int					m_nExpiredTime;			// 0x51C (lua ITEM_GetExpiredTime)
	int					m_nLockCount;			// 0x520 khoa: -1 vinh vien, >0 co thoi han, 0 khong khoa (s2c 0x65C110)

public:
	int GetTotalMagicLevel();
	int	GetRepairPrice();
};

class KBuySell
{
public:
	KItem*		GetItem(int nIndex);
	int			GetItemIndex(int nBuyIdx, int nID);
};

// Kiem tra layout 10_10 luc bien dich
#define CHECK_OFF(c, m, o) static_assert(offsetof(c, m) == (o), #c "::" #m " sai offset")
CHECK_OFF(KNpc, m_dwID, 0x0008);			CHECK_OFF(KNpc, m_Index, 0x000C);
CHECK_OFF(KNpc, m_Level, 0x0028);			CHECK_OFF(KNpc, m_Kind, 0x002C);
CHECK_OFF(KNpc, m_Series, 0x0030);			CHECK_OFF(KNpc, m_btRankId, 0x0038);
CHECK_OFF(KNpc, m_cGold, 0x004C);			CHECK_OFF(KNpc, m_PoisonState, 0x0094);
CHECK_OFF(KNpc, m_Camp, 0x00F4);			CHECK_OFF(KNpc, m_Doing, 0x00FC);
CHECK_OFF(KNpc, m_pHeadNode, 0x0110);		CHECK_OFF(KNpc, m_ActiveSkillID, 0x011C);
CHECK_OFF(KNpc, m_Skills, 0x015C);			CHECK_OFF(KNpc, m_CurrentLife, 0x105C);		CHECK_OFF(KNpc, m_CurrentMana, 0x1070);
CHECK_OFF(KNpc, m_CurrentStaminaMax, 0x107C);
CHECK_OFF(KNpc, m_CurrentRunSpeed, 0x1154);	CHECK_OFF(KNpc, m_CurrentAttackRadius, 0x1170);
CHECK_OFF(KNpc, m_MapX, 0x13A0);			CHECK_OFF(KNpc, m_OffY, 0x13B0);
CHECK_OFF(KNpc, Name, 0x1409);				CHECK_OFF(KNpc, m_NpcSettingIdx, 0x1434);
CHECK_OFF(KNpc, m_nPeopleIdx, 0x1498);		CHECK_OFF(KNpc, m_nObjectIdx, 0x14A8);
CHECK_OFF(KNpc, m_SyncSignal, 0x16B8);		CHECK_OFF(KNpc, m_RegX, 0x16C8);
CHECK_OFF(KNpc, m_nPKFlag, 0x16E4);			CHECK_OFF(KNpc, m_FightMode, 0x1920);
CHECK_OFF(KNpc, m_Command, 0x1960);			CHECK_OFF(KNpc, m_bRideHorse, 0x19C0);
CHECK_OFF(KNpc, m_CurrentLifeMax, 0x12B14);	CHECK_OFF(KNpc, m_CurrentManaMax, 0x12B1C);
static_assert(sizeof(KNpc) == NPC_DATA_SIZE, "sizeof(KNpc) sai");

CHECK_OFF(KSkill, m_szSkillName, 0x000C);	CHECK_OFF(KSkill, m_dwSkillID, 0x002C);	CHECK_OFF(KSkill, m_nSkillRange, 0x0050);	CHECK_OFF(KSkill, m_nHorseLimited, 0x00B8);

// KObj 10-10: khong doi (KObjSet::AddData 0x6826F0, ClientAdd 0x683490)
CHECK_OFF(KObj, m_nDataID, 0x0004);			CHECK_OFF(KObj, m_Kind, 0x0008);			CHECK_OFF(KObj, m_RegX, 0x0010);
CHECK_OFF(KObj, m_MapX, 0x001C);			CHECK_OFF(KObj, m_OffX, 0x0024);			CHECK_OFF(KObj, m_szName, 0x0060);
CHECK_OFF(KObj, m_nColorID, 0x0084);		CHECK_OFF(KObj, m_nItemDataID, 0x0088);		CHECK_OFF(KObj, m_nItemWidth, 0x008C);
CHECK_OFF(KObj, m_nMoneyNum, 0x0094);

CHECK_OFF(KItem, m_aryAllAttrib, 0x0228);	CHECK_OFF(KItem, m_MagicLevel, 0x03AC);	CHECK_OFF(KItem, m_dwUniID, 0x04E0);	CHECK_OFF(KItem, m_nCurDur, 0x04E4);	CHECK_OFF(KItem, m_nMaxDur, 0x04E8);
CHECK_OFF(KItem, m_nItemPlus, 0x0514);	CHECK_OFF(KItem, m_nLockCount, 0x0520);

CHECK_OFF(KItemList, m_nForeHand, 0x001C);	CHECK_OFF(KItemList, m_EquipItem, 0x0028);
CHECK_OFF(KItemList, m_Items, 0x006C);		CHECK_OFF(KItemList, m_Room, 0x4C5C);

CHECK_OFF(KPlayer, m_nEntrustMode, 0x0028);	CHECK_OFF(KPlayer, m_nLeftSkillID, 0x002C);
CHECK_OFF(KPlayer, m_RunStatus, 0x1808);	CHECK_OFF(KPlayer, m_nSendMoveFrames, 0x1810);
CHECK_OFF(KPlayer, m_dwHashID, 0x1850);		CHECK_OFF(KPlayer, m_nIndex, 0x1854);
CHECK_OFF(KPlayer, m_ItemList, 0x185C);		CHECK_OFF(KPlayer, m_nCurShop, 0x6794);
CHECK_OFF(KPlayer, m_nCurExp, 0x7220);		CHECK_OFF(KPlayer, m_cTeam, 0x7258);
CHECK_OFF(KPlayer, m_nCurFaction, 0x7800);	CHECK_OFF(KPlayer, m_nPKValue, 0x7AA0);
CHECK_OFF(KPlayer, m_cTong, 0x7CFC);		CHECK_OFF(KPlayer, m_cTask, 0xA1A0);
static_assert(sizeof(KPlayer) == PLAYER_OFFSET, "sizeof(KPlayer) sai");
#undef CHECK_OFF

////////////////////////////////////////////
// Owner-Definition Classes & Structs
////////////////////////////////////////////

#pragma pack(1)
struct GlobalTongInfo
{
	DWORD dwTongID;
	char szTongName[16];
	BOOL bIsRestricted;
	int WorkShopLevel[7];
};

struct TongInfo
{
	short nTongLevel;
	short nDesignLevel;
	char szTongName[10];
};

struct TongList
{
	BYTE ListData[9];
	BYTE nTongCount;
	TongInfo cTongInfo[MAX_TOP_TONG];
};
#pragma pack()

////////////////////////////////////////////
// Global Functions & Variables
////////////////////////////////////////////
extern DWORD			dwObjBaseAdd;
extern DWORD			dwItmBaseAdd;

extern KPlayer*			g_pCurPlayer;
extern KNpc*			g_pNpcArray[MAX_NPC];
extern KObj*			g_pObjArray[MAX_OBJ];
extern KItem*			g_pItemArray[MAX_ITM];

extern KTeam*			g_pTeam;
extern KCoreShell*		g_pCoreShell;
extern KBuySell*		g_pBuySell;
extern GlobalTongInfo*	g_GlobalTongInfo;
extern int*				g_pMapID;
extern char*			g_szMapName;

extern BOOL(*ExecuteScript)(const char*);
extern void(*SendClientCmdSkill)(int, int, int);

unsigned int			HashString2ID(char* szFileName);
void					PostChatMessage(const char *szChatMsg);
int						__distance(int X1, int Y1, int X2, int Y2);
int						__distance(POINT p1, POINT p2);
int						__getheight(POINT p1, POINT p2, POINT p3);
