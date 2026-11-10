#pragma once

// Bo dem log debug (VLAttack.cpp): gom nhieu dong trong RAM, vuot nguong / qua thoi gian moi ghi file 1 lan.
// Dung cho cuusat.log, danhquai.log, DiChuyen.log
struct KBufLog
{
	const char*	szName;		// Ten file (khong duoi) trong <auto>\Logs
	int			nMaxSize;	// File lon hon => doi ten <ten>_bak.log, mo file moi
	int			nBufSize;
	int			nFlushAt;	// Bo dem vuot nguong => ghi
	DWORD		dwFlushTime;// Hoac qua thoi gian nay chua ghi => ghi
	char*		pBuf;
	int			nLen;
	DWORD		dwFlushTick;
};

void	BLFlush(KBufLog* L);
void	BLAppend(KBufLog* L, const char* szTcvnLine);	// 1 dong TCVN3 (doi sang UTF-8, them CRLF)
void	DebugLogFlushAll();								// Ghi het cac bo dem (go hook / thoat game)
