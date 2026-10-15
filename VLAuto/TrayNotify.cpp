// TrayNotify.cpp: implementation of the CTrayNotify class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "TrayNotify.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CTrayNotify::CTrayNotify()
{
}

CTrayNotify::~CTrayNotify()
{
}

BOOL CTrayNotify::TrayMessage(HWND hWnd, DWORD dwMessage, UINT uID, HICON hIcon, LPCTSTR pszTip)
{
	BOOL res;
	NOTIFYICONDATA tnd;
	memset(&tnd, 0, sizeof(tnd));
	tnd.cbSize = sizeof(NOTIFYICONDATA);
	tnd.hWnd = hWnd;
	tnd.uID	= uID;
	tnd.uFlags = NIF_MESSAGE;
	tnd.uCallbackMessage = WM_NOTIFYICON;
	if (hIcon) {
		tnd.uFlags = tnd.uFlags|NIF_ICON;
		tnd.hIcon = hIcon;
	}
	if (pszTip)
	{
		tnd.uFlags = tnd.uFlags|NIF_TIP;
		lstrcpyn(tnd.szTip, pszTip, sizeof(tnd.szTip));
	}
	res = Shell_NotifyIcon(dwMessage, &tnd);
	if (hIcon)
		DestroyIcon(hIcon);
	return res;
}
