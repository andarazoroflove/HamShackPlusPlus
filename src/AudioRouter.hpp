#pragma once
// ============================================================================
// AudioRouter.hpp — Carbon Engine Zero-Copy Lock-Free Dual-Source Audio Router
// Dynamically routes RX (12 kHz DSP tap) and TX (48 kHz 16-bit PCM SPSC stream)
// between:
//   - Source A (PrimaryG90): Xiegu G90 HF transceiver stream (shared zero-copy tap)
//   - Source B (UsbDongleVhf): Dedicated USB Audio Dongle (BTECH APRS-K1 / UV-5R)
// Strictly adheres to Carbon Engine Rules:
//   - C++20 RAII, std::unique_ptr, std::span, std::string_view (zero raw new/delete)
//   - Zero dynamic allocations inside real-time audio callbacks or DSP read paths
//   - Cache-line padded (alignas(64)) lock-free SPSC & monotonic broadcast rings
// ============================================================================

#include <algorithm>
#include <array>
#include <atomic>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace hamshack::carbon {

inline constexpr int    kRouterSampleRateHz   = 12000;
inline constexpr int    kTxOutputSampleRateHz = 48000;
inline constexpr size_t kTxCosineRampSamples  = 72; // 1.5 ms @ 48 kHz raised-cosine anti-click taper
inline constexpr size_t kHardwareCacheLine    = 64;

enum class AudioSource : uint8_t {
    PrimaryG90   = 0, // Source A: Xiegu G90 HF Transceiver stream
    UsbDongleVhf = 1  // Source B: Dedicated USB Audio Dongle (BTECH APRS / UV-5R)
};

[[nodiscard]] constexpr std::string_view audio_source_label(AudioSource src) noexcept {
    return (src == AudioSource::PrimaryG90)
        ? std::string_view{"Source A: Xiegu G90 (HF Primary)"}
        : std::string_view{"Source B: USB Audio Dongle (VHF / UV-5R)"};
}

[[nodiscard]] constexpr std::string_view audio_source_short_label(AudioSource src) noexcept {
    return (src == AudioSource::PrimaryG90)
        ? std::string_view{"Xiegu G90 (HF)"}
        : std::string_view{"USB Dongle (UV-5R)"};
}

// ============================================================================
// LockFreeSpscRing<T, CapacityPow2>
// Cache-line isolated Single-Producer Single-Consumer bounded ring buffer.
// Zero heap allocations, zero mutexes, deterministic O(N) span transfers.
// ============================================================================
template <typename T, size_t CapacityPow2>
class LockFreeSpscRing {
    static_assert((CapacityPow2 >= 2) && ((CapacityPow2 & (CapacityPow2 - 1)) == 0),
                  "CapacityPow2 must be an exact power of two");
public:
    static constexpr size_t kCapacity = CapacityPow2;
    static constexpr size_t kMask     = CapacityPow2 - 1;

    constexpr LockFreeSpscRing() noexcept = default;

    LockFreeSpscRing(const LockFreeSpscRing&) = delete;
    LockFreeSpscRing& operator=(const LockFreeSpscRing&) = delete;

    // Producer-only: push contiguous span of items; drops excess if full (never blocks RT thread).
    size_t push_span(std::span<const T> items) noexcept {
        const size_t w = write_idx_.load(std::memory_order_relaxed);
        const size_t r = read_idx_.load(std::memory_order_acquire);
        const size_t avail = kCapacity - (w - r);
        const size_t to_write = std::min(items.size(), avail);
        for (size_t i = 0; i < to_write; ++i) {
            storage_[(w + i) & kMask] = items[i];
        }
        write_idx_.store(w + to_write, std::memory_order_release);
        return to_write;
    }

    bool try_push(const T& item) noexcept {
        return push_span(std::span<const T>{&item, 1}) == 1;
    }

    // Consumer-only: pop up to dst.size() items into caller-provided pre-allocated span.
    size_t pop_span(std::span<T> dst) noexcept {
        const size_t r = read_idx_.load(std::memory_order_relaxed);
        const size_t w = write_idx_.load(std::memory_order_acquire);
        const size_t ready = w - r;
        const size_t to_read = std::min(dst.size(), ready);
        for (size_t i = 0; i < to_read; ++i) {
            dst[i] = storage_[(r + i) & kMask];
        }
        read_idx_.store(r + to_read, std::memory_order_release);
        return to_read;
    }

    bool try_pop(T& out) noexcept {
        return pop_span(std::span<T>{&out, 1}) == 1;
    }

    [[nodiscard]] size_t size_approx() const noexcept {
        const size_t w = write_idx_.load(std::memory_order_acquire);
        const size_t r = read_idx_.load(std::memory_order_acquire);
        return w - r;
    }

    [[nodiscard]] size_t free_space_approx() const noexcept {
        const size_t used = size_approx();
        return (used < kCapacity) ? (kCapacity - used) : 0;
    }

    void reset_consumer() noexcept {
        const size_t w = write_idx_.load(std::memory_order_acquire);
        read_idx_.store(w, std::memory_order_release);
    }

private:
    alignas(kHardwareCacheLine) std::atomic<size_t> write_idx_{0};
    alignas(kHardwareCacheLine) std::atomic<size_t> read_idx_{0};
    alignas(kHardwareCacheLine) std::array<T, CapacityPow2> storage_{};
};

// ============================================================================
// AudioTapCursor & LockFreeBroadcastRing
// Single-writer, multi-consumer lock-free monotonic sequence ring.
// Allows SSTV, APRS monitor, and spectrum widgets to tap the active stream
// simultaneously with ZERO stream buffer duplication and ZERO locks.
// ============================================================================
struct AudioTapCursor {
    uint64_t next_seq    = 0;
    uint32_t route_epoch = 0;
    bool     initialized = false;
};

template <size_t CapacityPow2>
class LockFreeBroadcastRing {
    static_assert((CapacityPow2 >= 1024) && ((CapacityPow2 & (CapacityPow2 - 1)) == 0),
                  "Broadcast ring capacity must be a power of two >= 1024");
public:
    static constexpr size_t kCapacity = CapacityPow2;
    static constexpr size_t kMask     = CapacityPow2 - 1;

    constexpr LockFreeBroadcastRing() noexcept = default;

    // Single-writer (router pump or active RT producer)
    void publish(std::span<const float> samples) noexcept {
        if (samples.empty()) return;
        const uint64_t seq = published_seq_.load(std::memory_order_relaxed);
        for (size_t i = 0; i < samples.size(); ++i) {
            buffer_[(seq + i) & kMask] = samples[i];
        }
        published_seq_.store(seq + samples.size(), std::memory_order_release);
    }

    // Lock-free multi-consumer tap: copies only into caller's pre-allocated span.
    size_t read_tap(AudioTapCursor& cursor, std::span<float> out, uint32_t current_epoch) const noexcept {
        const uint64_t pub = published_seq_.load(std::memory_order_acquire);
        if (!cursor.initialized || cursor.route_epoch != current_epoch) {
            cursor.next_seq    = pub;
            cursor.route_epoch = current_epoch;
            cursor.initialized = true;
            return 0;
        }
        if (pub <= cursor.next_seq) {
            return 0;
        }
        // If consumer lagged by more than 3/4 of the ring, advance safely to avoid overwrite hazard
        const uint64_t safety_window = (kCapacity * 3) / 4;
        if (pub - cursor.next_seq > safety_window) {
            cursor.next_seq = pub - (kCapacity / 2);
        }
        const size_t avail   = static_cast<size_t>(pub - cursor.next_seq);
        const size_t to_copy = std::min(avail, out.size());
        for (size_t i = 0; i < to_copy; ++i) {
            out[i] = buffer_[(cursor.next_seq + i) & kMask];
        }
        cursor.next_seq += to_copy;
        return to_copy;
    }

    [[nodiscard]] uint64_t published_samples() const noexcept {
        return published_seq_.load(std::memory_order_acquire);
    }

private:
    alignas(kHardwareCacheLine) std::atomic<uint64_t> published_seq_{0};
    alignas(kHardwareCacheLine) std::array<float, CapacityPow2> buffer_{};
};

// POD descriptor for enumerated secondary USB audio capture/playback devices
struct RouterDeviceEntry {
    uint32_t                  os_device_id    = 0;
    uint32_t                  os_out_dev_id   = 0;
    std::array<char, 96>      name{};
    std::array<char, 64>      uid{};
    int                       nominal_rate_hz = 48000;
    bool                      is_usb_dongle   = false;
    bool                      has_output      = false;

    [[nodiscard]] std::string_view name_view() const noexcept {
        return std::string_view{name.data()};
    }
};

// Telemetry POD snapshot (lock-free atomic reads for UI meters)
struct AudioRouterTelemetry {
    AudioSource active_source        = AudioSource::PrimaryG90;
    uint32_t    route_epoch          = 1;
    float       source_a_rms_dbfs    = -120.0f;
    float       source_b_rms_dbfs    = -120.0f;
    float       active_rms_dbfs      = -120.0f;
    bool        dongle_stream_active = false;
    bool        tx_streaming_active  = false;
    int         dongle_device_index  = -1;
    uint64_t    routed_samples       = 0;
    size_t      tx_queued_samples    = 0;
};

// ============================================================================
// AudioRouter — Lock-Free Dual-Source RX/TX Router & Device Hot-Swapper
// ============================================================================
class AudioRouter {
public:
    static constexpr size_t kSourceRingSamples    = 65536;  // ~5.46s @ 12 kHz per RX source
    static constexpr size_t kBroadcastRingSamples = 65536;  // ~5.46s shared zero-copy RX tap
    static constexpr size_t kTxRingSamples48k     = 262144; // ~5.46s @ 48 kHz 16-bit TX output ring

    AudioRouter();
    ~AudioRouter();

    AudioRouter(const AudioRouter&) = delete;
    AudioRouter& operator=(const AudioRouter&) = delete;

    // Enumerate system capture & playback devices to locate Secondary USB Audio Dongle (BTECH / UV-5R)
    void refresh_usb_dongle_devices(int primary_g90_device_idx = 0);

    [[nodiscard]] std::span<const RouterDeviceEntry> devices() const noexcept {
        return std::span<const RouterDeviceEntry>{devices_.data(), device_count_};
    }

    // Atomic, non-blocking source switch (0.00 ms GUI stall; bumps route_epoch for clean decoder resync)
    void select_source(AudioSource src) noexcept;
    void toggle_source() noexcept;
    [[nodiscard]] AudioSource active_source() const noexcept {
        return active_source_.load(std::memory_order_acquire);
    }

    // Bind secondary USB Audio Dongle hardware RX + TX streams (Source B) via RAII
    bool bind_usb_dongle_device(int device_idx);
    void stop_usb_dongle_stream() noexcept;
    [[nodiscard]] int selected_dongle_device_index() const noexcept {
        return selected_dongle_idx_.load(std::memory_order_acquire);
    }
    [[nodiscard]] std::string_view selected_dongle_name() const noexcept;

    // Real-time lock-free RX producer entry points (safe to call from OS audio IRQ / callback threads):
    //   - push_primary_g90_samples: fed from existing Xiegu G90 SoundcardEngine capture
    //   - push_usb_dongle_samples : fed from dedicated USB Dongle OS callback (or loopback test)
    void push_primary_g90_samples(std::span<const float> mono_12k) noexcept;
    void push_usb_dongle_samples(std::span<const float> mono_12k) noexcept;

    // Drains both SPSC RX source rings and publishes the currently selected source into the
    // shared zero-copy broadcast ring. Safe to call from DSP worker thread; zero allocations.
    size_t pump_routing() noexcept;

    // Lock-free consumer RX tap: SSTV engine, APRS monitor, or spectrum widget reads active 12 kHz PCM
    // directly into its pre-allocated span without duplicating stream buffers or taking locks.
    size_t read_active_stream(AudioTapCursor& cursor, std::span<float> dst) noexcept;

    // ========================================================================
    // Zero-Copy Lock-Free SPSC TX Output Streaming (48 kHz 16-bit PCM)
    // Used by SstvModulator & APRS/test tone generators for click-free TX.
    // ========================================================================
    size_t queue_tx_pcm48(std::span<const int16_t> samples_48k,
                          bool apply_fade_in  = false,
                          bool apply_fade_out = false) noexcept;

    // Consumer entry points for OS playback / Primary G90 SoundcardEngine bridge:
    //   - pop_tx_pcm48          : pops 48 kHz 16-bit PCM directly (for USB Dongle VOX / unit tests)
    //   - pop_tx_for_primary_12k: pops & anti-alias decimates 4:1 into 12 kHz float for Xiegu G90 TX
    size_t pop_tx_pcm48(std::span<int16_t> dst_48k) noexcept;
    size_t pop_tx_for_primary_12k(std::span<float> dst_12k) noexcept;

    // Smoothly ramps down active TX audio over 1.5 ms (72 samples @ 48 kHz) and flushes the TX ring
    void abort_tx() noexcept;

    [[nodiscard]] size_t tx_buffered_samples() const noexcept { return tx_pcm48_ring_.size_approx(); }
    [[nodiscard]] size_t tx_free_space() const noexcept       { return tx_pcm48_ring_.free_space_approx(); }
    [[nodiscard]] bool   is_tx_active() const noexcept {
        return tx_pcm48_ring_.size_approx() > 0 && !tx_abort_requested_.load(std::memory_order_acquire);
    }

    // Apply 1.5 ms raised-cosine fade-in / fade-out envelope in-place on a 48 kHz buffer
    static void apply_cosine_taper_48k(std::span<int16_t> buf, bool fade_in, bool fade_out) noexcept;

    [[nodiscard]] AudioRouterTelemetry telemetry() const noexcept;

private:
    struct PlatformDongleStream;
    struct PlatformDongleTxStream;

    static float compute_rms_dbfs(std::span<const float> pcm) noexcept;

    static constexpr size_t kMaxDevices = 16;
    std::array<RouterDeviceEntry, kMaxDevices> devices_{};
    size_t                                     device_count_ = 0;

    alignas(kHardwareCacheLine) std::atomic<AudioSource> active_source_{AudioSource::PrimaryG90};
    std::atomic<uint32_t> route_epoch_{1};
    std::atomic<int>      selected_dongle_idx_{-1};
    std::atomic<bool>     dongle_stream_open_{false};
    std::atomic<bool>     tx_abort_requested_{false};
    std::atomic<float>    rms_a_dbfs_{-120.0f};
    std::atomic<float>    rms_b_dbfs_{-120.0f};

    LockFreeSpscRing<float, kSourceRingSamples>      ring_source_a_{};
    LockFreeSpscRing<float, kSourceRingSamples>      ring_source_b_{};
    LockFreeBroadcastRing<kBroadcastRingSamples>     broadcast_ring_{};

    // Lock-free 48 kHz 16-bit PCM TX output SPSC ring (~5.46s capacity)
    LockFreeSpscRing<int16_t, kTxRingSamples48k>     tx_pcm48_ring_{};

    // Pre-allocated scratchpads — strictly zero heap allocations on real-time paths
    alignas(kHardwareCacheLine) std::array<float, 2048>   pump_scratch_a_{};
    alignas(kHardwareCacheLine) std::array<float, 2048>   pump_scratch_b_{};
    alignas(kHardwareCacheLine) std::array<int16_t, 4096> tx_taper_scratch_{};
    alignas(kHardwareCacheLine) std::array<int16_t, 4096> tx_decim_scratch_{};

    std::unique_ptr<PlatformDongleStream>   dongle_hw_{nullptr};
    std::unique_ptr<PlatformDongleTxStream> dongle_tx_hw_{nullptr};
};

} // namespace hamshack::carbon
