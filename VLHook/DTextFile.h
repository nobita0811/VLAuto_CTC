#pragma once

class DTextFile
{
private:
	FILE* m_pFile;

public:
	DTextFile();
	virtual ~DTextFile();
	BOOL Open(const char* szFileName, const char* openMode = NULL);
	void Close();
	int ReadLine(char* szLine, int nMaxLen);
	int WriteLine(const char* szFormat, ...);
	void ReadAllLines(char* szAllLines, int nMaxLine, int nMaxChar);
	void WriteAllLines(char* szAllLines, int nMaxLine, int nMaxChar);
};
