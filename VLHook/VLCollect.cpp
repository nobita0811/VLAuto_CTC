#include "stdafx.h"
#include "../GameDef.h"
#include "DTextFile.h"
#include "GameBase.h"
#include "JxCore.h"
#include "S3Client.h"
#include "VLClasses.h"

#define byte_type(x)	__asm _emit x 
#define word_type(x)	byte_type((x>>(0*8))&0xFF)	byte_type((x>>(1*8))&0xFF)
#define dword_type(x)	byte_type((x>>(0*8))&0xFF)	byte_type((x>>(1*8))&0xFF)	byte_type((x>>(2*8))&0xFF)	byte_type((x>>(3*8))&0xFF)
#define START_MAGIC		0xCDDE51A9

KBadObj	g_BadObjs;

//================================Class KBadObj================================
KBadObj::KBadObj(void)
{
	memset(m_ItemData, 0, sizeof(m_ItemData));
	memset(m_ItemID, 0, sizeof(m_ItemID));
}

void KBadObj::Push(int nItemDataID, int nID)
{
	if (!nItemDataID) return;
	for (int i = 0; i < NUM_BADITEMS; i++)
	{
		if (m_ItemData[i] == nItemDataID) return;
	}
	for (int i = 0; i < NUM_BADITEMS - 1; i++)
	{
		m_ItemData[i] = m_ItemData[i + 1];
		m_ItemID[i] = m_ItemID[i + 1];
	}
	m_ItemData[NUM_BADITEMS - 1] = nItemDataID;
	m_ItemID[NUM_BADITEMS - 1] = nID;
}

BOOL KBadObj::IsBadObj(int nItemDataID, int nID)
{
	if (!nItemDataID) return TRUE;
	for (int i = 0; i < NUM_BADITEMS; i++)
	{
		if (m_ItemData[i] == nItemDataID && m_ItemID[i] != nID) return TRUE;
	}
	return 0;
}

void KObjSet_DataAdded(int objIndex)
{
	int nPlayerX, nPlayerY;
	P_PLAYER->GetMapPos(&nPlayerX, &nPlayerY);
	if (g_NetworkOffline) return;
	if (!theCollect.m_bAutoCollect) return;
	if (!P_PLAYER->m_FightMode && !theCollect.m_bPickUpInCity) return; // Ko nhat do trong thanh
	int dZ = theCollect.CheckObj(g_pObjArray[objIndex], nPlayerX, nPlayerY);
	if (dZ > 0 && dZ < PLAYER_PICKUP_DISTANCE)
	{
		g_BadObjs.Push(g_pObjArray[objIndex]->m_nItemDataID, g_pObjArray[objIndex]->m_nID);
		g_pCurPlayer->PickUpObj(objIndex);
	}
}

void OnAddNewObj()
{
	__asm
	{
		dword_type(START_MAGIC);
		mov eax, _ORG_PROC_;
		call eax;
		push eax;
		call KObjSet_DataAdded;
		pop eax;
		mov ebx, _RET_ADD_;
		jmp ebx;
	}
}

DWORD SearchCode(void* FuncAdd, DWORD findstr)
{
	__asm
	{
		mov eax, FuncAdd;
		jmp df;
	hjg:	inc eax;
	df:		mov ebx, [eax];
		cmp ebx, findstr;
		jnz hjg;
	}
}

int CompareItemPos(const void* a, const void* b)
{
	PlayerItem* p1 = (PlayerItem*)a;
	PlayerItem* p2 = (PlayerItem*)b;

	int pos1 = p1->nY * 6 + p1->nX;
	int pos2 = p2->nY * 6 + p2->nX;

	if (pos1 < pos2)
		return -1;
	if (pos1 > pos2)
		return 1;
	return 0;
}

//================================Class CVLCollect================================

CVLCollect::CVLCollect(void)
{
	m_nDropIndex = 0;
	m_nArrangeIndex = 0;
	m_bGetCNHK = TRUE;
	memset(m_nDecomposeFail, 0, sizeof(m_nDecomposeFail));
	m_nDecomposeIdx = 0;
	m_nDecomposeTry = 0;
	m_dwDecomposeFailTime = 0;
}

void CVLCollect::InjectCode(BOOL bInject)
{
#ifndef VLTK_FREE_MODE
	DWORD* dwValue; BYTE* btValue;
	DWORD dwOldProtect;

	if (!VirtualProtect((void*)_INJ_ADD_, 8, PAGE_EXECUTE_READWRITE, &dwOldProtect))
		return;

	if (bInject)
	{
		DWORD dwJmpCode = SearchCode(OnAddNewObj, START_MAGIC) + 4;
		btValue = (BYTE*)_INJ_ADD_;
		*btValue = 0xE9; // Jump Short
		dwValue = (DWORD*)(_INJ_ADD_ + 1);
		*dwValue = dwJmpCode - (DWORD)dwValue - 4;
	}
	else
	{
		btValue = (BYTE*)_INJ_ADD_;
		*btValue = 0xE8; // Call Proc
		dwValue = (DWORD*)(_INJ_ADD_ + 1);
		*dwValue = _ORG_PROC_ - (DWORD)dwValue - 4;
	}

	VirtualProtect((void*)_INJ_ADD_, 8, dwOldProtect, &dwOldProtect);
	FlushInstructionCache(GetCurrentProcess(), NULL, 0);
#endif
}

void CVLCollect::Breathe()
{
	static int nTimerCount = 0;
	if (g_NetworkOffline) return;
	if (!m_bAutoCollect) return;
	if (!P_PLAYER->m_FightMode && !m_bPickUpInCity) return; // Ko nhat do trong thanh

	SearchObj(m_nCollectRange); // Search OBJs by FOR LOOP

	if (!P_PLAYER->m_FightMode || m_nCollectEquip == 0) return;
	m_bNeedFilter = (m_nCollectEquip == 2 || m_bCollectPrice || m_bCollectLevel);
	if (!m_bNeedFilter && !m_bAutoArrange) return; // Ko co nhu cau sap xep, hoac chon loc

	nTimerCount++;
	if (nTimerCount % 10) return; // Check & Arrange Item each 0.5s
	if (theRecover.m_nAMBuffCount < MAX_BUFF_COUNT) return; // Just used AM

	if (CheckHandItem(PLAY_HAND_IDX)) return;
	if (m_bNeedFilter)
	{
		if (FilterItems()) return;
	}
	if (m_bAutoArrange)
	{
		ArrangeItems();
	}
}

void CVLCollect::SearchObj(int nRange)
{
	int nIndexMin = 0;
	BOOL bPicked = 0;
	int nPlayerX, nPlayerY;
	int dZ, dMin = nRange;
	P_PLAYER->GetMapPos(&nPlayerX, &nPlayerY);

	static int nTimeCount = 0;
	nTimeCount++;

	if (theMove.m_nStatus == status_pickup)
		theMove.m_nStatus = status_idle;

	for (int i = 1; i < MAX_OBJ; i++)
	{
		dZ = CheckObj(g_pObjArray[i], nPlayerX, nPlayerY);
		if (dZ <= 0) continue;	// Obj is NOTFOUND or INVALID
		if (dZ < PLAYER_PICKUP_DISTANCE)
		{
			g_BadObjs.Push(g_pObjArray[i]->m_nItemDataID, g_pObjArray[i]->m_nID);
			g_pCurPlayer->PickUpObj(i);
			bPicked = TRUE;
			if (m_bForcePickup && (nTimeCount % 80 > 10))
				theMove.m_nStatus = status_pickup;
			return;
		}
		if (dZ < dMin) {
			dMin = dZ;
			nIndexMin = i;
		}
	}

	if (!bPicked && nIndexMin) // FOUND an Object but TOO FAR
	{
		if (dMin >= PLAYER_PICKUP_DISTANCE && (m_bForcePickup || P_PLAYER->m_Doing == do_stand))
		{
			int nObjX, nObjY;
			g_pObjArray[nIndexMin]->GetMapPos(&nObjX, &nObjY);
			g_pCurPlayer->MoveTo(nObjX, nObjY);
		}
	}
}

//CheckObj
//return: > 0: Distance to Obj; 0: UNFOUND or INVALID
int CVLCollect::CheckObj(KObj* pObj, int nPlayerX, int nPlayerY)
{
	POINT ptItem;
	int nObjX, nObjY, dZ;
	int nWidth, nHeight, nSize;

	if (pObj->m_nID == -1) return 0;
	if (pObj->m_Kind != Obj_Kind_Money && pObj->m_Kind != Obj_Kind_Item) return 0;
	if (pObj->m_Kind == Obj_Kind_Money && !m_bCollectMoney) return 0; // Ko nhat tien
	if (pObj->m_Kind == Obj_Kind_Item)
	{
		if (pObj->m_nDataID == 18 || pObj->m_nDataID == 20) return 0; // Mau, mana rom
		if ((pObj->m_nDataID > 37 || pObj->m_nDataID == 19) && !m_bCollectMoney) return 0; // Ko nhat dac pham
		nWidth = pObj->m_nItemWidth;
		nHeight = pObj->m_nItemHeight;
		nSize = nWidth * nHeight;
		if (pObj->m_nDataID < 38 && pObj->m_nDataID != 19) // Vu khi, trang bi
		{
			if (!m_nCollectEquip) return 0; // Ko nhat VK & TB
			if (m_nCollectEquip < 4 && !pObj->m_nColorID) return 0; // Ko nhat trang bi trang
			if (m_nVioletLine == 6 && pObj->m_nColorID == 4) return 0; // Ko nhat trang bi tim
			if (m_nCollectEquip == 1 && nSize > 2 && nWidth > 1) return 0; // Ko nhat do > 2,3,4 o
		}
		if (!m_bGetCNHK && strstr(pObj->m_szName, TV(L"Cẩm nang hoàng kim"))) return 0;
		if (IsInBlackList(pObj->m_szName)) return 0; // Ko nhat trong BlackList
		if (m_bNeedFilter && g_BadObjs.IsBadObj(pObj->m_nItemDataID, pObj->m_nID)) return 0; // Ko nhat lai vat pham cu
		if (!EQUIP_ROOM.FindRoom(nWidth, nHeight, &ptItem)) return 0;
	}
	pObj->GetMapPos(&nObjX, &nObjY);
	dZ = __distance(nPlayerX, nPlayerY, nObjX, nObjY);
	return dZ;
}

void CVLCollect::LoadBlackList()
{
	memset(m_szBlackItems, 0, sizeof(m_szBlackItems));
	char szBlackItmFile[MAX_PATH];
	sprintf(szBlackItmFile, "%s\\Maps\\BlackItems.txt", theMain.m_szCurFolder);

	DTextFile itemFile;
	if (!itemFile.Open(szBlackItmFile))
		return;

	itemFile.ReadAllLines(m_szBlackItems[0], MAX_BLACK_ITEMS, 32);

	int i = 0;
	while (m_szBlackItems[i][0] && i < MAX_BLACK_ITEMS)
	{
		_strlwr(m_szBlackItems[i]);
		i++;
	}
}

BOOL CVLCollect::IsInBlackList(const char* szObjName)
{
	char szName[40] = "";
	strncpy(szName, szObjName, 32);
	_strlwr(szName);
	int i = 0;
	while (m_szBlackItems[i][0] && i < MAX_BLACK_ITEMS)
	{
		if (strstr(szName, m_szBlackItems[i]))
			return TRUE;
		i++;
	}
	return 0;
}

void CVLCollect::LoadRedList()
{
	memset(m_RedItems, 0, sizeof(m_RedItems));
	char szRedItmFile[MAX_PATH];
	sprintf(szRedItmFile, "%s\\Maps\\RedItems.txt", theMain.m_szCurFolder);

	DTextFile itemFile;
	if (!itemFile.Open(szRedItmFile))
		return;

	int nLineNum = 0;
	char szLine[256];
	while (nLineNum < MAX_RED_ITEMS)
	{
		if (itemFile.ReadLine(szLine, 256) < 0)
			break;
		if (szLine[0] != '#')
		{
			char* chSeries = strrchr(szLine, '-');
			if (chSeries != NULL)
			{
				chSeries[0] = 0;
				strcpy(m_RedItems[nLineNum].szItemName, szLine);
				chSeries++;
				m_RedItems[nLineNum].nItemSeries = chSeries[0] - '0';
			}
			nLineNum++;
		}
	}
}

BOOL CVLCollect::IsInRedList(const char* szItemName, int nSeries)
{
	int i = 0;
	while (m_RedItems[i].szItemName[0] && i < MAX_RED_ITEMS)
	{
		if (strstr(szItemName, m_RedItems[i].szItemName))
		{
			if (m_RedItems[i].nItemSeries == 5) return TRUE; // ALL Series
			if (m_RedItems[i].nItemSeries == nSeries) return TRUE;
		}
		i++;
	}
	return 0;
}

ITEM_KEEP_CAUSE CVLCollect::CheckItemAttrib(KItem* pItem)
{
	int j, k;
	if (ComAttr.nItemGenre != item_equip) return keep_not_equip;
	for (j = 13; j < 19; j++)
	{
		if (ItmAttr(j) <= item_begin) continue;
		for (k = 0; k < NUM_ATTRIB && arrMagic[k].nAttribType; k++)
			if (ItmAttr(j) == arrMagic[k].nAttribType) break;
		if (k < NUM_ATTRIB && arrMagic[k].nAttribType)
		{	// Check with user options (uu tien hon "luon giu" ben duoi)
			if (pItem->m_aryAllAttrib[j].nValue[0] > arrMagic[k].nAttribValue)
				return keep_good_attrib; // Thuoc tinh do
			continue;
		}
		if (ItmAttr(j) == indestructible_b || ItmAttr(j) == ignoredefense_p)
			return keep_good_attrib1; // Do khong the pha huy, bo qua ne tranh
		if (ItmAttr(j) >= steallifeenhance_p && ItmAttr(j) <= earthskill_v)
			return keep_good_attrib2; // Do hut NL/SL, do + ky nang
	}
	return keep_nothing;
}

ITEM_KEEP_CAUSE CVLCollect::KeepTheItem(int nItemIdx, BOOL bForSelling)
{
	int nItemSize;
	KItem* pItem = (KItem*)(dwItmBaseAdd + nItemIdx * ITM_DATA_SIZE);
	if (ComAttr.nItemGenre != item_equip) return keep_not_equip;
	if (pItem->m_nLockCount && (bForSelling || !m_bDecompose)) return keep_locked_equip; // Do khoa: khong vut duoc, chi Ra
	if (ComAttr.nSpecialType == 1) return keep_golden_equip;
	if (ComAttr.nSpecialType == 2)
	{
		if (!bForSelling) return keep_violet_equip;
#ifndef VLTK_FREE_MODE
		int nMagicLine = 0;
		while (nMagicLine < 6)
		{
			if (pItem->m_MagicLevel[nMagicLine] == 0)
				break;
			if (pItem->m_MagicLevel[nMagicLine] != 0xFFFFFFFF)
				return keep_violet_equip;
			nMagicLine++;
		}
		//nMagicLine = 6 - nMagicLine;
		if (nMagicLine > m_nVioletLine) return keep_violet_equip;
#else
		int nMagicLine = 0;
		while (nMagicLine < 6)
		{
			if (pItem->m_MagicLevel[nMagicLine] == 0)
				break;
			if (pItem->m_MagicLevel[nMagicLine] != 0xFFFF)
				return keep_violet_equip;
			nMagicLine++;
		}
		if (nMagicLine > m_nVioletLine) return keep_violet_equip;
#endif
	}
	if (ComAttr.nSpecialType > 2) return keep_special_equip;
	if (m_bSaveJewellery)
	{
		int nMagicLine = 0;
		while (nMagicLine < 6 && pItem->m_aryAllAttrib[13 + nMagicLine].nAttribType > item_begin)
			nMagicLine++;
		if (ComAttr.nObjIdx == 37 && nMagicLine >= m_nRingLine) return keep_save_ring;
		if (ComAttr.nObjIdx == 36 && nMagicLine >= m_nLaceLine) return keep_save_lace;
	}
	if (ComAttr.nDetailType == equip_mask) return keep_mask_equip;
	if (ComAttr.nDetailType == equip_horse && ComAttr.nParticularType >= 5) return keep_good_horse;
	if (IsInRedList(ComAttr.szItemName, ComAttr.nSeries)) return keep_red_item;
	if (!bForSelling)
	{
		nItemSize = ComAttr.nWidth * ComAttr.nHeight;
		if (m_bCollectPrice && ComAttr.nPrice > m_nCollectPrice * nItemSize * 4) return keep_high_price;
		if (m_bCollectLevel && ComAttr.nLevel > m_nCollectLevel) return keep_high_level;
	}
	if (m_nCollectEquip == 2 || bForSelling)
		return CheckItemAttrib(pItem);
	return keep_nothing;
}

BOOL CVLCollect::CheckHandItem(int nHandIdx)
{
	if (!nHandIdx) return 0;

	// Check Drop Items
	if (P_PLAYER->m_FightMode)
	{
		if (m_bNeedFilter)
			if (KeepTheItem(nHandIdx) == keep_nothing)
			{
				if (!m_bDecompose)
				{
					ExecuteScript("ThrowAwayItem()");
					return TRUE;
				}
				// Ra: dat lai vao hanh trang, FilterItems se Ra
				KItem* pItem = (KItem*)(dwItmBaseAdd + nHandIdx * ITM_DATA_SIZE);
				POINT itmPos;
				if (EQUIP_ROOM.FindRoom(ComAttr.nWidth, ComAttr.nHeight, &itmPos, FALSE))
				{
					g_pCurPlayer->SwitchItem(UOC_ITEM_TAKE_WITH, itmPos.x, itmPos.y);
					return TRUE;
				}
			}
		if (m_nDropIndex == nHandIdx)
		{
			ExecuteScript("ThrowAwayItem()");
			return TRUE;
		}
	}

	// Do tren tay ma hanh trang het cho: ve thanh ngay, khong cho 10s sap xep
	if (P_PLAYER->m_FightMode && theRecover.m_bAMByFullRoom)
	{
		KItem* pItem = (KItem*)(dwItmBaseAdd + nHandIdx * ITM_DATA_SIZE);
		POINT itmPos;
		if (ComAttr.nItemGenre != item_townportal &&
			!EQUIP_ROOM.FindRoom(ComAttr.nWidth, ComAttr.nHeight, &itmPos, ComAttr.nItemGenre != item_equip))
		{
			theRecover.OnUseAM();
			return TRUE;
		}
	}

	// Check Arrange Items
	if (m_nArrangeIndex != nHandIdx)
	{
		static int nTimeCount = 0;
		nTimeCount++;
		if (!P_PLAYER->m_FightMode || nTimeCount > 20) // 10s
		{
			nTimeCount = 0;
			m_nArrangeIndex = nHandIdx;
		}
	}
	else //(m_nArrangeIndex == nHandIdx)
	{
		KUiPlayerBar* pUiPlayerBar = (KUiPlayerBar*)GetPtrAddress(UPB_BASE_ADD);
		if (!pUiPlayerBar) return TRUE;
		KItem* pItem = (KItem*)(dwItmBaseAdd + PLAY_HAND_IDX * ITM_DATA_SIZE);
		if (ComAttr.nItemGenre == item_townportal && (!pUiPlayerBar->m_nHK3Gengre))
			g_pCurPlayer->SwitchItem(UOC_IMMEDIA_ITEM, 2, 0);
		else
		{
			POINT itmPos;
			if (EQUIP_ROOM.FindRoom(ComAttr.nWidth, ComAttr.nHeight, &itmPos, ComAttr.nItemGenre != item_equip))
				g_pCurPlayer->SwitchItem(UOC_ITEM_TAKE_WITH, itmPos.x, itmPos.y);
			else if (P_PLAYER->m_FightMode && theRecover.m_bAMByFullRoom)
				theRecover.OnUseAM();
		}
	}
	return TRUE;
}

BOOL CVLCollect::FilterItems()
{
	m_nDropIndex = 0;
	if (m_nDecomposeFail[0] && GetTickCount() - m_dwDecomposeFailTime > 30000)
		memset(m_nDecomposeFail, 0, sizeof(m_nDecomposeFail)); // Thu Ra lai (server co the chi ban tam thoi)
	for (int i = 0; i < MAX_PLAYER_ITEM; i++)
	{
		if (PLAYER_ITEM(i).nPlace != pos_equiproom) continue;
		if (KeepTheItem(PLAYER_ITEM(i).nIdx) == keep_nothing)
		{
			if (m_bDecompose)
			{
				int k, nIdx = PLAYER_ITEM(i).nIdx;
				for (k = 0; k < 8 && m_nDecomposeFail[k] != nIdx; k++);
				if (k < 8) continue; // Ra khong duoc - bo qua
				if (nIdx != m_nDecomposeIdx)
				{
					m_nDecomposeIdx = nIdx;
					m_nDecomposeTry = 0;
				}
				if (++m_nDecomposeTry > 10) // ~5s van con -> tam bo qua 30s
				{
					memmove(m_nDecomposeFail + 1, m_nDecomposeFail, sizeof(int) * 7);
					m_nDecomposeFail[0] = nIdx;
					m_dwDecomposeFailTime = GetTickCount();
					m_nDecomposeIdx = 0;
					continue;
				}
				g_pCurPlayer->DecomposeItem(nIdx);
				return TRUE;
			}
			g_pCurPlayer->SwitchItem(UOC_ITEM_TAKE_WITH, PLAYER_ITEM(i).nX, PLAYER_ITEM(i).nY);
			m_nDropIndex = PLAYER_ITEM(i).nIdx;
			return TRUE;
		}
	}
	return 0;
}

BOOL CVLCollect::ArrangeItems()
{
	m_nArrangeIndex = 0;
	PlayerItem roomItems[60];

	int itemCount = 0;
	for (int i = 0; i < MAX_PLAYER_ITEM; i++)
	{
		if (PLAYER_ITEM(i).nPlace != pos_equiproom) continue;
		roomItems[itemCount++] = PLAYER_ITEM(i);
	}

	if (itemCount == 0)
		return FALSE;

	::qsort(roomItems, itemCount, sizeof(PlayerItem), CompareItemPos);

	POINT itmPos;
	for (int i = 0; i < itemCount; i++)
	{
		KItem* pItem = g_pItemArray[roomItems[i].nIdx];
		if (ComAttr.nItemGenre == item_equip)
			continue;

		if (!EQUIP_ROOM.FindRoom(ComAttr.nWidth, ComAttr.nHeight, &itmPos, TRUE, roomItems[i].nIdx)) continue;

		int curPos = roomItems[i].nY * EQUIP_ROOM.m_nWidth + roomItems[i].nX;
		int newPos = itmPos.y * EQUIP_ROOM.m_nWidth + itmPos.x;

		if (newPos > curPos)
		{
			g_pCurPlayer->SwitchItem(UOC_ITEM_TAKE_WITH, roomItems[i].nX, roomItems[i].nY);
			m_nArrangeIndex = roomItems[i].nIdx;
			return TRUE;
		}
	}

	for (int i = itemCount - 1; i >= 0; i--)
	{
		KItem* pItem = g_pItemArray[roomItems[i].nIdx];
		if (ComAttr.nItemGenre != item_equip)
			continue;

		if (!EQUIP_ROOM.FindRoom(ComAttr.nWidth, ComAttr.nHeight, &itmPos, FALSE, roomItems[i].nIdx)) continue;

		int curPos = roomItems[i].nY * EQUIP_ROOM.m_nWidth + roomItems[i].nX;
		int newPos = itmPos.y * EQUIP_ROOM.m_nWidth + itmPos.x;

		if (newPos < curPos)
		{
			g_pCurPlayer->SwitchItem(UOC_ITEM_TAKE_WITH, roomItems[i].nX, roomItems[i].nY);
			m_nArrangeIndex = roomItems[i].nIdx;
			return TRUE;
		}
	}

	return 0;
}

void CVLCollect::SetAutoArrange()
{
	char szMsg[80];
	if (m_bAutoArrange)
		sprintf(szMsg, TV(L"<color=yellow>Tự sắp xếp đồ (Ctrl+B): <color=green>Bật"));
	else
	{
		sprintf(szMsg, TV(L"<color=yellow>Tự sắp xếp đồ (Ctrl+B): <color>Tắt"));
		CheckHandItem(PLAY_HAND_IDX);
	}
	SystemMessage(szMsg, 1);
}
