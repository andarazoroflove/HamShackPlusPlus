#pragma once
// ============================================================================
// hamshack_platform.hpp — Cross-Platform Socket, Time, String & OS Abstractions
// Supports Native Windows 10/11 (x86_64 Winsock2 / Win32) & macOS Sonoma+ (POSIX)
// ============================================================================

#ifdef _WIN32
#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0A00
#endif
#ifndef WINVER
#define WINVER 0x0A00
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <io.h>
#else
#include <arpa/inet.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <poll.h>
#include <strings.h>
#include <sys/ioctl.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <termios.h>
#include <unistd.h>
#endif

#include <cmath>
#include <cstdint>
#include <cstring>
#include <ctime>
#include <filesystem>
#include <string>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace hamshack {

#ifdef _WIN32
using hs_socket_t = SOCKET;
static constexpr hs_socket_t HS_INVALID_SOCKET = INVALID_SOCKET;
using hs_ssize_t = int;
#else
using hs_socket_t = int;
static constexpr hs_socket_t HS_INVALID_SOCKET = -1;
using hs_ssize_t = ssize_t;
#endif

inline bool hs_sock_valid(hs_socket_t s) {
#ifdef _WIN32
    return s != INVALID_SOCKET;
#else
    return s >= 0;
#endif
}

inline void ensure_winsock() {
#ifdef _WIN32
    struct WinsockInit {
        WinsockInit() {
            WSADATA wsa{};
            WSAStartup(MAKEWORD(2, 2), &wsa);
        }
        ~WinsockInit() {
            WSACleanup();
        }
    };
    static WinsockInit s_init;
    (void)s_init;
#endif
}

inline void hs_close_socket(hs_socket_t s) {
    if (!hs_sock_valid(s)) return;
#ifdef _WIN32
    ::closesocket(s);
#else
    ::close(s);
#endif
}

inline void hs_shutdown_both(hs_socket_t s) {
    if (!hs_sock_valid(s)) return;
#ifdef _WIN32
    ::shutdown(s, SD_BOTH);
#else
    ::shutdown(s, SHUT_RDWR);
#endif
}

inline void hs_set_reuseaddr(hs_socket_t s) {
    if (!hs_sock_valid(s)) return;
    int opt = 1;
#ifdef _WIN32
    ::setsockopt(s, SOL_SOCKET, SO_REUSEADDR, reinterpret_cast<const char*>(&opt), sizeof(opt));
#else
    ::setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
#ifdef SO_REUSEPORT
    ::setsockopt(s, SOL_SOCKET, SO_REUSEPORT, &opt, sizeof(opt));
#endif
#endif
}

inline void hs_set_nonblocking(hs_socket_t s) {
    if (!hs_sock_valid(s)) return;
#ifdef _WIN32
    u_long mode = 1;
    ::ioctlsocket(s, FIONBIO, &mode);
#else
    int flags = ::fcntl(s, F_GETFL, 0);
    if (flags >= 0) ::fcntl(s, F_SETFL, flags | O_NONBLOCK);
#endif
}

inline void hs_set_rw_timeout_ms(hs_socket_t s, int timeout_ms) {
    if (!hs_sock_valid(s)) return;
#ifdef _WIN32
    DWORD tv = static_cast<DWORD>(timeout_ms);
    ::setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, reinterpret_cast<const char*>(&tv), sizeof(tv));
    ::setsockopt(s, SOL_SOCKET, SO_SNDTIMEO, reinterpret_cast<const char*>(&tv), sizeof(tv));
#else
    struct timeval tv{};
    tv.tv_sec = timeout_ms / 1000;
    tv.tv_usec = (timeout_ms % 1000) * 1000;
    ::setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
    ::setsockopt(s, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv));
#endif
}

inline int hs_poll_read(hs_socket_t s, int timeout_ms) {
    if (!hs_sock_valid(s)) return -1;
#ifdef _WIN32
    fd_set fds;
    FD_ZERO(&fds);
    FD_SET(s, &fds);
    TIMEVAL tv{};
    tv.tv_sec = timeout_ms / 1000;
    tv.tv_usec = (timeout_ms % 1000) * 1000;
    return ::select(0, &fds, nullptr, nullptr, &tv);
#else
    struct pollfd pfd{};
    pfd.fd = s;
    pfd.events = POLLIN;
    return ::poll(&pfd, 1, timeout_ms);
#endif
}

inline void hs_gmtime(const std::time_t* t, std::tm* out) {
    if (!t || !out) return;
#ifdef _WIN32
    gmtime_s(out, t);
#else
    gmtime_r(t, out);
#endif
}

inline void hs_localtime(const std::time_t* t, std::tm* out) {
    if (!t || !out) return;
#ifdef _WIN32
    localtime_s(out, t);
#else
    localtime_r(t, out);
#endif
}

inline int hs_strcasecmp(const char* a, const char* b) {
    if (!a || !b) return (a == b) ? 0 : (a ? 1 : -1);
#ifdef _WIN32
    return _stricmp(a, b);
#else
    return strcasecmp(a, b);
#endif
}

inline void hs_mkdir_p(const std::string& dir) {
    std::error_code ec;
    std::filesystem::create_directories(dir, ec);
}

} // namespace hamshack
