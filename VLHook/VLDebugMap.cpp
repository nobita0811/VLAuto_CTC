#include "stdafx.h"
#include "../GameDef.h"
#include "GameBase.h"
#include "JxCore.h"
#include "S3Client.h"
#include "VLClasses.h"
#include "VLDebug.h"

// Tab Ban do cua cua so DEBUG: cac ham game dung trong VLMap.cpp (ve thanh, mua ban, ruong, xa phu)

extern BOOL g_SysPause;

#define STOREBOX_NPC_NAME	TV(L"Rương chứa đồ")	// "Ruong chua do" (TCVN3) - giong VLMap.cpp

static const char* s_szNpcPointNames[npc_Nums] =
{
	"Ruong do", "Xa phu 1", "Xa phu 2", "Xa phu 3", "Xa phu 4", "Tap hoa", "Hieu thuoc",
	"VL truyen nhan", "Tho ren", "Dich quan", "Da Tau", "Ban ngua", "Reserved1", "Reserved2", "Reserved3",
};

static BOOL CheckNpcPointIdx(int nIdx)
{
	if (nIdx < 0 || nIdx >= npc_Nums)
	{
		DbgOut("LOI: Idx = %d ngoai 0..%d (0 ruong, 1-4 xa phu, 5 tap hoa, 6 hieu thuoc, 7 VLTN...)\n", nIdx, npc_Nums - 1);
		return FALSE;
	}
	POINT pt = theMap.NpcPoints[nIdx];
	DbgOut("NpcPoints[%d] (%s) = (%d, %d)\n", nIdx, s_szNpcPointNames[nIdx], pt.x, pt.y);
	if (!pt.x || !pt.y)
	{
		DbgOut("LOI: diem nay = 0 - muc [NPC] cua %s khong co toa do NPC nay (hoac chua nap)\n", PathFileShow(*g_pMapID));
		return FALSE;
	}
	return TRUE;
}

static KItem* GetItem(int nItemIdx)
{
	return (nItemIdx > 0) ? (KItem*)(dwItmBaseAdd + nItemIdx * ITM_DATA_SIZE) : NULL;
}

static void OutMenu()
{
	KUiMsgSel *pMsgSel = (KUiMsgSel*)GetPtrAddress(MSG_BASE_ADD);
	KUiMsgSel2 *pMsgSel2 = (KUiMsgSel2*)GetPtrAddress(MSG2_BASE_ADD);
	DbgOut("theMain.GetMenuCount() = %d\n", theMain.GetMenuCount());

	DbgOut("KUiMsgSel  (MSG_BASE_ADD)  = 0x%08X  IsVisible = %d\n", (DWORD)pMsgSel, (pMsgSel && pMsgSel->IsVisible()) ? 1 : 0);
	if (pMsgSel && pMsgSel->IsVisible())
	{
		DbgOut("  m_InfoText: %.200s\n", pMsgSel->m_InfoText);
		for (int i = 0; i < pMsgSel->m_nNumMessage && i < 16; i++)
		{
			KOneMsgInfo *pMsg = pMsgSel->m_pMessages ? pMsgSel->m_pMessages[i] : NULL;
			if (pMsg) DbgOut("  [%d] %.*s\n", i, (pMsg->nLen > 0 && pMsg->nLen < 256) ? pMsg->nLen : 0, pMsg->Msg);
		}
	}

	DbgOut("KUiMsgSel2 (MSG2_BASE_ADD) = 0x%08X  IsVisible = %d\n", (DWORD)pMsgSel2, (pMsgSel2 && pMsgSel2->IsVisible()) ? 1 : 0);
	if (pMsgSel2 && pMsgSel2->IsVisible())
	{
		if (pMsgSel2->GetQuestion()[0]) DbgOut("  Loi thoai: %.200s\n", pMsgSel2->GetQuestionText());
		for (int i = 0; i < pMsgSel2->m_nNumMessage && i < 16; i++)
		{
			KOneMsgInfo *pMsg = pMsgSel2->m_pMessages ? pMsgSel2->m_pMessages[i] : NULL;
			if (pMsg) DbgOut("  [%d] %.*s\n", i, (pMsg->nLen > 0 && pMsg->nLen < 256) ? pMsg->nLen : 0, pMsg->Msg);
		}
	}
	DbgOut("g_UiInformation2 IsVisible = %d\n", g_UiInformation2->IsVisible() ? 1 : 0);
}

// In chuoi TCVN3 cua game: bo ma mau (0x02 r g b, 0x03), giu xuong dong, ma dieu khien khac in <NN>
static void DbgOutGameText(const char *szLabel, const char *szText, int nMax)
{
	static char szBuf[DEBUG_TEXT_SIZE];
	int n = 0, nColor = 0;
	int nLen = GameTextLen(szText, nMax);
	for (int i = 0; i < nLen && n < (int)sizeof(szBuf) - 8; i++)
	{
		unsigned char c = (unsigned char)szText[i];
		if (c == 0x02)
		{
			nColor++;
			i += 3;	// 3 byte mau r g b, co the = 0
		}
		else if (c == 0x03 || c == '\r')
			;
		else if (c == '\n' || c >= 0x20)
			szBuf[n++] = (char)c;
		else
			n += sprintf(szBuf + n, "<%02X>", c);
	}
	szBuf[n] = 0;
	DbgOut("%s (%d byte, %d ma mau):\n%s\n", szLabel, nLen, nColor, szBuf);
}

static void DbgOutHex(const char *szText, int nBytes)
{
	DbgOut("  hex:");
	for (int i = 0; i < nBytes; i++) DbgOut(" %02X", (unsigned char)szText[i]);
	DbgOut("\n");
}

// Loi thoai cua NPC + cac bang thong bao dang mo (khong tac dong)
static void OutDialogText()
{
	KUiMsgSel2 *pMsgSel2 = (KUiMsgSel2*)GetPtrAddress(MSG2_BASE_ADD);
	KUiMsgSel *pMsgSel = (KUiMsgSel*)GetPtrAddress(MSG_BASE_ADD);
	BOOL bAny = FALSE;

	if (pMsgSel2 && pMsgSel2->IsVisible())
	{
		bAny = TRUE;
		DbgOut("=== KUiMsgSel2 (0x%08X) ===\n", (DWORD)pMsgSel2);
		DbgOutGameText("Loi thoai (this+0x2578)", pMsgSel2->GetQuestion(), sizeof(pMsgSel2->m_szQuestion));
		DbgOutHex(pMsgSel2->GetQuestion(), 32);
		for (int i = 0; i < pMsgSel2->m_nNumMessage && i < 16; i++)
		{
			KOneMsgInfo *pMsg = pMsgSel2->m_pMessages ? pMsgSel2->m_pMessages[i] : NULL;
			if (pMsg) DbgOut("  [%d] %.*s\n", i, (pMsg->nLen > 0 && pMsg->nLen < 256) ? pMsg->nLen : 0, pMsg->Msg);
		}
	}
	if (pMsgSel && pMsgSel->IsVisible())
	{
		bAny = TRUE;
		DbgOut("=== KUiMsgSel (0x%08X) ===\n", (DWORD)pMsgSel);
		DbgOutGameText("m_InfoText", pMsgSel->m_InfoText, sizeof(pMsgSel->m_InfoText));
		for (int i = 0; i < pMsgSel->m_nNumMessage && i < 16; i++)
		{
			KOneMsgInfo *pMsg = pMsgSel->m_pMessages ? pMsgSel->m_pMessages[i] : NULL;
			if (pMsg) DbgOut("  [%d] %.*s\n", i, (pMsg->nLen > 0 && pMsg->nLen < 256) ? pMsg->nLen : 0, pMsg->Msg);
		}
	}
	if (g_UiInformation && g_UiInformation->IsVisible())
	{
		bAny = TRUE;
		DbgOutGameText("=== KUiInformation (bang thong bao 2 nut) ===", g_UiInformation->m_szInfoText, sizeof(g_UiInformation->m_szInfoText));
	}
	if (g_UiInformation2 && g_UiInformation2->IsVisible())
	{
		bAny = TRUE;
		DbgOutGameText("=== KUiInformation2 (bang thong bao nut OK) ===", g_UiInformation2->m_szInfoText, sizeof(g_UiInformation2->m_szInfoText));
	}
	if (!bAny)
		DbgOut("Khong co hop thoai / bang thong bao nao dang mo (bam vao NPC truoc)\n");
	g_DbgReply.bOK = TRUE;
}

static void MapStatus()
{
	if (!DbgCheckPlayer()) return;
	KNpc *pPlayer = P_PLAYER;
	int nOwnerCity = theMain.GetOwnerCity(*g_pMapID);

	DbgOut("MapID = %d (%s)  GetOwnerCity = %d  m_FightMode = %d  m_nEntrustMode = %d\n",
		*g_pMapID, g_szMapName, nOwnerCity, pPlayer->m_FightMode, g_pCurPlayer->m_nEntrustMode);
	DbgOut("m_bAutoReturn = %d  m_nTrainMapID = %d  m_nSaveMapID = %d  m_nDriverOption = %d  m_nDriverIndex = %d\n",
		theMap.m_bAutoReturn, theMap.m_nTrainMapID, theMap.m_nSaveMapID, theMap.m_nDriverOption, theMap.m_nDriverIndex);
	DbgOut("m_nLastMapID = %d  m_szLastMapName = %s  m_nPoints = %d  m_nCurPoint = %d\n",
		theMap.m_nLastMapID, theMap.m_szLastMapName, theMap.m_nPoints, theMap.m_nCurPoint);
	DbgOut("m_bDoShoping = %d  m_nTimeWait = %d  m_bFinished = %d  m_nDriverStep = %d  m_bIsReady = %d  m_bRoomLocked = %d\n",
		theMap.m_bDoShoping, theMap.m_nTimeWait, theMap.m_bFinished, theMap.m_nDriverStep, theMap.m_bIsReady, theMap.m_bRoomLocked);
	DbgOut("Need: Money=%d GiveKCL=%d FirstRepos=%d Sell=%d Save=%d Repair=%d Groceries=%d Pharmacy=%d MingWang=%d FuYuan=%d Driver=%d\n",
		theMap.m_bNeedMoney, theMap.m_bNeedGiveKCL, theMap.m_bFirstSaveRepos, theMap.m_bNeedSellItems, theMap.m_bNeedSaveItems,
		theMap.m_bNeedRepairItems, theMap.m_bNeedGroceries, theMap.m_bNeedPharmacy, theMap.m_bNeedMingWang, theMap.m_bNeedFuYuan,
		theMap.m_bNeedDriver);
	DbgOut("Tuy chon: Repair=%d BuyTDP=%d(%d) SellItems=%d PutMoney=%d(%d) GetMoney=%d SaveItemMode=%d\n",
		theMap.m_bRepair, theMap.m_bBuyTDP, theMap.m_nAMNumbers, theMap.m_bSellItems, theMap.m_bPutMoney, theMap.m_nMoneyAmount,
		theMap.m_nGetMoneyAmount, theMap.m_nSaveItemMode);

	// Cung thu tu voi CVLMap::Breathe
	DbgOut("\n--- CVLMap::Breathe ---\n");
	const char *szStep = NULL;
	if (g_NetworkOffline) szStep = "DUNG: g_NetworkOffline != 0";
	else if (pPlayer->m_FightMode) szStep = "Dang o ngoai thanh (m_FightMode = 1): Breathe chi tu chon xa phu khi vua ve; viec ve thanh do Hoi phuc (Tho dia phu)";
	else if (g_SysPause) szStep = "DUNG: g_SysPause";
	else if (theMain.m_bPause) szStep = "DUNG: theMain.m_bPause";
	else if (PathTravelActive()) szStep = "DUNG: dang tu di theo MapPath (xem dong MapPath ben duoi)";
	else if (theMove.IsFlagMoving()) szStep = "DUNG: dang chay theo cam co (FLAG_X/Y > 0)";
	else if (theMap.m_nPoints <= 0) szStep = "DUNG: m_nPoints = 0 - khong co muc [NPC] trong Maps\\Path\\<MapID>_<Ten>.txt";
	else if (theUtil.m_bDoingUtil) szStep = "DUNG: theUtil.m_bDoingUtil (dang chay tien ich)";
	else if (theMap.m_bNeedMoney) szStep = "WithdrawMoney (rut tien)";
	else if (theQuest.m_bAutoDoQuest && !theQuest.m_bNeedGoTrain) szStep = "theQuest.Breath (dang lam nhiem vu)";
	else if (!theMap.m_bAutoReturn) szStep = "DUNG: chua bat 'Ve thanh tu quay lai' (Ctrl+G)";
	else if (g_pCurPlayer->m_nEntrustMode) szStep = "DUNG: m_nEntrustMode != 0 (dang uy thac)";
	else if (theMap.m_bNeedGiveKCL) szStep = "Giao nop Khieu chien lenh";
	else if (*g_pMapID != nOwnerCity) szStep = "ChangeMap -> thanh chinh (chay toi xa phu, chon thanh)";
	else if (theMap.m_bFirstSaveRepos) szStep = "FirstRepository (mo ruong lan dau)";
	else if (theMap.m_bNeedSellItems) szStep = "SellItems";
	else if (theMap.m_bNeedSaveItems) szStep = "RunningToPos(Ruong) -> GetSaveItems";
	else if (theMap.m_bNeedGroceries) szStep = "RunningToPos(Tap hoa) -> GetGroceries";
	else if (theMap.m_bNeedPharmacy) szStep = "RunningToPos(Hieu thuoc) -> GetPharmacy";
	else if (theMap.m_bNeedMingWang) szStep = "RunningToPos(VLTN) -> GetReputation(1)";
	else if (!theMap.m_bIsReady) szStep = "CheckReady (xem nut 'CheckReady')";
	else if (theMap.m_bNeedDriver)
		szStep = (theMap.m_nDriverOption == 5) ? "UseThanHanhPhu" : "RunningToPos(Xa phu) -> GetDriver";
	else szStep = "Khong con viec gi";
	DbgOut("=> Buoc tiep theo: %s\n", szStep);
	extern DWORD g_dwNoRouteTick;
	if (g_dwNoRouteTick && GetTickCount() - g_dwNoRouteTick < 30000)
		DbgOut("GetDriver: xa phu khong co map train, khong co lo trinh MapPath - cho %ds roi noi lai\n",
			(30000 - (GetTickCount() - g_dwNoRouteTick)) / 1000);
	int nTarget = theMap.m_nTrainMapID ? theMap.m_nTrainMapID : (theMove.m_bRouteMove && theMove.m_Nodes[0].x) ? theMove.m_nNodesMapID : 0;
	DbgOut("Map can toi bang xa phu = %d (%s)\n", nTarget, theMap.m_nTrainMapID ? "Di theo MAP" : nTarget ? "map cua danh sach Theo toa do" : "khong biet - xa phu chon dong theo tuy chon");
	char szTv[256];
	PathTravelInfo(szTv, sizeof(szTv));
	DbgOut("MapPath tu di: %s\n", szTv);
	g_DbgReply.bOK = TRUE;
}

static void CityMap()
{
	if (!DbgCheckPlayer()) return;
	BOOL bLoaded = theMap.LoadMap(*g_pMapID);
	DbgOut("theMap.LoadMap(%d): muc [NPC] cua %s = %s, %d NPC co toa do\n", *g_pMapID, PathFileShow(*g_pMapID),
		bLoaded ? "OK" : "KHONG CO (chua co file / muc [NPC] trong)", theMap.m_nPoints);
	if (!bLoaded) return;

	int nPlayerX, nPlayerY;
	P_PLAYER->GetMapPos(&nPlayerX, &nPlayerY);
	for (int i = 0; i < npc_Nums; i++)
	{
		POINT pt = theMap.NpcPoints[i];
		if (!pt.x && !pt.y) continue;
		DbgOut("  [%2d] %-15s (%6d, %6d)  dZ = %d\n", i, s_szNpcPointNames[i], pt.x, pt.y,
			__distance(nPlayerX, nPlayerY, pt.x, pt.y));
	}
	theMap.NearestDriver();
	DbgOut("NearestDriver() => m_nDriverIndex = %d\n", theMap.m_nDriverIndex);
	g_DbgReply.bOK = TRUE;
}

static void CityInfo()
{
	char szName[64] = "";
	int nMapID = *g_pMapID;
	int nOwner = theMain.GetOwnerCity(nMapID);
	theMain.GetCityName(nOwner, szName);
	DbgOut("MapID %d: GetOwnerCity = %d (%s)  GetSubCity = %d\n", nMapID, nOwner, szName, theMain.GetSubCity(nMapID));
	theMain.GetMapName(nMapID, szName);
	DbgOut("GetMapName(%d) = %s  (g_szMapName = %s)\n", nMapID, szName, g_szMapName);
	if (theMap.m_nTrainMapID)
	{
		theMain.GetMapName(theMap.m_nTrainMapID, szName);
		int nReturn = theMain.GetReturnCity(theMap.m_nTrainMapID);
		DbgOut("Noi luyen cong m_nTrainMapID = %d (%s)  GetReturnCity = %d", theMap.m_nTrainMapID, szName, nReturn);
		theMain.GetCityName(nReturn, szName);
		DbgOut(" (%s)\n", szName);
	}
	else
		DbgOut("Chua chon noi luyen cong (m_nTrainMapID = 0)\n");
	g_DbgReply.bOK = TRUE;
}

static void NpcAtPoint(int nIdx)
{
	if (!DbgCheckPlayer() || !CheckNpcPointIdx(nIdx)) return;
	POINT pt = theMap.NpcPoints[nIdx];
	int nPlayerX, nPlayerY, nNpcX, nNpcY, nFound = 0;
	P_PLAYER->GetMapPos(&nPlayerX, &nPlayerY);
	DbgOut("Khoang cach Player -> diem: %d\n", __distance(nPlayerX, nPlayerY, pt.x, pt.y));
	for (int i = 1; i < MAX_NPC; i++)
	{
		KNpc *pNpc = g_pNpcArray[i];
		if (!pNpc->IsExist()) continue;
		pNpc->GetMapPos(&nNpcX, &nNpcY);
		int dZ = __distance(pt.x, pt.y, nNpcX, nNpcY);
		if (dZ > 200) continue;
		BOOL bMatch = (pNpc->m_Kind == kind_dialoger && dZ <= 40);
		if (bMatch) nFound++;
		DbgOut("  [%d] %s  Kind = %d  cach diem = %d  cach Player = %d%s\n", i, pNpc->Name, pNpc->m_Kind, dZ,
			__distance(nPlayerX, nPlayerY, nNpcX, nNpcY), bMatch ? "  <== ClickToNpc chon" : "");
	}
	if (!nFound)
		DbgOut("=> Khong co NPC dialoger trong 40 quanh diem: ClickToNpc se khong lam gi (sai toa do trong muc [NPC] cua %s, hoac NPC chua hien)\n", PathFileShow(*g_pMapID));
	g_DbgReply.nResult[0] = nFound;
	g_DbgReply.bOK = TRUE;
}

static void ClickNpc(int nIdx)
{
	if (!DbgCheckPlayer() || !CheckNpcPointIdx(nIdx)) return;
	DbgOut("PEOPLE_IDX truoc = %d  m_nTimeWait = %d -> 0\n", PEOPLE_IDX, theMap.m_nTimeWait);
	theMap.m_nTimeWait = 0;
	theMap.ClickToNpc(theMap.NpcPoints[nIdx].x, theMap.NpcPoints[nIdx].y);
	DbgOutPlayerPos("Sau");
	DbgOut("PEOPLE_IDX sau = %d  m_nTimeWait = %d (>0: da chon NPC, game tu chay toi & mo hoi thoai)\n", PEOPLE_IDX, theMap.m_nTimeWait);
	g_DbgReply.bOK = TRUE;
}

static void RunToPos(int nIdx)
{
	if (!DbgCheckPlayer() || !CheckNpcPointIdx(nIdx)) return;
	DbgOut("m_nCurPoint truoc = %d\n", theMap.m_nCurPoint);
	BOOL bRet = theMap.RunningToPos(theMap.NpcPoints[nIdx]);
	DbgOut("theMap.RunningToPos = %d (%s)  m_nCurPoint = %d\n", bRet, bRet ? "dang chay theo MapPath / chay thang" : "da toi gan NPC", theMap.m_nCurPoint);
	if (bRet) DbgOut("(bam tiep de chay tung buoc; bat 'Ve duong di' de xem duong)\n");
	DbgOutPlayerPos("Player");
	g_DbgReply.bOK = TRUE;
}

static void ShopList()
{
	KUiShop *pShop = (KUiShop*)GetPtrAddress(SHOP_BASE_ADD);
	DbgOut("KUiShop (SHOP_BASE_ADD) = 0x%08X\n", (DWORD)pShop);
	if (!pShop)
	{
		DbgOut("Chua mo cua hang (NULL)\n");
		g_DbgReply.bOK = TRUE;
		return;
	}
	DbgOut("IsVisible = %d  m_nObjCount = %d  m_nCurShop = %d\n", pShop->IsVisible() ? 1 : 0, pShop->m_nObjCount, g_pCurPlayer->m_nCurShop);
	for (int i = 0; i < pShop->m_nObjCount && i < 40; i++)
	{
		char szItemDesc[512] = "";
		KItemObject *pObj = &pShop->m_pObjectList[i];
		g_pCoreShell->GetGameData(GDI_TRADE_ITEM_PRICE, (int)pObj, (int)szItemDesc);
		for (char *p = szItemDesc; *p; p++) if (*p == '\n' || *p == '\r') *p = ' ';
		DbgOut("  [%d] uId = %d  %.100s\n", i, pObj->uId, szItemDesc);
	}
	DbgOut("(BuyItem dung uId, Tap hoa: TDP la uId 0)\n");
	g_DbgReply.bOK = TRUE;
}

static void Inventory()
{
	if (!DbgCheckPlayer()) return;
	DbgOut("EQUIP_MONEY = %d  ROOM_MONEY = %d  CheckRoom() = %d/%d  PLAY_HAND_IDX = %d\n",
		EQUIP_MONEY, ROOM_MONEY, g_pCurPlayer->CheckRoom(), MAX_ROOM_SPACE, PLAY_HAND_IDX);
	DbgOut("TDP = %d  HTP = %d\n", g_pCurPlayer->GetItemsCount(AMULET_PAPER), g_pCurPlayer->GetItemsCount(AMULETX_PAPER));
	DbgOut("Idx   | X,Y  | Genre/Detail/Part | So luong | Ten\n");
	int nCount = 0;
	for (int i = 0; i < MAX_PLAYER_ITEM; i++)
	{
		if (PLAYER_ITEM(i).nPlace != pos_equiproom || !PLAYER_ITEM(i).nIdx) continue;
		KItem *pItem = GetItem(PLAYER_ITEM(i).nIdx);
		nCount++;
		DbgOut("%-5d | %d,%d  | %d/%d/%d | %s%d | %s%s\n", PLAYER_ITEM(i).nIdx, PLAYER_ITEM(i).nX, PLAYER_ITEM(i).nY,
			ComAttr.nItemGenre, ComAttr.nDetailType, ComAttr.nParticularType,
			ComAttr.bStack ? "x" : "", ComAttr.bStack ? pItem->m_nCurDur : 1, ComAttr.szItemName,
			theMap.IsInSellList(ComAttr.szItemName) ? "  [SellItems.txt]" : "");
	}
	DbgOut("Tong: %d vat pham trong hanh trang\n", nCount);
	g_DbgReply.nResult[0] = nCount;
	g_DbgReply.bOK = TRUE;
}

static void EquipDur()
{
	if (!DbgCheckPlayer()) return;
	for (int i = 0; i < MAX_EQUIP_SLOT; i++)
	{
		int nItemIdx = g_pCurPlayer->m_ItemList.GetEquip(i);
		KItem *pItem = GetItem(nItemIdx);
		if (!pItem) continue;
		const char *szNote = "";
		if (pItem->m_nCurDur < 0) szNote = "khong co do ben";
		else if (pItem->m_nItemPlus > 5) szNote = "+>5 khong sua";
		else if (pItem->m_nCurDur < 3) szNote = "SAP HONG (CheckReady chan)";
		else if (pItem->m_nCurDur < pItem->m_nMaxDur) szNote = "can sua";
		DbgOut("  part %2d  idx %-5d  %3d/%-3d  +%d  %s  %s\n", i, nItemIdx, pItem->m_nCurDur, pItem->m_nMaxDur,
			pItem->m_nItemPlus, ComAttr.szItemName, szNote);
	}
	g_DbgReply.bOK = TRUE;
}

static void StoreBoxInfo()
{
	if (!DbgCheckPlayer()) return;
	KWndImage *pStoreBox = (KWndImage*)GetPtrAddress(SBO_BASE_ADD);
	DbgOut("KUiStoreBox (SBO_BASE_ADD) = 0x%08X  IsVisible = %d\n", (DWORD)pStoreBox, (pStoreBox && pStoreBox->IsVisible()) ? 1 : 0);
	DbgOut("P_PLAYER->m_nObjectIdx = %d  ROOM_MONEY = %d\n", P_PLAYER->m_nObjectIdx, ROOM_MONEY);

	int nPlayerX, nPlayerY, nX, nY;
	P_PLAYER->GetMapPos(&nPlayerX, &nPlayerY);
	DbgOut("MapID %d %s city_visondao(%d) => tim ruong theo %s\n", *g_pMapID, (*g_pMapID >= city_visondao) ? ">=" : "<",
		city_visondao, (*g_pMapID >= city_visondao) ? "NPC 'Ruong chua do'" : "KObj Obj_Kind_StoreBox");
	for (int i = 1; i < MAX_OBJ; i++)
	{
		if (g_pObjArray[i]->m_nID == -1 || g_pObjArray[i]->m_Kind != Obj_Kind_StoreBox) continue;
		g_pObjArray[i]->GetMapPos(&nX, &nY);
		DbgOut("  KObj[%d] StoreBox (%d, %d)  dZ = %d (mo khi <= %d)\n", i, nX, nY, __distance(nPlayerX, nPlayerY, nX, nY), PLAYER_PICKUP_DISTANCE);
	}
	for (int i = 1; i < MAX_NPC; i++)
	{
		if (!g_pNpcArray[i]->IsExist() || strcmp(g_pNpcArray[i]->Name, STOREBOX_NPC_NAME)) continue;
		g_pNpcArray[i]->GetMapPos(&nX, &nY);
		DbgOut("  NPC[%d] %s (%d, %d)  dZ = %d  Kind = %d\n", i, g_pNpcArray[i]->Name, nX, nY,
			__distance(nPlayerX, nPlayerY, nX, nY), g_pNpcArray[i]->m_Kind);
	}
	POINT pt = theMap.NpcPoints[npc_Repository];
	DbgOut("NpcPoints[Ruong] = (%d, %d) trong muc [NPC] cua %s\n", pt.x, pt.y, PathFileShow(*g_pMapID));
	g_DbgReply.bOK = TRUE;
}

static void HandToRoom()
{
	if (!DbgCheckPlayer()) return;
	KItem *pItem = GetItem(PLAY_HAND_IDX);
	if (!pItem)
	{
		DbgOut("Khong co do tren tay (PLAY_HAND_IDX = 0)\n");
		g_DbgReply.bOK = TRUE;
		return;
	}
	POINT itmPos;
	DbgOut("Tren tay: idx %d  %s  (%dx%d)\n", PLAY_HAND_IDX, ComAttr.szItemName, ComAttr.nWidth, ComAttr.nHeight);
	if (!EQUIP_ROOM.FindRoom(ComAttr.nWidth, ComAttr.nHeight, &itmPos))
	{
		DbgOut("EQUIP_ROOM.FindRoom: het cho\n");
		return;
	}
	DbgOut("EQUIP_ROOM.FindRoom = (%d, %d) => SwitchItem(UOC_ITEM_TAKE_WITH, %d, %d)\n", itmPos.x, itmPos.y, itmPos.x, itmPos.y);
	g_pCurPlayer->SwitchItem(UOC_ITEM_TAKE_WITH, itmPos.x, itmPos.y);
	g_DbgReply.bOK = TRUE;
}

static void CheckReadyInfo()
{
	if (!DbgCheckPlayer()) return;
	KUiPlayerBar *pUiPlayerBar = (KUiPlayerBar*)GetPtrAddress(UPB_BASE_ADD);
	DbgOut("KUiPlayerBar (UPB_BASE_ADD) = 0x%08X%s\n", (DWORD)pUiPlayerBar, pUiPlayerBar ? "" : "  => NULL: CheckReady luon TRUE");
	if (!pUiPlayerBar)
	{
		g_DbgReply.bOK = TRUE;
		return;
	}
	BOOL bReady = TRUE;
	if (theMap.m_bPutMoney && EQUIP_MONEY > theMap.m_nMoneyAmount)
	{
		DbgOut("x Gui tien: EQUIP_MONEY %d > %d => OperationRequest(GOI_MONEY_INOUT_STORE_BOX)\n", EQUIP_MONEY, theMap.m_nMoneyAmount);
		bReady = FALSE;
	}
	for (int i = 0; i < MAX_EQUIP_SLOT; i++)
	{
		KItem *pItem = GetItem(g_pCurPlayer->m_ItemList.GetEquip(i));
		if (pItem && pItem->m_nCurDur >= 0 && pItem->m_nCurDur < 3)
		{
			DbgOut("x Trang bi sap hong: %s (%d)\n", ComAttr.szItemName, pItem->m_nCurDur);
			bReady = FALSE;
		}
	}
	if (P_PLAYER->m_PoisonState.nTime)
	{
		DbgOut("x Dang trung doc (m_PoisonState.nTime = %d)\n", P_PLAYER->m_PoisonState.nTime);
		bReady = FALSE;
	}
	int nAmulet = g_pCurPlayer->GetItemsCount(AMULET_PAPER) + g_pCurPlayer->GetItemsCount(AMULETX_PAPER);
	BOOL bHaveTP = theMain.HaveTownPortal();
	DbgOut("%s HaveTownPortal = %d (hanh trang TDP/HTP = %d, phim tat 3 m_nHK3Gengre = %d)\n", bHaveTP ? "v" : "x",
		bHaveTP, nAmulet, pUiPlayerBar->m_nHK3Gengre);
	if (!bHaveTP) bReady = FALSE;
	DbgOut("  Mau %d (tu an < %d: %d)  Mana %d (tu an < %d: %d)\n", P_PLAYER->m_CurrentLife, theRecover.m_nAutoHPAmount,
		theRecover.m_bAutoHP, P_PLAYER->m_CurrentMana, theRecover.m_nAutoMPAmount, theRecover.m_bAutoMP);
	DbgOut("=> CheckReady %s\n", bReady ? "SAN SANG (sau 1 luot an thuoc)" : "CHUA san sang");
	g_DbgReply.nResult[0] = bReady;
	g_DbgReply.bOK = TRUE;
}

BOOL DebugMapCmd(const DEBUG_PARAM *pParam)
{
	const int *P = pParam->nParam;
	const char *szText = pParam->szParam;
	switch (pParam->nCmd)
	{
	case dbg_mp_status:
		MapStatus();
		break;
	case dbg_mp_citymap:
		CityMap();
		break;
	case dbg_mp_cityinfo:
		CityInfo();
		break;
	case dbg_mp_npcatpoint:
		NpcAtPoint(P[2]);
		break;
	case dbg_mp_clicknpc:
		ClickNpc(P[2]);
		break;
	case dbg_mp_runtopos:
		RunToPos(P[2]);
		break;
	case dbg_mp_menu:
		OutMenu();
		g_DbgReply.bOK = TRUE;
		break;
	case dbg_mp_dialog:
		OutDialogText();
		break;
	case dbg_mp_clickmenuidx:
		g_DbgReply.nResult[0] = theMain.ClickMenuIdx(P[2]);
		DbgOut("theMain.ClickMenuIdx(%d) = %d%s\n", P[2], g_DbgReply.nResult[0], g_DbgReply.nResult[0] ? "" : "  (khong co menu nao dang mo)");
		g_DbgReply.bOK = TRUE;
		break;
	case dbg_mp_clickmenustr:
		if (!szText[0])
		{
			DbgOut("LOI: chua nhap o 'Chuoi'\n");
			break;
		}
		g_DbgReply.nResult[0] = theMain.ClickMenuStr(szText);
		DbgOut("theMain.ClickMenuStr(\"%s\") = %d%s\n", szText, g_DbgReply.nResult[0], g_DbgReply.nResult[0] ? "" : "  (khong thay dong nao chua chuoi nay)");
		g_DbgReply.bOK = TRUE;
		break;
	case dbg_mp_closeall:
		CloseAllWindows(0);
		DbgOut("CloseAllWindows(0)  CLA_FUNC_ADD 0x%08X\n", CLA_FUNC_ADD);
		OutMenu();
		g_DbgReply.bOK = TRUE;
		break;
	case dbg_mp_shop:
		ShopList();
		break;
	case dbg_mp_buyitem:
		if (!DbgCheckPlayer()) break;
		if (!GetPtrAddress(SHOP_BASE_ADD))
		{
			DbgOut("LOI: chua mo cua hang\n");
			break;
		}
		DbgOut("Tien truoc = %d\n", EQUIP_MONEY);
		g_pCurPlayer->BuyItem(P[2]);
		DbgOut("g_pCurPlayer->BuyItem(%d) da gui (OperationRequest GOI_TRADE_NPC_BUY). Bam 'Hanh trang' de xem ket qua\n", P[2]);
		g_DbgReply.bOK = TRUE;
		break;
	case dbg_mp_inventory:
		Inventory();
		break;
	case dbg_mp_itemcount:
		if (!DbgCheckPlayer()) break;
		g_DbgReply.nResult[0] = g_pCurPlayer->GetItemsCount(szText[0] ? szText : AMULET_PAPER);
		DbgOut("g_pCurPlayer->GetItemsCount(\"%s\") = %d\n", szText[0] ? szText : AMULET_PAPER, g_DbgReply.nResult[0]);
		g_DbgReply.bOK = TRUE;
		break;
	case dbg_mp_sellitem:
		{
			if (!DbgCheckPlayer()) break;
			KItem *pItem = GetItem(P[2]);
			if (!pItem)
			{
				DbgOut("LOI: Idx = %d khong hop le (lay Idx o nut 'Hanh trang')\n", P[2]);
				break;
			}
			DbgOut("SellItem(%d) %s  tien truoc = %d\n", P[2], ComAttr.szItemName, EQUIP_MONEY);
			g_pCurPlayer->SellItem(P[2]);
			g_DbgReply.bOK = TRUE;
		}
		break;
	case dbg_mp_useitem:
		if (!DbgCheckPlayer()) break;
		if (!szText[0])
		{
			DbgOut("LOI: chua nhap ten vat pham o 'Chuoi'\n");
			break;
		}
		g_DbgReply.nResult[0] = g_pCurPlayer->UseItem(szText);
		DbgOut("g_pCurPlayer->UseItem(\"%s\") = %d%s\n", szText, g_DbgReply.nResult[0], g_DbgReply.nResult[0] ? "" : "  (khong tim thay trong hanh trang)");
		g_DbgReply.bOK = TRUE;
		break;
	case dbg_mp_eatmedicine:
		if (!DbgCheckPlayer()) break;
		DbgOut("Mau %d  Mana %d\n", P_PLAYER->m_CurrentLife, P_PLAYER->m_CurrentMana);
		g_DbgReply.nResult[0] = g_pCurPlayer->EatMedicine(P[2]);
		DbgOut("g_pCurPlayer->EatMedicine(%d) = %d  (0 mau, 1 mana, 2 ca hai, 3 the luc, 4 giai doc)\n", P[2], g_DbgReply.nResult[0]);
		g_DbgReply.bOK = TRUE;
		break;
	case dbg_mp_equipdur:
		EquipDur();
		break;
	case dbg_mp_repairitem:
		{
			if (!DbgCheckPlayer()) break;
			KItem *pItem = GetItem(P[2]);
			if (!pItem)
			{
				DbgOut("LOI: Idx = %d khong hop le (lay idx o nut 'Do ben trang bi')\n", P[2]);
				break;
			}
			DbgOut("RepairItem(%d) %s  %d/%d  tien truoc = %d\n", P[2], ComAttr.szItemName, pItem->m_nCurDur, pItem->m_nMaxDur, EQUIP_MONEY);
			g_pCurPlayer->RepairItem(P[2]);
			g_DbgReply.bOK = TRUE;
		}
		break;
	case dbg_mp_decompose:
		{
			if (!DbgCheckPlayer()) break;
			KItem *pItem = GetItem(P[2]);
			if (!pItem)
			{
				DbgOut("LOI: Idx = %d khong hop le (lay Idx o nut 'Hanh trang')\n", P[2]);
				break;
			}
			DbgOut("DecomposeItem(%d) %s  Genre/Detail/Part = %d/%d/%d  CheckRoom() truoc = %d\n", P[2], ComAttr.szItemName,
				ComAttr.nItemGenre, ComAttr.nDetailType, ComAttr.nParticularType, g_pCurPlayer->CheckRoom());
			g_DbgReply.nResult[0] = g_pCurPlayer->DecomposeItem(P[2]);
			DbgOut("g_pCurPlayer->DecomposeItem(%d) = %d%s\n", P[2], g_DbgReply.nResult[0],
				g_DbgReply.nResult[0] ? "  (da gui 0x9B, bam 'Hanh trang' de xem Manh Trang Bi)" : "  (item khong nam trong hanh trang)");
			g_DbgReply.bOK = TRUE;
		}
		break;
	case dbg_mp_storebox:
		StoreBoxInfo();
		break;
	case dbg_mp_openstore:
		if (!DbgCheckPlayer()) break;
		theMap.m_nTimeWait = 0;
		g_DbgReply.nResult[0] = theMap.OpenStoreBox();
		DbgOut("theMap.OpenStoreBox() = %d  (1 da mo, 0 dang mo/chay toi, -1 khong thay ruong)  m_nObjectIdx = %d  PEOPLE_IDX = %d\n",
			g_DbgReply.nResult[0], P_PLAYER->m_nObjectIdx, PEOPLE_IDX);
		g_DbgReply.bOK = TRUE;
		break;
	case dbg_mp_lockinfo:
		DbgOut("g_pCoreShell = 0x%08X  PGS_FUNC_ADD 0x%08X\n", (DWORD)g_pCoreShell, PGS_FUNC_ADD);
		DbgOut("ProcessGameSecurity(1) = %d (co chuc nang khoa)\n", g_pCoreShell->ProcessGameSecurity(1, 0, 0));
		DbgOut("ProcessGameSecurity(0) = %d (dang khoa)\n", g_pCoreShell->ProcessGameSecurity(0, 0, 0));
		DbgOut("Mat khau ruong da cai: %s\n", theMap.m_nRepositoryPass ? "co" : "KHONG (OpenLockRoom bo qua)");
		g_DbgReply.bOK = TRUE;
		break;
	case dbg_mp_unlock:
		g_DbgReply.nResult[0] = theMain.OpenLockRoom(theMap.m_nRepositoryPass);
		DbgOut("theMain.OpenLockRoom(mat khau da cai) = %d (1 = dang khoa, da gui lenh mo)\n", g_DbgReply.nResult[0]);
		g_DbgReply.bOK = TRUE;
		break;
	case dbg_mp_lock:
		theMain.CloseLockRoom();
		DbgOut("theMain.CloseLockRoom()  => ProcessGameSecurity(0) = %d\n", g_pCoreShell->ProcessGameSecurity(0, 0, 0));
		g_DbgReply.bOK = TRUE;
		break;
	case dbg_mp_moneyin:
	case dbg_mp_moneyout:
		{
			if (!DbgCheckPlayer()) break;
			BOOL bIn = (pParam->nCmd == dbg_mp_moneyin);
			if (P[2] <= 0)
			{
				DbgOut("LOI: nhap so luong o 'Idx/So'\n");
				break;
			}
			DbgOut("Truoc: tren nguoi %d  trong ruong %d\n", EQUIP_MONEY, ROOM_MONEY);
			g_DbgReply.nResult[0] = g_pCoreShell->OperationRequest(GOI_MONEY_INOUT_STORE_BOX, bIn, P[2]);
			DbgOut("OperationRequest(GOI_MONEY_INOUT_STORE_BOX, %d, %d) = %d  (can mo ruong truoc)\n", bIn, P[2], g_DbgReply.nResult[0]);
			g_DbgReply.bOK = TRUE;
		}
		break;
	case dbg_mp_filelog:
		DebugFileLogToggle(P[0]);
		break;
	case dbg_mp_filelogview:
		DebugFileLogView(P[0], szText);
		break;
	case dbg_mp_tongmoney:
		{
			if (!DbgCheckPlayer()) break;
			if (P[2] <= 0 || P[2] > EQUIP_MONEY)
			{
				DbgOut("LOI: nhap so luong o 'Idx/So' (1..%d = tien tren nguoi)\n", EQUIP_MONEY);
				break;
			}
			// Nhu nut Gop quy (10-10: KUiTong 0x4EB060 -> 0x4EB430(2, 0, nMoney)).
			// 15-10: nhanh 4 cua TongOperation 0x5C5300 (bang 0x5C5A80, lenh con 0..12), lenh con 2 ->
			// 0x5BDDA0(this 0x17A4F00, [m_cTong+0x28], TongNameID, __int64 nMoney) (10-10: 0x5BC630, this 0x22100A0).
			// X != 0: thu lenh con khac. p/buf = NULL thi game bo qua
			int  nSub = P[0] ? P[0] : 2;
			int  p[13] = { nSub, 0, P[2] };
			char buf[0x138] = { 0 };
			DWORD dwPlr = *(DWORD*)PLR_BASE_ADD;
			DbgOut("Truoc: tren nguoi %d  [Player[1].m_cTong+0x28] = %d\n", EQUIP_MONEY, dwPlr ? *(int*)(dwPlr + 0x12714) : -1);	// 15-10: PLAYER_OFFSET + m_cTong 0x7E74 + 0x28 (10-10: 0x1259C)
			g_DbgReply.nResult[0] = g_pCoreShell->TongOperation(4, (int)p, (int)buf);
			DbgOut("TongOperation(4, {%d, 0, %d}, buf) = %d  (bam lai de xem tien tren nguoi da giam chua)\n", nSub, P[2], g_DbgReply.nResult[0]);
			g_DbgReply.bOK = TRUE;
		}
		break;
	case dbg_mp_handtoroom:
		HandToRoom();
		break;
	case dbg_mp_checkready:
		CheckReadyInfo();
		break;
	case dbg_mp_showwalk:
	{
		int nOn = PathShowWalk(-1);
		if (nOn < 0)
		{
			DbgOut("LOI: khong chen duoc ham ve (sai ban game?)\n");
			break;
		}
		DbgOut("Ve duong di trong game: %s\n", nOn ? "BAT" : "TAT");
		if (nOn)
			DbgOut("Xam = canh MapPath, xanh duong = duong MapPath con lai (so = diem), xanh than = da di,\n"
				"trang = toi diem dang nham, do (dau X) = lenh MoveTo gan nhat + nguon,\n"
				"thoi vang = NPC trong thanh dang chay toi, tim = tu di qua map, xanh la = danh sach Theo toa do (TD1 = diem dau)\n");
		g_DbgReply.nResult[0] = nOn;
		g_DbgReply.bOK = TRUE;
		break;
	}
	case dbg_mp_script:
		if (!szText[0])
		{
			DbgOut("LOI: chua nhap script o 'Chuoi', vd: Switch('PK',0)\n");
			break;
		}
		g_DbgReply.nResult[0] = ExecuteScript(szText);
		DbgOut("ExecuteScript(\"%s\") = %d\n", szText, g_DbgReply.nResult[0]);
		g_DbgReply.bOK = TRUE;
		break;
	case dbg_mp_saverepo:
		if (!DbgCheckPlayer()) break;
		theMap.SaveNewRepository();
		DbgOut("theMap.SaveNewRepository(): NpcPoints[Ruong] = (%d, %d) - xem thong bao trong game\n",
			theMap.NpcPoints[npc_Repository].x, theMap.NpcPoints[npc_Repository].y);
		g_DbgReply.bOK = TRUE;
		break;
	default:
		return FALSE;
	}
	return TRUE;
}
