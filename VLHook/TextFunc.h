#pragma once
#include "stdafx.h"
unsigned char Fold(unsigned char c);
BOOL EqualFold(const char* a, const char* b);
const char* FindFold(const char* szText, const char* szKey);
unsigned char YsFold(unsigned char c);

// strstr khong phan biet hoa thuong
const char* YsFindFold(const char* szText, const char* szKey);
const char* YsSkipSpace(const char* p);

// Chep [p, pEnd) bo khoang trang 2 dau
void YsCopyTrim(char* szOut, int nOut, const char* p, const char* pEnd);

// Bo the <link=..> <color=..> <enter>..., ky tu dieu khien -> ' ', gop khoang trang
void YsPlainText(const char* szText, char* szOut, int nOut);