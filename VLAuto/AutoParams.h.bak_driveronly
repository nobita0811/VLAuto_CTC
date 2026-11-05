#pragma once

#define	NUM_ATTRIB			40
#define	ALL_ATTRIB			79	// 15-10: thuoc tinh co trong magicdesc.ini 15_10
#define MAXSETSKILL			8
#define MAX_NODES			64
#define MAX_MEMBERS			16
#define MAX_CITY_NUM		7
#define MAX_ITEMPRICE_NUM	10
#define MAX_9XMAP_NUM		10
#define MAX_USE_ITEM		8
#define MAX_PRIZE_ITEM		32
#define MAX_COLLQ_NUM		30
#define MAX_KILLQ_NUM		10
#define MAX_HARD_QUEST		40

extern const int CitiesID[MAX_CITY_NUM];

struct ITEM_ATTRIB
{
	int nAttVal;
	TCHAR* szAttDesc;
};

extern const ITEM_ATTRIB AllAttribs[ALL_ATTRIB];

struct AutoParams
{
	UINT	m_cbSize;

public:
	BOOL	ReadFromFile(CFile& cfgFile);
	BOOL	WriteToFile(CFile& cfgFile);
};

// Basic Parameters
struct PARAM_BASIC : public AutoParams
{
	BOOL	m_bExitWhenAM;
	BOOL	m_bAutoStart;	// Tu check Auto khi nhan vat vao game
	int		m_nAutoStartTime;	// So mili giay cho truoc khi tu check Auto
	BOOL	m_bExpLimit;	// Gioi han Exp
	int		m_nExpMax;		// % exp bat dau
	int		m_nExpMin;		// % exp dung
	int		m_nExpLevel;	// cap toi thieu de chay Gioi han Exp
	BOOL	m_bTrimRam;		// Giam RAM dinh ky (SetProcessWorkingSetSize trong game)
	int		m_nTrimRamTime;	// So giay giua 2 lan giam RAM

public:
	void	InitValue();
};

// Recover Parameters
struct PARAM_RECOVER : public AutoParams
{
	BOOL	m_bAutoHP;
	int		m_nAutoHPAmount;
	int		m_nAutoHPTimer;
	BOOL	m_bAutoMP;
	int		m_nAutoMPAmount;
	int		m_nAutoMPTimer;
	BOOL	m_bAMByHP;
	int		m_nAMByHPAmount;
	BOOL	m_bAMByMP;
	int		m_nAMByMPAmount;
	BOOL	m_bAMByBlood;
	BOOL	m_bAMByMana;
	BOOL	m_bAMByFullRoom;
	int		m_nAMByFullRoomVal;
	BOOL	m_bAMByMoney;
	int		m_nAMByMoneyAmount;
	BOOL	m_bAMByDur;
	int		m_nAMByDurAmount;
	BOOL	m_bUsePoison;
	BOOL	m_bUseDoubleExp;
	BOOL	m_bUseMaster;
	BOOL	m_bIsNgaMiBuff;
	int		m_nNgaMiBuffPercent;
	BOOL	m_bPartyBuff;
	BOOL	m_bAutoRepair;

public:
	void	InitValue();
};

// Attack Parameters
struct ALTER_SKILLS
{
	int nUseSkill;
	int nAlterValue;
	int	nAlterType;
};

struct PARAM_ATTACK : public AutoParams
{
	BOOL	m_bAutoHit;
	int		m_nSearchRange;
	int		m_nHorseMode;
	BOOL	m_bHitClose;
	int		m_nSkillRange;
	BOOL	m_bAvoidBoss;
	int		m_nAutoDefence;
	int		m_nDefenceRange;
	BOOL	m_bDefHorseMode;
	BOOL	m_bDefHitClose;
	int		m_nDefSkillRange;
	BOOL	m_bAlwaysF9;
	int		m_SettingSkills[MAXSETSKILL];
	ALTER_SKILLS m_AlterSkill[2];
	int		m_Revserved[1];		// PARAM_ATTACK::InitValue ghi tran m_AlterSkill[2] vao day (2000) - khong dung
	BOOL	m_bOnlyBlueBoss;	// Chi danh boss xanh (ten quai mau xanh)
	int		m_Revserved2;

public:
	void	InitValue();
};

// Collect Parameters
struct MAGICATTRIB
{
	int nAttribType;
	int nAttribValue;
};

struct PARAM_COLLECT : public AutoParams
{
	BOOL	m_bAutoCollect;
	int		m_nCollectRange;
	BOOL	m_bCollectMoney;
	int		m_nCollectEquip;
	MAGICATTRIB m_arrMagic[NUM_ATTRIB];
	BOOL	m_bPickUpInCity;
	BOOL	m_bSaveJewellery;
	BOOL	m_bAutoArrange;
	BOOL	m_bCollectPrice;
	int		m_nCollectPrice;
	BOOL	m_bCollectLevel;
	int		m_nCollectLevel;
	int		m_nRingLine;
	int		m_nLaceLine;
	int		m_nVioletLine;
	BOOL	m_bForcePickup;
	BOOL	m_bDecompose;	// 10-10: Ra do bi loc thay vi vut

public:
	void	InitValue();
} ;

// Move Parameters
struct PARAM_MOVE : public AutoParams
{
	BOOL	m_bFollow;
	char	m_szLeader[32];
	int		m_nFollowDistance;
	BOOL	m_bRideHorseFollow;
	BOOL	m_bAroundPoint;
	int		m_nPointX;
	int		m_nPointY;
	int		m_nPointRange;
	BOOL	m_bRouteMove;
	POINT	m_Nodes[MAX_NODES];
	BOOL	m_bRideHorseMove;
	BOOL	m_bAttackWhenMove;
	BOOL	m_bNoAttackFollow;
	BOOL	m_bMoveByMap;
	int		m_nNodesMapID;	// Ban do cua danh sach toa do (0 = chua xac dinh)
	POINT	m_GoNodes[MAX_NODES];	// Danh sach toa do di den noi luyen cong (sau khi vao cong map train)
	int		m_nGoNodesMapID;	// Ban do cua m_GoNodes (0 = chua xac dinh)

public:
	void	InitValue();
};

// Party Parameters
struct PARAM_PARTY : public AutoParams
{
	BOOL	m_bAutoPT;
	char	m_szMemberName[MAX_MEMBERS][32];
	int		m_nPTAll;
	int		m_nAcceptAll;
	BOOL	m_bAutoLeave;
	int		m_nAutoLeaveMemNum;
	int		m_nAutoLeaveTime;
	BOOL	m_bAutoKick;
	int		m_nAutoKickTime;
	BOOL	m_bAlwaysCaptain;	// Luon lam chu PT; khong tick: chi nhan loi moi

public:
	void	InitValue();
};

// Map Parameters
struct TRADEITEMS
{
	BOOL bBuyCheck;
	int nItemPos;
	int nBuyAmout;
};

struct PARAM_MAP : public AutoParams
{
	BOOL	m_bAutoReturn;
	BOOL	m_bWithDraw;
	int		m_nRepositoryPass;
	TRADEITEMS m_TradeItem[3];
	BOOL	m_bBuyTDP;
	BOOL	m_bRepair;
	BOOL	m_bSellItems;
	BOOL	m_bGetReputation;
	BOOL	m_bPutMoney;
	int		m_nMoneyAmount;
	BOOL	m_bGetDriver;
	int		m_nDriverOption;
	int		m_nGetMoneyAmount;
	int		m_nAMNumbers;
	int		m_nSaveItemMode;
	int		m_nTrainMapID;
	BOOL	m_bGiveKCL;
	int		m_nMinKCL;

public:
	void	InitValue();
};

struct YSBuyItemOpt
{
	int		m_VisitCities[MAX_CITY_NUM];
	int		m_MaxPrice[MAX_ITEMPRICE_NUM];
	BOOL	m_bNoReturnNoTake;
	BOOL	m_bDontSearchBuy;
	BOOL	m_bNotFoundCancel;
	BOOL	m_bGetAllRep;
	BOOL	m_bItemFilter;
	int		m_Reserved[2];

public:
	void InitValue();
};

struct YSTuZhiOpt
{
	BOOL	m_TuZhiMapPass[MAX_9XMAP_NUM];
	BOOL	m_MiZhiMapPass[MAX_9XMAP_NUM];
	BOOL	m_bCancelNoMap;
	BOOL	m_bCancelTooLong;
	int		m_nTuZhiTime;
	int		m_Reserved[4];

public:
	void	InitValue();
};

struct YSAddPointOpt
{
	char	m_ExpItemName[MAX_USE_ITEM][40];
	BOOL	m_bExpCancel;
	int		m_nExpCancelVal;
	BOOL	m_bFYBuyItem;
	int		m_nFYItem01;
	int		m_nFYItem02;
	int		m_nFYItem03;
	BOOL	m_bMWCancel;
	BOOL	m_bSJCancel;
	int		m_nSJCancelVal;
	int		m_nMWCancelVal;
	BOOL	m_bUseTienThaoLo;
	BOOL	m_bPCKOnly;
	BOOL	m_bNoClickTS;

public:
	void InitValue();
};

struct YSPrizeSelOpt
{
	int		m_PrizeVal[MAX_PRIZE_ITEM];
	int		m_Reserved[4];

public:
	void	InitValue();
};

struct YSTongQOpt
{
	BOOL	m_CollQPass[MAX_COLLQ_NUM];	// Cac nhiem vu Thu thap bo qua
	BOOL	m_KillQPass[MAX_KILLQ_NUM];	// Cac nhiem vu Diet quai bo qua
	BOOL	m_bGetLBMin;
	int		m_nGetLBMin;
	BOOL	m_bGetLBMax;
	int		m_nGetLBMax;
	BOOL	m_bKillDT;
	BOOL	m_bBuyTBTN;
	int		m_nBuyTBTN;
	int		m_Reserved[4];

public:
	void InitValue();
};

struct TongPKQuest
{
	short	nQuestKind;
	char	nWsLevel;
	char	nLetters;
};

struct YSTongPKOpt
{
	// Ban do
	int		m_nDriverOpt;
	BOOL	m_bMovePt;
	int		m_nPointX;
	int		m_nPointY;
	// Xu ly nhiem vu
	int		m_nChooseLB;
	BOOL	m_bAutoChange;
	BOOL	m_bUseWithYS;
	BOOL	m_bKillOther;
	char	m_szVictimName[32];
	BOOL	m_bAutoLogin;
	int		m_nAutoLoginIdx;
	BOOL	m_bExitVictim;
	int		m_nMaxQuestCount;
	// Resevation
	int		m_Reserved[3];
	// Ver 6.19
	TongPKQuest m_HardQuests[MAX_HARD_QUEST];

public:
	void InitValue();
};

struct PARAM_YESOU : public AutoParams
{
	BOOL	m_bAutoDoQuest;
	int		m_nFindItem;
	int		m_nTuZhi;
	int		m_nJingYan;
	int		m_nMingWang;
	int		m_nFuYuan;
	int		m_nYesouPK;
	int		m_nSongJin;
	BOOL	m_bUseR1;
	BOOL	m_bUseR2;
	BOOL	m_bUseR3;
	int		m_nPostAction;
	BOOL	m_bInTownOnly;
	//Version 6.1
	int		m_nTongPK;
	int		m_nTongItem;
	int		m_nTongKill;
	// Additional Options
	YSBuyItemOpt	m_BuyItemOpt;
	YSTuZhiOpt		m_TuZhiOpt;
	YSAddPointOpt	m_AddPointOpt;
	YSPrizeSelOpt	m_PrizeSelOpt;
	YSTongQOpt		m_TongQOpt;
	YSTongPKOpt		m_TongPKOpt;

public:
	void	InitValue();
};

struct PARAM_UTIL
{
	BOOL	m_bDoingUtil;
	int		m_nUtilTask;
	int		m_nItemObjIdx;
	int		m_nFromRoom;
	int		m_nToRoom;
	int		m_nHTLevel;
	//Buy Item Params
	char	m_szItemName[80];
	int		m_nEquipDetail;
	int		m_nEquipSex;
	int		m_nItemSeries;
	int		m_nItemLevel;
	int		m_nItemSpType;
	int		m_nItemAttrib;
	int		m_nAttLine;
	int		m_nAttMin;
	int		m_nAttMax;
	BOOL	m_bFromR0;
	BOOL	m_bFromR1;
	BOOL	m_bFromR2;
	BOOL	m_bFromR3;
	BOOL	m_bFromR4;
	BOOL	m_bFromPlrShop;
	int		m_nItemPrice;
	BOOL	m_bBuyIt;
};
