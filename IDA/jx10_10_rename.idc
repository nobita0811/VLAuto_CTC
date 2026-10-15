// jx10_10_rename.idc - dat ten ham / bien / struct / enum cho gamecl_dump_SCY_10_10.exe
// Sinh tu jx10_10_rename.py (Offsets_10_10.md + GameBase.h). IDA 7.0+ (IDC).
// Chay: File -> Script file... (Alt+F7). Nen luu snapshot IDB truoc. Chay lai nhieu lan van an toan.
#include <idc.idc>

// 1 = ghi de ca ten ban da tu dat trong IDB
#define OVERWRITE_USER_NAMES 0

extern gOk, gSkip, gErr;

static IsAutoName(s)
{
  if (s == "") return 1;
  if (strstr(s, "sub_") == 0 || strstr(s, "loc_") == 0 || strstr(s, "unk_") == 0 ||
      strstr(s, "byte_") == 0 || strstr(s, "word_") == 0 || strstr(s, "dword_") == 0 ||
      strstr(s, "qword_") == 0 || strstr(s, "off_") == 0 || strstr(s, "stru_") == 0 ||
      strstr(s, "asc_") == 0 || strstr(s, "nullsub_") == 0 || strstr(s, "j_") == 0 ||
      strstr(s, "flt_") == 0 || strstr(s, "dbl_") == 0 || strstr(s, "xmmword_") == 0 ||
      strstr(s, "locret_") == 0 || strstr(s, "algn_") == 0)
    return 1;
  return 0;
}

static Rn(ea, name)
{
  auto cur;
  cur = get_name(ea);
  if (cur == name) return 1;
  if (!OVERWRITE_USER_NAMES && !IsAutoName(cur)) {
    msg("[JX10_10]  - giu ten cu %08X '%s' (muon dat '%s')\n", ea, cur, name);
    gSkip = gSkip + 1;
    return 0;
  }
  if (set_name(ea, name, SN_NOWARN | SN_NOCHECK | SN_FORCE)) { gOk = gOk + 1; return 1; }
  msg("[JX10_10]  ! khong dat duoc ten %08X -> %s\n", ea, name);
  gErr = gErr + 1;
  return 0;
}

static AddFuncCmt(ea, cmt)
{
  auto old;
  if (cmt == "") return;
  old = get_func_cmt(ea, 1);
  if (strstr(old, cmt) != -1) return;
  if (old != "") old = old + "\n";
  set_func_cmt(ea, old + cmt, 1);
}

static AddCmt(ea, cmt)
{
  auto old;
  old = get_cmt(ea, 0);
  if (strstr(old, cmt) != -1) return;
  if (old != "") old = old + " | ";
  set_cmt(ea, old + cmt, 0);
}

// name == "" -> chi comment; proto/proto2 == "" -> khong dat kieu
static Fn(ea, name, proto, proto2, cmt)
{
  if (get_func_attr(ea, FUNCATTR_START) != ea) add_func(ea, BADADDR);
  if (get_func_attr(ea, FUNCATTR_START) != ea) {
    msg("[JX10_10]  ! %08X (%s) khong phai dau ham - chi them comment\n", ea, name);
    AddCmt(ea, name + " " + cmt);
    gErr = gErr + 1;
    return;
  }
  if (name != "") Rn(ea, name);
  AddFuncCmt(ea, cmt);
  if (proto != "") {
    if (!SetType(ea, proto)) {
      if (proto2 == "" || !SetType(ea, proto2)) {
        msg("[JX10_10]  ! loi SetType %08X: %s\n", ea, proto);
        gErr = gErr + 1;
      }
    }
  }
}

static FnApprox(ea, name, cmt)
{
  if (get_func_attr(ea, FUNCATTR_START) == ea) Rn(ea, name);
  else {
    AddCmt(ea, cmt);
    msg("[JX10_10]  - %s: %08X la dia chi gan dung, chi them comment\n", name, ea);
  }
}

static Gv(ea, name, decl, cmt)
{
  if (name != "") Rn(ea, name);
  if (decl != "" && !SetType(ea, decl)) {
    msg("[JX10_10]  ! loi SetType %08X: %s\n", ea, decl);
    gErr = gErr + 1;
  }
  if (cmt != "") set_cmt(ea, cmt, 1);
}

static Ty(name, decl)
{
  if (parse_decls("#pragma pack(push, 1)\n" + decl + "\n#pragma pack(pop)\n", PT_SILENT) != 0) {
    if (parse_decls(decl, PT_SILENT) != 0) {
      msg("[JX10_10]  ! loi tao kieu %s\n", name);
      gErr = gErr + 1;
      return;
    }
  }
  import_type(-1, name);   // IDA < 9: hien trong cua so Structures/Enums
}

static main()
{
  auto d;
  gOk = 0; gSkip = 0; gErr = 0;
  d = get_imagebase() - 0x400000;
  if (d != 0) msg("[JX10_10] ImageBase lech 0x%X so voi 0x400000\n", d);

  msg("[JX10_10] Enum (tu GameDef.h) ...\n");
  Ty("NPCCAMP", "enum NPCCAMP { camp_begin, camp_justice, camp_evil, camp_balance, camp_free, camp_animal, camp_event, camp_num, };");
  Ty("NPC_RELATION", "enum NPC_RELATION { relation_none = 1, relation_self = 2, relation_ally = 4, relation_enemy = 8, relation_dialog = 16, relation_all = relation_none | relation_ally | relation_enemy | relation_self | relation_dialog, relation_num, };");
  Ty("NPCKIND", "enum NPCKIND { kind_normal, kind_player, kind_partner, kind_dialoger, kind_bird, kind_mouse, kind_num, };");
  Ty("NPCCMD", "enum NPCCMD { do_none, do_stand, do_walk, do_run, do_jump, do_skill, do_magic, do_attack, do_sit, do_hurt, do_death, do_defense, do_idle, do_revive = 21, };");
  Ty("OBJKIND", "enum OBJKIND { Obj_Kind_MapObj, Obj_Kind_Body, Obj_Kind_Box, Obj_Kind_Item, Obj_Kind_Money, Obj_Kind_StoreBox = 10 };");
  Ty("INVENTORY_ROOM", "enum INVENTORY_ROOM { room_equipment = 0, room_repository, room_trade, room_tradeback, room_trade1, room_immediacy, room_06, room_repo02, room_repo03, room_09, room_yesou, room_0a, room_0b, room_0c, room_guid, room_num, };");
  Ty("ITEM_POSITION", "enum ITEM_POSITION { pos_hand = 1, pos_equip, pos_equiproom, pos_repositoryroom, pos_traderoom, pos_trade1, pos_immediacy, pos_expand, pos_repo02, pos_repo03, pos_yesoubox = 12, pos_guid = 23, };");
  Ty("ITEMGENRE", "enum ITEMGENRE { item_equip = 0, item_medicine, item_mine, item_materials, item_task, item_townportal, item_special, item_number, };");
  Ty("EQUIPDETAILTYPE", "enum EQUIPDETAILTYPE { equip_meleeweapon = 0, equip_rangeweapon, equip_armor, equip_ring, equip_amulet, equip_boots, equip_belt, equip_helm, equip_cuff, equip_pendant, equip_horse, equip_mask, equip_detailnum, };");
  Ty("MEDICINEDETAILTYPE", "enum MEDICINEDETAILTYPE { medicine_blood = 0, medicine_mana, medicine_both, medicine_stamina, medicine_antipoison, medicine_detailnum, };");
  Ty("ITEM_PART", "enum ITEM_PART { itempart_head = 0, itempart_body, itempart_belt, itempart_weapon, itempart_foot, itempart_cuff, itempart_amulet, itempart_ring1, itempart_ring2, itempart_pendant, itempart_horse, itempart_mask, itempart_pp, itempart_an, itempart_ts, itempart_ts1, itempart_ts2, itempart_num, };");
  Ty("UIOBJECT_CONTAINER", "enum UIOBJECT_CONTAINER { UOC_IN_HAND = 1, UOC_GAMESPACE, UOC_IMMEDIA_ITEM, UOC_IMMEDIA_SKILL, UOC_ITEM_TAKE_WITH, UOC_GUID_BOX, UOC_TRADE_BOX, UOC_EQUIPTMENT = 9, UOC_NPC_SHOP, UOC_STORE_BOX, UOC_STORE_BOX2, UOC_STORE_BOX3, UOC_YESOU_BOX = 20, UOC_HTINH1_BOX, UOC_HTINH2_BOX, UOC_HTINH3_BOX, };");
  Ty("CORE_OBJECT_GENRE", "enum CORE_OBJECT_GENRE { CGOG_NOTHING = 0, CGOG_MONEY, CGOG_PLAYER_FACE, CGOG_OBJECT, CGOG_SKILL, CGOG_ITEM, CGOG_NPC, CGOG_MENU_NPC, CGOG_NPC_BLUR, CGOG_MISSLE, CGOG_EQUIP, CGOG_NPCSELLITEM, };");
  Ty("GAMEDATA_INDEX", "enum GAMEDATA_INDEX { GDI_GAME_OBJ_DESC = 1, GDI_GAME_OBJ_DESC_INCLUDE_TRADEINFO, GDI_GAME_OBJ_DESC_INCLUDE_REPAIRINFO, GDI_GAME_OBJ_LIGHT_PROP, GDI_PLAYER_BASE_INFO, GDI_TRADE_ITEM_PRICE = 0x0BBA, };");
  Ty("OPTIONS_LIST", "enum OPTIONS_LIST { OPTION_PERSPECTIVE, OPTION_DYNALIGHT, OPTION_MUSIC_VALUE, OPTION_SOUND_VALUE, OPTION_BRIGHTNESS, OPTION_WEATHER, };");
  Ty("GAMEOPERATION_INDEX", "enum GAMEOPERATION_INDEX { GOI_EXIT_GAME = 1, GOI_SWITCH_OBJECT_QUERY, GOI_SWITCH_OBJECT, GOI_REJECT_OBJECT, GOI_MONEY_INOUT_STORE_BOX, GOI_PLAYER_ACTION, GOI_PLAYER_RENASCENCE, GOI_INFORMATION_CONFIRM_NOTIFY, GOI_QUESTION_CHOOSE, GOI_USE_ITEM, GOI_WEAR_EQUIP, GOI_USE_SKILL, GOI_SET_IMMDIA_SKILL, GOI_TRADE_NPC_BUY = 0x18, GOI_TRADE_NPC_CHECK, GOI_TRADE_NPC_SELL, GOI_TRADE_NPC_REPAIR, GOI_TRADE_PLAYER_SETPRICE = 0x45, GOI_TRADE_PLAYER_GETPRICE, GOI_TRADE_PLAYER_SELL, GOI_TRADE_PLAYER_BUY, GOI_SHIFT_ITEM = 0x59, GOI_DECOMPOSE_EQUIP = 0x9B, };");
  Ty("TEAMOPERATION_INDEX", "enum TEAMOPERATION_INDEX { TEAM_CREATE = 0x05, TEAM_APPOINT, TEAM_INVITE, TEAM_KICK, TEAM_LEAVE, TEAM_JOIN_RESPONSE = 0x0C, TEAM_INVITE_RESPONSE = 0x0D, };");
  Ty("PROCESSPEOPLE_INDEX", "enum PROCESSPEOPLE_INDEX { ACTION_CHAT = 0, ACTION_MAKEFRIEND, ACTION_TRADE, ACTION_JOINTEAM, ACTION_INVITETEAM, ACTION_FOLLOW, ACTION_VIEWITEM, ACTION_TONG, ACTION_BLACKLIST = 9, ACTION_REVENGE = 10, };");
  Ty("MAGIC_ATTRIB", "enum MAGIC_ATTRIB { item_begin = 27, weapondamagemin_v, weapondamagemax_v, armordefense_v, durability_v, requirestr, requiredex, requirevit, requireeng, requirelevel, requireseries, requiresex, requiremenpai, weapondamageenhance_p, armordefenseenhance_p, requirementreduce_p, indestructible_b, item_usagetime_v, req_levelortranslife, require_translife, item_reserve4, item_reserve5, item_reserve6, item_reserve7, item_reserve8, item_reserve9, item_reserve10, item_end, damage_begin, attackrating_v, attackrating_p, ignoredefense_p, physicsdamage_v, colddamage_v, firedamage_v, lightingdamage_v, poisondamage_v, magicdamage_v, physicsenhance_p, steallife_p, stealmana_p, stealstamina_p, knockback_p, deadlystrike_p, fatallystrike_p, stun_p, addskillexp1, addskillexp2, seriesdamage_p, damage_reserve4, damage_reserve5, damage_reserve6, damage_reserve7, damage_reserve8, damage_reserve9, damage_reserve10, damage_end, normal_begin, lifemax_v, lifemax_p, life_v, lifereplenish_v, manamax_v, manamax_p, mana_v, manareplenish_v, staminamax_v, staminamax_p, stamina_v, staminareplenish_v, strength_v, dexterity_v, vitality_v, energy_v, poisonres_p, fireres_p, lightingres_p, physicsres_p, coldres_p, freezetimereduce_p, burntimereduce_p, poisontimereduce_p, poisondamagereduce_v, stuntimereduce_p, fastwalkrun_p, visionradius_p, fasthitrecover_v, allres_p, attackspeed_v, castspeed_v, meleedamagereturn_v, meleedamagereturn_p, rangedamagereturn_v, rangedamagereturn_p, addphysicsdamage_v, addfiredamage_v, addcolddamage_v, addlightingdamage_v, addpoisondamage_v, addphysicsdamage_p, slowmissle_b, changecamp_b, physicsarmor_v, coldarmor_v, firearmor_v, poisonarmor_v, lightingarmor_v, damage2addmana_p, lucky_v, steallifeenhance_p, stealmanaenhance_p, stealstaminaenhance_p, allskill_v, metalskill_v, woodskill_v, waterskill_v, fireskill_v, earthskill_v, knockbackenhance_p, deadlystrikeenhance_p, stunenhance_p, badstatustimereduce_v, manashield_p, adddefense_v, adddefense_p, fatallystrikeenhance_p, lifepotion_v, manapotion_v, physicsresmax_p, coldresmax_p, fireresmax_p, lightingresmax_p, poisonresmax_p, allresmax_p, coldenhance_p, fireenhance_p, lightingenhance_p, poisonenhance_p, magicenhance_p, attackratingenhance_v, attackratingenhance_p, addphysicsmagic_v, addcoldmagic_v, addfiremagic_v, addlightingmagic_v, addpoisonmagic_v, fatallystrikeres_p, statusimmunity_b, expenhance_v, expenhance_p, seriesres_p, seriesenhance_p, createnpc, autoattacknpc, dynamicmagicshield_v, nomovespeed, execscript, changefeature1, changefeature2, stealfeature, addstealfeatureskill, lucky_v_partner, listen_msg, lifereplenish_p, ignoreskill_p, returnskill_p, poisondamagereturn_v, poisondamagereturn_p, autoreplyskill, autoattackskill, autorescueskill, autodeathskill, randmove, hide, ignorenegativestate_p, poison2decmana_p, staticmagicshield_v, staticmagicshield_p, returnres_p, add120skillexpenhance_p, dec_percasttimehorse, dec_percasttime, enhance_709_auto, enhance_708_life_p, enhance_93_life_v, enhance_711_auto, enhance_714_auto, enhance_717_auto, enhance_723_miss_p, showpersoninfo, ignoredamage, sorbdamage_p, anti_hitrecover, anti_stuntimereduce_p, anti_poisonres_p, anti_fireres_p, anti_lightingres_p, anti_physicsres_p, anti_coldres_p, block_rate, enhancehit_rate, poisonres_yan_p, lightingres_yan_p, fireres_yan_p, physicsres_yan_p, coldres_yan_p, lifemax_yan_v, lifemax_yan_p, manamax_yan_v, manamax_yan_p, sorbdamage_yan_p, fastwalkrun_yan_p, attackspeed_yan_v, castspeed_yan_v, allres_yan_p, anti_maxres_p, skill_enhance, magicdamage_p, fasthitrecover_yan_v, five_elements_enhance_v, five_elements_resist_v, manareplenish_p, add_damage_p, forbit_attack, frozen_action, forbit_takemedicine, invincibility, not_add_pkvalue_p, add_boss_damage, anti_poisontimereduce_p = 258, do_hurt_p, anti_do_hurt_p, do_stun_p, anti_do_stun_p, anti_physicsres_yan_p, anti_poisonres_yan_p, anti_coldres_yan_p, anti_fireres_yan_p, anti_lightingres_yan_p, anti_allres_yan_p, anti_sorbdamage_yan_p, anti_block_rate, anti_enhancehit_rate, autocastskill, transfigure, oncastskill, me2metaldamage_p = 276, metal2medamage_p, me2wooddamage_p, wood2medamage_p, me2waterdamage_p, water2medamage_p, me2firedamage_p, fire2medamage_p, me2earthdamage_p, earth2medamage_p, meleedamagereturnmana_p, rangedamagereturnmana_p, reduceskillcd1, reduceskillcd2, reduceskillcd3, clearallcd, addblockrate, walkrunshadow, returnskill2enemy, candetonate1, candetonate2, candetonate3, manatoskill_enhance, movedistanc_710_enhance, validtiem_1366_enhance, daoxutian_enhance, foxinciyou_enhance, dec_pskill_cdtime, expenhance_p2, enhancehit_effect, anti_enhancehit_effect, monthcard_power, normal_end, damagetomana_p = damage2addmana_p, magicshield = dynamicmagicshield_v, };");

  msg("[JX10_10] Struct ...\n");
  Ty("KMagicAttrib16", "struct KMagicAttrib16 {\n  int nAttribType;\n  int nValue[3];\n};");  // size 0x10
  Ty("KNpcSkillSlot", "struct KNpcSkillSlot {\n  int nSkillId;\n  int nLevel_maybe;\n  unsigned char _pad_0008[40];\n};");  // size 0x30
  Ty("KPlayerItemRec", "struct KPlayerItemRec {\n  unsigned char _pad_0000[20];\n};");  // size 0x14
  Ty("KInventory", "struct KInventory {\n  unsigned char _pad_0000[4];\n  int m_nMoney;\n  unsigned char _pad_0008[20];\n};");  // size 0x1C
  Ty("KLoginServer", "struct KLoginServer {\n  char Title[32];\n  unsigned char IP[4];\n};");  // size 0x24
  Ty("KRoleChiefInfo", "struct KRoleChiefInfo {\n  char Name[32];\n  unsigned char Gender;\n  unsigned char Attribute;\n  short Level;\n};");  // size 0x24
  Ty("KItemList", "struct KItemList {\n  unsigned char _pad_0000[28];\n  int m_nForeHand;\n  unsigned char _pad_0020[8];\n  int m_EquipItem[17];\n  KPlayerItemRec m_Items[483];\n  unsigned char _pad_2628[9684];\n  int m_UseIdx;\n  unsigned char _pad_4C00[8];\n  int m_nListCurIdx;\n  int m_bLockOperation;\n  unsigned char _pad_4C10[76];\n  KInventory m_Room[26];\n};");  // size 0x4F34
  Ty("KNpc", "struct KNpc {\n  char *vn_pBuf401;\n  unsigned char _pad_0004[4];\n  unsigned int m_dwID;\n  int m_Index;\n  int m_Node;\n  unsigned char _pad_0014[20];\n  int m_Level;\n  int m_Kind;\n  int m_Series;\n  int m_Height;\n  unsigned char m_btRankId;\n  unsigned char _pad_0039[7];\n  int vn_Flag40;\n  int vn_Flag44;\n  int vn_Flag48;\n  int m_cGold;\n  unsigned char _pad_0050[68];\n  int m_PoisonState;\n  unsigned char _pad_0098[92];\n  int m_Camp;\n  int m_CurrentCamp;\n  int m_Doing;\n  unsigned char _pad_0100[16];\n  void *m_pHeadNode;\n  void *m_pTailNode;\n  int m_nNodeCount;\n  int m_ActiveSkillID;\n  unsigned char _pad_0120[4];\n  unsigned char m_SkillList_hdr[8];\n  KNpcSkillSlot m_Skills[80];\n  unsigned char _pad_102C[40];\n  int m_RegionIndex;\n  unsigned char _pad_1058[4];\n  int m_CurrentLife;\n  unsigned char _pad_1060[16];\n  int m_CurrentMana;\n  unsigned char _pad_1074[4];\n  int m_CurrentStamina;\n  int m_CurrentStaminaMax;\n  unsigned char _pad_1080[208];\n  int m_CurrentWalkSpeed;\n  int m_CurrentRunSpeed;\n  unsigned char _pad_1158[24];\n  int m_CurrentAttackRadius;\n  unsigned char _pad_1174[556];\n  int m_MapX;\n  int m_MapY;\n  int m_MapZ;\n  int m_OffX;\n  int m_OffY;\n  unsigned char _pad_13B4[84];\n  char Name0;\n  char Name[32];\n  unsigned char _pad_1429[11];\n  int m_NpcSettingIdx;\n  unsigned char _pad_1438[96];\n  int m_nPeopleIdx;\n  unsigned char _pad_149C[12];\n  int m_nObjectIdx;\n  unsigned char _pad_14AC[524];\n  unsigned int m_SyncSignal;\n  unsigned char _pad_16BC[4];\n  int m_sClientNpcID[2];\n  short m_RegX;\n  short m_RegY;\n  int m_sSyncPos;\n  unsigned char _pad_16D0[20];\n  int m_nPKFlag;\n  unsigned char _pad_16E8[296];\n  unsigned int m_dwTongNameID_unverified;\n  int m_bHaveStall_unverified;\n  unsigned char _pad_1818[264];\n  int m_FightMode;\n  unsigned char _pad_1924[60];\n  int m_Command[4];\n  unsigned char _pad_1970[80];\n  int m_bRideHorse;\n  unsigned char _pad_19C4[69968];\n  int m_CurrentLifeMax_a;\n  int m_CurrentLifeMax_b;\n  int m_CurrentManaMax_a;\n  int m_CurrentManaMax_b;\n  unsigned char _pad_12B24[176];\n};");  // size 0x12BD4
  Ty("KPlayer", "struct KPlayer {\n  unsigned char _pad_0000[40];\n  unsigned char m_nEntrustMode;\n  unsigned char _pad_0029[3];\n  int m_nLeftSkillID;\n  unsigned char _pad_0030[6104];\n  int m_RunStatus;\n  unsigned char _pad_180C[4];\n  int m_nSendMoveFrames;\n  unsigned char _pad_1814[60];\n  unsigned int m_dwID;\n  int m_nIndex;\n  unsigned char _pad_1858[4];\n  KItemList m_ItemList;\n  unsigned char _pad_6790[4];\n  int m_nCurShop;\n  unsigned char _pad_6798[2696];\n  __int64 m_nCurExp;\n  __int64 m_nNextExp;\n  unsigned char _pad_7230[40];\n  int m_cTeam_nFlag;\n  int m_cTeam_nFigure;\n  unsigned char _pad_7260[1440];\n  int m_cFaction_nCurFaction;\n  unsigned char _pad_7804[580];\n  unsigned char m_cPK[88];\n  int m_cPK_nPKValue;\n  unsigned char _pad_7AA4[600];\n  unsigned char m_cTong[44];\n  int m_cTong_nFlag;\n  unsigned char _pad_7D2C[8];\n  unsigned int m_cTong_dwTongNameID;\n  unsigned char _pad_7D38[9320];\n  int m_cTask;\n  unsigned char _pad_A1A4[1748];\n};");  // size 0xA878
  Ty("KItem", "struct KItem {\n  unsigned char _pad_0000[8];\n  int m_nGenre_maybe;\n  unsigned char _pad_000C[36];\n  char m_szName[64];\n  unsigned char _pad_0070[440];\n  KMagicAttrib16 m_aryAllAttrib[24];\n  unsigned char _pad_03A8[4];\n  int m_MagicLevel;\n  unsigned char _pad_03B0[304];\n  unsigned int m_dwUniID;\n  int m_nCurDur;\n  int m_nMaxDur;\n  unsigned char _pad_04EC[40];\n  unsigned char m_nItemPlus;\n  unsigned char m_ExParam1;\n  unsigned short m_ExParam2;\n  int m_nLeftUsageTime;\n  int m_nExpiredTime;\n  int m_nLockCount;\n  unsigned char _pad_0524[412];\n  int m_nNoDecompose_maybe;\n  unsigned char _pad_06C4[136];\n};");  // size 0x74C
  Ty("KObj", "struct KObj {\n  unsigned char _pad_0000[8];\n  int m_nKind;\n  int m_nIndex;\n  short m_RegX;\n  short m_RegY;\n  int m_nSubWorldID;\n  int m_nRegionIdx;\n  int m_nMapX;\n  int m_nMapY;\n  int m_nOffX;\n  int m_nOffY;\n  unsigned char _pad_002C[52];\n  char m_szName[36];\n  int m_nColorID;\n  int m_nItemDataID;\n  int m_nItemWidth;\n  int m_nItemHeight;\n  int m_nMoneyNum;\n  unsigned char _pad_0098[536];\n  int m_nDropState;\n  unsigned char _pad_02B4[208];\n  unsigned int m_dwNameColor;\n  unsigned char _pad_0388[108];\n};");  // size 0x3F4
  Ty("KSkillInfo", "struct KSkillInfo {\n  int TabFileRowId;\n  int SkillStyle;\n  int MaxLevel;\n  char SkillName[32];\n  unsigned int SkillId;\n  unsigned char _pad_0030[8];\n  int IsPhysical;\n  unsigned char _pad_003C[4];\n  int IsAura;\n  int IsMelee;\n  unsigned char _pad_0048[8];\n  int AttackRadius;\n  int WaitTime;\n  unsigned char _pad_0058[8];\n  int Series;\n  unsigned char _pad_0064[20];\n  int CharClass;\n  unsigned char _pad_007C[36];\n  int CostValue;\n  unsigned char _pad_00A4[4];\n  int TimePerCast;\n  int TimePerCastOnHorse;\n  unsigned char _pad_00B0[4];\n  int EqtLimit;\n  int HorseLimit;\n  unsigned char _pad_00BC[8];\n  int ChildSkillId;\n  unsigned char _pad_00C8[84];\n  char SkillDesc[600];\n  char SkillIcon[128];\n  unsigned char _pad_03F4[264];\n};");  // size 0x4FC
  Ty("KTeam", "struct KTeam {\n  unsigned char _pad_0000[8];\n  int m_nCaptain;\n  unsigned char _pad_000C[28];\n  int m_nMemNum;\n  unsigned char _pad_002C[4];\n  int m_nMemLevel[8];\n  char m_szMemName[256];\n  int m_nServerID;\n};");  // size 0x154
  Ty("KLogin", "struct KLogin {\n  unsigned char _pad_0000[8];\n  int m_Status;\n  unsigned char _pad_000C[6];\n  short m_nNumRole;\n  KRoleChiefInfo m_RoleList[3];\n  unsigned char _pad_0080[1696];\n  char m_Choices_Account[32];\n};");  // size 0x740
  Ty("KScenePlaceRegionC", "struct KScenePlaceRegionC {\n  unsigned char _pad_0000[4];\n  int m_Status;\n  int m_LeftTopX;\n  int m_LeftTopY;\n  int m_RegionIndexX;\n  int m_RegionIndexY;\n  unsigned char _pad_0018[2096];\n  unsigned int m_TrapInfo[512];\n};");  // size 0x1048
  Ty("KWndMessageListBox", "struct KWndMessageListBox {\n  unsigned char _pad_0000[1176];\n  void *pMessages;\n  int nNumMessage;\n  int nCapability;\n};");  // size 0x4A4
  Ty("KUiMsgSel", "struct KUiMsgSel {\n  unsigned char _pad_0000[1412];\n  int wnd_Select;\n  unsigned char _pad_0588[1168];\n  KWndMessageListBox m_List;\n  unsigned char _pad_0EBC[3392];\n  int wnd_InfoText;\n  unsigned char _pad_1C00[1396];\n  char m_szInfoText[512];\n  unsigned char _pad_2374[12];\n};");  // size 0x2380
  Ty("KUiMsgSel2", "struct KUiMsgSel2 {\n  unsigned char _pad_0000[1412];\n  int wnd_MsgScroll;\n  unsigned char _pad_0588[3296];\n  int wnd_Select;\n  unsigned char _pad_126C[1168];\n  KWndMessageListBox m_List;\n  unsigned char _pad_1BA0[3392];\n  int wnd_MessageList;\n  unsigned char _pad_28E4[1168];\n  void *m_MessageList_pMessages;\n  unsigned char _pad_2D78[4];\n  int m_MessageList_nNum;\n  unsigned char _pad_2D80[80];\n  int wnd_Image;\n  unsigned char _pad_2DD4[1360];\n  int wnd_Text;\n  unsigned char _pad_3328[6820];\n};");  // size 0x4DCC
  Ty("KUiShop", "struct KUiShop {\n  unsigned char _pad_0000[1412];\n  void *m_pObjsList;\n  unsigned char _pad_0588[28];\n  int m_nObjCount;\n  int m_nPageCount;\n  int m_nCurrentPage;\n  unsigned char _pad_05B0[18364];\n};");  // size 0x4D6C
  Ty("KUiInformation", "struct KUiInformation {\n  unsigned char _pad_0000[1412];\n  int wnd_Info;\n  unsigned char _pad_0588[1396];\n  char m_szInfoText[512];\n  unsigned char _pad_0CFC[2288];\n  void *m_pCallerWnd;\n};");  // size 0x15F0
  Ty("KUiPlayerBar", "struct KUiPlayerBar {\n  unsigned char _pad_0000[3268];\n  int wnd_GShopButton;\n  unsigned char _pad_0CC8[5896];\n  unsigned char m_ImmediaItem[11376];\n};");  // size 0x5040
  Ty("KUiItem", "struct KUiItem {\n  unsigned char _pad_0000[26960];\n  int wnd_OpenStatus;\n  unsigned char _pad_6954[4928];\n  int wnd_DecomposeEquip;\n};");  // size 0x7C98
  Ty("KUiInit", "struct KUiInit {\n  unsigned char _pad_0000[1444];\n  int wnd_EnterGame;\n  unsigned char _pad_05A8[3804];\n  int wnd_Options;\n  unsigned char _pad_1488[3804];\n  int wnd_ExitGame;\n};");  // size 0x2368
  Ty("KUiSelServer", "struct KUiSelServer {\n  unsigned char _pad_0000[2308];\n  KLoginServer *m_pRegionList;\n  KLoginServer *m_pServerList;\n  int wnd_LeftList;\n  unsigned char _pad_0910[1196];\n  int m_LeftList_nCurSel;\n  unsigned char _pad_0DC0[252];\n  int wnd_RightList;\n  unsigned char _pad_0EC0[1196];\n  int m_RightList_nCurSel;\n  unsigned char _pad_1370[252];\n  int wnd_IpList;\n  unsigned char _pad_1470[1196];\n  int m_IpList_nCurSel;\n  unsigned char _pad_1920[252];\n  int wnd_Text;\n  unsigned char _pad_1A20[1428];\n  int wnd_Login;\n  unsigned char _pad_1FB8[1900];\n  int wnd_Cancel;\n  unsigned char _pad_2728[1936];\n  int m_nCurRegion;\n  unsigned char _pad_2EBC[508];\n};");  // size 0x30B8
  Ty("KUiLogin", "struct KUiLogin {\n  unsigned char _pad_0000[1416];\n  int wnd_Account;\n  unsigned char _pad_058C[1288];\n  int wnd_PassWord;\n  unsigned char _pad_0A98[1288];\n  int wnd_Login;\n  unsigned char _pad_0FA4[1900];\n  int wnd_Cancel;\n  unsigned char _pad_1714[1900];\n  int wnd_Remember;\n  unsigned char _pad_1E84[1900];\n  int wnd_Invisible;\n  unsigned char _pad_25F4[1900];\n  int wnd_VirtualKeyboard;\n  unsigned char _pad_2D64[21340];\n  int wnd_AgreeTerms;\n  unsigned char _pad_80C4[28452];\n};");  // size 0xEFE8
  Ty("KUiSelPlayer", "struct KUiSelPlayer {\n  unsigned char _pad_0000[13780];\n  int wnd_Name;\n  unsigned char _pad_35D8[4292];\n  int wnd_Level;\n  unsigned char _pad_46A0[4292];\n  unsigned char m_player[5712];\n  unsigned char _pad_6DB4[4092];\n  int wnd_Ok;\n  unsigned char _pad_7DB4[1900];\n  int wnd_Cancel;\n  unsigned char _pad_8524[1900];\n  int wnd_New;\n  unsigned char _pad_8C94[1900];\n  int wnd_Del;\n  unsigned char _pad_9404[1900];\n  int wnd_Transfer;\n  unsigned char _pad_9B74[1900];\n  int wnd_Next;\n  unsigned char _pad_A2E4[1900];\n  int wnd_Pre;\n  unsigned char _pad_AA54[3856];\n  short m_nNumPlayer;\n  short m_nSelPlayer;\n  unsigned char _pad_B968[8];\n  int m_nPageStart;\n};");  // size 0xB974

  msg("[JX10_10] Ham ...\n");
  Fn(d + 0x004360B0, "ExecuteScript", "int __cdecl f(int a1);", "", "EXS_FUNC_ADD");
  Fn(d + 0x0044C560, "UiCloseWndsInGame", "void __cdecl f(int bAll);", "int __cdecl f(int a1);", "CLA_FUNC_ADD");
  Fn(d + 0x005B9F30, "KCoreShell__GetGameData", "int __thiscall f(void *this, GAMEDATA_INDEX uDataId, unsigned int uParam, int nParam);", "int __thiscall f(void *this, int a1, int a2, int a3);", "GGD_FUNC_ADD");
  Fn(d + 0x005C3820, "KCoreShell__OperationRequest", "int __thiscall f(void *this, GAMEOPERATION_INDEX uOper, unsigned int uParam, int nParam);", "int __thiscall f(void *this, int a1, int a2, int a3);", "OPR_FUNC_ADD. case 1 EXIT_GAME, 5 MONEY_INOUT_STORE_BOX, 0x2B OPTION_SETTING (0x5C5181), 0x9B DECOMPOSE (0x5C4A4F)");
  Fn(d + 0x005C2050, "KCoreShell__TongOperation", "int __thiscall f(void *this, int a1, int a2, int a3);", "", "TNP_FUNC_ADD (GetTongNameID, switch 0..0x3D)");
  Fn(d + 0x005BB090, "KCoreShell__TeamOperation", "int __thiscall f(void *this, TEAMOPERATION_INDEX uOper, unsigned int uParam, int nParam);", "int __thiscall f(void *this, int a1, int a2, int a3);", "TOP_FUNC_ADD");
  Fn(d + 0x005BB3C0, "KCoreShell__ProcessGameSecurity", "int __thiscall f(void *this, int a1, int a2, int a3);", "", "PGS_FUNC_ADD (switch 0..7, khoa ruong)");
  Fn(d + 0x005BAAF0, "KCoreShell__GetNPCRelation", "int __thiscall f(void *this, int a1);", "", "GRL_FUNC_ADD = NpcSet.GetRelation(me, n)");
  Fn(d + 0x005BAB50, "KCoreShell__DrawGameSpace", "int __thiscall f(void *this);", "", "DGS_FUNC_ADD: ScenePlace.Paint + DrawSelectInfo");
  Fn(d + 0x005BAC00, "KCoreShell__Breathe", "", "", "goi trong vong logic GameLoop");
  Fn(d + 0x005F3B30, "KNpc__MoveTo", "int __thiscall f(KNpc *this, int a1, int a2, int a3);", "int __thiscall f(void *this, int a1, int a2, int a3);", "MVT_FUNC_ADD: SendCommand(do_walk/do_run) + SendClientCmdWalk/Run");
  Fn(d + 0x005ECED0, "SendClientCmdSkill", "void __cdecl f(int nSkillId, int nX, int nY);", "int __cdecl f(int a1, int a2, int a3);", "SKL_FUNC_ADD, goi 0x4D dai 0x11");
  Fn(d + 0x005E2530, "KPlayer__PickUpObj", "int __thiscall f(KPlayer *this, int a1);", "int __thiscall f(void *this, int a1);", "PKO_FUNC_ADD: CheckTrading, kind 3/4");
  Fn(d + 0x0061C820, "KPlayerTask__GetSaveVal", "int __thiscall f(void *this, int a1);", "", "GTV_BASE_ADD, n <= 0x176F; this = KPlayer+0xA1A0");
  Fn(d + 0x006B9390, "KBuySell__GetItem", "int __thiscall f(void *this, int a1);", "", "BGI_FUNC_ADD");
  Fn(d + 0x006B93E0, "KBuySell__GetItemIndex", "int __thiscall f(void *this, int nBuyIdx, int nID);", "int __thiscall f(void *this, int a1, int a2);", "BGX_FUNC_ADD");
  Fn(d + 0x0051EB50, "KUiMsgSel__OnClickMsg", "int __thiscall f(KUiMsgSel *this, int a1);", "int __thiscall f(void *this, int a1);", "OMS_FUNC_ADD (dung this+0xA18)");
  Fn(d + 0x00509E40, "KUiMsgSel2__OnClickMsg", "int __thiscall f(KUiMsgSel2 *this, int a1);", "int __thiscall f(void *this, int a1);", "OMS2_FUNC_ADD: OperationRequest(9,0,n) + CloseWindow(1)");
  Fn(d + 0x004B9340, "KUiMsgCentrePad__SystemMessageArrival", "void __cdecl f(const char *pMsg, int nLen, int bChannelBox);", "int __cdecl f(int a1, int a2, int a3);", "SMA_FUNC_ADD: arg3=0 -> m_Sys(+0x7AD4); m_pSelf=NULL -> hang cho 0x4B5B10");
  Fn(d + 0x00498F80, "KUiStoreBox__OpenWindow", "int __cdecl f(void);", "", "SBO_FUNC_ADD (new 0x4980 -> m_pSelf 0x84051C)");
  Fn(d + 0x004BF2F0, "KUiShop__OpenWindow", "int __cdecl f(void);", "", "SHO_FUNC_ADD (new 0x4D6C -> m_pSelf 0x8478E0)");
  Fn(d + 0x00478440, "KUiPlayerBar__WndProc", "int __thiscall f(KUiPlayerBar *this, int a1, int a2, int a3);", "int __thiscall f(void *this, int a1, int a2, int a3);", "UWP_FUNC_ADD = vtable 0x791784[4]");
  Fn(d + 0x004768D0, "KUiPlayerBar__OnSendSomeoneMessage", "int __cdecl f(const char *szName, const char *pMsg, int nLen);", "int __cdecl f(int a1, int a2, int a3);", "OSM_FUNC_ADD, goi 0xFB/0x20");
  Fn(d + 0x0048FFB0, "KUiInit__OnClickButton", "int __thiscall f(KUiInit *this, int a1);", "int __thiscall f(void *this, int a1);", "IOC_FUNC_ADD; m_ExitGame = this+0x2364");
  Fn(d + 0x004AE490, "KUiInformation__Hide", "int __thiscall f(KUiInformation *this, int a1);", "int __thiscall f(void *this, int a1);", "IH1_FUNC_ADD");
  Fn(d + 0x004AE940, "KUiInformation2__Hide", "int __thiscall f(void *this);", "", "IH2_FUNC_ADD: caller==-1 -> OperationRequest(8,0,0)");
  Fn(d + 0x004C4530, "ProcessPeople", "int __cdecl f(int a1, int a2);", "", "PCP_FUNC_ADD: case 3 JOINTEAM, case 10 REVENGE -> OPR 0x2E");
  Fn(d + 0x004C5390, "KUiSysMsgCentre__DeleteMsgInHeap", "int __thiscall f(void *this, int a1, int a2);", "", "SMD_FUNC_ADD; m_MsgHeap = this+0x4C8 (12 byte)");
  Fn(d + 0x004C7C70, "MiniMap_SetFlag", "void __cdecl f(int x, int y, int bWorld);", "int __cdecl f(int a1, int a2, int a3);", "SMF_FUNC_ADD: toa do tuyet doi; ghi FLAG_X(x>>4)/FLAG_Y(y>>5)");
  Fn(d + 0x0063A500, "KItem__GetRepairPrice", "int __thiscall f(KItem *this);", "int __thiscall f(void *this);", "IGR_FUNC_ADD");
  Fn(d + 0x005E4C80, "KPlayer__DialogNpc", "int __thiscall f(KPlayer *this, int a1);", "int __thiscall f(void *this, int a1);", "DNPC_FUNC_ADD");
  Fn(d + 0x005DEA20, "KPlayer__ApplyAddBaseAttribute", "int __thiscall f(KPlayer *this, int a1, int a2);", "int __thiscall f(void *this, int a1, int a2);", "PAA_FUNC_ADD: attr<=3, goi 0x57 dai 0xE");
  Fn(d + 0x005F1A20, "KNpc__SendCommand", "", "", "ghi m_Command 0x1960..0x196C");
  Fn(d + 0x005FA900, "KNpc__SetFightMode", "", "", "");
  Fn(d + 0x005FB420, "KNpc__KNpc", "", "", "ctor: [esi]=new char[0x401], KList tai +0x110");
  Fn(d + 0x0066F340, "KNpcSet__SearchID", "", "", "");
  Fn(d + 0x0066FCF0, "KNpcSet__GetRelation", "", "", "");
  Fn(d + 0x005DED00, "KPlayer__SetLeftSkill", "", "", "dung npc+0x124 (m_SkillList)");
  Fn(d + 0x005E4CE0, "KPlayer__CheckObject", "", "", "xoa m_nObjectIdx (0x14A8)");
  Fn(d + 0x005E4DB0, "KPlayer__DrawSelectInfo", "", "", "");
  Fn(d + 0x005ED2B0, "SendClientCmdWalk", "", "", "");
  Fn(d + 0x005ED230, "SendClientCmdRun", "", "", "");
  Fn(d + 0x0065ECF0, "s2c_SyncNpc", "", "", "");
  Fn(d + 0x0065FE00, "s2c_SyncPlayer", "", "", "");
  Fn(d + 0x0065EAF0, "s2c_SyncCurNormalData", "", "", "");
  Fn(d + 0x0065C110, "s2c_SyncItemLockCount", "", "", "{itemID, value} -> KItem+0x520");
  Fn(d + 0x006123D0, "KItemList__GetFirstItem", "", "", "");
  Fn(d + 0x005E24B0, "KItemList__SendItemPosCmd", "", "", "(idx, &{place,x,y}, protocol) - ra do: 0xC6");
  Fn(d + 0x006ADC70, "KPlayerPK__SetPKValue", "", "", "");
  Fn(d + 0x00621D10, "KPlayerTong__GetTongNameID", "", "", "m_nFlag +0x2C, m_dwTongNameID +0x38");
  Fn(d + 0x00683B70, "KPlayerFaction__Init", "", "", "m_nCurFaction = -1");
  Fn(d + 0x00634650, "KItem__GetTotalMagicLevel", "", "", "");
  Fn(d + 0x006346D0, "KItem__CanBeRepaired", "", "", "10_10 khong xet +N");
  Fn(d + 0x006390E0, "KItem__GetDesc", "", "", "\"%s+%d%s\" voi byte [item+0x514]");
  Fn(d + 0x00612F00, "KItem__GetItemPlus", "", "", "");
  Fn(d + 0x0063D580, "KItem__SetItemPlus", "", "", "");
  Fn(d + 0x00631E20, "KItem__GetExParam2", "", "", "");
  Fn(d + 0x00631E30, "KItem__SetExParam2", "", "", "");
  Fn(d + 0x00634900, "KItem__ExParam1_634900", "", "", "ItemExParam[1] (0x515)");
  Fn(d + 0x0072A5B0, "Lua_ITEM_GetItemExParam", "", "", "");
  Fn(d + 0x0072A6E0, "Lua_ITEM_GetLeftUsageTime", "", "", "");
  Fn(d + 0x0072A780, "Lua_ITEM_GetExpiredTime", "", "", "");
  Fn(d + 0x006826F0, "KObjSet__AddData", "", "", "");
  Fn(d + 0x00683490, "KObjSet__ClientAdd", "", "", "");
  Fn(d + 0x0060D740, "KSkillManager__Init", "", "", "ecx = 0x1AB4A10");
  Fn(d + 0x006F7DB0, "KSkill__LoadSkillInfo", "", "", "nap 1 dong Skills.txt, ebp = entry+0xC");
  Fn(d + 0x0045E110, "KWndMessageListBox__GetOneMessage", "", "", "");
  Fn(d + 0x0045F0D0, "KWndMessageListBox__AddOneMessage", "", "", "");
  Fn(d + 0x00464C60, "KWndRichMessageListBox__AddOneMessage", "", "", "chu o msg+0x10D8");
  Fn(d + 0x0046B540, "KWndText512__KWndText512", "", "", "buf +0x578, 0x1FF");
  Fn(d + 0x00460000, "KWndObjectBox__GetObject", "", "", "");
  Fn(d + 0x00460070, "KWndObjectBox__HoldObject", "", "", "");
  Fn(d + 0x00458A30, "KWndEdit__SetText", "int __thiscall f(void *this, int a1);", "", "");
  Fn(d + 0x00458BD0, "KWndEdit__GetText", "int __thiscall f(void *this, int a1, int a2, int a3);", "", "");
  Fn(d + 0x00453AD0, "KWndButton__CheckButton", "int __thiscall f(void *this, int a1);", "", "co 0x200 o +0x554");
  Fn(d + 0x00453AC0, "KWndButton__IsButtonChecked", "int __thiscall f(void *this);", "", "");
  Fn(d + 0x0045BD20, "KWndList__SetCurSel", "int __thiscall f(void *this, int a1);", "", "bao 0x691 len cha");
  Fn(d + 0x00483C30, "KWndList__ClearSel", "", "", "bo chon, khong bao");
  Fn(d + 0x0045C790, "KWndList__SetContent", "int __thiscall f(void *this, int a1, int a2, int a3, int a4);", "", "");
  Fn(d + 0x00483B10, "KWndList__SetHighlight", "int __thiscall f(void *this, int a1);", "", "");
  Fn(d + 0x0051EA60, "KUiMsgSel__LoadScheme", "", "", "");
  Fn(d + 0x0051EBC0, "KUiMsgSel__GetMsgCount", "", "", "= [ecx+0xEB4]");
  Fn(d + 0x0051F000, "KUiMsgSel__KUiMsgSel", "", "", "");
  Fn(d + 0x00509590, "KUiMsgSel2__LoadScheme", "", "", "");
  Fn(d + 0x004BEDA0, "KUiShop__UpdateData", "", "", "GDI 0xBB9, phan tu 0x1C");
  Fn(d + 0x004BE960, "KUiShop__SetPage", "", "", "");
  Fn(d + 0x004AE100, "KUiInformation__LoadScheme", "", "", "");
  Fn(d + 0x004AE7D0, "KUiInformation2__LoadScheme", "", "", "");
  Fn(d + 0x00497EF0, "KUiStoreBox__GetIfVisible", "", "", "");
  Fn(d + 0x00491D50, "KUiItem__GetIfVisible", "", "", "");
  Fn(d + 0x00492990, "KUiItem__OnDecomposeButton", "", "", "bat/tat con tro 'Giai' (co 0x400000)");
  Fn(d + 0x00492620, "KUiItem__OnSpecialClickItem", "", "", "(pObj, 0x9B, 0x400000) -> OperationRequest(0x9B, &obj, 5)");
  Fn(d + 0x0044B7A0, "Ui_SetCursorState", "", "", "co trang thai chuot 0x8329C8");
  Fn(d + 0x004486B0, "Ui_GetCursorState", "", "", "");
  Fn(d + 0x0048F870, "KUiInit__CloseWindow", "", "", "");
  Fn(d + 0x00490340, "KUiInit__OpenWindow", "", "", "`开始.ini`");
  Fn(d + 0x004C3AC0, "KUiGameSpace__PaintWindow", "", "", "mov ecx,[CORE]; jmp DrawGameSpace (DGS_JMP_ADD 0x4C3AC6)");
  Fn(d + 0x00675C70, "KScenePlaceC__Paint", "int __thiscall f(void *this);", "", "ecx = g_ScenePlace");
  Fn(d + 0x00712180, "KScenePlaceRegionC__PaintGround", "", "", "");
  Fn(d + 0x007145E0, "KScenePlaceRegionC__Load", "", "", "%sRegion_C.dat");
  Fn(d + 0x00711F00, "KScenePlaceRegionC__LoadTrap", "", "", "");
  Fn(d + 0x00712560, "KScenePlaceRegionC__GetTrap", "", "", "(mpsX, mpsY)");
  Fn(d + 0x0062E4A0, "Map_LoadAllRegionC", "", "", "doc Region_C.dat ca map (luoi tim duong 10_10)");
  Fn(d + 0x00672DB0, "CoreDrawGameObj", "int __cdecl f(int a1, int a2, int a3, int a4, int a5, int a6, int a7, int a8);", "", "switch genre (0x10004...)");
  Fn(d + 0x006752D0, "KScenePlaceC__EnableDynamicLights", "", "", "OPTION 1");
  Fn(d + 0x006753C0, "KScenePlaceC__EnableWeather", "", "", "OPTION 5");
  Fn(d + 0x00678E60, "KOption__SetMusicVolume", "", "", "");
  Fn(d + 0x00678E10, "KOption__SetSoundVolume", "", "", "");
  Fn(d + 0x00678DF0, "KOption__SetBrightness", "", "", "");
  Fn(d + 0x005F08D0, "NpcTheSame_Set8", "", "", "OPTION 8");
  Fn(d + 0x005F08B0, "NpcTheSame_Set9", "", "", "OPTION 9");
  Fn(d + 0x0048D060, "KUiOptions__SaveSetting", "", "", "[Options] VisibleNum, NpcTheSame, MissleOpen, QualityOpen, PaintFPS");
  Fn(d + 0x0048D280, "KUiOptions__SetQuality", "", "", "g_pRepresentShell vt+0x74 SetOption(6/5)");
  Fn(d + 0x00591640, "KMyApp__GameLoop", "", "", "logic 18 nhip/s; ve khi now-[0x9BC790] >= [0x815838]; Sleep(1) 0x591818");
  Fn(d + 0x0044EAE0, "UiHeartBeat", "", "", "");
  Fn(d + 0x0044E980, "UiPaint", "int __cdecl f(int a1);", "", "");
  Fn(d + 0x0044C190, "UiPaintBlank", "", "", "chi RepresentBegin/End khi mat focus > 10s");
  Fn(d + 0x00468360, "Wnd_RenderWindows", "", "", "");
  Fn(d + 0x0048BA00, "KUiSelServer__OpenWindow", "", "", "new 0x30B8");
  Fn(d + 0x0048B4A0, "KUiSelServer__WndProc", "", "", "msg 0x691 = chon khu vuc");
  Fn(d + 0x0048AF80, "KUiSelServer__OnSelRegion", "int __thiscall f(KUiSelServer *this);", "int __thiscall f(void *this);", "");
  Fn(d + 0x0048ACD0, "KUiSelServer__OnLogin", "int __thiscall f(KUiSelServer *this);", "int __thiscall f(void *this);", "");
  Fn(d + 0x00597E00, "KLogin__GetServerRegionList", "", "", "");
  Fn(d + 0x00597F10, "KLogin__GetServerList", "", "", "");
  Fn(d + 0x00487CF0, "KUiLogin__OpenWindow", "", "", "new 0xEFE8");
  Fn(d + 0x00487DF0, "KUiLogin__WndProc", "", "", "");
  Fn(d + 0x004871D0, "KUiLogin__OnLogin", "int __thiscall f(KUiLogin *this);", "int __thiscall f(void *this);", "");
  Fn(d + 0x004872B0, "KUiLogin__DoAccountLogin", "", "", "MD5 + KLogin__AccountLogin");
  Fn(d + 0x00486670, "KUiLogin__GetInputInfo", "", "", "");
  Fn(d + 0x00599570, "", "", "", "g_LoginLogic (goi tu KUiLogin::OnLogin)");
  Fn(d + 0x00485D90, "KUiConnectInfo__OpenWindow", "", "", "");
  Fn(d + 0x00598FD0, "KLogin__AccountLogin", "", "", "");
  Fn(d + 0x0049D160, "KUiSelPlayer__OpenWindow", "", "", "");
  Fn(d + 0x0049D220, "KUiSelPlayer__UpdateData", "", "", "");
  Fn(d + 0x0049C940, "KUiSelPlayer__OnSelectPlayer", "void __thiscall f(KUiSelPlayer *this, int nSel, int bPlaySound);", "int __thiscall f(void *this, int a1, int a2);", "");
  Fn(d + 0x0049C210, "KUiSelPlayer__OnEnterGame", "int __thiscall f(KUiSelPlayer *this);", "int __thiscall f(void *this);", "");
  Fn(d + 0x00595E50, "KLogin__SelectRole", "int __thiscall f(KLogin *this, int a1);", "int __thiscall f(void *this, int a1);", "");
  Fn(d + 0x00596170, "KLogin__GetRoleInfo", "int __thiscall f(KLogin *this, int a1, int a2);", "int __thiscall f(void *this, int a1, int a2);", "");
  Fn(d + 0x005962C0, "KLogin__GetRoleCount", "int __thiscall f(KLogin *this, int a1);", "int __thiscall f(void *this, int a1);", "");
  FnApprox(d + 0x005F7600, "KNpc__Init", "~ dia chi gan dung: KNpc::Init");

  msg("[JX10_10] Bien toan cuc ...\n");
  Gv(d + 0x009C0474, "Player", "KPlayer *Player;", "PLR_BASE_ADD (Player[1] = minh)");
  Gv(d + 0x01A3B4B4, "Npc", "KNpc *Npc;", "NPC_BASE_ADD, new KNpc[0x320] tai 0x5CCAD6");
  Gv(d + 0x021A1898, "Object", "KObj *Object;", "OBJ_BASE_ADD, KObj[0x100]");
  Gv(d + 0x01F18E14, "Item", "KItem *Item;", "ITM_BASE_ADD, KItem[0x200]");
  Gv(d + 0x021D6840, "g_nMapID", "int g_nMapID;", "MAP_ID_ADD");
  Gv(d + 0x021D6820, "g_szMapName", "char g_szMapName[32];", "MAP_NAME_ADD");
  Gv(d + 0x009BC130, "g_pCoreShell", "void *g_pCoreShell;", "CORE_BASE_ADD");
  Gv(d + 0x009C08A8, "SubWorld", "void *SubWorld;", "WLD_BASE_ADD; [SubWorld]+0x50 = m_dwCurrentTime");
  Gv(d + 0x021A18A8, "NpcSet", "", "NSET_BASE_ADD");
  Gv(d + 0x024E7750, "BuySell", "", "BSL_BASE_ADD");
  Gv(d + 0x01AB4A10, "g_SkillManager", "KSkillInfo g_SkillManager[3000];", "SKL_BASE_ADD = g_SkillManager.m_SkillInfo");
  Gv(d + 0x01E5B330, "g_SkillManager_m_pOrdinSkill", "", "= SKL_BASE_ADD + 0x3A6920");
  Gv(d + 0x01F18A78, "g_Team", "KTeam g_Team;", "TEAM_ADDRESS = &g_Team.m_nCaptain (0x1F18A80)");
  Gv(d + 0x024F1E18, "PlayerSet", "", "");
  Gv(d + 0x024F1E4C, "PlayerSet_m_LevelExp", "int PlayerSet_m_LevelExp[260];", "LEVEL_EXP_TB, level_exp.txt");
  Gv(d + 0x024F4348, "ItemSet", "", "nPriceScale +0xE8 / nMagicScale +0xEC");
  Gv(d + 0x01F18300, "g_MagicAttribNames", "char * g_MagicAttribNames[346];", "ten MAGIC_ATTRIB (khoi tao 0x610C71)");
  Gv(d + 0x009BCBB8, "g_LoginLogic", "KLogin g_LoginLogic;", "m_Choices.Account +0x720 = ACNT_ID_ADD 0x9BD2D8 (dao bit)");
  Gv(d + 0x01F182F0, "g_pRepresent", "void *g_pRepresent;", "RPS_BASE_ADD; vt+0x14 OutputText, +0x4C DrawPrimitives, +0x74 SetOption");
  Gv(d + 0x009BC128, "g_pRepresentShell", "void *g_pRepresentShell;", "UI representshell");
  Gv(d + 0x021A3F18, "g_ScenePlace", "", "KScenePlaceC; regions +0x31E64 (15)");
  Gv(d + 0x0220F9AC, "g_Option", "", "KOption");
  Gv(d + 0x00811D24, "g_MiniMapFlagX", "int g_MiniMapFlagX;", "FLAG_X_OFFST (x>>4, -1 = khong co)");
  Gv(d + 0x00811D28, "g_MiniMapFlagY", "int g_MiniMapFlagY;", "FLAG_Y_OFFST (y>>5)");
  Gv(d + 0x008329C8, "g_UiCursorState", "int g_UiCursorState;", "0x400000 = 'Giai', 0x800000 = OpenStatus");
  Gv(d + 0x008C13D8, "KUiMsgSel_m_pSelf", "KUiMsgSel *KUiMsgSel_m_pSelf;", "MSG_BASE_ADD `滚动选择界面.ini`");
  Gv(d + 0x008C1368, "KUiMsgSel2_m_pSelf", "KUiMsgSel2 *KUiMsgSel2_m_pSelf;", "MSG2_BASE_ADD `npc描述界面.ini`");
  Gv(d + 0x00839890, "KUiPlayerBar_m_pSelf", "KUiPlayerBar *KUiPlayerBar_m_pSelf;", "UPB_BASE_ADD `玩家信息主界面.ini`");
  Gv(d + 0x0084051C, "KUiStoreBox_m_pSelf", "void *KUiStoreBox_m_pSelf;", "SBO_BASE_ADD `储物箱.ini`");
  Gv(d + 0x008478A8, "KUiMsgCentrePad_m_pSelf", "void *KUiMsgCentrePad_m_pSelf;", "MCP_BASE_ADD `消息集合面板_左.ini`");
  Gv(d + 0x00847EF4, "KUiSysMsgCentre_m_pSelf", "void *KUiSysMsgCentre_m_pSelf;", "SMC_BASE_ADD `系统消息.ini`");
  Gv(d + 0x008478E0, "KUiShop_m_pSelf", "KUiShop *KUiShop_m_pSelf;", "SHOP_BASE_ADD `npc买卖界面.ini`");
  Gv(d + 0x008478DC, "KUiGShop_m_pSelf", "void *KUiGShop_m_pSelf;", "`奇珍阁界面.ini`");
  Gv(d + 0x00847EF8, "KUiMiniMap_m_pSelf", "void *KUiMiniMap_m_pSelf;", "MMP_BASE_ADD");
  Gv(d + 0x00840460, "KUiInit_m_pSelf", "KUiInit *KUiInit_m_pSelf;", "UIN_BASE_ADD `开始.ini`");
  Gv(d + 0x00840474, "KUiItem_m_pSelf", "KUiItem *KUiItem_m_pSelf;", "");
  Gv(d + 0x00839A18, "KUiSelServer_m_pSelf", "KUiSelServer *KUiSelServer_m_pSelf;", "`UiNewLogin\\选服务器.ini`");
  Gv(d + 0x008399FC, "KUiLogin_m_pSelf", "KUiLogin *KUiLogin_m_pSelf;", "`UiNewLogin\\登陆.ini`");
  Gv(d + 0x0084053C, "KUiSelPlayer_m_pSelf", "KUiSelPlayer *KUiSelPlayer_m_pSelf;", "`选游戏存档人物.ini`");
  Gv(d + 0x00842460, "g_UiInformation", "KUiInformation g_UiInformation;", "IF1_BASE_ADD `提示.ini`");
  Gv(d + 0x00843B68, "g_UiInformation2", "KUiInformation g_UiInformation2;", "IF2_BASE_ADD `提示2.ini`");
  Gv(d + 0x008C1390, "KUiPrizeSel_m_pSelf_maybe", "", "? `奖励界面.ini` (0x6180)");
  Gv(d + 0x008C139C, "KUiYeSouBox_m_pSelf_maybe", "", "? `给予界面.ini` (0xB7CC)");
  Gv(d + 0x008C0A38, "KUiPlayerStall_m_pSelf_maybe", "", "? `摆摊物品.ini`");
  Gv(d + 0x008C0990, "KUiTongList_m_pSelf_maybe", "", "? `帮会列表窗口.ini`");
  Gv(d + 0x00791784, "KUiPlayerBar_vftable", "", "[4] = WndProc 0x478440");
  Gv(d + 0x00792724, "KUiLogin_vftable", "", "");
  Gv(d + 0x007A5488, "KMyApp_vftable", "", "slot 3 goi qua 0x591830");
  Gv(d + 0x009BC4B8, "g_App", "", "KMyApp");
  Gv(d + 0x009BC508, "g_App_dwLostFocusTime", "unsigned int g_App_dwLostFocusTime;", "app+0x50; != 0 va > 10s -> UiPaintBlank. Ghi 1 de tat hinh");
  Gv(d + 0x009BC790, "g_dwLastPaintTime", "unsigned int g_dwLastPaintTime;", "");
  Gv(d + 0x009BC798, "g_nLogicFrame", "unsigned int g_nLogicFrame;", "bo dem logic (18/s)");
  Gv(d + 0x00815838, "g_nPaintInterval", "int g_nPaintInterval;", "ms/khung = 1000/PaintFps (config.ini [Client])");
  Gv(d + 0x0081D15C, "g_nVisibleNum", "int g_nVisibleNum;", "OPTION 6: so nguoi choi hien thi (mac dinh 200, 0 = an)");
  Gv(d + 0x02213C44, "g_bMissleOpen", "int g_bMissleOpen;", "OPTION 7");
  Gv(d + 0x02213C48, "g_nMissleOpenRow", "int g_nMissleOpenRow;", "OPTION 7 (1..999)");
  Gv(d + 0x01A3B4C0, "g_szNpcTheSame", "", "OPTION 8/9");
  Gv(d + 0x02211174, "g_nOption10", "int g_nOption10;", "OPTION 10 (chua ro)");
  Gv(d + 0x009BC144, "g_nQualityMode", "int g_nQualityMode;", "QualityOpen chon SetOption 6/5");

  msg("[JX10_10] Comment ...\n");
  AddCmt(d + 0x005CCAD6, "Npc = new KNpc[0x320]");
  AddCmt(d + 0x006643B2, "RT_INFO: m_nCurExp 0x7220 / m_nNextExp 0x7228");
  AddCmt(d + 0x0051B150, "lay item dang cam (m_nForeHand) tao doi tuong keo-tha");
  AddCmt(d + 0x006EB658, "AI: SendClientCmdWalk -> DialogNpc");
  AddCmt(d + 0x004C5822, "TEAM_INVITE_RESPONSE 0x0D: nguoi moi o msg+0x108");
  AddCmt(d + 0x005099BD, "[ecx+0x1B98] = KUiMsgSel2 m_nNumMessage");
  AddCmt(d + 0x004746E5, "\"Item_%d\" m_ImmediaItem[9] tai 0x23D0, buoc 0x4F0");
  AddCmt(d + 0x005C8D95, "KSkillManager::Init(ecx = g_SkillManager)");
  AddCmt(d + 0x005D2685, "KSkillManager::Init(ecx = g_SkillManager)");
  AddCmt(d + 0x004923B3, "KUiItem::Init - nut DecomposeEquip +0x7C94");
  AddCmt(d + 0x0049331D, "click DecomposeEquip -> 0x492990");
  AddCmt(d + 0x00492EE0, "click o hanh trang khi dang 'Giai'");
  AddCmt(d + 0x00494A1F, "click o khac khi dang 'Giai'");
  AddCmt(d + 0x00492686, "OperationRequest(0x9B, &KItemObject, 5 UOC_ITEM_TAKE_WITH)");
  AddCmt(d + 0x005C4A4F, "OPR case 0x9B: uGenre=5, uId 1..0x1FF, item+0x6C0 != 1; nParam 5->place 3, 6->place 0x17; goi 0xC6");
  AddCmt(d + 0x005C5181, "OPR case 0x2B GOI_OPTION_SETTING");
  AddCmt(d + 0x0065CF6E, "s2c sync item: ExParam1 (goi +0x2F)");
  AddCmt(d + 0x00610C71, "khoi tao ten MAGIC_ATTRIB -> g_MagicAttribNames");
  AddCmt(d + 0x004B5B10, "SystemMessageArrival: hang cho khi m_pSelf = NULL");
  AddCmt(d + 0x004C3AC6, "DGS_JMP_ADD: VLHook doi jmp nay sang HookDrawGameSpace");
  AddCmt(d + 0x00593A25, "GameInit: config.ini [Client] PaintFps -> g_nPaintInterval = 1000/fps");
  AddCmt(d + 0x00591752, "GetTickCount - g_App_dwLostFocusTime >= 10000");
  AddCmt(d + 0x005917C9, "goi UiPaint (duy nhat)");
  AddCmt(d + 0x00591818, "Sleep(1): push ebp (ebp=1)");
  AddCmt(d + 0x00678D12, "g_nVisibleNum truoc CoreDrawGameObj");
  AddCmt(d + 0x006B30C9, "MissleOpen: thay hieu ung skill");

  msg("[JX10_10] Xong: dat ten %d, bo qua %d, loi %d\n", gOk, gSkip, gErr);
  refresh_idaview_anyway();
}
