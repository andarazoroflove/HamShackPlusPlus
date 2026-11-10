// ============================================================================
// AudioRouter.cpp — Carbon Engine Zero-Copy Lock-Free Dual-Source Audio Router
// ============================================================================

#include "AudioRouter.hpp"
#include "hamshack_platform.hpp"

#include <cctype>
#include <cstring>

#ifdef _WIN32
#include <mmsystem.h>
#endif

#ifdef __APPLE__
extern "C" {
typedef uint32_t AudioObjectID;
typedef AudioObjectID AudioDeviceID;
typedef uint32_t AudioObjectPropertySelector;
typedef uint32_t AudioObjectPropertyScope;
typedef uint32_t AudioObjectPropertyElement;
typedef int32_t OSStatus;
typedef const void* CFTypeRef;
typedef const struct __CFString* CFStringRef;
typedef unsigned int CFStringEncoding;

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

struct AudioQueueBuffer {
    uint32_t mAudioDataBytesCapacity;
    void*    mAudioData;
    uint32_t mAudioDataByteSize;
    void*    mUserData;
    uint32_t mPacketDescriptionCapacity;
    void*    mPacketDescriptions;
    uint32_t mPacketDescriptionCount;
};

typedef struct OpaqueAudioQueue* AudioQueueRef;
typedef AudioQueueBuffer* AudioQueueBufferRef;
struct AudioTimeStamp;
struct AudioStreamPacketDescription;

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

OSStatus AudioObjectGetPropertyDataSize(AudioObjectID, const AudioObjectPropertyAddress*, uint32_t, const void*, uint32_t*);
OSStatus AudioObjectGetPropertyData(AudioObjectID, const AudioObjectPropertyAddress*, uint32_t, const void*, uint32_t*, void*);
OSStatus AudioQueueNewInput(const AudioStreamBasicDescription*, AudioQueueInputCallback, void*, void*, const void*, uint32_t, AudioQueueRef*);
OSStatus AudioQueueNewOutput(const AudioStreamBasicDescription*, AudioQueueOutputCallback, void*, void*, const void*, uint32_t, AudioQueueRef*);
OSStatus AudioQueueSetProperty(AudioQueueRef, uint32_t, const void*, uint32_t);
OSStatus AudioQueueAllocateBuffer(AudioQueueRef, uint32_t, AudioQueueBufferRef*);
OSStatus AudioQueueEnqueueBuffer(AudioQueueRef, AudioQueueBufferRef, uint32_t, const void*);
OSStatus AudioQueueStart(AudioQueueRef, const void*);
OSStatus AudioQueueStop(AudioQueueRef, uint8_t);
OSStatus AudioQueueDispose(AudioQueueRef, uint8_t);
void CFRelease(CFTypeRef);
CFStringRef CFStringCreateWithCString(const void*, const char*, CFStringEncoding);
uint8_t CFStringGetCString(CFStringRef, char*, long, CFStringEncoding);
}
#endif

namespace hamshack::carbon {

namespace {

inline constexpr double kPi = 3.14159265358979323846;

void copy_bounded_str(std::span<char> dst, std::string_view src) noexcept {
    if (dst.empty()) return;
    const size_t n = std::min(dst.size() - 1, src.size());
    if (n > 0) {
        std::memcpy(dst.data(), src.data(), n);
    }
    dst[n] = '\0';
}

bool contains_usb_keyword(std::string_view text) noexcept {
    std::array<char, 128> lower{};
    const size_t n = std::min(lower.size() - 1, text.size());
    for (size_t i = 0; i < n; ++i) {
        lower[i] = static_cast<char>(std::tolower(static_cast<unsigned char>(text[i])));
    }
    std::string_view sv{lower.data(), n};
    return sv.find("usb") != std::string_view::npos ||
           sv.find("codec") != std::string_view::npos ||
           sv.find("c-media") != std::string_view::npos ||
           sv.find("pnp") != std::string_view::npos ||
           sv.find("aprs") != std::string_view::npos ||
           sv.find("digirig") != std::string_view::npos;
}

#ifdef _WIN32
uint32_t match_winmm_output_device(std::string_view input_dev_name) noexcept {
    const UINT n_out = waveOutGetNumDevs();
    if (n_out == 0) return WAVE_MAPPER;

    // Strip common capture prefixes ("Microphone (", "Line (") to match speaker/dongle suffix
    std::string_view suffix = input_dev_name;
    const size_t open_paren = suffix.find('(');
    if (open_paren != std::string_view::npos) {
        suffix = suffix.substr(open_paren);
    }

    uint32_t usb_fallback = WAVE_MAPPER;
    for (UINT i = 0; i < n_out; ++i) {
        WAVEOUTCAPSW caps{};
        if (waveOutGetDevCapsW(i, &caps, sizeof(caps)) == MMSYSERR_NOERROR) {
            char utf8[96]{};
            WideCharToMultiByte(CP_UTF8, 0, caps.szPname, -1, utf8, sizeof(utf8) - 1, nullptr, nullptr);
            std::string_view out_sv{utf8};
            if (!suffix.empty() && out_sv.find(suffix) != std::string_view::npos) {
                return static_cast<uint32_t>(i);
            }
            if (usb_fallback == WAVE_MAPPER && contains_usb_keyword(out_sv)) {
                usb_fallback = static_cast<uint32_t>(i);
            }
        }
    }
    return usb_fallback;
}
#endif

} // namespace

// ============================================================================
// PlatformDongleStream — RAII OS Audio Capture Handle for Source B (USB Dongle)
// Pre-allocates fixed DMA buffers; callback uses zero locks & zero heap allocs.
// ============================================================================
struct AudioRouter::PlatformDongleStream {
    AudioRouter* owner = nullptr;
    std::atomic<bool> closing{false};

#ifdef _WIN32
    static constexpr size_t kNumBufs   = 4;
    static constexpr size_t kBufFrames = 600; // 50 ms @ 12 kHz
    HWAVEIN hWaveIn = nullptr;
    std::array<WAVEHDR, kNumBufs> hdrs{};
    std::array<std::array<int16_t, kBufFrames>, kNumBufs> pcm_dma{};

    static void CALLBACK wave_in_proc(HWAVEIN hwi, UINT msg, DWORD_PTR inst, DWORD_PTR p1, DWORD_PTR) noexcept {
        if (msg != WIM_DATA || inst == 0 || p1 == 0) return;
        auto* self = reinterpret_cast<PlatformDongleStream*>(inst);
        if (self->closing.load(std::memory_order_acquire) || !self->owner) return;

        auto* hdr = reinterpret_cast<WAVEHDR*>(p1);
        const size_t n_samples = std::min<size_t>(hdr->dwBytesRecorded / sizeof(int16_t), kBufFrames);
        if (n_samples > 0 && hdr->lpData != nullptr) {
            std::array<float, kBufFrames> norm{};
            const auto* src = reinterpret_cast<const int16_t*>(hdr->lpData);
            std::span<const int16_t> src_span{src, n_samples};
            for (size_t i = 0; i < src_span.size(); ++i) {
                norm[i] = static_cast<float>(src_span[i]) * (1.0f / 32768.0f);
            }
            self->owner->push_usb_dongle_samples(std::span<const float>{norm.data(), n_samples});
        }
        if (!self->closing.load(std::memory_order_acquire)) {
            waveInAddBuffer(hwi, hdr, sizeof(WAVEHDR));
        }
    }

    bool start(AudioRouter* router, uint32_t winmm_dev_id) noexcept {
        owner = router;
        closing.store(false, std::memory_order_release);

        WAVEFORMATEX wfx{};
        wfx.wFormatTag      = WAVE_FORMAT_PCM;
        wfx.nChannels       = 1;
        wfx.nSamplesPerSec  = static_cast<DWORD>(kRouterSampleRateHz);
        wfx.wBitsPerSample  = 16;
        wfx.nBlockAlign     = 2;
        wfx.nAvgBytesPerSec = wfx.nSamplesPerSec * wfx.nBlockAlign;

        if (waveInOpen(&hWaveIn, static_cast<UINT>(winmm_dev_id), &wfx,
                       reinterpret_cast<DWORD_PTR>(&PlatformDongleStream::wave_in_proc),
                       reinterpret_cast<DWORD_PTR>(this), CALLBACK_FUNCTION) != MMSYSERR_NOERROR) {
            hWaveIn = nullptr;
            return false;
        }

        for (size_t i = 0; i < kNumBufs; ++i) {
            hdrs[i] = WAVEHDR{};
            hdrs[i].lpData         = reinterpret_cast<LPSTR>(pcm_dma[i].data());
            hdrs[i].dwBufferLength = static_cast<DWORD>(kBufFrames * sizeof(int16_t));
            waveInPrepareHeader(hWaveIn, &hdrs[i], sizeof(WAVEHDR));
            waveInAddBuffer(hWaveIn, &hdrs[i], sizeof(WAVEHDR));
        }
        if (waveInStart(hWaveIn) != MMSYSERR_NOERROR) {
            stop();
            return false;
        }
        return true;
    }

    void stop() noexcept {
        closing.store(true, std::memory_order_release);
        if (hWaveIn != nullptr) {
            waveInReset(hWaveIn);
            for (size_t i = 0; i < kNumBufs; ++i) {
                if (hdrs[i].dwFlags & WHDR_PREPARED) {
                    waveInUnprepareHeader(hWaveIn, &hdrs[i], sizeof(WAVEHDR));
                }
            }
            waveInClose(hWaveIn);
            hWaveIn = nullptr;
        }
    }
#elif defined(__APPLE__)
    static constexpr size_t kNumBufs   = 4;
    static constexpr size_t kBufFrames = 600;
    AudioQueueRef aq = nullptr;
    std::array<AudioQueueBufferRef, kNumBufs> bufs{};

    static void aq_input_proc(void* user, AudioQueueRef q, AudioQueueBufferRef buf,
                              const AudioTimeStamp*, uint32_t, const AudioStreamPacketDescription*) noexcept {
        auto* self = static_cast<PlatformDongleStream*>(user);
        if (!self || self->closing.load(std::memory_order_acquire) || !self->owner || !buf) return;

        const size_t n = buf->mAudioDataByteSize / sizeof(float);
        if (n > 0 && buf->mAudioData != nullptr) {
            const auto* fptr = static_cast<const float*>(buf->mAudioData);
            self->owner->push_usb_dongle_samples(std::span<const float>{fptr, n});
        }
        if (!self->closing.load(std::memory_order_acquire)) {
            AudioQueueEnqueueBuffer(q, buf, 0, nullptr);
        }
    }

    bool start(AudioRouter* router, std::string_view uid) noexcept {
        owner = router;
        closing.store(false, std::memory_order_release);

        AudioStreamBasicDescription fmt{};
        fmt.mSampleRate       = static_cast<double>(kRouterSampleRateHz);
        fmt.mFormatID         = 0x6C70636Du; // 'lpcm'
        fmt.mFormatFlags      = (1u << 0) | (1u << 3); // Float | Packed
        fmt.mBytesPerPacket   = 4;
        fmt.mFramesPerPacket  = 1;
        fmt.mBytesPerFrame    = 4;
        fmt.mChannelsPerFrame = 1;
        fmt.mBitsPerChannel   = 32;

        if (AudioQueueNewInput(&fmt, &PlatformDongleStream::aq_input_proc, this,
                               nullptr, nullptr, 0, &aq) != 0 || !aq) {
            aq = nullptr;
            return false;
        }
        if (!uid.empty()) {
            std::array<char, 96> c_uid{};
            copy_bounded_str(c_uid, uid);
            CFStringRef cf_uid = CFStringCreateWithCString(nullptr, c_uid.data(), 0x08000100u);
            if (cf_uid) {
                constexpr uint32_t kAudioQueueProperty_CurrentDevice = 0x61716364u; // 'aqcd'
                AudioQueueSetProperty(aq, kAudioQueueProperty_CurrentDevice, &cf_uid, sizeof(cf_uid));
                CFRelease(cf_uid);
            }
        }
        for (size_t i = 0; i < kNumBufs; ++i) {
            if (AudioQueueAllocateBuffer(aq, static_cast<uint32_t>(kBufFrames * sizeof(float)), &bufs[i]) == 0 && bufs[i]) {
                AudioQueueEnqueueBuffer(aq, bufs[i], 0, nullptr);
            }
        }
        if (AudioQueueStart(aq, nullptr) != 0) {
            stop();
            return false;
        }
        return true;
    }

    void stop() noexcept {
        closing.store(true, std::memory_order_release);
        if (aq != nullptr) {
            AudioQueueStop(aq, 1);
            AudioQueueDispose(aq, 1);
            aq = nullptr;
        }
    }
#else
    bool start(AudioRouter* router, uint32_t) noexcept {
        owner = router;
        return false;
    }
    void stop() noexcept {}
#endif

    ~PlatformDongleStream() {
        stop();
    }
};

// ============================================================================
// PlatformDongleTxStream — RAII 48 kHz 16-Bit Output Handle for Source B VOX
// Pulls lock-free from tx_pcm48_ring_ when UsbDongleVhf is selected; feeds
// zero-filled silence otherwise so OS DMA stays warm without glitches.
// ============================================================================
struct AudioRouter::PlatformDongleTxStream {
    AudioRouter* owner = nullptr;
    std::atomic<bool> closing{false};

#ifdef _WIN32
    static constexpr size_t kNumBufs   = 4;
    static constexpr size_t kBufFrames = 960; // 20 ms @ 48 kHz
    HWAVEOUT hWaveOut = nullptr;
    std::array<WAVEHDR, kNumBufs> hdrs{};
    std::array<std::array<int16_t, kBufFrames>, kNumBufs> pcm_dma{};

    void fill_and_queue(WAVEHDR* hdr) noexcept {
        if (!hdr || !hWaveOut || closing.load(std::memory_order_acquire)) return;
        auto* dst = reinterpret_cast<int16_t*>(hdr->lpData);
        std::span<int16_t> span_buf{dst, kBufFrames};
        size_t got = 0;
        if (owner && owner->active_source() == AudioSource::UsbDongleVhf) {
            got = owner->pop_tx_pcm48(span_buf);
        }
        if (got < kBufFrames) {
            std::fill(span_buf.begin() + static_cast<std::ptrdiff_t>(got), span_buf.end(), int16_t{0});
        }
        hdr->dwBufferLength = static_cast<DWORD>(kBufFrames * sizeof(int16_t));
        waveOutWrite(hWaveOut, hdr, sizeof(WAVEHDR));
    }

    static void CALLBACK wave_out_proc(HWAVEOUT, UINT msg, DWORD_PTR inst, DWORD_PTR p1, DWORD_PTR) noexcept {
        if (msg != WOM_DONE || inst == 0 || p1 == 0) return;
        auto* self = reinterpret_cast<PlatformDongleTxStream*>(inst);
        if (self->closing.load(std::memory_order_acquire)) return;
        self->fill_and_queue(reinterpret_cast<WAVEHDR*>(p1));
    }

    bool start(AudioRouter* router, uint32_t winmm_out_dev_id) noexcept {
        owner = router;
        closing.store(false, std::memory_order_release);

        WAVEFORMATEX wfx{};
        wfx.wFormatTag      = WAVE_FORMAT_PCM;
        wfx.nChannels       = 1;
        wfx.nSamplesPerSec  = static_cast<DWORD>(kTxOutputSampleRateHz);
        wfx.wBitsPerSample  = 16;
        wfx.nBlockAlign     = 2;
        wfx.nAvgBytesPerSec = wfx.nSamplesPerSec * wfx.nBlockAlign;

        if (waveOutOpen(&hWaveOut, static_cast<UINT>(winmm_out_dev_id), &wfx,
                        reinterpret_cast<DWORD_PTR>(&PlatformDongleTxStream::wave_out_proc),
                        reinterpret_cast<DWORD_PTR>(this), CALLBACK_FUNCTION) != MMSYSERR_NOERROR) {
            hWaveOut = nullptr;
            return false;
        }

        for (size_t i = 0; i < kNumBufs; ++i) {
            pcm_dma[i].fill(0);
            hdrs[i] = WAVEHDR{};
            hdrs[i].lpData         = reinterpret_cast<LPSTR>(pcm_dma[i].data());
            hdrs[i].dwBufferLength = static_cast<DWORD>(kBufFrames * sizeof(int16_t));
            waveOutPrepareHeader(hWaveOut, &hdrs[i], sizeof(WAVEHDR));
            waveOutWrite(hWaveOut, &hdrs[i], sizeof(WAVEHDR));
        }
        return true;
    }

    void stop() noexcept {
        closing.store(true, std::memory_order_release);
        if (hWaveOut != nullptr) {
            waveOutReset(hWaveOut);
            for (size_t i = 0; i < kNumBufs; ++i) {
                if (hdrs[i].dwFlags & WHDR_PREPARED) {
                    waveOutUnprepareHeader(hWaveOut, &hdrs[i], sizeof(WAVEHDR));
                }
            }
            waveOutClose(hWaveOut);
            hWaveOut = nullptr;
        }
    }
#elif defined(__APPLE__)
    static constexpr size_t kNumBufs   = 4;
    static constexpr size_t kBufFrames = 960; // 20 ms @ 48 kHz
    AudioQueueRef aq = nullptr;
    std::array<AudioQueueBufferRef, kNumBufs> bufs{};

    static void aq_output_proc(void* user, AudioQueueRef q, AudioQueueBufferRef buf) noexcept {
        auto* self = static_cast<PlatformDongleTxStream*>(user);
        if (!self || self->closing.load(std::memory_order_acquire) || !buf || !buf->mAudioData) return;

        auto* dst = static_cast<int16_t*>(buf->mAudioData);
        std::span<int16_t> span_buf{dst, kBufFrames};
        size_t got = 0;
        if (self->owner && self->owner->active_source() == AudioSource::UsbDongleVhf) {
            got = self->owner->pop_tx_pcm48(span_buf);
        }
        if (got < kBufFrames) {
            std::fill(span_buf.begin() + static_cast<std::ptrdiff_t>(got), span_buf.end(), int16_t{0});
        }
        buf->mAudioDataByteSize = static_cast<uint32_t>(kBufFrames * sizeof(int16_t));
        if (!self->closing.load(std::memory_order_acquire)) {
            AudioQueueEnqueueBuffer(q, buf, 0, nullptr);
        }
    }

    bool start(AudioRouter* router, std::string_view uid) noexcept {
        owner = router;
        closing.store(false, std::memory_order_release);

        AudioStreamBasicDescription fmt{};
        fmt.mSampleRate       = static_cast<double>(kTxOutputSampleRateHz);
        fmt.mFormatID         = 0x6C70636Du; // 'lpcm'
        fmt.mFormatFlags      = (1u << 2) | (1u << 3); // kAudioFormatFlagIsSignedInteger | Packed
        fmt.mBytesPerPacket   = 2;
        fmt.mFramesPerPacket  = 1;
        fmt.mBytesPerFrame    = 2;
        fmt.mChannelsPerFrame = 1;
        fmt.mBitsPerChannel   = 16;

        if (AudioQueueNewOutput(&fmt, &PlatformDongleTxStream::aq_output_proc, this,
                                nullptr, nullptr, 0, &aq) != 0 || !aq) {
            aq = nullptr;
            return false;
        }
        if (!uid.empty()) {
            std::array<char, 96> c_uid{};
            copy_bounded_str(c_uid, uid);
            CFStringRef cf_uid = CFStringCreateWithCString(nullptr, c_uid.data(), 0x08000100u);
            if (cf_uid) {
                constexpr uint32_t kAudioQueueProperty_CurrentDevice = 0x61716364u; // 'aqcd'
                AudioQueueSetProperty(aq, kAudioQueueProperty_CurrentDevice, &cf_uid, sizeof(cf_uid));
                CFRelease(cf_uid);
            }
        }
        for (size_t i = 0; i < kNumBufs; ++i) {
            if (AudioQueueAllocateBuffer(aq, static_cast<uint32_t>(kBufFrames * sizeof(int16_t)), &bufs[i]) == 0 && bufs[i]) {
                std::memset(bufs[i]->mAudioData, 0, kBufFrames * sizeof(int16_t));
                bufs[i]->mAudioDataByteSize = static_cast<uint32_t>(kBufFrames * sizeof(int16_t));
                AudioQueueEnqueueBuffer(aq, bufs[i], 0, nullptr);
            }
        }
        if (AudioQueueStart(aq, nullptr) != 0) {
            stop();
            return false;
        }
        return true;
    }

    void stop() noexcept {
        closing.store(true, std::memory_order_release);
        if (aq != nullptr) {
            AudioQueueStop(aq, 1);
            AudioQueueDispose(aq, 1);
            aq = nullptr;
        }
    }
#else
    bool start(AudioRouter* router, uint32_t) noexcept {
        owner = router;
        return false;
    }
    void stop() noexcept {}
#endif

    ~PlatformDongleTxStream() {
        stop();
    }
};

AudioRouter::AudioRouter() {
    refresh_usb_dongle_devices(0);
}

AudioRouter::~AudioRouter() {
    stop_usb_dongle_stream();
}

void AudioRouter::refresh_usb_dongle_devices(int primary_g90_device_idx) {
    device_count_ = 0;
#ifdef _WIN32
    const UINT n_in = waveInGetNumDevs();
    for (UINT i = 0; i < n_in && device_count_ < kMaxDevices; ++i) {
        WAVEINCAPSW caps{};
        if (waveInGetDevCapsW(i, &caps, sizeof(caps)) == MMSYSERR_NOERROR) {
            char utf8[96]{};
            WideCharToMultiByte(CP_UTF8, 0, caps.szPname, -1, utf8, sizeof(utf8) - 1, nullptr, nullptr);
            auto& entry = devices_[device_count_];
            entry = RouterDeviceEntry{};
            entry.os_device_id    = static_cast<uint32_t>(i);
            copy_bounded_str(entry.name, utf8);
            entry.os_out_dev_id   = match_winmm_output_device(entry.name_view());
            entry.has_output      = (waveOutGetNumDevs() > 0);
            std::string uid_s     = "winmm_in_" + std::to_string(i);
            copy_bounded_str(entry.uid, uid_s);
            entry.nominal_rate_hz = (caps.dwFormats & (WAVE_FORMAT_48M16 | WAVE_FORMAT_48S16)) ? 48000 : 44100;
            entry.is_usb_dongle   = contains_usb_keyword(entry.name_view());
            ++device_count_;
        }
    }
#elif defined(__APPLE__)
    constexpr uint32_t kSysObj      = 1u;
    constexpr uint32_t kPropDevs    = 0x64657623u; // 'dev#'
    constexpr uint32_t kPropName    = 0x6C6E616Du; // 'lnam'
    constexpr uint32_t kPropUid     = 0x75696420u; // 'uid '
    constexpr uint32_t kPropStreams = 0x73746D23u; // 'stm#'
    constexpr uint32_t kScopeGlob   = 0x676C6F62u; // 'glob'
    constexpr uint32_t kScopeIn     = 0x696E7074u; // 'inpt'
    constexpr uint32_t kScopeOut    = 0x6F757470u; // 'outp'

    AudioObjectPropertyAddress addr{kPropDevs, kScopeGlob, 0u};
    uint32_t sz = 0;
    if (AudioObjectGetPropertyDataSize(kSysObj, &addr, 0, nullptr, &sz) == 0 && sz >= sizeof(AudioDeviceID)) {
        std::array<AudioDeviceID, 32> ids{};
        uint32_t cap_bytes = static_cast<uint32_t>(std::min<size_t>(sz, ids.size() * sizeof(AudioDeviceID)));
        if (AudioObjectGetPropertyData(kSysObj, &addr, 0, nullptr, &cap_bytes, ids.data()) == 0) {
            const size_t count = cap_bytes / sizeof(AudioDeviceID);
            for (size_t i = 0; i < count && device_count_ < kMaxDevices; ++i) {
                AudioObjectPropertyAddress st_addr{kPropStreams, kScopeIn, 0u};
                uint32_t st_sz = 0;
                if (AudioObjectGetPropertyDataSize(ids[i], &st_addr, 0, nullptr, &st_sz) != 0 || st_sz == 0) {
                    continue;
                }
                auto& entry = devices_[device_count_];
                entry = RouterDeviceEntry{};
                entry.os_device_id  = ids[i];
                entry.os_out_dev_id = ids[i];

                AudioObjectPropertyAddress out_addr{kPropStreams, kScopeOut, 0u};
                uint32_t out_sz = 0;
                entry.has_output = (AudioObjectGetPropertyDataSize(ids[i], &out_addr, 0, nullptr, &out_sz) == 0 && out_sz > 0);

                CFStringRef cf_name = nullptr;
                uint32_t nsz = sizeof(cf_name);
                AudioObjectPropertyAddress n_addr{kPropName, kScopeGlob, 0u};
                if (AudioObjectGetPropertyData(ids[i], &n_addr, 0, nullptr, &nsz, &cf_name) == 0 && cf_name) {
                    CFStringGetCString(cf_name, entry.name.data(), static_cast<long>(entry.name.size()), 0x08000100u);
                    CFRelease(cf_name);
                } else {
                    copy_bounded_str(entry.name, "CoreAudio Input");
                }

                CFStringRef cf_uid = nullptr;
                uint32_t usz = sizeof(cf_uid);
                AudioObjectPropertyAddress u_addr{kPropUid, kScopeGlob, 0u};
                if (AudioObjectGetPropertyData(ids[i], &u_addr, 0, nullptr, &usz, &cf_uid) == 0 && cf_uid) {
                    CFStringGetCString(cf_uid, entry.uid.data(), static_cast<long>(entry.uid.size()), 0x08000100u);
                    CFRelease(cf_uid);
                }
                entry.is_usb_dongle = contains_usb_keyword(entry.name_view());
                ++device_count_;
            }
        }
    }
#endif

    // Prefer a second USB audio dongle distinct from the primary G90 stream when available
    int best_idx = -1;
    for (size_t i = 0; i < device_count_; ++i) {
        if (static_cast<int>(i) != primary_g90_device_idx && devices_[i].is_usb_dongle) {
            best_idx = static_cast<int>(i);
            break;
        }
    }
    if (best_idx < 0 && device_count_ > 1) {
        best_idx = (primary_g90_device_idx == 0) ? 1 : 0;
    } else if (best_idx < 0 && device_count_ == 1) {
        best_idx = 0;
    }
    selected_dongle_idx_.store(best_idx, std::memory_order_release);
}

void AudioRouter::select_source(AudioSource src) noexcept {
    const AudioSource prev = active_source_.exchange(src, std::memory_order_acq_rel);
    if (prev != src) {
        route_epoch_.fetch_add(1u, std::memory_order_acq_rel);
    }
}

void AudioRouter::toggle_source() noexcept {
    const AudioSource cur = active_source_.load(std::memory_order_acquire);
    select_source(cur == AudioSource::PrimaryG90 ? AudioSource::UsbDongleVhf : AudioSource::PrimaryG90);
}

bool AudioRouter::bind_usb_dongle_device(int device_idx) {
    stop_usb_dongle_stream();
    if (device_idx < 0 || static_cast<size_t>(device_idx) >= device_count_) {
        return false;
    }
    selected_dongle_idx_.store(device_idx, std::memory_order_release);
    auto rx_stream = std::make_unique<PlatformDongleStream>();
    auto tx_stream = std::make_unique<PlatformDongleTxStream>();

#ifdef __APPLE__
    const std::string_view uid_sv{devices_[static_cast<size_t>(device_idx)].uid.data()};
    const bool rx_ok = rx_stream->start(this, uid_sv);
    if (devices_[static_cast<size_t>(device_idx)].has_output && tx_stream->start(this, uid_sv)) {
        dongle_tx_hw_ = std::move(tx_stream);
    }
#else
    const auto& dev  = devices_[static_cast<size_t>(device_idx)];
    const bool rx_ok = rx_stream->start(this, dev.os_device_id);
    if (dev.has_output && tx_stream->start(this, dev.os_out_dev_id)) {
        dongle_tx_hw_ = std::move(tx_stream);
    }
#endif
    if (rx_ok) {
        dongle_hw_ = std::move(rx_stream);
        dongle_stream_open_.store(true, std::memory_order_release);
        route_epoch_.fetch_add(1u, std::memory_order_acq_rel);
        return true;
    }
    dongle_stream_open_.store(false, std::memory_order_release);
    return false;
}

void AudioRouter::stop_usb_dongle_stream() noexcept {
    dongle_stream_open_.store(false, std::memory_order_release);
    if (dongle_tx_hw_) {
        dongle_tx_hw_->stop();
        dongle_tx_hw_.reset();
    }
    if (dongle_hw_) {
        dongle_hw_->stop();
        dongle_hw_.reset();
    }
}

std::string_view AudioRouter::selected_dongle_name() const noexcept {
    const int idx = selected_dongle_idx_.load(std::memory_order_acquire);
    if (idx >= 0 && static_cast<size_t>(idx) < device_count_) {
        return devices_[static_cast<size_t>(idx)].name_view();
    }
    return std::string_view{"USB Audio Dongle (Unbound)"};
}

float AudioRouter::compute_rms_dbfs(std::span<const float> pcm) noexcept {
    if (pcm.empty()) return -120.0f;
    double acc = 0.0;
    for (float s : pcm) {
        acc += static_cast<double>(s) * static_cast<double>(s);
    }
    const double rms = std::sqrt(acc / static_cast<double>(pcm.size()));
    if (rms <= 1.0e-6) return -120.0f;
    return static_cast<float>(20.0 * std::log10(rms));
}

void AudioRouter::push_primary_g90_samples(std::span<const float> mono_12k) noexcept {
    if (mono_12k.empty()) return;
    rms_a_dbfs_.store(compute_rms_dbfs(mono_12k), std::memory_order_relaxed);
    ring_source_a_.push_span(mono_12k);
}

void AudioRouter::push_usb_dongle_samples(std::span<const float> mono_12k) noexcept {
    if (mono_12k.empty()) return;
    rms_b_dbfs_.store(compute_rms_dbfs(mono_12k), std::memory_order_relaxed);
    ring_source_b_.push_span(mono_12k);
}

size_t AudioRouter::pump_routing() noexcept {
    const AudioSource active = active_source_.load(std::memory_order_acquire);
    size_t routed = 0;

    // Drain both SPSC queues so the unselected source never accumulates stale latency!
    for (;;) {
        const size_t na = ring_source_a_.pop_span(pump_scratch_a_);
        const size_t nb = ring_source_b_.pop_span(pump_scratch_b_);
        if (na == 0 && nb == 0) break;

        if (active == AudioSource::PrimaryG90 && na > 0) {
            broadcast_ring_.publish(std::span<const float>{pump_scratch_a_.data(), na});
            routed += na;
        } else if (active == AudioSource::UsbDongleVhf && nb > 0) {
            broadcast_ring_.publish(std::span<const float>{pump_scratch_b_.data(), nb});
            routed += nb;
        }
    }
    return routed;
}

size_t AudioRouter::read_active_stream(AudioTapCursor& cursor, std::span<float> dst) noexcept {
    pump_routing();
    const uint32_t epoch = route_epoch_.load(std::memory_order_acquire);
    return broadcast_ring_.read_tap(cursor, dst, epoch);
}

void AudioRouter::apply_cosine_taper_48k(std::span<int16_t> buf, bool fade_in, bool fade_out) noexcept {
    if (buf.empty()) return;
    const size_t ramp = std::min(buf.size(), kTxCosineRampSamples);
    if (fade_in && ramp > 0) {
        for (size_t i = 0; i < ramp; ++i) {
            const double w = 0.5 * (1.0 - std::cos(kPi * static_cast<double>(i) / static_cast<double>(ramp)));
            buf[i] = static_cast<int16_t>(std::lround(static_cast<double>(buf[i]) * w));
        }
    }
    if (fade_out && ramp > 0) {
        const size_t start = buf.size() - ramp;
        for (size_t i = 0; i < ramp; ++i) {
            const double w = 0.5 * (1.0 + std::cos(kPi * static_cast<double>(i + 1) / static_cast<double>(ramp)));
            buf[start + i] = static_cast<int16_t>(std::lround(static_cast<double>(buf[start + i]) * w));
        }
    }
}

size_t AudioRouter::queue_tx_pcm48(std::span<const int16_t> samples_48k,
                                   bool apply_fade_in,
                                   bool apply_fade_out) noexcept {
    if (samples_48k.empty()) return 0;
    if (apply_fade_in) {
        // Starting a fresh TX burst clears any prior abort flag
        tx_abort_requested_.store(false, std::memory_order_release);
    }

    if (!apply_fade_in && !apply_fade_out) {
        return tx_pcm48_ring_.push_span(samples_48k);
    }

    size_t total_pushed = 0;
    size_t offset       = 0;
    while (offset < samples_48k.size()) {
        const size_t chunk = std::min(samples_48k.size() - offset, tx_taper_scratch_.size());
        std::span<int16_t> scratch{tx_taper_scratch_.data(), chunk};
        std::copy_n(samples_48k.data() + static_cast<std::ptrdiff_t>(offset), chunk, scratch.data());

        const bool first_chunk = (offset == 0) && apply_fade_in;
        const bool last_chunk  = (offset + chunk == samples_48k.size()) && apply_fade_out;
        apply_cosine_taper_48k(scratch, first_chunk, last_chunk);

        const size_t pushed = tx_pcm48_ring_.push_span(scratch);
        total_pushed += pushed;
        if (pushed < chunk) break;
        offset += chunk;
    }
    return total_pushed;
}

size_t AudioRouter::pop_tx_pcm48(std::span<int16_t> dst_48k) noexcept {
    if (dst_48k.empty()) return 0;
    const bool aborting = tx_abort_requested_.load(std::memory_order_acquire);
    if (aborting) {
        const size_t ramp_req = std::min(dst_48k.size(), kTxCosineRampSamples);
        const size_t got      = tx_pcm48_ring_.pop_span(dst_48k.subspan(0, ramp_req));
        if (got > 0) {
            apply_cosine_taper_48k(dst_48k.subspan(0, got), false, true);
        }
        tx_pcm48_ring_.reset_consumer();
        tx_abort_requested_.store(false, std::memory_order_release);
        return got;
    }
    return tx_pcm48_ring_.pop_span(dst_48k);
}

size_t AudioRouter::pop_tx_for_primary_12k(std::span<float> dst_12k) noexcept {
    if (dst_12k.empty()) return 0;
    if (active_source_.load(std::memory_order_acquire) != AudioSource::PrimaryG90) {
        return 0;
    }

    size_t out_written = 0;
    while (out_written < dst_12k.size()) {
        const size_t max_12k_step = std::min(dst_12k.size() - out_written, tx_decim_scratch_.size() / 4);
        const size_t req_48k      = max_12k_step * 4;
        if (req_48k == 0) break;

        const size_t got_48k = pop_tx_pcm48(std::span<int16_t>{tx_decim_scratch_.data(), req_48k});
        const size_t out_n   = got_48k / 4;
        if (out_n == 0) break;

        // 4:1 anti-aliasing boxcar decimation (attenuates > 6 kHz images cleanly;SSTV tones live strictly in 1100..2300 Hz)
        constexpr float kScale = 0.25f / 32768.0f;
        for (size_t i = 0; i < out_n; ++i) {
            const size_t base = i * 4;
            const int32_t sum = static_cast<int32_t>(tx_decim_scratch_[base + 0]) +
                                static_cast<int32_t>(tx_decim_scratch_[base + 1]) +
                                static_cast<int32_t>(tx_decim_scratch_[base + 2]) +
                                static_cast<int32_t>(tx_decim_scratch_[base + 3]);
            dst_12k[out_written + i] = static_cast<float>(sum) * kScale;
        }
        out_written += out_n;
        if (got_48k < req_48k) break;
    }
    return out_written;
}

void AudioRouter::abort_tx() noexcept {
    if (tx_pcm48_ring_.size_approx() > 0) {
        tx_abort_requested_.store(true, std::memory_order_release);
    } else {
        tx_abort_requested_.store(false, std::memory_order_release);
    }
}

AudioRouterTelemetry AudioRouter::telemetry() const noexcept {
    AudioRouterTelemetry t{};
    t.active_source        = active_source_.load(std::memory_order_acquire);
    t.route_epoch          = route_epoch_.load(std::memory_order_acquire);
    t.source_a_rms_dbfs    = rms_a_dbfs_.load(std::memory_order_relaxed);
    t.source_b_rms_dbfs    = rms_b_dbfs_.load(std::memory_order_relaxed);
    t.active_rms_dbfs      = (t.active_source == AudioSource::PrimaryG90) ? t.source_a_rms_dbfs : t.source_b_rms_dbfs;
    t.dongle_stream_active = dongle_stream_open_.load(std::memory_order_acquire);
    t.tx_streaming_active  = is_tx_active();
    t.dongle_device_index  = selected_dongle_idx_.load(std::memory_order_acquire);
    t.routed_samples       = broadcast_ring_.published_samples();
    t.tx_queued_samples    = tx_pcm48_ring_.size_approx();
    return t;
}

} // namespace hamshack::carbon
