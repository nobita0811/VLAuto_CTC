// X86Len.cpp : giai ma do dai lenh x86 32-bit
#include "stdafx.h"
#include "X86Len.h"

enum
{
	_M		= 0x0001,	// Co ModRM
	_I8		= 0x0002,	// imm8
	_I16	= 0x0004,	// imm16
	_IZ		= 0x0008,	// imm16/32 (theo operand size)
	_R8		= 0x0010,	// rel8
	_RZ		= 0x0020,	// rel16/32
	_MO		= 0x0040,	// moffs (dia chi tuyet doi, khong ModRM)
	_BAD	= 0x0080,	// Khong hop le
	_PFX	= 0x0100,	// Prefix
	_G3B	= 0x0200,	// F6: imm8 khi reg = 0/1
	_G3Z	= 0x0400,	// F7: immz khi reg = 0/1
	_ENT	= 0x0800,	// C8 enter: imm16 + imm8
	_PTR	= 0x1000,	// ptr16:32 (call far / jmp far)
	_ESC	= 0x2000,	// 0F
	_E38	= 0x4000,	// 0F 38 xx
	_E3A	= 0x8000,	// 0F 3A xx
};

#define MI8		(_M | _I8)
#define MIZ		(_M | _IZ)

static const WORD s_Op1[256] =
{
	// 0x00
	_M, _M, _M, _M, _I8, _IZ, 0, 0,				_M, _M, _M, _M, _I8, _IZ, 0, _ESC,
	// 0x10
	_M, _M, _M, _M, _I8, _IZ, 0, 0,				_M, _M, _M, _M, _I8, _IZ, 0, 0,
	// 0x20
	_M, _M, _M, _M, _I8, _IZ, _PFX, 0,			_M, _M, _M, _M, _I8, _IZ, _PFX, 0,
	// 0x30
	_M, _M, _M, _M, _I8, _IZ, _PFX, 0,			_M, _M, _M, _M, _I8, _IZ, _PFX, 0,
	// 0x40
	0, 0, 0, 0, 0, 0, 0, 0,						0, 0, 0, 0, 0, 0, 0, 0,
	// 0x50
	0, 0, 0, 0, 0, 0, 0, 0,						0, 0, 0, 0, 0, 0, 0, 0,
	// 0x60
	0, 0, _M, _M, _PFX, _PFX, _PFX, _PFX,		_IZ, MIZ, _I8, MI8, 0, 0, 0, 0,
	// 0x70
	_R8, _R8, _R8, _R8, _R8, _R8, _R8, _R8,		_R8, _R8, _R8, _R8, _R8, _R8, _R8, _R8,
	// 0x80
	MI8, MIZ, MI8, MI8, _M, _M, _M, _M,			_M, _M, _M, _M, _M, _M, _M, _M,
	// 0x90
	0, 0, 0, 0, 0, 0, 0, 0,						0, 0, _PTR, 0, 0, 0, 0, 0,
	// 0xA0
	_MO, _MO, _MO, _MO, 0, 0, 0, 0,				_I8, _IZ, 0, 0, 0, 0, 0, 0,
	// 0xB0
	_I8, _I8, _I8, _I8, _I8, _I8, _I8, _I8,		_IZ, _IZ, _IZ, _IZ, _IZ, _IZ, _IZ, _IZ,
	// 0xC0
	MI8, MI8, _I16, 0, _M, _M, MI8, MIZ,		_ENT, 0, _I16, 0, 0, _I8, 0, 0,
	// 0xD0
	_M, _M, _M, _M, _I8, _I8, 0, 0,				_M, _M, _M, _M, _M, _M, _M, _M,
	// 0xE0
	_R8, _R8, _R8, _R8, _I8, _I8, _I8, _I8,		_RZ, _RZ, _PTR, _R8, 0, 0, 0, 0,
	// 0xF0
	_PFX, 0, _PFX, _PFX, 0, 0, _M | _G3B, _M | _G3Z,	0, 0, 0, 0, 0, 0, _M, _M,
};

static const WORD s_Op0F[256] =
{
	// 0x00
	_M, _M, _M, _M, _BAD, 0, 0, 0,				0, 0, _BAD, 0, _BAD, _M, 0, MI8,
	// 0x10
	_M, _M, _M, _M, _M, _M, _M, _M,				_M, _M, _M, _M, _M, _M, _M, _M,
	// 0x20
	_M, _M, _M, _M, _BAD, _BAD, _BAD, _BAD,		_M, _M, _M, _M, _M, _M, _M, _M,
	// 0x30
	0, 0, 0, 0, 0, 0, _BAD, 0,					_E38, _BAD, _E3A, _BAD, _BAD, _BAD, _BAD, _BAD,
	// 0x40
	_M, _M, _M, _M, _M, _M, _M, _M,				_M, _M, _M, _M, _M, _M, _M, _M,
	// 0x50
	_M, _M, _M, _M, _M, _M, _M, _M,				_M, _M, _M, _M, _M, _M, _M, _M,
	// 0x60
	_M, _M, _M, _M, _M, _M, _M, _M,				_M, _M, _M, _M, _M, _M, _M, _M,
	// 0x70
	MI8, MI8, MI8, MI8, _M, _M, _M, 0,			_M, _M, _BAD, _BAD, _M, _M, _M, _M,
	// 0x80
	_RZ, _RZ, _RZ, _RZ, _RZ, _RZ, _RZ, _RZ,		_RZ, _RZ, _RZ, _RZ, _RZ, _RZ, _RZ, _RZ,
	// 0x90
	_M, _M, _M, _M, _M, _M, _M, _M,				_M, _M, _M, _M, _M, _M, _M, _M,
	// 0xA0
	0, 0, 0, _M, MI8, _M, _BAD, _BAD,			0, 0, 0, _M, MI8, _M, _M, _M,
	// 0xB0
	_M, _M, _M, _M, _M, _M, _M, _M,				_M, _M, MI8, _M, _M, _M, _M, _M,
	// 0xC0
	_M, _M, MI8, _M, MI8, MI8, MI8, _M,			0, 0, 0, 0, 0, 0, 0, 0,
	// 0xD0
	_M, _M, _M, _M, _M, _M, _M, _M,				_M, _M, _M, _M, _M, _M, _M, _M,
	// 0xE0
	_M, _M, _M, _M, _M, _M, _M, _M,				_M, _M, _M, _M, _M, _M, _M, _M,
	// 0xF0
	_M, _M, _M, _M, _M, _M, _M, _M,				_M, _M, _M, _M, _M, _M, _M, _M,
};

BOOL X86Decode(const BYTE *p, int nAvail, X86Ins &ins)
{
	memset(&ins, 0, sizeof(ins));
	if (nAvail <= 0) return FALSE;
	if (nAvail > 15) nAvail = 15;

	BOOL bOp16 = FALSE, bAd16 = FALSE;
	int i = 0;
	while (i < nAvail)
	{
		BYTE b = p[i];
		if (b == 0x66) bOp16 = TRUE;
		else if (b == 0x67) bAd16 = TRUE;
		else if (!(s_Op1[b] & _PFX)) break;
		i++;
	}
	if (i >= nAvail) return FALSE;

	BYTE byOp = p[i++];
	WORD wFlags = s_Op1[byOp];
	if (wFlags & _ESC)
	{
		if (i >= nAvail) return FALSE;
		ins.b0F = TRUE;
		byOp = p[i++];
		wFlags = s_Op0F[byOp];
		if (wFlags & (_E38 | _E3A))
		{
			if (i >= nAvail) return FALSE;
			i++;	// byte opcode thu 3
			wFlags = (wFlags & _E3A) ? MI8 : _M;
		}
	}
	if (wFlags & _BAD) return FALSE;
	ins.byOp = byOp;

	if (wFlags & _M)
	{
		if (i >= nAvail) return FALSE;
		BYTE byModRM = p[i++];
		int nMod = byModRM >> 6, nReg = (byModRM >> 3) & 7, nRM = byModRM & 7;
		if (nMod != 3)
		{
			if (!bAd16)
			{
				if (nRM == 4)
				{
					if (i >= nAvail) return FALSE;
					BYTE bySIB = p[i++];
					if (nMod == 0 && (bySIB & 7) == 5) { ins.nDispSize = 4; ins.bDispAbs = TRUE; }
				}
				else if (nMod == 0 && nRM == 5) { ins.nDispSize = 4; ins.bDispAbs = TRUE; }
				if (nMod == 1) ins.nDispSize = 1;
				else if (nMod == 2) ins.nDispSize = 4;
			}
			else
			{
				if (nMod == 0 && nRM == 6) { ins.nDispSize = 2; ins.bDispAbs = TRUE; }
				else if (nMod == 1) ins.nDispSize = 1;
				else if (nMod == 2) ins.nDispSize = 2;
			}
			if (ins.nDispSize)
			{
				ins.nDispOff = i;
				i += ins.nDispSize;
			}
		}
		if (((wFlags & _G3B) && nReg <= 1)) wFlags |= _I8;
		if (((wFlags & _G3Z) && nReg <= 1)) wFlags |= _IZ;
	}
	else if (wFlags & _MO)
	{
		ins.nDispOff = i;
		ins.nDispSize = bAd16 ? 2 : 4;
		ins.bDispAbs = TRUE;
		i += ins.nDispSize;
	}

	if (wFlags & (_I8 | _I16 | _IZ | _ENT))
	{
		ins.nImmOff = i;
		if (wFlags & _I8) ins.nImmSize = 1;
		else if (wFlags & (_I16 | _ENT)) ins.nImmSize = 2;
		else ins.nImmSize = bOp16 ? 2 : 4;
		i += ins.nImmSize;
		if (wFlags & _ENT) i += 1;
	}
	else if (wFlags & _PTR)
	{
		i += bOp16 ? 4 : 6;
	}
	else if (wFlags & (_R8 | _RZ))
	{
		ins.nRelOff = i;
		ins.nRelSize = (wFlags & _R8) ? 1 : (bOp16 ? 2 : 4);
		i += ins.nRelSize;
	}

	if (i > nAvail) return FALSE;
	ins.nLen = i;
	return TRUE;
}
