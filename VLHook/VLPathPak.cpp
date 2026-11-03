#ifndef PAK_CODEC_ONLY
#include "stdafx.h"
#include "../GameDef.h"
#include "GameBase.h"
#include "JxCore.h"
#include "S3Client.h"
#include "VLClasses.h"
#include "VLPathPak.h"
#endif

// Path.pak: [PAK_HDR][PAK_ENT x nCount (ma hoa)][du lieu nen + ma hoa cua tung file]
// Nen: LZ77 + ma Elias gamma xen bit nhu NRV2B (ucl_nrv2b_decompress_8 cua XPackFile) nhung khac:
//   bit doc tu bit thap, 0 = ky tu thuong / 1 = doan lap, co gamma 1 = con / 0 = het, byte thap offset XOR,
//   khong co ma ket thuc (dung theo kich thuoc goc). Sau khi nen XOR them chuoi xorshift theo salt + map + offset

#define PAK_SIG			0x8E3A51C7
#define PAK_KEY_IDX		0x5BD1E995
#define PAK_KEY_DAT		0xC2B2AE35
#define PAK_MAX_FILES	1024
#define PAK_MAX_LEN		4096		// doan lap dai nhat
#define PAK_MAX_OFF		0xFFFFF
#define PAK_HASH_BITS	13
#define PAK_CHAIN		256			// so ung vien toi da moi vi tri

struct PAK_HDR { DWORD dwSig, dwCount, dwSalt, dwIdxHash; };
struct PAK_ENT { int nMap; DWORD dwOfs, dwSize, dwPack, dwHash; WCHAR wszName[58]; };

static DWORD PakHash(const BYTE *p, DWORD n)
{
	DWORD h = 0x811C9DC5;
	for (DWORD i = 0; i < n; i++) { h ^= p[i]; h *= 0x01000193; }
	return h;
}

// XOR chuoi xorshift32 (ma hoa = giai ma)
static void PakCrypt(BYTE *p, DWORD n, DWORD dwSeed)
{
	DWORD s = dwSeed ? dwSeed : 0x6D2B79F5;
	for (DWORD i = 0; i < n; i++)
	{
		s ^= s << 13; s ^= s >> 17; s ^= s << 5;
		p[i] ^= (BYTE)((s >> 7) + i);
	}
}

static DWORD PakDataSeed(const PAK_ENT &e, DWORD dwSalt)
{
	return PAK_KEY_DAT ^ dwSalt ^ ((DWORD)e.nMap * 0x9E3779B1) ^ e.dwOfs;
}

//------------------------------------------------------------------ nen / giai nen

struct PAK_BITW { BYTE *p; DWORD n, nPos; int k; };

static void PutBit(PAK_BITW &w, int b)
{
	if (w.k == 8) { w.nPos = w.n++; w.p[w.nPos] = 0; w.k = 0; }	// giu cho 1 byte bit, byte du lieu ghi tiep sau
	if (b) w.p[w.nPos] |= (BYTE)(1 << w.k);
	w.k++;
}

// v >= 2: cac bit duoi bit cao nhat, moi bit kem co 1 = con / 0 = het
static void PutGamma(PAK_BITW &w, DWORD v)
{
	int h = 31;
	while (!(v >> h)) h--;
	for (int i = h - 1; i >= 0; i--) { PutBit(w, (v >> i) & 1); PutBit(w, i > 0); }
}

static DWORD MatchLen(const BYTE *a, const BYTE *b, DWORD nMax)
{
	DWORD n = 0;
	while (n < nMax && a[n] == b[n]) n++;
	return n;
}

static DWORD Hash3(const BYTE *p)
{
	return (((DWORD)p[0] << 16 | (DWORD)p[1] << 8 | p[2]) * 2654435761u) >> (32 - PAK_HASH_BITS);
}

// dst >= n + n / 8 + 64. Tra ve kich thuoc nen
static DWORD PakCompress(const BYTE *src, DWORD n, BYTE *dst)
{
	PAK_BITW w = { dst, 0, 0, 8 };
	static int head[1 << PAK_HASH_BITS];
	int *prev = (int*)malloc((n ? n : 1) * sizeof(int));
	memset(head, -1, sizeof(head));
	DWORD i = 0, dwLast = 0;
	while (i < n)
	{
		DWORD nMax = min(n - i, (DWORD)PAK_MAX_LEN), nBest = 0, dwBest = 0, nScore = 0;
		if (dwLast && dwLast <= i)
		{
			DWORD len = MatchLen(src + i, src + i - dwLast, nMax);
			if (len >= 2) { nBest = len; dwBest = dwLast; nScore = len + 1; }	// lap offset cu re hon 1 byte
		}
		if (i + 3 <= n)
		{
			int d = 0;
			for (int j = head[Hash3(src + i)]; j >= 0 && d < PAK_CHAIN && i - j <= PAK_MAX_OFF; j = prev[j], d++)
			{
				DWORD len = MatchLen(src + i, src + j, nMax);
				if (len >= 3 && len > nScore) { nBest = len; dwBest = i - j; nScore = len; }
			}
		}
		DWORD nStep = nBest ? nBest : 1;
		if (!nBest)
		{
			PutBit(w, 0);
			dst[w.n++] = src[i];
		}
		else
		{
			PutBit(w, 1);
			if (dwBest == dwLast) PutGamma(w, 2);
			else
			{
				PutGamma(w, ((dwBest - 1) >> 8) + 3);
				dst[w.n++] = (BYTE)((dwBest - 1) ^ 0xA5);
				dwLast = dwBest;
			}
			PutGamma(w, nBest);
		}
		for (DWORD k = 0; k < nStep; k++, i++)
			if (i + 3 <= n) { DWORD h = Hash3(src + i); prev[i] = head[h]; head[h] = (int)i; }
	}
	free(prev);
	return w.n;
}

struct PAK_BITR { const BYTE *p; DWORD n, i; int k; BYTE bb; BOOL bBad; };

static int GetBit(PAK_BITR &r)
{
	if (r.k == 8)
	{
		if (r.i >= r.n) { r.bBad = TRUE; return 0; }
		r.bb = r.p[r.i++]; r.k = 0;
	}
	return (r.bb >> r.k++) & 1;
}

static DWORD GetGamma(PAK_BITR &r)
{
	DWORD v = 1;
	do
	{
		v = v * 2 + GetBit(r);
		if (v > 0x1000000) r.bBad = TRUE;
	} while (!r.bBad && GetBit(r));
	return v;
}

static BOOL PakDecompress(const BYTE *src, DWORD nSrc, BYTE *dst, DWORD nDst)
{
	PAK_BITR r = { src, nSrc, 0, 8, 0, FALSE };
	DWORD o = 0, dwLast = 0;
	while (o < nDst)
	{
		if (!GetBit(r))
		{
			if (r.bBad || r.i >= r.n) return FALSE;
			dst[o++] = r.p[r.i++];
			continue;
		}
		DWORD v = GetGamma(r), dwOff;
		if (r.bBad) return FALSE;
		if (v == 2) dwOff = dwLast;
		else
		{
			if (r.i >= r.n) return FALSE;
			dwOff = (((v - 3) << 8) | (BYTE)(r.p[r.i++] ^ 0xA5)) + 1;
			dwLast = dwOff;
		}
		DWORD nLen = GetGamma(r);
		if (r.bBad || !dwOff || dwOff > o || nLen > nDst - o) return FALSE;
		for (DWORD k = 0; k < nLen; k++, o++) dst[o] = dst[o - dwOff];
	}
	return !r.bBad;
}

#ifndef PAK_CODEC_ONLY

//------------------------------------------------------------------ doc Path.pak (giu trong bo nho, doc lai khi file doi)

static struct
{
	BYTE		*pFile;
	DWORD		nFile, dwSalt;
	PAK_ENT		*pEnt;
	DWORD		nEnt;
	FILETIME	ft;
	DWORD		dwSize;		// kich thuoc file luc doc (cung ft: khong doc lai)
	BOOL		bTried;
} s_Pak;

static void PakPathFile(WCHAR *wszFile)
{
	swprintf(wszFile, MAX_PATH, L"%hs\\Maps\\Path.pak", theMain.m_szCurFolder);
}

static void PakClose()
{
	free(s_Pak.pFile);
	free(s_Pak.pEnt);
	memset(&s_Pak, 0, sizeof(s_Pak));
}

static BOOL PakOpen()
{
	WCHAR wszFile[MAX_PATH];
	PakPathFile(wszFile);
	WIN32_FILE_ATTRIBUTE_DATA fa;
	if (!GetFileAttributesExW(wszFile, GetFileExInfoStandard, &fa)) { PakClose(); return FALSE; }
	if (s_Pak.bTried && !CompareFileTime(&s_Pak.ft, &fa.ftLastWriteTime) && s_Pak.dwSize == fa.nFileSizeLow)
		return s_Pak.nEnt > 0;
	PakClose();
	s_Pak.bTried = TRUE;
	s_Pak.ft = fa.ftLastWriteTime;
	s_Pak.dwSize = fa.nFileSizeLow;
	if (fa.nFileSizeHigh || fa.nFileSizeLow < sizeof(PAK_HDR)) return FALSE;
	FILE *pFile = _wfopen(wszFile, L"rb");
	if (!pFile) return FALSE;
	BYTE *p = (BYTE*)malloc(fa.nFileSizeLow);
	DWORD n = p ? (DWORD)fread(p, 1, fa.nFileSizeLow, pFile) : 0;
	fclose(pFile);
	PAK_HDR hdr;
	if (n != fa.nFileSizeLow) { free(p); return FALSE; }
	memcpy(&hdr, p, sizeof(hdr));
	DWORD nCount = hdr.dwCount ^ (hdr.dwSalt * 0x2545F491);
	if (hdr.dwSig != PAK_SIG || !nCount || nCount > PAK_MAX_FILES || sizeof(hdr) + nCount * sizeof(PAK_ENT) > n) { free(p); return FALSE; }
	PAK_ENT *pEnt = (PAK_ENT*)malloc(nCount * sizeof(PAK_ENT));
	memcpy(pEnt, p + sizeof(hdr), nCount * sizeof(PAK_ENT));
	PakCrypt((BYTE*)pEnt, nCount * sizeof(PAK_ENT), PAK_KEY_IDX ^ hdr.dwSalt);
	if (PakHash((BYTE*)pEnt, nCount * sizeof(PAK_ENT)) != hdr.dwIdxHash) { free(pEnt); free(p); return FALSE; }
	s_Pak.pFile = p; s_Pak.nFile = n; s_Pak.dwSalt = hdr.dwSalt;
	s_Pak.pEnt = pEnt; s_Pak.nEnt = nCount;
	return TRUE;
}

static char* PakUnpack(const BYTE *pPack, const PAK_ENT &e, DWORD dwSalt)
{
	BYTE *pTmp = (BYTE*)malloc(e.dwPack ? e.dwPack : 1);
	char *pOut = (char*)malloc(e.dwSize + 1);
	memcpy(pTmp, pPack, e.dwPack);
	PakCrypt(pTmp, e.dwPack, PakDataSeed(e, dwSalt));
	BOOL bOk = PakDecompress(pTmp, e.dwPack, (BYTE*)pOut, e.dwSize) && PakHash((BYTE*)pOut, e.dwSize) == e.dwHash;
	free(pTmp);
	if (!bOk) { free(pOut); return NULL; }
	pOut[e.dwSize] = 0;
	return pOut;
}

char* PakPathRead(int nMapID, int *pnSize)
{
	if (nMapID <= 0 || !PakOpen()) return NULL;
	for (DWORD i = 0; i < s_Pak.nEnt; i++)
	{
		const PAK_ENT &e = s_Pak.pEnt[i];
		if (e.nMap != nMapID) continue;
		if (e.dwOfs > s_Pak.nFile || e.dwPack > s_Pak.nFile - e.dwOfs || e.dwSize > 0x1000000) return NULL;
		char *p = PakUnpack(s_Pak.pFile + e.dwOfs, e, s_Pak.dwSalt);
		if (p && pnSize) *pnSize = (int)e.dwSize;
		return p;
	}
	return NULL;
}

int PakPathMaps(int *pMaps, int nMax)
{
	if (!PakOpen()) return 0;
	int n = 0;
	for (DWORD i = 0; i < s_Pak.nEnt && n < nMax; i++)
		pMaps[n++] = s_Pak.pEnt[i].nMap;
	return n;
}

//------------------------------------------------------------------ tao Path.pak

int PakPathBuild(int *pnRaw, int *pnPack)
{
	static PAK_ENT Ent[PAK_MAX_FILES];
	static BYTE *pRaw[PAK_MAX_FILES], *pPack[PAK_MAX_FILES];
	int nEnt = 0, nRet = 0;
	*pnRaw = *pnPack = 0;
	WCHAR wszFind[MAX_PATH], wszFile[MAX_PATH];
	swprintf(wszFind, MAX_PATH, L"%hs\\Maps\\Path\\*.txt", theMain.m_szCurFolder);
	WIN32_FIND_DATAW fd;
	HANDLE hFind = FindFirstFileW(wszFind, &fd);
	if (hFind == INVALID_HANDLE_VALUE) return -1;
	do
	{
		int nMap = _wtoi(fd.cFileName);
		if (nMap <= 0 || fd.nFileSizeHigh || fd.nFileSizeLow > 0x1000000) continue;
		// Trung map (<MapID>.txt cu + <MapID>_<Ten>.txt): giu file co ten nhu FindPathFile
		int k = 0;
		while (k < nEnt && Ent[k].nMap != nMap) k++;
		BOOL bNamed = wcschr(fd.cFileName, L'_') != NULL;
		if (k < nEnt && (!bNamed || wcschr(Ent[k].wszName, L'_'))) continue;
		if (k == nEnt && nEnt >= PAK_MAX_FILES) continue;
		swprintf(wszFile, MAX_PATH, L"%hs\\Maps\\Path\\%s", theMain.m_szCurFolder, fd.cFileName);
		FILE *pFile = _wfopen(wszFile, L"rb");
		if (!pFile) continue;
		BYTE *p = (BYTE*)malloc(fd.nFileSizeLow ? fd.nFileSizeLow : 1);
		DWORD n = (DWORD)fread(p, 1, fd.nFileSizeLow, pFile);
		fclose(pFile);
		if (k < nEnt) free(pRaw[k]);
		else nEnt++;
		memset(&Ent[k], 0, sizeof(PAK_ENT));
		Ent[k].nMap = nMap;
		Ent[k].dwSize = n;
		wcsncpy(Ent[k].wszName, fd.cFileName, 57);
		pRaw[k] = p;
	} while (FindNextFileW(hFind, &fd));
	FindClose(hFind);
	if (!nEnt) return -1;

	PAK_HDR hdr;
	hdr.dwSig = PAK_SIG;
	hdr.dwSalt = GetTickCount() * 0x2C1B3C6D + (DWORD)nEnt;
	DWORD dwOfs = sizeof(PAK_HDR) + nEnt * sizeof(PAK_ENT);
	for (int k = 0; k < nEnt; k++)
	{
		PAK_ENT &e = Ent[k];
		pPack[k] = (BYTE*)malloc(e.dwSize + e.dwSize / 8 + 64);
		e.dwPack = PakCompress(pRaw[k], e.dwSize, pPack[k]);
		e.dwOfs = dwOfs;
		e.dwHash = PakHash(pRaw[k], e.dwSize);
		dwOfs += e.dwPack;
		*pnRaw += e.dwSize; *pnPack += e.dwPack;
		PakCrypt(pPack[k], e.dwPack, PakDataSeed(e, hdr.dwSalt));
		char *pChk = PakUnpack(pPack[k], e, hdr.dwSalt);	// kiem tra giai nen lai dung
		if (!pChk || memcmp(pChk, pRaw[k], e.dwSize)) nRet = -3;
		free(pChk);
	}
	hdr.dwIdxHash = PakHash((BYTE*)Ent, nEnt * sizeof(PAK_ENT));
	hdr.dwCount = (DWORD)nEnt ^ (hdr.dwSalt * 0x2545F491);
	if (!nRet)
	{
		WCHAR wszPak[MAX_PATH], wszTmp[MAX_PATH];
		PakPathFile(wszPak);
		swprintf(wszTmp, MAX_PATH, L"%s.tmp", wszPak);
		FILE *pFile = _wfopen(wszTmp, L"wb");
		if (!pFile) nRet = -2;
		else
		{
			static PAK_ENT Idx[PAK_MAX_FILES];	// ban ma hoa (Ent con dung o duoi)
			memcpy(Idx, Ent, nEnt * sizeof(PAK_ENT));
			PakCrypt((BYTE*)Idx, nEnt * sizeof(PAK_ENT), PAK_KEY_IDX ^ hdr.dwSalt);
			BOOL bOk = fwrite(&hdr, sizeof(hdr), 1, pFile) == 1 && fwrite(Idx, sizeof(PAK_ENT), nEnt, pFile) == (size_t)nEnt;
			for (int k = 0; k < nEnt && bOk; k++)
				bOk = fwrite(pPack[k], 1, Ent[k].dwPack, pFile) == Ent[k].dwPack;
			bOk = !fclose(pFile) && bOk;
			if (!bOk || !MoveFileExW(wszTmp, wszPak, MOVEFILE_REPLACE_EXISTING)) { DeleteFileW(wszTmp); nRet = -2; }
		}
	}
	for (int k = 0; k < nEnt; k++) { free(pRaw[k]); free(pPack[k]); }
	PakClose();		// lan doc sau nap lai file moi
	return nRet ? nRet : nEnt;
}

#endif
