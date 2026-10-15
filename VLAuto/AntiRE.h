#pragma once

#define JUNK_CODE_ONE		\
    __asm{push edx}			\
    __asm{xor edx, edx}		\
    __asm{setpo dl}			\
    __asm{push eax}			\
    __asm{xor eax, edx}		\
    __asm{sal eax, 2}		\
    __asm{xchg edx, eax}	\
    __asm{pop eax}			\
    __asm{or edx, ecx}		\
    __asm{pop edx}

// The Int2DCheck function will check to see if a debugger
// is attached to the current process. It does this by setting up
// SEH and using the Int 2D instruction which will only cause an
// exception if there is no debugger. Also when used in OllyDBG
// it will skip a byte in the disassembly and will create
// some havoc.
inline BOOL Int2DCheck()
{
	__try
	{
		__asm
		{
			int 0x2d
			xor eax, eax
			add eax, 2
		}
	}
	__except (EXCEPTION_EXECUTE_HANDLER)
	{
		return FALSE;
	}
	
	return TRUE;
}

// CheckCloseHandle will call CloseHandle on an invalid
// DWORD aligned value and if a debugger is running an exception
// will occur and the function will return TRUE otherwise it'll
// return FALSE
inline BOOL CheckDbgPresentCloseHandle()
{
	HANDLE Handle = (HANDLE)0x8000;
	__try
	{
		CloseHandle(Handle);
	}
	__except (EXCEPTION_EXECUTE_HANDLER)
	{
		return TRUE;
	}
	
	return FALSE;
}

// The IsDbgPresentPrefixCheck works in at least two debuggers
// OllyDBG and VS 2008, by utilizing the way the debuggers handle
// prefixes we can determine their presence. Specifically if this code
// is ran under a debugger it will simply be stepped over;
// however, if there is no debugger SEH will fire :D
inline BOOL IsDbgPresentPrefixCheck()
{
	__try
	{
		__asm __emit 0xF3 // 0xF3 0x64 disassembles as PREFIX REP:
		__asm __emit 0x64
		__asm __emit 0xF1 // One byte INT 1
	}
	__except (EXCEPTION_EXECUTE_HANDLER)
	{
		return FALSE;
	}

	return TRUE;
}
