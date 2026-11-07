#include "stdafx.h"
#include "../GameDef.h"
#include "DTextFile.h"
#include "GameBase.h"
#include "JxCore.h"
#include "S3Client.h"
#include "VLClasses.h"
#include "VLDebug.h"
#include "VLObstacle.h"

static int WalkDist(int px, int py, int tx, int ty, int dZ);

// ===== Ket lag: Khinh cong (Tab Phuc hoi "Dung khinh cong khi chay bi ket") =====
// Phai xuong ngua moi dung duoc khinh cong: dang cuoi ngua thi xuong -> nhay -> len ngua lai
#define LAG_JUMP_SKILLID	210
#define LAG_JUMP_RANGE		300		// Tam nhay khi khong doc duoc AttackRadius cua skill
static int s_nJumpStep = 0;			// 0 = khong nhay, 1 = cho xuong ngua, 2 = ra skill, 3 = cho tiep dat
static int s_nJumpWait = 0;
static int s_nJumpTry = 0;
static BOOL s_bJumpRemount = FALSE;
static int s_nJumpX = 0, s_nJumpY = 0;

static BOOL HaveJumpSkill()
{
	for (int i = 0; i < MAX_NPCSKILL; i++)
		if (P_PLAYER->m_Skills[i].SkillId == LAG_JUMP_SKILLID && P_PLAYER->m_Skills[i].SkillLevel > 0) return TRUE;
	return FALSE;
}

// Bat dau nhay tu (nPlayerX, nPlayerY) ve phia (nDestX, nDestY), cat theo tam skill. FALSE = khong nhay duoc
static BOOL LagJumpStart(int nPlayerX, int nPlayerY, int nDestX, int nDestY)
{
	if ((!nDestX && !nDestY) || !HaveJumpSkill()) return FALSE;
	int dZ = __distance(nPlayerX, nPlayerY, nDestX, nDestY);
	if (dZ <= 0) return FALSE;
	KSkill* pSkill = (KSkill*)(SKL_BASE_ADD + (LAG_JUMP_SKILLID - 1) * SKL_DATA_SIZE);
	int nRange = (pSkill->m_nSkillRange > 0) ? pSkill->m_nSkillRange * 9 / 10 : LAG_JUMP_RANGE;
	if (dZ > nRange)
	{
		nDestX = nPlayerX + (nDestX - nPlayerX) * nRange / dZ;
		nDestY = nPlayerY + (nDestY - nPlayerY) * nRange / dZ;
	}
	s_nJumpX = nDestX;
	s_nJumpY = nDestY;
	s_bJumpRemount = P_PLAYER->m_bRideHorse;
	s_nJumpWait = 0;
	s_nJumpTry = 0;
	s_nJumpStep = 1;
	return TRUE;
}

// Goi moi nhip CVLMove::Breathe. TRUE = dang nhay (dung di chuyen)
static BOOL LagJumpBreathe()
{
	if (!s_nJumpStep) return FALSE;
	if (s_nJumpWait > 0) { s_nJumpWait--; return TRUE; }
	switch (s_nJumpStep)
	{
	case 1:	// Xuong ngua
		if (P_PLAYER->m_bRideHorse)
		{
			if (++s_nJumpTry > 3) { s_nJumpStep = 0; return FALSE; }	// Khong xuong duoc ngua: bo
			ExecuteScript(SCK_SHORTCUT_HORSE);
			s_nJumpWait = 4;
			return TRUE;
		}
		s_nJumpStep = 2;
		// fall through
	case 2:	// Nhay
		P_PLAYER->SendCommand(do_skill, LAG_JUMP_SKILLID, s_nJumpX, s_nJumpY);
		theAttack.SendVLCmdSkill(LAG_JUMP_SKILLID, s_nJumpX, s_nJumpY);
		s_nJumpStep = 3;
		s_nJumpWait = 5;
		return TRUE;
	default:	// Tiep dat: len ngua lai
		if (s_bJumpRemount && !P_PLAYER->m_bRideHorse && HAVE_HORSE)
			ExecuteScript(SCK_SHORTCUT_HORSE);
		s_nJumpStep = 0;
		theMove.m_nLagCount = 0;
		theMove.m_nLastdZ = MAX_INTNUM;
		return FALSE;
	}
}

//================================Class CVLMove================================
CVLMove::CVLMove(void)
{
	m_nPreNode = -1;
	m_nCurNode = -1;
	m_nStatus = status_idle;
	m_nLeaderIdx = 0;
	memset(m_Nodes, 0, sizeof(m_Nodes));
	memset(m_GoNodes, 0, sizeof(m_GoNodes));
	m_nGoNodesMapID = 0;
	m_nGoNode = -1;
	m_nGoDone = -1;
	m_bFindingWay = TRUE;
	m_bGoTrain = 0;
	m_nGoTrainMap = 0;
	m_dwGoTrainTick = 0;
	nLastMapID = 0;
	m_nMapPoints = 0;
	m_nPathPoints = 0;
	m_nCurPoint = -1;
	m_nMaxSide = 0;
	m_nLastdZ = MAX_INTNUM;
	m_nLagCount = 0;
	m_nTotalLagCount = 0;
	m_bMoveWithPK = 0;
}

void CVLMove::Breathe()
{
	int nPlayerX, nPlayerY;
	int nDestX, nDestY, dZ;
	if (g_NetworkOffline) return;
	if (theMain.m_bPause || theAttack.m_nEnemyIdx) return;
	if (PathTravelActive()) return;	// Dang tu di theo MapPath
	if (DebugQuestHoldMove()) return;	// DEBUG > Nhiem vu: Buoc 3 dung tai map / Buoc 4 tu di vong theo MapPath

	if ((*g_pMapID) >= city_phongky1 && (*g_pMapID) <= city_baokho3)
	{	// Map PCK
		if (theQuest.m_bAutoDoQuest && theQuest.m_bNeedGoPCK)
		{
			MoveInPCK();
			return;
		}
		else if (!P_PLAYER->m_FightMode)
		{
			MoveOut();
			return;
		}
	}

	static int nWaitTime = 0;
	static BOOL bInFightMode = TRUE;
	if (!P_PLAYER->m_FightMode)
	{
		nLastMapID = 0;
		m_nLeaderIdx = 0;
		m_nPreNode = -1;
		m_nCurNode = -1;
		bInFightMode = 0;
		s_nJumpStep = 0;
		return;
	}
	if (!bInFightMode)	// return from city
	{
		if (nWaitTime++ < 6) return; // wait for 1.2s
		nWaitTime = 0;
		bInFightMode = TRUE;
	}
	if (LagJumpBreathe()) return;	// Dang dung khinh cong vuot cho ket

	BOOL bCurMoveByMap = m_bMoveByMap;
	m_bAroundPointPK = m_bAroundPoint;
	m_bMoveWithPK = theQuest.m_bAutoDoQuest && theQuest.m_bGoTrainGetPK && theQuest.m_TongPKOpt.m_bMovePt;
	if (m_bMoveWithPK)
	{
		bCurMoveByMap = 0;
		m_bAroundPointPK = TRUE;
		m_nPointX = theQuest.m_TongPKOpt.m_nPointX;
		m_nPointY = theQuest.m_TongPKOpt.m_nPointY;
	}

	if (IsFlagMoving()) return;
	if (LoadingMapPath()) return;

	if (m_bFollow)
		if (DoFollow())
		{	// Da thay nguoi can theo: bo tu di toi bai train (GoTrainBreathe khong con chay => m_bGoTrain ket lai,
			// theAttack.Breathe coi la "dang tim bai train" => khong danh quai, khong xuong ngua cho toi khi vao lai game)
			if (m_bGoTrain) GoTrainStop();
			if (PathTravelActive() && !theQuest.m_bAutoDoQuest && !theMain.m_nWcbStep) PathTravelStop();
			m_bFindingWay = 0; m_nPreNode = -1; m_nCurNode = -1;
			return;
		}

	m_bMoveByMapYS = bCurMoveByMap;
	if (theQuest.m_nYeSouMapID) m_bMoveByMapYS = TRUE;
	AutoGoTrainCheck();
	if (m_bGoTrain || m_bFindingWay)
	{	// Khong co TDP: khong tu tim duong sang map train (chi danh tai map dang dung)
		// Ngoai le: vua di xa phu (m_bGoTrain) ma bi tha o map ke ben (vd xa phu "Duoc Vuong tang 3" tha o tang 4)
		// -> van di bo qua cong sang map train, khong can TDP
		int nGoMap = m_bGoTrain ? m_nGoTrainMap : theMap.m_nTrainMapID;
		BOOL bNeedTDP = !m_bGoTrain || PathMapHasDriver(*g_pMapID);
		if (nGoMap && *g_pMapID != nGoMap && bNeedTDP && !theMain.HaveTownPortal())
		{
			SystemMessage("<color=yellow>Khong co Tho Dia Phu - khong tu di toi map luyen cong");
			GoTrainStop();
			m_bFindingWay = 0;
		}
		else if (nGoMap && *g_pMapID != nGoMap && theMap.m_bDriverOnly && PathMapHasDriver(*g_pMapID))
		{	// Len map = Xa phu hoac THP: dang o thanh/thon (vd ra khoi cong thanh) - khong di bo sang map khac
			SystemMessage("<color=yellow>Len map = Xa phu hoac THP - khong di bo tu thanh toi map luyen cong");
			GoTrainStop();
			m_bFindingWay = 0;
		}
	}
	if (m_bGoTrain && GoTrainBreathe()) return;	// Vua chon xa phu: tim duong toi bai train
	if (m_bFindingWay && theMap.m_nTrainMapID && !theQuest.m_nYeSouMapID)
		if (RouteFinding(theMap.m_nTrainMapID)) return;

	m_bFindingWay = 0;

	if (!m_bMoveByMapYS && !m_bAroundPointPK && !m_bRouteMove) { m_nStatus = status_idle; return; }

	if (theAttack.m_nBossWait) return;

	if (!theAttack.m_bAutoHit)
	{
		m_nStatus = status_idle;
		if (!m_bFollow) return;
	}
	P_PLAYER->GetMapPos(&nPlayerX, &nPlayerY);
	if (m_bMoveByMapYS)
	{
		if (m_nMapPoints <= 0) return;
		if (m_nCurNode < 0) { GetCurNode(); m_nStatus = status_movefirst; }
		if (m_nCurNode < 0) return;
		nDestX = MapPoints[m_nCurNode].x;
		nDestY = MapPoints[m_nCurNode].y;
	}
	else if (m_bAroundPointPK)
	{
		if (!m_nPointX && !m_nPointY) return;
		nDestX = m_nPointX;
		nDestY = m_nPointY;
	}
	else //m_bRouteMove
	{
		if (m_nNodesMapID && *g_pMapID != m_nNodesMapID)
		{	// Danh sach toa do thuoc ban do khac (vd "Tro lai dia diem cu" dua sai map): tu di theo MapPath, 30s thu 1 lan
			static DWORD dwTryTick = 0;
			if (!theMap.m_nTrainMapID && GetTickCount() - dwTryTick > 30000 && theMain.HaveTownPortal())
			{
				dwTryTick = GetTickCount();
				if (PathTravelStart(m_nNodesMapID, FALSE)) return;
			}
			m_nStatus = status_idle;
			m_nCurNode = -1;
			return;
		}
		if (m_nCurNode < 0) { GetCurNode(); m_nStatus = status_movefirst; }
		if (m_nCurNode < 0) return;
		if (!m_Nodes[m_nCurNode].x || !m_Nodes[m_nCurNode].y)
			return;
		nDestX = m_Nodes[m_nCurNode].x;
		nDestY = m_Nodes[m_nCurNode].y;
	}
	dZ = __distance(nPlayerX, nPlayerY, nDestX, nDestY);
	// Diem can toi con xa (vd vua ra cong thanh, vua hoi sinh): di theo MapPath cua map, khong dam thang vao tuong.
	// Het duong MapPath (0) hoac map khong co MapPath (-1) thi game tu tim duong (GoTo) toi diem;
	// GoTo ket thi chay thang nhu cu
	static POINT ptFar = { 0, 0 };	// Diem dang tiep can tu xa
	if (m_bRouteMove)
	{
		// Danh sach toa do train: nguoi choi da ghi nhan 2 diem lien tiep la di toi duoc
		// -> dung KPlayer::GoTo (game tu tim duong) di thang giua 2 diem, KHONG di vong theo
		// MapPath. GoTo dang dua toi -> xong; da gan (<=AUTO_GOTO_NEAR) / GoTo ket -> MoveTo not doan cuoi.
		if (AutoGoTo(nDestX, nDestY) > 0)
		{
			m_nLagCount = 0;
			m_nLastdZ = MAX_INTNUM;
			return;
		}
	}
	else
	{
		if (dZ > 1000) { ptFar.x = nDestX; ptFar.y = nDestY; }
		if (dZ <= AUTO_GOTO_NEAR) ptFar.x = ptFar.y = 0;
		if (ptFar.x == nDestX && ptFar.y == nDestY)
		{
			if (PathWalkTo(nDestX, nDestY) > 0 || AutoGoTo(nDestX, nDestY) > 0)
			{
				m_nLagCount = 0;
				m_nLastdZ = MAX_INTNUM;
				return;
			}
		}
	}
	if (dZ < 64)
	{
		m_nLagCount = 0;
		m_nTotalLagCount = 0;
		switch (m_nStatus)
		{
		case status_movefirst:
		case status_move:
			m_nStatus = status_stand;
			LureStart(nDestX, nDestY);	// Quay quai: vua toi toa do train
			break;
		case status_idle:
			if (m_bMoveByMapYS)
			{
				if (!P_PLAYER->m_bRideHorse && m_bRideHorseMove && HAVE_HORSE)
					ExecuteScript(SCK_SHORTCUT_HORSE); // Len ngua
				GetNextNode();
				m_nStatus = status_move;
			}
			else if (m_bAroundPointPK)
			{
				LureStart(nDestX, nDestY);	// Quay quai: ve lai tam danh quanh diem
				if (!P_PLAYER->m_bRideHorse && P_PLAYER->m_Doing == do_stand)
					g_pCurPlayer->DoSit(); // Sit down when reach center
			}
			else //m_bRouteMove
			{
				if (!P_PLAYER->m_bRideHorse && m_bRideHorseMove && HAVE_HORSE)
					ExecuteScript(SCK_SHORTCUT_HORSE); // Len ngua
				GetNextNode();
				m_nStatus = status_move;
			}
			break;
		}
	}
	else if (m_nStatus < status_stand)
	{
		static int nTimeCount = 0;
		nTimeCount++;
		if (nTimeCount % 3 == 1)
		{
			if (IsLagMoving(nPlayerX, nPlayerY, WalkDist(nPlayerX, nPlayerY, nDestX, nDestY, dZ), nDestX, nDestY)) return;
		}
		g_pCurPlayer->MoveTo(nDestX, nDestY);
	}
	else
	{
		m_nLagCount = 0;
		m_nTotalLagCount = 0;
	}
}

BOOL CVLMove::DoFollow()
{
	if (m_nLeaderIdx && PEOPLE_IDX == m_nLeaderIdx)
	{
		if (!g_pNpcArray[m_nLeaderIdx]->IsExist())
			theAttack.SetPeopleIdx(0);
		else
		{
			int nPlayerX, nPlayerY, nNpcX, nNpcY, dZ;
			P_PLAYER->GetMapPos(&nPlayerX, &nPlayerY);
			g_pNpcArray[m_nLeaderIdx]->GetMapPos(&nNpcX, &nNpcY);
			dZ = __distance(nPlayerX, nPlayerY, nNpcX, nNpcY);
			if (dZ > 1600)
				theAttack.SetPeopleIdx(0);
		}
	}
	m_nLeaderIdx = 0;
	if (GetLeaderChar(m_szLeader)) return TRUE;
	if (GetLeaderChar(g_pTeam->m_szMemName[0])) return TRUE;
	return 0;
}

// Khoang cach de do ket (IsLagMoving): dang di vong vat can thi tinh theo duong vong - duong thang co the tang
// khi vong qua tuong lam bao ket sai. Dich khong toi duoc / khong co du lieu => khoang cach thang
static int WalkDist(int px, int py, int tx, int ty, int dZ)
{
	int d = ObsWalkDist(px, py, tx, ty);
	return d < 0 ? dZ : d;
}

void CVLMove::GetCurNode()
{
	int nPlayerX, nPlayerY;
	P_PLAYER->GetMapPos(&nPlayerX, &nPlayerY);
	m_nCurNode = -1;
	if (m_bAroundPointPK && !m_bMoveByMapYS)
	{
		m_nCurNode = 0;
		return;
	}
	const POINT *pPts = m_bMoveByMapYS ? MapPoints : m_Nodes;
	int nCount = 0;
	if (m_bMoveByMapYS) nCount = m_nMapPoints;
	else while (nCount < MAX_NODES && m_Nodes[nCount].x && m_Nodes[nCount].y) nCount++;

	// Gan nhat theo duong di bo vong vat can: diem sat ben kia tuong khong con la "gan nhat" (truoc day chay thang vao tuong).
	// Khong diem nao toi duoc (bi vay / thieu du lieu) thi lay gan nhat theo duong thang nhu cu
	ObsFlood(nPlayerX, nPlayerY);
	for (int nPass = 0; nPass < 2 && m_nCurNode < 0; nPass++)
	{
		int dMin = MAX_INTNUM;
		for (int i = 0; i < nCount; i++)
		{
			int dZ = nPass == 0 ? ObsReachDist(pPts[i].x, pPts[i].y) : __distance(nPlayerX, nPlayerY, pPts[i].x, pPts[i].y);
			if (dZ < 0) continue;	// -1 = trong vung nhung bi tuong chan: bo qua (het diem toi duoc thi pass 1 xet theo duong thang)
			if (dZ < dMin)
			{
				dMin = dZ;
				m_nCurNode = i;
			}
		}
	}
}

void CVLMove::GetNextNode()
{
	if (m_bMoveByMapYS)
	{
		int nNextNode[8] = { -1, -1, -1, -1, -1, -1, -1, -1 };
		int nCount = 0;
		for (int i = 0; i < m_nMapPoints; i++)
		{
			if (i == m_nCurNode) continue;
			if (MapSides[i][m_nCurNode] == MAX_INTNUM) continue;
			if (i == m_nPreNode) continue;
			nNextNode[nCount++] = i;
		}
		if (nCount == 0 && m_nPreNode >= 0)
		{
			nNextNode[0] = m_nPreNode;
			m_nPreNode = m_nCurNode;
			m_nCurNode = nNextNode[0];
		}
		else
		{
			int i = P_PLAYER->m_SyncSignal % nCount; //random
			m_nPreNode = m_nCurNode;
			m_nCurNode = nNextNode[i];
		}
		return;
	}
	m_nPreNode = m_nCurNode;
	if (m_nCurNode < MAX_NODES - 1)
	{
		if (m_Nodes[m_nCurNode + 1].x && m_Nodes[m_nCurNode + 1].y)
			m_nCurNode = m_nCurNode + 1;
		else if (m_Nodes[0].x && m_Nodes[0].y)
			m_nCurNode = 0;
	}
	else if (m_Nodes[0].x && m_Nodes[0].y)
		m_nCurNode = 0;
}

void CVLMove::SavePoint()
{
	int nX, nY;
	char szMsg[128] = "";
	if (!m_bAroundPointPK && !m_bRouteMove) return;
	P_PLAYER->GetMapPos(&nX, &nY);
	if (m_bAroundPointPK)
	{
		if (m_bMoveWithPK) return;
		m_nPointX = nX;
		m_nPointY = nY;
		sprintf(szMsg, TV(L"<color=white>Quanh điểm: Ghi tọa độ <color=yellow>hiện tại <color=green>(%d/%d)"), nX >> 8, nY >> 9);
		KUiMiniMap *pMiniMap = (KUiMiniMap*)(MMP_BASE_ADD);
		if (pMiniMap) pMiniMap->SetMapFlag(0, 0);
	}
	else //m_bRouteMove
	{
		strcpy(szMsg, TV(L"<color=yellow>Lỗi: Số điểm đã đủ, không thể ghi thêm!"));
		for (int i = 0; i < MAX_NODES; i++)
		{
			if (m_Nodes[i].x && m_Nodes[i].y) continue;
			int nFlagX = *((int*)FLAG_X_OFFST);
			int nFlagY = *((int*)FLAG_Y_OFFST);
			if (nFlagX > 0 && nFlagY > 0)
			{
				nX = nFlagX << 4;
				nY = nFlagY << 5;
				sprintf(szMsg, TV(L"<color=white>Hành trình: Ghi tọa độ <color=yellow>cắm cờ <color=green>(%d/%d)"), nX >> 8, nY >> 9);
			}
			else
				sprintf(szMsg, TV(L"<color=white>Hành trình: Ghi tọa độ <color=yellow>hiện tại <color=green>(%d/%d)"), nX >> 8, nY >> 9);
			m_nCurNode = i;
			m_Nodes[m_nCurNode].x = nX;
			m_Nodes[m_nCurNode].y = nY;
			break;
		}
	}
	SystemMessage(szMsg);
	theMain.PostFeedBack(cmd_setpoint);
}

int CVLMove::GetLeaderChar(const char* szLeader)
{
	int nNpcX, nNpcY, nPlayerX, nPlayerY, dZ;
	if (!szLeader[0]) return 0;
	if (!strcmp(szLeader, P_PLAYER->Name)) return 0;
	P_PLAYER->GetMapPos(&nPlayerX, &nPlayerY);
	for (int i = 1; i < MAX_NPC; i++)
	{
		if (!g_pNpcArray[i]->IsExist()) continue;
		if (strcmp(g_pNpcArray[i]->Name, szLeader)) continue;
		//Found
		g_pNpcArray[i]->GetMapPos(&nNpcX, &nNpcY);
		dZ = __distance(nPlayerX, nPlayerY, nNpcX, nNpcY);
		if (dZ < 1200)
		{
			m_nStatus = status_idle;
			m_nLeaderIdx = i;
			// Gan va duong thang thong: gan PEOPLE_IDX = nguoi can theo (game tu chay theo nhu cu).
			// Xa / bi vat can chan: game (KNpcAI) chay thang se dam vao vat can => bo PEOPLE_IDX,
			// tu chay bang MoveTo (ne vat can theo luoi vat can - ObsSteer) toi gan nguoi can theo
			BOOL bClear = dZ <= MAX_MOVE_DISTANCE && ObsLineClear(nPlayerX, nPlayerY, nNpcX, nNpcY);
			if (theAttack.GetCurTargetIdx() == i && !bClear) theAttack.SetPeopleIdx(0);
			BOOL bGo = (theAttack.m_bAutoHit && dZ > m_nFollowDistance) ||
				((!theAttack.m_bAutoHit || (!theAttack.GetCurTargetIdx() && !theAttack.IsApproaching())) && dZ > MAX_FOLLOW_DISTANCE);
			if (bGo)
			{
				int nPeople = theAttack.GetCurTargetIdx();	// Xa qua: bo quai dang danh de chay lai (nhu truoc day)
				if (nPeople > 0 && nPeople < MAX_NPC && g_pNpcArray[nPeople]->m_Kind == kind_normal)
					theAttack.SetPeopleIdx(0);
				theAttack.StopApproach();
				if (bClear)
					theAttack.SetPeopleIdx(i);
				else
				{
					int nNear = min(m_nFollowDistance, MAX_FOLLOW_DISTANCE * 2) / 2;	// Dung cach nguoi can theo 1 chut
					int nDestX = nNpcX - (nNpcX - nPlayerX) * nNear / dZ;
					int nDestY = nNpcY - (nNpcY - nPlayerY) * nNear / dZ;
					PathNoteMove(nDestX, nDestY, "Theo sau");
					g_pCurPlayer->MoveTo(nDestX, nDestY);
				}
			}
			if (dZ > 300 && m_bRideHorseFollow && !P_PLAYER->m_bRideHorse && HAVE_HORSE)
				ExecuteScript(SCK_SHORTCUT_HORSE); // Len ngua
		}
		break;
	}
	return m_nLeaderIdx;
}

DWORD g_dwEnterMapTick = 0;

// Doi map: bo duong MapPath / trang thai di cu, cho map moi nap xong (nhan vat san sang + region quanh nhan vat
// da nap, toi da 8s) roi moi bat dau tinh g_dwEnterMapTick - cac buoc sau lap duong moi tu vi tri that tren map moi
void EnterMapUpdate()
{
	static int s_nEnterMap = 0;
	static BOOL s_bLoading = FALSE;
	static DWORD s_dwChange = 0;
	DWORD dwNow = GetTickCount();
	if (*g_pMapID != s_nEnterMap)
	{
		s_nEnterMap = *g_pMapID;
		s_bLoading = TRUE;
		s_dwChange = dwNow;
		PathWalkReset();
		theMove.OnMapChanged();
	}
	if (!s_bLoading) return;
	g_dwEnterMapTick = dwNow;
	BOOL bOk = g_pCurPlayer && g_pNpcArray[0] && g_pCurPlayer->m_nIndex > 0 && g_pCurPlayer->m_nIndex < MAX_NPC && !g_NetworkOffline;
	int x = 0, y = 0;
	if (bOk) P_PLAYER->GetMapPos(&x, &y);
	if ((bOk && (x || y) && ObsGet(x, y) >= 0) || dwNow - s_dwChange > 8000)
		s_bLoading = FALSE;
}

void CVLMove::OnMapChanged()
{
	m_nLagCount = 0;
	m_nTotalLagCount = 0;
	m_nLastdZ = MAX_INTNUM;
	m_nGoNode = -1;	// Danh sach di den noi luyen cong: chon lai diem bat dau tren map moi
	m_nGoDone = -1;
}

BOOL CVLMove::LoadingMapPath()
{
	if ((*g_pMapID) != nLastMapID)
	{	// Cho nap map 1.2s tinh tu luc vao map (vd MapPath da cho san khi qua cong thi khong cho them)
		if (MapEnterElapsed() < 1200) return TRUE;
		nLastMapID = (*g_pMapID);

		m_nCurPoint = -1;
		m_nMapPoints = 0;
		m_nPathPoints = 0;
		GetMapInfo();
		GetPathInfo();
	}

	return 0;
}

void CVLMove::GetMapInfo()
{
	char szMapID[4] = "0";
	sprintf(szMapID, "%d", (*g_pMapID));

	char szMapFile[64] = "";
	char szMapIniFile[MAX_PATH];
	sprintf(szMapIniFile, "%s\\Maps\\MapMenu.ini", theMain.m_szCurFolder);

	GetPrivateProfileString("Map_File", szMapID, NULL, szMapFile, 32, szMapIniFile);

	if (szMapFile[0])
		LoadMap(szMapFile);
}

void CVLMove::LoadMap(const char *szMapFile)
{
	char szMapFileName[MAX_PATH];
	sprintf(szMapFileName, "%s\\Maps\\%s", theMain.m_szCurFolder, szMapFile);
	FILE* pFile;
	pFile = fopen(szMapFileName, "r");
	if (pFile == NULL)
	{
		SystemMessage(0, TV(L"Không đọc được file MAP: %s!"), szMapFile);
		return;
	}
	int i = 0;
	while (i < MAX_POINTS)
	{
		int nRet = fscanf(pFile, "%d,%d\n", &MapPoints[i].x, &MapPoints[i].y);
		if (nRet < 2) break;
		i++;
	}
	m_nMapPoints = i;
	if (m_nMapPoints == 0)
	{
		SystemMessage(TV(L"Lỗi file MAP, xin đọc lại Hướng dẫn!"));
		return;
	}
	for (int i = 0; i < m_nMapPoints; i++)
		for (int j = 0; j < m_nMapPoints; j++)
			if (i == j) MapSides[i][j] = 0;
			else MapSides[i][j] = MAX_INTNUM;

	m_nMaxSide = 0;
	while (TRUE)
	{
		int si, sj;
		int nRet = fscanf(pFile, "%d,%d\n", &si, &sj);
		if (nRet < 2) break;
		MapSides[si][sj] = MapSides[sj][si] = __distance(MapPoints[si], MapPoints[sj]);
		if (MapSides[si][sj] > m_nMaxSide) m_nMaxSide = MapSides[si][sj];
	}
	fclose(pFile);
}

void CVLMove::GetPathInfo()
{
	char szMapID[4] = "0";
	sprintf(szMapID, "%d", (*g_pMapID));

	char szPathFile[64] = "";
	char szPathListFile[MAX_PATH];
	sprintf(szPathListFile, "%s\\Maps\\PathList.ini", theMain.m_szCurFolder);

	GetPrivateProfileString("Path_File", szMapID, NULL, szPathFile, 32, szPathListFile);

	if (szPathFile[0])
		LoadPath(szPathFile);
}

void CVLMove::LoadPath(const char *szPathFile)
{
	//Loading Path File
	char szPathFileName[MAX_PATH];
	sprintf(szPathFileName, "%s\\Maps\\Paths\\%s", theMain.m_szCurFolder, szPathFile);
	FILE* pFile;
	pFile = fopen(szPathFileName, "r");
	if (pFile == NULL)
	{
		SystemMessage(0, TV(L"Không đọc được file PATH: %s!"), szPathFile);
		return;
	}
	int i = 0;
	while (i < MAX_POINTS)
	{
		int nRet = fscanf(pFile, "%d,%d,%d\n", &PathPoints[i].x, &PathPoints[i].y, &PathPoints[i].z);
		if (nRet < 3) break;
		i++;
	}
	m_nPathPoints = i;
	if (m_nPathPoints == 0)
	{
		SystemMessage(TV(L"Lỗi file PATH, xin đọc lại Hướng dẫn!"));
		return;
	}
	fclose(pFile);
}

BOOL CVLMove::IsNpcInRange(int nNpcX, int nNpcY)
{
	int dZ;
	POINT ptNpc = { nNpcX, nNpcY };

	if (DebugQuestHoldMove()) return TRUE;	// Khong di theo danh sach toa do: khong gioi han quanh diem / duong
	if (m_bMoveByMapYS)
	{
		if (m_nMapPoints <= 0) return TRUE;
		if (m_nCurNode < 0) return 0;
		dZ = __distance(MapPoints[m_nCurNode].x, MapPoints[m_nCurNode].y, nNpcX, nNpcY);
		if (dZ <= m_nMaxSide / 4) return TRUE;
		//m_bAttackWhenMove is always TRUE
		if (m_nPreNode < 0) return TRUE;
		dZ = __getheight(MapPoints[m_nPreNode], MapPoints[m_nCurNode], ptNpc);
		if (dZ <= MapSides[m_nPreNode][m_nCurNode] / 4) return TRUE;
	}
	else if (m_bAroundPointPK)
	{
		if (m_nPointX && m_nPointY)
		{
			dZ = __distance(m_nPointX, m_nPointY, nNpcX, nNpcY);
			if (dZ <= theMove.m_nPointRange) return TRUE;
		}
	}
	else if (m_bRouteMove)
	{
		if (m_nCurNode < 0) return 0;
		dZ = __distance(m_Nodes[m_nCurNode].x, m_Nodes[m_nCurNode].y, nNpcX, nNpcY);
		if (dZ <= m_nPointRange) return TRUE;
		if (m_bAttackWhenMove)
		{
			if (m_nPreNode < 0) return TRUE;
			dZ = __getheight(m_Nodes[m_nPreNode], m_Nodes[m_nCurNode], ptNpc);
			if (m_nMaxSide > 0)
			{
				if (dZ <= m_nMaxSide / 4) return TRUE;
			}
			else if (dZ <= m_nPointRange) return TRUE;
		}
	}
	else
		return TRUE;
	return 0;
}

BOOL CVLMove::LoadYeSouMap(int nYSMapID)
{
	char szMapID[4] = "0";
	sprintf(szMapID, "%d", nYSMapID);

	char szMapFile[64] = "";
	char szMapIniFile[MAX_PATH];
	sprintf(szMapIniFile, "%s\\Maps\\MapMenu.ini", theMain.m_szCurFolder);
	GetPrivateProfileString("Map_File", szMapID, NULL, szMapFile, 32, szMapIniFile);
	if (szMapFile[0]) return TRUE;

	return 0;
}

void CVLMove::MoveOut()
{
	static int nTimeWait = 15;
	KUiMsgSel2 *pMsgSel2 = (KUiMsgSel2*)GetPtrAddress(MSG2_BASE_ADD);
	if (pMsgSel2 && pMsgSel2->IsVisible()) // DICH QUAN
	{
		pMsgSel2->OnClickMsg(1); // Exit
		nTimeWait = 10;
		return;
	}
	if (nTimeWait) { nTimeWait--; return; }
	if ((*g_pMapID) >= 390 && (*g_pMapID) <= 392) g_pCurPlayer->MoveTo(42438, 101866);
	for (int i = 1; i < MAX_NPC; i++)
	{
		if (!g_pNpcArray[i]->IsExist() || g_pNpcArray[i]->m_Kind != kind_dialoger) continue;
		if (strcmp(g_pNpcArray[i]->Name, TV(L"Dịch Quan"))) continue;
		theAttack.SetPeopleIdx(i);
		nTimeWait = 15;
		break;
	}
}

void CVLMove::MoveInPCK()
{
	static BOOL bExitMap = 0;
	static BOOL bQuestDone = 0;
	static int nTimeWait = 15;
	static int nCurNode = -1;

	KUiMsgSel2 *pMsgSel2 = (KUiMsgSel2*)GetPtrAddress(MSG2_BASE_ADD);
	if (!P_PLAYER->m_FightMode) // Starting
	{
		nCurNode = -1;
		if (pMsgSel2 && pMsgSel2->IsVisible()) // DICH QUAN
		{
			if (bExitMap)
			{
				pMsgSel2->OnClickMsg(1); // Exit
				bExitMap = 0;
			}
			else
			{
				if (!LoadPCKMap()) return;
				pMsgSel2->OnClickMsg(0); // Enter
				bQuestDone = 0;
			}
			nTimeWait = 10;
			return;
		}
		if (nTimeWait) { nTimeWait--; return; }
		if ((*g_pMapID) >= 390 && (*g_pMapID) <= 392) g_pCurPlayer->MoveTo(42438, 101866);
		for (int i = 1; i < MAX_NPC; i++)
		{
			if (!g_pNpcArray[i]->IsExist() || g_pNpcArray[i]->m_Kind != kind_dialoger) continue;
			if (strcmp(g_pNpcArray[i]->Name, TV(L"Dịch Quan"))) continue;
			theAttack.SetPeopleIdx(i);
			nTimeWait = 15;
			break;
		}
		return;
	}
	if (bQuestDone)
	{
		if (pMsgSel2 && pMsgSel2->IsVisible())
		{
			if (pMsgSel2->m_nNumMessage < 3)
				pMsgSel2->OnClickMsg(0);
			else
			{
				int nMark = g_pCurPlayer->GetTaskVal(pck_marks);
				if (nMark >= 5)
					pMsgSel2->OnClickMsg(0);
				else
					pMsgSel2->OnClickMsg(1);
			}
			bExitMap = TRUE;
			nTimeWait = 10;
			return;
		}
		if (nTimeWait) { nTimeWait--; return; }
		if (IAM_IN_A_PARTY)
		{
			g_pCoreShell->TeamOperation(TEAM_LEAVE, 0, 0);
			return;
		}
		for (int i = 1; i < MAX_NPC; i++)
		{
			if (!g_pNpcArray[i]->IsExist() || g_pNpcArray[i]->m_Kind != kind_dialoger) continue;
			if (strcmp(g_pNpcArray[i]->Name, TV(L"Cánh trắng")) &&
				strcmp(g_pNpcArray[i]->Name, TV(L"Tiêu Trấn")) &&
				strcmp(g_pNpcArray[i]->Name, TV(L"Nạp Lan Thanh Thành"))) continue;
			theAttack.SetPeopleIdx(i);
			nTimeWait = 15;
			break;
		}
	}
	if (g_UiInformation2->IsVisible())
		g_UiInformation2->Hide();
	// Check Lag
	static int nCheckLag = 0;
	if (PEOPLE_IDX)
	{
		nCheckLag++;
		if (nCheckLag > 7) { theAttack.SetPeopleIdx(0); }
		return;
	}
	nCheckLag = 0;
	int nPlayerX, nPlayerY;
	int dZ, dMin = MAX_INTNUM;
	P_PLAYER->GetMapPos(&nPlayerX, &nPlayerY);
	if (!theQuest.m_AddPointOpt.m_bNoClickTS)
	{
		//Keep Searching
		int nNpcX, nNpcY;
		static DWORD dwLastID1 = 0;
		static DWORD dwLastID2 = 0;
		static DWORD dwLastID3 = 0;
		for (int i = 1; i < MAX_NPC; i++)
		{
			if (!g_pNpcArray[i]->IsExist()) continue;
			if (g_pNpcArray[i]->m_Kind != kind_dialoger) continue;
			if (!strstr(g_pNpcArray[i]->Name, TV(L"Tọa độ trinh sát"))) continue;
			if (g_pNpcArray[i]->m_dwID == dwLastID1 || g_pNpcArray[i]->m_dwID == dwLastID2) continue;
			g_pNpcArray[i]->GetMapPos(&nNpcX, &nNpcY);
			dZ = __distance(nPlayerX, nPlayerY, nNpcX, nNpcY);
			if (dZ < 640)
			{
				if (!dwLastID1) dwLastID1 = g_pNpcArray[i]->m_dwID;
				else if (!dwLastID2) dwLastID2 = g_pNpcArray[i]->m_dwID;
				else if (!dwLastID3) dwLastID3 = g_pNpcArray[i]->m_dwID;
				else { dwLastID1 = dwLastID2; dwLastID2 = dwLastID3; dwLastID3 = g_pNpcArray[i]->m_dwID; }
				theAttack.SetPeopleIdx(i);
				return;
			}
		}
	}
	// Keep Moving
	static int nTimeCount = 0;
	nTimeCount++;
	if (nTimeCount % 3 != 1) return; // 600ms
	if (nCurNode < 0)
	{
		for (int i = 0; i < m_nPCKPoints; i++)
		{
			dZ = __distance(nPlayerX, nPlayerY, PCKPoints[i].x, PCKPoints[i].y);
			if (dZ < dMin)
			{
				dMin = dZ;
				nCurNode = i;
			}
		}
	}
	if (nCurNode < 0) return;
	dZ = __distance(nPlayerX, nPlayerY, PCKPoints[nCurNode].x, PCKPoints[nCurNode].y);
	if (dZ <= 100)
	{
		if (PCKPoints[nCurNode].z < 0) // Check Point)
		{
			bQuestDone = TRUE;
			return;
		}
		else
			nCurNode = PCKPoints[nCurNode].z;
	}
	g_pCurPlayer->MoveTo(PCKPoints[nCurNode].x, PCKPoints[nCurNode].y);
}

BOOL CVLMove::LoadPCKMap()
{
	char szFileName[MAX_PATH];
	sprintf(szFileName, "%s\\Maps\\", theMain.m_szCurFolder);
	if ((*g_pMapID) >= city_phongky1 && (*g_pMapID) <= city_phongky3)
	{
		if (theQuest.m_AddPointOpt.m_bNoClickTS)
			strcat(szFileName, "PhongKy0.map");
		else
			strcat(szFileName, "PhongKy1.map");
	}
	else if ((*g_pMapID) >= 390 && (*g_pMapID) <= 392)
		strcat(szFileName, "SonThan.map");
	else
		strcat(szFileName, "ThienBao.map");
	FILE* pFile = fopen(szFileName, "r");
	if (pFile == NULL) return 0;
	memset(PCKPoints, sizeof(PCKPoints), 0);
	int i = 0;
	while (i < MAX_POINTS)
	{
		int nRet = fscanf(pFile, "%d,%d,%d\n", &PCKPoints[i].x, &PCKPoints[i].y, &PCKPoints[i].z);
		if (nRet < 3) break;
		if (PCKPoints[i].z == 0)
			PCKPoints[i].z = i + 1;
		i++;
	}
	m_nPCKPoints = i;
	fclose(pFile);
	if (m_nPCKPoints <= 0) return 0;
	return TRUE;
}

BOOL CVLMove::IsFlagMoving()
{
	int nFlagX = *((int*)FLAG_X_OFFST);
	int nFlagY = *((int*)FLAG_Y_OFFST);
	return (nFlagX > 0 && nFlagY > 0);
}

// (nDestX, nDestY): noi dang chay toi - de nhay khinh cong (0,0 = khong biet, khong nhay)
BOOL CVLMove::IsLagMoving(int nPlayerX, int nPlayerY, int dZ, int nDestX, int nDestY)
{
	if (dZ > m_nLastdZ - 32)
		m_nLagCount++;
	m_nLastdZ = dZ;
	if (m_nLagCount > 7)
	{
		unsigned int number;
		rand_s(&number);
		int nDestX = nPlayerX + (int)(((double)number / (double)UINT_MAX * 512) - 256);
		rand_s(&number);
		int nDestY = nPlayerY + (int)(((double)number / (double)UINT_MAX * 1024) - 512);
		m_nLagCount = 0; m_nCurNode = -1; m_nCurPoint = -1;
		if ((*g_pMapID) != city_visondao)
		{
			m_nTotalLagCount++;
			// Bat khinh cong: ket > 5 lan thi nhay khinh cong, ket gap doi (> 10) moi dung TDP
			int nTDPCount = theRecover.m_bLagUseSkill ? 10 : 5;
			if (m_nTotalLagCount > nTDPCount && !theRecover.m_bLagUseTDP)
				m_nTotalLagCount = 0;	// Tat "dung TDP khi lag": chi doi huong chay tiep
			else if (m_nTotalLagCount > nTDPCount)
			{
				m_nTotalLagCount = 0;
				theMap.m_bNoDriverRecent = TRUE;
				SystemMessage(TV(L"<color=yellow>Kẹt hẳn rồi, về thành lát quay lại."));
				theRecover.OnUseAM();
				return TRUE;
			}
			else if (m_nTotalLagCount > 5 && theRecover.m_bLagUseSkill && LagJumpStart(nPlayerX, nPlayerY, nDestX, nDestY))
			{
				SystemMessage(TV(L"<color=yellow>Kẹt đường rồi, dùng khinh công nhảy qua."));
				return TRUE;
			}
		}
		SystemMessage(TV(L"<color=yellow>Kẹt đường rồi, chạy hướng khác xem sao."));
		g_pCurPlayer->MoveTo(nDestX, nDestY);
		return TRUE;
	}
	return 0;
}

void CVLMove::GoTrainStart(int nMapID)
{
	m_bGoTrain = TRUE;
	m_nGoTrainMap = nMapID;
	m_dwGoTrainTick = 0;
	m_nGoNode = -1;
	m_nGoDone = -1;
}

void CVLMove::GoTrainStop()
{
	m_bGoTrain = 0;
	m_nGoTrainMap = 0;
	m_dwGoTrainTick = 0;
	m_nGoNode = -1;
	m_nGoDone = -1;
}

// Nguoi can theo (o Theo sau hoac doi truong) dang co trong tam nhin, khong ke xa gan
static BOOL LeaderInSight(const char* szLeader)
{
	const char* szNames[2] = { szLeader, g_pTeam->m_szMemName[0] };
	for (int n = 0; n < 2; n++)
	{
		if (!szNames[n][0] || !strcmp(szNames[n], P_PLAYER->Name)) continue;
		for (int i = 1; i < MAX_NPC; i++)
			if (g_pNpcArray[i]->IsExist() && !strcmp(g_pNpcArray[i]->Name, szNames[n])) return TRUE;
	}
	return FALSE;
}

// Vua bat auto (Tu danh) khi dang dung san ngoai thanh, chua qua xa phu: map khac map train -> MapPath qua cong
// toi map train; dung map train ma xa bai train va co danh sach di den noi luyen cong -> di theo danh sach.
// Bat Theo sau: chi bo qua khi thay nguoi can theo (khong thay ai thi van tu di toi bai train)
void CVLMove::AutoGoTrainCheck()
{
	static BOOL s_bPrevAuto = FALSE;
	BOOL bAuto = theAttack.m_bAutoHit;
	BOOL bRise = bAuto && !s_bPrevAuto;
	s_bPrevAuto = bAuto;
	if (!bRise || m_bGoTrain || PathTravelActive()) return;
	if (m_bFollow && LeaderInSight(m_szLeader)) return;
	int nTarget = theMap.m_nTrainMapID;
	if (!nTarget || theQuest.m_nYeSouMapID || (theQuest.m_bAutoDoQuest && theQuest.m_bGoTrainGetPK)) return;
	if (*g_pMapID == nTarget)
	{
		int x, y;
		P_PLAYER->GetMapPos(&x, &y);
		if (!HaveGoNodes() || NearTrainArea(x, y)) return;
	}
	SystemMessage("<color=green>Bat auto ngoai bai train - tu di toi noi luyen cong");
	GoTrainStart(nTarget);
}

// Co danh sach toa do di den noi luyen cong o map dang dung
BOOL CVLMove::HaveGoNodes()
{
	if (!m_GoNodes[0].x || !m_GoNodes[0].y) return FALSE;
	return !m_nGoNodesMapID || m_nGoNodesMapID == *g_pMapID;
}

// Nhan vat da o gan bai train (vd "Tro lai dia diem cu" dua thang toi bai, hoac vua login o ngay bai
// train): khong can di theo danh sach di den (khong chay nguoc ra cong roi di lai tu dau)
BOOL CVLMove::NearTrainArea(int nPlayerX, int nPlayerY)
{
	const int nNear = 400;
	// 1) Da o sat diem / toa do luyen cong
	if (m_bMoveByMapYS)
	{
		for (int i = 0; i < m_nMapPoints; i++)
			if (__distance(nPlayerX, nPlayerY, MapPoints[i].x, MapPoints[i].y) <= nNear) return TRUE;
	}
	else if (m_bAroundPointPK)
	{
		if ((m_nPointX || m_nPointY) && __distance(nPlayerX, nPlayerY, m_nPointX, m_nPointY) <= nNear) return TRUE;
	}
	else if (m_bRouteMove && (!m_nNodesMapID || m_nNodesMapID == *g_pMapID))
	{
		for (int i = 0; i < MAX_NODES && m_Nodes[i].x && m_Nodes[i].y; i++)
			if (__distance(nPlayerX, nPlayerY, m_Nodes[i].x, m_Nodes[i].y) <= nNear) return TRUE;
	}
	// 2) Co danh sach di den noi luyen cong (GoNodes): neu dang o gan diem CUOI (bai train) hon la gan
	//    diem DAU (cong vao) => nhan vat da o trong map, da toi noi luyen cong -> danh tai cho, khong
	//    chay nguoc ra cong di lai danh sach tu dau.
	if (HaveGoNodes())
	{
		int nCount = 0;
		while (nCount < MAX_NODES && m_GoNodes[nCount].x && m_GoNodes[nCount].y) nCount++;
		if (nCount >= 2 &&
			__distance(nPlayerX, nPlayerY, m_GoNodes[nCount - 1].x, m_GoNodes[nCount - 1].y) <
			__distance(nPlayerX, nPlayerY, m_GoNodes[0].x, m_GoNodes[0].y))
			return TRUE;
	}
	return FALSE;
}

// Diem chua di qua (sau m_nGoDone) gan nhat THAT SU di toi duoc (loang vong vat can tu nhan vat), trong vung
// map da nap (~1500). -1 = khong co. *pReach = quang duong di bo toi diem do
int CVLMove::GoNodesNearest(int nPlayerX, int nPlayerY, int nCount, int *pReach)
{
	int nBest = -1, dBest = MAX_INTNUM;
	ObsFlood(nPlayerX, nPlayerY);
	for (int i = m_nGoDone + 1; i < nCount; i++)
	{
		if (__distance(nPlayerX, nPlayerY, m_GoNodes[i].x, m_GoNodes[i].y) > 1500) continue;
		int d = ObsReachDist(m_GoNodes[i].x, m_GoNodes[i].y);
		if (d < 0) continue;	// -1 = bi tuong chan: khong phai "toi duoc"
		if (d < dBest) { dBest = d; nBest = i; }
	}
	*pReach = dBest;
	return nBest;
}

// O map train, co danh sach di den noi luyen cong: MapPath (khong co thi game tu tim duong) toi diem dau tien,
// sau do chay thang lan luot qua cac diem con lai. Tu khi da toi 1 diem cua danh sach: moi 1s (va ngay khi bi dich
// chuyen, vd qua diem nhay) chon lai muc tieu = diem chua di qua gan nhat co duong toi (khong quay lai diem nhay).
// Chua toi diem nao (vua vao cong) thi luon theo MapPath toi diem dau tien, khong chon lai. TRUE = dang di; FALSE + m_bGoTrain tat = da toi diem cuoi;
// FALSE + m_bGoTrain con = khong co danh sach -> GoTrainBreathe di toi diem dau tien cua bai train nhu cu
BOOL CVLMove::GoNodesBreathe()
{
	if (!HaveGoNodes()) return FALSE;
	int nCount = 0;
	while (nCount < MAX_NODES && m_GoNodes[nCount].x && m_GoNodes[nCount].y) nCount++;

	int nPlayerX, nPlayerY;
	P_PLAYER->GetMapPos(&nPlayerX, &nPlayerY);
	static DWORD s_dwPickTick = 0;
	static POINT s_ptLast = { 0, 0 };
	BOOL bPick = FALSE;
	if (m_nGoNode < 0)
	{	// Bat dau: da o bai train -> bo qua; dang dung sat 1 diem cua danh sach -> di tiep tu diem do;
		// con lai -> MapPath toi diem dau tien
		m_nGoNode = 0;
		m_nGoDone = -1;
		if (NearTrainArea(nPlayerX, nPlayerY))
			m_nGoNode = nCount;
		else
			for (int i = nCount - 1; i > 0; i--)
				if (__distance(nPlayerX, nPlayerY, m_GoNodes[i].x, m_GoNodes[i].y) <= 300) { m_nGoNode = i; m_nGoDone = i - 1; break; }
		m_nLagCount = 0;
		m_nLastdZ = MAX_INTNUM;
		m_dwGoTrainTick = GetTickCount();
		s_dwPickTick = GetTickCount();
	}
	else if (__distance(nPlayerX, nPlayerY, s_ptLast.x, s_ptLast.y) > 600) bPick = TRUE;	// Bi dich chuyen (diem nhay)
	s_ptLast.x = nPlayerX; s_ptLast.y = nPlayerY;
	if (GetTickCount() - s_dwPickTick > 1000) bPick = TRUE;
	if (bPick && m_nGoDone >= 0 && m_nGoNode < nCount)
	{	// Diem gan nhat co duong toi gan hon muc tieu dang di -> doi muc tieu
		s_dwPickTick = GetTickCount();
		int nReach, nNear = GoNodesNearest(nPlayerX, nPlayerY, nCount, &nReach);
		int nCurReach = (__distance(nPlayerX, nPlayerY, m_GoNodes[m_nGoNode].x, m_GoNodes[m_nGoNode].y) > 1500) ? MAX_INTNUM :
			ObsReachDist(m_GoNodes[m_nGoNode].x, m_GoNodes[m_nGoNode].y);
		if (nCurReach < 0) nCurReach = MAX_INTNUM;	// Muc tieu hien tai bi tuong chan: uu tien chuyen sang diem toi duoc
		if (nNear >= 0 && nNear != m_nGoNode && nReach < nCurReach)
		{
			SystemMessage(0, "<color=white>Di den noi luyen cong: chuyen sang diem %d (%d/%d) - gan nhat co duong toi",
				nNear + 1, m_GoNodes[nNear].x >> 8, m_GoNodes[nNear].y >> 9);
			m_nGoNode = nNear;
			m_nLagCount = 0;
			m_nLastdZ = MAX_INTNUM;
			m_dwGoTrainTick = GetTickCount();
		}
	}

	if (m_nGoNode >= nCount || GetTickCount() - m_dwGoTrainTick > 180000)
	{
		if (m_nGoNode < nCount) SystemMessage(0, "<color=yellow>Di theo danh sach toa do di den noi luyen cong qua 3 phut - bat dau danh tai cho");
		else SystemMessage(0, "<color=green>Da toi noi luyen cong - bat dau danh quai");
		GoTrainStop();
		m_bFindingWay = 0;
		m_nPreNode = -1;
		m_nCurNode = -1;
		m_nStatus = status_idle;
		return FALSE;
	}

	int nDestX = m_GoNodes[m_nGoNode].x, nDestY = m_GoNodes[m_nGoNode].y;
	int dZ = __distance(nPlayerX, nPlayerY, nDestX, nDestY);
	if (dZ <= 64)
	{	// Toi diem -> diem tiep theo (moi doan duoc 3 phut)
		m_nGoDone = m_nGoNode;
		m_nGoNode++;
		m_nLagCount = 0;
		m_nLastdZ = MAX_INTNUM;
		m_dwGoTrainTick = GetTickCount();
		return TRUE;
	}
	if (m_nGoDone < 0)
	{	// Chua toi diem nao (vua vao map): theo MapPath cua map tu cong vao
		if (PathWalkTo(nDestX, nDestY) > 0 || AutoGoTo(nDestX, nDestY) > 0)
		{
			m_nLagCount = 0;
			m_nLastdZ = MAX_INTNUM;
			return TRUE;
		}
		if (IsLagMoving(nPlayerX, nPlayerY, WalkDist(nPlayerX, nPlayerY, nDestX, nDestY, dZ), nDestX, nDestY)) return TRUE;
		g_pCurPlayer->GoTo(nDestX, nDestY);
		return TRUE;
	}
	// Cac diem sau: chay thang theo danh sach
	static int nTimeCount = 0;
	if (++nTimeCount % 3 == 1 && IsLagMoving(nPlayerX, nPlayerY, WalkDist(nPlayerX, nPlayerY, nDestX, nDestY, dZ), nDestX, nDestY)) return TRUE;
	g_pCurPlayer->MoveTo(nDestX, nDestY);
	return TRUE;
}

// Ra ngoai thanh sau khi chon xa phu: map hien tai khac map train -> MapToMap (PathTravelStart);
// dung map train -> co "Danh sach toa do di den noi luyen cong" thi MapPath toi diem dau tien cua danh sach roi chay
// theo danh sach toi diem cuoi (GoNodesBreathe); khong co danh sach thi MapPath toi diem dau tien cua bai train.
// Toi noi thi tat co (Tab Chien dau bat dau danh). TRUE = dang xu ly (Breathe dung lai)
BOOL CVLMove::GoTrainBreathe()
{
	int nTarget = m_nGoTrainMap;
	if (!nTarget) { GoTrainStop(); return FALSE; }
	if (!m_dwGoTrainTick) m_dwGoTrainTick = GetTickCount();

	if (*g_pMapID != nTarget)
	{
		if (PathTravelStart(nTarget, FALSE))
		{
			m_dwGoTrainTick = GetTickCount();
			return TRUE;	// PathTravelBreathe di; toi map train thi Breathe goi lai day
		}
		// Khong co lo trinh MapPath: tra ve cach cu (RouteFinding theo file .map / danh sach toa do)
		char szName[64] = "";
		theMain.GetMapName(nTarget, szName);
		SystemMessage(0, "<color=yellow>Khong co lo trinh MapPath toi map train %s (%d) - di theo cach cu", szName, nTarget);
		GoTrainStop();
		return FALSE;
	}

	if (GoNodesBreathe()) return TRUE;
	if (!m_bGoTrain) return FALSE;	// Da di het danh sach di den noi luyen cong

	// Diem dau tien cua bai train
	int nDestX = 0, nDestY = 0;
	if (m_bMoveByMapYS && m_nMapPoints > 0) { nDestX = MapPoints[0].x; nDestY = MapPoints[0].y; }
	else if (m_bAroundPointPK) { nDestX = m_nPointX; nDestY = m_nPointY; }
	else if (m_bRouteMove && (!m_nNodesMapID || m_nNodesMapID == *g_pMapID)) { nDestX = m_Nodes[0].x; nDestY = m_Nodes[0].y; }

	int nPlayerX, nPlayerY;
	P_PLAYER->GetMapPos(&nPlayerX, &nPlayerY);
	int dZ = (nDestX || nDestY) ? __distance(nPlayerX, nPlayerY, nDestX, nDestY) : 0;
	if (dZ <= 64 || GetTickCount() - m_dwGoTrainTick > 180000)
	{
		if (dZ > 64) SystemMessage(0, "<color=yellow>Di toi bai train qua 3 phut - bat dau danh tai cho");
		else SystemMessage(0, "<color=green>Da toi bai train - bat dau danh quai");
		GoTrainStop();
		m_bFindingWay = 0;
		m_nPreNode = -1;
		m_nCurNode = -1;
		m_nStatus = status_idle;
		return FALSE;
	}
	if (PathWalkTo(nDestX, nDestY) > 0 || AutoGoTo(nDestX, nDestY) > 0)
	{
		m_nLagCount = 0;
		m_nLastdZ = MAX_INTNUM;
		return TRUE;
	}
	if (IsLagMoving(nPlayerX, nPlayerY, dZ, nDestX, nDestY)) return TRUE;
	g_pCurPlayer->GoTo(nDestX, nDestY);
	return TRUE;
}

BOOL CVLMove::RouteFinding(int nTrainMapID)
{
	static int nTimerCount = 0;
	nTimerCount++;
	if (nTimerCount % 3 != 1) return TRUE;	// make it as 600ms

	if ((*g_pMapID) != nTrainMapID)			// not yet reach the right Map
		return RunningToDoor();
	else
		return RunningToPos();
}

BOOL CVLMove::RunningToDoor()
{
	int dZ, dMin = MAX_INTNUM;
	int nPlayerX, nPlayerY;
	static POINT LastPos = { 0, 0 };

	if (m_nPathPoints <= 0) return 0;

	P_PLAYER->GetMapPos(&nPlayerX, &nPlayerY);

	if (m_nCurPoint < 0)
		SystemMessage(TV(L"Đang đi tìm bản đồ luyện công..."));
	else
	{
		dZ = __distance(LastPos.x, LastPos.y, nPlayerX, nPlayerY);
		if (dZ > 1600) m_nCurPoint = -1;
	}

	//Save last pos
	LastPos.x = nPlayerX;
	LastPos.y = nPlayerY;

	//Find nearest point.
	if (m_nCurPoint < 0)
	{
		for (int i = 0; i < m_nPathPoints; i++)
		{
			dZ = __distance(PathPoints[i].x, PathPoints[i].y, nPlayerX, nPlayerY);
			if (dZ < dMin)
			{
				dMin = dZ;
				m_nCurPoint = i;
			}
		}
	}

	dZ = __distance(PathPoints[m_nCurPoint].x, PathPoints[m_nCurPoint].y, nPlayerX, nPlayerY);
	if (dZ <= 64)
	{
		m_nLagCount = 0;
		m_nTotalLagCount = 0;
		if (PathPoints[m_nCurPoint].z > 0 && m_nCurPoint < m_nPathPoints)
		{
			m_nCurPoint++;	// To the next Point
			if (!P_PLAYER->m_bRideHorse && HAVE_HORSE)
				ExecuteScript(SCK_SHORTCUT_HORSE); // Len ngua
		}
		else
		{
			m_nCurPoint = -1;
			return TRUE;	// Reach the End
		}
	}
	if (IsLagMoving(nPlayerX, nPlayerY, dZ, PathPoints[m_nCurPoint].x, PathPoints[m_nCurPoint].y)) return TRUE;
	g_pCurPlayer->MoveTo(PathPoints[m_nCurPoint].x, PathPoints[m_nCurPoint].y);
	return TRUE;
}

BOOL CVLMove::RunningToPos()
{
	int nPlayerX, nPlayerY;
	int dZ, dMin;
	static int nStart = -1;
	static int nEnd = -1;

	if (m_nMapPoints <= 0) return 0;

	P_PLAYER->GetMapPos(&nPlayerX, &nPlayerY);
	POINT pntPlayer = { nPlayerX, nPlayerY };

	if (m_nCurPoint < 0)
	{
		SystemMessage(TV(L"Đến bản đồ luyện công! Đi tìm bãi..."));
		//Finding Start Point (near the destination)
		POINT ptnStart = { 0, 0 };
		nStart = m_nMapPoints / 2;
		if (m_bMoveByMapYS)
		{
			SystemMessage(TV(L"Đến khu vực luyện công! Bắt đầu..."));
			return 0;
		}
		else if (m_bAroundPointPK)
		{
			if (!m_nPointX && !m_nPointY) return 0;
			ptnStart.x = m_nPointX;
			ptnStart.y = m_nPointY;
		}
		else if (m_bRouteMove)
		{
			if (!m_Nodes[0].x && !m_Nodes[0].y) return 0;
			ptnStart = m_Nodes[0];
		}
		if (ptnStart.x && ptnStart.y)
		{
			dMin = MAX_INTNUM;
			for (int i = 0; i < m_nMapPoints; i++)
			{
				dZ = __distance(MapPoints[i], ptnStart);
				if (dZ < dMin)
				{
					dMin = dZ;
					nStart = i;
				}
			}
		}
		//Finding End Point (near the Player)
		dMin = MAX_INTNUM;
		for (int i = 0; i < m_nMapPoints; i++)
		{
			dZ = __distance(MapPoints[i], pntPlayer);
			if (dZ < dMin)
			{
				dMin = dZ;
				nEnd = i;
			}
		}
		if (!Dijkstra(nStart, nEnd)) return 0;
		m_nCurPoint = nEnd;
	}

	//Tracing the Path
	if (m_nCurPoint >= 0)
	{
		dZ = __distance(MapPoints[m_nCurPoint], pntPlayer);
		if (dZ <= 64)
		{
			m_nLagCount = 0;
			m_nTotalLagCount = 0;
			if (m_nCurPoint != nStart)
			{
				m_nCurPoint = TraceRoute[m_nCurPoint];
				if (!P_PLAYER->m_bRideHorse && HAVE_HORSE)
					ExecuteScript(SCK_SHORTCUT_HORSE); // Len ngua
			}
			else
			{
				SystemMessage(TV(L"Đến khu vực luyện công! Bắt đầu..."));
				m_nCurPoint = -1;
				return 0;
			}
		}
		if (IsLagMoving(nPlayerX, nPlayerY, dZ, MapPoints[m_nCurPoint].x, MapPoints[m_nCurPoint].y)) return TRUE;
		g_pCurPlayer->MoveTo(MapPoints[m_nCurPoint].x, MapPoints[m_nCurPoint].y);
		return TRUE;
	}
	return 0;
}

BOOL CVLMove::Dijkstra(int nStart, int nEnd)
{
	int Dist[MAX_POINTS];
	BOOL Free[MAX_POINTS];
	BOOL bStop = 0;
	int i, u, v, min;
	if (nEnd < 0 || nStart < 0) return 0;
	for (i = 0; i < m_nMapPoints; i++)
	{
		Dist[i] = MapSides[nStart][i];
		TraceRoute[i] = nStart;
	}
	memset(Free, TRUE, sizeof(Free));
	do {
		u = -1; min = MAX_INTNUM;
		for (i = 0; i < m_nMapPoints; i++)
			if (Free[i] && Dist[i] < min)
			{
				min = Dist[i];
				u = i;
			}
		if (u == -1 || u == nEnd) bStop = TRUE;
		if (!bStop)
		{
			Free[u] = FALSE;
			for (v = 0; v < m_nMapPoints; v++)
				if (Free[v] && Dist[v] > (Dist[u] + MapSides[u][v]))
				{
					Dist[v] = Dist[u] + MapSides[u][v];
					TraceRoute[v] = u;
				}
		}

	} while (!bStop);

	return (Dist[nEnd] < MAX_INTNUM);
}

//================================Game tu tim duong (KPlayer::GoTo)================================
// Goi GoTo 1 lan cho moi diem dich, chi goi lai khi nhan vat dung yen (bi ngat: danh quai, nhat do...)
// hoac 3s khong tien them. Goi lai ma van khong tien (dich nam trong vat can...) qua GOTO_MAX_FAILS lan thi bo
#define GOTO_RECALL		3000	// 3s khong tien them thi goi lai
#define GOTO_STAND_WAIT	1000	// Dung yen sau 1s thi goi lai
#define GOTO_MAX_FAILS	4

static int		s_nGoToMap = 0;
static POINT	s_ptGoTo = { 0, 0 };
static DWORD	s_dwGoToCall = 0;		// Lan goi GoTo gan nhat (0 = chua goi)
static DWORD	s_dwGoToProgress = 0;	// Lan gan nhat khoang cach giam
static int		s_nGoToBest = MAX_INTNUM;
static int		s_nGoToFails = 0;

int AutoGoTo(int tx, int ty)
{
	if (tx <= 0 || ty <= 0) return -1;
	int x, y;
	P_PLAYER->GetMapPos(&x, &y);
	int d = __distance(x, y, tx, ty);
	DWORD dwNow = GetTickCount();
	if (*g_pMapID != s_nGoToMap || abs(tx - s_ptGoTo.x) > 32 || abs(ty - s_ptGoTo.y) > 32)
	{	// Dich moi
		s_nGoToMap = *g_pMapID;
		s_ptGoTo.x = tx; s_ptGoTo.y = ty;
		s_dwGoToCall = 0;
		s_dwGoToProgress = dwNow;
		s_nGoToBest = MAX_INTNUM;
		s_nGoToFails = 0;
	}
	if (d <= AUTO_GOTO_NEAR) return 0;
	if (s_nGoToFails >= GOTO_MAX_FAILS) return -1;
	if (d < s_nGoToBest - 64)
	{
		s_nGoToBest = d;
		s_dwGoToProgress = dwNow;
	}

	BOOL bCall = !s_dwGoToCall;
	if (!bCall && dwNow - s_dwGoToCall > GOTO_STAND_WAIT && P_PLAYER->m_Doing == do_stand) bCall = TRUE;
	if (!bCall && dwNow - s_dwGoToCall > GOTO_RECALL && dwNow - s_dwGoToProgress > GOTO_RECALL) bCall = TRUE;
	if (!bCall) return 1;

	if (s_dwGoToCall && dwNow - s_dwGoToProgress > 2000)
	{	// Lan goi truoc khong dua nhan vat tien them
		if (++s_nGoToFails >= GOTO_MAX_FAILS)
		{
#if ADMIN_MODE
			// Chi bao o ban Tester; ban User (ADMIN_MODE==0) tat
			SystemMessage(0, "<color=yellow>GoTo: khong tu tim duong toi (%d/%d) duoc, chay thang", tx >> 8, ty >> 9);
#endif
			return -1;
		}
	}
	else if (s_dwGoToCall)
		s_nGoToFails = 0;
#if ADMIN_MODE
	// Chi hien "GoTo: tu tim duong toi (x/y)" o ban Tester de debug; ban User (ADMIN_MODE==0) tat
	if (!s_dwGoToCall)
		SystemMessage(0, "<color=white>GoTo: tu tim duong toi <color=green>(%d/%d)", tx >> 8, ty >> 9);
#endif
	s_dwGoToCall = dwNow;
	PathNoteMove(tx, ty, "GoTo");
	g_pCurPlayer->GoTo(tx, ty);
	return 1;
}
// hàm di chuyển đến bất kỳ nơi nào trong thế giơi game (yêu cầu cần làm full map path , có thể không tìm đường được)
BOOL CVLMove::GoToWorld(int MapId, int nDestX, int nDestY)
{
	// lập đường đi từ map hiện tại tới (mapid, nDestX, nDestY)
	// nếu có nhiều đường đi thì chọn đường đi ngắn nhất
	// cách tính đường đi: BFS, Dijkstra, A*, ... ưu tiên cách nào nhanh nhất, ít tốn tài nguyên nhất
	// khoảng cách giữa a và b: __distance(a.x, a.y, b.x, b.y)
	// dùng đã chạy tới XaPhu chọn qua map thì khoảng cách là 0. ví dụ map A -> B nếu dùng xa phu thì là 0, 
	// nếu không dùng xa phu thì khoảng cách tổng khoảng cách giữa các điểm trên đường đi đến cổng vào map B
	// di chuyển giữa 2 điểm dùng hàm KPlayer::GoTo(int nX, int nY)
	return FALSE;
}

// ===== Quay quai (tab Di chuyen: IDC_LURE_CREEP_CHECK / IDC_LURE_CREEP_RANGE_EDIT) =====
// Vua toi 1 toa do train (diem danh quanh / diem trong danh sach toa do) ma quanh do (m_nLureRange) co hon 1 con quai:
// lay toa do do lam diem neo tam, chay qua tung con (gan nhat truoc) cho quai thay va duoi theo, chay ve diem neo
// roi moi cho theAttack danh. Trong luc quay khong chon muc tieu (CVLAttack::Breathe dung truoc ProcessAttack).
#define LURE_MAX_NPC		12		// Toi da so quai chay qua 1 lan
#define LURE_TOUCH			120		// Cach quai bao nhieu thi coi la da chay qua (quai da thay)
#define LURE_NPC_TIMEOUT	3000	// Chay toi 1 con qua lau (bi chan / quai chay) -> bo qua con do
#define LURE_BACK_TIMEOUT	6000	// Chay ve diem neo toi da
#define LURE_ANCHOR_NEAR	64		// Toi diem neo
#define LURE_COOLDOWN		8000	// Giua 2 lan quay (diem danh quanh: quay lai moi lan ve tam)

enum { lure_none = 0, lure_visit, lure_back };
static int		s_nLureState = lure_none;
static int		s_nLureMap = 0;
static POINT	s_ptLureAnchor = { 0, 0 };
static int		s_nLureIdx[LURE_MAX_NPC];
static DWORD	s_dwLureID[LURE_MAX_NPC];
static int		s_nLureCount = 0;
static int		s_nLureStep = 0;
static DWORD	s_dwLureStepTick = 0;
static DWORD	s_dwLureStartTick = 0;
static DWORD	s_dwLureEndTick = 0;

static int LureRange(int nRange)
{
	if (nRange <= 0) return 500;
	if (nRange < 100) return 100;
	if (nRange > 2000) return 2000;
	return nRange;
}

void CVLMove::LureStop()
{
	if (s_nLureState != lure_none) s_dwLureEndTick = GetTickCount();
	s_nLureState = lure_none;
	s_nLureCount = 0;
}

void CVLMove::LureStart(int nAnchorX, int nAnchorY)
{
	if (!m_bLureCreep || s_nLureState != lure_none) return;
	if (!theAttack.m_bAutoHit || !P_PLAYER->m_FightMode || theAttack.m_nEnemyIdx) return;
	if (m_bFollow && m_nLeaderIdx) return;	// Dang theo sau nguoi khac
	DWORD dwNow = GetTickCount();
	if (s_dwLureEndTick && dwNow - s_dwLureEndTick < LURE_COOLDOWN) return;

	// Quai danh duoc (cung bo loc voi tim quai: con song, khong phai NPC ma / quai lag / ngoai vung train...)
	KAliveNpc::RefreshNpcs();
	int nRange = LureRange(m_nLureRange);
	int nIdx[LURE_MAX_NPC], nCount = 0;
	for (int i = 1; i < MAX_NPC && nCount < LURE_MAX_NPC; i++)
	{
		KAliveNpc& npc = theAttack.m_AliveNpc[i];
		if (!npc.m_dwID) continue;
		if (theAttack.m_bAvoidBoss && npc.m_nGoldType == 16) continue;
		if (__distance(nAnchorX, nAnchorY, npc.m_nPosX, npc.m_nPosY) > nRange) continue;
		nIdx[nCount++] = i;
	}
	if (nCount < 2) return;	// 0-1 con: danh luon

	// Thu tu chay: tu diem neo, moi lan lay con gan diem dang dung nhat
	int nX = nAnchorX, nY = nAnchorY;
	s_nLureCount = 0;
	while (s_nLureCount < nCount)
	{
		int nBest = -1, dBest = MAX_INTNUM;
		for (int k = 0; k < nCount; k++)
		{
			if (nIdx[k] < 0) continue;
			KAliveNpc& npc = theAttack.m_AliveNpc[nIdx[k]];
			int d = __distance(nX, nY, npc.m_nPosX, npc.m_nPosY);
			if (d < dBest) { dBest = d; nBest = k; }
		}
		if (nBest < 0) break;
		KAliveNpc& npc = theAttack.m_AliveNpc[nIdx[nBest]];
		s_nLureIdx[s_nLureCount] = nIdx[nBest];
		s_dwLureID[s_nLureCount] = npc.m_dwID;
		s_nLureCount++;
		nX = npc.m_nPosX; nY = npc.m_nPosY;
		nIdx[nBest] = -1;
	}
	s_ptLureAnchor.x = nAnchorX;
	s_ptLureAnchor.y = nAnchorY;
	s_nLureMap = *g_pMapID;
	s_nLureStep = 0;
	s_dwLureStepTick = dwNow;
	s_dwLureStartTick = dwNow;
	s_nLureState = lure_visit;
	if (PEOPLE_IDX > 0 && PEOPLE_IDX < MAX_NPC && g_pNpcArray[PEOPLE_IDX]->m_Kind == kind_normal)
		theAttack.SetPeopleIdx(0);
	SystemMessage(0, "<color=yellow>Quay quai: %d con", s_nLureCount);
}

BOOL CVLMove::LureBreathe()
{
	if (s_nLureState == lure_none) return FALSE;
	DWORD dwNow = GetTickCount();
	DWORD dwMaxTime = LURE_NPC_TIMEOUT * s_nLureCount + LURE_BACK_TIMEOUT + 2000;
	if (!m_bLureCreep || !P_PLAYER->m_FightMode || P_PLAYER->m_CurrentLife <= 0 || *g_pMapID != s_nLureMap ||
		theAttack.m_nEnemyIdx || dwNow - s_dwLureStartTick > dwMaxTime)
	{
		LureStop();
		return FALSE;
	}
	// Khong de game tu chay theo danh quai (PEOPLE_IDX)
	if (PEOPLE_IDX > 0 && PEOPLE_IDX < MAX_NPC && g_pNpcArray[PEOPLE_IDX]->m_Kind == kind_normal)
		theAttack.SetPeopleIdx(0);

	int nPlayerX, nPlayerY;
	P_PLAYER->GetMapPos(&nPlayerX, &nPlayerY);
	while (s_nLureState == lure_visit)
	{
		if (s_nLureStep >= s_nLureCount)
		{
			s_nLureState = lure_back;
			s_dwLureStepTick = dwNow;
			break;
		}
		int i = s_nLureIdx[s_nLureStep];
		KNpc* pNpc = g_pNpcArray[i];
		BOOL bNext = !pNpc->IsExist() || pNpc->m_dwID != s_dwLureID[s_nLureStep] || pNpc->m_CurrentLife <= 0 ||
			dwNow - s_dwLureStepTick > LURE_NPC_TIMEOUT;
		int nX = 0, nY = 0;
		if (!bNext)
		{
			pNpc->GetMapPos(&nX, &nY);
			if (__distance(nPlayerX, nPlayerY, nX, nY) <= LURE_TOUCH) bNext = TRUE;
		}
		if (bNext)
		{
			s_nLureStep++;
			s_dwLureStepTick = dwNow;
			continue;
		}
		g_pCurPlayer->MoveTo(nX, nY);
		return TRUE;
	}
	// Ve diem neo
	if (__distance(nPlayerX, nPlayerY, s_ptLureAnchor.x, s_ptLureAnchor.y) <= LURE_ANCHOR_NEAR ||
		dwNow - s_dwLureStepTick > LURE_BACK_TIMEOUT)
	{
		LureStop();
		return FALSE;
	}
	g_pCurPlayer->MoveTo(s_ptLureAnchor.x, s_ptLureAnchor.y);
	return TRUE;
}
