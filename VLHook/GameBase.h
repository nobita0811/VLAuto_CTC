#pragma once

#define	MAX_NPC					800	// 10-10: new KNpc[0x320]
#define	MAX_OBJ					256
#define	MAX_ITM					512
#define	MAX_NPCSKILL			80
#define MAX_TOP_TONG			25
#define MAX_TEAM_MEMBER			8

#define REGION_WIDTH			512
#define REGION_HEIGHT			1024
#define CELL_WIDTH				32
#define CELL_HEIGHT				32
#define MAX_FOLLOW_DISTANCE		48
#define	MIN_RETURN_DISTANCE		64
#define	PLAYER_PICKUP_DISTANCE	128
#define	MAX_PLAYER_MOVE_FRAME	7
#define MAX_MOVE_DISTANCE		400
#define MAX_ROOM_SPACE			60

#define	NGAMY_BUFF_SKILLID		0x005D
#define NGAMY_BUFF_RADIUS		400
#define WND_N_MENU_CLICK		0x0502
#define WND_N_BUTTON_CLICK		0x0565

//Game Data
#define _AUTOXP
#ifdef _AUTOXP
	#define ACNT_ID_ADD			0x009BD2D8	// AccountID (10-10): g_LoginLogic.m_Choices.Account, dao bit
	#define MAP_ID_ADD			0x021D6840	// Game Map ID (10-10)
	#define MAP_NAME_ADD		0x021D6820	// Game Map Name (10-10)
	#define CORE_BASE_ADD		0x009BC130	// KCoreShell (pointer) (10-10)
	#define NPC_BASE_ADD		0x01A3B4B4	// KNpc (pointer) (10-10)
	#define PLR_BASE_ADD		0x009C0474	// KPlayer (pointer) (10-10)
#else
	#define ACNT_ID_ADD			theGameData.AccountID
	#define MAP_ID_ADD			theGameData.MapAddress
	#define NPC_BASE_ADD		theGameData.NpcBaseAdd
	#define PLR_BASE_ADD		theGameData.PlrBaseAdd
	#define CORE_BASE_ADD		theGameData.KCoreShell
#endif

#define	MAX_PLAYER_ITEM	483	// 10-10
#define NPC_DATA_SIZE	0x00012BD4	// 10-10
#define OBJ_DATA_SIZE	0x03F4	// 10-10
#define ITM_DATA_SIZE	0x074C	// 10-10
#define SKL_DATA_SIZE	0x04FC	// 10-10: KSkillManager::m_SkillInfo[3000] (cu 0x4F8)
#define PLAYER_OFFSET	0xA878		// 10-10
#define MSG_PLR_OFFSET	0x0108		// Inviter (10-10)

#define FLAG_X_OFFST	0x00811D24	// MiniMap flag X = x>>4 (10-10)
#define FLAG_Y_OFFST	0x00811D28	// MiniMap flag Y = y>>5 (10-10)
#define LIFETIME_DYS	0x006DEFA4	// DeadLine Life Time
#define LIFETIME_HRS	0x006DEFA8	// Remaining Life Time
#define LEVEL_EXP_TB	0x024F1E4C	// Level Exp Table int[0x104] = PlayerSet(0x24F1E18)+0x34 (10-10)

// Con tro UI chua tim duoc o ban 10-10: GetPtrAddress() tra ve NULL
extern DWORD_PTR g_dwNullUiPtr;
#define NULL_PTR_ADD	((DWORD_PTR)&g_dwNullUiPtr)

// UI Classes
#define OBJ_BASE_ADD	0x021A1898	// KObj (pointer) (10-10)
#define SKL_BASE_ADD	0x01AB4A10	// KSkill (static) = g_SkillManager.m_SkillInfo (10-10; Init 0x60D740)
#define ITM_BASE_ADD	0x01F18E14	// KItem (pointer) (10-10)
#define IST_BASE_ADD	NULL_PTR_ADD	// KStallObject (pointer) - 10-10 CHUA TIM, tro toi 0 de bo qua
#define TEAM_ADDRESS	0x01F18A80	// KTeam = &g_Team(0x1F18A78).m_nCaptain (10-10)
#define UPB_BASE_ADD	0x00839890	// KUiPlayerBar (pointer) (10-10)
#define SBO_BASE_ADD	0x0084051C	// KUiStoreBox (pointer) (10-10)
#define MCP_BASE_ADD	0x008478A8	// KUiMsgCentrePad (pointer) (10-10)
#define SMC_BASE_ADD	0x00847EF4	// KUiSysMsgCentre (pointer) (10-10)
#define YSB_BASE_ADD	NULL_PTR_ADD	// KUiYeSouBox (pointer) - 10-10 CHUA TIM, tro toi 0 de bo qua
#define MSG_BASE_ADD	0x008C13D8	// KUiMsgSel (pointer) (10-10)
#define MSG2_BASE_ADD	0x008C1368	// KUiMsgSel2 (pointer) (10-10)
#define SHOP_BASE_ADD	0x008478E0	// KUiShop (pointer) (10-10)
#define MMP_BASE_ADD	0x00847EF8	// KUiMiniMap::m_pSelf (10-10) - SetMapFlag khong dung this
#define UIN_BASE_ADD	0x00840460	// KUiInit (pointer) (10-10)
#define IF1_BASE_ADD	0x00842460	// KUiInformation (static) (10-10)
#define IF2_BASE_ADD	0x00843B68	// KUiInformation2 (static) (10-10)
#define TMC_BASE_ADD	NULL_PTR_ADD	// KUiTongManagement - 10-10 CHUA TIM (0 = tat)
#define STL_BASE_ADD	NULL_PTR_ADD	// KUiPlayerStall (pointer) - 10-10 CHUA TIM, tro toi 0 de bo qua
#define CMP_BASE_ADD	NULL_PTR_ADD	// KUiCompound (pointer) - 10-10 CHUA TIM, tro toi 0 de bo qua
#define PZS_BASE_ADD	NULL_PTR_ADD	// KUiPrizeSel (pointer) - 10-10 CHUA TIM, tro toi 0 de bo qua
#define ITX_BASE_ADD	NULL_PTR_ADD	// KUiItemEx (pointer) - 10-10 CHUA TIM, tro toi 0 de bo qua
#define WLD_BASE_ADD	0x009C08A8	// KSubWorld (pointer) (10-10)
#define NSET_BASE_ADD	0x021A18A8	// KNpcSet (static) (10-10)
#define BSL_BASE_ADD	0x024E7750	// KBuySell (static) (10-10)

// UI Functions
#define UWP_FUNC_ADD	0x00478440	// KUiPlayerBar::WndProc (10-10) = vtable 0x791784[4]
#define OSM_FUNC_ADD	0x004768D0	// KUiPlayerBar::OnSendSomeoneMessage (10-10) cdecl(name,msg,len)
#define IOC_FUNC_ADD	0x0048FFB0	// KUiInit::OnClickButton (10-10)
#define SBO_FUNC_ADD	0x00498F80	// KUiStoreBox::OpenWindow Function (10-10)
#define IH1_FUNC_ADD	0x004AE490	// KUiInformation::Hide (10-10)
#define IH2_FUNC_ADD	0x004AE940	// KUiInformation2::Hide (10-10)
#define SMA_FUNC_ADD	0x004B9340	// 10-10: KUiMsgCentrePad::SystemMessageArrival(msg, len, bChannelBox)  thông báo hệ thống
#define SHO_FUNC_ADD	0x004BF2F0	// KUiShop::OpenWindow Function (10-10)
#define PCP_FUNC_ADD	0x004C4530	// ProcessPeople (10-10)
#define SMD_FUNC_ADD	0x004C5390	// KUiSysMsgCentre::DeleteMsgInHeap (10-10)
#define SMF_FUNC_ADD	0x004C7C70	// MiniMap goto: cdecl(x, y, bWorld) toa do tuyet doi (10-10)
#define TGL_FUNC_ADD	0	// KUiTongManagement::GetTongList - 10-10 CHUA TIM (0 = tat)
#define TGP_FUNC_ADD	0	// KUiTongManagement::PrepareTongList - 10-10 CHUA TIM (0 = tat)
#define OMS2_FUNC_ADD	0x00509E40	// KUiMsgSel2::OnClickMsg Function (10-10)
#define OCB_FUNC_ADD	0	// KUiYeSouBox::OnClickBtn - 10-10 CHUA TIM (0 = tat)
#define PSC_FUNC_ADD	0	// KUiPrizeSel::CloseWindow - 10-10 CHUA TIM (0 = tat)
#define OMS_FUNC_ADD	0x0051EB50	// KUiMsgSel::OnClickMsg Function (10-10)
#define EXS_FUNC_ADD	0x004360B0	// ExecuteScript Function (10-10)
#define CLA_FUNC_ADD	0x0044C560	// CloseAllWindows Function (10-10)
#define GGD_FUNC_ADD	0x005B9F30	// KCoreShell::GetGameData Function (10-10)
#define OPR_FUNC_ADD	0x005C3820	// KCoreShell::OperationRequest Function (10-10)
#define TNP_FUNC_ADD	0x005C2050	// KCoreShell::TongOperation Function (10-10)
#define TOP_FUNC_ADD	0x005BB090	// KCoreShell::TeamOperation Function (10-10)
#define PGS_FUNC_ADD	0x005BB3C0	// KCoreShell::ProcessGameSecurity Function (10-10)
#define PAA_FUNC_ADD	0x005DEA20	// KPlayer::ApplyAddBaseAttribute (10-10)
#define PKO_FUNC_ADD	0x005E2530	// KPlayer::PickUpObj Function (10-10)
#define DNPC_FUNC_ADD	0x005E4C80	// KPlayer::DialogNpc (10-10)
#define GRL_FUNC_ADD	0x005BAAF0	// KCoreShell::GetNPCRelation (10-10)
#define MVT_FUNC_ADD	0x005F3B30	// KNpc::MoveTo Function (10-10)
#define SKL_FUNC_ADD	0x005ECED0	// SendClientCmdSkill Function (10-10)
#define IGR_FUNC_ADD	0x0063A500	// KItem::GetRepairPrice (10-10)
#define GTV_BASE_ADD	0x0061C820	// KPlayerTask::GetTaskVal Function (10-10)
#define BGI_FUNC_ADD	0x006B9390	// KBuySell::GetItem Function (10-10)
#define BGX_FUNC_ADD	0x006B93E0	// KBuySell::GetItemIndex Function (10-10)

// Ve trong game (MapPath DEBUG) (10-10)
#define RPS_BASE_ADD	0x01F182F0	// g_pRepresent (iRepresentShell*) - SetRepresentShell / KObj::Draw
#define RPS_VT_OUTTEXT	0x14		// iRepresentShell::OutputText (9 tham so) - vtable offset
#define RPS_VT_DRAWPRIM	0x4C		// iRepresentShell::DrawPrimitives (4 tham so) - vtable offset
#define DGS_JMP_ADD		0x004C3AC6	// KUiGameSpace::PaintWindow: mov ecx,[CORE]; JMP DrawGameSpace
#define DGS_FUNC_ADD	0x005BAB50	// KCoreShell::DrawGameSpace (ScenePlace.Paint + DrawSelectInfo)

// Trap ban do (cong di bo) - KScenePlaceRegionC::m_TrapInfo nap tu <x>_<y>Region_C.dat (10-10)
#define SCP_BASE_ADD	0x021A3F18	// g_ScenePlace (KScenePlaceC) - DrawGameSpace: mov ecx,0x21A3F18; call Paint
#define SCP_REGIONS_OFF	0x31E64		// KScenePlaceC::m_pInProcessAreaRegions[] - Paint: vong 15 lan PaintGround
#define SCP_NUM_REGIONS	15			// 10-10: 15 region (source: 9)
#define SPR_STATUS_OFF	0x04		// KScenePlaceRegionC::m_Status (3 = REGION_S_STANDBY - Load 0x7145E0)
#define SPR_LEFTTOP_OFF	0x08		// m_LeftTopCornerScenePos: x = RegionX << 9, y = RegionY << 10
#define SPR_TRAP_OFF	0x848		// m_TrapInfo[16][32] (o 32x32 Mps) - LoadTrap 0x711F00, GetTrap(mpsX, mpsY) 0x712560

// Giam cau hinh (DEBUG tab Giam cau hinh) (10-10)
#define LCF_PAINT_MS_ADD	0x00815838	// So ms giua 2 lan ve = 1000 / PaintFps (GameInit 0x593A25: config.ini [Client] PaintFps, mac dinh 60)
#define LCF_INACTIVE_ADD	0x009BC508	// KMyApp(0x9BC4B8)+0x50: tick luc mat focus. != 0 va qua 10s => GameLoop 0x591640 khong ve (0x44C190 thay UiPaint)
#define LCF_VISIBLE_ADD		0x0081D15C	// OPTION_VISIBLE_NUM: so nguoi choi hien thi (mac dinh 200, <= 0 an) - kiem tra o 0x678D12
#define LCF_KOPTION_ADD		0x0220F9AC	// KOption: +0 m_nSndVolume, +4 m_nMusicVolume (SetSndVolume 0x678E10 / SetMusicVolume 0x678E60)
#define SCP_WEATHER_OFF		0x06		// BYTE KScenePlaceC::m_bEnableWeather (EnableWeather 0x6753C0)
#define SCP_DYNALIGHT_OFF	0x32A52		// BYTE bong dong: KIpoTree(+0x329F8)+0x5A (EnableDynamicLights 0x6752D0 -> 0x715560)

#define GOTO_BASE_ADD	0x01F18DA0	// tim thay trong ham CMD_FUNC_ADD
#define GOTO_FUNC_ADD	0x00630FE0	// hàm này này hàm tự tìm đường và di chuyển tới điểm x/32,y/32 (tuyệt đối) trong game


// Code Injection
#define _INJ_ADD_		0x005550DA	// CALL _ORG_PROC_
#define _RET_ADD_		0x005550DF	// TEST EAX,EAX
#define _ORG_PROC_		0x005AE5C0	// KObjSet::ClientAdd Function

#define WND_S_VISIBLE		0x80000000
#define WND_S_DISABLED		0x40000000
#define	WND_S_TOPMOST		0x20000000
#define	WND_S_MOVEALBE		0x10000000
#define	WND_S_DISABLE		0x04000000
#define SKILL_SHORTCUT		0x00040004

#define SCK_SHORTCUT_SIT	"Switch([[sit]])"
#define SCK_SHORTCUT_HORSE	"Switch([[horse]])"

#define AMULET_PAPER		"Thæ ®Þa phï"
#define AMULETX_PAPER		"Håi thµnh phï"
#define KC_LENH				"Khiªu chiÕn lÖnh"
#define FY_ITEM				"Phóc Duyªn Lé"
#define VISITED_PLACES		"Nh÷ng n¬i ®· ®i qua"
#define AGREE_TRADE			"Giao DÞch"
#define FULL_REPAIR			"Söa kü ["
