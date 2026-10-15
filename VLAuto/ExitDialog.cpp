// ExitDialog.cpp : implementation file
//

#include "stdafx.h"
#include "VLAuto.h"
#include "ExitDialog.h"

#define EXPIRY_TIME 20

// CExitDialog dialog

IMPLEMENT_DYNAMIC(CExitDialog, CDialog)
CExitDialog::CExitDialog(CWnd* pParent /*=NULL*/)
	: CDialog(CExitDialog::IDD, pParent)
{
}

CExitDialog::~CExitDialog()
{
}

void CExitDialog::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CExitDialog, CDialog)
	ON_WM_TIMER()
END_MESSAGE_MAP()

BOOL CExitDialog::OnInitDialog(void)
{
	CDialog::OnInitDialog();
	CProgressCtrl* pPrCtrl = (CProgressCtrl*)GetDlgItem(IDC_TIME_PROGRESS);
	pPrCtrl->SetRange(0, EXPIRY_TIME);
	SetTimer (20, 1000, NULL);
	return TRUE;
}

void CExitDialog::OnTimer(UINT nIDEvent)
{
	static int i=0;
	CProgressCtrl* pPrCtrl = (CProgressCtrl*)GetDlgItem(IDC_TIME_PROGRESS);
	pPrCtrl->OffsetPos(1);
	if (i++>=EXPIRY_TIME) {
		KillTimer(20);
		OnOK();
	}
	CDialog::OnTimer(nIDEvent);
}
