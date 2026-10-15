// PEImage.cpp : nap file exe game theo bo cuc bo nho
#include "stdafx.h"
#include "PEImage.h"
#include <map>

BOOL CPEImage::Load(LPCTSTR lpszPath, CString &strErr)
{
	m_strPath = lpszPath;
	m_Image.clear();

	CFile file;
	CFileException ex;
	if (!file.Open(lpszPath, CFile::modeRead | CFile::shareDenyNone, &ex))
	{
		strErr.Format(_T("Không mở được file %s"), lpszPath);
		return FALSE;
	}
	ULONGLONG ullLen = file.GetLength();
	if (ullLen < 0x400 || ullLen > 0x10000000)
	{
		strErr.Format(_T("Kích thước file không hợp lệ: %s"), lpszPath);
		return FALSE;
	}
	std::vector<BYTE> raw((size_t)ullLen);
	file.Read(raw.data(), (UINT)ullLen);
	file.Close();

	const IMAGE_DOS_HEADER *pDos = (const IMAGE_DOS_HEADER*)raw.data();
	if (pDos->e_magic != IMAGE_DOS_SIGNATURE || pDos->e_lfanew <= 0 || (size_t)pDos->e_lfanew + sizeof(IMAGE_NT_HEADERS32) > raw.size())
	{
		strErr.Format(_T("Không phải file PE: %s"), lpszPath);
		return FALSE;
	}
	const IMAGE_NT_HEADERS32 *pNt = (const IMAGE_NT_HEADERS32*)(raw.data() + pDos->e_lfanew);
	if (pNt->Signature != IMAGE_NT_SIGNATURE || pNt->FileHeader.Machine != IMAGE_FILE_MACHINE_I386 ||
		pNt->OptionalHeader.Magic != IMAGE_NT_OPTIONAL_HDR32_MAGIC)
	{
		strErr.Format(_T("Không phải file PE 32-bit: %s"), lpszPath);
		return FALSE;
	}

	m_dwBase = pNt->OptionalHeader.ImageBase;
	m_dwSize = pNt->OptionalHeader.SizeOfImage;
	if (m_dwSize < 0x2000 || m_dwSize > 0x10000000)
	{
		strErr.Format(_T("SizeOfImage không hợp lệ: %s"), lpszPath);
		return FALSE;
	}
	m_Image.assign(m_dwSize, 0);
	DWORD dwHdr = min((DWORD)raw.size(), pNt->OptionalHeader.SizeOfHeaders);
	memcpy(m_Image.data(), raw.data(), min(dwHdr, m_dwSize));

	// Chep cac section vao dung vi tri bo nho; vung code = section chua EntryPoint
	DWORD dwEP = pNt->OptionalHeader.AddressOfEntryPoint;
	const IMAGE_SECTION_HEADER *pSec = IMAGE_FIRST_SECTION(pNt);
	DWORD dwCodeRva = 0, dwCodeRaw = 0;
	for (int i = 0; i < pNt->FileHeader.NumberOfSections; i++, pSec++)
	{
		DWORD dwVA = pSec->VirtualAddress;
		DWORD dwVSize = pSec->Misc.VirtualSize ? pSec->Misc.VirtualSize : pSec->SizeOfRawData;
		DWORD dwRaw = min(pSec->SizeOfRawData, dwVSize);
		if (pSec->PointerToRawData >= raw.size() || dwVA >= m_dwSize) continue;
		dwRaw = min(dwRaw, (DWORD)raw.size() - pSec->PointerToRawData);
		dwRaw = min(dwRaw, m_dwSize - dwVA);
		memcpy(&m_Image[dwVA], raw.data() + pSec->PointerToRawData, dwRaw);
		// Section code: chua EntryPoint, hoac section dau tien co co IMAGE_SCN_CNT_CODE
		if (!dwCodeRaw && ((dwEP >= dwVA && dwEP < dwVA + dwVSize) || (pSec->Characteristics & IMAGE_SCN_CNT_CODE)))
		{
			dwCodeRva = dwVA;
			dwCodeRaw = dwRaw;
		}
	}
	if (!dwCodeRaw)
	{
		strErr.Format(_T("Không tìm thấy section code: %s"), lpszPath);
		return FALSE;
	}
	m_dwCodeBeg = m_dwBase + dwCodeRva;
	m_dwCodeEnd = m_dwCodeBeg + dwCodeRaw;

	// Ban dump UPX: code + rdata + data nam chung 1 section. Cat vung code tai trang
	// thap nhat cua bang IAT (dich cua cac lenh call [mem] = FF 15 xx xx xx xx)
	std::map<DWORD, int> iat;
	for (DWORD a = m_dwCodeBeg; a + 6 <= m_dwCodeEnd; a++)
	{
		const BYTE *p = Ptr(a);
		if (p[0] != 0xFF || p[1] != 0x15) continue;
		DWORD v = *(const DWORD*)(p + 2);
		if (v > m_dwCodeBeg && v < m_dwCodeEnd) iat[v]++;
	}
	for (auto &it : iat)
	{
		if (it.second >= 3)
		{
			DWORD dwEnd = it.first & ~0xFFF;
			if (dwEnd > m_dwCodeBeg + 0x1000) m_dwCodeEnd = dwEnd;
			break;
		}
	}
	return TRUE;
}
