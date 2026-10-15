// CYSAddPoint.cpp : implementation file
//

#include "stdafx.h"
#include "../GameDef.h"
#include "DWinProcess.h"
#include "VLAuto.h"
#include "GameCore.h"
#include "GameWindow.h"
#include "DlgYeSou.h"
#include "YSAddPoint.h"

// CYSAddPoint dialog

IMPLEMENT_DYNAMIC(CYSAddPoint, CPropertyPage)

CYSAddPoint::CYSAddPoint()
	: CPropertyPage(CYSAddPoint::IDD)
{
	m_pAddPointOpt = NULL;
}

CYSAddPoint::~CYSAddPoint()
{
}

void CYSAddPoint::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_USEITEM_LIST, m_UseItemList);
	
	if (!m_pAddPointOpt) return;

	// Checkboxes
	DDX_Check(pDX, IDC_CANCELJY_CHECK, m_pAddPointOpt->m_bExpCancel);
	DDX_Check(pDX, IDC_BUYFY_CHECK, m_pAddPointOpt->m_bFYBuyItem);
	DDX_Check(pDX, IDC_CANCELMW_CHECK, m_pAddPointOpt->m_bMWCancel);
	DDX_Check(pDX, IDC_CANCELSJ_CHECK, m_pAddPointOpt->m_bSJCancel);
	DDX_Check(pDX, IDC_USETTL_CHECK, m_pAddPointOpt->m_bUseTienThaoLo);
	DDX_Check(pDX, IDC_PCKONLY_CHECK, m_pAddPointOpt->m_bPCKOnly);
	DDX_Check(pDX, IDC_PCKNOCLICK_CHECK, m_pAddPointOpt->m_bNoClickTS);
	// Textboxes
	DDX_Text(pDX, IDC_CANCELJY_EDIT, m_pAddPointOpt->m_nExpCancelVal);
	DDX_Text(pDX, IDC_FY01_EDIT, m_pAddPointOpt->m_nFYItem01);
	DDX_Text(pDX, IDC_FY02_EDIT, m_pAddPointOpt->m_nFYItem02);
	DDX_Text(pDX, IDC_FY03_EDIT, m_pAddPointOpt->m_nFYItem03);
	DDX_Text(pDX, IDC_CANCELSJ_EDIT, m_pAddPointOpt->m_nSJCancelVal);
	DDX_Text(pDX, IDC_CANCELMW_EDIT, m_pAddPointOpt->m_nMWCancelVal);

	TCHAR szItemName[40];
	if (pDX->m_bSaveAndValidate)
	{
		memset(m_pAddPointOpt->m_ExpItemName, 0, sizeof(m_pAddPointOpt->m_ExpItemName));
		for (int i = 0; i < m_UseItemList.GetItemCount(); i++)
		{
			m_UseItemList.GetItemText(i, 0, szItemName, 36);
			UnicodeToTCVN3(szItemName, m_pAddPointOpt->m_ExpItemName[i], 36);
		}
	}
	else
	{
		m_UseItemList.DeleteAllItems();
		for (int i = 0; i < MAX_USE_ITEM; i++)
		{
			if (!m_pAddPointOpt->m_ExpItemName[i][0]) break;
			TCVN3ToUnicode(m_pAddPointOpt->m_ExpItemName[i], szItemName, 36);
			m_UseItemList.InsertItem(i, szItemName);
		}
	}
}


BEGIN_MESSAGE_MAP(CYSAddPoint, CPropertyPage)
	ON_BN_CLICKED(IDC_ADD_BUTTON, &CYSAddPoint::OnBnClickedAddButton)
	ON_BN_CLICKED(IDC_DEL_BUTTON, &CYSAddPoint::OnBnClickedDelButton)
END_MESSAGE_MAP()


// CYSAddPoint message handlers

BOOL CYSAddPoint::OnInitDialog()
{
	CPropertyPage::OnInitDialog();

	// Prepare ItemList
	m_UseItemList.SetExtendedStyle(LVS_EX_GRIDLINES|LVS_EX_FULLROWSELECT|LVS_EX_ONECLICKACTIVATE);
	CImageList imgList;
	imgList.Create(1, 16, ILC_COLOR4, 1, 1);
	m_UseItemList.SetImageList(&imgList, LVSIL_SMALL);
	m_UseItemList.InsertColumn(0, _T("Vật phẩm tăng Kinh nghiệm"), LVCFMT_LEFT, 180);
	UpdateItemSelCombo();

	UpdateData(0);
	return TRUE;  // return TRUE unless you set the focus to a control
}

void CYSAddPoint::OnOK()
{
	UpdateData();
	CPropertyPage::OnOK();
}

void CYSAddPoint::UpdateItemSelCombo()
{
	CYeSouOptions* theYSOpt = (CYeSouOptions*)GetParent();

	LPBYTE lpBaseAdd, lpCurAdd;
	DWinProcess winProcess(theYSOpt->m_hGameWnd);
	if (!winProcess.ReadMemory((LPVOID)ITM_BASE_ADD, (LPVOID)&lpBaseAdd, 4))
		return;

	CComboBox *pItemCombo = (CComboBox *)GetDlgItem(IDC_USEITEM_COMBO);
	TCHAR szItemName[40];
	KItem Item;
	for (int i = 1; i < MAX_ITEM; i++)
	{
		lpCurAdd = lpBaseAdd + i*ITM_DATA_SIZE;
		winProcess.ReadMemory(lpCurAdd, (LPVOID)&Item, sizeof(Item));
		if (!Item.nObjIdx) continue;
		if (Item.nItemGenre != item_special) continue;
		TCVN3ToUnicode(Item.szItemName, szItemName, 36);
		if (pItemCombo->FindString(0, szItemName) == CB_ERR)
			pItemCombo->AddString(szItemName);
	}
}

void CYSAddPoint::OnBnClickedAddButton()
{
	int nCount = m_UseItemList.GetItemCount();
	if (nCount >= MAX_USE_ITEM)
	{
		MessageBox(_T("Chỉ được đặt dùng tối đa 8 vật phẩm!"), 0, MB_ICONEXCLAMATION);
		return;
	}
	CComboBox *pItemCombo = (CComboBox *)GetDlgItem(IDC_USEITEM_COMBO);
	CString szItemName;
	pItemCombo->GetWindowText(szItemName);

	LVFINDINFOW findItem;
	findItem.flags = LVFI_STRING;
	findItem.psz = szItemName;
	if (m_UseItemList.FindItem(&findItem) >= 0) return;
	m_UseItemList.InsertItem(nCount, szItemName);
}

void CYSAddPoint::OnBnClickedDelButton()
{
	for (int i = 0; i < m_UseItemList.GetItemCount(); i++)
		if (m_UseItemList.GetItemState(i, LVIS_SELECTED) == LVIS_SELECTED)
		{
			m_UseItemList.DeleteItem(i);
			break;
		}
}
