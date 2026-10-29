#include "stdafx.h"
unsigned char Fold(unsigned char c)
{
	if (c >= 'A' && c <= 'Z') return c + 32;
	if (c >= 0xA1 && c <= 0xA7) return c + 7;	// TCVN3: A( A^ E^ O^ O+ U+ D- -> chu thuong
	return c;
}
BOOL EqualFold(const char* a, const char* b)
{
	while (*a && Fold(*a) == Fold(*b)) { a++; b++; }
	return Fold(*a) == Fold(*b);
}
const char* FindFold(const char* szText, const char* szKey)
{
	if (!szText || !szKey[0]) return NULL;
	for (const char* p = szText; *p; p++)
	{
		int i = 0;
		while (szKey[i] && Fold(p[i]) == Fold(szKey[i])) i++;
		if (!szKey[i]) return p;
	}
	return NULL;
}
unsigned char YsFold(unsigned char c)
{
	if (c >= 'A' && c <= 'Z') return c + 32;
	if (c >= 0xA1 && c <= 0xA7) return c + 7;	// TCVN3: A( A^ E^ O^ O+ U+ D- -> chu thuong
	return c;
}
// strstr khong phan biet hoa thuong
const char* YsFindFold(const char* szText, const char* szKey)
{
	if (!szText || !szKey[0]) return NULL;
	for (const char* p = szText; *p; p++)
	{
		int i = 0;
		while (szKey[i] && YsFold(p[i]) == YsFold(szKey[i])) i++;
		if (!szKey[i]) return p;
	}
	return NULL;
}
const char* YsSkipSpace(const char* p)
{
	while (*p == ' ') p++;
	return p;
}
// Chep [p, pEnd) bo khoang trang 2 dau
void YsCopyTrim(char* szOut, int nOut, const char* p, const char* pEnd)
{
	while (p < pEnd && *p == ' ') p++;
	while (pEnd > p && pEnd[-1] == ' ') pEnd--;
	int n = (int)(pEnd - p);
	if (n > nOut - 1) n = nOut - 1;
	if (n < 0) n = 0;
	memcpy(szOut, p, n);
	szOut[n] = 0;
}
// Bo the <link=..> <color=..> <enter>..., ky tu dieu khien -> ' ', gop khoang trang
void YsPlainText(const char* szText, char* szOut, int nOut)
{
	int n = 0;
	for (const char* p = szText; *p && n < nOut - 1; p++)
	{
		unsigned char c = (unsigned char)*p;
		if (c == '<')
		{
			unsigned char c1 = (unsigned char)p[1];
			const char* pEnd = strchr(p, '>');
			if (pEnd && pEnd - p < 160 && ((c1 >= 'a' && c1 <= 'z') || (c1 >= 'A' && c1 <= 'Z') || c1 == '/'))
			{
				p = pEnd;
				continue;
			}
		}
		if (c < 0x20) c = ' ';
		if (c == ' ' && (n == 0 || szOut[n - 1] == ' ')) continue;
		szOut[n++] = (char)c;
	}
	while (n > 0 && szOut[n - 1] == ' ') n--;
	szOut[n] = 0;
}