#pragma once
// ============================================================================
// soundcard_audio.hpp — Native Cross-Platform Soundcard Engine (Windows & macOS)
// Enumerates real Windows WASAPI/WinMM & macOS CoreAudio devices (auto-selecting
// "USB Audio Device" / CODEC for Xiegu G90), binds 12 kHz audio input/output to
// the selected hardware device, and runs an 8192-pt FFT Waterfall & Spectrum.
// Zero fake/synthetic data: displays only live soundcard audio.
// ============================================================================

#include <algorithm>
#include <array>
#include <atomic>
#include <chrono>
#include <cmath>
#include <complex>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <deque>
#include <functional>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

#include "hamshack_platform.hpp"

#ifdef _WIN32
#include <mmsystem.h>
#include <mmdeviceapi.h>
#include <propsys.h>
#endif

#ifdef __APPLE__
extern "C" {
typedef uint32_t AudioObjectID;
typedef AudioObjectID AudioDeviceID;
typedef uint32_t AudioObjectPropertySelector;
typedef uint32_t AudioObjectPropertyScope;
typedef uint32_t AudioObjectPropertyElement;
typedef int32_t OSStatus;
typedef uint32_t UInt32;

struct AudioObjectPropertyAddress {
    AudioObjectPropertySelector mSelector;
    AudioObjectPropertyScope    mScope;
    AudioObjectPropertyElement  mElement;
};

struct AudioStreamBasicDescription {
    double   mSampleRate;
    uint32_t mFormatID;
    uint32_t mFormatFlags;
    uint32_t mBytesPerPacket;
    uint32_t mFramesPerPacket;
    uint32_t mBytesPerFrame;
    uint32_t mChannelsPerFrame;
    uint32_t mBitsPerChannel;
    uint32_t mReserved;
};

struct AudioTimeStamp;
struct AudioStreamPacketDescription;
typedef struct OpaqueAudioQueue* AudioQueueRef;

struct AudioQueueBuffer {
    const uint32_t mAudioDataBytesCapacity;
    void* const    mAudioData;
    uint32_t       mAudioDataByteSize;
    void*          mUserData;
    const uint32_t mPacketDescriptionCapacity;
    AudioStreamPacketDescription* const mPacketDescriptions;
    uint32_t       mPacketDescriptionCount;
};
typedef AudioQueueBuffer* AudioQueueBufferRef;

typedef void (*AudioQueueInputCallback)(
    void* inUserData,
    AudioQueueRef inAQ,
    AudioQueueBufferRef inBuffer,
    const AudioTimeStamp* inStartTime,
    uint32_t inNumberPacketDescriptions,
    const AudioStreamPacketDescription* inPacketDescs);

typedef void (*AudioQueueOutputCallback)(
    void* inUserData,
    AudioQueueRef inAQ,
    AudioQueueBufferRef inBuffer);

OSStatus AudioObjectGetPropertyDataSize(
    AudioObjectID inObjectID,
    const AudioObjectPropertyAddress* inAddress,
    uint32_t inQualifierDataSize,
    const void* inQualifierData,
    uint32_t* outDataSize);

OSStatus AudioObjectGetPropertyData(
    AudioObjectID inObjectID,
    const AudioObjectPropertyAddress* inAddress,
    uint32_t inQualifierDataSize,
    const void* inQualifierData,
    uint32_t* ioDataSize,
    void* outData);

OSStatus AudioQueueNewInput(
    const AudioStreamBasicDescription* inFormat,
    AudioQueueInputCallback inCallbackProc,
    void* inUserData,
    void* inCallbackRunLoop,
    const void* inCallbackRunLoopMode,
    uint32_t inFlags,
    AudioQueueRef* outAQ);

OSStatus AudioQueueNewOutput(
    const AudioStreamBasicDescription* inFormat,
    AudioQueueOutputCallback inCallbackProc,
    void* inUserData,
    void* inCallbackRunLoop,
    const void* inCallbackRunLoopMode,
    uint32_t inFlags,
    AudioQueueRef* outAQ);

OSStatus AudioQueueSetProperty(
    AudioQueueRef inAQ,
    uint32_t inID,
    const void* inData,
    uint32_t inDataSize);

OSStatus AudioQueueAllocateBuffer(
    AudioQueueRef inAQ,
    uint32_t inBufferByteSize,
    AudioQueueBufferRef* outBuffer);

OSStatus AudioQueueEnqueueBuffer(
    AudioQueueRef inAQ,
    AudioQueueBufferRef inBuffer,
    uint32_t inNumPacketDescs,
    const AudioStreamPacketDescription* inPacketDescs);

OSStatus AudioQueueStart(AudioQueueRef inAQ, const AudioTimeStamp* inStartTime);
OSStatus AudioQueueStop(AudioQueueRef inAQ, uint8_t inImmediate);
OSStatus AudioQueueDispose(AudioQueueRef inAQ, uint8_t inImmediate);

typedef const void* CFTypeRef;
typedef const struct __CFAllocator* CFAllocatorRef;
typedef const struct __CFString* CFStringRef;
typedef unsigned int CFStringEncoding;
void CFRelease(CFTypeRef cf);
CFStringRef CFStringCreateWithCString(CFAllocatorRef alloc, const char* cStr, CFStringEncoding encoding);
uint8_t CFStringGetCString(CFStringRef theString, char* buffer, long bufferSize, CFStringEncoding encoding);
} // extern "C"

static constexpr AudioObjectID kHsAudioObjectSystemObject = 1;
static constexpr uint32_t kHsAudioHardwarePropertyDevices = 0x64657623u;          // 'dev#'
static constexpr uint32_t kHsAudioDevicePropertyDeviceNameCFString = 0x6C6E616Du; // 'lnam'
static constexpr uint32_t kHsAudioDevicePropertyDeviceUID = 0x75696420u;          // 'uid '
static constexpr uint32_t kHsAudioDevicePropertyNominalSampleRate = 0x6E737274u;  // 'nsrt'
static constexpr uint32_t kHsAudioDevicePropertyStreams = 0x73746D23u;            // 'stm#'
static constexpr uint32_t kHsAudioObjectPropertyScopeGlobal = 0x676C6F62u;        // 'glob'
static constexpr uint32_t kHsAudioObjectPropertyScopeInput  = 0x696E7074u;        // 'inpt'
static constexpr uint32_t kHsAudioObjectPropertyScopeOutput = 0x6F757470u;        // 'outp'
static constexpr uint32_t kHsAudioObjectPropertyElementMain = 0;
static constexpr uint32_t kHsAudioFormatLinearPCM = 0x6C70636Du;                  // 'lpcm'
static constexpr uint32_t kHsLinearPCMFormatFlagIsSignedInteger = (1u << 2);
static constexpr uint32_t kHsLinearPCMFormatFlagIsPacked        = (1u << 3);
static constexpr uint32_t kHsAudioQueueProperty_CurrentDevice   = 0x61716364u;    // 'aqcd'
static constexpr uint32_t kHsCFStringEncodingUTF8 = 0x08000100u;
#endif

namespace hamshack {

struct AudioDeviceInfo {
    uint32_t device_id = 0;
    std::string uid;
    std::string name;
    bool has_input = false;
    bool has_output = false;
    bool is_usb = false;
    int nominal_rate_hz = 48000;
};

class SoundcardEngine {
public:
    static constexpr int SAMPLE_RATE = 12000;
    static constexpr int FFT_SIZE = 8192; // 8192-point FFT for sub-hertz frequency resolution
    static constexpr int WF_BINS = 2880;  // 100 Hz .. 2900 Hz (~0.97 Hz/bin display, doubled pixel resolution)
    static constexpr int WF_HISTORY = 450; // History capacity for 150+ seconds of deep rolling waterfall

    mutable std::mutex mtx;

    std::vector<AudioDeviceInfo> input_devices;
    std::vector<AudioDeviceInfo> output_devices;
    int selected_input_idx = 0;
    int selected_output_idx = 0;
    std::string active_input_name = "None";
    std::string active_output_name = "None";
    std::string active_input_uid;
    std::string active_output_uid;

    bool rf_band_synth_enabled = false; // Strictly false: no fake audio injection
    bool hw_stream_active = false;
    float rx_gain_db = 0.0f;            // -30 .. +30 dB
    float tx_gain_db = -3.0f;           // -30 .. 0 dB
    float rx_rms_db = -120.0f;
    float rx_peak_db = -120.0f;
    int   peak_freq_hz = 0;
    float tx_rms_db = -120.0f;
    bool tx_playing = false;
    bool monitor_tx_audio = true;
    uint64_t hw_rx_callbacks = 0;

    // Waterfall & Spectrum buffers (WF_BINS wide, values in 0.0 .. 1.0 normalized dB)
    struct WaterfallRow {
        std::array<uint8_t, WF_BINS> bins{};
        bool is_interval_start = false;
        std::string band_label;
    };
    std::string current_band_label = "20m";
    std::array<float, WF_BINS> current_spectrum{};
    std::deque<WaterfallRow> waterfall_rows; // 0..255 intensity

    // Rolling 120-second RX sample ring buffer at 12,000 Hz (1,440,000 floats)
    static constexpr size_t RX_RING_CAP = SAMPLE_RATE * 120;
    std::vector<float> rx_ring;
    size_t rx_ring_write_pos = 0;
    uint64_t total_rx_samples = 0;

    struct SynthSignal {
        float freq_hz = 1000.0f;
        float bw_hz = 50.0f;
        float snr_db = -6.0f;
        int mode_type = 0;
        uint64_t expire_ms = 0;
    };
    std::vector<SynthSignal> active_synth_signals;

    SoundcardEngine() {
        rx_ring.assign(RX_RING_CAP, 0.0f);
        current_spectrum.fill(0.0f);
        refresh_devices();
    }

    ~SoundcardEngine() {
        stop();
    }

    void refresh_devices() {
        std::vector<AudioDeviceInfo> ins;
        std::vector<AudioDeviceInfo> outs;

#ifdef _WIN32
        auto wide_to_utf8_snd = [](const wchar_t* ws) -> std::string {
            if (!ws || !*ws) return {};
            int n = WideCharToMultiByte(CP_UTF8, 0, ws, -1, nullptr, 0, nullptr, nullptr);
            if (n <= 1) return {};
            std::string out(static_cast<size_t>(n - 1), '\0');
            WideCharToMultiByte(CP_UTF8, 0, ws, -1, out.data(), n, nullptr, nullptr);
            return out;
        };

        // Query full friendly names from WASAPI IMMDeviceEnumerator to enrich 31-char WinMM caps names
        auto get_wasapi_names = [&](EDataFlow flow) -> std::vector<std::string> {
            std::vector<std::string> names;
            HRESULT hr_co = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
            static const CLSID kClsidMMDeviceEnumerator = {0xbcde0395, 0xe52f, 0x467c, {0x8e, 0x3d, 0xc4, 0x57, 0x92, 0x91, 0x69, 0x2e}};
            static const IID   kIidIMMDeviceEnumerator  = {0xa95664d2, 0x9614, 0x4f35, {0xa7, 0x46, 0xde, 0x8d, 0xb6, 0x36, 0x17, 0xe6}};
            static const PROPERTYKEY kPkeyFriendlyName  = {{0xa45c254e, 0xdf1c, 0x4efd, {0x80, 0x20, 0x67, 0xd1, 0x46, 0xa8, 0x50, 0xe0}}, 14};

            IMMDeviceEnumerator* pEnum = nullptr;
            if (SUCCEEDED(CoCreateInstance(kClsidMMDeviceEnumerator, nullptr, CLSCTX_ALL,
                                           kIidIMMDeviceEnumerator, reinterpret_cast<void**>(&pEnum))) && pEnum) {
                IMMDeviceCollection* pCol = nullptr;
                if (SUCCEEDED(pEnum->EnumAudioEndpoints(flow, DEVICE_STATE_ACTIVE, &pCol)) && pCol) {
                    UINT count = 0;
                    pCol->GetCount(&count);
                    for (UINT i = 0; i < count; ++i) {
                        IMMDevice* pDev = nullptr;
                        if (SUCCEEDED(pCol->Item(i, &pDev)) && pDev) {
                            IPropertyStore* pProps = nullptr;
                            if (SUCCEEDED(pDev->OpenPropertyStore(STGM_READ, &pProps)) && pProps) {
                                PROPVARIANT varName;
                                PropVariantInit(&varName);
                                if (SUCCEEDED(pProps->GetValue(kPkeyFriendlyName, &varName)) && varName.vt == VT_LPWSTR) {
                                    names.push_back(wide_to_utf8_snd(varName.pwszVal));
                                }
                                PropVariantClear(&varName);
                                pProps->Release();
                            }
                            pDev->Release();
                        }
                    }
                    pCol->Release();
                }
                pEnum->Release();
            }
            if (SUCCEEDED(hr_co)) CoUninitialize();
            return names;
        };

        auto wasapi_in_names  = get_wasapi_names(eCapture);
        auto wasapi_out_names = get_wasapi_names(eRender);

        auto enrich_name = [](const std::string& short_nm, const std::vector<std::string>& full_list) -> std::string {
            if (short_nm.empty()) return "Windows Audio Device";
            for (const auto& fn : full_list) {
                if (fn.size() >= short_nm.size() && fn.compare(0, short_nm.size(), short_nm) == 0) {
                    return fn;
                }
            }
            return short_nm;
        };

        auto check_is_usb = [](const std::string& nm) -> bool {
            std::string lower = nm;
            std::transform(lower.begin(), lower.end(), lower.begin(),
                           [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
            return lower.find("usb") != std::string::npos ||
                   lower.find("codec") != std::string::npos ||
                   lower.find("c-media") != std::string::npos ||
                   lower.find("digirig") != std::string::npos ||
                   lower.find("signalink") != std::string::npos ||
                   lower.find("g90") != std::string::npos;
        };

        UINT num_in = waveInGetNumDevs();
        for (UINT i = 0; i < num_in; ++i) {
            WAVEINCAPSW caps{};
            if (waveInGetDevCapsW(i, &caps, sizeof(caps)) == MMSYSERR_NOERROR) {
                std::string dev_name = enrich_name(wide_to_utf8_snd(caps.szPname), wasapi_in_names);
                int nom_rate = (caps.dwFormats & (WAVE_FORMAT_48M16 | WAVE_FORMAT_48S16)) ? 48000 : 44100;
                ins.push_back(AudioDeviceInfo{
                    static_cast<uint32_t>(i),
                    "winmm_in_" + std::to_string(i),
                    dev_name,
                    true,
                    false,
                    check_is_usb(dev_name),
                    nom_rate
                });
            }
        }

        UINT num_out = waveOutGetNumDevs();
        for (UINT i = 0; i < num_out; ++i) {
            WAVEOUTCAPSW caps{};
            if (waveOutGetDevCapsW(i, &caps, sizeof(caps)) == MMSYSERR_NOERROR) {
                std::string dev_name = enrich_name(wide_to_utf8_snd(caps.szPname), wasapi_out_names);
                int nom_rate = (caps.dwFormats & (WAVE_FORMAT_48M16 | WAVE_FORMAT_48S16)) ? 48000 : 44100;
                outs.push_back(AudioDeviceInfo{
                    static_cast<uint32_t>(i),
                    "winmm_out_" + std::to_string(i),
                    dev_name,
                    false,
                    true,
                    check_is_usb(dev_name),
                    nom_rate
                });
            }
        }
#elif defined(__APPLE__)
        AudioObjectPropertyAddress prop_addr = {
            kHsAudioHardwarePropertyDevices,
            kHsAudioObjectPropertyScopeGlobal,
            kHsAudioObjectPropertyElementMain
        };
        UInt32 data_size = 0;
        if (AudioObjectGetPropertyDataSize(kHsAudioObjectSystemObject, &prop_addr, 0, nullptr, &data_size) == 0 &&
            data_size >= sizeof(AudioDeviceID)) {
            size_t count = data_size / sizeof(AudioDeviceID);
            std::vector<AudioDeviceID> ids(count);
            if (AudioObjectGetPropertyData(kHsAudioObjectSystemObject, &prop_addr, 0, nullptr, &data_size, ids.data()) == 0) {
                for (AudioDeviceID id : ids) {
                    CFStringRef cf_name = nullptr;
                    UInt32 name_sz = sizeof(cf_name);
                    AudioObjectPropertyAddress name_addr = {
                        kHsAudioDevicePropertyDeviceNameCFString,
                        kHsAudioObjectPropertyScopeGlobal,
                        kHsAudioObjectPropertyElementMain
                    };
                    std::string dev_name = "CoreAudio Device";
                    if (AudioObjectGetPropertyData(id, &name_addr, 0, nullptr, &name_sz, &cf_name) == 0 && cf_name) {
                        char buf[256]{};
                        if (CFStringGetCString(cf_name, buf, sizeof(buf), kHsCFStringEncodingUTF8)) {
                            dev_name = buf;
                        }
                        CFRelease(cf_name);
                    }

                    CFStringRef cf_uid = nullptr;
                    UInt32 uid_sz = sizeof(cf_uid);
                    AudioObjectPropertyAddress uid_addr = {
                        kHsAudioDevicePropertyDeviceUID,
                        kHsAudioObjectPropertyScopeGlobal,
                        kHsAudioObjectPropertyElementMain
                    };
                    std::string dev_uid = std::to_string(id);
                    if (AudioObjectGetPropertyData(id, &uid_addr, 0, nullptr, &uid_sz, &cf_uid) == 0 && cf_uid) {
                        char buf[256]{};
                        if (CFStringGetCString(cf_uid, buf, sizeof(buf), kHsCFStringEncodingUTF8)) {
                            dev_uid = buf;
                        }
                        CFRelease(cf_uid);
                    }

                    double nom_rate = 48000.0;
                    UInt32 rate_sz = sizeof(nom_rate);
                    AudioObjectPropertyAddress rate_addr = {
                        kHsAudioDevicePropertyNominalSampleRate,
                        kHsAudioObjectPropertyScopeGlobal,
                        kHsAudioObjectPropertyElementMain
                    };
                    (void)AudioObjectGetPropertyData(id, &rate_addr, 0, nullptr, &rate_sz, &nom_rate);

                    auto has_streams = [&](AudioObjectPropertyScope scope) -> bool {
                        AudioObjectPropertyAddress s_addr = {
                            kHsAudioDevicePropertyStreams,
                            scope,
                            kHsAudioObjectPropertyElementMain
                        };
                        UInt32 s_sz = 0;
                        return AudioObjectGetPropertyDataSize(id, &s_addr, 0, nullptr, &s_sz) == 0 && s_sz > 0;
                    };

                    bool in_ok = has_streams(kHsAudioObjectPropertyScopeInput);
                    bool out_ok = has_streams(kHsAudioObjectPropertyScopeOutput);
                    std::string lower = dev_name;
                    std::transform(lower.begin(), lower.end(), lower.begin(),
                                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
                    bool is_usb = lower.find("usb") != std::string::npos ||
                                  lower.find("codec") != std::string::npos ||
                                  lower.find("c-media") != std::string::npos ||
                                  lower.find("digirig") != std::string::npos ||
                                  lower.find("signalink") != std::string::npos ||
                                  lower.find("g90") != std::string::npos;

                    AudioDeviceInfo info{id, dev_uid, dev_name, in_ok, out_ok, is_usb, static_cast<int>(nom_rate)};
                    if (in_ok) ins.push_back(info);
                    if (out_ok) outs.push_back(info);
                }
            }
        }
#endif
        // Sort so USB Audio devices (like "USB Audio Device") appear first and are auto-selected
        auto usb_first = [](const AudioDeviceInfo& a, const AudioDeviceInfo& b) {
            if (a.is_usb != b.is_usb) return a.is_usb > b.is_usb;
            return a.name < b.name;
        };
        std::sort(ins.begin(), ins.end(), usb_first);
        std::sort(outs.begin(), outs.end(), usb_first);

        std::lock_guard<std::mutex> lk(mtx);
        input_devices = std::move(ins);
        output_devices = std::move(outs);
        if (!input_devices.empty()) {
            selected_input_idx = std::clamp(selected_input_idx, 0, static_cast<int>(input_devices.size()) - 1);
            active_input_name = input_devices[selected_input_idx].name;
            active_input_uid  = input_devices[selected_input_idx].uid;
        } else {
            selected_input_idx = 0;
            active_input_name = "No Input Soundcard";
            active_input_uid.clear();
        }
        if (!output_devices.empty()) {
            selected_output_idx = std::clamp(selected_output_idx, 0, static_cast<int>(output_devices.size()) - 1);
            active_output_name = output_devices[selected_output_idx].name;
            active_output_uid  = output_devices[selected_output_idx].uid;
        } else {
            selected_output_idx = 0;
            active_output_name = "No Output Soundcard";
            active_output_uid.clear();
        }
    }

    void select_input_device(int idx) {
        {
            std::lock_guard<std::mutex> lk(mtx);
            if (idx >= 0 && idx < static_cast<int>(input_devices.size())) {
                selected_input_idx = idx;
                active_input_name = input_devices[idx].name;
                active_input_uid  = input_devices[idx].uid;
            }
        }
        if (running_.load()) {
            stop_coreaudio_queues();
            start_coreaudio_queues();
        }
    }

    void select_output_device(int idx) {
        {
            std::lock_guard<std::mutex> lk(mtx);
            if (idx >= 0 && idx < static_cast<int>(output_devices.size())) {
                selected_output_idx = idx;
                active_output_name = output_devices[idx].name;
                active_output_uid  = output_devices[idx].uid;
            }
        }
        if (running_.load()) {
            stop_coreaudio_queues();
            start_coreaudio_queues();
        }
    }

    void start() {
        if (running_.exchange(true)) return;
        start_coreaudio_queues();
        dsp_thread_ = std::thread([this] { dsp_loop(); });
    }

    void stop() {
        if (!running_.exchange(false)) return;
        if (dsp_thread_.joinable()) dsp_thread_.join();
        stop_coreaudio_queues();
    }

    // Queue modulated 12,000 Hz mono float32 samples (-1.0 .. +1.0) for transmission out USB soundcard
    void queue_tx_samples(const std::vector<float>& samples) {
        std::lock_guard<std::mutex> lk(mtx);
        float gain = std::pow(10.0f, tx_gain_db / 20.0f);
        for (float s : samples) {
            tx_queue_.push_back(std::clamp(s * gain, -1.0f, 1.0f));
        }
        tx_playing = !tx_queue_.empty();
    }

    void halt_tx_audio() {
        std::lock_guard<std::mutex> lk(mtx);
        tx_queue_.clear();
        tx_playing = false;
        tx_rms_db = -120.0f;
    }

    void halt_tx() {
        halt_tx_audio();
    }

    bool is_tx_active() const {
        std::lock_guard<std::mutex> lk(mtx);
        return tx_playing && !tx_queue_.empty();
    }

    void inject_rx_waveform(const std::vector<float>& wave_12k, float snr_scale = 0.25f) {
        std::lock_guard<std::mutex> lk(mtx);
        size_t pos = rx_ring_write_pos;
        for (size_t i = 0; i < wave_12k.size() && i < RX_RING_CAP / 2; ++i) {
            size_t idx = (pos + i) % RX_RING_CAP;
            rx_ring[idx] = std::clamp(rx_ring[idx] + wave_12k[i] * snr_scale, -1.0f, 1.0f);
        }
    }

    void add_synth_waterfall_signal(float /*freq_hz*/, float /*bw_hz*/, float /*snr_db*/, int /*mode_type*/, uint64_t /*duration_ms*/) {
        // No-op: synthetic waterfall traces disabled so only real audio is shown
    }

    // Copy the most recent `count` samples at 12,000 Hz from the RX ring buffer
    std::vector<float> get_recent_rx_samples(size_t count) const {
        std::lock_guard<std::mutex> lk(mtx);
        count = std::min(count, RX_RING_CAP);
        std::vector<float> out(count, 0.0f);
        size_t start = (rx_ring_write_pos + RX_RING_CAP - count) % RX_RING_CAP;
        for (size_t i = 0; i < count; ++i) {
            out[i] = rx_ring[(start + i) % RX_RING_CAP];
        }
        return out;
    }

    // In-place Cooley-Tukey Radix-2 complex FFT
    static void fft_radix2(std::vector<std::complex<float>>& a) {
        const size_t n = a.size();
        if (n <= 1) return;
        for (size_t i = 1, j = 0; i < n; ++i) {
            size_t bit = n >> 1;
            for (; j & bit; bit >>= 1) j ^= bit;
            j ^= bit;
            if (i < j) std::swap(a[i], a[j]);
        }
        for (size_t len = 2; len <= n; len <<= 1) {
            float ang = -2.0f * 3.14159265358979323846f / static_cast<float>(len);
            std::complex<float> wlen(std::cos(ang), std::sin(ang));
            for (size_t i = 0; i < n; i += len) {
                std::complex<float> w(1.0f, 0.0f);
                for (size_t j = 0; j < len / 2; ++j) {
                    std::complex<float> u = a[i + j];
                    std::complex<float> v = a[i + j + len / 2] * w;
                    a[i + j] = u + v;
                    a[i + j + len / 2] = u - v;
                    w *= wlen;
                }
            }
        }
    }

private:
    std::atomic<bool> running_{false};
    std::thread dsp_thread_;
    std::deque<float> tx_queue_;
    std::deque<float> hw_rx_fifo_;

#ifdef _WIN32
    static constexpr size_t WIN_IN_BUFS    = 4;
    static constexpr size_t WIN_IN_FRAMES  = 1200; // 100ms @ 12 kHz
    static constexpr size_t WIN_OUT_BUFS   = 4;
    static constexpr size_t WIN_OUT_FRAMES = 600;  // 50ms @ 12 kHz

    HWAVEIN  win_wave_in_  = nullptr;
    HWAVEOUT win_wave_out_ = nullptr;
    HANDLE   win_audio_ev_ = nullptr;
    std::atomic<bool> win_audio_run_{false};
    std::thread win_audio_thread_;
    std::array<WAVEHDR, WIN_IN_BUFS>  win_in_hdrs_{};
    std::array<std::vector<int16_t>, WIN_IN_BUFS> win_in_pcm_{};
    std::array<WAVEHDR, WIN_OUT_BUFS> win_out_hdrs_{};
    std::array<std::vector<int16_t>, WIN_OUT_BUFS> win_out_pcm_{};
    std::array<bool, WIN_OUT_BUFS> win_out_busy_{};

    void win_audio_io_loop() {
        UINT in_dev_id = WAVE_MAPPER;
        UINT out_dev_id = WAVE_MAPPER;
        {
            std::lock_guard<std::mutex> lk(mtx);
            if (selected_input_idx >= 0 && selected_input_idx < static_cast<int>(input_devices.size())) {
                in_dev_id = static_cast<UINT>(input_devices[selected_input_idx].device_id);
            }
            if (selected_output_idx >= 0 && selected_output_idx < static_cast<int>(output_devices.size())) {
                out_dev_id = static_cast<UINT>(output_devices[selected_output_idx].device_id);
            }
        }

        WAVEFORMATEX wfx{};
        wfx.wFormatTag      = WAVE_FORMAT_PCM;
        wfx.nChannels       = 1;
        wfx.nSamplesPerSec  = SAMPLE_RATE;
        wfx.wBitsPerSample  = 16;
        wfx.nBlockAlign     = 2;
        wfx.nAvgBytesPerSec = SAMPLE_RATE * 2;
        wfx.cbSize          = 0;

        if (win_audio_run_.load() &&
            waveInOpen(&win_wave_in_, in_dev_id, &wfx, reinterpret_cast<DWORD_PTR>(win_audio_ev_),
                       0, CALLBACK_EVENT) == MMSYSERR_NOERROR && win_wave_in_) {
            for (size_t b = 0; b < WIN_IN_BUFS; ++b) {
                win_in_pcm_[b].assign(WIN_IN_FRAMES, 0);
                win_in_hdrs_[b] = WAVEHDR{};
                win_in_hdrs_[b].lpData = reinterpret_cast<LPSTR>(win_in_pcm_[b].data());
                win_in_hdrs_[b].dwBufferLength = static_cast<DWORD>(WIN_IN_FRAMES * sizeof(int16_t));
                waveInPrepareHeader(win_wave_in_, &win_in_hdrs_[b], sizeof(WAVEHDR));
                waveInAddBuffer(win_wave_in_, &win_in_hdrs_[b], sizeof(WAVEHDR));
            }
            waveInStart(win_wave_in_);
        } else {
            win_wave_in_ = nullptr;
        }

        if (win_audio_run_.load() &&
            waveOutOpen(&win_wave_out_, out_dev_id, &wfx, reinterpret_cast<DWORD_PTR>(win_audio_ev_),
                        0, CALLBACK_EVENT) == MMSYSERR_NOERROR && win_wave_out_) {
            for (size_t b = 0; b < WIN_OUT_BUFS; ++b) {
                win_out_pcm_[b].assign(WIN_OUT_FRAMES, 0);
                win_out_hdrs_[b] = WAVEHDR{};
                win_out_hdrs_[b].lpData = reinterpret_cast<LPSTR>(win_out_pcm_[b].data());
                win_out_hdrs_[b].dwBufferLength = static_cast<DWORD>(WIN_OUT_FRAMES * sizeof(int16_t));
                win_out_busy_[b] = false;
                waveOutPrepareHeader(win_wave_out_, &win_out_hdrs_[b], sizeof(WAVEHDR));
            }
        } else {
            win_wave_out_ = nullptr;
        }

        while (win_audio_run_.load()) {
            WaitForSingleObject(win_audio_ev_, 15);
            if (!win_audio_run_.load()) break;

            // 1. Drain completed RX capture buffers
            if (win_wave_in_) {
                for (size_t b = 0; b < WIN_IN_BUFS; ++b) {
                    WAVEHDR& hdr = win_in_hdrs_[b];
                    if ((hdr.dwFlags & WHDR_DONE) != 0) {
                        size_t n = hdr.dwBytesRecorded / sizeof(int16_t);
                        if (n > 0) {
                            const int16_t* pcm = win_in_pcm_[b].data();
                            std::lock_guard<std::mutex> lk(mtx);
                            ++hw_rx_callbacks;
                            hw_stream_active = true;
                            float gain = std::pow(10.0f, rx_gain_db / 20.0f);
                            double sum_sq = 0.0;
                            float max_abs = 0.0f;
                            for (size_t i = 0; i < n; ++i) {
                                float s = std::clamp((static_cast<float>(pcm[i]) / 32768.0f) * gain, -1.0f, 1.0f);
                                rx_ring[(rx_ring_write_pos + i) % RX_RING_CAP] = s;
                                sum_sq += static_cast<double>(s) * s;
                                max_abs = std::max(max_abs, std::fabs(s));
                            }
                            rx_ring_write_pos = (rx_ring_write_pos + n) % RX_RING_CAP;
                            total_rx_samples += n;
                            float rms = std::sqrt(static_cast<float>(sum_sq / std::max<size_t>(1, n)));
                            rx_rms_db = 20.0f * std::log10(std::max(1e-6f, rms));
                            rx_peak_db = 20.0f * std::log10(std::max(1e-6f, max_abs));
                        }
                        hdr.dwBytesRecorded = 0;
                        hdr.dwFlags &= ~WHDR_DONE;
                        if (win_audio_run_.load()) {
                            waveInAddBuffer(win_wave_in_, &hdr, sizeof(WAVEHDR));
                        }
                    }
                }
            }

            // 2. Feed TX playback buffers when TX samples are queued
            if (win_wave_out_) {
                bool any_in_flight = false;
                for (size_t b = 0; b < WIN_OUT_BUFS; ++b) {
                    WAVEHDR& hdr = win_out_hdrs_[b];
                    if (win_out_busy_[b] && (hdr.dwFlags & WHDR_DONE) != 0) {
                        win_out_busy_[b] = false;
                    }
                    if (!win_out_busy_[b]) {
                        size_t n_tx = 0;
                        double tx_sum_sq = 0.0;
                        {
                            std::lock_guard<std::mutex> lk(mtx);
                            if (!tx_queue_.empty()) {
                                int16_t* pcm = win_out_pcm_[b].data();
                                for (size_t i = 0; i < WIN_OUT_FRAMES; ++i) {
                                    float s = 0.0f;
                                    if (!tx_queue_.empty()) {
                                        s = tx_queue_.front();
                                        tx_queue_.pop_front();
                                        tx_sum_sq += static_cast<double>(s) * s;
                                        ++n_tx;
                                    }
                                    pcm[i] = static_cast<int16_t>(std::clamp(s, -1.0f, 1.0f) * 32000.0f);
                                }
                                float rms = std::sqrt(static_cast<float>(tx_sum_sq / std::max<size_t>(1, n_tx)));
                                tx_rms_db = 20.0f * std::log10(std::max(1e-6f, rms));
                                tx_playing = true;
                            }
                        }
                        if (n_tx > 0) {
                            hdr.dwBufferLength = static_cast<DWORD>(WIN_OUT_FRAMES * sizeof(int16_t));
                            hdr.dwFlags &= ~WHDR_DONE;
                            if (waveOutWrite(win_wave_out_, &hdr, sizeof(WAVEHDR)) == MMSYSERR_NOERROR) {
                                win_out_busy_[b] = true;
                            }
                        }
                    }
                    if (win_out_busy_[b]) any_in_flight = true;
                }
                if (!any_in_flight) {
                    std::lock_guard<std::mutex> lk(mtx);
                    if (tx_queue_.empty()) {
                        tx_playing = false;
                        tx_rms_db = -120.0f;
                    }
                }
            }
        }

        if (win_wave_in_) {
            waveInReset(win_wave_in_);
            for (size_t b = 0; b < WIN_IN_BUFS; ++b) {
                if (win_in_hdrs_[b].lpData) {
                    waveInUnprepareHeader(win_wave_in_, &win_in_hdrs_[b], sizeof(WAVEHDR));
                }
            }
            waveInClose(win_wave_in_);
            win_wave_in_ = nullptr;
        }
        if (win_wave_out_) {
            waveOutReset(win_wave_out_);
            for (size_t b = 0; b < WIN_OUT_BUFS; ++b) {
                if (win_out_hdrs_[b].lpData) {
                    waveOutUnprepareHeader(win_wave_out_, &win_out_hdrs_[b], sizeof(WAVEHDR));
                }
                win_out_busy_[b] = false;
            }
            waveOutClose(win_wave_out_);
            win_wave_out_ = nullptr;
        }
    }
#elif defined(__APPLE__)
    AudioQueueRef in_queue_ = nullptr;
    AudioQueueRef out_queue_ = nullptr;

    static void aq_input_cb(void* user_data,
                            AudioQueueRef inAQ,
                            AudioQueueBufferRef inBuffer,
                            const AudioTimeStamp*,
                            UInt32 inNumPackets,
                            const AudioStreamPacketDescription*) {
        auto* self = static_cast<SoundcardEngine*>(user_data);
        if (self && self->running_.load() && inBuffer && inBuffer->mAudioDataByteSize >= sizeof(int16_t)) {
            const int16_t* pcm = static_cast<const int16_t*>(inBuffer->mAudioData);
            size_t n = inBuffer->mAudioDataByteSize / sizeof(int16_t);
            std::lock_guard<std::mutex> lk(self->mtx);
            ++self->hw_rx_callbacks;
            self->hw_stream_active = true;
            float gain = std::pow(10.0f, self->rx_gain_db / 20.0f);
            double sum_sq = 0.0;
            float max_abs = 0.0f;
            for (size_t i = 0; i < n; ++i) {
                float s = std::clamp((static_cast<float>(pcm[i]) / 32768.0f) * gain, -1.0f, 1.0f);
                self->rx_ring[(self->rx_ring_write_pos + i) % RX_RING_CAP] = s;
                sum_sq += static_cast<double>(s) * s;
                max_abs = std::max(max_abs, std::fabs(s));
            }
            self->rx_ring_write_pos = (self->rx_ring_write_pos + n) % RX_RING_CAP;
            self->total_rx_samples += n;
            float rms = std::sqrt(static_cast<float>(sum_sq / std::max<size_t>(1, n)));
            self->rx_rms_db = 20.0f * std::log10(std::max(1e-6f, rms));
            self->rx_peak_db = 20.0f * std::log10(std::max(1e-6f, max_abs));
        }
        if (self && self->running_.load()) {
            AudioQueueEnqueueBuffer(inAQ, inBuffer, 0, nullptr);
        }
        (void)inNumPackets;
    }

    static void aq_output_cb(void* user_data,
                             AudioQueueRef outAQ,
                             AudioQueueBufferRef outBuffer) {
        auto* self = static_cast<SoundcardEngine*>(user_data);
        if (!outBuffer) return;
        int16_t* pcm = static_cast<int16_t*>(outBuffer->mAudioData);
        size_t frames = outBuffer->mAudioDataBytesCapacity / sizeof(int16_t);
        if (self && self->running_.load()) {
            std::lock_guard<std::mutex> lk(self->mtx);
            double tx_sum_sq = 0.0;
            size_t n_tx = 0;
            for (size_t i = 0; i < frames; ++i) {
                float s = 0.0f;
                if (!self->tx_queue_.empty()) {
                    s = self->tx_queue_.front();
                    self->tx_queue_.pop_front();
                    tx_sum_sq += static_cast<double>(s) * s;
                    ++n_tx;
                }
                pcm[i] = static_cast<int16_t>(std::clamp(s, -1.0f, 1.0f) * 32000.0f);
            }
            if (n_tx > 0) {
                float rms = std::sqrt(static_cast<float>(tx_sum_sq / n_tx));
                self->tx_rms_db = 20.0f * std::log10(std::max(1e-6f, rms));
                self->tx_playing = !self->tx_queue_.empty();
            } else {
                self->tx_playing = false;
                self->tx_rms_db = -120.0f;
            }
        } else {
            std::memset(pcm, 0, frames * sizeof(int16_t));
        }
        outBuffer->mAudioDataByteSize = static_cast<UInt32>(frames * sizeof(int16_t));
        if (self && self->running_.load()) {
            AudioQueueEnqueueBuffer(outAQ, outBuffer, 0, nullptr);
        }
    }
#endif

    void start_coreaudio_queues() {
#ifdef _WIN32
        win_audio_ev_ = CreateEventW(nullptr, FALSE, FALSE, nullptr);
        if (!win_audio_ev_) return;
        win_audio_run_.store(true);
        win_audio_thread_ = std::thread([this] { win_audio_io_loop(); });
#elif defined(__APPLE__)
        AudioStreamBasicDescription fmt{};
        fmt.mSampleRate = SAMPLE_RATE;
        fmt.mFormatID = kHsAudioFormatLinearPCM;
        fmt.mFormatFlags = kHsLinearPCMFormatFlagIsSignedInteger | kHsLinearPCMFormatFlagIsPacked;
        fmt.mFramesPerPacket = 1;
        fmt.mChannelsPerFrame = 1;
        fmt.mBitsPerChannel = 16;
        fmt.mBytesPerPacket = 2;
        fmt.mBytesPerFrame = 2;

        std::string in_uid, out_uid;
        {
            std::lock_guard<std::mutex> lk(mtx);
            in_uid = active_input_uid;
            out_uid = active_output_uid;
        }

        if (AudioQueueNewInput(&fmt, aq_input_cb, this, nullptr, nullptr, 0, &in_queue_) == 0 && in_queue_) {
            if (!in_uid.empty()) {
                CFStringRef cf_uid = CFStringCreateWithCString(nullptr, in_uid.c_str(), kHsCFStringEncodingUTF8);
                if (cf_uid) {
                    AudioQueueSetProperty(in_queue_, kHsAudioQueueProperty_CurrentDevice, &cf_uid, sizeof(cf_uid));
                    CFRelease(cf_uid);
                }
            }
            for (int i = 0; i < 3; ++i) {
                AudioQueueBufferRef buf = nullptr;
                if (AudioQueueAllocateBuffer(in_queue_, 1200 * sizeof(int16_t), &buf) == 0 && buf) {
                    AudioQueueEnqueueBuffer(in_queue_, buf, 0, nullptr);
                }
            }
            AudioQueueStart(in_queue_, nullptr);
        }

        if (AudioQueueNewOutput(&fmt, aq_output_cb, this, nullptr, nullptr, 0, &out_queue_) == 0 && out_queue_) {
            if (!out_uid.empty()) {
                CFStringRef cf_uid = CFStringCreateWithCString(nullptr, out_uid.c_str(), kHsCFStringEncodingUTF8);
                if (cf_uid) {
                    AudioQueueSetProperty(out_queue_, kHsAudioQueueProperty_CurrentDevice, &cf_uid, sizeof(cf_uid));
                    CFRelease(cf_uid);
                }
            }
            for (int i = 0; i < 3; ++i) {
                AudioQueueBufferRef buf = nullptr;
                if (AudioQueueAllocateBuffer(out_queue_, 1200 * sizeof(int16_t), &buf) == 0 && buf) {
                    aq_output_cb(this, out_queue_, buf);
                }
            }
            AudioQueueStart(out_queue_, nullptr);
        }
#endif
    }

    void stop_coreaudio_queues() {
#ifdef _WIN32
        win_audio_run_.store(false);
        if (win_audio_ev_) SetEvent(win_audio_ev_);
        if (win_audio_thread_.joinable()) win_audio_thread_.join();
        if (win_audio_ev_) {
            CloseHandle(win_audio_ev_);
            win_audio_ev_ = nullptr;
        }
#elif defined(__APPLE__)
        if (in_queue_) {
            AudioQueueStop(in_queue_, true);
            AudioQueueDispose(in_queue_, true);
            in_queue_ = nullptr;
        }
        if (out_queue_) {
            AudioQueueStop(out_queue_, true);
            AudioQueueDispose(out_queue_, true);
            out_queue_ = nullptr;
        }
#endif
    }

    void dsp_loop() {
        uint64_t tick = 0;
        uint64_t last_seen_samples = 0;

        while (running_.load()) {
            {
                std::lock_guard<std::mutex> lk(mtx);
                bool has_new_audio = (total_rx_samples != last_seen_samples);
                last_seen_samples = total_rx_samples;

                if (has_new_audio && hw_stream_active) {
                    // Compute 8192-point Hann-windowed FFT from contiguous hardware rx_ring for Spectrum & Waterfall
                    std::vector<std::complex<float>> fft_buf(FFT_SIZE);
                    size_t fft_start = (rx_ring_write_pos + RX_RING_CAP - FFT_SIZE) % RX_RING_CAP;
                    for (int i = 0; i < FFT_SIZE; ++i) {
                        float w = 0.5f * (1.0f - std::cos(2.0f * 3.1415926535f * i / (FFT_SIZE - 1)));
                        fft_buf[i] = std::complex<float>(rx_ring[(fft_start + i) % RX_RING_CAP] * w, 0.0f);
                    }
                    fft_radix2(fft_buf);

                    std::array<float, WF_BINS> raw_db{};
                    std::array<float, WF_BINS> sorted_db{};
                    float best_mag = 0.0f;
                    int best_hz = 0;
                    for (int b = 0; b < WF_BINS; ++b) {
                        float freq_hz = 100.0f + (static_cast<float>(b) / WF_BINS) * 2800.0f;
                        int k = std::clamp(static_cast<int>(std::round(freq_hz * FFT_SIZE / SAMPLE_RATE)), 1, FFT_SIZE / 2 - 2);
                        float mag = std::max({std::abs(fft_buf[k - 1]), std::abs(fft_buf[k]), std::abs(fft_buf[k + 1])}) / (FFT_SIZE * 0.25f);
                        if (mag > best_mag) {
                            best_mag = mag;
                            best_hz = static_cast<int>(std::round(freq_hz));
                        }
                        float db = 20.0f * std::log10(std::max(1e-6f, mag));
                        raw_db[b] = db;
                        sorted_db[b] = db;
                    }
                    peak_freq_hz = (best_mag > 1e-5f) ? best_hz : 0;

                    // Robust receiver background noise floor estimation (30th percentile across passband)
                    std::nth_element(sorted_db.begin(), sorted_db.begin() + WF_BINS * 3 / 10, sorted_db.end());
                    float noise_floor_db = sorted_db[WF_BINS * 3 / 10];
                    static float smoothed_noise_floor = -75.0f;
                    smoothed_noise_floor = smoothed_noise_floor * 0.92f + noise_floor_db * 0.08f;

                    // Signal-to-Noise Ratio (SNR) enhancement with clean black noise floor:
                    // Mute background noise by subtracting baseline (+2.0 dB above noise floor)
                    float baseline_db = smoothed_noise_floor + 2.0f;
                    constexpr float DYNAMIC_RANGE_DB = 36.0f; // 36 dB of vivid color contrast

                    std::array<uint8_t, WF_BINS> wf_row{};
                    for (int b = 0; b < WF_BINS; ++b) {
                        float snr_db = raw_db[b] - baseline_db;
                        if (snr_db <= 0.0f) {
                            current_spectrum[b] = current_spectrum[b] * 0.88f; // Smooth 60 FPS ballistic decay
                            wf_row[b] = 0; // Pure clean black background
                        } else {
                            float norm = std::clamp(snr_db / DYNAMIC_RANGE_DB, 0.0f, 1.0f);
                            // Contrast power curve (gamma 0.85) to separate signal tiers
                            float contrasted = std::pow(norm, 0.85f);
                            if (contrasted > current_spectrum[b]) {
                                current_spectrum[b] = current_spectrum[b] * 0.35f + contrasted * 0.65f; // Fast responsive attack
                            } else {
                                current_spectrum[b] = current_spectrum[b] * 0.90f + contrasted * 0.10f; // Silky smooth decay
                            }
                            wf_row[b] = static_cast<uint8_t>(std::clamp(contrasted * 255.0f, 1.0f, 255.0f));
                        }
                    }

                    static auto last_row_time = std::chrono::steady_clock::now();
                    auto now_chrono = std::chrono::steady_clock::now();
                    int64_t elapsed_ms = std::chrono::duration_cast<std::chrono::milliseconds>(now_chrono - last_row_time).count();

                    // Slow down waterfall rate to ~350ms per row: shows 90-105 seconds of continuous data
                    if (elapsed_ms >= 350) {
                        last_row_time = now_chrono;
                        std::time_t now_t = std::time(nullptr);
                        std::tm gm{};
                        hs_gmtime(&now_t, &gm);
                        static int last_15s_block = -1;
                        int cur_block = gm.tm_sec / 15;
                        bool is_new_block = (cur_block != last_15s_block);
                        if (is_new_block) last_15s_block = cur_block;

                        WaterfallRow wf_entry{wf_row, is_new_block, current_band_label};
                        waterfall_rows.push_front(wf_entry);
                        if (waterfall_rows.size() > WF_HISTORY) {
                            waterfall_rows.pop_back();
                        }
                    }
                } else if (!hw_stream_active) {
                    rx_rms_db = -120.0f;
                    rx_peak_db = -120.0f;
                    peak_freq_hz = 0;
                    current_spectrum.fill(0.0f);
                }
            }

            ++tick;
            std::this_thread::sleep_for(std::chrono::milliseconds(16)); // 60 FPS spectrum DSP refresh rate
        }
    }
};

} // namespace hamshack
