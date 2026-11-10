// ============================================================================
// SstvEngine.cpp — Carbon Engine Multi-Mode SSTV Transceiver (RX DSP + TX Modulator)
// ============================================================================

#include "SstvEngine.hpp"
#include "hamshack_platform.hpp"

#include <algorithm>
#include <cctype>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <fstream>

#ifdef _WIN32
#include <objbase.h>
#include <wincodec.h>
#endif

#ifdef __APPLE__
extern "C" {
typedef const void* CFTypeRef;
typedef const struct __CFData* CFDataRef;
typedef long CFIndex;
typedef struct CGImage* CGImageRef;
typedef struct CGColorSpace* CGColorSpaceRef;
typedef struct CGContext* CGContextRef;
typedef struct CGImageSource* CGImageSourceRef;
struct CGPoint { double x; double y; };
struct CGSize  { double width; double height; };
struct CGRect  { CGPoint origin; CGSize size; };

CFDataRef CFDataCreate(const void*, const uint8_t*, CFIndex);
void CFRelease(CFTypeRef);
CGImageSourceRef CGImageSourceCreateWithData(CFDataRef, const void*);
CGImageRef CGImageSourceCreateImageAtIndex(CGImageSourceRef, size_t, const void*);
size_t CGImageGetWidth(CGImageRef);
size_t CGImageGetHeight(CGImageRef);
void CGImageRelease(CGImageRef);
CGColorSpaceRef CGColorSpaceCreateDeviceRGB(void);
void CGColorSpaceRelease(CGColorSpaceRef);
CGContextRef CGBitmapContextCreate(void*, size_t, size_t, size_t, size_t, CGColorSpaceRef, uint32_t);
void CGContextSetInterpolationQuality(CGContextRef, int);
void CGContextDrawImage(CGContextRef, CGRect, CGImageRef);
void CGContextRelease(CGContextRef);
}
#endif

namespace hamshack::carbon {

namespace {

inline constexpr double kPi    = 3.14159265358979323846;
inline constexpr double kTwoPi = 6.28318530717958647692;

[[nodiscard]] constexpr uint32_t pack_bgra(uint8_t r, uint8_t g, uint8_t b, uint8_t a = 255u) noexcept {
    return (static_cast<uint32_t>(a) << 24) |
           (static_cast<uint32_t>(r) << 16) |
           (static_cast<uint32_t>(g) << 8)  |
            static_cast<uint32_t>(b);
}

[[nodiscard]] constexpr uint8_t bgra_r(uint32_t px) noexcept { return static_cast<uint8_t>((px >> 16) & 0xFFu); }
[[nodiscard]] constexpr uint8_t bgra_g(uint32_t px) noexcept { return static_cast<uint8_t>((px >> 8)  & 0xFFu); }
[[nodiscard]] constexpr uint8_t bgra_b(uint32_t px) noexcept { return static_cast<uint8_t>( px        & 0xFFu); }

[[nodiscard]] inline uint8_t clamp_u8(float v) noexcept {
    const float c = (v < 0.0f) ? 0.0f : ((v > 255.0f) ? 255.0f : v);
    return static_cast<uint8_t>(c + 0.5f);
}

// Standard SSTV ITU-R BT.601 RGB <-> YCrCb conversions
inline void rgb_to_sstv_ycrcb(uint8_t r, uint8_t g, uint8_t b,
                              uint8_t& out_y, uint8_t& out_cr, uint8_t& out_cb) noexcept {
    const float rf = static_cast<float>(r);
    const float gf = static_cast<float>(g);
    const float bf = static_cast<float>(b);
    out_y  = clamp_u8(16.0f  + 0.003906f * ( 65.738f * rf + 129.057f * gf +  25.064f * bf));
    out_cr = clamp_u8(128.0f + 0.003906f * (112.439f * rf -  94.154f * gf -  18.285f * bf));
    out_cb = clamp_u8(128.0f + 0.003906f * (-37.945f * rf -  74.494f * gf + 112.439f * bf));
}

[[nodiscard]] inline uint32_t sstv_ycrcb_to_bgra(uint8_t y, uint8_t cr, uint8_t cb) noexcept {
    const float yf  = 1.164383f * (static_cast<float>(y) - 16.0f);
    const float crf = static_cast<float>(cr) - 128.0f;
    const float cbf = static_cast<float>(cb) - 128.0f;
    const uint8_t r = clamp_u8(yf + 1.596027f * crf);
    const uint8_t g = clamp_u8(yf - 0.812968f * crf - 0.391762f * cbf);
    const uint8_t b = clamp_u8(yf + 2.017232f * cbf);
    return pack_bgra(r, g, b, 255u);
}

[[nodiscard]] inline uint8_t rgb_to_luma_bw(uint8_t r, uint8_t g, uint8_t b) noexcept {
    return clamp_u8(0.299f * static_cast<float>(r) +
                    0.587f * static_cast<float>(g) +
                    0.114f * static_cast<float>(b));
}

// Branchless linear mapping: 1500 Hz = Black (0), 2300 Hz = White (255)
[[nodiscard]] inline uint8_t freq_to_pixel_u8(float freq_hz) noexcept {
    const float norm = (freq_hz - 1500.0f) * (255.0f / 800.0f);
    return clamp_u8(norm);
}

[[nodiscard]] constexpr double pixel_u8_to_freq_hz(uint8_t val) noexcept {
    return 1500.0 + (static_cast<double>(val) * (800.0 / 255.0));
}

// Precomputed 8 non-zero odd taps of a 31-tap Blackman-windowed Analytic Hilbert FIR:
// h[k] = (2 / (pi * m)) * w_blackman(15 + m) for m = 1, 3, 5, 7, 9, 11, 13, 15
inline constexpr std::array<float, 8> kHilbertOddTaps{{
    0.6313880f, // m = 1
    0.1945484f, // m = 3
    0.0958999f, // m = 5
    0.0488587f, // m = 7
    0.0232024f, // m = 9
    0.0090963f, // m = 11
    0.0023285f, // m = 13
    0.0000000f  // m = 15 (Blackman endpoint = 0)
}};

// Sample a scanline channel segment [start_samp, start_samp + dur_samp) into contiguous dst[0..width-1]
// Completely branchless inner pixel interpolation for SIMD/auto-vectorization efficiency.
void sample_scan_segment(std::span<const float> line_freqs,
                         double start_samp,
                         double dur_samp,
                         uint16_t width,
                         std::span<uint8_t> dst) noexcept {
    if (width == 0 || line_freqs.empty() || dst.size() < width) return;
    const double max_idx = static_cast<double>(line_freqs.size() - 1);
    const double step    = dur_samp / static_cast<double>(width);
    double pos           = start_samp + 0.5 * step;

    for (uint16_t x = 0; x < width; ++x) {
        const double clamped = (pos < 0.0) ? 0.0 : ((pos > max_idx) ? max_idx : pos);
        const size_t idx0    = static_cast<size_t>(clamped);
        const size_t idx1    = (idx0 + 1 < line_freqs.size()) ? (idx0 + 1) : idx0;
        const float  frac    = static_cast<float>(clamped - static_cast<double>(idx0));
        const float  f_hz    = line_freqs[idx0] + frac * (line_freqs[idx1] - line_freqs[idx0]);
        dst[x]               = freq_to_pixel_u8(f_hz);
        pos += step;
    }
}

// Waterfall color palette (deep navy -> cyan -> emerald -> gold -> hot white)
[[nodiscard]] uint32_t db_to_waterfall_bgra(float db) noexcept {
    float t = (db + 92.0f) * (1.0f / 62.0f);
    t = (t < 0.0f) ? 0.0f : ((t > 1.0f) ? 1.0f : t);

    uint8_t r = 0, g = 0, b = 0;
    if (t < 0.25f) {
        const float u = t * 4.0f;
        r = clamp_u8(6.0f  + 10.0f * u);
        g = clamp_u8(12.0f + 58.0f * u);
        b = clamp_u8(32.0f + 130.0f * u);
    } else if (t < 0.55f) {
        const float u = (t - 0.25f) * (1.0f / 0.30f);
        r = clamp_u8(16.0f + 24.0f * u);
        g = clamp_u8(70.0f + 150.0f * u);
        b = clamp_u8(162.0f + 60.0f * u);
    } else if (t < 0.82f) {
        const float u = (t - 0.55f) * (1.0f / 0.27f);
        r = clamp_u8(40.0f + 210.0f * u);
        g = clamp_u8(220.0f + 20.0f * u);
        b = clamp_u8(222.0f - 180.0f * u);
    } else {
        const float u = (t - 0.82f) * (1.0f / 0.18f);
        r = 255u;
        g = clamp_u8(240.0f + 15.0f * u);
        b = clamp_u8(42.0f  + 213.0f * u);
    }
    return pack_bgra(r, g, b, 255u);
}

// Crisp 5x7 ASCII bitmap font (printable 32..95 uppercase/symbols) for SSTV Callsign/RST Text Overlay
[[nodiscard]] std::array<uint8_t, 7> glyph_5x7(char ch) noexcept {
    const unsigned char uc = static_cast<unsigned char>(std::toupper(static_cast<unsigned char>(ch)));
    switch (uc) {
        case 'A': return {0x0E,0x11,0x11,0x1F,0x11,0x11,0x11};
        case 'B': return {0x1E,0x11,0x11,0x1E,0x11,0x11,0x1E};
        case 'C': return {0x0E,0x11,0x10,0x10,0x10,0x11,0x0E};
        case 'D': return {0x1E,0x11,0x11,0x11,0x11,0x11,0x1E};
        case 'E': return {0x1F,0x10,0x10,0x1E,0x10,0x10,0x1F};
        case 'F': return {0x1F,0x10,0x10,0x1E,0x10,0x10,0x10};
        case 'G': return {0x0E,0x11,0x10,0x17,0x11,0x11,0x0F};
        case 'H': return {0x11,0x11,0x11,0x1F,0x11,0x11,0x11};
        case 'I': return {0x0E,0x04,0x04,0x04,0x04,0x04,0x0E};
        case 'J': return {0x07,0x02,0x02,0x02,0x12,0x12,0x0C};
        case 'K': return {0x11,0x12,0x14,0x18,0x14,0x12,0x11};
        case 'L': return {0x10,0x10,0x10,0x10,0x10,0x10,0x1F};
        case 'M': return {0x11,0x1B,0x15,0x15,0x11,0x11,0x11};
        case 'N': return {0x11,0x19,0x15,0x13,0x11,0x11,0x11};
        case 'O': return {0x0E,0x11,0x11,0x11,0x11,0x11,0x0E};
        case 'P': return {0x1E,0x11,0x11,0x1E,0x10,0x10,0x10};
        case 'Q': return {0x0E,0x11,0x11,0x11,0x15,0x12,0x0D};
        case 'R': return {0x1E,0x11,0x11,0x1E,0x14,0x12,0x11};
        case 'S': return {0x0F,0x10,0x10,0x0E,0x01,0x01,0x1E};
        case 'T': return {0x1F,0x04,0x04,0x04,0x04,0x04,0x04};
        case 'U': return {0x11,0x11,0x11,0x11,0x11,0x11,0x0E};
        case 'V': return {0x11,0x11,0x11,0x11,0x11,0x0A,0x04};
        case 'W': return {0x11,0x11,0x11,0x15,0x15,0x1B,0x11};
        case 'X': return {0x11,0x11,0x0A,0x04,0x0A,0x11,0x11};
        case 'Y': return {0x11,0x11,0x0A,0x04,0x04,0x04,0x04};
        case 'Z': return {0x1F,0x01,0x02,0x04,0x08,0x10,0x1F};
        case '0': return {0x0E,0x11,0x13,0x15,0x19,0x11,0x0E};
        case '1': return {0x04,0x0C,0x04,0x04,0x04,0x04,0x0E};
        case '2': return {0x0E,0x11,0x01,0x06,0x08,0x10,0x1F};
        case '3': return {0x1F,0x02,0x04,0x06,0x01,0x11,0x0E};
        case '4': return {0x02,0x06,0x0A,0x12,0x1F,0x02,0x02};
        case '5': return {0x1F,0x10,0x1E,0x01,0x01,0x11,0x0E};
        case '6': return {0x06,0x08,0x10,0x1E,0x11,0x11,0x0E};
        case '7': return {0x1F,0x01,0x02,0x04,0x08,0x08,0x08};
        case '8': return {0x0E,0x11,0x11,0x0E,0x11,0x11,0x0E};
        case '9': return {0x0E,0x11,0x11,0x0F,0x01,0x02,0x0C};
        case '-': return {0x00,0x00,0x00,0x1F,0x00,0x00,0x00};
        case '/': return {0x01,0x02,0x02,0x04,0x08,0x08,0x10};
        case ':': return {0x00,0x04,0x04,0x00,0x04,0x04,0x00};
        case '.': return {0x00,0x00,0x00,0x00,0x00,0x0C,0x0C};
        case '!': return {0x04,0x04,0x04,0x04,0x04,0x00,0x04};
        case '+': return {0x00,0x04,0x04,0x1F,0x04,0x04,0x00};
        case '#': return {0x0A,0x1F,0x0A,0x0A,0x1F,0x0A,0x00};
        default:  return {0x00,0x00,0x00,0x00,0x00,0x00,0x00};
    }
}

void burn_text_overlay(std::span<uint32_t> raster, uint16_t w, uint16_t h, uint16_t stride,
                       std::string_view text, bool mono_1bit) noexcept {
    while (!text.empty() && text.back() == ' ') text.remove_suffix(1);
    if (text.empty() || w < 32 || h < 24) return;

    const int scale = (w >= 512) ? 4 : ((w >= 280) ? 3 : 2);
    const int banner_h = 7 * scale + 6 * (scale / 2 + 1);
    const uint32_t bg_px = mono_1bit ? pack_bgra(0, 0, 0) : pack_bgra(10, 14, 24);
    const uint32_t fg_px = mono_1bit ? pack_bgra(255, 255, 255) : pack_bgra(255, 232, 64);

    // High-contrast top callsign bar (SSTV standard practice so QRM on sync never obscures callsign)
    const int bar_rows = std::min<int>(h, banner_h);
    for (int y = 0; y < bar_rows; ++y) {
        uint32_t* row = raster.data() + static_cast<size_t>(y) * stride;
        for (int x = 0; x < w; ++x) {
            if (mono_1bit) {
                row[x] = bg_px;
            } else {
                const uint32_t cur = row[x];
                row[x] = pack_bgra(bgra_r(cur) / 4, bgra_g(cur) / 4, bgra_b(cur) / 4);
            }
        }
    }

    int cursor_x = scale * 2;
    const int top_y = (bar_rows - 7 * scale) / 2;
    for (char ch : text) {
        if (cursor_x + 5 * scale >= w) break;
        const auto rows = glyph_5x7(ch);
        for (int gy = 0; gy < 7; ++gy) {
            const uint8_t bits = rows[static_cast<size_t>(gy)];
            for (int gx = 0; gx < 5; ++gx) {
                if ((bits & (1u << (4 - gx))) == 0) continue;
                for (int sy = 0; sy < scale; ++sy) {
                    for (int sx = 0; sx < scale; ++sx) {
                        const int px = cursor_x + gx * scale + sx;
                        const int py = top_y + gy * scale + sy;
                        if (px >= 0 && px < w && py >= 0 && py < h) {
                            // Draw 1px black outline for maximum readability over noisy HF paths
                            for (int oy = -1; oy <= 1; ++oy) {
                                for (int ox = -1; ox <= 1; ++ox) {
                                    const int nx = px + ox, ny = py + oy;
                                    if (nx >= 0 && nx < w && ny >= 0 && ny < h && (ox != 0 || oy != 0)) {
                                        if (raster[static_cast<size_t>(ny) * stride + static_cast<size_t>(nx)] != fg_px) {
                                            raster[static_cast<size_t>(ny) * stride + static_cast<size_t>(nx)] = pack_bgra(0, 0, 0);
                                        }
                                    }
                                }
                            }
                            raster[static_cast<size_t>(py) * stride + static_cast<size_t>(px)] = fg_px;
                        }
                    }
                }
            }
        }
        cursor_x += 6 * scale;
    }
}

// RFC 1952 CRC-32 & RFC 1950 Adler-32 helpers for zero-dependency PNG writer
uint32_t png_crc32(std::span<const uint8_t> bytes, uint32_t crc_init = 0xFFFFFFFFu) noexcept {
    uint32_t crc = crc_init;
    for (uint8_t b : bytes) {
        crc ^= b;
        for (int k = 0; k < 8; ++k) {
            crc = (crc & 1u) ? ((crc >> 1) ^ 0xEDB88320u) : (crc >> 1);
        }
    }
    return ~crc;
}

void write_be32(std::vector<uint8_t>& out, uint32_t v) {
    out.push_back(static_cast<uint8_t>((v >> 24) & 0xFFu));
    out.push_back(static_cast<uint8_t>((v >> 16) & 0xFFu));
    out.push_back(static_cast<uint8_t>((v >> 8)  & 0xFFu));
    out.push_back(static_cast<uint8_t>( v        & 0xFFu));
}

void append_png_chunk(std::vector<uint8_t>& out, const char type4[4], std::span<const uint8_t> payload) {
    write_be32(out, static_cast<uint32_t>(payload.size()));
    const size_t crc_start = out.size();
    out.insert(out.end(), reinterpret_cast<const uint8_t*>(type4), reinterpret_cast<const uint8_t*>(type4) + 4);
    out.insert(out.end(), payload.begin(), payload.end());
    const uint32_t crc = png_crc32(std::span<const uint8_t>{out.data() + crc_start, 4 + payload.size()});
    write_be32(out, crc);
}

} // namespace

// ============================================================================
// SstvModulator Implementation — Zero-Allocation Inner PCM Synthesis
// ============================================================================
template <typename ChunkFn>
bool SstvModulator::modulate_image(SstvMode mode,
                                   std::span<const uint32_t> bgra_raster,
                                   uint16_t raster_stride_px,
                                   int sample_rate_hz,
                                   ChunkFn&& emit_chunk) noexcept {
    const SstvModeSpec& spec = sstv_mode_spec(mode);
    if (sample_rate_hz < 8000 || bgra_raster.size() < static_cast<size_t>(raster_stride_px) * spec.height) {
        return false;
    }

    const double fs = static_cast<double>(sample_rate_hz);
    constexpr double kAmp = 26000.0; // Clean -2.0 dBFS peak for SSB/FM modulators

    // Compute exact total duration in ms for progress reporting & final chunk flagging
    const double vis_total_ms = 300.0 + 10.0 + 100.0 + 10.0 * 30.0; // 710.0 ms standard VIS header
    const size_t num_units    = (spec.encoding == SstvColorEncoding::PdDualLineYuv)
                                ? static_cast<size_t>(spec.height / 2)
                                : static_cast<size_t>(spec.height);
    const double extra_sync_ms = (spec.encoding == SstvColorEncoding::GbrMidSync) ? spec.sync_ms : 0.0;
    const double total_ms      = vis_total_ms + extra_sync_ms + static_cast<double>(num_units) * spec.line_period_ms;
    const size_t total_samples = static_cast<size_t>(std::llround(total_ms * fs / 1000.0));

    alignas(64) std::array<int16_t, kChunkSamples> chunk_buf{};
    size_t chunk_fill     = 0;
    size_t emitted_samps  = 0;
    double elapsed_ms     = 0.0;
    double phase          = 0.0;
    bool   first_chunk    = true;

    auto flush_chunk = [&](bool is_last) noexcept -> bool {
        if (chunk_fill == 0) return true;
        const float prog = (total_samples > 0)
            ? std::min(1.0f, static_cast<float>(emitted_samps + chunk_fill) / static_cast<float>(total_samples))
            : 1.0f;
        const bool keep_going = emit_chunk(std::span<const int16_t>{chunk_buf.data(), chunk_fill},
                                           first_chunk, is_last, prog);
        first_chunk    = false;
        emitted_samps += chunk_fill;
        chunk_fill     = 0;
        return keep_going;
    };

    auto emit_sample_hz = [&](double freq_hz) noexcept -> bool {
        const double s = std::sin(phase) * kAmp;
        chunk_buf[chunk_fill++] = static_cast<int16_t>(std::lround(s));
        phase += kTwoPi * freq_hz / fs;
        if (phase >= kTwoPi) phase -= kTwoPi;
        if (chunk_fill == kChunkSamples) {
            return flush_chunk(false);
        }
        return true;
    };

    // Synthesize constant tone up to exact cumulative millisecond timestamp (0.00 ppm rounding drift!)
    auto emit_tone_ms = [&](double duration_ms, double freq_hz) noexcept -> bool {
        if (duration_ms <= 0.0) return true;
        const size_t s0 = static_cast<size_t>(std::llround(elapsed_ms * fs / 1000.0));
        elapsed_ms += duration_ms;
        const size_t s1 = static_cast<size_t>(std::llround(elapsed_ms * fs / 1000.0));
        for (size_t s = s0; s < s1; ++s) {
            if (!emit_sample_hz(freq_hz)) return false;
        }
        return true;
    };

    // Synthesize 1D pixel row (1500..2300 Hz linear ramp) across exact fractional sample span
    auto emit_scan_ms = [&](double duration_ms, std::span<const uint8_t> row_vals) noexcept -> bool {
        if (duration_ms <= 0.0 || row_vals.empty()) return true;
        const size_t s0 = static_cast<size_t>(std::llround(elapsed_ms * fs / 1000.0));
        elapsed_ms += duration_ms;
        const size_t s1 = static_cast<size_t>(std::llround(elapsed_ms * fs / 1000.0));
        const size_t n_samps = (s1 > s0) ? (s1 - s0) : 0;
        const size_t w = row_vals.size();
        for (size_t i = 0; i < n_samps; ++i) {
            const size_t px = std::min(w - 1, (i * w) / n_samps);
            if (!emit_sample_hz(pixel_u8_to_freq_hz(row_vals[px]))) return false;
        }
        return true;
    };

    // 1. Standard SSTV Calibration Header + Digital VIS Code:
    //    1900 Hz (300 ms) -> 1200 Hz (10 ms) -> 1900 Hz (100 ms) -> 1200 Hz Start (30 ms)
    //    -> 7 LSB-first bits (1100 Hz = '1', 1300 Hz = '0', 30 ms) -> Even Parity (30 ms) -> 1200 Hz Stop (30 ms)
    if (!emit_tone_ms(300.0, 1900.0)) return false;
    if (!emit_tone_ms( 10.0, 1200.0)) return false;
    if (!emit_tone_ms(100.0, 1900.0)) return false;
    if (!emit_tone_ms( 30.0, 1200.0)) return false; // VIS start bit

    const uint8_t vis = spec.vis_code & 0x7Fu;
    uint8_t ones = 0;
    for (int bit = 0; bit < 7; ++bit) {
        const bool is_one = ((vis >> bit) & 1u) != 0;
        if (is_one) ++ones;
        if (!emit_tone_ms(30.0, is_one ? 1100.0 : 1300.0)) return false;
    }
    const bool parity_one = (ones & 1u) != 0; // Even parity across 8 bits
    if (!emit_tone_ms(30.0, parity_one ? 1100.0 : 1300.0)) return false;
    if (!emit_tone_ms(30.0, 1200.0)) return false; // VIS stop bit

    // Scottie 1/2 initial 9.0 ms starting sync pulse before Line 0
    if (spec.encoding == SstvColorEncoding::GbrMidSync) {
        if (!emit_tone_ms(spec.sync_ms, 1200.0)) return false;
    }

    // Pre-allocated scanline scratch rows (zero heap allocations)
    alignas(64) std::array<uint8_t, SstvEngine::kMaxWidth> ch0{};
    alignas(64) std::array<uint8_t, SstvEngine::kMaxWidth> ch1{};
    alignas(64) std::array<uint8_t, SstvEngine::kMaxWidth> ch2{};
    alignas(64) std::array<uint8_t, SstvEngine::kMaxWidth> ch3{};
    const uint16_t w = spec.width;

    // 2. Synthesize Scanlines per Mode Specification
    if (spec.encoding == SstvColorEncoding::PdDualLineYuv) {
        for (uint16_t y = 0; y + 1 < spec.height; y += 2) {
            const uint32_t* r0 = bgra_raster.data() + static_cast<size_t>(y) * raster_stride_px;
            const uint32_t* r1 = bgra_raster.data() + static_cast<size_t>(y + 1) * raster_stride_px;
            for (uint16_t x = 0; x < w; ++x) {
                uint8_t y0, cr0, cb0, y1, cr1, cb1;
                rgb_to_sstv_ycrcb(bgra_r(r0[x]), bgra_g(r0[x]), bgra_b(r0[x]), y0, cr0, cb0);
                rgb_to_sstv_ycrcb(bgra_r(r1[x]), bgra_g(r1[x]), bgra_b(r1[x]), y1, cr1, cb1);
                ch0[x] = y0;
                ch1[x] = static_cast<uint8_t>((static_cast<uint16_t>(cr0) + cr1 + 1u) >> 1);
                ch2[x] = static_cast<uint8_t>((static_cast<uint16_t>(cb0) + cb1 + 1u) >> 1);
                ch3[x] = y1;
            }
            if (!emit_tone_ms(spec.sync_ms,  1200.0)) return false;
            if (!emit_tone_ms(spec.porch_ms, 1500.0)) return false;
            if (!emit_scan_ms(spec.luma_scan_ms,   std::span<const uint8_t>{ch0.data(), w})) return false; // Y0
            if (!emit_scan_ms(spec.chroma_scan_ms, std::span<const uint8_t>{ch1.data(), w})) return false; // R-Y
            if (!emit_scan_ms(spec.chroma_scan_ms, std::span<const uint8_t>{ch2.data(), w})) return false; // B-Y
            if (!emit_scan_ms(spec.luma_scan_ms,   std::span<const uint8_t>{ch3.data(), w})) return false; // Y1
        }
    } else {
        for (uint16_t y = 0; y < spec.height; ++y) {
            const uint32_t* row = bgra_raster.data() + static_cast<size_t>(y) * raster_stride_px;

            switch (spec.encoding) {
                case SstvColorEncoding::GbrSequential: {
                    for (uint16_t x = 0; x < w; ++x) {
                        ch0[x] = bgra_g(row[x]);
                        ch1[x] = bgra_b(row[x]);
                        ch2[x] = bgra_r(row[x]);
                    }
                    if (!emit_tone_ms(spec.sync_ms,  1200.0)) return false;
                    if (!emit_tone_ms(spec.porch_ms, 1500.0)) return false;
                    if (!emit_scan_ms(spec.luma_scan_ms, std::span<const uint8_t>{ch0.data(), w})) return false;
                    if (!emit_tone_ms(spec.sep_ms,   1500.0)) return false;
                    if (!emit_scan_ms(spec.luma_scan_ms, std::span<const uint8_t>{ch1.data(), w})) return false;
                    if (!emit_tone_ms(spec.sep_ms,   1500.0)) return false;
                    if (!emit_scan_ms(spec.luma_scan_ms, std::span<const uint8_t>{ch2.data(), w})) return false;
                    if (!emit_tone_ms(spec.sep_ms,   1500.0)) return false;
                    break;
                }
                case SstvColorEncoding::GbrMidSync: {
                    for (uint16_t x = 0; x < w; ++x) {
                        ch0[x] = bgra_g(row[x]);
                        ch1[x] = bgra_b(row[x]);
                        ch2[x] = bgra_r(row[x]);
                    }
                    if (!emit_tone_ms(spec.sep_ms,   1500.0)) return false;
                    if (!emit_scan_ms(spec.luma_scan_ms, std::span<const uint8_t>{ch0.data(), w})) return false;
                    if (!emit_tone_ms(spec.sep_ms,   1500.0)) return false;
                    if (!emit_scan_ms(spec.luma_scan_ms, std::span<const uint8_t>{ch1.data(), w})) return false;
                    if (!emit_tone_ms(spec.sync_ms,  1200.0)) return false;
                    if (!emit_tone_ms(spec.porch_ms, 1500.0)) return false;
                    if (!emit_scan_ms(spec.luma_scan_ms, std::span<const uint8_t>{ch2.data(), w})) return false;
                    break;
                }
                case SstvColorEncoding::GrayscaleBw: {
                    // Pure direct luminance scanlines — strictly zero chroma subcarriers!
                    for (uint16_t x = 0; x < w; ++x) {
                        ch0[x] = rgb_to_luma_bw(bgra_r(row[x]), bgra_g(row[x]), bgra_b(row[x]));
                    }
                    if (!emit_tone_ms(spec.sync_ms,  1200.0)) return false;
                    if (!emit_tone_ms(spec.porch_ms, 1500.0)) return false;
                    if (!emit_scan_ms(spec.luma_scan_ms, std::span<const uint8_t>{ch0.data(), w})) return false;
                    break;
                }
                case SstvColorEncoding::RobotYuv420: {
                    const uint32_t* next_row = (y + 1 < spec.height)
                        ? (bgra_raster.data() + static_cast<size_t>(y + 1) * raster_stride_px)
                        : row;
                    const uint32_t* prev_row = (y > 0)
                        ? (bgra_raster.data() + static_cast<size_t>(y - 1) * raster_stride_px)
                        : row;

                    for (uint16_t x = 0; x < w; ++x) {
                        uint8_t yv, cr0, cb0, y_adj, cr1, cb1;
                        rgb_to_sstv_ycrcb(bgra_r(row[x]), bgra_g(row[x]), bgra_b(row[x]), yv, cr0, cb0);
                        ch0[x] = yv;
                        if ((y & 1u) == 0) {
                            rgb_to_sstv_ycrcb(bgra_r(next_row[x]), bgra_g(next_row[x]), bgra_b(next_row[x]), y_adj, cr1, cb1);
                            ch1[x] = static_cast<uint8_t>((static_cast<uint16_t>(cr0) + cr1 + 1u) >> 1);
                        } else {
                            rgb_to_sstv_ycrcb(bgra_r(prev_row[x]), bgra_g(prev_row[x]), bgra_b(prev_row[x]), y_adj, cr1, cb1);
                            ch1[x] = static_cast<uint8_t>((static_cast<uint16_t>(cb0) + cb1 + 1u) >> 1);
                        }
                    }
                    if (!emit_tone_ms(spec.sync_ms,  1200.0)) return false;
                    if (!emit_tone_ms(spec.porch_ms, 1500.0)) return false;
                    if (!emit_scan_ms(spec.luma_scan_ms, std::span<const uint8_t>{ch0.data(), w})) return false;
                    // Even line: 1500 Hz separator + R-Y; Odd line: 2300 Hz separator + B-Y
                    if (!emit_tone_ms(spec.sep_ms, ((y & 1u) == 0) ? 1500.0 : 2300.0)) return false;
                    if (!emit_tone_ms(spec.chroma_porch_ms, 1900.0)) return false;
                    if (!emit_scan_ms(spec.chroma_scan_ms, std::span<const uint8_t>{ch1.data(), w})) return false;
                    break;
                }
                case SstvColorEncoding::RobotYuv422: {
                    for (uint16_t x = 0; x < w; ++x) {
                        rgb_to_sstv_ycrcb(bgra_r(row[x]), bgra_g(row[x]), bgra_b(row[x]), ch0[x], ch1[x], ch2[x]);
                    }
                    if (!emit_tone_ms(spec.sync_ms,  1200.0)) return false;
                    if (!emit_tone_ms(spec.porch_ms, 1500.0)) return false;
                    if (!emit_scan_ms(spec.luma_scan_ms,   std::span<const uint8_t>{ch0.data(), w})) return false;
                    if (!emit_tone_ms(spec.sep_ms,         1500.0)) return false;
                    if (!emit_tone_ms(spec.chroma_porch_ms,1900.0)) return false;
                    if (!emit_scan_ms(spec.chroma_scan_ms, std::span<const uint8_t>{ch1.data(), w})) return false; // R-Y
                    if (!emit_tone_ms(spec.sep_ms,         2300.0)) return false;
                    if (!emit_tone_ms(spec.chroma_porch_ms,1900.0)) return false;
                    if (!emit_scan_ms(spec.chroma_scan_ms, std::span<const uint8_t>{ch2.data(), w})) return false; // B-Y
                    break;
                }
                default:
                    break;
            }
        }
    }

    return flush_chunk(true);
}

void SstvModulator::synthesize_to_vector(SstvMode mode,
                                         std::span<const uint32_t> bgra_raster,
                                         uint16_t raster_stride_px,
                                         int sample_rate_hz,
                                         std::vector<int16_t>& out_pcm) {
    const SstvModeSpec& spec = sstv_mode_spec(mode);
    const size_t units = (spec.encoding == SstvColorEncoding::PdDualLineYuv)
                         ? static_cast<size_t>(spec.height / 2)
                         : static_cast<size_t>(spec.height);
    const double approx_ms = 800.0 + static_cast<double>(units) * spec.line_period_ms;
    out_pcm.clear();
    out_pcm.reserve(static_cast<size_t>(approx_ms * static_cast<double>(sample_rate_hz) / 1000.0) + 4096);

    modulate_image(mode, bgra_raster, raster_stride_px, sample_rate_hz,
                   [&](std::span<const int16_t> chunk, bool, bool, float) noexcept -> bool {
                       out_pcm.insert(out_pcm.end(), chunk.begin(), chunk.end());
                       return true;
                   });
}

// ============================================================================
// SstvEngine Implementation
// ============================================================================
SstvEngine::SstvEngine(AudioRouter& router)
    : router_(router) {
    // Precompute 512-pt Hann window, bit-reversal permutation, and FFT twiddle factors
    for (size_t i = 0; i < kFftSize; ++i) {
        fft_window_[i] = static_cast<float>(0.5 * (1.0 - std::cos(kTwoPi * static_cast<double>(i) / static_cast<double>(kFftSize - 1))));
        uint16_t rev = 0;
        uint16_t v   = static_cast<uint16_t>(i);
        for (int b = 0; b < 9; ++b) { // log2(512) = 9
            rev = static_cast<uint16_t>((rev << 1) | (v & 1u));
            v >>= 1;
        }
        fft_bitrev_[i] = rev;
    }
    for (size_t k = 0; k < kFftSize / 2; ++k) {
        const double ang = -kTwoPi * static_cast<double>(k) / static_cast<double>(kFftSize);
        fft_tw_re_[k] = static_cast<float>(std::cos(ang));
        fft_tw_im_[k] = static_cast<float>(std::sin(ang));
    }

    spectrum_db_.fill(-80.0f);
    waterfall_bgra_.fill(pack_bgra(6, 12, 32));
    const uint32_t blank_bg = pack_bgra(12, 16, 24);
    rx_work_bgra_.fill(blank_bg);
    rx_published_bgra_[0].fill(blank_bg);
    rx_published_bgra_[1].fill(blank_bg);
    prev_y_row_.fill(16u);
    last_cr_row_.fill(128u);
    last_cb_row_.fill(128u);

    generate_default_test_card();
}

SstvEngine::~SstvEngine() {
    abort_transmit();
    if (tx_thread_.joinable()) {
        tx_thread_.join();
    }
    stop_worker();
}

void SstvEngine::start_worker() {
    if (worker_running_.exchange(true, std::memory_order_acq_rel)) {
        return;
    }
    if (dsp_thread_.joinable()) {
        dsp_thread_.join();
    }
    dsp_thread_ = std::thread(&SstvEngine::dsp_worker_loop, this);
}

void SstvEngine::stop_worker() {
    worker_running_.store(false, std::memory_order_release);
    if (dsp_thread_.joinable()) {
        dsp_thread_.join();
    }
}

void SstvEngine::set_mode_override(bool auto_vis, SstvMode manual_mode) noexcept {
    auto_vis_.store(auto_vis, std::memory_order_release);
    manual_rx_mode_.store(manual_mode, std::memory_order_release);
    if (!auto_vis) {
        force_rx_requested_.store(true, std::memory_order_release);
    }
}

void SstvEngine::force_start_manual_rx(SstvMode mode) noexcept {
    manual_rx_mode_.store(mode, std::memory_order_release);
    force_rx_requested_.store(true, std::memory_order_release);
}

void SstvEngine::adjust_slant_ppm(float delta_ppm) noexcept {
    const float cur = slant_ppm_.load(std::memory_order_relaxed);
    slant_ppm_.store(std::clamp(cur + delta_ppm, -1500.0f, 1500.0f), std::memory_order_release);
}

void SstvEngine::set_slant_ppm(float ppm) noexcept {
    slant_ppm_.store(std::clamp(ppm, -1500.0f, 1500.0f), std::memory_order_release);
}

void SstvEngine::clear_rx_canvas() noexcept {
    clear_requested_.store(true, std::memory_order_release);
}

void SstvEngine::set_capture_directory(std::string_view dir) {
    if (!dir.empty()) {
        capture_dir_ = std::string(dir);
    }
}

// ============================================================================
// Analytic Hilbert FIR + Branchless Quadrature FM Discriminator
// Zero heap allocations, zero inner branches, 64-byte aligned circular window.
// ============================================================================
void SstvEngine::demodulate_fm_block(std::span<const float> pcm, std::span<float> out_freq_hz) noexcept {
    constexpr float kRadToHz = static_cast<float>(static_cast<double>(kRouterSampleRateHz) / kTwoPi);
    const size_t n = std::min(pcm.size(), out_freq_hz.size());

    for (size_t idx = 0; idx < n; ++idx) {
        hilbert_hist_[hilbert_pos_] = pcm[idx];

        // Delayed in-phase tap I[n] = x[n - 15]
        const size_t center = (hilbert_pos_ + kHilbertTaps - kHilbertDelay) % kHilbertTaps;
        const float i_cur   = hilbert_hist_[center];

        // Odd-symmetric quadrature Hilbert convolution Q[n] (unrolled 7 active odd pairs)
        float q_cur = 0.0f;
        for (size_t t = 0; t < 7; ++t) {
            const size_t odd = 2 * t + 1;
            const float x_old = hilbert_hist_[(center + kHilbertTaps - odd) % kHilbertTaps];
            const float x_new = hilbert_hist_[(center + odd) % kHilbertTaps];
            q_cur += kHilbertOddTaps[t] * (x_old - x_new);
        }

        hilbert_pos_ = (hilbert_pos_ + 1) % kHilbertTaps;

        // Branchless quadrature phase-difference discriminator: z[n] * conj(z[n-1])
        const float dot   = i_cur * prev_i_ + q_cur * prev_q_ + 1.0e-12f;
        const float cross = q_cur * prev_i_ - i_cur * prev_q_;
        prev_i_           = i_cur;
        prev_q_           = q_cur;

        last_mag_sq_ = i_cur * i_cur + q_cur * q_cur;

        float raw_hz = std::atan2(cross, dot) * kRadToHz;
        raw_hz       = std::clamp(raw_hz, 900.0f, 2600.0f);

        // 3-tap binomial post-discriminator smoothing filter [0.25, 0.50, 0.25]
        const float smoothed = 0.25f * freq_lp2_ + 0.50f * freq_lp1_ + 0.25f * raw_hz;
        freq_lp2_ = freq_lp1_;
        freq_lp1_ = raw_hz;
        out_freq_hz[idx] = smoothed;
    }
    if (n > 0) {
        inst_freq_hz_.store(out_freq_hz[n - 1], std::memory_order_relaxed);
    }
}

void SstvEngine::update_waterfall_fft(std::span<const float> pcm) noexcept {
    constexpr size_t kHopSamples = 240; // 20 ms hop (50 FPS smooth tuning waterfall @ 12 kHz)
    for (float s : pcm) {
        fft_ring_[fft_ring_pos_] = s;
        fft_ring_pos_ = (fft_ring_pos_ + 1) & (kFftSize - 1);
        if (++fft_hop_cnt_ < kHopSamples) {
            continue;
        }
        fft_hop_cnt_ = 0;

        // Windowed bit-reversed copy into pre-allocated FFT work arrays
        for (size_t i = 0; i < kFftSize; ++i) {
            const float x = fft_ring_[(fft_ring_pos_ + i) & (kFftSize - 1)] * fft_window_[i];
            const uint16_t r = fft_bitrev_[i];
            fft_work_re_[r] = x;
            fft_work_im_[r] = 0.0f;
        }

        // In-place Radix-2 Cooley-Tukey FFT using precomputed twiddle LUT
        for (size_t len = 2; len <= kFftSize; len <<= 1) {
            const size_t half = len >> 1;
            const size_t step = kFftSize / len;
            for (size_t base = 0; base < kFftSize; base += len) {
                for (size_t j = 0; j < half; ++j) {
                    const size_t tw = j * step;
                    const float wr = fft_tw_re_[tw];
                    const float wi = fft_tw_im_[tw];
                    const float ur = fft_work_re_[base + j];
                    const float ui = fft_work_im_[base + j];
                    const float vr = fft_work_re_[base + j + half] * wr - fft_work_im_[base + j + half] * wi;
                    const float vi = fft_work_re_[base + j + half] * wi + fft_work_im_[base + j + half] * wr;
                    fft_work_re_[base + j]        = ur + vr;
                    fft_work_im_[base + j]        = ui + vi;
                    fft_work_re_[base + j + half] = ur - vr;
                    fft_work_im_[base + j + half] = ui - vi;
                }
            }
        }

        // Scroll waterfall down by 1 row (memmove on contiguous flat std::array)
        std::memmove(waterfall_bgra_.data() + kWaterfallW,
                     waterfall_bgra_.data(),
                     static_cast<size_t>(kWaterfallW) * (kWaterfallH - 1) * sizeof(uint32_t));

        // Map 1000 Hz .. 2500 Hz across 256 columns with sub-bin linear interpolation
        constexpr float kBinHz = static_cast<float>(kRouterSampleRateHz) / static_cast<float>(kFftSize); // 23.4375 Hz
        for (uint16_t x = 0; x < kWaterfallW; ++x) {
            const float f_hz = 1000.0f + (1500.0f * static_cast<float>(x)) / static_cast<float>(kWaterfallW - 1);
            const float fbin = f_hz / kBinHz;
            const size_t b0  = std::min<size_t>(kFftSize / 2 - 2, static_cast<size_t>(fbin));
            const float frac = fbin - static_cast<float>(b0);

            const float m0 = std::sqrt(fft_work_re_[b0] * fft_work_re_[b0] + fft_work_im_[b0] * fft_work_im_[b0]);
            const float m1 = std::sqrt(fft_work_re_[b0 + 1] * fft_work_re_[b0 + 1] + fft_work_im_[b0 + 1] * fft_work_im_[b0 + 1]);
            const float mag = (m0 + frac * (m1 - m0)) * (2.0f / static_cast<float>(kFftSize));
            const float db  = 20.0f * std::log10(std::max(mag, 1.0e-5f));

            spectrum_db_[x]    = 0.55f * spectrum_db_[x] + 0.45f * db;
            waterfall_bgra_[x] = db_to_waterfall_bgra(spectrum_db_[x]);
        }
        wf_seq_.fetch_add(1u, std::memory_order_release);
    }
}

void SstvEngine::begin_image_rx(SstvMode mode) noexcept {
    const SstvModeSpec& spec = sstv_mode_spec(mode);
    active_rx_mode_.store(mode, std::memory_order_release);
    rx_active_w_.store(spec.width, std::memory_order_release);
    rx_active_h_.store(spec.height, std::memory_order_release);
    rx_current_line_.store(0, std::memory_order_release);
    rx_total_lines_.store(spec.height, std::memory_order_release);

    current_unit_idx_   = 0;
    total_units_        = (spec.encoding == SstvColorEncoding::PdDualLineYuv)
                          ? static_cast<uint16_t>(spec.height / 2)
                          : spec.height;
    line_sample_target_ = spec.line_period_ms * static_cast<double>(kRouterSampleRateHz) / 1000.0;
    line_frac_accum_    = 0.0;
    line_sample_idx_    = 0;
    scottie_first_sync_ = (spec.encoding == SstvColorEncoding::GbrMidSync);
    sync_phase_err_ema_ = 0.0;
    auto_slant_ppm_.store(0.0f, std::memory_order_release);

    prev_y_row_.fill(16u);
    last_cr_row_.fill(128u);
    last_cb_row_.fill(128u);

    const uint32_t bg = pack_bgra(10, 14, 22);
    rx_work_bgra_.fill(bg);
    const uint8_t back = rx_front_idx_.load(std::memory_order_relaxed) ^ 1u;
    rx_published_bgra_[back] = rx_work_bgra_;
    rx_front_idx_.store(back, std::memory_order_release);
    rx_surface_seq_.fetch_add(1u, std::memory_order_release);

    rx_state_.store(SstvRxState::ReceivingImage, std::memory_order_release);
}

void SstvEngine::publish_rx_scanline_to_front(uint16_t y_start, uint16_t num_rows) noexcept {
    const uint16_t w = rx_active_w_.load(std::memory_order_relaxed);
    const uint8_t cur_front = rx_front_idx_.load(std::memory_order_relaxed);
    const uint8_t next_buf  = cur_front ^ 1u;

    // Copy newly decoded scanlines into BOTH buffers with atomic index flip for tear-free reads!
    for (uint16_t r = 0; r < num_rows && (y_start + r) < kMaxHeight; ++r) {
        const size_t row_off = static_cast<size_t>(y_start + r) * kMaxWidth;
        std::memcpy(rx_published_bgra_[next_buf].data() + row_off,
                    rx_work_bgra_.data() + row_off,
                    static_cast<size_t>(w) * sizeof(uint32_t));
    }
    rx_front_idx_.store(next_buf, std::memory_order_release);
    for (uint16_t r = 0; r < num_rows && (y_start + r) < kMaxHeight; ++r) {
        const size_t row_off = static_cast<size_t>(y_start + r) * kMaxWidth;
        std::memcpy(rx_published_bgra_[cur_front].data() + row_off,
                    rx_work_bgra_.data() + row_off,
                    static_cast<size_t>(w) * sizeof(uint32_t));
    }
    rx_surface_seq_.fetch_add(1u, std::memory_order_release);
}

void SstvEngine::decode_completed_line(uint16_t unit_idx) noexcept {
    const SstvMode mode      = active_rx_mode_.load(std::memory_order_relaxed);
    const SstvModeSpec& spec = sstv_mode_spec(mode);
    const size_t n_samps     = std::min(line_sample_idx_, line_freq_buf_.size());
    if (n_samps < 32) return;

    std::span<const float> line_f{line_freq_buf_.data(), n_samps};
    const double spms = static_cast<double>(n_samps) / spec.line_period_ms;
    const uint16_t w  = spec.width;

    // Automatic 1200 Hz horizontal sync centroid alignment & soundcard clock slant tracking
    const double expected_sync_start_ms = (spec.encoding == SstvColorEncoding::GbrMidSync)
        ? (2.0 * spec.sep_ms + 2.0 * spec.luma_scan_ms)
        : 0.0;
    const double expected_sync_center = (expected_sync_start_ms + 0.5 * spec.sync_ms) * spms;
    const size_t win_radius = static_cast<size_t>(std::max(12.0, spec.sync_ms * spms * 1.25));
    const size_t s_lo = (expected_sync_center > static_cast<double>(win_radius))
                        ? static_cast<size_t>(expected_sync_center) - win_radius : 0;
    const size_t s_hi = std::min(n_samps, static_cast<size_t>(expected_sync_center) + win_radius);

    double weighted_pos = 0.0;
    double weight_sum   = 0.0;
    for (size_t i = s_lo; i < s_hi; ++i) {
        if (line_f[i] < 1330.0f && line_f[i] > 1050.0f) {
            const double wgt = 1340.0 - static_cast<double>(line_f[i]);
            weighted_pos += static_cast<double>(i) * wgt;
            weight_sum   += wgt;
        }
    }
    double sync_shift = 0.0;
    if (weight_sum > 250.0) {
        const double detected_center = weighted_pos / weight_sum;
        const double err = detected_center - expected_sync_center;
        if (std::abs(err) < spec.sync_ms * spms * 0.9) {
            sync_phase_err_ema_ = 0.80 * sync_phase_err_ema_ + 0.20 * err;
            sync_shift = sync_phase_err_ema_ * 0.35;
            const float est_ppm = static_cast<float>((sync_phase_err_ema_ / static_cast<double>(n_samps)) * 1.0e6);
            auto_slant_ppm_.store(est_ppm, std::memory_order_relaxed);
        }
    }

    alignas(64) std::array<uint8_t, kMaxWidth> c0{};
    alignas(64) std::array<uint8_t, kMaxWidth> c1{};
    alignas(64) std::array<uint8_t, kMaxWidth> c2{};
    alignas(64) std::array<uint8_t, kMaxWidth> c3{};

    switch (spec.encoding) {
        case SstvColorEncoding::GbrSequential: {
            const double g_start = (spec.sync_ms + spec.porch_ms) * spms + sync_shift;
            const double scan_s  = spec.luma_scan_ms * spms;
            const double sep_s   = spec.sep_ms * spms;
            const double b_start = g_start + scan_s + sep_s;
            const double r_start = b_start + scan_s + sep_s;

            sample_scan_segment(line_f, g_start, scan_s, w, c0);
            sample_scan_segment(line_f, b_start, scan_s, w, c1);
            sample_scan_segment(line_f, r_start, scan_s, w, c2);

            uint32_t* dst = rx_work_bgra_.data() + static_cast<size_t>(unit_idx) * kMaxWidth;
            for (uint16_t x = 0; x < w; ++x) {
                dst[x] = pack_bgra(c2[x], c0[x], c1[x]);
            }
            publish_rx_scanline_to_front(unit_idx, 1);
            rx_current_line_.store(static_cast<uint16_t>(unit_idx + 1), std::memory_order_release);
            break;
        }
        case SstvColorEncoding::GbrMidSync: {
            const double sep_s   = spec.sep_ms * spms;
            const double scan_s  = spec.luma_scan_ms * spms;
            const double g_start = sep_s + sync_shift;
            const double b_start = g_start + scan_s + sep_s;
            const double r_start = b_start + scan_s + (spec.sync_ms + spec.porch_ms) * spms;

            sample_scan_segment(line_f, g_start, scan_s, w, c0);
            sample_scan_segment(line_f, b_start, scan_s, w, c1);
            sample_scan_segment(line_f, r_start, scan_s, w, c2);

            uint32_t* dst = rx_work_bgra_.data() + static_cast<size_t>(unit_idx) * kMaxWidth;
            for (uint16_t x = 0; x < w; ++x) {
                dst[x] = pack_bgra(c2[x], c0[x], c1[x]);
            }
            publish_rx_scanline_to_front(unit_idx, 1);
            rx_current_line_.store(static_cast<uint16_t>(unit_idx + 1), std::memory_order_release);
            break;
        }
        case SstvColorEncoding::GrayscaleBw: {
            const double y_start = (spec.sync_ms + spec.porch_ms) * spms + sync_shift;
            const double scan_s  = spec.luma_scan_ms * spms;
            sample_scan_segment(line_f, y_start, scan_s, w, c0);

            uint32_t* dst = rx_work_bgra_.data() + static_cast<size_t>(unit_idx) * kMaxWidth;
            for (uint16_t x = 0; x < w; ++x) {
                dst[x] = pack_bgra(c0[x], c0[x], c0[x]);
            }
            publish_rx_scanline_to_front(unit_idx, 1);
            rx_current_line_.store(static_cast<uint16_t>(unit_idx + 1), std::memory_order_release);
            break;
        }
        case SstvColorEncoding::RobotYuv420: {
            const double y_start = (spec.sync_ms + spec.porch_ms) * spms + sync_shift;
            const double y_dur   = spec.luma_scan_ms * spms;
            const double sep_start = y_start + y_dur;
            const double sep_dur   = spec.sep_ms * spms;
            const double c_start   = sep_start + sep_dur + spec.chroma_porch_ms * spms;
            const double c_dur     = spec.chroma_scan_ms * spms;

            sample_scan_segment(line_f, y_start, y_dur, w, c0);
            sample_scan_segment(line_f, c_start, c_dur, w, c1);

            // Detect separator polarity (1500 Hz = even R-Y line, 2300 Hz = odd B-Y line)
            double sep_sum = 0.0;
            size_t sep_cnt = 0;
            const size_t sp0 = static_cast<size_t>(std::max(0.0, sep_start + 0.2 * sep_dur));
            const size_t sp1 = std::min(n_samps, static_cast<size_t>(sep_start + 0.8 * sep_dur));
            for (size_t i = sp0; i < sp1; ++i) { sep_sum += line_f[i]; ++sep_cnt; }
            const bool is_even_ry = (sep_cnt > 0) ? ((sep_sum / static_cast<double>(sep_cnt)) < 1900.0) : ((unit_idx & 1u) == 0);

            if (is_even_ry) {
                std::copy_n(c1.data(), w, last_cr_row_.data());
                std::copy_n(c0.data(), w, prev_y_row_.data());
                uint32_t* dst = rx_work_bgra_.data() + static_cast<size_t>(unit_idx) * kMaxWidth;
                for (uint16_t x = 0; x < w; ++x) {
                    dst[x] = sstv_ycrcb_to_bgra(c0[x], last_cr_row_[x], last_cb_row_[x]);
                }
                publish_rx_scanline_to_front(unit_idx, 1);
            } else {
                std::copy_n(c1.data(), w, last_cb_row_.data());
                // Refine previous even row now that exact paired B-Y has arrived!
                if (unit_idx > 0) {
                    uint32_t* prev_dst = rx_work_bgra_.data() + static_cast<size_t>(unit_idx - 1) * kMaxWidth;
                    for (uint16_t x = 0; x < w; ++x) {
                        prev_dst[x] = sstv_ycrcb_to_bgra(prev_y_row_[x], last_cr_row_[x], last_cb_row_[x]);
                    }
                }
                uint32_t* dst = rx_work_bgra_.data() + static_cast<size_t>(unit_idx) * kMaxWidth;
                for (uint16_t x = 0; x < w; ++x) {
                    dst[x] = sstv_ycrcb_to_bgra(c0[x], last_cr_row_[x], last_cb_row_[x]);
                }
                const uint16_t start_row = (unit_idx > 0) ? static_cast<uint16_t>(unit_idx - 1) : unit_idx;
                const uint16_t row_cnt   = (unit_idx > 0) ? 2u : 1u;
                publish_rx_scanline_to_front(start_row, row_cnt);
            }
            rx_current_line_.store(static_cast<uint16_t>(unit_idx + 1), std::memory_order_release);
            break;
        }
        case SstvColorEncoding::RobotYuv422: {
            const double y_start  = (spec.sync_ms + spec.porch_ms) * spms + sync_shift;
            const double y_dur    = spec.luma_scan_ms * spms;
            const double cr_start = y_start + y_dur + (spec.sep_ms + spec.chroma_porch_ms) * spms;
            const double c_dur    = spec.chroma_scan_ms * spms;
            const double cb_start = cr_start + c_dur + (spec.sep_ms + spec.chroma_porch_ms) * spms;

            sample_scan_segment(line_f, y_start,  y_dur, w, c0);
            sample_scan_segment(line_f, cr_start, c_dur, w, c1);
            sample_scan_segment(line_f, cb_start, c_dur, w, c2);

            uint32_t* dst = rx_work_bgra_.data() + static_cast<size_t>(unit_idx) * kMaxWidth;
            for (uint16_t x = 0; x < w; ++x) {
                dst[x] = sstv_ycrcb_to_bgra(c0[x], c1[x], c2[x]);
            }
            publish_rx_scanline_to_front(unit_idx, 1);
            rx_current_line_.store(static_cast<uint16_t>(unit_idx + 1), std::memory_order_release);
            break;
        }
        case SstvColorEncoding::PdDualLineYuv: {
            const uint16_t y0_idx = static_cast<uint16_t>(unit_idx * 2);
            if (y0_idx + 1 >= spec.height) break;

            const double seg_start = (spec.sync_ms + spec.porch_ms) * spms + sync_shift;
            const double seg_dur   = spec.luma_scan_ms * spms; // Y0, R-Y, B-Y, Y1 all share 121.600 ms

            sample_scan_segment(line_f, seg_start + 0.0 * seg_dur, seg_dur, w, c0); // Y0
            sample_scan_segment(line_f, seg_start + 1.0 * seg_dur, seg_dur, w, c1); // R-Y
            sample_scan_segment(line_f, seg_start + 2.0 * seg_dur, seg_dur, w, c2); // B-Y
            sample_scan_segment(line_f, seg_start + 3.0 * seg_dur, seg_dur, w, c3); // Y1

            uint32_t* dst0 = rx_work_bgra_.data() + static_cast<size_t>(y0_idx) * kMaxWidth;
            uint32_t* dst1 = rx_work_bgra_.data() + static_cast<size_t>(y0_idx + 1) * kMaxWidth;
            for (uint16_t x = 0; x < w; ++x) {
                dst0[x] = sstv_ycrcb_to_bgra(c0[x], c1[x], c2[x]);
                dst1[x] = sstv_ycrcb_to_bgra(c3[x], c1[x], c2[x]);
            }
            publish_rx_scanline_to_front(y0_idx, 2);
            rx_current_line_.store(static_cast<uint16_t>(y0_idx + 2), std::memory_order_release);
            break;
        }
        default:
            break;
    }
}

void SstvEngine::finalize_rx_image() noexcept {
    rx_completed_count_.fetch_add(1u, std::memory_order_release);
    rx_state_.store(SstvRxState::HuntingVis, std::memory_order_release);

    if (auto_save_png_.load(std::memory_order_acquire)) {
        std::string saved;
        save_current_rx_png(saved);
        if (!saved.empty()) {
            last_saved_png_ = std::move(saved);
        }
    }
}

void SstvEngine::process_freq_sample(float freq_hz, float mag_sq) noexcept {
    const SstvRxState state = rx_state_.load(std::memory_order_relaxed);

    if (state == SstvRxState::HuntingVis) {
        if (mag_sq < 1.0e-7f) {
            vis_leader1_samples_ = 0;
            vis_break_samples_   = 0;
            vis_leader2_samples_ = 0;
            vis_trans_samples_   = 0;
            vis_saw_break_       = false;
            return;
        }

        const bool in_1900 = (freq_hz >= 1800.0f && freq_hz <= 2000.0f);
        const bool in_1200 = (freq_hz >= 1080.0f && freq_hz <= 1340.0f);

        if (!vis_saw_break_) {
            if (in_1900) {
                ++vis_leader1_samples_;
                vis_trans_samples_ = 0;
            } else if (in_1200 && vis_leader1_samples_ >= 1200u) { // >= 100 ms initial 1900 Hz leader
                vis_saw_break_     = true;
                vis_break_samples_ = (vis_trans_samples_ / 2u) + 1u;
                vis_trans_samples_ = 0;
            } else {
                ++vis_trans_samples_;
                if (vis_trans_samples_ > 48u) { // > 4 ms out-of-band cancels leader
                    vis_leader1_samples_ = 0;
                    vis_trans_samples_   = 0;
                }
            }
        } else {
            if (vis_leader2_samples_ == 0) {
                if (in_1200) {
                    ++vis_break_samples_;
                    vis_trans_samples_ = 0;
                    if (vis_break_samples_ > 480u) { // > 40 ms break is too long for 10 ms VIS break
                        vis_saw_break_       = false;
                        vis_leader1_samples_ = 0;
                        vis_break_samples_   = 0;
                    }
                } else if (in_1900 && vis_break_samples_ >= 40u) { // >= 3.3 ms 1200 Hz break -> enter 2nd 1900 Hz leader
                    vis_leader2_samples_ = (vis_trans_samples_ / 2u) + 1u;
                    vis_trans_samples_   = 0;
                } else {
                    // Allow Hilbert FIR slew (~1 ms) between 1200 Hz break and 1900 Hz second leader
                    ++vis_trans_samples_;
                    if (vis_trans_samples_ > 48u) {
                        vis_saw_break_       = false;
                        vis_leader1_samples_ = 0;
                        vis_break_samples_   = 0;
                        vis_trans_samples_   = 0;
                    }
                }
            } else {
                if (in_1900) {
                    ++vis_leader2_samples_;
                    vis_trans_samples_ = 0;
                } else if (in_1200 && vis_leader2_samples_ >= 800u) { // >= 66 ms second 1900 Hz leader -> VIS Start Bit!
                    vis_bit_sample_      = (vis_trans_samples_ / 2u) + 1u;
                    vis_bit_index_       = 0u; // 0 = Start, 1..7 = LSB data, 8 = Parity, 9 = Stop
                    vis_shift_reg_       = 0u;
                    vis_bit_freq_accum_  = 0.0;
                    vis_bit_accum_cnt_   = 0u;
                    vis_leader1_samples_ = 0u;
                    vis_break_samples_   = 0u;
                    vis_leader2_samples_ = 0u;
                    vis_trans_samples_   = 0u;
                    vis_saw_break_       = false;
                    rx_state_.store(SstvRxState::DecodingVisBits, std::memory_order_release);
                } else {
                    // Allow Hilbert FIR slew (~1 ms) between 1900 Hz second leader and 1200 Hz start bit
                    ++vis_trans_samples_;
                    if (vis_trans_samples_ > 48u) {
                        vis_saw_break_       = false;
                        vis_leader1_samples_ = 0;
                        vis_break_samples_   = 0;
                        vis_leader2_samples_ = 0;
                        vis_trans_samples_   = 0;
                    }
                }
            }
        }
        return;
    }

    if (state == SstvRxState::DecodingVisBits) {
        constexpr uint32_t kSamplesPerVisBit = 360u; // 30 ms @ 12 kHz
        // Integrate middle 50% of each 30 ms bit cell (samples 90..270) for maximum noise immunity
        if (vis_bit_sample_ >= 90u && vis_bit_sample_ < 270u) {
            vis_bit_freq_accum_ += static_cast<double>(freq_hz);
            ++vis_bit_accum_cnt_;
        }
        ++vis_bit_sample_;

        if (vis_bit_sample_ >= kSamplesPerVisBit) {
            const double avg_hz = (vis_bit_accum_cnt_ > 0)
                ? (vis_bit_freq_accum_ / static_cast<double>(vis_bit_accum_cnt_))
                : 1500.0;

            if (vis_bit_index_ == 0u) {
                // Validate 1200 Hz VIS start bit
                if (avg_hz < 1050.0 || avg_hz > 1360.0) {
                    rx_state_.store(SstvRxState::HuntingVis, std::memory_order_release);
                    return;
                }
            } else if (vis_bit_index_ >= 1u && vis_bit_index_ <= 8u) {
                // 1100 Hz = logic '1', 1300 Hz = logic '0'
                const uint8_t bit_val = (avg_hz < 1200.0) ? 1u : 0u;
                vis_shift_reg_ |= static_cast<uint8_t>(bit_val << (vis_bit_index_ - 1u));
            } else if (vis_bit_index_ == 9u) {
                // VIS Stop Bit completed! Decode 7-bit VIS + check even parity
                const uint8_t vis7 = vis_shift_reg_ & 0x7Fu;
                uint8_t ones = 0;
                for (int b = 0; b < 8; ++b) {
                    if ((vis_shift_reg_ >> b) & 1u) ++ones;
                }
                const bool parity_ok = ((ones & 1u) == 0u);

                SstvMode detected = manual_rx_mode_.load(std::memory_order_acquire);
                if (!auto_vis_.load(std::memory_order_acquire)) {
                    begin_image_rx(detected);
                } else if (find_sstv_mode_by_vis(vis7, detected) && (parity_ok || (avg_hz >= 1080.0 && avg_hz <= 1340.0))) {
                    begin_image_rx(detected);
                } else {
                    rx_state_.store(SstvRxState::HuntingVis, std::memory_order_release);
                }
                return;
            }

            ++vis_bit_index_;
            vis_bit_sample_     = 0u;
            vis_bit_freq_accum_ = 0.0;
            vis_bit_accum_cnt_  = 0u;
        }
        return;
    }

    // SstvRxState::ReceivingImage
    if (scottie_first_sync_) {
        // Consume initial 9.0 ms (108 samples @ 12 kHz) Scottie starting sync pulse before Line 0
        ++line_sample_idx_;
        const SstvModeSpec& spec = sstv_mode_spec(active_rx_mode_.load(std::memory_order_relaxed));
        const size_t init_sync_samps = static_cast<size_t>(std::llround(spec.sync_ms * static_cast<double>(kRouterSampleRateHz) / 1000.0));
        if (line_sample_idx_ >= init_sync_samps) {
            scottie_first_sync_ = false;
            line_sample_idx_    = 0;
            line_frac_accum_    = 0.0;
        }
        return;
    }

    if (line_sample_idx_ < line_freq_buf_.size()) {
        line_freq_buf_[line_sample_idx_] = freq_hz;
    }
    ++line_sample_idx_;

    const double ppm = static_cast<double>(slant_ppm_.load(std::memory_order_relaxed));
    const double adjusted_period = line_sample_target_ * (1.0 + ppm * 1.0e-6);
    const double target_with_frac = adjusted_period + line_frac_accum_;
    const size_t int_target = static_cast<size_t>(std::max(32.0, std::floor(target_with_frac + 0.5)));

    if (line_sample_idx_ >= int_target) {
        line_frac_accum_ = target_with_frac - static_cast<double>(int_target);
        decode_completed_line(current_unit_idx_);
        line_sample_idx_ = 0;
        ++current_unit_idx_;
        if (current_unit_idx_ >= total_units_) {
            finalize_rx_image();
        }
    }
}

void SstvEngine::process_rx_samples_sync(std::span<const float> mono_12k) noexcept {
    if (clear_requested_.exchange(false, std::memory_order_acq_rel)) {
        const uint32_t bg = pack_bgra(12, 16, 24);
        rx_work_bgra_.fill(bg);
        rx_published_bgra_[0].fill(bg);
        rx_published_bgra_[1].fill(bg);
        rx_current_line_.store(0, std::memory_order_release);
        rx_state_.store(SstvRxState::HuntingVis, std::memory_order_release);
        rx_surface_seq_.fetch_add(1u, std::memory_order_release);
    }
    if (force_rx_requested_.exchange(false, std::memory_order_acq_rel)) {
        begin_image_rx(manual_rx_mode_.load(std::memory_order_acquire));
    }

    size_t offset = 0;
    while (offset < mono_12k.size()) {
        const size_t chunk = std::min(mono_12k.size() - offset, rx_freq_scratch_.size());
        std::span<const float> sub_in{mono_12k.data() + offset, chunk};
        std::span<float> sub_freq{rx_freq_scratch_.data(), chunk};

        update_waterfall_fft(sub_in);
        demodulate_fm_block(sub_in, sub_freq);

        // Estimate average block energy for VIS squelch gate
        float energy = 0.0f;
        for (float v : sub_in) energy += v * v;
        const float mean_sq = (chunk > 0) ? (energy / static_cast<float>(chunk)) : 0.0f;

        for (size_t i = 0; i < chunk; ++i) {
            process_freq_sample(sub_freq[i], mean_sq);
        }
        offset += chunk;
    }
}

void SstvEngine::dsp_worker_loop() {
    while (worker_running_.load(std::memory_order_acquire)) {
        const size_t got = router_.read_active_stream(rx_tap_cursor_, rx_tap_scratch_);
        if (got == 0) {
            if (clear_requested_.load(std::memory_order_acquire) ||
                force_rx_requested_.load(std::memory_order_acquire)) {
                process_rx_samples_sync(std::span<const float>{});
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(6));
            continue;
        }
        process_rx_samples_sync(std::span<const float>{rx_tap_scratch_.data(), got});
    }
}

// ============================================================================
// Zero-Dependency RFC 1950/1951 PNG Auto-Saver
// ============================================================================
bool SstvEngine::write_bgra_png(const std::string& filepath,
                                std::span<const uint32_t> bgra,
                                uint16_t width,
                                uint16_t height,
                                uint16_t stride_px) {
    if (width == 0 || height == 0 || bgra.size() < static_cast<size_t>(stride_px) * height) {
        return false;
    }

    // Build raw scanlines: 1 filter byte (0x00) + RGB24 bytes per row
    const size_t row_bytes = 1u + static_cast<size_t>(width) * 3u;
    std::vector<uint8_t> raw;
    raw.reserve(row_bytes * height);
    for (uint16_t y = 0; y < height; ++y) {
        raw.push_back(0x00u); // Filter Type 0 (None)
        const uint32_t* src_row = bgra.data() + static_cast<size_t>(y) * stride_px;
        for (uint16_t x = 0; x < width; ++x) {
            const uint32_t px = src_row[x];
            raw.push_back(bgra_r(px));
            raw.push_back(bgra_g(px));
            raw.push_back(bgra_b(px));
        }
    }

    // Wrap raw scanlines in valid zlib stream (CMF=0x78, FLG=0x01 + uncompressed DEFLATE blocks + Adler32)
    std::vector<uint8_t> zstream;
    zstream.reserve(raw.size() + (raw.size() / 65535u + 2u) * 5u + 8u);
    zstream.push_back(0x78u);
    zstream.push_back(0x01u);

    uint32_t s1 = 1u, s2 = 0u;
    size_t offset = 0;
    while (offset < raw.size()) {
        const size_t blk = std::min<size_t>(65535u, raw.size() - offset);
        const bool is_last = (offset + blk == raw.size());
        zstream.push_back(is_last ? 0x01u : 0x00u);
        const uint16_t len16  = static_cast<uint16_t>(blk);
        const uint16_t nlen16 = static_cast<uint16_t>(~len16);
        zstream.push_back(static_cast<uint8_t>( len16        & 0xFFu));
        zstream.push_back(static_cast<uint8_t>((len16 >> 8)  & 0xFFu));
        zstream.push_back(static_cast<uint8_t>( nlen16       & 0xFFu));
        zstream.push_back(static_cast<uint8_t>((nlen16 >> 8) & 0xFFu));

        for (size_t i = 0; i < blk; ++i) {
            const uint8_t b = raw[offset + i];
            zstream.push_back(b);
            s1 = (s1 + b) % 65521u;
            s2 = (s2 + s1) % 65521u;
        }
        offset += blk;
    }
    write_be32(zstream, (s2 << 16) | s1);

    std::vector<uint8_t> png;
    png.reserve(zstream.size() + 64u);
    constexpr std::array<uint8_t, 8> kPngSig{{137u, 80u, 78u, 71u, 13u, 10u, 26u, 10u}};
    png.insert(png.end(), kPngSig.begin(), kPngSig.end());

    std::array<uint8_t, 13> ihdr{};
    ihdr[0] = static_cast<uint8_t>((width  >> 24) & 0xFFu);
    ihdr[1] = static_cast<uint8_t>((width  >> 16) & 0xFFu);
    ihdr[2] = static_cast<uint8_t>((width  >> 8)  & 0xFFu);
    ihdr[3] = static_cast<uint8_t>( width         & 0xFFu);
    ihdr[4] = static_cast<uint8_t>((height >> 24) & 0xFFu);
    ihdr[5] = static_cast<uint8_t>((height >> 16) & 0xFFu);
    ihdr[6] = static_cast<uint8_t>((height >> 8)  & 0xFFu);
    ihdr[7] = static_cast<uint8_t>( height        & 0xFFu);
    ihdr[8] = 8u; // 8 bits per channel
    ihdr[9] = 2u; // Color type 2 = Truecolor RGB

    append_png_chunk(png, "IHDR", ihdr);
    append_png_chunk(png, "IDAT", zstream);
    append_png_chunk(png, "IEND", std::span<const uint8_t>{});

    std::ofstream out(filepath, std::ios::binary);
    if (!out) return false;
    out.write(reinterpret_cast<const char*>(png.data()), static_cast<std::streamsize>(png.size()));
    return out.good();
}

bool SstvEngine::save_current_rx_png(std::string& out_saved_path) const {
    hamshack::hs_mkdir_p(capture_dir_);

    const std::time_t now_t = std::time(nullptr);
    std::tm utc_tm{};
    hamshack::hs_gmtime(&now_t, &utc_tm);
    const SstvModeSpec& spec = sstv_mode_spec(active_rx_mode_.load(std::memory_order_acquire));

    std::string mode_tag;
    for (char c : spec.short_name) {
        mode_tag.push_back((c == ' ') ? '_' : c);
    }

    std::array<char, 256> path_buf{};
    std::snprintf(path_buf.data(), path_buf.size(), "%s/SSTV_%04d%02d%02d_%02d%02d%02d_%s.png",
                  capture_dir_.c_str(),
                  utc_tm.tm_year + 1900, utc_tm.tm_mon + 1, utc_tm.tm_mday,
                  utc_tm.tm_hour, utc_tm.tm_min, utc_tm.tm_sec,
                  mode_tag.c_str());

    uint16_t w = 0, h = 0;
    const auto surf = active_rx_surface(w, h);
    if (!write_bgra_png(path_buf.data(), surf, w, h, kMaxWidth)) {
        return false;
    }
    out_saved_path = std::string(path_buf.data());
    return true;
}

// ============================================================================
// TX Image Loading, Center-Crop Auto-Scaler, 1-Bit Floyd-Steinberg & Test Card
// ============================================================================
void SstvEngine::generate_default_test_card() {
    constexpr uint16_t kW = 640;
    constexpr uint16_t kH = 480;
    tx_source_w_ = kW;
    tx_source_h_ = kH;
    tx_source_bgra_.assign(static_cast<size_t>(kW) * kH, pack_bgra(0, 0, 0));

    // Classic 75%/100% SMPTE / Amateur Radio Color Bar + Grayscale Ramp Test Pattern
    constexpr std::array<uint32_t, 8> kBars{{
        pack_bgra(240, 240, 240), // White
        pack_bgra(235, 225,  20), // Yellow
        pack_bgra( 20, 225, 235), // Cyan
        pack_bgra( 30, 220,  45), // Green
        pack_bgra(225,  35, 225), // Magenta
        pack_bgra(230,  35,  35), // Red
        pack_bgra( 35,  65, 235), // Blue
        pack_bgra( 18,  22,  30)  // Dark Slate
    }};

    for (uint16_t y = 0; y < kH; ++y) {
        uint32_t* row = tx_source_bgra_.data() + static_cast<size_t>(y) * kW;
        if (y < (kH * 3) / 4) {
            for (uint16_t x = 0; x < kW; ++x) {
                const size_t bi = std::min<size_t>(7, (static_cast<size_t>(x) * 8) / kW);
                row[x] = kBars[bi];
            }
        } else if (y < (kH * 7) / 8) {
            // Smooth linear grayscale ramp for SSTV linearity & slant checks
            for (uint16_t x = 0; x < kW; ++x) {
                const uint8_t g = static_cast<uint8_t>((static_cast<uint32_t>(x) * 255u) / (kW - 1));
                row[x] = pack_bgra(g, g, g);
            }
        } else {
            // High-frequency multiburst / checkerboard bar
            for (uint16_t x = 0; x < kW; ++x) {
                const bool hi = (((x / 16) + (y / 16)) & 1) == 0;
                row[x] = hi ? pack_bgra(235, 235, 235) : pack_bgra(24, 28, 36);
            }
        }
    }
    rebuild_tx_preview();
}

bool SstvEngine::set_tx_source_bgra(std::span<const uint32_t> pixels, uint16_t width, uint16_t height) {
    if (width == 0 || height == 0 || pixels.size() < static_cast<size_t>(width) * height) {
        return false;
    }
    tx_source_w_ = width;
    tx_source_h_ = height;
    tx_source_bgra_.assign(pixels.begin(), pixels.begin() + static_cast<std::ptrdiff_t>(static_cast<size_t>(width) * height));
    rebuild_tx_preview();
    return true;
}

bool SstvEngine::load_tx_image_file(const std::wstring& path, std::string& out_error) {
    if (path.empty()) {
        out_error = "No image path provided.";
        return false;
    }

#ifdef _WIN32
    const HRESULT co_hr = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    const bool need_uninit = (co_hr == S_OK || co_hr == S_FALSE);
    struct CoGuard {
        bool active;
        ~CoGuard() { if (active) CoUninitialize(); }
    } co_guard{need_uninit};

    IWICImagingFactory* wic = nullptr;
    HRESULT hr = CoCreateInstance(CLSID_WICImagingFactory, nullptr, CLSCTX_INPROC_SERVER,
                                  IID_PPV_ARGS(&wic));
    if (FAILED(hr) || !wic) {
        out_error = "Failed to initialize Windows Imaging Component (WIC).";
        return false;
    }
    struct ComReleaser {
        IUnknown* p;
        ~ComReleaser() { if (p) p->Release(); }
    };
    ComReleaser wic_rel{wic};

    IWICBitmapDecoder* decoder = nullptr;
    hr = wic->CreateDecoderFromFilename(path.c_str(), nullptr, GENERIC_READ,
                                        WICDecodeMetadataCacheOnLoad, &decoder);
    if (FAILED(hr) || !decoder) {
        out_error = "WIC could not decode selected image (PNG/JPEG/BMP).";
        return false;
    }
    ComReleaser dec_rel{decoder};

    IWICBitmapFrameDecode* frame = nullptr;
    hr = decoder->GetFrame(0, &frame);
    if (FAILED(hr) || !frame) {
        out_error = "Failed to read image frame.";
        return false;
    }
    ComReleaser frm_rel{frame};

    UINT w = 0, h = 0;
    frame->GetSize(&w, &h);
    if (w == 0 || h == 0 || w > 8192 || h > 8192) {
        out_error = "Invalid image dimensions.";
        return false;
    }

    IWICFormatConverter* conv = nullptr;
    hr = wic->CreateFormatConverter(&conv);
    if (FAILED(hr) || !conv) {
        out_error = "Failed to create WIC BGRA32 converter.";
        return false;
    }
    ComReleaser conv_rel{conv};

    hr = conv->Initialize(frame, GUID_WICPixelFormat32bppBGRA, WICBitmapDitherTypeNone,
                          nullptr, 0.0, WICBitmapPaletteTypeCustom);
    if (FAILED(hr)) {
        out_error = "Failed to initialize 32-bit BGRA pixel conversion.";
        return false;
    }

    std::vector<uint32_t> pixels(static_cast<size_t>(w) * static_cast<size_t>(h));
    hr = conv->CopyPixels(nullptr, w * 4u, static_cast<UINT>(pixels.size() * 4u),
                          reinterpret_cast<BYTE*>(pixels.data()));
    if (FAILED(hr)) {
        out_error = "Failed to copy decoded image pixels.";
        return false;
    }

    tx_source_w_    = static_cast<uint16_t>(w);
    tx_source_h_    = static_cast<uint16_t>(h);
    tx_source_bgra_ = std::move(pixels);
    rebuild_tx_preview();
    return true;

#elif defined(__APPLE__)
    std::string utf8_path;
    utf8_path.reserve(path.size());
    for (wchar_t wc : path) {
        if (wc < 0x80) utf8_path.push_back(static_cast<char>(wc));
    }
    std::ifstream f(utf8_path, std::ios::binary);
    if (!f) {
        out_error = "Could not open selected image file.";
        return false;
    }
    std::vector<uint8_t> bytes((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
    if (bytes.empty()) {
        out_error = "Image file is empty.";
        return false;
    }

    CFDataRef cf_data = CFDataCreate(nullptr, bytes.data(), static_cast<CFIndex>(bytes.size()));
    if (!cf_data) {
        out_error = "CFData allocation failed.";
        return false;
    }
    CGImageSourceRef src = CGImageSourceCreateWithData(cf_data, nullptr);
    CFRelease(cf_data);
    if (!src) {
        out_error = "ImageIO could not parse image data.";
        return false;
    }
    CGImageRef cg_img = CGImageSourceCreateImageAtIndex(src, 0, nullptr);
    CFRelease(src);
    if (!cg_img) {
        out_error = "ImageIO failed to decode image frame.";
        return false;
    }

    const size_t raw_w = CGImageGetWidth(cg_img);
    const size_t raw_h = CGImageGetHeight(cg_img);
    if (raw_w == 0 || raw_h == 0) {
        CGImageRelease(cg_img);
        out_error = "Invalid image dimensions.";
        return false;
    }

    const uint16_t w = static_cast<uint16_t>(std::min<size_t>(1280, raw_w));
    const uint16_t h = static_cast<uint16_t>(std::min<size_t>(1280, raw_h));
    std::vector<uint32_t> pixels(static_cast<size_t>(w) * h, 0u);

    CGColorSpaceRef cs = CGColorSpaceCreateDeviceRGB();
    // kCGImageAlphaPremultipliedFirst (2) | kCGBitmapByteOrder32Little (2 << 12) => native BGRA32!
    constexpr uint32_t kBgra32LittleFlags = 2u | (2u << 12);
    CGContextRef ctx = CGBitmapContextCreate(pixels.data(), w, h, 8, static_cast<size_t>(w) * 4u, cs, kBgra32LittleFlags);
    if (cs) CGColorSpaceRelease(cs);
    if (!ctx) {
        CGImageRelease(cg_img);
        out_error = "CoreGraphics BGRA32 context creation failed.";
        return false;
    }
    CGContextSetInterpolationQuality(ctx, 3);
    CGContextDrawImage(ctx, CGRect{{0.0, 0.0}, {static_cast<double>(w), static_cast<double>(h)}}, cg_img);
    CGContextRelease(ctx);
    CGImageRelease(cg_img);

    tx_source_w_    = w;
    tx_source_h_    = h;
    tx_source_bgra_ = std::move(pixels);
    rebuild_tx_preview();
    return true;
#else
    out_error = "Native OS image decoder unavailable on this target.";
    return false;
#endif
}

void SstvEngine::set_tx_mode(SstvMode mode) {
    tx_mode_.store(mode, std::memory_order_release);
    rebuild_tx_preview();
}

void SstvEngine::set_tx_dither_1bit(bool enabled) {
    tx_dither_1bit_.store(enabled, std::memory_order_release);
    rebuild_tx_preview();
}

void SstvEngine::set_tx_overlay_text(std::string_view text) {
    tx_overlay_text_ = std::string(text.substr(0, std::min<size_t>(40, text.size())));
    rebuild_tx_preview();
}

void SstvEngine::rebuild_tx_preview() {
    if (tx_source_bgra_.empty() || tx_source_w_ == 0 || tx_source_h_ == 0) {
        return;
    }

    const SstvModeSpec& spec = sstv_mode_spec(tx_mode_.load(std::memory_order_acquire));
    const uint16_t dst_w = spec.width;
    const uint16_t dst_h = spec.height;
    tx_preview_w_ = dst_w;
    tx_preview_h_ = dst_h;
    tx_preview_bgra_.fill(pack_bgra(0, 0, 0));

    // 1. Aspect-ratio preserving Center-Crop + Bilinear Resampling into [dst_w x dst_h]
    const double src_aspect = static_cast<double>(tx_source_w_) / static_cast<double>(tx_source_h_);
    const double dst_aspect = static_cast<double>(dst_w) / static_cast<double>(dst_h);

    double crop_x = 0.0, crop_y = 0.0;
    double crop_w = static_cast<double>(tx_source_w_);
    double crop_h = static_cast<double>(tx_source_h_);

    if (src_aspect > dst_aspect) {
        crop_w = crop_h * dst_aspect;
        crop_x = 0.5 * (static_cast<double>(tx_source_w_) - crop_w);
    } else {
        crop_h = crop_w / dst_aspect;
        crop_y = 0.5 * (static_cast<double>(tx_source_h_) - crop_h);
    }

    for (uint16_t y = 0; y < dst_h; ++y) {
        const double sy = crop_y + ((static_cast<double>(y) + 0.5) / static_cast<double>(dst_h)) * crop_h - 0.5;
        const int y0 = std::clamp(static_cast<int>(std::floor(sy)), 0, static_cast<int>(tx_source_h_) - 1);
        const int y1 = std::min(y0 + 1, static_cast<int>(tx_source_h_) - 1);
        const float fy = static_cast<float>(std::clamp(sy - static_cast<double>(y0), 0.0, 1.0));

        uint32_t* dst_row = tx_preview_bgra_.data() + static_cast<size_t>(y) * kMaxWidth;
        for (uint16_t x = 0; x < dst_w; ++x) {
            const double sx = crop_x + ((static_cast<double>(x) + 0.5) / static_cast<double>(dst_w)) * crop_w - 0.5;
            const int x0 = std::clamp(static_cast<int>(std::floor(sx)), 0, static_cast<int>(tx_source_w_) - 1);
            const int x1 = std::min(x0 + 1, static_cast<int>(tx_source_w_) - 1);
            const float fx = static_cast<float>(std::clamp(sx - static_cast<double>(x0), 0.0, 1.0));

            const uint32_t p00 = tx_source_bgra_[static_cast<size_t>(y0) * tx_source_w_ + x0];
            const uint32_t p10 = tx_source_bgra_[static_cast<size_t>(y0) * tx_source_w_ + x1];
            const uint32_t p01 = tx_source_bgra_[static_cast<size_t>(y1) * tx_source_w_ + x0];
            const uint32_t p11 = tx_source_bgra_[static_cast<size_t>(y1) * tx_source_w_ + x1];

            auto bilerp = [&](auto chan) noexcept -> uint8_t {
                const float top = static_cast<float>(chan(p00)) + fx * (static_cast<float>(chan(p10)) - static_cast<float>(chan(p00)));
                const float bot = static_cast<float>(chan(p01)) + fx * (static_cast<float>(chan(p11)) - static_cast<float>(chan(p01)));
                return clamp_u8(top + fy * (bot - top));
            };

            uint8_t r = bilerp(bgra_r);
            uint8_t g = bilerp(bgra_g);
            uint8_t b = bilerp(bgra_b);

            if (spec.encoding == SstvColorEncoding::GrayscaleBw) {
                const uint8_t luma = rgb_to_luma_bw(r, g, b);
                r = g = b = luma;
            }
            dst_row[x] = pack_bgra(r, g, b);
        }
    }

    // 2. Optional 1-Bit Serpentine Floyd-Steinberg Dithering (High-Contrast Retro / Apple II Mode)
    const bool dither_1bit = tx_dither_1bit_.load(std::memory_order_acquire);
    if (dither_1bit) {
        std::array<float, kMaxWidth + 2> cur_err{};
        std::array<float, kMaxWidth + 2> nxt_err{};

        for (uint16_t y = 0; y < dst_h; ++y) {
            nxt_err.fill(0.0f);
            uint32_t* row = tx_preview_bgra_.data() + static_cast<size_t>(y) * kMaxWidth;
            const bool rev = ((y & 1u) != 0u);

            for (uint16_t step = 0; step < dst_w; ++step) {
                const uint16_t x = rev ? static_cast<uint16_t>(dst_w - 1 - step) : step;
                const uint8_t lum0 = rgb_to_luma_bw(bgra_r(row[x]), bgra_g(row[x]), bgra_b(row[x]));
                const float old_p  = std::clamp(static_cast<float>(lum0) + cur_err[x + 1], 0.0f, 255.0f);
                const float new_p  = (old_p >= 128.0f) ? 255.0f : 0.0f;
                const float quant_err = old_p - new_p;

                const uint8_t bw = static_cast<uint8_t>(new_p);
                row[x] = pack_bgra(bw, bw, bw);

                if (!rev) {
                    cur_err[x + 2] += quant_err * (7.0f / 16.0f);
                    nxt_err[x + 0] += quant_err * (3.0f / 16.0f);
                    nxt_err[x + 1] += quant_err * (5.0f / 16.0f);
                    nxt_err[x + 2] += quant_err * (1.0f / 16.0f);
                } else {
                    cur_err[x + 0] += quant_err * (7.0f / 16.0f);
                    nxt_err[x + 2] += quant_err * (3.0f / 16.0f);
                    nxt_err[x + 1] += quant_err * (5.0f / 16.0f);
                    nxt_err[x + 0] += quant_err * (1.0f / 16.0f);
                }
            }
            cur_err = nxt_err;
        }
    }

    // 3. Burn Customizable Callsign / Grid / RST Text Overlay into TX Raster
    if (!tx_overlay_text_.empty()) {
        burn_text_overlay(tx_preview_bgra_, dst_w, dst_h, kMaxWidth, tx_overlay_text_, dither_1bit);
    }

    tx_preview_seq_.fetch_add(1u, std::memory_order_release);
}

// ============================================================================
// Asynchronous Lock-Free SSTV TX Worker -> AudioRouter 48 kHz 16-Bit SPSC Ring
// ============================================================================
bool SstvEngine::start_transmit() {
    if (tx_running_.exchange(true, std::memory_order_acq_rel)) {
        return false; // Already transmitting
    }
    if (tx_thread_.joinable()) {
        tx_thread_.join();
    }

    // Snapshot TX raster so UI edits during TX never tear the live transmission!
    tx_active_snap_bgra_ = tx_preview_bgra_;
    const SstvMode mode  = tx_mode_.load(std::memory_order_acquire);
    const SstvModeSpec& spec = sstv_mode_spec(mode);

    tx_abort_.store(false, std::memory_order_release);
    tx_progress_.store(0.0f, std::memory_order_release);
    tx_thread_ = std::thread(&SstvEngine::tx_worker_loop, this, mode, spec.width, spec.height);
    return true;
}

void SstvEngine::abort_transmit() noexcept {
    tx_abort_.store(true, std::memory_order_release);
    router_.abort_tx();
}

void SstvEngine::tx_worker_loop(SstvMode mode, uint16_t, uint16_t) {
    SstvModulator::modulate_image(
        mode,
        tx_active_snap_bgra_,
        kMaxWidth,
        kTxOutputSampleRateHz,
        [this](std::span<const int16_t> chunk, bool is_first, bool is_last, float synthesis_prog) noexcept -> bool {
            if (tx_abort_.load(std::memory_order_acquire)) {
                return false;
            }

            // Pace SPSC ring smoothly so we keep ~1.5s of lookahead queued without overflowing
            while (!tx_abort_.load(std::memory_order_acquire) &&
                   router_.tx_free_space() < chunk.size() + 2048u) {
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
            }
            if (tx_abort_.load(std::memory_order_acquire)) {
                return false;
            }

            // First chunk applies 1.5 ms cosine fade-in; final chunk applies 1.5 ms cosine fade-out!
            router_.queue_tx_pcm48(chunk, is_first, is_last);
            tx_progress_.store(synthesis_prog * 0.96f, std::memory_order_release);
            return true;
        });

    // Wait for remaining queued TX samples to finish draining out to the radio (unless aborted)
    const size_t initial_tail = router_.tx_buffered_samples();
    while (!tx_abort_.load(std::memory_order_acquire)) {
        const size_t rem = router_.tx_buffered_samples();
        if (rem == 0) break;
        if (initial_tail > 0) {
            const float drain_frac = 1.0f - static_cast<float>(rem) / static_cast<float>(initial_tail);
            tx_progress_.store(std::clamp(0.96f + 0.04f * drain_frac, 0.96f, 1.0f), std::memory_order_release);
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(15));
    }

    tx_progress_.store(tx_abort_.load(std::memory_order_acquire) ? 0.0f : 1.0f, std::memory_order_release);
    tx_running_.store(false, std::memory_order_release);
}

std::span<const uint32_t> SstvEngine::active_rx_surface(uint16_t& out_w, uint16_t& out_h) const noexcept {
    out_w = rx_active_w_.load(std::memory_order_acquire);
    out_h = rx_active_h_.load(std::memory_order_acquire);
    const uint8_t front = rx_front_idx_.load(std::memory_order_acquire) & 1u;
    return std::span<const uint32_t>{rx_published_bgra_[front].data(), kMaxPixels};
}

std::span<const uint32_t> SstvEngine::tx_preview_surface(uint16_t& out_w, uint16_t& out_h) const noexcept {
    out_w = tx_preview_w_;
    out_h = tx_preview_h_;
    return std::span<const uint32_t>{tx_preview_bgra_.data(), kMaxPixels};
}

SstvTelemetry SstvEngine::telemetry() const noexcept {
    SstvTelemetry t{};
    t.rx_state            = rx_state_.load(std::memory_order_acquire);
    t.active_rx_mode      = active_rx_mode_.load(std::memory_order_acquire);
    t.tx_mode             = tx_mode_.load(std::memory_order_acquire);
    t.auto_vis_enabled    = auto_vis_.load(std::memory_order_acquire);
    t.auto_save_png       = auto_save_png_.load(std::memory_order_acquire);
    t.tx_active           = tx_running_.load(std::memory_order_acquire) || router_.is_tx_active();
    t.tx_dither_1bit      = tx_dither_1bit_.load(std::memory_order_acquire);
    t.rx_current_line     = rx_current_line_.load(std::memory_order_acquire);
    t.rx_total_lines      = rx_total_lines_.load(std::memory_order_acquire);
    t.rx_width            = rx_active_w_.load(std::memory_order_acquire);
    t.rx_height           = rx_active_h_.load(std::memory_order_acquire);
    t.slant_ppm           = slant_ppm_.load(std::memory_order_acquire);
    t.auto_slant_ppm      = auto_slant_ppm_.load(std::memory_order_acquire);
    t.inst_freq_hz        = inst_freq_hz_.load(std::memory_order_relaxed);
    t.tx_progress         = tx_progress_.load(std::memory_order_acquire);
    t.rx_completed_images = rx_completed_count_.load(std::memory_order_acquire);
    t.rx_surface_seq      = rx_surface_seq_.load(std::memory_order_acquire);
    t.wf_surface_seq      = wf_seq_.load(std::memory_order_acquire);
    t.tx_preview_seq      = tx_preview_seq_.load(std::memory_order_acquire);
    return t;
}

} // namespace hamshack::carbon
