#include "stdafx.h"
#include "../GameDef.h"
#include "GameBase.h"
#include "JxCore.h"
#include "S3Client.h"
#include "VLClasses.h"

//================================Class CVLUtil================================
CVLUtil::CVLUtil(void)
{
	m_bDoingUtil = 0;
	m_nUtilTask = 0;
	m_nItemObjIdx = 0;
	m_nFromRoom = 0;
	m_nToRoom = 0;
	m_nHTLevel = 0;
	m_nMaxLBCount = 60;
	// Private Members
	m_nDoJobStep = 0;
	m_bPausingEntrust = 0;
	m_nEventKind = 0;
}

void CVLUtil::Breath()
{
	if (g_NetworkOffline) return;

	//AutoOpenItems();
	CheckPMControl();

	if (P_PLAYER->m_FightMode)
	{
		if (m_bDoingUtil)
			SystemMessage("<color=yellow>§ang trong tr¹ng th¸i chiÕn ®Êu, kh«ng thÓ thùc hiÖn thao t¸c nµy!");
		m_bDoingUtil = 0;
		m_nUtilTask = 0;
		m_nDoJobStep = 0;
		return;
	}

	if (!m_bDoingUtil)
	{
		m_nUtilTask = 0;
		m_nDoJobStep = 0;
		return;
	}
	switch (m_nUtilTask)
	{
	case util_moveitem:
		MoveItems();
		break;
	case util_givekcl:
		GiveKCL();
		break;
	case util_buyitem:
		SearhBuyItem();
		break;
	case util_opengif:
		OpenGifts();
		break;
	case util_getlenhbai:
		DoEventGetLB();
		break;
	default:
		m_bDoingUtil = 0;
		m_nUtilTask = 0;
		m_nDoJobStep = 0;
	}
}

void CVLUtil::AutoClickNpc()
{
	int nPlayerX, nPlayerY, nNpcX, nNpcY;
	int dZ, dMin = 1600;
	int nNpcIdx = 0;
	P_PLAYER->GetMapPos(&nPlayerX, &nPlayerY);
	for (int i = 1; i < MAX_NPC; i++)
	{
		if (!g_pNpcArray[i]->IsExist() || g_pNpcArray[i]->m_Kind != kind_dialoger) continue;
		g_pNpcArray[i]->GetMapPos(&nNpcX, &nNpcY);
		dZ = __distance(nPlayerX, nPlayerY, nNpcX, nNpcY);
		if (dZ < dMin)
		{
			nNpcIdx = i;
			dMin = dZ;
		}
	}
	if (nNpcIdx)
		theAttack.SetPeopleIdx(nNpcIdx);
}

void CVLUtil::ReadPosition()
{
	int nLen;
	char szPos[16];	char szInfo[128];
	int nFlagX = *((int*)FLAG_X_OFFST);
	int nFlagY = *((int*)FLAG_Y_OFFST);

	if (!_OpenClipboard(theMain.m_hWndGame))	return;

	if (nFlagX < 1 || nFlagY < 1)
	{
		P_PLAYER->GetMapPos(&nFlagX, &nFlagY);
		sprintf(szInfo, "Täa ®é hiÖn t¹i: <color=green>(%d,%d)<color>...\n", nFlagX, nFlagY);
	}
	else
	{
		nFlagX <<= 4;
		nFlagY <<= 5;
		sprintf(szInfo, "Täa ®é c¾m cê: <color=green>(%d,%d)<color>...\n", nFlagX, nFlagY);
	}
	strcat(szInfo, "  ®· ®­îc l­u vµo Cliboard!");

	sprintf(szPos, "%d,%d\r\n", nFlagX, nFlagY);
	_EmptyClipboard();
	nLen = (int)strlen(szPos);
	HGLOBAL hCopy = GlobalAlloc(GMEM_MOVEABLE, nLen + 1);
	if (hCopy == NULL)
	{
		_CloseClipboard();
		return;
	}
	char *szCopy = (char*)GlobalLock(hCopy); 
	strcpy(szCopy, szPos);
	GlobalUnlock(hCopy);
	_SetClipboardData(CF_TEXT, hCopy);
	_CloseClipboard();

	ExecuteScript("ClearMessage()");
	SystemMessage(szInfo);
}

void CVLUtil::DeleteSysMessages()
{
	KUiSysMsgCentre *pSysMsg = (KUiSysMsgCentre*)GetPtrAddress(SMC_BASE_ADD);
	if (!pSysMsg) return;
	while (pSysMsg->m_MsgHeap[0].nNumValid) pSysMsg->DeleteMsgInHeap(0, 0);
}

void CVLUtil::OpenCloseStoreBox()
{
	KWndImage *pKUiStoreBox = (KWndImage*)(*(LPDWORD)SBO_BASE_ADD);
	if (pKUiStoreBox && pKUiStoreBox->IsVisible())
		CloseAllWindows(0);
	else
		OpenStoreWindow();
}

void CVLUtil::OpenCloseNpcShop()
{
	KUiShop *pShop = (KUiShop*)GetPtrAddress(SHOP_BASE_ADD);
	if (pShop && pShop->IsVisible())
		CloseAllWindows(0);
	else
		OpenShopWindow();
}

void CVLUtil::CheckLifeTime()
{
#ifndef VLTK_FREE_MODE
	time_t *pTime = (time_t*)LIFETIME_DYS;
	if (*pTime > 0)
	{
		tm tmDeadLine;
		localtime_s(&tmDeadLine, pTime);
		SystemMessage(0, "H¹n sö dông: <color=green>%d-%02d-%02d %02d:%02d", tmDeadLine.tm_year + 1900,
			tmDeadLine.tm_mon + 1, tmDeadLine.tm_mday, tmDeadLine.tm_hour, tmDeadLine.tm_min);
	}
	DWORD	dwLifeTime = *(LPDWORD)LIFETIME_HRS;
	if (dwLifeTime > 0)
	{
		int nHours, nMinutes;
		nHours = dwLifeTime / 3600;
		nMinutes = (dwLifeTime / 60) % 60;
		SystemMessage(0, "Thêi gian: <color=green>%02d giê %02d phót", nHours, nMinutes);
	}
#else
	SystemMessage("<color=green>B¹n ®ang ch¬i VLTK b¶n miÔn phÝ!!!");
#endif
}

void CVLUtil::CheckPMControl()
{
	if (!theMain.m_szControlPass[0]) return;
	if (!CHATROOM_LIST) return;	// 10-10: layout hop tin nhan chua tim

	static int nTimeCount = 0;
	if (nTimeCount++ % 30 != 15) return; // make it at 3s

	KWndMessageListBox *szMsgListBox = (KWndMessageListBox*)(GetPtrAddress(MCP_BASE_ADD) + CHATROOM_LIST);
	char szMessage[80] = "";
	char szMsgReply[80] = "";
	int nMsgCount = szMsgListBox->m_nNumMessage;

	for (int i = nMsgCount-1; i >= 0; i--)
	{
		int *nMsgCode = (int*)(szMsgListBox->m_pMessages[i]->Msg + 0x0007);

		if (*nMsgCode != 0x75115004 && *nMsgCode != 0x9200A704) continue;
		strncpy(szMessage, szMsgListBox->m_pMessages[i]->Msg + 0x0013, 64);
		char *szSender = szMessage;
		char *szPwd = strchr(szMessage, ':');	// Password
		if (!szPwd) continue;
		szPwd[0] = 0; szPwd += 3;
		if (strlen(szSender) > 32) continue;
		char *szCmd = strchr(szPwd, 0x20);		// Command
		if (!szCmd) continue;
		szCmd[0] = 0; szCmd += 1;

		if (strcmp(szPwd, theMain.m_szControlPass)) continue;

		//Correct Password
		if (!strcmp(szCmd, "hello"))
			sprintf(szMsgReply, "Hello <pic=0>");
		else if (!strcmp(szCmd, "where"))
		{
			char szMapName[64];
			theMain.GetMapName((*g_pMapID), szMapName);
			int nPlayerX, nPlayerY;
			P_PLAYER->GetMapPos(&nPlayerX, &nPlayerY);
			sprintf(szMsgReply, "%s (%d/%d).", szMapName, nPlayerX>>8, nPlayerY>>9);
		}
		else if (!strcmp(szCmd, "money"))
		{
			sprintf(szMsgReply, "Ng©n l­îng hµnh trang = %d, r­¬ng = %d.", EQUIP_MONEY, ROOM_MONEY);
		}
		else if (!strcmp(szCmd, "datau"))
		{
			theQuest.ReportQuest(0, szMsgReply);
		}
		else if (!strcmp(szCmd, "leavept"))
		{
			sprintf(szMsgReply, "§· rêi khái Tæ ®éi.");
			g_pCoreShell->TeamOperation(TEAM_LEAVE, 0, 0);
		}
		else if (!strcmp(szCmd, "exit"))
		{
			sprintf(szMsgReply, "§ang tho¸t Game.");
			theMain.ExitGame();
		}
		else if (!strcmp(szCmd, "exitall"))
		{
			sprintf(szMsgReply, "§ang tho¸t tÊt c¶ Game.");
			theMain.PostFeedBack(cmd_exitall, 0);
		}
		else if (!strcmp(szCmd, "shutdown"))
		{
			sprintf(szMsgReply, "ChuÈn bÞ t¾t m¸y tÝnh.");
			theMain.PostFeedBack(cmd_exitall, 1);
		}
		else if (!strcmp(szCmd, "restart"))
		{
			sprintf(szMsgReply, "ChuÈn bÞ khëi ®éng l¹i m¸y.");
			theMain.PostFeedBack(cmd_exitall, 2);
		}
		else
			sprintf(szMsgReply, "Sai lÖnh! Xin ®äc l¹i H­íng dÉn!");

		SendPrivateMessage(szSender, szMsgReply);

		ExecuteScript("ClearMessage()");
		return;
	}
}

void CVLUtil::MoveItems()
{
	static int nItemGenre = -1;
	static int nItemDetail = -1;
	static int nItemParticular = -1;
	static char szItemName[32] = "";

	int nSrcContainer, nDstContainer, nSrcPos, nDstPos;
	theMain.GetContainerPos(m_nFromRoom, &nSrcContainer, &nSrcPos);
	theMain.GetContainerPos(m_nToRoom, &nDstContainer, &nDstPos);
	if (!nSrcContainer || !nDstContainer) return;

	int nItemIdx;
	KItem *pItem;
	KInventory DestRoom;
	KInventory SourceRoom = PLAYER_ROOM[m_nFromRoom];
	if (m_nToRoom < room_num)
		DestRoom = PLAYER_ROOM[m_nToRoom];

	if (m_nDoJobStep == 0)
	{
		nItemGenre = -1;
		nItemDetail = -1;
		nItemParticular = -1;
		szItemName[0] = 0;
		if (nSrcContainer == UOC_GUID_BOX || nDstContainer == UOC_GUID_BOX)
		{
			KWndImage *pKUiItemEx = (KWndImage*)GetPtrAddress(ITX_BASE_ADD);
			if (!pKUiItemEx || !pKUiItemEx->IsVisible())
			{
				ExecuteScript("Open([[items]])");
				ExecuteScript("Open([[ItemEx]])");
			}
		}
		if ((nSrcContainer >= UOC_STORE_BOX && nSrcContainer <= UOC_STORE_BOX3) ||
			(nDstContainer >= UOC_STORE_BOX && nDstContainer <= UOC_STORE_BOX3))
		{
			if (theMain.OpenLockRoom(theMap.m_nRepositoryPass))
				return;
			int nOpeningStore = theMap.OpenStoreBox();
			if (nOpeningStore <= 0)
			{
				if (nOpeningStore < 0)
				{
					SystemMessage("<color=yellow>Kh«ng t×m thÊy R­¬ng ®å!!!");
					m_bDoingUtil = 0;
				}
				return;
			}
			m_nDoJobStep = 1;
			return;
		}
		PLAYER_ROOM[room_trade].m_nMaxWidth = 8;
		m_nDoJobStep = 1;
		return;
	}
	if (m_nDoJobStep == 1)
	{
		switch(m_nItemObjIdx)
		{
		case 0: // Vat pham mau
			if (nDstContainer == UOC_GAMESPACE)
				nItemIdx = PLAY_HAND_IDX;
			else
				nItemIdx = DestRoom.m_pArray[0];
			if (nItemIdx > 0)
			{
				pItem = (KItem*)(dwItmBaseAdd + nItemIdx*ITM_DATA_SIZE);
				nItemGenre = ComAttr.nItemGenre;
				nItemDetail = ComAttr.nDetailType;
				nItemParticular = ComAttr.nParticularType;
			}
			break;
		case 1: // Trang suc
			nItemGenre = item_equip;
			nItemDetail = equip_ring + equip_amulet + equip_pendant;
			break;
		case 2: // Nhan
			nItemGenre = item_equip;
			nItemDetail = equip_ring;
			break;
		case 3: // Day chuyen
			nItemGenre = item_equip;
			nItemDetail = equip_amulet;
			break;
		case 4: // Ngoc boi
			nItemGenre = item_equip;
			nItemDetail = equip_pendant;
			break;
		case 5: // Kim Nguyen Bao
			strcpy(szItemName, "Kim Nguyªn b¶o");
			break;
		case 6: // Thuy Tinh
			strcpy(szItemName, "Thñy Tinh");
			break;
		case 7: // Tinh Hong BT
			strcpy(szItemName, "Tinh Hång B¶o Th¹ch");
			break;
		case 8: // Tien Thao Lo
			strcpy(szItemName, "Tiªn Th¶o Lé");
			break;
		case 9: // Phuc Duyen Lo
			strcpy(szItemName, "Phóc Duyªn Lé");
			break;
		case 10: // Khieu chien lenh
			strcpy(szItemName, KC_LENH);
			break;
		case 11:
			nItemGenre = item_number; //All Item
			break;
		}
		m_nDoJobStep = 2;
	}
	if (m_nDoJobStep == 2)
	{
		if (PLAY_HAND_IDX)
		{
			if (nDstContainer == UOC_GAMESPACE)
			{
				ExecuteScript("ThrowAwayItem()");
				return;
			}
			POINT ptDestPos;
			pItem = (KItem*)(dwItmBaseAdd + PLAY_HAND_IDX*ITM_DATA_SIZE);
			if (!DestRoom.FindRoom(ComAttr.nWidth, ComAttr.nHeight, &ptDestPos))
			{
				SystemMessage("<color=yellow>N¬i chuyÓn ®Õn kh«ng ®ñ chç!!!");
				m_bDoingUtil = 0;
				return;
			}
			g_pCurPlayer->SwitchItem(nDstContainer, ptDestPos.x, ptDestPos.y);
			return;
		}
		BOOL bFound = 0;
		for (int i = 0; i < MAX_PLAYER_ITEM; i++)
		{
			if (PLAYER_ITEM(i).nPlace != nSrcPos) continue;
			pItem = (KItem*)(dwItmBaseAdd + PLAYER_ITEM(i).nIdx*ITM_DATA_SIZE);
			if (nDstContainer == UOC_TRADE_BOX)
			{
				if (pItem->m_nLockCount) continue;
			}
			else if (nDstContainer == UOC_GAMESPACE)
			{
				if (pItem->m_nLockCount) continue;
				if (ComAttr.nSpecialType >= 1) continue;
			}

			if (nItemGenre != item_number)
			{
				if (nItemGenre >= 0 && ComAttr.nItemGenre != nItemGenre) continue;
				if (nItemDetail >= 0)
				{
					if (nItemGenre == item_equip && nItemDetail == (equip_ring + equip_amulet + equip_pendant))
					{
						if (ComAttr.nSpecialType >= 1) continue;
						if (ComAttr.nDetailType != equip_ring &&
							ComAttr.nDetailType != equip_amulet &&
							ComAttr.nDetailType != equip_pendant) continue;
					}
					else if (ComAttr.nDetailType != nItemDetail) continue; 
				}
				if (nItemParticular >= 0 && ComAttr.nParticularType != nItemParticular) continue;
				if (szItemName[0] && !strstr(ComAttr.szItemName, szItemName)) continue;
			}
			bFound = TRUE;
			g_pCurPlayer->SwitchItem(nSrcContainer, PLAYER_ITEM(i).nX, PLAYER_ITEM(i).nY);
		}
		if (!bFound)
		{
			SystemMessage("<color=yellow>Kh«ng cßn ®å cÇn chuyÓn => KÕt thóc!");
			m_bDoingUtil = 0;
		}
	}
}

int CVLUtil::ClickToNpc(const char *szNpcName)
{
	int nPlayerX, nPlayerY, nNpcX, nNpcY, dZ;
	P_PLAYER->GetMapPos(&nPlayerX, &nPlayerY);
	for (int i = 1; i < MAX_NPC; i++)
	{
		if (!g_pNpcArray[i]->IsExist() || g_pNpcArray[i]->m_Kind != kind_dialoger) continue;

		g_pNpcArray[i]->GetMapPos(&nNpcX, &nNpcY);
		if (strcmp(g_pNpcArray[i]->Name, szNpcName)) continue;

		dZ = __distance(nPlayerX, nPlayerY, nNpcX, nNpcY);
		if (dZ > PLAYER_PICKUP_DISTANCE)
		{
			g_pCurPlayer->MoveTo(nNpcX, nNpcY);
			return 1;
		}

		theAttack.SetPeopleIdx(i);
		return i;
	}
	return 0;
}

void CVLUtil::StartGiveKCL()
{
	if (m_bDoingUtil)
	{
		SystemMessage("VLAuto: <color=yellow>HiÖn ®ang ch¹y TiÖn Ých kh¸c!!!");
		return;
	}
	m_bDoingUtil = TRUE;
	m_nUtilTask = util_givekcl;
}

void CVLUtil::GiveKCL()
{
	// GiveKCL in Town
	if (m_nDoJobStep == 0)
	{
		static int nTimeCount = 0;
		if (nTimeCount++ % 6 != 3) return; // make it as 600ms

		//int nSubCityID = theMain.GetSubCity(*g_pMapID);
		//if (theMap.ChangeMap(nSubCityID)) return;
		int nOwnMapID = theMain.GetOwnerCity(*g_pMapID);
		if (theMap.ChangeMap(nOwnMapID)) return;

		if (theMap.RunningToPos(theMap.NpcPoints[npc_Reseved2])) return;
		theMap.m_bFirstSaveRepos = TRUE;
		m_nDoJobStep = 1;
	}

	static int nTimeWait = 5;
	KUiMsgSel *pMsgSel = (KUiMsgSel*)GetPtrAddress(MSG_BASE_ADD);
	if (pMsgSel && pMsgSel->IsVisible())
	{
		if (pMsgSel->m_nNumMessage > 4)
		{
			if (strstr(pMsgSel->m_pMessages[0]->Msg, "Ta ®Õn giao lÖnh bµi"))
				pMsgSel->OnClickMsg(0);
			else
			{
				SystemMessage("<color=yellow>§ang giê c«ng thµnh, lÇn sau giao vËy!");
				pMsgSel->OnClickMsg(pMsgSel->m_nNumMessage);
				m_bDoingUtil = 0;
				theMap.m_bEnableGiveKCL = 0;
			}
		}
		else
		{
			SystemMessage("<color=yellow>KÕt thóc giao [Khiªu chiÕn lÖnh]!");
			pMsgSel->OnClickMsg(pMsgSel->m_nNumMessage);
			m_bDoingUtil = 0;
		}
		nTimeWait = 5;
		return;
	}

	KUiYeSouBox *pYSBox = (KUiYeSouBox*)GetPtrAddress(YSB_BASE_ADD);
	if (pYSBox && pYSBox->IsVisible())
	{
		KItem *pItem;
		KInventory YeSouRoom = PLAYER_ROOM[room_yesou];
		if (PLAY_HAND_IDX)
		{
			pItem = (KItem*)(dwItmBaseAdd + PLAY_HAND_IDX*ITM_DATA_SIZE);
			if (strstr(ComAttr.szItemName, KC_LENH))
			{
				POINT itmPos;
				if (YeSouRoom.FindRoom(1, 1, &itmPos))
					g_pCurPlayer->SwitchItem(UOC_YESOU_BOX, itmPos.x, itmPos.y);
				else
					pYSBox->OnConfirm();
			}
			nTimeWait = 5;
			return;
		}
		//Find the Item
		BOOL bFound = 0;
		for (int i = 0; i < MAX_PLAYER_ITEM; i++)
		{
			if (PLAYER_ITEM(i).nPlace != pos_equiproom) continue;
			pItem = (KItem*)(dwItmBaseAdd + PLAYER_ITEM(i).nIdx*ITM_DATA_SIZE);
			if (strstr(ComAttr.szItemName, KC_LENH))
			{
				bFound = TRUE;
				g_pCurPlayer->SwitchItem(UOC_ITEM_TAKE_WITH, PLAYER_ITEM(i).nX, PLAYER_ITEM(i).nY);
				break;
			}
		}
		if (!bFound)
		{
			SystemMessage("<color=yellow>Kh«ng cßn Khiªu chiÕn lÖnh => KÕt thóc!");
			if (YeSouRoom.m_pArray[0])
				pYSBox->OnConfirm();
			else
				pYSBox->OnConfirm(0);
			CloseAllWindows(0);
			m_bDoingUtil = 0;
		}
		nTimeWait = 5;
		return;
	}
	// Search & Found
	if (nTimeWait)
	{
		if (nTimeWait == 10)
			SystemMessage("<color=yellow>Lag qu¸, chê 3s click l¹i...");
		nTimeWait--;
		return;
	}
	int nResult = ClickToNpc("Xa phu c«ng thµnh");
	if (nResult > 1)
		nTimeWait = 20; // Wait for 3s for next
	else if (nResult < 1)
	{
		SystemMessage("<color=yellow>Kh«ng t×m thÊy Xa phu c«ng thµnh!!!");
		m_bDoingUtil = 0;
	}
}

void CVLUtil::StartOpenGift()
{
	if (m_bDoingUtil)
	{
		SystemMessage("VLAuto: <color=yellow>HiÖn ®ang ch¹y TiÖn Ých kh¸c!!!");
		return;
	}

	int nItemIdx = EQUIP_ROOM.m_pArray[0];
	if (nItemIdx <= 0) return;
	KItem* pItem = (KItem*)(dwItmBaseAdd + nItemIdx*ITM_DATA_SIZE);
	if (ComAttr.nItemGenre == item_special && ComAttr.bStack)
	{
		strcpy(m_szItemName, ComAttr.szItemName);
		SystemMessage(0, "Tù më <color=yellow>[%s]", m_szItemName);
		m_bDoingUtil = TRUE;
		m_nUtilTask = util_opengif;
	}
}

void CVLUtil::OpenGifts()
{
	if (g_pCurPlayer->CheckRoom() >= MAX_ROOM_SPACE)
	{
		SystemMessage("<color=yellow>§· ®µy r­¬ng => KÕt thóc!");
		m_szItemName[0] = 0;
		m_bDoingUtil = 0;
		return;
	}
	if (!g_pCurPlayer->UseItem(m_szItemName))
	{
		SystemMessage(0, "Kh«ng cßn <color=yellow>[%s] <color=red>=> KÕt thóc!", m_szItemName);
		m_szItemName[0] = 0;
		m_bDoingUtil = 0;
	}
}

void CVLUtil::AutoOpenItems()
{
#ifdef VLTK_FREE_MODE
	if (!P_PLAYER->m_FightMode) return; // Do not open in city

	static int nTimeCount = 10;

	if (g_UiInformation2->IsVisible()
		&& strstr(g_UiInformation2->m_szInfoText, "H«m nay ng­¬i ®· sö dông 10,"))
	{
		theCollect.m_bGetCNHK = 0;
		nTimeCount = 36000; // 1 hour later
		g_UiInformation2->Hide();
	}

	if (theMain.ClickMenuStr("Më miÔn phÝ"))
		return;

	if (nTimeCount > 0) { nTimeCount--; return; }
	nTimeCount = 50; // 5 seconds later

	if (g_pCurPlayer->UseItem("CÈm nang hoµng kim"))
		theCollect.m_bGetCNHK = 0;
	else
		theCollect.m_bGetCNHK = TRUE;
#endif
}

BOOL CVLUtil::ArrangeStackItems(const char *szItemName)
{
	KItem *pItem;
	int nItemIdx, nCount = 0, nMaxStack = 0, nMinStack = 1000;
	POINT ptMax = {-1, -1}; POINT ptMin = {-1, -1};
	for (int i = 0; i < MAX_PLAYER_ITEM; i++)
	{
		nItemIdx = PLAYER_ITEM(i).nIdx;
		if (PLAYER_ITEM(i).nPlace != pos_equiproom) continue;
		pItem = (KItem*)(dwItmBaseAdd + nItemIdx*ITM_DATA_SIZE);
		if (!ComAttr.bStack) continue;
		if (!strstr(ComAttr.szItemName, szItemName)) continue;
		if (pItem->m_nCurDur == pItem->m_nMaxDur) continue;
		if (pItem->m_nCurDur > nMaxStack)
		{
			nMaxStack = pItem->m_nCurDur;
			ptMax.x = PLAYER_ITEM(i).nX;
			ptMax.y = PLAYER_ITEM(i).nY;
		}
		if (pItem->m_nCurDur < nMinStack)
		{
			nMinStack = pItem->m_nCurDur;
			ptMin.x = PLAYER_ITEM(i).nX;
			ptMin.y = PLAYER_ITEM(i).nY;
		}
		nCount++;
	}
	if (PLAY_HAND_IDX)
	{
		pItem = (KItem*)(dwItmBaseAdd + PLAY_HAND_IDX*ITM_DATA_SIZE);
		if (!strstr(ComAttr.szItemName, szItemName))
		{
			SystemMessage("VËt phÈm cÇm trªn tay kh¸c lo¹i!!!");
			return 0;
		}
		if (nMaxStack > 0)
			g_pCurPlayer->SwitchItem(UOC_ITEM_TAKE_WITH, ptMax.x, ptMax.y);
		else
		{	// Find a new Pos
			POINT newPos;
			if (EQUIP_ROOM.FindRoom(ComAttr.nWidth, ComAttr.nHeight, &newPos))
			{
				g_pCurPlayer->SwitchItem(UOC_ITEM_TAKE_WITH, newPos.x, newPos.y);
				return TRUE;
			}
			else
				return 0;
		}
		return TRUE;
	}
	if (nCount > 1)
	{
		g_pCurPlayer->SwitchItem(UOC_ITEM_TAKE_WITH, ptMin.x, ptMin.y);
		return TRUE;
	}
	return 0;// Finish
}

BOOL CVLUtil::CheckItem(KItem *pItem)
{
	if (m_szItemName[0])
		if (!strstr(ComAttr.szItemName, m_szItemName)) return 0;
	if (m_nEquipDetail > 1)
	{
		if (ComAttr.nItemGenre != item_equip) return 0;
		if (ComAttr.nDetailType != m_nEquipDetail) return 0;
		if (m_nEquipSex >= 0)
		{
			BOOL bFound = 0;
			for (int j = 7; j < 13; j++)
			{
				if (ItmAttr(j) <= item_begin) continue;
				if (ItmAttr(j) == requiresex)
				{
					if (pItem->m_aryAllAttrib[j].nValue[0] == m_nEquipSex)
						bFound = TRUE;
					break;
				}
			}
			if (!bFound) return 0;
		}
	}

	if (m_nItemSeries >= 0 && m_nItemSeries != ComAttr.nSeries) return 0;
	if (m_nItemLevel > 0 && m_nItemLevel != ComAttr.nLevel) return 0;
	if (m_nItemSpType >= 0 && m_nItemSpType != ComAttr.nSpecialType) return 0;

	// Check Attribute
	if (m_nItemAttrib > 0)
	{
		BOOL bFound = 0;
		for (int j = 13; j < 19; j++)
		{
			if (ItmAttr(j) <= item_begin) continue;
			if (ItmAttr(j) == m_nItemAttrib)
			{
				if (m_nAttLine > 0 && m_nAttLine != j - 12) break;
				if (m_nAttMin > 0 && pItem->m_aryAllAttrib[j].nValue[0] < m_nAttMin) break;
				if (m_nAttMax > 0 && pItem->m_aryAllAttrib[j].nValue[0] > m_nAttMax) break;
				bFound = TRUE;
				break;
			}
		}
		if (!bFound) return 0;
	}

	return TRUE; // Item is OK
}

void CVLUtil::SearhBuyItem()
{
	static int nTimeWait = 0;
	static int nCurIndex = 0;
	static int nLastMoney = 0;
	static BOOL bFoundIt = 0;
	static char szGotItem[80];
	static int nItemPrice = 0;

	int nX, nY, dMin;
	int nPlayerX, nPlayerY, nNpcX, nNpcY, dZ;

	static int nTimeCount = 0;
	if (nTimeCount++ % 6 != 4) return; // make it as 600ms

	if (m_nDoJobStep == 0)
	{
		// Search Room Item
		if (m_bFromR0 || m_bFromR1 || m_bFromR2 || m_bFromR3 || m_bFromR4)
		{
			for (int i = 0; i < MAX_PLAYER_ITEM; i++)
			{
				if (PLAYER_ITEM(i).nPlace != pos_equiproom &&
					PLAYER_ITEM(i).nPlace != pos_repositoryroom &&
					PLAYER_ITEM(i).nPlace != pos_repo02 &&
					PLAYER_ITEM(i).nPlace != pos_repo03 &&
					PLAYER_ITEM(i).nPlace != pos_guid) continue;
				if (PLAYER_ITEM(i).nPlace == pos_equiproom && !m_bFromR0) continue;
				if (PLAYER_ITEM(i).nPlace == pos_repositoryroom && !m_bFromR1) continue;
				if (PLAYER_ITEM(i).nPlace == pos_repo02 && !m_bFromR2) continue;
				if (PLAYER_ITEM(i).nPlace == pos_repo03 && !m_bFromR3) continue;
				if (PLAYER_ITEM(i).nPlace == pos_guid && !m_bFromR4) continue;

				KItem *pItem = (KItem*)(dwItmBaseAdd + PLAYER_ITEM(i).nIdx*ITM_DATA_SIZE);

				if (CheckItem(pItem))
				{
					m_bDoingUtil = 0;
					SystemMessage(0, "T×m thÊy <color=yellow>%s", ComAttr.szItemName);

					char szMsg[64];
					if (PLAYER_ITEM(i).nPlace == pos_equiproom)
						strcpy(szMsg, "CÊt t¹i <color=green>Hµnh trang");
					else if (PLAYER_ITEM(i).nPlace == pos_repositoryroom)
						strcpy(szMsg, "CÊt t¹i <color=green>R­¬ng ®å 1");
					else if (PLAYER_ITEM(i).nPlace == pos_repo02)
						strcpy(szMsg, "CÊt t¹i <color=green>R­¬ng ®å 2");
					else if (PLAYER_ITEM(i).nPlace == pos_repo03)
						strcpy(szMsg, "CÊt t¹i <color=green>R­¬ng ®å 3");
					else if (PLAYER_ITEM(i).nPlace == pos_guid)
						strcpy(szMsg, "CÊt t¹i <color=green>R­¬ng Bang héi");
					SystemMessage(szMsg);

					SystemMessage(0, "VÞ trÝ (cét,dßng) lµ: <color=yellow>(%d,%d)",
						PLAYER_ITEM(i).nX + 1, PLAYER_ITEM(i).nY + 1);
					return;
				}
			}
		}

		memset(m_bVsNpc, 0, sizeof(m_bVsNpc));
		nTimeWait = 0;
		bFoundIt = 0;
		m_nDoJobStep = 1;
	}

	if (!m_bFromPlrShop)
	{
		m_bDoingUtil = 0;
		SystemMessage("<color=yellow>Kh«ng t×m thÊy vËt phÈm!!!");
		return; // Done
	}

	if (bFoundIt)
	{
		bFoundIt = 0;
		if (nLastMoney <= EQUIP_MONEY)
		{
			CloseAllWindows(0);
			nTimeWait = 0;
			return; // Continue
		}
		SystemMessage(0, "Mua ®­îc <color=yellow>%s <color=blue>%d l­îng", szGotItem, nItemPrice);
		CloseAllWindows(0);
		m_bDoingUtil = 0;
		return; // Successful
	}

	KUiPlayerStall *pPlrStall = (KUiPlayerStall*)GetPtrAddress(STL_BASE_ADD);

	if (pPlrStall && pPlrStall->IsVisible())
	{
		POINT itmPos;
		KItem* pItem;
		KStallObject* pItemSt = (KStallObject*)GetPtrAddress(IST_BASE_ADD);
		KShopObject* pItemList = pPlrStall->m_pObjectList;
		if (nCurIndex) m_bVsNpc[nCurIndex] = TRUE;

		for (int i = 0; i < pPlrStall->m_nObjCount; i++)
		{
			pItem = (KItem*)(dwItmBaseAdd + pItemList[i].uId*ITM_DATA_SIZE);
			if (!CheckItem(pItem)) continue;
			if (m_nItemPrice > 0 && pItemSt[i].nPrice > m_nItemPrice) continue;

			// Found it
			if (m_bBuyIt)
			{
				if (pItemSt[i].nPrice > EQUIP_MONEY)
				{
					m_bDoingUtil = 0;
					SystemMessage("<color=yellow>TiÒn mang theo kh«ng ®ñ mua!!!");
					return; // Successful
				}
				// Check Room
				if (!EQUIP_ROOM.FindRoom(ComAttr.nWidth, ComAttr.nHeight, &itmPos))
				{
					m_bDoingUtil = 0;
					SystemMessage("<color=yellow>Hµnh trang kh«ng ®ñ chç trèng!!!");
					return; // Successful
				}
				bFoundIt = TRUE;
				nLastMoney = EQUIP_MONEY;
				strcpy(szGotItem, ComAttr.szItemName);
				nItemPrice = pItemSt[i].nPrice;
				g_pCoreShell->OperationRequest(GOI_TRADE_PLAYER_BUY, pItemList[i].uId, 0);
				return; // Check if Done
			}
			else
			{
				m_bDoingUtil = 0;
				SystemMessage(0, "T×m thÊy <color=yellow>%s", ComAttr.szItemName);
				SystemMessage(0, "VÞ trÝ (cét,dßng) lµ: <color=yellow>(%d,%d)", pItemSt[i].DataX + 1, pItemSt[i].DataY + 1);
				return; // Successful
			}
		}
		CloseAllWindows(0);
		nTimeWait = 0;
		return; // Continue
	}
	if (nTimeWait) { nTimeWait--; return; }

	if (PEOPLE_IDX)
	{
		m_bVsNpc[PEOPLE_IDX] = TRUE;
		theAttack.SetPeopleIdx(0);
	}

	P_PLAYER->GetMapPos(&nPlayerX, &nPlayerY);
	nCurIndex = 0;
	dMin = MAX_INTNUM;
	for (int i = 1; i < MAX_NPC; i++)
	{
		if (!g_pNpcArray[i]->IsExist() || g_pNpcArray[i]->m_Kind != kind_player) continue;
		if (!g_pNpcArray[i]->m_bHaveStall || g_pNpcArray[i]->m_Doing != do_sit) continue;
		if (m_bVsNpc[i]) continue; // Already Visited.
		g_pNpcArray[i]->GetMapPos(&nNpcX, &nNpcY);
		dZ = __distance(nPlayerX, nPlayerY, nNpcX, nNpcY);
		if (dZ < dMin)
		{
			dMin = dZ;
			nCurIndex = i;
			nX = nNpcX;
			nY = nNpcY;
		}
	}
	if (!nCurIndex)
	{
		m_bDoingUtil = 0;
		SystemMessage("<color=yellow>Kh«ng t×m thÊy vËt cÇn mua!!!");
		return; // Done
	}

	theAttack.SetPeopleIdx(nCurIndex);

	nTimeWait = 10;
	return; // Continue
}

void CVLUtil::StopAllUtils()
{
	m_bDoingUtil = 0;
	SystemMessage("<color=yellow>Ng­ng c¸c ho¹t ®éng TiÖn Ých!!!");
}

// Bang hoi Utilities
void CVLUtil::StartGetLB(int nMaxLBCount)
{
	if (m_bDoingUtil)
	{
		SystemMessage("VLAuto: <color=yellow>HiÖn ®ang ch¹y TiÖn Ých kh¸c!!!");
		return;
	}
	m_bDoingUtil = TRUE;
	m_nUtilTask = util_getlenhbai;
	m_nMaxLBCount = nMaxLBCount;
}

void CVLUtil::DoEventGetLB()
{
	static int nEventItem = -1;
	static int nTimeWait = 5;

	if (g_pCurPlayer->CheckRoom() >= MAX_ROOM_SPACE)
	{
		SystemMessage("<color=yellow>§· ®µy r­¬ng => KÕt thóc!");
		nEventItem = -1;
		m_bDoingUtil = 0;
		return;
	}

	KUiMsgSel *pMsgSel = (KUiMsgSel*)GetPtrAddress(MSG_BASE_ADD);
	if (pMsgSel && pMsgSel->IsVisible())
	{
		nTimeWait = 15;
		char *szMsg0 = pMsgSel->m_pMessages[0]->Msg;
		if (strstr(szMsg0, "NhËn nhiÖm vô ngÉu nhiªn"))
			pMsgSel->OnClickMsg(0);
		else if (strstr(szMsg0, "ta muèn ®æi"))
		{
			nEventItem = g_pCurPlayer->GetItemsCount(item_special, 2, 1020, -1, 0);
			pMsgSel->OnClickMsg(0);
			nTimeWait = 1;
		}
		else if (strstr(pMsgSel->m_InfoText, "Kh«ng ®­îc råi"))
		{
			SystemMessage("<color=yellow>HÕt ®iÓm cèng hiÕn => KÕt thóc!");
			nEventItem = -1;
			m_bDoingUtil = 0;
			pMsgSel->OnClickMsg(0);
		}
		else if (strstr(szMsg0, "KÕt thóc ®èi tho¹i"))
		{
			if (strstr(pMsgSel->m_InfoText, "NhiÖm vô ngÉu nhiªn h«m nay ®· kÕt thóc"))
				SystemMessage("<color=yellow>§· ph©n ph¸t hÕt LB => KÕt thóc!");
			else
				SystemMessage("<color=yellow>Kh«ng ®óng Bang m×nh, hÑn lÇn sau!");
			nEventItem = -1;
			m_bDoingUtil = 0;
			pMsgSel->OnClickMsg(0);
		}
		return;
	}

	// Check quantity
	int nCurItemCount = g_pCurPlayer->GetItemsCount(item_special, 2, 1020, -1, 0);
	if (nCurItemCount == nEventItem) return; // Wait for LB
	nEventItem = -1;

	if (nCurItemCount >= m_nMaxLBCount)
	{
		SystemMessage("<color=yellow>§· nhËn ®ñ sè l­îng => KÕt thóc!");
		nEventItem = -1;
		m_bDoingUtil = 0;
		return;
	}

	if (nTimeWait)
	{
		if (nTimeWait == 20)
			SystemMessage("<color=yellow>Lag qu¸, chê 3s click l¹i...");
		nTimeWait--;
		return;
	}

	// Sell unnecessary
	if (theQuest.SellUnusedLB())
	{
		nTimeWait = 1;
		return;
	}

	int nResult = ClickToNpc("Tæng qu¶n Thiªn ý ph­êng");
	if (nResult > 1)
		nTimeWait = 30; // Wait for 3s for next
	else if (nResult < 1)
	{
		SystemMessage("<color=yellow>Kh«ng t×m thÊy Tæng qu¶n Thiªn ý ph­êng!!!");
		nEventItem = -1;
		m_bDoingUtil = 0;
	}
}
