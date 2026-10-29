#include "stdafx.h"
#include "TcvnText.h"

// Thu tu dau trong moi nhom: huyen, hoi, nga, sac, nang
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

C_ASSERT(sizeof(s_wUni) / sizeof(wchar_t) == sizeof(s_szTcvn));	// 2 bang phai cung so ky tu

static unsigned char s_Map[0x1F00];	// U+0000..U+1EFF -> TCVN3 (0 = khong co)
static wchar_t s_Rev[0x80];			// TCVN3 0x80..0xFF -> Unicode (0 = khong co)
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

int Tcvn3ToUtf8(const char* szTcvn, char* szOut, int nOut)
{
	if (!s_bMapReady) BuildMap();
	int n = 0;
	for (const unsigned char* p = (const unsigned char*)szTcvn; *p; p++)
	{
		unsigned int w = *p;
		if (w >= 0x80 && s_Rev[w - 0x80]) w = s_Rev[w - 0x80];	// byte ngoai bang: giu nguyen nhu Latin-1
		int nNeed = w < 0x80 ? 1 : (w < 0x800 ? 2 : 3);
		if (n + nNeed > nOut - 1) break;
		if (nNeed == 1)
			szOut[n++] = (char)w;
		else if (nNeed == 2)
		{
			szOut[n++] = (char)(0xC0 | (w >> 6));
			szOut[n++] = (char)(0x80 | (w & 0x3F));
		}
		else
		{
			szOut[n++] = (char)(0xE0 | (w >> 12));
			szOut[n++] = (char)(0x80 | ((w >> 6) & 0x3F));
			szOut[n++] = (char)(0x80 | (w & 0x3F));
		}
	}
	szOut[n] = 0;
	return n;
}

int UniToTcvn3(const wchar_t* szUni, char* szOut, int nOut)
{
	if (!s_bMapReady) BuildMap();	// 2 thread cung build: ghi cung gia tri, vo hai
	int n = 0;
	for (const wchar_t* p = szUni; *p && n < nOut - 1; p++)
	{
		wchar_t w = *p;
		unsigned char c;
		if (w < 0x80) c = (unsigned char)w;
		else if (w < 0x1F00 && s_Map[w]) c = s_Map[w];
		else c = '?';	// ky tu ngoai bang (vd dau to hop U+0300..)
		szOut[n++] = (char)c;
	}
	szOut[n] = 0;
	return n;
}

// Bang bam theo dia chi chuoi hang (khoang 800 chuoi trong VLHook)
#define TV_HASH_SIZE	4096	// luy thua 2

static struct { const wchar_t* pKey; const char* pVal; } s_Cache[TV_HASH_SIZE];
static volatile LONG s_nLock = 0;

const char* TV(const wchar_t* szUni)
{
	if (!szUni) return "";
	while (InterlockedCompareExchange(&s_nLock, 1, 0)) YieldProcessor();

	UINT_PTR h = ((UINT_PTR)szUni >> 1) * 2654435761u;
	int i = (int)((h >> 8) & (TV_HASH_SIZE - 1));
	for (int k = 0; k < TV_HASH_SIZE; k++, i = (i + 1) & (TV_HASH_SIZE - 1))
	{
		if (s_Cache[i].pKey == szUni)
		{
			const char* pVal = s_Cache[i].pVal;
			InterlockedExchange(&s_nLock, 0);
			return pVal;
		}
		if (!s_Cache[i].pKey) break;
	}

	int nLen = (int)wcslen(szUni) + 1;
	char* pVal = new char[nLen];		// moi chuoi hang chi cap 1 lan, khong giai phong
	UniToTcvn3(szUni, pVal, nLen);
	if (!s_Cache[i].pKey)
	{
		s_Cache[i].pKey = szUni;
		s_Cache[i].pVal = pVal;
	}
	InterlockedExchange(&s_nLock, 0);
	return pVal;
}
