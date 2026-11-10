#pragma once

// Debug log Sua do -> <thu muc auto>\Logs\SuaDo.log (code o VLMap.cpp; goi tu VLMap.cpp + VLRecover.cpp)
// Ghi: danh sach do khi ve thanh (mon nao can sua, sua o cua hang nao), moi lenh sua (do gi, do ben, gia uoc tinh, tien,
// trong thanh hay tren map), bang hoi "Sua ky [..]" do Hoang Kim (cac dong, bam dong nao), ket qua (do ben co day khong,
// mat bao nhieu tien), quet tu sua tren map moi 30s (vi sao sua / khong sua).
// 1 = bat, 0 = tat (khong bien dich code log)
#define DebugSuaDo 1

#if DebugSuaDo
void	SuaDoLog(const char* fmt, ...);
void	SuaDoEquipDump(const char* szWhy);				// Liet ke trang bi + do ben + gia sua
void	SuaDoSent(int nItemIdx, const char* szWho);		// Vua goi RepairItem: ghi + cho ket qua
void	SuaDoPoll();									// Moi vong timer (CVLRecover::CheckEquip): ket qua sua, bang hoi sua do
#endif
void	SuaDoLogFlush();								// Ghi bo dem ra file (go hook / thoat game)
