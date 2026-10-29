#include "stdafx.h"
#include "../GameDef.h"
#include "DTextFile.h"
#include "GameBase.h"
#include "JxCore.h"
#include "S3Client.h"
#include "VLClasses.h"

// To doi da on dinh: tat QTRACE trong file nay (doi thanh 1 de bat lai log PT o kenh he thong / Logs)
#define PARTY_TRACE 0
#if !PARTY_TRACE
#undef QTRACE
#define QTRACE(...) ((void)0)
#endif

//================================ CHAT GAN KHI MOI / NHAN PT ================================
// Sua loi chat o 2 dong #define duoi day (ma TCVN3, %s = ten nhan vat). Muon tat chat: de chuoi rong "".
// Khong dung dau nhay don ' trong loi chat (lenh chat la script Chat('CH_NEARBY','...')).
#define PT_CHAT_INVITE		TV(L"<color=green>VLCTCPro<color> : <color=red>%s <color>vào PT bạn ơi (shx)!")			// Khi Auto moi PT: "<ten>, vao PT ban oi!"
#define PT_CHAT_ACCEPT		TV(L"<color=green>VLCTCPro<color> : cảm ơn <color=red>%s<color>  nhé^0^!")		// Khi Auto nhan loi moi PT: "Cam on <ten>!"
#define PT_CHAT_REPEAT_MS	60000					// Cung 1 nguoi chi chat lai sau 60s (che do d.sach moi lai moi 1.5s)

static void PartyChat(const char* szFormat, const char* szName)
{
	static struct { char szName[32]; DWORD dwTick; } s_Last[16];	// Moi nguoi 1 o, tranh chat lap khi moi nhieu nguoi
	static int s_nNext = 0;
	if (!szFormat[0])
		return;
	DWORD dwNow = GetTickCount();
	int k;
	for (k = 0; k < 16; k++)
		if (!strcmp(s_Last[k].szName, szName))
			break;
	if (k < 16 && dwNow - s_Last[k].dwTick < PT_CHAT_REPEAT_MS)
		return;
	if (k >= 16)
	{
		k = s_nNext;
		s_nNext = (s_nNext + 1) % 16;
		strncpy(s_Last[k].szName, szName, sizeof(s_Last[k].szName) - 1);
	}
	s_Last[k].dwTick = dwNow;
	char szMsg[128];
	_snprintf(szMsg, sizeof(szMsg) - 1, szFormat, szName);
	szMsg[sizeof(szMsg) - 1] = 0;
	PostChatMessage(szMsg);
}

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

	// "Chi moi trong d.sach" / "Chi nhan tu d.sach" / "Luon lam chu PT": bo qua thu tu chu PT trong danh sach
	if (m_nPTAll == 3 || m_nAcceptAll == 3 || m_bAlwaysCaptain)
	{
		BreatheListMode();
		return;
	}

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
					SystemMessage(1,TV(L"<color=yellow>Rời nhóm để lập nhóm mới!"));
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

int CVLParty::CountListNames()
{
	int n = 0;
	while (n < MAX_MEMBERS && m_szMemberName[n][0]) n++;
	return n;
}

// Che do danh sach (Moi/Nhan theo d.sach hoac tick "Luon lam chu PT"):
//  - Moi:  m_nPTAll == 3 -> chi moi nguoi trong danh sach; 1/2 (dang bat) -> moi tat ca; 0 -> khong moi
//  - Nhan: m_nAcceptAll == 3 -> chi nhan loi moi cua nguoi trong danh sach; 1/2 (dang bat) -> nhan tat ca; 0 -> khong nhan
//  - Tick "Luon lam chu PT": chua co PT thi tu lap; dang la thanh vien PT nguoi khac thi roi ra de tu lap.
//    La chu PT: moi theo o Moi, dong y yeu cau xin vao PT cua nguoi trong danh sach.
//  - Khong tick: khong tu lap PT, chi nhan loi moi theo o Nhan. Neu bi con lai 1 minh trong PT (chu PT) thi roi PT
//    de nhan loi moi khac.
void CVLParty::BreatheListMode()
{
	BOOL bInviteList = (m_nPTAll == 3);
	BOOL bAcceptList = (m_nAcceptAll == 3);
	BOOL bCanAccept = bAcceptList || m_bAcceptAllYS;

	if (!m_bAlwaysCaptain)
	{
		if (!IAM_IN_A_PARTY)
		{
			if (!bCanAccept)
				return;
			for (int i = 1; i < MAX_NPC; i++)
			{
				if (!g_pNpcArray[i]->IsExist() || g_pNpcArray[i]->m_Kind != kind_player) continue;
				if (bAcceptList ? !IsInAcceptList(g_pNpcArray[i]->Name) : IsInNoPTList(g_pNpcArray[i]->Name)) continue;
				if (AcceptInvite(g_pNpcArray[i]->Name))
					return;
			}
			return;
		}
		if (IAM_THE_CAPTAIN && g_pTeam->m_nMemNum == 0)	// Chi con minh trong PT -> roi de nhan loi moi
		{
			g_pCoreShell->TeamOperation(TEAM_LEAVE, 0, 0);
			return;
		}
		if (!IAM_THE_CAPTAIN)
			return;
		// Lo dang la chu PT (chu cu roi PT): van gom thanh vien theo o Moi
	}
	else
	{
		if (!IAM_IN_A_PARTY)
		{
			//QTRACE("PT: chua co PT -> tu lap PT (Luon lam chu PT)");
			g_pCoreShell->TeamOperation(TEAM_CREATE, 0, 0);
			return;
		}
		if (!IAM_THE_CAPTAIN)
		{
			//QTRACE("PT: dang la thanh vien PT nguoi khac -> roi PT de tu lam chu");
			g_pCoreShell->TeamOperation(TEAM_LEAVE, 0, 0);	// Roi PT nguoi khac de tu lam chu
			return;
		}
	}

	if (g_pTeam->m_nMemNum < 7)
	{
		int nInvited = 0;
		for (int i = 1; i < MAX_NPC; i++)
		{
			if (!g_pNpcArray[i]->IsExist() || g_pNpcArray[i]->m_Kind != kind_player) continue;
			if (IsInSamePT(g_pNpcArray[i]->m_dwID)) continue;
			// Nguoi trong danh sach: chu PT luon moi (ke ca o Moi = "Khong moi tat ca", giong che do cu).
			// Nguoi ngoai danh sach: chi moi khi o Moi = Luon moi / Moi khi DT (dang chien dau, khong nam trong NoPTNames)
			BOOL bInList = IsInAcceptList(g_pNpcArray[i]->Name);
			BOOL bInvite = bInList || (!bInviteList && m_bPTAllYS && P_PLAYER->m_FightMode && !IsInNoPTList(g_pNpcArray[i]->Name));
			if (!bInvite) continue;
			nInvited++;
			//QTRACE("PT: chu PT moi %s (#%d, id %u, trong d.sach: %d)", g_pNpcArray[i]->Name, i, g_pNpcArray[i]->m_dwID, bInList);
			KUiPlayerItem thePlayer;
			memset(&thePlayer, 0, sizeof(KUiPlayerItem));
			strcpy(thePlayer.Name, g_pNpcArray[i]->Name);
			thePlayer.uId = g_pNpcArray[i]->m_dwID;
			if (bInvite)
			{
				g_pCoreShell->TeamOperation(TEAM_INVITE, (int)& thePlayer, 0);
				PartyChat(PT_CHAT_INVITE, thePlayer.Name);	// Chat gan khi moi
			}
			if (bInList)	// Nguoi trong danh sach xin vao PT -> dong y
				g_pCoreShell->TeamOperation(TEAM_JOIN_RESPONSE, (int)& thePlayer, 1);
		}
		//if (!nInvited)
			//QTRACE("PT: chu PT, khong co ai de moi (d.sach %d ten, o Moi=%d, chien dau=%d)", CountListNames(), m_nPTAll, P_PLAYER->m_FightMode);
	}
	if (m_bAutoKick && P_PLAYER->m_FightMode && g_pTeam->m_nMemNum > 0)
		CheckAbsentMem();
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

	// Quet het cac heap / tin: truoc day gap tin khong phai loi moi (vd "... roi to doi") la break
	// => loi moi moi nam sau tin do khong bao gio duoc nhan.
	// Duyet tu tin moi nhat ve cu va xoa tin loi moi da tra loi: neu de lai, lan sau Auto tra loi
	// lai loi moi cu (du lieu het han) thay vi loi moi moi => khong vao lai PT.
	for (int i = 0; i < 6; i++)
	{
		if (pSysMsg->m_MsgHeap[i].nNumValid <= 0 || !pSysMsg->m_MsgHeap[i].pMsgList)
			continue;

		for (int j = pSysMsg->m_MsgHeap[i].nNumValid - 1; j >= 0; j--)
		{
			KSystemMessage* pMsg = pSysMsg->m_MsgHeap[i].pMsgList[j];
			if (!pMsg)
				continue;

			char* pInviteMsg = strstr(pMsg->szMessage, TV(L"mời tổ đội của Bạn!"));
			if (!pInviteMsg)
				continue;

			if (!strcmp(pMsg->mInviter.Name, szInviter) && pMsg->mInviter.nIndex > 0)
			{
				QTRACE("PT: nhan loi moi PT cua %s (heap %d, tin %d/%d)", szInviter, i, j, pSysMsg->m_MsgHeap[i].nNumValid);
				g_pCoreShell->TeamOperation(TEAM_INVITE_RESPONSE, (int)& pMsg->mInviter, 1);
				PartyChat(PT_CHAT_ACCEPT, szInviter);	// Chat gan khi nhan loi moi
				pSysMsg->DeleteMsgInHeap(i, j);
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
			PartyChat(PT_CHAT_INVITE, thePlayer.Name);	// Chat gan khi moi
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
					sprintf(szScript, TV(L"Chat('CH_TEAM','Khai trừ [%s] ra khỏi nhóm vì vắng mặt quá lâu!!!')"), g_pTeam->m_szMemName[i]);
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
