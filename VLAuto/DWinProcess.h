// DWinProcess.h : header file
//

#pragma once

#include <psapi.h>
#pragma comment(lib, "psapi.lib")

class DWinProcess
{
private:
	HANDLE m_hProcess;

public:
	DWinProcess(HWND hWnd, DWORD dwDesiredAccess = PROCESS_VM_READ | PROCESS_QUERY_INFORMATION);
	virtual ~DWinProcess() { if (m_hProcess != NULL) CloseHandle(m_hProcess); }

	HANDLE GetHandle() const { return m_hProcess; }

	SIZE_T ReadMemory(LPCVOID lpBaseAddress, LPVOID lpBuffer, SIZE_T nSize);

	SIZE_T VirtualQueryEx(LPCVOID lpAddress, PMEMORY_BASIC_INFORMATION lpBuffer, SIZE_T dwLength)
	{
		return ::VirtualQueryEx(m_hProcess, lpAddress, lpBuffer, dwLength);
	}

	DWORD GetModuleBaseName(HMODULE hModule, LPTSTR lpBaseName, DWORD nSize)
	{
		return ::GetModuleBaseName(m_hProcess, hModule, lpBaseName, nSize);
	}
	DWORD GetModuleFileNameEx(HMODULE hModule, LPTSTR lpFilename, DWORD nSize)
	{
		return ::GetModuleFileNameEx(m_hProcess, hModule, lpFilename, nSize);
	}
};
