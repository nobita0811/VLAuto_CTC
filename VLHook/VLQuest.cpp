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
}

void CVLQuest::Breath()
{
	if (!m_bInfoLoaded) return;

	if (m_nGenDoing == qt_suspending)
	{
		DoSuspendOpt();
		return;
	}
	if (m_nGenDoing == qt_suspended) return;

	if (g_pCurPlayer->m_nEntrustMode) return;

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
		if (strstr(pMsgSel->m_InfoText, "LÇn nµy chóng ta ®· khiÕn h¾n t©m phôc"))
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
			SaveYSLog("Qu¸ thêi gian, vÒ thµnh hñy nhiÖm vô!\n");
			SystemMessage("<color=yellow>Qu¸ thêi gian, vÒ thµnh hñy nhiÖm vô!");
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
			SystemMessage("<color=yellow>VÒ thµnh tr¶ nhiÖm vô!");
			break;
		case ys_cancel:
			SystemMessage("<color=yellow>VÒ thµnh hñy nhiÖm vô!");
			theQuest.m_nYSDoing = ys_cancel;
			break;
		case ys_suspending:
			SystemMessage("<color=yellow>VÒ thµnh kiÓm tra nhiÖm vô!");
			break;
		}
		theRecover.OnUseAM();
	}
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
		fprintf(pLogFile, szLogInfo);
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
		fclose(pLogFile);
		if (fsize > 32000)
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
		fprintf(pLogFile, "====** Font TCVN3 **====\n%s - NhËt ký NhiÖm vô VLAuto\n\n", P_PLAYER->Name);
	if (bStartNewDate)
	{
		fprintf(pLogFile, "Ngµy %02d-%02d-%04d\n", lct.tm_mday, lct.tm_mon + 1, lct.tm_year + 1900);
		fprintf(pLogFile, "======================================================================\n");
	}
	fprintf(pLogFile, "\n[%02d-%02d %02d:%02d:%02d] ", lct.tm_mday, lct.tm_mon + 1, lct.tm_hour, lct.tm_min, lct.tm_sec);
	fprintf(pLogFile, "Khëi ®éng VLAuto...\n");
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
	if (m_bAutoDoQuest)
	{
		sprintf(szMsg, "<color=yellow>Tù lµm NhiÖm vô (Ctrl+D): <color=green>BËt");
		if (!LoadTaskInfo())
			sprintf(szMsg, "Lçi ®äc file D÷ liÖu, xin h·y t¶i l¹i phÇn mÒm!");
	}
	else
	{
		sprintf(szMsg, "<color=yellow>Tù lµm NhiÖm vô (Ctrl+D): <color>T¾t");
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

void CVLQuest::DoSuspendOpt()
{
	switch (m_nPostAction)
	{
	case 0: // Dung im
		SaveYSLog("Ng­ng lµm NhiÖm vô, ®øng im.\n");
		SystemMessage("<color=yellow>Ng­ng lµm NhiÖm vô, ®øng im.");
		break;
	case 1: // Thoat game
		theMain.ExitGame();
		SaveYSLog("Ng­ng lµm NhiÖm vô, tho¸t Game.\n");
		break;
	case 2: // Bao chuong
		theMain.PostFeedBack(cmd_amulet);
		SaveYSLog("Ng­ng lµm NhiÖm vô, b¸o chu«ng.\n");
		SystemMessage("<color=yellow>Ng­ng lµm NhiÖm vô, b¸o chu«ng.");
		break;
	case 3: // Di luyen cong
		m_bNeedGoTrain = TRUE;
		m_bNeedGoPCK = 0;
		m_bGoTrainGetPK = 0;
		theMap.m_bFirstSaveRepos = TRUE;
		theMap.m_bNoDriverRecent = TRUE;
		if (!theMap.m_bAutoReturn)
			theMain.PostFeedBack(cmd_setmap);
		SaveYSLog("Ng­ng lµm NhiÖm vô, ®i luyÖn c«ng.\n");
		SystemMessage("<color=yellow>Ng­ng lµm NhiÖm vô, ®i luyÖn c«ng.");
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
