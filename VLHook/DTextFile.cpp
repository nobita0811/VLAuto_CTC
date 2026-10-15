#include "stdafx.h"
#include "DTextFile.h"

DTextFile::DTextFile()
{
	m_pFile = NULL;
}

DTextFile::~DTextFile()
{
	Close();
}

void DTextFile::Close()
{
	if (m_pFile != NULL)
	{
		fclose(m_pFile);
		m_pFile = NULL;
	}
}

BOOL DTextFile::Open(const char* szFileName, const char* openMode)
{
	if (m_pFile != NULL)
		fclose(m_pFile);

	if (openMode == NULL)
		m_pFile = fopen(szFileName, "rt");
	else
		m_pFile = fopen(szFileName, openMode);

	return (m_pFile != NULL);
}

int DTextFile::ReadLine(char* szLine, int nMaxLen)
{
	if (m_pFile == NULL)
		return -1;

	int nc, nLen = 0;
	while (nLen < nMaxLen - 1)
	{
		nc = fgetc(m_pFile);
		if (nc == EOF || nc == '\n')
			break;
		if (nc >= 0x20)
			szLine[nLen++] = (char)nc;
	}
	szLine[nLen] = 0;

	if (nLen == 0 && nc == EOF)
		return -1;

	return nLen;
}

int DTextFile::WriteLine(const char* szFormat, ...)
{
	va_list vlist;
	va_start(vlist, szFormat);
	int nRet = vfprintf(m_pFile, szFormat, vlist);
	va_end(vlist);

	return nRet;
}

void DTextFile::ReadAllLines(char* szAllLines, int nMaxLine, int nMaxChar)
{
	int nLineNum = 0;
	char szLine[256];

	if (nMaxChar > 255)
		nMaxChar = 255;

	while (nLineNum < nMaxLine)
	{
		if (ReadLine(szLine, 256) < 0)
			break;
		if (szLine[0] != '#')
		{
			char* szDest = szAllLines + nLineNum*nMaxChar;
			strncpy(szDest, szLine, nMaxChar);
			nLineNum++;
		}
	}
}

void DTextFile::WriteAllLines(char* szAllLines, int nMaxLine, int nMaxChar)
{
	for (int i = 0; i < nMaxLine; i++)
	{
		char* szLine = szAllLines + i*nMaxChar;
		fprintf(m_pFile, szLine);
	}
}
