#include "stdafx.h"
#include "../GameDef.h"
#include "GameBase.h"
#include "JxCore.h"
#include "S3Client.h"
#include "VLClasses.h"
#include "VLDebug.h"
#include "MagicAttrib.h"

// Tab Nhat do cua cua so DEBUG: cac ham game dung trong VLCollect.cpp (nhat Obj, loc, sap xep hanh trang)

extern KBadObj	g_BadObjs;

#define CNHK_NAME	TV(L"Cẩm nang hoàng kim")	// "Cam nang hoang kim" (TCVN3) - giong VLCollect.cpp

static const char* s_szKeepCause[] =
{
	"keep_nothing (VUT/BAN)", "not_equip", "locked_equip", "golden_equip", "violet_equip", "special_equip",
	"save_jewellery", "save_ring", "save_lace", "mask_equip", "good_horse", "red_item (RedItems.txt)",
	"high_price", "high_level", "good_attrib (tuy chon)", "good_attrib1 (khong the pha huy/bo qua ne)",
	"good_attrib2 (hut NL/SL, + ky nang)", "good_attrib3", "good_attrib4", "good_attrib5",
};

static const char* KeepCauseName(int nCause)
{
	if (nCause < 0 || nCause >= sizeof(s_szKeepCause) / sizeof(s_szKeepCause[0])) return "?";
	return s_szKeepCause[nCause];
}

static const char* ObjKindName(int nKind)
{
	switch (nKind)
	{
	case Obj_Kind_MapObj: return "MapObj";
	case Obj_Kind_Body: return "Body";
	case Obj_Kind_Box: return "Box";
	case Obj_Kind_Item: return "Item";
	case Obj_Kind_Money: return "Money";
	case Obj_Kind_StoreBox: return "StoreBox";
	}
	return "?";
}

static KItem* GetItem(int nItemIdx)
{
	return (nItemIdx > 0 && nItemIdx < MAX_ITM) ? (KItem*)(dwItmBaseAdd + nItemIdx * ITM_DATA_SIZE) : NULL;
}

static BOOL CheckObjIdx(int nIdx)
{
	if (nIdx < 1 || nIdx >= MAX_OBJ)
	{
		DbgOut("LOI: Idx = %d ngoai 1..%d (lay o nut 'Danh sach Obj')\n", nIdx, MAX_OBJ - 1);
		return FALSE;
	}
	if (g_pObjArray[nIdx]->m_nID == -1)
	{
		DbgOut("LOI: KObj[%d] trong (m_nID = -1)\n", nIdx);
		return FALSE;
	}
	return TRUE;
}

// Giong het CVLCollect::CheckObj nhung tra ve ly do bo qua (NULL = se nhat)
static const char* CheckObjReason(KObj *pObj)
{
	CVLCollect &c = theCollect;
	POINT ptItem;
	if (pObj->m_nID == -1) return "m_nID = -1";
	if (pObj->m_Kind != Obj_Kind_Money && pObj->m_Kind != Obj_Kind_Item) return "khong phai Item/Money";
	if (pObj->m_Kind == Obj_Kind_Money && !c.m_bCollectMoney) return "tien - tat nhat tien/dac pham";
	if (pObj->m_Kind == Obj_Kind_Item)
	{
		if (pObj->m_nDataID == 18 || pObj->m_nDataID == 20) return "DataID 18/20 = mau, mana rom";
		if ((pObj->m_nDataID > 37 || pObj->m_nDataID == 19) && !c.m_bCollectMoney) return "dac pham - tat nhat tien/dac pham";
		int nWidth = pObj->m_nItemWidth, nHeight = pObj->m_nItemHeight;
		if (pObj->m_nDataID < 38 && pObj->m_nDataID != 19)
		{
			if (!c.m_nCollectEquip) return "VK/TB - m_nCollectEquip = 0";
			if (c.m_nCollectEquip < 4 && !pObj->m_nColorID) return "trang bi trang (ColorID 0)";
			if (c.m_nVioletLine == 6 && pObj->m_nColorID == 4) return "trang bi tim - m_nVioletLine = 6";
			if (c.m_nCollectEquip == 1 && nWidth * nHeight > 2 && nWidth > 1) return "> 2 o - m_nCollectEquip = 1";
		}
		if (!c.m_bGetCNHK && strstr(pObj->m_szName, CNHK_NAME)) return "Cam nang HK - m_bGetCNHK = 0";
		if (c.IsInBlackList(pObj->m_szName)) return "trong BlackItems.txt";
		if (c.m_bNeedFilter && g_BadObjs.IsBadObj(pObj->m_nItemDataID, pObj->m_nID)) return "g_BadObjs (da nhat roi vut)";
		if (!EQUIP_ROOM.FindRoom(nWidth, nHeight, &ptItem)) return "hanh trang het cho (FindRoom)";
	}
	return NULL;
}

static void OutObj(int i, int nPlayerX, int nPlayerY)
{
	KObj *pObj = g_pObjArray[i];
	int nX, nY;
	pObj->GetMapPos(&nX, &nY);
	int dZ = __distance(nPlayerX, nPlayerY, nX, nY);
	int nCheck = theCollect.CheckObj(pObj, nPlayerX, nPlayerY);
	const char *szReason = CheckObjReason(pObj);
	DbgOut("%-3d | %-8s | %3d | %d | %dx%d | (%d, %d) dZ %-5d | %s",
		i, ObjKindName(pObj->m_Kind), pObj->m_nDataID, pObj->m_nColorID, pObj->m_nItemWidth, pObj->m_nItemHeight,
		nX, nY, dZ, pObj->m_szName);
	if (pObj->m_Kind == Obj_Kind_Money) DbgOut(" (%d luong)", pObj->m_nMoneyNum);
	if (szReason)
		DbgOut("  => BO QUA: %s\n", szReason);
	else
		DbgOut("  => NHAT%s%s\n", (dZ < PLAYER_PICKUP_DISTANCE) ? " (du gan: PickUpObj)" : (dZ < theCollect.m_nCollectRange ? " (chay toi: MoveTo)" : " (ngoai tam m_nCollectRange)"),
			(nCheck > 0) == (szReason == NULL) ? "" : "  [LECH voi CheckObj!]");
}

static void CollectStatus()
{
	if (!DbgCheckPlayer()) return;
	CVLCollect &c = theCollect;
	DbgOut("m_cbSize = %d (VLAuto gui) - phan public = %d\n", c.m_cbSize, (int)offsetof(CVLCollect, m_bNeedFilter));
	DbgOut("m_bAutoCollect %d  m_nCollectRange %d  m_bCollectMoney %d  m_nCollectEquip %d  m_bPickUpInCity %d  m_bForcePickup %d\n",
		c.m_bAutoCollect, c.m_nCollectRange, c.m_bCollectMoney, c.m_nCollectEquip, c.m_bPickUpInCity, c.m_bForcePickup);
	DbgOut("m_bAutoArrange %d  m_bSaveJewellery %d (nhan %d, day chuyen %d dong)  m_nVioletLine %d\n",
		c.m_bAutoArrange, c.m_bSaveJewellery, c.m_nRingLine, c.m_nLaceLine, c.m_nVioletLine);
	DbgOut("m_bCollectPrice %d (%d)  m_bCollectLevel %d (%d)  m_bGetCNHK %d  m_bDecompose %d\n",
		c.m_bCollectPrice, c.m_nCollectPrice, c.m_bCollectLevel, c.m_nCollectLevel, c.m_bGetCNHK, c.m_bDecompose);
	DbgOut("Thuoc tinh giu do (arrMagic):");
	int nMagic = 0;
	for (int k = 0; k < NUM_ATTRIB && c.arrMagic[k].nAttribType; k++, nMagic++)
		DbgOut(" %d>%d", c.arrMagic[k].nAttribType, c.arrMagic[k].nAttribValue);
	DbgOut(nMagic ? "\n" : " (khong co)\n");
	DbgOut("m_bNeedFilter %d  m_nDropIndex %d  m_nArrangeIndex %d  PLAY_HAND_IDX %d  theMove.m_nStatus %d%s\n",
		c.m_bNeedFilter, c.m_nDropIndex, c.m_nArrangeIndex, PLAY_HAND_IDX, theMove.m_nStatus,
		(theMove.m_nStatus == status_pickup) ? " (status_pickup)" : "");
	DbgOut("InjectCode (hook KObjSet::ClientAdd) dang TAT - dia chi _INJ_ADD_ 0x%08X la cua ban cu, 10-10 ClientAdd = 0x683490\n", _INJ_ADD_);

	// Giong thu tu CVLCollect::Breathe
	DbgOut("Breathe(): ");
	if (g_NetworkOffline) { DbgOut("DUNG - g_NetworkOffline = %d\n", g_NetworkOffline); goto END; }
	if (!c.m_bAutoCollect) { DbgOut("DUNG - chua bat Tu nhat do\n"); goto END; }
	if (!P_PLAYER->m_FightMode && !c.m_bPickUpInCity) { DbgOut("DUNG - trong thanh (m_FightMode 0) va khong bat nhat trong thanh\n"); goto END; }
	DbgOut("SearchObj(%d) moi 50ms", c.m_nCollectRange);
	if (!P_PLAYER->m_FightMode || c.m_nCollectEquip == 0) { DbgOut(", khong loc/sap xep (%s)\n", P_PLAYER->m_FightMode ? "m_nCollectEquip = 0" : "trong thanh"); goto END; }
	{
		BOOL bNeedFilter = (c.m_nCollectEquip == 2 || c.m_bCollectPrice || c.m_bCollectLevel);
		if (!bNeedFilter && !c.m_bAutoArrange) { DbgOut(", khong loc/sap xep (tat ca 2)\n"); goto END; }
		DbgOut(", moi 0.5s: %s%s", bNeedFilter ? "FilterItems " : "", c.m_bAutoArrange ? "ArrangeItems" : "");
		if (theRecover.m_nAMBuffCount < MAX_BUFF_COUNT) DbgOut(" - DANG CHO: vua dung TDP (m_nAMBuffCount %d < %d)", theRecover.m_nAMBuffCount, MAX_BUFF_COUNT);
		DbgOut("\n");
	}
END:
	g_DbgReply.bOK = TRUE;
}

static void ObjList()
{
	if (!DbgCheckPlayer()) return;
	int nPlayerX, nPlayerY;
	P_PLAYER->GetMapPos(&nPlayerX, &nPlayerY);
	DbgOutPlayerPos("Player");
	DbgOut("Idx | Kind     | Data| Mau | WxH | Toa do, khoang cach | Ten => CheckObj (nhat khi dZ < %d)\n", PLAYER_PICKUP_DISTANCE);
	int nCount = 0;
	for (int i = 1; i < MAX_OBJ; i++)
	{
		if (g_pObjArray[i]->m_nID == -1) continue;
		OutObj(i, nPlayerX, nPlayerY);
		nCount++;
	}
	DbgOut("Tong: %d Obj (OBJ_BASE_ADD, MAX_OBJ %d)\n", nCount, MAX_OBJ);
	g_DbgReply.nResult[0] = nCount;
	g_DbgReply.bOK = TRUE;
}

static void CheckOneObj(int nIdx)
{
	if (!DbgCheckPlayer() || !CheckObjIdx(nIdx)) return;
	KObj *pObj = g_pObjArray[nIdx];
	int nPlayerX, nPlayerY;
	P_PLAYER->GetMapPos(&nPlayerX, &nPlayerY);
	DbgOut("KObj[%d] = 0x%08X  m_nID %d  m_Kind %d  m_nDataID %d  m_nState %d  m_nColorID %d  m_nItemDataID %d  m_dwScriptID 0x%X\n",
		nIdx, (DWORD)pObj, pObj->m_nID, pObj->m_Kind, pObj->m_nDataID, pObj->m_nState, pObj->m_nColorID, pObj->m_nItemDataID, pObj->m_dwScriptID);
	DbgOut("  Region %d (%d, %d)  Map %d,%d  Off %d,%d  LifeTime %d\n", pObj->m_RegionIdx, pObj->m_RegX, pObj->m_RegY,
		pObj->m_MapX, pObj->m_MapY, pObj->m_OffX, pObj->m_OffY, pObj->m_nLifeTime);
	OutObj(nIdx, nPlayerX, nPlayerY);
	g_DbgReply.nResult[0] = theCollect.CheckObj(pObj, nPlayerX, nPlayerY);
	DbgOut("theCollect.CheckObj() = %d (> 0: khoang cach, 0: bo qua)\n", g_DbgReply.nResult[0]);
	g_DbgReply.bOK = TRUE;
}

static void PickUp(int nIdx)
{
	if (!DbgCheckPlayer() || !CheckObjIdx(nIdx)) return;
	int nPlayerX, nPlayerY, nX, nY;
	P_PLAYER->GetMapPos(&nPlayerX, &nPlayerY);
	g_pObjArray[nIdx]->GetMapPos(&nX, &nY);
	int dZ = __distance(nPlayerX, nPlayerY, nX, nY);
	int nRoomBefore = g_pCurPlayer->CheckRoom();
	DbgOut("KObj[%d] %s  dZ = %d%s\n", nIdx, g_pObjArray[nIdx]->m_szName, dZ,
		(dZ < PLAYER_PICKUP_DISTANCE) ? "" : "  (xa hon PLAYER_PICKUP_DISTANCE - game se chay toi hoac khong nhat)");
	g_pCurPlayer->PickUpObj(nIdx);
	DbgOut("g_pCurPlayer->PickUpObj(%d) da goi. CheckRoom truoc = %d, m_nObjectIdx = %d - bam 'Danh sach Obj' de xem Obj con khong\n",
		nIdx, nRoomBefore, P_PLAYER->m_nObjectIdx);
	g_DbgReply.bOK = TRUE;
}

static void MoveToObj(int nIdx)
{
	if (!DbgCheckPlayer() || !CheckObjIdx(nIdx)) return;
	int nX, nY;
	g_pObjArray[nIdx]->GetMapPos(&nX, &nY);
	DbgOutPlayerPos("Truoc");
	g_pCurPlayer->MoveTo(nX, nY);
	DbgOut("g_pCurPlayer->MoveTo(%d, %d) toi KObj[%d] %s\n", nX, nY, nIdx, g_pObjArray[nIdx]->m_szName);
	g_DbgReply.nResult[0] = nX;
	g_DbgReply.nResult[1] = nY;
	g_DbgReply.bOK = TRUE;
}

static void SearchObjOnce(int nRange)
{
	if (!DbgCheckPlayer()) return;
	if (nRange <= 0) nRange = theCollect.m_nCollectRange;
	DbgOutPlayerPos("Truoc");
	DbgOut("theCollect.SearchObj(%d) - %s\n", nRange,
		(nRange == theCollect.m_nCollectRange) ? "tam m_nCollectRange" : "tam = Idx");
	theCollect.SearchObj(nRange);
	DbgOut("Xong: theMove.m_nStatus = %d%s  m_nObjectIdx = %d  Doing = %s\n", theMove.m_nStatus,
		(theMove.m_nStatus == status_pickup) ? " (status_pickup)" : "", P_PLAYER->m_nObjectIdx, DbgDoingName(P_PLAYER->m_Doing));
	g_DbgReply.bOK = TRUE;
}

static void BlackList(const char *szText)
{
	theCollect.LoadBlackList();
	DbgOut("LoadBlackList(): %s\\Maps\\BlackItems.txt\n", theMain.m_szCurFolder);
	int i = 0;
	while (i < MAX_BLACK_ITEMS && theCollect.m_szBlackItems[i][0])
	{
		DbgOut("  [%d] %s\n", i, theCollect.m_szBlackItems[i]);
		i++;
	}
	DbgOut("Tong: %d dong (toi da %d, moi dong 31 ky tu)\n", i, MAX_BLACK_ITEMS);
	if (szText[0])
		DbgOut("IsInBlackList(\"%s\") = %d\n", szText, theCollect.IsInBlackList(szText));
	g_DbgReply.nResult[0] = i;
	g_DbgReply.bOK = TRUE;
}

static void RedList(const char *szText)
{
	theCollect.LoadRedList();
	DbgOut("LoadRedList(): %s\\Maps\\RedItems.txt (dong: Ten-He, he 5 = moi he)\n", theMain.m_szCurFolder);
	int i = 0;
	while (i < MAX_RED_ITEMS && theCollect.m_RedItems[i].szItemName[0])
	{
		DbgOut("  [%d] %s - he %d\n", i, theCollect.m_RedItems[i].szItemName, theCollect.m_RedItems[i].nItemSeries);
		i++;
	}
	DbgOut("Tong: %d dong\n", i);
	if (szText[0])
	{
		for (int s = 0; s <= 5; s++)
			DbgOut("IsInRedList(\"%s\", he %d) = %d\n", szText, s, theCollect.IsInRedList(szText, s));
	}
	g_DbgReply.nResult[0] = i;
	g_DbgReply.bOK = TRUE;
}

static void KeepItems()
{
	if (!DbgCheckPlayer()) return;
	DbgOut("Idx   | X,Y  | Genre/Spec | Loc nhat (Filter)              | Ban (bForSelling)  | Ten\n");
	int nDrop = 0, nSell = 0;
	for (int i = 0; i < MAX_PLAYER_ITEM; i++)
	{
		if (PLAYER_ITEM(i).nPlace != pos_equiproom || !PLAYER_ITEM(i).nIdx) continue;
		int nIdx = PLAYER_ITEM(i).nIdx;
		KItem *pItem = GetItem(nIdx);
		int nKeep = theCollect.KeepTheItem(nIdx, FALSE);
		int nKeepSell = theCollect.KeepTheItem(nIdx, TRUE);
		if (nKeep == keep_nothing) nDrop++;
		if (nKeepSell == keep_nothing) nSell++;
		DbgOut("%-5d | %d,%d  | %d/%d | %-30s | %-18s | %s\n", nIdx, PLAYER_ITEM(i).nX, PLAYER_ITEM(i).nY,
			ComAttr.nItemGenre, ComAttr.nSpecialType, KeepCauseName(nKeep), KeepCauseName(nKeepSell), ComAttr.szItemName);
	}
	DbgOut("FilterItems se vut %d mon (khi m_bNeedFilter); SellItems se ban %d mon\n", nDrop, nSell);
	g_DbgReply.nResult[0] = nDrop;
	g_DbgReply.nResult[1] = nSell;
	g_DbgReply.bOK = TRUE;
}

static void ItemAttrib(int nIdx)
{
	if (!DbgCheckPlayer()) return;
	KItem *pItem = GetItem(nIdx);
	if (!pItem)
	{
		DbgOut("LOI: Idx = %d khong hop le (lay Idx o nut 'KeepTheItem' hoac 'Hanh trang')\n", nIdx);
		return;
	}
	DbgOut("KItem[%d] = 0x%08X  %s\n", nIdx, (DWORD)pItem, ComAttr.szItemName);
	DbgOut("  Genre %d  Special %d  Detail %d  Particular %d  ObjIdx %d  Series %d  Level %d  Price %d  %dx%d  Stack %d\n",
		ComAttr.nItemGenre, ComAttr.nSpecialType, ComAttr.nDetailType, ComAttr.nParticularType, ComAttr.nObjIdx,
		ComAttr.nSeries, ComAttr.nLevel, ComAttr.nPrice, ComAttr.nWidth, ComAttr.nHeight, ComAttr.bStack);
	DbgOut("  Do ben %d/%d  +%d  m_nLockCount %d  m_dwUniID 0x%08X  +0x6C0 %d (=1: client khong cho Ra)\n", pItem->m_nCurDur, pItem->m_nMaxDur,
		pItem->m_nItemPlus, pItem->m_nLockCount, pItem->m_dwUniID, *(int*)((BYTE*)pItem + 0x860));
	DbgOut("  m_MagicLevel:");
	for (int j = 0; j < 6; j++) DbgOut(" 0x%X", pItem->m_MagicLevel[j]);
	DbgOut("\n");
	for (int j = 0; j < 19; j++)
	{
		if (pItem->m_aryAllAttrib[j].nAttribType <= 0) continue;
		const char *szNote = "";
		int t = pItem->m_aryAllAttrib[j].nAttribType;
		if (j >= 13 && t > item_begin)
		{
			BOOL bUser = FALSE;
			for (int k = 0; k < NUM_ATTRIB && theCollect.arrMagic[k].nAttribType; k++)
				if (t == theCollect.arrMagic[k].nAttribType)
				{
					bUser = TRUE;
					szNote = (pItem->m_aryAllAttrib[j].nValue[0] > theCollect.arrMagic[k].nAttribValue) ? "  <= giu (dat tuy chon)" : "  (co trong tuy chon, chua du)";
				}
			if (!bUser && (t == indestructible_b || t == ignoredefense_p)) szNote = "  <= giu (good_attrib1)";
			else if (!bUser && t >= steallifeenhance_p && t <= earthskill_v) szNote = "  <= giu (good_attrib2)";
		}
		char szTier[16] = "";
		int nTier = (j >= 13) ? MagicAttribTier(t, pItem->m_aryAllAttrib[j].nValue[0]) : 0;
		if (nTier) sprintf(szTier, "cap %2d/10", nTier);
		char szAttr[128];
		const char *szDesc = MagicAttribDesc(t);
		_snprintf(szAttr, sizeof(szAttr) - 1, szDesc[0] ? "%s(%s)" : "%s", MagicAttribName(t), szDesc);
		szAttr[sizeof(szAttr) - 1] = 0;
		DbgOut("  attr[%2d] %s type %3d %s  value %d, %d, %d  %s%s\n", j, (j < 13) ? "goc " : "phep", t, szAttr,
			pItem->m_aryAllAttrib[j].nValue[0], pItem->m_aryAllAttrib[j].nValue[1], pItem->m_aryAllAttrib[j].nValue[2], szTier, szNote);
	}
	int nAttr = theCollect.CheckItemAttrib(pItem);
	int nKeep = theCollect.KeepTheItem(nIdx, FALSE);
	int nKeepSell = theCollect.KeepTheItem(nIdx, TRUE);
	DbgOut("CheckItemAttrib = %s\nKeepTheItem(%d, 0) = %s\nKeepTheItem(%d, 1) = %s\n", KeepCauseName(nAttr),
		nIdx, KeepCauseName(nKeep), nIdx, KeepCauseName(nKeepSell));
	g_DbgReply.nResult[0] = nKeep;
	g_DbgReply.nResult[1] = nKeepSell;
	g_DbgReply.bOK = TRUE;
}

static void RoomGrid()
{
	if (!DbgCheckPlayer()) return;
	KInventory &room = EQUIP_ROOM;
	DbgOut("EQUIP_ROOM: m_pArray 0x%08X  m_nWidth %d  m_nHeight %d  m_nMaxWidth %d  m_nMaxHeight %d  m_nMoney %d\n",
		(DWORD)room.m_pArray, room.m_nWidth, room.m_nHeight, room.m_nMaxWidth, room.m_nMaxHeight, room.m_nMoney);
	if (room.m_nWidth != room.m_nMaxWidth || room.m_nHeight != room.m_nMaxHeight)
		DbgOut("CHU Y: Width/Height khac MaxWidth/MaxHeight - FindRoom duyet theo Max, CanPlaceItem tinh o theo m_nWidth\n");
	if (!room.m_pArray || room.m_nWidth <= 0 || room.m_nHeight <= 0 || room.m_nWidth > 16 || room.m_nHeight > 32)
	{
		DbgOut("LOI: kich thuoc / m_pArray khong hop le\n");
		return;
	}
	DbgOut("Luoi (so = Idx vat pham, . = trong):\n");
	for (int y = 0; y < room.m_nHeight; y++)
	{
		DbgOut("  %2d:", y);
		for (int x = 0; x < room.m_nWidth; x++)
		{
			int v = room.m_pArray[y * room.m_nWidth + x];
			if (v) DbgOut(" %5d", v); else DbgOut("     .");
		}
		DbgOut("\n");
	}
	DbgOut("CheckRoom() = %d o da dung / %d\n", g_pCurPlayer->CheckRoom(), room.m_nWidth * room.m_nHeight);
	static const int s_Size[][2] = { {1, 1}, {1, 2}, {1, 3}, {2, 1}, {2, 2}, {2, 3}, {2, 4} };
	for (int i = 0; i < sizeof(s_Size) / sizeof(s_Size[0]); i++)
	{
		POINT pt, ptR;
		BOOL b = room.FindRoom(s_Size[i][0], s_Size[i][1], &pt);
		BOOL bR = room.FindRoom(s_Size[i][0], s_Size[i][1], &ptR, TRUE);
		DbgOut("  FindRoom(%dx%d) = %d (%d,%d)   nguoc = %d (%d,%d)\n", s_Size[i][0], s_Size[i][1], b, pt.x, pt.y, bR, ptR.x, ptR.y);
	}
	g_DbgReply.bOK = TRUE;
}

static void HandItemInfo()
{
	if (!DbgCheckPlayer()) return;
	int nHand = PLAY_HAND_IDX;
	KItem *pItem = GetItem(nHand);
	DbgOut("PLAY_HAND_IDX = %d  m_nDropIndex = %d  m_nArrangeIndex = %d  m_FightMode = %d  m_bNeedFilter = %d\n",
		nHand, theCollect.m_nDropIndex, theCollect.m_nArrangeIndex, P_PLAYER->m_FightMode, theCollect.m_bNeedFilter);
	if (!pItem)
	{
		DbgOut("Khong co do tren tay - CheckHandItem tra ve 0\n");
		g_DbgReply.bOK = TRUE;
		return;
	}
	int nKeep = theCollect.KeepTheItem(nHand, FALSE);
	DbgOut("Tren tay: %s (%dx%d) Genre %d  KeepTheItem = %s\n", ComAttr.szItemName, ComAttr.nWidth, ComAttr.nHeight,
		ComAttr.nItemGenre, KeepCauseName(nKeep));
	// Thu tu trong CVLCollect::CheckHandItem
	DbgOut("CheckHandItem se: ");
	if (P_PLAYER->m_FightMode && theCollect.m_bNeedFilter && nKeep == keep_nothing)
		DbgOut("ThrowAwayItem() (loc: khong giu)\n");
	else if (P_PLAYER->m_FightMode && theCollect.m_nDropIndex == nHand)
		DbgOut("ThrowAwayItem() (m_nDropIndex)\n");
	else if (theCollect.m_nArrangeIndex != nHand)
		DbgOut("cho (m_nArrangeIndex != tay; %s)\n", P_PLAYER->m_FightMode ? "sau ~10s moi nhan" : "trong thanh: lan sau nhan ngay");
	else
	{
		KUiPlayerBar *pUiPlayerBar = (KUiPlayerBar*)GetPtrAddress(UPB_BASE_ADD);
		POINT itmPos;
		if (!pUiPlayerBar)
			DbgOut("UPB_BASE_ADD = NULL - khong lam gi\n");
		else if (ComAttr.nItemGenre == item_townportal && !pUiPlayerBar->m_nHK3Gengre)
			DbgOut("dat TDP vao o phim tat 3 (UOC_IMMEDIA_ITEM, 2, 0)\n");
		else if (EQUIP_ROOM.FindRoom(ComAttr.nWidth, ComAttr.nHeight, &itmPos, ComAttr.nItemGenre != item_equip))
			DbgOut("SwitchItem(UOC_ITEM_TAKE_WITH, %d, %d) - cat vao hanh trang\n", itmPos.x, itmPos.y);
		else
			DbgOut("het cho%s\n", (P_PLAYER->m_FightMode && theRecover.m_bAMByFullRoom) ? " -> theRecover.OnUseAM() (ve thanh)" : "");
	}
	g_DbgReply.nResult[0] = nHand;
	g_DbgReply.bOK = TRUE;
}

static void TakeItem(int nIdx)
{
	if (!DbgCheckPlayer()) return;
	for (int i = 0; i < MAX_PLAYER_ITEM; i++)
	{
		if (PLAYER_ITEM(i).nPlace != pos_equiproom || PLAYER_ITEM(i).nIdx != nIdx || !nIdx) continue;
		KItem *pItem = GetItem(nIdx);
		DbgOut("PLAY_HAND_IDX truoc = %d\n", PLAY_HAND_IDX);
		g_pCurPlayer->SwitchItem(UOC_ITEM_TAKE_WITH, PLAYER_ITEM(i).nX, PLAYER_ITEM(i).nY);
		DbgOut("SwitchItem(UOC_ITEM_TAKE_WITH, %d, %d) - cam %s len tay. Bam 'Do tren tay' de xem\n",
			PLAYER_ITEM(i).nX, PLAYER_ITEM(i).nY, pItem ? ComAttr.szItemName : "?");
		g_DbgReply.bOK = TRUE;
		return;
	}
	DbgOut("LOI: khong co vat pham Idx = %d trong hanh trang (lay Idx o nut 'KeepTheItem')\n", nIdx);
}

static void BadObjs()
{
	DbgOut("g_BadObjs (NUM_BADITEMS %d, vat pham da nhat - khong nhat lai khi m_bNeedFilter):\n", NUM_BADITEMS);
	int nCount = 0;
	for (int i = 0; i < NUM_BADITEMS; i++)
	{
		if (!g_BadObjs.m_ItemData[i]) continue;
		DbgOut("  [%d] ItemDataID %d  ObjID %d\n", i, g_BadObjs.m_ItemData[i], g_BadObjs.m_ItemID[i]);
		nCount++;
	}
	DbgOut("Tong: %d\n", nCount);
	g_DbgReply.nResult[0] = nCount;
	g_DbgReply.bOK = TRUE;
}

// Quet do co the Ra: giong CVLCollect::FilterItems (m_bDecompose) + dieu kien core case 0x9B (Offsets_10_10.md muc 11)
static void DecomposeScan(BOOL bClearFail)
{
	if (!DbgCheckPlayer()) return;
	CVLCollect &c = theCollect;
	if (bClearFail)
	{
		memset(c.m_nDecomposeFail, 0, sizeof(c.m_nDecomposeFail));
		c.m_nDecomposeIdx = 0;
		c.m_nDecomposeTry = 0;
		DbgOut("Da xoa m_nDecomposeFail / m_nDecomposeIdx / m_nDecomposeTry\n");
	}

	// Dieu kien de Breathe goi toi FilterItems
	BOOL bNeedFilter = (c.m_nCollectEquip == 2 || c.m_bCollectPrice || c.m_bCollectLevel);
	DbgOut("m_bDecompose %d  m_bAutoCollect %d  m_FightMode %d  m_nCollectEquip %d  loc (VK/TB=2 | gia %d | cap %d) => m_bNeedFilter %d\n",
		c.m_bDecompose, c.m_bAutoCollect, P_PLAYER->m_FightMode, c.m_nCollectEquip, c.m_bCollectPrice, c.m_bCollectLevel, bNeedFilter);
	DbgOut("PLAY_HAND_IDX %d  theRecover.m_nAMBuffCount %d/%d  m_nDecomposeIdx %d  m_nDecomposeTry %d/10\n",
		PLAY_HAND_IDX, theRecover.m_nAMBuffCount, MAX_BUFF_COUNT, c.m_nDecomposeIdx, c.m_nDecomposeTry);
	DbgOut("m_nDecomposeFail:");
	int nFail = 0;
	for (int k = 0; k < 8; k++)
		if (c.m_nDecomposeFail[k]) { DbgOut(" %d", c.m_nDecomposeFail[k]); nFail++; }
	if (nFail)
		DbgOut("  (xoa sau %ds)\n", max(0, 30 - (int)(GetTickCount() - c.m_dwDecomposeFailTime) / 1000));
	else
		DbgOut(" (trong)\n");
	const char *szStop = NULL;
	if (!c.m_bDecompose) szStop = "chua bat Ra (m_bDecompose = 0) - do bi loc se bi VUT";
	else if (!c.m_bAutoCollect) szStop = "chua bat Tu nhat do - Breathe dung truoc FilterItems";
	else if (!P_PLAYER->m_FightMode) szStop = "trong thanh - khong loc";
	else if (!c.m_nCollectEquip) szStop = "m_nCollectEquip = 0 - khong loc";
	else if (!bNeedFilter) szStop = "m_bNeedFilter = 0 (can VK/TB = 2 hoac loc theo gia / cap)";
	else if (PLAY_HAND_IDX) szStop = "dang cam do tren tay - CheckHandItem xu ly truoc";
	else if (theRecover.m_nAMBuffCount < MAX_BUFF_COUNT) szStop = "vua dung AM - cho m_nAMBuffCount";
	DbgOut("Breathe -> FilterItems: %s\n\n", szStop ? szStop : "CO chay (moi 0.5s)");

	KInventory &room = EQUIP_ROOM;
	DbgOut("Idx | o     | Mau | +N | Khoa | +6C0 | Ten => ket qua\n");
	int nCan = 0, nKeep = 0, nFirst = 0, nOrder = 0, nNoGood = 0;
	for (int i = 0; i < MAX_PLAYER_ITEM; i++)
	{
		if (PLAYER_ITEM(i).nPlace != pos_equiproom) continue;
		int nIdx = PLAYER_ITEM(i).nIdx;
		KItem *pItem = GetItem(nIdx);
		if (!pItem || ComAttr.nItemGenre != item_equip) continue;
		DWORD dw6C0 = *(DWORD*)((BYTE*)pItem + 0x860);
		int nMagic = 0;
		while (nMagic < 6 && pItem->m_aryAllAttrib[13 + nMagic].nAttribType > item_begin) nMagic++;
		DbgOut("%-3d | %2d,%-2d | %d/%d | %-2d | %-4d | %-4X | %s =>", nIdx, PLAYER_ITEM(i).nX, PLAYER_ITEM(i).nY,
			ComAttr.nSpecialType, nMagic, pItem->m_nItemPlus, pItem->m_nLockCount, dw6C0, ComAttr.szItemName);

		ITEM_KEEP_CAUSE nCause = c.KeepTheItem(nIdx);
		if (nCause != keep_nothing)
		{
			DbgOut(" GIU: %s\n", KeepCauseName(nCause));
			nKeep++;
			continue;
		}
		nOrder++;	// FilterItems duyet theo thu tu nay, dung o mon dau tien chua bi bo qua
		int k;
		for (k = 0; k < 8 && c.m_nDecomposeFail[k] != nIdx; k++);
		BOOL bCell = FALSE;
		for (int n = 0; n < room.m_nWidth * room.m_nHeight && !bCell; n++)
			bCell = (room.m_pArray[n] == nIdx);
		if (nIdx > 0x1FF)
			DbgOut(" KHONG RA DUOC: Idx > 0x1FF (core 0x9B tu choi)");
		else if (dw6C0 == 1)
			DbgOut(" KHONG RA DUOC: item+0x6C0 = 1 (core 0x9B tu choi)");
		else if (!bCell)
			DbgOut(" KHONG RA DUOC: khong thay Idx trong EQUIP_ROOM.m_pArray (DecomposeItem tra FALSE)");
		else if (k < 8)
			DbgOut(" BO QUA: trong m_nDecomposeFail (da thu 10 lan khong mat)");
		else
		{
			DbgOut(" CO THE RA");
			if (!nFirst)
			{
				nFirst = nIdx;
				DbgOut(" <== FilterItems se Ra mon nay (thu %d trong danh sach loc)", nOrder);
			}
			nCan++;
		}
		if (nIdx > 0x1FF || dw6C0 == 1 || !bCell) nNoGood++;
		DbgOut("\n");
	}
	DbgOut("\nTong: %d co the Ra, %d giu lai, %d khong Ra duoc (phia client), %d trong m_nDecomposeFail\n", nCan, nKeep, nNoGood, nFail);
	if (nFail >= 8)
		DbgOut("CANH BAO: m_nDecomposeFail day 8 o - mon thu 9 khong Ra duoc se day mon cu ra, Auto co the lap mai o cac mon dau va khong toi duoc mon Ra duoc phia sau!\n");
	DbgOut("Mon 'CO THE RA' ma bam 'Ra Idx' van con => server tu choi (xem thong bao he thong trong game)\n");
	g_DbgReply.nResult[0] = nCan;
	g_DbgReply.nResult[1] = nFirst;
	g_DbgReply.bOK = TRUE;
}

static void NoSaveList(const char *szText)
{
	theMap.LoadNoSaveList();
	DbgOut("LoadNoSaveList(): %s\\Maps\\RuongItem.txt (khong cat vao ruong)\n", theMain.m_szCurFolder);
	int i = 0;
	while (i < MAX_NOSAVE_ITEMS && theMap.m_szNoSaveItems[i][0])
	{
		DbgOut("  [%d] %s\n", i, theMap.m_szNoSaveItems[i]);
		i++;
	}
	DbgOut("Tong: %d dong (toi da %d, moi dong 31 ky tu)\n", i, MAX_NOSAVE_ITEMS);
	if (szText[0])
		DbgOut("IsInNoSaveList(\"%s\") = %d\n", szText, theMap.IsInNoSaveList(szText));
	g_DbgReply.nResult[0] = i;
	g_DbgReply.bOK = TRUE;
}

static void InvNames()
{
	if (!DbgCheckPlayer()) return;
	char szNames[MAX_PLAYER_ITEM][32];
	int nCount = 0;
	for (int i = 0; i < MAX_PLAYER_ITEM; i++)
	{
		if (PLAYER_ITEM(i).nPlace != pos_equiproom || !PLAYER_ITEM(i).nIdx) continue;
		KItem *pItem = GetItem(PLAYER_ITEM(i).nIdx);
		if (!pItem || !ComAttr.szItemName[0]) continue;
		int k = 0;
		while (k < nCount && strcmp(szNames[k], ComAttr.szItemName)) k++;
		if (k < nCount) continue;
		strncpy(szNames[nCount], ComAttr.szItemName, 31);
		szNames[nCount][31] = 0;
		DbgOut("%s\n", szNames[nCount]);
		nCount++;
	}
	g_DbgReply.nResult[0] = nCount;
	g_DbgReply.bOK = TRUE;
}

BOOL DebugCollectCmd(const DEBUG_PARAM *pParam)
{
	const int *P = pParam->nParam;
	const char *szText = pParam->szParam;
	switch (pParam->nCmd)
	{
	case dbg_cl_status:
		CollectStatus();
		break;
	case dbg_cl_objlist:
		ObjList();
		break;
	case dbg_cl_checkobj:
		CheckOneObj(P[2]);
		break;
	case dbg_cl_pickup:
		PickUp(P[2]);
		break;
	case dbg_cl_movetoobj:
		MoveToObj(P[2]);
		break;
	case dbg_cl_searchobj:
		SearchObjOnce(P[2]);
		break;
	case dbg_cl_blacklist:
		BlackList(szText);
		break;
	case dbg_cl_redlist:
		RedList(szText);
		break;
	case dbg_cl_nosavelist:
		NoSaveList(szText);
		break;
	case dbg_cl_invnames:
		InvNames();
		break;
	case dbg_cl_keepitems:
		KeepItems();
		break;
	case dbg_cl_itemattrib:
		ItemAttrib(P[2]);
		break;
	case dbg_cl_room:
		RoomGrid();
		break;
	case dbg_cl_handitem:
		HandItemInfo();
		break;
	case dbg_cl_takeitem:
		TakeItem(P[2]);
		break;
	case dbg_cl_checkhand:
		if (!DbgCheckPlayer()) break;
		DbgOut("PLAY_HAND_IDX truoc = %d\n", PLAY_HAND_IDX);
		g_DbgReply.nResult[0] = theCollect.CheckHandItem(PLAY_HAND_IDX);
		DbgOut("theCollect.CheckHandItem(PLAY_HAND_IDX) = %d  m_nArrangeIndex = %d (bam lan 2 neu dang cho)\n",
			g_DbgReply.nResult[0], theCollect.m_nArrangeIndex);
		g_DbgReply.bOK = TRUE;
		break;
	case dbg_cl_filter:
		if (!DbgCheckPlayer()) break;
		g_DbgReply.nResult[0] = theCollect.FilterItems();
		DbgOut("theCollect.FilterItems() = %d  m_nDropIndex = %d%s\n", g_DbgReply.nResult[0], theCollect.m_nDropIndex,
			g_DbgReply.nResult[0] ? " - da cam mon nay len tay, 'CheckHandItem' de vut (khi ngoai thanh)" : " - khong co mon nao can vut");
		g_DbgReply.bOK = TRUE;
		break;
	case dbg_cl_arrange:
		if (!DbgCheckPlayer()) break;
		g_DbgReply.nResult[0] = theCollect.ArrangeItems();
		DbgOut("theCollect.ArrangeItems() = %d  m_nArrangeIndex = %d%s\n", g_DbgReply.nResult[0], theCollect.m_nArrangeIndex,
			g_DbgReply.nResult[0] ? " - da cam 1 mon len tay, bam 'CheckHandItem' 2 lan de dat vao cho moi" : " - da gon");
		g_DbgReply.bOK = TRUE;
		break;
	case dbg_cl_throwaway:
		if (!DbgCheckPlayer()) break;
		DbgOut("PLAY_HAND_IDX truoc = %d\n", PLAY_HAND_IDX);
		g_DbgReply.nResult[0] = ExecuteScript("ThrowAwayItem()");
		DbgOut("ExecuteScript(\"ThrowAwayItem()\") = %d\n", g_DbgReply.nResult[0]);
		g_DbgReply.bOK = TRUE;
		break;
	case dbg_cl_badobjs:
		BadObjs();
		break;
	case dbg_cl_decomposescan:
		DecomposeScan(P[2] == 1);
		break;
	case dbg_cl_decompose:
		if (!DbgCheckPlayer()) break;
		{
			KItem *pItem = GetItem(P[2]);
			if (!pItem) { DbgOut("LOI: Idx = %d (lay o nut 'Quet do co the Ra')\n", P[2]); break; }
			g_DbgReply.nResult[0] = g_pCurPlayer->DecomposeItem(P[2]);
			DbgOut("DecomposeItem(%d) %s = %d%s\n", P[2], ComAttr.szItemName, g_DbgReply.nResult[0],
				g_DbgReply.nResult[0] ? " - da gui lenh, bam 'Quet' lai xem mon nay con khong" : " - khong thay trong hanh trang");
			g_DbgReply.bOK = TRUE;
		}
		break;
	default:
		return FALSE;
	}
	return TRUE;
}
