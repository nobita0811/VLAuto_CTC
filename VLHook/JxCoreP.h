#ifndef VLTK_FREE_MODE

struct KStallObject
{
	unsigned int	uId;
	int				UnusedData01[14];
	int				DataX;
	int				DataY;
	int				DataW;
	int				DataH;
	int				nPrice;
	int				UnusedData02[2];
};

class KTeam
{
public:
	int		m_nMember[MAX_TEAM_MEMBER]; //MemberID; m_nMember[0] is Captain
	int		m_nMemNum;
	int		m_nMemLevel[MAX_TEAM_MEMBER];
	char	m_szMemName[MAX_TEAM_MEMBER][32];
};

class KNpc
{
public:
	DWORD				m_dwID;
	int					m_Index;
	KIndexNode			m_Node;
	int					m_Level;
	DWORD				m_Kind;
	int					m_Series;
	int					m_Height;
	BYTE				m_btRankId;
	int					m_nStature;
	int					m_nReviveTime;
	KNpcGold			m_cGold;
	//Reserved Data
	BYTE				NpcData1[0x0028];
	KState				m_PowerState;
	KState				m_PoisonState;
	KState				m_FreezeState;
	KState				m_BurnState;
	KState				m_ConfuseState;
	KState				m_StunState;
	KState				m_DrunkState;
	int					m_Camp;
	int					m_CurrentCamp;
	NPCCMD				m_Doing;
	//Reserved Data
	BYTE				NpcData2[0x000C];
	KList				m_StateSkillList;
	int					m_ActiveSkillID;
	//Reserved Data
	BYTE				NpcData3[0x003C];
	NpcSkill			m_Skills[MAX_NPCSKILL];
	//Reserved Data
	BYTE				NpcData3A[0x0010];
	int					m_CurrentLife;
	int					m_LifeRep[3];
	int					m_CurrentMana;
	int					m_ManaRep[3];
	//Reserved Data
	BYTE				NpcData4[0x00D0];
	int					m_CurrentWalkSpeed;		//0x1150
	int					m_CurrentRunSpeed;
	//Reserved Data
	BYTE				NpcData5[0x0018];
	int					m_CurrentAttackRadius;	//0x116C
	int					m_CurrentActiveRadius;
	//Reserved Data
	BYTE				NpcData6[0x0130];
	int					m_Dir;
	int					m_RedLum;
	int					m_GreenLum;
	int					m_BlueLum;
	int					m_MapX, m_MapY, m_MapZ;
	int					m_OffX, m_OffY;
	//Reserved Data
	BYTE				NpcData7[0x0050];
	char				Name0;
	char				Name[39];				//0x131D
	int					m_nSex;
	int					m_NpcSettingIdx;
	int					m_CorpseSettingIdx;
	//Reserved Data
	BYTE				NpcData8[0x005C];
	int					m_nPeopleIdx;
	int					m_nLastDam[3];
	int					m_nObjectIdx;
	//Reserved Data
	BYTE				NpcData9[0x01B4];
	BOOL				m_FightMode;
	int					m_ExchangeServer[3];
	int					m_SyncSignal;
	//Reserved Data
	BYTE				NpcDataA[0x000C];
	WORD				m_RegX;
	WORD				m_RegY;
	//Reserved Data
	BYTE				NpcDataB[0x0018];
	int					m_nPKFlag;
	//Reserved Data
	BYTE				NpcDataC[0x0128];
	DWORD				m_dwTongNameID;
	BOOL				m_bHaveStall;
	//Reserved Data
	BYTE				NpcDataD[0x0044];
	NpcCommand			m_Command;
	//Reserved Data
	BYTE				NpcDataE[0x0050];
	BOOL				m_bRideHorse;			//0x1780
	//Reserved Data
	BYTE				NpcDataF[0xFB84];
	int					m_CurrentLifeMax;		//0x11308
	int					m_CurrentStaminaMax;
	int					m_CurrentManaMax;

public:
	BOOL IsExist();
	void GetMapPos(int *nX, int *nY);
	void MoveTo(int nX, int nY, int nAction = 0);
	void SendCommand(NPCCMD cmd, int x = 0, int y = 0, int z = 0);
};

class KItemList
{
public:
	int			m_PlayerIdx;
	int			m_Hand;
	int			m_nBackHand;
	int			m_EquipItem[itempart_num];
	PlayerItem	m_Items[MAX_PLAYER_ITEM];
	BYTE		ItemListData1[0x2608];
	KInventory	m_Room[room_num];
};

class KPlayer
{
public:
	LPDWORD			__vfptr;
	int				m_nDataID[2];
	int				m_nEntrustMode;
	int				m_nLeftSkillID;
	int				m_nLeftSkillLevel;
	int				m_nRightSkillID;
	int				m_nRightSkillLevel;
	BYTE			PlayerData1[0x008C];
	int				m_RunStatus;
	DWORD			m_dwNextLevelLeadExp;
	int				m_nSendMoveFrames;
	BYTE			PlayerData2[0x003C];
	DWORD			m_dwHashID;
	int				m_nIndex;
	int				m_nNetConnectIdx;
	KItemList		m_ItemList;
	int				m_nTaskItems[20];
	int				m_nCurShop;
	BYTE			PlayerData3[0x0920];
	int				m_nAttributePoint;
	int				m_nSkillPoint;
	int				m_nStrength;
	int				m_nDexterity;
	int				m_nVitality;
	int				m_nEngergy;
	int				m_nLucky;
	int				m_nChatChannel;
	int				m_nCurStrength;
	int				m_nCurDexterity;
	int				m_nCurVitality;
	int				m_nCurEngergy;
	int				m_nCurLucky1;
	int				m_nCurLucky2;
	__int64			m_nExp;
	__int64			m_nNextLevelExp;
	char			m_PlayerName[40];
	KPlayerTeam		m_cTeam;
	BYTE			PlayerData4[0x05A8];
	int				m_nCurFaction;
	BYTE			PlayerData5[0x0350];
	int				m_nPKValue;
	KPlayerTong		m_cTong;
	BYTE			PlayerData6[0x2324];
	KPlayerTask		m_cTask;				//0x849C

public:
	void MoveTo(int nX, int nY);
	void DoSit();
	int GetTaskVal(int nTaskNum);
	void PickUpObj(int nObjIndex);
	int CheckRoom(int nRoom = room_equipment);
	int GetItemsCount(const char *szItemName, int nPlace = pos_equiproom);
	int GetItemsCount(int nGenre, int nDetailType, int nParticular = -1, int nSeries = -1, int nPlace = pos_equiproom);
	BOOL UseItem(const char *szItemName);
	BOOL EatMedicine(int nDetailType);
	void BuyItem(int nItemIdx);
	void SellItem(int nItemIdx);
	void RepairItem(int nItemIdx);
	void SwitchItem(int nContainer, int nX, int nY);
	void DialogNpc(int nIndex);
	BOOL EnterTongMap(int nStep = 0, DWORD dwTongID = 0); // nStep = 0,1,2
	void ApplyAddBaseAttribute(int nAttribute, int nNo);
};

class KItem
{
public:
	LPDWORD				__vfptr;
	KItemCommonAttrib	m_CommonAttrib;
	KMagicAttrib		m_aryAllAttrib[19];
	BYTE				ItemData1[0x30];
	int					m_MagicLevel[6];
	int					m_nUnUsed[6];
	DWORD				m_dwUniID;
	int					m_nCurDur;
	int					m_nMaxDur;
	BYTE				ItemData2[0x28];
	int					m_nItemPlus;
	BYTE				ItemData3[0x08];
	BOOL				m_nLockCount;

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

#endif
