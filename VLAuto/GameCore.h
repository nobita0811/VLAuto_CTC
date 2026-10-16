#pragma once

#include <stddef.h>

#define VLTK_WINCLASS _T("Sword3 Class")
#define VLTK_ERROR "<GAME "
#define VLTK_UNNAMED "<GAME OFFLINE>"
#define VLTK_WRONGVER "<GAME ERROR>"

#define MAX_NPC				256	// 15-10: new KNpc[0x100] (10-10: 800)
#define MAX_ITEM			1024	// 15-10: new KItem[0x400] (10-10: 512)

//Game Data
#define _AUTOXP
#ifdef _AUTOXP
	#define ACNT_ID_ADD			0x008491E8	// AccountID (15-10): g_LoginLogic(0x848A68)+0x780, dao bit
	#define MAP_ID_ADD			0x0175D6E8	// Game Map ID (15-10)
	#define MAP_NAME_ADD		0x0175D6C8	// Game Map Name (15-10)
	#define CORE_BASE_ADD		0x00848314	// KCoreShell (pointer) (15-10)
	#define NPC_BASE_ADD		0x017220F0	// KNpc (pointer) (15-10)
	#define PLR_BASE_ADD		0x00EE403C	// KPlayer (pointer) (15-10)
#else
	#define ACNT_ID_ADD			theGameData.AccountID
	#define MAP_ID_ADD			theGameData.MapAddress
	#define NPC_BASE_ADD		theGameData.NpcBaseAdd
	#define PLR_BASE_ADD		theGameData.PlrBaseAdd
	#define CORE_BASE_ADD		theGameData.KCoreShell
#endif

#define PLAYER_OFFSET	0xA878		// 15-10 (khong doi)
#define PLR_IDX_OFFSET	0x013C		// 15-10: PLAYER_OFFSET + PLR_IDX_OFFSET = 0xA9B4 (10-10: 0x1854)
#define PLR_EXP_OFFSET	0x75F0		// 15-10 (10-10: 0x7220)
#define PLR_ROOM_OFFSET	0x6C88		// 15-10: KPlayer::m_ItemList(0x144) + KItemList::m_Room(0x6B44), KInventory[room_num] (10-10: 0x64B8)
#define INVENTORY_SIZE	0x1C		// sizeof(KInventory): m_pArray, m_nMoney(+4), m_nWidth...

#define FLAG_X_OFFST	0x007AAA58	// MiniMap X Offset (15-10)
#define FLAG_Y_OFFST	0x007AAA5C	// MiniMap Y Offset (15-10)
#define SERVERIP_ADD	0x07946BB0	// Server IP Address
#define SKL_BASE_ADD	0x0084ACB8	// KSkill (static) = g_SkillManager.m_SkillInfo (15-10)
#define ITM_BASE_ADD	0x01722148	// KItem (pointer) (15-10)

#define NPC_DATA_SIZE	0x00011AB8	// 15-10 (10-10: 0x12BD4)
#define ITM_DATA_SIZE	0x0864	// 15-10 (10-10: 0x74C)
#define SKL_DATA_SIZE	0x04F8	// 15-10 (10-10: 0x4FC)

#define REGION_WIDTH	512
#define REGION_HEIGHT	1024
#define CELL_WIDTH		32
#define CELL_HEIGHT		32

#define MAX_SKILL			3000	// KSkillManager::m_SkillInfo[3000] (15-10: cmp 0xBB7)
#define SKILL_TYPE_PASSIVE	3		// KSkill::m_nSkillType: 0 tan cong, 1 can chien, 2 hao quang/trang thai, 3 bi dong, 4 bay

class KSkill
{
public:
	int m_nIndex;
	int m_nSkillType;
	int m_nMaxLevel;
	char m_szSkillName[32];
	DWORD m_dwSkillID;		// 0x2C
	int SkillData[2];		// 10-10: SkillStyle, CharAnimId (cu [3])
	BOOL m_bIsPhysics;		// 0x38 = cot IsPhysical (15-10 khong doi, KSkill load 0x5C99F0)
};
static_assert(offsetof(KSkill, m_szSkillName) == 0x0C && offsetof(KSkill, m_dwSkillID) == 0x2C
	&& offsetof(KSkill, m_bIsPhysics) == 0x38, "KSkill sai offset");

#define	MAX_NPCSKILL	80
struct NpcSkill
{
	int		SkillId;
	int		SkillLevel;
	int		Reserverds[10];
};

class KItem
{
public:
	LPDWORD	__vfptr;
	int		nItemGenre;
	int		nSpecialType;
	int		nDetailType;
	int		nParticularType;
	int		nObjIdx;
	BOOL	bStack;
	int		nWidth;
	int		nHeight;
	int		nPrice;
	int		nLevel;
	int		nSeries;
	char	szItemName[80];
};

// Layout ban client 15_10 - phai khop voi KNpc trong VLHook/JxCore.h
class KNpc
{
public:
	DWORD			m_dwID;				// 0x0000
	int				m_Index;			// 0x0004
	int				m_Node[6];			// 0x0008
	int				m_Level;			// 0x0020
	DWORD			m_NpcKind;			// 0x0024
	int				m_Series;			// 0x0028
	BYTE			NpcData1[0x00C0];
	int				m_Doing;			// 0x00EC
	BYTE			NpcData3[0x0058];
	NpcSkill		m_Skills[MAX_NPCSKILL];	// 0x0148 = KSkillList(0x110).m_Skills[1]
	int				m_CurLife;			// 0x1048
	int				m_LifeRep[4];
	int				m_CurMana;			// 0x105C
	int				m_ManaRep;
	int				m_CurStamina;		// 0x1064
	int				m_CurStaminaMax;	// 0x1068
	BYTE			NpcData4[0x02E8];
	int				m_MapX, m_MapY;		// 0x1354
	int				m_MapZ;
	int				m_OffX, m_OffY;		// 0x1360
	BYTE			NpcData7[0x0050];
	char			Name0;				// 0x13B8
	char			Name[39];			// 0x13B9
	BYTE			NpcData8[0x0068];
	int				m_nPeopleIdx;		// 0x1448
	int				m_nLastDam[3];
	int				m_nObjectIdx;		// 0x1458
	BYTE			NpcData9[0x05FC];
	BOOL			m_FightMode;		// 0x1A58 (15-10: nam truoc m_RegX)
	BYTE			NpcDataA[0x0028];
	WORD			m_RegX, m_RegY;		// 0x1A84
	BYTE			NpcDataB[0xFF78];
	int				m_CurLifeMaxA;		// 0x11A00 game dung max(A, B)
	int				m_CurLifeMaxB;		// 0x11A04
	int				m_CurManaMaxA;		// 0x11A08 game dung max(A, B)
	int				m_CurManaMaxB;		// 0x11A0C

public:
	int GetLifeMax() { return m_CurLifeMaxA > m_CurLifeMaxB ? m_CurLifeMaxA : m_CurLifeMaxB; }
	int GetManaMax() { return m_CurManaMaxA > m_CurManaMaxB ? m_CurManaMaxA : m_CurManaMaxB; }
	void GetMapPos(int* nX, int* nY)
	{
		*nX = m_RegX*REGION_WIDTH + m_MapX*CELL_WIDTH + (m_OffX >> 10);
		*nY = m_RegY*REGION_HEIGHT + m_MapY*CELL_HEIGHT + (m_OffY >> 10);
	}
};

#define CHECK_NPC(m, o) static_assert(offsetof(KNpc, m) == (o), "KNpc::" #m " sai offset")
CHECK_NPC(m_dwID, 0x0000);		CHECK_NPC(m_Index, 0x0004);		CHECK_NPC(m_Level, 0x0020);
CHECK_NPC(m_NpcKind, 0x0024);	CHECK_NPC(m_Series, 0x0028);	CHECK_NPC(m_Doing, 0x00EC);
CHECK_NPC(m_Skills, 0x0148);	CHECK_NPC(m_CurLife, 0x1048);	CHECK_NPC(m_CurMana, 0x105C);	CHECK_NPC(m_MapX, 0x1354);
CHECK_NPC(m_OffX, 0x1360);		CHECK_NPC(Name, 0x13B9);		CHECK_NPC(m_nPeopleIdx, 0x1448);
CHECK_NPC(m_RegX, 0x1A84);		CHECK_NPC(m_FightMode, 0x1A58);	CHECK_NPC(m_CurLifeMaxA, 0x11A00);
CHECK_NPC(m_CurManaMaxA, 0x11A08);
#undef CHECK_NPC

