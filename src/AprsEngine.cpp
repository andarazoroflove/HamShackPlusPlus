// ============================================================================
// AprsEngine.cpp — Carbon Engine AX.25 / APRS 1200-Baud KISS-over-TCP Engine
// ============================================================================

#include "AprsEngine.hpp"
#include "hamshack_platform.hpp"

#include <algorithm>
#include <cctype>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstring>

namespace hamshack::carbon {

namespace {

void copy_fixed_str(std::span<char> dst, std::string_view src) noexcept {
    if (dst.empty()) return;
    const size_t n = std::min(dst.size() - 1, src.size());
    if (n > 0) {
        std::memcpy(dst.data(), src.data(), n);
    }
    dst[n] = '\0';
}

std::string_view trim_spaces(std::string_view sv) noexcept {
    while (!sv.empty() && (sv.front() == ' ' || sv.front() == '\t' || sv.front() == '\r' || sv.front() == '\n')) {
        sv.remove_prefix(1);
    }
    while (!sv.empty() && (sv.back() == ' ' || sv.back() == '\t' || sv.back() == '\r' || sv.back() == '\n')) {
        sv.remove_suffix(1);
    }
    return sv;
}

uint64_t now_epoch_ms() noexcept {
    const auto now = std::chrono::system_clock::now().time_since_epoch();
    return static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::milliseconds>(now).count());
}

void stamp_utc_hms(std::span<char, 12> out_hms) noexcept {
    const std::time_t now_t = std::time(nullptr);
    std::tm utc_tm{};
    hamshack::hs_gmtime(&now_t, &utc_tm);
    std::snprintf(out_hms.data(), out_hms.size(), "%02d:%02d:%02d",
                  utc_tm.tm_hour, utc_tm.tm_min, utc_tm.tm_sec);
}

void split_callsign_ssid(std::string_view full, std::span<char, 7> out_call, uint8_t& out_ssid, bool& out_repeated) noexcept {
    std::fill(out_call.begin(), out_call.end(), ' ');
    out_call[6]  = '\0';
    out_ssid     = 0;
    out_repeated = false;

    full = trim_spaces(full);
    if (!full.empty() && full.back() == '*') {
        out_repeated = true;
        full.remove_suffix(1);
    }

    const size_t dash = full.find('-');
    std::string_view base = (dash == std::string_view::npos) ? full : full.substr(0, dash);
    const size_t c_len = std::min<size_t>(6, base.size());
    for (size_t i = 0; i < c_len; ++i) {
        out_call[i] = static_cast<char>(std::toupper(static_cast<unsigned char>(base[i])));
    }

    if (dash != std::string_view::npos && dash + 1 < full.size()) {
        int v = 0;
        for (size_t i = dash + 1; i < full.size(); ++i) {
            const char ch = full[i];
            if (ch >= '0' && ch <= '9') {
                v = v * 10 + (ch - '0');
            } else {
                break;
            }
        }
        out_ssid = static_cast<uint8_t>(std::clamp(v, 0, 15));
    }
}

void encode_ax25_addr_field(std::string_view call_ssid, bool is_last, std::span<uint8_t, 7> out_addr) noexcept {
    std::array<char, 7> call6{};
    uint8_t ssid  = 0;
    bool repeated = false;
    split_callsign_ssid(call_ssid, call6, ssid, repeated);

    for (size_t i = 0; i < 6; ++i) {
        out_addr[i] = static_cast<uint8_t>(static_cast<unsigned char>(call6[i]) << 1);
    }
    uint8_t ssid_byte = 0x60u | static_cast<uint8_t>((ssid & 0x0Fu) << 1);
    if (repeated) ssid_byte |= 0x80u;
    if (is_last)  ssid_byte |= 0x01u;
    out_addr[6] = ssid_byte;
}

void decode_ax25_addr_field(std::span<const uint8_t, 7> addr,
                            std::span<char> out_base,
                            uint8_t& out_ssid,
                            bool& out_repeated,
                            std::span<char> out_formatted) noexcept {
    std::array<char, 7> base{};
    size_t len = 0;
    for (size_t i = 0; i < 6; ++i) {
        const char ch = static_cast<char>((addr[i] >> 1) & 0x7Fu);
        if (ch != ' ' && ch >= 0x20 && ch <= 0x7E) {
            base[len++] = ch;
        }
    }
    base[len]    = '\0';
    out_ssid     = static_cast<uint8_t>((addr[6] >> 1) & 0x0Fu);
    out_repeated = (addr[6] & 0x80u) != 0;

    copy_fixed_str(out_base, std::string_view{base.data(), len});
    if (!out_formatted.empty()) {
        if (out_ssid > 0) {
            std::snprintf(out_formatted.data(), out_formatted.size(), "%s-%u%s",
                          base.data(), static_cast<unsigned>(out_ssid), out_repeated ? "*" : "");
        } else {
            std::snprintf(out_formatted.data(), out_formatted.size(), "%s%s",
                          base.data(), out_repeated ? "*" : "");
        }
    }
}

bool parse_fixed_digits(std::string_view sv, int& out_val) noexcept {
    if (sv.empty()) return false;
    int v = 0;
    for (char c : sv) {
        if (c < '0' || c > '9') return false;
        v = v * 10 + (c - '0');
    }
    out_val = v;
    return true;
}

// Parse APRS DDMM.mmN (8 chars) & DDDMM.mmW (9 chars)
bool parse_uncompressed_lat_lon(std::string_view lat8, std::string_view lon9,
                                double& out_lat, double& out_lon) noexcept {
    if (lat8.size() != 8 || lon9.size() != 9 || lat8[4] != '.' || lon9[5] != '.') {
        return false;
    }
    const char ns = static_cast<char>(std::toupper(static_cast<unsigned char>(lat8[7])));
    const char ew = static_cast<char>(std::toupper(static_cast<unsigned char>(lon9[8])));
    if ((ns != 'N' && ns != 'S') || (ew != 'E' && ew != 'W')) {
        return false;
    }

    auto digit_or_space = [](char c) noexcept -> int {
        if (c >= '0' && c <= '9') return c - '0';
        if (c == ' ') return 0; // APRS ambiguity space
        return -1;
    };

    int lat_deg = 0;
    if (!parse_fixed_digits(lat8.substr(0, 2), lat_deg)) return false;
    int lm0 = digit_or_space(lat8[2]), lm1 = digit_or_space(lat8[3]);
    int lf0 = digit_or_space(lat8[5]), lf1 = digit_or_space(lat8[6]);
    if (lm0 < 0 || lm1 < 0 || lf0 < 0 || lf1 < 0) return false;

    int lon_deg = 0;
    if (!parse_fixed_digits(lon9.substr(0, 3), lon_deg)) return false;
    int xm0 = digit_or_space(lon9[3]), xm1 = digit_or_space(lon9[4]);
    int xf0 = digit_or_space(lon9[6]), xf1 = digit_or_space(lon9[7]);
    if (xm0 < 0 || xm1 < 0 || xf0 < 0 || xf1 < 0) return false;

    const double lat_min = static_cast<double>(lm0 * 10 + lm1) + static_cast<double>(lf0 * 10 + lf1) * 0.01;
    const double lon_min = static_cast<double>(xm0 * 10 + xm1) + static_cast<double>(xf0 * 10 + xf1) * 0.01;
    double lat = static_cast<double>(lat_deg) + lat_min / 60.0;
    double lon = static_cast<double>(lon_deg) + lon_min / 60.0;
    if (ns == 'S') lat = -lat;
    if (ew == 'W') lon = -lon;
    if (std::abs(lat) > 90.0 || std::abs(lon) > 180.0) return false;

    out_lat = lat;
    out_lon = lon;
    return true;
}

// Parse Base-91 Compressed Position (13 chars: /YYYYXXXX$csT)
bool parse_compressed_position(std::string_view comp13, AprsDecodedPacket& pkt) noexcept {
    if (comp13.size() < 13) return false;
    const char sym_tbl = comp13[0];
    if (sym_tbl != '/' && sym_tbl != '\\' && !(sym_tbl >= 'A' && sym_tbl <= 'Z') && !(sym_tbl >= '0' && sym_tbl <= '9')) {
        return false;
    }
    int32_t y_val = 0;
    int32_t x_val = 0;
    for (size_t i = 1; i <= 4; ++i) {
        const int c = static_cast<unsigned char>(comp13[i]) - 33;
        if (c < 0 || c > 90) return false;
        y_val = y_val * 91 + c;
    }
    for (size_t i = 5; i <= 8; ++i) {
        const int c = static_cast<unsigned char>(comp13[i]) - 33;
        if (c < 0 || c > 90) return false;
        x_val = x_val * 91 + c;
    }

    const double lat = 90.0 - static_cast<double>(y_val) / 380926.0;
    const double lon = -180.0 + static_cast<double>(x_val) / 190463.0;
    if (std::abs(lat) > 90.0 || std::abs(lon) > 180.0) return false;

    pkt.has_position = true;
    pkt.latitude     = lat;
    pkt.longitude    = lon;
    pkt.symbol_table = sym_tbl;
    pkt.symbol_code  = comp13[9];

    const int cs_c = static_cast<unsigned char>(comp13[10]) - 33;
    const int cs_s = static_cast<unsigned char>(comp13[11]) - 33;
    const int comp_t = static_cast<unsigned char>(comp13[12]) - 33;

    if (cs_c >= 0 && cs_c <= 89 && cs_s >= 0 && cs_s <= 90) {
        if ((comp_t & 0x18) == 0x10) {
            // Compressed GGA altitude: 1.002^(cs_c*91 + cs_s) feet
            const int exp_v = cs_c * 91 + cs_s;
            pkt.altitude_ft  = static_cast<float>(std::pow(1.002, static_cast<double>(exp_v)));
            pkt.has_altitude = true;
        } else {
            pkt.course_deg     = static_cast<float>(cs_c * 4);
            pkt.speed_knots    = static_cast<float>(std::pow(1.08, static_cast<double>(cs_s)) - 1.0);
            pkt.has_kinematics = true;
        }
    }
    return true;
}

// Parse Mic-E Position (DTI '`' or '\'', 8+ byte info + 6-char AX.25 destination)
bool parse_mice_payload(std::string_view info, std::string_view dest_call, AprsDecodedPacket& pkt) noexcept {
    if (info.size() < 9 || dest_call.size() < 6) return false;

    std::array<int, 6> lat_digits{};
    bool is_north   = false;
    bool lon_add100 = false;
    bool is_west    = false;

    for (size_t i = 0; i < 6; ++i) {
        const unsigned char c = static_cast<unsigned char>(std::toupper(static_cast<unsigned char>(dest_call[i])));
        bool bit = false;
        if (c >= '0' && c <= '9') {
            lat_digits[i] = c - '0';
            bit = false;
        } else if (c >= 'A' && c <= 'J') {
            lat_digits[i] = c - 'A';
            bit = true;
        } else if (c == 'K') {
            lat_digits[i] = 0;
            bit = true;
        } else if (c == 'L') {
            lat_digits[i] = 0;
            bit = false;
        } else if (c >= 'P' && c <= 'Y') {
            lat_digits[i] = c - 'P';
            bit = true;
        } else if (c == 'Z') {
            lat_digits[i] = 0;
            bit = true;
        } else {
            return false;
        }
        if (i == 3) is_north   = bit;
        if (i == 4) lon_add100 = bit;
        if (i == 5) is_west    = bit;
    }

    const int lat_deg = lat_digits[0] * 10 + lat_digits[1];
    const double lat_min = static_cast<double>(lat_digits[2] * 10 + lat_digits[3]) +
                           static_cast<double>(lat_digits[4] * 10 + lat_digits[5]) * 0.01;
    double lat = static_cast<double>(lat_deg) + lat_min / 60.0;
    if (!is_north) lat = -lat;

    // Decode Mic-E longitude from info[1..3]
    int d_lon = static_cast<int>(static_cast<unsigned char>(info[1])) - 28;
    if (lon_add100) d_lon += 100;
    if (d_lon >= 180 && d_lon <= 189) d_lon -= 80;
    else if (d_lon >= 190 && d_lon <= 199) d_lon -= 190;

    int m_lon = static_cast<int>(static_cast<unsigned char>(info[2])) - 28;
    if (m_lon >= 60) m_lon -= 60;
    const int h_lon = static_cast<int>(static_cast<unsigned char>(info[3])) - 28;

    double lon = static_cast<double>(d_lon) + (static_cast<double>(m_lon) + static_cast<double>(h_lon) * 0.01) / 60.0;
    if (is_west) lon = -lon;
    if (std::abs(lat) > 90.0 || std::abs(lon) > 180.0) return false;

    // Decode Mic-E speed & course from info[4..6]
    const int sp = static_cast<int>(static_cast<unsigned char>(info[4])) - 28;
    const int dc = static_cast<int>(static_cast<unsigned char>(info[5])) - 28;
    const int se = static_cast<int>(static_cast<unsigned char>(info[6])) - 28;

    int speed_kts = sp * 10 + (dc / 10);
    if (speed_kts >= 800) speed_kts -= 800;
    int course = (dc % 10) * 100 + se;
    if (course >= 400) course -= 400;

    pkt.kind           = AprsPacketKind::MicE;
    pkt.has_position   = true;
    pkt.latitude       = lat;
    pkt.longitude      = lon;
    pkt.has_kinematics = true;
    pkt.speed_knots    = static_cast<float>(std::max(0, speed_kts));
    pkt.course_deg     = static_cast<float>(std::clamp(course, 0, 360));
    pkt.symbol_code    = info[7];
    pkt.symbol_table   = info[8];

    std::string_view rem = info.substr(9);
    // Optional Mic-E telemetry prefix skip ('`' or '\'' or 5-byte binary) and Base-91 altitude "xxx}"
    const size_t brace = rem.find('}');
    if (brace != std::string_view::npos && brace >= 3) {
        const int a0 = static_cast<int>(static_cast<unsigned char>(rem[brace - 3])) - 33;
        const int a1 = static_cast<int>(static_cast<unsigned char>(rem[brace - 2])) - 33;
        const int a2 = static_cast<int>(static_cast<unsigned char>(rem[brace - 1])) - 33;
        if (a0 >= 0 && a0 <= 90 && a1 >= 0 && a1 <= 90 && a2 >= 0 && a2 <= 90) {
            const int alt_m = a0 * 8281 + a1 * 91 + a2 - 10000;
            pkt.altitude_ft  = static_cast<float>(static_cast<double>(alt_m) * 3.280839895);
            pkt.has_altitude = true;
            rem = rem.substr(brace + 1);
        }
    }
    copy_fixed_str(pkt.comment, trim_spaces(rem));
    return true;
}

void extract_comment_kinematics_and_altitude(std::string_view rest, AprsDecodedPacket& pkt) noexcept {
    // Check optional CSE/SPD ("DDD/SSS") at beginning of comment
    if (rest.size() >= 7 && rest[3] == '/') {
        int cse = 0, spd = 0;
        if (parse_fixed_digits(rest.substr(0, 3), cse) && parse_fixed_digits(rest.substr(4, 3), spd)) {
            if (cse >= 0 && cse <= 360) {
                pkt.course_deg     = static_cast<float>(cse);
                pkt.speed_knots    = static_cast<float>(spd);
                pkt.has_kinematics = true;
                rest.remove_prefix(7);
            }
        }
    }

    // Check "/A=XXXXXX" or "/A=-XXXXX" altitude in feet anywhere in comment
    const size_t alt_pos = rest.find("/A=");
    if (alt_pos != std::string_view::npos && alt_pos + 9 <= rest.size()) {
        std::string_view a_sv = rest.substr(alt_pos + 3, 6);
        bool neg = false;
        if (!a_sv.empty() && a_sv.front() == '-') {
            neg = true;
            a_sv.remove_prefix(1);
        }
        int alt_ft = 0;
        if (parse_fixed_digits(a_sv, alt_ft)) {
            pkt.altitude_ft  = static_cast<float>(neg ? -alt_ft : alt_ft);
            pkt.has_altitude = true;
        }
    }

    // Build clean human-readable comment stripping "/A=XXXXXX"
    std::array<char, 160> clean{};
    size_t out_i = 0;
    for (size_t i = 0; i < rest.size() && out_i + 1 < clean.size();) {
        if (i + 9 <= rest.size() && rest.substr(i, 3) == "/A=") {
            i += 9;
            continue;
        }
        const unsigned char c = static_cast<unsigned char>(rest[i]);
        if (c >= 0x20 && c <= 0x7E) {
            clean[out_i++] = static_cast<char>(c);
        }
        ++i;
    }
    clean[out_i] = '\0';
    copy_fixed_str(pkt.comment, trim_spaces(std::string_view{clean.data(), out_i}));
}

bool calls_equal_ci(std::string_view a, std::string_view b) noexcept {
    a = trim_spaces(a);
    b = trim_spaces(b);
    if (a.size() != b.size()) return false;
    for (size_t i = 0; i < a.size(); ++i) {
        if (std::toupper(static_cast<unsigned char>(a[i])) !=
            std::toupper(static_cast<unsigned char>(b[i]))) {
            return false;
        }
    }
    return true;
}

} // namespace

AprsEngine::AprsEngine() {
    packet_log_.reserve(kMaxPacketLog);
    stations_.reserve(kMaxStations);
    chat_log_.reserve(kMaxChatItems);
}

AprsEngine::~AprsEngine() {
    disconnect_kiss();
}

void AprsEngine::set_station_identity(std::string_view callsign, uint8_t ssid, std::string_view digi_path) {
    std::string clean;
    clean.reserve(10);
    for (char c : trim_spaces(callsign)) {
        if (c == '-') break;
        if (std::isalnum(static_cast<unsigned char>(c))) {
            clean.push_back(static_cast<char>(std::toupper(static_cast<unsigned char>(c))));
        }
        if (clean.size() >= 6) break;
    }
    if (!clean.empty()) {
        my_call_ = std::move(clean);
    }
    my_ssid_   = std::min<uint8_t>(ssid, 15);
    digi_path_ = std::string(trim_spaces(digi_path));
}

void AprsEngine::set_kiss_endpoint(std::string_view host, uint16_t port) {
    const std::string_view h = trim_spaces(host);
    if (!h.empty()) {
        kiss_host_ = std::string(h);
    }
    if (port > 0) {
        kiss_port_ = port;
    }
}

std::string AprsEngine::full_station_call() const {
    if (my_ssid_ > 0) {
        return my_call_ + "-" + std::to_string(my_ssid_);
    }
    return my_call_;
}

void AprsEngine::connect_kiss() {
    if (worker_running_.exchange(true, std::memory_order_acq_rel)) {
        return;
    }
    if (net_thread_.joinable()) {
        net_thread_.join();
    }
    net_thread_ = std::thread(&AprsEngine::network_worker_loop, this);
}

void AprsEngine::disconnect_kiss() {
    worker_running_.store(false, std::memory_order_release);
    if (net_thread_.joinable()) {
        net_thread_.join();
    }
    tcp_connected_.store(false, std::memory_order_release);
}

size_t AprsEngine::kiss_escape_frame(std::span<const uint8_t> ax25_payload,
                                     std::span<uint8_t> out_kiss,
                                     uint8_t port_cmd) noexcept {
    if (ax25_payload.empty() || out_kiss.size() < 4) return 0;
    size_t pos = 0;
    out_kiss[pos++] = kKissFend;
    out_kiss[pos++] = port_cmd;

    for (uint8_t b : ax25_payload) {
        if (b == kKissFend) {
            if (pos + 3 > out_kiss.size()) return 0;
            out_kiss[pos++] = kKissFesc;
            out_kiss[pos++] = kKissTfend;
        } else if (b == kKissFesc) {
            if (pos + 3 > out_kiss.size()) return 0;
            out_kiss[pos++] = kKissFesc;
            out_kiss[pos++] = kKissTfesc;
        } else {
            if (pos + 2 > out_kiss.size()) return 0;
            out_kiss[pos++] = b;
        }
    }
    out_kiss[pos++] = kKissFend;
    return pos;
}

size_t AprsEngine::encode_ax25_ui_frame(std::string_view src_call_ssid,
                                        std::string_view dst_call_ssid,
                                        std::string_view digi_path_csv,
                                        std::string_view info_field,
                                        std::span<uint8_t> out_ax25) noexcept {
    // Parse up to 8 comma-separated digipeaters with zero heap allocations
    std::array<std::string_view, 8> digis{};
    size_t n_digis = 0;
    std::string_view rem = trim_spaces(digi_path_csv);
    while (!rem.empty() && n_digis < digis.size()) {
        const size_t comma = rem.find(',');
        const std::string_view token = trim_spaces(comma == std::string_view::npos ? rem : rem.substr(0, comma));
        if (!token.empty()) {
            digis[n_digis++] = token;
        }
        if (comma == std::string_view::npos) break;
        rem.remove_prefix(comma + 1);
    }

    const size_t header_len = (2 + n_digis) * 7 + 2;
    const size_t total_len  = header_len + info_field.size();
    if (out_ax25.size() < total_len) return 0;

    encode_ax25_addr_field(dst_call_ssid, false, out_ax25.subspan<0, 7>());
    encode_ax25_addr_field(src_call_ssid, n_digis == 0, out_ax25.subspan<7, 7>());

    size_t offset = 14;
    for (size_t i = 0; i < n_digis; ++i) {
        std::array<uint8_t, 7> d_addr{};
        encode_ax25_addr_field(digis[i], (i + 1 == n_digis), d_addr);
        std::copy(d_addr.begin(), d_addr.end(), out_ax25.begin() + static_cast<std::ptrdiff_t>(offset));
        offset += 7;
    }

    out_ax25[offset++] = 0x03u; // AX.25 UI Control byte
    out_ax25[offset++] = 0xF0u; // AX.25 PID: No Layer 3 protocol

    if (!info_field.empty()) {
        std::memcpy(out_ax25.data() + offset, info_field.data(), info_field.size());
    }
    return total_len;
}

bool AprsEngine::decode_ax25_ui_frame(std::span<const uint8_t> ax25_bytes,
                                      AprsDecodedPacket& out_pkt) noexcept {
    // Minimum AX.25 UI frame: 7B dest + 7B src + 1B ctrl + 1B pid = 16 bytes
    if (ax25_bytes.size() < 16) return false;

    out_pkt = AprsDecodedPacket{};
    out_pkt.rx_epoch_ms = now_epoch_ms();
    stamp_utc_hms(out_pkt.utc_hms);

    std::array<char, 12> dest_base{};
    uint8_t dest_ssid = 0;
    bool dest_rep     = false;
    decode_ax25_addr_field(ax25_bytes.subspan<0, 7>(), dest_base, dest_ssid, dest_rep, out_pkt.dest_call);

    bool src_rep = false;
    decode_ax25_addr_field(ax25_bytes.subspan<7, 7>(), out_pkt.source_call, out_pkt.source_ssid, src_rep, out_pkt.full_source);

    size_t pos = 14;
    bool addr_end = (ax25_bytes[13] & 0x01u) != 0;
    size_t digi_str_len = 0;
    size_t digi_count   = 0;

    while (!addr_end && pos + 7 <= ax25_bytes.size() && digi_count < 8) {
        std::array<char, 12> d_base{};
        std::array<char, 16> d_fmt{};
        uint8_t d_ssid = 0;
        bool d_rep     = false;
        std::array<uint8_t, 7> d_raw{};
        std::copy_n(ax25_bytes.data() + pos, 7, d_raw.data());
        decode_ax25_addr_field(d_raw, d_base, d_ssid, d_rep, d_fmt);

        const std::string_view df_sv{d_fmt.data()};
        if (!df_sv.empty()) {
            if (digi_str_len > 0 && digi_str_len + 1 < out_pkt.digi_path.size()) {
                out_pkt.digi_path[digi_str_len++] = ',';
            }
            const size_t copy_n = std::min(out_pkt.digi_path.size() - 1 - digi_str_len, df_sv.size());
            std::memcpy(out_pkt.digi_path.data() + digi_str_len, df_sv.data(), copy_n);
            digi_str_len += copy_n;
            out_pkt.digi_path[digi_str_len] = '\0';
        }

        addr_end = (ax25_bytes[pos + 6] & 0x01u) != 0;
        pos += 7;
        ++digi_count;
    }

    if (!addr_end || pos + 2 > ax25_bytes.size()) {
        return false;
    }

    const uint8_t ctrl = ax25_bytes[pos++];
    const uint8_t pid  = ax25_bytes[pos++];
    // UI frame control is 0x03 (or 0x13 with Poll bit); PID 0xF0 is APRS No-L3
    if ((ctrl & 0xEFu) != 0x03u || pid != 0xF0u) {
        return false;
    }

    std::string_view info_sv{reinterpret_cast<const char*>(ax25_bytes.data() + pos), ax25_bytes.size() - pos};
    // Trim trailing CR/LF from TNC info payload
    while (!info_sv.empty() && (info_sv.back() == '\r' || info_sv.back() == '\n' || info_sv.back() == '\0')) {
        info_sv.remove_suffix(1);
    }

    // Format TNC2 monitor line: "SRC>DEST,PATH:INFO"
    if (digi_str_len > 0) {
        std::snprintf(out_pkt.tnc2_raw.data(), out_pkt.tnc2_raw.size(), "%s>%s,%s:%.*s",
                      out_pkt.full_source.data(), out_pkt.dest_call.data(), out_pkt.digi_path.data(),
                      static_cast<int>(info_sv.size()), info_sv.data());
    } else {
        std::snprintf(out_pkt.tnc2_raw.data(), out_pkt.tnc2_raw.size(), "%s>%s:%.*s",
                      out_pkt.full_source.data(), out_pkt.dest_call.data(),
                      static_cast<int>(info_sv.size()), info_sv.data());
    }

    parse_aprs_info_field(info_sv, std::string_view{dest_base.data()}, out_pkt);
    return true;
}

void AprsEngine::parse_aprs_info_field(std::string_view info,
                                       std::string_view dest_call,
                                       AprsDecodedPacket& pkt) noexcept {
    if (info.empty()) return;
    const char dti = info.front();

    // 1. APRS Position (without timestamp: '!' or '='; with 7-char timestamp: '/' or '@')
    if (dti == '!' || dti == '=' || dti == '/' || dti == '@') {
        const size_t offset = (dti == '/' || dti == '@') ? 8u : 1u;
        if (info.size() > offset) {
            std::string_view body = info.substr(offset);
            // Uncompressed DDMM.mmN/DDDMM.mmW$ (19 chars)
            if (body.size() >= 19 && body[4] == '.' && body[14] == '.') {
                double lat = 0.0, lon = 0.0;
                if (parse_uncompressed_lat_lon(body.substr(0, 8), body.substr(9, 9), lat, lon)) {
                    pkt.kind         = AprsPacketKind::Position;
                    pkt.has_position = true;
                    pkt.latitude     = lat;
                    pkt.longitude    = lon;
                    pkt.symbol_table = body[8];
                    pkt.symbol_code  = body[18];
                    extract_comment_kinematics_and_altitude(body.substr(19), pkt);
                    return;
                }
            }
            // Base-91 Compressed Position (13 chars)
            if (body.size() >= 13 && parse_compressed_position(body.substr(0, 13), pkt)) {
                pkt.kind = AprsPacketKind::Position;
                extract_comment_kinematics_and_altitude(body.substr(13), pkt);
                return;
            }
        }
    }

    // 2. Mic-E Compressed Position ('`' or '\'' or 0x1C / 0x1D)
    if (dti == '`' || dti == '\'' || dti == 0x1C || dti == 0x1D) {
        if (parse_mice_payload(info, dest_call, pkt)) {
            return;
        }
    }

    // 3. APRS 2-Way Text Message (":ADDRESSEE:message text{id")
    if (dti == ':' && info.size() >= 11 && info[10] == ':') {
        const std::string_view addr_sv = trim_spaces(info.substr(1, 9));
        std::string_view text_sv       = info.substr(11);

        copy_fixed_str(pkt.msg_addressee, addr_sv);

        // Extract optional "{id" message sequence suffix (up to 5 chars after last '{')
        const size_t brace = text_sv.rfind('{');
        if (brace != std::string_view::npos && brace + 1 < text_sv.size() && (text_sv.size() - brace - 1) <= 5) {
            copy_fixed_str(pkt.msg_id, trim_spaces(text_sv.substr(brace + 1)));
            text_sv = text_sv.substr(0, brace);
        }

        if (text_sv.size() >= 3 && text_sv.substr(0, 3) == "ack") {
            pkt.kind = AprsPacketKind::MessageAck;
            copy_fixed_str(pkt.msg_id, trim_spaces(text_sv.substr(3)));
            copy_fixed_str(pkt.msg_text, text_sv);
        } else if (text_sv.size() >= 3 && text_sv.substr(0, 3) == "rej") {
            pkt.kind = AprsPacketKind::MessageRej;
            copy_fixed_str(pkt.msg_id, trim_spaces(text_sv.substr(3)));
            copy_fixed_str(pkt.msg_text, text_sv);
        } else {
            pkt.kind = AprsPacketKind::TextMessage;
            copy_fixed_str(pkt.msg_text, text_sv);
            copy_fixed_str(pkt.comment, text_sv);
        }
        return;
    }

    // 4. Status ('>') or Telemetry ('T') or Generic Comment
    if (dti == '>') {
        pkt.kind = AprsPacketKind::Status;
        copy_fixed_str(pkt.comment, trim_spaces(info.substr(1)));
    } else if (dti == 'T') {
        pkt.kind = AprsPacketKind::Telemetry;
        copy_fixed_str(pkt.comment, trim_spaces(info));
    } else {
        pkt.kind = AprsPacketKind::Unknown;
        copy_fixed_str(pkt.comment, trim_spaces(info));
    }
}

std::string AprsEngine::format_uncompressed_position(double lat_deg, double lon_deg,
                                                     char sym_table, char sym_code,
                                                     std::string_view comment) {
    lat_deg = std::clamp(lat_deg, -90.0, 90.0);
    lon_deg = std::clamp(lon_deg, -180.0, 180.0);

    const char ns = (lat_deg >= 0.0) ? 'N' : 'S';
    const char ew = (lon_deg >= 0.0) ? 'E' : 'W';
    const double alat = std::abs(lat_deg);
    const double alon = std::abs(lon_deg);

    int lat_d = static_cast<int>(alat);
    double lat_m = (alat - static_cast<double>(lat_d)) * 60.0;
    if (lat_m >= 59.995) { lat_m = 0.0; ++lat_d; }

    int lon_d = static_cast<int>(alon);
    double lon_m = (alon - static_cast<double>(lon_d)) * 60.0;
    if (lon_m >= 59.995) { lon_m = 0.0; ++lon_d; }

    std::array<char, 256> buf{};
    std::snprintf(buf.data(), buf.size(), "=%02d%05.2f%c%c%03d%05.2f%c%c%.*s",
                  lat_d, lat_m, ns, sym_table,
                  lon_d, lon_m, ew, sym_code,
                  static_cast<int>(std::min<size_t>(comment.size(), 180)), comment.data());
    return std::string(buf.data());
}

bool AprsEngine::enqueue_outbound_ax25(std::string_view info_field, std::string_view custom_src) noexcept {
    std::array<char, 24> src_buf{};
    if (!custom_src.empty()) {
        copy_fixed_str(src_buf, custom_src);
    } else if (my_ssid_ > 0) {
        std::snprintf(src_buf.data(), src_buf.size(), "%s-%u", my_call_.c_str(), static_cast<unsigned>(my_ssid_));
    } else {
        copy_fixed_str(src_buf, my_call_);
    }

    std::array<uint8_t, 384> ax25{};
    const size_t ax25_len = encode_ax25_ui_frame(
        std::string_view{src_buf.data()},
        "APRS",
        digi_path_,
        info_field,
        ax25);
    if (ax25_len == 0) return false;

    AprsKissWireFrame wire{};
    wire.length = kiss_escape_frame(std::span<const uint8_t>{ax25.data(), ax25_len}, wire.bytes, 0x00u);
    if (wire.length == 0) return false;

    if (!tx_wire_ring_.try_push(wire)) {
        return false;
    }
    tx_frames_.fetch_add(1u, std::memory_order_relaxed);

    // Also record a local TX echo packet so the Packet Log & Station Map reflect outbound frames immediately
    AprsDecodedPacket echo{};
    if (decode_ax25_ui_frame(std::span<const uint8_t>{ax25.data(), ax25_len}, echo)) {
        echo.is_tx = true;
        apply_packet_to_ui_tables(echo, nullptr);
    }
    return true;
}

bool AprsEngine::send_position_beacon(double lat_deg, double lon_deg,
                                      char sym_table, char sym_code,
                                      std::string_view comment) {
    const std::string payload = format_uncompressed_position(lat_deg, lon_deg, sym_table, sym_code, comment);
    return enqueue_outbound_ax25(payload);
}

bool AprsEngine::send_text_message(std::string_view to_call_ssid, std::string_view message_text) {
    const std::string_view clean_to  = trim_spaces(to_call_ssid);
    const std::string_view clean_txt = trim_spaces(message_text);
    if (clean_to.empty() || clean_txt.empty()) return false;

    std::array<char, 10> addr9{};
    addr9.fill(' ');
    addr9[9] = '\0';
    for (size_t i = 0; i < std::min<size_t>(9, clean_to.size()); ++i) {
        addr9[i] = static_cast<char>(std::toupper(static_cast<unsigned char>(clean_to[i])));
    }

    const uint32_t seq = (next_msg_seq_++ % 999u) + 1u;
    std::array<char, 160> payload{};
    std::snprintf(payload.data(), payload.size(), ":%s:%.*s{%03u",
                  addr9.data(),
                  static_cast<int>(std::min<size_t>(67, clean_txt.size())),
                  clean_txt.data(),
                  static_cast<unsigned>(seq));

    return enqueue_outbound_ax25(std::string_view{payload.data()});
}

bool AprsEngine::send_raw_aprs_payload(std::string_view info_payload) {
    if (info_payload.empty()) return false;
    return enqueue_outbound_ax25(info_payload);
}

void AprsEngine::handle_complete_ax25_frame(std::span<const uint8_t> ax25) noexcept {
    AprsDecodedPacket pkt{};
    if (!decode_ax25_ui_frame(ax25, pkt)) {
        return;
    }
    rx_frames_.fetch_add(1u, std::memory_order_relaxed);
    rx_event_ring_.try_push(pkt);
}

void AprsEngine::process_kiss_chunk(std::span<const uint8_t> bytes) noexcept {
    for (uint8_t b : bytes) {
        if (b == kKissFend) {
            if (kiss_in_frame_ && kiss_rx_len_ > 1) {
                const uint8_t cmd_nibble = kiss_rx_accum_[0] & 0x0Fu;
                if (cmd_nibble == 0x00u) {
                    handle_complete_ax25_frame(std::span<const uint8_t>{kiss_rx_accum_.data() + 1, kiss_rx_len_ - 1});
                }
            }
            kiss_rx_len_   = 0;
            kiss_in_esc_   = false;
            kiss_in_frame_ = true;
            continue;
        }

        if (!kiss_in_frame_) {
            // Tolerate streams starting mid-frame if FEND was initial delimiter
            kiss_in_frame_ = true;
            kiss_rx_len_   = 0;
        }

        if (kiss_in_esc_) {
            kiss_in_esc_ = false;
            if (b == kKissTfend) b = kKissFend;
            else if (b == kKissTfesc) b = kKissFesc;
        } else if (b == kKissFesc) {
            kiss_in_esc_ = true;
            continue;
        }

        if (kiss_rx_len_ < kiss_rx_accum_.size()) {
            kiss_rx_accum_[kiss_rx_len_++] = b;
        } else {
            // Corrupt oversized frame — drop and wait for next FEND
            kiss_rx_len_   = 0;
            kiss_in_frame_ = false;
            kiss_in_esc_   = false;
        }
    }
}

size_t AprsEngine::inject_kiss_bytes(std::span<const uint8_t> kiss_stream) noexcept {
    const uint64_t before = rx_frames_.load(std::memory_order_relaxed);
    process_kiss_chunk(kiss_stream);
    return static_cast<size_t>(rx_frames_.load(std::memory_order_relaxed) - before);
}

void AprsEngine::apply_packet_to_ui_tables(const AprsDecodedPacket& pkt,
                                           const std::function<void(const AprsStationEntry&)>& on_station_gps) {
    if (packet_log_.size() >= kMaxPacketLog) {
        packet_log_.erase(packet_log_.begin());
    }
    packet_log_.push_back(pkt);

    const std::string_view full_src{pkt.full_source.data()};
    if (!full_src.empty()) {
        AprsStationEntry* st = nullptr;
        for (auto& existing : stations_) {
            if (calls_equal_ci(existing.callsign_ssid, full_src)) {
                st = &existing;
                break;
            }
        }
        if (!st) {
            if (stations_.size() >= kMaxStations) {
                stations_.erase(stations_.begin());
            }
            stations_.push_back(AprsStationEntry{});
            st = &stations_.back();
            st->callsign_ssid = std::string(full_src);
            st->base_callsign = std::string(pkt.source_call.data());
            st->ssid          = pkt.source_ssid;
        }

        st->packet_count += 1;
        st->last_utc      = std::string(pkt.utc_hms.data());
        st->last_path     = std::string(pkt.digi_path.data());
        if (pkt.comment[0] != '\0') {
            st->last_comment = std::string(pkt.comment.data());
        }
        if (pkt.has_position) {
            st->has_position = true;
            st->latitude     = pkt.latitude;
            st->longitude    = pkt.longitude;
            st->symbol_table = pkt.symbol_table;
            st->symbol_code  = pkt.symbol_code;
            if (pkt.has_altitude)   st->altitude_ft = pkt.altitude_ft;
            if (pkt.has_kinematics) {
                st->speed_knots = pkt.speed_knots;
                st->course_deg  = pkt.course_deg;
            }
            if (on_station_gps) {
                on_station_gps(*st);
            }
        }
    }

    // Handle 2-Way APRS Messaging & Auto-ACK
    const std::string my_full = full_station_call();
    if (pkt.kind == AprsPacketKind::TextMessage) {
        const std::string_view to_sv{pkt.msg_addressee.data()};
        const std::string_view from_sv{pkt.full_source.data()};
        if (chat_log_.size() >= kMaxChatItems) {
            chat_log_.erase(chat_log_.begin());
        }
        AprsChatEntry ce{};
        ce.utc_hms     = std::string(pkt.utc_hms.data());
        ce.peer_call   = std::string(pkt.is_tx ? to_sv : from_sv);
        ce.sender_call = std::string(from_sv);
        ce.text        = std::string(pkt.msg_text.data());
        ce.msg_id      = std::string(pkt.msg_id.data());
        ce.outbound    = pkt.is_tx;
        chat_log_.push_back(std::move(ce));

        // If inbound message is addressed to us and carries a sequence ID, transmit KISS Auto-ACK!
        if (!pkt.is_tx && pkt.msg_id[0] != '\0' &&
            (calls_equal_ci(to_sv, my_full) || calls_equal_ci(to_sv, my_call_))) {
            std::array<char, 10> reply_addr{};
            reply_addr.fill(' ');
            reply_addr[9] = '\0';
            for (size_t i = 0; i < std::min<size_t>(9, from_sv.size()); ++i) {
                reply_addr[i] = static_cast<char>(std::toupper(static_cast<unsigned char>(from_sv[i])));
            }
            std::array<char, 48> ack_payload{};
            std::snprintf(ack_payload.data(), ack_payload.size(), ":%s:ack%s",
                          reply_addr.data(), pkt.msg_id.data());

            std::array<uint8_t, 256> ax25{};
            const size_t ax_len = encode_ax25_ui_frame(my_full, "APRS", digi_path_,
                                                       std::string_view{ack_payload.data()}, ax25);
            if (ax_len > 0) {
                AprsKissWireFrame wire{};
                wire.length = kiss_escape_frame(std::span<const uint8_t>{ax25.data(), ax_len}, wire.bytes, 0x00u);
                if (wire.length > 0 && tx_wire_ring_.try_push(wire)) {
                    tx_frames_.fetch_add(1u, std::memory_order_relaxed);
                }
            }
        }
    } else if (!pkt.is_tx && (pkt.kind == AprsPacketKind::MessageAck || pkt.kind == AprsPacketKind::MessageRej)) {
        const std::string_view ack_id{pkt.msg_id.data()};
        const std::string_view from_sv{pkt.full_source.data()};
        if (!ack_id.empty()) {
            for (auto it = chat_log_.rbegin(); it != chat_log_.rend(); ++it) {
                if (it->outbound && it->msg_id == ack_id && calls_equal_ci(it->peer_call, from_sv)) {
                    if (pkt.kind == AprsPacketKind::MessageAck) it->acked = true;
                    else it->rejected = true;
                    break;
                }
            }
        }
    }
}

size_t AprsEngine::poll_ui_events(const std::function<void(const AprsStationEntry&)>& on_station_gps) {
    std::array<AprsDecodedPacket, 32> batch{};
    size_t total = 0;
    for (;;) {
        const size_t n = rx_event_ring_.pop_span(batch);
        if (n == 0) break;
        for (size_t i = 0; i < n; ++i) {
            apply_packet_to_ui_tables(batch[i], on_station_gps);
        }
        total += n;
    }
    return total;
}

void AprsEngine::clear_logs() noexcept {
    packet_log_.clear();
    stations_.clear();
    chat_log_.clear();
}

void AprsEngine::network_worker_loop() {
    hamshack::ensure_winsock();

    hs_socket_t sock = hamshack::HS_INVALID_SOCKET;
    auto close_sock = [&]() noexcept {
        if (hamshack::hs_sock_valid(sock)) {
            hamshack::hs_shutdown_both(sock);
            hamshack::hs_close_socket(sock);
            sock = hamshack::HS_INVALID_SOCKET;
        }
        tcp_connected_.store(false, std::memory_order_release);
    };

    std::array<uint8_t, 1024> rx_buf{};
    AprsKissWireFrame out_frame{};
    uint64_t next_retry_ms = 0;

    while (worker_running_.load(std::memory_order_acquire)) {
        if (!hamshack::hs_sock_valid(sock)) {
            const uint64_t now = now_epoch_ms();
            if (now < next_retry_ms) {
                std::this_thread::sleep_for(std::chrono::milliseconds(40));
                continue;
            }
            next_retry_ms = now + 2000;

            addrinfo hints{};
            hints.ai_family   = AF_UNSPEC;
            hints.ai_socktype = SOCK_STREAM;
            hints.ai_protocol = IPPROTO_TCP;

            std::array<char, 16> port_s{};
            std::snprintf(port_s.data(), port_s.size(), "%u", static_cast<unsigned>(kiss_port_));

            addrinfo* res = nullptr;
            if (::getaddrinfo(kiss_host_.c_str(), port_s.data(), &hints, &res) != 0 || !res) {
                if (res) ::freeaddrinfo(res);
                continue;
            }

            // RAII guard for addrinfo linked list
            struct AddrInfoGuard {
                addrinfo* ptr;
                ~AddrInfoGuard() { if (ptr) ::freeaddrinfo(ptr); }
            } guard{res};

            hs_socket_t candidate = ::socket(res->ai_family, res->ai_socktype, res->ai_protocol);
            if (!hamshack::hs_sock_valid(candidate)) {
                continue;
            }
            hamshack::hs_set_nonblocking(candidate);
            const int rc = ::connect(candidate, res->ai_addr, static_cast<int>(res->ai_addrlen));
            bool connected_ok = (rc == 0);

            if (!connected_ok) {
#ifdef _WIN32
                const int err = ::WSAGetLastError();
                const bool in_prog = (err == WSAEWOULDBLOCK || err == WSAEINPROGRESS);
#else
                const bool in_prog = (errno == EINPROGRESS || errno == EWOULDBLOCK);
#endif
                if (in_prog) {
                    fd_set wfds;
                    FD_ZERO(&wfds);
                    FD_SET(candidate, &wfds);
                    timeval tv{};
                    tv.tv_sec  = 0;
                    tv.tv_usec = 250000; // 250 ms non-blocking connect poll
                    const int sel = ::select(static_cast<int>(candidate) + 1, nullptr, &wfds, nullptr, &tv);
                    if (sel > 0) {
                        int so_err = 0;
#ifdef _WIN32
                        int slen = sizeof(so_err);
                        ::getsockopt(candidate, SOL_SOCKET, SO_ERROR, reinterpret_cast<char*>(&so_err), &slen);
#else
                        socklen_t slen = sizeof(so_err);
                        ::getsockopt(candidate, SOL_SOCKET, SO_ERROR, &so_err, &slen);
#endif
                        connected_ok = (so_err == 0);
                    }
                }
            }

            if (!connected_ok) {
                hamshack::hs_close_socket(candidate);
                continue;
            }

            sock           = candidate;
            kiss_rx_len_   = 0;
            kiss_in_esc_   = false;
            kiss_in_frame_ = false;
            tcp_connected_.store(true, std::memory_order_release);
        }

        // 1. Drain outbound KISS TX ring non-blockingly
        while (tx_wire_ring_.try_pop(out_frame)) {
            size_t sent_total = 0;
            while (sent_total < out_frame.length && worker_running_.load(std::memory_order_acquire)) {
#ifdef _WIN32
                const int n = ::send(sock,
                                     reinterpret_cast<const char*>(out_frame.bytes.data() + sent_total),
                                     static_cast<int>(out_frame.length - sent_total), 0);
#else
                const ssize_t n = ::send(sock,
                                         out_frame.bytes.data() + sent_total,
                                         out_frame.length - sent_total,
#ifdef MSG_NOSIGNAL
                                         MSG_NOSIGNAL
#else
                                         0
#endif
                                         );
#endif
                if (n > 0) {
                    sent_total += static_cast<size_t>(n);
                } else {
#ifdef _WIN32
                    const int err = ::WSAGetLastError();
                    if (err == WSAEWOULDBLOCK) { std::this_thread::sleep_for(std::chrono::milliseconds(2)); continue; }
#else
                    if (errno == EAGAIN || errno == EWOULDBLOCK) { std::this_thread::sleep_for(std::chrono::milliseconds(2)); continue; }
#endif
                    close_sock();
                    break;
                }
            }
            if (!hamshack::hs_sock_valid(sock)) break;
        }

        if (!hamshack::hs_sock_valid(sock)) continue;

        // 2. Poll inbound KISS socket (25 ms non-blocking wait)
        const int pr = hamshack::hs_poll_read(sock, 25);
        if (pr < 0) {
            close_sock();
            continue;
        }
        if (pr > 0) {
#ifdef _WIN32
            const int n = ::recv(sock, reinterpret_cast<char*>(rx_buf.data()), static_cast<int>(rx_buf.size()), 0);
#else
            const ssize_t n = ::recv(sock, rx_buf.data(), rx_buf.size(), 0);
#endif
            if (n > 0) {
                process_kiss_chunk(std::span<const uint8_t>{rx_buf.data(), static_cast<size_t>(n)});
            } else if (n == 0) {
                close_sock();
            } else {
#ifdef _WIN32
                const int err = ::WSAGetLastError();
                if (err != WSAEWOULDBLOCK) close_sock();
#else
                if (errno != EAGAIN && errno != EWOULDBLOCK) close_sock();
#endif
            }
        }
    }

    close_sock();
}

} // namespace hamshack::carbon
