// UpdateGameData.h : main header file for the UpdateGameData application
#pragma once

#include "resource.h"

class CUpdateGameDataApp : public CWinApp
{
public:
	CUpdateGameDataApp();

	virtual BOOL InitInstance();

private:
	// UpdateGameData.exe /cli <ban cu.exe> <ban moi.exe> <thu muc ket qua> <file.h> [file.h ...]
	void RunCommandLine();

	DECLARE_MESSAGE_MAP()
};

extern CUpdateGameDataApp theApp;
