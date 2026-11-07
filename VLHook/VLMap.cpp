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
	m_ptRunNpc.x = m_ptRunNpc.y = 0;
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

//------------------------------------------------------------------ tien mua do trong thanh
// Rut tien ngay o cua hang khi con phai mua ma tien tren nguoi it hon muc can (CheckReady da cat tien truoc do,
// hoac quay lai mua bu tu xa phu), cat lai o CheckReady sau khi mua xong
static int s_nTownMoneyBase = -1;		// Tien tren nguoi truoc lan rut dau tien o thanh nay (-1 = chua rut) - CheckReady cat ve muc nay
static int s_nShopMoneyBefore = -1;		// Dang cho tien rut ve: tien tren nguoi luc rut (-1 = khong cho)
static DWORD s_dwShopMoneyTick = 0;		// Luc rut tien / bat dau mo khoa ruong
static BOOL s_bShopWithdrew = FALSE;	// Lan ghe cua hang nay da rut (hoac bo qua) - reset o ResetShoping
static BOOL s_bShopNoMoney = FALSE;		// Mua khong duoc vi het tien: xa phu khong quay lai mua bu (reset o InitValues)
static int s_nBuyMoney = -1;			// Tien tren nguoi lan BuyItem gan nhat (doi = mua duoc)
static DWORD s_dwBuyTick = 0;
static int s_nShopEat = 0;				// Da uong o tiem thuoc lan ghe nay: 1 = mau, 2 = mana (moi loai 1 binh)
BOOL g_bQuestGoTrainFirst = FALSE;		// Ngung nhiem vu -> lan dau len map luyen cong (chua ra khoi thanh lan nao)
DWORD g_dwTownEatHP = 0;				// Luc uong binh mau / mana o tiem thuoc (tab Nhiem vu: Refill khong uong them)
DWORD g_dwTownEatMP = 0;
static DWORD s_dwShopEatTick = 0;

#define SHOP_MONEY_DEF		100000	// Chua dat "Rut tien": can 10 van tren nguoi de mua
#define SHOP_BUY_STALL		8000	// Goi BuyItem 8s ma tien khong doi => khong mua duoc (het tien)

static void ShopMoneyReset()
{
	s_nShopMoneyBefore = -1;
	s_dwShopMoneyTick = 0;
	s_bShopWithdrew = FALSE;
	s_nBuyMoney = -1;
	s_dwBuyTick = 0;
	s_nShopEat = 0;
	s_dwShopEatTick = 0;
}

// Cua hang dang mo va con phai mua: tien tren nguoi < muc can thi rut tu ruong. TRUE = dang mo khoa / cho tien ve
static BOOL ShopWithdraw()
{
	DWORD dwNow = GetTickCount();
	if (s_nShopMoneyBefore >= 0)
	{
		if (EQUIP_MONEY <= s_nShopMoneyBefore && dwNow - s_dwShopMoneyTick < 5000) return TRUE;	// Cho tien ve tren nguoi
		s_nShopMoneyBefore = -1;
		return FALSE;
	}
	if (s_bShopWithdrew) return FALSE;
	int nWant = theMap.m_nGetMoneyAmount > 0 ? theMap.m_nGetMoneyAmount * 10000 : SHOP_MONEY_DEF;
	if (EQUIP_MONEY >= nWant || ROOM_MONEY <= 0) { s_bShopWithdrew = TRUE; return FALSE; }
	if (!s_dwShopMoneyTick) s_dwShopMoneyTick = dwNow;
	if (theMain.OpenLockRoom(theMap.m_nRepositoryPass))
	{
		theMap.m_bRoomLocked = TRUE;
		if (dwNow - s_dwShopMoneyTick < 15000) return TRUE;
		SystemMessage(1, TV(L"<color=red>Mở khóa rương 15s không được - mua bằng tiền đang có (%d)"), EQUIP_MONEY);
		s_bShopWithdrew = TRUE;
		return FALSE;
	}
	int nTake = nWant - EQUIP_MONEY;
	if (nTake > ROOM_MONEY) nTake = ROOM_MONEY;
	if (s_nTownMoneyBase < 0) s_nTownMoneyBase = EQUIP_MONEY;
#if ADMIN_MODE
	if (g_bDbgText) SystemMessage(1, TV(L"<color=yellow>Rút %d lượng để mua (trên người %d, rương %d)"), nTake, EQUIP_MONEY, ROOM_MONEY);	// Tat debug text
#endif
	g_pCoreShell->OperationRequest(GOI_MONEY_INOUT_STORE_BOX, 0, nTake);
	s_nShopMoneyBefore = EQUIP_MONEY;
	s_dwShopMoneyTick = dwNow;
	s_bShopWithdrew = TRUE;
	return TRUE;
}

// Goi truoc moi lan BuyItem: tien khong doi qua SHOP_BUY_STALL => khong mua duoc (het tien), TRUE = dung mua
static BOOL ShopBuyStalled(const char *szWhat)
{
	DWORD dwNow = GetTickCount();
	if (EQUIP_MONEY != s_nBuyMoney || !s_dwBuyTick) { s_nBuyMoney = EQUIP_MONEY; s_dwBuyTick = dwNow; return FALSE; }
	if (dwNow - s_dwBuyTick < SHOP_BUY_STALL) return FALSE;
	SystemMessage(1, TV(L"<color=red>Không mua được %s (trên người %d lượng, rương %d) - bỏ qua, không quay lại mua"), szWhat, EQUIP_MONEY, ROOM_MONEY);
	s_bShopNoMoney = TRUE;
	return TRUE;
}

// Tiem thuoc, da mua du: mau/mana duoi muc tu uong thi uong luon o day roi mua bu (tranh CheckReady uong
// xong lai thieu binh, ra xa phu moi biet phai quay lai). TRUE = vua uong / cho so binh cap nhat
static BOOL ShopTopUpMedicine()
{
	DWORD dwNow = GetTickCount();
	if (s_dwShopEatTick && dwNow - s_dwShopEatTick < 1500) return TRUE;	// Cho so binh trong hanh trang giam (lag)
	BOOL bEat = FALSE;
	if (!(s_nShopEat & 1) && theRecover.m_bAutoHP && P_PLAYER->m_CurrentLife < theRecover.m_nAutoHPAmount && P_PLAYER->m_CurrentLife * 10 < P_PLAYER->GetLifeMax() * 7 && g_pCurPlayer->EatMedicine(medicine_blood))
	{
		s_nShopEat |= 1;
		g_dwTownEatHP = dwNow;
		bEat = TRUE;
	}
	if (!(s_nShopEat & 2) && theRecover.m_bAutoMP && P_PLAYER->m_CurrentMana < theRecover.m_nAutoMPAmount && P_PLAYER->m_CurrentMana * 10 < P_PLAYER->GetManaMax() * 7 && g_pCurPlayer->EatMedicine(medicine_mana))
	{
		s_nShopEat |= 2;
		g_dwTownEatMP = dwNow;
		bEat = TRUE;
	}
	if (!bEat) return FALSE;
	s_dwShopEatTick = dwNow;
	return TRUE;
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
	s_nTownMoneyBase = -1;
	s_bShopNoMoney = FALSE;
	ShopMoneyReset();
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
	if (!m_nSaveMapID && theQuest.m_bNeedGoTrain)	// Map luyen cong khong co thanh ve: luu ruong o thanh chinh hien tai
		m_nSaveMapID = theMain.GetOwnerCity(*g_pMapID);
	NearestDriver();
}

static BOOL CustomBoxActive();

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
			g_bQuestGoTrainFirst = FALSE;	// Da ra khoi thanh: cac lan sau dung xa phu binh thuong
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
		//if (ChangeMap(nOwnMapID)) return;

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
		if (CustomBoxActive() && ChangeMap(m_nBoxMapID)) return;
		if (!m_bDoShoping && RunningToBox()) return;
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
		sprintf(szMsg, TV(L"<color=yellow>Về thành tự quay lại (Ctrl+Y): <color=green>Bật"));
	else
	{
		sprintf(szMsg, TV(L"<color=yellow>Về thành tự quay lại (Ctrl+Y): <color>Tắt"));
		m_nCurPoint = -1;
		m_bDoShoping = 0;
		PathTravelStop("tat Ve thanh tu quay lai"); // Tắt về thành quay lại
	}
	SystemMessage(szMsg, 1);
}

// Toa do NPC trong thanh: muc [NPC] cua Maps\Path\<MapID>_<Ten>.txt (thay CityMaps\City<MapID>.map).
// m_nPoints = so NPC co toa do (0 = map khong co / chua ghi [NPC])
BOOL CVLMap::LoadMap(int nMapID)
{
	int n = PathLoadNpcs(nMapID, NpcPoints);
	m_nPoints = max(n, 0);
	return n > 0;
}

// Ruong tuy chon (Tab Ban do: "Luu ruong" + "lay vi tri"): TRUE = bat va da lay vi tri
static BOOL CustomBoxActive()
{
	return theMap.m_bCustomBox && theMap.m_nBoxMapID && (theMap.m_nBoxX || theMap.m_nBoxY);
}

// Chay toi ruong cat do: dang o map ruong tuy chon thi chay toi toa do da lay,
// khong thi chay toi Ruong mac dinh cua thanh ([NPC] trong MapPath). TRUE = dang di
// (Doi sang map ruong tuy chon: noi goi ChangeMap(m_nBoxMapID) truoc, khong phu thuoc m_bDoShoping)
BOOL CVLMap::RunningToBox()
{
	if (CustomBoxActive() && *g_pMapID == m_nBoxMapID)
	{
		POINT ptBox = { m_nBoxX, m_nBoxY };
		return RunningToPos(ptBox);
	}
	return RunningToPos(NpcPoints[npc_Repository]);
}

// Chay toi gan NPC (ptDest): theo MapPath cua map (PathWalkTo), het duong / map khong co MapPath thi chay thang
// (KPlayer::MoveTo tu ne vat can). Tra ve TRUE = dang chay, 0 = da toi gan (goi tiep ClickToNpc)
#define CITY_NPC_NEAR		200		// Toi gan NPC hon muc nay thi xong (ClickToNpc tu chay not toi NPC)
#define CITY_NPC_SEEN		500		// Doan chay thang khong tien them 3s ma da cach NPC < muc nay: coi nhu toi
BOOL CVLMap::RunningToPos(POINT ptDest)
{
	static POINT s_ptDest = { 0, 0 };
	static int s_nBest = MAX_INTNUM;
	static DWORD s_dwBest = 0, s_dwHorse = 0;
	static int s_nDestMap = 0;
	if (!ptDest.x && !ptDest.y) return 0;	// Thanh khong co NPC nay ([NPC] = 0,0)

	int nPlayerX, nPlayerY;
	P_PLAYER->GetMapPos(&nPlayerX, &nPlayerY);
	int dZ = __distance(nPlayerX, nPlayerY, ptDest.x, ptDest.y);
	DWORD dwNow = GetTickCount();
	if (m_nCurPoint < 0 || ptDest.x != s_ptDest.x || ptDest.y != s_ptDest.y || *g_pMapID != s_nDestMap)
	{	// Dich moi / doi map: tinh lai tu dau
		s_ptDest = ptDest;
		s_nDestMap = *g_pMapID;
		PathWalkReset();
		s_nBest = MAX_INTNUM;
		s_dwBest = dwNow;
		m_nLagCount = 0;
		m_nLastdZ = MAX_INTNUM;
	}
	m_ptRunNpc = ptDest;
	if (dZ <= CITY_NPC_NEAR)
	{
		m_nCurPoint = -1;
		return 0;
	}
	m_nCurPoint = 0;
	if (!P_PLAYER->m_bRideHorse && HAVE_HORSE && dwNow - s_dwHorse > 3000)
	{
		s_dwHorse = dwNow;
		ExecuteScript(SCK_SHORTCUT_HORSE); // Len ngua
	}
	if (PathWalkTo(ptDest.x, ptDest.y) > 0) return TRUE;

	// Doan cuoi: chay thang toi NPC
	if (dZ < s_nBest - 32) { s_nBest = dZ; s_dwBest = dwNow; }
	if (dZ <= CITY_NPC_SEEN && dwNow - s_dwBest > 3000)
	{	// Khong tien them (NPC dung tren o vat can...) - de ClickToNpc chay toi NPC that
		m_nCurPoint = -1;
		return 0;
	}
	if (IsLagMoving(nPlayerX, nPlayerY, dZ)) return TRUE;
	PathNoteMove(ptDest.x, ptDest.y, "NPC");
	g_pCurPlayer->MoveTo(ptDest.x, ptDest.y);
	return TRUE;
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

	// Chon ruong gan nhat (ruong tuy chon: co the co nhieu ruong trong tam nhin)
	int nBest = 0, dMin = MAX_INTNUM;
	P_PLAYER->GetMapPos(&nPlayerX, &nPlayerY);
	if ((*g_pMapID) >= city_visondao)
	{
		for (int i = 1; i < MAX_NPC; i++)
		{
			if (!g_pNpcArray[i]->IsExist() || g_pNpcArray[i]->m_Kind != kind_dialoger) continue;
			if (strcmp(g_pNpcArray[i]->Name, TV(L"Rương chứa đồ"))) continue;
			g_pNpcArray[i]->GetMapPos(&nNpcX, &nNpcY);
			dZ = __distance(nPlayerX, nPlayerY, nNpcX, nNpcY);
			if (dZ < dMin) { dMin = dZ; nBest = i; }
		}
		if (nBest)
		{
			g_pNpcArray[nBest]->GetMapPos(&nNpcX, &nNpcY);
			if (dMin > MAX_MOVE_DISTANCE)
				g_pCurPlayer->MoveTo(nNpcX, nNpcY);
			else
			{
				theAttack.SetPeopleIdx(nBest);
				m_nTimeWait = MAX_TIME_WAIT;
			}
			bFound = TRUE;
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
			if (dZ < dMin) { dMin = dZ; nBest = i; }
		}
		if (nBest)
		{
			g_pObjArray[nBest]->GetMapPos(&nObjX, &nObjY);
			if (dMin > PLAYER_PICKUP_DISTANCE)
				g_pCurPlayer->MoveTo(nObjX, nObjY);
			else
			{
				P_PLAYER->m_nObjectIdx = nBest;
				m_nTimeWait = MAX_TIME_WAIT;
			}
			bFound = TRUE;
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
		// Muc [NPC] cua Maps\Path\<MapID>_<Ten>.txt
		if (!PathSaveNpc(*g_pMapID, npc_Repository, nRepX, nRepY))
		{
			SystemMessage(TV(L"<color=yellow>Lỗi ghi file MapPath, xin kiểm tra lại!!!"));
			return;
		}
		NpcPoints[npc_Repository].x = nRepX;
		NpcPoints[npc_Repository].y = nRepY;
		SystemMessage(TV(L"<color=yellow>Đã lưu thành công tọa độ Rương đồ mới!"));
		m_nCurPoint = -1;
		m_bDoShoping = 0;
		return;
	}
	SystemMessage(TV(L"<color=yellow>Không tìm thấy Rương đồ!!!"));
}

void CVLMap::FirstRepository()
{
	int nSaveMap = m_nSaveMapID;
	if (theQuest.m_nYeSouMapID)
		nSaveMap = theQuest.m_nSaveMapID;
	if (CustomBoxActive())
		nSaveMap = m_nBoxMapID;
	if (!nSaveMap)
	{
		m_bFirstSaveRepos = 0;
		return;
	}
	if (ChangeMap(nSaveMap)) return;
	if (!m_bDoShoping && RunningToBox()) return;
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

// Chon "Sua ky [..." trong bang hoi sua do Hoang Kim.
// Index dong "Sua ky" khong co dinh (co bang [0]=Sua ky, [1]=Tam thoi khong sua),
// nen quet het cac dong tim FULL_REPAIR roi click. Khong thay -> giu hanh vi cu (click dong cuoi).
template <class TMsgSel>
static void ClickRepairChoice(TMsgSel* pMsgSel)
{
	for (int i = 0; i < pMsgSel->m_nNumMessage; i++)
	{
		if (strstr((char*)pMsgSel->m_pMessages[i]->Msg, FULL_REPAIR))
		{
			pMsgSel->OnClickMsg(i);		// Sua ky do Hoang Kim
			SystemMessage(TV(L"<color=yellow>Sửa kỹ đồ Hoàng Kim"));
			return;
		}
	}
	pMsgSel->OnClickMsg(pMsgSel->m_nNumMessage);
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
			ClickRepairChoice(pMsgSel);
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
			if (ShopWithdraw()) return;	// Rut tien truoc khi mua
			if (ShopBuyStalled(TV(L"Thổ Địa Phù"))) { m_bFinished = TRUE; return; }
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
			ClickRepairChoice(pMsgSel);
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
		//Rut tien truoc khi mua; tien khong giam qua 8s (het tien) thi thoi mua
		if (nNeedCells > 0)
		{
			if (ShopWithdraw()) return;
			if (ShopBuyStalled(TV(L"thuốc"))) { m_bFinished = TRUE; return; }
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
		//Kiem tra lai truoc khi roi tiem: mau/mana thap thi uong o day, lan sau mua bu luon
		if (!s_bShopNoMoney && ShopTopUpMedicine()) return;
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

// Map can toi bang xa phu: map train (Di theo MAP), neu "Ban do bat ky" thi lay map cua danh sach Theo toa do,
// khong co thi lay map cua danh sach toa do di den noi luyen cong
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
// Theo MapPath cua map toi diem gan nhat, roi chay thang doan con lai
static BOOL RunToNearestNode()
{
	if (!theMove.m_bRouteMove || theMove.m_nNodesMapID != *g_pMapID) return FALSE;
	if (!theMove.m_Nodes[0].x || !theMove.m_Nodes[0].y) return FALSE;
	int nBest = 0;	// Diem dau tien cua danh sach
	int x, y;
	P_PLAYER->GetMapPos(&x, &y);
	int dMin = __distance(x, y, theMove.m_Nodes[0].x, theMove.m_Nodes[0].y);
	static int s_nNodeMap = 0;
	if (*g_pMapID != s_nNodeMap)
	{	// Doi map: MapPath map moi, tinh lai duong
		s_nNodeMap = *g_pMapID;
		s_bNodeDirect = FALSE;
		PathWalkReset();
	}
	if (!s_bNodeDirect)
	{
		// Uu tien MapPath cua map (Maps\Path): node gan nhat -> theo canh -> node gan diem dau danh sach
		int nWalk = PathWalkTo(theMove.m_Nodes[0].x, theMove.m_Nodes[0].y);
		if (nWalk > 0) return TRUE;
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
			SystemMessage(0, nDriverOpt == 3
				? "<color=yellow>Khong tro lai dia diem cu duoc - dung im o xa phu"
				: "<color=yellow>Khong tro lai dia diem cu duoc - chon Nhung noi da di qua");
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
			if (nDriverOpt == 3)
			{
				// "Quay lai dia diem cu": buoc 0 da thu "Tro lai dia diem cu" nhung khong ve duoc
				// -> dung im o xa phu (khong chon "Noi da di qua", khong thoat game), thu lai sau 30s
				SystemMessage(0, "<color=yellow>Khong tro lai dia diem cu duoc - dung im o xa phu");
				CloseAllWindows(0);
				dwNoRouteTick = GetTickCount();
				return;
			}
			theMove.m_bFindingWay = TRUE;
			// Menu chinh co san map can toi (vd ten thanh) -> chon luon
			if (szTarget[0] && nDriverOpt != 3 && theMain.ClickMenuStr(szTarget))
			{
				GoTrainMark(nTarget);
				return;
			}
			if (theMain.ClickMenuStr(VISITED_PLACES, 0))
			{
				// (opt 3 "Quay lai dia diem cu" da duoc xu ly o tren: dung im o xa phu, khong vao day)
				if (nDriverOpt == 4)
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
					// Ngung nhiem vu, lan dau len map luyen cong: khong chon dong khac, chay bo len map luyen cong
					// (cac lan sau ve thanh bang TDP thi nhu binh thuong: Tro lai dia diem cu / Noi da di qua / chay bo)
					int nVisitedIdx = (g_bQuestGoTrainFirst && theQuest.m_bNeedGoTrain && !theQuest.m_bGoTrainGetPK) ? -1 : nDriverOpt;
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
	if (g_pCurPlayer->CheckRoom() != MAX_ROOM_SPACE && !s_bShopNoMoney)	// Het tien mua: khong quay lai
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

// Cat do: dem so lan cam 1 mon tu tui len de cat. Server tu choi (ruong that da day / trang ruong
// chua tai / mon khong duoc cat) thi mon bi tra ve tui => cam len bo xuong mai. Qua 3 lan thi bo qua mon do.
#define SAVE_TRY_MAX	3
#define SAVE_TRY_SLOT	32
static int s_nSaveTryIdx[SAVE_TRY_SLOT];
static int s_nSaveTryCnt[SAVE_TRY_SLOT];
static DWORD s_dwSaveTryTick = 0;

static int* SaveTryCount(int nIdx)
{
	DWORD dwNow = GetTickCount();
	if (!s_dwSaveTryTick || dwNow - s_dwSaveTryTick > 120000)
	{	// Lan cat do moi (2 phut khong cam mon nao): dem lai tu dau
		memset(s_nSaveTryIdx, 0, sizeof(s_nSaveTryIdx));
		memset(s_nSaveTryCnt, 0, sizeof(s_nSaveTryCnt));
	}
	s_dwSaveTryTick = dwNow;
	int nFree = -1;
	for (int i = 0; i < SAVE_TRY_SLOT; i++)
	{
		if (s_nSaveTryIdx[i] == nIdx) return &s_nSaveTryCnt[i];
		if (nFree < 0 && !s_nSaveTryIdx[i]) nFree = i;
	}
	if (nFree < 0) nFree = nIdx % SAVE_TRY_SLOT;
	s_nSaveTryIdx[nFree] = nIdx;
	s_nSaveTryCnt[nFree] = 0;
	return &s_nSaveTryCnt[nFree];
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
		int* pTry = SaveTryCount(PLAYER_ITEM(i).nIdx);
		if (*pTry >= SAVE_TRY_MAX)
		{
			if (*pTry == SAVE_TRY_MAX)
			{
				(*pTry)++;
				SystemMessage(0, TV(L"<color=yellow>Không cất được '%s' vào rương (rương đầy hoặc không cho cất) => bỏ qua."), ComAttr.szItemName);
			}
			continue;
		}
		if (Room1.FindRoom(ComAttr.nWidth, ComAttr.nHeight, &itmPos))
			bHaveRoom = TRUE;
		else if (m_nSaveItemMode > 1 && Room2.FindRoom(ComAttr.nWidth, ComAttr.nHeight, &itmPos))
			bHaveRoom = TRUE;
		else if (m_nSaveItemMode > 2 && Room3.FindRoom(ComAttr.nWidth, ComAttr.nHeight, &itmPos))
			bHaveRoom = TRUE;
		if (bHaveRoom)
		{
			(*pTry)++;
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
	// Cat tien sau khi mua: giu m_nMoneyAmount (Cat tien), khong bat Cat tien thi cat phan vua rut de mua (ShopWithdraw)
	int nKeepMoney = m_bPutMoney ? m_nMoneyAmount : s_nTownMoneyBase;
	if (nKeepMoney >= 0 && EQUIP_MONEY > nKeepMoney)
	{
		int nSaveAmount = EQUIP_MONEY - nKeepMoney;
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
		if (theRecover.m_bAutoHP && P_PLAYER->m_CurrentLife < theRecover.m_nAutoHPAmount && P_PLAYER->m_CurrentLife * 10 < P_PLAYER->GetLifeMax() * 7)
			g_pCurPlayer->EatMedicine(medicine_blood);
		if (theRecover.m_bAutoMP && P_PLAYER->m_CurrentMana < theRecover.m_nAutoMPAmount && P_PLAYER->m_CurrentMana * 10 < P_PLAYER->GetManaMax() * 7)
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
	ShopMoneyReset();
	CloseAllWindows(0);
	NearestDriver();
	m_nTimeWait = 2;
	if (bResetAll)
		m_bDoShoping = 0;
}
