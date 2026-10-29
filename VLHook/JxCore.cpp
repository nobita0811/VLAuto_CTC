#include "stdafx.h"
#include "../GameDef.h"
#include "GameBase.h"
#include "JxCore.h"
#include <intrin.h>

BOOL DebugQuestMoveOK(void *pCaller);	// VLDebugQuest.cpp

// Global Variables
DWORD				dwObjBaseAdd;
DWORD				dwItmBaseAdd;

KPlayer*			g_pCurPlayer;
KNpc*				g_pNpcArray[MAX_NPC];
KObj*				g_pObjArray[MAX_OBJ];
KItem*				g_pItemArray[MAX_ITM];

KTeam*				g_pTeam;
KCoreShell*			g_pCoreShell;
KBuySell*			g_pBuySell;
GlobalTongInfo*		g_GlobalTongInfo = NULL;
int*				g_pMapID;
char*				g_szMapName;

// Global Functions
BOOL(*ExecuteScript)(const char*);
void(*SendClientCmdSkill)(int, int, int);

unsigned int HashString2ID(char* szFileName)
{
	unsigned int Id = 0;
	char c = 0;
	for (int i = 0; szFileName[i]; i++)
	{
		c = szFileName[i];
		Id = (Id + (i + 1) * c) % 0x8000000b * 0xffffffef;
	}
	return (Id ^ 0x12345678);
}

void PostChatMessage(const char *szChatMsg)
{
	char szScript[MAX_PATH];
	sprintf(szScript, "Chat('CH_NEARBY','%s')", szChatMsg);
	ExecuteScript(szScript);
}
void PostChatMessage2(const char* szChatMsg)
{
	if (QUANGCAOAUTO)
	{
		char szScript[MAX_PATH];
		sprintf(szScript, "Chat('CH_NEARBY','%s')", szChatMsg);
		ExecuteScript(szScript);
	}	
}
// Chu hoa TCVN3: a-z -> A-Z, ¨ © ª « ¬ ­ ® (0xA8-0xAE) -> ¡ ¢ £ ¤ ¥ ¦ § (0xA1-0xA7).
// Chu co dau (a sac, a mu sac, o mu huyen...) TCVN3 chi co 1 ma dung chung cho hoa/thuong => giu nguyen.
// Khong dung toupper/CharUpper: chung doi theo code page Windows (0xE0-0xFE...) lam hong ma TCVN3
unsigned char TcvnUpChar(unsigned char c)
{
	if (c >= 'a' && c <= 'z') return c - ('a' - 'A');
	if (c >= 0xA8 && c <= 0xAE) return c - (0xA8 - 0xA1);
	return c;
}

char* TcvnUpper(char* sz)
{
	for (unsigned char *p = (unsigned char*)sz; *p; p++)
		*p = TcvnUpChar(*p);
	return sz;
}

int TcvnStrCmpI(const char* sz1, const char* sz2)
{
	const unsigned char *p1 = (const unsigned char*)sz1, *p2 = (const unsigned char*)sz2;
	unsigned char c1, c2;
	do
	{
		c1 = TcvnUpChar(*p1++);
		c2 = TcvnUpChar(*p2++);
	} while (c1 && c1 == c2);
	return c1 - c2;
}

int TcvnStrNCmpI(const char* sz1, const char* sz2, int n)
{
	const unsigned char *p1 = (const unsigned char*)sz1, *p2 = (const unsigned char*)sz2;
	unsigned char c1 = 0, c2 = 0;
	while (n-- > 0)
	{
		c1 = TcvnUpChar(*p1++);
		c2 = TcvnUpChar(*p2++);
		if (!c1 || c1 != c2) break;
	}
	return c1 - c2;
}

// Nhu strstr nhung khong phan biet hoa/thuong (TCVN3); tra ve vi tri trong szText
const char* TcvnStrStrI(const char* szText, const char* szFind)
{
	if (!szText || !szFind) return NULL;
	if (!*szFind) return szText;
	for (; *szText; szText++)
	{
		const unsigned char *t = (const unsigned char*)szText, *f = (const unsigned char*)szFind;
		while (*f && TcvnUpChar(*t) == TcvnUpChar(*f)) { t++; f++; }
		if (!*f) return szText;
	}
	return NULL;
}

int __distance(int X1, int Y1, int X2, int Y2)
{
	double dX = (double)(X2 - X1);
	double dY = (double)(Y2 - Y1);
	return (int)sqrt(dX*dX + dY * dY);
}

int __distance(POINT p1, POINT p2)
{
	return __distance(p1.x, p1.y, p2.x, p2.y);
}

int __getheight(POINT p1, POINT p2, POINT p3)
{
	double dZ = (double)(p1.x - p2.x)*(p1.x - p2.x) + (double)(p1.y - p2.y)*(p1.y - p2.y);
	if (dZ == 0) return 0;
	double dX = (double)(p1.x - p2.x)*(p1.y - p2.y) / dZ * (p3.y - p2.y) + (double)(p1.y - p2.y)*(p1.y - p2.y) / dZ * (p2.x - p3.x);
	double dY = (double)(p1.x - p2.x)*(p1.y - p2.y) / dZ * (p3.x - p2.x) + (double)(p1.x - p2.x)*(p1.x - p2.x) / dZ * (p2.y - p3.y);
	return (int)sqrt(dX*dX + dY * dY);
}

// Class KNpc
BOOL KNpc::IsExist()
{
	return (m_Index > 0);
}

BOOL KNpc::IsNotMob()
{
	if (m_Index == 0)
		return FALSE;

	if (m_CurrentLife <= 0)
		return FALSE;

	return (m_Kind != kind_normal);
}

void KNpc::GetMapPos(int *nX, int *nY)
{
	*nX = m_RegX * REGION_WIDTH + m_MapX * CELL_WIDTH + (m_OffX >> 10);
	*nY = m_RegY * REGION_HEIGHT + m_MapY * CELL_HEIGHT + (m_OffY >> 10);
}

// KNpc::GetMpsPos cua game: region[m_RegionIndex].LeftTop + m_MapX/Y * cell + (m_OffX/Y >> 10).
// Day la toa do game dung khi tu danh (NpcSet::GetDistanceSquare / KNpcAI::FollowAttack).
// m_RegionIndex ngoai 0..so region thi game khong ghi => giu gia tri GetMapPos.
void KNpc::GetGamePos(int *nX, int *nY)
{
	GetMapPos(nX, nY);
	__asm
	{
		push nY;
		push nX;
		mov ecx, this;
		mov eax, NMP_FUNC_ADD;
		call eax;
	}
}

// NPC "ma": con trong mang NPC client nhung o region game dang giu da bi nap lai cho vung khac
// => game tinh toa do (GetGamePos) lech voi toa do dong bo (GetMapPos, m_RegX/m_RegY), khong ve,
// nhung KNpcAI::FollowAttack van thay "trong tam" va ra skill vao cho trong.
BOOL KNpc::IsGhost()
{
	int nX, nY, nGX, nGY;
	GetMapPos(&nX, &nY);
	GetGamePos(&nGX, &nGY);
	return abs(nX - nGX) > CELL_WIDTH || abs(nY - nGY) > CELL_HEIGHT;
}

void KNpc::MoveTo(int nX, int nY, int nAction)
{
	__asm
	{
		push nAction;
		push nY;
		push nX;
		mov eax, MVT_FUNC_ADD;
		call eax;
	}
}

void KNpc::SendCommand(NPCCMD cmd, int x, int y, int z)
{
	m_Command.CmdKind = cmd;
	m_Command.Param_X = x;
	m_Command.Param_Y = y;
	m_Command.Param_Z = z;
}

// Class KObj
void KObj::GetMapPos(int *nX, int *nY)
{
	*nX = m_RegX * REGION_WIDTH + m_MapX * CELL_WIDTH + (m_OffX >> 10);
	*nY = m_RegY * REGION_HEIGHT + m_MapY * CELL_HEIGHT + (m_OffY >> 10);
}

// Class KItem
int KItem::GetTotalMagicLevel()
{
	int nRet = 0;
	for (int i = 0; i < 6; i++)
	{
		nRet += m_MagicLevel[i];
	}
	return nRet;
}

int KItem::GetRepairPrice()
{
	__asm
	{
		mov eax, IGR_FUNC_ADD;
		call eax;
	}
}

// Class KInventory
BOOL KInventory::FindRoom(int nWidth, int nHeight, POINT* pPos, BOOL bReversed, int itemIdx)
{
	if (!pPos)
		return FALSE;
	if (nWidth < 1 || nWidth > m_nMaxWidth || nHeight < 1 || nHeight > m_nMaxHeight)
		return FALSE;

	int nMaxX = m_nMaxWidth - nWidth;
	int nMaxY = m_nMaxHeight - nHeight;

	if (bReversed)
	{
		for (int j = nMaxY; j >= 0; j--)
		{
			for (int i = nMaxX; i >= 0; i--)
			{
				if (CanPlaceItem(i, j, nWidth, nHeight, itemIdx))
				{
					pPos->x = i;
					pPos->y = j;
					return TRUE;
				}
			}
		}
	}
	else
	{
		for (int j = 0; j <= nMaxY; j++)
		{
			for (int i = 0; i <= nMaxX; i++)
			{
				if (CanPlaceItem(i, j, nWidth, nHeight, itemIdx))
				{
					pPos->x = i;
					pPos->y = j;
					return TRUE;
				}
			}
		}
	}
	pPos->x = 0;
	pPos->y = 0;
	return FALSE;
}

//FindRoomR: Reversed Finding
BOOL KInventory::CanPlaceItem(int nX, int nY, int nWidth, int nHeight, int itemIdx)
{
	if (!m_pArray)
		return FALSE;

	for (int i = nX; i < nX + nWidth; i++)
	{
		for (int j = nY; j < nY + nHeight; j++)
		{
			int posVal = m_pArray[j * m_nWidth + i];
			if (posVal > 0 && posVal != itemIdx)
				return FALSE;
		}
	}

	return TRUE;
}

// Class KBuySell
KItem* KBuySell::GetItem(int nIndex)
{
	__asm
	{
		push nIndex;
		mov eax, BGI_FUNC_ADD;
		call eax;
	}
}

int KBuySell::GetItemIndex(int nBuyIdx, int nID)
{
	__asm
	{
		push nID;
		push nBuyIdx;
		mov eax, BGX_FUNC_ADD;
		call eax;
	}
}

// Class KPlayerTask
int KPlayerTask::GetTaskVal(int nTaskNum)
{
	__asm
	{
		push nTaskNum;
		mov eax, GTV_BASE_ADD;
		call eax;
	}
}

// Class Player
void KPlayer::DoSit()
{
	if (m_nSendMoveFrames >= MAX_PLAYER_MOVE_FRAME)
	{
		ExecuteScript(SCK_SHORTCUT_SIT);
		m_nSendMoveFrames = 0;
	}
}

void KPlayer::MoveTo(KNpc* pNpc)
{
	int nNpcX, nNpcY, nPlayerX, nPlayerY;
	pNpc->GetMapPos(&nNpcX, &nNpcY);
	P_PLAYER->GetMapPos(&nPlayerX, &nPlayerY);
	P_PLAYER->MoveTo((nPlayerX + nNpcX) / 2, (nPlayerY + nNpcY) / 2);
}

void KPlayer::MoveTo(int nX, int nY)
{
	int nDestX = nX;
	int nDestY = nY;
	int nPlayerX, nPlayerY, dZ;

	if (!DebugQuestMoveOK(_ReturnAddress())) return;	// DEBUG > Nhiem vu: Buoc 4 / giu dung tai map
	if (m_nSendMoveFrames > MAX_PLAYER_MOVE_FRAME)
	{
		P_PLAYER->GetMapPos(&nPlayerX, &nPlayerY);
		dZ = __distance(nDestX, nDestY, nPlayerX, nPlayerY);
		if (dZ > MAX_MOVE_DISTANCE)
		{
			nDestX = nPlayerX + (nDestX - nPlayerX)*MAX_MOVE_DISTANCE / dZ;
			nDestY = nPlayerY + (nDestY - nPlayerY)*MAX_MOVE_DISTANCE / dZ;
		}
		P_PLAYER->MoveTo(nDestX, nDestY);
		m_nSendMoveFrames = 0;
	}
}
void KPlayer::GoTo(int nX, int nY)
{
	if (!DebugQuestMoveOK(_ReturnAddress())) return;
	int x = nX / 32;
	int y = nY / 32;
	__asm
	{
		push y;
		push x;
		mov ecx, GOTO_BASE_ADD;
		mov eax, GOTO_FUNC_ADD;
		call eax;
	}
}


int KPlayer::GetTaskVal(int nTaskNum)
{
	int retValue = m_cTask.GetTaskVal(nTaskNum);
	if (nTaskNum == ys_preexp)
		retValue += m_cTask.GetTaskVal(ys_preexp_ex) * 100000;

	return retValue;
}

void KPlayer::PickUpObj(int nObjIndex)
{
	__asm
	{
		push nObjIndex;
		mov eax, PKO_FUNC_ADD;
		call eax;
	}
}

int KPlayer::CheckRoom(int nRoom)
{
	int nRoomWidth = m_ItemList.m_Room[nRoom].m_nWidth;
	int nRoomHeight = m_ItemList.m_Room[nRoom].m_nHeight;
	int *pArray = m_ItemList.m_Room[nRoom].m_pArray;
	int i, j, nCount = 0;
	for (j = 0; j < nRoomHeight; j++)
	{
		for (i = 0; i < nRoomWidth; i++)
		{
			if (pArray[j*nRoomWidth + i])
				nCount++;
		}
	}
	return nCount;
}

int KPlayer::GetItemsCount(const char *szItemName, int nPlace)
{
	KItem *pItem;
	int nItemIdx, nCount = 0;
	for (int i = 0; i < MAX_PLAYER_ITEM; i++)
	{
		nItemIdx = m_ItemList.m_Items[i].nIdx;
		if (!nItemIdx) continue;
		if (nPlace > 0 && m_ItemList.m_Items[i].nPlace != nPlace)
			continue;
		pItem = (KItem*)(dwItmBaseAdd + nItemIdx * ITM_DATA_SIZE);
		if (strstr(ComAttr.szItemName, szItemName))
		{
			if (pItem->m_CommonAttrib.bStack)
				nCount += pItem->m_nCurDur;
			else
				nCount++;
		}
	}
	return nCount;
}

int KPlayer::GetItemsCount(int nGenre, int nDetailType, int nParticular, int nSeries, int nPlace)
{
	KItem *pItem;
	int nItemIdx, nCount = 0;
	for (int i = 0; i < MAX_PLAYER_ITEM; i++)
	{
		nItemIdx = m_ItemList.m_Items[i].nIdx;
		if (!nItemIdx) continue;
		if (nPlace > 0 && m_ItemList.m_Items[i].nPlace != nPlace)
			continue;
		pItem = (KItem*)(dwItmBaseAdd + nItemIdx * ITM_DATA_SIZE);
		if (nParticular >= 0 && ComAttr.nParticularType != nParticular) continue;
		if (nSeries >= 0 && ComAttr.nSeries != nSeries) continue;
		if (ComAttr.nItemGenre == nGenre && ComAttr.nDetailType == nDetailType)
		{
			if (pItem->m_CommonAttrib.bStack)
				nCount += pItem->m_nCurDur;
			else
				nCount++;
		}
	}
	return nCount;
}

BOOL KPlayer::UseItem(const char *szItemName)
{
	KItem *pItem;
	int nItemIdx;
	int nRoomWidth = m_ItemList.m_Room[room_equipment].m_nWidth;
	int nRoomHeight = m_ItemList.m_Room[room_equipment].m_nHeight;
	int *pArray = m_ItemList.m_Room[room_equipment].m_pArray;
	int i, j;
	for (j = 0; j < nRoomHeight; j++)
	{
		for (i = 0; i < nRoomWidth; i++)
		{
			nItemIdx = pArray[j*nRoomWidth + i];
			if (!nItemIdx) continue;
			pItem = (KItem*)(dwItmBaseAdd + nItemIdx * ITM_DATA_SIZE);
			if (strstr(ComAttr.szItemName, szItemName))
			{
				KItemObject useObj;
				useObj.uId = nItemIdx;
				useObj.uGenre = CGOG_ITEM;
				useObj.DataX = i;
				useObj.DataY = j;
				g_pCoreShell->OperationRequest(GOI_USE_ITEM, (int)&useObj, UOC_ITEM_TAKE_WITH);
				return TRUE;
			}
		}
	}
	return 0;
}

BOOL KPlayer::EatMedicine(int nDetailType)
{
	KItem* pItem;
	int nItemIdx;
	int nRoomWidth = m_ItemList.m_Room[room_equipment].m_nWidth;
	int nRoomHeight = m_ItemList.m_Room[room_equipment].m_nHeight;
	int* pArray = m_ItemList.m_Room[room_equipment].m_pArray;

	for (int j = 0; j < nRoomHeight; j++)
	{
		for (int i = 0; i < nRoomWidth; i++)
		{
			nItemIdx = pArray[j*nRoomWidth + i];
			if (!nItemIdx) continue;
			pItem = (KItem*)(dwItmBaseAdd + nItemIdx * ITM_DATA_SIZE);

			BOOL bFound = 0;

			if (ComAttr.nItemGenre == item_medicine)
			{
				bFound = ((ComAttr.nDetailType == nDetailType) ||
					(ComAttr.nDetailType == medicine_both && nDetailType <= medicine_mana) || (ComAttr.nDetailType == medicine_both_ex && nDetailType <= medicine_mana));
			}
			else if (ComAttr.nItemGenre == item_special)
			{
				bFound = ((nDetailType <= medicine_mana) &&
					(ComAttr.nParticularType >= 0xD7 && ComAttr.nParticularType <= 0xD9));
			}

			if (bFound)
			{
				KItemObject useObj;
				useObj.uId = nItemIdx;
				useObj.uGenre = CGOG_ITEM;
				useObj.DataX = i;
				useObj.DataY = j;
				g_pCoreShell->OperationRequest(GOI_USE_ITEM, (int)&useObj, UOC_ITEM_TAKE_WITH);
				return TRUE;
			}
		}
	}

	return 0;
}

void KPlayer::BuyItem(int nItemIdx)
{
	KItemObject Obj;
	Obj.uGenre = CGOG_NPCSELLITEM;
	Obj.uId = nItemIdx;
	Obj.eContainer = UOC_NPC_SHOP;
	g_pCoreShell->OperationRequest(GOI_TRADE_NPC_BUY, (unsigned int)(&Obj), 0);
}

void KPlayer::SellItem(int nItemIdx)
{
	KItemObject Obj;
	Obj.uGenre = CGOG_ITEM;
	Obj.uId = nItemIdx;
	Obj.eContainer = UOC_ITEM_TAKE_WITH;
	g_pCoreShell->OperationRequest(GOI_TRADE_NPC_SELL, (unsigned int)(&Obj), 0);
}

void KPlayer::RepairItem(int nItemIdx)
{
	KItemObject Obj;
	Obj.uGenre = CGOG_ITEM;;
	Obj.uId = nItemIdx;
	Obj.eContainer = UOC_EQUIPTMENT;
	g_pCoreShell->OperationRequest(GOI_TRADE_NPC_REPAIR, (unsigned int)(&Obj), 0);
}

// 10-10: giong click item o che do "Giai" (KUiItem 0x492620):
// OperationRequest(0x9B, &Obj, UOC_ITEM_TAKE_WITH). Core chi nhan uGenre == CGOG_ITEM,
// uId 1..0x1FF, va gui len server vi tri (place, DataX, DataY) chu khong gui index.
BOOL KPlayer::DecomposeItem(int nItemIdx)
{
	if (!GOI_DECOMPOSE_EQUIP) return FALSE;	// 15-10: game khong con tinh nang Ra trang bi
	int nRoomWidth = m_ItemList.m_Room[room_equipment].m_nWidth;
	int nRoomHeight = m_ItemList.m_Room[room_equipment].m_nHeight;
	int* pArray = m_ItemList.m_Room[room_equipment].m_pArray;
	if (nItemIdx <= 0 || !pArray) return FALSE;

	for (int j = 0; j < nRoomHeight; j++)
	{
		for (int i = 0; i < nRoomWidth; i++)
		{
			if (pArray[j*nRoomWidth + i] != nItemIdx) continue;
			KItemObject Obj = {0};
			Obj.uGenre = CGOG_ITEM;
			Obj.uId = nItemIdx;
			Obj.DataX = i;
			Obj.DataY = j;
			Obj.eContainer = UOC_ITEM_TAKE_WITH;
			g_pCoreShell->OperationRequest(GOI_DECOMPOSE_EQUIP, (int)&Obj, UOC_ITEM_TAKE_WITH);
			return TRUE;
		}
	}
	return FALSE;
}

void KPlayer::SwitchItem(int nContainer, int nX, int nY)
{
	KItemObject Obj;
	Obj.DataX = nX;
	Obj.DataY = nY;
	Obj.eContainer = nContainer;
	g_pCoreShell->OperationRequest(GOI_SWITCH_OBJECT, (int)&Obj, 0);
}

void KPlayer::DialogNpc(int nIndex)
{
	__asm
	{
		push nIndex;
		mov eax, DNPC_FUNC_ADD;
		call eax;
	}
}

BOOL KPlayer::EnterTongMap(int nStep, DWORD dwTongID)
{
	if (!dwTongID)
		dwTongID = m_cTong.m_dwTongNameID;
	if (!dwTongID && g_GlobalTongInfo)
		dwTongID = g_GlobalTongInfo[0].dwTongID;
	if (!dwTongID)
		return 0; // Invalid TongID

	static int nCount = 0;
	static int nLastMapID = 0;
	static char szLastMapNm[32] = "";

	char *szLanhDia = strstr(g_szMapName, TV(L" - lã"));
	if (szLanhDia)
	{
		char szMapTongName[16] = "";
		strncpy(szMapTongName, g_szMapName, szLanhDia - g_szMapName);
		if (HashString2ID(szMapTongName) == dwTongID)
			return TRUE;	// Already in this TONG
	}

	switch (nStep)
	{
	case 0: // EnterDirect
		g_pCoreShell->TongOperation(0x23, dwTongID, 0);
		break;
	case 1:	// SaveStatus
		nCount = 0;
		nLastMapID = *g_pMapID;
		strcpy(szLastMapNm, g_szMapName);
		g_pCoreShell->TongOperation(0x23, dwTongID, 0);
		break;
	case 2: // CheckStatus
		nCount++;
		if (nCount > 10 || nLastMapID != *g_pMapID || strcmp(szLastMapNm, g_szMapName))
			return TRUE;
		break;
	}

	return 0;
}

void KPlayer::ApplyAddBaseAttribute(int nAttribute, int nNo)
{
	__asm
	{
		push nNo;
		push nAttribute;
		mov eax, PAA_FUNC_ADD;
		call eax;
	}
}

// Class KCoreShell
int KCoreShell::GetGameData(unsigned int nRequest, int uParam, int nParam)
{
	__asm
	{
		push nParam;
		push uParam;
		push nRequest;
		mov eax, GGD_FUNC_ADD;
		call eax;
	}
}

int KCoreShell::OperationRequest(unsigned int nRequest, int uParam, int nParam)
{
	__asm
	{
		push nParam;
		push uParam;
		push nRequest;
		mov eax, OPR_FUNC_ADD;
		call eax;
	}
}

int KCoreShell::GetNPCRelation(int nIndex)
{
	__asm
	{
		push nIndex;
		mov eax, GRL_FUNC_ADD;
		call eax;
	}
}

int KCoreShell::TongOperation(unsigned int nRequest, int uParam, int nParam)
{
	__asm
	{
		push nParam;
		push uParam;
		push nRequest;
		mov eax, TNP_FUNC_ADD;
		call eax;
	}
}

int KCoreShell::TeamOperation(unsigned int nRequest, int uParam, int nParam)
{
	__asm
	{
		push nParam;
		push uParam;
		push nRequest;
		mov eax, TOP_FUNC_ADD;
		call eax;
	}
}

int KCoreShell::ProcessGameSecurity(unsigned int nRequest, int uParam, int nParam)
{
	__asm
	{
		push nParam;
		push uParam;
		push nRequest;
		mov eax, PGS_FUNC_ADD;
		call eax;
	}
}
