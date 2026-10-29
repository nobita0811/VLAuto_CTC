// KDtrFix.cpp : sua crash do hook chong bot cua game cai vao tien trinh Auto
//
// Game (gamethienha_y.exe) chep 1 doan code (dau vung nho co chu ky "KDtr") vao cac tien trinh khac
// va hook GDI32!GetPixel, SendMessage, PostMessage, SendInput... Doan code nay dung 1 bang con tro API
// lay tu chinh tien trinh game. Game chay che do tuong thich 16BITCOLOR (Reduced color mode) nen
// 1 so con tro (vd WindowFromDC) tro vao stub shim cua apphelp.dll. Trong tien trinh Auto shim engine
// khong khoi tao => stub doc [NULL+0x354] => crash (vd khi ComCtl32 ve checkbox goi GetPixel).
//
// Cach sua: quet cac vung "KDtr", voi moi con tro tro vao stub apphelp dang
//     mov edi,edi / push ebp / mov ebp,esp / mov eax,[g] / ... / mov esi,[eax+X] / ... / call esi
// doc ham goc [[g]+X] tu tien trinh game (o do shim da khoi tao; DLL he thong cung dia chi giua cac
// tien trinh trong 1 lan khoi dong Windows) roi ghi de con tro trong bang.

#include "stdafx.h"
#include "GameCore.h"
#include "KDtrFix.h"

static HMODULE s_hAppHelp = NULL;
static DWORD s_dwAppHelpSize = 0;

static BOOL InAppHelp(DWORD dwAddr)
{
	return s_hAppHelp && dwAddr >= (DWORD)s_hAppHelp && dwAddr < (DWORD)s_hAppHelp + s_dwAppHelpSize;
}

static BOOL ReadLocal(DWORD dwAddr, void* pBuf, SIZE_T nSize)
{
	__try { memcpy(pBuf, (void*)dwAddr, nSize); return TRUE; }
	__except (EXCEPTION_EXECUTE_HANDLER) { return FALSE; }
}

// Stub apphelp: lay dia chi bien toan cuc g va offset X
static BOOL DecodeShimStub(DWORD dwStub, DWORD& g, DWORD& x)
{
	BYTE b[48];
	if (!ReadLocal(dwStub, b, sizeof(b)))
		return FALSE;
	if (b[0] != 0x8B || b[1] != 0xFF || b[2] != 0x55 || b[3] != 0x8B || b[4] != 0xEC)
		return FALSE;
	int i = 5;
	g = 0;
	for (; i < 24; i++)
		if (b[i] == 0xA1) { g = *(DWORD*)&b[i + 1]; i += 5; break; }	// mov eax, [g]
	if (!g || !InAppHelp(g))
		return FALSE;
	for (; i < (int)sizeof(b) - 6; i++)
		if (b[i] == 0x8B && b[i + 1] == 0xB0) { x = *(DWORD*)&b[i + 2]; return x < 0x10000; }	// mov esi, [eax+X]
	return FALSE;
}

static HANDLE OpenGameProcess()
{
	HWND hWnd = NULL;
	while ((hWnd = ::FindWindowEx(NULL, hWnd, VLTK_WINCLASS, NULL)) != NULL)
	{
		DWORD dwPid = 0;
		GetWindowThreadProcessId(hWnd, &dwPid);
		HANDLE hProc = dwPid ? OpenProcess(PROCESS_VM_READ, FALSE, dwPid) : NULL;
		if (hProc)
			return hProc;
	}
	return NULL;
}

// Ham goc ma stub apphelp [g]+X tro toi, doc tu tien trinh game. 0 neu khong hop le
static DWORD ResolveOrg(HANDLE hGame, DWORD g, DWORD x)
{
	DWORD dwTable = 0, dwOrg = 0;
	SIZE_T nRead;
	if (!ReadProcessMemory(hGame, (void*)g, &dwTable, 4, &nRead) || !dwTable
		|| !ReadProcessMemory(hGame, (void*)(dwTable + x), &dwOrg, 4, &nRead) || !dwOrg)
		return 0;
	HMODULE hMod = NULL;	// Ham goc phai nam trong 1 DLL da nap o tien trinh Auto (khong phai apphelp)
	if (InAppHelp(dwOrg) || !GetModuleHandleEx(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT, (LPCTSTR)dwOrg, &hMod))
		return 0;
	return dwOrg;
}

// Tra ve so con tro da sua
static int FixRegion(BYTE* pBase, SIZE_T nSize, HANDLE& hGame)
{
	int nFixed = 0;
	for (SIZE_T off = 0; off + 4 <= nSize; off += 4)
	{
		DWORD dwPtr;
		if (!ReadLocal((DWORD)(pBase + off), &dwPtr, 4) || !InAppHelp(dwPtr))
			continue;
		DWORD g, x;
		if (!DecodeShimStub(dwPtr, g, x))
			continue;
		if (!hGame && (hGame = OpenGameProcess()) == NULL)
			return nFixed;
		DWORD dwOrg = ResolveOrg(hGame, g, x);
		if (!dwOrg)
			continue;
		DWORD dwOld;
		if (VirtualProtect(pBase + off, 4, PAGE_EXECUTE_READWRITE, &dwOld))
		{
			InterlockedExchange((volatile LONG*)(pBase + off), (LONG)dwOrg);
			VirtualProtect(pBase + off, 4, dwOld, &dwOld);
			nFixed++;
		}
	}
	return nFixed;
}

static BOOL InitAppHelp()
{
	if (s_dwAppHelpSize)
		return TRUE;
	HMODULE hMod = GetModuleHandle(_T("apphelp.dll"));
	if (!hMod)
		return FALSE;
	IMAGE_DOS_HEADER* pDos = (IMAGE_DOS_HEADER*)hMod;
	IMAGE_NT_HEADERS* pNt = (IMAGE_NT_HEADERS*)((BYTE*)hMod + pDos->e_lfanew);
	s_hAppHelp = hMod;
	s_dwAppHelpSize = pNt->OptionalHeader.SizeOfImage;
	return TRUE;
}

int KDtrFixScan()
{
	if (!InitAppHelp())
		return 0;
	int nFixed = 0;
	HANDLE hGame = NULL;
	MEMORY_BASIC_INFORMATION mbi;
	BYTE* p = NULL;
	while (VirtualQuery(p, &mbi, sizeof(mbi)) == sizeof(mbi))
	{
		BYTE* pNext = (BYTE*)mbi.BaseAddress + mbi.RegionSize;
		if (mbi.State == MEM_COMMIT && mbi.Type == MEM_PRIVATE && mbi.BaseAddress == mbi.AllocationBase
			&& (mbi.Protect & (PAGE_EXECUTE_READWRITE | PAGE_EXECUTE_READ)) && !(mbi.Protect & PAGE_GUARD))
		{
			DWORD dwSig = 0;
			if (ReadLocal((DWORD)mbi.BaseAddress, &dwSig, 4) && dwSig == 'rtDK')	// "KDtr"
			{
				// Ca vung cap phat (co the gom nhieu region)
				SIZE_T nSize = 0;
				MEMORY_BASIC_INFORMATION m2;
				BYTE* q = (BYTE*)mbi.BaseAddress;
				while (VirtualQuery(q, &m2, sizeof(m2)) == sizeof(m2) && m2.AllocationBase == mbi.AllocationBase && m2.State == MEM_COMMIT)
				{
					nSize += m2.RegionSize;
					q += m2.RegionSize;
				}
				nFixed += FixRegion((BYTE*)mbi.BaseAddress, nSize, hGame);
				pNext = (BYTE*)mbi.BaseAddress + nSize;
			}
		}
		if (pNext <= p)
			break;
		p = pNext;
	}
	if (hGame)
		CloseHandle(hGame);
	return nFixed;
}

static DWORD WINAPI KDtrFixThread(LPVOID)
{
	for (;;)
	{
		KDtrFixScan();
		Sleep(200);
	}
	return 0;
}

// Luoi an toan neu hook bi goi truoc khi luong quet kip sua: AV dung tai lenh "mov esi,[eax+X]"
// cua stub apphelp (eax = NULL) -> nap ham goc vao esi, bo qua lenh, chay tiep.
static LONG CALLBACK KDtrFixVeh(EXCEPTION_POINTERS* pEx)
{
	if (pEx->ExceptionRecord->ExceptionCode != EXCEPTION_ACCESS_VIOLATION || !InitAppHelp())
		return EXCEPTION_CONTINUE_SEARCH;
	CONTEXT* c = pEx->ContextRecord;
	DWORD dwEip = c->Eip;
	BYTE b[6];
	if (c->Eax != 0 || !InAppHelp(dwEip) || !ReadLocal(dwEip, b, 6) || b[0] != 0x8B || b[1] != 0xB0)
		return EXCEPTION_CONTINUE_SEARCH;
	DWORD x = *(DWORD*)&b[2];
	DWORD g = 0, dwStart = 0;
	for (DWORD a = dwEip - 1; a + 32 > dwEip && !dwStart; a--)	// Tim dau stub (mov edi,edi/push ebp/mov ebp,esp)
	{
		BYTE h[5];
		if (ReadLocal(a, h, 5) && h[0] == 0x8B && h[1] == 0xFF && h[2] == 0x55 && h[3] == 0x8B && h[4] == 0xEC)
			dwStart = a;
	}
	DWORD gx, xx;
	if (!dwStart || !DecodeShimStub(dwStart, gx, xx) || xx != x)
		return EXCEPTION_CONTINUE_SEARCH;
	g = gx;
	HANDLE hGame = OpenGameProcess();
	DWORD dwOrg = hGame ? ResolveOrg(hGame, g, x) : 0;
	if (hGame)
		CloseHandle(hGame);
	if (!dwOrg)
		return EXCEPTION_CONTINUE_SEARCH;
	c->Esi = dwOrg;
	c->Eip = dwEip + 6;
	return EXCEPTION_CONTINUE_EXECUTION;
}

void KDtrFixStart()
{
	AddVectoredExceptionHandler(1, KDtrFixVeh);
	HANDLE hThread = CreateThread(NULL, 0, KDtrFixThread, NULL, 0, NULL);
	if (hThread)
		CloseHandle(hThread);
}
