#include "stdafx.h"
#include "PELib.h"

PELibrary::PELibrary()
{
	m_hFile = INVALID_HANDLE_VALUE;
	m_pImageBase = NULL;
}

PELibrary::~PELibrary()
{
	if (m_pImageBase)
	{
		GlobalFree(m_pImageBase);
		m_pImageBase = NULL;
	}
	if (m_hFile != INVALID_HANDLE_VALUE)
	{
		CloseHandle(m_hFile);
		m_hFile = INVALID_HANDLE_VALUE;
	}
}

BOOL PELibrary::GetImportDlls(LPCTSTR szFileName, LPSTR ImportDlls)
{
	if (!OpenPEFile(szFileName))
		return FALSE;

	PIMAGE_DOS_HEADER pimage_dos_header = (PIMAGE_DOS_HEADER)m_pImageBase;
	PIMAGE_NT_HEADERS pimage_nt_headers = (PIMAGE_NT_HEADERS)(m_pImageBase + pimage_dos_header->e_lfanew);
	DWORD dwImportDirectory = RVA2Offset(m_pImageBase, pimage_nt_headers->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress);
	if (dwImportDirectory == 0)
		return FALSE;

	PIMAGE_IMPORT_DESCRIPTOR pimage_import_descriptor = (PIMAGE_IMPORT_DESCRIPTOR)(m_pImageBase + dwImportDirectory);
	while (pimage_import_descriptor->Name)
	{
		PCHAR pDllName = m_pImageBase + RVA2Offset(m_pImageBase, pimage_import_descriptor->Name);
		strcat(ImportDlls, pDllName);
		pimage_import_descriptor++;
	}

	_strlwr(ImportDlls);

	if (m_pImageBase)
	{
		GlobalFree(m_pImageBase);
		m_pImageBase = NULL;
	}

	return TRUE;
}

//----------------------------------------------------------------
BOOL PELibrary::OpenPEFile(LPCTSTR szFileName)
{
	HANDLE m_hFile = CreateFile(szFileName, GENERIC_READ, FILE_SHARE_READ,
		0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);

	if (m_hFile == INVALID_HANDLE_VALUE)
		return FALSE;

	DWORD dwSize = GetFileSize(m_hFile, 0);
	if (dwSize == 0)
		return FALSE;

	m_pImageBase = (char*)(char*)GlobalAlloc(GMEM_FIXED|GMEM_ZEROINIT, dwSize + 4);
	if (!m_pImageBase)
		return FALSE;

	DWORD dwBytesRead = 0;
	ReadFile(m_hFile, m_pImageBase, dwSize, &dwBytesRead, NULL);

	CloseHandle(m_hFile);
	m_hFile = INVALID_HANDLE_VALUE;

	return (dwBytesRead == dwSize);
}

PIMAGE_SECTION_HEADER PELibrary::ImageRVA2Section(PIMAGE_NT_HEADERS pimage_nt_headers, DWORD dwRVA)
{
	PIMAGE_SECTION_HEADER pimage_section_header = (PIMAGE_SECTION_HEADER)((PCHAR(pimage_nt_headers)) + sizeof(IMAGE_NT_HEADERS));
	for (int i = 0; i < pimage_nt_headers->FileHeader.NumberOfSections; i++)
	{
		if ((pimage_section_header->VirtualAddress) && (dwRVA <= (pimage_section_header->VirtualAddress+pimage_section_header->SizeOfRawData)))
		{
			return ((PIMAGE_SECTION_HEADER)pimage_section_header);
		}
		pimage_section_header++;
	}
	return NULL;
}

DWORD PELibrary::RVA2Offset(PCHAR pImageBase,DWORD dwRVA)
{
	DWORD dwOffset;
	PIMAGE_SECTION_HEADER section;
	PIMAGE_DOS_HEADER pimage_dos_header;
	PIMAGE_NT_HEADERS pimage_nt_headers;
	pimage_dos_header = PIMAGE_DOS_HEADER(pImageBase);
	pimage_nt_headers = (PIMAGE_NT_HEADERS)(pImageBase + pimage_dos_header->e_lfanew);
	section = ImageRVA2Section(pimage_nt_headers,dwRVA);

	if (section == NULL)
		return 0;

	dwOffset = dwRVA + section->PointerToRawData - section->VirtualAddress;

	return dwOffset;
}
