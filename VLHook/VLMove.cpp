#include "stdafx.h"
#include "../GameDef.h"
#include "DTextFile.h"
#include "GameBase.h"
#include "JxCore.h"
#include "S3Client.h"
#include "VLClasses.h"
#include "VLDebug.h"
#include "VLObstacle.h"
#include "BufLog.h"
#include "../CrashHandler.h"


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

// Debug log Di chuyen -> <thu muc auto>\Logs\DiChuyen.log: moi vong CVLMove::Breathe (200ms) ghi trang thai di chuyen,
// diem dich + ly do toi do, khoang cach thang / quang duong (ne vat can, MapPath) toi dich, diem gan nhat cua
// "Danh sach toa do luyen cong" (m_Nodes); cac su kien (chon diem, doi trang thai, ket duong, GoTrain...) ghi dong rieng.
// 1 = bat, 0 = tat (khong bien dich code log)
#if DebugMoveMode
#define MV_LOG_MAXSIZE		(16 * 1024 * 1024)	// Qua 16MB => DiChuyen_bak.log
#define MV_BUF_SIZE			(128 * 1024)
#define MV_BUF_FLUSH		(64 * 1024)
#define MV_FLUSH_TIME		30000

static char		s_MVBuf[MV_BUF_SIZE];
static KBufLog	s_MVFile = { "DiChuyen", MV_LOG_MAXSIZE, MV_BUF_SIZE, MV_BUF_FLUSH, MV_FLUSH_TIME, s_MVBuf, 0, 0 };

static const char*	s_pszMVExit = NULL;		// Diem thoat Breathe vong nay (MV_EXIT)
static char			s_szMVExitBuf[300];
static BOOL			s_bMVDest = FALSE;		// Vong nay co diem dich
static int			s_nMVDestX = 0, s_nMVDestY = 0;
static char			s_szMVDest[160];		// Dich la gi (danh sach nao, diem thu may)
static const char*	s_pszMVAct = NULL;		// Cach di vong nay (MapPath / GoTo / chay thang ...)
static char			s_szMVNodeWhy[240] = "";	// Ly do chon diem dang toi cua Breathe (m_nCurNode)
static char			s_szMVGoWhy[240] = "";		// Ly do chon diem dang toi cua danh sach di den noi luyen cong (m_nGoNode)
static char			s_szMVCmd[300];			// Lenh MoveTo/GoTo theMove ra vong nay (DiChuyenNoteMove)
static int			s_nMVCmd = 0;
static char			s_szMVOther[300] = "";	// Lenh MoveTo/GoTo gan nhat tu phan khac (khong phai theMove.Breathe)
static DWORD		s_dwMVOtherTick = 0;
static char			s_szMVLast[2400] = "";	// Dong VONG truoc (bo trung)
static int			s_nMVRepeat = 0;
// Trang thai vong truoc (ghi su kien khi doi)
static int			s_nMVPrevStatus = -99, s_nMVPrevNode = -99, s_nMVPrevGoNode = -99, s_nMVPrevMap = 0;
static BOOL			s_bMVPrevGoTrain = -1, s_bMVPrevFinding = -1, s_bMVPrevTravel = -1;

static const char* MVStatus(int n)
{
	static const char* s_szName[] = { "idle", "movefirst", "move", "stand", "moveattack", "attack", "pickup" };
	return (n >= 0 && n < (int)(sizeof(s_szName) / sizeof(s_szName[0]))) ? s_szName[n] : "?";
}

// Dong "[hh:mm:ss.ms] msg" (truoc do ghi so vong VONG lap lai neu co)
static void MVLog(const char* fmt, ...)
{
	char szMsg[2400];
	va_list va;
	va_start(va, fmt);
	_vsnprintf(szMsg, sizeof(szMsg) - 1, fmt, va);
	va_end(va);
	szMsg[sizeof(szMsg) - 1] = 0;
	if (s_nMVRepeat)
	{
		char sz[64];
		sprintf(sz, "	  (giong tren x%d vong)", s_nMVRepeat);
		BLAppend(&s_MVFile, sz);
		s_nMVRepeat = 0;
	}
	SYSTEMTIME st;
	GetLocalTime(&st);
	char szLine[2500];
	_snprintf(szLine, sizeof(szLine) - 1, "[%02d:%02d:%02d.%03d] %s", st.wHour, st.wMinute, st.wSecond, st.wMilliseconds, szMsg);
	szLine[sizeof(szLine) - 1] = 0;
	BLAppend(&s_MVFile, szLine);
}

static void MVExitF(const char* fmt, ...)
{
	va_list va;
	va_start(va, fmt);
	_vsnprintf(s_szMVExitBuf, sizeof(s_szMVExitBuf) - 1, fmt, va);
	va_end(va);
	s_szMVExitBuf[sizeof(s_szMVExitBuf) - 1] = 0;
	s_pszMVExit = s_szMVExitBuf;
}

static void MVDest(int x, int y, const char* fmt, ...)
{
	s_bMVDest = (x || y);
	s_nMVDestX = x; s_nMVDestY = y;
	va_list va;
	va_start(va, fmt);
	_vsnprintf(s_szMVDest, sizeof(s_szMVDest) - 1, fmt, va);
	va_end(va);
	s_szMVDest[sizeof(s_szMVDest) - 1] = 0;
}

// Ly do chon diem: luu (in o moi dong VONG) + ghi su kien ngay
static void MVWhy(char* szBuf, int nBuf, const char* fmt, ...)
{
	va_list va;
	va_start(va, fmt);
	_vsnprintf(szBuf, nBuf - 1, fmt, va);
	va_end(va);
	szBuf[nBuf - 1] = 0;
	MVLog(">> %s", szBuf);
}

// Quang duong toi (tx,ty) tu (px,py): "thang d | ne vat can d | MapPath ..." (bMapPathWalk: ca duong PathWalkTo dang di)
static int MVDistText(char* szOut, int nOut, int px, int py, int tx, int ty, BOOL bMapPathWalk)
{
	int dLine = __distance(px, py, tx, ty);
	int dWalk = ObsWalkDist(px, py, tx, ty);
	char szWalk[48], szPath[160];
	if (dWalk < 0) strcpy(szWalk, "KHONG TOI DUOC");
	else if (dWalk == dLine) sprintf(szWalk, "%d (thang duoc / ngoai vung nap)", dWalk);
	else sprintf(szWalk, "%d", dWalk);
	int nNodes = 0;
	int dPlan = PathRouteLen(px, py, tx, ty, &nNodes);
	if (dPlan == -1) strcpy(szPath, "map khong co MapPath");
	else if (dPlan == -2) strcpy(szPath, "2 diem MapPath khong noi nhau");
	else sprintf(szPath, "%d qua %d diem", dPlan, nNodes);
	if (bMapPathWalk)
	{
		int nLeft = 0;
		POINT ptNext = { 0, 0 };
		int dRemain = PathWalkRemain(tx, ty, &nLeft, &ptNext);
		if (dRemain >= 0)
		{
			int n = (int)strlen(szPath);
			_snprintf(szPath + n, sizeof(szPath) - 1 - n, ", DANG DI con %d qua %d diem (toi %d/%d)", dRemain, nLeft,
				ptNext.x >> 8, ptNext.y >> 9);
			szPath[sizeof(szPath) - 1] = 0;
		}
	}
	_snprintf(szOut, nOut - 1, "thang %d | duong ne vat can %s | duong MapPath %s", dLine, szWalk, szPath);
	szOut[nOut - 1] = 0;
	return dLine;
}

static int LureStateNow();	// Cuoi file (quay quai)

static void MVLoopBegin()
{
	s_pszMVExit = NULL;
	s_bMVDest = FALSE;
	s_szMVDest[0] = 0;
	s_pszMVAct = NULL;
	s_szMVCmd[0] = 0;
	s_nMVCmd = 0;
}

static void MVLoopEnd(CVLMove* p)
{
	BOOL bTravel = PathTravelActive();
	if (!theAttack.m_bAutoHit && !p->m_bFollow && !p->m_bGoTrain && !bTravel) return;	// Khong tu di: khong ghi
	int x, y;
	P_PLAYER->GetMapPos(&x, &y);

	// Su kien: doi map / doi trang thai / doi diem / bat tat co
	if (*g_pMapID != s_nMVPrevMap)
	{
		if (s_nMVPrevMap) MVLog("== DOI MAP: %d -> %d, vi tri %d/%d", s_nMVPrevMap, *g_pMapID, x >> 8, y >> 9);
		s_nMVPrevMap = *g_pMapID;
	}
	if (p->m_nStatus != s_nMVPrevStatus)
	{
		if (s_nMVPrevStatus != -99) MVLog("TRANG THAI: %s -> %s", MVStatus(s_nMVPrevStatus), MVStatus(p->m_nStatus));
		s_nMVPrevStatus = p->m_nStatus;
	}
	if (p->m_nCurNode != s_nMVPrevNode)
	{
		if (s_nMVPrevNode != -99) MVLog("DOI DIEM (m_nCurNode): #%d -> #%d (pre #%d)", s_nMVPrevNode, p->m_nCurNode, p->m_nPreNode);
		s_nMVPrevNode = p->m_nCurNode;
	}
	if (p->m_nGoNode != s_nMVPrevGoNode)
	{
		if (s_nMVPrevGoNode != -99) MVLog("DOI DIEM DS di den noi luyen cong (m_nGoNode): #%d -> #%d (da toi #%d)", s_nMVPrevGoNode, p->m_nGoNode, p->m_nGoDone);
		s_nMVPrevGoNode = p->m_nGoNode;
	}
	if (p->m_bGoTrain != s_bMVPrevGoTrain)
	{
		if (s_bMVPrevGoTrain != -1) MVLog("CO GoTrain (di toi bai train): %d -> %d (map %d)", s_bMVPrevGoTrain, p->m_bGoTrain, p->m_nGoTrainMap);
		s_bMVPrevGoTrain = p->m_bGoTrain;
	}
	if (p->m_bFindingWay != s_bMVPrevFinding)
	{
		if (s_bMVPrevFinding != -1) MVLog("CO m_bFindingWay (tim duong toi map train): %d -> %d", s_bMVPrevFinding, p->m_bFindingWay);
		s_bMVPrevFinding = p->m_bFindingWay;
	}
	if (bTravel != s_bMVPrevTravel)
	{
		if (s_bMVPrevTravel != -1) MVLog("PathTravel (tu di theo MapPath qua map): %d -> %d", s_bMVPrevTravel, bTravel);
		s_bMVPrevTravel = bTravel;
	}

	char szLine[2400];
	int n = 0;
#define MVCAT(...)	do { if (n < (int)sizeof(szLine) - 1) { int k = _snprintf(szLine + n, sizeof(szLine) - 1 - n, __VA_ARGS__); n = (k < 0) ? (int)sizeof(szLine) - 1 : n + k; } } while (0)
	MVCAT("VONG | %s | map %d vi tri %d/%d (%d,%d)", s_pszMVExit ? s_pszMVExit : "(het ham)", *g_pMapID, x >> 8, y >> 9, x, y);
	MVCAT(" | che do %s%s%s%s st %s node #%d pre #%d",
		p->m_bMoveByMapYS ? "BanDo " : "", p->m_bAroundPointPK ? "QuanhDiem " : "", p->m_bRouteMove ? "DanhSach " : "",
		p->m_bFollow ? "TheoSau " : "", MVStatus(p->m_nStatus), p->m_nCurNode, p->m_nPreNode);
	if (s_bMVDest)
	{
		char szDist[300];
		MVDistText(szDist, sizeof(szDist), x, y, s_nMVDestX, s_nMVDestY, TRUE);
		MVCAT(" | DICH %s %d/%d (%d,%d) | %s", s_szMVDest, s_nMVDestX >> 8, s_nMVDestY >> 9, s_nMVDestX, s_nMVDestY, szDist);
		if (s_pszMVAct) MVCAT(" | cach di: %s", s_pszMVAct);
	}
	if (s_szMVNodeWhy[0] && p->m_nCurNode >= 0) MVCAT(" | ly do diem: %s", s_szMVNodeWhy);
	if (p->m_bGoTrain && s_szMVGoWhy[0]) MVCAT(" | ly do diem di den: %s", s_szMVGoWhy);

	// Diem gan nhat cua Danh sach toa do luyen cong (m_Nodes)
	int nCount = 0;
	while (nCount < MAX_NODES && p->m_Nodes[nCount].x && p->m_Nodes[nCount].y) nCount++;
	if (!nCount) MVCAT(" | DS luyen cong: trong");
	else if (p->m_nNodesMapID && p->m_nNodesMapID != *g_pMapID) MVCAT(" | DS luyen cong (%d diem) o map %d", nCount, p->m_nNodesMapID);
	else
	{
		int nNear = 0, dNear = MAX_INTNUM;
		for (int i = 0; i < nCount; i++)
		{
			int d = __distance(x, y, p->m_Nodes[i].x, p->m_Nodes[i].y);
			if (d < dNear) { dNear = d; nNear = i; }
		}
		char szDist[300];
		MVDistText(szDist, sizeof(szDist), x, y, p->m_Nodes[nNear].x, p->m_Nodes[nNear].y, FALSE);
		MVCAT(" | DS luyen cong gan nhat #%d/%d %d/%d: %s", nNear, nCount, p->m_Nodes[nNear].x >> 8, p->m_Nodes[nNear].y >> 9, szDist);
	}

	MVCAT(" | lag %d dZcu %d tong %d", p->m_nLagCount, p->m_nLastdZ == MAX_INTNUM ? -1 : p->m_nLastdZ, p->m_nTotalLagCount);
	if (s_nJumpStep) MVCAT(" khinhcong buoc %d", s_nJumpStep);
	MVCAT(" | lenh: %s", s_nMVCmd ? s_szMVCmd : "khong");
	if (s_szMVOther[0] && GetTickCount() - s_dwMVOtherTick < 2000)
		MVCAT(" | lenh phan khac %.1fs truoc: %s", (GetTickCount() - s_dwMVOtherTick) / 1000.0, s_szMVOther);
	MVCAT(" | nv doing %d ngua %d | danh auto %d muctieu #%d tiepcan %d boss %d kethu %d quay %d",
		P_PLAYER->m_Doing, P_PLAYER->m_bRideHorse, theAttack.m_bAutoHit, PEOPLE_IDX, theAttack.IsApproaching(),
		theAttack.m_nBossWait, theAttack.m_nEnemyIdx, LureStateNow());
	if (p->m_bGoTrain) MVCAT(" | GoTrain map %d go #%d done #%d %ds", p->m_nGoTrainMap, p->m_nGoNode, p->m_nGoDone,
		p->m_dwGoTrainTick ? (int)((GetTickCount() - p->m_dwGoTrainTick) / 1000) : 0);
	if (p->m_bFollow) MVCAT(" | theo #%d", p->m_nLeaderIdx);
#undef MVCAT
	szLine[sizeof(szLine) - 1] = 0;

	if (!strcmp(szLine, s_szMVLast)) { s_nMVRepeat++; return; }	// Giong vong truoc: chi dem
	strcpy(s_szMVLast, szLine);
	MVLog("%s", szLine);
}

struct KMVLoop
{
	CVLMove* p;
	KMVLoop(CVLMove* pMove) : p(pMove) { MVLoopBegin(); }
	~KMVLoop() { MVLoopEnd(p); }
};

#define MV_EXIT(s)			(s_pszMVExit = (s))
#define MV_EXITF(...)		MVExitF(__VA_ARGS__)
#define MV_DEST(x, y, ...)	MVDest((x), (y), __VA_ARGS__)
#define MV_ACT(s)			(s_pszMVAct = (s))
#define MV_ACT_IF(s)		(s_pszMVAct ? (void)0 : (void)(s_pszMVAct = (s)))
#define MV_NODEWHY(...)		MVWhy(s_szMVNodeWhy, sizeof(s_szMVNodeWhy), __VA_ARGS__)
#define MV_GOWHY(...)		MVWhy(s_szMVGoWhy, sizeof(s_szMVGoWhy), __VA_ARGS__)
#define MV_LOG(...)			MVLog(__VA_ARGS__)
#else
#define MV_EXIT(s)			((void)0)
#define MV_EXITF(...)		((void)0)
#define MV_DEST(x, y, ...)	((void)0)
#define MV_ACT(s)			((void)0)
#define MV_ACT_IF(s)		((void)0)
#define MV_NODEWHY(...)		((void)0)
#define MV_GOWHY(...)		((void)0)
#define MV_LOG(...)			((void)0)
#endif

// Goi tu KPlayer::MoveTo / GoTo (JxCore.cpp): lenh di chuyen thuc te. theMove.Breathe => in vao dong VONG,
// phan khac (danh quai, nhat do, MapPath qua map...) => ghi lai lenh gan nhat
void DiChuyenNoteMove(void* pCaller, int nX, int nY, const char* szWhat)
{
#if DebugMoveMode
	const char* szStage = g_pszCrashStage ? g_pszCrashStage : "?";
	int x, y;
	P_PLAYER->GetMapPos(&x, &y);
	if (!strcmp(szStage, "theMove.Breathe"))
	{
		int n = (int)strlen(s_szMVCmd);
		if (n < (int)sizeof(s_szMVCmd) - 64)
			_snprintf(s_szMVCmd + n, sizeof(s_szMVCmd) - 1 - n, "%s%s(%d/%d) cach %d", n ? ", " : "", szWhat, nX >> 8, nY >> 9,
				__distance(x, y, nX, nY));
		s_szMVCmd[sizeof(s_szMVCmd) - 1] = 0;
		s_nMVCmd++;
		return;
	}
	HMODULE hMod = NULL;
	GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT, (LPCSTR)pCaller, &hMod);
	_snprintf(s_szMVOther, sizeof(s_szMVOther) - 1, "%s(%d/%d) cach %d tu %s VLHook+0x%X", szWhat, nX >> 8, nY >> 9,
		__distance(x, y, nX, nY), szStage, (DWORD)((BYTE*)pCaller - (BYTE*)hMod));
	s_szMVOther[sizeof(s_szMVOther) - 1] = 0;
	s_dwMVOtherTick = GetTickCount();
#endif
}

void DiChuyenLogFlush()
{
#if DebugMoveMode
	BLFlush(&s_MVFile);
#endif
}


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
#if DebugMoveMode
	KMVLoop mvLoop(this);	// Cuoi vong (moi return) ghi dong VONG vao DiChuyen.log
#endif
	if (theMain.m_bPause || theAttack.m_nEnemyIdx)
	{
		MV_EXITF("dung: %s", theMain.m_bPause ? "auto dang tam dung" : "dang danh ke thu (theAttack.m_nEnemyIdx)");
		return;
	}
	if (PathTravelActive())	// Dang tu di theo MapPath
	{
#if DebugMoveMode
		char szInfo[200] = "";
		PathTravelInfo(szInfo, sizeof(szInfo));
		MV_EXITF("PathTravel dang tu di theo MapPath qua map: %s", szInfo);
#endif
		return;
	}
	if (DebugQuestHoldMove()) { MV_EXIT("DEBUG Nhiem vu giu di chuyen (DebugQuestHoldMove)"); return; }	// DEBUG > Nhiem vu: Buoc 3 dung tai map / Buoc 4 tu di vong theo MapPath

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
		MV_EXIT("khong o trang thai chien dau (trong thanh) - reset diem");
		return;
	}
	if (!bInFightMode)	// return from city
	{
		if (nWaitTime++ < 3) { MV_EXIT("vua ra khoi thanh: cho 0.5s"); return; } // wait for 0.5s
		nWaitTime = 0;
		bInFightMode = TRUE;
	}
	if (LagJumpBreathe()) { MV_EXITF("dang dung khinh cong vuot ket (buoc %d, toi %d/%d)", s_nJumpStep, s_nJumpX >> 8, s_nJumpY >> 9); return; }	// Dang dung khinh cong vuot cho ket

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

	if (IsFlagMoving()) { MV_EXITF("dang chay toi co (click ban do) %d/%d", *((int*)FLAG_X_OFFST) >> 8, *((int*)FLAG_Y_OFFST) >> 9); return; }
	if (LoadingMapPath()) { MV_EXIT("dang nap MapPath / ban do"); return; }

	if (m_bFollow)
		if (DoFollow())
		{	// Da thay nguoi can theo: bo tu di toi bai train (GoTrainBreathe khong con chay => m_bGoTrain ket lai,
			// theAttack.Breathe coi la "dang tim bai train" => khong danh quai, khong xuong ngua cho toi khi vao lai game)
			if (m_bGoTrain) GoTrainStop();
			if (PathTravelActive() && !theQuest.m_bAutoDoQuest && !theMain.m_nWcbStep) PathTravelStop();
			m_bFindingWay = 0; m_nPreNode = -1; m_nCurNode = -1;
			MV_EXITF("Theo sau: dang theo nguoi #%d (DoFollow)", m_nLeaderIdx);
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
	m_bFindingWay = 0;

	if (!m_bMoveByMapYS && !m_bAroundPointPK && !m_bRouteMove) { m_nStatus = status_idle; MV_EXIT("khong chon che do di chuyen (BanDo/QuanhDiem/DanhSach)"); return; }

	if (theAttack.m_nBossWait) { MV_EXITF("cho boss (m_nBossWait %d)", theAttack.m_nBossWait); return; }

	if (!theAttack.m_bAutoHit)
	{
		m_nStatus = status_idle;
		if (!m_bFollow) { MV_EXIT("khong bat danh quai tu dong"); return; }
	}
	P_PLAYER->GetMapPos(&nPlayerX, &nPlayerY);
	if (m_bMoveByMapYS)
	{
		if (m_nMapPoints <= 0) { MV_EXIT("BanDo: map chua co MapPath (Maps\\Path)"); return; }
		if (m_nCurNode < 0) { GetCurNode(); m_nStatus = status_movefirst; }
		if (m_nCurNode < 0) { MV_EXIT("BanDo: khong chon duoc diem"); return; }
		nDestX = MapPoints[m_nCurNode].x;
		nDestY = MapPoints[m_nCurNode].y;
		MV_DEST(nDestX, nDestY, "BanDo diem #%d/%d", m_nCurNode, m_nMapPoints);
	}
	else if (m_bAroundPointPK)
	{
		if (!m_nPointX && !m_nPointY) { MV_EXIT("QuanhDiem: chua ghi toa do"); return; }
		nDestX = m_nPointX;
		nDestY = m_nPointY;
		MV_DEST(nDestX, nDestY, "QuanhDiem%s", m_bMoveWithPK ? " (toa do PK bang)" : "");
	}
	else //m_bRouteMove
	{
		if (m_nNodesMapID && *g_pMapID != m_nNodesMapID)
		{	// Danh sach toa do thuoc ban do khac (vd "Tro lai dia diem cu" dua sai map): tu di theo MapPath, 30s thu 1 lan
			static DWORD dwTryTick = 0;
			if (!theMap.m_nTrainMapID && GetTickCount() - dwTryTick > 30000 && theMain.HaveTownPortal())
			{
				dwTryTick = GetTickCount();
				if (PathTravelStart(m_nNodesMapID, FALSE)) { MV_EXITF("DS luyen cong o map %d: bat PathTravel toi map do", m_nNodesMapID); return; }
			}
			m_nStatus = status_idle;
			m_nCurNode = -1;
			MV_EXITF("DS luyen cong thuoc map %d, dang o map %d: dung", m_nNodesMapID, *g_pMapID);
			return;
		}
		if (m_nCurNode < 0) { GetCurNode(); m_nStatus = status_movefirst; }
		if (m_nCurNode < 0) { MV_EXIT("DS luyen cong: khong chon duoc diem (trong?)"); return; }
		if (!m_Nodes[m_nCurNode].x || !m_Nodes[m_nCurNode].y)
		{
			MV_EXITF("DS luyen cong: diem #%d rong", m_nCurNode);
			return;
		}
		nDestX = m_Nodes[m_nCurNode].x;
		nDestY = m_Nodes[m_nCurNode].y;
		MV_DEST(nDestX, nDestY, "DS luyen cong diem #%d", m_nCurNode);
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
		int nGo = AutoGoTo(nDestX, nDestY);
		if (nGo > 0)
		{
			MV_ACT("GoTo - game tu tim duong (AutoGoTo)");
			MV_EXIT("dang di toi dich bang GoTo");
			m_nLagCount = 0;
			m_nLastdZ = MAX_INTNUM;
			return;
		}
		MV_ACT(nGo == 0 ? "da gan (<=150): MoveTo not doan cuoi" : "AutoGoTo ket (GoTo khong tien) -> MoveTo chay thang");
	}
	else
	{
		if (dZ > 1000 || (m_bMoveByMapYS && dZ > AUTO_GOTO_NEAR)) { ptFar.x = nDestX; ptFar.y = nDestY; }	// BanDo: luon theo canh MapPath
		if (dZ <= AUTO_GOTO_NEAR) ptFar.x = ptFar.y = 0;
		if (ptFar.x == nDestX && ptFar.y == nDestY)
		{
			int nPW = PathWalkTo(nDestX, nDestY);
			int nGo = nPW > 0 ? 1 : AutoGoTo(nDestX, nDestY);
			if (nGo > 0)
			{
				MV_ACT(nPW > 0 ? "MapPath (PathWalkTo) - dich xa (>1000 luc chon) / BanDo" :
					nPW == 0 ? "GoTo (het duong MapPath)" : "GoTo (map khong co MapPath / khong noi)");
				MV_EXIT("dang di toi dich (xa)");
				m_nLagCount = 0;
				m_nLastdZ = MAX_INTNUM;
				return;
			}
			MV_ACT(nGo == 0 ? "xa: MapPath het, da gan (<=150) -> MoveTo" : "xa: MapPath + GoTo deu khong di duoc -> MoveTo chay thang");
		}
	}
	if (dZ < 64)
	{
		MV_EXITF("toi dich (<64) luc trang thai %s", MVStatus(m_nStatus));
		m_nLagCount = 0;
		m_nTotalLagCount = 0;
		switch (m_nStatus)
		{
		case status_movefirst:
		case status_move:
			MV_LOG("TOI DICH %d/%d: dung lai (stand), bat dau quay quai / danh", nDestX >> 8, nDestY >> 9);
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
			if (IsLagMoving(nPlayerX, nPlayerY, WalkDist(nPlayerX, nPlayerY, nDestX, nDestY, dZ), nDestX, nDestY))
			{
				MV_EXIT("ket duong (IsLagMoving) - chay huong khac / khinh cong / TDP");
				return;
			}
		}
		MV_ACT_IF("MoveTo chay thang (dich gan <=1000 luc chon)");
		MV_EXIT("MoveTo toi dich");
		g_pCurPlayer->MoveTo(nDestX, nDestY);
	}
	else
	{
		MV_EXITF("trang thai %s: dung tai cho (khong di)", MVStatus(m_nStatus));
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
		MV_NODEWHY("GetCurNode: QuanhDiem -> diem 0");
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
#if DebugMoveMode
		if (m_nCurNode >= 0)
			MV_NODEWHY("GetCurNode: chon %s #%d/%d (%d/%d) - %s %d, thang %d", m_bMoveByMapYS ? "BanDo" : "DS luyen cong", m_nCurNode, nCount,
				pPts[m_nCurNode].x >> 8, pPts[m_nCurNode].y >> 9,
				nPass == 0 ? "gan nhat theo duong di bo ne vat can, di bo" : "KHONG diem nao toi duoc bang di bo -> gan nhat duong thang",
				dMin, __distance(nPlayerX, nPlayerY, pPts[m_nCurNode].x, pPts[m_nCurNode].y));
#endif
	}
	if (m_nCurNode < 0) MV_NODEWHY("GetCurNode: khong co diem (%d diem)", nCount);
}

void CVLMove::GetNextNode()
{
#if DebugMoveMode
	int nMVOld = m_nCurNode;
#endif
	if (m_bMoveByMapYS)
	{	// Diem MapPath: ngau nhien 1 trong 4 diem gan diem vua toi nhat (tru diem truoc); PathWalkTo di theo canh MapPath
		if (m_nCurNode < 0 || m_nCurNode >= m_nMapPoints) { GetCurNode(); return; }
		int nNext[4], dNext[4], nCount = 0;
		for (int i = 0; i < m_nMapPoints; i++)
		{
			if (i == m_nCurNode || i == m_nPreNode) continue;
			int d = __distance(MapPoints[i], MapPoints[m_nCurNode]);
			int k = nCount < 4 ? nCount++ : 4;
			while (k > 0 && dNext[k - 1] > d)
			{
				if (k < 4) { nNext[k] = nNext[k - 1]; dNext[k] = dNext[k - 1]; }
				k--;
			}
			if (k < 4) { nNext[k] = i; dNext[k] = d; }
		}
		if (nCount == 0)
		{
			if (m_nPreNode >= 0) { int n = m_nPreNode; m_nPreNode = m_nCurNode; m_nCurNode = n; }
		}
		else
		{
			m_nPreNode = m_nCurNode;
			m_nCurNode = nNext[P_PLAYER->m_SyncSignal % nCount];	//random
		}
		MV_NODEWHY("GetNextNode BanDo: toi #%d -> #%d (%s)", nMVOld, m_nCurNode,
			nCount ? "ngau nhien 1 trong 4 diem MapPath gan nhat, tru diem truoc" : "chi con diem truoc: quay lai");
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
	MV_NODEWHY("GetNextNode DS luyen cong: toi #%d -> #%d%s", nMVOld, m_nCurNode,
		(m_nCurNode == 0 && nMVOld > 0) ? " (het danh sach, quay ve diem dau)" : " (diem tiep theo)");
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
		if (MapEnterElapsed() < 600) return TRUE;
		nLastMapID = (*g_pMapID);

		m_nMapPoints = PathMapPoints(*g_pMapID, MapPoints, MAX_MAP_POINTS, TRUE);	// Che do Ban do: diem MapPath, bo diem gan cong
		if (m_nCurNode >= m_nMapPoints && m_bMoveByMapYS) { m_nCurNode = -1; m_nPreNode = -1; }
	}

	return 0;
}

BOOL CVLMove::IsNpcInRange(int nNpcX, int nNpcY)
{
	int dZ;
	POINT ptNpc = { nNpcX, nNpcY };

	if (DebugQuestHoldMove()) return TRUE;	// Khong di theo danh sach toa do: khong gioi han quanh diem / duong
	if (m_bMoveByMapYS)
	{
		if (m_nMapPoints <= 0) return TRUE;
		if (m_nCurNode < 0 || m_nCurNode >= m_nMapPoints) return 0;
		dZ = __distance(MapPoints[m_nCurNode].x, MapPoints[m_nCurNode].y, nNpcX, nNpcY);
		if (dZ <= m_nPointRange) return TRUE;
		//m_bAttackWhenMove is always TRUE
		if (m_nPreNode < 0 || m_nPreNode >= m_nMapPoints) return TRUE;
		dZ = __getheight(MapPoints[m_nPreNode], MapPoints[m_nCurNode], ptNpc);
		if (dZ <= m_nPointRange) return TRUE;
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
			if (dZ <= m_nPointRange) return TRUE;
		}
	}
	else
		return TRUE;
	return 0;
}

// Map Da Tau phai co MapPath (Maps\Path\<MapID>_<Ten>.txt) de di vong danh quai
BOOL CVLMove::LoadYeSouMap(int nYSMapID)
{
	static POINT pt[8];
	return nYSMapID > 0 && PathMapPoints(nYSMapID, pt, 8) > 0;
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
		MV_LOG("KET DUONG (IsLagMoving): 8 lan kiem tra khoang cach khong giam (dZ %d), tong ket %d -> chay huong ngau nhien %d/%d, bo diem #%d",
			dZ, m_nTotalLagCount + 1, nDestX >> 8, nDestY >> 9, m_nCurNode);
		m_nLagCount = 0; m_nCurNode = -1;
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
				MV_LOG("KET DUONG: ket qua nhieu lan -> dung TDP ve thanh");
				theMap.m_bNoDriverRecent = TRUE;
				SystemMessage(TV(L"<color=yellow>Kẹt hẳn rồi, về thành lát quay lại."));
				theRecover.OnUseAM();
				return TRUE;
			}
			else if (m_nTotalLagCount > 5 && theRecover.m_bLagUseSkill && LagJumpStart(nPlayerX, nPlayerY, nDestX, nDestY))
			{
				MV_LOG("KET DUONG: nhay khinh cong");
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
		if (m_nGoNode >= nCount) MV_GOWHY("GoNodes bat dau: da o gan bai train (NearTrainArea) -> bo qua DS di den");
		else if (m_nGoDone >= 0) MV_GOWHY("GoNodes bat dau: dang dung sat diem #%d (<=300) -> di tiep tu do", m_nGoNode);
		else MV_GOWHY("GoNodes bat dau: MapPath/GoTo toi diem dau tien #0 (%d diem)", nCount);
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
			MV_GOWHY("GoNodes doi sang #%d: gan nhat co duong toi (di bo %d) < diem dang toi #%d (%d)", nNear, nReach, m_nGoNode,
				nCurReach == MAX_INTNUM ? -1 : nCurReach);
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
	MV_DEST(nDestX, nDestY, "DS di den noi luyen cong #%d/%d", m_nGoNode, nCount);
	MV_EXIT("GoTrain: di theo DS di den noi luyen cong (GoNodesBreathe)");
	if (dZ <= 64)
	{	// Toi diem -> diem tiep theo (moi doan duoc 3 phut)
		MV_GOWHY("GoNodes toi diem #%d -> diem tiep #%d", m_nGoNode, m_nGoNode + 1);
		m_nGoDone = m_nGoNode;
		m_nGoNode++;
		m_nLagCount = 0;
		m_nLastdZ = MAX_INTNUM;
		m_dwGoTrainTick = GetTickCount();
		return TRUE;
	}
	if (m_nGoDone < 0)
	{	// Chua toi diem nao (vua vao map): theo MapPath cua map tu cong vao
		MV_ACT("chua toi diem nao: MapPath, khong duoc thi GoTo");
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
	MV_ACT("MoveTo chay thang theo DS di den");
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
			MV_EXITF("GoTrain: bat PathTravel sang map train %d", nTarget);
			m_dwGoTrainTick = GetTickCount();
			return TRUE;	// PathTravelBreathe di; toi map train thi Breathe goi lai day
		}
		// Khong co lo trinh MapPath toi map train
		char szName[64] = "";
		theMain.GetMapName(nTarget, szName);
		SystemMessage(0, "<color=yellow>Khong co lo trinh MapPath toi map train %s (%d)", szName, nTarget);
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
	MV_DEST(nDestX, nDestY, "GoTrain: diem dau tien cua bai train");
	MV_EXIT("GoTrain: di toi diem dau bai train (GoTrainBreathe)");
	MV_ACT("MapPath, khong duoc thi GoTo");
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
		MV_LOG("AutoGoTo: dich moi %d/%d cach %d", tx >> 8, ty >> 9, d);
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
		MV_LOG("AutoGoTo: lan goi GoTo truoc khong tien them (fail %d/%d, gan nhat %d, con %d)", s_nGoToFails + 1, GOTO_MAX_FAILS,
			s_nGoToBest, d);
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

#if DebugMoveMode
static int LureStateNow() { return s_nLureState; }
#endif
