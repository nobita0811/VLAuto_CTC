#include "stdafx.h"
#include "../GameDef.h"
#include "DTextFile.h"
#include "GameBase.h"
#include "JxCore.h"
#include "S3Client.h"
#include "VLClasses.h"

//================================Class CVLRecover================================
CVLRecover::CVLRecover(void)
{
	m_bDoingBuff = 0;
	m_bEmptyMana = 0;
	m_nAMBuffCount = MAX_BUFF_COUNT;			// Magic Paper
	m_nNMBuffCount = MAX_NMBF_COUNT;			// Nga Mi Buff
	m_nPNBuffCount = MAX_NMBF_COUNT;			// Poison Buff
}

void CVLRecover::Breathe()
{
	if (g_NetworkOffline) return;
	CheckEquip();
	if (m_bUseMaster) OnUseSDT();
	if (m_bUseDoubleExp || theQuest.m_bUseDoubleExp) OnUseTTL();

	if (!P_PLAYER->m_FightMode) return;			// AutoBuff only when in FightMode

	int nCurHP = P_PLAYER->m_CurrentLife;
	int nMaxHP = P_PLAYER->GetLifeMax();
	int nCurMP = P_PLAYER->m_CurrentMana;
	int nMaxMP = P_PLAYER->GetManaMax();

	if (m_bAutoHP && nCurHP < m_nAutoHPAmount && m_nHPBuffCount >= m_nAutoHPTimer * 20)
	{	//Auto Buff HP
		m_nHPBuffCount = 0;
		if (!g_pCurPlayer->EatMedicine(medicine_blood))
		{
			if (m_bAMByBlood)
			{
				OnUseAM();
				return;
			}
		}
	}

	if (nCurMP * 100 / nMaxMP > 60) m_bEmptyMana = 0;
	if (m_bAutoMP && nCurMP < m_nAutoMPAmount && m_nMPBuffCount >= m_nAutoMPTimer * 20)
	{	//Auto Buff MP
		m_nMPBuffCount = 0;
		if (!g_pCurPlayer->EatMedicine(medicine_mana))
		{
			if (m_bAMByMana)
			{
				OnUseAM();
				return;
			}
			m_bEmptyMana = TRUE;
		}
	}

	if (m_bAMByHP && nCurHP < m_nAMByHPAmount)
	{
		OnUseAM(TRUE);
		if (theMain.m_bPrintScreen && !m_nAMBuffCount)
			ExecuteScript("PrintScreen()");
		m_nAMBuffCount++;
		return;
	}

	if (m_bAMByMP && nCurMP < m_nAMByMPAmount)
	{
		OnUseAM(TRUE);
		if (theMain.m_bPrintScreen && !m_nAMBuffCount)
			ExecuteScript("PrintScreen()");
		m_nAMBuffCount++;
		return;
	}

	CheckInventory();
	CheckPKQuest();

	//Nga Mi Buff
	if (g_pCurPlayer->m_nCurFaction == 4) DoNgaMiBuff();

	//Anti-Poison
	if (m_bUsePoison && m_nPNBuffCount >= MAX_BUFF_COUNT && P_PLAYER->m_PoisonState.nTime && nCurHP < nMaxHP * 80 / 100)
	{
		g_pCurPlayer->EatMedicine(medicine_antipoison);
		m_nPNBuffCount = MAX_BUFF_COUNT / 2;
	}

	m_nNMBuffCount++;
	m_nHPBuffCount++;
	m_nMPBuffCount++;
	m_nAMBuffCount++;
	m_nPNBuffCount++;
}

void CVLRecover::DoNgaMiBuff()
{
	//Chech if Player is Nga Mi here...
	if (m_nNMBuffCount >= 10)						// NM Stop Buffing
		m_bDoingBuff = 0;
	int nCurHP = P_PLAYER->m_CurrentLife;
	int nMaxHP = P_PLAYER->GetLifeMax();
	if (m_bIsNgaMiBuff && m_nNMBuffCount >= MAX_NMBF_COUNT)
	{	// Nga Mi Bufff
		if (nMaxHP == 0) nMaxHP = 100;
		if (nCurHP * 100 / nMaxHP < m_nNgaMiBuffPercent)
		{	//Self Buff
			P_PLAYER->SendCommand(do_skill, NGAMY_BUFF_SKILLID, -1, 1);
			theAttack.SendVLCmdSkill(NGAMY_BUFF_SKILLID, -1, P_PLAYER->m_dwID);
			m_bDoingBuff = TRUE;
			m_nNMBuffCount = 0;
		}
		else if (m_bPartyBuff)
		{	//Team Buff
			int nNpcX, nNpcY, nPlayerX, nPlayerY, dZ;
			P_PLAYER->GetMapPos(&nPlayerX, &nPlayerY);

			for (int i = 1; i < MAX_NPC; i++)
			{	// Search for the target Npc
				if (!g_pNpcArray[i]->IsExist()) continue;
				g_pNpcArray[i]->GetMapPos(&nNpcX, &nNpcY);
				dZ = __distance(nPlayerX, nPlayerY, nNpcX, nNpcY);
				if (dZ > NGAMY_BUFF_RADIUS) continue;

				for (int j = 0; j <= g_pTeam->m_nMemNum; j++)
				{
					if (g_pTeam->m_nMember[j] == g_pNpcArray[i]->m_dwID && g_pNpcArray[i]->m_CurrentLife < m_nNgaMiBuffPercent)
					{
						P_PLAYER->SendCommand(do_skill, NGAMY_BUFF_SKILLID, -1, g_pNpcArray[i]->m_Index);
						theAttack.SendVLCmdSkill(NGAMY_BUFF_SKILLID, -1, g_pNpcArray[i]->m_dwID);
						m_bDoingBuff = TRUE;
						m_nNMBuffCount = 0;
						break; //for j
					}
				} // end of for j
				if (m_nNMBuffCount == 0) break; //for i
			} // end of for i
		}
	}
}

void CVLRecover::CheckEquip()
{
	static int nRepairingItem = 0;
	static int nCloseShop = 0;		// Da mo Ky Tran Cac (OnClickGShop) de sua do: sua xong thi tat
	static int nTimerCount = 0;
	nTimerCount++;

	if (nTimerCount % 10 == 7)	// each 0.5s
	{
		KUiMsgSel *pMsgSel = (KUiMsgSel*)GetPtrAddress(MSG_BASE_ADD);
		if (pMsgSel && pMsgSel->IsVisible() && pMsgSel->m_nNumMessage >= 4)
		{
			char *szMsg1 = (char*)pMsgSel->m_pMessages[1]->Msg;
			if (strstr(szMsg1, FULL_REPAIR))	// Söa kü [%d v¹n%d l­îng]
			{
				char *szPriceV = szMsg1 + strlen(FULL_REPAIR);
				int nRepairPrice = atoi(szPriceV);

				char *szPriceL = strstr(szPriceV, TV(L"vạn"));
				if (szPriceL)
				{
					szPriceL += 3;
					nRepairPrice = nRepairPrice * 10000 + atoi(szPriceL);
				}

				if (nRepairPrice <= EQUIP_MONEY)
				{
					pMsgSel->OnClickMsg(1);
					SystemMessage(TV(L"<color=yellow>Sửa kỹ đồ Hoàng Kim"));
				}
				else
				{
					pMsgSel->OnClickMsg(3);
					SystemMessage(TV(L"<color=yellow>Không đủ tiền sửa đồ"));
				}
				return;
			}
		}
		if (nCloseShop && !nRepairingItem && --nCloseShop == 0)
			CloseAllWindows(0);		// Sua do xong (0.5s sau RepairItem): tat cua so Ky Tran Cac
		if (nRepairingItem)
		{
			g_pCurPlayer->RepairItem(nRepairingItem);
			nRepairingItem = 0;
			nCloseShop = 2;
		}
		return;
	}

	if (nTimerCount % 600 != 40) return; //each 30s

	for (int i = 0; i < MAX_EQUIP_SLOT; i++)
	{
		int nEquipIndex = g_pCurPlayer->m_ItemList.GetEquip(i);
		if (!nEquipIndex) continue;
		KItem *pItem = (KItem*)(dwItmBaseAdd + nEquipIndex * ITM_DATA_SIZE);
		if (pItem->m_nCurDur < 0) continue; // Durability not available
		if (pItem->m_nCurDur >= pItem->m_nMaxDur) continue; // No need to repair

		if (m_bAutoRepair && P_PLAYER->m_FightMode && !nRepairingItem)
		{
			int nAutoRepairDur = pItem->m_nMaxDur * 8 / 10;
			if (pItem->m_nCurDur <= nAutoRepairDur && pItem->m_nItemPlus < 6)
			{
				char szMsg[128];
				if (pItem->GetRepairPrice() <= EQUIP_MONEY)
				{
					KUiPlayerBar *pUiPlayerBar = (KUiPlayerBar*)GetPtrAddress(UPB_BASE_ADD);
					pUiPlayerBar->OnClickGShop();
					nRepairingItem = nEquipIndex;
					sprintf(szMsg, TV(L"Tự sửa đồ: <color=yellow><%s>"), ComAttr.szItemName);
				}
				else
					sprintf(szMsg, TV(L"<color=yellow>Không đủ tiền sửa đồ"));
				SystemMessage(szMsg);
			}
		}

		if (pItem->m_nCurDur < m_nAMByDurAmount)
		{
			char szMessage[128];
			// Trang bị sắp hỏng
			sprintf(szMessage, TV(L"<color=green>VLCTCPro<color> : Trang bị sắp hỏng: <color=red><%s>"), ComAttr.szItemName); 
			PostChatMessage(szMessage);
			if (P_PLAYER->m_FightMode)
			{
				if (m_bAMByDur || pItem->m_nCurDur < 3)
					OnUseAM();
				else if (i == itempart_weapon)
					theMain.SetPause();
			}
			return;
		}
	}
}

void CVLRecover::CheckInventory()
{
	// Truoc day ca 2 kiem tra chi chay moi 600 nhip (~30s) => day ruong phai cho toi 30s moi ve thanh.
	// Gio kiem tra moi 60 nhip (~3s).
	static int nTimerCount = 0;
	nTimerCount++;
	if (nTimerCount % 60 != 7) return;

	//Check Money
	if (m_bAMByMoney && EQUIP_MONEY > m_nAMByMoneyAmount * 10000)
	{
		char szMessage[128];
		sprintf(szMessage, TV(L"<color=green>VLCTCPro<color> : Tiền <color=red>%d vạn <color>  về thành cất tiền (!cy) "), EQUIP_MONEY/10000);
		PostChatMessage(szMessage);
		OnUseAM();
		return;
	}

	//Chec Full Room
	if (m_bAMByFullRoom)
	{
		POINT itmPos;
		BOOL bHaveRoom = 0;
		switch (m_nAMByFullRoomVal)
		{
		case 0: // Do 2 o
			bHaveRoom = EQUIP_ROOM.FindRoom(2, 1, &itmPos);
			if (bHaveRoom) bHaveRoom = EQUIP_ROOM.FindRoom(1, 2, &itmPos);
			break;
		case 1: // Do 3 o
			bHaveRoom = EQUIP_ROOM.FindRoom(1, 3, &itmPos);
			break;
		case 2: // Do 4 o
			bHaveRoom = EQUIP_ROOM.FindRoom(2, 2, &itmPos);
			break;
		case 3: // Do 6 o
			bHaveRoom = EQUIP_ROOM.FindRoom(2, 3, &itmPos);
			break;
		}
		if (!bHaveRoom)
		{			
			PostChatMessage(TV(L"<color=green>VLCTCPro<color> : <color=red> Hành trang đã đầy cần về thành<color> (!cy)"));
			OnUseAM();
		}
			
	}
}

void CVLRecover::CheckPKQuest()
{
	if (theQuest.m_nGenDoing == qt_dopk && g_pCurPlayer->m_nPKValue)
	{
		OnUseAM();
		if (!m_nAMBuffCount)
			SystemMessage(TV(L"<color=yellow>Đạt PK, về thành trả nhiệm vụ!"));
		m_nAMBuffCount++;
	}
}

void CVLRecover::OnUseAM(BOOL bUrgent)
{
	KUiPlayerBar *pUiPlayerBar = (KUiPlayerBar*)GetPtrAddress(UPB_BASE_ADD);
	if (!pUiPlayerBar) return;

	if (m_nAMBuffCount >= MAX_BUFF_COUNT)
	{
		if (theMap.m_nTrainMapID && (*g_pMapID) != theMap.m_nTrainMapID)
			theMap.m_bNoDriverRecent = TRUE;
		if ((*g_pMapID) == city_visondao)
		{
			if (bUrgent)
				theMain.ExitGame();
			else if (!theMove.IsFlagMoving())
			{
				int nDestX = 44914, nDestY = 89316; // ViSonDao Center
				int nPlayerX, nPlayerY, dX, dY;
				P_PLAYER->GetMapPos(&nPlayerX, &nPlayerY);
				dX = nDestX - nPlayerX;
				dY = nDestY - nPlayerY;
				KUiMiniMap *pMiniMap = (KUiMiniMap*)(MMP_BASE_ADD);
				if (pMiniMap) pMiniMap->SetMapFlag(dX >> 4, dY >> 5);
				SystemMessage(TV(L"<color=yellow>Quay về tâm đảo!!!"));
			}
		}
		else if ((*g_pMapID) >= 387 && (*g_pMapID) <= 395)
		{
			// Do nothing here
		}
		else if (!g_pCurPlayer->UseItem(AMULET_PAPER))
		{
			if (!g_pCurPlayer->UseItem(AMULETX_PAPER))
			{
				if (pUiPlayerBar->m_nHK3Gengre)
				{
					ExecuteScript("ShortcutUseItem(2)");
					theMain.PostFeedBack(cmd_useam);
				}
				else if (bUrgent)
					//theMain.ExitGame(); // Sap chet ma het AM -> thoat game
					SystemMessage(TV(L"<color=yellow>Hết Thổ địa phù - không về thành được!"),1);
				else // Do hu / day ruong / nhieu tien ma het AM -> chi thong bao
					SystemMessage(TV(L"<color=yellow>Hết Thổ địa phù - không về thành được!"),1);
			}
			else
				theMain.PostFeedBack(cmd_useam);
		}
		else
			theMain.PostFeedBack(cmd_useam);
		if (theBasic.m_bExitWhenAM)
			theMain.ExitGame();
		if (!theMain.m_bSilentMode)
		{
			theMain.PostFeedBack(cmd_amulet);
			_ShowWindow(theMain.m_hWndGame, SW_RESTORE);
			_SetForegroundWindow(theMain.m_hWndGame);
		}
		m_nAMBuffCount = 0;
	}
}

void CVLRecover::OnUseSDT()
{
	static int nTimerCount = g_RunningTimer + 20; // 1 second to go
	KUiMsgSel *pMsgSel = (KUiMsgSel*)GetPtrAddress(MSG_BASE_ADD);
	if (nTimerCount <= g_RunningTimer)
	{
		theUtil.m_bPausingEntrust = TRUE;
		if (g_pCurPlayer->m_nEntrustMode)
			nTimerCount = g_RunningTimer + 100; // wait for 5s
		else if (g_pCurPlayer->UseItem(TV(L"Sư đồ thiếp")))
			nTimerCount = g_RunningTimer + 600; // wait for 30s
		else
		{
			theUtil.m_bPausingEntrust = 0;
			nTimerCount = g_RunningTimer + 12000; // 10 minutes
		}
		return;
	}
	if (theUtil.m_bPausingEntrust && pMsgSel && pMsgSel->IsVisible())
	{
		if (strstr(pMsgSel->m_InfoText, TV(L"Hãy lựa chọn thao tác liên quan")))
		{
			pMsgSel->OnClickMsg(0);
			nTimerCount = g_RunningTimer + 30000; // 25 minutes
			theUtil.m_bPausingEntrust = 0;
		}
	}
}

void CVLRecover::LoadX2List()
{
	memset(m_szX2Items, 0, sizeof(m_szX2Items));
	strcpy(m_szX2Items[0], TV(L"Tiên Thảo Lộ"));
	char szX2ItemFile[MAX_PATH];
	sprintf(szX2ItemFile, "%s\\Maps\\X2Items.txt", theMain.m_szCurFolder);

	DTextFile itemFile;
	if (!itemFile.Open(szX2ItemFile))
		return;

	itemFile.ReadAllLines(m_szX2Items[0], MAX_X2_ITEMS, 80);
}

void CVLRecover::OnUseTTL()
{
	static int nTimerCount = g_RunningTimer + 60; // 3 seconds to go
	if (g_RunningTimer % 20 != 1) return; // Checking each 1s
	if (P_PLAYER->m_FightMode)
	{
		if (theQuest.m_bNeedGoPCK) return;
		if (theQuest.m_bGoTrainGetPK) return;
	}

	KStateNode* pNode = (KStateNode*)P_PLAYER->m_pHeadNode;
	while (pNode)
	{
		if (pNode->m_SkillID == 0x01B8 ||
			pNode->m_SkillID == 0x01C3 ||
			pNode->m_SkillID == 0x037C)
		{
			nTimerCount = g_RunningTimer + 200; // 10 seconds to go
			return;
		}
		pNode = (KStateNode*)pNode->GetNext();
	}
	if (nTimerCount > g_RunningTimer) return;

	int nX2Item = -1; int i = 0;
	while (i < MAX_X2_ITEMS && m_szX2Items[i][0])
	{
		if (g_pCurPlayer->GetItemsCount(m_szX2Items[i]) >= 1)
		{
			nX2Item = i;
			break;
		}
		i++;
	}
	if (nX2Item < 0) return;

	if (g_pCurPlayer->m_nEntrustMode)
	{
		theUtil.m_bPausingEntrust = TRUE;
		nTimerCount = g_RunningTimer + 100; // 5 seconds to go
		SystemMessage(TV(L"<color=yellow>Tạm ngưng ủy thác, ăn Tiên Thảo Lộ..."));
		return;
	}

	g_pCurPlayer->UseItem(m_szX2Items[nX2Item]);
	nTimerCount = g_RunningTimer + 200; // 10 seconds to go
	theUtil.m_bPausingEntrust = 0;
}

BOOL CVLRecover::IsInX2List(const char *szItemName)
{
	int i = 0;
	while (i < MAX_X2_ITEMS && m_szX2Items[i][0])
	{
		if (strstr(szItemName, m_szX2Items[i]))
			return TRUE;
		i++;
	}
	return 0;
}
