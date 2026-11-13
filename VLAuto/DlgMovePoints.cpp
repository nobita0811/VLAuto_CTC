// DlgMovePoints.cpp : chi con cac ham tien ich tinh (ten ban do train).
// Giao dien danh sach toa do da chuyen vao CDlgMove (tab Di chuyen).
//

#include "stdafx.h"
#include "../GameDef.h"
#include "VLAuto.h"
#include "GameCore.h"
#include "GameWindow.h"
#include "DlgMovePoints.h"

// Ten ban do theo MapNames.ini (giong tab Co ban)
CString CDlgMovePoints::GetMapName(int nMapID)
{
	TCHAR szMapID[12], szInfo[64];
	TCHAR szMapNameFile[MAX_PATH];
	_stprintf(szMapID, _T("%d"), nMapID);
	_stprintf(szMapNameFile, _T("%s\\Maps\\MapNames.ini"), g_szAppFolder);
	GetPrivateProfileString(_T("Map_Name"), szMapID, _T("<Unknown_Map>"), szInfo, 64, szMapNameFile);
	char szTCNV3Str[64];
	WideCharToMultiByte(CP_ACP, 0, szInfo, -1, szTCNV3Str, 64, 0, 0);
	TCVN3ToUnicode(szTCNV3Str, szInfo, 63);
	CString szName;
	szName.Format(_T("%s (%d)"), szInfo, nMapID);
	return szName;
}

// Bo "Noi luyen cong" (Tab Ban do): map train = map cua danh sach toa do luyen cong,
// khong co thi lay map cua danh sach toa do di den noi luyen cong (hook cung tinh y nhu vay)
int CDlgMovePoints::GetTrainMapID()
{
	if (g_pCurGameWin == NULL) return 0;
	PARAM_MOVE &pm = g_pCurGameWin->m_prMove;
	// Map train chi co khi tick 1 trong 2 option (loai tru nhau); khong tick = "Ban do bat ky"
	if (pm.m_bRouteMove && pm.m_nNodesMapID && pm.m_Nodes[0].x && pm.m_Nodes[0].y) return pm.m_nNodesMapID;
	if (pm.m_bAroundPoint && pm.m_nPointMapID && pm.m_nPointX && pm.m_nPointY) return pm.m_nPointMapID;
	return 0;
}
