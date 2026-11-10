#pragma once
// ============================================================================
// JS8Call Protocol & DSP Modem Engine (Normal / Fast / Turbo / Slow)
// Ported from js8call/lib/js8/genjs8.f90, js8call/lib/crc12.cpp,
// js8call/lib/ft8/encode174.f90, and js8call/lib/ft8/bpdecode174.f90
// ============================================================================

#include <cstdint>
#include <cstring>
#include <cmath>
#include <string>
#include <vector>
#include <array>
#include <map>
#include <mutex>
#include <ctime>
#include <algorithm>
#include <sstream>
#include <iomanip>
#include <functional>
#include <atomic>
#include "psk_reporter.hpp"

namespace js8 {

enum class Submode : int {
    NORMAL = 0, // 15s period, 6.25 baud, 50 Hz BW, ~16 WPM
    FAST   = 1, // 10s period, 10.0 baud, 80 Hz BW, ~24 WPM
    TURBO  = 2, //  6s period, 20.0 baud, 160 Hz BW, ~40 WPM
    SLOW   = 3  // 30s period, 3.125 baud, 25 Hz BW, ~8 WPM
};

struct SubmodeSpec {
    Submode mode;
    const char* name;
    const char* badge;
    int period_sec;
    int nsps_12k;       // Samples per symbol at 12000 Hz
    double baud;
    double bandwidth_hz;
    int wpm_approx;
    int costas_type;    // 1 = Original, 2 = Modified (JS8 0.9+)
};

static constexpr SubmodeSpec SUBMODES[4] = {
    {Submode::NORMAL, "Normal", "N", 15, 1920,  6.25,   50.0, 16, 2},
    {Submode::FAST,   "Fast",   "F", 10, 1200, 10.00,   80.0, 24, 2},
    {Submode::TURBO,  "Turbo",  "T",  6,  600, 20.00,  160.0, 40, 2},
    {Submode::SLOW,   "Slow",   "S", 30, 3840,  3.125,  25.0,  8, 2}
};

inline const SubmodeSpec& submode_params(Submode m) {
    return SUBMODES[static_cast<int>(m) & 3];
}

// 64-symbol 6-bit JS8 frame alphabet (0-9, A-Z, a-z, space, colon) + extended symbols
static const char ALPHABET_68[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz :+-/?";

inline int char_to_js8_val(char c) {
    for (int i = 0; i < 64; ++i) {
        if (ALPHABET_68[i] == c) return i;
    }
    return 62; // Space
}

// Costas 7x7 arrays from genjs8.f90
static constexpr int ICOS7_ORIG[3][7] = {
    {4, 2, 5, 6, 1, 3, 0},
    {4, 2, 5, 6, 1, 3, 0},
    {4, 2, 5, 6, 1, 3, 0}
};
static constexpr int ICOS7_MOD[3][7] = {
    {0, 6, 2, 3, 5, 4, 1},
    {1, 5, 0, 2, 3, 6, 4},
    {2, 5, 0, 6, 4, 1, 3}
};

static constexpr int GRAYMAP[8] = {0, 1, 3, 2, 6, 4, 5, 7};

// LDPC(174,87) generator matrix hex rows from ldpc_174_87_params.f90
static const char* LDPC_174_87_GEN_HEX[87] = {
    "23bba830e23b6b6f50982e", "1f8e55da218c5df3309052", "ca7b3217cd92bd59a5ae20",
    "56f78313537d0f4382964e", "29c29dba9c545e267762fe", "6be396b5e2e819e373340c",
    "293548a138858328af4210", "cb6c6afcdc28bb3f7c6e86", "3f2a86f5c5bd225c961150",
    "849dd2d63673481860f62c", "56cdaec6e7ae14b43feeee", "04ef5cfa3766ba778f45a4",
    "c525ae4bd4f627320a3974", "fe37802941d66dde02b99c", "41fd9520b2e4abeb2f989c",
    "40907b01280f03c0323946", "7fb36c24085a34d8c1dbc4", "40fc3e44bb7d2bb2756e44",
    "d38ab0a1d2e52a8ec3bc76", "3d0f929ef3949bd84d4734", "45d3814f504064f80549ae",
    "f14dbf263825d0bd04b05e", "f08a91fb2e1f78290619a8", "7a8dec79a51e8ac5388022",
    "ca4186dd44c3121565cf5c", "db714f8f64e8ac7af1a76e", "8d0274de71e7c1a8055eb0",
    "51f81573dd4049b082de14", "d037db825175d851f3af00", "d8f937f31822e57c562370",
    "1bf1490607c54032660ede", "1616d78018d0b4745ca0f2", "a9fa8e50bcb032c85e3304",
    "83f640f1a48a8ebc0443ea", "eca9afa0f6b01d92305edc", "3776af54ccfbae916afde6",
    "6abb212d9739dfc02580f2", "05209a0abb530b9e7e34b0", "612f63acc025b6ab476f7c",
    "0af7723161ec223080be86", "a8fc906976c35669e79ce0", "45b7ab6242b77474d9f11a",
    "b274db8abd3c6f396ea356", "9059dfa2bb20ef7ef73ad4", "3d188ea477f6fa41317a4e",
    "8d9071b7e7a6a2eed6965e", "a377253773ea678367c3f6", "ecbd7c73b9cd34c3720c8a",
    "b6537f417e61d1a7085336", "6c280d2a0523d9c4bc5946", "d36d662a69ae24b74dcbd8",
    "d747bfc5fd65ef70fbd9bc", "a9fa2eefa6f8796a355772", "cc9da55fe046d0cb3a770c",
    "f6ad4824b87c80ebfce466", "cc6de59755420925f90ed2", "164cc861bdd803c547f2ac",
    "c0fc3ec4fb7d2bb2756644", "0dbd816fba1543f721dc72", "a0c0033a52ab6299802fd2",
    "bf4f56e073271f6ab4bf80", "57da6d13cb96a7689b2790", "81cfc6f18c35b1e1f17114",
    "481a2a0df8a23583f82d6c", "1ac4672b549cd6dba79bcc", "c87af9a5d5206abca532a8",
    "97d4169cb33e7435718d90", "a6573f3dc8b16c9d19f746", "2c4142bf42b01e71076acc",
    "081c29a10d468ccdbcecb6", "5b0f7742bca86b8012609a", "012dee2198eba82b19a1da",
    "f1627701a2d692fd9449e6", "35ad3fb0faeb5f1b0c30dc", "b1ca4ea2e3d173bad4379c",
    "37d8e0af9258b9e8c5f9b2", "cd921fdf59e882683763f6", "6114e08483043fd3f38a8a",
    "2e547dd7a05f6597aac516", "95e45ecd0135aca9d6e6ae", "b33ec97be83ce413f9acc8",
    "c8b5dffc335095dcdcaf2a", "3dd01a59d86310743ec752", "14cd0f642fc0c5fe3a65ca",
    "3a0a1dfd7eee29c2e827e0", "8abdb889efbe39a510a118", "3f231f212055371cf3e2a2"
};

static constexpr int LDPC_174_87_COLORDER[174] = {
      0,  1,  2,  3, 30,  4,  5,  6,  7,  8,  9, 10, 11, 32, 12, 40, 13, 14, 15, 16,
     17, 18, 37, 45, 29, 19, 20, 21, 41, 22, 42, 31, 33, 34, 44, 35, 47, 51, 50, 43,
     36, 52, 63, 46, 25, 55, 27, 24, 23, 53, 39, 49, 59, 38, 48, 61, 60, 57, 28, 62,
     56, 58, 65, 66, 26, 70, 64, 69, 68, 67, 74, 71, 54, 76, 72, 75, 78, 77, 80, 79,
     73, 83, 84, 81, 82, 85, 86, 87, 88, 89, 90, 91, 92, 93, 94, 95, 96, 97, 98, 99,
    100,101,102,103,104,105,106,107,108,109,110,111,112,113,114,115,116,117,118,119,
    120,121,122,123,124,125,126,127,128,129,130,131,132,133,134,135,136,137,138,139,
    140,141,142,143,144,145,146,147,148,149,150,151,152,153,154,155,156,157,158,159,
    160,161,162,163,164,165,166,167,168,169,170,171,172,173
};

static constexpr int LDPC_174_87_MN[174][3] = {
    { 1, 25, 69},{ 2,  5, 73},{ 3, 32, 68},{ 4, 51, 61},{ 6, 63, 70},{ 7, 33, 79},
    { 8, 50, 86},{ 9, 37, 43},{10, 41, 65},{11, 14, 64},{12, 75, 77},{13, 23, 81},
    {15, 16, 82},{17, 56, 66},{18, 53, 60},{19, 31, 52},{20, 67, 84},{21, 29, 72},
    {22, 24, 44},{26, 35, 76},{27, 36, 38},{28, 40, 42},{30, 54, 55},{34, 49, 87},
    {39, 57, 58},{45, 74, 83},{46, 62, 80},{47, 48, 85},{59, 71, 78},{ 1, 50, 53},
    { 2, 47, 84},{ 3, 25, 79},{ 4,  6, 14},{ 5,  7, 80},{ 8, 34, 55},{ 9, 36, 69},
    {10, 43, 83},{11, 23, 74},{12, 17, 44},{13, 57, 76},{15, 27, 56},{16, 28, 29},
    {18, 19, 59},{20, 40, 63},{21, 35, 52},{22, 54, 64},{24, 62, 78},{26, 32, 77},
    {30, 72, 85},{31, 65, 87},{33, 39, 51},{37, 48, 75},{38, 70, 71},{41, 42, 68},
    {45, 67, 86},{46, 81, 82},{49, 66, 73},{58, 60, 66},{61, 65, 85},{ 1, 14, 21},
    { 2, 13, 59},{ 3, 67, 82},{ 4, 32, 73},{ 5, 36, 54},{ 6, 43, 46},{ 7, 28, 75},
    { 8, 33, 71},{ 9, 49, 76},{10, 58, 64},{11, 48, 68},{12, 19, 45},{15, 50, 61},
    {16, 22, 26},{17, 72, 80},{18, 40, 55},{20, 35, 51},{23, 25, 34},{24, 63, 87},
    {27, 39, 74},{29, 78, 83},{30, 70, 77},{31, 69, 84},{22, 37, 86},{38, 41, 81},
    {42, 44, 57},{47, 53, 62},{52, 56, 79},{60, 75, 81},{ 1, 39, 77},{ 2, 16, 41},
    { 3, 31, 54},{ 4, 36, 78},{ 5, 45, 65},{ 6, 57, 85},{ 7, 14, 49},{ 8, 21, 46},
    { 9, 15, 72},{10, 20, 62},{11, 17, 71},{12, 34, 47},{13, 68, 86},{18, 23, 43},
    {19, 64, 73},{24, 48, 79},{25, 70, 83},{26, 80, 87},{27, 32, 40},{28, 56, 69},
    {29, 63, 66},{30, 42, 50},{33, 37, 82},{35, 60, 74},{38, 55, 84},{44, 52, 61},
    {51, 53, 72},{58, 59, 67},{47, 56, 76},{ 1, 19, 37},{ 2, 61, 75},{ 3,  8, 66},
    { 4, 60, 84},{ 5, 34, 39},{ 6, 26, 53},{ 7, 32, 57},{ 9, 52, 67},{10, 12, 15},
    {11, 51, 69},{13, 14, 65},{16, 31, 43},{17, 20, 36},{18, 80, 86},{21, 48, 59},
    {22, 40, 46},{23, 33, 62},{24, 30, 74},{25, 42, 64},{27, 49, 85},{28, 38, 73},
    {29, 44, 81},{35, 68, 70},{41, 63, 76},{45, 49, 71},{50, 58, 87},{48, 54, 83},
    {13, 55, 79},{77, 78, 82},{ 1,  2, 24},{ 3,  6, 75},{ 4, 56, 87},{ 5, 44, 53},
    { 7, 50, 83},{ 8, 10, 28},{ 9, 55, 62},{11, 29, 67},{12, 33, 40},{14, 16, 20},
    {15, 35, 73},{17, 31, 39},{18, 36, 57},{19, 46, 76},{21, 42, 84},{22, 34, 59},
    {23, 26, 61},{25, 60, 65},{27, 64, 80},{30, 37, 66},{32, 45, 72},{38, 51, 86},
    {41, 77, 79},{43, 56, 68},{47, 74, 82},{40, 52, 78},{54, 61, 71},{46, 58, 69}
};

static constexpr int LDPC_174_87_NM[87][7] = {
    {  1, 30, 60, 89,118,147,  0},{  2, 31, 61, 90,119,147,  0},{  3, 32, 62, 91,120,148,  0},
    {  4, 33, 63, 92,121,149,  0},{  2, 34, 64, 93,122,150,  0},{  5, 33, 65, 94,123,148,  0},
    {  6, 34, 66, 95,124,151,  0},{  7, 35, 67, 96,120,152,  0},{  8, 36, 68, 97,125,153,  0},
    {  9, 37, 69, 98,126,152,  0},{ 10, 38, 70, 99,127,154,  0},{ 11, 39, 71,100,126,155,  0},
    { 12, 40, 61,101,128,145,  0},{ 10, 33, 60, 95,128,156,  0},{ 13, 41, 72, 97,126,157,  0},
    { 13, 42, 73, 90,129,156,  0},{ 14, 39, 74, 99,130,158,  0},{ 15, 43, 75,102,131,159,  0},
    { 16, 43, 71,103,118,160,  0},{ 17, 44, 76, 98,130,156,  0},{ 18, 45, 60, 96,132,161,  0},
    { 19, 46, 73, 83,133,162,  0},{ 12, 38, 77,102,134,163,  0},{ 19, 47, 78,104,135,147,  0},
    {  1, 32, 77,105,136,164,  0},{ 20, 48, 73,106,123,163,  0},{ 21, 41, 79,107,137,165,  0},
    { 22, 42, 66,108,138,152,  0},{ 18, 42, 80,109,139,154,  0},{ 23, 49, 81,110,135,166,  0},
    { 16, 50, 82, 91,129,158,  0},{  3, 48, 63,107,124,167,  0},{  6, 51, 67,111,134,155,  0},
    { 24, 35, 77,100,122,162,  0},{ 20, 45, 76,112,140,157,  0},{ 21, 36, 64, 92,130,159,  0},
    {  8, 52, 83,111,118,166,  0},{ 21, 53, 84,113,138,168,  0},{ 25, 51, 79, 89,122,158,  0},
    { 22, 44, 75,107,133,155,172},{  9, 54, 84, 90,141,169,  0},{ 22, 54, 85,110,136,161,  0},
    {  8, 37, 65,102,129,170,  0},{ 19, 39, 85,114,139,150,  0},{ 26, 55, 71, 93,142,167,  0},
    { 27, 56, 65, 96,133,160,174},{ 28, 31, 86,100,117,171,  0},{ 28, 52, 70,104,132,144,  0},
    { 24, 57, 68, 95,137,142,  0},{  7, 30, 72,110,143,151,  0},{  4, 51, 76,115,127,168,  0},
    { 16, 45, 87,114,125,172,  0},{ 15, 30, 86,115,123,150,  0},{ 23, 46, 64, 91,144,173,  0},
    { 23, 35, 75,113,145,153,  0},{ 14, 41, 87,108,117,149,170},{ 25, 40, 85, 94,124,159,  0},
    { 25, 58, 69,116,143,174,  0},{ 29, 43, 61,116,132,162,  0},{ 15, 58, 88,112,121,164,  0},
    {  4, 59, 72,114,119,163,173},{ 27, 47, 86, 98,134,153,  0},{  5, 44, 78,109,141,  0,  0},
    { 10, 46, 69,103,136,165,  0},{  9, 50, 59, 93,128,164,  0},{ 14, 57, 58,109,120,166,  0},
    { 17, 55, 62,116,125,154,  0},{  3, 54, 70,101,140,170,  0},{  1, 36, 82,108,127,174,  0},
    {  5, 53, 81,105,140,  0,  0},{ 29, 53, 67, 99,142,173,  0},{ 18, 49, 74, 97,115,167,  0},
    {  2, 57, 63,103,138,157,  0},{ 26, 38, 79,112,135,171,  0},{ 11, 52, 66, 88,119,148,  0},
    { 20, 40, 68,117,141,160,  0},{ 11, 48, 81, 89,146,169,  0},{ 29, 47, 80, 92,146,172,  0},
    {  6, 32, 87,104,145,169,  0},{ 27, 34, 74,106,131,165,  0},{ 12, 56, 84, 88,139,  0,  0},
    { 13, 56, 62,111,146,171,  0},{ 26, 37, 80,105,144,151,  0},{ 17, 31, 82,113,121,161,  0},
    { 28, 49, 59, 94,137,  0,  0},{  7, 55, 83,101,131,168,  0},{ 24, 50, 78,106,143,149,  0}
};

static constexpr int LDPC_174_87_NRW[87] = {
    6,6,6,6,6,6,6,6,6,6, 6,6,6,6,6,6,6,6,6,6, 6,6,6,6,6,6,6,6,6,6,
    6,6,6,6,6,6,6,6,6,7, 6,6,6,6,6,7,6,6,6,6, 6,6,6,6,6,7,6,6,6,6,
    7,6,5,6,6,6,6,6,6,5, 6,6,6,6,6,6,6,6,6,6, 5,6,6,6,5,6,6
};

// ----------------------------------------------------------------------------
// CRC-12 (polynomial 0xc06, XOR 42) from js8call/lib/crc12.cpp & genjs8.f90
// ----------------------------------------------------------------------------
inline uint16_t crc12(const uint8_t* data, int len) {
    uint16_t rem = 0;
    const uint16_t poly = 0x0C06; // 12-bit poly x^12 + x^11 + x^3 + x^2 + 1
    for (int i = 0; i < len; ++i) {
        uint8_t b = data[i];
        for (int bit = 7; bit >= 0; --bit) {
            uint8_t in_bit = (b >> bit) & 1;
            uint8_t top = (rem >> 11) & 1;
            rem = ((rem << 1) & 0x0FFF) | in_bit;
            if (top) rem ^= poly;
        }
    }
    for (int bit = 0; bit < 12; ++bit) {
        uint8_t top = (rem >> 11) & 1;
        rem = (rem << 1) & 0x0FFF;
        if (top) rem ^= poly;
    }
    return (rem & 0x0FFF) ^ 42;
}

inline int hex_nibble(char c) {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    return 0;
}

// ----------------------------------------------------------------------------
// LDPC(174,87) Systematic Encoder (from js8call/lib/ft8/encode174.f90)
// ----------------------------------------------------------------------------
inline void encode174(const uint8_t msg87[87], uint8_t codeword174[174]) {
    uint8_t pchecks[87] = {0};
    for (int i = 0; i < 87; ++i) {
        const char* row_hex = LDPC_174_87_GEN_HEX[i];
        int sum = 0;
        for (int j = 0; j < 87; ++j) {
            int nib_idx = j / 4;
            int bit_idx = 3 - (j % 4);
            int g_ij = (hex_nibble(row_hex[nib_idx]) >> bit_idx) & 1;
            sum += (msg87[j] & 1) * g_ij;
        }
        pchecks[i] = (uint8_t)(sum & 1);
    }
    uint8_t itmp[174];
    for (int i = 0; i < 87; ++i) {
        itmp[i] = pchecks[i];
        itmp[87 + i] = msg87[i] & 1;
    }
    for (int i = 0; i < 174; ++i) {
        codeword174[LDPC_174_87_COLORDER[i]] = itmp[i];
    }
}

// ----------------------------------------------------------------------------
// LDPC(174,87) Belief-Propagation Decoder (from js8call/lib/ft8/bpdecode174.f90)
// ----------------------------------------------------------------------------
inline bool bpdecode174(const float llr[174], int max_iterations, uint8_t decoded87[87], uint8_t cw174[174]) {
    constexpr int N = 174, K = 87, M = 87;
    float toc[7][M] = {{0}};
    float tov[3][N] = {{0}};
    float tanhtoc[7][M] = {{0}};
    float zn[N] = {0};

    for (int j = 0; j < M; ++j) {
        for (int i = 0; i < LDPC_174_87_NRW[j]; ++i) {
            toc[i][j] = llr[LDPC_174_87_NM[j][i] - 1];
        }
    }

    int ncnt = 0;
    int nclast = 0;
    for (int iter = 0; iter <= max_iterations; ++iter) {
        for (int i = 0; i < N; ++i) {
            zn[i] = llr[i] + tov[0][i] + tov[1][i] + tov[2][i];
            cw174[i] = (zn[i] > 0.0f) ? 1 : 0;
        }

        int ncheck = 0;
        for (int j = 0; j < M; ++j) {
            int synd = 0;
            for (int i = 0; i < LDPC_174_87_NRW[j]; ++i) {
                synd += cw174[LDPC_174_87_NM[j][i] - 1];
            }
            if ((synd & 1) != 0) ncheck++;
        }

        if (ncheck == 0) {
            uint8_t reordered[N];
            for (int i = 0; i < N; ++i) reordered[i] = cw174[LDPC_174_87_COLORDER[i]];
            for (int i = 0; i < K; ++i) decoded87[i] = reordered[M + i];
            return true;
        }

        if (iter > 0) {
            int nd = ncheck - nclast;
            if (nd < 0) ncnt = 0; else ncnt++;
            if (ncnt >= 5 && iter >= 10 && ncheck > 15) return false;
        }
        nclast = ncheck;

        for (int j = 0; j < M; ++j) {
            for (int i = 0; i < LDPC_174_87_NRW[j]; ++i) {
                int bit_idx = LDPC_174_87_NM[j][i] - 1;
                toc[i][j] = zn[bit_idx];
                for (int kk = 0; kk < 3; ++kk) {
                    if (LDPC_174_87_MN[bit_idx][kk] - 1 == j) {
                        toc[i][j] -= tov[kk][bit_idx];
                    }
                }
                tanhtoc[i][j] = std::tanh(-toc[i][j] * 0.5f);
            }
        }

        for (int i = 0; i < N; ++i) {
            for (int k = 0; k < 3; ++k) {
                int chk_idx = LDPC_174_87_MN[i][k] - 1;
                float prod = 1.0f;
                for (int m = 0; m < LDPC_174_87_NRW[chk_idx]; ++m) {
                    if (LDPC_174_87_NM[chk_idx][m] - 1 != i) {
                        prod *= tanhtoc[m][chk_idx];
                    }
                }
                prod = std::max(-0.999999f, std::min(0.999999f, prod));
                tov[k][i] = -2.0f * std::atanh(prod);
            }
        }
    }
    return false;
}

// ----------------------------------------------------------------------------
// JS8 12-Character Frame Encoder & Decoder (from js8call/lib/js8/genjs8.f90)
// ----------------------------------------------------------------------------
inline void encode_js8_frame(const std::string& msg12_in, uint8_t frame_flags3, int costas_type,
                             uint8_t tones79[79], uint8_t msgbits87_out[87] = nullptr) {
    std::string msg = msg12_in;
    while (msg.size() < 12) msg.push_back(' ');
    if (msg.size() > 12) msg.resize(12);

    uint8_t msgbits[87] = {0};
    // 12 characters * 6 bits = 72 bits
    int k = 0;
    for (int j = 0; j < 12; ++j) {
        int val = char_to_js8_val(msg[j]) & 0x3F;
        for (int b = 5; b >= 0; --b) {
            msgbits[k++] = (val >> b) & 1;
        }
    }
    // 3 frame flag bits (72..74): bit0=First, bit1=Last, bit2=Data/Directed
    msgbits[k++] = (frame_flags3 >> 2) & 1;
    msgbits[k++] = (frame_flags3 >> 1) & 1;
    msgbits[k++] = (frame_flags3 >> 0) & 1;

    // Pack 75 bits into 11 bytes (88 bits padded with 0) for CRC12
    uint8_t i1msg[11] = {0};
    for (int i = 0; i < 75; ++i) {
        if (msgbits[i]) i1msg[i / 8] |= (1 << (7 - (i % 8)));
    }
    uint16_t crc = crc12(i1msg, 11);
    for (int b = 11; b >= 0; --b) {
        msgbits[75 + (11 - b)] = (crc >> b) & 1;
    }
    if (msgbits87_out) std::memcpy(msgbits87_out, msgbits, 87);

    uint8_t cw174[174] = {0};
    encode174(msgbits, cw174);

    const auto& icos = (costas_type == 1) ? ICOS7_ORIG : ICOS7_MOD;
    for (int i = 0; i < 7; ++i) {
        tones79[i]      = (uint8_t)icos[0][i];
        tones79[36 + i] = (uint8_t)icos[1][i];
        tones79[72 + i] = (uint8_t)icos[2][i];
    }
    int sym_k = 6;
    for (int j = 0; j < 58; ++j) {
        int idx = j * 3;
        sym_k++;
        if (j == 29) sym_k += 7;
        int g = (cw174[idx] << 2) | (cw174[idx + 1] << 1) | cw174[idx + 2];
        tones79[sym_k] = (uint8_t)GRAYMAP[g & 7];
    }
}

inline bool decode_js8_bits87(const uint8_t msgbits[87], std::string& out_msg12, uint8_t& out_flags3) {
    uint8_t i1msg[11] = {0};
    for (int i = 0; i < 75; ++i) {
        if (msgbits[i]) i1msg[i / 8] |= (1 << (7 - (i % 8)));
    }
    uint16_t calc_crc = crc12(i1msg, 11);
    uint16_t rx_crc = 0;
    for (int i = 0; i < 12; ++i) {
        rx_crc = (rx_crc << 1) | (msgbits[75 + i] & 1);
    }
    if (calc_crc != rx_crc) return false;

    out_msg12.clear();
    for (int j = 0; j < 12; ++j) {
        int val = 0;
        for (int b = 0; b < 6; ++b) {
            val = (val << 1) | (msgbits[j * 6 + b] & 1);
        }
        out_msg12.push_back(ALPHABET_68[val % 68]);
    }
    out_flags3 = ((msgbits[72] & 1) << 2) | ((msgbits[73] & 1) << 1) | (msgbits[74] & 1);
    return true;
}

inline bool decode_js8_soft(const float llr[174], std::string& out_msg12, uint8_t& out_flags3) {
    uint8_t dec87[87] = {0};
    uint8_t cw_out[174] = {0};
    if (!bpdecode174(llr, 35, dec87, cw_out)) return false;
    return decode_js8_bits87(dec87, out_msg12, out_flags3);
}

// Synthesize 12,000 Hz audio waveform for a 79-symbol JS8 frame
inline std::vector<float> synthesize_js8_wave(const uint8_t tones79[79], Submode submode, double audio_freq_hz, float amp = 0.45f) {
    const auto& spec = SUBMODES[(int)submode];
    int nsps = spec.nsps_12k;
    int total_samples = 79 * nsps;
    std::vector<float> wave(total_samples, 0.0f);
    double fs = 12000.0;
    double spacing_hz = fs / (double)nsps;
    double phase = 0.0;

    for (int s = 0; s < 79; ++s) {
        double f = audio_freq_hz + tones79[s] * spacing_hz;
        double dphi = 2.0 * M_PI * f / fs;
        for (int k = 0; k < nsps; ++k) {
            int idx = s * nsps + k;
            float env = 1.0f;
            int ramp = nsps / 8;
            if (s == 0 && k < ramp) env = 0.5f * (1.0f - std::cos(M_PI * k / ramp));
            if (s == 78 && k > nsps - ramp) env = 0.5f * (1.0f - std::cos(M_PI * (nsps - k) / ramp));
            wave[idx] = amp * env * (float)std::sin(phase);
            phase += dphi;
            if (phase > 2.0 * M_PI) phase -= 2.0 * M_PI;
        }
    }
    return wave;
}

// ----------------------------------------------------------------------------
// JS8Call Directed Protocol, Heard List, Inbox & Multi-Frame Controller
// ----------------------------------------------------------------------------
struct JS8FrameLog {
    std::string utc_time;
    int snr_db;
    float dt_sec;
    int offset_hz;
    Submode submode;
    uint8_t flags;       // bit2=First, bit1=Last, bit0=Directed
    std::string text;
    bool is_tx;
};

struct JS8HeardStation {
    std::string callsign;
    std::string grid;
    int snr_db;
    int offset_hz;
    Submode submode;
    std::string last_utc;
    std::string info;
    int frame_count;
};

struct JS8InboxMessage {
    int id;
    std::string from_call;
    std::string to_call;
    std::string utc_time;
    std::string body;
    bool unread;
    bool bridged_to_mesh;
};

class JS8ModemEngine {
public:
    Submode current_submode = Submode::NORMAL;
    int tx_offset_hz = 1500;
    int rx_offset_hz = 1500;

    std::string my_call = "W1AW";
    std::string my_grid = "FN31";
    std::string my_info = "XIEGU G90 20W + DIPOLE + MESHCORE BLE BRIDGE";
    std::string my_status = "QRV JS8CALL + HF/LORA MESH GATEWAY";
    std::string selected_call = "@ALLCALL";

    bool heartbeat_enabled = true;
    bool auto_reply_enabled = true;
    bool bridge_to_meshcore = true;
    bool tx_active = false;
    std::atomic<uint64_t> cached_dial_hz{7078000ULL};

    std::vector<JS8FrameLog> band_frames;
    std::vector<std::string> qso_transcript;
    std::vector<JS8HeardStation> heard_stations;
    std::vector<JS8InboxMessage> inbox;
    std::vector<std::string> tx_frame_queue;
    std::string composer_text;

    // Optional callback to forward JS8 messages into MeshCore BLE/LoRa mesh!
    std::function<void(const std::string& from, const std::string& text)> on_mesh_bridge_forward;

    mutable std::mutex mtx;

    JS8ModemEngine() = default;

    static std::string current_utc_str() {
        std::time_t now = std::time(nullptr);
        std::tm g{};
        hamshack::hs_gmtime(&now, &g);
        char buf[16];
        std::snprintf(buf, sizeof(buf), "%02d:%02d:%02d", g.tm_hour, g.tm_min, g.tm_sec);
        return buf;
    }

    // Split arbitrary text into 12-character JS8 frames and queue for TX
    void queue_message(const std::string& full_msg, std::vector<float>* out_audio_samples = nullptr) {
        std::lock_guard<std::mutex> lk(mtx);
        std::string clean = full_msg;
        for (char& c : clean) {
            if (c >= 'a' && c <= 'z') c = (char)(c - 'a' + 'A');
        }
        if (clean.empty()) return;

        std::string utc = current_utc_str();
        qso_transcript.push_back("[" + utc + " TX] " + clean);

        int total_frames = ((int)clean.size() + 11) / 12;
        for (int f = 0; f < total_frames; ++f) {
            std::string chunk = clean.substr(f * 12, 12);
            while (chunk.size() < 12) chunk.push_back(' ');
            uint8_t flags = 0;
            if (f == 0) flags |= 0x04;
            if (f == total_frames - 1) flags |= 0x02;
            tx_frame_queue.push_back(chunk);

            JS8FrameLog fl;
            fl.utc_time = utc;
            fl.snr_db = 0;
            fl.dt_sec = 0.0f;
            fl.offset_hz = tx_offset_hz;
            fl.submode = current_submode;
            fl.flags = flags;
            fl.text = chunk;
            fl.is_tx = true;
            band_frames.push_back(fl);

            if (out_audio_samples) {
                uint8_t tones[79];
                encode_js8_frame(chunk, flags, 2, tones);
                auto wave = synthesize_js8_wave(tones, current_submode, (double)tx_offset_hz);
                out_audio_samples->insert(out_audio_samples->end(), wave.begin(), wave.end());
            }
        }
        tx_active = true;
    }

    // Directed command builders matching JS8Call UI buttons
    void send_directed_command(const std::string& cmd, std::vector<float>* out_audio = nullptr) {
        std::string target = selected_call.empty() ? "@ALLCALL" : selected_call;
        std::string msg;
        if (cmd == "CQ") {
            msg = my_call + ": @ALLCALL CQ CQ " + my_grid;
        } else if (cmd == "HB") {
            msg = my_call + ": @HB HEARTBEAT " + my_grid;
        } else if (cmd == "SNR?") {
            msg = my_call + ": " + target + " SNR?";
        } else if (cmd == "INFO?") {
            msg = my_call + ": " + target + " INFO?";
        } else if (cmd == "GRID?") {
            msg = my_call + ": " + target + " GRID?";
        } else if (cmd == "STATUS?") {
            msg = my_call + ": " + target + " STATUS?";
        } else if (cmd == "HEARING?") {
            msg = my_call + ": " + target + " HEARING?";
        } else if (cmd == "AGM" || cmd == "ACK") {
            int snr = -6;
            for (const auto& h : heard_stations) if (h.callsign == target) snr = h.snr_db;
            char sbuf[16];
            std::snprintf(sbuf, sizeof(sbuf), "%+03d", snr);
            msg = my_call + ": " + target + " ACK " + sbuf;
        } else if (cmd == "73") {
            msg = my_call + ": " + target + " RR 73 GL";
        } else {
            msg = my_call + ": " + target + " " + cmd;
        }
        queue_message(msg, out_audio);
    }

    // Process an incoming decoded JS8 message and trigger AUTO-replies or MeshCore BLE bridge
    void on_receive_complete_message(const std::string& from_call, const std::string& to_call,
                                     const std::string& body, int snr_db, int offset_hz, Submode sm) {
        std::string utc = current_utc_str();
        {
            std::lock_guard<std::mutex> lk(mtx);
            // Update heard list
            bool found = false;
            for (auto& h : heard_stations) {
                if (h.callsign == from_call) {
                    h.snr_db = snr_db;
                    h.offset_hz = offset_hz;
                    h.submode = sm;
                    h.last_utc = utc;
                    h.frame_count++;
                    found = true;
                    break;
                }
            }
            if (!found) {
                JS8HeardStation hs;
                hs.callsign = from_call;
                hs.grid = "CM97";
                hs.snr_db = snr_db;
                hs.offset_hz = offset_hz;
                hs.submode = sm;
                hs.last_utc = utc;
                hs.info = "HF JS8 STATION";
                hs.frame_count = 1;
                heard_stations.insert(heard_stations.begin(), hs);
            }

            std::string full_line = from_call + ": " + to_call + " " + body;
            qso_transcript.push_back("[" + utc + " RX " + std::to_string(snr_db) + "dB] " + full_line);

            // Post JS8 spot to PSK Reporter if web reporting enabled
            if (from_call != my_call && from_call.rfind("@", 0) != 0) {
                uint64_t dial = cached_dial_hz.load();
                if (dial == 0) dial = 7078000ULL;
                hamshack::get_psk_reporter().post_spot(from_call, "", dial + static_cast<uint64_t>(std::max(0, offset_hz)), snr_db, "JS8");
            }

            // Check if message is directed to us or to MSG/MESH
            if (to_call == my_call || to_call == "@ALLCALL" || body.find("MSG") != std::string::npos) {
                if (body.find("MSG") == 0 || to_call == my_call) {
                    JS8InboxMessage im;
                    im.id = (int)inbox.size() + 1;
                    im.from_call = from_call;
                    im.to_call = to_call;
                    im.utc_time = utc;
                    im.body = body;
                    im.unread = true;
                    im.bridged_to_mesh = bridge_to_meshcore;
                    inbox.insert(inbox.begin(), im);
                }
                if (bridge_to_meshcore && on_mesh_bridge_forward) {
                    on_mesh_bridge_forward(from_call, full_line);
                }
            }
        }

        // Handle JS8Call AUTO replies to directed queries
        if (auto_reply_enabled && to_call == my_call) {
            char snr_str[16];
            std::snprintf(snr_str, sizeof(snr_str), "%+03d", snr_db);
            if (body.find("SNR?") != std::string::npos) {
                queue_message(my_call + ": " + from_call + " SNR " + snr_str);
            } else if (body.find("INFO?") != std::string::npos) {
                queue_message(my_call + ": " + from_call + " INFO " + my_info);
            } else if (body.find("GRID?") != std::string::npos) {
                queue_message(my_call + ": " + from_call + " GRID " + my_grid);
            } else if (body.find("STATUS?") != std::string::npos) {
                queue_message(my_call + ": " + from_call + " STATUS " + my_status);
            } else if (body.find("HEARING?") != std::string::npos) {
                std::string hlist;
                int cnt = 0;
                for (const auto& h : heard_stations) {
                    if (cnt++ < 4) hlist += h.callsign + " ";
                }
                queue_message(my_call + ": " + from_call + " HEARING " + hlist);
            }
        }
    }

    void simulate_incoming_cycle() {
        // Removed fake simulation: only live soundcard JS8 frames are decoded
    }

    // Demodulate live 12 kHz soundcard samples for JS8 8-FSK frames using Costas sync + LDPC(174,87) + CRC-12
    void decode_live_audio_slot(const std::vector<float>& pcm_12k) {
        const auto& sp = submode_params(current_submode);
        const int nsps = sp.nsps_12k;
        const int step = nsps / 2;
        if ((int)pcm_12k.size() < nsps * 80) return;

        double energy = 0.0;
        for (float s : pcm_12k) energy += (double)s * s;
        if (energy < 1e-8) return;

        const int nsteps = ((int)pcm_12k.size() - nsps) / step;
        if (nsteps < 158) return;

        double baud = 12000.0 / (double)nsps;
        std::string utc = current_utc_str();

        // Scan candidate frequencies around rx_offset_hz and across the passband (400..2600 Hz)
        for (int f_base = 400; f_base <= 2600; f_base += 25) {
            float best_sync = 0.0f;
            int best_t0 = 0;

            auto goertzel_mag = [&](int sample_start, double freq_hz) -> float {
                if (sample_start < 0 || sample_start + nsps > (int)pcm_12k.size()) return 0.0f;
                double w = 2.0 * M_PI * freq_hz / 12000.0;
                double coeff = 2.0 * std::cos(w);
                double s0 = 0.0, s1 = 0.0, s2 = 0.0;
                for (int i = 0; i < nsps; ++i) {
                    s0 = pcm_12k[sample_start + i] + coeff * s1 - s2;
                    s2 = s1;
                    s1 = s0;
                }
                double power = s1 * s1 + s2 * s2 - coeff * s1 * s2;
                return (float)std::sqrt(std::max(0.0, power)) / (float)nsps;
            };

            for (int t0 = 0; t0 + 78 * 2 < std::min(nsteps, 190); t0 += 4) {
                float sig = 0.0f, bg = 1e-6f;
                for (int k = 0; k < 7; ++k) {
                    int pos[3] = {k, 36 + k, 72 + k};
                    for (int blk = 0; blk < 3; ++blk) {
                        int s_idx = (t0 + pos[blk] * 2) * step;
                        int expected_tone = ICOS7_MOD[blk][k];
                        float m_sig = goertzel_mag(s_idx, f_base + expected_tone * baud);
                        float m_alt = goertzel_mag(s_idx, f_base + ((expected_tone + 4) & 7) * baud);
                        sig += m_sig;
                        bg  += m_alt;
                    }
                }
                float ratio = sig / bg;
                if (ratio > best_sync) {
                    best_sync = ratio;
                    best_t0 = t0;
                }
            }

            if (best_sync > 1.85f) {
                float llr[174]{};
                int sym = 7;
                for (int j = 0; j < 58; ++j) {
                    if (j == 29) sym += 7;
                    int s_idx = (best_t0 + sym * 2) * step;
                    ++sym;
                    float E[8];
                    for (int m = 0; m < 8; ++m) {
                        E[m] = goertzel_mag(s_idx, f_base + m * baud);
                    }
                    for (int bit = 0; bit < 3; ++bit) {
                        int mask = 1 << (2 - bit);
                        float max1 = 0.0f, max0 = 0.0f;
                        for (int m = 0; m < 8; ++m) {
                            if (m & mask) max1 = std::max(max1, E[m]);
                            else          max0 = std::max(max0, E[m]);
                        }
                        float norm = std::max(1e-5f, max1 + max0);
                        llr[3 * j + bit] = 4.5f * (max1 - max0) / norm;
                    }
                }
                std::string decoded_12;
                uint8_t flags = 0;
                if (decode_js8_soft(llr, decoded_12, flags)) {
                    int snr = std::clamp((int)std::round(20.0f * std::log10(std::max(0.1f, best_sync - 1.0f)) - 10.0f), -24, +24);
                    {
                        std::lock_guard<std::mutex> lk(mtx);
                        JS8FrameLog fl{utc, snr, (best_t0 * (float)step) / 12000.0f, f_base, current_submode, flags, decoded_12, false};
                        band_frames.push_back(fl);
                        if (band_frames.size() > 200) band_frames.erase(band_frames.begin());
                    }
                    size_t colon = decoded_12.find(':');
                    if (colon != std::string::npos && colon > 0) {
                        std::string fc = decoded_12.substr(0, colon);
                        std::string rest = decoded_12.substr(colon + 1);
                        while (!rest.empty() && rest.front() == ' ') rest.erase(rest.begin());
                        on_receive_complete_message(fc, "@ALLCALL", rest, snr, f_base, current_submode);
                    }
                }
            }
        }
    }
};

} // namespace js8
