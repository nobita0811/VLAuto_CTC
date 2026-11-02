#pragma once
// SealGate: giai "blob offset" (cac dia chi game) bang khoa seal K lay tu token
// activation (chi co sau khi dang nhap va session_begin thanh cong), roi dien vao
// theGameData. Khong co K hop le (vd bi patch bo cong license, khong goi server)
// => khong mo duoc blob => theGameData khong co offset => tool vo dung.
//
// Phase 1.2b: moi chi DIEN theGameData (v"n duoc WM_HOOK_EXDATA day sang VLHook
// san). Viec CHUYEN macro sang dung theGameData (tat _AUTOXP) la buoc 1.2c.
//
// Tra ve true neu lay duoc K va mo blob thanh cong.
bool UnsealGameAddr();
