// PKQuestFilter.cpp : implementation file
//

#include "stdafx.h"
#include "VLAuto.h"
#include "DlgPKQFilter.h"


// CPKQuestFilter dialog

IMPLEMENT_DYNAMIC(CPKQuestFilter, CDialog)

CPKQuestFilter::CPKQuestFilter(CWnd* pParent /*=NULL*/)
	: CDialog(CPKQuestFilter::IDD, pParent)
	, m_nQuestKind(0)
	, m_nLevel(0)
	, m_nLetters(0)
{

}

CPKQuestFilter::~CPKQuestFilter()
{
}

void CPKQuestFilter::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_CBIndex(pDX, IDC_WORKSHOP_COMBO, m_nQuestKind);
	DDX_Text(pDX, IDC_LEVEL_EDIT, m_nLevel);
	DDV_MinMaxInt(pDX, m_nLevel, 1, 9);
	DDX_Text(pDX, IDC_LETTER_EDIT, m_nLetters);
	DDV_MinMaxInt(pDX, m_nLetters, 1, 9);
}


BEGIN_MESSAGE_MAP(CPKQuestFilter, CDialog)
END_MESSAGE_MAP()


// CPKQuestFilter message handlers

BOOL CPKQuestFilter::OnInitDialog()
{
	CDialog::OnInitDialog();

	CComboBox *pQuestList = (CComboBox *)GetDlgItem(IDC_WORKSHOP_COMBO);
	for (int i = 0; i < TONGPK_KIND; i++)
		pQuestList->AddString(TongPKKind[i]);

	pQuestList->SetCurSel(0);

	return TRUE;
}
