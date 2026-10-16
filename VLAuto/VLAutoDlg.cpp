// VLAutoDlg.cpp : implementation file
//

#include "stdafx.h"
#include "../GameDef.h"
#include "../VLHook/VLHook.h"
#include "AntiRE.h"
#include "VLAuto.h"
#include "GameCore.h"
#include "GameWindow.h"
#include "DlgBasic.h"
#include "DlgRecover.h"
#include "DlgAttack.h"
#include "DlgCollect.h"
#include "DlgMove.h"
#include "DlgParty.h"
#include "DlgMap.h"
#include "DlgYeSou.h"
#include "DlgAbout.h"
#include "DlgExit.h"
#include "DlgOption.h"
#include "VLAutoDlg.h"

#define LVIS_UNCHECKED	0x1000
#define LVIS_CHECKED	0x2000
#define SYS_TIMER		500

const UINT WM_TASKBARCREATED = RegisterWindowMessage(_T("TaskbarCreated"));

// CVLAutoDlg dialog
CVLAutoDlg::CVLAutoDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CVLAutoDlg::IDD, pParent)
{
	g_pCharList = &m_CharList;
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

CVLAutoDlg::~CVLAutoDlg()
{
}

void CVLAutoDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_MAIN_TAB, m_MainTab);
	DDX_Control(pDX, IDC_CHAR_LIST, m_CharList);
	//DDX_Control(pDX, IDC_WEBSITE_STATIC, m_WebLink);
}

BEGIN_MESSAGE_MAP(CVLAutoDlg, CDialog)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	//}}AFX_MSG_MAP
	ON_MESSAGE(WM_NOTIFYICON, &CVLAutoDlg::OnTrayNotify)
	ON_REGISTERED_MESSAGE(WM_TASKBARCREATED, &CVLAutoDlg::OnTaskBarCreated)
	ON_WM_ACTIVATE()
	ON_WM_DESTROY()
	ON_WM_TIMER()
	ON_NOTIFY(LVN_ITEMCHANGED, IDC_CHAR_LIST, &CVLAutoDlg::OnLvnItemchangedCharList)
	ON_NOTIFY(NM_DBLCLK, IDC_CHAR_LIST, &CVLAutoDlg::OnNMDblclkCharList)
	ON_NOTIFY(NM_CUSTOMDRAW, IDC_CHAR_LIST, &CVLAutoDlg::OnCustomdrawCharList)
	ON_MESSAGE(WM_FEEDBACK, &CVLAutoDlg::OnReceiveFeedBack)
	ON_BN_CLICKED(IDC_AUTO_LOGIN_BUTTON, &CVLAutoDlg::OnBnClickedAutoLoginButton)
END_MESSAGE_MAP()


// CVLAutoDlg message handlers
BOOL CVLAutoDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	// Set the icon for this dialog.  The framework does this automatically
	// when the application's main window is not a dialog
	SetIcon(m_hIcon, TRUE);			// Set big icon
	SetIcon(m_hIcon, FALSE);		// Set small icon

	SetWindowText(_T(VLADlgTitle));

	// VLAuto chay Admin, Game chay quyen thuong: Windows (UIPI) chan WM_FEEDBACK tu Game neu khong cho phep
	AllowMessageFromGame(m_hWnd, WM_FEEDBACK);

	CMenu mainMenu;
	mainMenu.LoadMenu(IDR_MAIN_MENU);
	mainMenu.GetSubMenu(0)->SetDefaultItem(ID__SHOWVLAUTO, FALSE);

	UINT uWinX = GetPrivateProfileInt(_T("Options"), _T("WindX"), 0, g_szIniFile);
	UINT uWinY = GetPrivateProfileInt(_T("Options"), _T("WindY"), 0, g_szIniFile);
	SetWindowPos(NULL, uWinX, uWinY, NULL, NULL, SWP_NOSIZE | SWP_NOZORDER);
	//m_WebLink.SetURL(_T("https://chatgpt.com"));

	if (m_ToolTip.Create(this))
	{
		m_ToolTip.AddTool(GetDlgItem(IDC_CHAR_LIST), _T("Danh sách nhân vật"));
		m_ToolTip.AddTool(GetDlgItem(IDC_WEBSITE_STATIC), _T("Trang chủ VLAuto"));
	}

	m_TrayIcon.Create(this, IDR_MAIN_MENU);
	m_TrayIcon.SetIcon(m_hIcon, _TTrayToolTip);

	g_pCharList->InsertColumn(0, _T("Tên NV"), LVCFMT_LEFT, 162);
	g_pCharList->InsertColumn(1, _T("% KN"), LVCFMT_CENTER, 50);
	g_pCharList->SetExtendedStyle(LVS_EX_CHECKBOXES|LVS_EX_FULLROWSELECT|LVS_EX_GRIDLINES|LVS_EX_ONECLICKACTIVATE);

	InitTabDialogs();

#ifndef _DEBUG
	if (Int2DCheck()) return TRUE;
#endif

	SetTimer(1, SYS_TIMER, NULL);

	// Tạo sẵn cửa sổ Auto Login (ẩn) để đăng nhập chạy ngầm từ lúc mở Auto
	m_DlgAutoLogin.Create(CDlgAutoLogin::IDD, this);

	return TRUE;  // return TRUE  unless you set the focus to a control
}

BOOL CVLAutoDlg::PreTranslateMessage(MSG* pMsg)
{
	m_ToolTip.RelayEvent(pMsg);
	return CDialog::PreTranslateMessage(pMsg);
}

// If you add a minimize button to your dialog, you will need the code below
//  to draw the icon.  For MFC applications using the document/view model,
//  this is automatically done for you by the framework.
void CVLAutoDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // device context for painting

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// Center icon in client rectangle
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// Draw the icon
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialog::OnPaint();
	}
}

// The system calls this function to obtain the cursor to display while the user drags
//  the minimized window.
HCURSOR CVLAutoDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}

void CVLAutoDlg::InitTabDialogs()
{
	int nTab = tab_basic;
	//m_MainTab.SetBmpImageList(IDB_TAB_BITMAP, 16);

	// CDlgBasic
	CDlgBasic* pBasicDlg = new CDlgBasic();
	pBasicDlg->Create(CDlgBasic::IDD, &m_MainTab);
	m_MainTab.InsertTabDlg(nTab, pBasicDlg, _T("Cơ bản"), nTab);
	m_MainTab.SetTabToolTip(nTab++, _T("Thiết lập Cơ bản"));

	// CDlgRecover
	CDlgRecover* pRecoverDlg = new CDlgRecover();
	pRecoverDlg->Create(CDlgRecover::IDD, &m_MainTab);
	m_MainTab.InsertTabDlg(nTab, pRecoverDlg, _T("Phục hồi"), nTab);
	m_MainTab.SetTabToolTip(nTab++, _T("Thiết lập Phục hồi"));

	// CDlgAttack
	CDlgAttack* pCombatDlg = new CDlgAttack();
	pCombatDlg->Create(CDlgAttack::IDD, &m_MainTab);
	m_MainTab.InsertTabDlg(nTab, pCombatDlg, _T("Chiến đấu"), nTab);
	m_MainTab.SetTabToolTip(nTab++, _T("Thiết lập Chiến đấu"));

	// CDlgCollect
	CDlgCollect* pPickupDlg = new CDlgCollect();
	pPickupDlg->Create(CDlgCollect::IDD, &m_MainTab);
	m_MainTab.InsertTabDlg(nTab, pPickupDlg, _T("Nhặt đồ"), nTab);
	m_MainTab.SetTabToolTip(nTab++, _T("Thiết lập Nhặt đồ"));

	// CDlgMove
	CDlgMove* pMoveDlg = new CDlgMove();
	pMoveDlg->Create(CDlgMove::IDD, &m_MainTab);
	m_MainTab.InsertTabDlg(nTab, pMoveDlg, _T("Di chuyển"), nTab);
	m_MainTab.SetTabToolTip(nTab++, _T("Thiết lập Di chuyển"));

	// CDlgParty
	CDlgParty* pPartyDlg = new CDlgParty();
	pPartyDlg->Create(CDlgParty::IDD, &m_MainTab);
	m_MainTab.InsertTabDlg(nTab, pPartyDlg, _T("Tổ đội"), nTab);
	m_MainTab.SetTabToolTip(nTab++, _T("Thiết lập Tổ đội"));

	// CDlgMap
	CDlgMap* pLogisticsDlg = new CDlgMap();
	pLogisticsDlg->Create(CDlgMap::IDD, &m_MainTab);
	m_MainTab.InsertTabDlg(nTab, pLogisticsDlg, _T("Bản đồ"), nTab);
	m_MainTab.SetTabToolTip(nTab++, _T("Thiết lập Hậu cần khi về thành"));

	// CDlgYeSou
	CDlgYeSou* pQuestDlg = new CDlgYeSou();
	pQuestDlg->Create(CDlgYeSou::IDD, &m_MainTab);
	m_MainTab.InsertTabDlg(nTab, pQuestDlg, _T("Nhiệm vụ"), nTab);
	m_MainTab.SetTabToolTip(nTab++, _T("Thiết lập Nhiệm vụ (Dã Tẩu, Bang hội)"));

	// CDlgAbout
	CDlgAbout* pAboutDlg = new CDlgAbout();
	pAboutDlg->Create(CDlgAbout::IDD, &m_MainTab);
	m_MainTab.InsertTabDlg(nTab, pAboutDlg, _T("Trợ giúp"), nTab);
	m_MainTab.SetTabToolTip(nTab++, _T("Trợ giúp, Hướng dẫn, Tiện ích khác"));

	m_MainTab.ArrageDialogs();
}

void CVLAutoDlg::OnExitDlg(INT_PTR nExitCode)
{
	if (nExitCode != IDOK && nExitCode != IDCANCEL) return;
	if (MessageBox(_T("Bạn thật sự muốn thoát chương trình?"), 0, MB_ICONQUESTION | MB_YESNO) == IDYES)
	{
		if (nExitCode == IDOK)
			CDialog::OnOK();
		else
			CDialog::OnCancel();
	}
}

void CVLAutoDlg::OnCancel()
{
	OnExitDlg(IDCANCEL);
}

BOOL CVLAutoDlg::OnCommand(WPARAM wParam, LPARAM lParam)
{
	TCHAR szFileName[MAX_PATH];
	COptionDlg dlgOption;
	int nCharCount = g_pCharList->GetItemCount();

	switch (wParam)
	{
	case ID__SHOWVLAUTO:
		ShowWindow(SW_RESTORE);
		SetForegroundWindow();
		break;
	case ID__HIDEALLGAME:
		for (int i = 0; i < nCharCount; i++)
		{
			GameWindow* pGameWin = (GameWindow*)g_pCharList->GetItemData(i);
			::ShowWindow(pGameWin->m_hWnd, SW_HIDE);
		}
		break;
	case ID__SHOWALLGAME:
		for (int i = 0; i < nCharCount; i++)
		{
			GameWindow* pGameWin = (GameWindow*)g_pCharList->GetItemData(i);
			::ShowWindow(pGameWin->m_hWnd, SW_SHOW);
		}
		break;
	case ID__OPTIONS:
		ShowWindow(SW_RESTORE);
		SetForegroundWindow();
		if (dlgOption.DoModal() != IDOK)
			break;
		for (int i = 0; i < nCharCount; i++)
		{
			GameWindow* pGameWin = (GameWindow*)g_pCharList->GetItemData(i);
			if (!pGameWin->m_hWnd || !pGameWin->m_bAttached) continue;
			::PostMessage(pGameWin->m_hWnd, WM_HOOK_MAIN, cmd_loadini, 0);
		}
		break;
	case ID__BLACKLIST:
			_stprintf(szFileName, _T("%s\\Maps\\BlackItems.txt"), g_szAppFolder);
			::ShellExecute(NULL, _T("Open"), szFileName, 0, 0, SW_SHOW);
			break;
	case ID__REDLIST:
			_stprintf(szFileName, _T("%s\\Maps\\RedItems.txt"), g_szAppFolder);
			::ShellExecute(NULL, _T("Open"), szFileName, 0, 0, SW_SHOW);
			break;
	case ID__SELLLIST:
			_stprintf(szFileName, _T("%s\\Maps\\SellItems.txt"), g_szAppFolder);
			::ShellExecute(NULL, _T("Open"), szFileName, 0, 0, SW_SHOW);
			break;
	case ID__X2LIST:
			_stprintf(szFileName, _T("%s\\Maps\\X2Items.txt"), g_szAppFolder);
			::ShellExecute(NULL, _T("Open"), szFileName, 0, 0, SW_SHOW);
			break;
	case ID__NOPTLIST:
			_stprintf(szFileName, _T("%s\\Maps\\NoPTNames.txt"), g_szAppFolder);
			::ShellExecute(NULL, _T("Open"), szFileName, 0, 0, SW_SHOW);
			break;
	case ID__SIGNOUT:
		WritePrivateProfileString(_T("LoginInfo"), _T("LoginEmail"), _T(""), g_szIniFile);
		ShowWindow(SW_RESTORE);
		SetForegroundWindow();
		OnExitDlg(IDOK);
		break;
	case ID__HIDEVLAUTO:
		ShowWindow(SW_HIDE);
		m_TrayIcon.SetIcon(0);
		break;
	case ID__EXITVLAUTO:
		ShowWindow(SW_RESTORE);
		SetForegroundWindow();
		OnExitDlg(IDCANCEL);
		break;
	}
	return CDialog::OnCommand(wParam, lParam);
}

void CVLAutoDlg::SendAttackCommand(HWND hWnd)
{
	/*static int nTimeCount = 25;

	if (::GetForegroundWindow() == hWnd)
	{
		nTimeCount = 0;
		INPUT input[2];
		::ZeroMemory(&input[0], sizeof(INPUT));
		input[0].type = INPUT_KEYBOARD;
		input[0].ki.wVk = VK_F10;
		input[1] = input[0];
		input[1].ki.dwFlags = KEYEVENTF_KEYUP;
		::SendInput(2, input, sizeof(INPUT));
	}
	else if (nTimeCount++ > 30)
	{
		nTimeCount = 0;
		::SetForegroundWindow(hWnd);
	}*/
}

LRESULT CVLAutoDlg::OnTrayNotify(WPARAM wParam, LPARAM lParam)
{
	return m_TrayIcon.OnTrayNotify(wParam, lParam);
}

LRESULT CVLAutoDlg::OnTaskBarCreated(WPARAM wParam, LPARAM lParam)
{
	return m_TrayIcon.OnTaskBarCreated(wParam, lParam);
}

void CVLAutoDlg::OnActivate(UINT nState, CWnd* pWndOther, BOOL bMinimized)
{
	CDialog::OnActivate(nState, pWndOther, bMinimized);
	if (bMinimized) ShowWindow(SW_HIDE);
}

void CVLAutoDlg::OnBnClickedAutoLoginButton()
{
	if (!m_DlgAutoLogin.GetSafeHwnd())
	{
		if (!m_DlgAutoLogin.Create(CDlgAutoLogin::IDD, this))
		{
			MessageBox(_T("Không tạo được cửa sổ Auto Login!"), 0, MB_ICONEXCLAMATION);
			return;
		}
	}
	m_DlgAutoLogin.ShowAutoLogin();
}

void CVLAutoDlg::OnDestroy()
{
	KillTimer(1);

	for (int i = 0; i < g_pCharList->GetItemCount(); i++)
	{
		GameWindow* pGameWin = (GameWindow*)g_pCharList->GetItemData(i);

		if (pGameWin->m_hWnd && strcmp(pGameWin->m_szPlayerName, VLTK_UNNAMED))
			pGameWin->SaveProfile();

		if (pGameWin->m_bAttached)
			StopInject(pGameWin->m_hWnd);

		delete pGameWin;
	}

	RECT rect;
	GetWindowRect(&rect);
	TCHAR szValue[16];

	_itot(rect.left, szValue, 10);
	WritePrivateProfileString(_T("Options"), _T("WindX"), szValue, g_szIniFile);
	_itot(rect.top, szValue, 10);
	WritePrivateProfileString(_T("Options"), _T("WindY"), szValue, g_szIniFile);

	CDialog::OnDestroy();
}

void CVLAutoDlg::OnTimer(UINT nIDEvent)
{
	static int nTimerCount = 0;
	nTimerCount++;

	TCHAR szInfo[32];
	CDlgBasic* pBasicDlg = (CDlgBasic*)m_MainTab.GetTabDlg(tab_basic);

	//ShutDown Check Here
	int nCharCount = g_pCharList->GetItemCount();

	if (nTimerCount%6 == 1) // each 3s
	{
		GameWindowsSet::RefreshWindows(TRUE);
		return;
	}

	GameWindowsSet::RefreshWindows();
	for (int i = 0; i < nCharCount; i++)
	{
		GameWindow* pGameWin = (GameWindow*)g_pCharList->GetItemData(i);
		if (pGameWin->m_hWnd == NULL)
		{
			g_pCharList->DeleteItem(i);
			if (nCharCount > 1)
				g_pCharList->SetItemState(0, LVIS_SELECTED, LVIS_SELECTED);
			else
			{
				g_pCurGameWin = NULL;
				m_MainTab.GetCurTabDlg()->UpdateData(0);
			}

			delete pGameWin;

			break;
		}

		// Tu check Auto: moi lan vao game chi kich hoat 1 lan, bo check tay se khong bi check lai
		if (pGameWin->m_prBasic.m_bAutoStart && !pGameWin->m_bAutoStartDone &&
			pGameWin->m_bLoadDone && pGameWin->m_bRegistered)
		{
			DWORD dwNow = GetTickCount();
			if (pGameWin->m_dwAutoStartTick == 0)
				pGameWin->m_dwAutoStartTick = dwNow ? dwNow : 1;
			else if (dwNow - pGameWin->m_dwAutoStartTick >= (DWORD)max(pGameWin->m_prBasic.m_nAutoStartTime, 0))
			{
				pGameWin->m_bAutoStartDone = TRUE;
				pGameWin->m_dwAutoStartTick = 0;
				if (!pGameWin->m_bAttached && pGameWin->m_nAttachTimeCount == 0)
					g_pCharList->SetItemState(i, LVIS_CHECKED, LVIS_STATEIMAGEMASK);
			}
		}

		// Check for START & RESTART COMMAND
		if (pGameWin->m_nAttachTimeCount > 0 && pGameWin->m_bLoadDone)
			pGameWin->m_nAttachTimeCount--;
		if (pGameWin->m_nAttachTimeCount == 1)
		{
			g_pCharList->SetItemState(i, LVIS_CHECKED, LVIS_STATEIMAGEMASK);
			pGameWin->m_nAttachTimeCount = 0;
		}

		// Display Information for all Chars
		if (pGameWin->m_bInfoChanged)
		{
			TCVN3ToUnicode(pGameWin->m_szPlayerName, szInfo, 32);
			g_pCharList->SetItemText(i, 0, szInfo);
			if (pGameWin->m_nNextExp > 0)
				_stprintf(szInfo, _T("%0.2f"), (float)pGameWin->m_nExp*100/pGameWin->m_nNextExp);
			else
				_stprintf(szInfo, _T("0.00"));
			g_pCharList->SetItemText(i, 1, szInfo);
		}
	}

	if (!IsIconic() && m_MainTab.GetCurTabIdx() == tab_basic)
		pBasicDlg->RefreshUI();
}

void CVLAutoDlg::OnLvnItemchangedCharList(NMHDR* pNMHDR, LRESULT* pResult)
{
	*pResult = 0;

	LPNMLISTVIEW pNMLV = reinterpret_cast<LPNMLISTVIEW>(pNMHDR);
	if ((pNMLV->uChanged & LVIF_STATE) != LVIF_STATE)
		return;

	UINT uChangedState = pNMLV->uNewState ^ pNMLV->uOldState;

	if (uChangedState & LVIS_SELECTED)
	{
		g_pCurGameWin = (pNMLV->uNewState & LVIS_SELECTED) ?
			(GameWindow*)g_pCharList->GetItemData(pNMLV->iItem) : NULL;

		m_MainTab.GetCurTabDlg()->UpdateData(0);

		return;
	}

	if (uChangedState & LVIS_CHECKED)
	{
		GameWindow* pGameWin = (GameWindow*)g_pCharList->GetItemData(pNMLV->iItem);
		if (pGameWin == NULL || pGameWin->m_hWnd == NULL)
			return;
	
		if (pNMLV->uNewState & LVIS_CHECKED)
		{
			if (pGameWin->m_bLoadDone && pGameWin->m_bRegistered)
			{
				if (StartInject(pGameWin->m_hWnd, m_hWnd))
				{
					pGameWin->m_bAttached = TRUE;
					::PostMessage(pGameWin->m_hWnd, WM_HOOK_EXDATA, (WPARAM)(&theGameData) ^ CLIENT_SERIAL, sizeof(GameData));
					pGameWin->SetGameParams();
					pGameWin->SaveProfile();
				}
			}
			else
				g_pCharList->SetItemState(pNMLV->iItem, LVIS_UNCHECKED, LVIS_STATEIMAGEMASK);
		}
		else
		{
			StopInject(pGameWin->m_hWnd);
			pGameWin->m_bAttached = 0;
			pGameWin->SaveProfile();
		}
	}
}

void CVLAutoDlg::OnNMDblclkCharList(NMHDR* pNMHDR, LRESULT* pResult)
{
	*pResult = 0;

	LPNMLISTVIEW pNMLV = reinterpret_cast<LPNMLISTVIEW>(pNMHDR);
	if (pNMLV->iItem == -1 || pNMLV->iSubItem == -1)
		return;

	GameWindow* pGameWin = (GameWindow*)g_pCharList->GetItemData(pNMLV->iItem);
	if (pGameWin == NULL || pGameWin->m_hWnd == NULL)
		return;

	HWND hGameWnd = pGameWin->m_hWnd;

	if (::IsIconic(hGameWnd) || !::IsWindowVisible(hGameWnd))
	{
		::ShowWindow(hGameWnd, SW_SHOW);
		::ShowWindow(hGameWnd, SW_RESTORE);
	}
	else
	{
		::ShowWindow(hGameWnd, SW_MINIMIZE);
		::ShowWindow(hGameWnd, SW_HIDE);
	}
}

void CVLAutoDlg::OnCustomdrawCharList(NMHDR* pNMHDR, LRESULT* pResult)
{
    *pResult = 0;

	NMLVCUSTOMDRAW* pLVCD = reinterpret_cast<NMLVCUSTOMDRAW*>(pNMHDR);

	if (CDDS_PREPAINT == pLVCD->nmcd.dwDrawStage)
		*pResult = CDRF_NOTIFYITEMDRAW;
	else if (CDDS_ITEMPREPAINT == pLVCD->nmcd.dwDrawStage)
	{
		GameWindow* pGameWin = (GameWindow*)pLVCD->nmcd.lItemlParam;
		if (pGameWin == NULL) return;

		COLORREF crText = RGB(128, 128, 128);
		if (pGameWin->m_bLoadDone)
		{
			if (pGameWin->m_bRegistered)
				crText = RGB(0, 0, 255);
			else
				crText = RGB(255, 0, 0);
		}

		pLVCD->clrText = crText;
		*pResult = CDRF_DODEFAULT;
	}
}

LRESULT CVLAutoDlg::OnReceiveFeedBack(WPARAM wParam, LPARAM lParam)
{
	int nCharCount = g_pCharList->GetItemCount();
	GameWindow* pGameWin = NULL;
	int nCurIndex = -1;
	HWND hGameWnd = NULL;

	for (int i = 0; i < nCharCount; i++)
	{
		pGameWin = (GameWindow*)g_pCharList->GetItemData(i);
		if (pGameWin->m_hWnd == (HWND)lParam)
		{
			nCurIndex = i;
			hGameWnd = pGameWin->m_hWnd;
			break;
		}
	}

	switch (wParam)
	{
	case cmd_amulet:
		PlaySound(MAKEINTRESOURCE(IDR_EXIT_WAVE), AfxGetApp()->m_hInstance, SND_RESOURCE | SND_ASYNC);
		ShowWindow(SW_RESTORE);
		SetForegroundWindow();
		break;

	case cmd_useam:		// Dem cho bang Thong ke (Auto Login)
		if (nCurIndex >= 0)
			pGameWin->m_nAMUsed++;
		break;

	case cmd_autohit:
		if (pGameWin != NULL)
		{
			CDlgAttack* pTabDlg = (CDlgAttack*)m_MainTab.GetTabDlg(tab_attack);
			pGameWin->m_prAttack.m_bAutoHit = !pGameWin->m_prAttack.m_bAutoHit;
			pGameWin->SetGameParams(tab_attack);
			pTabDlg->UpdateData(0);
		}
		break;

	case cmd_autoarng:
		if (pGameWin != NULL)
		{
			CDlgCollect* pTabDlg = (CDlgCollect*)m_MainTab.GetTabDlg(tab_collect);
			pGameWin->m_prCollect.m_bAutoArrange = !pGameWin->m_prCollect.m_bAutoArrange;
			pGameWin->SetGameParams(tab_collect);
			pTabDlg->UpdateData(0);
		}
		break;

	case cmd_setpoint:
		if (pGameWin != NULL)
		{
			CDlgMove* pTabDlg = (CDlgMove*)m_MainTab.GetTabDlg(tab_move);
			if (pGameWin->m_prMove.m_bAroundPoint)
			{
				pGameWin->m_prMove.m_nPointX = pGameWin->m_nOffX;
				pGameWin->m_prMove.m_nPointY = pGameWin->m_nOffY;
			}
			else if (pGameWin->m_prMove.m_bRouteMove)
			{
				for (int i = 0; i < MAX_NODES; i++)
				{
					if (pGameWin->m_prMove.m_Nodes[i].x &&
						pGameWin->m_prMove.m_Nodes[i].y) continue;
					if (pGameWin->m_nFlagX > 0 && pGameWin->m_nFlagY > 0)
					{
						pGameWin->m_prMove.m_Nodes[i].x = pGameWin->m_nFlagX;
						pGameWin->m_prMove.m_Nodes[i].y = pGameWin->m_nFlagY;
					}
					else
					{
						pGameWin->m_prMove.m_Nodes[i].x = pGameWin->m_nOffX;
						pGameWin->m_prMove.m_Nodes[i].y = pGameWin->m_nOffY;
					}
					break;
				}
			}
			pTabDlg->UpdateData(0);
			break;
		}
		break;

	case cmd_setmap:
		if (pGameWin != NULL)
		{
			CDlgMap* pTabDlg = (CDlgMap*)m_MainTab.GetTabDlg(tab_map);
			pGameWin->m_prMap.m_bAutoReturn = !pGameWin->m_prMap.m_bAutoReturn;
			pGameWin->SetGameParams(tab_map);
			pTabDlg->UpdateData(0);
		}
		break;

	case cmd_setyesou:
		if (pGameWin != NULL)
		{
			CDlgYeSou* pTabDlg = (CDlgYeSou*)m_MainTab.GetTabDlg(tab_yesou);
			pGameWin->m_prYeSou.m_bAutoDoQuest = !pGameWin->m_prYeSou.m_bAutoDoQuest;
			pGameWin->SetGameParams(tab_yesou);
			pTabDlg->UpdateData(0);
		}
		break;

	case cmd_activate:
		if (pGameWin != NULL)
			g_pCharList->SetItemState(nCurIndex, LVIS_SELECTED, LVIS_SELECTED);
		break;

	case cmd_exitall:
		for (int i = 0; i < nCharCount; i++)
		{
			GameWindow* pExitWin = (GameWindow*)g_pCharList->GetItemData(i);
			::PostMessage(pExitWin->m_hWnd, WM_HOOK_MAIN, cmd_exit, 0);
		}
		if (lParam == 1)
			OnShutDown();		// Shutdown PC
		else if (lParam == 2)
			OnShutDown(TRUE);	// Restart PC
		break;

	case cmd_stop:
		if (pGameWin != NULL)
			g_pCharList->SetItemState(nCurIndex, LVIS_UNCHECKED, LVIS_STATEIMAGEMASK);
		break;

	case cmd_start:
		if (pGameWin != NULL)
			pGameWin->m_nAttachTimeCount = 6;	// CountDownTime
		break;

	case cmd_restart:
		if (pGameWin != NULL)
		{
			g_pCharList->SetItemState(nCurIndex, LVIS_UNCHECKED, LVIS_STATEIMAGEMASK);
			pGameWin->m_nAttachTimeCount = 6;	// CountDownTime
		}
		break;

	case cmd_beginpk:
		if (pGameWin != NULL)
		{
			CDlgYeSou* pTabDlg = (CDlgYeSou*)m_MainTab.GetTabDlg(tab_yesou);
			for (int i = 0; i < nCharCount; i++)
			{
				if (i == nCurIndex) continue;
				GameWindow* pVictimWin = (GameWindow*)g_pCharList->GetItemData(i);
				if (!pGameWin->m_hWnd) continue;
				if (!strcmp(pVictimWin->m_szPlayerName, pGameWin->m_prYeSou.m_TongPKOpt.m_szVictimName) && // Found victim
					!pVictimWin->m_bAttached)
				{
					pVictimWin->m_nAttachTimeCount = 6;
					return 0;
				}
			}
			if (pGameWin->m_prYeSou.m_TongPKOpt.m_bAutoLogin)
			{
				HWND hAutoLoginWnd = ::FindWindow(_TVLGWinClass, NULL);
				if (hAutoLoginWnd)
					::PostMessage(hAutoLoginWnd, WM_FEEDBACK, cmd_beginpk, (LPARAM)pGameWin->m_prYeSou.m_TongPKOpt.m_nAutoLoginIdx);
			}
		}
		break;

	case cmd_endpk:
		if (pGameWin != NULL)
		{
			CDlgYeSou* pTabDlg = (CDlgYeSou*)m_MainTab.GetTabDlg(tab_yesou);
			if (pGameWin->m_prYeSou.m_TongPKOpt.m_bExitVictim)
			{
				for (int i = 0; i < nCharCount; i++)
				{
					if (i == nCurIndex) continue;
					GameWindow* pVictimWin = (GameWindow*)g_pCharList->GetItemData(i);
					if (!pVictimWin->m_hWnd) continue;

					if (!strcmp(pVictimWin->m_szPlayerName, pGameWin->m_prYeSou.m_TongPKOpt.m_szVictimName))
					{
						::PostMessage(pVictimWin->m_hWnd, WM_HOOK_MAIN, cmd_exit, 0);
						break;
					}
				}
			}
		}
		break;

	case cmd_showme:
		m_TrayIcon.SetIcon(m_hIcon, _TTrayToolTip);
		break;

	case cmd_attack:
		SendAttackCommand((HWND)lParam);
		break;
	}

	return 0;
}

void CVLAutoDlg::OnShutDown(BOOL bRestart)
{
	static BOOL bShuttingDown = 0;
	if (bShuttingDown)
		return;

	bShuttingDown = TRUE;
	ShowWindow(SW_RESTORE);
	SetForegroundWindow();
	CExitDialog ExitDlg;
	if (ExitDlg.DoModal() != IDOK)
	{
		bShuttingDown = 0;
		return;
	}

	HANDLE hToken;
	TOKEN_PRIVILEGES tkp;
	OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES|TOKEN_QUERY, &hToken);
	LookupPrivilegeValue(NULL, SE_SHUTDOWN_NAME, &tkp.Privileges[0].Luid);
	tkp.PrivilegeCount = 1;
	tkp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
	AdjustTokenPrivileges(hToken, FALSE, &tkp, 0, (PTOKEN_PRIVILEGES)NULL, 0);
	UINT uExiFlag = EWX_SHUTDOWN|EWX_POWEROFF|EWX_FORCE;

	if (bRestart) uExiFlag = EWX_REBOOT|EWX_FORCE;
	ExitWindowsEx(uExiFlag,
		SHTDN_REASON_MAJOR_APPLICATION| SHTDN_REASON_MINOR_MAINTENANCE| SHTDN_REASON_FLAG_PLANNED);

	CDialog::OnCancel();
}
