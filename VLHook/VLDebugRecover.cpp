#include "stdafx.h"
#include "../GameDef.h"
#include "GameBase.h"
#include "JxCore.h"
#include "S3Client.h"
#include "VLClasses.h"
#include "VLDebug.h"

// Tab Hoi phuc cua cua so DEBUG: cac ham game dung trong VLRecover.cpp (an thuoc, sua do, ve thanh, buff Nga Mi, X2)

#define SDT_NAME	"S\xAD \xAE\xE5 thi\xD5p"	// "Su do thiep" (TCVN3) - giong VLRecover.cpp
#define VAN_NAME	"v\xB9n"					// "van" trong gia sua do

static const char* s_szMedicine[medicine_detailnum] = { "mau", "mana", "ca hai", "the luc", "giai doc" };
static const char* s_szFullRoom[] = { "do 2 o (2x1 + 1x2)", "do 3 o (1x3)", "do 4 o (2x2)", "do 6 o (2x3)" };

static KItem* GetItem(int nItemIdx)
{
	return (nItemIdx > 0 && nItemIdx < MAX_ITM) ? (KItem*)(dwItmBaseAdd + nItemIdx * ITM_DATA_SIZE) : NULL;
}

// Giong dieu kien chon thuoc trong KPlayer::EatMedicine
static BOOL IsMedicineFor(KItem *pItem, int nDetailType)
{
	if (ComAttr.nItemGenre == item_medicine)
		return (ComAttr.nDetailType == nDetailType) || (ComAttr.nDetailType == medicine_both && nDetailType <= medicine_mana);
	if (ComAttr.nItemGenre == item_special)
		return (nDetailType <= medicine_mana) && (ComAttr.nParticularType >= 0xD7 && ComAttr.nParticularType <= 0xD9);
	return FALSE;
}

static int OutLifeMana()
{
	KNpc *pPlayer = P_PLAYER;
	int nMaxHP = pPlayer->GetLifeMax(), nMaxMP = pPlayer->GetManaMax();
	DbgOut("Mau %d / %d (Max %d, MaxEx %d)  Mana %d / %d (Max %d, MaxEx %d)  Doc nTime %d\n",
		pPlayer->m_CurrentLife, nMaxHP, pPlayer->m_CurrentLifeMax, pPlayer->m_CurrentLifeMaxEx,
		pPlayer->m_CurrentMana, nMaxMP, pPlayer->m_CurrentManaMax, pPlayer->m_CurrentManaMaxEx, pPlayer->m_PoisonState.nTime);
	return nMaxMP;
}

static void RecoverStatus()
{
	if (!DbgCheckPlayer()) return;
	CVLRecover &r = theRecover;
	KNpc *pPlayer = P_PLAYER;

	DbgOut("m_cbSize = %d (VLAuto gui) - phan public = %d\n", r.m_cbSize, (int)offsetof(CVLRecover, m_bDoingBuff));
	DbgOut("Mau: m_bAutoHP %d (< %d, moi %ds)  Mana: m_bAutoMP %d (< %d, moi %ds)\n",
		r.m_bAutoHP, r.m_nAutoHPAmount, r.m_nAutoHPTimer, r.m_bAutoMP, r.m_nAutoMPAmount, r.m_nAutoMPTimer);
	DbgOut("Ve thanh khi: Mau < %d (%d)  Mana < %d (%d)  het binh mau %d  het binh mana %d\n",
		r.m_nAMByHPAmount, r.m_bAMByHP, r.m_nAMByMPAmount, r.m_bAMByMP, r.m_bAMByBlood, r.m_bAMByMana);
	DbgOut("              day hanh trang %d (%s)  tien > %d van (%d)  do ben < %d (%d)\n",
		r.m_bAMByFullRoom, (r.m_nAMByFullRoomVal >= 0 && r.m_nAMByFullRoomVal < 4) ? s_szFullRoom[r.m_nAMByFullRoomVal] : "?",
		r.m_nAMByMoneyAmount, r.m_bAMByMoney, r.m_nAMByDurAmount, r.m_bAMByDur);
	DbgOut("Giai doc %d  X2 (TTL) %d  Su do thiep %d  Nga Mi buff %d (< %d%%, ca doi %d)  Tu sua do %d\n",
		r.m_bUsePoison, r.m_bUseDoubleExp, r.m_bUseMaster, r.m_bIsNgaMiBuff, r.m_nNgaMiBuffPercent, r.m_bPartyBuff, r.m_bAutoRepair);
	DbgOut("Noi bo: m_bDoingBuff %d  m_bEmptyMana %d  Dem (50ms): HP %d  MP %d  AM %d  NM %d  PN %d\n",
		r.m_bDoingBuff, r.m_bEmptyMana, r.m_nHPBuffCount, r.m_nMPBuffCount, r.m_nAMBuffCount, r.m_nNMBuffCount, r.m_nPNBuffCount);
	int nMaxMP = OutLifeMana();
	DbgOut("Tien hanh trang %d  Phai %d  PKValue %d  EntrustMode %d  theQuest.m_nGenDoing %d\n",
		EQUIP_MONEY, g_pCurPlayer->m_nCurFaction, g_pCurPlayer->m_nPKValue, g_pCurPlayer->m_nEntrustMode, theQuest.m_nGenDoing);

	// Giong thu tu CVLRecover::Breathe (goi moi 50ms, ca khi tam dung)
	int nCurHP = pPlayer->m_CurrentLife, nCurMP = pPlayer->m_CurrentMana;
	DbgOut("\nBreathe() (50ms): ");
	if (g_NetworkOffline) { DbgOut("DUNG - g_NetworkOffline = %d\n", g_NetworkOffline); return; }
	DbgOut("CheckEquip%s%s -> ", r.m_bUseMaster ? " + OnUseSDT" : "", (r.m_bUseDoubleExp || theQuest.m_bUseDoubleExp) ? " + OnUseTTL" : "");
	if (!pPlayer->m_FightMode) { DbgOut("DUNG - trong thanh (chi an thuoc khi m_FightMode)\n"); return; }
	if (nMaxMP == 0) { DbgOut("LOI: GetManaMax() = 0 => 'nCurMP * 100 / nMaxMP' chia cho 0 (crash)!\n"); return; }
	if (r.m_bAutoHP && nCurHP < r.m_nAutoHPAmount)
		DbgOut("AN MAU %s -> ", r.m_nHPBuffCount >= r.m_nAutoHPTimer * 20 ? "ngay" : "(dang cho dem)");
	if (r.m_bAutoMP && nCurMP < r.m_nAutoMPAmount)
		DbgOut("AN MANA %s -> ", r.m_nMPBuffCount >= r.m_nAutoMPTimer * 20 ? "ngay" : "(dang cho dem)");
	if ((r.m_bAMByHP && nCurHP < r.m_nAMByHPAmount) || (r.m_bAMByMP && nCurMP < r.m_nAMByMPAmount))
	{
		DbgOut("VE THANH (OnUseAM khan cap) - mau/mana duoi muc\n");
		return;
	}
	DbgOut("CheckInventory + CheckPKQuest%s%s\n", g_pCurPlayer->m_nCurFaction == 4 ? " + DoNgaMiBuff" : "",
		(r.m_bUsePoison && pPlayer->m_PoisonState.nTime && nCurHP < pPlayer->GetLifeMax() * 80 / 100) ? " + GIAI DOC" : "");
	if (theQuest.m_nGenDoing == qt_dopk && g_pCurPlayer->m_nPKValue)
		DbgOut("CheckPKQuest: dang lam nhiem vu PK va PKValue %d => VE THANH\n", g_pCurPlayer->m_nPKValue);
	g_DbgReply.bOK = TRUE;
}

static void MedicineList()
{
	if (!DbgCheckPlayer()) return;
	KInventory &room = EQUIP_ROOM;
	int nFirst[medicine_detailnum] = { 0 }, nCount[medicine_detailnum] = { 0 };
	DbgOut("Thuoc trong hanh trang (EQUIP_ROOM %dx%d, thu tu EatMedicine duyet):\n", room.m_nWidth, room.m_nHeight);
	for (int j = 0; j < room.m_nHeight; j++)
	{
		for (int i = 0; i < room.m_nWidth; i++)
		{
			int nItemIdx = room.m_pArray[j * room.m_nWidth + i];
			KItem *pItem = GetItem(nItemIdx);
			if (!pItem) continue;
			BOOL bAny = FALSE;
			for (int t = 0; t < medicine_detailnum; t++)
			{
				if (!IsMedicineFor(pItem, t)) continue;
				if (!bAny)
					DbgOut("  (%d,%d) Idx %-3d G%d D%d P%-3d x%-3d %s =>", i, j, nItemIdx, ComAttr.nItemGenre, ComAttr.nDetailType,
						ComAttr.nParticularType, ComAttr.bStack ? pItem->m_nCurDur : 1, ComAttr.szItemName);
				DbgOut(" %s", s_szMedicine[t]);
				bAny = TRUE;
				nCount[t] += ComAttr.bStack ? pItem->m_nCurDur : 1;
				if (!nFirst[t]) nFirst[t] = nItemIdx;
			}
			if (bAny) DbgOut("\n");
		}
	}
	for (int t = 0; t < medicine_detailnum; t++)
	{
		KItem *pItem = GetItem(nFirst[t]);
		DbgOut("EatMedicine(%d %s): %d vien%s%s\n", t, s_szMedicine[t], nCount[t], pItem ? " - se dung " : " - KHONG CO", pItem ? ComAttr.szItemName : "");
	}
	g_DbgReply.nResult[0] = nCount[medicine_blood];
	g_DbgReply.nResult[1] = nCount[medicine_mana];
	g_DbgReply.bOK = TRUE;
}

static void EquipDur()
{
	if (!DbgCheckPlayer()) return;
	CVLRecover &r = theRecover;
	DbgOut("Trang bi (CheckEquip moi 30s): tien %d, tu sua %d khi Dur <= 80%% va +N < 6, ve thanh khi Dur < %d\n",
		EQUIP_MONEY, r.m_bAutoRepair, r.m_nAMByDurAmount);
	int nCount = 0;
	for (int i = 0; i < itempart_num; i++)
	{
		int nEquipIndex = g_pCurPlayer->m_ItemList.m_EquipItem[i];
		KItem *pItem = GetItem(nEquipIndex);
		if (!pItem) continue;
		nCount++;
		DbgOut("  [%2d] Idx %-3d Dur %d/%d +%d  %s", i, nEquipIndex, pItem->m_nCurDur, pItem->m_nMaxDur, pItem->m_nItemPlus, ComAttr.szItemName);
		if (pItem->m_nCurDur < 0) { DbgOut(" => khong co do ben\n"); continue; }
		if (pItem->m_nCurDur >= pItem->m_nMaxDur) { DbgOut(" => day\n"); continue; }
		int nPrice = pItem->GetRepairPrice();
		DbgOut("  GetRepairPrice %d", nPrice);
		if (pItem->m_nCurDur < r.m_nAMByDurAmount)
			DbgOut(" => SAP HONG: %s", (r.m_bAMByDur || pItem->m_nCurDur < 3) ? "ve thanh" : (i == itempart_weapon ? "tam dung Auto" : "chi bao"));
		else if (pItem->m_nCurDur <= pItem->m_nMaxDur * 8 / 10 && pItem->m_nItemPlus < 6)
			DbgOut(" => %s", nPrice <= EQUIP_MONEY ? "se tu sua (OnClickGShop)" : "khong du tien sua");
		DbgOut("\n");
	}
	DbgOut("Tong: %d trang bi\n", nCount);
	g_DbgReply.nResult[0] = nCount;
	g_DbgReply.bOK = TRUE;
}

static void MsgSelRepair()
{
	KUiMsgSel *pMsgSel = (KUiMsgSel*)GetPtrAddress(MSG_BASE_ADD);
	DbgOut("KUiMsgSel (MSG_BASE_ADD 0x%08X) = 0x%08X", MSG_BASE_ADD, (DWORD)pMsgSel);
	if (!pMsgSel) { DbgOut("\n"); return; }
	DbgOut("  IsVisible %d  m_nNumMessage %d\n", pMsgSel->IsVisible(), pMsgSel->m_nNumMessage);
	if (!pMsgSel->IsVisible()) { DbgOut("(mo menu sua do cua tho ren: bam 'OnClickGShop' hoac noi chuyen voi tho ren)\n"); g_DbgReply.bOK = TRUE; return; }
	DbgOut("InfoText: %s\n", pMsgSel->m_InfoText);
	for (int i = 0; i < pMsgSel->m_nNumMessage && i < 10; i++)
		DbgOut("  [%d] %s\n", i, (char*)pMsgSel->m_pMessages[i]->Msg);
	if (pMsgSel->m_nNumMessage >= 2)
	{
		char *szMsg1 = (char*)pMsgSel->m_pMessages[1]->Msg;
		if (strstr(szMsg1, FULL_REPAIR))
		{
			char *szPriceV = szMsg1 + strlen(FULL_REPAIR);
			int nPrice = atoi(szPriceV);
			char *szPriceL = strstr(szPriceV, VAN_NAME);
			if (szPriceL) nPrice = nPrice * 10000 + atoi(szPriceL + 3);
			DbgOut("Dong [1] la '%s' - gia doc duoc %d, tien %d => CheckEquip se chon [%d]%s\n", FULL_REPAIR, nPrice, EQUIP_MONEY,
				nPrice <= EQUIP_MONEY ? 1 : 3, pMsgSel->m_nNumMessage >= 4 ? "" : " (nhung can >= 4 dong moi xu ly)");
			g_DbgReply.nResult[0] = nPrice;
		}
		else
			DbgOut("Dong [1] khong bat dau bang '%s' - khong phai menu sua do\n", FULL_REPAIR);
	}
	g_DbgReply.bOK = TRUE;
}

static void AMCheck()
{
	if (!DbgCheckPlayer()) return;
	CVLRecover &r = theRecover;
	KUiPlayerBar *pUiPlayerBar = (KUiPlayerBar*)GetPtrAddress(UPB_BASE_ADD);
	int nAM = g_pCurPlayer->GetItemsCount(AMULET_PAPER), nAMX = g_pCurPlayer->GetItemsCount(AMULETX_PAPER);
	DbgOut("KUiPlayerBar (UPB_BASE_ADD) = 0x%08X  m_nHK3Gengre (o phim tat 3) %d\n", (DWORD)pUiPlayerBar, pUiPlayerBar ? pUiPlayerBar->m_nHK3Gengre : 0);
	DbgOut("'%s' %d  '%s' %d  m_nAMBuffCount %d / %d\n", AMULET_PAPER, nAM, AMULETX_PAPER, nAMX, r.m_nAMBuffCount, MAX_BUFF_COUNT);
	DbgOut("MapID %d  theMap.m_nTrainMapID %d  theBasic.m_bExitWhenAM %d  theMain.m_bSilentMode %d\n",
		*g_pMapID, theMap.m_nTrainMapID, theBasic.m_bExitWhenAM, theMain.m_bSilentMode);
	DbgOut("OnUseAM() se: ");
	if (!pUiPlayerBar) DbgOut("KHONG lam gi - KUiPlayerBar = NULL\n");
	else if (r.m_nAMBuffCount < MAX_BUFF_COUNT) DbgOut("cho (m_nAMBuffCount %d < %d, vua dung AM)\n", r.m_nAMBuffCount, MAX_BUFF_COUNT);
	else if (*g_pMapID == city_visondao) DbgOut("o Vi Son Dao: khan cap => thoat game, thuong => cam co ve tam dao\n");
	else if (*g_pMapID >= 387 && *g_pMapID <= 395) DbgOut("map 387..395 - khong lam gi\n");
	else if (nAM) DbgOut("UseItem('%s')\n", AMULET_PAPER);
	else if (nAMX) DbgOut("UseItem('%s')\n", AMULETX_PAPER);
	else if (pUiPlayerBar->m_nHK3Gengre) DbgOut("ShortcutUseItem(2) - o phim tat 3\n");
	else DbgOut("THOAT GAME (khong con AM)\n");

	// CheckInventory
	POINT itmPos;
	BOOL bHaveRoom = FALSE;
	switch (r.m_nAMByFullRoomVal)
	{
	case 0: bHaveRoom = EQUIP_ROOM.FindRoom(2, 1, &itmPos) && EQUIP_ROOM.FindRoom(1, 2, &itmPos); break;
	case 1: bHaveRoom = EQUIP_ROOM.FindRoom(1, 3, &itmPos); break;
	case 2: bHaveRoom = EQUIP_ROOM.FindRoom(2, 2, &itmPos); break;
	case 3: bHaveRoom = EQUIP_ROOM.FindRoom(2, 3, &itmPos); break;
	}
	DbgOut("CheckInventory (moi 30s): tien %d %s %d van%s;  cho trong cho %s: %s%s\n",
		EQUIP_MONEY, EQUIP_MONEY > r.m_nAMByMoneyAmount * 10000 ? ">" : "<=", r.m_nAMByMoneyAmount,
		(r.m_bAMByMoney && EQUIP_MONEY > r.m_nAMByMoneyAmount * 10000) ? " => VE THANH" : "",
		(r.m_nAMByFullRoomVal >= 0 && r.m_nAMByFullRoomVal < 4) ? s_szFullRoom[r.m_nAMByFullRoomVal] : "?",
		bHaveRoom ? "con" : "HET", (r.m_bAMByFullRoom && !bHaveRoom) ? " => VE THANH" : "");
	g_DbgReply.nResult[0] = nAM;
	g_DbgReply.nResult[1] = nAMX;
	g_DbgReply.bOK = TRUE;
}

static void NgaMiInfo()
{
	if (!DbgCheckPlayer()) return;
	CVLRecover &r = theRecover;
	KSkill *pSkill = (KSkill*)(SKL_BASE_ADD + (NGAMY_BUFF_SKILLID - 1) * SKL_DATA_SIZE);
	int nLevel = 0;
	for (int i = 0; i < MAX_NPCSKILL; i++)
		if (P_PLAYER->m_Skills[i].SkillId == NGAMY_BUFF_SKILLID) nLevel = P_PLAYER->m_Skills[i].SkillLevel;
	DbgOut("Phai %d (%s)  NGAMY_BUFF_SKILLID %d = %s, cap %d  m_bIsNgaMiBuff %d (< %d%%)  ca doi %d  m_nNMBuffCount %d / %d\n",
		g_pCurPlayer->m_nCurFaction, g_pCurPlayer->m_nCurFaction == 4 ? "Nga Mi" : "KHONG phai Nga Mi - DoNgaMiBuff khong chay",
		NGAMY_BUFF_SKILLID, pSkill->m_szSkillName, nLevel, r.m_bIsNgaMiBuff, r.m_nNgaMiBuffPercent, r.m_bPartyBuff, r.m_nNMBuffCount, MAX_NMBF_COUNT);
	int nMaxHP = P_PLAYER->GetLifeMax();
	DbgOut("Ban than: %d / %d = %d%%%s\n", P_PLAYER->m_CurrentLife, nMaxHP, nMaxHP ? P_PLAYER->m_CurrentLife * 100 / nMaxHP : 0,
		(nMaxHP && P_PLAYER->m_CurrentLife * 100 / nMaxHP < r.m_nNgaMiBuffPercent) ? " => BUFF MINH" : "");

	int nPlayerX, nPlayerY, nX, nY;
	P_PLAYER->GetMapPos(&nPlayerX, &nPlayerY);
	DbgOut("g_pTeam m_nMemNum %d (DoNgaMiBuff duyet m_nMember[0..m_nMemNum], tam %d):\n", g_pTeam ? g_pTeam->m_nMemNum : -1, NGAMY_BUFF_RADIUS);
	if (!g_pTeam) { g_DbgReply.bOK = TRUE; return; }
	for (int j = 0; j <= g_pTeam->m_nMemNum && j < MAX_TEAM_MEMBER; j++)
	{
		DbgOut("  [%d] ID %u %s", j, g_pTeam->m_nMember[j], g_pTeam->m_szMemName[j]);
		for (int i = 1; i < MAX_NPC; i++)
		{
			KNpc *pNpc = g_pNpcArray[i];
			if (!pNpc->IsExist() || pNpc->m_dwID != (DWORD)g_pTeam->m_nMember[j]) continue;
			pNpc->GetMapPos(&nX, &nY);
			int dZ = __distance(nPlayerX, nPlayerY, nX, nY);
			// Code goc so m_CurrentLife voi % (dung neu client chi nhan % mau cua nguoi khac)
			DbgOut(" = Npc[%d] Life %d (LifeMax %d) dZ %d%s", i, pNpc->m_CurrentLife, pNpc->GetLifeMax(), dZ,
				(dZ <= NGAMY_BUFF_RADIUS && pNpc->m_CurrentLife < r.m_nNgaMiBuffPercent) ? " => BUFF" : "");
			break;
		}
		DbgOut("\n");
	}
	g_DbgReply.bOK = TRUE;
}

static void X2Info()
{
	if (!DbgCheckPlayer()) return;
	theRecover.LoadX2List();
	DbgOut("LoadX2List (Maps\\X2Items.txt, MAX_X2_ITEMS %d):\n", MAX_X2_ITEMS);
	int nFirst = -1;
	for (int i = 0; i < MAX_X2_ITEMS && theRecover.m_szX2Items[i][0]; i++)
	{
		int nCount = g_pCurPlayer->GetItemsCount(theRecover.m_szX2Items[i]);
		DbgOut("  [%d] %s - co %d\n", i, theRecover.m_szX2Items[i], nCount);
		if (nCount >= 1 && nFirst < 0) nFirst = i;
	}
	int nState = 0;
	for (KStateNode *pNode = (KStateNode*)P_PLAYER->m_pHeadNode; pNode; pNode = (KStateNode*)pNode->GetNext())
		if (pNode->m_SkillID == 0x01B8 || pNode->m_SkillID == 0x01C3 || pNode->m_SkillID == 0x037C) { nState = pNode->m_SkillID; break; }
	DbgOut("Trang thai X2 (SkillID 0x1B8/0x1C3/0x37C): %s", nState ? "DANG CO" : "khong");
	if (nState) DbgOut(" (%d)", nState);
	DbgOut("\nOnUseTTL (m_bUseDoubleExp %d / theQuest %d) se: %s%s\n", theRecover.m_bUseDoubleExp, theQuest.m_bUseDoubleExp,
		nState ? "cho het trang thai" : (nFirst < 0 ? "khong co vat pham X2" : (g_pCurPlayer->m_nEntrustMode ? "tam ngung uy thac roi an " : "an ")),
		(!nState && nFirst >= 0) ? theRecover.m_szX2Items[nFirst] : "");
	DbgOut("'%s' %d  EntrustMode %d  theUtil.m_bPausingEntrust %d (OnUseSDT)\n", SDT_NAME, g_pCurPlayer->GetItemsCount(SDT_NAME),
		g_pCurPlayer->m_nEntrustMode, theUtil.m_bPausingEntrust);
	g_DbgReply.nResult[0] = nFirst;
	g_DbgReply.bOK = TRUE;
}

static void RepairOne(int nItemIdx)
{
	if (!DbgCheckPlayer()) return;
	KItem *pItem = GetItem(nItemIdx);
	if (!pItem) { DbgOut("LOI: Idx = %d (lay o nut 'Do ben trang bi')\n", nItemIdx); return; }
	DbgOut("Truoc: Dur %d/%d  tien %d  GetRepairPrice %d\n", pItem->m_nCurDur, pItem->m_nMaxDur, EQUIP_MONEY, pItem->GetRepairPrice());
	g_pCurPlayer->RepairItem(nItemIdx);
	DbgOut("RepairItem(%d) %s - doi server; bam 'Do ben trang bi' de xem lai (can dang mo cua hang tho ren)\n", nItemIdx, ComAttr.szItemName);
	g_DbgReply.bOK = TRUE;
}

BOOL DebugRecoverCmd(const DEBUG_PARAM *pParam)
{
	const int *P = pParam->nParam;
	switch (pParam->nCmd)
	{
	case dbg_rc_status:
		RecoverStatus();
		break;
	case dbg_rc_medicine:
		MedicineList();
		break;
	case dbg_rc_eat:
		if (!DbgCheckPlayer()) break;
		if (P[2] < 0 || P[2] >= medicine_detailnum) { DbgOut("LOI: Idx 0 mau, 1 mana, 2 ca hai, 3 the luc, 4 giai doc\n"); break; }
		OutLifeMana();
		g_DbgReply.nResult[0] = g_pCurPlayer->EatMedicine(P[2]);
		DbgOut("EatMedicine(%d %s) = %d (bam 'Trang thai' lai de xem mau/mana tang)\n", P[2], s_szMedicine[P[2]], g_DbgReply.nResult[0]);
		g_DbgReply.bOK = TRUE;
		break;
	case dbg_rc_equipdur:
		EquipDur();
		break;
	case dbg_rc_gshop:
		{
			KUiPlayerBar *pUiPlayerBar = (KUiPlayerBar*)GetPtrAddress(UPB_BASE_ADD);
			if (!pUiPlayerBar) { DbgOut("LOI: KUiPlayerBar = NULL\n"); break; }
			pUiPlayerBar->OnClickGShop();
			DbgOut("KUiPlayerBar::OnClickGShop() - mo menu tho ren; bam 'Menu sua do' de xem\n");
			g_DbgReply.bOK = TRUE;
		}
		break;
	case dbg_rc_msgsel:
		MsgSelRepair();
		break;
	case dbg_rc_msgclick:
		{
			KUiMsgSel *pMsgSel = (KUiMsgSel*)GetPtrAddress(MSG_BASE_ADD);
			if (!pMsgSel || !pMsgSel->IsVisible()) { DbgOut("LOI: KUiMsgSel khong hien\n"); break; }
			if (P[2] < 0 || P[2] >= pMsgSel->m_nNumMessage) { DbgOut("LOI: Idx %d ngoai 0..%d\n", P[2], pMsgSel->m_nNumMessage - 1); break; }
			pMsgSel->OnClickMsg(P[2]);
			DbgOut("KUiMsgSel::OnClickMsg(%d)\n", P[2]);
			g_DbgReply.bOK = TRUE;
		}
		break;
	case dbg_rc_repair:
		RepairOne(P[2]);
		break;
	case dbg_rc_amcheck:
		AMCheck();
		break;
	case dbg_rc_useam:
		if (!DbgCheckPlayer()) break;
		theRecover.m_nAMBuffCount = MAX_BUFF_COUNT;
		theRecover.OnUseAM(FALSE);
		DbgOut("OnUseAM() (m_nAMBuffCount = %d truoc khi goi) -> m_nAMBuffCount %d\n", MAX_BUFF_COUNT, theRecover.m_nAMBuffCount);
		g_DbgReply.bOK = TRUE;
		break;
	case dbg_rc_ngami:
		NgaMiInfo();
		break;
	case dbg_rc_x2:
		X2Info();
		break;
	case dbg_rc_usex2:
		if (!DbgCheckPlayer()) break;
		{
			const char *szName = pParam->szParam[0] ? pParam->szParam : theRecover.m_szX2Items[0];
			g_DbgReply.nResult[0] = g_pCurPlayer->UseItem(szName);
			DbgOut("UseItem('%s') = %d\n", szName, g_DbgReply.nResult[0]);
			g_DbgReply.bOK = TRUE;
		}
		break;
	case dbg_rc_usesdt:
		if (!DbgCheckPlayer()) break;
		g_DbgReply.nResult[0] = g_pCurPlayer->UseItem(SDT_NAME);
		DbgOut("UseItem('%s') = %d - neu co menu 'Huy lua chon...' thi OnUseSDT bam [0]\n", SDT_NAME, g_DbgReply.nResult[0]);
		g_DbgReply.bOK = TRUE;
		break;
	default:
		return FALSE;
	}
	return TRUE;
}
