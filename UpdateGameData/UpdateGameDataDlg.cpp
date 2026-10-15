// UpdateGameDataDlg.cpp : cua so chinh
#include "stdafx.h"
#include "UpdateGameData.h"
#include "UpdateGameDataDlg.h"

#define INI_SECTION		_T("UpdateGameData")

// Cot cua danh sach ket qua
enum { COL_NAME, COL_KIND, COL_OLD, COL_NEW, COL_VOTES, COL_STATUS, COL_POS, COL_DETAIL, COL_COMMENT };

CUpdateGameDataDlg::CUpdateGameDataDlg(CWnd* pParent)
	: CDialogEx(IDD_UPDATEGAMEDATA_DIALOG, pParent)
	, m_bAnalyzed(FALSE)
	, m_pThread(NULL)
	, m_lCancel(0)
	, m_nListMarginR(0), m_nListMarginB(0), m_nStatusMarginR(0)
{
}

void CUpdateGameDataDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_LIST_FILES, m_lstFiles);
	DDX_Control(pDX, IDC_LIST_RESULT, m_lstResult);
	DDX_Control(pDX, IDC_PROGRESS, m_Progress);
}

BEGIN_MESSAGE_MAP(CUpdateGameDataDlg, CDialogEx)
	ON_WM_SIZE()
	ON_WM_GETMINMAXINFO()
	ON_BN_CLICKED(IDC_BTN_REF, &CUpdateGameDataDlg::OnBnClickedBtnRef)
	ON_BN_CLICKED(IDC_BTN_TGT, &CUpdateGameDataDlg::OnBnClickedBtnTgt)
	ON_BN_CLICKED(IDC_BTN_ADDFILE, &CUpdateGameDataDlg::OnBnClickedBtnAddFile)
	ON_BN_CLICKED(IDC_BTN_DELFILE, &CUpdateGameDataDlg::OnBnClickedBtnDelFile)
	ON_BN_CLICKED(IDC_BTN_OUT, &CUpdateGameDataDlg::OnBnClickedBtnOut)
	ON_BN_CLICKED(IDC_BTN_RUN, &CUpdateGameDataDlg::OnBnClickedBtnRun)
	ON_BN_CLICKED(IDC_BTN_WRITE, &CUpdateGameDataDlg::OnBnClickedBtnWrite)
	ON_BN_CLICKED(IDC_BTN_REPORT, &CUpdateGameDataDlg::OnBnClickedBtnReport)
	ON_BN_CLICKED(IDC_CHK_SHOWSAME, &CUpdateGameDataDlg::OnBnClickedChkShowSame)
	ON_NOTIFY(NM_CUSTOMDRAW, IDC_LIST_RESULT, &CUpdateGameDataDlg::OnNMCustomDrawListResult)
	ON_NOTIFY(NM_DBLCLK, IDC_LIST_RESULT, &CUpdateGameDataDlg::OnNMDblclkListResult)
	ON_MESSAGE(WM_APP_PROGRESS, &CUpdateGameDataDlg::OnAppProgress)
	ON_MESSAGE(WM_APP_DONE, &CUpdateGameDataDlg::OnAppDone)
END_MESSAGE_MAP()

BOOL CUpdateGameDataDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	CRect rcWnd, rcClient, rc;
	GetWindowRect(&rcWnd);
	m_szMin = rcWnd.Size();
	GetClientRect(&rcClient);
	m_lstResult.GetWindowRect(&rc);
	ScreenToClient(&rc);
	m_nListMarginR = rcClient.right - rc.right;
	m_nListMarginB = rcClient.bottom - rc.bottom;
	GetDlgItem(IDC_STATUS)->GetWindowRect(&rc);
	ScreenToClient(&rc);
	m_nStatusMarginR = rcClient.right - rc.right;

	m_lstResult.SetExtendedStyle(LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES | LVS_EX_DOUBLEBUFFER);
	m_lstResult.InsertColumn(COL_NAME, _T("Tên"), LVCFMT_LEFT, 140);
	m_lstResult.InsertColumn(COL_KIND, _T("Loại"), LVCFMT_LEFT, 65);
	m_lstResult.InsertColumn(COL_OLD, _T("Cũ"), LVCFMT_LEFT, 80);
	m_lstResult.InsertColumn(COL_NEW, _T("Mới"), LVCFMT_LEFT, 80);
	m_lstResult.InsertColumn(COL_VOTES, _T("Phiếu"), LVCFMT_RIGHT, 50);
	m_lstResult.InsertColumn(COL_STATUS, _T("Trạng thái"), LVCFMT_LEFT, 95);
	m_lstResult.InsertColumn(COL_POS, _T("File:dòng"), LVCFMT_LEFT, 110);
	m_lstResult.InsertColumn(COL_DETAIL, _T("Chi tiết"), LVCFMT_LEFT, 300);
	m_lstResult.InsertColumn(COL_COMMENT, _T("Ghi chú trong .h"), LVCFMT_LEFT, 300);
	m_Progress.SetRange32(0, 100);

	LoadSettings();
	EnableControls(TRUE);
	SetDlgItemText(IDC_STATUS, _T("Chọn bản cũ (địa chỉ trong .h đang đúng với bản này), bản mới, rồi bấm Phân tích."));
	return TRUE;
}

CString CUpdateGameDataDlg::ExeDir()
{
	TCHAR szPath[MAX_PATH];
	GetModuleFileName(NULL, szPath, MAX_PATH);
	CString s = szPath;
	return s.Left(s.ReverseFind(_T('\\')));
}

static CString FullPath(LPCTSTR lpszPath)
{
	TCHAR szFull[MAX_PATH];
	if (GetFullPathName(lpszPath, MAX_PATH, szFull, NULL)) return szFull;
	return lpszPath;
}

void CUpdateGameDataDlg::LoadSettings()
{
	CString strIni = IniPath();
	TCHAR szBuf[8192];
	GetPrivateProfileString(INI_SECTION, _T("Ref"), _T(""), szBuf, _countof(szBuf), strIni);
	SetDlgItemText(IDC_EDIT_REF, szBuf);
	GetPrivateProfileString(INI_SECTION, _T("Tgt"), _T(""), szBuf, _countof(szBuf), strIni);
	SetDlgItemText(IDC_EDIT_TGT, szBuf);

	// Mac dinh: thu muc giai phap = thu muc cha cua _Bin
	CString strSln = FullPath(ExeDir() + _T("\\.."));
	GetPrivateProfileString(INI_SECTION, _T("Out"), strSln + _T("\\_Update"), szBuf, _countof(szBuf), strIni);
	SetDlgItemText(IDC_EDIT_OUT, szBuf);

	GetPrivateProfileString(INI_SECTION, _T("Files"), _T(""), szBuf, _countof(szBuf), strIni);
	CString strFiles = szBuf;
	if (strFiles.IsEmpty())
	{
		static LPCTSTR s_szDefault[] = { _T("\\VLHook\\GameBase.h"), _T("\\VLAuto\\GameCore.h"), _T("\\VLAuto\\GameLogin.h") };
		for (int i = 0; i < _countof(s_szDefault); i++)
		{
			CString s = strSln + s_szDefault[i];
			if (GetFileAttributes(s) != INVALID_FILE_ATTRIBUTES) m_lstFiles.AddString(s);
		}
	}
	else
	{
		int nPos = 0;
		CString s = strFiles.Tokenize(_T("|"), nPos);
		while (!s.IsEmpty())
		{
			m_lstFiles.AddString(s);
			s = strFiles.Tokenize(_T("|"), nPos);
		}
	}
	m_lstFiles.SetHorizontalExtent(1200);
	CheckDlgButton(IDC_CHK_SHOWSAME, GetPrivateProfileInt(INI_SECTION, _T("ShowSame"), 0, strIni) ? BST_CHECKED : BST_UNCHECKED);
}

void CUpdateGameDataDlg::SaveSettings()
{
	CString strIni = IniPath(), s;
	GetDlgItemText(IDC_EDIT_REF, s);
	WritePrivateProfileString(INI_SECTION, _T("Ref"), s, strIni);
	GetDlgItemText(IDC_EDIT_TGT, s);
	WritePrivateProfileString(INI_SECTION, _T("Tgt"), s, strIni);
	GetDlgItemText(IDC_EDIT_OUT, s);
	WritePrivateProfileString(INI_SECTION, _T("Out"), s, strIni);
	std::vector<CString> files;
	GetFileList(files);
	s.Empty();
	for (size_t i = 0; i < files.size(); i++) s += (i ? _T("|") : _T("")) + files[i];
	WritePrivateProfileString(INI_SECTION, _T("Files"), s, strIni);
	WritePrivateProfileString(INI_SECTION, _T("ShowSame"), IsDlgButtonChecked(IDC_CHK_SHOWSAME) ? _T("1") : _T("0"), strIni);
}

void CUpdateGameDataDlg::GetFileList(std::vector<CString> &files)
{
	files.clear();
	for (int i = 0; i < m_lstFiles.GetCount(); i++)
	{
		CString s;
		m_lstFiles.GetText(i, s);
		files.push_back(s);
	}
}

void CUpdateGameDataDlg::EnableControls(BOOL bEnable)
{
	static const UINT s_nIDs[] = { IDC_EDIT_REF, IDC_BTN_REF, IDC_EDIT_TGT, IDC_BTN_TGT, IDC_LIST_FILES, IDC_BTN_ADDFILE,
		IDC_BTN_DELFILE, IDC_EDIT_OUT, IDC_BTN_OUT, IDC_CHK_OVERWRITE };
	for (int i = 0; i < _countof(s_nIDs); i++) GetDlgItem(s_nIDs[i])->EnableWindow(bEnable);
	SetDlgItemText(IDC_BTN_RUN, bEnable ? _T("Phân tích") : _T("Dừng"));
	GetDlgItem(IDC_BTN_WRITE)->EnableWindow(bEnable && m_bAnalyzed);
	GetDlgItem(IDC_BTN_REPORT)->EnableWindow(bEnable && m_bAnalyzed);
}

void CUpdateGameDataDlg::OnSize(UINT nType, int cx, int cy)
{
	CDialogEx::OnSize(nType, cx, cy);
	if (!m_lstResult.GetSafeHwnd() || nType == SIZE_MINIMIZED) return;
	CRect rc;
	m_lstResult.GetWindowRect(&rc);
	ScreenToClient(&rc);
	m_lstResult.MoveWindow(rc.left, rc.top, max(50, cx - m_nListMarginR - rc.left), max(50, cy - m_nListMarginB - rc.top));
	CWnd *pStatus = GetDlgItem(IDC_STATUS);
	pStatus->GetWindowRect(&rc);
	ScreenToClient(&rc);
	pStatus->MoveWindow(rc.left, rc.top, max(50, cx - m_nStatusMarginR - rc.left), rc.Height());
	Invalidate(FALSE);
}

void CUpdateGameDataDlg::OnGetMinMaxInfo(MINMAXINFO* lpMMI)
{
	if (m_szMin.cx)
	{
		lpMMI->ptMinTrackSize.x = m_szMin.cx;
		lpMMI->ptMinTrackSize.y = m_szMin.cy;
	}
	CDialogEx::OnGetMinMaxInfo(lpMMI);
}

void CUpdateGameDataDlg::BrowseExe(UINT nID)
{
	CString s;
	GetDlgItemText(nID, s);
	CFileDialog dlg(TRUE, _T("exe"), s, OFN_FILEMUSTEXIST | OFN_HIDEREADONLY,
		_T("File game (*.exe;*.dll)|*.exe;*.dll|Tất cả (*.*)|*.*||"), this);
	if (dlg.DoModal() == IDOK) SetDlgItemText(nID, dlg.GetPathName());
}

void CUpdateGameDataDlg::OnBnClickedBtnRef()
{
	BrowseExe(IDC_EDIT_REF);
}

void CUpdateGameDataDlg::OnBnClickedBtnTgt()
{
	BrowseExe(IDC_EDIT_TGT);
}

void CUpdateGameDataDlg::OnBnClickedBtnAddFile()
{
	CFileDialog dlg(TRUE, _T("h"), NULL, OFN_FILEMUSTEXIST | OFN_HIDEREADONLY | OFN_ALLOWMULTISELECT,
		_T("File header (*.h)|*.h|Tất cả (*.*)|*.*||"), this);
	TCHAR szBuf[16384] = { 0 };
	dlg.m_ofn.lpstrFile = szBuf;
	dlg.m_ofn.nMaxFile = _countof(szBuf);
	if (dlg.DoModal() != IDOK) return;
	POSITION pos = dlg.GetStartPosition();
	while (pos)
	{
		CString s = dlg.GetNextPathName(pos);
		if (m_lstFiles.FindStringExact(-1, s) == LB_ERR) m_lstFiles.AddString(s);
	}
}

void CUpdateGameDataDlg::OnBnClickedBtnDelFile()
{
	for (int i = m_lstFiles.GetCount() - 1; i >= 0; i--)
		if (m_lstFiles.GetSel(i) > 0) m_lstFiles.DeleteString(i);
}

void CUpdateGameDataDlg::OnBnClickedBtnOut()
{
	CString s;
	GetDlgItemText(IDC_EDIT_OUT, s);
	CFolderPickerDialog dlg(s, 0, this);
	if (dlg.DoModal() == IDOK) SetDlgItemText(IDC_EDIT_OUT, dlg.GetPathName());
}

//////////////////////////////////////////////////////////////////////////
// Phan tich (chay trong thread rieng)

void CUpdateGameDataDlg::OnBnClickedBtnRun()
{
	if (m_pThread)
	{
		InterlockedExchange(&m_lCancel, 1);
		SetDlgItemText(IDC_STATUS, _T("Đang dừng..."));
		return;
	}
	GetDlgItemText(IDC_EDIT_REF, m_strRef);
	GetDlgItemText(IDC_EDIT_TGT, m_strTgt);
	m_strRef.Trim();
	m_strTgt.Trim();
	GetFileList(m_ThreadFiles);
	if (m_strRef.IsEmpty() || m_strTgt.IsEmpty())
	{
		AfxMessageBox(_T("Chưa chọn file game bản cũ / bản mới."), MB_ICONWARNING);
		return;
	}
	if (m_ThreadFiles.empty())
	{
		AfxMessageBox(_T("Chưa có file .h chứa địa chỉ."), MB_ICONWARNING);
		return;
	}
	SaveSettings();

	m_bAnalyzed = FALSE;
	m_lstResult.DeleteAllItems();
	m_Progress.SetPos(0);
	m_lCancel = 0;
	EnableControls(FALSE);
	SetDlgItemText(IDC_STATUS, _T("Đang nạp file game và lập chỉ mục lệnh..."));
	m_pThread = AfxBeginThread(ThreadProc, this, THREAD_PRIORITY_NORMAL, 0, CREATE_SUSPENDED);
	m_pThread->m_bAutoDelete = FALSE;
	m_pThread->ResumeThread();
}

UINT CUpdateGameDataDlg::ThreadProc(LPVOID pParam)
{
	CUpdateGameDataDlg *pDlg = (CUpdateGameDataDlg*)pParam;
	HWND hWnd = pDlg->GetSafeHwnd();
	CString strErr;
	BOOL bOK = pDlg->m_Diff.LoadImages(pDlg->m_strRef, pDlg->m_strTgt, strErr) &&
		pDlg->m_Diff.ParseHeaders(pDlg->m_ThreadFiles, strErr);
	if (bOK)
	{
		pDlg->m_Diff.Resolve([hWnd](int i, int n) { ::PostMessage(hWnd, WM_APP_PROGRESS, i, n); }, &pDlg->m_lCancel);
		if (pDlg->m_lCancel)
		{
			bOK = FALSE;
			strErr = _T("Đã dừng.");
		}
	}
	pDlg->m_strThreadErr = strErr;
	::PostMessage(hWnd, WM_APP_DONE, bOK, 0);
	return 0;
}

LRESULT CUpdateGameDataDlg::OnAppProgress(WPARAM wParam, LPARAM lParam)
{
	int i = (int)wParam, n = (int)lParam;
	if (n <= 0) return 0;
	m_Progress.SetPos(i * 100 / n);
	CString s;
	if (i < n && i < (int)m_Diff.m_Defines.size())
		s.Format(_T("Đang dò %d/%d: %s"), i + 1, n, (LPCTSTR)CString(m_Diff.m_Defines[i].strName));
	else s.Format(_T("Đã dò %d/%d"), n, n);
	SetDlgItemText(IDC_STATUS, s);
	return 0;
}

LRESULT CUpdateGameDataDlg::OnAppDone(WPARAM wParam, LPARAM lParam)
{
	if (m_pThread)
	{
		WaitForSingleObject(m_pThread->m_hThread, INFINITE);
		delete m_pThread;
		m_pThread = NULL;
	}
	m_bAnalyzed = (BOOL)wParam;
	EnableControls(TRUE);
	if (!m_bAnalyzed)
	{
		m_Progress.SetPos(0);
		SetDlgItemText(IDC_STATUS, m_strThreadErr);
		if (!m_lCancel) AfxMessageBox(m_strThreadErr, MB_ICONERROR);
		return 0;
	}
	m_Progress.SetPos(100);
	FillResult();

	int nCount[GDS_NOTFOUND + 1] = { 0 };
	for (auto &d : m_Diff.m_Defines) nCount[d.nStatus]++;
	CString s;
	s.Format(_T("Tổng %d: Không đổi %d, Đã đổi %d, Suy ra %d, Nhiều ứng viên %d, Cần kiểm tra %d, Không tìm thấy %d, Bỏ qua %d"),
		(int)m_Diff.m_Defines.size(), nCount[GDS_SAME], nCount[GDS_CHANGED], nCount[GDS_DERIVED], nCount[GDS_AMBIG], nCount[GDS_SUSPECT],
		nCount[GDS_NOTFOUND], nCount[GDS_SKIP]);
	SetDlgItemText(IDC_STATUS, s);
	return 0;
}

// Chu thich trong .h: thu UTF-8, khong duoc thi dung bang ma ANSI
static CString DecodeComment(const CStringA &s)
{
	if (s.IsEmpty()) return CString();
	int n = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, s, s.GetLength(), NULL, 0);
	UINT nCP = n ? CP_UTF8 : CP_ACP;
	n = MultiByteToWideChar(nCP, 0, s, s.GetLength(), NULL, 0);
	CString w;
	MultiByteToWideChar(nCP, 0, s, s.GetLength(), w.GetBuffer(n), n);
	w.ReleaseBuffer(n);
	return w;
}

void CUpdateGameDataDlg::FillResult()
{
	BOOL bShowSame = IsDlgButtonChecked(IDC_CHK_SHOWSAME);
	m_lstResult.SetRedraw(FALSE);
	m_lstResult.DeleteAllItems();
	int nItem = 0;
	for (size_t i = 0; i < m_Diff.m_Defines.size(); i++)
	{
		const GD_DEFINE &d = m_Diff.m_Defines[i];
		if (!bShowSame && (d.nStatus == GDS_SAME || d.nStatus == GDS_SKIP)) continue;
		CString s;
		m_lstResult.InsertItem(nItem, CString(d.strName));
		m_lstResult.SetItemData(nItem, i);
		m_lstResult.SetItemText(nItem, COL_KIND, CGameDiff::KindName(d.nKind));
		s.Format(_T("0x%08X"), d.dwOld);
		m_lstResult.SetItemText(nItem, COL_OLD, s);
		if (d.nStatus != GDS_SKIP && d.nStatus != GDS_NOTFOUND && d.nStatus != GDS_AMBIG)
		{
			s.Format(_T("0x%08X"), d.dwNew);
			m_lstResult.SetItemText(nItem, COL_NEW, s);
		}
		if (d.nTotal)
		{
			s.Format(_T("%d/%d"), d.nVotes, d.nTotal);
			m_lstResult.SetItemText(nItem, COL_VOTES, s);
		}
		m_lstResult.SetItemText(nItem, COL_STATUS, CGameDiff::StatusName(d.nStatus));
		CString strFile = m_Diff.m_Files[d.nFile].strPath;
		s.Format(_T("%s:%d"), (LPCTSTR)strFile.Mid(strFile.ReverseFind(_T('\\')) + 1), d.nLine + 1);
		m_lstResult.SetItemText(nItem, COL_POS, s);
		m_lstResult.SetItemText(nItem, COL_DETAIL, d.strDetail);
		m_lstResult.SetItemText(nItem, COL_COMMENT, DecodeComment(d.strComment));
		nItem++;
	}
	m_lstResult.SetRedraw(TRUE);
	m_lstResult.Invalidate();
}

void CUpdateGameDataDlg::OnBnClickedChkShowSame()
{
	if (m_bAnalyzed) FillResult();
}

void CUpdateGameDataDlg::OnNMCustomDrawListResult(NMHDR *pNMHDR, LRESULT *pResult)
{
	NMLVCUSTOMDRAW *pCD = (NMLVCUSTOMDRAW*)pNMHDR;
	*pResult = CDRF_DODEFAULT;
	if (pCD->nmcd.dwDrawStage == CDDS_PREPAINT)
	{
		*pResult = CDRF_NOTIFYITEMDRAW;
		return;
	}
	if (pCD->nmcd.dwDrawStage != CDDS_ITEMPREPAINT) return;
	size_t i = (size_t)pCD->nmcd.lItemlParam;
	if (i >= m_Diff.m_Defines.size()) return;
	switch (m_Diff.m_Defines[i].nStatus)
	{
	case GDS_CHANGED:	pCD->clrTextBk = RGB(220, 245, 220); break;
	case GDS_DERIVED:	pCD->clrTextBk = RGB(255, 250, 210); break;
	case GDS_AMBIG:
	case GDS_SUSPECT:	pCD->clrTextBk = RGB(255, 228, 196); break;
	case GDS_NOTFOUND:	pCD->clrTextBk = RGB(255, 215, 215); break;
	case GDS_SKIP:		pCD->clrText = RGB(140, 140, 140); break;
	}
}

void CUpdateGameDataDlg::OnNMDblclkListResult(NMHDR *pNMHDR, LRESULT *pResult)
{
	*pResult = 0;
	int nItem = ((NMITEMACTIVATE*)pNMHDR)->iItem;
	if (nItem < 0) return;
	size_t i = m_lstResult.GetItemData(nItem);
	if (i >= m_Diff.m_Defines.size()) return;
	const GD_DEFINE &d = m_Diff.m_Defines[i];
	CString s;
	s.Format(_T("%s\n%s:%d\n\nLoại: %s\nCũ: 0x%08X\nMới: 0x%08X\nTrạng thái: %s (%d/%d phiếu)\n\n%s\n\nGhi chú: %s"),
		(LPCTSTR)CString(d.strName), (LPCTSTR)m_Diff.m_Files[d.nFile].strPath, d.nLine + 1, CGameDiff::KindName(d.nKind),
		d.dwOld, d.dwNew, CGameDiff::StatusName(d.nStatus), d.nVotes, d.nTotal, (LPCTSTR)d.strDetail, (LPCTSTR)DecodeComment(d.strComment));
	AfxMessageBox(s, MB_ICONINFORMATION);
}

//////////////////////////////////////////////////////////////////////////
// Ghi ket qua

void CUpdateGameDataDlg::OnBnClickedBtnWrite()
{
	if (!m_bAnalyzed) return;
	CString strOut;
	GetDlgItemText(IDC_EDIT_OUT, strOut);
	strOut.Trim();
	strOut.TrimRight(_T('\\'));
	BOOL bOverwrite = IsDlgButtonChecked(IDC_CHK_OVERWRITE);
	if (!bOverwrite && strOut.IsEmpty())
	{
		AfxMessageBox(_T("Chưa chọn thư mục kết quả."), MB_ICONWARNING);
		return;
	}
	int nAmbig = 0, nNotFound = 0;
	for (auto &d : m_Diff.m_Defines)
	{
		if (d.nStatus == GDS_AMBIG || d.nStatus == GDS_SUSPECT) nAmbig++;
		else if (d.nStatus == GDS_NOTFOUND) nNotFound++;
	}
	CString s;
	s.Format(_T("Các mục Nhiều ứng viên / Cần kiểm tra (%d) và Không tìm thấy (%d) sẽ giữ nguyên giá trị cũ.\n\n%s\n\nTiếp tục?"), nAmbig, nNotFound,
		bOverwrite ? _T("GHI ĐÈ trực tiếp các file .h gốc (bản cũ lưu thành .bak).") : (LPCTSTR)(_T("Ghi file .h mới vào: ") + strOut));
	if (AfxMessageBox(s, MB_YESNO | (bOverwrite ? MB_ICONWARNING : MB_ICONQUESTION)) != IDYES) return;
	SaveSettings();

	CString strResult, strErr;
	if (!m_Diff.WriteHeaders(strOut, bOverwrite, strResult))
	{
		AfxMessageBox(strResult, MB_ICONERROR);
		return;
	}
	if (!strOut.IsEmpty())
	{
		SHCreateDirectoryEx(NULL, strOut, NULL);
		if (m_Diff.WriteReport(strOut + _T("\\BaoCao.txt"), strErr)) strResult += _T("\r\nBáo cáo: ") + strOut + _T("\\BaoCao.txt");
	}
	AfxMessageBox(strResult, MB_ICONINFORMATION);
}

void CUpdateGameDataDlg::OnBnClickedBtnReport()
{
	if (!m_bAnalyzed) return;
	CString strOut;
	GetDlgItemText(IDC_EDIT_OUT, strOut);
	strOut.TrimRight(_T('\\'));
	CFileDialog dlg(FALSE, _T("txt"), strOut + _T("\\BaoCao.txt"), OFN_OVERWRITEPROMPT | OFN_HIDEREADONLY,
		_T("Báo cáo (*.txt)|*.txt||"), this);
	if (dlg.DoModal() != IDOK) return;
	CString strErr;
	if (!m_Diff.WriteReport(dlg.GetPathName(), strErr)) AfxMessageBox(strErr, MB_ICONERROR);
}

void CUpdateGameDataDlg::OnOK()
{
	// Enter: khong dong cua so
}

void CUpdateGameDataDlg::OnCancel()
{
	if (m_pThread)
	{
		InterlockedExchange(&m_lCancel, 1);
		WaitForSingleObject(m_pThread->m_hThread, INFINITE);
		delete m_pThread;
		m_pThread = NULL;
	}
	SaveSettings();
	CDialogEx::OnCancel();
}
