#include "stdafx.h"
#include "../GameDef.h"
#include "GameBase.h"
#include "JxCore.h"
#include "S3Client.h"
#include "VLClasses.h"
#include "VLDebug.h"

// Tab Ban do - nut "Ghi file/lua game nap": chen hook vao cac ham export cua engine.dll
// (KPakFile::Open, KFile::Open, KLuaScript::Load/LoadBuffer/DoString/CallFunction) va ghi lai ten file / ham lua
// + dia chi goi (return address) => bam vao NPC luc dang bat de biet client nap gi, lay lam diem neo do tim.
// + CoreDataChanged cua exe (core bao UI: id 64 = hop thoai NPC KUiQuestionAndAnswer -> KUiMsgSel2::OpenWindow)
// + task value thay doi (so sanh 1..5999 moi 200ms, DebugFileBreathe).
// + KUiMsgCentrePad::SystemMessageArrival(msg, len, kenh) 0x4BD680: dong chu khung chat he thong
//   (kenh 0 = khung thong bao duoi, kenh 1 = dong cua Auto).
// + KClientCallback::ChannelMessageArrival 0x40DA10 (vtable 0x7600B0 [1]): tin kenh chat tu server
//   ("Cong bo: Nhiem vu Da Tau -Tieu diet: 1 / 50", hien o tab Tat ca / Khac).
// Hook: 5 byte dau ham -> jmp stub; stub = pushad, goi FileLogHook(id, ecx, esp luc vao ham), popad,
// chay lai cac byte da chep (chi chep lenh khong nhay tuong doi, kiem tra byte truoc khi patch), jmp ve ham.

#define FLOG_MAX		2000	// so dong toi da (vong tron)
#define FLOG_TEXT		400
#define FLOG_DEDUP		16		// trung voi 1 trong 16 dong cuoi => chi tang so lan
#define FLOG_TASK		0xFF	// nHook cua dong "task value doi"
#define FLOG_TASK_MAX	0x1770	// KPlayerTask::GetTaskVal 0x609990: n > 0x176F tra ve 0

// 15-10: CoreDataChanged(uDataId, uParam, nParam) - switch (uDataId - 1) bang 0x40CC1C, 0xB3 truong hop
#define CDC_FUNC_ADD	0x00409C20
#define CDC_QUESTION	64		// 0x40ABAB: KUiMsgSel2::OpenWindow 0x5210C0(uParam = KUiQuestionAndAnswer*)

// enum flk_... : flk_sysmsg = SystemMessageArrival (SMA_FUNC_ADD, __cdecl: [esp+4] msg, [esp+8] len (WORD), [esp+C] kenh)
// flk_chanmsg = KClientCallback::ChannelMessageArrival (__thiscall, ret 18h): [esp+4] nChannelID, [esp+8] szSendName,
//   [esp+C] pMsgBuff, [esp+10h] nMsgLength (WORD), [esp+14h] bSucc (BYTE), [esp+18h] ? (15-10 them 1 tham so)
#define CMA_FUNC_ADD	0x0040DA10

enum { flk_string = 0, flk_coredata, flk_sysmsg, flk_chanmsg };

struct FLOG_HOOK
{
	const char	*szExport;		// ten export (decorated) trong engine.dll, NULL = dia chi co dinh trong exe
	const char	*szLabel;
	DWORD		dwAddr;			// khi szExport = NULL
	BYTE		nKind;			// flk_...
	BYTE		nArgOff;		// vi tri chuoi tren stack luc vao ham (esp+n, [esp] = return address)
	BYTE		nCopy;			// so byte dau ham chep sang stub (ranh gioi lenh, >= 5)
	BYTE		nExpect;		// so byte phai khop truoc khi patch
	BYTE		Expect[8];
	// Luc chay
	BYTE		*pFunc;
	BYTE		*pStub;
	BYTE		Org[16];
	BOOL		bPatched;
};

// 15_10 engine.dll (disasm): xem Offsets_15_10.md
static FLOG_HOOK s_Hooks[] =
{
	// push esi; mov esi, [esp+8]
	{ "?Open@KPakFile@@QAEHPBD@Z",					"PakOpen",	0, flk_string,	4,  5, 5, { 0x56, 0x8B, 0x74, 0x24, 0x08 } },
	// mov eax, [esp+4]; push imm32
	{ "?Open@KFile@@QAEHPBD@Z",						"FileOpen",	0, flk_string,	4,  9, 5, { 0x8B, 0x44, 0x24, 0x04, 0x68 } },
	// push -1; push imm32
	{ "?Load@KLuaScript@@UAEHPBD@Z",				"LuaLoad",	0, flk_string,	4,  7, 3, { 0x6A, 0xFF, 0x68 } },
	// mov eax, [esp+8]; test eax, eax; push esi  (LoadBuffer(buf, len, ten))
	{ "?LoadBuffer@KLuaScript@@QAEHPAEKPBD@Z",		"LuaBuf",	0, flk_string,	12, 7, 7, { 0x8B, 0x44, 0x24, 0x08, 0x85, 0xC0, 0x56 } },
	// push ecx; push ebx; mov ebx, [abs32]
	{ "?DoString@KLuaScript@@QAEHPBD@Z",			"DoString",	0, flk_string,	4,  8, 4, { 0x51, 0x53, 0x8B, 0x1D } },
	// push ebp; mov ebp, esp; and esp, -64
	{ "?CallFunction@KLuaScript@@QAEHPBDH0PAD@Z",	"CallFn",	0, flk_string,	4,  6, 6, { 0x55, 0x8B, 0xEC, 0x83, 0xE4, 0xC0 } },
	// __cdecl (this tren stack): mov ecx, [esp+10h]; mov edx, [esp+0Ch]
	{ "?CallFunction@KLuaScript@@UAAHPBDH0ZZ",		"CallFnV",	0, flk_string,	8,  8, 8, { 0x8B, 0x4C, 0x24, 0x10, 0x8B, 0x54, 0x24, 0x0C } },
	// exe: push ebp; mov ebp, esp; push -1
	{ NULL,											"CoreData",	CDC_FUNC_ADD, flk_coredata, 0, 5, 5, { 0x55, 0x8B, 0xEC, 0x6A, 0xFF } },
	// exe: push ebp; mov ebp, esp; sub esp, 8
	{ NULL,											"SysMsg",	SMA_FUNC_ADD, flk_sysmsg,   0, 6, 6, { 0x55, 0x8B, 0xEC, 0x83, 0xEC, 0x08 } },
	// exe: push ebp; mov ebp, esp; push -1; push 724640h
	{ NULL,											"ChanMsg",	CMA_FUNC_ADD, flk_chanmsg,  0, 5, 8, { 0x55, 0x8B, 0xEC, 0x6A, 0xFF, 0x68, 0x40, 0x46 } },
};
#define FLOG_HOOKS	(sizeof(s_Hooks) / sizeof(s_Hooks[0]))

struct FLOG_LINE
{
	DWORD	dwTick;
	DWORD	dwRet;
	int		nCount;
	BYTE	nHook;
	char	szText[FLOG_TEXT];
};

static FLOG_LINE		s_Log[FLOG_MAX];
static int				s_nLogNext = 0;		// vi tri ghi tiep
static int				s_nLogTotal = 0;	// tong so dong da ghi (> FLOG_MAX = da ghi de)
static volatile BOOL	s_bLogOn = FALSE;
static DWORD			s_dwLogStart = 0;
static CRITICAL_SECTION	s_csLog;
static BOOL				s_bCsInit = FALSE;
static BOOL				s_bSkipMedia = TRUE;
static volatile BOOL	s_bQuestWatch = FALSE;	// Tab Nhiem vu: CoreDataChanged / SystemMessageArrival -> VLDebugQuest.cpp

// File hinh / am thanh nap lien tuc khi di chuyen: bo qua (chi giu ini/lua/txt/dat...)
static BOOL IsMediaFile(const char *szName)
{
	static const char *s_szExt[] = { ".spr", ".wav", ".mp3", ".jpg", ".bmp", ".tga", ".png", ".pcx", ".gif", ".avi", ".mpg", ".mp4", ".ogg", ".jxv" };
	const char *p = strrchr(szName, '.');
	if (!p) return FALSE;
	for (int i = 0; i < sizeof(s_szExt) / sizeof(s_szExt[0]); i++)
		if (_stricmp(p, s_szExt[i]) == 0) return TRUE;
	return FALSE;
}

// Chep chuoi game (bo ma mau 0x02 r g b / 0x03, doi xuong dong thanh ' '), tra ve so ky tu
static int CopyGameText(char *szOut, int nOut, const char *szText, int nMax)
{
	int n = 0;
	for (int i = 0; i < nMax && szText[i] && n < nOut - 1; i++)
	{
		char c = szText[i];
		if (c == 0x02) { i += 3; continue; }
		if (c == 0x03) continue;
		szOut[n++] = (c == '\r' || c == '\n' || c == '\t') ? ' ' : c;
	}
	szOut[n] = 0;
	return n;
}

// uParam co ve la chuoi (>= 4 ky tu doc duoc)?
static BOOL LooksLikeText(const char *p)
{
	for (int i = 0; i < 4; i++)
		if ((BYTE)p[i] < 0x20 && p[i] != 0x02 && p[i] != 0x03) return FALSE;
	return TRUE;
}

static void AddLine(int nHook, DWORD dwRet, const char *szBuf)
{
	EnterCriticalSection(&s_csLog);
	int nScan = (s_nLogTotal < FLOG_DEDUP) ? s_nLogTotal : FLOG_DEDUP;
	for (int k = 1; k <= nScan; k++)
	{
		FLOG_LINE &L = s_Log[(s_nLogNext - k + FLOG_MAX) % FLOG_MAX];
		if (L.nHook == nHook && L.dwRet == dwRet && strcmp(L.szText, szBuf) == 0)
		{
			L.nCount++;
			LeaveCriticalSection(&s_csLog);
			return;
		}
	}
	FLOG_LINE &L = s_Log[s_nLogNext];
	L.dwTick = GetTickCount() - s_dwLogStart;
	L.dwRet = dwRet;
	L.nCount = 1;
	L.nHook = (BYTE)nHook;
	strncpy(L.szText, szBuf, FLOG_TEXT - 1);
	L.szText[FLOG_TEXT - 1] = 0;
	s_nLogNext = (s_nLogNext + 1) % FLOG_MAX;
	s_nLogTotal++;
	LeaveCriticalSection(&s_csLog);
}

// pStack = esp luc vao ham: [0] return address, [1..] tham so (__thiscall: this o ecx)
static void __cdecl FileLogHook(int nHook, void *pThis, DWORD *pStack)
{
	FLOG_HOOK &H = s_Hooks[nHook];
	if (H.nKind != flk_string && s_bQuestWatch)
	{
		__try
		{
			if (H.nKind == flk_sysmsg)
				DebugQuestSysMsg((const char*)pStack[1], (WORD)pStack[2], (int)pStack[3]);
			else if (H.nKind == flk_chanmsg)
				DebugQuestChannelMsg(pStack[1], (const char*)pStack[2], (const char*)pStack[3], (WORD)pStack[4]);
			else
				DebugQuestCoreData(pStack[1], pStack[2], (int)pStack[3]);
		}
		__except (EXCEPTION_EXECUTE_HANDLER)
		{
		}
	}
	if (!s_bLogOn) return;
	char szBuf[FLOG_TEXT];
	szBuf[0] = 0;
	DWORD dwRet = 0;
	__try
	{
		dwRet = pStack[0];
		if (H.nKind == flk_string)
		{
			const char *szText = (const char*)pStack[H.nArgOff / 4];
			if (szText) CopyGameText(szBuf, sizeof(szBuf), szText, FLOG_TEXT);
		}
		else if (H.nKind == flk_chanmsg)
		{
			const char *szSend = (const char*)pStack[2];
			const char *szText = (const char*)pStack[3];
			int nLen = (WORD)pStack[4];
			int n = _snprintf(szBuf, sizeof(szBuf) - 1, "kenh chat %08X [%.32s]: ", pStack[1], szSend ? szSend : "");
			if (szText && n > 0) CopyGameText(szBuf + n, sizeof(szBuf) - n, szText, nLen < FLOG_TEXT ? nLen : FLOG_TEXT);
		}
		else if (H.nKind == flk_sysmsg)
		{
			const char *szText = (const char*)pStack[1];
			int nLen = (WORD)pStack[2];
			int n = _snprintf(szBuf, sizeof(szBuf) - 1, "kenh %d: ", (int)pStack[3]);
			if (szText && n > 0) CopyGameText(szBuf + n, sizeof(szBuf) - n, szText, nLen < FLOG_TEXT ? nLen : FLOG_TEXT);
		}
		else
		{
			DWORD uId = pStack[1], uParam = pStack[2];
			int nParam = (int)pStack[3];
			int n = _snprintf(szBuf, sizeof(szBuf) - 1, "id %3u u=%08X n=%d", uId, uParam, nParam);
			szBuf[sizeof(szBuf) - 1] = 0;
			if (n > 0 && uParam > 0x10000)
			{
				const char *p = (const char*)uParam;
				if (uId == CDC_QUESTION)
				{
					// KUiQuestionAndAnswer: loi thoai o +0 (Show 0x5217B0 strcat), +0x804 so dong, +0x808, dong tra loi +0x80C (0x104/dong)
					n += _snprintf(szBuf + n, sizeof(szBuf) - 1 - n, " [hoi thoai] +804=%d +808=%d: ",
						*(int*)(p + 0x804), *(int*)(p + 0x808));
					if (n > 0 && n < (int)sizeof(szBuf) - 1) CopyGameText(szBuf + n, sizeof(szBuf) - n, p, 0x800);
				}
				else if (LooksLikeText(p))
				{
					n += _snprintf(szBuf + n, sizeof(szBuf) - 1 - n, " s=\"");
					if (n > 0 && n < (int)sizeof(szBuf) - 2) CopyGameText(szBuf + n, sizeof(szBuf) - n - 1, p, 120);
					strncat(szBuf, "\"", sizeof(szBuf) - 1 - strlen(szBuf));
				}
			}
			szBuf[sizeof(szBuf) - 1] = 0;
		}
	}
	__except (EXCEPTION_EXECUTE_HANDLER)
	{
		strncat(szBuf, " <loi doc>", sizeof(szBuf) - 1 - strlen(szBuf));
	}
	if (H.nKind == flk_string && nHook <= 1 && s_bSkipMedia && IsMediaFile(szBuf)) return;
	AddLine(nHook, dwRet, szBuf);
}

// Task value: so sanh voi lan truoc moi 200ms (luong game, goi tu VLHook.cpp)
static int		s_TaskSnap[FLOG_TASK_MAX];
static BOOL		s_bTaskSnap = FALSE;

static BOOL TaskReady()
{
	return g_pCurPlayer && g_pNpcArray[0] && g_pCurPlayer->m_nIndex > 0 && g_pCurPlayer->m_nIndex < MAX_NPC;
}

void DebugFileBreathe()
{
	if (!s_bLogOn || !TaskReady()) return;
	int nChanged = 0;
	for (int i = 1; i < FLOG_TASK_MAX; i++)
	{
		int nVal = g_pCurPlayer->m_cTask.GetTaskVal(i);
		if (s_bTaskSnap && nVal != s_TaskSnap[i] && nChanged < 64)
		{
			char szBuf[80];
			_snprintf(szBuf, sizeof(szBuf) - 1, "[%d] %d -> %d", i, s_TaskSnap[i], nVal);
			szBuf[sizeof(szBuf) - 1] = 0;
			AddLine(FLOG_TASK, 0, szBuf);
			nChanged++;
		}
		s_TaskSnap[i] = nVal;
	}
	s_bTaskSnap = TRUE;
}

static const char* HookLabel(int nHook)
{
	return (nHook == FLOG_TASK) ? "TaskVal" : s_Hooks[nHook].szLabel;
}

// Tao stub cho hook i (bo nho khong giai phong: tranh luong game dang chay trong stub khi go hook)
static BYTE* BuildStub(int i, FLOG_HOOK &H)
{
	BYTE *s = (BYTE*)VirtualAlloc(NULL, 64, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
	if (!s) return NULL;
	BYTE *p = s;
	*p++ = 0x60;												// pushad
	*p++ = 0x8D; *p++ = 0x44; *p++ = 0x24; *p++ = 0x20;		// lea eax, [esp+20h] (esp luc vao ham)
	*p++ = 0x50;												// push eax
	*p++ = 0x51;												// push ecx (this)
	*p++ = 0x68; *(int*)p = i; p += 4;							// push i
	*p++ = 0xE8; *(int*)p = (int)((DWORD)FileLogHook - (DWORD)(p + 4)); p += 4;	// call FileLogHook
	*p++ = 0x83; *p++ = 0xC4; *p++ = 0x0C;						// add esp, 0Ch
	*p++ = 0x61;												// popad
	memcpy(p, H.pFunc, H.nCopy); p += H.nCopy;					// cac lenh dau ham
	*p++ = 0xE9; *(int*)p = (int)((DWORD)(H.pFunc + H.nCopy) - (DWORD)(p + 4)); p += 4;	// jmp ve ham
	FlushInstructionCache(GetCurrentProcess(), s, p - s);
	return s;
}

static BOOL PatchHook(FLOG_HOOK &H, BOOL bOn)
{
	if (H.bPatched == bOn) return TRUE;
	DWORD dwOld;
	if (!VirtualProtect(H.pFunc, H.nCopy, PAGE_EXECUTE_READWRITE, &dwOld)) return FALSE;
	if (bOn)
	{
		memcpy(H.Org, H.pFunc, H.nCopy);
		BYTE Patch[16];
		memset(Patch, 0x90, sizeof(Patch));
		Patch[0] = 0xE9;
		*(int*)(Patch + 1) = (int)((DWORD)H.pStub - (DWORD)(H.pFunc + 5));
		memcpy(H.pFunc, Patch, H.nCopy);
	}
	else
		memcpy(H.pFunc, H.Org, H.nCopy);
	VirtualProtect(H.pFunc, H.nCopy, dwOld, &dwOld);
	FlushInstructionCache(GetCurrentProcess(), H.pFunc, H.nCopy);
	H.bPatched = bOn;
	return TRUE;
}

// bCoreOnly: chi hook CoreDataChanged + SystemMessageArrival (tab Nhiem vu)
static int InstallHooks(BOOL bCoreOnly = FALSE)
{
	HMODULE hEngine = GetModuleHandleA("engine.dll");
	if (!hEngine && !bCoreOnly)
	{
		DbgOut("LOI: khong thay engine.dll trong tien trinh game\n");
		return 0;
	}
	int nOK = 0;
	for (int i = 0; i < FLOG_HOOKS; i++)
	{
		FLOG_HOOK &H = s_Hooks[i];
		if (bCoreOnly && H.nKind == flk_string) continue;
		if (H.bPatched) { nOK++; continue; }
		if (!H.pFunc) H.pFunc = H.szExport ? (BYTE*)GetProcAddress(hEngine, H.szExport) : (BYTE*)H.dwAddr;
		if (!H.pFunc)
		{
			DbgOut("  %-8s KHONG co export %s\n", H.szLabel, H.szExport ? H.szExport : "?");
			continue;
		}
		if (memcmp(H.pFunc, H.Expect, H.nExpect) != 0)
		{
			DbgOut("  %-8s 0x%08X dau ham khac (sai ban engine.dll?):", H.szLabel, (DWORD)H.pFunc);
			for (int k = 0; k < H.nCopy; k++) DbgOut(" %02X", H.pFunc[k]);
			DbgOut("  -> bo qua\n");
			continue;
		}
		if (!H.pStub) H.pStub = BuildStub(i, H);
		if (!H.pStub || !PatchHook(H, TRUE))
		{
			DbgOut("  %-8s 0x%08X LOI patch\n", H.szLabel, (DWORD)H.pFunc);
			continue;
		}
		DbgOut("  %-8s 0x%08X OK\n", H.szLabel, (DWORD)H.pFunc);
		nOK++;
	}
	return nOK;
}

// Go hook, giu hook CoreDataChanged / SystemMessageArrival neu tab Nhiem vu con dung (bKeepQuest)
static void UnhookAll(BOOL bKeepQuest)
{
	for (int i = 0; i < FLOG_HOOKS; i++)
	{
		FLOG_HOOK &H = s_Hooks[i];
		if (!H.bPatched) continue;
		if (bKeepQuest && s_bQuestWatch && H.nKind != flk_string) continue;
		PatchHook(H, FALSE);
	}
}

// Truoc khi FreeLibrary: go het
void DebugFileUnhook()
{
	s_bLogOn = FALSE;
	s_bQuestWatch = FALSE;
	UnhookAll(FALSE);
}

// Tab Nhiem vu: bat/tat chuyen CoreDataChanged sang DebugQuestCoreData (dung chung hook voi nut Ghi file/lua)
BOOL DebugFileCoreDataHook(BOOL bOn)
{
	if (bOn)
	{
		if (!InstallHooks(TRUE)) return FALSE;
		s_bQuestWatch = TRUE;
		return TRUE;
	}
	s_bQuestWatch = FALSE;
	if (!s_bLogOn)
	{
		for (int i = 0; i < FLOG_HOOKS; i++)
			if (s_Hooks[i].nKind != flk_string && s_Hooks[i].bPatched) PatchHook(s_Hooks[i], FALSE);
	}
	return TRUE;
}

// Ghi toan bo log ra Logs\FileLog.txt (cua so DEBUG chi hien duoc 4096 ky tu)
static BOOL SaveLogFile(char *szFile)
{
	sprintf(szFile, "%s\\Logs\\FileLog.txt", theMain.m_szCurFolder);
	FILE *pFile = fopen(szFile, "wt");
	if (!pFile) return FALSE;
	EnterCriticalSection(&s_csLog);
	int nNum = (s_nLogTotal < FLOG_MAX) ? s_nLogTotal : FLOG_MAX;
	int nFirst = (s_nLogTotal < FLOG_MAX) ? 0 : s_nLogNext;
	fprintf(pFile, "# %d dong (tong da ghi %d). Cot: ms tu luc bat | ham | dia chi goi | so lan | chuoi\n", nNum, s_nLogTotal);
	for (int k = 0; k < nNum; k++)
	{
		FLOG_LINE &L = s_Log[(nFirst + k) % FLOG_MAX];
		fprintf(pFile, "%7u\t%-8s\t%08X\t%d\t%s\n", L.dwTick, HookLabel(L.nHook), L.dwRet, L.nCount, L.szText);
	}
	LeaveCriticalSection(&s_csLog);
	fclose(pFile);
	return TRUE;
}

// In cac dong moi nhat vua cua so (szFilter != "" => chi dong chua chuoi do)
static void OutLog(const char *szFilter)
{
	EnterCriticalSection(&s_csLog);
	int nNum = (s_nLogTotal < FLOG_MAX) ? s_nLogTotal : FLOG_MAX;
	// Tim tu cuoi len, du cho cua so (~3000 ky tu)
	int nShow = 0, nChars = 0, nStart = nNum;
	for (int k = nNum - 1; k >= 0; k--)
	{
		FLOG_LINE &L = s_Log[(s_nLogNext - nNum + k + FLOG_MAX) % FLOG_MAX];
		if (szFilter[0] && !strstr(L.szText, szFilter)) continue;
		int nLen = 32 + (int)strlen(L.szText);
		if (nChars + nLen > 3000) break;
		nChars += nLen;
		nShow++;
		nStart = k;
	}
	for (int k = nStart; k < nNum; k++)
	{
		FLOG_LINE &L = s_Log[(s_nLogNext - nNum + k + FLOG_MAX) % FLOG_MAX];
		if (szFilter[0] && !strstr(L.szText, szFilter)) continue;
		DbgOut("%6u %-8s %08X", L.dwTick, HookLabel(L.nHook), L.dwRet);
		if (L.nCount > 1) DbgOut(" x%d", L.nCount);
		DbgOut(" %s\n", L.szText);
	}
	LeaveCriticalSection(&s_csLog);
	if (!nShow) DbgOut("(chua co dong nao%s)\n", szFilter[0] ? " khop chuoi loc" : "");
}

// dbg_mp_filelog: bat/tat. X = 1 thi ghi ca file hinh/am thanh
void DebugFileLogToggle(int nX)
{
	if (!s_bCsInit)
	{
		InitializeCriticalSection(&s_csLog);
		s_bCsInit = TRUE;
	}
	if (!s_bLogOn)
	{
		EnterCriticalSection(&s_csLog);
		s_nLogNext = s_nLogTotal = 0;
		LeaveCriticalSection(&s_csLog);
		s_bSkipMedia = (nX != 1);
		s_bTaskSnap = FALSE;
		if (TaskReady())
		{
			DbgOut("Task Da Tau hien tai:");
			for (int t = 1020; t <= 1044; t++)
			{
				int v = g_pCurPlayer->m_cTask.GetTaskVal(t);
				if (v) DbgOut(" [%d]=%d", t, v);
			}
			DbgOut("\n");
		}
		s_dwLogStart = GetTickCount();
		DbgOut("Chen hook engine.dll:\n");
		int nOK = InstallHooks();
		if (!nOK)
		{
			DbgOut("LOI: khong chen duoc hook nao\n");
			return;
		}
		s_bLogOn = TRUE;
		DbgOut("=> BAT ghi (%d/%d ham)%s. Bay gio bam vao NPC / chon dong menu, roi bam nut nay lan nua de TAT,\n"
			"hoac bam 'Xem log' de xem ngay. Log day du: Logs\\FileLog.txt\n",
			nOK, (int)FLOG_HOOKS, s_bSkipMedia ? ", bo qua file hinh/am thanh (X = 1 de ghi ca)" : ", GHI CA file hinh/am thanh");
	}
	else
	{
		s_bLogOn = FALSE;
		UnhookAll(TRUE);
		char szFile[MAX_PATH];
		DbgOut("=> TAT ghi, da go hook%s. %d dong\n", s_bQuestWatch ? " (giu CoreDataChanged cho tab Nhiem vu)" : "", s_nLogTotal);
		if (SaveLogFile(szFile)) DbgOut("Da luu: %s\n", szFile);
		else DbgOut("LOI: khong ghi duoc %s (thu muc Logs co ton tai?)\n", szFile);
		OutLog("");
	}
	g_DbgReply.nResult[0] = s_bLogOn;
	g_DbgReply.bOK = TRUE;
}

// dbg_mp_filelogview: xem log (o Chuoi = loc), luu file. X = 1 thi xoa log sau khi xem
void DebugFileLogView(int nX, const char *szFilter)
{
	if (!s_bCsInit)
	{
		DbgOut("Chua bat ghi lan nao (bam 'Ghi file/lua game nap')\n");
		return;
	}
	DbgOut("Ghi: %s | %d dong%s\n", s_bLogOn ? "DANG BAT" : "tat", s_nLogTotal,
		(s_nLogTotal > FLOG_MAX) ? " (da ghi de dong cu)" : "");
	char szFile[MAX_PATH];
	if (SaveLogFile(szFile)) DbgOut("Day du: %s\n", szFile);
	OutLog(szFilter ? szFilter : "");
	if (nX == 1)
	{
		EnterCriticalSection(&s_csLog);
		s_nLogNext = s_nLogTotal = 0;
		s_dwLogStart = GetTickCount();
		LeaveCriticalSection(&s_csLog);
		DbgOut("=> Da xoa log\n");
	}
	g_DbgReply.bOK = TRUE;
}
