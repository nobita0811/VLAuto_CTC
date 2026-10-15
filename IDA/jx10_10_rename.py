# -*- coding: utf-8 -*-
"""
jx10_10_rename.py - IDAPython: dat ten ham / bien / struct / enum da do cho
gamecl_dump_SCY_10_10.exe (ImageBase 0x400000).

Nguon du lieu: VLAuto/Offsets_10_10.md + VLHook/GameBase.h (cac gia tri 10-10).

Cach dung: IDA -> File -> Script file... (Alt+F7) -> chon file nay.
Ho tro IDA 7.4+ (Python 3), IDA 8.x, IDA 9.x.
Nen luu snapshot IDB truoc khi chay (View -> Database snapshot manager).
Chay lai nhieu lan van an toan: ten trung se bo qua.
"""
from __future__ import print_function
import os
import re
import unicodedata

import idc
import idaapi
import ida_bytes
import ida_funcs
import ida_name
import ida_nalt
import ida_typeinf

# ============================================================================
# Tuy chon
# ============================================================================
EXPECTED_IMAGEBASE = 0x400000
OVERWRITE_USER_NAMES = False   # True: ghi de ca ten ban da tu dat trong IDB
APPLY_PROTOTYPES     = True    # dat kieu ham (cdecl/thiscall + so tham so)
APPLY_GLOBAL_TYPES   = True    # dat kieu cho bien toan cuc (KPlayer *, KNpc *...)
CREATE_STRUCTS       = True
IMPORT_ENUMS         = True    # doc enum tu VLAuto/GameDef.h
STRIP_DIACRITICS     = False   # True neu IDA hien comment tieng Viet bi loi font
GAMEDEF_PATH = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "GameDef.h")

ENUM_WHITELIST = [
    "NPCCAMP", "NPC_RELATION", "NPCKIND", "NPCCMD", "OBJKIND", "INVENTORY_ROOM",
    "ITEM_POSITION", "ITEMGENRE", "EQUIPDETAILTYPE", "MEDICINEDETAILTYPE", "ITEM_PART",
    "UIOBJECT_CONTAINER", "CORE_OBJECT_GENRE", "GAMEDATA_INDEX", "OPTIONS_LIST",
    "GAMEOPERATION_INDEX", "TEAMOPERATION_INDEX", "PROCESSPEOPLE_INDEX", "MAGIC_ATTRIB",
]

# ============================================================================
# Struct: (offset, kieu C, ten, so phan tu | None)
# Khoang trong tu dong chen "unsigned char _pad_XXXX[n]". Trung offset -> bo qua.
# ============================================================================
STRUCTS = [
    # --- phan tu nho ---
    ("KMagicAttrib16", 0x10, [
        (0x00, "int", "nAttribType", None),
        (0x04, "int", "nValue", 3),
    ]),
    ("KNpcSkillSlot", 0x30, [
        (0x00, "int", "nSkillId", None),    # FindIdx so [list+0x38+(i-1)*0x30]
        (0x04, "int", "nLevel_maybe", None),
    ]),
    ("KPlayerItemRec", 0x14, []),            # PlayerItem 10_10 (0x14), chua do field
    ("KInventory", 0x1C, [
        (0x04, "int", "m_nMoney", None),
    ]),
    ("KLoginServer", 0x24, [
        (0x00, "char", "Title", 32),
        (0x20, "unsigned char", "IP", 4),
    ]),
    ("KRoleChiefInfo", 0x24, [
        (0x00, "char", "Name", 32),
        (0x20, "unsigned char", "Gender", None),
        (0x21, "unsigned char", "Attribute", None),
        (0x22, "short", "Level", None),
    ]),

    # --- KItemList (o KPlayer+0x185C) ---
    ("KItemList", 0x4C5C + 26 * 0x1C, [      # so luong m_Room chua xac minh (<=26)
        (0x1C, "int", "m_nForeHand", None),
        (0x28, "int", "m_EquipItem", 17),    # 10_10: int[17] (cu: {idx,data}[17])
        (0x6C, "KPlayerItemRec", "m_Items", 483),
        (0x4BFC, "int", "m_UseIdx", None),
        (0x4C08, "int", "m_nListCurIdx", None),
        (0x4C0C, "int", "m_bLockOperation", None),
        (0x4C5C, "KInventory", "m_Room", 26),
    ]),

    # --- KNpc (0x12BD4) ---
    ("KNpc", 0x12BD4, [
        (0x00, "char *", "vn_pBuf401", None),
        (0x08, "unsigned int", "m_dwID", None),
        (0x0C, "int", "m_Index", None),
        (0x10, "int", "m_Node", None),
        (0x28, "int", "m_Level", None),
        (0x2C, "int", "m_Kind", None),
        (0x30, "int", "m_Series", None),
        (0x34, "int", "m_Height", None),
        (0x38, "unsigned char", "m_btRankId", None),
        (0x40, "int", "vn_Flag40", None),
        (0x44, "int", "vn_Flag44", None),
        (0x48, "int", "vn_Flag48", None),
        (0x4C, "int", "m_cGold", None),
        (0x94, "int", "m_PoisonState", None),
        (0xF4, "int", "m_Camp", None),
        (0xF8, "int", "m_CurrentCamp", None),
        (0xFC, "int", "m_Doing", None),
        (0x110, "void *", "m_pHeadNode", None),
        (0x114, "void *", "m_pTailNode", None),
        (0x118, "int", "m_nNodeCount", None),
        (0x11C, "int", "m_ActiveSkillID", None),
        (0x124, "unsigned char", "m_SkillList_hdr", 8),
        (0x12C, "KNpcSkillSlot", "m_Skills", 80),     # m_Skills[1] = 0x15C
        (0x1054, "int", "m_RegionIndex", None),
        (0x105C, "int", "m_CurrentLife", None),
        (0x1070, "int", "m_CurrentMana", None),
        (0x1078, "int", "m_CurrentStamina", None),
        (0x107C, "int", "m_CurrentStaminaMax", None),
        (0x1150, "int", "m_CurrentWalkSpeed", None),
        (0x1154, "int", "m_CurrentRunSpeed", None),
        (0x1170, "int", "m_CurrentAttackRadius", None),
        (0x13A0, "int", "m_MapX", None),
        (0x13A4, "int", "m_MapY", None),
        (0x13A8, "int", "m_MapZ", None),
        (0x13AC, "int", "m_OffX", None),
        (0x13B0, "int", "m_OffY", None),
        (0x1408, "char", "Name0", None),
        (0x1409, "char", "Name", 32),
        (0x1434, "int", "m_NpcSettingIdx", None),
        (0x1498, "int", "m_nPeopleIdx", None),
        (0x14A8, "int", "m_nObjectIdx", None),
        (0x16B8, "unsigned int", "m_SyncSignal", None),
        (0x16C0, "int", "m_sClientNpcID", 2),
        (0x16C8, "short", "m_RegX", None),           # = m_dwRegionID
        (0x16CA, "short", "m_RegY", None),
        (0x16CC, "int", "m_sSyncPos", None),
        (0x16E4, "int", "m_nPKFlag", None),
        (0x1810, "unsigned int", "m_dwTongNameID_unverified", None),
        (0x1814, "int", "m_bHaveStall_unverified", None),
        (0x1920, "int", "m_FightMode", None),
        (0x1960, "int", "m_Command", 4),             # cmd, p1, p2, p3
        (0x19C0, "int", "m_bRideHorse", None),
        (0x12B14, "int", "m_CurrentLifeMax_a", None), # game lay max(a, b)
        (0x12B18, "int", "m_CurrentLifeMax_b", None),
        (0x12B1C, "int", "m_CurrentManaMax_a", None),
        (0x12B20, "int", "m_CurrentManaMax_b", None),
    ]),

    # --- KPlayer (0xA878); [PLR] = &Player[0], nguoi choi = Player[1] ---
    ("KPlayer", 0xA878, [
        (0x28, "unsigned char", "m_nEntrustMode", None),
        (0x2C, "int", "m_nLeftSkillID", None),
        (0x1808, "int", "m_RunStatus", None),
        (0x1810, "int", "m_nSendMoveFrames", None),
        (0x1850, "unsigned int", "m_dwID", None),
        (0x1854, "int", "m_nIndex", None),
        (0x185C, "KItemList", "m_ItemList", None),
        (0x6794, "int", "m_nCurShop", None),          # m_BuyInfo.m_nBuyIdx
        (0x7220, "__int64", "m_nCurExp", None),
        (0x7228, "__int64", "m_nNextExp", None),
        (0x7258, "int", "m_cTeam_nFlag", None),
        (0x725C, "int", "m_cTeam_nFigure", None),
        (0x7800, "int", "m_cFaction_nCurFaction", None),
        (0x7A48, "unsigned char", "m_cPK", 0x58),
        (0x7AA0, "int", "m_cPK_nPKValue", None),
        (0x7CFC, "unsigned char", "m_cTong", 0x2C),
        (0x7D28, "int", "m_cTong_nFlag", None),
        (0x7D34, "unsigned int", "m_cTong_dwTongNameID", None),
        (0xA1A0, "int", "m_cTask", None),             # this cua GetSaveVal
    ]),

    # --- KItem (0x74C) ---
    ("KItem", 0x74C, [
        (0x08, "int", "m_nGenre_maybe", None),        # GetDesc: [item+8]==4
        (0x30, "char", "m_szName", 0x40),
        (0x228, "KMagicAttrib16", "m_aryAllAttrib", 24),
        (0x3AC, "int", "m_MagicLevel", None),
        (0x4E0, "unsigned int", "m_dwUniID", None),
        (0x4E4, "int", "m_nCurDur", None),
        (0x4E8, "int", "m_nMaxDur", None),
        (0x514, "unsigned char", "m_nItemPlus", None),  # ItemExParam[0]
        (0x515, "unsigned char", "m_ExParam1", None),
        (0x516, "unsigned short", "m_ExParam2", None),
        (0x518, "int", "m_nLeftUsageTime", None),
        (0x51C, "int", "m_nExpiredTime", None),
        (0x520, "int", "m_nLockCount", None),           # -1 vinh vien, >0 co han
        (0x6C0, "int", "m_nNoDecompose_maybe", None),   # OPR 0x9B: != 1
    ]),

    # --- KObj (0x3F4) ---
    ("KObj", 0x3F4, [
        (0x08, "int", "m_nKind", None),
        (0x0C, "int", "m_nIndex", None),
        (0x10, "short", "m_RegX", None),
        (0x12, "short", "m_RegY", None),
        (0x14, "int", "m_nSubWorldID", None),
        (0x18, "int", "m_nRegionIdx", None),
        (0x1C, "int", "m_nMapX", None),
        (0x20, "int", "m_nMapY", None),
        (0x24, "int", "m_nOffX", None),
        (0x28, "int", "m_nOffY", None),
        (0x60, "char", "m_szName", 0x24),
        (0x84, "int", "m_nColorID", None),
        (0x88, "int", "m_nItemDataID", None),
        (0x8C, "int", "m_nItemWidth", None),
        (0x90, "int", "m_nItemHeight", None),
        (0x94, "int", "m_nMoneyNum", None),
        (0x2B0, "int", "m_nDropState", None),
        (0x384, "unsigned int", "m_dwNameColor", None),
    ]),

    # --- g_SkillManager.m_SkillInfo[3000] (0x4FC) ---
    ("KSkillInfo", 0x4FC, [
        (0x00, "int", "TabFileRowId", None),
        (0x04, "int", "SkillStyle", None),
        (0x08, "int", "MaxLevel", None),
        (0x0C, "char", "SkillName", 32),
        (0x2C, "unsigned int", "SkillId", None),
        (0x38, "int", "IsPhysical", None),
        (0x40, "int", "IsAura", None),
        (0x44, "int", "IsMelee", None),
        (0x50, "int", "AttackRadius", None),
        (0x54, "int", "WaitTime", None),
        (0x60, "int", "Series", None),
        (0x78, "int", "CharClass", None),
        (0xA0, "int", "CostValue", None),
        (0xA8, "int", "TimePerCast", None),
        (0xAC, "int", "TimePerCastOnHorse", None),
        (0xB4, "int", "EqtLimit", None),
        (0xB8, "int", "HorseLimit", None),
        (0xC4, "int", "ChildSkillId", None),
        (0x11C, "char", "SkillDesc", 0x258),
        (0x374, "char", "SkillIcon", 0x80),
    ]),

    ("KTeam", 0x154, [
        (0x08, "int", "m_nCaptain", None),          # TEAM_ADDRESS
        (0x28, "int", "m_nMemNum", None),
        (0x30, "int", "m_nMemLevel", 8),
        (0x50, "char", "m_szMemName", 8 * 32),
        (0x150, "int", "m_nServerID", None),
    ]),

    ("KLogin", 0x740, [
        (0x08, "int", "m_Status", None),            # SelectRole can 7
        (0x12, "short", "m_nNumRole", None),
        (0x14, "KRoleChiefInfo", "m_RoleList", 3),
        (0x720, "char", "m_Choices_Account", 32),  # dao bit (ACNT_ID_ADD)
    ]),

    ("KScenePlaceRegionC", 0x1048, [
        (0x04, "int", "m_Status", None),            # 3 = REGION_S_STANDBY
        (0x08, "int", "m_LeftTopX", None),
        (0x0C, "int", "m_LeftTopY", None),
        (0x10, "int", "m_RegionIndexX", None),
        (0x14, "int", "m_RegionIndexY", None),
        (0x848, "unsigned int", "m_TrapInfo", 16 * 32),
    ]),

    # --- UI (cac field "wnd_*" chi la moc dau cua control con) ---
    ("KWndMessageListBox", 0x4A4, [
        (0x498, "void *", "pMessages", None),
        (0x49C, "int", "nNumMessage", None),
        (0x4A0, "int", "nCapability", None),
    ]),
    ("KUiMsgSel", 0x2380, [
        (0x584, "int", "wnd_Select", None),
        (0xA18, "KWndMessageListBox", "m_List", None),   # m_pMessages 0xEB0, num 0xEB4
        (0x1BFC, "int", "wnd_InfoText", None),
        (0x2174, "char", "m_szInfoText", 0x200),
    ]),
    ("KUiMsgSel2", 0x4DCC, [
        (0x584, "int", "wnd_MsgScroll", None),
        (0x1268, "int", "wnd_Select", None),
        (0x16FC, "KWndMessageListBox", "m_List", None),  # 0x1B94 / 0x1B98
        (0x28E0, "int", "wnd_MessageList", None),
        (0x2D74, "void *", "m_MessageList_pMessages", None),
        (0x2D7C, "int", "m_MessageList_nNum", None),
        (0x2DD0, "int", "wnd_Image", None),
        (0x3324, "int", "wnd_Text", None),
    ]),
    ("KUiShop", 0x4D6C, [
        (0x584, "void *", "m_pObjsList", None),
        (0x5A4, "int", "m_nObjCount", None),
        (0x5A8, "int", "m_nPageCount", None),
        (0x5AC, "int", "m_nCurrentPage", None),
    ]),
    ("KUiInformation", None, [
        (0x584, "int", "wnd_Info", None),
        (0xAFC, "char", "m_szInfoText", 0x200),
        (0x15EC, "void *", "m_pCallerWnd", None),   # KUiInformation::Hide -> WndProc(0x501)
    ]),
    ("KUiPlayerBar", None, [
        (0xCC4, "int", "wnd_GShopButton", None),
        # KWndObjectBox[9] buoc 0x4F0, m_Object.uGenre o +0x4B0 -> o 3 = 0x3260 (m_nHK3Gengre)
        (0x23D0, "unsigned char", "m_ImmediaItem", 9 * 0x4F0),
    ]),
    ("KUiItem", 0x7C98, [
        (0x6950, "int", "wnd_OpenStatus", None),
        (0x7C94, "int", "wnd_DecomposeEquip", None),
    ]),
    ("KUiInit", 0x2368, [
        (0x5A4, "int", "wnd_EnterGame", None),
        (0x1484, "int", "wnd_Options", None),
        (0x2364, "int", "wnd_ExitGame", None),
    ]),
    ("KUiSelServer", 0x30B8, [
        (0x904, "KLoginServer *", "m_pRegionList", None),
        (0x908, "KLoginServer *", "m_pServerList", None),
        (0x90C, "int", "wnd_LeftList", None),
        (0xDBC, "int", "m_LeftList_nCurSel", None),
        (0xEBC, "int", "wnd_RightList", None),
        (0x136C, "int", "m_RightList_nCurSel", None),
        (0x146C, "int", "wnd_IpList", None),
        (0x191C, "int", "m_IpList_nCurSel", None),
        (0x1A1C, "int", "wnd_Text", None),
        (0x1FB4, "int", "wnd_Login", None),
        (0x2724, "int", "wnd_Cancel", None),
        (0x2EB8, "int", "m_nCurRegion", None),
    ]),
    ("KUiLogin", 0xEFE8, [
        (0x588, "int", "wnd_Account", None),
        (0xA94, "int", "wnd_PassWord", None),
        (0xFA0, "int", "wnd_Login", None),
        (0x1710, "int", "wnd_Cancel", None),
        (0x1E80, "int", "wnd_Remember", None),
        (0x25F0, "int", "wnd_Invisible", None),
        (0x2D60, "int", "wnd_VirtualKeyboard", None),
        (0x80C0, "int", "wnd_AgreeTerms", None),
    ]),
    ("KUiSelPlayer", 0xB974, [
        (0x35D4, "int", "wnd_Name", None),
        (0x469C, "int", "wnd_Level", None),
        (0x5764, "unsigned char", "m_player", 3 * 0x770),
        (0x7DB0, "int", "wnd_Ok", None),
        (0x8520, "int", "wnd_Cancel", None),
        (0x8C90, "int", "wnd_New", None),
        (0x9400, "int", "wnd_Del", None),
        (0x9B70, "int", "wnd_Transfer", None),
        (0xA2E0, "int", "wnd_Next", None),
        (0xAA50, "int", "wnd_Pre", None),
        (0xB964, "short", "m_nNumPlayer", None),
        (0xB966, "short", "m_nSelPlayer", None),
        (0xB970, "int", "m_nPageStart", None),
    ]),
]

# ============================================================================
# Ham: (ea, ten, kieu, comment)
#   kieu: None | "c:N" (cdecl N tham so) | "t:N[:Lop]" (thiscall) | prototype C day du
#   ten None = chi them comment. Dia chi "~" (gan dung) dung FUNCS_APPROX.
# ============================================================================
FUNCS = [
    # --- macro trong GameBase.h ---
    (0x004360B0, "ExecuteScript", "c:1", "EXS_FUNC_ADD"),
    (0x0044C560, "UiCloseWndsInGame", "void __cdecl f(int bAll);", "CLA_FUNC_ADD"),
    (0x005B9F30, "KCoreShell__GetGameData",
        "int __thiscall f(void *this, GAMEDATA_INDEX uDataId, unsigned int uParam, int nParam);", "GGD_FUNC_ADD"),
    (0x005C3820, "KCoreShell__OperationRequest",
        "int __thiscall f(void *this, GAMEOPERATION_INDEX uOper, unsigned int uParam, int nParam);",
        "OPR_FUNC_ADD. case 1 EXIT_GAME, 5 MONEY_INOUT_STORE_BOX, 0x2B OPTION_SETTING (0x5C5181), 0x9B DECOMPOSE (0x5C4A4F)"),
    (0x005C2050, "KCoreShell__TongOperation", "t:3", "TNP_FUNC_ADD (GetTongNameID, switch 0..0x3D)"),
    (0x005BB090, "KCoreShell__TeamOperation",
        "int __thiscall f(void *this, TEAMOPERATION_INDEX uOper, unsigned int uParam, int nParam);", "TOP_FUNC_ADD"),
    (0x005BB3C0, "KCoreShell__ProcessGameSecurity", "t:3", "PGS_FUNC_ADD (switch 0..7, khoa ruong)"),
    (0x005BAAF0, "KCoreShell__GetNPCRelation", "t:1", "GRL_FUNC_ADD = NpcSet.GetRelation(me, n)"),
    (0x005BAB50, "KCoreShell__DrawGameSpace", "t:0", "DGS_FUNC_ADD: ScenePlace.Paint + DrawSelectInfo"),
    (0x005BAC00, "KCoreShell__Breathe", None, "goi trong vong logic GameLoop"),
    (0x005F3B30, "KNpc__MoveTo", "t:3:KNpc", "MVT_FUNC_ADD: SendCommand(do_walk/do_run) + SendClientCmdWalk/Run"),
    (0x005ECED0, "SendClientCmdSkill", "void __cdecl f(int nSkillId, int nX, int nY);", "SKL_FUNC_ADD, goi 0x4D dai 0x11"),
    (0x005E2530, "KPlayer__PickUpObj", "t:1:KPlayer", "PKO_FUNC_ADD: CheckTrading, kind 3/4"),
    (0x0061C820, "KPlayerTask__GetSaveVal", "t:1", "GTV_BASE_ADD, n <= 0x176F; this = KPlayer+0xA1A0"),
    (0x006B9390, "KBuySell__GetItem", "t:1", "BGI_FUNC_ADD"),
    (0x006B93E0, "KBuySell__GetItemIndex", "int __thiscall f(void *this, int nBuyIdx, int nID);", "BGX_FUNC_ADD"),
    (0x0051EB50, "KUiMsgSel__OnClickMsg", "t:1:KUiMsgSel", "OMS_FUNC_ADD (dung this+0xA18)"),
    (0x00509E40, "KUiMsgSel2__OnClickMsg", "t:1:KUiMsgSel2", "OMS2_FUNC_ADD: OperationRequest(9,0,n) + CloseWindow(1)"),
    (0x004B9340, "KUiMsgCentrePad__SystemMessageArrival",
        "void __cdecl f(const char *pMsg, int nLen, int bChannelBox);",
        "SMA_FUNC_ADD: arg3=0 -> m_Sys(+0x7AD4); m_pSelf=NULL -> hang cho 0x4B5B10"),
    (0x00498F80, "KUiStoreBox__OpenWindow", "c:0", "SBO_FUNC_ADD (new 0x4980 -> m_pSelf 0x84051C)"),
    (0x004BF2F0, "KUiShop__OpenWindow", "c:0", "SHO_FUNC_ADD (new 0x4D6C -> m_pSelf 0x8478E0)"),
    (0x00478440, "KUiPlayerBar__WndProc", "t:3:KUiPlayerBar", "UWP_FUNC_ADD = vtable 0x791784[4]"),
    (0x004768D0, "KUiPlayerBar__OnSendSomeoneMessage",
        "int __cdecl f(const char *szName, const char *pMsg, int nLen);", "OSM_FUNC_ADD, goi 0xFB/0x20"),
    (0x0048FFB0, "KUiInit__OnClickButton", "t:1:KUiInit", "IOC_FUNC_ADD; m_ExitGame = this+0x2364"),
    (0x004AE490, "KUiInformation__Hide", "t:1:KUiInformation", "IH1_FUNC_ADD"),
    (0x004AE940, "KUiInformation2__Hide", "t:0", "IH2_FUNC_ADD: caller==-1 -> OperationRequest(8,0,0)"),
    (0x004C4530, "ProcessPeople", "c:2", "PCP_FUNC_ADD: case 3 JOINTEAM, case 10 REVENGE -> OPR 0x2E"),
    (0x004C5390, "KUiSysMsgCentre__DeleteMsgInHeap", "t:2", "SMD_FUNC_ADD; m_MsgHeap = this+0x4C8 (12 byte)"),
    (0x004C7C70, "MiniMap_SetFlag", "void __cdecl f(int x, int y, int bWorld);",
        "SMF_FUNC_ADD: toa do tuyet doi; ghi FLAG_X(x>>4)/FLAG_Y(y>>5)"),
    (0x0063A500, "KItem__GetRepairPrice", "t:0:KItem", "IGR_FUNC_ADD"),
    (0x005E4C80, "KPlayer__DialogNpc", "t:1:KPlayer", "DNPC_FUNC_ADD"),
    (0x005DEA20, "KPlayer__ApplyAddBaseAttribute", "t:2:KPlayer", "PAA_FUNC_ADD: attr<=3, goi 0x57 dai 0xE"),

    # --- ham phu (khong co macro) ---
    (0x005F1A20, "KNpc__SendCommand", None, "ghi m_Command 0x1960..0x196C"),
    (0x005FA900, "KNpc__SetFightMode", None, None),
    (0x005FB420, "KNpc__KNpc", None, "ctor: [esi]=new char[0x401], KList tai +0x110"),
    (0x0066F340, "KNpcSet__SearchID", None, None),
    (0x0066FCF0, "KNpcSet__GetRelation", None, None),
    (0x005DED00, "KPlayer__SetLeftSkill", None, "dung npc+0x124 (m_SkillList)"),
    (0x005E4CE0, "KPlayer__CheckObject", None, "xoa m_nObjectIdx (0x14A8)"),
    (0x005E4DB0, "KPlayer__DrawSelectInfo", None, None),
    (0x005ED2B0, "SendClientCmdWalk", None, None),
    (0x005ED230, "SendClientCmdRun", None, None),
    (0x0065ECF0, "s2c_SyncNpc", None, None),
    (0x0065FE00, "s2c_SyncPlayer", None, None),
    (0x0065EAF0, "s2c_SyncCurNormalData", None, None),
    (0x0065C110, "s2c_SyncItemLockCount", None, "{itemID, value} -> KItem+0x520"),
    (0x006123D0, "KItemList__GetFirstItem", None, None),
    (0x005E24B0, "KItemList__SendItemPosCmd", None, "(idx, &{place,x,y}, protocol) - ra do: 0xC6"),
    (0x006ADC70, "KPlayerPK__SetPKValue", None, None),
    (0x00621D10, "KPlayerTong__GetTongNameID", None, "m_nFlag +0x2C, m_dwTongNameID +0x38"),
    (0x00683B70, "KPlayerFaction__Init", None, "m_nCurFaction = -1"),
    (0x00634650, "KItem__GetTotalMagicLevel", None, None),
    (0x006346D0, "KItem__CanBeRepaired", None, "10_10 khong xet +N"),
    (0x006390E0, "KItem__GetDesc", None, "\"%s+%d%s\" voi byte [item+0x514]"),
    (0x00612F00, "KItem__GetItemPlus", None, None),
    (0x0063D580, "KItem__SetItemPlus", None, None),
    (0x00631E20, "KItem__GetExParam2", None, None),
    (0x00631E30, "KItem__SetExParam2", None, None),
    (0x00634900, "KItem__ExParam1_634900", None, "ItemExParam[1] (0x515)"),
    (0x0072A5B0, "Lua_ITEM_GetItemExParam", None, None),
    (0x0072A6E0, "Lua_ITEM_GetLeftUsageTime", None, None),
    (0x0072A780, "Lua_ITEM_GetExpiredTime", None, None),
    (0x006826F0, "KObjSet__AddData", None, None),
    (0x00683490, "KObjSet__ClientAdd", None, None),
    (0x0060D740, "KSkillManager__Init", None, "ecx = 0x1AB4A10"),
    (0x006F7DB0, "KSkill__LoadSkillInfo", None, "nap 1 dong Skills.txt, ebp = entry+0xC"),
    (0x0045E110, "KWndMessageListBox__GetOneMessage", None, None),
    (0x0045F0D0, "KWndMessageListBox__AddOneMessage", None, None),
    (0x00464C60, "KWndRichMessageListBox__AddOneMessage", None, "chu o msg+0x10D8"),
    (0x0046B540, "KWndText512__KWndText512", None, "buf +0x578, 0x1FF"),
    (0x00460000, "KWndObjectBox__GetObject", None, None),
    (0x00460070, "KWndObjectBox__HoldObject", None, None),
    (0x00458A30, "KWndEdit__SetText", "t:1", None),
    (0x00458BD0, "KWndEdit__GetText", "t:3", None),
    (0x00453AD0, "KWndButton__CheckButton", "t:1", "co 0x200 o +0x554"),
    (0x00453AC0, "KWndButton__IsButtonChecked", "t:0", None),
    (0x0045BD20, "KWndList__SetCurSel", "t:1", "bao 0x691 len cha"),
    (0x00483C30, "KWndList__ClearSel", None, "bo chon, khong bao"),
    (0x0045C790, "KWndList__SetContent", "t:4", None),
    (0x00483B10, "KWndList__SetHighlight", "t:1", None),
    (0x0051EA60, "KUiMsgSel__LoadScheme", None, None),
    (0x0051EBC0, "KUiMsgSel__GetMsgCount", None, "= [ecx+0xEB4]"),
    (0x0051F000, "KUiMsgSel__KUiMsgSel", None, None),
    (0x00509590, "KUiMsgSel2__LoadScheme", None, None),
    (0x004BEDA0, "KUiShop__UpdateData", None, "GDI 0xBB9, phan tu 0x1C"),
    (0x004BE960, "KUiShop__SetPage", None, None),
    (0x004AE100, "KUiInformation__LoadScheme", None, None),
    (0x004AE7D0, "KUiInformation2__LoadScheme", None, None),
    (0x00497EF0, "KUiStoreBox__GetIfVisible", None, None),
    (0x00491D50, "KUiItem__GetIfVisible", None, None),
    (0x00492990, "KUiItem__OnDecomposeButton", None, "bat/tat con tro 'Giai' (co 0x400000)"),
    (0x00492620, "KUiItem__OnSpecialClickItem", None, "(pObj, 0x9B, 0x400000) -> OperationRequest(0x9B, &obj, 5)"),
    (0x0044B7A0, "Ui_SetCursorState", None, "co trang thai chuot 0x8329C8"),
    (0x004486B0, "Ui_GetCursorState", None, None),
    (0x0048F870, "KUiInit__CloseWindow", None, None),
    (0x00490340, "KUiInit__OpenWindow", None, "`开始.ini`"),
    (0x004C3AC0, "KUiGameSpace__PaintWindow", None, "mov ecx,[CORE]; jmp DrawGameSpace (DGS_JMP_ADD 0x4C3AC6)"),
    (0x00675C70, "KScenePlaceC__Paint", "t:0", "ecx = g_ScenePlace"),
    (0x00712180, "KScenePlaceRegionC__PaintGround", None, None),
    (0x007145E0, "KScenePlaceRegionC__Load", None, "%sRegion_C.dat"),
    (0x00711F00, "KScenePlaceRegionC__LoadTrap", None, None),
    (0x00712560, "KScenePlaceRegionC__GetTrap", None, "(mpsX, mpsY)"),
    (0x0062E4A0, "Map_LoadAllRegionC", None, "doc Region_C.dat ca map (luoi tim duong 10_10)"),
    (0x00672DB0, "CoreDrawGameObj", "c:8", "switch genre (0x10004...)"),
    (0x006752D0, "KScenePlaceC__EnableDynamicLights", None, "OPTION 1"),
    (0x006753C0, "KScenePlaceC__EnableWeather", None, "OPTION 5"),
    (0x00678E60, "KOption__SetMusicVolume", None, None),
    (0x00678E10, "KOption__SetSoundVolume", None, None),
    (0x00678DF0, "KOption__SetBrightness", None, None),
    (0x005F08D0, "NpcTheSame_Set8", None, "OPTION 8"),
    (0x005F08B0, "NpcTheSame_Set9", None, "OPTION 9"),
    (0x0048D060, "KUiOptions__SaveSetting", None, "[Options] VisibleNum, NpcTheSame, MissleOpen, QualityOpen, PaintFPS"),
    (0x0048D280, "KUiOptions__SetQuality", None, "g_pRepresentShell vt+0x74 SetOption(6/5)"),
    (0x00591640, "KMyApp__GameLoop", None, "logic 18 nhip/s; ve khi now-[0x9BC790] >= [0x815838]; Sleep(1) 0x591818"),
    (0x0044EAE0, "UiHeartBeat", None, None),
    (0x0044E980, "UiPaint", "c:1", None),
    (0x0044C190, "UiPaintBlank", None, "chi RepresentBegin/End khi mat focus > 10s"),
    (0x00468360, "Wnd_RenderWindows", None, None),

    # --- dang nhap ---
    (0x0048BA00, "KUiSelServer__OpenWindow", None, "new 0x30B8"),
    (0x0048B4A0, "KUiSelServer__WndProc", None, "msg 0x691 = chon khu vuc"),
    (0x0048AF80, "KUiSelServer__OnSelRegion", "t:0:KUiSelServer", None),
    (0x0048ACD0, "KUiSelServer__OnLogin", "t:0:KUiSelServer", None),
    (0x00597E00, "KLogin__GetServerRegionList", None, None),
    (0x00597F10, "KLogin__GetServerList", None, None),
    (0x00487CF0, "KUiLogin__OpenWindow", None, "new 0xEFE8"),
    (0x00487DF0, "KUiLogin__WndProc", None, None),
    (0x004871D0, "KUiLogin__OnLogin", "t:0:KUiLogin", None),
    (0x004872B0, "KUiLogin__DoAccountLogin", None, "MD5 + KLogin__AccountLogin"),
    (0x00486670, "KUiLogin__GetInputInfo", None, None),
    (0x00599570, None, None, "g_LoginLogic (goi tu KUiLogin::OnLogin)"),
    (0x00485D90, "KUiConnectInfo__OpenWindow", None, None),
    (0x00598FD0, "KLogin__AccountLogin", None, None),
    (0x0049D160, "KUiSelPlayer__OpenWindow", None, None),
    (0x0049D220, "KUiSelPlayer__UpdateData", None, None),
    (0x0049C940, "KUiSelPlayer__OnSelectPlayer",
        "void __thiscall f(KUiSelPlayer *this, int nSel, int bPlaySound);", None),
    (0x0049C210, "KUiSelPlayer__OnEnterGame", "t:0:KUiSelPlayer", None),
    (0x00595E50, "KLogin__SelectRole", "t:1:KLogin", None),
    (0x00596170, "KLogin__GetRoleInfo", "t:2:KLogin", None),
    (0x005962C0, "KLogin__GetRoleCount", "t:1:KLogin", None),
]

FUNCS_APPROX = [
    (0x005F7600, "KNpc__Init", "~ dia chi gan dung: KNpc::Init"),
]

# ============================================================================
# Bien toan cuc: (ea, ten, kieu C | None, comment)
# ============================================================================
GLOBALS = [
    (0x009C0474, "Player", "KPlayer *", "PLR_BASE_ADD (Player[1] = minh)"),
    (0x01A3B4B4, "Npc", "KNpc *", "NPC_BASE_ADD, new KNpc[0x320] tai 0x5CCAD6"),
    (0x021A1898, "Object", "KObj *", "OBJ_BASE_ADD, KObj[0x100]"),
    (0x01F18E14, "Item", "KItem *", "ITM_BASE_ADD, KItem[0x200]"),
    (0x021D6840, "g_nMapID", "int", "MAP_ID_ADD"),
    (0x021D6820, "g_szMapName", "char[32]", "MAP_NAME_ADD"),
    (0x009BC130, "g_pCoreShell", "void *", "CORE_BASE_ADD"),
    (0x009C08A8, "SubWorld", "void *", "WLD_BASE_ADD; [SubWorld]+0x50 = m_dwCurrentTime"),
    (0x021A18A8, "NpcSet", None, "NSET_BASE_ADD"),
    (0x024E7750, "BuySell", None, "BSL_BASE_ADD"),
    (0x01AB4A10, "g_SkillManager", "KSkillInfo[3000]", "SKL_BASE_ADD = g_SkillManager.m_SkillInfo"),
    (0x01E5B330, "g_SkillManager_m_pOrdinSkill", None, "= SKL_BASE_ADD + 0x3A6920"),
    (0x01F18A78, "g_Team", "KTeam", "TEAM_ADDRESS = &g_Team.m_nCaptain (0x1F18A80)"),
    (0x024F1E18, "PlayerSet", None, None),
    (0x024F1E4C, "PlayerSet_m_LevelExp", "int[260]", "LEVEL_EXP_TB, level_exp.txt"),
    (0x024F4348, "ItemSet", None, "nPriceScale +0xE8 / nMagicScale +0xEC"),
    (0x01F18300, "g_MagicAttribNames", "char *[346]", "ten MAGIC_ATTRIB (khoi tao 0x610C71)"),
    (0x009BCBB8, "g_LoginLogic", "KLogin", "m_Choices.Account +0x720 = ACNT_ID_ADD 0x9BD2D8 (dao bit)"),
    (0x01F182F0, "g_pRepresent", "void *", "RPS_BASE_ADD; vt+0x14 OutputText, +0x4C DrawPrimitives, +0x74 SetOption"),
    (0x009BC128, "g_pRepresentShell", "void *", "UI representshell"),
    (0x021A3F18, "g_ScenePlace", None, "KScenePlaceC; regions +0x31E64 (15)"),
    (0x0220F9AC, "g_Option", None, "KOption"),
    (0x00811D24, "g_MiniMapFlagX", "int", "FLAG_X_OFFST (x>>4, -1 = khong co)"),
    (0x00811D28, "g_MiniMapFlagY", "int", "FLAG_Y_OFFST (y>>5)"),
    (0x008329C8, "g_UiCursorState", "int", "0x400000 = 'Giai', 0x800000 = OpenStatus"),
    # m_pSelf cua UI
    (0x008C13D8, "KUiMsgSel_m_pSelf", "KUiMsgSel *", "MSG_BASE_ADD `滚动选择界面.ini`"),
    (0x008C1368, "KUiMsgSel2_m_pSelf", "KUiMsgSel2 *", "MSG2_BASE_ADD `npc描述界面.ini`"),
    (0x00839890, "KUiPlayerBar_m_pSelf", "KUiPlayerBar *", "UPB_BASE_ADD `玩家信息主界面.ini`"),
    (0x0084051C, "KUiStoreBox_m_pSelf", "void *", "SBO_BASE_ADD `储物箱.ini`"),
    (0x008478A8, "KUiMsgCentrePad_m_pSelf", "void *", "MCP_BASE_ADD `消息集合面板_左.ini`"),
    (0x00847EF4, "KUiSysMsgCentre_m_pSelf", "void *", "SMC_BASE_ADD `系统消息.ini`"),
    (0x008478E0, "KUiShop_m_pSelf", "KUiShop *", "SHOP_BASE_ADD `npc买卖界面.ini`"),
    (0x008478DC, "KUiGShop_m_pSelf", "void *", "`奇珍阁界面.ini`"),
    (0x00847EF8, "KUiMiniMap_m_pSelf", "void *", "MMP_BASE_ADD"),
    (0x00840460, "KUiInit_m_pSelf", "KUiInit *", "UIN_BASE_ADD `开始.ini`"),
    (0x00840474, "KUiItem_m_pSelf", "KUiItem *", None),
    (0x00839A18, "KUiSelServer_m_pSelf", "KUiSelServer *", "`UiNewLogin\\选服务器.ini`"),
    (0x008399FC, "KUiLogin_m_pSelf", "KUiLogin *", "`UiNewLogin\\登陆.ini`"),
    (0x0084053C, "KUiSelPlayer_m_pSelf", "KUiSelPlayer *", "`选游戏存档人物.ini`"),
    (0x00842460, "g_UiInformation", "KUiInformation", "IF1_BASE_ADD `提示.ini`"),
    (0x00843B68, "g_UiInformation2", "KUiInformation", "IF2_BASE_ADD `提示2.ini`"),
    (0x008C1390, "KUiPrizeSel_m_pSelf_maybe", None, "? `奖励界面.ini` (0x6180)"),
    (0x008C139C, "KUiYeSouBox_m_pSelf_maybe", None, "? `给予界面.ini` (0xB7CC)"),
    (0x008C0A38, "KUiPlayerStall_m_pSelf_maybe", None, "? `摆摊物品.ini`"),
    (0x008C0990, "KUiTongList_m_pSelf_maybe", None, "? `帮会列表窗口.ini`"),
    # vtable
    (0x00791784, "KUiPlayerBar_vftable", None, "[4] = WndProc 0x478440"),
    (0x00792724, "KUiLogin_vftable", None, None),
    (0x007A5488, "KMyApp_vftable", None, "slot 3 goi qua 0x591830"),
    # app / ve
    (0x009BC4B8, "g_App", None, "KMyApp"),
    (0x009BC508, "g_App_dwLostFocusTime", "unsigned int", "app+0x50; != 0 va > 10s -> UiPaintBlank. Ghi 1 de tat hinh"),
    (0x009BC790, "g_dwLastPaintTime", "unsigned int", None),
    (0x009BC798, "g_nLogicFrame", "unsigned int", "bo dem logic (18/s)"),
    (0x00815838, "g_nPaintInterval", "int", "ms/khung = 1000/PaintFps (config.ini [Client])"),
    (0x0081D15C, "g_nVisibleNum", "int", "OPTION 6: so nguoi choi hien thi (mac dinh 200, 0 = an)"),
    (0x02213C44, "g_bMissleOpen", "int", "OPTION 7"),
    (0x02213C48, "g_nMissleOpenRow", "int", "OPTION 7 (1..999)"),
    (0x01A3B4C0, "g_szNpcTheSame", None, "OPTION 8/9"),
    (0x02211174, "g_nOption10", "int", "OPTION 10 (chua ro)"),
    (0x009BC144, "g_nQualityMode", "int", "QualityOpen chon SetOption 6/5"),
]

# ============================================================================
# Comment tai lenh (neo bang chung)
# ============================================================================
COMMENTS = [
    (0x005CCAD6, "Npc = new KNpc[0x320]"),
    (0x006643B2, "RT_INFO: m_nCurExp 0x7220 / m_nNextExp 0x7228"),
    (0x0051B150, "lay item dang cam (m_nForeHand) tao doi tuong keo-tha"),
    (0x006EB658, "AI: SendClientCmdWalk -> DialogNpc"),
    (0x004C5822, "TEAM_INVITE_RESPONSE 0x0D: nguoi moi o msg+0x108"),
    (0x005099BD, "[ecx+0x1B98] = KUiMsgSel2 m_nNumMessage"),
    (0x004746E5, "\"Item_%d\" m_ImmediaItem[9] tai 0x23D0, buoc 0x4F0"),
    (0x005C8D95, "KSkillManager::Init(ecx = g_SkillManager)"),
    (0x005D2685, "KSkillManager::Init(ecx = g_SkillManager)"),
    (0x004923B3, "KUiItem::Init - nut DecomposeEquip +0x7C94"),
    (0x0049331D, "click DecomposeEquip -> 0x492990"),
    (0x00492EE0, "click o hanh trang khi dang 'Giai'"),
    (0x00494A1F, "click o khac khi dang 'Giai'"),
    (0x00492686, "OperationRequest(0x9B, &KItemObject, 5 UOC_ITEM_TAKE_WITH)"),
    (0x005C4A4F, "OPR case 0x9B: uGenre=5, uId 1..0x1FF, item+0x6C0 != 1; nParam 5->place 3, 6->place 0x17; goi 0xC6"),
    (0x005C5181, "OPR case 0x2B GOI_OPTION_SETTING"),
    (0x0065CF6E, "s2c sync item: ExParam1 (goi +0x2F)"),
    (0x00610C71, "khoi tao ten MAGIC_ATTRIB -> g_MagicAttribNames"),
    (0x004B5B10, "SystemMessageArrival: hang cho khi m_pSelf = NULL"),
    (0x004C3AC6, "DGS_JMP_ADD: VLHook doi jmp nay sang HookDrawGameSpace"),
    (0x00593A25, "GameInit: config.ini [Client] PaintFps -> g_nPaintInterval = 1000/fps"),
    (0x00591752, "GetTickCount - g_App_dwLostFocusTime >= 10000"),
    (0x005917C9, "goi UiPaint (duy nhat)"),
    (0x00591818, "Sleep(1): push ebp (ebp=1)"),
    (0x00678D12, "g_nVisibleNum truoc CoreDrawGameObj"),
    (0x006B30C9, "MissleOpen: thay hieu ung skill"),
]

# ============================================================================
# Thuc thi
# ============================================================================
_stats = {"func": 0, "glob": 0, "type": 0, "cmt": 0, "struct": 0, "enum": 0, "skip": 0, "err": 0}
_delta = 0


def log(msg):
    print("[JX10_10] " + msg)


def txt(s):
    if s is None:
        return None
    if STRIP_DIACRITICS:
        s = s.replace(u"đ", "d").replace(u"Đ", "D")
        s = unicodedata.normalize("NFKD", s)
        s = "".join(c for c in s if not unicodedata.combining(c))
    return s


def A(ea):
    return ea + _delta


TYPE_SIZES = {
    "char": 1, "unsigned char": 1, "short": 2, "unsigned short": 2,
    "int": 4, "unsigned int": 4, "__int64": 8, "void *": 4,
}


def type_size(t):
    if t.endswith("*"):
        return 4
    return TYPE_SIZES.get(t)


def get_type_size(name):
    tif = ida_typeinf.tinfo_t()
    if tif.get_named_type(None, name):
        return tif.get_size()
    return None


def gen_struct(name, size, fields):
    lines = ["struct %s {" % name]
    cur = 0
    for off, t, fname, cnt in sorted(fields, key=lambda f: f[0]):
        es = type_size(t)
        if es is None:
            es = TYPE_SIZES.get("struct " + t)
        if es is None:
            log("  ! %s.%s: khong biet kich thuoc kieu %s" % (name, fname, t))
            return None
        fs = es * (cnt or 1)
        if off < cur:
            log("  ! %s.%s @0x%X trung voi field truoc (0x%X) - bo qua" % (name, fname, off, cur))
            continue
        if off > cur:
            lines.append("  unsigned char _pad_%04X[%d];" % (cur, off - cur))
        decl = "%s %s" % (t, fname) if not t.endswith("*") else "%s%s" % (t, fname)
        if cnt:
            decl += "[%d]" % cnt
        lines.append("  %s;" % decl)
        cur = off + fs
    if size is not None:
        if cur > size:
            log("  ! %s vuot kich thuoc 0x%X (0x%X)" % (name, size, cur))
            return None
        if cur < size:
            lines.append("  unsigned char _pad_%04X[%d];" % (cur, size - cur))
    lines.append("};")
    return "\n".join(lines), (size if size is not None else cur)


def import_local_type(name):
    # IDA < 9: dua kieu tu Local Types vao cua so Structures/Enums
    try:
        if idaapi.IDA_SDK_VERSION < 900:
            idc.import_type(-1, name)
    except Exception:
        pass


def create_structs():
    for name, size, fields in STRUCTS:
        r = gen_struct(name, size, fields)
        if r is None:
            _stats["err"] += 1
            continue
        decl, sz = r
        nerr = idc.parse_decls("#pragma pack(push, 1)\n" + decl + "\n#pragma pack(pop)\n", idc.PT_SILENT)
        if nerr:
            # mot so ban IDA khong nhan #pragma: thu lai khong co
            nerr = idc.parse_decls(decl, idc.PT_SILENT)
        real = get_type_size(name)
        if nerr or real is None:
            log("  ! loi tao struct %s" % name)
            _stats["err"] += 1
            continue
        if real != sz:
            log("  ! %s: kich thuoc 0x%X != 0x%X (kiem tra pack)" % (name, real, sz))
        TYPE_SIZES["struct " + name] = sz
        import_local_type(name)
        _stats["struct"] += 1


def import_enums():
    path = os.path.normpath(GAMEDEF_PATH)
    if not os.path.isfile(path):
        log("  ! khong thay %s - bo qua enum" % path)
        return
    with open(path, "rb") as f:
        src = f.read().decode("utf-8", "ignore")
    for m in re.finditer(r"enum\s+(\w+)\s*\{(.*?)\}\s*;", src, re.S):
        name = m.group(1)
        if name not in ENUM_WHITELIST:
            continue
        body = re.sub(r"//[^\n]*", "", m.group(2))
        if idc.parse_decls("enum %s {%s};" % (name, body), idc.PT_SILENT):
            log("  ! loi parse enum %s" % name)
            _stats["err"] += 1
            continue
        import_local_type(name)
        _stats["enum"] += 1


def can_rename(ea, name):
    cur = ida_name.get_name(ea) or ""
    if cur == name:
        return False
    if ida_bytes.has_user_name(ida_bytes.get_flags(ea)) and not OVERWRITE_USER_NAMES:
        log("  - giu ten cu 0x%X '%s' (muon dat '%s')" % (ea, cur, name))
        _stats["skip"] += 1
        return False
    return True


def do_rename(ea, name):
    if not can_rename(ea, name):
        return False
    flags = ida_name.SN_NOWARN | ida_name.SN_NOCHECK | ida_name.SN_FORCE
    if ida_name.set_name(ea, name, flags):
        return True
    log("  ! khong dat duoc ten 0x%X -> %s" % (ea, name))
    _stats["err"] += 1
    return False


def build_proto(sig, name):
    if not sig:
        return None
    if "(" in sig:
        return sig
    parts = sig.split(":")
    n = int(parts[1])
    args = ["int a%d" % (i + 1) for i in range(n)]
    if parts[0] == "c":
        return "int __cdecl f(%s);" % (", ".join(args) or "void")
    cls = parts[2] + " *" if len(parts) > 2 else "void *"
    return "int __thiscall f(%s);" % ", ".join(["%sthis" % cls] + args)


def generic_proto(sig):
    # fallback khi prototype day du loi (vd enum chua co)
    m = re.search(r"__(cdecl|thiscall)", sig or "")
    if not m or "(" not in sig:
        return None
    n = sig[sig.index("(") + 1:sig.rindex(")")].count(",") + 1
    if sig[sig.index("(") + 1:sig.rindex(")")].strip() in ("", "void"):
        n = 0
    if m.group(1) == "cdecl":
        return build_proto("c:%d" % n, None)
    return build_proto("t:%d" % (n - 1), None)


def do_funcs():
    for ea, name, sig, cmt in FUNCS:
        ea = A(ea)
        f = ida_funcs.get_func(ea)
        if f is None:
            ida_funcs.add_func(ea)
            f = ida_funcs.get_func(ea)
        if f is None or f.start_ea != ea:
            log("  ! 0x%X (%s) khong phai dau ham - chi them comment" % (ea, name))
            if cmt or name:
                idc.set_cmt(ea, txt("%s %s" % (name or "", cmt or "")).strip(), 0)
            _stats["err"] += 1
            continue
        if name and do_rename(ea, name):
            _stats["func"] += 1
        if cmt:
            old = idc.get_func_cmt(ea, 1) or ""
            if txt(cmt) not in old:
                idc.set_func_cmt(ea, (old + "\n" if old else "") + txt(cmt), 1)
                _stats["cmt"] += 1
        if APPLY_PROTOTYPES and sig:
            p = build_proto(sig, name)
            if idc.SetType(ea, p):
                _stats["type"] += 1
            else:
                g = generic_proto(p)
                if g and idc.SetType(ea, g):
                    _stats["type"] += 1
                else:
                    log("  ! loi SetType 0x%X: %s" % (ea, p))
                    _stats["err"] += 1
    for ea, name, cmt in FUNCS_APPROX:
        ea = A(ea)
        f = ida_funcs.get_func(ea)
        if f is not None and f.start_ea == ea:
            do_rename(ea, name)
        else:
            idc.set_cmt(ea, txt(cmt), 0)
            log("  - %s: 0x%X la dia chi gan dung, chi them comment" % (name, ea))


def do_globals():
    for ea, name, ctype, cmt in GLOBALS:
        ea = A(ea)
        if name and do_rename(ea, name):
            _stats["glob"] += 1
        if APPLY_GLOBAL_TYPES and ctype:
            m = re.match(r"^(.*?)(\[[^\]]*\](?:\[[^\]]*\])*)$", ctype)
            if m:
                decl = "%s %s%s;" % (m.group(1).strip(), name, m.group(2))
            elif ctype.endswith("*"):
                decl = "%s%s;" % (ctype, name)
            else:
                decl = "%s %s;" % (ctype, name)
            if idc.SetType(ea, decl):
                _stats["type"] += 1
            else:
                log("  ! loi SetType 0x%X: %s" % (ea, decl))
                _stats["err"] += 1
        if cmt:
            idc.set_cmt(ea, txt(cmt), 1)
            _stats["cmt"] += 1


def do_comments():
    for ea, cmt in COMMENTS:
        ea = A(ea)
        old = idc.get_cmt(ea, 0) or ""
        if txt(cmt) in old:
            continue
        idc.set_cmt(ea, (old + " | " if old else "") + txt(cmt), 0)
        _stats["cmt"] += 1


def main():
    global _delta
    base = ida_nalt.get_imagebase()
    _delta = base - EXPECTED_IMAGEBASE
    if _delta:
        log("ImageBase 0x%X != 0x%X -> cong lech 0x%X" % (base, EXPECTED_IMAGEBASE, _delta))
    if IMPORT_ENUMS:
        log("Enum tu GameDef.h ...")
        import_enums()
    if CREATE_STRUCTS:
        log("Tao struct ...")
        create_structs()
    log("Ham ...")
    do_funcs()
    log("Bien toan cuc ...")
    do_globals()
    log("Comment ...")
    do_comments()
    log("Xong: ham %(func)d, bien %(glob)d, kieu %(type)d, comment %(cmt)d, struct %(struct)d, "
        "enum %(enum)d, bo qua %(skip)d, loi %(err)d" % _stats)
    idaapi.refresh_idaview_anyway()


main()
