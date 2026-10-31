#include "stdafx.h"
#include "../GameDef.h"
#include "GameBase.h"
#include "JxCore.h"
#include "VLObstacle.h"

#ifndef MAX_INTNUM
#define MAX_INTNUM	0x7FFFFF	// = VLClasses.h
#endif

// Ne vat can khi di chuyen. Truoc day MoveTo chay thang A -> B, gap tuong/goc la ket (lag).
// Luoi: moi o vat can 32x32 (m_ObstacleInfo) chia 4x4 nut 8x8 Mps; o tam giac (LT/RT/LB/RB) chi chan nua o
// (diem tren duong cheo coi la vat can). Nut sat vat can (8 nut quanh) bi cong them chi phi de duong khong om tuong.
// Tim duong A* (8 huong, khong cat goc), roi lay diem xa nhat tren duong ma tu nhan vat chay thang toi duoc.
// Vung chua nap (ngoai 9 region quanh nhan vat) coi nhu di duoc: dich o xa thi tim duong toi bien vung da nap.

#define OB_NODE			8			// Canh nut (Mps)
#define OB_MAX_W		384
#define OB_MAX_H		512
#define OB_MAX_NODES	(OB_MAX_W * OB_MAX_H)
#define OB_HEAP_MAX		(OB_MAX_NODES * 2)
#define OB_NEAR_COST	6			// Them chi phi cho nut sat vat can
#define OB_NEAR_SKIP	16			// LOS: bo qua kiem tra "sat vat can" trong pham vi nay quanh 2 dau
#define OB_PATH_MAX		1024

enum { nb_block = 1, nb_near = 2 };

BOOL g_bObsAvoid = TRUE;

static BYTE		*s_pGrid = NULL;		// nb_block / nb_near
static int		*s_pG = NULL;			// Chi phi (x10 / 8 Mps)
static int		*s_pPrev = NULL;
static DWORD	*s_pStamp = NULL;		// s_pG/s_pPrev hop le khi = s_dwGen
static DWORD	s_dwGen = 0;
struct OB_HEAP	{ int f, n; };
static OB_HEAP	*s_pHeap = NULL;
static int		s_nHeap = 0;

static int		s_nX0 = 0, s_nY0 = 0, s_nW = 0, s_nH = 0;	// Goc trai-tren luoi (Mps), so nut
static DWORD	s_dwGridKey = 0;		// Tong kiem cac region dang nap (khong doi thi khong dung lai luoi)

// Loang (ObsFlood)
static BOOL		s_bFlood = FALSE;
static POINT	s_ptFlood = { 0, 0 };
static DWORD	s_dwFloodGen = 0;

// Duong vua tim (de ve)
static POINT	s_ptPath[OB_PATH_MAX];
static int		s_nPath = 0;
static POINT	s_ptWay = { 0, 0 };
static DWORD	s_dwPathTick = 0;

//------------------------------------------------------------------ o vat can

int ObsGet(int x, int y)
{
	int nRet = -1;
	ObsForEachRegion([&](int nLeft, int nTop, const long *pObs)
	{
		int dx = x - nLeft, dy = y - nTop;
		if (nRet >= 0 || dx < 0 || dy < 0 || dx >= 16 * 32 || dy >= 32 * 32) return;
		long lInfo = pObs[(dx >> 5) * 32 + (dy >> 5)];
		dx &= 31; dy &= 31;
		nRet = lInfo & 0xF;
		switch ((lInfo >> 4) & 0xF)	// Nhu KScenePlaceRegionC::GetObstacleInfo
		{
		case 2: if (dx + dy > 32) nRet = 0; break;	// LT
		case 3: if (dx < dy) nRet = 0; break;		// RT
		case 4: if (dx > dy) nRet = 0; break;		// LB
		case 5: if (dx + dy < 32) nRet = 0; break;	// RB
		}
	});
	return nRet;
}

// Diem (dx,dy) trong o (0..31) co bi chan khong - duong cheo coi la chan
static BOOL SubBlocked(int nType, int dx, int dy)
{
	switch (nType)
	{
	case 2: return dx + dy <= 32;	// LT
	case 3: return dx >= dy;		// RT
	case 4: return dx <= dy;		// LB
	case 5: return dx + dy >= 32;	// RB
	}
	return TRUE;					// Full / Empty co loai
}

//------------------------------------------------------------------ luoi

static BOOL Alloc()
{
	if (s_pGrid) return TRUE;
	s_pGrid = new BYTE[OB_MAX_NODES];
	s_pG = new int[OB_MAX_NODES];
	s_pPrev = new int[OB_MAX_NODES];
	s_pStamp = new DWORD[OB_MAX_NODES];
	s_pHeap = new OB_HEAP[OB_HEAP_MAX];
	memset(s_pStamp, 0, OB_MAX_NODES * sizeof(DWORD));
	return TRUE;
}

void ObsFree()
{
	delete[] s_pGrid; delete[] s_pG; delete[] s_pPrev; delete[] s_pStamp; delete[] s_pHeap;
	s_pGrid = NULL; s_pG = s_pPrev = NULL; s_pStamp = NULL; s_pHeap = NULL;
	s_dwGridKey = 0;
	s_bFlood = FALSE;
}

static BOOL BuildGrid()
{
	int nL = MAX_INTNUM, nT = MAX_INTNUM, nR = -MAX_INTNUM, nB = -MAX_INTNUM, nRegions = 0;
	DWORD dwKey = 0x9E3779B9;
	ObsForEachRegion([&](int nLeft, int nTop, const long *pObs)
	{
		nRegions++;
		nL = min(nL, nLeft); nT = min(nT, nTop);
		nR = max(nR, nLeft + 16 * 32); nB = max(nB, nTop + 32 * 32);
		dwKey = (dwKey ^ (DWORD)nLeft) * 16777619 ^ (DWORD)nTop ^ (DWORD)(UINT_PTR)pObs;
	});
	if (!nRegions) return FALSE;
	if (dwKey == s_dwGridKey && s_pGrid) return TRUE;
	Alloc();

	s_nX0 = nL; s_nY0 = nT;
	s_nW = min((nR - nL) / OB_NODE, OB_MAX_W);
	s_nH = min((nB - nT) / OB_NODE, OB_MAX_H);
	memset(s_pGrid, 0, s_nW * s_nH);
	const int nSub = 32 / OB_NODE;
	ObsForEachCell([&](int x, int y, long lInfo)
	{
		int nType = (lInfo >> 4) & 0xF;
		int gx = (x - s_nX0) / OB_NODE, gy = (y - s_nY0) / OB_NODE;
		for (int sx = 0; sx < nSub; sx++)
			for (int sy = 0; sy < nSub; sy++)
			{
				int nx = gx + sx, ny = gy + sy;
				if (nx < 0 || ny < 0 || nx >= s_nW || ny >= s_nH) continue;
				if (SubBlocked(nType, sx * OB_NODE + OB_NODE / 2, sy * OB_NODE + OB_NODE / 2))
					s_pGrid[ny * s_nW + nx] = nb_block;
			}
	});
	for (int ny = 0; ny < s_nH; ny++)
		for (int nx = 0; nx < s_nW; nx++)
		{
			if (s_pGrid[ny * s_nW + nx] & nb_block) continue;
			for (int k = 0; k < 9; k++)
			{
				int ax = nx + k % 3 - 1, ay = ny + k / 3 - 1;
				if (ax >= 0 && ay >= 0 && ax < s_nW && ay < s_nH && (s_pGrid[ay * s_nW + ax] & nb_block))
				{
					s_pGrid[ny * s_nW + nx] |= nb_near;
					break;
				}
			}
		}
	s_dwGridKey = dwKey;
	s_bFlood = FALSE;
	return TRUE;
}

static inline BOOL InGrid(int x, int y)
{
	return x >= s_nX0 && y >= s_nY0 && x < s_nX0 + s_nW * OB_NODE && y < s_nY0 + s_nH * OB_NODE;
}
static inline int NodeOf(int x, int y)
{
	int nx = (x - s_nX0) / OB_NODE, ny = (y - s_nY0) / OB_NODE;
	nx = max(0, min(nx, s_nW - 1)); ny = max(0, min(ny, s_nH - 1));
	return ny * s_nW + nx;
}
static inline int NodeX(int n) { return s_nX0 + (n % s_nW) * OB_NODE + OB_NODE / 2; }
static inline int NodeY(int n) { return s_nY0 + (n / s_nW) * OB_NODE + OB_NODE / 2; }

// Diem (x,y) co bi chan tren luoi khong (bNear: ca nut sat vat can)
static inline BOOL Blocked(int x, int y, BOOL bNear)
{
	if (!InGrid(x, y)) return FALSE;
	BYTE b = s_pGrid[NodeOf(x, y)];
	return (b & nb_block) || (bNear && (b & nb_near));
}

// Duong thang di duoc: lay mau moi 4 Mps; xa 2 dau hon OB_NEAR_SKIP thi tranh ca nut sat vat can
static BOOL LineClear(int x1, int y1, int x2, int y2, BOOL bNear)
{
	int dx = x2 - x1, dy = y2 - y1;
	int nLen = (int)sqrt((double)dx * dx + (double)dy * dy);
	int nSteps = nLen / 4 + 1;
	for (int i = 1; i <= nSteps; i++)
	{
		int x = x1 + (int)((__int64)dx * i / nSteps), y = y1 + (int)((__int64)dy * i / nSteps);
		int d = i * nLen / nSteps;
		if (Blocked(x, y, bNear && d > OB_NEAR_SKIP && nLen - d > OB_NEAR_SKIP)) return FALSE;
	}
	return TRUE;
}

BOOL ObsLineClear(int x1, int y1, int x2, int y2)
{
	if (!BuildGrid()) return TRUE;
	return LineClear(x1, y1, x2, y2, FALSE);
}

//------------------------------------------------------------------ A* / Dijkstra

static void HeapPush(int f, int n)
{
	if (s_nHeap >= OB_HEAP_MAX) return;
	int i = s_nHeap++;
	while (i > 0)
	{
		int p = (i - 1) / 2;
		if (s_pHeap[p].f <= f) break;
		s_pHeap[i] = s_pHeap[p];
		i = p;
	}
	s_pHeap[i].f = f; s_pHeap[i].n = n;
}

static OB_HEAP HeapPop()
{
	OB_HEAP top = s_pHeap[0], last = s_pHeap[--s_nHeap];
	int i = 0;
	for (;;)
	{
		int c = i * 2 + 1;
		if (c >= s_nHeap) break;
		if (c + 1 < s_nHeap && s_pHeap[c + 1].f < s_pHeap[c].f) c++;
		if (s_pHeap[c].f >= last.f) break;
		s_pHeap[i] = s_pHeap[c];
		i = c;
	}
	if (s_nHeap > 0) s_pHeap[i] = last;
	return top;
}

static inline int Octile(int a, int b)
{
	int dx = abs(a % s_nW - b % s_nW), dy = abs(a / s_nW - b / s_nW);
	return dx > dy ? dx * 10 + dy * 4 : dy * 10 + dx * 4;
}

// nGoal < 0: loang het (Dijkstra). Tra ve nut dich, hoac nut toi duoc co h nho nhat neu dich khong toi duoc.
// Nut bat dau / dich nam trong o bi chan van duoc di (nhan vat dang dung o do / diem ghi o nua o tam giac)
static int Search(int nStart, int nGoal)
{
	static const int s_dx[8] = { 1, -1, 0, 0, 1, 1, -1, -1 }, s_dy[8] = { 0, 0, 1, -1, 1, -1, 1, -1 };
	if (++s_dwGen == 0) { memset(s_pStamp, 0, OB_MAX_NODES * sizeof(DWORD)); s_dwGen = 1; }
	s_nHeap = 0;
	s_pStamp[nStart] = s_dwGen;
	s_pG[nStart] = 0;
	s_pPrev[nStart] = -1;
	HeapPush(nGoal >= 0 ? Octile(nStart, nGoal) : 0, nStart);
	int nBest = nStart, hBest = nGoal >= 0 ? Octile(nStart, nGoal) : 0;
	while (s_nHeap > 0)
	{
		OB_HEAP h = HeapPop();
		int n = h.n, g = s_pG[n];
		if (h.f - (nGoal >= 0 ? Octile(n, nGoal) : 0) > g) continue;	// Ban cu trong heap
		if (n == nGoal) return n;
		if (nGoal >= 0)
		{
			int hn = Octile(n, nGoal);
			if (hn < hBest) { hBest = hn; nBest = n; }
		}
		int x = n % s_nW, y = n / s_nW;
		for (int k = 0; k < 8; k++)
		{
			int ax = x + s_dx[k], ay = y + s_dy[k];
			if (ax < 0 || ay < 0 || ax >= s_nW || ay >= s_nH) continue;
			int a = ay * s_nW + ax;
			if ((s_pGrid[a] & nb_block) && a != nGoal) continue;
			if (k >= 4 && (((s_pGrid[y * s_nW + ax] | s_pGrid[ay * s_nW + x]) & nb_block)))
				continue;	// Khong cat goc vat can
			int ga = g + (k < 4 ? 10 : 14) + ((s_pGrid[a] & nb_near) ? OB_NEAR_COST : 0);
			if (s_pStamp[a] == s_dwGen && s_pG[a] <= ga) continue;
			s_pStamp[a] = s_dwGen;
			s_pG[a] = ga;
			s_pPrev[a] = n;
			HeapPush(ga + (nGoal >= 0 ? Octile(a, nGoal) : 0), a);
		}
	}
	return nBest;
}

// Diem chan cua duong thang p -> t trong vung da nap (t ngoai vung: diem cat bien)
static void ClipToGrid(int px, int py, int *pX, int *pY)
{
	int tx = *pX, ty = *pY;
	if (InGrid(tx, ty)) return;
	int lo = 0, hi = 1024;	// Chia doi tren doan p -> t
	for (int i = 0; i < 12; i++)
	{
		int m = (lo + hi) / 2;
		if (InGrid(px + (int)((__int64)(tx - px) * m / 1024), py + (int)((__int64)(ty - py) * m / 1024))) lo = m;
		else hi = m;
	}
	*pX = px + (int)((__int64)(tx - px) * lo / 1024);
	*pY = py + (int)((__int64)(ty - py) * lo / 1024);
}

// Tim duong p -> t. Tra ve nut den (dich hoac gan dich nhat), -1 = khong dung duoc luoi / nhan vat ngoai luoi
static int FindPath(int px, int py, int tx, int ty)
{
	if (!g_bObsAvoid || !BuildGrid() || !InGrid(px, py)) return -1;
	ClipToGrid(px, py, &tx, &ty);
	int nGoal = NodeOf(tx, ty);
	// Dich nam trong nut bi chan nhung diem that di duoc (o tam giac): cho phep toi; that su la vat can: tim nut gan nhat
	if ((s_pGrid[nGoal] & nb_block) && ObsGet(tx, ty) > 0)
		nGoal = -2;
	s_bFlood = FALSE;
	if (nGoal == -2)
	{	// Dich trong vat can: A* toi nut gan dich nhat (Search tra ve nut co h nho nhat)
		int nStart = NodeOf(px, py), gx = (tx - s_nX0) / OB_NODE, gy = (ty - s_nY0) / OB_NODE;
		gx = max(0, min(gx, s_nW - 1)); gy = max(0, min(gy, s_nH - 1));
		int nFake = gy * s_nW + gx;
		BYTE bOld = s_pGrid[nFake];
		s_pGrid[nFake] = nb_block;	// Khong toi duoc nut nay tru khi la dich => Search dung o nut sat no
		int n = Search(nStart, nFake);
		s_pGrid[nFake] = bOld;
		return n == nFake ? s_pPrev[n] : n;
	}
	return Search(NodeOf(px, py), nGoal);
}

static int PathDist(int n)
{
	return s_pG[n] * OB_NODE / 10;
}

//------------------------------------------------------------------ API

// Duong thang (khong phai ne) - van luu de ve tuyen dang chay
static void SetStraightPath(int px, int py, int tx, int ty)
{
	s_ptPath[0].x = px; s_ptPath[0].y = py;
	s_ptPath[1].x = tx; s_ptPath[1].y = ty;
	s_nPath = 2;
	s_ptWay.x = tx; s_ptWay.y = ty;
	s_dwPathTick = GetTickCount();
}

BOOL ObsSteer(int px, int py, int *pX, int *pY)
{
	int tx = *pX, ty = *pY;
	if (!g_bObsAvoid || !BuildGrid() || !InGrid(px, py)) return FALSE;
	if (__distance(px, py, tx, ty) <= 32) return FALSE;
	int cx = tx, cy = ty;
	ClipToGrid(px, py, &cx, &cy);
	if (LineClear(px, py, cx, cy, FALSE) && ObsGet(tx, ty) <= 0)	// Chay thang duoc
	{
		SetStraightPath(px, py, tx, ty);
		return FALSE;
	}

	int nEnd = FindPath(px, py, tx, ty);
	if (nEnd < 0) return FALSE;
	int nStart = NodeOf(px, py);
	if (nEnd == nStart) return FALSE;	// Khong nhich duoc - de lenh cu (CVLMove tu xu ly ket)

	// Duong: nStart .. nEnd
	static int s_nNodes[OB_MAX_NODES / 4];
	int nLen = 0;
	for (int n = nEnd; n >= 0 && nLen < (int)(sizeof(s_nNodes) / sizeof(int)); n = s_pPrev[n])
		s_nNodes[nLen++] = n;
	for (int i = 0; i < nLen / 2; i++) { int t = s_nNodes[i]; s_nNodes[i] = s_nNodes[nLen - 1 - i]; s_nNodes[nLen - 1 - i] = t; }

	// Diem xa nhat chay thang toi duoc, toi da ~1.5 x MAX_MOVE_DISTANCE theo duong. Uu tien duong khong om tuong;
	// loi hep (ca duong sat vat can) thi chi can khong cat vat can - neu khong moi lenh MoveTo chi nhich vai Mps
	int nWay = min(1, nLen - 1);
	for (int nPass = 0; nPass < 2; nPass++)
	{
		int nFar = 0;
		for (int i = 1; i < nLen; i++)
		{
			if (PathDist(s_nNodes[i]) > MAX_MOVE_DISTANCE * 3 / 2) break;
			if (!LineClear(px, py, NodeX(s_nNodes[i]), NodeY(s_nNodes[i]), nPass == 0)) { if (i > 3) break; continue; }
			nFar = i;
		}
		if (nFar) nWay = nFar;
		if (nWay == nLen - 1 || PathDist(s_nNodes[nWay]) >= 96) break;
	}
	int wx = NodeX(s_nNodes[nWay]), wy = NodeY(s_nNodes[nWay]);
	if (nWay == nLen - 1 && nEnd == NodeOf(tx, ty) && InGrid(tx, ty) && LineClear(px, py, tx, ty, FALSE))
		{ wx = tx; wy = ty; }	// Diem cuoi chinh la dich

	// Luu de ve (thua bot neu dai)
	int nStep = nLen / OB_PATH_MAX + 1;
	s_nPath = 0;
	for (int i = 0; i < nLen && s_nPath < OB_PATH_MAX; i += nStep)
		{ s_ptPath[s_nPath].x = NodeX(s_nNodes[i]); s_ptPath[s_nPath].y = NodeY(s_nNodes[i]); s_nPath++; }
	if (s_nPath < OB_PATH_MAX) { s_ptPath[s_nPath].x = NodeX(nEnd); s_ptPath[s_nPath].y = NodeY(nEnd); s_nPath++; }
	s_ptWay.x = wx; s_ptWay.y = wy;
	s_dwPathTick = GetTickCount();

	*pX = wx; *pY = wy;
	return TRUE;
}

int ObsWalkDist(int px, int py, int tx, int ty)
{
	int d = __distance(px, py, tx, ty);
	if (!g_bObsAvoid || !BuildGrid() || !InGrid(px, py)) return d;
	int cx = tx, cy = ty;
	ClipToGrid(px, py, &cx, &cy);
	if (LineClear(px, py, cx, cy, FALSE)) return d;
	int nEnd = FindPath(px, py, tx, ty);
	if (nEnd < 0) return d;
	int ex = NodeX(nEnd), ey = NodeY(nEnd);
	int nRest = __distance(ex, ey, tx, ty);
	if (InGrid(tx, ty) && nRest > 64) return -1;	// Dich trong vung da nap ma khong toi duoc
	return PathDist(nEnd) + nRest;
}

void ObsFlood(int px, int py)
{
	s_ptFlood.x = px; s_ptFlood.y = py;
	s_bFlood = FALSE;
	if (!g_bObsAvoid || !BuildGrid() || !InGrid(px, py)) return;
	Search(NodeOf(px, py), -1);
	s_dwFloodGen = s_dwGen;
	s_bFlood = TRUE;
}

int ObsReachDist(int x, int y)
{
	int d = __distance(s_ptFlood.x, s_ptFlood.y, x, y);
	if (!s_bFlood || s_dwFloodGen != s_dwGen) return d;
	if (!InGrid(x, y))
	{	// Ngoai vung da nap: di toi bien roi coi nhu chay thang
		int cx = x, cy = y;
		ClipToGrid(s_ptFlood.x, s_ptFlood.y, &cx, &cy);
		int n = NodeOf(cx, cy);
		if (s_pStamp[n] != s_dwGen) return d;	// Bien bi chan - khong biet, coi nhu thang
		return PathDist(n) + __distance(cx, cy, x, y);
	}
	// Diem o nut bi chan (o tam giac): lay nut quanh no toi duoc
	int n = NodeOf(x, y), nx = n % s_nW, ny = n / s_nW, nBest = MAX_INTNUM;
	for (int k = 0; k < 9; k++)
	{
		int ax = nx + k % 3 - 1, ay = ny + k / 3 - 1;
		if (ax < 0 || ay < 0 || ax >= s_nW || ay >= s_nH) continue;
		int a = ay * s_nW + ax;
		if (s_pStamp[a] == s_dwGen) nBest = min(nBest, PathDist(a) + (k == 4 ? 0 : OB_NODE));
	}
	return nBest;
}

int ObsLastPath(POINT *pPts, int nMax, POINT *pWaypoint)
{
	if (!s_nPath || GetTickCount() - s_dwPathTick > 3000) return 0;
	int n = min(nMax, s_nPath);
	memcpy(pPts, s_ptPath, n * sizeof(POINT));
	if (pWaypoint) *pWaypoint = s_ptWay;
	return n;
}
