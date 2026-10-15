// YSTongQ.cpp : implementation file
//

#include "stdafx.h"
#include "VLAuto.h"
#include "AutoParams.h"
#include "YSTongQ.h"

// CYSTongQ dialog

IMPLEMENT_DYNAMIC(CYSTongQ, CPropertyPage)

CYSTongQ::CYSTongQ()
	: CPropertyPage(CYSTongQ::IDD)
{
	m_pTongQOpt = NULL;
}

CYSTongQ::~CYSTongQ()
{
}

void CYSTongQ::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_TUZHI_LIST, m_CollQList);
	DDX_Control(pDX, IDC_MIZHI_LIST, m_KillQList);

	if (!m_pTongQOpt) return;

	DDX_Check(pDX, IDC_LBMIN_CHECK, m_pTongQOpt->m_bGetLBMin);
	DDX_Text(pDX, IDC_LBMIN_EDIT, m_pTongQOpt->m_nGetLBMin);
	if (m_pTongQOpt->m_bGetLBMin)
		DDV_MinMaxInt(pDX, m_pTongQOpt->m_nGetLBMin, 1, 60);

	DDX_Check(pDX, IDC_LBMAX_CHECK, m_pTongQOpt->m_bGetLBMax);
	DDX_Text(pDX, IDC_LBMAX_EDIT, m_pTongQOpt->m_nGetLBMax);
	if (m_pTongQOpt->m_bGetLBMax)
		DDV_MinMaxInt(pDX, m_pTongQOpt->m_nGetLBMax, m_pTongQOpt->m_nGetLBMin, 60);

	DDX_Check(pDX, IDC_KILLDT_CHECK, m_pTongQOpt->m_bKillDT);
	DDX_Check(pDX, IDC_BUYTBTN_CHECK, m_pTongQOpt->m_bBuyTBTN);
	DDX_Text(pDX, IDC_BUYTBTN_EDIT, m_pTongQOpt->m_nBuyTBTN);

	if (pDX->m_bSaveAndValidate)
	{
		for (int i = 0; i < m_CollQList.GetItemCount(); i++)
			m_pTongQOpt->m_CollQPass[i] = m_CollQList.GetCheck(i);
		for (int i = 0; i < m_KillQList.GetItemCount(); i++)
			m_pTongQOpt->m_KillQPass[i] = m_KillQList.GetCheck(i);
	}
	else
	{
		for (int i = 0; i < m_CollQList.GetItemCount(); i++)
			m_CollQList.SetCheck(i, m_pTongQOpt->m_CollQPass[i]);
		for (int i = 0; i < m_KillQList.GetItemCount(); i++)
			m_KillQList.SetCheck(i, m_pTongQOpt->m_KillQPass[i]);
	}
}


BEGIN_MESSAGE_MAP(CYSTongQ, CPropertyPage)
END_MESSAGE_MAP()


// CYSTongQ message handlers

BOOL CYSTongQ::OnInitDialog()
{
	CPropertyPage::OnInitDialog();
	m_CollQList.SetExtendedStyle(LVS_EX_CHECKBOXES|LVS_EX_GRIDLINES|LVS_EX_FULLROWSELECT|LVS_EX_ONECLICKACTIVATE);
	m_CollQList.InsertColumn(0, _T("Vật phẩm - Hủy nhiệm vụ"), LVCFMT_LEFT, 140);
	int i = 0;
	m_CollQList.InsertItem(i++, _T("Trường mệnh hoàn"));
	m_CollQList.InsertItem(i++, _T("Gia bào hoàn"));
	m_CollQList.InsertItem(i++, _T("Cao thiểm hoàn"));
	m_CollQList.InsertItem(i++, _T("Cao trung hoàn"));
	m_CollQList.InsertItem(i++, _T("Băng phòng hoàn"));
	m_CollQList.InsertItem(i++, _T("Lôi phòng hoàn"));
	m_CollQList.InsertItem(i++, _T("Hỏa phòng hoàn"));
	m_CollQList.InsertItem(i++, _T("Độc phòng hoàn"));
	m_CollQList.InsertItem(i++, _T("Nguyên khoáng"));
	m_CollQList.InsertItem(i++, _T("Nguyên thạch"));
	m_CollQList.InsertItem(i++, _T("Sát thủ giản"));
	m_CollQList.InsertItem(i++, _T("Pháo Hoa"));
	m_CollQList.InsertItem(i++, _T("Tâm Tâm Tương Ánh"));
	m_CollQList.InsertItem(i++, _T("Thiên Sơn Bảo Lộ"));
	m_CollQList.InsertItem(i++, _T("Bách Quả Lộ"));
	m_CollQList.InsertItem(i++, _T("Phúc Duyên Tiểu"));
	m_CollQList.InsertItem(i++, _T("Phúc Duyên Đại"));

	m_KillQList.SetExtendedStyle(LVS_EX_CHECKBOXES|LVS_EX_GRIDLINES|LVS_EX_FULLROWSELECT|LVS_EX_ONECLICKACTIVATE);
	m_KillQList.InsertColumn(0, _T("Diệt quái - Hủy nhiệm vụ"), LVCFMT_LEFT, 140);
	int j = 0;
	m_KillQList.InsertItem(j++, _T("Mạc Cao Quật"));
	m_KillQList.InsertItem(j++, _T("Trường Bạch Sơn bắc"));
	m_KillQList.InsertItem(j++, _T("Trường Bạch Sơn nam"));
	m_KillQList.InsertItem(j++, _T("Sa Mạc Sơn động 3"));
	m_KillQList.InsertItem(j++, _T("Sa Mạc Sơn động 2"));
	m_KillQList.InsertItem(j++, _T("Dược Vương động 4"));
	m_KillQList.InsertItem(j++, _T("Khỏa Lang động"));
	m_KillQList.InsertItem(j++, _T("Tiến Cúc động"));
	m_KillQList.InsertItem(j++, _T("Cán Viên động"));

	SetDlgItemText(IDC_KILLDT_CHECK, _T("Tính năng chưa mở"));
	GetDlgItem(IDC_KILLDT_CHECK)->EnableWindow(0);

	UpdateData(0);
	return TRUE;  // return TRUE unless you set the focus to a control
}
