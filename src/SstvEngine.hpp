#pragma once
// ============================================================================
// SstvEngine.hpp — Carbon Engine Multi-Mode SSTV Transceiver (RX DSP + TX Modulator)
// Supports 14 Modes:
//   - Martin 1 (VIS 44), Martin 2 (VIS 40)
//   - Scottie 1 (VIS 60), Scottie 2 (VIS 56)
//   - Robot B&W 8 (VIS 2), 12 (VIS 6), 24 (VIS 10), 36 (VIS 14), 72 (VIS 18)
//   - Robot Color 12 (VIS 0), 24 (VIS 4), 36 (VIS 8), 72 (VIS 12)
//   - PD120 (VIS 95, 640x496 dual-line YCrCb)
// Strictly adheres to Carbon Engine Rules:
//   - Zero heap allocations in DSP demodulator, waterfall FFT, or scanline loops
//   - Pre-allocated 64-byte aligned contiguous flat arrays (std::array)
//   - Lock-free atomic double-buffering for tear-free progressive GUI rendering
// ============================================================================

#include "AudioRouter.hpp"

#include <array>
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <span>
#include <string>
#include <string_view>
#include <thread>
#include <vector>

namespace hamshack::carbon {

enum class SstvMode : uint8_t {
    Martin1      = 0,
    Martin2      = 1,
    Scottie1     = 2,
    Scottie2     = 3,
    RobotBw8     = 4,
    RobotBw12    = 5,
    RobotBw24    = 6,
    RobotBw36    = 7,
    RobotBw72    = 8,
    RobotColor12 = 9,
    RobotColor24 = 10,
    RobotColor36 = 11,
    RobotColor72 = 12,
    Pd120        = 13,
    Count        = 14
};

enum class SstvColorEncoding : uint8_t {
    GbrSequential   = 0, // Martin 1/2
    GbrMidSync      = 1, // Scottie 1/2 (sync pulse precedes Red scan)
    GrayscaleBw     = 2, // Robot B&W 8/12/24/36/72 (pure luminance, no chroma subcarriers)
    RobotYuv420     = 3, // Robot Color 12/36 (even line Y + R-Y, odd line Y + B-Y)
    RobotYuv422     = 4, // Robot Color 24/72 (every line Y + R-Y + B-Y)
    PdDualLineYuv   = 5  // PD120 (2 lines per frame segment: Sync, Porch, Y0, R-Y, B-Y, Y1)
};

struct SstvModeSpec {
    SstvMode          mode;
    uint8_t           vis_code;
    std::string_view  name;
    std::string_view  short_name;
    uint16_t          width;
    uint16_t          height;
    SstvColorEncoding encoding;
    double            sync_ms;
    double            porch_ms;
    double            sep_ms;
    double            chroma_porch_ms;
    double            luma_scan_ms;    // Y scan (or G/B/R scan for Martin/Scottie)
    double            chroma_scan_ms;  // Cr/Cb scan (0 for B&W / GBR)
    double            line_period_ms;  // Nominal duration per transmitted unit (1 line, or 2 lines for PD120)
};

inline constexpr size_t kNumSstvModes = static_cast<size_t>(SstvMode::Count);

inline constexpr std::array<SstvModeSpec, kNumSstvModes> kSstvModes{{
    // Martin 1 & 2 (320x256 GBR)
    {SstvMode::Martin1,      44, "Martin 1 (320x256 Color)",      "Martin 1",    320, 256, SstvColorEncoding::GbrSequential, 4.862, 0.572, 0.572, 0.0,   146.432,   0.0,   446.446},
    {SstvMode::Martin2,      40, "Martin 2 (320x256 Fast)",       "Martin 2",    320, 256, SstvColorEncoding::GbrSequential, 4.862, 0.572, 0.572, 0.0,    73.216,   0.0,   226.798},
    // Scottie 1 & 2 (320x256 GBR Mid-Sync)
    {SstvMode::Scottie1,     60, "Scottie 1 (320x256 Color)",     "Scottie 1",   320, 256, SstvColorEncoding::GbrMidSync,    9.000, 1.500, 1.500, 0.0,   138.240,   0.0,   428.220},
    {SstvMode::Scottie2,     56, "Scottie 2 (320x256 Fast)",      "Scottie 2",   320, 256, SstvColorEncoding::GbrMidSync,    9.000, 1.500, 1.500, 0.0,    88.064,   0.0,   277.692},
    // Robot B&W 8, 12, 24, 36, 72 (Direct Luminance Scanlines — Zero Chroma Subcarriers)
    {SstvMode::RobotBw8,      2, "Robot B&W 8 (160x120 Mono)",    "Robot BW8",   160, 120, SstvColorEncoding::GrayscaleBw,   6.000, 1.500, 0.000, 0.0,    59.167,   0.0,    66.667},
    {SstvMode::RobotBw12,     6, "Robot B&W 12 (160x120 Mono)",   "Robot BW12",  160, 120, SstvColorEncoding::GrayscaleBw,   7.000, 1.500, 0.000, 0.0,    91.500,   0.0,   100.000},
    {SstvMode::RobotBw24,    10, "Robot B&W 24 (320x240 Mono)",   "Robot BW24",  320, 240, SstvColorEncoding::GrayscaleBw,   9.000, 3.000, 0.000, 0.0,    88.000,   0.0,   100.000},
    {SstvMode::RobotBw36,    14, "Robot B&W 36 (320x240 Mono)",   "Robot BW36",  320, 240, SstvColorEncoding::GrayscaleBw,   9.000, 3.000, 0.000, 0.0,   138.000,   0.0,   150.000},
    {SstvMode::RobotBw72,    18, "Robot B&W 72 (320x240 Mono)",   "Robot BW72",  320, 240, SstvColorEncoding::GrayscaleBw,   9.000, 3.000, 0.000, 0.0,   288.000,   0.0,   300.000},
    // Robot Color 12, 24, 36, 72 (YCrCb 4:2:0 and 4:2:2)
    {SstvMode::RobotColor12,  0, "Robot Color 12 (160x120 YUV)",  "Robot C12",   160, 120, SstvColorEncoding::RobotYuv420,   7.000, 1.000, 4.500, 1.500,  60.000,  26.000, 100.000},
    {SstvMode::RobotColor24,  4, "Robot Color 24 (160x120 YUV)",  "Robot C24",   160, 120, SstvColorEncoding::RobotYuv422,   9.000, 3.000, 4.500, 1.500,  88.000,  44.000, 200.000},
    {SstvMode::RobotColor36,  8, "Robot Color 36 (320x240 YUV)",  "Robot C36",   320, 240, SstvColorEncoding::RobotYuv420,   9.000, 3.000, 4.500, 1.500,  88.000,  44.000, 150.000},
    {SstvMode::RobotColor72, 12, "Robot Color 72 (320x240 YUV)",  "Robot C72",   320, 240, SstvColorEncoding::RobotYuv422,   9.000, 3.000, 4.500, 1.500, 138.000,  69.000, 300.000},
    // PD120 (640x496 Dual-Line YCrCb)
    {SstvMode::Pd120,        95, "PD120 (640x496 Hi-Res Color)",  "PD120",       640, 496, SstvColorEncoding::PdDualLineYuv,20.000, 2.080, 0.000, 0.000, 121.600, 121.600, 508.480}
}};

[[nodiscard]] constexpr const SstvModeSpec& sstv_mode_spec(SstvMode mode) noexcept {
    const size_t idx = static_cast<size_t>(mode);
    return kSstvModes[(idx < kNumSstvModes) ? idx : 0];
}

[[nodiscard]] constexpr bool find_sstv_mode_by_vis(uint8_t vis_code, SstvMode& out_mode) noexcept {
    for (const auto& spec : kSstvModes) {
        if (spec.vis_code == vis_code) {
            out_mode = spec.mode;
            return true;
        }
    }
    return false;
}

enum class SstvRxState : uint8_t {
    HuntingVis     = 0,
    DecodingVisBits= 1,
    ReceivingImage = 2
};

struct SstvTelemetry {
    SstvRxState rx_state            = SstvRxState::HuntingVis;
    SstvMode    active_rx_mode      = SstvMode::Martin1;
    SstvMode    tx_mode             = SstvMode::Martin1;
    bool        auto_vis_enabled    = true;
    bool        auto_save_png       = true;
    bool        tx_active           = false;
    bool        tx_dither_1bit      = false;
    uint16_t    rx_current_line     = 0;
    uint16_t    rx_total_lines      = 256;
    uint16_t    rx_width            = 320;
    uint16_t    rx_height           = 256;
    float       slant_ppm           = 0.0f;
    float       auto_slant_ppm      = 0.0f;
    float       inst_freq_hz        = 1500.0f;
    float       tx_progress         = 0.0f;
    uint32_t    rx_completed_images = 0;
    uint64_t    rx_surface_seq      = 0;
    uint64_t    wf_surface_seq      = 0;
    uint64_t    tx_preview_seq      = 0;
};

// ============================================================================
// SstvModulator — Stateless / Pre-Allocated 16-Bit PCM SSTV Waveform Generator
// Generates VIS pre-burst + mode-accurate scanlines with zero inner allocations.
// ============================================================================
class SstvModulator {
public:
    static constexpr size_t kChunkSamples = 4096;

    SstvModulator() noexcept = default;

    // Synthesizes a complete SSTV frame (VIS header + all scanlines) at sample_rate_hz
    // (48000 Hz or 44100 Hz for TX output; 12000 Hz supported for fast loopback tests).
    // Invokes emit_chunk(std::span<const int16_t>, bool is_first, bool is_last, float progress).
    // Returns false if emit_chunk returns false (abort requested).
    template <typename ChunkFn>
    static bool modulate_image(SstvMode mode,
                               std::span<const uint32_t> bgra_raster,
                               uint16_t raster_stride_px,
                               int sample_rate_hz,
                               ChunkFn&& emit_chunk) noexcept;

    // Convenience helper generating full frame into a pre-sized / single-reserve vector (for unit tests)
    static void synthesize_to_vector(SstvMode mode,
                                     std::span<const uint32_t> bgra_raster,
                                     uint16_t raster_stride_px,
                                     int sample_rate_hz,
                                     std::vector<int16_t>& out_pcm);
};

// ============================================================================
// SstvEngine — Real-Time Lock-Free SSTV RX Decoder + TX Transceiver Controller
// ============================================================================
class SstvEngine {
public:
    static constexpr uint16_t kMaxWidth       = 640;
    static constexpr uint16_t kMaxHeight      = 496;
    static constexpr size_t   kMaxPixels      = static_cast<size_t>(kMaxWidth) * static_cast<size_t>(kMaxHeight);
    static constexpr uint16_t kWaterfallW     = 256; // Covers 1000 Hz .. 2500 Hz cleanly
    static constexpr uint16_t kWaterfallH     = 112;
    static constexpr size_t   kFftSize        = 512; // 23.4375 Hz bin resolution @ 12 kHz

    explicit SstvEngine(AudioRouter& router);
    ~SstvEngine();

    SstvEngine(const SstvEngine&) = delete;
    SstvEngine& operator=(const SstvEngine&) = delete;

    // Start / stop real-time background RX DSP worker thread
    void start_worker();
    void stop_worker();

    // Feed 12 kHz mono float samples directly into RX DSP (for deterministic synchronous unit tests)
    void process_rx_samples_sync(std::span<const float> mono_12k) noexcept;

    // RX Controls
    void set_mode_override(bool auto_vis, SstvMode manual_mode) noexcept;
    void force_start_manual_rx(SstvMode mode) noexcept;
    void adjust_slant_ppm(float delta_ppm) noexcept;
    void set_slant_ppm(float ppm) noexcept;
    void clear_rx_canvas() noexcept;
    void set_auto_save_png(bool enabled) noexcept { auto_save_png_.store(enabled, std::memory_order_release); }
    void set_capture_directory(std::string_view dir);
    bool save_current_rx_png(std::string& out_saved_path) const;

    // TX Image Preparation & Controls
    void generate_default_test_card();
    bool load_tx_image_file(const std::wstring& path, std::string& out_error);
    bool set_tx_source_bgra(std::span<const uint32_t> pixels, uint16_t width, uint16_t height);
    void set_tx_mode(SstvMode mode);
    void set_tx_dither_1bit(bool enabled);
    void set_tx_overlay_text(std::string_view text);
    void rebuild_tx_preview();

    // Start / Abort asynchronous lock-free SSTV TX via AudioRouter (48 kHz 16-bit SPSC stream)
    bool start_transmit();
    void abort_transmit() noexcept;

    // Lock-free surface snapshots for zero-jank GPU bitmap upload on GUI thread
    [[nodiscard]] std::span<const uint32_t> active_rx_surface(uint16_t& out_w, uint16_t& out_h) const noexcept;
    [[nodiscard]] std::span<const uint32_t> waterfall_surface() const noexcept {
        return std::span<const uint32_t>{waterfall_bgra_.data(), waterfall_bgra_.size()};
    }
    [[nodiscard]] std::span<const float, kWaterfallW> spectrum_db() const noexcept {
        return std::span<const float, kWaterfallW>{spectrum_db_.data(), kWaterfallW};
    }
    [[nodiscard]] std::span<const uint32_t> tx_preview_surface(uint16_t& out_w, uint16_t& out_h) const noexcept;

    [[nodiscard]] SstvTelemetry telemetry() const noexcept;
    [[nodiscard]] std::string   last_saved_png_path() const { return last_saved_png_; }
    [[nodiscard]] std::string   tx_overlay_text() const     { return tx_overlay_text_; }

    // Zero-dependency RFC 1950/1951 uncompressed PNG writer (exposed for testing & manual save)
    static bool write_bgra_png(const std::string& filepath,
                               std::span<const uint32_t> bgra,
                               uint16_t width,
                               uint16_t height,
                               uint16_t stride_px);

private:
    void dsp_worker_loop();
    void tx_worker_loop(SstvMode mode, uint16_t w, uint16_t h);

    // Zero-allocation DSP stages (all buffers pre-allocated & 64-byte cache-line aligned)
    void demodulate_fm_block(std::span<const float> pcm, std::span<float> out_freq_hz) noexcept;
    void process_freq_sample(float freq_hz, float mag_sq) noexcept;
    void update_waterfall_fft(std::span<const float> pcm) noexcept;
    void begin_image_rx(SstvMode mode) noexcept;
    void decode_completed_line(uint16_t line_idx) noexcept;
    void publish_rx_scanline_to_front(uint16_t y_start, uint16_t num_rows) noexcept;
    void finalize_rx_image() noexcept;

    AudioRouter&   router_;
    AudioTapCursor rx_tap_cursor_{};

    std::atomic<bool> worker_running_{false};
    std::thread       dsp_thread_;

    std::atomic<bool>  tx_running_{false};
    std::atomic<bool>  tx_abort_{false};
    std::atomic<float> tx_progress_{0.0f};
    std::thread        tx_thread_;

    // Atomic RX / TX configuration & telemetry
    std::atomic<SstvRxState> rx_state_{SstvRxState::HuntingVis};
    std::atomic<SstvMode>    active_rx_mode_{SstvMode::Martin1};
    std::atomic<SstvMode>    manual_rx_mode_{SstvMode::Martin1};
    std::atomic<SstvMode>    tx_mode_{SstvMode::Martin1};
    std::atomic<bool>        auto_vis_{true};
    std::atomic<bool>        auto_save_png_{true};
    std::atomic<bool>        tx_dither_1bit_{false};
    std::atomic<bool>        clear_requested_{false};
    std::atomic<bool>        force_rx_requested_{false};
    std::atomic<uint16_t>    rx_current_line_{0};
    std::atomic<uint16_t>    rx_total_lines_{256};
    std::atomic<uint16_t>    rx_active_w_{320};
    std::atomic<uint16_t>    rx_active_h_{256};
    std::atomic<float>       slant_ppm_{0.0f};
    std::atomic<float>       auto_slant_ppm_{0.0f};
    std::atomic<float>       inst_freq_hz_{1500.0f};
    std::atomic<uint32_t>    rx_completed_count_{0};

    // Hilbert Analytic FIR + Quadrature FM Discriminator state (31-tap odd-symmetric Blackman FIR)
    static constexpr size_t kHilbertTaps  = 31;
    static constexpr size_t kHilbertDelay = (kHilbertTaps - 1) / 2; // 15 samples
    alignas(64) std::array<float, kHilbertTaps> hilbert_hist_{};
    size_t hilbert_pos_ = 0;
    float  prev_i_      = 1.0f;
    float  prev_q_      = 0.0f;
    float  freq_lp1_    = 1500.0f;
    float  freq_lp2_    = 1500.0f;
    float  last_mag_sq_ = 0.0f;

    // VIS Digital Header state machine (runs @ 12 kHz sample clock)
    uint32_t vis_leader1_samples_ = 0;
    uint32_t vis_break_samples_   = 0;
    uint32_t vis_leader2_samples_ = 0;
    uint32_t vis_trans_samples_   = 0;
    bool     vis_saw_break_       = false;
    uint32_t vis_bit_sample_      = 0;
    uint8_t  vis_bit_index_       = 0;
    uint8_t  vis_shift_reg_       = 0;
    double   vis_bit_freq_accum_  = 0.0;
    uint32_t vis_bit_accum_cnt_   = 0;

    // Scanline timing, horizontal 1200 Hz sync tracker & slant estimator
    static constexpr size_t kMaxLineSamples = 7000; // Covers PD120 508.48 ms * 12 kHz = 6102 samples
    alignas(64) std::array<float, kMaxLineSamples> line_freq_buf_{};
    double   line_sample_target_ = 5357.352;
    double   line_frac_accum_    = 0.0;
    size_t   line_sample_idx_    = 0;
    uint16_t current_unit_idx_   = 0; // Line index (or line-pair index for PD120)
    uint16_t total_units_        = 256;
    bool     scottie_first_sync_ = false;
    double   sync_phase_err_ema_ = 0.0;

    // Chroma line memory for Robot 4:2:0 (even line R-Y, odd line B-Y interpolation)
    alignas(64) std::array<uint8_t, kMaxWidth> prev_y_row_{};
    alignas(64) std::array<uint8_t, kMaxWidth> last_cr_row_{};
    alignas(64) std::array<uint8_t, kMaxWidth> last_cb_row_{};

    // Pre-allocated DSP tap & discriminator scratchpads (zero heap allocs in worker)
    alignas(64) std::array<float, 1024> rx_tap_scratch_{};
    alignas(64) std::array<float, 1024> rx_freq_scratch_{};

    // Pre-allocated 512-pt FFT state for 1000..2500 Hz SSTV tuning waterfall
    alignas(64) std::array<float, kFftSize>      fft_ring_{};
    size_t                                       fft_ring_pos_ = 0;
    size_t                                       fft_hop_cnt_  = 0;
    alignas(64) std::array<float, kFftSize>      fft_window_{};
    alignas(64) std::array<float, kFftSize / 2>  fft_tw_re_{};
    alignas(64) std::array<float, kFftSize / 2>  fft_tw_im_{};
    alignas(64) std::array<uint16_t, kFftSize>   fft_bitrev_{};
    alignas(64) std::array<float, kFftSize>      fft_work_re_{};
    alignas(64) std::array<float, kFftSize>      fft_work_im_{};
    alignas(64) std::array<float, kWaterfallW>   spectrum_db_{};
    alignas(64) std::array<uint32_t, static_cast<size_t>(kWaterfallW) * kWaterfallH> waterfall_bgra_{};
    std::atomic<uint64_t>                        wf_seq_{1};

    // Lock-free double-buffered RX scanline surfaces (640x496 BGRA32 contiguous arrays)
    alignas(64) std::array<uint32_t, kMaxPixels> rx_work_bgra_{};
    alignas(64) std::array<std::array<uint32_t, kMaxPixels>, 2> rx_published_bgra_{};
    std::atomic<uint8_t>  rx_front_idx_{0};
    std::atomic<uint64_t> rx_surface_seq_{1};

    // TX Source Image & Mode-Cropped / Dithered / Overlayed TX Preview Surface
    std::vector<uint32_t>                        tx_source_bgra_;
    uint16_t                                     tx_source_w_ = 0;
    uint16_t                                     tx_source_h_ = 0;
    alignas(64) std::array<uint32_t, kMaxPixels> tx_preview_bgra_{};
    alignas(64) std::array<uint32_t, kMaxPixels> tx_active_snap_bgra_{};
    uint16_t                                     tx_preview_w_ = 320;
    uint16_t                                     tx_preview_h_ = 256;
    std::atomic<uint64_t>                        tx_preview_seq_{1};
    std::string                                  tx_overlay_text_ = "W1AW  FN31";

    std::string capture_dir_     = "sstv_captures";
    std::string last_saved_png_;
};

} // namespace hamshack::carbon
