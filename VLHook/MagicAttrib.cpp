#include "stdafx.h"
#include "MagicAttrib.h"

// 15-10: sinh tu dong tu exe (bang ten 0xEE0278 + 4*ID; 10-10: 0x1F18300) va pak settings/item/004/magicattriblevel.txt

static const char* const s_szMagicAttribName[] =
{
	"skill_begin", "skill_cost_v", "skill_costtype_v", "skill_mintimepercast_v", "skill_misslenum_v", "skill_misslesform_v",
	"skill_param1_v", "skill_param2_v", "skill_skillexp_v", "skill_waittime", "skill_mintimepercastonhorse_v", "skill_appendskill",
	"skill_eventskilllevel", "skill_end", "missle_begin", "missle_movekind_v", "missle_speed_v", "missle_lifetime_v",
	"missle_height_v", "missle_damagerange_v", "missle_radius_v", "missle_missrate", "missle_hitcount", "missle_reserve3",
	"missle_reserve4", "missle_reserve5", "missle_end", "item_begin", "weapondamagemin_v", "weapondamagemax_v",
	"armordefense_v", "durability_v", "requirestr", "requiredex", "requirevit", "requireeng",
	"requirelevel", "requireseries", "requiresex", "requiremenpai", "weapondamageenhance_p", "armordefenseenhance_p",
	"requirementreduce_p", "indestructible_b", "item_usagetime_v", "req_levelortranslife", "require_translife", "require_fortune_value",
	"item_reserve5", "item_reserve6", "item_reserve7", "item_reserve8", "item_reserve9", "item_reserve10",
	"item_end", "damage_begin", "attackrating_v", "attackrating_p", "ignoredefense_p", "physicsdamage_v",
	"colddamage_v", "firedamage_v", "lightingdamage_v", "poisondamage_v", "magicdamage_v", "physicsenhance_p",
	"steallife_p", "stealmana_p", "stealstamina_p", "knockback_p", "deadlystrike_p", "fatallystrike_p",
	"stun_p", "addskillexp1", "addskillexp2", "seriesdamage_p", "damage_reserve4", "damage_reserve5",
	"damage_reserve6", "damage_reserve7", "damage_reserve8", "damage_reserve9", "damage_reserve10", "damage_end",
	"normal_begin", "lifemax_v", "lifemax_p", "life_v", "lifereplenish_v", "manamax_v",
	"manamax_p", "mana_v", "manareplenish_v", "staminamax_v", "staminamax_p", "stamina_v",
	"staminareplenish_v", "strength_v", "dexterity_v", "vitality_v", "energy_v", "poisonres_p",
	"fireres_p", "lightingres_p", "physicsres_p", "coldres_p", "freezetimereduce_p", "burntimereduce_p",
	"poisontimereduce_p", "poisondamagereduce_v", "stuntimereduce_p", "fastwalkrun_p", "visionradius_p", "fasthitrecover_v",
	"allres_p", "attackspeed_v", "castspeed_v", "meleedamagereturn_v", "meleedamagereturn_p", "rangedamagereturn_v",
	"rangedamagereturn_p", "addphysicsdamage_v", "addfiredamage_v", "addcolddamage_v", "addlightingdamage_v", "addpoisondamage_v",
	"addphysicsdamage_p", "slowmissle_b", "changecamp_b", "physicsarmor_v", "coldarmor_v", "firearmor_v",
	"poisonarmor_v", "lightingarmor_v", "damage2addmana_p", "lucky_v", "steallifeenhance_p", "stealmanaenhance_p",
	"stealstaminaenhance_p", "allskill_v", "metalskill_v", "woodskill_v", "waterskill_v", "fireskill_v",
	"earthskill_v", "knockbackenhance_p", "deadlystrikeenhance_p", "stunenhance_p", "badstatustimereduce_v", "manashield_p",
	"adddefense_v", "adddefense_p", "fatallystrikeenhance_p", "lifepotion_v", "manapotion_v", "physicsresmax_p",
	"coldresmax_p", "fireresmax_p", "lightingresmax_p", "poisonresmax_p", "allresmax_p", "coldenhance_p",
	"fireenhance_p", "lightingenhance_p", "poisonenhance_p", "magicenhance_p", "attackratingenhance_v", "attackratingenhance_p",
	"addphysicsmagic_v", "addcoldmagic_v", "addfiremagic_v", "addlightingmagic_v", "addpoisonmagic_v", "fatallystrikeres_p",
	"statusimmunity_b", "expenhance_v", "expenhance_p", "seriesres_p", "seriesenhance_p", "createnpc",
	"autoattacknpc", "dynamicmagicshield_v", "nomovespeed", "execscript", "changefeature1", "changefeature2",
	"stealfeature", "addstealfeatureskill", "lucky_v_partner", "listen_msg", "lifereplenish_p", "ignoreskill_p",
	"returnskill_p", "poisondamagereturn_v", "poisondamagereturn_p", "autoreplyskill", "autoattackskill", "autorescueskill",
	"autodeathskill", "randmove", "hide", "ignorenegativestate_p", "poison2decmana_p", "staticmagicshield_v",
	"staticmagicshield_p", "returnres_p", "add120skillexpenhance_p", "dec_percasttimehorse", "dec_percasttime", "enhance_709_auto",
	"enhance_708_life_p", "enhance_93_life_v", "enhance_711_auto", "enhance_714_auto", "enhance_717_auto", "enhance_723_miss_p",
	"showpersoninfo", "ignoredamage", "sorbdamage_p", "anti_hitrecover", "anti_stuntimereduce_p", "anti_poisonres_p",
	"anti_fireres_p", "anti_lightingres_p", "anti_physicsres_p", "anti_coldres_p", "block_rate", "enhancehit_rate",
	"poisonres_yan_p", "lightingres_yan_p", "fireres_yan_p", "physicsres_yan_p", "coldres_yan_p", "lifemax_yan_v",
	"lifemax_yan_p", "manamax_yan_v", "manamax_yan_p", "sorbdamage_yan_p", "fastwalkrun_yan_p", "attackspeed_yan_v",
	"castspeed_yan_v", "allres_yan_p", "anti_maxres_p", "skill_enhance", "magicdamage_p", "fasthitrecover_yan_v",
	"five_elements_enhance_v", "five_elements_resist_v", "manareplenish_p", "add_damage_p", "forbit_attack", "frozen_action",
	"forbit_takemedicine", "invincibility", "not_add_pkvalue_p", "add_boss_damage", "pk_punish_weaken", "pk_punish_enhance",
	"anti_poisontimereduce_p", "do_hurt_p", "anti_do_hurt_p", "do_stun_p", "anti_do_stun_p", "anti_physicsres_yan_p",
	"anti_poisonres_yan_p", "anti_coldres_yan_p", "anti_fireres_yan_p", "anti_lightingres_yan_p", "anti_allres_yan_p", "anti_sorbdamage_yan_p",
	"anti_block_rate", "anti_enhancehit_rate", "autocastskill", "transfigure", "oncastskill", "enhancehiteffect_rate",
	"me2metaldamage_p", "metal2medamage_p", "me2wooddamage_p", "wood2medamage_p", "me2waterdamage_p", "water2medamage_p",
	"me2firedamage_p", "fire2medamage_p", "me2earthdamage_p", "earth2medamage_p", "meleedamagereturnmana_p", "rangedamagereturnmana_p",
	"reduceskillcd1", "reduceskillcd2", "reduceskillcd3", "clearallcd", "addblockrate", "walkrunshadow",
	"", "candetonate1", "candetonate2", "candetonate3", "manatoskill_enhance", "melee_returnres_p",
	"range_returnres_p", "forbit_usetownportal", "factionreduction_rate", "", "", "",
	"", "", "", "", "", "",
	"", "", "", "", "", "",
	"", "", "", "", "", "",
	"", "", "", "", "", "",
	"", "", "", "", "", "",
	"", "", "", "", "", "",
	"", "", "", "", "", "",
	"", "", "", "", "", "",
	"", "", "", "", "", "",
	"", "", "", "", "", "",
	"", "", "", "", "", "",
	"", "", "", "", "", "",
	"", "", "", "", "", "",
	"", "", "", "", "", "",
	"", "", "", "", "", "",
	"", "", "", "", "", "",
	"", "", "", "", "", "",
	"", "", "", "", "", "",
	"", "", "", "", "", "",
	"", "", "", "", "", "",
	"", "", "", "", "", "",
	"", "", "", "", "", "",
	"", "", "", "", "", "",
	"", "", "", "", "", "",
	"", "", "", "", "", "",
	"", "", "", "", "", "",
	"", "", "", "", "", "",
	"", "", "", "", "", "",
	"", "", "", "", "", "",
	"", "", "", "", "", "",
	"", "", "", "", "", "",
	"", "", "", "", "", "",
	"", "", "", "require_prestige_value", "anit_allres_p", "coldweaken_p",
	"longqiyunxiang", "anti_enhancehiteffect_rate", "decrease_damage", "normal_end", "skillexpbegin", "addskilldamage1",
	"addskilldamage2", "addskilldamage3", "addskilldamage4", "addskilldamage5", "addskilldamage6", "skill_attackradius",
	"skill_startevent", "skill_flyevent", "skill_collideevent", "skill_vanishedevent", "skill_dohurt", "skill_bymissle",
	"skill_showevent", "skill_desc", "", "", "", "",
	"skillexpend", "missle_exp_begin", "missle_range", "missle_dmginterval", "missle_zspeed", "missle_ablility",
	"missle_param", "missle_wait", "missle_fly", "missle_collide", "missle_vanish", "missle_exp_rev1",
	"missle_exp_rev2", "missle_exp_rev3", "missle_exp_rev4", "missle_exp_rev5", "missle_exp_end", "magic_end",
};

static const char* const s_szMagicAttribDesc[] =	// mo ta ngan (TCVN3) tu pak settings/magicdesc.ini
{
	"",	// 0 skill_begin
	"Ti\252u hao (\256i\323m)",	// 1 skill_cost_v
	"Lo\271i h\327nh ti\252u hao",	// 2 skill_costtype_v
	"Kho\266ng c\270ch th\352i gian",	// 3 skill_mintimepercast_v
	"\247\345ng th\352i xu\312t x",	// 4 skill_misslenum_v
	"",	// 5 skill_misslesform_v
	"",	// 6 skill_param1_v
	"",	// 7 skill_param2_v
	"",	// 8 skill_skillexp_v
	"",	// 9 skill_waittime
	"",	// 10 skill_mintimepercastonhorse_v
	"G\345m k\374 n\250ng x, c\312p cao nh\312t Lvx",	// 11 skill_appendskill
	"",	// 12 skill_eventskilllevel
	"",	// 13 skill_end
	"",	// 14 missle_begin
	"",	// 15 missle_movekind_v
	"T\350c \256\351 xu\312t chi\252u kh\335 c\253ng",	// 16 missle_speed_v
	"Hi\326u qu\266 xu\312t chi\252u kh\335 c\253ng",	// 17 missle_lifetime_v
	"",	// 18 missle_height_v
	"Ph\271m vi s\270t th\255\254ng kh\335 c\253ng",	// 19 missle_damagerange_v
	"",	// 20 missle_radius_v
	"",	// 21 missle_missrate
	"",	// 22 missle_hitcount
	"",	// 23 missle_reserve3
	"",	// 24 missle_reserve4
	"",	// 25 missle_reserve5
	"",	// 26 missle_end
	"",	// 27 item_begin
	"S\270t th\255\254ng nh\341 nh\312t (\256i\323m)",	// 28 weapondamagemin_v
	"S\270t th\255\254ng l\355n nh\312t (\256i\323m)",	// 29 weapondamagemax_v
	"N\320 tr\270nh (\256i\323m)",	// 30 armordefense_v
	"\247\351 b\322n (\256i\323m)",	// 31 durability_v
	"Y\252u c\307u s\370c m\271nh (\256i\323m)",	// 32 requirestr
	"Y\252u c\307u th\251n ph\270p (\256i\323m)",	// 33 requiredex
	"Y\252u c\307u sinh kh\335 (\256i\323m)",	// 34 requirevit
	"Y\252u c\307u n\351i c\253ng (\256i\323m)",	// 35 requireeng
	"Y\252u c\307u c\312p",	// 36 requirelevel
	"Thu\351c t\335nh ng\362 h\265nh",	// 37 requireseries
	"Gi\355i t\335nh",	// 38 requiresex
	"M\253n ph\270i",	// 39 requiremenpai
	"T\250ng s\270t th\255\254ng (%)",	// 40 weapondamageenhance_p
	"T\250ng n\320 tr\270nh (%)",	// 41 armordefenseenhance_p
	"Gi\266m thi\323u y\252u c\307u (%)",	// 42 requirementreduce_p
	"Kh\253ng th\323 ph\270 h\361y",	// 43 indestructible_b
	"",	// 44 item_usagetime_v
	"",	// 45 req_levelortranslife
	"Y\252u c\307u v\342 c\253ng",	// 46 require_translife
	"",	// 47 require_fortune_value
	"",	// 48 item_reserve5
	"",	// 49 item_reserve6
	"",	// 50 item_reserve7
	"",	// 51 item_reserve8
	"",	// 52 item_reserve9
	"",	// 53 item_reserve10
	"",	// 54 item_end
	"",	// 55 damage_begin
	"\247\351 ch\335nh x\270c (\256i\323m)",	// 56 attackrating_v
	"\247\351 ch\335nh x\270c (%)",	// 57 attackrating_p
	"B\341 qua n\320 tr\270nh (%)",	// 58 ignoredefense_p
	"S\270t th\255\254ng v\313t l\375 (\256i\323m)",	// 59 physicsdamage_v
	"B\250ng s\270t (\256i\323m)",	// 60 colddamage_v
	"H\341a s\270t (\256i\323m)",	// 61 firedamage_v
	"L\253i s\270t (\256i\323m)",	// 62 lightingdamage_v
	"\247\351c s\270t (\256i\323m)",	// 63 poisondamage_v
	"S\270t th\255\254ng ng\362 h\265nh x x \256i\323m",	// 64 magicdamage_v
	"S\270t th\255\254ng v\313t l\375 (%)",	// 65 physicsenhance_p
	"H\363t Sinh l\371c (%)",	// 66 steallife_p
	"H\363t N\351i l\371c (%)",	// 67 stealmana_p
	"H\363t Th\323 l\371c (%)",	// 68 stealstamina_p
	"\247\310y l\357i \256\350i ph\255\254ng (%)",	// 69 knockback_p
	"T\312n c\253ng ch\335 m\271ng (%)",	// 70 deadlystrike_p
	"T\312n c\253ng ch\335 t\366 (%)",	// 71 fatallystrike_p
	"L\265m cho\270ng (%)",	// 72 stun_p
	"",	// 73 addskillexp1
	"",	// 74 addskillexp2
	"",	// 75 seriesdamage_p
	"",	// 76 damage_reserve4
	"",	// 77 damage_reserve5
	"",	// 78 damage_reserve6
	"",	// 79 damage_reserve7
	"",	// 80 damage_reserve8
	"",	// 81 damage_reserve9
	"",	// 82 damage_reserve10
	"",	// 83 damage_end
	"",	// 84 normal_begin
	"Sinh l\371c t\350i \256a (\256i\323m)",	// 85 lifemax_v
	"Sinh l\371c t\350i \256a (%)",	// 86 lifemax_p
	"Sinh l\371c (\256i\323m)",	// 87 life_v
	"Ph\364c h\345i sinh l\371c m\347i n\366a gi\251y (\256i\323m)",	// 88 lifereplenish_v
	"N\351i l\371c t\350i \256a (\256i\323m)",	// 89 manamax_v
	"N\351i l\371c t\350i \256a (%)",	// 90 manamax_p
	"N\351i l\371c (\256i\323m)",	// 91 mana_v
	"Ph\364c h\345i n\351i l\371c m\347i n\366a gi\251y (\256i\323m)",	// 92 manareplenish_v
	"Th\323 l\371c t\350i \256a (\256i\323m)",	// 93 staminamax_v
	"Th\323 l\371c t\350i \256a (%)",	// 94 staminamax_p
	"Th\323 l\371c (%)",	// 95 stamina_v
	"Ph\364c h\345i th\323 l\371c m\347i n\366a gi\251y (\256i\323m)",	// 96 staminareplenish_v
	"S\370c m\271nh (\256i\323m)",	// 97 strength_v
	"Th\251n ph\270p (\256i\323m)",	// 98 dexterity_v
	"Sinh kh\335 (\256i\323m)",	// 99 vitality_v
	"N\351i c\253ng (\256i\323m)",	// 100 energy_v
	"Kh\270ng \256\351c (%)",	// 101 poisonres_p
	"Kh\270ng h\341a (%)",	// 102 fireres_p
	"Kh\270ng l\253i (%)",	// 103 lightingres_p
	"Ph\337ng th\361 v\313t l\375 (%)",	// 104 physicsres_p
	"Kh\270ng b\250ng (%)",	// 105 coldres_p
	"Th\352i gian l\265m ch\313m (%)",	// 106 freezetimereduce_p
	"",	// 107 burntimereduce_p
	"Th\352i gian tr\363ng \256\351c (%)",	// 108 poisontimereduce_p
	"\247\351c s\270t",	// 109 poisondamagereduce_v
	"Th\352i gian cho\270ng (%)",	// 110 stuntimereduce_p
	"T\350c \256\351 di chuy\323n (%)",	// 111 fastwalkrun_p
	"Ph\271m vi s\270t th\255\254ng (%)",	// 112 visionradius_p
	"Th\352i gian ph\364c h\345i",	// 113 fasthitrecover_v
	"Kh\270ng t\312t c\266 (%)",	// 114 allres_p
	"T\350c \256\351 \256\270nh - ngo\271i c\253ng (%)",	// 115 attackspeed_v
	"T\350c \256\351 \256\270nh - n\351i c\253ng (%)",	// 116 castspeed_v
	"Ph\266n \256\337n c\313n chi\325n (\256i\323m)",	// 117 meleedamagereturn_v
	"Ph\266n \256\337n c\313n chi\325n (%)",	// 118 meleedamagereturn_p
	"Ph\266n \256\337n t\307m xa (\256i\323m)",	// 119 rangedamagereturn_v
	"Ph\266n \256\337n t\307m xa (%)",	// 120 rangedamagereturn_p
	"S\270t th\255\254ng v\313t l\375 - ngo\271i c\253ng (\256i\323m)",	// 121 addphysicsdamage_v
	"H\341a s\270t - ngo\271i c\253ng (\256i\323m)",	// 122 addfiredamage_v
	"B\250ng s\270t - ngo\271i c\253ng (\256i\323m)",	// 123 addcolddamage_v
	"L\253i s\270t - ngo\271i c\253ng (\256i\323m)",	// 124 addlightingdamage_v
	"\247\351c s\270t - ngo\271i c\253ng (\256i\323m)",	// 125 addpoisondamage_v
	"S\270t th\255\254ng v\313t l\375 - ngo\271i c\253ng (%)",	// 126 addphysicsdamage_p
	"Gi\266m t\350c \256\351 xu\312t chi\252u kh\335 c\253ng",	// 127 slowmissle_b
	"M\252 ho\306c \256\350i ph\255\254ng",	// 128 changecamp_b
	"Ph\337ng th\361 v\313t l\375 (\256i\323m)",	// 129 physicsarmor_v
	"Kh\270ng b\250ng (\256i\323m)",	// 130 coldarmor_v
	"Kh\270ng h\341a (\256i\323m)",	// 131 firearmor_v
	"Kh\270ng \256\351c (\256i\323m)",	// 132 poisonarmor_v
	"Kh\270ng l\253i (\256i\323m)",	// 133 lightingarmor_v
	"Chuy\323n h\343a s\270t th\255\254ng th\265nh n\351i l\371c (%)",	// 134 damage2addmana_p
	"May m\276n (%)",	// 135 lucky_v
	"H\363t sinh l\371c (%)",	// 136 steallifeenhance_p
	"H\363t n\351i l\371c (%)",	// 137 stealmanaenhance_p
	"H\363t th\323 l\371c (%)",	// 138 stealstaminaenhance_p
	"T\312t c\266 k\374 n\250ng (c\312p)",	// 139 allskill_v
	"K\374 n\250ng h\326 Kim (c\312p)",	// 140 metalskill_v
	"K\374 n\250ng h\326 M\351c (c\312p)",	// 141 woodskill_v
	"K\374 n\250ng h\326 Th\361y (c\312p)",	// 142 waterskill_v
	"K\374 n\250ng h\326 H\341a (c\312p)",	// 143 fireskill_v
	"K\374 n\250ng h\326 Th\346 (c\312p)",	// 144 earthskill_v
	"\247\310y lui (%)",	// 145 knockbackenhance_p
	"T\250ng t\312n c\253ng ch\335 m\271ng (%)",	// 146 deadlystrikeenhance_p
	"L\265m cho\270ng (%)",	// 147 stunenhance_p
	"Th\352i gian k\320o d\265i tr\271ng th\270i kh\253ng t\350t",	// 148 badstatustimereduce_v
	"N\351i l\371c h\351 th\251n (%)",	// 149 manashield_p
	"N\320 tr\270nh (\256i\323m)",	// 150 adddefense_v
	"N\320 tr\270nh (%)",	// 151 adddefense_p
	"T\312n c\253ng ch\335 m\271ng (%)",	// 152 fatallystrikeenhance_p
	"Ph\364c h\345i sinh l\371c m\347i n\366a gi\251y (\256i\323m)",	// 153 lifepotion_v
	"Ph\364c h\345i n\351i l\371c m\347i n\366a gi\251y (\256i\323m)",	// 154 manapotion_v
	"Ph\337ng th\361 v\313t l\375 t\350i \256a (%)",	// 155 physicsresmax_p
	"Kh\270ng b\250ng t\350i \256a (%)",	// 156 coldresmax_p
	"Kh\270ng h\341a t\350i \256a (%)",	// 157 fireresmax_p
	"Kh\270ng l\253i t\350i \256a (%)",	// 158 lightingresmax_p
	"Kh\270ng \256\351c t\350i \256a (%)",	// 159 poisonresmax_p
	"Kh\270ng t\312t c\266 t\350i \256a (%)",	// 160 allresmax_p
	"Th\352i gian l\265m ch\313m (%)",	// 161 coldenhance_p
	"H\341a s\270t t\350i \256a (%)",	// 162 fireenhance_p
	"L\253i s\270t t\350i thi\323u (%)",	// 163 lightingenhance_p
	"Th\352i gian \256\351c ph\270t (%)",	// 164 poisonenhance_p
	"T\250ng x s\270t th\255\254ng ng\362 h\265nh",	// 165 magicenhance_p
	"T\330 l\326 c\253ng k\335ch ch\335nh x\270c (\256i\323m)",	// 166 attackratingenhance_v
	"T\330 l\326 c\253ng k\335ch ch\335nh x\270c (%)",	// 167 attackratingenhance_p
	"S\270t th\255\254ng v\313t l\375 - n\351i c\253ng (\256i\323m)",	// 168 addphysicsmagic_v
	"B\250ng s\270t - n\351i c\253ng (\256i\323m)",	// 169 addcoldmagic_v
	"H\341a s\270t - n\351i c\253ng (\256i\323m)",	// 170 addfiremagic_v
	"L\253i s\270t - n\351i c\253ng (\256i\323m)",	// 171 addlightingmagic_v
	"\247\351c s\270t - n\351i c\253ng (\256i\323m)",	// 172 addpoisonmagic_v
	"Kh\270ng t\312n c\253ng ch\335 t\366 (%)",	// 173 fatallystrikeres_p
	"",	// 174 statusimmunity_b
	"",	// 175 expenhance_v
	"Di\326t qu\270i nh\313n EXP (%)",	// 176 expenhance_p
	"",	// 177 seriesres_p
	"",	// 178 seriesenhance_p
	"",	// 179 createnpc
	"",	// 180 autoattacknpc
	"N\351i l\371c h\351 th\251n (\256i\323m)",	// 181 dynamicmagicshield_v
	"",	// 182 nomovespeed
	"",	// 183 execscript
	"",	// 184 changefeature1
	"",	// 185 changefeature2
	"",	// 186 stealfeature
	"S\350 l\307n s\366 d\364ng c\337n l\271i c\361a k\374 n\250ng ng\364y trang",	// 187 addstealfeatureskill
	"May m\276n t\346 \256\351i (\256i\323m)",	// 188 lucky_v_partner
	"",	// 189 listen_msg
	"T\250ng hi\326u su\312t h\345i sinh l\371c (%)",	// 190 lifereplenish_p
	"X\270c su\312t b\341 qua Ch\363 Thu\313t Gi\266m Kh\270ng T\335nh (%)",	// 191 ignoreskill_p
	"X\270c su\312t ph\266n \256\337n Ch\363 Thu\313t (%)",	// 192 returnskill_p
	"Ph\266n \256\337n khi ch\336u s\270t th\255\254ng \247\351c (\256i\323m)",	// 193 poisondamagereturn_v
	"Ph\266n \256\337n khi ch\336u s\270t th\255\254ng \247\351c (%)",	// 194 poisondamagereturn_p
	"Khi b\336 t\312n c\253ng c\343 x t\371 \256\351ng thi tri\323nxc\312pxTh\352i gian ch\352x gi\251y",	// 195 autoreplyskill
	"",	// 196 autoattackskill
	"",	// 197 autorescueskill
	"",	// 198 autodeathskill
	"",	// 199 randmove
	"\310n th\251n x gi\251y, thi tri\323n v\342 c\253ng s\317 hi\326n h\327nh tr\353 l\271i",	// 200 hide
	"",	// 201 ignorenegativestate_p
	"Khi\325n m\364c ti\252u khi ch\336u s\270t th\255\254ng \247\351c s\317 \256\345ng th\352i b\336 m\312t n\351i l\371c v\355i t\330 l\326 x, duy tr\327 x gi\251y",	// 202 poison2decmana_p
	"",	// 203 staticmagicshield_v
	"",	// 204 staticmagicshield_p
	"Kh\270ng ph\266n \256\337n (%)",	// 205 returnres_p
	"",	// 206 add120skillexpenhance_p
	"Gi\266m th\352i gian ch\352",	// 207 dec_percasttimehorse
	"Gi\266m th\352i gian ch\352",	// 208 dec_percasttime
	"X\270c su\312t k\335ch ho\271t [S\255 T\366 H\350ng] (%)",	// 209 enhance_709_auto
	"Hi\326u su\312t h\345i sinh l\371c t\365 [\247\266o H\255 Thi\252n] t\250ng x",	// 210 enhance_708_life_p
	"\247i\323m h\345i ph\364c sinh l\371c t\365 [T\365 H\265ng Ph\346 \247\351] (\256i\323m)",	// 211 enhance_93_life_v
	"X\270c su\312t k\335ch ho\271t [H\312p Tinh Y\323m] (%)",	// 212 enhance_711_auto
	"X\270c su\312t k\335ch ho\271t [H\347n Thi\252n Kh\335 C\253ng] (%)",	// 213 enhance_714_auto
	"X\270c su\312t k\335ch ho\271t [L\255\354ng Nghi Ch\251n Kh\335] (%)",	// 214 enhance_717_auto
	"X\270c su\312t [Ma \242m Ph\326 Ph\270ch] khi\325n k\316 \256\336ch Ho\266ng S\356 (%)",	// 215 enhance_723_miss_p
	"",	// 216 showpersoninfo
	"",	// 217 ignoredamage
	"Tri\326t ti\252u s\270t th\255\254ng",	// 218 sorbdamage_p
	"T\271o th\265nh s\270t th\255\254ng c\343 th\352i gian ho\271t \256\351ng",	// 219 anti_hitrecover
	"T\271o th\265nh th\352i gian cho\270ng (%)",	// 220 anti_stuntimereduce_p
	"B\341 qua \247\351c Ph\337ng m\364c ti\252u (%)",	// 221 anti_poisonres_p
	"B\341 qua H\341a Ph\337ng m\364c ti\252u (%)",	// 222 anti_fireres_p
	"B\341 qua L\253i Ph\337ng m\364c ti\252u (%)",	// 223 anti_lightingres_p
	"B\341 qua Ph\337ng Th\361 m\364c ti\252u (%)",	// 224 anti_physicsres_p
	"B\341 qua B\250ng Ph\337ng m\364c ti\252u (%)",	// 225 anti_coldres_p
	"X\270c su\312t h\343a gi\266i s\270t th\255\254ng (%)",	// 226 block_rate
	"X\270c su\312t tr\344ng k\335ch (%)",	// 227 enhancehit_rate
	"Kh\270ng \256\351c (%) (D\255\254ng)",	// 228 poisonres_yan_p
	"Kh\270ng l\253i (%) (D\255\254ng)",	// 229 lightingres_yan_p
	"Kh\270ng h\341a (%) (D\255\254ng)",	// 230 fireres_yan_p
	"Ph\337ng th\361 v\313t l\375 (%) (D\255\254ng)",	// 231 physicsres_yan_p
	"Kh\270ng b\250ng (%) (D\255\254ng)",	// 232 coldres_yan_p
	"Sinh l\371c t\350i \256a (\256i\323m) (D\255\254ng)",	// 233 lifemax_yan_v
	"Sinh l\371c t\350i \256a (%) (D\255\254ng)",	// 234 lifemax_yan_p
	"N\351i l\371c t\350i \256a (\256i\323m) (D\255\254ng)",	// 235 manamax_yan_v
	"N\351i l\371c t\350i \256a (%) (D\255\254ng)",	// 236 manamax_yan_p
	"Tri\326t ti\252u s\270t th\255\254ng (D\255\254ng)",	// 237 sorbdamage_yan_p
	"T\350c \256\351 ch\271y (%) (D\255\254ng)",	// 238 fastwalkrun_yan_p
	"T\350c \256\351 \256\270nh - ngo\271i c\253ng (%) (D\255\254ng)",	// 239 attackspeed_yan_v
	"T\350c \256\351 \256\270nh - n\351i c\253ng (%) (D\255\254ng)",	// 240 castspeed_yan_v
	"Kh\270ng t\312t c\266 (%) (D\255\254ng)",	// 241 allres_yan_p
	"Khi\325n Kh\270ng t\312t c\266 c\361a \256\350i ph\255\254ng \256\255\356c (%)",	// 242 anti_maxres_p
	"T\250ng c\253ng k\335ch k\374 n\250ng (%)",	// 243 skill_enhance
	"S\270t th\255\254ng v\313t l\375 - n\351i c\253ng (%)",	// 244 magicdamage_p
	"Th\352i gian ph\364c h\345i (D\255\254ng)",	// 245 fasthitrecover_yan_v
	"Ng\362 H\265nh C\255\352ng H\343a (\256i\323m)",	// 246 five_elements_enhance_v
	"Nh\255\356c h\343a Ng\362 H\265nh (\256i\323m)",	// 247 five_elements_resist_v
	"N\251ng cao hi\326u su\312t ph\364c h\345i n\351i l\371c (%)",	// 248 manareplenish_p
	"L\371c c\253ng k\335ch (%)",	// 249 add_damage_p
	"",	// 250 forbit_attack
	"",	// 251 frozen_action
	"",	// 252 forbit_takemedicine
	"",	// 253 invincibility
	"T\330 l\326 kh\253ng t\250ng tr\336 s\350 PK (%)",	// 254 not_add_pkvalue_p
	"S\270t th\255\254ng l\252n boss Ho\265ng Kim (%)",	// 255 add_boss_damage
	"",	// 256 pk_punish_weaken
	"",	// 257 pk_punish_enhance
	"Th\352i gian t\271o th\265nh tr\363ng \256\351c (%)",	// 258 anti_poisontimereduce_p
	"T\373 l\326 t\271o th\265nh s\270t th\255\254ng (%)",	// 259 do_hurt_p
	"T\373 l\326 s\270t th\255\254ng gi\266m kh\270ng (%)",	// 260 anti_do_hurt_p
	"T\373 l\326 t\271o th\265nh cho\270ng (%)",	// 261 do_stun_p
	"T\330 l\326 kh\270ng cho\270ng (%)",	// 262 anti_do_stun_p
	"B\341 qua ph\346 ph\337ng \256\350i ph\255\254ng (%) (D\255\254ng)",	// 263 anti_physicsres_yan_p
	"B\341 qua \256\351c ph\337ng \256\350i ph\255\254ng (%) (D\255\254ng)",	// 264 anti_poisonres_yan_p
	"B\341 qua b\250ng ph\337ng \256\350i ph\255\254ng (%) (D\255\254ng)",	// 265 anti_coldres_yan_p
	"B\341 qua h\341a ph\337ng \256\350i ph\255\254ng (%) (D\255\254ng)",	// 266 anti_fireres_yan_p
	"B\341 qua l\253i ph\337ng \256\350i ph\255\254ng (%) (D\255\254ng)",	// 267 anti_lightingres_yan_p
	"B\341 qua t\312t c\266 kh\270ng t\335nh \256\350i ph\255\254ng (%) (D\255\254ng)",	// 268 anti_allres_yan_p
	"T\250ng s\270t th\255\254ng",	// 269 anti_sorbdamage_yan_p
	"T\373 l\326 h\343a gi\266i c\270ch \256\266ng (%)",	// 270 anti_block_rate
	"T\373 l\326 gi\266m tr\344ng k\335ch (%)",	// 271 anti_enhancehit_rate
	"",	// 272 autocastskill
	"",	// 273 transfigure
	"",	// 274 oncastskill
	"",	// 275 enhancehiteffect_rate
	"T\250ng s\270t th\255\254ng \256\350i v\355i h\326 Kim (%)",	// 276 me2metaldamage_p
	"Gi\266m s\270t th\255\254ng \256\325n t\365 h\326 Kim (%)",	// 277 metal2medamage_p
	"T\250ng s\270t th\255\254ng \256\350i v\355i h\326 M\351c (%)",	// 278 me2wooddamage_p
	"Gi\266m s\270t th\255\254ng \256\325n t\365 h\326 M\351c (%)",	// 279 wood2medamage_p
	"T\250ng s\270t th\255\254ng \256\350i v\355i h\326 Th\361y (%)",	// 280 me2waterdamage_p
	"Gi\266m s\270t th\255\254ng \256\325n t\365 h\326 Th\361y (%)",	// 281 water2medamage_p
	"T\250ng s\270t th\255\254ng \256\350i v\355i h\326 H\341a (%)",	// 282 me2firedamage_p
	"Gi\266m s\270t th\255\254ng \256\325n t\365 h\326 H\341a (%)",	// 283 fire2medamage_p
	"T\250ng s\270t th\255\254ng \256\350i v\355i h\326 Th\346 (%)",	// 284 me2earthdamage_p
	"Gi\266m s\270t th\255\254ng \256\325n t\365 h\326 Th\346 (%)",	// 285 earth2medamage_p
	"Gi\266m n\351i l\371c phe \256\336ch khi ch\336u s\270t th\255\254ng t\312n c\253ng t\307m g\307n (%)",	// 286 meleedamagereturnmana_p
	"Gi\266m n\351i l\371c phe \256\336ch khi ch\336u s\270t th\255\254ng t\312n c\253ng t\307m xa (%)",	// 287 rangedamagereturnmana_p
	"Gi\266m x Th\352i gian \256\343ng b\250ngx gi\251y",	// 288 reduceskillcd1
	"Gi\266m x Th\352i gian \256\343ng b\250ngx gi\251y",	// 289 reduceskillcd2
	"Gi\266m x Th\352i gian \256\343ng b\250ngx gi\251y",	// 290 reduceskillcd3
	"x x\270c su\312t x\343a t\312t c\266 th\352i gian ch\352 k\374 n\250ng c\361a b\266n th\251n",	// 291 clearallcd
	"Xung quanh m\347i x c\270 nh\251n x t\373 l\326 h\343a gi\266i s\270t th\255\254ng",	// 292 addblockrate
	"L\250ng Ba Vi B\351",	// 293 walkrunshadow
	"",	// 294 -
	"",	// 295 candetonate1
	"",	// 296 candetonate2
	"",	// 297 candetonate3
	"Khi n\351i c\253ng \256\307y t\250ng k\374 n\250ng c\253ng k\335ch (%)",	// 298 manatoskill_enhance
	"",	// 299 melee_returnres_p
	"",	// 300 range_returnres_p
	"",	// 301 forbit_usetownportal
	"",	// 302 factionreduction_rate
	"",	// 303 -
	"",	// 304 -
	"",	// 305 -
	"",	// 306 -
	"",	// 307 -
	"",	// 308 -
	"",	// 309 -
	"",	// 310 -
	"",	// 311 -
	"",	// 312 -
	"",	// 313 -
	"",	// 314 -
	"",	// 315 -
	"",	// 316 -
	"",	// 317 -
	"",	// 318 -
	"",	// 319 -
	"",	// 320 -
	"",	// 321 -
	"",	// 322 -
	"",	// 323 -
	"",	// 324 -
	"",	// 325 -
	"",	// 326 -
	"",	// 327 -
	"",	// 328 -
	"",	// 329 -
	"",	// 330 -
	"",	// 331 -
	"",	// 332 -
	"",	// 333 -
	"",	// 334 -
	"",	// 335 -
	"",	// 336 -
	"",	// 337 -
	"",	// 338 -
	"",	// 339 -
	"",	// 340 -
	"",	// 341 -
	"",	// 342 -
	"",	// 343 -
	"",	// 344 -
	"",	// 345 -
	"",	// 346 -
	"",	// 347 -
	"",	// 348 -
	"",	// 349 -
	"",	// 350 -
	"",	// 351 -
	"",	// 352 -
	"",	// 353 -
	"",	// 354 -
	"",	// 355 -
	"",	// 356 -
	"",	// 357 -
	"",	// 358 -
	"",	// 359 -
	"",	// 360 -
	"",	// 361 -
	"",	// 362 -
	"",	// 363 -
	"",	// 364 -
	"",	// 365 -
	"",	// 366 -
	"",	// 367 -
	"",	// 368 -
	"",	// 369 -
	"",	// 370 -
	"",	// 371 -
	"",	// 372 -
	"",	// 373 -
	"",	// 374 -
	"",	// 375 -
	"",	// 376 -
	"",	// 377 -
	"",	// 378 -
	"",	// 379 -
	"",	// 380 -
	"",	// 381 -
	"",	// 382 -
	"",	// 383 -
	"",	// 384 -
	"",	// 385 -
	"",	// 386 -
	"",	// 387 -
	"",	// 388 -
	"",	// 389 -
	"",	// 390 -
	"",	// 391 -
	"",	// 392 -
	"",	// 393 -
	"",	// 394 -
	"",	// 395 -
	"",	// 396 -
	"",	// 397 -
	"",	// 398 -
	"",	// 399 -
	"",	// 400 -
	"",	// 401 -
	"",	// 402 -
	"",	// 403 -
	"",	// 404 -
	"",	// 405 -
	"",	// 406 -
	"",	// 407 -
	"",	// 408 -
	"",	// 409 -
	"",	// 410 -
	"",	// 411 -
	"",	// 412 -
	"",	// 413 -
	"",	// 414 -
	"",	// 415 -
	"",	// 416 -
	"",	// 417 -
	"",	// 418 -
	"",	// 419 -
	"",	// 420 -
	"",	// 421 -
	"",	// 422 -
	"",	// 423 -
	"",	// 424 -
	"",	// 425 -
	"",	// 426 -
	"",	// 427 -
	"",	// 428 -
	"",	// 429 -
	"",	// 430 -
	"",	// 431 -
	"",	// 432 -
	"",	// 433 -
	"",	// 434 -
	"",	// 435 -
	"",	// 436 -
	"",	// 437 -
	"",	// 438 -
	"",	// 439 -
	"",	// 440 -
	"",	// 441 -
	"",	// 442 -
	"",	// 443 -
	"",	// 444 -
	"",	// 445 -
	"",	// 446 -
	"",	// 447 -
	"",	// 448 -
	"",	// 449 -
	"",	// 450 -
	"",	// 451 -
	"",	// 452 -
	"",	// 453 -
	"",	// 454 -
	"",	// 455 -
	"",	// 456 -
	"",	// 457 -
	"",	// 458 -
	"",	// 459 -
	"",	// 460 -
	"",	// 461 -
	"",	// 462 -
	"",	// 463 -
	"",	// 464 -
	"",	// 465 -
	"",	// 466 -
	"",	// 467 -
	"",	// 468 -
	"",	// 469 -
	"",	// 470 -
	"",	// 471 -
	"",	// 472 -
	"",	// 473 -
	"",	// 474 -
	"",	// 475 -
	"",	// 476 -
	"",	// 477 -
	"",	// 478 -
	"",	// 479 -
	"",	// 480 -
	"",	// 481 -
	"",	// 482 -
	"",	// 483 -
	"",	// 484 -
	"",	// 485 -
	"",	// 486 -
	"",	// 487 -
	"",	// 488 -
	"",	// 489 -
	"",	// 490 -
	"",	// 491 -
	"",	// 492 -
	"",	// 493 -
	"",	// 494 -
	"",	// 495 -
	"",	// 496 -
	"",	// 497 -
	"",	// 498 -
	"",	// 499 -
	"",	// 500 -
	"",	// 501 require_prestige_value
	"",	// 502 anit_allres_p
	"",	// 503 coldweaken_p
	"",	// 504 longqiyunxiang
	"",	// 505 anti_enhancehiteffect_rate
	"",	// 506 decrease_damage
	"",	// 507 normal_end
	"",	// 508 skillexpbegin
	"",	// 509 addskilldamage1
	"",	// 510 addskilldamage2
	"",	// 511 addskilldamage3
	"",	// 512 addskilldamage4
	"",	// 513 addskilldamage5
	"",	// 514 addskilldamage6
	"",	// 515 skill_attackradius
	"",	// 516 skill_startevent
	"",	// 517 skill_flyevent
	"",	// 518 skill_collideevent
	"",	// 519 skill_vanishedevent
	"",	// 520 skill_dohurt
	"",	// 521 skill_bymissle
	"",	// 522 skill_showevent
	"",	// 523 skill_desc
	"",	// 524 -
	"",	// 525 -
	"",	// 526 -
	"",	// 527 -
	"",	// 528 skillexpend
	"",	// 529 missle_exp_begin
	"",	// 530 missle_range
	"",	// 531 missle_dmginterval
	"",	// 532 missle_zspeed
	"",	// 533 missle_ablility
	"",	// 534 missle_param
	"",	// 535 missle_wait
	"",	// 536 missle_fly
	"",	// 537 missle_collide
	"",	// 538 missle_vanish
	"",	// 539 missle_exp_rev1
	"",	// 540 missle_exp_rev2
	"",	// 541 missle_exp_rev3
	"",	// 542 missle_exp_rev4
	"",	// 543 missle_exp_rev5
	"",	// 544 missle_exp_end
	"",	// 545 magic_end
};

// {MAGIC_ID, P1 min/max cua cap 1..10}
static const struct { short nType; short nRange[10][2]; } s_MagicLevel[] =
{
	{ 43, { {1,1}, {1,1}, {1,1}, {1,1}, {1,1}, {1,1}, {1,1}, {1,1}, {1,1}, {1,1} } },	// indestructible_b
	{ 58, { {1,1}, {1,1}, {1,1}, {1,1}, {1,1}, {1,1}, {1,1}, {1,1}, {1,1}, {1,1} } },	// ignoredefense_p
	{ 85, { {5,20}, {20,40}, {40,60}, {60,80}, {80,100}, {100,120}, {120,140}, {140,160}, {160,180}, {180,240} } },	// lifemax_v
	{ 88, { {1,1}, {2,2}, {3,3}, {4,4}, {5,5}, {6,6}, {7,7}, {8,8}, {9,9}, {10,10} } },	// lifereplenish_v
	{ 89, { {5,20}, {20,40}, {40,60}, {60,80}, {80,100}, {100,120}, {120,140}, {140,160}, {160,180}, {180,240} } },	// manamax_v
	{ 92, { {1,1}, {2,2}, {3,3}, {4,4}, {5,5}, {6,6}, {7,7}, {8,8}, {9,9}, {10,10} } },	// manareplenish_v
	{ 93, { {5,20}, {20,40}, {40,60}, {60,80}, {80,100}, {100,120}, {120,140}, {140,160}, {160,180}, {180,240} } },	// staminamax_v
	{ 96, { {1,1}, {2,2}, {3,3}, {4,4}, {5,5}, {6,6}, {7,7}, {8,8}, {9,9}, {10,11} } },	// staminareplenish_v
	{ 97, { {1,2}, {3,4}, {5,6}, {7,8}, {9,10}, {11,12}, {13,14}, {15,16}, {17,18}, {19,24} } },	// strength_v
	{ 98, { {1,2}, {3,4}, {5,6}, {7,8}, {9,10}, {11,12}, {13,14}, {15,16}, {17,18}, {19,23} } },	// dexterity_v
	{ 99, { {1,2}, {3,4}, {5,6}, {7,8}, {9,10}, {11,12}, {13,14}, {15,16}, {17,18}, {19,23} } },	// vitality_v
	{ 101, { {1,3}, {4,6}, {7,9}, {10,12}, {13,15}, {16,18}, {19,21}, {22,23}, {24,24}, {25,25} } },	// poisonres_p
	{ 102, { {1,3}, {4,6}, {7,9}, {10,12}, {13,15}, {16,18}, {19,21}, {22,23}, {24,24}, {25,25} } },	// fireres_p
	{ 103, { {1,5}, {6,8}, {9,11}, {12,14}, {15,17}, {18,20}, {21,23}, {24,26}, {27,29}, {30,30} } },	// lightingres_p
	{ 104, { {1,3}, {4,6}, {7,9}, {10,12}, {13,15}, {16,18}, {19,21}, {22,23}, {24,24}, {25,25} } },	// physicsres_p
	{ 105, { {1,3}, {4,6}, {7,9}, {10,12}, {13,15}, {16,18}, {19,21}, {22,23}, {24,24}, {25,25} } },	// coldres_p
	{ 106, { {7,10}, {11,14}, {15,17}, {18,20}, {21,23}, {24,26}, {27,30}, {31,35}, {36,38}, {39,44} } },	// freezetimereduce_p
	{ 108, { {7,10}, {11,14}, {15,17}, {18,20}, {21,23}, {24,26}, {27,30}, {31,35}, {36,38}, {39,40} } },	// poisontimereduce_p
	{ 110, { {7,10}, {11,14}, {15,17}, {18,20}, {21,23}, {24,26}, {27,30}, {31,35}, {36,38}, {39,44} } },	// stuntimereduce_p
	{ 111, { {10,15}, {16,18}, {19,21}, {22,24}, {25,27}, {28,30}, {31,33}, {34,36}, {37,39}, {40,40} } },	// fastwalkrun_p
	{ 112, { {1,1}, {2,2}, {3,3}, {4,4}, {5,5}, {6,6}, {6,6}, {6,6}, {6,6}, {6,6} } },	// visionradius_p
	{ 113, { {5,6}, {7,8}, {9,10}, {11,13}, {14,16}, {17,20}, {21,25}, {26,30}, {30,30}, {30,30} } },	// fasthitrecover_v
	{ 114, { {1,3}, {4,6}, {7,9}, {10,12}, {13,14}, {15,16}, {17,18}, {19,20}, {21,22}, {22,22} } },	// allres_p
	{ 115, { {10,10}, {10,10}, {10,10}, {20,20}, {20,20}, {20,20}, {30,30}, {30,30}, {30,30}, {30,30} } },	// attackspeed_v
	{ 117, { {1,2}, {3,4}, {5,6}, {7,8}, {9,10}, {11,12}, {13,14}, {15,16}, {17,18}, {19,20} } },	// meleedamagereturn_v
	{ 121, { {1,5}, {6,10}, {11,15}, {16,20}, {21,25}, {26,30}, {31,35}, {36,40}, {41,45}, {46,60} } },	// addphysicsdamage_v
	{ 122, { {1,10}, {10,20}, {20,30}, {30,40}, {40,50}, {50,60}, {60,70}, {70,80}, {80,90}, {90,120} } },	// addfiredamage_v
	{ 123, { {1,19}, {20,29}, {30,39}, {40,49}, {50,59}, {60,69}, {70,79}, {80,89}, {90,99}, {100,120} } },	// addcolddamage_v
	{ 124, { {1,10}, {10,20}, {20,30}, {30,40}, {40,50}, {50,60}, {60,70}, {70,80}, {80,90}, {90,120} } },	// addlightingdamage_v
	{ 125, { {1,5}, {6,10}, {11,15}, {16,20}, {21,25}, {26,30}, {31,35}, {36,40}, {41,45}, {46,80} } },	// addpoisondamage_v
	{ 126, { {5,10}, {11,20}, {21,30}, {31,40}, {41,50}, {51,60}, {61,70}, {71,80}, {81,90}, {91,120} } },	// addphysicsdamage_p
	{ 134, { {1,1}, {2,2}, {3,3}, {4,4}, {5,5}, {6,6}, {7,7}, {8,8}, {9,9}, {10,10} } },	// damage2addmana_p
	{ 135, { {1,1}, {2,2}, {3,3}, {4,4}, {5,5}, {6,6}, {7,7}, {8,8}, {9,9}, {10,12} } },	// lucky_v
	{ 136, { {1,1}, {2,2}, {3,3}, {4,4}, {5,5}, {6,6}, {7,7}, {8,8}, {9,9}, {10,10} } },	// steallifeenhance_p
	{ 137, { {1,1}, {2,2}, {3,3}, {4,4}, {5,5}, {6,6}, {7,7}, {8,8}, {9,9}, {10,10} } },	// stealmanaenhance_p
	{ 139, { {1,1}, {1,1}, {1,1}, {1,1}, {1,1}, {2,2}, {2,2}, {2,2}, {2,2}, {2,2} } },	// allskill_v
	{ 140, { {1,1}, {1,1}, {2,2}, {2,2}, {3,3}, {3,3}, {4,4}, {4,4}, {5,5}, {5,5} } },	// metalskill_v
	{ 141, { {1,1}, {1,1}, {2,2}, {2,2}, {3,3}, {3,3}, {4,4}, {4,4}, {5,5}, {5,5} } },	// woodskill_v
	{ 142, { {1,1}, {1,1}, {2,2}, {2,2}, {3,3}, {3,3}, {4,4}, {4,4}, {5,5}, {5,5} } },	// waterskill_v
	{ 143, { {1,1}, {1,1}, {2,2}, {2,2}, {3,3}, {3,3}, {4,4}, {4,4}, {5,5}, {5,5} } },	// fireskill_v
	{ 144, { {1,1}, {1,1}, {2,2}, {2,2}, {3,3}, {3,3}, {4,4}, {4,4}, {5,5}, {5,5} } },	// earthskill_v
	{ 150, { {20,30}, {30,40}, {40,50}, {50,60}, {60,70}, {70,85}, {85,110}, {110,150}, {150,200}, {200,250} } },	// adddefense_v
	{ 166, { {10,30}, {31,60}, {61,90}, {91,120}, {121,150}, {151,180}, {181,210}, {211,240}, {241,270}, {271,360} } },	// attackratingenhance_v
	{ 168, { {1,20}, {21,40}, {41,60}, {61,80}, {81,100}, {101,120}, {121,140}, {141,160}, {161,180}, {181,250} } },	// addphysicsmagic_v
	{ 169, { {1,20}, {21,40}, {41,60}, {61,80}, {81,100}, {101,120}, {121,140}, {141,160}, {161,180}, {181,250} } },	// addcoldmagic_v
	{ 170, { {1,20}, {21,40}, {41,60}, {61,80}, {81,100}, {101,120}, {121,140}, {141,160}, {161,180}, {181,250} } },	// addfiremagic_v
	{ 171, { {1,20}, {21,40}, {41,60}, {61,80}, {81,100}, {101,120}, {121,140}, {141,160}, {161,180}, {181,250} } },	// addlightingmagic_v
	{ 172, { {1,5}, {6,10}, {11,15}, {16,20}, {21,25}, {26,30}, {31,35}, {36,40}, {41,45}, {46,80} } },	// addpoisonmagic_v
};

const char* MagicAttribName(int nType)
{
	if (nType < 0 || nType >= (int)(sizeof(s_szMagicAttribName) / sizeof(s_szMagicAttribName[0])) || !s_szMagicAttribName[nType][0])
		return "?";
	return s_szMagicAttribName[nType];
}

const char* MagicAttribDesc(int nType)
{
	if (nType < 0 || nType >= (int)(sizeof(s_szMagicAttribDesc) / sizeof(s_szMagicAttribDesc[0])))
		return "";
	return s_szMagicAttribDesc[nType];
}

int MagicAttribTier(int nType, int nValue)
{
	for (int i = 0; i < sizeof(s_MagicLevel) / sizeof(s_MagicLevel[0]); i++)
	{
		if (s_MagicLevel[i].nType != nType) continue;
		const short (*r)[2] = s_MagicLevel[i].nRange;
		if (nValue < r[0][0]) return 0;
		for (int k = 9; k >= 0; k--)
			if (nValue >= r[k][0]) return k + 1;	// tinh ca gia tri nam giua 2 cap (nhu 13 giua 1-10 va 11-20 -> cap 2)
		return 0;
	}
	return 0;
}
