#pragma once
// ============================================================================
// modem_ft8_ft4.hpp — WSJT-X FT8 & FT4 77-Bit Codec, LDPC(174,91) BP Decoder,
// 8-GFSK / 4-GFSK Waveform Generator, Spectrogram Demodulator & QSO Sequencer.
// Ported from wsjtx/lib/ft8/, wsjtx/lib/ft4/, and wsjtx/lib/77bit/packjt77.f90
// ============================================================================

#include <algorithm>
#include <array>
#include <atomic>
#include <cmath>
#include <complex>
#include <condition_variable>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <ctime>
#include <deque>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <mutex>
#include <sstream>
#include <string>
#include <thread>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "../src/soundcard_audio.hpp"
#include "psk_reporter.hpp"

namespace hamshack {

// Exact 83x91 LDPC(174,91) generator matrix rows from wsjtx/lib/ft8/ldpc_174_91_c_generator.f90
static constexpr const char* kFt8LdpcGenHex[83] = {
    "8329ce11bf31eaf509f27fc", "761c264e25c259335493132", "dc265902fb277c6410a1bdc",
    "1b3f417858cd2dd33ec7f62", "09fda4fee04195fd034783a", "077cccc11b8873ed5c3d48a",
    "29b62afe3ca036f4fe1a9da", "6054faf5f35d96d3b0c8c3e", "e20798e4310eed27884ae90",
    "775c9c08e80e26ddae56318", "b0b811028c2bf997213487c", "18a0c9231fc60adf5c5ea32",
    "76471e8302a0721e01b12b8", "ffbccb80ca8341fafb47b2e", "66a72a158f9325a2bf67170",
    "c4243689fe85b1c51363a18", "0dff739414d1a1b34b1c270", "15b48830636c8b99894972e",
    "29a89c0d3de81d665489b0e", "4f126f37fa51cbe61bd6b94", "99c47239d0d97d3c84e0940",
    "1919b75119765621bb4f1e8", "09db12d731faee0b86df6b8", "488fc33df43fbdeea4eafb4",
    "827423ee40b675f756eb5fe", "abe197c484cb74757144a9a", "2b500e4bc0ec5a6d2bdbdd0",
    "c474aa53d70218761669360", "8eba1a13db3390bd6718cec", "753844673a27782cc42012e",
    "06ff83a145c37035a5c1268", "3b37417858cc2dd33ec3f62", "9a4a5a28ee17ca9c324842c",
    "bc29f465309c977e89610a4", "2663ae6ddf8b5ce2bb29488", "46f231efe457034c1814418",
    "3fb2ce85abe9b0c72e06fbe", "de87481f282c153971a0a2e", "fcd7ccf23c69fa99bba1412",
    "f0261447e9490ca8e474cec", "4410115818196f95cdd7012", "088fc31df4bfbde2a4eafb4",
    "b8fef1b6307729fb0a078c0", "5afea7acccb77bbc9d99a90", "49a7016ac653f65ecdc9076",
    "1944d085be4e7da8d6cc7d0", "251f62adc4032f0ee714002", "56471f8702a0721e00b12b8",
    "2b8e4923f2dd51e2d537fa0", "6b550a40a66f4755de95c26", "a18ad28d4e27fe92a4f6c84",
    "10c2e586388cb82a3d80758", "ef34a41817ee02133db2eb0", "7e9c0c54325a9c15836e000",
    "3693e572d1fde4cdf079e86", "bfb2cec5abe1b0c72e07fbe", "7ee18230c583cccc57d4b08",
    "a066cb2fedafc9f52664126", "bb23725abc47cc5f4cc4cd2", "ded9dba3bee40c59b5609b4",
    "d9a7016ac653e6decdc9036", "9ad46aed5f707f280ab5fc4", "e5921c77822587316d7d3c2",
    "4f14da8242a8b86dca73352", "8b8b507ad467d4441df770e", "22831c9cf1169467ad04b68",
    "213b838fe2ae54c38ee7180", "5d926b6dd71f085181a4e12", "66ab79d4b29ee6e69509e56",
    "958148682d748a38dd68baa", "b8ce020cf069c32a723ab14", "f4331d6d461607e95752746",
    "6da23ba424b9596133cf9c8", "a636bcbc7b30c5fbeae67fe", "5cb0d86a07df654a9089a20",
    "f11f106848780fc9ecdd80a", "1fbb5364fb8d2c9d730d5ba", "fcb86bc70a50c9d02a5d034",
    "a534433029eac15f322e34c", "c989d9c7c3d3b8c55d75130", "7bb38b2f0186d46643ae962",
    "2644ebadeb44b9467d1f42c", "608cc857594bfbb55d69600"
};

// Exact parity-check rows Nm (83 checks x up to 7 variable nodes, 1-based) from ldpc_174_91_c_parity.f90
static constexpr uint8_t kFt8Nm[83][7] = {
    {  4,  31,  59,  91,  92,  96, 153}, {  5,  32,  60,  93, 115, 146,   0},
    {  6,  24,  61,  94, 122, 151,   0}, {  7,  33,  62,  95,  96, 143,   0},
    {  8,  25,  63,  83,  93,  96, 148}, {  6,  32,  64,  97, 126, 138,   0},
    {  5,  34,  65,  78,  98, 107, 154}, {  9,  35,  66,  99, 139, 146,   0},
    { 10,  36,  67, 100, 107, 126,   0}, { 11,  37,  67,  87, 101, 139, 158},
    { 12,  38,  68, 102, 105, 155,   0}, { 13,  39,  69, 103, 149, 162,   0},
    {  8,  40,  70,  82, 104, 114, 145}, { 14,  41,  71,  88, 102, 123, 156},
    { 15,  42,  59, 106, 123, 159,   0}, {  1,  33,  72, 106, 107, 157,   0},
    { 16,  43,  73, 108, 141, 160,   0}, { 17,  37,  74,  81, 109, 131, 154},
    { 11,  44,  75, 110, 121, 166,   0}, { 45,  55,  64, 111, 130, 161, 173},
    {  8,  46,  71, 112, 119, 166,   0}, { 18,  36,  76,  89, 113, 114, 143},
    { 19,  38,  77, 104, 116, 163,   0}, { 20,  47,  70,  92, 138, 165,   0},
    {  2,  48,  74, 113, 128, 160,   0}, { 21,  45,  78,  83, 117, 121, 151},
    { 22,  47,  58, 118, 127, 164,   0}, { 16,  39,  62, 112, 134, 158,   0},
    { 23,  43,  79, 120, 131, 145,   0}, { 19,  35,  59,  73, 110, 125, 161},
    { 20,  36,  63,  94, 136, 161,   0}, { 14,  31,  79,  98, 132, 164,   0},
    {  3,  44,  80, 124, 127, 169,   0}, { 19,  46,  81, 117, 135, 167,   0},
    {  7,  49,  58,  90, 100, 105, 168}, { 12,  50,  61, 118, 119, 144,   0},
    { 13,  51,  64, 114, 118, 157,   0}, { 24,  52,  76, 129, 148, 149,   0},
    { 25,  53,  69,  90, 101, 130, 156}, { 20,  46,  65,  80, 120, 140, 170},
    { 21,  54,  77, 100, 140, 171,   0}, { 35,  82, 133, 142, 171, 174,   0},
    { 14,  30,  83, 113, 125, 170,   0}, {  4,  29,  68, 120, 134, 173,   0},
    {  1,   4,  52,  57,  86, 136, 152}, { 26,  51,  56,  91, 122, 137, 168},
    { 52,  84, 110, 115, 145, 168,   0}, {  7,  50,  81,  99, 132, 173,   0},
    { 23,  55,  67,  95, 172, 174,   0}, { 26,  41,  77, 109, 141, 148,   0},
    {  2,  27,  41,  61,  62, 115, 133}, { 27,  40,  56, 124, 125, 126,   0},
    { 18,  49,  55, 124, 141, 167,   0}, {  6,  33,  85, 108, 116, 156,   0},
    { 28,  48,  70,  85, 105, 129, 158}, {  9,  54,  63, 131, 147, 155,   0},
    { 22,  53,  68, 109, 121, 174,   0}, {  3,  13,  48,  78,  95, 123,   0},
    { 31,  69, 133, 150, 155, 169,   0}, { 12,  43,  66,  89,  97, 135, 159},
    {  5,  39,  75, 102, 136, 167,   0}, {  2,  54,  86, 101, 135, 164,   0},
    { 15,  56,  87, 108, 119, 171,   0}, { 10,  44,  82,  91, 111, 144, 149},
    { 23,  34,  71,  94, 127, 153,   0}, { 11,  49,  88,  92, 142, 157,   0},
    { 29,  34,  87,  97, 147, 162,   0}, { 30,  50,  60,  86, 137, 142, 162},
    { 10,  53,  66,  84, 112, 128, 165}, { 22,  57,  85,  93, 140, 159,   0},
    { 28,  32,  72, 103, 132, 166,   0}, { 28,  29,  84,  88, 117, 143, 150},
    {  1,  26,  45,  80, 128, 147,   0}, { 17,  27,  89, 103, 116, 153,   0},
    { 51,  57,  98, 163, 165, 172,   0}, { 21,  37,  73, 138, 152, 169,   0},
    { 16,  47,  76, 130, 137, 154,   0}, {  3,  24,  30,  72, 104, 139,   0},
    {  9,  40,  90, 106, 134, 151,   0}, { 15,  58,  60,  74, 111, 150, 163},
    { 18,  42,  79, 144, 146, 152,   0}, { 25,  38,  65,  99, 122, 160,   0},
    { 17,  42,  75, 129, 170, 172,   0}
};

static constexpr int kFt8Costas7[7] = {3, 1, 4, 0, 6, 5, 2};
static constexpr int kFt8GrayMap[8] = {0, 1, 3, 2, 5, 6, 4, 7};
static constexpr int kFt8InvGray[8] = {0, 1, 3, 2, 6, 4, 5, 7};

static constexpr int kFt4Costas[4][4] = {
    {0, 1, 3, 2},
    {1, 0, 2, 3},
    {2, 3, 1, 0},
    {3, 2, 0, 1}
};

static constexpr uint8_t kFt4Rvec[77] = {
    0,1,0,0,1,0,1,0,0,1,0,1,1,1,1,0,1,0,0,0,1,0,0,1,1,0,1,1,0,
    1,0,0,1,0,1,1,0,0,0,0,1,0,0,0,1,0,1,0,0,1,1,1,1,0,0,1,0,1,
    0,1,0,1,0,1,1,0,1,1,1,1,1,0,0,0,1,0,1
};

struct DecodedFtMessage {
    std::string utc_str;    // e.g. "180515"
    int snr_db = -10;
    float dt_sec = 0.1f;
    int freq_hz = 1200;
    bool is_ft4 = false;
    std::string text;       // e.g. "CQ K1ABC FN42"
    bool is_cq = false;
    bool is_for_me = false;
    bool is_tx = false;
    bool is_even_slot = false;
    int slot_id = 0;

    bool determine_rx_is_even() const {
        if (slot_id != 0) return (slot_id % 2) == 0;
        if (utc_str.size() >= 6) {
            int sec = std::atoi(utc_str.substr(4, 2).c_str());
            return is_ft4 ? (((sec * 2 / 15) % 2) == 0) : (((sec / 15) % 2) == 0);
        }
        return is_even_slot;
    }
};

struct LoggedQso {
    std::string date_ymd;
    std::string time_utc;
    std::string call;
    std::string grid;
    std::string mode;
    uint64_t freq_hz = 14074000ULL;
    std::string rst_sent;
    std::string rst_rcvd;
    std::string band;
    bool confirmed = false;
};

class Ft8Ft4Engine {
public:
    mutable std::recursive_mutex mtx;

    // Mode & QSO State
    bool is_ft4 = false;            // false = FT8 (15s, 79 sym), true = FT4 (7.5s, 105 sym)
    std::string my_call = "W1AW";
    std::string my_grid = "FN31";
    std::string dx_call = "";
    std::string dx_grid = "";
    std::string my_cq_modifier = "";
    int rpt_sent = -10;
    int rpt_rcvd = -10;

    int rx_freq_hz = 1200;
    int tx_freq_hz = 1200;
    bool hold_tx_freq = true;
    bool tx_even_first = true;
    bool auto_seq = true;
    bool call_first = true;
    bool monitor_enabled = true;
    bool tx_enabled = false;
    bool transmitting_now = false;
    bool tune_active = false;

    int active_tx_slot = 5;         // 0..5 = Tx1..Tx6 (default Tx6 = CQ)
    std::array<std::string, 6> tx_msgs;

    std::atomic<bool> qso_prompt_log_pending{false};
    std::atomic<uint64_t> cached_dial_hz{14074000ULL};

    std::deque<DecodedFtMessage> band_activity;
    std::deque<DecodedFtMessage> rx_freq_activity;
    std::vector<LoggedQso> qso_log;

    // Worked Stations & Grids Database (populated from WSJT-X .adi & live QSOs)
    std::unordered_set<std::string> worked_calls;
    std::unordered_set<std::string> worked_grids;
    std::unordered_set<std::string> confirmed_grids;
    std::unordered_set<std::string> imported_qso_keys;
    std::string adif_status_text = "Scanning for WSJT-X .adi...";

    // Cycle timing
    float cycle_progress = 0.0f;    // 0.0 .. 1.0
    float cycle_sec = 0.0f;
    int current_slot_id = -1;
    int last_decoded_slot_id = -1;
    int last_late_decoded_slot_id = -1;
    int last_tx_slot_id = -1;
    mutable std::unordered_map<uint32_t, std::string> hash22_calls;
    mutable std::unordered_map<uint32_t, std::string> hash12_calls;
    mutable std::unordered_map<uint32_t, std::string> hash10_calls;

    std::atomic<bool> decode_in_progress{false};

    Ft8Ft4Engine() {
        init_ldpc_tables();
        regenerate_std_msgs();
        auto_import_wsjtx_adif();
    }

    ~Ft8Ft4Engine() {
        stop_decode_worker();
    }

    void stop_decode_worker() {
        {
            std::lock_guard<std::mutex> lk(decode_job_mtx_);
            decode_worker_stop_ = true;
        }
        decode_job_cv_.notify_all();
        if (decode_worker_.joinable()) {
            decode_worker_.join();
        }
    }

    // Exact WSJT-X CRC-14 (polynomial 0x2757 over 96 bits: 77 msg bits + 3 zeros + 16 zero bits)
    static uint16_t compute_crc14(const std::array<uint8_t, 77>& msg77) {
        uint8_t bytes[12]{};
        for (int i = 0; i < 77; ++i) {
            if (msg77[i]) {
                bytes[i / 8] |= static_cast<uint8_t>(1u << (7 - (i % 8)));
            }
        }
        uint16_t reg = 0;
        constexpr uint16_t poly = 0x2757;
        for (int i = 0; i < 12; ++i) {
            for (int b = 7; b >= 0; --b) {
                uint16_t bit = (bytes[i] >> b) & 1u;
                uint16_t top = (reg >> 13) & 1u;
                reg = static_cast<uint16_t>(((reg << 1) | bit) & 0x3FFFu);
                if (top) reg ^= poly;
            }
        }
        return reg & 0x3FFFu;
    }

    // Pack/Unpack 77-bit FT8/FT4 payload using standard Type 1 or Free Text (71-bit base-42 + 6-bit zero header)
    static void pack77_message(const std::string& raw_msg, std::array<uint8_t, 77>& bits) {
        bits.fill(0);
        // Pack up to 13 characters in WSJT-X 42-character alphabet (or full 71-bit hash/text)
        // To preserve full 28-char standard FT8 messages cleanly across our loopback & over-the-air GFSK codec:
        // If <= 13 chars, use exact WSJT-X base-42 `packtext77` (`i3=0, n3=0`);
        // If > 13 chars, Try standard Type-1 `<CALL1> <CALL2> <GRID/RPT>` packing (`i3=1`).
        std::string s = trim_upper(raw_msg);
        if (pack_type1_std(s, bits)) return;

        // Exact WSJT-X `packtext77` base-42 big-integer encoding for free text (13 chars -> 71 bits, n3=0, i3=0)
        static const char* ALPH = " 0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ+-./?";
        s.resize(13, ' ');
        uint8_t num[9]{}; // 72-bit big-endian accumulator
        for (int c_idx = 0; c_idx < 13; ++c_idx) {
            const char* p = std::strchr(ALPH, s[c_idx]);
            uint32_t v = p ? static_cast<uint32_t>(p - ALPH) : 0u;
            uint32_t carry = v;
            for (int b = 8; b >= 0; --b) {
                uint32_t cur = static_cast<uint32_t>(num[b]) * 42u + carry;
                num[b] = static_cast<uint8_t>(cur & 0xFFu);
                carry = cur >> 8;
            }
        }
        // Write 71 bits into bits[0..70], bits[71..76] = 0 (n3=0, i3=0)
        for (int i = 0; i < 71; ++i) {
            int bit_pos = 70 - i;
            int byte_idx = 8 - (bit_pos / 8);
            int bit_in_byte = bit_pos % 8;
            bits[i] = (num[byte_idx] >> bit_in_byte) & 1u;
        }
    }

    std::string unpack77_message(const std::array<uint8_t, 77>& bits) const {
        auto read_bits = [&](int offset, int len) -> uint64_t {
            uint64_t v = 0;
            for (int i = 0; i < len; ++i) v = (v << 1) | (bits[offset + i] & 1u);
            return v;
        };
        int i3 = (bits[74] << 2) | (bits[75] << 1) | bits[76];
        int n3 = (bits[71] << 2) | (bits[72] << 1) | bits[73];
        if (i3 == 1 || i3 == 2) {
            std::string out;
            if (unpack_type1_std(bits, out, i3)) return out;
        }
        if (i3 == 4) {
            // Type 4: Nonstandard callsign (12-bit hash + 58-bit base-38 callsign + 1-bit flip + 2-bit rpt + 1-bit CQ)
            static const char* C38 = " 0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ/";
            uint32_t n12 = static_cast<uint32_t>(read_bits(0, 12));
            uint64_t n58 = read_bits(12, 58);
            uint32_t iflip = static_cast<uint32_t>(read_bits(70, 1));
            uint32_t nrpt  = static_cast<uint32_t>(read_bits(71, 2));
            uint32_t icq   = static_cast<uint32_t>(read_bits(73, 1));
            std::string c11(11, ' ');
            for (int i = 10; i >= 0; --i) {
                c11[i] = C38[n58 % 38ULL];
                n58 /= 38ULL;
            }
            c11 = trim_upper(c11);
            if (c11.size() < 3) return "";
            save_hash_call_wsjtx(c11);
            std::string hcall = "<...>";
            auto it = hash12_calls.find(n12);
            if (it != hash12_calls.end()) hcall = "<" + it->second + ">";
            else if (!my_call.empty() && ihashcall_wsjtx(my_call, 12) == n12) hcall = "<" + my_call + ">";
            if (icq == 1) return "CQ " + c11;
            std::string call_1 = (iflip == 0) ? hcall : c11;
            std::string call_2 = (iflip == 0) ? c11   : hcall;
            if (nrpt == 0) return call_1 + " " + call_2;
            if (nrpt == 1) return call_1 + " " + call_2 + " RRR";
            if (nrpt == 2) return call_1 + " " + call_2 + " RR73";
            if (nrpt == 3) return call_1 + " " + call_2 + " 73";
        }
        if (i3 == 0 && n3 == 0) {
            static const char* ALPH = " 0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ+-./?";
            uint8_t num[9]{};
            for (int i = 0; i < 71; ++i) {
                if (bits[i]) {
                    int bit_pos = 70 - i;
                    int byte_idx = 8 - (bit_pos / 8);
                    int bit_in_byte = bit_pos % 8;
                    num[byte_idx] |= static_cast<uint8_t>(1u << bit_in_byte);
                }
            }
            std::string out(13, ' ');
            for (int c_idx = 12; c_idx >= 0; --c_idx) {
                uint32_t rem = 0;
                for (int b = 0; b < 9; ++b) {
                    uint32_t cur = (rem << 8) | num[b];
                    num[b] = static_cast<uint8_t>(cur / 42u);
                    rem = cur % 42u;
                }
                out[c_idx] = ALPH[rem];
            }
            return trim_upper(out);
        }
        return "";
    }

    // Exact WSJT-X `encode174_91` (77-bit message -> +14-bit CRC = 91 bits -> +83 LDPC parity bits = 174-bit codeword)
    void encode174_91(const std::array<uint8_t, 77>& msg77, std::array<uint8_t, 174>& codeword) const {
        std::array<uint8_t, 91> msg91{};
        for (int i = 0; i < 77; ++i) msg91[i] = msg77[i] & 1u;
        uint16_t crc = compute_crc14(msg77);
        for (int i = 0; i < 14; ++i) {
            msg91[77 + i] = (crc >> (13 - i)) & 1u;
        }
        for (int i = 0; i < 91; ++i) codeword[i] = msg91[i];
        for (int r = 0; r < 83; ++r) {
            uint8_t sum = 0;
            for (int c = 0; c < 91; ++c) {
                sum ^= (msg91[c] & gen_mat_[r][c]);
            }
            codeword[91 + r] = sum;
        }
    }

    // Exact WSJT-X `bpdecode174_91` log-domain belief propagation LDPC decoder
    bool bpdecode174_91(const std::array<float, 174>& llr,
                        std::array<uint8_t, 77>& out_msg77,
                        int max_iters = 35) const {
        float tov[174][3]{};
        float toc[83][7]{};
        float zn[174]{};
        uint8_t cw[174]{};

        for (int j = 0; j < 83; ++j) {
            for (int i = 0; i < nrw_[j]; ++i) {
                toc[j][i] = llr[kFt8Nm[j][i] - 1];
            }
        }

        for (int iter = 0; iter <= max_iters; ++iter) {
            for (int n = 0; n < 174; ++n) {
                zn[n] = llr[n] + tov[n][0] + tov[n][1] + tov[n][2];
                cw[n] = (zn[n] > 0.0f) ? 1u : 0u;
            }
            int ncheck = 0;
            for (int j = 0; j < 83; ++j) {
                int synd = 0;
                for (int i = 0; i < nrw_[j]; ++i) {
                    synd ^= cw[kFt8Nm[j][i] - 1];
                }
                if (synd) ++ncheck;
            }
            if (ncheck == 0) {
                std::array<uint8_t, 77> cand{};
                for (int i = 0; i < 77; ++i) cand[i] = cw[i];
                uint16_t rx_crc = 0;
                for (int i = 0; i < 14; ++i) rx_crc = static_cast<uint16_t>((rx_crc << 1) | cw[77 + i]);
                if (compute_crc14(cand) == rx_crc) {
                    out_msg77 = cand;
                    return true;
                }
            }
            if (iter == max_iters) break;

            // Variable -> Check messages
            for (int j = 0; j < 83; ++j) {
                for (int i = 0; i < nrw_[j]; ++i) {
                    int ibj = kFt8Nm[j][i] - 1;
                    float v = zn[ibj];
                    for (int kk = 0; kk < 3; ++kk) {
                        if (mn_[ibj][kk] == j) v -= tov[ibj][kk];
                    }
                    toc[j][i] = std::tanh(-v * 0.5f);
                }
            }
            // Check -> Variable messages
            for (int n = 0; n < 174; ++n) {
                for (int kk = 0; kk < 3; ++kk) {
                    int ichk = mn_[n][kk];
                    if (ichk < 0) continue;
                    float prod = 1.0f;
                    for (int i = 0; i < nrw_[ichk]; ++i) {
                        if (kFt8Nm[ichk][i] - 1 != n) prod *= toc[ichk][i];
                    }
                    prod = std::clamp(-prod, -0.99999f, 0.99999f);
                    tov[n][kk] = 2.0f * std::atanh(prod);
                }
            }
        }
        return false;
    }

    // Generate 79-symbol FT8 tone array (0..7) from message string (`genft8.f90`)
    std::array<int, 79> gen_ft8_tones(const std::string& msg) const {
        std::array<uint8_t, 77> bits{};
        pack77_message(msg, bits);
        std::array<uint8_t, 174> cw{};
        encode174_91(bits, cw);

        std::array<int, 79> itone{};
        for (int i = 0; i < 7; ++i) {
            itone[i] = kFt8Costas7[i];
            itone[36 + i] = kFt8Costas7[i];
            itone[72 + i] = kFt8Costas7[i];
        }
        int k = 7;
        for (int j = 0; j < 58; ++j) {
            if (j == 29) k += 7;
            int idx = (cw[3 * j] << 2) | (cw[3 * j + 1] << 1) | cw[3 * j + 2];
            itone[k++] = kFt8GrayMap[idx & 7];
        }
        return itone;
    }

    // Generate 103-symbol FT4 tone array (0..3) from message string (`genft4.f90`)
    std::array<int, 103> gen_ft4_tones(const std::string& msg) const {
        std::array<uint8_t, 77> bits{};
        pack77_message(msg, bits);
        for (int i = 0; i < 77; ++i) bits[i] ^= kFt4Rvec[i];
        std::array<uint8_t, 174> cw{};
        encode174_91(bits, cw);

        std::array<int, 103> itone{};
        for (int i = 0; i < 4; ++i) {
            itone[i] = kFt4Costas[0][i];
            itone[33 + i] = kFt4Costas[1][i];
            itone[66 + i] = kFt4Costas[2][i];
            itone[99 + i] = kFt4Costas[3][i];
        }
        for (int i = 0; i < 87; ++i) {
            int is = (cw[2 * i] << 1) | cw[2 * i + 1];
            int gray = (is <= 1) ? is : (is == 2 ? 3 : 2);
            int pos = (i < 29) ? (4 + i) : (i < 58) ? (37 + (i - 29)) : (70 + (i - 58));
            itone[pos] = gray;
        }
        return itone;
    }

    // Exact Gaussian-filtered FSK waveform synthesis (`gen_ft8wave.f90` / `gen_ft4wave.f90`) at 12,000 Hz
    static std::vector<float> synthesize_gfsk_wave(const int* tones,
                                                   int nsym,
                                                   int nsps,
                                                   float bt,
                                                   float f0_hz,
                                                   float amplitude = 0.8f) {
        constexpr float fs = 12000.0f;
        constexpr float pi = 3.14159265358979323846f;
        constexpr float twopi = 2.0f * pi;
        const float c_erf = pi * std::sqrt(2.0f / std::log(2.0f));

        std::vector<float> pulse(3 * nsps, 0.0f);
        for (int i = 0; i < 3 * nsps; ++i) {
            float tt = (static_cast<float>(i) - 1.5f * nsps) / static_cast<float>(nsps);
            pulse[i] = 0.5f * (std::erf(c_erf * bt * (tt + 0.5f)) - std::erf(c_erf * bt * (tt - 0.5f)));
        }

        std::vector<float> dphi((nsym + 2) * nsps, twopi * f0_hz / fs);
        float dphi_peak = twopi / static_cast<float>(nsps);
        for (int j = 0; j < nsym; ++j) {
            int ib = j * nsps;
            for (int i = 0; i < 3 * nsps; ++i) {
                dphi[ib + i] += dphi_peak * pulse[i] * static_cast<float>(tones[j]);
            }
        }

        int nwave = nsym * nsps;
        std::vector<float> wave(nwave, 0.0f);
        float phi = 0.0f;
        for (int j = 0; j < nwave; ++j) {
            wave[j] = amplitude * std::sin(phi);
            phi += dphi[nsps + j];
            if (phi >= twopi) phi -= twopi;
        }

        // Raised-cosine envelope ramp on first & last symbol/8
        int nramp = std::max(1, nsps / 8);
        for (int i = 0; i < nramp; ++i) {
            float w_up = 0.5f * (1.0f - std::cos(pi * static_cast<float>(i) / nramp));
            float w_dn = 0.5f * (1.0f + std::cos(pi * static_cast<float>(i) / nramp));
            wave[i] *= w_up;
            wave[nwave - nramp + i] *= w_dn;
        }
        return wave;
    }

    // Decode an FT8 79-symbol tone sequence directly through `bpdecode174_91` + `crc14` + `unpack77`
    bool decode_ft8_tones_with_ldpc(const std::array<int, 79>& rx_tones,
                                    float snr_db,
                                    std::string& out_msg) const {
        // Verify Costas sync score (max 21)
        int sync_hits = 0;
        for (int i = 0; i < 7; ++i) {
            if (rx_tones[i] == kFt8Costas7[i]) ++sync_hits;
            if (rx_tones[36 + i] == kFt8Costas7[i]) ++sync_hits;
            if (rx_tones[72 + i] == kFt8Costas7[i]) ++sync_hits;
        }
        if (sync_hits < 10) return false;

        std::array<float, 174> llr{};
        float scale = std::clamp((snr_db + 26.0f) * 0.35f, 1.5f, 8.0f);
        int k = 7;
        for (int j = 0; j < 58; ++j) {
            if (j == 29) k += 7;
            int sym = std::clamp(rx_tones[k++], 0, 7);
            int bits3 = kFt8InvGray[sym];
            llr[3 * j + 0] = ((bits3 & 4) ? +1.0f : -1.0f) * scale;
            llr[3 * j + 1] = ((bits3 & 2) ? +1.0f : -1.0f) * scale;
            llr[3 * j + 2] = ((bits3 & 1) ? +1.0f : -1.0f) * scale;
        }
        std::array<uint8_t, 77> msg77{};
        if (!bpdecode174_91(llr, msg77)) return false;
        out_msg = unpack77_message(msg77);
        return !out_msg.empty();
    }

    static std::string normalize_call_key(const std::string& raw) {
        std::string out;
        for (char c : raw) {
            if (c == '<' || c == '>' || c == ' ' || c == '\t' || c == '\r' || c == '\n') continue;
            out.push_back(static_cast<char>(std::toupper(static_cast<unsigned char>(c))));
        }
        return out;
    }

public:
    static std::vector<std::string> split_words(const std::string& s) {
        std::vector<std::string> w;
        std::stringstream ss(s);
        std::string item;
        while (ss >> item) w.push_back(item);
        return w;
    }

    static bool is_grid4_str(const std::string& g) {
        return g.size() == 4 &&
               g[0] >= 'A' && g[0] <= 'R' &&
               g[1] >= 'A' && g[1] <= 'R' &&
               g[2] >= '0' && g[2] <= '9' &&
               g[3] >= '0' && g[3] <= '9';
    }

    static std::string clean_callsign(const std::string& raw) {
        std::string s = raw;
        if (!s.empty() && s.front() == '<') s.erase(s.begin());
        if (!s.empty() && s.back() == '>') s.pop_back();
        return s;
    }

    static bool is_valid_callsign(const std::string& s_raw) {
        std::string s = clean_callsign(s_raw);
        if (s.size() < 3 || s.size() > 14) return false;
        if (s == "RR73" || s == "TEST" || s == "ROGER" || s == "POTA" || s == "SOTA" || s == "IOTA" || s == "BOTA") return false;
        bool has_letter = false;
        bool has_digit = false;
        for (char ch : s) {
            if (ch >= 'A' && ch <= 'Z') has_letter = true;
            else if (ch >= '0' && ch <= '9') has_digit = true;
            else if (ch == '/') {}
            else return false;
        }
        return has_letter && has_digit;
    }

    struct ParsedFtMessage {
        std::string call_to;     // Destination call (e.g. "N0CALL", "W7ABC", or "CQ", "QRZ", "DE")
        std::string call_from;   // Transmitting / calling station (e.g. "W7ABC")
        std::string extra;       // Grid (e.g. "EM00"), Report (e.g. "-08", "R-08"), "RR73", "73", "RRR"
        std::string cq_modifier; // e.g. "DX", "POTA", "SOTA", "IOTA", "NA", "TEST", "FD", "040", etc.
        bool is_cq = false;
        bool is_for_me = false;
    };

    static ParsedFtMessage parse_ft_message(const std::string& txt, const std::string& my_call = "") {
        ParsedFtMessage p;
        auto words = split_words(txt);
        if (words.empty()) return p;

        std::string my_norm = normalize_call_key(my_call);

        if (words[0] == "CQ" || words[0] == "QRZ" || words[0] == "DE") {
            p.is_cq = true;
            p.call_to = words[0];

            if (words.size() == 2) {
                p.call_from = clean_callsign(words[1]);
            } else if (words.size() == 3) {
                std::string w1 = clean_callsign(words[1]);
                std::string w2 = clean_callsign(words[2]);
                if (is_grid4_str(w2) && is_valid_callsign(w1)) {
                    p.call_from = w1;
                    p.extra = w2;
                } else {
                    p.cq_modifier = words[1];
                    p.call_from = w2;
                }
            } else if (words.size() >= 4) {
                std::string w1 = clean_callsign(words[1]);
                std::string w2 = clean_callsign(words[2]);
                std::string w3 = clean_callsign(words[3]);

                if (is_valid_callsign(w1) && is_grid4_str(w2)) {
                    p.call_from = w1;
                    p.extra = w2;
                } else {
                    p.cq_modifier = words[1];
                    p.call_from = w2;
                    if (is_grid4_str(w3)) p.extra = w3;
                    else p.extra = words.back();
                }
            }
        } else if (words.size() >= 2) {
            p.call_to = clean_callsign(words[0]);
            p.call_from = clean_callsign(words[1]);
            if (words.size() >= 3) {
                if (words.size() >= 4 && words[2] == "R") {
                    p.extra = "R" + words[3];
                } else {
                    p.extra = words[2];
                }
            }
            if (!my_norm.empty() && normalize_call_key(p.call_to) == my_norm) {
                p.is_for_me = true;
            }
        }
        return p;
    }

    void cycle_cq_modifier() {
        std::lock_guard<std::recursive_mutex> lk(mtx);
        if (my_cq_modifier.empty()) my_cq_modifier = "DX";
        else if (my_cq_modifier == "DX") my_cq_modifier = "POTA";
        else if (my_cq_modifier == "POTA") my_cq_modifier = "SOTA";
        else my_cq_modifier.clear();
        regenerate_std_msgs_locked();
    }

    void regenerate_std_msgs() {
        std::lock_guard<std::recursive_mutex> lk(mtx);
        regenerate_std_msgs_locked();
    }

    void regenerate_std_msgs_locked() {
        if (!dx_call.empty() && normalize_call_key(dx_call) == normalize_call_key(my_call)) {
            dx_call.clear();
        }
        std::string mc = my_call.empty() ? "N0CALL" : my_call;
        std::string mg = my_grid.empty() ? "DM62" : my_grid.substr(0, 4);
        std::string dc = dx_call;
        char rpt_s[16], rrpt_s[16];
        std::snprintf(rpt_s, sizeof(rpt_s), "%+03d", rpt_sent);
        std::snprintf(rrpt_s, sizeof(rrpt_s), "R%+03d", rpt_sent);

        if (!dc.empty()) {
            tx_msgs[0] = dc + " " + mc + " " + mg;
            tx_msgs[1] = dc + " " + mc + " " + rpt_s;
            tx_msgs[2] = dc + " " + mc + " " + rrpt_s;
            tx_msgs[3] = dc + " " + mc + " RR73";
            tx_msgs[4] = dc + " " + mc + " 73";
        } else {
            tx_msgs[0] = mc + " " + mg;
            tx_msgs[1] = mc + " " + rpt_s;
            tx_msgs[2] = mc + " R" + rpt_s;
            tx_msgs[3] = mc + " RR73";
            tx_msgs[4] = mc + " 73";
        }
        if (my_cq_modifier.empty()) {
            tx_msgs[5] = "CQ " + mc + " " + mg;
        } else {
            tx_msgs[5] = "CQ " + my_cq_modifier + " " + mc + " " + mg;
        }
    }

    void start_sequence(int slot = 0) {
        std::lock_guard<std::recursive_mutex> lk(mtx);
        active_tx_slot = std::clamp(slot, 0, 5);
        tx_enabled = true;
        auto_seq = true;
    }

    // Select a decoded station from Band Activity to start or advance a QSO
    void select_decoded_message(const DecodedFtMessage& d) {
        std::lock_guard<std::recursive_mutex> lk(mtx);
        std::string my_norm = normalize_call_key(my_call);

        ParsedFtMessage p = parse_ft_message(d.text, my_call);
        if (p.call_from.empty()) return;

        std::string cand_call = p.call_from;
        std::string cand_grid = is_grid4_str(p.extra) ? p.extra : "";
        int target_slot = 0;
        int rpt_s = d.snr_db;
        int rpt_r = 0;

        if (p.is_cq) {
            target_slot = 0; // Send Tx1: <DXCALL> <MYCALL> <MYGRID>
        } else if (p.is_for_me) {
            if (is_grid4_str(p.extra)) {
                target_slot = 1; // Send Tx2: <DXCALL> <MYCALL> <RPT>
            } else if (p.extra == "RRR" || p.extra == "RR73") {
                target_slot = 4; // Send Tx5: 73
            } else if (p.extra == "73") {
                target_slot = 5; // Back to CQ
            } else if (p.extra.rfind("R+", 0) == 0 || p.extra.rfind("R-", 0) == 0) {
                rpt_r = std::atoi(p.extra.c_str() + 1);
                target_slot = 3; // Send Tx4: RR73
            } else if (p.extra.rfind("+", 0) == 0 || p.extra.rfind("-", 0) == 0) {
                rpt_r = std::atoi(p.extra.c_str());
                target_slot = 2; // Send Tx3: R<RPT>
            }
        } else {
            target_slot = 0;
        }

        std::string cand_norm = normalize_call_key(cand_call);
        // Under NO circumstances allow calling self or empty/invalid callsign
        if (cand_norm.empty() || cand_norm == my_norm ||
            cand_norm == "CQ" || cand_norm == "QRZ" || cand_norm == "DE" || cand_norm == "DX") {
            return;
        }

        rx_freq_hz = d.freq_hz;
        if (!hold_tx_freq) tx_freq_hz = d.freq_hz;

        bool rx_is_even = d.determine_rx_is_even();
        tx_even_first = !rx_is_even;

        dx_call = cand_call;
        if (!cand_grid.empty()) dx_grid = cand_grid;
        rpt_sent = rpt_s;
        if (rpt_r != 0) rpt_rcvd = rpt_r;
        regenerate_std_msgs_locked();
        active_tx_slot = target_slot;

        rx_freq_activity.push_back(d);
        if (rx_freq_activity.size() > 150) rx_freq_activity.pop_front();
    }

    // Double-clicking a CQ starts transmission immediately if there are 10s or more left
    // in the transmission window, but ONLY if the current slot is the opposite period from the rx window.
    bool on_double_click_message(const DecodedFtMessage& d, SoundcardEngine& audio, bool& out_want_ptt) {
        std::lock_guard<std::recursive_mutex> lk(mtx);
        select_decoded_message(d);
        tx_enabled = true;

        ParsedFtMessage p = parse_ft_message(d.text, my_call);
        bool is_cq = p.is_cq || (d.text.rfind("CQ ", 0) == 0);
        if (!is_cq) {
            return false;
        }

        bool rx_is_even = d.determine_rx_is_even();
        tx_even_first = !rx_is_even;

        using namespace std::chrono;
        auto now = system_clock::now();
        double epoch_s = duration_cast<milliseconds>(now.time_since_epoch()).count() / 1000.0;
        double period = is_ft4 ? 7.5 : 15.0;
        double in_slot = std::fmod(epoch_s, period);
        int current_slot_id = static_cast<int>(std::floor(epoch_s / period));
        bool current_is_even = (current_slot_id % 2) == 0;

        // "only if its the opposite period from the rx window"
        bool is_opposite = (current_is_even != rx_is_even);
        if (!is_opposite) {
            return false; // Same period as CQ RX window -> wait for beginning of next opposite slot
        }

        // "if there's 10 seconds or more left, start the transmission please"
        double time_left = period - in_slot;
        double min_req = is_ft4 ? 5.0 : 10.0;
        if (time_left < min_req) {
            return false; // Less than 10s left in this window -> wait for next opposite slot
        }

        // Start transmission immediately!
        last_tx_slot_id = current_slot_id;
        std::string msg_to_send = tx_msgs[std::clamp(active_tx_slot, 0, 5)];
        std::vector<float> wave;
        if (is_ft4) {
            auto tones = gen_ft4_tones(msg_to_send);
            wave = synthesize_gfsk_wave(tones.data(), 103, 576, 1.0f, static_cast<float>(tx_freq_hz));
        } else {
            auto tones = gen_ft8_tones(msg_to_send);
            wave = synthesize_gfsk_wave(tones.data(), 79, 1920, 2.0f, static_cast<float>(tx_freq_hz));
        }

        audio.halt_tx();
        audio.queue_tx_samples(wave);
        audio.add_synth_waterfall_signal(static_cast<float>(tx_freq_hz), is_ft4 ? 85.0f : 50.0f, +6.0f, is_ft4 ? 1 : 0,
                                         static_cast<uint64_t>(time_left * 1000));

        std::time_t tt = system_clock::to_time_t(now);
        std::tm gm{};
        hs_gmtime(&tt, &gm);
        char cur_utc[16];
        std::snprintf(cur_utc, sizeof(cur_utc), "%02d%02d%02d", gm.tm_hour, gm.tm_min, (gm.tm_sec / 15) * 15);
        DecodedFtMessage tx_rec{cur_utc, 0, 0.0f, tx_freq_hz, is_ft4, msg_to_send,
                                msg_to_send.rfind("CQ ", 0) == 0, false, true, current_is_even, current_slot_id};
        rx_freq_activity.push_back(tx_rec);
        if (rx_freq_activity.size() > 150) rx_freq_activity.pop_front();
        last_sent_msg_ = msg_to_send;

        transmitting_now = true;
        out_want_ptt = true;
        return true;
    }

    // Halts transmission and clears the current station being worked if no QSO was made
    void halt_tx_and_clear_working(SoundcardEngine& audio) {
        std::lock_guard<std::recursive_mutex> lk(mtx);
        tx_enabled = false;
        tune_active = false;
        transmitting_now = false;
        audio.halt_tx();
        dx_call.clear();
        dx_grid.clear();
        active_tx_slot = 5; // Reset sequence back to calling CQ
        regenerate_std_msgs_locked();
    }

    void record_worked_station_locked(const std::string& call, const std::string& grid, bool is_qsl = false) {
        std::string ck = normalize_call_key(call);
        if (ck.size() >= 3 && ck != "DX" && ck != "--" && ck != "...") {
            worked_calls.insert(ck);
            // Also index base callsign without /P, /M, /QRP suffix/prefix
            size_t sl = ck.find('/');
            if (sl != std::string::npos) {
                std::string p1 = ck.substr(0, sl);
                std::string p2 = ck.substr(sl + 1);
                const std::string& base = (p1.size() >= p2.size()) ? p1 : p2;
                if (base.size() >= 3) worked_calls.insert(base);
            }
            save_hash_call_wsjtx(ck);
        }
        if (grid.size() >= 4) {
            std::string g4 = trim_upper(grid.substr(0, 4));
            if (is_grid4_str(g4)) {
                worked_grids.insert(g4);
                if (is_qsl) confirmed_grids.insert(g4);
            }
        }
    }

    bool is_call_worked(const std::string& call) const {
        std::string ck = normalize_call_key(call);
        if (ck.size() < 3 || ck == "--") return false;
        std::lock_guard<std::recursive_mutex> lk(mtx);
        if (worked_calls.count(ck) > 0) return true;
        size_t sl = ck.find('/');
        if (sl != std::string::npos) {
            std::string p1 = ck.substr(0, sl);
            std::string p2 = ck.substr(sl + 1);
            const std::string& base = (p1.size() >= p2.size()) ? p1 : p2;
            if (base.size() >= 3 && worked_calls.count(base) > 0) return true;
        }
        return false;
    }

    bool is_grid_worked(const std::string& g) const {
        if (g.size() < 4) return false;
        std::string g4 = trim_upper(g.substr(0, 4));
        std::lock_guard<std::recursive_mutex> lk(mtx);
        return worked_grids.count(g4) > 0;
    }

    bool is_grid_confirmed(const std::string& g) const {
        if (g.size() < 4) return false;
        std::string g4 = trim_upper(g.substr(0, 4));
        std::lock_guard<std::recursive_mutex> lk(mtx);
        return confirmed_grids.count(g4) > 0;
    }

    // Parse and import standard ADIF (.adi / .adif) file (WSJT-X, GridTracker, JS8Call, etc.)
    size_t import_adif_file(const std::string& filepath) {
        std::ifstream f(filepath, std::ios::in | std::ios::binary);
        if (!f) return 0;
        std::string content((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
        if (content.empty()) return 0;

        std::lock_guard<std::recursive_mutex> lk(mtx);
        return parse_adif_content_locked(content, filepath);
    }

    size_t parse_adif_content_locked(const std::string& content, const std::string& src_label) {
        size_t pos = 0;
        // Skip optional ADIF header before <EOH>
        for (size_t i = 0; i + 4 < content.size(); ++i) {
            if (content[i] == '<' &&
                std::toupper(static_cast<unsigned char>(content[i + 1])) == 'E' &&
                std::toupper(static_cast<unsigned char>(content[i + 2])) == 'O' &&
                std::toupper(static_cast<unsigned char>(content[i + 3])) == 'H' &&
                content[i + 4] == '>') {
                pos = i + 5;
                break;
            }
        }

        size_t added = 0;
        LoggedQso cur{};
        bool has_fields = false;

        while (pos < content.size()) {
            size_t lt = content.find('<', pos);
            if (lt == std::string::npos) break;
            size_t gt = content.find('>', lt + 1);
            if (gt == std::string::npos) break;

            std::string tag_spec = content.substr(lt + 1, gt - lt - 1);
            pos = gt + 1;

            std::string tag_name = tag_spec;
            size_t val_len = 0;
            size_t colon = tag_spec.find(':');
            if (colon != std::string::npos) {
                tag_name = tag_spec.substr(0, colon);
                val_len = static_cast<size_t>(std::atoi(tag_spec.c_str() + colon + 1));
            }
            for (char& c : tag_name) c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));

            if (tag_name == "EOR") {
                if (has_fields && !cur.call.empty()) {
                    cur.call = normalize_call_key(cur.call);
                    cur.grid = trim_upper(cur.grid);
                    if (cur.grid.size() > 4) cur.grid = cur.grid.substr(0, 4);
                    std::string dedup = cur.call + "|" + cur.date_ymd + "|" + cur.time_utc.substr(0, 4) + "|" + cur.mode;
                    record_worked_station_locked(cur.call, cur.grid, cur.confirmed);
                    if (imported_qso_keys.insert(dedup).second) {
                        qso_log.push_back(cur);
                        ++added;
                    }
                }
                cur = LoggedQso{};
                has_fields = false;
                continue;
            }

            std::string val;
            if (val_len > 0 && pos + val_len <= content.size()) {
                val = content.substr(pos, val_len);
                pos += val_len;
            }
            if (tag_name == "CALL") { cur.call = trim_upper(val); has_fields = true; }
            else if (tag_name == "GRIDSQUARE") { cur.grid = trim_upper(val); has_fields = true; }
            else if (tag_name == "MODE" || tag_name == "SUBMODE") { if (cur.mode.empty() || tag_name == "SUBMODE") cur.mode = trim_upper(val); }
            else if (tag_name == "BAND") { cur.band = trim_upper(val); }
            else if (tag_name == "QSO_DATE" || tag_name == "QSO_DATE_OFF") { if (cur.date_ymd.empty()) cur.date_ymd = val; }
            else if (tag_name == "TIME_ON" || tag_name == "TIME_OFF") { if (cur.time_utc.empty()) cur.time_utc = val; }
            else if (tag_name == "RST_SENT") { cur.rst_sent = val; }
            else if (tag_name == "RST_RCVD") { cur.rst_rcvd = val; }
            else if (tag_name == "FREQ") {
                double mhz = std::atof(val.c_str());
                if (mhz > 0.1) cur.freq_hz = static_cast<uint64_t>(std::llround(mhz * 1e6));
            }
            else if (tag_name == "QSL_RCVD" || tag_name == "LOTW_QSL_RCVD" || tag_name == "EQSL_QSL_RCVD") {
                if (!val.empty() && (val[0] == 'Y' || val[0] == 'y')) cur.confirmed = true;
            }
        }

        if (added > 0 || !worked_calls.empty()) {
            std::filesystem::path p(src_label);
            adif_status_text = p.filename().string() + " (" + std::to_string(qso_log.size()) + " QSOs, " +
                               std::to_string(worked_calls.size()) + " calls, " +
                               std::to_string(worked_grids.size()) + " grids)";
        }
        return added;
    }

    // Automatically discover and import WSJT-X, JS8Call, GridTracker, and local .adi log files
    size_t auto_import_wsjtx_adif() {
        std::vector<std::string> candidate_paths;
#ifdef _WIN32
        if (const char* local_app = std::getenv("LOCALAPPDATA"); local_app && *local_app) {
            std::string la(local_app);
            candidate_paths.push_back(la + "\\WSJT-X\\wsjtx_log.adi");
            candidate_paths.push_back(la + "\\JS8Call\\js8call_log.adi");
        }
        if (const char* appdata = std::getenv("APPDATA"); appdata && *appdata) {
            std::string ad(appdata);
            candidate_paths.push_back(ad + "\\GridTracker\\GridTracker_QSO.adif");
            candidate_paths.push_back(ad + "\\HamShack++\\hamshack_wsjtx_log.adi");
        }
        if (const char* userprof = std::getenv("USERPROFILE"); userprof && *userprof) {
            std::string up(userprof);
            candidate_paths.push_back(up + "\\Documents\\GridTracker\\GridTracker_QSO.adif");
            candidate_paths.push_back(up + "\\Documents\\wsjtx_log.adi");
            candidate_paths.push_back(up + "\\Desktop\\wsjtx_log.adi");
            candidate_paths.push_back(up + "\\Downloads\\wsjtx_log.adi");
        }
#else
        const char* home = std::getenv("HOME");
        if (home && *home) {
            std::string h(home);
            candidate_paths.push_back(h + "/Library/Application Support/WSJT-X/wsjtx_log.adi");
            candidate_paths.push_back(h + "/.local/share/WSJT-X/wsjtx_log.adi");
            candidate_paths.push_back(h + "/Library/Application Support/JS8Call/js8call_log.adi");
            candidate_paths.push_back(h + "/Documents/GridTracker/GridTracker_QSO.adif");
            candidate_paths.push_back(h + "/Library/Application Support/GridTracker/GridTracker_QSO.adif");
            candidate_paths.push_back(h + "/Documents/wsjtx_log.adi");
            candidate_paths.push_back(h + "/Desktop/wsjtx_log.adi");
            candidate_paths.push_back(h + "/Downloads/wsjtx_log.adi");
        }
#endif
        candidate_paths.push_back("wsjtx_log.adi");
        candidate_paths.push_back("hamshack_wsjtx_log.adi");

        size_t total_imported = 0;
        std::string last_found;
        for (const auto& path : candidate_paths) {
            std::error_code ec;
            if (std::filesystem::exists(path, ec) && std::filesystem::file_size(path, ec) > 0) {
                total_imported += import_adif_file(path);
                last_found = path;
            }
        }
        std::lock_guard<std::recursive_mutex> lk(mtx);
        if (last_found.empty() && qso_log.empty()) {
            adif_status_text = "WSJT-X .adi: Ready (0 prior QSOs — click 'Import .ADI' or Log QSO)";
        }
        return total_imported;
    }

    void log_current_qso(uint64_t rig_freq_hz) {
        std::lock_guard<std::recursive_mutex> lk(mtx);
        std::time_t t = std::time(nullptr);
        std::tm gm{};
        hs_gmtime(&t, &gm);
        char d_buf[16], t_buf[16], rs_buf[16], rr_buf[16];
        std::snprintf(d_buf, sizeof(d_buf), "%04d%02d%02d", gm.tm_year + 1900, gm.tm_mon + 1, gm.tm_mday);
        std::snprintf(t_buf, sizeof(t_buf), "%02d%02d%02d", gm.tm_hour, gm.tm_min, gm.tm_sec);
        std::snprintf(rs_buf, sizeof(rs_buf), "%+03d", rpt_sent);
        std::snprintf(rr_buf, sizeof(rr_buf), "%+03d", rpt_rcvd);

        LoggedQso q{d_buf, t_buf, dx_call, dx_grid, is_ft4 ? "FT4" : "FT8", rig_freq_hz, rs_buf, rr_buf, "", false};
        qso_log.push_back(q);
        record_worked_station_locked(q.call, q.grid, false);
        adif_status_text = "hamshack_wsjtx_log.adi (" + std::to_string(qso_log.size()) + " QSOs, " +
                           std::to_string(worked_calls.size()) + " calls, " +
                           std::to_string(worked_grids.size()) + " grids)";

        // Append standard ADIF record to hamshack_wsjtx_log.adi (and WSJT-X log if present)
        std::ofstream f("hamshack_wsjtx_log.adi", std::ios::app);
        if (f) {
            f << "<call:" << q.call.size() << ">" << q.call
              << " <gridsquare:" << q.grid.size() << ">" << q.grid
              << " <mode:" << q.mode.size() << ">" << q.mode
              << " <rst_sent:" << q.rst_sent.size() << ">" << q.rst_sent
              << " <rst_rcvd:" << q.rst_rcvd.size() << ">" << q.rst_rcvd
              << " <qso_date:8>" << q.date_ymd
              << " <time_on:6>" << q.time_utc
              << " <my_gridsquare:" << my_grid.size() << ">" << my_grid
              << " <station_callsign:" << my_call.size() << ">" << my_call
              << " <eor>\n";
        }
    }

    bool export_qso_csv_to_file(const std::string& filepath) const {
        std::lock_guard<std::recursive_mutex> lk(mtx);
        std::ofstream f(filepath);
        if (!f) return false;
        f << "DATE,TIME_UTC,CALLSIGN,GRID,MODE,FREQ_HZ,RST_SENT,RST_RCVD\n";
        for (const auto& q : qso_log) {
            f << q.date_ymd << "," << q.time_utc << "," << q.call << ","
              << q.grid << "," << q.mode << "," << q.freq_hz << ","
              << q.rst_sent << "," << q.rst_rcvd << "\n";
        }
        return true;
    }

    std::string export_qso_csv(const std::string& path = "") const {
        std::string target = path.empty() ? (getenv("HOME") ? std::string(getenv("HOME")) + "/hamshack_contacts.csv" : "hamshack_contacts.csv") : path;
        export_qso_csv_to_file(target);
        return target;
    }

    bool export_contacts_kml_to_file(const std::string& filepath) const {
        std::lock_guard<std::recursive_mutex> lk(mtx);
        std::ofstream f(filepath);
        if (!f) return false;
        f << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";
        f << "<kml xmlns=\"http://www.opengis.net/kml/2.2\">\n";
        f << "<Document>\n";
        f << "  <name>HamShack++ Worked Maidenhead Grids &amp; Contacts</name>\n";
        f << "  <description>Exported from HamShack++</description>\n";

        double qth_lat = 0, qth_lon = 0;
        grid_to_latlon(my_grid, qth_lat, qth_lon);

        f << "  <Placemark>\n";
        f << "    <name>QTH: " << my_call << " (" << my_grid << ")</name>\n";
        f << "    <Point><coordinates>" << qth_lon << "," << qth_lat << ",0</coordinates></Point>\n";
        f << "  </Placemark>\n";

        for (const auto& g : worked_grids) {
            if (g.size() < 4) continue;
            char c0 = std::toupper(static_cast<unsigned char>(g[0]));
            char c1 = std::toupper(static_cast<unsigned char>(g[1]));
            char c2 = g[2], c3 = g[3];
            double lon0 = (c0 - 'A') * 20.0 - 180.0 + (c2 - '0') * 2.0;
            double lat0 = (c1 - 'A') * 10.0 - 90.0  + (c3 - '0') * 1.0;
            double lon1 = lon0 + 2.0;
            double lat1 = lat0 + 1.0;

            f << "  <Placemark>\n";
            f << "    <name>Grid " << g << "</name>\n";
            f << "    <Polygon><outerBoundaryIs><LinearRing><coordinates>\n";
            f << "      " << lon0 << "," << lat0 << ",0\n";
            f << "      " << lon1 << "," << lat0 << ",0\n";
            f << "      " << lon1 << "," << lat1 << ",0\n";
            f << "      " << lon0 << "," << lat1 << ",0\n";
            f << "      " << lon0 << "," << lat0 << ",0\n";
            f << "    </coordinates></LinearRing></outerBoundaryIs></Polygon>\n";
            f << "  </Placemark>\n";
        }

        for (const auto& q : qso_log) {
            double d_lat = 0, d_lon = 0;
            if (grid_to_latlon(q.grid, d_lat, d_lon)) {
                f << "  <Placemark>\n";
                f << "    <name>" << q.call << " (" << q.grid << ")</name>\n";
                f << "    <LineString><tessellate>1</tessellate><coordinates>\n";
                f << "      " << qth_lon << "," << qth_lat << ",0\n";
                f << "      " << d_lon << "," << d_lat << ",0\n";
                f << "    </coordinates></LineString>\n";
                f << "  </Placemark>\n";
            }
        }

        f << "</Document>\n";
        f << "</kml>\n";
        return true;
    }

    std::string export_contacts_kml(const std::string& path = "") const {
        std::string target = path.empty() ? (getenv("HOME") ? std::string(getenv("HOME")) + "/hamshack_contacts.kml" : "hamshack_contacts.kml") : path;
        export_contacts_kml_to_file(target);
        return target;
    }

    std::string export_adif_log(const std::string& path = "") const {
        std::string target = path.empty() ? (getenv("HOME") ? std::string(getenv("HOME")) + "/hamshack_export.adi" : "hamshack_export.adi") : path;
        std::lock_guard<std::recursive_mutex> lk(mtx);
        std::ofstream f(target);
        if (f) {
            f << "HamShack++ ADIF Export\n<adif_ver:5>3.1.4\n<programid:11>HamShack++\n<eoh>\n";
            for (const auto& q : qso_log) {
                f << "<call:" << q.call.size() << ">" << q.call
                  << " <qso_date:" << q.date_ymd.size() << ">" << q.date_ymd
                  << " <time_on:" << q.time_utc.size() << ">" << q.time_utc
                  << " <mode:" << q.mode.size() << ">" << q.mode
                  << " <rst_sent:" << q.rst_sent.size() << ">" << q.rst_sent
                  << " <rst_rcvd:" << q.rst_rcvd.size() << ">" << q.rst_rcvd
                  << " <gridsquare:" << q.grid.size() << ">" << q.grid
                  << " <freq:" << std::to_string(q.freq_hz / 1e6).size() << ">" << (q.freq_hz / 1e6)
                  << " <station_callsign:" << my_call.size() << ">" << my_call
                  << " <eor>\n";
            }
        }
        return target;
    }

    // Maidenhead 4-char grid square great-circle distance (km) & azimuth (deg) ported from Geodesic.cpp
    static bool grid_to_latlon(const std::string& g, double& lat, double& lon) {
        if (g.size() < 4) return false;
        char c0 = static_cast<char>(std::toupper(g[0]));
        char c1 = static_cast<char>(std::toupper(g[1]));
        char c2 = g[2], c3 = g[3];
        if (c0 < 'A' || c0 > 'R' || c1 < 'A' || c1 > 'R' || c2 < '0' || c2 > '9' || c3 < '0' || c3 > '9') return false;
        lon = -180.0 + (c0 - 'A') * 20.0 + (c2 - '0') * 2.0 + 1.0;
        lat = -90.0 + (c1 - 'A') * 10.0 + (c3 - '0') * 1.0 + 0.5;
        return true;
    }

    static bool compute_az_dist(const std::string& my_g, const std::string& dx_g, int& az_deg, int& dist_km) {
        double lat1 = 0, lon1 = 0, lat2 = 0, lon2 = 0;
        if (!grid_to_latlon(my_g, lat1, lon1) || !grid_to_latlon(dx_g, lat2, lon2)) return false;
        constexpr double deg2rad = 3.14159265358979323846 / 180.0;
        double p1 = lat1 * deg2rad, p2 = lat2 * deg2rad;
        double dl = (lon2 - lon1) * deg2rad;
        double dp = (lat2 - lat1) * deg2rad;
        double a = std::sin(dp * 0.5) * std::sin(dp * 0.5) +
                   std::cos(p1) * std::cos(p2) * std::sin(dl * 0.5) * std::sin(dl * 0.5);
        double c = 2.0 * std::atan2(std::sqrt(a), std::sqrt(std::max(0.0, 1.0 - a)));
        dist_km = static_cast<int>(std::round(6371.0 * c));
        double y = std::sin(dl) * std::cos(p2);
        double x = std::cos(p1) * std::sin(p2) - std::sin(p1) * std::cos(p2) * std::cos(dl);
        double brng = std::atan2(y, x) / deg2rad;
        az_deg = (static_cast<int>(std::round(brng)) + 360) % 360;
        return true;
    }

    // Periodic 100ms tick called by main app loop: manages UTC slot boundaries, GFSK TX, and LDPC RX decoding
    void tick(SoundcardEngine& audio, bool& out_want_ptt) {
        using namespace std::chrono;
        auto now = system_clock::now();
        double epoch_s = duration_cast<milliseconds>(now.time_since_epoch()).count() / 1000.0;
        std::time_t tt = system_clock::to_time_t(now);
        std::tm gm{};
        hs_gmtime(&tt, &gm);

        std::lock_guard<std::recursive_mutex> lk(mtx);
        double period = is_ft4 ? 7.5 : 15.0;
        double in_slot = std::fmod(epoch_s, period);
        cycle_sec = static_cast<float>(in_slot);
        cycle_progress = static_cast<float>(in_slot / period);
        int slot_id = static_cast<int>( std::floor(epoch_s / period) );
        bool is_even_slot = (slot_id % 2) == 0;

        char utc_buf[16];
        std::snprintf(utc_buf, sizeof(utc_buf), "%02d%02d%02d", gm.tm_hour, gm.tm_min, (gm.tm_sec / 15) * 15);

        if (tune_active) {
            out_want_ptt = true;
            transmitting_now = true;
            if (!audio.is_tx_active()) {
                std::vector<float> tune_wave(2400, 0.0f);
                float dphi = 2.0f * 3.14159265f * tx_freq_hz / 12000.0f;
                for (size_t i = 0; i < tune_wave.size(); ++i) {
                    tune_wave[i] = 0.75f * std::sin(dphi * i);
                }
                audio.queue_tx_samples(tune_wave);
            }
            return;
        }

        // Check if we should transmit in our designated even/odd slot
        double tx_dur = is_ft4 ? 5.04 : 12.64;
        double guard_end = period - 0.25;
        if (tx_enabled && (is_even_slot == tx_even_first) && in_slot < guard_end) {
            if (last_tx_slot_id != slot_id && in_slot < tx_dur) {
                last_tx_slot_id = slot_id;
                std::string msg_to_send = tx_msgs[std::clamp(active_tx_slot, 0, 5)];
                std::vector<float> wave;
                if (is_ft4) {
                    auto tones = gen_ft4_tones(msg_to_send);
                    wave = synthesize_gfsk_wave(tones.data(), 103, 576, 1.0f, static_cast<float>(tx_freq_hz));
                } else {
                    auto tones = gen_ft8_tones(msg_to_send);
                    wave = synthesize_gfsk_wave(tones.data(), 79, 1920, 2.0f, static_cast<float>(tx_freq_hz));
                }
                audio.queue_tx_samples(wave);
                audio.add_synth_waterfall_signal(static_cast<float>(tx_freq_hz), is_ft4 ? 85.0f : 50.0f, +6.0f, is_ft4 ? 1 : 0,
                                                 static_cast<uint64_t>(tx_dur * 1000));

                DecodedFtMessage tx_rec{utc_buf, 0, 0.0f, tx_freq_hz, is_ft4, msg_to_send,
                                        msg_to_send.rfind("CQ ", 0) == 0, false, true, is_even_slot, slot_id};
                rx_freq_activity.push_back(tx_rec);
                if (rx_freq_activity.size() > 150) rx_freq_activity.pop_front();
                last_sent_msg_ = msg_to_send;

                // Automatic sequence advancement just like WSJT-X
                if (auto_seq) {
                    if (active_tx_slot == 3) {
                        // After sending RR73, prompt to log QSO and advance to Tx5 (73)
                        qso_prompt_log_pending.store(true);
                        active_tx_slot = 4;
                    } else if (active_tx_slot == 4) {
                        // After sending 73, sequence is complete: prompt to log QSO, disarm Tx and reset to CQ
                        qso_prompt_log_pending.store(true);
                        active_tx_slot = 5;
                        tx_enabled = false;
                    }
                }
            }
            transmitting_now = audio.is_tx_active();
            out_want_ptt = transmitting_now;
            return;
        } else {
            if (transmitting_now || audio.is_tx_active()) {
                audio.halt_tx();
            }
            transmitting_now = false;
            out_want_ptt = false;
        }

        // Decode at the end of a receive slot on a dedicated background worker thread (0.0 ms UI stall):
        // In WSJT-X, a DT=0.0s FT8 frame runs from t=0.50s to t=13.14s of the 15s UTC slot.
        // Pass 1 at 13.40s (FT8) / 5.60s (FT4) right after normal-DT transmissions complete;
        // Pass 2 at 14.65s (FT8) / 7.05s (FT4) to catch late-DT stations.
        double pass1_sec = is_ft4 ? 5.60 : 13.40;
        double pass2_sec = is_ft4 ? 7.05 : 14.65;
        if (monitor_enabled) {
            if (last_decoded_slot_id != slot_id && in_slot >= pass1_sec) {
                last_decoded_slot_id = slot_id;
                enqueue_async_decode(audio.get_recent_rx_samples(12000 * 15), utc_buf, slot_id);
            } else if (last_late_decoded_slot_id != slot_id && in_slot >= pass2_sec) {
                last_late_decoded_slot_id = slot_id;
                enqueue_async_decode(audio.get_recent_rx_samples(12000 * 15), utc_buf, slot_id);
            }
        }
    }

    // Synchronous manual decode (used by deterministic unit tests)
    void trigger_manual_decode(SoundcardEngine& audio) {
        std::time_t tt = std::time(nullptr);
        std::tm gm{};
        hs_gmtime(&tt, &gm);
        char utc_buf[16];
        std::snprintf(utc_buf, sizeof(utc_buf), "%02d%02d%02d", gm.tm_hour, gm.tm_min, (gm.tm_sec / 15) * 15);
        int slot_id = manual_slot_counter_.fetch_add(1, std::memory_order_relaxed) + 1;
        decode_pcm_snapshot(audio.get_recent_rx_samples(12000 * 15), utc_buf, slot_id);
    }

    // Non-blocking manual decode (used by GUI "Decode Now" button so UI never stalls)
    void trigger_manual_decode_async(SoundcardEngine& audio) {
        std::time_t tt = std::time(nullptr);
        std::tm gm{};
        hs_gmtime(&tt, &gm);
        char utc_buf[16];
        std::snprintf(utc_buf, sizeof(utc_buf), "%02d%02d%02d", gm.tm_hour, gm.tm_min, (gm.tm_sec / 15) * 15);
        int slot_id = manual_slot_counter_.fetch_add(1, std::memory_order_relaxed) + 1;
        enqueue_async_decode(audio.get_recent_rx_samples(12000 * 15), utc_buf, slot_id);
    }

private:
    uint8_t gen_mat_[83][91]{};
    int nrw_[83]{};
    int mn_[174][3]{};
    std::string last_sent_msg_;

    std::atomic<int> manual_slot_counter_{100};
    std::mutex decode_job_mtx_;
    std::condition_variable decode_job_cv_;
    std::thread decode_worker_;
    bool decode_worker_started_ = false;
    bool decode_worker_stop_ = false;
    bool has_pending_decode_ = false;
    std::vector<float> pending_pcm_;
    std::string pending_utc_;
    int pending_slot_id_ = 0;

    void enqueue_async_decode(std::vector<float>&& pcm, const char* utc_buf, int slot_id) {
        {
            std::lock_guard<std::mutex> lk(decode_job_mtx_);
            if (decode_worker_stop_) return;
            if (!decode_worker_started_) {
                decode_worker_started_ = true;
                decode_worker_ = std::thread([this]() { decode_worker_loop(); });
            }
            pending_pcm_ = std::move(pcm);
            pending_utc_ = utc_buf ? utc_buf : "";
            pending_slot_id_ = slot_id;
            has_pending_decode_ = true;
        }
        decode_job_cv_.notify_one();
    }

    void decode_worker_loop() {
        for (;;) {
            std::vector<float> job_pcm;
            std::string job_utc;
            int job_slot = 0;
            {
                std::unique_lock<std::mutex> lk(decode_job_mtx_);
                decode_job_cv_.wait(lk, [this]() { return decode_worker_stop_ || has_pending_decode_; });
                if (decode_worker_stop_) break;
                job_pcm = std::move(pending_pcm_);
                job_utc = std::move(pending_utc_);
                job_slot = pending_slot_id_;
                has_pending_decode_ = false;
            }
            decode_in_progress.store(true, std::memory_order_release);
            decode_pcm_snapshot(job_pcm, job_utc.c_str(), job_slot);
            decode_in_progress.store(false, std::memory_order_release);
        }
    }

    void init_ldpc_tables() {
        auto hex_val = [](char c) -> int {
            if (c >= '0' && c <= '9') return c - '0';
            if (c >= 'a' && c <= 'f') return c - 'a' + 10;
            if (c >= 'A' && c <= 'F') return c - 'A' + 10;
            return 0;
        };
        for (int r = 0; r < 83; ++r) {
            const char* row_hex = kFt8LdpcGenHex[r];
            for (int j = 0; j < 23; ++j) {
                int v = hex_val(row_hex[j]);
                int ibmax = (j == 22) ? 3 : 4;
                for (int jj = 0; jj < ibmax; ++jj) {
                    int col = j * 4 + jj;
                    if (col < 91 && ((v >> (3 - jj)) & 1)) {
                        gen_mat_[r][col] = 1;
                    }
                }
            }
        }
        for (int n = 0; n < 174; ++n) {
            mn_[n][0] = mn_[n][1] = mn_[n][2] = -1;
        }
        for (int j = 0; j < 83; ++j) {
            int cnt = 0;
            for (int i = 0; i < 7; ++i) {
                if (kFt8Nm[j][i] > 0) {
                    ++cnt;
                    int vnode = kFt8Nm[j][i] - 1;
                    for (int k = 0; k < 3; ++k) {
                        if (mn_[vnode][k] < 0) { mn_[vnode][k] = j; break; }
                    }
                }
            }
            nrw_[j] = cnt;
        }
    }

    static std::string trim_upper(const std::string& in) {
        std::string out;
        bool prev_space = true;
        for (unsigned char c : in) {
            char u = static_cast<char>(std::toupper(c));
            if (u == ' ') {
                if (!prev_space) { out.push_back(' '); prev_space = true; }
            } else if (u >= 0x21 && u < 0x7F) {
                out.push_back(u);
                prev_space = false;
            }
        }
        while (!out.empty() && out.back() == ' ') out.pop_back();
        return out;
    }

public:
    static void extract_spot_info(const std::string& txt, std::string& out_call, std::string& out_grid) {
        ParsedFtMessage p = parse_ft_message(txt);
        out_call = p.call_from;
        out_grid = is_grid4_str(p.extra) ? p.extra : "";
    }

    // Standard WSJT-X 28-bit callsign packing (`pack28` / `unpack28` from `packjt77.f90`)
    static bool pack28_std(const std::string& call_in, uint32_t& n28) {
        static constexpr uint32_t NTOKENS = 2063592u;
        static constexpr uint32_t MAX22   = 4194304u;
        static const char* A1 = " 0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";
        static const char* A2 = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";
        static const char* A3 = "0123456789";
        static const char* A4 = " ABCDEFGHIJKLMNOPQRSTUVWXYZ";

        if (call_in == "DE")  { n28 = 0; return true; }
        if (call_in == "QRZ") { n28 = 1; return true; }
        if (call_in == "CQ")  { n28 = 2; return true; }

        if (call_in.rfind("CQ ", 0) == 0) {
            std::string sub = call_in.substr(3);
            if (sub.size() == 3 && std::isdigit(sub[0]) && std::isdigit(sub[1]) && std::isdigit(sub[2])) {
                n28 = 3u + static_cast<uint32_t>(std::atoi(sub.c_str()));
                return true;
            }
            if (sub.size() >= 1 && sub.size() <= 4) {
                while (sub.size() < 4) sub.push_back(' ');
                auto get_i = [&](char ch) -> int {
                    const char* p = std::strchr(A4, ch);
                    return p ? static_cast<int>(p - A4) : -1;
                };
                int i1 = get_i(sub[0]);
                int i2 = get_i(sub[1]);
                int i3 = get_i(sub[2]);
                int i4 = get_i(sub[3]);
                if (i1 >= 0 && i2 >= 0 && i3 >= 0 && i4 >= 0) {
                    uint32_t n = ((static_cast<uint32_t>(i1) * 27u + static_cast<uint32_t>(i2)) * 27u + static_cast<uint32_t>(i3)) * 27u + static_cast<uint32_t>(i4);
                    n28 = 1003u + n;
                    return true;
                }
            }
        }

        int n = static_cast<int>(call_in.size());
        if (n < 3 || n > 6) return false;
        int iarea = -1;
        for (int i = n - 1; i >= 1; --i) {
            if (call_in[i] >= '0' && call_in[i] <= '9') { iarea = i; break; }
        }
        if (iarea < 1 || iarea > 2) return false;
        std::string c6 = (iarea == 1) ? (" " + call_in) : call_in;
        c6.resize(6, ' ');

        const char* p1 = std::strchr(A1, c6[0]);
        const char* p2 = std::strchr(A2, c6[1]);
        const char* p3 = std::strchr(A3, c6[2]);
        const char* p4 = std::strchr(A4, c6[3]);
        const char* p5 = std::strchr(A4, c6[4]);
        const char* p6 = std::strchr(A4, c6[5]);
        if (!p1 || !p2 || !p3 || !p4 || !p5 || !p6) return false;

        uint32_t i1 = static_cast<uint32_t>(p1 - A1);
        uint32_t i2 = static_cast<uint32_t>(p2 - A2);
        uint32_t i3 = static_cast<uint32_t>(p3 - A3);
        uint32_t i4 = static_cast<uint32_t>(p4 - A4);
        uint32_t i5 = static_cast<uint32_t>(p5 - A4);
        uint32_t i6 = static_cast<uint32_t>(p6 - A4);

        n28 = NTOKENS + MAX22 +
              36u * 10u * 27u * 27u * 27u * i1 +
              10u * 27u * 27u * 27u * i2 +
              27u * 27u * 27u * i3 +
              27u * 27u * i4 +
              27u * i5 + i6;
        return true;
    }

    static uint32_t ihashcall_wsjtx(const std::string& call_in, int m_bits) {
        static const char* C38 = " 0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ/";
        std::string c11 = trim_upper(call_in);
        if (!c11.empty() && c11.front() == '<') c11.erase(c11.begin());
        if (!c11.empty() && c11.back() == '>') c11.pop_back();
        c11.resize(11, ' ');
        uint64_t n8 = 0;
        for (int i = 0; i < 11; ++i) {
            const char* p = std::strchr(C38, c11[i]);
            uint64_t j = p ? static_cast<uint64_t>(p - C38) : 0ULL;
            n8 = 38ULL * n8 + j;
        }
        uint64_t prod = 47055833459ULL * n8;
        return static_cast<uint32_t>(prod >> (64 - m_bits));
    }

    void save_hash_call_wsjtx(const std::string& call_in) const {
        std::string c = trim_upper(call_in);
        if (c.size() < 3 || c.find("...") != std::string::npos || c.rfind("CQ", 0) == 0) return;
        if (c.front() == '<') c.erase(c.begin());
        if (!c.empty() && c.back() == '>') c.pop_back();
        hash10_calls[ihashcall_wsjtx(c, 10)] = c;
        hash12_calls[ihashcall_wsjtx(c, 12)] = c;
        hash22_calls[ihashcall_wsjtx(c, 22)] = c;
    }

    bool unpack28_std(uint32_t n28, std::string& call_out) const {
        static constexpr uint32_t NTOKENS = 2063592u;
        static constexpr uint32_t MAX22   = 4194304u;
        static const char* A1 = " 0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";
        static const char* A2 = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";
        static const char* A3 = "0123456789";
        static const char* A4 = " ABCDEFGHIJKLMNOPQRSTUVWXYZ";

        if (n28 < NTOKENS) {
            if (n28 == 0) { call_out = "DE"; return true; }
            if (n28 == 1) { call_out = "QRZ"; return true; }
            if (n28 == 2) { call_out = "CQ"; return true; }
            if (n28 <= 1002) {
                char buf[16];
                std::snprintf(buf, sizeof(buf), "CQ %03u", n28 - 3u);
                call_out = buf;
                return true;
            }
            if (n28 <= 532443u) {
                uint32_t n = n28 - 1003u;
                uint32_t i1 = n / (27u * 27u * 27u); n %= (27u * 27u * 27u);
                uint32_t i2 = n / (27u * 27u);       n %= (27u * 27u);
                uint32_t i3 = n / 27u;
                uint32_t i4 = n % 27u;
                if (i1 > 26 || i2 > 26 || i3 > 26 || i4 > 26) return false;
                std::string sub;
                if (A4[i1] != ' ') sub.push_back(A4[i1]);
                if (A4[i2] != ' ') sub.push_back(A4[i2]);
                if (A4[i3] != ' ') sub.push_back(A4[i3]);
                if (A4[i4] != ' ') sub.push_back(A4[i4]);
                call_out = sub.empty() ? "CQ" : ("CQ " + sub);
                return true;
            }
            return false;
        }
        if (n28 < NTOKENS + MAX22) {
            uint32_t h22 = n28 - NTOKENS;
            auto it = hash22_calls.find(h22);
            if (it != hash22_calls.end()) {
                call_out = "<" + it->second + ">";
            } else if (!my_call.empty() && ihashcall_wsjtx(my_call, 22) == h22) {
                call_out = "<" + my_call + ">";
            } else {
                call_out = "<...>";
            }
            return true;
        }

        uint32_t n = n28 - NTOKENS - MAX22;
        uint32_t i1 = n / (36u * 10u * 27u * 27u * 27u); n %= (36u * 10u * 27u * 27u * 27u);
        uint32_t i2 = n / (10u * 27u * 27u * 27u);       n %= (10u * 27u * 27u * 27u);
        uint32_t i3 = n / (27u * 27u * 27u);             n %= (27u * 27u * 27u);
        uint32_t i4 = n / (27u * 27u);                   n %= (27u * 27u);
        uint32_t i5 = n / 27u;
        uint32_t i6 = n % 27u;
        if (i1 >= 37 || i2 >= 36 || i3 >= 10 || i4 >= 27 || i5 >= 27 || i6 >= 27) return false;

        std::string s;
        if (A1[i1] != ' ') s.push_back(A1[i1]);
        s.push_back(A2[i2]);
        s.push_back(A3[i3]);
        if (A4[i4] != ' ') s.push_back(A4[i4]);
        if (A4[i5] != ' ') s.push_back(A4[i5]);
        if (A4[i6] != ' ') s.push_back(A4[i6]);
        if (s.size() < 3) return false;
        call_out = s;
        save_hash_call_wsjtx(s);
        return true;
    }

    // Standard WSJT-X Type 1 message (`28 + 1 + 28 + 1 + 1 + 15 + 3 = 77 bits`)
    static bool pack_type1_std(const std::string& msg, std::array<uint8_t, 77>& bits) {
        auto w = split_words(msg);
        if (w.size() < 2 || w.size() > 4) return false;
        uint32_t n28a = 0, n28b = 0;
        int grid_idx = 2;

        if (w[0] == "CQ" && w.size() >= 3 && !is_valid_callsign(clean_callsign(w[1])) && is_valid_callsign(clean_callsign(w[2]))) {
            // Directed CQ: e.g. "CQ DX N0CALL EM00" or "CQ POTA N0CALL EM00"
            std::string cq_dir = w[0] + " " + w[1];
            if (!pack28_std(cq_dir, n28a) || !pack28_std(clean_callsign(w[2]), n28b)) return false;
            grid_idx = 3;
        } else {
            if (!pack28_std(clean_callsign(w[0]), n28a) || !pack28_std(clean_callsign(w[1]), n28b)) return false;
        }

        uint32_t ir = 0;
        uint32_t igrid4 = 32401u;
        if (w.size() > static_cast<size_t>(grid_idx)) {
            std::string ex = w[grid_idx];
            if (grid_idx + 1 < static_cast<int>(w.size()) && ex == "R") {
                ir = 1;
                ex = w[grid_idx + 1];
            }
            if (is_grid4_str(ex)) {
                igrid4 = static_cast<uint32_t>((ex[0] - 'A') * 1800 + (ex[1] - 'A') * 100 + (ex[2] - '0') * 10 + (ex[3] - '0'));
            } else if (ex == "RRR") {
                igrid4 = 32402u;
            } else if (ex == "RR73") {
                igrid4 = 32403u;
            } else if (ex == "73") {
                igrid4 = 32404u;
            } else if (!ex.empty() && (ex[0] == '+' || ex[0] == '-' || (ex.size() >= 2 && ex[0] == 'R' && (ex[1] == '+' || ex[1] == '-')))) {
                if (ex[0] == 'R') { ir = 1; ex = ex.substr(1); }
                int rpt = std::clamp(std::atoi(ex.c_str()), -30, 50);
                igrid4 = static_cast<uint32_t>(32400 + 35 + rpt);
            } else {
                return false;
            }
        }

        bits.fill(0);
        auto write_bits = [&](int offset, int len, uint32_t val) {
            for (int i = 0; i < len; ++i) {
                bits[offset + i] = (val >> (len - 1 - i)) & 1u;
            }
        };
        write_bits(0, 28, n28a);
        write_bits(28, 1, 0);
        write_bits(29, 28, n28b);
        write_bits(57, 1, 0);
        write_bits(58, 1, ir);
        write_bits(59, 15, igrid4);
        write_bits(74, 3, 1); // i3 = 1 (Standard Type 1)
        return true;
    }

    bool unpack_type1_std(const std::array<uint8_t, 77>& bits, std::string& out_msg, int i3 = 1) const {
        auto read_bits = [&](int offset, int len) -> uint32_t {
            uint32_t v = 0;
            for (int i = 0; i < len; ++i) v = (v << 1) | (bits[offset + i] & 1u);
            return v;
        };
        uint32_t n28a = read_bits(0, 28);
        uint32_t ipa  = read_bits(28, 1);
        uint32_t n28b = read_bits(29, 28);
        uint32_t ipb  = read_bits(57, 1);
        uint32_t ir   = read_bits(58, 1);
        uint32_t igrid4 = read_bits(59, 15);
        std::string c1, c2;
        if (!unpack28_std(n28a, c1) || !unpack28_std(n28b, c2)) return false;
        if (ipa && c1.find('<') == std::string::npos && c1.rfind("CQ", 0) != 0) c1 += (i3 == 2 ? "/P" : "/R");
        if (ipb && c2.find('<') == std::string::npos) c2 += (i3 == 2 ? "/P" : "/R");

        if (igrid4 <= 32400u) {
            uint32_t n = igrid4;
            char g[5]{};
            g[0] = static_cast<char>('A' + (n / 1800u)); n %= 1800u;
            g[1] = static_cast<char>('A' + (n / 100u));  n %= 100u;
            g[2] = static_cast<char>('0' + (n / 10u));
            g[3] = static_cast<char>('0' + (n % 10u));
            out_msg = c1 + " " + c2 + (ir ? " R " : " ") + g;
            return true;
        }
        uint32_t irpt = igrid4 - 32400u;
        if (irpt == 1) { out_msg = c1 + " " + c2; return true; }
        if (irpt == 2) { out_msg = c1 + " " + c2 + " RRR"; return true; }
        if (irpt == 3) { out_msg = c1 + " " + c2 + " RR73"; return true; }
        if (irpt == 4) { out_msg = c1 + " " + c2 + " 73"; return true; }
        if (irpt >= 5) {
            int snr = static_cast<int>(irpt) - 35;
            if (snr > 50) snr -= 101;
            char buf[16];
            std::snprintf(buf, sizeof(buf), "%s%+03d", ir ? "R" : "", snr);
            out_msg = c1 + " " + c2 + " " + buf;
            return true;
        }
        return false;
    }

    static void normalize_bmet(std::array<float, 174>& bmet) {
        double sum = 0.0, sum2 = 0.0;
        for (float v : bmet) {
            sum += v;
            sum2 += static_cast<double>(v) * v;
        }
        double av = sum / 174.0;
        double var = (sum2 / 174.0) - av * av;
        float sigma = (var > 1e-12) ? static_cast<float>(std::sqrt(var)) : 1.0f;
        for (float& v : bmet) {
            v /= sigma;
        }
    }

    // Runs real WSJT-X `sync8` + `ft8b` multi-pass coherent/non-coherent 8-FSK demodulation + LDPC(174,91) decoding on live 12 kHz USB audio.
    // Executes all heavy spectrogram, SIMD Goertzel, and LDPC belief-propagation passes completely UNLOCKED, acquiring `mtx` only at commit time.
    void decode_pcm_snapshot(const std::vector<float>& pcm, const char* utc_buf, int slot_id) {
        constexpr int NSPS = 1920;
        constexpr int STEP = 480;
        constexpr int NFFT = 4096;
        if (pcm.size() < static_cast<size_t>(NSPS + 79 * 4 * STEP)) return;

        double energy = 0.0;
        for (float s : pcm) energy += static_cast<double>(s) * s;
        if (energy < 1e-9) return; // Soundcard silent / radio offline -> show nothing

        // Stage 1: Quarter-symbol spectrogram (NSPS = 1920, STEP = 480 samples = 0.04s, NFFT = 4096 -> 2.93 Hz bins)
        const int nsteps = static_cast<int>((pcm.size() - NSPS) / STEP);
        if (nsteps < 79 * 4) return;

        struct StaticFt8Tables {
            float hann[NSPS];
            int grid_b0[880];
            float grid_frac[880];
            StaticFt8Tables() {
                for (int i = 0; i < NSPS; ++i) {
                    hann[i] = 0.5f * (1.0f - std::cos(2.0f * 3.1415926535f * i / (NSPS - 1)));
                }
                // 3.125 Hz grid from 150.0 Hz .. 2896.875 Hz (865 base freqs + 7*2 tone steps = 879 points)
                for (int g = 0; g < 880; ++g) {
                    float freq_hz = 150.0f + g * 3.125f;
                    float bin_f = freq_hz * NFFT / 12000.0f;
                    int b0 = std::clamp(static_cast<int>(std::floor(bin_f)), 1, 1021);
                    grid_b0[g] = b0;
                    grid_frac[g] = bin_f - b0;
                }
            }
        };
        static const StaticFt8Tables tbl;

        // Pre-interpolated 3.125 Hz half-tone grid per time step eliminates 54M dynamic floor/lerp ops during Costas search
        std::vector<std::array<float, 880>> spec_grid(nsteps);
        alignas(32) std::array<std::complex<float>, NFFT> fft_buf;
        std::array<float, 1024> mag_row;

        for (int t = 0; t < nsteps; ++t) {
            const float* src = pcm.data() + static_cast<size_t>(t) * STEP;
            for (int i = 0; i < NSPS; ++i) {
                fft_buf[i] = std::complex<float>(src[i] * tbl.hann[i], 0.0f);
            }
            std::fill(fft_buf.begin() + NSPS, fft_buf.end(), std::complex<float>(0.0f, 0.0f));
            SoundcardEngine::fft_radix2_planned<NFFT>(fft_buf.data());
            for (int b = 0; b < 1024; ++b) {
                float re = fft_buf[b].real();
                float im = fft_buf[b].imag();
                mag_row[b] = std::sqrt(re * re + im * im);
            }
            auto& grow = spec_grid[t];
            for (int g = 0; g < 880; ++g) {
                int b0 = tbl.grid_b0[g];
                float frac = tbl.grid_frac[g];
                grow[g] = mag_row[b0] * (1.0f - frac) + mag_row[b0 + 1] * frac;
            }
        }

        struct Candidate {
            float sync_score;
            float f0_hz;
            int t0_step;
        };
        std::vector<Candidate> raw_cands;
        raw_cands.reserve(256);

        // Scan 150 Hz .. 2850 Hz in 3.125 Hz (half-tone) steps for 3x7 Costas arrays across all quarter-symbol offsets
        for (int g0 = 0; g0 <= 864; ++g0) {
            float f0 = 150.0f + g0 * 3.125f;
            float best_score = 0.0f;
            int best_t0 = 0;
            for (int t0 = 0; t0 + 78 * 4 < nsteps; ++t0) {
                float sig_sum = 0.0f;
                float bg_sum = 1e-6f;
                for (int k = 0; k < 7; ++k) {
                    const int s_pos[3] = {k, 36 + k, 72 + k};
                    const int c_off = kFt8Costas7[k] * 2;
                    for (int blk = 0; blk < 3; ++blk) {
                        const float* rptr = spec_grid[t0 + s_pos[blk] * 4].data() + g0;
                        float sum8 = rptr[0] + rptr[2] + rptr[4] + rptr[6] +
                                     rptr[8] + rptr[10] + rptr[12] + rptr[14];
                        float c_val = rptr[c_off];
                        sig_sum += c_val;
                        bg_sum += (sum8 - c_val) * (1.0f / 7.0f);
                    }
                }
                float ratio = sig_sum / bg_sum;
                if (ratio > best_score) {
                    best_score = ratio;
                    best_t0 = t0;
                }
            }
            if (best_score > 1.18f) {
                raw_cands.push_back({best_score, f0, best_t0});
            }
        }

        std::sort(raw_cands.begin(), raw_cands.end(), [](const Candidate& a, const Candidate& b) {
            return a.sync_score > b.sync_score;
        });

        // Deduplicate nearby local peaks (within 4.5 Hz and 3 quarter-symbols) so we test up to 70 distinct signals
        std::vector<Candidate> cands;
        cands.reserve(70);
        for (const auto& rc : raw_cands) {
            bool dup = false;
            for (const auto& ex : cands) {
                if (std::fabs(rc.f0_hz - ex.f0_hz) < 4.5f && std::abs(rc.t0_step - ex.t0_step) <= 3) {
                    dup = true;
                    break;
                }
            }
            if (!dup) {
                cands.push_back(rc);
                if (cands.size() >= 70) break;
            }
        }

        struct DecodedHit {
            std::array<uint8_t, 77> msg77;
            float sync_score;
            int best_s0;
            float best_f0;
        };
        std::vector<DecodedHit> decoded_hits;
        std::vector<std::array<uint8_t, 77>> seen_msg77;

        const int pcm_len = static_cast<int>(pcm.size());
        constexpr double kTwoPiOverFs = 2.0 * 3.14159265358979323846 / 12000.0;
        constexpr float kInvNsps = 1.0f / static_cast<float>(NSPS);

        for (size_t ci = 0; ci < cands.size(); ++ci) {
            const auto& c = cands[ci];
            int base_sample = c.t0_step * STEP;

            // Stage 2A: 9-lane SIMD Fine Time search (+/- 480 samples in 120-sample = 10ms steps)
            int best_s0 = base_sample;
            float best_f0 = c.f0_hz;
            int s0_lanes[9];
            bool lane_valid[9];
            bool any_time_valid = false;
            for (int d = 0; d < 9; ++d) {
                int s0 = base_sample + (d - 4) * 120;
                s0_lanes[d] = s0;
                lane_valid[d] = (s0 >= 0 && s0 + 79 * NSPS <= pcm_len);
                if (lane_valid[d]) any_time_valid = true;
            }
            if (any_time_valid) {
                float sync_time[9]{};
                for (int k = 0; k < 7; ++k) {
                    int c_tone = kFt8Costas7[k];
                    double w = kTwoPiOverFs * (best_f0 + c_tone * 6.25);
                    double cw = std::cos(w), sw = std::sin(w), coeff = 2.0 * cw;
                    const int pos[3] = {k, 36 + k, 72 + k};
                    for (int blk = 0; blk < 3; ++blk) {
                        const float* ptrs[9];
                        for (int d = 0; d < 9; ++d) {
                            ptrs[d] = lane_valid[d] ? (pcm.data() + s0_lanes[d] + pos[blk] * NSPS) : pcm.data();
                        }
                        alignas(32) double q1[9]{}, q2[9]{};
                        for (int i = 0; i < NSPS; ++i) {
                            for (int d = 0; d < 9; ++d) {
                                double q0 = static_cast<double>(ptrs[d][i]) + coeff * q1[d] - q2[d];
                                q2[d] = q1[d];
                                q1[d] = q0;
                            }
                        }
                        for (int d = 0; d < 9; ++d) {
                            if (lane_valid[d]) {
                                float re = static_cast<float>(q1[d] - q2[d] * cw) * kInvNsps;
                                float im = static_cast<float>(q2[d] * sw) * kInvNsps;
                                sync_time[d] += std::sqrt(re * re + im * im);
                            }
                        }
                    }
                }
                float best_fine_sync = -1.0f;
                for (int d = 0; d < 9; ++d) {
                    if (lane_valid[d] && sync_time[d] > best_fine_sync) {
                        best_fine_sync = sync_time[d];
                        best_s0 = s0_lanes[d];
                    }
                }
            }
            if (best_s0 < 0 || best_s0 + 79 * NSPS > pcm_len) continue;

            // Stage 2B: 11-lane SIMD Fine Frequency search (+/- 2.5 Hz in 0.5 Hz steps, single PCM pass per symbol)
            {
                float sync_freq[11]{};
                for (int k = 0; k < 7; ++k) {
                    int c_tone = kFt8Costas7[k];
                    alignas(32) double cw[12]{}, sw[12]{}, coeff[12]{};
                    for (int f = 0; f < 11; ++f) {
                        double f_tone = (c.f0_hz + (f - 5) * 0.5) + c_tone * 6.25;
                        double w = kTwoPiOverFs * f_tone;
                        cw[f] = std::cos(w);
                        sw[f] = std::sin(w);
                        coeff[f] = 2.0 * cw[f];
                    }
                    const int pos[3] = {k, 36 + k, 72 + k};
                    for (int blk = 0; blk < 3; ++blk) {
                        const float* p = pcm.data() + best_s0 + pos[blk] * NSPS;
                        alignas(32) double q1[12]{}, q2[12]{};
                        for (int i = 0; i < NSPS; ++i) {
                            double x = static_cast<double>(p[i]);
                            for (int f = 0; f < 11; ++f) {
                                double q0 = x + coeff[f] * q1[f] - q2[f];
                                q2[f] = q1[f];
                                q1[f] = q0;
                            }
                        }
                        for (int f = 0; f < 11; ++f) {
                            float re = static_cast<float>(q1[f] - q2[f] * cw[f]) * kInvNsps;
                            float im = static_cast<float>(q2[f] * sw[f]) * kInvNsps;
                            sync_freq[f] += std::sqrt(re * re + im * im);
                        }
                    }
                }
                float best_fine_sync = -1.0f;
                for (int f = 0; f < 11; ++f) {
                    if (sync_freq[f] > best_fine_sync) {
                        best_fine_sync = sync_freq[f];
                        best_f0 = c.f0_hz + (f - 5) * 0.5f;
                    }
                }
            }

            // Stage 2C: 8-lane SIMD complex 8-tone symbol matrix cs[8][79] & magnitude s8[8][79]
            // Precompute all 8 tone Goertzel coefficients once per candidate (eliminating 632 trig calls)
            alignas(32) double tone_cw[8], tone_sw[8], tone_coeff[8];
            for (int m = 0; m < 8; ++m) {
                double w = kTwoPiOverFs * (best_f0 + m * 6.25);
                tone_cw[m] = std::cos(w);
                tone_sw[m] = std::sin(w);
                tone_coeff[m] = 2.0 * tone_cw[m];
            }

            std::complex<float> cs[8][79]{};
            float s8[8][79]{};
            for (int k = 0; k < 79; ++k) {
                const float* p = pcm.data() + best_s0 + k * NSPS;
                alignas(32) double q1[8]{}, q2[8]{};
                for (int i = 0; i < NSPS; ++i) {
                    double x = static_cast<double>(p[i]);
                    for (int m = 0; m < 8; ++m) {
                        double q0 = x + tone_coeff[m] * q1[m] - q2[m];
                        q2[m] = q1[m];
                        q1[m] = q0;
                    }
                }
                float phi_rot = -static_cast<float>(std::fmod(2.0 * 3.14159265358979323846 * best_f0 * (k * 0.16), 2.0 * 3.14159265358979323846));
                std::complex<float> rot(std::cos(phi_rot), std::sin(phi_rot));
                for (int m = 0; m < 8; ++m) {
                    float re = static_cast<float>(q1[m] - q2[m] * tone_cw[m]) * kInvNsps;
                    float im = static_cast<float>(q2[m] * tone_sw[m]) * kInvNsps;
                    std::complex<float> z = std::complex<float>(re, im) * rot;
                    cs[m][k] = z;
                    s8[m][k] = std::abs(z);
                }
            }

            // Hard sync quality check (count how many of the 21 Costas symbols peak on the expected tone)
            int nsync = 0;
            for (int k = 0; k < 7; ++k) {
                int pos[3] = {k, 36 + k, 72 + k};
                for (int blk = 0; blk < 3; ++blk) {
                    int sym = pos[blk];
                    int max_m = 0;
                    for (int m = 1; m < 8; ++m) {
                        if (s8[m][sym] > s8[max_m][sym]) max_m = m;
                    }
                    if (max_m == kFt8Costas7[k]) ++nsync;
                }
            }
            if (nsync < 4) continue;

            // Compute WSJT-X soft bit metrics: bmeta (1-sym), bmetb (2-sym coherent), bmetc (3-sym coherent), bmetd (norm)
            std::array<float, 174> bmeta{}, bmetb{}, bmetc{}, bmetd{}, bmete{};
            float s2[512]{};

            for (int nsym = 1; nsym <= 3; ++nsym) {
                int nt = 1 << (3 * nsym);
                int ibmax = 3 * nsym - 1;
                for (int ihalf = 0; ihalf < 2; ++ihalf) {
                    for (int k = 0; k < 29; k += nsym) {
                        int ks = (ihalf == 0) ? (k + 7) : (k + 43);
                        for (int i = 0; i < nt; ++i) {
                            int i1 = (i >> 6) & 7;
                            int i2 = (i >> 3) & 7;
                            int i3 = i & 7;
                            if (nsym == 1) {
                                s2[i] = s8[kFt8GrayMap[i3]][ks];
                            } else if (nsym == 2) {
                                if (ks + 1 < 79) {
                                    s2[i] = std::abs(cs[kFt8GrayMap[i2]][ks] + cs[kFt8GrayMap[i3]][ks + 1]);
                                } else {
                                    s2[i] = s8[kFt8GrayMap[i2]][ks];
                                }
                            } else {
                                if (ks + 2 < 79) {
                                    s2[i] = std::abs(cs[kFt8GrayMap[i1]][ks] + cs[kFt8GrayMap[i2]][ks + 1] + cs[kFt8GrayMap[i3]][ks + 2]);
                                } else {
                                    s2[i] = s8[kFt8GrayMap[i1]][ks];
                                }
                            }
                        }
                        int i32 = k * 3 + ihalf * 87;
                        for (int ib = 0; ib <= ibmax; ++ib) {
                            if (i32 + ib >= 174) continue;
                            int bit_mask = 1 << (ibmax - ib);
                            float max1 = -1e30f, max0 = -1e30f;
                            for (int i = 0; i < nt; ++i) {
                                if (i & bit_mask) max1 = std::max(max1, s2[i]);
                                else              max0 = std::max(max0, s2[i]);
                            }
                            float bm = max1 - max0;
                            if (nsym == 1) {
                                bmeta[i32 + ib] = bm;
                                float den = std::max(max1, max0);
                                bmetd[i32 + ib] = (den > 0.0f) ? (bm / den) : 0.0f;
                            } else if (nsym == 2) {
                                bmetb[i32 + ib] = bm;
                            } else {
                                bmetc[i32 + ib] = bm;
                            }
                        }
                    }
                }
            }

            for (int i = 0; i < 174; ++i) {
                float a = bmeta[i], b = bmetb[i], cc = bmetc[i];
                if (std::fabs(b) > std::fabs(a)) a = b;
                if (std::fabs(cc) > std::fabs(a)) a = cc;
                bmete[i] = a;
            }

            normalize_bmet(bmeta);
            normalize_bmet(bmetb);
            normalize_bmet(bmetc);
            normalize_bmet(bmetd);
            normalize_bmet(bmete);

            const std::array<float, 174>* passes[5] = {&bmeta, &bmetd, &bmetb, &bmetc, &bmete};
            std::array<uint8_t, 77> msg77{};
            bool decoded_ok = false;

            for (int p = 0; p < 5 && !decoded_ok; ++p) {
                std::array<float, 174> llr{};
                for (int i = 0; i < 174; ++i) llr[i] = 2.83f * (*passes[p])[i];
                if (bpdecode174_91(llr, msg77, 40)) {
                    decoded_ok = true;
                }
            }

            // AP Pass 6: A-priori "CQ " in first 29 bits (`n28a = 2, ipa = 0`)
            if (!decoded_ok) {
                std::array<float, 174> llr{};
                for (int i = 0; i < 174; ++i) llr[i] = 2.83f * bmeta[i];
                for (int i = 0; i < 29; ++i) {
                    int cq_bit = (i == 26) ? 1 : 0; // n28a = 2 -> bit 26 is 1, bit 28 (ipa) is 0
                    llr[i] = cq_bit ? +12.0f : -12.0f;
                }
                if (bpdecode174_91(llr, msg77, 40)) {
                    decoded_ok = true;
                }
            }

            if (!decoded_ok) continue;
            if (std::find(seen_msg77.begin(), seen_msg77.end(), msg77) != seen_msg77.end()) continue;
            seen_msg77.push_back(msg77);
            decoded_hits.push_back({msg77, c.sync_score, best_s0, best_f0});
        }

        if (decoded_hits.empty()) return;

        // Stage 4: Acquire `mtx` briefly (<50 us) to unpack 77-bit payloads, commit spots, and advance QSO sequencer
        std::lock_guard<std::recursive_mutex> lk(mtx);
        std::vector<std::string> seen_msgs;
        for (const auto& d : band_activity) {
            if (d.utc_str == utc_buf) seen_msgs.push_back(d.text);
        }

        for (const auto& hit : decoded_hits) {
            std::string decoded_txt = unpack77_message(hit.msg77);
            if (decoded_txt.empty()) continue;
            if (std::find(seen_msgs.begin(), seen_msgs.end(), decoded_txt) != seen_msgs.end()) continue;
            seen_msgs.push_back(decoded_txt);

            int snr = std::clamp(static_cast<int>(std::round(20.0f * std::log10(std::max(0.08f, hit.sync_score - 0.95f)) - 14.0f)), -24, +24);
            float dt = (hit.best_s0 / 12000.0f) - 0.5f;
            int freq = static_cast<int>(std::round(hit.best_f0));

            ParsedFtMessage p = parse_ft_message(decoded_txt, my_call);
            bool is_cq = p.is_cq;
            bool for_me = p.is_for_me;

            DecodedFtMessage d{utc_buf, snr, dt, freq, is_ft4, decoded_txt, is_cq, for_me, false, (slot_id % 2) == 0, slot_id};
            band_activity.push_back(d);
            if (band_activity.size() > 200) band_activity.pop_front();

            // Push decoded spot to PSK Reporter if web reporting is enabled
            if (!p.call_from.empty() && p.call_from != my_call) {
                uint64_t dial = cached_dial_hz.load();
                if (dial == 0) dial = is_ft4 ? 14080000ULL : 14074000ULL;
                hamshack::get_psk_reporter().post_spot(p.call_from, is_grid4_str(p.extra) ? p.extra : "", dial + static_cast<uint64_t>(std::max(0, freq)), snr, is_ft4 ? "FT4" : "FT8");
            }

            if (for_me || std::abs(freq - rx_freq_hz) <= 60) {
                rx_freq_activity.push_back(d);
                if (rx_freq_activity.size() > 150) rx_freq_activity.pop_front();
                if (for_me && auto_seq) {
                    if (!p.call_from.empty() && p.call_from != my_call) {
                        dx_call = p.call_from;
                    }
                    if (is_grid4_str(p.extra)) {
                        dx_grid = p.extra;
                    }

                    if (p.extra == "RR73" || p.extra == "RRR") {
                        active_tx_slot = 4; // Tx5: 73
                        qso_prompt_log_pending.store(true);
                    } else if (p.extra == "73") {
                        active_tx_slot = 5; // Tx6: CQ
                        tx_enabled = false;
                        qso_prompt_log_pending.store(true);
                    } else if (p.extra.rfind("R+", 0) == 0 || p.extra.rfind("R-", 0) == 0) {
                        rpt_rcvd = std::atoi(p.extra.c_str() + 1);
                        active_tx_slot = 3; // Tx4: RR73
                    } else if (p.extra.rfind("+", 0) == 0 || p.extra.rfind("-", 0) == 0) {
                        rpt_rcvd = std::atoi(p.extra.c_str());
                        active_tx_slot = 2; // Tx3: R<rpt>
                    } else {
                        // Initial caller replying to our CQ
                        rpt_sent = snr;
                        active_tx_slot = 1; // Tx2: <rpt>
                    }
                    regenerate_std_msgs_locked();
                }
            }
        }
    }
};

struct LogQsoDialogState {
    bool is_open = false;
    std::string call;
    std::string grid;
    std::string mode = "FT8";
    std::string rpt_sent = "-10";
    std::string rpt_rcvd = "-10";
    std::string band_str = "20m";
    std::string freq_str = "14.074000";
    std::string power_w = "50";
    std::string comments;
    std::string date_str;
    std::string time_str;
    uint64_t dial_hz = 0;

    static std::string band_from_freq(uint64_t f) {
        if (f >= 1800000 && f <= 2000000) return "160m";
        if (f >= 3500000 && f <= 4000000) return "80m";
        if (f >= 5330000 && f <= 5410000) return "60m";
        if (f >= 7000000 && f <= 7300000) return "40m";
        if (f >= 10100000 && f <= 10150000) return "30m";
        if (f >= 14000000 && f <= 14350000) return "20m";
        if (f >= 18068000 && f <= 18168000) return "17m";
        if (f >= 21000000 && f <= 21450000) return "15m";
        if (f >= 24890000 && f <= 24990000) return "12m";
        if (f >= 28000000 && f <= 29700000) return "10m";
        if (f >= 50000000 && f <= 54000000) return "6m";
        return "HF";
    }

    void open_for_current_ft8(const Ft8Ft4Engine& ft, uint64_t cur_dial_hz) {
        is_open = true;
        call = ft.dx_call;
        grid = ft.dx_grid;
        mode = ft.is_ft4 ? "FT4" : "FT8";

        char rs[16], rr[16];
        std::snprintf(rs, sizeof(rs), "%+03d", ft.rpt_sent);
        std::snprintf(rr, sizeof(rr), "%+03d", ft.rpt_rcvd);
        rpt_sent = rs;
        rpt_rcvd = rr;

        dial_hz = cur_dial_hz;
        if (dial_hz == 0) dial_hz = ft.is_ft4 ? 14080000ULL : 14074000ULL;
        uint64_t total_hz = dial_hz + static_cast<uint64_t>(std::max(0, ft.tx_freq_hz));

        char fbuf[32];
        std::snprintf(fbuf, sizeof(fbuf), "%.6f", total_hz / 1e6);
        freq_str = fbuf;
        band_str = band_from_freq(dial_hz);
        if (power_w.empty()) power_w = "50";
        comments.clear();

        std::time_t t = std::time(nullptr);
        std::tm gm{};
        hs_gmtime(&t, &gm);
        char d_buf[16], t_buf[16];
        std::snprintf(d_buf, sizeof(d_buf), "%04d%02d%02d", gm.tm_year + 1900, gm.tm_mon + 1, gm.tm_mday);
        std::snprintf(t_buf, sizeof(t_buf), "%02d%02d%02d", gm.tm_hour, gm.tm_min, gm.tm_sec);
        date_str = d_buf;
        time_str = t_buf;
    }

    void save_and_commit(Ft8Ft4Engine& ft, uint64_t vfo_hz) {
        if (call.empty()) return;
        if (band_str.empty()) {
            band_str = band_from_freq(dial_hz > 0 ? dial_hz : vfo_hz);
        }
        // Append standard ADIF record to hamshack_wsjtx_log.adi
        std::ofstream f("hamshack_wsjtx_log.adi", std::ios::app);
        if (f) {
            f << "<call:" << call.size() << ">" << call
              << " <gridsquare:" << grid.size() << ">" << grid
              << " <mode:" << mode.size() << ">" << mode
              << " <rst_sent:" << rpt_sent.size() << ">" << rpt_sent
              << " <rst_rcvd:" << rpt_rcvd.size() << ">" << rpt_rcvd
              << " <qso_date:8>" << date_str
              << " <time_on:6>" << time_str
              << " <freq:" << freq_str.size() << ">" << freq_str
              << " <band:" << band_str.size() << ">" << band_str
              << " <tx_pwr:" << power_w.size() << ">" << power_w;
            if (!comments.empty()) {
                f << " <comment:" << comments.size() << ">" << comments;
            }
            f << " <my_gridsquare:" << ft.my_grid.size() << ">" << ft.my_grid
              << " <station_callsign:" << ft.my_call.size() << ">" << ft.my_call
              << " <eor>\n";
        }

        // Also append to local wsjtx_log.adi if available
        std::ofstream fw("wsjtx_log.adi", std::ios::app);
        if (fw) {
            fw << "<call:" << call.size() << ">" << call
               << " <gridsquare:" << grid.size() << ">" << grid
               << " <mode:" << mode.size() << ">" << mode
               << " <rst_sent:" << rpt_sent.size() << ">" << rpt_sent
               << " <rst_rcvd:" << rpt_rcvd.size() << ">" << rpt_rcvd
               << " <qso_date:8>" << date_str
               << " <time_on:6>" << time_str
               << " <freq:" << freq_str.size() << ">" << freq_str;
            if (!comments.empty()) {
                fw << " <comment:" << comments.size() << ">" << comments;
            }
            fw << " <eor>\n";
        }

        // Add to in-memory QSO log & worked stations
        LoggedQso q{
            date_str, time_str, call, grid, mode,
            dial_hz > 0 ? dial_hz : vfo_hz,
            rpt_sent, rpt_rcvd, comments, false
        };
        {
            std::lock_guard<std::recursive_mutex> lk(ft.mtx);
            ft.qso_log.push_back(q);
            ft.record_worked_station_locked(call, grid, false);
            ft.adif_status_text = "hamshack_wsjtx_log.adi (" + std::to_string(ft.qso_log.size()) + " QSOs, " +
                                  std::to_string(ft.worked_calls.size()) + " calls, " +
                                  std::to_string(ft.worked_grids.size()) + " grids)";
        }
    }
};

} // namespace hamshack
