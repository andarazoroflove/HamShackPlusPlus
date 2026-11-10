#pragma once

#include "hamshack_platform.hpp"
#include "hamshack_config.hpp"
#include <string>
#include <vector>
#include <mutex>
#include <chrono>
#include <atomic>
#include <thread>
#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <filesystem>

#ifdef _WIN32
#include <shellapi.h>
#else
#include <signal.h>
#endif

namespace hamshack {

inline std::string extract_json_string_field(const std::string& json, const std::string& key) {
    const std::string pattern = "\"" + key + "\"";
    size_t pos = json.find(pattern);
    if (pos == std::string::npos) return "";
    pos = json.find(':', pos + pattern.size());
    if (pos == std::string::npos) return "";
    pos = json.find('"', pos + 1);
    if (pos == std::string::npos) return "";
    const size_t end_pos = json.find('"', pos + 1);
    if (end_pos == std::string::npos) return "";
    return json.substr(pos + 1, end_pos - pos - 1);
}

inline std::string run_shell_command(const std::string& cmd) {
    std::string out;
#ifdef _WIN32
    SECURITY_ATTRIBUTES sa{};
    sa.nLength = sizeof(sa);
    sa.bInheritHandle = TRUE;
    HANDLE hRead = nullptr, hWrite = nullptr;
    if (!CreatePipe(&hRead, &hWrite, &sa, 0)) return out;
    SetHandleInformation(hRead, HANDLE_FLAG_INHERIT, 0);

    STARTUPINFOA si{};
    si.cb = sizeof(si);
    si.dwFlags = STARTF_USESTDHANDLES | STARTF_USESHOWWINDOW;
    si.wShowWindow = SW_HIDE;
    si.hStdOutput = hWrite;
    si.hStdError  = hWrite;
    si.hStdInput  = GetStdHandle(STD_INPUT_HANDLE);

    PROCESS_INFORMATION pi{};
    std::string mutable_cmd = cmd;
    BOOL ok = CreateProcessA(nullptr, mutable_cmd.data(), nullptr, nullptr, TRUE,
                             CREATE_NO_WINDOW, nullptr, nullptr, &si, &pi);
    CloseHandle(hWrite);
    if (!ok) {
        CloseHandle(hRead);
        return out;
    }

    char buf[1024];
    DWORD got = 0;
    while (ReadFile(hRead, buf, sizeof(buf), &got, nullptr) && got > 0) {
        out.append(buf, got);
    }
    CloseHandle(hRead);
    WaitForSingleObject(pi.hProcess, 8000);
    CloseHandle(pi.hThread);
    CloseHandle(pi.hProcess);
#else
    FILE* pipe = ::popen(cmd.c_str(), "r");
    if (!pipe) return out;
    char buf[1024];
    while (std::fgets(buf, sizeof(buf), pipe) != nullptr) {
        out += buf;
    }
    ::pclose(pipe);
#endif
    return out;
}

class TwitchStreamer {
public:
    std::string channel_name = "";
    std::string stream_key = ""; // live_...
    std::string rtmp_url = "rtmp://live.twitch.tv/app/";
    int target_fps = 30;
    int bitrate_kbps = 3500;

    std::atomic<bool> is_streaming{false};
    std::string status_text = "Twitch Offline (Click to Connect)";
    std::chrono::steady_clock::time_point stream_start_time;
    uint64_t frames_streamed = 0;
    uint64_t bytes_sent = 0;

    // Window frame capture buffer (RGBA, direct from window backbuffer - zero camera access!)
    mutable std::mutex frame_mtx;
    std::vector<uint8_t> latest_frame_rgba;
    int latest_w = 0;
    int latest_h = 0;
    uint64_t frame_seq = 0;

    // Background streaming thread
    std::thread stream_thread;
    std::atomic<bool> stream_abort{false};
#ifdef _WIN32
    mutable std::mutex proc_mtx_;
    HANDLE ffmpeg_proc_ = nullptr;
#endif

    // Automated Twitch Device OAuth authentication state
    std::atomic<bool> auth_in_progress{false};
    std::atomic<bool> auth_abort{false};
    std::string auth_user_code;
    std::thread auth_thread;

    mutable std::mutex mtx;

    static TwitchStreamer& instance() {
        static TwitchStreamer s_inst;
        return s_inst;
    }

    ~TwitchStreamer() {
        stop_stream();
        cancel_browser_login();
    }

    static std::string locate_ffmpeg() {
#ifdef _WIN32
        char exe_path[MAX_PATH] = {0};
        if (GetModuleFileNameA(nullptr, exe_path, MAX_PATH) > 0) {
            std::filesystem::path ep(exe_path);
            auto local_ff = ep.parent_path() / "ffmpeg.exe";
            std::error_code ec;
            if (std::filesystem::exists(local_ff, ec)) {
                return local_ff.string();
            }
        }
        {
            std::error_code ec;
            if (std::filesystem::exists("ffmpeg.exe", ec)) {
                return std::filesystem::absolute("ffmpeg.exe", ec).string();
            }
        }
        char found_path[MAX_PATH] = {0};
        if (SearchPathA(nullptr, "ffmpeg.exe", nullptr, MAX_PATH, found_path, nullptr) > 0) {
            return std::string(found_path);
        }
        if (const char* local_app = std::getenv("LOCALAPPDATA"); local_app && *local_app) {
            std::filesystem::path links = std::filesystem::path(local_app) / "Microsoft" / "WinGet" / "Links" / "ffmpeg.exe";
            std::error_code ec;
            if (std::filesystem::exists(links, ec)) return links.string();
            std::filesystem::path pkgs = std::filesystem::path(local_app) / "Microsoft" / "WinGet" / "Packages";
            if (std::filesystem::exists(pkgs, ec)) {
                for (const auto& entry : std::filesystem::directory_iterator(pkgs, ec)) {
                    if (entry.path().filename().string().find("FFmpeg") != std::string::npos) {
                        for (const auto& sub : std::filesystem::recursive_directory_iterator(entry.path(), ec)) {
                            if (sub.path().filename() == "ffmpeg.exe") return sub.path().string();
                        }
                    }
                }
            }
        }
        return "";
#else
        for (const char* path : {
            "HamShack++.app/Contents/MacOS/ffmpeg",
            "/Applications/HamShack++.app/Contents/MacOS/ffmpeg",
            "/opt/homebrew/bin/ffmpeg",
            "/usr/local/bin/ffmpeg",
            "/usr/bin/ffmpeg"
        }) {
            if (::access(path, X_OK) == 0) {
                return path;
            }
        }
        return "";
#endif
    }

    std::string build_ffmpeg_cmd(const std::string& ffmpeg_bin, int w = 1280, int h = 720) const {
        std::string dest = rtmp_url + stream_key;
        // Direct Window rawvideo via stdin pipe
        // Silent digital audio via anullsrc (ZERO microphone or soundcard capture)
        // Scaled to standard 720p 16:9 with letterbox/pillarbox
        return "\"" + ffmpeg_bin + "\""
            " -hide_banner -loglevel warning"
            " -f rawvideo -pix_fmt rgba -s " + std::to_string(w) + "x" + std::to_string(h) +
            " -r " + std::to_string(target_fps) + " -i -"
            " -f lavfi -i anullsrc=channel_layout=stereo:sample_rate=44100"
            " -vf \"scale=1280:720:force_original_aspect_ratio=decrease,pad=1280:720:(ow-iw)/2:(oh-ih)/2,format=yuv420p\""
            " -c:v libx264 -preset ultrafast -tune zerolatency"
            " -b:v " + std::to_string(bitrate_kbps) + "k -maxrate " + std::to_string(bitrate_kbps) + "k"
            " -bufsize " + std::to_string(bitrate_kbps * 2) + "k -g " + std::to_string(target_fps * 2) +
            " -c:a aac -b:a 128k -ar 44100"
            " -f flv \"" + dest + "\"";
    }

    void push_frame(const uint8_t* rgba, int w, int h, int bpr) {
        if (!is_streaming.load() || !rgba || w <= 0 || h <= 0) return;
        std::lock_guard<std::mutex> lk(frame_mtx);
        if (latest_w != w || latest_h != h) {
            latest_w = w;
            latest_h = h;
            latest_frame_rgba.resize(static_cast<size_t>(w * h * 4));
        }
        if (bpr == w * 4) {
            std::memcpy(latest_frame_rgba.data(), rgba, static_cast<size_t>(w * h * 4));
        } else {
            for (int y = 0; y < h; ++y) {
                std::memcpy(latest_frame_rgba.data() + (y * w * 4), rgba + (y * bpr), static_cast<size_t>(w * 4));
            }
        }
        frame_seq++;
    }

    void start_stream() {
        std::lock_guard<std::mutex> lk(mtx);
        if (is_streaming) return;

        std::string ffmpeg_bin = locate_ffmpeg();
        if (ffmpeg_bin.empty()) {
#ifdef _WIN32
            status_text = "Error: FFmpeg not found (install via 'winget install Gyan.FFmpeg')";
#else
            status_text = "Error: FFmpeg not installed (run 'brew install ffmpeg')";
#endif
            return;
        }
        if (stream_key.empty()) {
            status_text = "Error: Stream key empty (Connect Twitch first)";
            return;
        }

        is_streaming = true;
        stream_abort = false;
        stream_start_time = std::chrono::steady_clock::now();
        frames_streamed = 0;
        bytes_sent = 0;
        status_text = "Connecting to Twitch RTMP...";

        if (stream_thread.joinable()) {
            stream_thread.join();
        }

        std::string dest = rtmp_url + stream_key;
        int w = latest_w > 0 ? latest_w : 1520;
        int h = latest_h > 0 ? latest_h : 920;

        stream_thread = std::thread([this, dest, ffmpeg_bin, w, h]() {
            stream_worker_loop(dest, ffmpeg_bin, w, h);
        });
    }

    void stream_worker_loop(std::string rtmp_url_dest, std::string ffmpeg_bin, int init_w, int init_h) {
        // Wait briefly for first GUI window frame
        for (int i = 0; i < 60 && !stream_abort.load(); ++i) {
            {
                std::lock_guard<std::mutex> lk(frame_mtx);
                if (!latest_frame_rgba.empty() && latest_w > 0 && latest_h > 0) {
                    init_w = latest_w;
                    init_h = latest_h;
                    break;
                }
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(30));
        }

        if (init_w <= 0 || init_h <= 0) {
            init_w = 1280;
            init_h = 720;
        }

        int stream_fps = target_fps > 0 ? target_fps : 30;
        std::string base_cmd = "\"" + ffmpeg_bin + "\""
            " -hide_banner -loglevel warning"
            " -f rawvideo -pix_fmt rgba -s " + std::to_string(init_w) + "x" + std::to_string(init_h) +
            " -r " + std::to_string(stream_fps) + " -i -"
            " -f lavfi -i anullsrc=channel_layout=stereo:sample_rate=44100"
            " -vf \"scale=1280:720:force_original_aspect_ratio=decrease,pad=1280:720:(ow-iw)/2:(oh-ih)/2,format=yuv420p\""
            " -c:v libx264 -preset ultrafast -tune zerolatency"
            " -b:v " + std::to_string(bitrate_kbps) + "k -maxrate " + std::to_string(bitrate_kbps) + "k"
            " -bufsize " + std::to_string(bitrate_kbps * 2) + "k -g " + std::to_string(stream_fps * 2) +
            " -c:a aac -b:a 128k -ar 44100"
            " -f flv \"" + rtmp_url_dest + "\"";

#ifdef _WIN32
        SECURITY_ATTRIBUTES sa{};
        sa.nLength = sizeof(sa);
        sa.bInheritHandle = TRUE;
        HANDLE hStdinRead = nullptr, hStdinWrite = nullptr;
        if (!CreatePipe(&hStdinRead, &hStdinWrite, &sa, 4 * 1024 * 1024)) {
            std::lock_guard<std::mutex> lk(mtx);
            is_streaming.store(false);
            status_text = "Failed to create FFmpeg video pipe";
            return;
        }
        SetHandleInformation(hStdinWrite, HANDLE_FLAG_INHERIT, 0);

        STARTUPINFOA si{};
        si.cb = sizeof(si);
        si.dwFlags = STARTF_USESTDHANDLES | STARTF_USESHOWWINDOW;
        si.wShowWindow = SW_HIDE;
        si.hStdInput  = hStdinRead;
        si.hStdOutput = GetStdHandle(STD_OUTPUT_HANDLE);
        si.hStdError  = GetStdHandle(STD_ERROR_HANDLE);

        PROCESS_INFORMATION pi{};
        std::string mutable_cmd = base_cmd;
        BOOL launched = CreateProcessA(nullptr, mutable_cmd.data(), nullptr, nullptr, TRUE,
                                       CREATE_NO_WINDOW, nullptr, nullptr, &si, &pi);
        CloseHandle(hStdinRead);
        if (!launched) {
            CloseHandle(hStdinWrite);
            std::lock_guard<std::mutex> lk(mtx);
            is_streaming.store(false);
            status_text = "Failed to launch FFmpeg process";
            return;
        }
        {
            std::lock_guard<std::mutex> plk(proc_mtx_);
            ffmpeg_proc_ = pi.hProcess;
        }
#else
        std::string cmd = base_cmd + " 2>/tmp/hamshack_twitch.log";
        FILE* pipe = ::popen(cmd.c_str(), "w");
        if (!pipe) {
            std::lock_guard<std::mutex> lk(mtx);
            is_streaming.store(false);
            status_text = "Failed to launch FFmpeg stream pipe";
            return;
        }
#endif

        {
            std::lock_guard<std::mutex> lk(mtx);
            status_text = "LIVE on twitch.tv/" + (channel_name.empty() ? "channel" : channel_name);
        }

        std::vector<uint8_t> current_frame(static_cast<size_t>(init_w * init_h * 4), 0);
        const size_t frame_bytes = static_cast<size_t>(init_w * init_h * 4);
        auto next_tick = std::chrono::steady_clock::now();
        const auto tick_interval = std::chrono::microseconds(1000000 / stream_fps);

        while (!stream_abort.load()) {
            {
                std::lock_guard<std::mutex> lk(frame_mtx);
                if (!latest_frame_rgba.empty() && latest_frame_rgba.size() == frame_bytes) {
                    std::memcpy(current_frame.data(), latest_frame_rgba.data(), frame_bytes);
                }
            }

#ifdef _WIN32
            size_t total_written = 0;
            bool write_ok = true;
            while (total_written < frame_bytes && !stream_abort.load()) {
                DWORD chunk = static_cast<DWORD>(std::min<size_t>(frame_bytes - total_written, 256 * 1024));
                DWORD got = 0;
                if (!WriteFile(hStdinWrite, current_frame.data() + total_written, chunk, &got, nullptr) || got == 0) {
                    write_ok = false;
                    break;
                }
                total_written += got;
            }
            if (!write_ok) break;
#else
            size_t written = ::fwrite(current_frame.data(), 1, frame_bytes, pipe);
            if (written != frame_bytes) {
                break;
            }
            ::fflush(pipe);
#endif

            frames_streamed++;
            bytes_sent += (bitrate_kbps * 1000ULL / 8ULL / stream_fps);

            next_tick += tick_interval;
            auto now = std::chrono::steady_clock::now();
            if (next_tick > now) {
                std::this_thread::sleep_for(next_tick - now);
            } else {
                next_tick = now;
            }
        }

#ifdef _WIN32
        CloseHandle(hStdinWrite);
        if (WaitForSingleObject(pi.hProcess, 250) == WAIT_TIMEOUT) {
            TerminateProcess(pi.hProcess, 0);
            WaitForSingleObject(pi.hProcess, 250);
        }
        {
            std::lock_guard<std::mutex> plk(proc_mtx_);
            ffmpeg_proc_ = nullptr;
        }
        CloseHandle(pi.hThread);
        CloseHandle(pi.hProcess);
#else
        ::pclose(pipe);
#endif

        std::lock_guard<std::mutex> lk(mtx);
        is_streaming.store(false);
        if (!stream_abort.load()) {
            status_text = "Stream Disconnected (Check Stream Key)";
        } else {
            status_text = "Twitch Stream Ended";
        }
    }

    void stop_stream() {
        if (!is_streaming) return;
        stream_abort = true;
#ifdef _WIN32
        {
            std::lock_guard<std::mutex> plk(proc_mtx_);
            if (ffmpeg_proc_) {
                TerminateProcess(ffmpeg_proc_, 0);
            }
        }
#else
        ::system("pkill -9 -f 'ffmpeg.*live.twitch.tv' >/dev/null 2>&1");
#endif
        if (stream_thread.joinable() && std::this_thread::get_id() != stream_thread.get_id()) {
            stream_thread.join();
        }
        std::lock_guard<std::mutex> lk(mtx);
        is_streaming = false;
        status_text = "Twitch Stream Ended";
    }

    void stop_streaming() {
        stop_stream();
    }

    void toggle_stream() {
        if (is_streaming) stop_stream();
        else start_stream();
    }

    void tick() {
        // Telemetry updated automatically by streaming thread
    }

    std::string uptime_str() const {
        if (!is_streaming) return "00:00:00";
        auto dur = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::steady_clock::now() - stream_start_time).count();
        int hrs = static_cast<int>(dur / 3600);
        int mins = static_cast<int>((dur % 3600) / 60);
        int secs = static_cast<int>(dur % 60);
        char buf[32];
        std::snprintf(buf, sizeof(buf), "%02d:%02d:%02d", hrs, mins, secs);
        return buf;
    }

    void start_browser_login() {
        if (auth_in_progress.load()) return;
        auth_abort.store(false);
        auth_in_progress.store(true);
        {
            std::lock_guard<std::mutex> lk(mtx);
            auth_user_code.clear();
            status_text = "Opening Twitch Browser Login...";
        }

        if (auth_thread.joinable()) {
            auth_thread.join();
        }

        auth_thread = std::thread([this]() {
            twitch_browser_auth_loop();
        });
    }

    void cancel_browser_login() {
        auth_abort.store(true);
        if (auth_thread.joinable()) {
            auth_thread.join();
        }
        std::lock_guard<std::mutex> lk(mtx);
        auth_in_progress.store(false);
        auth_user_code.clear();
    }

    static void copy_text_to_os_clipboard(const std::string& text) {
#ifdef _WIN32
        if (OpenClipboard(nullptr)) {
            EmptyClipboard();
            HGLOBAL hg = GlobalAlloc(GMEM_MOVEABLE, text.size() + 1);
            if (hg) {
                if (void* dst = GlobalLock(hg)) {
                    std::memcpy(dst, text.c_str(), text.size() + 1);
                    GlobalUnlock(hg);
                    SetClipboardData(CF_TEXT, hg);
                } else {
                    GlobalFree(hg);
                }
            }
            CloseClipboard();
        }
#else
        if (FILE* pb = ::popen("/usr/bin/pbcopy", "w")) {
            std::fwrite(text.data(), 1, text.size(), pb);
            ::pclose(pb);
        }
#endif
    }

    static void open_url_in_os_browser(const std::string& url) {
#ifdef _WIN32
        ShellExecuteA(nullptr, "open", url.c_str(), nullptr, nullptr, SW_SHOWNORMAL);
#else
        ::system(("/usr/bin/open \"" + url + "\" &").c_str());
#endif
    }

    void twitch_browser_auth_loop() {
        constexpr const char* kTwitchClientId = "ue6666qo983tsx6so1t0vnawi233wa";
#ifdef _WIN32
        const std::string curl_bin = "curl.exe";
#else
        const std::string curl_bin = "/usr/bin/curl";
#endif

        // Request device and user verification code from Twitch OAuth
        const std::string deviceResp = run_shell_command(
            curl_bin + " -s -m 8 -X POST -d \"client_id=" + kTwitchClientId +
            "&scopes=channel%3Aread%3Astream_key\" \"https://id.twitch.tv/oauth2/device\""
        );
        const std::string deviceCode = extract_json_string_field(deviceResp, "device_code");
        const std::string userCode   = extract_json_string_field(deviceResp, "user_code");

        if (deviceCode.empty() || userCode.empty()) {
            std::lock_guard<std::mutex> lk(mtx);
            auth_in_progress.store(false);
            status_text = "Failed to contact Twitch OAuth";
            return;
        }

        // Copy activation code to OS clipboard
        copy_text_to_os_clipboard(userCode);

        // Open Twitch device activation URL with pre-filled code in user's default browser
        const std::string activateUrl = "https://www.twitch.tv/activate?public=true&device-code=" + userCode;
        open_url_in_os_browser(activateUrl);

        {
            std::lock_guard<std::mutex> lk(mtx);
            auth_in_progress.store(true);
            auth_user_code = userCode;
            status_text = "Approve in Browser (Code: " + userCode + ")";
        }

        // Poll id.twitch.tv/oauth2/token every 3 seconds until authorized or aborted
        std::string accessToken;
        for (int attempt = 0; attempt < 180 && !auth_abort.load(); ++attempt) {
            for (int w = 0; w < 30 && !auth_abort.load(); ++w) {
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
            }
            if (auth_abort.load()) return;

            const std::string tokenResp = run_shell_command(
                curl_bin + " -s -m 8 -X POST -d \"client_id=" + kTwitchClientId +
                "&device_code=" + deviceCode +
                "&grant_type=urn%3Aietf%3Aparams%3Aoauth%3Agrant-type%3Adevice_code\" \"https://id.twitch.tv/oauth2/token\""
            );
            accessToken = extract_json_string_field(tokenResp, "access_token");
            if (!accessToken.empty()) break;
        }

        if (accessToken.empty() || auth_abort.load()) {
            std::lock_guard<std::mutex> lk(mtx);
            auth_in_progress.store(false);
            auth_user_code.clear();
            if (!auth_abort.load()) {
                status_text = "Twitch browser login timed out";
            }
            return;
        }

        // Step 2: Validate token & get broadcaster user_id and channel login name
        const std::string validateResp = run_shell_command(
            curl_bin + " -s -m 8 -H \"Authorization: OAuth " + accessToken + "\" \"https://id.twitch.tv/oauth2/validate\""
        );
        const std::string userId = extract_json_string_field(validateResp, "user_id");
        const std::string loginName = extract_json_string_field(validateResp, "login");

        // Step 3: Fetch broadcaster Stream Key via Twitch Helix API
        std::string fetchedStreamKey;
        if (!userId.empty()) {
            const std::string keyResp = run_shell_command(
                curl_bin + " -s -m 8 -H \"Client-Id: " + kTwitchClientId +
                "\" -H \"Authorization: Bearer " + accessToken +
                "\" \"https://api.twitch.tv/helix/streams/key?broadcaster_id=" + userId + "\""
            );
            fetchedStreamKey = extract_json_string_field(keyResp, "stream_key");
        }

        // Fallback: Query Twitch GQL currentUser.streamKey.key if Helix returned empty
        if (fetchedStreamKey.empty()) {
            const std::string gqlResp = run_shell_command(
                curl_bin + " -s -m 8 -X POST -H \"Client-Id: " + kTwitchClientId +
                "\" -H \"Authorization: OAuth " + accessToken +
                "\" -H \"Content-Type: application/json\" -d \"{\\\"query\\\":\\\"query{currentUser{streamKey{key}}}\\\"}\" \"https://gql.twitch.tv/gql\""
            );
            fetchedStreamKey = extract_json_string_field(gqlResp, "key");
        }

        {
            std::lock_guard<std::mutex> lk(mtx);
            auth_in_progress.store(false);
            auth_user_code.clear();
            if (!loginName.empty()) {
                channel_name = loginName;
            }
            if (!fetchedStreamKey.empty()) {
                stream_key = fetchedStreamKey;
                status_text = "Logged in as @" + channel_name + " (Stream Key Ready)";
            } else {
                status_text = "Logged in as @" + channel_name + " (2FA required for Stream Key)";
            }
        }

        // Save channel and stream key to AppConfig and persist to disk
        auto& cfg = get_app_config();
        if (!loginName.empty()) cfg.twitch_channel = loginName;
        if (!fetchedStreamKey.empty()) cfg.twitch_stream_key = fetchedStreamKey;
        cfg.save();
    }

    void open_twitch_dashboard() {
        std::string url = channel_name.empty()
            ? "https://dashboard.twitch.tv/settings/stream"
            : "https://dashboard.twitch.tv/u/" + channel_name + "/settings/stream";
        open_url_in_os_browser(url);
    }
};

inline TwitchStreamer& get_twitch_streamer() {
    return TwitchStreamer::instance();
}

} // namespace hamshack
