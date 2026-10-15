#pragma once

enum c2s_PROTOCOL
{
	c2s_npcwalk = 75,
	c2s_npcrun,
	c2s_npcskill,
	c2s_npcjump,
	c2s_npctalk,
	c2s_npchurt,
	c2s_npcdeath,
};

#pragma	pack(1)

struct NPC_WALK_COMMAND
{
	BYTE	ProtocolType;
	int		nMpsX;
	int		nMpsY;
	int		nMpsZ;
};

struct NPC_RUN_COMMAND
{
	BYTE	ProtocolType;
	int		nMpsX;
	int		nMpsY;
	int		nMpsZ;
};

struct NPC_SKILL_COMMAND
{
	BYTE	ProtocolType;
	int		nReservered;
	int		nMpsY;
	int		nSkillID;
	int		nMpsX;
};

#pragma	pack()

class KProtocol
{
private:
	LPDWORD			__vfptr;
	void			SendPackToServer(LPCVOID pData, int nLength);

public:
	static void		SendClientCmdWalk(int nX, int nY);
	static void		SendClientCmdRun(int nX, int nY);
	static void		SendClientCmdSkill(int nSkillID, int nX, int nY);
};
