// UpdateGameData.cpp : so sanh 2 ban game, cap nhat dia chi trong cac file .h cua VLAuto
#include "stdafx.h"
#include "UpdateGameData.h"
#include "UpdateGameDataDlg.h"
#include "GameDiff.h"

BEGIN_MESSAGE_MAP(CUpdateGameDataApp, CWinApp)
END_MESSAGE_MAP()

CUpdateGameDataApp::CUpdateGameDataApp()
{
}

CUpdateGameDataApp theApp;

BOOL CUpdateGameDataApp::InitInstance()
{
	INITCOMMONCONTROLSEX icc = { sizeof(icc), ICC_WIN95_CLASSES | ICC_PROGRESS_CLASS | ICC_LISTVIEW_CLASSES };
	InitCommonControlsEx(&icc);
	CWinApp::InitInstance();

	if (__argc >= 2 && (_tcsicmp(__targv[1], _T("/cli")) == 0 || _tcsicmp(__targv[1], _T("-cli")) == 0))
	{
		RunCommandLine();
		return FALSE;
	}

	CUpdateGameDataDlg dlg;
	m_pMainWnd = &dlg;
	dlg.DoModal();
	return FALSE;
}

void CUpdateGameDataApp::RunCommandLine()
{
	CString strLog = CString(__argc >= 5 ? __targv[4] : _T(".")) + _T("\\UpdateGameData.log");
	if (__argc < 6)
	{
		AfxMessageBox(_T("UpdateGameData.exe /cli <bản cũ.exe> <bản mới.exe> <thư mục kết quả> <file.h> [file.h ...]"));
		return;
	}
	CString strOut = __targv[4];
	SHCreateDirectoryEx(NULL, strOut, NULL);
	std::vector<CString> files;
	for (int i = 5; i < __argc; i++) files.push_back(__targv[i]);

	CGameDiff diff;
	CString strErr, strResult;
	if (!diff.LoadImages(__targv[2], __targv[3], strErr) || !diff.ParseHeaders(files, strErr))
	{
		CStdioFile f;
		if (f.Open(strLog, CFile::modeCreate | CFile::modeWrite)) { f.WriteString(strErr); f.Close(); }
		return;
	}
	diff.Resolve(nullptr, nullptr);
	diff.WriteReport(strOut + _T("\\BaoCao.txt"), strErr);
	diff.WriteHeaders(strOut, FALSE, strResult);
}
