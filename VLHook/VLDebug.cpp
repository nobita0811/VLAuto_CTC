#include "stdafx.h"
#include "../GameDef.h"
#include "GameBase.h"
#include "JxCore.h"
#include "S3Client.h"
#include "VLClasses.h"
#include "VLDebug.h"

// Kiem tra tung ham game theo lenh tu cua so DEBUG cua VLAuto.
// Chay trong NewWndProc (luong cua Game) => goi ham game an toan nhu Breathe().
// Ket qua tra ve bang WM_COPYDATA (DEBUG_REPLY), chuoi theo ma TCVN3.

extern BOOL g_SysPause;

DEBUG_REPLY		g_DbgReply;
static int			s_nTextLen;

void DbgOut(const char *szFormat, ...)
{
	int nRemain = DEBUG_TEXT_SIZE - 1 - s_nTextLen;
	if (nRemain <= 0) return;

	va_list vlist;
	va_start(vlist, szFormat);
	int nLen = _vsnprintf(g_DbgReply.szText + s_nTextLen, nRemain, szFormat, vlist);
	va_end(vlist);

	if (nLen < 0 || nLen > nRemain) nLen = nRemain;	// bi cat bot
	s_nTextLen += nLen;
	g_DbgReply.szText[s_nTextLen] = 0;
}

const char* DbgDoingName(int nDoing)
{
	switch (nDoing)
	{
	case do_none: return "none";
	case do_stand: return "stand";
	case do_walk: return "walk";
	case do_run: return "run";
	case do_jump: return "jump";
	case do_skill: return "skill";
	case do_magic: return "magic";
	case do_attack: return "attack";
	case do_sit: return "sit";
	case do_hurt: return "hurt";
	case do_death: return "death";
	case do_defense: return "defense";
	case do_idle: return "idle";
	case do_revive: return "revive";
	}
	return "?";
}

void DbgOutPlayerPos(const char *szLabel)
{
	int nX, nY;
	P_PLAYER->GetMapPos(&nX, &nY);
	DbgOut("%s: (%d, %d)  toa do game (%d/%d)  Doing=%s  SendMoveFrames=%d\n",
		szLabel, nX, nY, nX >> 8, nY >> 9, DbgDoingName(P_PLAYER->m_Doing), g_pCurPlayer->m_nSendMoveFrames);
}

static void OutFlag()
{
	int nFlagX = *((int*)FLAG_X_OFFST);
	int nFlagY = *((int*)FLAG_Y_OFFST);
	DbgOut("FLAG_X/Y_OFFST = %d, %d", nFlagX, nFlagY);
	if (nFlagX > 0 && nFlagY > 0)
		DbgOut("  => cam co (%d, %d)  toa do game (%d/%d)\n", nFlagX << 4, nFlagY << 5, (nFlagX << 4) >> 8, (nFlagY << 5) >> 9);
	else
		DbgOut("  => khong co co\n");
}

// Tra ve FALSE neu Player chua san sang (chua co du lieu game)
BOOL DbgCheckPlayer()
{
	if (!g_pCurPlayer || !g_pNpcArray[0])
	{
		DbgOut("LOI: g_pCurPlayer = NULL - chua nhan WM_HOOK_EXDATA (InitGameData)\n");
		return FALSE;
	}
	int nIndex = g_pCurPlayer->m_nIndex;
	if (nIndex <= 0 || nIndex >= MAX_NPC)
	{
		DbgOut("LOI: g_pCurPlayer->m_nIndex = %d (ngoai 1..%d) - chua vao game hoac sai offset\n", nIndex, MAX_NPC - 1);
		return FALSE;
	}
	return TRUE;
}

static void MoveStatus()
{
	DWORD dwPlrBase = GetPtrAddress(PLR_BASE_ADD);
	DbgOut("PLR_BASE_ADD -> 0x%08X  (+PLAYER_OFFSET = 0x%08X)\n", dwPlrBase, dwPlrBase + PLAYER_OFFSET);
	DbgOut("g_pCurPlayer = 0x%08X%s\n", (DWORD)g_pCurPlayer,
		((DWORD)g_pCurPlayer == dwPlrBase + PLAYER_OFFSET) ? "" : "  <== KHAC dia chi hien tai!");
	if (!DbgCheckPlayer()) return;

	KNpc *pPlayer = P_PLAYER;
	DbgOut("m_nIndex = %d  P_PLAYER = 0x%08X  m_dwID = %u  m_Index = %d\n",
		g_pCurPlayer->m_nIndex, (DWORD)pPlayer, pPlayer->m_dwID, pPlayer->m_Index);
	DbgOut("Name = %s  Level = %d  Life = %d\n", pPlayer->Name, pPlayer->m_Level, pPlayer->m_CurrentLife);
	DbgOutPlayerPos("GetMapPos");
	DbgOut("m_FightMode = %d  m_bRideHorse = %d  HAVE_HORSE = %d  PEOPLE_IDX = %d\n",
		pPlayer->m_FightMode, pPlayer->m_bRideHorse, HAVE_HORSE ? 1 : 0, pPlayer->m_nPeopleIdx);
	DbgOut("MapID = %d  MapName = %s\n", *g_pMapID, g_szMapName);

	DbgOut("\n--- Dieu kien CVLMove::Breathe ---\n");
	DbgOut("g_NetworkOffline = %d  g_SysPause = %d  theMain.m_bPause = %d\n", g_NetworkOffline, g_SysPause, theMain.m_bPause);
	DbgOut("theAttack: m_bAutoHit = %d  m_nEnemyIdx = %d  m_nBossWait = %d\n", theAttack.m_bAutoHit, theAttack.m_nEnemyIdx, theAttack.m_nBossWait);
	DbgOut("theMove: m_bFollow = %d  Leader = '%s'  m_bAroundPoint = %d (%d, %d R=%d)\n",
		theMove.m_bFollow, theMove.m_szLeader, theMove.m_bAroundPoint, theMove.m_nPointX, theMove.m_nPointY, theMove.m_nPointRange);
	DbgOut("         m_bRouteMove = %d  m_bMoveByMap = %d  m_bMoveByMapYS = %d  m_bFindingWay = %d\n",
		theMove.m_bRouteMove, theMove.m_bMoveByMap, theMove.m_bMoveByMapYS, theMove.m_bFindingWay);
	DbgOut("         m_nStatus = %d  m_nCurNode = %d  m_nPreNode = %d  m_nLeaderIdx = %d\n",
		theMove.m_nStatus, theMove.m_nCurNode, theMove.m_nPreNode, theMove.m_nLeaderIdx);
	DbgOut("         nLastMapID = %d  m_nMapPoints = %d  m_nPathPoints = %d  m_nCurPoint = %d  LagCount = %d/%d\n",
		theMove.nLastMapID, theMove.m_nMapPoints, theMove.m_nPathPoints, theMove.m_nCurPoint, theMove.m_nLagCount, theMove.m_nTotalLagCount);
	DbgOut("theMap.m_nTrainMapID = %d  theQuest.m_nYeSouMapID = %d\n", theMap.m_nTrainMapID, theQuest.m_nYeSouMapID);

	// Cung thu tu return voi CVLMove::Breathe
	const char *szStop = NULL;
	if (g_SysPause) szStop = "g_SysPause (Ctrl+DblClick tam ngung)";
	else if (g_NetworkOffline) szStop = "g_NetworkOffline != 0 (m_SyncSignal khong cap nhat?)";
	else if (theMain.m_bPause) szStop = "theMain.m_bPause (dang click chuot)";
	else if (theAttack.m_nEnemyIdx) szStop = "theAttack.m_nEnemyIdx != 0 (dang PK)";
	else if (!pPlayer->m_FightMode) szStop = "m_FightMode = 0 (dang trong thanh)";
	else if (!theMove.m_bMoveByMap && !theMove.m_bAroundPoint && !theMove.m_bRouteMove && !theMove.m_bFollow && !theMap.m_nTrainMapID)
		szStop = "Khong bat che do di chuyen nao";
	else if (!theAttack.m_bAutoHit && !theMove.m_bFollow) szStop = "theAttack.m_bAutoHit = 0 (chua bat Tu danh - Ctrl+A)";
	if (szStop)
		DbgOut("=> Breathe DUNG tai: %s\n", szStop);
	else
		DbgOut("=> Breathe khong bi chan o cac dieu kien dau\n");
	g_DbgReply.bOK = TRUE;
}

static void MoveTo(int nX, int nY, BOOL bRawNpc)
{
	if (!DbgCheckPlayer()) return;
	if (nX <= 0 || nY <= 0)
	{
		DbgOut("LOI: toa do khong hop le (%d, %d)\n", nX, nY);
		return;
	}
	int nPlayerX, nPlayerY;
	P_PLAYER->GetMapPos(&nPlayerX, &nPlayerY);
	int dZ = __distance(nPlayerX, nPlayerY, nX, nY);
	DbgOutPlayerPos("Truoc");
	DbgOut("Dich: (%d, %d)  toa do game (%d/%d)  khoang cach = %d\n", nX, nY, nX >> 8, nY >> 9, dZ);

	if (bRawNpc)
	{
		DbgOut("Goi P_PLAYER->MoveTo -> MVT_FUNC_ADD 0x%08X (this = 0x%08X)\n", MVT_FUNC_ADD, (DWORD)P_PLAYER);
		P_PLAYER->MoveTo(nX, nY);
	}
	else
	{
		if (g_pCurPlayer->m_nSendMoveFrames > MAX_PLAYER_MOVE_FRAME)
		{
			if (dZ > MAX_MOVE_DISTANCE)
				DbgOut("dZ > MAX_MOVE_DISTANCE(%d) => se cat ngan quang duong\n", MAX_MOVE_DISTANCE);
		}
		else
			DbgOut("CANH BAO: m_nSendMoveFrames = %d <= %d => KPlayer::MoveTo se BO QUA lenh!\n",
				g_pCurPlayer->m_nSendMoveFrames, MAX_PLAYER_MOVE_FRAME);
		g_pCurPlayer->MoveTo(nX, nY);
	}
	DbgOutPlayerPos("Sau");
	DbgOut("(Vi tri se duoc doc lai sau 1 giay)\n");
	g_DbgReply.bOK = TRUE;
}

// KPlayer::GoTo: game tu tim duong toi (X/32, Y/32) - thiscall GOTO_FUNC_ADD, ecx = GOTO_BASE_ADD
static void GoTo(int nX, int nY)
{
	if (!DbgCheckPlayer()) return;
	if (nX <= 0 || nY <= 0)
	{
		DbgOut("LOI: toa do khong hop le (%d, %d) - lay o nut 'P_PLAYER->GetMapPos' hoac 'Doc cam co'\n", nX, nY);
		return;
	}
	int nPlayerX, nPlayerY;
	P_PLAYER->GetMapPos(&nPlayerX, &nPlayerY);
	DbgOutPlayerPos("Truoc");
	DbgOut("Dich: (%d, %d)  toa do game (%d/%d)  khoang cach = %d\n", nX, nY, nX >> 8, nY >> 9, __distance(nPlayerX, nPlayerY, nX, nY));
	const BYTE *pCode = (const BYTE*)GOTO_FUNC_ADD;
	DbgOut("GoTo(%d, %d) -> GOTO_FUNC_ADD 0x%08X (ma dau: %02X %02X %02X %02X %02X %02X)  ecx = GOTO_BASE_ADD 0x%08X ([0] = 0x%08X)\n",
		nX / 32, nY / 32, GOTO_FUNC_ADD, pCode[0], pCode[1], pCode[2], pCode[3], pCode[4], pCode[5],
		GOTO_BASE_ADD, *(const DWORD*)GOTO_BASE_ADD);
	g_pCurPlayer->GoTo(nX, nY);
	DbgOutPlayerPos("Sau");
	DbgOut("(Vi tri se duoc doc lai sau 1 giay; game tu tim duong nen co the di vong)\n");
	g_DbgReply.bOK = TRUE;
}

static void NpcList(int nMaxDist)
{
	if (!DbgCheckPlayer()) return;
	int nPlayerX, nPlayerY, nNpcX, nNpcY;
	P_PLAYER->GetMapPos(&nPlayerX, &nPlayerY);
	DbgOut("Idx  | Kind | Lv  | Doing  | Toa do         | dZ    | Ten\n");
	int nCount = 0;
	for (int i = 1; i < MAX_NPC; i++)
	{
		KNpc *pNpc = g_pNpcArray[i];
		if (!pNpc->IsExist()) continue;
		pNpc->GetMapPos(&nNpcX, &nNpcY);
		int dZ = __distance(nPlayerX, nPlayerY, nNpcX, nNpcY);
		if (nMaxDist > 0 && dZ > nMaxDist) continue;
		nCount++;
		DbgOut("%-4d | %-4d | %-3d | %-6s | %6d,%-7d | %-5d | %s%s\n", i, pNpc->m_Kind, pNpc->m_Level, DbgDoingName(pNpc->m_Doing),
			nNpcX, nNpcY, dZ, pNpc->Name, (i == g_pCurPlayer->m_nIndex) ? "  <== Player" : "");
	}
	DbgOut("Tong: %d NPC%s\n", nCount, nMaxDist > 0 ? " (trong pham vi)" : "");
	g_DbgReply.nResult[0] = nCount;
	g_DbgReply.bOK = TRUE;
}

static void FindNpc(const char *szName)
{
	if (!DbgCheckPlayer()) return;
	if (!szName[0])
	{
		DbgOut("LOI: chua nhap ten (o 'Chuoi')\n");
		return;
	}
	int nPlayerX, nPlayerY, nNpcX, nNpcY;
	P_PLAYER->GetMapPos(&nPlayerX, &nPlayerY);
	for (int i = 1; i < MAX_NPC; i++)
	{
		KNpc *pNpc = g_pNpcArray[i];
		if (!pNpc->IsExist()) continue;
		if (strcmp(pNpc->Name, szName)) continue;
		pNpc->GetMapPos(&nNpcX, &nNpcY);
		int dZ = __distance(nPlayerX, nPlayerY, nNpcX, nNpcY);
		DbgOut("Tim thay '%s': idx = %d  (%d, %d)  dZ = %d  Kind = %d  Doing = %s\n",
			szName, i, nNpcX, nNpcY, dZ, pNpc->m_Kind, DbgDoingName(pNpc->m_Doing));
		DbgOut("GetLeaderChar: %s (dZ %s 1200)\n", dZ < 1200 ? "SE theo" : "KHONG theo - qua xa", dZ < 1200 ? "<" : ">=");
		g_DbgReply.nResult[0] = i;
		g_DbgReply.nResult[1] = nNpcX;
		g_DbgReply.nResult[2] = nNpcY;
		g_DbgReply.bOK = TRUE;
		return;
	}
	DbgOut("Khong tim thay NPC ten '%s'\n", szName);
}

static void TeamInfo()
{
	if (!DbgCheckPlayer()) return;
	DbgOut("g_pTeam = 0x%08X  IAM_IN_A_PARTY = %d  IAM_THE_CAPTAIN = %d\n",
		(DWORD)g_pTeam, IAM_IN_A_PARTY, (IAM_THE_CAPTAIN) ? 1 : 0);
	DbgOut("m_nMemNum = %d\n", g_pTeam->m_nMemNum);
	for (int i = 0; i < MAX_TEAM_MEMBER; i++)
	{
		if (!g_pTeam->m_szMemName[i][0]) continue;
		DbgOut("  [%d] %s  (ID %d, Lv %d)\n", i, g_pTeam->m_szMemName[i], g_pTeam->m_nMember[i], g_pTeam->m_nMemLevel[i]);
	}
	g_DbgReply.bOK = TRUE;
}

static void MsgSel2Info()
{
	KUiMsgSel2 *pMsgSel2 = (KUiMsgSel2*)GetPtrAddress(MSG2_BASE_ADD);
	DbgOut("MSG2_BASE_ADD -> 0x%08X\n", (DWORD)pMsgSel2);
	if (!pMsgSel2)
	{
		DbgOut("KUiMsgSel2 chua duoc tao (NULL)\n");
		g_DbgReply.bOK = TRUE;
		return;
	}
	DbgOut("IsVisible = %d  m_Style = 0x%08X\n", pMsgSel2->IsVisible() ? 1 : 0, pMsgSel2->m_Style);
	if (pMsgSel2->GetQuestion()[0]) DbgOut("  Loi thoai: %.200s\n", pMsgSel2->GetQuestionText());
	DbgOut("m_nNumMessage = %d\n", pMsgSel2->m_nNumMessage);
	for (int i = 0; i < pMsgSel2->m_nNumMessage && i < 16; i++)
	{
		KOneMsgInfo *pMsg = pMsgSel2->m_pMessages ? pMsgSel2->m_pMessages[i] : NULL;
		if (pMsg) DbgOut("  Lua chon[%d]: %.*s\n", i, pMsg->nLen > 0 && pMsg->nLen < 256 ? pMsg->nLen : 0, pMsg->Msg);
	}
	g_DbgReply.bOK = TRUE;
}

static void MapInfo()
{
	DbgOut("*g_pMapID = %d  g_szMapName = %s\n", *g_pMapID, g_szMapName);
	DbgOut("theMove.nLastMapID = %d  m_nMapPoints = %d  m_nPathPoints = %d  m_nPCKPoints = %d\n",
		theMove.nLastMapID, theMove.m_nMapPoints, theMove.m_nPathPoints, theMove.m_nPCKPoints);
	char szMapID[16], szMapFile[64] = "", szMapIniFile[MAX_PATH];
	sprintf(szMapID, "%d", *g_pMapID);
	sprintf(szMapIniFile, "%s\\Maps\\MapMenu.ini", theMain.m_szCurFolder);
	GetPrivateProfileString("Map_File", szMapID, "", szMapFile, sizeof(szMapFile), szMapIniFile);
	DbgOut("MapMenu.ini [Map_File] %s = '%s'%s\n", szMapID, szMapFile, szMapFile[0] ? "" : "  (khong co file MAP cho map nay)");
	for (int i = 0; i < theMove.m_nMapPoints && i < 10; i++)
		DbgOut("  MapPoints[%d] = (%d, %d)\n", i, theMove.MapPoints[i].x, theMove.MapPoints[i].y);
	DbgOut("  m_nNodesMapID = %d  (map hien tai %d)\n", theMove.m_nNodesMapID, *g_pMapID);
	for (int i = 0; i < MAX_NODES && theMove.m_Nodes[i].x; i++)
		DbgOut("  m_Nodes[%d] = (%d, %d)%s\n", i, theMove.m_Nodes[i].x, theMove.m_Nodes[i].y, i == theMove.m_nCurNode ? "  <== CurNode" : "");
	g_DbgReply.bOK = TRUE;
}

static void ExecDebugCmd(const DEBUG_PARAM *pParam)
{
	const int *P = pParam->nParam;
	switch (pParam->nCmd)
	{
	case dbg_mv_status:
		MoveStatus();
		break;
	case dbg_mv_getmappos:
		if (!DbgCheckPlayer()) break;
		P_PLAYER->GetMapPos(&g_DbgReply.nResult[0], &g_DbgReply.nResult[1]);
		DbgOutPlayerPos("P_PLAYER->GetMapPos");
		g_DbgReply.bOK = TRUE;
		break;
	case dbg_mv_moveto:
		MoveTo(P[0], P[1], FALSE);
		break;
	case dbg_mv_npcmoveto:
		MoveTo(P[0], P[1], TRUE);
		break;
	case dbg_mv_movetonpc:
		if (!DbgCheckPlayer()) break;
		if (P[2] <= 0 || P[2] >= MAX_NPC || !g_pNpcArray[P[2]]->IsExist())
		{
			DbgOut("LOI: NPC idx = %d khong ton tai (nhap o 'Idx/So')\n", P[2]);
			break;
		}
		DbgOutPlayerPos("Truoc");
		DbgOut("Den NPC [%d] %s\n", P[2], g_pNpcArray[P[2]]->Name);
		g_pCurPlayer->MoveTo(g_pNpcArray[P[2]]);
		DbgOutPlayerPos("Sau");
		g_DbgReply.bOK = TRUE;
		break;
	case dbg_mv_dosit:
		if (!DbgCheckPlayer()) break;
		DbgOutPlayerPos("Truoc");
		if (g_pCurPlayer->m_nSendMoveFrames < MAX_PLAYER_MOVE_FRAME)
			DbgOut("CANH BAO: m_nSendMoveFrames < %d => DoSit se BO QUA\n", MAX_PLAYER_MOVE_FRAME);
		g_pCurPlayer->DoSit();
		DbgOutPlayerPos("Sau");
		g_DbgReply.bOK = TRUE;
		break;
	case dbg_mv_horse:
		if (!DbgCheckPlayer()) break;
		DbgOut("HAVE_HORSE = %d  m_bRideHorse truoc = %d\n", HAVE_HORSE ? 1 : 0, P_PLAYER->m_bRideHorse);
		g_DbgReply.nResult[0] = ExecuteScript(SCK_SHORTCUT_HORSE);
		DbgOut("ExecuteScript(\"%s\") = %d  (EXS_FUNC_ADD 0x%08X)\n", SCK_SHORTCUT_HORSE, g_DbgReply.nResult[0], EXS_FUNC_ADD);
		DbgOut("m_bRideHorse sau = %d (game cap nhat sau vai frame)\n", P_PLAYER->m_bRideHorse);
		g_DbgReply.bOK = TRUE;
		break;
	case dbg_mv_gettaskval:
		if (!DbgCheckPlayer()) break;
		g_DbgReply.nResult[0] = g_pCurPlayer->GetTaskVal(P[2]);
		DbgOut("g_pCurPlayer->GetTaskVal(%d) = %d (0x%08X)\n", P[2], g_DbgReply.nResult[0], g_DbgReply.nResult[0]);
		g_DbgReply.bOK = TRUE;
		break;
	case dbg_mv_npclist:
		NpcList(P[2]);
		break;
	case dbg_mv_findnpc:
		FindNpc(pParam->szParam);
		break;
	case dbg_mv_setpeople:
		if (!DbgCheckPlayer()) break;
		DbgOut("PEOPLE_IDX truoc = %d\n", PEOPLE_IDX);
		theAttack.SetPeopleIdx(P[2]);
		DbgOut("theAttack.SetPeopleIdx(%d) -> PEOPLE_IDX = %d\n", P[2], PEOPLE_IDX);
		g_DbgReply.bOK = TRUE;
		break;
	case dbg_mv_getflag:
		OutFlag();
		g_DbgReply.nResult[0] = *((int*)FLAG_X_OFFST) << 4;
		g_DbgReply.nResult[1] = *((int*)FLAG_Y_OFFST) << 5;
		g_DbgReply.bOK = TRUE;
		break;
	case dbg_mv_setflag:
		{
			if (!DbgCheckPlayer()) break;
			// SetMapFlag(dx, dy) tinh tu vi tri Player: dich = (X + dx<<4, Y + dy<<5)
			int nPlayerX, nPlayerY;
			P_PLAYER->GetMapPos(&nPlayerX, &nPlayerY);
			int nDx = (P[0] > 0) ? (P[0] - nPlayerX) >> 4 : 0;
			int nDy = (P[1] > 0) ? (P[1] - nPlayerY) >> 5 : 0;
			KUiMiniMap *pMiniMap = (KUiMiniMap*)(MMP_BASE_ADD);
			DbgOut("Dich (%d, %d) => KUiMiniMap::SetMapFlag(%d, %d)  SMF_FUNC_ADD 0x%08X\n", P[0], P[1], nDx, nDy, SMF_FUNC_ADD);
			pMiniMap->SetMapFlag(nDx, nDy);
			OutFlag();
			g_DbgReply.bOK = TRUE;
		}
		break;
	case dbg_mv_team:
		TeamInfo();
		break;
	case dbg_mv_teamleave:
		if (!DbgCheckPlayer()) break;
		DbgOut("IAM_IN_A_PARTY truoc = %d\n", IAM_IN_A_PARTY);
		g_DbgReply.nResult[0] = g_pCoreShell->TeamOperation(TEAM_LEAVE, 0, 0);
		DbgOut("g_pCoreShell(0x%08X)->TeamOperation(TEAM_LEAVE) = %d\n", (DWORD)g_pCoreShell, g_DbgReply.nResult[0]);
		g_DbgReply.bOK = TRUE;
		break;
	case dbg_mv_msgsel2:
		MsgSel2Info();
		break;
	case dbg_mv_msgsel2click:
		{
			KUiMsgSel2 *pMsgSel2 = (KUiMsgSel2*)GetPtrAddress(MSG2_BASE_ADD);
			if (!pMsgSel2 || !pMsgSel2->IsVisible())
			{
				DbgOut("KUiMsgSel2 khong hien thi - bo qua OnClickMsg\n");
				break;
			}
			pMsgSel2->OnClickMsg(P[2]);
			DbgOut("KUiMsgSel2::OnClickMsg(%d)  OMS2_FUNC_ADD 0x%08X  => IsVisible sau = %d\n",
				P[2], OMS2_FUNC_ADD, pMsgSel2->IsVisible() ? 1 : 0);
			g_DbgReply.bOK = TRUE;
		}
		break;
	case dbg_mv_info2:
		DbgOut("g_UiInformation2 = 0x%08X  IsVisible = %d\n", (DWORD)g_UiInformation2, g_UiInformation2->IsVisible() ? 1 : 0);
		if (g_UiInformation2->IsVisible())
		{
			DbgOut("Text: %.128s\n", g_UiInformation2->m_szInfoText);
			g_UiInformation2->Hide();
			DbgOut("Hide() => IsVisible = %d\n", g_UiInformation2->IsVisible() ? 1 : 0);
		}
		g_DbgReply.bOK = TRUE;
		break;
	case dbg_mv_sysmsg:
		SystemMessage(pParam->szParam[0] ? pParam->szParam : "VLAuto DEBUG: SystemMessage OK");
		DbgOut("SystemMessage -> SMA_FUNC_ADD 0x%08X: xem khung chat trong game\n", SMA_FUNC_ADD);
		g_DbgReply.bOK = TRUE;
		break;
	case dbg_mv_savepoint:
		if (!DbgCheckPlayer()) break;
		if (!theMove.m_bAroundPoint && !theMove.m_bRouteMove)
			DbgOut("CANH BAO: chua bat 'Quanh diem' hoac 'Theo toa do' => SavePoint khong lam gi\n");
		theMove.SavePoint();
		DbgOut("theMove.SavePoint(): Point = (%d, %d)  CurNode = %d\n", theMove.m_nPointX, theMove.m_nPointY, theMove.m_nCurNode);
		g_DbgReply.bOK = TRUE;
		break;
	case dbg_mv_mapinfo:
		MapInfo();
		break;
	case dbg_mv_goto:
		GoTo(P[0], P[1]);
		break;
	default:
		if (DebugMapCmd(pParam)) break;
		if (DebugCollectCmd(pParam)) break;
		if (DebugPathCmd(pParam)) break;
		if (DebugAttackCmd(pParam)) break;
		if (DebugRecoverCmd(pParam)) break;
		if (DebugLowCfgCmd(pParam)) break;
		DbgOut("Lenh khong ho tro: %d\n", pParam->nCmd);
		break;
	}
}

static int RunDebugCmd(const DEBUG_PARAM *pParam)
{
	__try
	{
		ExecDebugCmd(pParam);
	}
	__except (EXCEPTION_EXECUTE_HANDLER)
	{
		return GetExceptionCode();
	}
	return 0;
}

void OnDebugCommand(const DEBUG_PARAM *pParam)
{
	memset(&g_DbgReply, 0, sizeof(g_DbgReply));
	s_nTextLen = 0;
	g_DbgReply.nCmd = pParam->nCmd;
	g_DbgReply.nSeq = pParam->nSeq;

	if (pParam->cbSize != sizeof(DEBUG_PARAM))
		DbgOut("LOI: DEBUG_PARAM khac phien ban (cbSize %d != %d) - build lai ca VLAuto & VLHook\n", pParam->cbSize, sizeof(DEBUG_PARAM));
	else
	{
		int nCode = RunDebugCmd(pParam);
		if (nCode)
		{
			g_DbgReply.bOK = FALSE;
			DbgOut("\n*** EXCEPTION 0x%08X khi chay lenh - sai dia chi/offset! ***\n", nCode);
		}
	}

	if (!pParam->hWndReply || !_IsWindow(pParam->hWndReply))
		return;

	COPYDATASTRUCT cds;
	cds.dwData = DEBUG_COPYDATA_ID;
	cds.cbData = offsetof(DEBUG_REPLY, szText) + s_nTextLen + 1;
	cds.lpData = &g_DbgReply;
	_SendMessage(pParam->hWndReply, WM_COPYDATA, (WPARAM)theMain.m_hWndGame, (LPARAM)&cds);
}
