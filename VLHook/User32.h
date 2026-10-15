#pragma once

struct StructFuncs
{
	const char* FuncName;
	FARPROC* FuncAdd;
};

extern BOOL (WINAPI *_OpenClipboard)(HWND);
extern BOOL (WINAPI *_EmptyClipboard)();
extern BOOL (WINAPI *_CloseClipboard)();
extern HANDLE (WINAPI *_SetClipboardData)(UINT, HANDLE);
extern BOOL (WINAPI *_ShowWindow)(HWND, int);
extern BOOL (WINAPI *_SetForegroundWindow)(HWND);
extern BOOL (WINAPI *_IsWindow)(HWND);
extern HHOOK (WINAPI *_SetWindowsHookExA)(int, HOOKPROC, HINSTANCE, DWORD);
extern HHOOK (WINAPI *_SetWindowsHookExW)(int, HOOKPROC, HINSTANCE, DWORD);
extern LRESULT (WINAPI *_SendMessageA)(HWND, UINT, WPARAM, LPARAM);
extern LRESULT (WINAPI *_SendMessageW)(HWND, UINT, WPARAM, LPARAM);
extern BOOL (WINAPI *_UnhookWindowsHookEx)(HHOOK);
extern LONG (WINAPI *_SetWindowLongA)(HWND, int, LONG);
extern LONG (WINAPI *_SetWindowLongW)(HWND, int, LONG);
extern UINT_PTR (WINAPI *_SetTimer)(HWND hWnd, UINT_PTR, UINT, TIMERPROC);
extern BOOL (WINAPI *_KillTimer)(HWND, UINT_PTR);
extern LRESULT (WINAPI *_CallNextHookEx)(HHOOK, int, WPARAM, LPARAM);
extern DWORD (WINAPI *_GetWindowThreadProcessId)(HWND, LPDWORD);
extern SHORT (WINAPI *_GetKeyState)(int);
extern SHORT (WINAPI *_GetAsyncKeyState)(int);
extern BOOL (WINAPI *_PostMessageA)(HWND, UINT, WPARAM, LPARAM);
extern BOOL (WINAPI *_PostMessageW)(HWND, UINT, WPARAM, LPARAM);
extern BOOL (WINAPI *_PtInRect)(CONST RECT *, POINT);
extern LRESULT (WINAPI *_CallWindowProcA)(WNDPROC, HWND, UINT, WPARAM, LPARAM);
extern LRESULT (WINAPI *_CallWindowProcW)(WNDPROC, HWND, UINT, WPARAM, LPARAM);
extern BOOL (WINAPI *_GetKeyboardState)(LPBYTE);
extern BOOL (WINAPI *_SetKeyboardState)(LPBYTE);
extern BOOL (WINAPI *_GetCursorPos)(LPPOINT);
extern BOOL (WINAPI *_SetCursorPos)(int, int);

#ifdef UNICODE
	#define _SetWindowsHookEx		_SetWindowsHookExW
	#define _SendMessage			_SendMessageW
	#define _SetWindowLong			_SetWindowLongW
	#define _PostMessage			_PostMessageW
	#define _CallWindowProc			_CallWindowProcW
#else
	#define _SetWindowsHookEx		_SetWindowsHookExA
	#define _SendMessage			_SendMessageA
	#define _SetWindowLong			_SetWindowLongA
	#define _PostMessage			_PostMessageA
	#define _CallWindowProc			_CallWindowProcA
#endif // !UNICODE
