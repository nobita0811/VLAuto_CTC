#include "stdafx.h"
#include "../GameDef.h"
#include "GameBase.h"
#include "JxCore.h"
#include "S3Client.h"
#include "VLClasses.h"

SHORT __stdcall _VLGetKeyState(int nVirKey)
{
	if (nVirKey >= VK_F2 && nVirKey <= VK_F12)
		return (SHORT)0xFFFF;

	return _GetAsyncKeyState(nVirKey);
}

#define MAX_MAPLAG_NPC 1 
struct __MapLagNpc
{
	int nMapID;
	int nX, nY;
} MapLagNpc[MAX_MAPLAG_NPC] = { {124,51808,104064} };

//================================KAliveNpc================================
void KAliveNpc::RefreshNpcs()
{
	int nNpcX, nNpcY, dZ;
	memset(theAttack.m_AliveNpc, 0, sizeof(theAttack.m_AliveNpc));
	for (int i = 1; i < MAX_NPC; i++)
	{
		if (!g_pNpcArray[i]->IsExist()) continue;
		if (g_pNpcArray[i]->m_CurrentLife <= 0 || g_pNpcArray[i]->m_Kind != kind_normal) continue;
		KLagNpc lagNpc(g_pNpcArray[i]->m_dwID, g_pNpcArray[i]->m_CurrentLife);
		if (lagNpc == theAttack.m_BadNpc[0] || lagNpc == theAttack.m_BadNpc[1] || lagNpc == theAttack.m_BadNpc[2]) continue;
		g_pNpcArray[i]->GetMapPos(&nNpcX, &nNpcY);
		if (!theMove.m_bFollow || !theMove.m_nLeaderIdx) // Dang ko theo sau ai.
		{
			if (!theMove.IsNpcInRange(nNpcX, nNpcY)) continue;
		}
		BOOL bIgnore = 0;
		for (int j = 0; j < MAX_MAPLAG_NPC; j++)
		{
			if (MapLagNpc[j].nMapID == (*g_pMapID))
			{
				dZ = __distance(nNpcX, nNpcY, MapLagNpc[j].nX, MapLagNpc[j].nY);
				if (dZ < 64)
				{
					bIgnore = TRUE;
					break;
				}
			}
		}
		if (bIgnore) continue;
		int dRegX = g_pNpcArray[i]->m_RegX - P_PLAYER->m_RegX;
		int dRegY = g_pNpcArray[i]->m_RegY - P_PLAYER->m_RegY;
		int nRegionIndex = (dRegX + 1) * 3 + dRegY + 1;
		theAttack.m_AliveNpc[i].m_dwID = g_pNpcArray[i]->m_dwID;
		theAttack.m_AliveNpc[i].m_bIsGold = g_pNpcArray[i]->m_cGold.m_nIsGold || g_pNpcArray[i]->m_cGold.m_nIsGolding;
		theAttack.m_AliveNpc[i].m_nGoldType = g_pNpcArray[i]->m_cGold.m_nGoldType;
		theAttack.m_AliveNpc[i].m_CurLife = g_pNpcArray[i]->m_CurrentLife;
		theAttack.m_AliveNpc[i].m_nPosX = nNpcX;
		theAttack.m_AliveNpc[i].m_nPosY = nNpcY;
		theAttack.m_AliveNpc[i].m_nRegionIdx = nRegionIndex;
		theAttack.m_AliveNpc[i].m_nNpcSIdx = g_pNpcArray[i]->m_NpcSettingIdx;
	}
}

//================================Class CVLAttack================================

CVLAttack::CVLAttack(void)
{
	m_LastNpcID = 0;
	m_bDoingSkill = 0;
	m_bDoingSkillEx = 0;
	m_bSkillChanged = TRUE;
	m_nPeopleIdx = 0;
	m_nEnemyIdx = 0;
}

void CVLAttack::Breathe()
{
	int nX, nY, nRange;

	if (g_NetworkOffline) return;

	HandleDirectSkills();
	HandleInDirectSkills();
	if (m_bDoingSkill || m_bDoingSkillEx || theRecover.m_bDoingBuff || theMain.m_bPause) return;

	// ActiveSkill Adjustment
	if (P_PLAYER->m_CurrentAttackRadius < 75)
		P_PLAYER->m_CurrentAttackRadius = 1000;

	AutoDefence();

	if (m_nEnemyIdx || theQuest.m_bNeedGoPCK) return;

	if (m_bSkillChanged)
	{
		SetDefSkills(m_SettingSkills[skill_left], m_SettingSkills[skill_right]);
		int nSkillId = g_pCurPlayer->m_nLeftSkillID;
		SetActiveSkill(nSkillId);

		if (m_nHorseMode == 0)
		{
			KSkill* pActSkill = (KSkill*)(SKL_BASE_ADD + (nSkillId - 1) * SKL_DATA_SIZE);
			m_bDownHorse = (BOOL)pActSkill->m_nHorseLimited;
		}
		else
			m_bDownHorse = (m_nHorseMode == 1);
		m_bSkillChanged = 0;
	}

	if (!P_PLAYER->m_FightMode || !m_bAutoHit) return;
	if (theMove.m_bFindingWay) return;
	if (theMove.m_nStatus == status_movefirst) return;
	if (theMove.m_nStatus == status_move && !theMove.m_bAttackWhenMove) return;

	ProcessAttack();

	// BOSS wait setting
	if (m_bBossAttacking)
	{
		int nSkillId = g_pCurPlayer->m_nLeftSkillID;
		if (m_SettingSkills[skill_boss])
			nSkillId = m_SettingSkills[skill_boss];
		SetActiveSkill(nSkillId);

		KSkill* pActBossSkill = (KSkill*)(SKL_BASE_ADD + (nSkillId - 1) * SKL_DATA_SIZE);
		m_bDownHorse = (BOOL)pActBossSkill->m_nHorseLimited;
		if (m_bDownHorse && P_PLAYER->m_bRideHorse)
		{
			ExecuteScript(SCK_SHORTCUT_HORSE); // Xuong ngua
			return;
		}

		m_bBossAttacking = 0;
		m_nBossWait = MAX_BOSS_WAIT;
	}

	//Check if Target is LAG
	int nTargetIdx = GetCurTargetIdx();
	if (!IsLagTarget(nTargetIdx)) return;

	KAliveNpc::RefreshNpcs();

	P_PLAYER->GetMapPos(&nX, &nY);

	nRange = m_nSearchRange;
	if (theMove.m_bFollow)
	{
		if (!theMove.m_nLeaderIdx && theMove.m_bNoAttackFollow)
		{
			if (theMove.m_bAroundPointPK || theMove.m_bRouteMove)
				theMove.m_nStatus = status_idle;
			return;
		}
		if (theMove.m_nLeaderIdx)
		{
			if (g_pNpcArray[theMove.m_nLeaderIdx]->IsExist())
			{
				g_pNpcArray[theMove.m_nLeaderIdx]->GetMapPos(&nX, &nY);
				nRange = theMove.m_nFollowDistance + 75;
			}
		}
	}
	if (theMove.m_nStatus == status_pickup && nRange > PLAYER_PICKUP_DISTANCE)
		nRange = PLAYER_PICKUP_DISTANCE;
	if (m_nBossWait)
	{
		m_nBossWait--;
		if (m_bHitClose)
		{
			if (nRange > PLAYER_PICKUP_DISTANCE)
				nRange = PLAYER_PICKUP_DISTANCE;
		}
		else
		{
			if (nRange > P_PLAYER->m_CurrentAttackRadius)
				nRange = P_PLAYER->m_CurrentAttackRadius;
		}
	}
	if (theRecover.m_bEmptyMana) nRange = 0;
	if (theQuest.m_bAutoDoQuest && theQuest.m_bGoTrainGetPK) nRange = PLAYER_PICKUP_DISTANCE;

	nTargetIdx = SearchNearestNpc(nX, nY, nRange);
	if (nTargetIdx > 0)
	{
		if (m_bDownHorse && P_PLAYER->m_bRideHorse)
		{
			ExecuteScript(SCK_SHORTCUT_HORSE); // Xuong ngua
			m_LastNpcID = 0;
		}
		else if (!m_bDownHorse && !P_PLAYER->m_bRideHorse && HAVE_HORSE)
		{
			ExecuteScript(SCK_SHORTCUT_HORSE); // Len ngua
			m_LastNpcID = 0;
		}
		else
		{
			SetPeopleIdx(nTargetIdx);
			m_bSkillChanged = TRUE;
			theMove.m_nStatus = status_attack;
		}
	}
	else
	{
		SetPeopleIdx(0);
		if (theMove.m_bMoveByMapYS || theMove.m_bAroundPointPK || theMove.m_bRouteMove)
			theMove.m_nStatus = status_idle;
		else if (!P_PLAYER->m_bRideHorse && P_PLAYER->m_Doing == do_stand)
			g_pCurPlayer->DoSit();
	}
}

void CVLAttack::SetPeopleIdx(int nIndex)
{
	PEOPLE_IDX = nIndex;
}

void CVLAttack::SetAutoHit()
{
	if (m_bAutoHit)
		SystemMessage("<color=yellow>Tù ®¸nh qu¸i (Ctrl+A): <color=green>BËt", 1);
	else
	{
		SetPeopleIdx(0);
		m_LastNpcID = 0;
		SystemMessage("<color=yellow>Tù ®¸nh qu¸i (Ctrl+A): <color>T¾t", 1);
	}

	if (m_nSkillRange < 75)
		m_nSkillRange = 75;

	if (m_nDefSkillRange < 75)
		m_nDefSkillRange = 75;

	//ExecuteScript("AddCommand('F10, '', 'DirectShortcutSkill(8)')");
}

void CVLAttack::SetDefSkills(int nLeftID, int nRightID)
{
	KUiSkillData theSkill;

	if (nLeftID)
	{
		theSkill.uGenre = SKILL_SHORTCUT;
		theSkill.uId = nLeftID;
		g_pCoreShell->OperationRequest(GOI_SET_IMMDIA_SKILL, (int)& theSkill, 0);
	}
	if (nRightID)
	{
		theSkill.uGenre = SKILL_SHORTCUT;
		theSkill.uId = nRightID;
		g_pCoreShell->OperationRequest(GOI_SET_IMMDIA_SKILL, (int)& theSkill, 1);
	}
}

void CVLAttack::SetActiveSkill(int nSkillId)
{
	P_PLAYER->m_ActiveSkillID = nSkillId;
	int nSkillRange = P_PLAYER->m_CurrentAttackRadius;

	if (m_nEnemyIdx && m_bDefHitClose)
		nSkillRange = m_nDefSkillRange;
	else if (m_bHitClose)
		nSkillRange = m_nSkillRange;

	if (nSkillRange < 75)
		nSkillRange = 1000;

	P_PLAYER->m_CurrentAttackRadius = nSkillRange;
}

void CVLAttack::HandleDirectSkills()
{
	static int nTimerCount = 0;
	if (!P_PLAYER->m_FightMode)
	{
		nTimerCount = 0;
		return;
	}
	nTimerCount++;

	BOOL bDrSkillFlag[3] = { TRUE, TRUE, TRUE };
	KStateNode* pNode = (KStateNode*)P_PLAYER->m_pHeadNode;
	while (pNode)
	{
		if (pNode->m_LeftTime > 180)
		{
			if (pNode->m_SkillID == m_SettingSkills[0])
				bDrSkillFlag[0] = 0;
			if (pNode->m_SkillID == m_SettingSkills[1])
				bDrSkillFlag[1] = 0;
			if (pNode->m_SkillID == m_SettingSkills[2])
				bDrSkillFlag[2] = 0;
		}
		pNode = (KStateNode*)pNode->GetNext();
	}
	for (int i = 0; i < 3; i++)
	{
		if (m_SettingSkills[i] <= 0) continue;
		if (m_nDoingDirectSkill[i]) continue;

		BOOL bNeedSkill = bDrSkillFlag[i];

		if (bNeedSkill)
		{
			m_nDoingDirectSkill[i] = 18;	// 1800ms
			m_LastNpcID = 0;
			m_bDoingSkill = TRUE;
			SetPeopleIdx(0);
		}
	}
	for (int i = 0; i < 3; i++)
	{
		if (m_nDoingDirectSkill[i] <= 6) continue;	// 600ms
		m_nDoingDirectSkill[i]--;
		if (m_nDoingDirectSkill[i] == 12)	// 1200ms
		{
			P_PLAYER->SendCommand(do_skill, m_SettingSkills[i], -1, 1);
			theAttack.SendVLCmdSkill(m_SettingSkills[i], -1, P_PLAYER->m_dwID);
		}
		return;
	}
	m_bDoingSkill = FALSE;
	for (int i = 0; i < 3; i++)
		if (m_nDoingDirectSkill[i]) m_nDoingDirectSkill[i]--;
}

void CVLAttack::HandleInDirectSkills()
{
	static int nTimerCount = 0;
	static int nLastMana = 0;
	static int nLastLife = 0;

	if (m_AlterSkill[0].nAlterType == 0)
	{
		static int nSkillIdx = 0;
		static int nTimeWait = 0;

		if (!P_PLAYER->m_FightMode)
		{
			nTimerCount = 0;
			return;
		}
		if (nSkillIdx && nTimeWait == 6)
		{
			DoSpecialSkill(nSkillIdx, 1);
			nSkillIdx = 0;
		}
		if (nTimeWait)
		{
			nTimeWait--;
			return;
		}
		m_bDoingSkillEx = FALSE;
		if (m_AlterSkill[0].nUseSkill)
		{
			if (nTimerCount % (m_AlterSkill[0].nAlterValue / 100) == 1)
			{
				m_LastNpcID = 0;
				m_bDoingSkillEx = TRUE;
				SetPeopleIdx(0);
				nTimeWait = 12;
				nSkillIdx = m_AlterSkill[0].nUseSkill - 1;
			}
		}
		if (m_AlterSkill[1].nUseSkill)
		{
			if (nTimerCount % (m_AlterSkill[1].nAlterValue / 100) == 2)
			{
				m_LastNpcID = 0;
				m_bDoingSkillEx = TRUE;
				SetPeopleIdx(0);
				nTimeWait = 12;
				nSkillIdx = m_AlterSkill[1].nUseSkill - 1;
			}
		}
		nTimerCount++;
		return;
	}

	if (m_nEnemyIdx && m_SettingSkills[skill_defence])
	{
		nTimerCount = 0;
		return;
	}
	int nTotalTime = 0;

	if (m_AlterSkill[0].nUseSkill && m_AlterSkill[1].nUseSkill)
	{
		int nSkillIdx1 = m_AlterSkill[0].nUseSkill - 1;
		int nSkillIdx2 = m_AlterSkill[1].nUseSkill - 1;

		switch (m_AlterSkill[0].nAlterType)
		{
		case 0: //DirectSkill
			break;
		case 1: //By Time
			nTotalTime = (m_AlterSkill[0].nAlterValue + m_AlterSkill[1].nAlterValue) / 100;
			if (nTotalTime > 0)
			{
				if (nTimerCount % nTotalTime == 0)
					DoSpecialSkill(nSkillIdx1, 0);
				else if (nTimerCount % nTotalTime == m_AlterSkill[0].nAlterValue / 100)
					DoSpecialSkill(nSkillIdx2, 0);
			}
			break;
		case 2: //By Mana
			if (P_PLAYER->m_CurrentMana < nLastMana && P_PLAYER->m_CurrentMana < m_AlterSkill[0].nAlterValue)
				DoSpecialSkill(nSkillIdx1, 0);
			if (P_PLAYER->m_CurrentMana > nLastMana && P_PLAYER->m_CurrentMana > m_AlterSkill[1].nAlterValue)
				DoSpecialSkill(nSkillIdx2, 0);
			nLastMana = P_PLAYER->m_CurrentMana;
			break;
		case 3: //By Health
			if (P_PLAYER->m_CurrentLife < nLastLife && P_PLAYER->m_CurrentLife < m_AlterSkill[0].nAlterValue)
				DoSpecialSkill(nSkillIdx1, 0);
			if (P_PLAYER->m_CurrentLife > nLastLife && P_PLAYER->m_CurrentLife > m_AlterSkill[1].nAlterValue)
				DoSpecialSkill(nSkillIdx2, 0);
			nLastLife = P_PLAYER->m_CurrentLife;
			break;
		}
	}
	nTimerCount++;
}

void CVLAttack::DoSpecialSkill(int nSkill, BOOL bDirect)
{
	char szScript[32];
	if (bDirect)
		sprintf(szScript, "DirectShortcutSkill(%d)", nSkill);
	else
		sprintf(szScript, "ShortcutSkill(%d)", nSkill);
	ExecuteScript(szScript);
	m_bSkillChanged = TRUE;
}

BOOL CVLAttack::IsLagTarget(int nIndex)
{
	static DWORD dwLastNpcID = 0;
	static int nLagCount = 0;
	static int nFollowCount = 0;

	for (int i = 0; i < 3; i++)
	{
		if (m_BadNpc[i].m_LifeTime)
			m_BadNpc[i].m_LifeTime--;
		else
			m_BadNpc[i] = 0;
	}

	if (!nIndex)
		return TRUE;

	KNpc* pNpc = g_pNpcArray[nIndex];
	if (theMove.m_bFollow && pNpc->m_Kind == kind_player)
	{	//Follow a Player in 800ms only
		if (nFollowCount < 8)
		{
			nFollowCount++;
			return 0;
		}
		else
		{
			SetPeopleIdx(0);
			nFollowCount = 0;
			return TRUE;
		}
	}
	if (!pNpc->IsExist() || pNpc->m_Kind != kind_normal)
	{
		SetPeopleIdx(0);
		return TRUE;
	}
	if (m_bAvoidBoss && pNpc->m_cGold.m_nGoldType == 16)
	{
		SetPeopleIdx(0);
		return TRUE;
	}
	if (pNpc->m_cGold.m_nIsGold || pNpc->m_cGold.m_nIsGolding)
	{
		m_bBossAttacking = TRUE;	//Attacking a BOSS
		m_nBossWait = 0;
	}
	if (pNpc->m_dwID != dwLastNpcID)
	{
		nLagCount = 0; // new Npc, count from 0
		dwLastNpcID = pNpc->m_dwID;
	}
	if (pNpc->m_CurrentLife >= 0)
	{
		if (pNpc->m_CurrentLife < m_LastNpcLife - 1)
		{	// OK, Npc is dying
			m_LastNpcLife = pNpc->m_CurrentLife;
			nLagCount = 0;
			theMove.m_nStatus = status_attack;
			return 0;
		}
		else
		{	// Oh, Npc is not hurt
			m_LastNpcLife = pNpc->m_CurrentLife;
			int nLagRunCheck;
			if (P_PLAYER->m_CurrentRunSpeed)
				nLagRunCheck = MAX_LAG_CHECK / P_PLAYER->m_CurrentRunSpeed;
			else
				nLagRunCheck = MAX_LAG_CHECK / 10;

			int nLagAttCheck = nLagRunCheck;
			if (P_PLAYER->m_Level < 150)
				nLagAttCheck += (ATT_LAG_CHECK - P_PLAYER->m_Level / 5);
			else
				nLagAttCheck += ATT_LAG_CHECK - 30;
			nLagCount++;

			if (P_PLAYER->m_Doing == do_attack && nLagCount < nLagRunCheck && nLagCount > 2)
				nLagCount = nLagRunCheck; //attacking then increase check lag
			if (nLagCount < nLagAttCheck) return 0;
			for (int i = 0; i < 3; i++)
			{
				if (m_BadNpc[i].IsNull())
				{
					m_BadNpc[i].SetValue(pNpc->m_dwID, pNpc->m_CurrentLife);
					if (theMain.m_bAutoChat)
					{
						char szChat[16];
						sprintf(szChat, "SayPhrase(%d)", nIndex % 10);
						ExecuteScript(szChat);
					}
					break;
				}
			}
		}
	}
	return TRUE;
}

int CVLAttack::SearchNearestNpc(int nX, int nY, int nRange)
{
	int dZ, dMax = 0;
	int nMinIndex = 0, dMin = nRange;
	int nMinQIndex = 0, dMinQ = nRange;

	BOOL bNoFullLag = (m_BadNpc[0].IsNull() || m_BadNpc[1].IsNull() || m_BadNpc[2].IsNull());

	for (int i = 1; i < MAX_NPC; i++)
	{
		if (!m_AliveNpc[i].m_dwID) continue;
		if (m_AliveNpc[i].m_dwID == m_LastNpcID)
		{
			m_LastNpcID = 0;
			continue;
		}
		dZ = __distance(m_AliveNpc[i].m_nPosX, m_AliveNpc[i].m_nPosY, nX, nY);
		if (dZ >= nRange) continue;

		if (m_bAvoidBoss && m_AliveNpc[i].m_nGoldType == 16 && dZ <= 800) return 0;

		if (bNoFullLag)
		{
			if (theMain.m_bAttackBossFirst && m_AliveNpc[i].m_bIsGold) // Hey, BOSS
			{
				nMinIndex = i;
				break;
			}
			if (dZ < dMin)
			{
				dMin = dZ;
				nMinIndex = i;
			}
			if (m_AliveNpc[i].m_nNpcSIdx == theQuest.m_nNpcSIndex)
			{
				if (dZ < dMinQ)
				{
					dMinQ = dZ;
					nMinQIndex = i;
				}
			}
		}
		else if (dZ > dMax)
		{	//More than 3 lagged mobs
			dMax = dZ;
			nMinIndex = i;
			if (dZ > 128)
				break;
		}
	}
	if (nMinQIndex) nMinIndex = nMinQIndex;
	if (nMinIndex)
	{
		if (m_bAvoidBoss && m_AliveNpc[nMinIndex].m_nGoldType == 16)
			return 0;

		m_LastNpcID = m_AliveNpc[nMinIndex].m_dwID; // Save Last Npc ID
		m_LastNpcLife = m_AliveNpc[nMinIndex].m_CurLife; // Save Last Npc Life
		int nDestX = m_AliveNpc[nMinIndex].m_nPosX;
		int nDestY = m_AliveNpc[nMinIndex].m_nPosY;
		g_pCurPlayer->MoveTo(nDestX, nDestY);
	}
	return nMinIndex;
}

void CVLAttack::AutoDefence()
{
	static int nTimerCount = 0;

	nTimerCount++;
	if (nTimerCount % 3 != 1) return;

	if (!m_bAlwaysF9)
	{
		static int nPKTimeCount = 0;
		if (P_PLAYER->m_nPKFlag)
			nPKTimeCount++;
		else
			nPKTimeCount = 0;
		if (nPKTimeCount > 600 && nPKTimeCount % 50 == 1 && !m_nEnemyIdx)
		{	// 600 ~ 3 minutes
			ExecuteScript("Switch('PK',0)");
			return;
		}
	}

	int nDefenceMode = m_nAutoDefence;
	if (theQuest.m_bAutoDoQuest &&
		theQuest.m_bGoTrainGetPK &&
		theQuest.m_TongPKOpt.m_bKillOther)
		nDefenceMode = 5;

	if (!P_PLAYER->m_FightMode || !nDefenceMode) return;
	if (P_PLAYER->m_CurrentCamp == camp_begin) return;

	//Search for Enemies
	int nEnemyIdx[3] = { 0, 0, 0 };
	int nEnemyIndex = 0;
	int nMaxConquer = -1;

	int nNpcX, nNpcY, nPlayerX, nPlayerY, dZ, nRelation;
	P_PLAYER->GetMapPos(&nPlayerX, &nPlayerY);

	for (int i = 1; i < MAX_NPC; i++)
	{
		if (!g_pNpcArray[i]->IsExist()) continue;
		if (g_pNpcArray[i]->m_CurrentLife <= 0 || g_pNpcArray[i]->m_Kind != kind_player) continue;
		if (g_pNpcArray[i]->m_CurrentCamp == camp_begin) continue;
		if (nDefenceMode == 5 && !strcmp(g_pNpcArray[i]->Name, theQuest.m_TongPKOpt.m_szVictimName))
		{
			if (theQuest.m_nDoQuestStep < 15)
			{
				g_pNpcArray[i]->GetMapPos(&nNpcX, &nNpcY);
				dZ = __distance(nPlayerX, nPlayerY, nNpcX, nNpcY);
				if (dZ > m_nDefenceRange) continue;
				if (g_pNpcArray[i]->m_CurrentCamp == P_PLAYER->m_CurrentCamp)
				{
					KUiPlayerItem thePlayer;
					memset(&thePlayer, 0, sizeof(KUiPlayerItem));
					strcpy(thePlayer.Name, g_pNpcArray[i]->Name);
					thePlayer.uId = g_pNpcArray[i]->m_dwID;
					thePlayer.nIndex = i;
					thePlayer.nData = 1;
					ProcessPeople(&thePlayer, ACTION_REVENGE);
				}
				else
					ExecuteScript("Switch('PK',2)");
				theQuest.m_nDoQuestStep = 15;
				break;
			}
		}
		nRelation = g_pCoreShell->GetNPCRelation(i);
		if (nRelation == relation_ally) continue;
		if (nRelation == relation_none)
		{
			if (P_PLAYER->m_CurrentCamp == g_pNpcArray[i]->m_CurrentCamp && P_PLAYER->m_CurrentCamp < camp_free) continue;
			if (g_pNpcArray[i]->m_nPKFlag != 2) continue;
		}
		g_pNpcArray[i]->GetMapPos(&nNpcX, &nNpcY);
		dZ = __distance(nPlayerX, nPlayerY, nNpcX, nNpcY);
		if (dZ < m_nDefenceRange)
		{
			if (nDefenceMode == 2)
			{
				theRecover.OnUseAM();
				return;
			}
			int nConquer = theMain.GetConquerVal(P_PLAYER->m_Series, g_pNpcArray[i]->m_Series);
			if (!nEnemyIdx[0])
			{
				nEnemyIdx[0] = i;
				if (nConquer >= nMaxConquer)
				{
					nMaxConquer = nConquer;
					nEnemyIndex = i;
				}
			}
			else if (!nEnemyIdx[1])
			{
				nEnemyIdx[1] = i;
				if (nConquer >= nMaxConquer)
				{
					nMaxConquer = nConquer;
					nEnemyIndex = i;
				}
			}
			else if (!nEnemyIdx[2])
			{
				nEnemyIdx[2] = i;
				if (nConquer >= nMaxConquer)
				{
					nMaxConquer = nConquer;
					nEnemyIndex = i;
				}
				break;
			}
		}
	}
	if (nEnemyIndex)
	{
		SetPeopleIdx(nEnemyIndex);
		m_nEnemyIdx = nEnemyIndex;
		theMove.m_nStatus = status_attack;

		int nSkillId = g_pCurPlayer->m_nLeftSkillID;
		if (m_SettingSkills[skill_defence])
			nSkillId = m_SettingSkills[skill_defence];
		SetActiveSkill(nSkillId);

		if (m_bDefHorseMode)
		{
			KSkill* pActSkill = (KSkill*)(SKL_BASE_ADD + (nSkillId - 1) * SKL_DATA_SIZE);
			m_bDownHorse = (BOOL)pActSkill->m_nHorseLimited;
			if (m_bDownHorse && P_PLAYER->m_bRideHorse)
				ExecuteScript(SCK_SHORTCUT_HORSE);
		}

		if (!P_PLAYER->m_nPKFlag && nRelation == relation_none)
			ExecuteScript("Switch('PK',1)");
	}
	else if (m_nEnemyIdx)
	{
		SetDefSkills(m_SettingSkills[skill_left], 0);
		m_nEnemyIdx = 0;
	}
}

void CVLAttack::ProcessAttack()
{
#ifndef VLTK_FREE_MODE
	if (!P_PLAYER->m_FightMode || PEOPLE_IDX != 0)
		return;

	int nEnemyX, nEnemyY, nPlayerX, nPlayerY;
	KNpc* pEnemy = g_pNpcArray[m_nPeopleIdx];

	if (!pEnemy->IsExist() || pEnemy->m_Doing == do_death || pEnemy->m_Doing == do_revive)
	{
		SetPeopleIdx(0);
		return;
	}

	int nRelation = g_pCoreShell->GetNPCRelation(m_nPeopleIdx);
	if (nRelation != relation_enemy)
	{
		SetPeopleIdx(0);
		return;
	}

	pEnemy->GetMapPos(&nEnemyX, &nEnemyY);
	P_PLAYER->GetMapPos(&nPlayerX, &nPlayerY);
	int nDistance = __distance(nEnemyX, nEnemyY, nPlayerX, nPlayerY);

	if (nDistance > P_PLAYER->m_CurrentAttackRadius)
	{
		int attRadius = P_PLAYER->m_CurrentAttackRadius * 80 / 100;
		int nDestX = nEnemyX - (attRadius * (nEnemyX - nPlayerX) / nDistance);
		int nDestY = nEnemyY - (attRadius * (nEnemyY - nPlayerY) / nDistance);

		g_pCurPlayer->MoveTo(nDestX, nDestY);
	}
	else
	{
		int nX = nEnemyX - nPlayerX + 400;
		int nY = (nEnemyY - nPlayerY) / 2 + 280;
		SendVLCmdSkill(g_pCurPlayer->m_nLeftSkillID, nX, nY);
	}
#endif
}

int CVLAttack::GetCurTargetIdx()
{
#ifndef VLTK_FREE_MODE
	if (PEOPLE_IDX)
		return PEOPLE_IDX;
	return m_nPeopleIdx;
#else
	return PEOPLE_IDX;
#endif
}

void CVLAttack::SendVLCmdSkill(int nSkillId, int nX, int nY)
{
#ifndef VLTK_FREE_MODE
	KAutoLua::SenCmdSkill(nSkillId, nX, nY);
	//KUiSkillData* pShortcutSkills = (KUiSkillData*)0x00612B20;
	//KUiSkillData oldSkill = pShortcutSkills[8];
	//pShortcutSkills[8].uGenre = SKILL_SHORTCUT;
	//pShortcutSkills[8].uId = nSkillId;
	//pShortcutSkills[8].DataX = 1;

	//_SendMessage(theMain.m_hWndGame, WM_MOUSEMOVE, 0, MAKELPARAM((WORD)nX, (WORD)nY));
	//_SendMessage(theMain.m_hWndGame, WM_KEYDOWN, VK_F10, 0x00440001);
#else
	::SendClientCmdSkill(nSkillId, nX, nY);
#endif
}
