#pragma once
// ============================================================================
// hamshack_tabs.hpp — Unified HamShack++ Top FLRig Deck & 5-Mode Tab Views
// Integrates FLRig (Xiegu G90 CAT), CoreAudio Soundcards, WSJT-X (FT8/FT4),
// JS8Call (Normal/Fast/Turbo/Slow), Fldigi (CW SOM/Keyer/Digiscope), and
// MeshCore++ (BLE NUS / LoRa Multi-Radio) in a single Apple Silicon app.
// Strictly live hardware data: zero fake or simulated rows.
// ============================================================================

#include "../includes/flrig_cat.hpp"
#include "soundcard_audio.hpp"
#include "../includes/modem_ft8_ft4.hpp"
#include "../includes/modem_js8.hpp"
#include "../includes/modem_cw.hpp"
#include "web_maps_mac.hpp"
#include "hamshack_config.hpp"
#include "irc_client.hpp"
#include "../includes/winlink_engine.hpp"
#include "twitch_streamer.hpp"
#include "../includes/dxcc_lookup.hpp"
#include "AudioRouter.hpp"
#include "AprsEngine.hpp"
#include "SstvEngine.hpp"
#include <memory>

namespace hamshack_ui {

enum HsHitId : int {
    HS_TAB_FT8    = 1000,
    HS_TAB_JS8    = 1001,
    HS_TAB_CW     = 1002,
    HS_TAB_MESH   = 1003,
    HS_TAB_GT     = 1004, // Standalone Grid Tracker Tab
    HS_TAB_PAGES  = 1005, // Standalone Pages Tab (Web Maps & QRZ)
    HS_TAB_IRC    = 1006, // Native IRC Client Tab (Libera.Chat)
    HS_TAB_WINLINK = 1007, // Winlink / Email Client Tab (B2F v3.2 & ARDOP)
    HS_TAB_CONFIG = 1008, // Configuration Tab (FLRig CAT / Soundcard / Logs)

    // Top FLRig CAT Deck & Virtual FLRig Front-Panel Window Controls
    HS_VFO_STEP_M10K = 1009,
    HS_VFO_STEP_P10K = 1010,
    HS_VFO_STEP_M1K  = 1011,
    HS_VFO_STEP_M100 = 1012,
    HS_VFO_STEP_P100 = 1013,
    HS_VFO_STEP_P1K  = 1014,
    HS_VFO_SELECT_A  = 1015,
    HS_VFO_SELECT_B  = 1016,
    HS_VFO_SWAP_AB   = 1017,
    HS_VFO_COPY_AB   = 1018,
    HS_VFO_SPLIT     = 1019,

    HS_MODE_CYCLE    = 1020,
    HS_FILT_CYCLE    = 1021,
    HS_PTT_TOGGLE    = 1022,
    HS_ATU_TUNE      = 1023,
    HS_PRE_TOGGLE    = 1024,
    HS_ATT_TOGGLE    = 1025,
    HS_NB_TOGGLE     = 1026,
    HS_COMP_TOGGLE   = 1027,
    HS_AGC_CYCLE     = 1028,
    HS_PWR_DOWN      = 1029,
    HS_PWR_UP        = 1030,
    HS_VOL_DOWN      = 1031,
    HS_VOL_UP        = 1032,
    HS_SQL_DOWN      = 1033,
    HS_SQL_UP        = 1034,
    HS_MODE_SET_LSB  = 1035,
    HS_MODE_SET_USB  = 1036,
    HS_MODE_SET_CW   = 1037,
    HS_MODE_SET_AM   = 1038,
    HS_ADIF_IMPORT_BTN = 1039,

    HS_BAND_BASE       = 1040, // 1040 .. 1050 (160m .. 6m)
    HS_RIG_MODEL_CYCLE = 1060,
    HS_CAT_PORT_CYCLE  = 1061,
    HS_AUDIO_IN_CYCLE  = 1062,
    HS_AUDIO_OUT_CYCLE = 1063,
    HS_CAT_RESCAN      = 1064,
    HS_HF_CALLSIGN_BOX = 1065,
    HS_HF_GRID_BOX     = 1066,
    HS_WF_RESIZE_BAR   = 1070,
    HS_WF_HEIGHT_MINUS = 1071,
    HS_WF_HEIGHT_PLUS  = 1072,
    HS_WF_PALETTE_CYCLE= 1073,
    HS_WF_GRID_TOGGLE  = 1074,
    HS_WF_POS_TOGGLE   = 1075,

    // Log QSO Modal Dialog Controls (WSJT-X Style)
    HS_LOG_QSO_OK      = 1080,
    HS_LOG_QSO_CANCEL  = 1081,
    HS_LOG_QSO_CALL    = 1082,
    HS_LOG_QSO_GRID    = 1083,
    HS_LOG_QSO_MODE    = 1084,
    HS_LOG_QSO_RPT_S   = 1085,
    HS_LOG_QSO_RPT_R   = 1086,
    HS_LOG_QSO_FREQ    = 1087,
    HS_LOG_QSO_PWR     = 1088,
    HS_LOG_QSO_COMM    = 1089,

    // FT8 / FT4 (WSJT-X) Tab Controls
    HS_FT_SUBMODE_FT8 = 1100,
    HS_FT_SUBMODE_FT4 = 1101,
    HS_FT_ENABLE_TX   = 1102,
    HS_FT_HALT_TX     = 1103,
    HS_FT_TUNE        = 1104,
    HS_FT_AUTO_SEQ    = 1105,
    HS_FT_TX_EVEN     = 1106,
    HS_FT_HOLD_TX     = 1107,
    HS_FT_MONITOR     = 1108,
    HS_FT_LOG_QSO     = 1109,
    HS_FT_ERASE       = 1110,
    HS_FT_DECODE_NOW  = 1111,
    HS_FT_WATERFALL   = 1112,
    HS_FT_GEN_STD     = 1113,
    HS_FT_DX_CALL_BOX = 1114,
    HS_FT_DX_GRID_BOX = 1115,
    HS_FT_TX_SLOT_BASE = 1120, // 1120 .. 1125 (Tx1..Tx6)
    HS_FT_BAND_ACT_ROW = 1130,
    HS_FT_RX_FREQ_ROW  = 1131,

    // JS8Call Tab Controls
    HS_JS8_SUB_NORMAL = 1200,
    HS_JS8_SUB_FAST   = 1201,
    HS_JS8_SUB_TURBO  = 1202,
    HS_JS8_SUB_SLOW   = 1203,
    HS_JS8_HB_TOGGLE  = 1204,
    HS_JS8_AUTO_TOGGLE= 1205,
    HS_JS8_MESH_BRIDGE= 1206,
    HS_JS8_DECODE_NOW = 1207,
    HS_JS8_COMPOSER   = 1208,
    HS_JS8_SEND_BTN   = 1209,
    HS_JS8_HALT_BTN   = 1210,
    HS_JS8_WATERFALL  = 1211,
    HS_JS8_CLEAR_BTN  = 1212,
    HS_JS8_CMD_CQ     = 1220,
    HS_JS8_CMD_HB     = 1221,
    HS_JS8_CMD_SNR    = 1222,
    HS_JS8_CMD_INFO   = 1223,
    HS_JS8_CMD_GRID   = 1224,
    HS_JS8_CMD_STATUS = 1225,
    HS_JS8_CMD_HEARING= 1226,
    HS_JS8_CMD_ACK    = 1227,
    HS_JS8_CMD_73     = 1228,
    HS_JS8_HEARD_ROW  = 1240,
    HS_JS8_FRAME_ROW  = 1241,

    // CW (Fldigi) Tab Controls
    HS_CW_MACRO_BASE  = 1300, // 1300 .. 1311 (F1..F12)
    HS_CW_WPM_M5      = 1320,
    HS_CW_WPM_M1      = 1321,
    HS_CW_WPM_P1      = 1322,
    HS_CW_WPM_P5      = 1323,
    HS_CW_PITCH_M25   = 1324,
    HS_CW_PITCH_P25   = 1325,
    HS_CW_BW_CYCLE    = 1326,
    HS_CW_QSK_TOGGLE  = 1327,
    HS_CW_AFC_TOGGLE  = 1328,
    HS_CW_PROSIGN_TOG = 1329,
    HS_CW_SQL_DOWN    = 1330,
    HS_CW_CLEAR_RX    = 1331,
    HS_CW_TX_NOW      = 1332,
    HS_CW_COMPOSER    = 1333,
    HS_CW_WATERFALL   = 1334,
    HS_CW_SQL_UP      = 1335,

    // FLRig CAT & Audio Matrix Tab Controls
    HS_MEM_CH_BASE    = 1400, // 1400 .. 1415
    HS_AUDIO_IN_ROW   = 1420,
    HS_AUDIO_OUT_ROW  = 1430,
    HS_AUDIO_TEST_TONE= 1440,
    HS_AUDIO_MON_TOG  = 1441,
    HS_AUDIO_REFRESH  = 1442,
    HS_CAT_POLL_VFO   = 1450,
    HS_CAT_POLL_SMETER= 1451,
    HS_CAT_HEX_SEND   = 1452,
    HS_CAT_HEX_BOX    = 1453,
    HS_CAT_CLEAR_LOG  = 1454,
    HS_CFG_WEB_REPORT_TOGGLE = 1460,

    // Pages & Web Browser Tab Controls (Tab 5)
    HS_PAGE_SUBTAB_BASE  = 1500, // 1500=QRZ, 1501=PSKReporter, 1502=DXView, 1503=DXLook, 1504=OpenHamClock
    HS_WEB_BACK          = 1510,
    HS_WEB_FWD           = 1511,
    HS_WEB_RELOAD        = 1512,
    HS_WEB_HOME          = 1513,
    HS_PAGE_BOOKMARK     = 1514,
    HS_WEB_EXTERNAL      = 1515,
    HS_PAGE_URL_BOX      = 1516,
    HS_PAGE_ZOOM_IN      = 1517,
    HS_PAGE_ZOOM_OUT     = 1518,
    HS_PAGE_ZOOM_RESET   = 1519,

    HS_GT_IMPORT_ADI     = 1520,
    HS_GT_SYNC_WSJTX_ADI = 1521,
    HS_GT_ROSTER_TOGGLE  = 1522,
    HS_GT_GRIDS_TOGGLE   = 1523,
    HS_GT_ARCS_TOGGLE    = 1524,
    HS_GT_GRAYLINE_TOGGLE= 1525,
    HS_GT_PINS_TOGGLE    = 1526,
    HS_GT_LEGEND_TOGGLE  = 1527,
    HS_GT_FIT_WORLD      = 1528,
    HS_GT_CENTER_QTH     = 1529,
    HS_GT_CLEAR_LIVE     = 1530,
    HS_GT_LOG_QSO        = 1531,
    HS_GT_FILT_BAND      = 1532,
    HS_GT_FILT_MODE      = 1533,
    HS_GT_FILT_DATA      = 1534,
    HS_GT_FILT_AWARD     = 1535,
    HS_GT_ZOOM_IN        = 1536,
    HS_GT_ZOOM_OUT       = 1537,
    HS_GT_MAP_CANVAS     = 1538,
    HS_GT_ROSTER_ROW     = 1540,

    HS_CONFIG_EXPORT_CSV = 1550,
    HS_CONFIG_EXPORT_KML = 1551,
    HS_CONFIG_EXPORT_ADI = 1552,

    HS_IRC_CHAN_BASE     = 1560, // 1560 .. 1579
    HS_IRC_NICK_BOX      = 1580,
    HS_IRC_INPUT_BOX     = 1581,
    HS_IRC_CONNECT_BTN   = 1582,
    HS_IRC_DISCONN_BTN   = 1583,
    HS_IRC_SEND_BTN      = 1584,
    HS_IRC_IDENT_BTN     = 1585,
    HS_IRC_SASL_TOGGLE   = 1586,
    HS_IRC_ZOOM_IN       = 1587,
    HS_IRC_ZOOM_OUT      = 1588,
    HS_IRC_ZOOM_RESET    = 1589,
    HS_IRC_MENU_OP       = 1590,
    HS_IRC_MENU_VOICE    = 1591,
    HS_IRC_MENU_WHOIS    = 1592,
    HS_IRC_MENU_QUERY    = 1593,
    HS_IRC_MENU_KICK     = 1594,
    HS_IRC_MENU_BAN      = 1595,
    HS_IRC_MENU_CLOSE    = 1596,
    HS_IRC_USER_ROW_BASE = 1740, // 1740 .. 1799

    // Winlink / Email Client Tab Controls (Tab 7)
    HS_WL_SUBTAB_MAILBOX = 1600,
    HS_WL_SUBTAB_GATEWAYS= 1601,
    HS_WL_SUBTAB_CONFIG  = 1602,
    HS_WL_FOLDER_INBOX   = 1603,
    HS_WL_FOLDER_OUTBOX  = 1604,
    HS_WL_FOLDER_SENT    = 1605,
    HS_WL_FOLDER_ARCHIVE = 1606,
    HS_WL_COMPOSE_BTN    = 1607,
    HS_WL_EXEC_SESSION   = 1608, // The Send & Receive button
    HS_WL_ABORT_SESSION  = 1609,
    HS_WL_MSG_ROW_BASE   = 1610, // 1610..1647 (Mailbox message row clicks)
    HS_WL_EXEC_WEB       = 1648, // Send & Receive via Web (CMS Telnet)
    HS_WL_EXEC_RADIO     = 1649, // Send & Receive via Radio (ARDOP HF)
    HS_WL_DELETE_MSG     = 1650,
    HS_WL_COMP_TO_BOX    = 1652,
    HS_WL_COMP_CC_BOX    = 1653,
    HS_WL_COMP_SUBJ_BOX  = 1654,
    HS_WL_COMP_BODY_BOX  = 1655,
    HS_WL_ATTACH_ADD_BTN = 1656,
    HS_WL_ATTACH_CLR_BTN = 1657,
    HS_WL_QUEUE_OUTBOX   = 1658,
    HS_WL_CANCEL_COMP    = 1659,
    HS_WL_GW_ROW_BASE    = 1660, // 1660..1699 (Gateway table row clicks)
    HS_WL_GW_SEARCH_BOX  = 1700,
    HS_WL_GW_MODE_ALL    = 1701,
    HS_WL_GW_MODE_ARDOP  = 1702,
    HS_WL_GW_MODE_VARA   = 1703,
    HS_WL_GW_MODE_PACKET = 1704,
    HS_WL_GW_MODE_PACTOR = 1705,
    HS_WL_GW_SCROLL_UP   = 1706,
    HS_WL_GW_SCROLL_DOWN = 1707,
    HS_WL_CFG_TRANS_TELNET = 1710,
    HS_WL_CFG_TRANS_ARDOP  = 1711,
    HS_WL_CFG_TRANS_PACKET = 1712,
    HS_WL_CFG_CALL_BOX     = 1713,
    HS_WL_CFG_PASS_BOX     = 1714,
    HS_WL_CFG_GRID_BOX     = 1715,
    HS_WL_CFG_BW_CYCLE     = 1716,
    HS_WL_CFG_SAVE_BTN     = 1717,

    // Twitch Live Streaming Studio
    HS_TWITCH_STREAM_TOGGLE = 1720,
    HS_TWITCH_LOGIN_WEB     = 1721,
    HS_TWITCH_DASHBOARD_WEB = 1722,
    HS_TWITCH_CHANNEL_BOX   = 1723,
    HS_TWITCH_KEY_BOX       = 1724,
    HS_TWITCH_SAVE_BTN      = 1725,

    // Carbon Engine Tabs: APRS (Tab 9) & SSTV (Tab 10) + Lock-Free AudioRouter Dropdown
    HS_TAB_APRS             = 1810,
    HS_TAB_SSTV             = 1811,
    HS_AUDIO_SRC_DROPDOWN   = 1812,
    HS_AUDIO_SRC_OPT_G90    = 1813,
    HS_AUDIO_SRC_OPT_DONGLE = 1814,
    HS_AUDIO_DONGLE_CYCLE   = 1815,

    // APRS Tab Controls (AX.25 1200-baud KISS-over-TCP / Dire Wolf + Map + 2-Way Messaging)
    HS_APRS_HOST_BOX        = 1820,
    HS_APRS_PORT_BOX        = 1821,
    HS_APRS_SSID_CYCLE      = 1822,
    HS_APRS_PATH_BOX        = 1823,
    HS_APRS_COMMENT_BOX     = 1824,
    HS_APRS_CONNECT_TOG     = 1825,
    HS_APRS_BEACON_TX       = 1826,
    HS_APRS_MSG_TO_BOX      = 1827,
    HS_APRS_MSG_TEXT_BOX    = 1828,
    HS_APRS_MSG_SEND        = 1829,
    HS_APRS_CLEAR_BTN       = 1830,
    HS_APRS_STATION_ROW     = 1831,

    // SSTV Tab Controls (14-Mode RX Demodulator + Phase-Continuous 48 kHz TX Studio)
    HS_SSTV_AUTO_VIS_TOG    = 1850,
    HS_SSTV_RX_MODE_PREV    = 1851,
    HS_SSTV_RX_MODE_NEXT    = 1852,
    HS_SSTV_FORCE_RX        = 1853,
    HS_SSTV_SLANT_M10       = 1854,
    HS_SSTV_SLANT_M1        = 1855,
    HS_SSTV_SLANT_ZERO      = 1856,
    HS_SSTV_SLANT_P1        = 1857,
    HS_SSTV_SLANT_P10       = 1858,
    HS_SSTV_CLEAR_RX        = 1859,
    HS_SSTV_AUTOSAVE_TOG    = 1860,
    HS_SSTV_SAVE_NOW        = 1861,
    HS_SSTV_VIEW_CYCLE      = 1862,
    HS_SSTV_TX_MODE_PREV    = 1863,
    HS_SSTV_TX_MODE_NEXT    = 1864,
    HS_SSTV_LOAD_IMG        = 1865,
    HS_SSTV_TEST_CARD       = 1866,
    HS_SSTV_DITHER_TOG      = 1867,
    HS_SSTV_OVERLAY_BOX     = 1868,
    HS_SSTV_SEND_TX         = 1869,
    HS_SSTV_ABORT_TX        = 1870
};

enum class HsTextFocus : int {
    None = 0,
    Ft8DxCall = 1,
    Ft8DxGrid = 2,
    Js8Composer = 3,
    CwComposer = 4,
    CatHexInput = 5,
    HfCallsign = 6,
    HfGrid = 7,
    IrcInput = 8,
    IrcNick = 9,
    PageUrl = 10,
    WlCompTo = 11,
    WlCompCc = 12,
    WlCompSubj = 13,
    WlCompBody = 14,
    WlGwSearch = 15,
    WlCfgCall = 16,
    WlCfgPass = 17,
    WlCfgGrid = 18,
    LogQsoCall = 19,
    LogQsoGrid = 20,
    LogQsoMode = 21,
    LogQsoRptS = 22,
    LogQsoRptR = 23,
    LogQsoFreq = 24,
    LogQsoPwr  = 25,
    LogQsoComm = 26,
    TwitchChannel = 27,
    TwitchStreamKey = 28,
    AprsHost = 29,
    AprsPort = 30,
    AprsPath = 31,
    AprsComment = 32,
    AprsMsgTo = 33,
    AprsMsgText = 34,
    SstvOverlay = 35
};

using LogQsoDialogState = hamshack::LogQsoDialogState;

struct HsHitRect {
    D2D1_RECT_F rect;
    int id;
    int index;
    std::string payload;
};

struct HamShackSuite {
    hamshack::FlrigCatEngine flrig;
    hamshack::SoundcardEngine soundcard;
    hamshack::Ft8Ft4Engine ft8;
    js8::JS8ModemEngine js8;
    cw_fldigi::CWModemEngine cw;
    hamshack::IrcClientEngine irc;
    winlink::WinlinkEngine winlink;
    std::unique_ptr<hamshack::carbon::AudioRouter> audio_router;
    std::unique_ptr<hamshack::carbon::AprsEngine> aprs;
    std::unique_ptr<hamshack::carbon::SstvEngine> sstv;

    // 0 = FT8/FT4 (WSJT-X), 1 = JS8Call, 2 = CW (Fldigi), 3 = MeshCore++ (BLE/LoRa),
    // 4 = Grid Tracker, 5 = Pages, 6 = IRC, 7 = Winlink/Email, 8 = Configuration,
    // 9 = APRS (KISS/AX.25), 10 = SSTV (14-Mode Transceiver)
    int active_mode_tab = 0;
    // Pages Sub-Tab: 0 = QRZ, 1 = pskreporter.info, 2 = hf.dxview.org, 3 = dxlook.com, 4 = openhamclock.com
    int active_page_subtab = 0;
    // Winlink Sub-Tab: 0 = Mailbox, 1 = Gateways (4800+), 2 = Configuration
    int active_winlink_subtab = 0;

    bool cmd_key_down = false;
    HsTextFocus text_focus = HsTextFocus::None;
    std::string hf_callsign = "W1AW";
    std::string hf_grid = "FN31";
    std::string irc_input_text;
    std::string irc_nick_edit = "W1AW";
    float irc_font_size = 13.0f;
    struct IrcContextMenuState {
        bool is_open = false;
        float x = 0;
        float y = 0;
        std::string nick;
        std::string channel;
        char prefix = 0;
    } irc_menu;
    std::string page_url_input;
    std::string config_export_status;
    LogQsoDialogState log_qso_dialog;

    float wide_graph_height = 225.0f;
    int wide_graph_pos = 0; // 0 = Top, 1 = Bottom
    int wf_color_scheme = 0; // 0=Aqua/Emerald, 1=Amber Phosphor, 2=Cyber Neon, 3=Monokai Matrix, 4=Solarized Sunset
    bool wf_show_signal_grid = true;
    bool dragging_wide_graph = false;
    bool wf_drag_invert = false;
    int wf_drag_start_y = 0;
    float wf_drag_start_h = 225.0f;
    std::string custom_cat_hex = "FE FE 88 E0 03 FD";
    int last_js8_slot_id = -1;
    hamshack::TwitchStreamer& twitch = hamshack::get_twitch_streamer();

    // GridTracker Interactive Map & Sidebar State
    bool gt_show_roster   = true;
    bool gt_show_grids    = true;
    bool gt_show_arcs     = true;
    bool gt_show_grayline = true;
    bool gt_show_pins     = true;
    bool gt_show_legend   = true;
    int gt_band_filter    = 0; // 0=All Bands, 1=Auto (Rig), 2=20m, 3=40m, 4=15m, 5=10m, 6=80m
    int gt_mode_filter    = 0; // 0=All Modes, 1=FT8/FT4, 2=JS8Call, 3=CW
    int gt_data_filter    = 0; // 0=Logbook (.ADI) & Live, 1=Live Only, 2=Logbook (.ADI) Only
    int gt_award_filter   = 0; // 0=Maidenhead Grids, 1=US States (WAS), 2=DXCC, 3=CQ Zones
    double gt_map_nx      = 0.22; // Centered around North America
    double gt_map_ny      = 0.36;
    int gt_map_zoom       = 3;
    bool gt_map_dragging  = false;
    POINT gt_map_drag_start{0, 0};
    double gt_map_drag_nx = 0.22;
    double gt_map_drag_ny = 0.36;
    D2D1_RECT_F gt_map_rect{0, 0, 0, 0};

    // Carbon Engine UI State (Decoupled from APRS & SSTV worker threads)
    bool audio_source_dropdown_open = false;
    D2D1_RECT_F audio_source_dropdown_anchor{0, 0, 0, 0};

    std::string aprs_host_edit = "127.0.0.1";
    std::string aprs_port_edit = "8001";
    int aprs_ssid = 9;
    std::string aprs_path_edit = "WIDE1-1,WIDE2-1";
    std::string aprs_comment_edit = "HamShack++ APRS";
    std::string aprs_msg_to_edit;
    std::string aprs_msg_text_edit;
    std::string aprs_selected_call;
    std::string aprs_status_banner = "Ready — Connect to Dire Wolf KISS TCP (127.0.0.1:8001)";

    int sstv_canvas_layout = 0; // 0 = Split RX + TX Preview, 1 = Full RX Scanline, 2 = Full TX Studio
    hamshack::carbon::SstvMode sstv_manual_mode = hamshack::carbon::SstvMode::Martin1;
    std::string sstv_overlay_edit = "W1AW  FN31";
    std::string sstv_status_banner = "Ready — Auto VIS Armed (14 Modes: Scottie, Martin, Robot B&W/Color, PD120)";

    std::vector<HsHitRect> hits;

    HamShackSuite() {
        auto& cfg = hamshack::get_app_config();
        hf_callsign = cfg.hf_callsign.empty() ? "W1AW" : cfg.hf_callsign;
        hf_grid = cfg.hf_grid.empty() ? "FN31" : cfg.hf_grid;
        cfg.hf_callsign = hf_callsign;
        cfg.hf_grid = hf_grid;
        cfg.winlink_callsign = hf_callsign;
        cfg.winlink_grid = hf_grid;
        active_mode_tab = 0; // ALWAYS FT8 (WSJT-X) first on opening no matter what!
        wide_graph_height = (cfg.wide_graph_height >= 110.0f && cfg.wide_graph_height <= 600.0f)
            ? (cfg.wide_graph_height <= 225.0f ? 268.0f : cfg.wide_graph_height)
            : 268.0f;
        wide_graph_pos = std::clamp(cfg.wide_graph_pos, 0, 1);
        wf_color_scheme = std::clamp(cfg.wf_color_scheme, 0, 8);
        wf_show_signal_grid = true;
        twitch.channel_name = cfg.twitch_channel;
        twitch.stream_key = cfg.twitch_stream_key;
        twitch.rtmp_url = cfg.twitch_rtmp_url;
        irc_nick_edit = cfg.irc_nick;
        irc_font_size = cfg.irc_font_size;
        irc.current_nick = cfg.irc_nick;
        irc.server = cfg.irc_server;
        irc.port = cfg.irc_port;
        irc.username = cfg.irc_user;
        irc.realname = cfg.irc_realname;
        winlink.filter_gateways("", "All", hf_grid);
        if (!cfg.winlink_selected_gateway.empty()) {
            winlink.selected_gateway_call = cfg.winlink_selected_gateway;
            winlink.selected_gateway_freq = cfg.winlink_selected_freq;
            winlink.selected_gateway_mode = cfg.winlink_selected_mode;
        }

        // Initialize Carbon Engine lock-free AudioRouter, APRS KISS/AX.25 Engine, and 14-Mode SSTV Transceiver
        audio_router = std::make_unique<hamshack::carbon::AudioRouter>();
        aprs = std::make_unique<hamshack::carbon::AprsEngine>();
        sstv = std::make_unique<hamshack::carbon::SstvEngine>(*audio_router);
        soundcard.attach_carbon_router(audio_router.get());

        aprs_host_edit = cfg.aprs_kiss_host.empty() ? "127.0.0.1" : cfg.aprs_kiss_host;
        aprs_port_edit = std::to_string(cfg.aprs_kiss_port > 0 ? cfg.aprs_kiss_port : 8001);
        aprs_ssid = std::clamp(cfg.aprs_ssid, 0, 15);
        aprs_path_edit = cfg.aprs_digi_path.empty() ? "WIDE1-1,WIDE2-1" : cfg.aprs_digi_path;
        aprs_comment_edit = cfg.aprs_beacon_comment.empty() ? "HamShack++ APRS" : cfg.aprs_beacon_comment;
        aprs->set_station_identity(hf_callsign, static_cast<uint8_t>(aprs_ssid), aprs_path_edit);
        aprs->set_kiss_endpoint(aprs_host_edit, static_cast<uint16_t>(std::clamp(cfg.aprs_kiss_port > 0 ? cfg.aprs_kiss_port : 8001, 1, 65535)));

        sstv_overlay_edit = cfg.sstv_overlay_text.empty() ? (hf_callsign + "  " + hf_grid) : cfg.sstv_overlay_text;
        sstv->set_auto_save_png(cfg.sstv_auto_save);
        sstv->set_tx_overlay_text(sstv_overlay_edit);

        flrig.start();
        soundcard.start();
        sstv->start_worker();
        winlink.set_radio_interfaces(&flrig, &soundcard);
        ft8.my_call = hf_callsign;
        ft8.my_grid = hf_grid;
        js8.my_call = hf_callsign;
        js8.my_grid = hf_grid;
        cw.my_call  = hf_callsign;
        ft8.regenerate_std_msgs();
        center_gt_map_on_qth(3);
    }

    std::string current_band_label() const {
        uint64_t hz = flrig.vfo_a_hz;
        if (hz >= 1800000 && hz <= 2000000) return "160m";
        if (hz >= 3500000 && hz <= 4000000) return "80m";
        if (hz >= 5300000 && hz <= 5410000) return "60m";
        if (hz >= 7000000 && hz <= 7300000) return "40m";
        if (hz >= 10100000 && hz <= 10150000) return "30m";
        if (hz >= 14000000 && hz <= 14350000) return "20m";
        if (hz >= 18068000 && hz <= 18168000) return "17m";
        if (hz >= 21000000 && hz <= 21450000) return "15m";
        if (hz >= 24890000 && hz <= 24990000) return "12m";
        if (hz >= 28000000 && hz <= 29700000) return "10m";
        if (hz >= 50000000 && hz <= 54000000) return "6m";
        char buf[32];
        std::snprintf(buf, sizeof(buf), "%.3fM", hz / 1e6);
        return buf;
    }

    ~HamShackSuite() {
        if (aprs) aprs->disconnect_kiss();
        if (sstv) sstv->stop_worker();
        soundcard.attach_carbon_router(nullptr);
        if (audio_router) audio_router->stop_usb_dongle_stream();
        irc.disconnect();
        soundcard.stop();
        flrig.stop();
    }

    void center_gt_map_on_qth(int zoom_lvl = 4) {
        double lat = 40.5, lon = -111.0;
        if (hamshack::Ft8Ft4Engine::grid_to_latlon(hf_grid, lat, lon)) {
            gt_map_nx = (lon + 180.0) / 360.0;
            double r_lat = std::clamp(lat, -85.0, 85.0) * 3.14159265358979323846 / 180.0;
            gt_map_ny = (1.0 - std::log(std::tan(rlat_clamp(r_lat)) + 1.0 / std::cos(rlat_clamp(r_lat))) / 3.14159265358979323846) / 2.0;
        }
        gt_map_zoom = std::clamp(zoom_lvl, 2, 10);
    }

    static double rlat_clamp(double r_lat) noexcept { return r_lat; }

    void add_hit(const D2D1_RECT_F& r, int id, int idx = 0, const std::string& payload = "") {
        hits.push_back({r, id, idx, payload});
    }

    void sync_hf_station() {
        if (hf_callsign.empty()) return;
        {
            std::lock_guard<std::recursive_mutex> lk(ft8.mtx);
            if (ft8.my_call != hf_callsign || ft8.my_grid != hf_grid) {
                ft8.my_call = hf_callsign;
                ft8.my_grid = hf_grid;
                ft8.regenerate_std_msgs_locked();
            }
        }
        {
            std::lock_guard<std::mutex> lk(js8.mtx);
            js8.my_call = hf_callsign;
            js8.my_grid = hf_grid;
        }
        {
            std::lock_guard<std::mutex> lk(cw.mtx);
            cw.my_call = hf_callsign;
        }
        if (aprs) {
            aprs->set_station_identity(hf_callsign, static_cast<uint8_t>(aprs_ssid), aprs_path_edit);
        }
    }

    void switch_mode_tab(int tab_idx) {
        int target = std::clamp(tab_idx, 0, 10);
        if (active_mode_tab == target) return;
        active_mode_tab = target;
        audio_source_dropdown_open = false;
        text_focus = HsTextFocus::None;
        auto& cfg = hamshack::get_app_config();
        cfg.last_main_tab = active_mode_tab;
        if (active_mode_tab != 5) {
            hamshack_web::get_web_maps().hide_all();
        }
        if (flrig.auto_band_mode_on_tab && flrig.connected && flrig.active_band_idx >= 0) {
            int band = flrig.active_band_idx;
            if (active_mode_tab == 0) {
                flrig.select_band_for_mode(band, ft8.is_ft4 ? 1 : 0);
            } else if (active_mode_tab == 1) {
                flrig.select_band_for_mode(band, 2);
            } else if (active_mode_tab == 2) {
                flrig.select_band_for_mode(band, 3);
            }
        }
    }

    void tick() {
        soundcard.current_band_label = current_band_label();
        bool want_ptt = false;
        ft8.tick(soundcard, want_ptt);
        twitch.tick();

        uint64_t dial_hz = flrig.connected ? flrig.vfo_a_hz : (ft8.is_ft4 ? 14080000ULL : 14074000ULL);
        ft8.cached_dial_hz.store(dial_hz);
        js8.cached_dial_hz.store(flrig.connected ? flrig.vfo_a_hz : 7078000ULL);

        if (ft8.qso_prompt_log_pending.exchange(false)) {
            if (!log_qso_dialog.is_open && !ft8.dx_call.empty()) {
                log_qso_dialog.open_for_current_ft8(ft8, dial_hz);
                text_focus = HsTextFocus::LogQsoComm;
            }
        }

        // Feed live soundcard audio into CW Goertzel/SOM demodulator (at true 100ms cadence) and async JS8 slot decoder
        if (soundcard.hw_stream_active && soundcard.rx_rms_db > -115.0f) {
            using namespace std::chrono;
            uint64_t now_ms = static_cast<uint64_t>(duration_cast<milliseconds>(steady_clock::now().time_since_epoch()).count());
            if ((active_mode_tab == 2 || cw.tx_active) && now_ms - last_cw_chunk_ms_ >= 95) {
                last_cw_chunk_ms_ = now_ms;
                auto chunk_100ms = soundcard.get_recent_rx_samples(1200);
                cw.process_rx_audio_chunk(chunk_100ms);
            }

            double epoch_s = duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count() / 1000.0;
            double period = js8::submode_params(js8.current_submode).period_sec;
            int js8_slot = static_cast<int>(std::floor(epoch_s / period));
            double in_slot = std::fmod(epoch_s, period);
            if (js8_slot != last_js8_slot_id && in_slot >= 0.4) {
                last_js8_slot_id = js8_slot;
                auto slot_pcm = soundcard.get_recent_rx_samples(static_cast<size_t>(period * 12000));
                js8.decode_live_audio_slot_async(std::move(slot_pcm));
            }
        }

        if (soundcard.is_tx_active()) {
            want_ptt = true;
        }
        const bool sstv_hf_tx = sstv && audio_router &&
                                sstv->telemetry().tx_active &&
                                audio_router->active_source() == hamshack::carbon::AudioSource::PrimaryG90;
        if (sstv_hf_tx) {
            want_ptt = true;
        }
        if (want_ptt != flrig.ptt_active &&
            (ft8.tx_enabled || ft8.tune_active || js8.tx_active || cw.tx_active || sstv_hf_tx || sstv_was_ptt_)) {
            flrig.set_ptt(want_ptt);
            if (!want_ptt) {
                js8.tx_active = false;
                cw.tx_active = false;
            }
        }
        sstv_was_ptt_ = sstv_hf_tx;

        // Drain lock-free APRS events into decoupled GUI-owned tables & shared Map Engine
        if (aprs) {
            size_t n_ev = aprs->poll_ui_events([this](const hamshack::carbon::AprsStationEntry& st) {
                aprs_status_banner = "GPS Tracked: " + st.callsign_ssid + " (" + st.last_utc + ")";
            });
            if (n_ev == 0 && aprs->is_running()) {
                if (aprs->is_connected()) {
                    if (aprs_status_banner.rfind("Ready", 0) == 0 || aprs_status_banner.rfind("Connecting", 0) == 0) {
                        aprs_status_banner = "Connected to KISS TNC (" + aprs->host() + ":" + std::to_string(aprs->port()) + ")";
                    }
                }
            }
        }
    }

private:
    uint64_t last_cw_chunk_ms_ = 0;
    bool sstv_was_ptt_ = false;
};

// Map waterfall intensity (0..255) to 5 selectable high-contrast color palettes (Requirement 5)
// 0=Classic Aqua/Emerald/Gold, 1=Amber Phosphor, 2=Cyber Neon, 3=Monokai Matrix, 4=Solarized Sunset
inline D2D1_COLOR_F waterfall_heat_color(uint8_t val, bool light_mode, int scheme = 0) {
    if (val == 0) return D2D1::ColorF(0.01f, 0.02f, 0.04f, 1.0f); // Clean pitch black noise floor
    float t = val / 255.0f;
    if (light_mode) {
        if (t < 0.20f) {
            float u = t / 0.20f;
            return D2D1::ColorF(0.96f - 0.08f * u, 0.97f - 0.06f * u, 0.98f, 1.0f);
        } else if (t < 0.50f) {
            float u = (t - 0.20f) / 0.30f;
            return D2D1::ColorF(0.70f - 0.60f * u, 0.85f - 0.15f * u, 0.98f - 0.10f * u, 1.0f);
        } else if (t < 0.80f) {
            float u = (t - 0.50f) / 0.30f;
            return D2D1::ColorF(0.10f + 0.80f * u, 0.70f + 0.10f * u, 0.88f - 0.70f * u, 1.0f);
        } else {
            float u = (t - 0.80f) / 0.20f;
            return D2D1::ColorF(0.92f + 0.08f * u, 0.80f - 0.60f * u, 0.18f - 0.15f * u, 1.0f);
        }
    }

    scheme = std::clamp(scheme, 0, 8);

    if (scheme == 1) {
        // 1: Amber Phosphor: Deep mahogany ember -> Warm golden amber -> Radiant molten gold -> Bright cadmium yellow -> Incandescent white
        if (t < 0.12f) {
            float u = t / 0.12f;
            return D2D1::ColorF(0.15f + 0.20f * u, 0.04f + 0.06f * u, 0.01f, 1.0f);
        } else if (t < 0.35f) {
            float u = (t - 0.12f) / 0.23f;
            return D2D1::ColorF(0.35f + 0.52f * u, 0.10f + 0.30f * u, 0.01f + 0.02f * u, 1.0f);
        } else if (t < 0.65f) {
            float u = (t - 0.35f) / 0.30f;
            return D2D1::ColorF(0.87f + 0.11f * u, 0.40f + 0.32f * u, 0.03f + 0.04f * u, 1.0f);
        } else if (t < 0.86f) {
            float u = (t - 0.65f) / 0.21f;
            return D2D1::ColorF(0.98f + 0.02f * u, 0.72f + 0.23f * u, 0.07f + 0.18f * u, 1.0f);
        } else {
            float u = (t - 0.86f) / 0.14f;
            return D2D1::ColorF(1.0f, 0.95f + 0.05f * u, 0.25f + 0.75f * u, 1.0f);
        }
    } else if (scheme == 2) {
        // 2: Cyber Neon: Deep night indigo -> Electric neon magenta -> Laser hot pink -> Electric cyan flare -> Brilliant pure white
        if (t < 0.12f) {
            float u = t / 0.12f;
            return D2D1::ColorF(0.12f + 0.15f * u, 0.02f + 0.03f * u, 0.30f + 0.25f * u, 1.0f);
        } else if (t < 0.35f) {
            float u = (t - 0.12f) / 0.23f;
            return D2D1::ColorF(0.27f + 0.60f * u, 0.05f + 0.04f * u, 0.55f + 0.30f * u, 1.0f);
        } else if (t < 0.65f) {
            float u = (t - 0.35f) / 0.30f;
            return D2D1::ColorF(0.87f + 0.11f * u, 0.09f + 0.15f * u, 0.85f - 0.15f * u, 1.0f);
        } else if (t < 0.86f) {
            float u = (t - 0.65f) / 0.21f;
            return D2D1::ColorF(0.98f - 0.40f * u, 0.24f + 0.68f * u, 0.70f + 0.30f * u, 1.0f);
        } else {
            float u = (t - 0.86f) / 0.14f;
            return D2D1::ColorF(0.58f + 0.42f * u, 0.92f + 0.08f * u, 1.0f, 1.0f);
        }
    } else if (scheme == 3) {
        // 3: Monokai Matrix: Deep evergreen -> Vivid CRT phosphor green -> Intense chartreuse lime -> Sunburst yellow -> Crisp white-hot green
        if (t < 0.12f) {
            float u = t / 0.12f;
            return D2D1::ColorF(0.01f + 0.03f * u, 0.14f + 0.18f * u, 0.03f + 0.03f * u, 1.0f);
        } else if (t < 0.35f) {
            float u = (t - 0.12f) / 0.23f;
            return D2D1::ColorF(0.04f + 0.08f * u, 0.32f + 0.55f * u, 0.06f + 0.10f * u, 1.0f);
        } else if (t < 0.65f) {
            float u = (t - 0.35f) / 0.30f;
            return D2D1::ColorF(0.12f + 0.58f * u, 0.87f + 0.11f * u, 0.16f - 0.05f * u, 1.0f);
        } else if (t < 0.86f) {
            float u = (t - 0.65f) / 0.21f;
            return D2D1::ColorF(0.70f + 0.28f * u, 0.98f, 0.11f + 0.18f * u, 1.0f);
        } else {
            float u = (t - 0.86f) / 0.14f;
            return D2D1::ColorF(0.98f + 0.02f * u, 0.98f + 0.02f * u, 0.29f + 0.71f * u, 1.0f);
        }
    } else if (scheme == 4) {
        // 4: Solarized Sunset: Deep cobalt abyss -> Fiery ruby crimson -> Radiant coral orange -> Golden sunset yellow -> Supernova white-gold
        if (t < 0.12f) {
            float u = t / 0.12f;
            return D2D1::ColorF(0.02f + 0.05f * u, 0.06f + 0.08f * u, 0.25f + 0.30f * u, 1.0f);
        } else if (t < 0.35f) {
            float u = (t - 0.12f) / 0.23f;
            return D2D1::ColorF(0.07f + 0.78f * u, 0.14f - 0.06f * u, 0.55f - 0.25f * u, 1.0f);
        } else if (t < 0.65f) {
            float u = (t - 0.35f) / 0.30f;
            return D2D1::ColorF(0.85f + 0.14f * u, 0.08f + 0.44f * u, 0.30f - 0.22f * u, 1.0f);
        } else if (t < 0.86f) {
            float u = (t - 0.65f) / 0.21f;
            return D2D1::ColorF(0.99f + 0.01f * u, 0.52f + 0.38f * u, 0.08f + 0.06f * u, 1.0f);
        } else {
            float u = (t - 0.86f) / 0.14f;
            return D2D1::ColorF(1.0f, 0.90f + 0.10f * u, 0.14f + 0.86f * u, 1.0f);
        }
    } else if (scheme == 5) {
        // 5: Icom IC-7300 Waterfall: Deep navy indigo -> Crisp sky blue -> Vivid emerald green -> Solar yellow -> Rich crimson red -> Peak white
        if (t < 0.15f) {
            float u = t / 0.15f;
            return D2D1::ColorF(0.02f + 0.05f * u, 0.05f + 0.12f * u, 0.35f + 0.25f * u, 1.0f);
        } else if (t < 0.35f) {
            float u = (t - 0.15f) / 0.20f;
            return D2D1::ColorF(0.07f + 0.08f * u, 0.50f + 0.35f * u, 0.90f + 0.08f * u, 1.0f);
        } else if (t < 0.60f) {
            float u = (t - 0.35f) / 0.25f;
            return D2D1::ColorF(0.15f + 0.35f * u, 0.88f + 0.08f * u, 0.15f - 0.10f * u, 1.0f);
        } else if (t < 0.80f) {
            float u = (t - 0.60f) / 0.20f;
            return D2D1::ColorF(0.92f + 0.06f * u, 0.88f + 0.08f * u, 0.05f + 0.05f * u, 1.0f);
        } else if (t < 0.93f) {
            float u = (t - 0.80f) / 0.13f;
            return D2D1::ColorF(0.95f + 0.05f * u, 0.15f - 0.08f * u, 0.10f, 1.0f);
        } else {
            return D2D1::ColorF(1.0f, 1.0f, 1.0f, 1.0f);
        }
    } else if (scheme == 6) {
        // 6: Xiegu G90 Hardware LCD: Obsidian blue -> Saturated electric cyan -> Neon lime -> G90 orange-amber -> Hot magenta -> Incandescent solar white
        if (t < 0.12f) {
            float u = t / 0.12f;
            return D2D1::ColorF(0.01f + 0.02f * u, 0.03f + 0.08f * u, 0.20f + 0.30f * u, 1.0f);
        } else if (t < 0.32f) {
            float u = (t - 0.12f) / 0.20f;
            return D2D1::ColorF(0.02f, 0.70f + 0.25f * u, 0.90f + 0.08f * u, 1.0f);
        } else if (t < 0.58f) {
            float u = (t - 0.32f) / 0.26f;
            return D2D1::ColorF(0.10f + 0.40f * u, 0.95f + 0.04f * u, 0.10f + 0.05f * u, 1.0f);
        } else if (t < 0.78f) {
            float u = (t - 0.58f) / 0.20f;
            return D2D1::ColorF(0.98f, 0.55f + 0.30f * u, 0.05f, 1.0f);
        } else if (t < 0.92f) {
            float u = (t - 0.78f) / 0.14f;
            return D2D1::ColorF(0.92f + 0.06f * u, 0.10f + 0.15f * u, 0.75f + 0.20f * u, 1.0f);
        } else {
            return D2D1::ColorF(1.0f, 0.98f, 0.90f, 1.0f);
        }
    } else if (scheme == 7) {
        // 7: Aurora Borealis: Arctic night -> Ghostly mint -> Luminous aurora green -> Mystic amethyst violet -> Pearlescent diamond ice
        if (t < 0.14f) {
            float u = t / 0.14f;
            return D2D1::ColorF(0.02f + 0.03f * u, 0.02f + 0.05f * u, 0.18f + 0.22f * u, 1.0f);
        } else if (t < 0.38f) {
            float u = (t - 0.14f) / 0.24f;
            return D2D1::ColorF(0.05f + 0.10f * u, 0.65f + 0.30f * u, 0.60f + 0.20f * u, 1.0f);
        } else if (t < 0.65f) {
            float u = (t - 0.38f) / 0.27f;
            return D2D1::ColorF(0.18f + 0.22f * u, 0.96f, 0.35f - 0.15f * u, 1.0f);
        } else if (t < 0.88f) {
            float u = (t - 0.65f) / 0.23f;
            return D2D1::ColorF(0.72f + 0.20f * u, 0.20f + 0.30f * u, 0.95f + 0.05f * u, 1.0f);
        } else {
            return D2D1::ColorF(0.95f, 0.98f, 1.0f, 1.0f);
        }
    } else if (scheme == 8) {
        // 8: Inferno Flame: Charcoal smoke -> Deep blood ruby -> Molten lava orange -> Sunburst lemon yellow -> Supernova white flame
        if (t < 0.14f) {
            float u = t / 0.14f;
            return D2D1::ColorF(0.08f + 0.25f * u, 0.02f + 0.02f * u, 0.02f, 1.0f);
        } else if (t < 0.40f) {
            float u = (t - 0.14f) / 0.26f;
            return D2D1::ColorF(0.70f + 0.25f * u, 0.04f + 0.10f * u, 0.03f, 1.0f);
        } else if (t < 0.70f) {
            float u = (t - 0.40f) / 0.30f;
            return D2D1::ColorF(0.98f + 0.02f * u, 0.42f + 0.45f * u, 0.04f + 0.04f * u, 1.0f);
        } else if (t < 0.90f) {
            float u = (t - 0.70f) / 0.20f;
            return D2D1::ColorF(1.0f, 0.88f + 0.10f * u, 0.12f + 0.45f * u, 1.0f);
        } else {
            return D2D1::ColorF(1.0f, 1.0f, 0.95f, 1.0f);
        }
    }

    // Default Scheme 0: Classic Aqua / Emerald / Gold (Vivid Multi-Color Spectral Heat)
    if (t < 0.12f) {
        // Weakest signals: Deep electric royal sapphire blue
        float u = t / 0.12f;
        return D2D1::ColorF(0.02f + 0.03f * u, 0.10f + 0.25f * u, 0.35f + 0.35f * u, 1.0f);
    } else if (t < 0.35f) {
        // Weak-Medium signals: Electric glowing cyan / turquoise
        float u = (t - 0.12f) / 0.23f;
        return D2D1::ColorF(0.05f - 0.03f * u, 0.35f + 0.52f * u, 0.70f + 0.28f * u, 1.0f);
    } else if (t < 0.65f) {
        // Medium-Strong signals: Radiant neon mint / emerald green
        float u = (t - 0.35f) / 0.30f;
        return D2D1::ColorF(0.02f + 0.20f * u, 0.87f + 0.11f * u, 0.98f - 0.55f * u, 1.0f);
    } else if (t < 0.86f) {
        // Strong signals: Blazing solar yellow / bright gold
        float u = (t - 0.65f) / 0.21f;
        return D2D1::ColorF(0.22f + 0.76f * u, 0.98f - 0.08f * u, 0.43f - 0.35f * u, 1.0f);
    } else {
        // Super-strong signals: Incandescent hot white core with cyan corona
        float u = (t - 0.86f) / 0.14f;
        return D2D1::ColorF(0.98f + 0.02f * u, 0.90f + 0.10f * u, 0.08f + 0.92f * u, 1.0f);
    }
}

template <typename RendererT>
inline void draw_hs_btn(RendererT& r, HamShackSuite& hs, POINT mouse_pt,
                        float x, float y, float w, float h,
                        int hit_id, const std::string& label,
                        bool active = false, uint32_t accent_hex = 0x00E5FF,
                        int hit_idx = 0, const std::string& payload = "",
                        bool small_font = false) {
    D2D1_RECT_F rc = D2D1::RectF(x, y, x + w, y + h);
    bool hov = RendererT::pt_in_rect(mouse_pt, rc);
    D2D1_COLOR_F bg = active ? r.hex_color(accent_hex, 0.24f)
                    : hov    ? r.hex_color(0x1E293B)
                             : r.hex_color(0x131C2E);
    D2D1_COLOR_F brd = active ? r.hex_color(accent_hex)
                     : hov    ? r.hex_color(0x475569)
                              : r.hex_color(0x26354D);
    r.fill_round_rect(rc, 5.0f, bg);
    r.draw_round_rect(rc, 5.0f, brd, active ? 1.5f : 1.0f);
    r.draw_text(label, small_font ? r.font_small : r.font_ui_bold, rc,
                active ? r.hex_color(accent_hex) : r.hex_color(0xE2E8F0),
                DWRITE_TEXT_ALIGNMENT_CENTER);
    hs.add_hit(rc, hit_id, hit_idx, payload);
}

// ============================================================================
// 12-Cell Wide Live Xiegu G90 & USB Soundcard Telemetry Grid Strip
// ============================================================================
template <typename RendererT>
inline void render_live_g90_telemetry_grid(RendererT& r, HamShackSuite& hs,
                                           float x, float y, float w, float h) {
    D2D1_RECT_F strip = D2D1::RectF(x, y, x + w, y + h);
    r.fill_round_rect(strip, 6.0f, r.hex_color(0x0B111E));
    r.draw_round_rect(strip, 6.0f, hs.flrig.connected ? r.hex_color(0x00E5FF, 0.55f) : r.hex_color(0xEF4444, 0.65f), 1.1f);

    bool conn = false, use_b = false, split_on = false, ptt_on = false, atu_on = false, nb = false, comp = false;
    uint64_t vfo_a = 0, vfo_b = 0, rx_cnt = 0, tx_cnt = 0;
    hamshack::RigMode mode_a = hamshack::RigMode::USB;
    int filt_a = 1, band_idx = -1, smeter = 0, smeter_raw = 0, smeter_dbm = -128;
    int pwr_set = 0, vol = 0, sql = 0, agc = 1, pre = 0, att = 0;
    float pwr_out = 0.0f, swr = 0.0f;
    std::string port_str;
    {
        std::lock_guard<std::mutex> lk(hs.flrig.mtx);
        conn = hs.flrig.connected;
        use_b = hs.flrig.active_vfo_b;
        split_on = hs.flrig.split_enabled;
        ptt_on = hs.flrig.ptt_active;
        atu_on = hs.flrig.atu_tuning;
        nb = hs.flrig.nb_enabled;
        comp = hs.flrig.comp_enabled;
        vfo_a = hs.flrig.vfo_a_hz;
        vfo_b = hs.flrig.vfo_b_hz;
        mode_a = use_b ? hs.flrig.mode_b : hs.flrig.mode_a;
        filt_a = use_b ? hs.flrig.filter_b : hs.flrig.filter_a;
        band_idx = hs.flrig.active_band_idx;
        smeter = hs.flrig.smeter_pct;
        smeter_raw = hs.flrig.smeter_raw_bcd;
        smeter_dbm = hs.flrig.smeter_dbm;
        pwr_set = hs.flrig.power_watts;
        vol = hs.flrig.volume_pct;
        sql = hs.flrig.squelch_pct;
        agc = hs.flrig.agc_mode;
        pre = hs.flrig.preamp_level;
        att = hs.flrig.atten_level;
        pwr_out = hs.flrig.power_out_watts;
        swr = hs.flrig.swr_ratio;
        rx_cnt = hs.flrig.rx_frame_count;
        tx_cnt = hs.flrig.tx_frame_count;
        port_str = hs.flrig.serial_port;
    }

    float rx_db = -120.0f;
    int peak_hz = 0;
    std::string aud_in;
    {
        std::lock_guard<std::mutex> lk(hs.soundcard.mtx);
        rx_db = hs.soundcard.rx_rms_db;
        peak_hz = hs.soundcard.peak_freq_hz;
        aud_in = hs.soundcard.active_input_name;
    }
    if (aud_in.size() > 16) aud_in = aud_in.substr(0, 16);

    struct CellData {
        const char* hdr;
        std::string val;
        uint32_t color;
    };

    std::string short_port = port_str;
    if (short_port.rfind("/dev/cu.", 0) == 0) short_port = short_port.substr(8);
    if (short_port.empty()) short_port = "NONE";

    char c_smeter[48], c_dbm[48], c_tx[48], c_vol[48], c_fp[64], c_aud[64], c_civ[48];
    if (conn) {
        std::snprintf(c_smeter, sizeof(c_smeter), "%s (BCD %03d)", hamshack::FlrigCatEngine::smeter_s_unit_str(smeter).c_str(), smeter_raw);
        std::snprintf(c_dbm,    sizeof(c_dbm),    "%+d dBm (%d%%)", smeter_dbm, smeter);
        if (ptt_on) std::snprintf(c_tx, sizeof(c_tx), "TX %.1fW SWR %.2f", pwr_out, swr);
        else        std::snprintf(c_tx, sizeof(c_tx), "RX (Set %dW)", pwr_set);
        std::snprintf(c_vol, sizeof(c_vol), "Vol %d%% SQL %d%%", vol, sql);
        std::snprintf(c_fp,  sizeof(c_fp),  "%s %s %s %s",
                      hamshack::FlrigCatEngine::agc_label(agc),
                      pre ? "PRE" : "---", att ? "ATT" : "---", nb ? "NB" : "--");
        std::snprintf(c_civ, sizeof(c_civ), "RX:%llu TX:%llu",
                      static_cast<unsigned long long>(rx_cnt), static_cast<unsigned long long>(tx_cnt));
    } else {
        std::snprintf(c_smeter, sizeof(c_smeter), "OFFLINE");
        std::snprintf(c_dbm,    sizeof(c_dbm),    "--- dBm");
        std::snprintf(c_tx,     sizeof(c_tx),     "OFFLINE");
        std::snprintf(c_vol,    sizeof(c_vol),    "Vol -- SQL --");
        std::snprintf(c_fp,     sizeof(c_fp),     "OFFLINE");
        std::snprintf(c_civ,    sizeof(c_civ),    "NO CI-V REPLY");
    }

    if (rx_db > -115.0f) {
        std::snprintf(c_aud, sizeof(c_aud), "%.1fdBFS @%dHz", rx_db, peak_hz);
    } else {
        std::snprintf(c_aud, sizeof(c_aud), "SILENT");
    }

    std::string band_str = (conn && band_idx >= 0 && band_idx < 11)
        ? std::string(hamshack::kBandPresets[band_idx].label) + " (" + hamshack::FlrigCatEngine::mode_name(mode_a) + " F" + std::to_string(filt_a) + ")"
        : (conn ? (std::string("GEN (") + hamshack::FlrigCatEngine::mode_name(mode_a) + ")") : "OFFLINE");

    CellData cells[10] = {
        {"G90 CAT PORT",   conn ? ("ONLINE " + short_port) : ("OFFLINE " + short_port), conn ? 0x10B981u : 0xEF4444u},
        {"VFO A DIAL",     conn ? (hamshack::FlrigCatEngine::format_freq_mhz(vfo_a) + " MHz") : "---.---.--- MHz", conn ? 0x00E5FFu : 0x64748Bu},
        {"VFO B / SPLIT",  conn ? (hamshack::FlrigCatEngine::format_freq_mhz(vfo_b) + (split_on ? " [SPL]" : "")) : "---.---.---", conn ? 0x38BDF8u : 0x64748Bu},
        {"BAND / MODE",    band_str, conn ? 0x34D399u : 0x64748Bu},
        {"LIVE S-METER",   c_smeter, conn ? 0x10B981u : 0x64748Bu},
        {"SIGNAL LEVEL",   c_dbm,    conn ? 0x38BDF8u : 0x64748Bu},
        {"TX / RF POWER",  c_tx,     ptt_on ? 0xEF4444u : (conn ? 0xFBBF24u : 0x64748Bu)},
        {"AF VOL / SQL",   c_vol,    conn ? 0xE2E8F0u : 0x64748Bu},
        {"G90 FRONT PANEL",c_fp,     conn ? 0xA855F7u : 0x64748Bu},
        {"USB AUDIO DSP",  aud_in + ": " + c_aud, rx_db > -115.0f ? 0x34D399u : 0x94A3B8u}
    };

    float cw = w / 10.0f;
    for (int i = 0; i < 10; ++i) {
        float cx = x + i * cw;
        if (i > 0) {
            r.fill_round_rect(D2D1::RectF(cx, y + 3, cx + 1.0f, y + h - 3), 0.0f, r.hex_color(0x1E293B));
        }
        r.draw_text(cells[i].hdr, r.font_small, D2D1::RectF(cx + 6, y + 2, cx + cw - 4, y + 16), r.hex_color(0x94A3B8));
        r.draw_text(cells[i].val, r.font_mono_small, D2D1::RectF(cx + 6, y + 16, cx + cw - 4, y + h - 2), r.hex_color(cells[i].color));
    }
}

// ============================================================================
// Shared Wide-Graph FFT Waterfall + Live Spectrum Widget (Resizable)
// ============================================================================
template <typename RendererT>
inline void render_wide_graph_splitter(RendererT& r, HamShackSuite& hs, POINT mouse_pt,
                                       float x, float y, float w, bool invert_drag) {
    D2D1_RECT_F sp_rc = D2D1::RectF(x, y, x + w, y + 8.0f);
    bool hov = RendererT::pt_in_rect(mouse_pt, sp_rc) || hs.dragging_wide_graph;
    r.fill_round_rect(sp_rc, 3.0f, hov ? r.hex_color(0x00E5FF, 0.25f) : r.hex_color(0x0F172A));
    r.draw_round_rect(sp_rc, 3.0f, hov ? r.hex_color(0x00E5FF) : r.hex_color(0x26354D), 1.0f);
    float grip_w = 260.0f;
    float gx = x + (w - grip_w) * 0.5f;
    // Elegant grip bar without the "DRAG TO RESIZE WIDE GRAPH" text (Requirement 4)
    r.draw_text("\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90",
                r.font_mono_small, D2D1::RectF(gx, y - 2.0f, gx + grip_w, y + 9.0f),
                hov ? r.hex_color(0x00E5FF) : r.hex_color(0x64748B), DWRITE_TEXT_ALIGNMENT_CENTER);
    hs.add_hit(sp_rc, HS_WF_RESIZE_BAR, invert_drag ? 1 : 0);
}

template <typename RendererT>
inline void render_waterfall_widget(RendererT& r, HamShackSuite& hs, POINT mouse_pt,
                                    float x, float y, float w, float h,
                                    int rx_hz, int tx_hz, int bw_hz,
                                    int hit_id, const std::string& /*title_str*/) {
    D2D1_RECT_F card = D2D1::RectF(x, y, x + w, y + h);
    r.fill_round_rect(card, 8.0f, r.hex_color(0x040711));
    r.draw_round_rect(card, 8.0f, r.hex_color(0x1E293B), 1.2f);

    float ruler_h = 24.0f;
    D2D1_RECT_F ruler_r = D2D1::RectF(x + 2, y + 2, x + w - 2, y + ruler_h);

    float plot_x = x + 8.0f;
    float plot_w = std::max(100.0f, w - 16.0f);
    auto hz_to_x = [&](float hz) -> float {
        float norm = std::clamp((hz - 100.0f) / 2800.0f, 0.0f, 1.0f);
        return plot_x + norm * plot_w;
    };

    // Render horizontal intensity gradient across the top frequency bar:
    // Intense luminous focus where the center of the frequencies is (1500 Hz) and smoothly fading out to edges
    int ruler_segs = 128;
    float seg_w = (w - 4.0f) / static_cast<float>(ruler_segs);
    for (int s = 0; s < ruler_segs; ++s) {
        float sx = x + 2.0f + s * seg_w;
        float norm_pos = std::clamp((sx + seg_w * 0.5f - plot_x) / plot_w, 0.0f, 1.0f);
        float dist = std::abs(norm_pos - 0.5f) * 2.0f; // 0.0 at center (1500 Hz), 1.0 at edges
        float glow = std::exp(-dist * dist * 3.0f); // Intense center peak fading out gently

        // Base glass fill: intense deep sapphire/cyan at center fading to sleek obsidian dark slate at edges
        D2D1_COLOR_F seg_bg = D2D1::ColorF(
            0.035f + 0.085f * glow,
            0.055f + 0.165f * glow,
            0.105f + 0.285f * glow,
            1.0f
        );
        r.fill_round_rect(D2D1::RectF(sx, y + 2, sx + seg_w + 0.5f, y + ruler_h), 0.0f, seg_bg);

        // Radiant luminous bottom beam line (intense at center, fading out to edges)
        D2D1_COLOR_F beam_col = D2D1::ColorF(
            0.02f + 0.10f * glow,
            0.45f + 0.55f * glow,
            0.75f + 0.25f * glow,
            0.18f + 0.82f * glow
        );
        r.fill_round_rect(D2D1::RectF(sx, y + ruler_h - 1.5f, sx + seg_w + 0.5f, y + ruler_h), 0.0f, beam_col);
    }

    // Floating Grid / Signal Overlay Toggle Button
    draw_hs_btn(r, hs, mouse_pt, x + w - 326.0f, y + 2.0f, 62.0f, 17.0f, HS_WF_GRID_TOGGLE,
                hs.wf_show_signal_grid ? "▦ Grid: ON" : "⬚ Grid: OFF",
                hs.wf_show_signal_grid, hs.wf_show_signal_grid ? 0x00E5FF : 0x64748B, 0, "", true);

    // Floating Colored Bulb Button in upper-right of wide graph window (9 Colorways)
    static const uint32_t kPaletteBulbColors[9] = {
        0x00E5FF, 0xF59E0B, 0xE879F9, 0x10B981, 0xF97316, // Aqua, Amber, Neon, Matrix, Sunset
        0x38BDF8, 0x22C55E, 0xA855F7, 0xEF4444            // IC-7300, G90 LCD, Aurora, Inferno
    };
    static const char* kPaletteNames[9] = {
        "Aqua", "Amber", "Neon", "Matrix", "Sunset",
        "IC-7300", "G90 LCD", "Aurora", "Inferno"
    };
    int cur_scheme = std::clamp(hs.wf_color_scheme, 0, 8);
    uint32_t bulb_col = kPaletteBulbColors[cur_scheme];
    draw_hs_btn(r, hs, mouse_pt, x + w - 260.0f, y + 2.0f, 66.0f, 17.0f, HS_WF_PALETTE_CYCLE,
                std::string("💡 ") + kPaletteNames[cur_scheme], false, bulb_col, 0, "", true);

    // Graph Position Toggle Button (Top vs Bottom of workstation)
    draw_hs_btn(r, hs, mouse_pt, x + w - 190.0f, y + 2.0f, 58.0f, 17.0f, HS_WF_POS_TOGGLE,
                hs.wide_graph_pos == 0 ? "⬇ Bottom" : "⬆ Top", false, 0xF59E0B, 0, "", true);

    // Height Resize Buttons and indicator
    draw_hs_btn(r, hs, mouse_pt, x + w - 128.0f, y + 2.0f, 38.0f, 17.0f, HS_WF_HEIGHT_MINUS, "- H", false, 0x38BDF8, 0, "", true);
    draw_hs_btn(r, hs, mouse_pt, x + w - 86.0f,  y + 2.0f, 38.0f, 17.0f, HS_WF_HEIGHT_PLUS,  "+ H", false, 0x00E5FF, 0, "", true);
    r.draw_text(std::to_string(static_cast<int>(h)) + "px", r.font_mono_small,
                D2D1::RectF(x + w - 44.0f, y + 4.0f, x + w - 4.0f, y + ruler_h + 3.0f),
                r.hex_color(0x94A3B8), DWRITE_TEXT_ALIGNMENT_CENTER);

    // Frequency Hz ruler numbers and calibration ticks (doubled resolution: 50 Hz step):
    // Intense brilliant color where the center of them are, fading out slightly to edges
    for (int f = 100; f <= 2900; f += 50) {
        float fx = hz_to_x(static_cast<float>(f));
        float norm_f = std::clamp((static_cast<float>(f) - 100.0f) / 2800.0f, 0.0f, 1.0f);
        float dist_c = std::abs(norm_f - 0.5f) * 2.0f;
        float center_bright = std::exp(-dist_c * dist_c * 2.5f); // Peak at 1500 Hz

        if (f % 200 == 0) {
            if (fx < x + w - 316.0f) {
                // Intense bright cyan / crisp white near center, smoothly fading out towards edges
                D2D1_COLOR_F num_col = (center_bright > 0.65f) ? r.hex_color(0x00E5FF)
                                     : (center_bright > 0.35f) ? r.hex_color(0x38BDF8)
                                     : (center_bright > 0.15f) ? r.hex_color(0x94A3B8)
                                                               : r.hex_color(0x64748B);
                r.draw_text(std::to_string(f), r.font_mono_small,
                            D2D1::RectF(fx - 24, y + 2, fx + 24, y + ruler_h - 4),
                            num_col, DWRITE_TEXT_ALIGNMENT_CENTER);
            }
            float tick_h = 5.0f + 2.5f * center_bright;
            D2D1_COLOR_F tick_col = D2D1::ColorF(
                0.25f + 0.60f * center_bright,
                0.40f + 0.60f * center_bright,
                0.60f + 0.40f * center_bright,
                0.50f + 0.50f * center_bright
            );
            r.fill_round_rect(D2D1::RectF(fx - 0.5f, y + ruler_h - tick_h, fx + 0.5f, y + ruler_h), 0.0f, tick_col);
        } else if (f % 100 == 0) {
            float tick_h = 3.5f + 1.5f * center_bright;
            r.fill_round_rect(D2D1::RectF(fx - 0.5f, y + ruler_h - tick_h, fx + 0.5f, y + ruler_h), 0.0f,
                              r.hex_color(0x38BDF8, 0.30f + 0.50f * center_bright));
        } else {
            // 50 Hz intermediate sub-ticks (doubled resolution)
            float tick_h = 2.0f + 1.0f * center_bright;
            r.fill_round_rect(D2D1::RectF(fx - 0.5f, y + ruler_h - tick_h, fx + 0.5f, y + ruler_h), 0.0f,
                              r.hex_color(0x38BDF8, 0.15f + 0.35f * center_bright));
        }
    }

    // Center passband diamond indicator (1500 Hz center sweet spot)
    float c_fx = hz_to_x(1500.0f);
    r.draw_text("\xe2\x97\x86", r.font_mono_small, D2D1::RectF(c_fx - 8, y + ruler_h - 10, c_fx + 8, y + ruler_h),
                r.hex_color(0x00E5FF), DWRITE_TEXT_ALIGNMENT_CENTER);

    float spec_y = y + ruler_h + 2.0f;
    // Keep bottom half of graph (waterfall) at its baseline size (~138px),
    // and make the top portion of the graph (spectrum scope) that much larger:
    float wf_baseline = std::max(90.0f, (h - ruler_h - 43.0f - 8.0f) * 0.72f);
    float spec_h = std::max(35.0f, (h - ruler_h - 8.0f) - wf_baseline);
    float wf_y   = spec_y + spec_h + 2.0f;
    float wf_h   = std::max(30.0f, (y + h - 4.0f) - wf_y);

    // 1. Wide graph backgrounds: deep pristine obsidian glass with subtle center passband illumination vignette
    D2D1_RECT_F spec_box = D2D1::RectF(plot_x, spec_y, plot_x + plot_w, spec_y + spec_h);
    D2D1_RECT_F wf_box = D2D1::RectF(plot_x, wf_y, plot_x + plot_w, wf_y + wf_h);
    r.fill_round_rect(spec_box, 4.0f, r.hex_color(0x020409));
    r.fill_round_rect(wf_box, 4.0f, r.hex_color(0x010307));

    // Center passband focus glow behind spectrum and waterfall
    float c_span = plot_w * 0.32f;
    float center_x = plot_x + plot_w * 0.5f;
    r.fill_round_rect(D2D1::RectF(center_x - c_span, spec_y, center_x + c_span, wf_y + wf_h), 0.0f,
                      r.hex_color(0x0A192F, 0.22f));

    // 2. High-precision frequency division grid lines (drawn ON TOP of background, visible across spectrum & waterfall)
    for (int f = 150; f <= 2850; f += 50) {
        float fx = hz_to_x(static_cast<float>(f));
        bool major = (f % 500 == 0);
        bool mid   = (f % 100 == 0);
        r.fill_round_rect(D2D1::RectF(fx - 0.5f, spec_y, fx + 0.5f, wf_y + wf_h), 0.0f,
                          major ? r.hex_color(0x38BDF8, 0.38f) :
                          mid   ? r.hex_color(0x334155, 0.28f) :
                                  r.hex_color(0x1E293B, 0.18f));
    }
    // High-contrast horizontal division lines: 7 lines (12.5% intervals)
    for (float frac : {0.125f, 0.25f, 0.375f, 0.50f, 0.625f, 0.75f, 0.875f}) {
        float sy = spec_y + frac * spec_h;
        bool mid = (std::abs(frac - 0.50f) < 0.01f);
        r.fill_round_rect(D2D1::RectF(plot_x, sy, plot_x + plot_w, sy + 1.0f), 0.0f,
                          mid ? r.hex_color(0x38BDF8, 0.30f) : r.hex_color(0x334155, 0.22f));
    }
    for (float frac : {0.125f, 0.25f, 0.375f, 0.50f, 0.625f, 0.75f, 0.875f}) {
        float wy = wf_y + frac * wf_h;
        bool mid = (std::abs(frac - 0.50f) < 0.01f);
        r.fill_round_rect(D2D1::RectF(plot_x, wy, plot_x + plot_w, wy + 1.0f), 0.0f,
                          mid ? r.hex_color(0x38BDF8, 0.26f) : r.hex_color(0x334155, 0.18f));
    }

    constexpr int BINS = hamshack::SoundcardEngine::WF_BINS;
    std::array<float, BINS> spec{};
    bool has_audio = false;
    uint64_t cur_wf_seq = hs.soundcard.waterfall_seq.load(std::memory_order_acquire);

    // Precompute 256-entry heat palette table for both spectrum bars and streaming waterfall texture
    D2D1_COLOR_F heat_lut_f[256];
    heat_lut_f[0] = D2D1::ColorF(0.0f, 0.0f, 0.0f, 0.0f);
    for (int iv = 1; iv < 256; ++iv) {
        heat_lut_f[iv] = waterfall_heat_color(static_cast<uint8_t>(iv), r.light_mode, cur_scheme);
    }

#ifdef __APPLE__
    const int tex_w = std::clamp(static_cast<int>(std::round(plot_w * 2.0f)), 480, BINS);
#else
    const int tex_w = std::clamp(static_cast<int>(std::round(plot_w)), 360, BINS);
#endif
    const bool need_wf_upload = (cur_wf_seq != 0) && !r.is_waterfall_cache_valid(cur_wf_seq, cur_scheme, tex_w);
    {
        std::lock_guard<std::mutex> lk(hs.soundcard.mtx);
        spec = hs.soundcard.current_spectrum;
        has_audio = (hs.soundcard.rx_rms_db > -115.0f) && !hs.soundcard.waterfall_rows.empty();

        if (need_wf_upload) {
            const int n_rows = static_cast<int>(std::min<size_t>(hs.soundcard.waterfall_rows.size(), 300));
            uint8_t packed_lut[256][4]{};
            for (int iv = 1; iv < 256; ++iv) {
                const D2D1_COLOR_F& c = heat_lut_f[iv];
                uint8_t cr = static_cast<uint8_t>(std::clamp(c.r, 0.0f, 1.0f) * 255.0f + 0.5f);
                uint8_t cg = static_cast<uint8_t>(std::clamp(c.g, 0.0f, 1.0f) * 255.0f + 0.5f);
                uint8_t cb = static_cast<uint8_t>(std::clamp(c.b, 0.0f, 1.0f) * 255.0f + 0.5f);
                // Unified premultiplied BGRA32 layout for both Direct2D and CoreGraphics (2u | (2u << 12))
                packed_lut[iv][0] = cb;
                packed_lut[iv][1] = cg;
                packed_lut[iv][2] = cr;
                packed_lut[iv][3] = 255;
            }

            const size_t req_bytes = static_cast<size_t>(std::max(1, n_rows)) * static_cast<size_t>(tex_w) * 4u;
            if (r.wf_pixel_bytes.size() < req_bytes) {
                r.wf_pixel_bytes.resize(static_cast<size_t>(300) * BINS * 4u, 0);
            }
            r.wf_intervals.clear();

            for (int ri = 0; ri < n_rows; ++ri) {
                const auto& src_row = hs.soundcard.waterfall_rows[ri];
                uint8_t* dst_row = r.wf_pixel_bytes.data() + static_cast<size_t>(ri) * static_cast<size_t>(tex_w) * 4u;
                const uint8_t* bins_ptr = src_row.bins.data();
                // Horizontal max-pooling across [b0, b1) preserves every narrowband CW/FT8/JS8 tone without dilution
                for (int cx = 0; cx < tex_w; ++cx) {
                    const int b0 = (cx * BINS) / tex_w;
                    const int b1 = std::max(b0 + 1, ((cx + 1) * BINS) / tex_w);
                    uint8_t pk = 0;
                    for (int b = b0; b < b1; ++b) {
                        if (bins_ptr[b] > pk) pk = bins_ptr[b];
                    }
                    std::memcpy(dst_row + cx * 4, packed_lut[pk], 4);
                }
                if (src_row.is_interval_start) {
                    r.wf_intervals.push_back({ri, src_row.band_label.empty() ? hs.current_band_label() : src_row.band_label});
                }
            }
            r.update_waterfall_texture(tex_w, n_rows, r.wf_pixel_bytes.data(), cur_wf_seq, cur_scheme);
        }
    }

    // Render Spectrum Bars coalesced to screen pixel columns (eliminates sub-pixel overdraw)
    if (has_audio) {
        const int n_cols = std::clamp(static_cast<int>(plot_w), 120, BINS);
        const float col_w = plot_w / static_cast<float>(n_cols);
        for (int c = 0; c < n_cols; ++c) {
            int b0 = (c * BINS) / n_cols;
            int b1 = std::max(b0 + 1, ((c + 1) * BINS) / n_cols);
            float v = 0.0f;
            for (int b = b0; b < b1; ++b) {
                if (spec[b] > v) v = spec[b];
            }
            v = std::clamp(v, 0.0f, 1.0f);
            if (v <= 0.015f) continue; // Noise floor suppressed
            float bh = v * (spec_h - 4.0f);
            float bx = plot_x + c * col_w;
            D2D1_RECT_F bar = D2D1::RectF(bx, spec_y + spec_h - bh, bx + std::max(1.0f, col_w), spec_y + spec_h);
            uint8_t iv = static_cast<uint8_t>(std::clamp(v * 255.0f, 1.0f, 255.0f));
            r.fill_rect(bar, heat_lut_f[iv]);
        }
    }

    // Render Waterfall via 1 single GPU DrawBitmap call + crisp vector 15-second interval lines
    if (r.wf_tex_rows > 0 && r.wf_tex_width > 0) {
        r.draw_waterfall_texture(plot_x, wf_y, plot_w, wf_h, r.wf_tex_width);
        float row_h = wf_h / static_cast<float>(std::max(16, r.wf_tex_rows));
        for (const auto& ivm : r.wf_intervals) {
            float ry = wf_y + ivm.row_idx * row_h;
            r.fill_rect(D2D1::RectF(plot_x, ry, plot_x + plot_w, ry + 1.5f), r.hex_color(0x38BDF8, 0.85f));
            D2D1_RECT_F t_rect = D2D1::RectF(plot_x + 2.0f, ry + 2.0f, plot_x + 58.0f, ry + 16.0f);
            r.fill_round_rect(t_rect, 2.0f, r.hex_color(0x020617, 0.85f));
            r.draw_text(ivm.band_label, r.font_mono_small, t_rect, r.hex_color(0xFACC15), DWRITE_TEXT_ALIGNMENT_CENTER);
        }
    }

    // ========================================================================
    // Dedicated Visual Tracking Grid Lining the Most Active Signals
    // ========================================================================
    if (hs.wf_show_signal_grid) {
        struct ActiveSignalGridLine {
            int freq_hz;
            int bw_hz;
            std::string label;
            uint32_t color_hex;
            bool is_priority;
        };
        std::vector<ActiveSignalGridLine> active_sigs;
        std::unordered_set<int> seen_buckets;

        // 1. Tuned RX and TX channels
        active_sigs.push_back({rx_hz, bw_hz, "RX VFO", 0x10B981, true});
        seen_buckets.insert(rx_hz / 35);
        if (std::abs(tx_hz - rx_hz) >= 20) {
            active_sigs.push_back({tx_hz, bw_hz, "TX VFO", 0xEF4444, true});
            seen_buckets.insert(tx_hz / 35);
        }

        // 2. Active Decoded Stations (FT8 / FT4 band activity)
        {
            std::lock_guard<std::recursive_mutex> lk(hs.ft8.mtx);
            int take_n = std::min<int>(30, static_cast<int>(hs.ft8.band_activity.size()));
            int start_i = static_cast<int>(hs.ft8.band_activity.size()) - take_n;
            for (int i = start_i; i < static_cast<int>(hs.ft8.band_activity.size()); ++i) {
                const auto& d = hs.ft8.band_activity[i];
                int b_key = d.freq_hz / 35;
                if (seen_buckets.find(b_key) != seen_buckets.end()) continue;
                seen_buckets.insert(b_key);

                std::string c_out, g_out;
                hamshack::Ft8Ft4Engine::extract_spot_info(d.text, c_out, g_out);
                if (c_out.empty()) c_out = std::to_string(d.freq_hz) + "Hz";

                uint32_t col = d.is_for_me ? 0xEF4444
                             : (!hs.ft8.dx_call.empty() && c_out.find(hs.ft8.dx_call) != std::string::npos) ? 0x10B981
                             : d.is_cq ? 0x00E5FF
                                       : 0x38BDF8;
                active_sigs.push_back({d.freq_hz, d.is_ft4 ? 80 : 50, c_out, col, d.is_for_me || d.is_cq});
            }
        }

        // 3. Active JS8 Stations if in JS8 tab
        if (hs.active_mode_tab == 1) {
            std::lock_guard<std::mutex> lk(hs.js8.mtx);
            for (const auto& h : hs.js8.heard_stations) {
                int b_key = h.offset_hz / 35;
                if (seen_buckets.find(b_key) != seen_buckets.end()) continue;
                seen_buckets.insert(b_key);
                active_sigs.push_back({h.offset_hz, 50, h.callsign, 0x38BDF8, false});
            }
        }

        // 4. Real-time Spectral Peaks from live audio
        if (has_audio) {
            for (int b = 4; b < BINS - 4; ++b) {
                float v = spec[b];
                if (v > 0.28f && v > spec[b - 1] && v > spec[b + 1] && v > spec[b - 2] && v > spec[b + 2]) {
                    int peak_f = static_cast<int>(std::round(100.0f + (static_cast<float>(b) / BINS) * 2800.0f));
                    int b_key = peak_f / 35;
                    if (seen_buckets.find(b_key) == seen_buckets.end()) {
                        seen_buckets.insert(b_key);
                        active_sigs.push_back({peak_f, 50, std::to_string(peak_f) + "Hz", bulb_col, false});
                    }
                }
            }
        }

        // Draw the precision grid lines framing every active signal
        for (const auto& s : active_sigs) {
            float x0 = hz_to_x(static_cast<float>(s.freq_hz - s.bw_hz / 2));
            float xc = hz_to_x(static_cast<float>(s.freq_hz));
            float x1 = hz_to_x(static_cast<float>(s.freq_hz + s.bw_hz / 2));
            if (x1 < plot_x || x0 > plot_x + plot_w) continue;

            // Subtle shaded signal column lane across spectrum & waterfall
            D2D1_RECT_F sig_lane = D2D1::RectF(x0, spec_y, x1, wf_y + wf_h);
            r.fill_round_rect(sig_lane, 0.0f, r.hex_color(s.color_hex, s.is_priority ? 0.18f : 0.08f));

            // Precision vertical grid boundary lines lining each signal
            r.fill_round_rect(D2D1::RectF(x0 - 0.5f, spec_y, x0 + 0.5f, wf_y + wf_h), 0.0f,
                              r.hex_color(s.color_hex, s.is_priority ? 0.85f : 0.50f));
            r.fill_round_rect(D2D1::RectF(x1 - 0.5f, spec_y, x1 + 0.5f, wf_y + wf_h), 0.0f,
                              r.hex_color(s.color_hex, s.is_priority ? 0.85f : 0.50f));

            // Center carrier tracking line in spectrum box
            r.fill_round_rect(D2D1::RectF(xc - 0.5f, spec_y, xc + 0.5f, spec_y + spec_h), 0.0f,
                              r.hex_color(s.color_hex, 0.40f));

            // Horizontal cross-hatch grid ticks across the signal lane (doubled resolution: 12px step)
            for (float gy = spec_y + 4.0f; gy < wf_y + wf_h - 4.0f; gy += 12.0f) {
                r.fill_round_rect(D2D1::RectF(x0, gy, x1, gy + 1.0f), 0.0f,
                                  r.hex_color(s.color_hex, s.is_priority ? 0.32f : 0.16f));
            }

            // Active signal tag badge (rendered cleanly below the Hz ruler inside the spectrum area)
            if (s.is_priority || s.freq_hz % 200 == 0 || active_sigs.size() <= 10) {
                float tag_w = std::max(x1 - x0 + 8.0f, 48.0f);
                float tag_x = std::clamp(xc - tag_w * 0.5f, plot_x, plot_x + plot_w - tag_w);
                D2D1_RECT_F tag_r = D2D1::RectF(tag_x, spec_y + 2.0f, tag_x + tag_w, spec_y + 16.0f);
                r.fill_round_rect(tag_r, 2.0f, r.hex_color(0x020617, 0.90f));
                r.draw_round_rect(tag_r, 2.0f, r.hex_color(s.color_hex, 0.85f), 1.0f);
                r.draw_text(s.label, r.font_mono_small, tag_r, r.hex_color(s.color_hex), DWRITE_TEXT_ALIGNMENT_CENTER);
            }
        }
    }

    // Draw RX Goalpost (Green) and TX Goalpost (Red)
    float rx_x0 = hz_to_x(static_cast<float>(rx_hz));
    float rx_x1 = hz_to_x(static_cast<float>(rx_hz + bw_hz));
    float tx_x0 = hz_to_x(static_cast<float>(tx_hz));
    float tx_x1 = hz_to_x(static_cast<float>(tx_hz + bw_hz));

    r.fill_round_rect(D2D1::RectF(rx_x0, ruler_r.bottom - 4, rx_x1, ruler_r.bottom), 1.0f, r.hex_color(0x10B981));
    r.fill_round_rect(D2D1::RectF(rx_x0, spec_y, rx_x0 + 1.5f, wf_y + wf_h), 0.0f, r.hex_color(0x10B981, 0.90f));
    r.fill_round_rect(D2D1::RectF(rx_x1, spec_y, rx_x1 + 1.5f, wf_y + wf_h), 0.0f, r.hex_color(0x10B981, 0.70f));

    r.fill_round_rect(D2D1::RectF(tx_x0, ruler_r.top + 1, tx_x1, ruler_r.top + 5), 1.0f, r.hex_color(0xEF4444));
    r.fill_round_rect(D2D1::RectF(tx_x0, spec_y, tx_x0 + 1.5f, wf_y + wf_h), 0.0f, r.hex_color(0xEF4444, 0.85f));

    D2D1_RECT_F click_zone = D2D1::RectF(plot_x, spec_y, plot_x + plot_w, wf_y + wf_h);
    if (RendererT::pt_in_rect(mouse_pt, click_zone)) {
        int hover_hz = 100 + static_cast<int>(((mouse_pt.x - plot_x) / plot_w) * 2800.0f);
        D2D1_RECT_F tip = D2D1::RectF(std::min(mouse_pt.x + 10.0f, x + w - 95.0f), mouse_pt.y - 20.0f,
                                      std::min(mouse_pt.x + 92.0f, x + w - 8.0f), mouse_pt.y - 2.0f);
        r.fill_round_rect(tip, 4.0f, r.hex_color(0x0F172A, 0.92f));
        r.draw_round_rect(tip, 4.0f, r.hex_color(0x00E5FF), 1.0f);
        r.draw_text(std::to_string(hover_hz) + " Hz", r.font_mono_small, tip,
                    r.hex_color(0x00E5FF), DWRITE_TEXT_ALIGNMENT_CENTER);
    }
    hs.add_hit(click_zone, hit_id);
}

// ============================================================================
// Persistent Top Deck (y: 0..110) with Upper-Right Virtual FLRig Window
// ============================================================================
template <typename RendererT, typename AppT>
inline void render_flrig_top_deck_and_mode_tabs(RendererT& r, AppT& app, HamShackSuite& hs, float W) {
    hs.hits.clear();
    hs.sync_hf_station();

    r.fill_round_rect(D2D1::RectF(0, 0, W, 110), 0.0f, r.hex_color(0x0F172A));
    r.draw_round_rect(D2D1::RectF(0, 0, W, 110), 0.0f, r.hex_color(0x1E293B), 1.0f);

    bool conn = false;
    uint64_t vfo_a = 0, vfo_b = 0;
    hamshack::RigMode mode_a = hamshack::RigMode::USB;
    int filt_a = 1;
    bool use_b = false, split_on = false, ptt_on = false, atu_on = false;
    int pre = 0, att = 0, agc = 1, pwr_w = 0, vol_pct = 0, sql_pct = 0, smeter = 0, smeter_dbm = -128;
    bool nb = false, comp = false;
    float pwr_out = 0.0f, swr = 0.0f;
    std::string rig_name, cat_port;
    int active_band = -1;
    {
        std::lock_guard<std::mutex> lk(hs.flrig.mtx);
        conn = hs.flrig.connected;
        vfo_a = hs.flrig.vfo_a_hz;
        vfo_b = hs.flrig.vfo_b_hz;
        mode_a = hs.flrig.active_vfo_b ? hs.flrig.mode_b : hs.flrig.mode_a;
        filt_a = hs.flrig.active_vfo_b ? hs.flrig.filter_b : hs.flrig.filter_a;
        use_b = hs.flrig.active_vfo_b;
        split_on = hs.flrig.split_enabled;
        ptt_on = hs.flrig.ptt_active;
        atu_on = hs.flrig.atu_tuning;
        pre = hs.flrig.preamp_level;
        att = hs.flrig.atten_level;
        agc = hs.flrig.agc_mode;
        nb = hs.flrig.nb_enabled;
        comp = hs.flrig.comp_enabled;
        pwr_w = hs.flrig.power_watts;
        vol_pct = hs.flrig.volume_pct;
        sql_pct = hs.flrig.squelch_pct;
        smeter = hs.flrig.smeter_pct;
        smeter_dbm = hs.flrig.smeter_dbm;
        pwr_out = hs.flrig.power_out_watts;
        swr = hs.flrig.swr_ratio;
        rig_name = hamshack::FlrigCatEngine::model_name(hs.flrig.model);
        cat_port = hs.flrig.serial_port;
        active_band = hs.flrig.active_band_idx;
    }

    float vfw_w = std::min(796.0f, std::max(580.0f, W * 0.54f));
    float vfx = W - vfw_w - 8.0f;
    float left_avail_w = std::max(340.0f, vfx - 16.0f);

    // ========================================================================
    // LEFT SIDE OF TOP DECK (x: 10 .. vfx - 6, y: 4 .. 108)
    // ========================================================================
    // ---- Left Row 1 (y: 5 .. 39): Brand + CAT Port + USB Audio + BLE/LoRa + Theme ----
    D2D1_RECT_F brand_r = D2D1::RectF(10, 5, 162, 39);
    r.fill_round_rect(brand_r, 6.0f, r.hex_color(0x162238));
    r.draw_round_rect(brand_r, 6.0f, conn ? r.hex_color(0x10B981, 0.85f) : r.hex_color(0x334155, 0.75f), 1.3f);
    r.draw_text("HAMSHACK++", r.font_ui_bold, D2D1::RectF(16, conn ? 6 : 11, 158, conn ? 23 : 32), r.hex_color(0x00E5FF));
    if (conn) {
        r.draw_text("\xe2\x97\x8f ONLINE", r.font_small, D2D1::RectF(16, 22, 158, 38), r.hex_color(0x10B981));
    }

    float lx = 166.0f;
    std::string short_cat = cat_port;
    if (short_cat.rfind("/dev/cu.", 0) == 0) short_cat = short_cat.substr(8);
    if (short_cat.empty()) short_cat = "Scan CAT";
    draw_hs_btn(r, hs, app.mouse_pt, lx, 5, 134, 34, HS_CAT_PORT_CYCLE,
                (conn ? "G90: " : "CAT: ") + short_cat, conn, conn ? 0x10B981 : 0xEF4444, 0, "", true);
    lx += 138.0f;

    std::string aud_lbl = "Audio: " + hs.soundcard.active_input_name;
    if (aud_lbl.size() > 19) aud_lbl = aud_lbl.substr(0, 19) + "..";
    draw_hs_btn(r, hs, app.mouse_pt, lx, 5, 142, 34, HS_AUDIO_IN_CYCLE,
                aud_lbl, hs.soundcard.hw_stream_active, 0x34D399, 0, "", true);
    lx += 146.0f;

    bool serial_active = app.transport_connected && !app.is_sim && app.transport_label.find("SERIAL") != std::string::npos;
    bool ble_active    = (!app.is_sim && app.transport_label.find("BLE") != std::string::npos) || app.ble_dropdown_open;

    auto draw_app_btn = [&](int btn_id, float x_pos, float y_pos, float w_btn, float h_btn, const std::string& lbl, bool act, uint32_t acc) {
        D2D1_RECT_F rc = D2D1::RectF(x_pos, y_pos, x_pos + w_btn, y_pos + h_btn);
        bool hov = RendererT::pt_in_rect(app.mouse_pt, rc);
        r.fill_round_rect(rc, 6.0f, act ? r.hex_color(acc, 0.22f) : hov ? r.hex_color(0x1E293B) : r.hex_color(0x131C2E));
        r.draw_round_rect(rc, 6.0f, act ? r.hex_color(acc) : r.hex_color(0x26354D), act ? 1.4f : 1.0f);
        r.draw_text(lbl, r.font_small, rc, act ? r.hex_color(acc) : r.hex_color(0xE2E8F0), DWRITE_TEXT_ALIGNMENT_CENTER);
        app.button_hits.push_back({rc, btn_id, ""});
    };

    if (lx + 90.0f < vfx) {
        draw_app_btn(BTN_USB_SERIAL, lx, 5, 86, 34, serial_active ? "LoRa USB" : "LoRa v", serial_active, 0x10B981);
        lx += 90.0f;
    }
    if (lx + 84.0f < vfx) {
        draw_app_btn(BTN_BLE_NUS, lx, 5, 80, 34, ble_active ? "BLE: ON" : "BLE v", ble_active, 0x38BDF8);
        lx += 84.0f;
    }
    // Theme Icon Button (Sun / Moon)
    if (lx + 36.0f < vfx) {
        draw_app_btn(BTN_TOGGLE_THEME, lx, 5, 34, 34, app.light_mode ? "☀️" : "🌙", app.light_mode, 0xFBBF24);
        lx += 38.0f;
    }

    // Twitch Live Streaming Button
    bool twitch_live = hs.twitch.is_streaming;
    if (lx + 38.0f < vfx) {
        // Not colored if offline (neutral 0x64748B), turns bright green when live streaming (0x22C55E)
        draw_hs_btn(r, hs, app.mouse_pt, lx, 5, 36, 34, HS_TWITCH_STREAM_TOGGLE,
                    "📺", twitch_live, twitch_live ? 0x22C55E : 0x64748B, 0, "", true);
        lx += 40.0f;
    }

    // Flashing glowing "ON AIR" indicator in the empty spot between controls and the radio
    if (twitch_live && lx + 86.0f < vfx) {
        static auto s_t0 = std::chrono::steady_clock::now();
        float elapsed = std::chrono::duration<float>(std::chrono::steady_clock::now() - s_t0).count();
        float pulse = 0.5f + 0.5f * std::sin(elapsed * 5.0f); // 5 Hz glowing pulse

        float badge_w = 82.0f;
        float badge_h = 24.0f;
        float badge_y = 10.0f;
        D2D1_RECT_F glow_rc = D2D1::RectF(lx - 2.0f, badge_y - 2.0f, lx + badge_w + 2.0f, badge_y + badge_h + 2.0f);
        D2D1_RECT_F on_air_rc = D2D1::RectF(lx, badge_y, lx + badge_w, badge_y + badge_h);

        // Subtle glowing red aura
        r.fill_round_rect(glow_rc, 6.0f, r.hex_color(0xEF4444, 0.15f + 0.25f * pulse));
        r.fill_round_rect(on_air_rc, 4.0f, r.hex_color(0x3B070C));
        r.draw_round_rect(on_air_rc, 4.0f, r.hex_color(0xEF4444, 0.70f + 0.30f * pulse), 1.5f);
        r.draw_text("● ON AIR", r.font_ui_bold, on_air_rc, r.hex_color(0xFCA5A5), DWRITE_TEXT_ALIGNMENT_CENTER);
        lx += badge_w + 6.0f;
    }

    // ---- Left Row 2 (y: 43 .. 73): Operator Callsign & Grid ----
    if (hs.active_mode_tab == 3) {
        draw_app_btn(BTN_CALLSIGN, 10.0f, 43.0f, 210.0f, 29.0f, "MESH RADIO NODE: " + app.callsign, false, 0xA855F7);
    } else {
        bool call_foc = (hs.text_focus == HsTextFocus::HfCallsign);
        bool grid_foc = (hs.text_focus == HsTextFocus::HfGrid);
        std::string c_disp = hs.hf_callsign.empty() ? "W1AW" : hs.hf_callsign;
        std::string g_disp = hs.hf_grid.empty() ? "FN31" : hs.hf_grid;
        draw_hs_btn(r, hs, app.mouse_pt, 10.0f,  43.0f, 134.0f, 29.0f, HS_HF_CALLSIGN_BOX,
                    "CALL: " + c_disp + (call_foc ? "_" : ""), call_foc, 0x00E5FF, 0, "", true);
        draw_hs_btn(r, hs, app.mouse_pt, 148.0f, 43.0f, 94.0f,  29.0f, HS_HF_GRID_BOX,
                    "GRID: " + g_disp + (grid_foc ? "_" : ""), grid_foc, 0x10B981, 0, "", true);
    }
    size_t worked_call_cnt = 0, worked_qso_cnt = 0;
    {
        std::lock_guard<std::recursive_mutex> lk(hs.ft8.mtx);
        worked_call_cnt = hs.ft8.worked_calls.size();
        worked_qso_cnt  = hs.ft8.qso_log.size();
    }
    if (left_avail_w > 260.0f) {
        r.draw_text("Worked: " + std::to_string(worked_call_cnt) + " calls | Log: " + std::to_string(worked_qso_cnt) + " QSOs",
                    r.font_mono_small, D2D1::RectF(250.0f, 48.0f, vfx - 8.0f, 72.0f), r.hex_color(0xFACC15));
    }

    // ---- Left Row 3 (y: 77 .. 106): 11 HF Amateur Band Quick-QSY Buttons (160m..6m) ----
    float band_btn_w = std::floor((left_avail_w - 10 * 3.0f) / 11.0f);
    band_btn_w = std::clamp(band_btn_w, 34.0f, 54.0f);
    for (int b = 0; b < 11; ++b) {
        float bx_pos = 10.0f + b * (band_btn_w + 3.0f);
        bool b_act = conn && (b == active_band);
        draw_hs_btn(r, hs, app.mouse_pt, bx_pos, 77.0f, band_btn_w, 29.0f, HS_BAND_BASE + b,
                    hamshack::kBandPresets[b].label, b_act, 0x10B981, b, "", true);
    }

    // ========================================================================
    // UPPER-RIGHT CORNER: VIRTUAL FLRIG / XIEGU G90 FRONT-PANEL WINDOW
    // (x: vfx .. W - 8, y: 4 .. 108)
    // ========================================================================
    D2D1_RECT_F flrig_win = D2D1::RectF(vfx, 4.0f, W - 8.0f, 108.0f);
    r.fill_round_rect(flrig_win, 8.0f, r.hex_color(0x070B12));
    r.draw_round_rect(flrig_win, 8.0f,
                      ptt_on ? r.hex_color(0xEF4444)
                             : (conn ? r.hex_color(0x00E5FF, 0.85f) : r.hex_color(0x475569)),
                      1.6f);

    // Virtual Radio Window Title Bar (y: 5 .. 22)
    D2D1_RECT_F fw_hdr = D2D1::RectF(vfx + 2.0f, 5.0f, W - 10.0f, 22.0f);
    r.fill_round_rect(fw_hdr, 6.0f, r.hex_color(0x162238));
    std::string fw_title = rig_name + (conn ? (" [ONLINE " + short_cat + "]") : " [OFFLINE — NO CAT]");
    r.draw_text(fw_title, r.font_mono_small, D2D1::RectF(vfx + 10.0f, 5.0f, W - 150.0f, 22.0f),
                conn ? r.hex_color(0x00E5FF) : r.hex_color(0xFCA5A5));
    draw_hs_btn(r, hs, app.mouse_pt, W - 144.0f, 6.0f, 64.0f, 15.0f, HS_CAT_POLL_VFO, "Poll CAT", false, 0x38BDF8, 0, "", true);
    draw_hs_btn(r, hs, app.mouse_pt, W - 76.0f,  6.0f, 62.0f, 15.0f, HS_CAT_RESCAN,   "Rescan",   false, 0x10B981, 0, "", true);

    // ---- Left Half of Virtual FLRig Window: Dual VFO A/B LCD + S/Po/SWR Bargraph + Tuning Steppers ----
    float lcd_x = vfx + 6.0f;
    float lcd_w = std::floor(vfw_w * 0.45f);
    D2D1_RECT_F lcd_box = D2D1::RectF(lcd_x, 24.0f, lcd_x + lcd_w, 81.0f);
    r.fill_round_rect(lcd_box, 6.0f, r.hex_color(0x03060C));
    r.draw_round_rect(lcd_box, 6.0f, conn ? r.hex_color(0x00E5FF, 0.55f) : r.hex_color(0x334155), 1.2f);

    // VFO A (Left side of LCD) & VFO B (Right side of LCD)
    float vfoa_w = lcd_w * 0.58f;
    D2D1_RECT_F lcd_vfoa = D2D1::RectF(lcd_x + 4.0f, 25.0f, lcd_x + vfoa_w, 60.0f);
    D2D1_RECT_F lcd_vfob = D2D1::RectF(lcd_x + vfoa_w + 2.0f, 25.0f, lcd_x + lcd_w - 4.0f, 60.0f);
    if (!use_b && conn) r.fill_round_rect(lcd_vfoa, 4.0f, r.hex_color(0x00E5FF, 0.10f));
    if (use_b && conn)  r.fill_round_rect(lcd_vfob, 4.0f, r.hex_color(0x00E5FF, 0.10f));

    r.draw_text(std::string(!use_b ? "\xe2\x96\xb6" : " ") + "VFO A [" +
                (conn ? ( std::string(hamshack::FlrigCatEngine::mode_name(mode_a)) + " F" + std::to_string(filt_a)) : "OFF") + "]",
                r.font_mono_small, D2D1::RectF(lcd_vfoa.left + 4, 25, lcd_vfoa.right - 2, 39),
                !use_b ? r.hex_color(0x00E5FF) : r.hex_color(0x94A3B8));
    r.draw_text(conn ? hamshack::FlrigCatEngine::format_freq_mhz(vfo_a) : "---.---.---",
                r.font_title, D2D1::RectF(lcd_vfoa.left + 4, 38, lcd_vfoa.right - 2, 60),
                conn ? r.hex_color(0xF8FAFC) : r.hex_color(0x64748B));
    hs.add_hit(lcd_vfoa, HS_VFO_SELECT_A);

    r.draw_text(std::string(use_b ? "\xe2\x96\xb6" : " ") + (split_on ? "VFO B [SPLIT TX]" : "VFO B"),
                r.font_mono_small, D2D1::RectF(lcd_vfob.left + 4, 25, lcd_vfob.right - 2, 39),
                split_on ? r.hex_color(0xF59E0B) : (use_b ? r.hex_color(0x00E5FF) : r.hex_color(0x94A3B8)));
    r.draw_text(conn ? hamshack::FlrigCatEngine::format_freq_mhz(vfo_b) : "---.---.---",
                r.font_ui_bold, D2D1::RectF(lcd_vfob.left + 4, 40, lcd_vfob.right - 2, 60),
                conn ? r.hex_color(0x38BDF8) : r.hex_color(0x64748B));
    hs.add_hit(lcd_vfob, HS_VFO_SELECT_B);

    // S-Meter / RF Power / SWR Bar inside LCD (y: 62 .. 79)
    D2D1_RECT_F mtr_bg = D2D1::RectF(lcd_x + 6.0f, 63.0f, lcd_x + lcd_w * 0.56f, 76.0f);
    r.fill_round_rect(mtr_bg, 3.0f, r.hex_color(0x131C2E));
    if (conn) {
        float frac = ptt_on ? std::clamp(pwr_out / 20.0f, 0.0f, 1.0f) : std::clamp(smeter / 100.0f, 0.0f, 1.0f);
        D2D1_RECT_F mtr_fg = D2D1::RectF(mtr_bg.left, mtr_bg.top, mtr_bg.left + (mtr_bg.right - mtr_bg.left) * frac, mtr_bg.bottom);
        r.fill_round_rect(mtr_fg, 3.0f, ptt_on ? r.hex_color(0xEF4444) : (smeter > 55 ? r.hex_color(0xF59E0B) : r.hex_color(0x10B981)));
    }
    char mtr_txt[64];
    if (!conn) std::snprintf(mtr_txt, sizeof(mtr_txt), "S0 / OFFLINE");
    else if (ptt_on) std::snprintf(mtr_txt, sizeof(mtr_txt), "TX %.1fW SWR %.2f", pwr_out, swr);
    else std::snprintf(mtr_txt, sizeof(mtr_txt), "%s (%+ddBm)", hamshack::FlrigCatEngine::smeter_s_unit_str(smeter).c_str(), smeter_dbm);
    r.draw_text(mtr_txt, r.font_mono_small,
                D2D1::RectF(mtr_bg.right + 6.0f, 61.0f, lcd_x + lcd_w - 4.0f, 79.0f),
                ptt_on ? r.hex_color(0xEF4444) : (conn ? r.hex_color(0x34D399) : r.hex_color(0x64748B)));

    // Fine & Coarse VFO Tuning Steppers under LCD (y: 83 .. 104)
    float st_w = (lcd_w - 5 * 3.0f) / 6.0f;
    draw_hs_btn(r, hs, app.mouse_pt, lcd_x + 0*(st_w+3), 83.0f, st_w, 21.0f, HS_VFO_STEP_M10K, "-10k", false, 0x38BDF8, 0, "", true);
    draw_hs_btn(r, hs, app.mouse_pt, lcd_x + 1*(st_w+3), 83.0f, st_w, 21.0f, HS_VFO_STEP_M1K,  "-1k",  false, 0x38BDF8, 0, "", true);
    draw_hs_btn(r, hs, app.mouse_pt, lcd_x + 2*(st_w+3), 83.0f, st_w, 21.0f, HS_VFO_STEP_M100, "-100", false, 0x38BDF8, 0, "", true);
    draw_hs_btn(r, hs, app.mouse_pt, lcd_x + 3*(st_w+3), 83.0f, st_w, 21.0f, HS_VFO_STEP_P100, "+100", false, 0x00E5FF, 0, "", true);
    draw_hs_btn(r, hs, app.mouse_pt, lcd_x + 4*(st_w+3), 83.0f, st_w, 21.0f, HS_VFO_STEP_P1K,  "+1k",  false, 0x00E5FF, 0, "", true);
    draw_hs_btn(r, hs, app.mouse_pt, lcd_x + 5*(st_w+3), 83.0f, st_w, 21.0f, HS_VFO_STEP_P10K, "+10k", false, 0x00E5FF, 0, "", true);

    // ---- Right Half of Virtual FLRig Window: Virtual Front-Panel Pushbuttons ----
    float fpx = lcd_x + lcd_w + 6.0f;
    float fpw = (W - 12.0f) - fpx;

    // Front-Panel Row 1 (y: 24 .. 49): PTT, TUNE/ATU, A/B, A->B, SPLIT, FILT
    float r1_w = (fpw - 5 * 3.0f) / 6.0f;
    draw_hs_btn(r, hs, app.mouse_pt, fpx + 0*(r1_w+3), 24.0f, r1_w, 25.0f, HS_PTT_TOGGLE,  ptt_on ? "TX ON" : "PTT",     ptt_on,   0xEF4444, 0, "", true);
    draw_hs_btn(r, hs, app.mouse_pt, fpx + 1*(r1_w+3), 24.0f, r1_w, 25.0f, HS_ATU_TUNE,    atu_on ? "TUNING" : "TUNE",   atu_on,   0xF59E0B, 0, "", true);
    draw_hs_btn(r, hs, app.mouse_pt, fpx + 2*(r1_w+3), 24.0f, r1_w, 25.0f, HS_VFO_SWAP_AB, "A / B",                      use_b,    0x00E5FF, 0, "", true);
    draw_hs_btn(r, hs, app.mouse_pt, fpx + 3*(r1_w+3), 24.0f, r1_w, 25.0f, HS_VFO_COPY_AB, "A -> B",                     false,    0x38BDF8, 0, "", true);
    draw_hs_btn(r, hs, app.mouse_pt, fpx + 4*(r1_w+3), 24.0f, r1_w, 25.0f, HS_VFO_SPLIT,   split_on ? "SPL ON" : "SPLIT",split_on, 0xF59E0B, 0, "", true);
    draw_hs_btn(r, hs, app.mouse_pt, fpx + 5*(r1_w+3), 24.0f, r1_w, 25.0f, HS_FILT_CYCLE,  conn ? ("FIL" + std::to_string(filt_a)) : "FILT", conn, 0x10B981, 0, "", true);

    // Front-Panel Row 2 (y: 52 .. 77): Direct Mode Buttons (LSB, USB, CW, AM) + PRE, ATT, AGC, NB, CMP
    float r2_w = (fpw - 8 * 3.0f) / 9.0f;
    draw_hs_btn(r, hs, app.mouse_pt, fpx + 0*(r2_w+3), 52.0f, r2_w, 25.0f, HS_MODE_SET_LSB, "LSB", conn && mode_a == hamshack::RigMode::LSB, 0x00E5FF, 0, "", true);
    draw_hs_btn(r, hs, app.mouse_pt, fpx + 1*(r2_w+3), 52.0f, r2_w, 25.0f, HS_MODE_SET_USB, "USB", conn && mode_a == hamshack::RigMode::USB, 0x00E5FF, 0, "", true);
    draw_hs_btn(r, hs, app.mouse_pt, fpx + 2*(r2_w+3), 52.0f, r2_w, 25.0f, HS_MODE_SET_CW,  "CW",  conn && (mode_a == hamshack::RigMode::CW || mode_a == hamshack::RigMode::CWR), 0xFBBF24, 0, "", true);
    draw_hs_btn(r, hs, app.mouse_pt, fpx + 3*(r2_w+3), 52.0f, r2_w, 25.0f, HS_MODE_SET_AM,  "AM",  conn && mode_a == hamshack::RigMode::AM,  0x38BDF8, 0, "", true);
    draw_hs_btn(r, hs, app.mouse_pt, fpx + 4*(r2_w+3), 52.0f, r2_w, 25.0f, HS_PRE_TOGGLE,   "PRE", conn && pre != 0, 0x10B981, 0, "", true);
    draw_hs_btn(r, hs, app.mouse_pt, fpx + 5*(r2_w+3), 52.0f, r2_w, 25.0f, HS_ATT_TOGGLE,   "ATT", conn && att != 0, 0xF59E0B, 0, "", true);
    draw_hs_btn(r, hs, app.mouse_pt, fpx + 6*(r2_w+3), 52.0f, r2_w, 25.0f, HS_AGC_CYCLE,    conn ? hamshack::FlrigCatEngine::agc_label(agc) : "AGC", conn, 0x38BDF8, 0, "", true);
    draw_hs_btn(r, hs, app.mouse_pt, fpx + 7*(r2_w+3), 52.0f, r2_w, 25.0f, HS_NB_TOGGLE,    "NB",  conn && nb,   0x38BDF8, 0, "", true);
    draw_hs_btn(r, hs, app.mouse_pt, fpx + 8*(r2_w+3), 52.0f, r2_w, 25.0f, HS_COMP_TOGGLE,  "CMP", conn && comp, 0xA855F7, 0, "", true);

    // Front-Panel Row 3 (y: 80 .. 104): RF Power (1..20W), AF Volume (0..100%), Squelch (0..100%)
    float grp_w = (fpw - 6.0f) / 3.0f;
    auto draw_knob_stepper = [&](float gx_k, const std::string& title_k, int dec_id, int inc_id, uint32_t col_k) {
        float btn_sz = 30.0f;
        draw_hs_btn(r, hs, app.mouse_pt, gx_k, 80.0f, btn_sz, 24.0f, dec_id, "—", false, col_k, 0, "", true);
        D2D1_RECT_F mid_r = D2D1::RectF(gx_k + btn_sz + 2.0f, 80.0f, gx_k + grp_w - btn_sz - 2.0f, 104.0f);
        r.fill_round_rect(mid_r, 4.0f, r.hex_color(0x0B111E));
        r.draw_round_rect(mid_r, 4.0f, r.hex_color(0x26354D), 1.0f);
        r.draw_text(title_k, r.font_mono_small, mid_r, r.hex_color(col_k), DWRITE_TEXT_ALIGNMENT_CENTER);
        draw_hs_btn(r, hs, app.mouse_pt, gx_k + grp_w - btn_sz, 80.0f, btn_sz, 24.0f, inc_id, "+", false, col_k, 0, "", true);
    };
    int disp_pwr = (pwr_w > 0) ? pwr_w : (hs.flrig.power_watts > 0 ? hs.flrig.power_watts : 16);
    draw_knob_stepper(fpx,                      "PWR " + std::to_string(disp_pwr) + "W",   HS_PWR_DOWN, HS_PWR_UP, 0xFBBF24);
    draw_knob_stepper(fpx + grp_w + 3.0f,       "VOL " + std::to_string(vol_pct) + "%", HS_VOL_DOWN, HS_VOL_UP, 0x10B981);
    draw_knob_stepper(fpx + (grp_w + 3.0f) * 2, "SQL " + std::to_string(sql_pct) + "%", HS_SQL_DOWN, HS_SQL_UP, 0x38BDF8);
}

// ============================================================================
// Global Bottom Tab Bar (y: H - 42 .. H - 6, Live across ALL windows!)
// ============================================================================
template <typename RendererT, typename AppT>
inline void render_global_bottom_tab_bar(RendererT& r, AppT& app, HamShackSuite& hs, float W, float H) {
    float bar_y = H - 42.0f;
    float bar_h = 36.0f;
    D2D1_RECT_F bg_r = D2D1::RectF(0, bar_y - 2.0f, W, H);
    r.fill_round_rect(bg_r, 0.0f, r.hex_color(0x070B12));
    r.fill_round_rect(D2D1::RectF(0, bar_y - 2.0f, W, bar_y - 1.0f), 0.0f, r.hex_color(0x1E293B));

    struct GlobalTabSpec {
        int hit_id;
        int idx;
        const char* label;
        uint32_t color;
    };
    static constexpr GlobalTabSpec kGlobalTabs[11] = {
        {HS_TAB_FT8,    0,  "FT8 / FT4",     0x00E5FF},
        {HS_TAB_JS8,    1,  "JS8Call",       0x10B981},
        {HS_TAB_CW,     2,  "CW Keyer",      0xFBBF24},
        {HS_TAB_APRS,   9,  "APRS",          0x22D3EE},
        {HS_TAB_SSTV,   10, "SSTV",          0xF472B6},
        {HS_TAB_MESH,   3,  "MeshCore",      0xA855F7},
        {HS_TAB_GT,     4,  "Grid Tracker",  0xF59E0B},
        {HS_TAB_PAGES,  5,  "Pages",         0x38BDF8},
        {HS_TAB_IRC,    6,  "IRC",           0xEC4899},
        {HS_TAB_WINLINK,7,  "Winlink",       0x34D399},
        {HS_TAB_CONFIG, 8,  "Configuration", 0x94A3B8}
    };

    float tab_w = std::min(134.0f, (W - 20.0f - 10 * 4.0f) / 11.0f);
    for (int i = 0; i < 11; ++i) {
        float tx = 10.0f + i * (tab_w + 4.0f);
        D2D1_RECT_F tr = D2D1::RectF(tx, bar_y, tx + tab_w, bar_y + bar_h);
        bool sel = (hs.active_mode_tab == kGlobalTabs[i].idx);
        bool hov = RendererT::pt_in_rect(app.mouse_pt, tr);
        r.fill_round_rect(tr, 6.0f, sel ? r.hex_color(kGlobalTabs[i].color, 0.25f)
                                  : hov ? r.hex_color(0x1E293B)
                                        : r.hex_color(0x131C2E));
        r.draw_round_rect(tr, 6.0f, sel ? r.hex_color(kGlobalTabs[i].color) : r.hex_color(0x26354D), sel ? 1.8f : 1.0f);
        r.draw_text(kGlobalTabs[i].label, r.font_ui_bold, tr,
                    sel ? r.hex_color(kGlobalTabs[i].color) : r.hex_color(0xCBD5E1),
                    DWRITE_TEXT_ALIGNMENT_CENTER);
        hs.add_hit(tr, kGlobalTabs[i].hit_id, kGlobalTabs[i].idx);
    }
}

// ============================================================================
// TAB 0: WSJT-X FT8 / FT4 Integrated Workstation (Wide Multi-Column Grids)
// ============================================================================
template <typename RendererT>
inline void render_ft8_ft4_workspace(RendererT& r, HamShackSuite& hs, POINT mouse_pt,
                                     float x, float y, float w, float h) {
    // Redundant 10-cell telemetry strip removed — wide graph expands into this space
    // 1. Resizable Wide Graph Waterfall + Spectrum (Persistent Top or Bottom Position)
    float bot_h = 148.0f;
    float max_wf_h = std::max(140.0f, h - bot_h - 165.0f);
    float wf_h = std::clamp(hs.wide_graph_height, 110.0f, max_wf_h);
    int bw = hs.ft8.is_ft4 ? 85 : 50;
    std::string wf_title = std::string("WSJT-X WIDE GRAPH \xe2\x80\xa2 ") + (hs.ft8.is_ft4 ? "FT4 (4-GFSK 7.5s)" : "FT8 (8-GFSK 15.0s)") +
                           " \xe2\x80\xa2 RX " + std::to_string(hs.ft8.rx_freq_hz) + " Hz / TX " +
                           std::to_string(hs.ft8.tx_freq_hz) + " Hz [Click Waterfall to QSY]";

    float wf_y = 0.0f;
    float mid_y = 0.0f;
    float mid_h = 0.0f;

    if (hs.wide_graph_pos == 0) {
        // TOP POSITION (default)
        wf_y = y;
        mid_y = wf_y + wf_h + 11.0f;
        mid_h = std::max(140.0f, (y + h - bot_h - 6.0f) - mid_y);

        render_waterfall_widget(r, hs, mouse_pt, x, wf_y, w, wf_h, hs.ft8.rx_freq_hz, hs.ft8.tx_freq_hz, bw, HS_FT_WATERFALL, wf_title);
        render_wide_graph_splitter(r, hs, mouse_pt, x, wf_y + wf_h + 1.0f, w, false);
    } else {
        // BOTTOM POSITION
        mid_y = y;
        mid_h = std::max(140.0f, (y + h - bot_h - 6.0f) - mid_y - wf_h - 11.0f);
        wf_y = mid_y + mid_h + 11.0f;

        render_wide_graph_splitter(r, hs, mouse_pt, x, wf_y - 10.0f, w, true);
        render_waterfall_widget(r, hs, mouse_pt, x, wf_y, w, wf_h, hs.ft8.rx_freq_hz, hs.ft8.tx_freq_hz, bw, HS_FT_WATERFALL, wf_title);
    }

    // 3. Wide Main Data Grids: Band Activity & Rx Frequency (Equal 50% / 50% Split, Fixed)
    float left_w = std::floor((w - 8.0f) * 0.50f);
    float right_w = (w - 8.0f) - left_w;

    uint64_t dial_hz = hs.flrig.connected ? hs.flrig.vfo_a_hz : 0ULL;

    auto extract_call_grid = [&](const std::string& msg, std::string& call_out, std::string& grid_out, std::string& az_dist_out) {
        call_out = "--";
        grid_out = "--";
        az_dist_out = "--";
        hamshack::Ft8Ft4Engine::extract_spot_info(msg, call_out, grid_out);
        if (call_out.empty()) call_out = "--";
        if (grid_out.empty()) grid_out = "--";
        if (grid_out != "--") {
            int az = 0, km = 0;
            if (hamshack::Ft8Ft4Engine::compute_az_dist(hs.ft8.my_grid, grid_out, az, km)) {
                az_dist_out = std::to_string(az) + "\xc2\xb0/" + std::to_string(km) + "km";
            }
        }
    };

    auto draw_wide_decode_grid = [&](float gx, float gw, const std::string& title,
                                     const std::deque<hamshack::DecodedFtMessage>& list,
                                     int hit_id) {
        D2D1_RECT_F card = D2D1::RectF(gx, mid_y, gx + gw, mid_y + mid_h);
        r.fill_round_rect(card, 8.0f, r.hex_color(0x0F172A));
        r.draw_round_rect(card, 8.0f, r.hex_color(0x1E293B), 1.2f);

        // Title Bar
        D2D1_RECT_F hdr = D2D1::RectF(gx + 3, mid_y + 3, gx + gw - 3, mid_y + 25);
        r.fill_round_rect(hdr, 5.0f, r.hex_color(0x162238));
        r.draw_text(title + " (" + std::to_string(list.size()) + " rows)", r.font_ui_bold,
                    D2D1::RectF(gx + 10, mid_y + 3, gx + gw - 10, mid_y + 25), r.hex_color(0x00E5FF));

        // Column Header Row
        D2D1_RECT_F col_hdr = D2D1::RectF(gx + 3, mid_y + 26, gx + gw - 3, mid_y + 46);
        r.fill_round_rect(col_hdr, 3.0f, r.hex_color(0x090D16));
        r.draw_text("UTC     MODE   SNR    DT    AF Hz   RF DIAL MHz   CALLSIGN   GRID   AZ/DIST      DECODED MESSAGE",
                    r.font_mono_small, D2D1::RectF(gx + 10, mid_y + 26, gx + gw - 110, mid_y + 46), r.hex_color(0x38BDF8));
        r.draw_text("STATE / COUNTRY", r.font_mono_small,
                    D2D1::RectF(gx + gw - 114, mid_y + 26, gx + gw - 6, mid_y + 46), r.hex_color(0x00E5FF), DWRITE_TEXT_ALIGNMENT_CENTER);

        if (!hs.flrig.connected && list.empty()) {
            r.draw_text("XIEGU G90 OFFLINE — Connect & power on G90 USB CAT + USB Audio Device to decode live FT8/FT4 signals",
                        r.font_mono_small, D2D1::RectF(gx + 16, mid_y + 54, gx + gw - 16, mid_y + mid_h - 12),
                        r.hex_color(0x64748B), DWRITE_TEXT_ALIGNMENT_CENTER);
            return;
        }
        if (list.empty()) {
            r.draw_text("Listening on " + (dial_hz ? hamshack::FlrigCatEngine::format_freq_mhz(dial_hz) + " MHz" : "USB Audio") +
                        " — Waiting for live FT8/FT4 signals...",
                        r.font_mono_small, D2D1::RectF(gx + 16, mid_y + 54, gx + gw - 16, mid_y + mid_h - 12),
                        r.hex_color(0x94A3B8), DWRITE_TEXT_ALIGNMENT_CENTER);
            return;
        }

        float ry = mid_y + 48.0f;
        float row_h = 20.0f;
        int max_rows = std::max(1, static_cast<int>((mid_h - 52.0f) / row_h));
        int start_idx = std::max(0, static_cast<int>(list.size()) - max_rows);

        for (int i = start_idx; i < static_cast<int>(list.size()); ++i) {
            const auto& d = list[i];
            D2D1_RECT_F rr = D2D1::RectF(gx + 5, ry, gx + gw - 5, ry + row_h - 1.0f);
            bool hov = RendererT::pt_in_rect(mouse_pt, rr);

            std::string call_s, grid_s, az_s;
            extract_call_grid(d.text, call_s, grid_s, az_s);
            bool worked_before = hs.ft8.is_call_worked(call_s);
            bool being_worked = (!hs.ft8.dx_call.empty() &&
                                 hamshack::Ft8Ft4Engine::normalize_call_key(call_s) == hamshack::Ft8Ft4Engine::normalize_call_key(hs.ft8.dx_call));

            if (being_worked) {
                // Light green highlight behind station being worked in FT8/FT4 modes (Requirement 1)
                r.fill_round_rect(rr, 3.0f, r.hex_color(0x22C55E, 0.40f));
                r.draw_round_rect(rr, 3.0f, r.hex_color(0x4ADE80, 0.90f), 1.2f);
            } else if (worked_before) {
                // Yellow highlight behind previously worked contacts (from WSJT-X .adi or session log)
                r.fill_round_rect(rr, 3.0f, r.hex_color(0xEAB308, 0.36f));
                r.draw_round_rect(rr, 3.0f, r.hex_color(0xFACC15, 0.85f), 1.0f);
            } else if (d.is_for_me) {
                r.fill_round_rect(rr, 3.0f, r.hex_color(0xEF4444, 0.25f));
            } else if (d.is_cq) {
                r.fill_round_rect(rr, 3.0f, r.hex_color(0x10B981, 0.20f));
            } else if (d.is_tx) {
                r.fill_round_rect(rr, 3.0f, r.hex_color(0xF59E0B, 0.20f));
            } else if (hov) {
                r.fill_round_rect(rr, 3.0f, r.hex_color(0x1E293B));
            } else if (i % 2 == 1) {
                r.fill_round_rect(rr, 3.0f, r.hex_color(0x0B111E));
            }

            std::string loc_s = hamshack::lookup_state_or_country(call_s, grid_s);
            float badge_w = 98.0f;
            D2D1_RECT_F tab_badge = D2D1::RectF(rr.right - badge_w - 4.0f, rr.top + 2.0f, rr.right - 4.0f, rr.bottom - 2.0f);

            if (loc_s != "--") {
                bool is_us = (loc_s.find("USA") != std::string::npos || loc_s.find(", US") != std::string::npos);
                r.fill_round_rect(tab_badge, 3.0f, is_us ? r.hex_color(0x0284C7, 0.28f) : r.hex_color(0x7C3AED, 0.28f));
                r.draw_round_rect(tab_badge, 3.0f, is_us ? r.hex_color(0x38BDF8, 0.70f) : r.hex_color(0xA855F7, 0.70f), 1.0f);
                r.draw_text(loc_s, r.font_mono_small, tab_badge, is_us ? r.hex_color(0xBAE6FD) : r.hex_color(0xE9D5FF), DWRITE_TEXT_ALIGNMENT_CENTER);
            }

            uint64_t rf_hz = dial_hz ? (dial_hz + static_cast<uint64_t>(d.freq_hz)) : 0ULL;
            char rf_buf[24];
            if (rf_hz > 0) std::snprintf(rf_buf, sizeof(rf_buf), "%9.5f", rf_hz / 1e6);
            else           std::snprintf(rf_buf, sizeof(rf_buf), "   ---   ");

            std::string msg_disp = being_worked ? (d.text + " [WORKING]") : (worked_before ? (d.text + " [WORKED]") : d.text);
            char line_buf[240];
            std::snprintf(line_buf, sizeof(line_buf), "%-6s  %-4s  %+3ddB  %4.1f  %4dHz  %s   %-8s   %-4s   %-11s  %s",
                          d.utc_str.c_str(), d.is_ft4 ? "FT4" : "FT8", d.snr_db, d.dt_sec, d.freq_hz,
                          rf_buf, call_s.c_str(), grid_s.c_str(), az_s.c_str(), msg_disp.c_str());

            D2D1_COLOR_F col = being_worked  ? r.hex_color(0x86EFAC)
                             : worked_before ? r.hex_color(0xFEF08A)
                             : d.is_for_me   ? r.hex_color(0xFCA5A5)
                             : d.is_cq       ? r.hex_color(0x34D399)
                             : d.is_tx       ? r.hex_color(0xFBBF24)
                                             : r.hex_color(0xE2E8F0);
            r.draw_text(line_buf, r.font_mono_small, D2D1::RectF(rr.left + 6, rr.top, rr.right - badge_w - 6.0f, rr.bottom), col);
            hs.add_hit(rr, hit_id, i);
            ry += row_h;
        }
    };

    std::deque<hamshack::DecodedFtMessage> band_copy, rxf_copy;
    {
        std::lock_guard<std::recursive_mutex> lk(hs.ft8.mtx);
        band_copy = hs.ft8.band_activity;
        rxf_copy  = hs.ft8.rx_freq_activity;
    }
    draw_wide_decode_grid(x, left_w, "BAND ACTIVITY — LIVE LDPC(174,91) PASSBAND DECODES", band_copy, HS_FT_BAND_ACT_ROW);
    draw_wide_decode_grid(x + left_w + 8.0f, right_w, "RX FREQUENCY (" + std::to_string(hs.ft8.rx_freq_hz) + " Hz) & QSO ACTIVITY", rxf_copy, HS_FT_RX_FREQ_ROW);

    // 4. Bottom WSJT-X Control & QSO Sequencer Deck (Compact 148px)
    float by = mid_y + mid_h + 6.0f;
    D2D1_RECT_F bot_card = D2D1::RectF(x, by, x + w, y + h);
    r.fill_round_rect(bot_card, 8.0f, r.hex_color(0x0F172A));
    r.draw_round_rect(bot_card, 8.0f, r.hex_color(0x1E293B), 1.2f);

    float rw = std::min(430.0f, std::max(310.0f, w * 0.42f));
    float rx_col = x + w - rw - 10.0f;
    float lw = rx_col - x - 18.0f;

    // Left Controls Area: buttons on the left
    float lx = x + 10.0f;
    float ly = by + 8.0f;
    float btn_w = std::floor((lw - 5 * 4.0f) / 6.0f);
    draw_hs_btn(r, hs, mouse_pt, lx + 0*(btn_w+4), ly, btn_w, 26, HS_FT_SUBMODE_FT8, "FT8 (15s)", !hs.ft8.is_ft4, 0x00E5FF, 0, "", true);
    draw_hs_btn(r, hs, mouse_pt, lx + 1*(btn_w+4), ly, btn_w, 26, HS_FT_SUBMODE_FT4, "FT4 (7.5s)", hs.ft8.is_ft4, 0x00E5FF, 0, "", true);
    draw_hs_btn(r, hs, mouse_pt, lx + 2*(btn_w+4), ly, btn_w, 26, HS_FT_MONITOR,     hs.ft8.monitor_enabled ? "Monitor ON" : "Monitor", hs.ft8.monitor_enabled, 0x10B981, 0, "", true);
    draw_hs_btn(r, hs, mouse_pt, lx + 3*(btn_w+4), ly, btn_w, 26, HS_FT_AUTO_SEQ,    "Auto Seq", hs.ft8.auto_seq, 0x38BDF8, 0, "", true);
    draw_hs_btn(r, hs, mouse_pt, lx + 4*(btn_w+4), ly, btn_w, 26, HS_FT_TX_EVEN,     hs.ft8.tx_even_first ? "Tx Even/1st" : "Tx Odd/2nd", hs.ft8.tx_even_first, 0xA855F7, 0, "", true);
    draw_hs_btn(r, hs, mouse_pt, lx + 5*(btn_w+4), ly, btn_w, 26, HS_FT_HOLD_TX,     "Hold Tx Freq", hs.ft8.hold_tx_freq, 0xFBBF24, 0, "", true);

    float r2y = ly + 32.0f;
    float c_w = std::floor((lw - 4 * 4.0f) / 5.0f);
    D2D1_RECT_F myc_r = D2D1::RectF(lx + 0*(c_w+4), r2y, lx + 0*(c_w+4) + c_w, r2y + 28);
    bool myc_foc = (hs.text_focus == HsTextFocus::HfCallsign);
    r.fill_round_rect(myc_r, 5.0f, r.hex_color(0x05080F));
    r.draw_round_rect(myc_r, 5.0f, myc_foc ? r.hex_color(0x00E5FF) : r.hex_color(0x334155), 1.3f);
    std::string myc_disp = hs.hf_callsign.empty() ? "W1AW" : hs.hf_callsign;
    std::string myg_disp = hs.hf_grid.empty() ? "FN31" : hs.hf_grid;
    r.draw_text("My Call: " + myc_disp + (myc_foc ? "_" : ""), r.font_ui_bold,
                D2D1::RectF(myc_r.left + 6, myc_r.top, myc_r.right - 4, myc_r.bottom), r.hex_color(0x00E5FF));
    hs.add_hit(myc_r, HS_HF_CALLSIGN_BOX);

    D2D1_RECT_F myg_r = D2D1::RectF(lx + 1*(c_w+4), r2y, lx + 1*(c_w+4) + c_w, r2y + 28);
    bool myg_foc = (hs.text_focus == HsTextFocus::HfGrid);
    r.fill_round_rect(myg_r, 5.0f, r.hex_color(0x05080F));
    r.draw_round_rect(myg_r, 5.0f, myg_foc ? r.hex_color(0x00E5FF) : r.hex_color(0x334155), 1.3f);
    r.draw_text("Grid: " + myg_disp + (myg_foc ? "_" : ""), r.font_ui_bold,
                D2D1::RectF(myg_r.left + 6, myg_r.top, myg_r.right - 4, myg_r.bottom), r.hex_color(0x10B981));
    hs.add_hit(myg_r, HS_HF_GRID_BOX);

    bool working_dx = !hs.ft8.dx_call.empty();
    D2D1_RECT_F dxc_r = D2D1::RectF(lx + 2*(c_w+4), r2y, lx + 2*(c_w+4) + c_w, r2y + 28);
    bool dxc_foc = (hs.text_focus == HsTextFocus::Ft8DxCall);
    r.fill_round_rect(dxc_r, 5.0f, working_dx ? r.hex_color(0x14532D) : r.hex_color(0x05080F));
    r.draw_round_rect(dxc_r, 5.0f, dxc_foc ? r.hex_color(0x00E5FF) : (working_dx ? r.hex_color(0x4ADE80) : r.hex_color(0x334155)), 1.3f);
    r.draw_text("DX: " + hs.ft8.dx_call + (dxc_foc ? "_" : ""), r.font_ui_bold,
                D2D1::RectF(dxc_r.left + 6, dxc_r.top, dxc_r.right - 4, dxc_r.bottom),
                working_dx ? r.hex_color(0x86EFAC) : r.hex_color(0xF8FAFC));
    hs.add_hit(dxc_r, HS_FT_DX_CALL_BOX);

    D2D1_RECT_F dxg_r = D2D1::RectF(lx + 3*(c_w+4), r2y, lx + 3*(c_w+4) + c_w, r2y + 28);
    bool dxg_foc = (hs.text_focus == HsTextFocus::Ft8DxGrid);
    r.fill_round_rect(dxg_r, 5.0f, r.hex_color(0x05080F));
    r.draw_round_rect(dxg_r, 5.0f, dxg_foc ? r.hex_color(0x00E5FF) : r.hex_color(0x334155), 1.3f);
    r.draw_text("DXG: " + hs.ft8.dx_grid + (dxg_foc ? "_" : ""), r.font_ui_bold,
                D2D1::RectF(dxg_r.left + 6, dxg_r.top, dxg_r.right - 4, dxg_r.bottom), r.hex_color(0x38BDF8));
    hs.add_hit(dxg_r, HS_FT_DX_GRID_BOX);

    int az = 0, dist_km = 0;
    std::string geo_str = "Logged: " + std::to_string(hs.ft8.qso_log.size());
    if (hamshack::Ft8Ft4Engine::compute_az_dist(hs.ft8.my_grid, hs.ft8.dx_grid, az, dist_km)) {
        geo_str = std::to_string(az) + "\xc2\xb0/" + std::to_string(dist_km) + "km";
    }
    D2D1_RECT_F geo_r = D2D1::RectF(lx + 4*(c_w+4), r2y, lx + 4*(c_w+4) + c_w, r2y + 28);
    r.draw_text(geo_str, r.font_mono_small, geo_r, r.hex_color(0x34D399), DWRITE_TEXT_ALIGNMENT_CENTER);

    float r3y = r2y + 34.0f;
    float act_w = std::floor((lw - 5 * 4.0f) / 6.0f);
    draw_hs_btn(r, hs, mouse_pt, lx + 0*(act_w+4), r3y, act_w, 28, HS_FT_ENABLE_TX, hs.ft8.tx_enabled ? "TX ARMED" : "Enable Tx", hs.ft8.tx_enabled, 0xEF4444, 0, "", true);
    draw_hs_btn(r, hs, mouse_pt, lx + 1*(act_w+4), r3y, act_w, 28, HS_FT_HALT_TX,   "Halt Tx",   false, 0xF59E0B, 0, "", true);
    draw_hs_btn(r, hs, mouse_pt, lx + 2*(act_w+4), r3y, act_w, 28, HS_FT_TUNE,      "Tune",      hs.ft8.tune_active, 0xEF4444, 0, "", true);
    draw_hs_btn(r, hs, mouse_pt, lx + 3*(act_w+4), r3y, act_w, 28, HS_FT_LOG_QSO,   "Log QSO",   false, 0x10B981, 0, "", true);
    draw_hs_btn(r, hs, mouse_pt, lx + 4*(act_w+4), r3y, act_w, 28, HS_FT_ERASE,     "Erase",     false, 0x94A3B8, 0, "", true);
    draw_hs_btn(r, hs, mouse_pt, lx + 5*(act_w+4), r3y, act_w, 28, HS_FT_DECODE_NOW,"Decode Now",false, 0x00E5FF, 0, "", true);

    float r4y = r3y + 34.0f;
    D2D1_RECT_F cyc_bg = D2D1::RectF(lx, r4y, lx + lw, r4y + 22);
    r.fill_round_rect(cyc_bg, 4.0f, r.hex_color(0x05080F));
    float p_w = (cyc_bg.right - cyc_bg.left) * std::clamp(hs.ft8.cycle_progress, 0.0f, 1.0f);
    r.fill_round_rect(D2D1::RectF(cyc_bg.left, cyc_bg.top, cyc_bg.left + p_w, cyc_bg.bottom), 4.0f,
                      hs.ft8.transmitting_now ? r.hex_color(0xEF4444, 0.65f) : r.hex_color(0x10B981, 0.50f));
    r.draw_round_rect(cyc_bg, 4.0f, r.hex_color(0x26354D), 1.0f);
    char cbuf[128];
    std::snprintf(cbuf, sizeof(cbuf), "%s  %.1fs / %.1fs  |  Next: [Tx%d] %s",
                  hs.ft8.transmitting_now ? "TRANSMITTING" : "MONITORING",
                  hs.ft8.cycle_sec, hs.ft8.is_ft4 ? 7.5f : 15.0f,
                  hs.ft8.active_tx_slot + 1,
                  hs.ft8.tx_msgs[std::clamp(hs.ft8.active_tx_slot, 0, 5)].c_str());
    r.draw_text(cbuf, r.font_mono_small, cyc_bg, r.hex_color(0xF8FAFC), DWRITE_TEXT_ALIGNMENT_CENTER);

    // Right Column: Standard WSJT-X Messages Tx1 .. Tx6 stacked vertically
    float m_tot_h = (r4y + 22.0f) - ly;
    float m_row_h = (m_tot_h - 5 * 3.0f) / 6.0f;
    for (int s = 0; s < 6; ++s) {
        float my = ly + s * (m_row_h + 3.0f);
        bool sel = (hs.ft8.active_tx_slot == s);
        bool working_slot = (sel && working_dx && s < 5);
        std::string tag = "Tx " + std::to_string(s + 1);
        draw_hs_btn(r, hs, mouse_pt, rx_col, my, 46.0f, m_row_h, HS_FT_TX_SLOT_BASE + s, tag, sel,
                    working_slot ? 0x22C55E : (s == 5 ? 0x10B981 : 0x00E5FF), s, "", true);
        D2D1_RECT_F mr = D2D1::RectF(rx_col + 50.0f, my, rx_col + rw, my + m_row_h);
        r.fill_round_rect(mr, 4.0f, working_slot ? r.hex_color(0x22C55E, 0.28f) : (sel ? r.hex_color(0x00E5FF, 0.18f) : r.hex_color(0x070B14)));
        r.draw_round_rect(mr, 4.0f, working_slot ? r.hex_color(0x4ADE80) : (sel ? r.hex_color(0x00E5FF) : r.hex_color(0x1E293B)), sel ? 1.4f : 1.0f);
        r.draw_text(hs.ft8.tx_msgs[s], r.font_mono_small, D2D1::RectF(mr.left + 8, mr.top + 2, mr.right - 8, mr.bottom),
                    working_slot ? r.hex_color(0x86EFAC) : (sel ? r.hex_color(0xF8FAFC) : r.hex_color(0xCBD5E1)));
        hs.add_hit(mr, HS_FT_TX_SLOT_BASE + s, s);
    }
}

// ============================================================================
// TAB 1: JS8Call Integrated Workstation (Wide Multi-Column Grids)
// ============================================================================
template <typename RendererT>
inline void render_js8call_workspace(RendererT& r, HamShackSuite& hs, POINT mouse_pt,
                                     float x, float y, float w, float h) {
    // 1. Live G90 & Soundcard Telemetry Strip (38px)
    render_live_g90_telemetry_grid(r, hs, x, y, w, 38.0f);

    // 2. Submode & Auto/HB/Mesh Bridge Control Bar (y + 42 .. y + 78)
    float top_y = y + 42.0f;
    D2D1_RECT_F top_r = D2D1::RectF(x, top_y, x + w, top_y + 36);
    r.fill_round_rect(top_r, 7.0f, r.hex_color(0x0F172A));
    r.draw_round_rect(top_r, 7.0f, r.hex_color(0x1E293B), 1.1f);

    float bx = x + 8.0f;
    float by = top_y + 4.0f;
    draw_hs_btn(r, hs, mouse_pt, bx,       by, 128, 28, HS_JS8_SUB_NORMAL, "Normal (15s 16WPM)", hs.js8.current_submode == js8::Submode::NORMAL, 0x00E5FF, 0, "", true);
    draw_hs_btn(r, hs, mouse_pt, bx + 132, by, 120, 28, HS_JS8_SUB_FAST,   "Fast (10s 24WPM)",   hs.js8.current_submode == js8::Submode::FAST,   0x10B981, 0, "", true);
    draw_hs_btn(r, hs, mouse_pt, bx + 256, by, 124, 28, HS_JS8_SUB_TURBO,  "Turbo (6s 40WPM)",   hs.js8.current_submode == js8::Submode::TURBO,  0xFBBF24, 0, "", true);
    draw_hs_btn(r, hs, mouse_pt, bx + 384, by, 118, 28, HS_JS8_SUB_SLOW,   "Slow (30s 8WPM)",    hs.js8.current_submode == js8::Submode::SLOW,   0x38BDF8, 0, "", true);

    draw_hs_btn(r, hs, mouse_pt, bx + 510, by, 90,  28, HS_JS8_HB_TOGGLE,   hs.js8.heartbeat_enabled ? "HB: ON" : "HB: OFF", hs.js8.heartbeat_enabled, 0x10B981, 0, "", true);
    draw_hs_btn(r, hs, mouse_pt, bx + 604, by, 96,  28, HS_JS8_AUTO_TOGGLE, hs.js8.auto_reply_enabled ? "AUTO: ON" : "AUTO: OFF", hs.js8.auto_reply_enabled, 0x00E5FF, 0, "", true);
    draw_hs_btn(r, hs, mouse_pt, bx + 704, by, 172, 28, HS_JS8_MESH_BRIDGE,
                hs.js8.bridge_to_meshcore ? "BLE Mesh Bridge: ON" : "BLE Mesh Bridge: OFF",
                hs.js8.bridge_to_meshcore, 0xA855F7, 0, "", true);
    draw_hs_btn(r, hs, mouse_pt, bx + 880, by, 110, 28, HS_JS8_DECODE_NOW,  "Decode Audio", false, 0x38BDF8, 0, "", true);
    draw_hs_btn(r, hs, mouse_pt, bx + 994, by, 80,  28, HS_JS8_CLEAR_BTN,   "Clear",        false, 0x94A3B8, 0, "", true);

    // 3. Three Wide JS8Call Data Grids (Band Frames 36% | Directed QSO & Composer 34% | Heard Stations & Inbox 30%)
    float mid_y = top_y + 42.0f;
    float max_wf_h = std::max(140.0f, (y + h) - mid_y - 195.0f);
    float wf_h = std::clamp(hs.wide_graph_height, 110.0f, max_wf_h);
    float mid_h = std::max(180.0f, (y + h - wf_h - 11.0f) - mid_y);

    float left_w   = std::floor((w - 16.0f) * 0.36f);
    float center_w = std::floor((w - 16.0f) * 0.34f);
    float right_w  = (w - 16.0f) - left_w - center_w;
    float center_x = x + left_w + 8.0f;
    float right_x  = center_x + center_w + 8.0f;

    uint64_t dial_hz = hs.flrig.connected ? hs.flrig.vfo_a_hz : 0ULL;

    // ---- Left Column: Wide 8-Column Band Activity Frame Grid ----
    D2D1_RECT_F lcard = D2D1::RectF(x, mid_y, x + left_w, mid_y + mid_h);
    r.fill_round_rect(lcard, 8.0f, r.hex_color(0x0F172A));
    r.draw_round_rect(lcard, 8.0f, r.hex_color(0x1E293B), 1.2f);
    r.draw_text("JS8 BAND ACTIVITY — LDPC(174,87) FRAMES", r.font_ui_bold,
                D2D1::RectF(x + 10, mid_y + 4, x + left_w - 10, mid_y + 24), r.hex_color(0x00E5FF));

    D2D1_RECT_F lhdr = D2D1::RectF(x + 4, mid_y + 25, x + left_w - 4, mid_y + 44);
    r.fill_round_rect(lhdr, 3.0f, r.hex_color(0x090D16));
    r.draw_text("UTC      SUB   dB   DT   AF Hz  RF MHz    12-CHAR FRAME",
                r.font_mono_small, D2D1::RectF(x + 8, mid_y + 25, x + left_w - 8, mid_y + 44), r.hex_color(0x38BDF8));

    {
        std::lock_guard<std::mutex> lk(hs.js8.mtx);
        if (hs.js8.band_frames.empty()) {
            r.draw_text(hs.flrig.connected ? "No JS8 frames decoded yet — Listening on USB Audio..."
                                           : "XIEGU G90 OFFLINE — Connect G90 USB CAT & Audio",
                        r.font_mono_small, D2D1::RectF(x + 12, mid_y + 52, x + left_w - 12, mid_y + mid_h - 12),
                        r.hex_color(0x64748B), DWRITE_TEXT_ALIGNMENT_CENTER);
        } else {
            float fy = mid_y + 46.0f;
            float row_h = 20.0f;
            int max_r = std::max(1, static_cast<int>((mid_h - 50.0f) / row_h));
            int start_i = std::max(0, static_cast<int>(hs.js8.band_frames.size()) - max_r);
            for (int i = start_i; i < static_cast<int>(hs.js8.band_frames.size()); ++i) {
                const auto& fl = hs.js8.band_frames[i];
                D2D1_RECT_F rr = D2D1::RectF(x + 5, fy, x + left_w - 5, fy + row_h - 1.0f);
                // Extract any callsign token in the JS8 frame to check if previously worked in ADIF log
                bool worked_frame = false;
                if (!fl.is_tx) {
                    std::istringstream iss(fl.text);
                    std::string tok;
                    while (iss >> tok) {
                        while (!tok.empty() && !std::isalnum(static_cast<unsigned char>(tok.back()))) tok.pop_back();
                        if (tok.size() >= 3 && hs.ft8.is_call_worked(tok)) { worked_frame = true; break; }
                    }
                }
                if (fl.is_tx) r.fill_round_rect(rr, 3.0f, r.hex_color(0xF59E0B, 0.18f));
                else if (worked_frame) {
                    r.fill_round_rect(rr, 3.0f, r.hex_color(0xEAB308, 0.36f));
                    r.draw_round_rect(rr, 3.0f, r.hex_color(0xFACC15, 0.80f), 1.0f);
                }
                else if (i % 2 == 1) r.fill_round_rect(rr, 3.0f, r.hex_color(0x0B111E));

                const char* sm_s = (fl.submode == js8::Submode::FAST) ? "FAST"
                                 : (fl.submode == js8::Submode::TURBO)? "TURB"
                                 : (fl.submode == js8::Submode::SLOW) ? "SLOW" : "NORM";
                double rf_mhz = dial_hz ? ((dial_hz + fl.offset_hz) / 1e6) : 0.0;
                char fbuf[180];
                std::string disp_txt = fl.text + (worked_frame ? " [WORKED]" : "");
                if (rf_mhz > 0.0) {
                    std::snprintf(fbuf, sizeof(fbuf), "%-8s %-4s %+3d %4.1f %4dHz %8.4f  %s",
                                  fl.utc_time.c_str(), sm_s, fl.snr_db, fl.dt_sec, fl.offset_hz, rf_mhz, disp_txt.c_str());
                } else {
                    std::snprintf(fbuf, sizeof(fbuf), "%-8s %-4s %+3d %4.1f %4dHz   ---     %s",
                                  fl.utc_time.c_str(), sm_s, fl.snr_db, fl.dt_sec, fl.offset_hz, disp_txt.c_str());
                }
                r.draw_text(fbuf, r.font_mono_small, D2D1::RectF(rr.left + 4, rr.top, rr.right - 4, rr.bottom),
                            fl.is_tx ? r.hex_color(0xFBBF24) : worked_frame ? r.hex_color(0xFEF08A) : r.hex_color(0xE2E8F0));
                hs.add_hit(rr, HS_JS8_FRAME_ROW, fl.offset_hz);
                fy += row_h;
            }
        }
    }

    // ---- Center Column: Directed QSO Transcript + Composer + Directed Command Strip ----
    D2D1_RECT_F ccard = D2D1::RectF(center_x, mid_y, center_x + center_w, mid_y + mid_h);
    r.fill_round_rect(ccard, 8.0f, r.hex_color(0x0F172A));
    r.draw_round_rect(ccard, 8.0f, r.hex_color(0x1E293B), 1.2f);
    r.draw_text("DIRECTED QSO TRANSCRIPT  (Target: " + hs.js8.selected_call + ")", r.font_ui_bold,
                D2D1::RectF(center_x + 12, mid_y + 4, center_x + center_w - 12, mid_y + 26), r.hex_color(0x34D399));

    float cmd_bar_h = 34.0f;
    float comp_h = 38.0f;
    float trans_h = std::max(80.0f, mid_h - 32.0f - cmd_bar_h - comp_h - 16.0f);
    D2D1_RECT_F trans_box = D2D1::RectF(center_x + 8, mid_y + 28, center_x + center_w - 8, mid_y + 28 + trans_h);
    r.fill_round_rect(trans_box, 6.0f, r.hex_color(0x05080F));
    r.draw_round_rect(trans_box, 6.0f, r.hex_color(0x1E293B), 1.0f);

    {
        std::lock_guard<std::mutex> lk(hs.js8.mtx);
        if (hs.js8.qso_transcript.empty()) {
            r.draw_text("No directed JS8 messages yet.", r.font_mono_small, trans_box,
                        r.hex_color(0x64748B), DWRITE_TEXT_ALIGNMENT_CENTER);
        } else {
            float ty = trans_box.top + 6.0f;
            float lh = 21.0f;
            int max_l = std::max(1, static_cast<int>((trans_h - 10.0f) / lh));
            int start_l = std::max(0, static_cast<int>(hs.js8.qso_transcript.size()) - max_l);
            for (int i = start_l; i < static_cast<int>(hs.js8.qso_transcript.size()); ++i) {
                const std::string& line = hs.js8.qso_transcript[i];
                bool is_tx = line.find(" TX]") != std::string::npos;
                bool is_mesh = line.find("AUTO->MESH") != std::string::npos;
                D2D1_COLOR_F col = is_mesh ? r.hex_color(0xC084FC)
                                 : is_tx   ? r.hex_color(0xFBBF24)
                                           : r.hex_color(0x38BDF8);
                r.draw_text(line, r.font_mono_small,
                            D2D1::RectF(trans_box.left + 8, ty, trans_box.right - 8, ty + lh), col);
                ty += lh;
            }
        }
    }

    float cmd_y = trans_box.bottom + 6.0f;
    struct Js8CmdBtn { int id; const char* lbl; uint32_t col; };
    static constexpr Js8CmdBtn kCmds[9] = {
        {HS_JS8_CMD_CQ,      "CQ",       0x10B981},
        {HS_JS8_CMD_HB,      "HB",       0x38BDF8},
        {HS_JS8_CMD_SNR,     "SNR?",     0x00E5FF},
        {HS_JS8_CMD_INFO,    "INFO?",    0x00E5FF},
        {HS_JS8_CMD_GRID,    "GRID?",    0x00E5FF},
        {HS_JS8_CMD_STATUS,  "STATUS?",  0x00E5FF},
        {HS_JS8_CMD_HEARING, "HEARING?", 0xA855F7},
        {HS_JS8_CMD_ACK,     "ACK",      0xFBBF24},
        {HS_JS8_CMD_73,      "73",       0x10B981}
    };
    float c_w = (center_w - 16.0f - 8 * 4.0f) / 9.0f;
    for (int i = 0; i < 9; ++i) {
        draw_hs_btn(r, hs, mouse_pt, center_x + 8.0f + i * (c_w + 4.0f), cmd_y, c_w, 28.0f,
                    kCmds[i].id, kCmds[i].lbl, false, kCmds[i].col, 0, "", true);
    }

    float comp_y = cmd_y + 34.0f;
    D2D1_RECT_F comp_box = D2D1::RectF(center_x + 8, comp_y, center_x + center_w - 148, comp_y + 32);
    bool js8_foc = (hs.text_focus == HsTextFocus::Js8Composer);
    r.fill_round_rect(comp_box, 5.0f, r.hex_color(0x05080F));
    r.draw_round_rect(comp_box, 5.0f, js8_foc ? r.hex_color(0x00E5FF) : r.hex_color(0x334155), 1.3f);
    std::string prompt = hs.js8.composer_text.empty() && !js8_foc
                             ? ("Click to type JS8 message to " + hs.js8.selected_call + "...")
                             : (hs.js8.composer_text + (js8_foc ? "_" : ""));
    r.draw_text(prompt, r.font_mono_small, D2D1::RectF(comp_box.left + 8, comp_box.top, comp_box.right - 8, comp_box.bottom),
                hs.js8.composer_text.empty() && !js8_foc ? r.hex_color(0x64748B) : r.hex_color(0xF8FAFC));
    hs.add_hit(comp_box, HS_JS8_COMPOSER);

    draw_hs_btn(r, hs, mouse_pt, center_x + center_w - 142, comp_y, 76, 32, HS_JS8_SEND_BTN, "TX JS8", hs.js8.tx_active, 0x10B981);
    draw_hs_btn(r, hs, mouse_pt, center_x + center_w - 62,  comp_y, 54, 32, HS_JS8_HALT_BTN, "Halt",   false,            0xEF4444);

    // ---- Right Column: Wide Heard Callsigns Grid + JS8 Store & Forward Inbox ----
    D2D1_RECT_F rcard = D2D1::RectF(right_x, mid_y, right_x + right_w, mid_y + mid_h);
    r.fill_round_rect(rcard, 8.0f, r.hex_color(0x0F172A));
    r.draw_round_rect(rcard, 8.0f, r.hex_color(0x1E293B), 1.2f);
    r.draw_text("HEARD STATIONS TELEMETRY GRID", r.font_ui_bold,
                D2D1::RectF(right_x + 10, mid_y + 4, right_x + right_w - 10, mid_y + 24), r.hex_color(0x00E5FF));

    D2D1_RECT_F rhdr = D2D1::RectF(right_x + 4, mid_y + 25, right_x + right_w - 4, mid_y + 44);
    r.fill_round_rect(rhdr, 3.0f, r.hex_color(0x090D16));
    r.draw_text("CALLSIGN  GRID   SNR   AF Hz   FRMS  LAST UTC",
                r.font_mono_small, D2D1::RectF(right_x + 8, mid_y + 25, right_x + right_w - 8, mid_y + 44), r.hex_color(0x38BDF8));

    {
        std::lock_guard<std::mutex> lk(hs.js8.mtx);
        float hy = mid_y + 46.0f;
        float hh = 21.0f;
        if (hs.js8.heard_stations.empty()) {
            r.draw_text("No stations heard yet.", r.font_mono_small,
                        D2D1::RectF(right_x + 10, hy, right_x + right_w - 10, mid_y + mid_h * 0.56f),
                        r.hex_color(0x64748B), DWRITE_TEXT_ALIGNMENT_CENTER);
        } else {
            for (size_t i = 0; i < hs.js8.heard_stations.size() && hy + hh < mid_y + mid_h * 0.58f; ++i) {
                const auto& st = hs.js8.heard_stations[i];
                D2D1_RECT_F sr = D2D1::RectF(right_x + 6, hy, right_x + right_w - 6, hy + hh - 1.0f);
                bool sel = (hs.js8.selected_call == st.callsign);
                bool wkd = hs.ft8.is_call_worked(st.callsign);
                if (wkd) {
                    r.fill_round_rect(sr, 4.0f, r.hex_color(0xEAB308, 0.38f));
                    r.draw_round_rect(sr, 4.0f, sel ? r.hex_color(0x00E5FF) : r.hex_color(0xFACC15, 0.85f), sel ? 1.4f : 1.0f);
                } else {
                    r.fill_round_rect(sr, 4.0f, sel ? r.hex_color(0x00E5FF, 0.22f) : r.hex_color(0x131C2E));
                }
                char sbuf[140];
                std::snprintf(sbuf, sizeof(sbuf), "%-8s  %-4s  %+3ddB  %4dHz   %2d   %s%s",
                              st.callsign.c_str(), st.grid.c_str(), st.snr_db, st.offset_hz, st.frame_count, st.last_utc.c_str(),
                              wkd ? " [WORKED]" : "");
                r.draw_text(sbuf, r.font_mono_small, D2D1::RectF(sr.left + 6, sr.top, sr.right - 6, sr.bottom),
                            sel ? r.hex_color(0x00E5FF) : wkd ? r.hex_color(0xFEF08A) : r.hex_color(0xE2E8F0));
                hs.add_hit(sr, HS_JS8_HEARD_ROW, static_cast<int>(i), st.callsign);
                hy += hh;
            }
        }

        float in_y = mid_y + mid_h * 0.60f;
        r.draw_text("JS8 INBOX & MESHCORE BRIDGE", r.font_ui_bold,
                    D2D1::RectF(right_x + 10, in_y, right_x + right_w - 10, in_y + 22), r.hex_color(0xA855F7));
        in_y += 24.0f;
        if (hs.js8.inbox.empty()) {
            r.draw_text("Inbox empty.", r.font_mono_small,
                        D2D1::RectF(right_x + 10, in_y, right_x + right_w - 10, mid_y + mid_h - 8),
                        r.hex_color(0x64748B), DWRITE_TEXT_ALIGNMENT_CENTER);
        } else {
            for (const auto& msg : hs.js8.inbox) {
                if (in_y + 34.0f > mid_y + mid_h - 4.0f) break;
                D2D1_RECT_F mr = D2D1::RectF(right_x + 6, in_y, right_x + right_w - 6, in_y + 32.0f);
                r.fill_round_rect(mr, 4.0f, r.hex_color(0x131C2E));
                r.draw_text(msg.from_call + " -> " + msg.to_call + (msg.bridged_to_mesh ? " [->BLE MESH]" : ""),
                            r.font_small, D2D1::RectF(mr.left + 6, mr.top + 1, mr.right - 6, mr.top + 16), r.hex_color(0x34D399));
                r.draw_text(msg.body, r.font_mono_small, D2D1::RectF(mr.left + 6, mr.top + 15, mr.right - 6, mr.bottom), r.hex_color(0xCBD5E1));
                in_y += 35.0f;
            }
        }
    }

    // 4. Bottom JS8 Waterfall (Resizable)
    render_wide_graph_splitter(r, hs, mouse_pt, x, y + h - wf_h - 9.0f, w, true);
    int js8_bw = static_cast<int>(js8::SUBMODES[static_cast<int>(hs.js8.current_submode)].bandwidth_hz);
    render_waterfall_widget(r, hs, mouse_pt, x, y + h - wf_h, w, wf_h,
                            hs.js8.rx_offset_hz, hs.js8.tx_offset_hz, js8_bw, HS_JS8_WATERFALL,
                            "JS8CALL PASSBAND WATERFALL \xe2\x80\xa2 Offset " + std::to_string(hs.js8.tx_offset_hz) + " Hz (" +
                            js8::SUBMODES[static_cast<int>(hs.js8.current_submode)].name + " " + std::to_string(js8_bw) + " Hz BW)");
}

// ============================================================================
// TAB 2: Fldigi CW (Morse Code) Adaptive SOM Modem, Keyer & Digiscope
// ============================================================================
template <typename RendererT>
inline void render_cw_fldigi_workspace(RendererT& r, HamShackSuite& hs, POINT mouse_pt,
                                       float x, float y, float w, float h) {
    // 1. Live G90 & Soundcard Telemetry Strip (38px)
    render_live_g90_telemetry_grid(r, hs, x, y, w, 38.0f);

    // 2. Wide 6x2 F1..F12 Programmable Fldigi CW Macro Grid with Expanded Text Preview (y + 42 .. y + 104)
    float mac_y = y + 42.0f;
    float mac_w = (w - 5 * 4.0f) / 6.0f;
    for (int m = 0; m < 12; ++m) {
        int col = m % 6;
        int row = m / 6;
        float mx = x + col * (mac_w + 4.0f);
        float my = mac_y + row * 30.0f;
        std::string exp = hs.cw.expand_macro(m);
        if (exp.size() > 18) exp = exp.substr(0, 18) + "..";
        std::string lbl = std::string(hs.cw.macros[m].key_label) + " " + hs.cw.macros[m].title + ": " + exp;
        draw_hs_btn(r, hs, mouse_pt, mx, my, mac_w, 28.0f, HS_CW_MACRO_BASE + m, lbl, false, 0xFBBF24, m, "", true);
    }

    // 3. Middle Split: Wide Decoded RX Morse & Live Timing Telemetry Grid (Left 62%) + CW Digiscope (Right 38%)
    float mid_y = mac_y + 64.0f;
    float ctrl_h = 80.0f;
    float max_wf_h = std::max(140.0f, (y + h) - mid_y - ctrl_h - 165.0f);
    float wf_h = std::clamp(hs.wide_graph_height, 110.0f, max_wf_h);
    float mid_h = std::max(145.0f, (y + h - wf_h - ctrl_h - 17.0f) - mid_y);
    float left_w = (w - 8.0f) * 0.62f;
    float right_w = w - left_w - 8.0f;

    D2D1_RECT_F rx_card = D2D1::RectF(x, mid_y, x + left_w, mid_y + mid_h);
    r.fill_round_rect(rx_card, 8.0f, r.hex_color(0x0F172A));
    r.draw_round_rect(rx_card, 8.0f, r.hex_color(0x1E293B), 1.2f);

    int dit_ms = static_cast<int>(std::round(1200.0 / std::max(5, hs.cw.tx_wpm)));
    int dah_ms = 3 * dit_ms;
    char cw_hdr_buf[180];
    std::snprintf(cw_hdr_buf, sizeof(cw_hdr_buf),
                  "FLDIGI CW SOM DECODER  |  TX %d WPM (Dit %dms / Dah %dms)  |  Spot %dHz  |  BW %dHz  |  Tone %.1fdBFS",
                  hs.cw.tx_wpm, dit_ms, dah_ms, hs.cw.pitch_hz, hs.cw.filter_bw_hz, hs.cw.live_tone_db);
    r.draw_text(cw_hdr_buf, r.font_ui_bold, D2D1::RectF(x + 10, mid_y + 4, x + left_w - 10, mid_y + 25), r.hex_color(0xFBBF24));

    D2D1_RECT_F el_bar = D2D1::RectF(x + 8, mid_y + 27, x + left_w - 8, mid_y + 49);
    r.fill_round_rect(el_bar, 4.0f, r.hex_color(0x05080F));
    r.draw_text("MORSE ELEMENTS:  " + (hs.cw.last_morse_elements.empty() ? "(waiting for live CW audio or keyer TX)" : hs.cw.last_morse_elements),
                r.font_mono_small, D2D1::RectF(el_bar.left + 8, el_bar.top, el_bar.right - 8, el_bar.bottom), r.hex_color(0x00E5FF));

    D2D1_RECT_F txt_box = D2D1::RectF(x + 8, mid_y + 52, x + left_w - 8, mid_y + mid_h - 6);
    r.fill_round_rect(txt_box, 6.0f, r.hex_color(0x05080F));
    {
        std::lock_guard<std::mutex> lk(hs.cw.mtx);
        if (hs.cw.rx_decoded_text.empty()) {
            r.draw_text(hs.flrig.connected
                            ? "Listening for CW tone at " + std::to_string(hs.cw.pitch_hz) + " Hz on USB Audio Device..."
                            : "XIEGU G90 OFFLINE — Connect G90 USB CAT & USB Audio Device",
                        r.font_mono_small, txt_box, r.hex_color(0x64748B), DWRITE_TEXT_ALIGNMENT_CENTER);
        } else {
            std::stringstream ss(hs.cw.rx_decoded_text);
            std::string line;
            std::vector<std::string> lines;
            while (std::getline(ss, line)) if (!line.empty()) lines.push_back(line);
            float ly = txt_box.top + 6.0f;
            float lh = 20.0f;
            int max_l = std::max(1, static_cast<int>((txt_box.bottom - txt_box.top - 10.0f) / lh));
            int start_l = std::max(0, static_cast<int>(lines.size()) - max_l);
            for (int i = start_l; i < static_cast<int>(lines.size()); ++i) {
                bool is_tx = lines[i].find("[TX ") != std::string::npos;
                r.draw_text(lines[i], r.font_mono, D2D1::RectF(txt_box.left + 8, ly, txt_box.right - 8, ly + lh),
                            is_tx ? r.hex_color(0xFBBF24) : r.hex_color(0x34D399));
                ly += lh;
            }
        }
    }

    // Right Card: Live CW Digiscope Envelope Trace
    float sx = x + left_w + 8.0f;
    D2D1_RECT_F scope_card = D2D1::RectF(sx, mid_y, sx + right_w, mid_y + mid_h);
    r.fill_round_rect(scope_card, 8.0f, r.hex_color(0x0F172A));
    r.draw_round_rect(scope_card, 8.0f, r.hex_color(0x1E293B), 1.2f);
    r.draw_text("LIVE CW GOERTZEL ENVELOPE DIGISCOPE", r.font_ui_bold,
                D2D1::RectF(sx + 12, mid_y + 4, sx + right_w - 12, mid_y + 25), r.hex_color(0x00E5FF));

    D2D1_RECT_F sbox = D2D1::RectF(sx + 10, mid_y + 28, sx + right_w - 10, mid_y + mid_h - 28);
    r.fill_round_rect(sbox, 6.0f, r.hex_color(0x05080F));
    r.draw_round_rect(sbox, 6.0f, r.hex_color(0x1E293B), 1.0f);

    float thresh_y = sbox.bottom - hs.cw.squelch_metric * (sbox.bottom - sbox.top - 12.0f) - 6.0f;
    r.fill_round_rect(D2D1::RectF(sbox.left + 4, thresh_y, sbox.right - 4, thresh_y + 1.2f), 0.0f, r.hex_color(0xF59E0B, 0.7f));

    float sw = sbox.right - sbox.left - 12.0f;
    float sh = sbox.bottom - sbox.top - 12.0f;
    float step_x = sw / static_cast<float>(cw_fldigi::CWModemEngine::SCOPE_LEN);
    for (int i = 0; i < cw_fldigi::CWModemEngine::SCOPE_LEN; ++i) {
        int ring_i = (hs.cw.scope_write_pos + i) % cw_fldigi::CWModemEngine::SCOPE_LEN;
        float v = hs.cw.scope_envelope[ring_i];
        if (v <= 0.005f) continue;
        float bh = v * sh;
        float bx_pos = sbox.left + 6.0f + i * step_x;
        r.fill_round_rect(D2D1::RectF(bx_pos, sbox.bottom - 6.0f - bh, bx_pos + std::max(1.8f, step_x), sbox.bottom - 6.0f),
                          0.0f, v > hs.cw.squelch_metric ? r.hex_color(0x10B981) : r.hex_color(0x38BDF8, 0.4f));
    }
    r.draw_text("Squelch Threshold: " + std::to_string(static_cast<int>(hs.cw.squelch_metric * 100)) + "%  |  Key: " +
                (hs.cw.live_key_down ? "MARK (KEY DOWN)" : "SPACE"),
                r.font_mono_small, D2D1::RectF(sx + 12, mid_y + mid_h - 24, sx + right_w - 12, mid_y + mid_h - 4),
                hs.cw.live_key_down ? r.hex_color(0x10B981) : r.hex_color(0x94A3B8));

    // 4. Keyer Controls + TX Keyboard Buffer
    float cy = mid_y + mid_h + 6.0f;
    D2D1_RECT_F ccard = D2D1::RectF(x, cy, x + w, cy + ctrl_h);
    r.fill_round_rect(ccard, 8.0f, r.hex_color(0x0F172A));
    r.draw_round_rect(ccard, 8.0f, r.hex_color(0x1E293B), 1.2f);

    float kx = x + 10.0f;
    float ky = cy + 6.0f;
    draw_hs_btn(r, hs, mouse_pt, kx,       ky, 40, 26, HS_CW_WPM_M5, "-5", false, 0x38BDF8, 0, "", true);
    draw_hs_btn(r, hs, mouse_pt, kx + 43,  ky, 36, 26, HS_CW_WPM_M1, "-1", false, 0x38BDF8, 0, "", true);
    r.draw_text(std::to_string(hs.cw.tx_wpm) + " WPM", r.font_ui_bold, D2D1::RectF(kx + 82, ky, kx + 150, ky + 26),
                r.hex_color(0xFBBF24), DWRITE_TEXT_ALIGNMENT_CENTER);
    draw_hs_btn(r, hs, mouse_pt, kx + 152, ky, 36, 26, HS_CW_WPM_P1, "+1", false, 0x38BDF8, 0, "", true);
    draw_hs_btn(r, hs, mouse_pt, kx + 191, ky, 40, 26, HS_CW_WPM_P5, "+5", false, 0x38BDF8, 0, "", true);

    draw_hs_btn(r, hs, mouse_pt, kx + 240, ky, 44, 26, HS_CW_PITCH_M25, "-25", false, 0x00E5FF, 0, "", true);
    r.draw_text("Pitch " + std::to_string(hs.cw.pitch_hz) + "Hz", r.font_ui_bold,
                D2D1::RectF(kx + 286, ky, kx + 386, ky + 26), r.hex_color(0x00E5FF), DWRITE_TEXT_ALIGNMENT_CENTER);
    draw_hs_btn(r, hs, mouse_pt, kx + 388, ky, 44, 26, HS_CW_PITCH_P25, "+25", false, 0x00E5FF, 0, "", true);

    draw_hs_btn(r, hs, mouse_pt, kx + 440, ky, 102, 26, HS_CW_BW_CYCLE, "BW: " + std::to_string(hs.cw.filter_bw_hz) + "Hz", true, 0x10B981, 0, "", true);
    draw_hs_btn(r, hs, mouse_pt, kx + 548, ky, 76,  26, HS_CW_QSK_TOGGLE, "QSK", hs.cw.qsk_enabled, 0x10B981, 0, "", true);
    draw_hs_btn(r, hs, mouse_pt, kx + 628, ky, 76,  26, HS_CW_AFC_TOGGLE, "AFC", hs.cw.afc_enabled, 0x00E5FF, 0, "", true);
    draw_hs_btn(r, hs, mouse_pt, kx + 708, ky, 100, 26, HS_CW_PROSIGN_TOG, "Prosigns", hs.cw.prosigns_enabled, 0xA855F7, 0, "", true);
    draw_hs_btn(r, hs, mouse_pt, kx + 812, ky, 58,  26, HS_CW_SQL_DOWN,    "-SQL", false, 0xF59E0B, 0, "", true);
    draw_hs_btn(r, hs, mouse_pt, kx + 874, ky, 58,  26, HS_CW_SQL_UP,      "+SQL", false, 0xF59E0B, 0, "", true);
    draw_hs_btn(r, hs, mouse_pt, kx + 938, ky, 72,  26, HS_CW_CLEAR_RX,    "Clear", false, 0x94A3B8, 0, "", true);

    float ky2 = ky + 34.0f;
    D2D1_RECT_F cw_comp = D2D1::RectF(kx, ky2, x + w - 134, ky2 + 32);
    bool cw_foc = (hs.text_focus == HsTextFocus::CwComposer);
    r.fill_round_rect(cw_comp, 5.0f, r.hex_color(0x05080F));
    r.draw_round_rect(cw_comp, 5.0f, cw_foc ? r.hex_color(0xFBBF24) : r.hex_color(0x334155), 1.3f);
    std::string cw_prompt = hs.cw.tx_buffer_text.empty() && !cw_foc
                                ? "Click to type CW Keyboard Keyer text (or click F1..F12 macro above)..."
                                : (hs.cw.tx_buffer_text + (cw_foc ? "_" : ""));
    r.draw_text(cw_prompt, r.font_mono, D2D1::RectF(cw_comp.left + 8, cw_comp.top, cw_comp.right - 8, cw_comp.bottom),
                hs.cw.tx_buffer_text.empty() && !cw_foc ? r.hex_color(0x64748B) : r.hex_color(0xFBBF24));
    hs.add_hit(cw_comp, HS_CW_COMPOSER);
    draw_hs_btn(r, hs, mouse_pt, x + w - 126, ky2, 116, 32, HS_CW_TX_NOW, "KEY CW TX", hs.cw.tx_active, 0xEF4444);

    // 5. Bottom CW Waterfall (Resizable)
    render_wide_graph_splitter(r, hs, mouse_pt, x, y + h - wf_h - 9.0f, w, true);
    render_waterfall_widget(r, hs, mouse_pt, x, y + h - wf_h, w, wf_h,
                            hs.cw.pitch_hz - hs.cw.filter_bw_hz / 2,
                            hs.cw.pitch_hz - hs.cw.filter_bw_hz / 2,
                            hs.cw.filter_bw_hz, HS_CW_WATERFALL,
                            "FLDIGI CW WATERFALL \xe2\x80\xa2 Spot Tone " + std::to_string(hs.cw.pitch_hz) + " Hz \xe2\x80\xa2 Filter BW " +
                            std::to_string(hs.cw.filter_bw_hz) + " Hz [Click to QSY CW Pitch]");
}

// ============================================================================
// TAB 5: Maps & GridTracker Workspace (Maidenhead Grid Map, Arcs & Web Views)
// Placed BEFORE the FLRig Tab per user specification.
// Sub-tab 0: GridTracker interactive QSO map with Maidenhead 4-char grid square
//            overlays, great-circle QSO flight arcs, solar grayline, ADIF log drawer,
//            and full feature panel down the side of the window.
// Sub-tabs 1..4: PSKReporter, DXView HF, DXLook, OpenHamClock with persistent
//                cookies and state stored live in WKWebView.
// ============================================================================
// TAB 4: Native GridTracker Live Maidenhead Grid Map & ADIF Station Roster
// ============================================================================
template <typename RendererT, typename AppT>
inline void render_gridtracker_workspace(RendererT& r, AppT& app, HamShackSuite& hs, POINT mouse_pt,
                                         float x, float y, float w, float h) {
    // 1. Live G90 & Soundcard Telemetry Strip (38px)
    render_live_g90_telemetry_grid(r, hs, x, y, w, 38.0f);

    float top_y = y + 42.0f;
    float tot_h = h - 42.0f;

    // Ensure background web views are hidden when viewing native GridTracker
    hamshack_web::get_web_maps().hide_all();

    // Local snapshot of FT8 decodes and logbook grids for thread-safe lock-free rendering
    static thread_local std::vector<hamshack::DecodedFtMessage> ft_act_copy;
    static thread_local std::unordered_set<std::string> worked_grids_copy;
    static thread_local std::unordered_set<std::string> conf_grids_copy;
    ft_act_copy.clear();
    worked_grids_copy.clear();
    conf_grids_copy.clear();
    {
        std::lock_guard<std::recursive_mutex> lk(hs.ft8.mtx);
        ft_act_copy.assign(hs.ft8.band_activity.begin(), hs.ft8.band_activity.end());
        worked_grids_copy = hs.ft8.worked_grids;
        conf_grids_copy = hs.ft8.confirmed_grids;
    }

        float sb_w = (w > 860.0f) ? 310.0f : 240.0f;
        float map_w = w - sb_w - 8.0f;
        float ros_h = hs.gt_show_roster ? std::min(185.0f, tot_h * 0.38f) : 0.0f;
        float map_h = tot_h - ros_h - (hs.gt_show_roster ? 6.0f : 0.0f);

        // ---- Map Surface ----
        D2D1_RECT_F map_r = D2D1::RectF(x, top_y, x + map_w, top_y + map_h);
        hs.gt_map_rect = map_r;
        hs.add_hit(map_r, HS_GT_MAP_CANVAS);

        r.fill_round_rect(map_r, 8.0f, r.hex_color(0x0A0F1D));
        r.draw_round_rect(map_r, 8.0f, r.hex_color(0x1E293B), 1.2f);

        r.rt->PushAxisAlignedClip(map_r, D2D1_ANTIALIAS_MODE_ALIASED);

        // Web-Mercator map projection parameters
        const int z = hs.gt_map_zoom;
        const int n_tiles = 1 << z;
        const double ws = 256.0 * n_tiles;
        const double cx = (map_r.left + map_r.right) * 0.5;
        const double cy = (map_r.top + map_r.bottom) * 0.5;
        const double cam_x = hs.gt_map_nx * ws;
        const double cam_y = hs.gt_map_ny * ws;

        auto to_screen = [&](double nx, double ny) -> D2D1_POINT_2F {
            double dx = nx - hs.gt_map_nx;
            if (dx > 0.5) dx -= 1.0;
            else if (dx < -0.5) dx += 1.0;
            return D2D1::Point2F(static_cast<float>(cx + dx * ws), static_cast<float>(cy + (ny - hs.gt_map_ny) * ws));
        };
        auto geo_pt = [&](double lat, double lon) -> D2D1_POINT_2F {
            return to_screen(lon_to_nx(lon), lat_to_ny(lat));
        };

        // Render OSM tiles
        int tx0 = static_cast<int>(std::floor((cam_x - (cx - map_r.left)) / 256.0));
        int tx1 = static_cast<int>(std::floor((cam_x + (map_r.right - cx)) / 256.0));
        int ty0 = std::max(0, static_cast<int>(std::floor((cam_y - (cy - map_r.top)) / 256.0)));
        int ty1 = std::min(n_tiles - 1, static_cast<int>(std::floor((cam_y + (map_r.bottom - cy)) / 256.0)));

        for (int ty = ty0; ty <= ty1; ++ty) {
            for (int tx = tx0; tx <= tx1; ++tx) {
                int wx_tile = ((tx % n_tiles) + n_tiles) % n_tiles;
                float sx = static_cast<float>(cx + tx * 256.0 - cam_x);
                float sy = static_cast<float>(cy + ty * 256.0 - cam_y);
                D2D1_RECT_F dst = D2D1::RectF(sx, sy, sx + 256.0f, sy + 256.0f);
                if (ID2D1Bitmap* bmp = r.get_tile(tile_key(0, z, wx_tile, ty))) {
                    r.rt->DrawBitmap(bmp, dst, 1.0f, D2D1_BITMAP_INTERPOLATION_MODE_LINEAR);
                    continue;
                }
                g_tiles.request(0, z, wx_tile, ty);
                for (int up = 1; up <= 6 && z - up >= 0; ++up) {
                    auto it = r.tile_bitmaps.find(tile_key(0, z - up, wx_tile >> up, ty >> up));
                    if (it == r.tile_bitmaps.end()) continue;
                    it->second.last_used = r.frame_no;
                    float sub = 256.0f / static_cast<float>(1 << up);
                    float ox = static_cast<float>(wx_tile & ((1 << up) - 1)) * sub;
                    float oy = static_cast<float>(ty & ((1 << up) - 1)) * sub;
                    D2D1_RECT_F src = D2D1::RectF(ox, oy, ox + sub, oy + sub);
                    r.rt->DrawBitmap(it->second.bmp, dst, 1.0f, D2D1_BITMAP_INTERPOLATION_MODE_LINEAR, &src);
                    break;
                }
            }
        }

        // Night dark-tint overlay so maps match the dark UI
        if (!r.light_mode) {
            r.fill_rect(map_r, r.hex_color(0x0B1220, 0.54f));
        }

        // Solar Grayline Terminator shading
        if (hs.gt_show_grayline) {
            std::time_t now_t = std::time(nullptr);
            std::tm gmt_tm{};
#ifdef _WIN32
            gmtime_s(&gmt_tm, &now_t);
#else
            gmtime_r(&now_t, &gmt_tm);
#endif
            double utc_hrs = gmt_tm.tm_hour + gmt_tm.tm_min / 60.0 + gmt_tm.tm_sec / 3600.0;
            double subsolar_lon = (12.0 - utc_hrs) * 15.0;
            double decl_deg = -23.44 * std::cos(2.0 * 3.1415926535 * (gmt_tm.tm_yday + 10) / 365.25);
            double decl_rad = decl_deg * 3.1415926535 / 180.0;

            for (double g_lon = -180.0; g_lon < 180.0; g_lon += 4.0) {
                double h_angle = (g_lon - subsolar_lon) * 3.1415926535 / 180.0;
                double tan_lat = (std::abs(std::tan(decl_rad)) > 1e-4) ? (-std::cos(h_angle) / std::tan(decl_rad)) : 0.0;
                double term_lat = std::atan(tan_lat) * 180.0 / 3.1415926535;
                if (!std::isfinite(term_lat)) term_lat = 0.0;
                term_lat = std::clamp(term_lat, -84.0, 84.0);
                double n_lat0 = std::clamp((decl_deg >= 0.0) ? -84.0 : term_lat, -84.0, 84.0);
                double n_lat1 = std::clamp((decl_deg >= 0.0) ? term_lat : 84.0, -84.0, 84.0);
                if (n_lat1 > n_lat0) {
                    D2D1_POINT_2F p1 = geo_pt(n_lat1, g_lon);
                    D2D1_POINT_2F p2 = geo_pt(n_lat0, g_lon + 4.0);
                    float rx1 = std::min(p1.x, p2.x);
                    float rx2 = std::max(p1.x, p2.x);
                    float ry1 = std::min(p1.y, p2.y);
                    float ry2 = std::max(p1.y, p2.y);
                    if (rx2 > map_r.left && rx1 < map_r.right) {
                        r.fill_round_rect(D2D1::RectF(std::max(rx1, map_r.left), std::max(ry1, map_r.top),
                                                      std::min(rx2, map_r.right), std::min(ry2, map_r.bottom)),
                                          0.0f, r.hex_color(0x020617, 0.42f));
                    }
                }
            }
        }

        // Maidenhead 4-Character Grid Squares (2° lon x 1° lat)
        auto draw_grid_box = [&](const std::string& g, uint32_t fill_col, float fill_a, uint32_t border_col, const std::string& label) {
            if (g.size() < 4) return;
            char c0 = std::toupper(static_cast<unsigned char>(g[0]));
            char c1 = std::toupper(static_cast<unsigned char>(g[1]));
            char c2 = g[2], c3 = g[3];
            if (c0 < 'A' || c0 > 'R' || c1 < 'A' || c1 > 'R' || c2 < '0' || c2 > '9' || c3 < '0' || c3 > '9') return;

            double g_lon0 = (c0 - 'A') * 20.0 - 180.0 + (c2 - '0') * 2.0;
            double g_lat0 = (c1 - 'A') * 10.0 - 90.0  + (c3 - '0') * 1.0;
            D2D1_POINT_2F p_tl = geo_pt(g_lat0 + 1.0, g_lon0);
            D2D1_POINT_2F p_br = geo_pt(g_lat0, g_lon0 + 2.0);
            D2D1_RECT_F g_box = D2D1::RectF(std::min(p_tl.x, p_br.x), std::min(p_tl.y, p_br.y),
                                            std::max(p_tl.x, p_br.x), std::max(p_tl.y, p_br.y));

            if (g_box.right > map_r.left && g_box.left < map_r.right &&
                g_box.bottom > map_r.top && g_box.top < map_r.bottom) {
                r.fill_round_rect(g_box, 0.0f, r.hex_color(fill_col, fill_a));
                r.draw_round_rect(g_box, 0.0f, r.hex_color(border_col, 0.85f), 1.0f);
                if ((g_box.right - g_box.left) > 18.0f && (g_box.bottom - g_box.top) > 12.0f) {
                    r.draw_text(label.empty() ? g.substr(0, 4) : label, r.font_mono_small, g_box,
                                r.hex_color(border_col), DWRITE_TEXT_ALIGNMENT_CENTER);
                }
            }
        };

        // Helper to parse callsign and grid from FT8/FT4 message text
        auto extract_call_grid = [](const std::string& msg, std::string& call_out, std::string& grid_out) {
            hamshack::Ft8Ft4Engine::extract_spot_info(msg, call_out, grid_out);
        };

        if (hs.gt_show_grids) {
            // Draw previous worked contacts from imported ADIF in yellow (#EEEE00)
            for (const auto& w_grid : worked_grids_copy) {
                draw_grid_box(w_grid, 0xEEEE00, 0.36f, 0xFACC15, w_grid);
            }
            // Draw confirmed QSL contacts in red (#DD0000)
            for (const auto& q_grid : conf_grids_copy) {
                draw_grid_box(q_grid, 0xDD0000, 0.42f, 0xEF4444, q_grid);
            }
            // Draw live decoded FT8 / JS8 contacts (green for CQ, cyan for live QSO)
            for (const auto& msg : ft_act_copy) {
                std::string c_out, g_out;
                extract_call_grid(msg.text, c_out, g_out);
                if (g_out.size() >= 4) {
                    bool is_cq = (msg.text.find("CQ ") != std::string::npos);
                    draw_grid_box(g_out, is_cq ? 0x22C55E : 0x06B6D4, 0.36f,
                                  is_cq ? 0x10B981 : 0x00E5FF, g_out + (is_cq ? " CQ" : ""));
                }
            }
            // Draw home QTH grid in high-contrast orange (#FFA500)
            if (!hs.hf_grid.empty()) {
                draw_grid_box(hs.hf_grid, 0xF97316, 0.50f, 0xEA580C, hs.hf_grid + " [QTH]");
            }
        }

        // Curved Great-Circle QSO Arcs
        if (hs.gt_show_arcs) {
            double qth_lat = 0, qth_lon = 0;
            if (hamshack::Ft8Ft4Engine::grid_to_latlon(hs.hf_grid, qth_lat, qth_lon)) {
                auto draw_arc = [&](double d_lat, double d_lon, uint32_t col, float alpha) {
                    D2D1_POINT_2F p0 = geo_pt(qth_lat, qth_lon);
                    D2D1_POINT_2F p1 = geo_pt(d_lat, d_lon);
                    float mid_x = (p0.x + p1.x) * 0.5f;
                    float mid_y = (p0.y + p1.y) * 0.5f - std::min(45.0f, std::abs(p1.x - p0.x) * 0.18f);

                    D2D1_POINT_2F prev = p0;
                    D2D1_COLOR_F arc_col = r.hex_color(col, alpha);
                    for (int s = 1; s <= 12; ++s) {
                        float t = s / 12.0f;
                        float cur_x = (1 - t) * (1 - t) * p0.x + 2 * (1 - t) * t * mid_x + t * t * p1.x;
                        float cur_y = (1 - t) * (1 - t) * p0.y + 2 * (1 - t) * t * mid_y + t * t * p1.y;
                        D2D1_POINT_2F curr = D2D1::Point2F(cur_x, cur_y);
                        r.draw_line(prev, curr, arc_col, 1.6f);
                        prev = curr;
                    }
                };

                // Draw live arcs for active FT8 decodes
                for (const auto& msg : ft_act_copy) {
                    std::string c_out, g_out;
                    extract_call_grid(msg.text, c_out, g_out);
                    double dlat = 0, dlon = 0;
                    if (!g_out.empty() && hamshack::Ft8Ft4Engine::grid_to_latlon(g_out, dlat, dlon)) {
                        bool is_cq = (msg.text.find("CQ ") != std::string::npos);
                        draw_arc(dlat, dlon, is_cq ? 0x10B981 : 0x00E5FF, 0.85f);
                    }
                }
            }
        }

        // Station Push-Pins
        if (hs.gt_show_pins) {
            double qlat = 0, qlon = 0;
            if (hamshack::Ft8Ft4Engine::grid_to_latlon(hs.hf_grid, qlat, qlon)) {
                D2D1_POINT_2F qpt = geo_pt(qlat, qlon);
                r.fill_round_rect(D2D1::RectF(qpt.x - 5, qpt.y - 5, qpt.x + 5, qpt.y + 5), 5.0f, r.hex_color(0xEA580C));
                r.draw_text("★ " + hs.hf_callsign + " (" + hs.hf_grid + ")", r.font_ui_bold,
                            D2D1::RectF(qpt.x + 8, qpt.y - 10, qpt.x + 180, qpt.y + 10), r.hex_color(0xFBBF24));
            }
            // Decoded live stations pins
            for (const auto& msg : ft_act_copy) {
                std::string c_out, g_out;
                extract_call_grid(msg.text, c_out, g_out);
                double dlat = 0, dlon = 0;
                if (!g_out.empty() && hamshack::Ft8Ft4Engine::grid_to_latlon(g_out, dlat, dlon)) {
                    D2D1_POINT_2F pt = geo_pt(dlat, dlon);
                    bool is_cq = (msg.text.find("CQ ") != std::string::npos);
                    bool wkd = hs.ft8.is_call_worked(c_out);
                    uint32_t col = wkd ? 0xFACC15 : is_cq ? 0x10B981 : 0x00E5FF;
                    r.fill_round_rect(D2D1::RectF(pt.x - 4, pt.y - 4, pt.x + 4, pt.y + 4), 4.0f, r.hex_color(col));
                    r.draw_text(c_out + (wkd ? " [W]" : ""), r.font_mono_small,
                                D2D1::RectF(pt.x + 6, pt.y - 8, pt.x + 100, pt.y + 8), r.hex_color(col));
                }
            }
            // Forwarded APRS GPS Tracker Stations (from AprsEngine)
            if (hs.aprs) {
                for (const auto& st : hs.aprs->stations()) {
                    if (!st.has_position) continue;
                    D2D1_POINT_2F pt = geo_pt(st.latitude, st.longitude);
                    r.fill_round_rect(D2D1::RectF(pt.x - 5, pt.y - 5, pt.x + 5, pt.y + 5), 5.0f, r.hex_color(0x22D3EE));
                    r.draw_round_rect(D2D1::RectF(pt.x - 6, pt.y - 6, pt.x + 6, pt.y + 6), 6.0f, r.hex_color(0x020617), 1.2f);
                    std::string lbl = st.callsign_ssid + " [APRS]";
                    r.draw_text(lbl, r.font_mono_small,
                                D2D1::RectF(pt.x + 7, pt.y - 9, pt.x + 150, pt.y + 9), r.hex_color(0x22D3EE));
                }
            }
        }

        r.rt->PopAxisAlignedClip();

        // Floating Map Controls (top-right of map)
        float fbx = map_r.right - 210.0f;
        float fby = map_r.top + 8.0f;
        draw_hs_btn(r, hs, mouse_pt, fbx + 0*48, fby, 44, 25, HS_GT_ZOOM_IN,    "[+]",   false, 0x00E5FF, 0, "", true);
        draw_hs_btn(r, hs, mouse_pt, fbx + 1*48, fby, 44, 25, HS_GT_ZOOM_OUT,   "[-]",   false, 0x00E5FF, 0, "", true);
        draw_hs_btn(r, hs, mouse_pt, fbx + 2*48, fby, 54, 25, HS_GT_CENTER_QTH, "QTH",   false, 0xF97316, 0, "", true);
        draw_hs_btn(r, hs, mouse_pt, fbx + 3*50, fby, 56, 25, HS_GT_FIT_WORLD,  "World", false, 0x38BDF8, 0, "", true);

        // Map Color Legend (bottom-left of map)
        if (hs.gt_show_legend) {
            D2D1_RECT_F leg_box = D2D1::RectF(map_r.left + 10.0f, map_r.bottom - 118.0f, map_r.left + 175.0f, map_r.bottom - 10.0f);
            r.fill_round_rect(leg_box, 6.0f, r.hex_color(0x0F172A, 0.90f));
            r.draw_round_rect(leg_box, 6.0f, r.hex_color(0x334155), 1.0f);
            r.draw_text("GRIDTRACKER LEGEND", r.font_ui_bold,
                        D2D1::RectF(leg_box.left + 8, leg_box.top + 4, leg_box.right - 8, leg_box.top + 20), r.hex_color(0xFBBF24));

            struct LegItem { uint32_t col; const char* txt; };
            static constexpr LegItem kLeg[5] = {
                {0xF97316, "My Station QTH"},
                {0xFACC15, "Worked (.ADI Log)"},
                {0xEF4444, "Confirmed QSL"},
                {0x10B981, "Calling CQ (Live)"},
                {0x00E5FF, "Live Traffic"}
            };
            float ly = leg_box.top + 24.0f;
            for (int i = 0; i < 5; ++i) {
                r.fill_round_rect(D2D1::RectF(leg_box.left + 8, ly + 2, leg_box.left + 18, ly + 12), 2.0f, r.hex_color(kLeg[i].col));
                r.draw_text(kLeg[i].txt, r.font_mono_small,
                            D2D1::RectF(leg_box.left + 24, ly, leg_box.right - 6, ly + 16), r.hex_color(0xE2E8F0));
                ly += 17.0f;
            }
        }

        // ---- Bottom Call Roster & ADIF Worked Log Drawer ----
        if (hs.gt_show_roster) {
            float ry = top_y + map_h + 4.0f;
            D2D1_RECT_F ros_card = D2D1::RectF(x, ry, x + map_w, ry + ros_h);
            r.fill_round_rect(ros_card, 8.0f, r.hex_color(0x0F172A));
            r.draw_round_rect(ros_card, 8.0f, r.hex_color(0x1E293B), 1.2f);

            r.draw_text("GRIDTRACKER CALL ROSTER & WORKED ADIF LOG  (Click row to select DX QSO)", r.font_ui_bold,
                        D2D1::RectF(x + 10, ry + 4, x + map_w - 10, ry + 24), r.hex_color(0x38BDF8));

            D2D1_RECT_F rhdr = D2D1::RectF(x + 6, ry + 24, x + map_w - 6, ry + 42);
            r.fill_round_rect(rhdr, 3.0f, r.hex_color(0x090D16));
            r.draw_text("UTC       MODE   CALLSIGN    GRID     SNR    DETAILS / QSO TRANSCRIPT",
                        r.font_mono_small, D2D1::RectF(x + 10, ry + 25, x + map_w - 10, ry + 42), r.hex_color(0x38BDF8));

            float row_y = ry + 44.0f;
            float r_h = 20.0f;
            int r_idx = 0;

            // Render live FT8 band activity in roster
            for (int i = static_cast<int>(ft_act_copy.size()) - 1; i >= 0 && row_y + r_h < ros_card.bottom - 4.0f; --i) {
                const auto& m = ft_act_copy[i];
                std::string c_out, g_out;
                extract_call_grid(m.text, c_out, g_out);
                D2D1_RECT_F rr = D2D1::RectF(x + 6, row_y, x + map_w - 6, row_y + r_h - 1.0f);
                bool wkd = hs.ft8.is_call_worked(c_out);
                if (wkd) {
                    // Yellow highlight behind previously worked contacts
                    r.fill_round_rect(rr, 3.0f, r.hex_color(0xEAB308, 0.38f));
                    r.draw_round_rect(rr, 3.0f, r.hex_color(0xFACC15, 0.85f), 1.0f);
                } else if (r_idx % 2 == 1) {
                    r.fill_round_rect(rr, 3.0f, r.hex_color(0x0B111E));
                }
                char r_buf[160];
                std::snprintf(r_buf, sizeof(r_buf), "%-8s  %-5s  %-10s  %-6s  %+3ddB  %s%s",
                              m.utc_str.c_str(), hs.ft8.is_ft4 ? "FT4" : "FT8", c_out.c_str(),
                              g_out.c_str(), m.snr_db, m.text.c_str(), wkd ? " [WORKED]" : "");
                r.draw_text(r_buf, r.font_mono_small, D2D1::RectF(rr.left + 6, rr.top, rr.right - 6, rr.bottom),
                            wkd ? r.hex_color(0xFEF08A) : r.hex_color(0xE2E8F0));
                hs.add_hit(rr, HS_GT_ROSTER_ROW, r_idx, c_out);
                row_y += r_h;
                ++r_idx;
            }
        }

        // ---- Right Sidebar: GridTracker Features & ADIF Importer ----
        float sb_x = x + map_w + 6.0f;
        D2D1_RECT_F sb = D2D1::RectF(sb_x, top_y, sb_x + sb_w, top_y + tot_h);
        r.fill_round_rect(sb, 8.0f, r.hex_color(0x0F172A));
        r.draw_round_rect(sb, 8.0f, r.hex_color(0x1E293B), 1.2f);

        r.draw_text("GRIDTRACKER & ADIF LOGBOOK", r.font_ui_bold,
                    D2D1::RectF(sb_x + 10, top_y + 6, sb_x + sb_w - 10, top_y + 26), r.hex_color(0xF59E0B));

        float sy = top_y + 30.0f;

        // Radio Telemetry Card inside Sidebar
        D2D1_RECT_F r_card = D2D1::RectF(sb_x + 8, sy, sb_x + sb_w - 8, sy + 58);
        r.fill_round_rect(r_card, 5.0f, r.hex_color(0x05080F));
        r.draw_round_rect(r_card, 5.0f, r.hex_color(0x1E293B), 1.0f);
        r.draw_text("VFO: " + hamshack::FlrigCatEngine::format_freq_mhz(hs.flrig.vfo_a_hz) + " MHz  [" +
                    hamshack::FlrigCatEngine::mode_name(hs.flrig.mode_a) + "]",
                    r.font_mono_small, D2D1::RectF(r_card.left + 8, r_card.top + 4, r_card.right - 8, r_card.top + 20),
                    hs.flrig.connected ? r.hex_color(0x10B981) : r.hex_color(0xEF4444));
        r.draw_text("QTH: " + hs.hf_callsign + "  Grid: " + hs.hf_grid, r.font_mono_small,
                    D2D1::RectF(r_card.left + 8, r_card.top + 21, r_card.right - 8, r_card.top + 37), r.hex_color(0x00E5FF));
        r.draw_text(hs.flrig.connected ? "Xiegu G90 CAT ONLINE" : "Xiegu G90 CAT OFFLINE", r.font_mono_small,
                    D2D1::RectF(r_card.left + 8, r_card.top + 38, r_card.right - 8, r_card.top + 54),
                    hs.flrig.connected ? r.hex_color(0x34D399) : r.hex_color(0x64748B));
        sy += 64.0f;

        // ADIF Import & Sync Buttons
        draw_hs_btn(r, hs, mouse_pt, sb_x + 8, sy, (sb_w - 20) * 0.52f, 28, HS_GT_IMPORT_ADI, "Import .ADI...", false, 0xFBBF24, 0, "", true);
        draw_hs_btn(r, hs, mouse_pt, sb_x + 12 + (sb_w - 20) * 0.52f, sy, (sb_w - 20) * 0.48f, 28, HS_GT_SYNC_WSJTX_ADI, "Sync WSJT-X", false, 0x10B981, 0, "", true);
        sy += 34.0f;

        // ADIF Quick Stats Box
        D2D1_RECT_F s_box = D2D1::RectF(sb_x + 8, sy, sb_x + sb_w - 8, sy + 92);
        r.fill_round_rect(s_box, 5.0f, r.hex_color(0x05080F));
        r.draw_round_rect(s_box, 5.0f, r.hex_color(0x1E293B), 1.0f);
        r.draw_text("LOGBOOK STATISTICS (ADIF):", r.font_small,
                    D2D1::RectF(s_box.left + 8, s_box.top + 4, s_box.right - 8, s_box.top + 20), r.hex_color(0xFBBF24));
        r.draw_text("Imported QSOs:  " + std::to_string(hs.ft8.imported_qso_keys.size()), r.font_mono_small,
                    D2D1::RectF(s_box.left + 8, s_box.top + 22, s_box.right - 8, s_box.top + 38), r.hex_color(0xE2E8F0));
        r.draw_text("Worked Calls:   " + std::to_string(hs.ft8.worked_calls.size()) +
                    "  |  Grids: " + std::to_string(hs.ft8.worked_grids.size()), r.font_mono_small,
                    D2D1::RectF(s_box.left + 8, s_box.top + 39, s_box.right - 8, s_box.top + 55), r.hex_color(0x38BDF8));
        r.draw_text("Confirmed QSL:  " + std::to_string(hs.ft8.confirmed_grids.size()) + " Grids", r.font_mono_small,
                    D2D1::RectF(s_box.left + 8, s_box.top + 56, s_box.right - 8, s_box.top + 72), r.hex_color(0x34D399));
        r.draw_text(hs.ft8.adif_status_text, r.font_mono_small,
                    D2D1::RectF(s_box.left + 8, s_box.top + 73, s_box.right - 8, s_box.top + 89), r.hex_color(0x94A3B8));
        sy += 98.0f;

        // View Filters Section
        r.draw_text("GRIDTRACKER VIEW FILTERS:", r.font_small,
                    D2D1::RectF(sb_x + 10, sy, sb_x + sb_w - 10, sy + 18), r.hex_color(0x38BDF8));
        sy += 20.0f;

        const char* b_lbl = (hs.gt_band_filter == 0) ? "Band: All Bands" : "Band: Active VFO";
        const char* m_lbl = (hs.gt_mode_filter == 0) ? "Mode: All Modes" : (hs.gt_mode_filter == 1 ? "Mode: FT8/FT4" : "Mode: JS8Call");
        draw_hs_btn(r, hs, mouse_pt, sb_x + 8, sy, (sb_w - 20) * 0.5f, 26, HS_GT_FILT_BAND, b_lbl, false, 0x00E5FF, 0, "", true);
        draw_hs_btn(r, hs, mouse_pt, sb_x + 12 + (sb_w - 20) * 0.5f, sy, (sb_w - 20) * 0.5f, 26, HS_GT_FILT_MODE, m_lbl, false, 0x00E5FF, 0, "", true);
        sy += 30.0f;

        const char* d_lbl = (hs.gt_data_filter == 0) ? "Data: Live + .ADI" : (hs.gt_data_filter == 1 ? "Data: Live Only" : "Data: .ADI Only");
        const char* a_lbl = (hs.gt_award_filter == 0) ? "Award: All Grids" : (hs.gt_award_filter == 1 ? "Award: Worked Only" : "Award: New Grids");
        draw_hs_btn(r, hs, mouse_pt, sb_x + 8, sy, (sb_w - 20) * 0.5f, 26, HS_GT_FILT_DATA, d_lbl, false, 0x10B981, 0, "", true);
        draw_hs_btn(r, hs, mouse_pt, sb_x + 12 + (sb_w - 20) * 0.5f, sy, (sb_w - 20) * 0.5f, 26, HS_GT_FILT_AWARD, a_lbl, false, 0x10B981, 0, "", true);
        sy += 34.0f;

        // Feature Toggles 2x5 Grid
        r.draw_text("FEATURE DISPLAY TOGGLES:", r.font_small,
                    D2D1::RectF(sb_x + 10, sy, sb_x + sb_w - 10, sy + 18), r.hex_color(0xFBBF24));
        sy += 20.0f;

        float f_w = (sb_w - 20.0f) * 0.5f;
        draw_hs_btn(r, hs, mouse_pt, sb_x + 8, sy, f_w, 26, HS_GT_ROSTER_TOGGLE, "Call Roster", hs.gt_show_roster, 0x38BDF8, 0, "", true);
        draw_hs_btn(r, hs, mouse_pt, sb_x + 12 + f_w, sy, f_w, 26, HS_GT_GRIDS_TOGGLE, "Grid Squares", hs.gt_show_grids, 0xF59E0B, 0, "", true);
        sy += 30.0f;

        draw_hs_btn(r, hs, mouse_pt, sb_x + 8, sy, f_w, 26, HS_GT_ARCS_TOGGLE, "QSO Arcs", hs.gt_show_arcs, 0x00E5FF, 0, "", true);
        draw_hs_btn(r, hs, mouse_pt, sb_x + 12 + f_w, sy, f_w, 26, HS_GT_GRAYLINE_TOGGLE, "Grayline", hs.gt_show_grayline, 0xA855F7, 0, "", true);
        sy += 30.0f;

        draw_hs_btn(r, hs, mouse_pt, sb_x + 8, sy, f_w, 26, HS_GT_PINS_TOGGLE, "Push-Pins", hs.gt_show_pins, 0x10B981, 0, "", true);
        draw_hs_btn(r, hs, mouse_pt, sb_x + 12 + f_w, sy, f_w, 26, HS_GT_LEGEND_TOGGLE, "Map Legend", hs.gt_show_legend, 0xFBBF24, 0, "", true);
        sy += 30.0f;

        draw_hs_btn(r, hs, mouse_pt, sb_x + 8, sy, f_w, 26, HS_GT_CLEAR_LIVE, "Clear Live", false, 0xEF4444, 0, "", true);
        draw_hs_btn(r, hs, mouse_pt, sb_x + 12 + f_w, sy, f_w, 26, HS_GT_LOG_QSO, "Log QSO", false, 0x10B981, 0, "", true);
    }

// ============================================================================
// TAB 5: Pages — Full Web Map & Ham Radio Browser (QRZ, PSKReporter, etc.)
// ============================================================================
template <typename RendererT, typename AppT>
inline void render_pages_workspace(RendererT& r, AppT& app, HamShackSuite& hs, POINT mouse_pt,
                                   void* ns_view_ptr, float x, float y, float w, float h) {
    render_live_g90_telemetry_grid(r, hs, x, y, w, 38.0f);

    // 2. Pages Sub-Tab Bar (y + 42.0f .. y + 74.0f, 32px height) — QRZ is first!
    static constexpr const char* kPageTitles[5] = {
        "QRZ Callsign Database",
        "PSKReporter Live Map",
        "DXView HF Propagation",
        "DXLook HF Matrix",
        "OpenHamClock"
    };
    float stw = (w - 16.0f) / 5.0f;
    for (int i = 0; i < 5; ++i) {
        bool act = (hs.active_page_subtab == i);
        uint32_t col = (i == 0) ? 0xF59E0B : 0x00E5FF;
        draw_hs_btn(r, hs, mouse_pt, x + i * (stw + 4.0f), y + 42.0f, stw, 28.0f,
                    HS_PAGE_SUBTAB_BASE + i, kPageTitles[i], act, col, i, "", true);
    }

    float t_y = y + 74.0f;
    D2D1_RECT_F tb_r = D2D1::RectF(x, t_y, x + w, t_y + 32.0f);
    r.fill_round_rect(tb_r, 6.0f, r.hex_color(0x0F172A));
    r.draw_round_rect(tb_r, 6.0f, r.hex_color(0x1E293B), 1.0f);

    draw_hs_btn(r, hs, mouse_pt, x + 6.0f,   t_y + 3.0f, 68, 26, HS_WEB_BACK,   "◀ Back", false, 0x38BDF8, 0, "", true);
    draw_hs_btn(r, hs, mouse_pt, x + 78.0f,  t_y + 3.0f, 68, 26, HS_WEB_FWD,    "▶ Fwd",  false, 0x38BDF8, 0, "", true);
    draw_hs_btn(r, hs, mouse_pt, x + 150.0f, t_y + 3.0f, 76, 26, HS_WEB_RELOAD, "⟳ Reload",false, 0x00E5FF, 0, "", true);
    draw_hs_btn(r, hs, mouse_pt, x + 230.0f, t_y + 3.0f, 68, 26, HS_WEB_HOME,   "⌂ Home",  false, 0x10B981, 0, "", true);

    // Live URL display and edit box (Requirement 7)
    std::string live_url = hamshack_web::get_web_maps().current_url(hs.active_page_subtab);
    float cur_x = x + 304.0f;
    float right_ctrl_w = 460.0f;
    float url_w = std::max(120.0f, (x + w - right_ctrl_w) - cur_x);
    D2D1_RECT_F ubox = D2D1::RectF(cur_x, t_y + 3.0f, cur_x + url_w, t_y + 29.0f);
    bool u_foc = (hs.text_focus == HsTextFocus::PageUrl);
    r.fill_round_rect(ubox, 4.0f, r.hex_color(0x05080F));
    r.draw_round_rect(ubox, 4.0f, u_foc ? r.hex_color(0x00E5FF) : r.hex_color(0x1E293B), 1.0f);
    std::string disp_url = u_foc ? (hs.page_url_input + "_") : (live_url.empty() ? hamshack_web::kWebMapTitles[hs.active_page_subtab] : live_url);
    r.draw_text(disp_url, r.font_mono_small,
                D2D1::RectF(ubox.left + 8, ubox.top, ubox.right - 8, ubox.bottom),
                u_foc ? r.hex_color(0x00E5FF) : r.hex_color(0xCBD5E1));
    hs.add_hit(ubox, HS_PAGE_URL_BOX);

    float rx = cur_x + url_w + 6.0f;
    // [★ Bookmark] Button
    draw_hs_btn(r, hs, mouse_pt, rx, t_y + 3.0f, 100, 26, HS_PAGE_BOOKMARK, "★ Bookmark", false, 0xFACC15, 0, "", true);
    rx += 105.0f;

    // Zoom Controls: [-] [100%] [+] (Requirement 2)
    double z_val = hamshack_web::get_web_maps().get_zoom(hs.active_page_subtab);
    char z_buf[16];
    std::snprintf(z_buf, sizeof(z_buf), "%.0f%%", z_val * 100.0);
    draw_hs_btn(r, hs, mouse_pt, rx, t_y + 3.0f, 26, 26, HS_PAGE_ZOOM_OUT, "－", false, 0x38BDF8, 0, "", true);
    rx += 29.0f;
    draw_hs_btn(r, hs, mouse_pt, rx, t_y + 3.0f, 50, 26, HS_PAGE_ZOOM_RESET, z_buf, false, 0x00E5FF, 0, "", true);
    rx += 53.0f;
    draw_hs_btn(r, hs, mouse_pt, rx, t_y + 3.0f, 26, 26, HS_PAGE_ZOOM_IN, "＋", false, 0x38BDF8, 0, "", true);
    rx += 31.0f;

    // Persistent Cookies Indicator & External Safari Button
    D2D1_RECT_F badge = D2D1::RectF(rx, t_y + 3.0f, rx + 145.0f, t_y + 29.0f);
    r.fill_round_rect(badge, 4.0f, r.hex_color(0x10B981, 0.16f));
    r.draw_round_rect(badge, 4.0f, r.hex_color(0x10B981, 0.5f), 1.0f);
    r.draw_text("● STATE SAVED", r.font_mono_small, badge, r.hex_color(0x34D399), DWRITE_TEXT_ALIGNMENT_CENTER);
    rx += 150.0f;

#ifdef _WIN32
    draw_hs_btn(r, hs, mouse_pt, rx, t_y + 3.0f, 86, 26, HS_WEB_EXTERNAL, "↗ Browser", false, 0xFBBF24, 0, "", true);
#else
    draw_hs_btn(r, hs, mouse_pt, rx, t_y + 3.0f, 82, 26, HS_WEB_EXTERNAL, "↗ Safari", false, 0xFBBF24, 0, "", true);
#endif

    // Synchronize live WebView2 / WKWebView position and visibility
    float web_top = t_y + 36.0f;
    float web_h = std::max(100.0f, (y + h) - web_top);
    hamshack_web::get_web_maps().sync_visibility(ns_view_ptr, hs.active_page_subtab, x, web_top, w, web_h);
    if (!hamshack_web::get_web_maps().has_native_webview()) {
        D2D1_RECT_F fb_r = D2D1::RectF(x, web_top, x + w, web_top + web_h);
        r.fill_round_rect(fb_r, 8.0f, r.hex_color(0x0B111E));
        r.draw_round_rect(fb_r, 8.0f, r.hex_color(0x1E293B), 1.2f);
        float cy = web_top + web_h * 0.35f;
        r.draw_text(kPageTitles[hs.active_page_subtab], r.font_title,
                    D2D1::RectF(x + 20, cy, x + w - 20, cy + 32), r.hex_color(0x00E5FF), DWRITE_TEXT_ALIGNMENT_CENTER);
        r.draw_text(live_url, r.font_mono,
                    D2D1::RectF(x + 20, cy + 36, x + w - 20, cy + 62), r.hex_color(0x94A3B8), DWRITE_TEXT_ALIGNMENT_CENTER);
        draw_hs_btn(r, hs, mouse_pt, x + w * 0.5f - 130.0f, cy + 76.0f, 260.0f, 36.0f,
                    HS_WEB_EXTERNAL, "↗ Open Live Page in Browser", true, 0x10B981, 0, "", true);
    }
}

// ============================================================================
// TAB 6: Native Multi-Channel IRC Client (Libera.Chat / SASL / NickServ)
// ============================================================================
template <typename RendererT, typename AppT>
inline void render_irc_workspace(RendererT& r, AppT& app, HamShackSuite& hs, POINT mouse_pt,
                                 float x, float y, float w, float h) {
    // Top connection status & action bar
    D2D1_RECT_F top_bar = D2D1::RectF(x, y, x + w, y + 36.0f);
    r.fill_round_rect(top_bar, 6.0f, r.hex_color(0x0F172A));
    r.draw_round_rect(top_bar, 6.0f, r.hex_color(0x1E293B), 1.0f);

    float cx = x + 10.0f;
    r.draw_text("LIBERA.CHAT IRC", r.font_ui_bold, D2D1::RectF(cx, y + 8, cx + 130, y + 28), r.hex_color(0x38BDF8));
    cx += 140.0f;

    std::string s_txt;
    uint32_t s_col = 0x94A3B8;
    if (hs.irc.connected) {
        s_txt = "● ONLINE: " + hs.irc.server + ":" + std::to_string(hs.irc.port) + " (" + hs.irc.current_nick + ")";
        s_col = 0x10B981;
    } else if (hs.irc.connecting) {
        s_txt = "○ CONNECTING: " + hs.irc.server + "...";
        s_col = 0xF59E0B;
    } else {
        s_txt = "○ DISCONNECTED (" + hs.irc.server + ")";
        s_col = 0xEF4444;
    }
    r.draw_text(s_txt, r.font_mono_small, D2D1::RectF(cx, y + 9, cx + 340, y + 28), r.hex_color(s_col));
    cx += 350.0f;

    if (!hs.irc.connected && !hs.irc.connecting) {
        draw_hs_btn(r, hs, mouse_pt, cx, y + 5.0f, 105, 26, HS_IRC_CONNECT_BTN, "Connect", false, 0x10B981, 0, "", true);
    } else {
        draw_hs_btn(r, hs, mouse_pt, cx, y + 5.0f, 105, 26, HS_IRC_DISCONN_BTN, "Disconnect", false, 0xEF4444, 0, "", true);
    }
    cx += 115.0f;

    draw_hs_btn(r, hs, mouse_pt, cx, y + 5.0f, 135, 26, HS_IRC_IDENT_BTN, "Identify NickServ", false, 0xF59E0B, 0, "", true);
    cx += 145.0f;

    // SASL Toggle Button (Interactive & SASL by default with graceful fallback)
    auto& cfg = hamshack::get_app_config();
    std::string sasl_info = cfg.irc_use_sasl ? "SASL PLAIN: ON" : "SASL: OFF";
    draw_hs_btn(r, hs, mouse_pt, cx, y + 5.0f, 125, 26, HS_IRC_SASL_TOGGLE, sasl_info, cfg.irc_use_sasl, cfg.irc_use_sasl ? 0x34D399 : 0x64748B, 0, "", true);
    cx += 132.0f;

    // Chat Text Zoom Controls: [-] 13pt [+] (Also accessible via Cmd+ / Cmd-)
    draw_hs_btn(r, hs, mouse_pt, cx, y + 5.0f, 26, 26, HS_IRC_ZOOM_OUT, "-", false, 0x64748B, 0, "", true);
    cx += 29.0f;
    char font_sz_buf[16];
    std::snprintf(font_sz_buf, sizeof(font_sz_buf), "%.0fpt", hs.irc_font_size);
    draw_hs_btn(r, hs, mouse_pt, cx, y + 5.0f, 48, 26, HS_IRC_ZOOM_RESET, font_sz_buf, false, 0x38BDF8, 0, "", true);
    cx += 51.0f;
    draw_hs_btn(r, hs, mouse_pt, cx, y + 5.0f, 26, 26, HS_IRC_ZOOM_IN, "+", false, 0x64748B, 0, "", true);
    cx += 32.0f;

    // Channel Tabs Bar
    float tab_y = y + 42.0f;
    float tab_h = 28.0f;
    D2D1_RECT_F ch_bar = D2D1::RectF(x, tab_y, x + w, tab_y + tab_h);
    r.fill_round_rect(ch_bar, 4.0f, r.hex_color(0x0A0F1D));
    r.draw_round_rect(ch_bar, 4.0f, r.hex_color(0x1E293B), 1.0f);

    float t_x = x + 8.0f;
    {
        std::lock_guard<std::recursive_mutex> lk(hs.irc.mtx);
        for (size_t i = 0; i < hs.irc.channels.size(); ++i) {
            bool active = (static_cast<int>(i) == hs.irc.active_chan_idx);
            const auto& ch = hs.irc.channels[i];
            std::string t_lbl = ch.name;
            if (ch.unread_count > 0) t_lbl += " (" + std::to_string(ch.unread_count) + ")";
            float tw = std::max(70.0f, 20.0f + static_cast<float>(t_lbl.size() * 8.5f));
            draw_hs_btn(r, hs, mouse_pt, t_x, tab_y + 2.0f, tw, 24, HS_IRC_CHAN_BASE + static_cast<int>(i),
                        t_lbl, active, active ? 0x38BDF8 : 0x64748B, static_cast<int>(i), "", true);
            t_x += tw + 4.0f;
        }
    }

    // Quick Join Button (#hamradio default channel)
    if (t_x + 155.0f < x + w - 8.0f) {
        draw_hs_btn(r, hs, mouse_pt, x + w - 160.0f, tab_y + 2.0f, 152, 24, HS_IRC_CHAN_BASE + 99,
                    "+ #hamradio", false, 0x10B981, 99, "", true);
    }

    // Main Body: Chat Transcript (Left) + Users/Topic Roster (Right)
    float body_y = tab_y + tab_h + 6.0f;
    float input_h = 36.0f;
    float body_h = std::max(100.0f, (y + h) - body_y - input_h - 6.0f);
    float roster_w = 210.0f;
    float chat_w = w - roster_w - 8.0f;

    // Chat Transcript
    D2D1_RECT_F chat_box = D2D1::RectF(x, body_y, x + chat_w, body_y + body_h);
    r.fill_round_rect(chat_box, 6.0f, r.hex_color(0x05080F));
    r.draw_round_rect(chat_box, 6.0f, r.hex_color(0x1E293B), 1.0f);

    // Channel Topic Header in Chat
    std::string cur_topic = "No active channel";
    {
        std::lock_guard<std::recursive_mutex> lk(hs.irc.mtx);
        if (hs.irc.active_chan_idx >= 0 && hs.irc.active_chan_idx < static_cast<int>(hs.irc.channels.size())) {
            cur_topic = hs.irc.channels[hs.irc.active_chan_idx].topic;
        }
    }
    D2D1_RECT_F topic_bar = D2D1::RectF(chat_box.left + 4, chat_box.top + 4, chat_box.right - 4, chat_box.top + 24);
    r.fill_round_rect(topic_bar, 4.0f, r.hex_color(0x0F172A));
    r.draw_text("TOPIC: " + cur_topic, r.font_mono_small,
                D2D1::RectF(topic_bar.left + 8, topic_bar.top + 3, topic_bar.right - 8, topic_bar.bottom),
                r.hex_color(0x94A3B8));

    // Messages list with Cmd+/Cmd- dynamically scaled mono font
    IDWriteTextFormat* irc_log_font = r.get_scaled_mono_font(hs.irc_font_size);
    {
        std::lock_guard<std::recursive_mutex> lk(hs.irc.mtx);
        if (hs.irc.active_chan_idx >= 0 && hs.irc.active_chan_idx < static_cast<int>(hs.irc.channels.size())) {
            const auto& ch = hs.irc.channels[hs.irc.active_chan_idx];
            float my = chat_box.top + 28.0f;
            float line_h = hs.irc_font_size + 6.0f;
            int max_lines = std::max(1, static_cast<int>((chat_box.bottom - my - 6.0f) / line_h));
            int start_idx = std::max(0, static_cast<int>(ch.messages.size()) - max_lines);

            for (int i = start_idx; i < static_cast<int>(ch.messages.size()); ++i) {
                const auto& msg = ch.messages[i];
                std::string line_str = "[" + msg.timestamp + "] ";
                if (msg.sender == "***") {
                    line_str += "*** " + msg.text;
                } else if (msg.is_action) {
                    line_str += "* " + msg.sender + " " + msg.text;
                } else {
                    line_str += "<" + msg.sender + "> " + msg.text;
                }
                D2D1_RECT_F lr = D2D1::RectF(chat_box.left + 8, my, chat_box.right - 8, my + line_h);
                r.draw_text(line_str, irc_log_font, lr, r.hex_color(msg.color));
                my += line_h;
            }
        }
    }

    // User Roster Panel (Right)
    float rx = x + chat_w + 8.0f;
    D2D1_RECT_F roster_box = D2D1::RectF(rx, body_y, rx + roster_w, body_y + body_h);
    r.fill_round_rect(roster_box, 6.0f, r.hex_color(0x0A0F1D));
    r.draw_round_rect(roster_box, 6.0f, r.hex_color(0x1E293B), 1.0f);

    r.draw_text("CHANNEL USERS / ROSTER", r.font_ui_bold,
                D2D1::RectF(rx + 8, body_y + 6, rx + roster_w - 8, body_y + 24), r.hex_color(0x00E5FF));

    {
        std::lock_guard<std::recursive_mutex> lk(hs.irc.mtx);
        if (hs.irc.active_chan_idx >= 0 && hs.irc.active_chan_idx < static_cast<int>(hs.irc.channels.size())) {
            const auto& ch = hs.irc.channels[hs.irc.active_chan_idx];
            float uy = body_y + 28.0f;
            float uh = 19.0f;
            int u_idx = 0;

            // Sort: Ops first, Voices second, Regular users third (alphabetical within tier)
            std::vector<std::pair<std::string, char>> sorted_users(ch.users.begin(), ch.users.end());
            std::sort(sorted_users.begin(), sorted_users.end(), [](const auto& a, const auto& b) {
                int rank_a = (a.second == '@' || a.second == '~' || a.second == '&') ? 0 : (a.second == '+') ? 1 : 2;
                int rank_b = (b.second == '@' || b.second == '~' || b.second == '&') ? 0 : (b.second == '+') ? 1 : 2;
                if (rank_a != rank_b) return rank_a < rank_b;
                return hamshack::hs_strcasecmp(a.first.c_str(), b.first.c_str()) < 0;
            });

            for (const auto& [u, pfx] : sorted_users) {
                if (uy + uh > roster_box.bottom - 4.0f) break;
                D2D1_RECT_F u_rc = D2D1::RectF(rx + 4, uy, rx + roster_w - 4, uy + uh);
                bool hovered = RendererT::pt_in_rect(mouse_pt, u_rc);
                if (hovered) {
                    r.fill_round_rect(u_rc, 3.0f, r.hex_color(0x1E293B));
                }

                std::string prefix_str = (pfx == '@' || pfx == '~' || pfx == '&') ? "@ " :
                                         (pfx == '+') ? "+ " : "• ";
                uint32_t name_col = (u == hs.irc.current_nick) ? 0x38BDF8 :
                                    (pfx == '@' || pfx == '~' || pfx == '&') ? 0xF59E0B :
                                    (pfx == '+') ? 0x10B981 : 0xE2E8F0;

                r.draw_text(prefix_str + u, r.font_mono_small,
                            D2D1::RectF(rx + 8, uy + 2, rx + roster_w - 8, uy + uh),
                            r.hex_color(name_col));
                hs.add_hit(u_rc, HS_IRC_USER_ROW_BASE + std::min(59, u_idx), static_cast<int>(pfx), u);
                uy += uh;
                u_idx++;
            }
            if (ch.users.empty()) {
                r.draw_text("(No users synced yet)", r.font_mono_small,
                            D2D1::RectF(rx + 8, uy, rx + roster_w - 8, uy + uh), r.hex_color(0x64748B));
            }
        }
    }

    // Bottom Input Deck: Nickname Box (editable & stretched dynamically if text is longer) + Message Box + Send Button
    float in_y = y + h - input_h;
    float nick_char_w = hs.irc_font_size * 0.65f;
    float nick_w = std::max(110.0f, 26.0f + static_cast<float>(hs.irc_nick_edit.size()) * nick_char_w);
    D2D1_RECT_F nick_box = D2D1::RectF(x, in_y, x + nick_w, in_y + input_h);
    bool nick_foc = (hs.text_focus == HsTextFocus::IrcNick);
    r.fill_round_rect(nick_box, 5.0f, r.hex_color(0x0F172A));
    r.draw_round_rect(nick_box, 5.0f, nick_foc ? r.hex_color(0x38BDF8) : r.hex_color(0x334155), nick_foc ? 1.5f : 1.0f);
    r.draw_text(hs.irc_nick_edit + (nick_foc ? "_" : ""), irc_log_font,
                D2D1::RectF(nick_box.left + 6, nick_box.top + 8, nick_box.right - 6, nick_box.bottom - 4),
                r.hex_color(0x38BDF8), DWRITE_TEXT_ALIGNMENT_CENTER);
    hs.add_hit(nick_box, HS_IRC_NICK_BOX);

    float msg_x = x + nick_w + 6.0f;
    float btn_w = 75.0f;
    float msg_w = w - nick_w - 6.0f - btn_w - 6.0f;
    D2D1_RECT_F msg_box = D2D1::RectF(msg_x, in_y, msg_x + msg_w, in_y + input_h);
    bool msg_foc = (hs.text_focus == HsTextFocus::IrcInput);
    r.fill_round_rect(msg_box, 5.0f, r.hex_color(0x05080F));
    r.draw_round_rect(msg_box, 5.0f, msg_foc ? r.hex_color(0x00E5FF) : r.hex_color(0x334155), msg_foc ? 1.5f : 1.0f);
    std::string disp_msg = hs.irc_input_text + (msg_foc ? "_" : "");
    if (disp_msg.empty() && !msg_foc) disp_msg = "Type message or /command (/whois, /op, /voice, /kick, /topic, /me, /help)...";
    r.draw_text(disp_msg, irc_log_font,
                D2D1::RectF(msg_box.left + 10, msg_box.top + 8, msg_box.right - 10, msg_box.bottom - 4),
                disp_msg.empty() || (!msg_foc && hs.irc_input_text.empty()) ? r.hex_color(0x64748B) : r.hex_color(0xF8FAFC));
    hs.add_hit(msg_box, HS_IRC_INPUT_BOX);

    draw_hs_btn(r, hs, mouse_pt, msg_x + msg_w + 6.0f, in_y, btn_w, input_h, HS_IRC_SEND_BTN, "Send", false, 0x00E5FF, 0, "", true);

    // Floating Context Menu for IRC User Actions (Op, Voice, Whois, Query, Kick, Ban)
    if (hs.irc_menu.is_open) {
        float mw = 175.0f;
        float mh = 176.0f;
        float mx = std::clamp(hs.irc_menu.x, x + 10.0f, x + w - mw - 10.0f);
        float my = std::clamp(hs.irc_menu.y, y + 10.0f, y + h - mh - 10.0f);

        D2D1_RECT_F menu_rc = D2D1::RectF(mx, my, mx + mw, my + mh);
        r.fill_round_rect(menu_rc, 6.0f, r.hex_color(0x05080F));
        r.draw_round_rect(menu_rc, 6.0f, r.hex_color(0x38BDF8, 0.9f), 1.5f);

        // Header with target nickname
        D2D1_RECT_F hdr_rc = D2D1::RectF(mx + 6, my + 4, mx + mw - 6, my + 24);
        r.fill_round_rect(hdr_rc, 3.0f, r.hex_color(0x0F172A));
        r.draw_text("👤 " + hs.irc_menu.nick, r.font_ui_bold,
                    D2D1::RectF(hdr_rc.left + 4, hdr_rc.top + 3, hdr_rc.right - 4, hdr_rc.bottom),
                    r.hex_color(0x38BDF8), DWRITE_TEXT_ALIGNMENT_CENTER);

        float item_y = my + 28.0f;
        float item_h = 22.0f;

        // 1. Op toggle
        bool is_op = (hs.irc_menu.prefix == '@' || hs.irc_menu.prefix == '~' || hs.irc_menu.prefix == '&');
        std::string op_lbl = is_op ? "⭐ Take Op (-o)" : "⭐ Give Op (+o)";
        draw_hs_btn(r, hs, mouse_pt, mx + 5, item_y, mw - 10, item_h, HS_IRC_MENU_OP, op_lbl, false, 0xF59E0B, 0, "", true);
        item_y += item_h + 2.0f;

        // 2. Voice toggle
        bool is_voice = (hs.irc_menu.prefix == '+');
        std::string voice_lbl = is_voice ? "🎤 Take Voice (-v)" : "🎤 Give Voice (+v)";
        draw_hs_btn(r, hs, mouse_pt, mx + 5, item_y, mw - 10, item_h, HS_IRC_MENU_VOICE, voice_lbl, false, 0x10B981, 0, "", true);
        item_y += item_h + 2.0f;

        // 3. Whois
        draw_hs_btn(r, hs, mouse_pt, mx + 5, item_y, mw - 10, item_h, HS_IRC_MENU_WHOIS, "ℹ️ Whois (/whois)", false, 0x38BDF8, 0, "", true);
        item_y += item_h + 2.0f;

        // 4. Query / Direct Message
        draw_hs_btn(r, hs, mouse_pt, mx + 5, item_y, mw - 10, item_h, HS_IRC_MENU_QUERY, "💬 Direct Message", false, 0xA855F7, 0, "", true);
        item_y += item_h + 2.0f;

        // 5. Kick
        draw_hs_btn(r, hs, mouse_pt, mx + 5, item_y, mw - 10, item_h, HS_IRC_MENU_KICK, "👢 Kick from Channel", false, 0xEF4444, 0, "", true);
        item_y += item_h + 2.0f;

        // 6. Ban
        draw_hs_btn(r, hs, mouse_pt, mx + 5, item_y, mw - 10, item_h, HS_IRC_MENU_BAN, "🚫 Ban User (+b)", false, 0xEF4444, 0, "", true);
    }
}

// ============================================================================
// TAB 7: Winlink / Email Client (B2F v3.2 Protocol, ARDOP TNC & RMS Gateways)
// ============================================================================
template <typename RendererT, typename AppT>
inline void render_winlink_workspace(RendererT& r, AppT& app, HamShackSuite& hs, POINT mouse_pt,
                                     float x, float y, float w, float h) {
    auto& cfg = hamshack::get_app_config();

    // 1. Top Sub-Tab Bar (Mailbox, Gateways, Configuration)
    float bar_h = 32.0f;
    D2D1_RECT_F bar_r = D2D1::RectF(x, y, x + w, y + bar_h);
    r.fill_round_rect(bar_r, 6.0f, r.hex_color(0x0A0F1D));
    r.draw_round_rect(bar_r, 6.0f, r.hex_color(0x1E293B), 1.0f);

    float bx = x + 6.0f;
    struct WlSubTab {
        int id;
        int idx;
        const char* label;
        uint32_t col;
    };
    static const WlSubTab kWlTabs[3] = {
        {HS_WL_SUBTAB_MAILBOX,  0, "📬 Mailbox",           0x34D399},
        {HS_WL_SUBTAB_GATEWAYS, 1, "🌐 Gateways (4,807)",  0x38BDF8},
        {HS_WL_SUBTAB_CONFIG,   2, "⚙ Configuration",      0xF59E0B}
    };
    for (int i = 0; i < 3; ++i) {
        float tw = 150.0f;
        bool sel = (hs.active_winlink_subtab == kWlTabs[i].idx);
        draw_hs_btn(r, hs, mouse_pt, bx, y + 3.0f, tw, 26.0f, kWlTabs[i].id,
                    kWlTabs[i].label, sel, kWlTabs[i].col, kWlTabs[i].idx, "", true);
        bx += tw + 6.0f;
    }

    // Top status pill on right of bar
    std::string trans_lbl = (cfg.winlink_transport == 0) ? "Telnet CMS" : (cfg.winlink_transport == 1) ? "ARDOP HF" : "Packet";
    std::string info_str = "OP: " + cfg.winlink_callsign + " • Grid: " + cfg.winlink_grid + " • Mode: " + trans_lbl;
    D2D1_RECT_F info_r = D2D1::RectF(x + w - 340.0f, y + 4.0f, x + w - 8.0f, y + 28.0f);
    r.fill_round_rect(info_r, 4.0f, r.hex_color(0x05080F));
    r.draw_round_rect(info_r, 4.0f, r.hex_color(0x334155), 1.0f);
    r.draw_text(info_str, r.font_mono_small, info_r, r.hex_color(0x38BDF8), DWRITE_TEXT_ALIGNMENT_CENTER);

    float body_y = y + bar_h + 6.0f;
    float body_h = h - bar_h - 6.0f;

    // ------------------------------------------------------------------------
    // SUBTAB 0: MAILBOX (Email Client + Right Sidebar with Radio/Modem & BIG SEND/RECEIVE)
    // ------------------------------------------------------------------------
    if (hs.active_winlink_subtab == 0) {
        float sb_w = 320.0f;
        float main_w = w - sb_w - 8.0f;

        // Left Main Mailbox Area:
        // Top action bar (Folder tabs + Compose + Delete)
        float m_bar_h = 30.0f;
        D2D1_RECT_F m_bar = D2D1::RectF(x, body_y, x + main_w, body_y + m_bar_h);
        r.fill_round_rect(m_bar, 5.0f, r.hex_color(0x0F172A));
        r.draw_round_rect(m_bar, 5.0f, r.hex_color(0x1E293B), 1.0f);

        float fx = x + 6.0f;
        struct FolderSpec { int id; int idx; const char* name; size_t count; uint32_t col; };
        FolderSpec folders[4] = {
            {HS_WL_FOLDER_INBOX,   0, "📥 Inbox",   hs.winlink.inbox.size(),   0x38BDF8},
            {HS_WL_FOLDER_OUTBOX,  1, "📤 Outbox",  hs.winlink.outbox.size(),  0xF59E0B},
            {HS_WL_FOLDER_SENT,    2, "📨 Sent",    hs.winlink.sent.size(),    0x10B981},
            {HS_WL_FOLDER_ARCHIVE, 3, "📦 Archive", hs.winlink.archive.size(), 0x94A3B8}
        };
        for (int i = 0; i < 4; ++i) {
            std::string f_lbl = std::string(folders[i].name) + " (" + std::to_string(folders[i].count) + ")";
            float fw = 105.0f;
            bool sel_f = (!hs.winlink.is_composing && hs.winlink.active_folder == folders[i].idx);
            draw_hs_btn(r, hs, mouse_pt, fx, body_y + 2.0f, fw, 26.0f, folders[i].id,
                        f_lbl, sel_f, folders[i].col, folders[i].idx, "", true);
            fx += fw + 4.0f;
        }

        fx += 8.0f;
        draw_hs_btn(r, hs, mouse_pt, fx, body_y + 2.0f, 130.0f, 26.0f, HS_WL_COMPOSE_BTN,
                    "✏ Compose Email", hs.winlink.is_composing, 0x00E5FF, 0, "", true);
        fx += 136.0f;

        if (!hs.winlink.is_composing) {
            draw_hs_btn(r, hs, mouse_pt, x + main_w - 85.0f, body_y + 2.0f, 80.0f, 26.0f, HS_WL_DELETE_MSG,
                        "🗑 Delete", false, 0xEF4444, 0, "", true);
        }

        float c_top = body_y + m_bar_h + 6.0f;
        float c_h = body_h - m_bar_h - 6.0f;

        if (!hs.winlink.is_composing) {
            // Split view: Message List (top 42%) and Message Preview (bottom 58%)
            float list_h = std::round(c_h * 0.42f);
            float prev_y = c_top + list_h + 6.0f;
            float prev_h = c_h - list_h - 6.0f;

            // Message List Table
            D2D1_RECT_F list_r = D2D1::RectF(x, c_top, x + main_w, c_top + list_h);
            r.fill_round_rect(list_r, 6.0f, r.hex_color(0x05080F));
            r.draw_round_rect(list_r, 6.0f, r.hex_color(0x1E293B), 1.0f);

            // Table Header Row
            D2D1_RECT_F hdr_r = D2D1::RectF(x + 2, c_top + 2, x + main_w - 2, c_top + 22.0f);
            r.fill_round_rect(hdr_r, 4.0f, r.hex_color(0x0F172A));
            r.draw_text("SENDER / RECIPIENT", r.font_ui_bold, D2D1::RectF(x + 10, c_top + 4, x + 180, c_top + 20), r.hex_color(0x94A3B8));
            r.draw_text("DATE (UTC)", r.font_ui_bold, D2D1::RectF(x + 185, c_top + 4, x + 340, c_top + 20), r.hex_color(0x94A3B8));
            r.draw_text("SUBJECT", r.font_ui_bold, D2D1::RectF(x + 345, c_top + 4, x + main_w - 140, c_top + 20), r.hex_color(0x94A3B8));
            r.draw_text("SIZE", r.font_ui_bold, D2D1::RectF(x + main_w - 135, c_top + 4, x + main_w - 75, c_top + 20), r.hex_color(0x94A3B8));
            r.draw_text("FILE", r.font_ui_bold, D2D1::RectF(x + main_w - 70, c_top + 4, x + main_w - 10, c_top + 20), r.hex_color(0x94A3B8));

            // List items from active folder
            const std::vector<winlink::WinlinkMessage>* f_list = &hs.winlink.inbox;
            if (hs.winlink.active_folder == 1) f_list = &hs.winlink.outbox;
            else if (hs.winlink.active_folder == 2) f_list = &hs.winlink.sent;
            else if (hs.winlink.active_folder == 3) f_list = &hs.winlink.archive;

            float row_y = c_top + 24.0f;
            float row_h = 24.0f;
            int max_rows = static_cast<int>((list_h - 26.0f) / row_h);
            for (size_t i = 0; i < f_list->size() && static_cast<int>(i) < max_rows; ++i) {
                const auto& msg = (*f_list)[i];
                bool is_sel = (static_cast<int>(i) == hs.winlink.selected_msg_idx);
                D2D1_RECT_F row_r = D2D1::RectF(x + 2, row_y, x + main_w - 2, row_y + row_h);
                if (is_sel) {
                    r.fill_round_rect(row_r, 4.0f, r.hex_color(0x1E293B));
                    r.draw_round_rect(row_r, 4.0f, r.hex_color(0x38BDF8), 1.0f);
                } else if (RendererT::pt_in_rect(mouse_pt, row_r)) {
                    r.fill_round_rect(row_r, 4.0f, r.hex_color(0x0E1726));
                }
                std::string party = (hs.winlink.active_folder == 1 || hs.winlink.active_folder == 2) ? msg.to : msg.from;
                r.draw_text(party, r.font_mono_small, D2D1::RectF(x + 10, row_y + 4, x + 180, row_y + 20), r.hex_color(is_sel ? 0x38BDF8 : 0xE2E8F0));
                r.draw_text(msg.date_str.substr(0, std::min<size_t>(msg.date_str.size(), 20)), r.font_mono_small, D2D1::RectF(x + 185, row_y + 4, x + 340, row_y + 20), r.hex_color(0x94A3B8));
                r.draw_text(msg.subject, r.font_ui_bold, D2D1::RectF(x + 345, row_y + 4, x + main_w - 140, row_y + 20), r.hex_color(is_sel ? 0x00E5FF : 0xF1F5F9));
                char sz_buf[32];
                std::snprintf(sz_buf, sizeof(sz_buf), "%zu B", msg.total_bytes);
                r.draw_text(sz_buf, r.font_mono_small, D2D1::RectF(x + main_w - 135, row_y + 4, x + main_w - 75, row_y + 20), r.hex_color(0x94A3B8));
                if (!msg.attachments.empty()) {
                    r.draw_text("📎 " + std::to_string(msg.attachments.size()), r.font_mono_small, D2D1::RectF(x + main_w - 70, row_y + 4, x + main_w - 10, row_y + 20), r.hex_color(0xF59E0B));
                }
                hs.add_hit(row_r, HS_WL_MSG_ROW_BASE + static_cast<int>(i), static_cast<int>(i));
                row_y += row_h;
            }
            if (f_list->empty()) {
                r.draw_text("(Folder is empty)", r.font_mono_small, D2D1::RectF(x + 10, c_top + 35, x + main_w - 10, c_top + 60), r.hex_color(0x64748B), DWRITE_TEXT_ALIGNMENT_CENTER);
            }

            // Message Preview Pane
            D2D1_RECT_F prev_r = D2D1::RectF(x, prev_y, x + main_w, prev_y + prev_h);
            r.fill_round_rect(prev_r, 6.0f, r.hex_color(0x05080F));
            r.draw_round_rect(prev_r, 6.0f, r.hex_color(0x1E293B), 1.0f);

            if (!f_list->empty() && hs.winlink.selected_msg_idx >= 0 && hs.winlink.selected_msg_idx < static_cast<int>(f_list->size())) {
                const auto& msg = (*f_list)[hs.winlink.selected_msg_idx];
                float py = prev_y + 8.0f;
                r.draw_text("FROM:    " + msg.from, r.font_mono_small, D2D1::RectF(x + 12, py, x + main_w - 12, py + 16), r.hex_color(0x38BDF8));
                py += 18.0f;
                r.draw_text("TO:      " + msg.to + (msg.cc.empty() ? "" : "  (CC: " + msg.cc + ")"), r.font_mono_small, D2D1::RectF(x + 12, py, x + main_w - 12, py + 16), r.hex_color(0xCBD5E1));
                py += 18.0f;
                r.draw_text("DATE:    " + msg.date_str + "    MID: " + msg.mid, r.font_mono_small, D2D1::RectF(x + 12, py, x + main_w - 12, py + 16), r.hex_color(0x94A3B8));
                py += 18.0f;
                r.draw_text("SUBJECT: " + msg.subject, r.font_ui_bold, D2D1::RectF(x + 12, py, x + main_w - 12, py + 18), r.hex_color(0x00E5FF));
                py += 22.0f;

                if (!msg.attachments.empty()) {
                    float att_x = x + 12.0f;
                    for (const auto& a : msg.attachments) {
                        std::string a_lbl = "📎 " + a.filename + " (" + std::to_string(a.size_bytes / 1024) + " KB)";
                        float aw = 20.0f + static_cast<float>(a_lbl.size() * 7.5f);
                        D2D1_RECT_F ar = D2D1::RectF(att_x, py, att_x + aw, py + 22.0f);
                        r.fill_round_rect(ar, 4.0f, r.hex_color(0x1E293B));
                        r.draw_round_rect(ar, 4.0f, r.hex_color(0xF59E0B), 1.0f);
                        r.draw_text(a_lbl, r.font_mono_small, ar, r.hex_color(0xF59E0B), DWRITE_TEXT_ALIGNMENT_CENTER);
                        att_x += aw + 6.0f;
                    }
                    py += 28.0f;
                }

                // Separator line
                r.fill_round_rect(D2D1::RectF(x + 10, py, x + main_w - 10, py + 1.0f), 0.0f, r.hex_color(0x334155));
                py += 8.0f;

                // Message Body
                D2D1_RECT_F body_box = D2D1::RectF(x + 12, py, x + main_w - 12, prev_y + prev_h - 8.0f);
                r.draw_text(msg.body, r.font_mono_small, body_box, r.hex_color(0xF8FAFC));
            } else {
                r.draw_text("Select a message to preview", r.font_mono_small, prev_r, r.hex_color(0x64748B), DWRITE_TEXT_ALIGNMENT_CENTER);
            }
        } else {
            // Composer Mode
            D2D1_RECT_F comp_r = D2D1::RectF(x, c_top, x + main_w, c_top + c_h);
            r.fill_round_rect(comp_r, 6.0f, r.hex_color(0x05080F));
            r.draw_round_rect(comp_r, 6.0f, r.hex_color(0x00E5FF), 1.0f);

            float cy = c_top + 10.0f;
            r.draw_text("COMPOSE WINLINK MESSAGE (B2F v3.2 PROTOCOL)", r.font_ui_bold,
                        D2D1::RectF(x + 14, cy, x + main_w - 14, cy + 20), r.hex_color(0x00E5FF));
            cy += 26.0f;

            // To: Field
            r.draw_text("To:", r.font_ui_bold, D2D1::RectF(x + 14, cy + 4, x + 70, cy + 24), r.hex_color(0x94A3B8));
            D2D1_RECT_F to_box = D2D1::RectF(x + 75, cy, x + main_w - 14, cy + 26.0f);
            bool to_foc = (hs.text_focus == HsTextFocus::WlCompTo);
            r.fill_round_rect(to_box, 4.0f, r.hex_color(0x0F172A));
            r.draw_round_rect(to_box, 4.0f, to_foc ? r.hex_color(0x00E5FF) : r.hex_color(0x334155), 1.0f);
            std::string to_disp = hs.winlink.compose_to + (to_foc ? "_" : "");
            if (to_disp.empty() && !to_foc) to_disp = "Callsign or email (e.g. W1AW or operator@winlink.org)";
            r.draw_text(to_disp, r.font_mono_small, D2D1::RectF(to_box.left + 8, to_box.top + 5, to_box.right - 8, to_box.bottom),
                        to_foc || !hs.winlink.compose_to.empty() ? r.hex_color(0xF8FAFC) : r.hex_color(0x64748B));
            hs.add_hit(to_box, HS_WL_COMP_TO_BOX);
            cy += 32.0f;

            // Cc: Field
            r.draw_text("Cc:", r.font_ui_bold, D2D1::RectF(x + 14, cy + 4, x + 70, cy + 24), r.hex_color(0x94A3B8));
            D2D1_RECT_F cc_box = D2D1::RectF(x + 75, cy, x + main_w - 14, cy + 26.0f);
            bool cc_foc = (hs.text_focus == HsTextFocus::WlCompCc);
            r.fill_round_rect(cc_box, 4.0f, r.hex_color(0x0F172A));
            r.draw_round_rect(cc_box, 4.0f, cc_foc ? r.hex_color(0x00E5FF) : r.hex_color(0x334155), 1.0f);
            std::string cc_disp = hs.winlink.compose_cc + (cc_foc ? "_" : "");
            r.draw_text(cc_disp, r.font_mono_small, D2D1::RectF(cc_box.left + 8, cc_box.top + 5, cc_box.right - 8, cc_box.bottom),
                        cc_foc || !hs.winlink.compose_cc.empty() ? r.hex_color(0xF8FAFC) : r.hex_color(0x64748B));
            hs.add_hit(cc_box, HS_WL_COMP_CC_BOX);
            cy += 32.0f;

            // Subject: Field
            r.draw_text("Subject:", r.font_ui_bold, D2D1::RectF(x + 14, cy + 4, x + 70, cy + 24), r.hex_color(0x94A3B8));
            D2D1_RECT_F subj_box = D2D1::RectF(x + 75, cy, x + main_w - 14, cy + 26.0f);
            bool subj_foc = (hs.text_focus == HsTextFocus::WlCompSubj);
            r.fill_round_rect(subj_box, 4.0f, r.hex_color(0x0F172A));
            r.draw_round_rect(subj_box, 4.0f, subj_foc ? r.hex_color(0x00E5FF) : r.hex_color(0x334155), 1.0f);
            std::string subj_disp = hs.winlink.compose_subject + (subj_foc ? "_" : "");
            r.draw_text(subj_disp, r.font_mono_small, D2D1::RectF(subj_box.left + 8, subj_box.top + 5, subj_box.right - 8, subj_box.bottom),
                        subj_foc || !hs.winlink.compose_subject.empty() ? r.hex_color(0xF8FAFC) : r.hex_color(0x64748B));
            hs.add_hit(subj_box, HS_WL_COMP_SUBJ_BOX);
            cy += 32.0f;

            // Attachments Row
            draw_hs_btn(r, hs, mouse_pt, x + 75, cy, 150.0f, 26.0f, HS_WL_ATTACH_ADD_BTN, "📎 Add Attachment...", false, 0xF59E0B, 0, "", true);
            float ax = x + 232.0f;
            if (!hs.winlink.compose_attachments.empty()) {
                draw_hs_btn(r, hs, mouse_pt, ax, cy, 75.0f, 26.0f, HS_WL_ATTACH_CLR_BTN, "Clear", false, 0xEF4444, 0, "", true);
                ax += 82.0f;
                for (const auto& att : hs.winlink.compose_attachments) {
                    std::string att_chip = "📄 " + att.filename + " (" + std::to_string(att.size_bytes / 1024) + " KB)";
                    float cw = 16.0f + static_cast<float>(att_chip.size() * 7.5f);
                    D2D1_RECT_F cr = D2D1::RectF(ax, cy, ax + cw, cy + 26.0f);
                    r.fill_round_rect(cr, 4.0f, r.hex_color(0x1E293B));
                    r.draw_round_rect(cr, 4.0f, r.hex_color(0x10B981), 1.0f);
                    r.draw_text(att_chip, r.font_mono_small, cr, r.hex_color(0x34D399), DWRITE_TEXT_ALIGNMENT_CENTER);
                    ax += cw + 4.0f;
                }
            }
            cy += 32.0f;

            // Body Area
            float body_box_h = (c_top + c_h) - cy - 44.0f;
            D2D1_RECT_F body_box = D2D1::RectF(x + 14, cy, x + main_w - 14, cy + body_box_h);
            bool body_foc = (hs.text_focus == HsTextFocus::WlCompBody);
            r.fill_round_rect(body_box, 4.0f, r.hex_color(0x0F172A));
            r.draw_round_rect(body_box, 4.0f, body_foc ? r.hex_color(0x00E5FF) : r.hex_color(0x334155), 1.0f);
            std::string b_disp = hs.winlink.compose_body + (body_foc ? "_" : "");
            if (b_disp.empty() && !body_foc) b_disp = "Compose email message body here...";
            r.draw_text(b_disp, r.font_mono_small, D2D1::RectF(body_box.left + 8, body_box.top + 8, body_box.right - 8, body_box.bottom - 8),
                        body_foc || !hs.winlink.compose_body.empty() ? r.hex_color(0xF8FAFC) : r.hex_color(0x64748B));
            hs.add_hit(body_box, HS_WL_COMP_BODY_BOX);
            cy += body_box_h + 8.0f;

            // Bottom Buttons (Queue to Outbox / Cancel)
            draw_hs_btn(r, hs, mouse_pt, x + 14, cy, 160.0f, 30.0f, HS_WL_QUEUE_OUTBOX, "📤 Queue to Outbox", false, 0x10B981, 0, "", true);
            draw_hs_btn(r, hs, mouse_pt, x + 182, cy, 100.0f, 30.0f, HS_WL_CANCEL_COMP, "Cancel", false, 0x64748B, 0, "", true);
        }

        // ====================================================================
        // RIGHT SIDEBAR: Radio Status + Modem Info + Gateway + BIG BUTTON!
        // ====================================================================
        float rx = x + main_w + 8.0f;
        D2D1_RECT_F sb_r = D2D1::RectF(rx, body_y, rx + sb_w, body_y + body_h);
        r.fill_round_rect(sb_r, 6.0f, r.hex_color(0x0F172A));
        r.draw_round_rect(sb_r, 6.0f, r.hex_color(0x1E293B), 1.0f);

        float sy = body_y + 8.0f;

        // 1. Radio Status Card (FLRig CAT Xiegu G90)
        D2D1_RECT_F card1 = D2D1::RectF(rx + 6, sy, rx + sb_w - 6, sy + 74.0f);
        r.fill_round_rect(card1, 5.0f, r.hex_color(0x05080F));
        r.draw_round_rect(card1, 5.0f, r.hex_color(0x334155), 1.0f);
        r.draw_text("RADIO STATUS (XIEGU G90 CAT)", r.font_ui_bold, D2D1::RectF(card1.left + 8, card1.top + 6, card1.right - 8, card1.top + 22), r.hex_color(0x00E5FF));
        std::string rig_f = hamshack::FlrigCatEngine::format_freq_mhz(hs.flrig.vfo_a_hz) + " MHz " + hamshack::FlrigCatEngine::mode_name(hs.flrig.mode_a);
        r.draw_text(rig_f, r.font_mono, D2D1::RectF(card1.left + 8, card1.top + 26, card1.right - 8, card1.top + 46), hs.flrig.connected ? r.hex_color(0x10B981) : r.hex_color(0xEF4444));
        std::string rig_meta = std::string(hs.flrig.connected ? "● CAT ONLINE" : "○ CAT OFFLINE") + " • PTT: " + (hs.flrig.ptt_active ? "TX" : "RX") + " • " + std::to_string(hs.flrig.power_watts) + "W";
        r.draw_text(rig_meta, r.font_mono_small, D2D1::RectF(card1.left + 8, card1.top + 50, card1.right - 8, card1.top + 68), r.hex_color(0x94A3B8));
        sy += 80.0f;

        // 2. Modem & Transport Card
        D2D1_RECT_F card2 = D2D1::RectF(rx + 6, sy, rx + sb_w - 6, sy + 74.0f);
        r.fill_round_rect(card2, 5.0f, r.hex_color(0x05080F));
        r.draw_round_rect(card2, 5.0f, r.hex_color(0x334155), 1.0f);
        r.draw_text("MODEM & TRANSPORT", r.font_ui_bold, D2D1::RectF(card2.left + 8, card2.top + 6, card2.right - 8, card2.top + 22), r.hex_color(0x38BDF8));
        std::string trans_full = (cfg.winlink_transport == 0) ? "Telnet CMS Direct Socket" : (cfg.winlink_transport == 1) ? "ARDOP HF Soundcard TNC" : "Packet AX.25 TNC";
        r.draw_text(trans_full, r.font_mono_small, D2D1::RectF(card2.left + 8, card2.top + 26, card2.right - 8, card2.top + 44), r.hex_color(0x34D399));
        std::string mod_meta = (cfg.winlink_transport == 0)
            ? "Host: cms.winlink.org:8772"
            : (hs.winlink.ardop_modem.is_running()
                ? "ARDOP: ● ONLINE (127.0.0.1:8515)"
                : "ARDOP: ○ STANDBY (Auto-starts on TX)");
        r.draw_text(mod_meta, r.font_mono_small, D2D1::RectF(card2.left + 8, card2.top + 48, card2.right - 8, card2.top + 66),
                    hs.winlink.ardop_modem.is_running() ? r.hex_color(0x10B981) : r.hex_color(0x94A3B8));
        sy += 80.0f;

        // 3. Selected Target Gateway Card
        D2D1_RECT_F card3 = D2D1::RectF(rx + 6, sy, rx + sb_w - 6, sy + 74.0f);
        r.fill_round_rect(card3, 5.0f, r.hex_color(0x05080F));
        r.draw_round_rect(card3, 5.0f, r.hex_color(0x334155), 1.0f);
        r.draw_text("TARGET RMS GATEWAY", r.font_ui_bold, D2D1::RectF(card3.left + 8, card3.top + 6, card3.right - 8, card3.top + 22), r.hex_color(0xF59E0B));
        std::string gw_line = hs.winlink.selected_gateway_call.empty() ? "(Click Gateways tab to pick)" : (hs.winlink.selected_gateway_call + " • " + hs.winlink.selected_gateway_mode);
        r.draw_text(gw_line, r.font_mono, D2D1::RectF(card3.left + 8, card3.top + 26, card3.right - 8, card3.top + 46), r.hex_color(0xFBBF24));
        std::string gw_meta = (hs.winlink.selected_gateway_freq > 0) ? ("Dial: " + hamshack::FlrigCatEngine::format_freq_mhz(hs.winlink.selected_gateway_freq) + " MHz USB • " + hs.winlink.selected_gateway_grid) : "Worldwide CMS Node";
        r.draw_text(gw_meta, r.font_mono_small, D2D1::RectF(card3.left + 8, card3.top + 50, card3.right - 8, card3.top + 68), r.hex_color(0x94A3B8));
        sy += 80.0f;

        // 4. Quick Account Credentials & Sign In Status Pill
        D2D1_RECT_F cred_card = D2D1::RectF(rx + 6, sy, rx + sb_w - 6, sy + 30.0f);
        r.fill_round_rect(cred_card, 4.0f, r.hex_color(0x05080F));
        r.draw_round_rect(cred_card, 4.0f, r.hex_color(0x334155), 1.0f);
        std::string pass_display = cfg.winlink_password.empty() ? "(No Password)" : "••••••••";
        std::string cred_txt = "🔑 " + cfg.winlink_callsign + " • " + pass_display;
        r.draw_text(cred_txt, r.font_mono_small, D2D1::RectF(cred_card.left + 8, cred_card.top + 6, cred_card.right - 90, cred_card.bottom), r.hex_color(0x38BDF8));
        draw_hs_btn(r, hs, mouse_pt, cred_card.right - 88.0f, cred_card.top + 2.0f, 84.0f, 26.0f, HS_WL_SUBTAB_CONFIG,
                    "⚙ Sign In", false, 0xF59E0B, 2, "", true);
        sy += 36.0f;

        // 5. THE BIG SEND & RECEIVE BUTTONS (WEB & RADIO!)
        if (hs.winlink.session_active) {
            float big_btn_h = 42.0f;
            draw_hs_btn(r, hs, mouse_pt, rx + 6, sy, sb_w - 12.0f, big_btn_h, HS_WL_ABORT_SESSION,
                        "■ ABORT ACTIVE SESSION", true, 0xEF4444, 0, "", true);
            sy += big_btn_h + 8.0f;
        } else {
            float btn_w = sb_w - 12.0f;
            float btn_h = 34.0f;
            draw_hs_btn(r, hs, mouse_pt, rx + 6, sy, btn_w, btn_h, HS_WL_EXEC_WEB,
                        "🌐 SEND & RECEIVE VIA WEB", false, 0x10B981, 0, "", true);
            sy += btn_h + 6.0f;

            draw_hs_btn(r, hs, mouse_pt, rx + 6, sy, btn_w, btn_h, HS_WL_EXEC_RADIO,
                        "📻 SEND & RECEIVE VIA RADIO", false, 0x00E5FF, 1, "", true);
            sy += btn_h + 8.0f;
        }

        // Session status banner
        D2D1_RECT_F st_box = D2D1::RectF(rx + 6, sy, rx + sb_w - 6, sy + 24.0f);
        r.fill_round_rect(st_box, 4.0f, r.hex_color(0x05080F));
        r.draw_text(hs.winlink.session_status, r.font_mono_small, st_box,
                    hs.winlink.session_active ? r.hex_color(0x38BDF8) : r.hex_color(0x94A3B8), DWRITE_TEXT_ALIGNMENT_CENTER);
        sy += 28.0f;

        // 5. Live B2F Protocol Session Transcript
        float log_h = (body_y + body_h) - sy - 8.0f;
        if (log_h > 60.0f) {
            D2D1_RECT_F log_box = D2D1::RectF(rx + 6, sy, rx + sb_w - 6, sy + log_h);
            r.fill_round_rect(log_box, 5.0f, r.hex_color(0x05080F));
            r.draw_round_rect(log_box, 5.0f, r.hex_color(0x1E293B), 1.0f);
            r.draw_text("SESSION LOG (B2F TRANSCRIPT)", r.font_ui_bold, D2D1::RectF(log_box.left + 8, log_box.top + 4, log_box.right - 8, log_box.top + 20), r.hex_color(0x64748B));

            std::lock_guard<std::recursive_mutex> lk(hs.winlink.mtx);
            float ly = log_box.top + 22.0f;
            float lh = 16.0f;
            int max_lines = static_cast<int>((log_h - 26.0f) / lh);
            int start_idx = std::max(0, static_cast<int>(hs.winlink.session_transcript.size()) - max_lines);
            for (size_t i = start_idx; i < hs.winlink.session_transcript.size(); ++i) {
                const auto& line = hs.winlink.session_transcript[i];
                uint32_t col = 0xCBD5E1;
                if (line.find("ERROR") != std::string::npos || line.find("Failed") != std::string::npos) col = 0xEF4444;
                else if (line.find("SUCCESS") != std::string::npos || line.find("Complete") != std::string::npos) col = 0x10B981;
                else if (line.find(">>>") != std::string::npos) col = 0x38BDF8;
                else if (line.find("<<<") != std::string::npos) col = 0xF59E0B;
                r.draw_text(line, r.font_mono_small, D2D1::RectF(log_box.left + 8, ly, log_box.right - 8, ly + lh), r.hex_color(col));
                ly += lh;
            }
        }
    }
    // ------------------------------------------------------------------------
    // SUBTAB 1: GATEWAYS (4,807 Channels Spreadsheet View + QSY CAT Tuning)
    // ------------------------------------------------------------------------
    else if (hs.active_winlink_subtab == 1) {
        // Top Filter Bar
        D2D1_RECT_F f_bar = D2D1::RectF(x, body_y, x + w, body_y + 36.0f);
        r.fill_round_rect(f_bar, 5.0f, r.hex_color(0x0F172A));
        r.draw_round_rect(f_bar, 5.0f, r.hex_color(0x1E293B), 1.0f);

        // Search Input Box
        r.draw_text("Search:", r.font_ui_bold, D2D1::RectF(x + 10, body_y + 8, x + 65, body_y + 28), r.hex_color(0x94A3B8));
        D2D1_RECT_F s_box = D2D1::RectF(x + 70, body_y + 5.0f, x + 240, body_y + 31.0f);
        bool s_foc = (hs.text_focus == HsTextFocus::WlGwSearch);
        r.fill_round_rect(s_box, 4.0f, r.hex_color(0x05080F));
        r.draw_round_rect(s_box, 4.0f, s_foc ? r.hex_color(0x00E5FF) : r.hex_color(0x334155), 1.0f);
        std::string s_disp = hs.winlink.gateway_search_query + (s_foc ? "_" : "");
        if (s_disp.empty() && !s_foc) s_disp = "Call, Grid, Freq...";
        r.draw_text(s_disp, r.font_mono_small, D2D1::RectF(s_box.left + 6, s_box.top + 5, s_box.right - 6, s_box.bottom),
                    s_foc || !hs.winlink.gateway_search_query.empty() ? r.hex_color(0xF8FAFC) : r.hex_color(0x64748B));
        hs.add_hit(s_box, HS_WL_GW_SEARCH_BOX);

        // Mode Filter Buttons
        float mx = x + 252.0f;
        struct ModeBtn { int id; const char* label; const char* mode; };
        ModeBtn m_btns[5] = {
            {HS_WL_GW_MODE_ALL,    "All Modes", "All"},
            {HS_WL_GW_MODE_ARDOP,  "ARDOP",     "ARDOP"},
            {HS_WL_GW_MODE_VARA,   "VARA",      "VARA"},
            {HS_WL_GW_MODE_PACKET, "Packet",    "Packet"},
            {HS_WL_GW_MODE_PACTOR, "Pactor",    "Pactor"}
        };
        for (int i = 0; i < 5; ++i) {
            bool sel_m = (hs.winlink.gateway_mode_filter == m_btns[i].mode);
            float mw = 85.0f;
            draw_hs_btn(r, hs, mouse_pt, mx, body_y + 5.0f, mw, 26.0f, m_btns[i].id,
                        m_btns[i].label, sel_m, sel_m ? 0x00E5FF : 0x64748B, i, "", true);
            mx += mw + 4.0f;
        }

        // Count Banner
        std::string cnt_str = "Showing " + std::to_string(hs.winlink.filtered_gateways.size()) + " of " +
                              std::to_string(hs.winlink.all_gateways.size()) + " Channels (Sorted by Distance from " + cfg.winlink_grid + ")";
        r.draw_text(cnt_str, r.font_mono_small, D2D1::RectF(mx + 10, body_y + 8, x + w - 10, body_y + 28), r.hex_color(0x10B981));

        // Spreadsheet Table
        float tbl_y = body_y + 42.0f;
        float tbl_h = body_h - 42.0f;
        D2D1_RECT_F tbl_r = D2D1::RectF(x, tbl_y, x + w, tbl_y + tbl_h);
        r.fill_round_rect(tbl_r, 6.0f, r.hex_color(0x05080F));
        r.draw_round_rect(tbl_r, 6.0f, r.hex_color(0x1E293B), 1.0f);

        // Header Row
        D2D1_RECT_F thdr = D2D1::RectF(x + 2, tbl_y + 2, x + w - 2, tbl_y + 24.0f);
        r.fill_round_rect(thdr, 4.0f, r.hex_color(0x0F172A));
        r.draw_text("CALLSIGN", r.font_ui_bold, D2D1::RectF(x + 12, tbl_y + 4, x + 120, tbl_y + 22), r.hex_color(0x94A3B8));
        r.draw_text("DIAL FREQ", r.font_ui_bold, D2D1::RectF(x + 125, tbl_y + 4, x + 235, tbl_y + 22), r.hex_color(0x94A3B8));
        r.draw_text("MODE", r.font_ui_bold, D2D1::RectF(x + 240, tbl_y + 4, x + 380, tbl_y + 22), r.hex_color(0x94A3B8));
        r.draw_text("GRID", r.font_ui_bold, D2D1::RectF(x + 385, tbl_y + 4, x + 465, tbl_y + 22), r.hex_color(0x94A3B8));
        r.draw_text("DISTANCE", r.font_ui_bold, D2D1::RectF(x + 470, tbl_y + 4, x + 560, tbl_y + 22), r.hex_color(0x94A3B8));
        r.draw_text("AZIMUTH", r.font_ui_bold, D2D1::RectF(x + 565, tbl_y + 4, x + 645, tbl_y + 22), r.hex_color(0x94A3B8));
        r.draw_text("HOURS", r.font_ui_bold, D2D1::RectF(x + 650, tbl_y + 4, x + 760, tbl_y + 22), r.hex_color(0x94A3B8));
        r.draw_text("SERVICE", r.font_ui_bold, D2D1::RectF(x + 765, tbl_y + 4, x + 860, tbl_y + 22), r.hex_color(0x94A3B8));
        r.draw_text("RIG QSY ACTION", r.font_ui_bold, D2D1::RectF(x + 865, tbl_y + 4, x + w - 12, tbl_y + 22), r.hex_color(0x00E5FF));

        // Data Rows
        float ry = tbl_y + 26.0f;
        float rh = 24.0f;
        int max_gw_rows = static_cast<int>((tbl_h - 28.0f) / rh);

        for (size_t i = 0; i < hs.winlink.filtered_gateways.size() && static_cast<int>(i) < max_gw_rows; ++i) {
            const auto& gw = hs.winlink.filtered_gateways[i];
            bool is_sel = (static_cast<int>(i) == hs.winlink.selected_gateway_idx);
            D2D1_RECT_F row_r = D2D1::RectF(x + 2, ry, x + w - 2, ry + rh);

            // Rank gateways by distance to operator QTH:
            // < 500 km = Light Green (Best), 500..1500 km = Light Yellow (Moderate), >= 1500 km = Light Red (Distant)
            uint32_t rank_col = 0x22C55E; // Light Green (Best)
            float rank_alpha = 0.16f;
            if (gw.distance_km >= 1500.0) {
                rank_col = 0xEF4444;      // Light Red (Distant)
                rank_alpha = 0.12f;
            } else if (gw.distance_km >= 500.0) {
                rank_col = 0xEAB308;      // Light Yellow (Moderate)
                rank_alpha = 0.14f;
            }

            if (is_sel) {
                r.fill_round_rect(row_r, 4.0f, r.hex_color(0x1E293B));
                r.draw_round_rect(row_r, 4.0f, r.hex_color(0x38BDF8), 1.2f);
            } else if (RendererT::pt_in_rect(mouse_pt, row_r)) {
                r.fill_round_rect(row_r, 4.0f, r.hex_color(0x0E1726));
            } else {
                r.fill_round_rect(row_r, 4.0f, r.hex_color(rank_col, rank_alpha));
            }

            // Left ranking indicator stripe
            r.fill_round_rect(D2D1::RectF(x + 2, ry + 2.0f, x + 6.0f, ry + rh - 2.0f), 1.5f, r.hex_color(rank_col, 0.85f));

            r.draw_text(gw.callsign, r.font_mono, D2D1::RectF(x + 12, ry + 4, x + 120, ry + 22), r.hex_color(is_sel ? 0x00E5FF : 0x38BDF8));
            r.draw_text(hamshack::FlrigCatEngine::format_freq_mhz(gw.freq_hz) + " MHz", r.font_mono_small, D2D1::RectF(x + 125, ry + 4, x + 235, ry + 22), r.hex_color(0x34D399));
            r.draw_text(gw.mode, r.font_mono_small, D2D1::RectF(x + 240, ry + 4, x + 380, ry + 22), r.hex_color(0xFBBF24));
            r.draw_text(gw.grid, r.font_mono_small, D2D1::RectF(x + 385, ry + 4, x + 465, ry + 22), r.hex_color(0xCBD5E1));
            char d_buf[32];
            std::snprintf(d_buf, sizeof(d_buf), "%.0f km", gw.distance_km);
            r.draw_text(d_buf, r.font_mono_small, D2D1::RectF(x + 470, ry + 4, x + 560, ry + 22), r.hex_color(rank_col));
            char a_buf[32];
            std::snprintf(a_buf, sizeof(a_buf), "%.0f°", gw.azimuth_deg);
            r.draw_text(a_buf, r.font_mono_small, D2D1::RectF(x + 565, ry + 4, x + 645, ry + 22), r.hex_color(0x94A3B8));
            r.draw_text(gw.hours, r.font_mono_small, D2D1::RectF(x + 650, ry + 4, x + 760, ry + 22), r.hex_color(0x94A3B8));
            r.draw_text(gw.service, r.font_mono_small, D2D1::RectF(x + 765, ry + 4, x + 860, ry + 22), r.hex_color(0x64748B));

            // [⚡ TUNE & SELECT] Button
            float act_w = 120.0f;
            draw_hs_btn(r, hs, mouse_pt, x + 865, ry + 1.0f, act_w, 22.0f, HS_WL_GW_ROW_BASE + static_cast<int>(i),
                        is_sel ? "✓ SELECTED" : "⚡ TUNE & SELECT", is_sel, is_sel ? 0x10B981 : 0x00E5FF, static_cast<int>(i), "", true);

            hs.add_hit(row_r, HS_WL_GW_ROW_BASE + static_cast<int>(i), static_cast<int>(i));
            ry += rh;
        }
    }
    // ------------------------------------------------------------------------
    // SUBTAB 2: CONFIGURATION
    // ------------------------------------------------------------------------
    else if (hs.active_winlink_subtab == 2) {
        float cfg_w = std::min(680.0f, w - 20.0f);
        D2D1_RECT_F cfg_card = D2D1::RectF(x + 10, body_y, x + 10 + cfg_w, body_y + body_h);
        r.fill_round_rect(cfg_card, 8.0f, r.hex_color(0x0F172A));
        r.draw_round_rect(cfg_card, 8.0f, r.hex_color(0x1E293B), 1.0f);

        float cy = body_y + 16.0f;
        r.draw_text("WINLINK 2000 ACCOUNT & MODEM CONFIGURATION", r.font_ui_bold,
                    D2D1::RectF(x + 24, cy, x + cfg_w, cy + 24), r.hex_color(0x00E5FF));
        cy += 32.0f;

        // Callsign
        r.draw_text("Station Callsign:", r.font_ui_bold, D2D1::RectF(x + 24, cy + 4, x + 200, cy + 24), r.hex_color(0x94A3B8));
        D2D1_RECT_F call_box = D2D1::RectF(x + 210, cy, x + 400, cy + 26.0f);
        bool call_foc = (hs.text_focus == HsTextFocus::WlCfgCall);
        r.fill_round_rect(call_box, 4.0f, r.hex_color(0x05080F));
        r.draw_round_rect(call_box, 4.0f, call_foc ? r.hex_color(0x00E5FF) : r.hex_color(0x334155), 1.0f);
        r.draw_text(cfg.winlink_callsign + (call_foc ? "_" : ""), r.font_mono,
                    D2D1::RectF(call_box.left + 8, call_box.top + 4, call_box.right - 8, call_box.bottom), r.hex_color(0x38BDF8));
        hs.add_hit(call_box, HS_WL_CFG_CALL_BOX);
        cy += 34.0f;

        // Password
        r.draw_text("Winlink CMS Password:", r.font_ui_bold, D2D1::RectF(x + 24, cy + 4, x + 200, cy + 24), r.hex_color(0x94A3B8));
        D2D1_RECT_F pass_box = D2D1::RectF(x + 210, cy, x + 400, cy + 26.0f);
        bool pass_foc = (hs.text_focus == HsTextFocus::WlCfgPass);
        r.fill_round_rect(pass_box, 4.0f, r.hex_color(0x05080F));
        r.draw_round_rect(pass_box, 4.0f, pass_foc ? r.hex_color(0x00E5FF) : r.hex_color(0x334155), 1.0f);
        std::string p_mask = pass_foc ? (cfg.winlink_password + "_") : std::string(cfg.winlink_password.size(), '*');
        if (p_mask.empty() && !pass_foc) p_mask = "(Optional - set if required by CMS)";
        r.draw_text(p_mask, r.font_mono_small,
                    D2D1::RectF(pass_box.left + 8, pass_box.top + 5, pass_box.right - 8, pass_box.bottom),
                    pass_foc || !cfg.winlink_password.empty() ? r.hex_color(0xF8FAFC) : r.hex_color(0x64748B));
        hs.add_hit(pass_box, HS_WL_CFG_PASS_BOX);
        cy += 34.0f;

        // Grid Locator
        r.draw_text("Station Grid Locator:", r.font_ui_bold, D2D1::RectF(x + 24, cy + 4, x + 200, cy + 24), r.hex_color(0x94A3B8));
        D2D1_RECT_F grid_box = D2D1::RectF(x + 210, cy, x + 310, cy + 26.0f);
        bool grid_foc = (hs.text_focus == HsTextFocus::WlCfgGrid);
        r.fill_round_rect(grid_box, 4.0f, r.hex_color(0x05080F));
        r.draw_round_rect(grid_box, 4.0f, grid_foc ? r.hex_color(0x00E5FF) : r.hex_color(0x334155), 1.0f);
        r.draw_text(cfg.winlink_grid + (grid_foc ? "_" : ""), r.font_mono,
                    D2D1::RectF(grid_box.left + 8, grid_box.top + 4, grid_box.right - 8, grid_box.bottom), r.hex_color(0x38BDF8));
        hs.add_hit(grid_box, HS_WL_CFG_GRID_BOX);
        cy += 40.0f;

        // Transport Selector
        r.draw_text("Session Transport Mode:", r.font_ui_bold, D2D1::RectF(x + 24, cy, x + cfg_w, cy + 20), r.hex_color(0xF59E0B));
        cy += 24.0f;

        draw_hs_btn(r, hs, mouse_pt, x + 24, cy, 180.0f, 28.0f, HS_WL_CFG_TRANS_TELNET,
                    "● Telnet CMS (Internet)", cfg.winlink_transport == 0, 0x10B981, 0, "", true);
        draw_hs_btn(r, hs, mouse_pt, x + 212, cy, 180.0f, 28.0f, HS_WL_CFG_TRANS_ARDOP,
                    "● ARDOP HF Radio", cfg.winlink_transport == 1, 0x38BDF8, 1, "", true);
        draw_hs_btn(r, hs, mouse_pt, x + 400, cy, 180.0f, 28.0f, HS_WL_CFG_TRANS_PACKET,
                    "● Packet AX.25", cfg.winlink_transport == 2, 0xA855F7, 2, "", true);
        cy += 38.0f;

        // ARDOP TNC Settings
        r.draw_text("ARDOP TNC Settings (pat / ardop):", r.font_ui_bold, D2D1::RectF(x + 24, cy, x + cfg_w, cy + 20), r.hex_color(0xCBD5E1));
        cy += 24.0f;
        r.draw_text("Host: 127.0.0.1:8515  |  Bandwidth: " + std::to_string(cfg.winlink_ardop_bandwidth) + " Hz  |  Drive: " + std::to_string(cfg.winlink_ardop_drive),
                    r.font_mono_small, D2D1::RectF(x + 24, cy, x + cfg_w, cy + 18), r.hex_color(0x94A3B8));
        cy += 24.0f;

        draw_hs_btn(r, hs, mouse_pt, x + 24, cy, 160.0f, 26.0f, HS_WL_CFG_BW_CYCLE,
                    "Cycle ARDOP BW (" + std::to_string(cfg.winlink_ardop_bandwidth) + ")", false, 0x38BDF8, 0, "", true);
        cy += 38.0f;

        // Save Button
        draw_hs_btn(r, hs, mouse_pt, x + 24, cy, 150.0f, 32.0f, HS_WL_CFG_SAVE_BTN,
                    "💾 Save Config", false, 0x10B981, 0, "", true);
    }
}

// ============================================================================
// TAB 8: Configuration Deck, Log Exports, FLRig CAT Matrix & Soundcard Setup
// ============================================================================
template <typename RendererT, typename AppT>
inline void render_configuration_workspace(RendererT& r, AppT& app, HamShackSuite& hs, POINT mouse_pt,
                                           float x, float y, float w, float h) {
    render_live_g90_telemetry_grid(r, hs, x, y, w, 38.0f);

    float body_y = y + 44.0f;
    float body_h = h - 44.0f;
    float col_w = (w - 16.0f) / 3.0f;

    // ---- COLUMN 1: Live Xiegu G90 Hardware Control Matrix & Memory Bank ----
    D2D1_RECT_F c1 = D2D1::RectF(x, body_y, x + col_w, body_y + body_h);
    r.fill_round_rect(c1, 8.0f, r.hex_color(0x0F172A));
    r.draw_round_rect(c1, 8.0f, r.hex_color(0x1E293B), 1.2f);
    r.draw_text("XIEGU G90 CAT HARDWARE CONTROLLER", r.font_ui_bold,
                D2D1::RectF(x + 12, body_y + 6, x + col_w - 12, body_y + 26), r.hex_color(0x00E5FF));

    float y1 = body_y + 30.0f;
    r.draw_text(hs.flrig.status_text, r.font_mono_small,
                D2D1::RectF(x + 12, y1, x + col_w - 12, y1 + 18),
                hs.flrig.connected ? r.hex_color(0x10B981) : r.hex_color(0xEF4444));
    y1 += 22.0f;

    float bw3 = (col_w - 28.0f) / 3.0f;
    draw_hs_btn(r, hs, mouse_pt, x + 10,                y1, bw3, 26, HS_CAT_PORT_CYCLE,  "Cycle USB Port", false, 0x00E5FF, 0, "", true);
    draw_hs_btn(r, hs, mouse_pt, x + 14 + bw3,          y1, bw3, 26, HS_CAT_RESCAN,      "Rescan USB CAT", false, 0x10B981, 0, "", true);
    draw_hs_btn(r, hs, mouse_pt, x + 18 + bw3 * 2.0f,   y1, bw3, 26, HS_RIG_MODEL_CYCLE, "Model: " + std::string(hamshack::FlrigCatEngine::model_name(hs.flrig.model)), false, 0x38BDF8, 0, "", true);
    y1 += 32.0f;

    // Steppers + Vol + SQL + Pwr
    float bw6 = (col_w - 40.0f) / 6.0f;
    draw_hs_btn(r, hs, mouse_pt, x + 10 + 0*(bw6+4), y1, bw6, 25, HS_VFO_STEP_M10K, "-10k", false, 0x38BDF8, 0, "", true);
    draw_hs_btn(r, hs, mouse_pt, x + 10 + 1*(bw6+4), y1, bw6, 25, HS_VFO_STEP_M1K,  "-1k",  false, 0x38BDF8, 0, "", true);
    draw_hs_btn(r, hs, mouse_pt, x + 10 + 2*(bw6+4), y1, bw6, 25, HS_VFO_STEP_M100, "-100", false, 0x38BDF8, 0, "", true);
    draw_hs_btn(r, hs, mouse_pt, x + 10 + 3*(bw6+4), y1, bw6, 25, HS_VFO_STEP_P100, "+100", false, 0x38BDF8, 0, "", true);
    draw_hs_btn(r, hs, mouse_pt, x + 10 + 4*(bw6+4), y1, bw6, 25, HS_VFO_STEP_P1K,  "+1k",  false, 0x38BDF8, 0, "", true);
    draw_hs_btn(r, hs, mouse_pt, x + 10 + 5*(bw6+4), y1, bw6, 25, HS_VFO_STEP_P10K, "+10k", false, 0x38BDF8, 0, "", true);
    y1 += 30.0f;

    draw_hs_btn(r, hs, mouse_pt, x + 10 + 0*(bw6+4), y1, bw6, 25, HS_VOL_DOWN, "-Vol", false, 0x10B981, 0, "", true);
    draw_hs_btn(r, hs, mouse_pt, x + 10 + 1*(bw6+4), y1, bw6, 25, HS_VOL_UP,   "+Vol", false, 0x10B981, 0, "", true);
    draw_hs_btn(r, hs, mouse_pt, x + 10 + 2*(bw6+4), y1, bw6, 25, HS_SQL_DOWN, "-SQL", false, 0x00E5FF, 0, "", true);
    draw_hs_btn(r, hs, mouse_pt, x + 10 + 3*(bw6+4), y1, bw6, 25, HS_SQL_UP,   "+SQL", false, 0x00E5FF, 0, "", true);
    draw_hs_btn(r, hs, mouse_pt, x + 10 + 4*(bw6+4), y1, bw6, 25, HS_PWR_DOWN, "-Pwr", false, 0xFBBF24, 0, "", true);
    draw_hs_btn(r, hs, mouse_pt, x + 10 + 5*(bw6+4), y1, bw6, 25, HS_PWR_UP,   "+Pwr", false, 0xFBBF24, 0, "", true);
    y1 += 32.0f;

    r.draw_text("XIEGU G90 MEMORY CHANNEL MATRIX (Click to QSY):", r.font_ui_bold,
                D2D1::RectF(x + 12, y1, x + col_w - 12, y1 + 20), r.hex_color(0xFBBF24));
    y1 += 22.0f;

    for (size_t i = 0; i < hs.flrig.memories.size() && y1 + 24.0f < body_y + body_h - 6.0f; ++i) {
        const auto& m = hs.flrig.memories[i];
        char mbuf[128];
        std::snprintf(mbuf, sizeof(mbuf), "CH%02zu  %-17s  %s MHz  [%s F%d]",
                      i + 1, m.label.c_str(),
                      hamshack::FlrigCatEngine::format_freq_mhz(m.freq_hz).c_str(),
                      hamshack::FlrigCatEngine::mode_name(m.mode), m.filter);
        bool act = hs.flrig.connected && (hs.flrig.vfo_a_hz == m.freq_hz);
        draw_hs_btn(r, hs, mouse_pt, x + 10, y1, col_w - 20, 23, HS_MEM_CH_BASE + static_cast<int>(i),
                    mbuf, act, 0x10B981, static_cast<int>(i), "", true);
        y1 += 25.0f;
    }

    // ---- COLUMN 2: Soundcard Matrix & Live S-Meter History ----
    float x2 = x + col_w + 8.0f;
    D2D1_RECT_F c2 = D2D1::RectF(x2, body_y, x2 + col_w, body_y + body_h);
    r.fill_round_rect(c2, 8.0f, r.hex_color(0x0F172A));
    r.draw_round_rect(c2, 8.0f, r.hex_color(0x1E293B), 1.2f);
#ifdef _WIN32
    r.draw_text("WASAPI / WINMM & USB SOUNDCARD MATRIX", r.font_ui_bold,
                D2D1::RectF(x2 + 12, body_y + 6, x2 + col_w - 12, body_y + 26), r.hex_color(0x34D399));
#else
    r.draw_text("COREAUDIO & USB SOUNDCARD MATRIX", r.font_ui_bold,
                D2D1::RectF(x2 + 12, body_y + 6, x2 + col_w - 12, body_y + 26), r.hex_color(0x34D399));
#endif

    float y2 = body_y + 32.0f;
    draw_hs_btn(r, hs, mouse_pt, x2 + 10, y2, 114, 26, HS_AUDIO_REFRESH,   "Rescan Audio", false, 0x38BDF8, 0, "", true);
    draw_hs_btn(r, hs, mouse_pt, x2 + 128,y2, 122, 26, HS_AUDIO_TEST_TONE, "TX 1kHz Tone", false, 0xFBBF24, 0, "", true);
    draw_hs_btn(r, hs, mouse_pt, x2 + 254,y2, col_w - 264, 26, HS_AUDIO_MON_TOG,
                hs.soundcard.monitor_tx_audio ? "TX Sidetone: ON" : "TX Sidetone: OFF",
                hs.soundcard.monitor_tx_audio, 0x10B981, 0, "", true);
    y2 += 32.0f;

#ifdef _WIN32
    static constexpr const char* kSysAudioTag = "[WIN] ";
#else
    static constexpr const char* kSysAudioTag = "[MAC] ";
#endif

    r.draw_text("INPUT SOUNDCARDS (12,000 Hz DSP):", r.font_small,
                D2D1::RectF(x2 + 12, y2, x2 + col_w - 12, y2 + 18), r.hex_color(0x94A3B8));
    y2 += 19.0f;
    for (size_t i = 0; i < hs.soundcard.input_devices.size() && i < 4; ++i) {
        const auto& dev = hs.soundcard.input_devices[i];
        bool sel = (static_cast<int>(i) == hs.soundcard.selected_input_idx);
        std::string lbl = (dev.is_usb ? "[USB] " : kSysAudioTag) + dev.name + " (" + std::to_string(dev.nominal_rate_hz / 1000) + "kHz)";
        draw_hs_btn(r, hs, mouse_pt, x2 + 10, y2, col_w - 20, 25, HS_AUDIO_IN_ROW,
                    lbl, sel, 0x00E5FF, static_cast<int>(i), "", true);
        y2 += 27.0f;
    }

    y2 += 4.0f;
    r.draw_text("OUTPUT SOUNDCARDS (12,000 Hz DSP):", r.font_small,
                D2D1::RectF(x2 + 12, y2, x2 + col_w - 12, y2 + 18), r.hex_color(0x94A3B8));
    y2 += 19.0f;
    for (size_t i = 0; i < hs.soundcard.output_devices.size() && i < 4; ++i) {
        const auto& dev = hs.soundcard.output_devices[i];
        bool sel = (static_cast<int>(i) == hs.soundcard.selected_output_idx);
        std::string lbl = (dev.is_usb ? "[USB] " : kSysAudioTag) + dev.name + " (" + std::to_string(dev.nominal_rate_hz / 1000) + "kHz)";
        draw_hs_btn(r, hs, mouse_pt, x2 + 10, y2, col_w - 20, 25, HS_AUDIO_OUT_ROW,
                    lbl, sel, 0x10B981, static_cast<int>(i), "", true);
        y2 += 27.0f;
    }

    // Live S-Meter History Strip Chart & Twitch Live Streaming Studio
    y2 += 8.0f;
    float tw_h = 168.0f;
    D2D1_RECT_F tw_box = D2D1::RectF(x2 + 10, y2, x2 + col_w - 10, y2 + tw_h);
    r.fill_round_rect(tw_box, 6.0f, r.hex_color(0x05080F));
    r.draw_round_rect(tw_box, 6.0f, hs.twitch.is_streaming ? r.hex_color(0x22C55E, 0.85f) : r.hex_color(0xA855F7, 0.65f), 1.2f);

    float ty = y2 + 6.0f;
    r.draw_text("TWITCH LIVE STREAMING STUDIO (RTMP)", r.font_ui_bold,
                D2D1::RectF(tw_box.left + 8, ty, tw_box.right - 90, ty + 20), r.hex_color(0xA855F7));
    r.draw_text(hs.twitch.is_streaming ? "● LIVE 60FPS" : "○ OFFLINE", r.font_mono_small,
                D2D1::RectF(tw_box.right - 90, ty, tw_box.right - 8, ty + 20),
                hs.twitch.is_streaming ? r.hex_color(0x22C55E) : r.hex_color(0x94A3B8), DWRITE_TEXT_ALIGNMENT_TRAILING);
    ty += 24.0f;

    auto& cfg = hamshack::get_app_config();
    bool chan_foc = (hs.text_focus == HsTextFocus::TwitchChannel);
    bool key_foc  = (hs.text_focus == HsTextFocus::TwitchStreamKey);

    // Channel Box
    float half_w = (col_w - 28.0f) / 2.0f;
    D2D1_RECT_F chan_rc = D2D1::RectF(tw_box.left + 6, ty, tw_box.left + 6 + half_w, ty + 24);
    r.fill_round_rect(chan_rc, 4.0f, r.hex_color(0x0F172A));
    r.draw_round_rect(chan_rc, 4.0f, chan_foc ? r.hex_color(0x00E5FF) : r.hex_color(0x26354D), 1.0f);
    std::string disp_chan = cfg.twitch_channel.empty() ? "Channel: (Not connected)" : ("Ch: @" + cfg.twitch_channel);
    r.draw_text(disp_chan + (chan_foc ? "_" : ""), r.font_mono_small, D2D1::RectF(chan_rc.left + 6, chan_rc.top + 3, chan_rc.right - 4, chan_rc.bottom),
                chan_foc ? r.hex_color(0x00E5FF) : (cfg.twitch_channel.empty() ? r.hex_color(0x94A3B8) : r.hex_color(0x38BDF8)));
    hs.add_hit(chan_rc, HS_TWITCH_CHANNEL_BOX);

    // Key Box (masked for privacy)
    D2D1_RECT_F key_rc = D2D1::RectF(tw_box.left + 10 + half_w, ty, tw_box.right - 6, ty + 24);
    r.fill_round_rect(key_rc, 4.0f, r.hex_color(0x0F172A));
    r.draw_round_rect(key_rc, 4.0f, key_foc ? r.hex_color(0x00E5FF) : r.hex_color(0x26354D), 1.0f);
    std::string masked_key = cfg.twitch_stream_key.empty() ? "Key: (Auto on sign-in)" : ("Key: " + std::string(std::min<size_t>(8, cfg.twitch_stream_key.size()), '*') + "... (Ready)");
    r.draw_text(masked_key + (key_foc ? "_" : ""), r.font_mono_small, D2D1::RectF(key_rc.left + 6, key_rc.top + 3, key_rc.right - 4, key_rc.bottom),
                key_foc ? r.hex_color(0x00E5FF) : (cfg.twitch_stream_key.empty() ? r.hex_color(0x94A3B8) : r.hex_color(0x34D399)));
    hs.add_hit(key_rc, HS_TWITCH_KEY_BOX);
    ty += 28.0f;

    // Action Buttons
    float btn_w3 = (col_w - 32.0f) / 3.0f;
    std::string login_label;
    if (hs.twitch.auth_in_progress.load()) {
        login_label = hs.twitch.auth_user_code.empty() ? "⏳ Connecting..." : ("Code: " + hs.twitch.auth_user_code);
    } else {
        login_label = cfg.twitch_channel.empty() ? "🟣 Connect Twitch" : "🟣 Reconnect";
    }
    draw_hs_btn(r, hs, mouse_pt, tw_box.left + 6, ty, btn_w3, 26, HS_TWITCH_LOGIN_WEB,
                login_label, hs.twitch.auth_in_progress.load(), 0xA855F7, 0, "", true);
    draw_hs_btn(r, hs, mouse_pt, tw_box.left + 8 + btn_w3, ty, btn_w3, 26, HS_TWITCH_DASHBOARD_WEB,
                "🔑 Dashboard", false, 0x38BDF8, 0, "", true);
    draw_hs_btn(r, hs, mouse_pt, tw_box.left + 10 + btn_w3 * 2.0f, ty, btn_w3, 26, HS_TWITCH_STREAM_TOGGLE,
                hs.twitch.is_streaming ? "⏹ Stop Stream" : "▶ Go Live", hs.twitch.is_streaming,
                hs.twitch.is_streaming ? 0xEF4444 : 0x22C55E, 0, "", true);
    ty += 32.0f;

    // Status / metrics
    std::string tw_stat = hs.twitch.status_text;
    if (hs.twitch.is_streaming) {
        tw_stat += " • " + hs.twitch.uptime_str() + " • " + std::to_string(hs.twitch.frames_streamed) + " frames";
    }
    r.draw_text(tw_stat, r.font_mono_small, D2D1::RectF(tw_box.left + 8, ty, tw_box.right - 8, ty + 18),
                hs.twitch.is_streaming ? r.hex_color(0x34D399) : (hs.twitch.auth_in_progress.load() ? r.hex_color(0xFACC15) : r.hex_color(0x94A3B8)));
    ty += 20.0f;
    std::string footer_desc = "Live full-window streaming to twitch.tv/" + (cfg.twitch_channel.empty() ? "(your channel)" : cfg.twitch_channel) + " • Audio: Muted (-an)";
    r.draw_text(footer_desc,
                r.font_small, D2D1::RectF(tw_box.left + 8, ty, tw_box.right - 8, ty + 16), r.hex_color(0x64748B));

    y2 += tw_h + 8.0f;

    // Live S-Meter History Strip Chart
    float chart_h = std::max(60.0f, (body_y + body_h - 10.0f) - y2);
    D2D1_RECT_F ch_box = D2D1::RectF(x2 + 10, y2, x2 + col_w - 10, y2 + chart_h);
    r.fill_round_rect(ch_box, 6.0f, r.hex_color(0x05080F));
    r.draw_round_rect(ch_box, 6.0f, r.hex_color(0x1E293B), 1.0f);
    r.draw_text("LIVE G90 S-METER TELEMETRY GRAPH (CI-V 0x15 0x02)", r.font_mono_small,
                D2D1::RectF(ch_box.left + 8, ch_box.top + 4, ch_box.right - 8, ch_box.top + 20), r.hex_color(0x38BDF8));
    {
        std::lock_guard<std::mutex> lk(hs.flrig.mtx);
        if (!hs.flrig.connected || hs.flrig.smeter_history.empty()) {
            r.draw_text("G90 OFFLINE — No live S-Meter telemetry", r.font_mono_small, ch_box,
                        r.hex_color(0x64748B), DWRITE_TEXT_ALIGNMENT_CENTER);
        } else {
            int n = static_cast<int>(hs.flrig.smeter_history.size());
            float bw = (ch_box.right - ch_box.left - 16.0f) / static_cast<float>(std::max(20, n));
            float max_bh = std::max(20.0f, chart_h - 30.0f);
            for (int i = 0; i < n; ++i) {
                float v = std::clamp(hs.flrig.smeter_history[i] / 100.0f, 0.03f, 1.0f);
                float bh = v * max_bh;
                float bx_pos = ch_box.left + 8.0f + i * bw;
                r.fill_round_rect(D2D1::RectF(bx_pos, ch_box.bottom - 6.0f - bh, bx_pos + std::max(2.0f, bw - 1.0f), ch_box.bottom - 6.0f),
                                  1.0f, v > 0.55f ? r.hex_color(0xF59E0B) : r.hex_color(0x10B981));
            }
        }
    }

    // ---- COLUMN 3: Logbook Management, Export CSV/KML/ADI & CI-V Inspector ----
    float x3 = x2 + col_w + 8.0f;
    D2D1_RECT_F c3 = D2D1::RectF(x3, body_y, x3 + col_w, body_y + body_h);
    r.fill_round_rect(c3, 8.0f, r.hex_color(0x0F172A));
    r.draw_round_rect(c3, 8.0f, r.hex_color(0x1E293B), 1.2f);
    r.draw_text("LOGBOOK & DATA EXPORT / IMPORT CONTROLS", r.font_ui_bold,
                D2D1::RectF(x3 + 12, body_y + 6, x3 + col_w - 12, body_y + 26), r.hex_color(0xFBBF24));

    float y3 = body_y + 32.0f;
    float bw_imp = (col_w - 24.0f) / 2.0f;
    draw_hs_btn(r, hs, mouse_pt, x3 + 10, y3, bw_imp, 26, HS_GT_IMPORT_ADI, "Import .ADI", false, 0xFBBF24, 0, "", true);
    draw_hs_btn(r, hs, mouse_pt, x3 + 14 + bw_imp, y3, bw_imp, 26, HS_GT_SYNC_WSJTX_ADI, "Sync WSJT-X", false, 0x10B981, 0, "", true);
    y3 += 32.0f;

    float bw_exp = (col_w - 28.0f) / 3.0f;
    draw_hs_btn(r, hs, mouse_pt, x3 + 10,                y3, bw_exp, 26, HS_CONFIG_EXPORT_CSV, "Export CSV", false, 0x38BDF8, 0, "", true);
    draw_hs_btn(r, hs, mouse_pt, x3 + 14 + bw_exp,       y3, bw_exp, 26, HS_CONFIG_EXPORT_KML, "Export KML", false, 0x34D399, 0, "", true);
    draw_hs_btn(r, hs, mouse_pt, x3 + 18 + bw_exp * 2.0f,y3, bw_exp, 26, HS_CONFIG_EXPORT_ADI, "Export ADI", false, 0xF59E0B, 0, "", true);
    y3 += 30.0f;

    // Web Reporting (PSK Reporter) Controls & Toggle
    bool wr_en = hamshack::get_psk_reporter().is_enabled();
    draw_hs_btn(r, hs, mouse_pt, x3 + 10, y3, col_w - 20, 26, HS_CFG_WEB_REPORT_TOGGLE,
                wr_en ? "\xe2\x97\x8f Web Reporting (PSK Reporter): ENABLED" : "\xe2\x97\x8b Web Reporting (PSK Reporter): DISABLED",
                wr_en, wr_en ? 0x10B981 : 0xEF4444, 0, "", true);
    y3 += 30.0f;
    r.draw_text(hamshack::get_psk_reporter().status_summary(), r.font_mono_small,
                D2D1::RectF(x3 + 12, y3, x3 + col_w - 12, y3 + 18),
                wr_en ? r.hex_color(0x38BDF8) : r.hex_color(0x94A3B8));
    y3 += 22.0f;

    // Status of Logbook / Exports
    std::string exp_status = hs.config_export_status.empty() ? hs.ft8.adif_status_text : hs.config_export_status;
    r.draw_text(exp_status, r.font_mono_small, D2D1::RectF(x3 + 12, y3, x3 + col_w - 12, y3 + 18), r.hex_color(0x34D399));
    y3 += 24.0f;

    // Multi-Radio Bridge Box
    D2D1_RECT_F mr_box = D2D1::RectF(x3 + 10, y3, x3 + col_w - 10, y3 + 82);
    r.fill_round_rect(mr_box, 6.0f, r.hex_color(0x05080F));
    r.draw_round_rect(mr_box, 6.0f, r.hex_color(0x26354D), 1.0f);
    r.draw_text("RADIO 1 (HF CAT): " + hs.flrig.status_text, r.font_mono_small,
                D2D1::RectF(mr_box.left + 8, y3 + 4, mr_box.right - 8, y3 + 22),
                hs.flrig.connected ? r.hex_color(0x10B981) : r.hex_color(0xEF4444));
    r.draw_text("RADIO 2 (AUDIO):  " + hs.soundcard.active_input_name + " / " + hs.soundcard.active_output_name, r.font_mono_small,
                D2D1::RectF(mr_box.left + 8, y3 + 23, mr_box.right - 8, y3 + 41), r.hex_color(0x34D399));
    r.draw_text("RADIO 3 (LORA):   " + app.transport_label, r.font_mono_small,
                D2D1::RectF(mr_box.left + 8, y3 + 42, mr_box.right - 8, y3 + 60), r.hex_color(0xC084FC));
    r.draw_text("XML-RPC SERVER:   http://127.0.0.1:" + std::to_string(hs.flrig.xmlrpc_port) + "/RPC2", r.font_mono_small,
                D2D1::RectF(mr_box.left + 8, y3 + 61, mr_box.right - 8, y3 + 80), r.hex_color(0x38BDF8));
    y3 += 88.0f;

    float bw_p = (col_w - 28.0f) / 3.0f;
    draw_hs_btn(r, hs, mouse_pt, x3 + 10,               y3, bw_p, 26, HS_CAT_POLL_VFO,    "Poll VFO (0x03)",    false, 0x00E5FF, 0, "", true);
    draw_hs_btn(r, hs, mouse_pt, x3 + 14 + bw_p,        y3, bw_p, 26, HS_CAT_POLL_SMETER, "Poll S-Meter (0x15)",false, 0x10B981, 0, "", true);
    draw_hs_btn(r, hs, mouse_pt, x3 + 18 + bw_p * 2.0f, y3, bw_p, 26, HS_CAT_CLEAR_LOG,   "Clear Hex Log",      false, 0x94A3B8, 0, "", true);
    y3 += 32.0f;

    D2D1_RECT_F hex_box = D2D1::RectF(x3 + 10, y3, x3 + col_w - 92, y3 + 28);
    bool hex_foc = (hs.text_focus == HsTextFocus::CatHexInput);
    r.fill_round_rect(hex_box, 5.0f, r.hex_color(0x05080F));
    r.draw_round_rect(hex_box, 5.0f, hex_foc ? r.hex_color(0x00E5FF) : r.hex_color(0x334155), 1.2f);
    r.draw_text(hs.custom_cat_hex + (hex_foc ? "_" : ""), r.font_mono_small,
                D2D1::RectF(hex_box.left + 8, hex_box.top, hex_box.right - 8, hex_box.bottom), r.hex_color(0xF8FAFC));
    hs.add_hit(hex_box, HS_CAT_HEX_BOX);
    draw_hs_btn(r, hs, mouse_pt, x3 + col_w - 86, y3, 76, 28, HS_CAT_HEX_SEND, "Send CI-V", false, 0xFBBF24, 0, "", true);
    y3 += 34.0f;

    D2D1_RECT_F log_box = D2D1::RectF(x3 + 10, y3, x3 + col_w - 10, body_y + body_h - 8);
    r.fill_round_rect(log_box, 6.0f, r.hex_color(0x05080F));
    r.draw_round_rect(log_box, 6.0f, r.hex_color(0x1E293B), 1.0f);
    {
        std::lock_guard<std::mutex> lk(hs.flrig.mtx);
        float ly = log_box.top + 6.0f;
        float lh = 19.0f;
        int max_l = std::max(1, static_cast<int>((log_box.bottom - log_box.top - 10.0f) / lh));
        int start_i = std::max(0, static_cast<int>(hs.flrig.cat_traces.size()) - max_l);
        for (int i = start_i; i < static_cast<int>(hs.flrig.cat_traces.size()); ++i) {
            const auto& tr = hs.flrig.cat_traces[i];
            std::string line = tr.time_str + (tr.tx ? " TX " : " RX ") + tr.hex_str + "  [" + tr.summary + "]";
            r.draw_text(line, r.font_mono_small, D2D1::RectF(log_box.left + 6, ly, log_box.right - 6, ly + lh),
                        tr.tx ? r.hex_color(0x38BDF8) : r.hex_color(0x34D399));
            ly += lh;
        }
    }
}

// ============================================================================
// Modal Dialog: WSJT-X Style "Log QSO" Popup
// ============================================================================
template <typename RendererT>
inline void render_log_qso_modal(RendererT& r, HamShackSuite& hs, POINT mouse_pt, float W, float H) {
    if (!hs.log_qso_dialog.is_open) return;

    // Dark semi-transparent scrim over the entire window
    D2D1_RECT_F scrim = D2D1::RectF(0, 0, W, H);
    r.fill_round_rect(scrim, 0.0f, r.hex_color(0x020617, 0.75f));

    // Centered modal dialog card
    float mw = 490.0f;
    float mh = 390.0f;
    float mx = std::max(10.0f, (W - mw) * 0.5f);
    float my = std::max(10.0f, (H - mh) * 0.5f);
    D2D1_RECT_F card = D2D1::RectF(mx, my, mx + mw, my + mh);

    // Dialog Card Background & Glowing Cyan Border
    r.fill_round_rect(card, 10.0f, r.hex_color(0x0F172A));
    r.draw_round_rect(card, 10.0f, r.hex_color(0x38BDF8), 1.6f);

    // Title Bar
    D2D1_RECT_F hdr = D2D1::RectF(mx, my, mx + mw, my + 38.0f);
    r.fill_round_rect(hdr, 10.0f, r.hex_color(0x1E293B));
    r.draw_text("LOG QSO \xe2\x80\x94 WSJT-X LOGGING DIALOG", r.font_ui_bold,
                D2D1::RectF(mx + 16, my + 9, mx + mw - 50, my + 31), r.hex_color(0x00E5FF));

    // Close button (X) in top right
    D2D1_RECT_F close_btn = D2D1::RectF(mx + mw - 34.0f, my + 7.0f, mx + mw - 10.0f, my + 31.0f);
    bool c_hov = RendererT::pt_in_rect(mouse_pt, close_btn);
    r.fill_round_rect(close_btn, 4.0f, c_hov ? r.hex_color(0xEF4444) : r.hex_color(0x334155));
    r.draw_text("\xe2\x9c\x95", r.font_ui_bold, close_btn, r.hex_color(0xFFFFFF), DWRITE_TEXT_ALIGNMENT_CENTER);
    hs.add_hit(close_btn, HS_LOG_QSO_CANCEL);

    // Subtitle with timestamp & operator call/grid
    float cy = my + 44.0f;
    std::string sub = "UTC: " + hs.log_qso_dialog.date_str + " " + hs.log_qso_dialog.time_str +
                      " \xe2\x80\xa2 Operator: " + hs.ft8.my_call + " (" + hs.ft8.my_grid + ")";
    r.draw_text(sub, r.font_mono_small, D2D1::RectF(mx + 16, cy, mx + mw - 16, cy + 18), r.hex_color(0x94A3B8));
    cy += 24.0f;

    auto draw_labeled_field = [&](const std::string& label, const std::string& val, float fx, float fy, float fw,
                                  int hit_id, HsTextFocus foc_type) {
        r.draw_text(label, r.font_mono_small, D2D1::RectF(fx, fy, fx + fw, fy + 16), r.hex_color(0xCBD5E1));
        D2D1_RECT_F box = D2D1::RectF(fx, fy + 18.0f, fx + fw, fy + 44.0f);
        bool focused = (hs.text_focus == foc_type);
        r.fill_round_rect(box, 4.0f, r.hex_color(0x05080F));
        r.draw_round_rect(box, 4.0f, focused ? r.hex_color(0x00E5FF) : r.hex_color(0x334155), focused ? 1.6f : 1.0f);
        std::string disp = val + (focused ? "_" : "");
        r.draw_text(disp, r.font_mono_small, D2D1::RectF(box.left + 8, box.top + 4, box.right - 8, box.bottom - 4),
                    focused ? r.hex_color(0x38BDF8) : r.hex_color(0xF8FAFC));
        hs.add_hit(box, hit_id);
    };

    float half_w = (mw - 44.0f) * 0.5f;

    // Row 1: DX Callsign & Grid Locator
    draw_labeled_field("DX CALLSIGN", hs.log_qso_dialog.call, mx + 16, cy, half_w,
                       HS_LOG_QSO_CALL, HsTextFocus::LogQsoCall);
    draw_labeled_field("DX GRID LOCATOR", hs.log_qso_dialog.grid, mx + 28 + half_w, cy, half_w,
                       HS_LOG_QSO_GRID, HsTextFocus::LogQsoGrid);
    cy += 50.0f;

    // Row 2: Mode & Frequency (MHz)
    draw_labeled_field("MODE (FT8 / FT4 / JS8)", hs.log_qso_dialog.mode, mx + 16, cy, half_w,
                       HS_LOG_QSO_MODE, HsTextFocus::LogQsoMode);
    draw_labeled_field("FREQUENCY (MHz)", hs.log_qso_dialog.freq_str, mx + 28 + half_w, cy, half_w,
                       HS_LOG_QSO_FREQ, HsTextFocus::LogQsoFreq);
    cy += 50.0f;

    // Row 3: Report Sent & Report Received
    draw_labeled_field("RST SENT (dB)", hs.log_qso_dialog.rpt_sent, mx + 16, cy, half_w,
                       HS_LOG_QSO_RPT_S, HsTextFocus::LogQsoRptS);
    draw_labeled_field("RST RCVD (dB)", hs.log_qso_dialog.rpt_rcvd, mx + 28 + half_w, cy, half_w,
                       HS_LOG_QSO_RPT_R, HsTextFocus::LogQsoRptR);
    cy += 50.0f;

    // Row 4: Tx Power (W) & Band
    draw_labeled_field("TX POWER (Watts)", hs.log_qso_dialog.power_w, mx + 16, cy, half_w,
                       HS_LOG_QSO_PWR, HsTextFocus::LogQsoPwr);
    // Band display
    r.draw_text("BAND", r.font_mono_small, D2D1::RectF(mx + 28 + half_w, cy, mx + mw - 16, cy + 16), r.hex_color(0xCBD5E1));
    D2D1_RECT_F band_box = D2D1::RectF(mx + 28 + half_w, cy + 18.0f, mx + mw - 16, cy + 44.0f);
    r.fill_round_rect(band_box, 4.0f, r.hex_color(0x05080F));
    r.draw_round_rect(band_box, 4.0f, r.hex_color(0x334155), 1.0f);
    std::string b_disp = hs.log_qso_dialog.band_str.empty() ? "20m" : hs.log_qso_dialog.band_str;
    r.draw_text(b_disp, r.font_mono_small, D2D1::RectF(band_box.left + 8, band_box.top + 4, band_box.right - 8, band_box.bottom - 4), r.hex_color(0xFBBF24));
    cy += 50.0f;

    // Row 5: Comments / Notes
    draw_labeled_field("COMMENTS / NOTES (ADIF <comment>)", hs.log_qso_dialog.comments, mx + 16, cy, mw - 32.0f,
                       HS_LOG_QSO_COMM, HsTextFocus::LogQsoComm);
    cy += 52.0f;

    // Footer actions
    r.draw_text("Tab = Next \xe2\x80\xa2 Enter = Save QSO \xe2\x80\xa2 Esc = Cancel",
                r.font_mono_small, D2D1::RectF(mx + 16, cy + 8, mx + mw - 250, cy + 32), r.hex_color(0x64748B));

    float btn_w1 = 100.0f;
    float btn_w2 = 135.0f;
    draw_hs_btn(r, hs, mouse_pt, mx + mw - 16.0f - btn_w1 - 10.0f - btn_w2, cy, btn_w1, 32,
                HS_LOG_QSO_CANCEL, "Cancel", false, 0x64748B, 0, "", true);
    draw_hs_btn(r, hs, mouse_pt, mx + mw - 16.0f - btn_w2, cy, btn_w2, 32,
                HS_LOG_QSO_OK, "\xe2\x9c\x93 Save QSO", true, 0x10B981, 0, "", true);
}

// ============================================================================
// Shared Carbon Engine Audio Source Dropdown Widget (Source A: G90 vs Source B: USB Dongle)
// ============================================================================
template <typename RendererT>
inline void render_carbon_audio_source_bar(RendererT& r, HamShackSuite& hs, POINT mouse_pt,
                                           float x, float y, float drop_w) {
    const auto active_src = hs.audio_router
        ? hs.audio_router->active_source()
        : hamshack::carbon::AudioSource::PrimaryG90;
    const bool is_dongle = (active_src == hamshack::carbon::AudioSource::UsbDongleVhf);

    std::string drop_lbl = is_dongle
        ? "\xf0\x9f\x8e\xa7 Audio: Source B — USB Dongle (UV-5R / BTECH) \xe2\x96\xbe"
        : "\xf0\x9f\x93\xbb Audio: Source A — Xiegu G90 HF Stream \xe2\x96\xbe";
    D2D1_RECT_F drop_rc = D2D1::RectF(x, y, x + drop_w, y + 26.0f);
    hs.audio_source_dropdown_anchor = drop_rc;
    draw_hs_btn(r, hs, mouse_pt, x, y, drop_w, 26.0f, HS_AUDIO_SRC_DROPDOWN,
                drop_lbl, hs.audio_source_dropdown_open || is_dongle,
                is_dongle ? 0x22D3EE : 0x00E5FF, 0, "", true);

    if (hs.audio_router) {
        const auto rt_tel = hs.audio_router->telemetry();
        std::string dev_name(hs.audio_router->selected_dongle_name());
        if (dev_name.size() > 22) dev_name = dev_name.substr(0, 22) + "..";
        std::string hw_lbl = "USB HW: " + dev_name;
        draw_hs_btn(r, hs, mouse_pt, x + drop_w + 5.0f, y, 185.0f, 26.0f, HS_AUDIO_DONGLE_CYCLE,
                    hw_lbl, is_dongle && rt_tel.dongle_stream_active, 0x10B981, 0, "", true);
    }
}

template <typename RendererT>
inline void render_carbon_audio_dropdown_overlay(RendererT& r, HamShackSuite& hs, POINT mouse_pt) {
    if (!hs.audio_source_dropdown_open || !hs.audio_router) return;
    const float mx = hs.audio_source_dropdown_anchor.left;
    const float my = hs.audio_source_dropdown_anchor.bottom + 3.0f;
    const float mw = std::max(380.0f, hs.audio_source_dropdown_anchor.right - hs.audio_source_dropdown_anchor.left + 80.0f);
    const float mh = 90.0f;

    D2D1_RECT_F menu = D2D1::RectF(mx, my, mx + mw, my + mh);
    r.fill_round_rect(menu, 7.0f, r.hex_color(0x0B111E, 0.98f));
    r.draw_round_rect(menu, 7.0f, r.hex_color(0x00E5FF), 1.6f);
    r.draw_text("SELECT ATOMIC LOCK-FREE AUDIO ROUTER STREAM", r.font_small,
                D2D1::RectF(mx + 10.0f, my + 4.0f, mx + mw - 10.0f, my + 22.0f), r.hex_color(0x38BDF8));

    const auto cur = hs.audio_router->active_source();
    draw_hs_btn(r, hs, mouse_pt, mx + 8.0f, my + 25.0f, mw - 16.0f, 27.0f, HS_AUDIO_SRC_OPT_G90,
                "Source A: Xiegu G90 Primary HF Stream (Zero-Copy Shared Tap + CAT PTT)",
                cur == hamshack::carbon::AudioSource::PrimaryG90, 0x00E5FF, 0, "", true);
    draw_hs_btn(r, hs, mouse_pt, mx + 8.0f, my + 56.0f, mw - 16.0f, 27.0f, HS_AUDIO_SRC_OPT_DONGLE,
                "Source B: Dedicated USB Audio Dongle (BTECH APRS Cable / UV-5R VOX)",
                cur == hamshack::carbon::AudioSource::UsbDongleVhf, 0x22D3EE, 0, "", true);
}

// ============================================================================
// TAB 9: APRS — AX.25 1200-Baud KISS-over-TCP (Dire Wolf), OSM Map & Messaging
// ============================================================================
template <typename RendererT, typename AppT>
inline void render_aprs_workspace(RendererT& r, AppT& /*app*/, HamShackSuite& hs, POINT mouse_pt,
                                  float x, float y, float w, float h) {
    if (!hs.aprs) return;

    // 1. Top Control Strip (AudioRouter Dropdown + Dire Wolf KISS TCP + Beacon TX)
    const float bar_h = 34.0f;
    D2D1_RECT_F bar_r = D2D1::RectF(x, y, x + w, y + bar_h);
    r.fill_round_rect(bar_r, 6.0f, r.hex_color(0x0A0F1D));
    r.draw_round_rect(bar_r, 6.0f, r.hex_color(0x1E293B), 1.0f);

    float cx = x + 6.0f;
    render_carbon_audio_source_bar(r, hs, mouse_pt, cx, y + 4.0f, 285.0f);
    cx += 285.0f + 5.0f + 185.0f + 10.0f;

    auto draw_aprs_field = [&](float fx, float fw, const std::string& prefix, const std::string& val,
                               HsTextFocus foc, int hit_id) {
        D2D1_RECT_F rc = D2D1::RectF(fx, y + 4.0f, fx + fw, y + 30.0f);
        bool active = (hs.text_focus == foc);
        r.fill_round_rect(rc, 4.0f, r.hex_color(active ? 0x16243D : 0x05080F));
        r.draw_round_rect(rc, 4.0f, r.hex_color(active ? 0x00E5FF : 0x334155), active ? 1.5f : 1.0f);
        std::string txt = prefix + val + (active ? "|" : "");
        r.draw_text(txt, r.font_mono_small, D2D1::RectF(rc.left + 6.0f, rc.top + 2.0f, rc.right - 4.0f, rc.bottom - 2.0f),
                    r.hex_color(active ? 0x00E5FF : 0xE2E8F0));
        hs.add_hit(rc, hit_id);
    };

    draw_aprs_field(cx, 152.0f, "KISS: ", hs.aprs_host_edit, HsTextFocus::AprsHost, HS_APRS_HOST_BOX);
    cx += 156.0f;
    draw_aprs_field(cx, 78.0f, ":", hs.aprs_port_edit, HsTextFocus::AprsPort, HS_APRS_PORT_BOX);
    cx += 82.0f;

    draw_hs_btn(r, hs, mouse_pt, cx, y + 4.0f, 68.0f, 26.0f, HS_APRS_SSID_CYCLE,
                "-" + std::to_string(hs.aprs_ssid), false, 0xFBBF24, 0, "", true);
    cx += 72.0f;

    if (w > 1280.0f) {
        draw_aprs_field(cx, 158.0f, "Path: ", hs.aprs_path_edit, HsTextFocus::AprsPath, HS_APRS_PATH_BOX);
        cx += 162.0f;
    }

    const bool kiss_conn = hs.aprs->is_connected();
    const bool kiss_active = hs.aprs->is_running();
    std::string conn_lbl = kiss_conn ? "\xe2\x97\x8f KISS ONLINE"
                         : kiss_active ? "\xe2\x97\x8b CONNECTING.."
                                       : "Connect KISS";
    draw_hs_btn(r, hs, mouse_pt, cx, y + 4.0f, 112.0f, 26.0f, HS_APRS_CONNECT_TOG,
                conn_lbl, kiss_active, kiss_conn ? 0x10B981 : 0x38BDF8, 0, "", true);
    cx += 116.0f;

    draw_hs_btn(r, hs, mouse_pt, cx, y + 4.0f, 98.0f, 26.0f, HS_APRS_BEACON_TX,
                "\xf0\x9f\x93\xa1 Beacon TX", false, 0xF59E0B, 0, "", true);
    cx += 102.0f;

    if (cx + 58.0f <= x + w - 4.0f) {
        draw_hs_btn(r, hs, mouse_pt, x + w - 62.0f, y + 4.0f, 56.0f, 26.0f, HS_APRS_CLEAR_BTN,
                    "Clear", false, 0xEF4444, 0, "", true);
    }

    // 2. Main Split Workspace: Left (Map + AX.25 Packet Log) | Right (Heard Stations + 2-Way APRS Messaging)
    const float body_y = y + bar_h + 6.0f;
    const float body_h = std::max(120.0f, h - bar_h - 6.0f);
    const float right_w = std::clamp(w * 0.34f, 340.0f, 470.0f);
    const float left_w = std::max(320.0f, w - right_w - 8.0f);
    const float log_h = std::clamp(body_h * 0.33f, 145.0f, 240.0f);
    const float map_h = std::max(140.0f, body_h - log_h - 6.0f);

    // ---- 2A. Interactive APRS OpenStreetMap Surface ----
    D2D1_RECT_F map_r = D2D1::RectF(x, body_y, x + left_w, body_y + map_h);
    hs.gt_map_rect = map_r;
    hs.add_hit(map_r, HS_GT_MAP_CANVAS);

    r.fill_round_rect(map_r, 8.0f, r.hex_color(0x0A0F1D));
    r.draw_round_rect(map_r, 8.0f, r.hex_color(0x1E293B), 1.2f);
    r.rt->PushAxisAlignedClip(map_r, D2D1_ANTIALIAS_MODE_ALIASED);

    const int z = hs.gt_map_zoom;
    const int n_tiles = 1 << z;
    const double ws = 256.0 * n_tiles;
    const double mcx = (map_r.left + map_r.right) * 0.5;
    const double mcy = (map_r.top + map_r.bottom) * 0.5;
    const double cam_x = hs.gt_map_nx * ws;
    const double cam_y = hs.gt_map_ny * ws;

    auto to_screen = [&](double nx, double ny) -> D2D1_POINT_2F {
        double dx = nx - hs.gt_map_nx;
        if (dx > 0.5) dx -= 1.0;
        else if (dx < -0.5) dx += 1.0;
        return D2D1::Point2F(static_cast<float>(mcx + dx * ws),
                             static_cast<float>(mcy + (ny - hs.gt_map_ny) * ws));
    };
    auto geo_pt = [&](double lat, double lon) -> D2D1_POINT_2F {
        return to_screen(lon_to_nx(lon), lat_to_ny(lat));
    };

    int tx0 = static_cast<int>(std::floor((cam_x - (mcx - map_r.left)) / 256.0));
    int tx1 = static_cast<int>(std::floor((cam_x + (map_r.right - mcx)) / 256.0));
    int ty0 = std::max(0, static_cast<int>(std::floor((cam_y - (mcy - map_r.top)) / 256.0)));
    int ty1 = std::min(n_tiles - 1, static_cast<int>(std::floor((cam_y + (map_r.bottom - mcy)) / 256.0)));

    for (int ty = ty0; ty <= ty1; ++ty) {
        for (int tx = tx0; tx <= tx1; ++tx) {
            int wx_tile = ((tx % n_tiles) + n_tiles) % n_tiles;
            float sx = static_cast<float>(mcx + tx * 256.0 - cam_x);
            float sy = static_cast<float>(mcy + ty * 256.0 - cam_y);
            D2D1_RECT_F dst = D2D1::RectF(sx, sy, sx + 256.0f, sy + 256.0f);
            if (ID2D1Bitmap* bmp = r.get_tile(tile_key(0, z, wx_tile, ty))) {
                r.rt->DrawBitmap(bmp, dst, 1.0f, D2D1_BITMAP_INTERPOLATION_MODE_LINEAR);
                continue;
            }
            g_tiles.request(0, z, wx_tile, ty);
            for (int up = 1; up <= 6 && z - up >= 0; ++up) {
                auto it = r.tile_bitmaps.find(tile_key(0, z - up, wx_tile >> up, ty >> up));
                if (it == r.tile_bitmaps.end()) continue;
                it->second.last_used = r.frame_no;
                float sub = 256.0f / static_cast<float>(1 << up);
                float ox = static_cast<float>(wx_tile & ((1 << up) - 1)) * sub;
                float oy = static_cast<float>(ty & ((1 << up) - 1)) * sub;
                D2D1_RECT_F src = D2D1::RectF(ox, oy, ox + sub, oy + sub);
                r.rt->DrawBitmap(it->second.bmp, dst, 1.0f, D2D1_BITMAP_INTERPOLATION_MODE_LINEAR, &src);
                break;
            }
        }
    }
    if (!r.light_mode) {
        r.fill_rect(map_r, r.hex_color(0x0B1220, 0.50f));
    }

    // Home Station QTH Pin
    double qlat = 0.0, qlon = 0.0;
    if (hamshack::Ft8Ft4Engine::grid_to_latlon(hs.hf_grid, qlat, qlon)) {
        D2D1_POINT_2F qpt = geo_pt(qlat, qlon);
        r.fill_round_rect(D2D1::RectF(qpt.x - 5.0f, qpt.y - 5.0f, qpt.x + 5.0f, qpt.y + 5.0f), 5.0f, r.hex_color(0xF97316));
        std::string my_aprs = hs.hf_callsign + (hs.aprs_ssid > 0 ? ("-" + std::to_string(hs.aprs_ssid)) : "");
        r.draw_text("\xe2\x98\x85 " + my_aprs + " [QTH]", r.font_ui_bold,
                    D2D1::RectF(qpt.x + 8.0f, qpt.y - 10.0f, qpt.x + 190.0f, qpt.y + 10.0f), r.hex_color(0xFBBF24));
    }

    // Decoded APRS GPS Station Pins + Heading Vector
    const auto& aprs_stations = hs.aprs->stations();
    const auto& aprs_packets  = hs.aprs->packet_log();
    const auto& aprs_chats    = hs.aprs->chat_log();
    size_t gps_count = 0;
    for (const auto& st_rec : aprs_stations) {
        if (!st_rec.has_position) continue;
        ++gps_count;
        D2D1_POINT_2F pt = geo_pt(st_rec.latitude, st_rec.longitude);
        if (pt.x < map_r.left - 30.0f || pt.x > map_r.right + 30.0f ||
            pt.y < map_r.top - 30.0f || pt.y > map_r.bottom + 30.0f) {
            continue;
        }
        bool is_sel = (!hs.aprs_selected_call.empty() && hs.aprs_selected_call == st_rec.callsign_ssid);
        uint32_t pcol = is_sel ? 0xFBBF24 : 0x22D3EE;
        if (st_rec.speed_knots >= 1.0f) {
            float rad = st_rec.course_deg * 3.14159265f / 180.0f;
            D2D1_POINT_2F tip = D2D1::Point2F(pt.x + std::sin(rad) * 16.0f, pt.y - std::cos(rad) * 16.0f);
            r.draw_line(pt, tip, r.hex_color(0x10B981, 0.9f), 2.0f);
        }
        r.fill_round_rect(D2D1::RectF(pt.x - 5.5f, pt.y - 5.5f, pt.x + 5.5f, pt.y + 5.5f), 5.5f, r.hex_color(pcol));
        r.draw_round_rect(D2D1::RectF(pt.x - 6.5f, pt.y - 6.5f, pt.x + 6.5f, pt.y + 6.5f), 6.5f, r.hex_color(0x020617), 1.3f);

        char tag[96];
        if (st_rec.altitude_ft > 1.0f && st_rec.speed_knots > 0.5f) {
            std::snprintf(tag, sizeof(tag), "%s (%.0fkt %.0fft)",
                          st_rec.callsign_ssid.c_str(), st_rec.speed_knots, st_rec.altitude_ft);
        } else if (st_rec.altitude_ft > 1.0f) {
            std::snprintf(tag, sizeof(tag), "%s (%.0fft)", st_rec.callsign_ssid.c_str(), st_rec.altitude_ft);
        } else {
            std::snprintf(tag, sizeof(tag), "%s [%c]", st_rec.callsign_ssid.c_str(), st_rec.symbol_code);
        }
        D2D1_RECT_F badge = D2D1::RectF(pt.x + 8.0f, pt.y - 9.0f,
                                        pt.x + 8.0f + std::min(190.0f, static_cast<float>(std::strlen(tag) * 7 + 10)),
                                        pt.y + 9.0f);
        r.fill_round_rect(badge, 3.0f, r.hex_color(0x05080F, 0.86f));
        r.draw_round_rect(badge, 3.0f, r.hex_color(pcol, 0.75f), 1.0f);
        r.draw_text(tag, r.font_mono_small, D2D1::RectF(badge.left + 4.0f, badge.top, badge.right - 2.0f, badge.bottom), r.hex_color(pcol));
    }
    r.rt->PopAxisAlignedClip();

    // Floating Map Controls & KISS Status Pill
    D2D1_RECT_F kiss_pill = D2D1::RectF(map_r.left + 8.0f, map_r.top + 8.0f,
                                        std::min(map_r.right - 220.0f, map_r.left + 490.0f), map_r.top + 32.0f);
    r.fill_round_rect(kiss_pill, 5.0f, r.hex_color(0x05080F, 0.88f));
    r.draw_round_rect(kiss_pill, 5.0f, r.hex_color(kiss_conn ? 0x10B981 : 0x334155), 1.0f);
    char kstat[192];
    std::snprintf(kstat, sizeof(kstat), "RX:%llu  TX:%llu  GPS:%llu  |  %s",
                  static_cast<unsigned long long>(hs.aprs->rx_frame_count()),
                  static_cast<unsigned long long>(hs.aprs->tx_frame_count()),
                  static_cast<unsigned long long>(gps_count),
                  hs.aprs_status_banner.c_str());
    r.draw_text(kstat, r.font_mono_small,
                D2D1::RectF(kiss_pill.left + 8.0f, kiss_pill.top + 2.0f, kiss_pill.right - 6.0f, kiss_pill.bottom - 2.0f),
                r.hex_color(kiss_conn ? 0x34D399 : 0x94A3B8));

    float fbx = map_r.right - 210.0f;
    float fby = map_r.top + 8.0f;
    draw_hs_btn(r, hs, mouse_pt, fbx + 0*48, fby, 44, 24, HS_GT_ZOOM_IN,    "[+]",   false, 0x00E5FF, 0, "", true);
    draw_hs_btn(r, hs, mouse_pt, fbx + 1*48, fby, 44, 24, HS_GT_ZOOM_OUT,   "[-]",   false, 0x00E5FF, 0, "", true);
    draw_hs_btn(r, hs, mouse_pt, fbx + 2*48, fby, 54, 24, HS_GT_CENTER_QTH, "QTH",   false, 0xF97316, 0, "", true);
    draw_hs_btn(r, hs, mouse_pt, fbx + 3*50, fby, 56, 24, HS_GT_FIT_WORLD,  "World", false, 0x38BDF8, 0, "", true);

    // ---- 2B. Real-Time Scrolling AX.25 Packet Log ----
    const float log_y = body_y + map_h + 6.0f;
    D2D1_RECT_F log_card = D2D1::RectF(x, log_y, x + left_w, log_y + log_h);
    r.fill_round_rect(log_card, 8.0f, r.hex_color(0x05080F));
    r.draw_round_rect(log_card, 8.0f, r.hex_color(0x1E293B), 1.2f);

    D2D1_RECT_F lhdr = D2D1::RectF(x + 4.0f, log_y + 4.0f, x + left_w - 4.0f, log_y + 25.0f);
    r.fill_round_rect(lhdr, 4.0f, r.hex_color(0x0F172A));
    r.draw_text("REAL-TIME AX.25 1200-BAUD TNC2 PACKET STREAM  (KISS Deframer -> AX.25 UI -> APRS 1.01)",
                r.font_ui_bold, D2D1::RectF(x + 10.0f, log_y + 5.0f, x + left_w - 220.0f, log_y + 24.0f),
                r.hex_color(0x22D3EE));

    // Beacon Comment inline box on header right
    D2D1_RECT_F cmt_rc = D2D1::RectF(x + left_w - 225.0f, log_y + 4.0f, x + left_w - 6.0f, log_y + 25.0f);
    bool cmt_act = (hs.text_focus == HsTextFocus::AprsComment);
    r.fill_round_rect(cmt_rc, 3.0f, r.hex_color(cmt_act ? 0x16243D : 0x090D16));
    r.draw_round_rect(cmt_rc, 3.0f, r.hex_color(cmt_act ? 0x00E5FF : 0x334155), 1.0f);
    r.draw_text("Cmt: " + hs.aprs_comment_edit + (cmt_act ? "|" : ""), r.font_mono_small,
                D2D1::RectF(cmt_rc.left + 5.0f, cmt_rc.top + 1.0f, cmt_rc.right - 4.0f, cmt_rc.bottom - 1.0f),
                r.hex_color(0xCBD5E1));
    hs.add_hit(cmt_rc, HS_APRS_COMMENT_BOX);

    float pry = log_y + 28.0f;
    const float pr_h = 18.0f;
    if (aprs_packets.empty()) {
        r.draw_text("Waiting for AX.25 UI frames on KISS TCP (" + hs.aprs_host_edit + ":" + hs.aprs_port_edit + ")...",
                    r.font_mono_small, D2D1::RectF(x + 12.0f, pry + 8.0f, x + left_w - 12.0f, pry + 30.0f),
                    r.hex_color(0x64748B));
    } else {
        for (int i = static_cast<int>(aprs_packets.size()) - 1; i >= 0 && pry + pr_h <= log_card.bottom - 4.0f; --i) {
            const auto& pkt = aprs_packets[static_cast<size_t>(i)];
            const bool is_msg = (pkt.kind == hamshack::carbon::AprsPacketKind::TextMessage ||
                                 pkt.kind == hamshack::carbon::AprsPacketKind::MessageAck ||
                                 pkt.kind == hamshack::carbon::AprsPacketKind::MessageRej);
            uint32_t col = pkt.is_tx ? 0xFBBF24 : (is_msg ? 0xF472B6 : (pkt.has_position ? 0x34D399 : 0xCBD5E1));
            std::string line = "[" + std::string(pkt.utc_hms.data()) + "] " + std::string(pkt.tnc2_raw.data());
            r.draw_text(line, r.font_mono_small,
                        D2D1::RectF(x + 10.0f, pry, x + left_w - 10.0f, pry + pr_h),
                        r.hex_color(col));
            pry += pr_h;
        }
    }

    // ---- 2C. Right Column: Heard Stations Roster (Top 44%) + 2-Way APRS Text Messaging (Bottom 56%) ----
    const float rx_x = x + left_w + 8.0f;
    const float st_h = std::round(body_h * 0.44f);
    const float msg_y = body_y + st_h + 6.0f;
    const float msg_h = body_h - st_h - 6.0f;

    // Heard APRS Stations Card
    D2D1_RECT_F st_card = D2D1::RectF(rx_x, body_y, rx_x + right_w, body_y + st_h);
    r.fill_round_rect(st_card, 8.0f, r.hex_color(0x0F172A));
    r.draw_round_rect(st_card, 8.0f, r.hex_color(0x1E293B), 1.2f);
    r.draw_text("HEARD APRS GPS STATIONS  (Click to Pan & Message)", r.font_ui_bold,
                D2D1::RectF(rx_x + 10.0f, body_y + 5.0f, rx_x + right_w - 10.0f, body_y + 23.0f),
                r.hex_color(0x38BDF8));

    D2D1_RECT_F shdr = D2D1::RectF(rx_x + 6.0f, body_y + 25.0f, rx_x + right_w - 6.0f, body_y + 42.0f);
    r.fill_round_rect(shdr, 3.0f, r.hex_color(0x05080F));
    r.draw_text("CALLSIGN    LAT / LON          ALT/SPD   COMMENT", r.font_mono_small,
                D2D1::RectF(rx_x + 10.0f, body_y + 26.0f, rx_x + right_w - 10.0f, body_y + 41.0f),
                r.hex_color(0x94A3B8));

    float sry = body_y + 44.0f;
    const float sr_h = 21.0f;
    if (aprs_stations.empty()) {
        r.draw_text("No APRS stations decoded yet.", r.font_mono_small,
                    D2D1::RectF(rx_x + 12.0f, sry + 10.0f, rx_x + right_w - 12.0f, sry + 30.0f),
                    r.hex_color(0x64748B));
    } else {
        for (int i = 0; i < static_cast<int>(aprs_stations.size()) && sry + sr_h <= st_card.bottom - 4.0f; ++i) {
            const auto& s = aprs_stations[static_cast<size_t>(i)];
            D2D1_RECT_F rr = D2D1::RectF(rx_x + 6.0f, sry, rx_x + right_w - 6.0f, sry + sr_h - 1.0f);
            bool sel = (hs.aprs_selected_call == s.callsign_ssid);
            bool hov = RendererT::pt_in_rect(mouse_pt, rr);
            if (sel || hov) {
                r.fill_round_rect(rr, 4.0f, r.hex_color(sel ? 0x16243D : 0x1E293B));
            }
            char row_txt[160];
            char kin_txt[24] = "-";
            if (s.altitude_ft > 1.0f) std::snprintf(kin_txt, sizeof(kin_txt), "%.0fft", s.altitude_ft);
            else if (s.speed_knots > 0.5f) std::snprintf(kin_txt, sizeof(kin_txt), "%.0fkt", s.speed_knots);
            if (s.has_position) {
                std::snprintf(row_txt, sizeof(row_txt), "%-10s %+7.3f %+8.3f  %-8s  %s",
                              s.callsign_ssid.c_str(), s.latitude, s.longitude, kin_txt, s.last_comment.c_str());
            } else {
                std::snprintf(row_txt, sizeof(row_txt), "%-10s    ---     ---    %-8s  %s",
                              s.callsign_ssid.c_str(), kin_txt, s.last_comment.c_str());
            }
            r.draw_text(row_txt, r.font_mono_small,
                        D2D1::RectF(rr.left + 4.0f, rr.top + 1.0f, rr.right - 4.0f, rr.bottom - 1.0f),
                        r.hex_color(sel ? 0x00E5FF : 0xE2E8F0));
            hs.add_hit(rr, HS_APRS_STATION_ROW, i, s.callsign_ssid);
            sry += sr_h;
        }
    }

    // 2-Way APRS Text Messaging Sub-Panel
    D2D1_RECT_F msg_card = D2D1::RectF(rx_x, msg_y, rx_x + right_w, msg_y + msg_h);
    r.fill_round_rect(msg_card, 8.0f, r.hex_color(0x0F172A));
    r.draw_round_rect(msg_card, 8.0f, r.hex_color(0x1E293B), 1.2f);
    r.draw_text("APRS 2-WAY TEXT MESSAGING  (:ADDRESSEE:msg{id + Auto-ACK)", r.font_ui_bold,
                D2D1::RectF(rx_x + 10.0f, msg_y + 5.0f, rx_x + right_w - 10.0f, msg_y + 23.0f),
                r.hex_color(0xF472B6));

    const float comp_h = 62.0f;
    D2D1_RECT_F hist_r = D2D1::RectF(rx_x + 6.0f, msg_y + 26.0f, rx_x + right_w - 6.0f, msg_y + msg_h - comp_h - 6.0f);
    r.fill_round_rect(hist_r, 5.0f, r.hex_color(0x05080F));
    r.draw_round_rect(hist_r, 5.0f, r.hex_color(0x1E293B), 1.0f);

    float mry = hist_r.top + 6.0f;
    const float mr_h = 20.0f;
    int max_m = std::max(1, static_cast<int>((hist_r.bottom - hist_r.top - 10.0f) / mr_h));
    int start_m = std::max(0, static_cast<int>(aprs_chats.size()) - max_m);
    if (aprs_chats.empty()) {
        r.draw_text("Direct APRS messages & ACK confirmations appear here.", r.font_mono_small,
                    D2D1::RectF(hist_r.left + 8.0f, mry + 6.0f, hist_r.right - 8.0f, mry + 26.0f),
                    r.hex_color(0x64748B));
    } else {
        for (int i = start_m; i < static_cast<int>(aprs_chats.size()) && mry + mr_h <= hist_r.bottom - 2.0f; ++i) {
            const auto& m = aprs_chats[static_cast<size_t>(i)];
            std::string ack_tag;
            if (m.outbound && !m.msg_id.empty()) {
                ack_tag = m.acked ? " [ACK \xe2\x9c\x93]" : (m.rejected ? " [REJ \xe2\x9c\x97]" : " [WAIT]");
            }
            const std::string& dst_call = m.outbound ? m.peer_call : hs.aprs->full_station_call();
            std::string mline = "[" + m.utc_hms + "] " + m.sender_call + " -> " + dst_call + ": " + m.text + ack_tag;
            uint32_t mcol = m.outbound ? (m.acked ? 0x10B981 : 0x38BDF8) : 0xFBBF24;
            r.draw_text(mline, r.font_mono_small,
                        D2D1::RectF(hist_r.left + 6.0f, mry, hist_r.right - 6.0f, mry + mr_h),
                        r.hex_color(mcol));
            mry += mr_h;
        }
    }

    // Composer Row 1: To Callsign + Quick Hints
    float cy1 = msg_y + msg_h - comp_h;
    D2D1_RECT_F to_rc = D2D1::RectF(rx_x + 6.0f, cy1, rx_x + 156.0f, cy1 + 26.0f);
    bool to_act = (hs.text_focus == HsTextFocus::AprsMsgTo);
    r.fill_round_rect(to_rc, 4.0f, r.hex_color(to_act ? 0x16243D : 0x05080F));
    r.draw_round_rect(to_rc, 4.0f, r.hex_color(to_act ? 0x00E5FF : 0x334155), to_act ? 1.5f : 1.0f);
    r.draw_text("To: " + (hs.aprs_msg_to_edit.empty() && !to_act ? std::string("CALL-9") : hs.aprs_msg_to_edit) + (to_act ? "|" : ""),
                r.font_mono_small, D2D1::RectF(to_rc.left + 6.0f, to_rc.top + 2.0f, to_rc.right - 4.0f, to_rc.bottom - 2.0f),
                r.hex_color(hs.aprs_msg_to_edit.empty() && !to_act ? 0x64748B : 0x00E5FF));
    hs.add_hit(to_rc, HS_APRS_MSG_TO_BOX);

    r.draw_text("AX.25 UI via " + hs.aprs_path_edit, r.font_mono_small,
                D2D1::RectF(to_rc.right + 8.0f, cy1 + 3.0f, rx_x + right_w - 8.0f, cy1 + 24.0f),
                r.hex_color(0x94A3B8));

    // Composer Row 2: Message Text + Send TX Button
    float cy2 = cy1 + 30.0f;
    float send_w = 78.0f;
    D2D1_RECT_F txt_rc = D2D1::RectF(rx_x + 6.0f, cy2, rx_x + right_w - send_w - 10.0f, cy2 + 27.0f);
    bool txt_act = (hs.text_focus == HsTextFocus::AprsMsgText);
    r.fill_round_rect(txt_rc, 4.0f, r.hex_color(txt_act ? 0x16243D : 0x05080F));
    r.draw_round_rect(txt_rc, 4.0f, r.hex_color(txt_act ? 0xF472B6 : 0x334155), txt_act ? 1.5f : 1.0f);
    std::string body_disp = hs.aprs_msg_text_edit.empty() && !txt_act
        ? "Type APRS message (max 67 chars)..."
        : (hs.aprs_msg_text_edit + (txt_act ? "|" : ""));
    r.draw_text(body_disp, r.font_mono_small,
                D2D1::RectF(txt_rc.left + 6.0f, txt_rc.top + 2.0f, txt_rc.right - 4.0f, txt_rc.bottom - 2.0f),
                r.hex_color(hs.aprs_msg_text_edit.empty() && !txt_act ? 0x64748B : 0xF8FAFC));
    hs.add_hit(txt_rc, HS_APRS_MSG_TEXT_BOX);

    draw_hs_btn(r, hs, mouse_pt, rx_x + right_w - send_w - 6.0f, cy2, send_w, 27.0f,
                HS_APRS_MSG_SEND, "Send TX", !hs.aprs_msg_text_edit.empty(), 0x10B981, 0, "", true);

    render_carbon_audio_dropdown_overlay(r, hs, mouse_pt);
}

// ============================================================================
// TAB 10: SSTV — 14-Mode Slow-Scan TV Transceiver (Hilbert FM RX + 48 kHz TX)
// ============================================================================
template <typename RendererT, typename AppT>
inline void render_sstv_workspace(RendererT& r, AppT& /*app*/, HamShackSuite& hs, POINT mouse_pt,
                                  float x, float y, float w, float h) {
    if (!hs.sstv) return;
    const auto tel = hs.sstv->telemetry();
    const auto& rx_spec = hamshack::carbon::sstv_mode_spec(tel.active_rx_mode);
    const auto& tx_spec = hamshack::carbon::sstv_mode_spec(tel.tx_mode);
    const float rx_rms_db = hs.audio_router ? hs.audio_router->telemetry().active_rms_dbfs : -120.0f;
    const bool rx_decoding = (tel.rx_state == hamshack::carbon::SstvRxState::ReceivingImage);

    // 1. Top Control Strip (AudioRouter Dropdown + Canvas View + Live DSP Telemetry)
    const float bar_h = 34.0f;
    D2D1_RECT_F bar_r = D2D1::RectF(x, y, x + w, y + bar_h);
    r.fill_round_rect(bar_r, 6.0f, r.hex_color(0x0A0F1D));
    r.draw_round_rect(bar_r, 6.0f, r.hex_color(0x1E293B), 1.0f);

    float cx = x + 6.0f;
    render_carbon_audio_source_bar(r, hs, mouse_pt, cx, y + 4.0f, 285.0f);
    cx += 285.0f + 5.0f + 185.0f + 8.0f;

    const char* view_lbl = (hs.sstv_canvas_layout == 0) ? "View: Split RX+TX"
                         : (hs.sstv_canvas_layout == 1) ? "View: Large RX"
                                                        : "View: Large TX";
    draw_hs_btn(r, hs, mouse_pt, cx, y + 4.0f, 128.0f, 26.0f, HS_SSTV_VIEW_CYCLE,
                view_lbl, false, 0xA855F7, 0, "", true);
    cx += 134.0f;

    // Live FM Discriminator & VIS Status Pill
    D2D1_RECT_F tel_r = D2D1::RectF(cx, y + 4.0f, x + w - 6.0f, y + 30.0f);
    r.fill_round_rect(tel_r, 4.0f, r.hex_color(0x05080F));
    r.draw_round_rect(tel_r, 4.0f, r.hex_color(tel.tx_active ? 0xEF4444 : (rx_decoding ? 0x10B981 : 0x334155)), 1.0f);
    const char* st_name = tel.tx_active ? "TRANSMITTING"
                        : rx_decoding ? "DECODING SCANLINES"
                        : (tel.rx_state == hamshack::carbon::SstvRxState::DecodingVisBits) ? "VIS HEADER SYNC"
                                                                                           : "LISTENING (VIS ARMED)";
    char tbuf[220];
    std::snprintf(tbuf, sizeof(tbuf), "%s  |  RX: %s (%ux%u)  |  Tone: %.0f Hz (%.1f dBFS)  |  Slant: %+.1f ppm",
                  st_name, std::string(rx_spec.short_name).c_str(), rx_spec.width, rx_spec.height,
                  tel.inst_freq_hz, rx_rms_db, tel.auto_slant_ppm + tel.slant_ppm);
    r.draw_text(tbuf, r.font_mono_small, D2D1::RectF(tel_r.left + 8.0f, tel_r.top + 2.0f, tel_r.right - 6.0f, tel_r.bottom - 2.0f),
                r.hex_color(tel.tx_active ? 0xFCA5A5 : (rx_decoding ? 0x34D399 : 0x38BDF8)));

    // 2. Three-Column Workstation Layout:
    //    Left: 1000-2500 Hz Tuning Waterfall + RX Controls
    //    Center: GPU Progressive Scanline RX & TX Preview Canvas
    //    Right: SSTV TX Studio (14 Modes, Load Image, 1-Bit Dither, Text Overlay, Send / Abort TX)
    const float body_y = y + bar_h + 6.0f;
    const float body_h = std::max(150.0f, h - bar_h - 6.0f);
    const float left_w = std::clamp(w * 0.24f, 275.0f, 340.0f);
    const float right_w = std::clamp(w * 0.25f, 290.0f, 360.0f);
    const float center_x = x + left_w + 6.0f;
    const float center_w = std::max(260.0f, w - left_w - right_w - 12.0f);
    const float right_x = center_x + center_w + 6.0f;

    // ---- 2A. Left Column: 1000-2500 Hz Waterfall + RX Controls ----
    D2D1_RECT_F lcard = D2D1::RectF(x, body_y, x + left_w, body_y + body_h);
    r.fill_round_rect(lcard, 8.0f, r.hex_color(0x0F172A));
    r.draw_round_rect(lcard, 8.0f, r.hex_color(0x1E293B), 1.2f);

    r.draw_text("SSTV TUNING WATERFALL (1000-2500 Hz)", r.font_ui_bold,
                D2D1::RectF(x + 10.0f, body_y + 5.0f, x + left_w - 8.0f, body_y + 23.0f),
                r.hex_color(0x00E5FF));

    const float rx_ctrl_h = 206.0f;
    const float wf_box_y = body_y + 26.0f;
    const float wf_box_h = std::max(90.0f, body_h - rx_ctrl_h - 32.0f);
    D2D1_RECT_F wf_box = D2D1::RectF(x + 8.0f, wf_box_y, x + left_w - 8.0f, wf_box_y + wf_box_h);
    r.fill_round_rect(wf_box, 5.0f, r.hex_color(0x040711));
    r.draw_round_rect(wf_box, 5.0f, r.hex_color(0x26354D), 1.0f);

    const float ruler_h = 18.0f;
    const float spec_h = std::clamp(wf_box_h * 0.26f, 28.0f, 56.0f);
    const float spec_y = wf_box.top + ruler_h;
    const float wf_img_y = spec_y + spec_h;
    const float wf_img_h = std::max(24.0f, wf_box.bottom - wf_img_y - 2.0f);
    const float plot_x = wf_box.left + 3.0f;
    const float plot_w = std::max(60.0f, (wf_box.right - wf_box.left) - 6.0f);

    // Zero-allocation GPU upload of pre-rendered BGRA32 SSTV waterfall surface (256x112)
    auto wf_surf = hs.sstv->waterfall_surface();
    D2D1_RECT_F wf_dst = D2D1::RectF(plot_x, wf_img_y, plot_x + plot_w, wf_img_y + wf_img_h);
    r.draw_bgra_surface(2, wf_surf.data(),
                        static_cast<int>(hamshack::carbon::SstvEngine::kWaterfallW),
                        static_cast<int>(hamshack::carbon::SstvEngine::kWaterfallH),
                        static_cast<int>(hamshack::carbon::SstvEngine::kWaterfallW),
                        tel.wf_surface_seq, wf_dst, false);

    // Live Spectrum Trace (1000..2500 Hz)
    auto spec_span = hs.sstv->spectrum_db();
    D2D1_POINT_2F prev_pt{};
    constexpr int kWfBins = static_cast<int>(hamshack::carbon::SstvEngine::kWaterfallW);
    for (int b = 0; b < kWfBins; ++b) {
        float fx = plot_x + (static_cast<float>(b) / static_cast<float>(kWfBins - 1)) * plot_w;
        float norm = std::clamp((spec_span[static_cast<size_t>(b)] + 95.0f) / 65.0f, 0.0f, 1.0f);
        float fy = spec_y + spec_h - 2.0f - norm * (spec_h - 4.0f);
        D2D1_POINT_2F cur_pt = D2D1::Point2F(fx, fy);
        if (b > 0) r.draw_line(prev_pt, cur_pt, r.hex_color(0x00E5FF), 1.3f);
        prev_pt = cur_pt;
    }

    // SSTV Subcarrier Reference Guides: 1200 (Sync), 1500 (Black), 1900 (VIS), 2300 (White)
    struct ToneGuide { float hz; const char* lbl; uint32_t col; };
    static constexpr ToneGuide kGuides[4] = {
        {1200.0f, "1200S", 0xEF4444},
        {1500.0f, "1500B", 0x38BDF8},
        {1900.0f, "1900V", 0xFBBF24},
        {2300.0f, "2300W", 0x10B981}
    };
    for (const auto& g : kGuides) {
        float gx = plot_x + ((g.hz - 1000.0f) / 1500.0f) * plot_w;
        r.draw_line(D2D1::Point2F(gx, spec_y), D2D1::Point2F(gx, wf_img_y + wf_img_h), r.hex_color(g.col, 0.55f), 1.0f);
        r.draw_text(g.lbl, r.font_mono_small, D2D1::RectF(gx - 18.0f, wf_box.top + 1.0f, gx + 20.0f, wf_box.top + ruler_h),
                    r.hex_color(g.col), DWRITE_TEXT_ALIGNMENT_CENTER);
    }

    // RX Controls Block below Waterfall
    float ry = wf_box.bottom + 6.0f;
    r.draw_text("RX DECODER & SLANT CONTROLS:", r.font_small,
                D2D1::RectF(x + 10.0f, ry, x + left_w - 10.0f, ry + 16.0f), r.hex_color(0xFBBF24));
    ry += 18.0f;

    const bool auto_vis = tel.auto_vis_enabled;
    draw_hs_btn(r, hs, mouse_pt, x + 8.0f, ry, (left_w - 20.0f) * 0.54f, 25.0f, HS_SSTV_AUTO_VIS_TOG,
                auto_vis ? "VIS: Auto Detect" : "VIS: Manual Lock", auto_vis, 0x10B981, 0, "", true);
    draw_hs_btn(r, hs, mouse_pt, x + 12.0f + (left_w - 20.0f) * 0.54f, ry, (left_w - 20.0f) * 0.46f, 25.0f, HS_SSTV_FORCE_RX,
                "Force Decode", !auto_vis, 0xF59E0B, 0, "", true);
    ry += 29.0f;

    // Manual Mode Selector (< Mode >)
    const auto& man_spec = hamshack::carbon::sstv_mode_spec(hs.sstv_manual_mode);
    draw_hs_btn(r, hs, mouse_pt, x + 8.0f, ry, 30.0f, 25.0f, HS_SSTV_RX_MODE_PREV, "<", false, 0x38BDF8, 0, "", true);
    D2D1_RECT_F rxm_box = D2D1::RectF(x + 41.0f, ry, x + left_w - 41.0f, ry + 25.0f);
    r.fill_round_rect(rxm_box, 4.0f, r.hex_color(0x05080F));
    r.draw_round_rect(rxm_box, 4.0f, r.hex_color(0x26354D), 1.0f);
    r.draw_text(std::string(man_spec.name), r.font_mono_small,
                rxm_box, r.hex_color(0x00E5FF), DWRITE_TEXT_ALIGNMENT_CENTER);
    draw_hs_btn(r, hs, mouse_pt, x + left_w - 38.0f, ry, 30.0f, 25.0f, HS_SSTV_RX_MODE_NEXT, ">", false, 0x38BDF8, 0, "", true);
    ry += 29.0f;

    // Slant Adjust Row (-50, -5, 0 ppm, +5, +50)
    float sl_w = (left_w - 16.0f - 16.0f) / 5.0f;
    char zero_lbl[24];
    std::snprintf(zero_lbl, sizeof(zero_lbl), "%+.0fppm", tel.slant_ppm);
    draw_hs_btn(r, hs, mouse_pt, x + 8.0f + 0*(sl_w+4), ry, sl_w, 24.0f, HS_SSTV_SLANT_M10, "-50", false, 0x38BDF8, 0, "", true);
    draw_hs_btn(r, hs, mouse_pt, x + 8.0f + 1*(sl_w+4), ry, sl_w, 24.0f, HS_SSTV_SLANT_M1,  "-5",  false, 0x38BDF8, 0, "", true);
    draw_hs_btn(r, hs, mouse_pt, x + 8.0f + 2*(sl_w+4), ry, sl_w, 24.0f, HS_SSTV_SLANT_ZERO,zero_lbl, std::abs(tel.slant_ppm) > 0.1f, 0xFBBF24, 0, "", true);
    draw_hs_btn(r, hs, mouse_pt, x + 8.0f + 3*(sl_w+4), ry, sl_w, 24.0f, HS_SSTV_SLANT_P1,  "+5",  false, 0x00E5FF, 0, "", true);
    draw_hs_btn(r, hs, mouse_pt, x + 8.0f + 4*(sl_w+4), ry, sl_w, 24.0f, HS_SSTV_SLANT_P10, "+50", false, 0x00E5FF, 0, "", true);
    ry += 28.0f;

    // Clear Canvas + Auto-Save PNG + Save Now
    float b3_w = (left_w - 24.0f) / 3.0f;
    draw_hs_btn(r, hs, mouse_pt, x + 8.0f,                 ry, b3_w, 25.0f, HS_SSTV_CLEAR_RX, "Clear RX", false, 0xEF4444, 0, "", true);
    draw_hs_btn(r, hs, mouse_pt, x + 12.0f + b3_w,         ry, b3_w, 25.0f, HS_SSTV_AUTOSAVE_TOG,
                tel.auto_save_png ? "AutoPNG:ON" : "AutoPNG:OFF", tel.auto_save_png, 0x10B981, 0, "", true);
    draw_hs_btn(r, hs, mouse_pt, x + 16.0f + b3_w * 2.0f,  ry, b3_w, 25.0f, HS_SSTV_SAVE_NOW, "Save PNG", false, 0x38BDF8, 0, "", true);
    ry += 29.0f;

    std::string saved_path = hs.sstv->last_saved_png_path();
    std::string footer_str = saved_path.empty()
        ? ("RX Frames: " + std::to_string(tel.rx_completed_images) + " (Saves to ./sstv_captures/)")
        : ("Saved: " + saved_path);
    r.draw_text(footer_str, r.font_mono_small,
                D2D1::RectF(x + 10.0f, ry, x + left_w - 8.0f, body_y + body_h - 4.0f),
                r.hex_color(saved_path.empty() ? 0x64748B : 0x34D399));

    // ---- 2B. Center Column: Progressive Scanline RX Surface & TX Preview ----
    D2D1_RECT_F ccard = D2D1::RectF(center_x, body_y, center_x + center_w, body_y + body_h);
    r.fill_round_rect(ccard, 8.0f, r.hex_color(0x05080F));
    r.draw_round_rect(ccard, 8.0f, r.hex_color(0x1E293B), 1.2f);

    auto draw_fitted_raster = [&](int gpu_slot, const uint32_t* px, int rw, int rh, uint64_t seq,
                                  const D2D1_RECT_F& pane, const std::string& title, uint32_t title_col,
                                  bool nearest_px, int active_line) {
        r.draw_text(title, r.font_ui_bold,
                    D2D1::RectF(pane.left + 8.0f, pane.top + 4.0f, pane.right - 8.0f, pane.top + 22.0f),
                    r.hex_color(title_col));
        D2D1_RECT_F avail = D2D1::RectF(pane.left + 8.0f, pane.top + 24.0f, pane.right - 8.0f, pane.bottom - 8.0f);
        float aw = std::max(32.0f, avail.right - avail.left);
        float ah = std::max(32.0f, avail.bottom - avail.top);
        float scale = std::min(aw / static_cast<float>(std::max(1, rw)),
                               ah / static_cast<float>(std::max(1, rh)));
        float dw = std::max(16.0f, rw * scale);
        float dh = std::max(16.0f, rh * scale);
        float dx = avail.left + (aw - dw) * 0.5f;
        float dy = avail.top + (ah - dh) * 0.5f;
        D2D1_RECT_F img_r = D2D1::RectF(dx, dy, dx + dw, dy + dh);
        r.fill_round_rect(D2D1::RectF(img_r.left - 2.0f, img_r.top - 2.0f, img_r.right + 2.0f, img_r.bottom + 2.0f),
                          3.0f, r.hex_color(0x020617));
        r.draw_bgra_surface(gpu_slot, px, rw, rh, static_cast<int>(hamshack::carbon::SstvEngine::kMaxWidth), seq, img_r, nearest_px);
        r.draw_round_rect(img_r, 1.0f, r.hex_color(title_col, 0.7f), 1.1f);

        if (active_line >= 0 && active_line < rh) {
            float ly = img_r.top + (static_cast<float>(active_line) / static_cast<float>(rh)) * dh;
            r.draw_line(D2D1::Point2F(img_r.left, ly), D2D1::Point2F(img_r.right, ly),
                        r.hex_color(0x10B981, 0.92f), 1.8f);
        }
    };

    uint16_t rx_w_u = 320, rx_h_u = 256;
    auto rx_surf = hs.sstv->active_rx_surface(rx_w_u, rx_h_u);
    uint16_t tx_w_u = 320, tx_h_u = 256;
    auto tx_surf = hs.sstv->tx_preview_surface(tx_w_u, tx_h_u);
    const int rx_w = static_cast<int>(rx_w_u);
    const int rx_h = static_cast<int>(rx_h_u);
    const int tx_w = static_cast<int>(tx_w_u);
    const int tx_h = static_cast<int>(tx_h_u);

    char rx_hdr[128];
    std::snprintf(rx_hdr, sizeof(rx_hdr), "LIVE RX PROGRESSIVE SCANLINE CANVAS — %s (%dx%d, Line %u/%d)",
                  std::string(rx_spec.short_name).c_str(), rx_w, rx_h, tel.rx_current_line, rx_h);
    char tx_hdr[128];
    std::snprintf(tx_hdr, sizeof(tx_hdr), "TX RASTER PREVIEW — %s (%dx%d%s)",
                  std::string(tx_spec.short_name).c_str(), tx_w, tx_h,
                  tel.tx_dither_1bit ? ", 1-Bit Floyd-Steinberg" : "");

    const int cur_line_vis = rx_decoding ? static_cast<int>(tel.rx_current_line) : -1;
    if (hs.sstv_canvas_layout == 1) {
        draw_fitted_raster(0, rx_surf.data(), rx_w, rx_h, tel.rx_surface_seq, ccard, rx_hdr, 0x00E5FF, false, cur_line_vis);
    } else if (hs.sstv_canvas_layout == 2) {
        draw_fitted_raster(1, tx_surf.data(), tx_w, tx_h, tel.tx_preview_seq, ccard, tx_hdr, 0xF472B6, tel.tx_dither_1bit, -1);
    } else {
        // Split RX + TX side-by-side (or stacked if narrow)
        if (center_w >= 540.0f) {
            float half_w = (center_w - 6.0f) * 0.5f;
            D2D1_RECT_F lp = D2D1::RectF(center_x, body_y, center_x + half_w, body_y + body_h);
            D2D1_RECT_F rp = D2D1::RectF(center_x + half_w + 6.0f, body_y, center_x + center_w, body_y + body_h);
            r.draw_line(D2D1::Point2F(center_x + half_w + 3.0f, body_y + 8.0f),
                        D2D1::Point2F(center_x + half_w + 3.0f, body_y + body_h - 8.0f),
                        r.hex_color(0x1E293B), 1.0f);
            draw_fitted_raster(0, rx_surf.data(), rx_w, rx_h, tel.rx_surface_seq, lp, rx_hdr, 0x00E5FF, false, cur_line_vis);
            draw_fitted_raster(1, tx_surf.data(), tx_w, tx_h, tel.tx_preview_seq, rp, tx_hdr, 0xF472B6, tel.tx_dither_1bit, -1);
        } else {
            float half_h = (body_h - 6.0f) * 0.5f;
            D2D1_RECT_F tp = D2D1::RectF(center_x, body_y, center_x + center_w, body_y + half_h);
            D2D1_RECT_F bp = D2D1::RectF(center_x, body_y + half_h + 6.0f, center_x + center_w, body_y + body_h);
            draw_fitted_raster(0, rx_surf.data(), rx_w, rx_h, tel.rx_surface_seq, tp, rx_hdr, 0x00E5FF, false, cur_line_vis);
            draw_fitted_raster(1, tx_surf.data(), tx_w, tx_h, tel.tx_preview_seq, bp, tx_hdr, 0xF472B6, tel.tx_dither_1bit, -1);
        }
    }

    // ---- 2C. Right Column: SSTV TX Modulator Studio ----
    D2D1_RECT_F rcard = D2D1::RectF(right_x, body_y, right_x + right_w, body_y + body_h);
    r.fill_round_rect(rcard, 8.0f, r.hex_color(0x0F172A));
    r.draw_round_rect(rcard, 8.0f, r.hex_color(0x1E293B), 1.2f);

    r.draw_text("SSTV TX MODULATOR STUDIO (48 kHz)", r.font_ui_bold,
                D2D1::RectF(right_x + 10.0f, body_y + 6.0f, right_x + right_w - 10.0f, body_y + 24.0f),
                r.hex_color(0xF472B6));

    float ty = body_y + 28.0f;
    r.draw_text("1. SELECT TRANSMIT SSTV MODE (14 MODES):", r.font_small,
                D2D1::RectF(right_x + 10.0f, ty, right_x + right_w - 10.0f, ty + 16.0f), r.hex_color(0x94A3B8));
    ty += 18.0f;

    draw_hs_btn(r, hs, mouse_pt, right_x + 8.0f, ty, 32.0f, 28.0f, HS_SSTV_TX_MODE_PREV, "<", false, 0xF472B6, 0, "", true);
    D2D1_RECT_F txm_r = D2D1::RectF(right_x + 44.0f, ty, right_x + right_w - 44.0f, ty + 28.0f);
    r.fill_round_rect(txm_r, 4.0f, r.hex_color(0x05080F));
    r.draw_round_rect(txm_r, 4.0f, r.hex_color(0xF472B6, 0.7f), 1.0f);
    r.draw_text(std::string(tx_spec.name), r.font_ui_bold, txm_r, r.hex_color(0xF8FAFC), DWRITE_TEXT_ALIGNMENT_CENTER);
    draw_hs_btn(r, hs, mouse_pt, right_x + right_w - 40.0f, ty, 32.0f, 28.0f, HS_SSTV_TX_MODE_NEXT, ">", false, 0xF472B6, 0, "", true);
    ty += 32.0f;

    // Mode Spec Summary Card
    double frame_sec = (tx_spec.encoding == hamshack::carbon::SstvColorEncoding::PdDualLineYuv)
        ? ((tx_spec.height * 0.5) * tx_spec.line_period_ms / 1000.0)
        : (tx_spec.height * tx_spec.line_period_ms / 1000.0);
    const char* cspace = (tx_spec.encoding == hamshack::carbon::SstvColorEncoding::GrayscaleBw)
                       ? "Grayscale Luminance (No Chroma)"
                       : (tx_spec.encoding == hamshack::carbon::SstvColorEncoding::GbrSequential ||
                          tx_spec.encoding == hamshack::carbon::SstvColorEncoding::GbrMidSync)
                           ? "GBR Sequential"
                           : "YCrCb Subcarrier";
    D2D1_RECT_F spec_card = D2D1::RectF(right_x + 8.0f, ty, right_x + right_w - 8.0f, ty + 42.0f);
    r.fill_round_rect(spec_card, 5.0f, r.hex_color(0x05080F));
    r.draw_round_rect(spec_card, 5.0f, r.hex_color(0x1E293B), 1.0f);
    char sline1[96], sline2[96];
    std::snprintf(sline1, sizeof(sline1), "Res: %ux%u  |  VIS: 0x%02X (%u)  |  %.1fs",
                  tx_spec.width, tx_spec.height, tx_spec.vis_code, tx_spec.vis_code, frame_sec);
    std::snprintf(sline2, sizeof(sline2), "Encoding: %s", cspace);
    r.draw_text(sline1, r.font_mono_small, D2D1::RectF(spec_card.left + 8.0f, ty + 3.0f, spec_card.right - 6.0f, ty + 20.0f), r.hex_color(0x38BDF8));
    r.draw_text(sline2, r.font_mono_small, D2D1::RectF(spec_card.left + 8.0f, ty + 21.0f, spec_card.right - 6.0f, ty + 39.0f), r.hex_color(0x34D399));
    ty += 48.0f;

    // Image Loader + Test Card
    r.draw_text("2. SOURCE IMAGE & RASTER PRE-PROCESSING:", r.font_small,
                D2D1::RectF(right_x + 10.0f, ty, right_x + right_w - 10.0f, ty + 16.0f), r.hex_color(0x94A3B8));
    ty += 18.0f;

    float half_btn = (right_w - 20.0f) * 0.5f;
    draw_hs_btn(r, hs, mouse_pt, right_x + 8.0f, ty, half_btn, 28.0f, HS_SSTV_LOAD_IMG,
                "\xf0\x9f\x93\x82 Load Image...", false, 0x00E5FF, 0, "", true);
    draw_hs_btn(r, hs, mouse_pt, right_x + 12.0f + half_btn, ty, half_btn, 28.0f, HS_SSTV_TEST_CARD,
                "\xf0\x9f\x8e\xa8 Test Card", false, 0xFBBF24, 0, "", true);
    ty += 34.0f;

    // 1-Bit Floyd-Steinberg Dithering Toggle
    draw_hs_btn(r, hs, mouse_pt, right_x + 8.0f, ty, right_w - 16.0f, 27.0f, HS_SSTV_DITHER_TOG,
                tel.tx_dither_1bit
                    ? "\xe2\x96\xa3 1-Bit Floyd-Steinberg Dither: ENABLED"
                    : "\xe2\x96\xa2 1-Bit Floyd-Steinberg Dither: OFF (Continuous)",
                tel.tx_dither_1bit, 0xA855F7, 0, "", true);
    ty += 33.0f;

    // Text Overlay Input Box (Callsign / Grid / RST)
    r.draw_text("3. CALLSIGN / GRID / RST RASTER OVERLAY:", r.font_small,
                D2D1::RectF(right_x + 10.0f, ty, right_x + right_w - 10.0f, ty + 16.0f), r.hex_color(0x94A3B8));
    ty += 18.0f;

    D2D1_RECT_F ov_rc = D2D1::RectF(right_x + 8.0f, ty, right_x + right_w - 8.0f, ty + 28.0f);
    bool ov_act = (hs.text_focus == HsTextFocus::SstvOverlay);
    r.fill_round_rect(ov_rc, 4.0f, r.hex_color(ov_act ? 0x16243D : 0x05080F));
    r.draw_round_rect(ov_rc, 4.0f, r.hex_color(ov_act ? 0x00E5FF : 0x334155), ov_act ? 1.5f : 1.0f);
    r.draw_text(hs.sstv_overlay_edit + (ov_act ? "|" : ""), r.font_mono_small,
                D2D1::RectF(ov_rc.left + 8.0f, ov_rc.top + 3.0f, ov_rc.right - 6.0f, ov_rc.bottom - 3.0f),
                r.hex_color(0xF8FAFC));
    hs.add_hit(ov_rc, HS_SSTV_OVERLAY_BOX);
    ty += 34.0f;

    // Embedded mini TX preview when in Large RX mode and vertical space permits
    float bottom_block_h = 104.0f;
    if (hs.sstv_canvas_layout == 1 && (body_y + body_h - bottom_block_h - ty) >= 90.0f) {
        D2D1_RECT_F mini_p = D2D1::RectF(right_x + 6.0f, ty, right_x + right_w - 6.0f, body_y + body_h - bottom_block_h - 4.0f);
        draw_fitted_raster(1, tx_surf.data(), tx_w, tx_h, tel.tx_preview_seq, mini_p, "TX PREVIEW", 0xF472B6,
                           tel.tx_dither_1bit, -1);
    }

    // TX Progress Bar + Send + Abort TX pinned cleanly at bottom of Right Studio Card
    float by = body_y + body_h - bottom_block_h;
    D2D1_RECT_F prog_bg = D2D1::RectF(right_x + 8.0f, by, right_x + right_w - 8.0f, by + 24.0f);
    r.fill_round_rect(prog_bg, 5.0f, r.hex_color(0x05080F));
    if (tel.tx_active || tel.tx_progress > 0.001f) {
        float pw = (prog_bg.right - prog_bg.left) * std::clamp(tel.tx_progress, 0.0f, 1.0f);
        r.fill_round_rect(D2D1::RectF(prog_bg.left, prog_bg.top, prog_bg.left + pw, prog_bg.bottom),
                          5.0f, r.hex_color(tel.tx_active ? 0x10B981 : 0x334155, 0.65f));
    }
    r.draw_round_rect(prog_bg, 5.0f, r.hex_color(tel.tx_active ? 0x10B981 : 0x26354D), 1.0f);
    char ptxt[96];
    if (tel.tx_active) {
        std::snprintf(ptxt, sizeof(ptxt), "TX PROGRESS: %5.1f%%  (1.5ms Taper Armed)", tel.tx_progress * 100.0f);
    } else {
        std::snprintf(ptxt, sizeof(ptxt), "TX IDLE — Ready (%s)",
                      hs.audio_router && hs.audio_router->active_source() == hamshack::carbon::AudioSource::UsbDongleVhf
                          ? "UV-5R VOX 48kHz" : "Xiegu G90 CAT PTT");
    }
    r.draw_text(ptxt, r.font_mono_small, prog_bg, r.hex_color(tel.tx_active ? 0xF8FAFC : 0x94A3B8),
                DWRITE_TEXT_ALIGNMENT_CENTER);
    by += 30.0f;

    draw_hs_btn(r, hs, mouse_pt, right_x + 8.0f, by, half_btn, 34.0f, HS_SSTV_SEND_TX,
                tel.tx_active ? "\xf0\x9f\x93\xa1 Transmitting..." : "\xf0\x9f\x93\xa1 Send SSTV",
                tel.tx_active, 0x10B981, 0, "", false);
    draw_hs_btn(r, hs, mouse_pt, right_x + 12.0f + half_btn, by, half_btn, 34.0f, HS_SSTV_ABORT_TX,
                "\xe2\x9b\x94 Abort TX", false, 0xEF4444, 0, "", false);
    by += 38.0f;

    r.draw_text(hs.sstv_status_banner, r.font_mono_small,
                D2D1::RectF(right_x + 8.0f, by, right_x + right_w - 8.0f, body_y + body_h - 3.0f),
                r.hex_color(0x38BDF8));

    render_carbon_audio_dropdown_overlay(r, hs, mouse_pt);
}

// ============================================================================
// Mouse Click & Keyboard Handler for HamShack++ Top Deck & Mode Tabs
// ============================================================================
template <typename AppT>
inline bool handle_hamshack_mouse_down(AppT& app, HamShackSuite& hs, POINT pt, HWND hwnd = nullptr) {
    if (hs.audio_source_dropdown_open) {
        for (const auto& h : hs.hits) {
            if (pt.x < h.rect.left || pt.x > h.rect.right || pt.y < h.rect.top || pt.y > h.rect.bottom) continue;
            if (h.id == HS_AUDIO_SRC_OPT_G90) {
                if (hs.audio_router) hs.audio_router->select_source(hamshack::carbon::AudioSource::PrimaryG90);
                hs.audio_source_dropdown_open = false;
                return true;
            }
            if (h.id == HS_AUDIO_SRC_OPT_DONGLE) {
                if (hs.audio_router) hs.audio_router->select_source(hamshack::carbon::AudioSource::UsbDongleVhf);
                hs.audio_source_dropdown_open = false;
                return true;
            }
        }
        hs.audio_source_dropdown_open = false;
    }
    if (hs.irc_menu.is_open) {
        for (const auto& h : hs.hits) {
            if (pt.x < h.rect.left || pt.x > h.rect.right || pt.y < h.rect.top || pt.y > h.rect.bottom) continue;
            if (h.id == HS_IRC_MENU_OP) {
                if (!hs.irc_menu.nick.empty() && !hs.irc_menu.channel.empty()) {
                    bool is_op = (hs.irc_menu.prefix == '@' || hs.irc_menu.prefix == '~' || hs.irc_menu.prefix == '&');
                    if (is_op) hs.irc.deop_user(hs.irc_menu.channel, hs.irc_menu.nick);
                    else hs.irc.op_user(hs.irc_menu.channel, hs.irc_menu.nick);
                }
                hs.irc_menu.is_open = false;
                return true;
            } else if (h.id == HS_IRC_MENU_VOICE) {
                if (!hs.irc_menu.nick.empty() && !hs.irc_menu.channel.empty()) {
                    bool is_v = (hs.irc_menu.prefix == '+');
                    if (is_v) hs.irc.devoice_user(hs.irc_menu.channel, hs.irc_menu.nick);
                    else hs.irc.voice_user(hs.irc_menu.channel, hs.irc_menu.nick);
                }
                hs.irc_menu.is_open = false;
                return true;
            } else if (h.id == HS_IRC_MENU_WHOIS) {
                if (!hs.irc_menu.nick.empty()) hs.irc.whois_user(hs.irc_menu.nick);
                hs.irc_menu.is_open = false;
                return true;
            } else if (h.id == HS_IRC_MENU_QUERY) {
                if (!hs.irc_menu.nick.empty()) hs.irc.handle_user_slash_command("/query " + hs.irc_menu.nick);
                hs.irc_menu.is_open = false;
                return true;
            } else if (h.id == HS_IRC_MENU_KICK) {
                if (!hs.irc_menu.nick.empty() && !hs.irc_menu.channel.empty()) hs.irc.kick_user(hs.irc_menu.channel, hs.irc_menu.nick);
                hs.irc_menu.is_open = false;
                return true;
            } else if (h.id == HS_IRC_MENU_BAN) {
                if (!hs.irc_menu.nick.empty() && !hs.irc_menu.channel.empty()) hs.irc.ban_user(hs.irc_menu.channel, hs.irc_menu.nick);
                hs.irc_menu.is_open = false;
                return true;
            }
        }
        hs.irc_menu.is_open = false;
        // Close menu on click outside and proceed to test other hits below
    }

    if (hs.log_qso_dialog.is_open) {
        for (const auto& h : hs.hits) {
            if (pt.x < h.rect.left || pt.x > h.rect.right || pt.y < h.rect.top || pt.y > h.rect.bottom) continue;
            switch (h.id) {
                case HS_LOG_QSO_OK:
                    hs.log_qso_dialog.save_and_commit(hs.ft8, hs.flrig.connected ? hs.flrig.vfo_a_hz : 0ULL);
                    app.log_public_system("[WSJT-X ADIF LOG] Logged QSO with " + hs.log_qso_dialog.call +
                                          " (" + hs.log_qso_dialog.grid + ") on " + hs.log_qso_dialog.freq_str + " MHz to hamshack_wsjtx_log.adi");
                    hs.log_qso_dialog.is_open = false;
                    hs.text_focus = HsTextFocus::None;
                    return true;
                case HS_LOG_QSO_CANCEL:
                    hs.log_qso_dialog.is_open = false;
                    hs.text_focus = HsTextFocus::None;
                    return true;
                case HS_LOG_QSO_CALL: hs.text_focus = HsTextFocus::LogQsoCall; return true;
                case HS_LOG_QSO_GRID: hs.text_focus = HsTextFocus::LogQsoGrid; return true;
                case HS_LOG_QSO_MODE: hs.text_focus = HsTextFocus::LogQsoMode; return true;
                case HS_LOG_QSO_RPT_S: hs.text_focus = HsTextFocus::LogQsoRptS; return true;
                case HS_LOG_QSO_RPT_R: hs.text_focus = HsTextFocus::LogQsoRptR; return true;
                case HS_LOG_QSO_FREQ: hs.text_focus = HsTextFocus::LogQsoFreq; return true;
                case HS_LOG_QSO_PWR:  hs.text_focus = HsTextFocus::LogQsoPwr;  return true;
                case HS_LOG_QSO_COMM: hs.text_focus = HsTextFocus::LogQsoComm; return true;
            }
        }
        return true; // Modal consumes click
    }
    for (const auto& h : hs.hits) {
        if (pt.x < h.rect.left || pt.x > h.rect.right || pt.y < h.rect.top || pt.y > h.rect.bottom) continue;

        switch (h.id) {
            case HS_TAB_FT8:    hs.switch_mode_tab(0); return true;
            case HS_TAB_JS8:    hs.switch_mode_tab(1); return true;
            case HS_TAB_CW:     hs.switch_mode_tab(2); return true;
            case HS_TAB_MESH:   hs.switch_mode_tab(3); return true;
            case HS_TAB_GT:     hs.switch_mode_tab(4); return true;
            case HS_TAB_PAGES:  hs.switch_mode_tab(5); return true;
            case HS_TAB_IRC:    hs.switch_mode_tab(6); return true;
            case HS_TAB_WINLINK:hs.switch_mode_tab(7); return true;
            case HS_TAB_CONFIG: hs.switch_mode_tab(8); return true;
            case HS_TAB_APRS:   hs.switch_mode_tab(9); return true;
            case HS_TAB_SSTV:   hs.switch_mode_tab(10); return true;

            case HS_AUDIO_SRC_DROPDOWN:
                hs.audio_source_dropdown_open = !hs.audio_source_dropdown_open;
                return true;
            case HS_AUDIO_DONGLE_CYCLE: {
                if (hs.audio_router) {
                    hs.audio_router->refresh_usb_dongle_devices(hs.soundcard.selected_input_idx);
                    auto devs = hs.audio_router->devices();
                    if (!devs.empty()) {
                        int cur = hs.audio_router->selected_dongle_device_index();
                        int nxt = (cur < 0) ? 0 : ((cur + 1) % static_cast<int>(devs.size()));
                        hs.audio_router->bind_usb_dongle_device(nxt);
                    }
                }
                return true;
            }

            // APRS Tab Controls
            case HS_APRS_HOST_BOX:    hs.text_focus = HsTextFocus::AprsHost; return true;
            case HS_APRS_PORT_BOX:    hs.text_focus = HsTextFocus::AprsPort; return true;
            case HS_APRS_PATH_BOX:    hs.text_focus = HsTextFocus::AprsPath; return true;
            case HS_APRS_COMMENT_BOX: hs.text_focus = HsTextFocus::AprsComment; return true;
            case HS_APRS_MSG_TO_BOX:  hs.text_focus = HsTextFocus::AprsMsgTo; return true;
            case HS_APRS_MSG_TEXT_BOX:hs.text_focus = HsTextFocus::AprsMsgText; return true;
            case HS_APRS_SSID_CYCLE: {
                hs.aprs_ssid = (hs.aprs_ssid + 1) % 16;
                if (hs.aprs) hs.aprs->set_station_identity(hs.hf_callsign, static_cast<uint8_t>(hs.aprs_ssid), hs.aprs_path_edit);
                auto& cfg = hamshack::get_app_config();
                cfg.aprs_ssid = hs.aprs_ssid;
                cfg.save();
                return true;
            }
            case HS_APRS_CONNECT_TOG: {
                if (!hs.aprs) return true;
                if (hs.aprs->is_running()) {
                    hs.aprs->disconnect_kiss();
                    hs.aprs_status_banner = "Disconnected from KISS TCP";
                } else {
                    int port = std::clamp(std::atoi(hs.aprs_port_edit.c_str()), 1, 65535);
                    hs.aprs_port_edit = std::to_string(port);
                    auto& cfg = hamshack::get_app_config();
                    cfg.aprs_kiss_host = hs.aprs_host_edit;
                    cfg.aprs_kiss_port = port;
                    cfg.aprs_digi_path = hs.aprs_path_edit;
                    cfg.save();
                    hs.aprs->set_station_identity(hs.hf_callsign, static_cast<uint8_t>(hs.aprs_ssid), hs.aprs_path_edit);
                    hs.aprs->set_kiss_endpoint(hs.aprs_host_edit, static_cast<uint16_t>(port));
                    hs.aprs->connect_kiss();
                    hs.aprs_status_banner = "Connecting to Dire Wolf KISS TCP (" + hs.aprs_host_edit + ":" + hs.aprs_port_edit + ")...";
                }
                return true;
            }
            case HS_APRS_BEACON_TX: {
                if (hs.aprs) {
                    hs.aprs->set_station_identity(hs.hf_callsign, static_cast<uint8_t>(hs.aprs_ssid), hs.aprs_path_edit);
                    double lat = 41.7147, lon = -72.7272;
                    hamshack::Ft8Ft4Engine::grid_to_latlon(hs.hf_grid, lat, lon);
                    if (hs.aprs->send_position_beacon(lat, lon, '/', '-', hs.aprs_comment_edit)) {
                        hs.aprs_status_banner = "Queued AX.25 GPS Beacon (" + hs.aprs->full_station_call() + ")";
                    } else {
                        hs.aprs_status_banner = "Beacon queue full — check KISS TCP connection";
                    }
                }
                return true;
            }
            case HS_APRS_MSG_SEND: {
                if (hs.aprs && !hs.aprs_msg_to_edit.empty() && !hs.aprs_msg_text_edit.empty()) {
                    hs.aprs->set_station_identity(hs.hf_callsign, static_cast<uint8_t>(hs.aprs_ssid), hs.aprs_path_edit);
                    hs.aprs->send_text_message(hs.aprs_msg_to_edit, hs.aprs_msg_text_edit);
                    hs.aprs_msg_text_edit.clear();
                }
                return true;
            }
            case HS_APRS_CLEAR_BTN: {
                if (hs.aprs) hs.aprs->clear_logs();
                return true;
            }
            case HS_APRS_STATION_ROW: {
                hs.aprs_selected_call = h.payload;
                hs.aprs_msg_to_edit = h.payload;
                if (hs.aprs) {
                    for (const auto& s : hs.aprs->stations()) {
                        if (h.payload == s.callsign_ssid && s.has_position) {
                            hs.gt_map_nx = (s.longitude + 180.0) / 360.0;
                            double rlat = std::clamp(s.latitude, -85.0, 85.0) * 3.14159265358979323846 / 180.0;
                            hs.gt_map_ny = (1.0 - std::log(std::tan(rlat) + 1.0 / std::cos(rlat)) / 3.14159265358979323846) / 2.0;
                            hs.gt_map_zoom = std::max(hs.gt_map_zoom, 7);
                            break;
                        }
                    }
                }
                return true;
            }

            // SSTV Tab Controls (RX + TX Studio)
            case HS_SSTV_AUTO_VIS_TOG:
                if (hs.sstv) {
                    bool nxt = !hs.sstv->telemetry().auto_vis_enabled;
                    hs.sstv->set_mode_override(nxt, hs.sstv_manual_mode);
                }
                return true;
            case HS_SSTV_RX_MODE_PREV:
                if (hs.sstv) {
                    int m = (static_cast<int>(hs.sstv_manual_mode) + static_cast<int>(hamshack::carbon::kNumSstvModes) - 1) %
                            static_cast<int>(hamshack::carbon::kNumSstvModes);
                    hs.sstv_manual_mode = static_cast<hamshack::carbon::SstvMode>(m);
                    hs.sstv->set_mode_override(hs.sstv->telemetry().auto_vis_enabled, hs.sstv_manual_mode);
                }
                return true;
            case HS_SSTV_RX_MODE_NEXT:
                if (hs.sstv) {
                    int m = (static_cast<int>(hs.sstv_manual_mode) + 1) % static_cast<int>(hamshack::carbon::kNumSstvModes);
                    hs.sstv_manual_mode = static_cast<hamshack::carbon::SstvMode>(m);
                    hs.sstv->set_mode_override(hs.sstv->telemetry().auto_vis_enabled, hs.sstv_manual_mode);
                }
                return true;
            case HS_SSTV_FORCE_RX:
                if (hs.sstv) {
                    hs.sstv->force_start_manual_rx(hs.sstv_manual_mode);
                    hs.sstv_status_banner = "Forced RX Scanline Sync: " + std::string(hamshack::carbon::sstv_mode_spec(hs.sstv_manual_mode).name);
                }
                return true;
            case HS_SSTV_SLANT_M10:  if (hs.sstv) hs.sstv->adjust_slant_ppm(-50.0f); return true;
            case HS_SSTV_SLANT_M1:   if (hs.sstv) hs.sstv->adjust_slant_ppm(-5.0f);  return true;
            case HS_SSTV_SLANT_ZERO: if (hs.sstv) hs.sstv->set_slant_ppm(0.0f);      return true;
            case HS_SSTV_SLANT_P1:   if (hs.sstv) hs.sstv->adjust_slant_ppm(+5.0f);  return true;
            case HS_SSTV_SLANT_P10:  if (hs.sstv) hs.sstv->adjust_slant_ppm(+50.0f); return true;
            case HS_SSTV_CLEAR_RX:   if (hs.sstv) hs.sstv->clear_rx_canvas();        return true;
            case HS_SSTV_AUTOSAVE_TOG:
                if (hs.sstv) {
                    bool nxt = !hs.sstv->telemetry().auto_save_png;
                    hs.sstv->set_auto_save_png(nxt);
                    auto& cfg = hamshack::get_app_config();
                    cfg.sstv_auto_save = nxt;
                    cfg.save();
                }
                return true;
            case HS_SSTV_SAVE_NOW:
                if (hs.sstv) {
                    std::string p;
                    bool ok = hs.sstv->save_current_rx_png(p);
                    hs.sstv_status_banner = (!ok || p.empty()) ? "PNG save failed" : ("Saved PNG: " + p);
                }
                return true;
            case HS_SSTV_VIEW_CYCLE:
                hs.sstv_canvas_layout = (hs.sstv_canvas_layout + 1) % 3;
                return true;
            case HS_SSTV_TX_MODE_PREV:
                if (hs.sstv) {
                    int m = (static_cast<int>(hs.sstv->telemetry().tx_mode) + static_cast<int>(hamshack::carbon::kNumSstvModes) - 1) %
                            static_cast<int>(hamshack::carbon::kNumSstvModes);
                    hs.sstv->set_tx_mode(static_cast<hamshack::carbon::SstvMode>(m));
                }
                return true;
            case HS_SSTV_TX_MODE_NEXT:
                if (hs.sstv) {
                    int m = (static_cast<int>(hs.sstv->telemetry().tx_mode) + 1) % static_cast<int>(hamshack::carbon::kNumSstvModes);
                    hs.sstv->set_tx_mode(static_cast<hamshack::carbon::SstvMode>(m));
                }
                return true;
            case HS_SSTV_LOAD_IMG: {
                if (hs.sstv) {
                    std::wstring wpath = meshirc::open_image_file_dialog(hwnd);
                    if (!wpath.empty()) {
                        std::string err_msg;
                        if (hs.sstv->load_tx_image_file(wpath, err_msg)) {
                            hs.sstv_status_banner = "Loaded TX Image: " + std::filesystem::path(wpath).filename().string();
                        } else {
                            hs.sstv_status_banner = err_msg.empty() ? "Failed to decode image (use PNG/JPEG/BMP)" : err_msg;
                        }
                    }
                }
                return true;
            }
            case HS_SSTV_TEST_CARD:
                if (hs.sstv) {
                    hs.sstv->generate_default_test_card();
                    hs.sstv_status_banner = "Loaded SMPTE Color-Bar Test Card";
                }
                return true;
            case HS_SSTV_DITHER_TOG:
                if (hs.sstv) {
                    hs.sstv->set_tx_dither_1bit(!hs.sstv->telemetry().tx_dither_1bit);
                }
                return true;
            case HS_SSTV_OVERLAY_BOX:
                hs.text_focus = HsTextFocus::SstvOverlay;
                return true;
            case HS_SSTV_SEND_TX:
                if (hs.sstv) {
                    if (hs.sstv->start_transmit()) {
                        hs.sstv_status_banner = "Transmitting " + std::string(hamshack::carbon::sstv_mode_spec(hs.sstv->telemetry().tx_mode).name) + " (48 kHz PCM)";
                    }
                }
                return true;
            case HS_SSTV_ABORT_TX:
                if (hs.sstv) {
                    hs.sstv->abort_transmit();
                    hs.sstv_status_banner = "TX Aborted cleanly (1.5 ms cosine ramp-down)";
                }
                return true;

            case HS_MODE_SET_LSB: hs.flrig.set_mode(hamshack::RigMode::LSB); return true;
            case HS_MODE_SET_USB: hs.flrig.set_mode(hamshack::RigMode::USB); return true;
            case HS_MODE_SET_CW:  hs.flrig.set_mode(hamshack::RigMode::CW);  return true;
            case HS_MODE_SET_AM:  hs.flrig.set_mode(hamshack::RigMode::AM);  return true;
            case HS_ADIF_IMPORT_BTN: {
                std::string p = hamshack_web::pick_adif_file_macos();
                if (!p.empty()) {
                    hs.ft8.import_adif_file(p);
                    app.log_public_system("[ADIF IMPORT] Successfully loaded " + p + ": " + hs.ft8.adif_status_text);
                }
                return true;
            }

            case HS_VFO_STEP_M10K: hs.flrig.step_active_vfo(-10000); return true;
            case HS_VFO_STEP_P10K: hs.flrig.step_active_vfo(+10000); return true;
            case HS_VFO_STEP_M1K:  hs.flrig.step_active_vfo(-1000);  return true;
            case HS_VFO_STEP_P1K:  hs.flrig.step_active_vfo(+1000);  return true;
            case HS_VFO_STEP_M100: hs.flrig.step_active_vfo(-100);   return true;
            case HS_VFO_STEP_P100: hs.flrig.step_active_vfo(+100);   return true;
            case HS_VFO_SELECT_A:  hs.flrig.select_vfo(false); return true;
            case HS_VFO_SELECT_B:  hs.flrig.select_vfo(true);  return true;
            case HS_VFO_SWAP_AB:   hs.flrig.swap_vfo_ab();     return true;
            case HS_VFO_COPY_AB:   hs.flrig.copy_vfo_a_to_b(); return true;
            case HS_VFO_SPLIT:     hs.flrig.set_split(!hs.flrig.split_enabled); return true;

            case HS_MODE_CYCLE: {
                int next_m = (static_cast<int>(hs.flrig.active_vfo_b ? hs.flrig.mode_b : hs.flrig.mode_a) + 1) % 5;
                hs.flrig.set_mode(static_cast<hamshack::RigMode>(next_m));
                return true;
            }
            case HS_FILT_CYCLE:  hs.flrig.cycle_filter(); return true;
            case HS_PTT_TOGGLE:  hs.flrig.set_ptt(!hs.flrig.ptt_active); return true;
            case HS_ATU_TUNE:    hs.flrig.start_atu_tune(); return true;
            case HS_PRE_TOGGLE:  hs.flrig.toggle_preamp(); return true;
            case HS_ATT_TOGGLE:  hs.flrig.toggle_attenuator(); return true;
            case HS_NB_TOGGLE:   hs.flrig.toggle_nb(); return true;
            case HS_COMP_TOGGLE: hs.flrig.toggle_comp(); return true;
            case HS_AGC_CYCLE:   hs.flrig.cycle_agc(); return true;
            case HS_PWR_DOWN:    hs.flrig.step_power_watts(-1); return true;
            case HS_PWR_UP:      hs.flrig.step_power_watts(+1); return true;
            case HS_VOL_DOWN:    hs.flrig.step_volume(-5); return true;
            case HS_VOL_UP:      hs.flrig.step_volume(+5); return true;
            case HS_SQL_DOWN:    hs.flrig.step_squelch(-5); return true;
            case HS_SQL_UP:      hs.flrig.step_squelch(+5); return true;

            case HS_RIG_MODEL_CYCLE: {
                int nm = (static_cast<int>(hs.flrig.model) + 1) % 10;
                hs.flrig.select_rig_model(static_cast<hamshack::RigModel>(nm));
                return true;
            }
            case HS_CAT_PORT_CYCLE:
                hs.flrig.cycle_serial_port();
                return true;
            case HS_CAT_RESCAN:
                hs.flrig.scan_serial_ports();
                hs.flrig.connect_serial(hs.flrig.serial_port, hs.flrig.baud_rate);
                return true;
            case HS_AUDIO_IN_CYCLE: {
                if (!hs.soundcard.input_devices.empty()) {
                    int ni = (hs.soundcard.selected_input_idx + 1) % static_cast<int>(hs.soundcard.input_devices.size());
                    hs.soundcard.select_input_device(ni);
                }
                return true;
            }
            case HS_HF_CALLSIGN_BOX:
                hs.text_focus = HsTextFocus::HfCallsign;
                return true;
            case HS_HF_GRID_BOX:
                hs.text_focus = HsTextFocus::HfGrid;
                return true;
            case HS_WF_RESIZE_BAR:
                hs.dragging_wide_graph = true;
                hs.wf_drag_invert = (h.index != 0);
                hs.wf_drag_start_y = pt.y;
                hs.wf_drag_start_h = hs.wide_graph_height;
                return true;
            case HS_WF_HEIGHT_MINUS: {
                hs.wide_graph_height = std::clamp(hs.wide_graph_height - 25.0f, 110.0f, 540.0f);
                auto& cfg = hamshack::get_app_config();
                cfg.wide_graph_height = hs.wide_graph_height;
                cfg.save();
                return true;
            }
            case HS_WF_HEIGHT_PLUS: {
                hs.wide_graph_height = std::clamp(hs.wide_graph_height + 25.0f, 110.0f, 540.0f);
                auto& cfg = hamshack::get_app_config();
                cfg.wide_graph_height = hs.wide_graph_height;
                cfg.save();
                return true;
            }
            case HS_WF_POS_TOGGLE: {
                hs.wide_graph_pos = (hs.wide_graph_pos == 0) ? 1 : 0;
                auto& cfg = hamshack::get_app_config();
                cfg.wide_graph_pos = hs.wide_graph_pos;
                cfg.wide_graph_height = hs.wide_graph_height;
                cfg.save();
                return true;
            }
            case HS_WF_PALETTE_CYCLE: {
                hs.wf_color_scheme = (hs.wf_color_scheme + 1) % 9;
                auto& cfg = hamshack::get_app_config();
                cfg.wf_color_scheme = hs.wf_color_scheme;
                cfg.save();
                return true;
            }
            case HS_WF_GRID_TOGGLE: {
                hs.wf_show_signal_grid = !hs.wf_show_signal_grid;
                auto& cfg = hamshack::get_app_config();
                cfg.wf_show_signal_grid = hs.wf_show_signal_grid;
                cfg.save();
                return true;
            }

            // WSJT-X FT8/FT4 Controls
            case HS_FT_SUBMODE_FT8:
                hs.ft8.is_ft4 = false;
                if (hs.flrig.connected && hs.flrig.active_band_idx >= 0)
                    hs.flrig.select_band_for_mode(hs.flrig.active_band_idx, 0);
                return true;
            case HS_FT_SUBMODE_FT4:
                hs.ft8.is_ft4 = true;
                if (hs.flrig.connected && hs.flrig.active_band_idx >= 0)
                    hs.flrig.select_band_for_mode(hs.flrig.active_band_idx, 1);
                return true;
            case HS_FT_ENABLE_TX: hs.ft8.tx_enabled = !hs.ft8.tx_enabled; return true;
            case HS_FT_HALT_TX:
                hs.ft8.halt_tx_and_clear_working(hs.soundcard);
                hs.flrig.set_ptt(false);
                return true;
            case HS_FT_TUNE:
                hs.ft8.tune_active = !hs.ft8.tune_active;
                if (!hs.ft8.tune_active) {
                    hs.soundcard.halt_tx();
                    hs.flrig.set_ptt(false);
                }
                return true;
            case HS_FT_AUTO_SEQ:   hs.ft8.auto_seq = !hs.ft8.auto_seq; return true;
            case HS_FT_TX_EVEN:    hs.ft8.tx_even_first = !hs.ft8.tx_even_first; return true;
            case HS_FT_HOLD_TX:    hs.ft8.hold_tx_freq = !hs.ft8.hold_tx_freq; return true;
            case HS_FT_MONITOR:    hs.ft8.monitor_enabled = !hs.ft8.monitor_enabled; return true;
            case HS_FT_LOG_QSO:
                hs.log_qso_dialog.open_for_current_ft8(hs.ft8, hs.flrig.connected ? hs.flrig.vfo_a_hz : 0ULL);
                hs.text_focus = HsTextFocus::LogQsoComm;
                return true;
            case HS_FT_ERASE: {
                std::lock_guard<std::recursive_mutex> lk(hs.ft8.mtx);
                hs.ft8.rx_freq_activity.clear();
                hs.ft8.band_activity.clear();
                return true;
            }
            case HS_FT_DECODE_NOW:
                hs.ft8.trigger_manual_decode_async(hs.soundcard);
                return true;
            case HS_FT_DX_CALL_BOX:
                hs.text_focus = HsTextFocus::Ft8DxCall;
                return true;
            case HS_FT_DX_GRID_BOX:
                hs.text_focus = HsTextFocus::Ft8DxGrid;
                return true;
            case HS_FT_WATERFALL: {
                float norm = std::clamp((pt.x - h.rect.left) / std::max(1.0f, h.rect.right - h.rect.left), 0.0f, 1.0f);
                int hz = 100 + static_cast<int>(norm * 2800.0f);
                hs.ft8.rx_freq_hz = hz;
                if (hs.cmd_key_down || !hs.ft8.hold_tx_freq) hs.ft8.tx_freq_hz = hz;
                return true;
            }
            case HS_FT_BAND_ACT_ROW:
            case HS_FT_RX_FREQ_ROW: {
                hamshack::DecodedFtMessage chosen;
                bool ok = false;
                {
                    std::lock_guard<std::recursive_mutex> lk(hs.ft8.mtx);
                    if (h.id == HS_FT_BAND_ACT_ROW && h.index >= 0 && h.index < static_cast<int>(hs.ft8.band_activity.size())) {
                        chosen = hs.ft8.band_activity[h.index];
                        ok = true;
                    } else if (h.id == HS_FT_RX_FREQ_ROW && h.index >= 0 && h.index < static_cast<int>(hs.ft8.rx_freq_activity.size())) {
                        chosen = hs.ft8.rx_freq_activity[h.index];
                        ok = true;
                    }
                }
                if (ok) {
                    using namespace std::chrono;
                    uint64_t now_ms = duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
                    static uint64_t s_last_click_ms = 0;
                    static int s_last_click_row = -1;
                    static int s_last_click_id = -1;

                    bool is_dbl = (now_ms - s_last_click_ms < 450) && (s_last_click_row == h.index) && (s_last_click_id == h.id);
                    s_last_click_ms = now_ms;
                    s_last_click_row = h.index;
                    s_last_click_id = h.id;

                    if (is_dbl) {
                        bool want_ptt = false;
                        hs.ft8.on_double_click_message(chosen, hs.soundcard, want_ptt);
                        if (want_ptt && hs.flrig.connected) {
                            hs.flrig.set_ptt(true);
                        }
                    } else {
                        hs.ft8.select_decoded_message(chosen);
                        hs.ft8.tx_enabled = true;
                    }
                }
                return true;
            }

            // JS8Call Controls
            case HS_JS8_SUB_NORMAL: hs.js8.current_submode = js8::Submode::NORMAL; return true;
            case HS_JS8_SUB_FAST:   hs.js8.current_submode = js8::Submode::FAST;   return true;
            case HS_JS8_SUB_TURBO:  hs.js8.current_submode = js8::Submode::TURBO;  return true;
            case HS_JS8_SUB_SLOW:   hs.js8.current_submode = js8::Submode::SLOW;   return true;
            case HS_JS8_HB_TOGGLE:  hs.js8.heartbeat_enabled = !hs.js8.heartbeat_enabled; return true;
            case HS_JS8_AUTO_TOGGLE:hs.js8.auto_reply_enabled = !hs.js8.auto_reply_enabled; return true;
            case HS_JS8_MESH_BRIDGE:hs.js8.bridge_to_meshcore = !hs.js8.bridge_to_meshcore; return true;
            case HS_JS8_DECODE_NOW: {
                auto pcm = hs.soundcard.get_recent_rx_samples(180000);
                hs.js8.decode_live_audio_slot_async(std::move(pcm));
                return true;
            }
            case HS_JS8_CLEAR_BTN: {
                std::lock_guard<std::mutex> lk(hs.js8.mtx);
                hs.js8.band_frames.clear();
                hs.js8.qso_transcript.clear();
                return true;
            }
            case HS_JS8_COMPOSER:   hs.text_focus = HsTextFocus::Js8Composer; return true;
            case HS_JS8_SEND_BTN: {
                if (!hs.js8.composer_text.empty()) {
                    std::vector<float> wave;
                    hs.js8.queue_message(hs.js8.my_call + ": " + hs.js8.selected_call + " " + hs.js8.composer_text, &wave);
                    hs.soundcard.queue_tx_samples(wave);
                    hs.flrig.set_ptt(true);
                    hs.js8.composer_text.clear();
                }
                return true;
            }
            case HS_JS8_HALT_BTN:
                hs.js8.tx_active = false;
                hs.soundcard.halt_tx();
                hs.flrig.set_ptt(false);
                return true;
            case HS_JS8_WATERFALL: {
                float norm = std::clamp((pt.x - h.rect.left) / std::max(1.0f, h.rect.right - h.rect.left), 0.0f, 1.0f);
                int hz = 100 + static_cast<int>(norm * 2800.0f);
                hs.js8.rx_offset_hz = hz;
                hs.js8.tx_offset_hz = hz;
                return true;
            }
            case HS_JS8_CMD_CQ:
            case HS_JS8_CMD_HB:
            case HS_JS8_CMD_SNR:
            case HS_JS8_CMD_INFO:
            case HS_JS8_CMD_GRID:
            case HS_JS8_CMD_STATUS:
            case HS_JS8_CMD_HEARING:
            case HS_JS8_CMD_ACK:
            case HS_JS8_CMD_73: {
                static const char* kCmdNames[] = {"CQ", "HB", "SNR?", "INFO?", "GRID?", "STATUS?", "HEARING?", "ACK", "73"};
                std::vector<float> wave;
                hs.js8.send_directed_command(kCmdNames[h.id - HS_JS8_CMD_CQ], &wave);
                hs.soundcard.queue_tx_samples(wave);
                hs.flrig.set_ptt(true);
                return true;
            }
            case HS_JS8_HEARD_ROW:
                hs.js8.selected_call = h.payload;
                return true;
            case HS_JS8_FRAME_ROW:
                hs.js8.rx_offset_hz = h.index;
                return true;

            // CW (Fldigi) Controls
            case HS_CW_WPM_M5: hs.cw.tx_wpm = std::max(5, hs.cw.tx_wpm - 5); hs.flrig.set_cw_wpm(hs.cw.tx_wpm); return true;
            case HS_CW_WPM_M1: hs.cw.tx_wpm = std::max(5, hs.cw.tx_wpm - 1); hs.flrig.set_cw_wpm(hs.cw.tx_wpm); return true;
            case HS_CW_WPM_P1: hs.cw.tx_wpm = std::min(50, hs.cw.tx_wpm + 1); hs.flrig.set_cw_wpm(hs.cw.tx_wpm); return true;
            case HS_CW_WPM_P5: hs.cw.tx_wpm = std::min(50, hs.cw.tx_wpm + 5); hs.flrig.set_cw_wpm(hs.cw.tx_wpm); return true;
            case HS_CW_PITCH_M25: hs.cw.pitch_hz = std::max(300, hs.cw.pitch_hz - 25); hs.flrig.set_cw_spot_tone(hs.cw.pitch_hz); return true;
            case HS_CW_PITCH_P25: hs.cw.pitch_hz = std::min(900, hs.cw.pitch_hz + 25); hs.flrig.set_cw_spot_tone(hs.cw.pitch_hz); return true;
            case HS_CW_BW_CYCLE: {
                static const int kBws[5] = {50, 100, 150, 250, 400};
                int next = 150;
                for (int i = 0; i < 5; ++i) if (hs.cw.filter_bw_hz == kBws[i]) { next = kBws[(i + 1) % 5]; break; }
                hs.cw.filter_bw_hz = next;
                return true;
            }
            case HS_CW_QSK_TOGGLE:  hs.cw.qsk_enabled = !hs.cw.qsk_enabled; return true;
            case HS_CW_AFC_TOGGLE:  hs.cw.afc_enabled = !hs.cw.afc_enabled; return true;
            case HS_CW_PROSIGN_TOG: hs.cw.prosigns_enabled = !hs.cw.prosigns_enabled; return true;
            case HS_CW_SQL_DOWN:    hs.cw.squelch_metric = std::clamp(hs.cw.squelch_metric - 0.05f, 0.05f, 0.90f); return true;
            case HS_CW_SQL_UP:      hs.cw.squelch_metric = std::clamp(hs.cw.squelch_metric + 0.05f, 0.05f, 0.90f); return true;
            case HS_CW_CLEAR_RX:    hs.cw.rx_decoded_text.clear(); hs.cw.last_morse_elements.clear(); return true;
            case HS_CW_COMPOSER:    hs.text_focus = HsTextFocus::CwComposer; return true;
            case HS_CW_TX_NOW: {
                if (!hs.cw.tx_buffer_text.empty()) {
                    std::vector<float> wave;
                    hs.cw.transmit_text(hs.cw.tx_buffer_text, &wave);
                    hs.soundcard.queue_tx_samples(wave);
                    hs.flrig.set_ptt(true);
                    hs.cw.tx_buffer_text.clear();
                }
                return true;
            }
            case HS_CW_WATERFALL: {
                float norm = std::clamp((pt.x - h.rect.left) / std::max(1.0f, h.rect.right - h.rect.left), 0.0f, 1.0f);
                hs.cw.pitch_hz = std::clamp(100 + static_cast<int>(norm * 2800.0f), 300, 900);
                hs.flrig.set_cw_spot_tone(hs.cw.pitch_hz);
                return true;
            }

            // FLRig & Audio Matrix Controls
            case HS_AUDIO_IN_ROW:   hs.soundcard.select_input_device(h.index); return true;
            case HS_AUDIO_OUT_ROW:  hs.soundcard.select_output_device(h.index); return true;
            case HS_AUDIO_REFRESH:  hs.soundcard.refresh_devices(); return true;
            case HS_AUDIO_MON_TOG:  hs.soundcard.monitor_tx_audio = !hs.soundcard.monitor_tx_audio; return true;
            case HS_AUDIO_TEST_TONE: {
                std::vector<float> t(12000, 0.0f);
                for (size_t i = 0; i < t.size(); ++i) t[i] = 0.5f * std::sin(2.0f * 3.14159265f * 1000.0f * i / 12000.0f);
                hs.soundcard.queue_tx_samples(t);
                return true;
            }
            case HS_CAT_POLL_VFO:    hs.flrig.send_custom_hex_cat("FE FE 88 E0 03 FD"); return true;
            case HS_CAT_POLL_SMETER: hs.flrig.send_custom_hex_cat("FE FE 88 E0 15 02 FD"); return true;
            case HS_CAT_CLEAR_LOG: {
                std::lock_guard<std::mutex> lk(hs.flrig.mtx);
                hs.flrig.cat_traces.clear();
                return true;
            }
            case HS_CAT_HEX_BOX:     hs.text_focus = HsTextFocus::CatHexInput; return true;
            case HS_CAT_HEX_SEND:    hs.flrig.send_custom_hex_cat(hs.custom_cat_hex); return true;

            // Web Pages Controls (QRZ, PSKReporter, DXView HF, DXLook, OpenHamClock)
            case HS_WEB_BACK:     hamshack_web::get_web_maps().go_back(hs.active_page_subtab); return true;
            case HS_WEB_FWD:      hamshack_web::get_web_maps().go_forward(hs.active_page_subtab); return true;
            case HS_WEB_RELOAD:   hamshack_web::get_web_maps().reload(hs.active_page_subtab); return true;
            case HS_WEB_HOME:     hamshack_web::get_web_maps().load_url(hs.active_page_subtab, hamshack_web::kWebMapHomeUrls[hs.active_page_subtab]); return true;
            case HS_PAGE_BOOKMARK:hamshack_web::get_web_maps().bookmark_current_page(hs.active_page_subtab); return true;
            case HS_PAGE_URL_BOX:
                hs.page_url_input = hamshack_web::get_web_maps().current_url(hs.active_page_subtab);
                hs.text_focus = HsTextFocus::PageUrl;
                return true;
            case HS_PAGE_ZOOM_IN:    hamshack_web::get_web_maps().zoom_in(hs.active_page_subtab); return true;
            case HS_PAGE_ZOOM_OUT:   hamshack_web::get_web_maps().zoom_out(hs.active_page_subtab); return true;
            case HS_PAGE_ZOOM_RESET: hamshack_web::get_web_maps().zoom_reset(hs.active_page_subtab); return true;
            case HS_WEB_EXTERNAL: hamshack_web::get_web_maps().open_in_external_browser(hs.active_page_subtab); return true;

            // Configuration Exports
            case HS_CONFIG_EXPORT_CSV: {
                std::string p = hs.ft8.export_qso_csv();
                hs.config_export_status = "Exported: " + p;
                app.log_public_system("[CONFIG EXPORT] " + hs.config_export_status);
                return true;
            }
            case HS_CONFIG_EXPORT_KML: {
                std::string p = hs.ft8.export_contacts_kml();
                hs.config_export_status = "Exported: " + p;
                app.log_public_system("[CONFIG EXPORT] " + hs.config_export_status);
                return true;
            }
            case HS_CONFIG_EXPORT_ADI: {
                std::string p = hs.ft8.export_adif_log();
                hs.config_export_status = "Exported: " + p;
                app.log_public_system("[CONFIG EXPORT] " + hs.config_export_status);
                return true;
            }
            case HS_CFG_WEB_REPORT_TOGGLE: {
                bool current = hamshack::get_psk_reporter().is_enabled();
                hamshack::get_psk_reporter().set_enabled(!current);
                app.log_public_system(std::string("[WEB REPORTING] ") + (!current ? "ENABLED - Uploading spots to report.pskreporter.info" : "DISABLED - Reporting paused"));
                return true;
            }

            // Twitch Live Streaming Studio Controls
            case HS_TWITCH_STREAM_TOGGLE: {
                auto& cfg = hamshack::get_app_config();
                hs.twitch.channel_name = cfg.twitch_channel;
                hs.twitch.stream_key = cfg.twitch_stream_key;
                hs.twitch.rtmp_url = cfg.twitch_rtmp_url;
                if (!hs.twitch.is_streaming && hs.twitch.stream_key.empty()) {
                    hs.twitch.start_browser_login();
                } else {
                    hs.twitch.toggle_stream();
                }
                return true;
            }
            case HS_TWITCH_LOGIN_WEB: {
                if (hs.twitch.auth_in_progress.load()) {
                    hs.twitch.cancel_browser_login();
                } else {
                    hs.twitch.start_browser_login();
                }
                return true;
            }
            case HS_TWITCH_DASHBOARD_WEB: {
                auto& cfg = hamshack::get_app_config();
                hs.twitch.channel_name = cfg.twitch_channel;
                hs.twitch.open_twitch_dashboard();
                return true;
            }
            case HS_TWITCH_CHANNEL_BOX: {
                hs.text_focus = HsTextFocus::TwitchChannel;
                return true;
            }
            case HS_TWITCH_KEY_BOX: {
                hs.text_focus = HsTextFocus::TwitchStreamKey;
                return true;
            }

            // Native Libera.Chat IRC Controls
            case HS_IRC_CONNECT_BTN: {
                auto& cfg = hamshack::get_app_config();
                hs.irc.connect_async(cfg.irc_server, cfg.irc_port, hs.irc_nick_edit);
                return true;
            }
            case HS_IRC_DISCONN_BTN: {
                hs.irc.disconnect();
                return true;
            }
            case HS_IRC_IDENT_BTN: {
                auto& cfg = hamshack::get_app_config();
                auto it = cfg.nickserv_passwords.find(hs.irc_nick_edit);
                if (it != cfg.nickserv_passwords.end() && !it->second.empty()) {
                    hs.irc.handle_user_slash_command("/identify " + it->second);
                } else {
                    hs.irc.append_message("Status", "***", "No saved password for '" + hs.irc_nick_edit + "'. Type: /identify <your_password>", 0xF59E0B, true);
                }
                return true;
            }
            case HS_IRC_NICK_BOX:
                hs.text_focus = HsTextFocus::IrcNick;
                return true;
            case HS_IRC_INPUT_BOX:
                hs.text_focus = HsTextFocus::IrcInput;
                return true;
            case HS_IRC_SEND_BTN: {
                if (!hs.irc_input_text.empty()) {
                    std::string tgt = "Status";
                    {
                        std::lock_guard<std::recursive_mutex> lk(hs.irc.mtx);
                        if (hs.irc.active_chan_idx >= 0 && hs.irc.active_chan_idx < static_cast<int>(hs.irc.channels.size())) {
                            tgt = hs.irc.channels[hs.irc.active_chan_idx].name;
                        }
                    }
                    hs.irc.send_chat_message(tgt, hs.irc_input_text);
                    hs.irc_input_text.clear();
                }
                return true;
            }
            case HS_IRC_SASL_TOGGLE: {
                auto& cfg = hamshack::get_app_config();
                cfg.irc_use_sasl = !cfg.irc_use_sasl;
                cfg.save();
                hs.irc.append_message("Status", "***", std::string("SASL authentication ") + (cfg.irc_use_sasl ? "ENABLED." : "DISABLED."), 0x38BDF8, true);
                return true;
            }
            case HS_IRC_ZOOM_IN: {
                hs.irc_font_size = std::min(28.0f, hs.irc_font_size + 1.0f);
                auto& cfg = hamshack::get_app_config();
                cfg.irc_font_size = hs.irc_font_size;
                cfg.save();
                return true;
            }
            case HS_IRC_ZOOM_OUT: {
                hs.irc_font_size = std::max(9.0f, hs.irc_font_size - 1.0f);
                auto& cfg = hamshack::get_app_config();
                cfg.irc_font_size = hs.irc_font_size;
                cfg.save();
                return true;
            }
            case HS_IRC_ZOOM_RESET: {
                hs.irc_font_size = 13.0f;
                auto& cfg = hamshack::get_app_config();
                cfg.irc_font_size = 13.0f;
                cfg.save();
                return true;
            }

            // Winlink / Email Client Controls
            case HS_WL_SUBTAB_MAILBOX:   hs.active_winlink_subtab = 0; return true;
            case HS_WL_SUBTAB_GATEWAYS:  hs.active_winlink_subtab = 1; return true;
            case HS_WL_SUBTAB_CONFIG:    hs.active_winlink_subtab = 2; return true;
            case HS_WL_FOLDER_INBOX:     hs.winlink.active_folder = 0; hs.winlink.selected_msg_idx = 0; hs.winlink.is_composing = false; return true;
            case HS_WL_FOLDER_OUTBOX:    hs.winlink.active_folder = 1; hs.winlink.selected_msg_idx = 0; hs.winlink.is_composing = false; return true;
            case HS_WL_FOLDER_SENT:      hs.winlink.active_folder = 2; hs.winlink.selected_msg_idx = 0; hs.winlink.is_composing = false; return true;
            case HS_WL_FOLDER_ARCHIVE:   hs.winlink.active_folder = 3; hs.winlink.selected_msg_idx = 0; hs.winlink.is_composing = false; return true;
            case HS_WL_COMPOSE_BTN:      hs.winlink.is_composing = !hs.winlink.is_composing; hs.text_focus = hs.winlink.is_composing ? HsTextFocus::WlCompTo : HsTextFocus::None; return true;
            case HS_WL_DELETE_MSG:       hs.winlink.delete_message(hs.winlink.active_folder, hs.winlink.selected_msg_idx); return true;
            case HS_WL_COMP_TO_BOX:      hs.text_focus = HsTextFocus::WlCompTo; return true;
            case HS_WL_COMP_CC_BOX:      hs.text_focus = HsTextFocus::WlCompCc; return true;
            case HS_WL_COMP_SUBJ_BOX:    hs.text_focus = HsTextFocus::WlCompSubj; return true;
            case HS_WL_COMP_BODY_BOX:    hs.text_focus = HsTextFocus::WlCompBody; return true;
            case HS_WL_ATTACH_ADD_BTN: {
                std::string p = hamshack_web::pick_attachment_file_macos();
                if (!p.empty()) {
                    hs.winlink.add_compose_attachment(p);
                    app.log_public_system("[WINLINK] Attached file: " + p);
                }
                return true;
            }
            case HS_WL_ATTACH_CLR_BTN:   hs.winlink.compose_attachments.clear(); return true;
            case HS_WL_QUEUE_OUTBOX: {
                auto& cfg = hamshack::get_app_config();
                if (hs.winlink.post_compose_to_outbox(cfg.winlink_callsign)) {
                    app.log_public_system("[WINLINK] Queued new message to Outbox for transmission.");
                }
                return true;
            }
            case HS_WL_CANCEL_COMP:      hs.winlink.is_composing = false; return true;
            case HS_WL_EXEC_WEB: {
                if (!hs.winlink.session_active) {
                    auto& cfg = hamshack::get_app_config();
                    cfg.winlink_transport = 0; // Telnet
                    cfg.save();
                    hs.winlink.start_session("Telnet", cfg.winlink_callsign, cfg.winlink_password, cfg.winlink_grid,
                                             "CMS", 0);
                    app.log_public_system("[WINLINK] Initiating email session via Web / CMS Telnet to cms.winlink.org:8772");
                } else {
                    hs.winlink.abort_session();
                    app.log_public_system("[WINLINK] Aborted email session.");
                }
                return true;
            }
            case HS_WL_EXEC_RADIO: {
                if (!hs.winlink.session_active) {
                    auto& cfg = hamshack::get_app_config();
                    cfg.winlink_transport = 1; // ARDOP
                    cfg.save();
                    if (hs.winlink.selected_gateway_call.empty() && !hs.winlink.filtered_gateways.empty()) {
                        hs.winlink.selected_gateway_call = hs.winlink.filtered_gateways[0].callsign;
                        hs.winlink.selected_gateway_freq = hs.winlink.filtered_gateways[0].freq_hz;
                        hs.winlink.selected_gateway_mode = hs.winlink.filtered_gateways[0].mode;
                        hs.winlink.selected_gateway_grid = hs.winlink.filtered_gateways[0].grid;
                    }
                    if (hs.flrig.connected && hs.winlink.selected_gateway_freq > 0) {
                        hs.flrig.set_vfo_a(hs.winlink.selected_gateway_freq);
                        hs.flrig.set_mode(hamshack::RigMode::USB);
                    }
                    std::string gw = hs.winlink.selected_gateway_call.empty() ? "CMS" : hs.winlink.selected_gateway_call;
                    hs.winlink.start_session("ARDOP", cfg.winlink_callsign, cfg.winlink_password, cfg.winlink_grid,
                                             gw, hs.winlink.selected_gateway_freq);
                    app.log_public_system("[WINLINK] Initiating email session via ARDOP HF radio to " + gw);
                } else {
                    hs.winlink.abort_session();
                    app.log_public_system("[WINLINK] Aborted email session.");
                }
                return true;
            }
            case HS_WL_EXEC_SESSION: {
                auto& cfg = hamshack::get_app_config();
                if (cfg.winlink_transport == 1) {
                    if (!hs.winlink.session_active) {
                        if (hs.winlink.selected_gateway_call.empty() && !hs.winlink.filtered_gateways.empty()) {
                            hs.winlink.selected_gateway_call = hs.winlink.filtered_gateways[0].callsign;
                            hs.winlink.selected_gateway_freq = hs.winlink.filtered_gateways[0].freq_hz;
                            hs.winlink.selected_gateway_mode = hs.winlink.filtered_gateways[0].mode;
                            hs.winlink.selected_gateway_grid = hs.winlink.filtered_gateways[0].grid;
                        }
                        if (hs.flrig.connected && hs.winlink.selected_gateway_freq > 0) {
                            hs.flrig.set_vfo_a(hs.winlink.selected_gateway_freq);
                            hs.flrig.set_mode(hamshack::RigMode::USB);
                        }
                        std::string gw = hs.winlink.selected_gateway_call.empty() ? "CMS" : hs.winlink.selected_gateway_call;
                        hs.winlink.start_session("ARDOP", cfg.winlink_callsign, cfg.winlink_password, cfg.winlink_grid,
                                                 gw, hs.winlink.selected_gateway_freq);
                        app.log_public_system("[WINLINK] Initiating email session via ARDOP HF radio to " + gw);
                    } else {
                        hs.winlink.abort_session();
                        app.log_public_system("[WINLINK] Aborted email session.");
                    }
                } else {
                    if (!hs.winlink.session_active) {
                        hs.winlink.start_session("Telnet", cfg.winlink_callsign, cfg.winlink_password, cfg.winlink_grid,
                                                 "CMS", 0);
                        app.log_public_system("[WINLINK] Initiating email session via Web / CMS Telnet to cms.winlink.org:8772");
                    } else {
                        hs.winlink.abort_session();
                        app.log_public_system("[WINLINK] Aborted email session.");
                    }
                }
                return true;
            }
            case HS_WL_ABORT_SESSION:    hs.winlink.abort_session(); return true;
            case HS_WL_GW_SEARCH_BOX:    hs.text_focus = HsTextFocus::WlGwSearch; return true;
            case HS_WL_GW_MODE_ALL:      hs.winlink.filter_gateways(hs.winlink.gateway_search_query, "All", hs.hf_grid); return true;
            case HS_WL_GW_MODE_ARDOP:    hs.winlink.filter_gateways(hs.winlink.gateway_search_query, "ARDOP", hs.hf_grid); return true;
            case HS_WL_GW_MODE_VARA:     hs.winlink.filter_gateways(hs.winlink.gateway_search_query, "VARA", hs.hf_grid); return true;
            case HS_WL_GW_MODE_PACKET:   hs.winlink.filter_gateways(hs.winlink.gateway_search_query, "Packet", hs.hf_grid); return true;
            case HS_WL_GW_MODE_PACTOR:   hs.winlink.filter_gateways(hs.winlink.gateway_search_query, "Pactor", hs.hf_grid); return true;
            case HS_WL_CFG_CALL_BOX:     hs.text_focus = HsTextFocus::WlCfgCall; return true;
            case HS_WL_CFG_PASS_BOX:     hs.text_focus = HsTextFocus::WlCfgPass; return true;
            case HS_WL_CFG_GRID_BOX:     hs.text_focus = HsTextFocus::WlCfgGrid; return true;
            case HS_WL_CFG_TRANS_TELNET: {
                auto& cfg = hamshack::get_app_config();
                cfg.winlink_transport = 0;
                cfg.save();
                return true;
            }
            case HS_WL_CFG_TRANS_ARDOP: {
                auto& cfg = hamshack::get_app_config();
                cfg.winlink_transport = 1;
                cfg.save();
                return true;
            }
            case HS_WL_CFG_TRANS_PACKET: {
                auto& cfg = hamshack::get_app_config();
                cfg.winlink_transport = 2;
                cfg.save();
                return true;
            }
            case HS_WL_CFG_BW_CYCLE: {
                auto& cfg = hamshack::get_app_config();
                if (cfg.winlink_ardop_bandwidth == 500) cfg.winlink_ardop_bandwidth = 1000;
                else if (cfg.winlink_ardop_bandwidth == 1000) cfg.winlink_ardop_bandwidth = 2000;
                else cfg.winlink_ardop_bandwidth = 500;
                cfg.save();
                return true;
            }
            case HS_WL_CFG_SAVE_BTN: {
                auto& cfg = hamshack::get_app_config();
                cfg.save();
                app.log_public_system("[WINLINK CONFIG] Saved configuration.");
                return true;
            }

            // GridTracker Native Map & Sidebar Controls
            case HS_GT_IMPORT_ADI: {
                std::string p = hamshack_web::pick_adif_file_macos();
                if (!p.empty()) {
                    hs.ft8.import_adif_file(p);
                    app.log_public_system("[GRIDTRACKER] Imported ADIF log " + p + ": " + hs.ft8.adif_status_text);
                }
                return true;
            }
            case HS_GT_SYNC_WSJTX_ADI:
                hs.ft8.auto_import_wsjtx_adif();
                app.log_public_system("[GRIDTRACKER] Synced WSJT-X ADIF log: " + hs.ft8.adif_status_text);
                return true;
            case HS_GT_ROSTER_TOGGLE:   hs.gt_show_roster = !hs.gt_show_roster; return true;
            case HS_GT_GRIDS_TOGGLE:    hs.gt_show_grids = !hs.gt_show_grids; return true;
            case HS_GT_ARCS_TOGGLE:     hs.gt_show_arcs = !hs.gt_show_arcs; return true;
            case HS_GT_GRAYLINE_TOGGLE: hs.gt_show_grayline = !hs.gt_show_grayline; return true;
            case HS_GT_PINS_TOGGLE:     hs.gt_show_pins = !hs.gt_show_pins; return true;
            case HS_GT_LEGEND_TOGGLE:   hs.gt_show_legend = !hs.gt_show_legend; return true;
            case HS_GT_FIT_WORLD:       hs.gt_map_nx = 0.5; hs.gt_map_ny = 0.5; hs.gt_map_zoom = 2; return true;
            case HS_GT_CENTER_QTH:      hs.center_gt_map_on_qth(4); return true;
            case HS_GT_CLEAR_LIVE: {
                std::lock_guard<std::recursive_mutex> lk1(hs.ft8.mtx);
                hs.ft8.band_activity.clear();
                hs.ft8.rx_freq_activity.clear();
                std::lock_guard<std::mutex> lk2(hs.js8.mtx);
                hs.js8.band_frames.clear();
                hs.js8.heard_stations.clear();
                return true;
            }
            case HS_GT_LOG_QSO:
                hs.log_qso_dialog.open_for_current_ft8(hs.ft8, hs.flrig.connected ? hs.flrig.vfo_a_hz : 0ULL);
                hs.text_focus = HsTextFocus::LogQsoComm;
                return true;
            case HS_GT_FILT_BAND:       hs.gt_band_filter = (hs.gt_band_filter + 1) % 2; return true;
            case HS_GT_FILT_MODE:       hs.gt_mode_filter = (hs.gt_mode_filter + 1) % 3; return true;
            case HS_GT_FILT_DATA:       hs.gt_data_filter = (hs.gt_data_filter + 1) % 3; return true;
            case HS_GT_FILT_AWARD:      hs.gt_award_filter = (hs.gt_award_filter + 1) % 3; return true;
            case HS_GT_ZOOM_IN:         hs.gt_map_zoom = std::min(10, hs.gt_map_zoom + 1); return true;
            case HS_GT_ZOOM_OUT:        hs.gt_map_zoom = std::max(2, hs.gt_map_zoom - 1); return true;
            case HS_GT_MAP_CANVAS:
                hs.gt_map_dragging = true;
                hs.gt_map_drag_start.x = pt.x;
                hs.gt_map_drag_start.y = pt.y;
                hs.gt_map_drag_nx = hs.gt_map_nx;
                hs.gt_map_drag_ny = hs.gt_map_ny;
                return true;
            case HS_GT_ROSTER_ROW:
                if (!h.payload.empty()) {
                    hs.ft8.dx_call = h.payload;
                    hs.sync_hf_station();
                    hs.ft8.regenerate_std_msgs();
                }
                return true;
            default: break;
        }

        // Page Sub-Tabs (HS_PAGE_SUBTAB_BASE .. HS_PAGE_SUBTAB_BASE + 4)
        if (h.id >= HS_PAGE_SUBTAB_BASE && h.id <= HS_PAGE_SUBTAB_BASE + 4) {
            hs.active_page_subtab = h.id - HS_PAGE_SUBTAB_BASE;
            return true;
        }

        // IRC Channel Tabs (HS_IRC_CHAN_BASE .. HS_IRC_CHAN_BASE + 100)
        if (h.id >= HS_IRC_CHAN_BASE && h.id <= HS_IRC_CHAN_BASE + 100) {
            int ch_idx = h.id - HS_IRC_CHAN_BASE;
            if (ch_idx == 99) {
                hs.irc.handle_user_slash_command("/join #hamradio");
            } else {
                std::lock_guard<std::recursive_mutex> lk(hs.irc.mtx);
                if (ch_idx >= 0 && ch_idx < static_cast<int>(hs.irc.channels.size())) {
                    hs.irc.active_chan_idx = ch_idx;
                    hs.irc.channels[ch_idx].unread_count = 0;
                }
            }
            return true;
        }

        // IRC User Roster Rows (HS_IRC_USER_ROW_BASE .. HS_IRC_USER_ROW_BASE + 60)
        if (h.id >= HS_IRC_USER_ROW_BASE && h.id <= HS_IRC_USER_ROW_BASE + 60) {
            hs.irc_menu.is_open = true;
            hs.irc_menu.x = static_cast<float>(pt.x);
            hs.irc_menu.y = static_cast<float>(pt.y);
            hs.irc_menu.nick = h.payload;
            hs.irc_menu.prefix = static_cast<char>(h.index);
            std::string cur_ch;
            {
                std::lock_guard<std::recursive_mutex> lk(hs.irc.mtx);
                if (hs.irc.active_chan_idx >= 0 && hs.irc.active_chan_idx < static_cast<int>(hs.irc.channels.size())) {
                    cur_ch = hs.irc.channels[hs.irc.active_chan_idx].name;
                }
            }
            hs.irc_menu.channel = cur_ch;
            return true;
        }

        // Winlink Message Row clicks
        if (h.id >= HS_WL_MSG_ROW_BASE && h.id < HS_WL_MSG_ROW_BASE + 200) {
            hs.winlink.selected_msg_idx = h.index;
            return true;
        }

        // Winlink RMS Gateway Row clicks -> Auto tune Xiegu G90 CAT VFO!
        if (h.id >= HS_WL_GW_ROW_BASE && h.id < HS_WL_GW_ROW_BASE + 5000) {
            hs.winlink.select_gateway(static_cast<size_t>(h.index));
            if (hs.winlink.selected_gateway_freq > 0) {
                if (hs.flrig.connected) {
                    hs.flrig.set_vfo_a(hs.winlink.selected_gateway_freq);
                    hs.flrig.set_mode(hamshack::RigMode::USB);
                    app.log_public_system("[WINLINK CAT] Tuned Xiegu G90 to " +
                                          hamshack::FlrigCatEngine::format_freq_mhz(hs.winlink.selected_gateway_freq) +
                                          " MHz USB for RMS Gateway " + hs.winlink.selected_gateway_call);
                }
                auto& cfg = hamshack::get_app_config();
                cfg.winlink_selected_gateway = hs.winlink.selected_gateway_call;
                cfg.winlink_selected_freq = hs.winlink.selected_gateway_freq;
                cfg.winlink_selected_mode = hs.winlink.selected_gateway_mode;
                cfg.save();
            }
            return true;
        }

        // Band buttons (HS_BAND_BASE .. HS_BAND_BASE + 10)
        if (h.id >= HS_BAND_BASE && h.id <= HS_BAND_BASE + 10) {
            int b = h.id - HS_BAND_BASE;
            int op = (hs.active_mode_tab == 0) ? (hs.ft8.is_ft4 ? 1 : 0)
                   : (hs.active_mode_tab == 1) ? 2
                   : (hs.active_mode_tab == 2) ? 3 : 0;
            hs.flrig.select_band_for_mode(b, op);
            return true;
        }
        // FT8 Tx1..Tx6 Slot Buttons
        if (h.id >= HS_FT_TX_SLOT_BASE && h.id <= HS_FT_TX_SLOT_BASE + 5) {
            int slot = h.id - HS_FT_TX_SLOT_BASE;
            if (slot == 5 && hs.ft8.active_tx_slot == 5) {
                hs.ft8.cycle_cq_modifier();
            }
            hs.ft8.start_sequence(slot);
            return true;
        }
        // CW F1..F12 Macro Buttons
        if (h.id >= HS_CW_MACRO_BASE && h.id <= HS_CW_MACRO_BASE + 11) {
            std::string expanded = hs.cw.expand_macro(h.id - HS_CW_MACRO_BASE);
            std::vector<float> wave;
            hs.cw.transmit_text(expanded, &wave);
            hs.soundcard.queue_tx_samples(wave);
            hs.flrig.set_ptt(true);
            return true;
        }
        // G90 Memory Channel Bank Buttons
        if (h.id >= HS_MEM_CH_BASE && h.id < HS_MEM_CH_BASE + 20) {
            int idx = h.id - HS_MEM_CH_BASE;
            if (idx >= 0 && idx < static_cast<int>(hs.flrig.memories.size())) {
                const auto& m = hs.flrig.memories[idx];
                hs.flrig.set_vfo_a(m.freq_hz);
                hs.flrig.set_mode(m.mode, m.filter);
            }
            return true;
        }
    }
    return false;
}

template <typename AppT>
inline bool handle_hamshack_double_click(AppT& /*app*/, HamShackSuite& hs, POINT pt) {
    if (hs.log_qso_dialog.is_open) return false;

    for (const auto& h : hs.hits) {
        if (pt.x < h.rect.left || pt.x > h.rect.right || pt.y < h.rect.top || pt.y > h.rect.bottom) continue;

        if (h.id == HS_FT_BAND_ACT_ROW || h.id == HS_FT_RX_FREQ_ROW) {
            hamshack::DecodedFtMessage chosen;
            bool ok = false;
            {
                std::lock_guard<std::recursive_mutex> lk(hs.ft8.mtx);
                if (h.id == HS_FT_BAND_ACT_ROW && h.index >= 0 && h.index < static_cast<int>(hs.ft8.band_activity.size())) {
                    chosen = hs.ft8.band_activity[h.index];
                    ok = true;
                } else if (h.id == HS_FT_RX_FREQ_ROW && h.index >= 0 && h.index < static_cast<int>(hs.ft8.rx_freq_activity.size())) {
                    chosen = hs.ft8.rx_freq_activity[h.index];
                    ok = true;
                }
            }
            if (ok) {
                bool want_ptt = false;
                hs.ft8.on_double_click_message(chosen, hs.soundcard, want_ptt);
                if (want_ptt && hs.flrig.connected) {
                    hs.flrig.set_ptt(true);
                }
                return true;
            }
        }
    }
    return false;
}

inline bool handle_hamshack_key_char(HamShackSuite& hs, char c) {
    if (c == 27) { // Escape key closes Log QSO modal
        if (hs.log_qso_dialog.is_open) {
            hs.log_qso_dialog.is_open = false;
            hs.text_focus = HsTextFocus::None;
            return true;
        }
    }
    if (c == '\t') { // Tab cycles Log QSO dialog fields
        if (hs.log_qso_dialog.is_open) {
            switch (hs.text_focus) {
                case HsTextFocus::LogQsoCall: hs.text_focus = HsTextFocus::LogQsoGrid; break;
                case HsTextFocus::LogQsoGrid: hs.text_focus = HsTextFocus::LogQsoMode; break;
                case HsTextFocus::LogQsoMode: hs.text_focus = HsTextFocus::LogQsoFreq; break;
                case HsTextFocus::LogQsoFreq: hs.text_focus = HsTextFocus::LogQsoRptS; break;
                case HsTextFocus::LogQsoRptS: hs.text_focus = HsTextFocus::LogQsoRptR; break;
                case HsTextFocus::LogQsoRptR: hs.text_focus = HsTextFocus::LogQsoPwr;  break;
                case HsTextFocus::LogQsoPwr:  hs.text_focus = HsTextFocus::LogQsoComm; break;
                case HsTextFocus::LogQsoComm: hs.text_focus = HsTextFocus::LogQsoCall; break;
                default: hs.text_focus = HsTextFocus::LogQsoCall; break;
            }
            return true;
        }
    }

    if (hs.text_focus == HsTextFocus::None) return false;
    auto& cfg = hamshack::get_app_config();
    std::string* target = nullptr;
    size_t max_len = 120;
    bool force_upper = true;

    switch (hs.text_focus) {
        case HsTextFocus::Ft8DxCall:   target = &hs.ft8.dx_call; max_len = 12; break;
        case HsTextFocus::Ft8DxGrid:   target = &hs.ft8.dx_grid; max_len = 6;  break;
        case HsTextFocus::Js8Composer: target = &hs.js8.composer_text; max_len = 96; break;
        case HsTextFocus::CwComposer:  target = &hs.cw.tx_buffer_text; max_len = 120; break;
        case HsTextFocus::CatHexInput: target = &hs.custom_cat_hex; max_len = 64; break;
        case HsTextFocus::HfCallsign:  target = &hs.hf_callsign; max_len = 12; break;
        case HsTextFocus::HfGrid:      target = &hs.hf_grid; max_len = 6; break;
        case HsTextFocus::IrcNick:     target = &hs.irc_nick_edit; max_len = 24; force_upper = false; break;
        case HsTextFocus::IrcInput:    target = &hs.irc_input_text; max_len = 400; force_upper = false; break;
        case HsTextFocus::PageUrl:     target = &hs.page_url_input; max_len = 250; force_upper = false; break;
        case HsTextFocus::WlCompTo:    target = &hs.winlink.compose_to; max_len = 100; force_upper = false; break;
        case HsTextFocus::WlCompCc:    target = &hs.winlink.compose_cc; max_len = 100; force_upper = false; break;
        case HsTextFocus::WlCompSubj:  target = &hs.winlink.compose_subject; max_len = 120; force_upper = false; break;
        case HsTextFocus::WlCompBody:  target = &hs.winlink.compose_body; max_len = 8192; force_upper = false; break;
        case HsTextFocus::WlGwSearch:  target = &hs.winlink.gateway_search_query; max_len = 32; force_upper = true; break;
        case HsTextFocus::WlCfgCall:   target = &cfg.winlink_callsign; max_len = 12; force_upper = true; break;
        case HsTextFocus::WlCfgPass:   target = &cfg.winlink_password; max_len = 32; force_upper = false; break;
        case HsTextFocus::WlCfgGrid:   target = &cfg.winlink_grid; max_len = 6; force_upper = true; break;
        case HsTextFocus::LogQsoCall:  target = &hs.log_qso_dialog.call; max_len = 12; force_upper = true; break;
        case HsTextFocus::LogQsoGrid:  target = &hs.log_qso_dialog.grid; max_len = 6;  force_upper = true; break;
        case HsTextFocus::LogQsoMode:  target = &hs.log_qso_dialog.mode; max_len = 8;  force_upper = true; break;
        case HsTextFocus::LogQsoRptS:  target = &hs.log_qso_dialog.rpt_sent; max_len = 6; force_upper = false; break;
        case HsTextFocus::LogQsoRptR:  target = &hs.log_qso_dialog.rpt_rcvd; max_len = 6; force_upper = false; break;
        case HsTextFocus::LogQsoFreq:  target = &hs.log_qso_dialog.freq_str; max_len = 12; force_upper = false; break;
        case HsTextFocus::LogQsoPwr:   target = &hs.log_qso_dialog.power_w; max_len = 6; force_upper = false; break;
        case HsTextFocus::LogQsoComm:  target = &hs.log_qso_dialog.comments; max_len = 120; force_upper = false; break;
        case HsTextFocus::TwitchChannel:   target = &cfg.twitch_channel; max_len = 32; force_upper = false; break;
        case HsTextFocus::TwitchStreamKey: target = &cfg.twitch_stream_key; max_len = 64; force_upper = false; break;
        case HsTextFocus::AprsHost:        target = &hs.aprs_host_edit; max_len = 64; force_upper = false; break;
        case HsTextFocus::AprsPort:        target = &hs.aprs_port_edit; max_len = 6;  force_upper = false; break;
        case HsTextFocus::AprsPath:        target = &hs.aprs_path_edit; max_len = 40; force_upper = true;  break;
        case HsTextFocus::AprsComment:     target = &hs.aprs_comment_edit; max_len = 43; force_upper = false; break;
        case HsTextFocus::AprsMsgTo:       target = &hs.aprs_msg_to_edit; max_len = 9; force_upper = true; break;
        case HsTextFocus::AprsMsgText:     target = &hs.aprs_msg_text_edit; max_len = 67; force_upper = false; break;
        case HsTextFocus::SstvOverlay:     target = &hs.sstv_overlay_edit; max_len = 40; force_upper = false; break;
        default: return false;
    }
    if (!target) return false;

    if (c == '\b' || c == 127) {
        if (!target->empty()) target->pop_back();
    } else if (c == '\r' || c == '\n') {
        if (hs.log_qso_dialog.is_open) {
            hs.log_qso_dialog.save_and_commit(hs.ft8, hs.flrig.connected ? hs.flrig.vfo_a_hz : 0ULL);
            hs.log_qso_dialog.is_open = false;
            hs.text_focus = HsTextFocus::None;
            return true;
        } else if (hs.text_focus == HsTextFocus::Js8Composer && !hs.js8.composer_text.empty()) {
            std::vector<float> wave;
            hs.js8.queue_message(hs.js8.my_call + ": " + hs.js8.selected_call + " " + hs.js8.composer_text, &wave);
            hs.soundcard.queue_tx_samples(wave);
            hs.flrig.set_ptt(true);
            hs.js8.composer_text.clear();
        } else if (hs.text_focus == HsTextFocus::CwComposer && !hs.cw.tx_buffer_text.empty()) {
            std::vector<float> wave;
            hs.cw.transmit_text(hs.cw.tx_buffer_text, &wave);
            hs.soundcard.queue_tx_samples(wave);
            hs.flrig.set_ptt(true);
            hs.cw.tx_buffer_text.clear();
        } else if (hs.text_focus == HsTextFocus::CatHexInput) {
            hs.flrig.send_custom_hex_cat(hs.custom_cat_hex);
        } else if (hs.text_focus == HsTextFocus::IrcNick) {
            hs.irc.change_nickname(hs.irc_nick_edit);
            hs.text_focus = HsTextFocus::None;
        } else if (hs.text_focus == HsTextFocus::IrcInput) {
            if (!hs.irc_input_text.empty()) {
                std::string tgt = "Status";
                {
                    std::lock_guard<std::recursive_mutex> lk(hs.irc.mtx);
                    if (hs.irc.active_chan_idx >= 0 && hs.irc.active_chan_idx < static_cast<int>(hs.irc.channels.size())) {
                        tgt = hs.irc.channels[hs.irc.active_chan_idx].name;
                    }
                }
                hs.irc.send_chat_message(tgt, hs.irc_input_text);
                hs.irc_input_text.clear();
            }
        } else if (hs.text_focus == HsTextFocus::PageUrl) {
            if (!hs.page_url_input.empty()) {
                std::string u = hs.page_url_input;
                if (u.find("://") == std::string::npos) {
                    u = "https://" + u;
                }
                hamshack_web::get_web_maps().load_url(hs.active_page_subtab, u.c_str());
                auto& cfg = hamshack::get_app_config();
                if (hs.active_page_subtab >= 0 && hs.active_page_subtab < 5) {
                    cfg.bookmarked_urls[hs.active_page_subtab] = u;
                    cfg.save();
                }
                hs.text_focus = HsTextFocus::None;
            }
        } else if (hs.text_focus == HsTextFocus::WlCompBody) {
            hs.winlink.compose_body.push_back('\n');
        } else if (hs.text_focus == HsTextFocus::WlGwSearch) {
            auto& cfg = hamshack::get_app_config();
            hs.winlink.filter_gateways(hs.winlink.gateway_search_query, hs.winlink.gateway_mode_filter, cfg.winlink_grid);
            hs.text_focus = HsTextFocus::None;
        } else if (hs.text_focus == HsTextFocus::WlCfgCall || hs.text_focus == HsTextFocus::WlCfgPass || hs.text_focus == HsTextFocus::WlCfgGrid) {
            auto& cfg = hamshack::get_app_config();
            if (hs.text_focus == HsTextFocus::WlCfgGrid) {
                hs.hf_grid = cfg.winlink_grid;
                cfg.hf_grid = cfg.winlink_grid;
                hs.sync_hf_station();
            } else if (hs.text_focus == HsTextFocus::WlCfgCall) {
                hs.hf_callsign = cfg.winlink_callsign;
                cfg.hf_callsign = cfg.winlink_callsign;
                hs.sync_hf_station();
            }
            cfg.save();
            hs.text_focus = HsTextFocus::None;
        } else if (hs.text_focus == HsTextFocus::HfCallsign || hs.text_focus == HsTextFocus::HfGrid) {
            hs.text_focus = HsTextFocus::None;
            auto& cfg = hamshack::get_app_config();
            cfg.hf_callsign = hs.hf_callsign;
            cfg.hf_grid = hs.hf_grid;
            cfg.winlink_callsign = hs.hf_callsign;
            cfg.winlink_grid = hs.hf_grid;
            hs.sync_hf_station();
            cfg.save();
        } else if (hs.text_focus == HsTextFocus::TwitchChannel || hs.text_focus == HsTextFocus::TwitchStreamKey) {
            auto& cfg = hamshack::get_app_config();
            hs.twitch.channel_name = cfg.twitch_channel;
            hs.twitch.stream_key = cfg.twitch_stream_key;
            cfg.save();
            hs.text_focus = HsTextFocus::None;
        } else if (hs.text_focus == HsTextFocus::AprsMsgText) {
            if (hs.aprs && !hs.aprs_msg_to_edit.empty() && !hs.aprs_msg_text_edit.empty()) {
                hs.aprs->set_station_identity(hs.hf_callsign, static_cast<uint8_t>(hs.aprs_ssid), hs.aprs_path_edit);
                hs.aprs->send_text_message(hs.aprs_msg_to_edit, hs.aprs_msg_text_edit);
                hs.aprs_msg_text_edit.clear();
            }
        } else if (hs.text_focus == HsTextFocus::AprsMsgTo) {
            hs.text_focus = HsTextFocus::AprsMsgText;
        } else if (hs.text_focus == HsTextFocus::AprsHost || hs.text_focus == HsTextFocus::AprsPort ||
                   hs.text_focus == HsTextFocus::AprsPath || hs.text_focus == HsTextFocus::AprsComment ||
                   hs.text_focus == HsTextFocus::SstvOverlay) {
            auto& cfg = hamshack::get_app_config();
            cfg.aprs_kiss_host = hs.aprs_host_edit;
            cfg.aprs_kiss_port = std::clamp(std::atoi(hs.aprs_port_edit.c_str()), 1, 65535);
            cfg.aprs_digi_path = hs.aprs_path_edit;
            cfg.aprs_beacon_comment = hs.aprs_comment_edit;
            cfg.sstv_overlay_text = hs.sstv_overlay_edit;
            if (hs.aprs) {
                hs.aprs->set_station_identity(hs.hf_callsign, static_cast<uint8_t>(hs.aprs_ssid), hs.aprs_path_edit);
                hs.aprs->set_kiss_endpoint(hs.aprs_host_edit, static_cast<uint16_t>(cfg.aprs_kiss_port));
            }
            cfg.save();
            hs.text_focus = HsTextFocus::None;
        }
    } else if (c >= 0x20 && c < 0x7F && target->size() < max_len) {
        char out_c = force_upper ? static_cast<char>(std::toupper(static_cast<unsigned char>(c))) : c;
        target->push_back(out_c);
    }

    if (hs.text_focus == HsTextFocus::Ft8DxCall || hs.text_focus == HsTextFocus::Ft8DxGrid ||
        hs.text_focus == HsTextFocus::HfCallsign || hs.text_focus == HsTextFocus::HfGrid ||
        hs.text_focus == HsTextFocus::WlCfgGrid || hs.text_focus == HsTextFocus::WlCfgCall) {
        auto& cfg = hamshack::get_app_config();
        if (hs.text_focus == HsTextFocus::WlCfgGrid) {
            hs.hf_grid = cfg.winlink_grid;
            cfg.hf_grid = cfg.winlink_grid;
        } else if (hs.text_focus == HsTextFocus::WlCfgCall) {
            hs.hf_callsign = cfg.winlink_callsign;
            cfg.hf_callsign = cfg.winlink_callsign;
        } else {
            cfg.hf_callsign = hs.hf_callsign;
            cfg.hf_grid = hs.hf_grid;
            cfg.winlink_callsign = hs.hf_callsign;
            cfg.winlink_grid = hs.hf_grid;
        }
        if (!hs.ft8.dx_call.empty() &&
            hamshack::Ft8Ft4Engine::normalize_call_key(hs.ft8.dx_call) == hamshack::Ft8Ft4Engine::normalize_call_key(hs.hf_callsign)) {
            hs.ft8.dx_call.clear();
        }
        hs.sync_hf_station();
        hs.ft8.regenerate_std_msgs();
        hs.cw.dx_call = hs.ft8.dx_call;
        cfg.save();
    }
    if (hs.text_focus == HsTextFocus::TwitchChannel || hs.text_focus == HsTextFocus::TwitchStreamKey) {
        auto& cfg = hamshack::get_app_config();
        hs.twitch.channel_name = cfg.twitch_channel;
        hs.twitch.stream_key = cfg.twitch_stream_key;
        cfg.save();
    }
    if (hs.text_focus == HsTextFocus::SstvOverlay && hs.sstv) {
        hs.sstv->set_tx_overlay_text(hs.sstv_overlay_edit);
        auto& cfg = hamshack::get_app_config();
        cfg.sstv_overlay_text = hs.sstv_overlay_edit;
        cfg.save();
    }
    return true;
}

} // namespace hamshack_ui
