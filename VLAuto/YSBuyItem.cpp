// YeSouOptions.cpp : implementation file
//

#include "stdafx.h"
#include "VLAuto.h"
#include "../GameDef.h"
#include "AutoParams.h"
#include "YSBuyItem.h"

TCHAR* AllCityNames[] = { _T("Phượng Tường"), _T("Tương Dương"), _T("Biện Kinh"), _T("Thành Đô"), _T("Đại Lý"), _T("Dương Châu"), _T("Lâm An"), _T("") };
TCHAR* AllItemKinds[] = { _T("Nhẫn"), _T("Ngọc bội / Dây chuyền"), _T("Trang bị khác"), _T("Phúc Duyên (tiểu)"), _T("Phúc Duyên (trung)"), _T("Phúc Duyên (đại)"), _T("Sát thủ giản"), _T("") };

// CYSBuyItem dialog

IMPLEMENT_DYNAMIC(CYSBuyItem, CPropertyPage)

CYSBuyItem::CYSBuyItem()
	: CPropertyPage(CYSBuyItem::IDD)
{
	m_pBuyItemOpt = NULL;
}

CYSBuyItem::~CYSBuyItem()
{
}

void CYSBuyItem::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_CITY_LIST, m_CityList);
	DDX_Control(pDX, IDC_ITEMPRICE_LIST, m_ItemPriceList);

	if (!m_pBuyItemOpt) return;

	DDX_Check(pDX, IDC_NORETURN_CHECK, m_pBuyItemOpt->m_bNoReturnNoTake);
	DDX_Check(pDX, IDC_ALLREP_CHECK, m_pBuyItemOpt->m_bGetAllRep);
	DDX_Check(pDX, IDC_DONTBUY_CHECK, m_pBuyItemOpt->m_bDontSearchBuy);
	DDX_Check(pDX, IDC_NOTFOUND_CHECK, m_pBuyItemOpt->m_bNotFoundCancel);
	DDX_Check(pDX, IDC_ITEMFILTER_CHECK, m_pBuyItemOpt->m_bItemFilter);

	if (pDX->m_bSaveAndValidate)
	{
		for (int i = 0; i < MAX_CITY_NUM; i++)
		{
			int nCityID = CitiesID[i] << 16;
			m_pBuyItemOpt->m_VisitCities[i] = m_CityList.GetCheck(i)?nCityID+1:nCityID;
		}
		for (int i = 0; i < m_ItemPriceList.GetItemCount(); i++)
			m_pBuyItemOpt->m_MaxPrice[i] = _tstoi(m_ItemPriceList.GetItemText(i, 1));
	}
	else
	{
		TCHAR szValue[16];
		for (int i = 0; i < MAX_CITY_NUM; i++)
			m_CityList.SetCheck(i, (BOOL)(m_pBuyItemOpt->m_VisitCities[i] & 0xFFFF));
		for (int i = 0; i < m_ItemPriceList.GetItemCount(); i++)
		{
			_stprintf(szValue, _T("%d"), m_pBuyItemOpt->m_MaxPrice[i]);
			m_ItemPriceList.SetItemText(i, 1, szValue);
		}
	}
}

BEGIN_MESSAGE_MAP(CYSBuyItem, CPropertyPage)
END_MESSAGE_MAP()

// CYSBuyItem message handlers

BOOL CYSBuyItem::OnInitDialog()
{
	CPropertyPage::OnInitDialog();

	// Prepare CityList
	m_CityList.SetExtendedStyle(LVS_EX_CHECKBOXES|LVS_EX_GRIDLINES|LVS_EX_FULLROWSELECT|LVS_EX_ONECLICKACTIVATE);
	m_CityList.InsertColumn(0, _T("Thành thị"), LVCFMT_LEFT, 120);

	for (int i = 0; AllCityNames[i][0]; i++)
		m_CityList.InsertItem(i, AllCityNames[i]);

	// Prepare ItemList
	m_ItemPriceList.SetExtendedStyle(LVS_EX_GRIDLINES|LVS_EX_FULLROWSELECT|LVS_EX_ONECLICKACTIVATE);
	CImageList imgList;
	imgList.Create(1, 16, ILC_COLOR4, 1, 1);
	m_ItemPriceList.SetImageList(&imgList, LVSIL_SMALL);
	m_ItemPriceList.InsertColumn(0, _T("Trang bị / Vật phẩm"), LVCFMT_LEFT, 130);
	m_ItemPriceList.InsertColumn(1, _T("Giá (vạn)"), LVCFMT_CENTER, 60);

	for (int i = 0; AllItemKinds[i][0]; i++)
		m_ItemPriceList.InsertItem(i, AllItemKinds[i]);

	UpdateData(0);

	return TRUE;  // return TRUE unless you set the focus to a control
}

void CYSBuyItem::OnOK()
{
	UpdateData();
	CPropertyPage::OnOK();
}
