#include "stdafx.h"
#include "../GameDef.h"
#include "GameBase.h"
#include "JxCore.h"
#include "KProtocol.h"

void KProtocol::SendPackToServer(LPCVOID pData, int nLength)
{
	DWORD dwFuncAdd = __vfptr[4];
	__asm
	{
		mov ecx, this;
		push nLength;
		push pData;
		call dwFuncAdd;
	}
}

void KProtocol::SendClientCmdWalk(int nX, int nY)
{
	KPlayer* pMyPlayer = (KPlayer*)(GetPtrAddress(PLR_BASE_ADD) + PLAYER_OFFSET);
	if (pMyPlayer->m_nEntrustMode)
		return;

	NPC_WALK_COMMAND NetCommand;
	NetCommand.ProtocolType = (BYTE)c2s_npcwalk;
	NetCommand.nMpsX = nX;
	NetCommand.nMpsY = nY;
	NetCommand.nMpsZ = 0;

	KProtocol* pClient = (KProtocol*)GetPtrAddress(KPR_BASE_ADD);

	if (pClient != NULL)
		pClient->SendPackToServer(&NetCommand, sizeof(NetCommand));
}

void KProtocol::SendClientCmdRun(int nX, int nY)
{
	KPlayer* pMyPlayer = (KPlayer*)(GetPtrAddress(PLR_BASE_ADD) + PLAYER_OFFSET);
	if (pMyPlayer->m_nEntrustMode)
		return;

	NPC_RUN_COMMAND NetCommand;
	NetCommand.ProtocolType = (BYTE)c2s_npcrun;
	NetCommand.nMpsX = nX;
	NetCommand.nMpsY = nY;
	NetCommand.nMpsZ = 0;

	KProtocol* pClient = (KProtocol*)GetPtrAddress(KPR_BASE_ADD);

	if (pClient != NULL)
		pClient->SendPackToServer(&NetCommand, sizeof(NetCommand));
}

void KProtocol::SendClientCmdSkill(int nSkillID, int nX, int nY)
{
	NPC_SKILL_COMMAND NetCommand;
	NetCommand.ProtocolType = (BYTE)c2s_npcskill;
	NetCommand.nReservered = 0;
	NetCommand.nMpsY = nY;
	NetCommand.nSkillID = nSkillID;
	NetCommand.nMpsX = nX;

	KProtocol* pClient = (KProtocol*)GetPtrAddress(KPR_BASE_ADD);

	if (pClient != NULL)
		pClient->SendPackToServer(&NetCommand, sizeof(NetCommand));
}
