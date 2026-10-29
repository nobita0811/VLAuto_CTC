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
	TV(L"Tiêu hao (điểm)"),	// 1 skill_cost_v
	TV(L"Loại hình tiêu hao"),	// 2 skill_costtype_v
	TV(L"Khoảng cách thời gian"),	// 3 skill_mintimepercast_v
	TV(L"Đồng thời xuất x"),	// 4 skill_misslenum_v
	"",	// 5 skill_misslesform_v
	"",	// 6 skill_param1_v
	"",	// 7 skill_param2_v
	"",	// 8 skill_skillexp_v
	"",	// 9 skill_waittime
	"",	// 10 skill_mintimepercastonhorse_v
	TV(L"Gồm kỹ năng x, cấp cao nhất Lvx"),	// 11 skill_appendskill
	"",	// 12 skill_eventskilllevel
	"",	// 13 skill_end
	"",	// 14 missle_begin
	"",	// 15 missle_movekind_v
	TV(L"Tốc độ xuất chiêu khí công"),	// 16 missle_speed_v
	TV(L"Hiệu quả xuất chiêu khí công"),	// 17 missle_lifetime_v
	"",	// 18 missle_height_v
	TV(L"Phạm vi sát thương khí công"),	// 19 missle_damagerange_v
	"",	// 20 missle_radius_v
	"",	// 21 missle_missrate
	"",	// 22 missle_hitcount
	"",	// 23 missle_reserve3
	"",	// 24 missle_reserve4
	"",	// 25 missle_reserve5
	"",	// 26 missle_end
	"",	// 27 item_begin
	TV(L"Sát thương nhỏ nhất (điểm)"),	// 28 weapondamagemin_v
	TV(L"Sát thương lớn nhất (điểm)"),	// 29 weapondamagemax_v
	TV(L"Né tránh (điểm)"),	// 30 armordefense_v
	TV(L"Độ bền (điểm)"),	// 31 durability_v
	TV(L"Yêu cầu sức mạnh (điểm)"),	// 32 requirestr
	TV(L"Yêu cầu thân pháp (điểm)"),	// 33 requiredex
	TV(L"Yêu cầu sinh khí (điểm)"),	// 34 requirevit
	TV(L"Yêu cầu nội công (điểm)"),	// 35 requireeng
	TV(L"Yêu cầu cấp"),	// 36 requirelevel
	TV(L"Thuộc tính ngũ hành"),	// 37 requireseries
	TV(L"Giới tính"),	// 38 requiresex
	TV(L"Môn phái"),	// 39 requiremenpai
	TV(L"Tăng sát thương (%)"),	// 40 weapondamageenhance_p
	TV(L"Tăng né tránh (%)"),	// 41 armordefenseenhance_p
	TV(L"Giảm thiểu yêu cầu (%)"),	// 42 requirementreduce_p
	TV(L"Không thể phá hủy"),	// 43 indestructible_b
	"",	// 44 item_usagetime_v
	"",	// 45 req_levelortranslife
	TV(L"Yêu cầu võ công"),	// 46 require_translife
	"",	// 47 require_fortune_value
	"",	// 48 item_reserve5
	"",	// 49 item_reserve6
	"",	// 50 item_reserve7
	"",	// 51 item_reserve8
	"",	// 52 item_reserve9
	"",	// 53 item_reserve10
	"",	// 54 item_end
	"",	// 55 damage_begin
	TV(L"Độ chính xác (điểm)"),	// 56 attackrating_v
	TV(L"Độ chính xác (%)"),	// 57 attackrating_p
	TV(L"Bỏ qua né tránh (%)"),	// 58 ignoredefense_p
	TV(L"Sát thương vật lý (điểm)"),	// 59 physicsdamage_v
	TV(L"Băng sát (điểm)"),	// 60 colddamage_v
	TV(L"Hỏa sát (điểm)"),	// 61 firedamage_v
	TV(L"Lôi sát (điểm)"),	// 62 lightingdamage_v
	TV(L"Độc sát (điểm)"),	// 63 poisondamage_v
	TV(L"Sát thương ngũ hành x x điểm"),	// 64 magicdamage_v
	TV(L"Sát thương vật lý (%)"),	// 65 physicsenhance_p
	TV(L"Hút Sinh lực (%)"),	// 66 steallife_p
	TV(L"Hút Nội lực (%)"),	// 67 stealmana_p
	TV(L"Hút Thể lực (%)"),	// 68 stealstamina_p
	TV(L"Đẩy lùi đối phương (%)"),	// 69 knockback_p
	TV(L"Tấn công chí mạng (%)"),	// 70 deadlystrike_p
	TV(L"Tấn công chí tử (%)"),	// 71 fatallystrike_p
	TV(L"Làm choáng (%)"),	// 72 stun_p
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
	TV(L"Sinh lực tối đa (điểm)"),	// 85 lifemax_v
	TV(L"Sinh lực tối đa (%)"),	// 86 lifemax_p
	TV(L"Sinh lực (điểm)"),	// 87 life_v
	TV(L"Phục hồi sinh lực mỗi nửa giây (điểm)"),	// 88 lifereplenish_v
	TV(L"Nội lực tối đa (điểm)"),	// 89 manamax_v
	TV(L"Nội lực tối đa (%)"),	// 90 manamax_p
	TV(L"Nội lực (điểm)"),	// 91 mana_v
	TV(L"Phục hồi nội lực mỗi nửa giây (điểm)"),	// 92 manareplenish_v
	TV(L"Thể lực tối đa (điểm)"),	// 93 staminamax_v
	TV(L"Thể lực tối đa (%)"),	// 94 staminamax_p
	TV(L"Thể lực (%)"),	// 95 stamina_v
	TV(L"Phục hồi thể lực mỗi nửa giây (điểm)"),	// 96 staminareplenish_v
	TV(L"Sức mạnh (điểm)"),	// 97 strength_v
	TV(L"Thân pháp (điểm)"),	// 98 dexterity_v
	TV(L"Sinh khí (điểm)"),	// 99 vitality_v
	TV(L"Nội công (điểm)"),	// 100 energy_v
	TV(L"Kháng độc (%)"),	// 101 poisonres_p
	TV(L"Kháng hỏa (%)"),	// 102 fireres_p
	TV(L"Kháng lôi (%)"),	// 103 lightingres_p
	TV(L"Phòng thủ vật lý (%)"),	// 104 physicsres_p
	TV(L"Kháng băng (%)"),	// 105 coldres_p
	TV(L"Thời gian làm chậm (%)"),	// 106 freezetimereduce_p
	"",	// 107 burntimereduce_p
	TV(L"Thời gian trúng độc (%)"),	// 108 poisontimereduce_p
	TV(L"Độc sát"),	// 109 poisondamagereduce_v
	TV(L"Thời gian choáng (%)"),	// 110 stuntimereduce_p
	TV(L"Tốc độ di chuyển (%)"),	// 111 fastwalkrun_p
	TV(L"Phạm vi sát thương (%)"),	// 112 visionradius_p
	TV(L"Thời gian phục hồi"),	// 113 fasthitrecover_v
	TV(L"Kháng tất cả (%)"),	// 114 allres_p
	TV(L"Tốc độ đánh - ngoại công (%)"),	// 115 attackspeed_v
	TV(L"Tốc độ đánh - nội công (%)"),	// 116 castspeed_v
	TV(L"Phản đòn cận chiến (điểm)"),	// 117 meleedamagereturn_v
	TV(L"Phản đòn cận chiến (%)"),	// 118 meleedamagereturn_p
	TV(L"Phản đòn tầm xa (điểm)"),	// 119 rangedamagereturn_v
	TV(L"Phản đòn tầm xa (%)"),	// 120 rangedamagereturn_p
	TV(L"Sát thương vật lý - ngoại công (điểm)"),	// 121 addphysicsdamage_v
	TV(L"Hỏa sát - ngoại công (điểm)"),	// 122 addfiredamage_v
	TV(L"Băng sát - ngoại công (điểm)"),	// 123 addcolddamage_v
	TV(L"Lôi sát - ngoại công (điểm)"),	// 124 addlightingdamage_v
	TV(L"Độc sát - ngoại công (điểm)"),	// 125 addpoisondamage_v
	TV(L"Sát thương vật lý - ngoại công (%)"),	// 126 addphysicsdamage_p
	TV(L"Giảm tốc độ xuất chiêu khí công"),	// 127 slowmissle_b
	TV(L"Mê hoặc đối phương"),	// 128 changecamp_b
	TV(L"Phòng thủ vật lý (điểm)"),	// 129 physicsarmor_v
	TV(L"Kháng băng (điểm)"),	// 130 coldarmor_v
	TV(L"Kháng hỏa (điểm)"),	// 131 firearmor_v
	TV(L"Kháng độc (điểm)"),	// 132 poisonarmor_v
	TV(L"Kháng lôi (điểm)"),	// 133 lightingarmor_v
	TV(L"Chuyển hóa sát thương thành nội lực (%)"),	// 134 damage2addmana_p
	TV(L"May mắn (%)"),	// 135 lucky_v
	TV(L"Hút sinh lực (%)"),	// 136 steallifeenhance_p
	TV(L"Hút nội lực (%)"),	// 137 stealmanaenhance_p
	TV(L"Hút thể lực (%)"),	// 138 stealstaminaenhance_p
	TV(L"Tất cả kỹ năng (cấp)"),	// 139 allskill_v
	TV(L"Kỹ năng hệ Kim (cấp)"),	// 140 metalskill_v
	TV(L"Kỹ năng hệ Mộc (cấp)"),	// 141 woodskill_v
	TV(L"Kỹ năng hệ Thủy (cấp)"),	// 142 waterskill_v
	TV(L"Kỹ năng hệ Hỏa (cấp)"),	// 143 fireskill_v
	TV(L"Kỹ năng hệ Thổ (cấp)"),	// 144 earthskill_v
	TV(L"Đẩy lui (%)"),	// 145 knockbackenhance_p
	TV(L"Tăng tấn công chí mạng (%)"),	// 146 deadlystrikeenhance_p
	TV(L"Làm choáng (%)"),	// 147 stunenhance_p
	TV(L"Thời gian kéo dài trạng thái không tốt"),	// 148 badstatustimereduce_v
	TV(L"Nội lực hộ thân (%)"),	// 149 manashield_p
	TV(L"Né tránh (điểm)"),	// 150 adddefense_v
	TV(L"Né tránh (%)"),	// 151 adddefense_p
	TV(L"Tấn công chí mạng (%)"),	// 152 fatallystrikeenhance_p
	TV(L"Phục hồi sinh lực mỗi nửa giây (điểm)"),	// 153 lifepotion_v
	TV(L"Phục hồi nội lực mỗi nửa giây (điểm)"),	// 154 manapotion_v
	TV(L"Phòng thủ vật lý tối đa (%)"),	// 155 physicsresmax_p
	TV(L"Kháng băng tối đa (%)"),	// 156 coldresmax_p
	TV(L"Kháng hỏa tối đa (%)"),	// 157 fireresmax_p
	TV(L"Kháng lôi tối đa (%)"),	// 158 lightingresmax_p
	TV(L"Kháng độc tối đa (%)"),	// 159 poisonresmax_p
	TV(L"Kháng tất cả tối đa (%)"),	// 160 allresmax_p
	TV(L"Thời gian làm chậm (%)"),	// 161 coldenhance_p
	TV(L"Hỏa sát tối đa (%)"),	// 162 fireenhance_p
	TV(L"Lôi sát tối thiểu (%)"),	// 163 lightingenhance_p
	TV(L"Thời gian độc phát (%)"),	// 164 poisonenhance_p
	TV(L"Tăng x sát thương ngũ hành"),	// 165 magicenhance_p
	TV(L"Tỉ lệ công kích chính xác (điểm)"),	// 166 attackratingenhance_v
	TV(L"Tỉ lệ công kích chính xác (%)"),	// 167 attackratingenhance_p
	TV(L"Sát thương vật lý - nội công (điểm)"),	// 168 addphysicsmagic_v
	TV(L"Băng sát - nội công (điểm)"),	// 169 addcoldmagic_v
	TV(L"Hỏa sát - nội công (điểm)"),	// 170 addfiremagic_v
	TV(L"Lôi sát - nội công (điểm)"),	// 171 addlightingmagic_v
	TV(L"Độc sát - nội công (điểm)"),	// 172 addpoisonmagic_v
	TV(L"Kháng tấn công chí tử (%)"),	// 173 fatallystrikeres_p
	"",	// 174 statusimmunity_b
	"",	// 175 expenhance_v
	TV(L"Diệt quái nhận EXP (%)"),	// 176 expenhance_p
	"",	// 177 seriesres_p
	"",	// 178 seriesenhance_p
	"",	// 179 createnpc
	"",	// 180 autoattacknpc
	TV(L"Nội lực hộ thân (điểm)"),	// 181 dynamicmagicshield_v
	"",	// 182 nomovespeed
	"",	// 183 execscript
	"",	// 184 changefeature1
	"",	// 185 changefeature2
	"",	// 186 stealfeature
	TV(L"Số lần sử dụng còn lại của kỹ năng ngụy trang"),	// 187 addstealfeatureskill
	TV(L"May mắn tổ đội (điểm)"),	// 188 lucky_v_partner
	"",	// 189 listen_msg
	TV(L"Tăng hiệu suất hồi sinh lực (%)"),	// 190 lifereplenish_p
	TV(L"Xác suất bỏ qua Chú Thuật Giảm Kháng Tính (%)"),	// 191 ignoreskill_p
	TV(L"Xác suất phản đòn Chú Thuật (%)"),	// 192 returnskill_p
	TV(L"Phản đòn khi chịu sát thương Độc (điểm)"),	// 193 poisondamagereturn_v
	TV(L"Phản đòn khi chịu sát thương Độc (%)"),	// 194 poisondamagereturn_p
	TV(L"Khi bị tấn công có x tự động thi triểnxcấpxThời gian chờx giây"),	// 195 autoreplyskill
	"",	// 196 autoattackskill
	"",	// 197 autorescueskill
	"",	// 198 autodeathskill
	"",	// 199 randmove
	TV(L"ẩn thân x giây, thi triển võ công sẽ hiện hình trở lại"),	// 200 hide
	"",	// 201 ignorenegativestate_p
	TV(L"Khiến mục tiêu khi chịu sát thương Độc sẽ đồng thời bị mất nội lực với tỉ lệ x, duy trì x giây"),	// 202 poison2decmana_p
	"",	// 203 staticmagicshield_v
	"",	// 204 staticmagicshield_p
	TV(L"Kháng phản đòn (%)"),	// 205 returnres_p
	"",	// 206 add120skillexpenhance_p
	TV(L"Giảm thời gian chờ"),	// 207 dec_percasttimehorse
	TV(L"Giảm thời gian chờ"),	// 208 dec_percasttime
	TV(L"Xác suất kích hoạt [Sư Tử Hống] (%)"),	// 209 enhance_709_auto
	TV(L"Hiệu suất hồi sinh lực từ [Đảo Hư Thiên] tăng x"),	// 210 enhance_708_life_p
	TV(L"Điểm hồi phục sinh lực từ [Từ Hàng Phổ Độ] (điểm)"),	// 211 enhance_93_life_v
	TV(L"Xác suất kích hoạt [Hấp Tinh Yểm] (%)"),	// 212 enhance_711_auto
	TV(L"Xác suất kích hoạt [Hỗn Thiên Khí Công] (%)"),	// 213 enhance_714_auto
	TV(L"Xác suất kích hoạt [Lưỡng Nghi Chân Khí] (%)"),	// 214 enhance_717_auto
	TV(L"Xác suất [Ma Âm Phệ Phách] khiến kẻ địch Hoảng Sợ (%)"),	// 215 enhance_723_miss_p
	"",	// 216 showpersoninfo
	"",	// 217 ignoredamage
	TV(L"Triệt tiêu sát thương"),	// 218 sorbdamage_p
	TV(L"Tạo thành sát thương có thời gian hoạt động"),	// 219 anti_hitrecover
	TV(L"Tạo thành thời gian choáng (%)"),	// 220 anti_stuntimereduce_p
	TV(L"Bỏ qua Độc Phòng mục tiêu (%)"),	// 221 anti_poisonres_p
	TV(L"Bỏ qua Hỏa Phòng mục tiêu (%)"),	// 222 anti_fireres_p
	TV(L"Bỏ qua Lôi Phòng mục tiêu (%)"),	// 223 anti_lightingres_p
	TV(L"Bỏ qua Phòng Thủ mục tiêu (%)"),	// 224 anti_physicsres_p
	TV(L"Bỏ qua Băng Phòng mục tiêu (%)"),	// 225 anti_coldres_p
	TV(L"Xác suất hóa giải sát thương (%)"),	// 226 block_rate
	TV(L"Xác suất trọng kích (%)"),	// 227 enhancehit_rate
	TV(L"Kháng độc (%) (Dương)"),	// 228 poisonres_yan_p
	TV(L"Kháng lôi (%) (Dương)"),	// 229 lightingres_yan_p
	TV(L"Kháng hỏa (%) (Dương)"),	// 230 fireres_yan_p
	TV(L"Phòng thủ vật lý (%) (Dương)"),	// 231 physicsres_yan_p
	TV(L"Kháng băng (%) (Dương)"),	// 232 coldres_yan_p
	TV(L"Sinh lực tối đa (điểm) (Dương)"),	// 233 lifemax_yan_v
	TV(L"Sinh lực tối đa (%) (Dương)"),	// 234 lifemax_yan_p
	TV(L"Nội lực tối đa (điểm) (Dương)"),	// 235 manamax_yan_v
	TV(L"Nội lực tối đa (%) (Dương)"),	// 236 manamax_yan_p
	TV(L"Triệt tiêu sát thương (Dương)"),	// 237 sorbdamage_yan_p
	TV(L"Tốc độ chạy (%) (Dương)"),	// 238 fastwalkrun_yan_p
	TV(L"Tốc độ đánh - ngoại công (%) (Dương)"),	// 239 attackspeed_yan_v
	TV(L"Tốc độ đánh - nội công (%) (Dương)"),	// 240 castspeed_yan_v
	TV(L"Kháng tất cả (%) (Dương)"),	// 241 allres_yan_p
	TV(L"Khiến Kháng tất cả của đối phương được (%)"),	// 242 anti_maxres_p
	TV(L"Tăng công kích kỹ năng (%)"),	// 243 skill_enhance
	TV(L"Sát thương vật lý - nội công (%)"),	// 244 magicdamage_p
	TV(L"Thời gian phục hồi (Dương)"),	// 245 fasthitrecover_yan_v
	TV(L"Ngũ Hành Cường Hóa (điểm)"),	// 246 five_elements_enhance_v
	TV(L"Nhược hóa Ngũ Hành (điểm)"),	// 247 five_elements_resist_v
	TV(L"Nâng cao hiệu suất phục hồi nội lực (%)"),	// 248 manareplenish_p
	TV(L"Lực công kích (%)"),	// 249 add_damage_p
	"",	// 250 forbit_attack
	"",	// 251 frozen_action
	"",	// 252 forbit_takemedicine
	"",	// 253 invincibility
	TV(L"Tỉ lệ không tăng trị số PK (%)"),	// 254 not_add_pkvalue_p
	TV(L"Sát thương lên boss Hoàng Kim (%)"),	// 255 add_boss_damage
	"",	// 256 pk_punish_weaken
	"",	// 257 pk_punish_enhance
	TV(L"Thời gian tạo thành trúng độc (%)"),	// 258 anti_poisontimereduce_p
	TV(L"Tỷ lệ tạo thành sát thương (%)"),	// 259 do_hurt_p
	TV(L"Tỷ lệ sát thương giảm kháng (%)"),	// 260 anti_do_hurt_p
	TV(L"Tỷ lệ tạo thành choáng (%)"),	// 261 do_stun_p
	TV(L"Tỉ lệ kháng choáng (%)"),	// 262 anti_do_stun_p
	TV(L"Bỏ qua phổ phòng đối phương (%) (Dương)"),	// 263 anti_physicsres_yan_p
	TV(L"Bỏ qua độc phòng đối phương (%) (Dương)"),	// 264 anti_poisonres_yan_p
	TV(L"Bỏ qua băng phòng đối phương (%) (Dương)"),	// 265 anti_coldres_yan_p
	TV(L"Bỏ qua hỏa phòng đối phương (%) (Dương)"),	// 266 anti_fireres_yan_p
	TV(L"Bỏ qua lôi phòng đối phương (%) (Dương)"),	// 267 anti_lightingres_yan_p
	TV(L"Bỏ qua tất cả kháng tính đối phương (%) (Dương)"),	// 268 anti_allres_yan_p
	TV(L"Tăng sát thương"),	// 269 anti_sorbdamage_yan_p
	TV(L"Tỷ lệ hóa giải cách đảng (%)"),	// 270 anti_block_rate
	TV(L"Tỷ lệ giảm trọng kích (%)"),	// 271 anti_enhancehit_rate
	"",	// 272 autocastskill
	"",	// 273 transfigure
	"",	// 274 oncastskill
	"",	// 275 enhancehiteffect_rate
	TV(L"Tăng sát thương đối với hệ Kim (%)"),	// 276 me2metaldamage_p
	TV(L"Giảm sát thương đến từ hệ Kim (%)"),	// 277 metal2medamage_p
	TV(L"Tăng sát thương đối với hệ Mộc (%)"),	// 278 me2wooddamage_p
	TV(L"Giảm sát thương đến từ hệ Mộc (%)"),	// 279 wood2medamage_p
	TV(L"Tăng sát thương đối với hệ Thủy (%)"),	// 280 me2waterdamage_p
	TV(L"Giảm sát thương đến từ hệ Thủy (%)"),	// 281 water2medamage_p
	TV(L"Tăng sát thương đối với hệ Hỏa (%)"),	// 282 me2firedamage_p
	TV(L"Giảm sát thương đến từ hệ Hỏa (%)"),	// 283 fire2medamage_p
	TV(L"Tăng sát thương đối với hệ Thổ (%)"),	// 284 me2earthdamage_p
	TV(L"Giảm sát thương đến từ hệ Thổ (%)"),	// 285 earth2medamage_p
	TV(L"Giảm nội lực phe địch khi chịu sát thương tấn công tầm gần (%)"),	// 286 meleedamagereturnmana_p
	TV(L"Giảm nội lực phe địch khi chịu sát thương tấn công tầm xa (%)"),	// 287 rangedamagereturnmana_p
	TV(L"Giảm x Thời gian đóng băngx giây"),	// 288 reduceskillcd1
	TV(L"Giảm x Thời gian đóng băngx giây"),	// 289 reduceskillcd2
	TV(L"Giảm x Thời gian đóng băngx giây"),	// 290 reduceskillcd3
	TV(L"x xác suất xóa tất cả thời gian chờ kỹ năng của bản thân"),	// 291 clearallcd
	TV(L"Xung quanh mỗi x cá nhân x tỷ lệ hóa giải sát thương"),	// 292 addblockrate
	TV(L"Lăng Ba Vi Bộ"),	// 293 walkrunshadow
	"",	// 294 -
	"",	// 295 candetonate1
	"",	// 296 candetonate2
	"",	// 297 candetonate3
	TV(L"Khi nội công đầy tăng kỹ năng công kích (%)"),	// 298 manatoskill_enhance
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
