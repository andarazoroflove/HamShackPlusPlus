#pragma once
// ============================================================================
// AprsEngine.hpp — Carbon Engine AX.25 / APRS 1200-Baud KISS-over-TCP Engine
// Connects non-blockingly to local Dire Wolf KISS TNC (127.0.0.1:8001 default),
// decodes AX.25 UI frames & APRS 1.01 payloads (Uncompressed, Base-91, Mic-E,
// 2-Way Text Messages + Auto-ACK), forwards GPS coordinates to the shared Map
// Engine, and formats outbound KISS frames via lock-free SPSC queues.
// ============================================================================

#include "AudioRouter.hpp"

#include <array>
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <span>
#include <string>
#include <string_view>
#include <thread>
#include <vector>

namespace hamshack::carbon {

inline constexpr uint8_t kKissFend  = 0xC0;
inline constexpr uint8_t kKissFesc  = 0xDB;
inline constexpr uint8_t kKissTfend = 0xDC;
inline constexpr uint8_t kKissTfesc = 0xDD;

enum class AprsPacketKind : uint8_t {
    Unknown      = 0,
    Position     = 1,
    MicE         = 2,
    TextMessage  = 3,
    MessageAck   = 4,
    MessageRej   = 5,
    Status       = 6,
    Telemetry    = 7
};

// Fixed-layout POD struct transferred across LockFreeSpscRing (zero heap allocations!)
struct AprsDecodedPacket {
    uint64_t              rx_epoch_ms   = 0;
    AprsPacketKind        kind          = AprsPacketKind::Unknown;
    bool                  is_tx         = false;
    bool                  has_position  = false;
    bool                  has_altitude  = false;
    bool                  has_kinematics= false;

    std::array<char, 12>  utc_hms{};       // "HH:MM:SS"
    std::array<char, 12>  source_call{};   // Base callsign e.g. "W7ABC"
    uint8_t               source_ssid   = 0;
    std::array<char, 16>  full_source{};   // Formatted "W7ABC-9"
    std::array<char, 16>  dest_call{};     // Formatted "APRS" or Mic-E dest
    std::array<char, 64>  digi_path{};     // "WIDE1-1*,WIDE2-1"

    double                latitude      = 0.0;
    double                longitude     = 0.0;
    float                 altitude_ft   = 0.0f;
    float                 speed_knots   = 0.0f;
    float                 course_deg    = 0.0f;
    char                  symbol_table  = '/';
    char                  symbol_code   = '-';

    std::array<char, 160> comment{};       // APRS comment / status text
    std::array<char, 16>  msg_addressee{}; // For 2-way APRS messaging
    std::array<char, 96>  msg_text{};
    std::array<char, 8>   msg_id{};
    std::array<char, 220> tnc2_raw{};      // "W7ABC-9>APRS,WIDE1-1:!..."
};

// Fixed-capacity outbound KISS wire frame for lock-free GUI -> TCP thread handoff
struct AprsKissWireFrame {
    std::array<uint8_t, 512> bytes{};
    size_t                   length = 0;
};

// Aggregated live station table row for Shared Map Engine & APRS Station List
struct AprsStationEntry {
    std::string callsign_ssid;  // e.g. "W7ABC-9"
    std::string base_callsign;  // e.g. "W7ABC"
    uint8_t     ssid          = 0;
    bool        has_position  = false;
    double      latitude      = 0.0;
    double      longitude     = 0.0;
    float       altitude_ft   = 0.0f;
    float       speed_knots   = 0.0f;
    float       course_deg    = 0.0f;
    char        symbol_table  = '/';
    char        symbol_code   = '-';
    std::string last_comment;
    std::string last_path;
    std::string last_utc;
    uint32_t    packet_count  = 0;
};

// 2-Way APRS Conversation Entry
struct AprsChatEntry {
    std::string utc_hms;
    std::string peer_call;     // Remote station e.g. "W7ABC-7"
    std::string sender_call;
    std::string text;
    std::string msg_id;
    bool        outbound = false;
    bool        acked    = false;
    bool        rejected = false;
};

// ============================================================================
// AprsEngine — Lock-Free KISS-over-TCP Client, AX.25 Codec & APRS Router
// ============================================================================
class AprsEngine {
public:
    static constexpr size_t kMaxPacketLog = 400;
    static constexpr size_t kMaxStations  = 256;
    static constexpr size_t kMaxChatItems = 400;

    AprsEngine();
    ~AprsEngine();

    AprsEngine(const AprsEngine&) = delete;
    AprsEngine& operator=(const AprsEngine&) = delete;

    // Station & KISS endpoint configuration
    void set_station_identity(std::string_view callsign, uint8_t ssid, std::string_view digi_path = "WIDE1-1,WIDE2-1");
    void set_kiss_endpoint(std::string_view host, uint16_t port);

    [[nodiscard]] std::string host() const { return kiss_host_; }
    [[nodiscard]] uint16_t    port() const noexcept { return kiss_port_; }
    [[nodiscard]] uint8_t     station_ssid() const noexcept { return my_ssid_; }
    [[nodiscard]] std::string full_station_call() const;
    [[nodiscard]] std::string digi_path() const { return digi_path_; }

    // Non-blocking KISS-over-TCP lifecycle (spawns/stops background socket worker)
    void connect_kiss();
    void disconnect_kiss();
    [[nodiscard]] bool is_connected() const noexcept { return tcp_connected_.load(std::memory_order_acquire); }
    [[nodiscard]] bool is_running()   const noexcept { return worker_running_.load(std::memory_order_acquire); }
    [[nodiscard]] uint64_t rx_frame_count() const noexcept { return rx_frames_.load(std::memory_order_relaxed); }
    [[nodiscard]] uint64_t tx_frame_count() const noexcept { return tx_frames_.load(std::memory_order_relaxed); }

    // Drains the lock-free SPSC packet queue on the GUI thread (zero locks, deterministic!)
    // Invokes optional map callback for each newly decoded GPS station.
    size_t poll_ui_events(const std::function<void(const AprsStationEntry&)>& on_station_gps = nullptr);

    // Outbound KISS TX Generators (pushes pre-formatted KISS frames lock-free to TCP thread)
    bool send_position_beacon(double lat_deg, double lon_deg,
                              char sym_table = '/', char sym_code = '-',
                              std::string_view comment = "HamShack++ APRS");
    bool send_text_message(std::string_view to_call_ssid, std::string_view message_text);
    bool send_raw_aprs_payload(std::string_view info_payload);

    // Inject raw KISS frame bytes directly into the deframer (for unit tests & loopback verification)
    size_t inject_kiss_bytes(std::span<const uint8_t> kiss_stream) noexcept;

    // Pure functional static codecs (exposed for zero-overhead testing & external reuse)
    static size_t kiss_escape_frame(std::span<const uint8_t> ax25_payload,
                                    std::span<uint8_t> out_kiss,
                                    uint8_t port_cmd = 0x00) noexcept;

    static size_t encode_ax25_ui_frame(std::string_view src_call_ssid,
                                       std::string_view dst_call_ssid,
                                       std::string_view digi_path_csv,
                                       std::string_view info_field,
                                       std::span<uint8_t> out_ax25) noexcept;

    static bool decode_ax25_ui_frame(std::span<const uint8_t> ax25_bytes,
                                     AprsDecodedPacket& out_pkt) noexcept;

    static void parse_aprs_info_field(std::string_view info,
                                      std::string_view dest_call,
                                      AprsDecodedPacket& pkt) noexcept;

    static std::string format_uncompressed_position(double lat_deg, double lon_deg,
                                                    char sym_table, char sym_code,
                                                    std::string_view comment);

    // Read-only accessors for decoupled UI rendering (GUI thread owns these containers)
    [[nodiscard]] const std::vector<AprsDecodedPacket>& packet_log() const noexcept { return packet_log_; }
    [[nodiscard]] const std::vector<AprsStationEntry>&  stations()   const noexcept { return stations_; }
    [[nodiscard]] const std::vector<AprsChatEntry>&     chat_log()   const noexcept { return chat_log_; }
    void clear_logs() noexcept;

private:
    void network_worker_loop();
    void process_kiss_chunk(std::span<const uint8_t> bytes) noexcept;
    void handle_complete_ax25_frame(std::span<const uint8_t> ax25) noexcept;
    bool enqueue_outbound_ax25(std::string_view info_field, std::string_view custom_src = {}) noexcept;
    void apply_packet_to_ui_tables(const AprsDecodedPacket& pkt,
                                   const std::function<void(const AprsStationEntry&)>& on_station_gps);

    std::string kiss_host_ = "127.0.0.1";
    uint16_t    kiss_port_ = 8001;
    std::string my_call_   = "W1AW";
    uint8_t     my_ssid_   = 9;
    std::string digi_path_ = "WIDE1-1,WIDE2-1";
    uint32_t    next_msg_seq_ = 1;

    std::atomic<bool>     worker_running_{false};
    std::atomic<bool>     tcp_connected_{false};
    std::atomic<uint64_t> rx_frames_{0};
    std::atomic<uint64_t> tx_frames_{0};
    std::thread           net_thread_;

    // Pre-allocated KISS deframer state machine (owned by network thread / inject_kiss_bytes)
    std::array<uint8_t, 2048> kiss_rx_accum_{};
    size_t                    kiss_rx_len_  = 0;
    bool                      kiss_in_esc_  = false;
    bool                      kiss_in_frame_= false;

    // Lock-free SPSC rings decoupling TCP socket thread <-> GUI render thread
    LockFreeSpscRing<AprsDecodedPacket, 256> rx_event_ring_{};
    LockFreeSpscRing<AprsKissWireFrame, 64>  tx_wire_ring_{};

    // GUI-thread owned state tables (pre-reserved in constructor to prevent reallocations)
    std::vector<AprsDecodedPacket> packet_log_;
    std::vector<AprsStationEntry>  stations_;
    std::vector<AprsChatEntry>     chat_log_;
};

} // namespace hamshack::carbon
