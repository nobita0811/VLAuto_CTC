// LicenseGate.cpp : online single-session license gate for VLAuto (LPROTECT).
// Account model: login = username + password, the machine is identified by a
// machine id (HWID) computed by the SDK and sent automatically. The license
// (plan/quota/expiry) lives on the account; there is no license key to type.
#include "stdafx.h"
#include "LicenseGate.h"
#include "SealGate.h"
#include "Version.h"
#include "Harden.h"
#include "License.h"
#if LICENSE_REQUIRED	// Ban tester offline (ADMIN_MODE 1, ADMIN_TESTER 0) khong dung license
#include "lprotect/lprotect.h"
#include "Resource.h"    // IDD_LOGIN_DIALOG + IDC_LICENSE_* (dialog tai nguyen)

#include <string>
#include <vector>
#include <wincrypt.h>
#pragma comment(lib, "crypt32.lib")   // DPAPI: CryptProtectData / CryptUnprotectData

// g_szIniFile duoc VLAuto.cpp dat trong InitInstance (truoc khi goi gate).
extern TCHAR g_szIniFile[MAX_PATH];

// ---------------------------------------------------------------------------
// Cau hinh
// ---------------------------------------------------------------------------
static const wchar_t* kEndpoint =
    L"https://lprotect-activation.longit0811.workers.dev/activate";
static const char*    kAppId    = "VLAutoCTC";

// ---------------------------------------------------------------------------
// Trang thai (de LicenseGateShutdown tra lease dung tai khoan)
// ---------------------------------------------------------------------------
static bool        g_started = false;
static std::string g_userUtf8;     // tai khoan dang giu lease (UTF-8), lam "key" cache

// Nho lai tai khoan + mat khau giua cac lan thu. Mat khau duoc ma hoa bang DPAPI
// (gan voi tai khoan Windows hien tai) truoc khi ghi ini - khong luu plaintext.
static CString  g_lastUser;
static CStringW g_lastPass;
static bool     g_remember  = true;    // "Ghi nho tai khoan" (mac dinh bat)
static bool     g_autoLogin = false;   // "Tu dong dang nhap" (bo qua dialog lan sau)

// (Dialog dang nhap dung tai nguyen IDD_LOGIN_DIALOG; id control o Resource.h.)

// ---------------------------------------------------------------------------
// Tien ich
// ---------------------------------------------------------------------------
static std::string W2U(const CStringW& s)
{
    if (s.IsEmpty()) return std::string();
    int n = ::WideCharToMultiByte(CP_UTF8, 0, s, s.GetLength(), NULL, 0, NULL, NULL);
    if (n <= 0) return std::string();
    std::string o((size_t)n, '\0');
    ::WideCharToMultiByte(CP_UTF8, 0, s, s.GetLength(), &o[0], n, NULL, NULL);
    return o;
}

// Machine id ("<PC>-<hash>") tu SDK, dung de hien thi va rang buoc may.
static CStringW MachineIdW()
{
    char buf[128] = {0};
    lp::activation_machine_id(buf, sizeof buf);
    return CStringW(buf);
}

// ---- hex + DPAPI (ma hoa mat khau theo tai khoan Windows) ------------------
static CString ToHex(const BYTE* p, DWORD n)
{
    CString s;
    for (DWORD i = 0; i < n; ++i) s.AppendFormat(_T("%02x"), p[i]);
    return s;
}

static bool FromHex(const CString& h, std::vector<BYTE>& out)
{
    int n = h.GetLength();
    if (n <= 0 || (n & 1)) return false;
    auto hv = [](wchar_t c) -> int {
        if (c >= L'0' && c <= L'9') return c - L'0';
        if (c >= L'a' && c <= L'f') return c - L'a' + 10;
        if (c >= L'A' && c <= L'F') return c - L'A' + 10;
        return -1;
    };
    out.clear(); out.reserve(n / 2);
    for (int i = 0; i < n; i += 2) {
        int hi = hv(h[i]), lo = hv(h[i + 1]);
        if (hi < 0 || lo < 0) return false;
        out.push_back((BYTE)((hi << 4) | lo));
    }
    return true;
}

static CString ProtectPass(const CStringW& plain)
{
    if (plain.IsEmpty()) return CString();
    DATA_BLOB in, out;
    in.pbData = (BYTE*)(LPCWSTR)plain;
    in.cbData = (DWORD)(plain.GetLength() * sizeof(wchar_t));
    if (!::CryptProtectData(&in, L"VLAutoCTC", NULL, NULL, NULL,
                            CRYPTPROTECT_UI_FORBIDDEN, &out))
        return CString();
    CString hex = ToHex(out.pbData, out.cbData);
    ::LocalFree(out.pbData);
    return hex;
}

static CStringW UnprotectPass(const CString& hex)
{
    std::vector<BYTE> blob;
    if (hex.IsEmpty() || !FromHex(hex, blob) || blob.empty()) return CStringW();
    DATA_BLOB in, out;
    in.pbData = blob.data();
    in.cbData = (DWORD)blob.size();
    if (!::CryptUnprotectData(&in, NULL, NULL, NULL, NULL,
                              CRYPTPROTECT_UI_FORBIDDEN, &out))
        return CStringW();
    CStringW s((const wchar_t*)out.pbData, (int)(out.cbData / sizeof(wchar_t)));
    ::LocalFree(out.pbData);
    return s;
}

static void LoadRemembered()
{
    wchar_t buf[2048];
    ::GetPrivateProfileStringW(L"License", L"User", L"", buf, 2048, g_szIniFile);
    g_lastUser = buf;
    ::GetPrivateProfileStringW(L"License", L"PassEnc", L"", buf, 2048, g_szIniFile);
    g_lastPass = UnprotectPass(CString(buf));
    // Co tai khoan nho san -> mac dinh "ghi nho" bat; doc co that tu ini neu co.
    g_remember  = ::GetPrivateProfileIntW(L"License", L"Remember",
                      g_lastUser.IsEmpty() ? 0 : 1, g_szIniFile) != 0;
    g_autoLogin = ::GetPrivateProfileIntW(L"License", L"AutoLogin", 0, g_szIniFile) != 0;
}

// Luu tai khoan + mat khau (DPAPI) + co ghi nho / tu dang nhap. Chi goi khi thanh cong.
static void SaveRemembered(const CString& user, const CStringW& pass,
                           bool remember, bool autoLogin)
{
    ::WritePrivateProfileStringW(L"License", L"User", user, g_szIniFile);
    CString enc = ProtectPass(pass);
    ::WritePrivateProfileStringW(L"License", L"PassEnc",
                                 enc.IsEmpty() ? NULL : (LPCWSTR)enc, g_szIniFile);
    ::WritePrivateProfileStringW(L"License", L"Remember",  remember  ? L"1" : L"0", g_szIniFile);
    ::WritePrivateProfileStringW(L"License", L"AutoLogin", autoLogin ? L"1" : L"0", g_szIniFile);
}

// Xoa thong tin nho (khi bo chon "ghi nho") - tranh de lo tai khoan o may chung.
static void ClearRemembered()
{
    ::WritePrivateProfileStringW(L"License", L"User",      NULL, g_szIniFile);
    ::WritePrivateProfileStringW(L"License", L"PassEnc",   NULL, g_szIniFile);
    ::WritePrivateProfileStringW(L"License", L"Remember",  L"0", g_szIniFile);
    ::WritePrivateProfileStringW(L"License", L"AutoLogin", L"0", g_szIniFile);
}

// ---------------------------------------------------------------------------
// Dialog dang nhap: dung tai nguyen co san IDD_LOGIN_DIALOG (VLAuto.rc).
// ---------------------------------------------------------------------------
namespace {

// Chep chuoi vao clipboard (cho nut "Coppy" ma may).
static void CopyToClipboard(HWND h, const CStringW& text)
{
    if (!::OpenClipboard(h)) return;
    ::EmptyClipboard();
    size_t bytes = (text.GetLength() + 1) * sizeof(wchar_t);
    HGLOBAL hg = ::GlobalAlloc(GMEM_MOVEABLE, bytes);
    if (hg) {
        void* p = ::GlobalLock(hg);
        if (p) { memcpy(p, (LPCWSTR)text, bytes); ::GlobalUnlock(hg);
                 ::SetClipboardData(CF_UNICODETEXT, hg); }
        else   { ::GlobalFree(hg); }
    }
    ::CloseClipboard();
}

struct LoginResult {
    CStringW user, pass, mid;
    bool remember;      // "Ghi nho tai khoan"
    bool autologin;     // "Tu dong dang nhap"
    bool ok;
};

INT_PTR CALLBACK LoginProc(HWND h, UINT msg, WPARAM wp, LPARAM lp)
{
    switch (msg)
    {
    case WM_INITDIALOG:
    {
        ::SetWindowLongPtr(h, DWLP_USER, (LONG_PTR)lp);
        LoginResult* r = (LoginResult*)lp;
        ::SetDlgItemTextW(h, IDC_LICENSE_ACCOUNT_EDIT,  r->user);
        ::SetDlgItemTextW(h, IDC_LICENSE_PASSWORD_EDIT, r->pass);
        ::SetDlgItemTextW(h, IDC_LICENSE_HWID_EDIT,     r->mid);
        ::SetDlgItemTextW(h, IDC_STATUS_STATIC,
            L"Đăng nhập để sử dụng Auto");
        ::SendDlgItemMessageW(h, IDC_LICENSE_ACCOUNT_EDIT,  EM_SETLIMITTEXT, 63, 0);
        ::SendDlgItemMessageW(h, IDC_LICENSE_PASSWORD_EDIT, EM_SETLIMITTEXT, 254, 0);
        ::CheckDlgButton(h, IDC_SAVELOGIN_CHECK, r->remember  ? BST_CHECKED : BST_UNCHECKED);
        ::CheckDlgButton(h, IDC_AUTOLOGIN_CHECK, r->autologin ? BST_CHECKED : BST_UNCHECKED);
        ::SetForegroundWindow(h);
        // Focus o truong trong dau tien; neu da dien du thi focus nut Dang nhap.
        int focusId = r->user.IsEmpty() ? IDC_LICENSE_ACCOUNT_EDIT
                    : r->pass.IsEmpty() ? IDC_LICENSE_PASSWORD_EDIT : IDC_LOGIN_BUTTON;
        ::SetFocus(::GetDlgItem(h, focusId));
        return FALSE; // da tu dat focus
    }
    case WM_COMMAND:
        switch (LOWORD(wp))
        {
        case IDC_LOGIN_BUTTON:   // nut mac dinh (Enter cung vao day)
        case IDOK:
        {
            LoginResult* r = (LoginResult*)::GetWindowLongPtr(h, DWLP_USER);
            wchar_t buf[256];
            ::GetDlgItemTextW(h, IDC_LICENSE_ACCOUNT_EDIT, buf, 256);
            CStringW user(buf); user.Trim();
            if (user.IsEmpty())
            {
                ::SetDlgItemTextW(h, IDC_STATUS_STATIC, L"Vui lòng nhập tài khoản");
                ::SetFocus(::GetDlgItem(h, IDC_LICENSE_ACCOUNT_EDIT));
                return TRUE;
            }
            ::GetDlgItemTextW(h, IDC_LICENSE_PASSWORD_EDIT, buf, 256); CStringW pass(buf);
            if (pass.IsEmpty())
            {
                ::SetDlgItemTextW(h, IDC_STATUS_STATIC, L"Vui lòng nhập mật khẩu");
                ::SetFocus(::GetDlgItem(h, IDC_LICENSE_PASSWORD_EDIT));
                return TRUE;
            }
            r->user = user;
            r->pass = pass;         // KHONG trim mat khau
            r->remember  = (::IsDlgButtonChecked(h, IDC_SAVELOGIN_CHECK) == BST_CHECKED);
            r->autologin = (::IsDlgButtonChecked(h, IDC_AUTOLOGIN_CHECK) == BST_CHECKED);
            if (r->autologin) r->remember = true;   // tu dang nhap thi buoc phai nho
            r->ok = true;
            ::EndDialog(h, 1);
            return TRUE;
        }
        case IDC_LOGIN_BUTTON2:  // "Coppy" -> chep ma may (HWID) vao clipboard
        {
            LoginResult* r = (LoginResult*)::GetWindowLongPtr(h, DWLP_USER);
            if (r) CopyToClipboard(h, r->mid);
            ::SetDlgItemTextW(h, IDC_STATUS_STATIC, L"Đã coppy,liên hệ Tuyền (zalo : 0974 921 182)");
            return TRUE;
        }
        case IDCANCEL:
        {
            LoginResult* r = (LoginResult*)::GetWindowLongPtr(h, DWLP_USER);
            if (r) r->ok = false;
            ::EndDialog(h, 0);
            return TRUE;
        }
        }
        break;
    }
    return FALSE;
}

// Hien dialog dang nhap (tai nguyen IDD_LOGIN_DIALOG). Tra ve true neu Dang nhap,
// false neu huy (ESC/dong cua so) hoac loi.
bool ShowLoginDialog(LoginResult& out)
{
    out.user      = g_lastUser;
    out.pass      = g_lastPass;
    out.mid       = MachineIdW();
    out.remember  = g_remember;
    out.autologin = g_autoLogin;
    out.ok        = false;

    INT_PTR r = ::DialogBoxParamW(
        AfxGetInstanceHandle(), MAKEINTRESOURCEW(IDD_LOGIN_DIALOG),
        NULL, LoginProc, (LPARAM)&out);
    return (r == 1) && out.ok;
}

// Guard chay tren thread nen: khi mat phien / tai khoan bi khoa / het han -> bao va dong app.
void OnSessionLost(lp::ActResult res, void*)
{
    if (res != lp::ActResult::SessionInUse &&
        res != lp::ActResult::Revoked &&
        res != lp::ActResult::Expired)
        return;

    // Bi KICK tu server (mat phien) -> dong app NGAY, KHONG thong bao.
    // Khoa / het han thi van bao mot cau roi dong.
    if (res != lp::ActResult::SessionInUse)
    {
        const wchar_t* msg =
            (res == lp::ActResult::Revoked)
                ? L"Code:10001" //Tai khoan da bi khoa. Ung dung se dong.
                : L"Code:10002";// License da het han.Ung dung se dong.
        ::MessageBoxW(NULL, msg, L"VLCTCPro",
                      MB_OK | MB_ICONWARNING | MB_SYSTEMMODAL | MB_SETFOREGROUND);
    }

    // TERMINATE ngay lap tuc. KHONG dung WM_CLOSE (se kich hoat hop thoai
    // "dung auto?" cua app va phai bam OK); day la ngat cung, dong luon.
    ::TerminateProcess(::GetCurrentProcess(), 0);
}

} // namespace

// ---------------------------------------------------------------------------
// API cong khai
// ---------------------------------------------------------------------------
// Thu dang nhap + gianh lease voi tai khoan dang co.
// Tra ve: 1 = duoc chay, 0 = thu lai (hien dialog), -1 = loi nghiem trong (thoat).
static int TryActivate(const std::string& user, const std::string& pass)
{
    lp::ActResult la = lp::activation_login(user.c_str(), pass.c_str(), kAppId);
    if (la == lp::ActResult::BadCredentials)
    {
        ::MessageBoxW(NULL, L"Sai ten tai khoan hoac mat khau.", L"VLCTCPro",
                      MB_OK | MB_ICONERROR);
        return 0;
    }
    if (la == lp::ActResult::NetworkError)
    {
        ::MessageBoxW(NULL, L"Khong ket noi duoc may chu. Kiem tra mang.",
                      L"VLCTCPro", MB_OK | MB_ICONERROR);
        return 0;
    }
    if (la == lp::ActResult::NotConfigured)
    {
        ::MessageBoxW(NULL, L"Chua cau hinh may chu kich hoat.", L"VLCTCPro",
                      MB_OK | MB_ICONERROR);
        return -1;
    }
    // la == Ok -> tiep tuc gianh lease. "key" cache slot = ten tai khoan.

    lp::ActResult r = lp::activation_session_begin(user.c_str(), kAppId);
    switch (r)
    {
    case lp::ActResult::Ok:
        if (g_remember) SaveRemembered(g_lastUser, g_lastPass, g_remember, g_autoLogin);
        else            ClearRemembered();
        g_userUtf8 = user;
        g_started = true;
        lp::activation_start_session_guard(g_userUtf8.c_str(), kAppId,
                                           &OnSessionLost, NULL, 60);
        // Mo blob offset bang K trong token (chi co sau activation that).
        UnsealGameAddr();
        // Doc goi license (plan/quota/expiry) tu token da ky - de gate tinh nang.
        LicenseCapture(kAppId);
        return 1;

    case lp::ActResult::SessionInUse:
        ::MessageBoxW(NULL,
            L"Tai khoan dang duoc dung o may khac.\n"
            L"Hay thoat ban client dang chay roi thu lai.",
            L"VLCTCPro", MB_OK | MB_ICONWARNING);
        return 0;

    case lp::ActResult::NotFound:
        ::MessageBoxW(NULL,
            L"Tai khoan chua duoc kich hoat license.\n"
            L"Lien he admin de kich hoat goi.",
            L"VLCTCPro", MB_OK | MB_ICONWARNING);
        return 0;

    case lp::ActResult::AuthRequired:
        ::MessageBoxW(NULL, L"Can dang nhap tai khoan.", L"VLCTCPro",
                      MB_OK | MB_ICONWARNING);
        return 0;

    case lp::ActResult::Revoked:
        ::MessageBoxW(NULL, L"Tai khoan da bi khoa. Lien he admin.", L"VLCTCPro",
                      MB_OK | MB_ICONERROR);
        return -1;

    case lp::ActResult::Expired:
        ::MessageBoxW(NULL, L"License da het han. Lien he admin de gia han.",
                      L"VLCTCPro", MB_OK | MB_ICONERROR);
        return -1;

    case lp::ActResult::DeviceLimit:
        ::MessageBoxW(NULL,
            L"Tai khoan da gan voi mot may khac.\n"
            L"Lien he admin de doi may (reset HWID).",
            L"VLCTCPro", MB_OK | MB_ICONERROR);
        return -1;

    case lp::ActResult::NetworkError:
        ::MessageBoxW(NULL, L"Khong ket noi duoc may chu. Kiem tra mang.",
                      L"VLCTCPro", MB_OK | MB_ICONERROR);
        return 0;

    case lp::ActResult::BadToken:
        ::MessageBoxW(NULL, L"Phan hoi may chu khong hop le (chu ky sai).",
                      L"VLCTCPro", MB_OK | MB_ICONERROR);
        return -1;

    case lp::ActResult::NotConfigured:
        ::MessageBoxW(NULL, L"Chua cau hinh may chu kich hoat.", L"VLCTCPro",
                      MB_OK | MB_ICONERROR);
        return -1;

    default:
        ::MessageBoxW(NULL, L"Kich hoat that bai. Thu lai.", L"VLCTCPro",
                      MB_OK | MB_ICONERROR);
        return 0;
    }
}

bool LicenseGateStartup()
{
    lp::activation_set_endpoint(kEndpoint);
    LoadRemembered();

    // Tu dong dang nhap (bo qua dialog) CHI khi nguoi dung da bat "Tu dong dang nhap"
    // va con du tai khoan + mat khau nho san.
    if (g_autoLogin && !g_lastUser.IsEmpty() && !g_lastPass.IsEmpty())
    {
        int rc = TryActivate(W2U(g_lastUser), W2U(g_lastPass));
        if (rc == 1)  return true;
        if (rc == -1) return false;
        // rc == 0 -> roi xuong hien dialog cho nhap lai
    }

    for (;;)
    {
        LoginResult in;
        if (!ShowLoginDialog(in))
            return false;                 // nguoi dung huy

        g_lastUser  = in.user;
        g_lastPass  = in.pass;            // de ma hoa + luu khi thanh cong
        g_remember  = in.remember;
        g_autoLogin = in.autologin;

        int rc = TryActivate(W2U(in.user), W2U(in.pass));
        if (rc == 1)  return true;
        if (rc == -1) return false;
        // rc == 0 -> lap lai
    }
}

void LicenseGateShutdown()
{
    if (!g_started)
        return;
    g_started = false;
    lp::activation_session_end(g_userUtf8.c_str(), kAppId);
}

// ---------------------------------------------------------------------------
// Phase 2.1 - self-integrity .text (chi khi LP_HARDEN; LP_HARDEN => LICENSE_REQUIRED)
// ---------------------------------------------------------------------------
#if LP_HARDEN
namespace {

// Tim section .text cua module -> [addr, len) trong bo nho da load (sau reloc).
bool FindTextSection(HMODULE mod, const unsigned char** outAddr, size_t* outLen)
{
    const unsigned char* base = reinterpret_cast<const unsigned char*>(mod);
    const IMAGE_DOS_HEADER* dos = reinterpret_cast<const IMAGE_DOS_HEADER*>(base);
    if (dos->e_magic != IMAGE_DOS_SIGNATURE) return false;
    const IMAGE_NT_HEADERS* nt =
        reinterpret_cast<const IMAGE_NT_HEADERS*>(base + dos->e_lfanew);
    if (nt->Signature != IMAGE_NT_SIGNATURE) return false;
    const IMAGE_SECTION_HEADER* sec = IMAGE_FIRST_SECTION(nt);
    for (unsigned i = 0; i < nt->FileHeader.NumberOfSections; ++i) {
        if (memcmp(sec[i].Name, ".text", 6) == 0) {   // 6: ".text\0"
            size_t vs = sec[i].Misc.VirtualSize;
            *outAddr = base + sec[i].VirtualAddress;
            *outLen  = vs ? vs : sec[i].SizeOfRawData;
            return true;
        }
    }
    return false;
}

// Phase 2.3: CRC file tren dia so voi baseline NHUNG trong binary. Mot post-build
// tool (tools/filechk/patch_filechk.js) quet magic duoi day, zero 4 byte CRC roi
// ghi CRC32 cua toan file (voi 4 byte do = 0) vao. Runtime doc lai file cua chinh
// module, zero dung 4 byte do, CRC32, so voi gia tri nhung. Lech => file bi patch
// tren dia. FAIL-OPEN: neu CRC con sentinel (patcher chua chay) hoac khong doc
// duoc file => BO QUA, khong bao gio false-positive. Bypass duoc neu attacker tinh
// lai CRC, nhung van con lop .text runtime (2.1/2.2) -> tang chi phi.
#pragma pack(push, 1)
struct LpFileChk { unsigned char magic[16]; volatile unsigned int crc; };
#pragma pack(pop)
// 16 byte magic (co dinh, hiem dung): "LPFCHK" + bytes ngau nhien.
volatile LpFileChk g_fileChk = {
    { 0x4C,0x50,0x46,0x43,0x48,0x4B,0x9E,0x37,0xC1,0x5A,0x80,0x2D,0x6B,0xF4,0x11,0xA9 },
    0xFFFFFFFFu   // sentinel = chua patch -> fail-open
};

static void FileIntegrityCheck()
{
    unsigned int stored = g_fileChk.crc;
    if (stored == 0xFFFFFFFFu) return;                 // fail-open: chua patch

    wchar_t path[MAX_PATH];
    if (!GetModuleFileNameW(GetModuleHandleW(nullptr), path, MAX_PATH)) return;
    HANDLE hf = CreateFileW(path, GENERIC_READ,
        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
        nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (hf == INVALID_HANDLE_VALUE) return;            // fail-open

    DWORD sz = GetFileSize(hf, nullptr);
    bool tampered = false;
    if (sz != INVALID_FILE_SIZE && sz > sizeof(LpFileChk)) {
        BYTE* buf = static_cast<BYTE*>(malloc(sz));
        DWORD rd = 0;
        if (buf && ReadFile(hf, buf, sz, &rd, nullptr) && rd == sz) {
            // Tim magic de biet offset cua truong CRC.
            const unsigned char* mg = (const unsigned char*)g_fileChk.magic;
            DWORD pos = 0; bool found = false;
            for (DWORD i = 0; i + 16 <= sz; ++i)
                if (memcmp(buf + i, mg, 16) == 0) { pos = i; found = true; break; }
            if (found && pos + 16 + 4 <= sz) {
                memset(buf + pos + 16, 0, 4);          // zero CRC field (nhu patcher)
                unsigned int calc = lp::crc32(buf, sz);
                if (calc != stored) tampered = true;
            }
            // khong tim thay magic -> fail-open (khong flag)
        }
        if (buf) free(buf);
    }
    CloseHandle(hf);
    if (tampered) LP_TAMPER("file");
}

// Phase 2.2: hai checker kiem tra LAN NHAU. Moi thread (a) verify .text (vung
// nay chua ca code cua CA HAI thread), (b) tang nhip cua minh, (c) kiem nhip cua
// thread kia con chay khong. NOP/kill mot thread -> thread con lai thay nhip kia
// dong bang -> LP_TAMPER. Patch byte checker nao -> integrity_verify bat duoc.
volatile LONG g_beatA = 0;
volatile LONG g_beatB = 0;

// Vong lap chung cho ca 2 checker. self = nhip cua minh, other = nhip can theo doi.
static void IntegrityLoop(volatile LONG* self, volatile LONG* other, DWORD baseMs)
{
    LONG lastOther = InterlockedCompareExchange(other, 0, 0);
    int  stale = 0;
    for (;;) {
        Sleep(baseMs + (GetTickCount() & 0x7FF));   // jitter ~0.0-2.0s
        if (!lp::integrity_verify())
            LP_TAMPER("text");
        InterlockedIncrement(self);
        LONG now = InterlockedCompareExchange(other, 0, 0);
        if (now == lastOther) {                      // nhip kia khong tang
            if (++stale >= 2) LP_TAMPER("beat");     // ~2 chu ky moi flag (chong jitter)
        } else {
            stale = 0; lastOther = now;
        }
    }
}

DWORD WINAPI ThreadA(LPVOID) { FileIntegrityCheck(); IntegrityLoop(&g_beatA, &g_beatB, 2000); return 0; }
DWORD WINAPI ThreadB(LPVOID) { IntegrityLoop(&g_beatB, &g_beatA, 2600); return 0; }

// ---- Phase 3.1-3.3/3.5: anti-debug watchdog (detection o lp_antidebug) ----
// Callback chi duoc goi khi debugger_present() fire. Debounce 2 chu ky de tranh
// false-positive hiem (da bo AD_TIMING). Phan ung = LP_TAMPER (degrade tre).
void OnDbg(uint32_t fired)
{
    (void)fired;
    static volatile LONG hits = 0;
    if (InterlockedIncrement(&hits) >= 2)
        LP_TAMPER("dbg");
}

#if !LP_HARDEN_SAFE
// ---- Phase 3.4: anti-attach. CHI ban that (KHONG o Tester/SAFE, de tester con
// attach debugger duoc). Day la PHONG NGUA (khong phai degrade). ----
typedef LONG (NTAPI* PFN_NtSetInfoThread)(HANDLE, ULONG, PVOID, ULONG);
void HideCurrentThread()
{
    static PFN_NtSetInfoThread p = reinterpret_cast<PFN_NtSetInfoThread>(
        GetProcAddress(GetModuleHandleW(L"ntdll.dll"), "NtSetInformationThread"));
    if (p) p(GetCurrentThread(), 0x11 /*ThreadHideFromDebugger*/, nullptr, 0);
}
void BlockRemoteAttach()
{
    // Vo hieu DbgUiRemoteBreakin trong ntdll cua TIEN TRINH NAY: debugger attach
    // (CreateRemoteThread) khong vao breakpoint duoc. Chi anh huong viec attach.
    FARPROC f = GetProcAddress(GetModuleHandleW(L"ntdll.dll"), "DbgUiRemoteBreakin");
    if (!f) return;
    DWORD old = 0;
    if (VirtualProtect((void*)f, 1, PAGE_EXECUTE_READWRITE, &old)) {
        *reinterpret_cast<volatile BYTE*>(f) = 0xC3;   // ret
        DWORD tmp = 0; VirtualProtect((void*)f, 1, old, &tmp);
    }
}
#endif // !LP_HARDEN_SAFE

} // namespace

void IntegrityGuardStart()
{
    const unsigned char* addr = nullptr; size_t len = 0;
    if (!FindTextSection(GetModuleHandle(NULL), &addr, &len)) return;
    if (lp::integrity_add(addr, len) < 0) return;
    lp::integrity_snapshot();                       // baseline NGAY sau khi load
    HANDLE h;
    h = CreateThread(nullptr, 0, ThreadA, nullptr, 0, nullptr); if (h) CloseHandle(h);
    h = CreateThread(nullptr, 0, ThreadB, nullptr, 0, nullptr); if (h) CloseHandle(h);

    // Watchdog anti-debug: API + PEB + NtQuery + HWBP (bo TIMING -> it false-positive).
    lp::ad_install_guard(&OnDbg, 1500,
                         lp::AD_API | lp::AD_PEB | lp::AD_NTQUERY | lp::AD_HWBP);
#if !LP_HARDEN_SAFE
    HideCurrentThread();
    BlockRemoteAttach();
#endif
}

// ---- Phase 4.1: anti-dump scrub PE header (goi MUON, truoc DoModal) ----
// Mac dinh ADUMP_DEFAULT = Rich + section table: an toan tren x86 (resource/export
// di qua data-directory RVA, SEH la FS:[0], khong can section headers). Bo aggressive
// (xoa MZ/PE/SizeOfImage) ra sau macro opt-in vi co the hong parse header.
#ifdef LP_ANTIDUMP_IAT
// ---- Phase 4.2 (OPT-IN, mac dinh TAT): nhoe IAT cho mot whitelist API nhay cam.
// Ghi de thunk IAT cua chung bang decoy -> dumper dung lai IAT se tro sai. CHI an
// toan voi API ma app KHONG goi qua IAT tinh (ta goi dynamic). Co the hong neu
// MFC/CRT lo dung chung -> vi vay mac dinh TAT, phai test ky. Bypass: attacker
// dung lai IAT thu cong (do la muc dich lam kho, Scylla phai lam tay). ----
void ScrambleSensitiveIAT()
{
    static const char* kWhitelist[] = { "NtQueryInformationProcess",
                                        "NtSetInformationThread", "DbgUiRemoteBreakin" };
    BYTE* base = reinterpret_cast<BYTE*>(GetModuleHandleW(nullptr));
    auto dos = reinterpret_cast<IMAGE_DOS_HEADER*>(base);
    auto nt  = reinterpret_cast<IMAGE_NT_HEADERS*>(base + dos->e_lfanew);
    DWORD impRva = nt->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress;
    if (!impRva) return;
    auto imp = reinterpret_cast<IMAGE_IMPORT_DESCRIPTOR*>(base + impRva);
    for (; imp->Name; ++imp) {
        if (!imp->OriginalFirstThunk) continue;
        auto oft = reinterpret_cast<IMAGE_THUNK_DATA*>(base + imp->OriginalFirstThunk);
        auto ft  = reinterpret_cast<IMAGE_THUNK_DATA*>(base + imp->FirstThunk);
        for (; oft->u1.AddressOfData; ++oft, ++ft) {
            if (oft->u1.Ordinal & IMAGE_ORDINAL_FLAG) continue;
            auto ibn = reinterpret_cast<IMAGE_IMPORT_BY_NAME*>(base + oft->u1.AddressOfData);
            for (const char* w : kWhitelist) {
                if (lstrcmpA((const char*)ibn->Name, w) == 0) {
                    DWORD old = 0;
                    if (VirtualProtect(&ft->u1.Function, sizeof(void*), PAGE_READWRITE, &old)) {
                        ft->u1.Function ^= 0xA5A5A5A5u;   // decoy
                        DWORD tmp = 0; VirtualProtect(&ft->u1.Function, sizeof(void*), old, &tmp);
                    }
                }
            }
        }
    }
}
#endif // LP_ANTIDUMP_IAT

#ifdef LP_ANTIDUMP_PAGEGUARD
// ---- Phase 4.3 (OPT-IN, mac dinh TAT, THUC NGHIEM): PAGE_GUARD tripwire tren 1
// trang code nhay cam. Truy cap dau tien -> guard fault -> VEH. LUU Y: PAGE_GUARD
// tu dong bi xoa sau lan fault dau, nen day chi la tripwire 1 lan (CHUA re-arm,
// chua decrypt-on-demand). Lam day du thuoc Phase 5 (protector thuong mai). ----
LONG CALLBACK GuardVeh(EXCEPTION_POINTERS* ep)
{
    if (ep->ExceptionRecord->ExceptionCode == STATUS_GUARD_PAGE_VIOLATION) {
        LP_TAMPER("guard");
        return EXCEPTION_CONTINUE_EXECUTION;   // guard da bi OS xoa -> chay tiep
    }
    return EXCEPTION_CONTINUE_SEARCH;
}
void ArmPageGuard()
{
    AddVectoredExceptionHandler(1, GuardVeh);
    void* page = reinterpret_cast<void*>(&HardenScrub);
    DWORD old = 0;
    VirtualProtect(page, 1, PAGE_EXECUTE_READ | PAGE_GUARD, &old);
}
#endif // LP_ANTIDUMP_PAGEGUARD

void HardenScrub()
{
    uint32_t flags = lp::ADUMP_DEFAULT;
#ifdef LP_ANTIDUMP_AGGRESSIVE
    flags = lp::ADUMP_AGGRESSIVE;          // (!) opt-in, test cuc ky ky truoc khi dung
#endif
    lp::antidump_scrub(flags);
#ifdef LP_ANTIDUMP_IAT
    ScrambleSensitiveIAT();                // (!) opt-in Phase 4.2
#endif
#ifdef LP_ANTIDUMP_PAGEGUARD
    ArmPageGuard();                        // (!) opt-in Phase 4.3 (thuc nghiem)
#endif
}

#if !LP_HARDEN_SAFE
// ---- Phase 3.4: TLS callback - chay RAT SOM (truoc main). An luong chinh + neu
// dang bi debug thi dat co ngay. Chi ban that. ----
extern "C" void NTAPI LpTlsCallback(PVOID, DWORD reason, PVOID)
{
    if (reason != DLL_PROCESS_ATTACH) return;
    HideCurrentThread();
    if (lp::debugger_present(lp::AD_PEB | lp::AD_NTQUERY))
        LP_TAMPER("tls");
}
#pragma comment(linker, "/INCLUDE:__tls_used")
#pragma section(".CRT$XLB", long, read)
extern "C" __declspec(allocate(".CRT$XLB")) PIMAGE_TLS_CALLBACK p_lp_tls = LpTlsCallback;
#endif // !LP_HARDEN_SAFE
#endif // LP_HARDEN

#endif // LICENSE_REQUIRED
