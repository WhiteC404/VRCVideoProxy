#include "core.hpp"
#include <commctrl.h>
#include <algorithm>
#include <stdexcept>

namespace vrcvp {
namespace {
constexpr int ProxyEdit = 101, NoProxyEdit = 102, SteamEdit = 103, SaveCopy = 104, Defaults = 105, Close = 106, Status = 107, Cacher = 108;
struct SettingsState {
    std::wstring executable;
    Options options;
    Config initial;
    HFONT font = nullptr;
    int dpi = 96;
};
int Scale(SettingsState* state, int value) { return MulDiv(value, state->dpi, 96); }
HWND Control(HWND parent, SettingsState* state, const wchar_t* type, const wchar_t* text, DWORD style,
             int x, int y, int width, int height, int id = 0, DWORD extended = 0) {
    HWND child = CreateWindowExW(extended, type, text, WS_CHILD | WS_VISIBLE | style,
        Scale(state, x), Scale(state, y), Scale(state, width), Scale(state, height), parent,
        reinterpret_cast<HMENU>(static_cast<INT_PTR>(id)), GetModuleHandleW(nullptr), nullptr);
    if (!child) throw std::runtime_error("Cannot create a settings control.");
    SendMessageW(child, WM_SETFONT, reinterpret_cast<WPARAM>(state->font), TRUE);
    return child;
}
std::wstring Text(HWND window) {
    const int length = GetWindowTextLengthW(window);
    std::vector<wchar_t> value(static_cast<size_t>(length) + 1);
    GetWindowTextW(window, value.data(), static_cast<int>(value.size()));
    return value.data();
}
std::wstring Lines(std::wstring text) {
    std::wstring result;
    for (wchar_t c : text) result += c == L',' ? L"\r\n" : std::wstring(1, c);
    return result;
}
void Clipboard(HWND owner, const std::wstring& text) {
    if (!OpenClipboard(owner)) throw std::runtime_error("Cannot open the clipboard. Please try again.");
    const size_t bytes = (text.size() + 1) * sizeof(wchar_t);
    HGLOBAL block = GlobalAlloc(GMEM_MOVEABLE, bytes);
    if (!block) { CloseClipboard(); throw std::runtime_error("Cannot allocate clipboard data."); }
    void* memory = GlobalLock(block);
    if (!memory) { GlobalFree(block); CloseClipboard(); throw std::runtime_error("Cannot lock clipboard data."); }
    memcpy(memory, text.c_str(), bytes);
    GlobalUnlock(block);
    if (!EmptyClipboard() || !SetClipboardData(CF_UNICODETEXT, block)) {
        GlobalFree(block); CloseClipboard(); throw std::runtime_error("Cannot copy the Steam launch option.");
    }
    CloseClipboard(); // The clipboard now owns block.
}
void Populate(HWND window, const Config& config) {
    SetWindowTextW(GetDlgItem(window, ProxyEdit), config.proxy.c_str());
    SetWindowTextW(GetDlgItem(window, NoProxyEdit), Lines(config.noProxy).c_str());
}
LRESULT CALLBACK Procedure(HWND window, UINT message, WPARAM wParam, LPARAM lParam) {
    auto* state = reinterpret_cast<SettingsState*>(GetWindowLongPtrW(window, GWLP_USERDATA));
    try {
        if (message == WM_NCCREATE) {
            auto* create = reinterpret_cast<CREATESTRUCTW*>(lParam);
            state = reinterpret_cast<SettingsState*>(create->lpCreateParams);
            SetWindowLongPtrW(window, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(state));
        }
        if (message == WM_CREATE) {
            HDC display = GetDC(window);
            state->dpi = GetDeviceCaps(display, LOGPIXELSX);
            ReleaseDC(window, display);
            state->font = CreateFontW(-MulDiv(10, state->dpi, 72), 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH, L"Segoe UI");
            Control(window, state, L"STATIC", L"为 VRChat 视频请求设置代理", 0, 24, 20, 740, 28);
            Control(window, state, L"STATIC", L"代理地址（FLClash / Clash 的 HTTP 或 mixed 端口）", 0, 24, 60, 740, 24);
            HWND proxy = Control(window, state, L"EDIT", L"", WS_TABSTOP | ES_AUTOHSCROLL, 24, 88, 740, 30, ProxyEdit, WS_EX_CLIENTEDGE);
            SendMessageW(proxy, EM_SETLIMITTEXT, 2048, 0);
            Control(window, state, L"STATIC", L"直连域名（每行一个，或用逗号分隔）", 0, 24, 136, 740, 24);
            HWND domains = Control(window, state, L"EDIT", L"", WS_TABSTOP | ES_MULTILINE | ES_AUTOVSCROLL | WS_VSCROLL | ES_WANTRETURN,
                24, 164, 740, 148, NoProxyEdit, WS_EX_CLIENTEDGE);
            SendMessageW(domains, EM_SETLIMITTEXT, 16384, 0);
            Control(window, state, L"STATIC", L"YouTube 等请求使用代理；列表中的域名跳过 HTTP 代理。\r\nTUN 与 UU 仍按各自规则运行，游戏 UDP 的线路由它们处理。", 0, 24, 324, 740, 48);
            Control(window, state, L"STATIC", L"Steam 启动选项", 0, 24, 386, 740, 24);
            Control(window, state, L"EDIT", SteamOption(state->executable, state->options).c_str(), WS_TABSTOP | ES_READONLY | ES_AUTOHSCROLL,
                24, 414, 740, 30, SteamEdit, WS_EX_CLIENTEDGE);
            Control(window, state, L"STATIC", L"保存后，将这一行粘贴到 Steam → VRChat → 属性 → 启动选项。", 0, 24, 454, 740, 24);
            Control(window, state, L"BUTTON", L"保存并复制启动选项", WS_TABSTOP | BS_DEFPUSHBUTTON, 24, 500, 228, 36, SaveCopy);
            Control(window, state, L"BUTTON", L"恢复默认值", WS_TABSTOP | BS_PUSHBUTTON, 272, 500, 146, 36, Defaults);
            Control(window, state, L"BUTTON", L"添加 Cacher 直连域名", WS_TABSTOP | BS_PUSHBUTTON, 438, 500, 180, 36, Cacher);
            Control(window, state, L"BUTTON", L"关闭", WS_TABSTOP | BS_PUSHBUTTON, 638, 500, 126, 36, Close);
            Control(window, state, L"STATIC", L"配置只作用于本次启动的游戏及其子进程。", 0, 24, 552, 740, 34, Status);
            Populate(window, state->initial);
            SetFocus(proxy);
            return 0;
        }
        if (message == WM_COMMAND && state) {
            switch (LOWORD(wParam)) {
            case SaveCopy: {
                Config config{Text(GetDlgItem(window, ProxyEdit)), Text(GetDlgItem(window, NoProxyEdit))};
                Validate(config);
                WriteConfig(state->options.configPath, config);
                Clipboard(window, SteamOption(state->executable, state->options));
                Populate(window, config);
                SetWindowTextW(GetDlgItem(window, Status), L"已保存配置并复制启动选项。可关闭此窗口，回到 Steam 启动 VRChat。");
                return 0;
            }
            case Defaults:
                Populate(window, Config{});
                SetWindowTextW(GetDlgItem(window, Status), L"已填入默认值；点击“保存并复制启动选项”后生效。");
                return 0;
            case Cacher: {
                Config config{Text(GetDlgItem(window, ProxyEdit)), Text(GetDlgItem(window, NoProxyEdit))};
                EnableCacherCompatibility(config);
                SetWindowTextW(GetDlgItem(window, NoProxyEdit), Lines(config.noProxy).c_str());
                SetWindowTextW(GetDlgItem(window, Status), L"已补充本地播放域名；保存后生效。Cacher 也需单独通过本启动器启动。");
                return 0;
            }
            case Close: DestroyWindow(window); return 0;
            default: break;
            }
        }
        if (message == WM_CLOSE) { DestroyWindow(window); return 0; }
        if (message == WM_DESTROY) {
            if (state && state->font) { DeleteObject(state->font); state->font = nullptr; }
            PostQuitMessage(0);
            return 0;
        }
    } catch (const std::exception& error) {
        MessageBoxW(window, Wide(error.what()).c_str(), L"VRC Video Proxy", MB_OK | MB_ICONERROR);
        if (message == WM_CREATE) return -1;
        return 0;
    }
    return DefWindowProcW(window, message, wParam, lParam);
}
}
int Settings(const std::wstring& executable, const Options& options, const Config& initial) {
    INITCOMMONCONTROLSEX controls{sizeof(controls), ICC_STANDARD_CLASSES};
    InitCommonControlsEx(&controls);
    WNDCLASSW kind{};
    kind.lpfnWndProc = Procedure;
    kind.hInstance = GetModuleHandleW(nullptr);
    kind.hCursor = LoadCursorW(nullptr, IDC_ARROW);
    kind.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1);
    kind.lpszClassName = L"VRCVideoProxySettings";
    if (!RegisterClassW(&kind)) throw std::runtime_error("Cannot register the settings window.");
    SettingsState state{executable, options, initial};
    HDC display = GetDC(nullptr);
    const int dpi = GetDeviceCaps(display, LOGPIXELSX);
    ReleaseDC(nullptr, display);
    RECT size{0, 0, MulDiv(788, dpi, 96), MulDiv(604, dpi, 96)};
    const DWORD style = WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX;
    AdjustWindowRectEx(&size, style, FALSE, 0);
    HWND window = CreateWindowExW(WS_EX_CONTROLPARENT, kind.lpszClassName, L"VRC Video Proxy — 设置", style,
        CW_USEDEFAULT, CW_USEDEFAULT, size.right - size.left, size.bottom - size.top, nullptr, nullptr, kind.hInstance, &state);
    if (!window) throw std::runtime_error("Cannot create the settings window.");
    ShowWindow(window, SW_SHOWNORMAL);
    UpdateWindow(window);
    MSG message{};
    while (GetMessageW(&message, nullptr, 0, 0) > 0) {
        if (!IsDialogMessageW(window, &message)) { TranslateMessage(&message); DispatchMessageW(&message); }
    }
    return static_cast<int>(message.wParam);
}
}
