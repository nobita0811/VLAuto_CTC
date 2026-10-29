#pragma once

#include "S3Client.h"

#define WM_FEEDBACK			WM_USER + 200

//x1 Timers
#define MAX_NMBF_COUNT		30		// ~1.5s
#define MAX_BUFF_COUNT		80		// ~4s

//x2 Timers
#define MAX_LAG_CHECK		1000	// ~10s at 10 speed
#define ATT_LAG_CHECK		60		// ~6s at level 1
#define MAX_BOSS_WAIT		20		// ~2s

#define MAX_LIC_COUNT		200

#define COUNT_OF(x) ((sizeof(x)/sizeof(0[x])) / ((size_t)(!(sizeof(x) % sizeof(0[x])))))

struct GameData
{
	int NpcBaseAdd;
	int PlrBaseAdd;
	int MapAddress;
	int AccountID;
	int KCoreShell;
	int ExeFileSize;
	char RegData[MAX_LIC_COUNT][33];
	int RegCode[MAX_LIC_COUNT];
	int nAddDif;
};

extern GameData theGameData;
extern int g_FreeLICType;

class CVLMain
{
public:
	HWND	m_hWndGame;
	HWND	m_hWndAuto;
	char	m_szCurFolder[MAX_PATH];
	char	m_szIniFile[MAX_PATH];
	BOOL	m_bPause;
	BOOL	m_bSilentMode;
	BOOL	m_bPrintScreen;
	BOOL	m_bAttackBossFirst;
	char	m_szControlPass[40];
	BOOL	m_bQuitingGame;
	int		m_nTongMoney;		// Lenh gom tien len bang dang cho ve thanh (0 = khong co)
	int		m_nTongTimeout;		// g_RunningTimer: qua moc nay chua ve thanh => huy
	int		m_nTongCityTimer;	// g_RunningTimer: da ve thanh, cho toi moc nay moi gop (0 = chua ve thanh)
	int		m_nTongStep;		// TONG_STEP (VLMain.cpp): ve thanh -> rut tien ruong -> gop
	int		m_nTongEquipBase;	// Tien tren nguoi luc gui lenh rut tien ruong

private:
	unsigned m_uHardDiskID;

public:
	CVLMain(void);
	BOOL IsPEPacked();
	BOOL PostFeedBack(WPARAM wParam, LPARAM lParam = -1);
	BOOL StoreTongMoney(int nMoney);
	void TongMoneyBreathe();
	BOOL UseAmuletPaper();
	BOOL HaveTownPortal();
	void SetDllFilePath(char *szDllFile);
	int CheckRegistration();
	void InitParams(HWND hWndGame, HWND hWndServer);
	void InitGameData();
	void LoadOptions();
	void ExitGame(int nLicCheck = 1);
	void QuitGame();
	void RestartHook();
	void SetPause(BOOL bPause = TRUE);
	void GetContainerPos(int nRoom, int *nContainer, int *nPosition);
	BOOL OpenLockRoom(int nRoomPass);
	void CloseLockRoom();
	void GetMapName(int nMapID, char* szMapName);
	int GetSubCity(int nCityID);
	int GetOwnerCity(int nTownID);
	void GetCityName(int nCityID, char* szCityName);
	int GetReturnCity(int nTrainMapID);
	int GetLevelExp(int nLevel);
	int GetExpUpLevel(int nLevel1, int nLevel2);
	int GetConquerVal(int nSeries1, int nSeries2);
	void UpdateTongList();
	void ClearEverything();
	int GetMenuCount();
	BOOL ClickMenuIdx(int nIdx);
	BOOL ClickMenuStr(const char *szLine, BOOL bClick = TRUE);
};

class CVLBasic
{
public:
	UINT	m_cbSize;
	BOOL	m_bExitWhenAM;
	BOOL	m_bAutoStart;	// Chi dung ben VLAuto, giu cho khop PARAM_BASIC
	int		m_nAutoStartTime;
	BOOL	m_bExpLimit;	// Gioi han Exp
	int		m_nExpMax;		// % exp bat dau
	int		m_nExpMin;		// % exp dung
	int		m_nExpLevel;	// cap toi thieu de chay Gioi han Exp
	BOOL	m_bTrimRam;		// Giam RAM dinh ky
	int		m_nTrimRamTime;	// So giay giua 2 lan giam RAM
//private:
	int		m_nDistTimeCount;
	int		m_nIdleTimeCount;
	char	m_szCurName[32];
	int		m_nExpStep;		// EXP_LIMIT_STEP
	struct { int nMapID, x, y; } Last_Save_Repository;	// Ruong luu truoc khi Gioi han Exp
	DWORD	m_dwLastTrimRam;	// GetTickCount lan giam RAM truoc (0 = chua)
public:
	CVLBasic(void);
	void Breathe();
	BOOL IsExpLimitRunning() { return m_nExpStep != 0; }
	BOOL ExpLimitCheck();
	void ExpLimitBreathe();
	void TrimRamBreathe();	// 200ms, ca khi tam dung
	int GetExpPercent();
private:
	void SetExpStep(int nStep);
	void LoadExpState();
};

enum EXP_LIMIT_STEP
{
	exp_none,
	exp_return,		// Dung TDP ve thanh
	exp_saveold,	// Luu ruong thanh hien tai (Last_Save_Repository), rut tien
	exp_goto,		// Xa phu sang Duong Chau
	exp_savenew,	// Luu ruong Duong Chau
	exp_waitdie,	// Dung cho quai danh chet
	exp_restore,	// Luu lai ruong cu
	exp_num,
};

#define MAX_X2_ITEMS 16
class CVLRecover
{
public:
	UINT	m_cbSize;
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
//	Private
	BOOL	m_bDoingBuff;
	BOOL	m_bEmptyMana;
	int		m_nHPBuffCount;		//Life Buff
	int		m_nMPBuffCount;		//Mana Buff
	int		m_nAMBuffCount;		//Magic Paper
	int		m_nNMBuffCount;		//NgaMi Buff
	int		m_nPNBuffCount;		//Poison Buff
	char	m_szX2Items[MAX_X2_ITEMS][80];

public:
	CVLRecover(void);
	void	Breathe();
	void	DoNgaMiBuff();
	void	CheckEquip();
	void	CheckInventory();
	void	CheckPKQuest();
	void	OnUseAM(BOOL bUrgent = 0);
	void	OnUseSDT();
	void	LoadX2List();
	void	OnUseTTL();
	BOOL	IsInX2List(const char *szItemName);
};

class KAliveNpc
{
public:
	DWORD	m_dwID;
	BOOL	m_bIsGold;
	int		m_nGoldType;
	int		m_CurLife;
	int		m_nPosX;
	int		m_nPosY;
	int		m_nRegionIdx;
	int		m_nNpcSIdx;
public:
	static void RefreshNpcs();
};

#define LAG_NPC_TIME		600		// ~60s (giam moi lan IsLagTarget, 100ms)

class KLagNpc
{
public:
	DWORD	m_dwID;
	int		m_CurLife;
	int		m_LifeTime;
public:
	KLagNpc() { m_dwID = 0; m_CurLife = 0; m_LifeTime = LAG_NPC_TIME; }
	KLagNpc(DWORD dwID, int CurLife) { m_dwID = dwID; m_CurLife = CurLife; }
	void SetValue(DWORD dwID, int CurLife) { m_dwID = dwID; m_CurLife = CurLife; m_LifeTime = LAG_NPC_TIME; }
	BOOL IsNull() { return (m_dwID==0); }
	void operator = (DWORD dwID) { m_dwID = dwID; m_CurLife = 0; }
	void operator = (const KLagNpc& _Right)
	{
		m_dwID = _Right.m_dwID;
		m_CurLife = _Right.m_CurLife;
	}
	// Chi so ID: truoc day so ca mau => quai lag hoi mau / server dong bo lai mau la khong con khop,
	// Chien dau nhan lai ngay con lag do (khoa cung muc tieu). Het m_LifeTime moi thu lai.
	BOOL operator == (const KLagNpc& _Right)
	{
		return (m_dwID == _Right.m_dwID);
	}
	BOOL operator != (const KLagNpc& _Right)
	{
		return (m_dwID != _Right.m_dwID);
	}
};

#define MAXSETSKILL 8

class CVLAttack
{
	typedef struct
	{
		int nUseSkill;
		int nAlterValue;
		int	nAlterType;		// Direct, Time, Life, Mana...
	} ALTER_SKILLS;

public:
	UINT	m_cbSize;
	BOOL	m_bAutoHit;
	int		m_nSearchRange;
	int		m_nHorseMode;
	BOOL	m_bHitClose;
	int		m_nSkillRange;
	BOOL	m_bAvoidBoss;
	//Auto Defence
	int		m_nAutoDefence;
	int		m_nDefenceRange;
	BOOL	m_bDefHorseMode;
	BOOL	m_bDefHitClose;
	int		m_nDefSkillRange;
	BOOL	m_bAlwaysF9;
	//Skill Param
	int		m_SettingSkills[MAXSETSKILL];
	ALTER_SKILLS m_AlterSkill[2];
	//Reservations
	int		m_Revserved[1];		// PARAM_ATTACK::InitValue ghi tran m_AlterSkill[2] vao day (2000) - khong dung
	BOOL	m_bOnlyBlueBoss;	// Chi danh boss xanh (ten quai mau xanh)
	int		m_Revserved2;

	//Private
	DWORD	m_LastNpcID;
	int		m_LastNpcLife;
	KLagNpc	m_BadNpc[3];
	KAliveNpc	m_AliveNpc[MAX_NPC];
	BOOL	m_bDoingSkill;
	BOOL	m_bDoingSkillEx;
	int		m_nDoingDirectSkill[3];
	BOOL	m_bDownHorse;
	BOOL	m_bSkillChanged;
	BOOL	m_bBossAttacking;
	int		m_nBossWait;
	int		m_nPeopleIdx;
	int		m_nEnemyIdx;

public:
	CVLAttack(void);
	void	Breathe();
	void	SetPeopleIdx(int nIndex);
	void	SetAutoHit();
	void	SetDefSkills(int nLeftID, int nRightID);
	void	SetActiveSkill(int nSkillId);
	void	HandleDirectSkills();
	void	HandleInDirectSkills();
	void	DoSpecialSkill(int nSkillIdx, BOOL bDirect = TRUE);
	BOOL	IsLagTarget(int nIndex);
	void	AddBadNpc(DWORD dwID, int nLife);	// Ghi quai lag vao m_BadNpc (da co: gia han, day: thay o sap het han)
	int		SearchNearestNpc(int nX, int nY, int nRange);
	void	AutoDefence();
	void	ReleaseEnemy();
	void	ProcessAttack();
	int		GetCurTargetIdx();
	void	SendVLCmdSkill(int nSkillId, int nX, int nY);
};

#define	NUM_BADITEMS	60
class KBadObj
{
public:
	int m_ItemData[NUM_BADITEMS];
	int m_ItemID[NUM_BADITEMS];
public:
	KBadObj(void);
	void Push(int nItemDataID, int nID);
	BOOL IsBadObj(int nItemDataID, int nID);
};

#define MAX_BLACK_ITEMS 64
#define MAX_RED_ITEMS 64
#define	NUM_ATTRIB	40

typedef struct _REDITEM
{
	char szItemName[80];
	int nItemSeries;
} REDITEM;

class CVLCollect
{
	typedef struct
	{
		int nAttribType;
		int nAttribValue;
	} MAGICATTRIB;

public:
	UINT	m_cbSize;
	BOOL	m_bAutoCollect;
	int		m_nCollectRange;
	BOOL	m_bCollectMoney;
	int		m_nCollectEquip;
	MAGICATTRIB arrMagic[NUM_ATTRIB];
	BOOL	m_bPickUpInCity;
	BOOL	m_bSaveJewellery;
	BOOL	m_bAutoArrange;
	BOOL	m_bCollectPrice;
	int		m_nCollectPrice;
	BOOL	m_bCollectLevel;
	int		m_nCollectLevel;
	//Ver 4.5
	int		m_nRingLine;
	int		m_nLaceLine;
	int		m_nVioletLine;
	BOOL	m_bForcePickup;
	BOOL	m_bDecompose;	// 10-10: Ra do bi loc thay vi vut (do khoa khong vut duoc)
	//Private
	BOOL	m_bNeedFilter;
	char	m_szBlackItems[MAX_BLACK_ITEMS][32];
	REDITEM	m_RedItems[MAX_RED_ITEMS];
	int		m_nDropIndex;
	int		m_nArrangeIndex;
	int		m_nDecomposeFail[8];	// item Ra khong thanh cong - bo qua
	int		m_nDecomposeIdx;
	int		m_nDecomposeTry;
	DWORD	m_dwDecomposeFailTime;	// lan cuoi them vao m_nDecomposeFail (xoa sau 30s)
	BOOL	m_bGetCNHK;		// Cam nang HK

public:
	CVLCollect(void);
	void	InjectCode(BOOL bInject = TRUE);
	void	Breathe();
	void	SearchObj(int nRange);
	int		CheckObj(KObj *pObj, int nPlayerX, int nPlayerY);
	void	LoadBlackList();
	BOOL	IsInBlackList(const char *szObjName);
	void	LoadRedList();
	BOOL	IsInRedList(const char *szItemName, int nSeries);
	ITEM_KEEP_CAUSE CheckItemAttrib(KItem *pItem);
	ITEM_KEEP_CAUSE KeepTheItem(int nItemIdx, BOOL bForSelling = 0);
	BOOL	CheckHandItem(int nHandIdx); //return TRUE if DROP a bad Item
	BOOL	FilterItems();	//return TRUE if FOUND a bad Item
	BOOL	ArrangeItems();
	void	SetAutoArrange();
};

enum MOVE_STATUS
{
	status_idle = 0,
	status_movefirst,
	status_move,
	status_stand,
	status_moveattack,
	status_attack,
	status_pickup,
};

#define MAX_NODES 64
#define MAX_POINTS 128
#define MAX_INTNUM 0x7FFFFF
class CVLMove
{
public:
	UINT	m_cbSize;
	BOOL	m_bFollow;
	char	m_szLeader[32];
	int		m_nFollowDistance;
	BOOL	m_bRideHorseFollow;
	BOOL	m_bAroundPoint;
	int		m_nPointX, m_nPointY;
	int		m_nPointRange;
	BOOL	m_bRouteMove;
	POINT	m_Nodes[MAX_NODES];
	BOOL	m_bRideHorseMove;
	BOOL	m_bAttackWhenMove;
	BOOL	m_bNoAttackFollow;
	BOOL	m_bMoveByMap;
	int		m_nNodesMapID;	// Ban do cua m_Nodes (0 = khong kiem tra)
	POINT	m_GoNodes[MAX_NODES];	// Danh sach toa do di den noi luyen cong (GoTrainBreathe)
	int		m_nGoNodesMapID;	// Ban do cua m_GoNodes (0 = khong kiem tra)
	//private
	int		m_nStatus;
	int		m_nPreNode;
	int		m_nCurNode;
	int		m_nLeaderIdx;
	//Ver 2.42
	int		nLastMapID;
	BOOL	m_bFindingWay;
	// Di toi bai train sau khi chon xa phu (Tab Ban do bao): MapToMap -> MapPath toi diem dau tien.
	// Dang bat thi Tab Chien dau chi buff, khong tu ve / tim quai
	BOOL	m_bGoTrain;
	int		m_nGoTrainMap;
	DWORD	m_dwGoTrainTick;
	int		m_nGoNode;		// Diem dang toi trong m_GoNodes (-1 = chua chon diem bat dau)
	void	GoTrainStart(int nMapID);
	void	GoTrainStop();
	BOOL	GoTrainBreathe();
	BOOL	GoNodesBreathe();
	BOOL	HaveGoNodes();
	BOOL	NearTrainArea(int nPlayerX, int nPlayerY);
	int		m_nMapPoints, m_nStart, m_nEnd;
	POINT	MapPoints[MAX_POINTS];
	int		MapSides[MAX_POINTS][MAX_POINTS];
	int		TraceRoute[MAX_POINTS];
	//Ver 4.50
	int		m_nPathPoints;
	typedef struct _PATHPOINT { int x, y, z; } PATHPOINT;
	PATHPOINT PathPoints[MAX_POINTS];
	int		m_nCurPoint;
	//Ver 5.2
	BOOL	m_bMoveByMapYS;
	int		m_nMaxSide;
	int		m_nLastdZ;
	int		m_nLagCount;
	int		m_nTotalLagCount;
	PATHPOINT PCKPoints[MAX_POINTS];
	int		m_nPCKPoints;
	//Ver 6.15
	BOOL	m_bAroundPointPK;
	BOOL	m_bMoveWithPK;

public:
	CVLMove(void);
	void	Breathe();
	void	GetCurNode();
	void	GetNextNode();
	void	SavePoint();
	BOOL	DoFollow();
	int		GetLeaderChar(const char* szLeader);
	BOOL	IsNpcInRange(int nNpcX, int nNpcY);
	BOOL	LoadYeSouMap(int nYSMapID);
	void	MoveOut();
	void	MoveInPCK();
	BOOL	LoadPCKMap();
	BOOL	IsFlagMoving();

private:
	BOOL	LoadingMapPath();
	BOOL	RouteFinding(int nTrainMapID);

	void	GetMapInfo();
	void	LoadMap(const char *szMapFile);
	void	GetPathInfo();
	void	LoadPath(const char *szPathFile);

	BOOL	IsLagMoving(int nPlayerX, int nPlayerY, int dZ);
	BOOL	RunningToDoor();
	BOOL	RunningToPos();
	BOOL	Dijkstra(int nStart, int nEnd);
	// hàm di chuyển đến bất kỳ nơi nào trong thế giơi game (yêu cầu cần làm full map path , có thể không tìm đường được)
	BOOL	GoToWorld(int MapId, int nDestX, int nDestY);
};

#define MAX_MEMBERS 16
#define MAX_NOPT_NAMES 64
class CVLParty
{
public:
	UINT	m_cbSize;
	BOOL	m_bAutoPT;
	char	m_szMemberName[MAX_MEMBERS][32];
	int		m_nPTAll;
	int		m_nAcceptAll;
	BOOL	m_bAutoLeave;
	int		m_nAutoLeaveMemNum;
	int		m_nAutoLeaveTime;
	BOOL	m_bAutoKick;
	int		m_nAutoKickTime;
	BOOL	m_bAlwaysCaptain;	// Luon lam chu PT; khong tick: chi nhan loi moi (che do danh sach)
private:
	BOOL	m_bPTAllYS;
	BOOL	m_bAcceptAllYS;
	struct ABSENT_MEMBER
	{
		int nMemID;
		int nAbsentTime;
	} amMajor [MAX_TEAM_MEMBER];
	char	m_szNoPTNames[MAX_NOPT_NAMES][32];

public:
	CVLParty(void);
	void Breathe();
	BOOL IsInSamePT(DWORD dwPlayerID);
	BOOL IsInAcceptList(char* szPlayerName);
	BOOL AcceptInvite(char *szInviter);
	void AcceptJoinAll();
	void BreatheListMode();
	int  CountListNames();
	void CheckAbsentMem();
	void LoadNoPTList();
	BOOL IsInNoPTList(const char *szNpcName);
};

enum NPC_IN_CITY
{
	npc_Repository,		// Ruong do
	npc_Driver1,		// Xa phu 1
	npc_Driver2,		// Xa phu 2
	npc_Driver3,		// Xa phu 3
	npc_Driver4,		// Xa phu 4
	npc_Groceries,		// Tap hoa
	npc_Pharmacy,		// Hieu thuoc
	npc_Reputation,		// Vo lam truyen nhan
	npc_Blackman,		// Tho ren
	npc_Postman,		// Dich quan
	npc_YeSou,			// Da Tau
	npc_Horseman,		// Ban ngua
	npc_Reseved1,
	npc_Reseved2,
	npc_Reseved3,
	npc_Nums,
};
#define npc_ThienY npc_Postman

#define MAX_SHOP_ITEMS 18
struct ShopItems
{
	int		nShopIdx;
	int		nItemType;
	char	szItemName[80];
};

#define MAX_SELL_ITEMS 64
#define MAX_NOSAVE_ITEMS 64
class CVLMap
{
	typedef struct
	{
		BOOL bBuyCheck;
		int nItemPos;
		int nBuyAmout;
	} TRADEITEMS;

public:
	UINT	m_cbSize;
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
//	Private
	int		m_nTimeWait;
	BOOL	m_bFinished;
	BOOL	m_nDriverStep;
	//Ver 2.42
	int		m_nPoints;
	POINT	MapPoints[MAX_POINTS];
	int		MapSides[MAX_POINTS][MAX_POINTS];
	int		TraceRoute[MAX_POINTS];
	int		m_nDriverIndex;
	BOOL	m_bNeedMoney;
	BOOL	m_bNeedSellItems;
	BOOL	m_bNeedUseLB;
	BOOL	m_bNeedSaveItems;
	BOOL	m_bNeedRepairItems;
	BOOL	m_bNeedGroceries;
	BOOL	m_bNeedPharmacy;
	BOOL	m_bNeedFuYuan;
	BOOL	m_bNeedMingWang;
	BOOL	m_bNeedGiveKCL;
	BOOL	m_bIsReady;
	BOOL	m_bNeedDriver;
	BOOL	m_bIsMoving;
	int		m_nCurPoint;
	//Ver 5.2
	int		m_nLastdZ;
	int		m_nLagCount;
	int		m_nBigLagCount;
	int		m_nLastMapID;
	char	m_szLastMapName[32];
	int		m_nSaveMapID;
	POINT	NpcPoints[npc_Nums];
	BOOL	m_bDoShoping;
	BOOL	m_bFirstSaveRepos;
	BOOL	m_bRoomLocked;
	BOOL	m_bNoDriverRecent;
	int		m_nEntrustStatus;
	BOOL	m_bEnableGiveKCL;
	ShopItems m_ShopItem[MAX_SHOP_ITEMS];
	char	m_szSellItems[MAX_SELL_ITEMS][80];
	char	m_szNoSaveItems[MAX_NOSAVE_ITEMS][32];	// Maps\RuongItem.txt: khong cat vao ruong

public:
	CVLMap(void);
	void Breathe();
	void InitEquipCheck();
	void InitValues();
	void CheckAutoReturn();
	BOOL LoadMap(int nMapID);
	BOOL Dijkstra(int nStart, int nEnd);
	BOOL RunningToPos(POINT ptDest);
	BOOL IsLagMoving(int nPlayerX, int nPlayerY, int dZ);
	void NearestDriver();
	void WithdrawMoney();
	void WithdrawMoneyForXaPhu();
	BOOL ChangeMap(int nNewMapID);
	int OpenStoreBox(); // 0 = Opening, 1 = Opened, -1 = NotFound
	void SaveNewRepository();
	void FirstRepository();
	void GetSaveItems();
	void GetRepairItems();
	void GetGroceries();
	void GetPharmacy();
	BOOL GetReputation(int nRepType = 0); // 0 = FuYuan, 1 = MingWang
	void GetDriver();
	void UseThanHanhPhu();
	BOOL SellItems();
	BOOL MakeShopRoom(int nNeed);	// Shop dang mo: hanh trang thieu cho cho nNeed mon thi ban 1 mon (TRUE = da ban, cho nhip sau)
	BOOL SaveItems();//Put Items into Repository
	BOOL CheckReady();
	void ClickToNpc(int nX, int nY);
	void LoadSellList();
	BOOL IsInSellList(const char *szItemName);
	void LoadNoSaveList();
	BOOL IsInNoSaveList(const char *szItemName);
	void ResetShoping(BOOL bResetAll = TRUE);
};

enum GEN_QUEST
{
	qt_none,
	qt_coll,
	qt_yesou,
	qt_dopk,
	qt_suspending,
	qt_suspended,
};

enum YESOU_TASK_DOING
{
	ys_none,
	ys_doing,
	ys_return,
	ys_cancel,
	ys_suspending,
};

enum BANGHUI_TASK_DOING
{
	bh_none,
	bh_doing,		// Doing current task
	bh_searching,	// Searching for suitable new task
	bh_getting,		// Gettting a specified task
	bh_return,		// Return current task
	bh_suspending,	// Going to stop
};

enum YESOU_REQUIRED_VALUE
{
	ys_rqunknown,
	ys_rqtuzhi,
	ys_rqmizhi,
	ys_rqjingyan,
	ys_rqmingwang,
	ys_rqfuyuan,
	ys_rqpk,
	ys_rqsongjin,
	ys_rqshanhe,
};


const char* QTName_Gen(int nGen);
const char* QTName_YS(int nYS);
const char* QTName_Rq(int nRq);

struct YS_Task_Header
{
	char szVersion[12];
	int nTaskNum[8];
};

struct YS_Task01_Data
{
	int nMajorID;
	int nMinorID;
	int nItemGenre;
	int nDetail;
	int nParticular;
	int nLevel;
	int nSeries;
	int nNpcID;
	int nMapID;
};

struct YS_Task02_Data
{
	int nMajorID;
	int nMinorID;
	int nItemGenre;
	int nDetail;
	int nParticular;
	int nLevel;
	int nSeries;
	int nAttribute;
	int nMinValue;
	int nMaxValue;
};

struct YS_Task03_Data
{
	int nMajorID;
	int nMinorID;
	int nAttribute;
	int nMinValue;
	int nMaxValue;
};

struct YS_Task05_Data
{
	int nMajorID;
	int nMinorID;
	int nRqType;
	int nRqValue;
};

struct BH_Task01_Data
{
	int nTaskID;
	int nTaskIndex;
	int nItemGenre;
	int nDetail;
	int nParticular;
	int nLevel;
	int nSeries;
	int nQuantity;
	int nPricePD;
	int nMapID;
	int nNpcX;
	int nNpcY;
};

struct BH_Task02_Data
{
	int nTaskID;
	int nTaskIndex;
	int nMapID;
	int nNpcSettingIdx;
};

#define MAX_CITY_NUM 7
#define MAX_ITEMPRICE_NUM 10
struct YSBuyItemOpt
{
	int m_VisitCities[MAX_CITY_NUM];
	int m_MaxPrice[MAX_ITEMPRICE_NUM];
	BOOL m_bNoReturnNoTake;
	BOOL m_bDontSearchBuy;
	BOOL m_bNotFoundCancel;
	BOOL m_bGetAllRep;
	BOOL m_bItemFilter;
	int m_Reserved[2];
};

#define MAX_9XMAP_NUM 10
struct YSTuZhiOpt
{
	BOOL m_TuZhiMapPass[MAX_9XMAP_NUM];
	BOOL m_MiZhiMapPass[MAX_9XMAP_NUM];
	BOOL m_bCancelNoMap;
	BOOL m_bCancelTooLong;
	int m_nTuZhiTime;
	int m_Reserved[4];
};

#define MAX_USE_ITEM 8
struct YSAddPointOpt
{
	char m_ExpItemName[MAX_USE_ITEM][40];
	BOOL m_bExpCancel;
	int	m_nExpCancelVal;
	BOOL m_bFYBuyItem;
	int m_nFYItem01;
	int m_nFYItem02;
	int m_nFYItem03;
	BOOL m_bMWCancel;
	BOOL m_bSJCancel;
	int m_nSJCancelVal;
	int m_nMWCancelVal;
	BOOL m_bUseTienThaoLo;
	BOOL m_bPCKOnly;
	BOOL m_bNoClickTS;
};

#define MAX_PRIZE_ITEM 32
struct YSPrizeSelOpt
{
	int m_PrizeVal[MAX_PRIZE_ITEM];
	int m_Reserved[4];
};

#define MAX_COLLQ_NUM 30
#define MAX_KILLQ_NUM 10
struct YSTongQOpt
{
	BOOL m_CollQPass[MAX_COLLQ_NUM];	// Cac nhiem vu Thu thap bo qua
	BOOL m_KillQPass[MAX_KILLQ_NUM];	// Cac nhiem vu Diet quai bo qua
	BOOL m_bGetLBMin;
	int m_nGetLBMin;
	BOOL m_bGetLBMax;
	int m_nGetLBMax;
	BOOL m_bKillDT;
	BOOL m_bBuyTBTN;
	int m_nBuyTBTN;
	int m_Reserved[4];
};

#define MAX_HARD_QUEST 40
#pragma pack(1)
struct TongPKQuest
{
	unsigned short nQuestKind;
	unsigned char nWsLevel;
	unsigned char nLetters;
};
#pragma pack()

struct YSTongPKOpt
{
	// Ban do
	int m_nDriverOpt;
	BOOL m_bMovePt;
	int m_nPointX;
	int m_nPointY;
	// Xu ly nhiem vu
	int m_nChooseLB;
	BOOL m_bAutoChange;
	BOOL m_bUseWithYS;
	BOOL m_bKillOther;
	char m_szVictimName[32];
	BOOL m_bAutoLogin;
	int m_nAutoLoginIdx;
	BOOL m_bExitVictim;
	int m_nMaxQuestCount;
	// Resevation
	int m_Reserved[3];
	// Ver 6.19
	TongPKQuest m_HardQuests[MAX_HARD_QUEST];
};

#define MAX_COLL_QUEST 120
class CVLQuest
{
public:
	UINT	m_cbSize;
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
	YSBuyItemOpt m_BuyItemOpt;
	YSTuZhiOpt m_TuZhiOpt;
	YSAddPointOpt m_AddPointOpt;
	YSPrizeSelOpt m_PrizeSelOpt;
	YSTongQOpt m_TongQOpt;
	YSTongPKOpt m_TongPKOpt;
	// Private use
	char	m_szLogFile[MAX_PATH];
	int		m_nGenDoing;
	int		m_nYSDoing;
	int		m_nBHDoing;
	char	m_szYSQuest[512];
	int		m_nQuestGID;
	int		m_nQuestID;
	int		m_nQuestStatus;
	int		m_nFirstMapID;
	BOOL	m_bUseDoubleExp;
	BOOL	m_bNeedGoTrain;
	BOOL	m_bNeedGoPCK;
	BOOL	m_bGoTrainGetPK;
	BOOL	m_bIsFull100LB;
	int		m_nLastMapID;
	int		m_nYeSouMapID;
	int		m_nSaveMapID;
	int		m_nNpcSIndex;
	int		m_nDoQuestStep;
	int		m_nRemainQuest;
	int		m_nTotalPKQuest;
	BOOL	m_bIsYeSouAndPK;
	BOOL	m_bGetMingWang;
	time_t	m_tmStart;
	// phần này biến để làm lại logic nhiệm vụ dã tẩu mới (bản CTC 15-10_2026 trở đi)
	BOOL	m_bNoVipCard;		// 15-10: server bao can kich hoat the Vip -> ngung Da Tau
	char	m_szQuestYesouInfo[0x2000]; // dòng text báo nhiệm vụ
	int		m_nYesouKind; // loại nhiệm vụ theo enum TASK_CODES_EX
	int		m_nYesouCount;			// ys_attackNpc: so quai can diet / ys_ItemOption: gia tri thuoc tinh (lon hon N diem)
	int		m_nYesouMapID;			// ys_attackNpc: ID map theo Maps\MapNames.ini (0 = khong thay)
	char	m_szYesouTarget[64];	// ys_attackNpc: ten quai / ys_ItemOption: ten thuoc tinh (TCVN3)
	char	m_szYesouMap[64];		// ys_attackNpc: ten map (TCVN3)
	char	m_szYesouCurQuest[1024];	// doan sau "Nhiem vu hien tai:"
public:
	CVLQuest(void);
	void Breath();
	void BreathBH();
	void BreathYS();
	void BreathPK();

	void CheckStatus();
	void SaveYSLog(const char *szLogInfo);
	void PrepareLog();
	BOOL LoadTaskInfo();
	void CheckAutoYeSou();
	void DoCurrentQuest();
	void CancelQuest();
	void ReturnQuest();
	void ChoosePrize();
	BOOL YSPreparing();
	void GetNewQuest();
	void GetRemainQuest(KUiMsgSel *pMsgSel);
	void GetYSQuestText(KUiMsgSel* pMsgSel);
	void GetYSQuestText(KUiMsgSel2 *pMsgSel);
	void DoQuest01();
	void DoQuest0203();
	void DoQuest04();
	void DoQuest05();
	void DoQuest06();
	void DoQuestJY(int nRqValue);
	void DoQuestMW(int nRqValue);
	void DoQuestFY(int nRqValue);
	void DoQuestPK(int nRqValue);
	void DoSuspendOpt();
	void StopNoVip();
	YESOU_TASK_DOING ReportQuest(BOOL bAutoCheck = 0, char *szQuest = 0);
	YESOU_TASK_DOING GetRequiredCondition();
	YESOU_TASK_DOING GetRequiredConditionRaw();

	// Bang Hoi
	BOOL BHPreparing();
	BOOL SellUnusedLB();
	int CheckingBHTask(int nTaskID, BOOL bJustTry = 0);
	int AutoUseLB(int nTrainMapID = 0);
	BOOL GoBuySpItem();
	BOOL MoveSpItemToEquip();
	void ReturnBHTask(int nTaskID);
	int GetRefLetters();
	// dành cho nhiệm vụ Dã tẩu mới (bản CTC 15-10_2026 trở đi)
	int FindMapID(const char* szName, char* szFound, int nFound, BOOL bPrint);
	void GetYesouQuestFromKUiMsgSel2(KUiMsgSel2* pMsgSel2); // trả về loại nhiệm vụ, số lượng, tên vật phẩm, tên NPC, tọa độ, v.v... trong m_szQuestYesouInfo
	void DoQuestAttackNpc();
	void DoQuestItemOption();
private:
	BOOL CheckMyItem(PlayerItem *pPlayerItem, int nPlace = pos_equiproom, BOOL bNoStackItem = 0);
	BOOL GetUseableItem(PlayerItem *pPlayerItem, const char *szItemName, int nPlace = 0);
	BOOL GetTongTaskLB(PlayerItem *pPlayerItem, int nPlace = 0, int nTrainMapID = 0);
	//BOOL GetSpecialItem(int nDetail, int nParticular, int nSeries, int nQuantity);
	BOOL PutItemToRepo(PlayerItem *pRoomItem, PlayerItem *pEquipItem);
	BOOL GetItemFromRepo(PlayerItem *pPlayerItem);
	int GetNextSearchCity(BOOL bBuySuccess = 0);
	int SearchBuyItem(const char *szItemName = NULL, BOOL bIsNearYeSou =TRUE);
	void ArrangeEquipRoom();

private:
	BOOL m_bInfoLoaded;
	YS_Task_Header TaskInfoHeader;
	YS_Task01_Data *pTask01;
	YS_Task02_Data *pTask02;
	YS_Task03_Data *pTask03;
	YS_Task05_Data *pTask05;
	YS_Task05_Data *pTask06;
	BH_Task01_Data *pTask07;
	BH_Task02_Data *pTask08;
	BOOL m_bVsNpc[MAX_NPC];	// Visited Npcs (BUY ITEMS)

	int m_nRqGenre;
	int m_nRqDetail;
	int m_nRqParticular;
	int m_nRqLevel;
	int m_nRqSeries;
	int m_nReqAttrib;
	int m_nMinValue;
	int m_nMaxValue;
	int m_nRqNpcID;
	int m_nRqMapID;
	int m_nRqType;
	int m_nRqValue;
	int m_nNewCollTask;
	int m_nRqQuantity;

	//Return Item
	BOOL m_bReturnItem;
	PlayerItem m_itmRoom;
	PlayerItem m_itmEquip;
	//ArrangeRoom
	int m_nArrangeStep;
	//ArrangeCollQuest
	int m_CollTasksArray[MAX_COLL_QUEST];
};

class CVLUtil
{
public:
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
	//Bang hoi
	int		m_nMaxLBCount;
	//Private Use
	int		m_nEventKind;		//For DoEvent
	int		m_nDoJobStep;
	BOOL	m_bPausingEntrust;
	BOOL	m_bVsNpc[MAX_NPC];

public:
	CVLUtil(void);
	void Breath();
	void AutoClickNpc();
	void ReadPosition();
	void DeleteSysMessages();
	void OpenCloseStoreBox();
	void OpenCloseNpcShop();
	void CheckLifeTime();
	void CheckPMControl();
	void MoveItems();
	int ClickToNpc(const char *szNpcName);
	void StartGiveKCL();
	void GiveKCL();
	void StartOpenGift();
	void OpenGifts();
	void AutoOpenItems();
	BOOL ArrangeStackItems(const char *szItemName);
	BOOL CheckItem(KItem *pItem);
	void SearhBuyItem();
	void StopAllUtils();
	void TestFunction(int nParam);
	// Bang hoi Util
	void StartGetLB(int nMaxLBCount = 60);
	void DoEventGetLB();
};

extern CVLMain theMain;
extern CVLBasic theBasic;
extern CVLRecover theRecover;
extern CVLAttack theAttack;
extern CVLCollect theCollect;
extern CVLMove theMove;
extern CVLParty theParty;
extern CVLMap theMap;
extern CVLQuest theQuest;
extern CVLUtil theUtil;

extern int g_RunningTimer;
extern int g_NetworkOffline;
