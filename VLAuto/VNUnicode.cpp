#include "stdafx.h"
#include "VLAuto.h"

// ===== TCVN3 <-> Unicode, KHONG MAT DU LIEU =====
// Quy uoc: du lieu that (gui DLL, luu profile) luon la byte TCVN3 lay tu game.
// Unicode chi de HIEN THI. Byte TCVN3 ngoai bang (vd 0xD9 game dung trong ten "TN?So?33")
// duoc hien thi bang ky tu rieng U+F700+byte (vung Private Use) de doi nguoc dung byte cu.
// Ban cu doi byte la -> dau cach => luu lai thanh 0x20 => DLL so sai ten.

// Thu tu dau trong moi nhom: huyen, hoi, nga, sac, nang (giong VLHook/TcvnText.cpp)
static const wchar_t s_wUni[] =
	L"ăâêôơưđ" L"ĂÂÊÔƠƯĐ"
	L"àảãáạ" L"ằẳẵắặ" L"ầẩẫấậ" L"èẻẽéẹ" L"ềểễếệ" L"ìỉĩíị"
	L"òỏõóọ" L"ồổỗốộ" L"ờởỡớợ" L"ùủũúụ" L"ừửữứự" L"ỳỷỹýỵ"
	L"ÀẢÃÁẠ" L"ẰẲẴẮẶ" L"ẦẨẪẤẬ" L"ÈẺẼÉẸ" L"ỀỂỄẾỆ" L"ÌỈĨÍỊ"
	L"ÒỎÕÓỌ" L"ỒỔỖỐỘ" L"ỜỞỠỚỢ" L"ÙỦŨÚỤ" L"ỪỬỮỨỰ" L"ỲỶỸÝỴ";

// TCVN3 khong co chu HOA co dau thanh => dung chung ma chu thuong
#define TCVN3_TONED \
	"\xb5\xb6\xb7\xb8\xb9" "\xbb\xbc\xbd\xbe\xc6" "\xc7\xc8\xc9\xca\xcb" "\xcc\xce\xcf\xd0\xd1" \
	"\xd2\xd3\xd4\xd5\xd6" "\xd7\xd8\xdc\xdd\xde" "\xdf\xe1\xe2\xe3\xe4" "\xe5\xe6\xe7\xe8\xe9" \
	"\xea\xeb\xec\xed\xee" "\xef\xf1\xf2\xf3\xf4" "\xf5\xf6\xf7\xf8\xf9" "\xfa\xfb\xfc\xfd\xfe"

static const char s_szTcvn[] =
	"\xa8\xa9\xaa\xab\xac\xad\xae" "\xa1\xa2\xa3\xa4\xa5\xa6\xa7"
	TCVN3_TONED TCVN3_TONED;

C_ASSERT(sizeof(s_wUni) / sizeof(wchar_t) == sizeof(s_szTcvn));

#define TCVN_RAW_BASE	0xF700	// Byte TCVN3 la b (>= 0x80) <-> U+F700 + b

static unsigned char s_Map[0x1F00];	// U+0000..U+1EFF -> TCVN3 (0 = khong co)
static wchar_t s_Rev[0x80];			// TCVN3 0x80..0xFF -> Unicode (0 = ngoai bang)
static volatile LONG s_bMapReady = 0;

static void BuildMap()
{
	for (int i = 0; s_wUni[i]; i++)
	{
		unsigned char c = (unsigned char)s_szTcvn[i];
		s_Map[s_wUni[i]] = c;
		if (!s_Rev[c - 0x80]) s_Rev[c - 0x80] = s_wUni[i];	// ma dung chung: lay chu thuong (gap truoc)
	}
	InterlockedExchange(&s_bMapReady, 1);
}

// szUnicodeString phai chua duoc nMaxLen + 1 ky tu
void TCVN3ToUnicode(LPCSTR szTCVN3String, LPWSTR szUnicodeString, int nMaxLen)
{
	if (!s_bMapReady) BuildMap();
	int i;
	for (i = 0; szTCVN3String[i] && i < nMaxLen; i++)
	{
		unsigned char c = (unsigned char)szTCVN3String[i];
		if (c < 0x80)
			szUnicodeString[i] = c;
		else
			szUnicodeString[i] = s_Rev[c - 0x80] ? s_Rev[c - 0x80] : (wchar_t)(TCVN_RAW_BASE + c);
	}
	szUnicodeString[i] = 0;
}

// Ghep dau to hop (go Unikey kieu to hop / copy tu web) thanh chu dung san - dung NormalizeString neu co (Vista+)
static void NormalizeNFC(LPCWSTR szIn, CStringW& strOut)
{
	strOut = szIn;
	BOOL bCombining = FALSE;
	for (LPCWSTR p = szIn; *p; p++)
		if (*p >= 0x0300 && *p <= 0x036F) { bCombining = TRUE; break; }
	if (!bCombining) return;
	typedef int (WINAPI *PFN_NORMALIZE)(int, LPCWSTR, int, LPWSTR, int);
	static PFN_NORMALIZE pfn = (PFN_NORMALIZE)GetProcAddress(LoadLibraryW(L"Normaliz.dll"), "NormalizeString");
	if (!pfn) return;
	int n = pfn(1 /*NormalizationC*/, szIn, -1, NULL, 0);
	if (n <= 0) return;
	CStringW strTmp;
	n = pfn(1, szIn, -1, strTmp.GetBuffer(n), n);
	strTmp.ReleaseBuffer();
	if (n > 0) strOut = strTmp;
}

// szTCVN3String phai chua duoc nMaxLen + 1 byte
void UnicodeToTCVN3(LPCWSTR szUnicodeString, LPSTR szTCVN3String, int nMaxLen)
{
	if (!s_bMapReady) BuildMap();
	CStringW strNfc;
	NormalizeNFC(szUnicodeString, strNfc);
	LPCWSTR p = strNfc;
	int i;
	for (i = 0; p[i] && i < nMaxLen; i++)
	{
		wchar_t w = p[i];
		unsigned char c;
		if (w < 0x80) c = (unsigned char)w;
		else if (w >= TCVN_RAW_BASE + 0x80 && w <= TCVN_RAW_BASE + 0xFF) c = (unsigned char)(w - TCVN_RAW_BASE);	// byte goc tu game
		else if (w < 0x1F00 && s_Map[w]) c = s_Map[w];
		else if (w <= 0xFF) c = (unsigned char)w;	// Latin-1 ngoai bang: giu byte (text dan theo ANSI)
		else c = '?';
		szTCVN3String[i] = (char)c;
	}
	szTCVN3String[i] = 0;
}

// TRUE neu chuoi Unicode doi sang TCVN3 roi doi nguoc ma khong mat ky tu (dung de canh bao nguoi dung)
BOOL IsTCVN3Exact(LPCWSTR szUnicodeString)
{
	CStringW strNfc;
	NormalizeNFC(szUnicodeString, strNfc);
	int n = strNfc.GetLength();
	CStringA strA;
	UnicodeToTCVN3(strNfc, strA.GetBuffer(n + 1), n);
	strA.ReleaseBuffer(n);
	CStringW strBack;
	TCVN3ToUnicode(strA, strBack.GetBuffer(n + 1), n);
	strBack.ReleaseBuffer(n);
	// Chu HOA co dau (TCVN3 khong co) doi ve chu thuong: chap nhan neu khong phan biet hoa thuong
	return strBack.CompareNoCase(strNfc) == 0;
}

// ===== Dan (paste) ten copy tu game =====
// Game (ANSI) dat CF_TEXT = byte TCVN3; Windows tu sinh CF_UNICODETEXT theo ACP => sai chu.
// Lay byte goc roi doi sang Unicode hien thi bang bang TCVN3 o tren.
BOOL GetClipboardGameText(CStringW& strOut)
{
	strOut.Empty();
	if (!OpenClipboard(NULL)) return FALSE;
	// Dinh dang goc dung truoc dinh dang Windows tu sinh trong EnumClipboardFormats
	UINT uFirst = 0;
	for (UINT f = EnumClipboardFormats(0); f; f = EnumClipboardFormats(f))
		if (f == CF_TEXT || f == CF_UNICODETEXT || f == CF_OEMTEXT) { uFirst = f; break; }

	CStringA strRaw;
	BOOL bRaw = FALSE;
	if (uFirst == CF_TEXT || uFirst == CF_OEMTEXT)
	{
		HANDLE h = GetClipboardData(CF_TEXT);
		LPCSTR s = h ? (LPCSTR)GlobalLock(h) : NULL;
		if (s) { strRaw = s; bRaw = TRUE; GlobalUnlock(h); }
	}
	else if (uFirst == CF_UNICODETEXT)
	{
		HANDLE h = GetClipboardData(CF_UNICODETEXT);
		LPCWSTR s = h ? (LPCWSTR)GlobalLock(h) : NULL;
		if (s)
		{
			// Toan ky tu <= 0xFF: la byte TCVN3 bi "noi rong" => lay lai byte; co ky tu > 0xFF: Unicode that
			BOOL bWide = FALSE;
			for (LPCWSTR p = s; *p; p++) if (*p > 0xFF) { bWide = TRUE; break; }
			if (bWide)
				strOut = s;
			else
			{
				int n = (int)wcslen(s);
				LPSTR d = strRaw.GetBuffer(n + 1);
				for (int i = 0; i <= n; i++) d[i] = (char)s[i];
				strRaw.ReleaseBuffer(n);
				bRaw = TRUE;
			}
			GlobalUnlock(h);
		}
	}
	CloseClipboard();

	if (bRaw)
	{
		int n = strRaw.GetLength();
		TCVN3ToUnicode(strRaw, strOut.GetBuffer(n + 1), n);
		strOut.ReleaseBuffer(n);
	}
	strOut.Replace(L"\r", L""); strOut.Replace(L"\n", L"");
	return !strOut.IsEmpty();
}

// O nhap ten nhan vat: Ctrl+V / Shift+Ins / menu Dan => dung byte TCVN3 goc cua game
LRESULT CTcvnEdit::WindowProc(UINT message, WPARAM wParam, LPARAM lParam)
{
	if (message == WM_PASTE)
	{
		CStringW strText;
		if (GetClipboardGameText(strText))
			ReplaceSel(strText, TRUE);
		return 0;
	}
	return CEdit::WindowProc(message, wParam, lParam);
}

// Gan CTcvnEdit vao o nhap (EDIT) hoac phan nhap cua COMBOBOX kieu CBS_DROPDOWN
void CTcvnEdit::AttachTo(CWnd* pParent, UINT nID)
{
	HWND hCtrl = ::GetDlgItem(pParent->GetSafeHwnd(), nID);
	if (!hCtrl || GetSafeHwnd()) return;
	TCHAR szClass[16];
	::GetClassName(hCtrl, szClass, 16);
	if (!_tcsicmp(szClass, _T("ComboBox")))
		hCtrl = ::FindWindowEx(hCtrl, NULL, _T("Edit"), NULL);
	if (hCtrl) SubclassWindow(hCtrl);
}
