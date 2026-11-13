#pragma once

// Giao dien danh sach toa do da chuyen hẳn vào tab Di chuyển (CDlgMove).
// Danh sach 0: toa do luyen cong (m_Nodes); Danh sach 1: di den noi luyen cong (m_GoNodes).
// File nay chi con giu 2 ham tien ich tinh (ten ban do train) duoc goi tu nhieu tab.

#define MOVEPOINTS_LISTS	2

class CDlgMovePoints
{
public:
	static CString	GetMapName(int nMapID);		// Ten ban do theo Maps\MapNames.ini -> "Ten (id)"
	static int		GetTrainMapID();			// Map train = map cua danh sach toa do (da bo "Noi luyen cong")
};
