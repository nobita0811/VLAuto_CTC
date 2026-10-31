#pragma once

// Dung chung cho cac file VLDebug*.cpp (moi tab cua cua so DEBUG mot file)

extern DEBUG_REPLY	g_DbgReply;

void		DbgOut(const char *szFormat, ...);			// Ghi them vao g_DbgReply.szText
const char*	DbgDoingName(int nDoing);
void		DbgOutPlayerPos(const char *szLabel);
BOOL		DbgCheckPlayer();							// FALSE neu chua co du lieu Player

// Tra ve FALSE neu lenh khong thuoc tab nay
BOOL		DebugMapCmd(const DEBUG_PARAM *pParam);
BOOL		DebugCollectCmd(const DEBUG_PARAM *pParam);
BOOL		DebugAttackCmd(const DEBUG_PARAM *pParam);
void		DebugAttackWatch();						// CVLAttack::Breathe: bao muc tieu dang danh len kenh chat (neu bat)
void		DebugAttackUnhook();						// Go hook KNpc::Paint (Res cua nut Kiem tra doi tuong) truoc khi FreeLibrary
void		DebugFileLogToggle(int nX);				// Bat/tat ghi file/lua game nap (VLDebugFile.cpp)
void		DebugFileLogView(int nX, const char *szFilter);
void		DebugFileUnhook();						// Go hook engine.dll truoc khi FreeLibrary
void		DebugFileBreathe();						// Ghi task value thay doi khi dang bat ghi (200ms)
BOOL		DebugRecoverCmd(const DEBUG_PARAM *pParam);
BOOL		DebugLowCfgCmd(const DEBUG_PARAM *pParam);
BOOL		DebugQuestCmd(const DEBUG_PARAM *pParam);
void		DebugQuestCoreData(DWORD uId, DWORD uParam, int nParam);	// Goi tu hook CoreDataChanged (VLDebugFile.cpp)
void		DebugQuestSysMsg(const char *szMsg, int nLen, int nChannel);	// Goi tu hook SystemMessageArrival (VLDebugFile.cpp)
void		DebugQuestBreathe();						// Tab Nhiem vu: chay 5 buoc lam nhiem vu Da Tau (200ms, ca khi tam dung)
BOOL		DebugQuestTargetOK(const char *szNpcName);	// Buoc 4 dang chay: chi danh quai nhiem vu (KAliveNpc::RefreshNpcs)
BOOL		DebugQuestHoldMove();						// Buoc 3 / 4 (dang chay hoac vua xong o map nay): CVLMove khong di theo danh sach toa do
BOOL		DebugQuestMoveOK(void *pCaller);			// KPlayer::MoveTo / GoTo: Buoc 4 / giu dung thi chan lenh di cua phan khac (tru Chien dau, Nhat do)
void		DebugQuestMoveAllow(BOOL bAllow);			// Bao quanh Chien dau / Nhat do / Nhiem vu: cho phep MoveTo
struct CQuestMoveAllow { CQuestMoveAllow() { DebugQuestMoveAllow(TRUE); } ~CQuestMoveAllow() { DebugQuestMoveAllow(FALSE); } };
BOOL		DebugQuestActive();							// Tab Nhiem vu dang chay buoc nao do (chay ca khi tam dung)
// g_SysPause: KPlayer::MoveTo / GoTo bi chan, tru khi bao boi CPauseMoveAllow (JxCore.cpp)
extern int	g_nPauseMoveAllow;
struct CPauseMoveAllow
{
	BOOL m_bOn;
	CPauseMoveAllow(BOOL bOn = TRUE) : m_bOn(bOn) { if (m_bOn) g_nPauseMoveAllow++; }
	~CPauseMoveAllow() { if (m_bOn) g_nPauseMoveAllow--; }
};
void		DebugQuestChannelMsg(DWORD nChannelID, const char *szSend, const char *szMsg, int nLen);	// Goi tu hook ChannelMessageArrival
BOOL		DebugFileCoreDataHook(BOOL bOn);			// Tab Nhiem vu: hook CoreDataChanged (dung chung voi Ghi file/lua)

// Auto lam nhiem vu Da Tau diet quai (CVLQuest::DoQuestAttackNpc) bang Buoc 2 + 3 cua tab Nhiem vu (VLDebugQuest.cpp)
enum { qk_idle, qk_running, qk_done, qk_failed };
BOOL		QuestKillStart(int nCount, int nMapID, const char *szNpcName, const char *szMapName);	// TRUE = da bat dau
int			QuestKillState(int *pKilled, int *pNeed);	// qk_...; qk_done = dang hien bang phan thuong (chua chon o)
void		QuestKillStop();							// Auto thoi lam (dung buoc dang chay neu do Auto bat)
const char*	QuestKillLastMsg();						// Dong nhat ky buoc gan nhat (ly do dung)
void		DebugLowCfgBreathe();						// Giu tat hinh + tu giam RAM (200ms)
void		DebugLowCfgUnhook();						// Tra lai FPS/hinh/nguoi choi truoc khi FreeLibrary
BOOL		DebugPathCmd(const DEBUG_PARAM *pParam);
void		DebugPathBreathe();						// Tu ghi / chay thu MapPath (200ms)
BOOL		DebugPathKey(WPARAM wParam, LPARAM lParam);	// Phim tat MapPath (TRUE = da xu ly)
void		DebugPathUnhook();						// Go patch ve truoc khi FreeLibrary

// Tu di theo MapPath (VLDebugPath.cpp) - dung trong Auto khi xa phu khong co map can toi
BOOL		PathTravelStart(int nTarget, BOOL bUseMenu);	// TRUE = co lo trinh, bat dau di. bUseMenu: tinh ca menu xa phu dang mo
void		PathTravelBreathe();						// 200ms
BOOL		PathTravelItemStart(int nTarget);				// TRUE = co vat pham ([Item]) toi nTarget, bat dau di (CVLMap::ChangeMap)
BOOL		PathTravelActive();
BOOL		PathMapHasDriver(int nMap);						// Map co xa phu trong Maps\Path (thanh/thon)
void		PathTravelStop(const char *szReason = NULL);
void		PathTravelResume();								// Mo lai sau tam ngung Auto: lap ke hoach lai
void		PathTravelInfo(char *szInfo, int nMax);
void		PathNoteMove(int x, int y, const char *szSrc);	// Ghi lenh MoveTo de ve (nut Ve duong di)
int			PathShowWalk(int nOn);						// Bat/tat ve duong di (-1 dao)
void		PathWalkReset();							// Lan sau PathWalkTo lap lai duong
int			PathMapPoints(int nMapID, POINT *pPts, int nMax, BOOL bNoDoor = FALSE, int *pDoorSkip = NULL);	// Cac diem MapPath cua map (0 = chua co MapPath). bNoDoor: bo diem gan cong
void		PathWalkNoDoor(BOOL bOn);					// PathWalkTo khong di qua / toi diem gan cong (tab Nhiem vu, Buoc 4)
const char*	PathFileShow(int nMapID);					// "Maps\Path\<MapID>_<Ten>.txt" (TCVN3) de in ra DEBUG
int			PathLoadNpcs(int nMapID, POINT *pNpc);		// Muc [NPC] cua file MapPath -> pNpc[npc_Nums]: so NPC co toa do, -1 khong co file
BOOL		PathSaveNpc(int nMapID, int nIdx, int x, int y);	// Ghi 1 toa do NPC vao muc [NPC]
int			PathWalkTo(int tx, int ty);					// Di trong map theo MapPath: 1 dang di, 0 toi cuoi duong, -1 khong co MapPath

// Game tu tim duong (KPlayer::GoTo) - VLMove.cpp
#define AUTO_GOTO_NEAR	150								// Gan hon muc nay thi MoveTo not doan cuoi
int			AutoGoTo(int tx, int ty);					// 1 dang di bang GoTo, 0 da gan toi (<= AUTO_GOTO_NEAR), -1 GoTo ket (dung MoveTo)
