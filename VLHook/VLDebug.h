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
BOOL		DebugRecoverCmd(const DEBUG_PARAM *pParam);
BOOL		DebugLowCfgCmd(const DEBUG_PARAM *pParam);
void		DebugLowCfgBreathe();						// Giu tat hinh + tu giam RAM (200ms)
void		DebugLowCfgUnhook();						// Tra lai FPS/hinh/nguoi choi truoc khi FreeLibrary
BOOL		DebugPathCmd(const DEBUG_PARAM *pParam);
void		DebugPathBreathe();						// Tu ghi / chay thu MapPath (200ms)
BOOL		DebugPathKey(WPARAM wParam, LPARAM lParam);	// Phim tat MapPath (TRUE = da xu ly)
void		DebugPathUnhook();						// Go patch ve truoc khi FreeLibrary

// Tu di theo MapPath (VLDebugPath.cpp) - dung trong Auto khi xa phu khong co map can toi
BOOL		PathTravelStart(int nTarget, BOOL bUseMenu);	// TRUE = co lo trinh, bat dau di. bUseMenu: tinh ca menu xa phu dang mo
void		PathTravelBreathe();						// 200ms
BOOL		PathTravelActive();
void		PathTravelStop(const char *szReason = NULL);
void		PathTravelInfo(char *szInfo, int nMax);
void		PathNoteMove(int x, int y, const char *szSrc);	// Ghi lenh MoveTo de ve (nut Ve duong di)
int			PathShowWalk(int nOn);						// Bat/tat ve duong di (-1 dao)
void		PathWalkReset();							// Lan sau PathWalkTo lap lai duong
int			PathWalkTo(int tx, int ty);					// Di trong map theo MapPath: 1 dang di, 0 toi cuoi duong, -1 khong co MapPath

// Game tu tim duong (KPlayer::GoTo) - VLMove.cpp
#define AUTO_GOTO_NEAR	150								// Gan hon muc nay thi MoveTo not doan cuoi
int			AutoGoTo(int tx, int ty);					// 1 dang di bang GoTo, 0 da gan toi (<= AUTO_GOTO_NEAR), -1 GoTo ket (dung MoveTo)
