# gamememory.md — Bản đồ bộ nhớ client Võ Lâm Truyền Kỳ (JX1) bản 15_10

> Tài liệu này dành cho AI/lập trình viên đọc để hiểu cấu trúc bộ nhớ, class, hàm trong game và cách gọi chúng
> từ một DLL inject vào tiến trình game. Mọi địa chỉ là **địa chỉ ảo tuyệt đối** (VA) trong tiến trình game.
>
> **Nguồn:** VLAuto_CTC (VLAuto/GameCore.h, VLHook/GameBase.h, JxCore.h/.cpp, S3Client.h/.cpp, KProtocol.h,
> GameLogin.h, GameDef.h, Offsets_15_10.md).
>
> **Trạng thái:** offset 15_10 được dò bằng cách diff mã máy với bản 10_10 cũ (7.362 cặp hàm khớp) và đã
> kiểm bằng anchor/static_assert. **Chưa chạy thử trong game.** Mục nào đánh dấu ⚠ là chưa chắc chắn.

---

## 1. Phiên bản & môi trường

| Mục | Giá trị |
|---|---|
| Client | `gamethienha_y_15_10.exe` (thư mục `I:\CTC\phienbangame`) |
| Kiến trúc | x86 32-bit, ImageBase `0x00400000`, không ASLR |
| Đóng gói | UPX (section UPX0). Offset lấy trên bản dump đã unpack |
| Lớp cửa sổ | `"Sword3 Class"` |
| Mã hoá chữ | **TCVN3** (ABC), không phải Unicode/UTF-8 |
| Tốc độ logic | 18 frame/giây (`KSubWorld::m_nCurrentTime` tăng 1 mỗi frame) |
| Bản cũ tham chiếu | 10_10 (dump trong `I:\JX20`) |
| CLIENT_SERIAL (nội bộ tool) | 683251 |

Kiểm tra hợp lệ nhanh: mọi địa chỉ đọc được từ con trỏ gốc phải nằm trong `0x400000..0x2000000`
(heap của game có thể cao hơn — điều kiện này chỉ dùng cho **địa chỉ tĩnh** trong bảng bên dưới).

---

## 2. Ghi chú về "seal" địa chỉ (chỉ áp dụng cho VLAuto)

Trong bản User (ADMIN_MODE=0) của VLAuto, 5 địa chỉ lõi bị XOR-mask để chống patch:
- VLHook: `addr = g_XxxAdd ^ g_GMask`
- VLAuto: `addr = theGameData.X ^ (ExeFileSize ^ CLIENT_SERIAL ^ LP_POISON())`

Bản Tester dùng hằng số trực tiếp. **Dự án mới cứ dùng hằng số thẳng** như trong tài liệu này.

---

## 3. Các mẫu truy cập cơ bản

### 3.1 Con trỏ gốc → mảng đối tượng
Các mảng đối tượng nằm trên heap, chỉ có **con trỏ đến phần tử đầu** là tĩnh:

```cpp
// phần tử i của mảng = *(BASE) + i * SIZE
KNpc*    g_pNpcArray[i]  = (KNpc*)   (*(DWORD*)0x017220F0 + i * 0x11AB8);  // i < 256
KItem*   g_pItemArray[i] = (KItem*)  (*(DWORD*)0x01722148 + i * 0x864);    // i < 1024
KObj*    g_pObjArray[i]  = (KObj*)   (*(DWORD*)0x017286B4 + i * 0x4E8);    // i < 256
KSkill*  skill[i]        = (KSkill*) (0x0084ACB8 + i * 0x4F8);             // MẢNG TĨNH, không deref; i < 3000
KPlayer* g_pCurPlayer    = (KPlayer*)(*(DWORD*)0x00EE403C + 0xA878);        // Player[1] (Player[0] là rỗng)
KSubWorld* g_pSubWorld   = (KSubWorld*)(*(DWORD*)0x017286B0);
KCoreShell* g_pCoreShell = (KCoreShell*)(*(DWORD*)0x00848314);
```

Index 0 của mọi mảng là phần tử rỗng/không dùng. Một NPC tồn tại khi `m_Index > 0`.

### 3.2 NPC của chính người chơi
```cpp
KNpc* P_PLAYER = g_pNpcArray[g_pCurPlayer->m_nIndex];   // KPlayer+0x13C
int   PEOPLE_IDX = P_PLAYER->m_nPeopleIdx;              // NPC đang được chọn/target
```

### 3.3 Hệ toạ độ
Toạ độ "game" (pixel thế giới) tính từ region + ô + offset:
```
X = RegX * 512  + MapX * 32 + (OffX >> 10)
Y = RegY * 1024 + MapY * 32 + (OffY >> 10)
```
- Region: 16×32 ô; ô = 32×32 pixel logic.
- Toạ độ "bản đồ" hiển thị trên minimap/chat = `X/256, Y/512` (thường gặp kiểu "200/180").
- Hàm `KNpc::GetMpsPos (0x5BC920)` trả thẳng X, Y game.
- Lệnh tìm đường (GoTo) nhận `x/32, y/32` (đơn vị ô).

### 3.4 Chuỗi trong game
- Mã TCVN3. Viết hoa: `a-z` → `A-Z`, và `0xA8..0xAE` → `0xA1..0xA7`.
- Mã màu nhúng: `0x02 r g b` (bắt đầu màu, các byte màu có thể là 0) và `0x03` (kết thúc màu).
  Khi so chuỗi phải bỏ qua các mã này (tool có `GameTextLen`/`GameTextPlain`).
- Hash tên file/chuỗi (dùng trong pak và id tên bang):
```cpp
DWORD HashString2ID(const char* s) {
    DWORD Id = 0;
    for (int i = 0; s[i]; i++)
        Id = (Id + (i + 1) * (unsigned char)s[i]) % 0x8000000B * 0xFFFFFFEF;
    return Id ^ 0x12345678;
}
```

### 3.5 Quy ước gọi hàm
- **thiscall** (hàm thành viên MSVC): `this` trong `ecx`, tham số push phải→trái, callee dọn stack.
- **cdecl**: tham số push phải→trái, caller dọn stack (`add esp, 4*n`).

Mẫu wrapper:
```cpp
// thiscall: int KCoreShell::OperationRequest(unsigned uOper, int uParam, int nParam)
int OperationRequest(unsigned op, int p1, int p2) {
    int r; void* self = *(void**)0x00848314;
    __asm {
        push p2
        push p1
        push op
        mov  ecx, self
        mov  eax, 0x005C1180
        call eax
        mov  r, eax
    }
    return r;
}

// cdecl: dùng con trỏ hàm
typedef void (__cdecl *PFN_ExecuteScript)(const char*);
((PFN_ExecuteScript)0x00423B40)("Switch([[sit]])");
```

---

## 4. Bảng địa chỉ toàn cục (biến / con trỏ / đối tượng tĩnh)

### 4.1 Lõi (GameCore.h — VLAuto)
| Tên macro | Địa chỉ | Kiểu | Mô tả |
|---|---|---|---|
| `ACNT_ID_ADD` | `0x008491E8` | char[] (đảo bit) | Tên tài khoản đăng nhập = `g_LoginLogic(0x848A68)+0x780`. Mỗi byte bị `~` (NOT), đọc xong phải đảo lại |
| `MAP_ID_ADD` | `0x0175D6E8` | int | ID bản đồ hiện tại (xem bảng CITY_MAP_ID) |
| `MAP_NAME_ADD` | `0x0175D6C8` | char[32] | Tên bản đồ hiện tại (TCVN3) |
| `CORE_BASE_ADD` | `0x00848314` | KCoreShell** | Con trỏ g_pCoreShell — cổng gọi mọi thao tác game |
| `NPC_BASE_ADD` | `0x017220F0` | KNpc** | Con trỏ đầu mảng NPC (256 × 0x11AB8) |
| `PLR_BASE_ADD` | `0x00EE403C` | KPlayer** | Con trỏ đầu mảng Player; người chơi hiện tại = +0xA878 |
| `WLD_BASE_ADD` | `0x017286B0` | KSubWorld** | Thế giới con hiện tại |
| `ITM_BASE_ADD` | `0x01722148` | KItem** | Con trỏ đầu mảng vật phẩm (1024 × 0x864) |
| `SKL_BASE_ADD` | `0x0084ACB8` | KSkill[3000] | **Mảng tĩnh** kỹ năng (0x4F8/phần tử) |
| `SERVERIP_ADD` | `0x07946BB0` | char[] | IP server đang kết nối |
| `FLAG_X_ADD` / `FLAG_Y_ADD` | `0x007AAA58` / `0x007AAA5C` | int | Toạ độ cờ đích trên minimap (đặt bởi SMF) |

| Offset/kích thước | Giá trị | Ý nghĩa |
|---|---|---|
| `WLD_TIME_OFFSET` | `0x50` | `KSubWorld::m_nCurrentTime` (frame, 18/s) |
| `PLAYER_OFFSET` | `0xA878` | = sizeof(KPlayer); Player[1] |
| `PLR_IDX_OFFSET` | `0x13C` | `KPlayer::m_nIndex` (index NPC của mình) |
| `PLR_EXP_OFFSET` | `0x75F0` | `KPlayer::m_nCurExp` (__int64) |
| `PLR_ROOM_OFFSET` | `0x6C88` | = 0x144 (ItemList) + 0x6B44 (Room[]) → `KInventory[15]` |
| `INVENTORY_SIZE` | `0x1C` | sizeof(KInventory) |
| `NPC_SIZE` | `0x11AB8` | sizeof(KNpc) |
| `ITM_SIZE` | `0x864` | sizeof(KItem) |
| `SKL_SIZE` | `0x4F8` | sizeof(KSkill) |
| `MAX_NPC` / `MAX_ITEM` / `MAX_SKILL` | 256 / 1024 / 3000 | Giới hạn mảng |

`SKILL_TYPE`: 0 = tấn công tầm xa, 1 = cận chiến, 2 = hào quang (aura), 3 = bị động, 4 = bay/khinh công.

### 4.2 Đối tượng & UI (GameBase.h — VLHook)
| Macro | Địa chỉ | Kiểu / Ghi chú |
|---|---|---|
| `OBJ_BASE_ADD` | `0x017286B4` | KObj** — vật thể rơi trên đất (256 × 0x4E8) |
| `TEAM_BASE_ADD` | `0x00EE4850` | KTeam* — `g_Team` nằm ở `0xEE4848`, dữ liệu thành viên ở +8 |
| `NSET_BASE_ADD` | `0x017286C0` | KNpcSet (NpcSet::SearchID 0x6368D0) |
| `BSL_BASE_ADD` | `0x01A6A620` | KBuySell* (`g_pBuySell`) |
| `LEVEL_EXP_TB` | `0x01A7D518` | int[999] bảng kinh nghiệm cần để lên cấp (`PlayerSet 0x1A7D4E0 + 0x38`) |
| `LIFETIME_DYS` / `LIFETIME_HRS` | `0x006DEFA4` / `0x006DEFA8` | Chuỗi định dạng "ngày"/"giờ" hạn dùng vật phẩm |
| `GOLD_BLUE_MAX` | `0x01729548` | Bảng màu tên NPC tinh anh/boss |
| `RPS_BASE_ADD` | `0x00EDFC88` | iRepresentShell* (`g_pRepresent`) — vẽ chữ/hình |
| `SCP_BASE_ADD` | `0x0172B1F8` | KScenePlaceC (cảnh, region, chướng ngại) |
| `NET_CLIENT` | `0x0084A9EC` | Đối tượng mạng client |
| MAGIC_ATTRIB name table | `0x00EE0278 + 4*ID` | char* tên thuộc tính ma pháp theo ID |
| `UPB_BASE_ADD` | `0x007BE494` | KUiPlayerBar* (thanh chat / phím tắt dưới màn hình) |
| `SBO_BASE_ADD` | `0x007C488C` | KUiStoreBox* (rương) |
| `MCP_BASE_ADD` | `0x007C9BE8` | Popup menu/chọn |
| `SMC_BASE_ADD` | `0x007CA1FC` | KUiSysMsgCentre* (trung tâm thông báo hệ thống: mời tổ đội, giao dịch…) |
| `YSB_BASE_ADD` | `0x007CAD4C` | KUiYeSouBox* (hộp Dã Tẩu) |
| `MSG_BASE_ADD` | `0x007CBD2C` | KUiMsgSel* (hộp thoại chọn NPC kiểu 1) |
| `MSG2_BASE_ADD` | `0x007CB470` | KUiMsgSel2* (hộp thoại chọn NPC kiểu 2 — dùng nhiều nhất) |
| `SHOP_BASE_ADD` | `0x007C9D3C` | KUiShop* (cửa hàng NPC) |
| `GSHOP` m_pSelf | `0x007C9D38` | Kỳ Trân Các (shop tiền xu) |
| `MMP_BASE_ADD` | `0x007CA200` | KUiMiniMap* |
| `UIN_BASE_ADD` | `0x007C47D8` | KUiInit* (màn hình khởi đầu: vào game / thoát) |
| `IF1_BASE_ADD` | `0x007C5DD8` | KUiInformation (**đối tượng tĩnh**, không deref) |
| `IF2_BASE_ADD` | `0x007C6CC8` | KUiInformation2 (**tĩnh**) |
| `STL_BASE_ADD` | `0x007CAD54` | KUiPlayerStall* (bày bán) |
| `PZS_BASE_ADD` | `0x007CBD1C` | KUiPrizeSel* (chọn phần thưởng) |
| `TMC` / `CMP` / `ITX` | — | **Không tìm thấy** trong 15_10 (NULL_PTR) |
| `KOption` | `0x017969C0` | +0 âm lượng hiệu ứng, +4 nhạc nền |
| `KMyApp` | `0x00848360` | Ứng dụng chính |
| GoTo `this` | `0x01796838` | Đối tượng tìm đường tự động |

Hằng số liên quan:
| Tên | Giá trị | Ý nghĩa |
|---|---|---|
| `MAX_OBJ` | 256 | Số vật thể rơi |
| `MAX_TOP_TONG` | 25 | Bảng xếp hạng bang |
| `MAX_TEAM_MEMBER` | 8 | Thành viên tổ đội |
| `MAX_PLAYER_ITEM` | 678 | Số ô vật phẩm KItemList |
| `MAX_EQUIP_SLOT` | 16 | Ô trang bị |
| `OBJ_DATA_SIZE` | 0x4E8 | sizeof(KObj) |
| `MSG_PLR_OFFSET` | 0x408 | KSystemMessage: tên người mời |
| `MAX_LEVEL_EXP` | 999 | Kích thước bảng kinh nghiệm |
| `MAX_FOLLOW_DISTANCE` | 48 | Khoảng cách đi theo |
| `MIN_RETURN_DISTANCE` | 64 | |
| `MAX_PICKUP_DISTANCE` | 128 | Bán kính nhặt đồ |
| `MAX_PLAYER_MOVE_FRAME` | 7 | Số frame tối thiểu giữa 2 lệnh di chuyển |
| `MAX_MOVE_DISTANCE` | 400 | Mỗi lệnh MoveTo bị kẹp tối đa 400 |
| `MAX_ROOM_SPACE` | 60 | Số ô hành trang |
| `NGAMY_BUFF_SKILLID` | 0x5D | Skill buff của Nga Mi, bán kính 400 |
| `WND_N_MENU_CLICK` | 0x502 | Thông điệp UI: click menu |
| `WND_N_BUTTON_CLICK` | 0x565 | Thông điệp UI: click nút |
| `SKILL_SHORTCUT` | 0x40004 | |
| Window style | `VISIBLE 0x80000000`, `DISABLED 0x40000000`, `TOPMOST 0x20000000`, `MOVEABLE 0x10000000`, `DISABLE 0x04000000` | KWndImage::m_Style (+4) |

---

## 5. Bảng hàm trong game

Ký hiệu: `thiscall(n)` = hàm thành viên có n tham số; `cdecl(n)` = hàm tự do n tham số.

### 5.1 KCoreShell (this = `*(DWORD*)0x848314`)
| Macro | Địa chỉ | Chữ ký | Mô tả |
|---|---|---|---|
| `GGD` | `0x5BC2D0` | `int GetGameData(uint uDataId, uint uParam, int nParam)` thiscall(3) | Đọc dữ liệu UI/game (mô tả vật phẩm, giá, đồ bày bán…). Xem GDI |
| `OPR` | `0x5C1180` | `int OperationRequest(uint uOper, uint uParam, int nParam)` thiscall(3) | **Hàm thao tác chính**: dùng đồ, mua/bán/sửa, đổi chỗ, chọn menu, tuỳ chọn… Xem GOI |
| `TOP` | `0x5BD440` | `int TeamOperation(uint uOper, uint uParam, int nParam)` thiscall(3) | Tổ đội. Xem TEAM_* |
| `TNP` | `0x5C5300` | `int TongOperation(uint uOper, uint uParam, int nParam)` thiscall(3) | Bang hội |
| `PGS` | `0x5BD780` | `int ProcessGameSecurity(int op, int p1, int p2)` thiscall(3) | Khoá rương: op 1 = có chức năng khoá?, 0 = đang khoá?, 6 = mở khoá với mật khẩu p1, 5 = khoá lại |
| `GRL` | `0x5BD0C0` | `int GetNPCRelation(int nNpcIdx)` thiscall(1) | Quan hệ với NPC: 1 none, 2 self, 4 ally, 8 enemy, 16 dialog |
| `DGS` | `0x5BD120` | DrawGameSpace | Vẽ không gian game (hook tại `DGS_JMP 0x4D78CD`, lệnh E8 call) |

### 5.2 KPlayer (this = g_pCurPlayer)
| Macro | Địa chỉ | Chữ ký | Mô tả |
|---|---|---|---|
| `PAA` | `0x5E8E70` | `void ApplyAddBaseAttribute(int nAttrib, int nNo)` thiscall(2) | Cộng điểm tiềm năng nNo vào thuộc tính nAttrib (⚠ thứ tự mã thuộc tính chưa xác minh) |
| `PKO` | `0x5EC7E0` | `void PickUpObj(int nObjIndex)` thiscall(1) | Nhặt vật thể trên đất |
| `DNPC` | `0x5E9880` | `void DialogNpc(int nNpcIndex)` thiscall(1) | Nói chuyện với NPC (mở hộp thoại) |
| SetLeftSkill | `0x5E8FD0` | thiscall | Đặt skill tay trái |
| CheckObject | `0x5EE670` | | |
| (tạo gói skill) | `0x5E8280` | | Dựng packet skill (xem §11.6) |

### 5.3 KNpc (this = KNpc*)
| Macro | Địa chỉ | Chữ ký | Mô tả |
|---|---|---|---|
| `MVT` | `0x5F8CF0` | `void MoveTo(int x, int y, int action)` thiscall(3) | Di chuyển NPC của mình tới toạ độ game |
| `NMP` | `0x5BC920` | `void GetMpsPos(int* pX, int* pY)` thiscall(2) | Lấy toạ độ game |
| `NPCP` | `0x5F7CA0` | `KNpc::Paint` | Vẽ NPC (call site `NPCP_CALL 0x642B23` trong CoreDrawGameObj `0x642940`) |
| SendCommand | `0x5F7490` | thiscall | Gửi lệnh NpcCommand |
| SetFightMode | `0x5FEA90` | thiscall | Chế độ chiến đấu |
| KNpc ctor | `0x5FF860` | | |
| KNpc::Init | `0x5F94D0` | | |

### 5.4 Item / Task / BuySell
| Macro | Địa chỉ | Chữ ký | Mô tả |
|---|---|---|---|
| `IGR` | `0x6051B0` | `int KItem::GetRepairPrice()` thiscall(0) | Giá sửa |
| `GTV` | `0x609990` | `int KPlayerTask::GetTaskVal(int nTaskId)` thiscall(1) | Đọc biến nhiệm vụ (this = `&g_pCurPlayer->m_cTask`, KPlayer+0xA248). Trả 0 nếu id > 0x176F |
| `BGI` | `0x6852B0` | `int KBuySell::GetItem(int nIdx)` thiscall(1) | Vật phẩm của shop |
| `BGX` | `0x685300` | `int KBuySell::GetItemIndex(int nShop, int nIdx)` thiscall(2) | |

### 5.5 Hàm tự do (cdecl)
| Macro | Địa chỉ | Chữ ký | Mô tả |
|---|---|---|---|
| `EXS` | `0x423B40` | `void ExecuteScript(const char*)` cdecl(1) | Chạy lệnh script UI: `"Switch([[sit]])"` (ngồi), `"Switch([[horse]])"` (lên/xuống ngựa) |
| `CLA` | `0x432B10` | `void CloseAllWindows(int)` cdecl(1) | Đóng mọi cửa sổ UI |
| `SKL_FUNC` | `0x644CB0` | `void SendClientCmdSkill(int skill, int x, int y)` cdecl(3) | Gửi lệnh dùng skill. Lên mục tiêu: x = -1, y = npc->m_dwID (xem §11.6) |
| SendClientCmdRun | `0x644FD0` | cdecl | Gửi chạy |
| SendClientCmdWalk | `0x645060` | cdecl | Gửi đi bộ |
| `SBO_FUNC` | `0x4991B0` | `KUiStoreBox::OpenWindow()` cdecl(0) | Mở rương |
| `SHO` | `0x4CB1F0` | `KUiShop::OpenWindow()` cdecl(0) | Mở shop |
| `SMA` | `0x4BD680` | `void SystemMessageArrival(const char* msg, WORD len, int channel)` cdecl(3) | Hiện tin hệ thống trên khung chat (len gồm '\0') |
| `PCP` | `0x4D7A40` | `void ProcessPeople(KUiPlayerItem*, int action)` cdecl(2) | Hành động với người chơi (ACTION_*) |
| `SMF` | `0x4DE840` | `void MiniMapGoto(int x, int y, int bWorld)` cdecl(3) | Đặt cờ/đi tới điểm trên minimap |
| `OCB` | `0x516790` | `KUiYeSouBox::CloseWindow(int bAssemble)` cdecl(1) | Đóng hộp Dã Tẩu (1 = xác nhận) |
| `PSC` | `0x53EAE0` | `KUiPrizeSel::CloseWindow(int)` cdecl(1) | Đóng hộp chọn thưởng |
| GoTo | `0x642730` | thiscall(2), this = `0x1796838`, args `(x/32, y/32)` | Tự tìm đường tới ô |

### 5.6 UI (thiscall, this = đối tượng UI tương ứng)
| Macro | Địa chỉ | Chữ ký | Mô tả |
|---|---|---|---|
| `UWP` | `0x46FCA0` | `int KUiPlayerBar::WndProc(uint uMsg, uint uParam, int nParam)` thiscall(3) | vtable `0x751A84[4]`. Dùng giả lập click nút |
| `IOC` | `0x490EA0` | `KUiInit::OnClickButton(int nBtn)` thiscall(1) | |
| `IH1` | `0x4B7440` | `KUiInformation::Hide(int)` thiscall(1) | Đóng hộp thông báo 1 |
| `IH2` | `0x4B80E0` | `KUiInformation2::Hide()` thiscall(0) | Đóng hộp thông báo 2 |
| `SMD` | `0x4DA540` | `KUiSysMsgCentre::DeleteMsgInHeap(int heap, int idx)` thiscall(2) | Xoá tin hệ thống |
| `OMS` | `0x542110` | `KUiMsgSel::OnClickMsg(int nIdx)` thiscall(1) | Chọn dòng trong hộp thoại NPC kiểu 1 |
| `OMS2` | `0x522120` | `KUiMsgSel2::OnClickMsg(int nIdx)` thiscall(1) | Chọn dòng trong hộp thoại NPC kiểu 2 |
| YeSou OpenWindow | `0x5165D0` | | `new` 0x3794 byte |
| KUiMsgSel2::OpenWindow | `0x5210C0` | | Gọi từ CoreDataChanged id 64 |
| `OSM` | 0 | — | Không có trong 15_10 |

### 5.7 Hàm hệ thống khác (tham khảo, dùng khi hook)
| Hàm | Địa chỉ |
|---|---|
| GameLoop | `0x5A9160` |
| SelectRole | `0x5AAE40` |
| Skill load | `0x5C99F0` |
| SyncNpc | `0x622D90` |
| SyncPlayer | `0x623E00` |
| ScenePaint | `0x63C2E0` |
| ObjSet::AddData | `0x6774E0` |
| ObjSet::ClientAdd | `0x677F90` |
| NpcSet::SearchID | `0x6368D0` |
| CoreDataChanged | `0x409C20` |
| KClientCallback vtable | `0x7600B0` |
| ChannelMessageArrival | `0x40DA10` |
| Địa chỉ inject cũ (đã tắt) | `0x5550DA`, `0x5550DF`, `0x5AE5C0` |

---

## 6. Layout các struct/class lõi

> Offset theo byte, hex. "→" = theo sau liên tiếp. Các trường không ghi là padding/chưa rõ.

### 6.1 KNpc (sizeof = 0x11AB8)
| Offset | Kiểu | Trường | Ghi chú |
|---|---|---|---|
| 0x0000 | DWORD | m_dwID | ID mạng (dùng trong packet) |
| 0x0004 | int | m_Index | Index trong mảng; > 0 = tồn tại |
| 0x0008 | KIndexNode | m_Node | 0x18 byte |
| 0x0020 | int | m_Level | Cấp |
| 0x0024 | int | m_Kind | NPC_KIND: 0 normal(quái), 1 player, 2 partner, 3 dialoger(NPC nói chuyện), 4 bird, 5 mouse |
| 0x0028 | int | m_Series | Ngũ hành 0 kim,1 mộc,2 thuỷ,3 hoả,4 thổ |
| 0x002C | int | m_Height | |
| 0x0030 | int | m_btRankId | |
| 0x0034 | int | m_nStature | |
| 0x0038 | int | m_VNFlag | |
| 0x003C | KNpcGold | m_cGold | NpcIdx, IsGold, IsGolding, GoldType (quái tinh anh/boss) |
| 0x0074 | KState | m_PowerState | Mỗi KState = 0x10: attrib, value[2], time |
| 0x0084 → | KState ×6 | Poison, Freeze, Burn, Confuse, Stun, Drunk | Trạng thái bất lợi liên tiếp, mỗi cái 0x10 |
| 0x00E4 | int | m_Camp | Phe gốc |
| 0x00E8 | int | m_CurrentCamp | Phe hiện tại |
| 0x00EC | int | m_Doing | Hành động hiện tại (NPCCMD/do_*) |
| 0x00FC | KStateNode* | m_StateSkillList.head | Danh sách buff/debuff |
| 0x0100 | KStateNode* | tail | |
| 0x0104 | int | count | |
| 0x0108 | int | m_ActiveSkillID | Skill đang dùng |
| 0x0110 | KSkillList | m_SkillList | |
| 0x0148 | NpcSkill[80] | m_Skills | 0x30/phần tử: Id, Level, … |
| 0x1048 | int | m_CurrentLife | **HP hiện tại** |
| 0x104C → | int[4] | LifeReplenish… | |
| 0x105C | int | m_CurrentMana | **MP hiện tại** |
| 0x1060 | int | ManaReplenish | |
| 0x1064 | int | m_CurrentStamina | Thể lực |
| 0x1068 | int | m_CurrentStaminaMax | |
| 0x1144 | int | m_CurrentWalkSpeed | |
| 0x1148 | int | m_CurrentRunSpeed | |
| 0x1164 | int | m_CurrentAttackRadius | |
| 0x1168 | int | m_CurrentActiveRadius | |
| 0x1344 | int | m_Dir | Hướng 0..63 |
| 0x1348 → | int×3 | R/G/B Lum | |
| 0x1354 | int ×3 | m_MapX, m_MapY, m_MapZ | Ô trong region |
| 0x1360 | int ×2 | m_OffX, m_OffY | Offset trong ô (<<10) |
| 0x13B8 | char | Name0 | Byte đầu tên (thường là mã màu) |
| 0x13B9 | char[39] | Name | Tên (TCVN3) |
| 0x13E0 | int | m_nSex | |
| 0x13E4 | int | m_NpcSettingIdx | |
| 0x13E8 | int | m_CorpseSettingIdx | |
| 0x1448 | int | m_nPeopleIdx | NPC đang được chọn |
| 0x144C → | int[3] | m_nLastDamageIdx… | |
| 0x1458 | int | m_nObjectIdx | Vật thể đang chọn |
| 0x1A58 | int | m_FightMode | 0 = trong thành/không chiến, 1 = chiến đấu |
| 0x1A74 | DWORD | m_SyncSignal | |
| 0x1A7C | int | ClientNpcID | |
| 0x1A84 | WORD | m_RegionX | |
| 0x1A86 | WORD | m_RegionY | |
| 0x1A88 | int[6] | m_SyncPos | |
| 0x1AA0 | int | m_nPKFlag | Trạng thái PK (0 luyện công, 1 chính phái, 2 đồ sát…) |
| 0x1CCC | DWORD | m_dwTongNameID | ⚠ Hash tên bang |
| 0x1CD0 | int | m_nMenuState | 0 thường, 1 đang bày bán, 2 đang tạo đội |
| 0x1CDC | int | m_nRelGroup | |
| 0x1D18 | NpcCommand | m_Command | CmdKind, X, Y, Z (ghi trực tiếp = SendCommand) |
| 0x1D78 | int | m_bRideHorse | Đang cưỡi ngựa |
| 0x1D84 | char[64] | TopText | Chữ trên đầu |
| 0x11A00 | int | m_CurrentLifeMax | |
| 0x11A04 | int | m_CurrentLifeMaxEx | **HP max = max(0x11A00, 0x11A04)** |
| 0x11A08 | int | m_CurrentManaMax | |
| 0x11A0C | int | m_CurrentManaMaxEx | **MP max = max(0x11A08, 0x11A0C)** |

Hàm tiện ích (tool tự viết):
- `IsExist()` = `m_Index > 0`; `IsNotMob()` = Kind != 0.
- `GetGamePos(&x,&y)` gọi NMP; `IsGhost` = toạ độ sync khác toạ độ thật > 32.
- `MoveTo(x,y,act)` gọi MVT; `SendCommand(kind,x,y,z)` ghi vào m_Command.

### 6.2 KPlayer (sizeof = 0xA878)
| Offset | Kiểu | Trường | Ghi chú |
|---|---|---|---|
| 0x0000 | void* | vfptr | |
| 0x0020 | BYTE | m_nEntrustMode | ≠0 = đang **uỷ thác online** (không được điều khiển) |
| 0x0024 | int | m_nLeftSkillID | Skill tay trái |
| 0x0028 | int | m_nLeftSkillLevel | |
| 0x002C | int | m_nRightSkillID | Skill tay phải |
| 0x0030 | int | m_nRightSkillLevel | |
| 0x00EC | int | m_RunStatus | 1 chạy / 0 đi |
| 0x00F0 | int | m_nNextLevelLeadExp | |
| 0x00F4 | int | m_nSendMoveFrames | Frame kể từ lệnh di chuyển cuối; phải ≥7 mới gửi lệnh mới |
| 0x0138 | DWORD | m_dwHashID | |
| 0x013C | int | m_nIndex | Index KNpc của mình |
| 0x0140 | int | m_nNetConnectIdx | |
| 0x0144 | KItemList | m_ItemList | xem 6.3 |
| 0x6F24 | int | m_nCurShop | Shop đang mở |
| 0x75F0 | __int64 | m_nCurExp | Kinh nghiệm hiện tại |
| 0x75F8 | __int64 | m_nNextExp | |
| 0x7628 | KPlayerTeam | m_cTeam | +0 m_nFlag (đang trong đội), +4 m_nFigure (0 = đội trưởng) |
| 0x7BD0 | int | m_nCurFaction | Môn phái |
| 0x7E18 | KPlayerPK | m_cPK | |
| 0x7E70 | int | m_nPKValue | Điểm PK |
| 0x7E74 | KPlayerTong | m_cTong | xem 6.8 |
| 0xA248 | KPlayerTask | m_cTask | 0x630 byte; std::map ở +0x54C; đọc bằng GTV |

Macro hay dùng:
```
IAM_IN_A_PARTY  = g_pCurPlayer->m_cTeam.m_nFlag
IAM_THE_CAPTAIN = g_pCurPlayer->m_cTeam.m_nFigure == 0
PLAY_HAND_IDX   = g_pCurPlayer->m_ItemList.m_nForeHand   // vật phẩm đang cầm trên chuột
HAVE_HORSE      = m_ItemList.GetEquip(itempart_horse) > 0
```

### 6.3 KItemList (nằm ở KPlayer+0x144)
| Offset (trong KItemList) | Kiểu | Trường |
|---|---|---|
| 0x0020 | int | m_nForeHand (item index trên tay chuột) |
| 0x0028 | {int nIdx; int nData}[16] | m_EquipItem — trang bị đang mặc theo ITEM_PART |
| 0x00A8 | PlayerItem[678] | m_Items — mỗi phần tử 0x14: nIdx, nPlace, X, Y, Z |
| 0x6AF0 | int[] | m_UseIdx |
| 0x6AFC | int | m_nListCurIdx |
| 0x6B44 | KInventory[15] | m_Room — mỗi phòng 0x1C, index theo INVENTORY_ROOM |

Hàm `0x60D940` truy cập: `+0x6BD0` = room 5 (immediacy), `+0x6CCC` = room 14 (guid).

### 6.4 KInventory (0x1C)
| Offset | Trường | Ghi chú |
|---|---|---|
| 0x00 | int* m_pArray | Lưới W×H; giá trị = item index (0 = trống). Ô (i,j) = `pArray[j*W + i]` |
| 0x04 | int m_nMoney | Tiền trong phòng |
| 0x08 | int m_nWidth | |
| 0x0C | int m_nHeight | |
| 0x10 | int m_nMaxWidth | |
| 0x14 | int m_nMaxHeight | |
| 0x18 | int reserved | |

```
PLAYER_ROOM  = g_pCurPlayer->m_ItemList.m_Room
EQUIP_ROOM   = PLAYER_ROOM[room_equipment(0)]       // hành trang
EQUIP_MONEY  = PLAYER_ROOM[0].m_nMoney              // tiền mang theo
ROOM_MONEY   = PLAYER_ROOM[room_repository(1)].m_nMoney  // tiền trong rương
```

### 6.5 KItem (sizeof = 0x864)
| Offset | Kiểu | Trường | Ghi chú |
|---|---|---|---|
| 0x000 | void* | vfptr | |
| 0x004 | KCommonAttrib | m_CommonAttrib | xem 6.6; tên ở **0x30** |
| … | KCommonAttribEx | 0x108 byte | |
| 0x330 | KMagicAttrib[19] | aryAllAttrib | Mỗi cái 0x10: type, value[3] |
| 0x530 | int[6] | MagicLevel | |
| 0x6A4 | DWORD | m_dwUniID | |
| 0x6A8 | int | m_nCurDurability | **= số lượng** khi bStack |
| 0x6AC | int | m_nMaxDurability | |
| 0x6D8 | BYTE | ItemPlus | Cấp cường hoá |
| 0x6D9 | BYTE | ExParam1 | |
| 0x6DA | WORD | ExParam2 | |
| 0x6DC | int | LeftUsageTime | |
| 0x6E0 | int | ExpiredTime | Hạn dùng |
| 0x6E4 | int | LockCount | -1 khoá vĩnh viễn, >0 khoá có hạn, 0 không khoá |
| 0x860 | int | NoSell | Cờ không bán được |

### 6.6 KCommonAttrib (KItem+4)
| Offset trong KItem | Trường |
|---|---|
| 0x04 | nItemGenre (ITEMGENRE) |
| 0x08 | nSpecialType |
| 0x0C | nDetailType (EQUIPDETAIL / MEDICINE…) |
| 0x10 | nParticularType |
| 0x14 | nObjIdx |
| 0x18 | bStack |
| 0x1C | nWidth |
| 0x20 | nHeight |
| 0x24 | nPrice |
| 0x28 | nLevel |
| 0x2C | nSeries |
| 0x30 | szItemName[80] |
| 0x80 | szImageName[80] |
| 0xD0 | szIntro[256] |
| 0x1D0 | szScript[84] |
| … | nNpcImage |

### 6.7 KObj — vật thể trên đất (0x4E8)
| Offset | Trường | Offset | Trường |
|---|---|---|---|
| 0x00 | m_nID | 0x34 | m_nLifeTime |
| 0x04 | m_nDataID | 0x38 | m_nBornTime |
| 0x08 | m_nKind (OBJKIND) | 0x3C | m_nWaitTime |
| 0x0C | m_nIndex | 0x40 | ObjSkill m_cSkill (8 int: Kind, Camp, Range, CastTime, ID, Level, TarX, TarY) |
| 0x10 | WORD m_nRegionX | 0x60 | char m_szName[32] |
| 0x12 | WORD m_nRegionY | 0x194 | m_nScriptID |
| 0x14 | m_nSubWorldID | 0x198 | m_nColorID |
| 0x18 | m_nRegionIdx | 0x19C | m_nItemDataID (index KItem) |
| 0x1C | m_nMapX | 0x1A0 | m_nItemWidth |
| 0x20 | m_nMapY | 0x1A4 | m_nItemHeight |
| 0x24 | m_nOffX | 0x1A8 | m_nMoneyNum |
| 0x28 | m_nOffY | 0x3A4 | m_nDropState |
| 0x2C | m_nDir | 0x478 | m_dwNameColor |
| 0x30 | m_nState | | |

OBJKIND: 0 MapObj, 1 Body (xác), 2 Box, 3 **Item**, 4 **Money**, 10 StoreBox.

### 6.8 Các struct khác
**KSkill (0x4F8, mảng tĩnh 0x84ACB8)**
| Offset | Trường |
|---|---|
| 0x00 | m_nIndex |
| 0x04 | m_nType (SKILL_TYPE) |
| 0x08 | m_nMaxLevel |
| 0x0C | char m_szName[32] |
| 0x2C | m_nId |
| 0x38 | m_bIsPhysics |
| 0x50 | m_nAttackRadius (Range) |
| 0xB4 | m_bHorseLimited (không dùng khi cưỡi ngựa) |

**KSubWorld**: 12 int dự trữ, rồi RegionWidth, RegionHeight, TotalRegion, CellWidth, CellHeight, RegionBeginX, RegionBeginY, Weather; **m_nCurrentTime ở 0x50**.

**KTeam** (`0xEE4850`): `int Member[8]` (NPC/Player ID), `int MemNum`, `int MemNumEx`, `int MemLevel[8]`, `char MemName[8][32]`.

**KPlayerTong** (KPlayer+0x7E74): `char MasterName[32]`, 3 int, `m_nFlag 0x2C` (có bang), `m_nFigure` (chức vụ), `m_nCamp`, unk 0x38, `m_dwTongNameID 0x3C`, `char TongName[32]`, … 0x68 byte, int ở 0xC8.

**KStallObject** (0x154 — món hàng bày bán): uId +0, X/Y/W/H ở 0x138/0x13C/0x140/0x144, giá ở 0x148.

**KItemObject** (0x1C — tham số cho OperationRequest với item):
```cpp
struct KItemObject {   // a.k.a. KUiObjAtContRegion
    unsigned uGenre;   // CGOG_*
    unsigned uId;      // item index / id
    int DataX, DataY;  // vị trí trong container
    int DataW, DataH;
    int eContainer;    // UOC_*
};
```

**Struct nhỏ**
- KIndexNode: vfptr, next, prev, 3 DWORD.
- KNpcGold: NpcIdx, IsGold, IsGolding, GoldType.
- KState (0x10): attrib, value[2], time.
- KMagicAttrib (0x10): nAttribType, nValue[3].
- KStateNode : KNode(vfptr, next, prev) → Data, SkillID, Level, LeftTime, State[10], Graphics.
- NpcCommand: CmdKind, X, Y, Z.
- NpcSkill (0x30): Id, Level, + 10 int.
- PlayerItem (0x14): nIdx, nPlace, X, Y, Z.
- KPlayerTeam: m_nFlag, m_nFigure.

---

## 7. Class UI & con trỏ m_pSelf

| Class | m_pSelf | Trường quan trọng |
|---|---|---|
| KWndImage (base) | — | +0 handle, **+4 m_Style** (WND_S_VISIBLE = cửa sổ đang mở) |
| KWndText512 | — | buffer chữ +0x36C |
| KWndButton | — | cờ checked `0x200` tại +0x34C |
| ObjectBox | — | uGenre +0x2B0 |
| KUiPlayerBar | `0x7BE494` | HK3Genre +0x2B74; GSHOP button +0x1378 |
| KUiInit | `0x7C47D8` | nút: ENTERGAME +0x39C, +0xE64, +0x23F4, EXITGAME +0x192C |
| KUiMsgSel | `0x7CBD2C` | pMessages +0x8A8, nNum +0x8AC, InfoText +0x154C |
| KUiMsgSel2 | `0x7CB470` | pMessages +0x1178, nNum +0x117C, InfoMsgs +0x1D44, nNumInfo +0x1D4C, szQuestion[0x2000] +0x2578 |
| KOneMsgInfo (dòng chọn) | — | nLen +0x18, szMsg +0x1C |
| KUiShop | `0x7C9D3C` | pObjectList +0x37C (KItemObject[]), ObjCount +0x39C, PageCount +0x3A0, CurPage +0x3A4 |
| KUiInformation / 2 | `0x7C5DD8` / `0x7C6CC8` (tĩnh) | szInfoText +0x6E8 |
| KUiSysMsgCentre | `0x7CA1FC` | MsgHeap[8] {pList, space, numValid} ở +0x294 |
| KSystemMessage | — | szMessage +0, tên người mời +0x408 |
| KUiPrizeSel | `0x7CBD1C` | PrzItem[3] ở +0x8E8, mỗi cái 0xFB4; icon +0x2B4, text +0x358 |
| KUiYeSouBox | `0x7CAD4C` | Items +0x34C, Assemble +0x2CC4, Close +0x3228 |
| KUiPlayerStall | `0x7CAD54` | GetObjects: `GGD(0xBBF,0,0)` → số món, `GGD(0xBBF,buf,n)` → điền buffer |
| KUiPlayerItem | — | Name[64], uId, nIndex, nData (tham số cho ProcessPeople) |

Luồng hộp thoại NPC: server gửi → `CoreDataChanged (0x409C20)` id 64 → `KUiMsgSel2::OpenWindow (0x5210C0)`.
Đọc lựa chọn ở `pMessages[i].szMsg`, chọn bằng `OMS2(i)`.

---

## 8. Màn hình đăng nhập (GameLogin.h)

| Đối tượng | Địa chỉ / offset |
|---|---|
| **KUiSelServer** m_pSelf | `0x7BE604` |
| Login fn | `0x487D00` |
| Regions +0x6FC, Servers +0x700 | mỗi dòng 0x24: Title[32] + IP ở +0x20 |
| Region hiện tại | +0x2080 |
| LeftList / RightList / IpList | +0x704 / +0xAB0 / +0xE5C (left list 17 cụm) |
| Update / AutoUpdate flag | `0x487160` / +0x226C |
| **KWndList** | count +0x29C, cursel +0x2B0; SetCurSel `0x44C4F0`, ClearSel `0x480DD0`, Focus `0x480C90` |
| **KUiLogin** m_pSelf | `0x7BE5EC` |
| Ô tài khoản / mật khẩu | +0x380 / +0x67C |
| OnLogin | `0x484C20` |
| EditSetText / BtnCheck | `0x4488F0` / `0x442DD0` |
| **KUiSelPlayer** m_pSelf | `0x7C48AC` |
| Select / Enter | `0x49FEE0` / `0x4A0370` |
| Checkbox đồng ý | +0x8464 |
| Num / Sel / Page | +0x7E1C / +0x7E1E / +0x7E28 (3 nhân vật/trang) |
| **g_LoginLogic** | `0x848A68`: Status +8 (6 = đã nhận danh sách nhân vật), NumRole +0x12, Roles +0x14 (0x28/nhân vật: Gender +0x20, Attr +0x21, Level +0x24); tài khoản +0x780 (đảo bit) |
| **KUiHealthNotice** (thông báo sức khoẻ) | self `0x7BE5E8`, nút OK +0x37C, WndProc `0x483B10` |
| **KUiConnectInfo** | self `0x7BE5E4`, Desire +0x2814, Message +0x2818 (0x180 byte), MsgLen +0x2998; mã CI_NS bắt đầu từ 1000 |

---

## 9. Cảnh, vẽ, cấu hình thấp

**Scene (SCP 0x172B1F8)**: 9 region bắt đầu tại +0x31E4C.
Trường region: status +4 (3 = standby), lefttop +8, obstacle +0x48 `[16][32]` (bit 0-3 kind, bit 4-7 type), trap +0x848 `[16][32]`.
Hàm: LoadObstacle `0x6F8090`, LoadTrap `0x6F80D0`, GetTrap `0x6F86B0`.

**Vẽ (g_pRepresent `0xEDFC88`)**: vtable +0x10 OutputText (9 tham số), +0x48 DrawPrimitives (4 tham số).
Hook vẽ thêm: thay call E8 tại `0x4D78CD` (DrawGameSpace `0x5BD120`).

**Cấu hình thấp (giảm tải CPU)**:
| Địa chỉ | Ý nghĩa |
|---|---|
| `0x7AD7B0` | FPS vẽ (mặc định 30) |
| `0x8483B0` | Cờ inactive (KMyApp+0x50) |
| `0x7AE608` | Số đối tượng hiển thị |
| SCP+0x06 | Thời tiết bật/tắt |
| SCP+0x3261A | Ánh sáng động |
| KOption `0x17969C0` | +0 âm thanh, +4 nhạc |

Hoặc dùng `OPR(GOI_OPTION_SETTING 0x2B, OPTION_xxx, value)`.

---

## 10. Enum & mã thao tác

### 10.1 GOI — OperationRequest
| Mã | Tên | uParam / nParam |
|---|---|---|
| 1 | GOI_EXIT_GAME | |
| 2 | GOI_SWITCH_OBJECT_QUERY | |
| 3 | GOI_SWITCH_OBJECT | &KItemObject, 0 — nhấc/thả đồ |
| 4 | GOI_REJECT_OBJECT | |
| 5 | GOI_MONEY_INOUT_STORE_BOX | bIn (1 gửi, 0 rút), số tiền — không cần mở rương |
| 6 | GOI_PLAYER_ACTION | |
| 7 | GOI_PLAYER_RENASCENCE | Hồi sinh |
| 8 | GOI_INFORMATION_CONFIRM_NOTIFY | |
| 9 | GOI_QUESTION_CHOOSE | 0, index lựa chọn |
| 0xA | GOI_USE_ITEM | &KItemObject, UOC_ITEM_TAKE_WITH |
| 0xB | GOI_WEAR_EQUIP | |
| 0xC | GOI_USE_SKILL | |
| 0xD | GOI_SET_IMMDIA_SKILL | &KItemObject(skill), 0 = trái / 1 = phải |
| 0x18 | GOI_TRADE_NPC_BUY | &KItemObject, 0 |
| 0x19 | GOI_TRADE_NPC_CHECK | |
| 0x1A | GOI_TRADE_NPC_SELL | &KItemObject, 0 |
| 0x1B | GOI_TRADE_NPC_REPAIR | &KItemObject, 0 |
| 0x2B | GOI_OPTION_SETTING | OPTION_*, value |
| 0x45/0x46/0x47/0x48 | Trade player SetPrice / GetPrice / Sell / Buy | |
| 0x59 | GOI_SHIFT_ITEM | |
| 0x81 | GOI_MAP_GOTO | (15_10; bản cũ khác) |
| 0x3B, 0x5D (yesou), 0x2E (ProcessPeople case 10) | khác | |
| — | GOI_DECOMPOSE | **Đã bỏ** trong 15_10 (bản cũ 0x9B) |

Mã 1..0x68 giống 10_10.

### 10.2 GDI — GetGameData
| Mã | Tên |
|---|---|
| 1 | GDI_GAME_OBJ_DESC (mô tả vật phẩm) |
| 2 | GDI_GAME_OBJ_DESC_INCLUDE_TRADE |
| 3 | GDI_GAME_OBJ_DESC_INCLUDE_REPAIR |
| 4 | GDI_GAME_OBJ_LIGHT_PROP |
| 5 | GDI_PLAYER_BASE_INFO |
| 0xBBA | GDI_TRADE_ITEM_PRICE (`GGD(0xBBA, &obj, szBuf)`) |
| 0xBBF | GDI_STALL_ITEMS |
| 0xBC0 | GDI_STALL_OWNER |

### 10.3 OPTION
0 perspective, 1 dynalight, 2 music_value, 3 sound_value, 4 brightness, 5 weather, 6 visible_num, 7 missle.

### 10.4 TEAM (TeamOperation)
5 CREATE, 6 APPOINT (chuyển đội trưởng), 7 INVITE, 8 KICK, 9 LEAVE, 0xC JOIN_RESPONSE, 0xD INVITE_RESPONSE.

### 10.5 ACTION (ProcessPeople)
0 chat, 1 makefriend, 2 trade, 3 jointeam, 4 inviteteam, 5 follow, 6 viewitem, 7 tong, 9 blacklist, 10 revenge.

### 10.6 NPCCMD (m_Doing / m_Command.CmdKind)
0 none, 1 stand, 2 walk, 3 run, 4 jump, 5 skill, 6 magic, 7 attack, 8 sit, 9 hurt, 10 death, 11 defense, 12 idle, … 21 revive.

### 10.7 Quan hệ / phe / ngũ hành
- Relation: 1 none, 2 self, 4 ally, 8 enemy, 16 dialog.
- Camp: 0 begin (tân thủ), 1 justice (chính), 2 evil (tà), 3 balance (trung), 4 free (sát thủ), 5 animal (quái), 6 event.
- Series: 0 kim, 1 mộc, 2 thuỷ, 3 hoả, 4 thổ.

### 10.8 Hành trang
**INVENTORY_ROOM** (index m_Room): 0 equipment (hành trang), 1 repository (rương), 2 trade, 3 tradeback, 4 trade1, 5 immediacy (ô phím tắt), 6 room06, 7 repo02, 8 repo03, 9 room09, 10 yesou, 11 0a, 12 0b, 13 0c, 14 guid.

**ITEM_POSITION** (PlayerItem.nPlace): 1 hand, 2 equip, 3 equiproom, 4 repositoryroom, 5 traderoom, 6 trade1, 7 immediacy, 8 expand, 9 repo02, 10 repo03, 12 yesoubox, 23 guid.

**UOC (container UI)**: 1 IN_HAND, 2 GAMESPACE, 3 IMMEDIA_ITEM, 4 IMMEDIA_SKILL, 5 ITEM_TAKE_WITH (hành trang), 6 GUID_BOX, 7 TRADE_BOX, 9 EQUIPTMENT, 10 NPC_SHOP, 11/12/13 STORE_BOX (rương 1/2/3), 20 YESOU_BOX, 21-23 HTINH.

**CGOG (loại đối tượng UI)**: 0 nothing, 1 money, 2 player_face, 3 object, 4 skill, 5 item, 6 npc, 7 menu_npc, 8 npc_blur, 9 missle, 10 equip, 11 npcsellitem.

### 10.9 Vật phẩm
- **ITEMGENRE**: 0 equip, 1 medicine, 2 mine, 3 materials, 4 task, 5 townportal, 6 special.
- **EQUIPDETAIL**: 0 melee, 1 range, 2 armor, 3 ring, 4 amulet, 5 boots, 6 belt, 7 helm, 8 cuff, 9 pendant, 10 horse, 11 mask.
- **MEDICINE** (nDetailType khi genre=1): 0 máu, 1 mana, 2 cả hai, 3 thể lực, 4 giải độc, 8 cả hai (ex).
- item_special nParticularType `0xD7..0xD9`: thuốc đặc biệt cũng ăn được.
- **ITEM_PART** (m_EquipItem): 0 head, 1 body, 2 belt, 3 weapon, 4 foot, 5 cuff, 6 amulet, 7 ring1, 8 ring2, 9 pendant, 10 horse, 11 mask, 12 pp, 13 an, 14 ts, 15 ts1, 16 ts2.
- Tên vật phẩm hay so (TCVN3 trong code): Thổ địa phù, Hồi thành phù, Khiêu chiến lệnh, Phúc Duyên Lộ, "Những nơi đã đi qua", "Giao dịch", "Sửa kỹ [".

### 10.10 MAGIC_ATTRIB (thuộc tính KMagicAttrib.nAttribType)
Dải item: 27 (item_begin) … 507 (normal_end). Hay dùng: 85 lifemax_v, 89 manamax_v, 111 fastwalkrun_p, 114 allres_p, 115 attackspeed, 116 castspeed.
15_10 đổi các ID 294-302 và thêm mới 501-506. Tên: `*(char**)(0xEE0278 + 4*ID)`.

### 10.11 Map ID thành thị
| Thành | ID | Thôn/trấn | ID |
|---|---|---|---|
| Phượng Tường | 1 | Long Môn | 121 |
| Thành Đô | 11 | Vĩnh Lạc | 99 |
| Biện Kinh | 37 | Ba Lăng | 53 |
| Tương Dương | 78 | Chu Tiên | 100 |
| Dương Châu | 80 | Giang Tân | 20 |
| Đại Lý | 162 | Thạch Cổ | 153 |
| Lâm An | 176 | Đạo Hương | 101 |
| | | Long Tuyền | 174 |
Khác: Phong Kỳ 387+, Vi Sơn đảo 342, bản đồ bang 586-604.

### 10.12 Task ID (GetTaskVal)
**Dã Tẩu (ys)**: taskgid 1020, majorid 1021, maphave 1025, repvalue 1026, shxtmap 1027, taskstatus 1028, task405 1029, minorid 1030, mapid 1031, smapreq 1032, prelevel 1033, preexp 1034, tasknum 1044, taskccl 1046, preexp_ex 2574 (kinh nghiệm thật = preexp + preexp_ex×100000).
**Phúc duyên/PK**: pck_marks 1207, pck_time 0x4C6.
**Bang hội (bh)**: lsttask 0x515, pk 0x6D0..0x6DC, curtask 0x899, tasktime 0xA06.

### 10.13 Giao thức mạng
- c2s: npcwalk = 75, rồi run 76, skill 77, jump, talk, hurt, death (liên tiếp).
- **Gói skill 15_10** (dựng tại `0x5E8280`): byte [0] opcode, [1] = **y**, [5] = **skillID**, [9] = **x**, [0xD] = token. (Thứ tự khác 10_10 — đừng tự dựng gói, hãy gọi `SKL_FUNC`.)

---

## 11. Công thức chức năng (cách tool đang làm)

### 11.1 Dùng vật phẩm trong hành trang
```cpp
KInventory& r = PLAYER_ROOM[room_equipment];
for (int j = 0; j < r.m_nHeight; j++)
  for (int i = 0; i < r.m_nWidth; i++) {
    int idx = r.m_pArray[j * r.m_nWidth + i];
    if (idx <= 0) continue;
    KItem* it = g_pItemArray[idx];
    if (strcmp(it->Name, "...") == 0) {           // TCVN3
        KItemObject o = {0};
        o.uGenre = CGOG_ITEM;  // 5
        o.uId = idx; o.DataX = i; o.DataY = j;
        g_pCoreShell->OperationRequest(GOI_USE_ITEM /*0xA*/, (int)&o, UOC_ITEM_TAKE_WITH /*5*/);
        return;
    }
  }
```

### 11.2 Ăn thuốc
Như 11.1, lọc `genre == item_medicine(1)` và `nDetailType` = 0 (máu) / 1 (mana); khi cần cả hai thì chấp nhận 2 hoặc 8; thêm `item_special` với particular 0xD7..0xD9.
Điều kiện tool đang dùng: chỉ ăn khi `HP < ngưỡng cấu hình` **và** `HP < 70% HPmax` (tương tự MP). HPmax = `max(m_CurrentLifeMax, m_CurrentLifeMaxEx)`.

### 11.3 Mua / bán / sửa
```cpp
// Mua: shop đang mở (KUiShop), obj = pShop->m_pObjectList[i]
KItemObject o = { CGOG_NPCSELLITEM /*11*/, uId, ..., UOC_NPC_SHOP /*10*/ };
OPR(GOI_TRADE_NPC_BUY /*0x18*/, &o, 0);
// Giá: GGD(GDI_TRADE_ITEM_PRICE 0xBBA, &o, szDesc)

// Bán: item trong hành trang
o = { CGOG_ITEM, idx, x, y, w, h, UOC_ITEM_TAKE_WITH };  OPR(0x1A, &o, 0);

// Sửa: item đang mặc
o = { CGOG_ITEM, idx, ..., UOC_EQUIPTMENT /*9*/ };       OPR(0x1B, &o, 0);
// Giá sửa: KItem::GetRepairPrice (IGR)
```

### 11.4 Di chuyển / ngồi / tìm đường
```cpp
// Di chuyển: chỉ khi m_nSendMoveFrames > 7, kẹp khoảng cách ≤ 400
P_PLAYER->MoveTo(x, y, 0);            // MVT thiscall
g_pCurPlayer->m_nSendMoveFrames = 0;

// Ngồi: khi m_nSendMoveFrames >= 7
ExecuteScript("Switch([[sit]])");     // EXS
// Lên/xuống ngựa
ExecuteScript("Switch([[horse]])");

// Tự tìm đường (pathing của game)
__asm { mov ecx, 0x1796838; push y32; push x32; mov eax, 0x642730; call eax }  // x32 = x/32
```

### 11.5 Đặt cờ minimap
`absX = playerX + (dx << 4); absY = playerY + (dy << 5); SMF(absX, absY, 1);` → đọc lại `0x7AAA58/5C`.

### 11.6 Dùng kỹ năng / đánh
```cpp
// Lên mục tiêu NPC (kể cả chính mình để buff): x = -1, y = m_dwID
SendClientCmdSkill(skillId, -1, pNpc->m_dwID);   // SKL_FUNC 0x644CB0 cdecl
// Lên điểm (skill tầm xa / khinh công): x, y là toạ độ TƯƠNG ĐỐI MÀN HÌNH, không phải toạ độ game:
//   x = (enemyX - playerX) + 400;  y = (enemyY - playerY) / 2 + 280
SendClientCmdSkill(skillId, x, y);
// Nếu ở ngoài tầm (khoảng cách > AttackRadius), tool MoveTo tới điểm cách địch đúng AttackRadius trước.
// Đặt skill tay trái/phải:
KItemObject s = { CGOG_SKILL /*4*/, skillId };
OPR(GOI_SET_IMMDIA_SKILL /*0xD*/, &s, 0 /*trái*/ hoặc 1 /*phải*/);
// Kiểm tra địch: GetNPCRelation(i) == relation_enemy (8)
```

### 11.7 Nói chuyện NPC & chọn menu
```cpp
g_pCurPlayer->DialogNpc(npcIdx);                // DNPC 0x5E9880
// chờ KUiMsgSel2 (0x7CB470) mở (m_Style & WND_S_VISIBLE)
// duyệt pMessages[0..nNum) -> szMsg (TCVN3, có mã màu)
pMsgSel2->OnClickMsg(i);                        // OMS2 0x522120 (hoặc OMS 0x542110 cho KUiMsgSel)
// Chọn thưởng: OPR(GOI_QUESTION_CHOOSE 9, 0, n); PSC(0);
// Đóng thông báo: IH1(0) / IH2()
```

### 11.8 Nhặt đồ
Duyệt `g_pObjArray[1..255]`, lọc `m_nKind == Item(3) || Money(4)`, khoảng cách < 128 → `g_pCurPlayer->PickUpObj(i)` (PKO).

### 11.9 Tin hệ thống / chat
- Hiện tin: `SMA(msg, strlen(msg)+1, channel)`.
- Chat gần: `ExecuteScript("Chat('CH_NEARBY','<text>')")`.
- Lời mời (tổ đội/giao dịch) nằm trong `KUiSysMsgCentre.MsgHeap[k]`; tên người mời ở `KSystemMessage+0x408`; xoá bằng SMD.

### 11.10 Tổ đội / bang
- Tạo đội `TOP(5,0,0)`, rời `TOP(9,0,0)`; xin vào đội người khác: `ProcessPeople(&KUiPlayerItem, ACTION_JOINTEAM 3)`.
- Vào bản đồ bang: `TongOperation(0x23, tongID, 0)`.
- Nộp tiền quỹ bang: `int p[13] = {2, 0, money}; TongOperation(4, (int)p, (int)buf /*0x138 byte*/)`.

### 11.11 Rương & tiền
- Gửi/rút tiền: `OPR(GOI_MONEY_INOUT_STORE_BOX 5, bIn, amount)` (không cần mở rương).
- Khoá rương: `PGS(1)` = có tính năng?, `PGS(0)` = đang khoá?, `PGS(6, pass)` mở khoá, `PGS(5)` khoá lại.
- Mở rương UI: `SBO_FUNC()`.

### 11.12 Điểm tiềm năng & kinh nghiệm
- Cộng điểm: `g_pCurPlayer->ApplyAddBaseAttribute(attr, n)` (PAA).
- % kinh nghiệm: `m_nCurExp / ((int*)LEVEL_EXP_TB)[level]`.

### 11.13 Click nút UI giả lập
```cpp
// Mở Kỳ Trân Các (GShop) qua WndProc thanh PlayerBar
__asm { push 0; push (UPB+0x1378); push 0x565 /*WND_N_BUTTON_CLICK*/;
        mov ecx, UPB; mov eax, 0x46FCA0; call eax }
```

---

## 12. Định dạng file Pak
- Header: magic `"PACK"`, số file, vị trí bảng chỉ mục, vị trí dữ liệu.
- Mỗi entry chỉ mục 16 byte: id file, offset, kích thước gốc, `(method << 24) | kích thước nén`.
- Method 1 = UCL nrv2b (bit buffer 8 bit). `slistcache.pak` dùng method 0x20.
- Id file = `HashString2ID(đường dẫn chữ thường)` (xem §3.4).

---

## 13. Lưu ý & điểm chưa chắc

1. **Chưa kiểm chứng in-game cho bản 15_10.** Toàn bộ offset được dò tĩnh; nên viết công cụ debug đọc/so
   giá trị (HP, toạ độ, tên NPC) trước khi dùng các hàm có ghi/gửi gói.
2. ⚠ `KNpc::m_dwTongNameID (0x1CCC)`, các trường giữa của KPlayerTong, `GOI 0x3B/0x5D/0x2E` chưa xác nhận.
3. TMC, CMP, ITX, OSM **không tồn tại** trong 15_10.
4. Thứ tự byte của gói skill đã đổi so với 10_10 — luôn gọi qua `SKL_FUNC`.
5. Gọi hàm game nên thực hiện **trên thread game** (hook GameLoop `0x5A9160` hoặc DrawGameSpace) để
   tránh race với logic game.
6. Nếu client cập nhật bản mới: dùng các anchor (chuỗi, vtable `0x751A84`, `0x7600B0`) và diff mã máy
   như cách đã làm từ 10_10 → 15_10 để dò lại.
7. Các enum nội bộ của tool (SENDCMD, DEBUG_CMD, WM_COPYDATA id `'VDBG' 0x56444247`) không thuộc game.
