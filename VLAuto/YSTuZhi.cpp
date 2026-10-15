// YSTuZhi.cpp : implementation file
//

#include "stdafx.h"
#include "VLAuto.h"
#include "AutoParams.h"
#include "YSTuZhi.h"

TCHAR* All9xMaps[] = { _T("Cán Viên động"), _T("Tiến Cúc động"), _T("Khỏa Lang động"), _T("Dược Vương động 4"), _T("Trường Bạch Sơn nam"), _T("Trường Bạch Sơn bắc"), _T("Sa Mạc Sơn động 1"), _T("Sa Mạc Sơn động 2"), _T("Sa Mạc Sơn động 3"), _T("") };

// CYSTuZhi dialog

IMPLEMENT_DYNAMIC(CYSTuZhi, CPropertyPage)

CYSTuZhi::CYSTuZhi()
	: CPropertyPage(CYSTuZhi::IDD)
{
	m_pTuZhiOpt = NULL;
}

CYSTuZhi::~CYSTuZhi()
{
}

void CYSTuZhi::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_TUZHI_LIST, m_TuZhiList);
	DDX_Control(pDX, IDC_MIZHI_LIST, m_MiZhiList);

	if (!m_pTuZhiOpt) return;
	DDX_Check(pDX, IDC_CANCELNOMAP_CHECK, m_pTuZhiOpt->m_bCancelNoMap);
	DDX_Check(pDX, IDC_CANCELLONG_CHECK, m_pTuZhiOpt->m_bCancelTooLong);
	DDX_Text(pDX, IDC_TUZHITIME_EDIT, m_pTuZhiOpt->m_nTuZhiTime);

	if (pDX->m_bSaveAndValidate)
	{
		for (int i = 0; i < m_TuZhiList.GetItemCount(); i++)
			m_pTuZhiOpt->m_TuZhiMapPass[i] = m_TuZhiList.GetCheck(i);
		for (int i = 0; i < m_MiZhiList.GetItemCount(); i++)
			m_pTuZhiOpt->m_MiZhiMapPass[i] = m_MiZhiList.GetCheck(i);
	}
	else
	{
		for (int i = 0; i < m_TuZhiList.GetItemCount(); i++)
			m_TuZhiList.SetCheck(i, m_pTuZhiOpt->m_TuZhiMapPass[i]);
		for (int i = 0; i < m_MiZhiList.GetItemCount(); i++)
			m_MiZhiList.SetCheck(i, m_pTuZhiOpt->m_MiZhiMapPass[i]);
	}
}


BEGIN_MESSAGE_MAP(CYSTuZhi, CPropertyPage)
END_MESSAGE_MAP()


// CYSTuZhi message handlers

BOOL CYSTuZhi::OnInitDialog()
{
	CPropertyPage::OnInitDialog();

	m_TuZhiList.SetExtendedStyle(LVS_EX_CHECKBOXES|LVS_EX_GRIDLINES|LVS_EX_FULLROWSELECT|LVS_EX_ONECLICKACTIVATE);
	m_TuZhiList.InsertColumn(0, _T("Đồ chí - Bỏ qua bản đồ"), LVCFMT_LEFT, 150);
	for (int i = 0; All9xMaps[i][0]; i++)
		m_TuZhiList.InsertItem(i, All9xMaps[i]);

	m_MiZhiList.SetExtendedStyle(LVS_EX_CHECKBOXES|LVS_EX_GRIDLINES|LVS_EX_FULLROWSELECT|LVS_EX_ONECLICKACTIVATE);
	m_MiZhiList.InsertColumn(0, _T("Mật chí - Bỏ qua bản đồ"), LVCFMT_LEFT, 150);
	for (int i = 0; All9xMaps[i][0]; i++)
		m_MiZhiList.InsertItem(i, All9xMaps[i]);

	UpdateData(0);

	return TRUE;  // return TRUE unless you set the focus to a control
}

void CYSTuZhi::OnOK()
{
	UpdateData();
	CPropertyPage::OnOK();
}
