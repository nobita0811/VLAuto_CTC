#pragma once

#include "KNode.h"
#include <stddef.h>

////////////////////////////////////////////
// Common Definitions
////////////////////////////////////////////
#define GetPtrAddress(p) (*(PDWORD_PTR)p)
#define P_PLAYER g_pNpcArray[g_pCurPlayer->m_nIndex]
#define PEOPLE_IDX P_PLAYER->m_nPeopleIdx
#define HAVE_HORSE (g_pCurPlayer->m_ItemList.GetEquip(itempart_horse) > 0)
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
	BYTE		SkillData2[0x0060];		// 15-10 (10-10: 0x64)
	int			m_nHorseLimited;		// 0xB4 = cot HorseLimit (15-10, KSkill load 0x5C99F0; 10-10: 0xB8)
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
	BYTE		m_szNameEx[0x0114];	// 15-10: bo dem ten dai hon (ColorID 0x84 -> 0x198)
	DWORD		m_dwScriptID;		// 0x0194
	int			m_nColorID;			// 0x0198 (ClientAdd 0x677F90)
	int			m_nItemDataID;		// 0x019C
	int			m_nItemWidth;		// 0x01A0
	int			m_nItemHeight;		// 0x01A4
	int			m_nMoneyNum;		// 0x01A8
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
	int			m_nUnknown38;			// 15-10: chen them (GetTongNameID 0x6891C0 doc +0x3C)
	DWORD		m_dwTongNameID;			// +0x3C (15-10, 10-10: +0x38)
	char		m_szTongName[32];
	BYTE		m_TongData[0x0068];
	int			m_nUnusedC8;			// 15-10: cho cu cua m_nContributePoint (giu kich thuoc 0xCC)
public:
	int ContributePoint() const { return 0; }	// 15-10: khong con diem cong hien
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
// 15-10: 1 mon hang tren sap = 0x154 byte, lay bang GetGameData(GDI_STALL_ITEMS, buf, n)
// (handler 0x628011 ghi DU so mon, khong xet n; uId da doi sang chi so KItem)
struct KStallObject
{
	unsigned int	uId;						// 0x000 chi so KItem
	BYTE			Data01[0x0134];
	int				DataX;						// 0x138
	int				DataY;						// 0x13C
	int				DataW;						// 0x140
	int				DataH;						// 0x144
	int				nPrice;						// 0x148 (KUiPlayerStall 0x518C51)
	int				Data02[2];
};
static_assert(sizeof(KStallObject) == 0x154, "KStallObject sai kich thuoc");
#define MAX_STALL_OBJ	256

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
// Layout ban client 15_10 (gamethienha_y_15_10.exe)
// Do bang cach doi chieu ma may voi ban 10_10. Xem Offsets_15_10.md.
// Offset da xac minh co static_assert ben duoi; "(?)" = chua xac minh.
// ==========================================================================
class KNpc
{
public:
	DWORD				m_dwID;					// 0x0000 (15-10: bo con tro buffer VN o dau)
	int					m_Index;				// 0x0004
	KIndexNode			m_Node;					// 0x0008 (ctor 0x5FF860: vtable o +8)
	int					m_Level;				// 0x0020
	DWORD				m_Kind;					// 0x0024
	int					m_Series;				// 0x0028
	int					m_Height;				// 0x002C
	BYTE				m_btRankId;				// 0x0030
	int					m_nStature;				// 0x0034
	int					m_nVNFlag;				// 0x0038 (?)
	KNpcGold			m_cGold;				// 0x003C (lea ecx,[npc+0x3C] -> KNpcGold 0x6D4600)
	BYTE				NpcData1[0x0028];
	KState				m_PowerState;			// 0x0074
	KState				m_PoisonState;			// 0x0084
	KState				m_FreezeState;
	KState				m_BurnState;
	KState				m_ConfuseState;
	KState				m_StunState;
	KState				m_DrunkState;
	int					m_Camp;					// 0x00E4
	int					m_CurrentCamp;			// 0x00E8
	NPCCMD				m_Doing;				// 0x00EC (OPR case 6: == 8 do_sit)
	BYTE				NpcData2[0x000C];
	KStateNode*			m_pHeadNode;			// 0x00FC KList{head,tail,count}
	KStateNode*			m_pTailNode;			// 0x0100
	int					m_nStateCount;			// 0x0104
	int					m_ActiveSkillID;		// 0x0108
	BYTE				NpcData3[0x003C];		// 0x010C; KSkillList o 0x0110 (phan tu 0 khong dung)
	NpcSkill			m_Skills[MAX_NPCSKILL];	// 0x0148 = KSkillList.m_Skills[1]
	int					m_CurrentLife;			// 0x1048
	int					m_LifeRep[4];
	int					m_CurrentMana;			// 0x105C
	int					m_ManaRep;
	int					m_CurrentStamina;		// 0x1064
	int					m_CurrentStaminaMax;	// 0x1068
	BYTE				NpcData4[0x00D8];
	int					m_CurrentWalkSpeed;		// 0x1144
	int					m_CurrentRunSpeed;		// 0x1148
	BYTE				NpcData5[0x0018];
	int					m_CurrentAttackRadius;	// 0x1164
	int					m_CurrentActiveRadius;	// 0x1168
	BYTE				NpcData6[0x01D8];
	int					m_Dir;					// 0x1344
	int					m_RedLum;
	int					m_GreenLum;
	int					m_BlueLum;
	int					m_MapX, m_MapY, m_MapZ;	// 0x1354
	int					m_OffX, m_OffY;			// 0x1360
	BYTE				NpcData7[0x0050];
	char				Name0;					// 0x13B8
	char				Name[39];				// 0x13B9
	int					m_nSex;					// 0x13E0
	int					m_NpcSettingIdx;		// 0x13E4
	int					m_CorpseSettingIdx;		// 0x13E8
	BYTE				NpcData8[0x005C];
	int					m_nPeopleIdx;			// 0x1448
	int					m_nLastDam[3];
	int					m_nObjectIdx;			// 0x1458
	BYTE				NpcData9[0x05FC];
	BOOL				m_FightMode;			// 0x1A58 (SetFightMode 0x5FEA90) - 15-10 nam TRUOC SyncSignal
	BYTE				NpcData9b[0x0018];
	int					m_SyncSignal;			// 0x1A74
	int					m_nUnknown1A78;
	int					m_sClientNpcID[2];		// 0x1A7C
	WORD				m_RegX;					// 0x1A84 = LOWORD(m_dwRegionID)
	WORD				m_RegY;					// 0x1A86 = HIWORD(m_dwRegionID)
	int					m_sSyncPos[6];			// 0x1A88
	int					m_nPKFlag;				// 0x1AA0
	BYTE				NpcDataA[0x022C];		// 15-10: khong con m_dwTongNameID
	int					m_nMenuState;			// 0x1CD0 0=thuong, 1=mo ban hang/giao dich (sap), 2=mo doi (sync bit 0x40/0x80 0x624024; click player 0x6DFE60)
	BYTE				NpcDataA2[0x0044];
	NpcCommand			m_Command;				// 0x1D18 (SendCommand 0x5F7490)
	BYTE				NpcDataD[0x0050];
	BOOL				m_bRideHorse;			// 0x1D78
	BYTE				NpcDataE[0xFC84];
	int					m_CurrentLifeMax;		// 0x11A00 game dung max(LifeMax, LifeMaxEx)
	int					m_CurrentLifeMaxEx;		// 0x11A04
	int					m_CurrentManaMax;		// 0x11A08 game dung max(ManaMax, ManaMaxEx)
	int					m_CurrentManaMaxEx;		// 0x11A0C
	BYTE				NpcDataF[0x00A8];

public:
	int GetLifeMax() { return m_CurrentLifeMax > m_CurrentLifeMaxEx ? m_CurrentLifeMax : m_CurrentLifeMaxEx; }
	int GetManaMax() { return m_CurrentManaMax > m_CurrentManaMaxEx ? m_CurrentManaMax : m_CurrentManaMaxEx; }
	BOOL IsExist();
	BOOL IsNotMob();
	void GetMapPos(int *nX, int *nY);
	void GetGamePos(int *nX, int *nY);	// toa do game dung de danh (theo m_RegionIndex), khac GetMapPos => NPC "ma"
	BOOL IsGhost();
	void MoveTo(int nX, int nY, int nAction = 0);
	void SendCommand(NPCCMD cmd, int x = 0, int y = 0, int z = 0);
};

struct KEquipSlot
{
	int			nIdx;						// chi so KItem (0 = trong)
	int			nData;
};

class KItemList
{
public:
	BYTE		m_ItemData0[0x0020];
	int			m_nForeHand;				// 0x0020 m_Hand (vat dang cam tren chuot) (15-10, 10-10: 0x1C)
	int			m_ItemData1;
	KEquipSlot	m_EquipItem[MAX_EQUIP_SLOT];	// 0x0028 (15-10: {nIdx, nData}[16]; 0x60D940 cmp 0x10, [ecx+eax*8+0x28])
	PlayerItem	m_Items[MAX_PLAYER_ITEM];	// 0x00A8 (memset 0x34F8)
	BYTE		m_ItemData2[0x35A4];		// UseIdx 0x6AF0, ListCurIdx 0x6AFC
	KInventory	m_Room[room_num];			// 0x6B44
public:
	int GetEquip(int nPart) const { return (nPart >= 0 && nPart < MAX_EQUIP_SLOT) ? m_EquipItem[nPart].nIdx : 0; }
};

class KPlayerTask
{
public:
	BYTE	m_TaskData[0x0630];				// 15-10 (10-10: 0x6D8)
public:
	int GetTaskVal(int nTaskNum);
};

class KPlayer
{
public:
	LPDWORD			__vfptr;
	BYTE			PlayerData0[0x001C];
	BYTE			m_nEntrustMode;			// 0x0020 game doc nhu BYTE (SendCommand: byte [PLR+0xA898])
	BYTE			m_btEntrustPad[3];
	int				m_nLeftSkillID;			// 0x0024 (SetLeftSkill 0x5E8FD0)
	int				m_nLeftSkillLevel;
	int				m_nRightSkillID;
	int				m_nRightSkillLevel;
	BYTE			PlayerData1[0x00B8];
	int				m_RunStatus;			// 0x00EC (OPR case 6: [PLR+0xA964])
	DWORD			m_dwNextLevelLeadExp;
	int				m_nSendMoveFrames;		// 0x00F4
	BYTE			PlayerData2[0x0040];
	DWORD			m_dwHashID;				// 0x0138 = m_dwID
	int				m_nIndex;				// 0x013C ([PLR+0xA9B4] = Player[1].m_nIndex)
	int				m_nNetConnectIdx;
	KItemList		m_ItemList;				// 0x0144
	BYTE			PlayerData3[0x00F8];
	int				m_nCurShop;				// 0x6F24 = m_BuyInfo.m_nBuyIdx
	BYTE			PlayerData4[0x06C8];
	__int64			m_nCurExp;				// 0x75F0
	__int64			m_nNextExp;				// 0x75F8
	BYTE			PlayerData5[0x0028];
	KPlayerTeam		m_cTeam;				// 0x7628
	BYTE			PlayerData6[0x05A0];
	int				m_nCurFaction;			// 0x7BD0 m_cFaction.m_nCurFaction
	BYTE			PlayerData7[0x029C];
	int				m_nPKValue;				// 0x7E70 = m_cPK(0x7E18) + 0x58
	KPlayerTong		m_cTong;				// 0x7E74 (GetTongNameID 0x6891C0: add ecx,0x126EC)
	BYTE			PlayerData9[0x2308];
	KPlayerTask		m_cTask;				// 0xA248 (this cua GetSaveVal 0x609990)

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
	BOOL DecomposeItem(int nItemIdx);	// Ra trang bi (15-10: game khong con tinh nang nay -> FALSE)
	void SwitchItem(int nContainer, int nX, int nY);
	void DialogNpc(int nIndex);
	BOOL EnterTongMap(int nStep = 0, DWORD dwTongID = 0); // nStep = 0,1,2
	void ApplyAddBaseAttribute(int nAttribute, int nNo);
	
	
};

class KItem
{
public:
	LPDWORD				__vfptr;
	KCommonAttrib		m_CommonAttrib;			// cac truong dung toi (<= szItemName 0x30) khong doi
	BYTE				m_CommonAttribEx[0x108];	// 15-10: KCommonAttrib dai them 0x108
	KMagicAttrib		m_aryAllAttrib[19];		// 0x330 (15-10, 10-10: 0x228); [13] = 0x400
	BYTE				ItemData1[0xD0];
	int					m_MagicLevel[6];		// 0x530 - KItem::GetTotalMagicLevel 0x6004F0
	int					m_nUnUsed[6];
	BYTE				ItemData1b[0x144];
	DWORD				m_dwUniID;				// 0x6A4
	int					m_nCurDur;				// 0x6A8 - KItem::GetRepairPrice 0x6051B0
	int					m_nMaxDur;				// 0x6AC
	BYTE				ItemData2[0x28];
	BYTE				m_nItemPlus;			// 0x6D8 = "+N" cuong hoa; ItemExParam[0]
	BYTE				m_btExParam1;			// 0x6D9 = ItemExParam[1]
	WORD				m_wExParam2;			// 0x6DA
	int					m_nLeftUsageTime;		// 0x6DC
	int					m_nExpiredTime;			// 0x6E0
	int					m_nLockCount;			// 0x6E4 khoa: -1 vinh vien, >0 co thoi han, 0 khong khoa

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

// Kiem tra layout 15_10 luc bien dich
#define CHECK_OFF(c, m, o) static_assert(offsetof(c, m) == (o), #c "::" #m " sai offset")
CHECK_OFF(KNpc, m_dwID, 0x0000);			CHECK_OFF(KNpc, m_Index, 0x0004);
CHECK_OFF(KNpc, m_Level, 0x0020);			CHECK_OFF(KNpc, m_Kind, 0x0024);
CHECK_OFF(KNpc, m_Series, 0x0028);			CHECK_OFF(KNpc, m_btRankId, 0x0030);
CHECK_OFF(KNpc, m_cGold, 0x003C);			CHECK_OFF(KNpc, m_PoisonState, 0x0084);
CHECK_OFF(KNpc, m_Camp, 0x00E4);			CHECK_OFF(KNpc, m_Doing, 0x00EC);
CHECK_OFF(KNpc, m_pHeadNode, 0x00FC);		CHECK_OFF(KNpc, m_ActiveSkillID, 0x0108);
CHECK_OFF(KNpc, m_Skills, 0x0148);			CHECK_OFF(KNpc, m_CurrentLife, 0x1048);		CHECK_OFF(KNpc, m_CurrentMana, 0x105C);
CHECK_OFF(KNpc, m_CurrentStaminaMax, 0x1068);
CHECK_OFF(KNpc, m_CurrentRunSpeed, 0x1148);	CHECK_OFF(KNpc, m_CurrentAttackRadius, 0x1164);
CHECK_OFF(KNpc, m_Dir, 0x1344);				CHECK_OFF(KNpc, m_MapX, 0x1354);			CHECK_OFF(KNpc, m_OffY, 0x1364);
CHECK_OFF(KNpc, Name, 0x13B9);				CHECK_OFF(KNpc, m_NpcSettingIdx, 0x13E4);
CHECK_OFF(KNpc, m_nPeopleIdx, 0x1448);		CHECK_OFF(KNpc, m_nObjectIdx, 0x1458);
CHECK_OFF(KNpc, m_FightMode, 0x1A58);		CHECK_OFF(KNpc, m_SyncSignal, 0x1A74);		CHECK_OFF(KNpc, m_RegX, 0x1A84);
CHECK_OFF(KNpc, m_nPKFlag, 0x1AA0);			CHECK_OFF(KNpc, m_Command, 0x1D18);			CHECK_OFF(KNpc, m_bRideHorse, 0x1D78);
CHECK_OFF(KNpc, m_CurrentLifeMax, 0x11A00);	CHECK_OFF(KNpc, m_CurrentManaMax, 0x11A08);
static_assert(sizeof(KNpc) == NPC_DATA_SIZE, "sizeof(KNpc) sai");

CHECK_OFF(KSkill, m_szSkillName, 0x000C);	CHECK_OFF(KSkill, m_dwSkillID, 0x002C);	CHECK_OFF(KSkill, m_nSkillRange, 0x0050);	CHECK_OFF(KSkill, m_nHorseLimited, 0x00B4);

// KObj 15-10: 0..0x60 khong doi, tu ColorID dich +0x114 (KObjSet::AddData 0x6774E0, ClientAdd 0x677F90)
CHECK_OFF(KObj, m_nDataID, 0x0004);			CHECK_OFF(KObj, m_Kind, 0x0008);			CHECK_OFF(KObj, m_RegX, 0x0010);
CHECK_OFF(KObj, m_MapX, 0x001C);			CHECK_OFF(KObj, m_OffX, 0x0024);			CHECK_OFF(KObj, m_szName, 0x0060);
CHECK_OFF(KObj, m_nColorID, 0x0198);		CHECK_OFF(KObj, m_nItemDataID, 0x019C);		CHECK_OFF(KObj, m_nItemWidth, 0x01A0);
CHECK_OFF(KObj, m_nMoneyNum, 0x01A8);

CHECK_OFF(KItem, m_aryAllAttrib, 0x0330);	CHECK_OFF(KItem, m_MagicLevel, 0x0530);	CHECK_OFF(KItem, m_dwUniID, 0x06A4);	CHECK_OFF(KItem, m_nCurDur, 0x06A8);	CHECK_OFF(KItem, m_nMaxDur, 0x06AC);
CHECK_OFF(KItem, m_nItemPlus, 0x06D8);	CHECK_OFF(KItem, m_nLockCount, 0x06E4);

CHECK_OFF(KItemList, m_nForeHand, 0x0020);	CHECK_OFF(KItemList, m_EquipItem, 0x0028);
CHECK_OFF(KItemList, m_Items, 0x00A8);		CHECK_OFF(KItemList, m_Room, 0x6B44);

CHECK_OFF(KPlayerTong, m_nFlag, 0x002C);	CHECK_OFF(KPlayerTong, m_dwTongNameID, 0x003C);

CHECK_OFF(KPlayer, m_nEntrustMode, 0x0020);	CHECK_OFF(KPlayer, m_nLeftSkillID, 0x0024);
CHECK_OFF(KPlayer, m_RunStatus, 0x00EC);	CHECK_OFF(KPlayer, m_nSendMoveFrames, 0x00F4);
CHECK_OFF(KPlayer, m_dwHashID, 0x0138);		CHECK_OFF(KPlayer, m_nIndex, 0x013C);
CHECK_OFF(KPlayer, m_ItemList, 0x0144);		CHECK_OFF(KPlayer, m_nCurShop, 0x6F24);
CHECK_OFF(KPlayer, m_nCurExp, 0x75F0);		CHECK_OFF(KPlayer, m_cTeam, 0x7628);
CHECK_OFF(KPlayer, m_nCurFaction, 0x7BD0);	CHECK_OFF(KPlayer, m_nPKValue, 0x7E70);
CHECK_OFF(KPlayer, m_cTong, 0x7E74);		CHECK_OFF(KPlayer, m_cTask, 0xA248);
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
void					PostChatMessage2(const char *szChatMsg); // dành cho quang cáo auto kênh chat lân cận
int						__distance(int X1, int Y1, int X2, int Y2);
int						__distance(POINT p1, POINT p2);
int						__getheight(POINT p1, POINT p2, POINT p3);
// So sanh text TCVN3 khong phan biet hoa/thuong (doi ca 2 ve chu hoa roi so sanh)
unsigned char			TcvnUpChar(unsigned char c);
char*					TcvnUpper(char* sz);
int						TcvnStrCmpI(const char* sz1, const char* sz2);
int						TcvnStrNCmpI(const char* sz1, const char* sz2, int n);
const char*				TcvnStrStrI(const char* szText, const char* szFind);
