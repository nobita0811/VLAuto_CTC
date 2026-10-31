//GameDef.h
#pragma once
#define QUANGCAOAUTO	0
#define CLIENT_SERIAL		683251

enum OBJ_ATTRIBYTE_TYPE
{
	series_undef = -1,
	series_metal = 0,
	series_wood,
	series_water,
	series_fire,
	series_earth,
	series_num,
};

enum NPCCAMP
{
	camp_begin,
	camp_justice,
	camp_evil,
	camp_balance,
	camp_free,
	camp_animal,
	camp_event,
	camp_num,
};

enum NPC_RELATION
{
	relation_none	= 1,
	relation_self	= 2,
	relation_ally	= 4,
	relation_enemy	= 8,
	relation_dialog	= 16,
	relation_all	= relation_none | relation_ally | relation_enemy | relation_self | relation_dialog,	
	relation_num,
};

enum NPCKIND
{
	kind_normal,
	kind_player,
	kind_partner,
	kind_dialoger,
	kind_bird,
	kind_mouse,
    kind_num,
};

enum NPCCMD
{
	do_none,
	do_stand,
	do_walk,
	do_run,
	do_jump,
	do_skill,
	do_magic,
	do_attack,
	do_sit,
	do_hurt,
	do_death,
	do_defense,
	do_idle,
	do_revive = 21,
};

enum OBJKIND
{
	Obj_Kind_MapObj,
	Obj_Kind_Body,
	Obj_Kind_Box,
	Obj_Kind_Item,
	Obj_Kind_Money,
	Obj_Kind_StoreBox = 10
} ;

enum INVENTORY_ROOM
{
	room_equipment = 0,
	room_repository,
	room_trade,
	room_tradeback,
	room_trade1,
	room_immediacy,
	room_06,
	room_repo02,
	room_repo03,
	room_09,
	room_yesou,
	room_0a,
	room_0b,
	room_0c,
	room_guid,
	room_num,
};

enum ITEM_POSITION
{
	pos_hand = 1,
	pos_equip,
	pos_equiproom,
	pos_repositoryroom,
	pos_traderoom,
	pos_trade1,
	pos_immediacy,
	pos_expand,
	pos_repo02,
	pos_repo03,
	pos_yesoubox = 12,
	pos_guid = 23,
};

enum ITEMGENRE
{
	item_equip = 0,
	item_medicine,
	item_mine,
	item_materials,
	item_task,
	item_townportal,
	item_special,
	item_number,
};

enum EQUIPDETAILTYPE
{
	equip_meleeweapon = 0,
	equip_rangeweapon,
	equip_armor,
	equip_ring,
	equip_amulet,
	equip_boots,
	equip_belt,
	equip_helm,
	equip_cuff,
	equip_pendant,
	equip_horse,
	equip_mask,
	equip_detailnum,
};

enum MEDICINEDETAILTYPE
{
	medicine_blood = 0,
	medicine_mana,
	medicine_both,
	medicine_stamina,
	medicine_antipoison,
	medicine_detailnum,
	medicine_both_ex = 8,
};

enum ITEM_PART
{
	itempart_head = 0,
	itempart_body,
	itempart_belt,
	itempart_weapon,
	itempart_foot,
	itempart_cuff,
	itempart_amulet,
	itempart_ring1,
	itempart_ring2,
	itempart_pendant,
	itempart_horse,
	itempart_mask,
	itempart_pp,
	itempart_an,
	itempart_ts,
	itempart_ts1,
	itempart_ts2,
	itempart_num,
};

enum UIOBJECT_CONTAINER
{
	UOC_IN_HAND	= 1,
	UOC_GAMESPACE,
	UOC_IMMEDIA_ITEM,
	UOC_IMMEDIA_SKILL,
	UOC_ITEM_TAKE_WITH,
	UOC_GUID_BOX,
	UOC_TRADE_BOX,
	UOC_EQUIPTMENT = 9,
	UOC_NPC_SHOP,
	UOC_STORE_BOX,
	UOC_STORE_BOX2,
	UOC_STORE_BOX3,
	UOC_YESOU_BOX = 20,
	UOC_HTINH1_BOX,		// Thang cap Huyen Tinh
	UOC_HTINH2_BOX,		// Thang cap Khoang Thach
	UOC_HTINH3_BOX,		// Tinh luyen Huyen Tinh
};

enum CORE_OBJECT_GENRE
{
	CGOG_NOTHING = 0,
	CGOG_MONEY,
	CGOG_PLAYER_FACE,
	CGOG_OBJECT,
	CGOG_SKILL,
	CGOG_ITEM,
	CGOG_NPC,
	CGOG_MENU_NPC,
	CGOG_NPC_BLUR,
	CGOG_MISSLE,
	CGOG_EQUIP,
	CGOG_NPCSELLITEM,
};

enum GAMEDATA_INDEX
{
	GDI_GAME_OBJ_DESC = 1,
	GDI_GAME_OBJ_DESC_INCLUDE_TRADEINFO,
	GDI_GAME_OBJ_DESC_INCLUDE_REPAIRINFO,
	GDI_GAME_OBJ_LIGHT_PROP,
	GDI_PLAYER_BASE_INFO,
	GDI_TRADE_ITEM_PRICE = 0x0BBA,
	GDI_STALL_ITEMS = 0x0BBF,		// 15-10: hang tren sap dang xem (uParam = KStallObject[], nParam = so mon; 0,0 = dem)
	GDI_STALL_OWNER = 0x0BC0,		// 15-10: ten chu sap
};

enum OPTIONS_LIST
{
	OPTION_PERSPECTIVE,
	OPTION_DYNALIGHT,
	OPTION_MUSIC_VALUE,
	OPTION_SOUND_VALUE,
	OPTION_BRIGHTNESS,
	OPTION_WEATHER,
	OPTION_VISIBLE_NUM,		// 10-10: [Options] VisibleNum - so nguoi choi hien thi (0 = an), ghi [0x81D15C]
	OPTION_MISSLE,			// 10-10: [Options] MissleOpen - 1..999 thay hieu ung skill, khac = tat
};

enum GAMEOPERATION_INDEX
{
	GOI_EXIT_GAME = 1,
	GOI_SWITCH_OBJECT_QUERY,
	GOI_SWITCH_OBJECT,
	GOI_REJECT_OBJECT,
	GOI_MONEY_INOUT_STORE_BOX,
	GOI_PLAYER_ACTION,
	GOI_PLAYER_RENASCENCE,
	GOI_INFORMATION_CONFIRM_NOTIFY,
	GOI_QUESTION_CHOOSE,
	GOI_USE_ITEM,
	GOI_WEAR_EQUIP,
	GOI_USE_SKILL,
	GOI_SET_IMMDIA_SKILL,
	GOI_TRADE_NPC_BUY = 0x18,
	GOI_TRADE_NPC_CHECK,
	GOI_TRADE_NPC_SELL,
	GOI_TRADE_NPC_REPAIR,
	GOI_TRADE_PLAYER_SETPRICE = 0x45,
	GOI_TRADE_PLAYER_GETPRICE,
	GOI_TRADE_PLAYER_SELL,
	GOI_TRADE_PLAYER_BUY,
	GOI_OPTION_SETTING = 0x2B,	// 10-10: case OPR 0x5C5181 (uParam = OPTION_*, nParam = gia tri)
	GOI_SHIFT_ITEM = 0x59,
	GOI_DECOMPOSE_EQUIP = 0,	// 15-10: KHONG CO (10-10: 0x9B Ra trang bi "DecomposeEquip" -> protocol 0xC6). 0 = tat
	GOI_MAP_GOTO = 0x81,		// 15-10: tu di toi o ban do (SetMapFlag 0x4DE840; 10-10: 0x85). Tu 0x6D tro di ma OPR lui 4 so voi 10-10
};

enum TEAMOPERATION_INDEX
{
	TEAM_CREATE = 0x05,
	TEAM_APPOINT,
	TEAM_INVITE,
	TEAM_KICK,
	TEAM_LEAVE,
	TEAM_JOIN_RESPONSE = 0x0C,
	TEAM_INVITE_RESPONSE = 0x0D,
};

enum PROCESSPEOPLE_INDEX
{
	ACTION_CHAT = 0,
	ACTION_MAKEFRIEND,
	ACTION_TRADE,
	ACTION_JOINTEAM,
	ACTION_INVITETEAM,
	ACTION_FOLLOW,
	ACTION_VIEWITEM,
	ACTION_TONG,
	ACTION_BLACKLIST = 9,
	ACTION_REVENGE = 10,
};

enum MAGIC_ATTRIB
{
	// 15-10: ID lay tu bang ten thuoc tinh trong exe (mang con tro 0xEE0278 + 4*ID; 10-10: 0x1F18300).
	// ID 0..293 giong 10-10 (47 doi ten require_fortune_value, them ten 256/257/275), tu 294 khac - xem Offsets_15_10.md
	item_begin = 27,
	weapondamagemin_v,
	weapondamagemax_v,
	armordefense_v,		// 30
	durability_v,
	requirestr,
	requiredex,
	requirevit,
	requireeng,
	requirelevel,
	requireseries,
	requiresex,
	requiremenpai,
	weapondamageenhance_p,		// 40
	armordefenseenhance_p,
	requirementreduce_p,
	indestructible_b,
	item_usagetime_v,
	req_levelortranslife,
	require_translife,
	item_reserve4,
	item_reserve5,
	item_reserve6,
	item_reserve7,		// 50
	item_reserve8,
	item_reserve9,
	item_reserve10,
	item_end,
	damage_begin,
	attackrating_v,
	attackrating_p,
	ignoredefense_p,
	physicsdamage_v,
	colddamage_v,		// 60
	firedamage_v,
	lightingdamage_v,
	poisondamage_v,
	magicdamage_v,
	physicsenhance_p,
	steallife_p,
	stealmana_p,
	stealstamina_p,
	knockback_p,
	deadlystrike_p,		// 70
	fatallystrike_p,
	stun_p,
	addskillexp1,
	addskillexp2,
	seriesdamage_p,
	damage_reserve4,
	damage_reserve5,
	damage_reserve6,
	damage_reserve7,
	damage_reserve8,		// 80
	damage_reserve9,
	damage_reserve10,
	damage_end,
	normal_begin,
	lifemax_v,
	lifemax_p,
	life_v,
	lifereplenish_v,
	manamax_v,
	manamax_p,		// 90
	mana_v,
	manareplenish_v,
	staminamax_v,
	staminamax_p,
	stamina_v,
	staminareplenish_v,
	strength_v,
	dexterity_v,
	vitality_v,
	energy_v,		// 100
	poisonres_p,
	fireres_p,
	lightingres_p,
	physicsres_p,
	coldres_p,
	freezetimereduce_p,
	burntimereduce_p,
	poisontimereduce_p,
	poisondamagereduce_v,
	stuntimereduce_p,		// 110
	fastwalkrun_p,
	visionradius_p,
	fasthitrecover_v,
	allres_p,
	attackspeed_v,
	castspeed_v,
	meleedamagereturn_v,
	meleedamagereturn_p,
	rangedamagereturn_v,
	rangedamagereturn_p,		// 120
	addphysicsdamage_v,
	addfiredamage_v,
	addcolddamage_v,
	addlightingdamage_v,
	addpoisondamage_v,
	addphysicsdamage_p,
	slowmissle_b,
	changecamp_b,
	physicsarmor_v,
	coldarmor_v,		// 130
	firearmor_v,
	poisonarmor_v,
	lightingarmor_v,
	damage2addmana_p,
	lucky_v,
	steallifeenhance_p,
	stealmanaenhance_p,
	stealstaminaenhance_p,
	allskill_v,
	metalskill_v,		// 140
	woodskill_v,
	waterskill_v,
	fireskill_v,
	earthskill_v,
	knockbackenhance_p,
	deadlystrikeenhance_p,
	stunenhance_p,
	badstatustimereduce_v,
	manashield_p,
	adddefense_v,		// 150
	adddefense_p,
	fatallystrikeenhance_p,
	lifepotion_v,
	manapotion_v,
	physicsresmax_p,
	coldresmax_p,
	fireresmax_p,
	lightingresmax_p,
	poisonresmax_p,
	allresmax_p,		// 160
	coldenhance_p,
	fireenhance_p,
	lightingenhance_p,
	poisonenhance_p,
	magicenhance_p,
	attackratingenhance_v,
	attackratingenhance_p,
	addphysicsmagic_v,
	addcoldmagic_v,
	addfiremagic_v,		// 170
	addlightingmagic_v,
	addpoisonmagic_v,
	fatallystrikeres_p,
	statusimmunity_b,
	expenhance_v,
	expenhance_p,
	seriesres_p,
	seriesenhance_p,
	createnpc,
	autoattacknpc,		// 180
	dynamicmagicshield_v,
	nomovespeed,
	execscript,
	changefeature1,
	changefeature2,
	stealfeature,
	addstealfeatureskill,
	lucky_v_partner,
	listen_msg,
	lifereplenish_p,		// 190
	ignoreskill_p,
	returnskill_p,
	poisondamagereturn_v,
	poisondamagereturn_p,
	autoreplyskill,
	autoattackskill,
	autorescueskill,
	autodeathskill,
	randmove,
	hide,		// 200
	ignorenegativestate_p,
	poison2decmana_p,
	staticmagicshield_v,
	staticmagicshield_p,
	returnres_p,
	add120skillexpenhance_p,
	dec_percasttimehorse,
	dec_percasttime,
	enhance_709_auto,
	enhance_708_life_p,		// 210
	enhance_93_life_v,
	enhance_711_auto,
	enhance_714_auto,
	enhance_717_auto,
	enhance_723_miss_p,
	showpersoninfo,
	ignoredamage,
	sorbdamage_p,
	anti_hitrecover,
	anti_stuntimereduce_p,		// 220
	anti_poisonres_p,
	anti_fireres_p,
	anti_lightingres_p,
	anti_physicsres_p,
	anti_coldres_p,
	block_rate,
	enhancehit_rate,
	poisonres_yan_p,
	lightingres_yan_p,
	fireres_yan_p,		// 230
	physicsres_yan_p,
	coldres_yan_p,
	lifemax_yan_v,
	lifemax_yan_p,
	manamax_yan_v,
	manamax_yan_p,
	sorbdamage_yan_p,
	fastwalkrun_yan_p,
	attackspeed_yan_v,
	castspeed_yan_v,		// 240
	allres_yan_p,
	anti_maxres_p,
	skill_enhance,
	magicdamage_p,
	fasthitrecover_yan_v,
	five_elements_enhance_v,
	five_elements_resist_v,
	manareplenish_p,
	add_damage_p,
	forbit_attack,		// 250
	frozen_action,
	forbit_takemedicine,
	invincibility,
	not_add_pkvalue_p,
	add_boss_damage,
	pk_punish_weaken,			// 256 (15-10: ten moi)
	pk_punish_enhance,			// 257
	anti_poisontimereduce_p = 258,
	do_hurt_p,
	anti_do_hurt_p,		// 260
	do_stun_p,
	anti_do_stun_p,
	anti_physicsres_yan_p,
	anti_poisonres_yan_p,
	anti_coldres_yan_p,
	anti_fireres_yan_p,
	anti_lightingres_yan_p,
	anti_allres_yan_p,
	anti_sorbdamage_yan_p,
	anti_block_rate,		// 270
	anti_enhancehit_rate,
	autocastskill,
	transfigure,
	oncastskill,
	enhancehiteffect_rate,		// 275 (15-10: ten moi)
	me2metaldamage_p = 276,
	metal2medamage_p,
	me2wooddamage_p,
	wood2medamage_p,
	me2waterdamage_p,		// 280
	water2medamage_p,
	me2firedamage_p,
	fire2medamage_p,
	me2earthdamage_p,
	earth2medamage_p,
	meleedamagereturnmana_p,
	rangedamagereturnmana_p,
	reduceskillcd1,
	reduceskillcd2,
	reduceskillcd3,		// 290
	clearallcd,
	addblockrate,
	walkrunshadow,
	returnskill2enemy,			// 294 - 15-10: KHONG CON (bo trong)
	candetonate1,
	candetonate2,
	candetonate3,
	manatoskill_enhance,
	melee_returnres_p,			// 299 (15-10; 10-10: movedistanc_710_enhance)
	range_returnres_p,			// 300 (15-10; 10-10: validtiem_1366_enhance)
	forbit_usetownportal,		// 301 (15-10; 10-10: daoxutian_enhance)
	factionreduction_rate,		// 302 (15-10; 10-10: foxinciyou_enhance)
	require_prestige_value = 501,	// 15-10: 303..500 trong
	anit_allres_p,
	coldweaken_p,
	longqiyunxiang,
	anti_enhancehiteffect_rate,	// 505
	decrease_damage,
	normal_end,					// 507 (15-10; 10-10: 308)

	// Ten cu (truoc 10-10) - giu de code/ita cu van dung
	damagetomana_p = damage2addmana_p,	// 134
	magicshield = dynamicmagicshield_v,	// 181
};

enum ITEM_KEEP_CAUSE
{
	keep_nothing,
	keep_not_equip,
	keep_locked_equip,
	keep_golden_equip,
	keep_violet_equip,
	keep_special_equip,
	keep_save_jewellery,
	keep_save_ring,
	keep_save_lace,
	keep_mask_equip,
	keep_good_horse,
	keep_red_item,
	keep_high_price,
	keep_high_level,
	keep_good_attrib,
	keep_good_attrib1,
	keep_good_attrib2,
	keep_good_attrib3,
	keep_good_attrib4,
	keep_good_attrib5,
};

enum CITY_MAP_ID
{
	city_phuongtuong = 1,
	city_tuongduong = 78,
	city_bienkinh = 37,
	city_thanhdo = 11,
	city_daily = 162,
	city_duongchau = 80,
	city_laman = 176,
	//Small Towns
	city_longmon = 121,
	city_vinhlac = 99,
	city_balang = 53,
	city_chutien = 100,
	city_giangtan = 20,
	city_thachco = 153,
	city_daohuong = 101,
	city_longtuyen = 174,
	//Ban do Tin su
	city_phongky1 = 387,
	city_phongky2,
	city_phongky3,
	city_sonmieu1,
	city_sonmieu2,
	city_sonmieu3,
	city_baokho1,
	city_baokho2,
	city_baokho3,
	// Vi Son dao
	city_visondao = 342,
	//Khu vuc Bang
	city_banghoimin = 586,
	city_banghoimax = 604,
};

enum _ATTACK_SKILL
{
	skill_supp01,
	skill_supp02,
	skill_supp03,
	skill_left,
	skill_right,
	skill_boss,
	skill_defence,
	skill_total
};

enum TASK_CODES
{
	ys_taskgid = 1020,
	ys_majorid,
	ys_maphave = 1025,
	ys_repvalue,	// Chi so PD & DV
	ys_shxtmap,		// So luong ban do SHXT / CTC 20-10 đổi thành đánh quái
	ys_taskstatus,
	ys_task405,
	ys_minorid,
	ys_mapid,
	ys_smapreq,
	ys_prelevel,
	ys_preexp,
	ys_tasknum = 1044,
	ys_taskccl = 1046,	// Task Cancel
	pck_marks = 1207,
	pck_time = 0x04C6,
	bh_lsttask = 0x0515,
	bh_pklbid = 0x06D0,
	bh_pkqty = 0x06D1,
	bh_pkkind = 0x06D2,
	bh_pkfirst = 0x06D3,
	bh_pklast = 0x06DB,
	bh_pklevel = 0x06DC,
	bh_curtask = 0x0899,
	bh_tasktime = 0x0A06,
	ys_preexp_ex = 2574
};
enum TASK_CODES_EX
{
	ys_attackNpc = 1, // đánh quái theo tên
	ys_ItemOption, // tìm đồ theo thuộc tính và giao cho Dã Tẩu

};
enum QUEST_ACTIONS
{
	act_auto,
	act_cancel,
	act_useitem,
	act_ignore,
};

enum _PRICE_ITEM
{
	price_ring,
	price_pendant,
	price_other,
	price_fuyuan01,
	price_fuyuan02,
	price_fuyuan03,
	price_satthugian,
};

enum _PRIZE_ITEM
{
	prize_hoangkimmp,
	prize_nhutinhao,
	prize_nhutinhnb,
	prize_nhutinhot,
	prize_hiepcotnb,
	prize_hiepcotot,
	prize_anbangdc,
	prize_anbangot,
	prize_dinhquocmu,
	prize_dinhquocot,
	prize_ttthbt,
	prize_huyentinh,
	prize_vlmtttk,
	prize_tlhbntk,
	prize_bikipmp,
	prize_ttl,
	prize_kinhnghiem,
	prize_phucduyen,
	prize_dauhoi,
	prize_cohoihuy,
	prize_nkhoang,
	prize_nthach,
	prize_tsblo,
	prize_bqualo,
	prize_duochoan,
	prize_phaohoa,
	prize_tttanhphu,
	prize_levelht,
	prize_other,
};

enum UTIL_TASKS
{
	util_moveitem = 1,
	util_makeht,
	util_upgradeht,
	util_givekcl,
	util_buyitem,
	util_opengif,
	util_doevent,
	//Bang hoi
	util_getlenhbai,
};

enum SENDCMD
{
	cmd_none,
	cmd_amulet,			// Thong bao da tho dia phu
	cmd_autohit,		// Tu dong danh quai
	cmd_autoarng,		// Tu dong xep do
	cmd_savepoint,		// Ghi nho diem
	cmd_setpoint,		// Danh quanh diem, theo toa do
	cmd_setmap,			// Ve thanh tu quay lai
	cmd_setyesou,		// Tu lam nv Da Tau
	cmd_loadini,		// Load ini file
	cmd_activate,		// Activate GameWindow
	cmd_exit,			// Gui lenh thoat Game (tu VLAuto)
	cmd_exitall,		// Gui lenh thoat tat ca Game (tu Game)
	cmd_stop,			// Gui lenh STOP Hook
	cmd_start,			// Gui lenh START Hook
	cmd_restart,		// Gui lenh RESTART Hook
	cmd_beginpk,		// Chuan bi gui lenh den AutoLogin
	cmd_endpk,			// Gui lenh thoat nv phu
	cmd_showme,			// Gui lenh Hien VLAuto
	cmd_attack,			// Gui lenh tan cong doi thu
	cmd_useam,			// Da dung 1 Tho Dia Phu / Hoi Thanh Phu (VLAuto dem cho bang Thong ke)
	cmd_tongmoney,		// VLAuto -> Game: gop lParam luong tien tren nguoi vao quy bang (bang Thong ke > Gom tien)
	cmd_walkcitybox,	// VLAuto -> Game: chay bo (MapPath, khong xa phu / vat pham) toi map lParam roi mo ruong de luu ruong (tab Tro giup)
};

//=============================== DEBUG ===============================
// VLAuto -> VLHook : PostMessage(hWndGame, WM_HOOK_DEBUG, (WPARAM)&DEBUG_PARAM, sizeof(DEBUG_PARAM))
// VLHook -> VLAuto : SendMessage(hWndReply, WM_COPYDATA, hWndGame, &COPYDATASTRUCT)
//                    dwData = DEBUG_COPYDATA_ID, lpData = DEBUG_REPLY (szText: chuoi TCVN3)
#define DEBUG_COPYDATA_ID	0x56444247	// 'VDBG'
#define DEBUG_TEXT_SIZE		4096
#define DEBUG_RAW_MARK		'\x02'	// Dong bat dau bang ky tu nay: VLAuto hien nguyen byte TCVN3 (cp1252), khong doi sang Unicode

enum DEBUG_CMD
{
	dbg_none,
	// Tab Di chuyen (VLMove.cpp)
	dbg_mv_status,			// Trang thai Player + dieu kien chay CVLMove::Breathe
	dbg_mv_getmappos,		// P_PLAYER->GetMapPos
	dbg_mv_moveto,			// g_pCurPlayer->MoveTo(x, y)
	dbg_mv_npcmoveto,		// P_PLAYER->MoveTo(x, y) - KNpc::MoveTo (MVT_FUNC_ADD)
	dbg_mv_movetonpc,		// g_pCurPlayer->MoveTo(g_pNpcArray[idx])
	dbg_mv_dosit,			// g_pCurPlayer->DoSit
	dbg_mv_horse,			// ExecuteScript(SCK_SHORTCUT_HORSE)
	dbg_mv_gettaskval,		// g_pCurPlayer->GetTaskVal(n)
	dbg_mv_npclist,			// g_pNpcArray[i]->IsExist/Name/GetMapPos
	dbg_mv_findnpc,			// Tim NPC theo ten (nhu CVLMove::GetLeaderChar, khong tac dong)
	dbg_mv_setpeople,		// theAttack.SetPeopleIdx(idx) - PEOPLE_IDX
	dbg_mv_getflag,			// Doc FLAG_X_OFFST/FLAG_Y_OFFST
	dbg_mv_setflag,			// KUiMiniMap::SetMapFlag(dx, dy)
	dbg_mv_team,			// g_pTeam, IAM_IN_A_PARTY
	dbg_mv_teamleave,		// g_pCoreShell->TeamOperation(TEAM_LEAVE, 0, 0)
	dbg_mv_msgsel2,			// KUiMsgSel2 (MSG2_BASE_ADD): IsVisible, noi dung
	dbg_mv_msgsel2click,	// KUiMsgSel2::OnClickMsg(n)
	dbg_mv_info2,			// g_UiInformation2->IsVisible/Hide
	dbg_mv_sysmsg,			// SystemMessage(text)
	dbg_mv_savepoint,		// theMove.SavePoint (Ctrl+S)
	dbg_mv_mapinfo,			// *g_pMapID, g_szMapName, theMove map/path dang nap
	dbg_mv_goto,			// g_pCurPlayer->GoTo(x, y) - tu tim duong (GOTO_FUNC_ADD)
	// Tab Ban do (VLMap.cpp)
	dbg_mp_status = 100,	// Trang thai CVLMap + buoc Breathe se lam
	dbg_mp_citymap,			// CVLMap::LoadMap(*g_pMapID) + NpcPoints, NearestDriver
	dbg_mp_cityinfo,		// GetOwnerCity/GetSubCity/GetCityName/GetReturnCity/GetMapName
	dbg_mp_npcatpoint,		// NPC dialoger gan NpcPoints[idx] (nhu ClickToNpc, khong tac dong)
	dbg_mp_clicknpc,		// theMap.ClickToNpc(NpcPoints[idx])
	dbg_mp_runtopos,		// theMap.RunningToPos(NpcPoints[idx]) - 1 buoc
	dbg_mp_menu,			// KUiMsgSel/KUiMsgSel2: GetMenuCount + noi dung
	dbg_mp_clickmenuidx,	// theMain.ClickMenuIdx(idx)
	dbg_mp_clickmenustr,	// theMain.ClickMenuStr(text)
	dbg_mp_closeall,		// CloseAllWindows(0)
	dbg_mp_shop,			// KUiShop: danh sach hang + GetGameData(GDI_TRADE_ITEM_PRICE)
	dbg_mp_buyitem,			// g_pCurPlayer->BuyItem(idx)
	dbg_mp_inventory,		// Tien, CheckRoom, vat pham trong hanh trang
	dbg_mp_itemcount,		// g_pCurPlayer->GetItemsCount(text)
	dbg_mp_sellitem,		// g_pCurPlayer->SellItem(idx)
	dbg_mp_useitem,			// g_pCurPlayer->UseItem(text)
	dbg_mp_eatmedicine,		// g_pCurPlayer->EatMedicine(idx)
	dbg_mp_equipdur,		// Do ben trang bi (m_EquipItem)
	dbg_mp_repairitem,		// g_pCurPlayer->RepairItem(idx)
	dbg_mp_storebox,		// SBO_BASE_ADD + vi tri ruong (KObj StoreBox / NPC)
	dbg_mp_openstore,		// theMap.OpenStoreBox()
	dbg_mp_lockinfo,		// ProcessGameSecurity(1/0)
	dbg_mp_unlock,			// theMain.OpenLockRoom(theMap.m_nRepositoryPass)
	dbg_mp_lock,			// theMain.CloseLockRoom()
	dbg_mp_moneyin,			// OperationRequest(GOI_MONEY_INOUT_STORE_BOX, TRUE, idx)
	dbg_mp_moneyout,		// OperationRequest(GOI_MONEY_INOUT_STORE_BOX, 0, idx)
	dbg_mp_handtoroom,		// Do tren tay: EQUIP_ROOM.FindRoom + SwitchItem
	dbg_mp_checkready,		// Dieu kien CVLMap::CheckReady (chi doc)
	dbg_mp_script,			// ExecuteScript(text)
	dbg_mp_saverepo,		// theMap.SaveNewRepository (Ctrl+L)
	dbg_mp_decompose,		// g_pCurPlayer->DecomposeItem(idx) - Ra trang bi
	dbg_mp_showwalk,		// Bat/tat ve duong Auto dang di trong game (MapPath, City.map, MoveTo, danh sach toa do)
	dbg_mp_dialog,			// Loi thoai NPC (KUiMsgSel2 +0x2578, KUiMsgSel m_InfoText) + KUiInformation/2
	dbg_mp_tongmoney,		// Gop quy bang: TongOperation(4, {2 (X), 0, Idx}, buf)
	dbg_mp_filelog,			// Bat/tat hook engine.dll ghi file/lua game nap (KPakFile::Open, KLuaScript::Load/DoString/CallFunction)
	dbg_mp_filelogview,		// Xem log file/lua (Chuoi = loc, X = 1 xoa sau khi xem), luu Logs\FileLog.txt

	// Tab Nhat do (VLCollect.cpp)
	dbg_cl_status = 200,	// Tuy chon + CVLCollect::Breathe dung o dau
	dbg_cl_objlist,			// g_pObjArray + CheckObj (ly do bo qua)
	dbg_cl_checkobj,		// CheckObj(KObj[Idx]) chi tiet
	dbg_cl_pickup,			// g_pCurPlayer->PickUpObj(Idx)
	dbg_cl_movetoobj,		// g_pCurPlayer->MoveTo(KObj[Idx])
	dbg_cl_searchobj,		// theCollect.SearchObj(tam) 1 lan
	dbg_cl_blacklist,		// LoadBlackList + IsInBlackList(Chuoi)
	dbg_cl_redlist,			// LoadRedList + IsInRedList(Chuoi)
	dbg_cl_keepitems,		// KeepTheItem moi vat pham hanh trang
	dbg_cl_itemattrib,		// Thuoc tinh KItem[Idx] + CheckItemAttrib
	dbg_cl_room,			// Luoi EQUIP_ROOM + FindRoom
	dbg_cl_handitem,		// Do tren tay: CheckHandItem se lam gi (chi doc)
	dbg_cl_takeitem,		// SwitchItem(UOC_ITEM_TAKE_WITH) cam vat pham Idx
	dbg_cl_checkhand,		// theCollect.CheckHandItem(PLAY_HAND_IDX)
	dbg_cl_filter,			// theCollect.FilterItems()
	dbg_cl_arrange,			// theCollect.ArrangeItems()
	dbg_cl_throwaway,		// ExecuteScript("ThrowAwayItem()")
	dbg_cl_badobjs,			// g_BadObjs
	dbg_cl_decomposescan,	// Quet do co the Ra (FilterItems + core 0x9B), Idx = 1: xoa m_nDecomposeFail
	dbg_cl_decompose,		// g_pCurPlayer->DecomposeItem(Idx)
	dbg_cl_invnames,		// Ten vat pham trong hanh trang (moi dong 1 ten, khong trung) - cua so Cai dat nhat do
	dbg_cl_nosavelist,		// LoadNoSaveList (Maps\RuongItem.txt) + IsInNoSaveList(Chuoi)

	// Tab MapPath (VLDebugPath.cpp) - ghi/kiem tra du lieu duong di Maps\Path\<MapID>_<Ten map>.txt
	dbg_pt_status = 300,	// Trang thai ghi + do thi map hien tai
	dbg_pt_rec,				// Bat/tat tu ghi (Idx = khoang cach)
	dbg_pt_addpoint,		// Them diem tai vi tri hien tai (noi diem truoc)
	dbg_pt_undo,			// Xoa diem cuoi
	dbg_pt_delnear,			// Xoa diem gan nhat
	dbg_pt_setlast,			// Diem gan nhat lam diem truoc (re nhanh)
	dbg_pt_link,			// Noi diem gan nhat voi diem Idx
	dbg_pt_unlink,			// Bo noi diem gan nhat voi diem Idx
	dbg_pt_door,			// Diem gan nhat la cong sang map Idx
	dbg_pt_driver,			// Ghi xa phu: menu dang mo -> cac map
	dbg_pt_undotrans,		// Huy cong/xa phu vua tu ghi khi doi map
	dbg_pt_list,			// In do thi
	dbg_pt_save,			// Ghi file
	dbg_pt_load,			// Doc lai file
	dbg_pt_import,			// Nhap file cu (.map / CityMaps / .pth)
	dbg_pt_clear,			// Xoa do thi map hien tai
	dbg_pt_route,			// Dijkstra diem gan nhat -> diem Idx
	dbg_pt_run,				// Chay thu toi diem Idx
	dbg_pt_stop,			// Dung chay thu
	dbg_pt_maproute,		// BFS giua cac map -> map Idx
	dbg_pt_show,			// Bat/tat ve node trong game + phim Space/Delete/End
	dbg_pt_scan,			// Quet cong quanh nhan vat: trap ban do, Obj, NPC (Idx = pham vi)
	dbg_pt_doortrap,		// Ghi cong = trap gan nhat -> map Idx
	dbg_pt_doorobj,			// Ghi cong = Obj gan nhat (click) -> map Idx
	dbg_pt_doornpc,			// Ghi cong = NPC doi thoai gan nhat -> map Idx
	dbg_pt_trydoor,			// Thu chay vao toa do X/Y (trong = co): qua map => ghi cong
	dbg_pt_runxy,			// Chay thu toi toa do X/Y (theo do thi roi di thang)
	dbg_pt_doorxy,			// Ghi cong tai toa do X/Y -> map Idx
	dbg_pt_obstacle,		// Bat/tat ve o vat can (m_ObstacleInfo)
	dbg_pt_obsavoid,		// Bat/tat ne vat can trong KPlayer::MoveTo (Idx/X/Y: thu tim duong)
	dbg_pt_last,

	// Tab Chien dau (VLAttack.cpp)
	dbg_at_status = 400,	// Tuy chon + CVLAttack::Breathe dung o dau
	dbg_at_skills,			// P_PLAYER->m_Skills + KSkill (ten, tam, ngua)
	dbg_at_skillinfo,		// KSkill[Idx]
	dbg_at_states,			// KStateNode tren nguoi + HandleDirectSkills
	dbg_at_npclist,			// Quai quanh + loc RefreshNpcs (Idx = khoang cach)
	dbg_at_search,			// RefreshNpcs + SearchNearestNpc 1 lan (Idx = tam)
	dbg_at_attacknpc,		// SetActiveSkill + SetPeopleIdx(Idx)
	dbg_at_islag,			// IsLagTarget(GetCurTargetIdx())
	dbg_at_badnpc,			// m_BadNpc (Idx = 1: xoa)
	dbg_at_setdefskills,	// SetDefSkills(X, Y)
	dbg_at_setactive,		// SetActiveSkill(Idx)
	dbg_at_castself,		// SendCommand(do_skill) + SendVLCmdSkill(Idx, -1, minh)
	dbg_at_casttarget,		// SendVLCmdSkill(Idx, -1, Npc[X])
	dbg_at_shortcut,		// DoSpecialSkill(Idx, X)
	dbg_at_players,			// Nguoi choi quanh + AutoDefence
	dbg_at_autodefence,		// AutoDefence() x3
	dbg_at_pk,				// ExecuteScript("Switch('PK',Idx)")
	dbg_at_autohit,			// Dao m_bAutoHit + SetAutoHit (Ctrl+A)
	dbg_at_watchtarget,		// Bat/tat theo doi muc tieu Breathe dang danh (SystemMessage kenh 1)

	// Tab Hoi phuc (VLRecover.cpp)
	dbg_rc_status = 500,	// Tuy chon + CVLRecover::Breathe se lam gi
	dbg_rc_medicine,		// Thuoc trong hanh trang theo EatMedicine
	dbg_rc_eat,				// g_pCurPlayer->EatMedicine(Idx)
	dbg_rc_equipdur,		// Do ben trang bi + GetRepairPrice (CheckEquip)
	dbg_rc_gshop,			// KUiPlayerBar::OnClickGShop
	dbg_rc_msgsel,			// KUiMsgSel + doc gia "Sua ky ["
	dbg_rc_msgclick,		// KUiMsgSel::OnClickMsg(Idx)
	dbg_rc_repair,			// g_pCurPlayer->RepairItem(Idx)
	dbg_rc_amcheck,			// OnUseAM se lam gi + CheckInventory
	dbg_rc_useam,			// theRecover.OnUseAM()
	dbg_rc_ngami,			// DoNgaMiBuff: ky nang, doi, mau
	dbg_rc_x2,				// LoadX2List + OnUseTTL + OnUseSDT
	dbg_rc_usex2,			// UseItem(Chuoi / X2 dau tien)
	dbg_rc_usesdt,			// UseItem("Su do thiep")

	dbg_lg_status = 600,	// KUiInit + cac nut + cua so dang nhap
	dbg_lg_entergame,		// KUiInit::EnterGame() - nut "Bat Dau Tro Choi"
	dbg_lg_click,			// KUiInit::OnClickButton(nut Idx)
	dbg_lg_region,			// Chon khu vuc thu Idx (Left/RightList + nap lai IpList)
	dbg_lg_server,			// Chon may chu thu Idx trong khu vuc (IpList)
	dbg_lg_selserver,		// Chon may chu: nut Dang nhap (may chu dang chon / Idx)
	dbg_lg_login,			// Cua so dang nhap: SetText TK + MK (Chuoi "TK MK"), tick dong y, OnLogin
	dbg_lg_role,			// Chon nhan vat: KUiSelPlayer::OnSelectPlayer(Idx - trang)
	dbg_lg_entergame2,		// KUiSelPlayer::OnEnterGame()
	dbg_lg_serverlist,		// Doc \settings\serverlist.ini trong pak cua Game, xuat UiConfig\serverlist_.txt
	dbg_lg_notice,			// Bang "Nhac nho" (KUiHealthNotice) sau Bat Dau Tro Choi: bam Xac dinh
	dbg_lg_last = dbg_lg_notice,

	// Tab Giam cau hinh (VLDebugLowCfg.cpp) - thu truoc khi dua vao tab Co ban
	dbg_lc_status = 700,	// Gia tri hien tai: FPS ve, tat hinh, nguoi choi, thoi tiet, am thanh, RAM
	dbg_lc_trim,			// Giam RAM ngay: SetProcessWorkingSetSize(-1, -1)
	dbg_lc_autotrim,		// Tu giam RAM moi Idx giay (0 = tat)
	dbg_lc_fps,				// FPS ve = Idx (0 = tra lai gia tri goc)
	dbg_lc_screen,			// Tat hinh: Idx = 1 bat, 0 tat
	dbg_lc_players,			// So nguoi choi hien thi = Idx (0 = an nguoi choi khac, mac dinh 200)
	dbg_lc_weather,			// Thoi tiet: Idx = 1 bat, 0 tat
	dbg_lc_dynalight,		// Bong dong: Idx = 1 bat, 0 tat
	dbg_lc_sound,			// Am luong nhac + tieng = Idx (0..100, 0 = tat)
	dbg_lc_lowall,			// Ap dung tat ca muc thap (FPS = Idx hoac 5)
	dbg_lc_restore,			// Tra lai tat ca nhu luc dau

	// Tab Nhiem vu (VLDebugQuest.cpp) - Da Tau ban CTC: server dieu khien, doc chu hop thoai / thong bao
	dbg_qt_status = 800,	// Buoc dang chay + nhat ky 5 buoc + ket qua phan tich nhiem vu
	dbg_qt_step1,			// Buoc 1: toi Da Tau, doc nhiem vu / bam Nhan nhiem vu (X = 1 chi kiem tra, bam lai = dung)
	dbg_qt_step2,			// Buoc 2: DoQuestKillNpc = ban do / mua mau, mana, TDP -> MapPath toi map -> danh quai nhiem vu
	dbg_qt_step3,			// Buoc 3: ve Da Tau, Hoan thanh nhiem vu, chon o phan thuong Idx
	dbg_qt_step4,			// (khong dung - giu so thu tu)
	dbg_qt_step5,			// (khong dung)
	dbg_qt_stop,			// Dung buoc dang chay
	dbg_qt_watch,			// Bat/tat theo doi CoreDataChanged id 23/64/161 (X = 1: moi id)
	dbg_qt_watchview,		// Xem thong bao da ghi (Chuoi = loc, X = 1 xoa), luu Logs\QuestLog.txt
	dbg_qt_reset,			// Xoa ket qua phan tich + nhat ky
	dbg_qt_taskval,			// g_pCurPlayer->GetTaskVal(Idx); X > Idx: liet ke task Idx..X khac 0
};

struct DEBUG_PARAM
{
	UINT	cbSize;
	HWND	hWndReply;		// Cua so nhan WM_COPYDATA
	int		nCmd;			// DEBUG_CMD
	int		nSeq;			// So thu tu lenh (tra ve trong DEBUG_REPLY)
	int		nParam[4];
	char	szParam[64];	// Chuoi TCVN3
};

struct DEBUG_REPLY
{
	int		nCmd;
	int		nSeq;
	BOOL	bOK;
	int		nResult[4];
	char	szText[DEBUG_TEXT_SIZE];
};

// Theo doi tung buoc lam nhiem vu: hien o kenh he thong va ghi vao Logs\<ten>.log.
// Moi cho goi QTRACE chi bao lai khi noi dung khac lan truoc (khong spam theo chu ky Breath).
#define QUEST_TRACE 0
#if QUEST_TRACE
void QTraceSite(char* szLast, const char* szFormat, ...);
#define QTRACE(...) do { static char _szQTLast[256] = ""; QTraceSite(_szQTLast, __VA_ARGS__); } while (0)
#else
#define QTRACE(...) ((void)0)
#endif
