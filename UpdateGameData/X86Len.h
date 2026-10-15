// X86Len.h : giai ma do dai lenh x86 32-bit (chi lay vi tri cac truong disp/imm/rel)
#pragma once

struct X86Ins
{
	int		nLen;			// Do dai lenh
	BYTE	byOp;			// Opcode (byte sau prefix, hoac byte sau 0F)
	BOOL	b0F;			// Opcode 2 byte (0F xx)
	int		nDispOff;		// disp: vi tri trong lenh, kich thuoc (0/1/2/4)
	int		nDispSize;
	BOOL	bDispAbs;		// disp la dia chi tuyet doi ([disp32], moffs)
	int		nImmOff;		// imm: vi tri, kich thuoc (0/1/2/4)
	int		nImmSize;
	int		nRelOff;		// rel cua call/jmp/jcc: vi tri, kich thuoc (0/1/2/4)
	int		nRelSize;
};

// Giai ma 1 lenh tai p (con nAvail byte). FALSE: khong phai lenh hop le
BOOL X86Decode(const BYTE *p, int nAvail, X86Ins &ins);
