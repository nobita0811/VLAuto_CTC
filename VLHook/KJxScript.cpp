#include "stdafx.h"
#include "../GameDef.h"
#include "GameBase.h"
#include "JxCore.h"
#include "KJxScript.h"

int				(__stdcall *_LoadBuffer)(const char*, int, char*);
int				(__stdcall *_ExecuteCode)();

struct ExportFunctions
{
	const char* FuncName;
	FARPROC* FuncAdd;
};

ExportFunctions KJxScriptFuncs[] = 
{
	{ "?LoadBuffer@KLuaScript@@QAEHPAEKPBD@Z", (FARPROC*)&_LoadBuffer },
	{ "?ExecuteCode@KLuaScript@@QAEHXZ", (FARPROC*)&_ExecuteCode },
};

// ======================================
// Class KJxScript Implementation
// ======================================
BOOL KJxScript::InitFunctions()
{
	HMODULE hEngine = LoadLibrary("engine");
	if (hEngine == NULL)
		return FALSE;

	int nProcNums = sizeof(KJxScriptFuncs) / sizeof(ExportFunctions);
	for (int i = 0; i < nProcNums; i++)
	{
		FARPROC FuncAdd = GetProcAddress(hEngine, KJxScriptFuncs[i].FuncName);
		if (FuncAdd == NULL)
			return FALSE;
		*(KJxScriptFuncs[i].FuncAdd) = FuncAdd;
	}

	return TRUE;
}

BOOL KJxScript::ExecuteScript(const char* szScript)
{
#ifndef VLTK_FREE_MODE
	KJxScript* pJxScript = (KJxScript*)GetPtrAddress(SCR_BASE_ADD);
	if (pJxScript && szScript && szScript[0] != 0)
	{
		if (pJxScript->LoadBuffer(szScript, strlen(szScript)))
			return pJxScript->ExecuteCode();
	}
#endif

	return FALSE;
}

int KJxScript::LoadBuffer(const char* szCode, int nLength, char* szParam)
{
	__asm
	{
		push szParam;
		push nLength;
		push szCode;
		call _LoadBuffer;
	}
}

int KJxScript::ExecuteCode()
{
	__asm call _ExecuteCode;
}
