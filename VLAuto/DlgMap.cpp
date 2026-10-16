// DlgMap.cpp : implementation file
//

#include "stdafx.h"
#include "VLAuto.h"
#include "GameWindow.h"
#include "DlgCollect.h"
#include "DlgMap.h"

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
	DDX_Check(pDX, IDC_BUYITEM_CHECK1, g_pCurGameWin->m_prMap.m_TradeItem[0].bBuyCheck);
	DDX_Check(pDX, IDC_BUYITEM_CHECK2, g_pCurGameWin->m_prMap.m_TradeItem[1].bBuyCheck);
	DDX_Check(pDX, IDC_BUYITEM_CHECK3, g_pCurGameWin->m_prMap.m_TradeItem[2].bBuyCheck);
	DDX_Check(pDX, IDC_BUYAM_CHECK, g_pCurGameWin->m_prMap.m_bBuyTDP);
	DDX_Check(pDX, IDC_REPAIRITEM_CHECK, g_pCurGameWin->m_prMap.m_bRepair);
	DDX_Check(pDX, IDC_SELLITEM_CHECK, g_pCurGameWin->m_prMap.m_bSellItems);
	DDX_Check(pDX, IDC_GETREPUTATION_CHECK, g_pCurGameWin->m_prMap.m_bGetReputation);
	DDX_Check(pDX, IDC_SAVEMONEY_CHECK, g_pCurGameWin->m_prMap.m_bPutMoney);
	DDX_Check(pDX, IDC_DRIVER_CHECK, g_pCurGameWin->m_prMap.m_bGetDriver);
	DDX_Check(pDX, IDC_GETMONEY_CHECK, g_pCurGameWin->m_prMap.m_bWithDraw);
	DDX_Check(pDX, IDC_GIVEKCL_CHECK, g_pCurGameWin->m_prMap.m_bGiveKCL);

	//Textboxes
	DDX_Text(pDX, IDC_BUYITEM_EDIT1, g_pCurGameWin->m_prMap.m_TradeItem[0].nBuyAmout);
	DDX_Text(pDX, IDC_BUYITEM_EDIT2, g_pCurGameWin->m_prMap.m_TradeItem[1].nBuyAmout);
	DDX_Text(pDX, IDC_BUYITEM_EDIT3, g_pCurGameWin->m_prMap.m_TradeItem[2].nBuyAmout);
	DDX_Text(pDX, IDC_SAVEMONEY_EDIT, g_pCurGameWin->m_prMap.m_nMoneyAmount);
	DDX_Text(pDX, IDC_REPOSITORYPASS_EDIT, g_pCurGameWin->m_prMap.m_nRepositoryPass);
	DDX_Text(pDX, IDC_GETMONEY_EDIT, g_pCurGameWin->m_prMap.m_nGetMoneyAmount);
	DDX_Text(pDX, IDC_BUYAM_EDIT, g_pCurGameWin->m_prMap.m_nAMNumbers);
	DDX_Text(pDX, IDC_GIVEKCL_EDIT, g_pCurGameWin->m_prMap.m_nMinKCL);

	//ComboBoxes
	DDX_CBIndex(pDX, IDC_BUYITEM_COMBO1, g_pCurGameWin->m_prMap.m_TradeItem[0].nItemPos);
	DDX_CBIndex(pDX, IDC_BUYITEM_COMBO2, g_pCurGameWin->m_prMap.m_TradeItem[1].nItemPos);
	DDX_CBIndex(pDX, IDC_BUYITEM_COMBO3, g_pCurGameWin->m_prMap.m_TradeItem[2].nItemPos);
	DDX_CBIndex(pDX, IDC_DRIVER_COMBO, g_pCurGameWin->m_prMap.m_nDriverOption);
	DDX_CBIndex(pDX, IDC_SAVEITEM_COMBO, g_pCurGameWin->m_prMap.m_nSaveItemMode);
	UpdateTrainMap(pDX->m_bSaveAndValidate);

	//UI Refresh
	BOOL bChecked;
	bChecked = ((CButton*)GetDlgItem(IDC_GETMONEY_CHECK))->GetCheck();
	GetDlgItem(IDC_REPOSITORYPASS_EDIT)->EnableWindow(!bChecked);
	GetDlgItem(IDC_GETMONEY_EDIT)->EnableWindow(!bChecked);
	bChecked = ((CButton*)GetDlgItem(IDC_BUYITEM_CHECK1))->GetCheck();
	GetDlgItem(IDC_BUYITEM_COMBO1)->EnableWindow(!bChecked);
	GetDlgItem(IDC_BUYITEM_EDIT1)->EnableWindow(!bChecked);
	bChecked = ((CButton*)GetDlgItem(IDC_BUYITEM_CHECK2))->GetCheck();
	GetDlgItem(IDC_BUYITEM_COMBO2)->EnableWindow(!bChecked);
	GetDlgItem(IDC_BUYITEM_EDIT2)->EnableWindow(!bChecked);
	bChecked = ((CButton*)GetDlgItem(IDC_BUYITEM_CHECK3))->GetCheck();
	GetDlgItem(IDC_BUYITEM_COMBO3)->EnableWindow(!bChecked);
	GetDlgItem(IDC_BUYITEM_EDIT3)->EnableWindow(!bChecked);
	bChecked = ((CButton*)GetDlgItem(IDC_BUYAM_CHECK))->GetCheck();
	GetDlgItem(IDC_BUYAM_EDIT)->EnableWindow(!bChecked);
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
	ON_CBN_SELCHANGE(IDC_TRAINMAP_COMBO, &CDlgMap::OnCbnSelchangeTrainmapCombo)
	ON_CBN_SELCHANGE(IDC_SAVEITEM_COMBO, &CDlgMap::OnCbnSelchangeSaveitemCombo)
END_MESSAGE_MAP()

// CDlgMap message handlers

BOOL CDlgMap::OnInitDialog()
{
	CDialog::OnInitDialog();

	CComboBox *pComboBox;
	pComboBox = (CComboBox*)GetDlgItem(IDC_BUYITEM_COMBO1);
	InitComboBox(pComboBox);
	pComboBox = (CComboBox*)GetDlgItem(IDC_BUYITEM_COMBO2);
	InitComboBox(pComboBox);
	pComboBox = (CComboBox*)GetDlgItem(IDC_BUYITEM_COMBO3);
	InitComboBox(pComboBox);
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

	TCHAR szMapIniFile[MAX_PATH];
	_stprintf(szMapIniFile, _T("%s\\Maps\\MapMenu.ini"), g_szAppFolder);
	TCHAR szMapNameFile[MAX_PATH];
	_stprintf(szMapNameFile, _T("%s\\Maps\\MapNames.ini"), g_szAppFolder);

	FILE *pFile = _tfopen(szMapIniFile, _T("r"));
	if (pFile == NULL) return FALSE;
	pComboBox = (CComboBox*)GetDlgItem(IDC_TRAINMAP_COMBO);
	int nIndex = pComboBox->AddString(_T("_Bản đồ khác_"));
	pComboBox->SetItemData(nIndex, 0);
	do
	{
		char szLine[64];
		TCHAR szULine[64];
		memset(szLine, 0, 64);
		int i = 0; int c; BOOL bstop = FALSE;
		while (i < 64)
		{	//ReadLine
			c = fgetc(pFile);
			if (c == EOF) { bstop = TRUE; break; }
			if (c == '\n') break;
			szLine[i++] = c;
		}
		if (!szLine[0]) break;
		char *cPos = strchr(szLine, '=');
		if (szLine[0]!= '[' && cPos)
		{
			cPos[0]='\0';
			int nMapID = atoi(szLine);
			_stprintf(szULine, _T("%d"), nMapID);
			GetPrivateProfileString(_T("Map_Name"), szULine, _T("<Unknown_Map>"), szULine, 64, szMapNameFile);
			WideCharToMultiByte(CP_ACP, 0, szULine, -1, szLine, 64, 0, 0);
			TCVN3ToUnicode(szLine, szULine, 63);
			nIndex = pComboBox->AddString(szULine);
			pComboBox->SetItemData(nIndex, nMapID);
		}
		if (bstop) break;
	}
	while (TRUE);
	fclose(pFile);

	if (m_ToolTip.Create(this))
	{
		m_ToolTip.AddTool(GetDlgItem(IDC_AUTORETURN_CHECK), _T("Hồi thành tự quay lại nơi Luyện công (phím tắt Ctrl + G)"));
		m_ToolTip.AddTool(GetDlgItem(IDC_GETMONEY_CHECK), _T("Rút tiền trong Rương khi Hồi thành"));
		m_ToolTip.AddTool(GetDlgItem(IDC_GETMONEY_EDIT), _T("Số tiền cần rút (vạn lượng)"));
		m_ToolTip.AddTool(GetDlgItem(IDC_REPOSITORYPASS_EDIT), _T("Mật mã Rương (để rút tiền)"));
		m_ToolTip.AddTool(GetDlgItem(IDC_BUYITEM_COMBO1), _T("Loại thuốc cần mua"));
		m_ToolTip.AddTool(GetDlgItem(IDC_BUYITEM_EDIT1), _T("Số lượng cần mua"));
		m_ToolTip.AddTool(GetDlgItem(IDC_BUYITEM_COMBO2), _T("Loại thuốc cần mua"));
		m_ToolTip.AddTool(GetDlgItem(IDC_BUYITEM_EDIT2), _T("Số lượng cần mua"));
		m_ToolTip.AddTool(GetDlgItem(IDC_BUYITEM_COMBO3), _T("Loại thuốc cần mua"));
		m_ToolTip.AddTool(GetDlgItem(IDC_BUYITEM_EDIT3), _T("Số lượng cần mua"));
		m_ToolTip.AddTool(GetDlgItem(IDC_BUYAM_CHECK), _T("Mua Thổ Địa Phù (ở Tạp hóa)"));
		m_ToolTip.AddTool(GetDlgItem(IDC_BUYITEM_EDIT3), _T("Số lượng TĐP cần mua"));
		m_ToolTip.AddTool(GetDlgItem(IDC_SELLITEM_CHECK), _T("Bán đồ (ở Tạp hóa / Hiệu thuốc)"));
		m_ToolTip.AddTool(GetDlgItem(IDC_REPAIRITEM_CHECK), _T("Sửa đồ (ở Hiệu thuốc)"));
		m_ToolTip.AddTool(GetDlgItem(IDC_GETREPUTATION_CHECK), _T("Nhận PD / DV ở Võ Lâm Truyền Nhân"));
		m_ToolTip.AddTool(GetDlgItem(IDC_SAVEMONEY_CHECK), _T("Gửi lại tiền vào Rương trước khi đi Luyện công"));
		m_ToolTip.AddTool(GetDlgItem(IDC_SAVEMONEY_EDIT), _T("Số tiền mang theo đi Luyện công (lượng)"));
		m_ToolTip.AddTool(GetDlgItem(IDC_SAVEITEM_COMBO), _T("Cất đồ vào Rương 1-2-3"));
		m_ToolTip.AddTool(GetDlgItem(IDC_DRIVER_COMBO), _T("Chế độ đi Xa phu"));
		m_ToolTip.AddTool(GetDlgItem(IDC_TRAINMAP_COMBO), _T("Bản đồ luyện công"));
		m_ToolTip.AddTool(GetDlgItem(IDC_GIVEKCL_CHECK), _T("Tự nộp KCL mỗi khi về thành"));
		m_ToolTip.AddTool(GetDlgItem(IDC_GIVEKCL_EDIT), _T("Số lượng KCL tối thiểu để về giao nộp"));
	}

	return FALSE;
}

BOOL CDlgMap::PreTranslateMessage(MSG* pMsg)
{
	m_ToolTip.RelayEvent(pMsg);
	return CDialog::PreTranslateMessage(pMsg);
}

void CDlgMap::InitComboBox(CComboBox *pComboBox)
{
	pComboBox->ResetContent();
	pComboBox->AddString(_T("Kim Sáng Dược (tiểu)"));
	pComboBox->AddString(_T("Kim Sáng Dược (trung)"));
	pComboBox->AddString(_T("Kim Sáng Dược (đại)"));
	pComboBox->AddString(_T("Hồi Thiên Đan"));
	pComboBox->AddString(_T("Ngưng Thần Đan (tiểu)"));
	pComboBox->AddString(_T("Ngưng Thần Đan (trung)"));
	pComboBox->AddString(_T("Ngưng Thần Đan (đại)"));
	pComboBox->AddString(_T("Đại Bổ Tán"));
	pComboBox->AddString(_T("Thừa Tiên Mật (tiểu)"));
	pComboBox->AddString(_T("Thừa Tiên Mật (trung)"));
	pComboBox->AddString(_T("Tam Hoa Tán"));
	pComboBox->AddString(_T("Hoắc Hương Chính Khí Hoàn"));
	pComboBox->AddString(_T("Tam Thuận Tán"));
	pComboBox->AddString(_T("Ngân Kiều Khứ Độc Đan"));
	pComboBox->AddString(_T("Quy Nguyên Đơn"));
	pComboBox->AddString(_T("Thanh Hồng Đơn"));
	pComboBox->AddString(_T("Ngũ Hoa Ngọc Lộ Hoàn"));
	pComboBox->AddString(_T("Long Đàm Hóa Độc đan"));
}

void CDlgMap::UpdateTrainMap(BOOL bSave)
{
	if (g_pCurGameWin == NULL) return;
	CComboBox *pTrainMapCmb = (CComboBox *)GetDlgItem(IDC_TRAINMAP_COMBO);
	if (bSave)
		g_pCurGameWin->m_prMap.m_nTrainMapID = (int)pTrainMapCmb->GetItemData(pTrainMapCmb->GetCurSel());
	else
	{
		int nCount = pTrainMapCmb->GetCount();
		pTrainMapCmb->SetCurSel(0);
		for (int i = 0; i < nCount; i++)
		{
			if (pTrainMapCmb->GetItemData(i) == g_pCurGameWin->m_prMap.m_nTrainMapID)
			{
				pTrainMapCmb->SetCurSel(i);
				break;
			}
		}
	}
}

void CDlgMap::OnButtonsClicked(UINT nID)
{
	if (g_pCurGameWin == NULL) return;

	CString szMessage;
	CButton *pButton = (CButton*)GetDlgItem(nID);

	if (nID == IDC_GETMONEY_CHECK)
	{
		if (pButton->GetCheck())
		{
			szMessage = _T("Việc lưu Pass rương vào chương trình có thể gây nguy hiểm!\n");
			szMessage += _T("Nếu máy có nhiều người sử dụng, bạn không nên dùng chức năng này.\n");
			szMessage += _T("Chỉ nên lưu Pass rương nếu bạn đang dùng máy ở nhà!");
			MessageBox(szMessage, _T("VLAuto - Chú ý"), MB_ICONEXCLAMATION);
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
				szMessage += _T("Bạn đã đọc kỹ Hướng dẫn cho mục này chưa?");
			}
			int nRet = MessageBox(szMessage, _T("VLAuto - Xác nhận"), MB_YESNO|MB_ICONQUESTION);
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
		MessageBox(szMesage, _T("VLAuto"), MB_ICONEXCLAMATION);
	}
}

void CDlgMap::OnCbnSelchangeTrainmapCombo()
{
	UpdateData();
	g_pCurGameWin->SetGameParams(tab_map);
}

void CDlgMap::OnCbnSelchangeSaveitemCombo()
{
	UpdateData();
	g_pCurGameWin->SetGameParams(tab_map);
}
