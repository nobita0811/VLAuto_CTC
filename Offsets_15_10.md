# Offset bản client 15_10 (gamethienha_y_15_10.exe)

Dò ngày 2026-10-15 bằng cách đối chiếu mã máy của bản mới với bản 10_10 (`I:\JX20\PhienBanGame\10_10\gamecl_dump_SCY_10_10.exe`).
Cả hai đều là bản dump, ImageBase 0x400000, section UPX0 không nén.

Cách làm:
- Dịch ngược tuyến tính cả hai bản (capstone), chuẩn hóa toán hạng địa chỉ, rồi ghép hàm theo 3 nguồn: chữ ký byte (bỏ qua địa chỉ/hằng), chuỗi tham chiếu duy nhất và lan truyền qua đồ thị lời gọi. Tổng cộng 7.362 cặp hàm.
- Biến toàn cục: bỏ phiếu từ các cặp hàm khớp tốt.
- Offset struct: căn hàng lệnh giữa 2 hàm tương ứng, đọc cặp displacement cũ → mới. Sau đó **xác minh lại bằng "neo"**, tức các hàm đã ghi trong `Offsets_10_10.md`.
- Phần UI của bản mới được biên dịch khác hẳn (có frame pointer `push ebp; mov ebp,esp`), nên dò bằng tên control trong LoadScheme (`push "Select"; add ecx, X`), vtable và hàm OpenWindow (`new` → ghi `m_pSelf`).

Đã ghi vào `VLHook/GameBase.h`, `VLHook/JxCore.h`, `VLHook/S3Client.h`, `VLAuto/GameCore.h`, `VLAuto/GameLogin.h` và `GameDef.h`
(đánh dấu `15-10`). Bố cục đã xác minh đều có `static_assert`, build kiểm tra lúc biên dịch.

## 0. Thay đổi lớn so với 10_10 (cần biết khi sửa code)

| Mục | 10_10 | 15_10 |
|---|---|---|
| sizeof KNpc / số NPC | 0x12BD4 / 800 | **0x11AB8 / 256** (`new KNpc[0x100]`) |
| sizeof KItem / số item | 0x74C / 512 | **0x864 / 1024** (OPR kiểm `idx-1 <= 0x3FE`) |
| sizeof KObj | 0x3F4 | **0x4E8** (256 phần tử) |
| sizeof KSkill (m_SkillInfo) | 0x4FC | **0x4F8** |
| sizeof KPlayer | 0xA878 | 0xA878 (không đổi, nhưng bố cục bên trong đổi hết) |
| KItemList.m_EquipItem | `int[17]` ở +0x28 | **`{nIdx, nData}[16]`** ở +0x28 → code dùng `GetEquip(i)`, `MAX_EQUIP_SLOT` |
| Số ô hành trang (m_Items) | 483 | **678** |
| Rã trang bị (GOI 0x9B) | có | **không còn** (không có chuỗi "DecomposeEquip") → `GOI_DECOMPOSE_EQUIP = 0`, `DecomposeItem` trả FALSE |
| OnSendSomeoneMessage | 0x4768D0 | **không có hàm tương đương** → `OSM_FUNC_ADD = 0`, `SendPrivateMessage` bỏ qua |
| PaintWindow → DrawGameSpace | `jmp` (E9) | **`call`** (E8) ở 0x4D78CD → `PatchDraw` nhận cả E8 |
| PaintFps | lưu ms/khung = 1000/fps | **lưu thẳng FPS** (0x7AD7B0, mặc định 30) |
| Gói SendClientCmdSkill | `[0]=0x4D,[1]=myID,[5]=y,[9]=skill,[0xD]=x` | `[1]=y,[5]=skill,[9]=x,[0xD]=token` (0x5E8280). Hàm vẫn nhận `(skill, x, y)` |
| Đăng nhập | ô "đồng ý" ở cửa sổ Login (+0x80C0) | **ô đồng ý ở cửa sổ chọn nhân vật** (`cbUserAgreement` +0x8464); Login không còn |
| KLogin.m_Status khi có danh sách nhân vật | 7 | **6** |
| KRoleChiefInfo | 0x24 byte, short Level +0x22 | **0x28 byte, int Level +0x24** |
| LeftList khu vực máy chủ | 14 | **17** |
| ScenePlace: số region đang nạp | 15 | **9** |
| vtable iRepresentShell | OutputText 0x14, DrawPrimitives 0x4C | **0x10 / 0x48** (bớt 1 hàm phía trước) |
| MAGIC_ATTRIB | normal_end = 308 | **normal_end = 507**: ID 0..293 như cũ, 294 bỏ, 299..302 đổi tên, 303..500 trống, 501..506 mới |
| OPR (GOI_*) | switch 1..0x9D | switch 1..0xA9. **1..0x68 giữ nguyên** (gồm mọi mã hook đang dùng); từ ~0x6D lùi 4 (vd đi tới ô bản đồ 0x85 → **0x81**) |

## 1. Biến toàn cục

| Macro | 15_10 | 10_10 | Neo / bằng chứng |
|---|---|---|---|
| CORE_BASE_ADD | 0x00848314 | 0x009BC130 | `mov ecx,[0x848314]` trước 270 lời gọi GetGameData |
| PLR_BASE_ADD | 0x00EE403C | 0x009C0474 | 183 phiếu; `[PLR]+0xA9B4` = Player[1].m_nIndex |
| NPC_BASE_ADD | 0x017220F0 | 0x01A3B4B4 | `new KNpc[0x100]` (size 0x11AB8) |
| OBJ_BASE_ADD | 0x017286B4 | 0x021A1898 | `new KObj[0x100]` (0x4E8) |
| ITM_BASE_ADD | 0x01722148 | 0x01F18E14 | `new KItem[0x400]` (0x864) |
| WLD_BASE_ADD | 0x017286B0 | 0x009C08A8 | `[[WLD]+0x50]` = m_dwCurrentTime |
| NSET_BASE_ADD | 0x017286C0 | 0x021A18A8 | this của NpcSet.SearchID trong SyncPlayer |
| SKL_BASE_ADD | 0x0084ACB8 | 0x01AB4A10 | 0x5D23B0: `cmp id-1, 0xBB7; imul id, 0x4F8; [ecx+id*0x4F8-0x4F8]` |
| BSL_BASE_ADD | 0x01A6A620 | 0x024E7750 | `mov ecx,0x1A6A620; call GetItemIndex/GetItem` |
| TEAM_ADDRESS | 0x00EE4850 | 0x01F18A80 | g_Team 0xEE4848; layout KTeam không đổi (+0x28, +0x30, +0x50, +0x150 đều khớp phiếu) |
| MAP_ID_ADD / MAP_NAME_ADD | 0x0175D6E8 / 0x0175D6C8 | 0x021D6840 / 0x021D6820 | cùng ngữ cảnh 3 xref / strncpy 0x20 |
| ACNT_ID_ADD | 0x008491E8 | 0x009BD2D8 | g_LoginLogic 0x848A68 + 0x780 (0x5AD430 sao chép rồi `not` từng byte) |
| LEVEL_EXP_TB | 0x01A7D518 | 0x024F1E4C | PlayerSet 0x1A7D4E0 + 0x38 (nạp `\settings\npc\player\level_exp.txt` 0x6D2C10); **int[999]** (cột 2 ở +0xF9C) |
| FLAG_X/Y_OFFST | 0x007AAA58 / 0x007AAA5C | 0x00811D24 / 28 | ghi trong SetMapFlag 0x4DE840 (x>>4, y>>5) |
| GOTO_BASE_ADD | 0x01796838 | 0x01F18DA0 | `mov ecx,0x1796838; call 0x642730` |
| RPS_BASE_ADD (g_pRepresent) | 0x00EDFC88 | 0x01F182F0 | DrawGameSpace `cmp [0xEDFC88],0` |
| SCP_BASE_ADD (g_ScenePlace) | 0x0172B1F8 | 0x021A3F18 | DrawGameSpace `mov ecx,0x172B1F8; call Paint` |
| NetClient | 0x0084A9EC | 0x009BE8D4 | gửi gói `[ecx]+0x10` |
| Bảng tên MAGIC_ATTRIB | 0x00EE0278 + 4*ID | 0x01F18300 | `mov [0xEE0278+4*ID], "name"` |

## 2. Hàm

| Macro | 15_10 | 10_10 | Kiểu | Neo |
|---|---|---|---|---|
| GGD_FUNC_ADD | 0x005BC2D0 | 0x005B9F30 | thiscall(3) | giống hệt từng byte (sim 1.00) |
| OPR_FUNC_ADD | 0x005C1180 | 0x005C3820 | thiscall(3) | 435 lời gọi qua [CORE]; switch `id-1 <= 0xA8`, bảng 0x5C4B00 |
| TNP_FUNC_ADD | 0x005C5300 | 0x005C2050 | thiscall(3) | 56 lời gọi qua [CORE] |
| TOP_FUNC_ADD | 0x005BD440 | 0x005BB090 | thiscall(3) | sim 0.97, cùng các hàm team của KPlayer |
| PGS_FUNC_ADD | 0x005BD780 | 0x005BB3C0 | thiscall(3) | switch 0..7 |
| GRL_FUNC_ADD | 0x005BD0C0 | 0x005BAAF0 | thiscall(1) | khớp chữ ký byte, `ret 4` |
| DGS_FUNC_ADD | 0x005BD120 | 0x005BAB50 | thiscall(0) | `cmp [g_pRepresent]; ScenePlace.Paint; DrawSelectInfo` |
| DGS_JMP_ADD | 0x004D78CD | 0x004C3AC6 | – | PaintWindow: `mov ecx,[0x848314]; call 0x5BD120` (**E8**) |
| MVT_FUNC_ADD | 0x005F8CF0 | 0x005F3B30 | thiscall(3) | KNpc::MoveTo, sim 1.00 |
| SKL_FUNC_ADD | 0x00644CB0 | 0x005ECED0 | cdecl(3) | gói 0x4D dài 0x11; lời gọi duy nhất `(skill, -1, npc->m_dwID)` |
| PKO_FUNC_ADD | 0x005EC7E0 | 0x005E2530 | thiscall(1) | KPlayer::PickUpObj, sim 1.00 |
| DNPC_FUNC_ADD | 0x005E9880 | 0x005E4C80 | thiscall(1) | kiểm `npc.m_Index > 0` rồi gửi gói 0x6E (0x5E9820) |
| PAA_FUNC_ADD | 0x005E8E70 | 0x005DEA20 | thiscall(2) | attr ≤ 3, gói 0x57 |
| GTV_BASE_ADD | 0x00609990 | 0x0061C820 | thiscall(1) | KPlayerTask::GetSaveVal, sim 1.00 |
| IGR_FUNC_ADD | 0x006051B0 | 0x0063A500 | thiscall(0) | KItem::GetRepairPrice (Dur +0x6A8/+0x6AC) |
| BGI / BGX_FUNC_ADD | 0x006852B0 / 0x00685300 | 0x006B9390 / 0x006B93E0 | thiscall(1)/(2) | KBuySell, sim 1.00 |
| GOTO_FUNC_ADD | 0x00642730 | 0x00630FE0 | thiscall(2) | sim 1.00 |
| EXS_FUNC_ADD | 0x00423B40 | 0x004360B0 | cdecl(1) | 5 lời gọi tương ứng |
| CLA_FUNC_ADD | 0x00432B10 | 0x0044C560 | cdecl(1) | chuỗi ~20 lời gọi GetIfVisible giống hệt |
| SMA_FUNC_ADD | 0x004BD680 | 0x004B9340 | cdecl(3) | đọc m_pSelf MCP 0x7C9BE8 |
| SBO_FUNC_ADD | 0x004991B0 | 0x00498F80 | cdecl(0) | KUiStoreBox::OpenWindow: `new 0x24F0` → [0x7C488C] |
| SHO_FUNC_ADD | 0x004CB1F0 | 0x004BF2F0 | cdecl(0) | KUiShop::OpenWindow: `new 0x26F0` → [0x7C9D3C] |
| OMS_FUNC_ADD | 0x00542110 | 0x0051EB50 | thiscall(1) | KUiMsgSel::OnClickMsg: list this+0x610, OPR(9,0,n) |
| OMS2_FUNC_ADD | 0x00522120 | 0x00509E40 | thiscall(1) | OPR(9,0,n) + CloseWindow(1) 0x521200 |
| UWP_FUNC_ADD | 0x0046FCA0 | 0x00478440 | thiscall(3) | KUiPlayerBar vtable 0x751A84[4] |
| IOC_FUNC_ADD | 0x00490EA0 | 0x0048FFB0 | thiscall(1) | so tham số với this+0x39C / 0xE64 / 0x192C / 0x23F4 |
| IH1_FUNC_ADD | 0x004B7440 | 0x004AE490 | thiscall(1) | KUiInformation::Hide(n): m_pCallerWnd +0xFF0 → WndProc(0x501) |
| IH2_FUNC_ADD | 0x004B80E0 | 0x004AE940 | thiscall(0) | caller(+0xBE0) == -1 → OPR(8,0,0) |
| PCP_FUNC_ADD | 0x004D7A40 | 0x004C4530 | cdecl(2) | switch 0..0x11; case 3 JOINTEAM, case 10 OPR(0x2E) |
| SMD_FUNC_ADD | 0x004DA540 | 0x004C5390 | thiscall(2) | this + heap*12 + 0x294 |
| SMF_FUNC_ADD | 0x004DE840 | 0x004C7C70 | cdecl(3) | (x, y, bWorld); OPR(**0x81**, x/32, y/32) |
| OSM_FUNC_ADD | 0 | 0x004768D0 | – | không tìm thấy hàm nhắn riêng tương đương |
| Phụ | | | | SendCommand 0x5F7490, SetFightMode 0x5FEA90, NpcSet::SearchID 0x6368D0, SetLeftSkill 0x5E8FD0, SendClientCmdWalk/Run 0x645060/0x644FD0, KNpc ctor 0x5FF860, KNpc::Init 0x5F94D0, ScenePaint 0x63C2E0, ObjSet::AddData/ClientAdd 0x6774E0/0x677F90, Skill load 0x5C99F0, GameLoop 0x5A9160, SelectRole 0x5AAE40 |

## 3. KNpc (size 0x11AB8)

Phần đầu bị bỏ con trỏ buffer VN (8 byte), nên từ m_dwID đến m_cGold dịch −8 rồi −0x10. Khối giữa dịch −0x14 / −0x4C / −0x50.
Riêng m_FightMode nằm **trước** m_SyncSignal.

| Field | 15_10 | 10_10 | Neo |
|---|---|---|---|
| m_dwID / m_Index / m_Node | 0x00 / 0x04 / 0x08 | 0x08 / 0x0C / 0x10 | ctor: vtable KNode ở +8 |
| m_Level / m_Kind / m_Series / m_Height | 0x20 / 0x24 / 0x28 / 0x2C | 0x28.. | SyncNpc 0x622D90, SyncPlayer 0x623E00 |
| m_btRankId / m_nStature | 0x30 / 0x34 | 0x38 / 0x3C | KNpc::Init |
| m_cGold (KNpcGold) | 0x3C | 0x4C | `lea ecx,[npc+0x3C]` → KNpcGold::IsGold 0x6D4600 |
| m_PoisonState | 0x84 | 0x94 | suy ra (7 KState 0x74..0xE3) |
| m_Camp / m_CurrentCamp / m_Doing | 0xE4 / 0xE8 / 0xEC | 0xF4.. | Init; OPR case 6 `cmp [npc+0xEC],8` |
| KList head/tail/count | 0xFC / 0x100 / 0x104 | 0x110.. | ctor |
| m_ActiveSkillID / KSkillList / m_Skills | 0x108 / 0x110 / 0x148 | 0x11C / 0x124 / 0x15C | SetLeftSkill `lea [npc+0x110]`; 0x148 + 80×0x30 = 0x1048 |
| m_CurrentLife / Mana / Stamina / StaminaMax | 0x1048 / 0x105C / 0x1064 / 0x1068 | 0x105C.. | 19 phiếu |
| Walk / Run speed | 0x1144 / 0x1148 | 0x1150 / 0x1154 | Init (=5 / =10), SyncPlayer |
| AttackRadius / ActiveRadius | 0x1164 / 0x1168 | 0x1170 / 0x1174 | Init (=30) |
| m_Dir / m_MapX,Y,Z / m_OffX,Y | 0x1344 / 0x1354.. / 0x1360.. | 0x1390 / 0x13A0 / 0x13AC | SyncNpc (52 phiếu) |
| Name0 / Name / m_nSex | 0x13B8 / 0x13B9 / 0x13E0 | 0x1408.. | Init (xóa tên) |
| m_NpcSettingIdx / m_CorpseSettingIdx | 0x13E4 / 0x13E8 | 0x1434 / 0x1438 | SyncNpc |
| m_nPeopleIdx / m_nObjectIdx | 0x1448 / 0x1458 | 0x1498 / 0x14A8 | CheckObject 0x5EE670 |
| **m_FightMode** | **0x1A58** | 0x1920 | SetFightMode 0x5FEA90 `cmp [esi+0x1A58]` (×3) |
| m_SyncSignal / m_sClientNpcID | 0x1A74 / 0x1A7C | 0x16B8 / 0x16C0 | Init |
| m_RegX / m_RegY | 0x1A84 / 0x1A86 | 0x16C8 | SyncNpc |
| m_sSyncPos / m_nPKFlag | 0x1A88 / 0x1AA0 | 0x16CC / 0x16E4 | Init |
| m_dwTongNameID / m_bHaveStall | **không còn** | 0x1810 / 0x1814 (?) | bản 15_10 bỏ; tìm sạp chỉ xét người chơi đang ngồi (`m_Doing == do_sit`) |
| m_Command | 0x1D18 | 0x1960 | SendCommand 0x5F7490 ghi 0x1D18..0x1D24 |
| m_bRideHorse | 0x1D78 | 0x19C0 | OPR case 6 |
| m_CurrentLifeMax/Ex, ManaMax/Ex | 0x11A00 / 04 / 08 / 0C | 0x12B14.. | 7/4 phiếu |

## 4. KPlayer (size 0xA878, `[PLR]` = &Player[0], người chơi = Player[1] = [PLR]+0xA878)

| Field | 15_10 | 10_10 | Neo |
|---|---|---|---|
| m_nEntrustMode (BYTE) | 0x20 | 0x28 | SendCommand `cmp byte [PLR+0xA898],0` |
| m_nLeftSkillID | 0x24 | 0x2C | SetLeftSkill 0x5E8FD0 |
| m_RunStatus / m_nSendMoveFrames | 0xEC / 0xF4 | 0x1808 / 0x1810 | OPR case 6 `[PLR+0xA964]`; MoveTo |
| m_dwHashID / m_nIndex | 0x138 / 0x13C | 0x1850 / 0x1854 | `[PLR+0xA9B4]` (116 phiếu) |
| m_ItemList | 0x144 | 0x185C | PickUpObj `add ecx,0x144` |
| m_nCurShop | 0x6F24 | 0x6794 | `[PLR+0x1179C]` (10 phiếu) |
| m_nCurExp / m_nNextExp | 0x75F0 / 0x75F8 | 0x7220 / 0x7228 | |
| m_cTeam (m_nFlag, m_nFigure) | 0x7628 / 0x762C | 0x7258 | TeamOperation `add ecx,0x11EA0` |
| m_nCurFaction | 0x7BD0 | 0x7800 | |
| m_cPK / m_nPKValue | 0x7E18 / 0x7E70 | 0x7A48 / 0x7AA0 | |
| m_cTong | 0x7E74 | 0x7CFC | 17 lời gọi GetTongNameID 0x6891C0 với `add ecx,0x126EC` |
| KPlayerTong.m_nFlag / m_dwTongNameID | +0x2C / **+0x3C** | +0x2C / +0x38 | GetTongNameID |
| m_cTask (KPlayerTask, 0x630 byte) | 0xA248 | 0xA1A0 | this của GetSaveVal 0x609990 |

## 5. KItemList (KPlayer+0x144)

| Field | 15_10 | 10_10 | Neo |
|---|---|---|---|
| m_nForeHand (m_Hand) | 0x20 | 0x1C | OPR 0x5C2BC9 `cmp [PLR+0xA9DC],0` (cũ 0x5C57C7 `[PLR+0xC0F0]`); 0x60D940 `[ecx+0x20]` |
| m_EquipItem | 0x28, **{nIdx,nData}[16]** | 0x28, int[17] | 0x60D940: `cmp eax,0x10; mov eax,[ecx+eax*8+0x28]` |
| m_Items (PlayerItem 0x14) | 0xA8, [678] | 0x6C, [483] | memset 0x34F8 (0x60B93C) |
| m_UseIdx / m_nListCurIdx | 0x6AF0 / 0x6AFC | 0x4BFC / 0x4C08 | GetFirstItem 0x60B830 |
| m_Room[15] (KInventory 0x1C) | 0x6B44 | 0x4C5C | 0x60D940 (+0x6B44, +0x6BD0 = room 5, +0x6CCC = room 14) |

## 6. KItem (size 0x864)

| Field | 15_10 | 10_10 | Neo |
|---|---|---|---|
| KCommonAttrib (genre +8, … tên +0x30) | không đổi đến 0x80 | | phiếu 0x0C..0x30 giữ nguyên |
| m_aryAllAttrib[19] (0x10/phần tử) | 0x330 ([13] = 0x400) | 0x228 ([13] = 0x2F8) | 7 phiếu mỗi phần tử |
| m_MagicLevel[6] | 0x530 | 0x3AC | GetTotalMagicLevel 0x6004F0 |
| m_dwUniID / m_nCurDur / m_nMaxDur | 0x6A4 / 0x6A8 / 0x6AC | 0x4E0.. | GetRepairPrice 0x6051B0 |
| m_nItemPlus (BYTE) / ExParam1 / ExParam2 | 0x6D8 / 0x6D9 / 0x6DA | 0x514.. | 9 phiếu |
| m_nLeftUsageTime / m_nExpiredTime / m_nLockCount | 0x6DC / 0x6E0 / 0x6E4 | 0x518.. | |
| cờ "không bán/không rã" | 0x860 | 0x6C0 | OPR case 0x1A `cmp [item+0x860], edi` |

## 7. KObj (size 0x4E8) và KSkill (0x4F8)

KObj: 0x00..0x60 không đổi (m_nKind +8, m_Index +0xC, Region +0x10, MapX/Y +0x1C/+0x20, OffX/Y +0x24/+0x28, ObjSkill +0x40, m_szName +0x60).
Từ m_nColorID trở đi dịch +0x114: ScriptID 0x194, **ColorID 0x198, ItemDataID 0x19C, Width/Height 0x1A0/0x1A4, MoneyNum 0x1A8**,
DropState 0x3A4, NameColor 0x478. Nguồn: ClientAdd 0x677F90 (sim 1.00).

KSkill (phần tử của g_SkillManager, chỉ số từ 1): header +0/+4/+8 không đổi; tên +0xC, SkillID +0x2C, IsPhysical +0x38,
AttackRadius +0x50 không đổi; **HorseLimit +0xB4** (cũ 0xB8).

## 8. UI

Engine UI mới: widget gốc nhỏ hơn 0x200 byte. `m_Style` vẫn ở +4 (bit 0x80000000 = hiện).

| Lớp / mục | 15_10 | 10_10 |
|---|---|---|
| KWndList: data / count / cursel | +0x298 / +0x29C / +0x2B0 | +0x498 / +0x49C / +0x4B0 |
| KWndMessageListBox: pMessages / nNum / nCap | +0x298 / +0x29C / +0x2A0 | +0x498.. |
| List kiểu rich (lời thoại): pMessages / nNum / chữ | +0x294 / +0x29C / msg+0x10D8 | +0x494 / +0x49C / msg+0x10D8 |
| KOneMsgInfo: nLen / Msg | +0x18 / +0x1C (không đổi) | |
| KWndText512: bộ đệm chữ | +0x36C (ctor 0x460080) | +0x578 |
| KWndObjectBox.m_Object.uGenre | +0x2B0 (0x451D10) | +0x4B0 |
| KWndButton: cờ checked 0x200 | +0x34C | +0x554 |

| Lớp | m_pSelf | Thành phần |
|---|---|---|
| KUiMsgSel | 0x7CBD2C (new 0x1758) | Select 0x37C (list 0x610 → pMessages **0x8A8**, nNum **0x8AC**); InfoText 0x11E0 → chữ **0x154C** |
| KUiMsgSel2 | 0x7CB470 (new 0x4588) | Msg_Scroll 0x37C, MessageList 0x1AB0 (pMsg **0x1D44**, nNum **0x1D4C**; đọc chữ ở msg+0x10D8 ra rác), **lời thoại NPC 0x2578** char[0x2000] (Show 0x5217B0 strcat từ KUiQuestionAndAnswer rồi mới AddMessage; ctor 0x5212A0 xóa 0x2000 byte, Clear 0x5219C0). Mã màu `02 r g b` có thể chứa byte 0 (vàng `02 FF C7 00`), `03` = hết màu → đọc bằng GameTextLen/GameTextPlain, không dùng strlen/strstr, Image 0x1DA0, Text 0x20EC, Select 0xC4C (list 0xEE0 → pMessages **0x1178**, nNum **0x117C**) |
| KUiInformation (tĩnh) | 0x7C5DD8 (còn bản thứ 2 0x7BA118) | Info 0x37C → m_szInfoText **0x6E8**; FirstBtn 0x8E8, SecondBtn 0xBE0; caller +0xFF0 |
| KUiInformation2 (tĩnh) | 0x7C6CC8 | Info 0x37C → **0x6E8**; OK 0x8E8; caller +0xBE0 |
| KUiShop | 0x7C9D3C | m_pObjectList **0x37C**, m_nObjCount **0x39C**, PageCount 0x3A0, CurrentPage 0x3A4 (UpdateData 0x4CBCC0) |
| KUiPlayerBar | 0x7BE494 | WndProc 0x46FCA0; nút Kỳ Trân Các **+0x1378**; m_ImmediaItem[9] 0x22E4 bước 0x2F0 → m_nHK3Gengre **0x2B74** |
| KUiSysMsgCentre | 0x7CA1FC | m_MsgHeap **+0x294** (12 byte/heap); KSystemMessage 0x408 byte, người mời ở **msg+0x408** (MSG_PLR_OFFSET) |
| KUiInit | 0x7C47D8 | EnterGame **0x39C**, GameConfig 0xE64, ExitGame **0x192C**, OpenRep 0x23F4; OnClickButton 0x490EA0 |
| KUiStoreBox / MsgCentrePad / MiniMap | 0x7C488C / 0x7C9BE8 / 0x7CA200 | |
| Kỳ Trân Các (GShop) | 0x7C9D38 | |
| KUiYeSouBox (Dã Tẩu, scheme 给予界面.ini) | 0x7CAD4C (OpenWindow 0x5165D0, new 0x3794) | Items 0x34C, Title 0x1D78, ContentList 0x2104, ContentScroll 0x23F4, Assemble 0x2CC4, Close 0x3228. **OnConfirm = CloseWindow(bool bAssemble) 0x516790** cdecl(1): OPR(0x3B,0,0) + OPR(0x5D, bAssemble, 0) + đóng; WndProc: Assemble → 1, Close → 0 |
| KUiPlayerStall (sạp hàng, scheme 摆摊物品.ini) | 0x7CAD54 (new 0x103C) | CloseBtn 0x37C, ItemBox 0x8E0, Owner 0xBCC. Không giữ danh sách: UpdateData 0x5187B0 gọi **GetGameData(0xBBF)** (0,0 = đếm; buf,n = điền, handler 0x628011 ghi đủ n món, không xét nParam). Bản ghi 0x154: uId (chỉ số KItem) +0, X/Y/W/H +0x138..+0x144, **giá +0x148**. GDI 0xBC0 = tên chủ sạp |
| KUiPrizeSel (chọn thưởng, scheme 奖励界面.ini) | 0x7CBD1C (OpenWindow 0x53EA00, new 0x3804) | PriseText 0x37C; 3 nút PriseBtn 0x8E8 / 0x189C / 0x2850 (KWndButton 0xFB4). Dữ liệu "chữ/ảnh" tách ở 0x53EDD0 → ảnh nút+0x2B4 (SetImage 0x4494E0), chữ nút+0x358 (SetLabel 0x443580, độ dài +0x558). WndProc 0x53F110: nút i → OPR(9,0,i) + **CloseWindow(0) 0x53EAE0** |
| Chưa dùng | | Tong 0x7CAD28 (bang hội đã bỏ) |

## 9. Đăng nhập (VLAuto/GameLogin.h)

| Tên | 15_10 | 10_10 |
|---|---|---|
| KUiSelServer::m_pSelf | 0x7BE604 (OpenWindow 0x486BE0, new 0x2274) | 0x839A18 |
| LeftList / RightList / IpList | +0x704 / +0xAB0 / +0xE5C | +0x90C / +0xEBC / +0x146C |
| danh sách khu vực / máy chủ / khu vực đang chọn | +0x6FC / +0x700 / +0x2080 | +0x904 / +0x908 / +0x2EB8 |
| LeftList chứa | 17 khu vực | 14 |
| Nguồn khu vực | `\Settings\ServerList.ini`: RegionCount, `Region_%d` → mảng 0x24/dòng (Title[32]) ở +0x6FC (GetServerRegionList 0x5AC720) | như cũ |
| Nguồn máy chủ | GetServerList 0x5AC910(region, &count, &cursel): `%d_Title` → Title[32] +0, `%d_Address` → IP 4 byte +0x20 (0x5AA8E0); mảng ở +0x700, đổ vào IpList (SetContent bước 0x24) | như cũ |
| Chọn khu vực (0x487160) | khu vực = LeftList.cursel (+0x9B4), nếu = -1 thì RightList.cursel (+0xD60) + 17 → +0x2080; nạp lại +0x700 và IpList | như cũ (14) |
| OnLogin (0x487D00) | [+0x700][IpList.cursel +0x110C]: SetAccountServer 0x5ACE10 (chép 0x24 byte vào g_LoginLogic+0x75C), kết nối IP 0x5AACA0, KUiConnectInfo(1,1), đóng cửa sổ 0x486F80 | |
| OnLogin / cập nhật khu vực | 0x487D00 / 0x487160 | 0x48ACD0 / 0x48AF80 |
| KWndList SetCurSel / ClearSel / Focus | 0x44C4F0 / 0x480DD0 / 0x480C90 | 0x45BD20 / 0x483C30 / 0x483B10 |
| KUiLogin::m_pSelf | 0x7BE5EC (OpenWindow 0x483E70) | 0x8399FC |
| Account / Password | +0x380 / +0x67C | +0x588 / +0xA94 |
| KUiLogin::OnLogin | 0x484C20 (không còn ô đồng ý) | 0x4871D0 (+0x80C0) |
| KWndEdit::SetText / KWndButton::CheckButton | 0x4488F0 / 0x442DD0 | 0x458A30 / 0x453AD0 |
| KUiSelPlayer::m_pSelf | 0x7C48AC (new 0x89C8) | 0x84053C |
| OnSelectPlayer / OnEnterGame | 0x49FEE0 / 0x4A0370 | 0x49C940 / 0x49C210 |
| **cbUserAgreement** | **+0x8464** (OnEnterGame kiểm tra trước) | – |
| m_nNumPlayer / m_nSelPlayer / trang | +0x7E1C / +0x7E1E / +0x7E28 | +0xB964 / +0xB966 / +0xB970 |
| g_LoginLogic | 0x848A68: m_Status +8 (= **6** khi có nhân vật), short m_nNumRole +0x12, roles +0x14 (**0x28**/nhân vật, Level int +0x24) | 0x9BCBB8 (7; 0x24/nhân vật) |

## 10. Bản đồ, vẽ, giảm cấu hình

| Macro | 15_10 | 10_10 | Neo |
|---|---|---|---|
| SCP_REGIONS_OFF / SCP_NUM_REGIONS | 0x31E4C / 9 | 0x31E64 / 15 | ScenePaint 0x63C2E0: `lea edi,[esi+0x31E4C]; mov ebx,9` |
| SPR_STATUS/LEFTTOP/TRAP_OFF | 0x04 / 0x08 / 0x848 | như cũ | LoadTrap 0x6F80D0 / GetTrap 0x6F86B0 giống hệt |
| RPS_VT_OUTTEXT / DRAWPRIM | 0x10 / 0x48 | 0x14 / 0x4C | 45 lời gọi [vt+0x48]; OutputText `push 0xFF000000; push 5; push 3; push -1` |
| LCF_PAINT_FPS_ADD | 0x7AD7B0 (FPS, mặc định 30) | 0x815838 (ms) | config `PaintFps`; GameLoop 0x5A9160 `imul eax,[0x7AD7B0]` |
| LCF_INACTIVE_ADD | 0x8483B0 = KMyApp(0x848360)+0x50 | 0x9BC508 | GameLoop `sub eax,[ecx+0x50]; cmp eax,0x2710` |
| LCF_VISIBLE_ADD | 0x7AE608 | 0x81D15C | OPR 0x2B uParam 6 → 0x63A670 |
| LCF_KOPTION_ADD | 0x17969C0 | 0x220F9AC | OPR 0x2B uParam 2/3 |
| SCP_WEATHER_OFF / SCP_DYNALIGHT_OFF | 0x06 / 0x3261A (KIpoTree 0x325C0 + 0x5A) | 0x06 / 0x32A52 | EnableWeather 0x63B9F0 / EnableDynLight 0x63B900 |

## 11. Mã lệnh

- **GOI_\*** (OperationRequest): các mã hook dùng giữ nguyên: 3, 5, 9, 0xA, 0xD, 0x18, 0x1A, 0x1B, 0x2B, 0x46, 0x48, 0x59.
  Đã đối chiếu handler từng case. Map goto 0x85 → **0x81**. Rã trang bị 0x9B **không còn**.
- GDI_GAME_OBJ_DESC (1), GDI_TRADE_ITEM_PRICE (0xBBA): GetGameData giống hệt từng byte.
- TEAM_* (5..0xD), ProcessPeople (case 3, 10), ProcessGameSecurity (0..7): không đổi.
- MAGIC_ATTRIB: xem `GameDef.h` và `VLHook/MagicAttrib.cpp` (sinh lại từ bảng tên 0xEE0278, giữ mô tả TCVN3 cũ theo tên).
  File `.ita` cũ vẫn dùng được với ID ≤ 293.

## 12. Còn lại / chưa xác minh

- KNpc `m_dwTongNameID` / `m_bHaveStall` và KPlayerTong `m_nContributePoint`: **bản 15_10 không còn** (giữ chỗ trống để không lệch layout; `ContributePoint()` trả 0).
- Chọn thưởng: hook bỏ 9 ký tự đầu của chữ ("Nhận được") như bản cũ, chưa kiểm tra định dạng chữ thật ở 15_10.
- KSystemMessage: chỉ xác minh vị trí người mời (+0x408); `szMessage` giả định vẫn ở +0.
- OnSendSomeoneMessage (nhắn tin riêng): chưa tìm thấy. Chat 0xFB dựng ở 0x471640 (thiscall 5 tham số, kênh 0x2D).
- Bản này **chưa chạy thử trong game**. Mọi offset trên chỉ được xác minh tĩnh (mã máy + static_assert lúc build).

### File danh sách server (nút "Đọc danh sách server (file)")
- 15_10 không còn `UserDataCl\serverlist.ini` (XOR 0x32). Game đọc `\settings\serverlist.ini` qua hệ file pak theo thứ tự `package.ini` ([Package] Path=\data, 0=vngitem.pak, 1=slistcache.pak, 2=serverlist.pak, ...), pak đầu tiên có file được dùng (hiện tại: `vngitem.pak`).
- Pak: header `PACK`, số file, vị trí bảng chỉ mục, vị trí dữ liệu; mỗi dòng chỉ mục 16 byte = mã file, vị trí, kích thước gốc, (method << 24) | kích thước nén. Method 1 = UCL nrv2b (bit buffer 8 bit); slistcache.pak dùng method 0x20 (không chứa serverlist).
- Mã file: chữ thường, `id = (id + (i+1)*c) % 0x8000000B * 0xFFFFFFEF`, cuối cùng `^ 0x12345678` ("\settings\serverlist.ini" = 0x769629AC).
- Nội dung TCVN3: [List] RegionCount, Region_%d; [Region_%d] Count, %d_Title, %d_Address. VLAuto xuất UTF-8 ra `UiConfig\serverlist_.txt` cho Auto Login.

### Bảng "Nhắc nhở" sau Bắt Đầu Trò Chơi (KUiHealthNotice)
- 15_10: KUiInit nút Bắt Đầu (+0x39C) gọi 0x483760 = KUiHealthNotice::OpenWindow (m_pSelf `0x7BE5E8`, size 0x190C, vtable 0x752834, "UiNewLogin\健康游戏公告.ini": Close +0x37C, List +0x8E0, MessageList +0xD4C, MessageScroll +0x103C) rồi đóng KUiInit.
- WndProc `0x483B10` (vtable +0x10): msg 0x565 với uParam = this+0x37C (nút Xác định), hoặc phím Enter/Space/Esc => `KUiSelServer::OpenWindow 0x486BE0`, thành công thì `CloseWindow(1) 0x483860`.
- Auto Login / Debug bấm Xác định bằng WndProc(0x565, this+0x37C, 0) chạy trong thread của Game.

### Chọn cụm server / server (KUiSelServer) - lỗi treo Game
- WndProc `0x487690`: 0x691 từ Left/RightList (SetCurSel báo lên) => bỏ chọn list kia, nếu `+0x226C == 1` (OpenWindow đặt) thì nạp lại IpList `0x487160`; 0x693 (double click IpList) hoặc nút "Login" `+0x1594` (Xác định, 0x565 => 0x487CC0) => `OnLogin 0x487D00` (máy chủ = IpList cursel `+0x110C`); nút `+0x1AF8` = Hủy bỏ (0x487DB0).
- `0x487160` gọi `0x488560`: `0x4885A0` TerminateThread thread ping cũ (`+0x2244`) rồi CreateThread(`0x488340`). OnLogin cũng gọi `0x4885A0`.
- Gọi `0x487160` lần 2 ngay sau SetCurSel (hoặc OnLogin ngay sau khi đổi cụm) => thread ping vừa tạo bị TerminateThread lúc đang khởi tạo, giữ loader lock => mọi thread mới (kể cả thread của VLAuto) treo vĩnh viễn. Auto Login: đổi cụm 1 nhịp, chọn server + Xác định ở nhịp sau.

## Góp quỹ bang (dò 2026-10-16, tĩnh)

- `KCoreShell::TongOperation` 0x5C5300: switch nRequest (byte 0x5C5A44 → bảng 0x5C59C4); nRequest **4** → 0x5C53E1: `p[0]` = lệnh con 0..12 (bảng 0x5C5A80), cần p và buf khác NULL.
- Lệnh con **2** = góp quỹ (như 10_10): 0x5C54DD → `0x5BDDA0(this 0x17A4F00, [Player+0x12714] (= m_cTong 0x7E74 + 0x28), TongNameID, __int64 p[2])` (10_10: 0x5BC630, this 0x22100A0).
- Gọi từ VLHook: `int p[13] = {2, 0, nMoney}; char buf[0x138]; g_pCoreShell->TongOperation(4, (int)p, (int)buf);` (CVLMain::StoreTongMoney, nút Debug tab Map "Góp Idx lượng vào quỹ bang"). **Chưa thử trong game 15_10.**
- Tiền rương cho bảng Thống kê (VLAuto/GameCore.h): `PLR_ROOM_OFFSET` = m_ItemList 0x144 + m_Room 0x6B44 = **0x6C88** (KInventory 0x1C, m_nMoney +4; room 0 = trên người, room 1 = rương).
