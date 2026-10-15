// CrashHandler.cpp - xem CrashHandler.h
// Chi dung Win32 (khong MFC, khong precompiled header) de bien dich duoc trong ca VLAuto lan VLHook.

#include <windows.h>
#include <tlhelp32.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <dbghelp.h>
#include <psapi.h>
#include "CrashHandler.h"

#pragma warning(disable: 4996)	// GetVersionExA, _snprintf

#define MAX_FIRST_CHANCE	20			// So lan ghi first-chance toi da moi lan chay
#define CRASH_CODE_CRT		0xE0435254	// 'CRT' - loi CRT doi thanh exception
#define STACK_SCAN_BYTES	0x4000
#define STACK_SCAN_MAX		48

const char* volatile g_pszCrashStage = NULL;

static char							s_szTag[32];
static WCHAR						s_szFolder[MAX_PATH];		// ...\Crash
static WCHAR						s_szBaseName[MAX_PATH];		// ...\Crash\<Tag>_<ngay_gio>_<pid>
static HMODULE						s_hSelf = NULL;
static BOOL							s_bInstalled = FALSE;
static LPTOP_LEVEL_EXCEPTION_FILTER	s_pPrevFilter = NULL;
static PVOID						s_pVectored = NULL;
static volatile LONG				s_nFirstChance = 0;
static volatile LONG				s_nInHandler = 0;
static DWORD						s_dwFCAddr[MAX_FIRST_CHANCE];

// ---------------------------------------------------------------------------
// dbghelp / psapi nap dong (Win7 co san dbghelp 6.1)

typedef BOOL (WINAPI *PFN_MINIDUMP)(HANDLE, DWORD, HANDLE, MINIDUMP_TYPE, PMINIDUMP_EXCEPTION_INFORMATION,
	PMINIDUMP_USER_STREAM_INFORMATION, PMINIDUMP_CALLBACK_INFORMATION);
typedef BOOL (WINAPI *PFN_SYMINIT)(HANDLE, PCSTR, BOOL);
typedef DWORD (WINAPI *PFN_SYMSETOPT)(DWORD);
typedef BOOL (WINAPI *PFN_STACKWALK)(DWORD, HANDLE, HANDLE, LPSTACKFRAME64, PVOID, PREAD_PROCESS_MEMORY_ROUTINE64,
	PFUNCTION_TABLE_ACCESS_ROUTINE64, PGET_MODULE_BASE_ROUTINE64, PTRANSLATE_ADDRESS_ROUTINE64);
typedef BOOL (WINAPI *PFN_SYMFROMADDR)(HANDLE, DWORD64, PDWORD64, PSYMBOL_INFO);
typedef BOOL (WINAPI *PFN_SYMLINE)(HANDLE, DWORD64, PDWORD, PIMAGEHLP_LINE64);
typedef BOOL (WINAPI *PFN_GETPMI)(HANDLE, PPROCESS_MEMORY_COUNTERS, DWORD);

static struct
{
	BOOL							bLoaded;
	PFN_MINIDUMP					MiniDumpWriteDump;
	PFN_SYMINIT						SymInitialize;
	PFN_SYMSETOPT					SymSetOptions;
	PFN_STACKWALK					StackWalk64;
	PFN_SYMFROMADDR					SymFromAddr;
	PFN_SYMLINE						SymGetLineFromAddr64;
	PFUNCTION_TABLE_ACCESS_ROUTINE64	SymFunctionTableAccess64;
	PGET_MODULE_BASE_ROUTINE64		SymGetModuleBase64;
	PFN_GETPMI						GetProcessMemoryInfo;
	BOOL							bSymInit;
} s_Dbg;

static void LoadDbgHelp()
{
	if (s_Dbg.bLoaded) return;
	s_Dbg.bLoaded = TRUE;
	HMODULE h = LoadLibraryA("dbghelp.dll");
	if (h)
	{
		s_Dbg.MiniDumpWriteDump = (PFN_MINIDUMP)GetProcAddress(h, "MiniDumpWriteDump");
		s_Dbg.SymInitialize = (PFN_SYMINIT)GetProcAddress(h, "SymInitialize");
		s_Dbg.SymSetOptions = (PFN_SYMSETOPT)GetProcAddress(h, "SymSetOptions");
		s_Dbg.StackWalk64 = (PFN_STACKWALK)GetProcAddress(h, "StackWalk64");
		s_Dbg.SymFromAddr = (PFN_SYMFROMADDR)GetProcAddress(h, "SymFromAddr");
		s_Dbg.SymGetLineFromAddr64 = (PFN_SYMLINE)GetProcAddress(h, "SymGetLineFromAddr64");
		s_Dbg.SymFunctionTableAccess64 = (PFUNCTION_TABLE_ACCESS_ROUTINE64)GetProcAddress(h, "SymFunctionTableAccess64");
		s_Dbg.SymGetModuleBase64 = (PGET_MODULE_BASE_ROUTINE64)GetProcAddress(h, "SymGetModuleBase64");
	}
	h = LoadLibraryA("psapi.dll");
	if (h)
		s_Dbg.GetProcessMemoryInfo = (PFN_GETPMI)GetProcAddress(h, "GetProcessMemoryInfo");
}

static void InitSymbols()
{
	if (s_Dbg.bSymInit || !s_Dbg.SymInitialize) return;
	if (s_Dbg.SymSetOptions)
		s_Dbg.SymSetOptions(SYMOPT_UNDNAME | SYMOPT_DEFERRED_LOADS | SYMOPT_LOAD_LINES | SYMOPT_FAIL_CRITICAL_ERRORS);
	// Tim pdb canh exe/dll (thu muc Auto) - fInvadeProcess nap danh sach module
	char szPath[MAX_PATH * 2] = "";
	WideCharToMultiByte(CP_ACP, 0, s_szFolder, -1, szPath, MAX_PATH, NULL, NULL);
	char *p = strrchr(szPath, '\\');	// bo "\Crash"
	if (p) *p = 0;
	s_Dbg.bSymInit = s_Dbg.SymInitialize(GetCurrentProcess(), szPath, TRUE);
}

// ---------------------------------------------------------------------------
// Ghi log

struct CLog
{
	HANDLE	hFile;
	CLog() : hFile(INVALID_HANDLE_VALUE) {}
	~CLog() { if (hFile != INVALID_HANDLE_VALUE) CloseHandle(hFile); }

	BOOL Open(const WCHAR *szExt)
	{
		WCHAR szFile[MAX_PATH];
		_snwprintf(szFile, MAX_PATH - 1, L"%s%s", s_szBaseName, szExt);
		szFile[MAX_PATH - 1] = 0;
		CreateDirectoryW(s_szFolder, NULL);
		hFile = CreateFileW(szFile, FILE_APPEND_DATA, FILE_SHARE_READ, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
		return hFile != INVALID_HANDLE_VALUE;
	}

	void Out(const char *szFormat, ...)
	{
		if (hFile == INVALID_HANDLE_VALUE) return;
		char szBuf[1024];
		va_list vl;
		va_start(vl, szFormat);
		int n = _vsnprintf(szBuf, sizeof(szBuf) - 1, szFormat, vl);
		va_end(vl);
		if (n < 0 || n > (int)sizeof(szBuf) - 1) n = sizeof(szBuf) - 1;
		DWORD dw;
		WriteFile(hFile, szBuf, n, &dw, NULL);
	}
};

static const char* ExceptionName(DWORD dwCode)
{
	switch (dwCode)
	{
	case EXCEPTION_ACCESS_VIOLATION:		return "ACCESS_VIOLATION (truy cap sai dia chi)";
	case EXCEPTION_STACK_OVERFLOW:			return "STACK_OVERFLOW (tran stack - de quy?)";
	case EXCEPTION_INT_DIVIDE_BY_ZERO:		return "INT_DIVIDE_BY_ZERO (chia cho 0)";
	case EXCEPTION_INT_OVERFLOW:			return "INT_OVERFLOW";
	case EXCEPTION_ILLEGAL_INSTRUCTION:		return "ILLEGAL_INSTRUCTION";
	case EXCEPTION_PRIV_INSTRUCTION:		return "PRIV_INSTRUCTION";
	case EXCEPTION_IN_PAGE_ERROR:			return "IN_PAGE_ERROR";
	case EXCEPTION_ARRAY_BOUNDS_EXCEEDED:	return "ARRAY_BOUNDS_EXCEEDED";
	case EXCEPTION_FLT_DIVIDE_BY_ZERO:		return "FLT_DIVIDE_BY_ZERO";
	case EXCEPTION_FLT_INVALID_OPERATION:	return "FLT_INVALID_OPERATION";
	case EXCEPTION_BREAKPOINT:				return "BREAKPOINT";
	case EXCEPTION_DATATYPE_MISALIGNMENT:	return "DATATYPE_MISALIGNMENT";
	case 0xC0000374:						return "HEAP_CORRUPTION (hong heap - ghi qua bo dem / free 2 lan)";
	case 0xC0000409:						return "STACK_BUFFER_OVERRUN (ghi qua bo dem tren stack)";
	case 0xE06D7363:						return "C++ exception (throw khong ai catch)";
	case CRASH_CODE_CRT:					return "Loi CRT (xem dong CRT)";
	}
	return "?";
}

// Module chua dia chi: ten file + offset
static BOOL AddrToModule(DWORD dwAddr, char *szName, int nMax, DWORD *pdwOffset, HMODULE *phMod = NULL)
{
	MEMORY_BASIC_INFORMATION mbi;
	szName[0] = 0;
	*pdwOffset = dwAddr;
	if (!dwAddr || !VirtualQuery((LPCVOID)dwAddr, &mbi, sizeof(mbi)) || !mbi.AllocationBase || mbi.State != MEM_COMMIT)
		return FALSE;
	char szPath[MAX_PATH];
	if (!GetModuleFileNameA((HMODULE)mbi.AllocationBase, szPath, MAX_PATH))
		return FALSE;
	const char *p = strrchr(szPath, '\\');
	strncpy(szName, p ? p + 1 : szPath, nMax - 1);
	szName[nMax - 1] = 0;
	*pdwOffset = dwAddr - (DWORD)mbi.AllocationBase;
	if (phMod) *phMod = (HMODULE)mbi.AllocationBase;
	return TRUE;
}

static BOOL IsCodeAddr(DWORD dwAddr)
{
	MEMORY_BASIC_INFORMATION mbi;
	if (dwAddr < 0x10000 || !VirtualQuery((LPCVOID)dwAddr, &mbi, sizeof(mbi)) || mbi.State != MEM_COMMIT || mbi.Type != MEM_IMAGE)
		return FALSE;
	return (mbi.Protect & (PAGE_EXECUTE | PAGE_EXECUTE_READ | PAGE_EXECUTE_READWRITE | PAGE_EXECUTE_WRITECOPY)) != 0;
}

static BOOL IsInSelf(DWORD dwAddr)
{
	MEMORY_BASIC_INFORMATION mbi;
	return dwAddr && VirtualQuery((LPCVOID)dwAddr, &mbi, sizeof(mbi)) && (HMODULE)mbi.AllocationBase == s_hSelf;
}

static void OutAddr(CLog &log, DWORD dwAddr)
{
	char szMod[64];
	DWORD dwOff;
	if (AddrToModule(dwAddr, szMod, sizeof(szMod), &dwOff))
		log.Out("%08X  %s+0x%X", dwAddr, szMod, dwOff);
	else
		log.Out("%08X  ?", dwAddr);

	if (!s_Dbg.bSymInit || !s_Dbg.SymFromAddr) return;
	BYTE buf[sizeof(SYMBOL_INFO) + 256];
	PSYMBOL_INFO pSym = (PSYMBOL_INFO)buf;
	memset(buf, 0, sizeof(buf));
	pSym->SizeOfStruct = sizeof(SYMBOL_INFO);
	pSym->MaxNameLen = 255;
	DWORD64 dwDisp = 0;
	if (s_Dbg.SymFromAddr(GetCurrentProcess(), dwAddr, &dwDisp, pSym))
	{
		log.Out("  %s+0x%X", pSym->Name, (DWORD)dwDisp);
		IMAGEHLP_LINE64 line;
		memset(&line, 0, sizeof(line));
		line.SizeOfStruct = sizeof(line);
		DWORD dwLineDisp = 0;
		if (s_Dbg.SymGetLineFromAddr64 && s_Dbg.SymGetLineFromAddr64(GetCurrentProcess(), dwAddr, &dwLineDisp, &line))
		{
			const char *p = strrchr(line.FileName, '\\');
			log.Out("  (%s:%d)", p ? p + 1 : line.FileName, line.LineNumber);
		}
	}
}

static void OutStackWalk(CLog &log, const CONTEXT *pCtx, HANDLE hThread)
{
	log.Out("\nCall stack (StackWalk64):\n");
	if (!s_Dbg.StackWalk64 || !s_Dbg.SymFunctionTableAccess64 || !s_Dbg.SymGetModuleBase64)
	{
		log.Out("  (khong co dbghelp)\n");
		return;
	}
	CONTEXT ctx = *pCtx;
	STACKFRAME64 sf;
	memset(&sf, 0, sizeof(sf));
	sf.AddrPC.Offset = ctx.Eip;		sf.AddrPC.Mode = AddrModeFlat;
	sf.AddrFrame.Offset = ctx.Ebp;	sf.AddrFrame.Mode = AddrModeFlat;
	sf.AddrStack.Offset = ctx.Esp;	sf.AddrStack.Mode = AddrModeFlat;
	for (int i = 0; i < 40; i++)
	{
		if (!s_Dbg.StackWalk64(IMAGE_FILE_MACHINE_I386, GetCurrentProcess(), hThread, &sf, &ctx, NULL,
			s_Dbg.SymFunctionTableAccess64, s_Dbg.SymGetModuleBase64, NULL))
			break;
		if (!sf.AddrPC.Offset) break;
		log.Out("  [%2d] ", i);
		OutAddr(log, (DWORD)sf.AddrPC.Offset);
		log.Out("\n");
	}
}

// Quet stack tim dia chi tro ve vao code (khi StackWalk dut do code game khong co frame pointer)
static void OutStackScan(CLog &log, DWORD dwEsp)
{
	log.Out("\nDia chi code tren stack (quet tho tu ESP, co the co gia tri cu):\n");
	MEMORY_BASIC_INFORMATION mbi;
	if (!VirtualQuery((LPCVOID)dwEsp, &mbi, sizeof(mbi))) return;
	DWORD dwEnd = (DWORD)mbi.BaseAddress + mbi.RegionSize;
	if (dwEnd > dwEsp + STACK_SCAN_BYTES) dwEnd = dwEsp + STACK_SCAN_BYTES;
	int nCount = 0;
	for (DWORD p = dwEsp & ~3; p + 4 <= dwEnd && nCount < STACK_SCAN_MAX; p += 4)
	{
		DWORD dwVal = *(DWORD*)p;
		if (!IsCodeAddr(dwVal)) continue;
		log.Out("  ESP+%04X: ", p - dwEsp);
		OutAddr(log, dwVal);
		log.Out("\n");
		nCount++;
	}
}

static void OutModules(CLog &log)
{
	log.Out("\nModule:\n");
	HANDLE hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, GetCurrentProcessId());
	if (hSnap == INVALID_HANDLE_VALUE) return;
	MODULEENTRY32 me;
	me.dwSize = sizeof(me);
	for (BOOL b = Module32First(hSnap, &me); b; b = Module32Next(hSnap, &me))
	{
		char szPath[MAX_PATH];
#ifdef UNICODE
		WideCharToMultiByte(CP_ACP, 0, me.szExePath, -1, szPath, MAX_PATH, NULL, NULL);
#else
		strncpy(szPath, me.szExePath, MAX_PATH);
		szPath[MAX_PATH - 1] = 0;
#endif
		log.Out("  %08X - %08X  %s\n", (DWORD)me.modBaseAddr, (DWORD)me.modBaseAddr + me.modBaseSize, szPath);
	}
	CloseHandle(hSnap);
}

static void OutHeader(CLog &log, const char *szKind, EXCEPTION_POINTERS *pEx)
{
	SYSTEMTIME st;
	GetLocalTime(&st);
	char szExe[MAX_PATH];
	GetModuleFileNameA(NULL, szExe, MAX_PATH);
	OSVERSIONINFOEXA osv;
	memset(&osv, 0, sizeof(osv));
	osv.dwOSVersionInfoSize = sizeof(osv);
	typedef LONG (WINAPI *PFN_RTLGETVERSION)(OSVERSIONINFOEXW*);
	PFN_RTLGETVERSION pfnVer = (PFN_RTLGETVERSION)GetProcAddress(GetModuleHandleA("ntdll.dll"), "RtlGetVersion");
	OSVERSIONINFOEXW osvw;
	memset(&osvw, 0, sizeof(osvw));
	osvw.dwOSVersionInfoSize = sizeof(osvw);
	if (pfnVer && pfnVer(&osvw) == 0)	// GetVersionEx bao sai tu Win8 neu exe khong khai bao trong manifest
	{
		osv.dwMajorVersion = osvw.dwMajorVersion;
		osv.dwMinorVersion = osvw.dwMinorVersion;
		osv.dwBuildNumber = osvw.dwBuildNumber;
		osv.wServicePackMajor = osvw.wServicePackMajor;
	}
	else
		GetVersionExA((OSVERSIONINFOA*)&osv);
	BOOL bWow64 = FALSE;
	typedef BOOL (WINAPI *PFN_ISWOW64)(HANDLE, PBOOL);
	PFN_ISWOW64 pfnWow = (PFN_ISWOW64)GetProcAddress(GetModuleHandleA("kernel32.dll"), "IsWow64Process");
	if (pfnWow) pfnWow(GetCurrentProcess(), &bWow64);

	log.Out("\n==================== %s - %s ====================\n", s_szTag, szKind);
	log.Out("Thoi gian: %04d-%02d-%02d %02d:%02d:%02d.%03d   PID %u  TID %u\n", st.wYear, st.wMonth, st.wDay,
		st.wHour, st.wMinute, st.wSecond, st.wMilliseconds, GetCurrentProcessId(), GetCurrentThreadId());
	log.Out("Process: %s\n", szExe);
	log.Out("Windows %u.%u build %u SP%u %s\n", osv.dwMajorVersion, osv.dwMinorVersion, osv.dwBuildNumber,
		osv.wServicePackMajor, bWow64 ? "(64-bit, WOW64)" : "(32-bit)");
	log.Out("Dang lam (CRASH_STAGE): %s\n", g_pszCrashStage ? g_pszCrashStage : "-");

	const EXCEPTION_RECORD *pRec = pEx->ExceptionRecord;
	log.Out("Ma loi: 0x%08X  %s\n", pRec->ExceptionCode, ExceptionName(pRec->ExceptionCode));
	log.Out("Tai:    ");
	OutAddr(log, (DWORD)pRec->ExceptionAddress);
	log.Out("\n");
	if ((pRec->ExceptionCode == EXCEPTION_ACCESS_VIOLATION || pRec->ExceptionCode == EXCEPTION_IN_PAGE_ERROR) && pRec->NumberParameters >= 2)
	{
		ULONG_PTR nType = pRec->ExceptionInformation[0];
		log.Out("        %s dia chi 0x%08X\n", nType == 0 ? "DOC" : (nType == 1 ? "GHI" : "THUC THI (DEP)"),
			(DWORD)pRec->ExceptionInformation[1]);
	}
	if (pRec->ExceptionCode == CRASH_CODE_CRT && pRec->NumberParameters >= 1 && pRec->ExceptionInformation[0])
		log.Out("CRT:    %s\n", (const char*)pRec->ExceptionInformation[0]);

	const CONTEXT *c = pEx->ContextRecord;
	log.Out("EAX %08X  EBX %08X  ECX %08X  EDX %08X  ESI %08X  EDI %08X\n", c->Eax, c->Ebx, c->Ecx, c->Edx, c->Esi, c->Edi);
	log.Out("EIP %08X  ESP %08X  EBP %08X  EFL %08X\n", c->Eip, c->Esp, c->Ebp, c->EFlags);
}

static void OutMemory(CLog &log)
{
	PROCESS_MEMORY_COUNTERS pmc;
	memset(&pmc, 0, sizeof(pmc));
	pmc.cb = sizeof(pmc);
	if (s_Dbg.GetProcessMemoryInfo && s_Dbg.GetProcessMemoryInfo(GetCurrentProcess(), &pmc, sizeof(pmc)))
		log.Out("RAM: Working set %u MB (dinh %u MB)  Commit %u MB (dinh %u MB)\n", (UINT)(pmc.WorkingSetSize >> 20),
			(UINT)(pmc.PeakWorkingSetSize >> 20), (UINT)(pmc.PagefileUsage >> 20), (UINT)(pmc.PeakPagefileUsage >> 20));
	MEMORYSTATUSEX ms;
	ms.dwLength = sizeof(ms);
	if (GlobalMemoryStatusEx(&ms))
		log.Out("May: RAM trong %u / %u MB  Virtual trong %u MB\n", (UINT)(ms.ullAvailPhys >> 20),
			(UINT)(ms.ullTotalPhys >> 20), (UINT)(ms.ullAvailVirtual >> 20));
}

// ---------------------------------------------------------------------------
// Unhandled: ghi tren thread rieng (stack overflow khong con cho de ghi)

struct CRASH_JOB
{
	EXCEPTION_POINTERS	*pEx;
	DWORD				dwThreadId;
	HANDLE				hThread;
};

static void WriteDump(CRASH_JOB *pJob, CLog &log)
{
	if (!s_Dbg.MiniDumpWriteDump)
	{
		log.Out("\nKhong ghi duoc dump: khong co dbghelp MiniDumpWriteDump\n");
		return;
	}
	WCHAR szFile[MAX_PATH];
	_snwprintf(szFile, MAX_PATH - 1, L"%s.dmp", s_szBaseName);
	szFile[MAX_PATH - 1] = 0;
	HANDLE hFile = CreateFileW(szFile, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hFile == INVALID_HANDLE_VALUE)
	{
		log.Out("\nKhong tao duoc file dump (GetLastError %u)\n", GetLastError());
		return;
	}
	MINIDUMP_EXCEPTION_INFORMATION mei;
	mei.ThreadId = pJob->dwThreadId;
	mei.ExceptionPointers = pJob->pEx;
	mei.ClientPointers = FALSE;
	BOOL bOK = s_Dbg.MiniDumpWriteDump(GetCurrentProcess(), GetCurrentProcessId(), hFile,
		(MINIDUMP_TYPE)(MiniDumpNormal | MiniDumpWithIndirectlyReferencedMemory), &mei, NULL, NULL);
	DWORD dwErr = GetLastError();
	CloseHandle(hFile);
	char szName[MAX_PATH];
	WideCharToMultiByte(CP_ACP, 0, wcsrchr(szFile, L'\\') + 1, -1, szName, MAX_PATH, NULL, NULL);
	if (bOK)
		log.Out("\nDump: Crash\\%s (mo bang Visual Studio cung pdb cua dung ban build)\n", szName);
	else
		log.Out("\nMiniDumpWriteDump loi %u\n", dwErr);
}

static DWORD WINAPI CrashWorker(LPVOID pParam)
{
	CRASH_JOB *pJob = (CRASH_JOB*)pParam;
	LoadDbgHelp();
	InitSymbols();
	CLog log;
	if (!log.Open(L".log")) return 0;
	OutHeader(log, "CRASH", pJob->pEx);
	OutMemory(log);
	OutStackWalk(log, pJob->pEx->ContextRecord, pJob->hThread);
	OutStackScan(log, pJob->pEx->ContextRecord->Esp);
	OutModules(log);
	WriteDump(pJob, log);
	return 0;
}

static void HandleCrash(EXCEPTION_POINTERS *pEx)
{
	CRASH_JOB job;
	job.pEx = pEx;
	job.dwThreadId = GetCurrentThreadId();
	job.hThread = NULL;
	DuplicateHandle(GetCurrentProcess(), GetCurrentThread(), GetCurrentProcess(), &job.hThread, 0, FALSE, DUPLICATE_SAME_ACCESS);
	HANDLE hWorker = CreateThread(NULL, 256 * 1024, CrashWorker, &job, 0, NULL);
	if (hWorker)
	{
		WaitForSingleObject(hWorker, 60000);
		CloseHandle(hWorker);
	}
	else
		CrashWorker(&job);	// Khong tao duoc thread: ghi tren thread hien tai
	if (job.hThread) CloseHandle(job.hThread);
}

static LONG WINAPI CrashFilter(EXCEPTION_POINTERS *pEx)
{
	if (InterlockedExchange(&s_nInHandler, 1) == 0)
		HandleCrash(pEx);
	if (s_pPrevFilter)
		return s_pPrevFilter(pEx);
	return EXCEPTION_CONTINUE_SEARCH;	// Hop thoai loi cua Windows / WER nhu cu
}

// ---------------------------------------------------------------------------
// First-chance: chi ghi log (khong dump), toi da MAX_FIRST_CHANCE lan, moi dia chi 1 lan

static BOOL IsSerious(DWORD dwCode)
{
	switch (dwCode)
	{
	case EXCEPTION_ACCESS_VIOLATION:
	case EXCEPTION_STACK_OVERFLOW:
	case EXCEPTION_INT_DIVIDE_BY_ZERO:
	case EXCEPTION_ILLEGAL_INSTRUCTION:
	case EXCEPTION_PRIV_INSTRUCTION:
	case EXCEPTION_IN_PAGE_ERROR:
	case 0xC0000374:
		return TRUE;
	}
	return FALSE;
}

// Module nay co tren stack khong (loi xay ra trong ham game do Auto goi)
static BOOL SelfOnStack(DWORD dwEsp)
{
	MEMORY_BASIC_INFORMATION mbi;
	if (!VirtualQuery((LPCVOID)dwEsp, &mbi, sizeof(mbi))) return FALSE;
	DWORD dwEnd = (DWORD)mbi.BaseAddress + mbi.RegionSize;
	if (dwEnd > dwEsp + 0x1000) dwEnd = dwEsp + 0x1000;
	for (DWORD p = dwEsp & ~3; p + 4 <= dwEnd; p += 4)
		if (IsInSelf(*(DWORD*)p) && IsCodeAddr(*(DWORD*)p))
			return TRUE;
	return FALSE;
}

static LONG WINAPI FirstChanceHandler(EXCEPTION_POINTERS *pEx)
{
	DWORD dwCode = pEx->ExceptionRecord->ExceptionCode;
	if (!IsSerious(dwCode) || s_nInHandler || s_nFirstChance >= MAX_FIRST_CHANCE)
		return EXCEPTION_CONTINUE_SEARCH;
	if (dwCode == EXCEPTION_STACK_OVERFLOW)
		return EXCEPTION_CONTINUE_SEARCH;	// Khong du stack de ghi - de CrashFilter lo
	DWORD dwAddr = (DWORD)pEx->ExceptionRecord->ExceptionAddress;
	if (!IsInSelf(dwAddr) && !SelfOnStack(pEx->ContextRecord->Esp))
		return EXCEPTION_CONTINUE_SEARCH;
	LONG nIdx = InterlockedIncrement(&s_nFirstChance) - 1;
	if (nIdx >= MAX_FIRST_CHANCE) return EXCEPTION_CONTINUE_SEARCH;
	for (LONG i = 0; i < nIdx; i++)
		if (s_dwFCAddr[i] == dwAddr) return EXCEPTION_CONTINUE_SEARCH;
	s_dwFCAddr[nIdx] = dwAddr;

	LoadDbgHelp();
	CLog log;
	if (!log.Open(L".log")) return EXCEPTION_CONTINUE_SEARCH;
	OutHeader(log, "FIRST-CHANCE (co the bi __try hoac Windows nuot, chuong trinh chay tiep)", pEx);
	OutStackScan(log, pEx->ContextRecord->Esp);
	if (nIdx + 1 == MAX_FIRST_CHANCE)
		log.Out("\n(Da ghi %d lan first-chance - khong ghi them)\n", MAX_FIRST_CHANCE);
	return EXCEPTION_CONTINUE_SEARCH;
}

// ---------------------------------------------------------------------------
// Loi CRT -> exception

static void RaiseCrt(const char *szWhat)
{
	ULONG_PTR arg = (ULONG_PTR)szWhat;
	RaiseException(CRASH_CODE_CRT, EXCEPTION_NONCONTINUABLE, 1, &arg);
}

static void __cdecl OnInvalidParameter(const wchar_t*, const wchar_t*, const wchar_t*, unsigned int, uintptr_t)
{
	RaiseCrt("invalid parameter (ham CRT nhan tham so sai: chuoi NULL, buffer qua nho...)");
}

static void __cdecl OnPureCall()
{
	RaiseCrt("pure virtual function call");
}

static void __cdecl OnAbort(int)
{
	RaiseCrt("abort() / SIGABRT");
}

// ---------------------------------------------------------------------------

static void SelfAnchor() {}

void CrashInstall(const char *szTag, BOOL bLogFirstChance)
{
	if (s_bInstalled) return;
	strncpy(s_szTag, szTag, sizeof(s_szTag) - 1);

	GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
		(LPCWSTR)&SelfAnchor, &s_hSelf);
	GetModuleFileNameW(s_hSelf, s_szFolder, MAX_PATH);
	WCHAR *p = wcsrchr(s_szFolder, L'\\');
	if (p) *p = 0;
	wcsncat(s_szFolder, L"\\Crash", MAX_PATH - wcslen(s_szFolder) - 1);

	SYSTEMTIME st;
	GetLocalTime(&st);
	WCHAR szTagW[32];
	MultiByteToWideChar(CP_ACP, 0, s_szTag, -1, szTagW, 32);
	_snwprintf(s_szBaseName, MAX_PATH - 1, L"%s\\%s_%04d%02d%02d_%02d%02d%02d_%u", s_szFolder, szTagW,
		st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond, GetCurrentProcessId());

	s_pPrevFilter = SetUnhandledExceptionFilter(CrashFilter);
	if (bLogFirstChance)
		s_pVectored = AddVectoredExceptionHandler(1, FirstChanceHandler);
	_set_invalid_parameter_handler(OnInvalidParameter);
	_set_purecall_handler(OnPureCall);
	signal(SIGABRT, OnAbort);
	_set_abort_behavior(0, _WRITE_ABORT_MSG | _CALL_REPORTFAULT);
	s_bInstalled = TRUE;
}

void CrashUninstall()
{
	if (!s_bInstalled) return;
	if (s_pVectored)
	{
		RemoveVectoredExceptionHandler(s_pVectored);
		s_pVectored = NULL;
	}
	// Chi tra lai neu filter hien tai van la cua minh (module khac co the da dat sau)
	LPTOP_LEVEL_EXCEPTION_FILTER pCur = SetUnhandledExceptionFilter(s_pPrevFilter);
	if (pCur != CrashFilter)
		SetUnhandledExceptionFilter(pCur);
	_set_invalid_parameter_handler(NULL);
	_set_purecall_handler(NULL);
	signal(SIGABRT, SIG_DFL);
	s_bInstalled = FALSE;
}
