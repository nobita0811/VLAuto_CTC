#pragma once

// Viet chuoi tieng Viet Unicode trong code, doi sang TCVN3 (bang ma game dung) luc chay:
//     SystemMessage(TV(L"Hủy nhiệm vụ!"));
// File .cpp chua L"tieng Viet" phai luu UTF-8 co BOM. Go Unicode dung san (khong dung to hop).

// Chi dung cho chuoi hang L"..." : cache theo dia chi chuoi, doi 1 lan, con tro tra ve song mai.
const char*	TV(const wchar_t* szUni);
// Doi vao bo dem cua nguoi goi (chuoi tam / chuoi ghep luc chay). Tra ve so byte (khong tinh 0).
int			UniToTcvn3(const wchar_t* szUni, char* szOut, int nOut);
// Chieu nguoc: chuoi TCVN3 (cua game) -> UTF-8, dung khi ghi file log. Tra ve so byte (khong tinh 0).
int			Tcvn3ToUtf8(const char* szTcvn, char* szOut, int nOut);
