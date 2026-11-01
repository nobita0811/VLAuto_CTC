// LicenseGate.cpp : online single-session license gate for VLAuto (LPROTECT).
// Account model: login = username + password, the machine is identified by a
// machine id (HWID) computed by the SDK and sent automatically. The license
// (plan/quota/expiry) lives on the account; there is no license key to type.
#include "stdafx.h"
#include "LicenseGate.h"
#include "lprotect/lprotect.h"

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

// Control id cho dialog dung trong bo nho.
enum { IDC_USER = 1001, IDC_PASS = 1002, IDC_HWID = 1003, IDC_STATUS = 1004 };

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
}

// Luu tai khoan + mat khau (DPAPI). Chi luu khi dang nhap thanh cong.
static void SaveRemembered(const CString& user, const CStringW& pass)
{
    ::WritePrivateProfileStringW(L"License", L"User", user, g_szIniFile);
    CString enc = ProtectPass(pass);
    ::WritePrivateProfileStringW(L"License", L"PassEnc",
                                 enc.IsEmpty() ? NULL : (LPCWSTR)enc, g_szIniFile);
}

// ---------------------------------------------------------------------------
// Dung DLGTEMPLATE trong bo nho (khong can sua VLAuto.rc / Resource.h)
// ---------------------------------------------------------------------------
namespace {

struct TBuf {
    std::vector<BYTE> b;
    void align4()          { while (b.size() & 3) b.push_back(0); }
    void w16(WORD v)       { b.push_back((BYTE)(v & 0xFF)); b.push_back((BYTE)((v >> 8) & 0xFF)); }
    void w32(DWORD v)      { w16((WORD)(v & 0xFFFF)); w16((WORD)((v >> 16) & 0xFFFF)); }
    void wstr(const wchar_t* s) { if (s) { for (; *s; ++s) w16((WORD)*s); } w16(0); }
};

// atom lop control chuan
const WORD ATOM_BUTTON = 0x0080, ATOM_EDIT = 0x0081, ATOM_STATIC = 0x0082;

void AddItem(TBuf& t, DWORD style, short x, short y, short cx, short cy,
             WORD id, WORD atom, const wchar_t* text)
{
    t.align4();
    t.w32(style);
    t.w32(0);                 // exStyle
    t.w16((WORD)x); t.w16((WORD)y); t.w16((WORD)cx); t.w16((WORD)cy);
    t.w16(id);
    t.w16(0xFFFF); t.w16(atom);   // class = ordinal
    t.wstr(text);                 // title
    t.w16(0);                     // no creation data
}

struct LoginResult {
    CStringW user, pass, mid;
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
        ::SetDlgItemTextW(h, IDC_USER, r->user);
        ::SetDlgItemTextW(h, IDC_PASS, r->pass);
        ::SetDlgItemTextW(h, IDC_HWID, r->mid);
        ::SetDlgItemTextW(h, IDC_STATUS,
            L"Dang nhap de chay. Chi mot ban client duoc chay cung luc.");
        ::SendDlgItemMessageW(h, IDC_USER, EM_SETLIMITTEXT, 63, 0);
        ::SendDlgItemMessageW(h, IDC_PASS, EM_SETLIMITTEXT, 254, 0);
        ::SetForegroundWindow(h);
        // Focus o truong trong dau tien; neu da dien du thi focus nut Dang nhap.
        int focusId = r->user.IsEmpty() ? IDC_USER
                    : r->pass.IsEmpty() ? IDC_PASS : IDOK;
        ::SetFocus(::GetDlgItem(h, focusId));
        return FALSE; // da tu dat focus
    }
    case WM_COMMAND:
        switch (LOWORD(wp))
        {
        case IDOK:
        {
            LoginResult* r = (LoginResult*)::GetWindowLongPtr(h, DWLP_USER);
            wchar_t buf[256];
            ::GetDlgItemTextW(h, IDC_USER, buf, 256);
            CStringW user(buf); user.Trim();
            if (user.IsEmpty())
            {
                ::SetDlgItemTextW(h, IDC_STATUS, L"Vui long nhap ten tai khoan.");
                ::SetFocus(::GetDlgItem(h, IDC_USER));
                return TRUE;
            }
            ::GetDlgItemTextW(h, IDC_PASS, buf, 256); CStringW pass(buf);
            if (pass.IsEmpty())
            {
                ::SetDlgItemTextW(h, IDC_STATUS, L"Vui long nhap mat khau.");
                ::SetFocus(::GetDlgItem(h, IDC_PASS));
                return TRUE;
            }
            r->user = user;
            r->pass = pass;         // KHONG trim mat khau
            r->ok = true;
            ::EndDialog(h, 1);
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

// Hien dialog dang nhap. Tra ve true neu bam OK, false neu Cancel/loi.
bool ShowLoginDialog(LoginResult& out)
{
    TBuf t;
    const DWORD dlgStyle = DS_MODALFRAME | DS_SETFONT | DS_CENTER |
                           WS_POPUP | WS_CAPTION | WS_SYSMENU;
    t.w32(dlgStyle);
    t.w32(0);                 // exStyle
    t.w16(9);                 // cItems (3 label + 3 edit + 1 status + 2 button)
    t.w16(0); t.w16(0);       // x,y (DS_CENTER)
    t.w16(224); t.w16(104);   // cx,cy (dialog units)
    t.wstr(L"");              // menu
    t.wstr(L"");              // class
    t.wstr(L"VLAuto - Dang nhap");               // title
    t.w16(8);                 // font point size (DS_SETFONT)
    t.wstr(L"MS Shell Dlg");  // font face

    const DWORD stLabel = WS_CHILD | WS_VISIBLE | SS_LEFT;
    const DWORD stEdit  = WS_CHILD | WS_VISIBLE | WS_BORDER | WS_TABSTOP | ES_AUTOHSCROLL;
    const DWORD stBtn   = WS_CHILD | WS_VISIBLE | WS_TABSTOP;

    AddItem(t, stLabel,              8,  10, 60, 10, (WORD)-1, ATOM_STATIC, L"Ten tai khoan:");
    AddItem(t, stEdit,              74,   8, 142, 12, IDC_USER, ATOM_EDIT, L"");
    AddItem(t, stLabel,              8,  30, 60, 10, (WORD)-1, ATOM_STATIC, L"Mat khau:");
    AddItem(t, stEdit | ES_PASSWORD,74,  28, 142, 12, IDC_PASS, ATOM_EDIT, L"");
    AddItem(t, stLabel,              8,  50, 60, 10, (WORD)-1, ATOM_STATIC, L"HWID:");
    AddItem(t, stEdit | ES_READONLY,74,  48, 142, 12, IDC_HWID, ATOM_EDIT, L"");
    AddItem(t, stLabel,              8,  66, 208, 18, IDC_STATUS, ATOM_STATIC, L"");
    AddItem(t, stBtn | BS_DEFPUSHBUTTON, 108, 86, 50, 14, IDOK,     ATOM_BUTTON, L"Dang nhap");
    AddItem(t, stBtn | BS_PUSHBUTTON,    166, 86, 50, 14, IDCANCEL, ATOM_BUTTON, L"Thoat");

    out.user = g_lastUser;
    out.pass = g_lastPass;
    out.mid  = MachineIdW();
    out.ok   = false;

    INT_PTR r = ::DialogBoxIndirectParamW(
        AfxGetInstanceHandle(), (LPCDLGTEMPLATEW)t.b.data(),
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
                ? L"Tai khoan da bi khoa. Ung dung se dong."
                : L"License da het han. Ung dung se dong.";
        ::MessageBoxW(NULL, msg, L"VLAuto",
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
        ::MessageBoxW(NULL, L"Sai ten tai khoan hoac mat khau.", L"VLAuto",
                      MB_OK | MB_ICONERROR);
        return 0;
    }
    if (la == lp::ActResult::NetworkError)
    {
        ::MessageBoxW(NULL, L"Khong ket noi duoc may chu. Kiem tra mang.",
                      L"VLAuto", MB_OK | MB_ICONERROR);
        return 0;
    }
    if (la == lp::ActResult::NotConfigured)
    {
        ::MessageBoxW(NULL, L"Chua cau hinh may chu kich hoat.", L"VLAuto",
                      MB_OK | MB_ICONERROR);
        return -1;
    }
    // la == Ok -> tiep tuc gianh lease. "key" cache slot = ten tai khoan.

    lp::ActResult r = lp::activation_session_begin(user.c_str(), kAppId);
    switch (r)
    {
    case lp::ActResult::Ok:
        SaveRemembered(g_lastUser, g_lastPass);
        g_userUtf8 = user;
        g_started = true;
        lp::activation_start_session_guard(g_userUtf8.c_str(), kAppId,
                                           &OnSessionLost, NULL, 60);
        return 1;

    case lp::ActResult::SessionInUse:
        ::MessageBoxW(NULL,
            L"Tai khoan dang duoc dung o may khac.\n"
            L"Hay thoat ban client dang chay roi thu lai.",
            L"VLAuto", MB_OK | MB_ICONWARNING);
        return 0;

    case lp::ActResult::NotFound:
        ::MessageBoxW(NULL,
            L"Tai khoan chua duoc kich hoat license.\n"
            L"Lien he admin de kich hoat goi.",
            L"VLAuto", MB_OK | MB_ICONWARNING);
        return 0;

    case lp::ActResult::AuthRequired:
        ::MessageBoxW(NULL, L"Can dang nhap tai khoan.", L"VLAuto",
                      MB_OK | MB_ICONWARNING);
        return 0;

    case lp::ActResult::Revoked:
        ::MessageBoxW(NULL, L"Tai khoan da bi khoa. Lien he admin.", L"VLAuto",
                      MB_OK | MB_ICONERROR);
        return -1;

    case lp::ActResult::Expired:
        ::MessageBoxW(NULL, L"License da het han. Lien he admin de gia han.",
                      L"VLAuto", MB_OK | MB_ICONERROR);
        return -1;

    case lp::ActResult::DeviceLimit:
        ::MessageBoxW(NULL,
            L"Tai khoan da gan voi mot may khac.\n"
            L"Lien he admin de doi may (reset HWID).",
            L"VLAuto", MB_OK | MB_ICONERROR);
        return -1;

    case lp::ActResult::NetworkError:
        ::MessageBoxW(NULL, L"Khong ket noi duoc may chu. Kiem tra mang.",
                      L"VLAuto", MB_OK | MB_ICONERROR);
        return 0;

    case lp::ActResult::BadToken:
        ::MessageBoxW(NULL, L"Phan hoi may chu khong hop le (chu ky sai).",
                      L"VLAuto", MB_OK | MB_ICONERROR);
        return -1;

    case lp::ActResult::NotConfigured:
        ::MessageBoxW(NULL, L"Chua cau hinh may chu kich hoat.", L"VLAuto",
                      MB_OK | MB_ICONERROR);
        return -1;

    default:
        ::MessageBoxW(NULL, L"Kich hoat that bai. Thu lai.", L"VLAuto",
                      MB_OK | MB_ICONERROR);
        return 0;
    }
}

bool LicenseGateStartup()
{
    lp::activation_set_endpoint(kEndpoint);
    LoadRemembered();

    // Tu dong dang nhap neu da nho du tai khoan + mat khau (khong hien dialog).
    if (!g_lastUser.IsEmpty() && !g_lastPass.IsEmpty())
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

        g_lastUser = in.user;
        g_lastPass = in.pass;             // de ma hoa + luu khi thanh cong

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
