// PEImage.h : nap file exe game (ca ban dump UPX) theo bo cuc bo nho
#pragma once

#include <vector>

class CPEImage
{
public:
	CPEImage() : m_dwBase(0), m_dwSize(0), m_dwCodeBeg(0), m_dwCodeEnd(0) {}

	BOOL Load(LPCTSTR lpszPath, CString &strErr);

	// Dia chi nam trong image cua game
	BOOL InImage(DWORD dwAddr) const { return dwAddr >= m_dwBase + 0x1000 && dwAddr < m_dwBase + m_dwSize; }
	// Dia chi nam trong vung code
	BOOL InCode(DWORD dwAddr) const { return dwAddr >= m_dwCodeBeg && dwAddr < m_dwCodeEnd; }

	const BYTE *Ptr(DWORD dwAddr) const { return &m_Image[dwAddr - m_dwBase]; }
	DWORD ReadDword(DWORD dwAddr) const { return *(const DWORD*)Ptr(dwAddr); }
	// So byte con lai tu dwAddr toi het vung code
	int CodeAvail(DWORD dwAddr) const { return InCode(dwAddr) ? (int)(m_dwCodeEnd - dwAddr) : 0; }

	CString				m_strPath;
	DWORD				m_dwBase;		// ImageBase
	DWORD				m_dwSize;		// SizeOfImage
	DWORD				m_dwCodeBeg;	// Vung code [m_dwCodeBeg, m_dwCodeEnd)
	DWORD				m_dwCodeEnd;
	std::vector<BYTE>	m_Image;
};
