#pragma once
// ============================================================================
// winlink_engine.hpp — Native Winlink Email Client & B2F v3.2 Protocol Engine
// Integrates Winlink 2000 CMS Telnet, ARDOP TNC Host Interface, B2 LZHUF +
// XMODEM CRC-16 Compression, Mailbox storage (Inbox, Outbox, Sent, Archive),
// and Live Refreshable RMS Gateways Directory with Xiegu G90 CAT QSY.
// ============================================================================

#include <string>
#include <vector>
#include <array>
#include <deque>
#include <unordered_set>
#include <unordered_map>
#include <mutex>
#include <thread>
#include <atomic>
#include <chrono>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <cmath>
#include <cstring>
#include <cctype>
#include <cstdlib>
#include <functional>
#include <filesystem>
#include "../src/hamshack_platform.hpp"

#include "winlink_gateways_data.hpp"
#include "../src/hamshack_config.hpp"
#include "ardop_modem.hpp"

namespace winlink {

struct WinlinkAttachment {
    std::string filename;
    std::string content_type;
    std::vector<uint8_t> data;
    size_t size_bytes = 0;
};

struct WinlinkMessage {
    std::string mid;            // 12-char unique identifier, e.g. "WL2K4A9102B1"
    std::string date_str;       // UTC "YYYY/MM/DD HH:MM" (B2F standard)
    std::string from;           // e.g. "W1AW" or "SMTP:user@example.com"
    std::string to;             // e.g. "W7ABC" or "SMTP:user@example.com"
    std::string cc;
    std::string subject;
    std::string body;
    std::vector<WinlinkAttachment> attachments;
    bool is_read = false;
    size_t total_bytes = 0;
};

struct GatewayChannel {
    std::string callsign;
    uint64_t freq_hz = 0;
    std::string mode;
    std::string grid;
    std::string hours;
    std::string baud;
    std::string service;
    float lat = 0.0f;
    float lon = 0.0f;
    double distance_km = 0.0;
    double azimuth_deg = 0.0;
};

// Simple Base64 decoder/encoder for MIME attachments
inline std::string base64_encode(const std::vector<uint8_t>& in) {
    static constexpr char tbl[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    std::string out;
    int val = 0, valb = -6;
    for (uint8_t c : in) {
        val = (val << 8) + c;
        valb += 8;
        while (valb >= 0) {
            out.push_back(tbl[(val >> valb) & 0x3F]);
            valb -= 6;
        }
    }
    if (valb > -6) out.push_back(tbl[((val << 8) >> (valb + 8)) & 0x3F]);
    while (out.size() % 4) out.push_back('=');
    return out;
}

inline std::vector<uint8_t> base64_decode(const std::string& in) {
    std::vector<uint8_t> out;
    std::vector<int> T(256, -1);
    for (int i = 0; i < 64; ++i)
        T[static_cast<unsigned char>("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/"[i])] = i;
    int val = 0, valb = -8;
    for (unsigned char c : in) {
        if (T[c] == -1) continue;
        val = (val << 6) + T[c];
        valb += 6;
        if (valb >= 0) {
            out.push_back(static_cast<uint8_t>((val >> valb) & 0xFF));
            valb -= 8;
        }
    }
    return out;
}

// ============================================================================
// B2 LZHUF Compressor / Decompressor + XMODEM CRC-16 (Winlink B2F Standard)
// ============================================================================
class B2Lzhuf {
public:
    static constexpr int kN         = 2048;
    static constexpr int kF         = 60;
    static constexpr int kNil       = kN;
    static constexpr int kThreshold = 2;
    static constexpr int kNumChar   = 256 - kThreshold + kF; // 314
    static constexpr int kT         = (kNumChar * 2) - 1;    // 627
    static constexpr int kR         = kT - 1;                // 626
    static constexpr uint32_t kMaxFreq = 0x8000;

    static uint16_t crc16_step(uint8_t byte_val, uint16_t sum) {
        static constexpr std::array<uint16_t, 256> kCrc16Tab = {
            0x0000, 0x1021, 0x2042, 0x3063, 0x4084, 0x50a5, 0x60c6, 0x70e7,
            0x8108, 0x9129, 0xa14a, 0xb16b, 0xc18c, 0xd1ad, 0xe1ce, 0xf1ef,
            0x1231, 0x0210, 0x3273, 0x2252, 0x52b5, 0x4294, 0x72f7, 0x62d6,
            0x9339, 0x8318, 0xb37b, 0xa35a, 0xd3bd, 0xc39c, 0xf3ff, 0xe3de,
            0x2462, 0x3443, 0x0420, 0x1401, 0x64e6, 0x74c7, 0x44a4, 0x5485,
            0xa56a, 0xb54b, 0x8528, 0x9509, 0xe5ee, 0xf5cf, 0xc5ac, 0xd58d,
            0x3653, 0x2672, 0x1611, 0x0630, 0x76d7, 0x66f6, 0x5695, 0x46b4,
            0xb75b, 0xa77a, 0x9719, 0x8738, 0xf7df, 0xe7fe, 0xd79d, 0xc7bc,
            0x48c4, 0x58e5, 0x6886, 0x78a7, 0x0840, 0x1861, 0x2802, 0x3823,
            0xc9cc, 0xd9ed, 0xe98e, 0xf9af, 0x8948, 0x9969, 0xa90a, 0xb92b,
            0x5af5, 0x4ad4, 0x7ab7, 0x6a96, 0x1a71, 0x0a50, 0x3a33, 0x2a12,
            0xdbfd, 0xcbdc, 0xfbbf, 0xeb9e, 0x9b79, 0x8b58, 0xbb3b, 0xab1a,
            0x6ca6, 0x7c87, 0x4ce4, 0x5cc5, 0x2c22, 0x3c03, 0x0c60, 0x1c41,
            0xedae, 0xfd8f, 0xcdec, 0xddcd, 0xad2a, 0xbd0b, 0x8d68, 0x9d49,
            0x7e97, 0x6eb6, 0x5ed5, 0x4ef4, 0x3e13, 0x2e32, 0x1e51, 0x0e70,
            0xff9f, 0xefbe, 0xdfdd, 0xcffc, 0xbf1b, 0xaf3a, 0x9f59, 0x8f78,
            0x9188, 0x81a9, 0xb1ca, 0xa1eb, 0xd10c, 0xc12d, 0xf14e, 0xe16f,
            0x1080, 0x00a1, 0x30c2, 0x20e3, 0x5004, 0x4025, 0x7046, 0x6067,
            0x83b9, 0x9398, 0xa3fb, 0xb3da, 0xc33d, 0xd31c, 0xe37f, 0xf35e,
            0x02b1, 0x1290, 0x22f3, 0x32d2, 0x4235, 0x5214, 0x6277, 0x7256,
            0xb5ea, 0xa5cb, 0x95a8, 0x8589, 0xf56e, 0xe54f, 0xd52c, 0xc50d,
            0x34e2, 0x24c3, 0x14a0, 0x0481, 0x7466, 0x6447, 0x5424, 0x4405,
            0xa7db, 0xb7fa, 0x8799, 0x97b8, 0xe75f, 0xf77e, 0xc71d, 0xd73c,
            0x26d3, 0x36f2, 0x0691, 0x16b0, 0x6657, 0x7676, 0x4615, 0x5634,
            0xd94c, 0xc96d, 0xf90e, 0xe92f, 0x99c8, 0x89e9, 0xb98a, 0xa9ab,
            0x5844, 0x4865, 0x7806, 0x6827, 0x18c0, 0x08e1, 0x3882, 0x28a3,
            0xcb7d, 0xdb5c, 0xeb3f, 0xfb1e, 0x8bf9, 0x9bd8, 0xabbb, 0xbb9a,
            0x4a75, 0x5a54, 0x6a37, 0x7a16, 0x0af1, 0x1ad0, 0x2ab3, 0x3a92,
            0xfd2e, 0xed0f, 0xdd6c, 0xcd4d, 0xbdaa, 0xad8b, 0x9de8, 0x8dc9,
            0x7c26, 0x6c07, 0x5c64, 0x4c45, 0x3ca2, 0x2c83, 0x1ce0, 0x0cc1,
            0xef1f, 0xff3e, 0xcf5d, 0xdf7c, 0xaf9b, 0xbfba, 0x8fd9, 0x9ff8,
            0x6e17, 0x7e36, 0x4e55, 0x5e74, 0x2e93, 0x3eb2, 0x0ed1, 0x1ef0
        };
        return static_cast<uint16_t>(((sum << 8) & 0xFF00u) ^ kCrc16Tab[(sum >> 8) & 0xFFu] ^ static_cast<uint16_t>(byte_val));
    }

    static uint16_t crc16_b2(const uint8_t* data, size_t len) {
        uint16_t sum = 0;
        for (size_t i = 0; i < len; ++i) sum = crc16_step(data[i], sum);
        sum = crc16_step(0, sum);
        sum = crc16_step(0, sum);
        return sum;
    }

private:
    struct State {
        std::array<uint32_t, kT + 1> freq{};
        std::array<int, kT + kNumChar> prnt{};
        std::array<int, kT> son{};
        std::array<int, kN + 1> dad{};
        std::array<int, kN + 1> lson{};
        std::array<int, kN + 257> rson{};
        std::array<uint8_t, kN + kF - 1> text_buf{};
        int match_length = 0;
        int match_position = 0;

        State() {
            for (int i = 0; i < kNumChar; ++i) {
                freq[i] = 1;
                son[i] = i + kT;
                prnt[i + kT] = i;
            }
            for (int i = 0, j = kNumChar; j <= kR; i += 2, ++j) {
                freq[j] = freq[i] + freq[i + 1];
                son[j] = i;
                prnt[i] = j;
                prnt[i + 1] = j;
            }
            freq[kT] = 0xFFFFu;
            prnt[kR] = 0;
        }

        void init_tree() {
            for (int i = kN + 1; i <= kN + 256; ++i) rson[i] = kNil;
            for (int i = 0; i < kN; ++i) dad[i] = kNil;
        }

        void delete_node(int p) {
            if (dad[p] == kNil) return;
            int q = 0;
            if (rson[p] == kNil) {
                q = lson[p];
            } else if (lson[p] == kNil) {
                q = rson[p];
            } else {
                q = lson[p];
                if (rson[q] != kNil) {
                    do { q = rson[q]; } while (rson[q] != kNil);
                    rson[dad[q]] = lson[q];
                    dad[lson[q]] = dad[q];
                    lson[q] = lson[p];
                    dad[lson[p]] = q;
                }
                rson[q] = rson[p];
                dad[rson[p]] = q;
            }
            dad[q] = dad[p];
            if (rson[dad[p]] == p) rson[dad[p]] = q;
            else lson[dad[p]] = q;
            dad[p] = kNil;
        }

        void insert_node(int r) {
            int cmp = 1;
            int p = kN + 1 + static_cast<int>(text_buf[r]);
            rson[r] = kNil;
            lson[r] = kNil;
            match_length = 0;
            for (;;) {
                if (cmp >= 0) {
                    if (rson[p] != kNil) p = rson[p];
                    else { rson[p] = r; dad[r] = p; return; }
                } else {
                    if (lson[p] != kNil) p = lson[p];
                    else { lson[p] = r; dad[r] = p; return; }
                }
                int i = 1;
                for (; i < kF; ++i) {
                    cmp = static_cast<int>(text_buf[r + i]) - static_cast<int>(text_buf[p + i]);
                    if (cmp != 0) break;
                }
                if (i > kThreshold) {
                    if (i > match_length) {
                        match_position = ((r - p) & (kN - 1)) - 1;
                        match_length = i;
                        if (match_length >= kF) break;
                    }
                    if (i == match_length) {
                        int c = ((r - p) & (kN - 1)) - 1;
                        if (c < match_position) match_position = c;
                    }
                }
            }
            dad[r] = dad[p];
            lson[r] = lson[p];
            rson[r] = rson[p];
            dad[lson[p]] = r;
            dad[rson[p]] = r;
            if (rson[dad[p]] == p) rson[dad[p]] = r;
            else lson[dad[p]] = r;
            dad[p] = kNil;
        }

        void reconst() {
            int j = 0;
            for (int i = 0; i < kT; ++i) {
                if (son[i] >= kT) {
                    freq[j] = (freq[i] + 1) / 2;
                    son[j] = son[i];
                    ++j;
                }
            }
            for (int i = 0, jj = kNumChar; jj < kT; i += 2, ++jj) {
                int k = i + 1;
                uint32_t f = freq[i] + freq[k];
                freq[jj] = f;
                for (k = jj; f < freq[k - 1]; --k) {}
                int move_n = jj - k;
                if (move_n > 0) {
                    std::memmove(&freq[k + 1], &freq[k], static_cast<size_t>(move_n) * sizeof(uint32_t));
                    std::memmove(&son[k + 1], &son[k], static_cast<size_t>(move_n) * sizeof(int));
                }
                freq[k] = f;
                son[k] = i;
            }
            for (int i = 0; i < kT; ++i) {
                int k = son[i];
                if (k >= kT) {
                    prnt[k] = i;
                } else {
                    prnt[k] = i;
                    prnt[k + 1] = i;
                }
            }
        }

        void update(int c) {
            if (freq[kR] == kMaxFreq) reconst();
            c = prnt[c + kT];
            for (;;) {
                ++freq[c];
                if (c + 2 >= static_cast<int>(freq.size()) || freq[c] <= freq[c + 1]) {
                    c = prnt[c];
                    if (c == 0) break;
                    continue;
                }
                int l = c + 1;
                uint32_t k = freq[c];
                while (k > freq[l + 1]) ++l;
                freq[c] = freq[l];
                freq[l] = k;
                int i = son[c];
                prnt[i] = l;
                if (i < kT) prnt[i + 1] = l;
                int j = son[l];
                son[l] = i;
                prnt[j] = c;
                if (j < kT) prnt[j + 1] = c;
                son[c] = j;
                c = prnt[l];
                if (c == 0) break;
            }
        }
    };

    static constexpr std::array<uint8_t, 64> kPCode = {
        0x00, 0x20, 0x30, 0x40, 0x50, 0x58, 0x60, 0x68,
        0x70, 0x78, 0x80, 0x88, 0x90, 0x94, 0x98, 0x9C,
        0xA0, 0xA4, 0xA8, 0xAC, 0xB0, 0xB4, 0xB8, 0xBC,
        0xC0, 0xC2, 0xC4, 0xC6, 0xC8, 0xCA, 0xCC, 0xCE,
        0xD0, 0xD2, 0xD4, 0xD6, 0xD8, 0xDA, 0xDC, 0xDE,
        0xE0, 0xE2, 0xE4, 0xE6, 0xE8, 0xEA, 0xEC, 0xEE,
        0xF0, 0xF1, 0xF2, 0xF3, 0xF4, 0xF5, 0xF6, 0xF7,
        0xF8, 0xF9, 0xFA, 0xFB, 0xFC, 0xFD, 0xFE, 0xFF
    };
    static constexpr std::array<uint8_t, 64> kPLen = {
        0x03, 0x04, 0x04, 0x04, 0x05, 0x05, 0x05, 0x05,
        0x05, 0x05, 0x05, 0x05, 0x06, 0x06, 0x06, 0x06,
        0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06,
        0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07,
        0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07,
        0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07,
        0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08,
        0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08
    };
    static constexpr std::array<uint8_t, 256> kDCode = {
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
        0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02,
        0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03,
        0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05,
        0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07,
        0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09,
        0x0A, 0x0A, 0x0A, 0x0A, 0x0A, 0x0A, 0x0A, 0x0A, 0x0B, 0x0B, 0x0B, 0x0B, 0x0B, 0x0B, 0x0B, 0x0B,
        0x0C, 0x0C, 0x0C, 0x0C, 0x0D, 0x0D, 0x0D, 0x0D, 0x0E, 0x0E, 0x0E, 0x0E, 0x0F, 0x0F, 0x0F, 0x0F,
        0x10, 0x10, 0x10, 0x10, 0x11, 0x11, 0x11, 0x11, 0x12, 0x12, 0x12, 0x12, 0x13, 0x13, 0x13, 0x13,
        0x14, 0x14, 0x14, 0x14, 0x15, 0x15, 0x15, 0x15, 0x16, 0x16, 0x16, 0x16, 0x17, 0x17, 0x17, 0x17,
        0x18, 0x18, 0x19, 0x19, 0x1A, 0x1A, 0x1B, 0x1B, 0x1C, 0x1C, 0x1D, 0x1D, 0x1E, 0x1E, 0x1F, 0x1F,
        0x20, 0x20, 0x21, 0x21, 0x22, 0x22, 0x23, 0x23, 0x24, 0x24, 0x25, 0x25, 0x26, 0x26, 0x27, 0x27,
        0x28, 0x28, 0x29, 0x29, 0x2A, 0x2A, 0x2B, 0x2B, 0x2C, 0x2C, 0x2D, 0x2D, 0x2E, 0x2E, 0x2F, 0x2F,
        0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x3A, 0x3B, 0x3C, 0x3D, 0x3E, 0x3F
    };
    static constexpr std::array<uint8_t, 256> kDLen = {
        0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03,
        0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03,
        0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04,
        0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04,
        0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04,
        0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05,
        0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05,
        0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05,
        0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05,
        0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06,
        0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06,
        0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06,
        0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07,
        0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07,
        0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07,
        0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08
    };

public:
    static std::vector<uint8_t> compress(const uint8_t* data, size_t data_len) {
        auto z = std::make_unique<State>();
        z->init_tree();
        int r = kN - kF;
        int s = 0;
        for (int i = 0; i < r; ++i) z->text_buf[i] = ' ';

        uint32_t putbuf = 0;
        uint8_t putlen = 0;
        std::vector<uint8_t> body;
        body.reserve(data_len / 2 + 16);

        auto put_code = [&](int l, uint32_t c) {
            putbuf |= (c >> putlen);
            putlen = static_cast<uint8_t>(putlen + l);
            if (putlen < 8) return;
            body.push_back(static_cast<uint8_t>((putbuf >> 8) & 0xFFu));
            putlen = static_cast<uint8_t>(putlen - 8);
            if (putlen >= 8) {
                body.push_back(static_cast<uint8_t>(putbuf & 0xFFu));
                putlen = static_cast<uint8_t>(putlen - 8);
                putbuf = (c << (l - static_cast<int>(putlen))) & 0xFFFFu;
            } else {
                putbuf = (putbuf << 8) & 0xFFFFu;
            }
        };

        auto encode_char = [&](uint32_t c) {
            uint32_t i = 0;
            int j = 0;
            int k = z->prnt[c + kT];
            do {
                i >>= 1;
                if (k & 1) i += 0x8000u;
                ++j;
                k = z->prnt[k];
            } while (k != kR);
            put_code(j, i);
            z->update(static_cast<int>(c));
        };

        auto encode_pos = [&](uint32_t c) {
            uint32_t i = c >> 6;
            put_code(static_cast<int>(kPLen[i]), static_cast<uint32_t>(kPCode[i]) << 8);
            put_code(6, (c & 0x3Fu) << 10);
        };

        int len = 0;
        int last_match_len = 0;
        size_t in_pos = 0;
        while (len < kF && in_pos < data_len) {
            z->text_buf[r + len] = data[in_pos++];
            ++len;
            z->insert_node(r - len);
            last_match_len = 1;
        }

        auto do_encode = [&]() {
            if (len == 0) return;
            if (z->match_length > len) z->match_length = len;
            if (z->match_length <= kThreshold) {
                z->match_length = 1;
                encode_char(z->text_buf[r]);
            } else {
                encode_char(static_cast<uint32_t>(255 - kThreshold + z->match_length));
                encode_pos(static_cast<uint32_t>(z->match_position));
            }
            last_match_len = z->match_length;
        };

        auto advance = [&](const uint8_t* next_byte) {
            if (next_byte) {
                z->text_buf[s] = *next_byte;
                if (s < kF - 1) z->text_buf[s + kN] = *next_byte;
                ++len;
            }
            z->insert_node(r);
            --last_match_len;
            if (last_match_len == 0) do_encode();
            z->delete_node(s);
            s = (s + 1) & (kN - 1);
            r = (r + 1) & (kN - 1);
            --len;
        };

        while (in_pos < data_len) {
            advance(&data[in_pos++]);
        }
        while (len > 0) {
            advance(nullptr);
        }
        do_encode();
        if (putlen > 0) {
            body.push_back(static_cast<uint8_t>((putbuf >> 8) & 0xFFu));
        }

        std::vector<uint8_t> size_and_body;
        size_and_body.reserve(4 + body.size());
        uint32_t fsz = static_cast<uint32_t>(data_len);
        size_and_body.push_back(static_cast<uint8_t>(fsz & 0xFFu));
        size_and_body.push_back(static_cast<uint8_t>((fsz >> 8) & 0xFFu));
        size_and_body.push_back(static_cast<uint8_t>((fsz >> 16) & 0xFFu));
        size_and_body.push_back(static_cast<uint8_t>((fsz >> 24) & 0xFFu));
        size_and_body.insert(size_and_body.end(), body.begin(), body.end());

        uint16_t crc = crc16_b2(size_and_body.data(), size_and_body.size());
        std::vector<uint8_t> out;
        out.reserve(2 + size_and_body.size());
        out.push_back(static_cast<uint8_t>(crc & 0xFFu));
        out.push_back(static_cast<uint8_t>((crc >> 8) & 0xFFu));
        out.insert(out.end(), size_and_body.begin(), size_and_body.end());
        return out;
    }

    static bool decompress(const uint8_t* comp, size_t comp_len, std::vector<uint8_t>& out, std::string* out_err = nullptr) {
        out.clear();
        if (comp_len < 6) {
            if (out_err) *out_err = "payload shorter than 6-byte B2 header";
            return false;
        }
        uint16_t expected_crc = static_cast<uint16_t>(comp[0]) | (static_cast<uint16_t>(comp[1]) << 8);
        uint16_t actual_crc = crc16_b2(comp + 2, comp_len - 2);
        if (expected_crc != actual_crc) {
            if (out_err) *out_err = "B2 CRC16 mismatch";
            return false;
        }

        uint32_t target_size = static_cast<uint32_t>(comp[2]) |
                              (static_cast<uint32_t>(comp[3]) << 8) |
                              (static_cast<uint32_t>(comp[4]) << 16) |
                              (static_cast<uint32_t>(comp[5]) << 24);
        if (target_size > 20 * 1024 * 1024) return false;
        if (target_size == 0) return true;

        auto z = std::make_unique<State>();
        for (int i = 0; i < kN - kF; ++i) z->text_buf[i] = ' ';
        int state_r = kN - kF;

        size_t byte_idx = 6;
        uint64_t bit_buf = 0;
        uint32_t bit_cnt = 0;
        bool eof_err = false;

        auto read_bits = [&](uint32_t nbits) -> uint32_t {
            while (bit_cnt < nbits) {
                if (byte_idx >= comp_len) {
                    eof_err = true;
                    return 0;
                }
                bit_buf = (bit_buf << 8) | static_cast<uint64_t>(comp[byte_idx++]);
                bit_cnt += 8;
            }
            uint32_t val = static_cast<uint32_t>((bit_buf >> (bit_cnt - nbits)) & ((1ULL << nbits) - 1ULL));
            bit_cnt -= nbits;
            return val;
        };

        auto decode_char = [&]() -> uint32_t {
            uint32_t c = static_cast<uint32_t>(z->son[kR]);
            while (c < static_cast<uint32_t>(kT)) {
                c += read_bits(1);
                if (eof_err) return 0;
                c = static_cast<uint32_t>(z->son[c]);
            }
            c -= static_cast<uint32_t>(kT);
            z->update(static_cast<int>(c));
            return c;
        };

        auto decode_pos = [&]() -> int {
            uint32_t i = read_bits(8);
            uint32_t c = static_cast<uint32_t>(kDCode[i]) << 6;
            uint32_t j = static_cast<uint32_t>(kDLen[i]);
            for (j -= 2; j > 0; --j) {
                i = (i << 1) + read_bits(1);
            }
            return static_cast<int>(c | (i & 0x3Fu));
        };

        out.reserve(target_size);
        while (out.size() < target_size && !eof_err) {
            uint32_t c = decode_char();
            if (eof_err) return false;
            if (c < 256u) {
                uint8_t b = static_cast<uint8_t>(c);
                out.push_back(b);
                z->text_buf[state_r] = b;
                state_r = (state_r + 1) & (kN - 1);
            } else {
                int pos = (state_r - decode_pos() - 1) & (kN - 1);
                int len = static_cast<int>(c) - 255 + kThreshold;
                for (int k = 0; k < len && out.size() < target_size; ++k) {
                    uint8_t b = z->text_buf[(pos + k) & (kN - 1)];
                    out.push_back(b);
                    z->text_buf[state_r] = b;
                    state_r = (state_r + 1) & (kN - 1);
                }
            }
        }
        return out.size() == target_size;
    }
};

// RFC 1321 MD5 Digest implementation for Winlink B2F Secure Login Challenge-Response
class Md5 {
public:
    static std::array<uint8_t, 16> hash(const uint8_t* initial_msg, size_t initial_len) {
        uint32_t h0 = 0x67452301;
        uint32_t h1 = 0xefcdab89;
        uint32_t h2 = 0x98badcfe;
        uint32_t h3 = 0x10325476;

        static constexpr uint32_t k[64] = {
            0xd76aa478, 0xe8c7b756, 0x242070db, 0xc1bdceee,
            0xf57c0faf, 0x4787c62a, 0xa8304613, 0xfd469501,
            0x698098d8, 0x8b44f7af, 0xffff5bb1, 0x895cd7be,
            0x6b901122, 0xfd987193, 0xa679438e, 0x49b40821,
            0xf61e2562, 0xc040b340, 0x265e5a51, 0xe9b6c7aa,
            0xd62f105d, 0x02441453, 0xd8a1e681, 0xe7d3fbc8,
            0x21e1cde6, 0xc33707d6, 0xf4d50d87, 0x455a14ed,
            0xa9e3e905, 0xfcefa3f8, 0x676f02d9, 0x8d2a4c8a,
            0xfffa3942, 0x8771f681, 0x6d9d6122, 0xfde5380c,
            0xa4beea44, 0x4bdecfa9, 0xf6bb4b60, 0xbebfbc70,
            0x289b7ec6, 0xeaa127fa, 0xd4ef3085, 0x04881d05,
            0xd9d4d039, 0xe6db99e5, 0x1fa27cf8, 0xc4ac5665,
            0xf4292244, 0x432aff97, 0xab9423a7, 0xfc93a039,
            0x655b59c3, 0x8f0ccc92, 0xffeff47d, 0x85845dd1,
            0x6fa87e4f, 0xfe2ce6e0, 0xa3014314, 0x4e0811a1,
            0xf7537e82, 0xbd3af235, 0x2ad7d2bb, 0xeb86d391
        };

        static constexpr uint32_t r[64] = {
            7, 12, 17, 22,  7, 12, 17, 22,  7, 12, 17, 22,  7, 12, 17, 22,
            5,  9, 14, 20,  5,  9, 14, 20,  5,  9, 14, 20,  5,  9, 14, 20,
            4, 11, 16, 23,  4, 11, 16, 23,  4, 11, 16, 23,  4, 11, 16, 23,
            6, 10, 15, 21,  6, 10, 15, 21,  6, 10, 15, 21,  6, 10, 15, 21
        };

        size_t new_len = ((((initial_len + 8) / 64) + 1) * 64);
        std::vector<uint8_t> msg(new_len, 0);
        std::memcpy(msg.data(), initial_msg, initial_len);
        msg[initial_len] = 0x80;

        uint64_t bits_len = static_cast<uint64_t>(initial_len) * 8;
        for (int i = 0; i < 8; ++i) {
            msg[new_len - 8 + i] = static_cast<uint8_t>((bits_len >> (i * 8)) & 0xFF);
        }

        auto left_rotate = [](uint32_t x, uint32_t c) -> uint32_t {
            return (x << c) | (x >> (32 - c));
        };

        for (size_t offset = 0; offset < new_len; offset += 64) {
            uint32_t w[16];
            for (int i = 0; i < 16; ++i) {
                w[i] = static_cast<uint32_t>(msg[offset + i * 4]) |
                      (static_cast<uint32_t>(msg[offset + i * 4 + 1]) << 8) |
                      (static_cast<uint32_t>(msg[offset + i * 4 + 2]) << 16) |
                      (static_cast<uint32_t>(msg[offset + i * 4 + 3]) << 24);
            }

            uint32_t a = h0, b = h1, c = h2, d = h3;
            for (uint32_t i = 0; i < 64; ++i) {
                uint32_t f, g;
                if (i < 16) { f = (b & c) | ((~b) & d); g = i; }
                else if (i < 32) { f = (d & b) | ((~d) & c); g = (5 * i + 1) % 16; }
                else if (i < 48) { f = b ^ c ^ d; g = (3 * i + 5) % 16; }
                else { f = c ^ (b | (~d)); g = (7 * i) % 16; }
                uint32_t temp = d;
                d = c;
                c = b;
                b = b + left_rotate(a + f + k[i] + w[g], r[i]);
                a = temp;
            }
            h0 += a; h1 += b; h2 += c; h3 += d;
        }

        std::array<uint8_t, 16> digest{};
        auto store32 = [&](uint32_t val, int idx) {
            digest[idx]     = static_cast<uint8_t>(val & 0xFF);
            digest[idx + 1] = static_cast<uint8_t>((val >> 8) & 0xFF);
            digest[idx + 2] = static_cast<uint8_t>((val >> 16) & 0xFF);
            digest[idx + 3] = static_cast<uint8_t>((val >> 24) & 0xFF);
        };
        store32(h0, 0); store32(h1, 4); store32(h2, 8); store32(h3, 12);
        return digest;
    }
};

inline std::string calculate_winlink_secure_login_response(const std::string& challenge, const std::string& password) {
    static constexpr std::array<uint8_t, 64> raw_salt = {
        77, 197, 101, 206, 190, 249, 93, 200, 51, 243, 93, 237, 71, 94, 239, 138,
        68, 108, 70, 185, 225, 137, 217, 16, 51, 122, 193, 48, 194, 195, 198, 175,
        172, 169, 70, 84, 61, 62, 104, 186, 114, 52, 61, 168, 66, 129, 192, 208,
        187, 249, 232, 193, 41, 113, 41, 45, 240, 16, 29, 228, 208, 228, 61, 20
    };
    std::vector<uint8_t> inp;
    inp.reserve(challenge.size() + password.size() + raw_salt.size());
    for (char c : challenge) inp.push_back(static_cast<uint8_t>(c));
    for (char c : password) inp.push_back(static_cast<uint8_t>(c));
    for (uint8_t b : raw_salt) inp.push_back(b);

    auto h = Md5::hash(inp.data(), inp.size());
    uint32_t resp_int = static_cast<uint32_t>(h[0]) |
                       (static_cast<uint32_t>(h[1]) << 8) |
                       (static_cast<uint32_t>(h[2]) << 16) |
                       ((static_cast<uint32_t>(h[3]) & 0x3Fu) << 24);
    char buf[32];
    std::snprintf(buf, sizeof(buf), "%08u", resp_int);
    std::string s(buf);
    if (s.size() > 8) s = s.substr(s.size() - 8);
    return s;
}

// ============================================================================
// Winlink B2F Address, Date & Wire Message Formatting Helpers
// ============================================================================
inline std::string trim_ws(std::string s) {
    while (!s.empty() && (s.front() == ' ' || s.front() == '\t' || s.front() == '\r' || s.front() == '\n')) s.erase(s.begin());
    while (!s.empty() && (s.back() == ' ' || s.back() == '\t' || s.back() == '\r' || s.back() == '\n')) s.pop_back();
    return s;
}

inline std::string to_upper_ascii(std::string s) {
    for (char& c : s) c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
    return s;
}

// Formats a single recipient or sender per Winlink B2F rules:
// - "W1AW" or "W1AW@winlink.org" -> "W1AW"
// - "user@example.com" -> "SMTP:user@example.com"
// - "SMTP:user@example.com" -> "SMTP:user@example.com"
inline std::string format_b2f_address(std::string addr) {
    addr = trim_ws(addr);
    // Strip display name if "Name <email>"
    size_t lt = addr.find('<'), gt = addr.find('>');
    if (lt != std::string::npos && gt != std::string::npos && gt > lt + 1) {
        addr = trim_ws(addr.substr(lt + 1, gt - lt - 1));
    }
    if (addr.empty()) return "";
    size_t colon = addr.find(':');
    if (colon != std::string::npos) {
        std::string proto = to_upper_ascii(addr.substr(0, colon));
        std::string rest = trim_ws(addr.substr(colon + 1));
        return proto + ":" + rest;
    }
    size_t at = addr.find('@');
    if (at == std::string::npos) {
        return to_upper_ascii(addr);
    }
    std::string user = trim_ws(addr.substr(0, at));
    std::string dom = to_upper_ascii(trim_ws(addr.substr(at + 1)));
    if (dom == "WINLINK.ORG") {
        return to_upper_ascii(user);
    }
    return "SMTP:" + user + "@" + trim_ws(addr.substr(at + 1));
}

inline std::vector<std::string> split_b2f_addresses(const std::string& raw) {
    std::vector<std::string> out;
    std::string cur;
    for (char c : raw) {
        if (c == ',' || c == ';') {
            std::string f = format_b2f_address(cur);
            if (!f.empty()) out.push_back(f);
            cur.clear();
        } else {
            cur.push_back(c);
        }
    }
    std::string f = format_b2f_address(cur);
    if (!f.empty()) out.push_back(f);
    return out;
}

inline std::string sanitize_ascii_header(const std::string& s, size_t max_len = 80) {
    std::string out;
    out.reserve(std::min(s.size(), max_len));
    for (unsigned char c : s) {
        if (c == '\r' || c == '\n' || c == 0) continue;
        out.push_back((c >= 32 && c < 127) ? static_cast<char>(c) : '?');
        if (out.size() >= max_len) break;
    }
    return out;
}

// Normalizes CRLF line endings inside message body and ensures non-empty body (CMS requires Body > 0)
inline std::string normalize_b2f_body_crlf(const std::string& raw) {
    std::string out;
    out.reserve(raw.size() + 16);
    for (size_t i = 0; i < raw.size(); ++i) {
        if (raw[i] == '\r') {
            out += "\r\n";
            if (i + 1 < raw.size() && raw[i + 1] == '\n') ++i;
        } else if (raw[i] == '\n') {
            out += "\r\n";
        } else {
            out.push_back(raw[i]);
        }
    }
    if (out.empty()) out = "\r\n";
    else if (out.size() < 2 || out.substr(out.size() - 2) != "\r\n") out += "\r\n";
    return out;
}

// Ensures B2F Date header is strictly "YYYY/MM/DD HH:MM" UTC
inline std::string ensure_b2f_date_format(const std::string& existing) {
    // Check if already "YYYY/MM/DD HH:MM"
    if (existing.size() >= 16 && existing[4] == '/' && existing[7] == '/' && existing[10] == ' ' && existing[13] == ':') {
        return existing.substr(0, 16);
    }
    std::time_t t = std::time(nullptr);
    std::tm tm_buf{};
    hamshack::hs_gmtime(&t, &tm_buf);
    char buf[32];
    std::snprintf(buf, sizeof(buf), "%04d/%02d/%02d %02d:%02d",
                  tm_buf.tm_year + 1900, tm_buf.tm_mon + 1, tm_buf.tm_mday,
                  tm_buf.tm_hour, tm_buf.tm_min);
    return buf;
}

// Serializes a WinlinkMessage into exact Winlink B2F uncompressed wire bytes
inline std::vector<uint8_t> serialize_b2f_wire_message(const WinlinkMessage& msg, const std::string& fallback_from) {
    std::string body_crlf = normalize_b2f_body_crlf(msg.body);
    std::string from_addr = format_b2f_address(msg.from.empty() ? fallback_from : msg.from);
    // CMS expects bare callsign in From/Mbo when sent from a radio/telnet client
    if (from_addr.rfind("SMTP:", 0) == 0 && !fallback_from.empty()) {
        from_addr = to_upper_ascii(trim_ws(fallback_from));
    }
    auto to_list = split_b2f_addresses(msg.to);
    if (to_list.empty() && !fallback_from.empty()) to_list.push_back(to_upper_ascii(trim_ws(fallback_from)));
    auto cc_list = split_b2f_addresses(msg.cc);
    std::string subj = sanitize_ascii_header(msg.subject.empty() ? "(No Subject)" : msg.subject, 120);
    if (subj.empty()) subj = "(No Subject)";
    std::string b2_date = ensure_b2f_date_format(msg.date_str);

    std::string hdr;
    hdr.reserve(512);
    hdr += "Mid: " + sanitize_ascii_header(msg.mid, 12) + "\r\n";
    hdr += "Body: " + std::to_string(body_crlf.size()) + "\r\n";
    hdr += "Content-Transfer-Encoding: 8bit\r\n";
    hdr += "Content-Type: text/plain; charset=UTF-8\r\n";
    hdr += "Date: " + b2_date + "\r\n";
    hdr += "From: " + from_addr + "\r\n";
    hdr += "Mbo: " + from_addr + "\r\n";
    hdr += "Subject: " + subj + "\r\n";
    for (const auto& t : to_list) hdr += "To: " + t + "\r\n";
    for (const auto& c : cc_list) hdr += "Cc: " + c + "\r\n";
    hdr += "Type: Private\r\n";
    for (const auto& att : msg.attachments) {
        std::string fn = sanitize_ascii_header(att.filename.empty() ? "attachment.bin" : att.filename, 80);
        hdr += "File: " + std::to_string(att.data.size()) + " " + fn + "\r\n";
    }
    hdr += "\r\n"; // Blank line separating B2F headers from body

    std::vector<uint8_t> out;
    out.reserve(hdr.size() + body_crlf.size() + 256);
    out.insert(out.end(), hdr.begin(), hdr.end());
    out.insert(out.end(), body_crlf.begin(), body_crlf.end());
    if (!msg.attachments.empty()) {
        out.push_back('\r');
        out.push_back('\n');
        for (const auto& att : msg.attachments) {
            out.insert(out.end(), att.data.begin(), att.data.end());
            out.push_back('\r');
            out.push_back('\n');
        }
    }
    return out;
}

// Parses both standard Winlink B2F wire messages (blank line after headers) AND legacy local .b2f files
inline bool parse_b2f_wire_message(const uint8_t* data, size_t len, WinlinkMessage& msg) {
    msg = WinlinkMessage{};
    size_t pos = 0;
    int body_len = -1;
    std::vector<std::pair<size_t, std::string>> pending_files;
    bool legacy_inline_body = false;

    auto read_hdr_line = [&](std::string& line_out) -> bool {
        if (pos >= len) return false;
        size_t start = pos;
        while (pos < len && data[pos] != '\n') ++pos;
        size_t end = pos;
        if (pos < len && data[pos] == '\n') ++pos;
        if (end > start && data[end - 1] == '\r') --end;
        line_out.assign(reinterpret_cast<const char*>(data + start), end - start);
        return true;
    };

    std::string line;
    while (read_hdr_line(line)) {
        if (line.empty()) {
            break; // Standard B2F end-of-headers blank line
        }
        size_t colon = line.find(':');
        if (colon == std::string::npos) continue;
        std::string key = to_upper_ascii(trim_ws(line.substr(0, colon)));
        std::string val = trim_ws(line.substr(colon + 1));

        if (key == "MID") msg.mid = val;
        else if (key == "DATE") msg.date_str = val;
        else if (key == "FROM") msg.from = val;
        else if (key == "TO") {
            if (!msg.to.empty()) msg.to += ", ";
            msg.to += val;
        }
        else if (key == "CC") {
            if (!msg.cc.empty()) msg.cc += ", ";
            msg.cc += val;
        }
        else if (key == "SUBJECT") msg.subject = val;
        else if (key == "BODY") {
            body_len = std::atoi(val.c_str());
            // Check if this is a legacy local .b2f where body immediately followed "Body: N\r\n" with no blank line
            // In standard B2F, either more headers ("Content-...", "Date:", "File:") or "\r\n" follow "Body:".
            size_t peek = pos;
            bool looks_like_hdr_or_blank = false;
            if (peek < len && (data[peek] == '\r' || data[peek] == '\n')) {
                looks_like_hdr_or_blank = true;
            } else {
                size_t eol = peek;
                while (eol < len && eol - peek < 64 && data[eol] != '\n' && data[eol] != ':') ++eol;
                if (eol < len && data[eol] == ':') {
                    std::string nk = to_upper_ascii(trim_ws(std::string(reinterpret_cast<const char*>(data + peek), eol - peek)));
                    if (nk == "CONTENT-TRANSFER-ENCODING" || nk == "CONTENT-TYPE" || nk == "DATE" ||
                        nk == "FROM" || nk == "MBO" || nk == "SUBJECT" || nk == "TO" || nk == "CC" ||
                        nk == "TYPE" || nk == "FILE" || nk == "X-SOURCE") {
                        looks_like_hdr_or_blank = true;
                    }
                }
            }
            if (!looks_like_hdr_or_blank && body_len >= 0) {
                legacy_inline_body = true;
                size_t take = std::min<size_t>(static_cast<size_t>(body_len), len - pos);
                msg.body.assign(reinterpret_cast<const char*>(data + pos), take);
                pos += take;
                if (pos < len && data[pos] == '\r') ++pos;
                if (pos < len && data[pos] == '\n') ++pos;
            }
        }
        else if (key == "FILE") {
            size_t sp = val.find(' ');
            if (sp != std::string::npos) {
                size_t fsz = static_cast<size_t>(std::strtoull(val.substr(0, sp).c_str(), nullptr, 10));
                std::string fname = trim_ws(val.substr(sp + 1));
                if (legacy_inline_body) {
                    size_t take = std::min<size_t>(fsz, len - pos);
                    WinlinkAttachment att;
                    att.filename = fname;
                    att.data.assign(data + pos, data + pos + take);
                    att.size_bytes = take;
                    msg.attachments.push_back(std::move(att));
                    pos += take;
                    if (pos < len && data[pos] == '\r') ++pos;
                    if (pos < len && data[pos] == '\n') ++pos;
                } else {
                    pending_files.emplace_back(fsz, fname);
                }
            }
        }
    }

    if (!legacy_inline_body && body_len >= 0 && pos <= len) {
        size_t take = std::min<size_t>(static_cast<size_t>(body_len), len - pos);
        msg.body.assign(reinterpret_cast<const char*>(data + pos), take);
        pos += take;
        if (!pending_files.empty()) {
            if (pos < len && data[pos] == '\r') ++pos;
            if (pos < len && data[pos] == '\n') ++pos;
            for (const auto& pf : pending_files) {
                if (pos > len) break;
                size_t ftake = std::min<size_t>(pf.first, len - pos);
                WinlinkAttachment att;
                att.filename = pf.second;
                att.data.assign(data + pos, data + pos + ftake);
                att.size_bytes = ftake;
                msg.attachments.push_back(std::move(att));
                pos += ftake;
                if (pos < len && data[pos] == '\r') ++pos;
                if (pos < len && data[pos] == '\n') ++pos;
            }
        }
    }

    msg.total_bytes = msg.body.size();
    for (const auto& a : msg.attachments) msg.total_bytes += a.data.size();
    return !msg.mid.empty();
}

// Fast zero-dependency parser for https://api.winlink.org/gateway/status.json
inline size_t parse_winlink_gateways_json(const std::string& json, std::vector<GatewayChannel>& out_list) {
    out_list.clear();
    const size_t n = json.size();
    size_t pos = json.find("\"Gateways\"");
    if (pos == std::string::npos) return 0;
    pos = json.find('[', pos);
    if (pos == std::string::npos) return 0;
    ++pos;

    auto skip_ws = [&]() {
        while (pos < n && (json[pos] == ' ' || json[pos] == '\t' || json[pos] == '\r' || json[pos] == '\n')) ++pos;
    };

    auto parse_str = [&]() -> std::string {
        skip_ws();
        if (pos >= n || json[pos] != '"') return "";
        ++pos;
        std::string s;
        while (pos < n) {
            char c = json[pos++];
            if (c == '"') break;
            if (c == '\\' && pos < n) {
                char esc = json[pos++];
                if (esc == '"' || esc == '\\' || esc == '/') s.push_back(esc);
                else if (esc == 'n') s.push_back(' ');
                else if (esc == 't') s.push_back(' ');
                else if (esc == 'u' && pos + 4 <= n) pos += 4;
            } else {
                s.push_back(c);
            }
        }
        return s;
    };

    auto parse_num_str = [&]() -> std::string {
        skip_ws();
        size_t st = pos;
        while (pos < n && (std::isdigit(static_cast<unsigned char>(json[pos])) ||
                           json[pos] == '-' || json[pos] == '+' || json[pos] == '.' ||
                           json[pos] == 'e' || json[pos] == 'E')) {
            ++pos;
        }
        return json.substr(st, pos - st);
    };

    std::function<void()> skip_value = [&]() {
        skip_ws();
        if (pos >= n) return;
        char c = json[pos];
        if (c == '"') { (void)parse_str(); }
        else if (c == '{') {
            ++pos;
            int depth = 1;
            while (pos < n && depth > 0) {
                if (json[pos] == '"') { (void)parse_str(); continue; }
                if (json[pos] == '{') ++depth;
                else if (json[pos] == '}') --depth;
                ++pos;
            }
        } else if (c == '[') {
            ++pos;
            int depth = 1;
            while (pos < n && depth > 0) {
                if (json[pos] == '"') { (void)parse_str(); continue; }
                if (json[pos] == '[') ++depth;
                else if (json[pos] == ']') --depth;
                ++pos;
            }
        } else {
            while (pos < n && json[pos] != ',' && json[pos] != '}' && json[pos] != ']') ++pos;
        }
    };

    out_list.reserve(8192);
    while (pos < n) {
        skip_ws();
        if (pos >= n || json[pos] == ']') break;
        if (json[pos] == ',') { ++pos; continue; }
        if (json[pos] != '{') break;
        ++pos; // inside Gateway object

        std::string gw_call;
        float gw_lat = 0.0f, gw_lon = 0.0f;

        while (pos < n) {
            skip_ws();
            if (pos >= n) break;
            if (json[pos] == '}') { ++pos; break; }
            if (json[pos] == ',') { ++pos; continue; }
            std::string key = parse_str();
            skip_ws();
            if (pos < n && json[pos] == ':') ++pos;
            skip_ws();

            if (key == "Callsign") {
                gw_call = trim_ws(parse_str());
            } else if (key == "Latitude") {
                gw_lat = static_cast<float>(std::atof(parse_num_str().c_str()));
            } else if (key == "Longitude") {
                gw_lon = static_cast<float>(std::atof(parse_num_str().c_str()));
            } else if (key == "GatewayChannels" && pos < n && json[pos] == '[') {
                ++pos; // inside GatewayChannels array
                while (pos < n) {
                    skip_ws();
                    if (pos >= n) break;
                    if (json[pos] == ']') { ++pos; break; }
                    if (json[pos] == ',') { ++pos; continue; }
                    if (json[pos] != '{') { skip_value(); continue; }
                    ++pos; // inside Channel object

                    GatewayChannel ch;
                    ch.callsign = gw_call;
                    ch.lat = gw_lat;
                    ch.lon = gw_lon;
                    ch.hours = "00-23";
                    ch.service = "PUBLIC";

                    while (pos < n) {
                        skip_ws();
                        if (pos >= n) break;
                        if (json[pos] == '}') { ++pos; break; }
                        if (json[pos] == ',') { ++pos; continue; }
                        std::string ckey = parse_str();
                        skip_ws();
                        if (pos < n && json[pos] == ':') ++pos;
                        skip_ws();

                        if (ckey == "SupportedModes") ch.mode = trim_ws(parse_str());
                        else if (ckey == "Gridsquare") ch.grid = to_upper_ascii(trim_ws(parse_str()));
                        else if (ckey == "Frequency") {
                            double fval = std::atof(parse_num_str().c_str());
                            ch.freq_hz = (fval > 0.0) ? static_cast<uint64_t>(std::llround(fval)) : 0ULL;
                        }
                        else if (ckey == "OperatingHours") ch.hours = trim_ws(parse_str());
                        else if (ckey == "Baud") ch.baud = trim_ws(parse_str());
                        else if (ckey == "ServiceCode") ch.service = trim_ws(parse_str());
                        else skip_value();
                    }

                    if (!ch.callsign.empty() && ch.freq_hz > 0) {
                        if (ch.grid.size() > 6) ch.grid = ch.grid.substr(0, 6);
                        out_list.push_back(std::move(ch));
                    }
                }
            } else {
                skip_value();
            }
        }
    }
    return out_list.size();
}


class WinlinkEngine {
public:
    mutable std::recursive_mutex mtx;

    // Mailbox storage
    std::vector<WinlinkMessage> inbox;
    std::vector<WinlinkMessage> outbox;
    std::vector<WinlinkMessage> sent;
    std::vector<WinlinkMessage> archive;

    int active_folder = 0; // 0=Inbox, 1=Outbox, 2=Sent, 3=Archive
    int selected_msg_idx = 0;
    int msg_list_scroll = 0;

    // Composer State
    bool is_composing = false;
    std::string compose_to;
    std::string compose_cc;
    std::string compose_subject;
    std::string compose_body;
    std::vector<WinlinkAttachment> compose_attachments;

    // Live Gateway Directory & On-Demand Web Refresh
    std::vector<GatewayChannel> all_gateways;
    std::vector<GatewayChannel> filtered_gateways;
    std::string gateway_search_query;
    std::string gateway_mode_filter = "All"; // All, ARDOP, VARA, Packet, Pactor
    int selected_gateway_idx = -1;
    int gateway_scroll = 0;
    std::atomic<bool> gateway_refresh_active{false};
    std::string gateway_refresh_status = "Embedded + Cached Directory Ready";
    std::thread gateway_refresh_thread;

    // Session Execution State
    std::atomic<bool> session_active{false};
    std::atomic<bool> session_abort_req{false};
    std::string session_status = "IDLE — Click 'Send & Receive' to start Winlink session";
    float session_progress = 0.0f; // 0.0 .. 1.0
    int session_tx_count = 0;
    int session_rx_count = 0;
    std::deque<std::string> session_transcript;
    std::thread session_thread;
    hamshack::hs_socket_t active_sock_fd = hamshack::HS_INVALID_SOCKET;

    // Hardware & Modem state cached for UI
    std::string modem_name = "Telnet CMS";
    std::string modem_status = "Ready";
    std::string modem_info = "cms.winlink.org:8772 (B2F v3.2)";
    std::string selected_gateway_call = "W7ABC-10";
    uint64_t selected_gateway_freq = 14103500;
    std::string selected_gateway_grid = "FN31";
    std::string selected_gateway_mode = "ARDOP 1000";

    // Built-in ARDOP TNC Soundcard Modem
    hamshack::ardop::ArdopModem ardop_modem;
    hamshack::FlrigCatEngine* flrig_ptr = nullptr;
    hamshack::SoundcardEngine* soundcard_ptr = nullptr;

    std::string mail_dir_override_;

    void set_radio_interfaces(hamshack::FlrigCatEngine* flrig, hamshack::SoundcardEngine* snd) {
        flrig_ptr = flrig;
        soundcard_ptr = snd;
    }

    explicit WinlinkEngine(const std::string& custom_mail_dir = "") : mail_dir_override_(custom_mail_dir) {
        init_gateways();
        load_mailboxes();
    }

    ~WinlinkEngine() {
        abort_session();
        if (gateway_refresh_thread.joinable()) {
            gateway_refresh_thread.join();
        }
        if (ardop_modem.is_running()) {
            ardop_modem.stop();
        }
    }

    std::string get_mail_dir() const {
        std::string dir = mail_dir_override_.empty()
                              ? (hamshack::AppConfig::get_config_dir() + "/winlink")
                              : mail_dir_override_;
        hamshack::hs_mkdir_p(dir);
        hamshack::hs_mkdir_p(dir + "/inbox");
        hamshack::hs_mkdir_p(dir + "/outbox");
        hamshack::hs_mkdir_p(dir + "/sent");
        hamshack::hs_mkdir_p(dir + "/archive");
        return dir;
    }

    static std::string generate_mid(const std::string& my_call) {
        static constexpr char kChars[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";
        std::string clean_call;
        for (char c : my_call) {
            if (std::isalnum(static_cast<unsigned char>(c))) {
                clean_call.push_back(static_cast<char>(std::toupper(static_cast<unsigned char>(c))));
            }
        }
        std::string base = clean_call.substr(0, std::min<size_t>(clean_call.size(), 4));
        while (base.size() < 4) base.push_back('X');
        std::string mid = base;
        uint64_t seed = static_cast<uint64_t>(
            std::chrono::high_resolution_clock::now().time_since_epoch().count());
        for (int i = 0; i < 8; ++i) {
            seed = seed * 6364136223846793005ULL + 1442695040888963407ULL + static_cast<uint64_t>(std::rand());
            mid.push_back(kChars[(seed >> 32) % 36]);
        }
        return mid;
    }

    static std::string format_current_b2f_date() {
        return ensure_b2f_date_format("");
    }

    static std::string format_current_rfc2822_date() {
        return ensure_b2f_date_format("");
    }

    void load_mailboxes() {
        std::lock_guard<std::recursive_mutex> lk(mtx);
        inbox.clear();
        outbox.clear();
        sent.clear();
        archive.clear();

        load_folder("inbox", inbox);
        load_folder("outbox", outbox);
        load_folder("sent", sent);
        load_folder("archive", archive);

        if (mail_dir_override_.empty() && inbox.empty() && outbox.empty() && sent.empty()) {
            WinlinkMessage welcome;
            welcome.mid = "WL2K00000001";
            welcome.date_str = format_current_b2f_date();
            welcome.from = "SYSTEM";
            welcome.to = "W1AW";
            welcome.subject = "Welcome to HamShack++ Winlink Client";
            welcome.body =
                "Welcome to HamShack++ Integrated Winlink Email Client!\r\n\r\n"
                "Features included in this build:\r\n"
                "- One-click 'Send & Receive' email session via Telnet CMS or ARDOP HF radio.\r\n"
                "- Full B2F (Binary Forwarding Protocol v3.2) + LZHUF compression.\r\n"
                "- Complete Winlink RMS Gateway directory with live web Refresh.\r\n"
                "- Automatic Xiegu G90 CAT QSY to gateway channel frequencies.\r\n"
                "- Cmd+/Cmd- (Ctrl+/Ctrl-) UI scaling and resizable session log.\r\n\r\n"
                "73,\r\n"
                "HamShack++ Engineering Team\r\n";
            welcome.total_bytes = welcome.body.size();
            welcome.is_read = false;
            inbox.push_back(welcome);
            save_message_to_disk("inbox", welcome);
        }
    }

    void load_folder(const std::string& folder_name, std::vector<WinlinkMessage>& list) {
        std::string dir = get_mail_dir() + "/" + folder_name;
        std::error_code ec;
        if (!std::filesystem::exists(dir, ec)) return;
        for (const auto& entry : std::filesystem::directory_iterator(dir, ec)) {
            if (!entry.is_regular_file(ec)) continue;
            std::string fname = entry.path().filename().string();
            if (fname.size() > 4 && (fname.ends_with(".b2f") || fname.ends_with(".eml"))) {
                WinlinkMessage msg;
                if (read_message_from_disk(entry.path().string(), msg)) {
                    msg.is_read = (folder_name != "inbox");
                    list.push_back(std::move(msg));
                }
            }
        }
        std::sort(list.begin(), list.end(), [](const WinlinkMessage& a, const WinlinkMessage& b) {
            if (a.date_str != b.date_str) return a.date_str > b.date_str;
            return a.mid > b.mid;
        });
    }

    bool save_message_to_disk(const std::string& folder_name, const WinlinkMessage& msg) {
        std::string safe_mid = sanitize_ascii_header(msg.mid, 24);
        if (safe_mid.empty()) safe_mid = generate_mid("WL2K");
        std::string path = get_mail_dir() + "/" + folder_name + "/" + safe_mid + ".b2f";
        std::ofstream f(path, std::ios::binary);
        if (!f) return false;
        auto wire = serialize_b2f_wire_message(msg, msg.from);
        f.write(reinterpret_cast<const char*>(wire.data()), static_cast<std::streamsize>(wire.size()));
        return static_cast<bool>(f);
    }

    bool read_message_from_disk(const std::string& filepath, WinlinkMessage& msg) {
        std::ifstream f(filepath, std::ios::binary | std::ios::ate);
        if (!f) return false;
        auto sz = f.tellg();
        if (sz <= 0 || sz > 16 * 1024 * 1024) return false;
        f.seekg(0, std::ios::beg);
        std::vector<uint8_t> raw(static_cast<size_t>(sz));
        if (!f.read(reinterpret_cast<char*>(raw.data()), sz)) return false;
        return parse_b2f_wire_message(raw.data(), raw.size(), msg);
    }

    bool has_message_mid_locked(const std::string& mid) const {
        for (const auto& m : inbox) if (m.mid == mid) return true;
        for (const auto& m : sent) if (m.mid == mid) return true;
        for (const auto& m : archive) if (m.mid == mid) return true;
        return false;
    }

    void delete_message(int folder_idx, int msg_idx) {
        std::lock_guard<std::recursive_mutex> lk(mtx);
        std::vector<WinlinkMessage>* f_list = nullptr;
        std::string f_name;
        if (folder_idx == 0) { f_list = &inbox; f_name = "inbox"; }
        else if (folder_idx == 1) { f_list = &outbox; f_name = "outbox"; }
        else if (folder_idx == 2) { f_list = &sent; f_name = "sent"; }
        else if (folder_idx == 3) { f_list = &archive; f_name = "archive"; }

        if (f_list && msg_idx >= 0 && msg_idx < static_cast<int>(f_list->size())) {
            std::string path = get_mail_dir() + "/" + f_name + "/" + (*f_list)[msg_idx].mid + ".b2f";
            std::error_code ec;
            std::filesystem::remove(path, ec);
            f_list->erase(f_list->begin() + msg_idx);
            if (selected_msg_idx >= static_cast<int>(f_list->size())) {
                selected_msg_idx = static_cast<int>(f_list->size()) - 1;
            }
        }
    }

    void add_compose_attachment(const std::string& filepath) {
        if (filepath.empty()) return;
        std::ifstream f(filepath, std::ios::binary);
        if (!f) return;
        f.seekg(0, std::ios::end);
        size_t sz = static_cast<size_t>(f.tellg());
        f.seekg(0, std::ios::beg);
        if (sz > 2000000) return;

        WinlinkAttachment att;
        size_t last_slash = filepath.find_last_of("/\\");
        att.filename = (last_slash != std::string::npos) ? filepath.substr(last_slash + 1) : filepath;
        att.data.resize(sz);
        f.read(reinterpret_cast<char*>(&att.data[0]), static_cast<std::streamsize>(sz));
        att.size_bytes = sz;

        std::lock_guard<std::recursive_mutex> lk(mtx);
        compose_attachments.push_back(std::move(att));
    }

    bool post_compose_to_outbox(const std::string& my_call) {
        std::lock_guard<std::recursive_mutex> lk(mtx);
        if (trim_ws(compose_to).empty()) return false;

        WinlinkMessage msg;
        msg.mid = generate_mid(my_call);
        msg.date_str = format_current_b2f_date();
        msg.from = to_upper_ascii(trim_ws(my_call));
        msg.to = trim_ws(compose_to);
        msg.cc = trim_ws(compose_cc);
        msg.subject = compose_subject.empty() ? "(No Subject)" : compose_subject;
        msg.body = normalize_b2f_body_crlf(compose_body);
        msg.attachments = compose_attachments;
        msg.total_bytes = msg.body.size();
        for (const auto& a : msg.attachments) msg.total_bytes += a.data.size();
        msg.is_read = true;

        save_message_to_disk("outbox", msg);
        outbox.insert(outbox.begin(), msg);

        compose_to.clear();
        compose_cc.clear();
        compose_subject.clear();
        compose_body.clear();
        compose_attachments.clear();
        is_composing = false;
        active_folder = 1;
        selected_msg_idx = 0;
        return true;
    }

    static void compute_distance_and_azimuth(double lat1, double lon1, double lat2, double lon2,
                                             double& dist_km, double& az_deg) {
        static constexpr double kPi = 3.14159265358979323846;
        double p1 = lat1 * kPi / 180.0, p2 = lat2 * kPi / 180.0;
        double dp = (lat2 - lat1) * kPi / 180.0;
        double dl = (lon2 - lon1) * kPi / 180.0;
        double a = std::sin(dp / 2.0) * std::sin(dp / 2.0) +
                   std::cos(p1) * std::cos(p2) * std::sin(dl / 2.0) * std::sin(dl / 2.0);
        double c = 2.0 * std::atan2(std::sqrt(a), std::sqrt(std::max(0.0, 1.0 - a)));
        dist_km = 6371.0 * c;

        double y = std::sin(dl) * std::cos(p2);
        double x = std::cos(p1) * std::sin(p2) - std::sin(p1) * std::cos(p2) * std::cos(dl);
        az_deg = std::fmod((std::atan2(y, x) * 180.0 / kPi + 360.0), 360.0);
    }

    static bool grid_to_latlon(const std::string& g, double& lat, double& lon) {
        if (g.size() < 4) return false;
        char c0 = static_cast<char>(std::toupper(static_cast<unsigned char>(g[0])));
        char c1 = static_cast<char>(std::toupper(static_cast<unsigned char>(g[1])));
        char c2 = g[2], c3 = g[3];
        if (c0 < 'A' || c0 > 'R' || c1 < 'A' || c1 > 'R' || c2 < '0' || c2 > '9' || c3 < '0' || c3 > '9') return false;
        lon = -180.0 + (c0 - 'A') * 20.0 + (c2 - '0') * 2.0 + 1.0;
        lat = -90.0 + (c1 - 'A') * 10.0 + (c3 - '0') * 1.0 + 0.5;
        return true;
    }

    bool load_cached_gateways(std::vector<GatewayChannel>& out_list) const {
        std::string tsv_path = get_mail_dir() + "/gateways_live.tsv";
        std::ifstream f(tsv_path);
        if (!f) return false;
        out_list.clear();
        std::string line;
        while (std::getline(f, line)) {
            if (!line.empty() && line.back() == '\r') line.pop_back();
            if (line.empty() || line[0] == '#') continue;
            std::vector<std::string> cols;
            size_t st = 0;
            while (st <= line.size()) {
                size_t tab = line.find('\t', st);
                if (tab == std::string::npos) {
                    cols.push_back(line.substr(st));
                    break;
                }
                cols.push_back(line.substr(st, tab - st));
                st = tab + 1;
            }
            if (cols.size() < 9) continue;
            GatewayChannel gw;
            gw.callsign = cols[0];
            gw.freq_hz = static_cast<uint64_t>(std::strtoull(cols[1].c_str(), nullptr, 10));
            gw.mode = cols[2];
            gw.grid = cols[3];
            gw.hours = cols[4];
            gw.baud = cols[5];
            gw.service = cols[6];
            gw.lat = static_cast<float>(std::atof(cols[7].c_str()));
            gw.lon = static_cast<float>(std::atof(cols[8].c_str()));
            if (!gw.callsign.empty() && gw.freq_hz > 0) out_list.push_back(std::move(gw));
        }
        return out_list.size() >= 100;
    }

    bool save_cached_gateways(const std::vector<GatewayChannel>& list) const {
        std::string tsv_path = get_mail_dir() + "/gateways_live.tsv";
        std::ofstream f(tsv_path, std::ios::binary);
        if (!f) return false;
        f << "# HamShack++ Cached Live Winlink RMS Gateways\n";
        for (const auto& gw : list) {
            f << gw.callsign << '\t' << gw.freq_hz << '\t' << gw.mode << '\t'
              << gw.grid << '\t' << gw.hours << '\t' << gw.baud << '\t'
              << gw.service << '\t' << gw.lat << '\t' << gw.lon << '\n';
        }
        return static_cast<bool>(f);
    }

    void init_gateways() {
        std::vector<GatewayChannel> cached;
        if (mail_dir_override_.empty() && load_cached_gateways(cached)) {
            all_gateways = std::move(cached);
            gateway_refresh_status = "Loaded " + std::to_string(all_gateways.size()) + " cached live gateways";
        } else {
            all_gateways.clear();
            all_gateways.reserve(kEmbeddedGatewayCount);
            for (size_t i = 0; i < kEmbeddedGatewayCount; ++i) {
                const auto& raw = kEmbeddedGateways[i];
                GatewayChannel gw;
                gw.callsign = raw.call;
                gw.freq_hz = raw.freq;
                gw.mode = raw.mode;
                gw.grid = raw.grid;
                gw.hours = raw.hours;
                gw.baud = raw.baud;
                gw.service = raw.service;
                gw.lat = raw.lat;
                gw.lon = raw.lon;
                all_gateways.push_back(std::move(gw));
            }
            gateway_refresh_status = "Embedded directory (" + std::to_string(all_gateways.size()) + " channels)";
        }
        filter_gateways("", "All", hamshack::get_app_config().hf_grid);
    }

    void refresh_gateways_from_web(const std::string& my_grid) {
        if (gateway_refresh_active.exchange(true)) return;
        {
            std::lock_guard<std::recursive_mutex> lk(mtx);
            gateway_refresh_status = "Downloading live RMS directory from api.winlink.org...";
        }
        if (gateway_refresh_thread.joinable()) gateway_refresh_thread.join();
        gateway_refresh_thread = std::thread([this, my_grid]() {
            std::string json_path = get_mail_dir() + "/gateways_fetch_tmp.json";
            std::error_code ec;
            std::filesystem::remove(json_path, ec);

            bool ok_exec = false;
#ifdef _WIN32
            std::string cmd = "curl.exe -s -S --max-time 25 -X POST "
                              "-d \"Mode=AnyAll&key=1880278F11684B358F36845615BD039A\" "
                              "\"https://api.winlink.org/gateway/status.json\" -o \"" + json_path + "\"";
            STARTUPINFOA si{};
            si.cb = sizeof(si);
            PROCESS_INFORMATION pi{};
            std::vector<char> cmd_buf(cmd.begin(), cmd.end());
            cmd_buf.push_back('\0');
            if (CreateProcessA(nullptr, cmd_buf.data(), nullptr, nullptr, FALSE,
                               CREATE_NO_WINDOW, nullptr, nullptr, &si, &pi)) {
                WaitForSingleObject(pi.hProcess, 30000);
                DWORD exit_code = 1;
                GetExitCodeProcess(pi.hProcess, &exit_code);
                CloseHandle(pi.hProcess);
                CloseHandle(pi.hThread);
                ok_exec = (exit_code == 0);
            }
#else
            std::string cmd = "/usr/bin/curl -s -S --max-time 25 -X POST "
                              "-d 'Mode=AnyAll&key=1880278F11684B358F36845615BD039A' "
                              "'https://api.winlink.org/gateway/status.json' -o '" + json_path + "' >/dev/null 2>&1";
            ok_exec = (std::system(cmd.c_str()) == 0);
#endif
            std::string json_body;
            if (ok_exec) {
                std::ifstream jf(json_path, std::ios::binary | std::ios::ate);
                if (jf) {
                    auto sz = jf.tellg();
                    if (sz > 1024 && sz < 64 * 1024 * 1024) {
                        jf.seekg(0, std::ios::beg);
                        json_body.resize(static_cast<size_t>(sz));
                        jf.read(json_body.data(), sz);
                    }
                }
            }
            std::filesystem::remove(json_path, ec);

            std::vector<GatewayChannel> fresh;
            if (!json_body.empty()) {
                parse_winlink_gateways_json(json_body, fresh);
            }

            if (fresh.size() >= 100) {
                save_cached_gateways(fresh);
                std::string cur_q, cur_m;
                {
                    std::lock_guard<std::recursive_mutex> lk(mtx);
                    cur_q = gateway_search_query;
                    cur_m = gateway_mode_filter;
                    all_gateways = std::move(fresh);
                    gateway_refresh_status = "Updated live from winlink.org (" + std::to_string(all_gateways.size()) + " channels)";
                }
                filter_gateways(cur_q, cur_m, my_grid);
                log_session("Refreshed RMS Gateway Directory from api.winlink.org (" +
                            std::to_string(all_gateways.size()) + " active channels).");
            } else {
                std::lock_guard<std::recursive_mutex> lk(mtx);
                gateway_refresh_status = "Refresh failed (check internet); keeping " + std::to_string(all_gateways.size()) + " gateways";
            }
            gateway_refresh_active.store(false);
        });
    }

    void filter_gateways(const std::string& query, const std::string& mode, const std::string& my_grid) {
        std::lock_guard<std::recursive_mutex> lk(mtx);
        gateway_search_query = query;
        gateway_mode_filter = mode;

        double my_lat = 40.5, my_lon = -111.0;
        grid_to_latlon(my_grid, my_lat, my_lon);

        std::string q_upper = to_upper_ascii(query);

        filtered_gateways.clear();
        for (auto& gw : all_gateways) {
            if (mode != "All") {
                std::string m_up = to_upper_ascii(gw.mode);
                std::string want_up = to_upper_ascii(mode);
                if (m_up.find(want_up) == std::string::npos) continue;
            }
            if (!q_upper.empty()) {
                std::string call_u = to_upper_ascii(gw.callsign);
                std::string mode_u = to_upper_ascii(gw.mode);
                char f_buf[32];
                std::snprintf(f_buf, sizeof(f_buf), "%.4f", gw.freq_hz / 1e6);
                if (call_u.find(q_upper) == std::string::npos &&
                    gw.grid.find(q_upper) == std::string::npos &&
                    mode_u.find(q_upper) == std::string::npos &&
                    std::string(f_buf).find(q_upper) == std::string::npos) {
                    continue;
                }
            }
            compute_distance_and_azimuth(my_lat, my_lon, gw.lat, gw.lon, gw.distance_km, gw.azimuth_deg);
            filtered_gateways.push_back(gw);
        }

        std::sort(filtered_gateways.begin(), filtered_gateways.end(), [](const GatewayChannel& a, const GatewayChannel& b) {
            if (std::abs(a.distance_km - b.distance_km) > 1.0) {
                return a.distance_km < b.distance_km;
            }
            return a.freq_hz < b.freq_hz;
        });

        if (selected_gateway_idx >= static_cast<int>(filtered_gateways.size())) {
            selected_gateway_idx = filtered_gateways.empty() ? -1 : 0;
        }
    }

    void select_gateway(size_t idx) {
        std::lock_guard<std::recursive_mutex> lk(mtx);
        if (idx < filtered_gateways.size()) {
            selected_gateway_idx = static_cast<int>(idx);
            const auto& gw = filtered_gateways[idx];
            selected_gateway_call = gw.callsign;
            selected_gateway_freq = gw.freq_hz;
            selected_gateway_grid = gw.grid;
            selected_gateway_mode = gw.mode;
        }
    }

    void log_session(const std::string& line) {
        std::lock_guard<std::recursive_mutex> lk(mtx);
        std::time_t t = std::time(nullptr);
        std::tm tm_buf{};
        hamshack::hs_localtime(&t, &tm_buf);
        char t_buf[16];
        std::snprintf(t_buf, sizeof(t_buf), "%02d:%02d:%02d ", tm_buf.tm_hour, tm_buf.tm_min, tm_buf.tm_sec);
        session_transcript.push_back(t_buf + line);
        if (session_transcript.size() > 500) session_transcript.pop_front();
    }

    void clear_session_log() {
        std::lock_guard<std::recursive_mutex> lk(mtx);
        session_transcript.clear();
        if (!session_active) {
            session_progress = 0.0f;
            session_tx_count = 0;
            session_rx_count = 0;
            session_status = "IDLE — Click 'Send & Receive' to start Winlink session";
        }
    }

    void start_session(const std::string& transport, const std::string& call, const std::string& pass,
                       const std::string& grid, const std::string& gw_call, uint64_t gw_freq) {
        if (session_active) {
            abort_session();
            return;
        }
        session_active = true;
        session_abort_req = false;
        session_progress = 0.05f;
        session_tx_count = 0;
        session_rx_count = 0;
        session_status = "Starting " + transport + " session to " + gw_call + "...";

        log_session("*** WINLINK B2F SESSION INITIATED ***");
        log_session("Transport: " + transport + " | Call: " + call + " | Gateway: " + gw_call);

        if (session_thread.joinable()) session_thread.join();
        session_thread = std::thread(&WinlinkEngine::session_worker_thread, this, transport, call, pass, grid, gw_call, gw_freq);
    }

    void abort_session() {
        session_abort_req = true;
        if (hamshack::hs_sock_valid(active_sock_fd)) {
            hamshack::hs_shutdown_both(active_sock_fd);
            hamshack::hs_close_socket(active_sock_fd);
            active_sock_fd = hamshack::HS_INVALID_SOCKET;
        }
        if (ardop_modem.is_running()) {
            ardop_modem.stop();
        }
        if (session_thread.joinable()) {
            session_thread.join();
        }
        session_active = false;
        session_status = "Session aborted.";
        log_session("*** SESSION ABORTED BY OPERATOR ***");
    }

private:
    void session_worker_thread(std::string transport, std::string call, std::string pass,
                               std::string grid, std::string gw_call, uint64_t gw_freq) {
        hamshack::ensure_winsock();
        const auto& cfg = hamshack::get_app_config();
        call = to_upper_ascii(trim_ws(call));
        pass = trim_ws(pass);
        grid = to_upper_ascii(trim_ws(grid));
        if (grid.empty()) grid = "DM00";

        bool is_telnet = (transport == "Telnet" || transport == "Internet" || transport == "CMS");
        std::string cms_host = trim_ws(cfg.winlink_cms_server.empty() ? "cms.winlink.org" : cfg.winlink_cms_server);
        int cms_port = (cfg.winlink_cms_port >= 1 && cfg.winlink_cms_port <= 65535) ? cfg.winlink_cms_port : 8772;
        std::string cms_remote_call = to_upper_ascii(trim_ws(cfg.winlink_cms_call.empty() ? "WL2K" : cfg.winlink_cms_call));
        std::string cms_gate_pass = trim_ws(cfg.winlink_cms_passcode.empty() ? "CMSTELNET" : cfg.winlink_cms_passcode);
        int cms_timeout_s = std::clamp(cfg.winlink_cms_timeout_sec, 5, 300);

        std::string host = is_telnet ? cms_host : (cfg.winlink_ardop_host.empty() ? "127.0.0.1" : cfg.winlink_ardop_host);
        int port = is_telnet ? cms_port : (cfg.winlink_ardop_port > 0 ? cfg.winlink_ardop_port : 8515);
        if (is_telnet && (gw_call.empty() || gw_call == "CMS")) {
            gw_call = cms_remote_call;
        }
        gw_call = to_upper_ascii(trim_ws(gw_call));
        modem_info = cms_host + ":" + std::to_string(cms_port) + " (" + cms_remote_call + " • " + std::to_string(cms_timeout_s) + "s)";

        auto fail_session = [&](const std::string& status_msg) {
            session_status = status_msg;
            log_session("*** " + status_msg + " ***");
            if (hamshack::hs_sock_valid(active_sock_fd)) {
                hamshack::hs_close_socket(active_sock_fd);
                active_sock_fd = hamshack::HS_INVALID_SOCKET;
            }
            if (!is_telnet && ardop_modem.is_running()) {
                ardop_modem.stop();
            }
            session_active = false;
        };

        if (call.empty() || call == "N0CALL") {
            fail_session("Configuration Error: Set your Callsign in Winlink Configuration.");
            return;
        }

        if (!is_telnet) {
            log_session("[ARDOP] Starting built-in ARDOP HF TNC Modem on 127.0.0.1:" + std::to_string(port) + "...");
            if (!ardop_modem.is_running()) {
                if (!ardop_modem.start(port, flrig_ptr, soundcard_ptr)) {
                    fail_session("ARDOP Modem Bind Error on port " + std::to_string(port));
                    return;
                }
            }
            log_session("[ARDOP] Built-in ARDOP Modem ONLINE. Soundcard & Xiegu G90 CAT linked.");
            std::this_thread::sleep_for(std::chrono::milliseconds(150));
        }

        log_session("Resolving " + host + ":" + std::to_string(port) + "...");
        session_progress = 0.10f;

        struct addrinfo hints{}, *res = nullptr;
        hints.ai_family = AF_UNSPEC;
        hints.ai_socktype = SOCK_STREAM;
        std::string port_str = std::to_string(port);

        if (getaddrinfo(host.c_str(), port_str.c_str(), &hints, &res) != 0 || !res) {
            fail_session("Connection Failed: Could not resolve " + host);
            return;
        }

        active_sock_fd = ::socket(res->ai_family, res->ai_socktype, res->ai_protocol);
        if (!hamshack::hs_sock_valid(active_sock_fd)) {
            freeaddrinfo(res);
            fail_session("Socket creation error.");
            return;
        }

        int timeout_ms = is_telnet ? (cms_timeout_s * 1000) : 20000;
        hamshack::hs_set_rw_timeout_ms(active_sock_fd, timeout_ms);

        log_session("Connecting to Winlink RMS (" + host + ":" + port_str + ")...");
        if (::connect(active_sock_fd, res->ai_addr, static_cast<int>(res->ai_addrlen)) < 0) {
            freeaddrinfo(res);
            fail_session(is_telnet ? "Connection Refused / CMS Timeout." : "ARDOP Local Connection Failed.");
            return;
        }
        freeaddrinfo(res);

        session_progress = 0.25f;
        session_status = is_telnet ? "Connected! Authenticating with Winlink CMS..." : "Connected to ARDOP TNC!";
        log_session("Connected to " + host + ":" + port_str);

        // Buffered socket reader so binary SOH/STX/EOT streams never lose a byte
        std::deque<uint8_t> rx_q;
        auto fill_rx = [&]() -> bool {
            if (!rx_q.empty()) return true;
            if (session_abort_req || !hamshack::hs_sock_valid(active_sock_fd)) return false;
            uint8_t tmp[1024];
            hamshack::hs_ssize_t n = ::recv(active_sock_fd, reinterpret_cast<char*>(tmp), sizeof(tmp), 0);
            if (n <= 0) return false;
            for (hamshack::hs_ssize_t i = 0; i < n; ++i) rx_q.push_back(tmp[i]);
            return true;
        };

        auto peek_byte = [&](uint8_t& b) -> bool {
            if (!fill_rx()) return false;
            b = rx_q.front();
            return true;
        };

        auto read_byte = [&](uint8_t& b) -> bool {
            if (!fill_rx()) return false;
            b = rx_q.front();
            rx_q.pop_front();
            return true;
        };

        auto read_exact = [&](uint8_t* dst, size_t count) -> bool {
            for (size_t i = 0; i < count; ++i) {
                if (!read_byte(dst[i])) return false;
            }
            return true;
        };

        auto send_raw = [&](const uint8_t* data, size_t len) -> bool {
            size_t sent_tot = 0;
            while (sent_tot < len && !session_abort_req && hamshack::hs_sock_valid(active_sock_fd)) {
                hamshack::hs_ssize_t n = ::send(active_sock_fd,
                                                reinterpret_cast<const char*>(data + sent_tot),
                                                static_cast<int>(len - sent_tot), 0);
                if (n <= 0) return false;
                sent_tot += static_cast<size_t>(n);
            }
            return sent_tot == len;
        };

        // Winlink CMS & B2F strictly terminate protocol lines with CR ('\r', 0x0D) ONLY!
        auto send_cr = [&](const std::string& l, bool mask_in_log = false) -> bool {
            std::string wire = l + "\r";
            bool ok = send_raw(reinterpret_cast<const uint8_t*>(wire.data()), wire.size());
            if (!mask_in_log) {
                log_session("> " + l);
            }
            return ok;
        };

        auto read_until_char = [&](char target) -> std::string {
            std::string buf;
            uint8_t b = 0;
            while (read_byte(b)) {
                char c = static_cast<char>(b);
                if (c != '\r' && c != '\n') buf.push_back(c);
                if (c == target) break;
                if (buf.find("Disconnecting") != std::string::npos) break;
            }
            return buf;
        };

        auto read_banner_line = [&]() -> std::string {
            std::string s;
            uint8_t b = 0;
            while (read_byte(b)) {
                char c = static_cast<char>(b);
                if (c == '\r' || c == '\n') {
                    if (!s.empty()) break;
                    continue;
                }
                s.push_back(c);
                if (s.ends_with("CMS>") || s.ends_with("Disconnecting...")) break;
            }
            if (!s.empty()) log_session("< " + s);
            return s;
        };

        auto read_b2f_line = [&](std::string& out) -> bool {
            out.clear();
            uint8_t b = 0;
            while (read_byte(b)) {
                if (b == '\r') {
                    if (!out.empty()) log_session("< " + out);
                    return true;
                }
                if (b == '\n') {
                    if (out.empty()) continue; // skip stray LF after CR if a TNC emitted CRLF
                    if (!out.empty()) log_session("< " + out);
                    return true;
                }
                out.push_back(static_cast<char>(b));
            }
            if (!out.empty()) {
                log_session("< " + out);
                return true;
            }
            return false;
        };

        auto send_b2f_compressed_stream = [&](const std::string& subj, const std::vector<uint8_t>& comp) -> bool {
            static constexpr uint8_t kCHRSOH = 0x01;
            static constexpr uint8_t kCHRSTX = 0x02;
            static constexpr uint8_t kCHREOT = 0x04;
            std::string title = sanitize_ascii_header(subj.empty() ? "(No Subject)" : subj, 80);
            if (title.empty()) title = "Msg";
            const std::string offset_str = "0";

            std::vector<uint8_t> pkt;
            pkt.reserve(comp.size() + title.size() + 64);
            // Header: SOH + len(title + NUL + offset + NUL) + title + NUL + "0" + NUL
            uint8_t hdr_len = static_cast<uint8_t>(title.size() + offset_str.size() + 2);
            pkt.push_back(kCHRSOH);
            pkt.push_back(hdr_len);
            pkt.insert(pkt.end(), title.begin(), title.end());
            pkt.push_back(0x00);
            pkt.insert(pkt.end(), offset_str.begin(), offset_str.end());
            pkt.push_back(0x00);

            int sum = 0;
            size_t pos = 0;
            while (pos < comp.size()) {
                size_t chunk = std::min<size_t>(125, comp.size() - pos);
                pkt.push_back(kCHRSTX);
                pkt.push_back(static_cast<uint8_t>(chunk));
                for (size_t i = 0; i < chunk; ++i) {
                    uint8_t v = comp[pos + i];
                    pkt.push_back(v);
                    sum = (sum + v) & 0xFF;
                }
                pos += chunk;
            }
            uint8_t eot_chk = static_cast<uint8_t>((-sum) & 0xFF);
            pkt.push_back(kCHREOT);
            pkt.push_back(eot_chk);
            return send_raw(pkt.data(), pkt.size());
        };

        auto read_b2f_compressed_stream = [&](std::vector<uint8_t>& out_comp, std::string& out_err) -> bool {
            static constexpr uint8_t kCHRSOH = 0x01;
            static constexpr uint8_t kCHRSTX = 0x02;
            static constexpr uint8_t kCHREOT = 0x04;
            out_comp.clear();
            uint8_t c = 0;
            if (!read_byte(c)) { out_err = "Unexpected EOF waiting for B2F SOH"; return false; }
            if (c == '\n') {
                if (!read_byte(c)) { out_err = "Unexpected EOF after LF"; return false; }
            }
            if (c != kCHRSOH) {
                // Might be "*** ..." error text from CMS
                std::string rest(1, static_cast<char>(c));
                std::string line_tail;
                read_b2f_line(line_tail);
                out_err = "Expected SOH (0x01), got: " + rest + line_tail;
                return false;
            }
            uint8_t hdr_len = 0;
            if (!read_byte(hdr_len) || hdr_len < 3) {
                out_err = "Invalid B2F binary header length";
                return false;
            }
            std::vector<uint8_t> hdr_buf(hdr_len);
            if (!read_exact(hdr_buf.data(), hdr_len)) {
                out_err = "Truncated B2F binary header";
                return false;
            }
            int sum = 0;
            while (true) {
                uint8_t blk_type = 0;
                if (!read_byte(blk_type)) {
                    out_err = "Truncated B2F binary stream";
                    return false;
                }
                if (blk_type == kCHRSTX) {
                    uint8_t len_b = 0;
                    if (!read_byte(len_b)) { out_err = "Missing STX length"; return false; }
                    size_t chunk_sz = (len_b == 0) ? 256 : static_cast<size_t>(len_b);
                    size_t old_sz = out_comp.size();
                    out_comp.resize(old_sz + chunk_sz);
                    if (!read_exact(out_comp.data() + old_sz, chunk_sz)) {
                        out_err = "Truncated STX payload";
                        return false;
                    }
                    for (size_t i = 0; i < chunk_sz; ++i) {
                        sum = (sum + out_comp[old_sz + i]) & 0xFF;
                    }
                } else if (blk_type == kCHREOT) {
                    uint8_t rx_chk = 0;
                    if (!read_byte(rx_chk)) { out_err = "Missing EOT checksum"; return false; }
                    uint8_t want_chk = static_cast<uint8_t>((-sum) & 0xFF);
                    if (want_chk != rx_chk) {
                        out_err = "B2F EOT checksum mismatch";
                        return false;
                    }
                    return true;
                } else {
                    out_err = "Unknown B2F framing byte " + std::to_string(static_cast<int>(blk_type));
                    return false;
                }
            }
        };

        // ====================================================================
        // 1. Handshake & Secure Login
        // ====================================================================
        if (is_telnet) {
            log_session("CMS Profile: " + cms_host + ":" + std::to_string(cms_port) +
                        " | Remote Call: " + cms_remote_call + " | Timeout: " + std::to_string(cms_timeout_s) + "s");

            std::string prompt1 = read_until_char(':');
            if (!prompt1.empty()) log_session("< " + prompt1);
            if (prompt1.find("Callsign") == std::string::npos) {
                fail_session("Unexpected CMS greeting (no Callsign prompt).");
                return;
            }
            if (!send_cr(call)) {
                fail_session("Socket write error sending Callsign.");
                return;
            }
            log_session("Sent Callsign: " + call + " -> Remote Gate: " + cms_remote_call);

            std::string prompt2 = read_until_char(':');
            if (!prompt2.empty()) log_session("< " + prompt2);
            if (prompt2.find("Password") != std::string::npos) {
                if (!send_cr(cms_gate_pass, true)) {
                    fail_session("Socket write error sending CMSTELNET passcode.");
                    return;
                }
                log_session("> " + std::string(cms_gate_pass.size(), '*') + " (Telnet Gate Passcode)");
            }

            session_progress = 0.40f;
            std::string banner;
            std::string challenge;
            bool got_sid = false;

            while (!session_abort_req && hamshack::hs_sock_valid(active_sock_fd)) {
                std::string l = read_banner_line();
                if (l.empty()) break;
                banner += l + "\n";
                if (l.find("[WL2K-") != std::string::npos) got_sid = true;
                if (l.find(";PQ:") != std::string::npos) {
                    size_t p = l.find(";PQ:");
                    std::string rem = trim_ws(l.substr(p + 4));
                    size_t sp = rem.find_first_of(" \t\r\n");
                    challenge = (sp != std::string::npos) ? rem.substr(0, sp) : rem;
                }
                if (l.find("CMS>") != std::string::npos ||
                    l.find("Disconnecting") != std::string::npos ||
                    l.find("Invalid") != std::string::npos) {
                    break;
                }
            }

            if (banner.find("Invalid callsign") != std::string::npos) {
                fail_session("Invalid Callsign rejected by CMS: " + call);
                return;
            }
            if (banner.find("Telnet access code") != std::string::npos) {
                fail_session("CMS Gate Passcode Error (must be CMSTELNET).");
                return;
            }
            if (!got_sid) {
                fail_session("Handshake Failed: Did not receive [WL2K-...] SID from CMS.");
                return;
            }
            if (!challenge.empty() && pass.empty()) {
                fail_session("Winlink Password Required: Enter your account password in Winlink Configuration.");
                return;
            }

            // Exact B2F v3.2 Client Handshake Sequence (CR-delimited):
            // 1) ;FW: <CALL>
            // 2) [Pat-0.16.0-B2FHM$]
            // 3) ;PR: <8-digit secure challenge hash>
            // 4) ; <REMOTE_CALL> DE <CALL> (<GRID>)
            send_cr(";FW: " + call);
            send_cr("[Pat-0.16.0-B2FHM$]");
            if (!challenge.empty()) {
                std::string pr = calculate_winlink_secure_login_response(challenge, pass);
                log_session("Authenticating challenge PQ: " + challenge + " -> PR: " + pr);
                send_cr(";PR: " + pr);
            }
            send_cr("; " + gw_call + " DE " + call + " (" + grid + ")");
        } else {
            // ARDOP HF Radio Transport
            session_progress = 0.35f;
            send_cr("INITIALIZE");
            read_banner_line();
            send_cr("MYCALL " + call);
            read_banner_line();
            send_cr("GRIDSQUARE " + grid);
            read_banner_line();
            send_cr("ARQBW 1000MAX");
            read_banner_line();
            if (gw_freq > 0) {
                log_session("Tuned Xiegu G90 CAT to " + hamshack::FlrigCatEngine::format_freq_mhz(gw_freq) + " MHz USB");
            }
            log_session("Initiating ARDOP ARQ Call to Gateway " + gw_call + " (CAT PTT Active)...");
            send_cr("ARQCALL " + gw_call + " 10");

            bool ardop_connected = false;
            std::string challenge;
            while (!session_abort_req && hamshack::hs_sock_valid(active_sock_fd)) {
                std::string l = read_banner_line();
                if (l.empty()) break;
                if (l.find("CONNECTED") != std::string::npos) {
                    ardop_connected = true;
                    log_session("ARDOP Radio Link Established with Gateway " + gw_call + "!");
                }
                if (l.find(";PQ:") != std::string::npos) {
                    size_t p = l.find(";PQ:");
                    std::string rem = trim_ws(l.substr(p + 4));
                    size_t sp = rem.find_first_of(" \t\r\n");
                    challenge = (sp != std::string::npos) ? rem.substr(0, sp) : rem;
                }
                if (l.find("[WL2K-") != std::string::npos || l.find("]>") != std::string::npos || l.find("CMS>") != std::string::npos) {
                    if (challenge.empty()) {
                        // Peek briefly or proceed if gateway has no PQ
                    }
                    if (l.find("CMS>") != std::string::npos || l.find("]>") != std::string::npos) break;
                }
                if (l.find("DISC") != std::string::npos || l.find("FAILURE") != std::string::npos) break;
            }
            if (!ardop_connected) {
                fail_session("ARDOP Link Failed: No response from " + gw_call);
                return;
            }
            send_cr(";FW: " + call);
            send_cr("[Pat-0.16.0-B2FHM$]");
            if (!challenge.empty() && !pass.empty()) {
                std::string pr = calculate_winlink_secure_login_response(challenge, pass);
                send_cr(";PR: " + pr);
            }
            send_cr("; " + gw_call + " DE " + call + " (" + grid + ")");
        }

        // ====================================================================
        // 2. Full B2F v3.2 Bidirectional Forwarding Loop (LZHUF + Proposals)
        // ====================================================================
        struct PreparedOutbound {
            WinlinkMessage msg;
            std::vector<uint8_t> wire;
            std::vector<uint8_t> comp;
            bool handled = false;
        };
        std::vector<PreparedOutbound> out_queue;
        {
            std::lock_guard<std::recursive_mutex> lk(mtx);
            for (const auto& m : outbox) {
                PreparedOutbound po;
                po.msg = m;
                po.wire = serialize_b2f_wire_message(m, call);
                po.comp = B2Lzhuf::compress(po.wire.data(), po.wire.size());
                out_queue.push_back(std::move(po));
            }
        }

        auto mark_outbound_delivered = [&](const WinlinkMessage& m) {
            std::lock_guard<std::recursive_mutex> lk(mtx);
            std::string old_p = get_mail_dir() + "/outbox/" + m.mid + ".b2f";
            std::error_code ec;
            std::filesystem::remove(old_p, ec);
            for (auto it = outbox.begin(); it != outbox.end(); ++it) {
                if (it->mid == m.mid) {
                    outbox.erase(it);
                    break;
                }
            }
            save_message_to_disk("sent", m);
            sent.insert(sent.begin(), m);
        };

        struct InboundProposal {
            std::string type;
            std::string mid;
            size_t uncomp_size = 0;
            size_t comp_size = 0;
        };

        bool my_turn = true;          // Client initiates first turn after handshake
        bool remote_has_no_msgs = false;
        bool clean_quit = false;
        int safety_turns = 0;

        while (!session_abort_req && hamshack::hs_sock_valid(active_sock_fd) && safety_turns++ < 64) {
            if (my_turn) {
                std::vector<size_t> batch_indices;
                for (size_t i = 0; i < out_queue.size() && batch_indices.size() < 5; ++i) {
                    if (!out_queue[i].handled) batch_indices.push_back(i);
                }

                if (!batch_indices.empty()) {
                    session_progress = 0.55f;
                    session_status = "Sending " + std::to_string(batch_indices.size()) + " outgoing message(s)...";

                    int prop_csum = 0;
                    for (size_t idx : batch_indices) {
                        const auto& po = out_queue[idx];
                        std::string prop = "FC EM " + sanitize_ascii_header(po.msg.mid, 12) + " " +
                                           std::to_string(po.wire.size()) + " " +
                                           std::to_string(po.comp.size()) + " 0";
                        std::string prop_cr = prop + "\r";
                        for (unsigned char ch : prop_cr) prop_csum = (prop_csum + ch) & 0xFF;
                        if (!send_cr(prop)) {
                            fail_session("Connection lost while proposing outbound mail.");
                            return;
                        }
                    }
                    prop_csum = (-prop_csum) & 0xFF;
                    char fchk[16];
                    std::snprintf(fchk, sizeof(fchk), "F> %02X", prop_csum);
                    if (!send_cr(fchk)) {
                        fail_session("Connection lost sending proposal checksum.");
                        return;
                    }

                    // Wait for CMS FS response (or *** Login failed / error line!)
                    std::string fs_line;
                    while (true) {
                        if (!read_b2f_line(fs_line)) {
                            fail_session("CMS disconnected without answering proposal (check Winlink password).");
                            return;
                        }
                        if (fs_line.empty()) continue;
                        if (fs_line.rfind("***", 0) == 0) {
                            if (fs_line.find("Login failed") != std::string::npos ||
                                fs_line.find("Password") != std::string::npos) {
                                fail_session("Authentication Failed: " + fs_line);
                            } else {
                                fail_session("Winlink CMS Error: " + fs_line);
                            }
                            return;
                        }
                        if (fs_line[0] == ';') continue; // Informational comment e.g. ;PM:
                        if (fs_line.rfind("FS", 0) == 0) break;
                        if (fs_line == "FQ") {
                            fail_session("CMS terminated session before accepting message.");
                            return;
                        }
                    }

                    // Parse FS answers (e.g. "FS +" or "FS Y" or "FS +-")
                    std::string ans_str = trim_ws(fs_line.substr(2));
                    std::vector<size_t> accepted_indices;
                    for (size_t k = 0; k < batch_indices.size(); ++k) {
                        size_t q_idx = batch_indices[k];
                        char code = (k < ans_str.size()) ? ans_str[k] : '+';
                        if (code == 'Y' || code == 'y' || code == '+') {
                            accepted_indices.push_back(q_idx);
                        } else if (code == 'N' || code == 'n' || code == 'R' || code == 'r' || code == '-') {
                            log_session("CMS already holds or rejected MID " + out_queue[q_idx].msg.mid + " (marked handled).");
                            out_queue[q_idx].handled = true;
                            mark_outbound_delivered(out_queue[q_idx].msg);
                        } else {
                            log_session("CMS deferred MID " + out_queue[q_idx].msg.mid + " for later.");
                            out_queue[q_idx].handled = true;
                        }
                    }

                    for (size_t q_idx : accepted_indices) {
                        auto& po = out_queue[q_idx];
                        log_session("Uploading LZHUF message " + po.msg.mid + " (" +
                                    std::to_string(po.comp.size()) + " compressed / " +
                                    std::to_string(po.wire.size()) + " raw bytes)...");
                        if (!send_b2f_compressed_stream(po.msg.subject, po.comp)) {
                            fail_session("Socket error during binary LZHUF upload of " + po.msg.mid);
                            return;
                        }
                    }

                    if (!accepted_indices.empty()) {
                        // Peek next byte from CMS: 'F' or ';' confirms receipt without error!
                        uint8_t next_b = 0;
                        if (!peek_byte(next_b)) {
                            fail_session("CMS closed connection before confirming message upload.");
                            return;
                        }
                        if (next_b == '*') {
                            std::string err_line;
                            read_b2f_line(err_line);
                            fail_session("CMS rejected uploaded message: " + err_line);
                            return;
                        }
                        for (size_t q_idx : accepted_indices) {
                            out_queue[q_idx].handled = true;
                            mark_outbound_delivered(out_queue[q_idx].msg);
                            session_tx_count++;
                            log_session("Delivered MID " + out_queue[q_idx].msg.mid + " to Winlink CMS.");
                        }
                    }
                    my_turn = false;
                } else {
                    // No more outbound messages to propose
                    session_progress = 0.75f;
                    if (remote_has_no_msgs) {
                        send_cr("FQ");
                        clean_quit = true;
                        break;
                    } else {
                        session_status = "Checking for incoming Winlink mail...";
                        if (!send_cr("FF")) {
                            fail_session("Connection lost sending FF turn-over.");
                            return;
                        }
                        my_turn = false;
                    }
                }
            } else {
                // Remote CMS's turn to send proposals, FF, or FQ
                std::vector<InboundProposal> in_props;
                int rx_prop_csum = 0;

                while (!session_abort_req && !my_turn) {
                    std::string line;
                    if (!read_b2f_line(line)) {
                        fail_session("CMS disconnected unexpectedly during login/receive (check Winlink password).");
                        return;
                    }
                    if (line.empty()) continue;

                    if (line.rfind("***", 0) == 0) {
                        if (line.find("Login failed") != std::string::npos ||
                            line.find("Password") != std::string::npos) {
                            fail_session("Authentication Failed: " + line);
                        } else {
                            fail_session("Winlink CMS Error: " + line);
                        }
                        return;
                    }

                    if (line[0] == ';') {
                        continue; // e.g. ;PM: W1AW ... pending messages notice
                    }

                    if (line.rfind("FC ", 0) == 0 || line.rfind("FD ", 0) == 0) {
                        std::string line_cr = line + "\r";
                        for (unsigned char ch : line_cr) rx_prop_csum = (rx_prop_csum + ch) & 0xFF;

                        std::stringstream pss(line.substr(3));
                        InboundProposal ip;
                        pss >> ip.type >> ip.mid >> ip.uncomp_size >> ip.comp_size;
                        if (!ip.mid.empty() && ip.comp_size > 0) {
                            in_props.push_back(std::move(ip));
                        }
                        continue;
                    }

                    if (line == "FF") {
                        remote_has_no_msgs = true;
                        bool any_unsent = false;
                        for (const auto& po : out_queue) {
                            if (!po.handled) { any_unsent = true; break; }
                        }
                        if (!any_unsent) {
                            send_cr("FQ");
                            clean_quit = true;
                        } else {
                            my_turn = true;
                        }
                        break;
                    }

                    if (line == "FQ") {
                        clean_quit = true;
                        break;
                    }

                    if (line.rfind("F>", 0) == 0) {
                        std::string hex_part = trim_ws(line.substr(2));
                        if (hex_part.size() >= 2) {
                            int their_chk = static_cast<int>(std::strtol(hex_part.c_str(), nullptr, 16));
                            int calc_chk = (-rx_prop_csum) & 0xFF;
                            if (their_chk != calc_chk) {
                                fail_session("Inbound proposal checksum mismatch from remote node.");
                                return;
                            }
                        }
                        rx_prop_csum = 0;

                        std::string fs_reply;
                        std::vector<InboundProposal> accepted_in;
                        {
                            std::lock_guard<std::recursive_mutex> lk(mtx);
                            for (const auto& ip : in_props) {
                                if (has_message_mid_locked(ip.mid)) {
                                    fs_reply.push_back('-');
                                } else {
                                    fs_reply.push_back('+');
                                    accepted_in.push_back(ip);
                                }
                            }
                        }
                        if (!send_cr("FS " + fs_reply)) {
                            fail_session("Socket error sending FS proposal reply.");
                            return;
                        }

                        session_progress = 0.85f;
                        for (const auto& ip : accepted_in) {
                            session_status = "Downloading message " + ip.mid + " (" + std::to_string(ip.comp_size) + " B)...";
                            std::vector<uint8_t> comp_bytes;
                            std::string stream_err;
                            if (!read_b2f_compressed_stream(comp_bytes, stream_err)) {
                                fail_session("Download error on " + ip.mid + ": " + stream_err);
                                return;
                            }
                            std::vector<uint8_t> raw_msg;
                            std::string lzh_err;
                            if (!B2Lzhuf::decompress(comp_bytes.data(), comp_bytes.size(), raw_msg, &lzh_err)) {
                                fail_session("LZHUF decompression failed on " + ip.mid + ": " + lzh_err);
                                return;
                            }
                            WinlinkMessage in_msg;
                            if (!parse_b2f_wire_message(raw_msg.data(), raw_msg.size(), in_msg)) {
                                in_msg.mid = ip.mid;
                                in_msg.date_str = format_current_b2f_date();
                                in_msg.from = gw_call;
                                in_msg.to = call;
                                in_msg.subject = "Winlink Message " + ip.mid;
                                in_msg.body.assign(reinterpret_cast<const char*>(raw_msg.data()), raw_msg.size());
                                in_msg.total_bytes = raw_msg.size();
                            }
                            in_msg.is_read = false;
                            save_message_to_disk("inbox", in_msg);
                            {
                                std::lock_guard<std::recursive_mutex> lk(mtx);
                                inbox.insert(inbox.begin(), in_msg);
                            }
                            session_rx_count++;
                            log_session("Received & decompressed MID " + in_msg.mid + " (" + in_msg.subject + ")");
                        }
                        my_turn = true;
                        break;
                    }
                }

                if (clean_quit) break;
            }
        }

        // ====================================================================
        // 3. Clean Session Teardown
        // ====================================================================
        if (hamshack::hs_sock_valid(active_sock_fd)) {
            hamshack::hs_close_socket(active_sock_fd);
            active_sock_fd = hamshack::HS_INVALID_SOCKET;
        }
        if (!is_telnet && ardop_modem.is_running()) {
            log_session("[ARDOP] Radio session completed. Stopping ARDOP modem and releasing CAT PTT.");
            ardop_modem.stop();
        }

        session_progress = 1.0f;
        session_status = "Session Complete: " + std::to_string(session_tx_count) + " sent, " +
                         std::to_string(session_rx_count) + " received.";
        log_session("*** " + session_status + " ***");
        session_active = false;
    }
};

} // namespace winlink
