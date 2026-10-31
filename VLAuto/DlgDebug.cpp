// DlgDebug.cpp : implementation file
//
// Cửa sổ DEBUG: gửi lệnh WM_HOOK_DEBUG tới VLHook (trong Game) để chạy thử
// từng hàm game, VLHook trả kết quả về bằng WM_COPYDATA (DEBUG_REPLY).
// Thêm tab mới: thêm tên vào s_szTabs, lệnh vào DEBUG_CMD (GameDef.h),
// nút vào s_Buttons và phần xử lý trong VLHook/VLDebug.cpp.

#include "stdafx.h"
#include "../GameDef.h"
#include "VLAuto.h"
#include "GameCore.h"
#include "GameWindow.h"
#include "DlgDebug.h"
#include "GameLogin.h"
#include <tlhelp32.h>
#include <shlwapi.h>
#include <psapi.h>
#include <vector>
#pragma comment(lib, "psapi.lib")
#pragma comment(lib, "shlwapi.lib")

#ifndef PROCESS_QUERY_LIMITED_INFORMATION
#define PROCESS_QUERY_LIMITED_INFORMATION	0x1000
#endif
#ifndef TH32CS_SNAPMODULE32
#define TH32CS_SNAPMODULE32					0x00000010
#endif
#ifndef MSGFLT_ALLOW
#define MSGFLT_ALLOW						1
#endif

// Control tạo động
#define IDC_DBG_TARGET		2901
#define IDC_DBG_X_EDIT		2902
#define IDC_DBG_Y_EDIT		2903
#define IDC_DBG_IDX_EDIT	2904
#define IDC_DBG_STR_EDIT	2905
#define IDC_DBG_GETPOS		2906
#define IDC_DBG_GETFLAG		2907
#define IDC_DBG_CLEAR		2908
#define IDC_DBG_AUTOPOS		2909
#define IDC_DBG_CHECKHOOK	2910
#define IDC_DBG_CMD_FIRST	3000
#define IDC_DBG_CMD_LAST	(IDC_DBG_CMD_FIRST + CDlgDebug::MAX_BUTTONS - 1)

#define TIMER_CHECK			1		// Cập nhật nhân vật, kiểm tra lệnh không phản hồi
#define TIMER_READPOS		2		// Đọc lại vị trí sau MoveTo
#define REPLY_TIMEOUT		3000

#define CLR_CMD				RGB(0, 0, 192)
#define CLR_OK				RGB(0, 128, 0)
#define CLR_ERROR			RGB(208, 0, 0)
#define CLR_TEXT			RGB(0, 0, 0)
#define CLR_RAW				RGB(128, 0, 128)	// Dòng TCVN3 nguyên byte

enum DEBUG_TABS
{
	dtab_move,
	dtab_map,
	dtab_collect,
	dtab_path,
	dtab_attack,
	dtab_recover,
	dtab_login,
	dtab_lowcfg,
	dtab_quest,
	dtab_num,
};

static LPCTSTR s_szTabs[dtab_num] =
{
	_T("Di chuyển"),
	_T("Bản đồ"),
	_T("Nhặt đồ"),
	_T("Đường đi (MapPath)"),
	_T("Chiến đấu"),
	_T("Hồi phục"),
	_T("Đăng nhập"),
	_T("Giảm cấu hình"),
	_T("Nhiệm vụ"),
};

static const CDlgDebug::DEBUG_BUTTON s_Buttons[] =
{
	// Tab Di chuyển - các hàm game dùng trong VLHook/VLMove.cpp
	{ dtab_move, dbg_mv_status,			_T("Trạng thái + điều kiện Breathe"),
		_T("Đọc g_pCurPlayer, P_PLAYER, m_FightMode, theMove... và cho biết CVLMove::Breathe() đang dừng ở điều kiện nào"), 0 },
	{ dtab_move, dbg_mv_getmappos,		_T("P_PLAYER->GetMapPos"),
		_T("Đọc tọa độ nhân vật (và điền vào X/Y)"), 0 },
	{ dtab_move, dbg_mv_moveto,			_T("g_pCurPlayer->MoveTo(X, Y)"),
		_T("KPlayer::MoveTo: chỉ gửi lệnh khi m_nSendMoveFrames > 7, quãng đường tối đa 400"), 0 },
	{ dtab_move, dbg_mv_npcmoveto,		_T("P_PLAYER->MoveTo(X, Y)  [KNpc]"),
		_T("KNpc::MoveTo gọi thẳng MVT_FUNC_ADD, không qua giới hạn m_nSendMoveFrames"), 0 },
	{ dtab_move, dbg_mv_goto,			_T("g_pCurPlayer->GoTo(X, Y)"),
		_T("KPlayer::GoTo: game tự tìm đường tới (X/32, Y/32) qua GOTO_FUNC_ADD - đi được xa, vòng qua vật cản"), 0 },
	{ dtab_move, dbg_mv_movetonpc,		_T("g_pCurPlayer->MoveTo(Npc[Idx])"),
		_T("Đi tới điểm giữa nhân vật và NPC có chỉ số = ô Idx/Số"), 0 },
	{ dtab_move, dbg_mv_dosit,			_T("g_pCurPlayer->DoSit()"),
		_T("Ngồi xuống: ExecuteScript(\"Switch([[sit]])\")"), 0 },
	{ dtab_move, dbg_mv_horse,			_T("ExecuteScript(Lên/xuống ngựa)"),
		_T("ExecuteScript(SCK_SHORTCUT_HORSE) - dùng khi di chuyển/theo sau"), 0 },
	{ dtab_move, dbg_mv_gettaskval,		_T("g_pCurPlayer->GetTaskVal(Idx)"),
		_T("Đọc giá trị Task số = ô Idx/Số"), 0 },
	{ dtab_move, dbg_mv_npclist,		_T("g_pNpcArray: DS NPC (dZ <= Idx)"),
		_T("Liệt kê NPC đang tồn tại (IsExist/GetMapPos/Name). Ô Idx/Số = phạm vi, 0 = tất cả"), 0 },
	{ dtab_move, dbg_mv_findnpc,		_T("Tìm NPC theo tên (Chuỗi)"),
		_T("Như CVLMove::GetLeaderChar nhưng không tác động: tìm NPC/người chơi có tên = ô Chuỗi"), 0 },
	{ dtab_move, dbg_mv_setpeople,		_T("theAttack.SetPeopleIdx(Idx)"),
		_T("Gán PEOPLE_IDX = ô Idx/Số (game tự chạy theo đối tượng này - dùng khi Theo sau). 0 = bỏ"), 0 },
	{ dtab_move, dbg_mv_getflag,		_T("Đọc cắm cờ FLAG_X/Y_OFFST"),
		_T("Đọc tọa độ cắm cờ trên bản đồ nhỏ (dùng khi ghi tọa độ Theo tọa độ)"), 0 },
	{ dtab_move, dbg_mv_setflag,		_T("KUiMiniMap::SetMapFlag(X, Y)"),
		_T("Cắm cờ tại tọa độ X/Y (để trống = vị trí hiện tại)"), 0 },
	{ dtab_move, dbg_mv_savepoint,		_T("theMove.SavePoint()  [Ctrl+S]"),
		_T("Ghi tọa độ (cần bật 'Quanh điểm' hoặc 'Theo tọa độ' ở tab Di chuyển)"), 0 },
	{ dtab_move, dbg_mv_mapinfo,		_T("g_pMapID + file MAP / điểm"),
		_T("Map ID, tên map, file MAP trong MapMenu.ini, các điểm theMove đang nạp"), 0 },
	{ dtab_move, dbg_mv_team,			_T("g_pTeam (Tổ đội)"),
		_T("Thông tin tổ đội: trưởng nhóm m_szMemName[0] được dùng khi Theo sau"), 0 },
	{ dtab_move, dbg_mv_teamleave,		_T("TeamOperation(TEAM_LEAVE)"),
		_T("Rời tổ đội (dùng khi vào Phong Kỳ)"), TRUE },
	{ dtab_move, dbg_mv_msgsel2,		_T("KUiMsgSel2: hội thoại NPC"),
		_T("Đọc bảng hội thoại (Dịch Quan, ...): IsVisible, lời thoại, các lựa chọn"), 0 },
	{ dtab_move, dbg_mv_msgsel2click,	_T("KUiMsgSel2::OnClickMsg(Idx)"),
		_T("Chọn dòng số = ô Idx/Số trong bảng hội thoại đang mở"), 0 },
	{ dtab_move, dbg_mv_info2,			_T("g_UiInformation2->Hide()"),
		_T("Đọc và đóng bảng thông báo Information2"), 0 },
	{ dtab_move, dbg_mv_sysmsg,			_T("SystemMessage(Chuỗi)"),
		_T("Hiện ô Chuỗi trong khung thông báo hệ thống của game"), 0 },

	// Tab Bản đồ - các hàm game dùng trong VLHook/VLMap.cpp (về thành, mua bán, rương, xa phu)
	// Idx của NpcPoints: 0 rương, 1-4 xa phu, 5 tạp hóa, 6 hiệu thuốc, 7 VL truyền nhân, 8 thợ rèn, 9 dịch quan
	{ dtab_map, dbg_mp_status,			_T("Trạng thái + bước Breathe"),
		_T("Các cờ m_bNeed..., tùy chọn và CVLMap::Breathe() sẽ làm bước nào tiếp theo (hoặc dừng ở đâu)"), 0 },
	{ dtab_map, dbg_mp_showwalk,		_T("Vẽ đường đi (bật/tắt)"),
		_T("Vẽ trong game đường Auto đang đi: MapPath (xanh), NPC trong thành đang chạy tới (thoi vàng), lệnh MoveTo gần nhất (dấu X đỏ), danh sách tọa độ (xanh lá)"), 0 },
	{ dtab_map, dbg_mp_citymap,			_T("LoadMap + NpcPoints ([NPC])"),
		_T("Nạp lại mục [NPC] của Maps\\Path\\<MapID>_<Tên map>.txt, liệt kê tọa độ NPC trong thành và NearestDriver()"), 0 },
	{ dtab_map, dbg_mp_cityinfo,		_T("GetOwnerCity / GetCityName"),
		_T("Thành chính của map hiện tại, tên map (MapNames.ini), GetReturnCity của nơi luyện công"), 0 },
	{ dtab_map, dbg_mp_npcatpoint,		_T("NPC tại NpcPoints[Idx]"),
		_T("Các NPC quanh điểm NpcPoints[Idx] - ClickToNpc chọn NPC đối thoại cách điểm <= 40"), 0 },
	{ dtab_map, dbg_mp_clicknpc,		_T("ClickToNpc(NpcPoints[Idx])"),
		_T("Chạy tới / bấm vào NPC tại NpcPoints[Idx] (SetPeopleIdx)"), 0 },
	{ dtab_map, dbg_mp_runtopos,		_T("RunningToPos(NpcPoints[Idx])"),
		_T("Chạy theo MapPath của map (hết đường thì chạy thẳng, tự né vật cản) tới NpcPoints[Idx], chạy 1 bước - bấm tiếp để chạy tiếp"), 0 },
	{ dtab_map, dbg_mp_menu,			_T("Menu NPC: GetMenuCount + dòng"),
		_T("Đọc bảng hội thoại KUiMsgSel / KUiMsgSel2 đang mở và các lựa chọn"), 0 },
	{ dtab_map, dbg_mp_dialog,			_T("Lời thoại / bảng thông báo"),
		_T("Đọc đầy đủ lời NPC trong hộp thoại đang mở (vd: mô tả nhiệm vụ Dã Tẩu) và bảng thông báo KUiInformation"), 0 },
	{ dtab_map, dbg_mp_filelog,			_T("Ghi file/lua game nạp (bật/tắt)"),
		_T("Hook engine.dll (KPakFile::Open, KFile::Open, KLuaScript::Load/LoadBuffer/DoString/CallFunction) + CoreDataChanged của exe (id, tham số; id 64 = hộp thoại NPC) + task value thay đổi: bật rồi bấm vào NPC, tắt để xem file .lua/.ini/.txt, hàm lua, thông báo core và task đổi + địa chỉ gọi trong exe. X = 1: ghi cả file hình/âm thanh. Lưu Logs\\FileLog.txt"), 0 },
	{ dtab_map, dbg_mp_filelogview,		_T("Xem log file/lua"),
		_T("Xem log đang ghi (ô Chuỗi = lọc, vd: .lua). X = 1: xóa log sau khi xem. Đầy đủ ở Logs\\FileLog.txt"), 0 },
	{ dtab_map, dbg_mp_clickmenuidx,	_T("theMain.ClickMenuIdx(Idx)"),
		_T("Chọn dòng số Idx trong bảng hội thoại đang mở"), 0 },
	{ dtab_map, dbg_mp_clickmenustr,	_T("theMain.ClickMenuStr(Chuỗi)"),
		_T("Chọn dòng có chứa ô Chuỗi (vd: Những thành thị đã đi qua)"), 0 },
	{ dtab_map, dbg_mp_closeall,		_T("CloseAllWindows(0)"),
		_T("Đóng mọi cửa sổ game (dùng trong ResetShoping)"), 0 },
	{ dtab_map, dbg_mp_shop,			_T("KUiShop: danh sách hàng"),
		_T("Cửa hàng đang mở: uId từng món + GetGameData(GDI_TRADE_ITEM_PRICE)"), 0 },
	{ dtab_map, dbg_mp_buyitem,			_T("g_pCurPlayer->BuyItem(Idx)"),
		_T("Mua món có uId = Idx trong cửa hàng đang mở"), TRUE },
	{ dtab_map, dbg_mp_inventory,		_T("Hành trang: tiền, ô, vật phẩm"),
		_T("EQUIP_MONEY, ROOM_MONEY, CheckRoom(), danh sách vật phẩm (Idx dùng cho SellItem)"), 0 },
	{ dtab_map, dbg_mp_itemcount,		_T("GetItemsCount(Chuỗi)"),
		_T("Đếm vật phẩm có tên chứa ô Chuỗi (để trống = Thổ Địa Phù)"), 0 },
	{ dtab_map, dbg_mp_sellitem,		_T("g_pCurPlayer->SellItem(Idx)"),
		_T("Bán vật phẩm Idx (lấy ở nút Hành trang) - cần mở cửa hàng"), TRUE },
	{ dtab_map, dbg_mp_useitem,			_T("g_pCurPlayer->UseItem(Chuỗi)"),
		_T("Dùng vật phẩm có tên chứa ô Chuỗi (vd: Thần Hành Phù)"), TRUE },
	{ dtab_map, dbg_mp_eatmedicine,		_T("g_pCurPlayer->EatMedicine(Idx)"),
		_T("Uống thuốc: Idx 0 máu, 1 mana, 2 cả hai, 3 thể lực, 4 giải độc"), 0 },
	{ dtab_map, dbg_mp_equipdur,		_T("Độ bền trang bị"),
		_T("m_EquipItem: độ bền hiện tại/tối đa (GetRepairItems, CheckReady)"), 0 },
	{ dtab_map, dbg_mp_repairitem,		_T("g_pCurPlayer->RepairItem(Idx)"),
		_T("Sửa trang bị Idx (lấy ở nút Độ bền) - cần mở cửa hàng"), TRUE },
	{ dtab_map, dbg_mp_decompose,		_T("g_pCurPlayer->DecomposeItem(Idx)"),
		_T("Rã trang bị Idx trong hành trang (lấy ở nút Hành trang) thành Mảnh Trang Bị - OperationRequest(0x9B)"), TRUE },
	{ dtab_map, dbg_mp_storebox,		_T("Rương: SBO + vị trí rương"),
		_T("KUiStoreBox có mở không, rương (KObj StoreBox / NPC Rương chứa đồ) ở đâu"), 0 },
	{ dtab_map, dbg_mp_openstore,		_T("theMap.OpenStoreBox()"),
		_T("Chạy tới và mở rương (bấm nhiều lần)"), 0 },
	{ dtab_map, dbg_mp_lockinfo,		_T("Khóa rương: ProcessGameSecurity"),
		_T("Có chức năng khóa không, rương đang khóa không"), 0 },
	{ dtab_map, dbg_mp_unlock,			_T("OpenLockRoom (mật khẩu đã cài)"),
		_T("Mở khóa rương bằng mật khẩu đã cài ở tab Bản đồ"), 0 },
	{ dtab_map, dbg_mp_lock,			_T("theMain.CloseLockRoom()"),
		_T("Khóa rương lại"), 0 },
	{ dtab_map, dbg_mp_moneyin,			_T("Gửi Idx lượng vào rương"),
		_T("OperationRequest(GOI_MONEY_INOUT_STORE_BOX, TRUE, Idx) - cần mở rương"), TRUE },
	{ dtab_map, dbg_mp_moneyout,		_T("Rút Idx lượng từ rương"),
		_T("OperationRequest(GOI_MONEY_INOUT_STORE_BOX, 0, Idx) - cần mở rương"), TRUE },
	{ dtab_map, dbg_mp_tongmoney,		_T("Góp Idx lượng vào quỹ bang"),
		_T("TongOperation(4, {2, 0, Idx}, buf) - như nút Góp quỹ (15_10: lệnh con 2 -> 0x5BDDA0, this 0x17A4F00). X khác 0 = thử lệnh con X (0..12). Không cần mở cửa sổ bang"), TRUE },
	{ dtab_map, dbg_mp_handtoroom,		_T("Đồ trên tay -> hành trang"),
		_T("EQUIP_ROOM.FindRoom + SwitchItem(UOC_ITEM_TAKE_WITH)"), 0 },
	{ dtab_map, dbg_mp_checkready,		_T("CheckReady (chỉ đọc)"),
		_T("Các điều kiện trước khi đi xa phu: tiền, độ bền, trúng độc, TĐP, máu/mana"), 0 },
	{ dtab_map, dbg_mp_script,			_T("ExecuteScript(Chuỗi)"),
		_T("Chạy script game, vd: Switch('PK',0)  hoặc  Switch([[horse]])"), 0 },
	{ dtab_map, dbg_mp_saverepo,		_T("SaveNewRepository  [Ctrl+L]"),
		_T("Ghi tọa độ rương mới vào mục [NPC] của Maps\\Path\\<MapID>_<Tên map>.txt (đứng gần rương)"), TRUE },

	// Tab Nhặt đồ - các hàm game dùng trong VLHook/VLCollect.cpp (nhặt Obj, lọc, sắp xếp hành trang)
	// Idx = số KObj (nút Danh sách Obj) hoặc Idx vật phẩm (nút KeepTheItem)
	{ dtab_collect, dbg_cl_status,		_T("Trạng thái + bước Breathe"),
		_T("Tùy chọn nhặt đồ đã nhận từ VLAuto và CVLCollect::Breathe() đang làm gì / dừng ở đâu"), 0 },
	{ dtab_collect, dbg_cl_objlist,		_T("Danh sách Obj + CheckObj"),
		_T("Mọi KObj quanh nhân vật (g_pObjArray): loại, DataID, màu, kích thước, khoảng cách và lý do nhặt / bỏ qua"), 0 },
	{ dtab_collect, dbg_cl_checkobj,	_T("CheckObj(KObj[Idx])"),
		_T("Chi tiết KObj số Idx và kết quả theCollect.CheckObj"), 0 },
	{ dtab_collect, dbg_cl_pickup,		_T("g_pCurPlayer->PickUpObj(Idx)"),
		_T("Nhặt KObj số Idx (cần đứng gần < 128)"), 0 },
	{ dtab_collect, dbg_cl_movetoobj,	_T("MoveTo(KObj[Idx])"),
		_T("Chạy tới vị trí KObj số Idx"), 0 },
	{ dtab_collect, dbg_cl_searchobj,	_T("theCollect.SearchObj() 1 lần"),
		_T("Chạy 1 lần SearchObj: nhặt Obj gần nhất hoặc chạy tới. Idx > 0 = tầm tìm, 0 = tầm đã cài"), 0 },
	{ dtab_collect, dbg_cl_blacklist,	_T("LoadBlackList / IsInBlackList(Chuỗi)"),
		_T("Nạp lại Maps\\BlackItems.txt, liệt kê, và thử ô Chuỗi có bị chặn không"), 0 },
	{ dtab_collect, dbg_cl_redlist,		_T("LoadRedList / IsInRedList(Chuỗi)"),
		_T("Nạp lại Maps\\RedItems.txt, liệt kê, và thử ô Chuỗi với từng hệ"), 0 },
	{ dtab_collect, dbg_cl_keepitems,	_T("KeepTheItem: cả hành trang"),
		_T("Mỗi vật phẩm: lý do giữ khi lọc nhặt và khi bán (keep_nothing = sẽ vứt / bán)"), 0 },
	{ dtab_collect, dbg_cl_itemattrib,	_T("Thuộc tính KItem[Idx]"),
		_T("CommonAttrib, m_aryAllAttrib, m_MagicLevel, CheckItemAttrib của vật phẩm Idx"), 0 },
	{ dtab_collect, dbg_cl_room,		_T("Lưới hành trang + FindRoom"),
		_T("EQUIP_ROOM m_pArray từng ô, CheckRoom, FindRoom cho các kích thước 1x1..2x4"), 0 },
	{ dtab_collect, dbg_cl_handitem,	_T("Đồ trên tay (chỉ đọc)"),
		_T("PLAY_HAND_IDX, KeepTheItem và CheckHandItem sẽ làm gì với nó"), 0 },
	{ dtab_collect, dbg_cl_takeitem,	_T("SwitchItem: cầm vật phẩm Idx"),
		_T("SwitchItem(UOC_ITEM_TAKE_WITH, x, y) cầm vật phẩm Idx trong hành trang lên tay"), 0 },
	{ dtab_collect, dbg_cl_checkhand,	_T("theCollect.CheckHandItem()"),
		_T("Xử lý đồ trên tay như Breathe: vứt (khi lọc) hoặc cất vào hành trang"), TRUE },
	{ dtab_collect, dbg_cl_filter,		_T("theCollect.FilterItems()"),
		_T("Cầm lên tay món đầu tiên không cần giữ (lần sau CheckHandItem sẽ vứt)"), TRUE },
	{ dtab_collect, dbg_cl_arrange,		_T("theCollect.ArrangeItems()"),
		_T("Sắp xếp 1 bước: cầm 1 món lên tay để dời chỗ (rồi CheckHandItem 2 lần)"), 0 },
	{ dtab_collect, dbg_cl_throwaway,	_T("ThrowAwayItem() đồ trên tay"),
		_T("ExecuteScript(\"ThrowAwayItem()\") - vứt đồ đang cầm xuống đất"), TRUE },
	{ dtab_collect, dbg_cl_badobjs,		_T("g_BadObjs"),
		_T("Các vật phẩm đã nhặt (không nhặt lại khi bật lọc)"), 0 },
	{ dtab_collect, dbg_cl_decomposescan,	_T("Quét đồ có thể rã"),
		_T("Mỗi trang bị trong hành trang: giữ / có thể rã / không rã được và vì sao; FilterItems sẽ rã món nào; Breathe có tới FilterItems không. Idx = 1: xóa danh sách rã lỗi"), 0 },
	{ dtab_collect, dbg_cl_decompose,	_T("Rã Idx (DecomposeItem)"),
		_T("Rã trang bị Idx (lấy ở nút Quét đồ có thể rã) thành Mảnh trang bị"), TRUE },

	// Tab MapPath - ghi/kiểm tra dữ liệu đường đi (VLHook/VLDebugPath.cpp), file Maps\\Path\\<MapID>_<Tên map>.txt
	{ dtab_path, dbg_pt_status,			_T("Trạng thái + đồ thị map"),
		_T("Map hiện tại, file Maps\\Path\\<MapID>_<Tên map>.txt, số điểm/cạnh/cổng/xa phu, điểm gần nhân vật nhất"), 0 },
	{ dtab_path, dbg_pt_show,			_T("BẬT/TẮT vẽ node + phím tắt"),
		_T("Vẽ điểm/đường nối trong game (số = chỉ số điểm). Phím: Space thêm điểm tại vị trí hiện tại, Delete xóa điểm gần nhất (≤ 500, màu trắng), End chọn điểm gần nhất để rẽ nhánh"), 0 },
	{ dtab_path, dbg_pt_rec,				_T("BẬT/TẮT tự ghi (Idx = k.cách)"),
		_T("Bật: chạy bộ theo đường, cứ Idx đơn vị (0 = 400) thêm 1 điểm nối điểm trước; chạy qua điểm cũ thì tự nối (rẽ nhánh). Qua cổng / đi xa phu tự ghi. Tắt: tự ghi file"), 0 },
	{ dtab_path, dbg_pt_addpoint,		_T("Thêm điểm tại vị trí hiện tại"),
		_T("Thêm 1 điểm tại vị trí nhân vật, nối với điểm trước"), 0 },
	{ dtab_path, dbg_pt_undo,			_T("Xóa điểm vừa ghi (cuối)"),
		_T("Xóa điểm có chỉ số lớn nhất"), 0 },
	{ dtab_path, dbg_pt_delnear,			_T("Xóa điểm gần nhất"),
		_T("Xóa điểm gần nhân vật nhất cùng cạnh/cổng/xa phu của nó (chỉ số các điểm sau giảm 1)"), 0 },
	{ dtab_path, dbg_pt_setlast,			_T("Chọn điểm gần nhất để rẽ nhánh"),
		_T("Điểm ghi tiếp theo sẽ nối vào điểm gần nhân vật nhất"), 0 },
	{ dtab_path, dbg_pt_link,			_T("Nối điểm gần nhất ↔ điểm Idx"),
		_T("Thêm cạnh giữa điểm gần nhân vật nhất và điểm Idx"), 0 },
	{ dtab_path, dbg_pt_unlink,			_T("Bỏ nối điểm gần nhất ↔ điểm Idx"),
		_T("Xóa cạnh giữa điểm gần nhân vật nhất và điểm Idx"), 0 },
	{ dtab_path, dbg_pt_door,			_T("Đánh dấu cổng → map Idx"),
		_T("Vị trí hiện tại (điểm gần trong 150) là cổng sang map có ID = ô Idx/Số"), 0 },
	{ dtab_path, dbg_pt_scan,			_T("Quét cổng gần (Idx = phạm vi, 0 = 500)"),
		_T("Liệt kê trap bản đồ (đi lên là chuyển map - từ Region_C.dat), Obj (click) và NPC (đối thoại) quanh nhân vật. Khi bật vẽ node, ô trap vẽ màu cam"), 0 },
	{ dtab_path, dbg_pt_trydoor,		_T("Thử đi vào cổng (X/Y, trống = cờ)"),
		_T("Chạy tới tọa độ tuyệt đối X/Y (tọa độ thật như P_PLAYER->GetMapPos; để trống = cờ cắm trên bản đồ nhỏ). Chuyển map => ghi cổng tại đó + cổng chiều về. Tới nơi 3s không chuyển map = không phải cổng. Bấm lại để dừng"), 0 },
	{ dtab_path, dbg_pt_runxy,			_T("Chạy thử tới điểm (X,Y)"),
		_T("Chạy tới tọa độ X/Y (tọa độ game x/y như trên màn hình hoặc tọa độ thật; để trống = cờ cắm trên bản đồ nhỏ): theo đồ thị MapPath tới điểm gần X/Y nhất rồi đi thẳng. Báo nếu X/Y là ô vật cản. Bấm lại để dừng"), 0 },
	{ dtab_path, dbg_pt_doorxy,			_T("Ghi Cổng (X,Y) qua map Idx"),
		_T("Thêm điểm tại tọa độ X/Y (để trống = cờ cắm trên bản đồ nhỏ), nối điểm trước, đánh dấu cổng đi bộ sang map có ID = ô Idx/Số (0 = chưa biết, tự điền khi đi qua lúc bật tự ghi)"), 0 },
	{ dtab_path, dbg_pt_obstacle,		_T("Bật / tắt hiển thị khu vực không đi lên được"),
		_T("Vẽ trong game ô vật cản (KScenePlaceRegionC::m_ObstacleInfo nạp từ file region của map, ô 32x32 cùng lưới ô trap): đỏ = vật cản thường, xanh = bay/nhảy qua được; ô tam giác vẽ nửa ô; đường xanh lá + dấu X vàng = đường né vật cản Auto vừa tính. In số ô vật cản các region đang nạp và ô dưới chân nhân vật"), 0 },
	{ dtab_path, dbg_pt_obsavoid,		_T("Bật / tắt né vật cản khi di chuyển"),
		_T("Mặc định BẬT: mọi lệnh MoveTo của Auto (điểm train, MapPath, qua map, nhặt đồ, đánh quái...) nếu đường thẳng cắt vật cản thì tìm đường vòng (A* trên lưới vật cản) và chạy tới điểm trung gian; chọn điểm train / MapPath gần nhất theo đường đi bộ. Bấm lần đầu = BẬT né + vẽ lưới sao trong game (sao đỏ = không đi được, sao vàng = đi được, sao xanh lá = tuyến Auto đang chạy, X trắng = điểm đang chạy tới); bấm lại = TẮT né và tắt vẽ. Có X/Y: không bật/tắt mà in thử đường tới X/Y (để trống = cờ)"), 0 },
	{ dtab_path, dbg_pt_doortrap,		_T("Ghi cổng = trap gần nhất → map Idx"),
		_T("Thêm điểm tại ô trap gần nhất (≤ 500), nối điểm trước, đánh dấu cổng đi bộ sang map Idx (0 = chưa biết, tự điền khi đi qua lúc bật tự ghi)"), 0 },
	{ dtab_path, dbg_pt_doorobj,		_T("Ghi cổng = Obj gần nhất → map Idx"),
		_T("Thêm điểm tại Obj gần nhất (≤ 500, click để qua map), lưu tên Obj. Idx = MapID đích (0 = chưa biết)"), 0 },
	{ dtab_path, dbg_pt_doornpc,		_T("Ghi cổng = NPC gần nhất → map Idx"),
		_T("Thêm điểm tại NPC đối thoại gần nhất (≤ 500), lưu tên NPC. Idx = MapID đích (0 = chưa biết)"), 0 },
	{ dtab_path, dbg_pt_driver,			_T("Ghi xa phu (menu đang mở)"),
		_T("Đứng cạnh xa phu, mở hội thoại rồi bấm: các dòng có tên map (MapNames.ini) được ghi. Mở menu con rồi bấm lại để ghi thêm"), 0 },
	{ dtab_path, dbg_pt_undotrans,		_T("Hủy cổng/xa phu vừa tự ghi"),
		_T("Xóa cổng/xa phu ghi tự động ở lần đổi map gần nhất (vd dùng Thần Hành Phù, chết về thành)"), 0 },
	{ dtab_path, dbg_pt_list,			_T("In đồ thị"),
		_T("Liệt kê điểm, các điểm nối, cổng và xa phu"), 0 },
	{ dtab_path, dbg_pt_save,			_T("Ghi file"),
		_T("Ghi Maps\\Path\\<MapID>_<Tên map>.txt (đổi map / tắt tự ghi cũng tự ghi)"), 0 },
	{ dtab_path, dbg_pt_load,			_T("Đọc lại file (bỏ thay đổi)"),
		_T("Đọc lại Maps\\Path\\<MapID>_<Tên map>.txt, bỏ các thay đổi chưa ghi"), 0 },
	{ dtab_path, dbg_pt_import,			_T("Nhập file cũ (.map/.pth)"),
		_T("Thêm điểm/cạnh từ file MAP (MapMenu.ini) và Paths (PathList.ini) vào đồ thị"), 0 },
	{ dtab_path, dbg_pt_clear,			_T("Xóa đồ thị map hiện tại"),
		_T("Xóa hết điểm/cạnh/cổng/xa phu của map hiện tại (file bị ghi đè khi Ghi file / đổi map)"), TRUE },
	{ dtab_path, dbg_pt_route,			_T("Tìm đường → điểm Idx"),
		_T("Dijkstra từ điểm gần nhân vật nhất tới điểm Idx, in các điểm đi qua"), 0 },
	{ dtab_path, dbg_pt_run,				_T("Chạy thử → điểm Idx"),
		_T("Chạy theo đường tìm được tới điểm Idx (nên tắt Auto đánh/di chuyển khi thử)"), 0 },
	{ dtab_path, dbg_pt_stop,			_T("Dừng chạy thử"),
		_T("Dừng chạy thử"), 0 },
	{ dtab_path, dbg_pt_maproute,		_T("Lộ trình giữa các map → map Idx"),
		_T("Đọc cổng/xa phu trong mọi file Maps\\Path\\*.txt, tìm lộ trình từ map hiện tại tới map ID = ô Idx/Số"), 0 },

	// Tab Chiến đấu - các hàm game dùng trong VLHook/VLAttack.cpp (đánh quái, kỹ năng, tự vệ)
	// Idx = số Npc (nút Danh sách quái) hoặc SkillID (nút Danh sách kỹ năng)
	{ dtab_attack, dbg_at_status,		_T("Trạng thái + bước Breathe"),
		_T("Tùy chọn đánh đã nhận từ VLAuto, kỹ năng cài đặt, biến nội bộ và CVLAttack::Breathe() đang dừng ở đâu"), 0 },
	{ dtab_attack, dbg_at_skills,		_T("Danh sách kỹ năng"),
		_T("P_PLAYER->m_Skills: SkillID, cấp, tên KSkill, tầm đánh, giới hạn ngựa; đánh dấu kỹ năng đang cài"), 0 },
	{ dtab_attack, dbg_at_skillinfo,	_T("KSkill[Idx]"),
		_T("Đọc KSkill tại SKL_BASE_ADD theo SkillID = Idx (kiểm tra offset SKL_DATA_SIZE)"), 0 },
	{ dtab_attack, dbg_at_states,		_T("Trạng thái trên người + buff"),
		_T("KStateNode (m_pHeadNode): SkillID, LeftTime; HandleDirectSkills sẽ buff lại supp01..03 nào"), 0 },
	{ dtab_attack, dbg_at_npclist,		_T("Danh sách quái + lọc"),
		_T("Quái quanh nhân vật (dZ <= Idx, 0 = 2 x tầm tìm): Life, BOSS, quan hệ và lý do RefreshNpcs bỏ qua"), 0 },
	{ dtab_attack, dbg_at_search,		_T("SearchNearestNpc() 1 lần"),
		_T("RefreshNpcs + SearchNearestNpc: chọn quái gần nhất và MoveTo tới nó. Idx > 0 = tầm tìm, 0 = tầm đã cài"), 0 },
	{ dtab_attack, dbg_at_attacknpc,	_T("Đánh Npc[Idx]"),
		_T("SetActiveSkill(kỹ năng trái) + SetPeopleIdx(Idx) - game tự chạy tới và đánh"), 0 },
	{ dtab_attack, dbg_at_islag,		_T("IsLagTarget(mục tiêu)"),
		_T("Gọi 1 nhịp kiểm tra quái lag của mục tiêu hiện tại (PEOPLE_IDX), in m_BadNpc"), 0 },
	{ dtab_attack, dbg_at_badnpc,		_T("m_BadNpc (Idx = 1: xóa)"),
		_T("3 quái bị đánh dấu lag (không đánh lại trong ~18s). Idx = 1 để xóa"), 0 },
	{ dtab_attack, dbg_at_setdefskills,	_T("SetDefSkills(X, Y)"),
		_T("Đặt kỹ năng tay trái = X, tay phải = Y (0 = theo cài đặt) qua GOI_SET_IMMDIA_SKILL"), 0 },
	{ dtab_attack, dbg_at_setactive,	_T("SetActiveSkill(Idx)"),
		_T("m_ActiveSkillID = Idx (0 = kỹ năng trái) và chỉnh m_CurrentAttackRadius theo tùy chọn đánh gần"), 0 },
	{ dtab_attack, dbg_at_castself,		_T("Dùng kỹ năng Idx lên mình"),
		_T("SendCommand(do_skill, Idx, -1, 1) + SendVLCmdSkill(Idx, -1, ID mình) - như buff supp01..03"), 0 },
	{ dtab_attack, dbg_at_casttarget,	_T("Dùng kỹ năng Idx lên Npc[X]"),
		_T("SendVLCmdSkill(Idx, -1, ID của Npc[X]); X = 0 = mục tiêu PEOPLE_IDX"), 0 },
	{ dtab_attack, dbg_at_shortcut,		_T("DoSpecialSkill(Idx, X)"),
		_T("ExecuteScript ShortcutSkill(Idx), X = 1: DirectShortcutSkill(Idx) - ô phím tắt kỹ năng (kỹ năng luân phiên)"), 0 },
	{ dtab_attack, dbg_at_players,		_T("Người chơi quanh + tự vệ"),
		_T("Quan hệ, phe, cờ PK, ngũ hành khắc; AutoDefence sẽ coi ai là kẻ thù"), 0 },
	{ dtab_attack, dbg_at_autodefence,	_T("theAttack.AutoDefence()"),
		_T("Chạy AutoDefence 1 lần: có thể đánh người, bật PK hoặc dùng AM"), TRUE },
	{ dtab_attack, dbg_at_pk,			_T("Switch('PK', Idx)"),
		_T("Đổi trạng thái PK: 0 luyện công, 1 chiến đấu, 2 đồ sát"), TRUE },
	{ dtab_attack, dbg_at_autohit,		_T("Bật/tắt tự đánh (Ctrl+A)"),
		_T("Đảo theAttack.m_bAutoHit + SetAutoHit() trong game (VLAuto không biết, gửi lại tùy chọn sẽ ghi đè)"), 0 },
	{ dtab_attack, dbg_at_watchtarget,	_T("Kiểm tra đối tượng đang đánh (bật/tắt)"),
		_T("Theo dõi mục tiêu CVLAttack::Breathe() đang đánh (PEOPLE_IDX): tên, Idx, tọa độ, khoảng cách, Res (1 = game còn vẽ NPC đó qua KNpc::Paint, 0 = không vẽ, ? = game đang không vẽ) hiện trên bảng chat - chỉ báo khi nội dung thay đổi"), 0 },

	// Tab Hồi phục - các hàm game dùng trong VLHook/VLRecover.cpp (ăn thuốc, sửa đồ, về thành, buff Nga Mi, X2)
	{ dtab_recover, dbg_rc_status,		_T("Trạng thái + bước Breathe"),
		_T("Tùy chọn hồi phục đã nhận từ VLAuto, bộ đếm, máu/mana và CVLRecover::Breathe() sẽ làm gì"), 0 },
	{ dtab_recover, dbg_rc_medicine,	_T("Thuốc trong hành trang"),
		_T("Mọi bình thuốc EatMedicine nhận ra (máu, mana, cả hai, thể lực, giải độc) và bình sẽ được dùng"), 0 },
	{ dtab_recover, dbg_rc_eat,			_T("EatMedicine(Idx)"),
		_T("Uống thuốc: Idx 0 máu, 1 mana, 2 cả hai, 3 thể lực, 4 giải độc"), 0 },
	{ dtab_recover, dbg_rc_equipdur,	_T("Độ bền trang bị"),
		_T("m_EquipItem: độ bền, +N, GetRepairPrice; CheckEquip sẽ tự sửa / về thành / tạm dừng"), 0 },
	{ dtab_recover, dbg_rc_gshop,		_T("KUiPlayerBar::OnClickGShop"),
		_T("Bấm nút thợ rèn trên thanh nhân vật (mở menu sửa đồ như Tự sửa đồ)"), 0 },
	{ dtab_recover, dbg_rc_msgsel,		_T("Menu sửa đồ (KUiMsgSel)"),
		_T("Nội dung KUiMsgSel và giá đọc được từ dòng \"Sửa kỹ [\" như CheckEquip"), 0 },
	{ dtab_recover, dbg_rc_msgclick,	_T("KUiMsgSel::OnClickMsg(Idx)"),
		_T("Chọn dòng Idx của menu KUiMsgSel đang mở"), 0 },
	{ dtab_recover, dbg_rc_repair,		_T("RepairItem(Idx)"),
		_T("Sửa trang bị Idx (lấy ở nút Độ bền trang bị) - cần đang mở cửa hàng thợ rèn, mất tiền"), TRUE },
	{ dtab_recover, dbg_rc_amcheck,		_T("Về thành: OnUseAM sẽ làm gì"),
		_T("Số Thổ địa phù / Hồi thành phù, ô phím tắt 3, map đặc biệt; CheckInventory (tiền, chỗ trống)"), 0 },
	{ dtab_recover, dbg_rc_useam,		_T("theRecover.OnUseAM()"),
		_T("Về thành ngay như Auto (dùng phù; không còn phù thì THOÁT GAME)"), TRUE },
	{ dtab_recover, dbg_rc_ngami,		_T("Buff Nga Mi (DoNgaMiBuff)"),
		_T("Kỹ năng NGAMY_BUFF_SKILLID, máu bản thân và thành viên đội trong tầm buff"), 0 },
	{ dtab_recover, dbg_rc_x2,			_T("X2 (TTL) + Sư đồ thiệp"),
		_T("LoadX2List (Maps\\X2Items.txt), số lượng trong hành trang, trạng thái X2 đang có; OnUseTTL / OnUseSDT sẽ làm gì"), 0 },
	{ dtab_recover, dbg_rc_usex2,		_T("UseItem(Chuỗi) - X2"),
		_T("Dùng vật phẩm tên = Chuỗi (trống = món đầu tiên trong X2Items)"), TRUE },
	{ dtab_recover, dbg_rc_usesdt,		_T("UseItem(Sư đồ thiệp)"),
		_T("Dùng Sư đồ thiệp như OnUseSDT"), TRUE },

	// Tab Giảm cấu hình - VLHook/VLDebugLowCfg.cpp (thử trước khi đưa vào tab Cơ bản)
	{ dtab_lowcfg, dbg_lc_status,		_T("Trạng thái"),
		_T("FPS vẽ, tắt hình, số người chơi hiển thị, thời tiết, bóng động, âm lượng, RAM (Working set / Commit)"), 0 },
	{ dtab_lowcfg, dbg_lc_trim,			_T("Giảm RAM ngay"),
		_T("SetProcessWorkingSetSize(-1, -1): đẩy bộ nhớ game ra standby/pagefile. Task Manager giảm mạnh, game nạp lại dần trang đang dùng"), 0 },
	{ dtab_lowcfg, dbg_lc_autotrim,		_T("Tự giảm RAM mỗi Idx giây"),
		_T("Idx = số giây giữa 2 lần giảm RAM (0 = tắt). Chạy cả khi tạm dừng Auto"), 0 },
	{ dtab_lowcfg, dbg_lc_fps,			_T("FPS vẽ = Idx"),
		_T("Ghi [0x7AD7B0] PaintFps = Idx (Idx = 0: trả lại giá trị gốc từ config.ini PaintFps). Logic game không đổi"), 0 },
	{ dtab_lowcfg, dbg_lc_screen,		_T("Tắt hình (Idx = 1/0)"),
		_T("Idx = 1: game ngừng vẽ (như khi thu nhỏ > 10s, ghi [0x8483B0] liên tục), Idx = 0: vẽ lại"), 0 },
	{ dtab_lowcfg, dbg_lc_players,		_T("Người chơi hiển thị = Idx"),
		_T("OperationRequest(GOI_OPTION_SETTING, 6, Idx): Idx = 0 ẩn người chơi khác, mặc định 200"), 0 },
	{ dtab_lowcfg, dbg_lc_weather,		_T("Thời tiết (Idx = 1/0)"),
		_T("OperationRequest(GOI_OPTION_SETTING, OPTION_WEATHER, Idx)"), 0 },
	{ dtab_lowcfg, dbg_lc_dynalight,	_T("Bóng động (Idx = 1/0)"),
		_T("OperationRequest(GOI_OPTION_SETTING, OPTION_DYNALIGHT, Idx)"), 0 },
	{ dtab_lowcfg, dbg_lc_sound,		_T("Âm lượng = Idx (0..100)"),
		_T("Nhạc + tiếng: OperationRequest(GOI_OPTION_SETTING, 2 và 3, Idx). 0 = tắt"), 0 },
	{ dtab_lowcfg, dbg_lc_lowall,		_T("Áp dụng tất cả (thấp nhất)"),
		_T("FPS = Idx (0 = 5), ẩn người chơi, tắt thời tiết + bóng động + âm thanh, tự giảm RAM mỗi 60s và giảm ngay. Không tắt hình"), 0 },
	{ dtab_lowcfg, dbg_lc_restore,		_T("Trả lại như cũ"),
		_T("FPS gốc, vẽ lại, người chơi / thời tiết / bóng động như trước khi đổi, âm lượng 100 (nếu đã đổi), tắt tự giảm RAM"), 0 },

	// Tab Đăng nhập - màn hình bắt đầu (KUiInit) và cửa sổ đăng nhập.
	// Game chưa nạp VLHook (chưa vào game) => VLAuto tự đọc bộ nhớ + CreateRemoteThread (RunLoginCmd)
	{ dtab_login, dbg_lg_status,		_T("Trạng thái đăng nhập"),
		_T("Đang trong game hay chưa; màn hình bắt đầu (0x7C47D8) + 4 nút; bảng Nhắc nhở (0x7BE5E8); chọn máy chủ (0x7BE604) + danh sách máy chủ; cửa sổ nhập tài khoản (0x7BE5EC)"), 0 },
	{ dtab_login, dbg_lg_serverlist,	_T("Đọc danh sách server (file)"),
		_T("Đọc \\settings\\serverlist.ini trong các file pak của Game theo thứ tự package.ini (pak đầu tiên có file được dùng, giải nén UCL), xuất UTF-8 ra UiConfig\\serverlist_.txt cho Auto Login. Chuỗi (tùy chọn) = thư mục Game, file .pak hoặc file .ini"), 0 },
	{ dtab_login, dbg_lg_entergame,		_T("Bấm Bắt Đầu Trò Chơi"),
		_T("KUiInit::EnterGame() = OnClickButton(this + 0x5A4): mở cửa sổ đăng nhập, đóng màn hình bắt đầu"), 0 },
	{ dtab_login, dbg_lg_notice,		_T("Nhắc nhở: Xác định"),
		_T("Bảng \"Nhắc nhở\" (KUiHealthNotice *0x7BE5E8, mở sau Bắt Đầu Trò Chơi): WndProc 0x483B10(0x565, nút Xác định +0x37C) => mở chọn máy chủ + đóng bảng"), 0 },
	{ dtab_login, dbg_lg_click,			_T("Bấm nút(Idx)"),
		_T("KUiInit::OnClickButton: Idx 0 Bắt Đầu, 1 Tùy Chọn, 2 Xem Ghi Hình, 3 Thoát Khỏi Trò Chơi (thoát game!)"), TRUE },
	{ dtab_login, dbg_lg_region,		_T("Chọn khu vực(Idx)"),
		_T("Chọn khu vực thứ Idx (xem Trạng thái): Left/RightList SetCurSel + nạp lại danh sách máy chủ 0x487160"), 0 },
	{ dtab_login, dbg_lg_server,		_T("Chọn máy chủ(Idx)"),
		_T("Chọn máy chủ thứ Idx trong khu vực đang chọn (xem Trạng thái): IpList SetCurSel"), 0 },
	{ dtab_login, dbg_lg_selserver,		_T("Chọn máy chủ: Đăng nhập"),
		_T("KUiSelServer::OnLogin() 0x487D00 với máy chủ đang chọn (xem danh sách ở Trạng thái): kết nối + mở cửa sổ nhập tài khoản"), 0 },
	{ dtab_login, dbg_lg_role,			_T("Chọn nhân vật(Idx)"),
		_T("KUiSelPlayer::OnSelectPlayer 0x49FEE0: Idx = số thứ tự trong danh sách nhân vật (xem Trạng thái), phải đang hiện trên trang"), 0 },
	{ dtab_login, dbg_lg_entergame2,	_T("Vào game"),
		_T("KUiSelPlayer: tick đồng ý (+0x8464) + OnEnterGame 0x4A0370: g_LoginLogic.SelectRole(nhân vật đang chọn) + cửa sổ đang vào game"), 0 },
	{ dtab_login, dbg_lg_login,			_T("Nhập TK/MK + Đăng nhập (Chuỗi)"),
		_T("Chuỗi = \"TàiKhoản MậtKhẩu\": KWndEdit::SetText 2 ô, KUiLogin::OnLogin() 0x484C20"), 0 },

	// Tab Nhiệm vụ - VLHook/VLDebugQuest.cpp: Dã Tẩu bản CTC (server điều khiển, nội dung nhiệm vụ chỉ có trong chữ
	// hội thoại NPC / thông báo). 3 bước: bấm = kiểm tra + chạy tự động (xong tự sang bước sau tới hết Bước 3), bấm lại = dừng.
	{ dtab_quest, dbg_qt_status,		_T("Trạng thái nhiệm vụ"),
		_T("Bước đang chạy, nhật ký 3 bước, nhiệm vụ đã phân tích (quái, map, tiến độ n / N), task 1027, MapPath tự đi, hội thoại Dã Tẩu chưa biết (nếu có)"), 0 },
	{ dtab_quest, dbg_qt_step1,		_T("Bước 1: Nhận nhiệm vụ (Idx)"),
		_T("Về Tương Dương (Thổ địa phù / MapPath), bấm NPC Dã Tẩu, đọc hội thoại: có nhiệm vụ => phân tích Tiêu diệt N <quái> ở <map>; chưa có => chọn Nhận nhiệm vụ. Nhiệm vụ đánh quái => tự sang Bước 2, rồi Bước 3 (chọn ô phần thưởng Idx: 0 Kinh nghiệm, 1 Ngân lượng, 2 Ngẫu nhiên). X = 1: chỉ kiểm tra. X = 2: chỉ làm Bước 1. Bấm lại: dừng"), 0 },
	{ dtab_quest, dbg_qt_step2,		_T("Bước 2: Diệt quái nhiệm vụ (Idx)"),
		_T("DoQuestKillNpc(số lượng, map, tên quái) theo nhiệm vụ Bước 1: trong thành thì bán đồ, rút tiền, mua máu / mana / Thổ địa phù (tab Bản đồ) => MapPath tự đi tới map => đánh quái: Chiến đấu CHỈ nhận quái tên nhiệm vụ, không thấy quái thì đi vòng các điểm MapPath của map (bỏ điểm gần cổng, chặn mọi lệnh đi khác trừ Chiến đấu / Nhặt đồ), tiến độ n / N từ kênh Công bố. Auto tạm dừng vẫn đánh được. Đủ quái => tự sang Bước 3 (ô Idx). X = 1: chỉ kiểm tra. X = 2: không tự sang Bước 3. Bấm lại: dừng"), 0 },
	{ dtab_quest, dbg_qt_step3,		_T("Bước 3: Trả nhiệm vụ (Idx)"),
		_T("Thổ địa phù về thành, MapPath về Tương Dương, bấm Dã Tẩu, chọn Hoàn thành nhiệm vụ, bảng phần thưởng chọn ô Idx: 0 trái = Kinh nghiệm, 1 giữa = Ngân lượng, 2 phải = Ngẫu nhiên. X = 1: chỉ kiểm tra. Bấm lại: dừng"), 0 },
	{ dtab_quest, dbg_qt_stop,			_T("Dừng bước đang chạy"),
		_T("Dừng bước đang chạy (dừng cả MapPath tự đi của bước đó) và in nhật ký"), 0 },
	{ dtab_quest, dbg_qt_watch,		_T("Theo dõi thông báo nhiệm vụ (bật/tắt)"),
		_T("Hook CoreDataChanged / SystemMessageArrival / ChannelMessageArrival: ghi thông báo + tự phân tích nhiệm vụ (Bước 1, 4, 5 tự bật). X = 1: ghi mọi id (trừ 33, 79)"), 0 },
	{ dtab_quest, dbg_qt_watchview,	_T("Xem thông báo nhiệm vụ"),
		_T("Kết quả phân tích + thông báo đã ghi, mới nhất trước (ô Chuỗi = lọc). X = 1: xóa sau khi xem. Đầy đủ ở Logs\\QuestLog.txt"), 0 },
	{ dtab_quest, dbg_qt_taskval,		_T("g_pCurPlayer->GetTaskVal(Idx)"),
		_T("Đọc giá trị Task số = ô Idx/Số. X > Idx: liệt kê các task Idx..X khác 0 (tối đa 6000)"), 0 },
	{ dtab_quest, dbg_qt_reset,		_T("Xóa kết quả phân tích"),
		_T("Xóa nhiệm vụ đã phân tích (tên quái, map, tiến độ) và nhật ký 3 bước"), 0 },
};

static const int s_nButtonCount = sizeof(s_Buttons) / sizeof(s_Buttons[0]);

void AllowMessageFromGame(HWND hWnd, UINT uMsg)
{
	// ChangeWindowMessageFilterEx: Windows 7+ (WINVER cua project la XP nen lay dong)
	typedef BOOL(WINAPI *PFN_CWMFEX)(HWND, UINT, DWORD, PVOID);
	static PFN_CWMFEX pfnFilterEx = (PFN_CWMFEX)GetProcAddress(GetModuleHandle(_T("user32.dll")), "ChangeWindowMessageFilterEx");
	if (pfnFilterEx && hWnd)
		pfnFilterEx(hWnd, uMsg, MSGFLT_ALLOW, NULL);
}

static LPCTSTR GetIntegrityName(HANDLE hProcess)
{
	HANDLE hToken = NULL;
	if (!OpenProcessToken(hProcess, TOKEN_QUERY, &hToken))
		return _T("?");

	LPCTSTR szName = _T("?");
	BYTE buffer[64];
	DWORD cbSize = 0;
	if (GetTokenInformation(hToken, TokenIntegrityLevel, buffer, sizeof(buffer), &cbSize))
	{
		PSID pSid = ((TOKEN_MANDATORY_LABEL*)buffer)->Label.Sid;
		DWORD dwRid = *GetSidSubAuthority(pSid, *GetSidSubAuthorityCount(pSid) - 1);
		if (dwRid >= SECURITY_MANDATORY_SYSTEM_RID) szName = _T("System");
		else if (dwRid >= SECURITY_MANDATORY_HIGH_RID) szName = _T("Admin (High)");
		else if (dwRid >= SECURITY_MANDATORY_MEDIUM_RID) szName = _T("Thường (Medium)");
		else szName = _T("Thấp (Low)");
	}
	CloseHandle(hToken);
	return szName;
}

// CDlgDebug dialog

IMPLEMENT_DYNAMIC(CDlgDebug, CDialog)

CDlgDebug::CDlgDebug(CWnd* pParent /*=NULL*/)
	: CDialog(CDlgDebug::IDD, pParent)
{
	memset(m_hButtons, 0, sizeof(m_hButtons));
	memset(m_Params, 0, sizeof(m_Params));
	m_nSeq = 0;
	m_nWaitSeq = 0;
	m_dwWaitTick = 0;
	m_nFillPosSeq = 0;
}

CDlgDebug::~CDlgDebug()
{
}

void CDlgDebug::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_DEBUG_MAIN_TAB, m_MainTab);
	DDX_Control(pDX, IDC_DEBUG_OUTPUT_RICHEDIT, m_Output);
}

BEGIN_MESSAGE_MAP(CDlgDebug, CDialog)
	ON_NOTIFY(TCN_SELCHANGE, IDC_DEBUG_MAIN_TAB, &CDlgDebug::OnTcnSelchangeMainTab)
	ON_CONTROL_RANGE(BN_CLICKED, IDC_DBG_CMD_FIRST, IDC_DBG_CMD_LAST, &CDlgDebug::OnCmdButtonClicked)
	ON_BN_CLICKED(IDC_DBG_GETPOS, &CDlgDebug::OnBnClickedGetPos)
	ON_BN_CLICKED(IDC_DBG_GETFLAG, &CDlgDebug::OnBnClickedGetFlag)
	ON_BN_CLICKED(IDC_DBG_CLEAR, &CDlgDebug::OnBnClickedClear)
	ON_BN_CLICKED(IDC_DBG_CHECKHOOK, &CDlgDebug::OnBnClickedCheckHook)
	ON_WM_COPYDATA()
	ON_WM_TIMER()
	ON_WM_DESTROY()
END_MESSAGE_MAP()

// CDlgDebug message handlers

CRect CDlgDebug::GetDlgRect(int x, int y, int cx, int cy)
{
	CRect rc(x, y, x + cx, y + cy);
	MapDialogRect(&rc);
	return rc;
}

static HWND CreateCtrl(CWnd* pParent, LPCTSTR szClass, LPCTSTR szText, DWORD dwStyle, DWORD dwExStyle, const CRect& rc, UINT nID)
{
	HWND hWnd = ::CreateWindowEx(dwExStyle, szClass, szText, WS_CHILD | WS_VISIBLE | dwStyle,
		rc.left, rc.top, rc.Width(), rc.Height(), pParent->GetSafeHwnd(), (HMENU)(UINT_PTR)nID, AfxGetInstanceHandle(), NULL);
	if (hWnd)
		::SendMessage(hWnd, WM_SETFONT, (WPARAM)pParent->GetFont()->GetSafeHandle(), FALSE);
	return hWnd;
}

BOOL CDlgDebug::OnInitDialog()
{
	CDialog::OnInitDialog();

	m_ToolTip.Create(this);
	m_ToolTip.SetMaxTipWidth(400);

	// Ô tham số (dùng chung cho các nút)
	CreateCtrl(this, _T("STATIC"), _T("Nhân vật:"), SS_LEFT, 0, GetDlgRect(7, 9, 36, 9), IDC_STATIC);
	CreateCtrl(this, _T("STATIC"), _T(""), SS_LEFT | SS_ENDELLIPSIS, 0, GetDlgRect(45, 9, 157, 9), IDC_DBG_TARGET);

	CreateCtrl(this, _T("STATIC"), _T("X"), SS_RIGHT, 0, GetDlgRect(7, 24, 10, 9), IDC_STATIC);
	HWND hX = CreateCtrl(this, _T("EDIT"), _T(""), WS_TABSTOP | ES_AUTOHSCROLL, WS_EX_CLIENTEDGE, GetDlgRect(20, 22, 45, 12), IDC_DBG_X_EDIT);
	CreateCtrl(this, _T("STATIC"), _T("Y"), SS_RIGHT, 0, GetDlgRect(68, 24, 10, 9), IDC_STATIC);
	HWND hY = CreateCtrl(this, _T("EDIT"), _T(""), WS_TABSTOP | ES_AUTOHSCROLL, WS_EX_CLIENTEDGE, GetDlgRect(81, 22, 45, 12), IDC_DBG_Y_EDIT);
	CreateCtrl(this, _T("STATIC"), _T("Idx/Số"), SS_RIGHT, 0, GetDlgRect(129, 24, 26, 9), IDC_STATIC);
	HWND hIdx = CreateCtrl(this, _T("EDIT"), _T("0"), WS_TABSTOP | ES_AUTOHSCROLL, WS_EX_CLIENTEDGE, GetDlgRect(158, 22, 40, 12), IDC_DBG_IDX_EDIT);
	CreateCtrl(this, _T("STATIC"), _T("Chuỗi"), SS_RIGHT, 0, GetDlgRect(7, 40, 24, 9), IDC_STATIC);
	HWND hStr = CreateCtrl(this, _T("EDIT"), _T(""), WS_TABSTOP | ES_AUTOHSCROLL, WS_EX_CLIENTEDGE, GetDlgRect(34, 38, 164, 12), IDC_DBG_STR_EDIT);
	::SendMessage(hStr, EM_LIMITTEXT, sizeof(m_Params[0].szParam) - 1, 0);

	CreateCtrl(this, _T("BUTTON"), _T("Kiểm tra VLHook"), WS_TABSTOP | BS_PUSHBUTTON, 0, GetDlgRect(205, 5, 77, 14), IDC_DBG_CHECKHOOK);
	HWND hGetPos = CreateCtrl(this, _T("BUTTON"), _T("Lấy vị trí hiện tại"), WS_TABSTOP | BS_PUSHBUTTON, 0, GetDlgRect(205, 21, 77, 14), IDC_DBG_GETPOS);
	HWND hGetFlag = CreateCtrl(this, _T("BUTTON"), _T("Lấy tọa độ cắm cờ"), WS_TABSTOP | BS_PUSHBUTTON, 0, GetDlgRect(205, 37, 77, 14), IDC_DBG_GETFLAG);
	HWND hAutoPos = CreateCtrl(this, _T("BUTTON"), _T("Tự đọc lại vị trí 1 giây sau khi MoveTo"), WS_TABSTOP | BS_AUTOCHECKBOX, 0, GetDlgRect(7, 57, 190, 10), IDC_DBG_AUTOPOS);
	::SendMessage(hAutoPos, BM_SETCHECK, BST_CHECKED, 0);
	CreateCtrl(this, _T("BUTTON"), _T("Xóa kết quả"), WS_TABSTOP | BS_PUSHBUTTON, 0, GetDlgRect(205, 55, 77, 14), IDC_DBG_CLEAR);

	CreateCtrl(this, _T("STATIC"),
		_T("Chọn nhân vật ở cửa sổ chính, bấm nút để chạy hàm đó trong Game (VLHook).\n")
		_T("X/Y: tọa độ thật (như GetMapPos). Lỗi sai địa chỉ sẽ báo EXCEPTION thay vì làm sập game."),
		SS_LEFT, 0, GetDlgRect(285, 9, 197, 60), IDC_STATIC);
	// Nhãn ngay trên ô kết quả (vị trí ô kết quả lấy theo .rc)
	CRect rcOut, rcLabel = GetDlgRect(0, 0, 0, 11);
	m_Output.GetWindowRect(&rcOut);
	ScreenToClient(&rcOut);
	rcLabel.SetRect(rcOut.left, rcOut.top - rcLabel.Height(), rcOut.right, rcOut.top - 1);
	CreateCtrl(this, _T("STATIC"), _T("Kết quả trả về từ VLHook:"), SS_LEFT, 0, rcLabel, IDC_STATIC);

	m_ToolTip.AddTool(CWnd::FromHandle(hX), _T("Tọa độ X thật (P_PLAYER->GetMapPos). Tọa độ game = X >> 8"));
	m_ToolTip.AddTool(CWnd::FromHandle(hY), _T("Tọa độ Y thật (P_PLAYER->GetMapPos). Tọa độ game = Y >> 9"));
	m_ToolTip.AddTool(CWnd::FromHandle(hIdx), _T("Chỉ số NPC / số Task / phạm vi / số dòng hội thoại (tùy nút)"));
	m_ToolTip.AddTool(CWnd::FromHandle(hStr), _T("Tên NPC / nội dung thông báo (tùy nút)"));
	m_ToolTip.AddTool(CWnd::FromHandle(hGetPos), _T("P_PLAYER->GetMapPos rồi điền vào X/Y"));
	m_ToolTip.AddTool(CWnd::FromHandle(hGetFlag), _T("Đọc tọa độ cắm cờ trên bản đồ nhỏ rồi điền vào X/Y"));

	// Kết quả
	m_OutFont.CreatePointFont(90, _T("Consolas"));
	m_Output.SetFont(&m_OutFont);
	m_Output.LimitText(4 * 1024 * 1024);
	m_Output.SetBackgroundColor(FALSE, RGB(255, 255, 255));

	// Tab + nút lệnh
	m_MainTab.ModifyStyle(0, WS_CLIPSIBLINGS);
	for (int i = 0; i < dtab_num; i++)
		m_MainTab.InsertItem(i, s_szTabs[i]);
	CreateButtons();
	m_MainTab.SetCurSel(dtab_move);
	ShowTabButtons();

	// Game (quyen thuong) gui ket qua len VLAuto (Admin) bang WM_COPYDATA => phai cho phep (UIPI)
	AllowMessageFromGame(GetSafeHwnd(), WM_COPYDATA);

	UpdateTarget();
	SetTimer(TIMER_CHECK, 500, NULL);
	CheckHook();

	return TRUE;
}

void CDlgDebug::CreateButtons()
{
	CRect rcTab;
	m_MainTab.GetWindowRect(&rcTab);
	ScreenToClient(&rcTab);
	m_MainTab.AdjustRect(FALSE, &rcTab);

	CRect rcUnit = GetDlgRect(0, 0, 3, 14);		// rcUnit.right = 3 DLU (khoảng cách), bottom = chiều cao nút
	int nGap = rcUnit.right;
	int nHeight = rcUnit.bottom;
	int nMinWidth = GetDlgRect(0, 0, 140, 0).Width();	// Số cột tùy độ rộng tab (sửa trong .rc)
	int nCols = max(1, (rcTab.Width() - nGap) / (nMinWidth + nGap));
	int nWidth = (rcTab.Width() - (nCols + 1) * nGap) / nCols;

	int nRow[dtab_num] = { 0 };
	m_nButtons = 0;
	for (int i = 0; i < s_nButtonCount && i < MAX_BUTTONS; i++)
	{
		const DEBUG_BUTTON& btn = s_Buttons[i];
		int n = nRow[btn.nTab]++;
		int x = rcTab.left + nGap + (n % nCols) * (nWidth + nGap);
		int y = rcTab.top + nGap + (n / nCols) * (nHeight + nGap);
		CRect rc(x, y, x + nWidth, y + nHeight);

		HWND hBtn = CreateCtrl(this, _T("BUTTON"), btn.szLabel, WS_TABSTOP | BS_PUSHBUTTON | BS_LEFT, 0, rc, IDC_DBG_CMD_FIRST + i);
		::SetWindowPos(hBtn, HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
		m_ToolTip.AddTool(CWnd::FromHandle(hBtn), btn.szTip);
		m_hButtons[m_nButtons++] = hBtn;
	}
}

void CDlgDebug::ShowTabButtons()
{
	int nTab = m_MainTab.GetCurSel();
	for (int i = 0; i < m_nButtons; i++)
		::ShowWindow(m_hButtons[i], s_Buttons[i].nTab == nTab ? SW_SHOW : SW_HIDE);
}

void CDlgDebug::OnTcnSelchangeMainTab(NMHDR *pNMHDR, LRESULT *pResult)
{
	ShowTabButtons();
	*pResult = 0;
}

BOOL CDlgDebug::PreTranslateMessage(MSG* pMsg)
{
	if (m_ToolTip.GetSafeHwnd())
		m_ToolTip.RelayEvent(pMsg);
	return CDialog::PreTranslateMessage(pMsg);
}

void CDlgDebug::OnCancel()
{
	ShowWindow(SW_HIDE);	// Giữ lại kết quả, bấm DEBUG để mở lại
}

void CDlgDebug::OnDestroy()
{
	KillTimer(TIMER_CHECK);
	KillTimer(TIMER_READPOS);
	CDialog::OnDestroy();
}

void CDlgDebug::ShowDebug()
{
	UpdateTarget();
	ShowWindow(SW_SHOW);
	SetForegroundWindow();
}

void CDlgDebug::UpdateTarget()
{
	CString strTarget;
	if (g_pCurGameWin == NULL || !g_pCurGameWin->m_hWnd)
		strTarget = _T("(chưa chọn nhân vật)");
	else
	{
		TCHAR szName[33] = _T("");
		TCVN3ToUnicode(g_pCurGameWin->m_szPlayerName, szName, 32);
		strTarget.Format(_T("%s   [HWND 0x%08X]  %s"), szName[0] ? szName : _T("?"), (UINT)(UINT_PTR)g_pCurGameWin->m_hWnd,
			g_pCurGameWin->m_bAttached ? _T("Đã gắn Auto") : _T("CHƯA gắn Auto - lệnh sẽ không chạy"));
	}

	CString strOld;
	GetDlgItemText(IDC_DBG_TARGET, strOld);
	if (strOld != strTarget)
		SetDlgItemText(IDC_DBG_TARGET, strTarget);
}

LPCTSTR CDlgDebug::GetCmdLabel(int nCmd)
{
	for (int i = 0; i < s_nButtonCount; i++)
		if (s_Buttons[i].nCmd == nCmd)
			return s_Buttons[i].szLabel;
	return _T("?");
}

void CDlgDebug::AppendText(LPCTSTR szText, COLORREF crColor, BOOL bBold)
{
	CString strText(szText);
	strText.Replace(_T("\r\n"), _T("\n"));
	strText.Replace(_T("\n"), _T("\r\n"));

	m_Output.SetSel(-1, -1);
	CHARFORMAT2 cf;
	memset(&cf, 0, sizeof(cf));
	cf.cbSize = sizeof(cf);
	cf.dwMask = CFM_COLOR | CFM_BOLD;
	cf.crTextColor = crColor;
	cf.dwEffects = bBold ? CFE_BOLD : 0;
	m_Output.SetSelectionCharFormat(cf);
	m_Output.ReplaceSel(strText);
	m_Output.SendMessage(WM_VSCROLL, SB_BOTTOM, 0);
}

// Kết quả từ VLHook (TCVN3): dòng thường đổi sang Unicode,
// dòng bắt đầu bằng DEBUG_RAW_MARK hiện nguyên byte TCVN3 (cp1252) - như chuỗi viết trong mã nguồn
void CDlgDebug::AppendReplyText(LPCSTR szText, COLORREF crColor)
{
	static TCHAR szLine[DEBUG_TEXT_SIZE + 1];
	CStringA strUni;
	const char *p = szText;
	while (*p)
	{
		const char *pEnd = strchr(p, '\n');
		int nLen = pEnd ? (int)(pEnd - p) + 1 : (int)strlen(p);
		if (*p == DEBUG_RAW_MARK)
		{
			if (!strUni.IsEmpty())
			{
				TCVN3ToUnicode(strUni, szLine, DEBUG_TEXT_SIZE);
				AppendText(szLine, crColor);
				strUni.Empty();
			}
			int nW = MultiByteToWideChar(1252, 0, p + 1, nLen - 1, szLine, DEBUG_TEXT_SIZE);
			szLine[nW > 0 ? nW : 0] = 0;
			AppendText(szLine, CLR_RAW);
		}
		else
			strUni.Append(p, nLen);
		p += nLen;
	}
	if (!strUni.IsEmpty())
	{
		TCVN3ToUnicode(strUni, szLine, DEBUG_TEXT_SIZE);
		AppendText(szLine, crColor);
	}
}

void CDlgDebug::AppendLog(LPCTSTR szText, COLORREF crColor, BOOL bBold)
{
	CString strLine;
	strLine.Format(_T("[%s] %s\n"), (LPCTSTR)CTime::GetCurrentTime().Format(_T("%H:%M:%S")), szText);
	AppendText(strLine, crColor, bBold);
}

BOOL CDlgDebug::SendDebugCmd(int nCmd, BOOL bFillPos)
{
	UpdateTarget();
	if (g_pCurGameWin == NULL || !g_pCurGameWin->m_hWnd || !::IsWindow(g_pCurGameWin->m_hWnd))
	{
		AppendLog(_T("LỖI: chưa chọn nhân vật / cửa sổ game không còn"), CLR_ERROR, TRUE);
		return FALSE;
	}
	if (nCmd >= dbg_lg_status && nCmd <= dbg_lg_last)
	{
		RunLoginCmd(nCmd);	// Màn hình đăng nhập: VLAuto tự chạy (Game chưa nạp VLHook)
		return TRUE;
	}
	if (!g_pCurGameWin->m_bAttached)
	{
		AppendLog(_T("LỖI: nhân vật chưa gắn Auto (VLHook chưa nạp vào game)"), CLR_ERROR, TRUE);
		return FALSE;
	}

	m_nSeq++;
	DEBUG_PARAM& param = m_Params[m_nSeq % MAX_PENDING];
	memset(&param, 0, sizeof(param));
	param.cbSize = sizeof(DEBUG_PARAM);
	param.hWndReply = GetSafeHwnd();
	param.nCmd = nCmd;
	param.nSeq = m_nSeq;
	param.nParam[0] = (int)GetDlgItemInt(IDC_DBG_X_EDIT, NULL, TRUE);
	param.nParam[1] = (int)GetDlgItemInt(IDC_DBG_Y_EDIT, NULL, TRUE);
	param.nParam[2] = (int)GetDlgItemInt(IDC_DBG_IDX_EDIT, NULL, TRUE);
	CString strParam;
	GetDlgItemText(IDC_DBG_STR_EDIT, strParam);
	UnicodeToTCVN3(strParam, param.szParam, sizeof(param.szParam) - 1);

	CString strLog;
	strLog.Format(_T("#%d  %s    (X=%d, Y=%d, Idx=%d%s%s)"), m_nSeq, GetCmdLabel(nCmd),
		param.nParam[0], param.nParam[1], param.nParam[2], strParam.IsEmpty() ? _T("") : _T(", Chuỗi="), (LPCTSTR)strParam);
	AppendLog(strLog, CLR_CMD, TRUE);

	if (!::PostMessage(g_pCurGameWin->m_hWnd, WM_HOOK_DEBUG, (WPARAM)&param, sizeof(DEBUG_PARAM)))
	{
		strLog.Format(_T("LỖI: PostMessage thất bại (GetLastError = %d)"), GetLastError());
		AppendLog(strLog, CLR_ERROR, TRUE);
		return FALSE;
	}

	m_nWaitSeq = m_nSeq;
	m_dwWaitTick = GetTickCount();
	if (bFillPos)
		m_nFillPosSeq = m_nSeq;
	return TRUE;
}

static LPCTSTR s_szInitButtons[] = { _T("Bắt Đầu Trò Chơi"), _T("Tùy Chọn Hệ Thống"), _T("Xem Ghi Hình"), _T("Thoát Khỏi Trò Chơi") };

static void AppendServerList(CString& strOut, HANDLE hGame, DWORD dwList, int nCount, int nCurSel)
{
	for (int i = 0; dwList && i < nCount && i < 64; i++)
	{
		BYTE server[LG_SERVER_ROW] = { 0 };
		SIZE_T nRead = 0;
		if (!ReadProcessMemory(hGame, (LPCVOID)(UINT_PTR)(dwList + i * LG_SERVER_ROW), server, sizeof(server), &nRead))
			break;
		server[31] = 0;
		WCHAR szTitle[64];
		TCVN3ToUnicode((LPCSTR)server, szTitle, 63);
		CString strLine;
		strLine.Format(_T("  %s[%d] %s  %d.%d.%d.%d\n"), i == nCurSel ? _T("=>") : _T("  "), i, szTitle,
			server[32], server[33], server[34], server[35]);
		strOut += strLine;
	}
}

static void AppendWndState(CString& strOut, HANDLE hGame, LPCTSTR szName, DWORD dwSelfPtr)
{
	CString strLine;
	DWORD dwWnd = ReadGameDword(hGame, dwSelfPtr);
	strLine.Format(_T("%s (*0x%08X) = 0x%08X"), szName, dwSelfPtr, dwWnd);
	strOut += strLine;
	if (dwWnd)
	{
		DWORD dwStyle = ReadGameDword(hGame, dwWnd + 4);
		strLine.Format(_T("  m_Style 0x%08X  IsVisible %d\n"), dwStyle, (dwStyle & LG_WND_S_VISIBLE) ? 1 : 0);
		strOut += strLine;
	}
	else
		strOut += _T("  (chưa mở)\n");
}

void CDlgDebug::LoginStatus(HANDLE hGame)
{
	CString strOut, strLine;
	DWORD dwSysMsg = ReadGameDword(hGame, LG_SMC_BASE_ADD);
	strLine.Format(_T("KUiSysMsgCentre (*0x%08X) = 0x%08X => %s\n"), LG_SMC_BASE_ADD, dwSysMsg, dwSysMsg ? _T("ĐANG TRONG GAME") : _T("chưa vào game"));
	strOut += strLine;

	AppendWndState(strOut, hGame, _T("KUiInit - màn hình bắt đầu"), LG_UIN_BASE_ADD);
	DWORD dwUiInit = ReadGameDword(hGame, LG_UIN_BASE_ADD);
	for (int i = 0; dwUiInit && i < (int)INIT_BUTTON_NUM; i++)
	{
		DWORD dwBtn = dwUiInit + s_dwInitButtons[i];
		DWORD dwBtnStyle = ReadGameDword(hGame, dwBtn + 4);
		strLine.Format(_T("  [%d] +0x%04X = 0x%08X  vtable 0x%08X  m_Style 0x%08X  IsVisible %d  %s\n"), i, s_dwInitButtons[i], dwBtn,
			ReadGameDword(hGame, dwBtn), dwBtnStyle, (dwBtnStyle & LG_WND_S_VISIBLE) ? 1 : 0, s_szInitButtons[i]);
		strOut += strLine;
	}

	AppendWndState(strOut, hGame, _T("KUiHealthNotice - bảng Nhắc nhở"), LG_NOTICE_SELF);

	AppendWndState(strOut, hGame, _T("KUiSelServer - chọn máy chủ"), LG_SEL_SELF);
	DWORD dwSel = ReadGameDword(hGame, LG_SEL_SELF);
	if (dwSel)
	{
		int nRegions = GetRegionCount(hGame, dwSel);
		int nRegion = (int)ReadGameDword(hGame, dwSel + LG_SEL_REGION);
		strLine.Format(_T("  Khu vực (*+0x%X, Left/RightList): %d khu vực, đang chọn %d  (Idx cho nút Chọn khu vực)\n"), LG_SEL_REGIONS, nRegions, nRegion);
		strOut += strLine;
		AppendServerList(strOut, hGame, ReadGameDword(hGame, dwSel + LG_SEL_REGIONS), nRegions, nRegion);
		int nLines = (int)ReadGameDword(hGame, dwSel + LG_SEL_IPLIST + LG_LIST_COUNT);
		int nLine = (int)ReadGameDword(hGame, dwSel + LG_SEL_IPLIST + LG_LIST_CURSEL);
		strLine.Format(_T("  Máy chủ của khu vực đang chọn (*+0x%X, IpList): %d máy chủ, đang chọn %d  (Idx cho nút Chọn máy chủ; nút Đăng nhập dùng máy chủ này)\n"), LG_SEL_SERVERS, nLines, nLine);
		strOut += strLine;
		AppendServerList(strOut, hGame, ReadGameDword(hGame, dwSel + LG_SEL_SERVERS), nLines, nLine);
	}

	AppendWndState(strOut, hGame, _T("KUiLogin - nhập tài khoản"), LG_LOGIN_SELF);

	AppendWndState(strOut, hGame, _T("KUiSelPlayer - chọn nhân vật"), LG_ROLE_SELF);
	DWORD dwRoleWnd = ReadGameDword(hGame, LG_ROLE_SELF);
	int nRoleSel = -1, nRolePage = 0;
	if (dwRoleWnd)
	{
		short sh[8] = { 0 };
		ReadProcessMemory(hGame, (LPCVOID)(UINT_PTR)(dwRoleWnd + LG_ROLE_NUM), sh, sizeof(sh), NULL);
		nRoleSel = sh[(LG_ROLE_SEL - LG_ROLE_NUM) / 2];
		nRolePage = sh[(LG_ROLE_PAGE - LG_ROLE_NUM) / 2];
		strLine.Format(_T("  m_nNumPlayer %d, m_nSelPlayer %d (ô trên trang), đã lật %d => nhân vật đang chọn %d\n"),
			sh[0], nRoleSel, nRolePage, nRoleSel >= 0 ? nRoleSel + nRolePage : -1);
		strOut += strLine;
	}
	short nRoles = 0;
	ReadProcessMemory(hGame, (LPCVOID)(UINT_PTR)(LG_LOGIC + LG_LOGIC_ROLENUM), &nRoles, sizeof(nRoles), NULL);
	strLine.Format(_T("g_LoginLogic 0x%08X: m_Status %d, %d nhân vật  (Idx cho nút Chọn nhân vật)\n"),
		LG_LOGIC, ReadGameDword(hGame, LG_LOGIC + LG_LOGIC_STATUS), nRoles);
	strOut += strLine;
	for (int i = 0; i < nRoles && i < 50; i++)
	{
		BYTE role[LG_ROLE_INFO_SIZE] = { 0 };
		if (!ReadProcessMemory(hGame, (LPCVOID)(UINT_PTR)(LG_LOGIC + LG_LOGIC_ROLES + i * LG_ROLE_INFO_SIZE), role, sizeof(role), NULL))
			break;
		role[31] = 0;
		WCHAR szName[64];
		TCVN3ToUnicode((LPCSTR)role, szName, 63);
		BOOL bCur = dwRoleWnd && nRoleSel >= 0 && i == nRoleSel + nRolePage;
		strLine.Format(_T("  %s[%d] %s  cấp %d  giới tính %d  ngũ hành %d%s\n"), bCur ? _T("=>") : _T("  "), i, szName,
			*(int*)(role + LG_ROLE_INFO_LEVEL), role[32], role[33], (dwRoleWnd && (i < nRolePage || i >= nRolePage + LG_ROLE_PER_PAGE)) ? _T("  (trang khác)") : _T(""));
		strOut += strLine;
	}

	AppendText(strOut, CLR_TEXT);
}

#define SERVERLIST_XOR		0x32	// 10_10: UserDataCl\serverlist.ini mã hóa XOR (updatedll.dll UnEncrypt)
#define SERVERLIST_PAK_FILE	"\\settings\\serverlist.ini"	// 15_10: nằm trong data\*.pak (0x5AC720 đọc "\Settings\ServerList.ini")
#define SERVERLIST_OUT_FILE	_T("%s\\UiConfig\\serverlist_.txt")	// Auto Login đọc file này

// Mã file trong pak (g_FileName2Id): chữ thường, '/' -> '\'
static DWORD PakFileNameId(LPCSTR szName)
{
	DWORD dwId = 0;
	for (int i = 0; szName[i]; i++)
	{
		char c = szName[i];
		if (c >= 'A' && c <= 'Z')
			c += 'a' - 'A';
		else if (c == '/')
			c = '\\';
		dwId = (dwId + (DWORD)((i + 1) * c)) % 0x8000000B * 0xFFFFFFEF;
	}
	return dwId ^ 0x12345678;
}

// Giải nén UCL nrv2b (bit buffer 8 bit) - pak method 1
static BOOL UclNrv2bDecompress8(const BYTE* pSrc, DWORD dwSrcLen, BYTE* pDst, DWORD dwDstLen)
{
	DWORD bb = 0, ip = 0, op = 0, dwLastOff = 1;
	BOOL bErr = FALSE;
	auto GetByte = [&]() -> DWORD { if (ip < dwSrcLen) return pSrc[ip++]; bErr = TRUE; return 0; };
	auto GetBit = [&]() -> DWORD { if (bb & 0x7F) bb *= 2; else bb = GetByte() * 2 + 1; return (bb >> 8) & 1; };
	for (;;)
	{
		while (GetBit())
		{
			if (op >= dwDstLen)
				return FALSE;
			pDst[op++] = (BYTE)GetByte();
			if (bErr)
				return FALSE;
		}
		DWORD dwOff = 1;
		do
		{
			dwOff = dwOff * 2 + GetBit();
			if (bErr || dwOff > 0x1000002)
				return FALSE;
		} while (!GetBit());
		if (dwOff == 2)
			dwOff = dwLastOff;
		else
		{
			dwOff = (dwOff - 3) * 256 + GetByte();
			if (dwOff == 0xFFFFFFFF)
				break;
			dwLastOff = ++dwOff;
		}
		DWORD dwLen = GetBit();
		dwLen = dwLen * 2 + GetBit();
		if (dwLen == 0)
		{
			dwLen = 1;
			do
			{
				dwLen = dwLen * 2 + GetBit();
				if (bErr || dwLen >= dwDstLen)
					return FALSE;
			} while (!GetBit());
			dwLen += 2;
		}
		dwLen += (dwOff > 0xD00);
		if (bErr || dwOff > op || op + dwLen + 1 > dwDstLen)
			return FALSE;
		for (DWORD i = 0; i <= dwLen; i++, op++)
			pDst[op] = pDst[op - dwOff];
	}
	return !bErr && op == dwDstLen;
}

static BOOL ReadFileAt(HANDLE hFile, DWORD dwOffset, void* pBuf, DWORD dwSize)
{
	DWORD dwRead = 0;
	return SetFilePointer(hFile, dwOffset, NULL, FILE_BEGIN) != INVALID_SET_FILE_POINTER &&
		ReadFile(hFile, pBuf, dwSize, &dwRead, NULL) && dwRead == dwSize;
}

// Tìm và giải nén szName trong file pak. 1 = OK, 0 = không có, -1 = lỗi (strErr)
static int ReadPakFile(LPCTSTR szPak, LPCSTR szName, CStringA& strData, CString& strErr)
{
	HANDLE hFile = CreateFile(szPak, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);
	if (hFile == INVALID_HANDLE_VALUE)
		return 0;
	// Header: "PACK", số file, vị trí bảng chỉ mục, vị trí dữ liệu; mỗi dòng chỉ mục 16 byte:
	// mã file, vị trí, kích thước gốc, (method << 24) | kích thước nén
	DWORD header[4] = { 0 }, dwId = PakFileNameId(szName);
	int nResult = 0;
	std::vector<DWORD> index;
	if (!ReadFileAt(hFile, 0, header, sizeof(header)) || header[0] != 0x4B434150 || header[1] > 0x100000)
		strErr = _T("không phải file pak");
	else
	{
		index.resize(header[1] * 4 + 4);
		if (!ReadFileAt(hFile, header[2], &index[0], header[1] * 16))
			strErr = _T("lỗi đọc bảng chỉ mục");
	}
	for (DWORD i = 0; strErr.IsEmpty() && i < header[1]; i++)
	{
		const DWORD* pEntry = &index[i * 4];
		if (pEntry[0] != dwId)
			continue;
		DWORD dwSize = pEntry[2], dwPackSize = pEntry[3] & 0xFFFFFF, dwMethod = pEntry[3] >> 24;
		nResult = -1;
		if (dwSize > 4 * 1024 * 1024 || dwPackSize > 4 * 1024 * 1024)
		{
			strErr.Format(_T("kích thước lạ (%u / %u)"), dwSize, dwPackSize);
			break;
		}
		std::vector<BYTE> pack(dwPackSize + 1);
		if (!ReadFileAt(hFile, pEntry[1], &pack[0], dwPackSize))
		{
			strErr = _T("lỗi đọc dữ liệu");
			break;
		}
		LPSTR pData = strData.GetBuffer(dwSize + 1);
		BOOL bOK = FALSE;
		if (dwMethod == 0)
			bOK = (dwPackSize == dwSize) && (memcpy(pData, &pack[0], dwSize), TRUE);
		else if (dwMethod == 1)
			bOK = UclNrv2bDecompress8(&pack[0], dwPackSize, (BYTE*)pData, dwSize);
		strData.ReleaseBuffer(bOK ? dwSize : 0);
		if (bOK)
			nResult = 1;
		else if (dwMethod <= 1)
			strErr = _T("giải nén lỗi");
		else
			strErr.Format(_T("chưa hỗ trợ kiểu nén %u"), dwMethod);
		break;
	}
	CloseHandle(hFile);
	return strErr.IsEmpty() ? nResult : -1;
}

// Đọc file thường (10_10: có thể mã hóa XOR, đã giải mã thì bắt đầu bằng '[')
static BOOL ReadServerListIni(LPCTSTR szFile, CStringA& strData, BOOL& bEncrypted)
{
	bEncrypted = FALSE;
	HANDLE hFile = CreateFile(szFile, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);
	if (hFile == INVALID_HANDLE_VALUE)
		return FALSE;
	DWORD dwSize = GetFileSize(hFile, NULL), dwRead = 0;
	if (dwSize == INVALID_FILE_SIZE || dwSize > 4 * 1024 * 1024)
		dwSize = 0;
	LPSTR pData = strData.GetBuffer(dwSize + 1);
	BOOL bRead = ReadFile(hFile, pData, dwSize, &dwRead, NULL);
	CloseHandle(hFile);
	if (!bRead)
		dwRead = 0;
	bEncrypted = dwRead > 0 && pData[0] != '[' && (pData[0] ^ SERVERLIST_XOR) == '[';
	for (DWORD i = 0; bEncrypted && i < dwRead; i++)
		pData[i] ^= SERVERLIST_XOR;
	strData.ReleaseBuffer(dwRead);
	return dwRead > 0;
}

// Đọc danh sách server như Game 15_10: "\settings\serverlist.ini" trong các pak theo thứ tự package.ini
// (pak đầu tiên có file được dùng). Chuỗi = thư mục Game, file .pak hoặc file .ini (đọc thẳng).
// Xuất UTF-8 ra UiConfig\serverlist_.txt của VLAuto cho Auto Login.
void CDlgDebug::ServerListFile(HANDLE hGame, CString strPath)
{
	CString strLog, strErr, strSource;
	CStringA strData;
	strPath.Trim();
	strPath.Trim(_T("\""));
	if (strPath.IsEmpty() || PathIsDirectory(strPath))
	{
		TCHAR szDir[MAX_PATH] = _T("");
		if (!strPath.IsEmpty())
			lstrcpyn(szDir, strPath, MAX_PATH);
		else if (GetModuleFileNameEx(hGame, NULL, szDir, MAX_PATH))
			PathRemoveFileSpec(szDir);
		else
		{
			strLog.Format(_T("LỖI: không lấy được đường dẫn Game (GetLastError = %d)"), GetLastError());
			AppendLog(strLog, CLR_ERROR, TRUE);
			return;
		}
		PathRemoveBackslash(szDir);
		CString strGameDir = szDir, strIni = strGameDir + _T("\\package.ini");
		TCHAR szValue[MAX_PATH] = _T("");
		GetPrivateProfileString(_T("Package"), _T("Path"), _T("\\data"), szValue, MAX_PATH, strIni);
		CString strPakDir = szValue;
		strPakDir.Replace(_T("\\\\"), _T("\\"));
		strPakDir.TrimRight(_T('\\'));
		if (!strPakDir.IsEmpty() && strPakDir[0] != _T('\\'))
			strPakDir = _T("\\") + strPakDir;
		strPakDir = strGameDir + strPakDir;
		strLog.Format(_T("%s: thư mục pak %s\n"), (LPCTSTR)strIni, (LPCTSTR)strPakDir);
		AppendText(strLog, CLR_TEXT);
		for (int i = 0; ; i++)
		{
			CString strKey;
			strKey.Format(_T("%d"), i);
			if (!GetPrivateProfileString(_T("Package"), strKey, _T(""), szValue, MAX_PATH, strIni))
				break;
			CString strPak = strPakDir + _T("\\") + szValue, strPakErr;
			CStringA strPakData;
			int nRet = ReadPakFile(strPak, SERVERLIST_PAK_FILE, strPakData, strPakErr);
			if (nRet == 0)
				continue;
			if (nRet < 0)
				strLog.Format(_T("  [%d] %s: LỖI %s\n"), i, szValue, (LPCTSTR)strPakErr);
			else if (strSource.IsEmpty())
				strLog.Format(_T("  [%d] %s: có %hs (%d byte) => DÙNG\n"), i, szValue, SERVERLIST_PAK_FILE, strPakData.GetLength());
			else
				strLog.Format(_T("  [%d] %s: có %hs (%d byte), bị pak trước che\n"), i, szValue, SERVERLIST_PAK_FILE, strPakData.GetLength());
			AppendText(strLog, nRet < 0 ? CLR_ERROR : CLR_TEXT);
			if (nRet > 0 && strSource.IsEmpty())
			{
				strSource.Format(_T("%s (%hs)"), (LPCTSTR)strPak, SERVERLIST_PAK_FILE);
				strData = strPakData;
			}
		}
		if (strSource.IsEmpty())
		{
			// Không có trong pak => file rời
			CString strLoose = strGameDir + CString(SERVERLIST_PAK_FILE);
			BOOL bEncrypted = FALSE;
			if (ReadServerListIni(strLoose, strData, bEncrypted))
				strSource = strLoose;
		}
		if (strSource.IsEmpty())
		{
			strLog.Format(_T("LỖI: không tìm thấy %hs trong các pak của %s"), SERVERLIST_PAK_FILE, (LPCTSTR)strIni);
			AppendLog(strLog, CLR_ERROR, TRUE);
			return;
		}
	}
	else if (!PathMatchSpec(strPath, _T("*.pak")))
	{
		BOOL bEncrypted = FALSE;
		if (!ReadServerListIni(strPath, strData, bEncrypted))
		{
			strLog.Format(_T("LỖI: không đọc được %s (GetLastError = %d)"), (LPCTSTR)strPath, GetLastError());
			AppendLog(strLog, CLR_ERROR, TRUE);
			return;
		}
		strSource = strPath + (bEncrypted ? _T(" (đã giải mã XOR 0x32)") : _T(""));
	}
	else if (ReadPakFile(strPath, SERVERLIST_PAK_FILE, strData, strErr) > 0)
		strSource.Format(_T("%s (%hs)"), (LPCTSTR)strPath, SERVERLIST_PAK_FILE);
	else
	{
		strLog.Format(_T("LỖI: %s: %s"), (LPCTSTR)strPath, strErr.IsEmpty() ? _T("không có \\settings\\serverlist.ini") : (LPCTSTR)strErr);
		AppendLog(strLog, CLR_ERROR, TRUE);
		return;
	}

	// TCVN3 -> Unicode từng dòng
	CStringW strText;
	int nPos = 0;
	while (nPos < strData.GetLength())
	{
		int nEnd = strData.Find('\n', nPos);
		if (nEnd < 0)
			nEnd = strData.GetLength();
		CStringA strLineA = strData.Mid(nPos, nEnd - nPos);
		strLineA.TrimRight("\r");
		WCHAR szLine[512];
		TCVN3ToUnicode(strLineA, szLine, 511);
		strText += szLine;
		strText += L"\r\n";
		nPos = nEnd + 1;
	}

	// Xuất UTF-8 (BOM) cho Auto Login
	CString strOut;
	strOut.Format(SERVERLIST_OUT_FILE, g_szAppFolder);
	CString strOutDir = strOut;
	PathRemoveFileSpec(strOutDir.GetBuffer());
	strOutDir.ReleaseBuffer();
	CreateDirectory(strOutDir, NULL);
	int nLen = WideCharToMultiByte(CP_UTF8, 0, strText, strText.GetLength(), NULL, 0, NULL, NULL);
	CStringA strUtf8;
	WideCharToMultiByte(CP_UTF8, 0, strText, strText.GetLength(), strUtf8.GetBuffer(nLen + 1), nLen, NULL, NULL);
	strUtf8.ReleaseBuffer(nLen);
	BOOL bWrite = FALSE;
	HANDLE hFile = CreateFile(strOut, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hFile != INVALID_HANDLE_VALUE)
	{
		DWORD dwWritten = 0;
		static const BYTE bom[3] = { 0xEF, 0xBB, 0xBF };
		bWrite = WriteFile(hFile, bom, 3, &dwWritten, NULL) && WriteFile(hFile, (LPCSTR)strUtf8, nLen, &dwWritten, NULL);
		CloseHandle(hFile);
	}

	strLog.Format(_T("%s: %d byte\n"), (LPCTSTR)strSource, strData.GetLength());
	AppendText(strLog, CLR_TEXT);
	AppendText(CString(strText), CLR_TEXT);
	if (bWrite)
	{
		strLog.Format(_T("=> OK, đã xuất %s (Auto Login dùng file này)"), (LPCTSTR)strOut);
		AppendLog(strLog, CLR_OK, TRUE);
	}
	else
	{
		strLog.Format(_T("LỖI: không ghi được %s (GetLastError = %d)"), (LPCTSTR)strOut, GetLastError());
		AppendLog(strLog, CLR_ERROR, TRUE);
	}
}

void CDlgDebug::RunLoginCmd(int nCmd)
{
	int nIdx = (int)GetDlgItemInt(IDC_DBG_IDX_EDIT, NULL, TRUE);
	CString strParam, strLog;
	GetDlgItemText(IDC_DBG_STR_EDIT, strParam);
	m_nSeq++;
	strLog.Format(_T("#%d  %s    (Idx=%d)  [VLAuto tự chạy bằng thread trong Game]"), m_nSeq, GetCmdLabel(nCmd), nIdx);
	AppendLog(strLog, CLR_CMD, TRUE);

	// Tài khoản + mật khẩu: Chuỗi = "TàiKhoản MậtKhẩu"
	char szAccount[64] = "", szPassword[64] = "";
	if (nCmd == dbg_lg_login)
	{
		strParam.Trim();
		int nSpace = strParam.Find(_T(' '));
		if (nSpace <= 0)
		{
			AppendLog(_T("LỖI: nhập Chuỗi = \"TàiKhoản MậtKhẩu\" (cách nhau 1 dấu cách)"), CLR_ERROR, TRUE);
			return;
		}
		UnicodeToTCVN3(strParam.Left(nSpace), szAccount, 31);
		UnicodeToTCVN3(strParam.Mid(nSpace + 1).Trim(), szPassword, 31);
	}
	if (nCmd == dbg_lg_entergame)
		nIdx = 0;
	if (nCmd == dbg_lg_click && (nIdx < 0 || nIdx >= (int)INIT_BUTTON_NUM))
	{
		AppendLog(_T("LỖI: Idx 0 Bắt Đầu, 1 Tùy Chọn, 2 Xem Ghi Hình, 3 Thoát"), CLR_ERROR, TRUE);
		return;
	}

	DWORD dwPID = 0;
	GetWindowThreadProcessId(g_pCurGameWin->m_hWnd, &dwPID);
	HANDLE hGame = OpenProcess(PROCESS_VM_READ | PROCESS_VM_WRITE | PROCESS_VM_OPERATION | PROCESS_CREATE_THREAD | PROCESS_QUERY_INFORMATION, FALSE, dwPID);
	if (!hGame)
	{
		strLog.Format(_T("LỖI: OpenProcess(PID %u) thất bại (GetLastError = %d)"), dwPID, GetLastError());
		AppendLog(strLog, CLR_ERROR, TRUE);
		return;
	}

	if (nCmd == dbg_lg_serverlist)
	{
		ServerListFile(hGame, strParam);
		CloseHandle(hGame);
		return;
	}

	if (nCmd == dbg_lg_status)
	{
		LoginStatus(hGame);
		AppendLog(_T("=> OK"), CLR_OK, TRUE);
		CloseHandle(hGame);
		return;
	}

	if (nCmd == dbg_lg_region || nCmd == dbg_lg_server)
	{
		DWORD dwSel = ReadGameDword(hGame, LG_SEL_SELF);
		int nCount = 0;
		if (dwSel)
			nCount = nCmd == dbg_lg_region ? GetRegionCount(hGame, dwSel) : (int)ReadGameDword(hGame, dwSel + LG_SEL_IPLIST + LG_LIST_COUNT);
		if (nIdx < 0 || nIdx >= nCount)
		{
			strLog.Format(_T("LỖI: Idx phải từ 0 đến %d (cửa sổ chọn máy chủ 0x%08X có %d %s)"), nCount - 1, dwSel, nCount,
				nCmd == dbg_lg_region ? _T("khu vực") : _T("máy chủ trong khu vực đang chọn"));
			AppendLog(strLog, CLR_ERROR, TRUE);
			CloseHandle(hGame);
			return;
		}
	}

	int nRoleSlot = 0;
	if (nCmd == dbg_lg_role)
	{
		// Chỉ chọn được nhân vật đang hiện trên trang (Next/Pre lật trang)
		DWORD dwRoleWnd = ReadGameDword(hGame, LG_ROLE_SELF);
		short sh[8] = { 0 };
		if (dwRoleWnd)
			ReadProcessMemory(hGame, (LPCVOID)(UINT_PTR)(dwRoleWnd + LG_ROLE_NUM), sh, sizeof(sh), NULL);
		int nPage = sh[(LG_ROLE_PAGE - LG_ROLE_NUM) / 2];
		nRoleSlot = nIdx - nPage;
		if (!dwRoleWnd || nIdx < 0 || nIdx >= sh[0] || nRoleSlot < 0 || nRoleSlot >= LG_ROLE_PER_PAGE)
		{
			strLog.Format(_T("LỖI: cửa sổ chọn nhân vật 0x%08X, %d nhân vật, trang đang hiện %d..%d - Idx %d không chọn được"),
				dwRoleWnd, sh[0], nPage, nPage + LG_ROLE_PER_PAGE - 1, nIdx);
			AppendLog(strLog, CLR_ERROR, TRUE);
			CloseHandle(hGame);
			return;
		}
	}

	LPVOID pRemote = VirtualAllocEx(hGame, NULL, CRemoteCode::TOTAL_SIZE, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
	if (!pRemote)
	{
		strLog.Format(_T("LỖI: VirtualAllocEx thất bại (GetLastError = %d)"), GetLastError());
		AppendLog(strLog, CLR_ERROR, TRUE);
		CloseHandle(hGame);
		return;
	}

	CRemoteCode code((DWORD)(UINT_PTR)pRemote);
	DWORD dwSelfPtr = 0;
	CString strCall;
	switch (nCmd)
	{
	case dbg_lg_entergame:
	case dbg_lg_click:
		dwSelfPtr = LG_UIN_BASE_ADD;
		code.Begin(dwSelfPtr);
		code.PushThis(s_dwInitButtons[nIdx]);
		code.EcxThis(0);
		code.Call(LG_IOC_FUNC_ADD);
		strCall.Format(_T("KUiInit::OnClickButton(this + 0x%X = %s)"), s_dwInitButtons[nIdx], s_szInitButtons[nIdx]);
		break;
	case dbg_lg_region:
		dwSelfPtr = LG_SEL_SELF;
		code.Begin(dwSelfPtr);
		// Giống chọn 1 dòng ở Left/RightList (WndProc 0x48B4A0 msg 0x691): bỏ chọn list kia rồi nạp lại IpList
		// (SetCurSel tự báo 0x691 lên cửa sổ cha, nên bỏ chọn list kia bằng 0x483C30 - không báo)
		code.EcxThis(nIdx < LG_LIST_LEFT_NUM ? LG_SEL_RIGHTLIST : LG_SEL_LEFTLIST);
		code.Call(LG_LIST_CLEARSEL);
		code.PushImm(nIdx < LG_LIST_LEFT_NUM ? nIdx : nIdx - LG_LIST_LEFT_NUM);
		code.EcxThis(nIdx < LG_LIST_LEFT_NUM ? LG_SEL_LEFTLIST : LG_SEL_RIGHTLIST);
		code.Call(LG_LIST_SETCURSEL);
		// SetCurSel báo 0x691 => WndProc tự nạp lại IpList khi +0x226C = 1. Gọi LG_SEL_UPDATE thêm lần nữa sẽ
		// TerminateThread thread ping vừa tạo (đang giữ loader lock) => Game treo mọi thread mới
		if (ReadGameDword(hGame, ReadGameDword(hGame, LG_SEL_SELF) + LG_SEL_AUTOUPDATE) != 1)
		{
			code.EcxThis(0);
			code.Call(LG_SEL_UPDATE);
		}
		// Màu list như khi bấm vào khu vực (WndProc msg 0x202): khu vực thường, máy chủ nổi bật
		code.PushImm(0);
		code.EcxThis(LG_SEL_LEFTLIST);
		code.Call(LG_LIST_FOCUS);
		code.PushImm(0);
		code.EcxThis(LG_SEL_RIGHTLIST);
		code.Call(LG_LIST_FOCUS);
		code.PushImm(1);
		code.EcxThis(LG_SEL_IPLIST);
		code.Call(LG_LIST_FOCUS);
		strCall.Format(_T("KUiSelServer: chọn khu vực %d (%s[%d]) + nạp lại máy chủ 0x487160"), nIdx,
			nIdx < LG_LIST_LEFT_NUM ? _T("LeftList") : _T("RightList"), nIdx < LG_LIST_LEFT_NUM ? nIdx : nIdx - LG_LIST_LEFT_NUM);
		break;
	case dbg_lg_server:
		dwSelfPtr = LG_SEL_SELF;
		code.Begin(dwSelfPtr);
		// Chọn dòng ở IpList: KUiSelServer::OnLogin đọc dòng này (+0x191C), msg 0x691 từ IpList không làm gì thêm
		code.PushImm(nIdx);
		code.EcxThis(LG_SEL_IPLIST);
		code.Call(LG_LIST_SETCURSEL);
		strCall.Format(_T("KUiSelServer: IpList.SetCurSel(%d) - chọn máy chủ trong khu vực"), nIdx);
		break;
	case dbg_lg_role:
		dwSelfPtr = LG_ROLE_SELF;
		code.Begin(dwSelfPtr);
		code.PushImm(0);			// bPlaySound
		code.PushImm(nRoleSlot);	// nSel = ô trên trang
		code.EcxThis(0);
		code.Call(LG_ROLE_SELECT);
		strCall.Format(_T("KUiSelPlayer::OnSelectPlayer(ô %d, 0) - nhân vật %d"), nRoleSlot, nIdx);
		break;
	case dbg_lg_entergame2:
		dwSelfPtr = LG_ROLE_SELF;
		code.Begin(dwSelfPtr);
		code.PushImm(1);		// 15_10: tick đồng ý điều khoản (cbUserAgreement +0x8464)
		code.EcxThis(LG_ROLE_AGREE);
		code.Call(LG_BTN_CHECK);
		code.EcxThis(0);
		code.Call(LG_ROLE_ENTER);
		strCall = _T("KUiSelPlayer: tick đồng ý + OnEnterGame() - nhân vật đang chọn");
		break;
	case dbg_lg_notice:
		dwSelfPtr = LG_NOTICE_SELF;
		code.Begin(dwSelfPtr);
		code.PushImm(0);
		code.PushThis(LG_NOTICE_OK);
		code.PushImm(LG_WND_BUTTON_CLICK);
		code.EcxThis(0);
		code.Call(LG_NOTICE_WNDPROC);
		strCall.Format(_T("KUiHealthNotice::WndProc(0x%X, this + 0x%X = Xác định, 0)"), LG_WND_BUTTON_CLICK, LG_NOTICE_OK);
		break;
	case dbg_lg_selserver:
		dwSelfPtr = LG_SEL_SELF;
		code.Begin(dwSelfPtr);
		code.EcxThis(0);
		code.Call(LG_SEL_LOGIN_FUNC);
		strCall = _T("KUiSelServer::OnLogin() - máy chủ đang chọn");
		break;
	case dbg_lg_login:
		dwSelfPtr = LG_LOGIN_SELF;
		code.Begin(dwSelfPtr);
		code.PushImm(code.AddString(szAccount));
		code.EcxThis(LG_LOGIN_ACCOUNT);
		code.Call(LG_EDIT_SETTEXT);
		code.PushImm(code.AddString(szPassword));
		code.EcxThis(LG_LOGIN_PASSWORD);
		code.Call(LG_EDIT_SETTEXT);
		if (LG_LOGIN_AGREE)	// 15_10: cửa sổ đăng nhập không còn ô đồng ý
		{
			code.PushImm(1);
			code.EcxThis(LG_LOGIN_AGREE);
			code.Call(LG_BTN_CHECK);
		}
		code.EcxThis(0);
		code.Call(LG_LOGIN_FUNC);
		strCall.Format(_T("KUiLogin: SetText(TK \"%S\"), SetText(MK %d ký tự), OnLogin()"), szAccount, (int)strlen(szPassword));
		break;
	}
	code.End();

	BYTE buf[CRemoteCode::TOTAL_SIZE];
	code.Build(buf);
	SecureZeroMemory(szPassword, sizeof(szPassword));
	HANDLE hThread = NULL;
	if (WriteGameMemory(hGame, pRemote, buf, sizeof(buf)))
		hThread = CreateRemoteThread(hGame, NULL, 0, (LPTHREAD_START_ROUTINE)pRemote, NULL, 0, NULL);
	SecureZeroMemory(buf, sizeof(buf));
	if (!hThread)
	{
		strLog.Format(_T("LỖI: tạo thread trong Game thất bại (GetLastError = %d)"), GetLastError());
		AppendLog(strLog, CLR_ERROR, TRUE);
		VirtualFreeEx(hGame, pRemote, 0, MEM_RELEASE);
		CloseHandle(hGame);
		return;
	}

	DWORD dwWait = WaitForSingleObject(hThread, REPLY_TIMEOUT);
	DWORD dwExit = 0;
	GetExitCodeThread(hThread, &dwExit);
	CloseHandle(hThread);
	if (dwWait != WAIT_OBJECT_0)
	{
		// Thread chưa xong: không giải phóng (Game còn đang chạy đoạn mã này)
		strLog.Format(_T("LỖI: thread trong Game chưa kết thúc sau %d giây"), REPLY_TIMEOUT / 1000);
		AppendLog(strLog, CLR_ERROR, TRUE);
		CloseHandle(hGame);
		return;
	}
	// Xóa mật khẩu trong vùng nhớ đã ghi vào Game rồi mới giải phóng
	BYTE zero[CRemoteCode::TOTAL_SIZE] = { 0 };
	WriteGameMemory(hGame, pRemote, zero, sizeof(zero));
	VirtualFreeEx(hGame, pRemote, 0, MEM_RELEASE);

	strLog.Format(_T("%s  thread exit %u\n"), (LPCTSTR)strCall, dwExit);
	AppendText(strLog, CLR_TEXT);
	if (!(nCmd == dbg_lg_click && nIdx == 3))	// Thoát: Game đóng luôn
	{
		Sleep(300);
		LoginStatus(hGame);
	}
	if (dwExit)
		AppendLog(_T("=> OK"), CLR_OK, TRUE);
	else
	{
		strLog.Format(_T("=> LỖI: cửa sổ (*0x%08X) chưa mở lúc thread chạy"), dwSelfPtr);
		AppendLog(strLog, CLR_ERROR, TRUE);
	}
	CloseHandle(hGame);
}

void CDlgDebug::OnCmdButtonClicked(UINT nID)
{
	int i = nID - IDC_DBG_CMD_FIRST;
	if (i < 0 || i >= s_nButtonCount)
		return;

	const DEBUG_BUTTON& btn = s_Buttons[i];
	if (btn.bConfirm)
	{
		CString strMsg;
		strMsg.Format(_T("Chạy %s trong game?"), btn.szLabel);
		if (MessageBox(strMsg, _T("Debug"), MB_YESNO | MB_ICONQUESTION) != IDYES)
			return;
	}
	SendDebugCmd(btn.nCmd, btn.nCmd == dbg_mv_getmappos);
}

void CDlgDebug::OnBnClickedGetPos()
{
	SendDebugCmd(dbg_mv_getmappos, TRUE);
}

void CDlgDebug::OnBnClickedGetFlag()
{
	SendDebugCmd(dbg_mv_getflag, TRUE);
}

void CDlgDebug::OnBnClickedClear()
{
	m_Output.SetWindowText(_T(""));
}

void CDlgDebug::OnBnClickedCheckHook()
{
	CheckHook();
}

// Kiem tra truc tiep tu VLAuto (khong can VLHook tra loi):
// VLHookPr.dll co thuc su nam trong process Game khong, quyen cua 2 process
void CDlgDebug::CheckHook()
{
	UpdateTarget();
	AppendLog(_T("Kiểm tra VLHook"), CLR_CMD, TRUE);
	if (g_pCurGameWin == NULL || !g_pCurGameWin->m_hWnd || !::IsWindow(g_pCurGameWin->m_hWnd))
	{
		AppendText(_T("Chưa chọn nhân vật / cửa sổ game không còn\n\n"), CLR_ERROR, TRUE);
		return;
	}

	CString strOut, strLine;
	TCHAR szPath[MAX_PATH] = _T("");

	// DLL ma VLAuto dang dung (HookProc nap chinh file nay vao Game)
	CString strMyDll;
	HMODULE hMyDll = GetModuleHandle(_T("VLHookPr.dll"));
	if (hMyDll && GetModuleFileName(hMyDll, szPath, MAX_PATH))
		strMyDll = szPath;
	strLine.Format(_T("VLAuto: quyền %s\nVLHook của VLAuto: %s\n"), GetIntegrityName(GetCurrentProcess()), (LPCTSTR)strMyDll);
	strOut += strLine;

	DWORD dwPID = 0;
	::GetWindowThreadProcessId(g_pCurGameWin->m_hWnd, &dwPID);
	HANDLE hGame = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, dwPID);
	strLine.Format(_T("Game: PID %u  quyền %s  m_bAttached = %d\n"), dwPID,
		hGame ? GetIntegrityName(hGame) : _T("? (không mở được process)"), g_pCurGameWin->m_bAttached);
	strOut += strLine;
	if (hGame) CloseHandle(hGame);

	BOOL bFound = FALSE;
	HANDLE hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, dwPID);
	if (hSnap == INVALID_HANDLE_VALUE)
	{
		strLine.Format(_T("LỖI: không đọc được danh sách module của Game (GetLastError = %d)\n"), GetLastError());
		strOut += strLine;
	}
	else
	{
		MODULEENTRY32 me;
		me.dwSize = sizeof(me);
		for (BOOL bOK = Module32First(hSnap, &me); bOK; bOK = Module32Next(hSnap, &me))
		{
			if (!_tcsstr(me.szModule, _T("VLHook")))
				continue;
			bFound = TRUE;
			strLine.Format(_T("VLHook trong Game: %s  (base 0x%08X)%s\n"), me.szExePath, (UINT)(UINT_PTR)me.modBaseAddr,
				strMyDll.CompareNoCase(me.szExePath) == 0 ? _T("") : _T("  <== KHÁC file của VLAuto!"));
			strOut += strLine;
		}
		CloseHandle(hSnap);
	}

	if (!bFound)
	{
		strOut += _T("=> VLHookPr.dll KHÔNG có trong Game: Auto chưa thực sự gắn vào game.\n")
			_T("   (m_bAttached chỉ cho biết SetWindowsHookEx thành công.)\n")
			_T("   Bỏ chọn rồi chọn lại nhân vật để gắn lại; nếu vẫn không có thì game (chống hack) có thể đã chặn/gỡ DLL.\n\n");
		AppendText(strOut, CLR_ERROR);
		return;
	}
	strOut += _T("=> VLHook đã nạp vào Game. Bấm một nút lệnh để thử.\n\n");
	AppendText(strOut, CLR_OK);
}

BOOL CDlgDebug::OnCopyData(CWnd* pWnd, COPYDATASTRUCT* pCopyDataStruct)
{
	if (pCopyDataStruct->dwData != DEBUG_COPYDATA_ID)
		return CDialog::OnCopyData(pWnd, pCopyDataStruct);

	static DEBUG_REPLY reply;
	DWORD cbHeader = offsetof(DEBUG_REPLY, szText);
	if (pCopyDataStruct->cbData < cbHeader || pCopyDataStruct->cbData > sizeof(DEBUG_REPLY) || !pCopyDataStruct->lpData)
	{
		AppendLog(_T("LỖI: dữ liệu trả về không đúng kích thước (VLAuto & VLHook khác phiên bản?)"), CLR_ERROR, TRUE);
		return TRUE;
	}
	memset(&reply, 0, sizeof(reply));
	memcpy(&reply, pCopyDataStruct->lpData, pCopyDataStruct->cbData);
	reply.szText[DEBUG_TEXT_SIZE - 1] = 0;

	if (reply.nSeq == m_nWaitSeq)
		m_nWaitSeq = 0;

	AppendReplyText(reply.szText, reply.bOK ? CLR_TEXT : CLR_ERROR);

	CString strResult;
	strResult.Format(_T("#%d  %s  %s\n\n"), reply.nSeq, GetCmdLabel(reply.nCmd), reply.bOK ? _T("=> OK") : _T("=> LỖI"));
	AppendText(strResult, reply.bOK ? CLR_OK : CLR_ERROR, TRUE);

	if (reply.bOK && reply.nSeq == m_nFillPosSeq)
	{
		m_nFillPosSeq = 0;
		if (reply.nResult[0] > 0 && reply.nResult[1] > 0)
		{
			SetDlgItemInt(IDC_DBG_X_EDIT, reply.nResult[0], TRUE);
			SetDlgItemInt(IDC_DBG_Y_EDIT, reply.nResult[1], TRUE);
		}
	}

	BOOL bMoveCmd = (reply.nCmd == dbg_mv_moveto || reply.nCmd == dbg_mv_npcmoveto || reply.nCmd == dbg_mv_movetonpc || reply.nCmd == dbg_mv_goto);
	if (reply.bOK && bMoveCmd && IsDlgButtonChecked(IDC_DBG_AUTOPOS))
		SetTimer(TIMER_READPOS, 1000, NULL);

	return TRUE;
}

void CDlgDebug::OnTimer(UINT_PTR nIDEvent)
{
	if (nIDEvent == TIMER_READPOS)
	{
		KillTimer(TIMER_READPOS);
		SendDebugCmd(dbg_mv_getmappos);
		return;
	}

	if (nIDEvent == TIMER_CHECK)
	{
		if (IsWindowVisible())
			UpdateTarget();
		if (m_nWaitSeq && GetTickCount() - m_dwWaitTick > REPLY_TIMEOUT)
		{
			CString strLog;
			strLog.Format(_T("#%d  Không có phản hồi từ VLHook sau %d giây!"), m_nWaitSeq, REPLY_TIMEOUT / 1000);
			AppendLog(strLog, CLR_ERROR, TRUE);
			m_nWaitSeq = 0;
			CheckHook();
		}
		return;
	}

	CDialog::OnTimer(nIDEvent);
}
