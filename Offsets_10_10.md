# Offset bản client 10_10 (gamecl_dump_SCY_10_10.exe)

Dò bằng cách đối chiếu mã nguồn `Src_JX` với disassembly của bản dump (ImageBase 0x400000).
"Neo" = hàm trong source dùng để nhận diện. Các giá trị ở mục 1–3 đã ghi vào
`VLHook/GameBase.h` và `VLAuto/GameCore.h` (đánh dấu `10-10`).

## 1. Biến toàn cục

| Macro | Giá trị | Neo / bằng chứng |
|---|---|---|
| PLR_BASE_ADD | 0x009C0474 | `[PLR]+0xC0CC` = Player[1].m_nIndex (330 lần) |
| NPC_BASE_ADD | 0x01A3B4B4 | `Npc = new KNpc[0x320]` tại 0x5CCAD6 |
| MAP_ID_ADD / MAP_NAME_ADD | 0x021D6840 / 0x021D6820 | (bạn tìm) |
| CORE_BASE_ADD | 0x009BC130 | `mov ecx,[0x9BC130]` trước mọi lời gọi OperationRequest/GetGameData |
| WLD_BASE_ADD | 0x009C08A8 | SyncNpc: `m_SyncSignal = [[0x9C08A8]+0x50]` (SubWorld[0].m_dwCurrentTime) |
| NSET_BASE_ADD | 0x021A18A8 | `this` của NpcSet.SearchID/Add trong SyncNpc |
| OBJ_BASE_ADD | 0x021A1898 | `new KObj[0x100]`, size 0x3F4 |
| ITM_BASE_ADD | 0x01F18E14 | `new KItem[0x200]`, size 0x74C (tên item ở +0x30) |
| BSL_BASE_ADD | 0x024E7750 | GOI_TRADE_NPC_BUY: `BuySell.GetItemIndex(...)` |
| MSG_BASE_ADD | 0x008C13D8 | KUiMsgSel::CloseWindow; LoadScheme dùng `滚动选择界面.ini` |
| MSG2_BASE_ADD | 0x008C1368 | lớp VN dùng `npc描述界面.ini` (size 0x4DCC > MsgSel 0x2380) |
| UPB_BASE_ADD | 0x00839890 | LoadScheme `玩家信息主界面.ini` |
| SBO_BASE_ADD | 0x0084051C | LoadScheme `储物箱.ini` |
| MCP_BASE_ADD | 0x008478A8 | LoadScheme `消息集合面板_左.ini` |
| SMC_BASE_ADD | 0x00847EF4 | LoadScheme `系统消息.ini` |
| SHOP_BASE_ADD | 0x008478E0 | LoadScheme `npc买卖界面.ini` |
| IF1_BASE_ADD / IF2_BASE_ADD | 0x00842460 / 0x00843B68 | `this` (static) truyền vào LoadScheme `提示.ini` / `提示2.ini` |

Kích thước: `NPC_DATA_SIZE 0x12BD4`, `PLAYER_OFFSET (= sizeof KPlayer) 0xA878`,
`ITM_DATA_SIZE 0x74C`, `OBJ_DATA_SIZE 0x3F4`, `MAX_PLAYER_ITEM 483`, **MAX_NPC = 800** (code còn để 256).

## 2. Hàm (đã kiểm tra lệnh `ret`, khớp cách hook gọi)

| Macro | Địa chỉ | Kiểu | Neo |
|---|---|---|---|
| EXS_FUNC_ADD | 0x004360B0 | cdecl (1) | ExecuteScript (bạn tìm), 38 lời gọi |
| CLA_FUNC_ADD | 0x0044C560 | cdecl (1) | UiCloseWndsInGame(bool bAll): chuỗi GetIfVisible() của ~20 cửa sổ (có StoreBox 0x497EF0, Item 0x491D50) — đã kiểm tra lại |
| GGD_FUNC_ADD | 0x005B9F30 | thiscall (3) | dispatcher theo dải ID (1–12, 0x3E9–0x7D0, 0xBB9…) |
| OPR_FUNC_ADD | 0x005C3820 | thiscall (3) | case 1 = GOI_EXIT_GAME, case 5 = MONEY_INOUT_STORE_BOX khớp source |
| TNP_FUNC_ADD | 0x005C2050 | thiscall (3) | mở đầu GetTongNameID, switch 0..0x3D (chứa 0x23 hook dùng) |
| TOP_FUNC_ADD | 0x005BB090 | thiscall (3) | đụng m_cTeam, gọi các hàm team của KPlayer |
| PGS_FUNC_ADD | 0x005BB3C0 | thiscall (3) | switch 0..7, gọi hàm khoá rương của ItemList |
| GRL_FUNC_ADD | 0x005BAAF0 | thiscall (1) | KCoreShell::GetNPCRelation = NpcSet.GetRelation(me, n) |
| MVT_FUNC_ADD | 0x005F3B30 | thiscall (3) | KNpc::MoveTo: SendCommand(do_walk/do_run) + SendClientCmdWalk/Run |
| SKL_FUNC_ADD | 0x005ECED0 | cdecl (3) | SendClientCmdSkill(skill,x,y), gói 0x4D dài 0x11 |
| PKO_FUNC_ADD | 0x005E2530 | thiscall (1) | KPlayer::PickUpObj: CheckTrading, kind 3/4 |
| GTV_BASE_ADD | 0x0061C820 | thiscall (1) | KPlayerTask::GetSaveVal, n ≤ 0x176F; this = KPlayer+0xA1A0 |
| BGI_FUNC_ADD | 0x006B9390 | thiscall (1) | KBuySell::GetItem |
| BGX_FUNC_ADD | 0x006B93E0 | thiscall (2) | KBuySell::GetItemIndex(nBuyIdx, nID) |
| OMS_FUNC_ADD | 0x0051EB50 | thiscall (1) | KUiMsgSel::OnClickMsg |
| OMS2_FUNC_ADD | 0x00509E40 | thiscall (1) | OperationRequest(9,0,n) + CloseWindow(1) |
| SMA_FUNC_ADD | 0x004B9340 | cdecl (3) | KUiMsgCentrePad::SystemMessageArrival(msg, len, bChannelBox): đọc m_pSelf 0x8478A8, check DefaultChannelResource (+0xC5C4) < nChannelsResource (+0xC5A0); arg3=0 → m_Sys (+0x7AD4) + ScrollBottom, ≠0 → ô +0x847C; m_pSelf=NULL → đệm vào hàng chờ 0x4B5B10 |
| SBO_FUNC_ADD | 0x00498F80 | cdecl (0) | KUiStoreBox::OpenWindow: duy nhất ghi `new`(0x4980) vào m_pSelf 0x84051C, rồi Initialize/UpdateData/BringToTop/Show |
| SHO_FUNC_ADD | 0x004BF2F0 | cdecl (0) | KUiShop::OpenWindow: duy nhất ghi `new`(0x4D6C) vào m_pSelf 0x8478E0, cuối gọi KUiItem::OnNpcTradeMode(1) |

Hàm phụ tìm được (chưa có macro): KNpc::SendCommand 0x5F1A20, KNpc::SetFightMode 0x5FA900,
NpcSet::SearchID 0x66F340, NpcSet::GetRelation 0x66FCF0, KPlayer::SetLeftSkill 0x5DED00,
KPlayer::CheckObject 0x5E4CE0, SendClientCmdWalk/Run 0x5ED2B0/0x5ED230, SyncNpc 0x65ECF0,
SyncPlayer 0x65FE00, SyncCurNormalData 0x65EAF0, KNpc::Init ~0x5F7600, KNpc ctor 0x5FB420.

## 3. KNpc (size 0x12BD4)

| Field | Cũ | Mới | Neo |
|---|---|---|---|
| (VN) buffer ptr | – | 0x00 | ctor: `[esi] = new char[0x401]` |
| m_dwID | 0x00 | **0x08** | NpcSet::AddID; SendClientCmdSkill(…, npc->[+8]) |
| m_Index | 0x04 | **0x0C** | so với Player.m_nIndex trong SendCommand |
| m_Node | 0x08 | 0x10 | ctor |
| m_Level | 0x20 | **0x28** | SyncNpc: LOWORD(NpcSettingIdx) |
| m_Kind | 0x24 | **0x2C** | SyncNpc / SyncPlayer (=1) |
| m_Series | 0x28 | **0x30** | SyncNpc |
| m_Height | 0x2C | 0x34 | SyncNpc (=0) |
| m_btRankId (BYTE) | 0x30 | 0x38 | SyncPlayer |
| (VN) cờ | – | 0x40/0x44/0x48 | bit cờ trong SyncNpc |
| m_cGold | 0x3C | **0x4C** | SetGoldCurrentType(this=npc+0x4C) |
| m_PoisonState | 0x84 | 0x94 | suy ra (khối 7 KState 0x84..0xF3) |
| m_Camp / m_CurrentCamp | 0xE4/0xE8 | **0xF4 / 0xF8** | SyncNpc |
| m_Doing | 0xEC | **0xFC** | GOI_PLAYER_ACTION: `== 8` (do_sit) |
| m_pHeadNode / Tail / Count | 0xFC.. | **0x110 / 0x114 / 0x118** | ctor gọi `KList::KList` tại +0x110 (KList = head,tail,count) |
| m_ActiveSkillID | 0x108 | **0x11C** | KNpc::Init |
| m_SkillList (KSkillList) | – | 0x124 | ctor; SetLeftSkill dùng npc+0x124 |
| m_Skills (= KSkillList.m_Skills[1], phần tử 0x30) | 0x148 | **0x15C** | FindIdx so `[list+0x38+(i-1)*0x30]`; 0x15C+80×0x30 = 0x105C (m_CurrentLife) |
| m_RegionIndex | – | 0x1054 | Init (= -1) |
| m_CurrentLife | 0x1048 | **0x105C** | SyncNpc `(Max*pct)>>7`; SyncCurNormalData |
| m_CurrentMana | 0x1058 | **0x1070** | SyncCurNormalData; Init |
| m_CurrentStamina / Max | – | 0x1078 / 0x107C | SyncCurNormalData; Init |
| m_CurrentWalkSpeed / RunSpeed | 0x1138/0x113C | **0x1150 / 0x1154** | Init (=5 / =10); SyncPlayer |
| m_CurrentAttackRadius | 0x1158 | **0x1170** | Init (=30) |
| m_MapX/Y/Z | 0x12A8.. | **0x13A0 / 0x13A4 / 0x13A8** | SyncNpc |
| m_OffX/Y | 0x12B4.. | **0x13AC / 0x13B0** | SyncNpc |
| Name0 / Name | 0x130C/0x130D | **0x1408 / 0x1409** | SyncNpc strcpy |
| m_NpcSettingIdx | 0x1338 | **0x1434** | SyncNpc HIWORD; Init |
| m_nPeopleIdx | 0x139C | **0x1498** | Init; GOI chọn mục tiêu |
| m_nObjectIdx | 0x13AC | **0x14A8** | KPlayer::CheckObject xoá về 0 |
| m_SyncSignal | 0x1574 | **0x16B8** | SyncNpc / SyncCurNormalData |
| m_RegX / m_RegY (= m_dwRegionID) | 0x1584 | **0x16C8 / 0x16CA** | thứ tự source: SyncSignal, sClientNpcID(0x16C0,-1@0x16C4), dwRegionID, sSyncPos(0x16CC×6) |
| m_nPKFlag | 0x15A0 | **0x16E4** | SyncPlayer (`pkt & 3`); Init |
| m_FightMode | 0x1564 | **0x1920** | music.Start(SubWorldID, time, FightMode); SetFightMode 0x5FA900 |
| m_Command | 0x1758 | **0x1960** | KNpc::SendCommand ghi 0x1960..0x196C |
| m_bRideHorse | 0x1800 | **0x19C0** | GOI_PLAYER_ACTION (không ngồi khi cưỡi ngựa) |
| m_CurrentLifeMax | 0x11300 | **max(0x12B14, 0x12B18)** | game luôn lấy max hai giá trị khi tính % máu |
| m_CurrentManaMax | 0x11308 | **max(0x12B1C, 0x12B20)** | tương tự |
| m_dwTongNameID / m_bHaveStall | 0x16CC/0x16D0 | 0x1810 / 0x1814 (?) | suy theo khoảng cách cũ, **chưa xác minh** (dùng ở tìm sạp hàng) |

## 4. KPlayer (size 0xA878, `[PLR]` = &Player[0], người chơi = Player[1])

| Field | Cũ | Mới | Neo |
|---|---|---|---|
| m_nEntrustMode | 0x20 | **0x28** | SendCommand/SendClientCmd* kiểm tra `byte [PLR+0xA8A0]` (game đọc như BYTE) |
| m_nLeftSkillID | 0x24 | **0x2C** | KPlayer::SetLeftSkill |
| m_RunStatus | 0xC4 | **0x1808** | GOI_PLAYER_ACTION PA_RUN |
| m_nSendMoveFrames | 0xCC | **0x1810** | tăng mỗi frame, so với 5, reset sau Walk/Run |
| m_dwHashID (= m_dwID) | 0x110 | **0x1850** | handler chat so ID người gửi |
| m_nIndex | 0x114 | **0x1854** | (bạn tìm) |
| m_ItemList | 0x13C | **0x185C** | ExchangeMoney / SetDefaultImmedSkill |
| m_nCurShop (= m_BuyInfo.m_nBuyIdx) | 0x4F80 | **0x6794** | GOI_TRADE_NPC_BUY |
| m_nCurExp / m_nNextExp (int64) | 0x58D8 | **0x7220 / 0x7228** | khối RT_INFO 0x6643B2 |
| m_cTeam (m_nFlag, m_nFigure) | 0x5910 | **0x7258 / 0x725C** | SyncCurNormalData xử lý byte team |
| m_cPK | – | **0x7A48** | layout KPlayerPK client khớp source |
| m_nPKValue | 0x6190 | **0x7AA0** (= m_cPK+0x58) | KPlayerPK::SetPKValue 0x6ADC70 |
| m_cTong | 0x6194 | **0x7CFC** | GetTongNameID 0x621D10 (m_nFlag +0x2C, m_dwTongNameID +0x38, layout hook vẫn đúng) |
| m_nCurFaction | 0x5EC0 | **0x7800** | KPlayerFaction::Init (= -1) 0x683B70 |
| m_cTask | 0x8584 | **0xA1A0** | this của GetSaveVal 0x61C820 |

## 5. KItemList (ở KPlayer+0x185C)

| Field | Cũ | Mới | Neo |
|---|---|---|---|
| m_nForeHand (m_Hand) | 0x00 | **0x1C** | 0x51B150 lấy item đang cầm để tạo đối tượng kéo-thả |
| m_EquipItem | 0x08, struct 8 byte `{m_nIndex,m_nData}`[17] | **0x28, `int[17]`** | `[PLR + part*4 + 0xC0FC]`; cần sửa code `m_EquipItem[i].m_nIndex` → `m_EquipItem[i]` |
| m_Items (PlayerItem 0x14) | 0x94, [472] | **0x6C, [483]** | GetFirstItem 0x6123D0; memset 0x25BC |
| m_UseIdx / m_nListCurIdx / m_bLockOperation | – | 0x4BFC / 0x4C08 / 0x4C0C | GetFirstItem, Lock/Unlock |
| m_Room[] (KInventory 0x1C) | 0x4C50 | **0x4C5C** | `[PLR + i*0x1C + 0x10D30]`; m_nMoney ở +4 |

## 6. Nhóm hàm/biến dò thêm (đợt 2)

| Macro | Địa chỉ | Kiểu | Neo |
|---|---|---|---|
| UWP_FUNC_ADD | 0x00478440 | thiscall (3) | KUiPlayerBar::WndProc = vtable 0x791784[4]; nhánh 0x565 so uParam với các nút |
| GSHOP_BUTTON | this+0xCC4 | – | nút mở cửa sổ m_pSelf 0x8478DC (LoadScheme `奇珍阁界面.ini`) |
| OSM_FUNC_ADD | 0x004768D0 | cdecl (3) | OnSendSomeoneMessage: check name/buf/len/m_pSelf 0x839890, gói 0xFB/0x20, packageID -1 |
| IOC_FUNC_ADD | 0x0048FFB0 | thiscall (1) | KUiInit::OnClickButton; m_ExitGame = **this+0x2364** → CloseWindow + UiPostQuitMsg |
| ENTERGAME_BUTTON | this+0x5A4 | – | nút "Bắt Đầu Trò Chơi": OnClickButton → mở cửa sổ đăng nhập 0x48BA00 (m_pSelf 0x839A18) + CloseWindow 0x48F870(1). Các nút khác: Tùy Chọn +0x1484, Xem Ghi Hình +0x3244 (cách nhau 0xEE0) |
| UIN_BASE_ADD | 0x00840460 | pointer | KUiInit::m_pSelf (OpenWindow 0x490340, `开始.ini`) |
| IH1_FUNC_ADD | 0x004AE490 | thiscall (1) | KUiInformation::Hide(n): m_pCallerWnd +0x15EC → WndProc(0x501) |
| IH2_FUNC_ADD | 0x004AE940 | thiscall (0) | KUiInformation2::Hide: caller==-1 → OperationRequest(8,0,0) |
| PCP_FUNC_ADD | 0x004C4530 | cdecl (2) | ProcessPeople: case 3 (JOINTEAM, nData==1), case 10 (REVENGE → OPR 0x2E) |
| SMD_FUNC_ADD | 0x004C5390 | thiscall (2) | DeleteMsgInHeap(heap,msg); **m_MsgHeap = this+0x4C8**, phần tử 12 byte |
| KSystemMessage | sizeof 0x108 | – | người mời ở **msg+0x108** (TEAM_INVITE_RESPONSE 0x0D, 0x4C5822) → MSG_PLR_OFFSET 0x108 |
| SMF_FUNC_ADD | 0x004C7C70 | cdecl (3) | **chữ ký mới** (x, y, bWorld) toạ độ tuyệt đối; wrapper SetMapFlag tự cộng vị trí nhân vật |
| FLAG_X/Y_OFFST | 0x00811D24 / 0x00811D28 | int | ghi trong 0x4C7C70: x>>4, y>>5; -1 = không có cờ |
| MMP_BASE_ADD | 0x00847EF8 | pointer | KUiMiniMap::m_pSelf (chỉ dùng để kiểm tra khác NULL) |
| IGR_FUNC_ADD | 0x0063A500 | thiscall (0) | KItem::GetRepairPrice: ItemSet(0x24F4348).nPriceScale/nMagicScale (+0xE8/+0xEC) |
| DNPC_FUNC_ADD | 0x005E4C80 | thiscall (1) | KPlayer::DialogNpc: gọi sau SendClientCmdWalk trong AI (0x6EB658), ActionScript +0x143C |
| PAA_FUNC_ADD | 0x005DEA20 | thiscall (2) | ApplyAddBaseAttribute: attr ≤ 3, n > 0, gói 0x57 dài 0xE |
| TEAM_ADDRESS | 0x01F18A80 | static | &g_Team(0x1F18A78).m_nCaptain; m_nMemNum g+0x28, level g+0x30, name[8][32] g+0x50, ServerID g+0x150 |
| LEVEL_EXP_TB | 0x024F1E4C | int[0x104] | PlayerSet(0x24F1E18).m_cLevelAdd (+0x34), nạp `level_exp.txt`, trần 2e9 |
| ACNT_ID_ADD | 0x009BD2D8 | char[32] **đảo bit** | g_LoginLogic(0x9BCBB8).m_Choices.Account (+0x720); VLAuto đã sửa để đảo lại |

KItem (0x74C): m_aryAllAttrib **0x228** (không đổi, xác nhận qua GetTotalMagicLevel 0x634650 đọc [13] ở 0x2F8),
m_MagicLevel **0x3AC**, m_dwUniID **0x4E0**, m_nCurDur/m_nMaxDur **0x4E4/0x4E8**. m_nItemPlus/m_nLockCount: chưa xác minh.

Ứng viên chưa chắc (chưa dùng):
- KUiPrizeSel? = `奖励界面.ini`, m_pSelf 0x8C1390 (size 0x6180)
- KUiYeSouBox? = `给予界面.ini`, m_pSelf 0x8C139C (size 0xB7CC); 0x51B1E0 = OPR(0x3B)+OPR(0x5D)+đóng
- KUiPlayerStall? = `摆摊物品.ini`, m_pSelf 0x8C0A38
- Tong: `帮会列表窗口.ini` m_pSelf 0x8C0990 (lớp bang hội viết lại hoàn toàn)

## 7. Layout bên trong lớp UI (đợt 3)

`KWndMessageListBox` (engine): pMessages **+0x498**, nNumMessage **+0x49C**, nCapability +0x4A0
(GetOneMessage 0x45E110, AddOneMessage 0x45F0D0). `KOneMsgInfo`: nLen +0x18, Msg **+0x1C** (không đổi).
`KScrollMessageListBox`: list nằm ở +0x494 của nó. `KWndText` base 0x578 → bộ đệm chữ của KWndText256/512 ở **+0x578**.

| Lớp | Field | Offset | Neo |
|---|---|---|---|
| KUiMsgSel (m_pSelf 0x8C13D8, 0x2380) | "Select" / list | 0x584 / 0xA18 | LoadScheme 0x51EA60; OnClickMsg 0x51EB50 dùng this+0xA18 |
| | m_pMessages / m_nNumMessage | **0xEB0 / 0xEB4** | GetMsgCount 0x51EBC0 = `[ecx+0xEB4]` |
| | m_InfoText (KWndText512 "InfoText") | 0x1BFC → chữ **0x2174** | ctor 0x51F000 → KWndText512 ctor 0x46B540 (buf +0x578, 0x1FF) |
| KUiMsgSel2 (m_pSelf 0x8C1368, 0x4DCC) | "Select" / list | 0x1268 / 0x16FC | Show thêm đáp án vào 0x16FC (từ QA+0x40C, bước 0x104, số lượng QA+0x404) |
| | m_pMessages / m_nNumMessage | **0x1B94 / 0x1B98** | `[ecx+0x1B98]` 0x5099BD |
| | "MessageList" (lời thoại NPC, list kiểu rich) | 0x28E0: pMessages **0x2D74**, num **0x2D7C**, chữ ở msg+**0x10D8** | AddOneMessage rich 0x464C60 |
| | "Msg_Scroll" / "Image" / "Text" | 0x584 / 0x2DD0 / 0x3324 | LoadScheme 0x509590 |
| KUiShop (m_pSelf 0x8478E0, 0x4D6C) | m_pObjsList / m_nObjCount / m_nPageCount / m_nCurrentPage | **0x584 / 0x5A4 / 0x5A8 / 0x5AC** | UpdateData 0x4BEDA0 (GDI 0xBB9, phần tử 0x1C); SetPage 0x4BE960 |
| KUiInformation / KUiInformation2 | m_szInfoText ("Info" + 0x578) | **0xAFC** | LoadScheme 0x4AE100 / 0x4AE7D0, "Info" ở 0x584 |

VLHook: `KUiMsgSel2::m_InfoText` bị bỏ, thay bằng `GetInfoText(i)`; `KUiMsgSel::m_nInfoLen` bị bỏ (dùng strlen).

## 8. Bảng kỹ năng và phím tắt (đợt 4)

**SKL_BASE_ADD = 0x01AB4A10** (g_SkillManager; `KSkillManager::Init` 0x60D740 được gọi với ecx này ở 0x5C8D95/0x5D2685),
**SKL_DATA_SIZE = 0x4FC** (m_SkillInfo[3000]; m_pOrdinSkill ở +0x3A6920 = 3000×0x4FC). Phần tử:
+0 TabFileRowId, +4 SkillStyle, +8 **MaxLevel**, +0xC khối dữ liệu nạp từ Skills.txt (0x6F7DB0, ebp = entry+0xC):

| Field hook | Entry + | Cột Skills.txt |
|---|---|---|
| m_szSkillName[32] | 0x0C | SkillName |
| m_dwSkillID | 0x2C | SkillId |
| m_bIsPhysics (VLAuto) | **0x38** (cũ 0x3C) | IsPhysical |
| m_nSkillRange | 0x50 | AttackRadius |
| m_nHorseLimited | **0xB8** (cũ 0xB4) | HorseLimit |

Các cột khác (obj+): EqtLimit 0xA8, IsAura 0x34, IsMelee 0x38, AttackRadius 0x44, WaitTime 0x48, Series 0x54, CharClass 0x6C,
CostValue 0x94, TimePerCast 0x9C, TimePerCastOnHorse 0xA0, ChildSkillId 0xB8, SkillDesc 0x110, SkillIcon 0x368.

**KUiPlayerBar::m_nHK3Gengre = 0x3260**: m_ImmediaItem[9] ở 0x23D0 (bước 0x4F0, "Item_%d" trong 0x4746E5),
KWndObjectBox::m_Object.uGenre ở +0x4B0 (GetObject 0x460000 / HoldObject 0x460070) → ô 3 = 0x23D0 + 2×0x4F0 + 0x4B0.

## 9. KObj (đợt 5) — layout KHÔNG đổi

sizeof 0x3F4 (OBJ_DATA_SIZE), mảng Object = [0x21A1898]. Xác nhận qua:
- `KObjSet::AddData` 0x6826F0: m_nIndex +0xC, BelongRegion +0x10 (hook đọc thành WORD m_RegX/m_RegY), SubWorldID +0x14,
  RegionIdx +0x18, MapX/MapY +0x1C/+0x20, OffX/OffY +0x24/+0x28.
- `KObjSet::ClientAdd` 0x683490 (thứ tự ghi y như source): m_nColorID +0x84, m_nItemDataID +0x88, Width/Height +0x8C/+0x90,
  m_nMoneyNum +0x94, m_szName +0x60 (strcpy), m_nDropState +0x2B0, m_dwNameColor +0x384.
- m_nKind +8 (Obj_Kind_Item = 3, Money = 4 — khớp PickUpObj 0x5E2530).

## 10. KItem phần đuôi (đợt 6)

| Offset | Kiểu | Field | Neo |
|---|---|---|---|
| 0x514 | BYTE | **m_nItemPlus** ("+N") = ItemExParam[0] | GetDesc 0x6390E0: `"%s+%d%s"` với byte [item+0x514] (khi [item+8] == 4); getter 0x612F00, setter 0x63D580 |
| 0x515 | BYTE | ItemExParam[1] | 0x634900; s2c sync item 0x65CF6E (gói +0x2F) |
| 0x516 | WORD | ExParam2 | 0x631E20 / 0x631E30; lua `ITEM_GetItemExParam` 0x72A5B0 |
| 0x518 | int | LeftUsageTime | lua `ITEM_GetLeftUsageTime` 0x72A6E0 |
| 0x51C | int | ExpiredTime | lua `ITEM_GetExpiredTime` 0x72A780 |
| 0x520 | int | **m_nLockCount** (khoá): -1 vĩnh viễn, >0 có hạn, 0 không | s2c 0x65C110 `{itemID, value}`; GetDesc so -1; UI đếm -2 |

Hook cũ khai m_nItemPlus là `int` → đọc lẫn 0x515–0x517; đã sửa thành BYTE.
Lưu ý: `KItem::CanBeRepaired` 10-10 (0x6346D0) không xét "+N"; giá sửa do `\script\item\repairitem.lua` (get_repair_price) tính,
nên điều kiện `m_nItemPlus > 5 → không sửa` trong hook chỉ là quy ước của bản cũ.

## 11. Rã trang bị (đợt 7)

| Mục | Giá trị | Bằng chứng |
|---|---|---|
| Nút "DecomposeEquip" (Rã, Alt+V) | KUiItem(`m_pSelf` 0x840474) + 0x7C94 | Init 0x4923B3; click → 0x49331D → 0x492990 |
| Bật/tắt con trỏ "Giải" | cờ trạng thái chuột `0x400000` tại 0x8329C8 | 0x492990 gọi 0x44B7A0 (set) / 0x4486B0 (get) |
| Click item khi đang "Giải" | 0x492620(pObj, 0x9B, 0x400000) | 0x492EE0 (ô hành trang), 0x494A1F (ô khác) |
| **Lệnh rã** | `OperationRequest(0x9B, &KItemObject, 5 /*UOC_ITEM_TAKE_WITH*/)` | 0x492686 |
| Core case 0x9B | 0x5C4A4F: uGenre phải = 5 (CGOG_ITEM), uId 1..0x1FF, item+0x6C0 != 1; nParam 5 → place 3, 6 → place 0x17 | |
| Gửi gói | KItemList(0x5E24B0)(idx, &{place,x,y}, 0xC6), gói 8 byte: [0]=0xC6, [5]=place, [6]=x, [7]=y | server nhận theo **vị trí**, không theo index |
| Cùng thân: 0x9D | nút "OpenStatus"(+0x6950), cờ `0x800000`, protocol 0xC9 | chưa rõ chức năng |

Hook: `GOI_DECOMPOSE_EQUIP = 0x9B` (GameDef.h), `KPlayer::DecomposeItem(int nItemIdx)` (JxCore.cpp) tự tìm ô (x,y) của item trong hành trang rồi gọi OperationRequest.

## 12. Bảng thuộc tính item MAGIC_ATTRIB (đợt 8)

- Nguồn: hàm khởi tạo 0x610C71 gán tên vào mảng con trỏ `0x1F18300 + 4*ID` (ID 0..345, 339 tên). `item_begin` = 27 khớp bản cũ.
- So với enum cũ: 27..183 **giữ nguyên ID** (chỉ đổi tên: 134 `damage2addmana_p`, 181 `dynamicmagicshield_v`; tên cũ giữ làm alias).
  Từ 184 lệch: chèn `changefeature1/2`, `stealfeature`, `addstealfeatureskill` → `lucky_v_partner` 184→188, `normal_end` 185→**308**.
  Thêm mới 189..307 (yan_*, anti_*, block_rate, enhancehit_*, five_elements_*, ...). Khoảng trống: 256-257, 275.
- Đã cập nhật: enum trong GameDef.h (27..308), bảng tên debug `s_szMagicAttribName` (VLDebugCollect.cpp), `AllAttribs` 42→101 (AutoParams.cpp).
- File .ita cũ vẫn dùng được (ID 27..183 không đổi).
- Dữ liệu pak đã unpack ở `I:\JX20\pak` (mã TCVN3): `settings\magicdesc.ini` → nhãn `AllAttribs` (120 mục, đúng chữ trong game);
  `settings\item\magicattriblevel.txt` (48 thuộc tính × 10 cấp, cột LEVEL của ID 111 gõ nhầm → lấy cấp theo thứ tự dòng)
  → `VLHook/MagicAttrib.cpp`: `MagicAttribName(type)`, `MagicAttribTier(type, value)` = cấp 1..10 (0 = không có trong bảng).

## 13. Còn phải dò

- Đang để 0 / NULL_PTR_ADD (code tự bỏ qua): TMC + TGL/TGP (danh sách bang), YSB + OCB (Dã Tẩu),
  PZS + PSC (chọn thưởng), STL + IST (bày bán), ITX, CMP, CHATROOM_LIST (điều khiển qua tin nhắn).
- LIFETIME_DYS/HRS, SERVERIP_ADD: không dùng ở VLTK_FREE_MODE.
- Bản 10_10 đã **bỏ Dã Tẩu và Bang hội** (sẽ thay bằng nhiệm vụ khác) → không dò YSB/OCB/PZS/PSC/TMC/TGL/TGP nữa.
- `AllowServers[]` trong VLMain.cpp nếu chạy trên server khác.

## Vẽ trong game (MapPath DEBUG)

| Macro | Giá trị | Neo |
|---|---|---|
| RPS_BASE_ADD | 0x01F182F0 | `g_pRepresent` (core): `mov ecx,[0x1F182F0]; mov edx,[ecx]; call [edx+0x4C]` với `push 0/1; push 3; push &m_Image; push 1` (KObj::Draw). UI dùng `g_pRepresentShell` = 0x009BC128 |
| RPS_VT_DRAWPRIM | +0x4C | DrawPrimitives(n, units, genre, bSinglePlane) — chỉ số 19 (source 18: 10_10 có thêm 1 hàm trước OutputText). Thời tiết: `push 1; push 1(RU_T_LINE); push buf; push 0x23`, bước 0x1C = sizeof(KRULine) |
| RPS_VT_OUTTEXT | +0x14 | OutputText(font, text, count, x, y, color, lineWidth, z, border): `push 0xFF000000; push 0xFFFF8001(TEXT_IN_SINGLE_PLANE_COORD); push 0; ...; push -1; push text; push 0xC` |
| DGS_FUNC_ADD | 0x005BAB50 | KCoreShell::DrawGameSpace: `cmp [g_pRepresent],0; je; call 0x6782A0; mov ecx,0x21A3F18 (g_ScenePlace); call 0x675C70 (Paint); mov ecx,[PLR]; add ecx,0xA878; jmp 0x5E4DB0 (DrawSelectInfo)` |
| DGS_JMP_ADD | 0x004C3AC6 | KUiGameSpace::PaintWindow 0x4C3AC0 = `mov ecx,[0x9BC130]; jmp 0x5BAB50` — lời gọi duy nhất. VLHook đổi `jmp` này sang HookDrawGameSpace (gọi hàm gốc rồi vẽ node) |

## Trap bản đồ (cổng đi bộ) — `KScenePlaceRegionC::m_TrapInfo`

| Macro | Giá trị | Neo |
|---|---|---|
| SCP_BASE_ADD | 0x021A3F18 | g_ScenePlace — DrawGameSpace: `mov ecx,0x21A3F18; call 0x675C70` (KScenePlaceC::Paint) |
| SCP_REGIONS_OFF / SCP_NUM_REGIONS | +0x31E64 / 15 | Paint: `lea edi,[esi+0x31E64]; mov ebx,0xF; ... call 0x712180` (PaintGround). Source: 9 region |
| SPR_STATUS_OFF | +0x04 | KScenePlaceRegionC::Load 0x7145E0: `cmp [esi+4],1` → `=2` → `=3` (REGION_S_STANDBY) |
| SPR_LEFTTOP_OFF | +0x08/+0x0C | Load: `[esi+8] = [esi+0x10] << 9`, `[esi+0xC] = [esi+0x14] << 10` (m_RegionIndex ở +0x10/+0x14), chuỗi `%sRegion_C.dat` |
| SPR_TRAP_OFF | +0x848 | LoadTrap 0x711F00: `cmp cY,0x20; cX+cNumCell-1 < 0x10; mov [esi+((cX+j)*32+cY)*4+0x848], uTrapId`. GetTrap(mpsX,mpsY) 0x712560: `(x-[ecx+8])>>5`, `(y-[ecx+0xC])>>5` |

ID trap = hash tên file script trap (KSPTrap.uTrapId), ô 32×32 Mps. Hàm 0x62E4A0 đọc Region_C.dat của cả map (lưới cho tìm đường của 10_10) — chưa dùng.

## Đăng nhập (màn hình bắt đầu → chọn máy chủ → nhập tài khoản)

| Tên | Địa chỉ | Kiểu | Ghi chú |
|---|---|---|---|
| KUiSelServer::m_pSelf | 0x00839A18 | pointer | `UiNewLogin\选服务器.ini`, OpenWindow 0x48BA00 (new 0x30B8), WndProc 0x48B4A0 |
| KUiSelServer members | +0x90C LeftList (14 khu vực đầu), +0xEBC RightList (khu vực thứ 15+), +0x146C IpList (máy chủ của khu vực đang chọn), +0x1A1C text, +0x1FB4 nút Đăng nhập, +0x2724 Hủy | – | **danh sách khu vực** `KLoginServer*` (0x24 byte: Title[32]+IP[4]) ở **+0x904** (GetServerRegionList 0x597E00), số lượng = LeftList+RightList count, khu vực đang chọn **+0x2EB8**; **máy chủ của khu vực** ở +0x908 (GetServerList 0x597F10), máy chủ đang chọn = IpList cursel **+0x191C** (chọn: IpList SetCurSel) |
| KUiSelServer: chọn khu vực | 0x0048AF80 | thiscall (0) | đọc LeftList cursel (+0xDBC) hoặc RightList cursel (+0x136C)+14 → +0x2EB8, nạp lại IpList. Chọn bằng tay = WndProc msg 0x691 |
| KWndList | +0x498 data, +0x49C count, +0x4A0 stride, +0x4B0 cursel | – | SetCurSel 0x45BD20 (1, báo 0x691 lên cha), bỏ chọn 0x483C30 (không báo), SetContent 0x45C790 (4), màu list nổi bật 0x483B10 (1) |
| KUiSelServer::OnLogin | 0x0048ACD0 | thiscall (0) | dùng [+0x908][+0x191C] → SetAccountServer, đóng cửa sổ, kết nối, mở KUiLogin 0x487CF0 |
| KUiLogin::m_pSelf | 0x008399FC | pointer | `UiNewLogin\登陆.ini`, new 0xEFE8, vtable 0x792724, WndProc 0x487DF0 |
| KUiLogin members | +0x588 Account, +0xA94 PassWord (KWndEdit), +0xFA0 Login, +0x1710 Cancel, +0x1E80 Remember, +0x25F0 Invisible, +0x2D60 VirtualKeyboard, **+0x80C0 tick đồng ý điều khoản** | – | chưa tick +0x80C0 thì OnLogin chỉ hiện các thông báo |
| KUiLogin::OnLogin | 0x004871D0 | thiscall (0) | GetInputInfo 0x486670 → g_LoginLogic 0x599570 → KUiConnectInfo 0x485D90; AccountLogin thật ở 0x4872B0 (MD5 + 0x598FD0) |
| KWndEdit::SetText / GetText | 0x00458A30 (1) / 0x00458BD0 (3) | thiscall | |
| KWndButton::CheckButton / IsButtonChecked | 0x00453AD0 (1) / 0x00453AC0 (0) | thiscall | cờ 0x200 ở +0x554 |
| g_LoginLogic (KLogin) | 0x009BCBB8 | object | m_Choices.Account = +0x720 (ACNT_ID_ADD 0x9BD2D8); AccountLogin 0x598FD0 |
| KUiSelPlayer::m_pSelf | 0x0084053C | pointer | `UiNewLogin\选游戏存档人物.ini`, OpenWindow 0x49D160, UpdateData 0x49D220 |
| KUiSelPlayer members | +0x35D4 Name[], +0x469C Level[], +0x5764 m_player[] (0x770/ô, 3 ô/trang), +0x7DB0 Ok, +0x8520 Cancel, +0x8C90 New, +0x9400 Del, +0x9B70 Transfer, +0xA2E0 Next, +0xAA50 Pre | – | **short +0xB964 m_nNumPlayer, +0xB966 m_nSelPlayer (ô trên trang), +0xB970 số nhân vật đã lật trang** |
| KUiSelPlayer::OnSelectPlayer | 0x0049C940 | thiscall (2) | (nSel ô trên trang, bPlaySound); m_nSelPlayer = nSel % m_nNumPlayer |
| KUiSelPlayer::OnEnterGame | 0x0049C210 | thiscall (0) | SelectRole(m_nSelPlayer + trang) → KUiConnectInfo(0xB, 0xC) + Hide |
| KLogin (g_LoginLogic) | +0x08 m_Status (SelectRole cần 7), short +0x12 m_nNumRole, +0x14 KRoleChiefInfo[] (0x24: Name[32], Gender, Attribute, short Level) | – | SelectRole 0x595E50 (1), GetRoleInfo 0x596170 (2), GetRoleCount 0x5962C0 (1) |

## Giảm cấu hình (dò 2026-10-14; macro LCF_* / SCP_WEATHER_OFF / SCP_DYNALIGHT_OFF trong VLHook/GameBase.h, dùng ở tab DEBUG "Giảm cấu hình")

Vòng lặp chính `KMyApp::GameLoop` = **0x00591640** (vtable KMyApp 0x7A5488, slot 3 gọi qua 0x591830; app global = **0x009BC4B8**).
Logic 18 nhịp/giây (`mul 0x12`, bộ đếm 0x9BC798), vòng logic gọi CoreShell::Breathe 0x5BAC00 + UiHeartBeat 0x44EAE0.
Sau đó **chỉ vẽ** khi `now - [0x9BC790] >= [0x815838]`; cuối hàm `Sleep(1)` tại 0x591818 (`push ebp`, ebp = 1).

| Tên | Địa chỉ | Cách dùng | Neo |
|---|---|---|---|
| Khoảng cách vẽ (ms/khung) | 0x00815838 | ghi `1000/FPS` lúc chạy (vd 200 = 5 FPS, 1000 = 1 FPS). Logic game không bị ảnh hưởng | GameInit 0x593A25: `config.ini [Client] PaintFps` (mặc định 60) → `1000/fps` |
| Thời điểm mất focus | app+0x50 = **0x009BC508** | ≠0 và đã quá 10 s (và [0x9BC148] ≠ 1) → GameLoop gọi 0x44C190 (chỉ RepresentBegin/End, **không vẽ gì**) thay cho UiPaint. Ghi 1 để tắt hình ngay; enginefree `KWin32App::MsgProc` 0x1003A6A0 tự ghi GetTickCount/0 khi đổi focus nên phải ghi lại định kỳ | GameLoop 0x591752: `GetTickCount - [ebx+0x50] >= 0x2710` |
| UiPaint | 0x0044E980 | cdecl(1). Vẽ toàn bộ (RepresentBegin vt+0x60, Wnd_RenderWindows 0x468360, RepresentEnd vt+0x64) | gọi duy nhất từ GameLoop 0x5917C9 |
| KScenePlaceC::Paint | 0x00675C70 | thiscall(0), ecx = 0x21A3F18 — vẽ đất/vật thể/thời tiết | xem DGS_FUNC_ADD |
| CoreDrawGameObj | 0x00672DB0 | cdecl(8) (genre, id, x, y, w, h, …) — vẽ NPC/obj; 3 lời gọi | switch genre `cmp eax,0x10004` |
| Sleep cuối vòng lặp | 0x00591818 | `55` (push ebp) → `6A nn` (push nn) để giảm CPU; 1 byte → 2 byte nên phải patch kèm lệnh sau hoặc hook | |

**GOI_OPTION_SETTING = 43 (0x2B)** ở 10_10 (OPR case 43 tại 0x5C5181). Gọi `OperationRequest(0x2B, uParam, nParam)`:

| uParam | Ý nghĩa | Đích |
|---|---|---|
| 0 | Perspective | g_pRepresent vt+0x74 SetOption(1, b) |
| 1 | Bóng động (DynaLight) | ScenePlace 0x6752D0 + SetOption(0, b) |
| 2 / 3 | Âm lượng nhạc / tiếng (0 = tắt) | KOption 0x220F9AC: 0x678E60 / 0x678E10 |
| 4 | Độ sáng | 0x678DF0 |
| 5 | Thời tiết (0 = tắt) | ScenePlace 0x6753C0 |
| 6 | **VisibleNum** — số người chơi hiển thị; **0 = ẩn người chơi khác** (kind 1/2, trừ mình) | ghi [0x81D15C] (mặc định 200); dùng ở 0x678D12 trước CoreDrawGameObj |
| 7 | **MissleOpen** — 1..999 = thay hiệu ứng skill bằng mẫu đơn giản (dòng n), khác = tắt | [0x2213C48] / cờ [0x2213C44], dùng ở 0x6B30C9 |
| 8 / 9 | **NpcTheSame** — chép tên mẫu NPC (chuỗi) để mọi NPC cùng hình | 0x5F08D0 / 0x5F08B0 (→ 0x1A3B4C0) |
| 10 | 0/1 → [0x2211174] (chưa rõ, tra bảng 0x820038) | |

Cửa sổ Tùy chọn (KUiOptions) lưu `[Options] VisibleNum, NpcTheSame, MissleOpen, QualityOpen, PaintFPS`
(hàm lưu 0x48D060). **QualityOpen** không qua OPR: 0x48D280 gọi `g_pRepresentShell(0x9BC128) vt+0x74 SetOption(6 hoặc 5, …)` tùy [0x9BC144].
