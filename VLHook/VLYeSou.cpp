#include "stdafx.h"
#include "../GameDef.h"
#include "GameBase.h"
#include "JxCore.h"
#include "S3Client.h"
#include "VLClasses.h"
#include "MagicAttrib.h"

#define PLAYER_SHOP_LOG 0 // = 1 bật ghi log danh sách đồ trong shop của người chơi khi mua đồ làm nv dã tẩu

#define MAX_TIME_WAIT 10

const int Map9xID[MAX_9XMAP_NUM] = { 124, 93, 75, 144, 321, 322, 225, 226, 227, 0 };

#define BIKIP_NUM 28
const char* BikipMP[BIKIP_NUM] =
{ TV(L"Thiên Vương Chùy"), TV(L"Thiên Vương Thương"), TV(L"Thiên Vương Đao"),
	TV(L"Thiếu Lâm Quyền"), TV(L"Thiếu Lâm Côn"), TV(L"Thiếu Lâm Đao"),
	TV(L"Phi Đao thuật."), TV(L"Phi Tiêu thuật."), TV(L"Tụ Tiễn thuật."), TV(L"Hãm Tĩnh thuật."),
	TV(L"Ngũ Độc Chưởng"), TV(L"Ngũ Độc Đao"), TV(L"Ngũ Độc Nhiếp"),
	TV(L"Diệt Kiếm Mật Tịch"), TV(L"Nga Mi  Phật Quang"), TV(L"Phổ Độ Mật Tịch"),
	TV(L"Thúy Yên Đao pháp"), TV(L"Thúy Yên Song đao"), TV(L"Cái Bang Chưởng"), TV(L", Cái Bang Côn"),
	TV(L"Vân Long Kích"), TV(L"Lưu Tinh. Đao pháp"), TV(L"Nhiếp Hồn. Chú thuật"),
	TV(L"Thái Cực Quyền"), TV(L"Thái Cực Kiếm"), TV(L"Ngự Phong thuật"), TV(L"Ngự Lôi thuật"), TV(L"Ngự Tâm thuật") };

#define HKIM_NUM 27
const char* HoangKimMP[HKIM_NUM] =
{ TV(L"Đồng Cừu"), TV(L"Địch Khái"), TV(L"Vô Gian"), TV(L"Vô Ma"), TV(L"Vô Trần"), TV(L"Chú Phược"), "U Lung", TV(L"Minh ảo"), TV(L"Cập Phong"),
	TV(L"Lăng Nhạc"), TV(L"Kế Nghiệp"), TV(L"Hám Thiên"), TV(L"Ngự Long"), TV(L"Vụ ảo"), TV(L"Sương Tinh"), TV(L"Lôi Khung"), TV(L"Địa Phách"), TV(L"Băng Hàn"),
	TV(L"Sâm Hoang"), TV(L"Tê Hoàng"), TV(L"Bích Hải"), TV(L"Ma Hoàng"), TV(L"Ma Thị"), TV(L"Ma Sát"), TV(L"Mộng Long"), TV(L"Tứ Không"), TV(L"Phục Ma") };

// 15-10: menu chính của NPC Dã Tẩu (hiện trước hộp mô tả nhiệm vụ)
// Chuỗi TCVN3 viết bằng escape hex để đúng byte dù file lưu TCVN3 hay UTF-8
const char* YS_MENU_GETQUEST = TV(L"Nhận nhiệm vụ");	// "Nhận nhiệm vụ"
const char* YS_MENU_RETURNQUEST = TV(L"Hoàn thành nhiệm vụ");	// "Hoàn thành nhiệm vụ"
const char* YS_CANCEL_QUEST = TV(L"Đã hủy nhiệm vụ");	// "Hoàn thành nhiệm vụ"

// Gop cac dong cua hop thoai thanh 1 chuoi de QTRACE
template <class T> static const char* QTJoinMsg(T* pMsgSel)
{
	static char szJoin[200];
	szJoin[0] = 0;
	for (int i = 0; i < pMsgSel->m_nNumMessage; i++)
	{
		size_t nLen = strlen(szJoin);
		if (nLen >= sizeof(szJoin) - 1) break;
		_snprintf(szJoin + nLen, sizeof(szJoin) - 1 - nLen, "%s[%d]%s", i ? " | " : "", i, pMsgSel->m_pMessages[i]->Msg);
		szJoin[sizeof(szJoin) - 1] = 0;
	}
	return szJoin;
}

// 15-10: thông báo "Cần kích hoạt thẻ Vip mới có thể nhận và làm nhiệm vụ"
// Tách "k\xdd" "ch": nếu viết liền "\xddch" thì trình biên dịch đọc cả "c" vào mã hex
const char* YS_NEED_VIP = TV(L"kích hoạt");	// "kích hoạt", phải đi kèm chữ Vip

static BOOL IsNeedVipText(const char* szText)
{
	return szText && strstr(szText, YS_NEED_VIP) &&
		(strstr(szText, "Vip") || strstr(szText, "VIP") || strstr(szText, "vip"));
}
//IsCancelQuestText
static BOOL IsCancelQuestText(const char* szText)
{
	return szText && strstr(szText, YS_CANCEL_QUEST) ;
}
// Tim thong bao can the Vip trong hop thoai (loi NPC + cac dong chon)
static BOOL HasNeedVipMsg(KUiMsgSel* pMsgSel)
{
	char szInfo[sizeof(pMsgSel->m_InfoText)];
	GameTextPlain(pMsgSel->m_InfoText, sizeof(pMsgSel->m_InfoText), szInfo, sizeof(szInfo));
	if (IsNeedVipText(szInfo)) return TRUE;
	for (int i = 0; i < pMsgSel->m_nNumMessage; i++)
		if (IsNeedVipText(pMsgSel->m_pMessages[i]->Msg)) return TRUE;
	return 0;
}

static BOOL HasNeedVipMsg(KUiMsgSel2* pMsgSel2)
{
	if (IsNeedVipText(pMsgSel2->GetQuestionText())) return TRUE;
	for (int i = 0; i < pMsgSel2->m_nNumMessage; i++)
		if (IsNeedVipText(pMsgSel2->m_pMessages[i]->Msg)) return TRUE;
	return 0;
}

const char* CANCEL_BY_USER = TV(L"Hủy nhiệm vụ hoặc dùng Sơ đề theo thiết lập của người chơi!");

void CVLQuest::BreathYS()
{
	if (m_bNoVipCard && m_nYSDoing != ys_suspending)
		m_nYSDoing = ys_suspending;	// Chua co the Vip: khong lam Da Tau nua

	// ngưng nhiệm vụ Da Tẩu, không làm nữa
	// phải reset lại sau 12h đêm, nếu không sẽ bị treo ở trạng thái ngưng nhiệm vụ
	if (m_nYSDoing == ys_suspending) 
	{
		m_nGenDoing = qt_suspending;
		m_nDoQuestStep = 0;
		theMap.ResetShoping();
		return;
	}

	//g_pCurPlayer->GetTaskVal đã không sử đụng được nữa
	//m_nQuestStatus = g_pCurPlayer->GetTaskVal(ys_taskstatus);

	QTRACE("BreathYS: YS=%s Status=%d QID=%d Step=%d Map=%d", QTName_YS(m_nYSDoing), m_nQuestStatus,
		g_pCurPlayer->GetTaskVal(ys_majorid), m_nDoQuestStep, *g_pMapID);

	switch (m_nQuestStatus)
	{
	case 1:
		if (m_nYSDoing == ys_none)
			GetNewQuest();
		else if (m_nYSDoing == ys_doing)
			DoCurrentQuest();
		else if (m_nYSDoing == ys_return)
			ReturnQuest();
		else if (m_nYSDoing == ys_cancel)
			CancelQuest();
		break;
	case 2:
		ChoosePrize();
		break;
	case 3:
		if (m_nYSDoing == ys_none)
		{
			if (YSPreparing()) return;
		}
		GetNewQuest();
		break;
	default:
		GetNewQuest();
	}
}

void CVLQuest::DoCurrentQuest()
{
	//CTC 15-10: nhiệm vụ Da Tẩu mới không dùng g_pCurPlayer->GetTaskVal nữa
	switch (m_nYesouKind)
	{
	case ys_attackNpc:
		DoQuestAttackNpc();
		break;
	case ys_ItemOption:
		DoQuestItemOption();
		break;
	default:
		break;
	}
	return; 
	// code lỗi thời không dùng tới

	m_nQuestID = g_pCurPlayer->GetTaskVal(ys_majorid);
	switch (m_nQuestID)
	{
	case 1:	// Buy an Item from NPC
		DoQuest01();
		break;
	case 2: // Find an Item - no return
	case 3: // Find an Item - return
		DoQuest0203();
		break;
	case 4: // Find special maps
		DoQuest04();
		break;
	case 5: // Increase some values
		DoQuest05();
		break;
	case 6: // Find SHXT maps
		DoQuest06();
		break;
	}
}

void CVLQuest::CancelQuest()
{
	QTRACE("CancelQuest: Map=%d Shoping=%d InTownOnly=%d", *g_pMapID, theMap.m_bDoShoping, m_bInTownOnly);
	// CancelQuest in town
	if (m_bInTownOnly || ((*g_pMapID) >= city_banghoimin && (*g_pMapID) <= city_banghoimax))
	{
		int nSubCityID = theMain.GetSubCity(*g_pMapID);
		if (theMap.ChangeMap(nSubCityID)) return;
	}
	int nCurMapID = *g_pMapID;
	if (nCurMapID != city_tuongduong)
	{
		if (theMap.ChangeMap(city_tuongduong)) return;
	}
	if (!theMap.m_bDoShoping && theMap.RunningToPos(theMap.NpcPoints[npc_YeSou])) return;
	theMap.m_bDoShoping = TRUE;
	// check đã bật cửa sổ thông báo chưa
	if (g_UiInformation2 && g_UiInformation2->IsVisible())
	{
		/*
		* g_UiInformation2 = 0x007C6CC8  IsVisible = 1
		  Text: Cần kích hoạt thẻ Vip mới có thể nhận và làm nhiệm vụ..
		  Text [TCVN3]: CÇn kÝch ho¹t thÎ Vip míi cã thÓ nhËn vµ lµm nhiÖm vô..
		*/
		char szInfo[sizeof(g_UiInformation2->m_szInfoText) + 1];
		// loại bỏ các mã màu nếu có
		GameTextPlain(g_UiInformation2->m_szInfoText, sizeof(g_UiInformation2->m_szInfoText), szInfo, sizeof(szInfo));
		int bCancelQuest = IsCancelQuestText(szInfo);
		QTRACE("GetNewQuest: Information2: %s (NeedVip = %d)", szInfo, bCancelQuest);
		if (bCancelQuest)
		{
			m_nQuestStatus = 1;
			m_nYSDoing = ys_none;
			CloseAllWindows(0);
			return;
		}
		else
			CloseAllWindows(0);
	}

	KUiMsgSel2* pMsgSel2 = (KUiMsgSel2*)GetPtrAddress(MSG2_BASE_ADD);
	if (pMsgSel2 && pMsgSel2->IsVisible())
	{
		QTRACE("CancelQuest: MsgSel2 %d dong: %s", pMsgSel2->m_nNumMessage, QTJoinMsg(pMsgSel2));
		if (pMsgSel2->m_nNumMessage == 4)
		{
			/*
			* Lua chon[0]: Hoàn thành nhiệm vụ thứ 0
			  Lua chon[0] [TCVN3]: Hoµn thµnh nhiÖm vô thø 0
			  Lua chon[1]: Hủy nhiệm vụ
			  Lua chon[1] [TCVN3]: Hñy nhiÖm vô
			  Lua chon[2]: Xem tình hình chuỗi nhiệm vụ
			  Lua chon[2] [TCVN3]: Xem t×nh h×nh chuçi nhiÖm vô
			  Lua chon[3]:  Kết thúc đối thoại!
			  Lua chon[3] [TCVN3]:  KÕt thóc ®èi tho¹i!
			*/
			size_t nKeyLen = strlen(TV(L"Hủy nhiệm vụ"));
			for (int i = 0; i < pMsgSel2->m_nNumMessage; i++)
			{
				if (!strncmp(pMsgSel2->m_pMessages[i]->Msg, TV(L"Hủy nhiệm vụ"), nKeyLen))
				{					
					//Lua chon[2]: Xem tình hình chuỗi nhiệm vụ
					QTRACE(TV(L"CancelQuest: chon dong [%d] Hủy nhiệm vụ"), i);
					pMsgSel2->OnClickMsg(i);
					m_nQuestStatus = 1;
					m_nYSDoing = ys_none;
					CloseAllWindows(0);
					break;					
				}
			}
		}		
		return;
	}
	/**
	KUiMsgSel* pMsgSel = (KUiMsgSel*)GetPtrAddress(MSG_BASE_ADD);
	if (pMsgSel && pMsgSel->IsVisible())
	{
		QTRACE("CancelQuest: MsgSel %d dong: %s", pMsgSel->m_nNumMessage, QTJoinMsg(pMsgSel));
		if (pMsgSel->m_nNumMessage == 2)
		{
			if (strstr(pMsgSel->m_pMessages[0]->Msg, "§óng, ta kh«ng muèn lµm nhiÖm vô"))
			{
				int nCancelChance = g_pCurPlayer->GetTaskVal(ys_taskccl);
				char szLogInfo[80];
				sprintf(szLogInfo, "Hñy nhiÖm vô b»ng c¬ héi hiÖn cã, sè c¬ héi cßn l¹i lµ %d.\n", nCancelChance - 1);
				SaveYSLog(szLogInfo);
				pMsgSel->OnClickMsg(0);
				if (m_nRemainQuest) m_nRemainQuest--;
				m_nYSDoing = ys_none;
				theMap.m_nTimeWait = 2;
			}
		}
		else if (pMsgSel->m_nNumMessage == 3)
		{
			if (strstr(pMsgSel->m_pMessages[1]->Msg, "100 m¶nh s¬n Hµ X· T¾c"))
			{
				int nMapSHXT = g_pCurPlayer->GetTaskVal(ys_shxtmap);
				if (nMapSHXT >= 100)
				{
					char szLogInfo[80];
					sprintf(szLogInfo, "Hñy nhiÖm vô b»ng 100 m¶nh SHXT, sè m¶nh SHXT cßn l¹i lµ %d.\n", nMapSHXT - 100);
					SaveYSLog(szLogInfo);
					pMsgSel->OnClickMsg(1);
					if (m_nRemainQuest) m_nRemainQuest--;
					m_nYSDoing = ys_none;
				}
				else
				{
					pMsgSel->OnClickMsg(2);
					m_nYSDoing = ys_suspending;
				}
				theMap.m_nTimeWait = 2;
			}
		}
		else if (pMsgSel->m_nNumMessage == 4)
		{
			if (strstr(pMsgSel->m_pMessages[2]->Msg, "NhiÖm vô lÇn nµy khã qu¸"))
				pMsgSel->OnClickMsg(2);
		}

		return;
	}
	/**/
	memset(m_szYSQuest, 0, sizeof(m_szYSQuest));
	theMap.ClickToNpc(theMap.NpcPoints[npc_YeSou].x, theMap.NpcPoints[npc_YeSou].y);
}

//về trả nhiệm vụ
void CVLQuest::ReturnQuest()
{
	QTRACE("ReturnQuest: Map=%d Shoping=%d Arrange=%d", *g_pMapID, theMap.m_bDoShoping, m_nArrangeStep);
	// ReturnQuest in town
	if (m_bInTownOnly || ((*g_pMapID) >= city_banghoimin && (*g_pMapID) <= city_banghoimax))
	{
		int nSubCityID = theMain.GetSubCity(*g_pMapID);
		if (theMap.ChangeMap(nSubCityID)) return;
	}

	if (!theMap.m_bDoShoping && theMap.RunningToPos(theMap.NpcPoints[npc_YeSou]))
	{
		m_nArrangeStep = 1;
		return;
	}
	theMap.m_bDoShoping = TRUE;

	ArrangeEquipRoom();
	if (m_nArrangeStep) return;

	KUiMsgSel2* pMsgSel2 = (KUiMsgSel2*)GetPtrAddress(MSG2_BASE_ADD);
	if (pMsgSel2 && pMsgSel2->IsVisible())
	{
		QTRACE("ReturnQuest: MsgSel2 %d dong: %s", pMsgSel2->m_nNumMessage, QTJoinMsg(pMsgSel2));
		if (pMsgSel2->m_nNumMessage == 6)
		{
			if (strstr(pMsgSel2->m_pMessages[1]->Msg, TV(L"Ta đã hoàn thành nhiệm vụ")))
				pMsgSel2->OnClickMsg(1);
		}

		return;
	}

	KUiMsgSel* pMsgSel = (KUiMsgSel*)GetPtrAddress(MSG_BASE_ADD);
	if (pMsgSel && pMsgSel->IsVisible())
	{
		QTRACE("ReturnQuest: MsgSel %d dong: %s", pMsgSel->m_nNumMessage, QTJoinMsg(pMsgSel));
		if (pMsgSel->m_nNumMessage == 1)
		{
			if (strstr(pMsgSel->m_InfoText, TV(L"Những việc bạn làm chưa đạt đúng yêu cầu")))
			{
				m_nYSDoing = ys_suspending;
				SystemMessage(TV(L"<color=yellow>Vật phẩm nhiệm vụ không đúng!"));
				SaveYSLog(TV(L"Vật phẩm nhiệm vụ không đúng => Tạm ngưng!"));
			}
			pMsgSel->OnClickMsg(0);
			theMap.m_nTimeWait = 2;
		}
		else if (pMsgSel->m_nNumMessage == 4)
		{
			if (strstr(pMsgSel->m_pMessages[1]->Msg, TV(L"Ta đã hoàn thành nhiệm vụ")))
				pMsgSel->OnClickMsg(1);
		}

		return;
	}

	KUiYeSouBox* pYSBox = (KUiYeSouBox*)GetPtrAddress(YSB_BASE_ADD);
	if (pYSBox && pYSBox->IsVisible())
	{
		KInventory YeSouRoom = PLAYER_ROOM[room_yesou];

		if (YeSouRoom.m_pArray[0])
		{
			pYSBox->OnConfirm();
			theMap.ResetShoping(0);
		}
		else if (PLAY_HAND_IDX)
			g_pCurPlayer->SwitchItem(UOC_YESOU_BOX, 0, 0);
		else
		{
			//Find the Item
			PlayerItem theItem;
			if (CheckMyItem(&theItem, pos_equiproom, TRUE))
			{
				if (m_bReturnItem) m_itmEquip = theItem;
				KItem* pItem = (KItem*)(dwItmBaseAdd + theItem.nIdx * ITM_DATA_SIZE);
				g_pCurPlayer->SwitchItem(UOC_ITEM_TAKE_WITH, theItem.nX, theItem.nY);
			}
			else
			{
				pYSBox->OnConfirm(0);
				theMap.ResetShoping(0);
				m_nYSDoing = ys_suspending;
				SystemMessage(TV(L"<color=yellow>Không thấy vật phẩm nhiệm vụ!"));
				SaveYSLog(TV(L"Không thấy vật phẩm nhiệm vụ => Tạm ngưng!"));
			}
		}
		return;
	}

	if (PLAY_HAND_IDX)
	{
		POINT itmPos;
		KItem* pItem = (KItem*)(dwItmBaseAdd + PLAY_HAND_IDX * ITM_DATA_SIZE);
		if (EQUIP_ROOM.FindRoom(ComAttr.nWidth, ComAttr.nHeight, &itmPos, ComAttr.nItemGenre != item_equip))
			g_pCurPlayer->SwitchItem(UOC_ITEM_TAKE_WITH, itmPos.x, itmPos.y);
	}

	memset(m_szYSQuest, 0, sizeof(m_szYSQuest));

	theMap.ClickToNpc(theMap.NpcPoints[npc_YeSou].x, theMap.NpcPoints[npc_YeSou].y);
}
// chọn phần thưởng
void CVLQuest::ChoosePrize()
{
	QTRACE("ChoosePrize: Map=%d Shoping=%d", *g_pMapID, theMap.m_bDoShoping);
	// ReturnQuest in town
	if (m_bInTownOnly || ((*g_pMapID) >= city_banghoimin && (*g_pMapID) <= city_banghoimax))
	{
		int nSubCityID = theMain.GetSubCity(*g_pMapID);
		if (theMap.ChangeMap(nSubCityID)) return;
	}

	if (!theMap.m_bDoShoping && theMap.RunningToPos(theMap.NpcPoints[npc_YeSou])) return;
	theMap.m_bDoShoping = TRUE;

	char szPrize[3][80];
	long nValues[3] = { 0, 0, 0 };
	KUiPrizeSel* pPrzSel = (KUiPrizeSel*)GetPtrAddress(PZS_BASE_ADD);
	if (pPrzSel && pPrzSel->IsVisible())
	{
		char* szItemIcon;
		char* szItemText;
		long nMaxPrize = 0;
		int nMaxIdx = 0;
		for (int i = 0; i < 3; i++)
		{
			szItemIcon = pPrzSel->PrzItem[i].m_szIcon;
			szItemText = pPrzSel->PrzItem[i].m_szText + 9;	// strlen("NhËn ®­îc");
			strcpy(szPrize[i], szItemText);
			if (strstr(szItemIcon, "quxiao_icon")) // Huy NV
				nValues[i] = m_PrizeSelOpt.m_PrizeVal[prize_cohoihuy] * 10000;
			else if (strstr(szItemIcon, "wenhao_icon")) // Dau hoi
			{
				strcpy(szPrize[i], TV(L"Dấu hỏi (?)"));
				nValues[i] = m_PrizeSelOpt.m_PrizeVal[prize_dauhoi] * 10000;
			}
			else if (strstr(szItemIcon, "jinqian_icon")) // Kim tien
				nValues[i] = atol(szItemText);
			else if (strstr(szItemIcon, "jingyan_icon")) // Kinh nghiem
				nValues[i] = (int)(atof(szItemText) / 100 * m_PrizeSelOpt.m_PrizeVal[prize_kinhnghiem]);
			else if (strstr(szItemText, TV(L"điểm tích l")))
				nValues[i] = (int)(atof(szItemText) / 100 * m_PrizeSelOpt.m_PrizeVal[prize_kinhnghiem]);
			else
			{
				if (strstr(szItemText, TV(L"Nhu Tình Cân Quốc")))
					nValues[i] = m_PrizeSelOpt.m_PrizeVal[prize_nhutinhao];
				else if (strstr(szItemText, TV(L"Nhu Tình  Tuệ Tâm")))
					nValues[i] = m_PrizeSelOpt.m_PrizeVal[prize_nhutinhnb];
				else if (strstr(szItemText, TV(L"Nhu Tình")))
					nValues[i] = m_PrizeSelOpt.m_PrizeVal[prize_nhutinhot];
				else if (strstr(szItemText, TV(L"Hiệp Cốt Tình ý Kết")))
					nValues[i] = m_PrizeSelOpt.m_PrizeVal[prize_hiepcotnb];
				else if (strstr(szItemText, TV(L"Hiệp Cốt")))
					nValues[i] = m_PrizeSelOpt.m_PrizeVal[prize_hiepcotot];
				else if (strstr(szItemText, TV(L"An Bang Băng Tinh")))
					nValues[i] = m_PrizeSelOpt.m_PrizeVal[prize_anbangdc];
				else if (strstr(szItemText, "An Bang"))
					nValues[i] = m_PrizeSelOpt.m_PrizeVal[prize_anbangot];
				else if (strstr(szItemText, TV(L"Định Quốc Ô Sa")))
					nValues[i] = m_PrizeSelOpt.m_PrizeVal[prize_dinhquocmu];
				else if (strstr(szItemText, TV(L"Định Quốc")))
					nValues[i] = m_PrizeSelOpt.m_PrizeVal[prize_dinhquocot];
				else if (strstr(szItemIcon, "½±Àø»Æ½ð×°±¸")) // Hoang Kim mon phai
					nValues[i] = m_PrizeSelOpt.m_PrizeVal[prize_hoangkimmp];
				else if (strstr(szItemText, TV(L"Hoàng Kim"))) // Hoang Kim mon phai
					nValues[i] = m_PrizeSelOpt.m_PrizeVal[prize_hoangkimmp];
				else if (strstr(szItemText, TV(L"Thủy Tinh")))
					nValues[i] = m_PrizeSelOpt.m_PrizeVal[prize_ttthbt];
				else if (strstr(szItemText, TV(L"Tinh Hồng Bảo Thạch")))
					nValues[i] = m_PrizeSelOpt.m_PrizeVal[prize_ttthbt];
				else if (strstr(szItemText, TV(L"Thiết La Hán")))
					nValues[i] = m_PrizeSelOpt.m_PrizeVal[prize_tlhbntk];
				else if (strstr(szItemText, TV(L"Bàn Nhược Tâm Kinh")))
					nValues[i] = m_PrizeSelOpt.m_PrizeVal[prize_tlhbntk];
				else if (strstr(szItemText, TV(L"Bát Nhã Tâm Kinh")))
					nValues[i] = m_PrizeSelOpt.m_PrizeVal[prize_tlhbntk];
				else if (strstr(szItemText, TV(L"Võ Lâm Mật Tịch")))
					nValues[i] = m_PrizeSelOpt.m_PrizeVal[prize_vlmtttk];
				else if (strstr(szItemText, TV(L"Tẩy Tủy Kinh")))
					nValues[i] = m_PrizeSelOpt.m_PrizeVal[prize_vlmtttk];
				else if (strstr(szItemText, TV(L"Tiên Thảo Lộ")))
					nValues[i] = m_PrizeSelOpt.m_PrizeVal[prize_ttl];
				else if (strstr(szItemText, TV(L"Huyền Thiết")))
					nValues[i] = m_PrizeSelOpt.m_PrizeVal[prize_nkhoang];
				else if (strstr(szItemText, TV(L"Mật Ngân")))
					nValues[i] = m_PrizeSelOpt.m_PrizeVal[prize_nkhoang];
				else if (strstr(szItemText, "Chu Sa"))
					nValues[i] = m_PrizeSelOpt.m_PrizeVal[prize_nkhoang];
				else if (strstr(szItemText, TV(L"Khổng Tước")))
					nValues[i] = m_PrizeSelOpt.m_PrizeVal[prize_nthach];
				else if (strstr(szItemText, TV(L"Phù Dung")))
					nValues[i] = m_PrizeSelOpt.m_PrizeVal[prize_nthach];
				else if (strstr(szItemText, TV(L"Chung Nhũ")))
					nValues[i] = m_PrizeSelOpt.m_PrizeVal[prize_nthach];
				else if (strstr(szItemText, TV(L"Thiên sơn  Bảo Lộ")))
					nValues[i] = m_PrizeSelOpt.m_PrizeVal[prize_tsblo];
				else if (strstr(szItemText, TV(L"Bách Quả Lộ")))
					nValues[i] = m_PrizeSelOpt.m_PrizeVal[prize_bqualo];
				else if (strstr(szItemText, TV(L" hoàn")))						//§¹i Lùc hoµn ,
					nValues[i] = m_PrizeSelOpt.m_PrizeVal[prize_duochoan];
				else if (strstr(szItemText, TV(L"Pháo Hoa")))
					nValues[i] = m_PrizeSelOpt.m_PrizeVal[prize_phaohoa];
				else if (strstr(szItemText, TV(L"Tâm Tâm Tương")))
					nValues[i] = m_PrizeSelOpt.m_PrizeVal[prize_tttanhphu];
				else if (strstr(szItemText, TV(L"Phúc Duyên Lộ (Tiểu)")))
					nValues[i] = m_PrizeSelOpt.m_PrizeVal[prize_phucduyen];
				else if (strstr(szItemText, TV(L"Phúc Duyên Lộ (Trung)")))
					nValues[i] = m_PrizeSelOpt.m_PrizeVal[prize_phucduyen] * 2;
				else if (strstr(szItemText, TV(L"Phúc Duyên Lộ (Đại)")))
					nValues[i] = m_PrizeSelOpt.m_PrizeVal[prize_phucduyen] * 3;
				else if (strstr(szItemText, TV(L"Huyền Tinh Khoáng")))
					nValues[i] = m_PrizeSelOpt.m_PrizeVal[prize_huyentinh];
				else if (strstr(szItemText, TV(L"Huyền tinh cấp")) ||
					strstr(szItemText, TV(L"Huyền Tinh cấp")))
				{
					int nMinLevel = m_PrizeSelOpt.m_PrizeVal[prize_levelht] + 1;
					int nLevel = atoi(szItemText + 15);
					nValues[i] = 0;
					if (nLevel >= nMinLevel)
					{
						nValues[i] = m_PrizeSelOpt.m_PrizeVal[prize_huyentinh];
						for (int j = nMinLevel; j < nLevel; j++)
							nValues[i] *= 3;
					}
				}
				else
				{
					BOOL bFound = 0;
					for (int j = 0; j < BIKIP_NUM; j++)
					{
						if (strstr(szItemText, BikipMP[j]))
						{
							nValues[i] = m_PrizeSelOpt.m_PrizeVal[prize_bikipmp];
							bFound = TRUE;
							break;
						}
					}
					if (!bFound) for (int j = 0; j < HKIM_NUM; j++)
					{
						if (strstr(szItemText, HoangKimMP[j]))
						{
							nValues[i] = m_PrizeSelOpt.m_PrizeVal[prize_hoangkimmp];
							bFound = TRUE;
							break;
						}
					}
				}
				//Convert Value
				nValues[i] *= 10000;
			}
			if (nValues[i] > nMaxPrize)
			{
				nMaxIdx = i;
				nMaxPrize = nValues[i];
			}
		}
		SystemMessage(1, TV(L"<color=yellow>%s = %d\n  %s = %d\n  %s = %d\n<color=green>  Đã chọn: %s"),
			szPrize[0], nValues[0],
			szPrize[1], nValues[1],
			szPrize[2], nValues[2],
			szPrize[nMaxIdx]);
		char szMsg[256];
		sprintf(szMsg, TV(L"Phần thưởng: %s = %d, %s = %d, %s = %d. Đã chọn: %s.\n"),
			szPrize[0], nValues[0],
			szPrize[1], nValues[1],
			szPrize[2], nValues[2],
			szPrize[nMaxIdx]);
		SaveYSLog(szMsg);
		QTRACE("ChoosePrize: chon o %d (gia tri %ld)", nMaxIdx + 1, nMaxPrize);
		pPrzSel->OnClickBtn(nMaxIdx);
		m_nDoQuestStep = 0;
		m_nYSDoing = ys_none;
		if (m_bReturnItem)
			theMap.m_bDoShoping = 0;
		theMap.m_nTimeWait = 2;
		return;
	}

	theMap.ClickToNpc(theMap.NpcPoints[npc_YeSou].x, theMap.NpcPoints[npc_YeSou].y);
}

BOOL CVLQuest::YSPreparing()
{
	static int nItem01 = 0;
	static int nItem02 = 0;
	static int nItem03 = 0;
	if (m_nDoQuestStep == 0) // Check FY Items
	{
		if (m_AddPointOpt.m_bFYBuyItem)
		{
			// Count PDL
			KItem* pItem; int nItemIdx;
			nItem01 = m_AddPointOpt.m_nFYItem01;
			nItem02 = m_AddPointOpt.m_nFYItem02;
			nItem03 = m_AddPointOpt.m_nFYItem03;
			for (int i = 0; i < MAX_PLAYER_ITEM; i++)
			{
				if (PLAYER_ITEM(i).nPlace != pos_equiproom &&
					PLAYER_ITEM(i).nPlace != pos_repositoryroom &&
					PLAYER_ITEM(i).nPlace != pos_repo02 &&
					PLAYER_ITEM(i).nPlace != pos_repo03 &&
					PLAYER_ITEM(i).nPlace != pos_guid) continue;
				nItemIdx = PLAYER_ITEM(i).nIdx;
				pItem = (KItem*)(dwItmBaseAdd + nItemIdx * ITM_DATA_SIZE);
				if (!strstr(ComAttr.szItemName, FY_ITEM)) continue;
				switch (ComAttr.nParticularType)
				{
				case 122: nItem01--; break;
				case 123: nItem02--; break;
				case 124: nItem03--; break;
				}
			}
			if (nItem01 > 0 || nItem02 > 0 || nItem03 > 0)
			{
				theMap.m_bDoShoping = 0;
				theMap.NearestDriver();
				m_nDoQuestStep = 1; // Buy FY Items
				SystemMessage(TV(L"<color=yellow>Đi đổi Phúc Duyên dự trữ..."));
			}
			else
				m_nDoQuestStep = 3; // Check Return Items
		}
		else
			m_nDoQuestStep = 3; // Check Return Items
		return TRUE;
	}
	if (m_nDoQuestStep == 1) // Buy FY Items
	{
		if (theMap.ChangeMap(city_tuongduong)) return TRUE;

		if (!theMap.m_bDoShoping && theMap.RunningToPos(theMap.NpcPoints[npc_Reseved1])) return TRUE;
		theMap.m_bDoShoping = TRUE;

		KUiShop* pShop = (KUiShop*)GetPtrAddress(SHOP_BASE_ADD);

		KUiMsgSel* pMsgSel = (KUiMsgSel*)GetPtrAddress(MSG_BASE_ADD);
		if (pMsgSel && pMsgSel->IsVisible())
		{
			if (pShop)
			{
				pMsgSel->OnClickMsg(pMsgSel->m_nNumMessage);
				return TRUE;
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
			return TRUE;
		}
		if (pShop) //Do Shoping
		{
			//Full Room
			if (g_pCurPlayer->CheckRoom() == MAX_ROOM_SPACE)
			{
				m_nDoQuestStep = 2;
				return TRUE;
			}
			//Buy FYL
			int nCurFY = g_pCurPlayer->GetTaskVal(0x0097);
			if (nItem01 > 0)
			{
				if (nCurFY >= 12)
				{
					nItem01--;
					g_pCurPlayer->BuyItem(11);
				}
				else
					m_nDoQuestStep = 2;
				return TRUE;
			}
			if (nItem02 > 0)
			{
				if (nCurFY >= 24)
				{
					nItem02--;
					g_pCurPlayer->BuyItem(12);
				}
				else
					m_nDoQuestStep = 2;
				return TRUE;
			}
			if (nItem03 > 0)
			{
				if (nCurFY >= 60)
				{
					nItem03--;
					g_pCurPlayer->BuyItem(13);
				}
				else
					m_nDoQuestStep = 2;
				return TRUE;
			}
			m_nDoQuestStep = 2;
			return TRUE;
		}

		theMap.ClickToNpc(theMap.NpcPoints[npc_Reseved1].x, theMap.NpcPoints[npc_Reseved1].y);
		return TRUE;
	}
	if (m_nDoQuestStep == 2) // Clear Buying
	{
		theMap.ResetShoping();
		m_nDoQuestStep = 3;
		return TRUE;
	}
	if (m_nDoQuestStep == 3) // Check Return Items
	{
		if (m_bReturnItem && !PutItemToRepo(&m_itmRoom, &m_itmEquip)) return TRUE;
		m_bReturnItem = 0;
		m_nDoQuestStep = 4;
	}
	if (m_nDoQuestStep == 4) // Check PK Values
	{
		BOOL bCanUsePK = (m_nTongPK != act_cancel && m_TongPKOpt.m_bUseWithYS && // user setting
			g_pCurPlayer->m_cTong.m_dwTongNameID && g_pCurPlayer->m_cTong.ContributePoint() >= 200); // current condition
		BOOL bGetMaxPK = ((m_nTotalPKQuest >= 100) ||
			(m_TongPKOpt.m_nMaxQuestCount > 0 && m_nTotalPKQuest >= m_TongPKOpt.m_nMaxQuestCount));
		if (g_pCurPlayer->m_nPKValue && bCanUsePK && !bGetMaxPK)
		{
			SaveYSLog(TV(L"Có PK, tạm thời đi tẩy PK!"));
			SystemMessage(TV(L"<color=yellow>Có PK, tạm thời đi tẩy PK!"));
			m_nGenDoing = qt_dopk;
			m_bIsYeSouAndPK = TRUE;
			m_nDoQuestStep = 0;
			return TRUE;
		}
		else
			m_nDoQuestStep = 5; // Continue
	}

	return 0;
}

void CVLQuest::GetNewQuest()
{
	QTRACE("GetNewQuest: Map=%d Shoping=%d NeedMoney=%d InTownOnly=%d", *g_pMapID, theMap.m_bDoShoping,
		theMap.m_bNeedMoney, m_bInTownOnly);
	if (theMap.m_bNeedMoney)
	{
		theMap.WithdrawMoney();
		return;
	}

	// GetQuest in town
	if (m_bInTownOnly || ((*g_pMapID) >= city_banghoimin && (*g_pMapID) <= city_banghoimax))
	{
		int nSubCityID = theMain.GetSubCity(*g_pMapID);
		if (theMap.ChangeMap(nSubCityID)) return;
	}
	//bản CTC , chỉ có thành Tương Dương có NPC Dã tẩu, nphải chạy về Tương Dương  mapid = 78
	int nCurMapID = *g_pMapID;
	if(nCurMapID != city_tuongduong)
	{
		if (theMap.ChangeMap(city_tuongduong)) return;
	}
	if (!theMap.m_bDoShoping && theMap.RunningToPos(theMap.NpcPoints[npc_YeSou])) return;
	theMap.m_bDoShoping = TRUE;
	QTRACE("GetNewQuest: da toi cho NPC Da Tau (%d,%d)", theMap.NpcPoints[npc_YeSou].x, theMap.NpcPoints[npc_YeSou].y);
	// check đã kích hoạt thẻ vip chưa, nếu chưa thì dừng lại
	if (g_UiInformation2 && g_UiInformation2->IsVisible())
	{		
		/*
		* g_UiInformation2 = 0x007C6CC8  IsVisible = 1
		  Text: Cần kích hoạt thẻ Vip mới có thể nhận và làm nhiệm vụ..
		  Text [TCVN3]: CÇn kÝch ho¹t thÎ Vip míi cã thÓ nhËn vµ lµm nhiÖm vô..
		*/
		char szInfo[sizeof(g_UiInformation2->m_szInfoText) + 1];
		// loại bỏ các mã màu nếu có
		GameTextPlain(g_UiInformation2->m_szInfoText, sizeof(g_UiInformation2->m_szInfoText), szInfo, sizeof(szInfo));	
		int nVipVal = IsNeedVipText(szInfo);
		QTRACE("GetNewQuest: Information2: %s (NeedVip = %d)", szInfo, nVipVal);
		if (nVipVal)
		{
			StopNoVip();
			CloseAllWindows(0);
			return;
		}
		else
			CloseAllWindows(0);
	}

	KUiMsgSel2* pMsgSel2 = (KUiMsgSel2*)GetPtrAddress(MSG2_BASE_ADD);
	if (pMsgSel2 && pMsgSel2->IsVisible())
	{
		QTRACE("GetNewQuest: MsgSel2 %d dong: %s", pMsgSel2->m_nNumMessage, QTJoinMsg(pMsgSel2));
		

		if (pMsgSel2->m_nNumMessage == 6)
		{
			
			if (strstr(pMsgSel2->m_pMessages[0]->Msg, TV(L"để ta hoàn thành nhiệm vụ")))
			{
				GetYSQuestText(pMsgSel2);
				m_nYSDoing = GetRequiredCondition();
				if (m_nYSDoing == ys_return || m_nYSDoing == ys_cancel) return;
				theMap.m_bDoShoping = 0;
				pMsgSel2->OnClickMsg(0);
				theMap.NearestDriver();
			}
		}
		else if (pMsgSel2->m_nNumMessage == 3) // Nhận nhiệm vụ
		{
			/*
			*  Lua chon[0]: Nhận nhiệm vụ
			  Lua chon[0] [TCVN3]: NhËn nhiÖm vô
			  Lua chon[1]: Xem tình hình chuỗi nhiệm vụ
			  Lua chon[1] [TCVN3]: Xem t×nh h×nh chuçi nhiÖm vô
			  Lua chon[2]:  Kết thúc đối thoại!
			  Lua chon[2] [TCVN3]:  KÕt thóc ®èi tho¹i!
			*/
			// 15-10: click NPC hien menu chinh truoc, chon dong "Nhan nhiem vu"
			size_t nKeyLen = strlen(YS_MENU_GETQUEST);
			for (int i = 0; i < pMsgSel2->m_nNumMessage; i++)
			{				
				if (!strncmp(pMsgSel2->m_pMessages[i]->Msg, YS_MENU_GETQUEST, nKeyLen))
				{
					m_nQuestStatus = 1;
					m_nYSDoing = ys_none;
					//Lua chon[0]: Nhận nhiệm vụ
					QTRACE(TV(L"GetNewQuest: chon dong [%d] Nhận nhiệm vụ"), i);
					pMsgSel2->OnClickMsg(i);
					break;
				}
			}
		}
		else if (pMsgSel2->m_nNumMessage == 4)
		{
			/*
			* Lua chon[0]: Hoàn thành nhiệm vụ thứ 0
			  Lua chon[0] [TCVN3]: Hoµn thµnh nhiÖm vô thø 0
			  Lua chon[1]: Hủy nhiệm vụ
			  Lua chon[1] [TCVN3]: Hñy nhiÖm vô
			  Lua chon[2]: Xem tình hình chuỗi nhiệm vụ
			  Lua chon[2] [TCVN3]: Xem t×nh h×nh chuçi nhiÖm vô
			  Lua chon[3]:  Kết thúc đối thoại!
			  Lua chon[3] [TCVN3]:  KÕt thóc ®èi tho¹i!
			*/
			size_t nKeyLen = strlen(TV(L"Xem tình hình chuỗi nhiệm vụ"));
			for (int i = 0; i < pMsgSel2->m_nNumMessage; i++)
			{				
				if (!strncmp(pMsgSel2->m_pMessages[i]->Msg, TV(L"Xem tình hình chuỗi nhiệm vụ"), nKeyLen))
				{
					m_nQuestStatus = 1;
					m_nYSDoing = ys_none;
					//Lua chon[2]: Xem tình hình chuỗi nhiệm vụ
					QTRACE(TV(L"GetNewQuest: chon dong [%d] Xem tình hình chuỗi nhiệm vụ"), i);
					pMsgSel2->OnClickMsg(i);
					break;
					break;
				}
			}
		}
		// xem thông tin nhiệm vụ => phân tích là loại nhiệm vụ gì, để biết là cần làm gì tiếp theo
		else if (pMsgSel2->m_nNumMessage == 1)
		{
			/*
			MSG2_BASE_ADD -> 0x21077B70
			IsVisible = 1  m_Style = 0x90000000
			  Loi thoai: Tổng số nhiệm vụ đã hoàn thành: 2 lần.  Số nhiệm vụ hôm nay đã nhận: 0 / 10 lần. Nhiệm vụ hiện tại: Tìm Trang bị tăng Nội Lực lớn hơn 150 điểm 
			  Loi thoai [TCVN3]: Tæng sè nhiÖm vô ®· hoµn thµnh: 2 lÇn.  Sè nhiÖm vô h«m nay ®· nhËn: 0 / 10 lÇn. NhiÖm vô hiÖn t¹i: T×m Trang bÞ t¨ng Néi Lùc lín h¬n 150 ®iÓm 
			m_nNumMessage = 1
			  Lua chon[0]: Ta biết rồi.
			  Lua chon[0] [TCVN3]: Ta biÕt råi.
			*/

			size_t nKeyLen = strlen(TV(L"Ta biết rồi"));
			for (int i = 0; i < pMsgSel2->m_nNumMessage; i++)
			{
				if (!strncmp(pMsgSel2->m_pMessages[i]->Msg, TV(L"Ta biết rồi"), nKeyLen))
				{
					GetYesouQuestFromKUiMsgSel2(pMsgSel2);
					m_nQuestStatus = 1;
					m_nYSDoing = ys_doing;
					QTRACE(TV(L"GetNewQuest: chon dong [%d] Ta biết rồi"), i);
					pMsgSel2->OnClickMsg(i);
					// Xong voi Da Tau: tra co Shoping, neu khong ChangeMap bo qua buoc chay toi xa phu
					// (ClickToNpc tu xa khong thay NPC => dung im)
					theMap.m_bDoShoping = 0;
					theMap.NearestDriver();
					// Moc dung vong tim cac thanh (GetNextSearchCity). Duong cu gan trong GetRequiredConditionRaw,
					// duong 15-10 khong goi ham do => di vong mai khong huy nhiem vu
					m_nFirstMapID = theMain.GetOwnerCity(*g_pMapID);
					break;
				}
			}
		}
		return;
	}

	KUiMsgSel* pMsgSel = (KUiMsgSel*)GetPtrAddress(MSG_BASE_ADD);
	if (pMsgSel && pMsgSel->IsVisible())
	{
		QTRACE("GetNewQuest: MsgSel %d dong: %s", pMsgSel->m_nNumMessage, QTJoinMsg(pMsgSel));
		if (HasNeedVipMsg(pMsgSel))
		{
			StopNoVip();
			return;
		}
		theMap.m_nTimeWait = 3;
		if (pMsgSel->m_nNumMessage == 1)
		{
			if (strstr(pMsgSel->m_InfoText, TV(L"Mỗi ngày làm 40 lần là đủ rồi!")))
			{
				m_nRemainQuest = 0;
				m_nYSDoing = ys_suspending;
			}
			pMsgSel->OnClickMsg(0);
			theMap.m_bDoShoping = 0;
		}
		
		else if (pMsgSel->m_nNumMessage == 2)
		{
			if (strstr(pMsgSel->m_pMessages[0]->Msg, TV(L"mau cho ta biết nhiệm vụ")))
			{
				GetRemainQuest(pMsgSel);
				pMsgSel->OnClickMsg(0);
			}
		}
		else if (pMsgSel->m_nNumMessage == 3)
		{
			/*
			* 
			*/
			if (strstr(pMsgSel->m_pMessages[0]->Msg, TV(L"Ta không tin có nhiệm vụ")))
			{
				m_nRemainQuest = 40;
				pMsgSel->OnClickMsg(0);
			}
		}
		else if (pMsgSel->m_nNumMessage == 4)
		{
			/**
			/**
			* MsgSel2 4 dong: [0]Hoµn thµnh nhiÖm vô thø 1 | [1]Hñy nhiÖm vô | [2]Xem t×nh h×nh chuçi nhiÖm vô | [3] KÕt thóc ®èi tho¹i!
			* 
			KUiMsgSel(MSG_BASE_ADD) = 0x1C25D818  IsVisible = 0
				KUiMsgSel2(MSG2_BASE_ADD) = 0x1B64BFA8  IsVisible = 1
				Loi thoai :  ầ
				[0] Hoàn thành nhiệm vụ thứ 1
				[1] Hủy nhiệm vụ
				[2] Xem tình hình chuỗi nhiệm vụ
				[3]  Kết thúc đối thoại!
			/**/
			/**/
			if (strstr(pMsgSel->m_pMessages[0]->Msg, TV(L"để ta hoàn thành nhiệm vụ")))
			{
				GetYSQuestText(pMsgSel);
				m_nYSDoing = GetRequiredCondition();
				if (m_nYSDoing == ys_return || m_nYSDoing == ys_cancel) return;
				theMap.m_bDoShoping = 0;
				pMsgSel->OnClickMsg(0);
				theMap.NearestDriver();
			}
		}

		return;
	}

	QTRACE("GetNewQuest: khong co hop thoai -> ClickToNpc");
	theMap.ClickToNpc(theMap.NpcPoints[npc_YeSou].x, theMap.NpcPoints[npc_YeSou].y);
}

void CVLQuest::GetRemainQuest(KUiMsgSel* pMsgSel)
{
	char* szInfo = (char*)pMsgSel->m_InfoText + 0x16;

	char* szRemain = strstr(szInfo, TV(L"có thể thực hiện lại"));
	if (szRemain)
	{
		szRemain += 20;

		while (szRemain[0] == ' ')
			szRemain++;

		m_nRemainQuest = atoi(szRemain);
	}
}

void CVLQuest::GetYSQuestText(KUiMsgSel* pMsgSel)
{
	char szLogLine[256] = "";
	int nYeSouCode = (g_pCurPlayer->GetTaskVal(ys_tasknum) * 1234 + g_pCurPlayer->GetTaskVal(ys_minorid)) ^ 19771984;
	sprintf(szLogLine, TV(L"Dã Tẩu (%08X): "), nYeSouCode);
	memset(m_szYSQuest, 0, sizeof(m_szYSQuest));
	int i = 0, j = 13, k = (int)strlen(szLogLine);
	strcpy(m_szYSQuest, "<color=green>");

	int nInfoLen = GameTextLen(pMsgSel->m_InfoText, sizeof(pMsgSel->m_InfoText));	// 15-10: byte mau co the = 0
	while (i < nInfoLen && j < (int)sizeof(m_szYSQuest) - 16 && k < (int)sizeof(szLogLine) - 2)
	{
		if (pMsgSel->m_InfoText[i] == 0x02)
		{
			strcat(m_szYSQuest, "<color=yellow>");
			i += 4;
			j += 14;
		}
		else if (pMsgSel->m_InfoText[i] == 0x03)
		{
			strcat(m_szYSQuest, "<color=green>");
			i += 1;
			j += 13;
		}
		else if (pMsgSel->m_InfoText[i])
		{
			m_szYSQuest[j] = pMsgSel->m_InfoText[i];
			szLogLine[k] = pMsgSel->m_InfoText[i];
			i++; j++; k++;
		}
	}

	SystemMessage(m_szYSQuest, 1);
	SaveYSLog(szLogLine);
}

void CVLQuest::GetYSQuestText(KUiMsgSel2* pMsgSel)
{
	char szLogLine[256] = "";
	int nYeSouCode = (g_pCurPlayer->GetTaskVal(ys_tasknum) * 1234 + g_pCurPlayer->GetTaskVal(ys_minorid)) ^ 19771984;
	sprintf(szLogLine, TV(L"Dã Tẩu (%08X): "), nYeSouCode);
	memset(m_szYSQuest, 0, sizeof(m_szYSQuest));
	int i = 0, j = 13, k = (int)strlen(szLogLine);
	strcpy(m_szYSQuest, "<color=green>");

	const char* szInfo = pMsgSel->GetQuestion();	// 15-10: loi thoai NPC o this+0x2578
	if (!szInfo[0]) return;

	// Loi thoai co the dai toi 8KB: chan de khong tran m_szYSQuest / szLogLine
	while (szInfo[i] && j < (int)sizeof(m_szYSQuest) - 16 && k < (int)sizeof(szLogLine) - 2)
	{
		if (szInfo[i] == 0x02)
		{
			strcat(m_szYSQuest, "<color=yellow>");
			i += 4;
			j += 14;
		}
		else if (szInfo[i] == 0x03)
		{
			strcat(m_szYSQuest, "<color=green>");
			i += 1;
			j += 13;
		}
		else if (szInfo[i])
		{
			m_szYSQuest[j] = szInfo[i];
			szLogLine[k] = szInfo[i];
			i++; j++; k++;
		}
	}

	SystemMessage(m_szYSQuest, 1);
	SaveYSLog(szLogLine);
}

void CVLQuest::DoQuest01()
{
	QTRACE("DoQuest01 (mua o NPC): Step=%d Map=%d/%d Npc=%d Shoping=%d", m_nDoQuestStep,
		*g_pMapID, m_nRqMapID, m_nRqNpcID, theMap.m_bDoShoping);
	if (m_nDoQuestStep == 0)
	{
		PlayerItem theItem;
		if (CheckMyItem(&theItem))
		{
			m_nYSDoing = ys_return;
			theMap.NearestDriver();
		}
		m_nDoQuestStep = 1;
		return;
	}

	if (theMap.ChangeMap(m_nRqMapID)) return;
	POINT ptNpcPoint = theMap.NpcPoints[m_nRqNpcID];
	if (!theMap.m_bDoShoping && theMap.RunningToPos(ptNpcPoint)) return;
	theMap.m_bDoShoping = TRUE;

	KUiShop* pShop = (KUiShop*)GetPtrAddress(SHOP_BASE_ADD);

	KUiMsgSel* pMsgSel = (KUiMsgSel*)GetPtrAddress(MSG_BASE_ADD);
	if (pMsgSel && pMsgSel->IsVisible())
	{
		if (pShop)
		{
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

	KUiMsgSel2* pMsgSel2 = (KUiMsgSel2*)GetPtrAddress(MSG2_BASE_ADD);
	if (pMsgSel2 && pMsgSel2->IsVisible())
	{
		if (pShop)
		{
			pMsgSel2->OnClickMsg(pMsgSel2->m_nNumMessage);
			return;
		}
		for (int i = 0; i < pMsgSel2->m_nNumMessage; i++)
		{
			char* szMsg = (char*)pMsgSel2->m_pMessages[i]->Msg;
			if (strstr(szMsg, AGREE_TRADE))
			{
				pMsgSel2->OnClickMsg(i);
				break;
			}
		}
		return;
	}

	if (pShop)
	{
		int nItemIdx = pShop->m_nObjCount;
		KItemObject* pItemList = pShop->m_pObjectList;
		while (--nItemIdx >= 0)
		{
			int idx = g_pBuySell->GetItemIndex(g_pCurPlayer->m_nCurShop, pItemList[nItemIdx].uId);
			KItem* pItem = g_pBuySell->GetItem(idx);
			if (ComAttr.nItemGenre != m_nRqGenre) continue;
			if (ComAttr.nDetailType != m_nRqDetail) continue;
			if (ComAttr.nParticularType != m_nRqParticular) continue;
			if (ComAttr.nLevel != m_nRqLevel) continue;
			if (ComAttr.nSeries != m_nRqSeries) continue;
			break;
		}
		QTRACE("DoQuest01: shop co %d mon, vat pham can o idx=%d", pShop->m_nObjCount, nItemIdx);
		POINT itmPos;
		if (EQUIP_ROOM.FindRoom(pShop->m_pObjectList[nItemIdx].DataW, pShop->m_pObjectList[nItemIdx].DataH, &itmPos))
		{
			g_pCurPlayer->BuyItem(pShop->m_pObjectList[nItemIdx].uId);

			SystemMessage(TV(L"Mua được vật phẩm/trang bị từ NPC"));
			SaveYSLog(TV(L"Mua được vật phẩm/trang bị từ NPC"));

			m_nYSDoing = ys_return;
			theMap.NearestDriver();
		}
		else
		{
			SystemMessage(TV(L"<color=yellow>Hành trang không đủ chỗ trống!"));
			SaveYSLog(TV(L"Hành trang không đủ chỗ trống => Tạm ngưng!"));
			m_nYSDoing = ys_suspending;
		}
		theMap.ResetShoping();
		return;
	}
	theMap.ClickToNpc(ptNpcPoint.x, ptNpcPoint.y);
}

void CVLQuest::DoQuest0203()
{
	static int nNextCityID = 0;
	static BOOL bRunToYeSou = 0;
	static PlayerItem theItem = { -1, -1, -1, -1, -1 };

	QTRACE("DoQuest0%d (tim vat pham): Step=%d Map=%d NextCity=%d RunToYS=%d", m_nQuestID,
		m_nDoQuestStep, *g_pMapID, nNextCityID, bRunToYeSou);

	if (m_nDoQuestStep == 0)
	{
		nNextCityID = 0;
		bRunToYeSou = 0;
		m_bReturnItem = 0;
		memset(&theItem, -1, sizeof(PlayerItem));
		memset(m_bVsNpc, 0, sizeof(m_bVsNpc));
		m_nDoQuestStep = 1;
	}

	if (m_nDoQuestStep == 1)
	{
		if (theItem.nIdx > 0 && theItem.nPlace >= pos_repositoryroom)
		{
			if (!GetItemFromRepo(&theItem)) return;
			if (m_nQuestID == 3) m_bReturnItem = TRUE;
			SaveYSLog(TV(L"Lấy vật phẩm/trang bị trong rương đồ."));
			m_nYSDoing = ys_return;
			theMap.NearestDriver();
			return;
		}
		BOOL bFound = CheckMyItem(&theItem);
		if (!bFound && (m_nQuestID == 3 || !m_BuyItemOpt.m_bNoReturnNoTake))
		{
			BOOL bAllRep = (m_nQuestID == 3 && m_BuyItemOpt.m_bGetAllRep);
			if (!bFound && (m_bUseR1 || bAllRep))
				bFound = CheckMyItem(&theItem, pos_repositoryroom);
			if (!bFound && (m_bUseR2 || bAllRep))
				bFound = CheckMyItem(&theItem, pos_repo02);
			if (!bFound && (m_bUseR3 || bAllRep))
				bFound = CheckMyItem(&theItem, pos_repo03);
		}
		if (bFound)
		{
			if (theItem.nPlace >= pos_repositoryroom)
			{
				m_itmRoom = theItem;
				return;
			}
			KItem* pItem = (KItem*)(dwItmBaseAdd + theItem.nIdx * ITM_DATA_SIZE);
			if (ComAttr.bStack && pItem->m_nCurDur > 1)
				g_pCoreShell->OperationRequest(GOI_SHIFT_ITEM, theItem.nIdx, 1);
			m_nYSDoing = ys_return;
			theMap.NearestDriver();
		}
		else
			m_nDoQuestStep = 2; // Check before BUY

		return;
	}

	if (m_nDoQuestStep == 2) // Check before BUY
	{
		if (m_BuyItemOpt.m_bDontSearchBuy)
		{
			m_nYSDoing = ys_cancel;
			theMap.NearestDriver();
			SaveYSLog(TV(L"Người chơi không thiết lập mua đồ => Hủy nhiệm vụ!"));
			return;
		}
		if (m_nQuestID == 2)
		{
			POINT itmPos;
			int nW = 0, nH = 0;
			if (m_nRqDetail <= 3) { nW = 1; nH = 1; }	// Nhan, STG, HT
			if (m_nRqDetail == 4) { nW = 2; nH = 1; }	// DayChuyen
			if (m_nRqDetail == 9) { nW = 1; nH = 2; }	// NgocBoi
			if (nW && nH)
			{
				if (!EQUIP_ROOM.FindRoom(nW, nH, &itmPos))
				{
					m_nYSDoing = ys_suspending;
					SystemMessage(TV(L"<color=yellow>Hành trang không đủ chỗ trống!"));
					SaveYSLog(TV(L"Hành trang không đủ chỗ trống => Tạm ngưng!"));
					return;
				}
			}
		}
		m_nDoQuestStep = 3;
	}

	//if (m_nDoQuestStep == 3)
	if (nNextCityID)
	{
		if (theMap.ChangeMap(nNextCityID)) return;
		m_nDoQuestStep = 1;
		nNextCityID = 0;
		return;
	}

	if (bRunToYeSou && theMap.RunningToPos(theMap.NpcPoints[npc_YeSou])) return;
	bRunToYeSou = 0;

	int nBuyResult = SearchBuyItem();
	QTRACE("DoQuest0203: SearchBuyItem => %d (0=khong thay, 1=dang tim, 2=da mua)", nBuyResult);
	if (nBuyResult == 1) return; // Continue

	m_nDoQuestStep = 1;
	bRunToYeSou = TRUE;
	theMap.NearestDriver();
	memset(m_bVsNpc, 0, sizeof(m_bVsNpc));

	if (nBuyResult == 2)
	{
		nNextCityID = GetNextSearchCity(TRUE);
		return;
	}

	if (nBuyResult == 0)
	{
		nNextCityID = GetNextSearchCity();
		if (!nNextCityID)
		{
			if (m_BuyItemOpt.m_bNotFoundCancel)
			{
				m_nYSDoing = ys_cancel;
				theMap.NearestDriver();
				SaveYSLog(TV(L"Tìm hết các thành không thấy => Hủy nhiệm vụ!"));
			}
			else
			{
				m_nYSDoing = ys_suspending;
				SaveYSLog(TV(L"Tìm hết các thành không thấy => Tạm ngưng!"));
			}
		}
		else
			bRunToYeSou = TRUE;
	}
}

void CVLQuest::DoQuest04()
{
	int nTotalMap = g_pCurPlayer->GetTaskVal(ys_maphave);
	QTRACE("DoQuest04 (do chi): co %d/%d Step=%d Map=%d/%d", nTotalMap, m_nRqValue, m_nDoQuestStep,
		*g_pMapID, m_nRqMapID);
	if (nTotalMap >= m_nRqValue && !theMap.m_bDoShoping)
	{
		m_bNeedGoTrain = 0;
		m_nYeSouMapID = 0;
		m_nYSDoing = ys_return;
		theMap.m_bFirstSaveRepos = TRUE;
		theMap.m_nCurPoint = -1;
		theMap.NearestDriver();
		return;
	}
	if (m_nDoQuestStep == 0)
	{
		m_bNeedGoTrain = TRUE;
		theMap.m_bFirstSaveRepos = TRUE;
		theMap.m_bNoDriverRecent = TRUE;
		m_nDoQuestStep = 1;
	}
	if (m_nDoQuestStep < 2 && !theMove.m_bFollow && !theAttack.m_bAutoHit)
	{
		theMain.PostFeedBack(cmd_autohit);
		m_nDoQuestStep = 2;
		return;
	}
	if (m_nDoQuestStep < 3 && !theMap.m_bAutoReturn)
	{
		theMain.PostFeedBack(cmd_setmap);
		m_nDoQuestStep = 3;
		return;
	}
}

void CVLQuest::DoQuest05()
{
	int nPreLevel;
	int nOldValue, nCurValue;

	QTRACE("DoQuest05 (%s): Step=%d RqValue=%d", QTName_Rq(m_nRqType), m_nDoQuestStep, m_nRqValue);

	switch (m_nRqType)
	{
	case ys_rqjingyan:
		nOldValue = g_pCurPlayer->GetTaskVal(ys_preexp);
		nPreLevel = g_pCurPlayer->GetTaskVal(ys_prelevel);
		nCurValue = theMain.GetExpUpLevel(nPreLevel, P_PLAYER->m_Level) + (int)g_pCurPlayer->m_nCurExp;
		QTRACE("DoQuest05 JY: dat %d/10", m_nRqValue ? (int)((__int64)(nCurValue - nOldValue) * 10 / m_nRqValue) : 0);
		if (nCurValue >= nOldValue + m_nRqValue)
		{
			char szMsg[64];
			sprintf(szMsg, "%d == %d == %d", nCurValue, nOldValue, m_nRqValue);
			SystemMessage(szMsg);

			m_bNeedGoTrain = 0;
			m_nYSDoing = ys_return;
			theMap.m_bFirstSaveRepos = TRUE;
			theMap.NearestDriver();
		}
		else
			DoQuestJY(m_nRqValue);
		break;
	case ys_rqmingwang:
		nOldValue = g_pCurPlayer->GetTaskVal(ys_repvalue);
		nCurValue = g_pCurPlayer->GetTaskVal(0x0064);
		QTRACE("DoQuest05 MW: dat %d/%d", nCurValue - nOldValue, m_nRqValue);
		if (nCurValue >= nOldValue + m_nRqValue && !theMap.m_bDoShoping)
		{
			m_bNeedGoPCK = 0;
			m_nYSDoing = ys_return;
			theMap.m_nCurPoint = -1;
			theMap.NearestDriver();
		}
		else
			DoQuestMW(m_nRqValue);
		break;
	case ys_rqfuyuan:
		nOldValue = g_pCurPlayer->GetTaskVal(ys_repvalue);
		nCurValue = g_pCurPlayer->GetTaskVal(0x0097);
		QTRACE("DoQuest05 FY: dat %d/%d", nCurValue - nOldValue, m_nRqValue);
		if (nCurValue >= nOldValue + m_nRqValue && !theMap.m_bDoShoping)
		{
			m_nYSDoing = ys_return;
			theMap.m_nCurPoint = -1;
			theMap.NearestDriver();
		}
		else
			DoQuestFY(nOldValue + m_nRqValue - nCurValue);
		break;
	case ys_rqpk:
		nOldValue = g_pCurPlayer->GetTaskVal(ys_repvalue);
		nCurValue = g_pCurPlayer->m_nPKValue;
		QTRACE("DoQuest05 PK: dat %d/%d", nCurValue - nOldValue, m_nRqValue);
		if (nCurValue >= nOldValue + m_nRqValue)
		{
			m_bNeedGoTrain = 0;
			m_bGoTrainGetPK = 0;
			m_nYSDoing = ys_return;
			theMap.m_bFirstSaveRepos = TRUE;
			theMap.NearestDriver();
			if (m_TongPKOpt.m_bExitVictim)
				theMain.PostFeedBack(cmd_endpk);
		}
		else
			DoQuestPK(m_nRqValue);
		break;
	case ys_rqsongjin:
		m_nYSDoing = ys_suspending;
		break;
	}
}

void CVLQuest::DoQuest06()
{
	int nMapSHXT = g_pCurPlayer->GetTaskVal(ys_shxtmap);
	QTRACE("DoQuest06 (SHXT): co %d/%d manh", nMapSHXT, m_nRqValue);
	if (nMapSHXT >= m_nRqValue)
		m_nYSDoing = ys_return;
	else
		m_nYSDoing = ys_suspending;
}

void CVLQuest::DoQuestJY(int nRqValue)
{
	static BOOL bUseItemFound = 0;
	static PlayerItem theItem;

	if (m_AddPointOpt.m_ExpItemName[m_nDoQuestStep][0])
	{
		KUiMsgSel* pMsgSel = (KUiMsgSel*)GetPtrAddress(MSG_BASE_ADD);
		if (pMsgSel && pMsgSel->IsVisible())
		{
			if (pMsgSel->m_nNumMessage == 1)
			{
				pMsgSel->OnClickMsg(0);
				m_nDoQuestStep++;
			}
			else
				pMsgSel->OnClickMsg(pMsgSel->m_nNumMessage);
			return;
		}
		char* szExpItemName = m_AddPointOpt.m_ExpItemName[m_nDoQuestStep];
		if (bUseItemFound)
		{
			if (theItem.nPlace == pos_equiproom)
			{
				g_pCurPlayer->UseItem(szExpItemName);
				memset(&theItem, -1, sizeof(PlayerItem));
				bUseItemFound = 0;
				return;
			}
			if (!GetItemFromRepo(&theItem)) return;
			bUseItemFound = 0;
			return;
		}
		bUseItemFound = GetUseableItem(&theItem, szExpItemName, pos_equiproom);
		if (!bUseItemFound)
			bUseItemFound = GetUseableItem(&theItem, szExpItemName);
		if (!bUseItemFound)
			m_nDoQuestStep++;
		return;
	}
	if (m_nDoQuestStep < 9)
	{
		BOOL bCanUsePK = (m_nTongPK != act_cancel && m_TongPKOpt.m_bUseWithYS && // user setting
			g_pCurPlayer->m_cTong.m_dwTongNameID && g_pCurPlayer->m_cTong.ContributePoint() >= 200); // current condition
		BOOL bGetMaxPK = ((m_nTotalPKQuest >= 100) ||
			(m_TongPKOpt.m_nMaxQuestCount > 0 && m_nTotalPKQuest >= m_TongPKOpt.m_nMaxQuestCount));
		if (bCanUsePK && !bGetMaxPK)
		{
			SaveYSLog(TV(L"Làm nvụ LB đặc biệt tăng KN!"));
			SystemMessage(TV(L"<color=yellow>Làm nvụ LB đặc biệt tăng KN!"));
			m_nGenDoing = qt_dopk;
			m_bIsYeSouAndPK = TRUE;
			m_nDoQuestStep = 0;
			return;
		}
		else
			m_nDoQuestStep = 9; // Continue
	}
	if (TRUE)
	{
		if (m_nDoQuestStep < 10 && !theMove.m_bFollow && !theAttack.m_bAutoHit)
		{
			theMain.PostFeedBack(cmd_autohit);
			m_nDoQuestStep = 10;
			return;
		}
		if (m_nDoQuestStep < 11 && !theMap.m_bAutoReturn)
		{
			theMain.PostFeedBack(cmd_setmap);
			m_nDoQuestStep = 11;
			return;
		}
		m_bUseDoubleExp = m_AddPointOpt.m_bUseTienThaoLo;
		m_bNeedGoTrain = TRUE;
		return;
	}

	m_nYSDoing = ys_suspending;
}

void CVLQuest::DoQuestMW(int nRqValue)
{
	KUiMsgSel2* pMsgSel2 = (KUiMsgSel2*)GetPtrAddress(MSG2_BASE_ADD);

	if (m_nDoQuestStep == 0)
	{
		// GetMW in BigCity
		int nOwnerID = theMain.GetOwnerCity(*g_pMapID);
		if (theMap.ChangeMap(nOwnerID)) return;
		if (m_bGetMingWang && nRqValue <= 10)
		{
			if (!theMap.m_bDoShoping && theMap.RunningToPos(theMap.NpcPoints[npc_Reputation])) return;
			theMap.m_bDoShoping = TRUE;
			if (theMap.GetReputation(1)) return;
			m_bGetMingWang = 0;
		}
		theMap.NearestDriver();
		m_nDoQuestStep = 1;
		return;
	}
	if (m_nDoQuestStep == 1)
	{
		if (theMap.m_bNeedPharmacy)
		{
			if (!theMap.m_bDoShoping && theMap.RunningToPos(theMap.NpcPoints[npc_Pharmacy])) return;
			theMap.m_bDoShoping = TRUE;
			theMap.GetPharmacy();
			return;
		}
		theMap.NearestDriver();
		m_nDoQuestStep = 2;
	}
	if (m_nDoQuestStep == 2) // DichQuan - Nhan NV
	{
		if (m_AddPointOpt.m_bPCKOnly)
		{
			if ((*g_pMapID) != city_bienkinh && (*g_pMapID) != city_duongchau)
				if (theMap.ChangeMap(city_duongchau)) return;
		}

		if (!theMap.m_bDoShoping && theMap.RunningToPos(theMap.NpcPoints[npc_Postman])) return;
		theMap.m_bDoShoping = TRUE;

		if (g_UiInformation2->IsVisible())
		{
			g_UiInformation2->Hide();
			m_nDoQuestStep = 4; // Di Xa phu
			theMap.ResetShoping();
			return;
		}
		if (pMsgSel2 && pMsgSel2->IsVisible())
		{
			if (pMsgSel2->m_nNumMessage == 1)
			{
				// Check Time
				int nTime = g_pCurPlayer->GetTaskVal(pck_time);
				if (!nTime)
				{
					if (m_AddPointOpt.m_bMWCancel && nRqValue < m_AddPointOpt.m_nMWCancelVal)
					{
						m_nYSDoing = ys_cancel;
						theMap.NearestDriver();
						SaveYSLog(TV(L"Quá giờ chạy Tín sứ => Hủy nhiệm vụ!"));
					}
					else
					{
						m_nYSDoing = ys_suspending;
						SaveYSLog(TV(L"Quá giờ chạy Tín sứ => Tạm ngưng!"));
					}
					pMsgSel2->OnClickMsg(0);
					m_bNeedGoPCK = 0;
					theMap.ResetShoping();
					return;
				}
				m_nDoQuestStep = 3; // Huy NV
				theMap.ResetShoping();
			}
			pMsgSel2->OnClickMsg(0);
			theMap.m_nTimeWait = 2;
			return;
		}
		theMap.ClickToNpc(theMap.NpcPoints[npc_Postman].x, theMap.NpcPoints[npc_Postman].y);
		return;
	}
	if (m_nDoQuestStep == 3) // DichQuan - Huy NV
	{
		if (pMsgSel2 && pMsgSel2->IsVisible())
		{
			if (strstr(pMsgSel2->m_pMessages[0]->Msg, TV(L"Đúng vậy!")))
				m_nDoQuestStep = 2;
			if (pMsgSel2->m_nNumMessage == 7)
				pMsgSel2->OnClickMsg(2);
			else
				pMsgSel2->OnClickMsg(0);
			theMap.ResetShoping(0);
			return;
		}
		theMap.ClickToNpc(theMap.NpcPoints[npc_Postman].x, theMap.NpcPoints[npc_Postman].y);
		return;
	}
	if (m_nDoQuestStep == 4) // XaPhu - Di lam NV
	{
		int nCount = theMain.GetMenuCount();
		if (nCount > 0)
		{
			if (nCount == 2 && theMain.ClickMenuStr(TV(L"Muốn")))
				m_nDoQuestStep = 5;
			else
				theMain.ClickMenuStr(TV(L"Nhiệm vụ Tín Sứ"));
			theMap.ResetShoping();
			return;
		}
		theMap.ClickToNpc(theMap.NpcPoints[theMap.m_nDriverIndex].x, theMap.NpcPoints[theMap.m_nDriverIndex].y);
	}
	if (m_nDoQuestStep == 5) // Dich Quan - Tra NV
	{
		if (pMsgSel2 && pMsgSel2->IsVisible())
		{
			if (pMsgSel2->m_nNumMessage == 1)
			{
				int nItemBlood = g_pCurPlayer->GetItemsCount(item_medicine, medicine_blood);
				int nItemBoth = g_pCurPlayer->GetItemsCount(item_medicine, medicine_both);
				if (nItemBlood < 5 && nItemBoth < 5)
					m_nDoQuestStep = 1;
				else
					m_nDoQuestStep = 2;
			}
			if (pMsgSel2->m_nNumMessage == 7)
				pMsgSel2->OnClickMsg(1);
			else
				pMsgSel2->OnClickMsg(0);
			theMap.ResetShoping();
			return;
		}
		theMap.ClickToNpc(theMap.NpcPoints[npc_Postman].x, theMap.NpcPoints[npc_Postman].y);
		return;
	}
}

void CVLQuest::DoQuestPK(int nRqValue)
{
	if (m_nYSDoing == ys_doing && !m_TongPKOpt.m_bUseWithYS)
	{
		m_nYSDoing = ys_suspending;
		return;
	}
	if (m_nDoQuestStep < 8 && m_TongPKOpt.m_bAutoLogin)
	{
		theMain.PostFeedBack(cmd_beginpk);
		m_nDoQuestStep = 8;
		return;
	}
	if (m_nDoQuestStep < 9 && !theMove.m_bFollow && !theAttack.m_bAutoHit)
	{
		theMain.PostFeedBack(cmd_autohit);
		m_nDoQuestStep = 9;
		return;
	}
	if (m_nDoQuestStep < 10 && !theMap.m_bAutoReturn)
	{
		theMain.PostFeedBack(cmd_setmap);
		m_nDoQuestStep = 10;
		return;
	}
	m_bNeedGoTrain = TRUE;
	m_bGoTrainGetPK = TRUE;
}

void CVLQuest::DoQuestFY(int nRqValue)
{
	static int nBuyResult = 0;
	static BOOL bUseItemFound = 0;
	static PlayerItem theItem;
	static char FYItem[3][32] = { "", "", "" };

	if (m_nDoQuestStep == 0)
	{
#ifndef VLTK_FREE_MODE
		if (nRqValue <= 20 && ONLINE_TIME_SEC < 18000)
		{
			// GetFY in BigCity
			int nOwnerID = theMain.GetOwnerCity(*g_pMapID);
			if (theMap.ChangeMap(nOwnerID)) return;
			if (!theMap.m_bDoShoping && theMap.RunningToPos(theMap.NpcPoints[npc_Reputation])) return;
			theMap.m_bDoShoping = TRUE;
			if (theMap.GetReputation()) return;
		}
#endif
		theMap.NearestDriver();
		m_nDoQuestStep = 1;
		return;
	}
	if (m_nDoQuestStep == 1)
	{
		if (nRqValue > 30)
		{
			strcpy(FYItem[0], TV(L"Phúc Duyên Lộ (Đại)"));
			strcpy(FYItem[1], TV(L"Phúc Duyên Lộ (Trung)"));
			strcpy(FYItem[2], TV(L"Phúc Duyên Lộ (Tiểu)"));
		}
		else if (nRqValue > 10)
		{
			strcpy(FYItem[0], TV(L"Phúc Duyên Lộ (Trung)"));
			strcpy(FYItem[1], TV(L"Phúc Duyên Lộ (Tiểu)"));
			strcpy(FYItem[2], TV(L"Phúc Duyên Lộ (Đại)"));
		}
		else
		{
			strcpy(FYItem[0], TV(L"Phúc Duyên Lộ (Tiểu)"));
			strcpy(FYItem[1], TV(L"Phúc Duyên Lộ (Trung)"));
			strcpy(FYItem[2], TV(L"Phúc Duyên Lộ (Đại)"));
		}
		m_nDoQuestStep = 2;
	}
	if (m_nDoQuestStep >= 2 && m_nDoQuestStep <= 4)
	{
		char* szFYItemName = FYItem[m_nDoQuestStep - 2];
		if (bUseItemFound)
		{
			if (theItem.nPlace == pos_equiproom)
			{
				g_pCurPlayer->UseItem(szFYItemName);
				nBuyResult = 0;
				bUseItemFound = 0;
				return;
			}
			if (!GetItemFromRepo(&theItem)) return;
			bUseItemFound = 0;
			return;
		}
		bUseItemFound = GetUseableItem(&theItem, szFYItemName, pos_equiproom);
		if (!bUseItemFound)
			bUseItemFound = GetUseableItem(&theItem, szFYItemName);
		if (!bUseItemFound)
			m_nDoQuestStep++;
		return;
	}
	if (m_nDoQuestStep > 4)
	{
		if (nBuyResult == 3)
		{
			if (theMap.ChangeMap(city_daily)) return;
			m_nDoQuestStep = 1; // Search Equip Again
			nBuyResult = 0;
			return;
		}
		if (theMap.ChangeMap(city_tuongduong)) return;
		if (!theMap.m_bDoShoping && theMap.RunningToPos(theMap.NpcPoints[npc_Reseved1])) return;
		theMap.m_bDoShoping = TRUE;
		nBuyResult = SearchBuyItem(FY_ITEM, 0);
		if (nBuyResult == 2)
		{
			m_nDoQuestStep = 1; // Search Equip Again
			memset(m_bVsNpc, 0, sizeof(m_bVsNpc));
			nBuyResult = 3;
			theMap.m_bDoShoping = 0;
			theMap.NearestDriver();
			return;
		}
		else if (nBuyResult == 0)
		{
			theMap.m_bDoShoping = 0;
			memset(m_bVsNpc, 0, sizeof(m_bVsNpc));
			if (nRqValue > 30)
			{
				m_nYSDoing = ys_suspending;
				SaveYSLog(TV(L"Không mua được Phúc Duyên Lộ => Tạm ngưng!"));
			}
			else
			{
				m_nYSDoing = ys_cancel;
				theMap.NearestDriver();
				SaveYSLog(TV(L"Không mua được Phúc Duyên Lộ => Hủy nhiệm vụ!"));
			}
		}
	}
}

YESOU_TASK_DOING CVLQuest::ReportQuest(BOOL bAutoCheck, char* szQuest)
{
	time_t tmNow;
	YESOU_TASK_DOING ys_Result = ys_none;

	char szMsg[128];
	if (!m_bAutoDoQuest)
	{
		if (!bAutoCheck)
		{
			ExecuteScript("ClearMessage()");
			if (szQuest) sprintf(szQuest, TV(L"VLAuto không tự làm Nhiệm vụ!"));
			SystemMessage(TV(L"<color=yellow>VLAuto không tự làm Nhiệm vụ!"), 1);
		}
		return ys_Result;
	}

	// CTC 15-10: GetTaskVal(ys_...) luôn trả 0 => không đọc lại m_nQuestID / m_nQuestStatus / m_nQuestGID ở đây
	// (CheckStatus gọi mỗi 5s, trước đây ghi đè Status=1 về 0 giữa chừng => BreathYS bỏ DoCurrentQuest, đứng im)
	//m_nQuestID = g_pCurPlayer->GetTaskVal(ys_majorid);
	//m_nQuestStatus = g_pCurPlayer->GetTaskVal(ys_taskstatus);
	//int nQuestGID = g_pCurPlayer->GetTaskVal(ys_taskgid);
	//if ((m_nYSDoing == ys_doing) && (nQuestGID != m_nQuestGID))
	//	m_nYSDoing = ys_none;
	//m_nQuestGID = nQuestGID;

	if (bAutoCheck)
	{
		if (!P_PLAYER->m_FightMode && !g_pCurPlayer->m_nEntrustMode && m_nYSDoing != ys_doing)
			return ys_Result;
		if (m_nYSDoing == ys_none && m_nQuestStatus == 1)
			m_nYSDoing = GetRequiredCondition();
	}

	time(&tmNow);
	ys_Result = ys_doing;
	int nTimeCount = (int)(tmNow - m_tmStart);
	if (m_nYSDoing != ys_doing)
	{
		if (szQuest) sprintf(szQuest, TV(L"Chưa nhận Nhiệm vụ mới!"));
		if (m_nYSDoing == ys_return)
			strcpy(szMsg, TV(L"<color=green>Đang trả nhiệm vụ."));
		else if (m_nYSDoing == ys_cancel)
			strcpy(szMsg, TV(L"<color=green>Đang hủy nhiệm vụ."));
		else
			strcpy(szMsg, TV(L"<color=green>Chưa nhận Nhiệm vụ mới!"));
		ys_Result = ys_none;
	}
	else if (m_nQuestID == 1)
	{
		if (szQuest) sprintf(szQuest, TV(L"NV mua vật phẩm tại NPC."));
		sprintf(szMsg, TV(L"<color=green>Nhiệm vụ Mua vật phẩm tại NPC."));
	}
	else if (m_nQuestID == 2)
	{
		if (szQuest) sprintf(szQuest, TV(L"NV tìm vật phẩm theo tên, hệ."));
		sprintf(szMsg, TV(L"<color=green>Nhiệm vụ Tìm vật phẩm theo tên, hệ."));
	}
	else if (m_nQuestID == 3)
	{
		if (szQuest) sprintf(szQuest, TV(L"NV tìm vật phẩm theo thuộc tính."));
		sprintf(szMsg, TV(L"<color=green>Nhiệm vụ Tìm vật phẩm theo thuộc tính."));
	}
	else if (m_nQuestID == 4)
	{
		int nTotalMap = g_pCurPlayer->GetTaskVal(ys_maphave);
		if (nTotalMap >= m_nRqValue)
			ys_Result = ys_return;
		if (theQuest.m_TuZhiOpt.m_bCancelTooLong &&
			nTimeCount > theQuest.m_TuZhiOpt.m_nTuZhiTime * 60 &&
			nTotalMap * 2 < m_nRqValue)
		{
			ys_Result = ys_cancel;
			SaveYSLog(TV(L"Hủy nhiệm vụ do thực hiện quá thời gian thiết lập."));
		}
		if (szQuest) sprintf(szQuest, TV(L"NV mật/đồ chí, hoàn thành %d/%d"), nTotalMap, m_nRqValue);
		sprintf(szMsg, TV(L"<color=green>Nhiệm vụ Thu thập Mật chí/Đồ chí.\n  Hoàn thành: <color=yellow>%d/%d"), nTotalMap, m_nRqValue);
	}
	else if (m_nQuestID == 5)
	{
		int nPreLevel;
		int nOldValue;
		int nDifferent;
		switch (m_nRqType)
		{
		case ys_rqjingyan:
			nOldValue = g_pCurPlayer->GetTaskVal(ys_preexp);
			nPreLevel = g_pCurPlayer->GetTaskVal(ys_prelevel);
			nDifferent = theMain.GetExpUpLevel(nPreLevel, P_PLAYER->m_Level) + (int)g_pCurPlayer->m_nCurExp - nOldValue;
			if (nDifferent >= m_nRqValue)
				ys_Result = ys_return;
			if (szQuest) sprintf(szQuest, TV(L"NV kinh nghiệm, hoàn thành %d/%d"), nDifferent, m_nRqValue);
			sprintf(szMsg, TV(L"<color=green>Nhiệm vụ nâng cấp Kinh nghiệm.\n  Hoàn thành: <color=yellow>%d/%d"), nDifferent, m_nRqValue);
			break;
		case ys_rqmingwang:
			nOldValue = g_pCurPlayer->GetTaskVal(ys_repvalue);
			nDifferent = g_pCurPlayer->GetTaskVal(0x0064) - nOldValue;
			if (nDifferent >= m_nRqValue)
				ys_Result = ys_return;
			if (szQuest) sprintf(szQuest, TV(L"NV danh vọng, hoàn thành %d/%d"), nDifferent, m_nRqValue);
			sprintf(szMsg, TV(L"<color=green>Nhiệm vụ nâng cấp Danh vọng.\n  Hoàn thành: <color=yellow>%d/%d"), nDifferent, m_nRqValue);
			break;
		case ys_rqfuyuan:
			nOldValue = g_pCurPlayer->GetTaskVal(ys_repvalue);
			nDifferent = g_pCurPlayer->GetTaskVal(0x0097) - nOldValue;
			if (nDifferent >= m_nRqValue)
				ys_Result = ys_return;
			if (szQuest) sprintf(szQuest, TV(L"NV phúc duyên, hoàn thành %d/%d"), nDifferent, m_nRqValue);
			sprintf(szMsg, TV(L"<color=green>Nhiệm vụ nâng cấp Phúc duyên.\n  Hoàn thành: <color=yellow>%d/%d"), nDifferent, m_nRqValue);
			break;
		case ys_rqpk:
			nOldValue = g_pCurPlayer->GetTaskVal(ys_repvalue);
			nDifferent = g_pCurPlayer->m_nPKValue - nOldValue;
			if (nDifferent >= m_nRqValue)
				ys_Result = ys_return;
			if (nTimeCount > 300) // 5 minutes
			{
				ys_Result = ys_cancel;
				SaveYSLog(TV(L"Quá thời gian, về thành hủy nhiệm vụ!\n"));
			}
			if (szQuest) sprintf(szQuest, TV(L"NV nâng PK , hoàn thành %d/%d"), nDifferent, m_nRqValue);
			sprintf(szMsg, TV(L"<color=green>Nhiệm vụ nâng trị PK.\n  Hoàn thành: <color=yellow>%d/%d"), nDifferent, m_nRqValue);
			break;
		}
	}
	else if (m_nQuestID == 6)
	{
		int nMapSHXT = g_pCurPlayer->GetTaskVal(ys_shxtmap);
		if (nMapSHXT >= m_nRqValue)
			ys_Result = ys_return;
		sprintf(szMsg, TV(L"<color=green>Nhiệm thu thập mảnh bản đồ SHXT.\n  Hoàn thành: <color=yellow>%d/%d"), nMapSHXT, m_nRqValue);
	}	
	if (!bAutoCheck)
	{
		ExecuteScript("ClearMessage()");
		SystemMessage(szMsg, 1);
		if (m_nYSDoing == ys_doing)
		{
			int nHour = nTimeCount / 3600;
			int nMin = (nTimeCount - 3600 * nHour) / 60;
			int nSec = nTimeCount % 60;
			SystemMessage(1, TV(L"<color=green>Thời gian đã thực hiện: <color=yellow>%02d:%02d:%02d"), nHour, nMin, nSec);
			SystemMessage(m_szYSQuest, 1);
			int nQuestCancel = g_pCurPlayer->GetTaskVal(ys_taskccl);
			int nSHXTMap = g_pCurPlayer->GetTaskVal(ys_shxtmap);
			SystemMessage(1, TV(L"<color=green>NV ngày / Cơ hội hủy / Mảnh SHXT:\n  <color=yellow>%d / %d / %d"), m_nRemainQuest, nQuestCancel, nSHXTMap);
		}
	}

	return ys_Result;
}

YESOU_TASK_DOING CVLQuest::GetRequiredCondition()
{
	YESOU_TASK_DOING ysResult = GetRequiredConditionRaw();
	QTRACE("Dieu kien: QID=%d Minor=%d Rq=%s RqValue=%d Map=%d Npc=%d => %s",
		m_nQuestID, g_pCurPlayer->GetTaskVal(ys_minorid), QTName_Rq(m_nRqType), m_nRqValue,
		m_nRqMapID, m_nRqNpcID, QTName_YS(ysResult));
	return ysResult;
}

YESOU_TASK_DOING CVLQuest::GetRequiredConditionRaw()
{
	// Init Param Values
	m_nRqGenre = -1; m_nRqDetail = -1; m_nRqParticular = -1;
	m_nRqLevel = -1; m_nRqSeries = -1; m_nRqQuantity = 1;
	m_nReqAttrib = -1; m_nMinValue = 0; m_nMaxValue = 0;
	m_nRqNpcID = 0; m_nRqMapID = 0; m_nRqType = 0; m_nRqValue = 0;
	// Init Status Values
	theMap.m_bNeedMoney = theMap.m_bWithDraw;
	m_bUseDoubleExp = 0;
	m_nFirstMapID = (*g_pMapID);
	m_nDoQuestStep = 0;
	m_bNeedGoTrain = 0;
	m_bNeedGoPCK = 0;
	m_bGoTrainGetPK = 0;
	m_nYeSouMapID = 0;
	m_tmStart = 0;

	m_nQuestGID = g_pCurPlayer->GetTaskVal(ys_taskgid);
	m_nQuestID = g_pCurPlayer->GetTaskVal(ys_majorid);

	int nMinorID = g_pCurPlayer->GetTaskVal(ys_minorid) - 2;
	if (m_nQuestID != 4)
	{
		m_nLastMapID = 0;
		if (nMinorID >= TaskInfoHeader.nTaskNum[m_nQuestID - 1])
		{
			SystemMessage(TV(L"<color=yellow>Không nhận dạng được yêu cầu Dã Tẩu!"));
			SaveYSLog(TV(L"Không nhận dạng được yêu cầu Dã Tẩu => Tạm ngưng!"));
			return ys_suspending;
		}
	}

	if (m_nQuestID == 1)
	{
		if (m_nFindItem == act_cancel || m_nFindItem == act_useitem)
		{
			SaveYSLog(CANCEL_BY_USER);
			return ys_cancel;
		}
		else if (m_nFindItem == act_ignore)
			return ys_suspending;
		m_nRqGenre = pTask01[nMinorID].nItemGenre;
		m_nRqDetail = pTask01[nMinorID].nDetail;
		m_nRqParticular = pTask01[nMinorID].nParticular;
		m_nRqLevel = pTask01[nMinorID].nLevel;
		m_nRqSeries = pTask01[nMinorID].nSeries;
		m_nRqNpcID = pTask01[nMinorID].nNpcID;
		m_nRqMapID = pTask01[nMinorID].nMapID;
	}
	else if (m_nQuestID == 2)
	{
		if (m_nFindItem == act_cancel || m_nFindItem == act_useitem)
		{
			SaveYSLog(CANCEL_BY_USER);
			return ys_cancel;
		}
		else if (m_nFindItem == act_ignore)
			return ys_suspending;
		m_nRqGenre = pTask02[nMinorID].nItemGenre;
		m_nRqDetail = pTask02[nMinorID].nDetail;
		m_nRqParticular = pTask02[nMinorID].nParticular;
		m_nRqLevel = pTask02[nMinorID].nLevel;
		m_nRqSeries = pTask02[nMinorID].nSeries;
		m_nReqAttrib = pTask02[nMinorID].nAttribute;
		m_nMinValue = pTask02[nMinorID].nMinValue;
		m_nMaxValue = pTask02[nMinorID].nMaxValue;
	}
	else if (m_nQuestID == 3)
	{
		if (m_nFindItem == act_cancel || m_nFindItem == act_useitem)
		{
			SaveYSLog(CANCEL_BY_USER);
			return ys_cancel;
		}
		else if (m_nFindItem == act_ignore)
			return ys_suspending;
		m_nReqAttrib = pTask03[nMinorID].nAttribute;
		m_nMinValue = pTask03[nMinorID].nMinValue;
		m_nMaxValue = pTask03[nMinorID].nMaxValue;
	}
	else if (m_nQuestID == 4)
	{
		if (m_nTuZhi == act_cancel || m_nTuZhi == act_useitem)
		{
			SaveYSLog(CANCEL_BY_USER);
			return ys_cancel;
		}
		else if (m_nTuZhi == act_ignore)
			return ys_suspending;

		m_nRqMapID = g_pCurPlayer->GetTaskVal(ys_mapid);
		if (!theMove.LoadYeSouMap(m_nRqMapID))
		{
			SystemMessage(TV(L"<color=yellow>Nơi cần đến chưa có MAP!"));
			m_nRqMapID = 0;
			if (m_TuZhiOpt.m_bCancelNoMap)
			{
				SaveYSLog(TV(L"Nơi cần đến chưa có MAP => Hủy nhiệm vụ!"));
				return ys_cancel;
			}
			else
				return ys_suspending;
		}
		int nRqMapType = g_pCurPlayer->GetTaskVal(ys_smapreq);
		if ((nRqMapType & 0xFF) == 1)
			m_nRqType = ys_rqtuzhi;
		else
			m_nRqType = ys_rqmizhi;

		// Check Pass-Maps
		for (int i = 0; i < MAX_9XMAP_NUM; i++)
		{
			if (m_nRqMapID == Map9xID[i])
			{
				if (m_nRqType == ys_rqtuzhi && m_TuZhiOpt.m_TuZhiMapPass[i])
				{
					SaveYSLog(CANCEL_BY_USER);
					return ys_cancel;
				}
				if (m_nRqType == ys_rqmizhi && m_TuZhiOpt.m_MiZhiMapPass[i])
				{
					SaveYSLog(CANCEL_BY_USER);
					return ys_cancel;
				}
				break;
			}
		}
		m_nRqValue = nRqMapType >> 8;
		m_nYeSouMapID = m_nRqMapID;
		m_nSaveMapID = theMain.GetReturnCity(m_nYeSouMapID);
		if (m_nLastMapID != m_nRqMapID)
		{
			if (!P_PLAYER->m_FightMode && IAM_IN_A_PARTY)
			{
				if (IAM_THE_CAPTAIN)
				{
					for (int i = 1; i < MAX_TEAM_MEMBER; i++)
					{
						if (g_pTeam->m_nMember[i] > 0)
						{
							KUiPlayerItem thePlayer;
							memset(&thePlayer, 0, sizeof(KUiPlayerItem));
							strcpy(thePlayer.Name, g_pTeam->m_szMemName[i]);
							thePlayer.uId = g_pTeam->m_nMember[i];
							g_pCoreShell->TeamOperation(TEAM_APPOINT, (int)& thePlayer, 0);
							break;
						}
					}
				}
				g_pCoreShell->TeamOperation(TEAM_LEAVE, 0, 0);
			}
			m_nLastMapID = m_nRqMapID;
		}
	}
	else if (m_nQuestID == 5)
	{
		m_nRqType = pTask05[nMinorID].nRqType;
		m_nRqValue = pTask05[nMinorID].nRqValue;
		theMap.m_bNoDriverRecent = TRUE;
		switch (m_nRqType)
		{
		case ys_rqjingyan:
			if (m_nJingYan == act_cancel || m_nJingYan == act_useitem)
			{
				SaveYSLog(CANCEL_BY_USER);
				return ys_cancel;
			}
			else if (m_nJingYan == act_ignore)
				return ys_suspending;
			break;
		case ys_rqmingwang:
			m_bNeedGoPCK = TRUE;
			if (m_nMingWang == act_cancel || m_nMingWang == act_useitem)
			{
				SaveYSLog(CANCEL_BY_USER);
				return ys_cancel;
			}
			else if (m_nMingWang == act_ignore)
				return ys_suspending;
			if (m_nFirstMapID >= 387 && m_nFirstMapID <= 389)
				m_nDoQuestStep = 5;
			break;
		case ys_rqfuyuan:
			if (m_nFuYuan == act_cancel || m_nFuYuan == act_useitem)
			{
				SaveYSLog(CANCEL_BY_USER);
				return ys_cancel;
			}
			else if (m_nFuYuan == act_ignore)
				return ys_suspending;
			break;
		case ys_rqpk:
			if (m_nYesouPK == act_cancel || m_nYesouPK == act_useitem)
			{
				SaveYSLog(CANCEL_BY_USER);
				return ys_cancel;
			}
			else if (m_nYesouPK == act_ignore)
				return ys_suspending;
			break;
		case ys_rqsongjin:
			if (m_nSongJin == act_cancel || m_nSongJin == act_useitem)
			{
				SaveYSLog(CANCEL_BY_USER);
				return ys_cancel;
			}
			else if (m_nSongJin == act_ignore)
			{
				if (m_AddPointOpt.m_bSJCancel &&
					m_nRqValue < m_AddPointOpt.m_nSJCancelVal)
				{
					SaveYSLog(TV(L"Nhiệm vụ thất bại, điểm yêu cầu thấp => Hủy nhiệm vụ!"));
					return ys_cancel;
				}
				else
					return ys_suspending;
			}
			break;
		}
	}
	else if (m_nQuestID == 6)
	{
		m_nRqType = pTask06[nMinorID].nRqType;
		m_nRqValue = pTask06[nMinorID].nRqValue;
		if (m_nRqValue <= g_pCurPlayer->GetTaskVal(ys_shxtmap))
			return ys_return;
		else
		{
			SaveYSLog(TV(L"Số mảnh bản đồ SHXT hiện có quá ít, không đủ trả nhiệm vụ."));
			return ys_suspending;
		}
	}
	time(&m_tmStart);
	return ys_doing;
}

BOOL CVLQuest::CheckMyItem(PlayerItem* pPlayerItem, int nPlace, BOOL bNoStackItem)
{
	memset(pPlayerItem, -1, sizeof(PlayerItem));
	for (int i = 0; i < MAX_PLAYER_ITEM; i++)
	{
		if (PLAYER_ITEM(i).nPlace != nPlace) continue;
		KItem* pItem = (KItem*)(dwItmBaseAdd + PLAYER_ITEM(i).nIdx * ITM_DATA_SIZE);
		if (pItem->m_nLockCount) continue;
		if (m_nQuestID < 3)
		{
			if (ComAttr.nSpecialType >= 1)
				continue;
			if (m_BuyItemOpt.m_bItemFilter &&
				theCollect.CheckItemAttrib(pItem) >= keep_good_attrib)
				continue;
		}
		if (bNoStackItem && ComAttr.bStack && pItem->m_nCurDur > 1) continue;
		if (m_nRqGenre >= 0)
		{
			if (m_nRqGenre != ComAttr.nItemGenre) continue;
			if (m_nRqDetail != ComAttr.nDetailType) continue;
			if (m_nRqParticular != ComAttr.nParticularType) continue;
		}
		if (m_nRqLevel >= 0 && m_nRqLevel != ComAttr.nLevel) continue;
		if (m_nRqSeries >= 0 && m_nRqSeries != ComAttr.nSeries) continue;
		if (m_nReqAttrib >= 0)
		{
			BOOL bFound = 0;
			for (int j = 13; j < 19; j++)
			{
				if (ItmAttr(j) <= item_begin) continue;
				if (ItmAttr(j) == m_nReqAttrib &&
					pItem->m_aryAllAttrib[j].nValue[0] >= m_nMinValue &&
					pItem->m_aryAllAttrib[j].nValue[0] <= m_nMaxValue)
				{
					bFound = TRUE;
					break;
				}
			}
			if (!bFound) continue;
		}
		*pPlayerItem = PLAYER_ITEM(i);
		return TRUE;
	}
	return 0;
}

BOOL CVLQuest::GetUseableItem(PlayerItem* pPlayerItem, const char* szItemName, int nPlace)
{
	memset(pPlayerItem, -1, sizeof(PlayerItem));
	KItemObject theObject;
	KItem* pItem;
	int nItemIdx, nMinItem = -1;
	char szItemDesc[512];
	char* szDateTime;

	tm expTime;
	time_t now, vnnow, vnexp, minexp = INT_MAX;
	time(&now);
	vnnow = now + 25200; // UTC + 7

	for (int i = 0; i < MAX_PLAYER_ITEM; i++)
	{
		if (nPlace)
			if (PLAYER_ITEM(i).nPlace != nPlace) continue;

		if (PLAYER_ITEM(i).nPlace != pos_equiproom &&
			PLAYER_ITEM(i).nPlace != pos_repositoryroom &&
			PLAYER_ITEM(i).nPlace != pos_repo02 &&
			PLAYER_ITEM(i).nPlace != pos_repo03 &&
			PLAYER_ITEM(i).nPlace != pos_guid) continue;

		nItemIdx = PLAYER_ITEM(i).nIdx;
		pItem = (KItem*)(dwItmBaseAdd + nItemIdx * ITM_DATA_SIZE);
		if (!strstr(ComAttr.szItemName, szItemName)) continue;
		theObject.uGenre = CGOG_ITEM;
		theObject.uId = nItemIdx;
		g_pCoreShell->GetGameData(GDI_GAME_OBJ_DESC, (int)& theObject, (int)szItemDesc);
		szDateTime = strstr(szItemDesc, TV(L"sử dụng:"));
		if (!szDateTime)
		{
			*pPlayerItem = PLAYER_ITEM(i);
			return TRUE;
		}
		memset(&expTime, 0, sizeof(expTime));
		szDateTime = strstr(szDateTime, "-200"); //dd-mm-yyyy
		if (szDateTime)
		{
			while (szDateTime[0] != 0x20) szDateTime--;
			expTime.tm_mday = atoi(szDateTime) + 1;
			szDateTime = strstr(szDateTime, "-");
			if (!szDateTime++) { *pPlayerItem = PLAYER_ITEM(i); return TRUE; }
			expTime.tm_mon = atoi(szDateTime) - 1;
			szDateTime = strstr(szDateTime, "-");
			if (!szDateTime++) { *pPlayerItem = PLAYER_ITEM(i); return TRUE; }
			int year = atoi(szDateTime);
			if (year > 20000)
				expTime.tm_year = year - 19900;
			else
				expTime.tm_year = year - 1900;
		}
		else
		{
			szDateTime = strstr(szItemDesc, " 200"); //yyyy-mm-dd
			if (szDateTime)
			{
				int year = atoi(szDateTime);
				if (year > 20000)
					expTime.tm_year = year - 19900;
				else
					expTime.tm_year = year - 1900;
				//expTime.tm_year = atoi(szDateTime) - 1900;
				szDateTime = strstr(szDateTime, "-");
				if (!szDateTime++) { *pPlayerItem = PLAYER_ITEM(i); return TRUE; }
				expTime.tm_mon = atoi(szDateTime) - 1;
				szDateTime = strstr(szDateTime, "-");
				if (!szDateTime++) { *pPlayerItem = PLAYER_ITEM(i); return TRUE; }
				expTime.tm_mday = atoi(szDateTime) + 1;
			}
			else
			{
				*pPlayerItem = PLAYER_ITEM(i);
				return TRUE;
			}
		}
		vnexp = _mkgmtime(&expTime);
		if (vnnow > vnexp) continue;
		if (vnexp < minexp)
		{
			minexp = vnexp;
			nMinItem = i;
		}
	}
	if (nMinItem >= 0)
	{
		*pPlayerItem = PLAYER_ITEM(nMinItem);
		return TRUE;
	}
	return 0;
}


BOOL CVLQuest::PutItemToRepo(PlayerItem* pRoomItem, PlayerItem* pEquipItem)
{
	if (theMain.OpenLockRoom(theMap.m_nRepositoryPass))
	{
		theMap.m_bRoomLocked = TRUE;
		return 0;
	}

	if (!theMap.m_bDoShoping && theMap.RunningToPos(theMap.NpcPoints[npc_Repository])) return 0;
	theMap.m_bDoShoping = TRUE;

	if (theMap.OpenStoreBox() <= 0) return 0;

	int nContainer = 0; int nRoom = -1;
	if (pRoomItem->nPlace == pos_repositoryroom)
	{
		nContainer = UOC_STORE_BOX;
		nRoom = room_repository;
	}
	else if (pRoomItem->nPlace == pos_repo02)
	{
		nContainer = UOC_STORE_BOX2;
		nRoom = room_repo02;
	}
	else if (pRoomItem->nPlace == pos_repo03)
	{
		nContainer = UOC_STORE_BOX3;
		nRoom = room_repo03;
	}

	static BOOL bFinished = 0;
	KItem* pItem;
	if (PLAY_HAND_IDX)
	{
		POINT itmPos;
		if (bFinished) return 0; // Awaiting
		pItem = (KItem*)(dwItmBaseAdd + PLAY_HAND_IDX * ITM_DATA_SIZE);
		KInventory DestRoom = PLAYER_ROOM[nRoom];
		if (DestRoom.FindRoom(ComAttr.nWidth, ComAttr.nHeight, &itmPos))
			g_pCurPlayer->SwitchItem(nContainer, itmPos.x, itmPos.y);
		else
			g_pCurPlayer->SwitchItem(UOC_ITEM_TAKE_WITH, pEquipItem->nX, pEquipItem->nY); // Put it back
		bFinished = TRUE;
		return 0; // Finish
	}
	if (bFinished)
	{
		bFinished = 0;
		theMap.ResetShoping();
		memset(pRoomItem, -1, sizeof(PlayerItem));
		memset(pEquipItem, -1, sizeof(PlayerItem));
		return TRUE;
	}
	pItem = (KItem*)(dwItmBaseAdd + pEquipItem->nIdx * ITM_DATA_SIZE);
	g_pCurPlayer->SwitchItem(UOC_ITEM_TAKE_WITH, pEquipItem->nX, pEquipItem->nY);
	return 0;
}

BOOL CVLQuest::GetItemFromRepo(PlayerItem* pPlayerItem)
{
	if (theMain.OpenLockRoom(theMap.m_nRepositoryPass))
	{
		theMap.m_bRoomLocked = TRUE;
		return 0;
	}

	if (pPlayerItem->nPlace != pos_guid)
	{
		if (!theMap.m_bDoShoping && theMap.RunningToPos(theMap.NpcPoints[npc_Repository])) return 0;
		theMap.m_bDoShoping = TRUE;

		if (theMap.OpenStoreBox() <= 0) return 0;
	}

	KItem* pItem;
	POINT itmPos;

	int nContainer = 0;
	if (pPlayerItem->nPlace == pos_repositoryroom)
		nContainer = UOC_STORE_BOX;
	else if (pPlayerItem->nPlace == pos_repo02)
		nContainer = UOC_STORE_BOX2;
	else if (pPlayerItem->nPlace == pos_repo03)
		nContainer = UOC_STORE_BOX3;
	else if (pPlayerItem->nPlace == pos_guid)
		nContainer = UOC_GUID_BOX;

	static BOOL bFinished = 0;
	if (PLAY_HAND_IDX)
	{
		if (bFinished) return 0; // Awaiting
		pItem = (KItem*)(dwItmBaseAdd + PLAY_HAND_IDX * ITM_DATA_SIZE);
		if (EQUIP_ROOM.FindRoom(ComAttr.nWidth, ComAttr.nHeight, &itmPos))
			g_pCurPlayer->SwitchItem(UOC_ITEM_TAKE_WITH, itmPos.x, itmPos.y);
		else
			g_pCurPlayer->SwitchItem(nContainer, pPlayerItem->nX, pPlayerItem->nY); // Put it back
		bFinished = TRUE;
		return 0; // Finish
	}
	if (bFinished)
	{
		bFinished = 0;
		theMap.ResetShoping();
		memset(pPlayerItem, -1, sizeof(PlayerItem));
		return TRUE;
	}
	pItem = (KItem*)(dwItmBaseAdd + pPlayerItem->nIdx * ITM_DATA_SIZE);
	if (ComAttr.bStack && pItem->m_nCurDur > 1)
		g_pCoreShell->OperationRequest(GOI_SHIFT_ITEM, pPlayerItem->nIdx, 1);
	else
		g_pCurPlayer->SwitchItem(nContainer, pPlayerItem->nX, pPlayerItem->nY);

	return 0;
}

int CVLQuest::GetNextSearchCity(BOOL bBuySuccess)
{
	// In SmallTown
	int nMapID = *g_pMapID;
	int nOwnerID = theMain.GetOwnerCity(nMapID);
	if (nOwnerID != nMapID)
	{
		m_nFirstMapID = nOwnerID;
		if (!bBuySuccess || nMapID == city_longtuyen)
			return nOwnerID;
		else
			nMapID = nOwnerID;
	}
	// In BigCity
	BOOL bVisit;
	int nCityID;
	int i, nIdx = 0;
	for (nIdx = 0; nIdx < MAX_CITY_NUM; nIdx++)
	{
		nCityID = m_BuyItemOpt.m_VisitCities[nIdx] >> 16;
		if (nCityID == nMapID) break;
	}
	if (nIdx < MAX_CITY_NUM - 1)
		i = nIdx + 1;
	else
		i = 0;

	nCityID = m_BuyItemOpt.m_VisitCities[i] >> 16;
	if (bBuySuccess) return nCityID;

	// Check Visit Setting: toi da 1 vong, quay ve thanh hien tai/thanh bat dau => da tim het
	for (int k = 0; k < MAX_CITY_NUM; k++, i = (i + 1) % MAX_CITY_NUM)
	{
		nCityID = m_BuyItemOpt.m_VisitCities[i] >> 16;
		bVisit = (BOOL)(m_BuyItemOpt.m_VisitCities[i] & 0xFFFF);
		if (nCityID == nMapID) break; // current map
		if (nCityID == m_nFirstMapID) break; // first visited map (ke ca khi khong tick)
		if (!bVisit) continue;
		QTRACE("GetNextSearchCity: %d -> %d (First=%d)", nMapID, nCityID, m_nFirstMapID);
		return nCityID;
	}

	QTRACE("GetNextSearchCity: %d da tim het vong (First=%d)", nMapID, m_nFirstMapID);
	return 0;
}

int CVLQuest::SearchBuyItem(const char* szItemName, BOOL bIsNearYeSou)
{
	static int nTimeWait = 0;
	static int nCurIndex = 0;
	static int nLastMoney = 0;
	static BOOL bFoundIt = 0;
	static char szGotItem[80];
	static int nItemPrice = 0;
	static BOOL bStallLogged = 0;	// da ghi log doi chieu cho lan mo sap hien tai

	char szMsg[128];
	char szLog[640];
	int nX, nY, dMin;
	int nPlayerX, nPlayerY, nNpcX, nNpcY, dZ;

	if (bFoundIt)
	{
		if (nTimeWait) { nTimeWait--; return 1; }

		bFoundIt = 0;
		if (nLastMoney <= EQUIP_MONEY)
		{
			CloseAllWindows(0);
			return 1;
		}

		//g_pCoreShell->OperationRequest(GOI_TRADE_PLAYER_GETPRICE, 0, 0);

		sprintf(szMsg, TV(L"Mua được <color=yellow>%s <color=blue>%d vạn"), szGotItem, nItemPrice / 10000);
		SystemMessage(szMsg);

		sprintf(szMsg, TV(L"Mua được <%s> giá %d vạn lượng."), szGotItem, nItemPrice / 10000);
		SaveYSLog(szMsg);

		CloseAllWindows(0);
		theMap.m_bNeedMoney = theMap.m_bWithDraw;
		return 2; // Successful
	}

	KUiPlayerStall* pPlrStall = (KUiPlayerStall*)GetPtrAddress(STL_BASE_ADD);
	if (pPlrStall && pPlrStall->IsVisible())
	{
		POINT itmPos;
		KItem* pItem;
		static KStallObject pItemSt[MAX_STALL_OBJ];	// 15-10: hang lay bang GetGameData(GDI_STALL_ITEMS)
		int nStallObj = pPlrStall->GetObjects(pItemSt, MAX_STALL_OBJ);
		if (nCurIndex) m_bVsNpc[nCurIndex] = TRUE;

		// Log doi chieu: moi lan mo sap ghi 1 lan (dieu kien can tim + tung mon + ly do loai) vao Logs\<ten>.log
		BOOL bLog = !bStallLogged;
		bStallLogged = TRUE;
		if (bLog && PLAYER_SHOP_LOG)
		{
			if (szItemName)
				sprintf(szLog, TV(L"[Sạp] #%d %s: %d món. Cần tên '%s'"),
					nCurIndex, nCurIndex ? g_pNpcArray[nCurIndex]->Name : "?", nStallObj, szItemName);
			else
				sprintf(szLog, TV(L"[Sạp] #%d %s: %d món. Cần G/D/P=%d/%d/%d Lv=%d Hệ=%d, %s (%d %s) %d..%d"),
					nCurIndex, nCurIndex ? g_pNpcArray[nCurIndex]->Name : "?", nStallObj,
					m_nRqGenre, m_nRqDetail, m_nRqParticular, m_nRqLevel, m_nRqSeries,
					m_nReqAttrib >= 0 ? MagicAttribDesc(m_nReqAttrib) : "-", m_nReqAttrib,
					m_nReqAttrib >= 0 ? MagicAttribName(m_nReqAttrib) : "-", m_nMinValue, m_nMaxValue);
			SaveYSLog(szLog);
		}

		for (int i = 0; i < nStallObj; i++)
		{
			pItem = (KItem*)(dwItmBaseAdd + pItemSt[i].uId * ITM_DATA_SIZE);

			const char* szWhy = 0;
			int nMaxPrice = m_BuyItemOpt.m_MaxPrice[price_other];
			do
			{
				if (ComAttr.nSpecialType >= 1) { szWhy = "SpecialType"; break; }
				if (szItemName)
				{
					if (!strstr(ComAttr.szItemName, szItemName)) { szWhy = TV(L"sai tên"); break; }
					if (strstr(ComAttr.szItemName, TV(L"(Tiểu)")))
						nMaxPrice = m_BuyItemOpt.m_MaxPrice[price_fuyuan01];
					else if (strstr(ComAttr.szItemName, "(Trung)"))
						nMaxPrice = m_BuyItemOpt.m_MaxPrice[price_fuyuan02];
					else if (strstr(ComAttr.szItemName, TV(L"(Đại)")))
						nMaxPrice = m_BuyItemOpt.m_MaxPrice[price_fuyuan03];
				}
				else
				{
					if (m_nRqGenre >= 0)
					{
						if (m_nRqGenre != ComAttr.nItemGenre) { szWhy = TV(L"sai loại (Genre)"); break; }
						if (m_nRqDetail != ComAttr.nDetailType) { szWhy = TV(L"sai loại (Detail)"); break; }
						if (m_nRqParticular != ComAttr.nParticularType) { szWhy = TV(L"sai loại (Particular)"); break; }
						if (m_nRqGenre == item_equip)
						{
							if (m_nRqDetail == equip_ring)
								nMaxPrice = m_BuyItemOpt.m_MaxPrice[price_ring];
							else if (m_nRqDetail == equip_amulet)
								nMaxPrice = m_BuyItemOpt.m_MaxPrice[price_pendant];
							else if (m_nRqDetail == equip_pendant)
								nMaxPrice = m_BuyItemOpt.m_MaxPrice[price_pendant];
						}
						else if (m_nRqGenre == item_special)
						{
							if (m_nRqParticular == 400)
								nMaxPrice = m_BuyItemOpt.m_MaxPrice[price_satthugian];
						}
					}
					if (m_nRqLevel >= 0 && m_nRqLevel != ComAttr.nLevel) { szWhy = TV(L"sai cấp"); break; }
					if (m_nRqSeries >= 0 && m_nRqSeries != ComAttr.nSeries) { szWhy = TV(L"sai hệ"); break; }
					// Check Attribute
					if (m_nReqAttrib >= 0)
					{
						BOOL bFound = 0, bHave = 0;
						for (int j = 13; j < 19; j++)
						{
							if (ItmAttr(j) <= item_begin) continue;
							if (ItmAttr(j) != m_nReqAttrib) continue;
							bHave = TRUE;
							if (pItem->m_aryAllAttrib[j].nValue[0] >= m_nMinValue &&
								pItem->m_aryAllAttrib[j].nValue[0] <= m_nMaxValue)
							{
								bFound = TRUE;
								break;
							}
						}
						if (!bFound) { szWhy = bHave ? TV(L"có option nhưng chưa đủ chỉ số") : TV(L"không có option"); break; }
					}
				}
				// Check Price
				if (pItemSt[i].nPrice > nMaxPrice * 10000) { szWhy = TV(L"giá cao hơn thiết lập"); break; }
				if (pItemSt[i].nPrice > EQUIP_MONEY)
				{
					SystemMessage(TV(L"<color=yellow>Tìm được đồ phù hợp, mà không đủ tiền!"));
					szWhy = TV(L"không đủ tiền");
					break;
				}
				// Check Room
				if (!EQUIP_ROOM.FindRoom(ComAttr.nWidth, ComAttr.nHeight, &itmPos)) { szWhy = TV(L"hết chỗ hành trang"); break; }
			} while (0);

			if (bLog && PLAYER_SHOP_LOG)
			{
				int nLen = sprintf(szLog, TV(L"  [%d] %s G/D/P=%d/%d/%d Lv=%d Hệ=%d giá=%d (tối đa %d vạn):"),
					i, ComAttr.szItemName, ComAttr.nItemGenre, ComAttr.nDetailType, ComAttr.nParticularType,
					ComAttr.nLevel, ComAttr.nSeries, pItemSt[i].nPrice, nMaxPrice);
				for (int j = 13; j < 19; j++)
				{
					if (ItmAttr(j) <= item_begin) continue;
					int nRoom = (int)sizeof(szLog) - nLen - 64;	// chua cho " => ly do"
					if (nRoom <= 0) break;
					int n = _snprintf(szLog + nLen, nRoom, " %s%s(%d)=%d", ItmAttr(j) == m_nReqAttrib ? "*" : "",
						MagicAttribDesc(ItmAttr(j)), ItmAttr(j), pItem->m_aryAllAttrib[j].nValue[0]);
					if (n < 0) { nLen += nRoom; szLog[nLen] = 0; break; }
					nLen += n;
				}
				sprintf(szLog + nLen, " => %s", szWhy ? szWhy : "OK");
				SaveYSLog(szLog);
			}
			if (szWhy) continue;
			// Found it
			bFoundIt = TRUE;
			nLastMoney = EQUIP_MONEY;
			strcpy(szGotItem, ComAttr.szItemName);
			nItemPrice = pItemSt[i].nPrice;
			g_pCoreShell->OperationRequest(GOI_TRADE_PLAYER_BUY, pItemSt[i].uId, 0);
			nTimeWait = 2;
			return 1;
		}
		CloseAllWindows(0);
		nTimeWait = 0;
		return 1; // Continue
	}
	bStallLogged = 0;
	if (nTimeWait) { nTimeWait--; return 1; }

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
		if (g_pNpcArray[i]->m_nMenuState != 1) continue;	// 1 = dang bay sap
		if (m_bVsNpc[i]) continue; // Already Visited.
		g_pNpcArray[i]->GetMapPos(&nNpcX, &nNpcY);
		if (bIsNearYeSou)
		{
			dZ = __distance(theMap.NpcPoints[npc_YeSou].x, theMap.NpcPoints[npc_YeSou].y, nNpcX, nNpcY);
			if (dZ > 1600)
			{
				m_bVsNpc[i] = TRUE;
				continue;
			}
		}
		dZ = __distance(nPlayerX, nPlayerY, nNpcX, nNpcY);
		if (dZ < dMin)
		{
			dMin = dZ;
			nCurIndex = i;
			nX = nNpcX;
			nY = nNpcY;
		}
	}
	if (!nCurIndex) return 0; // Not Found

	theAttack.SetPeopleIdx(nCurIndex);

	nTimeWait = MAX_TIME_WAIT;
	return 1; // Continue
}

void CVLQuest::ArrangeEquipRoom()
{
	POINT itmPos;
	if (!m_nArrangeStep) return;
	if (m_nArrangeStep == 1)
	{
		if (EQUIP_ROOM.FindRoom(2, 3, &itmPos))
		{
			m_nArrangeStep = 0;
			return;
		}
		else
		{
			m_nArrangeStep = 2;
			SystemMessage(TV(L"<color=yellow>Đang sắp xễp lại hành trang..."));
		}
	}
	if (m_nArrangeStep == 2)
	{
		if (theCollect.CheckHandItem(PLAY_HAND_IDX)) return;
		if (!theCollect.ArrangeItems())
		{
			if (EQUIP_ROOM.FindRoom(2, 3, &itmPos))
				m_nArrangeStep = 0;
			else
			{
				if (!g_pCurPlayer->EatMedicine(medicine_antipoison))
				{
					if (!g_pCurPlayer->EatMedicine(medicine_mana))
					{
						if (!g_pCurPlayer->EatMedicine(medicine_blood))
							m_nArrangeStep = 0;
					}
				}
			}
		}
	}
	return;
}
