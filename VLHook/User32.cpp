#include "stdafx.h"
#include "User32.h"

BOOL (WINAPI *_OpenClipboard)(HWND);
BOOL (WINAPI *_EmptyClipboard)();
BOOL (WINAPI *_CloseClipboard)();
HANDLE (WINAPI *_SetClipboardData)(UINT, HANDLE);
BOOL (WINAPI *_ShowWindow)(HWND, int);
BOOL (WINAPI *_SetForegroundWindow)(HWND);
BOOL (WINAPI *_IsWindow)(HWND);
HHOOK (WINAPI *_SetWindowsHookExA)(int, HOOKPROC, HINSTANCE, DWORD);
HHOOK (WINAPI *_SetWindowsHookExW)(int, HOOKPROC, HINSTANCE, DWORD);
LRESULT (WINAPI *_SendMessageA)(HWND, UINT, WPARAM, LPARAM);
LRESULT (WINAPI *_SendMessageW)(HWND, UINT, WPARAM, LPARAM);
BOOL (WINAPI *_UnhookWindowsHookEx)(HHOOK);
LONG (WINAPI *_SetWindowLongA)(HWND, int, LONG);
LONG (WINAPI *_SetWindowLongW)(HWND, int, LONG);
UINT_PTR (WINAPI *_SetTimer)(HWND hWnd, UINT_PTR, UINT, TIMERPROC);
BOOL (WINAPI *_KillTimer)(HWND, UINT_PTR);
LRESULT (WINAPI *_CallNextHookEx)(HHOOK, int, WPARAM, LPARAM);
DWORD (WINAPI *_GetWindowThreadProcessId)(HWND, LPDWORD);
SHORT (WINAPI *_GetKeyState)(int);
SHORT (WINAPI *_GetAsyncKeyState)(int);
BOOL (WINAPI *_PostMessageA)(HWND, UINT, WPARAM, LPARAM);
BOOL (WINAPI *_PostMessageW)(HWND, UINT, WPARAM, LPARAM);
BOOL (WINAPI *_PtInRect)(CONST RECT *, POINT);
LRESULT (WINAPI *_CallWindowProcA)(WNDPROC, HWND, UINT, WPARAM, LPARAM);
LRESULT (WINAPI *_CallWindowProcW)(WNDPROC, HWND, UINT, WPARAM, LPARAM);
BOOL (WINAPI *_GetKeyboardState)(LPBYTE);
BOOL (WINAPI *_SetKeyboardState)(LPBYTE);
BOOL (WINAPI *_GetCursorPos)(LPPOINT);
BOOL (WINAPI *_SetCursorPos)(int, int);

StructFuncs User32ExportFuncs[] = 
{
	{ "OpenClipboard", (FARPROC *)&_OpenClipboard },
	{ "EmptyClipboard", (FARPROC *)&_EmptyClipboard },
	{ "CloseClipboard", (FARPROC *)&_CloseClipboard },
	{ "SetClipboardData", (FARPROC *)&_SetClipboardData },
	{ "ShowWindow", (FARPROC *)&_ShowWindow },
	{ "SetForegroundWindow", (FARPROC *)&_SetForegroundWindow },
	{ "IsWindow", (FARPROC *)&_IsWindow },
	{ "SetWindowsHookExA", (FARPROC *)&_SetWindowsHookExA },
	{ "SetWindowsHookExW", (FARPROC *)&_SetWindowsHookExW },
	{ "SendMessageA", (FARPROC *)&_SendMessageA },
	{ "SendMessageW", (FARPROC *)&_SendMessageW },
	{ "UnhookWindowsHookEx", (FARPROC *)&_UnhookWindowsHookEx },
	{ "SetWindowLongA", (FARPROC *)&_SetWindowLongA },
	{ "SetWindowLongW", (FARPROC *)&_SetWindowLongW },
	{ "SetTimer", (FARPROC *)&_SetTimer },
	{ "KillTimer", (FARPROC *)&_KillTimer },
	{ "CallNextHookEx", (FARPROC *)&_CallNextHookEx },
	{ "GetWindowThreadProcessId", (FARPROC *)&_GetWindowThreadProcessId },
	{ "GetKeyState", (FARPROC *)&_GetKeyState },
	{ "GetAsyncKeyState", (FARPROC *)&_GetAsyncKeyState },
	{ "PostMessageA", (FARPROC *)&_PostMessageA },
	{ "PostMessageW", (FARPROC *)&_PostMessageW },
	{ "PtInRect", (FARPROC *)&_PtInRect },
	{ "CallWindowProcA", (FARPROC *)&_CallWindowProcA },
	{ "CallWindowProcW", (FARPROC *)&_CallWindowProcW },
	{ "GetKeyboardState", (FARPROC *)&_GetKeyboardState },
	{ "SetKeyboardState", (FARPROC *)&_SetKeyboardState },
	{ "GetCursorPos", (FARPROC *)&_GetCursorPos },
	{ "SetCursorPos", (FARPROC *)&_SetCursorPos },
};

BOOL InitUser32Dll()
{
	HMODULE hUser32 = LoadLibrary("User32.dll");
	if (!hUser32)
		return FALSE;

	int nFuncNums = sizeof(User32ExportFuncs) / sizeof(StructFuncs);
	for (int i = 0; i < nFuncNums; i++)
	{
		*(User32ExportFuncs[i].FuncAdd) = GetProcAddress(hUser32, User32ExportFuncs[i].FuncName);
		if (*(User32ExportFuncs[i].FuncAdd) == NULL)
			return FALSE;
	}

	return TRUE;
}

BOOL bIsUser32OK = InitUser32Dll();
