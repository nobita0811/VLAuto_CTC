#include "stdafx.h"
#include "../GameDef.h"
#include "GameBase.h"
#include "JxCore.h"
#include "S3Client.h"
#include "VLClasses.h"
#include "VLDebug.h"

// Tab Chien dau cua cua so DEBUG: cac ham game dung trong VLAttack.cpp (danh quai, ky nang, tu ve)

extern BOOL g_SysPause;

#define DBG_MAX_SKILL	3000	// KSkillManager::m_SkillInfo[3000]

static const char* s_szSkillSlot[skill_total] = { "supp01", "supp02", "supp03", "left", "right", "boss", "defence" };
static const char* s_szAlterType[] = { "DirectShortcutSkill (moi N ms)", "Theo thoi gian", "Theo mana", "Theo mau" };
static const char* s_szDefence[] = { "Tat", "1", "Dung AM (2)", "3", "4", "Giet PK nhiem vu (5)" };

static KSkill* GetSkill(int nSkillId)
{
	return (nSkillId > 0 && nSkillId < DBG_MAX_SKILL) ? (KSkill*)(SKL_BASE_ADD + (nSkillId - 1) * SKL_DATA_SIZE) : NULL;
}

static const char* SkillName(int nSkillId)
{
	KSkill *pSkill = GetSkill(nSkillId);
	return pSkill ? pSkill->m_szSkillName : "-";
}

static const char* RelationName(int nRelation)
{
	switch (nRelation)
	{
	case relation_none: return "none";
	case relation_self: return "self";
	case relation_ally: return "ally";
	case relation_enemy: return "enemy";
	case relation_dialog: return "dialog";
	}
	return "?";
}

static BOOL CheckNpcIdx(int nIdx)
{
	if (nIdx < 1 || nIdx >= MAX_NPC)
	{
		DbgOut("LOI: Idx = %d ngoai 1..%d (lay o nut 'Danh sach quai')\n", nIdx, MAX_NPC - 1);
		return FALSE;
	}
	if (!g_pNpcArray[nIdx]->IsExist())
	{
		DbgOut("LOI: Npc[%d] khong ton tai\n", nIdx);
		return FALSE;
	}
	return TRUE;
}

static void OutSkillLine(const char *szLabel, int nSkillId)
{
	KSkill *pSkill = GetSkill(nSkillId);
	if (!pSkill)
	{
		DbgOut("%-8s %4d  -\n", szLabel, nSkillId);
		return;
	}
	DbgOut("%-8s %4d  %-24s Range %-4d HorseLimit %d%s\n", szLabel, nSkillId, pSkill->m_szSkillName,
		pSkill->m_nSkillRange, pSkill->m_nHorseLimited,
		(pSkill->m_dwSkillID == (DWORD)nSkillId) ? "" : "  [m_dwSkillID LECH - sai SKL_BASE_ADD/SKL_DATA_SIZE?]");
}

// Giong dieu kien loc trong KAliveNpc::RefreshNpcs (NULL = duoc dua vao m_AliveNpc)
static const char* AliveNpcReason(int i)
{
	KNpc *pNpc = g_pNpcArray[i];
	int nX, nY;
	if (!pNpc->IsExist()) return "khong ton tai";
	if (pNpc->m_CurrentLife <= 0) return "Life <= 0";
	if (pNpc->m_Kind != kind_normal) return "khong phai quai (m_Kind)";
	KLagNpc lagNpc(pNpc->m_dwID, pNpc->m_CurrentLife);
	if (lagNpc == theAttack.m_BadNpc[0] || lagNpc == theAttack.m_BadNpc[1] || lagNpc == theAttack.m_BadNpc[2]) return "m_BadNpc (quai lag)";
	pNpc->GetMapPos(&nX, &nY);
	if ((!theMove.m_bFollow || !theMove.m_nLeaderIdx) && !theMove.IsNpcInRange(nX, nY)) return "ngoai vung (theMove.IsNpcInRange)";
	if (*g_pMapID == 124 && __distance(nX, nY, 51808, 104064) < 64) return "MapLagNpc (map 124)";
	return NULL;
}

static void AttackStatus()
{
	if (!DbgCheckPlayer()) return;
	CVLAttack &a = theAttack;
	KNpc *pPlayer = P_PLAYER;

	DbgOut("m_cbSize = %d (VLAuto gui) - phan public = %d\n", a.m_cbSize, (int)offsetof(CVLAttack, m_LastNpcID));
	DbgOut("m_bAutoHit %d  m_nSearchRange %d  m_nHorseMode %d  m_bHitClose %d (%d)  m_bAvoidBoss %d\n",
		a.m_bAutoHit, a.m_nSearchRange, a.m_nHorseMode, a.m_bHitClose, a.m_nSkillRange, a.m_bAvoidBoss);
	DbgOut("Tu ve: m_nAutoDefence %d (%s)  m_nDefenceRange %d  m_bDefHorseMode %d  m_bDefHitClose %d (%d)  m_bAlwaysF9 %d\n",
		a.m_nAutoDefence, (a.m_nAutoDefence >= 0 && a.m_nAutoDefence < 6) ? s_szDefence[a.m_nAutoDefence] : "?",
		a.m_nDefenceRange, a.m_bDefHorseMode, a.m_bDefHitClose, a.m_nDefSkillRange, a.m_bAlwaysF9);
	for (int i = 0; i < skill_total; i++)
		OutSkillLine(s_szSkillSlot[i], a.m_SettingSkills[i]);
	for (int i = 0; i < 2; i++)
	{
		int nUse = a.m_AlterSkill[i].nUseSkill, nType = a.m_AlterSkill[i].nAlterType;
		DbgOut("AlterSkill[%d]: nUseSkill %d (o phim tat %d)  nAlterType %d (%s)  nAlterValue %d\n", i,
			nUse, nUse - 1, nType, (nType >= 0 && nType < 4) ? s_szAlterType[nType] : "?", a.m_AlterSkill[i].nAlterValue);
	}
	if (a.m_AlterSkill[0].nAlterType == 0)
	{
		for (int i = 0; i < 2; i++)
			if (a.m_AlterSkill[i].nUseSkill && a.m_AlterSkill[i].nAlterValue / 100 == 0)
				DbgOut("  CANH BAO: AlterSkill[%d].nAlterValue = %d < 100 => HandleInDirectSkills chia cho 0 (crash game)!\n", i, a.m_AlterSkill[i].nAlterValue);
	}
	else
		DbgOut("  (kieu cua AlterSkill[0] ap dung cho ca 2; can ca 2 nUseSkill moi chay)\n");

	DbgOut("\nm_LastNpcID %u  m_LastNpcLife %d  m_nPeopleIdx %d  PEOPLE_IDX %d  m_nEnemyIdx %d\n",
		a.m_LastNpcID, a.m_LastNpcLife, a.m_nPeopleIdx, PEOPLE_IDX, a.m_nEnemyIdx);
	DbgOut("m_bDoingSkill %d  m_bDoingSkillEx %d  m_nDoingDirectSkill %d/%d/%d  m_bDownHorse %d  m_bSkillChanged %d\n",
		a.m_bDoingSkill, a.m_bDoingSkillEx, a.m_nDoingDirectSkill[0], a.m_nDoingDirectSkill[1], a.m_nDoingDirectSkill[2],
		a.m_bDownHorse, a.m_bSkillChanged);
	DbgOut("m_bBossAttacking %d  m_nBossWait %d  m_BadNpc: %u/%u/%u\n", a.m_bBossAttacking, a.m_nBossWait,
		a.m_BadNpc[0].m_dwID, a.m_BadNpc[1].m_dwID, a.m_BadNpc[2].m_dwID);
	DbgOut("Player: FightMode %d  Ngua %d (co ngua %d)  Camp %d  PKFlag %d  Series %d  RunSpeed %d  Level %d\n",
		pPlayer->m_FightMode, pPlayer->m_bRideHorse, HAVE_HORSE ? 1 : 0, pPlayer->m_CurrentCamp, pPlayer->m_nPKFlag,
		pPlayer->m_Series, pPlayer->m_CurrentRunSpeed, pPlayer->m_Level);
	DbgOut("        ActiveSkillID %d (%s)  AttackRadius %d  Left %d (%s)  Right %d (%s)\n",
		pPlayer->m_ActiveSkillID, SkillName(pPlayer->m_ActiveSkillID), pPlayer->m_CurrentAttackRadius,
		g_pCurPlayer->m_nLeftSkillID, SkillName(g_pCurPlayer->m_nLeftSkillID), g_pCurPlayer->m_nRightSkillID, SkillName(g_pCurPlayer->m_nRightSkillID));
	DbgOut("VLTK_FREE_MODE: ProcessAttack() rong - danh bang PEOPLE_IDX (game tu danh), GetCurTargetIdx = PEOPLE_IDX\n");

	// Giong thu tu CVLAttack::Breathe
	DbgOut("\nBreathe() (100ms): ");
	if (g_SysPause) { DbgOut("DUNG - g_SysPause (TimerProc khong goi)\n"); return; }
	if (g_NetworkOffline) { DbgOut("DUNG - g_NetworkOffline = %d\n", g_NetworkOffline); return; }
	DbgOut("HandleDirect/InDirectSkills -> ");
	if (a.m_bDoingSkill || a.m_bDoingSkillEx || theRecover.m_bDoingBuff || theMain.m_bPause)
	{
		DbgOut("DUNG - m_bDoingSkill %d / m_bDoingSkillEx %d / theRecover.m_bDoingBuff %d / theMain.m_bPause %d\n",
			a.m_bDoingSkill, a.m_bDoingSkillEx, theRecover.m_bDoingBuff, theMain.m_bPause);
		return;
	}
	DbgOut("AutoDefence -> ");
	if (a.m_nEnemyIdx || theQuest.m_bNeedGoPCK) { DbgOut("DUNG - dang danh nguoi (m_nEnemyIdx %d) / theQuest.m_bNeedGoPCK %d\n", a.m_nEnemyIdx, theQuest.m_bNeedGoPCK); return; }
	if (!pPlayer->m_FightMode) { DbgOut("DUNG - trong thanh (m_FightMode 0)\n"); return; }
	if (!a.m_bAutoHit) { DbgOut("DUNG - chua bat Tu danh quai (m_bAutoHit)\n"); return; }
	if (theMove.m_bFindingWay) { DbgOut("DUNG - theMove.m_bFindingWay\n"); return; }
	if (theMove.m_nStatus == status_movefirst) { DbgOut("DUNG - theMove.m_nStatus = status_movefirst\n"); return; }
	if (theMove.m_nStatus == status_move && !theMove.m_bAttackWhenMove) { DbgOut("DUNG - dang di chuyen va khong bat danh khi di\n"); return; }
	int nTarget = a.GetCurTargetIdx();
	if (nTarget) { DbgOut("dang danh muc tieu %d (IsLagTarget kiem tra lag)\n", nTarget); return; }

	int nRange = a.m_nSearchRange;
	const char *szWhy = "m_nSearchRange";
	if (theMove.m_bFollow)
	{
		if (!theMove.m_nLeaderIdx && theMove.m_bNoAttackFollow) { DbgOut("DUNG - theo sau nhung chua thay leader va bat khong danh khi theo\n"); return; }
		if (theMove.m_nLeaderIdx && g_pNpcArray[theMove.m_nLeaderIdx]->IsExist())
		{
			nRange = theMove.m_nFollowDistance + 75;
			szWhy = "quanh leader (m_nFollowDistance + 75)";
		}
	}
	if (theMove.m_nStatus == status_pickup && nRange > PLAYER_PICKUP_DISTANCE) { nRange = PLAYER_PICKUP_DISTANCE; szWhy = "dang nhat do"; }
	if (a.m_nBossWait) szWhy = "dang cho BOSS (m_nBossWait) - co the bi gioi han";
	if (theRecover.m_bEmptyMana) { nRange = 0; szWhy = "theRecover.m_bEmptyMana"; }
	if (theQuest.m_bAutoDoQuest && theQuest.m_bGoTrainGetPK) { nRange = PLAYER_PICKUP_DISTANCE; szWhy = "nhiem vu GoTrainGetPK"; }
	DbgOut("RefreshNpcs + SearchNearestNpc(tam %d - %s)\n", nRange, szWhy);
	g_DbgReply.nResult[0] = nRange;
	g_DbgReply.bOK = TRUE;
}

static void SkillList()
{
	if (!DbgCheckPlayer()) return;
	KNpc *pPlayer = P_PLAYER;
	DbgOut("P_PLAYER->m_Skills (MAX_NPCSKILL %d):\n", MAX_NPCSKILL);
	int nCount = 0;
	for (int i = 0; i < MAX_NPCSKILL; i++)
	{
		int nId = pPlayer->m_Skills[i].SkillId;
		if (nId <= 0) continue;
		KSkill *pSkill = GetSkill(nId);
		DbgOut("[%2d] %4d Lv %-2d %-24s Range %-4d Ngua %d", i, nId, pPlayer->m_Skills[i].SkillLevel,
			pSkill ? pSkill->m_szSkillName : "?", pSkill ? pSkill->m_nSkillRange : 0, pSkill ? pSkill->m_nHorseLimited : 0);
		for (int k = 0; k < skill_total; k++)
			if (theAttack.m_SettingSkills[k] == nId) DbgOut("  <%s>", s_szSkillSlot[k]);
		if (nId == g_pCurPlayer->m_nLeftSkillID) DbgOut("  [trai]");
		if (nId == g_pCurPlayer->m_nRightSkillID) DbgOut("  [phai]");
		if (pSkill && pSkill->m_dwSkillID != (DWORD)nId) DbgOut("  [m_dwSkillID %u LECH]", pSkill->m_dwSkillID);
		DbgOut("\n");
		nCount++;
	}
	for (int k = 0; k < skill_total; k++)
	{
		int nId = theAttack.m_SettingSkills[k];
		if (nId <= 0) continue;
		BOOL bHave = FALSE;
		for (int i = 0; i < MAX_NPCSKILL && !bHave; i++)
			bHave = (pPlayer->m_Skills[i].SkillId == nId);
		if (!bHave) DbgOut("CANH BAO: <%s> = %d (%s) khong co trong m_Skills\n", s_szSkillSlot[k], nId, SkillName(nId));
	}
	DbgOut("Tong: %d ky nang\n", nCount);
	g_DbgReply.nResult[0] = nCount;
	g_DbgReply.bOK = TRUE;
}

static void SkillInfo(int nSkillId)
{
	KSkill *pSkill = GetSkill(nSkillId);
	if (!pSkill)
	{
		DbgOut("LOI: SkillID = %d ngoai 1..%d\n", nSkillId, DBG_MAX_SKILL - 1);
		return;
	}
	DbgOut("KSkill[%d] @ 0x%08X (SKL_BASE_ADD 0x%08X + (ID-1) * 0x%X)\n", nSkillId, (DWORD)pSkill, SKL_BASE_ADD, SKL_DATA_SIZE);
	DbgOut("m_nIndex %d  m_nSkillType %d  m_nMaxLevel %d  m_dwSkillID %u%s\n", pSkill->m_nIndex, pSkill->m_nSkillType,
		pSkill->m_nMaxLevel, pSkill->m_dwSkillID, (pSkill->m_dwSkillID == (DWORD)nSkillId) ? "" : "  <== LECH voi ID!");
	DbgOut("m_szSkillName '%s'  m_nSkillRange (AttackRadius) %d  m_nHorseLimited %d\n",
		pSkill->m_szSkillName, pSkill->m_nSkillRange, pSkill->m_nHorseLimited);
	g_DbgReply.nResult[0] = pSkill->m_dwSkillID;
	g_DbgReply.nResult[1] = pSkill->m_nHorseLimited;
	g_DbgReply.bOK = TRUE;
}

static void StateList()
{
	if (!DbgCheckPlayer()) return;
	BOOL bNeed[3] = { TRUE, TRUE, TRUE };
	int nCount = 0;
	DbgOut("Trang thai tren nguoi (m_pHeadNode, m_nStateCount %d):\n", P_PLAYER->m_nStateCount);
	for (KStateNode *pNode = (KStateNode*)P_PLAYER->m_pHeadNode; pNode && nCount < 64; pNode = (KStateNode*)pNode->GetNext(), nCount++)
	{
		DbgOut("  SkillID %4d Lv %-2d LeftTime %-6d %s", pNode->m_SkillID, pNode->m_Level, pNode->m_LeftTime, SkillName(pNode->m_SkillID));
		for (int i = 0; i < 3; i++)
		{
			if (pNode->m_SkillID != theAttack.m_SettingSkills[i]) continue;
			DbgOut("  <supp0%d>", i + 1);
			if (pNode->m_LeftTime > 180) bNeed[i] = FALSE;
		}
		DbgOut("\n");
	}
	DbgOut("HandleDirectSkills (buff khi LeftTime <= 180):\n");
	for (int i = 0; i < 3; i++)
	{
		int nId = theAttack.m_SettingSkills[i];
		if (nId <= 0) { DbgOut("  supp0%d: chua cai\n", i + 1); continue; }
		DbgOut("  supp0%d %d (%s): %s  m_nDoingDirectSkill %d\n", i + 1, nId, SkillName(nId),
			bNeed[i] ? "SE BUFF" : "con hieu luc", theAttack.m_nDoingDirectSkill[i]);
	}
	g_DbgReply.nResult[0] = nCount;
	g_DbgReply.bOK = TRUE;
}

static void NpcList(int nMaxDist)
{
	if (!DbgCheckPlayer()) return;
	int nPlayerX, nPlayerY, nX, nY;
	P_PLAYER->GetMapPos(&nPlayerX, &nPlayerY);
	if (nMaxDist <= 0) nMaxDist = theAttack.m_nSearchRange > 0 ? theAttack.m_nSearchRange * 2 : 1600;
	DbgOut("Quai quanh nhan vat (dZ <= %d), RefreshNpcs + SearchNearestNpc(tam %d):\n", nMaxDist, theAttack.m_nSearchRange);
	DbgOut("Idx | SIdx | Life     | Gold | Rel    | dZ    | Ten => ket qua\n");
	int nCount = 0, nAlive = 0, nBest = 0, dBest = theAttack.m_nSearchRange;
	for (int i = 1; i < MAX_NPC; i++)
	{
		KNpc *pNpc = g_pNpcArray[i];
		if (!pNpc->IsExist() || pNpc->m_Kind != kind_normal) continue;
		pNpc->GetMapPos(&nX, &nY);
		int dZ = __distance(nPlayerX, nPlayerY, nX, nY);
		if (dZ > nMaxDist) continue;
		const char *szReason = AliveNpcReason(i);
		DbgOut("%-3d | %-4d | %-8d | %d/%-2d | %-6s | %-5d | %s", i, pNpc->m_NpcSettingIdx, pNpc->m_CurrentLife,
			pNpc->m_cGold.m_nIsGold || pNpc->m_cGold.m_nIsGolding, pNpc->m_cGold.m_nGoldType,
			RelationName(g_pCoreShell->GetNPCRelation(i)), dZ, pNpc->Name);
		if (szReason)
			DbgOut(" => BO QUA: %s\n", szReason);
		else
		{
			DbgOut(" => %s%s\n", dZ < theAttack.m_nSearchRange ? "trong tam" : "ngoai tam",
				(pNpc->m_dwID == theAttack.m_LastNpcID) ? " (m_LastNpcID - bo qua 1 lan)" : "");
			nAlive++;
			if (dZ < dBest && pNpc->m_dwID != theAttack.m_LastNpcID) { dBest = dZ; nBest = i; }
		}
		nCount++;
	}
	DbgOut("Tong: %d quai, %d hop le. Gan nhat trong tam: %d", nCount, nAlive, nBest);
	if (nBest && theAttack.m_bAvoidBoss && g_pNpcArray[nBest]->m_cGold.m_nGoldType == 16) DbgOut(" (BOSS GoldType 16 - m_bAvoidBoss => khong danh)");
	DbgOut("\n(chua tinh: uu tien BOSS, NPC nhiem vu m_nNpcSIndex %d, khi da du 3 quai lag thi chon quai xa nhat)\n", theQuest.m_nNpcSIndex);
	g_DbgReply.nResult[0] = nCount;
	g_DbgReply.nResult[1] = nBest;
	g_DbgReply.bOK = TRUE;
}

static void SearchOnce(int nRange)
{
	if (!DbgCheckPlayer()) return;
	int nX, nY;
	if (nRange <= 0) nRange = theAttack.m_nSearchRange;
	P_PLAYER->GetMapPos(&nX, &nY);
	KAliveNpc::RefreshNpcs();
	int nAlive = 0;
	for (int i = 1; i < MAX_NPC; i++)
		if (theAttack.m_AliveNpc[i].m_dwID) nAlive++;
	int nIdx = theAttack.SearchNearestNpc(nX, nY, nRange);
	DbgOut("RefreshNpcs: %d quai trong m_AliveNpc\n", nAlive);
	DbgOut("SearchNearestNpc((%d, %d), %d) = %d", nX, nY, nRange, nIdx);
	if (nIdx > 0)
		DbgOut(" - %s, da MoveTo toi quai. m_LastNpcID = %u\n", g_pNpcArray[nIdx]->Name, theAttack.m_LastNpcID);
	else
		DbgOut(" - khong co quai\n");
	g_DbgReply.nResult[0] = nIdx;
	g_DbgReply.bOK = TRUE;
}

static void AttackNpc(int nIdx)
{
	if (!DbgCheckPlayer() || !CheckNpcIdx(nIdx)) return;
	KNpc *pNpc = g_pNpcArray[nIdx];
	int nRelation = g_pCoreShell->GetNPCRelation(nIdx);
	theAttack.SetActiveSkill(g_pCurPlayer->m_nLeftSkillID);
	theAttack.SetPeopleIdx(nIdx);
	theMove.m_nStatus = status_attack;
	DbgOut("Npc[%d] %s  Kind %d  Relation %s%s\n", nIdx, pNpc->Name, pNpc->m_Kind, RelationName(nRelation),
		nRelation == relation_enemy ? "" : "  (khong phai enemy - game se khong danh)");
	DbgOut("SetActiveSkill(%d) -> ActiveSkillID %d  AttackRadius %d;  SetPeopleIdx(%d) -> PEOPLE_IDX %d\n",
		g_pCurPlayer->m_nLeftSkillID, P_PLAYER->m_ActiveSkillID, P_PLAYER->m_CurrentAttackRadius, nIdx, PEOPLE_IDX);
	g_DbgReply.bOK = TRUE;
}

static void BadNpcs(BOOL bClear)
{
	for (int i = 0; i < 3; i++)
	{
		KLagNpc &n = theAttack.m_BadNpc[i];
		DbgOut("m_BadNpc[%d]: ID %u  Life %d  LifeTime %d", i, n.m_dwID, n.m_CurLife, n.m_LifeTime);
		for (int k = 1; k < MAX_NPC && n.m_dwID; k++)
			if (g_pNpcArray[k]->IsExist() && g_pNpcArray[k]->m_dwID == n.m_dwID) { DbgOut("  = Npc[%d] %s", k, g_pNpcArray[k]->Name); break; }
		DbgOut("\n");
		if (bClear) n = 0;
	}
	if (bClear) DbgOut("Da xoa m_BadNpc\n");
	g_DbgReply.bOK = TRUE;
}

static void PlayerList()
{
	if (!DbgCheckPlayer()) return;
	CVLAttack &a = theAttack;
	int nPlayerX, nPlayerY, nX, nY, nCount = 0;
	P_PLAYER->GetMapPos(&nPlayerX, &nPlayerY);
	DbgOut("AutoDefence %d  m_nDefenceRange %d  Camp minh %d  PKFlag %d  Series %d\n",
		a.m_nAutoDefence, a.m_nDefenceRange, P_PLAYER->m_CurrentCamp, P_PLAYER->m_nPKFlag, P_PLAYER->m_Series);
	DbgOut("Idx | Rel    | Camp | PK | He | Khac | dZ    | Ten => AutoDefence\n");
	for (int i = 1; i < MAX_NPC; i++)
	{
		KNpc *pNpc = g_pNpcArray[i];
		if (!pNpc->IsExist() || pNpc->m_Kind != kind_player || i == g_pCurPlayer->m_nIndex) continue;
		pNpc->GetMapPos(&nX, &nY);
		int dZ = __distance(nPlayerX, nPlayerY, nX, nY);
		int nRelation = g_pCoreShell->GetNPCRelation(i);
		const char *szVerdict = "KE THU";
		if (pNpc->m_CurrentLife <= 0) szVerdict = "da chet";
		else if (pNpc->m_CurrentCamp == camp_begin) szVerdict = "tan thu";
		else if (nRelation == relation_ally) szVerdict = "dong minh";
		else if (nRelation == relation_none && P_PLAYER->m_CurrentCamp == pNpc->m_CurrentCamp && P_PLAYER->m_CurrentCamp < camp_free) szVerdict = "cung phe";
		else if (nRelation == relation_none && pNpc->m_nPKFlag != 2) szVerdict = "khong do sat";
		else if (dZ >= a.m_nDefenceRange) szVerdict = "ke thu - ngoai tam";
		DbgOut("%-3d | %-6s | %-4d | %-2d | %-2d | %-4d | %-5d | %s => %s\n", i, RelationName(nRelation), pNpc->m_CurrentCamp,
			pNpc->m_nPKFlag, pNpc->m_Series, theMain.GetConquerVal(P_PLAYER->m_Series, pNpc->m_Series), dZ, pNpc->Name, szVerdict);
		nCount++;
	}
	DbgOut("Tong: %d nguoi choi. m_nEnemyIdx hien tai = %d\n", nCount, a.m_nEnemyIdx);
	if (!P_PLAYER->m_FightMode || !a.m_nAutoDefence) DbgOut("(AutoDefence khong chay: %s)\n", P_PLAYER->m_FightMode ? "m_nAutoDefence = 0" : "trong thanh");
	g_DbgReply.nResult[0] = nCount;
	g_DbgReply.bOK = TRUE;
}

BOOL DebugAttackCmd(const DEBUG_PARAM *pParam)
{
	const int *P = pParam->nParam;
	switch (pParam->nCmd)
	{
	case dbg_at_status:
		AttackStatus();
		break;
	case dbg_at_skills:
		SkillList();
		break;
	case dbg_at_skillinfo:
		SkillInfo(P[2]);
		break;
	case dbg_at_states:
		StateList();
		break;
	case dbg_at_npclist:
		NpcList(P[2]);
		break;
	case dbg_at_search:
		SearchOnce(P[2]);
		break;
	case dbg_at_attacknpc:
		AttackNpc(P[2]);
		break;
	case dbg_at_islag:
		if (!DbgCheckPlayer()) break;
		{
			int nTarget = theAttack.GetCurTargetIdx();
			g_DbgReply.nResult[0] = theAttack.IsLagTarget(nTarget);
			DbgOut("IsLagTarget(GetCurTargetIdx() = %d) = %d (%s)  m_LastNpcLife %d  PEOPLE_IDX sau = %d\n", nTarget, g_DbgReply.nResult[0],
				g_DbgReply.nResult[0] ? "TRUE = tim quai moi" : "FALSE = dang danh tot", theAttack.m_LastNpcLife, PEOPLE_IDX);
			DbgOut("(moi lan bam = 1 nhip 100ms cua bo dem lag; quai khong mat mau du lau se vao m_BadNpc)\n");
			BadNpcs(FALSE);
		}
		break;
	case dbg_at_badnpc:
		BadNpcs(P[2] == 1);
		break;
	case dbg_at_setdefskills:
		if (!DbgCheckPlayer()) break;
		{
			int nLeft = P[0] ? P[0] : theAttack.m_SettingSkills[skill_left];
			int nRight = P[1] ? P[1] : theAttack.m_SettingSkills[skill_right];
			DbgOut("Truoc: Left %d  Right %d\n", g_pCurPlayer->m_nLeftSkillID, g_pCurPlayer->m_nRightSkillID);
			theAttack.SetDefSkills(nLeft, nRight);
			DbgOut("SetDefSkills(%d (%s), %d (%s))\nSau: Left %d  Right %d%s\n", nLeft, SkillName(nLeft), nRight, SkillName(nRight),
				g_pCurPlayer->m_nLeftSkillID, g_pCurPlayer->m_nRightSkillID,
				(nLeft && g_pCurPlayer->m_nLeftSkillID != nLeft) ? "  (Left chua doi - OperationRequest GOI_SET_IMMDIA_SKILL loi?)" : "");
			g_DbgReply.bOK = TRUE;
		}
		break;
	case dbg_at_setactive:
		if (!DbgCheckPlayer()) break;
		{
			int nId = P[2] ? P[2] : g_pCurPlayer->m_nLeftSkillID;
			DbgOut("Truoc: ActiveSkillID %d  AttackRadius %d\n", P_PLAYER->m_ActiveSkillID, P_PLAYER->m_CurrentAttackRadius);
			theAttack.SetActiveSkill(nId);
			DbgOut("SetActiveSkill(%d %s) -> ActiveSkillID %d  AttackRadius %d\n", nId, SkillName(nId),
				P_PLAYER->m_ActiveSkillID, P_PLAYER->m_CurrentAttackRadius);
			g_DbgReply.bOK = TRUE;
		}
		break;
	case dbg_at_castself:
		if (!DbgCheckPlayer()) break;
		if (!GetSkill(P[2])) { DbgOut("LOI: SkillID Idx = %d\n", P[2]); break; }
		P_PLAYER->SendCommand(do_skill, P[2], -1, 1);
		theAttack.SendVLCmdSkill(P[2], -1, P_PLAYER->m_dwID);
		DbgOut("SendCommand(do_skill, %d, -1, 1) + SendVLCmdSkill(%d, -1, %u) - %s len ban than (nhu HandleDirectSkills)\n",
			P[2], P[2], P_PLAYER->m_dwID, SkillName(P[2]));
		g_DbgReply.bOK = TRUE;
		break;
	case dbg_at_casttarget:
		if (!DbgCheckPlayer()) break;
		{
			int nNpc = P[0] ? P[0] : PEOPLE_IDX;
			if (!GetSkill(P[2])) { DbgOut("LOI: SkillID Idx = %d\n", P[2]); break; }
			if (!CheckNpcIdx(nNpc)) break;
			theAttack.SendVLCmdSkill(P[2], -1, g_pNpcArray[nNpc]->m_dwID);
			DbgOut("SendVLCmdSkill(%d %s, -1, %u) len Npc[%d] %s\n", P[2], SkillName(P[2]), g_pNpcArray[nNpc]->m_dwID, nNpc, g_pNpcArray[nNpc]->Name);
			g_DbgReply.bOK = TRUE;
		}
		break;
	case dbg_at_shortcut:
		if (!DbgCheckPlayer()) break;
		theAttack.DoSpecialSkill(P[2], P[0] != 0);
		DbgOut("DoSpecialSkill(%d, %d) = ExecuteScript(\"%s(%d)\")\n", P[2], P[0] != 0, P[0] ? "DirectShortcutSkill" : "ShortcutSkill", P[2]);
		g_DbgReply.bOK = TRUE;
		break;
	case dbg_at_players:
		PlayerList();
		break;
	case dbg_at_autodefence:
		if (!DbgCheckPlayer()) break;
		DbgOut("Truoc: m_nEnemyIdx %d  PEOPLE_IDX %d\n", theAttack.m_nEnemyIdx, PEOPLE_IDX);
		for (int i = 0; i < 3; i++)	// AutoDefence chi chay 1/3 lan goi
			theAttack.AutoDefence();
		DbgOut("AutoDefence() x3 -> m_nEnemyIdx %d  PEOPLE_IDX %d  ActiveSkillID %d  PKFlag %d\n",
			theAttack.m_nEnemyIdx, PEOPLE_IDX, P_PLAYER->m_ActiveSkillID, P_PLAYER->m_nPKFlag);
		g_DbgReply.nResult[0] = theAttack.m_nEnemyIdx;
		g_DbgReply.bOK = TRUE;
		break;
	case dbg_at_pk:
		if (!DbgCheckPlayer()) break;
		{
			char szScript[32];
			sprintf(szScript, "Switch('PK',%d)", P[2]);
			g_DbgReply.nResult[0] = ExecuteScript(szScript);
			DbgOut("ExecuteScript(\"%s\") = %d  (PKFlag hien tai %d - doi server cap nhat)\n", szScript, g_DbgReply.nResult[0], P_PLAYER->m_nPKFlag);
			g_DbgReply.bOK = TRUE;
		}
		break;
	case dbg_at_autohit:
		theAttack.m_bAutoHit = !theAttack.m_bAutoHit;
		theAttack.SetAutoHit();
		DbgOut("m_bAutoHit = %d + SetAutoHit() (giong Ctrl+A; VLAuto gui lai tuy chon se ghi de)\n", theAttack.m_bAutoHit);
		g_DbgReply.nResult[0] = theAttack.m_bAutoHit;
		g_DbgReply.bOK = TRUE;
		break;
	default:
		return FALSE;
	}
	return TRUE;
}
