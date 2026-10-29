#include "stdafx.h"
#include "../GameDef.h"
#include "DTextFile.h"
#include "GameBase.h"
#include "JxCore.h"
#include "S3Client.h"
#include "VLClasses.h"
#include "VLDebug.h"

#define MAX_TIME_WAIT 12

static int DriverTargetMap();
static BOOL RunToNearestNode();
static BOOL s_bNodeDirect = FALSE;	// Da het duong trong thanh (reset o InitValues)
static BOOL SameMapNodes();
static BOOL s_bInVisited = FALSE;		// Da chon "Nhung noi da di qua" o xa phu, menu dang mo la danh sach noi da di qua
static DWORD s_dwReturnTick = 0;		// Vua chon "Tro lai dia diem cu": cho xem co ra ngoai thanh khong
static int s_nReturnMap = 0;
static BOOL s_bTriedReturn = FALSE;		// Map train = map thanh: da toi xa phu thu "Tro lai dia diem cu" (reset o InitValues)
static DWORD s_dwTryReturnTick = 0;
static int s_nRebuyCount = 0;			// So lan quay lai mua TDP/thuoc truoc khi bam xa phu (reset o InitValues)
static BOOL NeedBuyMedicine();
static BOOL NeedBuyAmulet();

//================================Class CVLMap================================
CVLMap::CVLMap(void)
{
	memset(m_ShopItem, 0, sizeof(m_ShopItem));
	for (int i = 0; i < MAX_SHOP_ITEMS; i++)
		m_ShopItem[i].nShopIdx = -1;

	strcpy(m_ShopItem[0].szItemName, TV(L"Kim Sáng Dược (tiểu)"));
	strcpy(m_ShopItem[1].szItemName, TV(L"Kim Sáng Dược (trung)"));
	strcpy(m_ShopItem[2].szItemName, TV(L"Kim Sáng Dược (đại)"));
	strcpy(m_ShopItem[3].szItemName, TV(L"Hồi Thiên đan"));
	strcpy(m_ShopItem[4].szItemName, TV(L"Ngưng Thần đan (tiểu)"));
	m_ShopItem[4].nItemType = medicine_mana;
	strcpy(m_ShopItem[5].szItemName, TV(L"Ngưng Thần đan (trung)"));
	m_ShopItem[5].nItemType = medicine_mana;
	strcpy(m_ShopItem[6].szItemName, TV(L"Ngưng Thần đan (đại)"));
	m_ShopItem[6].nItemType = medicine_mana;
	strcpy(m_ShopItem[7].szItemName, TV(L"Đại Bổ tán"));
	m_ShopItem[7].nItemType = medicine_mana;
	strcpy(m_ShopItem[8].szItemName, TV(L"Thừa Tiên Mật (tiểu)"));
	m_ShopItem[8].nItemType = medicine_both;
	strcpy(m_ShopItem[9].szItemName, TV(L"Thừa Tiên Mật (trung)"));
	m_ShopItem[9].nItemType = medicine_both;
	strcpy(m_ShopItem[10].szItemName, TV(L"Tam Hoa Tán"));
	m_ShopItem[10].nItemType = medicine_stamina;
	strcpy(m_ShopItem[11].szItemName, TV(L"Hoắc Hương Chính khí hoàn"));
	m_ShopItem[11].nItemType = medicine_antipoison;
	strcpy(m_ShopItem[12].szItemName, TV(L"Tam Thuận tán"));
	m_ShopItem[12].nItemType = medicine_antipoison;
	strcpy(m_ShopItem[13].szItemName, TV(L"Ngân Kiều Khứ độc đan"));
	m_ShopItem[13].nItemType = medicine_antipoison;
	strcpy(m_ShopItem[14].szItemName, TV(L"Cửu Chuyển Hoàn Hồn đan"));
	strcpy(m_ShopItem[15].szItemName, TV(L"Ô đầu Hoàn thần đan"));
	m_ShopItem[15].nItemType = medicine_mana;
	strcpy(m_ShopItem[16].szItemName, TV(L"Ngũ Hoa Ngọc Lộ Hoàn"));
	m_ShopItem[16].nItemType = medicine_both;
	strcpy(m_ShopItem[17].szItemName, TV(L"Long Đàm Hóa Độc đan"));
	m_ShopItem[17].nItemType = medicine_antipoison;

	m_nLastMapID = 0;
	memset(m_szLastMapName, 0, 32);
	m_nSaveMapID = 0;
	m_nPoints = 0;
	m_nCurPoint = -1;
	m_bRoomLocked = 0;
	m_bNoDriverRecent = 0;
	m_bFirstSaveRepos = TRUE;
	m_nLastdZ = MAX_INTNUM;
	m_nLagCount = 0;
	m_nBigLagCount = 0;
	m_bEnableGiveKCL = TRUE;
}

void CVLMap::InitEquipCheck()
{
	// Check Equipments
	m_bNeedRepairItems = 0;

	if (m_bRepair)
	{
		for (int i = 0; i < MAX_EQUIP_SLOT; i++)
		{
			int nItemIdx = g_pCurPlayer->m_ItemList.GetEquip(i);
			if (!nItemIdx) continue;

			KItem* pItem = g_pItemArray[nItemIdx];

			if (pItem->m_nCurDur < 0) continue; // Durability not available
			if (pItem->m_nItemPlus > 5) continue; // Cannot repair this item
			if (pItem->m_nCurDur < pItem->m_nMaxDur)
			{
				m_bNeedRepairItems = TRUE;
				break;
			}
		}
	}

	// Check Amulet
	m_bNeedGroceries = NeedBuyAmulet();

	// Check Medicines
	m_bNeedPharmacy = NeedBuyMedicine();
}

// Con thieu Tho Dia Phu (so luong trong hanh trang < so luong cai dat, it nhat 1)
static BOOL NeedBuyAmulet()
{
	if (!theMap.m_bBuyTDP || !theMap.NpcPoints[npc_Groceries].x) return FALSE;
	int nNeedTDP = theMap.m_nAMNumbers > 0 ? theMap.m_nAMNumbers : 1;
	return g_pCurPlayer->GetItemsCount(AMULET_PAPER) < nNeedTDP;
}

// So mon Tho Dia Phu con phai mua (moi mon 1 o)
static int AmuletNeedCount()
{
	if (!theMap.m_bBuyTDP) return 0;
	int nNeedTDP = theMap.m_nAMNumbers > 0 ? theMap.m_nAMNumbers : 1;
	int nHave = g_pCurPlayer->GetItemsCount(AMULET_PAPER);
	return nNeedTDP > nHave ? nNeedTDP - nHave : 0;
}

// So mon mau/mana/thuoc con phai mua (moi mon 1 o)
static int MedicineNeedCount()
{
	int nNeed = 0;
	for (int i = 0; i < 3; i++)
	{
		if (!theMap.m_TradeItem[i].bBuyCheck) continue;
		int nItemPos = theMap.m_TradeItem[i].nItemPos;
		int nItemCount = g_pCurPlayer->GetItemsCount(item_medicine, theMap.m_ShopItem[nItemPos].nItemType);
		if (theMap.m_TradeItem[i].nBuyAmout > nItemCount)
			nNeed += theMap.m_TradeItem[i].nBuyAmout - nItemCount;
	}
	return nNeed;
}

// Con thieu mau/mana/thuoc da chon mua (so luong trong hanh trang < so luong cai dat)
static BOOL NeedBuyMedicine()
{
	if (!theMap.NpcPoints[npc_Pharmacy].x) return FALSE;
	for (int i = 0; i < 3; i++)
	{
		if (!theMap.m_TradeItem[i].bBuyCheck) continue;
		int nItemPos = theMap.m_TradeItem[i].nItemPos;
		int nItemCount = g_pCurPlayer->GetItemsCount(item_medicine, theMap.m_ShopItem[nItemPos].nItemType);
		if (theMap.m_TradeItem[i].nBuyAmout > nItemCount)
			return TRUE;
	}
	return FALSE;
}

void CVLMap::InitValues()
{
	s_bNodeDirect = FALSE;
	s_bTriedReturn = FALSE;
	s_dwTryReturnTick = 0;
	s_bInVisited = FALSE;
	PathWalkReset();
	m_nTimeWait = 2;
	m_bFinished = 0;
	m_nDriverStep = 0;
	s_dwReturnTick = 0;
	s_nRebuyCount = 0;
	theMove.GoTrainStop();
	m_nCurPoint = -1;
	m_bDoShoping = 0;
	m_nEntrustStatus = 0;
	m_nBigLagCount = 0;
	theMove.m_bFindingWay = TRUE;
	m_bNeedMoney = m_bWithDraw && (m_bAutoReturn || theQuest.m_bAutoDoQuest);
	m_bNeedSellItems = m_bSellItems;
	m_bNeedUseLB = g_pCurPlayer->m_cTong.m_dwTongNameID && theQuest.m_bAutoDoQuest && (theQuest.m_nTongKill == act_auto);
	m_bNeedSaveItems = (m_nSaveItemMode > 0);

	InitEquipCheck();

	if (!m_bNeedGroceries && !m_bNeedPharmacy)
		m_bNeedPharmacy = m_bNeedRepairItems;

	if (P_PLAYER->m_nPKFlag)
		ExecuteScript("Switch('PK',0)");

	m_bNeedFuYuan = m_bGetReputation && NpcPoints[npc_Reputation].x;
	m_bNeedMingWang = m_bNeedFuYuan;
	m_bNeedGiveKCL = g_pCurPlayer->m_cTong.m_dwTongNameID && m_bGiveKCL && m_bEnableGiveKCL;
	m_bIsReady = 0;
	m_bNeedDriver = m_bGetDriver || theQuest.m_bNeedGoTrain;
	m_nSaveMapID = theMain.GetReturnCity(m_nTrainMapID);
	NearestDriver();
}

void CVLMap::Breathe()
{
	static BOOL bInFightMode = 0;
	static int nWaitTime = 0;
	if (g_NetworkOffline) return;

	if (P_PLAYER->m_FightMode)
	{
		m_bEnableGiveKCL = TRUE;
		m_nLastMapID = 0;
		if (!bInFightMode) //return from City
		{
			KUiMsgSel* pMsgSel = (KUiMsgSel*)GetPtrAddress(MSG_BASE_ADD);
			if (pMsgSel && pMsgSel->IsVisible()) //Xa phu
				pMsgSel->OnClickMsg(pMsgSel->m_nNumMessage);
			bInFightMode = TRUE;
		}
		return;
	}

	if (bInFightMode) //return from Front
	{
		theAttack.SetPeopleIdx(0);
		if (nWaitTime++ < 5) return; //wait for 3s
		if (PLAY_HAND_IDX)
		{
			POINT itmPos;
			KItem* pItem = (KItem*)(dwItmBaseAdd + PLAY_HAND_IDX * ITM_DATA_SIZE);
			if (EQUIP_ROOM.FindRoom(ComAttr.nWidth, ComAttr.nHeight, &itmPos))
			{
				g_pCurPlayer->SwitchItem(UOC_ITEM_TAKE_WITH, itmPos.x, itmPos.y);
				return;
			}
		}
		m_bNeedSellItems = m_bAutoReturn && m_bSellItems;
		if (m_bNeedSellItems)
		{
			if (SellItems()) return;
			m_bNeedSellItems = 0;
		}
		bInFightMode = 0;
		nWaitTime = 0;
		return;
	}

	if (theMain.m_bPause) return;
	if (PathTravelActive()) return;	// Dang tu di theo MapPath (xa phu khong co map train)

	if (((*g_pMapID) != m_nLastMapID) ||
		(strcmp(m_szLastMapName, g_szMapName)))
	{	//Map Changed
		m_nPoints = 0;
		LoadMap(*g_pMapID);
		m_nLastMapID = (*g_pMapID);
		strcpy(m_szLastMapName, g_szMapName);
		InitValues();
	}

	if (theMove.IsFlagMoving())
		return;

	if (m_nPoints <= 0) return;
	if (theUtil.m_bDoingUtil) return;

	if (m_bNeedMoney)
	{
		WithdrawMoney();
		return;
	}

	if (theQuest.m_bAutoDoQuest)
	{
		theQuest.Breath();
		if (!theQuest.m_bNeedGoTrain) return;
	}

	if (!m_bAutoReturn) return;

	if (g_pCurPlayer->m_nEntrustMode) return;

	if (m_bNeedGiveKCL)
	{
		int nKCLCount = g_pCurPlayer->GetItemsCount(KC_LENH);
		if (nKCLCount >= m_nMinKCL && nKCLCount > 0)
		{
			theUtil.m_bDoingUtil = TRUE;
			theUtil.m_nUtilTask = util_givekcl;
			SystemMessage(TV(L"<color=yellow>Đi giao nộp Khiêu chiến lệnh!!!"));
		}
		m_bNeedGiveKCL = 0;
		return;
	}

	if (theQuest.m_bGoTrainGetPK)
	{
		m_bNeedSaveItems = 0;
		if ((*g_pMapID) >= city_banghoimin && (*g_pMapID) <= city_banghoimax)
		{	// Goto Phuong_Tuong
			if (ChangeMap(city_phuongtuong)) return;
		}
		else
		{
			// Go to main city
			int nOwnMapID = theMain.GetOwnerCity(*g_pMapID);
			if (ChangeMap(nOwnMapID)) return;
		}
	}
	else
	{
		// Go to main city
		int nOwnMapID = theMain.GetOwnerCity(*g_pMapID);
		if (ChangeMap(nOwnMapID)) return;

		if (m_bFirstSaveRepos)
		{
			FirstRepository();
			return;
		}
	}
	if (m_bNeedSellItems)
	{
		if (SellItems()) return;
		m_bNeedSellItems = 0;
	}
#ifndef VLTK_FREE_MODE
	if (m_bNeedUseLB && !theQuest.m_bIsFull100LB && !theQuest.m_bGoTrainGetPK)
	{
		int nTrainMapID = m_nTrainMapID;
		if (theQuest.m_nYeSouMapID)
			nTrainMapID = theQuest.m_nYeSouMapID;
		if (!theQuest.AutoUseLB(nTrainMapID)) return;
		m_bNeedUseLB = 0;
	}
#endif
	if (m_bNeedSaveItems)
	{
		if (!m_bDoShoping && RunningToPos(NpcPoints[npc_Repository])) return;
		m_bDoShoping = TRUE;
		GetSaveItems();
		return;
	}
	if (m_bNeedGroceries)
	{
		if (!m_bDoShoping && RunningToPos(NpcPoints[npc_Groceries])) return;
		m_bDoShoping = TRUE;
		GetGroceries();
		return;
	}
	if (m_bNeedPharmacy)
	{
		if (!m_bDoShoping && RunningToPos(NpcPoints[npc_Pharmacy])) return;
		m_bDoShoping = TRUE;
		GetPharmacy();
		return;
	}
	if (m_bNeedMingWang)
	{
		if (!m_bDoShoping && RunningToPos(NpcPoints[npc_Reputation])) return;
		m_bDoShoping = TRUE;
		GetReputation(1);
		return;
	}
#ifndef VLTK_FREE_MODE
	if (m_bNeedFuYuan)
	{
		GetReputation(0);
		return;
	}
#endif
	if (!m_bIsReady)
	{
		m_bIsReady = CheckReady();
		if (!m_bIsReady) return;
		if (m_bRoomLocked) theMain.CloseLockRoom();
		NearestDriver();
	}

	if (m_bNeedDriver)
	{
		if (m_nDriverOption == 5 && !theQuest.m_nYeSouMapID)
		{
			UseThanHanhPhu();
			return;
		}

		// Map can toi chinh la map nay (vd danh sach toa do o ngoai thanh Tuong Duong):
		// toi xa phu thu "Tro lai dia diem cu" truoc (GetDriver), khong duoc (hoac biet chac khong co) moi tu chay ra
		if (SameMapNodes() && (s_bTriedReturn || m_bNoDriverRecent))
		{
			if (s_dwTryReturnTick && GetTickCount() - s_dwTryReturnTick < 4000)
				return;	// Cho xem co duoc dua ve dia diem cu khong (duoc thi m_FightMode = 1, Breathe dung o tren)
			if (theMain.GetMenuCount() > 0)
			{
				theMain.ClickMenuIdx(theMain.GetMenuCount());	// Dong hoi thoai xa phu
				return;
			}
			if (RunToNearestNode())
			{
				theMove.m_bFindingWay = TRUE;
				return;
			}
		}

		if (!m_bDoShoping && RunningToPos(NpcPoints[m_nDriverIndex])) return;
		if (!m_bDoShoping) CloseAllWindows(0);	// Toi xa phu: tat het cua so tranh bi de
		m_bDoShoping = TRUE;
		GetDriver();
	}
}

void CVLMap::CheckAutoReturn()
{
	char szMsg[80];
	if (m_bAutoReturn)
		sprintf(szMsg, TV(L"<color=yellow>Về thành tự quay lại (Ctrl+T): <color=green>Bật"));
	else
	{
		sprintf(szMsg, TV(L"<color=yellow>Về thành tự quay lại (Ctrl+T): <color>Tắt"));
		m_nCurPoint = -1;
		m_bDoShoping = 0;
		PathTravelStop("tat Ve thanh tu quay lai"); // Tắt về thành quay lại
	}
	SystemMessage(szMsg, 1);
}

BOOL CVLMap::LoadMap(int nMapID)
{
	char szMapFile[MAX_PATH];
	sprintf(szMapFile, "%s\\Maps\\CityMaps\\City%d.map", theMain.m_szCurFolder, nMapID);
	FILE* pFile = fopen(szMapFile, "r");
	if (pFile == NULL) return 0;
	int nRet;
	//Read NPC Points
	int i = 0;
	while (TRUE)
	{
		nRet = fscanf(pFile, "%d,%d\n", &NpcPoints[i].x, &NpcPoints[i].y);
		if (nRet < 2) break;
		i++;
	}
	//Read Points
	i = 0;
	while (TRUE)
	{
		nRet = fscanf(pFile, "%d,%d\n", &MapPoints[i].x, &MapPoints[i].y);
		if (nRet < 2) break;
		i++;
	}
	m_nPoints = i;
	for (int i = 0; i < m_nPoints; i++)
		for (int j = 0; j < m_nPoints; j++)
			if (i == j) MapSides[i][j] = 0;
			else MapSides[i][j] = MAX_INTNUM;
	//Read Sides
	while (TRUE)
	{
		int si, sj;
		nRet = fscanf(pFile, "%d,%d\n", &si, &sj);
		if (nRet < 2) break;
		MapSides[si][sj] = MapSides[sj][si] = __distance(MapPoints[si], MapPoints[sj]);
	}
	fclose(pFile);
	return TRUE;
}

BOOL CVLMap::Dijkstra(int nStart, int nEnd)
{
	int Dist[MAX_POINTS];
	BOOL Free[MAX_POINTS];
	BOOL bStop = 0;
	int i, u, v, min;
	if (nEnd < 0 || nStart < 0) return 0;
	for (i = 0; i < m_nPoints; i++)
	{
		Dist[i] = MapSides[nStart][i];
		TraceRoute[i] = nStart;
	}
	memset(Free, TRUE, sizeof(Free));
	do {
		u = -1; min = MAX_INTNUM;
		for (i = 0; i < m_nPoints; i++)
			if (Free[i] && Dist[i] < min)
			{
				min = Dist[i];
				u = i;
			}
		if (u == -1 || u == nEnd) bStop = TRUE;
		if (!bStop)
		{
			Free[u] = FALSE;
			for (v = 0; v < m_nPoints; v++)
				if (Free[v] && Dist[v] > (Dist[u] + MapSides[u][v]))
				{
					Dist[v] = Dist[u] + MapSides[u][v];
					TraceRoute[v] = u;
				}
		}

	} while (!bStop);

	return (Dist[nEnd] < MAX_INTNUM);
}

BOOL CVLMap::RunningToPos(POINT ptDest)
{
	int nPlayerX, nPlayerY;
	int dZ, dMin;
	static int nStart = -1;
	static int nEnd = -1;

	if (m_nPoints <= 0) return 0;

	P_PLAYER->GetMapPos(&nPlayerX, &nPlayerY);
	POINT ptPlayer = { nPlayerX, nPlayerY };

	if (m_nCurPoint < 0)
	{
		//Finding Start Point (near Destination)
		dMin = MAX_INTNUM;
		for (int i = 0; i < m_nPoints; i++)
		{
			dZ = __distance(MapPoints[i], ptDest);
			if (dZ < dMin)
			{
				dMin = dZ;
				nStart = i;
			}
		}
		//Finding End Point (near Player)
		dMin = MAX_INTNUM;
		for (int j = 0; j < m_nPoints; j++)
		{
			dZ = __distance(MapPoints[j], ptPlayer);
			if (dZ < dMin)
			{
				dMin = dZ;
				nEnd = j;
			}
		}
		if (!Dijkstra(nStart, nEnd)) return 0;
		m_nCurPoint = nEnd;
	}

	//Tracing the Path
	if (m_nCurPoint >= 0)
	{
		dZ = __distance(MapPoints[m_nCurPoint], ptPlayer);
		if (dZ <= 100)
		{
			m_nLagCount = 0;
			if (m_nCurPoint != nStart)
			{
				m_nCurPoint = TraceRoute[m_nCurPoint];
				if (!P_PLAYER->m_bRideHorse && HAVE_HORSE)
					ExecuteScript(SCK_SHORTCUT_HORSE); // Len ngua
			}
			else
			{
				m_nCurPoint = -1;
				return 0;
			}
		}
		if (IsLagMoving(nPlayerX, nPlayerY, dZ)) return TRUE;
		PathNoteMove(MapPoints[m_nCurPoint].x, MapPoints[m_nCurPoint].y, "City.map");
		g_pCurPlayer->MoveTo(MapPoints[m_nCurPoint].x, MapPoints[m_nCurPoint].y);
		return TRUE;
	}

	return 0;
}

BOOL CVLMap::IsLagMoving(int nPlayerX, int nPlayerY, int dZ)
{
	if (dZ > m_nLastdZ - 96)
		m_nLagCount++;
	m_nLastdZ = dZ;
	if (m_nLagCount > 7)
	{
		m_nBigLagCount++;
		if (m_nBigLagCount > 3)
		{
			g_pCurPlayer->EnterTongMap();
			m_nBigLagCount = 0;
			return TRUE;
		}
		unsigned int number;
		rand_s(&number);
		int nDestX = nPlayerX + (int)(((double)number / (double)UINT_MAX * 512) - 256);
		rand_s(&number);
		int nDestY = nPlayerY + (int)(((double)number / (double)UINT_MAX * 1024) - 512);
		m_nLagCount = 0;
		m_nCurPoint = -1;
		SystemMessage(TV(L"<color=yellow>Kẹt đường rồi, chạy hướng khác xem sao."));
		g_pCurPlayer->MoveTo(nDestX, nDestY);
		return TRUE;
	}
	return 0;
}

void CVLMap::NearestDriver()
{
	// Find the nearest driver
	int nPlayerX, nPlayerY, dZ;
	int dMin = MAX_INTNUM;
	if (m_nPoints <= 0) return;
	P_PLAYER->GetMapPos(&nPlayerX, &nPlayerY);
	for (int i = npc_Driver1; i <= npc_Driver4; i++)
	{
		dZ = __distance(nPlayerX, nPlayerY, NpcPoints[i].x, NpcPoints[i].y);
		if (dZ < dMin)
		{
			dMin = dZ;
			m_nDriverIndex = i;
		}
	}
}

void CVLMap::WithdrawMoney()
{
	if (theMain.OpenLockRoom(m_nRepositoryPass))
	{
		m_bRoomLocked = TRUE;
		return;
	}
	int nWithdrawAmount = m_nGetMoneyAmount * 10000 - EQUIP_MONEY;
	if (nWithdrawAmount > ROOM_MONEY)
	{
		SystemMessage(TV(L"<color=yellow>Tiền trong rương không đủ!!!"));
		nWithdrawAmount = ROOM_MONEY;
	}
	if (nWithdrawAmount > 0)
		g_pCoreShell->OperationRequest(GOI_MONEY_INOUT_STORE_BOX, 0, nWithdrawAmount);
	m_bNeedMoney = 0;
}
void CVLMap::WithdrawMoneyForXaPhu()
{
	if (theMain.OpenLockRoom(m_nRepositoryPass))
	{
		m_bRoomLocked = TRUE;
		return;
	}
	int nWithdrawAmount = 1000 - EQUIP_MONEY;
	if (nWithdrawAmount > ROOM_MONEY)
	{
		SystemMessage(TV(L"<color=yellow>Tiền trong rương không đủ!!!"));
		nWithdrawAmount = ROOM_MONEY;
	}
	if (nWithdrawAmount > 0)
		g_pCoreShell->OperationRequest(GOI_MONEY_INOUT_STORE_BOX, 0, nWithdrawAmount);
	m_bNeedMoney = 0;
}
BOOL CVLMap::ChangeMap(int nNewMapID)
{
	char szCityName[64] = "";
	if ((*g_pMapID) == nNewMapID) return 0;

	// Uu tien vat pham ([Item] trong Maps\Path) neu co trong hanh trang; dang tu di thi cho (cac noi goi tra ve)
	if (PathTravelActive()) return TRUE;
	if (PathTravelItemStart(nNewMapID))
	{
		QTRACE("ChangeMap: %d -> %d dung vat pham (MapPath [Item])", *g_pMapID, nNewMapID);
		return TRUE;
	}

	int nOwnerID = theMain.GetOwnerCity(*g_pMapID);
	if (nOwnerID == (*g_pMapID))
		theMain.GetCityName(nNewMapID, szCityName);
	else
		theMain.GetCityName(nOwnerID, szCityName);

	QTRACE("ChangeMap: %d -> %d '%s' Shoping=%d Driver=%d (%d,%d) CurPoint=%d", *g_pMapID, nNewMapID, szCityName, m_bDoShoping, m_nDriverIndex, NpcPoints[m_nDriverIndex].x, NpcPoints[m_nDriverIndex].y, m_nCurPoint);
	if (!m_bDoShoping && RunningToPos(NpcPoints[m_nDriverIndex])) return TRUE;
	m_bDoShoping = TRUE;
	//rút tiền

	int nCount = theMain.GetMenuCount();
	QTRACE("ChangeMap: da toi xa phu, MenuCount=%d TimeWait=%d", nCount, m_nTimeWait);

	if (nCount > 0)
	{
		WithdrawMoneyForXaPhu();
		if (ROOM_MONEY < 500)
		{
			PostChatMessage2(TV(L"<color=green>VLCTCPro<color> :<color=yellow>Tiền trong rương không đủ, không thể rỳt tiền!!!"));
			// tắt tất cả cửa sổ
			CloseAllWindows(0);
			return FALSE;
		}
		// Menu co san ten thanh (vd "D­¬ng Ch©u phÝ") -> chon luon
		if (szCityName[0] && theMain.ClickMenuStr(szCityName))
		{
			m_nTimeWait = 5;
			return TRUE;
		}
		if (theMain.ClickMenuStr(TV(L"Những thành thị đã đi qua")))
			m_nTimeWait = 2;
		else if (theMain.ClickMenuStr(TV(L"lượng"), 0)) // City Selection
		{
			theMain.ClickMenuStr(szCityName);
			m_nTimeWait = 5;
		}
		else
			QTRACE("ChangeMap: menu %d dong khong co '%s' / 'Nhung thanh thi da di qua' / 'luong'", nCount, szCityName);
		return TRUE;
	}
	//cất tiền
	QTRACE("ChangeMap: chua co menu -> ClickToNpc xa phu");
	ClickToNpc(NpcPoints[m_nDriverIndex].x, NpcPoints[m_nDriverIndex].y);

	return TRUE;
}

int CVLMap::OpenStoreBox()
{
	KWndImage* pKUiStoreBox = (KWndImage*)(*(LPDWORD)SBO_BASE_ADD);
	if (pKUiStoreBox && pKUiStoreBox->IsVisible())
	{
		if (m_nTimeWait > 5) m_nTimeWait = 2;
		return 1;
	}

	BOOL bFound = FALSE;
	int nPlayerX, nPlayerY, dZ;
	int nObjX, nObjY, nNpcX, nNpcY;

	if (m_nTimeWait)
	{
		if (m_nTimeWait == 5)
		{
			int nX = (P_PLAYER->m_SyncSignal & 0x07) << 3;
			char szScript[32];
			sprintf(szScript, "MoveTo(%d, 0)", nX);
			ExecuteScript(szScript);
			SystemMessage(TV(L"<color=yellow>Lag quá, chờ 5s click lại..."));
		}
		m_nTimeWait--;
		return 0;
	}

	P_PLAYER->GetMapPos(&nPlayerX, &nPlayerY);
	if ((*g_pMapID) >= city_visondao)
	{
		for (int i = 1; i < MAX_NPC; i++)
		{
			if (!g_pNpcArray[i]->IsExist() || g_pNpcArray[i]->m_Kind != kind_dialoger) continue;
			if (strcmp(g_pNpcArray[i]->Name, TV(L"Rương chứa đồ"))) continue;
			g_pNpcArray[i]->GetMapPos(&nNpcX, &nNpcY);
			dZ = __distance(nPlayerX, nPlayerY, nNpcX, nNpcY);
			if (dZ > MAX_MOVE_DISTANCE)
				g_pCurPlayer->MoveTo(nNpcX, nNpcY);
			else
			{
				theAttack.SetPeopleIdx(i);
				m_nTimeWait = MAX_TIME_WAIT;
			}
			bFound = TRUE;
			break;
		}
	}
	else
	{
		for (int i = 1; i < MAX_OBJ; i++)
		{
			if (g_pObjArray[i]->m_nID == -1) continue;
			if (g_pObjArray[i]->m_Kind != Obj_Kind_StoreBox) continue;
			g_pObjArray[i]->GetMapPos(&nObjX, &nObjY);
			dZ = __distance(nPlayerX, nPlayerY, nObjX, nObjY);
			if (dZ > PLAYER_PICKUP_DISTANCE)
				g_pCurPlayer->MoveTo(nObjX, nObjY);
			else
			{
				P_PLAYER->m_nObjectIdx = i;
				m_nTimeWait = MAX_TIME_WAIT;
			}
			bFound = TRUE;
			break;
		}
	}
	if (!bFound)
		return -1;
	return 0;
}

void CVLMap::SaveNewRepository()
{
	if (P_PLAYER->m_FightMode)
		return;
	if ((*g_pMapID) == city_visondao)
	{
		SystemMessage(TV(L"<color=yellow>Không lưu Rương trên Vi Sơn Đảo!!!"));
		return;
	}
	if (m_nPoints <= 0)
	{
		SystemMessage(TV(L"<color=yellow>Lỗi đọc file MAP thành thị!!!"));
		return;
	}
	int nPlayerX, nPlayerY, nObjX, nObjY, dZ;
	int nRepX = 0, nRepY = 0;
	P_PLAYER->GetMapPos(&nPlayerX, &nPlayerY);
	for (int i = 1; i < MAX_OBJ; i++)
	{
		if (g_pObjArray[i]->m_nID == -1) continue;
		if (g_pObjArray[i]->m_Kind != Obj_Kind_StoreBox) continue;
		g_pObjArray[i]->GetMapPos(&nObjX, &nObjY);
		dZ = __distance(nPlayerX, nPlayerY, nObjX, nObjY);
		if (dZ > PLAYER_PICKUP_DISTANCE)
			g_pCurPlayer->MoveTo(nObjX, nObjY);
		else
			P_PLAYER->m_nObjectIdx = i;
		nRepX = nObjX;
		nRepY = nObjY;
		break;
	}
	if (nRepX && nRepY)
	{
		char szMapFile[MAX_PATH];
		sprintf(szMapFile, "%s\\Maps\\CityMaps\\City%d.map", theMain.m_szCurFolder, (*g_pMapID));
		FILE* pFile = fopen(szMapFile, "w");
		if (pFile == NULL)
		{
			SystemMessage(TV(L"<color=yellow>Lỗi ghi file MAP, xin kiểm tra lại!!!"));
			return;
		}
		//Write NPC Points
		NpcPoints[npc_Repository].x = nRepX;
		NpcPoints[npc_Repository].y = nRepY;
		for (int i = 0; i < npc_Nums; i++)
			fprintf(pFile, "%d,%d\n", NpcPoints[i].x, NpcPoints[i].y);
		fprintf(pFile, "0\n");
		//Write Points
		for (int i = 0; i < m_nPoints; i++)
			fprintf(pFile, "%d,%d\n", MapPoints[i].x, MapPoints[i].y);
		fprintf(pFile, "0\n");
		//Write Sides
		for (int i = 0; i < m_nPoints; i++)
			for (int j = i + 1; j < m_nPoints; j++)
				if (MapSides[i][j] < MAX_INTNUM)
					fprintf(pFile, "%d,%d\n", i, j);
		SystemMessage(TV(L"<color=yellow>Đã lưu thành công tọa độ Rương đồ mới!"));
		m_nCurPoint = -1;
		m_bDoShoping = 0;
		fclose(pFile);
		return;
	}
	SystemMessage(TV(L"<color=yellow>Không tìm thấy Rương đồ!!!"));
}

void CVLMap::FirstRepository()
{
	int nSaveMap = m_nSaveMapID;
	if (theQuest.m_nYeSouMapID)
		nSaveMap = theQuest.m_nSaveMapID;
	if (!nSaveMap)
	{
		m_bFirstSaveRepos = 0;
		return;
	}
	if (ChangeMap(nSaveMap)) return;
	if (!m_bDoShoping && RunningToPos(NpcPoints[npc_Repository])) return;
	m_bDoShoping = TRUE;

	if (OpenStoreBox() <= 0)
		return;

	SystemMessage(TV(L"<color=yellow>Đã nhớ rương về thành!!!"));
	ResetShoping();
	m_bFirstSaveRepos = 0;
}

void CVLMap::GetSaveItems()
{
	if (theMain.OpenLockRoom(m_nRepositoryPass))
	{
		m_bRoomLocked = TRUE;
		return;
	}
	//Running to Repository
	int nOpeningStore = OpenStoreBox();
	if (nOpeningStore <= 0)
	{
		if (nOpeningStore < 0)
		{
			SystemMessage(TV(L"<color=yellow>Không thấy Rương đồ, lần sau gửi vậy."));
			m_bNeedSaveItems = 0;
			m_bDoShoping = 0;
		}
		return;
	}

	if (SaveItems()) return;

	SystemMessage(TV(L"<color=yellow>Cất đồ vào rương hoàn tất!!!"));
	ResetShoping();
	m_bNeedSaveItems = 0;
}

void CVLMap::GetRepairItems()
{
	int nItemIdx;
	KItem* pItem;
	static int nLastRpEquip = -1;
	for (int i = nLastRpEquip + 1; i < MAX_EQUIP_SLOT; i++)
	{
		nItemIdx = g_pCurPlayer->m_ItemList.GetEquip(i);
		if (!nItemIdx) continue;
		pItem = (KItem*)(dwItmBaseAdd + nItemIdx * ITM_DATA_SIZE);
		if (pItem->m_nCurDur < 0) continue; // Durability not available
		if (pItem->m_nItemPlus > 5) continue; // Cannot repair this item
		if (pItem->m_nCurDur < pItem->m_nMaxDur)
		{	//Sua do
			SystemMessage(0, TV(L"Sửa đồ: <color=yellow>%s"), ComAttr.szItemName);
			g_pCurPlayer->RepairItem(nItemIdx);
			nLastRpEquip = i;
			return;
		}
	}
	nLastRpEquip = -1;
	m_bNeedRepairItems = 0;
}

void CVLMap::GetGroceries()
{
	KUiMsgSel2* pMsgSel = (KUiMsgSel2*)GetPtrAddress(MSG2_BASE_ADD);
	KUiShop* pShop = (KUiShop*)GetPtrAddress(SHOP_BASE_ADD);

	if (g_UiInformation2->IsVisible())
	{
		g_UiInformation2->Hide();
		return;
	}
	if (pMsgSel && pMsgSel->IsVisible())
	{
		if (pShop)
		{
			if (strstr(pMsgSel->m_pMessages[1]->Msg, FULL_REPAIR))
			{
				pMsgSel->OnClickMsg(1); // Full Repair
				SystemMessage(TV(L"<color=yellow>Sửa kỹ đồ Hoàng Kim"));
			}
			else
				pMsgSel->OnClickMsg(pMsgSel->m_nNumMessage);
			return;
		}
		for (int i = 0; i < pMsgSel->m_nNumMessage; i++)
		{
			char* szMsg = (char*)pMsgSel->m_pMessages[i]->Msg;
			if (strstr(szMsg, AGREE_TRADE))
			{
				pMsgSel->OnClickMsg(i);
				break;
			}
		}
		return;
	}
	if (m_bFinished)
	{
		ResetShoping();
		m_bFinished = 0;
		m_bNeedGroceries = 0;
		return;
	}
	if (pShop) //Do Shoping
	{
		//Repair Items
		if (m_bNeedRepairItems)
		{
			GetRepairItems();
			return;
		}
		//Thieu cho trong: ban do (SellItems.txt / Nhat do khong giu) truoc khi mua
		if (MakeShopRoom(AmuletNeedCount())) return;
		//Full Room
		if (g_pCurPlayer->CheckRoom() == MAX_ROOM_SPACE)
		{
			m_bFinished = TRUE;
			return;
		}
		//Buy TDP
		if (NeedBuyAmulet())
		{
			g_pCurPlayer->BuyItem(1);
			return;
		}
		m_bFinished = TRUE;
		return;
	}
	ClickToNpc(NpcPoints[npc_Groceries].x, NpcPoints[npc_Groceries].y);
}

void CVLMap::GetPharmacy()
{
	KUiMsgSel* pMsgSel = (KUiMsgSel*)GetPtrAddress(MSG_BASE_ADD);
	KUiShop* pShop = (KUiShop*)GetPtrAddress(SHOP_BASE_ADD);
	int nItemPos, nItemCount;
	static BOOL bShopLoaded = 0;

	if (g_UiInformation2->IsVisible())
	{
		g_UiInformation2->Hide();
		return;
	}
	if (pMsgSel && pMsgSel->IsVisible())
	{
		if (pShop)
		{
			if (strstr(pMsgSel->m_pMessages[1]->Msg, FULL_REPAIR))
			{
				pMsgSel->OnClickMsg(1); // Full Repair
				SystemMessage(TV(L"<color=yellow>Sửa kỹ đồ Hoàng Kim"));
			}
			else
				pMsgSel->OnClickMsg(pMsgSel->m_nNumMessage);
			return;
		}
		bShopLoaded = 0;
		for (int i = 0; i < pMsgSel->m_nNumMessage; i++)
		{
			char* szMsg = (char*)pMsgSel->m_pMessages[i]->Msg;
			if (strstr(szMsg, AGREE_TRADE))
			{
				pMsgSel->OnClickMsg(i);
				break;
			}
		}
		return;
	}
	if (m_bFinished)
	{
		ResetShoping();
		bShopLoaded = 0;
		m_bFinished = 0;
		m_bNeedPharmacy = 0;
		return;
	}
	if (pShop) //Do Shoping
	{
		if (!bShopLoaded)
		{
			char szItemDesc[128];
			for (int i = 0; i < pShop->m_nObjCount; i++)
			{
				g_pCoreShell->GetGameData(GDI_TRADE_ITEM_PRICE, (int)& pShop->m_pObjectList[i], (int)szItemDesc);
				for (int j = 0; j < MAX_SHOP_ITEMS; j++)
				{
					if (strstr(szItemDesc, m_ShopItem[j].szItemName))
					{
						m_ShopItem[j].nShopIdx = pShop->m_pObjectList[i].uId;
						break;
					}
				}
			}
			bShopLoaded = TRUE;
			return;
		}
		//Repair Items
		if (m_bNeedRepairItems)
		{
			GetRepairItems();
			return;
		}
		//Thieu cho trong: ban do (SellItems.txt / Nhat do khong giu) truoc khi mua
		int nNeedCells = MedicineNeedCount();
		if (theRecover.m_bUsePoison && P_PLAYER->m_PoisonState.nTime) nNeedCells++;
		if (MakeShopRoom(nNeedCells)) return;
		//Full Room
		if (g_pCurPlayer->CheckRoom() == MAX_ROOM_SPACE)
		{
			m_bFinished = TRUE;
			return;
		}
		//Buy Anti-poison
		if (theRecover.m_bUsePoison && P_PLAYER->m_PoisonState.nTime)
		{
			if (!g_pCurPlayer->EatMedicine(medicine_antipoison))
			{
				if (m_ShopItem[13].nShopIdx >= 0)
					g_pCurPlayer->BuyItem(m_ShopItem[13].nShopIdx); // Ngan Kieu Khu Doc Dan
				else if (m_ShopItem[17].nShopIdx >= 0)
					g_pCurPlayer->BuyItem(m_ShopItem[17].nShopIdx); // Long Dam Hoa Doc Dan
			}
			return;
		}
		nItemPos = m_TradeItem[0].nItemPos;
		nItemCount = g_pCurPlayer->GetItemsCount(item_medicine, m_ShopItem[nItemPos].nItemType);
		if (m_TradeItem[0].nBuyAmout > nItemCount && m_TradeItem[0].bBuyCheck)
		{
			g_pCurPlayer->BuyItem(m_ShopItem[nItemPos].nShopIdx);
			return;
		}
		nItemPos = m_TradeItem[1].nItemPos;
		nItemCount = g_pCurPlayer->GetItemsCount(item_medicine, m_ShopItem[nItemPos].nItemType);
		if (m_TradeItem[1].nBuyAmout > nItemCount && m_TradeItem[1].bBuyCheck)
		{
			g_pCurPlayer->BuyItem(m_ShopItem[nItemPos].nShopIdx);
			return;
		}
		nItemPos = m_TradeItem[2].nItemPos;
		nItemCount = g_pCurPlayer->GetItemsCount(item_medicine, m_ShopItem[nItemPos].nItemType);
		if (m_TradeItem[2].nBuyAmout > nItemCount && m_TradeItem[2].bBuyCheck)
		{
			g_pCurPlayer->BuyItem(m_ShopItem[nItemPos].nShopIdx);
			return;
		}
		m_bFinished = TRUE;
		return;
	}
	ClickToNpc(NpcPoints[npc_Pharmacy].x, NpcPoints[npc_Pharmacy].y);
}

BOOL CVLMap::GetReputation(int nRepType)
{
	KUiMsgSel* pMsgSel = (KUiMsgSel*)GetPtrAddress(MSG_BASE_ADD);
	KUiMsgSel2* pMsgSel2 = (KUiMsgSel2*)GetPtrAddress(MSG2_BASE_ADD);

	if (g_UiInformation2->IsVisible())
	{
		g_UiInformation2->Hide();
		m_bFinished = TRUE;
		m_nTimeWait = 2;
		return TRUE;
	}
	if (pMsgSel && pMsgSel->IsVisible() && pMsgSel->m_nNumMessage == 1)
	{
		pMsgSel->OnClickMsg(0);
		m_bFinished = TRUE;
		m_nTimeWait = 2;
		return TRUE;
	}
	if (pMsgSel2 && pMsgSel2->IsVisible())
	{
		char szMenuChoice[32] = "_Undefined_";
		if (nRepType == 0)
			strcpy(szMenuChoice, TV(L"Nhận điểm phúc duyên"));
		else
			strcpy(szMenuChoice, TV(L"Nhận điểm danh vọng"));
		for (int i = 0; i < pMsgSel2->m_nNumMessage; i++)
		{
			char* szMsg = (char*)pMsgSel2->m_pMessages[i]->Msg;
			if (TcvnStrStrI(szMsg, szMenuChoice))
				pMsgSel2->OnClickMsg(i);
		}
		return TRUE;
	}
	if (m_bFinished)
	{
		m_bFinished = 0;
		if (nRepType == 0)
			m_bNeedFuYuan = 0;
		else
			m_bNeedMingWang = 0;
		ResetShoping();
		return 0;
	}

	ClickToNpc(NpcPoints[npc_Reputation].x, NpcPoints[npc_Reputation].y);
	return TRUE;
}

// Map can toi bang xa phu: map train (Di theo MAP), neu "Ban do bat ky" thi lay map cua danh sach Theo toa do
static int DriverTargetMap()
{
	if (theMap.m_nTrainMapID) return theMap.m_nTrainMapID;
	if (theMove.m_bRouteMove && theMove.m_nNodesMapID && theMove.m_Nodes[0].x) return theMove.m_nNodesMapID;
	return 0;
}

// Map can toi bang xa phu = map dang dung va co danh sach Theo toa do o map nay
static BOOL SameMapNodes()
{
	if (theQuest.m_nYeSouMapID || (theQuest.m_bAutoDoQuest && theQuest.m_bGoTrainGetPK)) return FALSE;
	if (DriverTargetMap() != *g_pMapID) return FALSE;
	return theMove.m_bRouteMove && theMove.m_nNodesMapID == *g_pMapID && theMove.m_Nodes[0].x && theMove.m_Nodes[0].y;
}

// Danh sach toa do o ngay map dang dung: chay toi diem DAU TIEN cua danh sach (khong can xa phu). FALSE = khong co diem
// Theo MapPath cua map (khong co thi duong trong thanh City<MapID>.map) toi diem gan nhat, roi chay thang doan con lai
static BOOL RunToNearestNode()
{
	if (!theMove.m_bRouteMove || theMove.m_nNodesMapID != *g_pMapID) return FALSE;
	if (!theMove.m_Nodes[0].x || !theMove.m_Nodes[0].y) return FALSE;
	int nBest = 0;	// Diem dau tien cua danh sach
	int x, y;
	P_PLAYER->GetMapPos(&x, &y);
	int dMin = __distance(x, y, theMove.m_Nodes[0].x, theMove.m_Nodes[0].y);
	if (!s_bNodeDirect)
	{
		// Uu tien MapPath cua map (Maps\Path): node gan nhat -> theo canh -> node gan diem dau danh sach
		int nWalk = PathWalkTo(theMove.m_Nodes[0].x, theMove.m_Nodes[0].y);
		if (nWalk > 0) return TRUE;
		if (nWalk < 0 && theMap.RunningToPos(theMove.m_Nodes[nBest])) return TRUE;	// khong co MapPath: duong trong thanh
		s_bNodeDirect = TRUE;
		theMap.m_nLagCount = 0;
		theMap.m_nLastdZ = MAX_INTNUM;
	}
	// Game tu tim duong (GoTo) toi diem; GoTo ket thi chay thang, moi lan toi da ~500
	// (ra khoi vung an toan -> m_FightMode = 1, CVLMove tiep quan)
	int tx = theMove.m_Nodes[nBest].x, ty = theMove.m_Nodes[nBest].y;
	if (dMin <= 64) return TRUE;
	if (AutoGoTo(tx, ty) > 0) return TRUE;
	if (theMap.IsLagMoving(x, y, dMin)) return TRUE;
	if (dMin > 500)
	{
		tx = x + (tx - x) * 500 / dMin;
		ty = y + (ty - y) * 500 / dMin;
	}
	PathNoteMove(tx, ty, "Chay thang TD1");
	g_pCurPlayer->MoveTo(tx, ty);
	return TRUE;
}

// Da chon dong xa phu toi map train: bao Tab Di chuyen (CVLMove::GoTrainBreathe) khi ra ngoai thanh thi tu tim duong
// toi bai train (MapToMap neu xa phu dua toi map ke ben, roi MapPath toi diem dau tien), Tab Chien dau chi buff
static void GoTrainMark(int nTarget)
{
	if (!nTarget || theQuest.m_nYeSouMapID || (theQuest.m_bAutoDoQuest && theQuest.m_bGoTrainGetPK)) return;
	theMove.GoTrainStart(nTarget);
}

DWORD g_dwNoRouteTick = 0;	// Xa phu khong co map train + khong co lo trinh MapPath: cho 30s moi noi lai

void CVLMap::GetDriver()
{
	DWORD &dwNoRouteTick = g_dwNoRouteTick;
	if (dwNoRouteTick && GetTickCount() - dwNoRouteTick < 30000) return;
	dwNoRouteTick = 0;
	int nDriverOpt = m_nDriverOption;
	if (theQuest.m_nYeSouMapID) nDriverOpt = 4;

	if (theQuest.m_bAutoDoQuest && theQuest.m_bGoTrainGetPK)
	{
		m_nDriverStep = 1;
		nDriverOpt = theQuest.m_TongPKOpt.m_nDriverOpt + 10;
	}

	if (s_dwReturnTick)
	{	// Vua chon "Tro lai dia diem cu": ra ngoai thanh thi Breathe dung o tren (m_FightMode), Tab Di chuyen tu tim duong.
		// Sau 3s van trong thanh va chua doi map -> khong tro lai duoc: tat het cua so, bam lai xa phu chon "Nhung noi da di qua"
		if (GetTickCount() - s_dwReturnTick < 3000) return;
		s_dwReturnTick = 0;
		if (!P_PLAYER->m_FightMode && *g_pMapID == s_nReturnMap)
		{
			SystemMessage(0, "<color=yellow>Khong tro lai dia diem cu duoc - chon Nhung noi da di qua");
			CloseAllWindows(0);
			return;
		}
	}

	int nCount = theMain.GetMenuCount();
	int nTarget = (nDriverOpt < 10) ? DriverTargetMap() : 0;
	char szTarget[64] = "";
	if (nTarget)
	{
		if (nTarget >= 224 && nTarget <= 227)
			strcpy(szTarget, TV(L"Sa Mạc Mê Cung"));
		else
			theMain.GetMapName(nTarget, szTarget);
	}

	if (nCount > 0)
	{
		if (m_bNoDriverRecent) m_nDriverStep = 1;
		if (m_nDriverStep == 0)	// Return the recent place
		{
			theMove.m_bFindingWay = 0;
			BOOL bClicked = theMain.ClickMenuStr(TV(L"Trở lại địa điểm cũ"));
			m_nDriverStep = 1;
			m_nTimeWait = 5;
			if (bClicked)
			{
				GoTrainMark(nTarget);
				if (!SameMapNodes()) { s_dwReturnTick = GetTickCount(); s_nReturnMap = *g_pMapID; }
			}
			else if (!SameMapNodes())
				CloseAllWindows(0);	// Xa phu khong co dong "Tro lai dia diem cu"
			if (SameMapNodes())
			{
				// Map train = map thanh: thu xong thi Breathe tu chay ra danh sach toa do (khong chon dong khac)
				s_bTriedReturn = TRUE;
				s_dwTryReturnTick = GetTickCount();
				if (!bClicked)
				{
					SystemMessage(0, "<color=yellow>Xa phu khong co dong Tro lai dia diem cu - tu chay ra danh sach toa do");
					theMain.ClickMenuIdx(theMain.GetMenuCount());
				}
			}
		}
		else if (m_nDriverStep == 1 && SameMapNodes())
		{
			// Khong "Tro lai dia diem cu" duoc -> dong menu, tu chay ra danh sach toa do
			s_bTriedReturn = TRUE;
			s_dwTryReturnTick = 0;
			theMain.ClickMenuIdx(theMain.GetMenuCount());
		}
		else if (m_nDriverStep == 1)
		{
			m_bNoDriverRecent = 0;
			theMove.m_bFindingWay = TRUE;
			// Menu chinh co san map can toi (vd ten thanh) -> chon luon
			if (szTarget[0] && nDriverOpt != 3 && theMain.ClickMenuStr(szTarget))
			{
				GoTrainMark(nTarget);
				return;
			}
			if (theMain.ClickMenuStr(VISITED_PLACES, 0))
			{
				if (nDriverOpt == 3)
					theMain.ExitGame();
				else if (nDriverOpt == 4)
				{
					if (theMain.ClickMenuStr(TV(L"Đi đến nơi làm nhiệm vụ Dã Tẩu")))
						return;

					SystemMessage(TV(L"<color=yellow>Chưa nhận nhiệm vụ Dã Tẩu!!!"));
					theMain.ClickMenuStr(VISITED_PLACES);
				}
				else if (nDriverOpt == 6)
				{
					if (theMain.ClickMenuStr(TV(L"Đi Đến Bản Đồ Mật Cốc")))
						return;

					SystemMessage(TV(L"<color=yellow>Bản Đồ Mật Cốc chưưa khai mở!!!"));
					theMain.ClickMenuStr(VISITED_PLACES);
				}
				else
					theMain.ClickMenuStr(VISITED_PLACES);
			}
			else
			{
				if (theMain.ClickMenuStr(TV(L"Xác nhận luyện tập cá nhân")))
					return;
				if (theMain.ClickMenuStr(TV(L"Đúng vậy! Mau đưa ta đi!")))
					return;
				if (nTarget)
				{
					const char* szMapName = szTarget;
					if (theMain.ClickMenuStr(szMapName))
					{
						GoTrainMark(nTarget);
						return;
					}
					// Xa phu ghi ten ngan cua me cung (vd map "Mª Cung L­ìng Thñy §éng" -> dong "L­ìng Thñy §éng")
					if (!TcvnStrNCmpI(szMapName, TV(L"Mê Cung "), 8) && strlen(szMapName) > 11 && theMain.ClickMenuStr(szMapName + 8))
					{
						GoTrainMark(nTarget);
						return;
					}
					// Xa phu khong co ten map train: chon dong "Noi da di qua 1/2/3" theo cai dat (nDriverOpt 0-2)
					int nVisitedIdx = nDriverOpt;
					if (nVisitedIdx == 10) nVisitedIdx = 2;	// Nhiem vu PK ko xac dinh
					else if (nVisitedIdx > 10) nVisitedIdx -= 11;
					if (nVisitedIdx >= 0 && nVisitedIdx <= 2 && nVisitedIdx < theMain.GetMenuCount() - 1)	// Dong cuoi la dong ket thuc
					{
						SystemMessage(0, "<color=yellow>Xa phu khong co %s - chon Noi da di qua %d", szMapName, nVisitedIdx + 1);
						theMain.ClickMenuIdx(nVisitedIdx);
						return;
					}
					// Xa phu khong co map train (vd rot mang / moi login nen khong "Tro lai dia diem cu" duoc)
					// -> tu di theo du lieu MapPath (tinh ca cac dong "Nhung noi da di qua" dang hien)
					if (PathTravelStart(nTarget, TRUE))
					{
						GoTrainMark(nTarget);
						return;
					}
					// Che do khac 0-2 (hoac khong du dong) va khong co MapPath - dong menu, cho bo sung MapPath
					SystemMessage(0, "<color=yellow>Xa phu khong co %s, chua co lo trinh MapPath toi do - khong chon dong %d, thu lai sau 30s", szMapName, nDriverOpt + 1);
					theMain.ClickMenuIdx(theMain.GetMenuCount());
					dwNoRouteTick = GetTickCount();
					return;
				}

				if (nDriverOpt == 4)	// Nhiem vu DT ko xac dinh.
					nDriverOpt = 2;
				if (nDriverOpt == 10)	// Nhiem vu PK ko xac dinh.
					nDriverOpt = 2;
				else if (nDriverOpt > 10)
					nDriverOpt -= 11;

				theMain.ClickMenuIdx(nDriverOpt);
			}
		}
		else if (m_nDriverStep == 2)
		{
		}

		return;
	}

	// Kiem tra lai truoc khi bam xa phu: server lag -> tuong da mua TDP/mau/mana nhung chua mua that.	
	if (g_pCurPlayer->CheckRoom() != MAX_ROOM_SPACE)
	{
		BOOL bGroceries = NeedBuyAmulet();
		BOOL bPharmacy = NeedBuyMedicine();
		if (bGroceries || bPharmacy)
		{
			s_nRebuyCount++;
			SystemMessage(1, "<color=yellow>Chua mua du%s%s - quay lai mua (lan %d)",
				bGroceries ? " Tho Dia Phu" : "", bPharmacy ? " mau/mana" : "", s_nRebuyCount);
			m_bNeedGroceries = bGroceries;
			m_bNeedPharmacy = bPharmacy;
			m_bFinished = 0;
			m_bIsReady = 0;
			ResetShoping(TRUE);
			return;
		}
	}

	ClickToNpc(NpcPoints[m_nDriverIndex].x, NpcPoints[m_nDriverIndex].y);
}

void CVLMap::UseThanHanhPhu()
{
	if (theMain.ClickMenuStr(TV(L"Sử dụng thuật thần hành có thể")))
	{
		SystemMessage(TV(L"<color=yellow>Sử dụng thuật thần hành có thể"));
		return;
	}

	if (m_nTrainMapID < 920 && theMain.ClickMenuStr(TV(L"Bản đồ cấp 90")))
		return;

	if (m_nTrainMapID >= 920 && theMain.ClickMenuStr(TV(L"Bản đồ luyện công mới")))
		return;

	BOOL bClicked = FALSE;

	char szMapName[64];
	theMain.GetMapName(m_nTrainMapID, szMapName);
	auto uLen = strlen(szMapName);
	if (uLen > 3)
		szMapName[uLen - 2] = 0;

	switch (m_nTrainMapID)
	{
	case 225:
		bClicked = theMain.ClickMenuStr(TV(L"Sa Mạc Mê Cung 1"));
		break;
	case 226:
		bClicked = theMain.ClickMenuStr(TV(L"Sa Mạc Mê Cung 2"));
		break;
	case 227:
		bClicked = theMain.ClickMenuStr(TV(L"Sa Mạc Mê Cung 3"));
		break;
	case 321:
		bClicked = theMain.ClickMenuStr(TV(L"Trường Bạch Sơn Nam"));
		break;
	case 322:
		bClicked = theMain.ClickMenuStr(TV(L"Trường Bạch Sơn Bắc"));
		break;
	default:
		bClicked = theMain.ClickMenuStr(szMapName);
		break;
	}

	if (bClicked)
		return;

	if (m_nTrainMapID < 200)
	{
		if (theMain.ClickMenuStr(TV(L"Trang kế")))
			return;
	};

	if (m_nTimeWait)
	{
		m_nTimeWait--;
		return;
	}

	g_pCurPlayer->UseItem(TV(L"Thần Hành Phù"));
	m_nTimeWait = MAX_TIME_WAIT;
}

// Goi khi bang shop dang mo, truoc khi mua (bat "Ban do" tab Ban do): ban HET do trong SellItems.txt / do Nhat do
// khong giu (SellItems, moi nhip 1 mon, bo do dang cam tren chuot vao hanh trang) roi moi mua.
// Truoc day day ruong thi GetGroceries / GetPharmacy bo qua luon, khong mua gi.
// Mon khong ban duoc (server tu choi) se bi chon lai mai => toi da MAX_SHOP_SELL lan trong 60s
#define MAX_SHOP_SELL	60
BOOL CVLMap::MakeShopRoom(int nNeed)
{
	static int s_nSold = 0;
	static DWORD s_dwFirstTick = 0;
	static BOOL s_bSaid = FALSE;
	if (s_dwFirstTick && GetTickCount() - s_dwFirstTick > 60000)
	{
		s_nSold = 0;
		s_dwFirstTick = 0;
		s_bSaid = FALSE;
	}
	if (!m_bSellItems || nNeed <= 0) return FALSE;
	if (s_nSold < MAX_SHOP_SELL && SellItems())
	{
		if (!s_nSold)
		{
			s_dwFirstTick = GetTickCount();
			SystemMessage(TV(L"<color=yellow>Bán đồ trước khi mua..."));
		}
		s_nSold++;
		return TRUE;
	}
	int nFree = EQUIP_ROOM.m_nWidth * EQUIP_ROOM.m_nHeight - g_pCurPlayer->CheckRoom();
	if (!s_bSaid && (s_nSold || nFree < nNeed))
	{
		s_bSaid = TRUE;
		if (!s_dwFirstTick) s_dwFirstTick = GetTickCount();
		SystemMessage(0, TV(L"<color=yellow>Đã bán %d lần, hành trang còn %d ô trống (cần %d)%s"), s_nSold, nFree, nNeed,
			s_nSold >= MAX_SHOP_SELL ? TV(L" - bán quá nhiều lần, dừng bán") : "");
	}
	return FALSE;
}

BOOL CVLMap::SellItems()
{
	for (int i = 0; i < MAX_PLAYER_ITEM; i++)
	{
		if (PLAYER_ITEM(i).nPlace != pos_equiproom) continue;
		KItem* pItem = (KItem*)(dwItmBaseAdd + PLAYER_ITEM(i).nIdx * ITM_DATA_SIZE);
		if (IsInSellList(ComAttr.szItemName))
		{
			g_pCurPlayer->SellItem(PLAYER_ITEM(i).nIdx);
			return TRUE;
		}
		if (theCollect.KeepTheItem(PLAYER_ITEM(i).nIdx, TRUE) == keep_nothing)
		{
			g_pCurPlayer->SellItem(PLAYER_ITEM(i).nIdx);
			return TRUE;
		}
	}
	if (PLAY_HAND_IDX)
	{
		POINT itmPos;
		KItem* pItem = (KItem*)(dwItmBaseAdd + PLAY_HAND_IDX * ITM_DATA_SIZE);
		if (EQUIP_ROOM.FindRoom(ComAttr.nWidth, ComAttr.nHeight, &itmPos))
		{
			g_pCurPlayer->SwitchItem(UOC_ITEM_TAKE_WITH, itmPos.x, itmPos.y);
			return TRUE;
		}
		else
			return 0;
	}
	return 0;
}

BOOL CVLMap::SaveItems()
{
	KItem* pItem;
	POINT itmPos;
	KInventory Room0, Room1, Room2, Room3;
	Room0 = PLAYER_ROOM[room_equipment];
	Room1 = PLAYER_ROOM[room_repository];
	Room2 = PLAYER_ROOM[room_repo02];
	Room3 = PLAYER_ROOM[room_repo03];
	if (PLAY_HAND_IDX)
	{
		pItem = (KItem*)(dwItmBaseAdd + PLAY_HAND_IDX * ITM_DATA_SIZE);
		int nContainer = 0;
		if (Room1.FindRoom(ComAttr.nWidth, ComAttr.nHeight, &itmPos))
			nContainer = UOC_STORE_BOX;
		else if (m_nSaveItemMode > 1 && Room2.FindRoom(ComAttr.nWidth, ComAttr.nHeight, &itmPos))
			nContainer = UOC_STORE_BOX2;
		else if (m_nSaveItemMode > 2 && Room3.FindRoom(ComAttr.nWidth, ComAttr.nHeight, &itmPos))
			nContainer = UOC_STORE_BOX3;
		else if (Room0.FindRoom(ComAttr.nWidth, ComAttr.nHeight, &itmPos))
			nContainer = UOC_ITEM_TAKE_WITH;
		if (nContainer)
		{
			g_pCurPlayer->SwitchItem(nContainer, itmPos.x, itmPos.y);
			return TRUE;
		}
		return (nContainer > UOC_ITEM_TAKE_WITH);
	}
	for (int i = 0; i < MAX_PLAYER_ITEM; i++)
	{
		BOOL bHaveRoom = 0;
		if (PLAYER_ITEM(i).nPlace != pos_equiproom) continue;
		if (PLAYER_ITEM(i).nY == 9) continue;						// Items at last line
		pItem = (KItem*)(dwItmBaseAdd + PLAYER_ITEM(i).nIdx * ITM_DATA_SIZE);
		if (ComAttr.nItemGenre == item_medicine) continue;
		if (IsInNoSaveList(ComAttr.szItemName)) continue;	// Maps\RuongItem.txt
		if (theRecover.m_bUseDoubleExp || theQuest.m_AddPointOpt.m_bUseTienThaoLo)
		{
			if (theRecover.IsInX2List(ComAttr.szItemName)) continue;	// Tien Thao Lo
		}
		if (theMap.m_bGiveKCL)
		{
			if (ComAttr.nItemGenre == item_special &&
				ComAttr.nDetailType == 1 &&
				ComAttr.nParticularType == 1499) continue;	// Khieu chien lenh
		}
		if (ComAttr.nObjIdx == 19) continue;	// Ex-Medicine...
		if (ComAttr.nObjIdx == 38) continue;	// SDT, TDP...
		if (ComAttr.nObjIdx == 383) continue;	// Tong Kim Chieu thu
		if (ComAttr.bStack && pItem->m_nCurDur < pItem->m_nMaxDur) continue;	// Stackable items..
		if (Room1.FindRoom(ComAttr.nWidth, ComAttr.nHeight, &itmPos))
			bHaveRoom = TRUE;
		else if (m_nSaveItemMode > 1 && Room2.FindRoom(ComAttr.nWidth, ComAttr.nHeight, &itmPos))
			bHaveRoom = TRUE;
		else if (m_nSaveItemMode > 2 && Room3.FindRoom(ComAttr.nWidth, ComAttr.nHeight, &itmPos))
			bHaveRoom = TRUE;
		if (bHaveRoom)
		{
			g_pCurPlayer->SwitchItem(UOC_ITEM_TAKE_WITH, PLAYER_ITEM(i).nX, PLAYER_ITEM(i).nY);
			return TRUE;
		}
	}
	return 0;
}

BOOL CVLMap::CheckReady()
{
	int nItemIdx;
	KItem* pItem;
	KUiPlayerBar* pUiPlayerBar = (KUiPlayerBar*)GetPtrAddress(UPB_BASE_ADD);
	if (!pUiPlayerBar) return TRUE;
	static BOOL bNeedBuffHP = TRUE;
	if (m_bPutMoney && EQUIP_MONEY > m_nMoneyAmount)
	{
		int nSaveAmount = EQUIP_MONEY - m_nMoneyAmount;
		g_pCoreShell->OperationRequest(GOI_MONEY_INOUT_STORE_BOX, TRUE, nSaveAmount);
		return 0;
	}
	for (int i = 0; i < MAX_EQUIP_SLOT; i++)
	{	// Kiem tra do ben
		nItemIdx = g_pCurPlayer->m_ItemList.GetEquip(i);
		if (!nItemIdx) continue;
		pItem = (KItem*)(dwItmBaseAdd + nItemIdx * ITM_DATA_SIZE);
		if (pItem->m_nCurDur >= 0 && pItem->m_nCurDur < 3)
		{
			SystemMessage(TV(L"<color=yellow>Trang bị sắp hỏng, sao còn đòi đi đâu?"));
			return 0;
		}
	}
	if (P_PLAYER->m_PoisonState.nTime)
	{
		SystemMessage(TV(L"<color=yellow>Đèn thùi lùi, đi đâu người ta cười chết."));
		return 0;
	}

	if (!theMain.HaveTownPortal())
	{
		SystemMessage(TV(L"<color=yellow>Không mang TĐP, sao còn dám đi đâu?"));
		return 0;
	}
	if (bNeedBuffHP)
	{
		if (theRecover.m_bAutoHP && P_PLAYER->m_CurrentLife < theRecover.m_nAutoHPAmount)
			g_pCurPlayer->EatMedicine(medicine_blood);
		if (theRecover.m_bAutoMP && P_PLAYER->m_CurrentMana < theRecover.m_nAutoMPAmount)
			g_pCurPlayer->EatMedicine(medicine_mana);
		bNeedBuffHP = 0;
		return 0;
	}
	bNeedBuffHP = TRUE;
	return TRUE;
}

void CVLMap::ClickToNpc(int nX, int nY)
{
	int nPlayerX, nPlayerY, nNpcX, nNpcY, dZ;
	//Search & Found
	if (m_nTimeWait)
	{
		if (m_nTimeWait == 5)
		{
			int nX = (P_PLAYER->m_SyncSignal & 0x07) << 3;
			char szScript[32];
			sprintf(szScript, "MoveTo(%d, 0)", nX);
			ExecuteScript(szScript);
			SystemMessage(TV(L"<color=yellow>Lag quá, chờ 5s click lại..."));
		}
		//if (m_nTimeWait == 1)
			//QTRACE("ClickToNpc: het thoi gian cho, PEOPLE_IDX=%d Doing=%d", PEOPLE_IDX, P_PLAYER->m_Doing);
		m_nTimeWait--;
		return;
	}

	P_PLAYER->GetMapPos(&nPlayerX, &nPlayerY);
	int nDialogers = 0, nNearIdx = 0, nNearDist = MAX_INTNUM;
	for (int i = 1; i < MAX_NPC; i++)
	{
		if (!g_pNpcArray[i]->IsExist() || g_pNpcArray[i]->m_Kind != kind_dialoger) continue;

		g_pNpcArray[i]->GetMapPos(&nNpcX, &nNpcY);
		dZ = __distance(nX, nY, nNpcX, nNpcY);
		nDialogers++;
		if (dZ < nNearDist) { nNearDist = dZ; nNearIdx = i; }
		if (dZ > 40) continue;

		dZ = __distance(nPlayerX, nPlayerY, nNpcX, nNpcY);
		if (dZ > PLAYER_PICKUP_DISTANCE)
		{
			//QTRACE("ClickToNpc: thay #%d %s, cach nguoi %d > %d -> chay toi", i, g_pNpcArray[i]->Name,dZ, PLAYER_PICKUP_DISTANCE);
			g_pCurPlayer->MoveTo(nNpcX, nNpcY);
			return;
		}

		theAttack.SetPeopleIdx(i);
		//QTRACE("ClickToNpc: SetPeopleIdx(#%d %s) -> PEOPLE_IDX=%d", i, g_pNpcArray[i]->Name, PEOPLE_IDX);
		m_nTimeWait = MAX_TIME_WAIT; //wait for 5s for next
		return;
	}
	//QTRACE("ClickToNpc: KHONG co NPC hoi thoai trong 40 quanh (%d,%d). Tong dialoger=%d, gan nhat #%d %s cach %d",nX, nY, nDialogers, nNearIdx, nNearIdx ? g_pNpcArray[nNearIdx]->Name : "-", nNearIdx ? nNearDist : -1);
}

void CVLMap::LoadSellList()
{
	memset(m_szSellItems, 0, sizeof(m_szSellItems));
	char szSellItmFile[MAX_PATH];
	sprintf(szSellItmFile, "%s\\Maps\\SellItems.txt", theMain.m_szCurFolder);

	DTextFile itemFile;
	if (!itemFile.Open(szSellItmFile))
		return;

	itemFile.ReadAllLines(m_szSellItems[0], MAX_SELL_ITEMS, 80);
}

void CVLMap::LoadNoSaveList()
{
	memset(m_szNoSaveItems, 0, sizeof(m_szNoSaveItems));
	char szNoSaveFile[MAX_PATH];
	sprintf(szNoSaveFile, "%s\\Maps\\RuongItem.txt", theMain.m_szCurFolder);

	DTextFile itemFile;
	if (!itemFile.Open(szNoSaveFile))
		return;

	itemFile.ReadAllLines(m_szNoSaveItems[0], MAX_NOSAVE_ITEMS, 32);

	int i = 0;
	while (i < MAX_NOSAVE_ITEMS && m_szNoSaveItems[i][0])
	{
		_strlwr(m_szNoSaveItems[i]);
		i++;
	}
}

// Ten vat pham chua 1 dong trong RuongItem.txt (khong phan biet hoa thuong ASCII, giong BlackItems)
BOOL CVLMap::IsInNoSaveList(const char* szItemName)
{
	char szName[84] = "";
	strncpy(szName, szItemName, 80);
	_strlwr(szName);
	int i = 0;
	while (i < MAX_NOSAVE_ITEMS && m_szNoSaveItems[i][0])
	{
		if (strstr(szName, m_szNoSaveItems[i]))
			return TRUE;
		i++;
	}
	return 0;
}

BOOL CVLMap::IsInSellList(const char* szItemName)
{
	int i = 0;
	while (m_szSellItems[i][0] && i < MAX_SELL_ITEMS)
	{
		if (strstr(szItemName, m_szSellItems[i]))
			return TRUE;
		i++;
	}
	return 0;
}

void CVLMap::ResetShoping(BOOL bResetAll)
{
	CloseAllWindows(0);
	NearestDriver();
	m_nTimeWait = 2;
	if (bResetAll)
		m_bDoShoping = 0;
}
