// SealGate.cpp : see SealGate.h. Open the sealed offset blob with the per-session
// seal key K (from the signed activation token) and fill theGameData.
#include "stdafx.h"
#include "SealGate.h"
#include "Version.h"
#if LICENSE_REQUIRED              // ban offline-tester khong link lprotect -> bo qua
#include "VLAuto.h"               // theGameData, struct GameData
#include "../GameDef.h"           // CLIENT_SERIAL (mask transport obfuscation)
#include "lprotect/lprotect.h"    // lp::activation_seal_key, lp::seal_open
#include "SealedBlob.inc"         // g_SealBlob, g_SealBlob_len (ciphertext)

bool UnsealGameAddr()
{
    // K exists only after a real server activation (token carried it). If the
    // license gate was patched out, no server call happened, so no K -> locked.
    uint8_t K[32];
    if (!lp::activation_seal_key(K))
        return false;

    uint8_t pt[64];
    size_t n = lp::seal_open(K, g_SealBlob, g_SealBlob_len, pt, sizeof pt);

    if (n < 5 * sizeof(int))       // need the 5 core addresses
    {
        SecureZeroMemory(K, sizeof K);
        SecureZeroMemory(pt, sizeof pt);
        return false;
    }

    // 1.3: khong de dia chi dang plaintext trong theGameData / goi IPC. Mask moi dia chi
    // bang gia tri gap tu K (moi phien khac nhau); chuyen mask qua ExeFileSize (che bang
    // CLIENT_SERIAL). VLHook unmask khi dung (macro). Thieu K/mask dung => con tro deu sai.
    DWORD mask = 0;
    const DWORD* kd = reinterpret_cast<const DWORD*>(K);
    for (int i = 0; i < 8; i++) mask ^= kd[i];
    SecureZeroMemory(K, sizeof K);

    // 1.4: nhao challenge cua phien (nonce + server_time da ky) vao mask. Moi phien
    // server cap nonce/thoi gian khac => mask (va dia chi da mask) doi theo phien =>
    // snapshot/IPC cua phien cu khong tai dung, chu ky chan server gia cuc bo.
    uint8_t chal[16]; uint64_t stime = 0;
    if (lp::activation_session_challenge(chal, &stime))
    {
        const DWORD* cd = reinterpret_cast<const DWORD*>(chal);
        for (int i = 0; i < 4; i++) mask ^= cd[i];
        mask ^= (DWORD)stime ^ (DWORD)(stime >> 32);
        SecureZeroMemory(chal, sizeof chal);
    }
    const int m = (int)mask;

    // Field order matches the --u32 list the blob was generated from:
    //   Npc, Plr, MapID, Account, Core
    const int* a = reinterpret_cast<const int*>(pt);
    theGameData.NpcBaseAdd  = a[0] ^ m;
    theGameData.PlrBaseAdd  = a[1] ^ m;
    theGameData.MapAddress  = a[2] ^ m;
    theGameData.AccountID   = a[3] ^ m;
    theGameData.KCoreShell  = a[4] ^ m;
    theGameData.ExeFileSize = (int)(mask ^ (DWORD)CLIENT_SERIAL);

    SecureZeroMemory(pt, sizeof pt);
    return true;
}
#endif // LICENSE_REQUIRED
