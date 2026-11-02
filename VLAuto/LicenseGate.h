// LicenseGate.h : online single-session license gate for VLAuto (LPROTECT).
//
// Chinh sach "oldest-wins": chi mot ban client duoc chay cung luc (theo tai
// khoan neu key da bind account, neu khong thi theo key). Mo ban thu hai ->
// bi tu choi (SessionInUse).
//
//   InitInstance():  if (!LicenseGateStartup()) return FALSE;   // chan & thoat
//   ... sau khi main dialog dong:  LicenseGateShutdown();        // tra lease
#pragma once
#include "Version.h"

// Hien dialog dang nhap, dang nhap + gianh lease single-session.
// Tra ve TRUE neu duoc phep chay; FALSE neu nguoi dung huy hoac bi chan.
bool LicenseGateStartup();

// Tra lease tren server khi thoat sach (dung guard, xoa sid cuc bo). An toan
// goi nhieu lan / khi chua Startup.
void LicenseGateShutdown();

#if LP_HARDEN
// Phase 2.1: snapshot CRC vung .text cua EXE luc khoi dong roi verify dinh ky tren
// mot thread nen; neu bytes dang chay != baseline (hook/patch runtime) -> LP_TAMPER
// (dat co; degrade xay ra tre o GMASK_V). Goi 1 lan, som trong InitInstance.
// Dong thoi lap watchdog anti-debug (Phase 3) + anti-attach (ban that).
void IntegrityGuardStart();

// Phase 4.1: scrub PE header trong bo nho (chong dump). Goi MUON, truoc DoModal.
void HardenScrub();
#endif
