// DlgMap.cpp : implementation file
//

#include "stdafx.h"
#include "VLAuto.h"
#include "GameWindow.h"
#include "DlgCollect.h"
#include "DlgMap.h"
#include "DlgMovePoints.h"
#include "DlgBuySetting.h"

// CDlgMap dialog

IMPLEMENT_DYNAMIC(CDlgMap, CDialog)

CDlgMap::CDlgMap(CWnd* pParent /*=NULL*/)
	: CDialog(CDlgMap::IDD, pParent)
{
}

CDlgMap::~CDlgMap()
{
}

void CDlgMap::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);

	if (g_pCurGameWin == NULL) return;

	//Checkboxes
	DDX_Check(pDX, IDC_AUTORETURN_CHECK, g_pCurGameWin->m_prMap.m_bAutoReturn);
	DDX_Check(pDX, IDC_REPAIRITEM_CHECK, g_pCurGameWin->m_prMap.m_bRepair);
	DDX_Check(pDX, IDC_SELLITEM_CHECK, g_pCurGameWin->m_prMap.m_bSellItems);
	DDX_Check(pDX, IDC_GETREPUTATION_CHECK, g_pCurGameWin->m_prMap.m_bGetReputation);
	DDX_Check(pDX, IDC_SAVEMONEY_CHECK, g_pCurGameWin->m_prMap.m_bPutMoney);
	DDX_Check(pDX, IDC_DRIVER_CHECK, g_pCurGameWin->m_prMap.m_bGetDriver);
	DDX_Check(pDX, IDC_GETMONEY_CHECK, g_pCurGameWin->m_prMap.m_bWithDraw);
	DDX_Check(pDX, IDC_GIVEKCL_CHECK, g_pCurGameWin->m_prMap.m_bGiveKCL);
	DDX_Check(pDX, IDC_DRIVER_CHECK2, g_pCurGameWin->m_prMap.m_bDriverOnly);
	DDX_Check(pDX, IDC_SAVE_RUONG_CHECK, g_pCurGameWin->m_prMap.m_bCustomBox);

	//Textboxes
	DDX_Text(pDX, IDC_SAVEMONEY_EDIT, g_pCurGameWin->m_prMap.m_nMoneyAmount);
	DDX_Text(pDX, IDC_REPOSITORYPASS_EDIT, g_pCurGameWin->m_prMap.m_nRepositoryPass);
	DDX_Text(pDX, IDC_GETMONEY_EDIT, g_pCurGameWin->m_prMap.m_nGetMoneyAmount);
	DDX_Text(pDX, IDC_GIVEKCL_EDIT, g_pCurGameWin->m_prMap.m_nMinKCL);

	//ComboBoxes
	DDX_CBIndex(pDX, IDC_DRIVER_COMBO, g_pCurGameWin->m_prMap.m_nDriverOption);
	DDX_CBIndex(pDX, IDC_SAVEITEM_COMBO, g_pCurGameWin->m_prMap.m_nSaveItemMode);
	UpdateTrainMap(pDX->m_bSaveAndValidate);
	UpdateCustomBox();

	//UI Refresh
	BOOL bChecked;
	bChecked = ((CButton*)GetDlgItem(IDC_GETMONEY_CHECK))->GetCheck();
	GetDlgItem(IDC_REPOSITORYPASS_EDIT)->EnableWindow(!bChecked);
	GetDlgItem(IDC_GETMONEY_EDIT)->EnableWindow(!bChecked);
	bChecked = ((CButton*)GetDlgItem(IDC_SAVEMONEY_CHECK))->GetCheck();
	GetDlgItem(IDC_SAVEMONEY_EDIT)->EnableWindow(!bChecked);
	bChecked = ((CButton*)GetDlgItem(IDC_DRIVER_CHECK))->GetCheck();
	GetDlgItem(IDC_DRIVER_COMBO)->EnableWindow(!bChecked);
	bChecked = ((CButton*)GetDlgItem(IDC_GIVEKCL_CHECK))->GetCheck();
	GetDlgItem(IDC_GIVEKCL_EDIT)->EnableWindow(!bChecked);
}

BEGIN_MESSAGE_MAP(CDlgMap, CDialog)
	ON_CONTROL_RANGE(BN_CLICKED, 1000, 2000, &CDlgMap::OnButtonsClicked)
	ON_CBN_SELCHANGE(IDC_DRIVER_COMBO, &CDlgMap::OnCbnSelchangeDriverCombo)
	ON_CBN_SELCHANGE(IDC_SAVEITEM_COMBO, &CDlgMap::OnCbnSelchangeSaveitemCombo)
END_MESSAGE_MAP()

// CDlgMap message handlers

BOOL CDlgMap::OnInitDialog()
{
	CDialog::OnInitDialog();

	CComboBox *pComboBox;
	pComboBox = (CComboBox*)GetDlgItem(IDC_DRIVER_COMBO);
	pComboBox->AddString(_T("Nơi đã đi qua 1"));
	pComboBox->AddString(_T("Nơi đã đi qua 2"));
	pComboBox->AddString(_T("Nơi đã đi qua 3"));
	pComboBox->AddString(_T("Quay lại địa điểm cũ"));
	pComboBox->AddString(_T("Làm nhiệm vụ Dã Tẩu"));
	pComboBox->AddString(_T("Dùng Thần Hành Phù"));
	pComboBox->AddString(_T("Đi bản đồ Mật Cốc"));
	pComboBox = (CComboBox*)GetDlgItem(IDC_SAVEITEM_COMBO);
	pComboBox->AddString(_T("Không cất đồ"));
	pComboBox->AddString(_T("Cất R1"));
	pComboBox->AddString(_T("Cất R1-2"));
	pComboBox->AddString(_T("Cất R1-2-3"));

	if (m_ToolTip.Create(this))
	{
		m_ToolTip.AddTool(GetDlgItem(IDC_AUTORETURN_CHECK), _T("Hồi thành tự quay lại nơi Luyện công (phím tắt Ctrl + T)"));
		m_ToolTip.AddTool(GetDlgItem(IDC_GETMONEY_CHECK), _T("Rút tiền trong Rương khi Hồi thành"));
		m_ToolTip.AddTool(GetDlgItem(IDC_GETMONEY_EDIT), _T("Số tiền cần rút (vạn lượng)"));
		m_ToolTip.AddTool(GetDlgItem(IDC_REPOSITORYPASS_EDIT), _T("Mật mã Rương (để rút tiền)"));
		m_ToolTip.AddTool(GetDlgItem(IDC_BUYSETTING_BUTTON), _T("Cài đặt mua thuốc, Thổ Địa Phù khi về thành"));
		m_ToolTip.AddTool(GetDlgItem(IDC_SELLITEM_CHECK), _T("Bán đồ (ở Tạp hóa / Hiệu thuốc)"));
		m_ToolTip.AddTool(GetDlgItem(IDC_REPAIRITEM_CHECK), _T("Sửa đồ (ở Hiệu thuốc)"));
		m_ToolTip.AddTool(GetDlgItem(IDC_GETREPUTATION_CHECK), _T("Nhận PD / DV ở Võ Lâm Truyền Nhân"));
		m_ToolTip.AddTool(GetDlgItem(IDC_SAVEMONEY_CHECK), _T("Gửi lại tiền vào Rương trước khi đi Luyện công"));
		m_ToolTip.AddTool(GetDlgItem(IDC_SAVEMONEY_EDIT), _T("Số tiền mang theo đi Luyện công (lượng)"));
		m_ToolTip.AddTool(GetDlgItem(IDC_SAVEITEM_COMBO), _T("Cất đồ vào Rương 1-2-3"));
		m_ToolTip.AddTool(GetDlgItem(IDC_DRIVER_COMBO), _T("Chế độ đi Xa phu"));
		m_ToolTip.AddTool(GetDlgItem(IDC_GIVEKCL_CHECK), _T("Tự nộp KCL mỗi khi về thành"));
		m_ToolTip.AddTool(GetDlgItem(IDC_GIVEKCL_EDIT), _T("Số lượng KCL tối thiểu để về giao nộp"));
		m_ToolTip.AddTool(GetDlgItem(IDC_SAVE_RUONG_CHECK), _T("Về thành cất đồ ở Rương tại vị trí đã lấy (thay Rương mặc định của thành)"));
		m_ToolTip.AddTool(GetDlgItem(IDC_LUU_RUONG_BUTTON), _T("Lấy vị trí hiện tại của nhân vật (đứng cạnh Rương) làm nơi cất đồ"));
	}

	return FALSE;
}

BOOL CDlgMap::PreTranslateMessage(MSG* pMsg)
{
	m_ToolTip.RelayEvent(pMsg);
	return CDialog::PreTranslateMessage(pMsg);
}

// Da bo "Noi luyen cong": map train lay tu map ID cua danh sach toa do (Tab Di chuyen)
void CDlgMap::UpdateTrainMap(BOOL bSave)
{
	if (g_pCurGameWin == NULL) return;
	g_pCurGameWin->m_prMap.m_nTrainMapID = CDlgMovePoints::GetTrainMapID();
}

// Ruong tuy chon: hien "Ten map (id) - [x, y]" (toa do hien thi tren ban do game)
void CDlgMap::UpdateCustomBox()
{
	if (g_pCurGameWin == NULL) return;
	PARAM_MAP &pm = g_pCurGameWin->m_prMap;
	CString szText;
	if (pm.m_nBoxMapID && (pm.m_nBoxX || pm.m_nBoxY))
		szText.Format(_T("%s - [%d, %d]"), (LPCTSTR)CDlgMovePoints::GetMapName(pm.m_nBoxMapID), pm.m_nBoxX >> 8, pm.m_nBoxY >> 9);
	SetDlgItemText(IDC_LUU_RUONG_EDIT, szText);
}

void CDlgMap::OnButtonsClicked(UINT nID)
{
	if (g_pCurGameWin == NULL) return;

	CString szMessage;
	CButton *pButton = (CButton*)GetDlgItem(nID);

	if (nID == IDC_BUYSETTING_BUTTON)
	{
		UpdateData();
		CDlgBuySetting dlgSetting(this);
		dlgSetting.DoModal();
	}
	else if (nID == IDC_LUU_RUONG_BUTTON)
	{
		if (!g_pCurGameWin->m_nMapID || (!g_pCurGameWin->m_nOffX && !g_pCurGameWin->m_nOffY))
		{
			MessageBox(_T("Chưa đọc được vị trí nhân vật!"), _T("VLAuto"), MB_ICONEXCLAMATION);
			return;
		}
		g_pCurGameWin->m_prMap.m_nBoxMapID = g_pCurGameWin->m_nMapID;
		g_pCurGameWin->m_prMap.m_nBoxX = g_pCurGameWin->m_nOffX;
		g_pCurGameWin->m_prMap.m_nBoxY = g_pCurGameWin->m_nOffY;
	}
	else if (nID == IDC_GETMONEY_CHECK)
	{
		if (pButton->GetCheck())
		{
			szMessage = _T("Việc lưu Pass rương vào chương trình có thể gây nguy hiểm!\n");
			szMessage += _T("Nếu máy có nhiều người sử dụng, bạn không nên dùng chức năng này.\n");
			szMessage += _T("Chỉ nên lưu Pass rương nếu bạn đang dùng máy ở nhà!");
			//MessageBox(szMessage, _T("VLAuto - Chú ý"), MB_ICONEXCLAMATION);
		}
		else
		{
			SetDlgItemInt(IDC_REPOSITORYPASS_EDIT, 0);
			GetDlgItem(IDC_REPOSITORYPASS_EDIT)->SetFocus();
		}
	}
	else if (nID == IDC_SELLITEM_CHECK)
	{
		if (pButton->GetCheck())
		{
			if (g_pCurGameWin->m_prCollect.m_arrMagic[0].nAttribType == 0)
			{
				szMessage = _T("Bạn chưa thiết lập thuộc tính lọc Trang bị/ Vũ khí.\n");
				szMessage += _T("Nhân vật sẽ bán những trang bị không thỏa mãn điều kiện.\n\n");
				szMessage += _T("Bạn đã đọc kỹ Hướng dẫn cho mục này chưa?");
			}
			else
			{
				szMessage = _T("Nhân vật sẽ tự bán trang bị vũ khí theo thuộc tính thiết lập tại phần Nhặt đồ.\n\n");
				//szMessage += _T("Bạn đã đọc kỹ Hướng dẫn cho mục này chưa?");
			}
			int nRet = MessageBox(szMessage, _T("VLCTCPro - Xác nhận"), MB_YESNO|MB_ICONQUESTION);
			if (nRet != IDYES) pButton->SetCheck(0);
		}
	}
	UpdateData();
	g_pCurGameWin->SetGameParams(tab_map);
}

void CDlgMap::OnCbnSelchangeDriverCombo()
{
	CComboBox *pDriverCombo = (CComboBox *)GetDlgItem(IDC_DRIVER_COMBO);
	if (pDriverCombo->GetCurSel() == 3)
	{
		CString szMesage = _T("Chức năng này chỉ dùng để quay lại nếu chưa bị mất điểm nhớ\n");
		szMesage += _T("Trong trường hợp không thể quay lại, nhân vật sẽ thoát Game\n");
		szMesage += _T("Xin hãy dùng các chế độ quay lại [Nơi đã đi qua 1-2-3] để thay thế!");
		//MessageBox(szMesage, _T("VLAuto"), MB_ICONEXCLAMATION);
	}
}

void CDlgMap::OnCbnSelchangeSaveitemCombo()
{
	UpdateData();
	g_pCurGameWin->SetGameParams(tab_map);
}
