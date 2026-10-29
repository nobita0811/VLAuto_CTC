#pragma once

#define	MAX_NPC					256	// 15-10: new KNpc[0x100] (10-10: 800)
#define	MAX_OBJ					256	// 15-10: new KObj[0x100]
#define	MAX_ITM					1024	// 15-10: new KItem[0x400] (10-10: 512)
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

// ==========================================================================
// Ban client 15_10 (gamethienha_y_15_10.exe). Xem Offsets_15_10.md.
// Do bang cach doi chieu ma may voi ban 10_10 (gamecl_dump_SCY_10_10.exe).
// ==========================================================================

//Game Data
#define _AUTOXP
#ifdef _AUTOXP
	#define ACNT_ID_ADD			0x008491E8	// AccountID (15-10): g_LoginLogic(0x848A68)+0x780, dao bit
	#define MAP_ID_ADD			0x0175D6E8	// Game Map ID (15-10)
	#define MAP_NAME_ADD		0x0175D6C8	// Game Map Name (15-10)
	#define CORE_BASE_ADD		0x00848314	// KCoreShell (pointer) (15-10)
	#define NPC_BASE_ADD		0x017220F0	// KNpc (pointer) (15-10)
	#define PLR_BASE_ADD		0x00EE403C	// KPlayer (pointer) (15-10)
#else
	#define ACNT_ID_ADD			theGameData.AccountID
	#define MAP_ID_ADD			theGameData.MapAddress
	#define NPC_BASE_ADD		theGameData.NpcBaseAdd
	#define PLR_BASE_ADD		theGameData.PlrBaseAdd
	#define CORE_BASE_ADD		theGameData.KCoreShell
#endif

#define	MAX_PLAYER_ITEM	678	// 15-10: memset 0x34F8 (10-10: 483)
#define MAX_EQUIP_SLOT	16	// 15-10: KItemList::m_EquipItem {nIdx, nData}[16] (10-10: int[17])
#define NPC_DATA_SIZE	0x00011AB8	// 15-10 (10-10: 0x12BD4)
#define OBJ_DATA_SIZE	0x04E8	// 15-10 (10-10: 0x3F4)
#define ITM_DATA_SIZE	0x0864	// 15-10 (10-10: 0x74C)
#define SKL_DATA_SIZE	0x04F8	// 15-10: KSkillManager::m_SkillInfo[3000] (10-10: 0x4FC)
#define PLAYER_OFFSET	0xA878		// 15-10 (khong doi)
#define MSG_PLR_OFFSET	0x0408		// Inviter = KSystemMessage+0x408 (15-10, 10-10: 0x108)
#define MAX_LEVEL_EXP	999			// 15-10: bang level_exp int[999] (10-10: 0x104)

#define FLAG_X_OFFST	0x007AAA58	// MiniMap flag X = x>>4 (15-10: SetMapFlag 0x4DE840)
#define FLAG_Y_OFFST	0x007AAA5C	// MiniMap flag Y = y>>5 (15-10)
#define LIFETIME_DYS	0x006DEFA4	// DeadLine Life Time (khong dung o VLTK_FREE_MODE)
#define LIFETIME_HRS	0x006DEFA8	// Remaining Life Time (khong dung o VLTK_FREE_MODE)
#define LEVEL_EXP_TB	0x01A7D518	// Level Exp Table int[999] = PlayerSet(0x1A7D4E0)+0x38 (15-10)

// Con tro UI chua tim duoc: GetPtrAddress() tra ve NULL
extern DWORD_PTR g_dwNullUiPtr;
#define NULL_PTR_ADD	((DWORD_PTR)&g_dwNullUiPtr)

// UI Classes
#define OBJ_BASE_ADD	0x017286B4	// KObj (pointer) (15-10)
#define SKL_BASE_ADD	0x0084ACB8	// KSkill (static) = g_SkillManager.m_SkillInfo (15-10; 0x5D23B0: [ecx + id*0x4F8 - 0x4F8])
#define ITM_BASE_ADD	0x01722148	// KItem (pointer) (15-10)
#define TEAM_ADDRESS	0x00EE4850	// KTeam = &g_Team(0xEE4848).m_nCaptain (15-10, layout khong doi)
#define UPB_BASE_ADD	0x007BE494	// KUiPlayerBar (pointer) (15-10)
#define SBO_BASE_ADD	0x007C488C	// KUiStoreBox (pointer) (15-10)
#define MCP_BASE_ADD	0x007C9BE8	// KUiMsgCentrePad (pointer) (15-10)
#define SMC_BASE_ADD	0x007CA1FC	// KUiSysMsgCentre (pointer) (15-10)
#define YSB_BASE_ADD	0x007CAD4C	// KUiYeSouBox (pointer) (15-10: OpenWindow 0x5165D0, new 0x3794; Items 0x34C, Assemble 0x2CC4, Close 0x3228)
#define MSG_BASE_ADD	0x007CBD2C	// KUiMsgSel (pointer) (15-10)
#define MSG2_BASE_ADD	0x007CB470	// KUiMsgSel2 (pointer) (15-10)
#define SHOP_BASE_ADD	0x007C9D3C	// KUiShop (pointer) (15-10)
#define MMP_BASE_ADD	0x007CA200	// KUiMiniMap::m_pSelf (15-10) - SetMapFlag khong dung this
#define UIN_BASE_ADD	0x007C47D8	// KUiInit (pointer) (15-10)
#define IF1_BASE_ADD	0x007C5DD8	// KUiInformation (static) (15-10)
#define IF2_BASE_ADD	0x007C6CC8	// KUiInformation2 (static) (15-10)
#define TMC_BASE_ADD	NULL_PTR_ADD	// KUiTongManagement - CHUA TIM (0 = tat)
#define STL_BASE_ADD	0x007CAD54	// KUiPlayerStall (pointer) (15-10) - hang lay bang GetGameData(GDI_STALL_ITEMS)
#define CMP_BASE_ADD	NULL_PTR_ADD	// KUiCompound (pointer) - CHUA TIM, tro toi 0 de bo qua
#define PZS_BASE_ADD	0x007CBD1C	// KUiPrizeSel (pointer) (15-10: OpenWindow 0x53EA00, new 0x3804)
#define ITX_BASE_ADD	NULL_PTR_ADD	// KUiItemEx (pointer) - CHUA TIM, tro toi 0 de bo qua
#define WLD_BASE_ADD	0x017286B0	// KSubWorld (pointer) (15-10)
#define NSET_BASE_ADD	0x017286C0	// KNpcSet (static) (15-10)
#define BSL_BASE_ADD	0x01A6A620	// KBuySell (static) (15-10)

// UI Functions
#define UWP_FUNC_ADD	0x0046FCA0	// KUiPlayerBar::WndProc (15-10) = vtable 0x751A84[4]
#define OSM_FUNC_ADD	0			// KUiPlayerBar::OnSendSomeoneMessage - 15-10 KHONG CO (0 = tat nhan tin)
#define IOC_FUNC_ADD	0x00490EA0	// KUiInit::OnClickButton (15-10)
#define SBO_FUNC_ADD	0x004991B0	// KUiStoreBox::OpenWindow Function (15-10)
#define IH1_FUNC_ADD	0x004B7440	// KUiInformation::Hide(n) (15-10, m_pCallerWnd +0xFF0)
#define IH2_FUNC_ADD	0x004B80E0	// KUiInformation2::Hide (15-10)
#define SMA_FUNC_ADD	0x004BD680	// 15-10: KUiMsgCentrePad::SystemMessageArrival(msg, len, bChannelBox)  thong bao he thong
#define SHO_FUNC_ADD	0x004CB1F0	// KUiShop::OpenWindow Function (15-10)
#define PCP_FUNC_ADD	0x004D7A40	// ProcessPeople (15-10)
#define SMD_FUNC_ADD	0x004DA540	// KUiSysMsgCentre::DeleteMsgInHeap (15-10)
#define SMF_FUNC_ADD	0x004DE840	// MiniMap goto: cdecl(x, y, bWorld) toa do tuyet doi (15-10, OPR 0x81)
#define TGL_FUNC_ADD	0	// KUiTongManagement::GetTongList - CHUA TIM (0 = tat)
#define TGP_FUNC_ADD	0	// KUiTongManagement::PrepareTongList - CHUA TIM (0 = tat)
#define OMS2_FUNC_ADD	0x00522120	// KUiMsgSel2::OnClickMsg Function (15-10)
#define OCB_FUNC_ADD	0x00516790	// KUiYeSouBox::CloseWindow(bool bAssemble) cdecl (15-10): OPR(0x3B) + OPR(0x5D, bAssemble) + dong
#define PSC_FUNC_ADD	0x0053EAE0	// KUiPrizeSel::CloseWindow(int) cdecl (15-10, WndProc 0x53F110: OPR(9,0,n) + CloseWindow(0))
#define OMS_FUNC_ADD	0x00542110	// KUiMsgSel::OnClickMsg Function (15-10)
#define EXS_FUNC_ADD	0x00423B40	// ExecuteScript Function (15-10)
#define CLA_FUNC_ADD	0x00432B10	// CloseAllWindows Function (15-10)
#define GGD_FUNC_ADD	0x005BC2D0	// KCoreShell::GetGameData Function (15-10)
#define OPR_FUNC_ADD	0x005C1180	// KCoreShell::OperationRequest Function (15-10)
#define TNP_FUNC_ADD	0x005C5300	// KCoreShell::TongOperation Function (15-10)
#define TOP_FUNC_ADD	0x005BD440	// KCoreShell::TeamOperation Function (15-10)
#define PGS_FUNC_ADD	0x005BD780	// KCoreShell::ProcessGameSecurity Function (15-10)
#define PAA_FUNC_ADD	0x005E8E70	// KPlayer::ApplyAddBaseAttribute (15-10)
#define PKO_FUNC_ADD	0x005EC7E0	// KPlayer::PickUpObj Function (15-10)
#define DNPC_FUNC_ADD	0x005E9880	// KPlayer::DialogNpc (15-10)
#define GRL_FUNC_ADD	0x005BD0C0	// KCoreShell::GetNPCRelation (15-10)
#define MVT_FUNC_ADD	0x005F8CF0	// KNpc::MoveTo Function (15-10)
#define NMP_FUNC_ADD	0x005BC920	// KNpc::GetMpsPos thiscall(int *x, int *y) (15-10): SubWorld[m_SubWorldIndex(+0x103C)].Map2Mps(m_RegionIndex(+0x1040), ...) 0x630AD0
#define SKL_FUNC_ADD	0x00644CB0	// SendClientCmdSkill(skill, x, y) Function (15-10, goi tin co them token)
#define IGR_FUNC_ADD	0x006051B0	// KItem::GetRepairPrice (15-10)
#define GTV_BASE_ADD	0x00609990	// KPlayerTask::GetTaskVal Function (15-10)
#define BGI_FUNC_ADD	0x006852B0	// KBuySell::GetItem Function (15-10)
#define BGX_FUNC_ADD	0x00685300	// KBuySell::GetItemIndex Function (15-10)

// Ve trong game (MapPath DEBUG) (15-10)
#define RPS_BASE_ADD	0x00EDFC88	// g_pRepresent (iRepresentShell*) - SetRepresentShell / KObj::Draw
#define RPS_VT_OUTTEXT	0x10		// iRepresentShell::OutputText (9 tham so) - vtable offset (15-10, 10-10: 0x14)
#define RPS_VT_DRAWPRIM	0x48		// iRepresentShell::DrawPrimitives (4 tham so) - vtable offset (15-10, 10-10: 0x4C)
#define DGS_JMP_ADD		0x004D78CD	// KUiGameSpace::PaintWindow: mov ecx,[CORE]; CALL DrawGameSpace (15-10: E8, 10-10: E9)
#define DGS_FUNC_ADD	0x005BD120	// KCoreShell::DrawGameSpace (ScenePlace.Paint + DrawSelectInfo)
#define GOLD_BLUE_MAX_ADD	0x01729548	// int: ve ten NPC (0x5FDD6C) GetGoldType 0x6D4600 (IsGold && IsGolding ? m_nGoldType+1 : 0) <= gia tri nay => ten xanh (0xFF6365FF), lon hon => vang (0xFFEBB200)
#define NPCP_FUNC_ADD	0x005F7CA0	// KNpc::Paint thiscall(0) (15-10): xoay m_ResDir(+0x1DC4) ve m_Dir roi m_DataRes(+0x1DC8).Draw (KNpcRes::Draw 0x6BEEC0)
#define NPCP_CALL_ADD	0x00642B23	// CoreDrawGameObj 0x642940 (genre NPC, nParam & 2 = ve than): E8 call KNpc::Paint

// Trap ban do (cong di bo) - KScenePlaceRegionC::m_TrapInfo nap tu <x>_<y>Region_C.dat (15-10)
#define SCP_BASE_ADD	0x0172B1F8	// g_ScenePlace (KScenePlaceC) - DrawGameSpace: mov ecx,0x172B1F8; call Paint
#define SCP_REGIONS_OFF	0x31E4C		// KScenePlaceC::m_pInProcessAreaRegions[] - Paint 0x63C2E0: vong 9 lan PaintGround
#define SCP_NUM_REGIONS	9			// 15-10: 9 region (10-10: 15)
#define SPR_STATUS_OFF	0x04		// KScenePlaceRegionC::m_Status (3 = REGION_S_STANDBY - Load 0x6FA6B0)
#define SPR_LEFTTOP_OFF	0x08		// m_LeftTopCornerScenePos: x = RegionX << 9, y = RegionY << 10
#define SPR_TRAP_OFF	0x848		// m_TrapInfo[16][32] (o 32x32 Mps) - LoadTrap 0x6F80D0, GetTrap(mpsX, mpsY) 0x6F86B0

// Giam cau hinh (DEBUG tab Giam cau hinh) (15-10)
#define LCF_PAINT_FPS_ADD	0x007AD7B0	// 15-10: PaintFps (so khung/giay, config.ini [Client] PaintFps, mac dinh 30). GameLoop 0x5A9160 ve khi frame*1000 <= tick*PaintFps
#define LCF_PAINT_FPS_DEF	30			// gia tri mac dinh cua game
#define LCF_INACTIVE_ADD	0x008483B0	// KMyApp(0x848360)+0x50: tick luc mat focus. != 0 va qua 10s => GameLoop 0x5A9160 khong ve (0x432630 thay UiPaint)
#define LCF_VISIBLE_ADD		0x007AE608	// OPTION_VISIBLE_NUM: so nguoi choi hien thi (OPR 0x2B uParam 6 -> 0x63A670)
#define LCF_KOPTION_ADD		0x017969C0	// KOption: +0 m_nSndVolume, +4 m_nMusicVolume (SetSndVolume 0x6443C0 / SetMusicVolume 0x644410)
#define SCP_WEATHER_OFF		0x06		// BYTE KScenePlaceC::m_bEnableWeather (EnableWeather 0x63B9F0)
#define SCP_DYNALIGHT_OFF	0x3261A		// BYTE bong dong: KIpoTree(+0x325C0)+0x5A (EnableDynamicLights 0x63B900 -> 0x6FB4B0)

#define GOTO_BASE_ADD	0x01796838	// this cua ham GOTO_FUNC_ADD (15-10)
#define GOTO_FUNC_ADD	0x00642730	// ham tu tim duong va di chuyen toi diem x/32,y/32 (tuyet doi) trong game (15-10)


// Code Injection (dang TAT - dia chi cua ban cu)
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

#define AMULET_PAPER		TV(L"Thổ địa phù")
#define AMULETX_PAPER		TV(L"Hồi thành phù")
#define KC_LENH				TV(L"Khiêu chiến lệnh")
#define FY_ITEM				TV(L"Phúc Duyên Lộ")
#define VISITED_PLACES		TV(L"Những nơi đã đi qua")
#define AGREE_TRADE			TV(L"Giao dịch")
#define FULL_REPAIR			TV(L"Sửa kỹ [")
