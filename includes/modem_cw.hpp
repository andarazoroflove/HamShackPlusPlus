#pragma once
// ============================================================================
// Fldigi CW (Morse Code) Adaptive SOM Modem, Keyer & Digiscope Engine
// Ported from fldigi/src/cw_rtty/morse.cxx and fldigi/src/cw_rtty/cw.cxx
// ============================================================================

#include <cstdint>
#include <cstring>
#include <cmath>
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif
#include <string>
#include <vector>
#include <array>
#include <mutex>
#include <algorithm>

namespace cw_fldigi {

struct CWEntry {
    const char* chr;
    const char* prt;
    const char* rpr;
    float weights[7]; // Normalized 7-element SOM weights (0.33 = dit, 1.0 = dah)
};

// Complete Morse + Prosign table from fldigi/src/cw_rtty/morse.cxx & cw.cxx
static constexpr CWEntry CW_SOM_TABLE[] = {
    // Prosigns
    {"=", "<BT>",  "-...-",  {1.00f, 0.33f, 0.33f, 0.33f, 1.00f, 0.0f, 0.0f}},
    {"~", "<AA>",  ".-.-",   {0.33f, 1.00f, 0.33f, 1.00f, 0.00f, 0.0f, 0.0f}},
    {"<", "<AS>",  ".-...",  {0.33f, 1.00f, 0.33f, 0.33f, 0.33f, 0.0f, 0.0f}},
    {">", "<AR>",  ".-.-.",  {0.33f, 1.00f, 0.33f, 1.00f, 0.33f, 0.0f, 0.0f}},
    {"%", "<SK>",  "...-.-", {0.33f, 0.33f, 0.33f, 1.00f, 0.33f, 1.0f, 0.0f}},
    {"+", "<KN>",  "-.--.",  {1.00f, 0.33f, 1.00f, 1.00f, 0.33f, 0.0f, 0.0f}},
    {"&", "<INT>", "..-.-",  {0.33f, 0.33f, 1.00f, 0.33f, 1.00f, 0.0f, 0.0f}},
    {"{", "<HM>",  "....--", {0.33f, 0.33f, 0.33f, 0.33f, 1.00f, 1.0f, 0.0f}},
    {"}", "<VE>",  "...-.",  {0.33f, 0.33f, 0.33f, 1.00f, 0.33f, 0.0f, 0.0f}},
    // Letters A-Z
    {"A", "A", ".-",     {0.33f, 1.00f, 0.00f, 0.00f, 0.00f, 0.00f, 0.00f}},
    {"B", "B", "-...",   {1.00f, 0.33f, 0.33f, 0.33f, 0.00f, 0.00f, 0.00f}},
    {"C", "C", "-.-.",   {1.00f, 0.33f, 1.00f, 0.33f, 0.00f, 0.00f, 0.00f}},
    {"D", "D", "-..",    {1.00f, 0.33f, 0.33f, 0.00f, 0.00f, 0.00f, 0.00f}},
    {"E", "E", ".",      {0.33f, 0.00f, 0.00f, 0.00f, 0.00f, 0.00f, 0.00f}},
    {"F", "F", "..-.",   {0.33f, 0.33f, 1.00f, 0.33f, 0.00f, 0.00f, 0.00f}},
    {"G", "G", "--.",    {1.00f, 1.00f, 0.33f, 0.00f, 0.00f, 0.00f, 0.00f}},
    {"H", "H", "....",   {0.33f, 0.33f, 0.33f, 0.33f, 0.00f, 0.00f, 0.00f}},
    {"I", "I", "..",     {0.33f, 0.33f, 0.00f, 0.00f, 0.00f, 0.00f, 0.00f}},
    {"J", "J", ".---",   {0.33f, 1.00f, 1.00f, 1.00f, 0.00f, 0.00f, 0.00f}},
    {"K", "K", "-.-",    {1.00f, 0.33f, 1.00f, 0.00f, 0.00f, 0.00f, 0.00f}},
    {"L", "L", ".-..",   {0.33f, 1.00f, 0.33f, 0.33f, 0.00f, 0.00f, 0.00f}},
    {"M", "M", "--",     {1.00f, 1.00f, 0.00f, 0.00f, 0.00f, 0.00f, 0.00f}},
    {"N", "N", "-.",     {1.00f, 0.33f, 0.00f, 0.00f, 0.00f, 0.00f, 0.00f}},
    {"O", "O", "---",    {1.00f, 1.00f, 1.00f, 0.00f, 0.00f, 0.00f, 0.00f}},
    {"P", "P", ".--.",   {0.33f, 1.00f, 1.00f, 0.33f, 0.00f, 0.00f, 0.00f}},
    {"Q", "Q", "--.-",   {1.00f, 1.00f, 0.33f, 1.00f, 0.00f, 0.00f, 0.00f}},
    {"R", "R", ".-.",    {0.33f, 1.00f, 0.33f, 0.00f, 0.00f, 0.00f, 0.00f}},
    {"S", "S", "...",    {0.33f, 0.33f, 0.33f, 0.00f, 0.00f, 0.00f, 0.00f}},
    {"T", "T", "-",      {1.00f, 0.00f, 0.00f, 0.00f, 0.00f, 0.00f, 0.00f}},
    {"U", "U", "..-",    {0.33f, 0.33f, 1.00f, 0.00f, 0.00f, 0.00f, 0.00f}},
    {"V", "V", "...-",   {0.33f, 0.33f, 0.33f, 1.00f, 0.00f, 0.00f, 0.00f}},
    {"W", "W", ".--",    {0.33f, 1.00f, 1.00f, 0.00f, 0.00f, 0.00f, 0.00f}},
    {"X", "X", "-..-",   {1.00f, 0.33f, 0.33f, 1.00f, 0.00f, 0.00f, 0.00f}},
    {"Y", "Y", "-.--",   {1.00f, 0.33f, 1.00f, 1.00f, 0.00f, 0.00f, 0.00f}},
    {"Z", "Z", "--..",   {1.00f, 1.00f, 0.33f, 0.33f, 0.00f, 0.00f, 0.00f}},
    // Numerals 0-9
    {"0", "0", "-----",  {1.00f, 1.00f, 1.00f, 1.00f, 1.00f, 0.00f, 0.00f}},
    {"1", "1", ".----",  {0.33f, 1.00f, 1.00f, 1.00f, 1.00f, 0.00f, 0.00f}},
    {"2", "2", "..---",  {0.33f, 0.33f, 1.00f, 1.00f, 1.00f, 0.00f, 0.00f}},
    {"3", "3", "...--",  {0.33f, 0.33f, 0.33f, 1.00f, 1.00f, 0.00f, 0.00f}},
    {"4", "4", "....-",  {0.33f, 0.33f, 0.33f, 0.33f, 1.00f, 0.00f, 0.00f}},
    {"5", "5", ".....",  {0.33f, 0.33f, 0.33f, 0.33f, 0.33f, 0.00f, 0.00f}},
    {"6", "6", "-....",  {1.00f, 0.33f, 0.33f, 0.33f, 0.33f, 0.00f, 0.00f}},
    {"7", "7", "--...",  {1.00f, 1.00f, 0.33f, 0.33f, 0.33f, 0.00f, 0.00f}},
    {"8", "8", "---..",  {1.00f, 1.00f, 1.00f, 0.33f, 0.33f, 0.00f, 0.00f}},
    {"9", "9", "----.",  {1.00f, 1.00f, 1.00f, 1.00f, 0.33f, 0.00f, 0.00f}},
    // Punctuation
    {"/", "/", "-..-.",  {1.00f, 0.33f, 0.33f, 1.00f, 0.33f, 0.00f, 0.00f}},
    {"?", "?", "..--..", {0.33f, 0.33f, 1.00f, 1.00f, 0.33f, 0.33f, 0.00f}},
    {".", ".", ".-.-.-", {0.33f, 1.00f, 0.33f, 1.00f, 0.33f, 1.00f, 0.00f}},
    {",", ",", "--..--", {1.00f, 1.00f, 0.33f, 0.33f, 1.00f, 1.00f, 0.00f}},
    {"-", "-", "-....-", {1.00f, 0.33f, 0.33f, 0.33f, 0.33f, 1.00f, 0.00f}},
    {"@", "@", ".--.-.", {0.33f, 1.00f, 1.00f, 0.33f, 1.00f, 0.33f, 0.00f}},
    {"!", "!", "-.-.--", {1.00f, 0.33f, 1.00f, 0.33f, 1.00f, 1.00f, 0.00f}}
};
static constexpr int CW_SOM_TABLE_SIZE = sizeof(CW_SOM_TABLE) / sizeof(CW_SOM_TABLE[0]);

// Lookup Morse string representation (e.g. ".-") for an ASCII character
inline std::string char_to_morse(char c) {
    if (c >= 'a' && c <= 'z') c = (char)(c - 'a' + 'A');
    for (int i = 0; i < CW_SOM_TABLE_SIZE; ++i) {
        if (CW_SOM_TABLE[i].chr[0] == c) return CW_SOM_TABLE[i].rpr;
    }
    return "";
}

// Lookup character/prosign from exact dot-dash string (e.g. "...-.-" -> "<SK>")
inline std::string morse_to_char_exact(const std::string& rpr, bool show_prosigns = true) {
    for (int i = 0; i < CW_SOM_TABLE_SIZE; ++i) {
        if (rpr == CW_SOM_TABLE[i].rpr) {
            return show_prosigns ? CW_SOM_TABLE[i].prt : CW_SOM_TABLE[i].chr;
        }
    }
    return "*";
}

// Fldigi SOM (Self-Organizing Map) 7-weight Euclidean distance classifier
// Classifies a 7-element array of normalized mark durations (in [0, 1.0])
inline std::string find_winner_som(const float in_buf[7], bool show_prosigns = true) {
    float diff_best = 1e9f;
    int winner = -1;
    for (int n = 0; n < CW_SOM_TABLE_SIZE; ++n) {
        float diff = 0.0f;
        for (int i = 0; i < 7; ++i) {
            float d = in_buf[i] - CW_SOM_TABLE[n].weights[i];
            diff += d * d;
        }
        if (diff < diff_best) {
            diff_best = diff;
            winner = n;
        }
    }
    if (winner >= 0 && diff_best < 0.45f) {
        return show_prosigns ? CW_SOM_TABLE[winner].prt : CW_SOM_TABLE[winner].chr;
    }
    return "";
}

// Convert text string into human-readable Morse representation (e.g. "CQ" -> "-.-. --.-")
inline std::string text_to_morse_display(const std::string& text) {
    std::string out;
    for (char c : text) {
        if (c == ' ') {
            out += " / ";
        } else {
            std::string m = char_to_morse(c);
            if (!m.empty()) {
                if (!out.empty() && out.back() != ' ') out.push_back(' ');
                out += m;
            }
        }
    }
    return out;
}

// Synthesize shaped raised-cosine CW audio samples at 12,000 Hz (4 ms rise/fall edge from fldigi)
inline std::vector<float> synthesize_cw_wave(const std::string& text, int wpm, int pitch_hz, float amp = 0.55f) {
    double fs = 12000.0;
    int clamped_wpm = std::max(5, std::min(50, wpm));
    // Standard PARIS timing: 1 dit = 1.2 / WPM seconds
    int dot_samples = (int)std::round((1.2 / (double)clamped_wpm) * fs);
    int dash_samples = 3 * dot_samples;
    int edge_samples = (int)std::round(0.004 * fs); // 4 ms raised-cosine edge

    std::vector<float> out;
    double phase = 0.0;
    double dphi = 2.0 * M_PI * (double)pitch_hz / fs;

    auto append_mark = [&](int nsamples) {
        for (int i = 0; i < nsamples; ++i) {
            float env = 1.0f;
            if (i < edge_samples) {
                env = 0.5f * (1.0f - std::cos(M_PI * (double)i / edge_samples));
            } else if (i > nsamples - edge_samples) {
                env = 0.5f * (1.0f - std::cos(M_PI * (double)(nsamples - i) / edge_samples));
            }
            out.push_back(amp * env * (float)std::sin(phase));
            phase += dphi;
            if (phase > 2.0 * M_PI) phase -= 2.0 * M_PI;
        }
    };
    auto append_space = [&](int nsamples) {
        out.insert(out.end(), nsamples, 0.0f);
    };

    for (size_t idx = 0; idx < text.size(); ++idx) {
        char c = text[idx];
        if (c == ' ') {
            append_space(4 * dot_samples); // +3 from previous char = 7 dots inter-word
            continue;
        }
        std::string m = char_to_morse(c);
        for (size_t e = 0; e < m.size(); ++e) {
            append_mark(m[e] == '-' ? dash_samples : dot_samples);
            if (e + 1 < m.size()) append_space(dot_samples); // 1 dot intra-character
        }
        append_space(3 * dot_samples); // 3 dots inter-character
    }
    return out;
}

struct CWulMacro {
    const char* key_label;
    std::string title;
    std::string template_text;
};

class CWModemEngine {
public:
    int tx_wpm = 22;
    int rx_wpm_estimated = 22;
    int pitch_hz = 650;
    int filter_bw_hz = 150;
    float squelch_metric = 0.28f;
    float agc_metric = 0.78f;
    bool qsk_enabled = true;
    bool afc_enabled = true;
    bool prosigns_enabled = true;
    bool tx_active = false;

    std::string my_call = "W1AW";
    std::string dx_call = "";
    std::string my_rst  = "599";
    std::string my_qth  = "";
    std::string my_name = "";

    std::string rx_decoded_text;
    std::string tx_buffer_text;
    std::string last_morse_elements;

    // Live audio envelope & timing state
    float live_tone_db = -120.0f;
    bool  live_key_down = false;
    int   cur_mark_ticks = 0;
    int   cur_space_ticks = 0;
    std::vector<int> char_mark_ticks;

    // 240-sample CW Digiscope envelope trace (0..1)
    static constexpr int SCOPE_LEN = 240;
    std::array<float, SCOPE_LEN> scope_envelope{};
    int scope_write_pos = 0;

    // F1 - F12 Fldigi Macro bar
    std::array<CWulMacro, 12> macros = {{
        {"F1",  "CQ",      "CQ CQ DE <MYCALL> <MYCALL> K"},
        {"F2",  "ANS",     "<CALL> DE <MYCALL> GM TNX FER CALL = UR RST <RST> <RST> <KN>"},
        {"F3",  "RST",     "<CALL> DE <MYCALL> UR RST <RST> <RST> <KN>"},
        {"F4",  "73/TU",   "<CALL> DE <MYCALL> FB OM TNX QSO 73 GL <SK>"},
        {"F5",  "CALL",    "<CALL>"},
        {"F6",  "DE MY",   "DE <MYCALL>"},
        {"F7",  "KN",      "= <CALL> DE <MYCALL> +"},
        {"F8",  "AGN?",    "AGN?"},
        {"F9",  "QRZ?",    "QRZ? DE <MYCALL> K"},
        {"F10", "QTH",     "QTH <QTH>"},
        {"F11", "RIG/G90", "RIG XIEGU G90 20W"},
        {"F12", "SK/CL",   "TNX ALL 73 DE <MYCALL> %"}
    }};

    mutable std::mutex mtx;

    CWModemEngine() {
        scope_envelope.fill(0.0f);
    }

    std::string expand_macro(int macro_idx) const {
        if (macro_idx < 0 || macro_idx >= 12) return "";
        std::string s = macros[macro_idx].template_text;
        auto rep = [&](const std::string& tag, const std::string& val) {
            size_t pos = 0;
            while ((pos = s.find(tag, pos)) != std::string::npos) {
                s.replace(pos, tag.size(), val);
                pos += val.size();
            }
        };
        rep("<MYCALL>", my_call);
        rep("<CALL>", dx_call.empty() ? "QRZ" : dx_call);
        rep("<RST>", my_rst);
        rep("<QTH>", my_qth);
        rep("<NAME>", my_name);
        return s;
    }

    void transmit_text(const std::string& text, std::vector<float>* out_audio = nullptr) {
        std::lock_guard<std::mutex> lk(mtx);
        if (text.empty()) return;
        last_morse_elements = text_to_morse_display(text);
        seed_digiscope_pattern(last_morse_elements);
        rx_decoded_text += "\n[TX " + std::to_string(tx_wpm) + " WPM] " + text + "\n";
        tx_active = true;
        if (out_audio) {
            auto w = synthesize_cw_wave(text, tx_wpm, pitch_hz);
            out_audio->insert(out_audio->end(), w.begin(), w.end());
        }
    }

    // Process a 100ms chunk (1,200 samples @ 12 kHz) of live soundcard audio in 20ms (240-sample) Goertzel steps
    void process_rx_audio_chunk(const std::vector<float>& pcm_12k) {
        std::lock_guard<std::mutex> lk(mtx);
        constexpr int STEP = 240; // 20 ms per step at 12,000 Hz
        if ((int)pcm_12k.size() < STEP) return;

        double w = 2.0 * M_PI * (double)pitch_hz / 12000.0;
        double coeff = 2.0 * std::cos(w);

        int dot_ticks = std::max(2, (int)std::round((1.2 / std::max(5, rx_wpm_estimated)) / 0.020));

        for (size_t offset = 0; offset + STEP <= pcm_12k.size(); offset += STEP) {
            double s0 = 0.0, s1 = 0.0, s2 = 0.0;
            for (int i = 0; i < STEP; ++i) {
                s0 = pcm_12k[offset + i] + coeff * s1 - s2;
                s2 = s1;
                s1 = s0;
            }
            double power = s1 * s1 + s2 * s2 - coeff * s1 * s2;
            float mag = (float)std::sqrt(std::max(0.0, power)) / (float)(STEP * 0.5);
            live_tone_db = 20.0f * std::log10(std::max(1e-6f, mag));
            float env = std::clamp((live_tone_db + 75.0f) / 65.0f, 0.0f, 1.0f);

            scope_envelope[scope_write_pos] = env;
            scope_write_pos = (scope_write_pos + 1) % SCOPE_LEN;

            bool kd = (env > squelch_metric);
            if (kd) {
                ++cur_mark_ticks;
                cur_space_ticks = 0;
                live_key_down = true;
            } else {
                if (live_key_down && cur_mark_ticks > 0) {
                    if (char_mark_ticks.size() < 7) {
                        char_mark_ticks.push_back(cur_mark_ticks);
                    }
                    cur_mark_ticks = 0;
                }
                live_key_down = false;
                ++cur_space_ticks;

                // Flush character on inter-character space (>= 2.5 dots)
                if (!char_mark_ticks.empty() && cur_space_ticks >= std::max(3, (int)std::round(dot_ticks * 2.5))) {
                    int max_t = 1;
                    for (int t : char_mark_ticks) max_t = std::max(max_t, t);
                    float som_in[7]{};
                    std::string rpr;
                    int dash_threshold = std::max(2, (int)std::round(dot_ticks * 2.0));
                    for (size_t i = 0; i < char_mark_ticks.size() && i < 7; ++i) {
                        bool is_dash = (char_mark_ticks[i] >= dash_threshold);
                        som_in[i] = is_dash ? 1.0f : 0.33f;
                        rpr.push_back(is_dash ? '-' : '.');
                    }
                    std::string ch = find_winner_som(som_in, prosigns_enabled);
                    if (!ch.empty()) {
                        rx_decoded_text += ch;
                        if (rx_decoded_text.size() > 1200) rx_decoded_text.erase(0, 200);
                        last_morse_elements = rpr;
                    }
                    char_mark_ticks.clear();
                }
                // Insert word space on inter-word gap (>= 6 dots)
                if (cur_space_ticks == std::max(7, dot_ticks * 6) && !rx_decoded_text.empty() && rx_decoded_text.back() != ' ') {
                    rx_decoded_text.push_back(' ');
                }
            }
        }
    }

    void simulate_incoming_cw_phrase() {
        // Removed fake simulation: only live soundcard CW audio is decoded
    }

    void seed_digiscope_pattern(const std::string& morse_pat) {
        int pos = 0;
        for (char c : morse_pat) {
            int n_on = 0, n_off = 3;
            if (c == '.') { n_on = 4; n_off = 4; }
            else if (c == '-') { n_on = 12; n_off = 4; }
            else if (c == ' ') { n_on = 0; n_off = 6; }
            for (int i = 0; i < n_on && pos < SCOPE_LEN; ++i) {
                scope_envelope[pos++] = (i == 0 || i == n_on - 1) ? 0.55f : 0.92f;
            }
            for (int i = 0; i < n_off && pos < SCOPE_LEN; ++i) {
                scope_envelope[pos++] = 0.06f + 0.03f * (float)std::sin(pos * 0.7);
            }
        }
        while (pos < SCOPE_LEN) {
            scope_envelope[pos] = 0.06f + 0.02f * (float)std::cos(pos * 0.5);
            pos++;
        }
        scope_write_pos = 0;
    }
};

} // namespace cw_fldigi
