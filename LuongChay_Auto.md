# Luồng chạy tổng quát của Auto (VLAuto_CTC)

> Tài liệu mô tả cách bộ auto vận hành: từ lúc GUI gắn vào game, đồng bộ tham số,
> cho tới vòng lặp xử lý chính và từng module. Dùng để tra cứu nhanh khi sửa/mở rộng.

---

## 1. Hai thành phần

| Thành phần | File build | Vai trò |
|---|---|---|
| **VLAuto** (MFC GUI) | `VLCTCPro.exe` | Giao diện người dùng: danh sách nhân vật, các tab cấu hình, lưu/đọc config, đẩy tham số vào game. **Không** tự đánh — chỉ điều khiển. |
| **VLHook** (DLL tiêm) | `VLCTCdll.dll` | Được tiêm vào tiến trình game. Chứa toàn bộ logic auto (đánh, di chuyển, hồi phục…), chạy theo nhịp timer bên trong game. |

Mọi logic "auto" thật sự nằm ở **VLHook**. GUI chỉ là bảng điều khiển.

---

## 2. Tiêm DLL & gắn vào game

File: `VLHook/VLHook.cpp` (`HookProc`, `DllMain`), `VLAuto/GameWindow.cpp`.

1. GUI quét các cửa sổ game theo class `VLTK_WINCLASS` và đúng tên exe
   (`RefreshWindows`), tạo một `GameWindow` cho mỗi client.
2. GUI đặt một **Windows hook** (WH_CALLWNDPROC) nạp `VLCTCdll.dll` vào luồng
   của game → DLL được map vào tiến trình game.
3. Trong `HookProc` (chạy bên trong game), DLL:
   - `LoadLibrary(chính nó)` để ghim DLL ở lại kể cả khi gỡ hook.
   - **Subclass** cửa sổ game: `SetWindowLong(GWL_WNDPROC, NewWndProc)` — để nhận
     lệnh/tham số từ GUI.
   - `theMain.InitParams(...)` — lấy handle game + server.
   - `CrashInstall("Game")` — ghi log crash vào `<Auto>\Crash`.
   - **`SetTimer(SYS_TIMER = 40 → ~50ms)` → `TimerProc`** = tim đập của auto.
4. Khi tháo: kill timer, khôi phục `OldWndProc`, dọn debug/trạng thái, `FreeLibrary`.

---

## 3. Đồng bộ tham số GUI → DLL

File: `VLAuto/GameWindow.cpp` (`SetGameParams`), `VLHook/VLHook.cpp` (`NewWndProc`).

- Mỗi tab cấu hình có một struct tham số `PARAM_xxx` (Basic, Recover, Attack,
  Collect, **Move**, Party, Map, YeSou).
- Khi người dùng đổi một tùy chọn, dialog gọi `SetGameParams(tab_x)`:

  ```
  PostMessage(gameHWND, WM_HOOK_xxx, (WPARAM)&m_prXxx, sizeof(PARAM_xxx));
  ```

- Phía DLL, `NewWndProc` nhận `WM_HOOK_xxx` và **copy thẳng** struct từ bộ nhớ
  tiến trình GUI vào class mirror tương ứng:

  ```
  ReadProcessMemory(hProcess_Auto, wParam, &theMove, lParam, NULL);  // ví dụ WM_HOOK_MOVE
  ```

- Vì copy nguyên khối theo byte, **layout của `PARAM_MOVE` (GUI) và vùng public
  của `CVLMove` (DLL) phải khớp từng byte**. (`AutoParams` chỉ có `m_cbSize`, không
  vtable; mọi `PARAM_xxx` kế thừa nó.)
- `SetGameParams(-1)` = đẩy lại **tất cả** các tab (dùng khi mới gắn / login).

**Chiều ngược (DLL → GUI)** dùng `WM_COPYDATA`: ví dụ đọc tọa độ trong game
(`READPOS_REPLY`) hoặc kết quả DEBUG trả về bảng điều khiển.

---

## 4. Vòng đời config theo nhân vật

File: `VLAuto/GameWindow.cpp`, `VLAuto/VLAutoDlg.cpp`, `VLAuto/AutoParams.cpp`.

- Mỗi nhân vật có 1 file `UserConfig\<tên nhân vật>.cfg`.
- Cấu trúc file: `szVersion[12]` + `szPlayerName/AccountID[32]` + 8 khối tham số
  theo thứ tự Basic, Recover, Attack, Collect, **Move**, Party, Map, YeSou.
  Mỗi khối = `[cbSize:uint32][body:(cbSize-4)]`.
- `ReadFromFile` đọc `min(cbSize_trên_đĩa, cbSize_hiện_tại)` — **trường mới thêm
  cuối struct mà file cũ chưa có sẽ giữ giá trị mặc định `InitValue()`**. Đây là
  cách "nâng cấp" config cũ an toàn khi struct lớn lên.
- Các mốc lưu/đọc:
  - **Login** nhân vật (Disconnected→Connected): `LoadProfile()` đọc từ đĩa.
  - **Logout** (Connected→Disconnected): nếu đã load xong thì `SaveProfile()`.
  - **Tick/bỏ tick** nhân vật trong danh sách: gắn → `SetGameParams()` + `SaveProfile()`; gỡ → `SaveProfile()`.
  - **Thoát app** (`OnDestroy`): `SaveProfile()` cho tất cả.

---

## 5. Vòng lặp chính — `TimerProc` (~50ms/nhịp)

File: `VLHook/VLHook.cpp`. Mỗi nhịp tăng `g_RunningTimer`; các module được gọi
theo chu kỳ bằng phép chia dư (`% n`), để giãn tải thay vì chạy tất cả mỗi nhịp.

**Đầu mỗi nhịp:**
- Xử lý tạm ngừng (`bPausing`), lấy `KSubWorld`, `EnterMapUpdate()`.
- Tính **độ trễ mạng** `g_NetworkOffline` từ `m_SyncSignal` (dùng để bỏ qua xử lý
  khi đang lag — nhiều hàm `Breathe` return sớm nếu `g_NetworkOffline`).

**Lịch gọi module:**

| Module | Chu kỳ | Nhiệm vụ |
|---|---|---|
| `theRecover.Breathe()` | ~50ms | Hồi HP/MP, dùng thuốc, buff, **Auto thoát/phòng vệ** |
| `theCollect.Breathe()` | ~50ms | Nhặt đồ (tạm dừng khi đang gom tiền / chạy về thành) |
| `theUtil.Breath()` | 100ms | Tiện ích phụ |
| Debug/MapPath/GiảmCấuHình/TongMoney/WalkCityBox | 200ms | Các tác vụ nền (kể cả khi tạm dừng) |
| `theAttack.Breathe()` | 100ms | **Tìm mục tiêu + đánh** |
| `PathTravelBreathe()` | 200ms | Tự đi theo MapPath (liên bản đồ) |
| `theMove.Breathe()` | 200ms | **Di chuyển**: tới điểm train / quanh điểm / hành trình / theo dõi |
| `theMap.Breathe()` | 600ms | Chuyển map, mua đồ, về thành lưu rương |
| `theBasic.Breathe()` | 1s | Cấu hình cơ bản, giảm cấu hình |
| `theParty.Breathe()` | 1.5s | Tổ đội |
| `theQuest.CheckStatus()` | 5s | Trạng thái nhiệm vụ |

**Cổng tạm dừng toàn cục:** nếu `g_SysPause`, hoặc đang gom tiền (`m_nTongMoney`),
hoặc đang chạy về thành lưu rương (`m_nWcbStep`) → dừng phần auto chính nhịp đó.
Khi **Giới hạn Exp** bật thì chỉ chạy Recover/Collect + vài tác vụ nhẹ.

---

## 6. Quan hệ Đánh ↔ Di chuyển (phần cốt lõi)

Hai vòng `theAttack.Breathe()` (100ms) và `theMove.Breathe()` (200ms) **phối hợp
qua trạng thái dùng chung**, chủ yếu là `theAttack.m_nEnemyIdx` (mục tiêu đang
khóa) và `theMove.m_nStatus` (idle / movefirst / move / stand / attack / pickup).

### 6.1 `CVLAttack::Breathe()` — `VLHook/VLAttack.cpp:368`
Thứ tự xử lý mỗi nhịp đánh:
1. `HandleDirectSkills()` / `HandleInDirectSkills()` — buff & skill định kỳ.
2. Nếu đang tìm đường tới bãi train / đang ra skill / đang buff / tạm dừng → **return**
   (một trong các nguyên nhân gây "khựng 1 xíu" giữa các hit).
3. Ép tầm đánh `m_CurrentAttackRadius` theo skill đang dùng.
4. `AutoDefence()` — kiểm tra điều kiện phòng vệ/thoát.
5. Nếu đang khóa mục tiêu (`m_nEnemyIdx`) → để game tự đánh, return.
6. Nếu đang di chuyển bước đầu (`status_movefirst`) hoặc đang di chuyển mà không
   bật "đánh khi di chuyển" → return.
7. `LureBreathe()` — nếu đang **Quây quái** thì chưa đánh, return.
8. **`ProcessAttack()`** — ra đòn vào mục tiêu hiện tại.
9. `ApproachTarget()` — nếu đường thẳng tới quái bị tường chặn thì tự chạy vòng.
10. `IsLagTarget()` — bỏ mục tiêu lag/không hợp lệ.
11. **`SearchNearestNpc(nX, nY, nRange)`** — tìm quái gần **vị trí hiện tại của
    nhân vật** (không phải tâm điểm). Có quái → `SetPeopleIdx`, `status = attack`;
    hết quái → `SetPeopleIdx(0)`, `status = idle`.

### 6.2 `CVLMove::Breathe()` — `VLHook/VLMove.cpp:112`
- Ngay đầu: **`if (theAttack.m_nEnemyIdx) return;`** → **đang khóa quái thì KHÔNG
  di chuyển** (đứng đánh). Di chuyển chỉ xảy ra trong khoảng trống giữa hai mục tiêu.
- Chọn đích theo chế độ:
  - `m_bMoveByMapYS` → đi tuần theo các điểm trên map (`MapPoints`).
  - `m_bAroundPointPK` → đích là **tâm điểm** `m_nPointX/Y`.
  - `m_bRouteMove` → đích là node hiện tại trong `m_Nodes[]`.
- Tính khoảng cách `dZ` tới đích:
  - `dZ < 64` (đã tới) → `status_stand`, gọi `LureStart()` (nếu bật Quây quái);
    với around-point có thể ngồi nghỉ; với route/map thì lấy node kế tiếp.
  - `dZ` xa → `MoveTo(đích)` (kèm chống kẹt `IsLagMoving`).

### 6.3 Vì sao có hiện tượng "chạy đi một xíu rồi quay lại / khựng"
(kết luận điều tra — **không phải do cờ Quây quái bị kẹt**; mọi config đều `lure=0`):

1. Khi khóa 1 con quái, **game tự chạy tới đánh**; nếu quái di chuyển, nhân vật bị
   kéo xa dần khỏi tâm/điểm (`ApproachTarget` + FollowAttack của game).
2. Quái chết → tìm quái mới quanh **vị trí hiện tại** đã trôi → tiếp tục trôi.
3. Hết quái trong tầm → `status_idle` → `CVLMove::Breathe` thấy `dZ` tới tâm lớn →
   **chạy về tâm** = "chạy đi rồi quay lại".
4. "Khựng 1 xíu" = khoảng chờ ~1.2s khi trở lại chế độ đánh, hoặc lúc buff/đổi skill
   (bước 2 trong 6.1 return).

→ Đây là hành vi **đuổi quái rồi tự về tâm** vốn có của chế độ quanh-điểm/hành-trình,
không liên quan cờ lure. Muốn "đứng yên đánh" phải giới hạn việc đuổi ra ngoài vùng.

---

## 7. Tính năng "Quây quái" (lure) — tóm tắt

File: `VLHook/VLMove.cpp` (`LureStart` / `LureBreathe` / `LureStop`).

- Chỉ dùng **trạng thái tĩnh trong lúc chạy** (`s_nLureState`, `s_ptLureAnchor`…),
  **không ghi vào bất kỳ trường nào được lưu config**.
- Kích hoạt khi vừa tới tọa độ train, có ≥2 quái trong `LureRange`, và hết cooldown.
  Nhân vật chạy lướt qua từng quái rồi về mốc; trong lúc đó `CVLAttack::Breathe`
  return trước `ProcessAttack` (chưa đánh).
- Bị canh `if (!m_bLureCreep ...)` ở cả `LureStart` và `LureBreathe`: **tắt checkbox
  → `m_bLureCreep = FALSE` được đẩy xuống DLL ngay, lure dừng**. Khởi động lại game
  thì trạng thái tĩnh cũng reset → không thể "kẹt" qua phiên.

---

## 8. Các module khác (một dòng mỗi cái)

| Module | File | Vai trò |
|---|---|---|
| `CVLRecover` | `VLHook/VLRecover.cpp` | Hồi máu/mana, dùng thuốc, buff, auto-thoát khi nguy hiểm |
| `CVLCollect` | `VLHook/VLCollect.cpp` | Nhặt vật phẩm rơi theo bộ lọc |
| `CVLMap` | `VLHook/VLMap.cpp` | Chuyển bản đồ, đi xa phu/THP, mua đồ, về thành lưu rương |
| `CVLBasic` | `VLHook/VLBasic.cpp` | Thiết lập cơ bản, giảm cấu hình, giới hạn Exp |
| `CVLParty` | `VLHook/VLParty.cpp` | Lập/giữ tổ đội |
| `CVLQuest` | `VLHook/VLQuest.cpp` | Nhiệm vụ tự động (diệt quái, Tống Kim…) |
| `CVLUtil` | — | Tiện ích nền nhịp nhanh |

---

## 9. Sơ đồ rút gọn

```
VLAuto (GUI)                         VLHook (DLL trong game)
-----------                          -----------------------
Tick nhân vật ─ SetGameParams ──PostMessage(WM_HOOK_*)──► NewWndProc
                                       └─ ReadProcessMemory → theMove/theAttack/...
Login  ─ LoadProfile (đọc .cfg)
Logout ─ SaveProfile (ghi .cfg)

                          SetTimer 50ms → TimerProc (vòng lặp chính)
                            ├─ theRecover.Breathe   (50ms)
                            ├─ theCollect.Breathe   (50ms)
                            ├─ theAttack.Breathe    (100ms) ──┐ dùng chung
                            ├─ theMove.Breathe      (200ms) ──┘ m_nEnemyIdx / m_nStatus
                            ├─ theMap.Breathe       (600ms)
                            ├─ theBasic.Breathe     (1s)
                            ├─ theParty.Breathe     (1.5s)
                            └─ theQuest.CheckStatus (5s)
```

---

*Ghi chú: các con số offset/kích thước struct và hằng số lure nằm trong
`VLAuto/AutoParams.*`, `VLHook/VLClasses.h`, `VLHook/VLMove.cpp`. Khi thêm trường
mới vào `PARAM_MOVE`, luôn thêm ở **cuối** struct để config cũ vẫn đọc được.*
