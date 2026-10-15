// DWinProcess.cpp : implementation file
//

#include "stdafx.h"
#include "DWinProcess.h"

DWinProcess::DWinProcess(HWND hWnd, DWORD dwDesiredAccess)
{
	m_hProcess = NULL;
	if (!IsWindow(hWnd))
		return;

	DWORD dwProcessID;
	GetWindowThreadProcessId(hWnd, &dwProcessID);
	if (dwProcessID == NULL)
		return;

	m_hProcess = OpenProcess(dwDesiredAccess, FALSE, dwProcessID);
}

SIZE_T DWinProcess::ReadMemory(LPCVOID lpBaseAddress, LPVOID lpBuffer, SIZE_T nSize)
{
	if (!m_hProcess)
		return 0;

	SIZE_T nReadSize = 0;
	if (!ReadProcessMemory(m_hProcess, lpBaseAddress, lpBuffer, nSize, &nReadSize))
		return 0;
	else
		return nReadSize;
}
