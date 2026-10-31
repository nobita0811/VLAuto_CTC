#pragma once

// Ne vat can khi di chuyen (VLObstacle.cpp): luoi vat can lay tu KScenePlaceRegionC::m_ObstacleInfo
// cua cac region dang nap (9 region quanh nhan vat). Ngoai vung da nap coi nhu di duoc.

extern BOOL	g_bObsAvoid;								// TRUE = KPlayer::MoveTo tu di vong vat can

int			ObsGet(int x, int y);						// Obstacle_Kind tai (x,y): 0 di duoc, > 0 vat can, -1 region chua nap
template <class FN> void ObsForEachCell(FN fn);			// fn(x trai, y tren, info) cho moi o 32x32 co vat can (dinh nghia ben duoi)

// Duong thang (x1,y1) -> (x2,y2) co cat vat can khong (chi xet trong vung da nap)
BOOL		ObsLineClear(int x1, int y1, int x2, int y2);
// Neu duong thang toi (*pX,*pY) bi chan: thay bang diem trung gian xa nhat nhin thay duoc tren duong tim duoc (A*).
// Dich khong toi duoc => diem toi duoc gan dich nhat. TRUE = da doi dich
BOOL		ObsSteer(int px, int py, int *pX, int *pY);
// Quang duong di bo (vong vat can) tu (px,py) toi (tx,ty); thang duoc / tat ne vat can = khoang cach thang.
// -1 = khong toi duoc
int			ObsWalkDist(int px, int py, int tx, int ty);
// Loang tu (px,py) roi hoi quang duong toi tung diem (chon diem gan nhat THAT SU di toi duoc).
// ObsReachDist: MAX_INTNUM = khong toi duoc; ngoai vung da nap / tat ne vat can = khoang cach thang
void		ObsFlood(int px, int py);
int			ObsReachDist(int x, int y);
// Duong vua tim (de ve, ca khi chay thang duoc; het han sau 3s khong MoveTo): so diem, diem trung gian dang di toi
int			ObsLastPath(POINT *pPts, int nMax, POINT *pWaypoint);
void		ObsFree();

// Cac region dang nap: KScenePlaceC::m_pInProcessAreaRegions[] (Status 3 = REGION_S_STANDBY)
template <class FN> void ObsForEachRegion(FN fn)
{
	BYTE *pScene = (BYTE*)SCP_BASE_ADD;
	for (int r = 0; r < SCP_NUM_REGIONS; r++)
	{
		BYTE *pReg = *(BYTE**)(pScene + SCP_REGIONS_OFF + r * 4);
		if (!pReg || *(int*)(pReg + SPR_STATUS_OFF) != 3) continue;
		fn(*(int*)(pReg + SPR_LEFTTOP_OFF), *(int*)(pReg + SPR_LEFTTOP_OFF + 4), (const long*)(pReg + SPR_OBSTACLE_OFF));
	}
}

// m_ObstacleInfo[16][32]: bit 0-3 = Obstacle_Kind (0 khong, 1 thuong, 2 bay qua, 3 nhay qua, 4 nhay+bay), bit 4-7 = Obstacle_Type
template <class FN> void ObsForEachCell(FN fn)
{
	ObsForEachRegion([&](int nLeft, int nTop, const long *pObs)
	{
		for (int i = 0; i < 16; i++)
			for (int j = 0; j < 32; j++)
				if (pObs[i * 32 + j] & 0xF)	// Nhu GetObstacleInfo: chi xet loai (hinh Empty coi nhu ca o)
					fn(nLeft + i * 32, nTop + j * 32, pObs[i * 32 + j]);
	});
}
