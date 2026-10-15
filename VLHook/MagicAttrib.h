#pragma once

// Thuoc tinh dong phep cua item (10-10)
const char* MagicAttribName(int nType);			// ten ma trong game (vd "lifemax_v"), "?" neu khong co
const char* MagicAttribDesc(int nType);			// mo ta tieng Viet ngan (TCVN3), "" neu khong co
int MagicAttribTier(int nType, int nValue);		// cap 1..10 theo magicattriblevel.txt, 0 = thuoc tinh khong co trong bang
