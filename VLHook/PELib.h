#pragma once

class PELibrary
{
public:
	PELibrary();
	~PELibrary();
	BOOL GetImportDlls(LPCTSTR szFileName, LPSTR ImportDlls);

private:
	BOOL OpenPEFile(LPCTSTR szFileName);
	PIMAGE_SECTION_HEADER ImageRVA2Section(PIMAGE_NT_HEADERS pimage_nt_headers, DWORD dwRVA);
	DWORD RVA2Offset(PCHAR pImageBase, DWORD dwRVA);

protected:
	HANDLE m_hFile;
	char *m_pImageBase;
};
