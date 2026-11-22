#include "platform/input_bridge.h"

#include <mmsystem.h>

#include <iterator>
#include <string_view>

#include "core/process_match.h"
#include "core/raw_keys.h"

#pragma comment(lib, "winmm.lib")

namespace pskb {
namespace {

constexpr wchar_t kWindowClass[] = L"PS5KeyboardBridge.InputBridge";
constexpr UINT kMsgToggle = WM_APP + 1;
constexpr UINT kMsgDisarm = WM_APP + 2;
constexpr UINT kMsgLinkReady = WM_APP + 3;
constexpr UINT kMsgWake = WM_APP + 4;
constexpr DWORD kTickMs = 4;

double NowMs() {
    static const double frequency = [] {
        LARGE_INTEGER f{};
        QueryPerformanceFrequency(&f);
        return static_cast<double>(f.QuadPart);
    }();
    LARGE_INTEGER counter{};
    QueryPerformanceCounter(&counter);
    return static_cast<double>(counter.QuadPart) * 1000.0 / frequency;
}

struct ButtonMap {
    USHORT down;
    USHORT up;
    InputCode code;
};

constexpr ButtonMap kButtons[] = {
    {RI_MOUSE_LEFT_BUTTON_DOWN, RI_MOUSE_LEFT_BUTTON_UP, kMouseLeft},
    {RI_MOUSE_RIGHT_BUTTON_DOWN, RI_MOUSE_RIGHT_BUTTON_UP, kMouseRight},
    {RI_MOUSE_MIDDLE_BUTTON_DOWN, RI_MOUSE_MIDDLE_BUTTON_UP, kMouseMiddle},
    {RI_MOUSE_BUTTON_4_DOWN, RI_MOUSE_BUTTON_4_UP, kMouseX1},
    {RI_MOUSE_BUTTON_5_DOWN, RI_MOUSE_BUTTON_5_UP, kMouseX2},
};

}  // namespace

InputBridge::InputBridge(const Profile& profile, CaptureOptions options, PadSink sink, ModeListener onMode)
    : options_(std::move(options)),
      sink_(std::move(sink)),
      onMode_(std::move(onMode)),
      engine_(profile),
      session_(engine_, options_.hotkey) {}

InputBridge::~InputBridge() { Stop(); }

bool InputBridge::Start(std::string* error) {
    if (thread_.joinable()) return true;

    std::promise<std::string> ready;
    std::future<std::string> outcome = ready.get_future();
    running_ = true;
    thread_ = std::thread([this, &ready] { Run(ready); });

    const std::string failure = outcome.get();
    if (!failure.empty()) {
        if (thread_.joinable()) thread_.join();
        if (error) *error = failure;
        return false;
    }
    return true;
}

void InputBridge::Stop() {
    if (!thread_.joinable()) return;
    running_ = false;
    if (hwnd_) PostMessageW(hwnd_, kMsgWake, 0, 0);
    thread_.join();
}

void InputBridge::Toggle() { PostToWindow(kMsgToggle, 0); }

void InputBridge::Disarm() { PostToWindow(kMsgDisarm, 0); }

void InputBridge::SetLinkReady(bool ready) { PostToWindow(kMsgLinkReady, ready ? 1 : 0); }

void InputBridge::PostToWindow(UINT msg, WPARAM wParam) const {
    if (hwnd_) PostMessageW(hwnd_, msg, wParam, 0);
}

void InputBridge::Run(std::promise<std::string>& ready) {
    const HINSTANCE instance = GetModuleHandleW(nullptr);

    WNDCLASSW wc{};
    wc.lpfnWndProc = &InputBridge::WndProc;
    wc.hInstance = instance;
    wc.lpszClassName = kWindowClass;
    RegisterClassW(&wc);

    HWND window = CreateWindowExW(0, kWindowClass, L"", 0, 0, 0, 0, 0, HWND_MESSAGE, nullptr, instance, this);
    if (!window) {
        ready.set_value("Não foi possível criar a janela de captura.");
        return;
    }

    RAWINPUTDEVICE devices[2] = {
        {0x01, 0x06, RIDEV_INPUTSINK, window},  // keyboard, observed even when not focused
        {0x01, 0x02, RIDEV_INPUTSINK, window},  // mouse
    };
    if (!RegisterRawInputDevices(devices, 2, sizeof(RAWINPUTDEVICE))) {
        const DWORD code = GetLastError();
        DestroyWindow(window);
        UnregisterClassW(kWindowClass, instance);
        ready.set_value("Não foi possível ler o teclado e o mouse (código " + std::to_string(code) + ").");
        return;
    }

    hwnd_ = window;
    timeBeginPeriod(1);
    ready.set_value("");

    while (running_) {
        MsgWaitForMultipleObjects(0, nullptr, FALSE, kTickMs, QS_ALLINPUT);
        MSG msg{};
        while (PeekMessageW(&msg, nullptr, 0, 0, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
        }
        Tick();
    }

    session_.Disarm();
    const PadState neutral = session_.Tick(NowMs());
    if (sink_) sink_(neutral);
    if (mode_.exchange(CaptureMode::Off) != CaptureMode::Off && onMode_) onMode_(CaptureMode::Off);

    timeEndPeriod(1);
    RAWINPUTDEVICE remove[2] = {
        {0x01, 0x06, RIDEV_REMOVE, nullptr},
        {0x01, 0x02, RIDEV_REMOVE, nullptr},
    };
    RegisterRawInputDevices(remove, 2, sizeof(RAWINPUTDEVICE));
    hwnd_ = nullptr;
    lastForeground_ = nullptr;
    DestroyWindow(window);
    UnregisterClassW(kWindowClass, instance);
}

LRESULT CALLBACK InputBridge::WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    if (msg == WM_NCCREATE) {
        const auto* create = reinterpret_cast<const CREATESTRUCTW*>(lParam);
        SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(create->lpCreateParams));
        return TRUE;
    }

    auto* self = reinterpret_cast<InputBridge*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
    if (self) {
        switch (msg) {
            case WM_INPUT:
                self->OnRawInput(lParam);
                break;
            case kMsgToggle:
                self->session_.Toggle();
                break;
            case kMsgDisarm:
                self->session_.Disarm();
                break;
            case kMsgLinkReady:
                self->session_.SetLinkReady(wParam != 0);
                break;
            default:
                break;
        }
    }
    return DefWindowProcW(hwnd, msg, wParam, lParam);
}

void InputBridge::OnRawInput(LPARAM lParam) {
    const auto handle = reinterpret_cast<HRAWINPUT>(lParam);
    UINT size = 0;
    if (GetRawInputData(handle, RID_INPUT, nullptr, &size, sizeof(RAWINPUTHEADER)) != 0 || size == 0) return;
    rawBuffer_.resize(size);
    if (GetRawInputData(handle, RID_INPUT, rawBuffer_.data(), &size, sizeof(RAWINPUTHEADER)) != size) return;

    const auto* raw = reinterpret_cast<const RAWINPUT*>(rawBuffer_.data());
    const double now = NowMs();

    if (raw->header.dwType == RIM_TYPEKEYBOARD) {
        const RAWKEYBOARD& key = raw->data.keyboard;
        if (key.VKey == 0 || key.VKey == 0xFF) return;
        const bool down = (key.Flags & RI_KEY_BREAK) == 0;
        const bool extended = (key.Flags & RI_KEY_E0) != 0;
        session_.OnKey(KeyCodeFromRaw(key.VKey, key.MakeCode, extended), down);
        return;
    }

    if (raw->header.dwType == RIM_TYPEMOUSE) {
        const RAWMOUSE& mouse = raw->data.mouse;
        const bool relative = (mouse.usFlags & MOUSE_MOVE_ABSOLUTE) == 0;
        if (relative && (mouse.lLastX != 0 || mouse.lLastY != 0)) {
            session_.OnMouseMove(static_cast<int>(mouse.lLastX), static_cast<int>(mouse.lLastY), now);
        }
        for (const auto& button : kButtons) {
            if (mouse.usButtonFlags & button.down) session_.OnMouseButton(button.code, true);
            if (mouse.usButtonFlags & button.up) session_.OnMouseButton(button.code, false);
        }
    }
}

bool InputBridge::IsApprovedWindow(HWND window) const {
    if (!window) return false;
    DWORD pid = 0;
    GetWindowThreadProcessId(window, &pid);
    if (pid == 0) return false;

    HANDLE process = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pid);
    if (!process) return false;

    wchar_t path[32768];
    DWORD length = static_cast<DWORD>(std::size(path));
    const bool ok = QueryFullProcessImageNameW(process, 0, path, &length) != 0;
    CloseHandle(process);
    return ok && ProcessNameMatches(std::wstring_view(path, length), options_.approvedProcess);
}

void InputBridge::Tick() {
    const HWND foreground = GetForegroundWindow();
    if (foreground != lastForeground_) {
        lastForeground_ = foreground;
        session_.SetGameFocused(IsApprovedWindow(foreground));
    }

    const PadState state = session_.Tick(NowMs());
    if (sink_) sink_(state);

    const CaptureMode mode = session_.Mode();
    if (mode != mode_.load()) {
        mode_.store(mode);
        if (onMode_) onMode_(mode);
    }
}

}  // namespace pskb
