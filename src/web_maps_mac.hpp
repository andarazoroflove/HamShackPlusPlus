#pragma once
// ============================================================================
// web_maps_mac.hpp — Persistent WebView Manager & Native File Pickers
// - macOS: Live WKWebView instances with persistent WKWebsiteDataStore & NSOpenPanel
// - Windows: Zero-DLL Microsoft Edge WebView2 (ICoreWebView2) with persistent
//   cookies/localStorage & Win32 GetOpenFileNameW dialogs
// ============================================================================

#include <algorithm>
#include <array>
#include <filesystem>
#include <string>
#include <vector>
#include "hamshack_config.hpp"

#ifdef _WIN32
#include <commdlg.h>
#include <objbase.h>
#include <shellapi.h>
#endif

#ifdef __APPLE__
#include "../includes/macos_d2d_shim.hpp"
#endif

namespace hamshack_web {

static constexpr const char* kWebMapHomeUrls[5] = {
    "https://www.qrz.com/db/",
    "https://pskreporter.info/",
    "https://hf.dxview.org/",
    "https://dxlook.com/",
    "https://openhamclock.com/"
};

static constexpr const char* kWebMapTitles[5] = {
    "QRZ Callsign & Ham Database (https://www.qrz.com/)",
    "PSKReporter (https://pskreporter.info/)",
    "DXView HF Propagation (https://hf.dxview.org/)",
    "DXLook HF Map (https://dxlook.com/)",
    "OpenHamClock (https://openhamclock.com/)"
};

#if defined(__APPLE__) && !defined(MESHCORE_IOS)

// Handle target="_blank" links (e.g. PSKReporter's "Map" link) by opening them in the active WKWebView
static id wk_create_webview_for_action(id, SEL, id webView, id, id navigationAction, id) {
    if (webView && navigationAction) {
        id targetFrame = mac_msg_send<id>(navigationAction, sel_registerName("targetFrame"));
        bool is_main = targetFrame ? static_cast<bool>(mac_msg_send<ObjcBool>(targetFrame, sel_registerName("isMainFrame"))) : false;
        if (!is_main) {
            id req = mac_msg_send<id>(navigationAction, sel_registerName("request"));
            if (req) {
                mac_msg_send<id>(webView, sel_registerName("loadRequest:"), req);
            }
        }
    }
    return nullptr;
}

static Class ensure_wk_delegate_class() {
    static Class s_cls = nullptr;
    if (s_cls) return s_cls;
    s_cls = objc_getClass("HamShackWKDelegate");
    if (!s_cls) {
        Class ns_obj = objc_getClass("NSObject");
        s_cls = objc_allocateClassPair(ns_obj, "HamShackWKDelegate", 0);
        if (s_cls) {
            class_addMethod(s_cls,
                            sel_registerName("webView:createWebViewWithConfiguration:forNavigationAction:windowFeatures:"),
                            reinterpret_cast<IMP>(wk_create_webview_for_action),
                            "@@:@@@@");
            objc_registerClassPair(s_cls);
        }
    }
    return s_cls;
}

class PersistentWebMapsManager {
public:
    std::array<id, 5> web_views{nullptr, nullptr, nullptr, nullptr, nullptr};
    id wk_delegate = nullptr;
    int current_visible_idx = -1;
    CGRect last_rect{{0, 0}, {0, 0}};

    void ensure_web_view(int idx, id parent_ns_view, CGRect frame) {
        if (idx < 0 || idx >= 5 || !parent_ns_view) return;
        if (web_views[idx]) return;

        // Ensure WebKit classes are loaded
        dlopen("/System/Library/Frameworks/WebKit.framework/WebKit", 1);

        if (!wk_delegate) {
            Class dcls = ensure_wk_delegate_class();
            if (dcls) {
                id d_alloc = mac_msg_send<id>(reinterpret_cast<id>(dcls), sel_registerName("alloc"));
                wk_delegate = mac_msg_send<id>(d_alloc, sel_registerName("init"));
                if (wk_delegate) CFRetain(wk_delegate);
            }
        }

        id cfg_alloc = mac_cls_send<id>("WKWebViewConfiguration", sel_registerName("alloc"));
        id cfg = mac_msg_send<id>(cfg_alloc, sel_registerName("init"));

        // Use defaultDataStore so cookies, IndexedDB, and localStorage persist across app launches!
        id default_store = mac_cls_send<id>("WKWebsiteDataStore", sel_registerName("defaultDataStore"));
        if (cfg && default_store) {
            mac_msg_send<void>(cfg, sel_registerName("setWebsiteDataStore:"), default_store);
        }

        // Enable shared HTTPCookieStorage acceptance
        id shared_cookies = mac_cls_send<id>("NSHTTPCookieStorage", sel_registerName("sharedHTTPCookieStorage"));
        if (shared_cookies) {
            // NSHTTPCookieAcceptPolicyAlways = 0
            mac_msg_send<void>(shared_cookies, sel_registerName("setCookieAcceptPolicy:"), static_cast<unsigned long>(0));
        }

        id wv_alloc = mac_cls_send<id>("WKWebView", sel_registerName("alloc"));
        id wv = mac_msg_send<id>(wv_alloc,
                                 sel_registerName("initWithFrame:configuration:"),
                                 frame, cfg);
        if (!wv) return;
        CFRetain(wv);

        // Set modern macOS Safari User-Agent
        mac_msg_send<void>(wv, sel_registerName("setCustomUserAgent:"),
                           mac_ns_str("Mozilla/5.0 (Macintosh; Intel Mac OS X 14_5) AppleWebKit/605.1.15 (KHTML, like Gecko) Version/17.5 Safari/605.1.15"));

        if (wk_delegate) {
            mac_msg_send<void>(wv, sel_registerName("setUIDelegate:"), wk_delegate);
        }

        // Attach to main NSView and load default / bookmarked URL
        mac_msg_send<void>(wv, sel_registerName("setHidden:"), static_cast<ObjcBool>(1));
        mac_msg_send<void>(parent_ns_view, sel_registerName("addSubview:"), wv);

        web_views[idx] = wv;

        // Use bookmarked URL if saved by user, otherwise default home URL
        auto& app_cfg = hamshack::get_app_config();
        std::string init_url = kWebMapHomeUrls[idx];
        if (idx < static_cast<int>(app_cfg.bookmarked_urls.size()) && !app_cfg.bookmarked_urls[idx].empty()) {
            init_url = app_cfg.bookmarked_urls[idx];
        }
        load_url(idx, init_url.c_str());

        // Restore persistent zoom level for this tab
        double saved_zoom = (idx < static_cast<int>(app_cfg.page_zoom_levels.size())) ? app_cfg.page_zoom_levels[idx] : 1.0;
        if (saved_zoom < 0.3 || saved_zoom > 3.5) saved_zoom = 1.0;
        mac_msg_send<void>(wv, sel_registerName("setPageZoom:"), static_cast<CGFloat>(saved_zoom));
    }

    void zoom_in(int idx) {
        if (idx < 0 || idx >= 5) return;
        auto& app_cfg = hamshack::get_app_config();
        double z = std::min(3.0, app_cfg.page_zoom_levels[idx] * 1.15);
        set_zoom(idx, z);
    }

    void zoom_out(int idx) {
        if (idx < 0 || idx >= 5) return;
        auto& app_cfg = hamshack::get_app_config();
        double z = std::max(0.4, app_cfg.page_zoom_levels[idx] / 1.15);
        set_zoom(idx, z);
    }

    void zoom_reset(int idx) {
        set_zoom(idx, 1.0);
    }

    void set_zoom(int idx, double z) {
        if (idx < 0 || idx >= 5) return;
        auto& app_cfg = hamshack::get_app_config();
        app_cfg.page_zoom_levels[idx] = z;
        app_cfg.save();
        if (web_views[idx]) {
            mac_msg_send<void>(web_views[idx], sel_registerName("setPageZoom:"), static_cast<CGFloat>(z));
        }
    }

    double get_zoom(int idx) const {
        if (idx < 0 || idx >= 5) return 1.0;
        auto& app_cfg = hamshack::get_app_config();
        return app_cfg.page_zoom_levels[idx];
    }

    void load_url(int idx, const char* url_str) {
        if (idx < 0 || idx >= 5 || !web_views[idx] || !url_str) return;
        id url = mac_cls_send<id>("NSURL", sel_registerName("URLWithString:"), mac_ns_str(url_str));
        if (!url) return;
        id req = mac_cls_send<id>("NSURLRequest", sel_registerName("requestWithURL:"), url);
        if (!req) return;
        mac_msg_send<id>(web_views[idx], sel_registerName("loadRequest:"), req);
    }

    void bookmark_current_page(int idx) {
        if (idx < 0 || idx >= 5) return;
        std::string cur = current_url(idx);
        if (!cur.empty()) {
            auto& app_cfg = hamshack::get_app_config();
            app_cfg.bookmarked_urls[idx] = cur;
            app_cfg.save();
        }
    }

    void go_back(int idx) {
        if (idx >= 0 && idx < 5 && web_views[idx]) {
            mac_msg_send<id>(web_views[idx], sel_registerName("goBack"));
        }
    }

    void go_forward(int idx) {
        if (idx >= 0 && idx < 5 && web_views[idx]) {
            mac_msg_send<id>(web_views[idx], sel_registerName("goForward"));
        }
    }

    void reload(int idx) {
        if (idx >= 0 && idx < 5 && web_views[idx]) {
            mac_msg_send<id>(web_views[idx], sel_registerName("reload"));
        }
    }

    void open_in_external_browser(int idx) {
        std::string cur = current_url(idx);
        if (cur.empty() && idx >= 0 && idx < 5) cur = kWebMapHomeUrls[idx];
        id url = mac_cls_send<id>("NSURL", sel_registerName("URLWithString:"), mac_ns_str(cur.c_str()));
        id ws = mac_cls_send<id>("NSWorkspace", sel_registerName("sharedWorkspace"));
        if (url && ws) {
            mac_msg_send<ObjcBool>(ws, sel_registerName("openURL:"), url);
        }
    }

    std::string current_url(int idx) const {
        if (idx < 0 || idx >= 5 || !web_views[idx]) return "";
        id url = mac_msg_send<id>(web_views[idx], sel_registerName("URL"));
        if (!url) {
            auto& app_cfg = hamshack::get_app_config();
            if (idx < static_cast<int>(app_cfg.bookmarked_urls.size()) && !app_cfg.bookmarked_urls[idx].empty()) {
                return app_cfg.bookmarked_urls[idx];
            }
            return kWebMapHomeUrls[idx];
        }
        id abs_str = mac_msg_send<id>(url, sel_registerName("absoluteString"));
        std::string s = mac_ns_to_std_str(abs_str);
        return s.empty() ? std::string(kWebMapHomeUrls[idx]) : s;
    }

    bool is_loading(int idx) const {
        if (idx < 0 || idx >= 5 || !web_views[idx]) return false;
        return static_cast<bool>(mac_msg_send<ObjcBool>(web_views[idx], sel_registerName("isLoading")));
    }

    // Synchronize visibility & frame during UI render pass
    void sync_visibility(void* parent_view_ptr, int active_web_idx, float x, float y, float w, float h) {
        id parent_ns_view = reinterpret_cast<id>(parent_view_ptr);
        if (!parent_ns_view) return;
        CGRect target_r = CGRectMake(x, y, std::max(100.0f, w), std::max(100.0f, h));

        if (active_web_idx >= 0 && active_web_idx < 5) {
            ensure_web_view(active_web_idx, parent_ns_view, target_r);
        }

        for (int i = 0; i < 5; ++i) {
            if (!web_views[i]) continue;
            bool should_show = (i == active_web_idx);
            bool currently_hidden = static_cast<bool>(mac_msg_send<ObjcBool>(web_views[i], sel_registerName("isHidden")));
            if (should_show) {
                mac_msg_send<void>(web_views[i], sel_registerName("setFrame:"), target_r);
                if (currently_hidden) {
                    mac_msg_send<void>(web_views[i], sel_registerName("setHidden:"), static_cast<ObjcBool>(0));
                }
            } else {
                if (!currently_hidden) {
                    mac_msg_send<void>(web_views[i], sel_registerName("setHidden:"), static_cast<ObjcBool>(1));
                }
            }
        }
        current_visible_idx = active_web_idx;
        last_rect = target_r;
    }

    void hide_all() {
        if (current_visible_idx == -1) return;
        for (int i = 0; i < 5; ++i) {
            if (web_views[i]) {
                mac_msg_send<void>(web_views[i], sel_registerName("setHidden:"), static_cast<ObjcBool>(1));
            }
        }
        current_visible_idx = -1;
    }

    bool has_native_webview() const { return true; }
};

inline PersistentWebMapsManager& get_web_maps() {
    static PersistentWebMapsManager* s_mgr = new PersistentWebMapsManager();
    return *s_mgr;
}

// Native macOS NSOpenPanel to select an ADIF (.adi / .adif) file for import
inline std::string pick_adif_file_macos() {
    id panel = mac_cls_send<id>("NSOpenPanel", sel_registerName("openPanel"));
    if (!panel) return "";
    mac_msg_send<void>(panel, sel_registerName("setTitle:"), mac_ns_str("Import WSJT-X / GridTracker ADIF (.adi / .adif) Log"));
    mac_msg_send<void>(panel, sel_registerName("setCanChooseFiles:"), static_cast<ObjcBool>(1));
    mac_msg_send<void>(panel, sel_registerName("setCanChooseDirectories:"), static_cast<ObjcBool>(0));
    mac_msg_send<void>(panel, sel_registerName("setAllowsMultipleSelection:"), static_cast<ObjcBool>(0));

    const char* home = std::getenv("HOME");
    if (home && *home) {
        std::string wsjtx_dir = std::string(home) + "/Library/Application Support/WSJT-X";
        id dir_url = mac_cls_send<id>("NSURL", sel_registerName("fileURLWithPath:"), mac_ns_str(wsjtx_dir.c_str()));
        if (dir_url) {
            mac_msg_send<void>(panel, sel_registerName("setDirectoryURL:"), dir_url);
        }
    }

    long resp = mac_msg_send<long>(panel, sel_registerName("runModal"));
    if (resp != 1) return "";
    id url = mac_msg_send<id>(panel, sel_registerName("URL"));
    if (!url) return "";
    id path_ns = mac_msg_send<id>(url, sel_registerName("path"));
    return mac_ns_to_std_str(path_ns);
}

// Native macOS NSOpenPanel to select a file attachment for Winlink Email
inline std::string pick_attachment_file_macos() {
    id panel = mac_cls_send<id>("NSOpenPanel", sel_registerName("openPanel"));
    if (!panel) return "";
    mac_msg_send<void>(panel, sel_registerName("setTitle:"), mac_ns_str("Select Winlink File Attachment"));
    mac_msg_send<void>(panel, sel_registerName("setCanChooseFiles:"), static_cast<ObjcBool>(1));
    mac_msg_send<void>(panel, sel_registerName("setCanChooseDirectories:"), static_cast<ObjcBool>(0));
    mac_msg_send<void>(panel, sel_registerName("setAllowsMultipleSelection:"), static_cast<ObjcBool>(0));

    long resp = mac_msg_send<long>(panel, sel_registerName("runModal"));
    if (resp != 1) return "";
    id url = mac_msg_send<id>(panel, sel_registerName("URL"));
    if (!url) return "";
    id path_ns = mac_msg_send<id>(url, sel_registerName("path"));
    return mac_ns_to_std_str(path_ns);
}

#elif defined(_WIN32)

// ============================================================================
// Minimal Official MIDL v1 COM Interfaces for Microsoft Edge WebView2
// Dynamically binds to installed EdgeWebView2 runtime with zero external DLLs
// ============================================================================
namespace wv2_com {

struct EventRegistrationToken { int64_t value; };

static const IID kIID_EnvCompletedHandler  = {0x4e8a3389, 0xc9d8, 0x4bd2, {0xb6, 0xb5, 0x12, 0x4f, 0xee, 0x6c, 0xc1, 0x4d}};
static const IID kIID_CtrlCompletedHandler = {0x6c4819f3, 0xc9b7, 0x4260, {0x81, 0x27, 0xc9, 0xf5, 0xbd, 0xe7, 0xf6, 0x8c}};
static const IID kIID_NewWinHandler        = {0xd4c185fe, 0xc81c, 0x4989, {0x97, 0xaf, 0x2d, 0x3f, 0xa7, 0xab, 0x56, 0x51}};

struct ICoreWebView2;
struct ICoreWebView2Controller;
struct ICoreWebView2Environment;
struct ICoreWebView2NewWindowRequestedEventArgs;
struct ICoreWebView2NewWindowRequestedEventHandler;
struct ICoreWebView2CreateCoreWebView2ControllerCompletedHandler;
struct ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandler;

MIDL_INTERFACE("34acb11c-fc37-4418-9132-f9c21d1eafb9")
ICoreWebView2NewWindowRequestedEventArgs : public IUnknown {
public:
    virtual HRESULT STDMETHODCALLTYPE get_Uri(LPWSTR* uri) = 0;
    virtual HRESULT STDMETHODCALLTYPE put_NewWindow(ICoreWebView2* newWindow) = 0;
    virtual HRESULT STDMETHODCALLTYPE get_NewWindow(ICoreWebView2** newWindow) = 0;
    virtual HRESULT STDMETHODCALLTYPE put_Handled(BOOL handled) = 0;
    virtual HRESULT STDMETHODCALLTYPE get_Handled(BOOL* handled) = 0;
    virtual HRESULT STDMETHODCALLTYPE get_IsUserInitiated(BOOL* isUserInitiated) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetDeferral(IUnknown** deferral) = 0;
    virtual HRESULT STDMETHODCALLTYPE get_WindowFeatures(IUnknown** value) = 0;
};

MIDL_INTERFACE("d4c185fe-c81c-4989-97af-2d3fa7ab5651")
ICoreWebView2NewWindowRequestedEventHandler : public IUnknown {
public:
    virtual HRESULT STDMETHODCALLTYPE Invoke(ICoreWebView2* sender, ICoreWebView2NewWindowRequestedEventArgs* args) = 0;
};

MIDL_INTERFACE("76eceacb-0462-4d94-ac83-423a6793775e")
ICoreWebView2 : public IUnknown {
public:
    virtual HRESULT STDMETHODCALLTYPE get_Settings(IUnknown** settings) = 0;
    virtual HRESULT STDMETHODCALLTYPE get_Source(LPWSTR* uri) = 0;
    virtual HRESULT STDMETHODCALLTYPE Navigate(LPCWSTR uri) = 0;
    virtual HRESULT STDMETHODCALLTYPE NavigateToString(LPCWSTR htmlContent) = 0;
    virtual HRESULT STDMETHODCALLTYPE add_NavigationStarting(IUnknown* eventHandler, EventRegistrationToken* token) = 0;
    virtual HRESULT STDMETHODCALLTYPE remove_NavigationStarting(EventRegistrationToken token) = 0;
    virtual HRESULT STDMETHODCALLTYPE add_ContentLoading(IUnknown* eventHandler, EventRegistrationToken* token) = 0;
    virtual HRESULT STDMETHODCALLTYPE remove_ContentLoading(EventRegistrationToken token) = 0;
    virtual HRESULT STDMETHODCALLTYPE add_SourceChanged(IUnknown* eventHandler, EventRegistrationToken* token) = 0;
    virtual HRESULT STDMETHODCALLTYPE remove_SourceChanged(EventRegistrationToken token) = 0;
    virtual HRESULT STDMETHODCALLTYPE add_HistoryChanged(IUnknown* eventHandler, EventRegistrationToken* token) = 0;
    virtual HRESULT STDMETHODCALLTYPE remove_HistoryChanged(EventRegistrationToken token) = 0;
    virtual HRESULT STDMETHODCALLTYPE add_NavigationCompleted(IUnknown* eventHandler, EventRegistrationToken* token) = 0;
    virtual HRESULT STDMETHODCALLTYPE remove_NavigationCompleted(EventRegistrationToken token) = 0;
    virtual HRESULT STDMETHODCALLTYPE add_FrameNavigationStarting(IUnknown* eventHandler, EventRegistrationToken* token) = 0;
    virtual HRESULT STDMETHODCALLTYPE remove_FrameNavigationStarting(EventRegistrationToken token) = 0;
    virtual HRESULT STDMETHODCALLTYPE add_FrameNavigationCompleted(IUnknown* eventHandler, EventRegistrationToken* token) = 0;
    virtual HRESULT STDMETHODCALLTYPE remove_FrameNavigationCompleted(EventRegistrationToken token) = 0;
    virtual HRESULT STDMETHODCALLTYPE add_ScriptDialogOpening(IUnknown* eventHandler, EventRegistrationToken* token) = 0;
    virtual HRESULT STDMETHODCALLTYPE remove_ScriptDialogOpening(EventRegistrationToken token) = 0;
    virtual HRESULT STDMETHODCALLTYPE add_PermissionRequested(IUnknown* eventHandler, EventRegistrationToken* token) = 0;
    virtual HRESULT STDMETHODCALLTYPE remove_PermissionRequested(EventRegistrationToken token) = 0;
    virtual HRESULT STDMETHODCALLTYPE add_ProcessFailed(IUnknown* eventHandler, EventRegistrationToken* token) = 0;
    virtual HRESULT STDMETHODCALLTYPE remove_ProcessFailed(EventRegistrationToken token) = 0;
    virtual HRESULT STDMETHODCALLTYPE AddScriptToExecuteOnDocumentCreated(LPCWSTR javaScript, IUnknown* handler) = 0;
    virtual HRESULT STDMETHODCALLTYPE RemoveScriptToExecuteOnDocumentCreated(LPCWSTR id) = 0;
    virtual HRESULT STDMETHODCALLTYPE ExecuteScript(LPCWSTR javaScript, IUnknown* handler) = 0;
    virtual HRESULT STDMETHODCALLTYPE CapturePreview(int imageFormat, IUnknown* imageStream, IUnknown* handler) = 0;
    virtual HRESULT STDMETHODCALLTYPE Reload(void) = 0;
    virtual HRESULT STDMETHODCALLTYPE PostWebMessageAsJson(LPCWSTR webMessageAsJson) = 0;
    virtual HRESULT STDMETHODCALLTYPE PostWebMessageAsString(LPCWSTR webMessageAsString) = 0;
    virtual HRESULT STDMETHODCALLTYPE add_WebMessageReceived(IUnknown* handler, EventRegistrationToken* token) = 0;
    virtual HRESULT STDMETHODCALLTYPE remove_WebMessageReceived(EventRegistrationToken token) = 0;
    virtual HRESULT STDMETHODCALLTYPE CallDevToolsProtocolMethod(LPCWSTR methodName, LPCWSTR parametersAsJson, IUnknown* handler) = 0;
    virtual HRESULT STDMETHODCALLTYPE get_BrowserProcessId(UINT32* value) = 0;
    virtual HRESULT STDMETHODCALLTYPE get_CanGoBack(BOOL* canGoBack) = 0;
    virtual HRESULT STDMETHODCALLTYPE get_CanGoForward(BOOL* canGoForward) = 0;
    virtual HRESULT STDMETHODCALLTYPE GoBack(void) = 0;
    virtual HRESULT STDMETHODCALLTYPE GoForward(void) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetDevToolsProtocolEventReceiver(LPCWSTR eventName, IUnknown** receiver) = 0;
    virtual HRESULT STDMETHODCALLTYPE Stop(void) = 0;
    virtual HRESULT STDMETHODCALLTYPE add_NewWindowRequested(ICoreWebView2NewWindowRequestedEventHandler* eventHandler, EventRegistrationToken* token) = 0;
    virtual HRESULT STDMETHODCALLTYPE remove_NewWindowRequested(EventRegistrationToken token) = 0;
    virtual HRESULT STDMETHODCALLTYPE add_DocumentTitleChanged(IUnknown* eventHandler, EventRegistrationToken* token) = 0;
    virtual HRESULT STDMETHODCALLTYPE remove_DocumentTitleChanged(EventRegistrationToken token) = 0;
    virtual HRESULT STDMETHODCALLTYPE get_DocumentTitle(LPWSTR* title) = 0;
    virtual HRESULT STDMETHODCALLTYPE AddHostObjectToScript(LPCWSTR name, VARIANT* object) = 0;
    virtual HRESULT STDMETHODCALLTYPE RemoveHostObjectFromScript(LPCWSTR name) = 0;
    virtual HRESULT STDMETHODCALLTYPE OpenDevToolsWindow(void) = 0;
    virtual HRESULT STDMETHODCALLTYPE add_ContainsFullScreenElementChanged(IUnknown* eventHandler, EventRegistrationToken* token) = 0;
    virtual HRESULT STDMETHODCALLTYPE remove_ContainsFullScreenElementChanged(EventRegistrationToken token) = 0;
    virtual HRESULT STDMETHODCALLTYPE get_ContainsFullScreenElement(BOOL* containsFullScreenElement) = 0;
    virtual HRESULT STDMETHODCALLTYPE add_WebResourceRequested(IUnknown* eventHandler, EventRegistrationToken* token) = 0;
    virtual HRESULT STDMETHODCALLTYPE remove_WebResourceRequested(EventRegistrationToken token) = 0;
    virtual HRESULT STDMETHODCALLTYPE AddWebResourceRequestedFilter(LPCWSTR uri, int resourceContext) = 0;
    virtual HRESULT STDMETHODCALLTYPE RemoveWebResourceRequestedFilter(LPCWSTR uri, int resourceContext) = 0;
    virtual HRESULT STDMETHODCALLTYPE add_WindowCloseRequested(IUnknown* eventHandler, EventRegistrationToken* token) = 0;
    virtual HRESULT STDMETHODCALLTYPE remove_WindowCloseRequested(EventRegistrationToken token) = 0;
};

MIDL_INTERFACE("4d00c0d1-9434-4eb6-8078-8697a560334f")
ICoreWebView2Controller : public IUnknown {
public:
    virtual HRESULT STDMETHODCALLTYPE get_IsVisible(BOOL* isVisible) = 0;
    virtual HRESULT STDMETHODCALLTYPE put_IsVisible(BOOL isVisible) = 0;
    virtual HRESULT STDMETHODCALLTYPE get_Bounds(RECT* bounds) = 0;
    virtual HRESULT STDMETHODCALLTYPE put_Bounds(RECT bounds) = 0;
    virtual HRESULT STDMETHODCALLTYPE get_ZoomFactor(double* zoomFactor) = 0;
    virtual HRESULT STDMETHODCALLTYPE put_ZoomFactor(double zoomFactor) = 0;
    virtual HRESULT STDMETHODCALLTYPE add_ZoomFactorChanged(IUnknown* eventHandler, EventRegistrationToken* token) = 0;
    virtual HRESULT STDMETHODCALLTYPE remove_ZoomFactorChanged(EventRegistrationToken token) = 0;
    virtual HRESULT STDMETHODCALLTYPE SetBoundsAndZoomFactor(RECT bounds, double zoomFactor) = 0;
    virtual HRESULT STDMETHODCALLTYPE MoveFocus(int reason) = 0;
    virtual HRESULT STDMETHODCALLTYPE add_MoveFocusRequested(IUnknown* eventHandler, EventRegistrationToken* token) = 0;
    virtual HRESULT STDMETHODCALLTYPE remove_MoveFocusRequested(EventRegistrationToken token) = 0;
    virtual HRESULT STDMETHODCALLTYPE add_GotFocus(IUnknown* eventHandler, EventRegistrationToken* token) = 0;
    virtual HRESULT STDMETHODCALLTYPE remove_GotFocus(EventRegistrationToken token) = 0;
    virtual HRESULT STDMETHODCALLTYPE add_LostFocus(IUnknown* eventHandler, EventRegistrationToken* token) = 0;
    virtual HRESULT STDMETHODCALLTYPE remove_LostFocus(EventRegistrationToken token) = 0;
    virtual HRESULT STDMETHODCALLTYPE add_AcceleratorKeyPressed(IUnknown* eventHandler, EventRegistrationToken* token) = 0;
    virtual HRESULT STDMETHODCALLTYPE remove_AcceleratorKeyPressed(EventRegistrationToken token) = 0;
    virtual HRESULT STDMETHODCALLTYPE get_ParentWindow(HWND* parentWindow) = 0;
    virtual HRESULT STDMETHODCALLTYPE put_ParentWindow(HWND parentWindow) = 0;
    virtual HRESULT STDMETHODCALLTYPE NotifyParentWindowPositionChanged(void) = 0;
    virtual HRESULT STDMETHODCALLTYPE Close(void) = 0;
    virtual HRESULT STDMETHODCALLTYPE get_CoreWebView2(ICoreWebView2** coreWebView2) = 0;
};

MIDL_INTERFACE("6c4819f3-c9b7-4260-8127-c9f5bde7f68c")
ICoreWebView2CreateCoreWebView2ControllerCompletedHandler : public IUnknown {
public:
    virtual HRESULT STDMETHODCALLTYPE Invoke(HRESULT errorCode, ICoreWebView2Controller* result) = 0;
};

MIDL_INTERFACE("b96d755e-0319-4e92-a296-23436f46a1fc")
ICoreWebView2Environment : public IUnknown {
public:
    virtual HRESULT STDMETHODCALLTYPE CreateCoreWebView2Controller(HWND parentWindow, ICoreWebView2CreateCoreWebView2ControllerCompletedHandler* handler) = 0;
    virtual HRESULT STDMETHODCALLTYPE CreateWebResourceResponse(IUnknown* content, int statusCode, LPCWSTR reasonPhrase, LPCWSTR headers, IUnknown** response) = 0;
    virtual HRESULT STDMETHODCALLTYPE get_BrowserVersionString(LPWSTR* versionInfo) = 0;
    virtual HRESULT STDMETHODCALLTYPE add_NewBrowserVersionAvailable(IUnknown* eventHandler, EventRegistrationToken* token) = 0;
    virtual HRESULT STDMETHODCALLTYPE remove_NewBrowserVersionAvailable(EventRegistrationToken token) = 0;
};

MIDL_INTERFACE("4e8a3389-c9d8-4bd2-b6b5-124fee6cc14d")
ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandler : public IUnknown {
public:
    virtual HRESULT STDMETHODCALLTYPE Invoke(HRESULT errorCode, ICoreWebView2Environment* result) = 0;
};

typedef HRESULT (STDAPICALLTYPE *PFN_CreateWebViewEnvInternal)(
    bool checkRunningInstance, int runtimeType, PCWSTR userDataFolder,
    IUnknown* environmentOptions, ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandler* handler);

inline std::wstring utf8_to_wstr(const std::string& s) {
    if (s.empty()) return {};
    int len = MultiByteToWideChar(CP_UTF8, 0, s.c_str(), static_cast<int>(s.size()), nullptr, 0);
    if (len <= 0) return {};
    std::wstring w(static_cast<size_t>(len), L'\0');
    MultiByteToWideChar(CP_UTF8, 0, s.c_str(), static_cast<int>(s.size()), w.data(), len);
    return w;
}

inline std::string wstr_to_utf8(const wchar_t* ws) {
    if (!ws || !*ws) return {};
    int len = WideCharToMultiByte(CP_UTF8, 0, ws, -1, nullptr, 0, nullptr, nullptr);
    if (len <= 1) return {};
    std::string s(static_cast<size_t>(len - 1), '\0');
    WideCharToMultiByte(CP_UTF8, 0, ws, -1, s.data(), len, nullptr, nullptr);
    return s;
}

inline std::wstring locate_embedded_browser_dll() {
    auto probe_client_key = [](HKEY root, const wchar_t* subkey) -> std::wstring {
        HKEY hk = nullptr;
        if (RegOpenKeyExW(root, subkey, 0, KEY_READ, &hk) != ERROR_SUCCESS) return {};
        wchar_t pv[128] = {0};
        wchar_t loc[MAX_PATH] = {0};
        DWORD pv_sz = sizeof(pv), loc_sz = sizeof(loc), type = 0;
        bool ok = (RegQueryValueExW(hk, L"pv", nullptr, &type, reinterpret_cast<LPBYTE>(pv), &pv_sz) == ERROR_SUCCESS) &&
                  (RegQueryValueExW(hk, L"location", nullptr, &type, reinterpret_cast<LPBYTE>(loc), &loc_sz) == ERROR_SUCCESS);
        RegCloseKey(hk);
        if (!ok || !pv[0] || !loc[0]) return {};
        std::filesystem::path p = std::filesystem::path(loc) / pv / L"EBWebView" / L"x64" / L"EmbeddedBrowserWebView.dll";
        std::error_code ec;
        return std::filesystem::exists(p, ec) ? p.wstring() : std::wstring{};
    };

    const wchar_t* kKeyWow = L"SOFTWARE\\WOW6432Node\\Microsoft\\EdgeUpdate\\Clients\\{F3017226-FE2A-4295-8BDF-00C3A9A7E4C5}";
    const wchar_t* kKeyStd = L"SOFTWARE\\Microsoft\\EdgeUpdate\\Clients\\{F3017226-FE2A-4295-8BDF-00C3A9A7E4C5}";
    for (auto [r, k] : {std::pair{HKEY_LOCAL_MACHINE, kKeyWow}, {HKEY_CURRENT_USER, kKeyStd}, {HKEY_LOCAL_MACHINE, kKeyStd}}) {
        std::wstring found = probe_client_key(r, k);
        if (!found.empty()) return found;
    }

    // Fallback scan in standard Program Files (x86)\Microsoft\EdgeWebView\Application
    std::filesystem::path base = L"C:\\Program Files (x86)\\Microsoft\\EdgeWebView\\Application";
    std::error_code ec;
    if (std::filesystem::exists(base, ec)) {
        std::wstring best;
        for (const auto& entry : std::filesystem::directory_iterator(base, ec)) {
            auto cand = entry.path() / L"EBWebView" / L"x64" / L"EmbeddedBrowserWebView.dll";
            if (std::filesystem::exists(cand, ec)) best = cand.wstring();
        }
        if (!best.empty()) return best;
    }
    return {};
}

// Redirect target="_blank" popups into the same WebView2 tab
struct NewWinSameTabHandler : public ICoreWebView2NewWindowRequestedEventHandler {
    ULONG ref_ = 1;
    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** ppv) override {
        if (riid == IID_IUnknown || riid == kIID_NewWinHandler) { *ppv = this; AddRef(); return S_OK; }
        *ppv = nullptr; return E_NOINTERFACE;
    }
    ULONG STDMETHODCALLTYPE AddRef() override { return ++ref_; }
    ULONG STDMETHODCALLTYPE Release() override { ULONG r = --ref_; if (!r) delete this; return r; }
    HRESULT STDMETHODCALLTYPE Invoke(ICoreWebView2* sender, ICoreWebView2NewWindowRequestedEventArgs* args) override {
        if (sender && args) {
            LPWSTR uri = nullptr;
            if (SUCCEEDED(args->get_Uri(&uri)) && uri) {
                args->put_Handled(TRUE);
                sender->Navigate(uri);
                CoTaskMemFree(uri);
            }
        }
        return S_OK;
    }
};

} // namespace wv2_com

class PersistentWebMapsManager {
public:
    std::array<wv2_com::ICoreWebView2Controller*, 5> controllers{nullptr, nullptr, nullptr, nullptr, nullptr};
    std::array<wv2_com::ICoreWebView2*, 5> web_views{nullptr, nullptr, nullptr, nullptr, nullptr};
    std::array<bool, 5> creating{false, false, false, false, false};
    std::array<std::string, 5> fallback_urls;

    wv2_com::ICoreWebView2Environment* env_ = nullptr;
    bool env_creating_ = false;
    bool env_failed_ = false;
    HMODULE wv2_mod_ = nullptr;
    HWND parent_hwnd_ = nullptr;
    int current_visible_idx = -1;
    RECT target_bounds_{0, 0, 100, 100};

    PersistentWebMapsManager() {
        auto& cfg = hamshack::get_app_config();
        for (int i = 0; i < 5; ++i) {
            fallback_urls[i] = !cfg.bookmarked_urls[i].empty() ? cfg.bookmarked_urls[i] : kWebMapHomeUrls[i];
        }
    }

    bool has_native_webview() const {
        return !env_failed_ && (env_ != nullptr || env_creating_ || !wv2_com::locate_embedded_browser_dll().empty());
    }

    void ensure_environment() {
        if (env_ || env_creating_ || env_failed_) return;
        std::wstring dll_path = wv2_com::locate_embedded_browser_dll();
        if (dll_path.empty()) {
            env_failed_ = true;
            return;
        }
        wv2_mod_ = LoadLibraryW(dll_path.c_str());
        if (!wv2_mod_) {
            env_failed_ = true;
            return;
        }
        auto fn = reinterpret_cast<wv2_com::PFN_CreateWebViewEnvInternal>(
            GetProcAddress(wv2_mod_, "CreateWebViewEnvironmentWithOptionsInternal"));
        if (!fn) {
            env_failed_ = true;
            return;
        }

        std::string data_dir = hamshack::AppConfig::get_config_dir() + "\\WebView2Data";
        hamshack::hs_mkdir_p(data_dir);
        std::wstring w_data_dir = wv2_com::utf8_to_wstr(data_dir);

        struct EnvCb : public wv2_com::ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandler {
            ULONG ref_ = 1;
            PersistentWebMapsManager* mgr_;
            explicit EnvCb(PersistentWebMapsManager* m) : mgr_(m) {}
            HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** ppv) override {
                if (riid == IID_IUnknown || riid == wv2_com::kIID_EnvCompletedHandler) { *ppv = this; AddRef(); return S_OK; }
                *ppv = nullptr; return E_NOINTERFACE;
            }
            ULONG STDMETHODCALLTYPE AddRef() override { return ++ref_; }
            ULONG STDMETHODCALLTYPE Release() override { ULONG r = --ref_; if (!r) delete this; return r; }
            HRESULT STDMETHODCALLTYPE Invoke(HRESULT hr, wv2_com::ICoreWebView2Environment* result) override {
                mgr_->env_creating_ = false;
                if (SUCCEEDED(hr) && result) {
                    result->AddRef();
                    mgr_->env_ = result;
                    if (mgr_->current_visible_idx >= 0 && mgr_->current_visible_idx < 5 && mgr_->parent_hwnd_) {
                        mgr_->ensure_web_view(mgr_->current_visible_idx);
                    }
                } else {
                    mgr_->env_failed_ = true;
                }
                return S_OK;
            }
        };

        env_creating_ = true;
        auto* cb = new EnvCb(this);
        HRESULT hr = fn(true, 0, w_data_dir.c_str(), nullptr, cb);
        cb->Release();
        if (FAILED(hr)) {
            env_creating_ = false;
            env_failed_ = true;
        }
    }

    void ensure_web_view(int idx) {
        if (idx < 0 || idx >= 5 || !parent_hwnd_) return;
        if (web_views[idx] || creating[idx]) return;
        ensure_environment();
        if (!env_) return;

        struct CtrlCb : public wv2_com::ICoreWebView2CreateCoreWebView2ControllerCompletedHandler {
            ULONG ref_ = 1;
            PersistentWebMapsManager* mgr_;
            int idx_;
            CtrlCb(PersistentWebMapsManager* m, int i) : mgr_(m), idx_(i) {}
            HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** ppv) override {
                if (riid == IID_IUnknown || riid == wv2_com::kIID_CtrlCompletedHandler) { *ppv = this; AddRef(); return S_OK; }
                *ppv = nullptr; return E_NOINTERFACE;
            }
            ULONG STDMETHODCALLTYPE AddRef() override { return ++ref_; }
            ULONG STDMETHODCALLTYPE Release() override { ULONG r = --ref_; if (!r) delete this; return r; }
            HRESULT STDMETHODCALLTYPE Invoke(HRESULT hr, wv2_com::ICoreWebView2Controller* ctrl) override {
                mgr_->creating[idx_] = false;
                if (FAILED(hr) || !ctrl) return S_OK;
                ctrl->AddRef();
                mgr_->controllers[idx_] = ctrl;

                wv2_com::ICoreWebView2* wv = nullptr;
                if (SUCCEEDED(ctrl->get_CoreWebView2(&wv)) && wv) {
                    mgr_->web_views[idx_] = wv;
                    wv2_com::EventRegistrationToken tok{};
                    auto* nw = new wv2_com::NewWinSameTabHandler();
                    wv->add_NewWindowRequested(nw, &tok);
                    nw->Release();

                    auto& cfg = hamshack::get_app_config();
                    double z = (idx_ < static_cast<int>(cfg.page_zoom_levels.size())) ? cfg.page_zoom_levels[idx_] : 1.0;
                    if (z < 0.3 || z > 3.5) z = 1.0;
                    ctrl->put_ZoomFactor(z);

                    std::string init_u = mgr_->fallback_urls[idx_];
                    if (init_u.empty()) init_u = kWebMapHomeUrls[idx_];
                    std::wstring wu = wv2_com::utf8_to_wstr(init_u);
                    wv->Navigate(wu.c_str());
                }

                bool vis = (mgr_->current_visible_idx == idx_);
                ctrl->put_Bounds(mgr_->target_bounds_);
                ctrl->put_IsVisible(vis ? TRUE : FALSE);
                return S_OK;
            }
        };

        creating[idx] = true;
        auto* cb = new CtrlCb(this, idx);
        HRESULT hr = env_->CreateCoreWebView2Controller(parent_hwnd_, cb);
        cb->Release();
        if (FAILED(hr)) {
            creating[idx] = false;
        }
    }

    void zoom_in(int idx) {
        if (idx < 0 || idx >= 5) return;
        auto& app_cfg = hamshack::get_app_config();
        double z = std::min(3.0, app_cfg.page_zoom_levels[idx] * 1.15);
        set_zoom(idx, z);
    }

    void zoom_out(int idx) {
        if (idx < 0 || idx >= 5) return;
        auto& app_cfg = hamshack::get_app_config();
        double z = std::max(0.4, app_cfg.page_zoom_levels[idx] / 1.15);
        set_zoom(idx, z);
    }

    void zoom_reset(int idx) {
        set_zoom(idx, 1.0);
    }

    void set_zoom(int idx, double z) {
        if (idx < 0 || idx >= 5) return;
        auto& app_cfg = hamshack::get_app_config();
        app_cfg.page_zoom_levels[idx] = z;
        app_cfg.save();
        if (controllers[idx]) {
            controllers[idx]->put_ZoomFactor(z);
        }
    }

    double get_zoom(int idx) const {
        if (idx < 0 || idx >= 5) return 1.0;
        auto& app_cfg = hamshack::get_app_config();
        return app_cfg.page_zoom_levels[idx];
    }

    void load_url(int idx, const char* url_str) {
        if (idx < 0 || idx >= 5 || !url_str) return;
        fallback_urls[idx] = url_str;
        if (web_views[idx]) {
            std::wstring wu = wv2_com::utf8_to_wstr(url_str);
            web_views[idx]->Navigate(wu.c_str());
        }
    }

    void bookmark_current_page(int idx) {
        if (idx < 0 || idx >= 5) return;
        std::string cur = current_url(idx);
        if (!cur.empty()) {
            auto& app_cfg = hamshack::get_app_config();
            app_cfg.bookmarked_urls[idx] = cur;
            app_cfg.save();
        }
    }

    void go_back(int idx) {
        if (idx >= 0 && idx < 5 && web_views[idx]) {
            web_views[idx]->GoBack();
        }
    }

    void go_forward(int idx) {
        if (idx >= 0 && idx < 5 && web_views[idx]) {
            web_views[idx]->GoForward();
        }
    }

    void reload(int idx) {
        if (idx >= 0 && idx < 5 && web_views[idx]) {
            web_views[idx]->Reload();
        }
    }

    void open_in_external_browser(int idx) {
        std::string cur = current_url(idx);
        if (cur.empty() && idx >= 0 && idx < 5) cur = kWebMapHomeUrls[idx];
        ShellExecuteA(nullptr, "open", cur.c_str(), nullptr, nullptr, SW_SHOWNORMAL);
    }

    std::string current_url(int idx) const {
        if (idx < 0 || idx >= 5) return "";
        if (web_views[idx]) {
            LPWSTR uri = nullptr;
            if (SUCCEEDED(web_views[idx]->get_Source(&uri)) && uri) {
                std::string s = wv2_com::wstr_to_utf8(uri);
                CoTaskMemFree(uri);
                if (!s.empty() && s != "about:blank") return s;
            }
        }
        if (!fallback_urls[idx].empty()) return fallback_urls[idx];
        auto& app_cfg = hamshack::get_app_config();
        if (!app_cfg.bookmarked_urls[idx].empty()) return app_cfg.bookmarked_urls[idx];
        return kWebMapHomeUrls[idx];
    }

    bool is_loading(int idx) const {
        return (idx >= 0 && idx < 5) ? creating[idx] : false;
    }

    void sync_visibility(void* parent_hwnd_ptr, int active_web_idx, float x, float y, float w, float h) {
        HWND hwnd = reinterpret_cast<HWND>(parent_hwnd_ptr);
        if (!hwnd) return;
        parent_hwnd_ = hwnd;

        RECT r{};
        r.left   = static_cast<LONG>(std::round(x));
        r.top    = static_cast<LONG>(std::round(y));
        r.right  = static_cast<LONG>(std::round(x + std::max(100.0f, w)));
        r.bottom = static_cast<LONG>(std::round(y + std::max(100.0f, h)));
        target_bounds_ = r;
        current_visible_idx = active_web_idx;

        if (active_web_idx >= 0 && active_web_idx < 5) {
            ensure_web_view(active_web_idx);
        }

        for (int i = 0; i < 5; ++i) {
            if (!controllers[i]) continue;
            bool should_show = (i == active_web_idx);
            BOOL cur_vis = FALSE;
            controllers[i]->get_IsVisible(&cur_vis);
            if (should_show) {
                controllers[i]->put_Bounds(r);
                if (!cur_vis) controllers[i]->put_IsVisible(TRUE);
            } else {
                if (cur_vis) controllers[i]->put_IsVisible(FALSE);
            }
        }
    }

    void hide_all() {
        if (current_visible_idx == -1) return;
        for (int i = 0; i < 5; ++i) {
            if (controllers[i]) {
                controllers[i]->put_IsVisible(FALSE);
            }
        }
        current_visible_idx = -1;
    }
};

inline PersistentWebMapsManager& get_web_maps() {
    static PersistentWebMapsManager* s_mgr = new PersistentWebMapsManager();
    return *s_mgr;
}

inline std::string pick_adif_file_macos() {
    get_web_maps().hide_all();
    wchar_t file_buf[MAX_PATH] = {0};
    std::wstring init_dir;
    if (const char* local_app = std::getenv("LOCALAPPDATA"); local_app && *local_app) {
        std::filesystem::path p = std::filesystem::path(local_app) / "WSJT-X";
        std::error_code ec;
        if (std::filesystem::exists(p, ec)) init_dir = p.wstring();
    }

    OPENFILENAMEW ofn{};
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner   = GetActiveWindow();
    ofn.lpstrFilter = L"ADIF Log Files (*.adi;*.adif)\0*.adi;*.adif\0All Files (*.*)\0*.*\0";
    ofn.lpstrFile   = file_buf;
    ofn.nMaxFile    = MAX_PATH;
    ofn.lpstrInitialDir = init_dir.empty() ? nullptr : init_dir.c_str();
    ofn.lpstrTitle  = L"Import WSJT-X / GridTracker ADIF (.adi / .adif) Log";
    ofn.Flags       = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_NOCHANGEDIR;

    if (!GetOpenFileNameW(&ofn)) return "";
    return wv2_com::wstr_to_utf8(file_buf);
}

inline std::string pick_attachment_file_macos() {
    get_web_maps().hide_all();
    wchar_t file_buf[MAX_PATH] = {0};
    OPENFILENAMEW ofn{};
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner   = GetActiveWindow();
    ofn.lpstrFilter = L"All Files (*.*)\0*.*\0";
    ofn.lpstrFile   = file_buf;
    ofn.nMaxFile    = MAX_PATH;
    ofn.lpstrTitle  = L"Select Winlink File Attachment";
    ofn.Flags       = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_NOCHANGEDIR;

    if (!GetOpenFileNameW(&ofn)) return "";
    return wv2_com::wstr_to_utf8(file_buf);
}

#else

class PersistentWebMapsManager {
public:
    void zoom_in(int) {}
    void zoom_out(int) {}
    void zoom_reset(int) {}
    void set_zoom(int, double) {}
    double get_zoom(int) const { return 1.0; }
    void bookmark_current_page(int) {}
    void load_url(int, const char*) {}
    void go_back(int) {}
    void go_forward(int) {}
    void reload(int) {}
    void open_in_external_browser(int) {}
    std::string current_url(int idx) const { return (idx >= 0 && idx < 5) ? kWebMapHomeUrls[idx] : ""; }
    bool is_loading(int) const { return false; }
    void sync_visibility(void*, int, float, float, float, float) {}
    void hide_all() {}
    bool has_native_webview() const { return false; }
};

inline PersistentWebMapsManager& get_web_maps() {
    static PersistentWebMapsManager s_mgr;
    return s_mgr;
}

inline std::string pick_adif_file_macos() { return ""; }
inline std::string pick_attachment_file_macos() { return ""; }

#endif

} // namespace hamshack_web
