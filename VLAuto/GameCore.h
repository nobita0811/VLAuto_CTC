#pragma once

#include <stddef.h>

#define VLTK_WINCLASS _T("Sword3 Class")
#define VLTK_ERROR "<GAME "
#define VLTK_UNNAMED "<GAME OFFLINE>"
#define VLTK_WRONGVER "<GAME ERROR>"

#define MAX_NPC				800	// 10-10: new KNpc[0x320]
#define MAX_ITEM			512

//Game Data
#define _AUTOXP
#ifdef _AUTOXP
	#define ACNT_ID_ADD			0x009BD2D8	// AccountID (10-10): g_LoginLogic(0x9BCBB8).m_Choices.Account, dao bit
	#define MAP_ID_ADD			0x021D6840	// Game Map ID (10-10)
	#define MAP_NAME_ADD		0x021D6820	// Game Map Name (10-10)
	#define CORE_BASE_ADD		0x009BC130	// KCoreShell (pointer) (10-10)
	#define NPC_BASE_ADD		0x01A3B4B4	// KNpc (pointer) (10-10)
	#define PLR_BASE_ADD		0x009C0474	// KPlayer (pointer) (10-10)
#else
	#define ACNT_ID_ADD			theGameData.AccountID
	#define MAP_ID_ADD			theGameData.MapAddress
	#define NPC_BASE_ADD		theGameData.NpcBaseAdd
	#define PLR_BASE_ADD		theGameData.PlrBaseAdd
	#define CORE_BASE_ADD		theGameData.KCoreShell
#endif

#define PLAYER_OFFSET	0xA878		// 10-10
#define PLR_IDX_OFFSET	0x1854		// 10-10: PLAYER_OFFSET + PLR_IDX_OFFSET = 0xC0CC
#define PLR_EXP_OFFSET	0x7220		// 10-10

#define FLAG_X_OFFST	0x0067E608	// MiniMap X Offset
#define FLAG_Y_OFFST	0x0067E60C	// MiniMap Y Offset
#define SERVERIP_ADD	0x07946BB0	// Server IP Address
#define SKL_BASE_ADD	0x01AB4A10	// KSkill (static) = g_SkillManager.m_SkillInfo (10-10)
#define ITM_BASE_ADD	0x01F18E14	// KItem (pointer) (10-10)

#define NPC_DATA_SIZE	0x00012BD4	// 10-10
#define ITM_DATA_SIZE	0x074C	// 10-10
#define SKL_DATA_SIZE	0x04FC	// 10-10

#define REGION_WIDTH	512
#define REGION_HEIGHT	1024
#define CELL_WIDTH		32
#define CELL_HEIGHT		32

#define MAX_SKILL			3000	// KSkillManager::m_SkillInfo[3000] (10-10)
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
	BOOL m_bIsPhysics;		// 0x38 = cot IsPhysical (KSkill load 0x6F7DB0)
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

// Layout ban client 10_10 - phai khop voi KNpc trong VLHook/JxCore.h
class KNpc
{
public:
	BYTE			NpcData0[0x0008];
	DWORD			m_dwID;				// 0x0008
	int				m_Index;			// 0x000C
	int				m_Node[6];			// 0x0010
	int				m_Level;			// 0x0028
	DWORD			m_NpcKind;			// 0x002C
	int				m_Series;			// 0x0030
	BYTE			NpcData1[0x00C8];
	int				m_Doing;			// 0x00FC
	BYTE			NpcData3[0x005C];
	NpcSkill		m_Skills[MAX_NPCSKILL];	// 0x015C = KSkillList(0x124).m_Skills[1]
	int				m_CurLife;			// 0x105C
	int				m_LifeRep[4];
	int				m_CurMana;			// 0x1070
	int				m_ManaRep;
	int				m_CurStamina;		// 0x1078
	int				m_CurStaminaMax;	// 0x107C
	BYTE			NpcData4[0x0320];
	int				m_MapX, m_MapY;		// 0x13A0
	int				m_MapZ;
	int				m_OffX, m_OffY;		// 0x13AC
	BYTE			NpcData7[0x0054];
	char			Name0;				// 0x1408
	char			Name[39];			// 0x1409
	BYTE			NpcData8[0x0068];
	int				m_nPeopleIdx;		// 0x1498
	int				m_nLastDam[3];
	int				m_nObjectIdx;		// 0x14A8
	BYTE			NpcData9[0x021C];
	WORD			m_RegX, m_RegY;		// 0x16C8
	BYTE			NpcDataA[0x0254];
	BOOL			m_FightMode;		// 0x1920
	BYTE			NpcDataB[0x111F0];
	int				m_CurLifeMaxA;		// 0x12B14 game dung max(A, B)
	int				m_CurLifeMaxB;		// 0x12B18
	int				m_CurManaMaxA;		// 0x12B1C game dung max(A, B)
	int				m_CurManaMaxB;		// 0x12B20

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
CHECK_NPC(m_dwID, 0x0008);		CHECK_NPC(m_Index, 0x000C);		CHECK_NPC(m_Level, 0x0028);
CHECK_NPC(m_NpcKind, 0x002C);	CHECK_NPC(m_Series, 0x0030);	CHECK_NPC(m_Doing, 0x00FC);
CHECK_NPC(m_Skills, 0x015C);	CHECK_NPC(m_CurLife, 0x105C);	CHECK_NPC(m_CurMana, 0x1070);	CHECK_NPC(m_MapX, 0x13A0);
CHECK_NPC(m_OffX, 0x13AC);		CHECK_NPC(Name, 0x1409);		CHECK_NPC(m_nPeopleIdx, 0x1498);
CHECK_NPC(m_RegX, 0x16C8);		CHECK_NPC(m_FightMode, 0x1920);	CHECK_NPC(m_CurLifeMaxA, 0x12B14);
CHECK_NPC(m_CurManaMaxA, 0x12B1C);
#undef CHECK_NPC

