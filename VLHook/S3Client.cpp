#include "stdafx.h"
#include "../GameDef.h"
#include "GameBase.h"
#include "JxCore.h"
#include "S3Client.h"

// Global Variables
DWORD_PTR			g_dwNullUiPtr = 0;	// xem NULL_PTR_ADD trong GameBase.h
KUiInformation*		g_UiInformation;
KUiInformation2*	g_UiInformation2;

// Global Functions
void(*ProcessPeople)(KUiPlayerItem* pDest, int nAction);
void(*OnSendSomeoneMessage)(const char*, const char*, int);
void(*SystemMessageArrival)(const char*, unsigned short, int);
void(*OpenStoreWindow)();
void(*OpenShopWindow)();
void(*CloseAllWindows)(int);

void SendPrivateMessage(const char *szName, const char *szMsg)
{
	int nLen = (int)strlen(szMsg);
	OnSendSomeoneMessage(szName, szMsg, nLen);
}

void SystemMessage(const char *szMsg, int nChanel)
{
	int nMaxLen = (int)strlen(szMsg) + 1;
	if (nMaxLen > 1)
		SystemMessageArrival(szMsg, nMaxLen, nChanel);
}

void SystemMessage(int nChanel, const char *szMsgFormat, ...)
{
	char szMsg[260] = "";

	va_list vlist;
	va_start(vlist, szMsgFormat);
	vsprintf(szMsg, szMsgFormat, vlist);
	va_end(vlist);

	SystemMessage(szMsg, nChanel);
}

// Class KWndImage
BOOL KWndImage::IsVisible()
{
	return (m_Style & WND_S_VISIBLE);
}

// Class KUiInit
void KUiInit::ClickButton(DWORD dwButton)
{
	__asm
	{
		mov ecx, this;
		mov eax, ecx;
		add eax, dwButton;
		push eax;
		mov eax, IOC_FUNC_ADD;
		call eax;
	}
}

void KUiInit::EnterGame()
{
	ClickButton(ENTERGAME_BUTTON);
}

void KUiInit::QuitGame()
{
	ClickButton(EXITGAME_BUTTON);
}

// Class KUiMsgSel
void KUiMsgSel::OnClickMsg(int nMsg)
{
	__asm
	{
		push nMsg;
		mov eax, OMS_FUNC_ADD;
		call eax;
	}
}

// Class KUiMsgSel2
void KUiMsgSel2::OnClickMsg(int nMsg)
{
	__asm
	{
		push nMsg;
		mov eax, OMS2_FUNC_ADD
			call eax;
	}
}

// Class KUiPlayerBar
void KUiPlayerBar::OnClickGShop()
{
	__asm
	{
		push 0;
		mov eax, ecx;
		add eax, GSHOP_BUTTON;
		push eax;
		push WND_N_BUTTON_CLICK;
		mov eax, UWP_FUNC_ADD;
		call eax;
	}
}

// Class KUiMiniMap
// 10-10: ham game la cdecl (x, y, bWorld) nhan toa do tuyet doi,
// con nDx/nDy van la do lech theo don vi co (x>>4, y>>5) nhu ban cu
void KUiMiniMap::SetMapFlag(int nDx, int nDy)
{
	int nX, nY;
	P_PLAYER->GetMapPos(&nX, &nY);
	nX += nDx << 4;
	nY += nDy << 5;
	__asm
	{
		push 1;
		push nY;
		push nX;
		mov eax, SMF_FUNC_ADD;
		call eax;
		add esp, 0x0C;
	}
}

// Class KUiInformation
void KUiInformation::Hide(int nBtnIndex)
{
	__asm
	{
		push nBtnIndex;
		mov eax, IH1_FUNC_ADD;
		call eax;
	}
}

// Class KUiInformation2
void KUiInformation2::Hide()
{
	__asm
	{
		mov eax, IH2_FUNC_ADD;
		call eax;
	}
}

// Class KUiYeSouBox
void KUiYeSouBox::OnConfirm(int btnYes)
{
	if (!OCB_FUNC_ADD) return;	// 10-10: chua tim
	__asm
	{
		push btnYes;
		mov eax, OCB_FUNC_ADD;
		call eax;
		add esp, 0x04;
	}
}

// Class KUiPrizeSel
void KUiPrizeSel::OnClickBtn(int nBtn)
{
	g_pCoreShell->OperationRequest(GOI_QUESTION_CHOOSE, 0, nBtn);
	if (!PSC_FUNC_ADD) return;	// 10-10: chua tim
	__asm
	{
		push 0;
		mov eax, PSC_FUNC_ADD;
		call eax;
		add esp, 0x04;
	}
}

// Class KUiSysMsgCentre
void KUiSysMsgCentre::DeleteMsgInHeap(int nHeapIndex, int nMsgIndex)
{
	__asm
	{
		push nMsgIndex;
		push nHeapIndex;
		mov eax, SMD_FUNC_ADD;
		call eax;
	}
}

// Class KUiTongManagement
int KUiTongManagement::PrepareTongList(int nSortType)
{
	if (!TGP_FUNC_ADD) return 0;	// 10-10: chua tim
	__asm
	{
		push nSortType;
		mov eax, TGP_FUNC_ADD;
		call eax;
	}
}

TongList* KUiTongManagement::GetTongList()
{
	if (!TGL_FUNC_ADD) return NULL;	// 10-10: chua tim
	__asm
	{
		push 0;
		mov eax, TGL_FUNC_ADD;
		call eax;
	}
}
