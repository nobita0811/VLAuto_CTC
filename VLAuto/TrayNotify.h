// TrayNotify.h: interface for the CTrayNotify class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_TRAYNOTIFY_H__553EF0D0_D51C_445D_B809_C2A715C30843__INCLUDED_)
#define AFX_TRAYNOTIFY_H__553EF0D0_D51C_445D_B809_C2A715C30843__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#define WM_NOTIFYICON WM_APP + 100

class CTrayNotify  
{
public:
	BOOL TrayMessage(HWND hWnd, DWORD dwMessage, UINT uID, HICON hIcon, LPCTSTR pszTip);
	CTrayNotify();
	virtual ~CTrayNotify();

};

#endif // !defined(AFX_TRAYNOTIFY_H__553EF0D0_D51C_445D_B809_C2A715C30843__INCLUDED_)
