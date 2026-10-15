// YeSouOptions.cpp : implementation file
//

#include "stdafx.h"
#include "../GameDef.h"
#include "VLAuto.h"
#include "AutoParams.h"
#include "YSPrizeSel.h"

TCHAR* AllPrizeKinds[] = 
{
	_T("Hoàng Kim môn phái"),
	_T("Nhu Tình (áo)"),
	_T("Nhu Tình (ngọc bội)"),
	_T("Nhu Tình (khác)"),
	_T("Hiệp Cốt (ngọc bội)"),
	_T("Hiệp Cốt (khác)"),
	_T("An Bang (dây chuyền)"),
	_T("An Bang (khác)"),
	_T("Định Quốc (nón)"),
	_T("Định Quốc (khác)"),
	_T("Thủy Tinh / Tinh Hồng Bảo Thạch"),
	_T("Huyền tinh cấp xx (+1 cấp x3)"),
	_T("Võ Lâm Mật Tịch / Tẩy Tủy Kinh"),
	_T("Thiết La Hán / Bàn Nhược Tâm Kinh"),
	_T("Bí kíp môn phái"),
	_T("Tiên Thảo Lộ"),
	_T("Kinh nghiệm (1 triệu)"),
	_T("Phúc duyên tiểu (x2, x3)"),
	_T("Dấu hỏi (?)"),
	_T("Cơ hội hủy bỏ nhiệm vụ"),
	_T("Nguyên Khoáng"),
	_T("Nguyên Thạch"),
	_T("Thiên Sơn Bảo Lộ"),
	_T("Bách Quả Lộ"),
	_T("Dược hoàn"),
	_T("Pháo Hoa"),
	_T("Tâm Tâm phù"),
	_T("Chỉ nhận HT cấp xx >"),
	_T("Vật phẩm khác"),
	_T("")
};

// CYSPrizeSel dialog

IMPLEMENT_DYNAMIC(CYSPrizeSel, CPropertyPage)

CYSPrizeSel::CYSPrizeSel()
	: CPropertyPage(CYSPrizeSel::IDD)
{
	m_pPrizeSelOpt = NULL;
}

CYSPrizeSel::~CYSPrizeSel()
{
}

void CYSPrizeSel::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_PRIZE_LIST, m_PrizeList);

	if (!m_pPrizeSelOpt) return;

	int nCount = m_PrizeList.GetItemCount();
	if (pDX->m_bSaveAndValidate)
	{
		for (int i = 0; i < nCount; i++)
		{
			m_pPrizeSelOpt->m_PrizeVal[i] = _tstoi(m_PrizeList.GetItemText(i, 1));
			if (m_pPrizeSelOpt->m_PrizeVal[i] < 0)
			{
				m_pPrizeSelOpt->m_PrizeVal[i] = 0;
				m_PrizeList.SetItemText(i, 1, _T("0"));
			}
			else if (m_pPrizeSelOpt->m_PrizeVal[i] > 100000)
			{
				m_pPrizeSelOpt->m_PrizeVal[i] = 100000;
				m_PrizeList.SetItemText(i, 1, _T("100000"));
			}
		}
		if (m_pPrizeSelOpt->m_PrizeVal[prize_huyentinh] < 1)
		{
			m_pPrizeSelOpt->m_PrizeVal[prize_huyentinh] = 1;
			m_PrizeList.SetItemText(prize_huyentinh, 1, _T("1"));
		}
		if (m_pPrizeSelOpt->m_PrizeVal[prize_huyentinh] > 10)
		{
			m_pPrizeSelOpt->m_PrizeVal[prize_huyentinh] = 10;
			m_PrizeList.SetItemText(prize_huyentinh, 1, _T("10"));
		}
		if (m_pPrizeSelOpt->m_PrizeVal[prize_kinhnghiem] > 10000)
		{
			m_pPrizeSelOpt->m_PrizeVal[prize_kinhnghiem] = 10000;
			m_PrizeList.SetItemText(prize_kinhnghiem, 1, _T("10000"));
		}
		if (m_pPrizeSelOpt->m_PrizeVal[prize_phucduyen] < 1)
		{
			m_pPrizeSelOpt->m_PrizeVal[prize_phucduyen] = 1;
			m_PrizeList.SetItemText(prize_phucduyen, 1, _T("1"));
		}
		if (m_pPrizeSelOpt->m_PrizeVal[prize_levelht] > 8)
		{
			m_pPrizeSelOpt->m_PrizeVal[prize_levelht] = 8;
			m_PrizeList.SetItemText(prize_huyentinh, 1, _T("8"));
		}
	}
	else
	{
		TCHAR szValue[16];
		for (int i = 0; i < nCount; i++)
		{
			_stprintf(szValue, _T("%d"), m_pPrizeSelOpt->m_PrizeVal[i]);
			m_PrizeList.SetItemText(i, 1, szValue);
		}
	}
}

BEGIN_MESSAGE_MAP(CYSPrizeSel, CPropertyPage)
	ON_BN_CLICKED(IDC_RESET_BUTTON, &CYSPrizeSel::OnBnClickedResetButton)
	ON_BN_CLICKED(IDC_LOAD_BUTTON, &CYSPrizeSel::OnBnClickedLoadButton)
	ON_BN_CLICKED(IDC_SAVE_BUTTON, &CYSPrizeSel::OnBnClickedSaveButton)
END_MESSAGE_MAP()

// CYSPrizeSel message handlers

BOOL CYSPrizeSel::OnInitDialog()
{
	CPropertyPage::OnInitDialog();

	m_PrizeList.SetExtendedStyle(LVS_EX_GRIDLINES|LVS_EX_FULLROWSELECT|LVS_EX_ONECLICKACTIVATE);
	CImageList imgList;
	imgList.Create(1, 16, ILC_COLOR4, 1, 1);
	m_PrizeList.SetImageList(&imgList, LVSIL_SMALL);
	m_PrizeList.InsertColumn(0, _T("Phần thưởng nhiệm vụ"), LVCFMT_LEFT, 180);
	m_PrizeList.InsertColumn(1, _T("Giá (vạn)"), LVCFMT_RIGHT, 60);

	for (int i = 0; AllPrizeKinds[i][0]; i++)
	{
		m_PrizeList.InsertItem(i, AllPrizeKinds[i]);
		m_PrizeList.SetItemText(i, 1, _T("0"));
	}
	
	UpdateData(0);
	return TRUE;  // return TRUE unless you set the focus to a control
}

void CYSPrizeSel::OnOK()
{
	UpdateData();
	CPropertyPage::OnOK();
}

void CYSPrizeSel::OnBnClickedResetButton()
{
	m_pPrizeSelOpt->InitValue();
	UpdateData(0);
}

void CYSPrizeSel::OnBnClickedLoadButton()
{
	if (!m_pPrizeSelOpt) return;

	CFileDialog fDlg(TRUE, _T(".dtc"), 0,
		OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT, _T("Da Tau Configuration (*.dtc)|*.dtc|"), this);
	if (fDlg.DoModal() == IDCANCEL)
		return;
	CString szFileName = fDlg.GetPathName();
	CFile cfFile;
	if (!cfFile.Open(szFileName, CFile::modeRead | CFile::shareDenyWrite))
	{
		MessageBox(_T("Lỗi: không thể đọc tập tin!"), 0, MB_ICONEXCLAMATION);
		return;
	}
	char szVersion[12] = "";
	cfFile.Read(szVersion, sizeof(szVersion));
	if (strcmp(szVersion, _SZCURVERSION))
	{
		MessageBox(_T("Lỗi: phiên bản không tương thích!"), 0, MB_ICONEXCLAMATION);
		cfFile.Close();
		return;
	}
	cfFile.Read(m_pPrizeSelOpt, sizeof(YSPrizeSelOpt));
	cfFile.Close();
	// Update Data
	UpdateData(0);
}

void CYSPrizeSel::OnBnClickedSaveButton()
{
	if (!m_pPrizeSelOpt) return;

	CFileDialog fDlg(FALSE, _T(".dtc"), 0,
		OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT, _T("Da Tau Configuration (*.dtc)|*.dtc|"), this);
	if (fDlg.DoModal() == IDCANCEL)
		return;
	CString szFileName = fDlg.GetPathName();
	CFile cfFile;
	if (!cfFile.Open(szFileName, CFile::modeCreate|CFile::modeWrite))
	{
		MessageBox(_T("Lỗi: không thể ghi tập tin!"), 0, MB_ICONEXCLAMATION);
		return;
	}
	int nValue = 0;
	char szVersion[12] = "";
	strcpy(szVersion, _SZCURVERSION);
	cfFile.Write(szVersion, sizeof(szVersion));
	for (int i = 0; i < m_PrizeList.GetItemCount(); i++)
	{
		nValue = _tstoi(m_PrizeList.GetItemText(i, 1));
		cfFile.Write(&nValue, sizeof(nValue));
	}
	cfFile.Close();
}
