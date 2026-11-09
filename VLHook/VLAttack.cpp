#include "stdafx.h"
#include "../GameDef.h"
#include "GameBase.h"
#include "JxCore.h"
#include "S3Client.h"
#include "VLClasses.h"
#include "VLDebug.h"
#include "VLObstacle.h"
#include "TcvnText.h"

// Debug log Cuu sat / Do sat (tab Chien dau) -> <thu muc auto>\Logs\cuusat.log
// 1 = bat ghi log, 0 = tat (khong bien dich code log)
#define DebugCuuSat 1
SHORT __stdcall _VLGetKeyState(int nVirKey)
{
	if (nVirKey >= VK_F2 && nVirKey <= VK_F12)
		return (SHORT)0xFFFF;

	return _GetAsyncKeyState(nVirKey);
}

#define MAX_MAPLAG_NPC 1 
struct __MapLagNpc
{
	int nMapID;
	int nX, nY;
} MapLagNpc[MAX_MAPLAG_NPC] = { {124,51808,104064} };

// Boss xanh: game to ten quai mau xanh khi GetGoldType (0x6D4600) = m_nGoldType + 1 (can ca IsGold va IsGolding)
// tu 1 toi [GOLD_BLUE_MAX_ADD]; lon hon = boss vang (xem KNpc ve ten 0x5FDD6C)
static BOOL IsBlueBoss(KNpc *pNpc)
{
	if (!pNpc->m_cGold.m_nIsGold || !pNpc->m_cGold.m_nIsGolding) return FALSE;
	return pNpc->m_cGold.m_nGoldType + 1 <= *(int*)GOLD_BLUE_MAX_ADD;
}

//================================KAliveNpc================================
void KAliveNpc::RefreshNpcs()
{
	int nNpcX, nNpcY, dZ;
	memset(theAttack.m_AliveNpc, 0, sizeof(theAttack.m_AliveNpc));
	for (int i = 1; i < MAX_NPC; i++)
	{
		if (!g_pNpcArray[i]->IsExist()) continue;
		if (g_pNpcArray[i]->m_CurrentLife <= 0 || g_pNpcArray[i]->m_Kind != kind_normal) continue;
		if (theAttack.m_bOnlyBlueBoss == TRUE && !IsBlueBoss(g_pNpcArray[i])) continue;	// Chi danh boss xanh
		if (!DebugQuestTargetOK(g_pNpcArray[i]->Name)) continue;	// DEBUG > Nhiem vu > Buoc 4: chi danh quai nhiem vu
		if (g_pNpcArray[i]->IsGhost()) continue;	// NPC "ma": game ra skill vao cho trong (xem KNpc::IsGhost)
		KLagNpc lagNpc(g_pNpcArray[i]->m_dwID, g_pNpcArray[i]->m_CurrentLife);
		if (lagNpc == theAttack.m_BadNpc[0] || lagNpc == theAttack.m_BadNpc[1] || lagNpc == theAttack.m_BadNpc[2]) continue;
		g_pNpcArray[i]->GetMapPos(&nNpcX, &nNpcY);
		if (!theMove.m_bFollow || !theMove.m_nLeaderIdx) // Dang ko theo sau ai.
		{
			if (!theMove.IsNpcInRange(nNpcX, nNpcY)) continue;
		}
		BOOL bIgnore = 0;
		for (int j = 0; j < MAX_MAPLAG_NPC; j++)
		{
			if (MapLagNpc[j].nMapID == (*g_pMapID))
			{
				dZ = __distance(nNpcX, nNpcY, MapLagNpc[j].nX, MapLagNpc[j].nY);
				if (dZ < 64)
				{
					bIgnore = TRUE;
					break;
				}
			}
		}
		if (bIgnore) continue;
		int dRegX = g_pNpcArray[i]->m_RegX - P_PLAYER->m_RegX;
		int dRegY = g_pNpcArray[i]->m_RegY - P_PLAYER->m_RegY;
		int nRegionIndex = (dRegX + 1) * 3 + dRegY + 1;
		theAttack.m_AliveNpc[i].m_dwID = g_pNpcArray[i]->m_dwID;
		theAttack.m_AliveNpc[i].m_bIsGold = g_pNpcArray[i]->m_cGold.m_nIsGold || g_pNpcArray[i]->m_cGold.m_nIsGolding;
		theAttack.m_AliveNpc[i].m_nGoldType = g_pNpcArray[i]->m_cGold.m_nGoldType;
		theAttack.m_AliveNpc[i].m_CurLife = g_pNpcArray[i]->m_CurrentLife;
		theAttack.m_AliveNpc[i].m_nPosX = nNpcX;
		theAttack.m_AliveNpc[i].m_nPosY = nNpcY;
		theAttack.m_AliveNpc[i].m_nRegionIdx = nRegionIndex;
		theAttack.m_AliveNpc[i].m_nNpcSIdx = g_pNpcArray[i]->m_NpcSettingIdx;
	}
}

//================================Class CVLAttack================================

// Tam danh khi bat "Ap sat" (0 = khong ap sat). Nho hon 75 (vd 70) thi lay 75 - giong SetAutoHit.
// Truoc day SetActiveSkill doi tam < 75 thanh 1000 => ap sat 70 lai thanh danh tu 1000 (ra skill khi quai con xa 400+)
static int HitCloseRange(CVLAttack *p)
{
	BOOL bClose;
	int nRange;
	if (p->m_nEnemyIdx && p->m_bDefHitClose)
		bClose = TRUE, nRange = p->m_nDefSkillRange;
	else
		bClose = p->m_bHitClose, nRange = p->m_nSkillRange;
	if (!bClose) return 0;
	return (nRange < 75) ? 75 : nRange;
}

// Tiep can quai: PEOPLE_IDX = quai thi game (KNpcAI::FollowAttack) chay thang toi quai, dam vao vat can.
// Duong thang bi chan va quai ngoai tam danh: tam bo PEOPLE_IDX, tu chay bang MoveTo (ne vat can - ObsSteer),
// thay quai (duong thang thong) / vao tam danh thi gan lai PEOPLE_IDX cho game danh. Qua APPROACH_TIMEOUT thi bo quai
#define APPROACH_TIMEOUT	10000
static int		s_nApproachIdx = 0;
static DWORD	s_dwApproachID = 0;
static DWORD	s_dwApproachTick = 0;

static void ApproachReset()
{
	s_nApproachIdx = 0;
	s_dwApproachID = 0;
}

// TRUE = dang tu chay toi quai (Breathe dung lai, chua xet lag / tim quai khac)
static BOOL ApproachTarget(CVLAttack *p)
{
	int nPeople = PEOPLE_IDX;
	if (!s_nApproachIdx)
	{	// Muc tieu moi do game chay toi: kiem tra duong thang
		if (nPeople <= 0 || nPeople >= MAX_NPC) return FALSE;
		KNpc *pNpc = g_pNpcArray[nPeople];
		if (!pNpc->IsExist() || pNpc->m_Kind != kind_normal || pNpc->m_CurrentLife <= 0) return FALSE;
		int x, y, nx, ny;
		P_PLAYER->GetMapPos(&x, &y);
		pNpc->GetMapPos(&nx, &ny);
		if (__distance(x, y, nx, ny) <= P_PLAYER->m_CurrentAttackRadius + 32 || ObsLineClear(x, y, nx, ny)) return FALSE;
		s_nApproachIdx = nPeople;
		s_dwApproachID = pNpc->m_dwID;
		s_dwApproachTick = GetTickCount();
		p->SetPeopleIdx(0);
	}
	else if (nPeople && nPeople != s_nApproachIdx)
	{	// Cho khac da gan muc tieu (tu ve, theo sau...): thoi tiep can
		ApproachReset();
		return FALSE;
	}
	KNpc *pNpc = g_pNpcArray[s_nApproachIdx];
	if (!pNpc->IsExist() || pNpc->m_dwID != s_dwApproachID || pNpc->m_CurrentLife <= 0 ||
		pNpc->m_Doing == do_death || pNpc->m_Doing == do_revive)
	{
		p->SetPeopleIdx(0);
		ApproachReset();
		return FALSE;
	}
	if (GetTickCount() - s_dwApproachTick > APPROACH_TIMEOUT)
	{	// Khong toi duoc (bi vay ben kia tuong...): bo quai nay mot luc
		p->AddBadNpc(pNpc->m_dwID, pNpc->m_CurrentLife);
		p->SetPeopleIdx(0);
		ApproachReset();
		return FALSE;
	}
	int x, y, nx, ny;
	P_PLAYER->GetMapPos(&x, &y);
	pNpc->GetMapPos(&nx, &ny);
	if (__distance(x, y, nx, ny) <= P_PLAYER->m_CurrentAttackRadius || ObsLineClear(x, y, nx, ny))
	{	// Da thay quai: tra cho game danh
		p->SetPeopleIdx(s_nApproachIdx);
		ApproachReset();
		return FALSE;
	}
	PathNoteMove(nx, ny, "Toi quai");
	g_pCurPlayer->MoveTo(nx, ny);
	return TRUE;
}

CVLAttack::CVLAttack(void)
{
	m_LastNpcID = 0;
	m_bDoingSkill = 0;
	m_bDoingSkillEx = 0;
	m_bSkillChanged = TRUE;
	m_nPeopleIdx = 0;
	m_nEnemyIdx = 0;
	m_nBuaTarget = 0;
	m_dwBuaTargetID = 0;
	m_nBuaCur = 0;
	m_dwBuaTick = 0;
	m_bBuaCasting = FALSE;
	m_nBuaSavedRadius = 0;
	m_nBuaPhase = 0;
	memset(m_dwBuaDone, 0, sizeof(m_dwBuaDone));
	memset(m_dwBuaDoneTime, 0, sizeof(m_dwBuaDoneTime));
	m_nBuaDoneNext = 0;
	m_nBuaPrevPeople = 0;
	m_dwBuaPrevPeopleID = 0;
	m_bBuaOnlyPlayer = FALSE;
	m_nBuaRange = 0;
	m_dwBuaTargetStart = 0;
}

//================================Buff bua================================
// Chuyen tu BuffBua (KeoXe C#, TinSu.cs). Doc lap voi Danh quai: tu tim doi tuong (ke thu gan nhat
// chua dinh bua; quai + nguoi choi hoac chi nguoi choi), lan luot dat ActiveSkill = tung bua, het bua
// thi tra skill tay trai va tim doi tuong khac. Bat ca Danh quai: buff het doi tuong roi moi danh.
// Ban C# ghi bo nho tu ngoai (CreateRemoteThread); o day chay trong DLL, theo nhip Breathe (khong Sleep).
static BOOL SkipSameTong(KNpc* pNpc);	// dinh nghia ben duoi
#define NPC_STATESPR_OFFSET	(0x1DC8 + 0x15BC)	// KNpc::m_DataRes (KNpcRes, xem NPCP_FUNC_ADD) . m_cStateSpr[6]
#define NPC_STATESPR_SIZE	0x9C				// sizeof(KStateSpr); m_nID (id trang thai) o dau
#define NPC_STATESPR_NUM	6
#define BUA_CAST_TIMEOUT	4000				// ms/bua: qua thoi gian ma quai chua dinh (mien nhiem, het mana...) => bo qua bua do

static int PlayerSkillLevel(int nSkillId)
{
	for (int i = 0; i < MAX_NPCSKILL; i++)
		if (P_PLAYER->m_Skills[i].SkillId == nSkillId)
			return P_PLAYER->m_Skills[i].SkillLevel;
	return 0;
}

static BOOL NpcHasState(KNpc* pNpc, int nStateId)
{
	const BYTE* p = (const BYTE*)pNpc + NPC_STATESPR_OFFSET;
	for (int i = 0; i < NPC_STATESPR_NUM; i++)
		if (*(const int*)(p + i * NPC_STATESPR_SIZE) == nStateId)
			return TRUE;
	return FALSE;
}

void CVLAttack::BuffBuaStopCast()
{
	if (!m_bBuaCasting) return;
	m_bBuaCasting = FALSE;
	if (m_nBuaSavedRadius >= 75)
		P_PLAYER->m_CurrentAttackRadius = m_nBuaSavedRadius;
	SetActiveSkill(g_pCurPlayer->m_nLeftSkillID);
	m_bSkillChanged = TRUE;	// Nhip sau dat lai skill trai/phai + ngua nhu binh thuong
}

void CVLAttack::BuffBuaReset()
{
	BuffBuaStopCast();
	m_nBuaTarget = 0;
	m_dwBuaTargetID = 0;
	m_nBuaCur = 0;
	m_nBuaPhase = 0;
}

// Quai dang buff bua con song, dung con (m_dwID khong doi)
BOOL CVLAttack::BuaTargetValid()
{
	int n = m_nBuaTarget;
	if (n <= 0 || n >= MAX_NPC) return FALSE;
	KNpc* pNpc = g_pNpcArray[n];
	return pNpc->IsExist() && pNpc->m_dwID == m_dwBuaTargetID && pNpc->m_Kind == kind_normal
		&& pNpc->m_CurrentLife > 0 && pNpc->m_Doing != do_death && pNpc->m_Doing != do_revive;
}

// Bo qua bua hien tai, sang bua sau
void CVLAttack::BuaNext()
{
	m_nBuaCur++;
	m_dwBuaTick = GetTickCount();
	m_nBuaPhase = 0;
}

#define BUA_APPROACH_TIMEOUT	10000	// ms: chay toi doi tuong qua lau (ket vat can...) => bo
#define BUA_DONE_HOLD	30000	// ms: doi tuong da buff xong thi trong thoi gian nay khong buff lai

BOOL CVLAttack::BuaIsDone(DWORD dwID)
{
	if (!dwID) return FALSE;
	DWORD dwNow = GetTickCount();
	for (int i = 0; i < BUA_DONE_NUM; i++)
		if (m_dwBuaDone[i] == dwID && dwNow - m_dwBuaDoneTime[i] < BUA_DONE_HOLD) return TRUE;
	return FALSE;
}

void CVLAttack::BuaMarkDone(DWORD dwID)
{
	if (!dwID) return;
	int iSlot = -1;
	for (int i = 0; i < BUA_DONE_NUM && iSlot < 0; i++)
		if (m_dwBuaDone[i] == dwID) iSlot = i;
	if (iSlot < 0)
	{
		iSlot = m_nBuaDoneNext;
		m_nBuaDoneNext = (m_nBuaDoneNext + 1) % BUA_DONE_NUM;
	}
	m_dwBuaDone[iSlot] = dwID;
	m_dwBuaDoneTime[iSlot] = GetTickCount();
}

// Con bua nao da chon + da hoc ma doi tuong chua dinh
BOOL CVLAttack::BuaNeedCast(KNpc* pNpc)
{
	for (int k = 0; k < MAX_BUFFBUA; k++)
	{
		int nSkill = m_BuaSkills[k];
		int nState = BuffBuaStateOf(nSkill);
		if (nSkill > 0 && nState && PlayerSkillLevel(nSkill) > 0 && !NpcHasState(pNpc, nState))
			return TRUE;
	}
	return FALSE;
}

// Tim doi tuong gan nhat can buff bua: ke thu (quai + nguoi choi, hoac chi nguoi choi), trong tam tim quai,
// chua buff xong gan day, con thieu it nhat 1 bua
static int BuaSearchRange(CVLAttack* pAttack)
{
	int nRange = pAttack->m_nBuaRange;
	if (nRange < 75) nRange = pAttack->m_nSearchRange;	// Chua dat => dung tam tim quai
	if (nRange < 75) nRange = 400;
	return nRange;
}

int CVLAttack::SearchBuaTarget()
{
	int nRange = BuaSearchRange(this);
	int nSelfIdx = g_pCurPlayer->m_nIndex;
	int nPlayerX, nPlayerY, nNpcX, nNpcY;
	P_PLAYER->GetMapPos(&nPlayerX, &nPlayerY);

	int nBest = 0, dMin = nRange;
	for (int i = 1; i < MAX_NPC; i++)
	{
		if (i == nSelfIdx) continue;
		KNpc* pNpc = g_pNpcArray[i];
		if (!pNpc->IsExist()) continue;
		if (pNpc->m_Kind == kind_player)
		{
			if (pNpc->m_CurrentCamp == camp_begin || SkipSameTong(pNpc)) continue;
		}
		else if (pNpc->m_Kind != kind_normal || m_bBuaOnlyPlayer)
			continue;
		if (pNpc->m_CurrentLife <= 0 || pNpc->m_Doing == do_death || pNpc->m_Doing == do_revive) continue;
		if (pNpc->IsGhost()) continue;
		if (m_bAvoidBoss && pNpc->m_cGold.m_nGoldType == 16) continue;
		pNpc->GetMapPos(&nNpcX, &nNpcY);
		int dZ = __distance(nPlayerX, nPlayerY, nNpcX, nNpcY);
		if (dZ >= dMin) continue;
		if (BuaIsDone(pNpc->m_dwID) || !BuaNeedCast(pNpc)) continue;
		if (g_pCoreShell->GetNPCRelation(i) != relation_enemy) continue;
		dMin = dZ;
		nBest = i;
	}
	return nBest;
}

// Buff xong doi tuong hien tai: nho lai, tra skill; muc tieu danh truoc do (neu con) thi danh tiep,
// khong thi bo muc tieu => phan danh quai tu tim muc tieu moi (tat danh quai thi dung yen)
void CVLAttack::BuaFinishTarget()
{
	BuaMarkDone(m_dwBuaTargetID);
	BuffBuaStopCast();
	int nBack = 0;
	int n = m_nBuaPrevPeople;
	if (m_bAutoHit && n > 0 && n < MAX_NPC)
	{
		KNpc* pNpc = g_pNpcArray[n];
		if (pNpc->IsExist() && pNpc->m_dwID == m_dwBuaPrevPeopleID && pNpc->m_CurrentLife > 0
			&& pNpc->m_Doing != do_death && pNpc->m_Doing != do_revive)
			nBack = n;
	}
	SetPeopleIdx(nBack);
	m_nBuaTarget = 0;
	m_dwBuaTargetID = 0;
	m_nBuaCur = 0;
	m_nBuaPhase = 0;
}

BOOL CVLAttack::BuffBuaBreathe()
{
	if (!m_bBuffBua)
	{
		BuffBuaReset();
		return FALSE;
	}

	DWORD dwNow = GetTickCount();
	if (m_nBuaTarget && !BuaTargetValid())	// Doi tuong chet / mat => bo, tim con khac
	{
		int nOld = m_nBuaTarget;
		BuffBuaReset();
		if (PEOPLE_IDX == nOld) SetPeopleIdx(0);
	}
	if (!m_nBuaTarget)
	{
		int nTarget = SearchBuaTarget();
		if (!nTarget) return FALSE;	// Khong con ai can buff => phan danh quai lam viec
		BuffBuaReset();
		int nPeople = PEOPLE_IDX;
		m_nBuaPrevPeople = nPeople;
		m_dwBuaPrevPeopleID = (nPeople > 0 && nPeople < MAX_NPC) ? g_pNpcArray[nPeople]->m_dwID : 0;
		m_nBuaTarget = nTarget;
		m_dwBuaTargetID = g_pNpcArray[nTarget]->m_dwID;
		m_nBuaCur = 0;
		m_nBuaPhase = 0;
		m_dwBuaTick = dwNow;
		m_dwBuaTargetStart = dwNow;
	}
	KNpc* pNpc = g_pNpcArray[m_nBuaTarget];

	// Khoang cach toi doi tuong; chay ra ngoai 1.5 lan tam quet => bo
	int nPlayerX, nPlayerY, nNpcX, nNpcY;
	P_PLAYER->GetMapPos(&nPlayerX, &nPlayerY);
	pNpc->GetMapPos(&nNpcX, &nNpcY);
	int nDist = __distance(nPlayerX, nPlayerY, nNpcX, nNpcY);
	if (nDist > BuaSearchRange(this) * 3 / 2)
	{
		BuaFinishTarget();
		return TRUE;
	}

	// Bua hien tai da ra chieu xong => sang bua sau, KHONG doi trang thai hien tren doi tuong:
	// de ActiveSkill = bua dang hoi chieu thi game (KNpc::DoSkill, CanCast sai) xoa m_nPeopleIdx.
	if (m_bBuaCasting && m_nBuaCur < MAX_BUFFBUA)
	{
		// Phai thay ranh -> ra chieu -> ranh (tranh tinh nham dong tac danh thuong dang do)
		BOOL bInCast = (P_PLAYER->m_Doing == do_skill || P_PLAYER->m_Doing == do_magic);
		if (!bInCast)
		{
			if (m_nBuaPhase == 2) BuaNext();
			else m_nBuaPhase = 1;
		}
		else if (m_nBuaPhase == 1)
			m_nBuaPhase = 2;
	}

	// Tim bua tiep theo: da chon, da hoc, doi tuong chua dinh va chua qua thoi gian thu
	int nSkill = 0;
	while (m_nBuaCur < MAX_BUFFBUA)
	{
		nSkill = m_BuaSkills[m_nBuaCur];
		int nState = BuffBuaStateOf(nSkill);
		if (nSkill > 0 && nState && PlayerSkillLevel(nSkill) > 0
			&& !NpcHasState(pNpc, nState) && dwNow - m_dwBuaTick < BUA_CAST_TIMEOUT)
			break;
		BuaNext();
	}
	if (m_nBuaCur >= MAX_BUFFBUA)	// Da buff xong doi tuong nay; nhip sau tim doi tuong khac
	{
		BuaFinishTarget();
		return TRUE;
	}

	// Ra lenh cast: skill -> tam -> muc tieu (game KNpcAI::FollowAttack tu chay toi va ra chieu)
	if (!m_bBuaCasting)
	{
		m_nBuaSavedRadius = P_PLAYER->m_CurrentAttackRadius;
		m_bBuaCasting = TRUE;
	}
	P_PLAYER->m_ActiveSkillID = nSkill;
	int nRange = HitCloseRange(this);
	if (!nRange)
	{
		KSkill* pSkill = (KSkill*)(SKL_BASE_ADD + (nSkill - 1) * SKL_DATA_SIZE);
		nRange = (pSkill->m_dwSkillID == (DWORD)nSkill) ? pSkill->m_nSkillRange : 0;
		if (nRange < 75) nRange = 400;	// mac dinh giong ban C#
	}
	P_PLAYER->m_CurrentAttackRadius = nRange;
	// Ngoai tam skill: game (KNpcAI::FollowAttack) tu chay toi doi tuong roi moi ra chieu.
	// Luc dang chay chua tinh thoi gian thu bua; chay qua lau (ket vat can) thi bo doi tuong.
	if (nDist > nRange)
	{
		m_dwBuaTick = dwNow;
		if (dwNow - m_dwBuaTargetStart > BUA_APPROACH_TIMEOUT)
		{
			BuaFinishTarget();
			return TRUE;
		}
	}
	if (PEOPLE_IDX != m_nBuaTarget)
		SetPeopleIdx(m_nBuaTarget);
	return TRUE;
}

// Nguoi choi cung bang hoi voi minh: so ma bang KNpc+0x1CCC (game cung so the nay trong GetRelation 0x636C6E)
BOOL IsSameTong(KNpc* pNpc)	// VLDebugAttack dung chung
{
	DWORD dwTong = P_PLAYER->m_dwTongNameID;
	return dwTong && pNpc->m_dwTongNameID == dwTong;
}

// Bo qua nguoi cung bang khi tu ve (tru khi bat "Doi thu cung bang hoi")
static BOOL SkipSameTong(KNpc* pNpc)
{
	return !theAttack.m_bTongEnemy && IsSameTong(pNpc);
}

// Co nguoi choi thu dich trong tam tu ve khong (cung dieu kien loc ke thu voi AutoDefence)
static BOOL EnemyInDefenceRange(int nRange)
{
	if (!P_PLAYER->m_FightMode || P_PLAYER->m_CurrentCamp == camp_begin) return FALSE;
	if (theQuest.m_bAutoDoQuest && theQuest.m_bGoTrainGetPK && theQuest.m_TongPKOpt.m_bKillOther) return FALSE;	// Che do 5 (giet PK nhiem vu)

	int nPlayerX, nPlayerY, nNpcX, nNpcY;
	int nSelfIdx = g_pCurPlayer->m_nIndex;
	P_PLAYER->GetMapPos(&nPlayerX, &nPlayerY);
	for (int i = 1; i < MAX_NPC; i++)
	{
		if (i == nSelfIdx) continue;
		if (!g_pNpcArray[i]->IsExist()) continue;
		if (g_pNpcArray[i]->m_CurrentLife <= 0 || g_pNpcArray[i]->m_Kind != kind_player) continue;
		if (g_pNpcArray[i]->IsGhost()) continue;
		if (g_pNpcArray[i]->m_CurrentCamp == camp_begin) continue;
		if (SkipSameTong(g_pNpcArray[i])) continue;
		int nRelation = g_pCoreShell->GetNPCRelation(i);
		if (nRelation == relation_ally) continue;
		if (nRelation == relation_none)
		{
			if (P_PLAYER->m_CurrentCamp == g_pNpcArray[i]->m_CurrentCamp && P_PLAYER->m_CurrentCamp < camp_free) continue;
			if (g_pNpcArray[i]->m_nPKFlag != 2) continue;
		}
		g_pNpcArray[i]->GetMapPos(&nNpcX, &nNpcY);
		if (__distance(nPlayerX, nPlayerY, nNpcX, nNpcY) < nRange) return TRUE;
	}
	return FALSE;
}

// ===== Do sat / Cuu sat (cua so Cai dat Do Sat - VLAuto) =====
// Chi doi trang thai PK / gui cuu sat; danh tra hay khong van do Tu ve quyet dinh.
// Bat tu khi nhan vat o map m_nPKMapID va da toi diem dau tien cua danh sach toa do train
// (hoac toa do danh quanh diem); doi map thi tat, toi lai diem dau moi bat lai.
#define PK_ARM_NEAR			400		// Cach diem dau bao nhieu thi coi la da toi (giong NearTrainArea)
#define PK_CHECK_DELAY		1000	// Nhip kiem tra
#define PK_SWITCH_DELAY		3000	// Toi thieu giua 2 lan Switch('PK',x)
#define PK_LIST_HOLD		10000	// Do sat theo DS: ke thu ra khoi tam bao lau moi chuyen ve chien dau
#define PK_REVENGE_DELAY	10000	// Cuu sat cung 1 nguoi: gui lai sau
#define PK_ZERO_TRIES		3		// Do sat theo DS: ve Luyen cong that bai bao nhieu lan thi ve Chien dau

static BOOL		s_bPKArmed = FALSE;
static int		s_nPKArmMap = 0;
static DWORD	s_dwPKCheckTick = 0;
static DWORD	s_dwPKSwitchTick = 0;
static DWORD	s_dwPKSeenTick = 0;
static DWORD	s_dwRevengeID = 0;
static DWORD	s_dwRevengeTick = 0;
static int		s_nPKZeroTry = 0;

BOOL CVLAttack::IsListedEnemy(const char* szName)
{
	if (!szName || !szName[0]) return FALSE;
	for (int i = 0; i < MAX_ENEMY && m_szEnemies[i][0]; i++)
		if (!strncmp(m_szEnemies[i], szName, 31)) return TRUE;
	return FALSE;
}

BOOL CVLAttack::PKSwitchActive()
{
	if (m_nPKMode < 1 || m_nPKMode > 3 || !m_nPKMapID || *g_pMapID != m_nPKMapID) return FALSE;
	return s_bPKArmed && s_nPKArmMap == m_nPKMapID;
}

// Diem dau tien cua bai train: toa do danh quanh diem, khong co thi diem dau danh sach toa do train.
// FALSE = khong co diem nao (vao toi map la bat)
static BOOL GetPKArmPoint(int* pX, int* pY)
{
	if ((theMove.m_bAroundPoint || theMove.m_bAroundPointPK) && (theMove.m_nPointX || theMove.m_nPointY))
	{
		*pX = theMove.m_nPointX; *pY = theMove.m_nPointY;
		return TRUE;
	}
	if (theMove.m_bRouteMove && theMove.m_Nodes[0].x && theMove.m_Nodes[0].y &&
		(!theMove.m_nNodesMapID || theMove.m_nNodesMapID == *g_pMapID))
	{
		*pX = theMove.m_Nodes[0].x; *pY = theMove.m_Nodes[0].y;
		return TRUE;
	}
	return FALSE;
}

// ===== Debug log Cuu sat / Do sat =====
#if DebugCuuSat
#define CS_LOG_MAXSIZE		(8 * 1024 * 1024)	// Qua 8MB => doi ten sang cuusat_bak.log, mo file moi
#define CS_LOG_HEARTBEAT	30000				// Ghi tom tat trang thai moi 30s (ke ca khi khong doi)
#define CS_BUF_SIZE			(64 * 1024)			// Bo dem log trong RAM
#define CS_BUF_FLUSH		(32 * 1024)			// Bo dem vuot nguong nay => ghi ra file 1 lan
#define CS_FLUSH_TIME		30000				// Hoac da 30s chua ghi => ghi (de file khong bi cu qua lau)
#define CS_SCAN_RANGE		1000				// Quet nguoi choi trong pham vi nay, ghi ly do hanh dong

static char		s_szCSLastState[256] = "";	// Ly do / trang thai lan truoc (chi ghi khi doi)
static DWORD	s_dwCSHeartbeat = 0;
static DWORD	s_dwCSLastBreathe = 0;		// Lan cuoi PKSwitchBreathe duoc goi
static int		s_nCSLastPKFlag = -1;
static int		s_nCSReqFlag = -1;			// Lenh Switch('PK',x) gan nhat do auto gui
static DWORD	s_dwCSReqTick = 0;

static char		s_CSBuf[CS_BUF_SIZE];
static int		s_nCSBufLen = 0;
static DWORD	s_dwCSFlushTick = 0;

// Ghi ca bo dem ra file 1 lan (mo / dong file 1 lan thay vi moi dong)
static void CSFlush()
{
	s_dwCSFlushTick = GetTickCount();
	if (!s_nCSBufLen) return;
	char szFile[MAX_PATH], szDir[MAX_PATH];
	sprintf(szDir, "%s\\Logs", theMain.m_szCurFolder);
	CreateDirectoryA(szDir, NULL);
	sprintf(szFile, "%s\\cuusat.log", szDir);

	BOOL bNew = FALSE;
	WIN32_FILE_ATTRIBUTE_DATA fa;
	if (!GetFileAttributesExA(szFile, GetFileExInfoStandard, &fa))
		bNew = TRUE;
	else if (fa.nFileSizeLow > CS_LOG_MAXSIZE || fa.nFileSizeHigh)
	{
		char szBak[MAX_PATH];
		sprintf(szBak, "%s\\cuusat_bak.log", szDir);
		DeleteFileA(szBak);
		MoveFileA(szFile, szBak);
		bNew = TRUE;
	}
	FILE* f = fopen(szFile, "ab");
	if (!f) return;	// Khong mo duoc (file dang bi khoa...): giu bo dem, lan sau ghi tiep
	if (bNew) fputs("\xEF\xBB\xBF", f);	// BOM UTF-8
	fwrite(s_CSBuf, 1, s_nCSBufLen, f);
	fclose(f);
	s_nCSBufLen = 0;
}

// Them 1 dong (TCVN3) vao bo dem (doi sang UTF-8); vuot nguong / qua thoi gian thi ghi file
static void CSAppend(const char* szTcvnLine)
{
	char szUtf8[4200];
	int nLen = Tcvn3ToUtf8(szTcvnLine, szUtf8, sizeof(szUtf8) - 3);
	if (nLen < 0) nLen = 0;
	szUtf8[nLen++] = '\r'; szUtf8[nLen++] = '\n'; szUtf8[nLen] = 0;
	if (s_nCSBufLen + nLen > CS_BUF_SIZE) CSFlush();
	if (s_nCSBufLen + nLen > CS_BUF_SIZE) s_nCSBufLen = 0;	// File bi khoa lau: bo bo dem cu, khong tran
	memcpy(s_CSBuf + s_nCSBufLen, szUtf8, nLen);
	s_nCSBufLen += nLen;
	if (s_nCSBufLen >= CS_BUF_FLUSH || GetTickCount() - s_dwCSFlushTick >= CS_FLUSH_TIME)
		CSFlush();
}

// Ghi 1 dong: [ngay gio.ms] [ten nhan vat] noi dung. Chuoi TCVN3 (ten) -> UTF-8.
static void CSLog(const char* szFormat, ...)
{
	char szMsg[1024];
	va_list vl;
	va_start(vl, szFormat);
	_vsnprintf(szMsg, sizeof(szMsg) - 1, szFormat, vl);
	va_end(vl);
	szMsg[sizeof(szMsg) - 1] = 0;

	SYSTEMTIME st; GetLocalTime(&st);
	char szName[64] = "?";
	if (g_pCurPlayer && P_PLAYER) { strncpy(szName, P_PLAYER->Name, sizeof(szName) - 1); szName[sizeof(szName) - 1] = 0; }
	char szLine[1400];
	_snprintf(szLine, sizeof(szLine) - 1, "[%02d-%02d %02d:%02d:%02d.%03d] [%s] %s",
		st.wDay, st.wMonth, st.wHour, st.wMinute, st.wSecond, st.wMilliseconds, szName, szMsg);
	szLine[sizeof(szLine) - 1] = 0;
	CSAppend(szLine);
}

// Ghi ly do / trang thai chi khi khac lan truoc (tranh 1 dong moi giay)
static void CSState(const char* szFormat, ...)
{
	char szMsg[256];
	va_list vl;
	va_start(vl, szFormat);
	_vsnprintf(szMsg, sizeof(szMsg) - 1, szFormat, vl);
	va_end(vl);
	szMsg[sizeof(szMsg) - 1] = 0;
	if (!strcmp(szMsg, s_szCSLastState)) return;
	strcpy(s_szCSLastState, szMsg);
	CSLog("TRANG THAI: %s", szMsg);
}
#define CS_LOG(...)		CSLog(__VA_ARGS__)
#define CS_STATE(...)	CSState(__VA_ARGS__)
#else
#define CS_LOG(...)		((void)0)
#define CS_STATE(...)	((void)0)
#endif

static void SwitchPK(int nFlag)
{
	if (P_PLAYER->m_nPKFlag == nFlag) return;
	DWORD dwNow = GetTickCount();
	if (s_dwPKSwitchTick && dwNow - s_dwPKSwitchTick < PK_SWITCH_DELAY) return;
	s_dwPKSwitchTick = dwNow;
	CS_LOG("GUI Switch('PK',%d)  (PK hien tai %d)", nFlag, P_PLAYER->m_nPKFlag);
#if DebugCuuSat
	s_nCSReqFlag = nFlag;
	s_dwCSReqTick = dwNow;
#endif
	ExecuteScript(nFlag == 2 ? "Switch('PK',2)" : (nFlag == 1 ? "Switch('PK',1)" : "Switch('PK',0)"));
}

#if DebugCuuSat
// Goi moi nhip PKSwitchBreathe (truoc moi return): theo doi PKFlag doi, Breathe bi bo nhip, gui lenh ma server khong doi
static void CSWatch()
{
	DWORD dwNow = GetTickCount();
	if (s_dwCSLastBreathe && dwNow - s_dwCSLastBreathe > 3000)
		CS_LOG("CANH BAO: PKSwitchBreathe khong chay %u ms (Breathe bi dung: SysPause / ExpLimit / gom tien bang / ve thanh luu ruong / mat mang...)",
			dwNow - s_dwCSLastBreathe);
	s_dwCSLastBreathe = dwNow;

	int nFlag = P_PLAYER->m_nPKFlag;
	if (nFlag != s_nCSLastPKFlag)
	{
		if (s_nCSLastPKFlag >= 0)
		{
			if (s_nCSReqFlag == nFlag && dwNow - s_dwCSReqTick < 10000)
				CS_LOG("PKFlag %d -> %d (theo lenh auto, sau %u ms)", s_nCSLastPKFlag, nFlag, dwNow - s_dwCSReqTick);
			else
				CS_LOG("PKFlag %d -> %d (KHONG phai do Do sat/Cuu sat: nguoi choi tu doi / Tu ve / ve thanh / server)", s_nCSLastPKFlag, nFlag);
		}
		s_nCSLastPKFlag = nFlag;
		if (s_nCSReqFlag == nFlag) s_nCSReqFlag = -1;
	}
	else if (s_nCSReqFlag >= 0 && dwNow - s_dwCSReqTick > 2500)
	{
		CS_LOG("SERVER KHONG DOI PK: da gui Switch('PK',%d) %u ms truoc, PK van la %d", s_nCSReqFlag, dwNow - s_dwCSReqTick, nFlag);
		s_nCSReqFlag = -1;
	}
}
#endif

// Goi tu VLHook.cpp khi nhan tham so tab Chien dau (WM_HOOK_ATTACK)
void CuuSatLogParams()
{
#if DebugCuuSat
	CVLAttack &a = theAttack;
	char szList[700] = "";
	int nCount = 0;
	for (int i = 0; i < MAX_ENEMY && a.m_szEnemies[i][0]; i++, nCount++)
	{
		if (strlen(szList) + strlen(a.m_szEnemies[i]) + 4 >= sizeof(szList)) { strcat(szList, " ..."); break; }
		if (i) strcat(szList, ", ");
		strcat(szList, a.m_szEnemies[i]);
	}
	CS_LOG("NHAN THAM SO: che do %d  MapPK %d (map hien tai %d)  Tu ve %d  PhamVi %d  AlwaysF9 %d  DS %d ten: %s",
		a.m_nPKMode, a.m_nPKMapID, *g_pMapID, a.m_nAutoDefence, a.m_nDefenceRange, a.m_bAlwaysF9, nCount, szList);
	s_szCSLastState[0] = 0;	// Ghi lai trang thai ngay nhip sau
#endif
}

#if DebugCuuSat
// Quet moi nguoi choi trong CS_SCAN_RANGE, ghi: ten-Index-khoang cach-ke thu|bo qua-ly do.
// Chi ghi khi ket qua khac lan truoc (khoang cach lam tron 100).
static void CSScanPlayers(CVLAttack* a, int nPlayerX, int nPlayerY, BOOL bArmed)
{
	static DWORD s_dwLastHash = 0;
	const char* szEnemy = TV(L"kẻ thù");
	const char* szSkip = TV(L"bỏ qua");
	int nRange = a->m_nDefenceRange;
	int nMode = a->m_nPKMode;
	int nSelfIdx = g_pCurPlayer->m_nIndex;

	struct { int nIdx, dZ; BOOL bEnemy; const char* szWhy; } aList[64];
	int nCount = 0;
	DWORD dwHash = 2166136261u ^ (nMode * 31 + bArmed * 7 + P_PLAYER->m_nPKFlag * 3 + a->m_nAutoDefence + nRange * 131);
	for (int i = 1; i < MAX_NPC && nCount < 64; i++)
	{
		if (i == nSelfIdx) continue;
		KNpc* pNpc = g_pNpcArray[i];
		if (!pNpc->IsExist() || pNpc->m_Kind != kind_player) continue;
		int nX, nY;
		pNpc->GetMapPos(&nX, &nY);
		int dZ = __distance(nPlayerX, nPlayerY, nX, nY);
		if (dZ > CS_SCAN_RANGE) continue;

		BOOL bEnemy = FALSE;
		const char* szWhy;
		BOOL bListed = (nMode == 2 || nMode == 3) && a->IsListedEnemy(pNpc->Name);
		int nRel = g_pCoreShell->GetNPCRelation(i);
		if (pNpc->m_CurrentLife <= 0) szWhy = TV(L"đã chết");
		else if (pNpc->IsGhost()) szWhy = TV(L"hồn ma");
		else if (pNpc->m_CurrentCamp == camp_begin) szWhy = TV(L"tân thủ");
		else if (bListed)
		{
			if (dZ > nRange) szWhy = TV(L"có trong danh sách nhưng ngoài tầm");
			else if (!bArmed) szWhy = TV(L"có trong danh sách nhưng đồ sát/cừu sát chưa bật");
			else if (nMode == 3 && nRel == relation_enemy) { bEnemy = TRUE; szWhy = TV(L"cừu sát - đã là kẻ thù"); }
			else { bEnemy = TRUE; szWhy = nMode == 2 ? TV(L"có trong danh sách đồ sát") : TV(L"có trong danh sách cừu sát"); }
		}
		else if (!a->m_nAutoDefence)
			szWhy = nMode == 2 || nMode == 3 ? TV(L"không có trong danh sách đồ sát (tự vệ tắt)") : TV(L"tự vệ tắt");
		else if (SkipSameTong(pNpc)) szWhy = TV(L"cùng bang");
		else if (nRel == relation_ally) szWhy = TV(L"đồng minh");
		else if (nRel == relation_none && P_PLAYER->m_CurrentCamp == pNpc->m_CurrentCamp && P_PLAYER->m_CurrentCamp < camp_free)
			szWhy = TV(L"cùng phe");
		else if (nRel == relation_none && pNpc->m_nPKFlag != 2) szWhy = TV(L"không bật đồ sát");
		else if (dZ >= nRange) szWhy = TV(L"không trong tầm tự vệ");
		else { bEnemy = TRUE; szWhy = TV(L"tự vệ"); }

		aList[nCount].nIdx = i; aList[nCount].dZ = dZ; aList[nCount].bEnemy = bEnemy; aList[nCount].szWhy = szWhy;
		nCount++;
		dwHash = (dwHash ^ (DWORD)(i * 7919 + dZ / 100 * 13 + bEnemy)) * 16777619u;
		dwHash = (dwHash ^ (DWORD)(size_t)szWhy) * 16777619u;
	}
	if (dwHash == s_dwLastHash) return;
	s_dwLastHash = dwHash;

	CS_LOG("QUET %d: che do %d  armed %d  PK %d  Tu ve %d  tam %d  => %d nguoi", CS_SCAN_RANGE, nMode, bArmed,
		P_PLAYER->m_nPKFlag, a->m_nAutoDefence, nRange, nCount);
	for (int k = 0; k < nCount; k++)
		CS_LOG("  %s-%d-%d-%s-%s", g_pNpcArray[aList[k].nIdx]->Name, aList[k].nIdx, aList[k].dZ,
			aList[k].bEnemy ? szEnemy : szSkip, aList[k].szWhy);
}
#endif

// Tam quet bat Do sat theo DS / Cuu sat = "Pham vi tim ke thu" (IDC_DEFENCERANGE_EDIT) tren tab Chien dau.
// Khong phu thuoc che do Tu ve (Bo qua / Danh tra / ...) - co the vua Do sat vua Danh tra.
static int PKSkillRange(CVLAttack* p)
{
	return p->m_nDefenceRange;
}

void CVLAttack::PKSwitchBreathe(BOOL bFindingTrain)
{
	int nMap = *g_pMapID;
	if (m_nPKMode < 1 || m_nPKMode > 3 || !m_nPKMapID || nMap != m_nPKMapID)
	{
		if (s_bPKArmed)
			CS_LOG("TAT (disarm): che do %d  MapPK %d  map hien tai %d", m_nPKMode, m_nPKMapID, nMap);
#if DebugCuuSat
		if (m_nPKMode >= 1 && m_nPKMode <= 3)
		{
			if (!m_nPKMapID) CS_STATE("Chua chon map PK (MapPK = 0) => khong chay");
			else CS_STATE("Sai map: dang o map %d, map cai dat %d => khong chay", nMap, m_nPKMapID);
		}
		else
			CS_STATE("Che do = %d (tat)", m_nPKMode);
		s_dwCSLastBreathe = 0;
#endif
		s_bPKArmed = FALSE;
		s_nPKArmMap = 0;
		return;
	}
	DWORD dwNow = GetTickCount();
	if (dwNow - s_dwPKCheckTick < PK_CHECK_DELAY) return;
	s_dwPKCheckTick = dwNow;
#if DebugCuuSat
	CSWatch();
#endif

	if (theMain.m_bPause) { CS_STATE("Auto dang tam dung (m_bPause) => khong chay"); return; }
	if (!P_PLAYER->m_FightMode) { CS_STATE("Dang trong thanh (FightMode 0) => khong chay"); return; }
	if (P_PLAYER->m_CurrentCamp == camp_begin) { CS_STATE("Nhan vat la tan thu (camp_begin) => khong chay"); return; }
	if (P_PLAYER->m_CurrentLife <= 0) { CS_STATE("Nhan vat da chet => khong chay"); return; }

	int nPlayerX, nPlayerY;
	P_PLAYER->GetMapPos(&nPlayerX, &nPlayerY);
#if DebugCuuSat
	CSScanPlayers(this, nPlayerX, nPlayerY, s_bPKArmed && s_nPKArmMap == nMap);
#endif
	if (!s_bPKArmed || s_nPKArmMap != nMap)
	{
		if (bFindingTrain)
		{	// Dang tu di toi bai train
			CS_STATE("Chua bat: dang tu di toi bai train (GoTrain %d / MapPath) - cho toi noi", theMove.m_bGoTrain);
			return;
		}
		int nX, nY;
		if (GetPKArmPoint(&nX, &nY) && __distance(nPlayerX, nPlayerY, nX, nY) > PK_ARM_NEAR)
		{
			CS_STATE("Chua bat: cach diem bat (%d,%d) %d > %d (minh o %d,%d)", nX, nY,
				__distance(nPlayerX, nPlayerY, nX, nY) / 100 * 100, PK_ARM_NEAR, nPlayerX / 100 * 100, nPlayerY / 100 * 100);
			return;
		}
		s_bPKArmed = TRUE;
		s_nPKArmMap = nMap;
		s_dwPKSeenTick = 0;
		s_dwRevengeID = 0;
		s_nPKZeroTry = 0;
		CS_LOG("BAT (armed): che do %d  map %d  vi tri (%d,%d)  PK %d  PhamVi %d", m_nPKMode, nMap, nPlayerX, nPlayerY,
			P_PLAYER->m_nPKFlag, PKSkillRange(this));
	}

	if (m_nPKMode == 1)
	{	// Do sat tat ca
		CS_STATE("Do sat tat ca: giu PK 2");
		SwitchPK(2);
		return;
	}

	// Ke thu trong danh sach gan nhat trong tam ra skill
	int nRange = PKSkillRange(this);
	int nSelfIdx = g_pCurPlayer->m_nIndex;
	int nFound = 0, dMin = MAX_INTNUM;
#if DebugCuuSat
	int nListedSeen = 0, nListedOut = 0, nListedSkip = 0;	// Nguoi trong DS: thay / ngoai tam / bi bo qua (chet, tan thu...)
	char szNear[160] = "";
#endif
	for (int i = 1; i < MAX_NPC; i++)
	{
		if (i == nSelfIdx) continue;
		KNpc* pNpc = g_pNpcArray[i];
		if (!pNpc->IsExist() || pNpc->m_Kind != kind_player) continue;
		if (!IsListedEnemy(pNpc->Name)) continue;
#if DebugCuuSat
		nListedSeen++;
#endif
		if (pNpc->m_CurrentLife <= 0 || pNpc->IsGhost() || pNpc->m_CurrentCamp == camp_begin)
		{
#if DebugCuuSat
			nListedSkip++;
#endif
			continue;
		}
		int nX, nY;
		pNpc->GetMapPos(&nX, &nY);
		int dZ = __distance(nPlayerX, nPlayerY, nX, nY);
#if DebugCuuSat
		if (dZ > nRange)
		{
			nListedOut++;
			if (!szNear[0]) _snprintf(szNear, sizeof(szNear) - 1, "%s cach %d", pNpc->Name, dZ);
		}
#endif
		if (dZ <= nRange && dZ < dMin) { dMin = dZ; nFound = i; }
	}
#if DebugCuuSat
	if (nFound)
		CS_STATE("Che do %d: THAY %s TRONG TAM (tam %d)  Rel %d  PK nguoi do %d  PK minh %d", m_nPKMode, g_pNpcArray[nFound]->Name,
			nRange, g_pCoreShell->GetNPCRelation(nFound), g_pNpcArray[nFound]->m_nPKFlag, P_PLAYER->m_nPKFlag);
	else if (nListedOut)
		CS_STATE("Che do %d: co %d nguoi trong DS nhung NGOAI TAM %d", m_nPKMode, nListedOut, nRange);
	static DWORD s_dwCSNearTick = 0;
	if (!nFound && nListedOut && dwNow - s_dwCSNearTick >= 5000)
	{	// Nguoi trong DS dang o gan nhung ngoai tam: ghi khoang cach toi da 5s/lan
		s_dwCSNearTick = dwNow;
		CS_LOG("  DS ngoai tam: %s (tam %d)", szNear, nRange);
	}
	else if (nListedSkip)
		CS_STATE("Che do %d: nguoi trong DS chet / tan thu / hon ma => bo qua", m_nPKMode);
	else
		CS_STATE("Che do %d: khong thay ai trong DS quanh day (tam %d)", m_nPKMode, nRange);
	if (dwNow - s_dwCSHeartbeat >= CS_LOG_HEARTBEAT)
	{
		s_dwCSHeartbeat = dwNow;
		CS_LOG("NHIP: che do %d  armed  PK %d  vi tri (%d,%d)  DS thay %d / ngoai tam %d / bo qua %d  ZeroTry %d",
			m_nPKMode, P_PLAYER->m_nPKFlag, nPlayerX, nPlayerY, nListedSeen, nListedOut, nListedSkip, s_nPKZeroTry);
	}
#endif

	if (m_nPKMode == 2)
	{	// Do sat theo danh sach
		if (nFound)
		{
			s_dwPKSeenTick = dwNow;
			s_nPKZeroTry = 0;
			SwitchPK(2);
		}
		else if (!s_dwPKSeenTick || dwNow - s_dwPKSeenTick >= PK_LIST_HOLD)
		{
			// Het ke thu: ve Luyen cong; server tu choi (vua do sat) => sau PK_ZERO_TRIES lan van la 2 thi ve Chien dau.
			// (Ban cu goi SwitchPK(0) roi SwitchPK(1) lien nhau => lan 2 luon bi chan 3s => khong bao gio ve 1)
			if (!P_PLAYER->m_nPKFlag)
				s_nPKZeroTry = 0;
			else if (P_PLAYER->m_nPKFlag == 2 && s_nPKZeroTry >= PK_ZERO_TRIES)
			{
				CS_STATE("Do sat DS: het ke thu, ve Luyen cong that bai %d lan => ve Chien dau", s_nPKZeroTry);
				SwitchPK(1);
			}
			else if (P_PLAYER->m_nPKFlag == 2)
			{
				DWORD dwLast = s_dwPKSwitchTick;
				SwitchPK(0);
				if (s_dwPKSwitchTick != dwLast) s_nPKZeroTry++;	// Da thuc su gui lenh
			}
			else
				SwitchPK(0);	// Dang Chien dau => thu ve Luyen cong
		}
		return;
	}

	// Cuu sat theo danh sach: khong can Chien dau hay Do sat => uu tien dua PK ve Luyen cong
	// (khong chan: server KPlayerPK::EnmityPOpen khong doi PK 0, nen cuu sat van gui ngay ca khi chua ve duoc).
	// Tu ve se khong tu bat PK 1 khi dang Cuu sat (xem AutoDefence).
	if (P_PLAYER->m_nPKFlag) SwitchPK(0);
	if (!nFound) return;
	KNpc* pEnemy = g_pNpcArray[nFound];
	if (g_pCoreShell->GetNPCRelation(nFound) == relation_enemy)
	{	// Dang la ke thu roi (da cuu sat / do sat)
#if DebugCuuSat
		if (s_dwRevengeID == pEnemy->m_dwID && s_dwRevengeTick)
		{
			CS_LOG("CUU SAT THANH CONG: %s da la ke thu (sau %u ms)", pEnemy->Name, dwNow - s_dwRevengeTick);
			s_dwRevengeTick = 0;
		}
#endif
		return;
	}
	if (s_dwRevengeID == pEnemy->m_dwID && dwNow - s_dwRevengeTick < PK_REVENGE_DELAY) return;
	if (s_dwRevengeID == pEnemy->m_dwID && s_dwRevengeTick)
		CS_LOG("CUU SAT CHUA AN: gui %s %u ms truoc ma van chua la ke thu (Rel %d) => gui lai", pEnemy->Name,
			dwNow - s_dwRevengeTick, g_pCoreShell->GetNPCRelation(nFound));
	CS_LOG("GUI CUU SAT: %s  ID %u  idx %d  cach %d  Rel %d  PK minh %d  PK nguoi do %d", pEnemy->Name, pEnemy->m_dwID, nFound,
		dMin, g_pCoreShell->GetNPCRelation(nFound), P_PLAYER->m_nPKFlag, pEnemy->m_nPKFlag);
	s_dwRevengeID = pEnemy->m_dwID;
	s_dwRevengeTick = dwNow;
	KUiPlayerItem thePlayer;
	memset(&thePlayer, 0, sizeof(KUiPlayerItem));
	strncpy(thePlayer.Name, pEnemy->Name, sizeof(thePlayer.Name) - 1);
	thePlayer.uId = pEnemy->m_dwID;
	thePlayer.nIndex = nFound;
	thePlayer.nData = 1;
	ProcessPeople(&thePlayer, ACTION_REVENGE);
}

// Debug (VLDebugAttack PlayerList): trang thai Do sat / Cuu sat
void DbgOut(const char *szFormat, ...);	// VLDebug.cpp
void PKSwitchDump()
{
	CVLAttack &a = theAttack;
	static const char* s_szMode[] = { "Tat", "Do sat tat ca", "Do sat theo DS", "Cuu sat theo DS" };
	int nMode = (a.m_nPKMode >= 0 && a.m_nPKMode <= 3) ? a.m_nPKMode : 0;
	DbgOut("--- Do sat / Cuu sat: che do %d (%s)  MapPK %d  Map hien tai %d  Armed %d (map %d)  FightMode %d\n",
		a.m_nPKMode, s_szMode[nMode], a.m_nPKMapID, *g_pMapID, s_bPKArmed, s_nPKArmMap, P_PLAYER->m_FightMode);
	if (a.m_nPKMode < 1) return;
	int nPlayerX, nPlayerY, nX, nY;
	P_PLAYER->GetMapPos(&nPlayerX, &nPlayerY);
	if (GetPKArmPoint(&nX, &nY))
		DbgOut("    Diem bat: (%d,%d)  cach %d (can <= %d)\n", nX, nY, __distance(nPlayerX, nPlayerY, nX, nY), PK_ARM_NEAR);
	else
		DbgOut("    Diem bat: khong co (toi map la bat)\n");
	int nRange = PKSkillRange(&a);
	DbgOut("    Tam bat PK %d  PKFlag %d  ZeroTry %d  Pause %d\n", nRange, P_PLAYER->m_nPKFlag, s_nPKZeroTry, theMain.m_bPause);
	int nListed = 0;
	for (int i = 0; i < MAX_ENEMY && a.m_szEnemies[i][0]; i++) nListed++;
	DbgOut("    Danh sach ke thu: %d ten\n", nListed);
	for (int i = 1; i < MAX_NPC; i++)
	{
		KNpc* pNpc = g_pNpcArray[i];
		if (i == g_pCurPlayer->m_nIndex || !pNpc->IsExist() || pNpc->m_Kind != kind_player) continue;
		if (!a.IsListedEnemy(pNpc->Name)) continue;
		pNpc->GetMapPos(&nX, &nY);
		int dZ = __distance(nPlayerX, nPlayerY, nX, nY);
		DbgOut("    [DS] %s  dZ %d %s  Rel %d  PK %d  Life %d\n", pNpc->Name, dZ, dZ <= nRange ? "TRONG TAM" : "ngoai tam",
			g_pCoreShell->GetNPCRelation(i), pNpc->m_nPKFlag, pNpc->m_CurrentLife);
	}
}

void CVLAttack::Breathe()
{
	int nX, nY, nRange;

	if (g_NetworkOffline) return;

	DebugAttackWatch();	// Cua so DEBUG > Chien dau: Kiem tra doi tuong dang danh
	// Buff bua dang cast ma doi muc tieu / tat chuc nang / tat danh quai => tra skill tay trai ngay
	// (cac nhanh return ben duoi co the khong toi BuffBuaBreathe)
	if (m_bBuaCasting)
	{
		if (m_bBuffBua && P_PLAYER->m_FightMode && !m_nEnemyIdx && PEOPLE_IDX == 0 && BuaTargetValid())
		{	// Game xoa muc tieu vi khong ra duoc bua (dang hoi chieu / thieu mana...) => giu doi tuong, bo bua nay
			SetPeopleIdx(m_nBuaTarget);
			BuaNext();
		}
		else if (!m_bBuffBua || !P_PLAYER->m_FightMode || PEOPLE_IDX != m_nBuaTarget || m_nEnemyIdx)
			BuffBuaReset();
	}
	HandleDirectSkills();
	HandleInDirectSkills();
	// Tab Di chuyen dang tim duong toi bai train / dang tu di MapPath: chi buff (o tren), chua tu ve / tim quai.
	// Bo quai dang nham (neu khong PathTravelBreathe cho "danh quai doc duong" ma khong ai danh -> dung im)
	BOOL bFindingTrain = (theMove.m_bGoTrain || PathTravelActive()) && P_PLAYER->m_FightMode;
	if (bFindingTrain && m_nEnemyIdx) { SetPeopleIdx(0); m_nEnemyIdx = 0; }
	if (bFindingTrain || theMove.m_bFindingWay || !P_PLAYER->m_FightMode || !m_bAutoHit) ApproachReset();
	// Dang tim duong toi bai train (GoTrain / MapPath / RouteFinding cach cu): bo ca quai muc tieu con sot tu truoc
	// (PEOPLE_IDX van tro quai => game KNpcAI::FollowAttack tu chay theo danh quai doc duong, bo lo trinh).
	// Chi bo quai (kind_normal) - xa phu / NPC hoi thoai (kind_dialoger) MapPath can PEOPLE_IDX de mo menu
	if (bFindingTrain || (theMove.m_bFindingWay && P_PLAYER->m_FightMode))
	{
		int nPeople = PEOPLE_IDX;
		if (nPeople > 0 && nPeople < MAX_NPC && g_pNpcArray[nPeople]->m_Kind == kind_normal)
		{
			SetPeopleIdx(0);
			m_LastNpcID = 0;
		}
	}
	// Tu ve "Thoat game" (AutoDefence ben duoi bi bo qua luc dang di toi bai train / ra skill / buff):
	// dang chay tu cong vao toa do train dau tien => dung TDP; da o bai train => thoat game
	if (m_nAutoDefence == 3 && EnemyInDefenceRange(m_nDefenceRange))
	{
		if (bFindingTrain)
		{
			theRecover.OnUseAM();
			return;
		}
		TerminateProcess(GetCurrentProcess(), 0);
	}
	PKSwitchBreathe(bFindingTrain);	// Do sat / Cuu sat (chi doi trang thai PK)
	if (m_bDoingSkill || m_bDoingSkillEx || theRecover.m_bDoingBuff || theMain.m_bPause) return;
	if (bFindingTrain) return;

	// ActiveSkill Adjustment
	// Ap sat: ep tam danh moi nhip - game tu dat lai m_CurrentAttackRadius theo tam skill (doi skill, dong bo...)
	// ma truoc day chi SetActiveSkill (luc doi muc tieu) moi dat lai => game ra skill tu xa
	int nCloseRange = HitCloseRange(this);
	if (nCloseRange)
		P_PLAYER->m_CurrentAttackRadius = nCloseRange;
	else if (P_PLAYER->m_CurrentAttackRadius < 75)
		P_PLAYER->m_CurrentAttackRadius = 1000;

	AutoDefence();

	if (m_nEnemyIdx || theQuest.m_bNeedGoPCK) return;

	if (m_bSkillChanged)
	{
		SetDefSkills(m_SettingSkills[skill_left], m_SettingSkills[skill_right]);
		int nSkillId = g_pCurPlayer->m_nLeftSkillID;
		SetActiveSkill(nSkillId);

		if (m_nHorseMode == 0)
		{
			KSkill* pActSkill = (KSkill*)(SKL_BASE_ADD + (nSkillId - 1) * SKL_DATA_SIZE);
			m_bDownHorse = (BOOL)pActSkill->m_nHorseLimited;
		}
		else
			m_bDownHorse = (m_nHorseMode == 1);
		m_bSkillChanged = 0;
	}

	if (!P_PLAYER->m_FightMode || (!m_bAutoHit && !m_bBuffBua)) return;
	if (theMove.m_bFindingWay) return;
	if (theMove.m_nStatus == status_movefirst) return;
	if (theMove.m_nStatus == status_move && !theMove.m_bAttackWhenMove) return;
	if (m_bAutoHit && theMove.LureBreathe()) return;	// Dang quay quai: chua danh

	// Buff bua (doc lap voi Danh quai): con doi tuong can buff thi buff truoc, xong het moi danh
	if (BuffBuaBreathe()) return;
	if (!m_bAutoHit) return;

	ProcessAttack();

	// BOSS wait setting
	if (m_bBossAttacking)
	{
		int nSkillId = g_pCurPlayer->m_nLeftSkillID;
		if (m_SettingSkills[skill_boss])
			nSkillId = m_SettingSkills[skill_boss];
		SetActiveSkill(nSkillId);

		KSkill* pActBossSkill = (KSkill*)(SKL_BASE_ADD + (nSkillId - 1) * SKL_DATA_SIZE);
		m_bDownHorse = (BOOL)pActBossSkill->m_nHorseLimited;
		if (m_bDownHorse && P_PLAYER->m_bRideHorse)
		{
			ExecuteScript(SCK_SHORTCUT_HORSE); // Xuong ngua
			return;
		}

		m_bBossAttacking = 0;
		m_nBossWait = MAX_BOSS_WAIT;
	}

	if (ApproachTarget(this)) return;	// Duong thang toi quai bi chan: tu chay vong vat can


	//Check if Target is LAG
	int nTargetIdx = GetCurTargetIdx();
	if (!IsLagTarget(nTargetIdx)) return;

	KAliveNpc::RefreshNpcs();

	P_PLAYER->GetMapPos(&nX, &nY);

	nRange = m_nSearchRange;
	if (theMove.m_bFollow)
	{
		if (!theMove.m_nLeaderIdx && theMove.m_bNoAttackFollow)
		{
			if (theMove.m_bAroundPointPK || theMove.m_bRouteMove)
				theMove.m_nStatus = status_idle;
			return;
		}
		if (theMove.m_nLeaderIdx)
		{
			if (g_pNpcArray[theMove.m_nLeaderIdx]->IsExist())
			{
				g_pNpcArray[theMove.m_nLeaderIdx]->GetMapPos(&nX, &nY);
				nRange = theMove.m_nFollowDistance + 75;
			}
		}
	}
	if (theMove.m_nStatus == status_pickup && nRange > PLAYER_PICKUP_DISTANCE)
		nRange = PLAYER_PICKUP_DISTANCE;
	if (m_nBossWait)
	{
		m_nBossWait--;
		if (m_bHitClose)
		{
			if (nRange > PLAYER_PICKUP_DISTANCE)
				nRange = PLAYER_PICKUP_DISTANCE;
		}
		else
		{
			if (nRange > P_PLAYER->m_CurrentAttackRadius)
				nRange = P_PLAYER->m_CurrentAttackRadius;
		}
	}
	if (theRecover.m_bEmptyMana) nRange = 0;
	if (theQuest.m_bAutoDoQuest && theQuest.m_bGoTrainGetPK) nRange = PLAYER_PICKUP_DISTANCE;

	nTargetIdx = SearchNearestNpc(nX, nY, nRange);
	if (nTargetIdx > 0)
	{
		if (m_bDownHorse && P_PLAYER->m_bRideHorse)
		{
			ExecuteScript(SCK_SHORTCUT_HORSE); // Xuong ngua
			m_LastNpcID = 0;
		}
		else if (!m_bDownHorse && !P_PLAYER->m_bRideHorse && HAVE_HORSE)
		{
			ExecuteScript(SCK_SHORTCUT_HORSE); // Len ngua
			m_LastNpcID = 0;
		}
		else
		{
			SetPeopleIdx(nTargetIdx);
			m_bSkillChanged = TRUE;
			theMove.m_nStatus = status_attack;
		}
	}
	else
	{
		SetPeopleIdx(0);
		if (theMove.m_bMoveByMapYS || theMove.m_bAroundPointPK || theMove.m_bRouteMove)
			theMove.m_nStatus = status_idle;
		else if (!P_PLAYER->m_bRideHorse && P_PLAYER->m_Doing == do_stand)
			g_pCurPlayer->DoSit();
	}
}

void CVLAttack::SetPeopleIdx(int nIndex)
{
	PEOPLE_IDX = nIndex;
}

BOOL CVLAttack::IsApproaching()
{
	return s_nApproachIdx != 0;
}

void CVLAttack::StopApproach()
{
	ApproachReset();
}

void CVLAttack::SetAutoHit()
{
	if (m_bAutoHit)
		SystemMessage(TV(L"<color=yellow>Tự đánh quái (Ctrl+A): <color=green>Bật"), 1);
	else
	{
		SetPeopleIdx(0);
		m_LastNpcID = 0;
		SystemMessage(TV(L"<color=yellow>Tự đánh quái (Ctrl+A): <color>Tắt"), 1);
	}

	if (m_nSkillRange < 75)
		m_nSkillRange = 75;

	if (m_nDefSkillRange < 75)
		m_nDefSkillRange = 75;

	//ExecuteScript("AddCommand('F10, '', 'DirectShortcutSkill(8)')");
}

void CVLAttack::SetDefSkills(int nLeftID, int nRightID)
{
	KUiSkillData theSkill;

	if (nLeftID)
	{
		theSkill.uGenre = SKILL_SHORTCUT;
		theSkill.uId = nLeftID;
		g_pCoreShell->OperationRequest(GOI_SET_IMMDIA_SKILL, (int)& theSkill, 0);
	}
	if (nRightID)
	{
		theSkill.uGenre = SKILL_SHORTCUT;
		theSkill.uId = nRightID;
		g_pCoreShell->OperationRequest(GOI_SET_IMMDIA_SKILL, (int)& theSkill, 1);
	}
}

void CVLAttack::SetActiveSkill(int nSkillId)
{
	P_PLAYER->m_ActiveSkillID = nSkillId;
	int nSkillRange = HitCloseRange(this);

	if (!nSkillRange)
	{
		nSkillRange = P_PLAYER->m_CurrentAttackRadius;
		if (nSkillRange < 75)
			nSkillRange = 1000;
	}

	P_PLAYER->m_CurrentAttackRadius = nSkillRange;
}

void CVLAttack::HandleDirectSkills()
{
	static int nTimerCount = 0;
	if (!P_PLAYER->m_FightMode)
	{
		nTimerCount = 0;
		return;
	}
	nTimerCount++;

	BOOL bDrSkillFlag[3] = { TRUE, TRUE, TRUE };
	KStateNode* pNode = (KStateNode*)P_PLAYER->m_pHeadNode;
	while (pNode)
	{
		if (pNode->m_LeftTime > 180)
		{
			if (pNode->m_SkillID == m_SettingSkills[0])
				bDrSkillFlag[0] = 0;
			if (pNode->m_SkillID == m_SettingSkills[1])
				bDrSkillFlag[1] = 0;
			if (pNode->m_SkillID == m_SettingSkills[2])
				bDrSkillFlag[2] = 0;
		}
		pNode = (KStateNode*)pNode->GetNext();
	}
	for (int i = 0; i < 3; i++)
	{
		if (m_SettingSkills[i] <= 0) continue;
		if (m_nDoingDirectSkill[i]) continue;

		BOOL bNeedSkill = bDrSkillFlag[i];

		if (bNeedSkill)
		{
			m_nDoingDirectSkill[i] = 18;	// 1800ms
			m_LastNpcID = 0;
			m_bDoingSkill = TRUE;
			SetPeopleIdx(0);
		}
	}
	for (int i = 0; i < 3; i++)
	{
		if (m_nDoingDirectSkill[i] <= 6) continue;	// 600ms
		m_nDoingDirectSkill[i]--;
		if (m_nDoingDirectSkill[i] == 12)	// 1200ms
		{
			P_PLAYER->SendCommand(do_skill, m_SettingSkills[i], -1, 1);
			theAttack.SendVLCmdSkill(m_SettingSkills[i], -1, P_PLAYER->m_dwID);
		}
		return;
	}
	m_bDoingSkill = FALSE;
	for (int i = 0; i < 3; i++)
		if (m_nDoingDirectSkill[i]) m_nDoingDirectSkill[i]--;
}

void CVLAttack::HandleInDirectSkills()
{
	static int nTimerCount = 0;
	static int nLastMana = 0;
	static int nLastLife = 0;

	if (m_AlterSkill[0].nAlterType == 0)
	{
		static int nSkillIdx = 0;
		static int nTimeWait = 0;

		if (!P_PLAYER->m_FightMode)
		{
			nTimerCount = 0;
			return;
		}
		if (nSkillIdx && nTimeWait == 6)
		{
			DoSpecialSkill(nSkillIdx, 1);
			nSkillIdx = 0;
		}
		if (nTimeWait)
		{
			nTimeWait--;
			return;
		}
		m_bDoingSkillEx = FALSE;
		if (m_AlterSkill[0].nUseSkill)
		{
			if (nTimerCount % (m_AlterSkill[0].nAlterValue / 100) == 1)
			{
				m_LastNpcID = 0;
				m_bDoingSkillEx = TRUE;
				SetPeopleIdx(0);
				nTimeWait = 12;
				nSkillIdx = m_AlterSkill[0].nUseSkill - 1;
			}
		}
		if (m_AlterSkill[1].nUseSkill)
		{
			if (nTimerCount % (m_AlterSkill[1].nAlterValue / 100) == 2)
			{
				m_LastNpcID = 0;
				m_bDoingSkillEx = TRUE;
				SetPeopleIdx(0);
				nTimeWait = 12;
				nSkillIdx = m_AlterSkill[1].nUseSkill - 1;
			}
		}
		nTimerCount++;
		return;
	}

	if (m_nEnemyIdx && m_SettingSkills[skill_defence])
	{
		nTimerCount = 0;
		return;
	}
	int nTotalTime = 0;

	if (m_AlterSkill[0].nUseSkill && m_AlterSkill[1].nUseSkill)
	{
		int nSkillIdx1 = m_AlterSkill[0].nUseSkill - 1;
		int nSkillIdx2 = m_AlterSkill[1].nUseSkill - 1;

		switch (m_AlterSkill[0].nAlterType)
		{
		case 0: //DirectSkill
			break;
		case 1: //By Time
			nTotalTime = (m_AlterSkill[0].nAlterValue + m_AlterSkill[1].nAlterValue) / 100;
			if (nTotalTime > 0)
			{
				if (nTimerCount % nTotalTime == 0)
					DoSpecialSkill(nSkillIdx1, 0);
				else if (nTimerCount % nTotalTime == m_AlterSkill[0].nAlterValue / 100)
					DoSpecialSkill(nSkillIdx2, 0);
			}
			break;
		case 2: //By Mana
			if (P_PLAYER->m_CurrentMana < nLastMana && P_PLAYER->m_CurrentMana < m_AlterSkill[0].nAlterValue)
				DoSpecialSkill(nSkillIdx1, 0);
			if (P_PLAYER->m_CurrentMana > nLastMana && P_PLAYER->m_CurrentMana > m_AlterSkill[1].nAlterValue)
				DoSpecialSkill(nSkillIdx2, 0);
			nLastMana = P_PLAYER->m_CurrentMana;
			break;
		case 3: //By Health
			if (P_PLAYER->m_CurrentLife < nLastLife && P_PLAYER->m_CurrentLife < m_AlterSkill[0].nAlterValue)
				DoSpecialSkill(nSkillIdx1, 0);
			if (P_PLAYER->m_CurrentLife > nLastLife && P_PLAYER->m_CurrentLife > m_AlterSkill[1].nAlterValue)
				DoSpecialSkill(nSkillIdx2, 0);
			nLastLife = P_PLAYER->m_CurrentLife;
			break;
		}
	}
	nTimerCount++;
}

void CVLAttack::DoSpecialSkill(int nSkill, BOOL bDirect)
{
	char szScript[32];
	if (bDirect)
		sprintf(szScript, "DirectShortcutSkill(%d)", nSkill);
	else
		sprintf(szScript, "ShortcutSkill(%d)", nSkill);
	ExecuteScript(szScript);
	m_bSkillChanged = TRUE;
}

BOOL CVLAttack::IsLagTarget(int nIndex)
{
	static DWORD dwLastNpcID = 0;
	static int nLagCount = 0;
	static int nFollowCount = 0;

	for (int i = 0; i < 3; i++)
	{
		if (m_BadNpc[i].m_LifeTime)
			m_BadNpc[i].m_LifeTime--;
		else
			m_BadNpc[i] = 0;
	}

	if (!nIndex)
		return TRUE;

	KNpc* pNpc = g_pNpcArray[nIndex];
	if (theMove.m_bFollow && pNpc->m_Kind == kind_player)
	{	//Follow a Player in 800ms only
		if (nFollowCount < 8)
		{
			nFollowCount++;
			return 0;
		}
		else
		{
			SetPeopleIdx(0);
			nFollowCount = 0;
			return TRUE;
		}
	}
	if (!pNpc->IsExist() || pNpc->m_Kind != kind_normal)
	{
		SetPeopleIdx(0);
		return TRUE;
	}
	// NPC "ma": region game dang giu da nap lai cho vung khac => game (FollowAttack) thay quai "trong tam"
	// va ra skill vao cho trong, quai khong mat mau nhung phai doi het nLagAttCheck. Bo ngay.
	if (pNpc->IsGhost())
	{
		SetPeopleIdx(0);
		m_LastNpcLife = 0;
		dwLastNpcID = 0;
		nLagCount = 0;
		return TRUE;
	}
	// Quai da chet (Life <= 0 / dang nam xac / hoi sinh) bo ngay.
	// Truoc day Life == 0 lot vao nhanh "quai dang mat mau" roi "chua mat mau" => giu muc tieu
	// toi het nLagAttCheck => game van ra skill vao xac / cho trong.
	if (pNpc->m_CurrentLife <= 0 || pNpc->m_Doing == do_death || pNpc->m_Doing == do_revive)
	{
		SetPeopleIdx(0);
		m_LastNpcLife = 0;
		dwLastNpcID = 0;
		nLagCount = 0;
		return TRUE;
	}
	if (m_bAvoidBoss && pNpc->m_cGold.m_nGoldType == 16)
	{
		SetPeopleIdx(0);
		return TRUE;
	}
	if (m_bOnlyBlueBoss == TRUE && !IsBlueBoss(pNpc))
	{	// Chi danh boss xanh: bo muc tieu khac (vd quai dang danh truoc khi bat)
		SetPeopleIdx(0);
		return TRUE;
	}
	if (!DebugQuestTargetOK(pNpc->Name))
	{	// DEBUG > Nhiem vu > Buoc 4: bo quai khong phai quai nhiem vu (vd dang danh truoc khi bat Buoc 4)
		SetPeopleIdx(0);
		m_LastNpcLife = 0;
		dwLastNpcID = 0;
		nLagCount = 0;
		return TRUE;
	}
	if (pNpc->m_cGold.m_nIsGold || pNpc->m_cGold.m_nIsGolding)
	{
		m_bBossAttacking = TRUE;	//Attacking a BOSS
		m_nBossWait = 0;
	}
	if (pNpc->m_dwID != dwLastNpcID)
	{
		nLagCount = 0; // new Npc, count from 0
		dwLastNpcID = pNpc->m_dwID;
	}
	if (pNpc->m_CurrentLife >= 0)
	{
		if (pNpc->m_CurrentLife < m_LastNpcLife - 1)
		{	// OK, Npc is dying
			m_LastNpcLife = pNpc->m_CurrentLife;
			nLagCount = 0;
			theMove.m_nStatus = status_attack;
			return 0;
		}
		else
		{	// Oh, Npc is not hurt
			m_LastNpcLife = pNpc->m_CurrentLife;
			int nLagRunCheck;
			if (P_PLAYER->m_CurrentRunSpeed)
				nLagRunCheck = MAX_LAG_CHECK / P_PLAYER->m_CurrentRunSpeed;
			else
				nLagRunCheck = MAX_LAG_CHECK / 10;

			int nLagAttCheck = nLagRunCheck;
			if (P_PLAYER->m_Level < 150)
				nLagAttCheck += (ATT_LAG_CHECK - P_PLAYER->m_Level / 5);
			else
				nLagAttCheck += ATT_LAG_CHECK - 30;
			nLagCount++;

			if (P_PLAYER->m_Doing == do_attack && nLagCount < nLagRunCheck && nLagCount > 2)
				nLagCount = nLagRunCheck; //attacking then increase check lag
			if (nLagCount < nLagAttCheck) return 0;
			// Quai lag: ghi vao m_BadNpc va bo han. Truoc day du 3 o thi khong ghi duoc, nLagCount giu nguyen
			// => moi nhip deu "lag" nhung SearchNearestNpc van nhan lai chinh con do (khoa cung muc tieu)
			AddBadNpc(pNpc->m_dwID, pNpc->m_CurrentLife);
			SetPeopleIdx(0);
			m_LastNpcLife = 0;
			dwLastNpcID = 0;
			nLagCount = 0;
		}
	}
	return TRUE;
}

void CVLAttack::AddBadNpc(DWORD dwID, int nLife)
{
	int iSlot = -1;
	for (int i = 0; i < 3 && iSlot < 0; i++)
		if (m_BadNpc[i].m_dwID == dwID) iSlot = i;
	if (iSlot < 0)
	{	// O trong truoc, khong co thi o sap het han nhat
		iSlot = 0;
		for (int i = 1; i < 3; i++)
		{
			int nCur = m_BadNpc[i].IsNull() ? -1 : m_BadNpc[i].m_LifeTime;
			int nBest = m_BadNpc[iSlot].IsNull() ? -1 : m_BadNpc[iSlot].m_LifeTime;
			if (nCur < nBest) iSlot = i;
		}
	}
	m_BadNpc[iSlot].SetValue(dwID, nLife);
}

int CVLAttack::SearchNearestNpc(int nX, int nY, int nRange)
{
	int dZ, dMax = 0;
	int nMinIndex = 0, dMin = nRange;
	int nMinQIndex = 0, dMinQ = nRange;

	BOOL bNoFullLag = (m_BadNpc[0].IsNull() || m_BadNpc[1].IsNull() || m_BadNpc[2].IsNull());

	for (int i = 1; i < MAX_NPC; i++)
	{
		if (!m_AliveNpc[i].m_dwID) continue;
		if (m_AliveNpc[i].m_dwID == m_LastNpcID)
		{
			m_LastNpcID = 0;
			continue;
		}
		dZ = __distance(m_AliveNpc[i].m_nPosX, m_AliveNpc[i].m_nPosY, nX, nY);
		if (dZ >= nRange) continue;

		if (m_bAvoidBoss && m_AliveNpc[i].m_nGoldType == 16 && dZ <= 800) return 0;

		if (bNoFullLag)
		{
			if (theMain.m_bAttackBossFirst && m_AliveNpc[i].m_bIsGold) // Hey, BOSS
			{
				nMinIndex = i;
				break;
			}
			if (dZ < dMin)
			{
				dMin = dZ;
				nMinIndex = i;
			}
			if (m_AliveNpc[i].m_nNpcSIdx == theQuest.m_nNpcSIndex)
			{
				if (dZ < dMinQ)
				{
					dMinQ = dZ;
					nMinQIndex = i;
				}
			}
		}
		else if (dZ > dMax)
		{	//More than 3 lagged mobs
			dMax = dZ;
			nMinIndex = i;
			if (dZ > 128)
				break;
		}
	}
	if (nMinQIndex) nMinIndex = nMinQIndex;
	if (nMinIndex)
	{
		if (m_bAvoidBoss && m_AliveNpc[nMinIndex].m_nGoldType == 16)
			return 0;

		m_LastNpcID = m_AliveNpc[nMinIndex].m_dwID; // Save Last Npc ID
		m_LastNpcLife = m_AliveNpc[nMinIndex].m_CurLife; // Save Last Npc Life
		int nDestX = m_AliveNpc[nMinIndex].m_nPosX;
		int nDestY = m_AliveNpc[nMinIndex].m_nPosY;
		g_pCurPlayer->MoveTo(nDestX, nDestY);
	}
	return nMinIndex;
}

void CVLAttack::AutoDefence()
{
	static int nTimerCount = 0;

	nTimerCount++;
	if (nTimerCount % 3 != 1) return;

	if (!m_bAlwaysF9)
	{
		static int nPKTimeCount = 0;
		// Moi nhip o day ~300ms (Breathe 100ms, chi chay 1/3 nhip).
		// Dem thoi gian PK bat ma KHONG co ke thu: co ke thu thi dem lai tu dau.
		if (P_PLAYER->m_nPKFlag && !m_nEnemyIdx)
			nPKTimeCount++;
		else
			nPKTimeCount = 0;
		if (nPKTimeCount > 100 && nPKTimeCount % 20 == 1 && !PKSwitchActive())	// Dang Do sat / Cuu sat: khong tu tat PK
		{	// 100 ~ 30s khong co ke thu => ve Luyen cong; chua ve duoc thi thu lai moi ~6s
			if (m_nPKMode >= 1 && m_nPKMode <= 3)
				CS_LOG("TU VE tu tat PK (30s khong ke thu) vi Do sat/Cuu sat CHUA BAT (armed %d, map %d / MapPK %d)",
					PKSwitchActive(), *g_pMapID, m_nPKMapID);
			ExecuteScript("Switch('PK',0)");
			return;
		}
	}

	int nDefenceMode = m_nAutoDefence;
	if (theQuest.m_bAutoDoQuest &&
		theQuest.m_bGoTrainGetPK &&
		theQuest.m_TongPKOpt.m_bKillOther)
		nDefenceMode = 5;

	// Tat tu ve / ve thanh / tan thu: nha ke thu dang giu - neu khong m_nEnemyIdx ket lai,
	// Breathe "if (m_nEnemyIdx) return" => khong danh quai nua, dung im ke ca khi chuyen lai "Bo qua"
	if (!P_PLAYER->m_FightMode || !nDefenceMode || P_PLAYER->m_CurrentCamp == camp_begin)
	{
		ReleaseEnemy();
		return;
	}

	//Search for Enemies
	int nEnemyIdx[3] = { 0, 0, 0 };
	int nEnemyIndex = 0;
	int nMaxConquer = -1;

	int nNpcX, nNpcY, nPlayerX, nPlayerY, dZ, nRelation, nEnemyRelation = relation_none;
	int nSelfIdx = g_pCurPlayer->m_nIndex;
	P_PLAYER->GetMapPos(&nPlayerX, &nPlayerY);

	for (int i = 1; i < MAX_NPC; i++)
	{
		if (i == nSelfIdx) continue;	// Chinh minh (PK do sat + phe tu do => tu chon minh lam ke thu => dung im)
		if (!g_pNpcArray[i]->IsExist()) continue;
		if (g_pNpcArray[i]->m_CurrentLife <= 0 || g_pNpcArray[i]->m_Kind != kind_player) continue;
		if (g_pNpcArray[i]->IsGhost()) continue;
		if (g_pNpcArray[i]->m_CurrentCamp == camp_begin) continue;
		if (nDefenceMode == 5 && !strcmp(g_pNpcArray[i]->Name, theQuest.m_TongPKOpt.m_szVictimName))
		{
			if (theQuest.m_nDoQuestStep < 15)
			{
				g_pNpcArray[i]->GetMapPos(&nNpcX, &nNpcY);
				dZ = __distance(nPlayerX, nPlayerY, nNpcX, nNpcY);
				if (dZ > m_nDefenceRange) continue;
				if (g_pNpcArray[i]->m_CurrentCamp == P_PLAYER->m_CurrentCamp)
				{
					KUiPlayerItem thePlayer;
					memset(&thePlayer, 0, sizeof(KUiPlayerItem));
					strcpy(thePlayer.Name, g_pNpcArray[i]->Name);
					thePlayer.uId = g_pNpcArray[i]->m_dwID;
					thePlayer.nIndex = i;
					thePlayer.nData = 1;
					ProcessPeople(&thePlayer, ACTION_REVENGE);
				}
				else
					ExecuteScript("Switch('PK',2)");
				theQuest.m_nDoQuestStep = 15;
				break;
			}
		}
		if (SkipSameTong(g_pNpcArray[i])) continue;
		nRelation = g_pCoreShell->GetNPCRelation(i);
		if (nRelation == relation_ally) continue;
		if (nRelation == relation_none)
		{
			if (P_PLAYER->m_CurrentCamp == g_pNpcArray[i]->m_CurrentCamp && P_PLAYER->m_CurrentCamp < camp_free) continue;
			if (g_pNpcArray[i]->m_nPKFlag != 2) continue;
		}
		g_pNpcArray[i]->GetMapPos(&nNpcX, &nNpcY);
		dZ = __distance(nPlayerX, nPlayerY, nNpcX, nNpcY);
		if (dZ < m_nDefenceRange)
		{
			if (nDefenceMode == 2)
			{
				theRecover.OnUseAM();
				return;
			}
			if (nDefenceMode == 3)
			{	// Thoat game: giet tien trinh ngay, khong cho Exit() / dem nguoc cua game
#if DebugCuuSat
				CS_LOG("TU VE: thoat game vi %s trong tam %d", g_pNpcArray[i]->Name, dZ);
				CSFlush();	// Ghi bo dem log truoc khi tien trinh chet
#endif
				TerminateProcess(GetCurrentProcess(), 0);
				return;
			}
			int nConquer = theMain.GetConquerVal(P_PLAYER->m_Series, g_pNpcArray[i]->m_Series);
			if (!nEnemyIdx[0])
			{
				nEnemyIdx[0] = i;
				if (nConquer >= nMaxConquer)
				{
					nMaxConquer = nConquer;
					nEnemyIndex = i;
					nEnemyRelation = nRelation;
				}
			}
			else if (!nEnemyIdx[1])
			{
				nEnemyIdx[1] = i;
				if (nConquer >= nMaxConquer)
				{
					nMaxConquer = nConquer;
					nEnemyIndex = i;
					nEnemyRelation = nRelation;
				}
			}
			else if (!nEnemyIdx[2])
			{
				nEnemyIdx[2] = i;
				if (nConquer >= nMaxConquer)
				{
					nMaxConquer = nConquer;
					nEnemyIndex = i;
					nEnemyRelation = nRelation;
				}
				break;
			}
		}
	}
	if (nEnemyIndex)
	{
		SetPeopleIdx(nEnemyIndex);
		m_nEnemyIdx = nEnemyIndex;
		theMove.m_nStatus = status_attack;

		int nSkillId = g_pCurPlayer->m_nLeftSkillID;
		if (m_SettingSkills[skill_defence])
			nSkillId = m_SettingSkills[skill_defence];
		SetActiveSkill(nSkillId);

		if (m_bDefHorseMode)
		{
			KSkill* pActSkill = (KSkill*)(SKL_BASE_ADD + (nSkillId - 1) * SKL_DATA_SIZE);
			m_bDownHorse = (BOOL)pActSkill->m_nHorseLimited;
			if (m_bDownHorse && P_PLAYER->m_bRideHorse)
				ExecuteScript(SCK_SHORTCUT_HORSE);
		}

		// Danh tra khong tu chuyen PK sang Chien dau (PK 1) nua - trang thai PK do nguoi choi / Do sat - Cuu sat quyet dinh
		(void)nEnemyRelation;
	}
	else
		ReleaseEnemy();
}

// Bo ke thu dang giu: tra skill trai, xoa muc tieu PK de Breathe tu tim quai lai
void CVLAttack::ReleaseEnemy()
{
	if (!m_nEnemyIdx) return;
	if (PEOPLE_IDX == m_nEnemyIdx) SetPeopleIdx(0);
	SetDefSkills(m_SettingSkills[skill_left], 0);
	m_nEnemyIdx = 0;
	m_bSkillChanged = TRUE;
}

void CVLAttack::ProcessAttack()
{

	if (!P_PLAYER->m_FightMode || PEOPLE_IDX != 0)
		return;

	int nEnemyX, nEnemyY, nPlayerX, nPlayerY;
	KNpc* pEnemy = g_pNpcArray[m_nPeopleIdx];

	if (!pEnemy->IsExist() || pEnemy->m_Doing == do_death || pEnemy->m_Doing == do_revive)
	{
		SetPeopleIdx(0);
		return;
	}

	int nRelation = g_pCoreShell->GetNPCRelation(m_nPeopleIdx);
	if (nRelation != relation_enemy)
	{
		SetPeopleIdx(0);
		return;
	}

	pEnemy->GetMapPos(&nEnemyX, &nEnemyY);
	P_PLAYER->GetMapPos(&nPlayerX, &nPlayerY);
	int nDistance = __distance(nEnemyX, nEnemyY, nPlayerX, nPlayerY);

	if (nDistance > P_PLAYER->m_CurrentAttackRadius)
	{
		int attRadius = P_PLAYER->m_CurrentAttackRadius * 80 / 100;
		int nDestX = nEnemyX - (attRadius * (nEnemyX - nPlayerX) / nDistance);
		int nDestY = nEnemyY - (attRadius * (nEnemyY - nPlayerY) / nDistance);

		g_pCurPlayer->MoveTo(nDestX, nDestY);
	}
	else
	{
		int nX = nEnemyX - nPlayerX + 400;
		int nY = (nEnemyY - nPlayerY) / 2 + 280;
		SetPeopleIdx(m_nPeopleIdx);
		SendVLCmdSkill(g_pCurPlayer->m_nLeftSkillID, nX, nY);
	}

}

int CVLAttack::GetCurTargetIdx()
{
	return PEOPLE_IDX;
}

void CVLAttack::SendVLCmdSkill(int nSkillId, int nX, int nY)
{
	::SendClientCmdSkill(nSkillId, nX, nY);
}
