// GameDiff.h : so sanh 2 ban game, tim dia chi / offset moi cho cac #define cua VLAuto
#pragma once

#include <vector>
#include <map>
#include <string>
#include <functional>
#include "PEImage.h"
#include "X86Len.h"

// Loai gia tri cua #define
enum
{
	GDK_SKIP = 0,	// Bo qua (co, so nho, nam ngoai image)
	GDK_CODE,		// Dia chi trong vung code (ham, vi tri lenh)
	GDK_DATA,		// Dia chi bien toan cuc
	GDK_CONST,		// Offset / kich thuoc struct
};

// Ket qua
enum
{
	GDS_NONE = 0,	// Chua phan tich
	GDS_SKIP,		// Bo qua
	GDS_SAME,		// Khong doi
	GDS_CHANGED,	// Da doi
	GDS_DERIVED,	// Suy ra tu dia chi lan can
	GDS_AMBIG,		// Nhieu ung vien
	GDS_SUSPECT,	// Can kiem tra (dia chi cu khong phai dau lenh)
	GDS_NOTFOUND,	// Khong tim thay
};

struct GD_DEFINE
{
	int			nFile;			// Chi so trong m_Files
	int			nLine;			// Dong (tu 0)
	CStringA	strName;
	DWORD		dwOld;
	int			nValPos;		// Vi tri chuoi hex (sau "0x") trong dong
	int			nValLen;
	BOOL		bLower;			// Chu hex viet thuong
	CStringA	strComment;

	int			nKind;
	int			nStatus;
	DWORD		dwNew;
	int			nVotes;			// So phieu cua gia tri duoc chon
	int			nTotal;			// Tong so phieu
	CString		strDetail;
};

struct GD_FILE
{
	CString						strPath;
	std::vector<std::string>	lines;	// Giu nguyen byte + ky tu xuong dong
};

class CGameDiff
{
public:
	CGameDiff();

	BOOL LoadImages(LPCTSTR lpszRef, LPCTSTR lpszTgt, CString &strErr);
	BOOL ParseHeaders(const std::vector<CString> &files, CString &strErr);
	// Phan tich tat ca #define. fnProgress(i, n); *pCancel != 0: dung
	void Resolve(std::function<void(int, int)> fnProgress, volatile LONG *pCancel);
	// Ghi cac file .h da cap nhat vao lpszOutDir (bOverwrite: ghi de file goc, luu .bak)
	BOOL WriteHeaders(LPCTSTR lpszOutDir, BOOL bOverwrite, CString &strResult);
	BOOL WriteReport(LPCTSTR lpszPath, CString &strErr);

	static LPCTSTR KindName(int nKind);
	static LPCTSTR StatusName(int nStatus);
	BOOL CanApply(const GD_DEFINE &d) const;

	CPEImage				m_Ref;		// Ban dang chay dung (dia chi trong .h la cua ban nay)
	CPEImage				m_Tgt;		// Ban can tim dia chi
	std::vector<GD_FILE>	m_Files;
	std::vector<GD_DEFINE>	m_Defines;

private:
	struct REF		{ DWORD dwValue; DWORD dwSite; BYTE byField; };	// Lenh co truong disp32/imm32 = dwValue
	struct CALL		{ DWORD dwTarget; DWORD dwSite; };					// call/jmp rel32 toi dwTarget
	struct SIGFIELD	{ int nOff; BOOL bRel; DWORD dwRefVal; };			// Truong bi che: dia chi tuyet doi / dich cua rel32
	struct SIG		{ std::vector<BYTE> bytes, mask; int nFixed; BOOL bEnd; std::vector<SIGFIELD> fields; };
	typedef std::vector<std::pair<int, DWORD> > RELCHECK;					// (vi tri rel32, dich mong doi)
	struct RESULT	{ int nStatus; DWORD dwNew; int nVotes, nTotal; CString strDetail; };
	typedef std::map<DWORD, int> VOTES;

	void IndexRef();
	void BuildSig(const CPEImage &im, DWORD dwStart, int nForceOff, BOOL bRelax, int nMinLen, SIG &sig) const;
	int  FindSig(const CPEImage &im, const SIG &sig, int nMax, DWORD *pHits) const;
	BOOL Locate(DWORD dwStart, int nForceOff, DWORD &dwHit) const;
	BOOL LocateBack(DWORD dwAddr, DWORD &dwHit) const;
	int  FindSigRel(const CPEImage &im, const SIG &sig, const RELCHECK &rel, int nMax, DWORD *pHits) const;
	BOOL LocateTranslated(DWORD dwAddr, DWORD &dwHit) const;
	BOOL IsInsStart(DWORD dwAddr) const { return m_Ref.InCode(dwAddr) && m_InsStart[dwAddr - m_Ref.m_dwCodeBeg]; }
	BOOL IsFuncStart(DWORD dwAddr) const;
	static BOOL IsCodeSiteName(const CStringA &strName);
	void Decide(const VOTES &votes, DWORD dwOld, RESULT &r) const;
	void ResolveAddr(DWORD dwVal, BOOL bCode, RESULT &r);
	void ResolveConst(DWORD dwVal, RESULT &r);
	BOOL ResolveDerived(DWORD dwVal, RESULT &r);
	int  ClassifyValue(const GD_DEFINE &d) const;

	std::vector<REF>		m_Refs;		// Sap xep theo dwValue
	std::vector<CALL>		m_Calls;	// Sap xep theo dwTarget
	std::map<std::pair<DWORD, int>, RESULT>	m_Cache;
	std::vector<bool>		m_InsStart;	// Dau lenh trong vung code ban cu (theo quet tuyen tinh)
	std::map<DWORD, DWORD>	m_Map;		// Dia chi ban cu -> ban moi da do chac chan (dung de dich mau)
};
