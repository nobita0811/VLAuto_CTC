// DlgYeSou.cpp : implementation file
//

#include "stdafx.h"
#include "VLAuto.h"
#include "../GameDef.h"
#include "GameCore.h"
#include "GameWindow.h"
#include "MyListCtrl.h"
#include "DlgYeSou.h"

// CYeSouOptions

LPCTSTR szQuestType[] = { _T("Tự làm"), _T("Hủy bỏ"), _T("Dùng Sơ đề"), _T("Nhờ chủ") };

IMPLEMENT_DYNAMIC(CYeSouOptions, CPropertySheet)

CYeSouOptions::CYeSouOptions(LPCTSTR pszCaption, CWnd* pParentWnd)
	:CPropertySheet(pszCaption, pParentWnd)
{
	m_hGameWnd = NULL;
	m_psh.dwFlags |= PSH_NOAPPLYNOW;
	AddPage(&m_BuyItemPage);
	AddPage(&m_TuZhiPage);
	AddPage(&m_AddPointPage);
	AddPage(&m_PrizeSelPage);
#ifndef VLTK_FREE_MODE
	AddPage(&m_TongQPage);
	AddPage(&m_TongPKPage);
#endif
}

CYeSouOptions::~CYeSouOptions()
{
}

BEGIN_MESSAGE_MAP(CYeSouOptions, CPropertySheet)
END_MESSAGE_MAP()

void CYeSouOptions::SetOptionAdds()
{
	if (!g_pCurGameWin || !g_pCurGameWin->m_hWnd)
		return;
	m_hGameWnd = g_pCurGameWin->m_hWnd;
	m_BuyItemPage.m_pBuyItemOpt = &g_pCurGameWin->m_prYeSou.m_BuyItemOpt;
	m_TuZhiPage.m_pTuZhiOpt = &g_pCurGameWin->m_prYeSou.m_TuZhiOpt;
	m_AddPointPage.m_pAddPointOpt = &g_pCurGameWin->m_prYeSou.m_AddPointOpt;
	m_PrizeSelPage.m_pPrizeSelOpt = &g_pCurGameWin->m_prYeSou.m_PrizeSelOpt;
	m_TongQPage.m_pTongQOpt = &g_pCurGameWin->m_prYeSou.m_TongQOpt;
	m_TongPKPage.m_pTongPKOpt = &g_pCurGameWin->m_prYeSou.m_TongPKOpt;
}

// CDlgYeSou dialog

IMPLEMENT_DYNAMIC(CDlgYeSou, CDialog)

CDlgYeSou::CDlgYeSou(CWnd* pParent /*=NULL*/)
	: CDialog(CDlgYeSou::IDD, pParent)
{
	m_pMoreOptDlg = NULL;
}

CDlgYeSou::~CDlgYeSou()
{
	if (m_pMoreOptDlg)
	{
		delete m_pMoreOptDlg;
		m_pMoreOptDlg = NULL;
	}
}

void CDlgYeSou::DoDataExchange(CDataExchange* pDX)
{
	DDX_Control(pDX, IDC_YSQUEST_LIST, m_QuestList);

	CDialog::DoDataExchange(pDX);

	if (g_pCurGameWin == NULL)
	{
		GetDlgItem(IDC_LOGS_BUTTON)->EnableWindow(0);
		GetDlgItem(IDC_OPTION_BUTTON)->EnableWindow(0);
		return;
	}

	UpdateQuestList(pDX->m_bSaveAndValidate);
	
	DDX_Check(pDX, IDC_AUTOYESOU_CHECK, g_pCurGameWin->m_prYeSou.m_bAutoDoQuest);
	DDX_Check(pDX, IDC_INTOWN_CHECK, g_pCurGameWin->m_prYeSou.m_bInTownOnly);
	DDX_Check(pDX, IDC_R1_CHECK, g_pCurGameWin->m_prYeSou.m_bUseR1);
	DDX_Check(pDX, IDC_R2_CHECK, g_pCurGameWin->m_prYeSou.m_bUseR2);
	DDX_Check(pDX, IDC_R3_CHECK, g_pCurGameWin->m_prYeSou.m_bUseR3);
	DDX_CBIndex(pDX, IDC_SUSPEND_COMBO, g_pCurGameWin->m_prYeSou.m_nPostAction);
	
	BOOL bChecked = ((CButton*)GetDlgItem(IDC_AUTOYESOU_CHECK))->GetCheck();
	m_QuestList.SetReadOnly(bChecked);
	m_QuestList.Invalidate();

	GetDlgItem(IDC_LOGS_BUTTON)->EnableWindow();
	GetDlgItem(IDC_OPTION_BUTTON)->EnableWindow();
}

BEGIN_MESSAGE_MAP(CDlgYeSou, CDialog)
	ON_CONTROL_RANGE(BN_CLICKED, 1000, 2000, &CDlgYeSou::OnButtonsClicked)
	ON_CBN_SELCHANGE(IDC_SUSPEND_COMBO, &CDlgYeSou::OnCbnSelchangeSuspendCombo)
	ON_NOTIFY(NM_CUSTOMDRAW, IDC_YSQUEST_LIST, &CDlgYeSou::OnCustomdrawCharList)
END_MESSAGE_MAP()

// CDlgYeSou message handlers

BOOL CDlgYeSou::OnInitDialog()
{
	CDialog::OnInitDialog();

	m_QuestList.InsertColumn(0, _T("Loại nhiệm vụ"), LVCFMT_LEFT, 150);
	m_QuestList.InsertColumn(1, _T("Xử lý"), LVCFMT_LEFT, 100);
	m_QuestList.SetExtendedStyle(LVS_EX_GRIDLINES|LVS_EX_FULLROWSELECT);
	CImageList imgList;
	imgList.Create(1, 16, ILC_COLOR4, 1, 1);
	m_QuestList.SetImageList(&imgList, LVSIL_SMALL);
	m_QuestList.SetColumnType(column_combo);
	m_QuestList.InsertComboText(szQuestType[0]);
	m_QuestList.InsertComboText(szQuestType[1]);
	m_QuestList.InsertComboText(szQuestType[2]);
	m_QuestList.InsertItem(0, _T("Dã Tẩu - Trang bị"));
	m_QuestList.SetItemText(0, 1, szQuestType[0]);
	m_QuestList.SetItemData(0, 0);
	m_QuestList.InsertItem(1, _T("Dã Tẩu - Đồ chí"));
	m_QuestList.SetItemText(1, 1, szQuestType[0]);
	m_QuestList.SetItemData(1, 0);
	m_QuestList.InsertItem(2, _T("Dã Tẩu - Kinh nghiệm"));
	m_QuestList.SetItemText(2, 1, szQuestType[0]);
	m_QuestList.SetItemData(2, 0);
	m_QuestList.InsertItem(3, _T("Dã Tẩu - Danh vọng"));
	m_QuestList.SetItemText(3, 1, szQuestType[0]);
	m_QuestList.SetItemData(3, 0);
	m_QuestList.InsertItem(4, _T("Dã Tẩu - Phúc duyên"));
	m_QuestList.SetItemText(4, 1, szQuestType[0]);
	m_QuestList.SetItemData(4, 0);
	m_QuestList.InsertItem(5, _T("Dã Tẩu - Trị PK"));
	m_QuestList.SetItemText(5, 1, szQuestType[2]);
	m_QuestList.SetItemData(5, 2);
	m_QuestList.InsertItem(6, _T("Dã Tẩu - Tống Kim"));
	m_QuestList.SetItemText(6, 1, szQuestType[2]);
	m_QuestList.SetItemData(6, 2);
#ifndef VLTK_FREE_MODE
	m_QuestList.InsertItem(7, _T("Bang hội - Vật phẩm"));
	m_QuestList.SetItemText(7, 1, szQuestType[0]);
	m_QuestList.SetItemData(7, 0);
	m_QuestList.InsertItem(8, _T("Bang hội - Diệt quái"));
	m_QuestList.SetItemText(8, 1, szQuestType[0]);
	m_QuestList.SetItemData(8, 0);
	m_QuestList.InsertItem(9, _T("Bang hội - Giảm PK"));
	m_QuestList.SetItemText(9, 1, szQuestType[2]);
	m_QuestList.SetItemData(9, 2);
#endif
	CComboBox *pComboBox;
	pComboBox = (CComboBox*)GetDlgItem(IDC_SUSPEND_COMBO);
	pComboBox->AddString(_T("0 - Đứng im"));
	pComboBox->AddString(_T("1 - Thoát Game"));
	pComboBox->AddString(_T("2 - Báo chuông"));
	pComboBox->AddString(_T("3 - Đi luyện công"));

	if (m_ToolTip.Create(this))
	{
		m_ToolTip.AddTool(GetDlgItem(IDC_AUTOYESOU_CHECK), _T("Tự làm nhiệm vụ Dã Tẩu / Bang hội (phím tắt Ctrl + D)"));
		m_ToolTip.AddTool(GetDlgItem(IDC_INTOWN_CHECK), _T("Nhận nhiệm vụ Dã Tẩu ở thôn, trấn"));
		m_ToolTip.AddTool(GetDlgItem(IDC_YSQUEST_LIST), _T("Danh mục các nhiệm vụ Dã Tẩu / Bang hội"));
		m_ToolTip.AddTool(GetDlgItem(IDC_R1_CHECK), _T("Tự lấy đồ từ Rương 1"));
		m_ToolTip.AddTool(GetDlgItem(IDC_R2_CHECK), _T("Tự lấy đồ từ Rương 2"));
		m_ToolTip.AddTool(GetDlgItem(IDC_R3_CHECK), _T("Tự lấy đồ từ Rương 3"));
		m_ToolTip.AddTool(GetDlgItem(IDC_SUSPEND_COMBO), _T("Hoạt động sau khi ngưng làm nhiệm vụ"));
		m_ToolTip.AddTool(GetDlgItem(IDC_LOGS_BUTTON), _T("Xem Nhật ký nhiệm vụ (font TCVN3)"));
		m_ToolTip.AddTool(GetDlgItem(IDC_OPTION_BUTTON), _T("Các tùy chọn khác"));
	}

	m_pMoreOptDlg = new CYeSouOptions(_T("Nhiem vu - Options"), this);

	return FALSE;
}

BOOL CDlgYeSou::PreTranslateMessage(MSG* pMsg)
{
	m_ToolTip.RelayEvent(pMsg);
	return CDialog::PreTranslateMessage(pMsg);
}

void CDlgYeSou::OnCustomdrawCharList(NMHDR* pNMHDR, LRESULT* pResult)
{
    *pResult = 0;

	NMLVCUSTOMDRAW* pLVCD = reinterpret_cast<NMLVCUSTOMDRAW*>(pNMHDR);

	if (CDDS_PREPAINT == pLVCD->nmcd.dwDrawStage)
		*pResult = CDRF_NOTIFYITEMDRAW;
    else if (CDDS_ITEMPREPAINT == pLVCD->nmcd.dwDrawStage)
	{
		COLORREF crText = RGB(0, 64, 128);
		COLORREF crBkText = RGB(255, 255, 255);

		if (m_QuestList.IsReadOnly())
		{
			crText = RGB(0,0,0);
			crBkText = RGB(231,223,231);
		}
		else if (pLVCD->nmcd.dwItemSpec > 6)
		{
			crText = RGB(0,64,0);
			crBkText = RGB(255,255,231);
		}

		pLVCD->clrText = crText;
		pLVCD->clrTextBk = crBkText;
		*pResult = CDRF_DODEFAULT;
	}
}

void CDlgYeSou::OnButtonsClicked(UINT nID)
{
	if (g_pCurGameWin == NULL) return;

	if (nID == IDC_OPTION_BUTTON)
	{
		m_pMoreOptDlg->SetOptionAdds();
		if (m_pMoreOptDlg->DoModal() == IDOK)
			g_pCurGameWin->SetGameParams(tab_yesou);
		return;
	}
	if (nID == IDC_LOGS_BUTTON)
	{
		char *szPlayerName = g_pCurGameWin->m_szPlayerName;
		if (!szPlayerName[0] || strstr(szPlayerName, VLTK_ERROR))
			return;
		char szEngName[64] = "";
		TCHAR szUniName[64] = _T("");
		int i = 0, j = 0;
		while (szPlayerName[i] && i < 30 && j < 60)
		{
			if (szPlayerName[i] > 0x1F && szPlayerName[i] < 0x7F && !strchr("\\/:*?\"<>|", szPlayerName[i]))
				szEngName[j++] = szPlayerName[i];
			else
			{
				BYTE btHi = (szPlayerName[i] >> 4) & 0x0f;
				BYTE btLo = szPlayerName[i] & 0x0f;
				if (btHi < 10) btHi += 0x30; else btHi += 0x57;
				if (btLo < 10) btLo += 0x30; else btLo += 0x57;
				szEngName[j++] = (char)btHi;
				szEngName[j++] = (char)btLo;
			}
			i++;
		}
		MultiByteToWideChar(CP_ACP, 0, szEngName, -1, szUniName, 60);
		TCHAR szLogFile[MAX_PATH];
		_stprintf(szLogFile, _T("%s\\Logs\\%s.log"), g_szAppFolder, szUniName);
		ShellExecute(NULL, _T("Open"), _T("notepad.exe"), szLogFile, NULL, SW_SHOW);
		return;
	}
	UpdateData();
	g_pCurGameWin->SetGameParams(tab_yesou);
}

void CDlgYeSou::OnCbnSelchangeSuspendCombo()
{
	if (g_pCurGameWin == NULL) return;
	UpdateData();
	g_pCurGameWin->SetGameParams(tab_yesou);
}

void CDlgYeSou::UpdateQuestList(BOOL bSave)
{
	if (bSave)
	{
		g_pCurGameWin->m_prYeSou.m_nFindItem = (int)m_QuestList.GetItemData(0);
		g_pCurGameWin->m_prYeSou.m_nTuZhi = (int)m_QuestList.GetItemData(1);
		g_pCurGameWin->m_prYeSou.m_nJingYan = (int)m_QuestList.GetItemData(2);
		g_pCurGameWin->m_prYeSou.m_nMingWang = (int)m_QuestList.GetItemData(3);
		g_pCurGameWin->m_prYeSou.m_nFuYuan = (int)m_QuestList.GetItemData(4);
		g_pCurGameWin->m_prYeSou.m_nYesouPK = (int)m_QuestList.GetItemData(5);
		g_pCurGameWin->m_prYeSou.m_nSongJin = (int)m_QuestList.GetItemData(6);
		g_pCurGameWin->m_prYeSou.m_nTongItem = (int)m_QuestList.GetItemData(7);
		g_pCurGameWin->m_prYeSou.m_nTongKill = (int)m_QuestList.GetItemData(8);
		g_pCurGameWin->m_prYeSou.m_nTongPK = (int)m_QuestList.GetItemData(9);
	}
	else
	{
		m_QuestList.SetItemText(0, 1, szQuestType[g_pCurGameWin->m_prYeSou.m_nFindItem]);
		m_QuestList.SetItemData(0, g_pCurGameWin->m_prYeSou.m_nFindItem);
		m_QuestList.SetItemText(1, 1, szQuestType[g_pCurGameWin->m_prYeSou.m_nTuZhi]);
		m_QuestList.SetItemData(1, g_pCurGameWin->m_prYeSou.m_nTuZhi);
		m_QuestList.SetItemText(2, 1, szQuestType[g_pCurGameWin->m_prYeSou.m_nJingYan]);
		m_QuestList.SetItemData(2, g_pCurGameWin->m_prYeSou.m_nJingYan);
		m_QuestList.SetItemText(3, 1, szQuestType[g_pCurGameWin->m_prYeSou.m_nMingWang]);
		m_QuestList.SetItemData(3, g_pCurGameWin->m_prYeSou.m_nMingWang);
		m_QuestList.SetItemText(4, 1, szQuestType[g_pCurGameWin->m_prYeSou.m_nFuYuan]);
		m_QuestList.SetItemData(4, g_pCurGameWin->m_prYeSou.m_nFuYuan);
		m_QuestList.SetItemText(5, 1, szQuestType[g_pCurGameWin->m_prYeSou.m_nYesouPK]);
		m_QuestList.SetItemData(5, g_pCurGameWin->m_prYeSou.m_nYesouPK);
		m_QuestList.SetItemText(6, 1, szQuestType[g_pCurGameWin->m_prYeSou.m_nSongJin]);
		m_QuestList.SetItemData(6, g_pCurGameWin->m_prYeSou.m_nSongJin);
		m_QuestList.SetItemText(7, 1, szQuestType[g_pCurGameWin->m_prYeSou.m_nTongItem]);
		m_QuestList.SetItemData(7, g_pCurGameWin->m_prYeSou.m_nTongItem);
		m_QuestList.SetItemText(8, 1, szQuestType[g_pCurGameWin->m_prYeSou.m_nTongKill]);
		m_QuestList.SetItemData(8, g_pCurGameWin->m_prYeSou.m_nTongKill);
		m_QuestList.SetItemText(9, 1, szQuestType[g_pCurGameWin->m_prYeSou.m_nTongPK]);
		m_QuestList.SetItemData(9, g_pCurGameWin->m_prYeSou.m_nTongPK);
	}
}
