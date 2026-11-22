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

// Low-level hook callbacks receive no user pointer, and the hooks run on the bridge thread.
InputBridge* g_hookOwner = nullptr;

bool KeyHeld(int virtualKey) { return (GetAsyncKeyState(virtualKey) & 0x8000) != 0; }

}  // namespace

InputBridge::InputBridge(const Profile& profile, CaptureOptions options, PadSink sink, ModeListener onMode)
    : options_(std::move(options)),
      sink_(std::move(sink)),
      onMode_(std::move(onMode)),
      engine_(profile),
      session_(engine_, options_.hotkey) {
    for (const Binding& binding : engine_.CurrentProfile().bindings) mapped_.insert(binding.input);
}

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
    g_hookOwner = this;
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
    RemoveSuppression();
    ReleaseCursorLock();
    if (mode_.exchange(CaptureMode::Off) != CaptureMode::Off && onMode_) onMode_(CaptureMode::Off);

    g_hookOwner = nullptr;
    approved_ = false;
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

void InputBridge::RefreshForeground() {
    const HWND foreground = GetForegroundWindow();
    if (foreground == lastForeground_) return;
    lastForeground_ = foreground;
    approved_ = IsApprovedWindow(foreground);
    session_.SetGameFocused(approved_);
}

void InputBridge::Tick() {
    RefreshForeground();

    const PadState state = session_.Tick(NowMs());
    if (sink_) sink_(state);

    SyncSuppression();

    const CaptureMode mode = session_.Mode();
    if (mode != mode_.load()) {
        mode_.store(mode);
        if (onMode_) onMode_(mode);
    }
}

LRESULT CALLBACK InputBridge::KeyboardHookProc(int code, WPARAM wParam, LPARAM lParam) {
    if (code == HC_ACTION && g_hookOwner) {
        const auto* key = reinterpret_cast<const KBDLLHOOKSTRUCT*>(lParam);
        if (g_hookOwner->OnKeyboardHook(*key, wParam)) return 1;
    }
    return CallNextHookEx(nullptr, code, wParam, lParam);
}

LRESULT CALLBACK InputBridge::MouseHookProc(int code, WPARAM wParam, LPARAM lParam) {
    if (code == HC_ACTION && g_hookOwner) {
        const auto* mouse = reinterpret_cast<const MSLLHOOKSTRUCT*>(lParam);
        if (g_hookOwner->OnMouseHook(*mouse, wParam)) return 1;
    }
    return CallNextHookEx(nullptr, code, wParam, lParam);
}

GateEvent InputBridge::MakeGateEvent(InputCode code, bool down, bool keyboard, bool injected) const {
    GateEvent event;
    event.keyboard = keyboard;
    event.down = down;
    event.code = code;
    event.injected = injected;
    event.altHeld = KeyHeld(VK_MENU);
    event.ctrlHeld = KeyHeld(VK_CONTROL);
    event.winHeld = KeyHeld(VK_LWIN) || KeyHeld(VK_RWIN);
    event.reserved = code == options_.hotkey;
    event.mapped = mapped_.count(code) != 0;
    return event;
}

bool InputBridge::OnKeyboardHook(const KBDLLHOOKSTRUCT& key, WPARAM message) {
    const bool down = message == WM_KEYDOWN || message == WM_SYSKEYDOWN;
    const auto code = static_cast<InputCode>(key.vkCode);
    RefreshForeground();

    const CaptureMode mode = session_.Mode();
    const GateEvent event = MakeGateEvent(code, down, true, (key.flags & LLKHF_INJECTED) != 0);
    bool swallow = false;
    if (!down) {
        swallow = swallowed_.Consume(code);
    } else if (ShouldSwallow(event, mode, approved_)) {
        swallowed_.Remember(code);
        swallow = true;
    }

    if (mode == CaptureMode::Active && approved_ && !event.reserved && (event.mapped || code == CaptureSession::kEscape)) {
        session_.OnKey(code, down);
    }
    if (session_.Mode() != CaptureMode::Active) SyncSuppression();
    return swallow;
}

bool InputBridge::OnMouseHook(const MSLLHOOKSTRUCT& mouse, WPARAM message) {
    InputCode code = 0;
    bool down = false;
    bool wheel = false;
    switch (message) {
        case WM_LBUTTONDOWN: code = kMouseLeft; down = true; break;
        case WM_LBUTTONUP: code = kMouseLeft; break;
        case WM_RBUTTONDOWN: code = kMouseRight; down = true; break;
        case WM_RBUTTONUP: code = kMouseRight; break;
        case WM_MBUTTONDOWN: code = kMouseMiddle; down = true; break;
        case WM_MBUTTONUP: code = kMouseMiddle; break;
        case WM_XBUTTONDOWN:
        case WM_XBUTTONUP:
            code = HIWORD(mouse.mouseData) == XBUTTON1 ? kMouseX1 : kMouseX2;
            down = message == WM_XBUTTONDOWN;
            break;
        case WM_MOUSEWHEEL:
        case WM_MOUSEHWHEEL:
            code = kMouseWheel;
            down = true;
            wheel = true;
            break;
        default:
            return false;
    }
    RefreshForeground();

    const CaptureMode mode = session_.Mode();
    const GateEvent event = MakeGateEvent(code, down, false, (mouse.flags & LLMHF_INJECTED) != 0);
    bool swallow = false;
    if (wheel) {
        swallow = ShouldSwallow(event, mode, approved_);
    } else if (!down) {
        swallow = swallowed_.Consume(code);
    } else if (ShouldSwallow(event, mode, approved_)) {
        swallowed_.Remember(code);
        swallow = true;
    }

    if (!wheel && mode == CaptureMode::Active && approved_) session_.OnMouseButton(code, down);
    if (session_.Mode() != CaptureMode::Active) SyncSuppression();
    return swallow;
}

void InputBridge::SyncSuppression() {
    if (session_.Mode() != CaptureMode::Active) {
        RemoveSuppression();
        ReleaseCursorLock();
        return;
    }

    if (!keyHook_) {
        keyHook_ = SetWindowsHookExW(WH_KEYBOARD_LL, &InputBridge::KeyboardHookProc, GetModuleHandleW(nullptr), 0);
    }
    if (!mouseHook_) {
        mouseHook_ = SetWindowsHookExW(WH_MOUSE_LL, &InputBridge::MouseHookProc, GetModuleHandleW(nullptr), 0);
    }
    suppressionInstalled_.store(keyHook_ != nullptr && mouseHook_ != nullptr);
    ApplyCursorLock(lastForeground_);
}

void InputBridge::RemoveSuppression() {
    if (keyHook_) {
        UnhookWindowsHookEx(keyHook_);
        keyHook_ = nullptr;
    }
    if (mouseHook_) {
        UnhookWindowsHookEx(mouseHook_);
        mouseHook_ = nullptr;
    }
    suppressionInstalled_.store(false);
    swallowed_.Clear();
}

void InputBridge::ApplyCursorLock(HWND window) {
    RECT rect{};
    if (!window || !GetWindowRect(window, &rect)) {
        ReleaseCursorLock();
        return;
    }
    if (!clipped_ || !EqualRect(&rect, &clipRect_)) {
        if (ClipCursor(&rect)) {
            clipRect_ = rect;
            clipped_ = true;
        }
    }
    if (!cursorHidden_) {
        ShowCursor(FALSE);
        cursorHidden_ = true;
    }
}

void InputBridge::ReleaseCursorLock() {
    if (clipped_) {
        ClipCursor(nullptr);
        clipped_ = false;
        clipRect_ = {};
    }
    if (cursorHidden_) {
        ShowCursor(TRUE);
        cursorHidden_ = false;
    }
}

}  // namespace pskb
