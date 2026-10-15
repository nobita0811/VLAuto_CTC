#include "stdafx.h"
#include "../GameDef.h"
#include "DTextFile.h"
#include "GameBase.h"
#include "JxCore.h"
#include "S3Client.h"
#include "VLClasses.h"

//================================Class CVLParty================================
CVLParty::CVLParty(void)
{
	memset(amMajor, 0, sizeof(amMajor));
	m_bPTAllYS = 0;
	m_bAcceptAllYS = 0;
}

void CVLParty::Breathe()
{
	if (g_NetworkOffline) return;
	if (!m_bAutoPT) return;

	static BOOL bReadyJoin = 0;
	static int nFewMemTime = 0;

	m_bPTAllYS = (m_nPTAll == 1 || (m_nPTAll == 2 && theQuest.m_nYeSouMapID));
	m_bAcceptAllYS = (m_nAcceptAll == 1 || (m_nAcceptAll == 2 && theQuest.m_nYeSouMapID));

	if (m_bAcceptAllYS)
	{
		if (!P_PLAYER->m_FightMode)
		{
			nFewMemTime = 0;
			return;
		}
		if (!IAM_IN_A_PARTY || bReadyJoin)
		{
			for (int i = 1; i < MAX_NPC; i++)
			{
				if (!g_pNpcArray[i]->IsExist() || g_pNpcArray[i]->m_Kind != kind_player) continue;
				if (IsInNoPTList(g_pNpcArray[i]->Name)) continue;
				AcceptInvite(g_pNpcArray[i]->Name);
			}
		}

		if (m_bAutoLeave)
		{
			bReadyJoin = 0;
			if (IAM_IN_A_PARTY && g_pTeam->m_nMemNum < 2)
			{
				nFewMemTime++;
				bReadyJoin = TRUE;
			}
			else if (g_pTeam->m_nMemNum + 1 < m_nAutoLeaveMemNum)
				nFewMemTime++;
			else
				nFewMemTime = 0;
			if (nFewMemTime >= m_nAutoLeaveTime * 40)
			{
				if (m_bPTAllYS && IAM_IN_A_PARTY && !IAM_THE_CAPTAIN)
				{
					SystemMessage("<color=yellow>Rêi nhãm ®Ó lËp nhãm míi!");
					g_pCoreShell->TeamOperation(TEAM_LEAVE, 0, 0);
					return;
				}
				bReadyJoin = TRUE;
			}
			else
			{
				if (!IAM_IN_A_PARTY) return;
				if (!IAM_THE_CAPTAIN) return;
			}
		}
		else
		{
			nFewMemTime = 0;
			bReadyJoin = 0;
		}

		if (!m_bPTAllYS)
		{
			if (!IAM_IN_A_PARTY) return;
			if (g_pTeam->m_nMemNum == 0) // Only me in PT
			{
				g_pCoreShell->TeamOperation(TEAM_LEAVE, 0, 0);
				return;
			}
		}
	}

	//Search for Higher Player
	int nHigherIdx = 0;
	DWORD dwHigherID = 0;
	int nHigherPlayer = -1;
	for (int i = 0; m_szMemberName[i][0] && i < MAX_MEMBERS; i++)
	{
		if (!strcmp(m_szMemberName[i], P_PLAYER->Name)) break;
		for (int j = 1; j < MAX_NPC; j++)
		{
			if (!g_pNpcArray[j]->IsExist() || g_pNpcArray[j]->m_Kind != kind_player) continue;
			if (!strcmp(g_pNpcArray[j]->Name, m_szMemberName[i]))
			{
				nHigherPlayer = i;
				nHigherIdx = j;
				dwHigherID = g_pNpcArray[j]->m_dwID;
				break; // break for j
			}
		}
		if (nHigherPlayer >= 0) break; // break for i
	}

	if (!IAM_IN_A_PARTY)
	{
		if (nHigherPlayer < 0) // no Leader found, I will be the Leader
		{
			g_pCoreShell->TeamOperation(TEAM_CREATE, 0, 0);
			nFewMemTime = 0; // Restart Counter
		}
		else if (!AcceptInvite(m_szMemberName[nHigherPlayer]))
		{
			KUiPlayerItem thePlayer;
			memset(&thePlayer, 0, sizeof(KUiPlayerItem));
			strcpy(thePlayer.Name, m_szMemberName[nHigherPlayer]);
			thePlayer.uId = dwHigherID;
			thePlayer.nIndex = nHigherIdx;
			thePlayer.nData = 1;
			ProcessPeople(&thePlayer, ACTION_JOINTEAM);
		}
		return;
	}

	if (IAM_THE_CAPTAIN)
	{	// I am the Captain
		if (nHigherPlayer >= 0)
		{
			if (!IsInSamePT(dwHigherID))
				g_pCoreShell->TeamOperation(TEAM_LEAVE, 0, 0); // Leave Team to Join other Team
		}
		else if (g_pTeam->m_nMemNum < 7)
			AcceptJoinAll();
		// Check Member Absent Time
		if (m_bAutoKick && P_PLAYER->m_FightMode && g_pTeam->m_nMemNum > 0)
			CheckAbsentMem();
		return;
	}
	if (nHigherPlayer >= 0)
	{	// I am a Member
		BOOL bInRightPT = FALSE;
		for (int i = 0; m_szMemberName[i][0] && i <= nHigherPlayer; i++)
		{
			if (!strcmp(g_pTeam->m_szMemName[0], m_szMemberName[i]))
			{
				bInRightPT = TRUE;
				break;
			}
		}
		if (!bInRightPT)
			g_pCoreShell->TeamOperation(TEAM_LEAVE, 0, 0);
		return;
	}
}

BOOL CVLParty::IsInSamePT(DWORD dwPlayerID)
{
	for (int i = 0; i < MAX_TEAM_MEMBER; i++)
	{
		if (g_pTeam->m_nMember[i] == dwPlayerID)
			return TRUE;
	}
	return 0;
}

BOOL CVLParty::IsInAcceptList(char* szPlayerName)
{
	for (int i = 0; m_szMemberName[i][0] && i < MAX_MEMBERS; i++)
	{
		if (!strcmp(szPlayerName, m_szMemberName[i]))
			return TRUE;
	}
	return 0;
}

BOOL CVLParty::AcceptInvite(char* szInviter)
{
	KUiSysMsgCentre* pSysMsg = (KUiSysMsgCentre*)GetPtrAddress(SMC_BASE_ADD);
	if (!pSysMsg) return FALSE;

	for (int i = 0; i < 6; i++)
	{
		if (pSysMsg->m_MsgHeap[i].nNumValid == 0)
			break;

		for (int j = 0; j < pSysMsg->m_MsgHeap[i].nNumValid; j++)
		{
			KSystemMessage* pMsg = pSysMsg->m_MsgHeap[i].pMsgList[j];

			char* pInviteMsg = strstr(pMsg->szMessage, "mêi tæ ®éi cña B¹n!");
			if (!pInviteMsg)
				break;

			if (!strcmp(pMsg->mInviter.Name, szInviter) && pMsg->mInviter.nIndex > 0)
			{
				g_pCoreShell->TeamOperation(TEAM_INVITE_RESPONSE, (int)& pMsg->mInviter, 1);
				return TRUE;
			}
		}
	}

	return FALSE;
}

void CVLParty::AcceptJoinAll()
{
	int nCaptain = -1;
	int i;	static int nLastInvited = 1;
	for (i = nLastInvited + 1; i < MAX_NPC; i++)
	{
		if (!g_pNpcArray[i]->IsExist() || g_pNpcArray[i]->m_Kind != kind_player) continue;
		if (IsInSamePT(g_pNpcArray[i]->m_dwID)) continue;
		KUiPlayerItem thePlayer;
		memset(&thePlayer, 0, sizeof(KUiPlayerItem));
		strcpy(thePlayer.Name, g_pNpcArray[i]->Name);
		thePlayer.uId = g_pNpcArray[i]->m_dwID;
		if (m_bPTAllYS && P_PLAYER->m_FightMode && !IsInNoPTList(g_pNpcArray[i]->Name))
		{
			g_pCoreShell->TeamOperation(TEAM_INVITE, (int)& thePlayer, 0);
			if (theMain.m_bAutoChat)
			{
				char szScript[64];
				sprintf(szScript, "%s, vµo PT b¹n ¬i!", g_pNpcArray[i]->Name);
				PostChatMessage(szScript);
			}
			nLastInvited = i;
			return;
		}
		else if (IsInAcceptList(g_pNpcArray[i]->Name))
		{
			g_pCoreShell->TeamOperation(TEAM_INVITE, (int)& thePlayer, 0);
			g_pCoreShell->TeamOperation(TEAM_JOIN_RESPONSE, (int)& thePlayer, 1);
		}
	}
	if (i >= MAX_NPC) nLastInvited = 0;
}

void CVLParty::CheckAbsentMem()
{
	for (int i = 1; i < MAX_TEAM_MEMBER; i++)
	{
		if (amMajor[i].nMemID > 0 && amMajor[i].nAbsentTime < 0)
		{
			KUiPlayerItem thePlayer;
			memset(&thePlayer, 0, sizeof(KUiPlayerItem));
			thePlayer.uId = amMajor[i].nMemID;
			g_pCoreShell->TeamOperation(TEAM_KICK, (int)& thePlayer, 0);
			amMajor[i].nMemID = -1;
			amMajor[i].nAbsentTime = 0;
			return;
		}
	}
	ABSENT_MEMBER amTemp[MAX_TEAM_MEMBER];
	memset(amTemp, 0, sizeof(amTemp));
	amTemp[0].nMemID = g_pTeam->m_nMember[0];
	for (int i = 1; i < MAX_TEAM_MEMBER; i++)
	{
		amTemp[i].nMemID = g_pTeam->m_nMember[i];
		if (amTemp[i].nMemID < 0) continue;
		for (int j = 0; j < MAX_TEAM_MEMBER; j++)
		{
			if (amTemp[i].nMemID == amMajor[j].nMemID)
			{
				amTemp[i].nAbsentTime = amMajor[j].nAbsentTime + 1;
				if (amTemp[i].nAbsentTime > m_nAutoKickTime * 40)
				{
					char szScript[128];
					sprintf(szScript, "Chat('CH_TEAM','Khai trõ [%s] ra khái nhãm v× v¾ng mÆt qu¸ l©u!!!')", g_pTeam->m_szMemName[i]);
					ExecuteScript(szScript);
					amTemp[i].nAbsentTime = -1; // Prepare to KICK
				}
				break;
			}
		}
	}

	memcpy(amMajor, amTemp, sizeof(amMajor));

	for (int i = 1; i < MAX_NPC; i++)
	{
		if (!g_pNpcArray[i]->IsExist() || g_pNpcArray[i]->m_Kind != kind_player) continue;
		for (int j = 1; j < MAX_TEAM_MEMBER; j++)
		{
			if (g_pTeam->m_nMember[j] == g_pNpcArray[i]->m_dwID)
			{
				amMajor[j].nAbsentTime = 0;
				break;
			}
		}
	}
}

void CVLParty::LoadNoPTList()
{
	memset(m_szNoPTNames, 0, sizeof(m_szNoPTNames));
	char szNoPTNameFile[MAX_PATH];
	sprintf(szNoPTNameFile, "%s\\Maps\\NoPTNames.txt", theMain.m_szCurFolder);

	DTextFile itemFile;
	if (!itemFile.Open(szNoPTNameFile))
		return;

	itemFile.ReadAllLines(m_szNoPTNames[0], MAX_NOPT_NAMES, 32);
}

BOOL CVLParty::IsInNoPTList(const char* szNpcName)
{
	int i = 0;
	while (m_szNoPTNames[i][0] && i < MAX_NOPT_NAMES)
	{
		if (strstr(szNpcName, m_szNoPTNames[i]))
			return TRUE;
		i++;
	}
	return 0;
}
