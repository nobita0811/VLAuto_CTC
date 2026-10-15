#include "stdafx.h"
#include "../GameDef.h"
#include "GameBase.h"
#include "JxCore.h"
#include "S3Client.h"
#include "VLClasses.h"

#define TONGPK_KIND 7
const char TongPKKind[TONGPK_KIND][12] =
{ "Binh gi¸p", "Thiªn c«ng", "DÞ dung", "ThÝ luyÖn", "Thiªn ý", "LÔ phÈm", "Ho¹t ®éng" };

void CVLQuest::BreathBH()
{
	static char szLastMap[32] = "";
	static int nTimeWait = 10;

	if ((!g_pCurPlayer->m_cTong.m_dwTongNameID)
		|| ((m_nTongItem != act_auto) && (m_nTongKill != act_auto)))
	{
		m_nGenDoing = qt_yesou;
		m_nDoQuestStep = 0;
		return;
	}

	// Check Delay
	int nLastTask = g_pCurPlayer->GetTaskVal(bh_lsttask);
	if (nLastTask == 0) // waiting
	{
		if (strcmp(szLastMap, g_szMapName))
		{
			if (nTimeWait) { nTimeWait--; return; }
			nTimeWait = 10;
			strcpy(szLastMap, g_szMapName);
		}
	}
	else
	{
		nTimeWait = 10;
		strcpy(szLastMap, "");
	}

	int nIdx, nTemp;
	int nRet = 0;
	int nBHCurTask = g_pCurPlayer->GetTaskVal(bh_curtask);

	switch (m_nBHDoing)
	{
	case bh_none:
		if (BHPreparing()) return;

		if (m_nTongItem == act_auto)
		{
			if (nBHCurTask > 200 && nBHCurTask < 300)
			{
				m_nBHDoing = bh_doing;
				m_nDoQuestStep = 0;
			}
			else
				m_nBHDoing = bh_searching;
		}
		else
			m_nBHDoing = bh_suspending;
		break;

	case bh_doing:
		nRet = CheckingBHTask(nBHCurTask);
		if (!nRet) break; // Continue...
		if (nRet == 1) // Failed
			m_nBHDoing = bh_searching;
		else if (nRet == 2) // Return quest
			m_nBHDoing = bh_return;
		m_nDoQuestStep = 0;
		theMap.ResetShoping();
		break;

	case bh_searching:
		nIdx = 0;
		m_nNewCollTask = 0;
		memset(m_CollTasksArray, 0, sizeof(m_CollTasksArray));
		for (int i = 0; i < MAX_PLAYER_ITEM; i++)
		{
			KItem *pItem = (KItem*)(dwItmBaseAdd + PLAYER_ITEM(i).nIdx*ITM_DATA_SIZE);
			if (ComAttr.nItemGenre != item_special) continue;
			if (ComAttr.nDetailType != 2) continue;
			if (ComAttr.nParticularType != 1020) continue;
			int nLBTaskID = pItem->m_MagicLevel[0];
			if (nLBTaskID < 200 || nLBTaskID > 300) continue;
			// FOUND a new task
			int nRet = CheckingBHTask(nLBTaskID, TRUE);
			if (nRet == 1) continue; // Failed to do this task
			// PUSH to Array
			if (nIdx >= MAX_COLL_QUEST) break;
			m_CollTasksArray[nIdx++] = nLBTaskID;
		}
		if (nIdx > 0)
		{
			// Arrange the Array
			for (int i = 0; i < nIdx; i++)
				for (int j=i+1; j<nIdx; j++)
					if (m_CollTasksArray[i] > m_CollTasksArray[j])
					{
						nTemp = m_CollTasksArray[j];
						m_CollTasksArray[j] = m_CollTasksArray[i];
						m_CollTasksArray[i] = nTemp;
					}
			m_nNewCollTask = m_CollTasksArray[0];
			m_nBHDoing = bh_getting;
			m_nDoQuestStep = 0;
			theMap.ResetShoping();
		}
		else
		{
			SaveYSLog("Kh«ng cßn LB ngÉu nhiªn cã thÓ lµm!");
			SystemMessage("<color=yellow>Kh«ng cßn LB ngÉu nhiªn cã thÓ lµm!");
			m_nBHDoing = bh_suspending;
		}
		break;
	
	case bh_getting:
		nRet = AutoUseLB();
		if (nRet == 0) return; // Continue
		m_nNewCollTask = 0;
		if (nRet < 0)
			m_nBHDoing = bh_suspending;
		else
		{
			m_nBHDoing = bh_doing;
			m_nDoQuestStep = 0;
			theMap.ResetShoping();
		}
		break;

	case bh_return:
		if (nBHCurTask > 200 && nBHCurTask < 300)
			ReturnBHTask(nBHCurTask);
		else if (nLastTask)
		{
			m_nBHDoing = bh_searching;
			m_nDoQuestStep = 0;
			theMap.ResetShoping();
		}
		break;

	case bh_suspending:
		SaveYSLog("Ng­ng nhiÖm vô LB ngÉu nhiªn, lµm nhiÖm vô D· TÈu!\n");
		SystemMessage("<color=yellow>Ng­ng nvô LB ngÉu nhiªn, lµm nvô D· TÈu!");
		m_nGenDoing = qt_yesou;
		m_nDoQuestStep = 0;
		theMap.ResetShoping();
		break;
	}
}

BOOL CVLQuest::BHPreparing()
{
	if (m_nDoQuestStep == 0) // Check LB
	{
		if (!m_TongQOpt.m_bGetLBMin) return 0; // Don't auto get LB
		if (g_pCurPlayer->m_cTong.m_nContributePoint < 400)
		{
			SystemMessage("<color=yellow>Kh«ng ®ñ ®iÓm cèng hiÕn!");
			return 0; // Het diem CH
		}
		if (g_pCurPlayer->GetItemsCount(item_special, 2, 1020, -1, 0) >= m_TongQOpt.m_nGetLBMin) return 0;
		SaveYSLog("§i nhËn LÖnh bµi nhiÖm vô Bang héi.");
		SystemMessage("<color=yellow>§i nhËn LÖnh bµi nhiÖm vô Bang héi...");

		g_pCurPlayer->EnterTongMap(1);
		m_nDoQuestStep = 1;
		return TRUE;
	}
	if (m_nDoQuestStep == 1) // Get LB
	{
		// Enter the TongMap
		if (!g_pCurPlayer->EnterTongMap(2))
			return TRUE;

		// Runing to "Mr. Tong quan"
		if (!theMap.m_bDoShoping && theMap.RunningToPos(theMap.NpcPoints[npc_ThienY])) return TRUE;
		theMap.m_bDoShoping = TRUE;

		m_nDoQuestStep = 2;

		return TRUE;
	}
	if (m_nDoQuestStep == 2)
	{
		if (m_TongQOpt.m_bGetLBMax)
			theUtil.StartGetLB(m_TongQOpt.m_nGetLBMax);
		else
			theUtil.StartGetLB();
		m_nDoQuestStep = 3;
		return TRUE;
	}
	if (m_nDoQuestStep == 3) // Waiting for finish
	{
		theMap.ResetShoping();
		m_nDoQuestStep = 4;
		return TRUE;
	}

	return 0;
}

BOOL CVLQuest::SellUnusedLB()
{
	for (int i = 0; i < MAX_PLAYER_ITEM; i++)
	{
		if (PLAYER_ITEM(i).nPlace != pos_equiproom) continue;
		KItem *pItem = (KItem*)(dwItmBaseAdd + PLAYER_ITEM(i).nIdx*ITM_DATA_SIZE);
		if (ComAttr.nItemGenre != item_special) continue;
		if (ComAttr.nDetailType != 2) continue;
		if (ComAttr.nParticularType != 1020) continue;

		int nLBTaskID = pItem->m_MagicLevel[0];
		if (nLBTaskID <= 0) continue;

		BOOL bSellIt = 0;
		if (nLBTaskID > 200 && nLBTaskID < 300)
		{
			int nTaskIdx = pTask07[nLBTaskID-201].nTaskIndex;
			if (nTaskIdx >= 0)
				bSellIt = m_TongQOpt.m_CollQPass[nTaskIdx];
		}
		else if (nLBTaskID > 300 && nLBTaskID < 400)
		{
			int nTaskIdx = pTask08[nLBTaskID-301].nTaskIndex;
			if (nTaskIdx >= 0)
				bSellIt = m_TongQOpt.m_KillQPass[nTaskIdx];
		}
		if (bSellIt)
		{
			g_pCurPlayer->SellItem(PLAYER_ITEM(i).nIdx);
			return TRUE;
		}
	}
	return 0; // No more LB to be sold
}

int CVLQuest::CheckingBHTask(int nTaskID, BOOL bJustTry)
{
	m_nRqGenre = pTask07[nTaskID-201].nItemGenre;
	m_nRqDetail = pTask07[nTaskID-201].nDetail;
	m_nRqParticular = pTask07[nTaskID-201].nParticular;
	m_nRqLevel = pTask07[nTaskID-201].nLevel;
	m_nRqSeries = pTask07[nTaskID-201].nSeries;
	m_nRqQuantity = pTask07[nTaskID-201].nQuantity;

	int nTotal = g_pCurPlayer->GetItemsCount(m_nRqGenre, m_nRqDetail, m_nRqParticular, m_nRqSeries, 0);

	if (nTotal < m_nRqQuantity)
	{
		int nCurFY = g_pCurPlayer->GetTaskVal(0x0097);
		int nPricePD = pTask07[nTaskID-201].nPricePD;
		BOOL bCanBuy = (nPricePD > 0);
		bCanBuy &= (m_TongQOpt.m_bBuyTBTN && m_TongQOpt.m_nBuyTBTN > nPricePD);
		bCanBuy &= (nCurFY >= m_nRqQuantity*nPricePD);

		if (bCanBuy) // Can_Buy
		{
			if (!bJustTry)
			{
				if (!GoBuySpItem())
					return 1; // Quest Failed
			}
			return 0; // Continue...
		}
		return 1; // Quest Failed
	}

	if (m_nDoQuestStep < 10)
	{
		m_nDoQuestStep = 10;
		theMap.ResetShoping();
	}

	if (m_nDoQuestStep == 10)
	{
		int nEquip = g_pCurPlayer->GetItemsCount(m_nRqGenre, m_nRqDetail, m_nRqParticular, m_nRqSeries);
		if (nEquip < m_nRqQuantity)
			m_nDoQuestStep = 11;
		else
			m_nDoQuestStep = 12; // DONE
	}
	if (m_nDoQuestStep == 11)
	{
		if (!bJustTry)
		{
			if (MoveSpItemToEquip())
				return 0;
			m_nDoQuestStep = 10;
		}
		return 0; // Continue...
	}

	return 2; // DONE
}

int CVLQuest::AutoUseLB(int nTrainMapID)
{
	static int nTimeWait = 1;
	static BOOL bFoundIt = 0;
	static PlayerItem theItem = { -1, -1, -1, -1, -1 }; // LB Item

	//Mçi ngµy mçi ng­êi ch¬i chØ ®­îc
	if (g_UiInformation2->IsVisible()
		&& strstr(g_UiInformation2->m_szInfoText, "Mçi ngµy mçi ng­êi ch¬i chØ ®­îc"))
	{
		SaveYSLog("§· hoµn thµnh 100 LB ngÉu nhiªn!\n");
		SystemMessage("<color=yellow>§· hoµn thµnh 100 LB ngÉu nhiªn!");
		g_UiInformation2->Hide();
		m_bIsFull100LB = TRUE;
		return -1;
	}

	KUiMsgSel2 *pMsgSel2 = (KUiMsgSel2*)GetPtrAddress(MSG2_BASE_ADD);
	if (pMsgSel2 && pMsgSel2->IsVisible())
	{
		nTimeWait = 2;
		char *szMsg0 = pMsgSel2->m_pMessages[0]->Msg;
		if (strstr(szMsg0, "TiÕp nhËn nhiÖm vô"))
			pMsgSel2->OnClickMsg(0);
		else
			pMsgSel2->OnClickMsg(pMsgSel2->m_nNumMessage);
		return 0;
	}

	KUiMsgSel *pMsgSel = (KUiMsgSel*)GetPtrAddress(MSG_BASE_ADD);
	if (pMsgSel && pMsgSel->IsVisible())
	{
		char *szMsg0 = pMsgSel->m_pMessages[0]->Msg;
		if (strstr(szMsg0, "§óng vËy, ta muèn hñy bá nhiÖm vô"))
			pMsgSel->OnClickMsg(0);
		else
			pMsgSel->OnClickMsg(pMsgSel->m_nNumMessage);
		return 0;
	}

	if (nTimeWait) { nTimeWait--; return 0; }

	if (bFoundIt)
	{
		if (theItem.nPlace == pos_equiproom)
		{
			char szItemDesc[1024];
			KItemObject useObj;
			useObj.uId = theItem.nIdx;
			useObj.uGenre = CGOG_ITEM;
			useObj.DataX = theItem.nX;
			useObj.DataY = theItem.nY;

			g_pCoreShell->GetGameData(GDI_GAME_OBJ_DESC, (int)&useObj, (int)szItemDesc);
			char *szTaskDes = strstr(szItemDesc, "<color=yellow>");
			if (szTaskDes)
			{
				szTaskDes += 14;
				char *szEnd = strstr(szTaskDes, "<color");
				if (szEnd) *szEnd = 0;
				sprintf(szItemDesc, "NhiÖm vô LB ngÉu nhiªn: %s", szTaskDes);
				SaveYSLog(szItemDesc);
			}

			g_pCoreShell->OperationRequest(GOI_USE_ITEM, (int)&useObj, UOC_ITEM_TAKE_WITH);
			nTimeWait = 5;
		}
		else
		{
			if (!GetItemFromRepo(&theItem)) return 0;
		}
		memset(&theItem, -1, sizeof(theItem));
		bFoundIt = 0;
		return 0;
	}

	int nBHCurTask = g_pCurPlayer->GetTaskVal(bh_curtask);

	if (nTrainMapID)
	{
		if (nBHCurTask > 300 && nBHCurTask < 400)
		{
			int nBHCurMapID = pTask08[nBHCurTask-301].nMapID;
			if (nBHCurMapID == nTrainMapID) return 1; // Doing right
		}
		bFoundIt = GetTongTaskLB(&theItem, pos_equiproom, nTrainMapID);
		if (!bFoundIt)
		{
			bFoundIt = GetTongTaskLB(&theItem, 0, nTrainMapID);
			if (!bFoundIt) return -1;
		}
	}
	else
	{
		if (nBHCurTask > 200 && nBHCurTask < 300)
		{
			if (nBHCurTask == m_nNewCollTask) return 1; // Doing right
		}
		bFoundIt = GetTongTaskLB(&theItem, pos_equiproom);
		if (!bFoundIt)
		{
			bFoundIt = GetTongTaskLB(&theItem);
			if (!bFoundIt) return -1;
		}
	}

	return 0;
}

BOOL CVLQuest::GetTongTaskLB(PlayerItem *pPlayerItem, int nPlace, int nTrainMapID)
{
	memset(pPlayerItem, -1, sizeof(PlayerItem));

	for (int i = 0; i < MAX_PLAYER_ITEM; i++)
	{
		if (nPlace)
			if (PLAYER_ITEM(i).nPlace != nPlace) continue;

		KItem *pItem = (KItem*)(dwItmBaseAdd + PLAYER_ITEM(i).nIdx*ITM_DATA_SIZE);
		if (ComAttr.nItemGenre != item_special) continue;
		if (ComAttr.nDetailType != 2) continue;
		if (ComAttr.nParticularType != 1020) continue;
		int nLBTaskID = pItem->m_MagicLevel[0];
		if (nTrainMapID)	// Kill Quest
		{
			if (nLBTaskID < 301 || nLBTaskID > 400) continue;
			if (nTrainMapID == pTask08[nLBTaskID-301].nMapID)
			{ // Found it
				*pPlayerItem = PLAYER_ITEM(i);
				return TRUE;
			}
		}
		else				// Coll Quest (anyone)
		{
			if (nLBTaskID < 201 || nLBTaskID > 300) continue;
			if (nLBTaskID == m_nNewCollTask)
			{	// Found it
				*pPlayerItem = PLAYER_ITEM(i);
				return TRUE;
			}
		}
	}

	return 0;
}

BOOL CVLQuest::GoBuySpItem()
{
	KUiShop *pShop;
	KUiMsgSel *pMsgSel;

	switch (m_nDoQuestStep)
	{
	case 0:
		if (theMap.ChangeMap(city_tuongduong)) return TRUE;
		SystemMessage("<color=yellow>Mua vËt phÈm t¹i ThÇn bÝ th­¬ng nh©n...");
		SaveYSLog("§i mua vËt phÈm t¹i ThÇn bÝ th­¬ng nh©n...");
		m_nDoQuestStep = 1;
		break;
	case 1:
		if (theMap.RunningToPos(theMap.NpcPoints[npc_Reseved1])) return TRUE;
		m_nDoQuestStep = 2;
		break;
	case 2:
		pShop = (KUiShop*)GetPtrAddress(SHOP_BASE_ADD);
		pMsgSel = (KUiMsgSel*)GetPtrAddress(MSG_BASE_ADD);
		if (pMsgSel && pMsgSel->IsVisible())
		{
			if (pShop)
			{
				pMsgSel->OnClickMsg(pMsgSel->m_nNumMessage);
				return TRUE;
			}
			for (int i = 0; i < pMsgSel->m_nNumMessage; i++)
			{
				char *szMsg = (char*)pMsgSel->m_pMessages[i]->Msg;
				if (strstr(szMsg, AGREE_TRADE))
				{
					pMsgSel->OnClickMsg(i);
					break;
				}
			}
			return TRUE;
		}
		if (pShop) //Do Shoping
		{
			// Full Room
			if (g_pCurPlayer->CheckRoom() == MAX_ROOM_SPACE)
				return 0; // FAILED
			// Buy it
			int nItemIdx = -1;
			KItemObject *pItemList = pShop->m_pObjectList;
			for (int i = 0; i < pShop->m_nObjCount; i++)
			{
				int nIndex = g_pBuySell->GetItemIndex(g_pCurPlayer->m_nCurShop, pItemList[i].uId);
				KItem *pItem = g_pBuySell->GetItem(nIndex);
				if (ComAttr.nItemGenre != m_nRqGenre) continue;
				if (ComAttr.nDetailType != m_nRqDetail) continue;
				if (ComAttr.nParticularType != m_nRqParticular) continue;
				if (m_nRqLevel >= 0 && ComAttr.nLevel != m_nRqLevel) continue;
				if (m_nRqSeries >= 0 && ComAttr.nSeries != m_nRqSeries) continue;
				// FOUND IT
				nItemIdx = i;
				break;
			}
			if (nItemIdx < 0) return 0;
			POINT itmPos;
			if (EQUIP_ROOM.FindRoom(pShop->m_pObjectList[nItemIdx].DataW, pShop->m_pObjectList[nItemIdx].DataH, &itmPos))
				g_pCurPlayer->BuyItem(nItemIdx);
			else
			{
				SystemMessage("<color=yellow>Hµnh trang kh«ng ®ñ chç trèng!");
				return 0;
			}
			return TRUE;
		}

		theMap.ClickToNpc(theMap.NpcPoints[npc_Reseved1].x, theMap.NpcPoints[npc_Reseved1].y);
		break;
	}

	return TRUE;
}

BOOL CVLQuest::MoveSpItemToEquip()
{
	static BOOL bFoundIt = 0;
	static PlayerItem theItem = { -1, -1, -1, -1, -1 }; // Required Item

	if (bFoundIt)
	{
		if (!GetItemFromRepo(&theItem)) return TRUE;
		bFoundIt = 0;
		return 0;
	}
	for (int i = 0; i < MAX_PLAYER_ITEM; i++)
	{
		if (PLAYER_ITEM(i).nPlace == pos_equiproom) continue;
		KItem *pItem = (KItem*)(dwItmBaseAdd + PLAYER_ITEM(i).nIdx*ITM_DATA_SIZE);
		if (ComAttr.nItemGenre != m_nRqGenre) continue;
		if (ComAttr.nDetailType != m_nRqDetail) continue;
		if (ComAttr.nParticularType != m_nRqParticular) continue;
		if (m_nRqLevel >= 0 && ComAttr.nLevel != m_nRqLevel) continue;
		if (m_nRqSeries >= 0 && ComAttr.nSeries != m_nRqSeries) continue;
		theItem = PLAYER_ITEM(i);
		bFoundIt = TRUE;
		return TRUE;
	}

	return 0;
}

void CVLQuest::ReturnBHTask(int nTaskID)
{
	int nCount = 0;
	int nQMapID = pTask07[nTaskID - 201].nMapID;
	int nOwnID = theMain.GetOwnerCity(nQMapID);
	POINT ptTalkNpc = { pTask07[nTaskID - 201].nNpcX, pTask07[nTaskID - 201].nNpcY };

	KUiYeSouBox *pYSBox;
	KUiMsgSel *pMsgSel;
	KUiMsgSel2 *pMsgSel2;

	switch (m_nDoQuestStep)
	{
	case 0:
		if (nQMapID == (*g_pMapID))
		{
			m_nDoQuestStep = 2;
			return;
		}
		if (theMap.ChangeMap(nOwnID)) return;
		m_nDoQuestStep = 1;
		break;

	case 1:
		if (theMap.ChangeMap(nQMapID)) return;
		m_nDoQuestStep = 2;
		break;

	case 2:
		if (theMap.RunningToPos(ptTalkNpc)) return;
		m_nDoQuestStep = 3;
		break;

	case 3:	// Split Items
		if (PLAY_HAND_IDX)
		{
			POINT itmPos;
			KItem *pItem = (KItem*)(dwItmBaseAdd + PLAY_HAND_IDX*ITM_DATA_SIZE);
			if (EQUIP_ROOM.FindRoom(ComAttr.nWidth, ComAttr.nHeight, &itmPos))
				g_pCurPlayer->SwitchItem(UOC_ITEM_TAKE_WITH, itmPos.x, itmPos.y);
			else
			{
				SystemMessage("<color=yellow>Hµnh trang kh«ng ®ñ chç trèng!");
				SaveYSLog("Hµnh trang kh«ng ®ñ chç trèng => T¹m ng­ng!");
				m_nBHDoing = bh_suspending;
				m_nDoQuestStep = 0;
			}
			return;
		}
		nCount = 0;
		for (int i = 0; i < MAX_PLAYER_ITEM; i++)
		{
			if (PLAYER_ITEM(i).nPlace != pos_equiproom) continue;
			KItem *pItem = (KItem*)(dwItmBaseAdd + PLAYER_ITEM(i).nIdx*ITM_DATA_SIZE);
			if (ComAttr.nItemGenre != m_nRqGenre) continue;
			if (ComAttr.nDetailType != m_nRqDetail) continue;
			if (ComAttr.nParticularType != m_nRqParticular) continue;
			if (m_nRqLevel >= 0 && ComAttr.nLevel != m_nRqLevel) continue;
			if (m_nRqSeries >= 0 && ComAttr.nSeries != m_nRqSeries) continue;
			// FOUND IT
			if (!ComAttr.bStack)
				nCount ++;
			else if (pItem->m_nCurDur == 1)
				nCount ++;
		}
		if (nCount >= m_nRqQuantity)
		{
			m_nDoQuestStep = 4;
			return;
		}
		for (int i = 0; i < MAX_PLAYER_ITEM; i++)
		{
			if (PLAYER_ITEM(i).nPlace != pos_equiproom) continue;
			KItem *pItem = (KItem*)(dwItmBaseAdd + PLAYER_ITEM(i).nIdx*ITM_DATA_SIZE);
			if (ComAttr.nItemGenre != m_nRqGenre) continue;
			if (ComAttr.nDetailType != m_nRqDetail) continue;
			if (ComAttr.nParticularType != m_nRqParticular) continue;
			if (m_nRqLevel >= 0 && ComAttr.nLevel != m_nRqLevel) continue;
			if (m_nRqSeries >= 0 && ComAttr.nSeries != m_nRqSeries) continue;
			// FOUND IT
			if (!ComAttr.bStack)
			{
				SystemMessage("<color=yellow>VËt phÈm nhiÖm vô kh«ng ®óng!");
				SaveYSLog("VËt phÈm nhiÖm vô kh«ng ®óng => T¹m ng­ng!");
				m_nBHDoing = bh_suspending;
				m_nDoQuestStep = 0;
				break;
			}
			else if (pItem->m_nCurDur > 1)
			{
				g_pCoreShell->OperationRequest(GOI_SHIFT_ITEM, PLAYER_ITEM(i).nIdx, 1);
				break;
			}
		}
		break;

	case 4: // Talk to NPC
		pYSBox = (KUiYeSouBox*)GetPtrAddress(YSB_BASE_ADD);
		if (pYSBox && pYSBox->IsVisible())
		{
			if (PLAY_HAND_IDX)
			{
				POINT itmPos;
				KInventory YeSouRoom = PLAYER_ROOM[room_yesou];
				if (YeSouRoom.FindRoom(1, 1, &itmPos))
					g_pCurPlayer->SwitchItem(UOC_YESOU_BOX, itmPos.x, itmPos.y);
				return;
			}
			int nTotal = g_pCurPlayer->GetItemsCount(m_nRqGenre, m_nRqDetail, m_nRqParticular, m_nRqSeries, pos_yesoubox);
			if (nTotal == m_nRqQuantity)
			{
				SaveYSLog("Hoµn thµnh nhiÖm vô LB ngÉu nhiªn!");
				pYSBox->OnConfirm();
				return;
			}
			//Find the Item
			for (int i = 0; i < MAX_PLAYER_ITEM; i++)
			{
				if (PLAYER_ITEM(i).nPlace != pos_equiproom) continue;
				KItem *pItem = (KItem*)(dwItmBaseAdd + PLAYER_ITEM(i).nIdx*ITM_DATA_SIZE);
				if (ComAttr.nItemGenre != m_nRqGenre) continue;
				if (ComAttr.nDetailType != m_nRqDetail) continue;
				if (ComAttr.nParticularType != m_nRqParticular) continue;
				if (m_nRqLevel >= 0 && ComAttr.nLevel != m_nRqLevel) continue;
				if (m_nRqSeries >= 0 && ComAttr.nSeries != m_nRqSeries) continue;
				// FOUND IT
				if (!ComAttr.bStack || (ComAttr.bStack && pItem->m_nCurDur == 1))
				{
					g_pCurPlayer->SwitchItem(UOC_ITEM_TAKE_WITH, PLAYER_ITEM(i).nX, PLAYER_ITEM(i).nY);
					break;
				}
			}
			return;
		}

		pMsgSel = (KUiMsgSel*)GetPtrAddress(MSG_BASE_ADD);
		if (pMsgSel && pMsgSel->IsVisible())
		{
			theMap.m_nTimeWait = 2;
			char *szMsg0 = pMsgSel->m_pMessages[0]->Msg;
			if (strstr(szMsg0, "NhiÖm vô b¹n ®ång hµnh"))
				pMsgSel->OnClickMsg(0);
			else if (strstr(szMsg0, "Hoµn thµnh nhiÖm vô"))
				pMsgSel->OnClickMsg(0);
			else
				pMsgSel->OnClickMsg(pMsgSel->m_nNumMessage);
			return;
		}

		pMsgSel2 = (KUiMsgSel2*)GetPtrAddress(MSG2_BASE_ADD);
		if (pMsgSel2 && pMsgSel2->IsVisible())
		{
			theMap.m_nTimeWait = 2;
			char *szMsg0 = pMsgSel2->m_pMessages[0]->Msg;
			if (strstr(szMsg0, "Hoµn thµnh nhiÖm vô"))
				pMsgSel2->OnClickMsg(0);
			else if (strstr(szMsg0, "NhËn l·nh phÇn th­ëng"))
				pMsgSel2->OnClickMsg(0);
			else if (strstr(szMsg0, "KÕt thóc ®èi tho¹i"))
				pMsgSel2->OnClickMsg(0);
			else
				pMsgSel2->OnClickMsg(pMsgSel2->m_nNumMessage);
			return;
		}

		theMap.ClickToNpc(ptTalkNpc.x, ptTalkNpc.y);
		break;
	}
}


// TONG PK QUEST
void CVLQuest::BreathPK()
{
	if (m_nDoQuestStep < 4 && g_pCurPlayer->GetTaskVal(bh_pkqty))
		m_nDoQuestStep = 4; // Already have Task
	if (m_nDoQuestStep < 3 && g_pCurPlayer->UseItem("LÖnh bµi nhiÖm vô"))
		m_nDoQuestStep = 3; // Already have LB
	if (m_nDoQuestStep >= 7 && m_nDoQuestStep < 20)
	{
		if (g_pCurPlayer->m_nPKValue)
		{
			SaveYSLog("Tr¶ nhiÖm vô LB ®Æc biÖt!");
			SystemMessage("<color=yellow>Tr¶ nhiÖm vô LB ®Æc biÖt!");
			g_pCurPlayer->EnterTongMap(1);
			m_nDoQuestStep = 20; // Tra nhiem vu
			m_bNeedGoTrain = 0;
			m_bGoTrainGetPK = 0;
		}
		else
			DoQuestPK(1);
		return;
	}

	KUiMsgSel *pMsgSel = (KUiMsgSel*)GetPtrAddress(MSG_BASE_ADD);
	int nRet = 0;

	switch (m_nDoQuestStep)
	{
	case 0: // Vao Ban do bang
		if (g_pCurPlayer->m_cTong.m_nContributePoint < 200)
		{
			SaveYSLog("Kh«ng ®ñ ®iÓm cèng hiÕn!\n");
			SystemMessage("<color=yellow>Kh«ng ®ñ ®iÓm cèng hiÕn!");
			m_nDoQuestStep = 40;
			break;
		}
		if (g_pCurPlayer->CheckRoom() == MAX_ROOM_SPACE)
		{
			SaveYSLog("§· ®µy r­¬ng => KÕt thóc!\n");
			SystemMessage("<color=yellow>§· ®µy r­¬ng => KÕt thóc!");
			m_nDoQuestStep = 40;
		}
		SaveYSLog("NhËn nhiÖm vô LB ®Æc biÖt!");
		SystemMessage("<color=yellow>NhËn nhiÖm vô LB ®Æc biÖt!");
		g_pCurPlayer->EnterTongMap(1);
		m_nDoQuestStep = 1;
		break;

	case 1: // Di nhan nhiem vu
		if (!g_pCurPlayer->EnterTongMap(2))
			return;

		if (!theMap.m_bDoShoping && theMap.RunningToPos(theMap.NpcPoints[npc_ThienY])) return;
		theMap.m_bDoShoping = TRUE;

		if (pMsgSel && pMsgSel->IsVisible())
		{
			theMap.m_nTimeWait = 3;
			char *szMsg0 = pMsgSel->m_pMessages[0]->Msg;
			if (strstr(szMsg0, "NhËn nhiÖm vô ngÉu nhiªn"))
				pMsgSel->OnClickMsg(1);
			else if (strstr(szMsg0, "T×m hiÓu LÖnh bµi nhiÖm vô ®Æc biÖt"))
				pMsgSel->OnClickMsg(1);
			else if (strstr(szMsg0, "§æi lÖnh bµi cÊp 1"))
			{
				int nMaxLBKind = 3;
				if (m_TongPKOpt.m_nChooseLB < 1)
				{
					nMaxLBKind = (g_pCurPlayer->m_nPKValue + 1) / 2;
					if (nMaxLBKind > 3) nMaxLBKind = 3;
				}
				else
					nMaxLBKind = m_TongPKOpt.m_nChooseLB;

				if (nMaxLBKind > pMsgSel->m_nNumMessage - 1)
					nMaxLBKind = pMsgSel->m_nNumMessage - 1;

				m_nDoQuestStep = 2;
				pMsgSel->OnClickMsg(nMaxLBKind - 1);
			}
			else
				pMsgSel->OnClickMsg(pMsgSel->m_nNumMessage);
			return;
		}

		theMap.ClickToNpc(theMap.NpcPoints[npc_ThienY].x, theMap.NpcPoints[npc_ThienY].y);

		break;

	case 2:	// Waiting for LB
		break;

	case 3:	// Nhan nhiem vu
		if (pMsgSel && pMsgSel->IsVisible())
		{
			char *szMsg0 = pMsgSel->m_pMessages[0]->Msg;
			if (strstr(szMsg0, "§­îc, ta sÏ cè g¾ng."))
			{
				pMsgSel->OnClickMsg(0);
				theMap.ResetShoping();
			}
			else if (strstr(szMsg0, "Ta biÕt råi!"))
				pMsgSel->OnClickMsg(0);
			else
				pMsgSel->OnClickMsg(pMsgSel->m_nNumMessage);
		}
		break;

	case 4: // Kiem tra nhiem vu
		{
			int nKind = g_pCurPlayer->GetTaskVal(bh_pkkind);
			int nLevel = g_pCurPlayer->GetTaskVal(bh_pklevel);
			int nQty = g_pCurPlayer->GetTaskVal(bh_pkqty);;
			char szMsg[128];
			sprintf(szMsg, "LB ®Æc biÖt %02d: %s, cÊp=%d, th­=%d",
				m_nTotalPKQuest+1, TongPKKind[nKind-1], nLevel, nQty);
			SaveYSLog(szMsg);
			sprintf(szMsg, "LB ®Æc biÖt <color=green>%02d<color>: <color=yellow>%s<color>, cÊp=<color=yellow>%d<color>, th­=<color=yellow>%d<color>",
				m_nTotalPKQuest+1, TongPKKind[nKind-1], nLevel, nQty);
			SystemMessage(szMsg, 1);
			// Check in user settings
			if (m_TongPKOpt.m_bAutoChange)
			{
				int i = 0;
				while (m_TongPKOpt.m_HardQuests[i].nWsLevel)
				{
					if (m_TongPKOpt.m_HardQuests[i].nQuestKind == nKind
						|| m_TongPKOpt.m_HardQuests[i].nQuestKind == 0)
					{
						if (nLevel >= m_TongPKOpt.m_HardQuests[i].nWsLevel &&
							nQty >= m_TongPKOpt.m_HardQuests[i].nLetters)
						{
							SaveYSLog("Hñy nhiÖm vô theo thiÕt lËp cña ng­êi ch¬i!");
							SystemMessage("<color=yellow>Hñy nvô theo thiÕt lËp cña ng­êi ch¬i!");
							m_nDoQuestStep = 30;
							theMap.ResetShoping();
							return;
						}
					}
					i++;
				}
			}
			// Check in current information
			int nCount = 0;
			for (int i = 0; i < MAX_TOP_TONG; i++)
			{
				if (g_GlobalTongInfo[i].dwTongID == g_pCurPlayer->m_cTong.m_dwTongNameID) continue;
				int nCurLevel = g_GlobalTongInfo[i].WorkShopLevel[nKind-1];
				if (!nCurLevel || (nCurLevel > nLevel))
					nCount++;
			}
			if (nCount < nQty)
			{
				SystemMessage("<color=yellow>NhiÖm vô bÊt kh¶ thi!");
				m_nDoQuestStep = 30;
				theMap.ResetShoping();
			}
			else
				m_nDoQuestStep = 5;
		}
		break;

	case 5:	// Di nhan Thu tien cu
		nRet = GetRefLetters();
		if (nRet == 1) // Done
		{
			m_nDoQuestStep = 6;
			theMap.InitValues();
		}
		else if (nRet == 2) // Failed
		{
			m_nDoQuestStep = 30;
			theMap.ResetShoping();
		}
		break;

	case 6:	// Di nang PK
		if (pMsgSel && pMsgSel->IsVisible())
		{
			theMap.m_nTimeWait = 3;
			char *szMsg0 = pMsgSel->m_pMessages[0]->Msg;
			if (strstr(szMsg0, "KÕt thóc ®èi tho¹i"))
				pMsgSel->OnClickMsg(0);
			else if (strstr(szMsg0, VISITED_PLACES))
				pMsgSel->OnClickMsg(0);
			else if (strstr(pMsgSel->m_InfoText, "Xin lùa chän:"))
			{
				int nDriverOpt = m_TongPKOpt.m_nDriverOpt;
				if (nDriverOpt == 0) nDriverOpt = 3;
				pMsgSel->OnClickMsg(nDriverOpt -  1);
				if (m_TongPKOpt.m_bAutoLogin)
					theMain.PostFeedBack(cmd_beginpk);
				time(&m_tmStart);
				m_nDoQuestStep = 8;
				m_bNeedGoTrain = TRUE;
				m_bGoTrainGetPK = TRUE;
			}
			else
				pMsgSel->OnClickMsg(pMsgSel->m_nNumMessage);

			return;
		}
		if (g_pCurPlayer->m_nPKValue ||
			theMap.m_bNeedGroceries || theMap.m_bNeedPharmacy)
		{
			time(&m_tmStart);
			m_nDoQuestStep = 7;
		}
		else
		{
			if (theMap.m_bPutMoney && EQUIP_MONEY > theMap.m_nMoneyAmount)
			{
				int nSaveAmount = EQUIP_MONEY - theMap.m_nMoneyAmount;
				g_pCoreShell->OperationRequest(GOI_MONEY_INOUT_STORE_BOX, TRUE, nSaveAmount);
				return;
			}
			if (!theMap.m_bDoShoping && theMap.RunningToPos(theMap.NpcPoints[npc_Driver1])) return;
			theMap.m_bDoShoping = TRUE;
			theMap.ClickToNpc(theMap.NpcPoints[npc_Driver1].x, theMap.NpcPoints[npc_Driver1].y);
		}
		break;

	case 20: // Tra nhiem vu
		if (!g_pCurPlayer->EnterTongMap(2))
			return;

		if (!theMap.m_bDoShoping && theMap.RunningToPos(theMap.NpcPoints[npc_ThienY])) return;
		theMap.m_bDoShoping = TRUE;

		if (pMsgSel && pMsgSel->IsVisible())
		{
			theMap.m_nTimeWait = 3;
			char *szMsg0 = pMsgSel->m_pMessages[0]->Msg;
			if (strstr(pMsgSel->m_InfoText, "H«m nay b¹n thùc hiÖn nhiÖm vô nµy 100 lÇn råi"))
			{
				pMsgSel->OnClickMsg(0);
				SystemMessage("<color=yellow>§· hoµn thµnh 100 nhiÖm vô LB ®Æc biÖt!");
				SaveYSLog("§· hoµn thµnh 100 nhiÖm vô LB ®Æc biÖt!\n");
				theMap.ResetShoping();
				m_nDoQuestStep = 40;
				m_nTotalPKQuest = 100;
			}
			else if (strstr(szMsg0, "NhËn nhiÖm vô ngÉu nhiªn"))
				pMsgSel->OnClickMsg(1);
			else if (strstr(szMsg0, "T×m hiÓu LÖnh bµi nhiÖm vô ®Æc biÖt"))
				pMsgSel->OnClickMsg(4); // Tra nhiem vu
			else if (strstr(szMsg0, "KÕt thóc ®èi tho¹i"))
			{
				pMsgSel->OnClickMsg(0);
				theMap.ResetShoping();
				m_nDoQuestStep = 0;
				m_nTotalPKQuest++;
				if (m_TongPKOpt.m_nMaxQuestCount > 0 &&
					m_nTotalPKQuest >= m_TongPKOpt.m_nMaxQuestCount)
				{
					SaveYSLog("§· hoµn thµnh ®ñ sè nvô LB ®Æc biÖt!\n");
					SystemMessage("<color=yellow>§· hoµn thµnh ®ñ sè nvô LB ®Æc biÖt!");
					m_nDoQuestStep = 40;
				}
			}
			else
				pMsgSel->OnClickMsg(pMsgSel->m_nNumMessage);
		}

		if (m_bIsYeSouAndPK) // Check nv DaTau
		{
			if (m_nQuestID == 5 && m_nRqType == ys_rqjingyan)
			{
				int nOldValue = g_pCurPlayer->GetTaskVal(ys_preexp);
				int nPreLevel = g_pCurPlayer->GetTaskVal(ys_prelevel);
				int nDifferent = theMain.GetExpUpLevel(nPreLevel, P_PLAYER->m_Level) + (int)g_pCurPlayer->m_nCurExp - nOldValue;
				if (nDifferent >= m_nRqValue)
				{
					if (m_TongPKOpt.m_bExitVictim)
						theMain.PostFeedBack(cmd_endpk);
					m_nGenDoing = qt_yesou;
					m_nYSDoing = ys_return;
					m_nDoQuestStep = 0;
					m_bIsYeSouAndPK = 0;
					SystemMessage("<color=yellow>§ñ ®iÓm KN, tr¶ nvô D· TÈu!");
					return;
				}
			}
			else if (!g_pCurPlayer->m_nPKValue)
			{
				m_nGenDoing = qt_yesou;
				m_nYSDoing = ys_none;
				m_nDoQuestStep = 0;
				m_bIsYeSouAndPK = 0;
				return;
			}
		}

		theMap.ClickToNpc(theMap.NpcPoints[npc_ThienY].x, theMap.NpcPoints[npc_ThienY].y);

		break;

	case 30: // Doi nhiem vu
		if (g_pCurPlayer->m_cTong.m_nContributePoint < 20)
		{
			SystemMessage("<color=yellow>Kh«ng ®ñ ®iÓm cèng hiÕn nhËn!");
			SaveYSLog("Kh«ng ®ñ ®iÓm cèng hiÕn!\n");
			m_nDoQuestStep = 40;
			break;
		}
		SystemMessage("<color=yellow>§æi nhiÖm vô LB ®Æc biÖt kh¸c!");
		SaveYSLog("§æi nhiÖm vô LB ®Æc biÖt kh¸c!");
		g_pCurPlayer->EnterTongMap(1);
		m_nDoQuestStep = 31;
		break;

	case 31: // Di huy nhiem vu
		if (!g_pCurPlayer->EnterTongMap(2))
			return;

		if (!theMap.m_bDoShoping && theMap.RunningToPos(theMap.NpcPoints[npc_ThienY])) return;
		theMap.m_bDoShoping = TRUE;

		if (pMsgSel && pMsgSel->IsVisible())
		{
			theMap.m_nTimeWait = 3;
			char *szMsg0 = pMsgSel->m_pMessages[0]->Msg;
			if (strstr(szMsg0, "NhËn nhiÖm vô ngÉu nhiªn"))
				pMsgSel->OnClickMsg(1);
			else if (strstr(szMsg0, "T×m hiÓu LÖnh bµi nhiÖm vô ®Æc biÖt"))
				pMsgSel->OnClickMsg(3); // Doi lenh bai
			else if (strstr(szMsg0, "§­îc, ta sÏ cè g¾ng."))
			{
				m_nDoQuestStep = 4;
				pMsgSel->OnClickMsg(0);
				theMap.ResetShoping();
			}
			else
				pMsgSel->OnClickMsg(pMsgSel->m_nNumMessage);
			return;
		}

		theMap.ClickToNpc(theMap.NpcPoints[npc_ThienY].x, theMap.NpcPoints[npc_ThienY].y);

		break;

	case 40:
		if ((*g_pMapID) >= city_banghoimin && (*g_pMapID) <= city_banghoimax)
		{
			// Return City
			if (pMsgSel && pMsgSel->IsVisible())
			{
				theMap.m_nTimeWait = 3;
				char *szMsg0 = pMsgSel->m_pMessages[0]->Msg;
				if (strstr(szMsg0, VISITED_PLACES))
					pMsgSel->OnClickMsg(1);
				else if (strstr(szMsg0, "l­îng]"))
				{
					int nDriverOpt = (int)P_PLAYER->m_dwID % 7;
					pMsgSel->OnClickMsg(nDriverOpt);
					theMap.ResetShoping();
				}
				else
					pMsgSel->OnClickMsg(pMsgSel->m_nNumMessage);

				return;
			}

			if (!theMap.m_bDoShoping && theMap.RunningToPos(theMap.NpcPoints[npc_Driver1])) return;
			theMap.m_bDoShoping = TRUE;

			theMap.ClickToNpc(theMap.NpcPoints[npc_Driver1].x, theMap.NpcPoints[npc_Driver1].y);

			return;
		}

		if (m_TongPKOpt.m_bExitVictim)
			theMain.PostFeedBack(cmd_endpk);

		if (m_bIsYeSouAndPK) // Check nv DaTau
		{
			if (m_nQuestID == 5 && m_nRqType == ys_rqjingyan)
			{
				m_nGenDoing = qt_yesou;
				m_nDoQuestStep = 9;
				m_bGoTrainGetPK = 0;
				theMap.m_bNoDriverRecent = TRUE;
				SystemMessage("<color=yellow>TiÕp tôc lµm nvô D· TÈu!");
				return;
			}
			else
			{
				m_nGenDoing = qt_yesou;
				m_nYSDoing = ys_none;
				m_nDoQuestStep = 0;
				m_bIsYeSouAndPK = 0;
				return;
			}
		}

		m_nGenDoing = qt_suspending;
		m_nDoQuestStep = 0;

		break;
	}
}

int CVLQuest::GetRefLetters()
{
	static char szLastMap[32] = "";
	static int nTongIdx = -1;
	static int nLetterStep = 0;
	int nCount = 0;
	int nFirstQ = bh_pkfirst;

	while (nFirstQ <= bh_pklast && g_pCurPlayer->GetTaskVal(nFirstQ))
		{ nCount++; nFirstQ++; }

	int nQty = g_pCurPlayer->GetTaskVal(bh_pkqty);
	if (nQty == 0) return 0; // Wait
	if (nCount >= nQty)
	{
		nTongIdx = -1;
		nLetterStep = 0;
		return 1; // Done
	}

	if (nTongIdx == -1 && nCount > 0) nTongIdx = nCount - 1;

	// Check WorkShop Level
	int nRqWs = g_pCurPlayer->GetTaskVal(bh_pkkind) - 1;
	int nRqLevel = g_pCurPlayer->GetTaskVal(bh_pklevel);

	static int nDestNpc = npc_Driver1;
	KUiMsgSel *pMsgSel = (KUiMsgSel*)GetPtrAddress(MSG_BASE_ADD);

	switch (nLetterStep)
	{
	case 0:	// Enter other Tong
		if (!g_GlobalTongInfo)
			return 0; // Doing
		while (++nTongIdx < MAX_TOP_TONG)
		{
			if (g_GlobalTongInfo[nTongIdx].dwTongID != g_pCurPlayer->m_cTong.m_dwTongNameID)
			{
				int nCurLevel = g_GlobalTongInfo[nTongIdx].WorkShopLevel[nRqWs];
				if (!nCurLevel || (nCurLevel > nRqLevel)) break;
			}
		}
		if (nTongIdx < MAX_TOP_TONG)
		{
			strcpy(szLastMap, g_szMapName);
			if (g_GlobalTongInfo[nTongIdx].bIsRestricted)
				nDestNpc = npc_Driver1;
			else
				nDestNpc = nRqWs + 5;
			// g_pCoreShell->TongOperation(0x23, g_GlobalTongInfo[nTongIdx].dwTongID, 0);
			g_pCurPlayer->EnterTongMap(1, g_GlobalTongInfo[nTongIdx].dwTongID);
			nLetterStep = 1;
		}
		else
		{
			nTongIdx = -1;
			SystemMessage("<color=yellow>NhiÖm vô thùc hiÖn thÊt b¹i!");
			return 2; // Failed
		}

		break;

	case 1:
		if (!g_pCurPlayer->EnterTongMap(2, g_GlobalTongInfo[nTongIdx].dwTongID))
			return 0; // Doing

		if (pMsgSel && pMsgSel->IsVisible())
		{
			theMap.m_nTimeWait = 3;
			if (strstr(pMsgSel->m_InfoText, "CÊm ®Þa bang héi, kh«ng ®­îc b­íc vµo"))
			{
				g_GlobalTongInfo[nTongIdx].bIsRestricted = TRUE;
				nDestNpc = npc_Driver1;
				theMap.ResetShoping();
				pMsgSel->OnClickMsg(0);
			}
			else
			{
				char *szMsg0 = pMsgSel->m_pMessages[0]->Msg;
				if (strstr(szMsg0, VISITED_PLACES))
				{
					if (strstr(pMsgSel->m_pMessages[2]->Msg, "§©u còng kh«ng muèn ®i!"))
					{
						g_GlobalTongInfo[nTongIdx].bIsRestricted = 0;
						nDestNpc = nRqWs + 5;
						theMap.ResetShoping();
					}
					pMsgSel->OnClickMsg(2);
				}
				else if (strstr(szMsg0, "Hái tin tøc"))
					pMsgSel->OnClickMsg(0);
				else if (strstr(szMsg0, "KÕt thóc ®èi tho¹i"))
				{
					char szInfoText[256];
					char *pInfo = szInfoText;
					for (int i = 0; i < 256 && i < (int)strlen(pMsgSel->m_InfoText); i++)
						if (pMsgSel->m_InfoText[i] > 31 || pMsgSel->m_InfoText[i] < 0)
						{
							*pInfo = pMsgSel->m_InfoText[i];
							pInfo++;
						}
					if (strstr(szInfoText, "sè T¸c ph­êng vÉn ch­a ®ñ") ||
						strstr(szInfoText, "H×nh nh­ ng­¬i ®ang lµm nhiÖm vô lÖnh bµi"))
					{
						SystemMessage(0, "<color=yellow>%s<color>: §¼ng cÊp t¸c ph­êng ch­a ®ñ <color=yellow>%d<color>!",
							g_GlobalTongInfo[nTongIdx].szTongName, nRqLevel);
						g_GlobalTongInfo[nTongIdx].WorkShopLevel[nRqWs] = nRqLevel;
					}

					nLetterStep = 0;
					pMsgSel->OnClickMsg(0);
					theMap.ResetShoping();
				}
				else
				{
					nLetterStep = 0;
					pMsgSel->OnClickMsg(pMsgSel->m_nNumMessage);
					theMap.ResetShoping();
				}
			}
			return 0;
		}

		if (nDestNpc != npc_Driver1)
		{
			if (!theMap.m_bDoShoping && theMap.RunningToPos(theMap.NpcPoints[nDestNpc])) return 0;
			theMap.m_bDoShoping = TRUE;
		}

		theMap.ClickToNpc(theMap.NpcPoints[nDestNpc].x, theMap.NpcPoints[nDestNpc].y);

		break;
	}

	return 0;
}
