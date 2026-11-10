#pragma once
// ============================================================================
// irc_client.hpp — Multi-Channel IRC Client with SASL, Mode & NickServ Automation
// Supports RFC 1459/2812, IRCv3 SASL PLAIN auth, automatic NickServ ident,
// channel operators (+o), voice (+v), WHOIS formatting, and slash commands.
// ============================================================================

#include <string>
#include <vector>
#include <deque>
#include <map>
#include <set>
#include <mutex>
#include <thread>
#include <atomic>
#include <chrono>
#include <sstream>
#include <iomanip>
#include <cstring>
#include "hamshack_platform.hpp"
#include "hamshack_config.hpp"

namespace hamshack {

struct IrcMessage {
    std::string timestamp; // "HH:MM:SS"
    std::string sender;    // Nick or "***" for server notice
    std::string text;
    uint32_t color = 0xE2E8F0; // Hex RGB
    bool is_notice = false;
    bool is_action = false;
};

struct IrcChannelBuffer {
    std::string name;                  // e.g. "Status" or "#hamradio" or "UserNick"
    std::string topic;
    std::deque<IrcMessage> messages;
    std::map<std::string, char> users; // Nick -> Mode Prefix ('@' Op, '+' Voice, '~' Owner, '&' Admin, '%' Halfop, 0 Normal)
    int unread_count = 0;
};

// Base64 helper for IRCv3 SASL PLAIN
inline std::string base64_encode(const std::string& in) {
    static constexpr char tbl[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    std::string out;
    int val = 0, valb = -6;
    for (unsigned char c : in) {
        val = (val << 8) + c;
        valb += 8;
        while (valb >= 0) {
            out.push_back(tbl[(val >> valb) & 0x3F]);
            valb -= 6;
        }
    }
    if (valb > -6) out.push_back(tbl[((val << 8) >> (valb + 8)) & 0x3F]);
    while (out.size() % 4) out.push_back('=');
    return out;
}

class IrcClientEngine {
public:
    mutable std::recursive_mutex mtx;

    std::string server = "irc.libera.chat";
    int port = 6667;
    std::string current_nick = "W1AW";
    std::string username = "hamoperator";
    std::string realname = "HamShack++ Operator";

    std::atomic<bool> connected{false};
    std::atomic<bool> connecting{false};
    std::atomic<bool> running{false};
    std::string status_text = "Disconnected. Click 'Connect' to join Libera.Chat";

    std::vector<IrcChannelBuffer> channels;
    int active_chan_idx = 0;

    std::thread worker_thread;
    hs_socket_t sock_fd = HS_INVALID_SOCKET;

    IrcClientEngine() {
        IrcChannelBuffer status_buf;
        status_buf.name = "Status";
        status_buf.topic = "Libera.Chat Network Status & Notices";
        channels.push_back(status_buf);
    }

    ~IrcClientEngine() {
        disconnect();
    }

    std::string active_channel_name() const {
        std::lock_guard<std::recursive_mutex> lk(mtx);
        if (active_chan_idx >= 0 && active_chan_idx < static_cast<int>(channels.size())) {
            return channels[active_chan_idx].name;
        }
        return "Status";
    }

    void append_message(const std::string& chan_target, const std::string& sender,
                        const std::string& text, uint32_t color = 0xE2E8F0,
                        bool is_notice = false, bool is_action = false) {
        std::lock_guard<std::recursive_mutex> lk(mtx);
        std::time_t t = std::time(nullptr);
        std::tm tm_buf{};
        hs_localtime(&t, &tm_buf);
        char t_buf[16];
        std::snprintf(t_buf, sizeof(t_buf), "%02d:%02d:%02d", tm_buf.tm_hour, tm_buf.tm_min, tm_buf.tm_sec);

        IrcMessage msg{t_buf, sender, text, color, is_notice, is_action};

        int idx = find_or_create_channel(chan_target);
        if (idx >= 0 && idx < static_cast<int>(channels.size())) {
            channels[idx].messages.push_back(msg);
            if (channels[idx].messages.size() > 1000) channels[idx].messages.pop_front();
            if (idx != active_chan_idx) {
                channels[idx].unread_count++;
            }
        }
    }

    int find_or_create_channel(const std::string& target) {
        std::string t = target.empty() ? "Status" : target;
        for (size_t i = 0; i < channels.size(); ++i) {
            if (hs_strcasecmp(channels[i].name.c_str(), t.c_str()) == 0) return static_cast<int>(i);
        }
        IrcChannelBuffer b;
        b.name = t;
        b.topic = (t[0] == '#') ? "Channel " + t : "Private chat with " + t;
        channels.push_back(b);
        return static_cast<int>(channels.size() - 1);
    }

    void connect_async(const std::string& srv, int prt, const std::string& nick) {
        disconnect();
        server = srv;
        port = prt;
        current_nick = nick;

        running = true;
        connecting = true;
        status_text = "Connecting to " + server + ":" + std::to_string(port) + "...";
        worker_thread = std::thread(&IrcClientEngine::thread_func, this);
    }

    void disconnect() {
        running = false;
        if (hs_sock_valid(sock_fd)) {
            hs_shutdown_both(sock_fd); // Immediate unblock of socket recv()
            hs_close_socket(sock_fd);
            sock_fd = HS_INVALID_SOCKET;
        }
        if (worker_thread.joinable() && std::this_thread::get_id() != worker_thread.get_id()) {
            worker_thread.join();
        }
        connected = false;
        connecting = false;
        status_text = "Disconnected.";
    }

    void send_raw(const std::string& line) {
        if (!hs_sock_valid(sock_fd)) return;
        std::string full = line + "\r\n";
        ::send(sock_fd, full.data(), static_cast<int>(full.size()), 0);
    }

    void send_chat_message(const std::string& target, const std::string& msg) {
        if (msg.empty()) return;
        if (msg[0] == '/') {
            handle_user_slash_command(msg);
            return;
        }
        if (!connected) {
            append_message(target, "***", "Not connected to IRC server.", 0xEF4444, true);
            return;
        }
        send_raw("PRIVMSG " + target + " :" + msg);
        append_message(target, current_nick, msg, 0x38BDF8);
    }

    void change_nickname(const std::string& new_nick) {
        if (new_nick.empty() || new_nick == current_nick) return;
        if (connected) {
            send_raw("NICK " + new_nick);
        }
        current_nick = new_nick;
        auto& cfg = get_app_config();
        cfg.irc_nick = new_nick;
        cfg.save();
    }

    // Operator and Voice Actions
    void op_user(const std::string& chan, const std::string& nick) {
        if (!chan.empty() && !nick.empty()) send_raw("MODE " + chan + " +o " + nick);
    }
    void deop_user(const std::string& chan, const std::string& nick) {
        if (!chan.empty() && !nick.empty()) send_raw("MODE " + chan + " -o " + nick);
    }
    void voice_user(const std::string& chan, const std::string& nick) {
        if (!chan.empty() && !nick.empty()) send_raw("MODE " + chan + " +v " + nick);
    }
    void devoice_user(const std::string& chan, const std::string& nick) {
        if (!chan.empty() && !nick.empty()) send_raw("MODE " + chan + " -v " + nick);
    }
    void kick_user(const std::string& chan, const std::string& nick, const std::string& reason = "") {
        if (!chan.empty() && !nick.empty()) {
            send_raw("KICK " + chan + " " + nick + (reason.empty() ? "" : (" :" + reason)));
        }
    }
    void ban_user(const std::string& chan, const std::string& nick) {
        if (!chan.empty() && !nick.empty()) {
            std::string mask = (nick.find('!') != std::string::npos) ? nick : (nick + "!*@*");
            send_raw("MODE " + chan + " +b " + mask);
        }
    }
    void unban_user(const std::string& chan, const std::string& mask) {
        if (!chan.empty() && !mask.empty()) {
            std::string m = (mask.find('!') != std::string::npos) ? mask : (mask + "!*@*");
            send_raw("MODE " + chan + " -b " + m);
        }
    }
    void whois_user(const std::string& nick) {
        if (!nick.empty()) send_raw("WHOIS " + nick);
    }

    void handle_user_slash_command(const std::string& cmd_line) {
        std::stringstream ss(cmd_line.substr(1));
        std::string verb;
        ss >> verb;
        for (char& c : verb) c = std::tolower(static_cast<unsigned char>(c));

        std::string cur_chan = active_channel_name();

        if (verb == "join" || verb == "j") {
            std::string ch;
            ss >> ch;
            if (!ch.empty()) {
                if (ch[0] != '#' && ch[0] != '&') ch = "#" + ch;
                send_raw("JOIN " + ch);
                int idx = find_or_create_channel(ch);
                active_chan_idx = idx;
            }
        } else if (verb == "part" || verb == "leave") {
            std::string ch;
            ss >> ch;
            std::string reason;
            std::getline(ss, reason);
            if (!reason.empty() && reason[0] == ' ') reason.erase(0, 1);
            if (ch.empty()) ch = cur_chan;
            else if (ch[0] != '#') {
                if (!cur_chan.empty() && cur_chan[0] == '#') {
                    reason = ch + (reason.empty() ? "" : (" " + reason));
                    ch = cur_chan;
                }
            }
            if (!ch.empty() && ch[0] == '#') {
                send_raw("PART " + ch + (reason.empty() ? "" : (" :" + reason)));
            }
        } else if (verb == "whois" || verb == "w") {
            std::string n;
            ss >> n;
            if (!n.empty()) {
                send_raw("WHOIS " + n);
                append_message(cur_chan, "***", "Querying WHOIS for " + n + "...", 0x38BDF8, true);
            } else {
                append_message(cur_chan, "***", "Usage: /whois <nickname>", 0x94A3B8, true);
            }
        } else if (verb == "whowas") {
            std::string n;
            ss >> n;
            if (!n.empty()) send_raw("WHOWAS " + n);
        } else if (verb == "op") {
            std::string n;
            ss >> n;
            if (!n.empty()) op_user(cur_chan, n);
        } else if (verb == "deop") {
            std::string n;
            ss >> n;
            if (!n.empty()) deop_user(cur_chan, n);
        } else if (verb == "voice" || verb == "v") {
            std::string n;
            ss >> n;
            if (!n.empty()) voice_user(cur_chan, n);
        } else if (verb == "devoice" || verb == "dv") {
            std::string n;
            ss >> n;
            if (!n.empty()) devoice_user(cur_chan, n);
        } else if (verb == "kick" || verb == "k") {
            std::string n, reason;
            ss >> n;
            std::getline(ss, reason);
            if (!reason.empty() && reason[0] == ' ') reason.erase(0, 1);
            if (!n.empty()) kick_user(cur_chan, n, reason);
        } else if (verb == "ban" || verb == "b") {
            std::string n;
            ss >> n;
            if (!n.empty()) ban_user(cur_chan, n);
        } else if (verb == "unban") {
            std::string n;
            ss >> n;
            if (!n.empty()) unban_user(cur_chan, n);
        } else if (verb == "mode" || verb == "m") {
            std::string rest;
            std::getline(ss, rest);
            if (!rest.empty() && rest[0] == ' ') rest.erase(0, 1);
            if (!rest.empty()) {
                if (rest[0] == '+' || rest[0] == '-') {
                    send_raw("MODE " + cur_chan + " " + rest);
                } else {
                    send_raw("MODE " + rest);
                }
            }
        } else if (verb == "topic" || verb == "t") {
            std::string top;
            std::getline(ss, top);
            if (!top.empty() && top[0] == ' ') top.erase(0, 1);
            if (!top.empty()) {
                send_raw("TOPIC " + cur_chan + " :" + top);
            } else {
                send_raw("TOPIC " + cur_chan);
            }
        } else if (verb == "me") {
            std::string act;
            std::getline(ss, act);
            if (!act.empty() && act[0] == ' ') act.erase(0, 1);
            if (!act.empty()) {
                send_raw("PRIVMSG " + cur_chan + " :\x01" "ACTION " + act + "\x01");
                append_message(cur_chan, current_nick, act, 0xA855F7, false, true);
            }
        } else if (verb == "notice") {
            std::string tgt, txt;
            ss >> tgt;
            std::getline(ss, txt);
            if (!txt.empty() && txt[0] == ' ') txt.erase(0, 1);
            if (!tgt.empty() && !txt.empty()) {
                send_raw("NOTICE " + tgt + " :" + txt);
                append_message(tgt, current_nick, "-> (" + tgt + ") " + txt, 0xFBBF24, true);
            }
        } else if (verb == "query" || verb == "q") {
            std::string n, txt;
            ss >> n;
            std::getline(ss, txt);
            if (!txt.empty() && txt[0] == ' ') txt.erase(0, 1);
            if (!n.empty()) {
                int idx = find_or_create_channel(n);
                active_chan_idx = idx;
                if (!txt.empty()) {
                    send_raw("PRIVMSG " + n + " :" + txt);
                    append_message(n, current_nick, txt, 0x38BDF8);
                }
            }
        } else if (verb == "nick") {
            std::string n;
            ss >> n;
            if (!n.empty()) change_nickname(n);
        } else if (verb == "msg") {
            std::string tgt, txt;
            ss >> tgt;
            std::getline(ss, txt);
            if (!txt.empty() && txt[0] == ' ') txt.erase(0, 1);
            if (!tgt.empty() && !txt.empty()) {
                send_raw("PRIVMSG " + tgt + " :" + txt);
                append_message(tgt, current_nick, txt, 0x38BDF8);
            }
        } else if (verb == "away") {
            std::string reason;
            std::getline(ss, reason);
            if (!reason.empty() && reason[0] == ' ') reason.erase(0, 1);
            if (!reason.empty()) {
                send_raw("AWAY :" + reason);
                append_message(cur_chan, "***", "You are now marked as away: " + reason, 0xF59E0B, true);
            } else {
                send_raw("AWAY");
                append_message(cur_chan, "***", "You are no longer marked as away.", 0x10B981, true);
            }
        } else if (verb == "back") {
            send_raw("AWAY");
            append_message(cur_chan, "***", "You are no longer marked as away.", 0x10B981, true);
        } else if (verb == "invite") {
            std::string n, ch;
            ss >> n >> ch;
            if (ch.empty()) ch = cur_chan;
            if (!n.empty() && !ch.empty()) send_raw("INVITE " + n + " " + ch);
        } else if (verb == "names") {
            std::string ch;
            ss >> ch;
            if (ch.empty()) ch = cur_chan;
            if (!ch.empty()) send_raw("NAMES " + ch);
        } else if (verb == "motd") {
            send_raw("MOTD");
        } else if (verb == "time") {
            send_raw("TIME");
        } else if (verb == "version") {
            send_raw("VERSION");
        } else if (verb == "identify" || verb == "ident") {
            std::string pass;
            ss >> pass;
            if (!pass.empty()) {
                send_raw("PRIVMSG NickServ :IDENTIFY " + pass);
                auto& cfg = get_app_config();
                cfg.nickserv_passwords[current_nick] = pass;
                cfg.save();
                append_message("Status", "***", "Sent IDENTIFY to NickServ and saved password for '" + current_nick + "'.", 0x10B981, true);
            }
        } else if (verb == "quit") {
            std::string q_msg;
            std::getline(ss, q_msg);
            if (!q_msg.empty() && q_msg[0] == ' ') q_msg.erase(0, 1);
            send_raw("QUIT :" + (q_msg.empty() ? "HamShack++ Unified Communications Suite" : q_msg));
            disconnect();
        } else if (verb == "quote" || verb == "raw") {
            std::string raw;
            std::getline(ss, raw);
            if (!raw.empty() && raw[0] == ' ') raw.erase(0, 1);
            send_raw(raw);
        } else if (verb == "help") {
            append_message(cur_chan, "***", "Available IRC Commands:", 0x00E5FF, true);
            append_message(cur_chan, "***", "  /join <#chan>, /part [reason], /msg <nick> <text>, /query <nick>, /notice <nick> <text>", 0x94A3B8, true);
            append_message(cur_chan, "***", "  /whois <nick>, /whowas <nick>, /me <action>, /topic [new topic], /nick <new_nick>", 0x94A3B8, true);
            append_message(cur_chan, "***", "  /op <nick>, /deop <nick>, /voice <nick>, /devoice <nick>, /kick <nick> [reason], /ban <nick>", 0x94A3B8, true);
            append_message(cur_chan, "***", "  /away [reason], /back, /invite <nick>, /names, /motd, /identify <pass>, /raw <cmd>, /quit", 0x94A3B8, true);
        } else {
            append_message(cur_chan, "***", "Unknown command /" + verb + ". Type /help for available commands.", 0xEF4444, true);
        }
    }

private:
    void thread_func() {
        ensure_winsock();
        struct addrinfo hints{}, *res = nullptr;
        hints.ai_family = AF_UNSPEC;
        hints.ai_socktype = SOCK_STREAM;

        std::string port_str = std::to_string(port);
        if (getaddrinfo(server.c_str(), port_str.c_str(), &hints, &res) != 0 || !res) {
            status_text = "Connection failed: Could not resolve " + server;
            connecting = false;
            return;
        }

        sock_fd = ::socket(res->ai_family, res->ai_socktype, res->ai_protocol);
        if (!hs_sock_valid(sock_fd)) {
            freeaddrinfo(res);
            status_text = "Failed to create socket.";
            connecting = false;
            return;
        }

        if (::connect(sock_fd, res->ai_addr, static_cast<int>(res->ai_addrlen)) < 0) {
            freeaddrinfo(res);
            hs_close_socket(sock_fd);
            sock_fd = HS_INVALID_SOCKET;
            status_text = "Connection to " + server + " refused.";
            connecting = false;
            return;
        }
        freeaddrinfo(res);

        connected = true;
        connecting = false;
        status_text = "Connected to " + server + ". Authenticating...";
        append_message("Status", "***", "Connected to " + server + ":" + port_str, 0x10B981, true);

        auto& cfg = get_app_config();
        if (cfg.irc_use_sasl) {
            send_raw("CAP LS 302");
        }
        send_raw("NICK " + current_nick);
        send_raw("USER " + username + " 0 * :" + realname);

        std::string recv_buf;
        char chunk[1024];

        while (running && hs_sock_valid(sock_fd)) {
            hs_ssize_t n = ::recv(sock_fd, chunk, static_cast<int>(sizeof(chunk) - 1), 0);
            if (n <= 0) break;
            chunk[n] = '\0';
            recv_buf.append(chunk, n);

            size_t pos = 0;
            while ((pos = recv_buf.find("\r\n")) != std::string::npos) {
                std::string line = recv_buf.substr(0, pos);
                recv_buf.erase(0, pos + 2);
                parse_irc_line(line);
            }
        }

        connected = false;
        connecting = false;
        status_text = "Disconnected from " + server;
        append_message("Status", "***", "Disconnected from server.", 0xEF4444, true);
    }

public:
    void parse_irc_line(const std::string& line) {
        if (line.empty()) return;

        // PING / PONG keepalive
        if (line.rfind("PING :", 0) == 0) {
            std::string token = line.substr(6);
            send_raw("PONG :" + token);
            return;
        }

        std::string prefix, cmd, trailing;
        std::vector<std::string> params;

        std::string work = line;
        if (work[0] == ':') {
            size_t sp = work.find(' ');
            if (sp != std::string::npos) {
                prefix = work.substr(1, sp - 1);
                work.erase(0, sp + 1);
            }
        }

        size_t colon_pos = work.find(" :");
        if (colon_pos != std::string::npos) {
            trailing = work.substr(colon_pos + 2);
            work.erase(colon_pos);
        }

        std::stringstream ss(work);
        ss >> cmd;
        std::string p;
        while (ss >> p) params.push_back(p);

        std::string sender_nick = prefix;
        size_t ex = prefix.find('!');
        if (ex != std::string::npos) sender_nick = prefix.substr(0, ex);

        std::string cur_chan = active_channel_name();

        // Capability / SASL Negotiation
        if (cmd == "CAP") {
            auto& cfg = get_app_config();
            if (params.size() >= 2 && params[1] == "LS") {
                if (trailing.find("sasl") != std::string::npos && cfg.irc_use_sasl) {
                    send_raw("CAP REQ :sasl");
                } else {
                    send_raw("CAP END");
                }
            } else if (params.size() >= 2 && params[1] == "ACK") {
                if (trailing.find("sasl") != std::string::npos) {
                    send_raw("AUTHENTICATE PLAIN");
                }
            } else if (params.size() >= 2 && params[1] == "NAK") {
                send_raw("CAP END");
                append_message("Status", "***", "SASL capability rejected by server; falling back to standard IRC.", 0xF59E0B, true);
            }
        } else if (cmd == "AUTHENTICATE" && params.size() >= 1 && params[0] == "+") {
            auto& cfg = get_app_config();
            std::string auth_user = cfg.irc_sasl_user.empty() ? current_nick : cfg.irc_sasl_user;
            std::string auth_pass = cfg.irc_sasl_pass;
            if (auth_pass.empty()) {
                auto it = cfg.nickserv_passwords.find(current_nick);
                if (it != cfg.nickserv_passwords.end()) auth_pass = it->second;
            }

            if (!auth_pass.empty()) {
                std::string auth_raw = "\0" + auth_user + "\0" + auth_pass;
                std::string b64 = base64_encode(auth_raw);
                send_raw("AUTHENTICATE " + b64);
            } else {
                send_raw("AUTHENTICATE *");
                send_raw("CAP END");
                append_message("Status", "***", "No SASL password set for '" + auth_user + "'; falling back to standard IRC.", 0xF59E0B, true);
            }
        } else if (cmd == "900" || cmd == "903") { // SASL success
            send_raw("CAP END");
            append_message("Status", "***", "SASL authentication successful.", 0x10B981, true);
        } else if (cmd == "904" || cmd == "905") { // SASL failed
            send_raw("CAP END");
            append_message("Status", "***", "SASL authentication failed; falling back to standard IRC registration.", 0xF59E0B, true);
        }

        // Welcome (001) -> Auto-join default channel
        if (cmd == "001") {
            status_text = "Online on " + server + " as " + current_nick;
            append_message("Status", "***", "Welcome to Libera.Chat: " + trailing, 0x00E5FF, true);

            auto& cfg = get_app_config();
            auto it = cfg.nickserv_passwords.find(current_nick);
            if (it != cfg.nickserv_passwords.end() && !it->second.empty()) {
                send_raw("PRIVMSG NickServ :IDENTIFY " + it->second);
                append_message("Status", "***", "Auto-identifying to NickServ for '" + current_nick + "'...", 0xFACC15, true);
            }

            std::string def_ch = cfg.irc_default_channel.empty() ? "#hamradio" : cfg.irc_default_channel;
            send_raw("JOIN " + def_ch);
            find_or_create_channel(def_ch);
        }

        // Channel or Private Messages (PRIVMSG / NOTICE)
        if (cmd == "PRIVMSG" || cmd == "NOTICE") {
            std::string target = params.empty() ? "Status" : params[0];
            bool is_notice = (cmd == "NOTICE");

            // CTCP ACTION (/me)
            bool is_action = false;
            std::string clean_text = trailing;
            if (clean_text.size() >= 8 && clean_text.front() == '\x01' && clean_text.back() == '\x01') {
                clean_text = clean_text.substr(1, clean_text.size() - 2);
                if (clean_text.rfind("ACTION ", 0) == 0) {
                    is_action = true;
                    clean_text = clean_text.substr(7);
                }
            }

            // Check if from NickServ
            if (hs_strcasecmp(sender_nick.c_str(), "NickServ") == 0) {
                append_message("Status", "NickServ", trailing, 0xF59E0B, true);
                if (trailing.find("IDENTIFY") != std::string::npos || trailing.find("registered") != std::string::npos) {
                    auto& cfg = get_app_config();
                    auto it = cfg.nickserv_passwords.find(current_nick);
                    if (it != cfg.nickserv_passwords.end() && !it->second.empty()) {
                        send_raw("PRIVMSG NickServ :IDENTIFY " + it->second);
                        append_message("Status", "***", "Auto-sent NickServ IDENTIFY password for " + current_nick, 0x10B981, true);
                    }
                }
                return;
            }

            std::string dest_chan = (target[0] == '#' || target[0] == '&') ? target : sender_nick;
            append_message(dest_chan, sender_nick, clean_text, is_notice ? 0xFBBF24 : (is_action ? 0xA855F7 : 0xF8FAFC), is_notice, is_action);
        }

        // Channel User List (353 RPL_NAMREPLY)
        if (cmd == "353" && params.size() >= 3) {
            std::string ch = params[2];
            int idx = find_or_create_channel(ch);
            if (idx >= 0 && idx < static_cast<int>(channels.size())) {
                std::stringstream nss(trailing);
                std::string usr;
                while (nss >> usr) {
                    char pfx = 0;
                    if (usr[0] == '@' || usr[0] == '+' || usr[0] == '~' || usr[0] == '&' || usr[0] == '%') {
                        pfx = usr[0];
                        usr.erase(0, 1);
                    }
                    if (!usr.empty()) channels[idx].users[usr] = pfx;
                }
            }
        }

        // Channel Topic (332 RPL_TOPIC)
        if (cmd == "332" && params.size() >= 2) {
            std::string ch = params[1];
            int idx = find_or_create_channel(ch);
            if (idx >= 0 && idx < static_cast<int>(channels.size())) {
                channels[idx].topic = trailing;
            }
        }

        // Channel or User MODE change
        if (cmd == "MODE" && params.size() >= 2) {
            std::string target = params[0];
            std::string mode_str = params[1];
            std::string nick_arg = (params.size() >= 3) ? params[2] : "";

            int idx = find_or_create_channel(target);
            if (idx >= 0 && idx < static_cast<int>(channels.size())) {
                if (!nick_arg.empty() && mode_str.size() >= 2) {
                    char dir = mode_str[0]; // '+' or '-'
                    char flag = mode_str[1]; // 'o', 'v', 'b', etc.
                    auto it = channels[idx].users.find(nick_arg);
                    if (it != channels[idx].users.end()) {
                        if (dir == '+') {
                            if (flag == 'o') it->second = '@';
                            else if (flag == 'v' && it->second != '@') it->second = '+';
                        } else if (dir == '-') {
                            if (flag == 'o' && it->second == '@') it->second = 0;
                            else if (flag == 'v' && it->second == '+') it->second = 0;
                        }
                    }
                    std::string desc = (dir == '+' && flag == 'o') ? "gives channel operator (+o) to " + nick_arg :
                                       (dir == '-' && flag == 'o') ? "removes channel operator (-o) from " + nick_arg :
                                       (dir == '+' && flag == 'v') ? "gives voice (+v) to " + nick_arg :
                                       (dir == '-' && flag == 'v') ? "removes voice (-v) from " + nick_arg :
                                       "sets mode " + mode_str + " " + nick_arg;
                    append_message(target, "***", sender_nick + " " + desc, 0xF59E0B, true);
                } else {
                    append_message(target, "***", sender_nick + " sets mode " + mode_str, 0x94A3B8, true);
                }
            }
        }

        // KICK message: :kicker!user@host KICK #channel targetNick :reason
        if (cmd == "KICK" && params.size() >= 2) {
            std::string ch = params[0];
            std::string kicked = params[1];
            int idx = find_or_create_channel(ch);
            if (idx >= 0 && idx < static_cast<int>(channels.size())) {
                channels[idx].users.erase(kicked);
                std::string kick_msg = kicked + " was kicked by " + sender_nick;
                if (!trailing.empty()) kick_msg += " (" + trailing + ")";
                append_message(ch, "***", kick_msg, 0xEF4444, true);
            }
        }

        // User JOIN
        if (cmd == "JOIN") {
            std::string ch = params.empty() ? trailing : params[0];
            int idx = find_or_create_channel(ch);
            if (idx >= 0 && idx < static_cast<int>(channels.size())) {
                channels[idx].users[sender_nick] = 0;
                append_message(ch, "***", sender_nick + " has joined " + ch, 0x10B981, true);
            }
        }

        // User PART
        if (cmd == "PART") {
            std::string ch = params.empty() ? "" : params[0];
            int idx = find_or_create_channel(ch);
            if (idx >= 0 && idx < static_cast<int>(channels.size())) {
                channels[idx].users.erase(sender_nick);
                append_message(ch, "***", sender_nick + " has left " + ch, 0x94A3B8, true);
            }
        }

        // User QUIT
        if (cmd == "QUIT") {
            for (auto& ch : channels) {
                if (ch.users.erase(sender_nick) > 0) {
                    append_message(ch.name, "***", sender_nick + " has quit (" + trailing + ")", 0x94A3B8, true);
                }
            }
        }

        // User NICK change
        if (cmd == "NICK") {
            std::string new_n = trailing.empty() ? (params.empty() ? "" : params[0]) : trailing;
            if (sender_nick == current_nick) current_nick = new_n;
            for (auto& ch : channels) {
                auto it = ch.users.find(sender_nick);
                if (it != ch.users.end()) {
                    char pfx = it->second;
                    ch.users.erase(it);
                    ch.users[new_n] = pfx;
                    append_message(ch.name, "***", sender_nick + " is now known as " + new_n, 0x38BDF8, true);
                }
            }
        }

        // WHOIS Numeric Responses
        if (cmd == "311" && params.size() >= 4) { // RPL_WHOISUSER
            append_message(cur_chan, "***", "[WHOIS] " + params[1] + " is " + params[2] + "@" + params[3] + " (" + trailing + ")", 0x38BDF8, true);
        } else if (cmd == "312" && params.size() >= 3) { // RPL_WHOISSERVER
            append_message(cur_chan, "***", "[WHOIS] " + params[1] + " connected via " + params[2] + " (" + trailing + ")", 0x38BDF8, true);
        } else if (cmd == "319" && params.size() >= 2) { // RPL_WHOISCHANNELS
            append_message(cur_chan, "***", "[WHOIS] " + params[1] + " channels: " + trailing, 0x38BDF8, true);
        } else if (cmd == "301" && params.size() >= 2) { // RPL_AWAY
            append_message(cur_chan, "***", "[WHOIS] " + params[1] + " is away: " + trailing, 0xF59E0B, true);
        } else if (cmd == "330" && params.size() >= 3) { // RPL_WHOISACCOUNT
            append_message(cur_chan, "***", "[WHOIS] " + params[1] + " is logged in as " + params[2], 0x10B981, true);
        } else if (cmd == "317" && params.size() >= 3) { // RPL_WHOISIDLE
            append_message(cur_chan, "***", "[WHOIS] " + params[1] + " idle " + params[2] + "s", 0x94A3B8, true);
        } else if (cmd == "318" && params.size() >= 2) { // RPL_ENDOFWHOIS
            append_message(cur_chan, "***", "[WHOIS] End of WHOIS for " + params[1], 0x38BDF8, true);
        } else if (cmd == "401") { // ERR_NOSUCHNICK
            std::string n = (params.size() > 1) ? params[1] : trailing;
            append_message(cur_chan, "***", "No such nick/channel: " + n, 0xEF4444, true);
        } else if (cmd == "404") { // ERR_CANNOTSENDTOCHAN
            append_message(cur_chan, "***", "Cannot send to channel: " + trailing, 0xEF4444, true);
        } else if (cmd == "482") { // ERR_CHANOPRIVSNEEDED
            append_message(cur_chan, "***", "You're not channel operator (+o needed): " + trailing, 0xEF4444, true);
        } else if (cmd == "433") { // ERR_NICKNAMEINUSE
            std::string n = (params.size() > 1) ? params[1] : "";
            append_message("Status", "***", "Nickname '" + n + "' is already in use.", 0xEF4444, true);
        }
    }
};

inline IrcClientEngine& get_irc_engine() {
    static IrcClientEngine irc;
    return irc;
}

} // namespace hamshack
