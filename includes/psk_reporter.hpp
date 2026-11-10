#pragma once
// ============================================================================
// psk_reporter.hpp — Native IPFIX UDP Client for PSK Reporter (RFC 5101)
// Pushes decoded FT8, FT4, and JS8Call spots to https://pskreporter.info/
// Collector host: report.pskreporter.info:4739 (Enterprise PEN 30351)
// ============================================================================

#include <string>
#include <vector>
#include <deque>
#include <unordered_map>
#include <mutex>
#include <thread>
#include <atomic>
#include <chrono>
#include <cstring>
#include <ctime>
#include <random>
#include "../src/hamshack_platform.hpp"
#include "../src/hamshack_config.hpp"

namespace hamshack {

class PskReporterClient {
public:
    struct Spot {
        std::string callsign;
        std::string grid;
        uint64_t freq_hz = 0;
        int snr = 0;
        std::string mode = "FT8";
        uint32_t timestamp = 0;
    };

    std::atomic<bool> running{false};
    std::atomic<uint64_t> total_spots_sent{0};
    std::atomic<uint64_t> total_packets_sent{0};
    std::string last_report_status = "Waiting for decoded spots...";
    std::string last_report_time;

    static PskReporterClient& get_instance() {
        static PskReporterClient instance;
        return instance;
    }

    PskReporterClient() {
        std::random_device rd;
        observation_id_ = rd();
        start();
    }

    ~PskReporterClient() {
        stop();
    }

    void start() {
        if (running.exchange(true)) return;
        worker_thread_ = std::thread(&PskReporterClient::worker_loop, this);
    }

    void stop() {
        if (!running.exchange(false)) return;
        if (worker_thread_.joinable()) worker_thread_.join();
    }

    bool is_enabled() const {
        return get_app_config().web_reporting_enabled;
    }

    void set_enabled(bool enabled) {
        auto& cfg = get_app_config();
        cfg.web_reporting_enabled = enabled;
        cfg.save();
    }

    std::string status_summary() const {
        if (!get_app_config().web_reporting_enabled) {
            return "DISABLED (Web reporting paused)";
        }
        std::string s = last_report_status;
        s += " [" + std::to_string(total_spots_sent.load()) + " spots, " +
             std::to_string(total_packets_sent.load()) + " pkts]";
        return s;
    }

    void post_spot(const std::string& call, const std::string& grid, uint64_t freq_hz, int snr_db, const std::string& mode) {
        auto& cfg = get_app_config();
        if (!cfg.web_reporting_enabled) return;
        if (call.empty() || call == "CQ" || call == "QRZ" || call == "DE" || call == "--") return;
        if (call == cfg.hf_callsign) return; // Don't report self
        if (freq_hz == 0) return;

        uint32_t now = static_cast<uint32_t>(std::time(nullptr));

        // Deduplicate spots within 300 seconds (5 minutes) for same call/mode/frequency band
        std::string dedupe_key = call + "_" + mode + "_" + std::to_string(freq_hz / 500);
        {
            std::lock_guard<std::mutex> lk(mtx_);
            auto it = spot_cache_.find(dedupe_key);
            if (it != spot_cache_.end() && (now - it->second) < 300) {
                return; // Already spotted recently
            }
            spot_cache_[dedupe_key] = now;

            // Trim deduplication cache if too large
            if (spot_cache_.size() > 4096) {
                for (auto cit = spot_cache_.begin(); cit != spot_cache_.end();) {
                    if (now - cit->second > 600) cit = spot_cache_.erase(cit);
                    else ++cit;
                }
            }

            Spot s;
            s.callsign = call;
            s.grid = grid;
            s.freq_hz = freq_hz;
            s.snr = snr_db;
            s.mode = mode;
            s.timestamp = now;
            pending_spots_.push_back(s);
            if (pending_spots_.size() > 1024) pending_spots_.pop_front();
        }
    }

private:
    std::mutex mtx_;
    std::thread worker_thread_;
    std::deque<Spot> pending_spots_;
    std::unordered_map<std::string, uint32_t> spot_cache_;
    uint32_t sequence_number_ = 0;
    uint32_t observation_id_ = 0;
    int descriptor_countdown_ = 0;

    static void write_u8(std::vector<uint8_t>& buf, uint8_t v) {
        buf.push_back(v);
    }

    static void write_u16(std::vector<uint8_t>& buf, uint16_t v) {
        buf.push_back(static_cast<uint8_t>((v >> 8) & 0xFF));
        buf.push_back(static_cast<uint8_t>(v & 0xFF));
    }

    static void write_u32(std::vector<uint8_t>& buf, uint32_t v) {
        buf.push_back(static_cast<uint8_t>((v >> 24) & 0xFF));
        buf.push_back(static_cast<uint8_t>((v >> 16) & 0xFF));
        buf.push_back(static_cast<uint8_t>((v >> 8) & 0xFF));
        buf.push_back(static_cast<uint8_t>(v & 0xFF));
    }

    static void write_utf8_string(std::vector<uint8_t>& buf, const std::string& str, size_t max_len) {
        size_t len = std::min(str.size(), max_len);
        buf.push_back(static_cast<uint8_t>(len));
        for (size_t i = 0; i < len; ++i) {
            buf.push_back(static_cast<uint8_t>(str[i]));
        }
    }

    static void pad_to_4(std::vector<uint8_t>& buf) {
        size_t pad = (4 - (buf.size() % 4)) % 4;
        for (size_t i = 0; i < pad; ++i) buf.push_back(0);
    }

    static void patch_u16(std::vector<uint8_t>& buf, size_t offset, uint16_t val) {
        if (offset + 1 < buf.size()) {
            buf[offset] = static_cast<uint8_t>((val >> 8) & 0xFF);
            buf[offset + 1] = static_cast<uint8_t>(val & 0xFF);
        }
    }

    std::vector<uint8_t> build_descriptor_sets() {
        std::vector<uint8_t> sets;

        // Template 0x50e3: Sender Information Template
        {
            size_t start = sets.size();
            write_u16(sets, 2);      // Set ID = 2 (Template Set)
            write_u16(sets, 0);      // Length placeholder
            write_u16(sets, 0x50e3); // Template ID
            write_u16(sets, 7);      // 7 fields

            // 1. senderCallsign (enterprise)
            write_u16(sets, 0x8001); write_u16(sets, 0xFFFF); write_u32(sets, 30351);
            // 2. frequency (enterprise, 5-byte integer)
            write_u16(sets, 0x8005); write_u16(sets, 5);      write_u32(sets, 30351);
            // 3. sNR (enterprise, 1-byte signed)
            write_u16(sets, 0x8006); write_u16(sets, 1);      write_u32(sets, 30351);
            // 4. mode (enterprise)
            write_u16(sets, 0x800a); write_u16(sets, 0xFFFF); write_u32(sets, 30351);
            // 5. senderLocator (enterprise)
            write_u16(sets, 0x8003); write_u16(sets, 0xFFFF); write_u32(sets, 30351);
            // 6. informationSource (enterprise, 1-byte)
            write_u16(sets, 0x800b); write_u16(sets, 1);      write_u32(sets, 30351);
            // 7. dateTimeSeconds (IETF 150, 4 bytes)
            write_u16(sets, 150);    write_u16(sets, 4);

            pad_to_4(sets);
            patch_u16(sets, start + 2, static_cast<uint16_t>(sets.size() - start));
        }

        // Template 0x50e2: Receiver Options Template
        {
            size_t start = sets.size();
            write_u16(sets, 3);      // Set ID = 3 (Options Template Set)
            write_u16(sets, 0);      // Length placeholder
            write_u16(sets, 0x50e2); // Template ID
            write_u16(sets, 5);      // 5 fields total
            write_u16(sets, 1);      // 1 scope field count

            // Scope field 1: receiverCallsign
            write_u16(sets, 0x8002); write_u16(sets, 0xFFFF); write_u32(sets, 30351);
            // Non-scope 1: receiverLocator
            write_u16(sets, 0x8004); write_u16(sets, 0xFFFF); write_u32(sets, 30351);
            // Non-scope 2: decodingSoftware
            write_u16(sets, 0x8008); write_u16(sets, 0xFFFF); write_u32(sets, 30351);
            // Non-scope 3: antennaInformation
            write_u16(sets, 0x8009); write_u16(sets, 0xFFFF); write_u32(sets, 30351);
            // Non-scope 4: rigInformation
            write_u16(sets, 0x800d); write_u16(sets, 0xFFFF); write_u32(sets, 30351);

            pad_to_4(sets);
            patch_u16(sets, start + 2, static_cast<uint16_t>(sets.size() - start));
        }

        return sets;
    }

    std::vector<uint8_t> build_receiver_set(const std::string& my_call, const std::string& my_grid) {
        std::vector<uint8_t> set;
        write_u16(set, 0x50e2); // Set ID = Receiver Template 0x50e2
        write_u16(set, 0);      // Length placeholder

        write_utf8_string(set, my_call, 32);
        write_utf8_string(set, my_grid, 16);
        write_utf8_string(set, "HamShack++ Unified Suite v1.2", 80);
        write_utf8_string(set, "Dipole", 128);
        write_utf8_string(set, "Xiegu G90 HF Transceiver", 128);

        pad_to_4(set);
        patch_u16(set, 2, static_cast<uint16_t>(set.size()));
        return set;
    }

    std::vector<uint8_t> build_sender_set(const std::vector<Spot>& spots) {
        std::vector<uint8_t> set;
        write_u16(set, 0x50e3); // Set ID = Sender Template 0x50e3
        write_u16(set, 0);      // Length placeholder

        for (const auto& s : spots) {
            // Field 1: senderCallsign
            write_utf8_string(set, s.callsign, 32);
            // Field 2: frequency (5 bytes big-endian)
            set.push_back(static_cast<uint8_t>((s.freq_hz >> 32) & 0xFF));
            set.push_back(static_cast<uint8_t>((s.freq_hz >> 24) & 0xFF));
            set.push_back(static_cast<uint8_t>((s.freq_hz >> 16) & 0xFF));
            set.push_back(static_cast<uint8_t>((s.freq_hz >> 8) & 0xFF));
            set.push_back(static_cast<uint8_t>(s.freq_hz & 0xFF));
            // Field 3: sNR (1 byte signed)
            set.push_back(static_cast<uint8_t>(static_cast<int8_t>(std::clamp(s.snr, -128, 127))));
            // Field 4: mode
            write_utf8_string(set, s.mode, 16);
            // Field 5: senderLocator
            write_utf8_string(set, s.grid, 16);
            // Field 6: informationSource (1 byte = local reception)
            set.push_back(1);
            // Field 7: dateTimeSeconds (4 bytes)
            write_u32(set, s.timestamp);
        }

        pad_to_4(set);
        patch_u16(set, 2, static_cast<uint16_t>(set.size()));
        return set;
    }

    std::vector<uint8_t> build_ipfix_message(const std::vector<uint8_t>& payload_sets, uint32_t export_time) {
        std::vector<uint8_t> msg;
        // 16-byte IPFIX Header
        write_u16(msg, 10); // Version 10 (IPFIX)
        write_u16(msg, 0);  // Total length placeholder
        write_u32(msg, export_time);
        write_u32(msg, sequence_number_++);
        write_u32(msg, observation_id_);

        msg.insert(msg.end(), payload_sets.begin(), payload_sets.end());
        pad_to_4(msg);
        patch_u16(msg, 2, static_cast<uint16_t>(msg.size()));
        return msg;
    }

    void worker_loop() {
        while (running) {
            std::this_thread::sleep_for(std::chrono::milliseconds(5000));
            if (!running) break;

            auto& cfg = get_app_config();
            if (!cfg.web_reporting_enabled) continue;

            std::vector<Spot> to_send;
            {
                std::lock_guard<std::mutex> lk(mtx_);
                if (pending_spots_.empty() && descriptor_countdown_ > 0) {
                    continue;
                }
                size_t batch = std::min<size_t>(pending_spots_.size(), 50);
                for (size_t i = 0; i < batch; ++i) {
                    to_send.push_back(pending_spots_.front());
                    pending_spots_.pop_front();
                }
            }

            // Resolve report.pskreporter.info:4739
            ensure_winsock();
            struct addrinfo hints{}, *res = nullptr;
            hints.ai_family = AF_INET; // IPv4 UDP
            hints.ai_socktype = SOCK_DGRAM;
            if (getaddrinfo("report.pskreporter.info", "4739", &hints, &res) != 0 || !res) {
                last_report_status = "DNS resolution failed for report.pskreporter.info";
                if (res) freeaddrinfo(res);
                continue;
            }

            hs_socket_t sock = ::socket(res->ai_family, res->ai_socktype, res->ai_protocol);
            if (!hs_sock_valid(sock)) {
                freeaddrinfo(res);
                continue;
            }

            uint32_t now = static_cast<uint32_t>(std::time(nullptr));
            std::vector<uint8_t> payload_sets;

            // Send descriptors every 10 report batches or on startup
            if (descriptor_countdown_ <= 0) {
                std::vector<uint8_t> desc = build_descriptor_sets();
                payload_sets.insert(payload_sets.end(), desc.begin(), desc.end());
                descriptor_countdown_ = 10;
            } else {
                --descriptor_countdown_;
            }

            // Append receiver information
            std::vector<uint8_t> rcv = build_receiver_set(cfg.hf_callsign, cfg.hf_grid);
            payload_sets.insert(payload_sets.end(), rcv.begin(), rcv.end());

            // Append spots if any
            if (!to_send.empty()) {
                std::vector<uint8_t> snd = build_sender_set(to_send);
                payload_sets.insert(payload_sets.end(), snd.begin(), snd.end());
            }

            std::vector<uint8_t> dgram = build_ipfix_message(payload_sets, now);

            hs_ssize_t sent = ::sendto(sock, reinterpret_cast<const char*>(dgram.data()),
                                       static_cast<int>(dgram.size()), 0,
                                       res->ai_addr, static_cast<int>(res->ai_addrlen));
            hs_close_socket(sock);
            freeaddrinfo(res);

            if (sent > 0) {
                total_spots_sent += to_send.size();
                total_packets_sent++;
                std::time_t t = std::time(nullptr);
                std::tm tm_buf{};
                hs_localtime(&t, &tm_buf);
                char t_buf[32];
                std::snprintf(t_buf, sizeof(t_buf), "%02d:%02d:%02d", tm_buf.tm_hour, tm_buf.tm_min, tm_buf.tm_sec);
                last_report_time = t_buf;
                last_report_status = "Online • " + std::to_string(total_spots_sent.load()) + " spots uploaded (" +
                                     std::to_string(to_send.size()) + " in last packet at " + last_report_time + ")";
            } else {
                last_report_status = "UDP send failed to report.pskreporter.info:4739";
            }
        }
    }
};

inline PskReporterClient& get_psk_reporter() {
    return PskReporterClient::get_instance();
}

} // namespace hamshack
