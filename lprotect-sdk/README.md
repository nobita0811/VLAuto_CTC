# LPROTECT SDK (prebuilt) — hướng dẫn tích hợp

Bộ này đã **build sẵn** để copy thẳng vào dự án client — không cần source của
LPROTECT. Nếu bạn là một session Claude ở dự án client: đọc file này rồi làm theo.

```
lprotect-sdk/
├─ include/lprotect/*.h             ← header để #include
├─ lib/Win32/Release/lprotect.lib   ← link cho app Win32 (x86) Release
├─ lib/Win32/Debug/lprotect.lib     ← link cho app Win32 (x86) Debug
├─ lib/x64/Release/lprotect.lib     ← link cho app x64 Release
├─ lib/x64/Debug/lprotect.lib       ← link cho app x64 Debug
├─ bin/lprotect_stamp.exe           ← công cụ đóng dấu hash, chạy SAU khi build
└─ lprotect.props                   ← property sheet VS: cấu hình tự động
```

> **Nền tảng:** Windows, C++17, toolset MSVC v143. Có lib cho **cả Win32 (x86)
> lẫn x64** — property sheet tự chọn theo Platform. Lib build với
> `LP_BUILD_SEED=0xA5C3F17B` (đã ghi cứng trong `lprotect.props`).

---

## A. Visual Studio (nhanh nhất — dùng property sheet)

1. Copy cả thư mục `lprotect-sdk/` vào dự án client (ví dụ `third_party/lprotect-sdk/`).
2. Mở **View → Property Manager**.
3. Chuột phải project client (chọn **All Configurations / All Platforms** nếu có) →
   **Add Existing Property Sheet…** → chọn `lprotect-sdk/lprotect.props`.
4. Chọn **Platform = Win32** (app của bạn build Win32) — hoặc x64 nếu app x64.
   Xong: include path, `lprotect.lib` + `winhttp.lib` + `bcrypt.lib` +
   `iphlpapi.lib` + `advapi32.lib`, define `LP_BUILD_SEED`, và chuẩn C++17 được
   thiết lập tự động.

`.props` tự chọn `lib\$(Platform)\$(Configuration)` — tức `lib\Win32\Release`,
`lib\Win32\Debug`, `lib\x64\Release`... đúng theo Platform + Configuration đang build.

### Nếu không dùng property sheet (cấu hình tay, app Win32)
- C/C++ → General → Additional Include Directories: `…\lprotect-sdk\include`
- Linker → General → Additional Library Directories: `…\lprotect-sdk\lib\Win32\$(Configuration)`
- Linker → Input → Additional Dependencies: `lprotect.lib;winhttp.lib;bcrypt.lib;iphlpapi.lib;advapi32.lib`
- C/C++ → Language → C++ Language Standard: **ISO C++17** (hoặc mới hơn)
- C/C++ → Preprocessor → Definitions: `LP_BUILD_SEED=0xA5C3F17B`
- **Runtime Library** phải khớp lib: Release = **/MD**, Debug = **/MDd** (mặc định VS).

> `advapi32.lib` (hàm registry) thường đã có sẵn trong Additional Dependencies mặc
> định của project VS; liệt kê ở đây cho chắc nếu project bạn đã xóa danh sách mặc định.

---

## B. CMake

```cmake
# Trỏ tới thư mục SDK
set(LPROTECT_SDK "${CMAKE_SOURCE_DIR}/third_party/lprotect-sdk")

# App Win32 => dùng Win32; app x64 => đổi thành x64. (CMake: cấu hình bằng -A Win32)
set(LPROTECT_ARCH "Win32")

add_library(lprotect STATIC IMPORTED)
set_target_properties(lprotect PROPERTIES
    IMPORTED_LOCATION       "${LPROTECT_SDK}/lib/${LPROTECT_ARCH}/Release/lprotect.lib"
    IMPORTED_LOCATION_DEBUG "${LPROTECT_SDK}/lib/${LPROTECT_ARCH}/Debug/lprotect.lib"
    INTERFACE_INCLUDE_DIRECTORIES "${LPROTECT_SDK}/include")
target_compile_definitions(lprotect INTERFACE LP_BUILD_SEED=0xA5C3F17B)
target_link_libraries(lprotect INTERFACE winhttp bcrypt iphlpapi advapi32)

# App của bạn:
target_link_libraries(my_app PRIVATE lprotect)
target_compile_features(my_app PRIVATE cxx_std_17)
```

---

## C. Dùng trong code

```cpp
#include "lprotect/lprotect.h"

// trỏ tới server (hoặc build với -DLP_ACTIVATION_URL="https://.../activate")
lp::activation_set_endpoint(L"https://lprotect-activation.<ban>.workers.dev/activate");

lp::ActResult r = lp::activation_check("KEY-CUA-KHACH", "MyApp");
if (r != lp::ActResult::Ok && r != lp::ActResult::OkOffline) { /* chặn */ }

lp::LicenseInfo info;
if (lp::activation_get_info(info, "MyApp")) {
    char line[64];
    lp::license_info_format(info, line, sizeof line);   // vd "Basic-8-20273101"
    // phân quyền theo info.plan / info.quota / info.expiry
}
```

---

## D. Sau khi build app — BẮT BUỘC chạy stamp

Đóng dấu hash SHA-256 cả file (chống sửa file trên đĩa). Thêm Post-Build Event:

```
"$(ProjectDir)third_party\lprotect-sdk\bin\lprotect_stamp.exe" "$(TargetPath)"
```

Chạy **sau cùng** (sau mọi hậu xử lý; nếu có ký số Authenticode thì sau khi ký).

---

## E. Những điều PHẢI nhớ

1. **Platform khớp (Win32 hay x64) + C++17 + CRT khớp** (/MD Release, /MDd Debug).
   Link lib Win32 vào app x64 (hoặc ngược lại) → LNK1112; lệch /MD↔/MT → LNK2038.
2. **`LP_BUILD_SEED` không được đổi** với SDK dựng sẵn — property sheet/định nghĩa
   CMake đã ghim đúng giá trị lib. Muốn seed khác (đa hình mỗi release) → phải lấy
   **source** LPROTECT build lại lib.
3. **Public key production**: lib trong SDK này nhúng public key gì thì verify key
   của bạn phải do seed tương ứng ký. Nếu bạn build SDK từ source của mình, nhớ đã
   dán public key production vào `src/lp_activation.cpp` TRƯỚC khi build lib.
4. **Server** (Cloudflare Worker) deploy riêng, không nằm trong SDK này.

Tài liệu API/luồng đầy đủ nằm ở repo LPROTECT gốc: `docs/USAGE.md`, `docs/FLOW.md`,
`INTEGRATION.md`.

---

## Cách build lại SDK này (phía bạn, khi có source)

```bat
:: --- x64 (dùng MSBuild trên .sln) ---
msbuild LPROTECT.sln /t:lprotect /p:Configuration=Release /p:Platform=x64
msbuild LPROTECT.sln /t:lprotect /p:Configuration=Debug   /p:Platform=x64

:: --- Win32 (.sln chỉ có cấu hình x64, nên build x86 trực tiếp bằng cl) ---
call "<VS>\VC\Auxiliary\Build\vcvarsall.bat" x86 10.0.19041.0
set DEFS=/D LP_BUILD_SEED=0xA5C3F17B /D _CRT_SECURE_NO_WARNINGS /D UNICODE /D _UNICODE
cl /nologo /c /EHsc /std:c++17 /O2 /MD  %DEFS% /I include src\*.cpp /Fox86\Release\
lib /nologo /OUT:x86\Release\lprotect.lib x86\Release\*.obj
cl /nologo /c /EHsc /std:c++17 /Od /MDd %DEFS% /I include src\*.cpp /Fox86\Debug\
lib /nologo /OUT:x86\Debug\lprotect.lib x86\Debug\*.obj

:: rồi copy include\ + các lprotect.lib (x64\, x86\) + lprotect_stamp.exe vào
:: cây lprotect-sdk\ (lib\Win32\... cho x86, lib\x64\... cho x64).
```
