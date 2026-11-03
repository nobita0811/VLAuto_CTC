#pragma once

// Goi du lieu MapPath (VLPathPak.cpp): Maps\Path\*.txt -> Maps\Path.pak.
// Nen LZ kieu NRV2B (nhu UCL trong file pak cua game) nhung doi dinh dang bit, them ma hoa chi muc + du lieu
// => cong cu giai nen pak JX khong doc duoc. VLDebugPath doc Maps\Path truoc (de test/sua), khong co moi lay Path.pak

// Noi dung file path cua map nMapID trong Path.pak (TCVN3, ket thuc 0). free() sau khi dung. NULL = khong co
char*		PakPathRead(int nMapID, int *pnSize);
// Cac MapID co trong Path.pak. Tra ve so map (toi da nMax)
int			PakPathMaps(int *pMaps, int nMax);
// Nen Maps\Path\*.txt -> Maps\Path.pak (giai nen lai de kiem tra truoc khi ghi).
// Tra ve so file; -1 khong co file, -2 khong ghi duoc, -3 kiem tra giai nen sai. *pnRaw/*pnPack = tong kich thuoc
int			PakPathBuild(int *pnRaw, int *pnPack);
