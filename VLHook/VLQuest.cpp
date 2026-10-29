#include "stdafx.h"
#include <io.h>
#include <sys/stat.h>
#include <sys/types.h>
#include "../GameDef.h"
#include "DTextFile.h"
#include "GameBase.h"
#include "JxCore.h"
#include "S3Client.h"
#include "VLClasses.h"
#include "VLDebug.h"
#include "TextFunc.h"
#include "MagicAttrib.h"
// Tu khoa (chu thuong, TCVN3)
//: T×m Trang bÞ t¨ng Néi Lùc lín h¬n 150 ®iÓm 
static const char* const YS_KW_CURQUEST = TV(L"Nhiệm vụ hiện tại");	// nhiem vu hien tai
static const char* const YS_KW_KILL = TV(L"tiêu diệt");				// tieu diet
static const char* const YS_KW_KILL2 = TV(L"tiệu diệt");				// server CTC viet sai (cung do dai YS_KW_KILL)
static const char* const YS_KW_AT = TV(L" ở ");						// " o "
static const char* const YS_KW_FIND = TV(L"Tìm Trang bị");						// Tìm Trang bị
static const char* const YS_KW_INC = TV(L"tăng");						// tăng
static const char* const YS_KW_GREATER = TV(L"lớn hơn");				// lon hon
//================================Quest Trace================================

extern const char* CANCEL_BY_USER;	// VLYeSou.cpp
#define YS_MAX_MAGIC_ATTRIB		546	// magic_end + 1 (MagicAttrib.cpp)

#if QUEST_TRACE
void QTraceSite(char *szLast, const char *szFormat, ...)
{
	char szMsg[256];
	va_list vlist;
	va_start(vlist, szFormat);
	_vsnprintf(szMsg, sizeof(szMsg) - 1, szFormat, vlist);
	va_end(vlist);
	szMsg[sizeof(szMsg) - 1] = 0;

	if (!strcmp(szMsg, szLast)) return;
	strcpy(szLast, szMsg);

	SystemMessage(1, "<color=cyan>[QT] %s", szMsg);
	char szLog[270];
	_snprintf(szLog, sizeof(szLog) - 1, "[QT] %s", szMsg);
	szLog[sizeof(szLog) - 1] = 0;
	theQuest.SaveYSLog(szLog);
}
#endif

const char* QTName_Gen(int nGen)
{
	static const char* szNames[] = { "qt_none", "qt_coll", "qt_yesou", "qt_dopk", "qt_suspending", "qt_suspended" };
	return (nGen >= 0 && nGen < _countof(szNames)) ? szNames[nGen] : "qt_?";
}

const char* QTName_YS(int nYS)
{
	static const char* szNames[] = { "ys_none", "ys_doing", "ys_return", "ys_cancel", "ys_suspending" };
	return (nYS >= 0 && nYS < _countof(szNames)) ? szNames[nYS] : "ys_?";
}

const char* QTName_Rq(int nRq)
{
	static const char* szNames[] = { "unknown", "tuzhi", "mizhi", "jingyan", "mingwang", "fuyuan", "pk", "songjin", "shanhe" };
	return (nRq >= 0 && nRq < _countof(szNames)) ? szNames[nRq] : "rq_?";
}

//================================Class CVLQuest================================
CVLQuest::CVLQuest(void)
{
	m_nQuestGID = -1;
	m_nGenDoing = qt_none;
	m_nYSDoing = ys_none;
	m_nBHDoing = bh_none;
	m_bInfoLoaded = 0;
	pTask01 = NULL; pTask02 = NULL;
	pTask03 = NULL; pTask05 = NULL; pTask06 = NULL;

	m_bUseDoubleExp = 0;
	m_bNeedGoTrain = 0;
	m_bNeedGoPCK = 0;
	m_bGoTrainGetPK = 0;
	m_nLastMapID = 0;
	m_nYeSouMapID = 0;
	m_nRemainQuest = -1;
	m_tmStart = 0;
	memset(m_szYSQuest, 0, sizeof(m_szYSQuest));
	m_bReturnItem = 0;
	memset(m_bVsNpc, 0, sizeof(m_bVsNpc));
	m_nArrangeStep = 0;
	m_nNewCollTask = 0;
	m_nTotalPKQuest = 0;
	m_bIsYeSouAndPK = 0;
	m_bGetMingWang = TRUE;
	m_bIsFull100LB = 0;
	m_bNoVipCard = 0;
}

void CVLQuest::Breath()
{
	if (!m_bInfoLoaded)
	{
		QTRACE("Breath: chua nap duoc Maps\\YSInfoX.dat");
		return;
	}
	QTRACE("Breath: Gen=%s Entrust=%d", QTName_Gen(m_nGenDoing), g_pCurPlayer->m_nEntrustMode);

	if (m_nGenDoing == qt_suspending)
	{
		DoSuspendOpt(); // khi ngưng nhiệm vụ thì chạy hàm này
		return;
	}
	if (m_nGenDoing == qt_suspended) return;

	if (g_pCurPlayer->m_nEntrustMode) return; // ủy thác thì không làm nhiệm vụ

	switch (m_nGenDoing)
	{
	case qt_none:
		m_nYSDoing = ys_none;
		m_nBHDoing = bh_none;
		m_nDoQuestStep = 0;
		m_bNeedGoTrain = 0;
		m_bNeedGoPCK = 0;
		m_bGoTrainGetPK = 0;
		m_bIsFull100LB = 0;
		m_nTotalPKQuest = 0;
		m_bGetMingWang = TRUE;
		m_nGenDoing = qt_yesou;
		break;
	//case qt_coll:
	//	BreathBH();
	//	break;
	case qt_yesou:
		BreathYS();
		break;
	case qt_dopk:
		BreathPK();
		break;
	}
}

void CVLQuest::CheckStatus()
{
	if (g_NetworkOffline) return;

	/*if (ONLINE_TIME_SEC < 6)
	{	// A new day, restart all values...
		if (P_PLAYER->m_FightMode && m_bAutoDoQuest)
		{
			theRecover.OnUseAM();
			m_nGenDoing = qt_none;
			return;
		}
		if (g_pCurPlayer->m_nEntrustMode)
		{
			theUtil.m_nEntrustType = 1; // Cancel
			m_nGenDoing = qt_none;
			return;
		}
	}*/

	KUiMsgSel *pMsgSel = (KUiMsgSel*)GetPtrAddress(MSG_BASE_ADD);
	if (pMsgSel && pMsgSel->IsVisible())
	{
		if (strstr(pMsgSel->m_InfoText, TV(L"Lần này chúng ta đã khiến hắn tâm phục")))
		{
			pMsgSel->OnClickMsg(0);
			return;
		}
	}

	int nBHCurMapID = 0;
	m_nNpcSIndex = 0;
	if (m_nTongKill == act_auto)
	{
		int nBHCurTask = g_pCurPlayer->GetTaskVal(bh_curtask);
		if (nBHCurTask > 300 && nBHCurTask < 400)
		{
			nBHCurMapID = pTask08[nBHCurTask-301].nMapID;
			m_nNpcSIndex = pTask08[nBHCurTask-301].nNpcSettingIdx;
		}
	}

	if (m_nGenDoing == qt_dopk)
	{
		if (!P_PLAYER->m_FightMode)
			return; // Nothing to do

		time_t tmNow;
		time(&tmNow);
		int nTimeCount = (int)(tmNow - m_tmStart);

		if (nTimeCount % 11 == 3 && m_nDoQuestStep == 15)
			m_nDoQuestStep = 14; // Kill victim again!

		if (nTimeCount > 240) // 4 minutes
		{
			nTimeCount = 0;
			SaveYSLog(TV(L"Quá thời gian, về thành hủy nhiệm vụ!\n"));
			SystemMessage(TV(L"<color=yellow>Quá thời gian, về thành hủy nhiệm vụ!"));
			m_nDoQuestStep = 40;
			theRecover.OnUseAM();
			return;
		}

		return;
	}

	YESOU_TASK_DOING ysResult = ReportQuest(TRUE);
	if (P_PLAYER->m_FightMode && ysResult > ys_doing)
	{
		if (nBHCurMapID == (*g_pMapID)) return; // Doing Kill Quest

		switch (ysResult)
		{
		case ys_return:
			SystemMessage(TV(L"<color=yellow>Về thành trả nhiệm vụ!"));
			break;
		case ys_cancel:
			SystemMessage(TV(L"<color=yellow>Về thành hủy nhiệm vụ!"));
			theQuest.m_nYSDoing = ys_cancel;
			break;
		case ys_suspending:
			SystemMessage(TV(L"<color=yellow>Về thành kiểm tra nhiệm vụ!"));
			break;
		}
		theRecover.OnUseAM();
	}
}

// File log ghi UTF-8 (co BOM): chuoi trong auto/game la TCVN3 => doi truoc khi ghi
static void LogPuts(FILE *pFile, const char *szTcvn)
{
	char szUtf8[1600];	// 1 byte TCVN3 -> toi da 3 byte UTF-8
	char szPart[512];
	int nLen = (int)strlen(szTcvn);
	for (int nPos = 0; nPos < nLen; nPos += sizeof(szPart) - 1)
	{
		strncpy(szPart, szTcvn + nPos, sizeof(szPart) - 1);
		szPart[sizeof(szPart) - 1] = 0;
		Tcvn3ToUtf8(szPart, szUtf8, sizeof(szUtf8));
		fputs(szUtf8, pFile);
	}
}

static void LogPrintf(FILE *pFile, const char *szFormat, ...)
{
	char szMsg[512];
	va_list vlist;
	va_start(vlist, szFormat);
	_vsnprintf(szMsg, sizeof(szMsg) - 1, szFormat, vlist);
	va_end(vlist);
	szMsg[sizeof(szMsg) - 1] = 0;
	LogPuts(pFile, szMsg);
}

void CVLQuest::SaveYSLog(const char *szLogInfo)
{
	FILE *pLogFile = fopen(m_szLogFile, "at");
	if (pLogFile == NULL) return;
	if (szLogInfo)
	{
		time_t tmNow; time(&tmNow);
		tm lct; localtime_s(&lct, &tmNow);
		fprintf(pLogFile, "[%02d-%02d %02d:%02d:%02d] ", lct.tm_mday, lct.tm_mon + 1, lct.tm_hour, lct.tm_min, lct.tm_sec);
		LogPuts(pLogFile, szLogInfo);
	}
	fprintf(pLogFile, "\n");
	fclose(pLogFile);
}

void CVLQuest::PrepareLog()
{
	char szEngName[64] = "";
	int i = 0, j = 0;
	while (P_PLAYER->Name[i] && i < 30 && j < 60)
	{
		if (P_PLAYER->Name[i] > 0x1F && P_PLAYER->Name[i] < 0x7F && !strchr("\\/:*?\"<>|", P_PLAYER->Name[i]))
			szEngName[j++] = P_PLAYER->Name[i];
		else
		{
			BYTE btHi = (P_PLAYER->Name[i] >> 4) & 0x0f;
			BYTE btLo = P_PLAYER->Name[i] & 0x0f;
			if (btHi < 10) btHi += 0x30; else btHi += 0x57;
			if (btLo < 10) btLo += 0x30; else btLo += 0x57;
			szEngName[j++] = (char)btHi;
			szEngName[j++] = (char)btLo;
		}
		i++;
	}

	time_t tmNow; time(&tmNow);
	tm lct; localtime_s(&lct, &tmNow);

	BOOL bStartNewFile = 0;
	BOOL bStartNewDate = TRUE;
	sprintf(m_szLogFile, "%s\\Logs\\%s.log", theMain.m_szCurFolder, szEngName);

	FILE *pLogFile = fopen(m_szLogFile, "rt");
	if (pLogFile == NULL)
		bStartNewFile = TRUE;
	else
	{
		int fh = _fileno(pLogFile);
		struct _stat fbuf;
		_fstat(fh, &fbuf);
		tm fmt; localtime_s(&fmt, &fbuf.st_mtime);
		if (fmt.tm_mday == lct.tm_mday && fmt.tm_mon == lct.tm_mon)
			bStartNewDate = 0;
		long fsize = fbuf.st_size;
		char szBom[3] = { 0 };
		BOOL bUtf8 = fread(szBom, 1, 3, pLogFile) == 3 && !memcmp(szBom, "\xEF\xBB\xBF", 3);
		fclose(pLogFile);
		if (fsize > 32000 || !bUtf8)	// file cu ghi TCVN3 (khong BOM) => cat sang _bak, mo file UTF-8 moi
		{
			char szBakFile[MAX_PATH];
			sprintf(szBakFile, "%s\\Logs\\%s_bak.log", theMain.m_szCurFolder, szEngName);
			remove(szBakFile);
			rename(m_szLogFile, szBakFile);
			bStartNewFile = TRUE;
			bStartNewDate = TRUE;
		}
	}

	pLogFile = fopen(m_szLogFile, "at");
	if (pLogFile == NULL) return;
	if (bStartNewFile)
	{
		fputs("\xEF\xBB\xBF", pLogFile);	// BOM UTF-8
		LogPrintf(pLogFile, TV(L"====** UTF-8 **====\n%s - Nhật ký Nhiệm vụ VLAuto\n\n"), P_PLAYER->Name);
	}
	if (bStartNewDate)
	{
		LogPrintf(pLogFile, TV(L"Ngày %02d-%02d-%04d\n"), lct.tm_mday, lct.tm_mon + 1, lct.tm_year + 1900);
		fprintf(pLogFile, "======================================================================\n");
	}
	fprintf(pLogFile, "\n[%02d-%02d %02d:%02d:%02d] ", lct.tm_mday, lct.tm_mon + 1, lct.tm_hour, lct.tm_min, lct.tm_sec);
	LogPuts(pLogFile, TV(L"Khởi động VLAuto...\n"));
	fclose(pLogFile);
}

BOOL CVLQuest::LoadTaskInfo()
{
	char szInfoFile[MAX_PATH];
	sprintf(szInfoFile, "%s\\Maps\\YSInfoX.dat", theMain.m_szCurFolder);
	FILE* pFile;
	pFile = fopen(szInfoFile, "rb");
	if (pFile == NULL) return 0;
	m_bInfoLoaded = TRUE;
	fread(&TaskInfoHeader, sizeof(TaskInfoHeader), 1, pFile);
	if (strcmp(TaskInfoHeader.szVersion, "VLAUTO_0600"))
	{
		fclose(pFile);
		return 0;
	}

	pTask01 = new YS_Task01_Data[TaskInfoHeader.nTaskNum[0]];
	fread(pTask01, sizeof(YS_Task01_Data)*TaskInfoHeader.nTaskNum[0], 1, pFile);
	pTask02 = new YS_Task02_Data[TaskInfoHeader.nTaskNum[1]];
	fread(pTask02, sizeof(YS_Task02_Data)*TaskInfoHeader.nTaskNum[1], 1, pFile);
	pTask03 = new YS_Task03_Data[TaskInfoHeader.nTaskNum[2]];
	fread(pTask03, sizeof(YS_Task03_Data)*TaskInfoHeader.nTaskNum[2], 1, pFile);
	pTask05 = new YS_Task05_Data[TaskInfoHeader.nTaskNum[4]];
	fread(pTask05, sizeof(YS_Task05_Data)*TaskInfoHeader.nTaskNum[4], 1, pFile);
	pTask06 = new YS_Task05_Data[TaskInfoHeader.nTaskNum[5]];
	fread(pTask06, sizeof(YS_Task05_Data)*TaskInfoHeader.nTaskNum[5], 1, pFile);
	pTask07 = new BH_Task01_Data[TaskInfoHeader.nTaskNum[6]];
	fread(pTask07, sizeof(BH_Task01_Data)*TaskInfoHeader.nTaskNum[6], 1, pFile);
	pTask08 = new BH_Task02_Data[TaskInfoHeader.nTaskNum[7]];
	fread(pTask08, sizeof(BH_Task02_Data)*TaskInfoHeader.nTaskNum[7], 1, pFile);
	fclose(pFile);

	return TRUE;
}

void CVLQuest::CheckAutoYeSou()
{
	char szMsg[80];
	m_bNoVipCard = 0;	// Bat lai / doi tuy chon: cho thu lai (vd. vua kich hoat the Vip)
	if (m_bAutoDoQuest)
	{
		sprintf(szMsg, TV(L"<color=yellow>Tự làm Nhiệm vụ (Ctrl+D): <color=green>Bật"));
		if (!LoadTaskInfo())
			sprintf(szMsg, TV(L"Lỗi đọc file Dữ liệu, xin hãy tải lại phần mềm!"));
	}
	else
	{
		sprintf(szMsg, TV(L"<color=yellow>Tự làm Nhiệm vụ (Ctrl+D): <color>Tắt"));
		m_nYSDoing = ys_none;
		theMap.m_bFirstSaveRepos = TRUE;
		theMap.m_nCurPoint = -1;
		theMap.m_bDoShoping = 0;
		m_bInfoLoaded = 0;
		if (pTask01) { delete pTask01; pTask01 = NULL; }
		if (pTask02) { delete pTask02; pTask02 = NULL; }
		if (pTask03) { delete pTask03; pTask03 = NULL; }
		if (pTask05) { delete pTask05; pTask05 = NULL; }
		if (pTask06) { delete pTask06; pTask06 = NULL; }
		if (pTask07) { delete pTask07; pTask07 = NULL; }
		if (pTask08) { delete pTask08; pTask08 = NULL; }
	}
	SystemMessage(szMsg, 1);
}

void CVLQuest::StopNoVip()
{
	m_bNoVipCard = TRUE;
	m_nYSDoing = ys_suspending;
	theMap.ResetShoping();	// Dong hop thoai, thoi dung o NPC
	SaveYSLog("Chua kich hoat the Vip, khong nhan duoc nhiem vu Da Tau => Ngung!\n");
	//SystemMessage("<color=yellow>Chua kich hoat the Vip, khong nhan duoc nhiem vu Da Tau => Ngung!", 1);
	SystemMessage(TV(L"<<color=green>VLCTCPro<color> : <color=yellow>Chưa kích hoạt thẻ Vip , Auto Dã Tẩu STOP!"), 1);
	PostChatMessage(TV(L"<color=green>VLCTCPro<color> : <color=yellow>Chưa kích hoạt thẻ Vip , Auto Dã Tẩu STOP!"));
}


void CVLQuest::DoSuspendOpt()
{
	switch (m_nPostAction)
	{
	case 0: // Dung im
		SaveYSLog(TV(L"Ngưng làm Nhiệm vụ, đứng im.\n"));
		SystemMessage(TV(L"<color=yellow>Ngưng làm Nhiệm vụ, đứng im."));
		break;
	case 1: // Thoat game
		theMain.ExitGame();
		SaveYSLog(TV(L"Ngưng làm Nhiệm vụ, thoát Game.\n"));
		break;
	case 2: // Bao chuong
		theMain.PostFeedBack(cmd_amulet);
		SaveYSLog(TV(L"Ngưng làm Nhiệm vụ, báo chuông.\n"));
		SystemMessage(TV(L"<color=yellow>Ngưng làm Nhiệm vụ, báo chuông."));
		break;
	case 3: // Di luyen cong
		m_bNeedGoTrain = TRUE;
		m_bNeedGoPCK = 0;
		m_bGoTrainGetPK = 0;
		theMap.m_bFirstSaveRepos = TRUE;
		theMap.m_bNoDriverRecent = TRUE;
		if (!theMap.m_bAutoReturn)
			theMain.PostFeedBack(cmd_setmap);
		SaveYSLog(TV(L"Ngưng làm Nhiệm vụ, đi luyện công.\n"));
		SystemMessage(TV(L"<color=yellow>Ngưng làm Nhiệm vụ, đi luyện công."));
		break;
	}

	// Reset Status Values
	m_bUseDoubleExp = 0;
	m_nLastMapID = 0;
	theMap.m_bNeedMoney = theMap.m_bWithDraw;
	theMap.m_bDoShoping = 0;
	theMap.m_nCurPoint = -1;
	m_nGenDoing = qt_suspended;
}
// dành cho nhiệm vụ Dã tẩu mới (bản CTC 15-10_2026 trở đi)

/*
* Kết quả sau khi hàm chạy:

Loại				m_nYesouCount					m_szYesouTarget					m_szYesouMap / m_nYesouMapID
ys_attackNpc		số quái cần diệt (50)			tên quái “TuyÕt anh”			tên map và ID map
ys_ItemOption		giá trị cần lớn hơn (150)		tên thuộc tính “Néi Lùc”		(trống)

0					0								(trống)							chưa nhận ra loại; chữ gốc vẫn còn trong m_szYesouCurQuest
*/
void CVLQuest::GetYesouQuestFromKUiMsgSel2(KUiMsgSel2* pMsgSel2)
{
	m_nYesouKind = 0; // reset lại loại nhiệm vụ Dã tẩu
	m_nYesouCount = 0;
	m_nYesouMapID = 0;
	m_szYesouTarget[0] = m_szYesouMap[0] = m_szYesouCurQuest[0] = 0;
	m_szQuestYesouInfo[0] = 0;
	if (!pMsgSel2) return;

	// dòng text báo nhiệm vụ (đã bỏ mã màu + thẻ <..>)
	YsPlainText(pMsgSel2->GetQuestionText(), m_szQuestYesouInfo, sizeof(m_szQuestYesouInfo));

	/*
	  Loi thoai: Tổng số nhiệm vụ đã hoàn thành: 2 lần.  Số nhiệm vụ hôm nay đã nhận: 0 / 10 lần. Nhiệm vụ hiện tại: Tìm Trang bị tăng Nội Lực lớn hơn 150 điểm
	  Loi thoai: ... Nhiệm vụ hiện tại: Tiêu diệt 50 Tuyết anh ở Chân Núi Trường Bạch
	*/
	const char* p = YsFindFold(m_szQuestYesouInfo, YS_KW_CURQUEST);
	if (!p)
	{
		QTRACE("GetYesouQuest: khong thay 'Nhiem vu hien tai' trong loi thoai");
		return;
	}
	p = YsSkipSpace(p + strlen(YS_KW_CURQUEST));
	if (*p == ':') p = YsSkipSpace(p + 1);
	const char* pEnd = p;
	while (*pEnd && *pEnd != '.' && *pEnd != '!' && *pEnd != ';') pEnd++;
	YsCopyTrim(m_szYesouCurQuest, sizeof(m_szYesouCurQuest), p, pEnd);
	const char* szQuest = m_szYesouCurQuest;

	const char* q = YsFindFold(szQuest, YS_KW_KILL);
	if (!q) q = YsFindFold(szQuest, YS_KW_KILL2);
	if (q)
	{
		// Tieu diet N <ten quai> o <ten map>
		q = YsSkipSpace(q + strlen(YS_KW_KILL));
		int nCount = atoi(q);
		while (*q >= '0' && *q <= '9') q++;
		const char* pAt = YsFindFold(q, YS_KW_AT);
		if (nCount > 0 && pAt)
		{
			const char* pMap = pAt + strlen(YS_KW_AT);
			const char* pMapEnd = pMap;
			while (*pMapEnd && *pMapEnd != ',' && *pMapEnd != '(' && *pMapEnd != '[') pMapEnd++;
			m_nYesouCount = nCount;
			YsCopyTrim(m_szYesouTarget, sizeof(m_szYesouTarget), q, pAt);
			YsCopyTrim(m_szYesouMap, sizeof(m_szYesouMap), pMap, pMapEnd);
			m_nYesouMapID = FindMapID(m_szYesouMap, NULL, 0, FALSE);
			m_nYesouKind = ys_attackNpc;
		}
	}
	else if ((q = YsFindFold(szQuest, YS_KW_FIND)) != NULL && (q = YsFindFold(q, YS_KW_INC)) != NULL)
	{
		// Tim Trang bi tang <thuoc tinh> lon hon N diem
		q += strlen(YS_KW_INC);
		const char* pGt = YsFindFold(q, YS_KW_GREATER);
		if (pGt)
		{
			YsCopyTrim(m_szYesouTarget, sizeof(m_szYesouTarget), q, pGt);
			m_nYesouCount = atoi(YsSkipSpace(pGt + strlen(YS_KW_GREATER)));
			m_nYesouKind = ys_ItemOption;
		}
	}

	char szLog[512];
	if (m_nYesouKind == ys_attackNpc)
		_snprintf(szLog, sizeof(szLog) - 1, TV(L"Dã Tẩu: %s => diet %d '%s' o '%s' (MapID %d)\n"),
			m_szYesouCurQuest, m_nYesouCount, m_szYesouTarget, m_szYesouMap, m_nYesouMapID);
	else if (m_nYesouKind == ys_ItemOption)
		_snprintf(szLog, sizeof(szLog) - 1, TV(L"Dã Tẩu: %s => tim trang bi '%s' > %d\n"),
			m_szYesouCurQuest, m_szYesouTarget, m_nYesouCount);
	else
		_snprintf(szLog, sizeof(szLog) - 1, TV(L"Dã Tẩu: %s => chua nhan dang duoc loai nhiem vu\n"), m_szYesouCurQuest);
	szLog[sizeof(szLog) - 1] = 0;
	QTRACE("GetYesouQuest: Kind=%d Count=%d Target='%s' Map='%s' (%d)", m_nYesouKind, m_nYesouCount, m_szYesouTarget, m_szYesouMap, m_nYesouMapID);
	SaveYSLog(szLog);
}


int CVLQuest::FindMapID(const char* szName, char* szFound, int nFound, BOOL bPrint)
{
	static char* s_pSection = NULL;
	if (!s_pSection)
	{
		char szFile[MAX_PATH];
		sprintf(szFile, "%s\\Maps\\MapNames.ini", theMain.m_szCurFolder);
		s_pSection = (char*)malloc(0x20000);
		if (!s_pSection) return 0;
		if (!GetPrivateProfileSectionA("Map_Name", s_pSection, 0x20000, szFile))
		{
			s_pSection[0] = s_pSection[1] = 0;
			if (bPrint) DbgOut("LOI: khong doc duoc [Map_Name] trong %s\n", szFile);
		}
	}
	if (szFound) szFound[0] = 0;
	if (!szName || !szName[0]) return 0;
	int nExact = 0, nPart = 0, nPrinted = 0;
	for (char* p = s_pSection; *p; p += strlen(p) + 1)
	{
		char* pEq = strchr(p, '=');
		if (!pEq) continue;
		int nID = atoi(p);
		BOOL bExact = EqualFold(pEq + 1, szName);
		BOOL bPart = !bExact && (FindFold(pEq + 1, szName) || FindFold(szName, pEq + 1));
		if (!bExact && !bPart) continue;
		if (bPrint && nPrinted < 12)
		{
			DbgOut("  %s ID %d = %s\n", bExact ? "TRUNG  " : "gan dung", nID, pEq + 1);
			nPrinted++;
		}
		if (bExact && !nExact)
		{
			nExact = nID;
			if (szFound) { strncpy(szFound, pEq + 1, nFound - 1); szFound[nFound - 1] = 0; }
		}
		else if (bPart && !nPart)
			nPart = nID;
	}
	return nExact;
}

void CVLQuest::DoQuestAttackNpc()
{
	
}


// Tim ma thuoc tinh theo ten trong loi thoai (vd "Néi Lùc" -> 89 manamax_v)
// nUnit: 0 = khong ro, 1 = (diem), 2 = (%)
static int YsFindMagicAttrib(const char* szName, int nUnit)
{
	int nLen = (int)strlen(szName);
	if (!nLen) return -1;
	int nBest = -1, nBestScore = 0;
	for (int nID = 0; nID < YS_MAX_MAGIC_ATTRIB; nID++)
	{
		const char* szDesc = MagicAttribDesc(nID);
		if (!szDesc[0]) continue;
		int i = 0;
		while (i < nLen && YsFold(szDesc[i]) == YsFold(szName[i])) i++;
		if (i < nLen) continue;								// mo ta phai bat dau bang ten thuoc tinh
		unsigned char c = (unsigned char)szDesc[nLen];
		if (c && c != ' ' && c != '(') continue;			// tranh "Néi lùc" khop "Néi lùcX..."

		int nScore = 1;
		if (MagicAttribTier(nID, 0x7FFF) > 0) nScore += 4;	// co trong magicattriblevel.txt = dong phep trang bi
		if (nUnit == 1 && strstr(szDesc, TV(L"(điểm)"))) nScore += 2;	// (diem)
		if (nUnit == 2 && strstr(szDesc, "(%)")) nScore += 2;
		if (strstr(szDesc, TV(L"(Dương)"))) nScore -= 3;	// bo thuoc tinh (Duong)
		if (!c || c == '(') nScore += 1;					// khop sat ten
		if (nScore > nBestScore) { nBestScore = nScore; nBest = nID; }
	}
	return nBest;
}
void CVLQuest::DoQuestItemOption()
{
	if (m_nDoQuestStep == 0)
	{
		if (m_nFindItem == act_cancel || m_nFindItem == act_useitem)
		{
			SaveYSLog(CANCEL_BY_USER);
			m_nYSDoing = ys_cancel;
			theMap.NearestDriver();
			return;
		}
		if (m_nFindItem == act_ignore)
		{
			m_nYSDoing = ys_suspending;
			return;
		}

		// don vi sau "lon hon N": diem / %
		int nUnit = 0;
		const char* pGt = YsFindFold(m_szYesouCurQuest, YS_KW_GREATER);
		if (pGt)
		{
			if (YsFindFold(pGt, TV(L"điểm"))) nUnit = 1;
			else if (strchr(pGt, '%')) nUnit = 2;
		}

		m_nRqGenre = -1; m_nRqDetail = -1; m_nRqParticular = -1;
		m_nRqLevel = -1; m_nRqSeries = -1; m_nRqQuantity = 1;
		m_nReqAttrib = YsFindMagicAttrib(m_szYesouTarget, nUnit);
		m_nMinValue = m_nYesouCount + 1;		// "lon hon N" => >= N+1
		m_nMaxValue = 0x7FFFFFFF;
		m_nQuestID = 2;							// CheckMyItem/SearchBuyItem/DoQuest0203: tim do, khong cat tra
		m_bReturnItem = 0;

		char szLog[256];
		if (m_nReqAttrib < 0)
		{
			_snprintf(szLog, sizeof(szLog) - 1, TV(L"Không nhận dạng được thuộc tính '%s' => Tạm ngưng!"), m_szYesouTarget);
			szLog[sizeof(szLog) - 1] = 0;
			SaveYSLog(szLog);
			SystemMessage(TV(L"<color=yellow>Không nhận dạng được yêu cầu Dã Tẩu!"));
			m_nYSDoing = ys_suspending;
			return;
		}
		_snprintf(szLog, sizeof(szLog) - 1, TV(L"Tìm trang bị : %s (%d %s) >= %d"), MagicAttribDesc(m_nReqAttrib),
			m_nReqAttrib, MagicAttribName(m_nReqAttrib), m_nMinValue);
		szLog[sizeof(szLog) - 1] = 0;
		SaveYSLog(szLog);
		QTRACE("DoQuestItemOption: Target='%s' Unit=%d => Attrib=%d (%s) Min=%d", m_szYesouTarget, nUnit,
			m_nReqAttrib, MagicAttribName(m_nReqAttrib), m_nMinValue);
		// m_nDoQuestStep van = 0 => DoQuest0203 reset bien static cua no roi sang step 1
	}

	DoQuest0203();
}