// ExitDialog.cpp : implementation file
//

#include "stdafx.h"
#include "VLAuto.h"
#include "DlgExit.h"

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
	ON_BN_CLICKED(IDOK, &CExitDialog::OnBnClickedOk)
	ON_BN_CLICKED(IDCANCEL, &CExitDialog::OnBnClickedCancel)
END_MESSAGE_MAP()

BOOL CExitDialog::OnInitDialog(void)
{
	CDialog::OnInitDialog();
	CProgressCtrl* pPrCtrl = (CProgressCtrl*)GetDlgItem(IDC_TIME_PROGRESS);
	pPrCtrl->SetRange(0, EXPIRY_TIME);
	SetTimer(20, 1000, NULL);
	return TRUE;
}

void CExitDialog::OnTimer(UINT nIDEvent)
{
	CProgressCtrl* pPrCtrl = (CProgressCtrl*)GetDlgItem(IDC_TIME_PROGRESS);
	if (pPrCtrl->GetPos() >= EXPIRY_TIME)
		OnOK();
	else
		pPrCtrl->OffsetPos(1);
	CDialog::OnTimer(nIDEvent);
}

void CExitDialog::OnBnClickedOk()
{
	KillTimer(20);
	OnOK();
}

void CExitDialog::OnBnClickedCancel()
{
	KillTimer(20);
	OnCancel();
}
