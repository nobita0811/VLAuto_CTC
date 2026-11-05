#include "stdafx.h"
#include "../GameDef.h"
#include "GameBase.h"
#include "JxCore.h"
#include "S3Client.h"
#include "VLClasses.h"
#include "VLDebug.h"
#include "VLObstacle.h"

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

static BOOL		s_bPKArmed = FALSE;
static int		s_nPKArmMap = 0;
static DWORD	s_dwPKCheckTick = 0;
static DWORD	s_dwPKSwitchTick = 0;
static DWORD	s_dwPKSeenTick = 0;
static DWORD	s_dwRevengeID = 0;
static DWORD	s_dwRevengeTick = 0;

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

static void SwitchPK(int nFlag)
{
	if (P_PLAYER->m_nPKFlag == nFlag) return;
	DWORD dwNow = GetTickCount();
	if (s_dwPKSwitchTick && dwNow - s_dwPKSwitchTick < PK_SWITCH_DELAY) return;
	s_dwPKSwitchTick = dwNow;
	ExecuteScript(nFlag == 2 ? "Switch('PK',2)" : (nFlag == 1 ? "Switch('PK',1)" : "Switch('PK',0)"));
}

// Tam ra skill: AttackRadius cua skill tu ve (khong dat thi skill tay trai); khong doc duoc thi tam tu ve
static int PKSkillRange(CVLAttack* p)
{
	int nSkillId = p->m_SettingSkills[skill_defence] ? p->m_SettingSkills[skill_defence] : g_pCurPlayer->m_nLeftSkillID;
	if (nSkillId > 0 && nSkillId < 3000)
	{
		KSkill* pSkill = (KSkill*)(SKL_BASE_ADD + (nSkillId - 1) * SKL_DATA_SIZE);
		if (pSkill->m_dwSkillID == (DWORD)nSkillId && pSkill->m_nSkillRange > 0)
			return pSkill->m_nSkillRange;
	}
	return p->m_nDefenceRange;
}

void CVLAttack::PKSwitchBreathe(BOOL bFindingTrain)
{
	int nMap = *g_pMapID;
	if (m_nPKMode < 1 || m_nPKMode > 3 || !m_nPKMapID || nMap != m_nPKMapID)
	{
		s_bPKArmed = FALSE;
		s_nPKArmMap = 0;
		return;
	}
	DWORD dwNow = GetTickCount();
	if (dwNow - s_dwPKCheckTick < PK_CHECK_DELAY) return;
	s_dwPKCheckTick = dwNow;

	if (theMain.m_bPause || !P_PLAYER->m_FightMode || P_PLAYER->m_CurrentCamp == camp_begin) return;
	if (P_PLAYER->m_CurrentLife <= 0) return;

	int nPlayerX, nPlayerY;
	P_PLAYER->GetMapPos(&nPlayerX, &nPlayerY);
	if (!s_bPKArmed || s_nPKArmMap != nMap)
	{
		if (bFindingTrain) return;	// Dang tu di toi bai train
		int nX, nY;
		if (GetPKArmPoint(&nX, &nY) && __distance(nPlayerX, nPlayerY, nX, nY) > PK_ARM_NEAR) return;
		s_bPKArmed = TRUE;
		s_nPKArmMap = nMap;
		s_dwPKSeenTick = 0;
		s_dwRevengeID = 0;
	}

	if (m_nPKMode == 1)
	{	// Do sat tat ca
		SwitchPK(2);
		return;
	}

	// Ke thu trong danh sach gan nhat trong tam ra skill
	int nRange = PKSkillRange(this);
	int nSelfIdx = g_pCurPlayer->m_nIndex;
	int nFound = 0, dMin = MAX_INTNUM;
	for (int i = 1; i < MAX_NPC; i++)
	{
		if (i == nSelfIdx) continue;
		KNpc* pNpc = g_pNpcArray[i];
		if (!pNpc->IsExist() || pNpc->m_Kind != kind_player) continue;
		if (pNpc->m_CurrentLife <= 0 || pNpc->IsGhost() || pNpc->m_CurrentCamp == camp_begin) continue;
		if (!IsListedEnemy(pNpc->Name)) continue;
		int nX, nY;
		pNpc->GetMapPos(&nX, &nY);
		int dZ = __distance(nPlayerX, nPlayerY, nX, nY);
		if (dZ <= nRange && dZ < dMin) { dMin = dZ; nFound = i; }
	}

	if (m_nPKMode == 2)
	{	// Do sat theo danh sach
		if (nFound)
		{
			s_dwPKSeenTick = dwNow;
			SwitchPK(2);
		}
		else if (!s_dwPKSeenTick || dwNow - s_dwPKSeenTick >= PK_LIST_HOLD)
			SwitchPK(1);
		return;
	}

	// Cuu sat theo danh sach
	if (!nFound) return;
	KNpc* pEnemy = g_pNpcArray[nFound];
	if (g_pCoreShell->GetNPCRelation(nFound) == relation_enemy) return;	// Dang la ke thu roi (da cuu sat / do sat)
	if (!P_PLAYER->m_nPKFlag) { SwitchPK(1); return; }	// Luyen cong: chuyen chien dau truoc
	if (s_dwRevengeID == pEnemy->m_dwID && dwNow - s_dwRevengeTick < PK_REVENGE_DELAY) return;
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

void CVLAttack::Breathe()
{
	int nX, nY, nRange;

	if (g_NetworkOffline) return;

	DebugAttackWatch();	// Cua so DEBUG > Chien dau: Kiem tra doi tuong dang danh
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

	if (!P_PLAYER->m_FightMode || !m_bAutoHit) return;
	if (theMove.m_bFindingWay) return;
	if (theMove.m_nStatus == status_movefirst) return;
	if (theMove.m_nStatus == status_move && !theMove.m_bAttackWhenMove) return;
	if (theMove.LureBreathe()) return;	// Dang quay quai: chua danh

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
		if (P_PLAYER->m_nPKFlag)
			nPKTimeCount++;
		else
			nPKTimeCount = 0;
		if (nPKTimeCount > 600 && nPKTimeCount % 50 == 1 && !m_nEnemyIdx && !PKSwitchActive())	// Dang Do sat / Cuu sat: khong tu tat PK
		{	// 600 ~ 3 minutes
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

		if (!P_PLAYER->m_nPKFlag && nEnemyRelation == relation_none)
			ExecuteScript("Switch('PK',1)");
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
