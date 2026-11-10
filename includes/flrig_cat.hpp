#pragma once
// ============================================================================
// flrig_cat.hpp — FLRig CAT Transceiver Engine & Xiegu G90 Native USB Driver
// Ported from flrig/src/rigs/xiegu/Xiegu-G90.cxx, flrig/src/rigs/icom/ICbase.cxx,
// and flrig/src/support/serial.cxx for HamShack++ on macOS Apple Silicon.
// Zero simulated/fake data: connects directly to hardware over USB CI-V serial.
// ============================================================================

#include <algorithm>
#include <array>
#include <atomic>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <ctime>
#include <deque>
#include <functional>
#include <iomanip>
#include <mutex>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

#include "../src/hamshack_platform.hpp"
#ifdef _WIN32
#include <setupapi.h>
#endif

namespace hamshack {

enum class RigModel : int {
    Xiegu_G90 = 0,
    Xiegu_X6100 = 1,
    Xiegu_5105 = 2,
    Icom_IC7300 = 3,
    Icom_IC705 = 4,
    Yaesu_FT891 = 5,
    Yaesu_FT991A = 6,
    Kenwood_TS590 = 7,
    Elecraft_KX3 = 8,
    Flrig_XmlRpc = 9
};

enum class RigMode : int {
    LSB = 0,
    USB = 1,
    AM  = 2,
    CW  = 3,
    CWR = 4
};

struct RigBandPreset {
    const char* label;
    uint64_t min_hz;
    uint64_t max_hz;
    uint64_t ft8_hz;
    uint64_t ft4_hz;
    uint64_t js8_hz;
    uint64_t cw_hz;
    uint64_t ssb_hz;
};

static constexpr RigBandPreset kBandPresets[11] = {
    {"160m",  1800000ULL,  2000000ULL,  1840000ULL,  1840000ULL,  1842000ULL,  1820000ULL,  1900000ULL},
    {"80m",   3500000ULL,  4000000ULL,  3573000ULL,  3575000ULL,  3578000ULL,  3530000ULL,  3800000ULL},
    {"60m",   5250000ULL,  5450000ULL,  5357000ULL,  5357000ULL,  5358000ULL,  5330500ULL,  5357000ULL},
    {"40m",   7000000ULL,  7300000ULL,  7074000ULL,  7047500ULL,  7078000ULL,  7030000ULL,  7185000ULL},
    {"30m",  10100000ULL, 10150000ULL, 10136000ULL, 10140000ULL, 10130000ULL, 10110000ULL, 10136000ULL},
    {"20m",  14000000ULL, 14350000ULL, 14074000ULL, 14080000ULL, 14078000ULL, 14030000ULL, 14250000ULL},
    {"17m",  18068000ULL, 18168000ULL, 18100000ULL, 18104000ULL, 18104000ULL, 18080000ULL, 18130000ULL},
    {"15m",  21000000ULL, 21450000ULL, 21074000ULL, 21140000ULL, 21078000ULL, 21030000ULL, 21300000ULL},
    {"12m",  24890000ULL, 24990000ULL, 24915000ULL, 24919000ULL, 24922000ULL, 24900000ULL, 24950000ULL},
    {"10m",  28000000ULL, 29700000ULL, 28074000ULL, 28180000ULL, 28078000ULL, 28030000ULL, 28400000ULL},
    {"6m",   50000000ULL, 54000000ULL, 50313000ULL, 50318000ULL, 50318000ULL, 50090000ULL, 50125000ULL}
};

struct CatTraceEntry {
    std::string time_str;
    bool tx = false;
    std::string hex_str;
    std::string summary;
};

struct MemoryChannel {
    std::string label;
    uint64_t freq_hz = 14074000ULL;
    RigMode mode = RigMode::USB;
    int filter = 1;
};

// Exact Xiegu G90 lookup tables ported from flrig/src/rigs/xiegu/Xiegu-G90.cxx
struct G90MeterPair { int mtr; float val; };

static constexpr G90MeterPair kG90PwrTbl[20] = {
    {0, 1},   {13, 2},   {26, 3},   {40, 4},   {53, 5},
    {67, 6},  {80, 7},   {93, 8},   {107, 9},  {121, 10},
    {134, 11},{147, 12}, {161, 13}, {174, 14}, {187, 15},
    {201, 16},{214, 17}, {228, 18}, {241, 19}, {255, 20}
};

static constexpr G90MeterPair kG90SmeterTbl[16] = {
    {0,   0},  // S0  -128 dBm
    {10,  5},  // S1  -121 dBm
    {22, 11},  // S2  -115 dBm
    {35, 16},  // S3  -109 dBm
    {56, 21},  // S4  -102 dBm
    {71, 26},  // S5   -95 dBm
    {89, 31},  // S6   -88 dBm
    {109,37},  // S7   -80 dBm
    {132,43},  // S8   -71 dBm
    {147,50},  // S9   -65 dBm
    {168,58},  // S9+10
    {173,66},  // S9+20
    {183,74},  // S9+30
    {193,82},  // S9+40
    {201,90},  // S9+50
    {209,100}  // S9+60
};

static constexpr G90MeterPair kG90SwrTbl[5] = {
    {0,   0},  // SWR 1.0
    {48, 12},  // SWR 1.5
    {80, 25},  // SWR 2.0
    {120,50},  // SWR 3.0
    {255,100}  // SWR 5.0+
};

static constexpr uint8_t kG90ModeByte[5] = {
    0x00, // LSB
    0x01, // USB
    0x02, // AM
    0x03, // CW
    0x07  // CW-R
};

class FlrigCatEngine {
public:
    mutable std::mutex mtx;

    // Rig Configuration
    RigModel model = RigModel::Xiegu_G90;
    uint8_t civ_addr = 0x88; // Default Xiegu G90 CI-V address
    int baud_rate = 19200;   // Default Xiegu G90 CAT baud rate
    std::string serial_port; // Auto-detected "/dev/cu.usbserial-..."
    std::vector<std::string> available_ports;
    std::string xmlrpc_host = "127.0.0.1";
    int xmlrpc_port = 12345;
    bool xmlrpc_server_enabled = true;
    bool dtr_high = true;
    bool rts_high = true;
    bool rts_ptt = false;
    bool auto_band_mode_on_tab = false; // Do not overwrite live G90 frequency merely by switching UI tabs

    // Live Hardware Transceiver State (Thread-safe via mtx; 0/offline when G90 is off)
    bool connected = false;
    bool port_open = false;
    bool is_sim = false;
    std::string status_text = "OFFLINE — Scanning USB Serial for Xiegu G90 (CI-V 0x88 @ 19200)...";

    bool active_vfo_b = false;
    uint64_t vfo_a_hz = 0;
    uint64_t vfo_b_hz = 0;
    RigMode mode_a = RigMode::USB;
    RigMode mode_b = RigMode::USB;
    int filter_a = 1; // 1=FIL1 (2.4kHz), 2=FIL2 (1.8kHz), 3=FIL3 (500Hz)
    int filter_b = 1;
    bool split_enabled = false;
    bool ptt_active = false;
    bool atu_tuning = false;
    bool atu_enabled = false;

    int preamp_level = 0;     // 0=OFF, 1=PRE
    int atten_level = 0;      // 0=OFF, 1=ATT (-20dB)
    int agc_mode = 1;         // 0=AGC-O, 1=AGC-F, 2=AGC-S, 3=AGC-A
    bool nb_enabled = false;  // Noise blanker
    bool comp_enabled = false;// Speech compressor

    int power_watts = 16;     // Live 1..20W read from G90 (default 16W)
    int volume_pct = 20;      // Live 0..100% read from G90
    int squelch_pct = 0;      // Live 0..100% read from G90
    int cw_wpm = 20;          // 5..50 WPM
    int cw_spot_tone_hz = 700;// 300..900 Hz
    int active_band_idx = -1; // Live band index (-1 when offline or out-of-band)

    // Live Hardware Meter Readouts (0 when G90 is offline)
    int smeter_pct = 0;       // 0..100 (50 = S9)
    int smeter_raw_bcd = 0;   // Raw G90 BCD value (0..241)
    int smeter_dbm = -128;    // Calculated dBm from G90 S-meter table
    float power_out_watts = 0.0f;
    float swr_ratio = 0.0f;
    uint64_t rx_frame_count = 0;
    uint64_t tx_frame_count = 0;

    std::deque<float> smeter_history;
    std::deque<float> power_history;
    std::deque<float> swr_history;
    std::deque<CatTraceEntry> cat_traces;
    std::vector<MemoryChannel> memories;

    FlrigCatEngine() {
        memories = {
            {"10m FT8 Calling",   28074000ULL, RigMode::USB, 1},
            {"10m JS8Call",       28078000ULL, RigMode::USB, 1},
            {"10m CW Calling",    28030000ULL, RigMode::CW,  3},
            {"15m FT8 Calling",   21074000ULL, RigMode::USB, 1},
            {"17m FT8 Calling",   18100000ULL, RigMode::USB, 1},
            {"20m FT8 Calling",   14074000ULL, RigMode::USB, 1},
            {"20m FT4 Calling",   14080000ULL, RigMode::USB, 1},
            {"20m JS8Call",       14078000ULL, RigMode::USB, 1},
            {"20m QRP CW",        14060000ULL, RigMode::CW,  3},
            {"30m FT8 Calling",   10136000ULL, RigMode::USB, 1},
            {"40m FT8 Calling",    7074000ULL, RigMode::USB, 1},
            {"40m JS8Call",        7078000ULL, RigMode::USB, 1},
            {"40m QRP CW",         7030000ULL, RigMode::CW,  3},
            {"80m FT8 Calling",    3573000ULL, RigMode::USB, 1}
        };
        scan_serial_ports();
    }

    ~FlrigCatEngine() {
        stop();
    }

    void scan_serial_ports() {
        std::vector<std::string> ports;
#ifdef _WIN32
        struct WinPortCand {
            std::string port;
            bool preferred = false;
        };
        std::vector<WinPortCand> win_cands;
        static const GUID kHsPortsGuid = {0x4d36e978L, 0xe325, 0x11ce, {0xbf, 0xc1, 0x08, 0x00, 0x2b, 0xe1, 0x03, 0x18}};
        HDEVINFO hDevInfo = SetupDiGetClassDevsA(&kHsPortsGuid, nullptr, nullptr, DIGCF_PRESENT);
        if (hDevInfo != INVALID_HANDLE_VALUE) {
            SP_DEVINFO_DATA devData{};
            devData.cbSize = sizeof(SP_DEVINFO_DATA);
            for (DWORD idx = 0; SetupDiEnumDeviceInfo(hDevInfo, idx, &devData); ++idx) {
                char friendly[256] = {0};
                char hwid[512] = {0};
                SetupDiGetDeviceRegistryPropertyA(hDevInfo, &devData, SPDRP_FRIENDLYNAME, nullptr,
                                                  reinterpret_cast<PBYTE>(friendly), sizeof(friendly) - 1, nullptr);
                SetupDiGetDeviceRegistryPropertyA(hDevInfo, &devData, SPDRP_HARDWAREID, nullptr,
                                                  reinterpret_cast<PBYTE>(hwid), sizeof(hwid) - 1, nullptr);
                std::string fname(friendly);
                std::string port_name;
                size_t com_pos = fname.find("(COM");
                if (com_pos != std::string::npos) {
                    size_t end_pos = fname.find(')', com_pos);
                    if (end_pos != std::string::npos && end_pos > com_pos + 1) {
                        port_name = fname.substr(com_pos + 1, end_pos - com_pos - 1);
                    }
                }
                if (port_name.empty()) continue;

                std::string combined = fname + " " + std::string(hwid);
                std::transform(combined.begin(), combined.end(), combined.begin(),
                               [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
                // Skip standard Bluetooth SPP virtual ports when looking for USB CAT cables
                if (combined.find("standard serial over bluetooth") != std::string::npos) continue;

                bool pref = combined.find("ftdi") != std::string::npos ||
                            combined.find("vid_0403") != std::string::npos ||
                            combined.find("ch34") != std::string::npos ||
                            combined.find("vid_1a86") != std::string::npos ||
                            combined.find("cp210") != std::string::npos ||
                            combined.find("vid_10c4") != std::string::npos ||
                            combined.find("digirig") != std::string::npos ||
                            combined.find("usb serial") != std::string::npos ||
                            combined.find("usb-serial") != std::string::npos;
                win_cands.push_back({port_name, pref});
            }
            SetupDiDestroyDeviceInfoList(hDevInfo);
        }
        std::sort(win_cands.begin(), win_cands.end(), [](const WinPortCand& a, const WinPortCand& b) {
            if (a.preferred != b.preferred) return a.preferred > b.preferred;
            return a.port < b.port;
        });
        for (const auto& c : win_cands) ports.push_back(c.port);
#elif defined(__APPLE__)
        DIR* d = opendir("/dev");
        if (d) {
            struct dirent* ent = nullptr;
            while ((ent = readdir(d)) != nullptr) {
                std::string name = ent->d_name;
                if (name.rfind("cu.", 0) == 0) {
                    if (name.find("Bluetooth-Incoming-Port") != std::string::npos ||
                        name.find("wlan-debug") != std::string::npos ||
                        name.find("JoyXtra") != std::string::npos) {
                        continue;
                    }
                    ports.push_back("/dev/" + name);
                }
            }
            closedir(d);
        }
        // Sort so usbserial-* (FTDI Xiegu G90 CAT cable) always comes first
        std::sort(ports.begin(), ports.end(), [](const std::string& a, const std::string& b) {
            bool a_ser = a.find("usbserial") != std::string::npos || a.find("SLAB") != std::string::npos || a.find("wchusb") != std::string::npos;
            bool b_ser = b.find("usbserial") != std::string::npos || b.find("SLAB") != std::string::npos || b.find("wchusb") != std::string::npos;
            if (a_ser != b_ser) return a_ser > b_ser;
            return a < b;
        });
#endif

        std::lock_guard<std::mutex> lk(mtx);
        available_ports = ports;
        if (serial_port.empty() && !available_ports.empty()) {
            serial_port = available_ports.front();
            reconnect_requested_ = true;
        }
    }

    static const char* model_name(RigModel m) {
        switch (m) {
            case RigModel::Xiegu_G90:   return "Xiegu-G90";
            case RigModel::Xiegu_X6100: return "Xiegu-X6100";
            case RigModel::Xiegu_5105:  return "Xiegu-5105";
            case RigModel::Icom_IC7300: return "IC-7300";
            case RigModel::Icom_IC705:  return "IC-705";
            case RigModel::Yaesu_FT891: return "FT-891";
            case RigModel::Yaesu_FT991A:return "FT-991A";
            case RigModel::Kenwood_TS590:return "TS-590SG";
            case RigModel::Elecraft_KX3:return "KX3/K3";
            case RigModel::Flrig_XmlRpc:return "FLRig XML-RPC";
            default:                    return "Xiegu-G90";
        }
    }

    static uint8_t default_civ_for_model(RigModel m) {
        switch (m) {
            case RigModel::Xiegu_G90:   return 0x88;
            case RigModel::Xiegu_X6100: return 0x70;
            case RigModel::Xiegu_5105:  return 0x70;
            case RigModel::Icom_IC7300: return 0x94;
            case RigModel::Icom_IC705:  return 0xA4;
            default:                    return 0x88;
        }
    }

    static const char* mode_name(RigMode m) {
        switch (m) {
            case RigMode::LSB: return "LSB";
            case RigMode::USB: return "USB";
            case RigMode::AM:  return "AM";
            case RigMode::CW:  return "CW";
            case RigMode::CWR: return "CW-R";
            default:           return "USB";
        }
    }

    static const char* agc_label(int agc) {
        static const char* kLabels[4] = {"AGC-O", "AGC-F", "AGC-S", "AGC-A"};
        return kLabels[std::clamp(agc, 0, 3)];
    }

    static std::string format_freq_mhz(uint64_t hz) {
        if (hz == 0) return "---.---.---";
        uint64_t mhz = hz / 1000000ULL;
        uint64_t khz = (hz / 1000ULL) % 1000ULL;
        uint64_t sub = hz % 1000ULL;
        char buf[32];
        std::snprintf(buf, sizeof(buf), "%2llu.%03llu.%03llu",
                      static_cast<unsigned long long>(mhz),
                      static_cast<unsigned long long>(khz),
                      static_cast<unsigned long long>(sub));
        return std::string(buf);
    }

    static int band_index_for_freq(uint64_t hz) {
        for (int b = 0; b < 11; ++b) {
            if (hz >= kBandPresets[b].min_hz && hz <= kBandPresets[b].max_hz) {
                return b;
            }
        }
        return -1;
    }

    static std::string smeter_s_unit_str(int pct) {
        if (pct <= 0) return "S0";
        if (pct < 5)  return "S0";
        if (pct < 11) return "S1";
        if (pct < 16) return "S2";
        if (pct < 21) return "S3";
        if (pct < 26) return "S4";
        if (pct < 31) return "S5";
        if (pct < 37) return "S6";
        if (pct < 43) return "S7";
        if (pct < 50) return "S8";
        if (pct < 56) return "S9";
        int over = ((pct - 50) * 60) / 50;
        over = (over / 5) * 5;
        return "S9+" + std::to_string(std::max(5, over)) + "dB";
    }

    void start() {
        if (running_.exchange(true)) return;
        scan_serial_ports();
        reconnect_requested_ = true;
        worker_ = std::thread([this] { worker_loop(); });
        xmlrpc_thread_ = std::thread([this] { xmlrpc_server_loop(); });
    }

    void stop() {
        if (!running_.exchange(false)) return;
        if (worker_.joinable()) worker_.join();
        if (xmlrpc_thread_.joinable()) xmlrpc_thread_.join();
        close_serial();
    }

    void connect_serial(const std::string& port, int baud = 19200) {
        std::lock_guard<std::mutex> lk(mtx);
        serial_port = port;
        baud_rate = baud;
        reconnect_requested_ = true;
    }

    void cycle_serial_port() {
        scan_serial_ports();
        std::lock_guard<std::mutex> lk(mtx);
        if (available_ports.empty()) {
            serial_port.clear();
            reconnect_requested_ = true;
            return;
        }
        auto it = std::find(available_ports.begin(), available_ports.end(), serial_port);
        if (it == available_ports.end() || ++it == available_ports.end()) {
            serial_port = available_ports.front();
        } else {
            serial_port = *it;
        }
        reconnect_requested_ = true;
    }

    void select_rig_model(RigModel m) {
        std::lock_guard<std::mutex> lk(mtx);
        model = m;
        civ_addr = default_civ_for_model(m);
        if (m == RigModel::Xiegu_G90 || m == RigModel::Xiegu_5105 || m == RigModel::Xiegu_X6100) {
            baud_rate = 19200;
        }
        reconnect_requested_ = true;
    }

    // ---- Rig Control Actions (Send real CI-V commands to G90) ----
    void set_vfo_a(uint64_t hz) {
        std::lock_guard<std::mutex> lk(mtx);
        if (!connected && !is_serial_open()) return;
        vfo_a_hz = std::clamp<uint64_t>(hz, 100000ULL, 60000000ULL);
        active_band_idx = band_index_for_freq(vfo_a_hz);
        if (active_vfo_b) {
            queue_civ_cmd(build_civ_frame({0x07, 0x00}), "selectA()");
            active_vfo_b = false;
        }
        queue_civ_cmd(build_civ_freq(vfo_a_hz), "set_vfoA(" + format_freq_mhz(vfo_a_hz) + ")");
    }

    void set_vfo_b(uint64_t hz) {
        std::lock_guard<std::mutex> lk(mtx);
        if (!connected && !is_serial_open()) return;
        vfo_b_hz = std::clamp<uint64_t>(hz, 100000ULL, 60000000ULL);
        if (!active_vfo_b) {
            queue_civ_cmd(build_civ_frame({0x07, 0x01}), "selectB()");
            queue_civ_cmd(build_civ_freq(vfo_b_hz), "set_vfoB(" + format_freq_mhz(vfo_b_hz) + ")");
            queue_civ_cmd(build_civ_frame({0x07, 0x00}), "selectA()");
        } else {
            queue_civ_cmd(build_civ_freq(vfo_b_hz), "set_vfoB(" + format_freq_mhz(vfo_b_hz) + ")");
        }
    }

    void step_active_vfo(int64_t delta_hz) {
        std::lock_guard<std::mutex> lk(mtx);
        if (!connected) return;
        uint64_t& f = active_vfo_b ? vfo_b_hz : vfo_a_hz;
        if (f == 0) return;
        int64_t next = static_cast<int64_t>(f) + delta_hz;
        f = static_cast<uint64_t>(std::clamp<int64_t>(next, 500000LL, 54000000LL));
        if (!active_vfo_b) active_band_idx = band_index_for_freq(f);
        queue_civ_cmd(build_civ_freq(f), std::string(active_vfo_b ? "step_vfoB(" : "step_vfoA(") + format_freq_mhz(f) + ")");
    }

    void select_vfo(bool use_b) {
        std::lock_guard<std::mutex> lk(mtx);
        if (!connected) return;
        active_vfo_b = use_b;
        std::vector<uint8_t> payload = {0x07, static_cast<uint8_t>(use_b ? 0x01 : 0x00)};
        queue_civ_cmd(build_civ_frame(payload), use_b ? "selectB()" : "selectA()");
        queue_civ_cmd(build_civ_frame({0x03}), "get_vfo()");
        queue_civ_cmd(build_civ_frame({0x04}), "get_mode()");
    }

    void copy_vfo_a_to_b() {
        std::lock_guard<std::mutex> lk(mtx);
        if (!connected || vfo_a_hz == 0) return;
        vfo_b_hz = vfo_a_hz;
        mode_b = mode_a;
        filter_b = filter_a;
        queue_civ_cmd(build_civ_frame({0x07, 0xA0}), "A2B (0x07 0xA0)");
        queue_civ_cmd(build_civ_frame({0x07, 0x01}), "selectB()");
        queue_civ_cmd(build_civ_freq(vfo_b_hz), "set_vfoB(" + format_freq_mhz(vfo_b_hz) + ")");
        queue_civ_cmd(build_civ_frame({0x07, 0x00}), "selectA()");
    }

    void swap_vfo_ab() {
        std::lock_guard<std::mutex> lk(mtx);
        if (!connected) return;
        std::swap(vfo_a_hz, vfo_b_hz);
        std::swap(mode_a, mode_b);
        std::swap(filter_a, filter_b);
        queue_civ_cmd(build_civ_frame({0x07, 0xB0}), "swapAB (0x07 0xB0)");
        uint64_t cur = active_vfo_b ? vfo_b_hz : vfo_a_hz;
        if (cur > 0) queue_civ_cmd(build_civ_freq(cur), "sync_vfo(" + format_freq_mhz(cur) + ")");
    }

    void set_split(bool on) {
        std::lock_guard<std::mutex> lk(mtx);
        if (!connected) return;
        split_enabled = on;
        std::vector<uint8_t> payload = {0x0F, static_cast<uint8_t>(on ? 0x01 : 0x00)};
        queue_civ_cmd(build_civ_frame(payload), on ? "set_split(ON)" : "set_split(OFF)");
    }

    void set_mode(RigMode m, int filt = -1) {
        std::lock_guard<std::mutex> lk(mtx);
        if (!connected) return;
        if (active_vfo_b) {
            mode_b = m;
            if (filt >= 1 && filt <= 3) filter_b = filt;
        } else {
            mode_a = m;
            if (filt >= 1 && filt <= 3) filter_a = filt;
        }
        int idx = std::clamp(static_cast<int>(m), 0, 4);
        uint8_t fbyte = static_cast<uint8_t>(active_vfo_b ? filter_b : filter_a);
        std::vector<uint8_t> payload = {0x06, kG90ModeByte[idx], fbyte};
        queue_civ_cmd(build_civ_frame(payload), std::string("set_mode(") + mode_name(m) + ", FIL" + std::to_string(fbyte) + ")");
    }

    void cycle_filter() {
        std::lock_guard<std::mutex> lk(mtx);
        if (!connected) return;
        int& flt = active_vfo_b ? filter_b : filter_a;
        flt = (flt % 3) + 1;
        RigMode m = active_vfo_b ? mode_b : mode_a;
        int idx = std::clamp(static_cast<int>(m), 0, 4);
        std::vector<uint8_t> payload = {0x06, kG90ModeByte[idx], static_cast<uint8_t>(flt)};
        queue_civ_cmd(build_civ_frame(payload), "set_filter(FIL" + std::to_string(flt) + ")");
    }

    void set_ptt(bool tx) {
        std::lock_guard<std::mutex> lk(mtx);
        if (!connected) return;
        ptt_active = tx;
        std::vector<uint8_t> payload = {0x1C, 0x00, static_cast<uint8_t>(tx ? 0x01 : 0x00)};
        queue_civ_cmd(build_civ_frame(payload), tx ? "set_PTT(TX ON)" : "set_PTT(RX)");
    }

    void start_atu_tune() {
        std::lock_guard<std::mutex> lk(mtx);
        if (!connected) return;
        atu_tuning = !atu_tuning;
        uint8_t how = atu_tuning ? 0x02 : 0x00; // 0x02 = start G90 ATU tune, 0x00 = ATU off
        std::vector<uint8_t> payload = {0x1C, 0x01, how};
        queue_civ_cmd(build_civ_frame(payload), atu_tuning ? "tune_rig(G90 ATU TUNE)" : "tune_rig(ATU OFF)");
    }

    void toggle_preamp() {
        std::lock_guard<std::mutex> lk(mtx);
        if (!connected) return;
        preamp_level = preamp_level ? 0 : 1;
        std::vector<uint8_t> payload = {0x16, 0x02, static_cast<uint8_t>(preamp_level)};
        queue_civ_cmd(build_civ_frame(payload), preamp_level ? "set_preamp(ON)" : "set_preamp(OFF)");
    }

    void toggle_attenuator() {
        std::lock_guard<std::mutex> lk(mtx);
        if (!connected) return;
        atten_level = atten_level ? 0 : 1;
        std::vector<uint8_t> payload = {0x11, static_cast<uint8_t>(atten_level)};
        queue_civ_cmd(build_civ_frame(payload), atten_level ? "set_attenuator(ON)" : "set_attenuator(OFF)");
    }

    void cycle_agc() {
        std::lock_guard<std::mutex> lk(mtx);
        if (!connected) return;
        agc_mode = (agc_mode + 1) % 4;
        std::vector<uint8_t> payload = {0x16, 0x12, static_cast<uint8_t>(agc_mode)};
        queue_civ_cmd(build_civ_frame(payload), std::string("incr_agc(") + agc_label(agc_mode) + ")");
    }

    void toggle_nb() {
        std::lock_guard<std::mutex> lk(mtx);
        if (!connected) return;
        nb_enabled = !nb_enabled;
        std::vector<uint8_t> payload = {0x16, 0x22, static_cast<uint8_t>(nb_enabled ? 1 : 0)};
        queue_civ_cmd(build_civ_frame(payload), nb_enabled ? "set_noise(NB ON)" : "set_noise(NB OFF)");
    }

    void toggle_comp() {
        std::lock_guard<std::mutex> lk(mtx);
        if (!connected) return;
        comp_enabled = !comp_enabled;
        std::vector<uint8_t> payload = {0x16, 0x44, static_cast<uint8_t>(comp_enabled ? 1 : 0)};
        queue_civ_cmd(build_civ_frame(payload), comp_enabled ? "set_compression(ON)" : "set_compression(OFF)");
    }

    void step_power_watts(int delta) {
        std::lock_guard<std::mutex> lk(mtx);
        int max_w = (model == RigModel::Xiegu_G90) ? 20 : (model == RigModel::Xiegu_X6100 || model == RigModel::Xiegu_5105 || model == RigModel::Icom_IC705) ? 10 : 100;
        if (power_watts <= 0) power_watts = 16;
        power_watts = std::clamp(power_watts + delta, 1, max_w);
        if (!connected) return;
        int p = 255;
        for (int n = 0; n < 20; ++n) {
            if (power_watts <= kG90PwrTbl[n].val) {
                p = kG90PwrTbl[n].mtr;
                break;
            }
        }
        std::vector<uint8_t> payload = {0x14, 0x0A, hex_bcd_byte(p / 100), hex_bcd_byte(p % 100)};
        queue_civ_cmd(build_civ_frame(payload), "set_power_control(" + std::to_string(power_watts) + "W)");
    }

    void set_power_watts(int w) {
        std::lock_guard<std::mutex> lk(mtx);
        int max_w = (model == RigModel::Xiegu_G90) ? 20 : (model == RigModel::Xiegu_X6100 || model == RigModel::Xiegu_5105 || model == RigModel::Icom_IC705) ? 10 : 100;
        power_watts = std::clamp(w, 1, max_w);
        if (!connected) return;
        int p = 255;
        for (int n = 0; n < 20; ++n) {
            if (power_watts <= kG90PwrTbl[n].val) {
                p = kG90PwrTbl[n].mtr;
                break;
            }
        }
        std::vector<uint8_t> payload = {0x14, 0x0A, hex_bcd_byte(p / 100), hex_bcd_byte(p % 100)};
        queue_civ_cmd(build_civ_frame(payload), "set_power_control(" + std::to_string(power_watts) + "W)");
    }

    void step_volume(int delta) {
        std::lock_guard<std::mutex> lk(mtx);
        volume_pct = std::clamp(volume_pct + delta, 0, 100);
        if (!connected) return;
        int v255 = (volume_pct * 255) / 100;
        std::vector<uint8_t> payload = {0x14, 0x01, hex_bcd_byte(v255 / 100), hex_bcd_byte(v255 % 100)};
        queue_civ_cmd(build_civ_frame(payload), "set_volume_control(" + std::to_string(volume_pct) + "%)");
    }

    void set_volume(int vol) {
        std::lock_guard<std::mutex> lk(mtx);
        volume_pct = std::clamp(vol, 0, 100);
        if (!connected) return;
        int v255 = (volume_pct * 255) / 100;
        std::vector<uint8_t> payload = {0x14, 0x01, hex_bcd_byte(v255 / 100), hex_bcd_byte(v255 % 100)};
        queue_civ_cmd(build_civ_frame(payload), "set_volume_control(" + std::to_string(volume_pct) + "%)");
    }

    void step_squelch(int delta) {
        std::lock_guard<std::mutex> lk(mtx);
        squelch_pct = std::clamp(squelch_pct + delta, 0, 100);
        if (!connected) return;
        int s255 = (squelch_pct * 255) / 100;
        std::vector<uint8_t> payload = {0x14, 0x03, hex_bcd_byte(s255 / 100), hex_bcd_byte(s255 % 100)};
        queue_civ_cmd(build_civ_frame(payload), "set_squelch(" + std::to_string(squelch_pct) + "%)");
    }

    void set_squelch(int sql) {
        std::lock_guard<std::mutex> lk(mtx);
        squelch_pct = std::clamp(sql, 0, 100);
        if (!connected) return;
        int s255 = (squelch_pct * 255) / 100;
        std::vector<uint8_t> payload = {0x14, 0x03, hex_bcd_byte(s255 / 100), hex_bcd_byte(s255 % 100)};
        queue_civ_cmd(build_civ_frame(payload), "set_squelch(" + std::to_string(squelch_pct) + "%)");
    }

    void set_cw_wpm(int wpm) {
        std::lock_guard<std::mutex> lk(mtx);
        cw_wpm = std::clamp(wpm, 5, 50);
        if (!connected) return;
        int v255 = std::clamp(static_cast<int>(std::round((cw_wpm - 6) * 255.0 / 45.0)), 0, 255);
        std::vector<uint8_t> payload = {0x14, 0x0C, hex_bcd_byte(v255 / 100), hex_bcd_byte(v255 % 100)};
        queue_civ_cmd(build_civ_frame(payload), "set_cw_wpm(" + std::to_string(cw_wpm) + " WPM)");
    }

    void set_cw_spot_tone(int hz) {
        std::lock_guard<std::mutex> lk(mtx);
        cw_spot_tone_hz = std::clamp(hz, 300, 900);
        if (!connected) return;
        int v255 = std::clamp(static_cast<int>(std::round((cw_spot_tone_hz - 300) * 255.0 / 600.0 + 0.5)), 0, 255);
        std::vector<uint8_t> payload = {0x14, 0x09, hex_bcd_byte(v255 / 100), hex_bcd_byte(v255 % 100)};
        queue_civ_cmd(build_civ_frame(payload), "set_cw_spot_tone(" + std::to_string(cw_spot_tone_hz) + " Hz)");
    }

    // Tunes the G90 to the selected band for the active operating mode (0=FT8, 1=FT4, 2=JS8, 3=CW, 4=SSB)
    void select_band_for_mode(int band_idx, int op_mode) {
        band_idx = std::clamp(band_idx, 0, 10);
        uint64_t f = kBandPresets[band_idx].ft8_hz;
        RigMode rm = RigMode::USB;
        int flt = 1;
        if (op_mode == 1) {
            f = kBandPresets[band_idx].ft4_hz;
            rm = RigMode::USB;
        } else if (op_mode == 2) {
            f = kBandPresets[band_idx].js8_hz;
            rm = RigMode::USB;
        } else if (op_mode == 3) {
            f = kBandPresets[band_idx].cw_hz;
            rm = RigMode::CW;
            flt = 3; // Narrow 500 Hz filter for CW
        } else if (op_mode == 4) {
            f = kBandPresets[band_idx].ssb_hz;
            rm = (f < 10000000ULL) ? RigMode::LSB : RigMode::USB;
        }
        set_vfo_a(f);
        set_mode(rm, flt);
    }

    void send_custom_hex_cat(const std::string& hex_str) {
        std::vector<uint8_t> raw;
        auto nib = [](char c) -> int {
            if (c >= '0' && c <= '9') return c - '0';
            if (c >= 'a' && c <= 'f') return c - 'a' + 10;
            if (c >= 'A' && c <= 'F') return c - 'A' + 10;
            return -1;
        };
        int hi = -1;
        for (char c : hex_str) {
            int v = nib(c);
            if (v < 0) continue;
            if (hi < 0) hi = v;
            else {
                raw.push_back(static_cast<uint8_t>((hi << 4) | v));
                hi = -1;
            }
        }
        if (!raw.empty()) {
            std::lock_guard<std::mutex> lk(mtx);
            queue_civ_cmd(raw, "Custom CI-V Hex Command");
        }
    }

    void notify_audio_signal_level(float /*rx_rms_db*/) {
        // No-op: S-meter is driven strictly by live G90 CI-V 0x15 0x02 hardware polls
    }

private:
    std::atomic<bool> running_{false};
    std::thread worker_;
    std::thread xmlrpc_thread_;
#ifdef _WIN32
    HANDLE serial_win_ = INVALID_HANDLE_VALUE;
#endif
    int serial_fd_ = -1;
    bool reconnect_requested_ = true;
    int missed_polls_ = 0;

    bool is_serial_open() const {
#ifdef _WIN32
        return serial_win_ != INVALID_HANDLE_VALUE;
#else
        return serial_fd_ >= 0;
#endif
    }

    struct QueuedCmd {
        std::vector<uint8_t> bytes;
        std::string desc;
    };
    std::deque<QueuedCmd> cmd_queue_;

    static uint64_t now_ms() {
        using namespace std::chrono;
        return duration_cast<milliseconds>(steady_clock::now().time_since_epoch()).count();
    }

    static std::string now_hms_ms() {
        using namespace std::chrono;
        auto now = system_clock::now();
        auto ms = duration_cast<milliseconds>(now.time_since_epoch()) % 1000;
        std::time_t t = system_clock::to_time_t(now);
        std::tm tm_val{};
        hs_localtime(&t, &tm_val);
        char buf[32];
        std::snprintf(buf, sizeof(buf), "%02d:%02d:%02d.%03d",
                      tm_val.tm_hour, tm_val.tm_min, tm_val.tm_sec, static_cast<int>(ms.count()));
        return std::string(buf);
    }

    static uint8_t hex_bcd_byte(int val) {
        val = std::clamp(val, 0, 99);
        return static_cast<uint8_t>(((val / 10) << 4) | (val % 10));
    }

    static int bcd_to_int(uint8_t b) {
        return ((b >> 4) & 0x0F) * 10 + (b & 0x0F);
    }

    static std::string bytes_to_hex(const std::vector<uint8_t>& b) {
        static const char* HX = "0123456789ABCDEF";
        std::string out;
        for (size_t i = 0; i < b.size(); ++i) {
            if (i > 0) out += ' ';
            out += HX[b[i] >> 4];
            out += HX[b[i] & 0x0F];
        }
        return out;
    }

    std::vector<uint8_t> build_civ_frame(const std::vector<uint8_t>& payload) const {
        std::vector<uint8_t> frame;
        frame.reserve(payload.size() + 5);
        frame.push_back(0xFE);
        frame.push_back(0xFE);
        frame.push_back(civ_addr);
        frame.push_back(0xE0);
        frame.insert(frame.end(), payload.begin(), payload.end());
        frame.push_back(0xFD);
        return frame;
    }

    // Exact 10-digit little-endian BCD frequency encoding from flrig/src/rigs/rigbase.cxx (`to_bcd_be`)
    std::vector<uint8_t> build_civ_freq(uint64_t freq_hz) const {
        std::vector<uint8_t> payload(6, 0);
        payload[0] = 0x05;
        uint64_t f = freq_hz;
        for (int i = 0; i < 5; ++i) {
            int d_lo = static_cast<int>(f % 10ULL); f /= 10ULL;
            int d_hi = static_cast<int>(f % 10ULL); f /= 10ULL;
            payload[1 + i] = static_cast<uint8_t>((d_hi << 4) | d_lo);
        }
        return build_civ_frame(payload);
    }

    static uint64_t decode_civ_freq(const uint8_t* bcd5) {
        uint64_t f = 0;
        uint64_t mult = 1;
        for (int i = 0; i < 5; ++i) {
            int lo = bcd5[i] & 0x0F;
            int hi = (bcd5[i] >> 4) & 0x0F;
            f += static_cast<uint64_t>(lo) * mult; mult *= 10ULL;
            f += static_cast<uint64_t>(hi) * mult; mult *= 10ULL;
        }
        return f;
    }

    void log_trace_locked(bool tx, const std::vector<uint8_t>& bytes, const std::string& summary) {
        cat_traces.push_back({now_hms_ms(), tx, bytes_to_hex(bytes), summary});
        if (cat_traces.size() > 250) cat_traces.pop_front();
    }

    void queue_civ_cmd(const std::vector<uint8_t>& bytes, const std::string& desc) {
        log_trace_locked(true, bytes, desc);
        cmd_queue_.push_back({bytes, desc});
        if (cmd_queue_.size() > 64) cmd_queue_.pop_front();
    }

    void set_offline_state_locked(const std::string& reason) {
        connected = false;
        status_text = reason;
        vfo_a_hz = 0;
        vfo_b_hz = 0;
        active_band_idx = -1;
        ptt_active = false;
        atu_tuning = false;
        smeter_pct = 0;
        smeter_raw_bcd = 0;
        smeter_dbm = -128;
        power_out_watts = 0.0f;
        swr_ratio = 0.0f;
        smeter_history.clear();
        power_history.clear();
        swr_history.clear();
    }

    void close_serial() {
#ifdef _WIN32
        if (serial_win_ != INVALID_HANDLE_VALUE) {
            CloseHandle(serial_win_);
            serial_win_ = INVALID_HANDLE_VALUE;
        }
#elif defined(__APPLE__)
        if (serial_fd_ >= 0) {
            close(serial_fd_);
            serial_fd_ = -1;
        }
#endif
        port_open = false;
    }

    bool open_serial_port(const std::string& dev, int baud) {
#ifdef _WIN32
        close_serial();
        if (dev.empty()) return false;
        std::string win_path = (dev.rfind("\\\\.\\", 0) == 0) ? dev : ("\\\\.\\" + dev);
        HANDLE h = CreateFileA(win_path.c_str(), GENERIC_READ | GENERIC_WRITE, 0, nullptr, OPEN_EXISTING, 0, nullptr);
        if (h == INVALID_HANDLE_VALUE) return false;

        DCB dcb{};
        dcb.DCBlength = sizeof(DCB);
        if (!GetCommState(h, &dcb)) {
            CloseHandle(h);
            return false;
        }
        dcb.BaudRate = static_cast<DWORD>(baud > 0 ? baud : 19200);
        dcb.ByteSize = 8;
        dcb.Parity   = NOPARITY;
        dcb.StopBits = ONESTOPBIT;
        dcb.fBinary  = TRUE;
        dcb.fParity  = FALSE;
        dcb.fOutxCtsFlow = FALSE;
        dcb.fOutxDsrFlow = FALSE;
        dcb.fDtrControl  = dtr_high ? DTR_CONTROL_ENABLE : DTR_CONTROL_DISABLE;
        dcb.fRtsControl  = rts_high ? RTS_CONTROL_ENABLE : RTS_CONTROL_DISABLE;
        dcb.fOutX = FALSE;
        dcb.fInX  = FALSE;
        if (!SetCommState(h, &dcb)) {
            CloseHandle(h);
            return false;
        }

        COMMTIMEOUTS timeouts{};
        timeouts.ReadIntervalTimeout         = MAXDWORD;
        timeouts.ReadTotalTimeoutMultiplier  = 0;
        timeouts.ReadTotalTimeoutConstant    = 4;
        timeouts.WriteTotalTimeoutMultiplier = 0;
        timeouts.WriteTotalTimeoutConstant   = 100;
        SetCommTimeouts(h, &timeouts);

        std::this_thread::sleep_for(std::chrono::milliseconds(120));
        PurgeComm(h, PURGE_RXCLEAR | PURGE_TXCLEAR);
        serial_win_ = h;
        port_open = true;
        return true;
#elif defined(__APPLE__)
        close_serial();
        if (dev.empty()) return false;
        int fd = open(dev.c_str(), O_RDWR | O_NOCTTY | O_NONBLOCK);
        if (fd < 0) return false;

        struct termios tio{};
        if (tcgetattr(fd, &tio) != 0) {
            close(fd);
            return false;
        }
        cfmakeraw(&tio);
        tio.c_cflag &= ~CSIZE;
        tio.c_cflag |= CS8 | CLOCAL | CREAD;
        tio.c_cflag &= ~(PARENB | CSTOPB | CRTSCTS);
        tio.c_cc[VMIN] = 0;
        tio.c_cc[VTIME] = 0;

        speed_t spd = B19200;
        switch (baud) {
            case 4800:   spd = B4800; break;
            case 9600:   spd = B9600; break;
            case 19200:  spd = B19200; break;
            case 38400:  spd = B38400; break;
            case 57600:  spd = B57600; break;
            case 115200: spd = B115200; break;
            default:     spd = B19200; break;
        }
        cfsetispeed(&tio, spd);
        cfsetospeed(&tio, spd);
        tcflush(fd, TCIFLUSH);
        tcsetattr(fd, TCSANOW, &tio);

        int state = 0;
        if (ioctl(fd, TIOCMGET, &state) == 0) {
            if (dtr_high) state |= TIOCM_DTR; else state &= ~TIOCM_DTR;
            if (rts_high) state |= TIOCM_RTS; else state &= ~TIOCM_RTS;
            ioctl(fd, TIOCMSET, &state);
        }
        usleep(120000);
        tcflush(fd, TCIOFLUSH);
        serial_fd_ = fd;
        port_open = true;
        return true;
#else
        (void)dev; (void)baud;
        return false;
#endif
    }

    // Transact a single CI-V frame over serial and extract all reply frames from the radio (`FE FE E0 <civ> ... FD`)
    std::vector<std::vector<uint8_t>> transact_civ_frames(const std::vector<uint8_t>& cmd_bytes, int timeout_ms = 120) {
        std::vector<std::vector<uint8_t>> replies;
#ifdef _WIN32
        if (serial_win_ == INVALID_HANDLE_VALUE) return replies;
        PurgeComm(serial_win_, PURGE_RXCLEAR);
        DWORD written = 0;
        if (!WriteFile(serial_win_, cmd_bytes.data(), static_cast<DWORD>(cmd_bytes.size()), &written, nullptr)) {
            close_serial();
            return replies;
        }
        {
            std::lock_guard<std::mutex> lk(mtx);
            ++tx_frame_count;
        }

        std::vector<uint8_t> rx;
        uint64_t t_start = now_ms();
        while (static_cast<int>(now_ms() - t_start) < timeout_ms) {
            uint8_t tmp[128];
            DWORD got = 0;
            if (!ReadFile(serial_win_, tmp, sizeof(tmp), &got, nullptr)) {
                close_serial();
                return replies;
            }
            if (got > 0) {
                rx.insert(rx.end(), tmp, tmp + got);
                size_t pos = 0;
                replies.clear();
                while (pos + 5 < rx.size()) {
                    if (rx[pos] == 0xFE && rx[pos + 1] == 0xFE &&
                        (rx[pos + 2] == 0xE0 || rx[pos + 2] == 0x00) &&
                        rx[pos + 3] == civ_addr) {
                        size_t end_pos = pos + 4;
                        while (end_pos < rx.size() && rx[end_pos] != 0xFD) ++end_pos;
                        if (end_pos < rx.size() && rx[end_pos] == 0xFD) {
                            replies.emplace_back(rx.begin() + pos, rx.begin() + end_pos + 1);
                            pos = end_pos + 1;
                            continue;
                        } else {
                            break;
                        }
                    }
                    ++pos;
                }
                if (!replies.empty()) {
                    return replies;
                }
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(4));
        }
#elif defined(__APPLE__)
        if (serial_fd_ < 0) return replies;
        tcflush(serial_fd_, TCIFLUSH);
        ssize_t wn = write(serial_fd_, cmd_bytes.data(), cmd_bytes.size());
        if (wn < 0) {
            close_serial();
            return replies;
        }
        {
            std::lock_guard<std::mutex> lk(mtx);
            ++tx_frame_count;
        }

        std::vector<uint8_t> rx;
        uint64_t t_start = now_ms();
        while (static_cast<int>(now_ms() - t_start) < timeout_ms) {
            uint8_t tmp[128];
            ssize_t n = read(serial_fd_, tmp, sizeof(tmp));
            if (n > 0) {
                rx.insert(rx.end(), tmp, tmp + n);
                // Search for radio-to-controller frames: FE FE E0 <civ_addr> ... FD (or broadcast FE FE 00 <civ_addr> ... FD)
                size_t pos = 0;
                replies.clear();
                while (pos + 5 < rx.size()) {
                    if (rx[pos] == 0xFE && rx[pos + 1] == 0xFE &&
                        (rx[pos + 2] == 0xE0 || rx[pos + 2] == 0x00) &&
                        rx[pos + 3] == civ_addr) {
                        size_t end_pos = pos + 4;
                        while (end_pos < rx.size() && rx[end_pos] != 0xFD) ++end_pos;
                        if (end_pos < rx.size() && rx[end_pos] == 0xFD) {
                            replies.emplace_back(rx.begin() + pos, rx.begin() + end_pos + 1);
                            pos = end_pos + 1;
                            continue;
                        } else {
                            break; // Wait for remaining bytes up to 0xFD
                        }
                    }
                    ++pos;
                }
                if (!replies.empty()) {
                    return replies;
                }
            } else if (n < 0 && errno != EAGAIN && errno != EWOULDBLOCK) {
                close_serial();
                return replies;
            }
            usleep(4000);
        }
#else
        (void)cmd_bytes; (void)timeout_ms;
#endif
        return replies;
    }

    void parse_rx_civ_frame_locked(const std::vector<uint8_t>& frame, bool log_poll = false) {
        if (frame.size() < 6) return;
        if (frame[0] != 0xFE || frame[1] != 0xFE || frame.back() != 0xFD) return;
        // Ignore controller-to-radio echoes (where dst == civ_addr and src == 0xE0)
        if (frame[2] == civ_addr && frame[3] == 0xE0) return;

        ++rx_frame_count;
        missed_polls_ = 0;
        if (!connected) {
            connected = true;
            status_text = std::string(model_name(model)) + " ONLINE on " + serial_port +
                          " (" + std::to_string(baud_rate) + " 8N1 CI-V 0x" + bytes_to_hex({civ_addr}) + ")";
        }

        uint8_t cmd = frame[4];
        if (cmd == 0xFB) {
            log_trace_locked(false, frame, "G90 CI-V ACK (0xFB)");
            return;
        }
        if (cmd == 0xFA) {
            log_trace_locked(false, frame, "G90 CI-V NG (0xFA)");
            return;
        }
        if ((cmd == 0x03 || cmd == 0x00) && frame.size() >= 11) {
            uint64_t f = decode_civ_freq(&frame[5]);
            if (f >= 100000ULL && f <= 60000000ULL) {
                if (active_vfo_b) {
                    vfo_b_hz = f;
                } else {
                    vfo_a_hz = f;
                    active_band_idx = band_index_for_freq(f);
                    if (vfo_b_hz == 0) vfo_b_hz = f;
                }
                if (log_poll) log_trace_locked(false, frame, "VFO = " + format_freq_mhz(f) + " MHz");
            }
        } else if ((cmd == 0x04 || cmd == 0x01) && frame.size() >= 8) {
            uint8_t mb = frame[5];
            uint8_t fb = frame[6];
            RigMode rm = mode_a;
            for (int i = 0; i < 5; ++i) {
                if (kG90ModeByte[i] == mb) { rm = static_cast<RigMode>(i); break; }
            }
            if (active_vfo_b) { mode_b = rm; filter_b = std::clamp<int>(fb, 1, 3); }
            else { mode_a = rm; filter_a = std::clamp<int>(fb, 1, 3); }
            if (log_poll) log_trace_locked(false, frame, std::string("Mode = ") + mode_name(rm) + " FIL" + std::to_string(fb));
        } else if (cmd == 0x15 && frame.size() >= 8) {
            uint8_t sub = frame[5];
            int val = (frame[7] == 0xFD) ? bcd_to_int(frame[6]) : (100 * bcd_to_int(frame[6]) + bcd_to_int(frame[7]));
            if (sub == 0x02) { // S-meter (0000=S0, 0147=S9, 0209=S9+60)
                smeter_raw_bcd = std::clamp(val, 0, 241);
                size_t i = 0;
                for (i = 0; i < 14; ++i) {
                    if (smeter_raw_bcd >= kG90SmeterTbl[i].mtr && smeter_raw_bcd < kG90SmeterTbl[i + 1].mtr) break;
                }
                float interp = kG90SmeterTbl[i].val +
                               (kG90SmeterTbl[i + 1].val - kG90SmeterTbl[i].val) *
                               (smeter_raw_bcd - kG90SmeterTbl[i].mtr) /
                               std::max(1, kG90SmeterTbl[i + 1].mtr - kG90SmeterTbl[i].mtr);
                smeter_pct = std::clamp(static_cast<int>(std::ceil(interp)), 0, 100);
                // S9 = -73 dBm (HF standard), S0 = -127 dBm
                smeter_dbm = -127 + static_cast<int>(std::round(smeter_pct * 1.14f));
                if (log_poll) log_trace_locked(false, frame, "S-Meter = " + smeter_s_unit_str(smeter_pct) + " (raw " + std::to_string(smeter_raw_bcd) + ")");
            } else if (sub == 0x11) { // Power out (0..213 = 0..20W)
                val = std::clamp(val, 0, 213);
                power_out_watts = std::round((val * 20.0f / 213.0f) * 10.0f) / 10.0f;
            } else if (sub == 0x12) { // SWR (0=1.0, 48=1.5, 80=2.0, 120=3.0)
                val = std::clamp(val, 0, 255);
                swr_ratio = 1.0f + (val / 80.0f);
            }
        } else if (cmd == 0x14 && frame.size() >= 8) {
            uint8_t sub = frame[5];
            int val = (frame[7] == 0xFD) ? bcd_to_int(frame[6]) : (100 * bcd_to_int(frame[6]) + bcd_to_int(frame[7]));
            if (sub == 0x0A) { // RF Power setting (0..255 -> 1..20W)
                int pwr = 20;
                for (int n = 0; n < 20; ++n) {
                    if (val <= kG90PwrTbl[n].mtr) {
                        pwr = static_cast<int>(kG90PwrTbl[n].val);
                        break;
                    }
                }
                power_watts = pwr;
            } else if (sub == 0x01) { // AF Volume (0..255 -> 0..100%)
                volume_pct = std::clamp((val * 100) / 255, 0, 100);
            } else if (sub == 0x03) { // Squelch (0..255 -> 0..100%)
                squelch_pct = std::clamp((val * 100) / 255, 0, 100);
            }
        } else if (cmd == 0x16 && frame.size() >= 8) {
            uint8_t sub = frame[5];
            uint8_t v = frame[6];
            if (sub == 0x02) preamp_level = (v != 0) ? 1 : 0;
            else if (sub == 0x12) agc_mode = std::clamp<int>(v, 0, 3);
            else if (sub == 0x22) nb_enabled = (v != 0);
            else if (sub == 0x44) comp_enabled = (v != 0);
        } else if (cmd == 0x11 && frame.size() >= 7) {
            atten_level = (frame[5] != 0) ? 1 : 0;
        } else if (cmd == 0x0F && frame.size() >= 7) {
            if (frame[5] != 0xFA) split_enabled = (frame[5] != 0);
        } else if (cmd == 0x1C && frame.size() >= 8) {
            if (frame[5] == 0x00) ptt_active = (frame[6] != 0);
            else if (frame[5] == 0x01) {
                atu_enabled = (frame[6] != 0);
                atu_tuning = (frame[6] == 0x02);
            }
        }
    }

    void worker_loop() {
        uint64_t tick = 0;
        while (running_.load()) {
            std::string target_port;
            int baud = 19200;
            bool do_reconnect = false;
            {
                std::lock_guard<std::mutex> lk(mtx);
                target_port = serial_port;
                baud = baud_rate;
                do_reconnect = reconnect_requested_;
                reconnect_requested_ = false;
            }

            // Periodically re-scan USB ports if disconnected
            if (!is_serial_open() && (do_reconnect || (tick % 15 == 0))) {
                scan_serial_ports();
                {
                    std::lock_guard<std::mutex> lk(mtx);
                    target_port = serial_port;
                    baud = baud_rate;
                }
                if (target_port.empty()) {
                    std::lock_guard<std::mutex> lk(mtx);
                    set_offline_state_locked("OFFLINE — No USB CAT serial cable detected");
                } else {
                    bool ok = open_serial_port(target_port, baud);
                    if (!ok) {
                        std::lock_guard<std::mutex> lk(mtx);
                        set_offline_state_locked("OFFLINE — Cannot open " + target_port);
                    } else {
                        // Immediately probe the radio with G90 initial state queries
                        static const std::vector<std::vector<uint8_t>> kInitPolls = {
                            {0x03},       // VFO frequency
                            {0x04},       // Mode & filter
                            {0x15, 0x02}, // S-Meter
                            {0x14, 0x0A}, // RF Power setting
                            {0x14, 0x01}, // Volume
                            {0x14, 0x03}, // Squelch
                            {0x16, 0x12}, // AGC
                            {0x16, 0x02}, // Preamp
                            {0x11},       // Attenuator
                            {0x16, 0x22}, // Noise blanker
                            {0x0F}        // Split
                        };
                        bool any_reply = false;
                        for (const auto& p : kInitPolls) {
                            auto req = build_civ_frame(p);
                            auto frames = transact_civ_frames(req, 90);
                            std::lock_guard<std::mutex> lk(mtx);
                            if (p[0] == 0x03 || p[0] == 0x04) {
                                log_trace_locked(true, req, p[0] == 0x03 ? "init get_vfoA()" : "init get_modeA()");
                            }
                            for (const auto& fr : frames) {
                                any_reply = true;
                                parse_rx_civ_frame_locked(fr, p[0] == 0x03 || p[0] == 0x04);
                            }
                            if (!any_reply && p[0] == 0x03) break; // Radio is powered off
                        }
                        if (!any_reply) {
                            std::lock_guard<std::mutex> lk(mtx);
                            set_offline_state_locked("OFFLINE — " + target_port + " open, waiting for G90 power/CI-V...");
                        }
                    }
                }
            }

            // Drain outgoing user CAT commands
            std::vector<QueuedCmd> pending;
            {
                std::lock_guard<std::mutex> lk(mtx);
                while (!cmd_queue_.empty()) {
                    pending.push_back(std::move(cmd_queue_.front()));
                    cmd_queue_.pop_front();
                }
            }

            for (const auto& cmd : pending) {
                if (is_serial_open()) {
                    bool is_explicit_poll = (cmd.desc.find("Custom") != std::string::npos || cmd.desc.find("get_") != std::string::npos);
                    auto frames = transact_civ_frames(cmd.bytes, 120);
                    std::lock_guard<std::mutex> lk(mtx);
                    for (const auto& fr : frames) {
                        parse_rx_civ_frame_locked(fr, is_explicit_poll);
                    }
                }
            }

            // Live hardware polling loop (VFO, Mode, S-Meter, Power, SWR, Controls)
            if (is_serial_open()) {
                std::vector<uint8_t> poll_payload;
                bool is_tx = false;
                {
                    std::lock_guard<std::mutex> lk(mtx);
                    is_tx = ptt_active;
                }
                if (is_tx) {
                    // During TX, rapidly poll Power Out (0x15 0x11), SWR (0x15 0x12), and PTT (0x1C 0x00)
                    switch (tick % 3) {
                        case 0: poll_payload = {0x15, 0x11}; break;
                        case 1: poll_payload = {0x15, 0x12}; break;
                        default:poll_payload = {0x03};       break;
                    }
                } else {
                    // During RX, poll VFO (0x03), S-Meter (0x15 0x02), Mode (0x04), and front-panel controls in rotation
                    switch (tick % 8) {
                        case 0: poll_payload = {0x03};       break; // VFO frequency
                        case 1: poll_payload = {0x15, 0x02}; break; // S-Meter
                        case 2: poll_payload = {0x04};       break; // Mode & Filter
                        case 3: poll_payload = {0x15, 0x02}; break; // S-Meter
                        case 4: poll_payload = {0x14, 0x0A}; break; // RF Power setting
                        case 5: poll_payload = {0x15, 0x02}; break; // S-Meter
                        case 6: poll_payload = {0x16, 0x12}; break; // AGC
                        default:poll_payload = {0x16, 0x02}; break; // Preamp
                    }
                }

                auto req = build_civ_frame(poll_payload);
                auto frames = transact_civ_frames(req, 90);
                std::lock_guard<std::mutex> lk(mtx);
                if (!frames.empty()) {
                    for (const auto& fr : frames) {
                        parse_rx_civ_frame_locked(fr, false);
                    }
                    if (!ptt_active) {
                        power_out_watts = 0.0f;
                    }
                    if (tick % 2 == 0 && connected) {
                        smeter_history.push_back(static_cast<float>(smeter_pct));
                        if (smeter_history.size() > 80) smeter_history.pop_front();
                        power_history.push_back(power_out_watts);
                        if (power_history.size() > 80) power_history.pop_front();
                        swr_history.push_back(swr_ratio);
                        if (swr_history.size() > 80) swr_history.pop_front();
                    }
                } else {
                    ++missed_polls_;
                    if (missed_polls_ >= 6) {
                        set_offline_state_locked("OFFLINE — Xiegu G90 not responding on " + serial_port);
                    }
                }
            }

            ++tick;
            std::this_thread::sleep_for(std::chrono::milliseconds(60));
        }
    }

    // Built-in FLRig XML-RPC Server on 127.0.0.1:12345 (compatible with WSJT-X / JS8Call / Fldigi)
    void xmlrpc_server_loop() {
        ensure_winsock();
        hs_socket_t srv = ::socket(AF_INET, SOCK_STREAM, 0);
        if (!hs_sock_valid(srv)) return;
        hs_set_reuseaddr(srv);
        sockaddr_in addr{};
        addr.sin_family = AF_INET;
        addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
        addr.sin_port = htons(static_cast<uint16_t>(xmlrpc_port));
        if (::bind(srv, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) != 0 || ::listen(srv, 4) != 0) {
            hs_close_socket(srv);
            return;
        }
        hs_set_nonblocking(srv);

        while (running_.load()) {
            if (hs_poll_read(srv, 150) > 0) {
                sockaddr_in cli{};
                socklen_t clen = sizeof(cli);
                hs_socket_t cfd = ::accept(srv, reinterpret_cast<sockaddr*>(&cli), &clen);
                if (hs_sock_valid(cfd)) {
                    hs_set_rw_timeout_ms(cfd, 500);
                    char buf[2048]{};
                    hs_ssize_t n = ::recv(cfd, buf, static_cast<int>(sizeof(buf) - 1), 0);
                    if (n > 0) {
                        std::string req(buf, static_cast<size_t>(n));
                        std::string val_xml = "<value>OK</value>";
                        {
                            std::lock_guard<std::mutex> lk(mtx);
                            if (req.find("main.get_version") != std::string::npos) {
                                val_xml = "<value>2.0.05</value>";
                            } else if (req.find("rig.get_xcvr") != std::string::npos) {
                                val_xml = std::string("<value>") + (connected ? model_name(model) : "NONE") + "</value>";
                            } else if (req.find("rig.get_vfo") != std::string::npos || req.find("main.get_vfo") != std::string::npos) {
                                val_xml = "<value>" + std::to_string(connected ? (active_vfo_b ? vfo_b_hz : vfo_a_hz) : 0ULL) + "</value>";
                            } else if (req.find("rig.get_mode") != std::string::npos) {
                                val_xml = std::string("<value>") + (connected ? mode_name(active_vfo_b ? mode_b : mode_a) : "NONE") + "</value>";
                            } else if (req.find("rig.get_ptt") != std::string::npos) {
                                val_xml = "<value><i4>" + std::to_string((connected && ptt_active) ? 1 : 0) + "</i4></value>";
                            } else if (req.find("rig.get_smeter") != std::string::npos) {
                                val_xml = "<value><i4>" + std::to_string(connected ? smeter_pct : 0) + "</i4></value>";
                            } else if (req.find("rig.get_pwrmeter") != std::string::npos) {
                                val_xml = "<value><i4>" + std::to_string(connected ? static_cast<int>(power_out_watts) : 0) + "</i4></value>";
                            }
                        }
                        std::string body = "<?xml version=\"1.0\"?>\r\n<methodResponse><params><param>" +
                                           val_xml + "</param></params></methodResponse>\r\n";
                        std::string resp = "HTTP/1.1 200 OK\r\nContent-Type: text/xml\r\nContent-Length: " +
                                           std::to_string(body.size()) + "\r\nConnection: close\r\n\r\n" + body;
                        (void)::send(cfd, resp.data(), static_cast<int>(resp.size()), 0);
                    }
                    hs_close_socket(cfd);
                }
            }
        }
        hs_close_socket(srv);
    }
};

} // namespace hamshack
