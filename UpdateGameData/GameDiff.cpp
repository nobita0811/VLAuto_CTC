// GameDiff.cpp : so sanh 2 ban game, tim dia chi / offset moi cho cac #define cua VLAuto
//
// Cach lam (khong can ban do dia chi cua ban moi):
//  - Dia chi ham / vi tri lenh: lay mau byte tai dia chi do o ban cu (che cac dia chi tuyet doi,
//    rel32 cua call/jmp), noi dai toi khi mau la duy nhat, roi tim mau trong ban moi.
//    Them phieu tu cac lenh call/jmp toi ham: tim lai lenh call trong ban moi, doc dich cua no.
//  - Bien toan cuc: moi lenh o ban cu co truong disp32/imm32 = dia chi -> lay mau quanh lenh,
//    tim trong ban moi, doc lai truong do. Nhieu lenh cung ra 1 gia tri -> chac chan.
//  - Offset / kich thuoc struct: nhu bien toan cuc, truong chua offset bi che trong mau.
//  - Bien khong co lenh nao tham chieu truc tiep: suy tu bien lan can (cung khoang cach).
#include "stdafx.h"
#include "GameDiff.h"
#include <algorithm>

#define MAX_CALL_SITES		12		// So lenh call toi da dung de bo phieu cho 1 ham
#define MAX_REF_SITES		16		// So lenh tham chieu toi da cho 1 dia chi
#define MAX_CONST_SITES		40		// So lenh tham chieu toi da cho 1 offset struct
#define MAX_DERIVE_DIST		0x1000	// Khoang cach toi da toi bien lan can khi suy ra

CGameDiff::CGameDiff()
{
}

BOOL CGameDiff::LoadImages(LPCTSTR lpszRef, LPCTSTR lpszTgt, CString &strErr)
{
	m_Cache.clear();
	if (!m_Ref.Load(lpszRef, strErr)) return FALSE;
	if (!m_Tgt.Load(lpszTgt, strErr)) return FALSE;
	IndexRef();
	return TRUE;
}

// Quet tuyen tinh vung code ban cu: ghi lai cac truong disp32/imm32 va cac lenh call/jmp rel32.
// Quet 2 luot: luot 2 canh lai tai dich cua cac lenh call (dau ham) de khong bi lech nhip sau
// cac bang nhay switch nam xen trong code
void CGameDiff::IndexRef()
{
	std::vector<DWORD> targets;
	for (int nPass = 0; nPass < 2; nPass++)
	{
		m_Refs.clear();
		m_Calls.clear();
		m_InsStart.assign(m_Ref.m_dwCodeEnd - m_Ref.m_dwCodeBeg, false);
		size_t t = 0;
		DWORD a = m_Ref.m_dwCodeBeg;
		while (a < m_Ref.m_dwCodeEnd)
		{
			const BYTE *p = m_Ref.Ptr(a);
			while (t < targets.size() && targets[t] < a) t++;
			if (p[0] == 0xCC) { a++; continue; }
			X86Ins ins;
			if (!X86Decode(p, m_Ref.CodeAvail(a), ins)) { a++; continue; }
			// Lenh dang giai ma de len dau 1 ham: bo, giai ma lai tu dau ham
			if (t < targets.size() && targets[t] > a && targets[t] < a + ins.nLen) { a = targets[t]; continue; }
			m_InsStart[a - m_Ref.m_dwCodeBeg] = true;
			if (ins.nDispSize == 4)
			{
				REF r = { *(const DWORD*)(p + ins.nDispOff), a, (BYTE)ins.nDispOff };
				m_Refs.push_back(r);
			}
			if (ins.nImmSize == 4)
			{
				REF r = { *(const DWORD*)(p + ins.nImmOff), a, (BYTE)ins.nImmOff };
				m_Refs.push_back(r);
			}
			if (ins.nRelSize == 4 && !ins.b0F && (ins.byOp == 0xE8 || ins.byOp == 0xE9))
			{
				CALL c = { a + ins.nLen + *(const DWORD*)(p + ins.nRelOff), a };
				if (m_Ref.InCode(c.dwTarget)) m_Calls.push_back(c);
			}
			a += ins.nLen;
		}
		if (nPass == 0)
		{
			for (auto &c : m_Calls)
				if (m_Ref.Ptr(c.dwSite)[0] == 0xE8) targets.push_back(c.dwTarget);
			std::sort(targets.begin(), targets.end());
			targets.erase(std::unique(targets.begin(), targets.end()), targets.end());
		}
	}
	std::stable_sort(m_Refs.begin(), m_Refs.end(), [](const REF &x, const REF &y) { return x.dwValue < y.dwValue; });
	std::stable_sort(m_Calls.begin(), m_Calls.end(), [](const CALL &x, const CALL &y) { return x.dwTarget < y.dwTarget; });
}

//////////////////////////////////////////////////////////////////////////
// Doc cac #define dang "#define TEN  0xHEX" trong file .h

BOOL CGameDiff::ParseHeaders(const std::vector<CString> &files, CString &strErr)
{
	m_Files.clear();
	m_Defines.clear();
	m_Cache.clear();
	for (size_t f = 0; f < files.size(); f++)
	{
		CFile file;
		if (!file.Open(files[f], CFile::modeRead | CFile::shareDenyNone))
		{
			strErr.Format(_T("Không mở được file %s"), (LPCTSTR)files[f]);
			return FALSE;
		}
		std::string data((size_t)file.GetLength(), '\0');
		if (!data.empty()) file.Read(&data[0], (UINT)data.size());
		file.Close();

		GD_FILE gf;
		gf.strPath = files[f];
		size_t pos = 0;
		while (pos < data.size())
		{
			size_t eol = data.find('\n', pos);
			eol = (eol == std::string::npos) ? data.size() : eol + 1;
			gf.lines.push_back(data.substr(pos, eol - pos));
			pos = eol;
		}

		for (size_t l = 0; l < gf.lines.size(); l++)
		{
			const std::string &s = gf.lines[l];
			size_t i = 0;
			if (l == 0 && s.size() >= 3 && (BYTE)s[0] == 0xEF && (BYTE)s[1] == 0xBB && (BYTE)s[2] == 0xBF) i = 3;
			while (i < s.size() && (s[i] == ' ' || s[i] == '\t')) i++;
			if (s.compare(i, 7, "#define") != 0) continue;
			i += 7;
			if (i >= s.size() || (s[i] != ' ' && s[i] != '\t')) continue;
			while (i < s.size() && (s[i] == ' ' || s[i] == '\t')) i++;
			size_t nb = i;
			while (i < s.size() && (isalnum((BYTE)s[i]) || s[i] == '_')) i++;
			if (i == nb || i >= s.size() || (s[i] != ' ' && s[i] != '\t')) continue;
			std::string name = s.substr(nb, i - nb);
			while (i < s.size() && (s[i] == ' ' || s[i] == '\t')) i++;
			if (i < s.size() && s[i] == '(') i++;
			if (s.compare(i, 2, "0x") != 0 && s.compare(i, 2, "0X") != 0) continue;
			i += 2;
			size_t vb = i;
			while (i < s.size() && isxdigit((BYTE)s[i])) i++;
			if (i == vb || i - vb > 8) continue;

			GD_DEFINE d;
			d.nFile = (int)f;
			d.nLine = (int)l;
			d.strName = name.c_str();
			d.dwOld = strtoul(s.substr(vb, i - vb).c_str(), NULL, 16);
			d.nValPos = (int)vb;
			d.nValLen = (int)(i - vb);
			d.bLower = FALSE;
			for (size_t k = vb; k < i; k++) if (s[k] >= 'a' && s[k] <= 'f') d.bLower = TRUE;
			size_t c = s.find("//", i);
			if (c != std::string::npos)
			{
				std::string cmt = s.substr(c + 2);
				while (!cmt.empty() && (cmt.back() == '\r' || cmt.back() == '\n' || cmt.back() == ' ' || cmt.back() == '\t')) cmt.pop_back();
				size_t t = cmt.find_first_not_of(" \t");
				d.strComment = (t == std::string::npos) ? "" : cmt.substr(t).c_str();
			}
			d.nKind = GDK_SKIP;
			d.nStatus = GDS_NONE;
			d.dwNew = d.dwOld;
			d.nVotes = d.nTotal = 0;
			m_Defines.push_back(d);
		}
		m_Files.push_back(gf);
	}
	return TRUE;
}

int CGameDiff::ClassifyValue(const GD_DEFINE &d) const
{
	DWORD v = d.dwOld;
	if (m_Ref.InImage(v)) return m_Ref.InCode(v) ? GDK_CODE : GDK_DATA;
	if (v >= 0x80 && v < 0x01000000) return GDK_CONST;
	return GDK_SKIP;
}

//////////////////////////////////////////////////////////////////////////
// Mau byte

// Giai ma lenh tu dwStart toi khi du nMinLen byte. Che: rel32/rel16 cua call/jmp/jcc, disp/imm32 la
// dia chi trong image; bRelax: che ca moi disp32/imm32 (offset struct co the doi giua 2 ban).
// nForceOff >= 0: che 4 byte tai vi tri do trong lenh dau tien (truong can doc lai)
void CGameDiff::BuildSig(const CPEImage &im, DWORD dwStart, int nForceOff, BOOL bRelax, int nMinLen, SIG &sig) const
{
	sig.bytes.clear();
	sig.mask.clear();
	sig.nFixed = 0;
	sig.bEnd = FALSE;
	sig.fields.clear();
	DWORD a = dwStart;
	while ((int)sig.bytes.size() < nMinLen)
	{
		int nAvail = im.CodeAvail(a);
		if (nAvail <= 0) { sig.bEnd = TRUE; break; }
		const BYTE *p = im.Ptr(a);
		if (a != dwStart && p[0] == 0xCC && nAvail > 1 && p[1] == 0xCC) { sig.bEnd = TRUE; break; }
		X86Ins ins;
		if (!X86Decode(p, nAvail, ins)) { sig.bEnd = TRUE; break; }

		size_t o = sig.bytes.size();
		sig.bytes.insert(sig.bytes.end(), p, p + ins.nLen);
		sig.mask.insert(sig.mask.end(), ins.nLen, 1);
		BYTE *m = &sig.mask[o];
		if (ins.nRelSize >= 2)
		{
			memset(m + ins.nRelOff, 0, ins.nRelSize);
			if (ins.nRelSize == 4)
			{
				SIGFIELD f = { (int)o + ins.nRelOff, TRUE, a + ins.nLen + *(const DWORD*)(p + ins.nRelOff) };
				sig.fields.push_back(f);
			}
		}
		if (ins.nDispSize == 4)
		{
			DWORD v = *(const DWORD*)(p + ins.nDispOff);
			if (ins.bDispAbs || im.InImage(v) || bRelax) memset(m + ins.nDispOff, 0, 4);
			if (im.InImage(v)) { SIGFIELD f = { (int)o + ins.nDispOff, FALSE, v }; sig.fields.push_back(f); }
		}
		if (ins.nImmSize == 4)
		{
			DWORD v = *(const DWORD*)(p + ins.nImmOff);
			if (im.InImage(v) || bRelax) memset(m + ins.nImmOff, 0, 4);
			if (im.InImage(v)) { SIGFIELD f = { (int)o + ins.nImmOff, FALSE, v }; sig.fields.push_back(f); }
		}
		if (a == dwStart && nForceOff >= 0 && nForceOff + 4 <= ins.nLen) memset(m + nForceOff, 0, 4);
		a += ins.nLen;
	}
	for (size_t i = 0; i < sig.mask.size(); i++) sig.nFixed += sig.mask[i];
}

// Tim mau trong vung code, tra ve so lan khop (toi da nMax)
int CGameDiff::FindSig(const CPEImage &im, const SIG &sig, int nMax, DWORD *pHits) const
{
	size_t nLen = sig.bytes.size();
	size_t nCode = im.m_dwCodeEnd - im.m_dwCodeBeg;
	if (!nLen || nLen > nCode) return 0;

	// Doan byte co dinh dai nhat lam diem neo
	size_t rb = 0, rl = 0;
	for (size_t i = 0; i < nLen;)
	{
		if (!sig.mask[i]) { i++; continue; }
		size_t j = i;
		while (j < nLen && sig.mask[j]) j++;
		if (j - i > rl) { rb = i; rl = j - i; }
		i = j;
	}
	if (!rl) return 0;

	const BYTE *pBase = im.Ptr(im.m_dwCodeBeg);
	const BYTE *pNeedle = &sig.bytes[rb];
	const BYTE *p = pBase + rb;
	const BYTE *pEnd = pBase + (nCode - nLen) + rb + 1;
	int nHits = 0;
	while (p < pEnd)
	{
		p = (const BYTE*)memchr(p, pNeedle[0], pEnd - p);
		if (!p) break;
		if (memcmp(p, pNeedle, rl) == 0)
		{
			const BYTE *st = p - rb;
			size_t k = 0;
			for (; k < nLen; k++) if (sig.mask[k] && st[k] != sig.bytes[k]) break;
			if (k == nLen)
			{
				pHits[nHits++] = im.m_dwCodeBeg + (DWORD)(st - pBase);
				if (nHits >= nMax) break;
			}
		}
		p++;
	}
	return nHits;
}

// Tim vi tri tuong ung voi lenh dwStart (ban cu) trong ban moi
BOOL CGameDiff::Locate(DWORD dwStart, int nForceOff, DWORD &dwHit) const
{
	static const int s_nTiers[] = { 16, 24, 40, 64, 96 };
	SIG sig;
	DWORD hits[2];
	for (int nRelax = 0; nRelax < 2; nRelax++)
	{
		for (int t = 0; t < _countof(s_nTiers); t++)
		{
			BuildSig(m_Ref, dwStart, nForceOff, nRelax, s_nTiers[t], sig);
			if (sig.nFixed < 6)
			{
				if (sig.bEnd) break;
				continue;
			}
			int n = FindSig(m_Ref, sig, 2, hits);
			if (n != 1 || hits[0] != dwStart)
			{
				if (sig.bEnd) break;
				continue;
			}
			n = FindSig(m_Tgt, sig, 2, hits);
			if (n == 1)
			{
				dwHit = hits[0];
				return TRUE;
			}
			if (n == 0 || sig.bEnd) break;
		}
	}
	return FALSE;
}

// Nhu Locate nhung mau bat dau truoc dwAddr: thu cac diem bat dau dwAddr - k ma giai ma tuyen tinh
// tu do roi dung vao dwAddr (it nhat 2 lenh)
BOOL CGameDiff::LocateBack(DWORD dwAddr, DWORD &dwHit) const
{
	for (DWORD k = 4; k <= 48 && dwAddr - k >= m_Ref.m_dwCodeBeg; k++)
	{
		DWORD a = dwAddr - k;
		int nIns = 0;
		while (a < dwAddr)
		{
			const BYTE *p = m_Ref.Ptr(a);
			X86Ins ins;
			if (p[0] == 0xCC || !X86Decode(p, m_Ref.CodeAvail(a), ins)) break;
			a += ins.nLen;
			nIns++;
		}
		if (a != dwAddr || nIns < 2) continue;
		DWORD dwStartHit;
		if (Locate(dwAddr - k, -1, dwStartHit))
		{
			dwHit = dwStartHit + k;
			return TRUE;
		}
	}
	return FALSE;
}

// Dau ham: co lenh call/jmp toi, hoac ngay sau vung dem CC / NOP / lenh ret
BOOL CGameDiff::IsFuncStart(DWORD dwAddr) const
{
	auto it = std::lower_bound(m_Calls.begin(), m_Calls.end(), dwAddr, [](const CALL &c, DWORD v) { return c.dwTarget < v; });
	if (it != m_Calls.end() && it->dwTarget == dwAddr) return TRUE;
	if (dwAddr < m_Ref.m_dwCodeBeg + 3 || !m_Ref.InCode(dwAddr)) return FALSE;
	const BYTE *p = m_Ref.Ptr(dwAddr);
	return p[-1] == 0xCC || p[-1] == 0x90 || p[-1] == 0xC3 || p[-3] == 0xC2;
}

// Ten #define cho thay day la vi tri 1 lenh (khong phai dau ham)
BOOL CGameDiff::IsCodeSiteName(const CStringA &strName)
{
	static const char *s_szKeys[] = { "JMP", "INJ", "RET", "CALL", "HOOK", "PATCH", "SITE" };
	CStringA s = strName;
	s.MakeUpper();
	for (int i = 0; i < _countof(s_szKeys); i++) if (s.Find(s_szKeys[i]) >= 0) return TRUE;
	return FALSE;
}

// FindSig + kiem tra dich cua cac truong rel32
int CGameDiff::FindSigRel(const CPEImage &im, const SIG &sig, const RELCHECK &rel, int nMax, DWORD *pHits) const
{
	DWORD hits[256];
	int n = FindSig(im, sig, _countof(hits), hits);
	if (n >= _countof(hits)) return nMax;	// Qua nhieu - coi nhu khong duy nhat
	int nOut = 0;
	for (int i = 0; i < n && nOut < nMax; i++)
	{
		BOOL bOK = TRUE;
		for (auto &r : rel)
		{
			DWORD a = hits[i] + r.first;
			if (a + 4 + im.ReadDword(a) != r.second) { bOK = FALSE; break; }
		}
		if (bOK) pHits[nOut++] = hits[i];
	}
	return nOut;
}

// Mau qua chung (vd ham chi co "mov ecx,[CORE]; jmp DrawGameSpace"): thay cac dia chi trong mau
// bang dia chi tuong ung o ban moi da do duoc (m_Map), dich cua call/jmp cung phai khop
BOOL CGameDiff::LocateTranslated(DWORD dwAddr, DWORD &dwHit) const
{
	if (!IsInsStart(dwAddr)) return FALSE;
	for (DWORD k = 0; k <= 48 && dwAddr - k >= m_Ref.m_dwCodeBeg; k++)
	{
		DWORD dwStart = dwAddr - k;
		if (!IsInsStart(dwStart)) continue;
		SIG sig;
		BuildSig(m_Ref, dwStart, -1, FALSE, 48, sig);
		if ((DWORD)sig.bytes.size() <= k) continue;
		SIG sigRef = sig, sigTgt = sig;
		RELCHECK relRef, relTgt;
		int nTrans = 0;
		for (auto &f : sig.fields)
		{
			auto it = m_Map.find(f.dwRefVal);
			if (it == m_Map.end()) continue;
			nTrans++;
			if (f.bRel)
			{
				relRef.push_back(std::make_pair(f.nOff, f.dwRefVal));
				relTgt.push_back(std::make_pair(f.nOff, it->second));
			}
			else
			{
				memcpy(&sigRef.bytes[f.nOff], &f.dwRefVal, 4);
				memcpy(&sigTgt.bytes[f.nOff], &it->second, 4);
				memset(&sigRef.mask[f.nOff], 1, 4);
				memset(&sigTgt.mask[f.nOff], 1, 4);
			}
		}
		if (!nTrans) continue;
		DWORD hits[2];
		if (FindSigRel(m_Ref, sigRef, relRef, 2, hits) != 1 || hits[0] != dwStart) continue;
		if (FindSigRel(m_Tgt, sigTgt, relTgt, 2, hits) != 1) continue;
		dwHit = hits[0] + k;
		return TRUE;
	}
	return FALSE;
}

//////////////////////////////////////////////////////////////////////////
// Bo phieu

void CGameDiff::Decide(const VOTES &votes, DWORD dwOld, RESULT &r) const
{
	r.dwNew = dwOld;
	r.nVotes = r.nTotal = 0;
	if (votes.empty())
	{
		r.nStatus = GDS_NOTFOUND;
		return;
	}
	DWORD dwBest = 0;
	int nBest = 0, nSecond = 0;
	for (auto &it : votes)
	{
		r.nTotal += it.second;
		if (it.second > nBest) { nSecond = nBest; nBest = it.second; dwBest = it.first; }
		else if (it.second > nSecond) nSecond = it.second;
	}
	r.nVotes = nBest;
	if (nSecond == 0 || nBest >= 3 * nSecond)
	{
		r.dwNew = dwBest;
		r.nStatus = (dwBest == dwOld) ? GDS_SAME : GDS_CHANGED;
	}
	else r.nStatus = GDS_AMBIG;

	if (votes.size() > 1)
	{
		std::vector<std::pair<int, DWORD> > v;
		for (auto &it : votes) v.push_back(std::make_pair(it.second, it.first));
		std::sort(v.rbegin(), v.rend());
		CString s = _T("; ứng viên:");
		for (size_t i = 0; i < v.size() && i < 4; i++)
		{
			CString t;
			t.Format(_T(" 0x%X (%d)"), v[i].second, v[i].first);
			s += t;
		}
		r.strDetail += s;
	}
}

// Lay mau deu nMax phan tu trong [nBeg, nEnd)
static void SampleRange(size_t nBeg, size_t nEnd, size_t nMax, std::vector<size_t> &out)
{
	out.clear();
	size_t n = nEnd - nBeg;
	if (n <= nMax) { for (size_t i = nBeg; i < nEnd; i++) out.push_back(i); return; }
	for (size_t i = 0; i < nMax; i++) out.push_back(nBeg + i * n / nMax);
}

static BOOL IsDecided(const std::map<DWORD, int> &votes, int nNeed)
{
	return votes.size() == 1 && votes.begin()->second >= nNeed;
}

void CGameDiff::ResolveAddr(DWORD dwVal, BOOL bCode, RESULT &r)
{
	auto key = std::make_pair(dwVal, bCode ? GDK_CODE : GDK_DATA);
	auto it = m_Cache.find(key);
	if (it != m_Cache.end()) { r = it->second; return; }

	VOTES votes;
	int nAnchor = 0, nCallTry = 0, nCallOk = 0, nRefTry = 0, nRefOk = 0;
	DWORD dwHit;
	std::vector<size_t> idx;
	if (bCode)
	{
		// 1. Mau tai chinh dia chi do (3 phieu)
		if (Locate(dwVal, -1, dwHit)) { votes[dwHit] += 3; nAnchor = 1; }
		// Mau qua ngan (vd lenh cuoi ham): lay mau tu cac lenh truoc do, giai ma tu do phai dung vao dwVal
		else if (LocateBack(dwVal, dwHit)) { votes[dwHit] += 3; nAnchor = 2; }

		// 2. Cac lenh call/jmp toi dia chi
		auto lo = std::lower_bound(m_Calls.begin(), m_Calls.end(), dwVal, [](const CALL &c, DWORD v) { return c.dwTarget < v; });
		auto hi = lo;
		while (hi != m_Calls.end() && hi->dwTarget == dwVal) hi++;
		SampleRange(lo - m_Calls.begin(), hi - m_Calls.begin(), MAX_CALL_SITES, idx);
		for (size_t k : idx)
		{
			nCallTry++;
			if (!Locate(m_Calls[k].dwSite, -1, dwHit)) continue;
			const BYTE *p = m_Tgt.Ptr(dwHit);
			if (p[0] != 0xE8 && p[0] != 0xE9) continue;
			votes[dwHit + 5 + *(const DWORD*)(p + 1)]++;
			nCallOk++;
			if (IsDecided(votes, 8)) break;
		}
	}

	// 3. Cac lenh co truong disp32/imm32 = dia chi
	auto lo = std::lower_bound(m_Refs.begin(), m_Refs.end(), dwVal, [](const REF &x, DWORD v) { return x.dwValue < v; });
	auto hi = lo;
	while (hi != m_Refs.end() && hi->dwValue == dwVal) hi++;
	SampleRange(lo - m_Refs.begin(), hi - m_Refs.begin(), MAX_REF_SITES, idx);
	for (size_t k : idx)
	{
		nRefTry++;
		if (!Locate(m_Refs[k].dwSite, -1, dwHit)) continue;
		votes[m_Tgt.ReadDword(dwHit + m_Refs[k].byField)]++;
		nRefOk++;
		if (IsDecided(votes, 8)) break;
	}

	if (bCode) r.strDetail.Format(_T("mẫu %s, gọi %d/%d, tham chiếu %d/%d"), nAnchor == 1 ? _T("khớp") : nAnchor == 2 ? _T("khớp (lùi)") : _T("không"),
		nCallOk, nCallTry, nRefOk, nRefTry);
	else r.strDetail.Format(_T("tham chiếu %d/%d"), nRefOk, nRefTry);
	Decide(votes, dwVal, r);
	if (bCode && !IsInsStart(dwVal) && (r.nStatus == GDS_SAME || r.nStatus == GDS_CHANGED))
	{
		r.nStatus = GDS_SUSPECT;
		r.strDetail = _T("địa chỉ cũ không phải đầu lệnh (giá trị cũ đã sai?); ") + r.strDetail;
	}
	m_Cache[key] = r;
}

void CGameDiff::ResolveConst(DWORD dwVal, RESULT &r)
{
	auto key = std::make_pair(dwVal, (int)GDK_CONST);
	auto it = m_Cache.find(key);
	if (it != m_Cache.end()) { r = it->second; return; }

	VOTES votes;
	int nTry = 0, nOk = 0;
	DWORD dwHit;
	std::vector<size_t> idx;
	auto lo = std::lower_bound(m_Refs.begin(), m_Refs.end(), dwVal, [](const REF &x, DWORD v) { return x.dwValue < v; });
	auto hi = lo;
	while (hi != m_Refs.end() && hi->dwValue == dwVal) hi++;
	SampleRange(lo - m_Refs.begin(), hi - m_Refs.begin(), MAX_CONST_SITES, idx);
	for (size_t k : idx)
	{
		nTry++;
		if (!Locate(m_Refs[k].dwSite, m_Refs[k].byField, dwHit)) continue;
		votes[m_Tgt.ReadDword(dwHit + m_Refs[k].byField)]++;
		nOk++;
	}
	r.strDetail.Format(_T("lệnh dùng %d (thử %d), khớp %d"), (int)(hi - lo), nTry, nOk);
	Decide(votes, dwVal, r);
	m_Cache[key] = r;
}

// Bien khong tim duoc truc tiep: suy tu bien lan can co tham chieu (giu nguyen khoang cach)
BOOL CGameDiff::ResolveDerived(DWORD dwVal, RESULT &r)
{
	std::vector<DWORD> vNear;
	auto lo = std::lower_bound(m_Refs.begin(), m_Refs.end(), dwVal, [](const REF &x, DWORD v) { return x.dwValue < v; });
	// 2 bien lan can phia duoi
	for (auto p = lo; p != m_Refs.begin() && vNear.size() < 2;)
	{
		--p;
		if (dwVal - p->dwValue > MAX_DERIVE_DIST) break;
		if (!m_Ref.InImage(p->dwValue) || m_Ref.InCode(p->dwValue)) continue;
		if (vNear.empty() || vNear.back() != p->dwValue) vNear.push_back(p->dwValue);
	}
	// 2 bien lan can phia tren
	size_t nBelow = vNear.size();
	for (auto p = lo; p != m_Refs.end() && vNear.size() < nBelow + 2; ++p)
	{
		if (p->dwValue == dwVal) continue;
		if (p->dwValue - dwVal > MAX_DERIVE_DIST) break;
		if (!m_Ref.InImage(p->dwValue) || m_Ref.InCode(p->dwValue)) continue;
		if (vNear.size() == nBelow || vNear.back() != p->dwValue) vNear.push_back(p->dwValue);
	}

	VOTES votes;
	CString strFrom;
	for (DWORD v : vNear)
	{
		RESULT rn;
		ResolveAddr(v, FALSE, rn);
		if (rn.nStatus != GDS_SAME && rn.nStatus != GDS_CHANGED) continue;
		votes[rn.dwNew + (dwVal - v)] += rn.nVotes;
		CString t;
		t.Format(_T(" 0x%08X%+d"), v, (int)(dwVal - v));
		strFrom += t;
	}
	if (votes.empty()) return FALSE;
	RESULT rd;
	rd.strDetail = _T("suy từ biến lân cận:") + strFrom;
	Decide(votes, dwVal, rd);
	if (rd.nStatus == GDS_SAME || rd.nStatus == GDS_CHANGED) rd.nStatus = GDS_DERIVED;
	r = rd;
	return TRUE;
}

void CGameDiff::Resolve(std::function<void(int, int)> fnProgress, volatile LONG *pCancel)
{
	int n = (int)m_Defines.size();
	for (int i = 0; i < n; i++)
	{
		if (pCancel && *pCancel) break;
		if (fnProgress) fnProgress(i, n);
		GD_DEFINE &d = m_Defines[i];
		d.nKind = ClassifyValue(d);
		RESULT r;
		r.nStatus = GDS_SKIP;
		r.dwNew = d.dwOld;
		r.nVotes = r.nTotal = 0;
		switch (d.nKind)
		{
		case GDK_CODE:
			ResolveAddr(d.dwOld, TRUE, r);
			// Dia chi nam giua ham ma ten khong phai vi tri lenh (JMP, INJ, RET...): thuong la gia tri cu da sai
			if ((r.nStatus == GDS_SAME || r.nStatus == GDS_CHANGED) && !IsFuncStart(d.dwOld) && !IsCodeSiteName(d.strName))
			{
				r.nStatus = GDS_SUSPECT;
				r.strDetail = _T("nằm giữa hàm nhưng tên không giống vị trí lệnh (giá trị cũ đã sai?); ") + r.strDetail;
			}
			break;
		case GDK_DATA:
			ResolveAddr(d.dwOld, FALSE, r);
			if (r.nStatus == GDS_NOTFOUND)
			{
				RESULT rd;
				if (ResolveDerived(d.dwOld, rd)) r = rd;
			}
			break;
		case GDK_CONST:
			ResolveConst(d.dwOld, r);
			break;
		default:
			if (d.dwOld < 0x80) r.strDetail = _T("số nhỏ, không dò được");
			else r.strDetail = _T("nằm ngoài image game (cờ / địa chỉ heap)");
			break;
		}
		d.nStatus = r.nStatus;
		d.dwNew = r.dwNew;
		d.nVotes = r.nVotes;
		d.nTotal = r.nTotal;
		d.strDetail = r.strDetail;
	}

	// Lan 2: dich mau theo cac dia chi da do chac chan cho cac ham / vi tri lenh chua tim thay
	m_Map.clear();
	for (auto &d : m_Defines)
		if ((d.nKind == GDK_CODE || d.nKind == GDK_DATA) && (d.nStatus == GDS_SAME || d.nStatus == GDS_CHANGED || d.nStatus == GDS_DERIVED))
			m_Map[d.dwOld] = d.dwNew;
	for (auto &d : m_Defines)
	{
		if (pCancel && *pCancel) break;
		if (d.nKind != GDK_CODE || d.nStatus != GDS_NOTFOUND) continue;
		DWORD dwHit;
		if (!LocateTranslated(d.dwOld, dwHit)) continue;
		d.dwNew = dwHit;
		d.nStatus = (dwHit == d.dwOld) ? GDS_SAME : GDS_CHANGED;
		d.nVotes = d.nTotal = 1;
		d.strDetail = _T("dịch mẫu theo địa chỉ đã dò (biến / hàm trong mẫu)");
	}
	if (fnProgress) fnProgress(n, n);
}

//////////////////////////////////////////////////////////////////////////
// Ket qua

LPCTSTR CGameDiff::KindName(int nKind)
{
	switch (nKind)
	{
	case GDK_CODE:	return _T("Hàm/code");
	case GDK_DATA:	return _T("Biến");
	case GDK_CONST:	return _T("Offset");
	}
	return _T("-");
}

LPCTSTR CGameDiff::StatusName(int nStatus)
{
	switch (nStatus)
	{
	case GDS_SKIP:		return _T("Bỏ qua");
	case GDS_SAME:		return _T("Không đổi");
	case GDS_CHANGED:	return _T("Đã đổi");
	case GDS_DERIVED:	return _T("Suy ra");
	case GDS_AMBIG:		return _T("Nhiều ứng viên");
	case GDS_SUSPECT:	return _T("Cần kiểm tra");
	case GDS_NOTFOUND:	return _T("Không tìm thấy");
	}
	return _T("");
}

BOOL CGameDiff::CanApply(const GD_DEFINE &d) const
{
	return d.nStatus == GDS_SAME || d.nStatus == GDS_CHANGED || d.nStatus == GDS_DERIVED;
}

static BOOL WriteBytes(LPCTSTR lpszPath, const std::string &data)
{
	CFile file;
	if (!file.Open(lpszPath, CFile::modeCreate | CFile::modeWrite)) return FALSE;
	file.Write(data.data(), (UINT)data.size());
	file.Close();
	return TRUE;
}

BOOL CGameDiff::WriteHeaders(LPCTSTR lpszOutDir, BOOL bOverwrite, CString &strResult)
{
	strResult.Empty();
	if (!bOverwrite) SHCreateDirectoryEx(NULL, lpszOutDir, NULL);
	for (size_t f = 0; f < m_Files.size(); f++)
	{
		std::vector<std::string> lines = m_Files[f].lines;
		int nChanged = 0;
		for (auto &d : m_Defines)
		{
			if (d.nFile != (int)f || !CanApply(d) || d.dwNew == d.dwOld) continue;
			char szHex[16];
			sprintf_s(szHex, d.bLower ? "%0*x" : "%0*X", d.nValLen, d.dwNew);
			lines[d.nLine].replace(d.nValPos, d.nValLen, szHex);
			nChanged++;
		}
		std::string data;
		for (auto &s : lines) data += s;

		CString strSrc = m_Files[f].strPath;
		CString strName = strSrc.Mid(strSrc.ReverseFind(_T('\\')) + 1);
		CString strDst;
		if (bOverwrite)
		{
			if (!nChanged) continue;
			CopyFile(strSrc, strSrc + _T(".bak"), FALSE);
			strDst = strSrc;
		}
		else
		{
			// Trung ten file (vd 2 thu muc cung co GameCore.h): them ten thu muc cha
			for (size_t g = 0; g < m_Files.size(); g++)
			{
				CString s = m_Files[g].strPath;
				if (g != f && s.Mid(s.ReverseFind(_T('\\')) + 1).CompareNoCase(strName) == 0)
				{
					CString strDir = strSrc.Left(strSrc.ReverseFind(_T('\\')));
					strName = strDir.Mid(strDir.ReverseFind(_T('\\')) + 1) + _T("_") + strName;
					break;
				}
			}
			strDst = CString(lpszOutDir) + _T("\\") + strName;
		}
		if (!WriteBytes(strDst, data))
		{
			strResult.Format(_T("Không ghi được file %s"), (LPCTSTR)strDst);
			return FALSE;
		}
		CString t;
		t.Format(_T("%s: %d giá trị thay đổi\r\n"), (LPCTSTR)strDst, nChanged);
		strResult += t;
	}
	if (strResult.IsEmpty()) strResult = _T("Không có giá trị nào thay đổi.");
	return TRUE;
}

static std::string ToUtf8(const CString &s)
{
	int n = WideCharToMultiByte(CP_UTF8, 0, s, s.GetLength(), NULL, 0, NULL, NULL);
	std::string out(n, '\0');
	if (n) WideCharToMultiByte(CP_UTF8, 0, s, s.GetLength(), &out[0], n, NULL, NULL);
	return out;
}

BOOL CGameDiff::WriteReport(LPCTSTR lpszPath, CString &strErr)
{
	int nCount[GDS_NOTFOUND + 1] = { 0 };
	for (auto &d : m_Defines) nCount[d.nStatus]++;

	CString s, t;
	CTime now = CTime::GetCurrentTime();
	s.Format(_T("UpdateGameData - %s\r\n"), (LPCTSTR)now.Format(_T("%d/%m/%Y %H:%M:%S")));
	t.Format(_T("Bản cũ : %s (code 0x%08X - 0x%08X)\r\nBản mới: %s (code 0x%08X - 0x%08X)\r\n"),
		(LPCTSTR)m_Ref.m_strPath, m_Ref.m_dwCodeBeg, m_Ref.m_dwCodeEnd, (LPCTSTR)m_Tgt.m_strPath, m_Tgt.m_dwCodeBeg, m_Tgt.m_dwCodeEnd);
	s += t;
	t.Format(_T("Tổng %d: Không đổi %d, Đã đổi %d, Suy ra %d, Nhiều ứng viên %d, Cần kiểm tra %d, Không tìm thấy %d, Bỏ qua %d\r\n")
		_T("Chỉ các mục Không đổi / Đã đổi / Suy ra được ghi vào file .h; các mục khác giữ nguyên giá trị cũ.\r\n\r\n"),
		(int)m_Defines.size(), nCount[GDS_SAME], nCount[GDS_CHANGED], nCount[GDS_DERIVED], nCount[GDS_AMBIG], nCount[GDS_SUSPECT],
		nCount[GDS_NOTFOUND], nCount[GDS_SKIP]);
	s += t;
	t.Format(_T("%-20s %-9s %-10s %-10s %-7s %-15s %-22s %s\r\n"), _T("Tên"), _T("Loại"), _T("Cũ"), _T("Mới"), _T("Phiếu"), _T("Trạng thái"), _T("File:dòng"), _T("Chi tiết"));
	s += t;
	for (auto &d : m_Defines)
	{
		CString strFile = m_Files[d.nFile].strPath;
		strFile = strFile.Mid(strFile.ReverseFind(_T('\\')) + 1);
		CString strPos, strVotes;
		strPos.Format(_T("%s:%d"), (LPCTSTR)strFile, d.nLine + 1);
		if (d.nTotal) strVotes.Format(_T("%d/%d"), d.nVotes, d.nTotal);
		t.Format(_T("%-20s %-9s 0x%08X 0x%08X %-7s %-15s %-22s %s\r\n"), (LPCTSTR)CString(d.strName), KindName(d.nKind),
			d.dwOld, d.dwNew, (LPCTSTR)strVotes, StatusName(d.nStatus), (LPCTSTR)strPos, (LPCTSTR)d.strDetail);
		s += t;
	}

	std::string data = "\xEF\xBB\xBF" + ToUtf8(s);
	if (!WriteBytes(lpszPath, data))
	{
		strErr.Format(_T("Không ghi được file %s"), lpszPath);
		return FALSE;
	}
	return TRUE;
}
