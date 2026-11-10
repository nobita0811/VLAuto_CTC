#include "stdafx.h"
#include "../GameDef.h"
#include "DTextFile.h"
#include "GameBase.h"
#include "JxCore.h"
#include "S3Client.h"
#include "VLClasses.h"
#include "BufLog.h"
#include "SuaDoLog.h"

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

	if (m_bAutoHP && nCurHP < m_nAutoHPAmount && nCurHP * 10 < nMaxHP * 7 && m_nHPBuffCount >= m_nAutoHPTimer * 20)
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
	if (m_bAutoMP && nCurMP < m_nAutoMPAmount && nCurMP * 10 < nMaxMP * 7 && m_nMPBuffCount >= m_nAutoMPTimer * 20)
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

//------------------------------------------------------------------ Sua do qua Ky Tran Cac
// Dung chung tren map (Tu sua do, tab Phuc hoi) va trong thanh (Sua do, tab Ban do): mo Ky Tran Cac (OnClickGShop),
// gui RepairItem tung mon, bang hoi "Sua ky [x van y luong]" (do Hoang Kim) thi bam neu du tien. Trong thanh thieu
// tien thi bo bang hoi, rut tu ruong dung so con thieu roi gui lai lenh sua mon do (CheckReady cat lai tien sau).
extern void TownMoneyMarkBase();

enum { rj_idle = 0, rj_open, rj_wait, rj_unlock, rj_money, rj_close };
#define RJ_RESULT_WAIT		4000	// Gui lenh sua: cho do ben doi / bang hoi hien
#define RJ_UNLOCK_WAIT		15000	// Mo khoa ruong de rut tien
#define RJ_MONEY_WAIT		5000	// Cho tien rut ve tren nguoi

static int s_nRJSlots[MAX_EQUIP_SLOT];	// O trang bi can sua (luu o, khong luu idx: idx do co the doi)
static int s_nRJCount = 0;
static int s_nRJPos = 0;
static int s_nRJStep = rj_idle;
static DWORD s_dwRJTick = 0;
static BOOL s_bRJTown = FALSE;			// Dang o thanh: duoc rut tien tu ruong
static BOOL s_bRJWithdrew = FALSE;		// Mon hien tai da rut tien 1 lan
static int s_nRJDur0 = 0;
static int s_nRJMoney0 = 0;
static int s_nRJTake = 0;				// So tien can rut
static BOOL s_bRJTried = FALSE;			// Da thu sua (tu luc co mon hong < 3) - reset khi het mon hong
static const char* s_szRJWho = "";

static BOOL RJNeedRepair(KItem* pItem)
{
	return pItem->m_nCurDur >= 0 && pItem->m_nCurDur < pItem->m_nMaxDur && pItem->m_nItemPlus < 6;
}

static KItem* RJSlotItem(int nSlot, int* pIdx)
{
	int nIdx = g_pCurPlayer->m_ItemList.GetEquip(nSlot);
	if (pIdx) *pIdx = nIdx;
	if (!nIdx) return NULL;
	return (KItem*)(dwItmBaseAdd + nIdx * ITM_DATA_SIZE);
}

// Gia trong dong "Sua ky [12 van2880 luong]" => 122880
static int RJParsePrice(const char* szMsg)
{
	const char* p = strstr(szMsg, FULL_REPAIR);
	if (!p) return -1;
	p += strlen(FULL_REPAIR);
	int nPrice = atoi(p);
	const char* pL = strstr(p, TV(L"vạn"));
	if (pL) nPrice = nPrice * 10000 + atoi(pL + 3);
	return nPrice;
}

static void RJSend()
{
	DWORD dwNow = GetTickCount();
	while (s_nRJPos < s_nRJCount)
	{
		int nIdx;
		KItem* pItem = RJSlotItem(s_nRJSlots[s_nRJPos], &nIdx);
		if (pItem && RJNeedRepair(pItem))
		{
#if DebugSuaDo
			SuaDoSent(nIdx, s_szRJWho);
#endif
			SystemMessage(0, TV(L"Sửa đồ: <color=yellow>%s"), ComAttr.szItemName);
			g_pCurPlayer->RepairItem(nIdx);
			s_nRJDur0 = pItem->m_nCurDur;
			s_nRJStep = rj_wait;
			s_dwRJTick = dwNow;
			return;
		}
		s_nRJPos++;
		s_bRJWithdrew = FALSE;
	}
#if DebugSuaDo
	SuaDoLog("[Ky Tran Cac - %s] da gui lenh sua het %d mon => tat cua so", s_szRJWho, s_nRJCount);
#endif
	s_nRJStep = rj_close;
	s_dwRJTick = dwNow;
}

static void RJNext()
{
	s_nRJPos++;
	s_bRJWithdrew = FALSE;
	RJSend();
}

// Bang hoi "Sua ky [...]" dang hien: TRUE = da xu ly (bam / bo)
template <class TMsgSel>
static BOOL RJDialog(TMsgSel* pMsgSel, const char* szType)
{
	if (!pMsgSel || !pMsgSel->IsVisible()) return FALSE;
	int nNum = pMsgSel->m_nNumMessage;
	if (nNum <= 0 || nNum > 20) return FALSE;
	int nLine = -1;
	for (int i = 0; i < nNum; i++)
		if (strstr((char*)pMsgSel->m_pMessages[i]->Msg, FULL_REPAIR)) { nLine = i; break; }
	if (nLine < 0) return FALSE;

	char* szMsg = (char*)pMsgSel->m_pMessages[nLine]->Msg;
	int nPrice = RJParsePrice(szMsg);
	int nMoney = EQUIP_MONEY;
	int nCancel = nLine == nNum - 1 ? nNum : nNum - 1;	// Dong "Tam thoi khong sua" (dong cuoi)
	BOOL bJob = s_nRJStep == rj_wait;

	if (nPrice <= nMoney)
	{
#if DebugSuaDo
		SuaDoLog("[%s] bang hoi '%s': gia %d <= tien %d => bam [%d] Sua ky | %s", szType, szMsg, nPrice, nMoney, nLine,
			bJob ? s_szRJWho : "khong phai auto gui lenh");
#endif
		pMsgSel->OnClickMsg(nLine);
		SystemMessage(0, TV(L"<color=yellow>Sửa kỹ đồ Hoàng Kim (%d lượng)"), nPrice);
		if (bJob) s_dwRJTick = GetTickCount();	// Cho do ben doi
		return TRUE;
	}

	// Thieu tien: trong thanh thi rut tu ruong (1 lan moi mon)
	int nTake = nPrice - nMoney;
	BOOL bWithdraw = bJob && s_bRJTown && !s_bRJWithdrew && ROOM_MONEY >= nTake;
#if DebugSuaDo
	SuaDoLog("[%s] bang hoi '%s': gia %d > tien %d (ruong %d) => bam [%d] khong sua, %s", szType, szMsg, nPrice, nMoney, ROOM_MONEY,
		nCancel, bWithdraw ? "rut tien roi sua lai" : !bJob ? "khong phai auto gui lenh" : !s_bRJTown ? "dang tren map (khong rut duoc)" :
		s_bRJWithdrew ? "da rut 1 lan van thieu" : "ruong khong du tien");
#endif
	pMsgSel->OnClickMsg(nCancel);
	if (bWithdraw)
	{
		SystemMessage(0, TV(L"<color=yellow>Rút %d lượng để sửa đồ Hoàng Kim (giá %d, trên người %d)"), nTake, nPrice, nMoney);
		s_nRJTake = nTake;
		s_bRJWithdrew = TRUE;
		s_nRJStep = rj_unlock;
		s_dwRJTick = GetTickCount();
		return TRUE;
	}
	SystemMessage(0, TV(L"<color=yellow>Không đủ tiền sửa đồ Hoàng Kim (giá %d, trên người %d, rương %d)"), nPrice, nMoney, ROOM_MONEY);
	if (bJob) RJNext();
	return TRUE;
}

BOOL CVLRecover::StartRepair(BOOL bTown, const char* szWho)
{
	if (s_nRJStep != rj_idle) return TRUE;
	s_nRJCount = 0;
	for (int i = 0; i < MAX_EQUIP_SLOT; i++)
	{
		KItem* pItem = RJSlotItem(i, NULL);
		if (pItem && RJNeedRepair(pItem)) s_nRJSlots[s_nRJCount++] = i;
	}
	s_bRJTried = TRUE;
	if (!s_nRJCount) return FALSE;
	s_nRJPos = 0;
	s_bRJTown = bTown;
	s_bRJWithdrew = FALSE;
	s_szRJWho = szWho;
#if DebugSuaDo
	SuaDoEquipDump(szWho);
	SuaDoLog("[Ky Tran Cac - %s] bat dau sua %d mon, tien %d, ruong %d", szWho, s_nRJCount, EQUIP_MONEY, ROOM_MONEY);
#endif
	CloseAllWindows(0);
	KUiPlayerBar* pUiPlayerBar = (KUiPlayerBar*)GetPtrAddress(UPB_BASE_ADD);
	if (pUiPlayerBar) pUiPlayerBar->OnClickGShop();
	s_nRJStep = rj_open;
	s_dwRJTick = GetTickCount();
	return TRUE;
}

BOOL CVLRecover::IsRepairing()
{
	return s_nRJStep != rj_idle;
}

// Moi 0.5s
static void RJBreathe()
{
	if (RJDialog((KUiMsgSel*)GetPtrAddress(MSG_BASE_ADD), "KUiMsgSel")) return;
	if (RJDialog((KUiMsgSel2*)GetPtrAddress(MSG2_BASE_ADD), "KUiMsgSel2")) return;

	DWORD dwNow = GetTickCount();
	switch (s_nRJStep)
	{
	case rj_open:	// Ky Tran Cac vua mo (0.5s)
		RJSend();
		break;
	case rj_wait:
	{
		KItem* pItem = RJSlotItem(s_nRJSlots[s_nRJPos], NULL);
		if (!pItem || pItem->m_nCurDur != s_nRJDur0 || dwNow - s_dwRJTick >= RJ_RESULT_WAIT)
			RJNext();
		break;
	}
	case rj_unlock:
		if (theMain.OpenLockRoom(theMap.m_nRepositoryPass))
		{
			theMap.m_bRoomLocked = TRUE;
			if (dwNow - s_dwRJTick < RJ_UNLOCK_WAIT) break;
			SystemMessage(1, TV(L"<color=red>Mở khóa rương 15s không được - không rút được tiền sửa đồ"));
			RJNext();
			break;
		}
		TownMoneyMarkBase();
		s_nRJMoney0 = EQUIP_MONEY;
		g_pCoreShell->OperationRequest(GOI_MONEY_INOUT_STORE_BOX, 0, s_nRJTake);
#if DebugSuaDo
		SuaDoLog("[Ky Tran Cac - %s] rut %d luong tu ruong (tien %d, ruong %d)", s_szRJWho, s_nRJTake, EQUIP_MONEY, ROOM_MONEY);
#endif
		s_nRJStep = rj_money;
		s_dwRJTick = dwNow;
		break;
	case rj_money:
		if (EQUIP_MONEY <= s_nRJMoney0 && dwNow - s_dwRJTick < RJ_MONEY_WAIT) break;
#if DebugSuaDo
		SuaDoLog("[Ky Tran Cac - %s] tien %d -> %d, gui lai lenh sua", s_szRJWho, s_nRJMoney0, EQUIP_MONEY);
#endif
		RJSend();	// Gui lai lenh sua mon nay (s_bRJWithdrew = TRUE: thieu nua thi bo qua)
		break;
	case rj_close:
		if (dwNow - s_dwRJTick < 500) break;
		CloseAllWindows(0);
		s_nRJStep = rj_idle;
		break;
	}
}

// Mon hong (do ben < 3): idx, NULL = khong co
static KItem* RJBrokenItem()
{
	for (int i = 0; i < MAX_EQUIP_SLOT; i++)
	{
		KItem* pItem = RJSlotItem(i, NULL);
		if (pItem && pItem->m_nCurDur >= 0 && pItem->m_nCurDur < 3) return pItem;
	}
	return NULL;
}

void CVLRecover::CheckEquip()
{
	static int nTimerCount = 0;
	nTimerCount++;
#if DebugSuaDo
	SuaDoPoll();
#endif

	if (nTimerCount % 10 == 7)	// each 0.5s
	{
		RJBreathe();
		return;
	}

	if (nTimerCount % 200 == 107)	// each 10s: trang bi hong (< 3) => tam ngung auto
	{
		KItem* pItem = RJBrokenItem();
		if (!pItem) { s_bRJTried = FALSE; return; }
		if (IsRepairing()) return;
		if (!P_PLAYER->m_FightMode && theMap.m_bAutoReturn && theMap.m_bNeedRepairItems) return;	// Trong thanh: sap sua
		if (P_PLAYER->m_FightMode)
		{	// Tren map: dung Tho Dia Phu ve thanh sua; khong co TDP thi tam ngung
			if (theMain.HaveTownPortal())
			{
#if DebugSuaDo
				SuaDoLog("Tren map: %s do ben %d/%d (< 3) => dung Tho Dia Phu ve thanh sua", ComAttr.szItemName, pItem->m_nCurDur, pItem->m_nMaxDur);
#endif
				SystemMessage(0, TV(L"<color=yellow>Trang bị hư hỏng <%s> - dùng Thổ Địa Phù về thành sửa"), ComAttr.szItemName);
				OnUseAM();
				return;
			}
		}
		else if (!s_bRJTried && theMap.m_bRepair)
		{	// Trong thanh: thu sua 1 lan, khong duoc thi tam ngung
			StartRepair(TRUE, "Trong thanh - do hong < 3");
			return;
		}
#if DebugSuaDo
		SuaDoLog("TAM NGUNG AUTO: %s do ben %d/%d (< 3), tien %d, ruong %d | %s", ComAttr.szItemName, pItem->m_nCurDur, pItem->m_nMaxDur,
			EQUIP_MONEY, ROOM_MONEY, P_PLAYER->m_FightMode ? "tren map, het Tho Dia Phu" : "trong thanh, sua khong duoc");
#endif
		theMain.SetPause();
		PostChatMessage2(TV(L"<color=green>VLCTCPro<color> :<color=yellow>Tạm ngưng , trang bị đang bị hư hỏng (:H)!!!"));
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
		int nAutoRepairDur = pItem->m_nMaxDur * 8 / 10;
#if DebugSuaDo
		if (pItem->m_nCurDur <= nAutoRepairDur || pItem->m_nCurDur < m_nAMByDurAmount)
		{
			const char* szWhy;
			if (!m_bAutoRepair) szWhy = "KHONG tu sua: tat 'Tu sua do'";
			else if (!P_PLAYER->m_FightMode) szWhy = "KHONG tu sua tren map: dang trong thanh (sua o thanh theo tab Ban do)";
			else if (IsRepairing()) szWhy = "cho: dang sua";
			else if (pItem->m_nCurDur > nAutoRepairDur) szWhy = "chua toi 80% (chi canh bao sap hong)";
			else if (pItem->m_nItemPlus >= 6) szWhy = "KHONG tu sua: +N >= 6";
			else szWhy = "TU SUA: mo Ky Tran Cac";
			SuaDoLog("[Quet 30s] o %d %s do ben %d/%d (tu sua khi <= %d, canh bao < %d) +%d gia %d tien %d => %s", i,
				ComAttr.szItemName, pItem->m_nCurDur, pItem->m_nMaxDur, nAutoRepairDur, m_nAMByDurAmount, pItem->m_nItemPlus,
				pItem->GetRepairPrice(), EQUIP_MONEY, szWhy);
		}
#endif

		if (m_bAutoRepair && P_PLAYER->m_FightMode && !IsRepairing() && pItem->m_nCurDur <= nAutoRepairDur && pItem->m_nItemPlus < 6)
		{
			SystemMessage(0, TV(L"Tự sửa đồ: <color=yellow><%s>"), ComAttr.szItemName);
			StartRepair(FALSE, "Tren map - tu sua (Ky Tran Cac)");
			return;
		}

		if (pItem->m_nCurDur < m_nAMByDurAmount)
		{
			char szMessage[128];
			// Trang bị sắp hỏng
			sprintf(szMessage, TV(L"<color=green>VLCTCPro<color> : Trang bị sắp hỏng: <color=red><%s>"), ComAttr.szItemName);
			PostChatMessage2(szMessage);
			SystemMessage(1,szMessage);
			if (P_PLAYER->m_FightMode)
			{
				if (m_bAMByDur || (pItem->m_nCurDur < 3 && theMain.HaveTownPortal()))
					OnUseAM();
				else if (i == itempart_weapon && pItem->m_nCurDur >= 3)
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
		PostChatMessage2(szMessage);
		SystemMessage(1,szMessage);
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
			PostChatMessage2(TV(L"<color=green>VLCTCPro<color> : <color=red> Hành trang đã đầy cần về thành<color> (!cy)"));
			SystemMessage(TV(L"<color=green>VLCTCPro<color> : <color=red> Hành trang đã đầy cần về thành<color> (!cy)"));
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
