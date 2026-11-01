// LicenseGate.h : online single-session license gate for VLAuto (LPROTECT).
//
// Chinh sach "oldest-wins": chi mot ban client duoc chay cung luc (theo tai
// khoan neu key da bind account, neu khong thi theo key). Mo ban thu hai ->
// bi tu choi (SessionInUse).
//
//   InitInstance():  if (!LicenseGateStartup()) return FALSE;   // chan & thoat
//   ... sau khi main dialog dong:  LicenseGateShutdown();        // tra lease
#pragma once

// Hien dialog dang nhap, dang nhap + gianh lease single-session.
// Tra ve TRUE neu duoc phep chay; FALSE neu nguoi dung huy hoac bi chan.
bool LicenseGateStartup();

// Tra lease tren server khi thoat sach (dung guard, xoa sid cuc bo). An toan
// goi nhieu lan / khi chua Startup.
void LicenseGateShutdown();
