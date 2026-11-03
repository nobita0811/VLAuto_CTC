#include "stdafx.h"
#include "../GameDef.h"
#include "GameBase.h"
#include "JxCore.h"
#include "S3Client.h"
#include "VLClasses.h"
#include "VLDebug.h"

extern BOOL g_SysPause;

// Tab Nhiem vu cua cua so DEBUG: nhiem vu Da Tau ban CTC (15_10).
// Server dieu khien hoan toan (client khong nap lua, task value 1020..1046 khong doi khi nhan / huy),
// noi dung nhiem vu chi co trong chu: hop thoai NPC (CoreDataChanged id 64 -> KUiMsgSel2) va
// thong bao he thong (id 23, n = 1: "nhan duoc nhiem vu: ...", "Da huy nhiem vu: ...").
// NPC: npcs.txt dong 108 "Bach Bao Tau" (Kind 3, enemy081), server dat ten "Da tau"; chi co o Tuong Duong.
// Chuoi TCVN3 viet bang escape hex (file ASCII), so sanh khong phan biet hoa thuong qua Fold().

#define YS_NPC_SETTING		108		// <link=image[0,8]:#npcspr:?NPCSID=108?...> trong loi thoai
#define CDC_SYSMSG			23		// CoreDataChanged: thong bao he thong (uParam = chuoi, n = 1)
#define CDC_QUESTION		64		// CoreDataChanged: hop thoai NPC (uParam = KUiQuestionAndAnswer*)
#define CDC_AFTERCANCEL		161		// thay ngay sau khi huy nhiem vu (u = 1)
#define QW_SYSMSG_ID		1000	// id gia: SystemMessageArrival, 1000 + kenh (1000 = khung thong bao duoi, 1001 = dong cua Auto)
#define QW_CHANMSG_ID		2000	// id gia: ChannelMessageArrival (tin kenh chat tu server: Cong bo...), uParam = nChannelID
#define QW_MAX				64		// so thong bao giu lai (vong tron)
#define QW_TEXT				1024

// Tu khoa (chu thuong, TCVN3)
static const char* const KW_CURQUEST = TV(L"nhiệm vụ hiện tại");				// nhiem vu hien tai
static const char* const KW_KILL = TV(L"tiêu diệt");							// tieu diet
static const char* const KW_KILL2 = TV(L"tiệu diệt");							// "Tieu diet" server CTC viet sai: Ti\xd6u (cung do dai KW_KILL)
static const char* const KW_AT = TV(L" ở ");									// " o "
static const char* const KW_ACCEPTED = TV(L"tiếp nhận");							// (So nhiem vu) tiep nhan: 1/10
static const char* const KW_TODAY = TV(L"đã nhận");							// (hom nay) da nhan: 0 / 10
static const char* const KW_DONE = TV(L"hoàn thành");							// hoan thanh
static const char* const KW_ROUND = TV(L"vòng");									// [Vong 3]
static const char* const KW_YESOU = TV(L"dã tẩu");								// da tau
static const char* const KW_GOT = TV(L"nhận được nhiệm vụ");		// nhan duoc nhiem vu
static const char* const KW_CANCELED = TV(L"đã hủy nhiệm vụ");				// da huy nhiem vu
static const char* const KW_NOTDONE = TV(L"chưa hoàn thành");				// chua hoan thanh

// Dong menu cua NPC Da Tau (Idx cua nut "Chon menu Da Tau")
enum { ysm_get, ysm_finish, ysm_cancel, ysm_view, ysm_close, ysm_num };
static const char *s_szMenuKey[ysm_num] =
{
	TV(L"nhận nhiệm vụ"),					// Nhan nhiem vu
	TV(L"hoàn thành nhiệm vụ"),			// Hoan thanh nhiem vu thu N
	TV(L"hủy nhiệm vụ"),					// Huy nhiem vu
	TV(L"xem tình hình"),						// Xem tinh hinh chuoi nhiem vu
	TV(L"kết thúc"),						// Ket thuc doi thoai!
};
static const char *s_szMenuName[ysm_num] = { "Nhan nhiem vu", "Hoan thanh", "Huy nhiem vu", "Xem tinh hinh", "Ket thuc" };

// Ket qua phan tich nhiem vu gan nhat (tu hop thoai hoac thong bao)
struct YS_QUEST_INFO
{
	char	szQuest[160];		// "Tieu diet 50 Tuyet anh o Chan Nui Truong bach"
	int		nKill;				// so quai can diet (0 = khong phai nhiem vu diet quai / chua biet)
	char	szMonster[48];
	char	szMap[64];
	int		nMapID;				// theo Maps\MapNames.ini, 0 = khong thay
	int		nKilled;			// tien do "Nhiem vu Da Tau -Tieu diet: a / b" (kenh Cong bo), -1 = chua biet
	int		nKillNeed;
	DWORD	dwKillTick;			// luc nhan tien do gan nhat
	int		nAccepted;			// So nhiem vu tiep nhan / hom nay da nhan: a / b
	int		nAcceptMax;
	int		nDone;				// So nhiem vu hoan thanh (-1 = chua biet)
	int		nRound;				// [Vong N] (0 = chua biet)
	int		nRoundDone;
	char	szEvent[24];		// su kien cuoi: nhan / huy / chua xong
	char	szSource[24];		// lay tu dau
	DWORD	dwTick;
};
static YS_QUEST_INFO	s_Quest;
static BOOL				s_bQuestInit = FALSE;

static void ResetQuestInfo()
{
	memset(&s_Quest, 0, sizeof(s_Quest));
	s_Quest.nAccepted = s_Quest.nAcceptMax = s_Quest.nDone = s_Quest.nRoundDone = -1;
	s_Quest.nKilled = s_Quest.nKillNeed = -1;
	s_bQuestInit = TRUE;
}

static unsigned char Fold(unsigned char c)
{
	if (c >= 'A' && c <= 'Z') return c + 32;
	if (c >= 0xA1 && c <= 0xA7) return c + 7;	// TCVN3: A( A^ E^ O^ O+ U+ D- -> chu thuong
	return c;
}

static const char* FindFold(const char *szText, const char *szKey)
{
	if (!szText || !szKey[0]) return NULL;
	for (const char *p = szText; *p; p++)
	{
		int i = 0;
		while (szKey[i] && Fold(p[i]) == Fold(szKey[i])) i++;
		if (!szKey[i]) return p;
	}
	return NULL;
}

// TCVN3 -> chu cai goc khong dau, chu thuong (so ten quai: bo qua dau thanh / hoa thuong / dau mu)
static unsigned char BaseChar(unsigned char c)
{
	if (c >= 'A' && c <= 'Z') return c + 32;
	if (c < 0xA1) return c;
	if (c == 0xA1 || c == 0xA2 || c == 0xA8 || c == 0xA9 || (c >= 0xB5 && c <= 0xB9) || (c >= 0xBB && c <= 0xBE) || (c >= 0xC6 && c <= 0xCB)) return 'a';
	if (c == 0xA7 || c == 0xAE) return 'd';
	if (c == 0xA3 || c == 0xAA || (c >= 0xCC && c <= 0xD6)) return 'e';
	if (c >= 0xD7 && c <= 0xDE) return 'i';
	if (c == 0xA4 || c == 0xA5 || c == 0xAB || c == 0xAC || (c >= 0xDF && c <= 0xEE)) return 'o';
	if (c == 0xA6 || c == 0xAD || (c >= 0xEF && c <= 0xF9)) return 'u';
	if (c >= 0xFA && c <= 0xFE) return 'y';
	return c;
}

// Chuoi -> chu goc, bo ma mau / ky tu dieu khien, gop khoang trang
static void BaseName(const char *szIn, char *szOut, int nOut)
{
	int n = 0;
	for (const unsigned char *p = (const unsigned char*)szIn; *p && n < nOut - 1; p++)
	{
		if (*p == 0x02) { if (p[1] && p[2] && p[3]) p += 3; else break; continue; }
		if (*p < ' ') continue;
		unsigned char c = BaseChar(*p);
		if (c == ' ' && (n == 0 || szOut[n - 1] == ' ')) continue;
		szOut[n++] = c;
	}
	while (n > 0 && szOut[n - 1] == ' ') n--;
	szOut[n] = 0;
}

// Ten quai co chua ten nhiem vu khong (khong phan biet dau, hoa thuong)
static BOOL NameMatch(const char *szNpcName, const char *szKill)
{
	char szA[64], szB[64];
	BaseName(szNpcName, szA, sizeof(szA));
	BaseName(szKill, szB, sizeof(szB));
	return szB[0] && strstr(szA, szB) != NULL;
}

static BOOL EqualFold(const char *a, const char *b)
{
	while (*a && Fold(*a) == Fold(*b)) { a++; b++; }
	return Fold(*a) == Fold(*b);
}

// Chuoi game -> chu tron: bo ma mau (0x02 r g b / 0x03), the <color=..> <link=..> <enter>..., xuong dong -> ' ', gop khoang trang
static int PlainText(const char *szText, int nMax, char *szOut, int nOut)
{
	int nLen = GameTextLen(szText, nMax);
	int n = 0;
	for (int i = 0; i < nLen && n < nOut - 1; i++)
	{
		unsigned char c = (unsigned char)szText[i];
		if (c == 0x02) { i += 3; continue; }
		if (c == 0x03) continue;
		if (c == '<')
		{
			unsigned char c1 = (unsigned char)szText[i + 1];
			if ((c1 >= 'a' && c1 <= 'z') || (c1 >= 'A' && c1 <= 'Z') || c1 == '/')
			{
				int nScan = nLen - i < 160 ? nLen - i : 160;
				const char *pEnd = (const char*)memchr(szText + i, '>', nScan);
				if (pEnd)
				{
					i = (int)(pEnd - szText);
					continue;
				}
			}
		}
		if (c < 0x20) c = ' ';
		if (c == ' ' && (n == 0 || szOut[n - 1] == ' ')) continue;
		szOut[n++] = (char)c;
	}
	while (n > 0 && szOut[n - 1] == ' ') n--;
	szOut[n] = 0;
	return n;
}

static const char* SkipSpace(const char *p)
{
	while (*p == ' ') p++;
	return p;
}

// Chep [p, pEnd) bo khoang trang 2 dau
static void CopyTrim(char *szOut, int nOut, const char *p, const char *pEnd)
{
	while (p < pEnd && *p == ' ') p++;
	while (pEnd > p && pEnd[-1] == ' ') pEnd--;
	int n = (int)(pEnd - p);
	if (n > nOut - 1) n = nOut - 1;
	if (n < 0) n = 0;
	memcpy(szOut, p, n);
	szOut[n] = 0;
}

// Sau tu khoa: ": a / b" hoac ": a" (tra ve so so doc duoc)
static int ParseNumbers(const char *p, int *pA, int *pB)
{
	p = SkipSpace(p);
	if (*p != ':') return 0;
	p = SkipSpace(p + 1);
	if (*p < '0' || *p > '9') return 0;
	*pA = atoi(p);
	while (*p >= '0' && *p <= '9') p++;
	p = SkipSpace(p);
	if (*p != '/' || !pB) return 1;
	p = SkipSpace(p + 1);
	if (*p < '0' || *p > '9') return 1;
	*pB = atoi(p);
	return 2;
}

// Maps\MapNames.ini [Map_Name]: tim ID theo ten (CVLQuest::FindMapID - ca ten map server ghi sai)
static int FindMapID(const char *szName, char *szFound, int nFound, BOOL bPrint)
{
	return theQuest.FindMapID(szName, szFound, nFound, bPrint);
}

// Tim "Tieu diet N <quai> o <map>" + so nhiem vu trong chuoi da bo ma; tra ve TRUE neu thay gi do
static BOOL ParseQuest(const char *szPlain, const char *szSource)
{
	if (!s_bQuestInit) ResetQuestInfo();
	BOOL bAny = FALSE;

	const char *p = FindFold(szPlain, KW_KILL);
	if (!p) p = FindFold(szPlain, KW_KILL2);
	if (p)
	{
		int nA = 0, nB = 0;
		if (ParseNumbers(p + strlen(KW_KILL), &nA, &nB) == 2)
		{
			// "Nhiem vu Da Tau -Tieu diet: 1 / 50" (server gui moi lan diet 1 con dung ten, kenh Cong bo)
			s_Quest.nKilled = nA;
			s_Quest.nKillNeed = nB;
			s_Quest.dwKillTick = GetTickCount();
			if (s_Quest.nKill <= 0) s_Quest.nKill = nB;
			strcpy(s_Quest.szEvent, nA >= nB ? "diet du quai" : "tien do diet quai");
			strncpy(s_Quest.szSource, szSource, sizeof(s_Quest.szSource) - 1);
			s_Quest.dwTick = GetTickCount();
			return TRUE;
		}
		const char *q = SkipSpace(p + strlen(KW_KILL));
		int nKill = atoi(q);
		while (*q >= '0' && *q <= '9') q++;
		const char *pAt = FindFold(q, KW_AT);
		if (nKill > 0 && pAt)
		{
			const char *pMap = pAt + strlen(KW_AT);
			const char *pEnd = pMap;
			while (*pEnd && *pEnd != '.' && *pEnd != '!' && *pEnd != ',' && *pEnd != ';' && *pEnd != '(' && *pEnd != '[') pEnd++;
			s_Quest.nKill = nKill;
			CopyTrim(s_Quest.szMonster, sizeof(s_Quest.szMonster), q, pAt);
			CopyTrim(s_Quest.szMap, sizeof(s_Quest.szMap), pMap, pEnd);
			CopyTrim(s_Quest.szQuest, sizeof(s_Quest.szQuest), p, pEnd);
			s_Quest.nMapID = FindMapID(s_Quest.szMap, NULL, 0, FALSE);
			bAny = TRUE;
		}
	}
	else if ((p = FindFold(szPlain, KW_CURQUEST)) != NULL)
	{
		// Nhiem vu hien tai khong phai diet quai: giu nguyen chu de xem
		const char *q = SkipSpace(p + strlen(KW_CURQUEST));
		if (*q == ':') q++;
		const char *pEnd = q;
		while (*pEnd && *pEnd != '.' && *pEnd != '!') pEnd++;
		CopyTrim(s_Quest.szQuest, sizeof(s_Quest.szQuest), q, pEnd);
		s_Quest.nKill = 0;
		s_Quest.szMonster[0] = s_Quest.szMap[0] = 0;
		s_Quest.nMapID = 0;
		bAny = TRUE;
	}

	if ((p = FindFold(szPlain, KW_ACCEPTED)) != NULL && ParseNumbers(p + strlen(KW_ACCEPTED), &s_Quest.nAccepted, &s_Quest.nAcceptMax))
		bAny = TRUE;
	else if ((p = FindFold(szPlain, KW_TODAY)) != NULL && ParseNumbers(p + strlen(KW_TODAY), &s_Quest.nAccepted, &s_Quest.nAcceptMax))
		bAny = TRUE;

	if ((p = FindFold(szPlain, KW_ROUND)) != NULL)
	{
		const char *q = SkipSpace(p + strlen(KW_ROUND));
		if (*q >= '0' && *q <= '9') { s_Quest.nRound = atoi(q); bAny = TRUE; }
	}

	// "So nhiem vu hoan thanh: 0." / "Tong so nhiem vu da hoan thanh: 0 lan" / "[Vong 3] da hoan thanh: 0 lan"
	for (p = FindFold(szPlain, KW_DONE); p; p = FindFold(p + 1, KW_DONE))
	{
		int nVal = -1;
		if (!ParseNumbers(p + strlen(KW_DONE), &nVal, NULL)) continue;
		BOOL bRound = FALSE;
		for (const char *b = p - 1; b >= szPlain && b >= p - 12; b--)
			if (*b == ']') { bRound = TRUE; break; }
		if (bRound) s_Quest.nRoundDone = nVal;
		else s_Quest.nDone = nVal;
		bAny = TRUE;
	}

	if (FindFold(szPlain, KW_GOT)) { strcpy(s_Quest.szEvent, "nhan nhiem vu"); s_Quest.nKilled = 0; s_Quest.nKillNeed = s_Quest.nKill; bAny = TRUE; }
	else if (FindFold(szPlain, KW_CANCELED)) { strcpy(s_Quest.szEvent, "da huy"); s_Quest.nKilled = s_Quest.nKillNeed = -1; bAny = TRUE; }
	else if (FindFold(szPlain, KW_NOTDONE)) { strcpy(s_Quest.szEvent, "chua hoan thanh"); bAny = TRUE; }

	if (bAny)
	{
		strncpy(s_Quest.szSource, szSource, sizeof(s_Quest.szSource) - 1);
		s_Quest.dwTick = GetTickCount();
	}
	return bAny;
}

static void OutQuestInfo()
{
	if (!s_bQuestInit || !s_Quest.dwTick)
	{
		DbgOut("Nhiem vu: chua phan tich (bam Buoc 1, hoac bat 'Theo doi thong bao' roi noi chuyen voi Da Tau)\n");
		return;
	}
	DbgOut("Nhiem vu (tu %s, %u giay truoc):\n", s_Quest.szSource, (GetTickCount() - s_Quest.dwTick) / 1000);
	DbgOut("  Noi dung : %s\n", s_Quest.szQuest[0] ? s_Quest.szQuest : "?");
	if (s_Quest.nKill)
		DbgOut("  Diet quai: %d x '%s'  tai '%s' (MapID %d%s)\n", s_Quest.nKill, s_Quest.szMonster, s_Quest.szMap,
			s_Quest.nMapID, s_Quest.nMapID ? (s_Quest.nMapID == *g_pMapID ? ", DANG O DAY" : "") : " - khong thay trong MapNames.ini");
	if (s_Quest.nKilled >= 0)
		DbgOut("  Tien do  : %d / %d%s%s\n", s_Quest.nKilled, s_Quest.nKillNeed,
			s_Quest.nKillNeed > 0 && s_Quest.nKilled >= s_Quest.nKillNeed ? " -> DU, ve Da Tau hoan thanh" : "",
			s_Quest.dwKillTick ? "" : " (chua co thong bao diet quai)");
	if (s_Quest.nAccepted >= 0)
		DbgOut("  Da nhan  : %d / %d\n", s_Quest.nAccepted, s_Quest.nAcceptMax);
	if (s_Quest.nDone >= 0)
		DbgOut("  Hoan thanh: %d\n", s_Quest.nDone);
	if (s_Quest.nRound)
		DbgOut("  Vong %d, da hoan thanh: %d\n", s_Quest.nRound, s_Quest.nRoundDone);
	if (s_Quest.szEvent[0])
		DbgOut("  Su kien cuoi: %s\n", s_Quest.szEvent);
}

//============================== Theo doi CoreDataChanged ==============================

struct QW_MSG
{
	DWORD	dwTick;
	DWORD	uId;
	DWORD	uParam;
	int		nParam;
	int		nCount;
	char	szText[QW_TEXT];
};
static QW_MSG	s_QMsg[QW_MAX];
static int		s_nQNext = 0;
static int		s_nQTotal = 0;
static BOOL		s_bWatchOn = FALSE;
static BOOL		s_bWatchAll = FALSE;
static DWORD	s_dwWatchStart = 0;

static BOOL LooksLikeText(const char *p)
{
	for (int i = 0; i < 4; i++)
		if ((BYTE)p[i] < 0x20 && p[i] != 0x02 && p[i] != 0x03) return FALSE;
	return TRUE;
}

// Ghi vao vong tron; trung dong cuoi => chi tang so lan, tra ve FALSE
static BOOL AddWatch(DWORD uId, DWORD uParam, int nParam, const char *szText)
{
	QW_MSG *pLast = s_nQTotal ? &s_QMsg[(s_nQNext + QW_MAX - 1) % QW_MAX] : NULL;
	if (pLast && pLast->uId == uId && pLast->uParam == uParam && pLast->nParam == nParam && !strcmp(pLast->szText, szText))
	{
		pLast->nCount++;
		return FALSE;
	}
	QW_MSG &M = s_QMsg[s_nQNext];
	M.dwTick = GetTickCount() - s_dwWatchStart;
	M.uId = uId;
	M.uParam = uParam;
	M.nParam = nParam;
	M.nCount = 1;
	strncpy(M.szText, szText, QW_TEXT - 1);
	M.szText[QW_TEXT - 1] = 0;
	s_nQNext = (s_nQNext + 1) % QW_MAX;
	s_nQTotal++;
	return TRUE;
}

// Goi tu hook CoreDataChanged (VLDebugFile.cpp, luong game, trong __try)
void DebugQuestCoreData(DWORD uId, DWORD uParam, int nParam)
{
	if (!s_bWatchOn) return;
	BOOL bQuest = (uId == CDC_SYSMSG || uId == CDC_QUESTION || uId == CDC_AFTERCANCEL);
	if (!bQuest && (!s_bWatchAll || uId == 33 || uId == 79)) return;	// 33, 79: lien tuc, khong lien quan

	char szText[QW_TEXT];
	szText[0] = 0;
	if (uParam > 0x10000)
	{
		const char *p = (const char*)uParam;
		if (uId == CDC_QUESTION)
		{
			// KUiQuestionAndAnswer: loi thoai +0, so dong tra loi +0x804, dong tra loi +0x80C (0x104 / dong)
			int n = PlainText(p, 0x800, szText, sizeof(szText));
			int nAns = *(int*)(p + 0x804);
			for (int i = 0; i < nAns && i < 8 && n < (int)sizeof(szText) - 16; i++)
			{
				n += _snprintf(szText + n, sizeof(szText) - 1 - n, " | [%d] ", i);
				if (n < 0 || n >= (int)sizeof(szText) - 1) { n = (int)strlen(szText); break; }
				n += PlainText(p + 0x80C + i * 0x104, 0x100, szText + n, sizeof(szText) - n);
			}
		}
		else if (LooksLikeText(p))
			PlainText(p, 0x400, szText, sizeof(szText));
	}

	if (!AddWatch(uId, uParam, nParam, szText)) return;
	if (szText[0] && (uId == CDC_SYSMSG || uId == CDC_QUESTION || s_bWatchAll))
	{
		char szSrc[24];
		sprintf(szSrc, uId == CDC_QUESTION ? "hop thoai id %u" : "thong bao id %u", uId);
		ParseQuest(szText, szSrc);
	}
}

// Goi tu hook KClientCallback::ChannelMessageArrival (VLDebugFile.cpp, luong game, trong __try):
// tin kenh chat tu server, "Cong bo: Nhiem vu Da Tau -Tieu diet: 1 / 50" (tab Tat ca / Khac)
void DebugQuestChannelMsg(DWORD nChannelID, const char *szSend, const char *szMsg, int nLen)
{
	if (!s_bWatchOn || !szMsg || nLen <= 0) return;
	char szText[QW_TEXT];
	int n = 0;
	if (szSend && szSend[0])
	{
		n = _snprintf(szText, 40, "[%.32s] ", szSend);
		if (n < 0) n = 0;
	}
	PlainText(szMsg, nLen < 0x400 ? nLen : 0x400, szText + n, sizeof(szText) - n);
	if (!szText[0]) return;
	if (!AddWatch(QW_CHANMSG_ID, nChannelID, nLen, szText)) return;
	ParseQuest(szText, "kenh chat (server)");
}

// Goi tu hook KUiMsgCentrePad::SystemMessageArrival (VLDebugFile.cpp, luong game, trong __try):
// moi dong chu vao khung chat he thong (kenh 0 = khung thong bao duoi, kenh 1 = dong cua Auto)
static BOOL s_bStepMsg = FALSE;	// Dang bao tien trinh 5 buoc len kenh chat: khong ghi / phan tich lai

void DebugQuestSysMsg(const char *szMsg, int nLen, int nChannel)
{
	if (!s_bWatchOn || s_bStepMsg || !szMsg || nLen <= 0) return;
	char szText[QW_TEXT];
	PlainText(szMsg, nLen < 0x400 ? nLen : 0x400, szText, sizeof(szText));
	if (!szText[0]) return;
	if (!AddWatch(QW_SYSMSG_ID + nChannel, nLen, nChannel, szText)) return;
	char szSrc[24];
	sprintf(szSrc, "khung chat kenh %d", nChannel);
	ParseQuest(szText, szSrc);
}

static BOOL SaveWatchFile(char *szFile)
{
	sprintf(szFile, "%s\\Logs\\QuestLog.txt", theMain.m_szCurFolder);
	FILE *pFile = fopen(szFile, "wt");
	if (!pFile) return FALSE;
	int nNum = s_nQTotal < QW_MAX ? s_nQTotal : QW_MAX;
	fprintf(pFile, "# %d thong bao (tong %d). Cot: ms | id (1000 + kenh = khung chat) | uParam (khung chat: do dai) | nParam | so lan | chu (TCVN3)\n", nNum, s_nQTotal);
	for (int k = 0; k < nNum; k++)
	{
		QW_MSG &M = s_QMsg[(s_nQNext - nNum + k + QW_MAX) % QW_MAX];
		fprintf(pFile, "%7u\t%3u\t%08X\t%d\t%d\t%s\n", M.dwTick, M.uId, M.uParam, M.nParam, M.nCount, M.szText);
	}
	fclose(pFile);
	return TRUE;
}

// Bat hook theo doi (tien do diet quai chi co trong tin kenh chat server); FALSE = khong hook duoc
static BOOL EnsureWatch(BOOL bAll = FALSE)
{
	if (s_bWatchOn) return TRUE;
	s_nQNext = s_nQTotal = 0;
	s_bWatchAll = bAll;
	s_dwWatchStart = GetTickCount();
	s_bWatchOn = TRUE;
	if (DebugFileCoreDataHook(TRUE)) return TRUE;
	s_bWatchOn = FALSE;
	return FALSE;
}

static void WatchToggle(int nX)
{
	if (!s_bWatchOn)
	{
		if (!EnsureWatch(nX == 1))
		{
			DbgOut("LOI: khong hook duoc CoreDataChanged 0x409C20 / SystemMessageArrival 0x4BD680 / ChannelMessageArrival 0x40DA10\n");
			return;
		}
		DbgOut("=> BAT theo doi: kenh chat server (id 2000: Cong bo...), khung thong bao (id 1000 + kenh), CoreDataChanged id 23 (thong bao), 64 (hop thoai NPC), 161%s.\n"
			"Noi chuyen voi Da Tau / nhan / huy nhiem vu, roi bam 'Xem thong bao nhiem vu'.\n",
			s_bWatchAll ? " + MOI id khac (tru 33, 79)" : " (X = 1: ghi moi id)");
	}
	else
	{
		s_bWatchOn = FALSE;
		DebugFileCoreDataHook(FALSE);
		char szFile[MAX_PATH];
		DbgOut("=> TAT theo doi, %d thong bao\n", s_nQTotal);
		if (SaveWatchFile(szFile)) DbgOut("Da luu: %s\n", szFile);
	}
	g_DbgReply.nResult[0] = s_bWatchOn;
	g_DbgReply.bOK = TRUE;
}

static void WatchView(int nX, const char *szFilter)
{
	DbgOut("Theo doi: %s | %d thong bao\n", s_bWatchOn ? "DANG BAT" : "tat", s_nQTotal);
	OutQuestInfo();
	char szFile[MAX_PATH];
	if (s_nQTotal && SaveWatchFile(szFile)) DbgOut("Day du: %s\n", szFile);
	int nNum = s_nQTotal < QW_MAX ? s_nQTotal : QW_MAX;
	// Moi nhat truoc, cat moi dong 300 ky tu cho vua cua so
	for (int k = nNum - 1; k >= 0; k--)
	{
		QW_MSG &M = s_QMsg[(s_nQNext - nNum + k + QW_MAX) % QW_MAX];
		if (szFilter[0] && !FindFold(M.szText, szFilter)) continue;
		if (M.uId == QW_CHANMSG_ID) DbgOut("%6u chat %08X", M.dwTick, M.uParam);
		else if (M.uId >= QW_SYSMSG_ID) DbgOut("%6u kenh %d", M.dwTick, M.nParam);
		else DbgOut("%6u id %3u n=%d", M.dwTick, M.uId, M.nParam);
		if (M.nCount > 1) DbgOut(" x%d", M.nCount);
		DbgOut(": %.300s\n", M.szText[0] ? M.szText : (M.uParam <= 0x10000 ? "" : "(khong phai chu)"));
	}
	if (nX == 1)
	{
		s_nQNext = s_nQTotal = 0;
		s_dwWatchStart = GetTickCount();
		DbgOut("=> Da xoa\n");
	}
	g_DbgReply.bOK = TRUE;
}

//============================== NPC Da Tau ==============================

static BOOL IsYeSouNpc(KNpc *pNpc)
{
	return FindFold(pNpc->Name, KW_YESOU) != NULL ||
		(pNpc->m_Kind == kind_dialoger && pNpc->m_NpcSettingIdx == YS_NPC_SETTING);
}

// Tra ve idx NPC Da Tau gan nhat (0 = khong thay)
static int FindYeSou(int *pDist = NULL)
{
	int nPlayerX, nPlayerY, nNpcX, nNpcY, nBest = 0, nBestDist = 0x7FFFFFFF;
	P_PLAYER->GetMapPos(&nPlayerX, &nPlayerY);
	for (int i = 1; i < MAX_NPC; i++)
	{
		KNpc *pNpc = g_pNpcArray[i];
		if (!pNpc->IsExist() || i == g_pCurPlayer->m_nIndex || !IsYeSouNpc(pNpc)) continue;
		pNpc->GetMapPos(&nNpcX, &nNpcY);
		int dZ = __distance(nPlayerX, nPlayerY, nNpcX, nNpcY);
		if (dZ < nBestDist)
		{
			nBest = i;
			nBestDist = dZ;
		}
	}
	if (pDist) *pDist = nBestDist;
	return nBest;
}

//============================== Hop thoai ==============================

static int MenuKind(const char *szLine)
{
	char szPlain[256];
	PlainText(szLine, 255, szPlain, sizeof(szPlain));
	for (int k = 0; k < ysm_num; k++)
		if (FindFold(szPlain, s_szMenuKey[k])) return k;
	return -1;
}

static const char* MsgLine(KOneMsgInfo **ppMsg, int i)
{
	KOneMsgInfo *pMsg = ppMsg ? ppMsg[i] : NULL;
	return pMsg ? pMsg->Msg : "";
}

// Hop thoai NPC dang mo: KUiMsgSel2 (Da Tau ban CTC) hoac KUiMsgSel
struct YS_DIALOG
{
	KUiMsgSel2		*pSel2;
	KUiMsgSel		*pSel;
	int				nNum;
	KOneMsgInfo		**ppMsg;
	const char		*szPlain;	// loi thoai da bo ma (sau ReadDialog)
	BOOL			bYeSou;
};

static BOOL GetDialog(YS_DIALOG &D)
{
	memset(&D, 0, sizeof(D));
	D.pSel2 = (KUiMsgSel2*)GetPtrAddress(MSG2_BASE_ADD);
	if (D.pSel2 && !D.pSel2->IsVisible()) D.pSel2 = NULL;
	if (!D.pSel2)
	{
		D.pSel = (KUiMsgSel*)GetPtrAddress(MSG_BASE_ADD);
		if (D.pSel && !D.pSel->IsVisible()) D.pSel = NULL;
	}
	if (!D.pSel2 && !D.pSel) return FALSE;
	D.nNum = D.pSel2 ? D.pSel2->m_nNumMessage : D.pSel->m_nNumMessage;
	D.ppMsg = D.pSel2 ? D.pSel2->m_pMessages : D.pSel->m_pMessages;
	if (D.nNum > 16) D.nNum = 16;
	if (D.nNum < 0) D.nNum = 0;
	return TRUE;
}

// Bo ma loi thoai + phan tich nhiem vu (ParseQuest); bPrint: in loi thoai + cac dong menu ra cua so DEBUG
static void ReadDialog(YS_DIALOG &D, BOOL bPrint)
{
	static char szPlain[0x2000];
	const char *szSrc = D.pSel2 ? D.pSel2->GetQuestion() : D.pSel->m_InfoText;
	int nSrcMax = D.pSel2 ? (int)sizeof(D.pSel2->m_szQuestion) : (int)sizeof(D.pSel->m_InfoText);
	int nLen = PlainText(szSrc, nSrcMax, szPlain, sizeof(szPlain));
	D.szPlain = szPlain;
	D.bYeSou = FindFold(szPlain, KW_YESOU) || strstr(szSrc, "NPCSID=108");
	if (bPrint)
	{
		DbgOut("=== %s%s: loi thoai %d ky tu ===\n%.1500s\n", D.pSel2 ? "KUiMsgSel2" : "KUiMsgSel",
			D.bYeSou ? " (Da Tau)" : "", nLen, szPlain);
		for (int i = 0; i < D.nNum; i++)
		{
			int nKind = MenuKind(MsgLine(D.ppMsg, i));
			DbgOut("  [%d] %-14s %.120s\n", i, nKind >= 0 ? s_szMenuName[nKind] : "-", MsgLine(D.ppMsg, i));
		}
	}
	ParseQuest(szPlain, D.pSel2 ? "hop thoai KUiMsgSel2" : "hop thoai KUiMsgSel");
}

// Dong menu loai nKind (ysm_...), -1 = khong co
static int FindMenu(const YS_DIALOG &D, int nKind)
{
	for (int i = 0; i < D.nNum; i++)
		if (MenuKind(MsgLine(D.ppMsg, i)) == nKind) return i;
	return -1;
}

static void ClickMenu(const YS_DIALOG &D, int i)
{
	if (D.pSel2) D.pSel2->OnClickMsg(i);
	else D.pSel->OnClickMsg(i);
}

static KUiPrizeSel *GetPrizeSel()
{
	KUiPrizeSel *pPrzSel = (KUiPrizeSel*)GetPtrAddress(PZS_BASE_ADD);
	return (pPrzSel && pPrzSel->IsVisible()) ? pPrzSel : NULL;
}

static const char* PrizeKind(const char *szIcon)
{
	return strstr(szIcon, "jingyan_icon") ? "Kinh nghiem"
		: strstr(szIcon, "jinqian_icon") ? "Ngan luong"
		: strstr(szIcon, "wenhao_icon") ? "Ngau nhien"
		: strstr(szIcon, "quxiao_icon") ? "Co hoi huy" : "?";
}

// Bang "Phan thuong nhiem vu" (KUiPrizeSel 0x7CBD1C, sau khi chon Hoan thanh): 3 o anh + chu
static void PrintPrize(KUiPrizeSel *pPrzSel)
{
	DbgOut("Bang phan thuong 0x%08X dang hien:\n", (DWORD)pPrzSel);
	for (int i = 0; i < 3; i++)
	{
		PrizeItem &P = pPrzSel->PrzItem[i];
		char szText[0x200];
		PlainText(P.m_szText, sizeof(P.m_szText), szText, sizeof(szText));
		const char *szFile = strrchr(P.m_szIcon, '\\');
		DbgOut("  [%d] %-11s %s  (%s)\n", i, PrizeKind(P.m_szIcon), szText, szFile ? szFile + 1 : P.m_szIcon);
	}
}

// Bang thong bao nut OK (g_UiInformation2): chu da bo ma, "" = khong hien
static const char* InfoText()
{
	static char szText[256];
	szText[0] = 0;
	if (g_UiInformation2 && g_UiInformation2->IsVisible())
		PlainText(g_UiInformation2->m_szInfoText, sizeof(g_UiInformation2->m_szInfoText), szText, sizeof(szText));
	return szText;
}

static BOOL IsTown()
{
	return !P_PLAYER->m_FightMode;
}

// So quai ten chua szName quanh nhan vat (con song); pNear / pNearDist: con gan nhat
static int CountMonsters(const char *szName, int *pNear, int *pNearDist)
{
	int nPlayerX, nPlayerY, nNpcX, nNpcY, nCount = 0, nBest = 0, nBestDist = 0x7FFFFFFF;
	P_PLAYER->GetMapPos(&nPlayerX, &nPlayerY);
	for (int i = 1; i < MAX_NPC; i++)
	{
		KNpc *pNpc = g_pNpcArray[i];
		if (!pNpc->IsExist() || i == g_pCurPlayer->m_nIndex || pNpc->m_Kind != kind_normal) continue;
		if (pNpc->m_CurrentLife <= 0 || pNpc->m_Doing == do_death || !NameMatch(pNpc->Name, szName)) continue;
		nCount++;
		pNpc->GetMapPos(&nNpcX, &nNpcY);
		int dZ = __distance(nPlayerX, nPlayerY, nNpcX, nNpcY);
		if (dZ < nBestDist)
		{
			nBest = i;
			nBestDist = dZ;
		}
	}
	if (pNear) *pNear = nBest;
	if (pNearDist) *pNearDist = nBestDist;
	return nCount;
}

//============================== 5 buoc lam nhiem vu ==============================
// Bam "Buoc N": kiem tra + bat dau, bam lai: dung. DebugQuestBreathe (200ms, ca khi tam dung Auto) lam tung viec
// cach nhau 600ms, bao tien trinh len kenh chat (dong cua Auto) va ghi nhat ky (nut Trang thai).
// 3 nut: Buoc 1 nhan nhiem vu (qs_get) -> Buoc 2 DoQuestKillNpc (qs_shop -> qs_gomap -> qs_kill) -> Buoc 3 tra nhiem vu (qs_return).
// Xong buoc nay tu chuyen buoc sau (s_bChain, X = 2 thi chi lam 1 buoc). Moi giai doan tu lam ca khi Auto tam dung,
// tru qs_kill can Auto chay (Chien dau) - chi loc quai + theo doi tien do.

enum { qs_none, qs_get, qs_shop, qs_gomap, qs_kill, qs_return, qs_home, qs_num };
static const char *s_szStepName[qs_num] = { "", "Buoc 1 (nhan nhiem vu)", "Buoc 2 (diet quai: cat do/mua mau/mana/TDP)",
	"Buoc 2 (diet quai: di toi map)", "Buoc 2 (diet quai: danh quai)", "Buoc 3 (tra nhiem vu)", "Ve Tuong Duong (bat lai Tu lam nhiem vu)" };
static int StepButton(int nStep) { return nStep == qs_get ? 1 : nStep == qs_return ? 3 : nStep == qs_home ? 0 : nStep ? 2 : 0; }
static BOOL		s_bChain = FALSE;	// Xong buoc thi tu chuyen nut sau (1 -> 2 -> 3)
static int		s_nPrize = 0;		// O phan thuong Buoc 3 (0 trai = Kinh nghiem, 1 giua = Ngan luong, 2 phai = Ngau nhien)
static BOOL StepStart(int nStep);
static void StepNext(int nDone);
static const int s_nStepTimeout[qs_num] = { 0, 180, 900, 1200, 7200, 900, 900 };	// giay (Buoc 2 mua: ca cat do vao ruong)

#define QS_LOG		32
#define QS_LINE		200

static int		s_nStep = qs_none;
static int		s_nStage = 0;
#define QS_SHOP_MONEY	2			// Buoc 2, giai doan rut tien
#define QS_SHOP_SELL	3			// Buoc 2, giai doan ban do
#define QS_SHOP_PLAN	4			// Buoc 2, giai doan xem can mua gi
#define QS_SHOP_SAVE	5			// Buoc 2, giai doan cat tien sau khi mua ("Gui tien" tab Ban do)
static int		s_nShopSold = 0;		// Buoc 2: so lan ban do
#define QS_MONEY_DEF	50000		// Tien can tren nguoi khi tab Ban do chua dat "Rut tien" (van)
#define QS_DRIVER_MONEY	500			// Tien xa phu: cat tien van giu lai, thieu thi rut truoc khi di
static int		s_nShopMoney = -1;		// Tien tren nguoi luc gui lenh rut (-1 = chua rut)
static int		s_nShopSave = -1;		// Tien tren nguoi luc gui lenh cat (-1 = chua cat)
static BOOL		s_bShopMedOnly = FALSE;	// Mua lai thuoc (Refill het thuoc): bo qua cat ruong / ban do
static DWORD	s_dwMedShopKey = 0;		// Lan ve thanh da quay lai mua thuoc (s_dwQuestStart + s_nTownTrips + 1)
static BOOL		s_bSaveAtBox = FALSE;	// Cat tien lan 1 khong duoc: da toi ruong mo ruong roi cat lai
static int		s_nShopBase = -1;		// Tien tren nguoi truoc lan rut dau o Buoc 2 nay (-1 = chua rut) - khong bat Gui tien thi cat ve muc nay
static char		s_szShopDone[200] = "";	// Loi bao xong Buoc 2 (bao sau khi cat tien)
static DWORD	s_dwStepStart = 0;
static DWORD	s_dwStageTick = 0;
static int		s_nTick = 0;
static int		s_nTarget = 0;		// Buoc 3: map dich; Buoc 5: o phan thuong
static int		s_nClicks = 0;		// so lan bam dong menu (Nhan / Hoan thanh)
static DWORD	s_dwClickTick = 0;
static int		s_nNpcClicks = 0;
static BOOL		s_bUsedTDP = FALSE;
static DWORD	s_dwWaitMapTick = 0;
static int		s_nWaitMap = 0;
static BOOL		s_bTravelStarted = FALSE;
static int		s_nCityMap = 0;		// map da nap theMap.LoadMap (muc [NPC] cua MapPath)
static DWORD	s_dwReportTick = 0;
static DWORD	s_dwNoMonsterTick = 0;
static BOOL		s_bNoMonsterSaid = FALSE;
static char		s_szKillName[48] = "";	// Buoc 4: chi danh quai ten chua chuoi nay (KAliveNpc::RefreshNpcs)
static char		s_StepLog[QS_LOG][QS_LINE];
static int		s_nLogNext = 0, s_nLogTotal = 0;
static char		s_szLastDialog[0x800] = "";	// hop thoai Da Tau chua biet lam gi (Buoc 1 / 5 dung lai)
static int		s_nHoldMap = 0;		// Buoc 3 / 4 xong o map nay: CVLMove dung (khong di theo danh sach toa do) toi khi bat buoc khac
static BOOL		s_bAutoQuest = FALSE;	// Dang chay do Auto goi (QuestKillStart), khong phai nut bam
static int		s_nAutoResult = qk_idle;

// Auto diet quai: cai dat map nhiem vu (Maps\MapDaTau.ini, tab Danh quai cua nhan vat de len)
static KQMapInfo s_KQ = { 0 };		// Map dang lam (nMapID = 0: chua nap)
static int		s_nTownTrips = 0;		// So lan ve thanh giua chung nhiem vu nay (het mau / day tui / chet)
static DWORD	s_dwQuestStart = 0;		// Luc Auto bat dau nhiem vu nay (kiem tra dai han)
static DWORD	s_dwLongCheckTick = 0;	// Lan kiem tra dinh ky truoc
static int		s_nLongKilled = -1;		// Tien do luc kiem tra truoc
static BOOL		s_bProbed = FALSE;		// Nhiem vu nay da ve bam Hoan thanh thu it nhat 1 lan
static BOOL		s_bProbe = FALSE;		// Buoc 5 do kiem tra dinh ky: ve bam Hoan thanh thu (server co the khong bao tien do)
#define QS_PROBE_VIEW	2			// Buoc 5 (s_bProbe), giai doan: chua xong -> dong cua so, bam lai Da Tau
#define QS_PROBE_WAIT	3			//   da chon "Xem tinh hinh": cho thong bao / hop thoai
static int		s_nGoTry = 0;			// Buoc 3: lan thu (vat pham / xa phu / di bo)
static BOOL		s_bRoamList = FALSE;	// Buoc 4: di lan luot danh sach diem MapDaTau.ini thay vi MapPath
static int		s_nKillRange = 0;		// Buoc 4: chi danh quai cach nhan vat <= muc nay (0 = tam tim Auto)
static BOOL		s_bHomeItem = FALSE;	// GoYeSou da thu vat pham ve Tuong Duong
static DWORD	s_dwTripCheck = 0;		// Buoc 4: lan kiem tra tui day / het thuoc truoc
#define QS_SHOP_REPO	6			// Buoc 2, giai doan cat do vao ruong (Ruong= trong MapDaTau.ini)
#define KQ_MAX_SECT		60

static void KQParsePoints(const char *szText, KQMapInfo *pInfo)
{
	const char *p = szText;
	while (*p && pInfo->nPoints < MAX_KQ_POINT)
	{
		int x, y;
		if (sscanf(p, "%d,%d", &x, &y) == 2 && (x || y))
		{
			pInfo->Pts[pInfo->nPoints].x = x;
			pInfo->Pts[pInfo->nPoints].y = y;
			pInfo->nPoints++;
		}
		p = strchr(p, ';');
		if (!p) break;
		p++;
	}
}

// Nap cai dat map nMapID: muc [n] trong Maps\MapDaTau.ini co Map=<nMapID>_<ten>, roi de cai dat rieng nhan vat
// (theQuest.m_KillQOpt). TRUE = map co trong ini
static BOOL KQLoad(int nMapID, KQMapInfo *pInfo)
{
	memset(pInfo, 0, sizeof(*pInfo));
	pInfo->nMapID = nMapID;
	if (nMapID <= 0) return FALSE;
	char szFile[MAX_PATH], szSect[8], szBuf[4096];
	_snprintf(szFile, sizeof(szFile) - 1, "%s\\Maps\\MapDaTau.ini", theMain.m_szCurFolder);
	szFile[sizeof(szFile) - 1] = 0;
	for (int n = 1; n <= KQ_MAX_SECT; n++)
	{
		sprintf(szSect, "%d", n);
		if (!GetPrivateProfileStringA(szSect, "Map", "", szBuf, sizeof(szBuf), szFile)) continue;
		if (atoi(szBuf) != nMapID) continue;
		pInfo->bInList = TRUE;
		GetPrivateProfileStringA(szSect, "Ruong", "", szBuf, sizeof(szBuf), szFile);
		if (sscanf(szBuf, "%d,%d,%d", &pInfo->nRepoMap, &pInfo->nRepoX, &pInfo->nRepoY) != 3 || !pInfo->nRepoX || !pInfo->nRepoY)
			pInfo->nRepoMap = pInfo->nRepoX = pInfo->nRepoY = 0;
		pInfo->nRange = GetPrivateProfileIntA(szSect, "AttackRange", 0, szFile);
		GetPrivateProfileStringA(szSect, "Points", "", szBuf, sizeof(szBuf), szFile);
		KQParsePoints(szBuf, pInfo);
		break;
	}
	const YSKillQOpt &O = theQuest.m_KillQOpt;
	for (int i = 0; i < O.m_nMaps && i < MAX_KQ_MAP; i++)
	{
		const KQMapOpt &M = O.m_Maps[i];
		if (M.nMapID != nMapID) continue;
		pInfo->bDisable = M.bDisable;
		if (M.nFlags & KQF_RANGE) pInfo->nRange = M.nRange;
		if (M.nFlags & KQF_POINTS)
		{
			pInfo->nPoints = M.nPoints < 0 ? 0 : M.nPoints > MAX_KQ_POINT ? MAX_KQ_POINT : M.nPoints;
			memcpy(pInfo->Pts, M.Pts, pInfo->nPoints * sizeof(POINT));
		}
		break;
	}
	return pInfo->bInList;
}

int QuestKillMapCheck(int nMapID)
{
	KQMapInfo Info;
	if (!KQLoad(nMapID, &Info)) return kqm_nomap;
	return Info.bDisable ? kqm_disable : kqm_ok;
}

// Buoc 4: di vong cac diem MapPath cua map (Maps\Path\<MapID>_*.txt), khong dung danh sach toa do tab Di chuyen
#define QS_ROAM_MAX		512
#define QS_ROAM_NEAR	200		// Diem cach nhan vat < muc nay coi nhu da qua
static POINT	s_RoamPt[QS_ROAM_MAX];
static BYTE		s_RoamDone[QS_ROAM_MAX];
static int		s_nRoamPts = 0;
static int		s_nRoamIdx = -1;	// Diem dang toi
static DWORD	s_dwRoamTick = 0;
static int		s_nRoamRounds = 0;
static int		s_nRoamDoorSkip = 0;	// So diem MapPath bo vi gan cong

// Buoc 4 / giu dung: KPlayer::MoveTo / GoTo chi chay khi goi tu Chien dau, Nhat do, tab Nhiem vu (CQuestMoveAllow)
static int		s_nMoveAllow = 0;
static int		s_nMoveBlocked = 0;		// So lenh di cua phan khac da chan
static DWORD	s_dwMoveBlockAddr = 0;	// Dia chi goi lenh bi chan gan nhat (VLCTCdll + offset)

static void SetStage(int nStage)
{
	s_nStage = nStage;
	s_dwStageTick = GetTickCount();
}

static int StageSec()
{
	return (int)((GetTickCount() - s_dwStageTick) / 1000);
}

static void StepMsg(const char *szFormat, ...)
{
	char szMsg[QS_LINE - 8];
	va_list va;
	va_start(va, szFormat);
	_vsnprintf(szMsg, sizeof(szMsg) - 1, szFormat, va);
	va_end(va);
	szMsg[sizeof(szMsg) - 1] = 0;
	_snprintf(s_StepLog[s_nLogNext], QS_LINE - 1, "%4us %s", (GetTickCount() - s_dwStepStart) / 1000, szMsg);
	s_StepLog[s_nLogNext][QS_LINE - 1] = 0;
	s_nLogNext = (s_nLogNext + 1) % QS_LOG;
	s_nLogTotal++;
	s_bStepMsg = TRUE;
#if ADMIN_MODE
	SystemMessage(1, "<color=yellow>NV: %s", szMsg);	// Ban User: tat thong bao NV tren kenh chat (van luu s_StepLog noi bo)
#endif
	s_bStepMsg = FALSE;
}

static void StepEnd(BOOL bOK, const char *szFormat, ...)
{
	if (!s_nStep) return;
	char szMsg[QS_LINE - 40];
	va_list va;
	va_start(va, szFormat);
	_vsnprintf(szMsg, sizeof(szMsg) - 1, szFormat, va);
	va_end(va);
	szMsg[sizeof(szMsg) - 1] = 0;
	StepMsg("%s %s: %s", s_szStepName[s_nStep], bOK ? "XONG" : "DUNG", szMsg);
	if (!bOK && PathTravelActive() && (s_nStep == qs_get || s_nStep == qs_gomap || s_nStep == qs_return || s_nStep == qs_home))
		PathTravelStop("tab Nhiem vu dung");
	if (s_nStep == qs_home && !bOK)
	{
		char szLog[QS_LINE];
		_snprintf(szLog, sizeof(szLog) - 1, TV(L"Bật lại Tự làm nhiệm vụ: không về được Tương Dương (%s) => Lỗi!\n"), szMsg);
		szLog[sizeof(szLog) - 1] = 0;
		theQuest.SaveYSLog(szLog);
		SystemMessage(TV(L"<color=yellow>Không về được Tương Dương, xem nhật ký Dã Tẩu!"));
	}
	if (s_nStep == qs_shop && !bOK)
		theMap.ResetShoping();
	if (s_nStep == qs_kill) PathWalkNoDoor(FALSE);
	s_szKillName[0] = 0;
	s_bProbe = FALSE;
	int nDone = s_nStep;
	if (s_bAutoQuest)
	{
		if (!bOK) s_nAutoResult = qk_failed;
		else if (nDone == qs_return) s_nAutoResult = qk_done;
	}
	s_nStep = qs_none;
	if (!bOK) s_bChain = FALSE;
	else StepNext(nDone);
}

// Nap toa do NPC (muc [NPC] cua Maps\Path\<MapID>_<Ten>.txt) vao theMap.NpcPoints neu chua nap
static BOOL EnsureCityMap()
{
	if (s_nCityMap != *g_pMapID || theMap.m_nPoints <= 0)
	{
		theMap.m_nCurPoint = -1;
		if (!theMap.LoadMap(*g_pMapID)) return FALSE;
		s_nCityMap = *g_pMapID;
	}
	return theMap.m_nPoints > 0;
}

// Buoc 5 (s_bProbe): server bao chua hoan thanh -> dong cua so, bam lai Da Tau chon "Xem tinh hinh" roi danh tiep
static void ProbeNotDone()
{
	char szLog[QS_LINE];
	_snprintf(szLog, sizeof(szLog) - 1, TV(L"Kiểm tra định kỳ: bấm Hoàn thành => chưa hoàn thành (%d / %d), xem tình hình rồi đánh tiếp\n"),
		s_Quest.nKilled > 0 ? s_Quest.nKilled : 0, s_Quest.nKillNeed);
	szLog[sizeof(szLog) - 1] = 0;
	theQuest.SaveYSLog(szLog);
	StepMsg("chua hoan thanh - dong cua so, bam lai Da Tau xem tinh hinh");
	CloseAllWindows(0);
	s_nClicks = s_nNpcClicks = 0;
	SetStage(QS_PROBE_VIEW);
}

// Bang thong bao nut OK: ghi lai + an. TRUE = da xu ly nhip nay (co the da dung buoc)
static BOOL InfoTick()
{
	const char *szText = InfoText();
	if (!szText[0]) return FALSE;
	ParseQuest(szText, "KUiInformation2");
	StepMsg("thong bao: %s", szText);
	g_UiInformation2->Hide();
	if (FindFold(szText, "vip"))
		StepEnd(FALSE, "chua kich hoat the Vip");
	else if (s_bProbe && s_nStep == qs_return && FindFold(szText, KW_NOTDONE))
		ProbeNotDone();	// Kiem tra dinh ky: that su chua xong -> xem tinh hinh roi danh tiep (giu tien do da doc)
	else if (FindFold(szText, KW_NOTDONE))
	{	// Tien do doc sai: bo, cho tin kenh chat lan sau (khong thi lam lai -> "da du" -> tra -> chua xong mai)
		StepEnd(FALSE, "server bao chua hoan thanh nhiem vu (tien do %d / %d) - bo tien do da doc", s_Quest.nKilled, s_Quest.nKillNeed);
		s_Quest.nKilled = -1;
	}
	return TRUE;
}

// Ghi lai hop thoai Da Tau chua biet lam gi (in o nut Trang thai)
static void KeepDialog(const YS_DIALOG &D)
{
	int n = _snprintf(s_szLastDialog, sizeof(s_szLastDialog) - 1, "%s\n", D.szPlain);
	for (int i = 0; i < D.nNum && n > 0 && n < (int)sizeof(s_szLastDialog) - 1; i++)
	{
		int k = _snprintf(s_szLastDialog + n, sizeof(s_szLastDialog) - 1 - n, "  [%d] %s\n", i, MsgLine(D.ppMsg, i));
		if (k < 0) break;
		n += k;
	}
	s_szLastDialog[sizeof(s_szLastDialog) - 1] = 0;
}

// Ve Tuong Duong (Tho dia phu neu dang o ngoai thanh, roi MapPath) va bam NPC Da Tau. Goi khi chua co hop thoai.
static void GoYeSou()
{
	int nMap = *g_pMapID;
	if (nMap != city_tuongduong)
	{
		if (PathTravelActive()) return;
		if (s_dwWaitMapTick)
		{	// Vua dung Tho dia phu: cho doi map
			if (nMap == s_nWaitMap && GetTickCount() - s_dwWaitMapTick < 20000) return;
			s_dwWaitMapTick = 0;
			if (nMap == s_nWaitMap) StepMsg("dung Tho dia phu 20s van chua doi map");
		}
		if (!s_bHomeItem)
		{	// Vat pham ve thang Tuong Duong ([Item] trong Maps\Path) truoc, khong co thi TDP / di bo
			s_bHomeItem = TRUE;
			if (PathTravelItemStart(city_tuongduong))
			{
				StepMsg("dung vat pham ve Tuong Duong (map %d)", nMap);
				return;
			}
		}
		if (!IsTown() && !s_bUsedTDP)
		{
			s_bUsedTDP = TRUE;
			if (theMain.UseAmuletPaper())
			{
				StepMsg("dung Tho dia phu ve thanh (map %d)", nMap);
				s_nWaitMap = nMap;
				s_dwWaitMapTick = GetTickCount();
				return;
			}
			StepMsg("khong co Tho dia phu, tu di bang MapPath");
		}
		if (s_bTravelStarted)
		{
			StepEnd(FALSE, "MapPath dung o map %d (%s), chua toi Tuong Duong", nMap, g_szMapName);
			return;
		}
		s_bTravelStarted = TRUE;
		if (PathTravelStart(city_tuongduong, FALSE))
			StepMsg("MapPath tu di %d -> Tuong Duong (78)", nMap);
		else
			StepEnd(FALSE, "MapPath khong co lo trinh %d -> Tuong Duong (78), xem kenh chat", nMap);
		return;
	}
	if (PathTravelActive()) return;
	if (!IsTown())
	{
		StepEnd(FALSE, "o map 78 nhung m_FightMode = 1 (ngoai thanh?)");
		return;
	}

	int nDist, nX, nY;
	int nIdx = FindYeSou(&nDist);
	if (nIdx)
		g_pNpcArray[nIdx]->GetMapPos(&nX, &nY);
	else
	{
		if (!EnsureCityMap())
		{
			StepEnd(FALSE, "khong thay NPC Da Tau va khong co muc [NPC] trong %s", PathFileShow(*g_pMapID));
			return;
		}
		POINT pt = theMap.NpcPoints[npc_YeSou];
		if (!pt.x || !pt.y)
		{
			StepEnd(FALSE, "NpcPoints[Da Tau] = 0 trong muc [NPC] cua %s", PathFileShow(*g_pMapID));
			return;
		}
		if (theMap.RunningToPos(pt)) return;	// Chay theo MapPath trong thanh toi gan Da Tau
		nX = pt.x;
		nY = pt.y;
	}
	if (s_nNpcClicks++ % 10 == 0)
		StepMsg("bam NPC Da Tau (%d, %d)%s", nX, nY, nIdx ? "" : " theo NpcPoints");
	theMap.ClickToNpc(nX, nY);
}

// Bam 1 dong menu Da Tau (toi da 2 lan, cach nhau 5s). FALSE = da bam 2 lan ma hop thoai van vay
static BOOL ClickOnce(const YS_DIALOG &D, int i)
{
	if (s_nClicks && GetTickCount() - s_dwClickTick < 5000) return TRUE;
	if (s_nClicks >= 2) return FALSE;
	char szLine[128];
	PlainText(MsgLine(D.ppMsg, i), 127, szLine, sizeof(szLine));
	StepMsg("chon menu [%d] %s", i, szLine);
	ClickMenu(D, i);
	s_nClicks++;
	s_dwClickTick = GetTickCount();
	return TRUE;
}

static void CloseDialog(const YS_DIALOG &D)
{
	int i = FindMenu(D, ysm_close);
	if (i >= 0) ClickMenu(D, i);
	else CloseAllWindows(0);
}

static const char* QuestSummary()
{
	static char szText[QS_LINE];
	if (s_Quest.nKill > 0)
		_snprintf(szText, sizeof(szText) - 1, "diet %d %s o %s (MapID %d)", s_Quest.nKill, s_Quest.szMonster, s_Quest.szMap, s_Quest.nMapID);
	else
		_snprintf(szText, sizeof(szText) - 1, "%s", s_Quest.szQuest[0] ? s_Quest.szQuest : "?");
	szText[sizeof(szText) - 1] = 0;
	return szText;
}

// Buoc 1: toi Da Tau, co nhiem vu thi doc, chua co thi bam "Nhan nhiem vu"
static void StepGet()
{
	if (InfoTick()) return;
	YS_DIALOG D;
	if (GetDialog(D))
	{
		ReadDialog(D, FALSE);
		if (!D.bYeSou)
		{
			if (PathTravelActive()) return;	// Menu xa phu / NPC cua MapPath dang tu di: de MapPath xu ly
			StepMsg("dong hop thoai khong phai Da Tau");
			CloseAllWindows(0);
			return;
		}
		int iFinish = FindMenu(D, ysm_finish), iGet = FindMenu(D, ysm_get);
		if (iFinish >= 0)
		{
			CloseDialog(D);
			if (s_Quest.nKill > 0 && !s_Quest.nMapID)
				StepEnd(TRUE, "%s - KHONG THAY map '%s' trong Maps\\MapNames.ini", QuestSummary(), s_Quest.szMap);
			else
				StepEnd(TRUE, "%s, da nhan %d / %d", QuestSummary(), s_Quest.nAccepted, s_Quest.nAcceptMax);
			return;
		}
		if (iGet >= 0)
		{
			if (!ClickOnce(D, iGet))
			{
				KeepDialog(D);
				StepEnd(FALSE, "da bam Nhan nhiem vu 2 lan, hop thoai van vay (xem Trang thai)");
			}
			return;
		}
		KeepDialog(D);
		StepEnd(FALSE, "hop thoai Da Tau chua biet (bang chap nhan?), noi dung o nut Trang thai");
		return;
	}
	GoYeSou();
}

// Buoc 2, giai doan QS_SHOP_REPO: toi Ruong= (map thanh, x, y) trong MapDaTau.ini, cat do vao ruong. Loi thi bo qua (van mua)
static int		s_nRepoPhase = 0;	// 0 di toi map, 1 chay toi ruong, 2 mo ruong, 3 cat do
static DWORD	s_dwRepoTick = 0;
static int		s_nRepoSaved = 0;
static int		s_nRepoMap = 0;		// Ruong dang toi: Ruong= trong MapDaTau.ini, khong co thi Ruong do [NPC] cua thanh dang dung
static POINT	s_ptRepo = { 0, 0 };

// Chon ruong cat do. FALSE = khong co ca Ruong= lan [NPC] Ruong do cua thanh nay
static BOOL RepoTarget()
{
	if (s_KQ.nRepoMap)
	{
		s_nRepoMap = s_KQ.nRepoMap;
		s_ptRepo.x = s_KQ.nRepoX;
		s_ptRepo.y = s_KQ.nRepoY;
		return TRUE;
	}
	if (!EnsureCityMap()) return FALSE;
	s_ptRepo = theMap.NpcPoints[npc_Repository];
	s_nRepoMap = *g_pMapID;
	return s_ptRepo.x && s_ptRepo.y;
}

static void RepoPhase(int nPhase)
{
	s_nRepoPhase = nPhase;
	s_dwRepoTick = GetTickCount();
}

static void RepoDone(const char *szFormat, ...)
{
	char szMsg[QS_LINE - 40];
	va_list va;
	va_start(va, szFormat);
	_vsnprintf(szMsg, sizeof(szMsg) - 1, szFormat, va);
	va_end(va);
	szMsg[sizeof(szMsg) - 1] = 0;
	StepMsg("cat do: %s", szMsg);
	if (PathTravelActive()) PathTravelStop("tab Nhiem vu: cat do xong");
	CloseAllWindows(0);
	theMap.m_nCurPoint = -1;
	theMap.m_bDoShoping = 0;
	if (!IsTown())
	{
		StepEnd(FALSE, "dang o ngoai thanh (map %d), khong mua duoc", *g_pMapID);
		return;
	}
	if (!EnsureCityMap())
	{
		StepEnd(FALSE, "khong co muc [NPC] trong %s", PathFileShow(*g_pMapID));
		return;
	}
	if (theMap.m_bSellItems)
		SetStage(QS_SHOP_SELL);
	else
	{
		StepMsg("tab Ban do chua bat Ban do - bo qua ban do");
		SetStage(QS_SHOP_PLAN);
	}
}

static void StepShopRepo()
{
	int nSec = (int)((GetTickCount() - s_dwRepoTick) / 1000);
	if (*g_pMapID != s_nRepoMap)
	{
		if (PathTravelActive()) return;
		if (s_bTravelStarted)
		{
			RepoDone("MapPath dung o map %d (%s), chua toi map ruong %d - bo qua", *g_pMapID, g_szMapName, s_nRepoMap);
			return;
		}
		s_bTravelStarted = TRUE;
		if (PathTravelStart(s_nRepoMap, FALSE))
			StepMsg("cat do: di %d -> map ruong %d", *g_pMapID, s_nRepoMap);
		else
			RepoDone("MapPath khong co lo trinh %d -> %d - bo qua", *g_pMapID, s_nRepoMap);
		return;
	}
	if (PathTravelActive()) return;
	if (!IsTown())
	{
		RepoDone("map ruong %d khong phai thanh - bo qua", s_nRepoMap);
		return;
	}
	switch (s_nRepoPhase)
	{
	case 0:
		EnsureCityMap();
		theMap.m_nCurPoint = -1;
		theMap.m_bDoShoping = 0;
		theMap.m_nTimeWait = 0;
		RepoPhase(1);
		// tiep
	case 1:
	{
		POINT pt = s_ptRepo;
		if (nSec < 120 && theMap.RunningToPos(pt)) return;
		if (nSec >= 120) StepMsg("cat do: 120s chua toi ruong (%d, %d) - mo ruong tai cho", pt.x, pt.y);
		RepoPhase(2);
		return;
	}
	case 2:
	{
		if (theMain.OpenLockRoom(theMap.m_nRepositoryPass))
		{	// Ruong khoa: da gui lenh mo (mat khau o tab Ban do)
			theMap.m_bRoomLocked = TRUE;
			if (nSec > 15) RepoDone("mo khoa ruong 15s khong duoc (mat khau tab Ban do?) - bo qua");
			return;
		}
		int nOpen = theMap.OpenStoreBox();
		if (nOpen < 0)
		{
			RepoDone("khong thay Ruong chua do quanh (%d, %d) - bo qua", s_ptRepo.x, s_ptRepo.y);
			return;
		}
		if (nOpen == 0)
		{
			if (nSec > 30) RepoDone("mo ruong 30s khong duoc - bo qua");
			return;
		}
		s_nRepoSaved = 0;
		RepoPhase(3);
		return;
	}
	case 3:
		if (nSec < 90 && theMap.SaveItems())
		{
			s_nRepoSaved++;
			return;
		}
		RepoDone("%d lan chuyen do vao ruong%s", s_nRepoSaved, nSec >= 90 ? " (dung: qua 90s)" : "");
		return;
	}
}

// Thuoc tab Ban do dat mua: "ten co/can, ..." (de log ly do mua / khong mua)
static const char* MedicineInfo()
{
	static char szInfo[200];
	int nLen = 0;
	szInfo[0] = 0;
	for (int i = 0; i < 3; i++)
	{
		int nPos = theMap.m_TradeItem[i].nItemPos;
		if (!theMap.m_TradeItem[i].bBuyCheck || nPos < 0 || nPos >= MAX_SHOP_ITEMS) continue;
		int nHave = g_pCurPlayer->GetItemsCount(item_medicine, theMap.m_ShopItem[nPos].nItemType);
		nLen += _snprintf(szInfo + nLen, sizeof(szInfo) - 1 - nLen, "%s%s %d/%d", nLen ? ", " : "",
			theMap.m_ShopItem[nPos].szItemName, nHave, theMap.m_TradeItem[i].nBuyAmout);
		if (nLen < 0 || nLen >= (int)sizeof(szInfo) - 1) { nLen = sizeof(szInfo) - 1; break; }
	}
	szInfo[sizeof(szInfo) - 1] = 0;
	if (!szInfo[0]) return "tab Ban do chua chon mua thuoc nao";
	if (!theMap.NpcPoints[npc_Pharmacy].x) { strncat(szInfo, " - thanh nay chua co [NPC] Hieu thuoc", sizeof(szInfo) - 1 - strlen(szInfo)); }
	return szInfo;
}

// Con thieu thuoc dat mua va thanh nay co Hieu thuoc (giong NeedBuyMedicine cua CVLMap)
static BOOL MedicineShort()
{
	if (!theMap.NpcPoints[npc_Pharmacy].x) return FALSE;
	for (int i = 0; i < 3; i++)
	{
		int nPos = theMap.m_TradeItem[i].nItemPos;
		if (!theMap.m_TradeItem[i].bBuyCheck || nPos < 0 || nPos >= MAX_SHOP_ITEMS) continue;
		if (g_pCurPlayer->GetItemsCount(item_medicine, theMap.m_ShopItem[nPos].nItemType) < theMap.m_TradeItem[i].nBuyAmout)
			return TRUE;
	}
	return FALSE;
}

// Buoc 2: mua Tho dia phu (Tap hoa) + mau/mana (Hieu thuoc) theo tuy chon tab Ban do / Ve thanh
static void StepShop()
{
	if (s_nStage == QS_SHOP_REPO)
	{
		StepShopRepo();
		return;
	}
	if (s_nStage == 0)
	{
		if (!IsTown())
		{
			StepEnd(FALSE, "dang o ngoai thanh, ve thanh truoc");
			return;
		}
		s_nShopSold = 0;
		s_nShopBase = -1;
		s_bSaveAtBox = FALSE;
		if (s_bShopMedOnly)
		{	// Quay lai mua thuoc (Refill): khong cat ruong / ban do lai
			s_bShopMedOnly = FALSE;
			if (!EnsureCityMap())
			{
				StepEnd(FALSE, "khong co muc [NPC] trong %s", PathFileShow(*g_pMapID));
				return;
			}
			SetStage(QS_SHOP_PLAN);
			return;
		}
		if (s_bAutoQuest && RepoTarget())
		{	// Auto diet quai: cat do vao ruong (Ruong= trong Maps\MapDaTau.ini, khong co thi ruong thanh nay) truoc khi ban / mua
			StepMsg("cat do: ruong map %d (%d, %d)%s", s_nRepoMap, s_ptRepo.x, s_ptRepo.y, s_KQ.nRepoMap ? "" : " theo [NPC]");
			RepoPhase(0);
			SetStage(QS_SHOP_REPO);
			return;
		}
		if (!EnsureCityMap())
		{
			StepEnd(FALSE, "khong co muc [NPC] trong %s", PathFileShow(*g_pMapID));
			return;
		}
		if (theMap.m_bSellItems)
			SetStage(QS_SHOP_SELL);
		else
		{
			StepMsg("tab Ban do chua bat Ban do - bo qua ban do");
			SetStage(QS_SHOP_PLAN);
		}
		return;
	}
	if (s_nStage == QS_SHOP_SELL)
	{	// Ban do trong SellItems.txt / do Nhat do khong giu (CVLMap::SellItems, moi nhip 1 mon)
		if (s_nShopSold < 100 && StageSec() < 60 && theMap.SellItems())
		{
			s_nShopSold++;
			return;
		}
		StepMsg("ban do: %d lan ban%s", s_nShopSold, s_nShopSold >= 100 || StageSec() >= 60 ? " (dung: qua 100 mon / 60s)" : "");
		SetStage(QS_SHOP_PLAN);
		return;
	}
	if (s_nStage == QS_SHOP_PLAN)
	{
		theMap.InitEquipCheck();
		int nTDP = g_pCurPlayer->GetItemsCount(AMULET_PAPER);
		int nNeedTDP = theMap.m_nAMNumbers > 0 ? theMap.m_nAMNumbers : 1;
		theMap.m_bNeedGroceries = theMap.m_bBuyTDP && nTDP < nNeedTDP && theMap.NpcPoints[npc_Groceries].x;
		if (!theMap.m_bNeedGroceries && !theMap.m_bNeedPharmacy)
		{
			_snprintf(s_szShopDone, sizeof(s_szShopDone) - 1, "khong can mua (TDP %d, thuoc: %s)", nTDP, MedicineInfo());
			s_szShopDone[sizeof(s_szShopDone) - 1] = 0;
			s_nShopSave = -1;
			SetStage(QS_SHOP_SAVE);
			return;
		}
		StepMsg("can mua:%s%s (thuoc: %s)", theMap.m_bNeedGroceries ? " Tho dia phu" : "", theMap.m_bNeedPharmacy ? " mau/mana" : "", MedicineInfo());
		theMap.m_nCurPoint = -1;
		theMap.m_bDoShoping = 0;
		theMap.m_bFinished = 0;
		s_nShopMoney = -1;
		SetStage(QS_SHOP_MONEY);
		return;
	}
	if (s_nStage == QS_SHOP_MONEY)
	{	// Rut tien tu ruong truoc khi mua (so tien = "Rut tien" tab Ban do, 0 thi QS_MONEY_DEF)
		int nWant = theMap.m_nGetMoneyAmount > 0 ? theMap.m_nGetMoneyAmount * 10000 : QS_MONEY_DEF;
		if (s_nShopMoney < 0)
		{
			if (EQUIP_MONEY >= nWant)
			{
				SetStage(1);
				return;
			}
			if (ROOM_MONEY <= 0)
			{
				StepMsg("tren nguoi %d luong, ruong het tien - mua bang tien dang co", EQUIP_MONEY);
				SetStage(1);
				return;
			}
			if (theMain.OpenLockRoom(theMap.m_nRepositoryPass))
			{	// Ruong khoa: da gui lenh mo (mat khau o tab Ban do)
				theMap.m_bRoomLocked = TRUE;
				if (StageSec() > 15)
				{
					StepMsg("mo khoa ruong 15s khong duoc (mat khau tab Ban do?) - mua bang tien dang co");
					SetStage(1);
				}
				return;
			}
			int nTake = nWant - EQUIP_MONEY;
			if (nTake > ROOM_MONEY) nTake = ROOM_MONEY;
			s_nShopMoney = EQUIP_MONEY;
			if (s_nShopBase < 0) s_nShopBase = EQUIP_MONEY;
			StepMsg("rut %d luong tu ruong (tren nguoi %d, ruong %d)", nTake, EQUIP_MONEY, ROOM_MONEY);
			g_pCoreShell->OperationRequest(GOI_MONEY_INOUT_STORE_BOX, 0, nTake);
			SetStage(QS_SHOP_MONEY);
			return;
		}
		if (EQUIP_MONEY == s_nShopMoney && StageSec() < 5) return;	// Cho tien ve tren nguoi
		if (EQUIP_MONEY == s_nShopMoney)
			StepMsg("rut tien 5s chua thay tien ve (tren nguoi %d) - van di mua", EQUIP_MONEY);
		SetStage(1);
		return;
	}
	if (s_nStage == QS_SHOP_SAVE)
	{	// Cat tien vao ruong. Bat Gui tien: giu "Gui tien" tab Ban do; khong bat: cat lai phan vua rut de mua (giong CVLMap::CheckReady). Luon giu tien xa phu
		int nKeep = theMap.m_bPutMoney ? theMap.m_nMoneyAmount : s_nShopBase;
		if (nKeep < QS_DRIVER_MONEY) nKeep = QS_DRIVER_MONEY;
		if ((!theMap.m_bPutMoney && s_nShopBase < 0) || EQUIP_MONEY <= nKeep)
		{
			if (s_nShopSave < 0)
				StepEnd(TRUE, "%s, khong cat tien (tren nguoi %d, giu %d%s)", s_szShopDone, EQUIP_MONEY, nKeep,
					theMap.m_bPutMoney ? "" : ", tab Ban do chua bat Gui tien va khong rut");
			else
				StepEnd(TRUE, "%s, da cat tien (tren nguoi %d)", s_szShopDone, EQUIP_MONEY);
			return;
		}
		if (s_nShopSave == -2)
		{	// Cat tien xa ruong khong duoc: chay toi Ruong do ([NPC] thanh nay), mo ruong roi cat lai
			POINT pt = theMap.NpcPoints[npc_Repository];
			if (pt.x && pt.y && StageSec() < 60 && theMap.RunningToPos(pt)) return;
			if (theMain.OpenLockRoom(theMap.m_nRepositoryPass))
			{
				theMap.m_bRoomLocked = TRUE;
				if (StageSec() < 75) return;
			}
			else if (theMap.OpenStoreBox() == 0 && StageSec() < 75) return;
			s_nShopSave = -1;
			SetStage(QS_SHOP_SAVE);
		}
		if (s_nShopSave < 0)
		{
			if (theMain.OpenLockRoom(theMap.m_nRepositoryPass))
			{	// Ruong khoa: da gui lenh mo (mat khau o tab Ban do)
				theMap.m_bRoomLocked = TRUE;
				if (StageSec() > 15)
					StepEnd(TRUE, "%s - mo khoa ruong 15s khong duoc, khong cat tien (tren nguoi %d)", s_szShopDone, EQUIP_MONEY);
				return;
			}
			int nSave = EQUIP_MONEY - nKeep;
			s_nShopSave = EQUIP_MONEY;
			StepMsg("cat %d luong vao ruong (tren nguoi %d, giu %d)", nSave, EQUIP_MONEY, nKeep);
			g_pCoreShell->OperationRequest(GOI_MONEY_INOUT_STORE_BOX, TRUE, nSave);
			SetStage(QS_SHOP_SAVE);
			return;
		}
		if (EQUIP_MONEY == s_nShopSave && StageSec() < 5) return;	// Cho tien vao ruong
		if (EQUIP_MONEY == s_nShopSave && !s_bSaveAtBox)
		{
			s_bSaveAtBox = TRUE;
			StepMsg("cat tien 5s chua thay tien giam - toi ruong mo ruong roi cat lai");
			EnsureCityMap();
			theMap.m_nCurPoint = -1;
			theMap.m_bDoShoping = 0;
			theMap.m_nTimeWait = 0;
			s_nShopSave = -2;
			SetStage(QS_SHOP_SAVE);
			return;
		}
		if (EQUIP_MONEY == s_nShopSave)
			StepEnd(TRUE, "%s - cat tien 5s chua thay tien giam (tren nguoi %d)", s_szShopDone, EQUIP_MONEY);
		else
			StepEnd(TRUE, "%s, da cat tien (tren nguoi %d)", s_szShopDone, EQUIP_MONEY);
		return;
	}
	if (theMap.m_bNeedGroceries)
	{
		if (!theMap.m_bDoShoping && theMap.RunningToPos(theMap.NpcPoints[npc_Groceries])) return;
		theMap.m_bDoShoping = TRUE;
		theMap.GetGroceries();
		return;
	}
	if (theMap.m_bNeedPharmacy)
	{
		if (!theMap.m_bDoShoping && theMap.RunningToPos(theMap.NpcPoints[npc_Pharmacy])) return;
		theMap.m_bDoShoping = TRUE;
		theMap.GetPharmacy();
		return;
	}
	sprintf(s_szShopDone, "da mua xong, Tho dia phu %d", g_pCurPlayer->GetItemsCount(AMULET_PAPER));
	s_nShopSave = -1;
	SetStage(QS_SHOP_SAVE);
}

// Mau / mana can co truoc khi ra khoi thanh: tren nguong tab Phuc hoi (an thuoc, ve thanh) va >= QS_REFILL_PCT % toi da
#define QS_REFILL_PCT	90
#define QS_REFILL_MAX	60		// giay, qua thi van di
static BOOL NeedRefill(int nCur, int nMax, BOOL bAuto, int nAuto, BOOL bAM, int nAM)
{
	if (nMax <= 0) return FALSE;
	if (bAuto && nCur < nAuto && nAuto < nMax) return TRUE;
	if (bAM && nCur < nAM) return TRUE;
	return nCur * 100 < nMax * QS_REFILL_PCT;
}

static BOOL StepStartAuto(int nStep);

// Hoi mau / mana trong thanh (an thuoc, het thuoc thi quay lai mua 1 lan / cho tu hoi). TRUE = du, di duoc
extern DWORD g_dwTownEatHP, g_dwTownEatMP;	// VLMap.cpp: luc uong binh o tiem thuoc
#define QS_TOWN_EAT_MS	180000		// Uong o tiem thuoc trong 3 phut nay = lan ve thanh nay da uong

static BOOL Refill()
{
	static DWORD s_dwEatTick = 0;
	static DWORD s_dwEatKey = 0;	// Lan ve thanh cua s_bAteHP / s_bAteMP
	static BOOL s_bAteHP = FALSE, s_bAteMP = FALSE;
	DWORD dwKey = s_dwQuestStart + s_nTownTrips + 1;
	if (s_dwEatKey != dwKey)
	{	// Moi lan ve thanh: moi loai chi uong 1 binh (tinh ca binh da uong o tiem thuoc)
		s_dwEatKey = dwKey;
		s_bAteHP = s_bAteMP = FALSE;
	}
	if (g_dwTownEatHP && GetTickCount() - g_dwTownEatHP < QS_TOWN_EAT_MS) s_bAteHP = TRUE;
	if (g_dwTownEatMP && GetTickCount() - g_dwTownEatMP < QS_TOWN_EAT_MS) s_bAteMP = TRUE;
	int nHP = P_PLAYER->m_CurrentLife, nMaxHP = P_PLAYER->GetLifeMax();
	int nMP = P_PLAYER->m_CurrentMana, nMaxMP = P_PLAYER->GetManaMax();
	// Da uong 1 binh: chi cho tu hoi khi con duoi nguong "Ve thanh khi mau/mana" (ra map la TDP ngay)
	BOOL bHP = s_bAteHP ? (theRecover.m_bAMByHP && nHP < theRecover.m_nAMByHPAmount) :
		NeedRefill(nHP, nMaxHP, theRecover.m_bAutoHP, theRecover.m_nAutoHPAmount, theRecover.m_bAMByHP, theRecover.m_nAMByHPAmount);
	BOOL bMP = s_bAteMP ? (theRecover.m_bAMByMP && nMP < theRecover.m_nAMByMPAmount) :
		NeedRefill(nMP, nMaxMP, theRecover.m_bAutoMP, theRecover.m_nAutoMPAmount, theRecover.m_bAMByMP, theRecover.m_nAMByMPAmount);
	if (!bHP && !bMP) return TRUE;
	if (StageSec() >= QS_REFILL_MAX)
	{
		StepMsg("hoi mau/mana %ds chua du (mau %d/%d, mana %d/%d) - van di", QS_REFILL_MAX, nHP, nMaxHP, nMP, nMaxMP);
		return TRUE;
	}
	if (GetTickCount() - s_dwEatTick < 1500) return FALSE;	// Cho thuoc ngam
	s_dwEatTick = GetTickCount();
	BOOL bAte = FALSE, bTry = FALSE;
	if (bHP && !s_bAteHP)
	{
		bTry = TRUE;
		if (g_pCurPlayer->EatMedicine(medicine_blood)) bAte = s_bAteHP = TRUE;
	}
	if (bMP && !s_bAteMP)
	{
		bTry = TRUE;
		if (g_pCurPlayer->EatMedicine(medicine_mana)) bAte = s_bAteMP = TRUE;
	}
	if (bTry && !bAte && s_nStep == qs_gomap && IsTown() && s_dwMedShopKey != dwKey && EnsureCityMap() && MedicineShort())
	{	// Het thuoc ma chua mua (lan mua truoc khong mua duoc / bo qua): quay lai Hieu thuoc 1 lan
		s_dwMedShopKey = dwKey;
		StepMsg("hoi mau/mana: het thuoc (%s) - quay lai mua", MedicineInfo());
		s_bShopMedOnly = TRUE;
		s_nStep = qs_none;
		StepStartAuto(qs_shop);
		return FALSE;
	}
	if (s_nTick % 15 == 0)	// ~9s bao 1 lan
		StepMsg("hoi %s%s truoc khi di (mau %d/%d, mana %d/%d)%s", bHP ? "mau" : "", bHP && bMP ? " + " : bMP ? "mana" : "",
			bHP && bMP ? "mana" : "", nHP, nMaxHP, nMP, nMaxMP, bTry && !bAte ? " - het thuoc, cho tu hoi" : bTry ? "" : " - da uong 1 binh, cho tu hoi");
	return FALSE;
}

// Tien xa phu: tren nguoi < QS_DRIVER_MONEY thi rut tu ruong. TRUE = du (hoac khong rut duoc), di duoc
static BOOL DriverMoney()
{
	static int s_nMoneyBefore = -1;	// Tien tren nguoi luc gui lenh rut (-1 = chua rut)
	static DWORD s_dwMoneyTick = 0;	// Luc bat dau mo ruong / gui lenh rut
	if (EQUIP_MONEY >= QS_DRIVER_MONEY)
	{
		s_nMoneyBefore = -1;
		s_dwMoneyTick = 0;
		return TRUE;
	}
	if (!s_dwMoneyTick) s_dwMoneyTick = GetTickCount();
	int nSec = (int)((GetTickCount() - s_dwMoneyTick) / 1000);
	if (s_nMoneyBefore < 0)
	{
		if (ROOM_MONEY <= 0)
		{
			StepMsg("tren nguoi %d luong, ruong het tien - khong co tien xa phu", EQUIP_MONEY);
			s_dwMoneyTick = 0;
			return TRUE;
		}
		if (theMain.OpenLockRoom(theMap.m_nRepositoryPass))
		{	// Ruong khoa: da gui lenh mo (mat khau o tab Ban do)
			theMap.m_bRoomLocked = TRUE;
			if (nSec > 15)
			{
				StepMsg("mo khoa ruong 15s khong duoc - khong rut duoc tien xa phu (tren nguoi %d)", EQUIP_MONEY);
				s_dwMoneyTick = 0;
				return TRUE;
			}
			return FALSE;
		}
		int nTake = QS_DRIVER_MONEY - EQUIP_MONEY;
		if (nTake > ROOM_MONEY) nTake = ROOM_MONEY;
		s_nMoneyBefore = EQUIP_MONEY;
		StepMsg("rut %d luong tien xa phu (tren nguoi %d, ruong %d)", nTake, EQUIP_MONEY, ROOM_MONEY);
		g_pCoreShell->OperationRequest(GOI_MONEY_INOUT_STORE_BOX, 0, nTake);
		s_dwMoneyTick = GetTickCount();	// dem 5s cho tien ve
		return FALSE;
	}
	if (EQUIP_MONEY == s_nMoneyBefore && nSec < 5) return FALSE;	// Cho tien ve tren nguoi
	if (EQUIP_MONEY == s_nMoneyBefore)
		StepMsg("rut tien xa phu 5s chua thay tien ve (tren nguoi %d) - van di", EQUIP_MONEY);
	s_nMoneyBefore = -1;
	s_dwMoneyTick = 0;
	return TRUE;
}

// Buoc 3: cach di toi map nhiem vu tiep theo (s_nGoTry). Lan dau: vat pham -> xa phu -> di bo;
// ve thanh giua chung (Auto, s_nTownTrips > 0): xa phu -> vat pham -> di bo. TRUE = da bat dau di
static BOOL GoMapTry()
{
	BOOL bReturn = s_bAutoQuest && s_nTownTrips > 0;
	while (s_nGoTry < 3)
	{
		int nKind = s_nGoTry++;
		if (bReturn && nKind < 2) nKind = 1 - nKind;
		switch (nKind)
		{
		case 0:
			if (PathTravelItemStart(s_nTarget))
			{
				StepMsg("dung vat pham di %d -> %d (%s)", *g_pMapID, s_nTarget, s_Quest.szMap);
				return TRUE;
			}
			break;
		case 1:
			if (PathTravelStart(s_nTarget, TRUE))
			{
				StepMsg("MapPath tu di %d -> %d (%s)", *g_pMapID, s_nTarget, s_Quest.szMap);
				return TRUE;
			}
			break;
		case 2:
			if (PathTravelStart(s_nTarget, FALSE, TRUE))
			{
				StepMsg("MapPath chay bo %d -> %d (%s)", *g_pMapID, s_nTarget, s_Quest.szMap);
				return TRUE;
			}
			break;
		}
	}
	return FALSE;
}

// Buoc 3: MapPath tu di toi map nhiem vu
static void StepGoMap()
{
	if (*g_pMapID == s_nTarget)
	{	// Toi map la dung: khong di tiep theo danh sach toa do luyen cong (GoTrain / CVLMove)
		if (PathTravelActive()) PathTravelStop();
		theMove.GoTrainStop();
		theMove.m_bFindingWay = 0;
		s_nHoldMap = s_nTarget;
		StepEnd(TRUE, "da toi map %d (%s), dung tai cho", s_nTarget, g_szMapName);
		return;
	}
	if (s_nStage == 0)
	{	// Trong thanh Phuc hoi khong chay (chi chay khi chien dau): hoi mau / mana truoc,
		// khong thi vua len map (bat chien dau) la duoi nguong "Ve thanh khi mau/mana" => dung TDP ngay
		if (IsTown() && !Refill()) return;
		if (IsTown() && !DriverMoney()) return;
		if (!GoMapTry())
		{
			StepEnd(FALSE, "MapPath khong tu di duoc %d -> %d (xem ly do o kenh chat)", *g_pMapID, s_nTarget);
			return;
		}
		SetStage(1);
		return;
	}
	if (PathTravelActive()) return;
	StepMsg("MapPath dung o map %d (%s), chua toi map %d - thu cach khac", *g_pMapID, g_szMapName, s_nTarget);
	if (!GoMapTry())
		StepEnd(FALSE, "da thu vat pham / xa phu / di bo, dung o map %d (%s), chua toi map %d", *g_pMapID, g_szMapName, s_nTarget);
}

// Nap cac diem MapPath cua map hien tai cho Buoc 4. Tra ve so diem
static int RoamLoad()
{
	s_bRoamList = FALSE;
	s_nKillRange = 0;
	if (s_bAutoQuest && s_KQ.nMapID == *g_pMapID)
	{	// Auto: diem / ban kinh danh trong Maps\MapDaTau.ini (hoac tab Danh quai)
		s_nKillRange = s_KQ.nRange > 0 ? s_KQ.nRange : 0;
		if (s_KQ.nPoints > 0)
		{
			s_bRoamList = TRUE;
			s_nRoamPts = s_KQ.nPoints;
			memcpy(s_RoamPt, s_KQ.Pts, s_nRoamPts * sizeof(POINT));
			s_nRoamDoorSkip = 0;
		}
	}
	if (!s_bRoamList)
		s_nRoamPts = PathMapPoints(*g_pMapID, s_RoamPt, QS_ROAM_MAX, TRUE, &s_nRoamDoorSkip);	// Bo diem gan cong: khong di vao cong
	memset(s_RoamDone, 0, sizeof(s_RoamDone));
	s_nRoamIdx = -1;
	s_nRoamRounds = 0;
	PathWalkNoDoor(TRUE);
	PathWalkReset();
	return s_nRoamPts;
}

// Diem MapPath chua qua gan nhat (bo qua diem sat nhan vat); qua het thi bat dau vong moi
static int RoamNext(int x, int y)
{
	if (s_bRoamList)
	{	// Danh sach MapDaTau.ini: lan luot tu diem dau, het thi vong lai
		int n = s_nRoamIdx + 1;
		if (n >= s_nRoamPts)
		{
			n = 0;
			s_nRoamRounds++;
		}
		return s_nRoamPts > 0 ? n : -1;
	}
	for (int nPass = 0; nPass < 2; nPass++)
	{
		int nBest = -1, dBest = 0x7FFFFFFF;
		for (int i = 0; i < s_nRoamPts; i++)
		{
			if (s_RoamDone[i]) continue;
			int d = __distance(s_RoamPt[i].x, s_RoamPt[i].y, x, y);
			if (d < QS_ROAM_NEAR) { s_RoamDone[i] = 1; continue; }
			if (d < dBest) { dBest = d; nBest = i; }
		}
		if (nBest >= 0) return nBest;
		memset(s_RoamDone, 0, sizeof(s_RoamDone));
		s_nRoamRounds++;
	}
	return -1;
}

// Di toi diem MapPath tiep theo (PathWalkTo di theo canh MapPath). FALSE = map khong co MapPath
static BOOL RoamTick()
{
	if (s_nRoamPts <= 0) return FALSE;
	int x, y;
	P_PLAYER->GetMapPos(&x, &y);
	if (s_nRoamIdx >= 0)
	{
		POINT pt = s_RoamPt[s_nRoamIdx];
		int nRet = PathWalkTo(pt.x, pt.y);
		BOOL bInTime = GetTickCount() - s_dwRoamTick < 60000;
		if (nRet > 0 && bInTime) return TRUE;
		if (nRet <= 0 && bInTime && __distance(pt.x, pt.y, x, y) > QS_ROAM_NEAR)
		{	// Cuoi duong MapPath con doan ngan / diem khong noi duoc: chay thang
			g_pCurPlayer->MoveTo(pt.x, pt.y);
			return TRUE;
		}
		s_RoamDone[s_nRoamIdx] = 1;	// Toi noi / 60s khong toi: sang diem khac
	}
	s_nRoamIdx = RoamNext(x, y);
	if (s_nRoamIdx < 0) return FALSE;
	s_dwRoamTick = GetTickCount();
	PathWalkReset();
	PathWalkTo(s_RoamPt[s_nRoamIdx].x, s_RoamPt[s_nRoamIdx].y);
	return TRUE;
}

static DWORD	s_dwNoTargetTick = 0;	// Buoc 4: luc thay quai nhiem vu ma chua co muc tieu
static DWORD	s_dwNoTargetSaid = 0;
#define QS_LAG_MS	10000				// Buoc 4: muc tieu khong mat mau ngan nay ms = quai lag
static DWORD	s_dwLagWatchID = 0;		// Buoc 4: muc tieu dang canh lag
static int		s_nLagWatchLife = 0;
static DWORD	s_dwLagWatchTick = 0;	// Luc muc tieu mat mau lan cuoi (hoac moi nhan)
static DWORD	s_dwLagSaid = 0;

// Quai nhiem vu gan nhat danh duoc (bo quai "lag" Chien dau da ghi trong m_BadNpc, quai ma, dang chet)
static int NearestKillable(int *pDist)
{
	int nPlayerX, nPlayerY, nNpcX, nNpcY, nBest = 0, nBestDist = 0x7FFFFFFF;
	P_PLAYER->GetMapPos(&nPlayerX, &nPlayerY);
	for (int i = 1; i < MAX_NPC; i++)
	{
		KNpc *pNpc = g_pNpcArray[i];
		if (!pNpc->IsExist() || i == g_pCurPlayer->m_nIndex || pNpc->m_Kind != kind_normal) continue;
		if (pNpc->m_CurrentLife <= 0 || pNpc->m_Doing == do_death || pNpc->IsGhost()) continue;
		if (!NameMatch(pNpc->Name, s_szKillName)) continue;
		KLagNpc lagNpc(pNpc->m_dwID, pNpc->m_CurrentLife);
		if (lagNpc == theAttack.m_BadNpc[0] || lagNpc == theAttack.m_BadNpc[1] || lagNpc == theAttack.m_BadNpc[2]) continue;
		pNpc->GetMapPos(&nNpcX, &nNpcY);
		int dZ = __distance(nPlayerX, nPlayerY, nNpcX, nNpcY);
		if (dZ < nBestDist)
		{
			nBest = i;
			nBestDist = dZ;
		}
	}
	if (pDist) *pDist = nBestDist;
	return nBest;
}

// Ten cac quai (khac nhau) dang thay quanh nhan vat - bao khi khong co con nao trung ten nhiem vu
static const char* NearbyNames()
{
	static char szList[160];
	const char *pNames[4];
	int nNames = 0, n = 0;
	szList[0] = 0;
	for (int i = 1; i < MAX_NPC && nNames < 4; i++)
	{
		KNpc *pNpc = g_pNpcArray[i];
		if (!pNpc->IsExist() || i == g_pCurPlayer->m_nIndex || pNpc->m_Kind != kind_normal || pNpc->m_CurrentLife <= 0) continue;
		int j = 0;
		while (j < nNames && strcmp(pNames[j], pNpc->Name)) j++;
		if (j < nNames) continue;
		pNames[nNames++] = pNpc->Name;
		int k = _snprintf(szList + n, sizeof(szList) - 1 - n, "%s'%.24s'", n ? ", " : "", pNpc->Name);
		if (k < 0) break;
		n += k;
	}
	szList[sizeof(szList) - 1] = 0;
	return nNames ? szList : "khong co quai nao";
}

// Ly do CVLAttack::Breathe khong tim quai (in khi Buoc 4 phai tu nhan muc tieu)
static const char* AttackBlockReason()
{
	static char szWhy[160];
	int n = 0;
	szWhy[0] = 0;
#define WHY(b, s)	if ((b) && n < (int)sizeof(szWhy) - 24) n += sprintf(szWhy + n, ", %s", s)
	WHY(!theAttack.m_bAutoHit, "Tu danh quai TAT");
	WHY(!P_PLAYER->m_FightMode, "khong o che do chien dau");
	WHY(theAttack.m_nEnemyIdx, "dang phan don nguoi choi");
	WHY(theAttack.m_bDoingSkill || theAttack.m_bDoingSkillEx, "dang ra skill phu");
	WHY(theRecover.m_bDoingBuff, "dang buff");
	WHY(theRecover.m_bEmptyMana, "het mana (tam tim = 0)");
	WHY(theQuest.m_bNeedGoPCK, "Da Tau cu: di PCK");
	WHY(theMove.m_nStatus == status_pickup, "dang nhat do");
	WHY(theAttack.m_nSearchRange <= 0, "tam tim = 0");
#undef WHY
	return szWhy;
}

// Tui hanh trang khong con o trong 1x1
static BOOL BagFull()
{
	KInventory Room = EQUIP_ROOM;
	POINT pt;
	return !Room.FindRoom(1, 1, &pt);
}

// Het cac loai thuoc tab Ban do dat mua (co dat mua it nhat 1 loai)
static BOOL NoMedicine()
{
	int nTypes = 0, nHave = 0;
	for (int i = 0; i < 3; i++)
	{
		int nPos = theMap.m_TradeItem[i].nItemPos;
		if (!theMap.m_TradeItem[i].bBuyCheck || nPos < 0 || nPos >= MAX_SHOP_ITEMS) continue;
		nTypes++;
		nHave += g_pCurPlayer->GetItemsCount(item_medicine, theMap.m_ShopItem[nPos].nItemType);
	}
	return nTypes > 0 && nHave <= 0;
}

// Buoc 4 (Auto): da roi map nhiem vu. Vao thanh: Buoc 2 lai tu dau (cat do -> ban -> rut tien -> mua -> cat tien
// -> xa phu / vat pham / di bo); map khac: di lai toi map. Khong tinh la loi (s_nTownTrips: kiem tra dai han)
static void KillTownTrip()
{
	if (PathTravelActive()) return;	// Dang tu di (vd vat pham / MapPath ve thanh): cho toi noi
	int nNext = IsTown() ? qs_shop : qs_gomap;
	if (nNext == qs_shop) s_nTownTrips++;
	StepMsg("roi map nhiem vu, dang o map %d (%s)%s - tien do %d / %d, lam lai %s", *g_pMapID, g_szMapName,
		nNext == qs_shop ? " trong thanh" : "", s_Quest.nKilled, s_Quest.nKillNeed, s_szStepName[nNext]);
	PathWalkNoDoor(FALSE);
	s_szKillName[0] = 0;
	s_nStep = qs_none;
	StepStartAuto(nNext);
}

// Buoc 4: Auto (Chien dau) danh, chi nhan quai nhiem vu; khong thay quai thi di vong theo MapPath;
// theo doi tien do tu kenh Cong bo
static void StepKill()
{
	if (s_Quest.nKillNeed > 0 && s_Quest.nKilled >= s_Quest.nKillNeed)
	{
		s_nHoldMap = *g_pMapID;
		StepEnd(TRUE, "du %d / %d %s -> Buoc 3", s_Quest.nKilled, s_Quest.nKillNeed, s_Quest.szMonster);
		return;
	}
	if (s_nTarget && *g_pMapID != s_nTarget)
	{
		if (s_bAutoQuest)
			KillTownTrip();	// Ve thanh giua chung (TDP / chet): cat do, mua, di lai - khong tinh la loi
		else
			StepEnd(FALSE, "roi map nhiem vu, dang o map %d (%s) - chet / ve thanh?", *g_pMapID, g_szMapName);
		return;
	}
	if (s_bAutoQuest && !theMain.m_bPause && (int)(GetTickCount() - s_dwTripCheck) >= 3000)
	{	// Tui day / het thuoc: TDP ve thanh (vao thanh thi KillTownTrip lam tiep)
		s_dwTripCheck = GetTickCount();
		const char *szWhy = BagFull() ? "tui day" : NoMedicine() ? "het thuoc" : NULL;
		if (szWhy)
		{
			if (theMain.UseAmuletPaper())
			{
				StepMsg("%s - dung Tho dia phu ve thanh", szWhy);
				s_dwTripCheck = GetTickCount() + 17000;	// cho doi map 20s moi kiem tra lai
				return;
			}
			if (s_nTick % 50 == 0) StepMsg("%s nhung khong co Tho dia phu - danh tiep", szWhy);
		}
	}
	int nNear, nNearDist;
	int nCount = CountMonsters(s_szKillName, &nNear, &nNearDist);
	static DWORD s_dwNamesTick = 0;
	if (!nCount && GetTickCount() - s_dwNamesTick >= 10000)
	{	// Dang di vong: quai thay duoc khong trung ten nhiem vu -> in ten de so
		s_dwNamesTick = GetTickCount();
		const char *szNames = NearbyNames();
		if (szNames[0] == '\'')
			StepMsg("quanh day co %s - khong con nao trung ten '%s'", szNames, s_szKillName);
	}
	if (GetTickCount() - s_dwReportTick >= 30000)
	{
		s_dwReportTick = GetTickCount();
		int nPeople = PEOPLE_IDX;
		StepMsg("tien do %d / %d, task1027 = %d, quai quanh %d con, dang danh '%s', MapPath diem %d / %d vong %d, chan %d lenh di khac%s",
			s_Quest.nKilled, s_Quest.nKillNeed, g_pCurPlayer->GetTaskVal(ys_shxtmap), nCount,
			(nPeople > 0 && nPeople < MAX_NPC) ? g_pNpcArray[nPeople]->Name : "",
			s_nRoamIdx, s_nRoamPts, s_nRoamRounds, s_nMoveBlocked,
			theMain.m_bPause ? ", AUTO DANG TAM DUNG (m_bPause)" : "");
	}
	if (theMain.m_bPause) return;	// g_SysPause: DebugQuestBreathe goi thay theAttack.Breathe
	// CVLMove dung (khong ai dat lai): status_movefirst / status_move lam Chien dau khong danh
	if (theMove.m_nStatus == status_movefirst || theMove.m_nStatus == status_move) theMove.m_nStatus = status_idle;
	// Co che do di nao bat lai: Chien dau bo muc tieu / khong tim quai (CVLAttack::Breathe)
	if (theMove.m_bGoTrain) theMove.GoTrainStop();
	theMove.m_bFindingWay = 0;
	// Dang danh: de Chien dau lam, nhung tu canh quai lag - IsLagTarget chi dem khi Chien dau chay toi buoc tim quai
	// (dang buff / ra skill phu / di chuyen thi khong dem) => muc tieu khong mat mau QS_LAG_MS thi bo
	int nPeople = PEOPLE_IDX;
	if (nPeople > 0 && nPeople < MAX_NPC && g_pNpcArray[nPeople]->m_Kind == kind_normal)
	{
		KNpc *pNpc = g_pNpcArray[nPeople];
		if (pNpc->m_dwID != s_dwLagWatchID || pNpc->m_CurrentLife < s_nLagWatchLife)
		{
			s_dwLagWatchID = pNpc->m_dwID;
			s_nLagWatchLife = pNpc->m_CurrentLife;
			s_dwLagWatchTick = GetTickCount();
		}
		else if (GetTickCount() - s_dwLagWatchTick >= QS_LAG_MS)
		{
			int nX, nY, px, py;
			pNpc->GetMapPos(&nX, &nY);
			P_PLAYER->GetMapPos(&px, &py);
			StepMsg("'%s' (ID %u) %ds khong mat mau (mau %d, cach %d) - quai lag, bo qua %ds", pNpc->Name, pNpc->m_dwID,
				QS_LAG_MS / 1000, pNpc->m_CurrentLife, __distance(px, py, nX, nY), LAG_NPC_TIME / 10);
			theAttack.AddBadNpc(pNpc->m_dwID, pNpc->m_CurrentLife);
			theAttack.SetPeopleIdx(0);
			theAttack.m_LastNpcID = 0;
			theAttack.m_LastNpcLife = 0;
			s_dwLagWatchID = 0;
			s_dwNoTargetTick = GetTickCount();
			return;
		}
		s_dwNoMonsterTick = 0;
		s_dwNoTargetTick = 0;
		return;
	}
	s_dwLagWatchID = 0;
	// Quai nhiem vu danh duoc (bo quai lag / ma). Chi con quai lag thi di vong tim con khac, khong chay toi quai lag
	int nKillDist = 0x7FFFFFFF;
	int nKill = nCount ? NearestKillable(&nKillDist) : 0;
	BOOL bFar = nKill && s_nKillRange > 0 && nKillDist > s_nKillRange;	// Ngoai ban kinh danh MapDaTau.ini: di vong tiep
	if (nKill && !bFar)
	{
		s_dwNoMonsterTick = 0;
		s_bNoMonsterSaid = FALSE;
		if (!s_dwNoTargetTick) s_dwNoTargetTick = GetTickCount();
		int nX, nY;
		g_pNpcArray[nKill]->GetMapPos(&nX, &nY);
		if (GetTickCount() - s_dwNoTargetTick >= 400)
		{	// Chien dau chua nhan quai nhiem vu dang thay (400ms): tu nhan con gan nhat (khong chay ngang qua quai)
			if (GetTickCount() - s_dwNoTargetSaid >= 10000)
			{
				s_dwNoTargetSaid = GetTickCount();
				StepMsg("Chien dau chua nhan '%s' cach %d (tam tim %d%s) - tu nhan muc tieu", g_pNpcArray[nKill]->Name, nKillDist,
					theAttack.m_nSearchRange, AttackBlockReason());
			}
			theAttack.SetPeopleIdx(nKill);
			theAttack.m_LastNpcLife = g_pNpcArray[nKill]->m_CurrentLife;
			theMove.m_nStatus = status_attack;
			g_pCurPlayer->MoveTo(nX, nY);
			s_dwNoTargetTick = GetTickCount();
		}
		else if (nKillDist > theAttack.m_nSearchRange)	// Ngoai tam tim cua Auto: chay lai gan
			g_pCurPlayer->MoveTo(nX, nY);
		return;
	}
	if (nCount && !bFar && GetTickCount() - s_dwLagSaid >= 10000)
	{
		s_dwLagSaid = GetTickCount();
		StepMsg("%d con '%s' quanh day deu la quai lag / ma (gan nhat cach %d) - di vong tim con khac", nCount, s_szKillName, nNearDist);
	}
	s_dwNoTargetTick = 0;
	// Khong thay quai nhiem vu: di vong theo MapPath
	if (RoamTick()) return;
	if (!s_dwNoMonsterTick) s_dwNoMonsterTick = GetTickCount();
	if (!s_bNoMonsterSaid && GetTickCount() - s_dwNoMonsterTick > 20000)
	{
		s_bNoMonsterSaid = TRUE;
		StepMsg("20s khong thay '%s' va map %d chua co MapPath (tab MapPath ghi diem) - dung tai cho", s_szKillName, *g_pMapID);
	}
}

static const char* ProbeDoneLog()
{
	static char szLog[QS_LINE];
	_snprintf(szLog, sizeof(szLog) - 1, TV(L"Kiểm tra định kỳ: bấm Hoàn thành => đã hoàn thành (kênh chat chỉ báo %d / %d)\n"),
		s_Quest.nKilled > 0 ? s_Quest.nKilled : 0, s_Quest.nKillNeed);
	szLog[sizeof(szLog) - 1] = 0;
	return szLog;
}

// Buoc 5 (s_bProbe) chua hoan thanh, da xem tinh hinh: lam lai Buoc 2 nhu luc moi nhan (cat ruong -> ban / rut tien / mua
// -> di toi map -> danh), khong tinh ve thanh
static void ProbeBack(const char *szWhy)
{
	StepMsg("%s - tien do %d / %d, di danh tiep", szWhy, s_Quest.nKilled, s_Quest.nKillNeed);
	if (PathTravelActive()) PathTravelStop("kiem tra xong");
	s_bProbe = FALSE;
	s_nLongKilled = s_Quest.nKilled > 0 ? s_Quest.nKilled : 0;
	s_dwLongCheckTick = GetTickCount();
	s_nStep = qs_none;
	theMap.ResetShoping();	// Nhu luc moi nhan nhiem vu (CVLQuest::DoQuestAttackNpc)
	StepStartAuto(IsTown() ? qs_shop : qs_gomap);
}

// Buoc 5 (s_bProbe), giai doan QS_PROBE_VIEW / QS_PROBE_WAIT: bam lai Da Tau, chon "Xem tinh hinh", doc roi dong
static void StepProbeView()
{
	if (StageSec() < 1) return;	// Cho cua so dong han
	const char *szText = InfoText();
	if (szText[0])
	{
		ParseQuest(szText, "KUiInformation2");
		StepMsg("tinh hinh: %s", szText);
		g_UiInformation2->Hide();
		if (s_nStage == QS_PROBE_WAIT) ProbeBack("da xem tinh hinh");
		return;
	}
	YS_DIALOG D;
	if (GetDialog(D))
	{
		ReadDialog(D, FALSE);
		if (!D.bYeSou)
		{
			if (PathTravelActive()) return;
			CloseAllWindows(0);
			return;
		}
		int iView = FindMenu(D, ysm_view);
		if (s_nStage == QS_PROBE_WAIT || iView < 0)
		{	// Hop thoai tinh hinh (hoac Da Tau khong co dong Xem tinh hinh): doc xong thi dong
			StepMsg("tinh hinh: %.200s", D.szPlain);
			CloseDialog(D);
			ProbeBack(iView < 0 ? "Da Tau khong co dong Xem tinh hinh" : "da xem tinh hinh");
			return;
		}
		ClickOnce(D, iView);
		SetStage(QS_PROBE_WAIT);
		return;
	}
	if (s_nStage == QS_PROBE_WAIT)
	{
		if (StageSec() >= 5) ProbeBack("chon Xem tinh hinh 5s khong thay gi");
		return;
	}
	GoYeSou();
}

// Buoc 5: ve Da Tau, chon "Hoan thanh nhiem vu", chon o phan thuong s_nTarget
static void StepReturn()
{
	if (s_bProbe && s_nStage >= QS_PROBE_VIEW)
	{
		StepProbeView();
		return;
	}
	KUiPrizeSel *pPrzSel = GetPrizeSel();
	if (pPrzSel && s_bAutoQuest)
	{	// Auto: khong chon o o day, CVLQuest::ChoosePrize chon theo thiet lap Phan thuong
		if (s_bProbe)
			theQuest.SaveYSLog(ProbeDoneLog());
		s_Quest.nKilled = s_Quest.nKillNeed = -1;
		s_Quest.nKill = 0;
		s_Quest.szMonster[0] = 0;
		s_Quest.nMapID = 0;
		StepEnd(TRUE, "bang phan thuong da hien, Auto chon o");
		return;
	}
	if (s_nStage == 1)
	{	// Da bam o phan thuong: cho bang dong
		if (!pPrzSel || StageSec() >= 3)
		{
			s_Quest.nKilled = s_Quest.nKillNeed = -1;
			s_Quest.nKill = 0;
			s_Quest.szMonster[0] = 0;
			s_Quest.nMapID = 0;
			StepEnd(TRUE, "da nhan thuong o %d%s", s_nTarget, pPrzSel ? " (bang chua dong)" : "");
		}
		return;
	}
	if (pPrzSel)
	{
		PrizeItem &P = pPrzSel->PrzItem[s_nTarget];
		char szText[64];
		PlainText(P.m_szText, sizeof(P.m_szText), szText, sizeof(szText));
		StepMsg("bang phan thuong: chon o %d %s (%s)", s_nTarget, PrizeKind(P.m_szIcon), szText);
		pPrzSel->OnClickBtn(s_nTarget);
		SetStage(1);
		return;
	}
	if (InfoTick()) return;
	YS_DIALOG D;
	if (GetDialog(D))
	{
		ReadDialog(D, FALSE);
		if (!D.bYeSou)
		{
			if (PathTravelActive()) return;	// Menu xa phu / NPC cua MapPath dang tu di: de MapPath xu ly
			StepMsg("dong hop thoai khong phai Da Tau");
			CloseAllWindows(0);
			return;
		}
		if (s_bProbe && FindFold(D.szPlain, KW_NOTDONE))
		{
			ProbeNotDone();
			return;
		}
		int iFinish = FindMenu(D, ysm_finish);
		if (iFinish >= 0)
		{
			if (!ClickOnce(D, iFinish))
			{
				if (s_bProbe)
				{	// Bam Hoan thanh 2 lan khong hien bang phan thuong = chua xong
					ProbeNotDone();
					return;
				}
				KeepDialog(D);
				StepEnd(FALSE, "da chon Hoan thanh 2 lan, khong hien bang phan thuong");
			}
			return;
		}
		KeepDialog(D);
		CloseDialog(D);
		StepEnd(FALSE, FindMenu(D, ysm_get) >= 0 ? "chua nhan nhiem vu (co dong Nhan nhiem vu)" : "khong co dong Hoan thanh nhiem vu");
		return;
	}
	GoYeSou();
}

// Bat lai "Tu lam nhiem vu": ve Tuong Duong (vat pham -> TDP ngoai thanh -> MapPath), CVLQuest cho roi nhan nhiem vu
static void StepHome()
{
	if (*g_pMapID == city_tuongduong)
	{
		if (PathTravelActive()) PathTravelStop("da toi Tuong Duong");
		StepEnd(TRUE, "da ve Tuong Duong, nhan lai nhiem vu");
		return;
	}
	GoYeSou();	// Ngoai Tuong Duong: chi lam phan di (vat pham / TDP / MapPath)
}

// Buoc 4 -> Buoc 5 (s_bProbe): ve Da Tau bam Hoan thanh thu
static void ProbeStart()
{
	PathWalkNoDoor(FALSE);
	s_szKillName[0] = 0;
	s_nStep = qs_none;
	s_bProbed = TRUE;
	if (StepStartAuto(qs_return)) s_bProbe = TRUE;
}

// Auto diet quai (600ms): kiem tra dai han theo tab Danh quai
#define QS_TRIP_CANCEL	5		// Ve thanh tu lan nay ma chua diet duoc 50% thi huy (khi bat Huy nhiem vu lau)
static void KillLongCheck()
{
	if (!s_bAutoQuest || !s_dwQuestStart || s_nStep == qs_return) return;
	const YSKillQOpt &O = theQuest.m_KillQOpt;
	DWORD dwNow = GetTickCount();
	int nNeed = s_Quest.nKillNeed > 0 ? s_Quest.nKillNeed : s_Quest.nKill;
	int nKilled = s_Quest.nKilled > 0 ? s_Quest.nKilled : 0;
	int nMin = (int)((dwNow - s_dwQuestStart) / 60000);
	char szLog[QS_LINE];
	if (O.m_bCancelLong && O.m_nCancelTime > 0 && nNeed > 0 && nKilled * 2 < nNeed &&
		(nMin >= O.m_nCancelTime || s_nTownTrips >= QS_TRIP_CANCEL))
	{
		if (s_nStep == qs_kill && !s_bProbed)
		{	// Truoc khi huy: ve bam Hoan thanh thu 1 lan (server co the khong bao tien do). Chua xong thi huy o lan sau
			_snprintf(szLog, sizeof(szLog) - 1, TV(L"Diệt quái %d phút, mới %d / %d => về Dã Tẩu bấm Hoàn thành thử trước khi hủy\n"), nMin, nKilled, nNeed);
			szLog[sizeof(szLog) - 1] = 0;
			theQuest.SaveYSLog(szLog);
			StepMsg("%d phut moi %d / %d - ve Da Tau bam Hoan thanh thu truoc khi huy", nMin, nKilled, nNeed);
			ProbeStart();
			return;
		}	// Qua N phut / ve thanh nhieu lan ma chua diet duoc 50%: ve Da Tau huy
		_snprintf(szLog, sizeof(szLog) - 1, TV(L"Diệt quái %d phút, về thành %d lần, mới %d / %d (< 50%%) => Hủy nhiệm vụ!\n"),
			nMin, s_nTownTrips, nKilled, nNeed);
		szLog[sizeof(szLog) - 1] = 0;
		theQuest.SaveYSLog(szLog);
		StepEnd(FALSE, "%d phut, ve thanh %d lan, moi %d / %d (< 50%%) - huy nhiem vu", nMin, s_nTownTrips, nKilled, nNeed);
		s_nAutoResult = qk_cancel;
		return;
	}
	if (!O.m_bLongCheck || O.m_nCheckTime <= 0) return;
	if (dwNow - s_dwLongCheckTick < (DWORD)O.m_nCheckTime * 60000) return;
	s_dwLongCheckTick = dwNow;
	_snprintf(szLog, sizeof(szLog) - 1, TV(L"Kiểm tra định kỳ: %d phút, %s, tiến độ %d / %d, về thành %d lần\n"),
		nMin, s_szStepName[s_nStep], nKilled, nNeed, s_nTownTrips);
	szLog[sizeof(szLog) - 1] = 0;
	theQuest.SaveYSLog(szLog);
	StepMsg("kiem tra dinh ky: %d phut, tien do %d / %d, ve thanh %d lan", nMin, nKilled, nNeed, s_nTownTrips);
	if (s_nStep != qs_kill) return;	// Dang mua / di: so tien do o lan kiem tra sau
	if (nKilled != s_nLongKilled)
	{
		s_nLongKilled = nKilled;
		return;
	}
	// Tien do khong tang (co nhiem vu server khong bao so quai da giet len kenh chat): ve Da Tau bam Hoan thanh thu.
	// Hien bang phan thuong = xong; chua xong thi dong cua so, bam lai Da Tau xem tinh hinh roi danh tiep (ProbeBack)
	_snprintf(szLog, sizeof(szLog) - 1, TV(L"Kiểm tra định kỳ: %d phút tiến độ không tăng (%d / %d) => về Dã Tẩu bấm Hoàn thành thử\n"),
		O.m_nCheckTime, nKilled, nNeed);
	szLog[sizeof(szLog) - 1] = 0;
	theQuest.SaveYSLog(szLog);
	StepMsg("%d phut khong tang tien do (%d / %d) - ve Da Tau bam Hoan thanh thu", O.m_nCheckTime, nKilled, nNeed);
	ProbeStart();
}

// Goi tu TimerProc (200ms, ca khi tam dung Auto)
void DebugQuestBreathe()
{
	static DWORD s_dwPauseTick = 0;	// Luc bat dau tam dung (0 = dang chay)
	if (!s_nStep) { s_dwPauseTick = 0; return; }
	if (g_NetworkOffline || !g_pCurPlayer || !P_PLAYER || !P_PLAYER->m_dwID) return;
	// Auto goi ma tat "Tu lam nhiem vu" (tab Da Tau): thoi, bat lai thi CVLQuest goi QuestKillStart lam tiep
	if (s_bAutoQuest && !theQuest.m_bAutoDoQuest)
	{
		QuestKillStop();
		return;
	}
	if (s_nStep == qs_home && !theQuest.m_bAutoDoQuest)
	{	// Tat lai khi dang ve Tuong Duong: thoi (khong bao loi)
		if (PathTravelActive()) PathTravelStop("tat Tu lam nhiem vu");
		StepMsg("tat Tu lam nhiem vu - thoi ve Tuong Duong");
		s_nStep = qs_none;
		return;
	}
	// Auto tam ngung (Ctrl+DblClick phai): dung han - khong di, khong danh, khong dem gio qua han
	if (g_SysPause)
	{
		if (!s_dwPauseTick)
		{
			s_dwPauseTick = GetTickCount() | 1;
			StepMsg("Auto tam ngung - %s tam dung", s_szStepName[s_nStep]);
		}
		return;
	}
	if (s_dwPauseTick)
	{	// Mo lai: lui moc gio buoc / giai doan bang thoi gian tam dung, MapPath lap ke hoach lai tu cho dang dung
		DWORD dwPaused = GetTickCount() - s_dwPauseTick;
		s_dwPauseTick = 0;
		s_dwStepStart += dwPaused;
		s_dwStageTick += dwPaused;
		if (s_dwQuestStart) s_dwQuestStart += dwPaused;	// Kiem tra dai han khong tinh luc tam dung
		s_dwLongCheckTick += dwPaused;
		if (PathTravelActive()) PathTravelResume();
		StepMsg("Auto mo lai sau %ds - lam tiep %s", dwPaused / 1000, s_szStepName[s_nStep]);
	}
	if (++s_nTick % 3) return;	// 600ms

	if (GetTickCount() - s_dwStepStart > (DWORD)s_nStepTimeout[s_nStep] * 1000)
	{
		StepEnd(FALSE, "qua %d giay", s_nStepTimeout[s_nStep]);
		return;
	}
	KillLongCheck();
	if (!s_nStep) return;
	switch (s_nStep)
	{
	case qs_get:	StepGet();		break;
	case qs_shop:	StepShop();		break;
	case qs_gomap:	StepGoMap();	break;
	case qs_kill:	StepKill();		break;
	case qs_return:	StepReturn();	break;
	case qs_home:	StepHome();		break;
	}
}

// CVLMove::Breathe / IsNpcInRange: Buoc 3, 4 dang chay hoac vua xong o map nay => khong di theo danh sach toa do
BOOL DebugQuestHoldMove()
{
	if (s_nStep == qs_gomap || s_nStep == qs_kill || s_nStep == qs_home) return TRUE;
	return s_nHoldMap && s_nHoldMap == *g_pMapID;
}

BOOL DebugQuestActive()
{
	return s_nStep != qs_none;
}

void DebugQuestMoveAllow(BOOL bAllow)
{
	s_nMoveAllow += bAllow ? 1 : -1;
	if (s_nMoveAllow < 0) s_nMoveAllow = 0;
}

// KPlayer::MoveTo / GoTo: Buoc 4 dang chay / giu dung tai map => chi Chien dau, Nhat do, tab Nhiem vu duoc ra lenh di
BOOL DebugQuestMoveOK(void *pCaller)
{
	if (s_nMoveAllow > 0) return TRUE;
	if (s_nStep != qs_kill && !(s_nHoldMap && s_nHoldMap == *g_pMapID)) return TRUE;
	s_nMoveBlocked++;
	HMODULE hMod = NULL;
	GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT, (LPCSTR)pCaller, &hMod);
	s_dwMoveBlockAddr = (DWORD)pCaller - (DWORD)hMod;
	return FALSE;
}

// KAliveNpc::RefreshNpcs: Buoc 4 dang chay thi chi nhan quai nhiem vu
BOOL DebugQuestTargetOK(const char *szNpcName)
{
	if (s_nStep != qs_kill || !s_szKillName[0]) return TRUE;
	return NameMatch(szNpcName, s_szKillName);
}

//============================== Nut bam ==============================

static void OutStepLog()
{
	int nNum = s_nLogTotal < QS_LOG ? s_nLogTotal : QS_LOG;
	if (!nNum) return;
	DbgOut("Nhat ky 3 buoc (cu -> moi):\n");
	for (int k = 0; k < nNum; k++)
		DbgOut("  %s\n", s_StepLog[(s_nLogNext - nNum + k + QS_LOG) % QS_LOG]);
}

static void OutPause()
{
	DbgOut("Auto: %s\n", (g_SysPause || theMain.m_bPause) ? "TAM DUNG" : "dang chay");
}

// In tinh hinh lien quan toi buoc nStep (truoc khi chay)
static void StepCheck(int nStep)
{
	int nMap = *g_pMapID;
	DbgOut("Map %d (%s), %s\n", nMap, g_szMapName, IsTown() ? "trong thanh" : "ngoai thanh (m_FightMode = 1)");
	OutPause();
	const char *szInfo = InfoText();
	if (szInfo[0]) DbgOut("Bang thong bao (g_UiInformation2): %s\n", szInfo);
	YS_DIALOG D;
	switch (nStep)
	{
	case qs_get:
	case qs_return:
	{
		int nDist;
		int nIdx = FindYeSou(&nDist);
		if (nIdx) DbgOut("NPC Da Tau: idx %d '%s', cach %d\n", nIdx, g_pNpcArray[nIdx]->Name, nDist);
		else DbgOut("NPC Da Tau: khong thay quanh day%s\n", nMap != city_tuongduong ? " (chi o Tuong Duong 78)" : "");
		if (GetDialog(D)) ReadDialog(D, TRUE);
		if (nStep == qs_return)
		{
			KUiPrizeSel *pPrzSel = GetPrizeSel();
			if (pPrzSel) PrintPrize(pPrzSel);
			DbgOut("Tho dia phu: %d\n", g_pCurPlayer->GetItemsCount(AMULET_PAPER));
		}
		break;
	}
	case qs_shop:
	{
		DbgOut("Tho dia phu: co %d, mua toi %d (m_bBuyTDP = %d)\n", g_pCurPlayer->GetItemsCount(AMULET_PAPER),
			theMap.m_nAMNumbers, theMap.m_bBuyTDP);
		for (int i = 0; i < 3; i++)
		{
			int nPos = theMap.m_TradeItem[i].nItemPos;
			if (nPos < 0 || nPos >= MAX_SHOP_ITEMS) continue;
			DbgOut("Thuoc %d: %s '%s' co %d, mua toi %d\n", i, theMap.m_TradeItem[i].bBuyCheck ? "MUA" : "khong mua",
				theMap.m_ShopItem[nPos].szItemName, g_pCurPlayer->GetItemsCount(item_medicine, theMap.m_ShopItem[nPos].nItemType),
				theMap.m_TradeItem[i].nBuyAmout);
		}
		if (EnsureCityMap())
			DbgOut("NpcPoints: Tap hoa (%d, %d)  Hieu thuoc (%d, %d)\n", theMap.NpcPoints[npc_Groceries].x, theMap.NpcPoints[npc_Groceries].y,
				theMap.NpcPoints[npc_Pharmacy].x, theMap.NpcPoints[npc_Pharmacy].y);
		else
			DbgOut("Khong co muc [NPC] trong %s\n", PathFileShow(nMap));
		break;
	}
	case qs_gomap:
	{
		char szTv[256];
		PathTravelInfo(szTv, sizeof(szTv));
		DbgOut("MapPath tu di: %s\n", szTv);
		break;
	}
	case qs_kill:
	{
		DbgOut("Tien do: %d / %d (kenh Cong bo%s), task 1027 = %d\n", s_Quest.nKilled, s_Quest.nKillNeed,
			s_bWatchOn ? "" : " - CHUA BAT theo doi", g_pCurPlayer->GetTaskVal(ys_shxtmap));
		DbgOut("Tu danh quai (Ctrl+A): %s, tam tim %d\n", theAttack.m_bAutoHit ? "bat" : "TAT", theAttack.m_nSearchRange);
		if (s_Quest.szMonster[0])
		{
			int nNear, nNearDist;
			int nCount = CountMonsters(s_Quest.szMonster, &nNear, &nNearDist);
			DbgOut("Quai '%s' quanh day: %d con", s_Quest.szMonster, nCount);
			if (nNear) DbgOut(", gan nhat idx %d cach %d", nNear, nNearDist);
			DbgOut("\n");
		}
		break;
	}
	}
	OutQuestInfo();
}

// Kiem tra dieu kien bat dau; FALSE = in ly do
static BOOL StepCanStart(int nStep, int nIdx)
{
	switch (nStep)
	{
	case qs_get:
		if (!EnsureWatch()) DbgOut("Chu y: khong hook duoc thong bao (khong biet su kien nhan nhiem vu)\n");
		return TRUE;
	case qs_shop:
		if (!IsTown()) { DbgOut("LOI: dang o ngoai thanh, ve thanh truoc (Tho dia phu)\n"); return FALSE; }
		return TRUE;
	case qs_gomap:
		s_nTarget = s_bQuestInit ? s_Quest.nMapID : 0;
		if (!s_nTarget)
		{
			DbgOut("LOI: chua biet map nhiem vu (lam Buoc 1 truoc%s)\n",
				s_bQuestInit && s_Quest.szMap[0] ? ", hoac ten map khong co trong Maps\\MapNames.ini" : "");
			return FALSE;
		}
		return TRUE;
	case qs_kill:
		if (!s_bQuestInit || s_Quest.nKill <= 0 || !s_Quest.szMonster[0])
		{
			DbgOut("LOI: chua co nhiem vu diet quai (lam Buoc 1 truoc)\n");
			return FALSE;
		}
		s_nTarget = s_Quest.nMapID;
		if (s_nTarget && *g_pMapID != s_nTarget)
		{
			DbgOut("LOI: chua o map nhiem vu %d (%s), Buoc 2 tu di toi map\n", s_nTarget, s_Quest.szMap);
			return FALSE;
		}
		if (!EnsureWatch())
		{
			DbgOut("LOI: khong hook duoc ChannelMessageArrival - khong biet tien do diet quai\n");
			return FALSE;
		}
		theMove.GoTrainStop();		// Khong di theo danh sach toa do luyen cong / tab Di chuyen
		theMove.m_bFindingWay = 0;
		theMove.m_nStatus = status_idle;
		if (PathTravelActive()) PathTravelStop("tab Nhiem vu Buoc 2");
		if (RoamLoad() && s_bRoamList)
			StepMsg("di lan luot %d diem MapDaTau.ini, ban kinh danh %d", s_nRoamPts, s_nKillRange);
		else if (s_nRoamPts)
			DbgOut("Di vong theo MapPath map %d: %d diem (bo %d diem gan cong)\n", *g_pMapID, s_nRoamPts, s_nRoamDoorSkip);
		else
			DbgOut("Chu y: map %d chua co MapPath (Maps\\Path)%s - chi danh quai thay duoc tai cho\n", *g_pMapID,
				s_nRoamDoorSkip ? " ngoai cac diem gan cong" : "");
		if (s_Quest.nKillNeed <= 0) s_Quest.nKillNeed = s_Quest.nKill;
		if (s_Quest.nKilled < 0) s_Quest.nKilled = 0;
		strncpy(s_szKillName, s_Quest.szMonster, sizeof(s_szKillName) - 1);
		s_szKillName[sizeof(s_szKillName) - 1] = 0;
		DbgOut("Chi danh quai ten chua '%s'\n", s_szKillName);
		s_nMoveBlocked = 0;
		s_dwMoveBlockAddr = 0;
		{	// Dang danh quai khac: bo
			int nPeople = PEOPLE_IDX;
			if (nPeople > 0 && nPeople < MAX_NPC && g_pNpcArray[nPeople]->m_Kind == kind_normal && !NameMatch(g_pNpcArray[nPeople]->Name, s_szKillName))
				theAttack.SetPeopleIdx(0);
			theAttack.m_LastNpcID = 0;
		}
		if (!theAttack.m_bAutoHit)
		{
			theMain.PostFeedBack(cmd_autohit);	// VLAuto dao "Tu danh quai" roi gui lai tham so Chien dau
			DbgOut("Bat Tu danh quai (Ctrl+A)\n");
		}
		if (g_SysPause)
			DbgOut("Auto dang tam dung: tab Nhiem vu tu chay Chien dau (Phuc hoi, Nhat do van chay)\n");
		return TRUE;
	case qs_return:
		if (nIdx < 0 || nIdx > 2)
		{
			DbgOut("LOI: Idx = %d, o phan thuong phai 0..2 (0 trai = Kinh nghiem, 1 giua = Ngan luong, 2 phai = Ngau nhien)\n", nIdx);
			return FALSE;
		}
		s_nTarget = nIdx;
		if (s_Quest.nKillNeed > 0 && s_Quest.nKilled >= 0 && s_Quest.nKilled < s_Quest.nKillNeed)
			DbgOut("Chu y: tien do %d / %d chua du, van ve thu\n", s_Quest.nKilled, s_Quest.nKillNeed);
		EnsureWatch();
		return TRUE;
	case qs_home:
		return TRUE;
	}
	return FALSE;
}

// Bat dau giai doan nStep (kiem tra dieu kien, dat lai bien). FALSE = khong bat dau duoc (ly do in bang DbgOut)
static BOOL StepStart(int nStep)
{
	if (!StepCanStart(nStep, s_nPrize)) return FALSE;
	s_nStep = nStep;
	s_nHoldMap = 0;
	s_dwStepStart = GetTickCount();
	SetStage(0);
	s_nTick = 0;
	s_nClicks = s_nNpcClicks = 0;
	s_dwClickTick = s_dwWaitMapTick = 0;
	s_nWaitMap = 0;
	s_bUsedTDP = s_bTravelStarted = s_bHomeItem = FALSE;
	s_nGoTry = 0;
	s_dwTripCheck = GetTickCount();
	s_dwReportTick = GetTickCount();
	s_dwNoMonsterTick = 0;
	s_bNoMonsterSaid = FALSE;
	s_nCityMap = 0;
	s_szLastDialog[0] = 0;
	StepMsg("bat dau %s", s_szStepName[nStep]);
	DbgOut("=> BAT DAU %s (toi da %d giay)\n", s_szStepName[nStep], s_nStepTimeout[nStep]);
	return TRUE;
}

// Auto chuyen thang sang buoc nStep (khong qua StepEnd). Khong bat dau duoc = loi
static BOOL StepStartAuto(int nStep)
{
	if (StepStart(nStep)) return TRUE;
	StepMsg("khong bat dau duoc %s", s_szStepName[nStep]);
	s_bChain = FALSE;
	if (s_bAutoQuest) s_nAutoResult = qk_failed;
	return FALSE;
}

// Buoc 2: diet nCount quai ten szNpcName o map nMapID = mua mau/mana/TDP (neu dang trong thanh) -> MapPath toi map -> danh quai.
// TRUE = da bat dau
static BOOL DoQuestKillNpc(int nCount, int nMapID, const char *szNpcName)
{
	if (nCount <= 0 || !szNpcName || !szNpcName[0])
	{
		DbgOut("LOI: chua co nhiem vu diet quai (lam Buoc 1 truoc)\n");
		StepMsg("Buoc 2: chua co nhiem vu diet quai");
		return FALSE;
	}
	if (szNpcName != s_Quest.szMonster)
	{
		strncpy(s_Quest.szMonster, szNpcName, sizeof(s_Quest.szMonster) - 1);
		s_Quest.szMonster[sizeof(s_Quest.szMonster) - 1] = 0;
	}
	s_Quest.nKill = nCount;
	s_Quest.nMapID = nMapID;
	if (s_Quest.nKillNeed > 0 && s_Quest.nKilled >= s_Quest.nKillNeed)
	{	// Da du quai: sang Buoc 3
		StepMsg("Buoc 2: da du %d / %d %s", s_Quest.nKilled, s_Quest.nKillNeed, s_Quest.szMonster);
		StepNext(qs_kill);
		return TRUE;
	}
	int nFirst;
	if (nMapID && *g_pMapID == nMapID) nFirst = qs_kill;
	else if (IsTown()) nFirst = qs_shop;
	else if (nMapID) nFirst = qs_gomap;
	else
	{
		DbgOut("LOI: khong biet MapID cua map '%s' (Maps\\MapNames.ini)\n", s_Quest.szMap);
		StepMsg("Buoc 2: khong biet MapID cua map '%s'", s_Quest.szMap);
		return FALSE;
	}
	StepMsg("Buoc 2: diet %d %s o map %d (%s)", nCount, s_Quest.szMonster, nMapID, s_Quest.szMap);
	if (StepStart(nFirst)) return TRUE;
	StepMsg("khong bat dau duoc %s (bam nut Buoc 2 xem ly do)", s_szStepName[nFirst]);
	s_bChain = FALSE;
	return FALSE;
}

// Xong giai doan nDone (thanh cong): chuyen giai doan / nut sau
static void StepNext(int nDone)
{
	int nNext = qs_none;
	switch (nDone)
	{
	case qs_get:
		if (!s_bChain) return;
		if (s_Quest.nKill <= 0 || !s_Quest.szMonster[0])
		{
			s_bChain = FALSE;
			StepMsg("nhiem vu khong phai diet quai (%s) - chua tu lam duoc, dung", QuestSummary());
			return;
		}
		DoQuestKillNpc(s_Quest.nKill, s_Quest.nMapID, s_Quest.szMonster);
		return;
	case qs_shop:	nNext = qs_gomap;	break;	// Buoc 2 luon chay het 3 giai doan
	case qs_gomap:	nNext = qs_kill;	break;
	case qs_kill:
		if (!s_bChain) return;
		nNext = qs_return;
		break;
	default:
		s_bChain = FALSE;
		return;
	}
	if (StepStart(nNext)) return;
	StepMsg("khong bat dau duoc %s (bam nut Buoc %d xem ly do)", s_szStepName[nNext], StepButton(nNext));
	s_bChain = FALSE;
}

//============================== Auto: CVLQuest::DoQuestAttackNpc ==============================
// Chay Buoc 2 (mua -> di toi map -> danh quai) roi Buoc 3 (ve Da Tau, chon Hoan thanh) nhu bam nut,
// dung lai khi bang phan thuong hien de CVLQuest::ChoosePrize chon o.

BOOL QuestKillStart(int nCount, int nMapID, const char *szNpcName, const char *szMapName)
{
	if (!s_bQuestInit) ResetQuestInfo();
	if (s_nStep) StepEnd(FALSE, "Auto bat dau lai nhiem vu diet quai");
	EnsureWatch();	// Tien do "Tieu diet: a / b" chi co trong tin kenh chat server
	// Nhiem vu khac lan truoc: bo tien do cu (server bao lai o con quai ke tiep)
	BOOL bNew = s_Quest.nKill != nCount || s_Quest.nMapID != nMapID || !NameMatch(s_Quest.szMonster, szNpcName);
	if (bNew)
		s_Quest.nKilled = s_Quest.nKillNeed = -1;
	if (bNew || !s_dwQuestStart)
	{	// Nhiem vu moi (lam lai sau loi thi giu): moc gio / so lan ve thanh cho kiem tra dai han
		s_dwQuestStart = s_dwLongCheckTick = GetTickCount();
		s_nTownTrips = 0;
		s_nLongKilled = 0;	// Kiem tra dau tien van 0 = server khong bao tien do -> ve bam Hoan thanh thu
		s_bProbed = FALSE;
	}
	KQLoad(nMapID, &s_KQ);	// Ruong / diem / ban kinh danh (Maps\MapDaTau.ini + tab Danh quai)
	strncpy(s_Quest.szMap, szMapName, sizeof(s_Quest.szMap) - 1);
	s_Quest.szMap[sizeof(s_Quest.szMap) - 1] = 0;
	s_bAutoQuest = TRUE;
	s_nAutoResult = qk_running;
	s_bChain = TRUE;
	if (DoQuestKillNpc(nCount, nMapID, szNpcName)) return TRUE;
	s_nAutoResult = qk_failed;
	return FALSE;
}

int QuestKillState(int *pKilled, int *pNeed)
{
	if (pKilled) *pKilled = s_Quest.nKilled;
	if (pNeed) *pNeed = s_Quest.nKillNeed;
	if (!s_bAutoQuest) return qk_idle;
	if (s_nStep) return qk_running;
	if (s_nAutoResult == qk_done || s_nAutoResult == qk_cancel) return s_nAutoResult;
	return qk_failed;	// Chuoi buoc dung giua chung (khong bat dau duoc buoc sau) = loi
}

void QuestKillStop()
{
	if (!s_bAutoQuest) return;
	if (s_nStep) StepEnd(FALSE, "Auto thoi lam nhiem vu diet quai");
	s_bAutoQuest = FALSE;
	s_nAutoResult = qk_idle;
	s_nHoldMap = 0;
	s_dwQuestStart = 0;	// Lan sau la nhiem vu moi (ke ca trung ten / map)
}

void QuestKillRestart()
{
	if (s_bAutoQuest) QuestKillStop();
	else if (s_nStep) StepEnd(FALSE, "bat lai Tu lam nhiem vu");
	s_Quest.nKilled = s_Quest.nKillNeed = -1;
	s_Quest.nKill = 0;
	s_Quest.szMonster[0] = 0;
	s_Quest.nMapID = 0;
	s_nTownTrips = 0;
	s_dwQuestStart = 0;
	s_nHoldMap = 0;
	s_bChain = FALSE;
	memset(&s_KQ, 0, sizeof(s_KQ));
	if (g_NetworkOffline || !g_pCurPlayer || !P_PLAYER || !P_PLAYER->m_dwID) return;
	if (*g_pMapID == city_tuongduong) return;
	if (!StepStart(qs_home))
	{
		theQuest.SaveYSLog(TV(L"Bật lại Tự làm nhiệm vụ: không về được Tương Dương => Lỗi!\n"));
		SystemMessage(TV(L"<color=yellow>Không về được Tương Dương!"));
	}
}

BOOL QuestKillHoming()
{
	return s_nStep == qs_home;
}

const char* QuestKillLastMsg()
{
	if (!s_nLogTotal) return "";
	return s_StepLog[(s_nLogNext + QS_LOG - 1) % QS_LOG];
}

// Nut "Buoc N" (1 nhan, 2 diet quai, 3 tra): X = 1 chi kiem tra, X = 2 chi lam buoc nay (khong tu sang buoc sau);
// Idx = o phan thuong Buoc 3. Dang chay buoc nay thi dung
static void StepCmd(int nBtn, int nX, int nIdx)
{
	if (!DbgCheckPlayer()) return;
	g_DbgReply.bOK = TRUE;
	s_bAutoQuest = FALSE;	// Nut bam: Buoc 3 tu chon o phan thuong
	if (s_nStep && StepButton(s_nStep) == nBtn)
	{
		DbgOut("=> DA DUNG %s\n", s_szStepName[s_nStep]);
		StepEnd(FALSE, "dung theo yeu cau");
		OutStepLog();
		return;
	}
	if (!s_bQuestInit) ResetQuestInfo();
	static const char *szBtn[4] = { "", "Buoc 1: nhan nhiem vu", "Buoc 2: DoQuestKillNpc (mua -> di toi map -> danh quai)", "Buoc 3: tra nhiem vu" };
	DbgOut("=== %s ===\n", szBtn[nBtn]);
	StepCheck(nBtn == 1 ? qs_get : nBtn == 3 ? qs_return : qs_kill);
	if (nX == 1)
	{
		DbgOut("X = 1: chi kiem tra, khong chay\n");
		return;
	}
	if (nIdx < 0 || nIdx > 2)
	{
		DbgOut("LOI: Idx = %d, o phan thuong phai 0..2 (0 trai = Kinh nghiem, 1 giua = Ngan luong, 2 phai = Ngau nhien)\n", nIdx);
		return;
	}
	if (s_nStep) StepEnd(FALSE, "chuyen sang %s", szBtn[nBtn]);
	s_nPrize = nIdx;
	s_bChain = (nX != 2);
	BOOL bStarted;
	if (nBtn == 2) bStarted = DoQuestKillNpc(s_Quest.nKill, s_Quest.nMapID, s_Quest.szMonster);
	else bStarted = StepStart(nBtn == 1 ? qs_get : qs_return);
	if (!bStarted)
	{
		s_bChain = FALSE;
		return;
	}
	DbgOut("\n=> %s. Tien trinh bao o kenh chat, xem lai bang nut 'Trang thai'. Bam lai nut nay de dung.\n",
		s_bChain && nBtn < 3 ? "Xong tu chuyen buoc sau toi het Buoc 3 (X = 2: chi lam buoc nay)" : "Chi lam buoc nay");
	if (nBtn < 3 && s_bChain)
		DbgOut("Buoc 3 se chon o phan thuong %d (%s)\n", s_nPrize, s_nPrize == 0 ? "Kinh nghiem" : s_nPrize == 1 ? "Ngan luong" : "Ngau nhien");
	g_DbgReply.nResult[0] = s_nStep;
}

//============================== Trang thai ==============================

static void StatusCmd()
{
	if (!DbgCheckPlayer()) return;
	DbgOut("Dang chay: %s", s_nStep ? s_szStepName[s_nStep] : "khong");
	if (s_nStep) DbgOut(" (%u giay, giai doan %d)", (GetTickCount() - s_dwStepStart) / 1000, s_nStage);
	if (s_nHoldMap) DbgOut(", giu dung tai map %d (Di chuyen khong chay danh sach toa do)", s_nHoldMap);
	if (s_nMoveBlocked) DbgOut(", da chan %d lenh di cua phan khac (gan nhat VLCTCdll+0x%X)", s_nMoveBlocked, s_dwMoveBlockAddr);
	DbgOut("\nMap %d (%s), %s. ", *g_pMapID, g_szMapName, IsTown() ? "trong thanh" : "ngoai thanh");
	OutPause();
	char szTv[256];
	PathTravelInfo(szTv, sizeof(szTv));
	DbgOut("MapPath tu di: %s\n", szTv);
	DbgOut("Theo doi thong bao: %s, %d thong bao. Task 1027 = %d\n", s_bWatchOn ? "DANG BAT" : "tat", s_nQTotal,
		g_pCurPlayer->GetTaskVal(ys_shxtmap));
	OutQuestInfo();
	OutStepLog();
	if (s_szLastDialog[0]) DbgOut("Hop thoai Da Tau chua biet (lan dung gan nhat):\n%s", s_szLastDialog);
	g_DbgReply.nResult[0] = s_nStep;
	g_DbgReply.bOK = TRUE;
}

// Idx = so task; X > Idx: liet ke task Idx..X khac 0
static void TaskValCmd(int nTask, int nLast)
{
	if (!DbgCheckPlayer()) return;
	if (nLast <= nTask)
	{
		int v = g_pCurPlayer->GetTaskVal(nTask);
		DbgOut("g_pCurPlayer->GetTaskVal(%d) = %d (0x%08X)\n", nTask, v, v);
		g_DbgReply.nResult[0] = v;
		g_DbgReply.bOK = TRUE;
		return;
	}
	if (nTask < 0) nTask = 0;
	if (nLast > 0x176F) nLast = 0x176F;	// GetTaskVal 0x609990: n > 0x176F tra ve 0
	DbgOut("Task %d..%d khac 0:\n", nTask, nLast);
	int nShown = 0;
	for (int t = nTask; t <= nLast; t++)
	{
		int v = g_pCurPlayer->GetTaskVal(t);
		if (!v) continue;
		DbgOut(" [%d]=%d", t, v);
		if (++nShown % 8 == 0) DbgOut("\n");
	}
	DbgOut(nShown % 8 ? "\n=> %d task khac 0\n" : "=> %d task khac 0\n", nShown);
	g_DbgReply.nResult[0] = nShown;
	g_DbgReply.bOK = TRUE;
}

BOOL DebugQuestCmd(const DEBUG_PARAM *pParam)
{
	const int *P = pParam->nParam;
	const char *szText = pParam->szParam;
	switch (pParam->nCmd)
	{
	case dbg_qt_status:
		StatusCmd();
		break;
	case dbg_qt_step1:
		StepCmd(1, P[0], P[2]);
		break;
	case dbg_qt_step2:
		StepCmd(2, P[0], P[2]);
		break;
	case dbg_qt_step3:
		StepCmd(3, P[0], P[2]);
		break;
	case dbg_qt_stop:
		s_bChain = FALSE;
		if (s_nStep)
		{
			DbgOut("=> DUNG %s\n", s_szStepName[s_nStep]);
			StepEnd(FALSE, "dung theo yeu cau");
		}
		else
			DbgOut("Khong co buoc nao dang chay\n");
		if (s_nHoldMap) DbgOut("Bo giu dung tai map %d: Di chuyen chay lai binh thuong\n", s_nHoldMap);
		s_nHoldMap = 0;
		OutStepLog();
		g_DbgReply.bOK = TRUE;
		break;
	case dbg_qt_watch:
		WatchToggle(P[0]);
		break;
	case dbg_qt_watchview:
		WatchView(P[0], szText);
		break;
	case dbg_qt_taskval:
		TaskValCmd(P[2], P[0]);
		break;
	case dbg_qt_reset:
		ResetQuestInfo();
		s_nLogNext = s_nLogTotal = 0;
		s_szLastDialog[0] = 0;
		s_nHoldMap = 0;
		DbgOut("Da xoa ket qua phan tich nhiem vu + nhat ky 3 buoc\n");
		g_DbgReply.bOK = TRUE;
		break;
	default:
		return FALSE;
	}
	return TRUE;
}
