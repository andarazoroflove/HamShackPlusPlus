#pragma once
// ============================================================================
// hamshack_config.hpp — Persistent Application Settings for HamShack++
// Saves and restores station callsign, grid, last active tabs, bookmarked web
// pages, IRC credentials, SASL auth, and NickServ passwords across reboots.
// ============================================================================

#include <string>
#include <vector>
#include <array>
#include <algorithm>
#include <unordered_map>
#include <fstream>
#include <sstream>
#include <cstdlib>
#include <filesystem>
#include "hamshack_platform.hpp"

namespace hamshack {

struct AppConfig {
    std::string hf_callsign = "W1AW";
    std::string hf_grid = "FN31";
    int last_main_tab = 0;           // 0=FT8, 1=JS8, 2=CW, 3=MeshCore, 4=GridTracker, 5=Pages, 6=IRC, 7=Winlink, 8=Config
    int last_meshcore_tab = 1;       // MeshCore channel tab index (default 1 = Public)
    std::string last_meshcore_chan = "public";

    // Waterfall color scheme: 0=Aqua/Emerald, 1=Amber Phosphor, 2=Cyber Neon, 3=Monokai Matrix, 4=Solarized Sunset
    int wf_color_scheme = 0;
    bool wf_show_signal_grid = true;

    // 5 Bookmarked URLs for Pages tab (QRZ, PSKReporter, DXView, DXLook, OpenHamClock)
    std::array<std::string, 5> bookmarked_urls{
        "https://www.qrz.com/db/",
        "https://pskreporter.info/",
        "https://hf.dxview.org/",
        "https://dxlook.com/",
        "https://openhamclock.com/"
    };

    // 5 Zoom levels for Pages tab WKWebViews (1.0 = 100%)
    std::array<double, 5> page_zoom_levels{1.0, 1.0, 1.0, 1.0, 1.0};

    // IRC Configuration
    std::string irc_server = "irc.libera.chat";
    int irc_port = 6667;
    std::string irc_nick = "W1AW";
    std::string irc_user = "hamoperator";
    std::string irc_realname = "HamShack++ Operator";
    std::string irc_default_channel = "#hamradio";
    bool irc_use_sasl = false;
    std::string irc_sasl_user = "";
    std::string irc_sasl_pass = "";
    float irc_font_size = 13.0f;
    std::unordered_map<std::string, std::string> nickserv_passwords{}; // nick -> password

    // Winlink / Email Configuration
    std::string winlink_callsign = "W1AW";
    std::string winlink_password = "";
    std::string winlink_grid = "FN31";
    int winlink_transport = 0; // 0=Telnet CMS, 1=ARDOP HF, 2=Packet AX.25
    std::string winlink_cms_server = "cms.winlink.org";
    int winlink_cms_port = 8772;
    std::string winlink_cms_call = "WL2K";
    std::string winlink_cms_passcode = "CMSTELNET";
    int winlink_cms_timeout_sec = 30;

    void reset_winlink_cms_defaults() {
        winlink_cms_server = "cms.winlink.org";
        winlink_cms_port = 8772;
        winlink_cms_call = "WL2K";
        winlink_cms_passcode = "CMSTELNET";
        winlink_cms_timeout_sec = 30;
    }
    std::string winlink_ardop_host = "127.0.0.1";
    int winlink_ardop_port = 8515;
    int winlink_ardop_bandwidth = 1000; // 500, 1000, 2000
    int winlink_ardop_drive = 75;
    std::string winlink_selected_gateway = "";
    uint64_t winlink_selected_freq = 0;
    std::string winlink_selected_mode = "ARDOP 1000";
    float winlink_font_size = 13.0f;
    float winlink_sidebar_w = 380.0f;
    float winlink_log_extra_h = 0.0f;

    // Web Reporting (PSK Reporter IPFIX Push)
    bool web_reporting_enabled = true;

    // Twitch Live Streaming Studio
    std::string twitch_channel = "";
    std::string twitch_stream_key = "";
    std::string twitch_rtmp_url = "rtmp://live.twitch.tv/app/";

    // Window geometry persistence
    double window_width = 1520.0;
    double window_height = 920.0;

    // Wide Graph geometry and position persistence (0 = Top, 1 = Bottom)
    float wide_graph_height = 268.0f;
    int wide_graph_pos = 0;

    // APRS (AX.25 1200-baud KISS-over-TCP / Dire Wolf) & SSTV Transceiver Settings
    std::string aprs_kiss_host = "127.0.0.1";
    int aprs_kiss_port = 8001;
    int aprs_ssid = 9;
    std::string aprs_digi_path = "WIDE1-1,WIDE2-1";
    std::string aprs_beacon_comment = "HamShack++ APRS";
    std::string sstv_overlay_text = "W1AW  FN31";
    bool sstv_auto_save = true;

    static std::string get_config_dir() {
        std::string dir;
#ifdef _WIN32
        if (const char* appdata = std::getenv("APPDATA"); appdata && *appdata) {
            dir = std::string(appdata) + "\\HamShack++";
        } else if (const char* userprof = std::getenv("USERPROFILE"); userprof && *userprof) {
            dir = std::string(userprof) + "\\AppData\\Roaming\\HamShack++";
        } else {
            dir = ".\\HamShack++_Config";
        }
#else
        if (const char* home = std::getenv("HOME")) {
            dir = std::string(home) + "/Library/Application Support/HamShack++";
        } else {
            dir = "/tmp/HamShack++";
        }
#endif
        hs_mkdir_p(dir);
        return dir;
    }

    static std::string get_config_path() {
#ifdef _WIN32
        return get_config_dir() + "\\hamshack_config.txt";
#else
        return get_config_dir() + "/hamshack_config.txt";
#endif
    }

    void save() const {
        std::string path = get_config_path();
        std::ofstream f(path);
        if (!f) return;

        f << "callsign=" << hf_callsign << "\n";
        f << "grid=" << hf_grid << "\n";
        f << "main_tab=" << last_main_tab << "\n";
        f << "meshcore_tab=" << last_meshcore_tab << "\n";
        f << "meshcore_chan=" << last_meshcore_chan << "\n";
        f << "wf_color_scheme=" << wf_color_scheme << "\n";
        f << "wf_show_signal_grid=" << (wf_show_signal_grid ? 1 : 0) << "\n";

        for (size_t i = 0; i < bookmarked_urls.size(); ++i) {
            f << "bookmark_" << i << "=" << bookmarked_urls[i] << "\n";
        }
        for (size_t i = 0; i < page_zoom_levels.size(); ++i) {
            f << "zoom_" << i << "=" << page_zoom_levels[i] << "\n";
        }

        f << "irc_server=" << irc_server << "\n";
        f << "irc_port=" << irc_port << "\n";
        f << "irc_nick=" << irc_nick << "\n";
        f << "irc_user=" << irc_user << "\n";
        f << "irc_realname=" << irc_realname << "\n";
        f << "irc_channel=" << irc_default_channel << "\n";
        f << "irc_use_sasl=" << (irc_use_sasl ? 1 : 0) << "\n";
        f << "irc_sasl_user=" << irc_sasl_user << "\n";
        f << "irc_sasl_pass=" << irc_sasl_pass << "\n";
        f << "irc_font_size=" << irc_font_size << "\n";

        for (const auto& [n, p] : nickserv_passwords) {
            if (!n.empty() && !p.empty()) {
                f << "nickserv_pass:" << n << "=" << p << "\n";
            }
        }

        f << "winlink_callsign=" << winlink_callsign << "\n";
        f << "winlink_password=" << winlink_password << "\n";
        f << "winlink_grid=" << winlink_grid << "\n";
        f << "winlink_transport=" << winlink_transport << "\n";
        f << "winlink_cms_server=" << winlink_cms_server << "\n";
        f << "winlink_cms_port=" << winlink_cms_port << "\n";
        f << "winlink_cms_call=" << winlink_cms_call << "\n";
        f << "winlink_cms_passcode=" << winlink_cms_passcode << "\n";
        f << "winlink_cms_timeout_sec=" << winlink_cms_timeout_sec << "\n";
        f << "winlink_ardop_host=" << winlink_ardop_host << "\n";
        f << "winlink_ardop_port=" << winlink_ardop_port << "\n";
        f << "winlink_ardop_bandwidth=" << winlink_ardop_bandwidth << "\n";
        f << "winlink_ardop_drive=" << winlink_ardop_drive << "\n";
        f << "winlink_selected_gateway=" << winlink_selected_gateway << "\n";
        f << "winlink_selected_freq=" << winlink_selected_freq << "\n";
        f << "winlink_selected_mode=" << winlink_selected_mode << "\n";
        f << "winlink_font_size=" << winlink_font_size << "\n";
        f << "winlink_sidebar_w=" << winlink_sidebar_w << "\n";
        f << "winlink_log_extra_h=" << winlink_log_extra_h << "\n";
        f << "web_reporting=" << (web_reporting_enabled ? 1 : 0) << "\n";
        f << "window_width=" << window_width << "\n";
        f << "window_height=" << window_height << "\n";
        f << "wide_graph_height=" << wide_graph_height << "\n";
        f << "wide_graph_pos=" << wide_graph_pos << "\n";
        f << "twitch_channel=" << twitch_channel << "\n";
        f << "twitch_stream_key=" << twitch_stream_key << "\n";
        f << "twitch_rtmp_url=" << twitch_rtmp_url << "\n";
        f << "aprs_kiss_host=" << aprs_kiss_host << "\n";
        f << "aprs_kiss_port=" << aprs_kiss_port << "\n";
        f << "aprs_ssid=" << aprs_ssid << "\n";
        f << "aprs_digi_path=" << aprs_digi_path << "\n";
        f << "aprs_beacon_comment=" << aprs_beacon_comment << "\n";
        f << "sstv_overlay_text=" << sstv_overlay_text << "\n";
        f << "sstv_auto_save=" << (sstv_auto_save ? 1 : 0) << "\n";
    }

    void load() {
        std::string path = get_config_path();
        std::ifstream f(path);
        if (!f) return;

        std::string line;
        while (std::getline(f, line)) {
            while (!line.empty() && (line.back() == '\r' || line.back() == '\n' || line.back() == ' ')) {
                line.pop_back();
            }
            if (line.empty() || line[0] == '#') continue;

            size_t eq = line.find('=');
            if (eq == std::string::npos) continue;
            std::string key = line.substr(0, eq);
            std::string val = line.substr(eq + 1);

            if (key == "callsign" && !val.empty()) {
                hf_callsign = val;
                winlink_callsign = val;
            } else if (key == "grid" && !val.empty()) {
                hf_grid = val;
                winlink_grid = val;
            } else if (key == "main_tab") last_main_tab = std::atoi(val.c_str());
            else if (key == "meshcore_tab") last_meshcore_tab = std::atoi(val.c_str());
            else if (key == "meshcore_chan" && !val.empty()) last_meshcore_chan = val;
            else if (key == "wf_color_scheme") wf_color_scheme = std::clamp(std::atoi(val.c_str()), 0, 8);
            else if (key == "wf_show_signal_grid") wf_show_signal_grid = (std::atoi(val.c_str()) != 0);
            else if (key == "twitch_channel" && !val.empty()) {
                if (val != "HamShackRadio" && val != "HamShackTV") {
                    twitch_channel = val;
                }
            }
            else if (key == "twitch_stream_key") twitch_stream_key = val;
            else if (key == "twitch_rtmp_url" && !val.empty()) twitch_rtmp_url = val;
            else if (key.rfind("bookmark_", 0) == 0) {
                int b_idx = std::atoi(key.substr(9).c_str());
                if (b_idx >= 0 && b_idx < 5 && !val.empty()) {
                    bookmarked_urls[b_idx] = val;
                }
            } else if (key.rfind("zoom_", 0) == 0) {
                int z_idx = std::atoi(key.substr(5).c_str());
                if (z_idx >= 0 && z_idx < 5) {
                    double z = std::atof(val.c_str());
                    if (z >= 0.3 && z <= 3.5) page_zoom_levels[z_idx] = z;
                }
            } else if (key == "irc_server" && !val.empty()) irc_server = val;
            else if (key == "irc_port") irc_port = std::atoi(val.c_str());
            else if (key == "irc_nick" && !val.empty()) irc_nick = val;
            else if (key == "irc_user" && !val.empty()) irc_user = val;
            else if (key == "irc_realname" && !val.empty()) irc_realname = val;
            else if (key == "irc_channel" && !val.empty()) {
                irc_default_channel = val;
            }
            else if (key == "irc_use_sasl") irc_use_sasl = (val == "1");
            else if (key == "irc_sasl_user") irc_sasl_user = val;
            else if (key == "irc_sasl_pass") irc_sasl_pass = val;
            else if (key == "irc_font_size") {
                float sz = std::atof(val.c_str());
                if (sz >= 8.0f && sz <= 32.0f) irc_font_size = sz;
            }
            else if (key == "winlink_callsign" && !val.empty()) winlink_callsign = val;
            else if (key == "winlink_password") winlink_password = val;
            else if (key == "winlink_grid" && !val.empty()) {
                if (hf_grid == "FN31" && val != "FN31") hf_grid = val;
                winlink_grid = hf_grid;
            }
            else if (key == "winlink_transport") winlink_transport = std::atoi(val.c_str());
            else if (key == "winlink_cms_server" && !val.empty()) winlink_cms_server = val;
            else if (key == "winlink_cms_port") {
                int p = std::atoi(val.c_str());
                if (p >= 1 && p <= 65535) winlink_cms_port = p;
            }
            else if (key == "winlink_cms_call" && !val.empty()) winlink_cms_call = val;
            else if (key == "winlink_cms_passcode" && !val.empty()) winlink_cms_passcode = val;
            else if (key == "winlink_cms_timeout_sec") {
                int t = std::atoi(val.c_str());
                if (t >= 5 && t <= 300) winlink_cms_timeout_sec = t;
            }
            else if (key == "winlink_ardop_host" && !val.empty()) winlink_ardop_host = val;
            else if (key == "winlink_ardop_port") winlink_ardop_port = std::atoi(val.c_str());
            else if (key == "winlink_ardop_bandwidth") winlink_ardop_bandwidth = std::atoi(val.c_str());
            else if (key == "winlink_ardop_drive") winlink_ardop_drive = std::atoi(val.c_str());
            else if (key == "winlink_selected_gateway") winlink_selected_gateway = val;
            else if (key == "winlink_selected_freq") winlink_selected_freq = std::strtoull(val.c_str(), nullptr, 10);
            else if (key == "winlink_selected_mode") winlink_selected_mode = val;
            else if (key == "winlink_font_size") {
                float sz = static_cast<float>(std::atof(val.c_str()));
                if (sz >= 9.0f && sz <= 26.0f) winlink_font_size = sz;
            }
            else if (key == "winlink_sidebar_w") {
                float sw = static_cast<float>(std::atof(val.c_str()));
                if (sw >= 260.0f && sw <= 760.0f) winlink_sidebar_w = sw;
            }
            else if (key == "winlink_log_extra_h") {
                float lh = static_cast<float>(std::atof(val.c_str()));
                if (lh >= 0.0f && lh <= 400.0f) winlink_log_extra_h = lh;
            }
            else if (key == "web_reporting") web_reporting_enabled = (val == "1");
            else if (key == "window_width") {
                double w = std::atof(val.c_str());
                if (w >= 600.0 && w <= 4000.0) window_width = w;
            }
            else if (key == "window_height") {
                double h = std::atof(val.c_str());
                if (h >= 400.0 && h <= 3000.0) window_height = h;
            }
            else if (key == "wide_graph_height") {
                float gh = std::atof(val.c_str());
                if (gh >= 110.0f && gh <= 600.0f) wide_graph_height = gh;
            }
            else if (key == "wide_graph_pos") {
                wide_graph_pos = std::clamp(std::atoi(val.c_str()), 0, 1);
            }
            else if (key == "aprs_kiss_host" && !val.empty()) aprs_kiss_host = val;
            else if (key == "aprs_kiss_port") {
                int p = std::atoi(val.c_str());
                if (p >= 1 && p <= 65535) aprs_kiss_port = p;
            }
            else if (key == "aprs_ssid") aprs_ssid = std::clamp(std::atoi(val.c_str()), 0, 15);
            else if (key == "aprs_digi_path" && !val.empty()) aprs_digi_path = val;
            else if (key == "aprs_beacon_comment") aprs_beacon_comment = val;
            else if (key == "sstv_overlay_text") sstv_overlay_text = val;
            else if (key == "sstv_auto_save") sstv_auto_save = (std::atoi(val.c_str()) != 0);
            else if (key.rfind("nickserv_pass:", 0) == 0) {
                std::string n = key.substr(14);
                if (!n.empty() && !val.empty()) {
                    nickserv_passwords[n] = val;
                }
            }
        }
        if (!hf_grid.empty()) winlink_grid = hf_grid;
    }
};

inline AppConfig& get_app_config() {
    static AppConfig cfg;
    static bool loaded = false;
    if (!loaded) {
        cfg.load();
        loaded = true;
    }
    return cfg;
}

} // namespace hamshack
