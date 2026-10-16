#include "stdafx.h"
#include "../GameDef.h"
#include "GameBase.h"
#include "JxCore.h"
#include "S3Client.h"
#include "VLClasses.h"
#include "VLDebug.h"

// Tab MapPath cua cua so DEBUG: ghi/sua/kiem tra du lieu duong di (Auto dung qua PathTravelStart khi xa phu khong co map train)
//
// File Maps\Path\<MapID>_<Ten map Unicode>.txt (moi map 1 file, file cu <MapID>.txt van doc duoc):
//   [Points]  i,x,y                    diem duong di (toa do that - P_PLAYER->GetMapPos)
//   [Sides]   i,j                      canh noi 2 diem (2 chieu)
//   [Doors]   i,mapDich,xVao,yVao      diem i la cong sang mapDich, xuat hien tai (xVao,yVao)
//   [Driver]  i,mapDich,menu           diem i la xa phu, chon dong menu (TCVN3) de toi mapDich
//
// Tu ghi (DebugPathBreathe, 200ms): chay bo -> cu PT_SPACING them 1 diem noi diem truoc,
// di qua diem cu -> noi vao (re nhanh). Doi map: menu vua mo (< 8s) => xa phu, nguoc lai => cong.

#define PT_MAX_POINTS	512
#define PT_MAX_SIDES	1024
#define PT_MAX_DOORS	32
#define PT_MAX_DRIVERS	64
#define PT_MAX_MENU		16
#define PT_SPACING		400		// Khoang cach mac dinh giua 2 diem tu ghi
#define PT_REACH		64		// Toi diem (nhu RunningToPos)
#define PT_KEY_RANGE	500		// Phim Delete/End: chi tac dong diem trong pham vi nay
#define PT_JOIN_RANGE	150		// Phim Space: gan diem cu hon muc nay thi noi vao diem cu
#define PT_DRAW_RANGE	4000	// Chi ve diem/canh quanh nhan vat
#define PT_SCAN_RANGE	500		// Quet cong (trap/Obj/NPC) quanh nhan vat
#define PT_TRAP_DRAW	1200	// Ve o trap quanh nhan vat (khi bat ve node)
#define PT_MAX_TRAPS	32

// Loai cong
enum { door_walk = 0, door_obj, door_npc };	// di bo/trap, click Obj, doi thoai NPC

struct PT_DOOR		{ int nPoint, nDestMap, xIn, yIn, nKind; char szName[32]; };	// nDestMap 0 = chua biet (tu dien khi qua cong)
struct PT_TRAP		{ DWORD dwID; int nCells, xNear, yNear, dNear, xSum, ySum; };
struct PT_DRIVER	{ int nPoint, nDestMap; char szMenu[64]; };

struct PT_GRAPH
{
	int			nMapID;
	BOOL		bDirty;
	int			nPoints;
	POINT		pt[PT_MAX_POINTS];
	int			nSides;
	int			side[PT_MAX_SIDES][2];
	int			nDoors;
	PT_DOOR		door[PT_MAX_DOORS];
	int			nDrivers;
	PT_DRIVER	drv[PT_MAX_DRIVERS];
};

static PT_GRAPH	s_G;					// Do thi cua map dang dung (s_G.nMapID)
static BOOL		s_bRec = FALSE;			// Dang tu ghi
static int		s_nSpacing = PT_SPACING;
static int		s_nLast = -1;			// Diem ghi truoc (de noi)
static int		s_nCurMap = 0;			// Map on dinh gan nhat (Breathe)
static int		s_nMapWait = 0;
static POINT	s_ptLast = { 0, 0 };	// Vi tri gan nhat tren s_nCurMap
static DWORD	s_dwMenuTick = 0;		// Lan cuoi thay menu NPC mo
static int		s_nMenu = 0;
static char		s_szMenu[PT_MAX_MENU][64];

// Lan doi map vua ghi (de Huy)
static struct { int nOldMap, nNewMap; BOOL bDriver; } s_LastTrans = { 0, 0, 0 };

// Ve trong game + phim tat
static BOOL		s_bShow = FALSE;

// Ve duong Auto dang di (tab Ban do) + lenh MoveTo gan nhat
static BOOL		s_bShowWalk = FALSE;
static POINT	s_ptLastMove = { 0, 0 };
static DWORD	s_dwLastMove = 0;
static char		s_szLastMove[32] = "";
static void		DrawWalk();

void PathNoteMove(int x, int y, const char *szSrc)
{
	s_ptLastMove.x = x; s_ptLastMove.y = y;
	s_dwLastMove = GetTickCount();
	strncpy(s_szLastMove, szSrc, 31); s_szLastMove[31] = 0;
}

// Thu di vao cong
static BOOL		s_bDoorTest = FALSE;
static POINT	s_ptDoor = { 0, 0 };	// Toa do thu (Mps)
static int		s_nDoorLink = -1;		// Diem se noi voi cong
static DWORD	s_dwDoorTick = 0;		// Bat dau thu
static DWORD	s_dwDoorReach = 0;		// Luc toi noi (0 = chua toi)

// Chay thu
static int		s_nRunTarget = -1;
static int		s_nRoute[PT_MAX_POINTS];
static int		s_nRouteLen = 0, s_nRouteIdx = 0;
static DWORD	s_dwRunTick = 0;

//------------------------------------------------------------------ tien ich

static BOOL IsPlayerOk()
{
	if (!g_pCurPlayer || !g_pNpcArray[0]) return FALSE;
	int nIndex = g_pCurPlayer->m_nIndex;
	return nIndex > 0 && nIndex < MAX_NPC && !g_NetworkOffline;
}

static void GetPos(int *pX, int *pY)
{
	P_PLAYER->GetMapPos(pX, pY);
}

static const char* MapName(int nMapID)
{
	static char szName[4][64];
	static int n = 0;
	char *sz = szName[n++ & 3];
	theMain.GetMapName(nMapID, sz);
	return sz;
}

// TCVN3 -> Unicode (bang ma giong VLAuto/VNUnicode.cpp) - dung cho ten file
static void Tcvn3ToWide(const char *sz, WCHAR *wsz, int nMax)
{
	static const UCHAR TCVN3[] = {0xFC, 0xFD, 0xFB, 0xFE, 0xFA, 0xF9, 0xF7, 0xF6, 0xF5, 0xF8, 0xF1, 0xF4, 0xEE, 0xEC,
		0xEB, 0xEA, 0xED, 0xE9, 0xE7, 0xE6, 0xE5, 0xE8, 0xE1, 0xE4, 0xDE, 0xD8, 0xD6, 0xD4, 0xD3, 0xD2, 0xD5, 0xCF, 0xCE,
		0xD1, 0xC6, 0xBD, 0xBC, 0xAB, 0xBE, 0xCB, 0xC9, 0xC8, 0xC7, 0xCA, 0xB6, 0xB9, 0xAD, 0xA6, 0xAC, 0xA5, 0xF2, 0xDC,
		0xAE, 0xA8, 0xA1, 0xF3, 0xEF, 0xE2, 0xBB, 0xE3, 0xDF, 0xDD, 0xD7, 0xAA, 0xD0, 0xCC, 0xB7, 0xA9, 0xB8, 0xB5, 0xA4,
		0xA7, 0xA3, 0xA2};
	static const WCHAR UNI[] = {0x1EF9, 0x00FD, 0x1EF7, 0x1EF5, 0x1EF3, 0x1EF1, 0x1EEF, 0x1EED, 0x1EEB, 0x1EE9, 0x1EE7,
		0x1EE5, 0x1EE3, 0x1EE1, 0x1EDF, 0x1EDD, 0x1EDB, 0x1ED9, 0x1ED7, 0x1ED5, 0x1ED3, 0x1ED1, 0x1ECF, 0x1ECD, 0x1ECB,
		0x1EC9, 0x1EC7, 0x1EC5, 0x1EC3, 0x1EC1, 0x1EBF, 0x1EBD, 0x1EBB, 0x1EB9, 0x1EB7, 0x1EB5, 0x1EB3, 0x00F4, 0x1EAF,
		0x1EAD, 0x1EAB, 0x1EA9, 0x1EA7, 0x1EA5, 0x1EA3, 0x1EA1, 0x01B0, 0x01AF, 0x01A1, 0x01A0, 0x0169, 0x0129, 0x0111,
		0x0103, 0x0102, 0x00FA, 0x00F9, 0x00F5, 0x1EB1, 0x00F3, 0x00F2, 0x00ED, 0x00EC, 0x00EA, 0x00E9, 0x00E8, 0x00E3,
		0x00E2, 0x00E1, 0x00E0, 0x00D4, 0x0110, 0x00CA, 0x00C2};
	int i;
	for (i = 0; sz[i] && i < nMax - 1; i++)
	{
		UCHAR c = (UCHAR)sz[i];
		WCHAR w = c < 0x80 ? c : 0x0020;
		for (int j = 0; j < sizeof(TCVN3); j++)
			if (c == TCVN3[j]) { w = UNI[j]; break; }
		if (wcschr(L"\\/:*?\"<>|", w)) w = L'_';	// ky tu cam trong ten file
		wsz[i] = w;
	}
	wsz[i] = 0;
}

// File duong di: Maps\Path\<MapID>_<Ten map Unicode>.txt (ten theo MapNames.ini)
static void PathFileName(int nMapID, WCHAR *wszFile)
{
	const char *szName = MapName(nMapID);
	WCHAR wszName[64];
	Tcvn3ToWide(szName, wszName, 64);
	if (!szName[0] || !strcmp(szName, "<Unknown_Map>"))
		swprintf(wszFile, MAX_PATH, L"%hs\\Maps\\Path\\%d.txt", theMain.m_szCurFolder, nMapID);
	else
		swprintf(wszFile, MAX_PATH, L"%hs\\Maps\\Path\\%d_%s.txt", theMain.m_szCurFolder, nMapID, wszName);
}

// Ten file de in ra DEBUG (TCVN3, UI tu doi sang Unicode)
static const char* PathFileShow(int nMapID)
{
	static char szShow[MAX_PATH];
	const char *szName = MapName(nMapID);
	if (!szName[0] || !strcmp(szName, "<Unknown_Map>"))
		sprintf(szShow, "Maps\\Path\\%d.txt", nMapID);
	else
		sprintf(szShow, "Maps\\Path\\%d_%s.txt", nMapID, szName);
	return szShow;
}

// Tim file da co cua map: <MapID>_*.txt (ten map co the da doi) hoac file cu <MapID>.txt
static BOOL FindPathFile(int nMapID, WCHAR *wszFile)
{
	WCHAR wszFind[MAX_PATH];
	swprintf(wszFind, MAX_PATH, L"%hs\\Maps\\Path\\%d_*.txt", theMain.m_szCurFolder, nMapID);
	WIN32_FIND_DATAW fd;
	HANDLE hFind = FindFirstFileW(wszFind, &fd);
	if (hFind != INVALID_HANDLE_VALUE)
	{
		FindClose(hFind);
		swprintf(wszFile, MAX_PATH, L"%hs\\Maps\\Path\\%s", theMain.m_szCurFolder, fd.cFileName);
		return TRUE;
	}
	swprintf(wszFile, MAX_PATH, L"%hs\\Maps\\Path\\%d.txt", theMain.m_szCurFolder, nMapID);
	return GetFileAttributesW(wszFile) != INVALID_FILE_ATTRIBUTES;
}

static int Nearest(int x, int y, int *pDist = NULL, int nExcept = -1)
{
	int nBest = -1, dMin = MAX_INTNUM;
	for (int i = 0; i < s_G.nPoints; i++)
	{
		if (i == nExcept) continue;
		int d = __distance(s_G.pt[i].x, s_G.pt[i].y, x, y);
		if (d < dMin) { dMin = d; nBest = i; }
	}
	if (pDist) *pDist = dMin;
	return nBest;
}

static BOOL HasSide(int i, int j)
{
	for (int k = 0; k < s_G.nSides; k++)
		if ((s_G.side[k][0] == i && s_G.side[k][1] == j) || (s_G.side[k][0] == j && s_G.side[k][1] == i))
			return TRUE;
	return FALSE;
}

static BOOL AddSide(int i, int j)
{
	if (i < 0 || j < 0 || i == j || i >= s_G.nPoints || j >= s_G.nPoints) return FALSE;
	if (HasSide(i, j)) return TRUE;
	if (s_G.nSides >= PT_MAX_SIDES) return FALSE;
	s_G.side[s_G.nSides][0] = i;
	s_G.side[s_G.nSides][1] = j;
	s_G.nSides++;
	s_G.bDirty = TRUE;
	return TRUE;
}

static BOOL DelSide(int i, int j)
{
	for (int k = 0; k < s_G.nSides; k++)
		if ((s_G.side[k][0] == i && s_G.side[k][1] == j) || (s_G.side[k][0] == j && s_G.side[k][1] == i))
		{
			s_G.side[k][0] = s_G.side[s_G.nSides - 1][0];
			s_G.side[k][1] = s_G.side[s_G.nSides - 1][1];
			s_G.nSides--;
			s_G.bDirty = TRUE;
			return TRUE;
		}
	return FALSE;
}

// Them diem, noi voi nLink (-1 = khong noi). Tra ve chi so hoac -1
static int AddPoint(int x, int y, int nLink)
{
	if (s_G.nPoints >= PT_MAX_POINTS) return -1;
	int n = s_G.nPoints++;
	s_G.pt[n].x = x;
	s_G.pt[n].y = y;
	s_G.bDirty = TRUE;
	AddSide(nLink, n);
	return n;
}

// Diem tai (x,y): dung lai diem cu neu cach <= nReuse
static int PointAt(int x, int y, int nReuse, int nLink)
{
	int d, n = Nearest(x, y, &d);
	if (n >= 0 && d <= nReuse)
	{
		AddSide(nLink, n);
		return n;
	}
	return AddPoint(x, y, nLink);
}

static void DelPoint(int n)
{
	if (n < 0 || n >= s_G.nPoints) return;
	int k, m;
	for (k = 0, m = 0; k < s_G.nSides; k++)
	{
		int a = s_G.side[k][0], b = s_G.side[k][1];
		if (a == n || b == n) continue;
		s_G.side[m][0] = a > n ? a - 1 : a;
		s_G.side[m][1] = b > n ? b - 1 : b;
		m++;
	}
	s_G.nSides = m;
	for (k = 0, m = 0; k < s_G.nDoors; k++)
	{
		if (s_G.door[k].nPoint == n) continue;
		s_G.door[m] = s_G.door[k];
		if (s_G.door[m].nPoint > n) s_G.door[m].nPoint--;
		m++;
	}
	s_G.nDoors = m;
	for (k = 0, m = 0; k < s_G.nDrivers; k++)
	{
		if (s_G.drv[k].nPoint == n) continue;
		s_G.drv[m] = s_G.drv[k];
		if (s_G.drv[m].nPoint > n) s_G.drv[m].nPoint--;
		m++;
	}
	s_G.nDrivers = m;
	for (k = n; k < s_G.nPoints - 1; k++)
		s_G.pt[k] = s_G.pt[k + 1];
	s_G.nPoints--;
	if (s_nLast == n) s_nLast = -1;
	else if (s_nLast > n) s_nLast--;
	s_G.bDirty = TRUE;
}

static BOOL AddDoor(int nPoint, int nDestMap, int xIn, int yIn, int nKind = door_walk, const char *szName = "")
{
	for (int k = 0; k < s_G.nDoors; k++)
		if (s_G.door[k].nPoint == nPoint && s_G.door[k].nDestMap == nDestMap)
		{
			PT_DOOR &d = s_G.door[k];
			if (xIn || yIn) { d.xIn = xIn; d.yIn = yIn; }
			if (nKind != door_walk || szName[0]) { d.nKind = nKind; strncpy(d.szName, szName, 31); d.szName[31] = 0; }
			s_G.bDirty = TRUE;
			return TRUE;
		}
	if (s_G.nDoors >= PT_MAX_DOORS) return FALSE;
	PT_DOOR &d = s_G.door[s_G.nDoors++];
	d.nPoint = nPoint; d.nDestMap = nDestMap; d.xIn = xIn; d.yIn = yIn;
	d.nKind = nKind; strncpy(d.szName, szName, 31); d.szName[31] = 0;
	s_G.bDirty = TRUE;
	return TRUE;
}

static const char* DoorKindName(int nKind)
{
	return nKind == door_obj ? "Obj" : nKind == door_npc ? "NPC" : "di bo";
}

static BOOL AddDriver(int nPoint, int nDestMap, const char *szMenu)
{
	for (int k = 0; k < s_G.nDrivers; k++)
		if (s_G.drv[k].nPoint == nPoint && s_G.drv[k].nDestMap == nDestMap)
		{
			if (szMenu[0]) strncpy(s_G.drv[k].szMenu, szMenu, 63);
			s_G.bDirty = TRUE;
			return TRUE;
		}
	if (s_G.nDrivers >= PT_MAX_DRIVERS) return FALSE;
	PT_DRIVER &d = s_G.drv[s_G.nDrivers++];
	d.nPoint = nPoint; d.nDestMap = nDestMap;
	strncpy(d.szMenu, szMenu, 63); d.szMenu[63] = 0;
	s_G.bDirty = TRUE;
	return TRUE;
}

//------------------------------------------------------------------ file

static BOOL LoadGraph(PT_GRAPH *g, int nMapID)
{
	memset(g, 0, sizeof(PT_GRAPH));
	g->nMapID = nMapID;
	char szLine[256];
	WCHAR wszFile[MAX_PATH];
	if (!FindPathFile(nMapID, wszFile)) return FALSE;
	FILE *pFile = _wfopen(wszFile, L"r");
	if (!pFile) return FALSE;
	int nSec = 0;
	while (fgets(szLine, sizeof(szLine), pFile))
	{
		char *p = szLine;
		while (*p == ' ' || *p == '\t') p++;
		p[strcspn(p, "\r\n")] = 0;
		if (!*p || *p == ';') continue;
		if (*p == '[')
		{
			nSec = !_strnicmp(p, "[Points]", 8) ? 1 : !_strnicmp(p, "[Sides]", 7) ? 2 :
				!_strnicmp(p, "[Doors]", 7) ? 3 : !_strnicmp(p, "[Driver]", 8) ? 4 : 0;
			continue;
		}
		int a, b, c, d, n;
		switch (nSec)
		{
		case 1:
			if (sscanf(p, "%d,%d,%d", &a, &b, &c) == 3 && a >= 0 && a < PT_MAX_POINTS)
			{
				g->pt[a].x = b; g->pt[a].y = c;
				if (a + 1 > g->nPoints) g->nPoints = a + 1;
			}
			break;
		case 2:
			if (sscanf(p, "%d,%d", &a, &b) == 2 && g->nSides < PT_MAX_SIDES)
			{
				g->side[g->nSides][0] = a; g->side[g->nSides][1] = b; g->nSides++;
			}
			break;
		case 3:
			n = 0;
			c = d = 0;
			if (sscanf(p, "%d,%d,%d,%d%n", &a, &b, &c, &d, &n) >= 2 && g->nDoors < PT_MAX_DOORS)
			{
				PT_DOOR &dr = g->door[g->nDoors++];
				dr.nPoint = a; dr.nDestMap = b; dr.xIn = c; dr.yIn = d;
				int k = 0, m = 0;	// tuy chon: ,loai,ten
				if (n && sscanf(p + n, ",%d,%n", &k, &m) >= 1)
				{
					dr.nKind = k;
					if (m) { strncpy(dr.szName, p + n + m, 31); dr.szName[31] = 0; }
				}
			}
			break;
		case 4:
			n = 0;
			if (sscanf(p, "%d,%d,%n", &a, &b, &n) >= 2 && g->nDrivers < PT_MAX_DRIVERS)
			{
				PT_DRIVER &dv = g->drv[g->nDrivers++];
				dv.nPoint = a; dv.nDestMap = b;
				strncpy(dv.szMenu, n ? p + n : "", 63); dv.szMenu[63] = 0;
			}
			break;
		}
	}
	fclose(pFile);
	return TRUE;
}

static BOOL SaveGraph(PT_GRAPH *g)
{
	char szFile[MAX_PATH];
	sprintf(szFile, "%s\\Maps\\Path", theMain.m_szCurFolder);
	CreateDirectoryA(szFile, NULL);
	WCHAR wszFile[MAX_PATH], wszOld[MAX_PATH];
	PathFileName(g->nMapID, wszFile);
	BOOL bOld = FindPathFile(g->nMapID, wszOld) && _wcsicmp(wszOld, wszFile);
	FILE *pFile = _wfopen(wszFile, L"w");
	if (!pFile) return FALSE;
	fprintf(pFile, "; VLAuto MapPath - Map %d %s\n", g->nMapID, MapName(g->nMapID));
	fprintf(pFile, "; [Points] i,x,y  [Sides] i,j  [Doors] diem,mapDich,xVao,yVao[,loai(0 di bo/trap,1 Obj,2 NPC),ten]  [Driver] diem,mapDich,menu\n");
	fprintf(pFile, "[Points]\n");
	for (int i = 0; i < g->nPoints; i++)
		fprintf(pFile, "%d,%d,%d\n", i, g->pt[i].x, g->pt[i].y);
	fprintf(pFile, "[Sides]\n");
	for (int i = 0; i < g->nSides; i++)
		fprintf(pFile, "%d,%d\n", g->side[i][0], g->side[i][1]);
	fprintf(pFile, "[Doors]\n");
	for (int i = 0; i < g->nDoors; i++)
	{
		const PT_DOOR &d = g->door[i];
		if (d.nKind != door_walk || d.szName[0])
			fprintf(pFile, "%d,%d,%d,%d,%d,%s\n", d.nPoint, d.nDestMap, d.xIn, d.yIn, d.nKind, d.szName);
		else
			fprintf(pFile, "%d,%d,%d,%d\n", d.nPoint, d.nDestMap, d.xIn, d.yIn);
	}
	fprintf(pFile, "[Driver]\n");
	for (int i = 0; i < g->nDrivers; i++)
		fprintf(pFile, "%d,%d,%s\n", g->drv[i].nPoint, g->drv[i].nDestMap, g->drv[i].szMenu);
	fclose(pFile);
	if (bOld) DeleteFileW(wszOld);	// file cu (<MapID>.txt hoac ten map cu) da chuyen sang ten moi
	g->bDirty = FALSE;
	return TRUE;
}

// Do thi phai la cua map hien tai: luu map cu (neu sua) va nap map moi
static void EnsureGraph()
{
	int nMap = *g_pMapID;
	if (s_G.nMapID == nMap) return;
	if (s_G.nMapID && s_G.bDirty)
		SaveGraph(&s_G);
	LoadGraph(&s_G, nMap);
	s_nLast = -1;
	s_nRunTarget = -1;
}

//------------------------------------------------------------------ menu NPC

static int ReadMenu(char szMenu[PT_MAX_MENU][64])
{
	KUiMsgSel *pMsgSel = (KUiMsgSel*)GetPtrAddress(MSG_BASE_ADD);
	KUiMsgSel2 *pMsgSel2 = (KUiMsgSel2*)GetPtrAddress(MSG2_BASE_ADD);
	int nNum = 0;
	KOneMsgInfo **pMsgs = NULL;
	if (pMsgSel2 && pMsgSel2->IsVisible()) { nNum = pMsgSel2->m_nNumMessage; pMsgs = pMsgSel2->m_pMessages; }
	else if (pMsgSel && pMsgSel->IsVisible()) { nNum = pMsgSel->m_nNumMessage; pMsgs = pMsgSel->m_pMessages; }
	if (!pMsgs) return 0;
	int n = 0;
	for (int i = 0; i < nNum && n < PT_MAX_MENU; i++)
	{
		KOneMsgInfo *pMsg = pMsgs[i];
		if (!pMsg) continue;
		int nLen = (pMsg->nLen > 0 && pMsg->nLen < 63) ? pMsg->nLen : 63;
		strncpy(szMenu[n], (const char*)pMsg->Msg, nLen);
		szMenu[n][nLen] = 0;
		szMenu[n][strcspn(szMenu[n], "\r\n")] = 0;
		n++;
	}
	return n;
}

// Tim map co ten (MapNames.ini) nam trong dong menu - lay ten dai nhat
static int MatchMapID(const char *szMenu)
{
	static char *s_pSection = NULL;
	if (!s_pSection)
	{
		char szFile[MAX_PATH];
		sprintf(szFile, "%s\\Maps\\MapNames.ini", theMain.m_szCurFolder);
		s_pSection = (char*)malloc(65536);
		if (!s_pSection) return 0;
		if (!GetPrivateProfileSectionA("Map_Name", s_pSection, 65536, szFile))
			s_pSection[0] = s_pSection[1] = 0;
	}
	int nBest = 0, nBestLen = 0;
	for (char *p = s_pSection; *p; p += strlen(p) + 1)
	{
		char *pEq = strchr(p, '=');
		if (!pEq) continue;
		const char *szName = pEq + 1;
		int nLen = (int)strlen(szName);
		if (nLen > 3 && nLen > nBestLen && TcvnStrStrI(szMenu, szName))
		{
			nBest = atoi(p);
			nBestLen = nLen;
		}
	}
	return nBest;
}

//------------------------------------------------------------------ tim duong

// Dijkstra tren s_G tu nStart toi nEnd -> s_nRoute (nStart..nEnd). Tra ve do dai, 0 = khong co duong
static int FindRoute(int nStart, int nEnd, int *pRoute, int *pDist = NULL, const PT_GRAPH *g = &s_G)
{
	static int Dist[PT_MAX_POINTS], Prev[PT_MAX_POINTS];
	static BOOL Done[PT_MAX_POINTS];
	int n = g->nPoints;
	if (nStart < 0 || nEnd < 0 || nStart >= n || nEnd >= n) return 0;
	for (int i = 0; i < n; i++) { Dist[i] = MAX_INTNUM; Prev[i] = -1; Done[i] = FALSE; }
	Dist[nStart] = 0;
	while (TRUE)
	{
		int u = -1;
		for (int i = 0; i < n; i++)
			if (!Done[i] && Dist[i] < MAX_INTNUM && (u < 0 || Dist[i] < Dist[u])) u = i;
		if (u < 0 || u == nEnd) break;
		Done[u] = TRUE;
		for (int k = 0; k < g->nSides; k++)
		{
			int v;
			if (g->side[k][0] == u) v = g->side[k][1];
			else if (g->side[k][1] == u) v = g->side[k][0];
			else continue;
			if (v < 0 || v >= n || Done[v]) continue;
			int d = Dist[u] + __distance(g->pt[u], g->pt[v]);
			if (d < Dist[v]) { Dist[v] = d; Prev[v] = u; }
		}
	}
	if (Dist[nEnd] >= MAX_INTNUM) return 0;
	int nLen = 0;
	for (int v = nEnd; v >= 0; v = Prev[v]) nLen++;
	int k = nLen;
	for (int v = nEnd; v >= 0; v = Prev[v]) pRoute[--k] = v;
	if (pDist) *pDist = Dist[nEnd];
	return nLen;
}

//------------------------------------------------------------------ ve trong game
// Chen vao KUiGameSpace::PaintWindow (DGS_JMP_ADD: jmp DrawGameSpace) => ve ngay sau canh game,
// truoc cac cua so UI. Toa do the gioi = Mps (= P_PLAYER->GetMapPos), z = 0.

struct KRPos3	{ int x, y, z; };
struct KRULine	{ KRPos3 p1, p2; DWORD dwColor; };	// = KRULine trong source (RU_T_LINE = 1)

typedef void (__thiscall *PFN_DRAWPRIM)(void*, int, void*, unsigned int, int);
typedef void (__thiscall *PFN_OUTTEXT)(void*, int, const char*, int, int, int, unsigned int, int, int, unsigned int);
typedef void (__thiscall *PFN_DRAWGAMESPACE)(void*);

#define CLR_SIDE		0xFFFFFF00	// vang
#define CLR_ROUTE		0xFF00FFFF	// xanh duong nhat - duong dang chay thu
#define CLR_POINT		0xFF00FF00	// xanh la
#define CLR_LAST		0xFF00FFFF	// diem truoc (diem ghi tiep se noi vao)
#define CLR_DOOR		0xFFFF4040	// do - cong
#define CLR_DRIVER		0xFFFF40FF	// tim - xa phu
#define CLR_NEAR		0xFFFFFFFF	// trang - diem phim Delete/End se tac dong

static BOOL		s_bPatched = FALSE;
static BYTE		s_OrgJmp[5];
static KRULine	s_Lines[PT_MAX_SIDES + PT_MAX_POINTS * 4 + 8];

static DWORD PointColor(int i)
{
	for (int k = 0; k < s_G.nDoors; k++)
		if (s_G.door[k].nPoint == i) return CLR_DOOR;
	for (int k = 0; k < s_G.nDrivers; k++)
		if (s_G.drv[k].nPoint == i) return CLR_DRIVER;
	return i == s_nLast ? CLR_LAST : CLR_POINT;
}

static BOOL InRoute(int a, int b)
{
	if (s_nRunTarget < 0) return FALSE;
	for (int i = 0; i + 1 < s_nRouteLen; i++)
		if ((s_nRoute[i] == a && s_nRoute[i + 1] == b) || (s_nRoute[i] == b && s_nRoute[i + 1] == a))
			return TRUE;
	return FALSE;
}

static void AddLine(int &n, int x1, int y1, int x2, int y2, DWORD dwColor)
{
	KRULine &l = s_Lines[n++];
	l.p1.x = x1; l.p1.y = y1; l.p1.z = 0;
	l.p2.x = x2; l.p2.y = y2; l.p2.z = 0;
	l.dwColor = dwColor;
}

static BOOL IsNearPlayer(POINT pt, int px, int py)
{
	return abs(pt.x - px) < PT_DRAW_RANGE && abs(pt.y - py) < PT_DRAW_RANGE * 2;
}

//------------------------------------------------------------------ trap ban do
// g_ScenePlace.m_pInProcessAreaRegions[15] -> KScenePlaceRegionC: m_TrapInfo[x][y] (o 32x32 Mps) = ID script trap (hash ten file)

#define CLR_TRAP		0xFFFF8000	// cam - o trap

static KRULine	s_TrapLines[400 * 4];

// Goi fn(x trai, y tren cua o, ID) cho moi o trap trong cac region dang nap
template <class FN> static void ForEachTrapCell(FN fn)
{
	BYTE *pScene = (BYTE*)SCP_BASE_ADD;
	for (int r = 0; r < SCP_NUM_REGIONS; r++)
	{
		BYTE *pReg = *(BYTE**)(pScene + SCP_REGIONS_OFF + r * 4);
		if (!pReg || *(int*)(pReg + SPR_STATUS_OFF) != 3) continue;	// REGION_S_STANDBY
		int nLeft = *(int*)(pReg + SPR_LEFTTOP_OFF);
		int nTop = *(int*)(pReg + SPR_LEFTTOP_OFF + 4);
		const DWORD *pTrap = (const DWORD*)(pReg + SPR_TRAP_OFF);
		for (int i = 0; i < 16; i++)
			for (int j = 0; j < 32; j++)
				if (pTrap[i * 32 + j])
					fn(nLeft + i * 32, nTop + j * 32, pTrap[i * 32 + j]);
	}
}

// Gom cac o trap trong pham vi theo ID. Tra ve so trap
static int ScanTraps(int px, int py, int nRange, PT_TRAP *pTraps)
{
	int nTraps = 0;
	ForEachTrapCell([&](int x, int y, DWORD dwID)
	{
		int cx = x + 16, cy = y + 16;
		int d = __distance(cx, cy, px, py);
		if (d > nRange) return;
		int k;
		for (k = 0; k < nTraps && pTraps[k].dwID != dwID; k++);
		if (k == nTraps)
		{
			if (nTraps >= PT_MAX_TRAPS) return;
			memset(&pTraps[k], 0, sizeof(PT_TRAP));
			pTraps[k].dwID = dwID;
			pTraps[k].dNear = MAX_INTNUM;
			nTraps++;
		}
		PT_TRAP &t = pTraps[k];
		t.nCells++;
		t.xSum += cx; t.ySum += cy;
		if (d < t.dNear) { t.dNear = d; t.xNear = cx; t.yNear = cy; }
	});
	return nTraps;
}

static void DrawTraps(void *pRep, PFN_DRAWPRIM pfnDraw, int px, int py)
{
	int n = 0, nMax = sizeof(s_TrapLines) / sizeof(s_TrapLines[0]) - 4;
	ForEachTrapCell([&](int x, int y, DWORD)
	{
		if (n > nMax || abs(x - px) > PT_TRAP_DRAW || abs(y - py) > PT_TRAP_DRAW * 2) return;
		KRULine *l = &s_TrapLines[n];
		int xs[5] = { x, x + 32, x + 32, x, x }, ys[5] = { y, y, y + 32, y + 32, y };
		for (int k = 0; k < 4; k++, l++)
		{
			l->p1.x = xs[k]; l->p1.y = ys[k]; l->p1.z = 0;
			l->p2.x = xs[k + 1]; l->p2.y = ys[k + 1]; l->p2.z = 0;
			l->dwColor = CLR_TRAP;
		}
		n += 4;
	});
	if (n > 0)
		pfnDraw(pRep, n, s_TrapLines, 1, 0);
}

static void DrawPath()
{
	if (!s_bShow && !s_bRec && s_nRunTarget < 0 && !s_bDoorTest) return;
	if (!IsPlayerOk() || s_G.nMapID != *g_pMapID) return;
	void *pRep = *(void**)RPS_BASE_ADD;
	if (!pRep) return;
	DWORD *pVT = *(DWORD**)pRep;
	PFN_DRAWPRIM pfnDraw = (PFN_DRAWPRIM)pVT[RPS_VT_DRAWPRIM / 4];
	PFN_OUTTEXT pfnText = (PFN_OUTTEXT)pVT[RPS_VT_OUTTEXT / 4];

	int px, py, d;
	GetPos(&px, &py);
	int nNear = Nearest(px, py, &d);
	if (d > PT_KEY_RANGE) nNear = -1;

	if (s_bShow)
		DrawTraps(pRep, pfnDraw, px, py);
	if (s_bDoorTest)
	{	// Dau X do tai toa do dang thu
		KRULine l[2] = { { { s_ptDoor.x - 32, s_ptDoor.y - 64, 0 }, { s_ptDoor.x + 32, s_ptDoor.y + 64, 0 }, CLR_DOOR },
						 { { s_ptDoor.x + 32, s_ptDoor.y - 64, 0 }, { s_ptDoor.x - 32, s_ptDoor.y + 64, 0 }, CLR_DOOR } };
		pfnDraw(pRep, 2, l, 1, 0);
	}

	int n = 0;
	for (int k = 0; k < s_G.nSides; k++)
	{
		int a = s_G.side[k][0], b = s_G.side[k][1];
		if (a < 0 || b < 0 || a >= s_G.nPoints || b >= s_G.nPoints) continue;
		if (!IsNearPlayer(s_G.pt[a], px, py) && !IsNearPlayer(s_G.pt[b], px, py)) continue;
		AddLine(n, s_G.pt[a].x, s_G.pt[a].y, s_G.pt[b].x, s_G.pt[b].y, InRoute(a, b) ? CLR_ROUTE : CLR_SIDE);
	}
	for (int i = 0; i < s_G.nPoints; i++)
	{
		POINT pt = s_G.pt[i];
		if (!IsNearPlayer(pt, px, py)) continue;
		DWORD dwColor = (i == nNear) ? CLR_NEAR : PointColor(i);
		int r = (i == nNear) ? 32 : 20;		// hinh thoi: Mps y gap doi tren man hinh
		AddLine(n, pt.x - r, pt.y, pt.x, pt.y - r * 2, dwColor);
		AddLine(n, pt.x, pt.y - r * 2, pt.x + r, pt.y, dwColor);
		AddLine(n, pt.x + r, pt.y, pt.x, pt.y + r * 2, dwColor);
		AddLine(n, pt.x, pt.y + r * 2, pt.x - r, pt.y, dwColor);
	}
	if (n > 0)
		pfnDraw(pRep, n, s_Lines, 1, 0);	// RU_T_LINE, toa do the gioi

	char szText[16];
	for (int i = 0; i < s_G.nPoints; i++)
	{
		POINT pt = s_G.pt[i];
		if (!IsNearPlayer(pt, px, py)) continue;
		sprintf(szText, "%d", i);
		pfnText(pRep, 12, szText, -1, pt.x + 24, pt.y - 16, (i == nNear) ? CLR_NEAR : PointColor(i), 0, 0, 0xFF000000);
	}
}

static void __fastcall HookDrawGameSpace(void *pCoreShell)
{
	((PFN_DRAWGAMESPACE)DGS_FUNC_ADD)(pCoreShell);
	__try
	{
		DrawPath();
		DrawWalk();
	}
	__except (EXCEPTION_EXECUTE_HANDLER)
	{
		s_bShow = FALSE;	// Sai dia chi/offset - tat ve
		s_bShowWalk = FALSE;
	}
}

static BOOL PatchDraw(BOOL bOn)
{
	if (bOn == s_bPatched) return TRUE;
	BYTE *p = (BYTE*)DGS_JMP_ADD;
	DWORD dwOld;
	if (!VirtualProtect(p, 5, PAGE_EXECUTE_READWRITE, &dwOld)) return FALSE;
	BOOL bOK = TRUE;
	if (bOn)
	{
		// 10-10: "jmp DrawGameSpace" (E9), 15-10: "call DrawGameSpace" (E8) - HookDrawGameSpace (__fastcall) dung duoc ca hai
		if ((p[0] != 0xE9 && p[0] != 0xE8) || (DWORD)(p + 5) + *(int*)(p + 1) != DGS_FUNC_ADD)
			bOK = FALSE;	// Khong phai "jmp/call DrawGameSpace" - sai ban game
		else
		{
			memcpy(s_OrgJmp, p, 5);
			*(int*)(p + 1) = (int)((DWORD)HookDrawGameSpace - (DWORD)(p + 5));
		}
	}
	else
		memcpy(p, s_OrgJmp, 5);
	VirtualProtect(p, 5, dwOld, &dwOld);
	FlushInstructionCache(GetCurrentProcess(), p, 5);
	if (bOK) s_bPatched = bOn;
	return bOK;
}

void DebugPathUnhook()
{
	s_bShow = FALSE;
	s_bShowWalk = FALSE;
	s_bRec = FALSE;
	s_nRunTarget = -1;
	PatchDraw(FALSE);
}

//------------------------------------------------------------------ phim tat (khi bat ve node)
// Space: them diem tai vi tri hien tai (gan diem cu < PT_JOIN_RANGE thi noi vao diem cu)
// Delete: xoa diem gan nhat (<= PT_KEY_RANGE)   End: chon diem gan nhat lam diem truoc (re nhanh)

BOOL DebugPathKey(WPARAM wParam, LPARAM lParam)
{
	if (!s_bShow) return FALSE;
	if (wParam != VK_SPACE && wParam != VK_DELETE && wParam != VK_END) return FALSE;
	if ((_GetKeyState(VK_CONTROL) & 0x8000) || (_GetKeyState(VK_MENU) & 0x8000) || (_GetKeyState(VK_SHIFT) & 0x8000))
		return FALSE;
	if (lParam & 0x40000000) return TRUE;	// Giu phim (lap lai) - bo qua
	if (!IsPlayerOk()) return FALSE;
	EnsureGraph();

	char szMsg[128];
	int x, y, d;
	GetPos(&x, &y);
	int nNear = Nearest(x, y, &d);
	switch (wParam)
	{
	case VK_SPACE:
		if (nNear >= 0 && d <= PT_JOIN_RANGE)
		{
			if (s_nLast >= 0 && s_nLast != nNear) AddSide(s_nLast, nNear);
			sprintf(szMsg, "<color=yellow>MapPath: noi diem %d - %d", s_nLast, nNear);
			s_nLast = nNear;
		}
		else
		{
			int n = AddPoint(x, y, s_nLast);
			if (n < 0) { SystemMessage("<color=red>MapPath: day diem"); return TRUE; }
			if (s_nLast >= 0) sprintf(szMsg, "<color=yellow>MapPath: them diem %d (%d/%d) noi diem %d", n, x >> 8, y >> 9, s_nLast);
			else sprintf(szMsg, "<color=yellow>MapPath: them diem %d (%d/%d)", n, x >> 8, y >> 9);
			s_nLast = n;
		}
		break;
	case VK_DELETE:
		if (nNear < 0 || d > PT_KEY_RANGE)
		{
			sprintf(szMsg, "<color=red>MapPath: khong co diem nao trong pham vi %d", PT_KEY_RANGE);
			break;
		}
		DelPoint(nNear);
		sprintf(szMsg, "<color=yellow>MapPath: xoa diem %d (chi so cac diem sau giam 1)", nNear);
		break;
	case VK_END:
		if (nNear < 0 || d > PT_KEY_RANGE)
		{
			sprintf(szMsg, "<color=red>MapPath: khong co diem nao trong pham vi %d", PT_KEY_RANGE);
			break;
		}
		s_nLast = nNear;
		sprintf(szMsg, "<color=yellow>MapPath: diem truoc = %d (re nhanh tu day)", nNear);
		break;
	}
	SystemMessage(szMsg);
	return TRUE;
}

//------------------------------------------------------------------ Breathe (200ms)

static void OnMapChanged(int nOldMap, POINT ptOld, int nNewMap, int x, int y)
{
	BOOL bDriver = GetTickCount() - s_dwMenuTick < 8000;
	char szMsg[160];

	// Map cu: diem cong / xa phu
	if (s_G.nMapID != nOldMap)
	{
		if (s_G.nMapID && s_G.bDirty) SaveGraph(&s_G);
		LoadGraph(&s_G, nOldMap);
		s_nLast = -1;
	}
	// Cong da danh dau truoc (trap/Obj/NPC) nhung chua biet map dich -> dien vao
	int nUnknown = -1;
	for (int k = 0; k < s_G.nDoors; k++)
	{
		const PT_DOOR &d = s_G.door[k];
		if (d.nDestMap || d.nPoint < 0 || d.nPoint >= s_G.nPoints) continue;
		if (__distance(s_G.pt[d.nPoint].x, s_G.pt[d.nPoint].y, ptOld.x, ptOld.y) <= PT_SCAN_RANGE) { nUnknown = k; break; }
	}
	if (nUnknown >= 0)
	{
		PT_DOOR &d = s_G.door[nUnknown];
		d.nDestMap = nNewMap; d.xIn = x; d.yIn = y;
		s_G.bDirty = TRUE;
		sprintf(szMsg, "<color=yellow>MapPath: cong %s (diem %d) -> %s (%d)", DoorKindName(d.nKind), d.nPoint, MapName(nNewMap), nNewMap);
		bDriver = d.nKind == door_npc;	// NPC: khong ghi cong chieu ve
	}
	else if (bDriver)
	{
		int nPoint = PointAt(ptOld.x, ptOld.y, 600, -1);
		const char *szMenu = "";
		const char *szName = MapName(nNewMap);
		for (int i = 0; i < s_nMenu; i++)
			if (MatchMapID(s_szMenu[i]) == nNewMap || TcvnStrStrI(s_szMenu[i], szName)) { szMenu = s_szMenu[i]; break; }
		AddDriver(nPoint, nNewMap, szMenu);
		sprintf(szMsg, "<color=yellow>MapPath: xa phu (diem %d) -> %s (%d)", nPoint, szName, nNewMap);
	}
	else
	{
		int nPoint = PointAt(ptOld.x, ptOld.y, 150, s_nLast);
		AddDoor(nPoint, nNewMap, x, y);
		sprintf(szMsg, "<color=yellow>MapPath: cong (diem %d) -> %s (%d)", nPoint, MapName(nNewMap), nNewMap);
	}
	SaveGraph(&s_G);
	SystemMessage(szMsg);

	// Map moi: diem vao (+ cong chieu ve neu di bo)
	LoadGraph(&s_G, nNewMap);
	s_nLast = PointAt(x, y, s_nSpacing / 2, -1);
	if (!bDriver && s_nLast >= 0)
	{
		AddDoor(s_nLast, nOldMap, ptOld.x, ptOld.y);
		SystemMessage("<color=yellow>MapPath: ghi cong chieu ve (huy neu sai: nut Huy cong/xa phu vua ghi)");
	}
	s_LastTrans.nOldMap = nOldMap;
	s_LastTrans.nNewMap = nNewMap;
	s_LastTrans.bDriver = bDriver;
}

static void RunBreathe(int x, int y)
{
	if (s_nRouteIdx >= s_nRouteLen)
	{
		SystemMessage("<color=green>MapPath: da toi diem dich");
		s_nRunTarget = -1;
		return;
	}
	if (GetTickCount() - s_dwRunTick > 20000)
	{
		SystemMessage("<color=red>MapPath: ket - 20s khong toi duoc diem tiep theo, dung chay thu");
		s_nRunTarget = -1;
		return;
	}
	POINT pt = s_G.pt[s_nRoute[s_nRouteIdx]];
	if (__distance(pt.x, pt.y, x, y) <= PT_REACH)
	{
		s_nRouteIdx++;
		s_dwRunTick = GetTickCount();
		return;
	}
	g_pCurPlayer->MoveTo(pt.x, pt.y);
}

static void RecBreathe(int x, int y)
{
	if (s_G.nMapID != *g_pMapID) return;
	int d, nNear = Nearest(x, y, &d, s_nLast);
	if (s_nLast < 0 || s_nLast >= s_G.nPoints)
	{
		s_nLast = PointAt(x, y, s_nSpacing / 2, -1);
		return;
	}
	int dLast = __distance(s_G.pt[s_nLast].x, s_G.pt[s_nLast].y, x, y);
	if (dLast > s_nSpacing * 4)
	{	// Dich chuyen trong map (hoi sinh, phu...) - khong noi
		s_nLast = PointAt(x, y, s_nSpacing / 2, -1);
		return;
	}
	if (nNear >= 0 && d <= s_nSpacing / 3)
	{	// Di qua diem cu -> noi vao (re nhanh / gop duong)
		AddSide(s_nLast, nNear);
		s_nLast = nNear;
		return;
	}
	if (dLast >= s_nSpacing)
	{
		int n = AddPoint(x, y, s_nLast);
		if (n < 0)
		{
			SystemMessage("<color=red>MapPath: day diem, dung ghi");
			s_bRec = FALSE;
			return;
		}
		s_nLast = n;
	}
}

// Thu di vao cong: qua map => ghi cong tai toa do thu (map cu) + diem vao, cong chieu ve (map moi)
static void OnDoorTestDone(int nOldMap, int nNewMap, int x, int y)
{
	char szMsg[160];
	s_bDoorTest = FALSE;
	if (s_G.nMapID != nOldMap)
	{
		if (s_G.nMapID && s_G.bDirty) SaveGraph(&s_G);
		LoadGraph(&s_G, nOldMap);
	}
	int nPoint = PointAt(s_ptDoor.x, s_ptDoor.y, 64, s_nDoorLink < s_G.nPoints ? s_nDoorLink : -1);
	AddDoor(nPoint, nNewMap, x, y, door_walk, "");
	SaveGraph(&s_G);
	sprintf(szMsg, "<color=green>MapPath: THU CONG THANH CONG - diem %d (%d/%d) -> %s (%d)", nPoint,
		s_ptDoor.x >> 8, s_ptDoor.y >> 9, MapName(nNewMap), nNewMap);
	SystemMessage(szMsg);

	LoadGraph(&s_G, nNewMap);
	s_nLast = PointAt(x, y, s_nSpacing / 2, -1);
	if (s_nLast >= 0) AddDoor(s_nLast, nOldMap, s_ptDoor.x, s_ptDoor.y);
	SaveGraph(&s_G);
	s_LastTrans.nOldMap = nOldMap;
	s_LastTrans.nNewMap = nNewMap;
	s_LastTrans.bDriver = FALSE;
}

static void DoorTestBreathe(int x, int y)
{
	DWORD dwNow = GetTickCount();
	int d = __distance(s_ptDoor.x, s_ptDoor.y, x, y);
	if (d <= 32)
	{
		if (!s_dwDoorReach) s_dwDoorReach = dwNow;
		if (dwNow - s_dwDoorReach > 3000)
		{
			SystemMessage("<color=red>MapPath: da toi toa do 3s nhung KHONG chuyen map - khong phai cong");
			s_bDoorTest = FALSE;
		}
		return;
	}
	if (dwNow - s_dwDoorTick > 20000)
	{
		SystemMessage("<color=red>MapPath: 20s chua toi toa do thu (bi ket?) - dung thu cong");
		s_bDoorTest = FALSE;
		return;
	}
	int tx = s_ptDoor.x, ty = s_ptDoor.y;
	if (d > 400)
	{	// MoveTo moi lan toi da ~400
		tx = x + (s_ptDoor.x - x) * 400 / d;
		ty = y + (s_ptDoor.y - y) * 400 / d;
	}
	g_pCurPlayer->MoveTo(tx, ty);
}

void DebugPathBreathe()
{
	if (!s_bRec && s_nRunTarget < 0 && !s_bDoorTest) return;
	if (!IsPlayerOk()) return;

	if (s_bRec && theMain.GetMenuCount() > 0)
	{
		s_dwMenuTick = GetTickCount();
		s_nMenu = ReadMenu(s_szMenu);
	}

	int x, y, nMap = *g_pMapID;
	if (nMap != s_nCurMap)
	{
		if (s_nMapWait++ < 5) return;	// Cho nap map (1s)
		s_nMapWait = 0;
		GetPos(&x, &y);
		if (s_bDoorTest && s_nCurMap)
			OnDoorTestDone(s_nCurMap, nMap, x, y);
		else if (s_bRec && s_nCurMap && s_ptLast.x)
			OnMapChanged(s_nCurMap, s_ptLast, nMap, x, y);
		else
			EnsureGraph();
		if (s_nRunTarget >= 0)
		{
			SystemMessage("<color=red>MapPath: doi map, dung chay thu");
			s_nRunTarget = -1;
		}
		s_nCurMap = nMap;
	}
	s_nMapWait = 0;
	GetPos(&x, &y);
	if (!x && !y) return;
	s_ptLast.x = x;
	s_ptLast.y = y;

	EnsureGraph();
	if (s_bDoorTest)
	{
		DoorTestBreathe(x, y);
		return;	// khong tu ghi diem khi dang thu cong
	}
	if (s_nRunTarget >= 0)
		RunBreathe(x, y);
	if (s_bRec)
		RecBreathe(x, y);
}

//------------------------------------------------------------------ nhap file cu

// Doc cap "x,y" toi khi gap dong khong phai cap (vd "0") - nhu CVLMove/CVLMap::LoadMap
static int ReadPairs(FILE *pFile, POINT *pPts, int nMax)
{
	int n = 0, a, b;
	while (fscanf(pFile, "%d,%d\n", &a, &b) == 2)
	{
		if (n < nMax) { pPts[n].x = a; pPts[n].y = b; }
		n++;
	}
	return min(n, nMax);
}

static int ImportPointsSides(FILE *pFile, BOOL bCity)
{
	static POINT Pts[PT_MAX_POINTS];
	if (bCity) ReadPairs(pFile, Pts, PT_MAX_POINTS);	// NpcPoints - bo qua
	int nBase = s_G.nPoints;
	int nPts = ReadPairs(pFile, Pts, PT_MAX_POINTS);
	for (int i = 0; i < nPts; i++)
		AddPoint(Pts[i].x, Pts[i].y, -1);
	int nSides = ReadPairs(pFile, Pts, PT_MAX_POINTS);
	for (int i = 0; i < nSides; i++)
		AddSide(nBase + Pts[i].x, nBase + Pts[i].y);
	return nPts;
}

static void ImportOld()
{
	char szMapID[12], szName[64], szIni[MAX_PATH], szFile[MAX_PATH];
	sprintf(szMapID, "%d", *g_pMapID);
	int nTotal = 0;
	FILE *pFile;

	sprintf(szIni, "%s\\Maps\\MapMenu.ini", theMain.m_szCurFolder);
	GetPrivateProfileStringA("Map_File", szMapID, "", szName, sizeof(szName), szIni);
	if (szName[0])
	{
		sprintf(szFile, "%s\\Maps\\%s", theMain.m_szCurFolder, szName);
		if ((pFile = fopen(szFile, "r")) != NULL)
		{
			int n = ImportPointsSides(pFile, FALSE);
			fclose(pFile);
			DbgOut("Nhap %s: %d diem\n", szName, n);
			nTotal += n;
		}
		else DbgOut("Khong mo duoc %s\n", szFile);
	}

	sprintf(szFile, "%s\\Maps\\CityMaps\\City%d.map", theMain.m_szCurFolder, *g_pMapID);
	if ((pFile = fopen(szFile, "r")) != NULL)
	{
		int n = ImportPointsSides(pFile, TRUE);
		fclose(pFile);
		DbgOut("Nhap CityMaps\\City%d.map: %d diem (bo qua toa do NPC)\n", *g_pMapID, n);
		nTotal += n;
	}

	sprintf(szIni, "%s\\Maps\\PathList.ini", theMain.m_szCurFolder);
	GetPrivateProfileStringA("Path_File", szMapID, "", szName, sizeof(szName), szIni);
	if (szName[0])
	{
		sprintf(szFile, "%s\\Maps\\Paths\\%s", theMain.m_szCurFolder, szName);
		if ((pFile = fopen(szFile, "r")) != NULL)
		{
			int a, b, c, n = 0, nPrev = -1;
			while (fscanf(pFile, "%d,%d,%d\n", &a, &b, &c) == 3)
			{
				nPrev = AddPoint(a, b, nPrev);
				n++;
			}
			fclose(pFile);
			DbgOut("Nhap Paths\\%s: %d diem (duong toi cong - map dich chua biet, dung nut Danh dau cong)\n", szName, n);
			nTotal += n;
		}
	}
	if (!nTotal)
		DbgOut("Map %d khong co file cu (MapMenu.ini / CityMaps / PathList.ini)\n", *g_pMapID);
}

//------------------------------------------------------------------ BFS giua cac map

struct PT_LINK { int nFrom, nTo, nPoint; BOOL bDriver; char szMenu[64]; int nKind; };	// nPoint < 0: dong menu xa phu dang mo (chua ghi diem)

#define PT_MAX_LINKS	2048
#define PT_MAX_STEPS	64
static PT_LINK	s_Links[PT_MAX_LINKS];
static int		s_nLinks = 0;

// Doc tat ca Maps\Path\*.txt -> s_Links (cong + xa phu). Tra ve so file
static int LoadLinks()
{
	static PT_GRAPH g;
	s_nLinks = 0;
	char szFind[MAX_PATH];
	sprintf(szFind, "%s\\Maps\\Path\\*.txt", theMain.m_szCurFolder);
	WIN32_FIND_DATAA fd;
	HANDLE hFind = FindFirstFileA(szFind, &fd);
	int nFiles = 0;
	if (hFind == INVALID_HANDLE_VALUE) return 0;
	do
	{
		int nMap = atoi(fd.cFileName);
		if (nMap <= 0) continue;
		if (nMap == s_G.nMapID) memcpy(&g, &s_G, sizeof(g));	// dung ban dang sua
		else if (!LoadGraph(&g, nMap)) continue;
		nFiles++;
		for (int i = 0; i < g.nDoors && s_nLinks < PT_MAX_LINKS; i++)
		{
			if (!g.door[i].nDestMap) continue;	// cong chua biet map dich
			PT_LINK &l = s_Links[s_nLinks++];
			l.nFrom = nMap; l.nTo = g.door[i].nDestMap; l.nPoint = g.door[i].nPoint; l.bDriver = FALSE;
			l.nKind = g.door[i].nKind; strcpy(l.szMenu, g.door[i].szName);
		}
		for (int i = 0; i < g.nDrivers && s_nLinks < PT_MAX_LINKS; i++)
		{
			PT_LINK &l = s_Links[s_nLinks++];
			l.nFrom = nMap; l.nTo = g.drv[i].nDestMap; l.nPoint = g.drv[i].nPoint; l.bDriver = TRUE; l.nKind = 0;
			strcpy(l.szMenu, g.drv[i].szMenu);
		}
	} while (FindNextFileA(hFind, &fd));
	FindClose(hFind);
	return nFiles;
}

// BFS theo so lan doi map tren pExtra (xet truoc) + s_Links. pSteps nhan cac buoc theo thu tu di.
// Tra ve so buoc (0 = dang o nTarget), -1 = khong co duong
static int MapBFS(int nStart, int nTarget, PT_LINK *pSteps, const PT_LINK *pExtra = NULL, int nExtra = 0)
{
	static int Queue[PT_MAX_LINKS], PrevLink[PT_MAX_LINKS], Maps[PT_MAX_LINKS];
	int nAll = s_nLinks + nExtra;
	int nMaps = 0, qh = 0, qt = 0;
	Maps[nMaps] = nStart; PrevLink[nMaps] = -1; nMaps++;
	Queue[qt++] = 0;
	int nFound = (nStart == nTarget) ? 0 : -1;
	while (qh < qt && nFound < 0)
	{
		int mi = Queue[qh++];
		for (int k = 0; k < nAll && nFound < 0; k++)
		{
			const PT_LINK &l = (k < nExtra) ? pExtra[k] : s_Links[k - nExtra];
			if (l.nFrom != Maps[mi] || !l.nTo) continue;
			int j;
			for (j = 0; j < nMaps && Maps[j] != l.nTo; j++);
			if (j < nMaps || nMaps >= PT_MAX_LINKS) continue;
			Maps[nMaps] = l.nTo;
			PrevLink[nMaps] = k * 4096 + mi;	// link + map truoc
			if (l.nTo == nTarget) nFound = nMaps;
			Queue[qt++] = nMaps++;
		}
	}
	if (nFound < 0) return -1;
	int Rev[PT_MAX_STEPS], nSteps = 0;
	for (int m = nFound; PrevLink[m] >= 0 && nSteps < PT_MAX_STEPS; m = PrevLink[m] % 4096)
		Rev[nSteps++] = PrevLink[m] / 4096;
	for (int s = 0; s < nSteps; s++)
	{
		int k = Rev[nSteps - 1 - s];
		pSteps[s] = (k < nExtra) ? pExtra[k] : s_Links[k - nExtra];
	}
	return nSteps;
}

static void MapRoute(int nTarget)
{
	static PT_LINK Steps[PT_MAX_STEPS];
	int nFiles = LoadLinks();
	DbgOut("Doc %d file Maps\\Path\\*.txt, %d lien ket (cong + xa phu)\n", nFiles, s_nLinks);
	int nStart = *g_pMapID;
	int nSteps = MapBFS(nStart, nTarget, Steps);
	if (nSteps < 0)
	{
		DbgOut("KHONG tim duoc lo trinh %s (%d) -> %s (%d) - thieu du lieu cong/xa phu\n",
			MapName(nStart), nStart, MapName(nTarget), nTarget);
		return;
	}
	DbgOut("Lo trinh %s (%d) -> %s (%d): %d buoc\n", MapName(nStart), nStart, MapName(nTarget), nTarget, nSteps);
	for (int s = 0; s < nSteps; s++)
	{
		const PT_LINK &l = Steps[s];
		if (l.bDriver)
			DbgOut("  %d. %s (%d): toi xa phu diem %d, chon \"%s\" -> %s (%d)\n", s + 1, MapName(l.nFrom), l.nFrom, l.nPoint, l.szMenu, MapName(l.nTo), l.nTo);
		else
			DbgOut("  %d. %s (%d): chay toi cong %s diem %d -> %s (%d)\n", s + 1, MapName(l.nFrom), l.nFrom, DoorKindName(l.nKind), l.nPoint, MapName(l.nTo), l.nTo);
	}
	g_DbgReply.bOK = TRUE;
}

//------------------------------------------------------------------ quet cong

static BOOL IsDoorObjKind(int nKind)
{
	return nKind != 1 && nKind != 3 && nKind != 4;	// bo xac NPC (Body), vat pham, tien
}

static const char* ObjKindName(int nKind)
{
	static const char *s_szKind[] = { "MapObj", "Body", "Box", "Item", "Money", "LoopSnd", "RandSnd", "Light", "Door", "Trap", "Prop" };
	return (nKind >= 0 && nKind < 11) ? s_szKind[nKind] : "?";
}

static void ScanDoors(int nRange)
{
	int px, py;
	GetPos(&px, &py);
	DbgOut("Quet cong quanh (%d, %d) [%d/%d], pham vi %d\n", px, py, px >> 8, py >> 9, nRange);

	PT_TRAP Traps[PT_MAX_TRAPS];
	int nTraps = ScanTraps(px, py, nRange, Traps);
	DbgOut("\nRegion dang nap (g_ScenePlace 0x%08X + 0x%X, %d):\n", SCP_BASE_ADD, SCP_REGIONS_OFF, SCP_NUM_REGIONS);
	for (int r = 0; r < SCP_NUM_REGIONS; r++)
	{
		BYTE *pReg = *(BYTE**)((BYTE*)SCP_BASE_ADD + SCP_REGIONS_OFF + r * 4);
		if (!pReg) { DbgOut("  [%2d] NULL\n", r); continue; }
		int nLeft = *(int*)(pReg + SPR_LEFTTOP_OFF), nTop = *(int*)(pReg + SPR_LEFTTOP_OFF + 4);
		const DWORD *pTrap = (const DWORD*)(pReg + SPR_TRAP_OFF);
		int nCells = 0;
		for (int i = 0; i < 16 * 32; i++) if (pTrap[i]) nCells++;
		DbgOut("  [%2d] 0x%08X status %d  RegionIdx (%d,%d)  goc (%d, %d)  o trap %d%s\n", r, (DWORD)pReg,
			*(int*)(pReg + SPR_STATUS_OFF), *(int*)(pReg + 0x10), *(int*)(pReg + 0x14), nLeft, nTop, nCells,
			(px >= nLeft && px < nLeft + 512 && py >= nTop && py < nTop + 1024) ? "  <== nhan vat" : "");
	}
	DbgOut("\n1. TRAP ban do (di len la chuyen map) - Region_C.dat: %d\n", nTraps);
	for (int k = 0; k < nTraps; k++)
	{
		const PT_TRAP &t = Traps[k];
		DbgOut("  ID 0x%08X  %d o  gan nhat (%d, %d) [%d/%d] cach %d  tam (%d, %d)\n", t.dwID, t.nCells,
			t.xNear, t.yNear, t.xNear >> 8, t.yNear >> 9, t.dNear, t.xSum / t.nCells, t.ySum / t.nCells);
	}

	DbgOut("\n2. OBJ (click de qua map):\n");
	int nCount = 0;
	for (int i = 1; i < MAX_OBJ; i++)
	{
		KObj *pObj = g_pObjArray[i];
		if (pObj->m_nID == -1 || !IsDoorObjKind(pObj->m_Kind)) continue;
		int ox, oy;
		pObj->GetMapPos(&ox, &oy);
		int d = __distance(ox, oy, px, py);
		if (d > nRange) continue;
		DbgOut("  [%d] %-7s %-24s (%d, %d) [%d/%d] cach %d  script 0x%08X\n", i, ObjKindName(pObj->m_Kind), pObj->m_szName,
			ox, oy, ox >> 8, oy >> 9, d, pObj->m_dwScriptID);
		nCount++;
	}
	if (!nCount) DbgOut("  (khong co)\n");

	DbgOut("\n3. NPC (doi thoai de qua map):\n");
	nCount = 0;
	for (int i = 1; i < MAX_NPC; i++)
	{
		KNpc *pNpc = g_pNpcArray[i];
		if (!pNpc->IsExist() || i == g_pCurPlayer->m_nIndex || pNpc->m_Kind == kind_player) continue;
		int nx, ny;
		pNpc->GetMapPos(&nx, &ny);
		int d = __distance(nx, ny, px, py);
		if (d > nRange) continue;
		DbgOut("  [%d] %s%-24s (%d, %d) [%d/%d] cach %d\n", i, pNpc->m_Kind == kind_dialoger ? "doi thoai " : "kind " ,
			pNpc->Name, nx, ny, nx >> 8, ny >> 9, d);
		nCount++;
	}
	if (!nCount) DbgOut("  (khong co)\n");
	DbgOut("\nGhi cong: nut 'Ghi cong trap/Obj/NPC gan nhat' (Idx = MapID dich, 0 = chua biet - tu dien khi qua cong)\n");
	g_DbgReply.nResult[0] = nTraps;
	g_DbgReply.bOK = TRUE;
}

// Them diem cong tai (x,y), noi voi diem truoc (hoac diem gan nhat), ghi cong
static void MarkDoor(int x, int y, int nDestMap, int nKind, const char *szName)
{
	int nLink = s_nLast;
	if (nLink < 0)
	{
		int px, py, d;
		GetPos(&px, &py);
		nLink = Nearest(px, py, &d);
		if (d > PT_KEY_RANGE) nLink = -1;
	}
	int n = PointAt(x, y, 64, nLink);
	if (n < 0) { DbgOut("LOI: day diem\n"); return; }
	AddDoor(n, nDestMap, 0, 0, nKind, szName);
	DbgOut("Diem %d (%d, %d) [%d/%d] = cong %s%s%s -> %s (%d)%s\n", n, x, y, x >> 8, y >> 9, DoorKindName(nKind),
		szName[0] ? " " : "", szName, nDestMap ? MapName(nDestMap) : "chua biet", nDestMap,
		nLink >= 0 ? "" : "  (chua noi voi diem nao - dung phim End/Space de noi)");
	if (nLink >= 0) DbgOut("Noi voi diem %d\n", nLink);
	if (!nDestMap) DbgOut("Map dich se tu dien khi bat tu ghi va di qua cong nay.\n");
	g_DbgReply.bOK = TRUE;
}

//------------------------------------------------------------------ lenh DEBUG

static void OutStatus()
{
	int x, y, d;
	GetPos(&x, &y);
	int nNear = Nearest(x, y, &d);
	WCHAR wszFile[MAX_PATH];
	BOOL bHave = FindPathFile(s_G.nMapID, wszFile);
	DbgOut("Map %d (%s)  file %s%s\n", *g_pMapID, MapName(*g_pMapID), PathFileShow(s_G.nMapID),
		bHave ? "" : "  (chua co)");
	DbgOut("Tu ghi: %s  khoang cach %d  diem truoc s_nLast = %d  Ve node: %s (patch %d)\n", s_bRec ? "DANG GHI" : "tat",
		s_nSpacing, s_nLast, s_bShow ? "BAT" : "tat", s_bPatched);
	DbgOut("Do thi: %d diem, %d canh, %d cong, %d xa phu  %s\n", s_G.nPoints, s_G.nSides, s_G.nDoors, s_G.nDrivers,
		s_G.bDirty ? "(CHUA GHI FILE)" : "");
	DbgOut("Vi tri (%d, %d) [%d/%d]  diem gan nhat %d (cach %d)\n", x, y, x >> 8, y >> 9, nNear, nNear >= 0 ? d : 0);
	if (s_nRunTarget >= 0)
		DbgOut("Chay thu toi diem %d: buoc %d/%d\n", s_nRunTarget, s_nRouteIdx, s_nRouteLen);
	if (s_LastTrans.nOldMap)
		DbgOut("Doi map vua ghi: %d -> %d (%s)\n", s_LastTrans.nOldMap, s_LastTrans.nNewMap, s_LastTrans.bDriver ? "xa phu" : "cong");
	g_DbgReply.nResult[0] = x;
	g_DbgReply.nResult[1] = y;
	g_DbgReply.bOK = TRUE;
}

static void OutGraph()
{
	DbgOut("Map %d (%s): %d diem, %d canh\n", s_G.nMapID, MapName(s_G.nMapID), s_G.nPoints, s_G.nSides);
	for (int i = 0; i < s_G.nPoints; i++)
	{
		DbgOut("  %3d (%d, %d) [%d/%d] ->", i, s_G.pt[i].x, s_G.pt[i].y, s_G.pt[i].x >> 8, s_G.pt[i].y >> 9);
		for (int k = 0; k < s_G.nSides; k++)
		{
			if (s_G.side[k][0] == i) DbgOut(" %d", s_G.side[k][1]);
			else if (s_G.side[k][1] == i) DbgOut(" %d", s_G.side[k][0]);
		}
		for (int k = 0; k < s_G.nDoors; k++)
			if (s_G.door[k].nPoint == i)
				DbgOut("  [CONG %s%s%s -> %s (%d)]", DoorKindName(s_G.door[k].nKind), s_G.door[k].szName[0] ? " " : "", s_G.door[k].szName,
					s_G.door[k].nDestMap ? MapName(s_G.door[k].nDestMap) : "chua biet", s_G.door[k].nDestMap);
		for (int k = 0; k < s_G.nDrivers; k++)
			if (s_G.drv[k].nPoint == i) DbgOut("  [XA PHU -> %s (%d) \"%s\"]", MapName(s_G.drv[k].nDestMap), s_G.drv[k].nDestMap, s_G.drv[k].szMenu);
		DbgOut("\n");
	}
	g_DbgReply.bOK = TRUE;
}

static int NearestToPlayer(int *pDist = NULL)
{
	int x, y;
	GetPos(&x, &y);
	return Nearest(x, y, pDist);
}

static BOOL CheckPointIdx(int n)
{
	if (n < 0 || n >= s_G.nPoints)
	{
		DbgOut("LOI: Idx = %d ngoai 0..%d\n", n, s_G.nPoints - 1);
		return FALSE;
	}
	return TRUE;
}

static void UndoTransition()
{
	if (!s_LastTrans.nOldMap)
	{
		DbgOut("Chua ghi lan doi map nao\n");
		return;
	}
	int nOld = s_LastTrans.nOldMap, nNew = s_LastTrans.nNewMap;
	// Map moi: cong chieu ve
	if (s_G.nMapID == nNew)
		for (int k = s_G.nDoors - 1; k >= 0; k--)
			if (s_G.door[k].nDestMap == nOld)
			{
				s_G.door[k] = s_G.door[--s_G.nDoors];
				s_G.bDirty = TRUE;
				DbgOut("Xoa cong chieu ve (map %d -> %d)\n", nNew, nOld);
				break;
			}
	// Map cu: cong / xa phu
	static PT_GRAPH g;
	if (LoadGraph(&g, nOld))
	{
		BOOL bDel = FALSE;
		if (s_LastTrans.bDriver)
		{
			for (int k = g.nDrivers - 1; k >= 0 && !bDel; k--)
				if (g.drv[k].nDestMap == nNew) { g.drv[k] = g.drv[--g.nDrivers]; bDel = TRUE; }
		}
		else
		{
			for (int k = g.nDoors - 1; k >= 0 && !bDel; k--)
				if (g.door[k].nDestMap == nNew) { g.door[k] = g.door[--g.nDoors]; bDel = TRUE; }
		}
		if (bDel && SaveGraph(&g))
			DbgOut("Xoa %s map %d -> %d trong file map %d\n", s_LastTrans.bDriver ? "xa phu" : "cong", nOld, nNew, nOld);
	}
	s_LastTrans.nOldMap = 0;
	g_DbgReply.bOK = TRUE;
}

BOOL DebugPathCmd(const DEBUG_PARAM *pParam)
{
	if (pParam->nCmd < dbg_pt_status || pParam->nCmd >= dbg_pt_last)
		return FALSE;
	if (!DbgCheckPlayer()) return TRUE;
	EnsureGraph();
	if (!s_nCurMap) s_nCurMap = *g_pMapID;

	const int *P = pParam->nParam;
	int x, y, d, n;
	GetPos(&x, &y);
	switch (pParam->nCmd)
	{
	case dbg_pt_status:
		OutStatus();
		break;

	case dbg_pt_rec:
		if (s_bRec)
		{
			s_bRec = FALSE;
			if (s_G.bDirty) SaveGraph(&s_G);
			DbgOut("TAT tu ghi - da ghi file (%d diem, %d canh)\n", s_G.nPoints, s_G.nSides);
			SystemMessage("<color=yellow>MapPath: tat tu ghi");
		}
		else
		{
			s_nSpacing = P[2] > 50 ? P[2] : PT_SPACING;
			s_nLast = -1;
			s_nCurMap = *g_pMapID;
			s_ptLast.x = s_ptLast.y = 0;
			s_dwMenuTick = 0;
			s_bRec = TRUE;
			DbgOut("BAT tu ghi: cu %d don vi them 1 diem. Chay bo theo duong, qua cong / dung xa phu se tu ghi.\n", s_nSpacing);
			DbgOut("Re nhanh: chay qua diem cu se tu noi vao. Bam lai nut nay de tat (tu ghi file).\n");
			SystemMessage("<color=yellow>MapPath: BAT tu ghi duong di");
		}
		g_DbgReply.bOK = TRUE;
		break;

	case dbg_pt_addpoint:
		n = AddPoint(x, y, s_nLast);
		if (n < 0) { DbgOut("LOI: day diem (%d)\n", PT_MAX_POINTS); break; }
		DbgOut("Them diem %d (%d, %d) noi voi diem %d\n", n, x, y, s_nLast);
		s_nLast = n;
		g_DbgReply.bOK = TRUE;
		break;

	case dbg_pt_undo:
		if (s_G.nPoints <= 0) { DbgOut("Khong co diem\n"); break; }
		DelPoint(s_G.nPoints - 1);
		s_nLast = s_G.nPoints - 1;
		DbgOut("Xoa diem cuoi. Con %d diem, diem truoc = %d\n", s_G.nPoints, s_nLast);
		g_DbgReply.bOK = TRUE;
		break;

	case dbg_pt_delnear:
		n = NearestToPlayer(&d);
		if (n < 0) { DbgOut("Khong co diem\n"); break; }
		DelPoint(n);
		DbgOut("Xoa diem %d (cach %d) cung cac canh/cong/xa phu cua no - chi so cac diem sau giam 1\n", n, d);
		g_DbgReply.bOK = TRUE;
		break;

	case dbg_pt_setlast:
		n = NearestToPlayer(&d);
		if (n < 0) { DbgOut("Khong co diem\n"); break; }
		s_nLast = n;
		DbgOut("Diem truoc = %d (cach %d) - diem ghi tiep theo se noi vao diem nay\n", n, d);
		g_DbgReply.bOK = TRUE;
		break;

	case dbg_pt_link:
	case dbg_pt_unlink:
		n = NearestToPlayer(&d);
		if (n < 0 || !CheckPointIdx(P[2])) break;
		if (pParam->nCmd == dbg_pt_link)
			DbgOut(AddSide(n, P[2]) ? "Noi diem %d - %d\n" : "LOI: khong noi duoc %d - %d\n", n, P[2]);
		else
			DbgOut(DelSide(n, P[2]) ? "Bo noi diem %d - %d\n" : "Diem %d - %d khong noi voi nhau\n", n, P[2]);
		g_DbgReply.bOK = TRUE;
		break;

	case dbg_pt_door:
		if (P[2] <= 0) { DbgOut("LOI: nhap MapID dich vao o Idx/So\n"); break; }
		n = PointAt(x, y, 150, -1);
		AddDoor(n, P[2], 0, 0);
		DbgOut("Diem %d la cong sang %s (%d)\n", n, MapName(P[2]), P[2]);
		g_DbgReply.bOK = TRUE;
		break;

	case dbg_pt_driver:
	{
		char szMenu[PT_MAX_MENU][64];
		int nMenu = ReadMenu(szMenu);
		if (!nMenu) { DbgOut("LOI: chua mo hoi thoai xa phu\n"); break; }
		n = PointAt(x, y, 400, -1);
		int nAdd = 0;
		for (int i = 0; i < nMenu; i++)
		{
			int nMap = MatchMapID(szMenu[i]);
			if (nMap && nMap != *g_pMapID)
			{
				AddDriver(n, nMap, szMenu[i]);
				DbgOut("  [%d] %s  -> %s (%d)\n", i, szMenu[i], MapName(nMap), nMap);
				nAdd++;
			}
			else
				DbgOut("  [%d] %s  (khong khop ten map - bo qua)\n", i, szMenu[i]);
		}
		DbgOut("Xa phu tai diem %d: ghi %d map. Mo menu con (vd Nhung noi da di qua) roi bam lai de ghi them.\n", n, nAdd);
		g_DbgReply.bOK = TRUE;
		break;
	}

	case dbg_pt_undotrans:
		UndoTransition();
		break;

	case dbg_pt_list:
		OutGraph();
		break;

	case dbg_pt_save:
		if (SaveGraph(&s_G))
		{
			DbgOut("Ghi %s: %d diem, %d canh, %d cong, %d xa phu\n", PathFileShow(s_G.nMapID), s_G.nPoints, s_G.nSides, s_G.nDoors, s_G.nDrivers);
			g_DbgReply.bOK = TRUE;
		}
		else DbgOut("LOI: khong ghi duoc file\n");
		break;

	case dbg_pt_load:
		if (!LoadGraph(&s_G, *g_pMapID)) DbgOut("Chua co file cho map %d - do thi rong\n", *g_pMapID);
		else DbgOut("Doc lai file: %d diem, %d canh, %d cong, %d xa phu\n", s_G.nPoints, s_G.nSides, s_G.nDoors, s_G.nDrivers);
		s_nLast = -1;
		g_DbgReply.bOK = TRUE;
		break;

	case dbg_pt_import:
		ImportOld();
		DbgOut("Do thi: %d diem, %d canh (chua ghi file - bam Ghi file)\n", s_G.nPoints, s_G.nSides);
		g_DbgReply.bOK = TRUE;
		break;

	case dbg_pt_clear:
		n = s_G.nMapID;
		memset(&s_G, 0, sizeof(s_G));
		s_G.nMapID = n;
		s_G.bDirty = TRUE;
		s_nLast = -1;
		s_nRunTarget = -1;
		DbgOut("Xoa do thi map %d (file chi bi ghi de khi bam Ghi file / doi map)\n", n);
		g_DbgReply.bOK = TRUE;
		break;

	case dbg_pt_route:
	case dbg_pt_run:
	{
		n = NearestToPlayer(&d);
		if (n < 0 || !CheckPointIdx(P[2])) break;
		int nDist = 0;
		int nLen = FindRoute(n, P[2], s_nRoute, &nDist);
		if (!nLen) { DbgOut("KHONG co duong tu diem %d toi diem %d (do thi bi dut)\n", n, P[2]); break; }
		DbgOut("Duong diem %d (cach nhan vat %d) -> %d: %d diem, dai %d\n  ", n, d, P[2], nLen, nDist);
		for (int i = 0; i < nLen; i++)
			DbgOut("%d%s", s_nRoute[i], i < nLen - 1 ? " > " : "\n");
		if (pParam->nCmd == dbg_pt_run)
		{
			s_nRouteLen = nLen;
			s_nRouteIdx = 0;
			s_nRunTarget = P[2];
			s_dwRunTick = GetTickCount();
			s_nCurMap = *g_pMapID;
			DbgOut("Bat dau chay thu (nut Dung de dung). Nen tat Auto danh/di chuyen khi thu.\n");
		}
		g_DbgReply.bOK = TRUE;
		break;
	}

	case dbg_pt_stop:
		DbgOut("Dung chay thu%s\n", s_bRec ? " (van dang tu ghi - bam nut Bat/Tat ghi de tat)" : "");
		s_nRunTarget = -1;
		g_DbgReply.bOK = TRUE;
		break;

	case dbg_pt_maproute:
		if (P[2] <= 0) { DbgOut("LOI: nhap MapID dich vao o Idx/So\n"); break; }
		if (s_G.bDirty) SaveGraph(&s_G);
		MapRoute(P[2]);
		break;

	case dbg_pt_scan:
		ScanDoors(P[2] > 0 ? P[2] : PT_SCAN_RANGE);
		break;

	case dbg_pt_doortrap:
	{
		PT_TRAP Traps[PT_MAX_TRAPS];
		int nTraps = ScanTraps(x, y, PT_SCAN_RANGE, Traps), nBest = -1;
		for (int k = 0; k < nTraps; k++)
			if (nBest < 0 || Traps[k].dNear < Traps[nBest].dNear) nBest = k;
		if (nBest < 0) { DbgOut("Khong co trap nao trong pham vi %d\n", PT_SCAN_RANGE); break; }
		char szName[32];
		sprintf(szName, "0x%08X", Traps[nBest].dwID);
		MarkDoor(Traps[nBest].xNear, Traps[nBest].yNear, P[2], door_walk, szName);
		break;
	}

	case dbg_pt_doorobj:
	case dbg_pt_doornpc:
	{
		int nBest = -1, dBest = PT_SCAN_RANGE + 1, bx = 0, by = 0;
		const char *szName = "";
		for (int i = 1; i < (pParam->nCmd == dbg_pt_doorobj ? MAX_OBJ : MAX_NPC); i++)
		{
			int ox, oy;
			if (pParam->nCmd == dbg_pt_doorobj)
			{
				KObj *pObj = g_pObjArray[i];
				if (pObj->m_nID == -1 || !IsDoorObjKind(pObj->m_Kind)) continue;
				pObj->GetMapPos(&ox, &oy);
			}
			else
			{
				KNpc *pNpc = g_pNpcArray[i];
				if (!pNpc->IsExist() || pNpc->m_Kind != kind_dialoger) continue;
				pNpc->GetMapPos(&ox, &oy);
			}
			d = __distance(ox, oy, x, y);
			if (d < dBest) { dBest = d; nBest = i; bx = ox; by = oy; }
		}
		if (nBest < 0) { DbgOut("Khong co %s nao trong pham vi %d\n", pParam->nCmd == dbg_pt_doorobj ? "Obj" : "NPC doi thoai", PT_SCAN_RANGE); break; }
		szName = pParam->nCmd == dbg_pt_doorobj ? g_pObjArray[nBest]->m_szName : g_pNpcArray[nBest]->Name;
		MarkDoor(bx, by, P[2], pParam->nCmd == dbg_pt_doorobj ? door_obj : door_npc, szName);
		break;
	}

	case dbg_pt_trydoor:
		if (s_bDoorTest)
		{
			s_bDoorTest = FALSE;
			DbgOut("Dung thu cong\n");
			g_DbgReply.bOK = TRUE;
			break;
		}
		{
			int tx = P[0], ty = P[1];
			if (!tx && !ty)
			{	// Trong X/Y: lay co cam tren ban do nho
				int nFlagX = *((int*)FLAG_X_OFFST), nFlagY = *((int*)FLAG_Y_OFFST);
				if (nFlagX <= 0 || nFlagY <= 0) { DbgOut("LOI: nhap X/Y (toa do tuyet doi) hoac cam co tren ban do nho\n"); break; }
				tx = nFlagX << 4; ty = nFlagY << 5;
				DbgOut("Lay toa do co cam\n");
			}
			else if (tx < 4096 && ty < 4096)
			{	// Toa do game (x/y hien tren man hinh) -> toa do that, giua o
				tx = (tx << 8) + 128; ty = (ty << 9) + 256;
				DbgOut("Toa do game %d/%d -> that (%d, %d)\n", P[0], P[1], tx, ty);
			}
			d = __distance(tx, ty, x, y);
			s_ptDoor.x = tx; s_ptDoor.y = ty;
			s_nDoorLink = s_nLast;
			if (s_nDoorLink < 0)
			{
				int dn, nn = Nearest(x, y, &dn);
				if (dn <= PT_KEY_RANGE) s_nDoorLink = nn;
			}
			s_dwDoorTick = GetTickCount();
			s_dwDoorReach = 0;
			s_nCurMap = *g_pMapID;
			s_nMapWait = 0;
			s_bDoorTest = TRUE;
			DbgOut("Thu di vao (%d, %d) [%d/%d], cach %d. Qua map => ghi cong (noi diem %d) + cong chieu ve.\n",
				tx, ty, tx >> 8, ty >> 9, d, s_nDoorLink);
			DbgOut("That bai neu toi noi 3s khong chuyen map, hoac 20s chua toi. Bam lai nut de dung.\n");
			if (d > 2000) DbgOut("CANH BAO: toa do cach xa (%d) - nen chay toi gan truoc\n", d);
			SystemMessage("<color=yellow>MapPath: dang thu di vao cong...");
			g_DbgReply.bOK = TRUE;
		}
		break;

	case dbg_pt_show:
		s_bShow = !s_bShow;
		if (s_bShow)
		{
			DbgOut("BAT ve node trong game + phim tat (khi khong giu Ctrl/Alt/Shift):\n");
			DbgOut("  Space  = them diem tai vi tri hien tai, noi diem truoc (gan diem cu < %d thi noi vao diem cu)\n", PT_JOIN_RANGE);
			DbgOut("  Delete = xoa diem gan nhat (<= %d) - diem do ve mau TRANG\n", PT_KEY_RANGE);
			DbgOut("  End    = chon diem gan nhat lam diem truoc (re nhanh)\n");
			DbgOut("Mau: vang = canh, xanh la = diem, xanh duong = diem truoc, do = cong, tim = xa phu, o CAM = trap ban do\n");
			DbgOut("LUU Y: khi bat, Space/Delete/End bi chiem (ke ca khi go chat). Bam lai nut nay de tat.\n");
			SystemMessage("<color=yellow>MapPath: BAT ve node - Space them, Delete xoa, End re nhanh");
		}
		else
		{
			if (s_G.bDirty) SaveGraph(&s_G);
			DbgOut("TAT ve node + phim tat (da ghi file)\n");
			SystemMessage("<color=yellow>MapPath: TAT ve node");
		}
		g_DbgReply.bOK = TRUE;
		break;
	}
	if ((s_bShow || s_bRec || s_nRunTarget >= 0 || s_bDoorTest) && !PatchDraw(TRUE))
		DbgOut("\nLOI: khong chen duoc ham ve (0x%08X khong phai jmp/call 0x%08X) - sai ban game?\n", DGS_JMP_ADD, DGS_FUNC_ADD);
	return TRUE;
}

//------------------------------------------------------------------ tu di theo MapPath (Auto)
// CVLMap::GetDriver: menu "Nhung noi da di qua" khong co map train -> PathTravelStart(map train, TRUE).
// Moi lan vao map moi: BFS lai tu map hien tai, lam buoc dau tien:
//   chay theo diem/canh toi diem cong/xa phu -> di vao trap / click Obj / noi NPC / chon menu xa phu.
// Do thi rieng s_T (khong dung chung s_G cua tab ghi). Trong luc chay CVLMap/CVLMove dung lai.

#define TV_STEP_TIMEOUT		20000	// 20s khong toi duoc diem tiep theo
#define TV_ACT_TIMEOUT		15000	// 15s o cong/xa phu ma chua qua map
#define TV_MAX_FAILS		3
#define TV_NPC_RANGE		400		// NPC/Obj cong quanh diem ghi

static int		s_nTvTarget = 0;		// 0 = khong chay
static int		s_nTvMap = 0;			// Map da lap ke hoach (0 = can lap lai)
static int		s_nTvMapWait = 0;
static PT_GRAPH	s_T;
static PT_LINK	s_TvLink;				// Buoc dang lam tren map hien tai
static int		s_nTvRoute[PT_MAX_POINTS], s_nTvLen = 0, s_nTvIdx = 0;
static int		s_nTvStage = 0;			// 0 chay toi diem, 1 tac dong cong/xa phu, 2 cho doi map
static DWORD	s_dwTvTick = 0;			// Lan tien trien gan nhat
static DWORD	s_dwTvAct = 0;			// Lan tac dong gan nhat (click NPC/Obj/menu)
static int		s_nTvFails = 0;
static POINT	s_ptTvGoal = { 0, 0 };	// Toa do diem cong/xa phu
static POINT	s_ptTvFrom = { 0, 0 };	// Diem truoc cong (huong di vao trap)
// Dong menu xa phu luc bat dau (map xuat phat) - dung lai khi lap ke hoach lai o map do
static PT_LINK	s_TvExtra[PT_MAX_MENU];
static int		s_nTvExtra = 0, s_nTvExtraMap = 0;
// Lan that bai gan nhat: khong thu lai cung map dich trong 5 phut (de Auto quay ve cach chon dong cu)
static int		s_nTvFailTarget = 0;
static DWORD	s_dwTvFailTick = 0;

static void TvMsg(const char *szFormat, ...)
{
	char szMsg[256];
	va_list va;
	va_start(va, szFormat);
	_vsnprintf(szMsg, sizeof(szMsg) - 1, szFormat, va);
	va_end(va);
	szMsg[sizeof(szMsg) - 1] = 0;
	SystemMessage(szMsg);
}

static void CloseMenu()
{
	KUiMsgSel *pMsgSel = (KUiMsgSel*)GetPtrAddress(MSG_BASE_ADD);
	KUiMsgSel2 *pMsgSel2 = (KUiMsgSel2*)GetPtrAddress(MSG2_BASE_ADD);
	if (pMsgSel2 && pMsgSel2->IsVisible()) pMsgSel2->OnClickMsg(pMsgSel2->m_nNumMessage);
	else if (pMsgSel && pMsgSel->IsVisible()) pMsgSel->OnClickMsg(pMsgSel->m_nNumMessage);
}

BOOL PathTravelActive()
{
	return s_nTvTarget != 0;
}

void PathTravelStop(const char *szReason)
{
	if (!s_nTvTarget) return;
	if (szReason)
	{
		TvMsg("<color=red>MapPath: %s - dung tu di", szReason);
		s_nTvFailTarget = s_nTvTarget;
		s_dwTvFailTick = GetTickCount();
	}
	s_nTvTarget = 0;
	s_nTvExtra = 0;
}

// Dong menu dang mo co dan toi map nTo? (ten trong MapNames.ini hoac dong menu da ghi)
static BOOL MenuLineFor(const char *szLine, const PT_LINK &l)
{
	if (l.szMenu[0] && !TcvnStrCmpI(szLine, l.szMenu)) return TRUE;
	if (MatchMapID(szLine) == l.nTo) return TRUE;
	const char *szName = MapName(l.nTo);
	return strlen(szName) > 3 && TcvnStrStrI(szLine, szName) != NULL;
}

// Toa do xa phu cho buoc khong co diem ghi: xa phu gan nhat trong file CityMaps (CVLMap)
static BOOL DriverPos(POINT *pt)
{
	int x, y, dMin = MAX_INTNUM;
	GetPos(&x, &y);
	for (int i = npc_Driver1; i <= npc_Driver4; i++)
	{
		POINT p = theMap.NpcPoints[i];
		if (!p.x && !p.y) continue;
		int d = __distance(x, y, p.x, p.y);
		if (d < dMin) { dMin = d; *pt = p; }
	}
	return dMin < MAX_INTNUM;
}

// Lap ke hoach tren map hien tai. FALSE = khong co duong (da dung)
static BOOL TvPlan()
{
	static PT_LINK Steps[PT_MAX_STEPS];
	int nMap = *g_pMapID;
	s_nTvMap = nMap;
	LoadLinks();
	int nSteps = (nMap == s_nTvExtraMap) ? MapBFS(nMap, s_nTvTarget, Steps, s_TvExtra, s_nTvExtra)
		: MapBFS(nMap, s_nTvTarget, Steps);
	if (nSteps < 0)
	{
		char szWhy[160];
		sprintf(szWhy, "khong co lo trinh %s -> %s", MapName(nMap), MapName(s_nTvTarget));
		PathTravelStop(szWhy);
		return FALSE;
	}
	s_TvLink = Steps[0];
	LoadGraph(&s_T, nMap);
	s_nTvLen = s_nTvIdx = 0;
	s_nTvStage = 0;
	s_dwTvTick = GetTickCount();
	s_dwTvAct = 0;

	int x, y;
	GetPos(&x, &y);
	s_ptTvFrom.x = x; s_ptTvFrom.y = y;
	if (s_TvLink.nPoint >= 0 && s_TvLink.nPoint < s_T.nPoints)
	{
		s_ptTvGoal = s_T.pt[s_TvLink.nPoint];
		int nStart = -1, dMin = MAX_INTNUM;
		for (int i = 0; i < s_T.nPoints; i++)
		{
			int d = __distance(s_T.pt[i].x, s_T.pt[i].y, x, y);
			if (d < dMin) { dMin = d; nStart = i; }
		}
		s_nTvLen = FindRoute(nStart, s_TvLink.nPoint, s_nTvRoute, NULL, &s_T);
		if (s_nTvLen >= 2)
			s_ptTvFrom = s_T.pt[s_nTvRoute[s_nTvLen - 2]];
	}
	else if (s_TvLink.bDriver)
	{	// Dong menu xa phu (chua ghi diem): toi xa phu gan nhat
		if (!DriverPos(&s_ptTvGoal)) { s_ptTvGoal.x = x; s_ptTvGoal.y = y; }
	}
	else
	{
		PathTravelStop("cong khong co diem trong file MapPath");
		return FALSE;
	}
	if (!s_nTvLen)
	{	// Khong co canh noi: chay thang toi cong/xa phu
		s_nTvRoute[0] = -1;
		s_nTvLen = 0;
	}

	if (s_TvLink.bDriver)
		TvMsg("<color=yellow>MapPath: %s -> xa phu chon \"%s\" -> %s (con %d buoc)", MapName(nMap),
			s_TvLink.szMenu[0] ? s_TvLink.szMenu : MapName(s_TvLink.nTo), MapName(s_TvLink.nTo), nSteps);
	else
		TvMsg("<color=yellow>MapPath: %s -> cong %s sang %s (con %d buoc)", MapName(nMap),
			DoorKindName(s_TvLink.nKind), MapName(s_TvLink.nTo), nSteps);
	return TRUE;
}

static void TvFail(const char *szWhy)
{
	if (++s_nTvFails >= TV_MAX_FAILS)
	{
		PathTravelStop(szWhy);
		return;
	}
	TvMsg("<color=yellow>MapPath: %s - thu lai (%d/%d)", szWhy, s_nTvFails, TV_MAX_FAILS);
	CloseMenu();
	s_nTvMap = 0;	// lap ke hoach lai
}

static void TvMoveTo(int x, int y, int tx, int ty)
{
	int d = __distance(tx, ty, x, y);
	if (d > 600)
	{	// MoveTo moi lan toi da ~600
		tx = x + (tx - x) * 600 / d;
		ty = y + (ty - y) * 600 / d;
	}
	PathNoteMove(tx, ty, "Qua map");
	g_pCurPlayer->MoveTo(tx, ty);
}

// Menu dang mo (xa phu / NPC cong): chon dong dan toi map dich. TRUE = da click
static BOOL TvClickMenu()
{
	char szMenu[PT_MAX_MENU][64];
	int n = ReadMenu(szMenu);
	for (int i = 0; i < n; i++)
		if (MenuLineFor(szMenu[i], s_TvLink)) { theMain.ClickMenuIdx(i); return TRUE; }
	if (theMain.ClickMenuStr(VISITED_PLACES)) return TRUE;
	if (theMain.ClickMenuStr("! Mau ")) return TRUE;										// "Dung vay! Mau dua ta di!"
	if (theMain.ClickMenuStr("X\xb8\x63 nh\xcbn")) return TRUE;							// Xac nhan
	return FALSE;
}

// Tim NPC doi thoai (xa phu / NPC cong) quanh diem cong
static int TvFindNpc()
{
	int nBest = 0, dMin = TV_NPC_RANGE;
	for (int i = 1; i < MAX_NPC; i++)
	{
		KNpc *pNpc = g_pNpcArray[i];
		if (!pNpc->IsExist() || i == g_pCurPlayer->m_nIndex || pNpc->m_Kind != kind_dialoger) continue;
		if (!s_TvLink.bDriver && s_TvLink.szMenu[0] && strcmp(pNpc->Name, s_TvLink.szMenu)) continue;
		int nx, ny;
		pNpc->GetMapPos(&nx, &ny);
		int d = __distance(nx, ny, s_ptTvGoal.x, s_ptTvGoal.y);
		if (d < dMin) { dMin = d; nBest = i; }
	}
	return nBest;
}

static int TvFindObj()
{
	int nBest = 0, dMin = TV_NPC_RANGE;
	for (int i = 1; i < MAX_OBJ; i++)
	{
		KObj *pObj = g_pObjArray[i];
		if (pObj->m_nID == -1 || !IsDoorObjKind(pObj->m_Kind)) continue;
		if (s_TvLink.szMenu[0] && strcmp(pObj->m_szName, s_TvLink.szMenu)) continue;
		int ox, oy;
		pObj->GetMapPos(&ox, &oy);
		int d = __distance(ox, oy, s_ptTvGoal.x, s_ptTvGoal.y);
		if (d < dMin) { dMin = d; nBest = i; }
	}
	return nBest;
}

static void TvAct(int x, int y)
{
	DWORD dwNow = GetTickCount();
	if (dwNow - s_dwTvTick > TV_ACT_TIMEOUT)
	{
		TvFail(s_TvLink.bDriver ? "xa phu khong dua toi map can toi" : "toi cong nhung khong qua map");
		return;
	}
	if (s_TvLink.bDriver || s_TvLink.nKind == door_npc)
	{
		if (theMain.GetMenuCount() > 0)
		{
			if (dwNow - s_dwTvAct < 1000) return;
			s_dwTvAct = dwNow;
			if (!TvClickMenu())
				TvFail("menu khong co dong toi map can toi");
			return;
		}
		if (s_dwTvAct && dwNow - s_dwTvAct < 3000) return;	// cho menu hien
		int nNpc = TvFindNpc();
		if (!nNpc) { TvMoveTo(x, y, s_ptTvGoal.x, s_ptTvGoal.y); return; }
		int nx, ny;
		g_pNpcArray[nNpc]->GetMapPos(&nx, &ny);
		if (__distance(nx, ny, x, y) > PLAYER_PICKUP_DISTANCE) { g_pCurPlayer->MoveTo(nx, ny); return; }
		theAttack.SetPeopleIdx(nNpc);
		s_dwTvAct = dwNow;
		return;
	}
	if (s_TvLink.nKind == door_obj)
	{
		if (s_dwTvAct && dwNow - s_dwTvAct < 2000) return;
		int nObj = TvFindObj();
		if (!nObj) { TvMoveTo(x, y, s_ptTvGoal.x, s_ptTvGoal.y); return; }
		g_pCurPlayer->PickUpObj(nObj);
		s_dwTvAct = dwNow;
		return;
	}
	// Trap: dung o diem cong; sau 2s chua qua map thi buoc them ve phia truoc (theo huong di toi cong)
	int d = __distance(s_ptTvGoal.x, s_ptTvGoal.y, x, y);
	if (!s_dwTvAct) s_dwTvAct = dwNow;
	DWORD dwStay = dwNow - s_dwTvAct;
	int tx = s_ptTvGoal.x, ty = s_ptTvGoal.y;
	if (dwStay > 2000)
	{
		int dx = s_ptTvGoal.x - s_ptTvFrom.x, dy = s_ptTvGoal.y - s_ptTvFrom.y;
		int dl = __distance(s_ptTvGoal.x, s_ptTvGoal.y, s_ptTvFrom.x, s_ptTvFrom.y);
		int nPush = dwStay > 6000 ? 256 : 128;
		if (dl > 0) { tx += dx * nPush / dl; ty += dy * nPush / dl; }
	}
	if (d > 16 || dwStay > 2000) g_pCurPlayer->MoveTo(tx, ty);
}

void PathTravelBreathe()
{
	if (!s_nTvTarget) return;
	if (theMain.m_bPause || !IsPlayerOk()) return;

	int nMap = *g_pMapID;
	if (nMap != s_nTvMap)
	{
		if (s_nTvMap && s_nTvMapWait++ < 5) return;	// Cho nap map (1s)
		s_nTvMapWait = 0;
		if (nMap == s_nTvTarget)
		{
			TvMsg("<color=green>MapPath: da toi %s", MapName(nMap));
			s_nTvTarget = 0;
			s_nTvExtra = 0;
			return;
		}
		if (s_nTvMap) s_nTvFails = 0;	// qua map thanh cong
		if (!TvPlan()) return;
	}
	if (s_nTvStage == 2)
	{	// Vua chon dong menu xa phu luc bat dau - cho doi map
		if (GetTickCount() - s_dwTvTick > TV_ACT_TIMEOUT) { s_nTvStage = 1; s_dwTvTick = GetTickCount(); }
		return;
	}
	if (theAttack.m_nEnemyIdx && P_PLAYER->m_FightMode) return;	// Dang danh quai doc duong

	int x, y;
	GetPos(&x, &y);
	if (!x && !y) return;

	if (s_nTvStage == 0)
	{
		if (s_nTvIdx >= s_nTvLen)
		{
			int d = __distance(s_ptTvGoal.x, s_ptTvGoal.y, x, y);
			if (d <= PT_REACH || (s_TvLink.bDriver || s_TvLink.nKind != door_walk) && d <= TV_NPC_RANGE / 2)
			{
				s_nTvStage = 1;
				s_dwTvTick = GetTickCount();
				return;
			}
			if (GetTickCount() - s_dwTvTick > TV_STEP_TIMEOUT) { TvFail("ket duong toi cong/xa phu"); return; }
			TvMoveTo(x, y, s_ptTvGoal.x, s_ptTvGoal.y);
			return;
		}
		POINT pt = s_T.pt[s_nTvRoute[s_nTvIdx]];
		if (__distance(pt.x, pt.y, x, y) <= PT_REACH)
		{
			s_nTvIdx++;
			s_dwTvTick = GetTickCount();
			return;
		}
		if (GetTickCount() - s_dwTvTick > TV_STEP_TIMEOUT) { TvFail("ket duong (20s khong toi diem tiep)"); return; }
		TvMoveTo(x, y, pt.x, pt.y);
		return;
	}
	TvAct(x, y);
}

BOOL PathTravelStart(int nTarget, BOOL bUseMenu)
{
	static PT_LINK Steps[PT_MAX_STEPS];
	int nMap = *g_pMapID;
	if (!nTarget || nMap == nTarget || !IsPlayerOk()) return FALSE;
	if (nTarget == s_nTvFailTarget && GetTickCount() - s_dwTvFailTick < 300000)
	{
		TvMsg("<color=red>MapPath: vua tu di toi %s that bai, cho %ds roi thu lai", MapName(nTarget),
			(300000 - (GetTickCount() - s_dwTvFailTick)) / 1000);
		return FALSE;
	}

	// Dong menu dang mo (vd "Nhung noi da di qua") = lien ket xa phu tu map hien tai
	s_nTvExtra = 0;
	if (bUseMenu)
	{
		char szMenu[PT_MAX_MENU][64];
		int n = ReadMenu(szMenu);
		for (int i = 0; i < n && s_nTvExtra < PT_MAX_MENU; i++)
		{
			int nTo = MatchMapID(szMenu[i]);
			if (!nTo || nTo == nMap) continue;
			PT_LINK &l = s_TvExtra[s_nTvExtra++];
			l.nFrom = nMap; l.nTo = nTo; l.nPoint = -1; l.bDriver = TRUE; l.nKind = 0;
			strcpy(l.szMenu, szMenu[i]);
		}
	}
	s_nTvExtraMap = nMap;

	LoadLinks();
	int nSteps = MapBFS(nMap, nTarget, Steps, s_TvExtra, s_nTvExtra);
	if (nSteps <= 0)
	{	// Bao ly do de bo sung du lieu MapPath
		char szLines[160] = "";
		for (int i = 0; i < s_nTvExtra; i++)
		{
			if (strlen(szLines) + strlen(MapName(s_TvExtra[i].nTo)) + 3 >= sizeof(szLines)) break;
			if (i) strcat(szLines, ", ");
			strcat(szLines, MapName(s_TvExtra[i].nTo));
		}
		TvMsg("<color=red>MapPath: khong co lo trinh %s (%d) -> %s (%d)", MapName(nMap), nMap, MapName(nTarget), nTarget);
		TvMsg("<color=red>MapPath: %d lien ket trong Maps\\Path, xa phu co: %s", s_nLinks, szLines[0] ? szLines : "(khong doc duoc)");
		s_nTvExtra = 0;
		return FALSE;
	}

	s_nTvTarget = nTarget;
	s_nTvFails = 0;
	s_nTvMapWait = 0;
	TvMsg("<color=yellow>MapPath: tu di %s -> %s (%d buoc)", MapName(nMap), MapName(nTarget), nSteps);
	if (bUseMenu && Steps[0].nPoint < 0 && theMain.GetMenuCount() > 0 && theMain.ClickMenuStr(Steps[0].szMenu))
	{	// Buoc dau la dong menu xa phu dang mo: chon luon
		TvMsg("<color=yellow>MapPath: xa phu chon \"%s\" -> %s", Steps[0].szMenu, MapName(Steps[0].nTo));
		s_TvLink = Steps[0];
		s_nTvMap = nMap;
		s_nTvStage = 2;
		s_dwTvTick = GetTickCount();
		return TRUE;
	}
	CloseMenu();
	s_nTvMap = 0;	// Lap ke hoach o PathTravelBreathe
	return TRUE;
}

// Trang thai tu di (cho nut Trang thai tab Ban do)
void PathTravelInfo(char *szInfo, int nMax)
{
	if (!s_nTvTarget)
	{
		_snprintf(szInfo, nMax, "khong chay. That bai gan nhat: map %d (%ds truoc)", s_nTvFailTarget,
			s_nTvFailTarget ? (int)((GetTickCount() - s_dwTvFailTick) / 1000) : 0);
	}
	else
	{
		const char *szStage = s_nTvStage == 0 ? "chay toi cong/xa phu" : s_nTvStage == 1 ? "tac dong cong/xa phu" : "cho doi map";
		_snprintf(szInfo, nMax, "DANG CHAY toi %s (%d). Map ke hoach %d, buoc %s -> %s (diem %d), diem %d/%d, giai doan: %s, %ds, that bai %d",
			MapName(s_nTvTarget), s_nTvTarget, s_nTvMap, s_TvLink.bDriver ? "xa phu" : DoorKindName(s_TvLink.nKind),
			MapName(s_TvLink.nTo), s_TvLink.nPoint, s_nTvIdx, s_nTvLen, szStage, (int)((GetTickCount() - s_dwTvTick) / 1000), s_nTvFails);
	}
	szInfo[nMax - 1] = 0;
}

//------------------------------------------------------------------ di trong 1 map theo MapPath (Auto)
// Vi tri hien tai -> diem MapPath gan nhat -> theo canh toi diem MapPath gan (tx,ty) nhat.
// Tra ve 1 = dang di, 0 = da toi diem cuoi duong (con doan ngan toi (tx,ty)), -1 = map khong co MapPath / khong noi duoc

static PT_GRAPH	s_W;
static int		s_nWMap = 0;
static POINT	s_ptWTarget = { 0, 0 };
static int		s_nWRoute[PT_MAX_POINTS], s_nWLen = -1, s_nWIdx = 0;
static DWORD	s_dwWTick = 0;
static int		s_nWReplan = 0;

int PathWalkTo(int tx, int ty)
{
	if (!IsPlayerOk()) return 1;
	int nMap = *g_pMapID;
	if (nMap != s_nWMap || tx != s_ptWTarget.x || ty != s_ptWTarget.y)
	{
		s_nWMap = nMap;
		s_ptWTarget.x = tx; s_ptWTarget.y = ty;
		s_nWLen = -1;
		s_nWReplan = 0;
	}
	int x, y;
	GetPos(&x, &y);
	if (s_nWLen < 0)
	{	// Lap duong (doc lai file - co the vua sua o tab Debug)
		if (s_nWReplan++ > 3) return -1;
		if (!LoadGraph(&s_W, nMap) || s_W.nPoints <= 0) return -1;
		int nStart = -1, nEnd = -1, dS = MAX_INTNUM, dE = MAX_INTNUM;
		for (int i = 0; i < s_W.nPoints; i++)
		{
			if (!s_W.pt[i].x && !s_W.pt[i].y) continue;
			int d = __distance(s_W.pt[i].x, s_W.pt[i].y, x, y);
			if (d < dS) { dS = d; nStart = i; }
			d = __distance(s_W.pt[i].x, s_W.pt[i].y, tx, ty);
			if (d < dE) { dE = d; nEnd = i; }
		}
		if (nStart < 0 || nEnd < 0) return -1;
		s_nWLen = FindRoute(nStart, nEnd, s_nWRoute, NULL, &s_W);
		if (s_nWLen <= 0)
		{
			s_nWLen = -1;
			char szMsg[128];
			sprintf(szMsg, "<color=red>MapPath: diem %d va %d khong noi voi nhau trong %s", nStart, nEnd, MapName(nMap));
			SystemMessage(szMsg);
			s_nWReplan = 99;
			return -1;
		}
		s_nWIdx = 0;
		// Da nam giua diem 0 va 1 thi khong quay lai diem 0
		if (s_nWLen >= 2)
		{
			POINT p0 = s_W.pt[s_nWRoute[0]], p1 = s_W.pt[s_nWRoute[1]];
			if (__distance(x, y, p1.x, p1.y) < __distance(p0.x, p0.y, p1.x, p1.y)) s_nWIdx = 1;
		}
		s_dwWTick = GetTickCount();
	}
	while (s_nWIdx < s_nWLen)
	{
		POINT pt = s_W.pt[s_nWRoute[s_nWIdx]];
		int d = __distance(pt.x, pt.y, x, y);
		if (d > PT_REACH) break;
		s_nWIdx++;
		s_dwWTick = GetTickCount();
	}
	if (s_nWIdx >= s_nWLen) return 0;
	if (GetTickCount() - s_dwWTick > 15000)
	{	// 15s khong toi duoc diem tiep: lap lai tu vi tri hien tai
		SystemMessage("<color=yellow>MapPath: ket duong, tinh lai duong di");
		s_nWLen = -1;
		return 1;
	}
	POINT pt = s_W.pt[s_nWRoute[s_nWIdx]];
	int d = __distance(pt.x, pt.y, x, y);
	int mx = pt.x, my = pt.y;
	if (d > 600) { mx = x + (pt.x - x) * 600 / d; my = y + (pt.y - y) * 600 / d; }
	PathNoteMove(mx, my, "MapPath");
	g_pCurPlayer->MoveTo(mx, my);
	return 1;
}

void PathWalkReset()
{
	s_nWMap = 0;
	s_nWLen = -1;
}

//------------------------------------------------------------------ ve duong Auto dang di (tab Ban do: nut Ve duong di)
// Xam = canh MapPath; xanh duong = duong PathWalkTo con lai, xanh than = da di; trang = toi diem dang nham;
// do (dau X) = lenh MoveTo gan nhat; vang = duong trong thanh City.map (RunningToPos); tim = tu di qua map (PathTravel);
// xanh la = danh sach Theo toa do (TD1 = diem dau).

#define CLR_W_SIDE		0xFF707070
#define CLR_W_ROUTE		0xFF00C0FF
#define CLR_W_DONE		0xFF305080
#define CLR_W_AIM		0xFFFFFFFF
#define CLR_W_MOVE		0xFFFF2020
#define CLR_W_CITY		0xFFFFFF00
#define CLR_W_TRAVEL	0xFFFF40FF
#define CLR_W_NODES		0xFF40FF40

#define W_MAX_LINES		(sizeof(s_Lines) / sizeof(s_Lines[0]) - 8)

static void WLine(int &n, int x1, int y1, int x2, int y2, DWORD dwColor)
{
	if (n < (int)W_MAX_LINES) AddLine(n, x1, y1, x2, y2, dwColor);
}

static void WDiamond(int &n, int x, int y, int r, DWORD dwColor)
{
	WLine(n, x - r, y, x, y - r * 2, dwColor);
	WLine(n, x, y - r * 2, x + r, y, dwColor);
	WLine(n, x + r, y, x, y + r * 2, dwColor);
	WLine(n, x, y + r * 2, x - r, y, dwColor);
}

static void WCross(int &n, int x, int y, int r, DWORD dwColor)
{
	WLine(n, x - r, y - r * 2, x + r, y + r * 2, dwColor);
	WLine(n, x + r, y - r * 2, x - r, y + r * 2, dwColor);
}

static void DrawWalk()
{
	if (!s_bShowWalk || !IsPlayerOk()) return;
	void *pRep = *(void**)RPS_BASE_ADD;
	if (!pRep) return;
	DWORD *pVT = *(DWORD**)pRep;
	PFN_DRAWPRIM pfnDraw = (PFN_DRAWPRIM)pVT[RPS_VT_DRAWPRIM / 4];
	PFN_OUTTEXT pfnText = (PFN_OUTTEXT)pVT[RPS_VT_OUTTEXT / 4];
	int nMap = *g_pMapID;
	int px, py;
	GetPos(&px, &py);
	int n = 0;
	char szText[64];

	// MapPath cua PathWalkTo
	BOOL bWalk = s_W.nMapID == nMap && s_W.nPoints > 0;
	if (bWalk)
	{
		for (int k = 0; k < s_W.nSides; k++)
		{
			int a = s_W.side[k][0], b = s_W.side[k][1];
			if (a < 0 || b < 0 || a >= s_W.nPoints || b >= s_W.nPoints) continue;
			if (!IsNearPlayer(s_W.pt[a], px, py) && !IsNearPlayer(s_W.pt[b], px, py)) continue;
			WLine(n, s_W.pt[a].x, s_W.pt[a].y, s_W.pt[b].x, s_W.pt[b].y, CLR_W_SIDE);
		}
		if (s_nWMap == nMap && s_nWLen > 0)
		{
			for (int i = 0; i + 1 < s_nWLen; i++)
			{
				POINT a = s_W.pt[s_nWRoute[i]], b = s_W.pt[s_nWRoute[i + 1]];
				WLine(n, a.x, a.y, b.x, b.y, (i + 1 <= s_nWIdx) ? CLR_W_DONE : CLR_W_ROUTE);
			}
			for (int i = 0; i < s_nWLen; i++)
			{
				POINT a = s_W.pt[s_nWRoute[i]];
				WDiamond(n, a.x, a.y, i == s_nWIdx ? 32 : 18, i == s_nWIdx ? CLR_W_AIM : (i < s_nWIdx ? CLR_W_DONE : CLR_W_ROUTE));
			}
			if (s_nWIdx < s_nWLen)
			{
				POINT a = s_W.pt[s_nWRoute[s_nWIdx]];
				WLine(n, px, py, a.x, a.y, CLR_W_AIM);
			}
			else	// het duong MapPath: doan chay thang toi diem dich
				WLine(n, px, py, s_ptWTarget.x, s_ptWTarget.y, CLR_W_AIM);
		}
	}

	// Duong trong thanh (CVLMap::RunningToPos)
	if (theMap.m_nPoints > 0 && theMap.m_nCurPoint >= 0 && theMap.m_nCurPoint < MAX_POINTS && !P_PLAYER->m_FightMode)
	{
		POINT a = theMap.MapPoints[theMap.m_nCurPoint];
		WLine(n, px, py, a.x, a.y, CLR_W_CITY);
		WDiamond(n, a.x, a.y, 24, CLR_W_CITY);
	}

	// Tu di qua map
	if (s_nTvTarget && s_nTvMap == nMap)
	{
		for (int i = 0; i + 1 < s_nTvLen; i++)
		{
			POINT a = s_T.pt[s_nTvRoute[i]], b = s_T.pt[s_nTvRoute[i + 1]];
			WLine(n, a.x, a.y, b.x, b.y, CLR_W_TRAVEL);
		}
		WCross(n, s_ptTvGoal.x, s_ptTvGoal.y, 40, CLR_W_TRAVEL);
	}

	// Danh sach Theo toa do
	if (theMove.m_bRouteMove && (!theMove.m_nNodesMapID || theMove.m_nNodesMapID == nMap))
	{
		for (int i = 0; i < MAX_NODES && theMove.m_Nodes[i].x; i++)
		{
			POINT a = theMove.m_Nodes[i];
			WDiamond(n, a.x, a.y, i == 0 ? 36 : 22, CLR_W_NODES);
			if (i + 1 < MAX_NODES && theMove.m_Nodes[i + 1].x)
				WLine(n, a.x, a.y, theMove.m_Nodes[i + 1].x, theMove.m_Nodes[i + 1].y, CLR_W_NODES);
		}
	}

	// Lenh MoveTo gan nhat (10s)
	BOOL bMove = s_dwLastMove && GetTickCount() - s_dwLastMove < 10000;
	if (bMove)
	{
		WLine(n, px, py, s_ptLastMove.x, s_ptLastMove.y, CLR_W_MOVE);
		WCross(n, s_ptLastMove.x, s_ptLastMove.y, 28, CLR_W_MOVE);
	}

	if (n > 0)
		pfnDraw(pRep, n, s_Lines, 1, 0);

	// Chu
	if (bWalk && s_nWMap == nMap && s_nWLen > 0)
		for (int i = 0; i < s_nWLen; i++)
		{
			POINT a = s_W.pt[s_nWRoute[i]];
			if (!IsNearPlayer(a, px, py)) continue;
			sprintf(szText, "%d", s_nWRoute[i]);
			pfnText(pRep, 12, szText, -1, a.x + 24, a.y - 16, i == s_nWIdx ? CLR_W_AIM : CLR_W_ROUTE, 0, 0, 0xFF000000);
		}
	if (theMove.m_bRouteMove && (!theMove.m_nNodesMapID || theMove.m_nNodesMapID == nMap))
		for (int i = 0; i < MAX_NODES && theMove.m_Nodes[i].x; i++)
		{
			POINT a = theMove.m_Nodes[i];
			if (!IsNearPlayer(a, px, py)) continue;
			sprintf(szText, "TD%d", i + 1);
			pfnText(pRep, 12, szText, -1, a.x + 28, a.y + 16, CLR_W_NODES, 0, 0, 0xFF000000);
		}
	if (bMove)
	{
		sprintf(szText, "MoveTo %s", s_szLastMove);
		pfnText(pRep, 12, szText, -1, s_ptLastMove.x + 30, s_ptLastMove.y + 30, CLR_W_MOVE, 0, 0, 0xFF000000);
	}
	if (bWalk && s_nWMap == nMap && s_nWLen > 0)
		sprintf(szText, "MapPath %d/%d", s_nWIdx, s_nWLen);
	else if (!bWalk && s_nWMap == nMap)
		sprintf(szText, "Khong co MapPath");
	else
		szText[0] = 0;
	if (szText[0])
		pfnText(pRep, 12, szText, -1, px + 40, py - 120, CLR_W_AIM, 0, 0, 0xFF000000);
}

// Bat/tat ve duong di (nOn < 0: dao). Tra ve trang thai moi, -1 = khong chen duoc ham ve
int PathShowWalk(int nOn)
{
	s_bShowWalk = (nOn < 0) ? !s_bShowWalk : (nOn != 0);
	if (s_bShowWalk && !PatchDraw(TRUE))
	{
		s_bShowWalk = FALSE;
		return -1;
	}
	return s_bShowWalk;
}
