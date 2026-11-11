// Harden.h : Phase 0 - khung PHAN UNG TRE cho anti-tamper.
//
// Nguyen tac: checker (integrity P2, anti-debug P3...) khi phat hien tamper thi
// goi LP_TAMPER("noi") -> KHONG giet tai cho (de bi NOP/lan ra). Thay vao do dat
// mot co; viec "lam hong" xay ra SAU, o cho khac, qua LP_POISON() tron vao gia tri
// runtime. Nho vay diem check va diem phan ung tach roi nhau.
//
// Che do (quyet dinh o Version.h theo ban build):
//   LP_HARDEN=0      (Tester offline): macro bay hoi hoan toan - khong code, khong co.
//   LP_HARDEN_SAFE=1 (Tester online) : chi GHI LOG, LP_POISON() luon 0 (khong degrade).
//   LP_HARDEN=1,SAFE=0 (User)        : dat co that; LP_POISON() != 0 sau khi bi flag.
#pragma once
#include "Version.h"

#if LP_HARDEN

namespace hard {
    // Danh dau da phat hien tamper tai `where` (chuoi tinh de doc trong log).
    void Flag(const char* where);
    // Da tung bi flag chua (luon false o SAFE mode).
    bool Tripped();
    // Mask de XOR vao gia tri runtime: 0 khi sach, != 0 khi da tamper (va khong
    // o SAFE mode). Goi RAI RAC, cach xa diem Flag, de kho lan nguoc.
    unsigned int PoisonMask();
    // CHAN DOAN (chi dung cho log): checker nao bay DAU TIEN va luc nao (GetTickCount).
    // Tra "" / 0 khi chua trip hoac o SAFE mode. KHONG anh huong co che degrade.
    const char*   TripWhere();
    unsigned long TripTick();
}

#define LP_TAMPER(where)	(::hard::Flag(where))
#define LP_TRIPPED()		(::hard::Tripped())
#define LP_POISON()			(::hard::PoisonMask())
#define LP_TRIP_WHERE()		(::hard::TripWhere())
#define LP_TRIP_TICK()		(::hard::TripTick())

#else   // LP_HARDEN == 0 : khong dinh gi (Tester offline)

#define LP_TAMPER(where)	((void)0)
#define LP_TRIPPED()		(false)
#define LP_POISON()			(0u)
#define LP_TRIP_WHERE()		("")
#define LP_TRIP_TICK()		(0ul)

#endif
